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
#include "DownloadListCtrl.h"
#include "otherfunctions.h" 
#include "updownclient.h"
#include "MenuCmds.h"
#include "ClientDetailDialog.h"
#include "FileDetailDialog.h"
#include "commentdialoglst.h"
#include "MetaDataDlg.h"
#include "InputBox.h"
#include "KademliaWnd.h"
#include "emuledlg.h"
#include "DownloadQueue.h"
#include "FriendList.h"
#include "PartFile.h"
#include "ClientCredits.h"
#include "MemDC.h"
#include "ChatWnd.h"
#include "TransferWnd.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "Kademlia/Kademlia/Prefs.h"
#include "Kademlia/net/KademliaUDPListener.h"
#include "WebServices.h"
#include "Preview.h"
#include "SharedFileList.h" //MORPH - Added by SiRoB
#include "version.h" //MORPH - Added by SiRoB
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CDownloadListCtrl

#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)
#define DLC_BARUPDATE 512


IMPLEMENT_DYNAMIC(CDownloadListCtrl, CListBox)
CDownloadListCtrl::CDownloadListCtrl() {
}

CDownloadListCtrl::~CDownloadListCtrl(){
	if (m_PrioMenu) VERIFY( m_PrioMenu.DestroyMenu() );
	if (m_A4AFMenu) VERIFY( m_A4AFMenu.DestroyMenu() );
	//MORPH START - Added by AndCycle, showSharePermissions
	if (m_PermMenu) VERIFY( m_PermMenu.DestroyMenu() );	// xMule_MOD: showSharePermissions
	//MORPH END   - Added by AndCycle, showSharePermissions
	//MORPH START - Added by SiRoB, Advanced A4AF derivated from Khaos
	if (m_A4AFMenuFlag) VERIFY( m_A4AFMenuFlag.DestroyMenu() );
	//MORPH END   - Added by SiRoB, Advanced A4AF derivated from Khaos
	if (m_FileMenu) VERIFY( m_FileMenu.DestroyMenu() );
	while(m_ListItems.empty() == false){
		delete m_ListItems.begin()->second; // second = CtrlItem_Struct*
		m_ListItems.erase(m_ListItems.begin());
	}
}

void CDownloadListCtrl::Init()
{
	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy, theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	SetStyle();
	ModifyStyle(LVS_SINGLESEL,0);
	
	CToolTipCtrl* tooltip = GetToolTips();
	if (tooltip){
		tooltip->ModifyStyle(0, TTS_NOPREFIX);
		tooltip->SetDelayTime(TTDT_AUTOPOP, 20000);
		tooltip->SetDelayTime(TTDT_INITIAL, thePrefs.GetToolTipDelay()*1000);
	}

	InsertColumn(0,GetResString(IDS_DL_FILENAME),LVCFMT_LEFT, 260);
	InsertColumn(1,GetResString(IDS_DL_SIZE),LVCFMT_LEFT, 60);
	InsertColumn(2,GetResString(IDS_DL_TRANSF),LVCFMT_LEFT, 65);
	InsertColumn(3,GetResString(IDS_DL_TRANSFCOMPL),LVCFMT_LEFT, 65);
	InsertColumn(4,GetResString(IDS_DL_SPEED),LVCFMT_LEFT, 65);
	InsertColumn(5,GetResString(IDS_DL_PROGRESS),LVCFMT_LEFT, 170);
	InsertColumn(6,GetResString(IDS_DL_SOURCES),LVCFMT_LEFT, 50);
	InsertColumn(7,GetResString(IDS_PRIORITY),LVCFMT_LEFT, 55);
	InsertColumn(8,GetResString(IDS_STATUS),LVCFMT_LEFT, 70);
	// khaos::accuratetimerem+
	/*
	InsertColumn(9,GetResString(IDS_DL_REMAINS),LVCFMT_LEFT, 110);
	*/
	InsertColumn(9,GetResString(IDS_DL_REMAINS),LVCFMT_LEFT, 100);
	// khaos::accuratetimerem-

	CString lsctitle=GetResString(IDS_LASTSEENCOMPL);
	lsctitle.Remove(':');
	InsertColumn(10, lsctitle,LVCFMT_LEFT, 220);
	lsctitle=GetResString(IDS_FD_LASTCHANGE);
	lsctitle.Remove(':');
	InsertColumn(11, lsctitle,LVCFMT_LEFT, 220);
	// khaos::categorymod+ Two new ResStrings, too.
	InsertColumn(12, GetResString(IDS_CAT_COLCATEGORY),LVCFMT_LEFT,60);
	InsertColumn(13, GetResString(IDS_CAT_COLORDER),LVCFMT_LEFT,60);
	// khaos::categorymod-
	// khaos::accuratetimerem+
	InsertColumn(14, GetResString(IDS_REMAININGSIZE), LVCFMT_LEFT, 80);
	// khaos::accuratetimerem-

	SetAllIcons();
	Localize();
	LoadSettings(CPreferences::tableDownload);
	curTab=0;

	// Barry - Use preferred sort order from preferences
	//MORPH START - Changed by SiRoB, Remain time and size Columns have been splited
	/*
	m_bRemainSort=thePrefs.TransferlistRemainSortStyle();

	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableDownload);
	bool sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableDownload);

	uint8 adder=0;
	if (sortItem!=9 || !m_bRemainSort)
		SetSortArrow(sortItem, sortAscending);
	else {
        SetSortArrow(sortItem, sortAscending?arrowDoubleUp : arrowDoubleDown);
		adder=81;
	}
	SortItems(SortProc, sortItem + (sortAscending ? 0:100) + adder);
	*/
	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableDownload);
	bool sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableDownload);
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, 0x8000);	// SLUGFILLER: DLsortFix - uses multi-sort for fall-back
	// SLUGFILLER: multiSort - load multiple params
	for (int i = thePrefs.GetColumnSortCount(CPreferences::tableDownload); i > 0; ) {
		i--;
		sortItem = thePrefs.GetColumnSortItem(CPreferences::tableDownload, i);
		sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableDownload, i);
		SortItems(SortProc, sortItem + (sortAscending ? 0:100));	// SLUGFILLER: DLsortFix
	}
	// SLUGFILLER: multiSort
	//MORPH END - Changed by SiRoB, Remain time and size Columns have been splited
}

void CDownloadListCtrl::OnSysColorChange()
{
	CMuleListCtrl::OnSysColorChange();
	SetAllIcons();
}

void CDownloadListCtrl::SetAllIcons()
{
	m_ImageList.DeleteImageList();
	m_ImageList.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	m_ImageList.SetBkColor(CLR_NONE);
	m_ImageList.Add(CTempIconLoader(_T("SrcDownloading")));
	m_ImageList.Add(CTempIconLoader(_T("SrcOnQueue")));
	m_ImageList.Add(CTempIconLoader(_T("SrcConnecting")));
	m_ImageList.Add(CTempIconLoader(_T("SrcNNPQF")));
	m_ImageList.Add(CTempIconLoader(_T("SrcUnknown")));
	m_ImageList.Add(CTempIconLoader(_T("ClientCompatible")));
	m_ImageList.Add(CTempIconLoader(_T("Friend")));
	m_ImageList.Add(CTempIconLoader(_T("ClientEDonkey")));
	m_ImageList.Add(CTempIconLoader(_T("ClientMLDonkey")));
	m_ImageList.Add(CTempIconLoader(_T("RatingReceived")));
	m_ImageList.Add(CTempIconLoader(_T("BadRatingReceived")));
	m_ImageList.Add(CTempIconLoader(_T("ClientEDonkeyHybrid")));
	m_ImageList.Add(CTempIconLoader(_T("ClientShareaza")));
	m_ImageList.Add(CTempIconLoader(_T("Server")));
	m_ImageList.Add(CTempIconLoader(_T("ClientAMule")));
	m_ImageList.Add(CTempIconLoader(_T("ClientLPhant")));
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("ClientSecureOvl"))), 1);
	//MORPH START - Added by SiRoB, More client & Credit Overlay Icon
	m_ImageList.Add(CTempIconLoader("ClientRightEdonkey")); //17
	m_ImageList.Add(CTempIconLoader("ClientMorph")); //18
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader("ClientCreditOvl")), 2); //19
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader("ClientCreditSecureOvl")), 3); //20
	//MORPH END   - Added by SiRoB, More client & Credit Overlay Icon
	//MORPH START - Added by IceCream, eMule Plus rating icones
	m_ImageList.Add(CTempIconLoader("RATING_NO"));  // 21
	m_ImageList.Add(CTempIconLoader("RATING_EXCELLENT"));  // 22
	m_ImageList.Add(CTempIconLoader("RATING_FAIR"));  // 23
	m_ImageList.Add(CTempIconLoader("RATING_GOOD"));  // 24
	m_ImageList.Add(CTempIconLoader("RATING_POOR"));  // 25
	m_ImageList.Add(CTempIconLoader("RATING_FAKE"));  // 26
	//MORPH END   - Added by IceCream, eMule Plus rating icones

	// Mighty Knife: Community icon
	m_overlayimages.DeleteImageList ();
	m_overlayimages.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	m_overlayimages.SetBkColor(CLR_NONE);
	m_overlayimages.Add(CTempIconLoader("Community"));
	// [end] Mighty Knife
	//MORPH START - Addded by SiRoB, Friend Addon
	m_overlayimages.Add(CTempIconLoader("ClientFriendOvl"));
	m_overlayimages.Add(CTempIconLoader("ClientFriendSlotOvl"));
	//MORPH END   - Addded by SiRoB, Friend Addon
	
}

void CDownloadListCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	strRes = GetResString(IDS_DL_FILENAME);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(0, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_DL_SIZE);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(1, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_DL_TRANSF);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(2, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_DL_TRANSFCOMPL);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(3, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_DL_SPEED);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(4, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_DL_PROGRESS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(5, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_DL_SOURCES);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(6, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_PRIORITY);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(7, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_STATUS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(8, &hdi);
	strRes.ReleaseBuffer();

	// khaos::accuratetimerem+
	strRes = GetResString(IDS_DL_REMAINS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(9, &hdi);
	strRes.ReleaseBuffer();
	// khaos::accuratetimerem-

	strRes = GetResString(IDS_LASTSEENCOMPL);
	strRes.Remove(':');
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(10, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_FD_LASTCHANGE);
	strRes.Remove(':');
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(11, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_CAT);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(12, &hdi);
	strRes.ReleaseBuffer();

	// khaos::categorymod+
	strRes = GetResString(IDS_CAT_COLCATEGORY);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(12, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_CAT_COLORDER);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(13, &hdi);
	strRes.ReleaseBuffer();
	// khaos::categorymod-

	// khaos::accuratetimerem+
	strRes = GetResString(IDS_REMAININGSIZE);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(14, &hdi);
	strRes.ReleaseBuffer();
	// khaos::accuratetimerem-

	CreateMenues();

	ShowFilesCount();
}

void CDownloadListCtrl::AddFile(CPartFile* toadd)
{
	// Create new Item
    CtrlItem_Struct* newitem = new CtrlItem_Struct;
    uint16 itemnr = GetItemCount();
    newitem->owner = NULL;
    newitem->type = FILE_TYPE;
    newitem->value = toadd;
    newitem->parent = NULL;
	newitem->dwUpdated = 0; 

	// The same file shall be added only once
	ASSERT(m_ListItems.find(toadd) == m_ListItems.end());
	m_ListItems.insert(ListItemsPair(toadd, newitem));

	if (toadd->CheckShowItemInGivenCat(curTab))
		InsertItem(LVIF_PARAM|LVIF_TEXT,itemnr,LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)newitem);

	ShowFilesCount();
}

void CDownloadListCtrl::AddSource(CPartFile* owner, CUpDownClient* source, bool notavailable)
{
	// Create new Item
    CtrlItem_Struct* newitem = new CtrlItem_Struct;
    newitem->owner = owner;
    newitem->type = (notavailable) ? UNAVAILABLE_SOURCE : AVAILABLE_SOURCE;
    newitem->value = source;
	newitem->dwUpdated = 0; 

	// Update cross link to the owner
	ListItems::const_iterator ownerIt = m_ListItems.find(owner);
	ASSERT(ownerIt != m_ListItems.end());
	CtrlItem_Struct* ownerItem = ownerIt->second;
	ASSERT(ownerItem->value == owner);
	newitem->parent = ownerItem;

	// The same source could be added a few time but only one time per file 
	{
		// Update the other instances of this source
		bool bFound = false;
		std::pair<ListItems::const_iterator, ListItems::const_iterator> rangeIt = m_ListItems.equal_range(source);
		for(ListItems::const_iterator it = rangeIt.first; it != rangeIt.second; it++){
			CtrlItem_Struct* cur_item = it->second;

			// Check if this source has been already added to this file => to be sure
			if(cur_item->owner == owner){
				// Update this instance with its new setting
				cur_item->type = newitem->type;
				cur_item->dwUpdated = 0;
				bFound = true;
			}
			else if(notavailable == false){
				// The state 'Available' is exclusive
				cur_item->type = UNAVAILABLE_SOURCE;
				cur_item->dwUpdated = 0;
			}
		}

		if(bFound == true){
			delete newitem; 
			return;
		}
	}
	m_ListItems.insert(ListItemsPair(source, newitem));

	if (owner->srcarevisible) {
		// find parent from the CListCtrl to add source
		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)ownerItem;
		sint16 result = FindItem(&find);
		if(result != (-1))
			InsertItem(LVIF_PARAM|LVIF_TEXT,result+1,LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)newitem);
	}
}

void CDownloadListCtrl::RemoveSource(CUpDownClient* source, CPartFile* owner)
{
	if (!theApp.emuledlg->IsRunning())
		return;

	// Retrieve all entries matching the source
	std::pair<ListItems::iterator, ListItems::iterator> rangeIt = m_ListItems.equal_range(source);
	for(ListItems::iterator it = rangeIt.first; it != rangeIt.second; ){
		CtrlItem_Struct* delItem  = it->second;
		if(owner == NULL || owner == delItem->owner){
			// Remove it from the m_ListItems			
			it = m_ListItems.erase(it);

			// Remove it from the CListCtrl
 			LVFINDINFO find;
			find.flags = LVFI_PARAM;
			find.lParam = (LPARAM)delItem;
			sint16 result = FindItem(&find);
			if(result != (-1)){
				DeleteItem(result);
			}

			// finally it could be delete
			delete delItem;
		}
		else{
			it++;
		}
	}
}

bool CDownloadListCtrl::RemoveFile(const CPartFile* toremove)
{
	bool bResult = false;
	if (!theApp.emuledlg->IsRunning())
		return bResult;
	// Retrieve all entries matching the File or linked to the file
	// Remark: The 'asked another files' clients must be removed from here
	ASSERT(toremove != NULL);
	for(ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); ){
		CtrlItem_Struct* delItem = it->second;
		if(delItem->owner == toremove || delItem->value == (void*)toremove){
			// Remove it from the m_ListItems
			it = m_ListItems.erase(it);

			// Remove it from the CListCtrl
			LVFINDINFO find;
			find.flags = LVFI_PARAM;
			find.lParam = (LPARAM)delItem;
			sint16 result = FindItem(&find);
			if(result != (-1)){
				DeleteItem(result);
			}

			// finally it could be delete
			delete delItem;
			bResult = true;
		}
		else {
			it++;
		}
	}
	ShowFilesCount();
	return bResult;
}

void CDownloadListCtrl::UpdateItem(void* toupdate)
{
	if (!theApp.emuledlg->IsRunning())
		return;

	// Retrieve all entries matching the source
	std::pair<ListItems::const_iterator, ListItems::const_iterator> rangeIt = m_ListItems.equal_range(toupdate);
	for(ListItems::const_iterator it = rangeIt.first; it != rangeIt.second; it++){
		CtrlItem_Struct* updateItem  = it->second;

		// Find entry in CListCtrl and update object
 		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)updateItem;
		sint16 result = FindItem(&find);
	if (result != -1){
			updateItem->dwUpdated = 0;
			Update(result);
		}
	}
}

