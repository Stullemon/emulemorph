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
// FriendListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "FriendListCtrl.h"
#include "friend.h"
#include "ClientDetailDialog.h"
#include "Addfriend.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CFriendListCtrl

IMPLEMENT_DYNAMIC(CFriendListCtrl, CListCtrl)
CFriendListCtrl::CFriendListCtrl()
{
}

CFriendListCtrl::~CFriendListCtrl()
{
}


BEGIN_MESSAGE_MAP(CFriendListCtrl, CListCtrl)
	ON_NOTIFY_REFLECT (NM_RCLICK, OnNMRclick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
END_MESSAGE_MAP()



// CFriendListCtrl message handlers

void CFriendListCtrl::Init(){
	SetExtendedStyle(LVS_EX_FULLROWSELECT);
	RECT rcWindow;
	GetWindowRect(&rcWindow);
	InsertColumn(0, GetResString(IDS_QL_USERNAME), LVCFMT_LEFT,
		rcWindow.right - rcWindow.left - 4, 0);
	imagelist.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,10);
	imagelist.SetBkColor(RGB(255,255,255));
	imagelist.Add(theApp.LoadIcon(IDI_FRIENDS1));
	imagelist.Add(theApp.LoadIcon(IDI_FRIENDS2));
	imagelist.Add(theApp.LoadIcon(IDI_FRIENDS3));
	SetImageList(&imagelist,LVSIL_SMALL);
	theApp.friendlist->SetWindow(this);
	theApp.friendlist->ShowFriends();
}

void CFriendListCtrl::Localize() {
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;
	
	strRes = GetResString(IDS_QL_USERNAME);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(0, &hdi);
	strRes.ReleaseBuffer();
}

void CFriendListCtrl::AddFriend(CFriend* toadd){
	uint32 itemnr = GetItemCount();
	itemnr = InsertItem(LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE,itemnr,toadd->m_strName.GetBuffer(),0,0,1,(LPARAM)toadd);
	RefreshFriend(toadd);
}

void CFriendListCtrl::RemoveFriend(CFriend* toremove){
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)toremove;
	sint32 result = FindItem(&find);
	if (result != (-1) )
		DeleteItem(result);
}

void CFriendListCtrl::RefreshFriend(CFriend* toupdate){
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)toupdate;
	sint32 itemnr = FindItem(&find);
	CString temp;
	temp.Format( "%s", toupdate->m_strName );
	SetItemText(itemnr,0,(LPCTSTR)temp);
	if (itemnr == (-1))
		return;
	uint8 image;
//MORPH START - Added by Yun.SF3, ZZ Upload System
	if (!toupdate->GetLinkedClient())
		image = 0;
	else if (toupdate->GetLinkedClient()->socket && toupdate->GetLinkedClient()->socket->IsConnected())
//MORPH END - Added by Yun.SF3, ZZ Upload System
		image = 2;
	else
		image = 1;
	SetItem(itemnr,0,LVIF_IMAGE,0,image,0,0,0,0);
}

void CFriendListCtrl::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult){	
	POINT point;
	::GetCursorPos(&point);	
	CFriend* cur_friend=NULL;

	if (m_ClientMenu) VERIFY( m_ClientMenu.DestroyMenu() );
	m_ClientMenu.CreatePopupMenu();
	m_ClientMenu.AddMenuTitle(GetResString(IDS_FRIENDLIST));

	if (GetSelectionMark() != (-1)){
		cur_friend = (CFriend*)GetItemData(GetSelectionMark());

		m_ClientMenu.AppendMenu(MF_STRING,MP_DETAIL, GetResString(IDS_SHOWDETAILS));
	}

	m_ClientMenu.AppendMenu(MF_STRING,MP_ADDFRIEND, GetResString(IDS_ADDAFRIEND));

	if (GetSelectionMark() != (-1)){

		m_ClientMenu.AppendMenu(MF_STRING,MP_REMOVEFRIEND, GetResString(IDS_REMOVEFRIEND));
		m_ClientMenu.AppendMenu(MF_STRING,MP_MESSAGE, GetResString(IDS_SEND_MSG));
		m_ClientMenu.AppendMenu(MF_STRING,MP_SHOWLIST, GetResString(IDS_VIEWFILES));
		m_ClientMenu.AppendMenu(MF_STRING,MP_FRIENDSLOT, GetResString(IDS_FRIENDSLOT));
		//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System
		//if (cur_friend && cur_friend->GetLinkedClient() && !cur_friend->GetLinkedClient()->HasLowID()){
		m_ClientMenu.EnableMenuItem(MP_FRIENDSLOT,MF_ENABLED);
		m_ClientMenu.CheckMenuItem(MP_FRIENDSLOT, ((cur_friend->GetFriendSlot())?MF_CHECKED : MF_UNCHECKED) );  
		//MORPH START - Added by IceCream, List Requested Files
		m_ClientMenu.AppendMenu(MF_SEPARATOR); // Added by sivka [sivka: -listing all requested files from user-]
		m_ClientMenu.AppendMenu(MF_STRING,MP_LIST_REQUESTED_FILES, _T(GetResString(IDS_LISTREQUESTED))); // Added by sivka
		//MORPH END - Added by IceCream, List Requested Files	
		//}
		//else
		//	m_ClientMenu.EnableMenuItem(MP_FRIENDSLOT,MF_GRAYED);
		//MORPH END - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System
	}

	//SetMenu(&m_ClientMenu);
	m_ClientMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	*pResult = 0;
	VERIFY( m_ClientMenu.DestroyMenu() );
}

