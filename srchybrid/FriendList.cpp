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

#include "StdAfx.h"
#include "friendlist.h"
#include "emule.h"
#include <io.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define EMFRIENDS_MET_FILENAME	_T("emfriends.met")

CFriendList::CFriendList()
{
	LoadList();
	m_nLastSaved = ::GetTickCount();
}

CFriendList::~CFriendList()
{
	SaveList();
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;m_listFriends.GetNext(pos))
		delete m_listFriends.GetAt(pos);
	m_listFriends.RemoveAll();
}

bool CFriendList::LoadList(){
	CString strFileName = CString(theApp.glob_prefs->GetConfigDir()) + CString(EMFRIENDS_MET_FILENAME);
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(strFileName.GetBuffer(),CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(GetResString(IDS_ERR_READEMFRIENDS));
			char szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError,MAX_CFEXP_ERRORMSG)){
				strError += _T(" - ");
				strError += szError;
			}
			AddLogLine(true, _T("%s"), strError);
		}
		return false;
	}

	try {
		uint8 header;
		file.Read(&header,1);
		if (header != MET_HEADER){
			file.Close();
			return false;
		}
		uint32 nRecordsNumber;
		file.Read(&nRecordsNumber,4);
		for (uint32 i = 0; i < nRecordsNumber; i++) {
			CFriend* Record =  new CFriend();
			Record->LoadFromFile(&file);
			m_listFriends.AddTail(Record);
		}
		file.Close();
	}
	catch(CFileException* error){
		OUTPUT_DEBUG_TRACE();
		if (error->m_cause == CFileException::endOfFile)
			AddLogLine(true,GetResString(IDS_ERR_EMFRIENDSINVALID));
		else{
			char buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,MAX_CFEXP_ERRORMSG);
			AddLogLine(true,GetResString(IDS_ERR_READEMFRIENDS),buffer);
		}
		error->Delete();
		return false;
	}

	return true;
}

void CFriendList::SaveList(){
	DEBUG_ONLY(AddDebugLogLine(false, "Saved Friend list"));
	m_nLastSaved = ::GetTickCount();

	CString strFileName = CString(theApp.glob_prefs->GetConfigDir()) + CString(EMFRIENDS_MET_FILENAME);
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(strFileName.GetBuffer(),CFile::modeCreate|CFile::modeWrite|CFile::typeBinary, &fexp)){
		CString strError(_T("Failed to save ") EMFRIENDS_MET_FILENAME _T(" file"));
		char szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError,MAX_CFEXP_ERRORMSG)){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(true, _T("%s"), strError);
		return;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
	
	try{
		uint8 header = MET_HEADER;
		file.Write(&header,1);
		uint32 nRecordsNumber = m_listFriends.GetCount();
		file.Write(&nRecordsNumber,4);
		for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;m_listFriends.GetNext(pos)){
			m_listFriends.GetAt(pos)->WriteToFile(&file);
		}
		if (theApp.glob_prefs->GetCommitFiles() >= 2 || (theApp.glob_prefs->GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			file.Flush(); // flush file stream buffers to disk buffers
			if (_commit(_fileno(file.m_pStream)) != 0) // commit disk buffers to disk
				AfxThrowFileException(CFileException::hardIO, GetLastError(), file.GetFileName());
		}
		file.Close();
	}
	catch(CFileException* error){
		CString strError(_T("Failed to save ") EMFRIENDS_MET_FILENAME _T(" file"));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (error->GetErrorMessage(szError, ELEMENT_COUNT(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(true, _T("%s"), strError);
		error->Delete();
	}
}

CFriend* CFriendList::SearchFriend(const uchar* abyUserHash, uint32 dwIP, uint16 nPort) const {
	POSITION pos = m_listFriends.GetHeadPosition();
	while (pos){
		CFriend* cur_friend = m_listFriends.GetNext(pos);
		// to avoid that unwanted clients become a friend, we have to distinguish between friends with
		// a userhash and of friends which are identified by IP+port only.
		if (cur_friend->m_dwHasHash){
			// check for a friend which has the same userhash as the specified one
			if (!md4cmp(cur_friend->m_abyUserhash, abyUserHash))
				return cur_friend;
		}
		else{
			if (cur_friend->m_dwLastUsedIP == dwIP && cur_friend->m_nLastUsedPort == nPort)
				return cur_friend;
		}
	}
	return NULL;
}

void CFriendList::RefreshFriend(CFriend* torefresh) const {
	if (m_wndOutput)
		m_wndOutput->RefreshFriend(torefresh);
}

void CFriendList::ShowFriends() const {
	if (!m_wndOutput){
		ASSERT ( false );
		return;
	}
	m_wndOutput->DeleteAllItems();
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;m_listFriends.GetNext(pos)){
		CFriend* cur_friend = m_listFriends.GetAt(pos);
		m_wndOutput->AddFriend(cur_friend);	
	}
}

//You can add a friend without a IP to allow the IRC to trade links with lowID users.
void CFriendList::AddFriend(const uchar* abyUserhash, uint32 dwLastSeen, uint32 dwLastUsedIP, uint32 nLastUsedPort, 
							uint32 dwLastChatted, LPCTSTR pszName, uint32 dwHasHash){
	if( dwLastUsedIP && IsAlreadyFriend(dwLastUsedIP, nLastUsedPort))
		return;
	CFriend* Record = new CFriend( abyUserhash, dwLastSeen, dwLastUsedIP, nLastUsedPort, dwLastChatted, pszName, dwHasHash );
	m_listFriends.AddTail(Record);
	ShowFriends();
	SaveList();
}

// Added for the friends function in the IRC..
bool CFriendList::IsAlreadyFriend( uint32 dwLastUsedIP, uint32 nLastUsedPort ) const {
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;m_listFriends.GetNext(pos)){
		CFriend* cur_friend = m_listFriends.GetAt(pos);
		if ( cur_friend->m_dwLastUsedIP == dwLastUsedIP && cur_friend->m_nLastUsedPort == nLastUsedPort ){
			return true;
		}
	}
	return false;
}

void CFriendList::AddFriend(CUpDownClient* toadd){
	if (toadd->IsFriend())
		return;
	CFriend* NewFriend = new CFriend(toadd);
	toadd->m_Friend = NewFriend;
	m_listFriends.AddTail(NewFriend);
	if (m_wndOutput)
		m_wndOutput->AddFriend(NewFriend);	
	SaveList();
}

void CFriendList::RemoveFriend(CFriend* todel){
	POSITION pos = m_listFriends.Find(todel);
	if (!pos){
		ASSERT ( false );
		return;
	}
//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System
todel->SetLinkedClient(NULL);
//MORPH END - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System

	if (m_wndOutput)
		m_wndOutput->RemoveFriend(todel);
	m_listFriends.RemoveAt(pos);
	delete todel;
	SaveList();
}

void CFriendList::RemoveAllFriendSlots(){
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;m_listFriends.GetNext(pos)){
		CFriend* cur_friend = m_listFriends.GetAt(pos);
		//MORPH START - Added by Yun.SF3, ZZ Upload System
		cur_friend->SetFriendSlot(false);
		//MORPH END - Added by Yun.SF3, ZZ Upload System
	}
}

void CFriendList::Process()
{
	if (::GetTickCount() - m_nLastSaved > MIN2MS(19))
		this->SaveList();
}