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
#include "HelpIDs.h"
#include "Opcodes.h"
//MORPH END   - Changed by SiRoB, New friend message window [TPT]
#include "friend.h"
#include "ClientCredits.h"
//MORPH END   - Changed by SiRoB, New friend message window [TPT]

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
	ON_WM_HELPINFO()
	//MORPH START - Added by SiRoB, New friend message window [TPT]
	ON_NOTIFY(LVN_ITEMACTIVATE, IDC_LIST2, OnLvnItemActivateFrlist)
	ON_NOTIFY(NM_CLICK, IDC_LIST2, OnNMClickFrlist)
	//MORPH END   - Added by SiRoB, New friend message window [TPT]
END_MESSAGE_MAP()

// CChatWnd message handlers

//MORPH START - Added by SiRoB, New friend message window [TPT]
void CChatWnd::OnLvnItemActivateFrlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (m_FriendListCtrl.GetSelectionMark() != (-1) ) {
		CFriend* mFriend = (CFriend*)m_FriendListCtrl.GetItemData(m_FriendListCtrl.GetSelectionMark());

		ShowFriendMsgDetails(mFriend);
	}
}

void CChatWnd::ShowFriendMsgDetails(CFriend* pFriend) 
{
	if (pFriend)
	{
	ASSERT( pFriend != NULL);
	CString buffer;

	// Name
	if (pFriend->m_LinkedClient)
	{
		GetDlgItem(IDC_FRIENDS_NAME_EDIT)->SetWindowText(pFriend->m_LinkedClient->GetUserName());
	}
	else
	{
		GetDlgItem(IDC_FRIENDS_NAME_EDIT)->SetWindowText("?");
	}

	// Hash
	if (pFriend->m_LinkedClient)
	{
		buffer ="";
		CString buffer2;
		for (uint16 i = 0;i != 16;i++)
		{
			buffer2.Format("%02X",pFriend->m_LinkedClient->GetUserHash()[i]);
			buffer+=buffer2;
		}
		GetDlgItem(IDC_FRIENDS_USERHASH_EDIT)->SetWindowText(buffer);
	}
	else
		GetDlgItem(IDC_FRIENDS_USERHASH_EDIT)->SetWindowText("?");
	
	// Client
	if (pFriend->m_LinkedClient)
	{
		GetDlgItem(IDC_FRIENDS_CLIENTE_EDIT)->SetWindowText(pFriend->m_LinkedClient->DbgGetFullClientSoftVer());
	}
	else
		GetDlgItem(IDC_FRIENDS_CLIENTE_EDIT)->SetWindowText("?");

	// Identification
	if (pFriend->m_LinkedClient)
	{
		if (theApp.clientcredits->CryptoAvailable())
		{
			switch(pFriend->m_LinkedClient->Credits()->GetCurrentIdentState(pFriend->m_LinkedClient->GetIP()))
			{
				case IS_NOTAVAILABLE:
					GetDlgItem(IDC_FRIENDS_IDENTIFICACION_EDIT)->SetWindowText(GetResString(IDS_IDENTNOSUPPORT));
					break;
				case IS_IDFAILED:
				case IS_IDNEEDED:
				case IS_IDBADGUY:
					GetDlgItem(IDC_FRIENDS_IDENTIFICACION_EDIT)->SetWindowText(GetResString(IDS_IDENTFAILED));
					break;
				case IS_IDENTIFIED:
					GetDlgItem(IDC_FRIENDS_IDENTIFICACION_EDIT)->SetWindowText(GetResString(IDS_IDENTOK));
					break;
			}
		}
		else
			GetDlgItem(IDC_FRIENDS_IDENTIFICACION_EDIT)->SetWindowText(GetResString(IDS_IDENTNOSUPPORT));
	}
	else
		GetDlgItem(IDC_FRIENDS_IDENTIFICACION_EDIT)->SetWindowText("?");

	// Upoload and downloaded
	if (pFriend->m_LinkedClient)
	{
		GetDlgItem(IDC_FRIENDS_DESCARGADO_EDIT)->SetWindowText(CastItoXBytes(pFriend->m_LinkedClient->Credits()->GetDownloadedTotal()));
	}
	else
		GetDlgItem(IDC_FRIENDS_DESCARGADO_EDIT)->SetWindowText("?");

	if (pFriend->m_LinkedClient)
	{
		GetDlgItem(IDC_FRIENDS_SUBIDO_EDIT)->SetWindowText(CastItoXBytes(pFriend->m_LinkedClient->Credits()->GetUploadedTotal()));
	}
	else
		GetDlgItem(IDC_FRIENDS_SUBIDO_EDIT)->SetWindowText("?");

	}
}
//MORPH END  - Added by SiRoB, New friend message window [TPT]

