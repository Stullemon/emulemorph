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
#include "FriendListCtrl.h"
#include "friend.h"
#include "ClientDetailDialog.h"
#include "Addfriend.h"
#include "FriendList.h"
#include "emuledlg.h"
#include "ClientList.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
#include "ListenSocket.h"
#include "MenuCmds.h"
#include "ChatWnd.h"
#include "DownloadQueue.h" //MORPH - Added by SiRoB
#include "PartFile.h" //MORPH - Added by SiRoB
#include "SharedFileList.h" //MORPH - Added by SiRoB

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CFriendListCtrl

IMPLEMENT_DYNAMIC(CFriendListCtrl, CMuleListCtrl)
CFriendListCtrl::CFriendListCtrl()
{
}

CFriendListCtrl::~CFriendListCtrl()
{
}


BEGIN_MESSAGE_MAP(CFriendListCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnLvnColumnclick)
END_MESSAGE_MAP()



// CFriendListCtrl message handlers

void CFriendListCtrl::Init()
{
	SetExtendedStyle(LVS_EX_FULLROWSELECT);

	RECT rcWindow;
	GetWindowRect(&rcWindow);
	InsertColumn(0, GetResString(IDS_QL_USERNAME), LVCFMT_LEFT, rcWindow.right - rcWindow.left - 4, 0);
	SetAllIcons();
	theApp.friendlist->SetWindow(this);
	SetSortArrow(0, true);
}

void CFriendListCtrl::OnSysColorChange()
{
	CMuleListCtrl::OnSysColorChange();
	SetAllIcons();
}

void CFriendListCtrl::SetAllIcons()
{
	CImageList iml;
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.SetBkColor(CLR_NONE);
	iml.Add(CTempIconLoader("FriendNoClient"));
	iml.Add(CTempIconLoader("FriendWithClient"));
	iml.Add(CTempIconLoader("FriendConnected"));
	//MORPH START - Added by SiRoB, Friend Addon
	iml.Add(CTempIconLoader("FriendNoClientSlot"));
	iml.Add(CTempIconLoader("FriendWithClientSlot"));
	iml.Add(CTempIconLoader("FriendConnectedSlot"));
	//MORPH END   - Added by SiRoB, Friend Addon

	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	HIMAGELIST himlOld = ApplyImageList(iml.Detach());
	if (himlOld)
		ImageList_Destroy(himlOld);
}

void CFriendListCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	strRes = GetResString(IDS_QL_USERNAME);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(0, &hdi);
	strRes.ReleaseBuffer();
}

void CFriendListCtrl::AddFriend(const CFriend* toadd)
{
	int itemnr = GetItemCount();
	InsertItem(LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE,itemnr,(LPCTSTR)toadd->m_strName,0,0,1,(LPARAM)toadd);
	RefreshFriend(toadd);
	theApp.emuledlg->chatwnd->UpdateFriendlistCount(theApp.friendlist->GetCount());
}

void CFriendListCtrl::RemoveFriend(const CFriend* toremove)
{
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)toremove;
	int result = FindItem(&find);
	if (result != -1)
		DeleteItem(result);
	theApp.emuledlg->chatwnd->UpdateFriendlistCount(theApp.friendlist->GetCount());
}

void CFriendListCtrl::RefreshFriend(const CFriend* toupdate)
{
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)toupdate;
	int itemnr = FindItem(&find);
	if (itemnr != -1)
	{
	    // Mighty Knife: log friend activities
		//CString temp;
		//temp.Format( "%s", toupdate->m_strName );
		CString OldName = GetItemText (itemnr,0);
		if ((OldName != toupdate->m_strName) && (thePrefs.GetLogFriendlistActivities ())) {
  			char buffer[100]; buffer [0] = 0;
			for (uint16 i = 0;i != 16;i++) sprintf(buffer,"%s%02X",buffer,toupdate->m_abyUserhash[i]);
			#ifdef MIGHTY_TWEAKS
			theApp.emuledlg->AddLogLine(false, "Friend changed his name: '%s'->'%s', ip %i.%i.%i.%i:%i, hash %s",
										(LPCTSTR) OldName, (LPCTSTR) toupdate->m_strName, (uint8)toupdate->m_dwLastUsedIP, 
										(uint8)(toupdate->m_dwLastUsedIP>>8), 
										(uint8)(toupdate->m_dwLastUsedIP>>16),(uint8)(toupdate->m_dwLastUsedIP>>24), 
										toupdate->m_nLastUsedPort, buffer);
			#else
			theApp.emuledlg->AddLogLine(false, "Friend changed his name: '%s'->'%s', hash %s",
										(LPCTSTR) OldName, (LPCTSTR) toupdate->m_strName, buffer);
			#endif
		}
		// [end] Mighty Knife

		SetItemText(itemnr,0,(LPCTSTR)toupdate->m_strName);
		int image;
		
		//MORPH START - Added by Yun.SF3, ZZ Upload System
		if (!toupdate->GetLinkedClient())
			image = 0;
		else if (toupdate->GetLinkedClient()->socket && toupdate->GetLinkedClient()->socket->IsConnected())
		//MORPH END - Added by Yun.SF3, ZZ Upload System
			image = 2;
		else
			image = 1;
		//MORPH START - Added by SiRoB, 
		if (toupdate->GetFS()) image += 3;
		//MORPH END   - Added by SiRoB, 
		
		SetItem(itemnr,0,LVIF_IMAGE,0,image,0,0,0,0);
	}
	else
		ASSERT(0);
}

void CFriendListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CTitleMenu ClientMenu;
	ClientMenu.CreatePopupMenu();
	ClientMenu.AddMenuTitle(GetResString(IDS_FRIENDLIST));

	const CFriend* cur_friend = NULL;
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1){
		cur_friend = (CFriend*)GetItemData(iSel);
		ClientMenu.AppendMenu(MF_STRING,MP_DETAIL, GetResString(IDS_SHOWDETAILS));
		ClientMenu.SetDefaultItem(MP_DETAIL);
	}

	ClientMenu.AppendMenu(MF_STRING,MP_ADDFRIEND, GetResString(IDS_ADDAFRIEND));
	ClientMenu.AppendMenu(MF_STRING | (cur_friend ? MF_ENABLED : MF_GRAYED), MP_REMOVEFRIEND, GetResString(IDS_REMOVEFRIEND));
	ClientMenu.AppendMenu(MF_STRING | (cur_friend ? MF_ENABLED : MF_GRAYED), MP_MESSAGE, GetResString(IDS_SEND_MSG));
	ClientMenu.AppendMenu(MF_STRING | ((cur_friend==NULL || (cur_friend && cur_friend->m_LinkedClient && !cur_friend->m_LinkedClient->GetViewSharedFilesSupport())) ? MF_GRAYED : MF_ENABLED), MP_SHOWLIST, GetResString(IDS_VIEWFILES));
	//MORPH START - Modified by SiRoB, ZZ Upload System
	/*
	ClientMenu.AppendMenu(MF_STRING, MP_FRIENDSLOT, GetResString(IDS_FRIENDSLOT));
	if (cur_friend && cur_friend->m_LinkedClient && !cur_friend->m_LinkedClient->HasLowID()){
		ClientMenu.EnableMenuItem(MP_FRIENDSLOT, MF_ENABLED);
		ClientMenu.CheckMenuItem(MP_FRIENDSLOT, (cur_friend->m_LinkedClient->GetFriendSlot()) ? MF_CHECKED : MF_UNCHECKED);
	}
	else
		ClientMenu.EnableMenuItem(MP_FRIENDSLOT, MF_GRAYED);
	*/
	ClientMenu.AppendMenu(MF_STRING | (cur_friend? MF_ENABLED | ((!cur_friend->m_LinkedClient->HasLowID() && cur_friend->m_LinkedClient->GetFriendSlot())? MF_CHECKED : MF_UNCHECKED) : MF_GRAYED) , MP_FRIENDSLOT, GetResString(IDS_FRIENDSLOT));
	//MORPH START - Added by SiRoB, Friend Addon
	ClientMenu.AppendMenu(MF_STRING,MP_REMOVEALLFRIENDSLOT, GetResString(IDS_REMOVEALLFRIENDSLOT));
	//MORPH END   - Added by SiRoB, Friend Addon
	//MORPH START - Added by IceCream, List Requested Files
	ClientMenu.AppendMenu(MF_SEPARATOR); // Added by sivka [sivka: -listing all requested files from user-]
	ClientMenu.AppendMenu(MF_STRING,MP_LIST_REQUESTED_FILES, GetResString(IDS_LISTREQUESTED)); // Added by sivka
	//MORPH END - Added by IceCream, List Requested Files	

	GetPopupMenuPos(*this, point);
	ClientMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
}

