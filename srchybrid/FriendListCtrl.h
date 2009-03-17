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
#pragma once
#include "MuleListCtrl.h"

class CFriend;

class CFriendListCtrl : public CMuleListCtrl
{
	DECLARE_DYNAMIC(CFriendListCtrl)
	friend class CFriendList;

public:
	CFriendListCtrl();
	virtual ~CFriendListCtrl();

	void	Init();
	void	Localize();
	void	UpdateList();

protected:
	void	SetAllIcons();
	void	AddFriend(const CFriend* pFriend);
	void	RemoveFriend(const CFriend* pFriend);
	void	RefreshFriend(const CFriend* pFriend);
	void	UpdateFriend(int iItem, const CFriend* pFriend);
	void	ShowFriendDetails(const CFriend* pFriend);
	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
	afx_msg void OnLvnColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNmDblClk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSysColorChange();

// MORPH START - Added by Commander, Friendlinks [emulEspa�a]
public:
	bool	AddEmfriendsMetToList(const CString& strFile);
// MORPH END - Added by Commander, Friendlinks [emulEspa�a]
};
