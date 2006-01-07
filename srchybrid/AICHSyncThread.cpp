//this file is part of eMule
//Copyright (C)2002-2004 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "StdAfx.h"
#include "aichsyncthread.h"
#include "shahashset.h"
#include "safefile.h"
#include "knownfile.h"
#include "sha.h"
#include "emule.h"
#include "emuledlg.h"
#include "sharedfilelist.h"
#include "knownfilelist.h"
#include "preferences.h"
#include "sharedfileswnd.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////////////////
///CAICHSyncThread
IMPLEMENT_DYNCREATE(CAICHSyncThread, CWinThread)

CAICHSyncThread::CAICHSyncThread()
{

}

BOOL CAICHSyncThread::InitInstance()
{
	DbgSetThreadName("AICHSyncThread");
	InitThreadLocale();
	return TRUE;
}

int CAICHSyncThread::Run()
{
	// SLUGFILLER: SafeHash
	CReadWriteLock lock(&theApp.m_threadlock);
	if (!lock.ReadLock(0))
		return 0;
	// SLUGFILLER: SafeHash
	if ( !theApp.emuledlg->IsRunning() )
		return 0;
	// we need to keep a lock on this file while the thread is running
	CSingleLock lockKnown2Met(&CAICHHashSet::m_mutKnown2File);
	lockKnown2Met.Lock();
	
	// we collect all masterhashs which we find in the known2.met and store them in a list
	CList<CAICHHash> liKnown2Hashs;
	CString fullpath=thePrefs.GetConfigDir();
	fullpath.Append(KNOWN2_MET_FILENAME);
	CSafeFile file;
	CFileException fexp;
	uint32 nLastVerifiedPos = 0;

	if (!file.Open(fullpath,CFile::modeCreate|CFile::modeReadWrite|CFile::modeNoTruncate|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyNone, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(GetResString(IDS_FAILEDTOLOAD)+_T(" ") KNOWN2_MET_FILENAME _T(" ")+GetResString(IDS_FILE)); // Localized by FrankyFive
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
				strError += _T(" - ");
				strError += szError;
			}
			LogError(LOG_STATUSBAR, _T("%s"), strError);
		}
		return false;
	}
	try {
		//setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
		uint32 nExistingSize = (UINT)file.GetLength();
		uint16 nHashCount;
		while (file.GetPosition() < nExistingSize){
			liKnown2Hashs.AddTail(CAICHHash(&file));
			nHashCount = file.ReadUInt16();
			if (file.GetPosition() + nHashCount*CAICHHash::GetHashSize() > nExistingSize){
				AfxThrowFileException(CFileException::endOfFile, 0, file.GetFileName());
			}
			// skip the rest of this hashset
			file.Seek(nHashCount*CAICHHash::GetHashSize(), CFile::current);
			nLastVerifiedPos = (UINT)file.GetPosition();
		}
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile){
			LogError(LOG_STATUSBAR,GetResString(IDS_ERR_SERVERMET_BAD));
			// truncate the file to the size to the last verified valid pos
			try{
				file.SetLength(nLastVerifiedPos);
			}
			catch(CFileException* error2){
				error2->Delete();
			}
		}
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, ARRSIZE(buffer));
			LogError(LOG_STATUSBAR,GetResString(IDS_ERR_SERVERMET_UNKNOWN),buffer);
		}
		error->Delete();
		return false;
	}
	
	// now we check that all files which are in the sharedfilelist have a corresponding hash in out list
	// those how don'T are added to the hashinglist
	CList<CAICHHash> liUsedHashs;	
	CSingleLock sharelock(&theApp.sharedfiles->m_mutWriteList);
	sharelock.Lock();

	for (uint32 i = 0; i < theApp.sharedfiles->GetCount(); i++){
		CKnownFile* pCurFile = theApp.sharedfiles->GetFileByIndex(i);
		if (pCurFile != NULL && !pCurFile->IsPartFile() ){
			if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning()) // in case of shutdown while still hashing
				return 0;
			if (pCurFile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE){
				bool bFound = false;
				for (POSITION pos = liKnown2Hashs.GetHeadPosition();pos != 0;)
				{
					CAICHHash current_hash = liKnown2Hashs.GetNext(pos);
					if (current_hash == pCurFile->GetAICHHashset()->GetMasterHash()){
						bFound = true;
						liUsedHashs.AddTail(current_hash);
						//theApp.QueueDebugLogLine(false, _T("%s - %s"), current_hash.GetString(), pCurFile->GetFileName());
#ifdef _DEBUG
						// in debugmode we load and verify all hashsets
						ASSERT( pCurFile->GetAICHHashset()->LoadHashSet() );
//			 			pCurFile->GetAICHHashset()->DbgTest();
						pCurFile->GetAICHHashset()->FreeHashSet();
#endif
						break;
					}
				}
				if (bFound) // hashset is available, everything fine with this file
					continue;
			}
			pCurFile->GetAICHHashset()->SetStatus(AICH_ERROR);
			m_liToHash.AddTail(pCurFile);
		}
	}
	sharelock.Unlock();

	// removed all unused AICH hashsets from known2.met
	if (!thePrefs.IsRememberingDownloadedFiles() && liUsedHashs.GetCount() != liKnown2Hashs.GetCount()){
		file.SeekToBegin();
		try {
			uint32 nExistingSize = (UINT)file.GetLength();
			uint16 nHashCount;
			ULONGLONG posWritePos = 0;
			ULONGLONG posReadPos = 0;
			uint32 nPurgeCount = 0;
			while (file.GetPosition() < nExistingSize){
				CAICHHash aichHash(&file);
				nHashCount = file.ReadUInt16();
				if (file.GetPosition() + nHashCount*CAICHHash::GetHashSize() > nExistingSize){
					AfxThrowFileException(CFileException::endOfFile, 0, file.GetFileName());
				}
				if (liUsedHashs.Find(aichHash) == NULL){
					// unused hashset skip the rest of this hashset
					file.Seek(nHashCount*CAICHHash::GetHashSize(), CFile::current);
					nPurgeCount++;
				}
				else if(nPurgeCount == 0){
					// used Hashset, but it does not need to be moved as nothing changed yet
					file.Seek(nHashCount*CAICHHash::GetHashSize(), CFile::current);
					posWritePos = file.GetPosition();
				}
				else{
					// used Hashset, move position in file
					BYTE* buffer = new BYTE[nHashCount*CAICHHash::GetHashSize()];
					file.Read(buffer, nHashCount*CAICHHash::GetHashSize());
					posReadPos = file.GetPosition();
					file.Seek(posWritePos, CFile::begin);
					file.Write(aichHash.GetRawHash(), CAICHHash::GetHashSize());
					file.WriteUInt16(nHashCount);
					file.Write(buffer, nHashCount*CAICHHash::GetHashSize());
					delete[] buffer;
					posWritePos = file.GetPosition();
					file.Seek(posReadPos, CFile::begin); 
				}
			}
			posReadPos = file.GetPosition();
			file.SetLength(posWritePos);
			theApp.QueueDebugLogLine(false, _T("Cleaned up known2.met, removed %u hashsets (%s)"), nPurgeCount, CastItoXBytes(posReadPos-posWritePos)); 

			file.Flush();
			file.Close();
		}
		catch(CFileException* error){
			if (error->m_cause == CFileException::endOfFile){
				// we just parsed this files some ms ago, should never happen here
				ASSERT( false );
			}
			else{
				TCHAR buffer[MAX_CFEXP_ERRORMSG];
				error->GetErrorMessage(buffer, ARRSIZE(buffer));
				LogError(LOG_STATUSBAR,GetResString(IDS_ERR_SERVERMET_UNKNOWN),buffer);
			}
			error->Delete();
			return false;
		}
	}
	lockKnown2Met.Unlock();
	// warn the user if he just upgraded
	if (thePrefs.IsFirstStart() && !m_liToHash.IsEmpty()){
		LogWarning(GetResString(IDS_AICH_WARNUSER));
	}	
	if (!m_liToHash.IsEmpty()){
		theApp.QueueLogLine(true, GetResString(IDS_AICH_SYNCTOTAL), m_liToHash.GetCount() );
		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.SetAICHHashing(m_liToHash.GetCount());
		// let first all normal hashing be done before starting out synchashing
		CSingleLock sLock1(&theApp.hashing_mut); // only one filehash at a time
		while (theApp.sharedfiles->GetHashingCount() != 0){
			Sleep(100);
		}
		sLock1.Lock();
		uint32 cDone = 0;
		for (POSITION pos = m_liToHash.GetHeadPosition();pos != 0; cDone++)
		{
			if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning()){ // in case of shutdown while still hashing
				return 0;
			}
			theApp.emuledlg->sharedfileswnd->sharedfilesctrl.SetAICHHashing(m_liToHash.GetCount()-cDone);
			if (theApp.emuledlg->sharedfileswnd->sharedfilesctrl.m_hWnd != NULL)
				theApp.emuledlg->sharedfileswnd->sharedfilesctrl.ShowFilesCount();
			CKnownFile* pCurFile = m_liToHash.GetNext(pos);
			// just to be sure that the file hasnt been deleted lately
			if (!(theApp.knownfiles->IsKnownFile(pCurFile) && theApp.sharedfiles->GetFileByID(pCurFile->GetFileHash())) )
				continue;
			theApp.QueueLogLine(false, GetResString(IDS_AICH_CALCFILE), pCurFile->GetFileName());
			if(!pCurFile->CreateAICHHashSetOnly())
				theApp.QueueDebugLogLine(false, _T("Failed to create AICH Hashset while sync. for file %s"), pCurFile->GetFileName());
		}

		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.SetAICHHashing(0);
		if (theApp.emuledlg->sharedfileswnd->sharedfilesctrl.m_hWnd != NULL)
			theApp.emuledlg->sharedfileswnd->sharedfilesctrl.ShowFilesCount();
		sLock1.Unlock();
	}
	theApp.QueueDebugLogLine(false, _T("AICHSyncThread finished"));
	return 0;
}

