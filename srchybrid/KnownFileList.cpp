//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "KnownFile.h"
#include "opcodes.h"
#include "Preferences.h"
#include "SafeFile.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
#include "DownloadQueue.h"
#include "emuledlg.h"
#include "TransferWnd.h"
#include "Log.h"
#include "packets.h"
#include "DownloadQueue.h" //MORPH - Added by SiRoB

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


#define KNOWN_MET_FILENAME	_T("known.met")
#define CANCELLED_MET_FILENAME	_T("cancelled.met")

CKnownFileList::CKnownFileList()
{
	m_Files_map.InitHashTable(2063);
	m_mapCancelledFiles.InitHashTable(1031);
	accepted = 0;
	requested = 0;
	transferred = 0;
	m_nLastSaved = ::GetTickCount();
	Init();
}

CKnownFileList::~CKnownFileList()
{
	Clear();
}

bool CKnownFileList::Init()
{
	return LoadKnownFiles() && LoadCancelledFiles();
}

bool CKnownFileList::LoadKnownFiles(){
	CString fullpath=thePrefs.GetConfigDir();
	fullpath.Append(KNOWN_MET_FILENAME);
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(fullpath,CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(GetResString(IDS_FAILEDTOLOAD)+_T(" ") KNOWN_MET_FILENAME _T(" ")+GetResString(IDS_FILE)); // Localized by FrankyFive
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			LogError(LOG_STATUSBAR, _T("%s"), strError);
		}
		return false;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

	CKnownFile* pRecord = NULL;
	try {
		uint8 header = file.ReadUInt8();
		if (header != MET_HEADER && header != MET_HEADER_I64TAGS){
			file.Close();
			return false;
		}
		AddDebugLogLine(false, _T("Known.met file version is %u (%s support 64bit tags)"), header, (header == MET_HEADER) ? _T("doesn't") : _T("does")); 

		UINT RecordsNumber = file.ReadUInt32();
		//Morph Start - added by AndCycle, minor tweak - prime
		if(RecordsNumber*1.5 > 2063){
			m_Files_map.InitHashTable(getPrime((int)(RecordsNumber*1.5)));
		}
		//Morph End - added by AndCycle, minor tweak - prime
		for (UINT i = 0; i < RecordsNumber; i++) {
			pRecord = new CKnownFile();
			if (!pRecord->LoadFromFile(&file)){
				TRACE(_T("*** Failed to load entry %u (name=%s  hash=%s  size=%I64u  parthashs=%u expected parthashs=%u) from known.met\n"), i, 
					pRecord->GetFileName(), md4str(pRecord->GetFileHash()), pRecord->GetFileSize(), pRecord->GetHashCount(), pRecord->GetED2KPartCount());	// SLUGFILLER: SafeHash - removed unnececery hash counter
				delete pRecord;
				pRecord = NULL;
				continue;
			}
			SafeAddKFile(pRecord);
			pRecord = NULL;
		}
		file.Close();
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_SERVERMET_BAD));
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, ARRSIZE(buffer));
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_SERVERMET_UNKNOWN),buffer);
		}
		error->Delete();
		delete pRecord;
		return false;
	}

	return true;
}

bool CKnownFileList::LoadCancelledFiles(){
	if (!thePrefs.IsRememberingCancelledFiles())
		return true;
	CString fullpath=thePrefs.GetConfigDir();
	fullpath.Append(CANCELLED_MET_FILENAME);
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(fullpath,CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(_T("Failed to load ") CANCELLED_MET_FILENAME _T(" file"));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			LogError(LOG_STATUSBAR, _T("%s"), strError);
		}
		return false;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
	uchar ucHash[16];
	try {
		uint8 header = file.ReadUInt8();
		if (header != MET_HEADER){
			file.Close();
			return false;
		}

		UINT RecordsNumber = file.ReadUInt32();
		for (UINT i = 0; i < RecordsNumber; i++) {
			file.ReadHash16(ucHash);
			uint8 nCount = file.ReadUInt8();
			// for compatibility with future versions which may add more data than just the hash
			for (UINT j = 0; j < nCount; j++) {
				CTag tag(&file, false);
			}
			m_mapCancelledFiles.SetAt(CSKey(ucHash), 1);
		}
		file.Close();
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_CONFIGCORRUPT), CANCELLED_MET_FILENAME);
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, ARRSIZE(buffer));
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_FAILEDTOLOAD), CANCELLED_MET_FILENAME, buffer);
		}
		error->Delete();
		return false;
	}
	return true;
}

