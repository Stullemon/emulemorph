//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "ClientListCtrl.h"
#include "otherfunctions.h"
#include "MenuCmds.h"
#include "ClientDetailDialog.h"
#include "KademliaWnd.h"
#include "ClientList.h"
#include "emuledlg.h"
#include "FriendList.h"
#include "TransferWnd.h"
#include "MemDC.h"
#include "UpDownClient.h"
#include "ClientCredits.h"
#include "ListenSocket.h"
#include "ChatWnd.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "Kademlia/net/KademliaUDPListener.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CClientListCtrl

IMPLEMENT_DYNAMIC(CClientListCtrl, CMuleListCtrl)
CClientListCtrl::CClientListCtrl(){
}

void CClientListCtrl::Init()
{
	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy,theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	SetExtendedStyle(LVS_EX_FULLROWSELECT);
	InsertColumn(0,GetResString(IDS_QL_USERNAME),LVCFMT_LEFT,150,0);
	InsertColumn(1,GetResString(IDS_CL_UPLOADSTATUS),LVCFMT_LEFT,150,1);
	InsertColumn(2,GetResString(IDS_CL_TRANSFUP),LVCFMT_LEFT,150,2);
	InsertColumn(3,GetResString(IDS_CL_DOWNLSTATUS),LVCFMT_LEFT,150,3);
	InsertColumn(4,GetResString(IDS_CL_TRANSFDOWN),LVCFMT_LEFT,150,4);
	CString coltemp;coltemp=GetResString(IDS_CD_CSOFT);coltemp.Remove(':');
	InsertColumn(5,coltemp,LVCFMT_LEFT,150,5);
	InsertColumn(6,GetResString(IDS_CONNECTED),LVCFMT_LEFT,150,6);
	coltemp=GetResString(IDS_CD_UHASH);coltemp.Remove(':');
	InsertColumn(7,coltemp,LVCFMT_LEFT,150,7);
	
	// Mighty Knife: Community affiliation
	InsertColumn(8,GetResString(IDS_COMMUNITY),LVCFMT_LEFT,100,8);
	// [end] Mighty Knife

	// EastShare - Added by Pretender, Friend Tab
	InsertColumn(9,GetResString(IDS_FRIENDLIST),LVCFMT_LEFT,75,9);
	// EastShare - Added by Pretender, Friend Tab
    // Commander - Added: IP2Country column - Start
	InsertColumn(10,GetResString(IDS_COUNTRY),LVCFMT_LEFT,100,10);
    // Commander - Added: IP2Country column - End

	SetAllIcons();
	Localize();
	LoadSettings(CPreferences::tableClientList);
	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableClientList);
	bool sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableClientList);
	SetSortArrow(sortItem, sortAscending);
	// SLUGFILLER: multiSort - load multiple params
	for (int i = thePrefs.GetColumnSortCount(CPreferences::tableClientList); i > 0; ) {
		i--;
		sortItem = thePrefs.GetColumnSortItem(CPreferences::tableClientList, i);
		sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableClientList, i);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));
	}
	// SLUGFILLER: multiSort
/*
	// Mighty Knife: Community affiliation
	if (thePrefs.IsCommunityEnabled ()) ShowColumn (8);
	else HideColumn (8);
	// [end] Mighty Knife
*/
// Commander - Added: IP2Country column - Start
	if (thePrefs.GetIP2CountryNameMode() == IP2CountryName_DISABLE)
	    HideColumn (10);
// Commander - Added: IP2Country column - End
}

CClientListCtrl::~CClientListCtrl()
{
}

void CClientListCtrl::OnSysColorChange()
{
	CMuleListCtrl::OnSysColorChange();
	SetAllIcons();
}

