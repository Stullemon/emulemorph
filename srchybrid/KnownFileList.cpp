//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include <io.h>
#include "emule.h"
#include "KnownFileList.h"
#include "KnownFile.h"
#include "opcodes.h"
#include "Preferences.h"
#include "SafeFile.h"
#include "OtherFunctions.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#endif
#include "DownloadQueue.h" //MORPH - Added by SiRoB

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define KNOWN_MET_FILENAME	_T("known.met")


CKnownFileList::CKnownFileList()
{
	accepted = 0;
	requested = 0;
	transferred = 0;
	m_nLastSaved = ::GetTickCount();
	Init();
}
//EastShare START - Added by TAHO, .met files control
CKnownFileList::CKnownFileList(CPreferences* in_prefs)
{
	app_prefs = in_prefs;
	accepted = 0;
	requested = 0;
	transferred = 0;
	m_nLastSaved = ::GetTickCount();
	Init();
}
//EastShare END - Added by TAHO, .met files control
CKnownFileList::~CKnownFileList()
{
	Clear();
}

bool CKnownFileList::Init()
{
	CString fullpath=theApp.glob_prefs->GetConfigDir();
	fullpath.Append(KNOWN_MET_FILENAME);
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(fullpath,CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(_T("Failed to load ") KNOWN_MET_FILENAME _T(" file"));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			AddLogLine(true, _T("%s"), strError);
		}
		return false;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

	CSingleLock sLock(&list_mut);
	try {
		uint8 header;
		file.Read(&header,1);
		if (header != MET_HEADER){
			file.Close();
			return false;
		}
		
		uint32 RecordsNumber;
		// EastShare START - Added by TAHO, .met file control
		uint32 cDeleted = 0; 
		uint32 cAdded = 0;
		uint32 ExpiredTime = time(NULL) - app_prefs->GetKnownMetDays()*86400;
		// EastShare END - Added by TAHO, .met file control
		file.Read(&RecordsNumber,4);
		sLock.Lock();
		for (uint32 i = 0; i < RecordsNumber; i++) {
			CKnownFile* pRecord =  new CKnownFile();
			if (!pRecord->LoadFromFile(&file)){
				TRACE("*** Failed to load entry %u (name=%s  hash=%s  size=%u  parthashs=%u expected parthashs=%u) from known.met\n", i, 
					pRecord->GetFileName(), md4str(pRecord->GetFileHash()), pRecord->GetFileSize(), pRecord->GetHashCount(), pRecord->GetED2KPartCount());	// SLUGFILLER: SafeHash - removed unnececery hash counter
				delete pRecord;
				continue;
			}
			// EastShare START - Modified by TAHO, .met file control
			//Add(pRecord);
			if ( app_prefs->GetKnownMetDays() == 0 || pRecord->statistic.GetLastUsed() > ExpiredTime )
			{
			Add(pRecord);
				cAdded++;
			}
			else
			{
				cDeleted++;
			}
			// EastShare END - Modified by TAHO, .met file control
		}
		sLock.Unlock();
		file.Close();
		AddLogLine(false, "known.met loaded, %i files are known, %i files are deleted.", cAdded, cDeleted); // EastShare - Added by TAHO, .met file control
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			AddLogLine(true,GetResString(IDS_ERR_SERVERMET_BAD));
		else{
			char buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,MAX_CFEXP_ERRORMSG);
			AddLogLine(true,GetResString(IDS_ERR_SERVERMET_UNKNOWN),buffer);
		}
		error->Delete();
		return false;
	}

	return true;
}

void CKnownFileList::Save()
{
	if (theApp.glob_prefs->GetLogFileSaving())
	DEBUG_ONLY(AddDebugLogLine(false, "Saved KnownFileList"));
	m_nLastSaved = ::GetTickCount(); 
	CString fullpath=theApp.glob_prefs->GetConfigDir();
	fullpath += KNOWN_MET_FILENAME;
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(fullpath, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary, &fexp)){
		CString strError(_T("Failed to save ") KNOWN_MET_FILENAME _T(" file"));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(true, _T("%s"), strError);
		return;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

	CSingleLock sLock(&list_mut);
	try{
		uint8 ucHeader = MET_HEADER;
		file.Write(&ucHeader, 1);

		uint32 RecordsNumber = GetCount();
		// SLUGFILLER: mergeKnown - add part files count
		uint32 RecordsNumberWithPartFiles = RecordsNumber + theApp.downloadqueue->GetPartFilesCount();
		file.Write(&RecordsNumberWithPartFiles, 4);
		// SLUGFILLER: mergeKnown
		
		// save known files
		for (uint32 i = 0; i < RecordsNumber; i++)
			ElementAt(i)->WriteToFile(&file);
		theApp.downloadqueue->SavePartFilesToKnown(&file);	// SLUGFILLER: mergeKnown - add part files
		if (theApp.glob_prefs->GetCommitFiles() >= 2 || (theApp.glob_prefs->GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			file.Flush(); // flush file stream buffers to disk buffers
			if (_commit(_fileno(file.m_pStream)) != 0) // commit disk buffers to disk
				AfxThrowFileException(CFileException::hardIO, GetLastError(), file.GetFileName());
		}
		file.Close();
	}
	catch(CFileException* error){
		CString strError(_T("Failed to save ") KNOWN_MET_FILENAME _T(" file"));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (error->GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(true, _T("%s"), strError);
		error->Delete();
	}
}

void CKnownFileList::Clear()
{
	for (int i = 0; i < GetSize();i++)
		safe_delete(this->ElementAt(i));
	RemoveAll();
	SetSize(0);
}

CKnownFile* CKnownFileList::FindKnownFile(LPCTSTR filename,uint32 in_date,uint32 in_size)
{
	for (int i = 0;i < GetCount();i++){
		CKnownFile* cur_file = ElementAt(i);
		if (cur_file->GetFileDate() == in_date && cur_file->GetFileSize() == in_size && (!_tcscmp(filename,cur_file->GetFileName())))
			return cur_file;
	}
	return 0;
}

void CKnownFileList::SafeAddKFile(CKnownFile* toadd)
{
	CSingleLock sLock(&list_mut,true);
	Add(toadd);
	sLock.Unlock();
}

CKnownFile* CKnownFileList::FindKnownFileByID(const uchar* hash){
	for (int i = 0; i < GetCount(); i++){
		CKnownFile* pCurKnownFile = ElementAt(i);
		if (!md4cmp(pCurKnownFile->GetFileHash(), hash))
			return pCurKnownFile;
	}
	return NULL;
}

void CKnownFileList::Process()
{
	if ( ::GetTickCount() - m_nLastSaved > MIN2MS(11))
		this->Save();
}

bool CKnownFileList::IsKnownFile(void* pToTest){
	for (int i = 0; i < GetCount(); i++){
		if (ElementAt(i) == pToTest)
			return true;
	}
	return false;
}

// SLUGFILLER: mergeKnown
void CKnownFileList::RemoveFile(CKnownFile* toremove){
	for (int i = 0;i != this->GetCount();i++)
		if (ElementAt(i) == toremove){
			RemoveAt(i);
			return;
		}
}

void CKnownFileList::FilterDuplicateKnownFiles(CKnownFile* original){
	const uchar* filehash = original->GetFileHash();
	for (int i = 0;i != this->GetCount();){
		CKnownFile* other = ElementAt(i);
		if (other == original || md4cmp(filehash, other->GetFileHash())){
			i++;
			continue;
		}
		original->statistic.Merge(&other->statistic);
		RemoveAt(i);
		delete other;
	}
}
// SLUGFILLER: mergeKnown
