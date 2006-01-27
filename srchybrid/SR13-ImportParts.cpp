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

bool CKnownFile::SR13_ImportParts(){
	// General idea from xmrb's CKnownFile::ImportParts()
	// Unlike xmrb's version which scans entire file designated for import and then tries
	// to match each PARTSIZE bytes with all parts of partfile, my version assumes that
	// in file you're importing all parts stay on same place as they should be in partfile
	// (for example you're importing damaged version of file to recover some parts from ED2K)
	// That way it works much faster and almost always it is what expected from this
	// function. --Rowaa[SR13].
	// CHANGE BY SIROB, Only comput missing full chunk part
	if(!IsPartFile()){
		LogError(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_ERR_ALREADYCOMPLETE));
		return false;
	}

	CPartFile* partfile=(CPartFile*)this;
	if (partfile->GetFileOp() == PFOP_SR13_IMPORTPARTS) {
		partfile->SetFileOp(PFOP_NONE);
		return false;
	}
	// Disallow files without hashset, unless it is one part long file
	if (GetPartCount() != 1 && GetHashCount() < GetPartCount()){
		LogError(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_ERR_HASHSETINCOMPLETE), GetFileName(), GetHashCount(), GetPartCount()); // do not try to import to files without hashset.
		return false;
	}

	// Disallow very small files
	// Maybe I make them allowed in future when I make my program insert slices in partially downloaded parts.
	if (GetFileSize() < (uint64)EMBLOCKSIZE){
		LogError(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_ERR_FILETOOSMALL)); // do not try to import to files without hashset.
		return false;
	}

	CFileDialog dlg(true, NULL, NULL, OFN_FILEMUSTEXIST|OFN_HIDEREADONLY);
	if(dlg.DoModal()!=IDOK)
		return false;
	CString pathName=dlg.GetPathName();

	CAddFileThread* addfilethread = (CAddFileThread*) AfxBeginThread(RUNTIME_CLASS(CAddFileThread), THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
	if (addfilethread){
		partfile->SetFileOpProgress(0);
        addfilethread->SetValues(theApp.sharedfiles, partfile->GetPath(), partfile->m_hpartfile.GetFileName(), partfile);
		if (addfilethread->SetPartToImport(pathName))
			partfile->SetFileOp(PFOP_SR13_IMPORTPARTS);
		else
			partfile->SetFileOp(PFOP_HASHING);
		addfilethread->ResumeThread();
	}
	return true;
}

inline uint32 WriteToPartFile(CPartFile *partfile, const BYTE *data, uint64 start, uint64 end){
	uint32 result;
	result=partfile->WriteToBuffer(end-start+1, data, start, end, NULL, NULL);
	return result;
}

// Special case for SR13-ImportParts
uint16 CAddFileThread::SetPartToImport(LPCTSTR import)
{
	if (m_partfile->GetFilePath() == import) {
		return 0;
	}
	m_strImport = import;

	for (int i = 0; i < m_partfile->GetPartCount(); i++)
		if (m_partfile->IsPureGap((uint64)i*PARTSIZE,(uint64)(i+1)*PARTSIZE-1)){
			uchar* cur_hash = new uchar[16];
			md4cpy(cur_hash, m_partfile->GetPartHash(i));

			m_PartsToImport.Add(i);
			m_DesiredHashes.Add(cur_hash);
		}
	return (uint16)m_PartsToImport.GetSize();
}

bool CAddFileThread::SR13_ImportParts(){

	uint16 partsuccess=0;
	uint16 badpartsuccess=0;
	
	CFile f;
	if(!f.Open(m_strImport, CFile::modeRead  | CFile::shareDenyWrite)){
		LogError(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_ERR_CANTOPENFILE), m_strImport);
		return false;
	}

	uint64 fileSize=f.GetLength();
	uchar *partData=new uchar[PARTSIZE];
	uint32 partSize;
	uchar hash[16];
	CKnownFile *kfcall=new CKnownFile;

	CString strFilePath;
	_tmakepath(strFilePath.GetBuffer(MAX_PATH), NULL, m_strDirectory, m_strFilename, NULL);
	strFilePath.ReleaseBuffer();
	
	Log(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_IMPORTSTART), m_PartsToImport.GetSize(), strFilePath);

	try {
		for (UINT i = 0; i < (UINT)m_PartsToImport.GetSize(); i++){
			uint16 partnumber = m_PartsToImport[i];
			m_partfile->FlushBuffer(true);
			try {
				if (PARTSIZE*partnumber > fileSize)
					break;
				f.Seek((LONGLONG)PARTSIZE*partnumber,0);
				partSize=f.Read(partData, PARTSIZE);
			} catch (...) {
				LogWarning(LOG_STATUSBAR, _T("Part %i: Not accessible (You may have to run scandisk to correct a bad cluster on your harddisk)."), partnumber);
				continue;
			}
			kfcall->CreateHash(partData, partSize, hash);
		
			if (md4cmp(hash,m_DesiredHashes[i])==0){
				WriteToPartFile(m_partfile, partData, (uint64)partnumber*PARTSIZE, (uint64)partnumber*PARTSIZE+partSize-1);
				//Log(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_PARTIMPORTEDGOOD), partnumber);
				partsuccess++;	
			} else if(md4cmp(hash,L"\xD7\xDE\xF2\x62\xA1\x27\xCD\x79\x09\x6A\x10\x8E\x7A\x9F\xC1\x38")){
				WriteToPartFile(m_partfile, partData, (uint64)partnumber*PARTSIZE, (uint64)partnumber*PARTSIZE+partSize-1);
				//LogWarning(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_PARTIMPORTEDBAD), partnumber);
				badpartsuccess++;
			} else {
				//Log(LOG_STATUSBAR, GetResString(IDS_SR13_IMPORTPARTS_PARTSKIPPEDDONTMATCH), partnumber);
			}
			if (theApp.emuledlg->IsRunning()){
				UINT uProgress = (UINT)(i * 100 / m_DesiredHashes.GetSize());
				VERIFY( PostMessage(theApp.emuledlg->GetSafeHwnd(), TM_FILEOPPROGRESS, uProgress, (LPARAM)m_partfile) );
		}
			if(partSize!=PARTSIZE || m_partfile->GetFileOp() != PFOP_SR13_IMPORTPARTS)
			break;
		}
		if (m_partfile->GetFileOp()!= PFOP_SR13_IMPORTPARTS)
			Log(LOG_STATUSBAR, _T("Import aborted. %i parts imported to %s."), partsuccess, m_strFilename);
		else
			Log(LOG_STATUSBAR, _T("Import finished. %i parts imported to %s."), partsuccess, m_strFilename);
		
		if(badpartsuccess>0){
			switch(m_partfile->GetAICHHashset()->GetStatus()){
			case AICH_TRUSTED:
			case AICH_VERIFIED:
			case AICH_HASHSETCOMPLETE:
				Log(LOG_STATUSBAR, _T("AICH hash available. Advanced recovery pending."));
				break;
			default:
				Log(LOG_STATUSBAR, _T("No AICH hash available. You may want to import file data once again latter."));
			}
		}
	} catch (...) {
		//Could occure when partfile instance has been canceled
	}
	for (UINT i = 0; i < (UINT)m_DesiredHashes.GetSize(); i++)
		delete m_DesiredHashes[i];

	f.Close();
	delete partData;
	delete kfcall;

	return true;
}
