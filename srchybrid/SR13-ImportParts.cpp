#include "stdafx.h"
#include "resource.h"
#include "SR13-ImportParts.h"
#include "emule.h"
#include "emuleDlg.h"
#include "Log.h"
#include "PartFile.h"
#include "SHAHashSet.h"
#include "SharedFileList.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

bool CKnownFile::SR13_ImportParts(){
	// General idea from xmrb's CKnownFile::ImportParts()
	// Unlike xmrb's version which scans entire file designated for import and then tries
	// to match each PARTSIZE bytes with all parts of partfile, my version assumes that
	// in file you're importing all parts stay on same place as they should be in partfile
	// (for example you're importing damaged version of file to recover some parts from ED2K)
	// That way it works much faster and almost always it is what expected from this
	// function. --Rowaa[SR13].
	// Current threading code is done by SiRoB of Morph MOD.

	// Very crude locking, will be replaced with something much better in next version.
	static bool inprogress=false;	

	if(inprogress){
		LogError(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_ERR_ALREADYINPROGRESS));
		return false;
	}

	if(!IsPartFile()){
		LogError(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_ERR_ALREADYCOMPLETE));
		return false;
	}

	CPartFile* partfile=(CPartFile*)this;

	uint16 partcount=GetPartCount();

	if (GetHashCount() < partcount){
		LogError(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_ERR_HASHSETINCOMPLETE), GetFileNameorInfo(partfile), GetHashCount(), partcount); // do not try to import to files without hashset.
		return false;
	}

	CString buffer=_T("*");
	CFileDialog dlg(true, _T("*.*"), buffer, OFN_FILEMUSTEXIST|OFN_HIDEREADONLY);
	if(dlg.DoModal()!=IDOK)
		return false;
	CString pathName=dlg.GetFileName();

	CImportPartsFileThread* importpartsfilethread = (CImportPartsFileThread*) AfxBeginThread(RUNTIME_CLASS(CImportPartsFileThread), THREAD_PRIORITY_BELOW_NORMAL,0, CREATE_SUSPENDED);
	if (importpartsfilethread){
		partfile->SetFileOp(PFOP_SR13_IMPORTPARTS);
		partfile->SetFileOpProgress(0);
        importpartsfilethread->SetValues(pathName,partfile,&inprogress);
		inprogress=true;
		importpartsfilethread->ResumeThread();
	}
	return true;
}

IMPLEMENT_DYNCREATE(CImportPartsFileThread, CWinThread);

CImportPartsFileThread::CImportPartsFileThread(){
	m_partfile = NULL;
}

BOOL CImportPartsFileThread::InitInstance(){
	InitThreadLocale();
	return TRUE;
}

void CImportPartsFileThread::SetValues(LPCTSTR filepath, CPartFile* partfile, bool *inrogress){
	 m_strFilePath = filepath;
	 m_partfile = partfile;
	 bInProgress = inrogress;
}

int CImportPartsFileThread::Run(){
	DbgSetThreadName("Importing parts to %s", m_strFilePath);
	if ( !m_partfile || !m_strFilePath || !theApp.emuledlg->IsRunning()){
		*bInProgress = false;
		return 0;
	}

	uint16 partsuccess=0;
	uint16 badpartsuccess=0;
	uint16 partcount=m_partfile->GetPartCount();

	CFile f;
	if(!f.Open(m_strFilePath, CFile::modeRead|CFile::shareDenyNone)){
		theApp.QueueLogLineEx(LOG_STATUSBAR|LOG_ERROR, GetResString(IDS_SR13_IMPORTPARTS_ERR_CANTOPENFILE), m_strFilePath);
		*bInProgress = false;
		return false;
	}

	uint32 fileSize=f.GetLength();
	uchar *partData=new uchar[PARTSIZE];
	uint32 partSize;
	uint32 part;
	uchar hash[16];
	CKnownFile *kfcall=new CKnownFile;

	theApp.QueueLogLineEx(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_IMPORTSTART), partcount+1, partcount, m_partfile->GetFileName());

	for(part=0; part<partcount; part++){
		// File for import is shorter than partfile and we reached its end. Shouldn't
		// happen(?), because of check at end of for block, but just in case I mess code
		// up somewhere. --Rowaa[SR13]
		if(part*PARTSIZE>fileSize)
			break;

		if (theApp.emuledlg && theApp.emuledlg->IsRunning()){
			UINT uProgress = ((ULONGLONG)(part*PARTSIZE) * 100) / m_partfile->GetFileSize();
			VERIFY(PostMessage(theApp.emuledlg->GetSafeHwnd(), TM_FILEOPPROGRESS, uProgress, (LPARAM)m_partfile));
		}

		if(m_partfile->IsComplete(part*PARTSIZE, part*PARTSIZE+PARTSIZE-1)){
			theApp.QueueLogLineEx(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_PARTSKIPPEDALREADYCOMPLETE), part);
			continue;
		}

		f.Seek(part*PARTSIZE,CFile::begin);
		partSize=f.Read(partData, PARTSIZE);

		if(partSize==0)
			break;

		kfcall->CreateHash(partData, partSize, hash);

		if(md4cmp(hash, m_partfile->GetPartHash(part))==0){
			m_partfile->WriteToBuffer(partSize-1, partData, part*PARTSIZE, part*PARTSIZE+partSize-1, NULL, NULL);
			//m_partfile->FlushBuffer(false);
			theApp.QueueLogLineEx(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_PARTIMPORTEDGOOD), part);
			partsuccess++;
		} else {
			if(m_partfile->IsPureGap(part*PARTSIZE, part*PARTSIZE+PARTSIZE-1)){
				m_partfile->WriteToBuffer(partSize-1, partData, part*PARTSIZE, part*PARTSIZE+partSize-1, NULL, NULL);
				//m_partfile->FlushBuffer(false);
				theApp.QueueLogLineEx(LOG_STATUSBAR|LOG_WARNING, GetResString(IDS_SR13_IMPORTPARTS_PARTIMPORTEDBAD), part);
				badpartsuccess++;
			} else {
				theApp.QueueLogLineEx(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_PARTSKIPPEDDONTMATCH), part);
			}
		}
		// If we've got less bytes than asked, that means it's end of file for import
		if(partSize!=PARTSIZE)
			break;
	}

	f.Close();
	delete partData;
	delete kfcall;

	Log(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_IMPORTFINISH), partsuccess, m_partfile->GetFileName());

	if(badpartsuccess>0){
		// Just a precaution. Never know where I can mess things up...
		LogWarning(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_BADPARTSREHASH), badpartsuccess);
		SR13_InitiateRehash(m_partfile);
	}
	*bInProgress = false;
	return 0;
}

// Initiates partfile rehash
// Copied from PartFile.cpp
// Check periodically if it needs update
void SR13_InitiateRehash(CPartFile* partfile){
	CString strFileInfo;
	strFileInfo.Format(_T("%s (%s)"), partfile->GetFilePath(), partfile->GetFileName());
	LogWarning(LOG_STATUSBAR, GetResString(IDS_ERR_REHASH), strFileInfo);
	// rehash
	//partfile->SetStatus(PS_WAITINGFORHASH);
	CAddFileThread* addfilethread = (CAddFileThread*) AfxBeginThread(RUNTIME_CLASS(CAddFileThread), THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
	if (addfilethread){
		partfile->SetFileOp(PFOP_HASHING);
		partfile->SetFileOpProgress(0);
		addfilethread->SetValues(theApp.sharedfiles, partfile->GetPath(), partfile->m_hpartfile.GetFileName(), partfile);
		addfilethread->ResumeThread();
	}
}