void CClientListCtrl::SetAllIcons()
{
	imagelist.DeleteImageList();
	imagelist.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	imagelist.SetBkColor(CLR_NONE);
	imagelist.Add(CTempIconLoader(_T("ClientEDonkey")));
	imagelist.Add(CTempIconLoader(_T("ClientCompatible")));
	imagelist.Add(CTempIconLoader(_T("Friend")));
	imagelist.Add(CTempIconLoader(_T("ClientMLDonkey")));
	imagelist.Add(CTempIconLoader(_T("ClientEDonkeyHybrid")));
	imagelist.Add(CTempIconLoader(_T("ClientShareaza")));
	imagelist.Add(CTempIconLoader(_T("Server")));
	imagelist.Add(CTempIconLoader(_T("ClientAMule")));
	imagelist.Add(CTempIconLoader(_T("ClientLPhant")));
	//MORPH START - Added by SiRoB, More client icon & Credit ovelay icon
	imagelist.Add(CTempIconLoader(_T("ClientRightEdonkey")));
	imagelist.Add(CTempIconLoader(_T("Morph")));
	//MORPH END   - Added by SiRoB, More client icon & Credit ovelay icon
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader(_T("ClientSecureOvl"))), 1);
	//MORPH START - Added by SiRoB, More client icon & Credit ovelay icon
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader(_T("ClientCreditOvl"))), 2);
	imagelist.SetOverlayImage(imagelist.Add(CTempIconLoader(_T("ClientCreditSecureOvl"))), 3);
	//MORPH END   - Added by SiRoB, More client icon & Credit ovelay icon

	// Mighty Knife: Community icon
	m_overlayimages.DeleteImageList ();
	m_overlayimages.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	m_overlayimages.SetBkColor(CLR_NONE);
	m_overlayimages.Add(CTempIconLoader(_T("Community")));
	// [end] Mighty Knife
	//MORPH START - Addded by SiRoB, Friend Addon
	m_overlayimages.Add(CTempIconLoader(_T("ClientFriendOvl")));
	m_overlayimages.Add(CTempIconLoader(_T("ClientFriendSlotOvl")));
	//MORPH END   - Addded by SiRoB, Friend Addon
}

void CClientListCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;

	if(pHeaderCtrl->GetItemCount() != 0) {
		CString strRes;

		strRes = GetResString(IDS_QL_USERNAME);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(0, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_CL_UPLOADSTATUS);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(1, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_CL_TRANSFUP);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(2, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_CL_DOWNLSTATUS);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(3, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_CL_TRANSFDOWN);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(4, &hdi);
		strRes.ReleaseBuffer();

		strRes=GetResString(IDS_CD_CSOFT);strRes.Remove(':');
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(5, &hdi);
		strRes.ReleaseBuffer();

		strRes = GetResString(IDS_CONNECTED);
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(6, &hdi);
		strRes.ReleaseBuffer();

		strRes=GetResString(IDS_CD_UHASH);strRes.Remove(':');
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(7, &hdi);
		strRes.ReleaseBuffer();
	}
}

void CClientListCtrl::ShowKnownClients()
{
	DeleteAllItems(); 
	int iItemCount = 0;
	for(POSITION pos = theApp.clientlist->list.GetHeadPosition(); pos != NULL;){
		const CUpDownClient* cur_client = theApp.clientlist->list.GetNext(pos);
		int iItem = InsertItem(LVIF_TEXT|LVIF_PARAM,iItemCount,LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)cur_client);
		Update(iItem);
		iItemCount++;
	}
	theApp.emuledlg->transferwnd->UpdateListCount(0, iItemCount);
}

void CClientListCtrl::AddClient(const CUpDownClient* client)
{
	if (!theApp.emuledlg->IsRunning())
		return;
	if (thePrefs.IsKnownClientListDisabled())
		return;

	int iItemCount = GetItemCount();
	int iItem = InsertItem(LVIF_TEXT|LVIF_PARAM,iItemCount,LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)client);
	Update(iItem);
	theApp.emuledlg->transferwnd->UpdateListCount(0, iItemCount+1);
}