void CDownloadListCtrl::DrawFileItem(CDC *dc, int nColumn, LPRECT lpRect, CtrlItem_Struct *lpCtrlItem)
{
	//MORPH START - Added by SiRoB, Don't draw hidden Rect
	CRect Rect;
	this->GetClientRect(Rect);
	Rect.IntersectRect(Rect,lpRect);
	if (Rect.IsRectEmpty())
		return;
	//MORPH END   - Added by SiRoB, Don't draw hidden Rect

	if(lpRect->left < lpRect->right)
	{
		CString buffer;
		CPartFile *lpPartFile = (CPartFile*)lpCtrlItem->value;

		//MORPH START - Added by SiRoB, Due to Don't draw hidden Rect
		if (thePrefs.GetCatColor(lpPartFile->GetCategory()) > 0)
			dc->SetTextColor(thePrefs.GetCatColor(lpPartFile->GetCategory()));
		//MORPH END   - Added by SiRoB, Due to Don't draw hidden Rect
		//MORPH START - Added by IceCream, show download in red
		if(thePrefs.GetEnableDownloadInRed() && lpPartFile->GetTransferingSrcCount())
			dc->SetTextColor(RGB(192,0,0));
		//MORPH END   - Added by IceCream, show download in red
		//MORPH START - Added by SiRoB, show download in Bold
		CFont newInBoldFont;
		CFont *pOldInBoldFont;
		if(thePrefs.GetEnableDownloadInBold() && lpPartFile->GetTransferingSrcCount()){
			LOGFONT logInBoldFont;
			CFont *pInBoldFont = GetFont();
			pInBoldFont->GetLogFont(&logInBoldFont);
			logInBoldFont.lfWeight = FW_BOLD;
			newInBoldFont.CreateFontIndirect(&logInBoldFont);
			pOldInBoldFont = dc->SelectObject(&newInBoldFont);
		}
		//MORPH END  - Added by SiRoB, show download in Bold

		switch(nColumn)
		{
		case 0:{		// file name
			if (thePrefs.GetCatColor(lpPartFile->GetCategory()) > 0)
				dc->SetTextColor(thePrefs.GetCatColor(lpPartFile->GetCategory()));
		
			//MORPH START - Added by IceCream, eMule Plus rating icons
			int iImage = theApp.GetFileTypeSystemImageIdx(lpPartFile->GetFileName());
			if (theApp.GetSystemImageList() != NULL)
				ImageList_Draw(theApp.GetSystemImageList(), iImage, dc->GetSafeHdc(), lpRect->left, lpRect->top, /*ILD_NORMAL*/ ILD_TRANSPARENT ); //Modified by IceCream, icons look better
			lpRect->left += theApp.GetSmallSytemIconSize().cx + 3;
			if ( thePrefs.ShowRatingIndicator() && ( lpPartFile->HasComment() || lpPartFile->HasRating() )) //Modified by IceCream, eMule plus rating icon
			{
				POINT point= {lpRect->left-4,lpRect->top+2};
				int image=16;
				switch(lpPartFile->GetRating())
				{
					case 0: 
						image=21; 
						break; 
					case 1: 
						image=26;
						break; 
					case 2: 
						image=25; 
						break; 
					case 3: 
						image=24; 
						break; 
					case 4: 
						image=23; 
						break; 
					case 5: 
						image=22; 
						break;
					default:
						image=21;
				}
				m_ImageList.Draw(dc, image, point, ILD_NORMAL);
			}
			lpRect->left += 9;
			dc->DrawText(lpPartFile->GetFileName(), (int)strlen(lpPartFile->GetFileName()),lpRect, DLC_DT_TEXT);
			lpRect->left -= (9 + theApp.GetSmallSytemIconSize().cx + 3);
			}
			break;
			//MORPH END - Added by IceCream, eMule Plus rating icons
		case 1:		// size
			buffer = CastItoXBytes(lpPartFile->GetFileSize());
			dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT | DT_RIGHT);
			break;

		case 2:		// transfered
			buffer = CastItoXBytes(lpPartFile->GetTransfered());
			dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT | DT_RIGHT);
			break;

		case 3:		// transfered complete
			buffer = CastItoXBytes(lpPartFile->GetCompletedSize());
			dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT | DT_RIGHT);
			break;
		case 4:		// speed
			if (lpPartFile->GetTransferingSrcCount())
				buffer.Format(_T("%.1f %s"), lpPartFile->GetDatarate() / 1024.0f,GetResString(IDS_KBYTESEC));
			dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT | DT_RIGHT);
			break;

		case 5:		// progress
			{
				lpRect->bottom --;
				lpRect->top ++;
				// added
				int  iWidth = lpRect->right - lpRect->left; 
				int iHeight = lpRect->bottom - lpRect->top;
				if (lpCtrlItem->status == (HBITMAP)NULL)
					VERIFY(lpCtrlItem->status.CreateBitmap(1, 1, 1, 8, NULL)); 
				CDC cdcStatus;
				HGDIOBJ hOldBitmap;
				cdcStatus.CreateCompatibleDC(dc);
				int cx = lpCtrlItem->status.GetBitmapDimension().cx; 
				DWORD dwTicks = GetTickCount();
				if(lpCtrlItem->dwUpdated + DLC_BARUPDATE < dwTicks || cx !=  iWidth || !lpCtrlItem->dwUpdated) {
					lpCtrlItem->status.DeleteObject(); 
					lpCtrlItem->status.CreateCompatibleBitmap(dc,  iWidth, iHeight); 
					lpCtrlItem->status.SetBitmapDimension(iWidth,  iHeight); 
					hOldBitmap = cdcStatus.SelectObject(lpCtrlItem->status); 

					RECT rec_status; 
					rec_status.left = 0; 
					rec_status.top = 0; 
					rec_status.bottom = iHeight; 
					rec_status.right = iWidth; 
					lpPartFile->DrawStatusBar(&cdcStatus,  &rec_status, thePrefs.UseFlatBar()); 

					lpCtrlItem->dwUpdated = dwTicks + (rand() % 128); 
				} else 
					hOldBitmap = cdcStatus.SelectObject(lpCtrlItem->status); 

				dc->BitBlt(lpRect->left, lpRect->top, iWidth, iHeight,  &cdcStatus, 0, 0, SRCCOPY); 
				cdcStatus.SelectObject(hOldBitmap);
				//added end

				if (thePrefs.GetUseDwlPercentage()) {
					// HoaX_69: BEGIN Display percent in progress bar
					COLORREF oldclr = dc->SetTextColor(RGB(255,255,255));
					int iOMode = dc->SetBkMode(TRANSPARENT);
					buffer.Format(_T("%.1f%%"), lpPartFile->GetPercentCompleted());
					   
					//MORPH START - Added by SiRoB, Bold percentage
					CFont newFont;
					CFont *pOldFont;
					LOGFONT logFont;
					CFont *pFont = GetFont();
					pFont->GetLogFont(&logFont);
					logFont.lfWeight = FW_BOLD;
					newFont.CreateFontIndirect(&logFont);
					pOldFont = dc->SelectObject(&newFont);
					//MORPH END   - Added by SiRoB, Bold percentage
					dc->DrawText(buffer, buffer.GetLength(), lpRect, (DLC_DT_TEXT & ~DT_RIGHT) | DT_CENTER);
					//MORPH START - Added by SiRoB, Bold percentage
					dc->SelectObject (pOldFont);
					newFont.DeleteObject ();
					//MORPH END   - Added by SiRoB, Bold percentage					   
					dc->SetBkMode(iOMode);
					dc->SetTextColor(oldclr);
					// HoaX_69: END
				}

				lpRect->bottom ++;
				lpRect->top --;
			}
			break;

		case 6:		// sources
			{
				uint16 sc = lpPartFile->GetSourceCount();
				//MORPH START - Modified by IceCream, [sivka: -counter for A4AF in sources column-]
				buffer.Format("%i/%i/%i (%i)",
					lpPartFile->GetSrcA4AFCount(), //MORPH - Added by SiRoB, A4AF counter
					(lpPartFile->GetSrcStatisticsValue(DS_ONQUEUE) + lpPartFile->GetSrcStatisticsValue(DS_DOWNLOADING)), //MORPH - Modified by SiRoB
					sc, lpPartFile->GetTransferingSrcCount());
				//MORPH END   - Modified by IceCream, [sivka: -counter for A4AF in sources column-]
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT | DT_RIGHT);
			}
			break;

		case 7:		// prio
			switch(lpPartFile->GetDownPriority()) {
			case PR_LOW:
				if( lpPartFile->IsAutoDownPriority() )
					dc->DrawText(GetResString(IDS_PRIOAUTOLOW),GetResString(IDS_PRIOAUTOLOW).GetLength(),lpRect, DLC_DT_TEXT);
				else
					dc->DrawText(GetResString(IDS_PRIOLOW),GetResString(IDS_PRIOLOW).GetLength(),lpRect, DLC_DT_TEXT);
				break;
			case PR_NORMAL:
				if( lpPartFile->IsAutoDownPriority() )
					dc->DrawText(GetResString(IDS_PRIOAUTONORMAL),GetResString(IDS_PRIOAUTONORMAL).GetLength(),lpRect, DLC_DT_TEXT);
				else
					dc->DrawText(GetResString(IDS_PRIONORMAL),GetResString(IDS_PRIONORMAL).GetLength(),lpRect, DLC_DT_TEXT);
				break;
			case PR_HIGH:
				if( lpPartFile->IsAutoDownPriority() )
					dc->DrawText(GetResString(IDS_PRIOAUTOHIGH),GetResString(IDS_PRIOAUTOHIGH).GetLength(),lpRect, DLC_DT_TEXT);
				else
					dc->DrawText(GetResString(IDS_PRIOHIGH),GetResString(IDS_PRIOHIGH).GetLength(),lpRect, DLC_DT_TEXT);
				break;
			}
			break;

		case 8:		// <<--9/21/02
			buffer = lpPartFile->getPartfileStatus();
			dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			break;

		// khaos::accuratetimerem+
		case 9:		// remaining time NOT size
			{
				if (lpPartFile->GetStatus() != PS_COMPLETING && lpPartFile->GetStatus() != PS_COMPLETE)
				{
					switch (thePrefs.GetTimeRemainingMode()) {
					case 0:
						{
							sint32 curTime = lpPartFile->getTimeRemaining();
							sint32 avgTime = lpPartFile->GetTimeRemainingAvg();
							buffer.Format("%s (%s)", CastSecondsToHM(curTime), CastSecondsToHM(avgTime));
							break;
						}
					case 1:
						{
							sint32 curTime = lpPartFile->getTimeRemaining();
							buffer.Format("%s", CastSecondsToHM(curTime));
							break;
						}
					case 2:
						{
							sint32 avgTime = lpPartFile->GetTimeRemainingAvg();
							buffer.Format("%s", CastSecondsToHM(avgTime));
							break;
						}
					}
				}
				dc->DrawText(buffer, buffer.GetLength(), lpRect, DLC_DT_TEXT);
				break;
			}
		// khaos::accuratetimerem-
		case 10: // last seen complete
			{
				CString tempbuffer;
				if (lpPartFile->m_nCompleteSourcesCountLo == 0)
				{
					tempbuffer.Format(_T("< %u"), lpPartFile->m_nCompleteSourcesCountHi);
				}
				else if (lpPartFile->m_nCompleteSourcesCountLo == lpPartFile->m_nCompleteSourcesCountHi)
				{
					tempbuffer.Format(_T("%u"), lpPartFile->m_nCompleteSourcesCountLo);
				}
				else
				{
					tempbuffer.Format(_T("%u - %u"), lpPartFile->m_nCompleteSourcesCountLo, lpPartFile->m_nCompleteSourcesCountHi);
				}
				if (lpPartFile->lastseencomplete==NULL)
					buffer.Format(_T("%s (%s)"),GetResString(IDS_UNKNOWN),tempbuffer);
				else
					buffer.Format(_T("%s (%s)"),lpPartFile->lastseencomplete.Format( thePrefs.GetDateTimeFormat()),tempbuffer);
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			}
			break;
		case 11: // last receive
			if (!IsColumnHidden(11)) {
				if(lpPartFile->GetFileDate()!=NULL)
					buffer=lpPartFile->GetCFileDate().Format( thePrefs.GetDateTimeFormat());
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			}
			break;
		// khaos::categorymod+
		case 12: // Category
			{
				if (!thePrefs.ShowCatNameInDownList())
					buffer.Format("%u", lpPartFile->GetCategory());
				else
					buffer.Format("%s", thePrefs.GetCategory(lpPartFile->GetCategory())->title);
				dc->DrawText(buffer, (int) strlen(buffer), lpRect, DLC_DT_TEXT);
				break;
			}
		case 13: // Resume Mod
			{
				buffer.Format("%u", lpPartFile->GetCatResumeOrder());
				dc->DrawText(buffer, (int) strlen(buffer), lpRect, DLC_DT_TEXT);
				break;
			}
		// khaos::categorymod-
		// khaos::accuratetimerem+
		case 14:		// remaining size
			{
				if (lpPartFile->GetStatus()!=PS_COMPLETING && lpPartFile->GetStatus()!=PS_COMPLETE ){
					//size 
					uint32 remains;
					remains=lpPartFile->GetFileSize()-lpPartFile->GetCompletedSize();	//<<-- 09/27/2002, CML

					buffer.Format("%s", CastItoXBytes(remains));

				}
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			}
			break;
		// khaos::accuratetimerem-
		}
		//MORPH START - Added by SiRoB, show download in Bold
		if(thePrefs.GetEnableDownloadInBold() && lpPartFile->GetTransferingSrcCount()){
			dc->SelectObject (pOldInBoldFont);
			newInBoldFont.DeleteObject ();
		}
		//MORPH END   - Added by SiRoB, show download in Bold
	}
}

