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
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "KnownFile.h"
#include "opcodes.h"
#include "Preferences.h"
#include "SafeFile.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
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
	m_Files_map.InitHashTable(1031);
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
	CString fullpath=thePrefs.GetConfigDir();
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

	try {
		uint8 header = file.ReadUInt8();
		if (header != MET_HEADER){
			file.Close();
			return false;
		}
		
		// EastShare START - Added by TAHO, .met file control
		uint32 cDeleted = 0; 
		uint32 cAdded = 0;
		uint32 ExpiredTime = time(NULL) - app_prefs->GetKnownMetDays()*86400;
		// EastShare END - Added by TAHO, .met file control
		
		
		UINT RecordsNumber = file.ReadUInt32();
		for (UINT i = 0; i < RecordsNumber; i++) {
			CKnownFile* pRecord =  new CKnownFile();
			if (!pRecord->LoadFromFile(&file)){
				TRACE("*** Failed to load entry %u (name=%s  hash=%s  size=%u  parthashs=%u expected parthashs=%u) from known.met\n", i, 
					pRecord->GetFileName(), md4str(pRecord->GetFileHash()), pRecord->GetFileSize(), pRecord->GetHashCount(), pRecord->GetED2KPartCount());	// SLUGFILLER: SafeHash - removed unnececery hash counter
				delete pRecord;
				continue;
			}
			//EastShare START - Modified by TAHO, .met file control
			//SafeAddKFile(pRecord);
			if ( thePrefs.GetKnownMetDays() == 0 || pRecord->statistic.GetLastUsed() > ExpiredTime )
			{
				SafeAddKFile(pRecord);
				cAdded++;
			}
			else
			{
				cDeleted++;
			}
			// EastShare END - Modified by TAHO, .met file control
		}
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
	if (thePrefs.GetLogFileSaving())
		AddDebugLogLine(false, "Saving known files list file \"%s\"", KNOWN_MET_FILENAME);
	m_nLastSaved = ::GetTickCount(); 
	CString fullpath=thePrefs.GetConfigDir();
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

	try{
		file.WriteUInt8(MET_HEADER);

		UINT nRecordsNumber = m_Files_map.GetCount();
		// SLUGFILLER: mergeKnown - add part files count
		UINT nRecordsNumberWithPartFiles = RecordsNumber + theApp.downloadqueue->GetPartFilesCount();
		file.WriteUInt32(nRecordsNumberWithPartFiles);
		// SLUGFILLER: mergeKnown
		POSITION pos = m_Files_map.GetStartPosition();
		while( pos != NULL )
		{
			CKnownFile* pFile;
			CCKey key;
			m_Files_map.GetNextAssoc( pos, key, pFile );
			pFile->WriteToFile(&file);
		}
		theApp.downloadqueue->SavePartFilesToKnown(&file);	// SLUGFILLER: mergeKnown - add part files

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
		AddLogLine(true, _T("%s"), strError);
		error->Delete();
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
		TRACE("%s: File already in known file list: %s \"%s\" \"%s\"\n", __FUNCTION__, md4str(pFileInMap->GetFileHash()), pFileInMap->GetFileName(), pFileInMap->GetFilePath());
		TRACE("%s: Old entry replaced with:         %s \"%s\" \"%s\"\n", __FUNCTION__, md4str(toadd->GetFileHash()), toadd->GetFileName(), toadd->GetFilePath());
#if 0
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
			theApp.sharedfiles->RemoveKeywords(pFileInMap);
			ASSERT( !theApp.sharedfiles->IsFilePtrInList(pFileInMap) );
		}
		//Double check to make sure this is the same file as it's possible that a two files have the same hash.
		//Maybe in the furture we can change the client to not just use Hash as a key throughout the entire client..
		if( toadd->GetFileSize() == pFileInMap->GetFileSize() )
			toadd->statistic.MergeFileStats( &pFileInMap->statistic );
		delete pFileInMap;
#else
		// if the new entry is already in list, update the stats and return false, but do not delete the entry which is
		// alreay in known file list!
		if (toadd->GetFileSize() == pFileInMap->GetFileSize())
		{
			pFileInMap->statistic.MergeFileStats(&toadd->statistic);
			pFileInMap->SetFileName(toadd->GetFileName(), false);
			pFileInMap->SetPath(toadd->GetPath());
			pFileInMap->SetFilePath(toadd->GetFilePath());
			pFileInMap->date = toadd->date;
		}
		return false;
#endif
	}
	m_Files_map.SetAt(key, toadd);
	return true;
}

CKnownFile* CKnownFileList::FindKnownFile(LPCTSTR filename, uint32 date, uint32 size) const
{
	POSITION pos = m_Files_map.GetStartPosition();
	while (pos != NULL)
	{
		CKnownFile* cur_file;
		CCKey key;
		m_Files_map.GetNextAssoc(pos, key, cur_file);
		
//#ifdef MIGHTY_SUMMERTIME
		// Mighty Knife: try to correct the daylight saving bug.
		// Very special. Never activate this in a release version !!!

		// If theApp.importknowndates has set bit 1, found files are reported even if the
		// date does not match. The caller can then retrieve the correct date
		// from the file directly and save it to known.met.
		if (cur_file->GetFileSize() == in_size && (!strcmp(filename,cur_file->GetFileName())))
			if ((cur_file->GetFileDate() == in_date)) // importing disabled at the moment
				return ElementAt(i);
//#else
//		if (cur_file->GetFileDate() == in_date && cur_file->GetFileSize() == in_size && (!_tcscmp(filename,cur_file->GetFileName())))
//			return cur_file;
//#endif
		// [end] Mighty Knife
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
