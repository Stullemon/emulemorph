//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "FriendList.h"
#include "UpDownClient.h"
#include "Friend.h"
#include "Preferences.h"
#include "SafeFile.h"
#include "OtherFunctions.h"
#include "opcodes.h"
#include "emuledlg.h"
#include "FriendListCtrl.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
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
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;)
		delete m_listFriends.GetNext(pos);
}

bool CFriendList::LoadList(){
	CString strFileName = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + EMFRIENDS_MET_FILENAME;
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(strFileName, CFile::modeRead | CFile::osSequentialScan | CFile::typeBinary | CFile::shareDenyWrite, &fexp)){
		if (fexp.m_cause != CFileException::fileNotFound){
			CString strError(GetResString(IDS_ERR_READEMFRIENDS));
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
		uint8 header = file.ReadUInt8();
		if (header != MET_HEADER){
			file.Close();
			return false;
		}
		UINT nRecordsNumber = file.ReadUInt32();
		for (UINT i = 0; i < nRecordsNumber; i++) {
			CFriend* Record =  new CFriend();
			Record->LoadFromFile(&file);
			m_listFriends.AddTail(Record);
		}
		file.Close();
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			LogError(LOG_STATUSBAR,GetResString(IDS_ERR_EMFRIENDSINVALID));
		else{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, ARRSIZE(buffer));
			LogError(LOG_STATUSBAR,GetResString(IDS_ERR_READEMFRIENDS),buffer);
		}
		error->Delete();
		return false;
	}

	return true;
}

void CFriendList::SaveList(){
	if (thePrefs.GetLogFileSaving())
		AddDebugLogLine(false, _T("Saving friends list file \"%s\""), EMFRIENDS_MET_FILENAME);
	m_nLastSaved = ::GetTickCount();

	CString strFileName = thePrefs.GetMuleDirectory(EMULE_CONFIGDIR) + EMFRIENDS_MET_FILENAME;
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(strFileName, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary | CFile::shareDenyWrite, &fexp)){
		CString strError(_T("Failed to save ") EMFRIENDS_MET_FILENAME _T(" file"));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		return;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
	
	try{
		file.WriteUInt8(MET_HEADER);
		file.WriteUInt32(m_listFriends.GetCount());
		for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;)
			m_listFriends.GetNext(pos)->WriteToFile(&file);
		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			file.Flush(); // flush file stream buffers to disk buffers
			if (_commit(_fileno(file.m_pStream)) != 0) // commit disk buffers to disk
				AfxThrowFileException(CFileException::hardIO, GetLastError(), file.GetFileName());
		}
		file.Close();
	}
	catch(CFileException* error){
		CString strError(_T("Failed to save ") EMFRIENDS_MET_FILENAME _T(" file"));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (error->GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		error->Delete();
	}
}