void CDownloadListCtrl::DrawSourceItem(CDC *dc, int nColumn, LPRECT lpRect, CtrlItem_Struct *lpCtrlItem) {
	//MORPH START - Added by SiRoB, Don't draw hidden colums
	CRect Rect;
	this->GetClientRect(Rect);
	Rect.IntersectRect(Rect,lpRect);
	if (Rect.IsRectEmpty())
		return;
	//MORPH END   - Added by SiRoB, Don't draw hidden colums
	if(lpRect->left < lpRect->right) { 

		CString buffer;
		CUpDownClient *lpUpDownClient = (CUpDownClient*)lpCtrlItem->value;
		switch(nColumn) {

		case 0:		// icon, name, status
			{
				RECT cur_rec = *lpRect;
				POINT point = {cur_rec.left, cur_rec.top+1};
				if (lpCtrlItem->type == AVAILABLE_SOURCE){
					switch (lpUpDownClient->GetDownloadState()) {
					case DS_CONNECTING:
						m_ImageList.Draw(dc, 2, point, ILD_NORMAL);
						break;
					case DS_CONNECTED:
						m_ImageList.Draw(dc, 2, point, ILD_NORMAL);
						break;
					case DS_WAITCALLBACK:
						m_ImageList.Draw(dc, 2, point, ILD_NORMAL);
						break;
					case DS_ONQUEUE:
						if(lpUpDownClient->IsRemoteQueueFull())
							m_ImageList.Draw(dc, 3, point, ILD_NORMAL);
						else
							m_ImageList.Draw(dc, 1, point, ILD_NORMAL);
						break;
					case DS_DOWNLOADING:
						m_ImageList.Draw(dc, 0, point, ILD_NORMAL);
						break;
					case DS_REQHASHSET:
						m_ImageList.Draw(dc, 0, point, ILD_NORMAL);
						break;
					case DS_NONEEDEDPARTS:
						m_ImageList.Draw(dc, 3, point, ILD_NORMAL);
						break;
					case DS_LOWTOLOWIP:
						m_ImageList.Draw(dc, 3, point, ILD_NORMAL);
						break;
					case DS_TOOMANYCONNS:
						m_ImageList.Draw(dc, 2, point, ILD_NORMAL);
						break;
					default:
						m_ImageList.Draw(dc, 4, point, ILD_NORMAL);
					}
				}
				else {

					m_ImageList.Draw(dc, 3, point, ILD_NORMAL);
				}
				cur_rec.left += 20;
				//MORPH START - Modified by SiRoB, More client & ownCredit overlay icon
				UINT uOvlImg = INDEXTOOVERLAYMASK(((lpUpDownClient->Credits() && lpUpDownClient->Credits()->GetCurrentIdentState(lpUpDownClient->GetIP()) == IS_IDENTIFIED) ? 1 : 0) | ((lpUpDownClient->Credits() && lpUpDownClient->Credits()->GetMyScoreRatio(lpUpDownClient->GetIP()) > 1) ? 2 : 0));
				POINT point2= {cur_rec.left,cur_rec.top+1};
				//MORPH - Removed by SiRoB, Friend Addon
				/*
				if (lpUpDownClient->IsFriend())
					m_ImageList.Draw(dc, 6, point2, ILD_NORMAL | uOvlImg);
				else*/ if ( lpUpDownClient->GetClientSoft() == SO_EDONKEYHYBRID)
					m_ImageList.Draw(dc, 11, point2, ILD_NORMAL | uOvlImg);
				else if( lpUpDownClient->GetClientSoft() == SO_MLDONKEY)
					m_ImageList.Draw(dc, 8, point2, ILD_NORMAL | uOvlImg);
				else if ( lpUpDownClient->GetClientSoft() == SO_SHAREAZA)
					m_ImageList.Draw(dc, 12, point2, ILD_NORMAL | uOvlImg);
				else if (lpUpDownClient->GetClientSoft() == SO_URL)
					m_ImageList.Draw(dc, 13, point2, ILD_NORMAL | uOvlImg);
				else if (lpUpDownClient->GetClientSoft() == SO_AMULE)
					m_ImageList.Draw(dc, 14, point2, ILD_NORMAL | uOvlImg);
				else if (lpUpDownClient->GetClientSoft() == SO_LPHANT)
					m_ImageList.Draw(dc, 15, point2, ILD_NORMAL | uOvlImg);
				else if (lpUpDownClient->ExtProtocolAvailable())
					m_ImageList.Draw(dc, (lpUpDownClient->IsMorph())?18:5, point2, ILD_NORMAL | uOvlImg);
				else if ( lpUpDownClient->GetClientSoft() == SO_EDONKEY)
						m_ImageList.Draw(dc, 17, point2, ILD_NORMAL | uOvlImg);
				else
					m_ImageList.Draw(dc, 7, point2, ILD_NORMAL | uOvlImg);

				// Mighty Knife: Community visualization
				if (lpUpDownClient->IsCommunity())
					m_overlayimages.Draw(dc,0, point2, ILD_TRANSPARENT);
				// [end] Mighty Knife
				//MORPH START - Added by SiRoB, Friend Addon
				if (lpUpDownClient->IsFriend())
					m_overlayimages.Draw(dc, lpUpDownClient->GetFriendSlot()?2:1,point2, ILD_TRANSPARENT);
				//MORPH END   - Added by SiRoB, Friend Addon
				cur_rec.left += 20;
				//MORPH END - Modified by SiRoB, More client & ownCredits overlay icon

				//Morph Start - added by AndCycle, IP to Country
				if(theApp.ip2country->ShowCountryFlag()){
					POINT point3= {cur_rec.left,cur_rec.top+1};
					theApp.ip2country->GetFlagImageList()->DrawIndirect(dc, lpUpDownClient->GetCountryFlagIndex(), point3, CSize(18,16), CPoint(0,0), ILD_NORMAL);
					cur_rec.left+=20;
				}
				//Morph End - added by AndCycle, IP to Country

				if (!lpUpDownClient->GetUserName())
					buffer = "?";
				else {
					buffer = lpUpDownClient->GetUserName();
					//MORPH START - Added by IceCream, [sivka: -A4AF counter, ahead of user nickname-]
					CString tempStr;
					tempStr.Format("(%i) %s",lpUpDownClient->m_OtherRequests_list.GetCount()+1+lpUpDownClient->m_OtherNoNeeded_list.GetCount(),buffer);
					buffer = tempStr;
					//MORPH END   - Added by IceCream, [sivka: -A4AF counter, ahead of user nickname-]
				}

				//EastShare Start - added by AndCycle, IP to Country, modified by Commander
				//CString tempStr2;
				//tempStr2.Format("%s%s", lpUpDownClient->GetCountryName(), buffer);
				//buffer = tempStr2;
				//EastShare End - added by AndCycle, IP to Country

				dc->DrawText(buffer,buffer.GetLength(),&cur_rec, DLC_DT_TEXT);
			}
			break;

		case 1:		// size
			switch(lpUpDownClient->GetSourceFrom()){
				case SF_SERVER:
					buffer = "ED2K Server";
					break;
				case SF_KADEMLIA:
					buffer = GetResString(IDS_KADEMLIA);
					break;
				case SF_SOURCE_EXCHANGE:
					buffer = GetResString(IDS_SE);
					break;
				case SF_PASSIVE:
					buffer = GetResString(IDS_PASSIVE);
					break;
				case SF_LINK:
					buffer = GetResString(IDS_SW_LINK);
					break;
				//MORPH START - Added by SiRoB, Source Loader Saver [SLS]
				case SF_SLS:
					buffer = "SLS";
					break;
				//MORPH END   - Added by SiRoB, Source Loader Saver [SLS]
				default:
					buffer = GetResString(IDS_ERROR);
			}
			dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			break;

		case 2:// transfered
			//MORPH START - Changed by SiRoB, Download/Upload
			/*
			if ( !IsColumnHidden(3)) {
				dc->DrawText(_T(""),0,lpRect, DLC_DT_TEXT);
				break;
			}
			*/
			if(lpUpDownClient->Credits() && (lpUpDownClient->Credits()->GetUploadedTotal() || lpUpDownClient->Credits()->GetDownloadedTotal())){
				buffer.Format( _T("%s/%s"),
				CastItoXBytes((float)lpUpDownClient->Credits()->GetDownloadedTotal()),
				CastItoXBytes((float)lpUpDownClient->Credits()->GetUploadedTotal()));
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			}
			break;
			//MORPH END  - Changed by SiRoB, Upload/Download
		case 3:// completed
			if (lpCtrlItem->type == AVAILABLE_SOURCE && lpUpDownClient->GetTransferedDown()) {
				buffer = CastItoXBytes(lpUpDownClient->GetTransferedDown());
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT | DT_RIGHT);
			}
			break;

		case 4:		// speed
			if (lpCtrlItem->type == AVAILABLE_SOURCE && lpUpDownClient->GetDownloadDatarate()){
				if (lpUpDownClient->GetDownloadDatarate())
					buffer.Format(_T("%.1f %s"), lpUpDownClient->GetDownloadDatarate()/1024.0f,GetResString(IDS_KBYTESEC));
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT | DT_RIGHT);
			}
			break;

		case 5:		// file info
			{
				lpRect->bottom--; 
				lpRect->top++; 

				int iWidth = lpRect->right - lpRect->left; 
				int iHeight = lpRect->bottom - lpRect->top; 
				if (lpCtrlItem->status == (HBITMAP)NULL)
					VERIFY(lpCtrlItem->status.CreateBitmap(1, 1, 1, 8, NULL)); 
				CDC cdcStatus;
				HGDIOBJ hOldBitmap;
				cdcStatus.CreateCompatibleDC(dc);
				int cx = lpCtrlItem->status.GetBitmapDimension().cx;
				DWORD dwTicks = GetTickCount();
				if(lpCtrlItem->dwUpdated + DLC_BARUPDATE < dwTicks || cx !=  iWidth  || !lpCtrlItem->dwUpdated) { 
					lpCtrlItem->status.DeleteObject(); 
					lpCtrlItem->status.CreateCompatibleBitmap(dc,  iWidth, iHeight); 
					lpCtrlItem->status.SetBitmapDimension(iWidth,  iHeight); 
					hOldBitmap = cdcStatus.SelectObject(lpCtrlItem->status); 

					RECT rec_status; 
					rec_status.left = 0; 
					rec_status.top = 0; 
					rec_status.bottom = iHeight; 
					rec_status.right = iWidth; 
					//MORPH START - Changed by SiRoB, Advanced A4AF derivated from Khaos
					//lpUpDownClient->DrawStatusBar(&cdcStatus,  &rec_status,(lpCtrlItem->type == UNAVAILABLE_SOURCE), thePrefs.UseFlatBar()); 
					lpUpDownClient->DrawStatusBar(&cdcStatus,  &rec_status,(CPartFile*)lpCtrlItem->owner, thePrefs.UseFlatBar());
					//MORPH END   - Changed by SiRoB, Advanced A4AF derivated from Khaos
					lpCtrlItem->dwUpdated = dwTicks + (rand() % 128); 
				} else 
					hOldBitmap = cdcStatus.SelectObject(lpCtrlItem->status); 

				dc->BitBlt(lpRect->left, lpRect->top, iWidth, iHeight,  &cdcStatus, 0, 0, SRCCOPY); 
				cdcStatus.SelectObject(hOldBitmap);

				lpRect->bottom++; 
				lpRect->top--; 
			}
			break;

		case 6:{		// sources
			buffer = lpUpDownClient->DbgGetFullClientSoftVer();
			if (buffer.IsEmpty())
				buffer = GetResString(IDS_UNKNOWN);
			dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			break;
		}

		case 7:		// prio
			if (lpUpDownClient->GetDownloadState()==DS_ONQUEUE){
				if( lpUpDownClient->IsRemoteQueueFull() ){
					buffer = GetResString(IDS_QUEUEFULL);
					dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
				}
				else{
					if ( lpUpDownClient->GetRemoteQueueRank()){
						//Morph - modified by AndCycle, DiffQR
						COLORREF crOldTxtColor;
						int	m_iDifference = lpUpDownClient->GetDiffQR();
						if(m_iDifference == 0){
							crOldTxtColor = dc->SetTextColor((COLORREF)RGB(0,0,0));
						}
						else if(m_iDifference > 0){
							crOldTxtColor = dc->SetTextColor((COLORREF)RGB(191,0,0));
						}
						else if(m_iDifference < 0){
							crOldTxtColor = dc->SetTextColor((COLORREF)RGB(0,191,0));
						}
						buffer.Format(_T("QR: %u (%+i)"),lpUpDownClient->GetRemoteQueueRank(), m_iDifference);
						dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
						dc->SetTextColor(crOldTxtColor);
						//Morph - modified by AndCycle, DiffQR
					}
					else{
						dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
					}
				}
			} else {
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			}

			break;

		case 8:	{	// status
			if (lpCtrlItem->type == AVAILABLE_SOURCE){
				buffer = lpUpDownClient->GetDownloadStateDisplayString();
			}
			else {
				buffer = GetResString(IDS_ASKED4ANOTHERFILE);

// ZZ:DownloadManager -->
                if(thePrefs.IsExtControlsEnabled()) {
                    if(!lpUpDownClient->IsInOtherRequestList(lpCtrlItem->owner)) {
                        buffer += _T(" (") + GetResString(IDS_NONEEDEDPARTS) + _T(")");
                    } else if(lpUpDownClient->GetDownloadState() == DS_DOWNLOADING) {
                        buffer += _T(" (") + GetResString(IDS_TRANSFERRING) + _T(")");
                    } else if(lpUpDownClient->IsSwapSuspended(lpCtrlItem->owner)) {
                        buffer += _T(" (") + GetResString(IDS_SOURCESWAPBLOCKED) + _T(")");
                    }
				
                    if (lpUpDownClient && lpUpDownClient->reqfile && lpUpDownClient->reqfile->GetFileName()){
                        buffer.AppendFormat(_T(": \"%s\""),lpUpDownClient->reqfile->GetFileName());
					}
                }
// <-- ZZ:DownloadManager
			}

// ZZ:DownloadManager -->
            if(thePrefs.IsExtControlsEnabled() && !lpUpDownClient->m_OtherRequests_list.IsEmpty()) {
                buffer.Append(_T("*"));
            }
// ZZ:DownloadManager <--

			dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			break;
				}
		case 9:		// remaining time & size
			break;
		}
	}
}

void CDownloadListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct){
	if (!theApp.emuledlg->IsRunning())
		return;
	if (!lpDrawItemStruct->itemData)
		return;

	CDC* odc = CDC::FromHandle(lpDrawItemStruct->hDC);
	CtrlItem_Struct* content = (CtrlItem_Struct*)lpDrawItemStruct->itemData;
	BOOL bCtrlFocused = ((GetFocus() == this) || (GetStyle() & LVS_SHOWSELALWAYS));

	if ((content->type == FILE_TYPE) && (lpDrawItemStruct->itemAction | ODA_SELECT) &&
	    (lpDrawItemStruct->itemState & ODS_SELECTED)) {

		if(bCtrlFocused)
			odc->SetBkColor(m_crHighlight);
		else
			odc->SetBkColor(m_crNoHighlight);
	}
	else
		odc->SetBkColor(GetBkColor());

	CMemDC dc(odc,&lpDrawItemStruct->rcItem);
	CFont *pOldFont = dc->SelectObject(GetFont());
	COLORREF crOldTextColor = dc->SetTextColor(m_crWindowText);

	int iOldBkMode;
	if (m_crWindowTextBk == CLR_NONE){
		DefWindowProc(WM_ERASEBKGND, (WPARAM)(HDC)dc, 0);
		iOldBkMode = dc.SetBkMode(TRANSPARENT);
	}
	else
		iOldBkMode = OPAQUE;

	BOOL notLast = lpDrawItemStruct->itemID + 1 != GetItemCount();
	BOOL notFirst = lpDrawItemStruct->itemID != 0;
	int tree_start=0;
	int tree_end=0;

	RECT cur_rec = lpDrawItemStruct->rcItem;

	//offset was 4, now it's the standard 2 spaces
	int iOffset = dc->GetTextExtent(_T(" "), 1 ).cx*2;
	CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	cur_rec.right = cur_rec.left;
	cur_rec.right -= iOffset;
	cur_rec.left += iOffset;

	if (content->type == FILE_TYPE){
		for(int iCurrent = 0; iCurrent < iCount; iCurrent++) {

			int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
			int cx = CListCtrl::GetColumnWidth(iColumn);
			if(iColumn == 5) {
				int iNextLeft = cur_rec.left + cx;
				//set up tree vars
				cur_rec.left = cur_rec.right + iOffset;
				cur_rec.right = cur_rec.left + min(8, cx);
				tree_start = cur_rec.left + 1;
				tree_end = cur_rec.right;
				//normal column stuff
				cur_rec.left = cur_rec.right + 1;
				cur_rec.right = tree_start + cx - iOffset;
				DrawFileItem(dc, 5, &cur_rec, content);
				cur_rec.left = iNextLeft;
			} else {
				cur_rec.right += cx;
				DrawFileItem(dc, iColumn, &cur_rec, content);
				cur_rec.left += cx;
			}

		}

	}
	else if (content->type == UNAVAILABLE_SOURCE || content->type == AVAILABLE_SOURCE){

		for(int iCurrent = 0; iCurrent < iCount; iCurrent++) {

			int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
			int cx = CListCtrl::GetColumnWidth(iColumn);

			if(iColumn == 5) {
				int iNextLeft = cur_rec.left + cx;
				//set up tree vars
				cur_rec.left = cur_rec.right + iOffset;
				cur_rec.right = cur_rec.left + min(8, cx);
				tree_start = cur_rec.left + 1;
				tree_end = cur_rec.right;
				//normal column stuff
				cur_rec.left = cur_rec.right + 1;
				cur_rec.right = tree_start + cx - iOffset;
				DrawSourceItem(dc, 5, &cur_rec, content);
				cur_rec.left = iNextLeft;
			} else {
				cur_rec.right += cx;
				DrawSourceItem(dc, iColumn, &cur_rec, content);
				cur_rec.left += cx;
			}
		}
	}

	//draw rectangle around selected item(s)
	if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
		(lpDrawItemStruct->itemState & ODS_SELECTED) &&
		(content->type == FILE_TYPE))
	{
		RECT outline_rec = lpDrawItemStruct->rcItem;

		outline_rec.top--;
		outline_rec.bottom++;
		dc->FrameRect(&outline_rec, &CBrush(GetBkColor()));
		outline_rec.top++;
		outline_rec.bottom--;
		outline_rec.left++;
		outline_rec.right--;

		if(notFirst && (GetItemState(lpDrawItemStruct->itemID - 1, LVIS_SELECTED))) {
			CtrlItem_Struct* prev = (CtrlItem_Struct*)this->GetItemData(lpDrawItemStruct->itemID - 1);
			if(prev->type == FILE_TYPE)
				outline_rec.top--;
		} 

		if(notLast && (GetItemState(lpDrawItemStruct->itemID + 1, LVIS_SELECTED))) {
			CtrlItem_Struct* next = (CtrlItem_Struct*)this->GetItemData(lpDrawItemStruct->itemID + 1);
			if(next->type == FILE_TYPE)
				outline_rec.bottom++;
		} 

		if(bCtrlFocused)
			dc->FrameRect(&outline_rec, &CBrush(m_crFocusLine));
		else
			dc->FrameRect(&outline_rec, &CBrush(m_crNoFocusLine));
	}
	//draw focus rectangle around non-highlightable items when they have the focus
	else if (((lpDrawItemStruct->itemState & ODS_FOCUS) == ODS_FOCUS) && (GetFocus() == this))
	{
		RECT focus_rec;
		focus_rec.top    = lpDrawItemStruct->rcItem.top;
		focus_rec.bottom = lpDrawItemStruct->rcItem.bottom;
		focus_rec.left   = lpDrawItemStruct->rcItem.left + 1;
		focus_rec.right  = lpDrawItemStruct->rcItem.right - 1;
		dc->FrameRect(&focus_rec, &CBrush(m_crNoFocusLine));
	}

	//draw tree last so it draws over selected and focus (looks better)
	if(tree_start < tree_end) {
		//set new bounds
		RECT tree_rect;
		tree_rect.top    = lpDrawItemStruct->rcItem.top;
		tree_rect.bottom = lpDrawItemStruct->rcItem.bottom;
		tree_rect.left   = tree_start;
		tree_rect.right  = tree_end;
		dc->SetBoundsRect(&tree_rect, DCB_DISABLE);

		//gather some information
		BOOL hasNext = notLast &&
			((CtrlItem_Struct*)this->GetItemData(lpDrawItemStruct->itemID + 1))->type != FILE_TYPE;
		BOOL isOpenRoot = hasNext && content->type == FILE_TYPE;
		BOOL isChild = content->type != FILE_TYPE;
		//BOOL isExpandable = !isChild && ((CPartFile*)content->value)->GetSourceCount() > 0;
		//might as well calculate these now
		int treeCenter = tree_start + 3;
		int middle = (cur_rec.top + cur_rec.bottom + 1) / 2;

		//set up a new pen for drawing the tree
		CPen pn, *oldpn;
		pn.CreatePen(PS_SOLID, 1, m_crWindowText);
		oldpn = dc->SelectObject(&pn);

		if(isChild) {
			//draw the line to the status bar
			dc->MoveTo(tree_end, middle);
			dc->LineTo(tree_start + 3, middle);

			//draw the line to the child node
			if(hasNext) {
				dc->MoveTo(treeCenter, middle);
				dc->LineTo(treeCenter, cur_rec.bottom + 1);
			}
		} else if(isOpenRoot) {
			//draw circle
			RECT circle_rec;
			COLORREF crBk = dc->GetBkColor();
			circle_rec.top    = middle - 2;
			circle_rec.bottom = middle + 3;
			circle_rec.left   = treeCenter - 2;
			circle_rec.right  = treeCenter + 3;
			dc->FrameRect(&circle_rec, &CBrush(m_crWindowText));
			dc->SetPixelV(circle_rec.left,      circle_rec.top,    crBk);
			dc->SetPixelV(circle_rec.right - 1, circle_rec.top,    crBk);
			dc->SetPixelV(circle_rec.left,      circle_rec.bottom - 1, crBk);
			dc->SetPixelV(circle_rec.right - 1, circle_rec.bottom - 1, crBk);
			//draw the line to the child node
			if(hasNext) {
				dc->MoveTo(treeCenter, middle + 3);
				dc->LineTo(treeCenter, cur_rec.bottom + 1);
			}
		} /*else if(isExpandable) {
			//draw a + sign
			dc->MoveTo(treeCenter, middle - 2);
			dc->LineTo(treeCenter, middle + 3);
			dc->MoveTo(treeCenter - 2, middle);
			dc->LineTo(treeCenter + 3, middle);
		}*/

		//draw the line back up to parent node
		if(notFirst && isChild) {
			dc->MoveTo(treeCenter, middle);
			dc->LineTo(treeCenter, cur_rec.top - 1);
		}

		//put the old pen back
		dc->SelectObject(oldpn);
		pn.DeleteObject();
	}

	//put the original objects back
	if (m_crWindowTextBk == CLR_NONE)
		dc.SetBkMode(iOldBkMode);
	dc->SelectObject(pOldFont);
	dc->SetTextColor(crOldTextColor);
}

