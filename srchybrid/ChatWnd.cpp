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
#include "emule.h"
#include "ChatWnd.h"
#include "HTRichEditCtrl.h"
#include "FriendList.h"
#include "emuledlg.h"
#include "UpDownClient.h"
#include "OtherFunctions.h"
#include "MenuCmds.h"

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
	icon_friend = NULL;
	icon_msg = NULL;
}

CChatWnd::~CChatWnd()
{
	if (icon_friend)
		VERIFY( DestroyIcon(icon_friend) );
	if (icon_msg)
		VERIFY( DestroyIcon(icon_msg) );
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
	ON_WM_SYSCOLORCHANGE()
    ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

// CChatWnd message handlers

BOOL CChatWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	chatselector.Init();
	m_FriendListCtrl.Init();

	AddAnchor(IDC_CHATSEL,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_LIST2,TOP_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_FRIENDS_LBL,TOP_LEFT);

	SetAllIcons();
	Localize();
	theApp.friendlist->ShowFriends();

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

void CChatWnd::SetAllIcons()
{
	InitWindowStyles(this);

	if (icon_friend)
		VERIFY( DestroyIcon(icon_friend) );
	if (icon_msg)
		VERIFY( DestroyIcon(icon_msg) );
	icon_friend = theApp.LoadIcon("Friend", 16, 16);
	icon_msg = theApp.LoadIcon("Message", 16, 16);
	((CStatic*)GetDlgItem(IDC_MESSAGEICON))->SetIcon(icon_msg);
	((CStatic*)GetDlgItem(IDC_FRIENDSICON))->SetIcon(icon_friend);
}

void CChatWnd::Localize()
{
	GetDlgItem(IDC_FRIENDS_LBL)->SetWindowText(GetResString(IDS_CW_FRIENDS));
	GetDlgItem(IDC_MESSAGES_LBL)->SetWindowText(GetResString(IDS_CW_MESSAGES));
	chatselector.Localize();
	m_FriendListCtrl.Localize();
}

LRESULT CChatWnd::OnCloseTab(WPARAM wparam, LPARAM lparam)
{
	TCITEM item = {0};
	item.mask = TCIF_PARAM;
	if (chatselector.GetItem((int)wparam, &item))
		chatselector.EndSession(((CChatItem*)item.lParam)->client);
	return TRUE;
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

void CChatWnd::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

BOOL CChatWnd::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam){ 
		case MP_REMOVE:{
			const CChatItem* ci = chatselector.GetCurrentChatItem();
			if (ci)
				chatselector.EndSession(ci->client);
			break;
		}
	}
	return TRUE;
}

void CChatWnd::OnContextMenu(CWnd* pWnd, CPoint point)
{ 
	if (!chatselector.GetCurrentChatItem())
		return;

	CTitleMenu ChatMenu;
	ChatMenu.CreatePopupMenu(); 
	ChatMenu.AddMenuTitle(GetResString(IDS_CW_MESSAGES));
	ChatMenu.AppendMenu(MF_STRING, MP_REMOVE, GetResString(IDS_FD_CLOSE));
	ChatMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
 	VERIFY( ChatMenu.DestroyMenu() );
}