void CKnownFileList::Save()
{
	if (thePrefs.GetLogFileSaving())
		AddDebugLogLine(false, _T("Saving known files list file \"%s\""), KNOWN_MET_FILENAME);
	m_nLastSaved = ::GetTickCount(); 
	CString fullpath=thePrefs.GetConfigDir();
	fullpath += KNOWN_MET_FILENAME;
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(fullpath, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		CString strError(_T("Failed to save ") KNOWN_MET_FILENAME _T(" file"));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
	}
	else{
		setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

		try{
			file.WriteUInt8(0); // we will write the version tag later depending if any large files are on the list
			UINT nRecordsNumber = 0;
			bool bContainsAnyLargeFiles = false;
			file.WriteUInt32(nRecordsNumber);
			POSITION pos = m_Files_map.GetStartPosition();
			// SLUGFILLER: mergeKnown, for TAHO, .met file control
			const uint32 dwExpired = time(NULL) - (thePrefs.GetKnownMetDays() == 0 ? 12960000 : thePrefs.GetKnownMetDays()*86400);	// Morph - modified by AndCycle, .met file control
			while( pos != NULL )
			{
				CKnownFile* pFile;
				CCKey key;
				m_Files_map.GetNextAssoc( pos, key, pFile );
				if (!thePrefs.IsRememberingDownloadedFiles() && !theApp.sharedfiles->IsFilePtrInList(pFile)){
						continue;
					}
				else{
					if (theApp.sharedfiles->GetFileByID(pFile->GetFileHash()) == pFile)
						pFile->SetLastSeen();
					if (pFile->GetLastSeen() >= dwExpired) {
						pFile->WriteToFile(&file);
						nRecordsNumber++;
					}
					if (pFile->IsLargeFile())
						bContainsAnyLargeFiles = true;
				}
			}
			file.SeekToBegin();
			file.WriteUInt8(bContainsAnyLargeFiles ? MET_HEADER_I64TAGS : MET_HEADER);
			file.WriteUInt32(nRecordsNumber);

			if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
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
			LogError(LOG_STATUSBAR, _T("%s"), strError);
			error->Delete();
		}
	}


	if (thePrefs.GetLogFileSaving())
		AddDebugLogLine(false, _T("Saving known files list file \"%s\""), CANCELLED_MET_FILENAME);
 	fullpath=thePrefs.GetConfigDir();
	fullpath += CANCELLED_MET_FILENAME;
	if (!file.Open(fullpath, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary|CFile::shareDenyWrite, &fexp)){
		CString strError(_T("Failed to save ") CANCELLED_MET_FILENAME _T(" file"));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
	}
	else{
		setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

		try{
			file.WriteUInt8(MET_HEADER);
			if (!thePrefs.IsRememberingCancelledFiles()){
				file.WriteUInt32(0);
			}
			else{
				UINT nRecordsNumber = m_mapCancelledFiles.GetCount();
				file.WriteUInt32(nRecordsNumber);
				POSITION pos = m_mapCancelledFiles.GetStartPosition();
				while( pos != NULL )
				{
					int dwDummy;
					CSKey key;
					m_mapCancelledFiles.GetNextAssoc( pos, key, dwDummy );
					file.WriteHash16(key.m_key);
					file.WriteUInt8(0);
				}
			}

			if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
				file.Flush(); // flush file stream buffers to disk buffers
				if (_commit(_fileno(file.m_pStream)) != 0) // commit disk buffers to disk
					AfxThrowFileException(CFileException::hardIO, GetLastError(), file.GetFileName());
			}
			file.Close();
		}
		catch(CFileException* error){
			CString strError(_T("Failed to save ") CANCELLED_MET_FILENAME _T(" file"));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (error->GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			LogError(LOG_STATUSBAR, _T("%s"), strError);
			error->Delete();
		}
	}
}