BOOL CFriendListCtrl::OnCommand(WPARAM wParam,LPARAM lParam ){
	CFriend* cur_friend = NULL;
	if (GetSelectionMark() != (-1)) 
		cur_friend = (CFriend*)GetItemData(GetSelectionMark());
	switch (wParam){
		case MP_MESSAGE:{
			if (cur_friend){
				//MORPH - Added by Yun.SF3, ZZ Upload System
				if (cur_friend->GetLinkedClient())
					theApp.emuledlg->chatwnd.StartSession(cur_friend->GetLinkedClient());
				//MORPH - Added by Yun.SF3, ZZ Upload System
				else{
					CUpDownClient* chatclient = new CUpDownClient(0,cur_friend->m_nLastUsedPort,cur_friend->m_dwLastUsedIP,0,0,true);
					chatclient->SetUserName(cur_friend->m_strName.GetBuffer());
					theApp.clientlist->AddClient(chatclient);
					theApp.emuledlg->chatwnd.StartSession(chatclient);
				}
			}
			break;
		}
		case MP_REMOVEFRIEND:{
			if (cur_friend)
				theApp.friendlist->RemoveFriend(cur_friend);
			break;
		}
		case MP_ADDFRIEND:{
			CAddFriend dialog2; 
			dialog2.DoModal();
			break;
		}
		case MPG_ALTENTER:
		case MP_DETAIL:
			if (cur_friend)
				ShowFriendDetails(cur_friend);
			break;
		case MP_SHOWLIST:
		{
			if (cur_friend){
				//MORPH - Added by Yun.SF3, ZZ Upload System
				if (cur_friend->GetLinkedClient())
					cur_friend->GetLinkedClient()->RequestSharedFileList();
				//MORPH - Added by Yun.SF3, ZZ Upload System
				else{
					CUpDownClient* newclient = new CUpDownClient(0,cur_friend->m_nLastUsedPort,cur_friend->m_dwLastUsedIP,0,0,true);
					newclient->SetUserName(cur_friend->m_strName.GetBuffer());
					theApp.clientlist->AddClient(newclient);
					newclient->RequestSharedFileList();
				}
			}
			break;
		}
		//MORPH START - Added by IceCream, List Requested Files
		case MP_LIST_REQUESTED_FILES: {
			if (cur_friend && cur_friend->GetLinkedClient())
			{
				CString fileList;
				fileList += GetResString(IDS_LISTREQDL);
				fileList += "\n--------------------------\n" ; 
				if (theApp.downloadqueue->IsPartFile(cur_friend->GetLinkedClient()->reqfile))
				{
					fileList += cur_friend->GetLinkedClient()->reqfile->GetFileName(); 
					for(POSITION pos = cur_friend->GetLinkedClient()->m_OtherRequests_list.GetHeadPosition();pos!=0;cur_friend->GetLinkedClient()->m_OtherRequests_list.GetNext(pos))
					{
						fileList += "\n" ; 
						fileList += cur_friend->GetLinkedClient()->m_OtherRequests_list.GetAt(pos)->GetFileName(); 
					}
				}
				else
					fileList += GetResString(IDS_LISTREQNODL);
				fileList += "\n\n\n";
				fileList += GetResString(IDS_LISTREQUL);
				fileList += "\n------------------------\n" ; 
				CKnownFile* uploadfile = theApp.sharedfiles->GetFileByID(cur_friend->GetLinkedClient()->GetUploadFileID());
				if(uploadfile)
					fileList += uploadfile->GetFileName();
				else
					fileList += GetResString(IDS_LISTREQNOUL);
				AfxMessageBox(fileList,MB_OK);
			}
			break;
		}
		//MORPH END - Added by IceCream, List Requested Files
		case MP_FRIENDSLOT:
		{
			//MORPH START - Modified by SIRoB, Added by Yun.SF3, ZZ Upload System
			if (cur_friend){
				bool IsAlready;
				IsAlready = cur_friend->GetFriendSlot();
				//theApp.friendlist->RemoveAllFriendSlots();
				if( IsAlready ) {
					cur_friend->SetFriendSlot(false);
				} else {
					cur_friend->SetFriendSlot(true);
				}
			}
			//MORPH END - Modified by SIRoB, Added by Yun.SF3, ZZ Upload System
			break;
		}
	}
	return true;
}

void CFriendListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult) {
	int iSel = GetSelectionMark();
	if (iSel != -1)
		ShowFriendDetails((CFriend*)GetItemData(iSel));
	*pResult = 0;
}

void CFriendListCtrl::ShowFriendDetails(CFriend* pFriend) {
	if (pFriend){
		//MORPH - Added by Yun.SF3, ZZ Upload System
		if (pFriend->GetLinkedClient()){
			CClientDetailDialog dialog(pFriend->GetLinkedClient());
		//MORPH - Added by Yun.SF3, ZZ Upload System
			dialog.DoModal();
		}
		else{
			CAddFriend dlg;
			dlg.m_pShowFriend = pFriend;
			dlg.DoModal();
		}
	}
}

BOOL CFriendListCtrl::PreTranslateMessage(MSG* pMsg) 
{
   	if ( pMsg->message == 260 && pMsg->wParam == 13 && GetAsyncKeyState(VK_MENU)<0 ) {
		PostMessage(WM_COMMAND, MPG_ALTENTER, 0);
		return TRUE;
	}

	return CListCtrl::PreTranslateMessage(pMsg);
}