void CClientListCtrl::RemoveClient(const CUpDownClient* client)
{
	if (!theApp.emuledlg->IsRunning())
		return;

	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	sint32 result = FindItem(&find);
	if (result != -1){
		DeleteItem(result);
		theApp.emuledlg->transferwnd->UpdateListCount(0);
	}
}

void CClientListCtrl::RefreshClient(const CUpDownClient* client)
{
	// There is some type of timing issue here.. If you click on item in the queue or upload and leave
	// the focus on it when you exit the cient, it breaks on line 854 of emuleDlg.cpp.. 
	// I added this IsRunning() check to this function and the DrawItem method and
	// this seems to keep it from crashing. This is not the fix but a patch until
	// someone points out what is going wrong.. Also, it will still assert in debug mode..
	if( !theApp.emuledlg->IsRunning())
		return;

	//MORPH START - SiRoB, Don't Refresh item if not needed
	if( theApp.emuledlg->activewnd != theApp.emuledlg->transferwnd)
		return;
	//MORPH END   - SiRoB, Don't Refresh item if not needed
	
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	sint16 result = FindItem(&find);
	if(result != -1)
		Update(result);
}

#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)

void CClientListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if( !theApp.emuledlg->IsRunning() )
		return;
	if (!lpDrawItemStruct->itemData)
		return;
	//MORPH START - Added by SiRoB, Don't draw hidden Rect
	RECT clientRect;
	GetClientRect(&clientRect);
	RECT cur_rec = lpDrawItemStruct->rcItem;
	if (cur_rec.top >= clientRect.bottom || cur_rec.bottom <= clientRect.top)
		return;
	//MORPH END   - Added by SiRoB, Don't draw hidden Rect
	CDC* odc = CDC::FromHandle(lpDrawItemStruct->hDC);
	BOOL bCtrlFocused = ((GetFocus() == this ) || (GetStyle() & LVS_SHOWSELALWAYS));
	if( (lpDrawItemStruct->itemAction | ODA_SELECT) && (lpDrawItemStruct->itemState & ODS_SELECTED )){
		if(bCtrlFocused)
			odc->SetBkColor(m_crHighlight);
		else
			odc->SetBkColor(m_crNoHighlight);
	}
	else
		odc->SetBkColor(GetBkColor());
	const CUpDownClient* client = (CUpDownClient*)lpDrawItemStruct->itemData;
	CMemDC dc(CDC::FromHandle(lpDrawItemStruct->hDC), &lpDrawItemStruct->rcItem);
	CFont* pOldFont = dc.SelectObject(GetFont());
	//MORPH - Moved by SiRoB, Don't draw hidden Rect
	/*
	RECT cur_rec = lpDrawItemStruct->rcItem;
	*/
	COLORREF crOldTextColor = dc.SetTextColor(m_crWindowText);

	int iOldBkMode;
	if (m_crWindowTextBk == CLR_NONE){
		DefWindowProc(WM_ERASEBKGND, (WPARAM)(HDC)dc, 0);
		iOldBkMode = dc.SetBkMode(TRANSPARENT);
	}
	else
		iOldBkMode = OPAQUE;

	CString Sbuffer;

	CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	cur_rec.right = cur_rec.left - 8;
	cur_rec.left += 4;

	for(int iCurrent = 0; iCurrent < iCount; iCurrent++){
		int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
		if( !IsColumnHidden(iColumn) ){
			cur_rec.right += GetColumnWidth(iColumn);
			//MORPH START - Added by SiRoB, Don't draw hidden columns
			if (cur_rec.left < clientRect.right && cur_rec.right > clientRect.left)
			{
			//MORPH END   - Added by SiRoB, Don't draw hidden columns
				switch(iColumn){
					case 0:{
						uint8 image;
						//MORPH - Removed by SiRoB, Friend Addon
						/*
						if (client->IsFriend())
							image = 2;
						
						else*/ if (client->GetClientSoft() == SO_EDONKEYHYBRID)
							image = 4;
						else if (client->GetClientSoft() == SO_MLDONKEY)
							image = 3;
						else if (client->GetClientSoft() == SO_SHAREAZA)
							image = 5;
						else if (client->GetClientSoft() == SO_URL)
							image = 6;
						else if (client->GetClientSoft() == SO_AMULE)
							image = 7;
						else if (client->GetClientSoft() == SO_LPHANT)
							image = 8;
						else if (client->ExtProtocolAvailable())
						//MORPH START - Modified by SiRoB, More client icon & Credit overlay icon
							image = (client->IsMorph())?10:1;
						else if (client->GetClientSoft() == SO_EDONKEY)
							image = 9;
						else
							image = 0;

						POINT point = {cur_rec.left, cur_rec.top+1};
						imagelist.Draw(dc,image, point, ILD_NORMAL | ((client->Credits() && client->Credits()->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED) ? INDEXTOOVERLAYMASK(1) : 0));
						// Mighty Knife: Community visualization
						if (client->IsCommunity())
							m_overlayimages.Draw(dc,0, point, ILD_NORMAL/*ILD_TRANSPARENT*/);
						// [end] Mighty Knife
						if (client->IsFriend())
							m_overlayimages.Draw(dc,client->GetFriendSlot()?2:1, point, ILD_NORMAL);
						//MORPH END - Modified by SiRoB, More client icon
						if (client->GetUserName()==NULL)
							Sbuffer.Format(_T("(%s)"), GetResString(IDS_UNKNOWN));
						else
							Sbuffer = client->GetUserName();

						//EastShare Start - added by AndCycle, IP to Country, modified by Commander
						//CString tempStr;
						//tempStr.Format("%s%s", client->GetCountryName(), Sbuffer);
						//Sbuffer = tempStr;
											//Commander: There is a column now to show the country name
						if(theApp.ip2country->ShowCountryFlag()){
							cur_rec.left+=20;
							POINT point2= {cur_rec.left,cur_rec.top+1};
							int index = client->GetCountryFlagIndex();
							theApp.ip2country->GetFlagImageList()->DrawIndirect(dc, index , point2, CSize(18,16), CPoint(0,0), ILD_NORMAL);
						}
						//EastShare End - added by AndCycle, IP to Country

						cur_rec.left +=20;
						dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
						cur_rec.left -=20;

						//EastShare Start - added by AndCycle, IP to Country
						if(theApp.ip2country->ShowCountryFlag()){
							cur_rec.left-=20;
						}
						//EastShare End - added by AndCycle, IP to Country

						break;
					}
					case 1:{
						Sbuffer = client->GetUploadStateDisplayString();
						break;
					}
					case 2:{
						if(client->credits)
							Sbuffer = CastItoXBytes(client->credits->GetUploadedTotal(), false, false);
						else
							Sbuffer.Empty();
						break;
					}
					case 3:{
						Sbuffer = client->GetDownloadStateDisplayString();
						break;
					}
					case 4:{
						if(client->credits)
							Sbuffer = CastItoXBytes(client->credits->GetDownloadedTotal(), false, false);
						else
							Sbuffer.Empty();
						break;
					}
					case 5:{
						Sbuffer = client->GetClientSoftVer();
						if (Sbuffer.IsEmpty())
							Sbuffer = GetResString(IDS_UNKNOWN);
						break;
					}
					case 6:{
						if(client->socket){
							if(client->socket->IsConnected()){
								Sbuffer = GetResString(IDS_YES);
								break;
							}
						}
						Sbuffer = GetResString(IDS_NO);
						break;
					}
					case 7:
						Sbuffer = md4str(client->GetUserHash());
						break;
					// Mighty Knife: Community affiliation
					case 8:
						Sbuffer = client->IsCommunity () ? GetResString(IDS_YES) : _T("");
						break;
					// [end] Mighty Knife
					// EastShare - Added by Pretender, Friend Tab
					case 9:
						Sbuffer = client->IsFriend () ? GetResString(IDS_YES) : _T("");
						break;
					// EastShare - Added by Pretender, Friend Tab
					// Commander - Added: IP2Country column - Start
					case 10:
						Sbuffer.Format(_T("%s"), client->GetCountryName());
						break;
					// Commander - Added: IP2Country column - End
				}
				if( iColumn != 0)
					dc->DrawText(Sbuffer,Sbuffer.GetLength(),&cur_rec,DLC_DT_TEXT);
			}//MORPH - Added by SiRoB, Don't draw hidden colums
			cur_rec.left += GetColumnWidth(iColumn);
		}
	}