// modifier-keys -view filtering [Ese Juani+xrmb]
void CDownloadListCtrl::HideSources(CPartFile* toCollapse, bool isShift, bool isCtrl, bool isAlt)
{
	SetRedraw(false);
	int pre,post;
	pre = post = 0;
	for(int i = 0; i < GetItemCount(); i++) {
		CtrlItem_Struct* item = (CtrlItem_Struct*)this->GetItemData(i);
		if(item->owner == toCollapse) {
			pre++;
			if(isShift || isCtrl || isAlt){
				EDownloadState ds=((CUpDownClient*)item->value)->GetDownloadState();
				if((isShift && ds==DS_DOWNLOADING) ||
					(isCtrl && ((CUpDownClient*)item->value)->GetRemoteQueueRank()> 0) ||
					(isAlt && ds!=DS_NONEEDEDPARTS)) continue;
			}
			item->dwUpdated = 0;
			item->status.DeleteObject();
			DeleteItem(i--);
			post++;
		}
	}
	if (pre-post==0) toCollapse->srcarevisible = false;
	SetRedraw(true);
}

BEGIN_MESSAGE_MAP(CDownloadListCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(LVN_ITEMACTIVATE, OnItemActivate)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnListModified)
	ON_NOTIFY_REFLECT(LVN_INSERTITEM, OnListModified)
	ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnListModified)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclkDownloadlist)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
	ON_WM_KEYDOWN()
END_MESSAGE_MAP()

void CDownloadListCtrl::ExpandCollapseItem(int item,uint8 expand,bool collapsesource){
	if (item==-1) return;

	CtrlItem_Struct* content = (CtrlItem_Struct*)this->GetItemData(item);

	// modifier-keys -view filtering [Ese Juani+xrmb]
	bool isShift=GetAsyncKeyState(VK_SHIFT) < 0;
	bool isCtrl=GetAsyncKeyState(VK_CONTROL) < 0;
	bool isAlt=GetAsyncKeyState(VK_MENU) < 0;

	if (collapsesource && content->parent!=NULL) {// to collapse/expand files when one of its source is selected
		content=content->parent;
		
 		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)content;
		item = FindItem(&find);
		if (item==-1) return;
	}

	if (!content || content->type != FILE_TYPE) return;
	
	CPartFile* partfile = reinterpret_cast<CPartFile*>(content->value);
	if (!partfile) return;

	if (partfile->GetStatus()==PS_COMPLETE) {
		TCHAR* buffer = new TCHAR[MAX_PATH];
		_stprintf(buffer,_T("%s"),partfile->GetFullName());
		ShellOpenFile(buffer, NULL);
		delete[] buffer;
		return;
	}

	// Check if the source branch is disable
	if(partfile->srcarevisible == false ) {
		if (expand>COLLAPSE_ONLY){
			SetRedraw(false);
			
			// Go throught the whole list to find out the sources for this file
			// Remark: don't use GetSourceCount() => UNAVAILABLE_SOURCE
			for(ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++){
				const CtrlItem_Struct* cur_item = it->second;
				if(cur_item->owner == partfile){
					if(isShift || isCtrl || isAlt) {
						ASSERT(cur_item->type != FILE_TYPE);
						EDownloadState ds=((CUpDownClient*)cur_item->value)->GetDownloadState();
						if(!(isShift && ds==DS_DOWNLOADING ||
							isCtrl && ((CUpDownClient*)cur_item->value)->GetRemoteQueueRank()>0 ||
							isAlt && ds!=DS_NONEEDEDPARTS))
							continue; // skip this source
					}
					partfile->srcarevisible = true;
					InsertItem(LVIF_PARAM|LVIF_TEXT,item+1,LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)cur_item);
				}
			}

			SetRedraw(true);
		}
	}
	else {
		if (expand==EXPAND_COLLAPSE || expand==COLLAPSE_ONLY) HideSources(partfile,isShift,isCtrl,isAlt);
	}
}

// CDownloadListCtrl message handlers
void CDownloadListCtrl::OnItemActivate(NMHDR *pNMHDR, LRESULT *pResult){
	LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (thePrefs.IsDoubleClickEnabled() || pNMIA->iSubItem > 0)
		ExpandCollapseItem(pNMIA->iItem,2);
	*pResult = 0;
}

void CDownloadListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1)
	{
		const CtrlItem_Struct* content = (CtrlItem_Struct*)GetItemData(iSel);
		if (content->type == FILE_TYPE)
		{
			// get merged settings
			bool bFirstItem = true;
			int iSelectedItems = 0;
			int iFilesNotDone = 0;
			int iFilesToPause = 0;
			int iFilesToStop = 0;
			int iFilesToResume = 0;
			int iFilesToOpen = 0;
			int iFilesToPreview = 0;
			int iFilesA4AFAuto = 0;

			int iFilePerm = 0; //MORPH START - Added by SiRoB, showSharePermissions
			int iFileForceA4AF = 0; //MORPH - Added by SiRoB, A4AF
			int iFileForceAllA4AF = 0; //MORPH - Added by SiRoB, A4AF
			int iFileForceA4AFOff = 0; //MORPH - Added by SiRoB, A4AF
			int iFileNotSeenCompleteSource = 0; //MORPH - Added by SiRoB, Only download complete files v2.1 (shadow)
			UINT uPrioMenuItem = 0;
			UINT uPermMenuItem = 0; //MORPH - Added by SiRoB, showSharePermissions
			const CPartFile* file1 = NULL;
			POSITION pos = GetFirstSelectedItemPosition();
			while (pos)
			{
				const CtrlItem_Struct* pItemData = (CtrlItem_Struct*)GetItemData(GetNextSelectedItem(pos));
				if (pItemData->type != FILE_TYPE)
					continue;
				const CPartFile* pFile = (CPartFile*)pItemData->value;
				if (bFirstItem)
					file1 = pFile;
				iSelectedItems++;

				bool bFileDone = (pFile->GetStatus()==PS_COMPLETE || pFile->GetStatus()==PS_COMPLETING);
				iFilesNotDone += !bFileDone ? 1 : 0;
				iFilesToStop += pFile->CanStopFile() ? 1 : 0;
				iFilesToPause += pFile->CanPauseFile() ? 1 : 0;
				iFilesToResume += pFile->CanResumeFile() ? 1 : 0;
				iFilesToOpen += pFile->CanOpenFile() ? 1 : 0;
				iFilesToPreview += pFile->CanPreviewFile() ? 1 : 0;
				iFilesA4AFAuto += (!bFileDone && pFile->IsA4AFAuto()) ? 1 : 0;
				//MORPH START - Added by SiRoB, Only download complete files v2.1 (shadow)
				iFileNotSeenCompleteSource += pFile->notSeenCompleteSource() && pFile->GetStatus() != PS_ERROR && !bFileDone;
				//MORPH END   - Added by SiRoB, Only download complete files v2.1 (shadow)
				//MORPH START - Added by SiRoB, A4AF
				iFileForceAllA4AF += (theApp.downloadqueue->forcea4af_file == pFile)?1:0;
				iFileForceA4AF += pFile->ForceAllA4AF()?1:0;
				iFileForceA4AFOff += pFile->ForceA4AFOff()?1:0;
				//MORPH END   - Added by SiRoB, A4AF
                UINT uCurPrioMenuItem = 0;
				if (pFile->IsAutoDownPriority())
					uCurPrioMenuItem = MP_PRIOAUTO;
				else if (pFile->GetDownPriority() == PR_HIGH)
					uCurPrioMenuItem = MP_PRIOHIGH;
				else if (pFile->GetDownPriority() == PR_NORMAL)
					uCurPrioMenuItem = MP_PRIONORMAL;
				else if (pFile->GetDownPriority() == PR_LOW)
					uCurPrioMenuItem = MP_PRIOLOW;
				else
					ASSERT(0);

				if (bFirstItem)
					uPrioMenuItem = uCurPrioMenuItem;
				else if (uPrioMenuItem != uCurPrioMenuItem)
					uPrioMenuItem = 0;

				//MORPH START - Added by SiRoB, showSharePermissions
				UINT uCurPermMenuItem = 0;
				if (pFile->GetPermissions()==-1)
					uCurPermMenuItem = MP_PERMDEFAULT;
				else if (pFile->GetPermissions()==PERM_ALL)
					uCurPermMenuItem = MP_PERMALL;
				else if (pFile->GetPermissions() == PERM_FRIENDS)
					uCurPermMenuItem = MP_PERMFRIENDS;
				else if (pFile->GetPermissions() == PERM_NOONE)
					uCurPermMenuItem = MP_PERMNONE;
				// Mighty Knife: Community visible filelist
				else if (pFile->GetPermissions() == PERM_COMMUNITY)
					uCurPermMenuItem = MP_PERMCOMMUNITY;
				// [end] Mighty Knife
				else
					ASSERT(0);

				if (bFirstItem)
					uPermMenuItem = uCurPermMenuItem;
				else if (uPermMenuItem != uCurPermMenuItem)
					uPermMenuItem = 0;
				//MORPH END   - Added by SiRoB, showSharePermissions
				bFirstItem = false;
			}

			m_FileMenu.EnableMenuItem((UINT_PTR)m_PrioMenu.m_hMenu, iFilesNotDone > 0 ? MF_ENABLED : MF_GRAYED);
			m_PrioMenu.CheckMenuRadioItem(MP_PRIOLOW, MP_PRIOAUTO, uPrioMenuItem, 0);

			//MORPH - Added by SiRoB, khaos::kmod+ Popup menu should be disabled when advanced A4AF mode is turned off and we need to check appropriate A4AF items.
			m_FileMenu.EnableMenuItem(MP_FORCEA4AF, thePrefs.UseSmartA4AFSwapping() && iSelectedItems == 1 && iFilesNotDone == 1? MF_ENABLED : MF_GRAYED);
			m_FileMenu.CheckMenuItem(MP_FORCEA4AF,  iFileForceAllA4AF > 0 && iSelectedItems == 1 ? MF_CHECKED : MF_UNCHECKED);
			
			m_FileMenu.EnableMenuItem((UINT_PTR)m_A4AFMenuFlag.m_hMenu, iFilesNotDone > 0 && (thePrefs.AdvancedA4AFMode() || thePrefs.UseSmartA4AFSwapping())? MF_ENABLED : MF_GRAYED);
			m_A4AFMenuFlag.ModifyMenu(MP_FORCEA4AFONFLAG, (iFileForceA4AF > 0 && iSelectedItems == 1 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, MP_FORCEA4AFONFLAG, ((GetSelectedCount() > 1) ? GetResString(IDS_INVERT) + " " : "") + GetResString(IDS_A4AF_ONFLAG));
			m_A4AFMenuFlag.ModifyMenu(MP_FORCEA4AFOFFFLAG, (iFileForceA4AFOff > 0 && iSelectedItems == 1 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, MP_FORCEA4AFOFFFLAG, ((GetSelectedCount() > 1) ? GetResString(IDS_INVERT) + " " : "") + GetResString(IDS_A4AF_OFFFLAG));
			// khaos::kmod-

			// enable commands if there is at least one item which can be used for the action
			m_FileMenu.EnableMenuItem(MP_CANCEL, iFilesNotDone > 0 ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_STOP, iFilesToStop > 0 ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_PAUSE, iFilesToPause > 0 ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_RESUME, iFilesToResume > 0 ? MF_ENABLED : MF_GRAYED);

			//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
			m_FileMenu.EnableMenuItem(MP_FORCE,iFileNotSeenCompleteSource > 0 ? MF_ENABLED : MF_GRAYED);
			//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)

			bool bOpenEnabled = (iSelectedItems == 1 && iFilesToOpen == 1);
			m_FileMenu.EnableMenuItem(MP_OPEN, bOpenEnabled ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_PREVIEW, (iSelectedItems == 1 && iFilesToPreview == 1) ? MF_ENABLED : MF_GRAYED);
			CMenu PreviewMenu;
			PreviewMenu.CreateMenu();
			int iPreviewMenuEntries = thePreviewApps.GetAllMenuEntries(PreviewMenu, (iSelectedItems == 1) ? file1 : NULL);
			if (iPreviewMenuEntries)
				m_FileMenu.InsertMenu(MP_METINFO, MF_POPUP | (iSelectedItems == 1 ? MF_ENABLED : MF_GRAYED), (UINT_PTR)PreviewMenu.m_hMenu, GetResString(IDS_DL_PREVIEW));

			bool bDetailsEnabled = (iSelectedItems > 0);
			m_FileMenu.EnableMenuItem(MP_METINFO, bDetailsEnabled ? MF_ENABLED : MF_GRAYED);
			if (thePrefs.IsDoubleClickEnabled() && bOpenEnabled)
				m_FileMenu.SetDefaultItem(MP_OPEN);
			else if (!thePrefs.IsDoubleClickEnabled() && bDetailsEnabled)
				m_FileMenu.SetDefaultItem(MP_METINFO);
			else
				m_FileMenu.SetDefaultItem((UINT)-1);
			m_FileMenu.EnableMenuItem(MP_VIEWFILECOMMENTS, (iSelectedItems == 1 && iFilesNotDone == 1) ? MF_ENABLED : MF_GRAYED);
			
			int total;
			m_FileMenu.EnableMenuItem(MP_CLEARCOMPLETED, GetCompleteDownloads(curTab, total) > 0 ? MF_ENABLED : MF_GRAYED);

			m_FileMenu.EnableMenuItem((UINT_PTR)m_A4AFMenu.m_hMenu, (iSelectedItems == 1 && iFilesNotDone == 1) ? MF_ENABLED : MF_GRAYED);
			m_A4AFMenu.CheckMenuItem(MP_ALL_A4AF_AUTO, (iSelectedItems == 1 && iFilesNotDone == 1 && iFilesA4AFAuto == 1) ? MF_CHECKED : MF_UNCHECKED);
			
			m_FileMenu.EnableMenuItem(MP_GETED2KLINK, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_GETHTMLED2KLINK, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_PASTE, theApp.IsEd2kFileLinkInClipboard() ? MF_ENABLED : MF_GRAYED);

			//MORPH START - Added by SiRoB, Show Share Permissions
			m_FileMenu.EnableMenuItem((UINT_PTR)m_PermMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
			CString buffer;
			switch (thePrefs.GetPermissions()){
				case PERM_ALL:
					buffer.Format(" (%s)",GetResString(IDS_FSTATUS_PUBLIC));
					break;
				case PERM_FRIENDS:
					buffer.Format(" (%s)",GetResString(IDS_FSTATUS_FRIENDSONLY));
					break;
				case PERM_NOONE:
					buffer.Format(" (%s)",GetResString(IDS_HIDDEN));
					break;
				// Mighty Knife: Community visible filelist
				case PERM_COMMUNITY:
					buffer.Format(" (%s)",GetResString(IDS_COMMUNITY));
					break;
				// [end] Mighty Knife
				default:
					buffer = " (?)";
					break;
			}
			m_PermMenu.ModifyMenu(MP_PERMDEFAULT, MF_STRING, MP_PERMDEFAULT, GetResString(IDS_DEFAULT) + buffer);
			// Mighty Knife: Community visible filelist
			m_PermMenu.CheckMenuRadioItem(MP_PERMDEFAULT, MP_PERMCOMMUNITY, uPermMenuItem, 0);
			// [end] Mighty Knife
			//MORPH END   - Added by SiRoB, Show Share Permissions

			m_FileMenu.EnableMenuItem(MP_COPYFEEDBACK, iSelectedItems > 0? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_COPYFEEDBACK_US, iSelectedItems > 0? MF_ENABLED : MF_GRAYED);
	
			CMenu WebMenu;
			WebMenu.CreateMenu();
			int iWebMenuEntries = theWebServices.GetFileMenuEntries(WebMenu);
			UINT flag = (iWebMenuEntries == 0 || iSelectedItems != 1) ? MF_GRAYED : MF_ENABLED;
			m_FileMenu.AppendMenu(MF_POPUP | flag, (UINT_PTR)WebMenu.m_hMenu, GetResString(IDS_WEBSERVICES));

			// create cat-submenue
			CMenu CatsMenu;
			CatsMenu.CreateMenu();
			flag = (thePrefs.GetCatCount() == 1) ? MF_GRAYED : MF_ENABLED;
			if (thePrefs.GetCatCount()>1) {
				for (int i = 0; i < thePrefs.GetCatCount(); i++)
					CatsMenu.AppendMenu(MF_STRING,MP_ASSIGNCAT+i, (i==0)?GetResString(IDS_CAT_UNASSIGN):thePrefs.GetCategory(i)->title);
			}
			m_FileMenu.AppendMenu(MF_POPUP | flag, (UINT_PTR)CatsMenu.m_hMenu, GetResString(IDS_TOCAT));
			
			// khaos::categorymod+			
			CTitleMenu mnuOrder;
			if (this->GetSelectedCount() > 1) {
				mnuOrder.CreatePopupMenu();
				mnuOrder.AddMenuTitle(GetResString(IDS_CAT_SETORDER));
				mnuOrder.AppendMenu(MF_STRING, MP_CAT_ORDERAUTOINC, GetResString(IDS_CAT_MNUAUTOINC));
				mnuOrder.AppendMenu(MF_STRING, MP_CAT_ORDERSTEPTHRU, GetResString(IDS_CAT_MNUSTEPTHRU));
				mnuOrder.AppendMenu(MF_STRING, MP_CAT_ORDERALLSAME, GetResString(IDS_CAT_MNUALLSAME));
				m_FileMenu.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)mnuOrder.m_hMenu, GetResString(IDS_CAT_SETORDER));
			}
			else {
				m_FileMenu.AppendMenu(MF_STRING, MP_CAT_SETRESUMEORDER, GetResString(IDS_CAT_SETORDER));
			}
			// khaos::categorymod-

			GetPopupMenuPos(*this, point);
			m_FileMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
			VERIFY( m_FileMenu.RemoveMenu(m_FileMenu.GetMenuItemCount() - 1, MF_BYPOSITION) );
			VERIFY( m_FileMenu.RemoveMenu(m_FileMenu.GetMenuItemCount() - 1, MF_BYPOSITION) );
			if (iPreviewMenuEntries)
				VERIFY( m_FileMenu.RemoveMenu((UINT)PreviewMenu.m_hMenu, MF_BYCOMMAND) );
			//MORPH START - Added by SiRoB, Khaos Category
			m_FileMenu.RemoveMenu(m_FileMenu.GetMenuItemCount()-1,MF_BYPOSITION);
			if (mnuOrder) VERIFY( mnuOrder.DestroyMenu() );
			//MORPH END   - Added by SiRoB, Khaos Category

			VERIFY( WebMenu.DestroyMenu() );
			VERIFY( CatsMenu.DestroyMenu() );
			VERIFY( PreviewMenu.DestroyMenu() );
		}
		else {
			const CUpDownClient* client = (CUpDownClient*)content->value;
			CTitleMenu ClientMenu;
			ClientMenu.CreatePopupMenu();
			ClientMenu.AddMenuTitle(GetResString(IDS_CLIENTS));
			ClientMenu.AppendMenu(MF_STRING,MP_DETAIL, GetResString(IDS_SHOWDETAILS));
			ClientMenu.SetDefaultItem(MP_DETAIL);
			ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && !client->IsFriend()) ? MF_ENABLED : MF_GRAYED), MP_ADDFRIEND, GetResString(IDS_ADDFRIEND));
			//MORPH START - Added by SiRoB, Friend Addon
			ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsFriend()) ? MF_ENABLED : MF_GRAYED), MP_REMOVEFRIEND, GetResString(IDS_REMOVEFRIEND));
			ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsFriend()) ? MF_ENABLED  | ((!client->HasLowID() && client->GetFriendSlot())?MF_CHECKED : MF_UNCHECKED) : MF_GRAYED), MP_FRIENDSLOT, GetResString(IDS_FRIENDSLOT));
			//MORPH END - Added by SiRoB, Friend Addon
			ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient()) ? MF_ENABLED : MF_GRAYED), MP_MESSAGE, GetResString(IDS_SEND_MSG));
			ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetViewSharedFilesSupport()) ? MF_ENABLED : MF_GRAYED), MP_SHOWLIST, GetResString(IDS_VIEWFILES));
			if (Kademlia::CKademlia::isRunning() && !Kademlia::CKademlia::getPrefs()->getLastContact())
				ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetKadPort()!=0) ? MF_ENABLED : MF_GRAYED), MP_BOOT, GetResString(IDS_BOOTSTRAP));

			CMenu A4AFMenu;
			A4AFMenu.CreateMenu();
			if (thePrefs.IsExtControlsEnabled()) {
// ZZ:DownloadManager -->
                if (content->type == UNAVAILABLE_SOURCE) {
                    A4AFMenu.AppendMenu(MF_STRING,MP_A4AF_CHECK_THIS_NOW,GetResString(IDS_A4AF_CHECK_THIS_NOW));
                }
// <-- ZZ:DownloadManager
				if (A4AFMenu.GetMenuItemCount()>0)
					ClientMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)A4AFMenu.m_hMenu, GetResString(IDS_A4AF));
			}
			
			//MORPH START - Added by Yun.SF3, List Requested Files
			ClientMenu.AppendMenu(MF_SEPARATOR);
			ClientMenu.AppendMenu(MF_STRING,MP_LIST_REQUESTED_FILES, _T(GetResString(IDS_LISTREQUESTED))); // Added by sivka
			//MORPH END - Added by Yun.SF3, List Requested Files

			GetPopupMenuPos(*this, point);
			ClientMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
			
			VERIFY( A4AFMenu.DestroyMenu() );
			VERIFY( ClientMenu.DestroyMenu() );
		}
	}
	else{
		int total;
		m_FileMenu.EnableMenuItem((UINT_PTR)m_PrioMenu.m_hMenu, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_CANCEL,MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_PAUSE,MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_STOP,MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_RESUME,MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_OPEN,MF_GRAYED);
		//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
		m_FileMenu.EnableMenuItem(MP_FORCE,MF_GRAYED);//shadow#(onlydownloadcompletefiles)
		//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
		//MORPH START - Added by SiRoB, A4AF
		m_FileMenu.EnableMenuItem(MP_FORCEA4AF, MF_GRAYED);
		m_FileMenu.EnableMenuItem((UINT_PTR)m_A4AFMenuFlag.m_hMenu, MF_GRAYED);
		//MORPH END   - Added by SiRoB, A4AF
		//MORPH START - Added by SiRoB, ShowPermissions
		m_FileMenu.EnableMenuItem((UINT_PTR)m_PermMenu.m_hMenu, MF_GRAYED);
		//MORPH END   - Added by SiRoB, ShowPermissions
		//MORPH START - Added by SiRoB, copy feedback feature
		m_FileMenu.EnableMenuItem(MP_COPYFEEDBACK, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_COPYFEEDBACK_US, MF_GRAYED);
		//MORPH END   - Added by SiRoB, copy feedback feature
		m_FileMenu.EnableMenuItem(MP_PREVIEW,MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_METINFO, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_VIEWFILECOMMENTS, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_CLEARCOMPLETED, GetCompleteDownloads(curTab,total) > 0 ? MF_ENABLED : MF_GRAYED);
		m_FileMenu.EnableMenuItem((UINT_PTR)m_A4AFMenu.m_hMenu, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_GETED2KLINK, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_GETHTMLED2KLINK, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_PASTE, theApp.IsEd2kFileLinkInClipboard() ? MF_ENABLED : MF_GRAYED);
		m_FileMenu.SetDefaultItem((UINT)-1);

		// also show the "Web Services" entry, even if its disabled and therefore not useable, it though looks a little 
		// less confusing this way.
		CMenu WebMenu;
		WebMenu.CreateMenu();
		int iWebMenuEntries = theWebServices.GetFileMenuEntries(WebMenu);
		m_FileMenu.AppendMenu(MF_POPUP | MF_GRAYED, (UINT_PTR)WebMenu.m_hMenu, GetResString(IDS_WEBSERVICES));

		GetPopupMenuPos(*this, point);
		m_FileMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
		m_FileMenu.RemoveMenu(m_FileMenu.GetMenuItemCount() - 1, MF_BYPOSITION);
		VERIFY( WebMenu.DestroyMenu() );
	}
}

