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
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
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
	if ( !theApp.emuledlg->IsRunning() )
		return 0;

	// we collect all masterhashs which we find in the known2.met and store them in a list
	CList<CAICHHash,CAICHHash&> liKnown2Hashs;
	CString fullpath=thePrefs.GetConfigDir();
	fullpath.Append(KNOWN2_MET_FILENAME);
	CSafeFile file;
	CFileException fexp;
	uint32 nLastVerifiedPos = 0;
	if (!file.Open(fullpath,CFile::modeCreate|CFile::modeReadWrite|CFile::modeNoTruncate|CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyNone, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(_T("Failed to load ") KNOWN2_MET_FILENAME _T(" file"));
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
		uint32 nExistingSize = file.GetLength();
		uint16 nHashCount;
		while (file.GetPosition() < nExistingSize){
			liKnown2Hashs.AddTail(CAICHHash(&file));
			nHashCount = file.ReadUInt16();
			if (file.GetPosition() + nHashCount*HASHSIZE > nExistingSize){
				AfxThrowFileException(CFileException::endOfFile, 0, file.GetFileName());
			}
			// skip the rest of this hashset
			file.Seek(nHashCount*HASHSIZE, CFile::current);
			nLastVerifiedPos = file.GetPosition();
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
	file.Close();

	// now we check that all files which are in the sharedfilelist have a corresponding hash in out list
	// those how don'T are added to the hashinglist
	for (uint32 i = 0; i < theApp.sharedfiles->GetCount(); i++){
		CKnownFile* pCurFile = theApp.sharedfiles->GetFileByIndex(i);
		if (pCurFile != NULL && !pCurFile->IsPartFile() ){
			if (theApp.emuledlg==NULL || !theApp.emuledlg->IsRunning()) // in case of shutdown while still hashing
				return 0;
			if (pCurFile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE){
				bool bFound = false;
				for (POSITION pos = liKnown2Hashs.GetHeadPosition();pos != 0;)
				{
					if (liKnown2Hashs.GetNext(pos) == pCurFile->GetAICHHashset()->GetMasterHash()){
						bFound = true;
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
// WebCache ////////////////////////////////////////////////////////////////////////////////////
				if(thePrefs.GetLogICHEvents()) //JP log ICH events
				theApp.QueueDebugLogLine(false, _T("Failed to create AICH Hashset while sync. for file %s"), pCurFile->GetFileName());
		}

		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.SetAICHHashing(0);
		if (theApp.emuledlg->sharedfileswnd->sharedfilesctrl.m_hWnd != NULL)
			theApp.emuledlg->sharedfileswnd->sharedfilesctrl.ShowFilesCount();
		sLock1.Unlock();
	}
// WebCache ////////////////////////////////////////////////////////////////////////////////////
	if(thePrefs.GetLogICHEvents()) //JP log ICH events
	theApp.QueueDebugLogLine(false, _T("AICHSyncThread finished"));
	return 0;
}