/*	
	// Mighty Knife: Community affiliation
	// Show/Hide community column if changed in the preferences
	if (thePrefs.IsCommunityEnabled () != !IsColumnHidden (8))
		if (thePrefs.IsCommunityEnabled ())
				ShowColumn (8);
		else HideColumn (8);
	// [end] Mighty Knife

    // Commander - Added: IP2Country column - Start
		if ((thePrefs.GetIP2CountryNameMode() == IP2CountryName_DISABLE) && !IsColumnHidden(10))
			HideColumn (10);
    // Commander - Added: IP2Country column - End
*/
	//draw rectangle around selected item(s)
	if ((lpDrawItemStruct->itemAction | ODA_SELECT) && (lpDrawItemStruct->itemState & ODS_SELECTED))
	{
		RECT outline_rec = lpDrawItemStruct->rcItem;

		outline_rec.top--;
		outline_rec.bottom++;
		dc->FrameRect(&outline_rec, &CBrush(GetBkColor()));
		outline_rec.top++;
		outline_rec.bottom--;
		outline_rec.left++;
		outline_rec.right--;

		if(bCtrlFocused)
			dc->FrameRect(&outline_rec, &CBrush(m_crFocusLine));
		else
			dc->FrameRect(&outline_rec, &CBrush(m_crNoFocusLine));
	}

	if (m_crWindowTextBk == CLR_NONE)
		dc.SetBkMode(iOldBkMode);
	dc.SelectObject(pOldFont);
	dc.SetTextColor(crOldTextColor);
}

