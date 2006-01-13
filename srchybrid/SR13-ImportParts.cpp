#include "stdafx.h"
#include "resource.h"
#include "SR13-ImportParts.h"
#include "emule.h"
#include "emuleDlg.h"
#include "Log.h"
#include "Opcodes.h"
#include "PartFile.h"
#include "SHAHashSet.h"
#include "SharedFileList.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef GetFileNameorInfo
#define GetFileNameorInfo(X)	X->GetFileName()
#endif

bool CKnownFile::SR13_ImportParts(){
	// General idea from xmrb's CKnownFile::ImportParts()
	// Unlike xmrb's version which scans entire file designated for import and then tries
	// to match each PARTSIZE bytes with all parts of partfile, my version assumes that
	// in file you're importing all parts stay on same place as they should be in partfile
	// (for example you're importing damaged version of file to recover some parts from ED2K)
	// That way it works much faster and almost always it is what expected from this
	// function. --Rowaa[SR13].

	if(!IsPartFile()){
		LogError(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_ERR_ALREADYCOMPLETE));
		return false;
	}

	CPartFile* partfile=(CPartFile*)this;

	// Disallow files without hashset, unless it is one part long file
	if (GetPartCount() != 1 && GetHashCount() < GetPartCount()){
		LogError(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_ERR_HASHSETINCOMPLETE), GetFileNameorInfo(partfile), GetHashCount(), GetPartCount()); // do not try to import to files without hashset.
		return false;
	}

	// Disallow very small files
	// Maybe I make them allowed in future when I make my program insert slices in partially downloaded parts.
	if (GetFileSize() < EMBLOCKSIZE){
		LogError(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_ERR_FILETOOSMALL)); // do not try to import to files without hashset.
		return false;
	}

	CFileDialog dlg(true, NULL, NULL, OFN_FILEMUSTEXIST|OFN_HIDEREADONLY);
	if(dlg.DoModal()!=IDOK)
		return false;
	CString pathName=dlg.GetPathName();

	partfile->SetStatus(PS_COMPLETING);
	CAddFileThread* addfilethread = (CAddFileThread*) AfxBeginThread(RUNTIME_CLASS(CAddFileThread), THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
	if (addfilethread){
		partfile->SetFileOp(PFOP_SR13_IMPORTPARTS);
		partfile->SetFileOpProgress(0);
        addfilethread->SetValues(0, partfile->GetPath(), partfile->m_hpartfile.GetFileName(), partfile, pathName);
		addfilethread->ResumeThread();
	}
	return true;
}

// Special case for SR13-ImportParts
void CAddFileThread::SetValues(CSharedFileList* pOwner, LPCTSTR directory, LPCTSTR filename, CPartFile* partfile, LPCTSTR import)
{
	 m_pOwner = pOwner;
	 m_strDirectory = directory;
	 m_strFilename = filename;
	 m_partfile = partfile;
	 m_strImport = import;
}

inline uint32 WriteToPartFile(CPartFile *partfile, const BYTE *data, uint32 start, uint32 end){
	uint32 result;
	partfile->FlushBuffer(true);
	result=partfile->WriteToBuffer(end-start, data, start, end, NULL, NULL);
	partfile->FlushBuffer(true);
	return result;
}

bool SR13_ImportParts(CPartFile* partfile, CString strFilePath){

	uint16 partsuccess=0;
	uint16 badpartsuccess=0;
	uint16 partcount=partfile->GetPartCount();

	CFile f;
	if(!f.Open(strFilePath, CFile::modeRead)){
		LogError(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_ERR_CANTOPENFILE), strFilePath);
		return false;
	}

	uint32 fileSize=f.GetLength();
	uchar *partData=new uchar[PARTSIZE];
	uint32 partSize;
	uint32 part;
	uchar hash[16];
	CKnownFile *kfcall=new CKnownFile;

	Log(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_IMPORTSTART), partcount+1, partcount, partfile->GetFileName());

	partfile->FlushBuffer(true);

	for(part=0; part<partcount; part++){
		// in case of shutdown while still importing
		if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning()){
			f.Close();
			delete partData;
			delete kfcall;
			return false;
		}

		if(part*PARTSIZE>fileSize)
			break;

		if (theApp.emuledlg && theApp.emuledlg->IsRunning()){
			UINT uProgress = ((ULONGLONG)(part*PARTSIZE) * 100) / fileSize;
			VERIFY(PostMessage(theApp.emuledlg->GetSafeHwnd(), TM_FILEOPPROGRESS, uProgress, (LPARAM)partfile));
		}

		if(partfile->IsComplete(part*PARTSIZE, part*PARTSIZE+PARTSIZE-1, true)){
			Log(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_PARTSKIPPEDALREADYCOMPLETE), part);
			continue;
		}

		try {
			f.Seek(part*PARTSIZE, CFile::begin);
            partSize=f.Read(partData, PARTSIZE);
		} catch (...) {
			LogWarning(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_PARTIMPORTEDBAD), part);
			continue;
		}
		if(partSize==0)
			break;

		kfcall->CreateHash(partData, partSize, hash);

		if(md4cmp(hash, (partcount==1?partfile->GetFileHash():partfile->GetPartHash(part)))==0){
			WriteToPartFile(partfile, partData, part*PARTSIZE, part*PARTSIZE+partSize-1);
			Log(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_PARTIMPORTEDGOOD), part);
			partsuccess++;
		} else {
			if(partfile->IsPureGap(part*PARTSIZE, part*PARTSIZE+PARTSIZE-1)){
				WriteToPartFile(partfile, partData, part*PARTSIZE, part*PARTSIZE+partSize-1);
				LogWarning(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_PARTIMPORTEDBAD), part);
				badpartsuccess++;
			} else {
				Log(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_PARTSKIPPEDDONTMATCH), part);
			}
		}
		if(partSize!=PARTSIZE)
			break;
	}

	f.Close();
	delete partData;
	delete kfcall;
	
	Log(LOG_STATUSBAR, _T("Import finished. %i parts imported to %s."), partsuccess, GetFileNameorInfo(partfile));
	
	if(badpartsuccess>0){
		switch(partfile->GetAICHHashset()->GetStatus()){
		case AICH_TRUSTED:
		case AICH_VERIFIED:
		case AICH_HASHSETCOMPLETE:
			Log(LOG_STATUSBAR, _T("AICH hash available. Advanced recovery pending."));
			break;
		default:
			Log(LOG_STATUSBAR, _T("No AICH hash available. You may want to import file data once again latter."));
		}
	}
	return true;
}

// Initiates partfile rehash
// Copied from PartFile.cpp
// Check periodically if it needs update
// No longer used by ImportParts... Maybe I'll remove it in next version
void SR13_InitiateRehash(CPartFile* partfile){
	CString strFileInfo;
	strFileInfo.Format(_T("%s (%s)"), partfile->GetFilePath(), partfile->GetFileName());
	LogWarning(LOG_STATUSBAR, GetResString(IDS_ERR_REHASH), strFileInfo);
	// rehash
	partfile->SetStatus(PS_WAITINGFORHASH);
	CAddFileThread* addfilethread = (CAddFileThread*) AfxBeginThread(RUNTIME_CLASS(CAddFileThread), THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
	if (addfilethread){
		partfile->SetFileOp(PFOP_HASHING);
		partfile->SetFileOpProgress(0);
		addfilethread->SetValues(0, partfile->GetPath(), partfile->GetFileName(), partfile);
		addfilethread->ResumeThread();
	}
}