BOOL CDownloadListCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case MP_PASTE:
			theApp.PasteClipboard(curTab);
			return TRUE;
	}

	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1)
	{
		const CtrlItem_Struct* content = (CtrlItem_Struct*)GetItemData(iSel);
		if (content->type == FILE_TYPE)
		{
			//for multiple selections 
			UINT selectedCount = 0;
			CTypedPtrList<CPtrList, CPartFile*> selectedList; 
			POSITION pos = GetFirstSelectedItemPosition();
			while(pos != NULL) 
			{ 
				int index = GetNextSelectedItem(pos);
				if(index > -1) 
				{
					if (((const CtrlItem_Struct*)GetItemData(index))->type == FILE_TYPE)
					{
						selectedCount++;
						selectedList.AddTail((CPartFile*)((const CtrlItem_Struct*)GetItemData(index))->value);
					}
				} 
			} 

			CPartFile* file = (CPartFile*)content->value;
			switch (wParam)
			{
				case MPG_DELETE:
				case MP_CANCEL:
				{
					//for multiple selections 
					if(selectedCount > 0)
					{
						SetRedraw(false);
						CString fileList;
						bool validdelete = false;
						bool removecompl =false;
						for (pos = selectedList.GetHeadPosition(); pos != 0; )
						{
							CPartFile* cur_file = selectedList.GetNext(pos);
							if (cur_file->GetStatus() != PS_COMPLETING && cur_file->GetStatus() != PS_COMPLETE){
								validdelete = true;
								if (selectedCount<50)
									fileList.Append(_T("\n") + CString(cur_file->GetFileName()));
							} else 
								if (cur_file->GetStatus() == PS_COMPLETE)
									removecompl=true;

						}
						CString quest;
						if (selectedCount==1)
							quest=GetResString(IDS_Q_CANCELDL2);
						else
							quest=GetResString(IDS_Q_CANCELDL);
						if ( (removecompl && !validdelete ) || (validdelete && AfxMessageBox(quest + fileList,MB_ICONQUESTION|MB_YESNO) == IDYES) )
						{ 
							while(!selectedList.IsEmpty())
							{
								HideSources(selectedList.GetHead());
								switch(selectedList.GetHead()->GetStatus()) { 
									case PS_WAITINGFORHASH: 
									case PS_HASHING: 
									case PS_COMPLETING: 
										selectedList.RemoveHead();
										break;
									case PS_COMPLETE: 
										RemoveFile(selectedList.GetHead());
										selectedList.RemoveHead(); 
										break;
									case PS_PAUSED:
										selectedList.GetHead()->DeleteFile(); 
										selectedList.RemoveHead();
										break;
									default:
										if (thePrefs.StartNextFile())
											theApp.downloadqueue->StartNextFile();
										selectedList.GetHead()->DeleteFile(); 
										selectedList.RemoveHead(); 
								}
							}  
						}
						SetRedraw(true);
					}
					break;
				}
				case MP_PRIOHIGH:
					SetRedraw(false);
					while(!selectedList.IsEmpty()) { 
						CPartFile* partfile = selectedList.GetHead();
						partfile->SetAutoDownPriority(false);
						partfile->SetDownPriority(PR_HIGH);
						selectedList.RemoveHead(); 
					} 
					SetRedraw(true);
					break; 
				case MP_PRIOLOW:
					SetRedraw(false);
					while(!selectedList.IsEmpty()) { 
						CPartFile* partfile = selectedList.GetHead();
						partfile->SetAutoDownPriority(false);
						partfile->SetDownPriority(PR_LOW);
						selectedList.RemoveHead(); 
					}
					SetRedraw(true);
					break; 
				case MP_PRIONORMAL:
					SetRedraw(false);
					while(!selectedList.IsEmpty()) {
						CPartFile* partfile = selectedList.GetHead();
						partfile->SetAutoDownPriority(false);
						partfile->SetDownPriority(PR_NORMAL);
						selectedList.RemoveHead();
					}
					SetRedraw(true);
					break;
				case MP_PRIOAUTO:
					SetRedraw(false);
					while(!selectedList.IsEmpty()) {
						CPartFile* partfile = selectedList.GetHead();
						partfile->SetAutoDownPriority(true);
						partfile->SetDownPriority(PR_HIGH);
						selectedList.RemoveHead();
					}
					SetRedraw(true);
					break;
				case MP_PAUSE:
					SetRedraw(false);
					while(!selectedList.IsEmpty()) { 
						CPartFile* partfile = selectedList.GetHead();
						if (partfile->CanPauseFile())
							partfile->PauseFile();
						selectedList.RemoveHead(); 
					} 
					SetRedraw(true);
					break; 
				case MP_RESUME:
					SetRedraw(false);
					while (!selectedList.IsEmpty()){
						CPartFile* partfile = selectedList.GetHead();
						if (partfile->CanResumeFile()){
						if (partfile->GetStatus() == PS_INSUFFICIENT)
							partfile->ResumeFileInsufficient();
						else
							partfile->ResumeFile();
						}
						selectedList.RemoveHead();
					}
					SetRedraw(true);
					break; 
				//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
				case MP_FORCE:
					SetRedraw(false);
					while(!selectedList.IsEmpty()) {
						CPartFile* partfile = selectedList.GetHead();
						partfile->lastseencomplete = CTime::GetCurrentTime();
						partfile->SavePartFile();
						selectedList.RemoveHead();
					}
					SetRedraw(true);
					break;
				//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
				case MP_STOP:
					SetRedraw(false);
					while(!selectedList.IsEmpty()) { 
						CPartFile *partfile = selectedList.GetHead();
						if (partfile->CanStopFile()){
							HideSources(partfile);
							partfile->StopFile();
						}
						selectedList.RemoveHead();
					}
					SetRedraw(true);
					break; 
				case MP_CLEARCOMPLETED:
					SetRedraw(false);
					ClearCompleted();
					SetRedraw(true);
					break;
				case MP_ALL_A4AF_AUTO:
					file->SetA4AFAuto(!file->IsA4AFAuto());
					break;
				case MPG_F2:
					if (file->GetStatus() != PS_COMPLETE && file->GetStatus() != PS_COMPLETING)
					{
						InputBox inputbox;
						CString title=GetResString(IDS_RENAME);
						title.Remove(_T('&'));
						inputbox.SetLabels(title,GetResString(IDS_DL_FILENAME),file->GetFileName());
						inputbox.SetEditFilenameMode();
						inputbox.DoModal();
						CString nn=inputbox.GetInput();
						if (!inputbox.WasCancelled() && nn.GetLength()>0){
							file->SetFileName(nn,true);
							file->UpdateDisplayedInfo();
							file->SavePartFile(); 
						}
					}
					else
						MessageBeep((UINT)-1);
					break;
				case MPG_ALTENTER:
				case MP_METINFO:
					ShowFileDialog();
						break;
				case MP_GETED2KLINK:{
					CString str;
					while(!selectedList.IsEmpty()) { 
						if (!str.IsEmpty())
							str += _T("\r\n");
						str += CreateED2kLink(selectedList.GetHead());
						selectedList.RemoveHead(); 
					}
					theApp.CopyTextToClipboard(str);
					break; 
				}
				case MP_GETHTMLED2KLINK:{
					CString str;
					while(!selectedList.IsEmpty()) { 
					if (!str.IsEmpty())
							str += _T("<br />\r\n");
						str += CreateHTMLED2kLink(selectedList.GetHead());
						selectedList.RemoveHead(); 
					}
					theApp.CopyTextToClipboard(str);
					break;
				}
				case MP_OPEN:
					if(selectedCount > 1)
						break;
					file->OpenFile();
					break; 
				case MP_PREVIEW:{
					if(selectedCount > 1)
						break;
					file->PreviewFile();
					break;
				}
				case MP_VIEWFILECOMMENTS:
					ShowFileDialog(NULL, true);
					break;
				// xMule_MOD: showSharePermissions
				case MP_PERMDEFAULT:
				case MP_PERMNONE:
				case MP_PERMFRIENDS:
				// Mighty Knife: Community visible filelist
				case MP_PERMCOMMUNITY:
				// [end] Mighty Knife
				case MP_PERMALL: {
					while(!selectedList.IsEmpty()) { 
						CPartFile *file = selectedList.GetHead();
						switch (wParam)
						{
							case MP_PERMDEFAULT:
								file->SetPermissions(-1);
								break;
							case MP_PERMNONE:
								file->SetPermissions(PERM_NOONE);
								break;
							case MP_PERMFRIENDS:
								file->SetPermissions(PERM_FRIENDS);
								break;
							// Mighty Knife: Community visible filelist
							case MP_PERMCOMMUNITY:
								file->SetPermissions(PERM_COMMUNITY);
								break;
							// [end] Mighty Knife
							default : // case MP_PERMALL:
								file->SetPermissions(PERM_ALL);
								break;
						}
						selectedList.RemoveHead();
					}
					Invalidate();
					break;
				}
				// xMule_MOD: showSharePermissions
 				//MORPH START - Added by IceCream, copy feedback feature
 				case MP_COPYFEEDBACK:
				{
					CString feed;
					feed.AppendFormat(GetResString(IDS_FEEDBACK_FROM), thePrefs.GetUserNick(), MOD_VERSION);
					feed.AppendFormat(" \r\n");
					feed.AppendFormat(GetResString(IDS_FEEDBACK_FILENAME), file->GetFileName());
					feed.AppendFormat(" \r\n");
					feed.AppendFormat(GetResString(IDS_FEEDBACK_FILETYPE), file->GetFileType());
					feed.AppendFormat(" \r\n");
					feed.AppendFormat(GetResString(IDS_FEEDBACK_FILESIZE), (file->GetFileSize()/1048576));
					feed.AppendFormat(" \r\n");
					feed.AppendFormat(GetResString(IDS_FEEDBACK_DOWNLOADED), (file->GetCompletedSize()/1048576));
					feed.AppendFormat(" \r\n");
					feed.AppendFormat(GetResString(IDS_FEEDBACK_TOTAL), file->GetSourceCount());
					feed.AppendFormat(" \r\n");
					feed.AppendFormat(GetResString(IDS_FEEDBACK_AVAILABLE),(file->GetAvailableSrcCount()));
					feed.AppendFormat(" \r\n");
					feed.AppendFormat(GetResString(IDS_FEEDBACK_NONEEDPART), file->GetSrcStatisticsValue(DS_NONEEDEDPARTS));
					feed.AppendFormat(" \r\n");
					feed.AppendFormat(GetResString(IDS_FEEDBACK_COMPLETE), file->m_nCompleteSourcesCount);
					//Todo: copy all the comments too
					theApp.CopyTextToClipboard(feed);
					break;
				}
				case MP_COPYFEEDBACK_US:
				{
					CString feed;
					feed.AppendFormat("Feedback from %s ",thePrefs.GetUserNick());
					feed.AppendFormat("on [%s] \r\n",MOD_VERSION);
					feed.AppendFormat("File Name: %s \r\n",file->GetFileName());
					feed.AppendFormat("File Type: %s \r\n",file->GetFileType());
					feed.AppendFormat("Size: %i Mo\r\n", (file->GetFileSize()/1048576));
					feed.AppendFormat("Downloaded: %i Mo\r\n", (file->GetCompletedSize()/1048576));
					feed.AppendFormat("Total sources: %i \r\n",file->GetSourceCount());
					feed.AppendFormat("Available sources : %i \r\n",(file->GetAvailableSrcCount()));
					feed.AppendFormat("No Need Part sources: %i \r\n",file->GetSrcStatisticsValue(DS_NONEEDEDPARTS));
					feed.AppendFormat("Estimate Complete source: %i \r\n",file->m_nCompleteSourcesCount);
					//Todo: copy all the comments too
					theApp.CopyTextToClipboard(feed);
					break;
				}
				//MORPH END - Added by IceCream, copy feedback feature
				// khaos::categorymod+
				// This is only called when there is a single selection, so we'll handle it thusly.
				case MP_CAT_SETRESUMEORDER: {
					InputBox	inputOrder;
					CString		currOrder;

					currOrder.Format("%u", file->GetCatResumeOrder());
					inputOrder.SetLabels(GetResString(IDS_CAT_SETORDER), GetResString(IDS_CAT_ORDER), currOrder);
					inputOrder.SetNumber(true);
					if (inputOrder.DoModal() == IDOK)
					{
					int newOrder = inputOrder.GetInputInt();
						if  (newOrder < 0 || newOrder == file->GetCatResumeOrder()) break;

					file->SetCatResumeOrder(newOrder);
					Invalidate(); // Display the new category.
					}
					break;
				}
				// These next three are only called when there are multiple selections.
				case MP_CAT_ORDERAUTOINC: {
					// This option asks the user for a starting point, and then increments each selected item
					// automatically.  It uses whatever order they appear in the list, from top to bottom.
					InputBox	inputOrder;
					if (selectedCount <= 1) break;
						
					inputOrder.SetLabels(GetResString(IDS_CAT_SETORDER), GetResString(IDS_CAT_EXPAUTOINC), "0");
					inputOrder.SetNumber(true);
                    if (inputOrder.DoModal() == IDOK)
					{
					int newOrder = inputOrder.GetInputInt();
					if  (newOrder < 0) break;

					while (!selectedList.IsEmpty()) {
						selectedList.GetHead()->SetCatResumeOrder(newOrder);
						newOrder++;
						selectedList.RemoveHead();
					}
					Invalidate();
					}
					break;
				}
				case MP_CAT_ORDERSTEPTHRU: {
					// This option asks the user for a different resume modifier for each file.  It
					// displays the filename in the inputbox so that they don't get confused about
					// which one they're setting at any given moment.
					InputBox	inputOrder;
					CString		currOrder;
					CString		currFile;
					CString		currInstructions;
					int			newOrder = 0;

					if (selectedCount <= 1) break;
					inputOrder.SetNumber(true);

					while (!selectedList.IsEmpty()) {
						currOrder.Format("%u", selectedList.GetHead()->GetCatResumeOrder());
						currFile = selectedList.GetHead()->GetFileName();
                        if (currFile.GetLength() > 50) currFile = currFile.Mid(0,47) + "...";
						currInstructions.Format("%s %s", GetResString(IDS_CAT_EXPSTEPTHRU), currFile);
						inputOrder.SetLabels(GetResString(IDS_CAT_SETORDER), currInstructions, currOrder);

						if (inputOrder.DoModal() == IDCANCEL) {
							if (MessageBox(GetResString(IDS_CAT_ABORTSTEPTHRU), GetResString(IDS_ABORT), MB_YESNO) == IDYES) {
								break;
							}
							else {
								selectedList.RemoveHead();
								continue;
							}
						}

						newOrder = inputOrder.GetInputInt();
						selectedList.GetHead()->SetCatResumeOrder(newOrder);
						selectedList.RemoveHead();
					}
					RedrawItems(0, GetItemCount() - 1);
					break;
				}
				case MP_CAT_ORDERALLSAME: {
					// This option asks the user for a single resume modifier and applies it to
					// all the selected files.
					InputBox	inputOrder;
					CString		currOrder;

					if (selectedCount <= 1) break;

					inputOrder.SetLabels(GetResString(IDS_CAT_SETORDER), GetResString(IDS_CAT_EXPALLSAME), "0");
					inputOrder.SetNumber(true);
					if (inputOrder.DoModal() == IDCANCEL)
						break;

					int newOrder = inputOrder.GetInputInt();
					if  (newOrder < 0) break;

					while (!selectedList.IsEmpty()) {
						selectedList.GetHead()->SetCatResumeOrder(newOrder);
						selectedList.RemoveHead();
					}
					RedrawItems(0, GetItemCount() - 1);
					break;
				}
				case MP_FORCEA4AF: {
					if (file && theApp.downloadqueue->forcea4af_file != file)
						theApp.downloadqueue->forcea4af_file = file;
					else if (file && theApp.downloadqueue->forcea4af_file == file)
						theApp.downloadqueue->forcea4af_file = NULL;
					break;
				}
				case MP_FORCEA4AFONFLAG: {
					while (!selectedList.IsEmpty()) {
						CPartFile* cur_file = (CPartFile*)selectedList.RemoveHead();
						cur_file->SetForceAllA4AF(cur_file->ForceAllA4AF() ? false : true);
						if (cur_file->ForceAllA4AF())
							cur_file->SetForceA4AFOff(false);
					}
					break;
				}
				case MP_FORCEA4AFOFFFLAG: {
					while (!selectedList.IsEmpty()) {
						CPartFile* cur_file = (CPartFile*)selectedList.RemoveHead();
						cur_file->SetForceA4AFOff(cur_file->ForceA4AFOff() ? false : true);
						if (cur_file->ForceA4AFOff())
							cur_file->SetForceAllA4AF(false);
					}
					break;
				}
				// khaos::kmod-
				default:
					if (wParam>=MP_WEBURL && wParam<=MP_WEBURL+99){
						theWebServices.RunURL(file, wParam);
					}
					else if (wParam>=MP_ASSIGNCAT && wParam<=MP_ASSIGNCAT+99){
						SetRedraw(FALSE);
						while (!selectedList.IsEmpty()){
							CPartFile *partfile = selectedList.GetHead();
							partfile->SetCategory(wParam - MP_ASSIGNCAT);
							partfile->UpdateDisplayedInfo(true);
							selectedList.RemoveHead();
						}
						SetRedraw(TRUE);
						ChangeCategory(curTab);
					}
					else if (wParam>=MP_PREVIEW_APP_MIN && wParam<=MP_PREVIEW_APP_MAX){
						thePreviewApps.RunApp(file, wParam);
					}
					break;
			}
		}
		else{
			CUpDownClient* client = (CUpDownClient*)content->value;
			CPartFile* file = (CPartFile*)content->owner; // added by sivka

			switch (wParam){
				case MP_SHOWLIST:
					client->RequestSharedFileList();
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
						this->UpdateItem(client);
					}
					//MORPH END - Modified by SIRoB, Added by Yun.SF3, ZZ Upload System
					break;
				}
				case MP_REMOVEFRIEND:{//LSD
					if (client && client->IsFriend())
					{					
						theApp.friendlist->RemoveFriend(client->m_Friend);
						this->UpdateItem(client);
					}
					break;
				}
				//Xman end
				//MORPH END   - Added by SIRoB, Friend Addon
				case MP_MESSAGE:
					theApp.emuledlg->chatwnd->StartSession(client);
					break;
				case MP_ADDFRIEND:
					if (theApp.friendlist->AddFriend(client))
						UpdateItem(client);
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
// ZZ:DownloadManager -->
				case MP_A4AF_CHECK_THIS_NOW:
					if(file->GetStatus(false) == PS_READY || file->GetStatus(false) == PS_EMPTY)
					{
						if (client->GetDownloadState() != DS_DOWNLOADING)
						{
							client->SwapToAnotherFile(_T("Manual init of source check. Test to be like ProcessA4AFClients(). CDownloadListCtrl::OnCommand() MP_SWAP_A4AF_DEBUG_THIS"), false, false, false, NULL, true, true, true); // ZZ:DownloadManager
							UpdateItem(file);
						}
					}
					break;
				//MORPH START - Added by Yun.SF3, List Requested Files
				case MP_LIST_REQUESTED_FILES: { // added by sivka
					if (client != NULL)
					{
						CString fileList;
						fileList += GetResString(IDS_LISTREQDL);
						fileList += "\n--------------------------\n" ; 
						if (theApp.downloadqueue->IsPartFile(client->reqfile))
						{
							fileList += client->reqfile->GetFileName(); 
							for(POSITION pos = client->m_OtherRequests_list.GetHeadPosition();pos!=0;client->m_OtherRequests_list.GetNext(pos))
							{
								fileList += "\n" ; 
								fileList += client->m_OtherRequests_list.GetAt(pos)->GetFileName(); 
							}
							for(POSITION pos = client->m_OtherNoNeeded_list.GetHeadPosition();pos!=0;client->m_OtherNoNeeded_list.GetNext(pos))
							{
								fileList += "\n" ;
								fileList += client->m_OtherNoNeeded_list.GetAt(pos)->GetFileName();
							}
						}
						else
							fileList += GetResString(IDS_LISTREQNODL);
						fileList += "\n\n\n";
						fileList += GetResString(IDS_LISTREQUL);
						fileList += "\n------------------------\n" ; 
						CKnownFile* uploadfile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
						if(uploadfile)
							fileList += uploadfile->GetFileName();
						else
							fileList += GetResString(IDS_LISTREQNOUL);
						AfxMessageBox(fileList,MB_OK);
					}
					break;
				}
				//MORPH END - Added by Yun.SF3, List Requested Files
			}
		}
	}
	else /*nothing selected*/
	{
		switch (wParam){
			case MP_CLEARCOMPLETED:
				ClearCompleted();
				break;
		}

	}

	return TRUE;
}

void CDownloadListCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult){
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableDownload);
	bool m_oldSortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableDownload);

	int userSort = (GetAsyncKeyState(VK_CONTROL) < 0) ? 0x4000:0;	// SLUGFILLER: DLsortFix Ctrl sorts sources only
	//MORPH START - Removed by SiRoB, Remain time and size Columns have been splited
	/*
	if (sortItem==9) {
		m_bRemainSort=(sortItem != pNMListView->iSubItem) ? false : (m_oldSortAscending?m_bRemainSort:!m_bRemainSort);
	}
	*/
	//MORPH START - Removed by SiRoB, Remain time and size Columns have been splited
	bool sortAscending = (sortItem != pNMListView->iSubItem + userSort) ? (pNMListView->iSubItem == 0) : !m_oldSortAscending;	// SLUGFILLER: DLsortFix - descending by default for all but filename/username

	// Item is column clicked
	sortItem = pNMListView->iSubItem + userSort;	// SLUGFILLER: DLsortFix

	// Save new preferences
	thePrefs.SetColumnSortItem(CPreferences::tableDownload, sortItem);
	thePrefs.SetColumnSortAscending(CPreferences::tableDownload, sortAscending);
	//MORPH START - Changed by SiRoB, Remain time and size Columns have been splited
	/*
	thePrefs.TransferlistRemainSortStyle(m_bRemainSort);
	// Sort table
	uint8 adder=0;
	if (sortItem!=9 || !m_bRemainSort)
		SetSortArrow(sortItem & 0xCFFF, sortAscending);	// SLUGFILLER: DLsortFix
	else {
        SetSortArrow(sortItem, sortAscending?arrowDoubleUp : arrowDoubleDown);
		adder=81;
	}

	SortItems(SortProc, sortItem + (sortAscending ? 0:100) + adder);
	*/
	SetSortArrow(sortItem & 0xCFFF, sortAscending);	// SLUGFILLER: DLsortFix
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));
	//MORPH END  - Changed by SiRoB, Remain time and size Columns have been splited
	*pResult = 0;
}

int CDownloadListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){
	CtrlItem_Struct* item1 = (CtrlItem_Struct*)lParam1;
	CtrlItem_Struct* item2 = (CtrlItem_Struct*)lParam2;

	int sortMod = 1;
	if((lParamSort & 0x3FFF) >= 100) {	// SLUGFILLER: DLsortFix
		sortMod = -1;
		lParamSort -= 100;
	}

	int comp;

	if(item1->type == FILE_TYPE && item2->type != FILE_TYPE) {
		if(item1->value == item2->parent->value)
			return -1;

		comp = Compare((CPartFile*)item1->value, (CPartFile*)(item2->parent->value), lParamSort);

	} else if(item2->type == FILE_TYPE && item1->type != FILE_TYPE) {
		if(item1->parent->value == item2->value)
			return 1;

		comp = Compare((CPartFile*)(item1->parent->value), (CPartFile*)item2->value, lParamSort);

	} else if (item1->type == FILE_TYPE) {
		CPartFile* file1 = (CPartFile*)item1->value;
		CPartFile* file2 = (CPartFile*)item2->value;

		comp = Compare(file1, file2, lParamSort);

	} else {
		// SLUGFILLER: DLsortFix - never compare sources of different files
		if (item1->parent->value != item2->parent->value)
			return sortMod * Compare((CPartFile*)(item1->parent->value), (CPartFile*)(item2->parent->value), lParamSort);
		// SLUGFILLER: DLsortFix
		if (item1->type != item2->type)
			return item1->type - item2->type;

		CUpDownClient* client1 = (CUpDownClient*)item1->value;
		CUpDownClient* client2 = (CUpDownClient*)item2->value;
		comp = Compare(client1, client2, lParamSort,sortMod);
	}

	return sortMod * comp;
}


void CDownloadListCtrl::ClearCompleted(bool ignorecats){
	// Search for completed file(s)
	for(ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); ){
		CtrlItem_Struct* cur_item = it->second;
		it++; // Already point to the next iterator. 
		if(cur_item->type == FILE_TYPE){
			CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);
			if(file->IsPartFile() == false && (file->CheckShowItemInGivenCat(curTab) || ignorecats) ){
				if (RemoveFile(file))
					it = m_ListItems.begin();
			}
		}
	}
}

void CDownloadListCtrl::ClearCompleted(const CPartFile* pFile)
{
	if (!pFile->IsPartFile())
	{
		for (ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); )
		{
			CtrlItem_Struct* cur_item = it->second;
			it++;
			if (cur_item->type == FILE_TYPE)
			{
				const CPartFile* pCurFile = reinterpret_cast<CPartFile*>(cur_item->value);
				if (pCurFile == pFile)
				{
					RemoveFile(pCurFile);
					return;
				}
			}
		}
	}
}

void CDownloadListCtrl::SetStyle() {
	if (thePrefs.IsDoubleClickEnabled())
		SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	else
		SetExtendedStyle(LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
}

void CDownloadListCtrl::OnListModified(NMHDR *pNMHDR, LRESULT *pResult) {
	NM_LISTVIEW *pNMListView = (NM_LISTVIEW*)pNMHDR;

	//this works because true is equal to 1 and false equal to 0
	BOOL notLast = pNMListView->iItem + 1 != GetItemCount();
	BOOL notFirst = pNMListView->iItem != 0;
	RedrawItems(pNMListView->iItem - notFirst, pNMListView->iItem + notLast);
}

int CDownloadListCtrl::Compare(const CPartFile* file1, const CPartFile* file2, LPARAM lParamSort)
{
	// SLUGFILLER: DLsortFix
	if (lParamSort & 0x4000)
		return 0;
	// SLUGFILLER: DLsortFix
	switch(lParamSort){
	case 0: //filename asc
		return _tcsicmp(file1->GetFileName(),file2->GetFileName());
	case 1: //size asc
		return CompareUnsigned(file1->GetFileSize(), file2->GetFileSize());
	case 2: //transfered asc
		return CompareUnsigned(file1->GetTransfered(), file2->GetTransfered());
	case 3: //completed asc
		return CompareUnsigned(file1->GetCompletedSize(), file2->GetCompletedSize());
	case 4: //speed asc
		return CompareUnsigned(file1->GetDatarate(), file2->GetDatarate());
	case 5: //progress asc
		{
			float comp = file1->GetPercentCompleted() - file2->GetPercentCompleted();
			if(comp > 0)
				return 1;
			else if(comp < 0)
				return -1;
			else
				return 0;
		}
	case 6: //sources asc
		return file1->GetSourceCount() - file2->GetSourceCount();
	case 7: //priority asc
		return file1->GetDownPriority() - file2->GetDownPriority();
	case 8: //Status asc 
		return file2->getPartfileStatusRang()-file1->getPartfileStatusRang();	// SLUGFILLER: DLsortFix - Low StatusRang<->Better status<->High status
	case 9: //Remaining Time asc 
	{
		//Make ascending sort so we can have the smaller remaining time on the top 
		//instead of unknowns so we can see which files are about to finish better..
		sint32 f1 = file1->getTimeRemaining();
		sint32 f2 = file2->getTimeRemaining();
		//Same, do nothing.
		if( f1 == f2 )
			return 0;
		//If decending, put first on top as it is unknown
		//If ascending, put first on bottom as it is unknown
		if( f1 == -1 )
			return 1;
		//If decending, put second on top as it is unknown
		//If ascending, put second on bottom as it is unknown
		if( f2 == -1 )
			return -1;
		//If decending, put first on top as it is bigger.
		//If ascending, put first on bottom as it is bigger.
		return f1 - f2;
	}
	//MORPH - Removed by SiRoB, Remain time and size Columns have been splited
	/*
	case 90: //Remaining SIZE asc 
		return CompareUnsigned(file1->GetFileSize()-file1->GetCompletedSize(), file2->GetFileSize()-file2->GetCompletedSize());
	*/
	case 10: //last seen complete asc 
		if (file1->lastseencomplete > file2->lastseencomplete)
			return 1;
		else if(file1->lastseencomplete < file2->lastseencomplete)
			return -1;
		else
			return 0;
	case 11: //last received Time asc 
		if (file1->GetFileDate() > file2->GetFileDate())
			return 1;
		else if(file1->GetFileDate() < file2->GetFileDate())
			return -1;
		else
			return 0;

	// khaos::categorymod+
	case 12: // Cat
		if (file1->GetCategory() > file2->GetCategory())
			return 1;
		else if (file1->GetCategory() < file2->GetCategory())
			return -1;
		else
			return 0;
	case 13: // Mod
		if (file1->GetCatResumeOrder() > file2->GetCatResumeOrder())
			return 1;
		else if (file1->GetCatResumeOrder() < file2->GetCatResumeOrder())
			return -1;
		else
			return 0;
	case 14: // Remaining Bytes
		if ((file1->GetFileSize() - file1->GetCompletedSize()) > (file2->GetFileSize() - file2->GetCompletedSize()))
			return 1;
		else if ((file1->GetFileSize() - file1->GetCompletedSize()) < (file2->GetFileSize() - file2->GetCompletedSize()))
			return -1;
		else
			return 0;
	// khaos::categorymod-
	//MORPH START - Added by IceCream, SLUGFILLER: DLsortFix
	case 0x8000:
		return file1->GetPartMetFileName().CompareNoCase(file2->GetPartMetFileName());
	// SLUGFILLER: DLsortFix
	default:
		return 0;
	}
}

int CDownloadListCtrl::Compare(const CUpDownClient *client1, const CUpDownClient *client2, LPARAM lParamSort, int sortMod)
{
	lParamSort &= 0xBFFF;	// SLUGFILLER: DLsortFix
	switch(lParamSort){
	case 0: //name asc
		if(client1->GetUserName() == client2->GetUserName())
			return 0;
		else if(!client1->GetUserName())
			return 1;
		else if(!client2->GetUserName())
			return -1;
		return _tcsicmp(client1->GetUserName(),client2->GetUserName());
	case 1: //size but we use status asc
		return client1->GetSourceFrom() - client2->GetSourceFrom();
	case 2://transfered asc
		//MORPH START - Added By SiRoB, Download/Upload
		if (!client1->Credits())
			return 1;
		else if (!client2->Credits())
			return -1;
		return client2->Credits()->GetDownloadedTotal() - client1->Credits()->GetDownloadedTotal();
		//MORPH END - Added By SiRoB, Download/Upload
	case 3://completed asc
		return CompareUnsigned(client1->GetTransferedDown(), client2->GetTransferedDown());
	case 4: //speed asc
		return CompareUnsigned(client1->GetDownloadDatarate(), client2->GetDownloadDatarate());
	case 5: //progress asc
		return client1->GetAvailablePartCount() - client2->GetAvailablePartCount();
	case 6:
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
		// Maella -Support for tag ET_MOD_VERSION 0x55-
		if( client1->GetClientSoft() == client2->GetClientSoft() )
			if(client2->GetVersion() == client1->GetVersion() && client1->GetClientSoft() == SO_EMULE){
				return strcmpi(client2->DbgGetFullClientSoftVer(), client1->DbgGetFullClientSoftVer());
			}
			else {
			return client2->GetVersion() - client1->GetVersion();
			}
		else
		return client1->GetClientSoft() - client2->GetClientSoft();
		// Maella end
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
	case 7: //qr asc
		//MORPH START - Added by IceCream, SLUGFILLER: DLsortFix - needed complete revamping
		if ( client1->GetDownloadState() == DS_DOWNLOADING ){
			if ( client2->GetDownloadState() != DS_DOWNLOADING )
				return 1;
			return CompareUnsigned(client1->GetDownloadDatarate(), client2->GetDownloadDatarate());
		}
		else if (client2->GetDownloadState() == DS_DOWNLOADING)
			return -1;

		if ( client1->GetRemoteQueueRank() != 0 ){
			if ( client2->GetRemoteQueueRank() == 0 )
				return 1;
			return client2->GetRemoteQueueRank() - client1->GetRemoteQueueRank();
		}
		else if ( client2->GetRemoteQueueRank() != 0 )
				return -1;

		if ( client1->GetDownloadState() == DS_ONQUEUE && !client1->IsRemoteQueueFull() ){
			if ( client2->GetDownloadState() != DS_ONQUEUE || client2->IsRemoteQueueFull() )
			return 1;
			return 0;
		}
		else if ( client2->GetDownloadState() == DS_ONQUEUE && !client2->IsRemoteQueueFull() )
			return -1;

		return 0;
		//MORPH END   - Added by IceCream, SLUGFILLER: DLsortFix
	case 8:
		if( client1->GetDownloadState() == client2->GetDownloadState() ){
			//MORPH START - Added by IceCream, SLUGFILLER: DLsortFix - needed partial revamping
			if( !client1->IsRemoteQueueFull() ){
				if( client2->IsRemoteQueueFull() )
					return 1;
				return 0;
			}
			else if( !client2->IsRemoteQueueFull() )
				return -1;
			return 0;
			//MORPH END   - Added by IceCream, SLUGFILLER: DLsortFix
		}
		return client2->GetDownloadState() - client1->GetDownloadState();	//MORPH - Added by IceCream, SLUGFILLER: DLsortFix - match status sorting
	default:
		return 0;
	}
}

void CDownloadListCtrl::OnNMDblclkDownloadlist(NMHDR *pNMHDR, LRESULT *pResult) {
	int iSel = GetSelectionMark();
	if (iSel != -1){
		CtrlItem_Struct* content = (CtrlItem_Struct*)this->GetItemData(iSel);
		if (content && content->value){
			if (content->type == FILE_TYPE){
				if (!thePrefs.IsDoubleClickEnabled()){
					CPoint pt;
					::GetCursorPos(&pt);
					ScreenToClient(&pt);
					LVHITTESTINFO hit;
					hit.pt = pt;
					if (HitTest(&hit) >= 0 && (hit.flags & LVHT_ONITEM)){
						LVHITTESTINFO subhit;
						subhit.pt = pt;
						if (SubItemHitTest(&subhit) >= 0 && subhit.iSubItem == 0){
							ShowFileDialog((CPartFile*)content->value);
						}
					}
				}
			}
			else{
				CClientDetailDialog dialog((CUpDownClient*)content->value);
				dialog.DoModal();
			}
		}
	}
	
	*pResult = 0;
}

void CDownloadListCtrl::CreateMenues() {
	if (m_PrioMenu) VERIFY( m_PrioMenu.DestroyMenu() );
	// khaos::kmod+
	if (m_A4AFMenu)	VERIFY( m_A4AFMenu.DestroyMenu() );
	if (m_PermMenu) VERIFY( m_PermMenu.DestroyMenu() );	// xMule_MOD: showSharePermissions
	//MORPH START - Added by SiRoB, Advanced A4AF Flag derivated from Khaos
	if (m_A4AFMenuFlag)	VERIFY( m_A4AFMenuFlag.DestroyMenu() );
	//MORPH END   - Added by SiRoB, Advanced A4AF Flag derivated from Khaos
	if (m_FileMenu) VERIFY( m_FileMenu.DestroyMenu() );

	m_PrioMenu.CreateMenu();
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOLOW,GetResString(IDS_PRIOLOW));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIONORMAL,GetResString(IDS_PRIONORMAL));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOHIGH, GetResString(IDS_PRIOHIGH));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOAUTO, GetResString(IDS_PRIOAUTO));

	m_A4AFMenu.CreateMenu();
// ZZ:DownloadManager -->
	//m_A4AFMenu.AppendMenu(MF_STRING, MP_ALL_A4AF_TO_THIS, GetResString(IDS_ALL_A4AF_TO_THIS)); // sivka [Tarod]
	//m_A4AFMenu.AppendMenu(MF_STRING, MP_ALL_A4AF_TO_OTHER, GetResString(IDS_ALL_A4AF_TO_OTHER)); // sivka
