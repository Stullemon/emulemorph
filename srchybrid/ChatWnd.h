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
#include "afxdlgs.h"
#include "HyperTextCtrl.h"
#include "chatselector.h"
#include "ResizableLib\ResizableDialog.h"
#include "friendlistctrl.h"
// CChatWnd dialog

class CChatWnd : public CResizableDialog
{
	DECLARE_DYNAMIC(CChatWnd)
public:
	void ScrollHistory(bool down);
	CChatSelector chatselector;
	CChatWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CChatWnd();

// Dialog Data
	enum { IDD = IDD_CHAT };
	void StartSession(CUpDownClient* client);
	void Localize();
	CFriendListCtrl m_FriendListCtrl;
protected:
	CEdit	inputtext;
	LRESULT		OnCloseTab(WPARAM wparam, LPARAM lparam);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support	
	virtual BOOL OnInitDialog(); 
	void	OnShowWindow(BOOL bShow,UINT nStatus);
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()
private:
	HICON icon_friend,icon_msg;
};