CFriend* CFriendList::SearchFriend(const uchar* abyUserHash, uint32 dwIP, uint16 nPort) const {
	POSITION pos = m_listFriends.GetHeadPosition();
	while (pos){
		CFriend* cur_friend = m_listFriends.GetNext(pos);
		// to avoid that unwanted clients become a friend, we have to distinguish between friends with
		// a userhash and of friends which are identified by IP+port only.
		if (abyUserHash != NULL && cur_friend->HasUserhash()){
			// check for a friend which has the same userhash as the specified one
			if (!md4cmp(cur_friend->m_abyUserhash, abyUserHash))
				return cur_friend;
		}
		else{
			if (cur_friend->m_dwLastUsedIP == dwIP && dwIP != 0 && cur_friend->m_nLastUsedPort == nPort && nPort != 0)
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
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;)
		m_wndOutput->AddFriend(m_listFriends.GetNext(pos));
	m_wndOutput->UpdateList();
}

//You can add a friend without a IP to allow the IRC to trade links with lowID users.
bool CFriendList::AddFriend(const uchar* abyUserhash, uint32 dwLastSeen, uint32 dwLastUsedIP, uint16 nLastUsedPort, 
							uint32 dwLastChatted, LPCTSTR pszName, uint32 dwHasHash){
	// client must have an IP (HighID) or a hash
	// TODO: check if this can be switched to a hybridID so clients with *.*.*.0 can be added..
	if (IsLowID(dwLastUsedIP) && dwHasHash==0)
		return false;
	if (SearchFriend(abyUserhash, dwLastUsedIP, nLastUsedPort) != NULL)
		return false;
	CFriend* Record = new CFriend( abyUserhash, dwLastSeen, dwLastUsedIP, nLastUsedPort, dwLastChatted, pszName, dwHasHash );
	m_listFriends.AddTail(Record);
	ShowFriends();
	SaveList();

	// Mighty Knife: log friend activities
	if (thePrefs.GetLogFriendlistActivities ()) {
		#ifdef MIGHTY_TWEAKS
 	    AddLogLine(false, GetResString(IDS_NEWFRIEND)+_T(" '%s', ip %i.%i.%i.%i:%i, ")+GetResString(IDS_CD_UHASH)+_T(" %s"),
									Record->m_strName, (uint8)Record->m_dwLastUsedIP, (uint8)(Record->m_dwLastUsedIP>>8), 
									(uint8)(Record->m_dwLastUsedIP>>16),(uint8)(Record->m_dwLastUsedIP>>24), 
									Record->m_nLastUsedPort, md4str(Record->m_abyUserhash));
		#else
		AddLogLine(false, GetResString(IDS_NEWFRIEND)+_T(" '%s', ")+GetResString(IDS_CD_UHASH)+_T(" %s"),
									Record->m_strName, md4str(Record->m_abyUserhash));
		#endif
	}
	// [end] Mighty Knife
	return true;
}


bool CFriendList::IsAlreadyFriend(CString strUserHash) const
{
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;){
		const CFriend* cur_friend = m_listFriends.GetNext(pos);
		if (cur_friend->HasUserhash() && strUserHash.Compare(md4str(cur_friend->m_abyUserhash)) == 0)
			return true;
	}
	return false;
}

bool CFriendList::AddFriend(CUpDownClient* toadd){
	if (toadd->IsFriend())
		return false;
	// client must have an IP (HighID) or a hash
	if (toadd->HasLowID() && !toadd->HasValidHash())
		return false;
	if (SearchFriend(toadd->GetUserHash(), toadd->GetIP(), toadd->GetUserPort()) != NULL)
		return false;

	CFriend* NewFriend = new CFriend(toadd);
	toadd->m_Friend = NewFriend;
	m_listFriends.AddTail(NewFriend);
	if (m_wndOutput){
		m_wndOutput->AddFriend(NewFriend);
		m_wndOutput->UpdateList();
	}
	SaveList();
	NewFriend->FindKadID(); // fetch the kadid of this friend if we don't have it already
	// Mighty Knife: log friend activities
	if (thePrefs.GetLogFriendlistActivities ()) {
		#ifdef MIGHTY_TWEAKS
		AddLogLine(false, GetResString(IDS_NEWFRIEND)+_T(" '%s', ip %i.%i.%i.%i:%i, ")+GetResString(IDS_CD_UHASH)+_T(" %s"),
									NewFriend->m_strName, (uint8)NewFriend->m_dwLastUsedIP, 
									(uint8)(NewFriend->m_dwLastUsedIP>>8), 
									(uint8)(NewFriend->m_dwLastUsedIP>>16),(uint8)(NewFriend->m_dwLastUsedIP>>24), 
									NewFriend->m_nLastUsedPort, md4str(NewFriend->m_abyUserhash));
		#else
		AddLogLine(false, GetResString(IDS_NEWFRIEND)+_T(" '%s', ")+GetResString(IDS_CD_UHASH)+_T(" %s"),
									NewFriend->m_strName, md4str(NewFriend->m_abyUserhash));
		#endif
	}
	// [end] Mighty Knife

	return true;
}