BEGIN_MESSAGE_MAP(CClientListCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
END_MESSAGE_MAP()

// CClientListCtrl message handlers
	
void CClientListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	const CUpDownClient* client = (iSel != -1) ? (CUpDownClient*)GetItemData(iSel) : NULL;

	CTitleMenu ClientMenu;
	ClientMenu.CreatePopupMenu();
	ClientMenu.AddMenuTitle(GetResString(IDS_CLIENTS),true);
	ClientMenu.AppendMenu(MF_STRING | (client ? MF_ENABLED : MF_GRAYED), MP_DETAIL, GetResString(IDS_SHOWDETAILS), _T("CLIENTDETAILS"));
	ClientMenu.SetDefaultItem(MP_DETAIL);
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && !client->IsFriend()) ? MF_ENABLED : MF_GRAYED), MP_ADDFRIEND, GetResString(IDS_ADDFRIEND), _T("ADDFRIEND"));
	//MORPH START - Added by SiRoB, Friend Addon
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsFriend()) ? MF_ENABLED : MF_GRAYED), MP_REMOVEFRIEND, GetResString(IDS_REMOVEFRIEND), _T("DELETEFRIEND"));
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsFriend()) ? MF_ENABLED  | ((!client->HasLowID() && client->GetFriendSlot())?MF_CHECKED : MF_UNCHECKED) : MF_GRAYED), MP_FRIENDSLOT, GetResString(IDS_FRIENDSLOT), _T("FRIENDSLOT"));
	//MORPH END - Added by SiRoB, Friend Addon
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient()) ? MF_ENABLED : MF_GRAYED), MP_MESSAGE, GetResString(IDS_SEND_MSG), _T("SENDMESSAGE"));
	ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetViewSharedFilesSupport()) ? MF_ENABLED : MF_GRAYED), MP_SHOWLIST, GetResString(IDS_VIEWFILES), _T("VIEWFILES"));
	if (Kademlia::CKademlia::isRunning() && !Kademlia::CKademlia::isConnected())
		ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetKadPort()!=0) ? MF_ENABLED : MF_GRAYED), MP_BOOT, GetResString(IDS_BOOTSTRAP));
	//MORPH START - Added by Yun.SF3, List Requested Files
	ClientMenu.AppendMenu(MF_SEPARATOR); // Added by sivka [sivka: -listing all requested files from user-]
	ClientMenu.AppendMenu(MF_STRING,MP_LIST_REQUESTED_FILES, GetResString(IDS_LISTREQUESTED), _T("FILEREQUESTED")); // Added by sivka
	//MORPH END - Added by Yun.SF3, List Requested Files

	GetPopupMenuPos(*this, point);
	ClientMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
}