BOOL CFriendListCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	CFriend* cur_friend = NULL;
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1) 
		cur_friend = (CFriend*)GetItemData(iSel);
	
	switch (wParam){
		case MP_MESSAGE:{
			if (cur_friend){
				//MORPH - Added by Yun.SF3, ZZ Upload System
				if (cur_friend->GetLinkedClient())
					theApp.emuledlg->chatwnd->StartSession(cur_friend->GetLinkedClient());
				//MORPH - Added by Yun.SF3, ZZ Upload System
				else{
					CUpDownClient* chatclient = new CUpDownClient(0,cur_friend->m_nLastUsedPort,cur_friend->m_dwLastUsedIP,0,0,true);
					chatclient->SetUserName(cur_friend->m_strName.GetBuffer());
					theApp.clientlist->AddClient(chatclient);
					theApp.emuledlg->chatwnd->StartSession(chatclient);
				}
			}
			break;
		}
		case MP_REMOVEFRIEND:{
			if (cur_friend){
				theApp.friendlist->RemoveFriend(cur_friend);
				// auto select next item after deleted one.
				if (iSel < GetItemCount()){
					SetSelectionMark(iSel);
					SetItemState(iSel, LVIS_SELECTED, LVIS_SELECTED);
				}
			}
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
		//MORPH START - Added by SiRoB, Friend Addon
		case MP_REMOVEALLFRIENDSLOT:
			theApp.friendlist->RemoveAllFriendSlots();	
			break;
		//MORPH START - Added by SiRoB, Friend Addon

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
					for(POSITION pos = cur_friend->GetLinkedClient()->m_OtherNoNeeded_list.GetHeadPosition();pos!=0;cur_friend->GetLinkedClient()->m_OtherNoNeeded_list.GetNext(pos))
					{
						fileList += "\n" ;
						fileList += cur_friend->GetLinkedClient()->m_OtherNoNeeded_list.GetAt(pos)->GetFileName();
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
			//MORPH START - Added by SiRoB, Friend Addon
			RefreshFriend(cur_friend); //KTS
			//MORPH END   - Added by SiRoB, Friend Addon
			break;
		}
	}
	return true;
}

void CFriendListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1) 
		ShowFriendDetails((CFriend*)GetItemData(iSel));
	*pResult = 0;
}

void CFriendListCtrl::ShowFriendDetails(const CFriend* pFriend)
{
	if (pFriend){
		//MORPH - Added by Yun.SF3, ZZ Upload System
		if (pFriend->GetLinkedClient()){
			CClientDetailDialog dialog(pFriend->GetLinkedClient());
		//MORPH - Added by Yun.SF3, ZZ Upload System
			dialog.DoModal();
		}
		else{
			CAddFriend dlg;
			dlg.m_pShowFriend = const_cast<CFriend*>(pFriend);
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
	else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_DELETE)
		PostMessage(WM_COMMAND, MP_REMOVEFRIEND, 0);
	else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_INSERT)
		PostMessage(WM_COMMAND, MP_ADDFRIEND, 0);

	return CMuleListCtrl::PreTranslateMessage(pMsg);
}

void CFriendListCtrl::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Determine ascending based on whether already sorted on this column
	int iSortItem = GetSortItem();
	bool bOldSortAscending = GetSortAscending();
	bool bSortAscending = (iSortItem != pNMListView->iSubItem) ? true : !bOldSortAscending;

	// Item is column clicked
	iSortItem = pNMListView->iSubItem;

	// Sort table
	SetSortArrow(iSortItem, bSortAscending);
	SortItems(SortProc, MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));

	*pResult = 0;
}

int CFriendListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CFriend* item1 = (CFriend*)lParam1;
	CFriend* item2 = (CFriend*)lParam2; 
	if (item1 == NULL || item2 == NULL)
		return 0;

	int iResult;
	switch (LOWORD(lParamSort))
	{
		case 0:
			iResult = _tcsicmp(item1->m_strName, item2->m_strName);
			break;
		default:
			return 0;
	}
	if (HIWORD(lParamSort))
		iResult = -iResult;
	return iResult;
}

void CFriendListCtrl::UpdateList()
{
	theApp.emuledlg->chatwnd->UpdateFriendlistCount(theApp.friendlist->GetCount());
	SortItems(SortProc, MAKELONG(GetSortItem(), (GetSortAscending() ? 0 : 0x0001)));
}