BOOL CChatWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();

	inputtext.SetLimitText(MAX_CLIENT_MSG_LEN);
	chatselector.Init();
	m_FriendListCtrl.Init();
	//MORPH START - Changed by SiRoB, Splitting Bar	[O
	/*	
	AddAnchor(IDC_CHATSEL,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_LIST2,TOP_LEFT,BOTTOM_LEFT);
	AddAnchor(IDC_FRIENDS_LBL,TOP_LEFT);
	*/
	RemoveAnchor(IDC_FRIENDS_NAME);
	AddAnchor(IDC_FRIENDS_NAME,CSize(0,100));
	RemoveAnchor(IDC_FRIENDS_NAME_EDIT);
	AddAnchor(IDC_FRIENDS_NAME_EDIT,CSize(0,100));
	RemoveAnchor(IDC_FRIENDS_USERHASH);
	AddAnchor(IDC_FRIENDS_USERHASH,CSize(0,100));
	RemoveAnchor(IDC_FRIENDS_USERHASH_EDIT);
	AddAnchor(IDC_FRIENDS_USERHASH_EDIT,CSize(0,100));
	RemoveAnchor(IDC_FRIENDS_CLIENT);
	AddAnchor(IDC_FRIENDS_CLIENT,CSize(0,100));
	RemoveAnchor(IDC_FRIENDS_CLIENTE_EDIT);
	AddAnchor(IDC_FRIENDS_CLIENTE_EDIT,CSize(0,100));
	RemoveAnchor(IDC_FRIENDS_IDENT);
	AddAnchor(IDC_FRIENDS_IDENT,CSize(0,100));
	RemoveAnchor(IDC_FRIENDS_IDENTIFICACION_EDIT);
	AddAnchor(IDC_FRIENDS_IDENTIFICACION_EDIT,CSize(0,100));
	RemoveAnchor(IDC_FRIENDS_UPLOADED);
	AddAnchor(IDC_FRIENDS_UPLOADED,CSize(0,100));
	RemoveAnchor(IDC_FRIENDS_SUBIDO_EDIT);
	AddAnchor(IDC_FRIENDS_SUBIDO_EDIT,CSize(0,100));
	RemoveAnchor(IDC_FRIENDS_DOWNLOADED);
	AddAnchor(IDC_FRIENDS_DOWNLOADED,CSize(0,100));
	RemoveAnchor(IDC_FRIENDS_DESCARGADO_EDIT);
	AddAnchor(IDC_FRIENDS_DESCARGADO_EDIT,CSize(0,100));
	//MORPH END   - Changed by SiRoB, Splitting Bar	[O²]

	SetAllIcons();

	//MORPH START - Added by SiRoB, Splitting bar [O²]
	CRect rc,rcSpl;

	CWnd* pWnd = GetDlgItem(IDC_LIST2);
	pWnd->GetWindowRect(rcSpl);
	ScreenToClient(rcSpl);
	
	GetWindowRect(rc);
	ScreenToClient(rc);

	rcSpl.bottom=rc.bottom-10; rcSpl.left=rcSpl.right +3; rcSpl.right=rcSpl.left+4;
	m_wndSplitterchat.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER_FRIEND);


	int PosStatVinit = rcSpl.left;
	int PosStatVnew = thePrefs.GetSplitterbarPositionFriend();
	int max = 700;
	int min = 300;
	if (thePrefs.GetSplitterbarPositionFriend() > max) PosStatVnew = max;
	else if (thePrefs.GetSplitterbarPositionFriend() < min) PosStatVnew = min;
	rcSpl.left = PosStatVnew;
	rcSpl.right = PosStatVnew+5;

	m_wndSplitterchat.MoveWindow(rcSpl);
	DoResize(PosStatVnew - PosStatVinit);

	//MORPH END - Added by SiRoB, Splitting bar [O²]

	Localize();
	theApp.friendlist->ShowFriends();

	return true;
}