void CFriendList::RemoveFriend(CFriend* todel){
	POSITION pos = m_listFriends.Find(todel);
	if (!pos){
		ASSERT ( false );
		return;
	}

	// Mighty Knife: log friend activities
	if (thePrefs.GetLogFriendlistActivities ()) {
		#ifdef MIGHTY_TWEAKS
		AddLogLine(false, GetResString(IDS_REMOVEDFRIEND)+_T(" '%s', ip %i.%i.%i.%i:%i, ")+GetResString(IDS_CD_UHASH)+_T(" %s"),
									todel->m_strName, (uint8)todel->m_dwLastUsedIP, 
									(uint8)(todel->m_dwLastUsedIP>>8), 
									(uint8)(todel->m_dwLastUsedIP>>16),(uint8)(todel->m_dwLastUsedIP>>24), 
									todel->m_nLastUsedPort, md4str(todel->m_abyUserhash));
		#else
		AddLogLine(false, GetResString(IDS_REMOVEDFRIEND)+_T(" '%s', ")+GetResString(IDS_CD_UHASH)+_T(" %s"),
									todel->m_strName, md4str(todel->m_abyUserhash));
		#endif
	}
	// [end] Mighty Knife

    todel->SetLinkedClient(NULL);

	if (m_wndOutput)
		m_wndOutput->RemoveFriend(todel);
	m_listFriends.RemoveAt(pos);
	delete todel;
	SaveList();
	if (m_wndOutput)
		m_wndOutput->UpdateList();
}

void CFriendList::RemoveAllFriendSlots(){
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;){
		CFriend* cur_friend = m_listFriends.GetNext(pos);
        cur_friend->SetFriendSlot(false);
		//MORPH - Added by SiRoB, Friend Addon
		RefreshFriend(cur_friend);
		//MORPH - Added by SiRoB, Friend Addon
	}
}

void CFriendList::Process()
{
	if (::GetTickCount() - m_nLastSaved > MIN2MS(19))
		SaveList();
}

bool CFriendList::IsValid(CFriend* pToCheck) const
{
	// debug/sanity check function
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0;){
		if (pToCheck == m_listFriends.GetNext(pos))
			return true;
	}
	return false;
}

//MORPH - Added by SiRoB, There is one slot friend or more
bool CFriendList::IsFriendSlot(){
	for (POSITION pos = m_listFriends.GetHeadPosition();pos != 0; ){
		CFriend* cur_friend = m_listFriends.GetNext(pos);
		if (cur_friend->GetFriendSlot())
			return true;
	}
	return false;
}
//MORPH - Added by SiRoB, There is one slot friend or more
// MORPH START - Added by Commander, Friendlinks [emulEspa�a]
bool CFriendList::IsAlreadyFriend(uchar userHash[]) const
{
	CFriend* cur_friend;

	POSITION pos = m_listFriends.GetHeadPosition();
	while ( pos != NULL )
	{
		cur_friend = m_listFriends.GetNext(pos);
		if ( memcmp(cur_friend->m_abyUserhash, userHash, 16*sizeof(uchar)) == 0 )
			return true;
	}
	return false;
}

bool CFriendList::AddEmfriendsMetToList(const CString& strFile)
{
	CSafeBufferedFile file;
	CFileException fexp;
	if ( !file.Open(strFile, CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary, &fexp) )
	{
		if ( fexp.m_cause != CFileException::fileNotFound )
		{
			CString strError(GetResString(IDS_ERR_READEMFRIENDS));
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			if ( fexp.GetErrorMessage(szError,MAX_CFEXP_ERRORMSG) )
			{
				strError += _T(" - ");
				strError += szError;
			}
			AddLogLine(true, _T("%s"), strError);
		}
		return false;
	}

	try
	{
		uint8 header = file.ReadUInt8();
		if ( header != MET_HEADER )
		{
			file.Close();
			return false;
		}
		UINT nRecordsNumber = file.ReadUInt32();
		for (UINT i = 0; i < nRecordsNumber; i++)
		{
			CFriend* Record =  new CFriend();
			Record->LoadFromFile(&file);
			if ( !IsAlreadyFriend(Record->m_abyUserhash) )
				m_listFriends.AddTail(Record);
		}
		file.Close();
	}
	catch ( CFileException* error )
	{
		if ( error->m_cause == CFileException::endOfFile )
			AddLogLine(true,GetResString(IDS_ERR_EMFRIENDSINVALID));
		else
		{
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer, MAX_CFEXP_ERRORMSG);
			AddLogLine(true, GetResString(IDS_ERR_READEMFRIENDS), buffer);
		}
		error->Delete();
		return false;
	}

	return true;
}
// MORPH END - Added by Commander, Friendlinks [emulEspa�a]
