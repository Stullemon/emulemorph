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
#pragma once
#include "loggable.h"

class CFriend;
class CFriendListCtrl;
class CUpDownClient;

class CFriendList: public CLoggable
{
public:
	CFriendList();
	~CFriendList();

	bool		IsAlreadyFriend(uint32 dwLastUsedIP, uint32 nLastUsedPort) const;
	void		SaveList();
	bool		LoadList();
	void		RefreshFriend(CFriend* torefresh) const;
	CFriend*	SearchFriend(const uchar* achUserHash, uint32 dwIP, uint16 nPort) const;
	void		SetWindow(CFriendListCtrl* NewWnd)	{ m_wndOutput = NewWnd; }
	void		ShowFriends() const;
	bool		AddFriend(CUpDownClient* toadd);
	bool		AddFriend(const uchar* abyUserhash, uint32 dwLastSeen, uint32 dwLastUsedIP, uint32 nLastUsedPort, 
                          uint32 dwLastChatted, LPCTSTR pszName, uint32 dwHasHash);
	void		RemoveFriend(CFriend* todel);
	void		RemoveAllFriendSlots();
	void		Process();

private:
	CTypedPtrList<CPtrList, CFriend*>	m_listFriends;
	CFriendListCtrl*					m_wndOutput;
	uint32								m_nLastSaved;
};