// <-- ZZ:DownloadManager
	m_A4AFMenu.AppendMenu(MF_STRING, MP_ALL_A4AF_AUTO, GetResString(IDS_ALL_A4AF_AUTO)); // sivka [Tarod]
	
	// xMule_MOD: showSharePermissions
	m_PermMenu.CreateMenu();
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMDEFAULT,	GetResString(IDS_DEFAULT));
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMNONE,	GetResString(IDS_HIDDEN));
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMFRIENDS,	GetResString(IDS_FSTATUS_FRIENDSONLY));
	// Mighty Knife: Community visible filelist
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMCOMMUNITY,GetResString(IDS_COMMUNITY));
	// [end] Mighty Knife
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMALL,		GetResString(IDS_FSTATUS_PUBLIC));
	// xMule_MOD: showSharePermissions

	//MORPH START - Added by SiRoB, Advanced A4AF Flag derivated from Khaos
	m_A4AFMenuFlag.CreateMenu();
	m_A4AFMenuFlag.AppendMenu(MF_STRING, MP_FORCEA4AFONFLAG, GetResString(IDS_A4AF_ONFLAG));
	m_A4AFMenuFlag.AppendMenu(MF_STRING, MP_FORCEA4AFOFFFLAG, GetResString(IDS_A4AF_OFFFLAG));
	//MORPH END   - Added by SiRoB, Advanced A4AF Flag derivated from Khaos

	m_FileMenu.CreatePopupMenu();
	m_FileMenu.AddMenuTitle(GetResString(IDS_DOWNLOADMENUTITLE));
	// khaos::kmod+
	m_FileMenu.AppendMenu(MF_STRING, MP_FORCEA4AF, GetResString(IDS_A4AF_FORCEALL));
	m_FileMenu.AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_A4AFMenuFlag.m_hMenu, GetResString(IDS_A4AF_FLAGS));
	if (thePrefs.IsExtControlsEnabled()) m_FileMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_A4AFMenu.m_hMenu, GetResString(IDS_A4AF));
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	// khaos::kmod-
	m_FileMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PermMenu.m_hMenu, GetResString(IDS_PERMISSION));	// xMule_MOD: showSharePermissions
	m_FileMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PrioMenu.m_hMenu, GetResString(IDS_PRIORITY) + _T(" (") + GetResString(IDS_DOWNLOAD) + _T(")"));
	// khaos::kmod+
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	// khaos::kmod-
	m_FileMenu.AppendMenu(MF_STRING,MP_PAUSE, GetResString(IDS_DL_PAUSE));
	m_FileMenu.AppendMenu(MF_STRING,MP_STOP, GetResString(IDS_DL_STOP));
	m_FileMenu.AppendMenu(MF_STRING,MP_RESUME, GetResString(IDS_DL_RESUME));
	m_FileMenu.AppendMenu(MF_STRING,MP_CANCEL,GetResString(IDS_MAIN_BTN_CANCEL) );
	//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
	m_FileMenu.AppendMenu(MF_STRING,MP_FORCE, GetResString(IDS_DL_FORCE));//shadow#(onlydownloadcompletefiles)
	//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	m_FileMenu.AppendMenu(MF_STRING,MP_OPEN, GetResString(IDS_DL_OPEN) );//<--9/21/02
	m_FileMenu.AppendMenu(MF_STRING,MP_PREVIEW, GetResString(IDS_DL_PREVIEW) );
	m_FileMenu.AppendMenu(MF_STRING,MP_METINFO, GetResString(IDS_DL_INFO) );//<--9/21/02
	m_FileMenu.AppendMenu(MF_STRING,MP_VIEWFILECOMMENTS, GetResString(IDS_CMT_SHOWALL) );

	m_FileMenu.AppendMenu(MF_SEPARATOR);
	m_FileMenu.AppendMenu(MF_STRING,MP_CLEARCOMPLETED, GetResString(IDS_DL_CLEAR));
	
	//MORPH - Moved by SiRoB, see on top
	/*
	if (thePrefs.IsExtControlsEnabled())
		m_FileMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_A4AFMenu.m_hMenu, GetResString(IDS_A4AF));
	*/

	m_FileMenu.AppendMenu(MF_SEPARATOR);
	m_FileMenu.AppendMenu(MF_STRING,MP_GETED2KLINK, GetResString(IDS_DL_LINK1) );
	m_FileMenu.AppendMenu(MF_STRING,MP_GETHTMLED2KLINK, GetResString(IDS_DL_LINK2));
	m_FileMenu.AppendMenu(MF_STRING,MP_PASTE, GetResString(IDS_SW_DIRECTDOWNLOAD));
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	//MORPH START - Added by IceCream, copy feedback feature
	m_FileMenu.AppendMenu(MF_STRING,MP_COPYFEEDBACK, GetResString(IDS_COPYFEEDBACK));
	m_FileMenu.AppendMenu(MF_STRING,MP_COPYFEEDBACK_US, GetResString(IDS_COPYFEEDBACK_US));
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	//MORPH END   - Added by IceCream, copy feedback feature

}

CString CDownloadListCtrl::getTextList() {
	CString out;

	// Search for file(s)
	for(ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++){ // const is better
		CtrlItem_Struct* cur_item = it->second;
		if(cur_item->type == FILE_TYPE){
			CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);

			TCHAR buffer[50+1];
			ASSERT( !file->GetFileName().IsEmpty() );
			_tcsncpy(buffer, file->GetFileName(), 50);
			buffer[50] = _T('\0');

			CString temp;
			temp.Format(_T("\n%s\t [%.1f%%] %i/%i - %s"),
						buffer,
						file->GetPercentCompleted(),
						file->GetTransferingSrcCount(),
						file->GetSourceCount(), 
						file->getPartfileStatus());
			
			out += temp;
		}
	}

	return out;
}

// khaos::categorymod+
void CDownloadListCtrl::ShowFilesCount() {
	uint16	count=0;
	CString counter;
	CtrlItem_Struct* cur_item;
	uint16	totcnt=0;

	for(ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++){
		cur_item = it->second;
		if (cur_item->type == FILE_TYPE){
			CPartFile* file=(CPartFile*)cur_item->value;
			if (file->CheckShowItemInGivenCat(curTab))
				++count;
			++totcnt;
		}
	}

	if (thePrefs.GetCategory(curTab))
		counter.Format(_T("%s: %u (%u Total | %s)"), GetResString(IDS_TW_DOWNLOADS),count,totcnt,thePrefs.GetCategory(curTab)->viewfilters.bSuspendFilters ? GetResString(IDS_CAT_FILTERSSUSP) : GetResString(IDS_CAT_FILTERSACTIVE));
	theApp.emuledlg->transferwnd->GetDlgItem(IDC_DOWNLOAD_TEXT)->SetWindowText(counter);
}
// khaos::categorymod-

void CDownloadListCtrl::ShowSelectedFileDetails()
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

	CtrlItem_Struct* content = (CtrlItem_Struct*)this->GetItemData(GetSelectionMark());
	if (content->type == FILE_TYPE){
		CPartFile* file = (CPartFile*)content->value;

		if ((file->HasComment() || file->HasRating()) && p.x<13 ) {
			ShowFileDialog(NULL, true);
		}
		else {
			ShowFileDialog();
		}
	}
	else {
		CUpDownClient* client = (CUpDownClient*)content->value;
		CClientDetailDialog dialog(client);
		dialog.DoModal();
	}
}

int CDownloadListCtrl::GetCompleteDownloads(int cat, int &total){
	int count=0;
	total=0;

	for(ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++){
		const CtrlItem_Struct* cur_item = it->second;
		if (cur_item->type == FILE_TYPE){
			CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);
			
			if ( file->CheckShowItemInGivenCat(cat)) {
				++total;

				if (file->GetStatus()==PS_COMPLETE  )
					++count;
			}
		}
	}

	return count;
}


void CDownloadListCtrl::ChangeCategory(int newsel){

	SetRedraw(FALSE);

	// remove all displayed files with a different cat and show the correct ones
	for(ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++){
		const CtrlItem_Struct* cur_item = it->second;
		if (cur_item->type == FILE_TYPE){
			CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);
			
			if (!file->CheckShowItemInGivenCat(newsel))
				HideFile(file);
			else
				ShowFile(file);
		}
	}

	SetRedraw(TRUE);
	curTab=newsel;
	ShowFilesCount();
}

void CDownloadListCtrl::HideFile(CPartFile* tohide)
{
	HideSources(tohide);

	// Retrieve all entries matching the source
	std::pair<ListItems::const_iterator, ListItems::const_iterator> rangeIt = m_ListItems.equal_range(tohide);
	for(ListItems::const_iterator it = rangeIt.first; it != rangeIt.second; it++){
		CtrlItem_Struct* updateItem  = it->second;

		// Find entry in CListCtrl and update object
 		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)updateItem;
		sint16 result = FindItem(&find);
		if(result != (-1)) {
			DeleteItem(result);
			return;
		}
	}
}

void CDownloadListCtrl::ShowFile(CPartFile* toshow){
	// Retrieve all entries matching the source
	std::pair<ListItems::const_iterator, ListItems::const_iterator> rangeIt = m_ListItems.equal_range(toshow);
	ListItems::const_iterator it = rangeIt.first;
	if(it != rangeIt.second){
		CtrlItem_Struct* updateItem  = it->second;

		// Check if entry is already in the List
 		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)updateItem;
		sint16 result = FindItem(&find);
		if(result == (-1))
			InsertItem(LVIF_PARAM|LVIF_TEXT,GetItemCount(),LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)updateItem);
	}
}

void CDownloadListCtrl::GetDisplayedFiles(CArray<CPartFile*,CPartFile*> *list){
	for(ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); ){
		CtrlItem_Struct* cur_item = it->second;
		it++; // Already point to the next iterator. 
		if(cur_item->type == FILE_TYPE){
			CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);
			list->Add(file);
		}
	}	
}

void CDownloadListCtrl::MoveCompletedfilesCat(uint8 from, uint8 to){
	uint8 mycat;

	for(ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); ){
		CtrlItem_Struct* cur_item = it->second;
		it++; // Already point to the next iterator.
		if(cur_item->type == FILE_TYPE){
			CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);
			if (!file->IsPartFile()){
				mycat=file->GetCategory();
				if ( mycat>=min(from,to) && mycat<=max(from,to)) {
					if (mycat==from) 
						file->SetCategory(to); 
					else
						if (from<to) file->SetCategory(mycat-1);
							else file->SetCategory(mycat+1);
				}
			}
		}
	}
}

void CDownloadListCtrl::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMLVDISPINFO* pDispInfo = (NMLVDISPINFO*)pNMHDR;
	/*TRACE("CDownloadListCtrl::OnGetDispInfo iItem=%d iSubItem=%d", pDispInfo->item.iItem, pDispInfo->item.iSubItem);
	if (pDispInfo->item.mask & LVIF_TEXT)
		TRACE(" LVIF_TEXT");
	if (pDispInfo->item.mask & LVIF_IMAGE)
		TRACE(" LVIF_IMAGE");
	if (pDispInfo->item.mask & LVIF_STATE)
		TRACE(" LVIF_STATE");
	TRACE("\n");*/

	// Although we have an owner drawn listview control we store the text for the primary item in the listview, to be
	// capable of quick searching those items via the keyboard. Because our listview items may change their contents,
	// we do this via a text callback function. The listview control will send us the LVN_DISPINFO notification if
	// it needs to know the contents of the primary item.
	//
	// But, the listview control sends this notification all the time, even if we do not search for an item. At least
	// this notification is only sent for the visible items and not for all items in the list. Though, because this
	// function is invoked *very* often, no *NOT* put any time consuming code here in.

    if (pDispInfo->item.mask & LVIF_TEXT){
        const CtrlItem_Struct* pItem = reinterpret_cast<CtrlItem_Struct*>(pDispInfo->item.lParam);
        if (pItem != NULL && pItem->value != NULL){
			if (pItem->type == FILE_TYPE){
				switch (pDispInfo->item.iSubItem){
					case 0:
						if (pDispInfo->item.cchTextMax > 0){
							_tcsncpy(pDispInfo->item.pszText, ((CPartFile*)pItem->value)->GetFileName(), pDispInfo->item.cchTextMax);
							pDispInfo->item.pszText[pDispInfo->item.cchTextMax-1] = _T('\0');
						}
						break;
					default:
						// shouldn't happen
						pDispInfo->item.pszText[0] = _T('\0');
						break;
				}
			}
			else if (pItem->type == UNAVAILABLE_SOURCE || pItem->type == AVAILABLE_SOURCE){
				switch (pDispInfo->item.iSubItem){
					case 0:
						if (((CUpDownClient*)pItem->value)->GetUserName() != NULL && pDispInfo->item.cchTextMax > 0){
							_tcsncpy(pDispInfo->item.pszText, ((CUpDownClient*)pItem->value)->GetUserName(), pDispInfo->item.cchTextMax);
							pDispInfo->item.pszText[pDispInfo->item.cchTextMax-1] = _T('\0');
						}
						break;
					default:
						// shouldn't happen
						pDispInfo->item.pszText[0] = _T('\0');
						break;
				}
			}
			else
				ASSERT(0);
        }
    }
    *pResult = 0;
}

void CDownloadListCtrl::OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	if (pGetInfoTip->iSubItem == 0)
	{
		LVHITTESTINFO hti = {0};
		::GetCursorPos(&hti.pt);
		ScreenToClient(&hti.pt);
		if (SubItemHitTest(&hti) == -1 || hti.iItem != pGetInfoTip->iItem || hti.iSubItem != 0){
			// don' show the default label tip for the main item, if the mouse is not over the main item
			if ((pGetInfoTip->dwFlags & LVGIT_UNFOLDED) == 0 && pGetInfoTip->cchTextMax > 0 && pGetInfoTip->pszText[0] != '\0')
				pGetInfoTip->pszText[0] = '\0';
			return;
		}

		CtrlItem_Struct* content = (CtrlItem_Struct*)GetItemData(pGetInfoTip->iItem);
		if (content && pGetInfoTip->pszText && pGetInfoTip->cchTextMax > 0)
		{
			CString info;

			// build info text and display it
			if (content->type == 1) // for downloading files
			{
				CPartFile* partfile = (CPartFile*)content->value;
				info=partfile->GetInfoSummary(partfile);
			}
			else if (content->type == 3 || content->type == 2) // for sources
			{
				CUpDownClient* client = (CUpDownClient*)content->value;
				if (client->IsEd2kClient())
				{
					in_addr server;
					server.S_un.S_addr = client->GetServerIP();

					info.Format(GetResString(IDS_NICKNAME) + _T(" %s\n")
								+ GetResString(IDS_SERVER) + _T(": %s:%d\n")
                                + GetResString(IDS_NEXT_REASK) + _T(": %s\n") // ZZ:DownloadManager
								+ GetResString(IDS_SOURCEINFO),
								client->GetUserName(),
								ipstr(server), client->GetServerPort(),
                                CastSecondsToHM(client->GetTimeUntilReask(content->owner)/1000), // ZZ:DownloadManager
								client->GetAskedCountDown(), client->GetAvailablePartCount());

					if (content->type == 2)
					{
						info += GetResString(IDS_CLIENTSOURCENAME) + client->GetClientFilename();

						if (!client->GetFileComment().IsEmpty())
							info += _T("\n") + GetResString(IDS_CMT_READ) + _T(" ") + client->GetFileComment();
						else
							info += _T("\n") + GetResString(IDS_CMT_NONE);
						info += _T("\n") + GetRateString(client->GetFileRate());
					}
					else
					{	// client asked twice
						info += GetResString(IDS_ASKEDFAF);
                        if (client->reqfile && client->reqfile->GetFileName()){
                            info.AppendFormat(_T(": %s"),client->reqfile->GetFileName());
                        }
					}

// ZZ:DownloadManager -->
				try { 
                        if (thePrefs.IsExtControlsEnabled() && !client->m_OtherRequests_list.IsEmpty()){
                            CString a4afStr;
                            a4afStr.AppendFormat(_T("\n\n") + GetResString(IDS_A4AF_FILES) + _T(":\n"));
                            bool first = TRUE;
                            for (POSITION pos3 = client->m_OtherRequests_list.GetHeadPosition(); pos3!=NULL; client->m_OtherRequests_list.GetNext(pos3)){
                                if(!first) {
                                    a4afStr.Append(_T("\n"));
                                }
                                first = FALSE;
                                a4afStr.AppendFormat(_T("%s"),client->m_OtherRequests_list.GetAt(pos3)->GetFileName());
                            };
                            info += a4afStr;
						} 
                    }catch (...){
                        ASSERT(false);
                    };
// ZZ:DownloadManager <--
				}
				else
				{
					info.Format(_T("URL: %s\nAvailable parts: %u"), client->GetUserName(), client->GetAvailablePartCount());
				} 
			}

			_tcsncpy(pGetInfoTip->pszText, info, pGetInfoTip->cchTextMax);
			pGetInfoTip->pszText[pGetInfoTip->cchTextMax-1] = _T('\0');
		}
	}
	*pResult = 0;
}

void CDownloadListCtrl::ShowFileDialog(CPartFile* pFile, bool bOpenCommentsPage)
{
	CSimpleArray<CPartFile*> aFiles;
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos != NULL)
	{
		int iItem = GetNextSelectedItem(pos);
		if (iItem != -1)
		{
			const CtrlItem_Struct* pCtrlItem = (CtrlItem_Struct*)GetItemData(iItem);
			if (pCtrlItem->type == FILE_TYPE)
				aFiles.Add((CPartFile*)pCtrlItem->value);
		}
	}

	if (aFiles.GetSize() > 0)
	{
		CFileDetailDialog dialog(&aFiles, bOpenCommentsPage);
		dialog.DoModal();
	}
}

void CDownloadListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		// Ctrl+C: Copy listview items to clipboard
		SendMessage(WM_COMMAND, MP_GETED2KLINK);
		return;
	}
	CMuleListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}