void CKnownFileList::Clear()
{
	POSITION pos = m_Files_map.GetStartPosition();
	while( pos != NULL )
	{
		CKnownFile* pFile;
		CCKey key;
		m_Files_map.GetNextAssoc( pos, key, pFile );
	    delete pFile;
	}
	m_Files_map.RemoveAll();
}

void CKnownFileList::Process()
{
	if (::GetTickCount() - m_nLastSaved > MIN2MS(11))
		Save();
}

bool CKnownFileList::SafeAddKFile(CKnownFile* toadd)
{
	CCKey key(toadd->GetFileHash());
	CKnownFile* pFileInMap;
	if (m_Files_map.Lookup(key, pFileInMap))
	{
		TRACE(_T("%hs: File already in known file list: %s \"%s\" \"%s\"\n"), __FUNCTION__, md4str(pFileInMap->GetFileHash()), pFileInMap->GetFileName(), pFileInMap->GetFilePath());
		TRACE(_T("%hs: Old entry replaced with:         %s \"%s\" \"%s\"\n"), __FUNCTION__, md4str(toadd->GetFileHash()), toadd->GetFileName(), toadd->GetFilePath());
#if 1
		// if we hash files which are already in known file list and add them later (when the hashing thread is finished),
		// we can not delete any already available entry from known files list. that entry can already be used by the
		// shared file list -> crash.

		m_Files_map.RemoveKey(CCKey(pFileInMap->GetFileHash()));
		//This can happen in a couple situations..
		//File was renamed outside of eMule.. 
		//A user decided to redownload a file he has downloaded and unshared..
		//RemovingKeyWords I believe is not thread safe if I'm looking at this right.
		//Not sure of a good solution yet..
		if (theApp.sharedfiles)
		{
			theApp.sharedfiles->RemoveFile(pFileInMap);
			ASSERT( !theApp.sharedfiles->IsFilePtrInList(pFileInMap) );
		}
		//Double check to make sure this is the same file as it's possible that a two files have the same hash.
		//Maybe in the furture we can change the client to not just use Hash as a key throughout the entire client..
		ASSERT( toadd->GetFileSize() == pFileInMap->GetFileSize() );
		ASSERT( toadd != pFileInMap );
		if( toadd->GetFileSize() == pFileInMap->GetFileSize() )
			toadd->statistic.MergeFileStats( &pFileInMap->statistic );

		ASSERT( theApp.sharedfiles==NULL || !theApp.sharedfiles->IsFilePtrInList(pFileInMap) );
		ASSERT( theApp.downloadqueue==NULL || !theApp.downloadqueue->IsPartFile(pFileInMap) );

		// Quick fix: If we downloaded already downloaded files again and if those files all had the same file names
		// and were renamed during file completion, we have a pending ptr in transfer window.
		if (theApp.emuledlg && theApp.emuledlg->transferwnd && theApp.emuledlg->transferwnd->downloadlistctrl.m_hWnd)
			theApp.emuledlg->transferwnd->downloadlistctrl.RemoveFile((CPartFile*)pFileInMap);

		delete pFileInMap;
#else
		// if the new entry is already in list, update the stats and return false, but do not delete the entry which is
		// alreay in known file list!
		ASSERT( toadd->GetFileSize() == pFileInMap->GetFileSize() );
		ASSERT( toadd != pFileInMap );
		if (toadd->GetFileSize() == pFileInMap->GetFileSize() && toadd != pFileInMap)
		{
			pFileInMap->statistic.MergeFileStats(&toadd->statistic);
			pFileInMap->SetFileName(toadd->GetFileName(), false);
			pFileInMap->SetPath(toadd->GetPath());
			pFileInMap->SetFilePath(toadd->GetFilePath());
			pFileInMap->date = toadd->date;
		}
		ASSERT( !theApp.sharedfiles->IsFilePtrInList(pFileInMap) );
		ASSERT( theApp.sharedfiles->IsFilePtrInList(toadd) );
		return false;
#endif
	}
	m_Files_map.SetAt(key, toadd);
	return true;
}