BOOL CClientListCtrl::OnCommand(WPARAM wParam,LPARAM lParam )
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1){
		CUpDownClient* client = (CUpDownClient*)GetItemData(iSel);
		switch (wParam){
			case MP_SHOWLIST:
				client->RequestSharedFileList();
				break;
			case MP_MESSAGE:
				theApp.emuledlg->chatwnd->StartSession(client);
				break;
			case MP_ADDFRIEND:
				if (theApp.friendlist->AddFriend(client))
					Update(iSel);
				break;
			case MP_UNBAN:
				if (client->IsBanned()){
					client->UnBan();
					Update(iSel);
				}
				break;
			case MPG_ALTENTER:
			case MP_DETAIL:{
				CClientDetailDialog dialog(client);
				dialog.DoModal();
				break;
			}
			case MP_BOOT:
				if (client->GetKadPort())
					Kademlia::CKademlia::bootstrap(ntohl(client->GetIP()), client->GetKadPort());
				break;
			//MORPH START - Added by SIRoB, Friend Addon
            		//Xman friendhandling
			case MP_FRIENDSLOT:{
				//MORPH START - Modified by SIRoB, Added by Yun.SF3, ZZ Upload System
				if (client){
					bool IsAlready;
					IsAlready = client->GetFriendSlot();
					//theApp.friendlist->RemoveAllFriendSlots();
					if( IsAlready ) {
						client->SetFriendSlot(false);
					} else {
						client->SetFriendSlot(true);
					}
					//KTS+
					for (POSITION pos = theApp.friendlist->m_listFriends.GetHeadPosition();pos != 0;theApp.friendlist->m_listFriends.GetNext(pos))
					{
						CFriend* cur_friend = theApp.friendlist->m_listFriends.GetAt(pos);
						theApp.friendlist->RefreshFriend(cur_friend);
					}
					//KTS-
				}
				//MORPH END - Modified by SIRoB, Added by Yun.SF3, ZZ Upload System
				break;
			}
			case MP_REMOVEFRIEND:{//LSD
				if (client && client->IsFriend())
				{					
					theApp.friendlist->RemoveFriend(client->m_Friend);
					//this->UpdateItem(client);
				}
				break;
			}
			//Xman end
			//MORPH END   - Added by SIRoB, Friend Addon
			//MORPH START - Added by Yun.SF3, List Requested Files
			case MP_LIST_REQUESTED_FILES: { // added by sivka
				if (client != NULL)
				{
					client->ShowRequestedFiles(); //Changed by SiRoB
				}
				break;
			}
			//MORPH END - Added by Yun.SF3, List Requested Files
		}
	}
	return true;
} 

void CClientListCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult){

	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableClientList);
	bool m_oldSortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableClientList);
	bool sortAscending = (sortItem != pNMListView->iSubItem) ? true : !m_oldSortAscending;
	// Item is column clicked
	sortItem = pNMListView->iSubItem;
	// Save new preferences
	thePrefs.SetColumnSortItem(CPreferences::tableClientList, sortItem);
	thePrefs.SetColumnSortAscending(CPreferences::tableClientList, sortAscending);
	// Sort table
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));

	*pResult = 0;
}

int CClientListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	const CUpDownClient* item1 = (CUpDownClient*)lParam1;
	const CUpDownClient* item2 = (CUpDownClient*)lParam2;
	switch(lParamSort){
		case 0: 
			if(item1->GetUserName() && item2->GetUserName())
				return CompareLocaleStringNoCase(item1->GetUserName(), item2->GetUserName());
			else if(item1->GetUserName())
				return 1;
			else
				return -1;
		case 100:
			if(item1->GetUserName() && item2->GetUserName())
				return CompareLocaleStringNoCase(item2->GetUserName(), item1->GetUserName());
			else if(item2->GetUserName())
				return 1;
			else
				return -1;
		case 1:
			return item1->GetUploadState()-item2->GetUploadState();
		case 101:
			return item2->GetUploadState()-item1->GetUploadState();
		case 2:
			if( item1->credits && item2->credits )
				return CompareUnsigned64(item1->credits->GetUploadedTotal(), item2->credits->GetUploadedTotal());
			else if( !item1->credits )
				return 1;
			else
				return -1;
		case 102:
			if( item1->credits && item2->credits )
				return CompareUnsigned64(item2->credits->GetUploadedTotal(), item1->credits->GetUploadedTotal());
			else if( !item1->credits )
				return 1;
			else
				return -1;
		case 3:
		    if( item1->GetDownloadState() == item2->GetDownloadState() ){
			    if( item1->IsRemoteQueueFull() && item2->IsRemoteQueueFull() )
				    return 0;
			    else if( item1->IsRemoteQueueFull() )
				    return 1;
			    else if( item2->IsRemoteQueueFull() )
				    return -1;
			    else
				    return 0;
		    }
			return item1->GetDownloadState()-item2->GetDownloadState();
		case 103:
		    if( item2->GetDownloadState() == item1->GetDownloadState() ){
			    if( item2->IsRemoteQueueFull() && item1->IsRemoteQueueFull() )
				    return 0;
			    else if( item2->IsRemoteQueueFull() )
				    return 1;
			    else if( item1->IsRemoteQueueFull() )
				    return -1;
			    else
				    return 0;
		    }
			return item2->GetDownloadState()-item1->GetDownloadState();
		case 4:
			if( item1->credits && item2->credits )
				return CompareUnsigned64(item1->credits->GetDownloadedTotal(), item2->credits->GetDownloadedTotal());
			else if( !item1->credits )
				return 1;
			else
				return -1;
		case 104:
			if( item1->credits && item2->credits )
				return CompareUnsigned64(item2->credits->GetDownloadedTotal(), item1->credits->GetDownloadedTotal());
			else if( !item1->credits )
				return 1;
			else
				return -1;
		//MORPH START - Added by IceCream, ET_MOD_VERSION
		case 5:
			if(item1->GetClientSoft() == item2->GetClientSoft())
				if(item2->GetVersion() == item1->GetVersion() && item1->GetClientSoft() == SO_EMULE){
					return CompareOptLocaleStringNoCase(item2->GetClientSoftVer(), item1->GetClientSoftVer());
				}
				else {
					return item2->GetVersion() - item1->GetVersion();
				}
			else
				return item1->GetClientSoft() - item2->GetClientSoft();
		case 105:
			if(item1->GetClientSoft() == item2->GetClientSoft())
				if(item2->GetVersion() == item1->GetVersion() && item1->GetClientSoft() == SO_EMULE){
					return CompareOptLocaleStringNoCase(item1->GetClientSoftVer(), item2->GetClientSoftVer());
				}
				else {
					return item1->GetVersion() - item2->GetVersion();
				}
			else
				return item2->GetClientSoft() - item1->GetClientSoft();
		//MORPH END   - Added by IceCream, ET_MOD_VERSION
		case 6:
			if( item1->socket && item2->socket )
				return item1->socket->IsConnected()-item2->socket->IsConnected();
			else if( !item1->socket )
				return -1;
			else
				return 1;
		case 106:
			if( item1->socket && item2->socket )
				return item2->socket->IsConnected()-item1->socket->IsConnected();
			else if( !item2->socket )
				return -1;
			else
				return 1;
		case 7:
			return memcmp(item1->GetUserHash(), item2->GetUserHash(), 16);
		case 107:
			return memcmp(item2->GetUserHash(), item1->GetUserHash(), 16);
		// Mighty Knife: Community affiliation
		case 8:
			return (item1->IsCommunity() && !item2->IsCommunity()) ? -1 : 1;
		case 108:
			return (item1->IsCommunity() && !item2->IsCommunity()) ? 1 : -1;
		// [end] Mighty Knife
		// EastShare - Added by Pretender, Friend Tab
		case 9:
			return (item1->IsFriend() && !item2->IsFriend()) ? -1 : 1;
		case 109:
			return (item1->IsFriend() && !item2->IsFriend()) ? 1 : -1;
		// EastShare - Added by Pretender, Friend Tab
        // Commander - Added: IP2Country column - Start
        case 10:
			if(item1->GetCountryName(true) && item2->GetCountryName(true))
				return CompareLocaleStringNoCase(item1->GetCountryName(true), item2->GetCountryName(true));
			else if(item1->GetCountryName(true))
				return 1;
			else
				return -1;

		case 110:
			if(item1->GetCountryName(true) && item2->GetCountryName(true))
				return CompareLocaleStringNoCase(item2->GetCountryName(true), item1->GetCountryName(true));
			else if(item2->GetCountryName(true))
				return 1;
			else
				return -1;
		// Commander - Added: IP2Country column - End		
		default:
			return 0;
	}
}