void CChatWnd::DoResize(int delta)
{

CSplitterControl::ChangeWidth(GetDlgItem(IDC_LIST2), delta);
CSplitterControl::ChangeWidth(GetDlgItem(IDC_FRIENDS_MSG), delta);
CSplitterControl::ChangeWidth(GetDlgItem(IDC_CHATSEL), -delta, CW_RIGHTALIGN);
CSplitterControl::ChangePos(GetDlgItem(IDC_MESSAGES_LBL), -delta, 0);
CSplitterControl::ChangePos(GetDlgItem(IDC_MESSAGEICON), -delta, 0);

CRect rcW;

GetWindowRect(rcW);
ScreenToClient(rcW);

CRect rcspl;
GetDlgItem(IDC_LIST2)->GetClientRect(rcspl);

thePrefs.SetSplitterbarPositionFriend(rcspl.right);

RemoveAnchor(m_wndSplitterchat);
AddAnchor(m_wndSplitterchat,CSize(0,0));
RemoveAnchor(IDC_LIST2);
AddAnchor(IDC_LIST2,CSize(0,0),CSize(0,100));
RemoveAnchor(IDC_FRIENDS_MSG);
AddAnchor(IDC_FRIENDS_MSG,CSize(0,100),CSize(0,100));
RemoveAnchor(IDC_CHATSEL);
AddAnchor(IDC_CHATSEL,CSize(0,0),CSize(100,100));
RemoveAnchor(IDC_MESSAGES_LBL);
AddAnchor(IDC_MESSAGES_LBL,CSize(0,0));
RemoveAnchor(IDC_MESSAGEICON);
AddAnchor(IDC_MESSAGEICON,CSize(0,0));

m_wndSplitterchat.SetRange(rcW.left+290, rcW.left+700);
//initCSize(thePrefs.GetSplitterbarPositionFriend());


m_FriendListCtrl.DeleteColumn(0);
m_FriendListCtrl.Init();

Invalidate();
UpdateWindow();
}

LRESULT CChatWnd::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
switch (message) {
 case WM_PAINT:
  if (m_wndSplitterchat) {
   CRect rctree,rcSpl,rcW;
   CWnd* pWnd;

   GetWindowRect(rcW);
   ScreenToClient(rcW);

   pWnd = GetDlgItem(IDC_LIST2);
   pWnd->GetWindowRect(rctree);

   ScreenToClient(rctree);
  

   if (rcW.Width()>0) {

	rcSpl.left=rctree.right+5;
    rcSpl.right=rcSpl.left+5;
    rcSpl.top=rctree.top;
    rcSpl.bottom=rcW.bottom-5;
    
    m_wndSplitterchat.MoveWindow(rcSpl,true);

	m_FriendListCtrl.DeleteColumn(0);
	m_FriendListCtrl.Init();


   }

  }
  break;
 case WM_NOTIFY:
  if (wParam == IDC_SPLITTER_FRIEND)
  { 
   SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
   DoResize(pHdr->delta);
  }
  break;
 case WM_WINDOWPOSCHANGED : 
  {
   CRect rcW;
   GetWindowRect(rcW);
   ScreenToClient(rcW);

   if (m_wndSplitterchat && rcW.Width()>0) Invalidate();
   break;
  }
 case WM_SIZE:
  {
      //set range
   if (m_wndSplitterchat)
   {
    CRect rc;
    GetWindowRect(rc);
    ScreenToClient(rc);
    m_wndSplitterchat.SetRange(rc.left+290, rc.left+700);
   }
   break;
  }

}

return CResizableDialog::DefWindowProc(message, wParam, lParam);

}
// eMule O² : Bzubzu Splitt-chat <<<
//MORPH END - Added by SiRoB, Splitting bar [O²]


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

//MORPH START - Added by SiRoB, New friend message window [TPT]
void CChatWnd::OnNMClickFrlist(NMHDR *pNMHDR, LRESULT *pResult){
	OnLvnItemActivateFrlist(pNMHDR,pResult);
	*pResult = 0;
}
//MORPH END   - Added by SiRoB, New friend message window [TPT]

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
	//MORPH START - Added by SiRoB, New friend message window [TPT]
	GetDlgItem(IDC_FRIENDS_DOWNLOADED)->SetWindowText(GetResString(IDS_CHAT_DOWNLOADED));
	GetDlgItem(IDC_FRIENDS_UPLOADED)->SetWindowText(GetResString(IDS_CHAT_UPLOADED));
	GetDlgItem(IDC_FRIENDS_IDENT)->SetWindowText(GetResString(IDS_CHAT_IDENT));
	GetDlgItem(IDC_FRIENDS_CLIENT)->SetWindowText(GetResString(IDS_CHAT_CLIENT));
	GetDlgItem(IDC_FRIENDS_NAME)->SetWindowText(GetResString(IDS_CHAT_NAME));
	//MORPH END   - Added by SiRoB, New friend message window [TPT]
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

void CChatWnd::UpdateFriendlistCount(uint16 count) {
	CString temp;
	temp.Format(" (%i)",count);
	temp=GetResString(IDS_CW_FRIENDS)+temp;

	GetDlgItem(IDC_FRIENDS_LBL)->SetWindowText(temp);
}

BOOL CChatWnd::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.ShowHelp(eMule_FAQ_Friends);
	return TRUE;
}