CKnownFile* CKnownFileList::FindKnownFile(LPCTSTR filename, uint32 date, uint64 size) const
{
	POSITION pos = m_Files_map.GetStartPosition();
	while (pos != NULL)
	{
		CKnownFile* cur_file;
		CCKey key;
		m_Files_map.GetNextAssoc(pos, key, cur_file);
		if (cur_file->GetUtcFileDate() == date && cur_file->GetFileSize() == size && !_tcscmp(filename, cur_file->GetFileName()))
				return cur_file;
	}
	return NULL;
}

CKnownFile* CKnownFileList::FindKnownFileByPath(const CString& sFilePath) const
{
	POSITION pos = m_Files_map.GetStartPosition();
	while (pos != NULL)
	{
		CKnownFile* cur_file;
		CCKey key;
		m_Files_map.GetNextAssoc(pos, key, cur_file);
		if (!cur_file->GetFilePath().CompareNoCase(sFilePath))
			return cur_file;
	}
	return NULL;
}

CKnownFile* CKnownFileList::FindKnownFileByID(const uchar* hash) const
{
	if (hash)
	{
		CKnownFile* found_file;
		CCKey key(hash);
		if (m_Files_map.Lookup(key, found_file))
			return found_file;
	}
	return NULL;
}

// SLUGFILLER: mergeKnown
void CKnownFileList::MergePartFileStats(CKnownFile* original){
	CCKey key(original->GetFileHash());
	CKnownFile* pFileInMap;
	if (m_Files_map.Lookup(key, pFileInMap) && pFileInMap != original)
	{
		m_Files_map.RemoveKey(CCKey(pFileInMap->GetFileHash()));
		//MORPH - mergeKnown fix
		if (theApp.emuledlg && theApp.emuledlg->transferwnd && theApp.emuledlg->transferwnd->downloadlistctrl.m_hWnd)
			theApp.emuledlg->transferwnd->downloadlistctrl.RemoveFile((CPartFile*)pFileInMap);
		//MORPH - mergeKnown fix
		ASSERT( original->GetFileSize() == pFileInMap->GetFileSize() );
		if (original->GetFileSize() == pFileInMap->GetFileSize())
			original->statistic.MergeFileStats(&pFileInMap->statistic);

		delete pFileInMap;
	}
}
// SLUGFILLER: mergeKnown

bool CKnownFileList::IsKnownFile(const CKnownFile* file) const
{
	if (file)
		return FindKnownFileByID(file->GetFileHash()) != NULL;
	return false;
}

bool CKnownFileList::IsFilePtrInList(const CKnownFile* file) const
{
	if (file)
	{
		POSITION pos = m_Files_map.GetStartPosition();
		while (pos)
		{
			CCKey key;
			CKnownFile* cur_file;
			m_Files_map.GetNextAssoc(pos, key, cur_file);
			if (file == cur_file)
				return true;
		}
	}
	return false;
}

void CKnownFileList::AddCancelledFileID(const uchar* hash){
	if (thePrefs.IsRememberingCancelledFiles()){
		m_mapCancelledFiles.SetAt(CSKey(hash), 1);	
	}
}

bool CKnownFileList::IsCancelledFileByID(const uchar* hash) const
{
	if (thePrefs.IsRememberingCancelledFiles()){
		int dwDummy;
		if (m_mapCancelledFiles.Lookup(CSKey(hash), dwDummy)){
			return true;
		}
	}
	return false;
}

void CKnownFileList::CopyKnownFileMap(CMap<CCKey,const CCKey&,CKnownFile*,CKnownFile*> &Files_Map)
{
	if (!m_Files_map.IsEmpty())
	{
		POSITION pos = m_Files_map.GetStartPosition();
		while (pos)
		{
			CCKey key;
			CKnownFile* cur_file;
			m_Files_map.GetNextAssoc(pos, key, cur_file);
			Files_Map.SetAt(key, cur_file);
		}
	}
}