void CClientListCtrl::ShowSelectedUserDetails()
{
	POINT point;
	::GetCursorPos(&point);
	CPoint p = point; 
    ScreenToClient(&p); 
    int it = HitTest(p); 
    if (it == -1)
		return;

	SetItemState(-1, 0, LVIS_SELECTED);
	SetItemState(it, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	SetSelectionMark(it);   // display selection mark correctly! 

	const CUpDownClient* client = (CUpDownClient*)GetItemData(GetSelectionMark());
	if (client){
		CClientDetailDialog dialog(client);
		dialog.DoModal();
	}
}

void CClientListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult) {
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1) {
		const CUpDownClient* client = (CUpDownClient*)GetItemData(iSel);
		if (client){
			CClientDetailDialog dialog(client);
			dialog.DoModal();
		}
	}
	*pResult = 0;
}

void CClientListCtrl::OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	if (theApp.emuledlg->IsRunning()){
		// Although we have an owner drawn listview control we store the text for the primary item in the listview, to be
		// capable of quick searching those items via the keyboard. Because our listview items may change their contents,
		// we do this via a text callback function. The listview control will send us the LVN_DISPINFO notification if
		// it needs to know the contents of the primary item.
		//
		// But, the listview control sends this notification all the time, even if we do not search for an item. At least
		// this notification is only sent for the visible items and not for all items in the list. Though, because this
		// function is invoked *very* often, no *NOT* put any time consuming code here in.

		if (pDispInfo->item.mask & LVIF_TEXT){
			const CUpDownClient* pClient = reinterpret_cast<CUpDownClient*>(pDispInfo->item.lParam);
			if (pClient != NULL){
				switch (pDispInfo->item.iSubItem){
					case 0:
						if (pClient->GetUserName() != NULL && pDispInfo->item.cchTextMax > 0){
							_tcsncpy(pDispInfo->item.pszText, pClient->GetUserName(), pDispInfo->item.cchTextMax);
							pDispInfo->item.pszText[pDispInfo->item.cchTextMax-1] = _T('\0');
						}
						break;
					default:
						// shouldn't happen
						pDispInfo->item.pszText[0] = _T('\0');
						break;
				}
			}
		}
	}
	*pResult = 0;
}
