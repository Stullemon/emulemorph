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

#include "types.h"
#include "updownclient.h"
#include "hypertextctrl.h"
#include "otherfunctions.h"
#include "ClosableTabCtrl.h"

#define URLINDICATOR	"http:|www.|.de |.net |.com |.org |.to |.tk |.cc |.fr |ftp:"

class CChatItem{
public:
	CChatItem();
	~CChatItem()		{safe_delete(log);}
	CUpDownClient*		client;
	CPreparedHyperText*	log;
	char*				messagepending;
	bool				notify;
	CStringArray		history;
	int					history_pos;
};
// CChatSelector

class CChatSelector : public CClosableTabCtrl
{
	DECLARE_DYNAMIC(CChatSelector)

public:
	CChatSelector();
	virtual		~CChatSelector();
	void		Init();
	CChatItem*	StartSession(CUpDownClient* client, bool show = true);
	void		EndSession(CUpDownClient* client = 0);
	uint16		GetTabByClient(CUpDownClient* client);
	CChatItem*	GetItemByClient(CUpDownClient* client);
	CHyperTextCtrl chatout;
	void		ProcessMessage(CUpDownClient* sender, char* message);
	bool		SendMessage(char* message);
	void		DeleteAllItems();
	void		ShowChat();
	void		ConnectingResult(CUpDownClient* sender,bool success);
	void		Send();
	void		UpdateFonts(CFont* pFont);
	CChatItem*	GetCurrentChatItem();
protected:
	void		OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnTcnSelchangeChatsel(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCsend();
	afx_msg void OnBnClickedCclose();
	DECLARE_MESSAGE_MAP()
	virtual INT		InsertItem(int nItem,TCITEM* pTabCtrlItem);
	virtual BOOL	DeleteItem(int nItem);
	void		AddTimeStamp(CChatItem*);
	bool		IsSpam(CString strMessage, CUpDownClient* client);
private:
	CImageList	imagelist;
	UINT_PTR	m_Timer;
	bool		blinkstate;
	bool		lastemptyicon;

	CWnd		*m_pMessageBox;
	CWnd		*m_pCloseBtn;
	CWnd		*m_pSendBtn;
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	void Localize(void);
	BOOL		RemoveItem(int nItem)		{ return DeleteItem(nItem);}
	afx_msg void OnDestroy();
};


