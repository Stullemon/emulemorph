#include "stdafx.h"
#include "resource.h"
#include "SR13-ImportParts.h"

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

	uint16 partsuccess=0;
	uint16 badpartsuccess=0;

	if(!IsPartFile()){
		LogError(LOG_STATUSBAR, _T("Import aborted: Can't import to a complete file."));
		return false;
	}

	CPartFile* partfile=(CPartFile*)this;

	uint16 partcount=GetPartCount();

	if (GetHashCount() < partcount){
	    LogError(LOG_STATUSBAR, _T("Import aborted: hashset incomplete for %s (%i hashes for %i parts)."), GetFileName(), GetHashCount(), partcount); // do not try to import to files without hashset.
		return false;
	}

	CString buffer=_T("*");
	CFileDialog dlg(true, _T("*.*"), buffer, OFN_FILEMUSTEXIST|OFN_HIDEREADONLY);
	if(dlg.DoModal()!=IDOK)
		return false;
	CString pathName=dlg.GetFileName();

	CFile f;
	if(!f.Open(pathName, CFile::modeRead)){
		LogError(LOG_STATUSBAR, _T("Import aborted: Couldn't open %s."), pathName.GetBuffer());
		return false;
	}

	uint32 fileSize=f.GetLength();
	uchar *partData=new uchar[PARTSIZE];
	uint32 partSize;

	Log(LOG_STATUSBAR, _T("Importing %i parts (from 0 to %i) to %s."), partcount+1, partcount,  GetFileName());

	for(uint16 part=0;part<partcount;part++){
		// File for import is shorter than partfile and we reached its end. Shouldn't
		// happen(?), because of check at end of for block, but just in case I mess code
		// up somewhere. --Rowaa[SR13]
		if((uint32)(part*PARTSIZE)>fileSize)
			break;

		if(partfile->IsComplete(part*PARTSIZE, part*PARTSIZE+PARTSIZE-1)){
			Log(LOG_STATUSBAR, _T("Part %i: skipped, already complete."), part);
			continue;
		}

		f.Seek(part*PARTSIZE,CFile::begin);
		partSize=f.Read(partData, PARTSIZE);

		if(partSize==0)
			break;

		uchar hash[16];
		CreateHash(partData, partSize, hash);

		if(md4cmp(hash, partfile->GetPartHash(part))==0){
			partfile->WriteToBuffer(partSize-1, partData, part*PARTSIZE, part*PARTSIZE+partSize-1, NULL, NULL);
			Log(LOG_STATUSBAR, _T("Part %i: imported."), part);
			partsuccess++;
		} else {
			if(partfile->IsPureGap(part*PARTSIZE, part*PARTSIZE+PARTSIZE-1)){
				partfile->WriteToBuffer(partSize-1, partData, part*PARTSIZE, part*PARTSIZE+partSize-1, NULL, NULL);
				LogWarning(LOG_STATUSBAR, _T("Part %i: imported for ICH handling."), part);
				badpartsuccess++;
			} else {
				Log(LOG_STATUSBAR, _T("Part %i: skipped, doesn't match partially completed block."), part);
			}
		}
		// If we've got less bytes than asked, that means it's end of file for import
		if(partSize!=PARTSIZE)
			break;
	}

	f.Close();
	delete partData;
	Log(LOG_STATUSBAR, _T("Import finished. %i parts imported to %s."), partsuccess, GetFileName());

	if(badpartsuccess>0){
		// Just a precaution. Never know where I can mess things up...
		LogWarning(LOG_STATUSBAR, _T("%i bad parts were imported for ICH. Initiating rehash."), badpartsuccess);
		SR13_InitiateRehash(partfile);
	}
	return true;
}

// Initiates partfile rehash
// Copied from PartFile.cpp
// Check periodically if it needs update
void SR13_InitiateRehash(CPartFile* partfile){
	CString strFileInfo;
	strFileInfo.Format(_T("%s (%s)"), partfile->GetFilePath(), partfile->GetFileName());
	LogWarning(LOG_STATUSBAR, GetResString(IDS_ERR_REHASH), strFileInfo);
	// rehash
	partfile->SetStatus(PS_WAITINGFORHASH);
	CAddFileThread* addfilethread = (CAddFileThread*) AfxBeginThread(RUNTIME_CLASS(CAddFileThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
	if (addfilethread){
		partfile->SetFileOp(PFOP_HASHING);
		partfile->SetFileOpProgress(0);
		addfilethread->SetValues(0, partfile->GetPath(), partfile->m_hpartfile.GetFileName(), partfile);
		addfilethread->ResumeThread();
	}
}
