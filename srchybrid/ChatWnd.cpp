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

// ChatWnd.cpp : implementation file
//
#include "stdafx.h"
#include "emule.h"
#include "ChatWnd.h"
#include "HyperTextCtrl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CChatWnd dialog

IMPLEMENT_DYNAMIC(CChatWnd, CDialog)
CChatWnd::CChatWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CChatWnd::IDD, pParent)
{
}

CChatWnd::~CChatWnd()
{
	DestroyIcon(icon_friend);
	DestroyIcon(icon_msg);
}

void CChatWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHATSEL, chatselector);
	DDX_Control(pDX, IDC_LIST2, m_FriendListCtrl);
	DDX_Control(pDX, IDC_CMESSAGE, inputtext);
}

BEGIN_MESSAGE_MAP(CChatWnd, CResizableDialog)
	ON_WM_KEYDOWN()
	ON_WM_SHOWWINDOW()
	ON_MESSAGE(WM_CLOSETAB, OnCloseTab)
END_MESSAGE_MAP()

// CChatWnd message handlers

BOOL CChatWnd::OnInitDialog(){
	CResizableDialog::OnInitDialog();

	chatselector.Init();
	m_FriendListCtrl.Init();
	InitWindowStyles(this);

	icon_friend=(HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_FRIEND), IMAGE_ICON, 16, 16, 0);
	icon_msg=(HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_MESSAGE), IMAGE_ICON, 16, 16, 0);
	((CStatic*)GetDlgItem(IDC_MESSAGEICON))->SetIcon(icon_msg);
	((CStatic*)GetDlgItem(IDC_FRIENDSICON))->SetIcon(icon_friend);

	AddAnchor(IDC_CHATSEL,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_LIST2,TOP_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_FRIENDS_LBL,TOP_LEFT);
	AddAnchor(IDC_FRIENDSICON,TOP_LEFT);

	Localize();

	return true;
}

void CChatWnd::StartSession(CUpDownClient* client){
	if (!client->GetUserName())
		return;
	theApp.emuledlg->SetActiveDialog(this);
	chatselector.StartSession(client,true);
}

void CChatWnd::OnShowWindow(BOOL bShow,UINT nStatus){
	if (bShow)
		chatselector.ShowChat();
}

BOOL CChatWnd::PreTranslateMessage(MSG* pMsg) 
{
   

   return CResizableDialog::PreTranslateMessage(pMsg);
}

void CChatWnd::Localize()
{
	if(m_hWnd)
	{
		GetDlgItem(IDC_FRIENDS_LBL)->SetWindowText(GetResString(IDS_CW_FRIENDS));
		GetDlgItem(IDC_MESSAGES_LBL)->SetWindowText(GetResString(IDS_CW_MESSAGES));
		chatselector.Localize();
		m_FriendListCtrl.Localize();
	}
}

LRESULT CChatWnd::OnCloseTab(WPARAM wparam, LPARAM lparam) {
	TCITEM item;
	item.mask = TCIF_PARAM;
	chatselector.GetItem((int)wparam,&item);
	
	chatselector.EndSession(((CChatItem*)item.lParam)->client);

	return true;
}

void CChatWnd::ScrollHistory(bool down) {
	CString buffer;

	CChatItem* ci = chatselector.GetCurrentChatItem();
	if (ci==NULL) return;

	if ( (ci->history_pos==0 && !down) || (ci->history_pos==ci->history.GetCount() && down)) return;
	
	
	if (down) ++ci->history_pos; else --ci->history_pos;

	buffer= (ci->history_pos==ci->history.GetCount())?"":ci->history.GetAt(ci->history_pos);

	inputtext.SetWindowText(buffer);
	inputtext.SetSel(buffer.GetLength(),buffer.GetLength());
}
