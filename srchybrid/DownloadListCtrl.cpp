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

// DownloadListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "DownloadListCtrl.h"
#include "otherfunctions.h" 
#include "updownclient.h"
#include "opcodes.h"
#include "ClientDetailDialog.h"
#include "FileDetailDialog.h"
#include "commentdialoglst.h"
#include "MetaDataDlg.h"
#include "InputBox.h"
#include "KademliaWnd.h"
#include "version.h" //MORPH - Added by SiRoB
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
		tooltip->SetDelayTime(TTDT_INITIAL, theApp.glob_prefs->GetToolTipDelay()*1000);
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
	//InsertColumn(9,GetResString(IDS_DL_REMAINS),LVCFMT_LEFT, 110);
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
	InsertColumn(14, GetResString(IDS_CAT_GROUP), LVCFMT_LEFT, 60);
	// khaos::categorymod-
	// khaos::accuratetimerem+
	InsertColumn(15, GetResString(IDS_REMAININGSIZE), LVCFMT_LEFT, 80);
	// khaos::accuratetimerem-

	Localize();
	LoadSettings(CPreferences::tableDownload);
	curTab=0;
	// Barry - Use preferred sort order from preferences
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableDownload);
	bool sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableDownload);
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, 0xFFFF);	// SLUGFILLER: DLsortFix - uses multi-sort for fall-back
	// SLUGFILLER: multiSort - load multiple params
	for (int i = theApp.glob_prefs->GetColumnSortCount(CPreferences::tableDownload); i > 0; ) {
		i--;
		sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableDownload, i);
		sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableDownload, i);
		SortItems(SortProc, sortItem + (sortAscending ? 0:100));	// SLUGFILLER: DLsortFix
	}
	// SLUGFILLER: multiSort
}

void CDownloadListCtrl::Localize()
{
	m_ImageList.DeleteImageList();
	m_ImageList.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	m_ImageList.SetBkColor(CLR_NONE);
	m_ImageList.Add(CTempIconLoader("SrcDownloading"));
	m_ImageList.Add(CTempIconLoader("SrcOnQueue"));
	m_ImageList.Add(CTempIconLoader("SrcConnecting"));
	m_ImageList.Add(CTempIconLoader("SrcNNPQF"));
	m_ImageList.Add(CTempIconLoader("SrcUnknown"));
	m_ImageList.Add(CTempIconLoader("ClientCompatible"));
	m_ImageList.Add(CTempIconLoader("Friend"));
	m_ImageList.Add(CTempIconLoader("ClientEDonkey"));
	m_ImageList.Add(CTempIconLoader("ClientMLDonkey"));
	m_ImageList.Add(CTempIconLoader("RatingReceived"));
	m_ImageList.Add(CTempIconLoader("BadRatingReceived"));
	m_ImageList.Add(CTempIconLoader("ClientEDonkeyHybrid"));
	m_ImageList.Add(CTempIconLoader("ClientShareaza"));
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader("ClientSecureOvl")), 1);

	//MORPH START - Added by SiRoB, More client & Credit Overlay Icon
	m_ImageList.Add(CTempIconLoader("ClientRightEdonkey")); //14
	m_ImageList.Add(CTempIconLoader("ClientMorph")); //15
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader("ClientCreditOvl")), 2); //16
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader("ClientCreditSecureOvl")), 3); //17
	//MORPH END   - Added by SiRoB, More client & Credit Overlay Icon
	//MORPH START - Added by IceCream, eMule Plus rating icones
	m_ImageList.Add(CTempIconLoader("RATING_NO"));  // 18
	m_ImageList.Add(CTempIconLoader("RATING_EXCELLENT"));  // 19
	m_ImageList.Add(CTempIconLoader("RATING_FAIR"));  // 20
	m_ImageList.Add(CTempIconLoader("RATING_GOOD"));  // 21
	m_ImageList.Add(CTempIconLoader("RATING_POOR"));  // 22
	m_ImageList.Add(CTempIconLoader("RATING_FAKE"));  // 23
	//MORPH END   - Added by IceCream, eMule Plus rating icones
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

	strRes = GetResString(IDS_CAT_GROUP);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(14, &hdi);
	strRes.ReleaseBuffer();
	// khaos::categorymod-

	// khaos::accuratetimerem+
	strRes = GetResString(IDS_REMAININGSIZE);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(15, &hdi);
	strRes.ReleaseBuffer();
	// khaos::accuratetimerem-

	CreateMenues();

	ShowFilesCount();
}

void CDownloadListCtrl::AddFile(CPartFile* toadd){
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

		if (CheckShowItemInGivenCat(toadd, curTab ))
		InsertItem(LVIF_PARAM|LVIF_TEXT,itemnr,LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)newitem);

		ShowFilesCount();
}

void CDownloadListCtrl::AddSource(CPartFile* owner,CUpDownClient* source,bool notavailable){
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


void CDownloadListCtrl::RemoveSource(CUpDownClient* source,CPartFile* owner){
	if (!theApp.emuledlg->IsRunning()) return;
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

bool CDownloadListCtrl::RemoveFile(CPartFile* toremove){
	bool bResult = false;
	if (!theApp.emuledlg->IsRunning())
		return bResult;
	// Retrieve all entries matching the File or linked to the file
	// Remark: The 'asked another files' clients must be removed from here
	ASSERT(toremove != NULL);
	for(ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); ){
		CtrlItem_Struct* delItem = it->second;
		if(delItem->owner == toremove || delItem->value == toremove){
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

void CDownloadListCtrl::UpdateItem(void* toupdate){
	if (theApp.emuledlg->IsRunning()) {

		// Retrieve all entries matching the source
		std::pair<ListItems::const_iterator, ListItems::const_iterator> rangeIt = m_ListItems.equal_range(toupdate);
		for(ListItems::const_iterator it = rangeIt.first; it != rangeIt.second; it++){
			CtrlItem_Struct* updateItem  = it->second;

			// Find entry in CListCtrl and update object
 			LVFINDINFO find;
			find.flags = LVFI_PARAM;
			find.lParam = (LPARAM)updateItem;
			sint16 result = FindItem(&find);
			if(result != (-1)){
				updateItem->dwUpdated = 0;
				Update(result);
			}
		}
	}
}

void CDownloadListCtrl::DrawFileItem(CDC *dc, int nColumn, LPRECT lpRect, CtrlItem_Struct *lpCtrlItem) {
	if(lpRect->left < lpRect->right) {
		CString buffer;
		CPartFile *lpPartFile = (CPartFile*)lpCtrlItem->value;

		//MORPH START - Added by IceCream, show download in red
		if(theApp.glob_prefs->GetEnableDownloadInRed())
			if((lpPartFile->GetTransferingSrcCount()) && (nColumn))
				dc->SetTextColor(RGB(192,0,0));
		//MORPH END   - Added by IceCream, show download in red

		switch(nColumn) {
		
		case 0:{		// file name
			dc->SetTextColor( (theApp.glob_prefs->GetCatColor(lpPartFile->GetCategory())>0)?theApp.glob_prefs->GetCatColor(lpPartFile->GetCategory()):(::GetSysColor(COLOR_WINDOWTEXT)) );
			//MORPH START - Added by IceCream, eMule Plus rating icons
			int iImage = theApp.GetFileTypeSystemImageIdx(lpPartFile->GetFileName());
			if (theApp.GetSystemImageList() != NULL)
				::ImageList_Draw(theApp.GetSystemImageList(), iImage, dc->GetSafeHdc(), lpRect->left, lpRect->top, /*ILD_NORMAL*/ ILD_TRANSPARENT ); //Modified by IceCream, icons look better
			lpRect->left += theApp.GetSmallSytemIconSize().cx + 3;
			if ( theApp.glob_prefs->ShowRatingIndicator() && ( lpPartFile->HasComment() || lpPartFile->HasRating() )) //Modified by IceCream, eMule plus rating icon
			{
				POINT point= {lpRect->left-4,lpRect->top+2};
				int image=16;
				switch(lpPartFile->GetRating())
				{
					case 0: 
						image=18; 
						break; 
					case 1: 
						image=23;
						break; 
					case 2: 
						image=22; 
						break; 
					case 3: 
						image=21; 
						break; 
					case 4: 
						image=20; 
						break; 
					case 5: 
						image=19; 
						break;
					default:
						image=18;
				}
				m_ImageList.Draw(dc, image, point, ILD_NORMAL);
			}
			lpRect->left += 9;
			dc->DrawText(lpPartFile->GetFileName(), (int)strlen(lpPartFile->GetFileName()),lpRect, DLC_DT_TEXT);
			lpRect->left -= (9 + theApp.GetSmallSytemIconSize().cx + 3);
			break;}
			/*	m_ImageList.Draw(dc,
					(lpPartFile->HasRating() && lpPartFile->HasBadRating()) ?10:9,
					point,
					ILD_NORMAL);

				lpRect->left+=9;
				dc->DrawText(lpPartFile->GetFileName(), (int)strlen(lpPartFile->GetFileName()),lpRect, DLC_DT_TEXT);
				lpRect->left-=9;
			} else {
				lpRect->left+=9; //MORPH - Added by IceCream, improve download list view without comment
				dc->DrawText(lpPartFile->GetFileName(), (int)strlen(lpPartFile->GetFileName()),lpRect, DLC_DT_TEXT);
				lpRect->left+=9; //MORPH - Added by IceCream, improve download list view without comment
			}
			}
			break;*/
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
				buffer.Format("%.1f %s", lpPartFile->GetDatarate() / 1024.0f,GetResString(IDS_KBYTESEC));
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
					lpPartFile->DrawStatusBar(&cdcStatus,  &rec_status, theApp.glob_prefs->UseFlatBar()); 
					
					lpCtrlItem->dwUpdated = dwTicks + (rand() % 128); 
				} else 
					hOldBitmap = cdcStatus.SelectObject(lpCtrlItem->status); 

				dc->BitBlt(lpRect->left, lpRect->top, iWidth, iHeight,  &cdcStatus, 0, 0, SRCCOPY); 
				cdcStatus.SelectObject(hOldBitmap);
				//added end

				if (theApp.glob_prefs->GetUseDwlPercentage()) {
					// HoaX_69: BEGIN Display percent in progress bar
					COLORREF oldclr = dc->SetTextColor(RGB(255,255,255));
					int iOMode = dc->SetBkMode(TRANSPARENT);
					buffer.Format("%.1f%%", lpPartFile->GetPercentCompleted());
					   
					int iOLeft = lpRect->left;
					lpRect->left += iWidth / 2 - 10; // Close enough 
					dc->DrawText(buffer, buffer.GetLength(), lpRect, DLC_DT_TEXT);
					   
					lpRect->left = iOLeft;
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
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			}
			break;

		case 7:		// prio
			switch(lpPartFile->GetDownPriority()) {
			case 0:
				if( lpPartFile->IsAutoDownPriority() )
					dc->DrawText(GetResString(IDS_PRIOAUTOLOW),GetResString(IDS_PRIOAUTOLOW).GetLength(),lpRect, DLC_DT_TEXT);
				else
					dc->DrawText(GetResString(IDS_PRIOLOW),GetResString(IDS_PRIOLOW).GetLength(),lpRect, DLC_DT_TEXT);
				break;
			case 1:
				if( lpPartFile->IsAutoDownPriority() )
					dc->DrawText(GetResString(IDS_PRIOAUTONORMAL),GetResString(IDS_PRIOAUTONORMAL).GetLength(),lpRect, DLC_DT_TEXT);
				else
					dc->DrawText(GetResString(IDS_PRIONORMAL),GetResString(IDS_PRIONORMAL).GetLength(),lpRect, DLC_DT_TEXT);
				break;
			case 2:
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
					switch (theApp.glob_prefs->GetTimeRemainingMode()) {
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
					if (lpPartFile->m_nCompleteSourcesCountHi == 0)
					{
						tempbuffer= "?";
					}
					else
					{
						tempbuffer.Format("< %u", lpPartFile->m_nCompleteSourcesCountHi);
					}
				}
				else if (lpPartFile->m_nCompleteSourcesCountLo == lpPartFile->m_nCompleteSourcesCountHi)
				{
					tempbuffer.Format("%u", lpPartFile->m_nCompleteSourcesCountLo);
				}
				else
				{
					tempbuffer.Format("%u - %u", lpPartFile->m_nCompleteSourcesCountLo, lpPartFile->m_nCompleteSourcesCountHi);
				}
				if (lpPartFile->lastseencomplete==NULL)
					buffer.Format("%s(%s)",GetResString(IDS_UNKNOWN),tempbuffer);
				else
					buffer.Format("%s(%s)",lpPartFile->lastseencomplete.Format( theApp.glob_prefs->GetDateTimeFormat()),tempbuffer);
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			}
			break;
		case 11: // last receive
			if (!IsColumnHidden(11)) {
				if(lpPartFile->GetFileDate()!=NULL)
					buffer=lpPartFile->GetCFileDate().Format( theApp.glob_prefs->GetDateTimeFormat());
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			}
			break;
		// khaos::categorymod+
		case 12: // Category
			{
				if (!theApp.glob_prefs->ShowCatNameInDownList())
					buffer.Format("%u", lpPartFile->GetCategory());
				else
					buffer.Format("%s", theApp.glob_prefs->GetCategory(lpPartFile->GetCategory())->title);
				dc->DrawText(buffer, (int) strlen(buffer), lpRect, DLC_DT_TEXT);
				break;
			}
		case 13: // Resume Mod
			{
				buffer.Format("%u", lpPartFile->GetCatResumeOrder());
				dc->DrawText(buffer, (int) strlen(buffer), lpRect, DLC_DT_TEXT);
				break;
			}
		case 14: // Group
			{
				buffer.Format("%u", lpPartFile->GetFileGroup());
				dc->DrawText(buffer, (int) strlen(buffer), lpRect, DLC_DT_TEXT);
				break;
			}
		// khaos::categorymod-
		// khaos::accuratetimerem+
		case 15:		// remaining size
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
	}
}

void CDownloadListCtrl::DrawSourceItem(CDC *dc, int nColumn, LPRECT lpRect, CtrlItem_Struct *lpCtrlItem) {
	//MORPH START - Added by SiRoB, Don't draw hidden colums
	CRect Rect;
	this->GetClientRect(Rect);
	if (!((Rect.left<=lpRect->right && lpRect->right<=Rect.right)
		||(Rect.left<=lpRect->left && lpRect->left<=Rect.right)
		||(Rect.left>=lpRect->left && lpRect->right>=Rect.right)))
		return;
	//MORPH END   - Added by SiRoB, Don't draw hidden colums
	if(lpRect->left < lpRect->right) { 

		CString buffer;
		CUpDownClient *lpUpDownClient = (CUpDownClient*)lpCtrlItem->value;
		switch(nColumn) {

		case 0:		// icon, name, status
			{
				RECT cur_rec;
				memcpy(&cur_rec, lpRect, sizeof(RECT));
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
				if (lpUpDownClient->IsFriend())
					m_ImageList.Draw(dc, 6, point2, ILD_NORMAL | uOvlImg);
				else{
					if( lpUpDownClient->GetClientSoft() == SO_MLDONKEY )
						m_ImageList.Draw(dc, 8, point2, ILD_NORMAL | uOvlImg);
					else if ( lpUpDownClient->GetClientSoft() == SO_EDONKEY )
						m_ImageList.Draw(dc, 14, point2, ILD_NORMAL | uOvlImg);
					else if ( lpUpDownClient->GetClientSoft() == SO_EDONKEYHYBRID )
						m_ImageList.Draw(dc, 11, point2, ILD_NORMAL | uOvlImg);
					else if ( lpUpDownClient->GetClientSoft() == SO_SHAREAZA )
						m_ImageList.Draw(dc, 12, point2, ILD_NORMAL | uOvlImg);
					else if (lpUpDownClient->ExtProtocolAvailable())
						m_ImageList.Draw(dc, (lpUpDownClient->IsMorph())?15:5, point2, ILD_NORMAL | uOvlImg);
					else
						m_ImageList.Draw(dc, 7, point2, ILD_NORMAL | uOvlImg);
				}
				cur_rec.left += 20;
				//MORPH END - Modified by SiRoB, More client & ownCredits overlay icon
				if (!lpUpDownClient->GetUserName()) {
					buffer = "?";
				}
				else {
					//MORPH START - Added by IceCream, [sivka: -A4AF counter, ahead of user nickname-]
					//buffer = lpUpDownClient->GetUserName();
					buffer.Format("(%i) %s",lpUpDownClient->m_OtherRequests_list.GetCount()+1,lpUpDownClient->GetUserName());
					//MORPH END   - Added by IceCream, [sivka: -A4AF counter, ahead of user nickname-]
				}
				dc->DrawText(buffer,buffer.GetLength(),&cur_rec, DLC_DT_TEXT);
			}
			break;

		case 1:		// size
			switch(lpUpDownClient->GetSourceFrom()){
				case 0:
					buffer = "ED2K Server/Queue";
					break;
				case 1:
					buffer = "Kademlia";
					break;
				case 2:
					buffer = "Source Exchange";
					break;
				//MORPH START - Added by SiRoB, Source Loader Saver [SLS]
				case 3:
					buffer = "SLS";
					break;
				//MORPH END   - Added by SiRoB, Source Loader Saver [SLS]
				default:
					buffer = "Error";
			}
			dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			break;

		case 2:// transfered
			if ( !IsColumnHidden(3)) {
				dc->DrawText("",0,lpRect, DLC_DT_TEXT);
				break;
			}
		case 3:// completed
			if (lpCtrlItem->type == AVAILABLE_SOURCE && lpUpDownClient->GetTransferedDown()) {
				buffer = CastItoXBytes(lpUpDownClient->GetTransferedDown());
				dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT | DT_RIGHT);
			}
			break;

		case 4:		// speed
			if (lpCtrlItem->type == AVAILABLE_SOURCE && lpUpDownClient->GetDownloadDatarate()){
				if (lpUpDownClient->GetDownloadDatarate())
					buffer.Format("%.1f %s", lpUpDownClient->GetDownloadDatarate()/1024.0f,GetResString(IDS_KBYTESEC));
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
				if((lpCtrlItem->dwUpdated + 2*lpCtrlItem->owner->GetSourceCount()+DLC_BARUPDATE) < dwTicks || cx !=  iWidth  || !lpCtrlItem->dwUpdated) { //MORPH - Changed by SiRoB, Reduce PartStatus CPU consomption
					lpCtrlItem->status.DeleteObject(); 
					lpCtrlItem->status.CreateCompatibleBitmap(dc,  iWidth, iHeight); 
					lpCtrlItem->status.SetBitmapDimension(iWidth,  iHeight); 
					hOldBitmap = cdcStatus.SelectObject(lpCtrlItem->status); 

					RECT rec_status; 
					rec_status.left = 0; 
					rec_status.top = 0; 
					rec_status.bottom = iHeight; 
					rec_status.right = iWidth; 
					lpUpDownClient->DrawStatusBar(&cdcStatus,  &rec_status,(lpCtrlItem->type == UNAVAILABLE_SOURCE), theApp.glob_prefs->UseFlatBar()); 

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
			//MORPH - Changed by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
			dc->DrawText(lpUpDownClient->GetClientVerString(), lpRect, DLC_DT_TEXT);
			break;
			//MORPH - Changed by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
		}

		case 7:		// prio
			if (lpCtrlItem->type == AVAILABLE_SOURCE){
				if (lpUpDownClient->GetDownloadState()==DS_ONQUEUE){
					if( lpUpDownClient->IsRemoteQueueFull() ){
						buffer = GetResString(IDS_QUEUEFULL);
						dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
					}
					else{
						if ( lpUpDownClient->GetRemoteQueueRank()){
							buffer.Format("QR: %u",lpUpDownClient->GetRemoteQueueRank());
							dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
						}
						else{
							dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
						}
					}
				} else {
					dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
				}
			}
			break;

		case 8:	{	// status
			if (lpCtrlItem->type == AVAILABLE_SOURCE){
				switch (lpUpDownClient->GetDownloadState()) {
					case DS_CONNECTING:
						buffer = GetResString(IDS_CONNECTING);
						break;
					case DS_CONNECTED:
						buffer = GetResString(IDS_ASKING);
						break;
					case DS_WAITCALLBACK:
						buffer = GetResString(IDS_CONNVIASERVER);
						break;
					case DS_ONQUEUE:
						if( lpUpDownClient->IsRemoteQueueFull() )
							buffer = GetResString(IDS_QUEUEFULL);
						else
							buffer = GetResString(IDS_ONQUEUE);
						break;
					case DS_DOWNLOADING:
						buffer = GetResString(IDS_TRANSFERRING);
						break;
					case DS_REQHASHSET:
						buffer = GetResString(IDS_RECHASHSET);
						break;
					case DS_NONEEDEDPARTS:
						buffer = GetResString(IDS_NONEEDEDPARTS);
						break;
					case DS_LOWTOLOWIP:
						buffer = GetResString(IDS_NOCONNECTLOW2LOW);
						break;
					case DS_TOOMANYCONNS:
						buffer = GetResString(IDS_TOOMANYCONNS);
						break;
					default:
						buffer = GetResString(IDS_UNKNOWN);
				}
			}
			else {
				//MORPH START - Added by IceCream, [sivka: -show A4AF-filenames in status column]
				//buffer = GetResString(IDS_ASKED4ANOTHERFILE);
				buffer.Format("A4AF(%s)", lpUpDownClient->reqfile->GetFileName());
				//MORPH END   - Added by IceCReam, [sivka: -show A4AF-filenames in status column]
			}
			dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			break;
				}
		case 9:		// remaining time & size
			// START enkeyDev(th1) -EDT-
			if (lpUpDownClient->GetDownloadTimeVersion() &&
					(lpCtrlItem->type != UNAVAILABLE_SOURCE) &&
					(lpUpDownClient->GetDownloadState() != DS_DOWNLOADING))
				buffer = theApp.m_edt.FormatEDT(lpUpDownClient);
			else
				buffer.Empty();
			dc->DrawText(buffer,buffer.GetLength(),lpRect, DLC_DT_TEXT);
			// END enkeyDev(th1) -EDT-
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

	} /*else if (content->type != FILE_TYPE) {
		COLORREF crHighlight = m_crWindowText;
		COLORREF crWindow = m_crWindow;
		COLORREF crHalf = DLC_RGBBLEND(crHighlight, crWindow, 16);
		odc->SetBkColor(crHalf);
	}*/ else
		odc->SetBkColor(GetBkColor());

	CMemDC dc(odc,&CRect(lpDrawItemStruct->rcItem));
	CFont *pOldFont = dc->SelectObject(GetFont());
	COLORREF crOldTextColor = dc->SetTextColor(m_crWindowText);

	BOOL notLast = lpDrawItemStruct->itemID + 1 != GetItemCount();
	BOOL notFirst = lpDrawItemStruct->itemID != 0;
	int tree_start=0;
	int tree_end=0;

	RECT cur_rec;
	memcpy(&cur_rec,&lpDrawItemStruct->rcItem,sizeof(RECT));

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
					int iNext = pHeaderCtrl->OrderToIndex(iCurrent + 1);
				cur_rec.right += cx;
				DrawSourceItem(dc, iColumn, &cur_rec, content);
				cur_rec.left += cx;
			}
		}
	}

	//draw rectangle around selected item(s)
	if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
		(lpDrawItemStruct->itemState & ODS_SELECTED) &&
		(content->type == FILE_TYPE)) {
			RECT outline_rec;
			memcpy(&outline_rec,&lpDrawItemStruct->rcItem,sizeof(RECT));

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
	else if (((lpDrawItemStruct->itemState & ODS_FOCUS) == ODS_FOCUS) &&
		(GetFocus() == this)) {

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
	dc->SelectObject(pOldFont);
	dc->SetTextColor(crOldTextColor);
}

// modifier-keys -view filtering [Ese Juani+xrmb]
void CDownloadListCtrl::HideSources(CPartFile* toCollapse,bool isShift,bool isCtrl,bool isAlt) {
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
	ON_NOTIFY_REFLECT(LVN_ITEMACTIVATE, OnItemActivate)
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnListModified)
	ON_NOTIFY_REFLECT(LVN_INSERTITEM, OnListModified)
	ON_NOTIFY_REFLECT(LVN_DELETEITEM, OnListModified)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclkDownloadlist)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
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
		char* buffer = new char[MAX_PATH];
		sprintf(buffer,"%s",partfile->GetFullName());
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

	if (theApp.glob_prefs->IsDoubleClickEnabled() || pNMIA->iSubItem > 0)
		ExpandCollapseItem(pNMIA->iItem,2);
	*pResult = 0;
}

void CDownloadListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (GetSelectionMark() != (-1)){
		CtrlItem_Struct* content = (CtrlItem_Struct*)this->GetItemData(GetSelectionMark());
		if (content->type == FILE_TYPE){
			CPartFile* file = (CPartFile*)content->value;
			bool filedone=(file->GetStatus()==PS_COMPLETE || file->GetStatus()==PS_COMPLETING);

			// khaos::kmod+ Popup menu should be disabled when advanced A4AF mode is turned off and we need to check appropriate A4AF items.
			m_FileMenu.CheckMenuItem(MP_FORCEA4AF, (theApp.downloadqueue->forcea4af_file == file && GetSelectedCount() == 1) ? MF_CHECKED : MF_UNCHECKED);
			m_FileMenu.EnableMenuItem(MP_FORCEA4AF, ((GetSelectedCount() == 1) ? MF_ENABLED : MF_GRAYED));
			m_FileMenu.EnableMenuItem(2, (theApp.glob_prefs->AdvancedA4AFMode() || theApp.glob_prefs->UseSmartA4AFSwapping() ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);
			m_A4AFMenu.ModifyMenu(MP_FORCEA4AFONFLAG, (file->ForceAllA4AF() && GetSelectedCount() == 1 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, MP_FORCEA4AFONFLAG, ((GetSelectedCount() > 1) ? GetResString(IDS_INVERT) + " " : "") + GetResString(IDS_A4AF_ONFLAG));
			m_A4AFMenu.ModifyMenu(MP_FORCEA4AFOFFFLAG, (file->ForceA4AFOff() && GetSelectedCount() == 1 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, MP_FORCEA4AFOFFFLAG, ((GetSelectedCount() > 1) ? GetResString(IDS_INVERT) + " " : "") + GetResString(IDS_A4AF_OFFFLAG));
			// khaos::kmod-

			m_FileMenu.EnableMenuItem(MP_PAUSE,((file->GetStatus() != PS_PAUSED && file->GetStatus() != PS_ERROR && !filedone) ? MF_ENABLED:MF_GRAYED));
			m_FileMenu.EnableMenuItem(MP_STOP,((!file->IsStopped() && file->GetStatus() != PS_ERROR && !filedone ) ? MF_ENABLED:MF_GRAYED));
			m_FileMenu.EnableMenuItem(MP_RESUME,((file->GetStatus() == PS_PAUSED) ? MF_ENABLED:MF_GRAYED));
			//EastShare Start - Only download complete files v2.1 by AndCycle
			m_FileMenu.EnableMenuItem(MP_FORCE,((file->lastseencomplete == NULL && file->GetStatus() != PS_ERROR && !filedone) ? MF_ENABLED:MF_GRAYED));//shadow#(onlydownloadcompletefiles)
			//EastShare End - Only download complete files v2.1 by AndCycle
	        m_FileMenu.EnableMenuItem(MP_OPEN,((file->GetStatus() == PS_COMPLETE) ? MF_ENABLED:MF_GRAYED)); //<<--9/21/02
			m_FileMenu.EnableMenuItem(MP_PREVIEW,((file->PreviewAvailable()) ? MF_ENABLED:MF_GRAYED));

			m_FileMenu.EnableMenuItem(MP_CANCEL,((!filedone) ? MF_ENABLED:MF_GRAYED));

			m_FileMenu.EnableMenuItem((UINT_PTR)m_PrioMenu.m_hMenu,((!filedone) ? MF_ENABLED:MF_GRAYED));


			m_PrioMenu.CheckMenuItem(MP_PRIOAUTO, ((file->IsAutoDownPriority()) ? MF_CHECKED:MF_UNCHECKED));
			m_PrioMenu.CheckMenuItem(MP_PRIOHIGH, ((file->GetDownPriority() == PR_HIGH && !file->IsAutoDownPriority()) ? MF_CHECKED:MF_UNCHECKED));
			m_PrioMenu.CheckMenuItem(MP_PRIONORMAL, ((file->GetDownPriority() == PR_NORMAL && !file->IsAutoDownPriority()) ? MF_CHECKED:MF_UNCHECKED));
			m_PrioMenu.CheckMenuItem(MP_PRIOLOW, ((file->GetDownPriority() == PR_LOW && !file->IsAutoDownPriority()) ? MF_CHECKED:MF_UNCHECKED));

			int counter;
			m_Web.CreateMenu();
			UpdateURLMenu(m_Web,counter);
			UINT flag;
			flag=(counter==0) ? MF_GRAYED:MF_STRING;
			
			m_FileMenu.AppendMenu(flag|MF_POPUP,(UINT_PTR)m_Web.m_hMenu, GetResString(IDS_WEBSERVICES) );

			// khaos::categorymod+
			//MORPH START- Readded by SiRoB, Use Official ASSIGNCAT methode
			CTitleMenu cats;
			cats.CreatePopupMenu();
			cats.AddMenuTitle(GetResString(IDS_CAT));
			flag=(theApp.glob_prefs->GetCatCount()==1) ? MF_GRAYED:MF_STRING;
			if (theApp.glob_prefs->GetCatCount()>1) {
				for (int i=0;i<theApp.glob_prefs->GetCatCount();i++) {
					cats.AppendMenu(MF_STRING,MP_ASSIGNCAT+i, (i==0)?GetResString(IDS_CAT_UNASSIGN):theApp.glob_prefs->GetCategory(i)->title);
				}
			}
			m_FileMenu.AppendMenu(flag|MF_POPUP,(UINT_PTR)cats.m_hMenu, GetResString(IDS_TOCAT) );
			//// Assign Cat now uses the SelCatDlg.
			//flag=(theApp.glob_prefs->GetCatCount()==1) ? MF_GRAYED:MF_STRING;
			//m_FileMenu.AppendMenu(flag, MP_ASSIGNCAT, GetResString(IDS_TOCAT) );
            //MORPH END - Readded by SiRoB, Use Official ASSIGNCAT methode
			//m_FileMenu.AppendMenu(MF_STRING, MP_SETFILEGROUP, GetResString(IDS_CAT_SETFILEGROUP));
			
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

			m_FileMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);

			// khaos::categorymod+ Condensed this code, and we needed to remove another menu item.
			for (int i=0; i<3; i++) m_FileMenu.RemoveMenu(m_FileMenu.GetMenuItemCount()-1,MF_BYPOSITION);
			
			if (mnuOrder) VERIFY( mnuOrder.DestroyMenu() );
			// khaos::categorymod-

			VERIFY( m_Web.DestroyMenu() );
			// khaos::categorymod+ VERIFY( cats.DestroyMenu() ); //khaos::categorymod-
		}else {
			m_ClientMenu.CreatePopupMenu();
			m_ClientMenu.AddMenuTitle(GetResString(IDS_CLIENTS));
			m_ClientMenu.AppendMenu(MF_STRING,MP_DETAIL, GetResString(IDS_SHOWDETAILS));
			m_ClientMenu.AppendMenu(MF_STRING,MP_ADDFRIEND, GetResString(IDS_ADDFRIEND));
			m_ClientMenu.AppendMenu(MF_STRING,MP_MESSAGE, GetResString(IDS_SEND_MSG));
			m_ClientMenu.AppendMenu(MF_STRING,MP_SHOWLIST, GetResString(IDS_VIEWFILES));
			if(theApp.kademlia->GetThreadID() && !theApp.kademlia->isConnected() )
				m_ClientMenu.AppendMenu(MF_STRING,MP_BOOT, "BootStrap");

			//MORPH - Removed by SiRoB, Due to Khaos A4AF
			/*CMenu mc_A4AFMenu;
			mc_A4AFMenu.CreateMenu();
			if (theApp.glob_prefs->IsExtControlsEnabled()) {
				if (content->type == UNAVAILABLE_SOURCE)
					mc_A4AFMenu.AppendMenu(MF_STRING,MP_SWAP_A4AF_TO_THIS,GetResString(IDS_SWAP_A4AF_TO_THIS)); // Added by sivka [Ambdribant]
				if (content->type == AVAILABLE_SOURCE)
					mc_A4AFMenu.AppendMenu(MF_STRING,MP_SWAP_A4AF_TO_OTHER,GetResString(IDS_SWAP_A4AF_TO_OTHER)); // Added by sivka

				if (mc_A4AFMenu.GetMenuItemCount()>0) 
					m_ClientMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)mc_A4AFMenu.m_hMenu, GetResString(IDS_A4AF));				
			}*/
			//MORPH START - Added by Yun.SF3, List Requested Files
			m_ClientMenu.AppendMenu(MF_SEPARATOR);
			m_ClientMenu.AppendMenu(MF_STRING,MP_LIST_REQUESTED_FILES, _T(GetResString(IDS_LISTREQUESTED))); // Added by sivka
			//MORPH END - Added by Yun.SF3, List Requested Files
			m_ClientMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
			VERIFY( m_ClientMenu.DestroyMenu() );
		}
	}
	else{
		m_FileMenu.EnableMenuItem(MP_CANCEL,MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_PAUSE,MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_STOP,MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_RESUME,MF_GRAYED);
		//EastShare Start - Only download complete files v2.1 by AndCycle
		m_FileMenu.EnableMenuItem(MP_FORCE,MF_GRAYED);//shadow#(onlydownloadcompletefiles)
		//EastShare End - Only download complete files v2.1 by AndCycle
		m_FileMenu.EnableMenuItem(MP_PREVIEW,MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_OPEN,MF_GRAYED); //<<--9/21/02
		m_FileMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	}
}

BOOL CDownloadListCtrl::OnCommand(WPARAM wParam,LPARAM lParam ){
	if (GetSelectionMark() != (-1)){
		CtrlItem_Struct* content = (CtrlItem_Struct*)this->GetItemData(GetSelectionMark());
		
		///////////////////////////////////////////////////////////// 
		//for multiple selections 
		UINT selectedCount = this->GetSelectedCount(); 
		CTypedPtrList<CPtrList, CPartFile*> selectedList; 
		int index = -1; 
		POSITION pos = this->GetFirstSelectedItemPosition(); 
		while(pos != NULL) 
		{ 
			index = this->GetNextSelectedItem(pos); 
			if(index > -1) 
			{
				if (((CtrlItem_Struct*)this->GetItemData(index))->type == FILE_TYPE) 
					selectedList.AddTail((CPartFile*)((CtrlItem_Struct*)this->GetItemData(index))->value); 
			} 
		} 

		if (content->type == FILE_TYPE){
			CPartFile* file = (CPartFile*)content->value;
			
			if (wParam>=MP_WEBURL && wParam<=MP_WEBURL+99) {
				RunURL(file, theApp.webservices.GetAt(wParam-MP_WEBURL) );
			}
			//MOPRH - Readded by SiRoB, Use Official ASSIGNCAT methode
			// khaos::categorymod+
			if (wParam>=MP_ASSIGNCAT && wParam<=MP_ASSIGNCAT+99) {
				while(!selectedList.IsEmpty()) { 
					CPartFile *selected = selectedList.GetHead();
					selected->SetCategory(wParam-MP_ASSIGNCAT);
					selectedList.RemoveHead(); 
				}				
				ChangeCategory(curTab);
			}
			// khaos::categorymod-

			switch (wParam)
			{
				case MPG_DELETE:
				case MP_CANCEL:
				{
					//for multiple selections 
					if(selectedCount > 0)
					{
						SetRedraw(false);
						CString fileList="";
						bool validdelete = false;
						
						for (pos = selectedList.GetHeadPosition() ; pos != 0 ; selectedList.GetNext(pos))
						{
							if(selectedList.GetAt(pos)->GetStatus() != PS_COMPLETING && selectedList.GetAt(pos)->GetStatus() != PS_COMPLETE){
								validdelete = true;
								if (selectedCount<50) fileList.Append("\n"+CString(selectedList.GetAt(pos)->GetFileName())); 
						} 
						} 

						CString quest;
						if (selectedCount==1)
							quest=GetResString(IDS_Q_CANCELDL2);
						else
							quest=GetResString(IDS_Q_CANCELDL);
						if (validdelete && AfxMessageBox(quest + fileList,MB_ICONQUESTION|MB_YESNO) == IDYES)
						{ 
							while(!selectedList.IsEmpty())
							{
								HideSources(selectedList.GetHead());
								switch(selectedList.GetHead()->GetStatus()) { 
									case PS_WAITINGFORHASH: 
									case PS_HASHING: 
									case PS_COMPLETING: 
									case PS_COMPLETE: 
										selectedList.RemoveHead(); 
										break;
									case PS_PAUSED:
										selectedList.GetHead()->DeleteFile(); 
										selectedList.RemoveHead();
										break;
									default:
										if (theApp.glob_prefs->StartNextFile()) theApp.downloadqueue->StartNextFile();
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
					if(selectedCount > 1)
					{
						SetRedraw(false);
						while(!selectedList.IsEmpty()) { 
							selectedList.GetHead()->SetAutoDownPriority(false); 
							selectedList.GetHead()->SetDownPriority(PR_HIGH); 
							selectedList.RemoveHead(); 
						} 
						SetRedraw(true);
						break; 
					} 
					file->SetAutoDownPriority(false);
					file->SetDownPriority(PR_HIGH);
					break;
				case MP_PRIOLOW:
					if(selectedCount > 1)
					{
						SetRedraw(false);
						while(!selectedList.IsEmpty()) { 
							selectedList.GetHead()->SetAutoDownPriority(false); 
							selectedList.GetHead()->SetDownPriority(PR_LOW); 
							selectedList.RemoveHead(); 
						}
						SetRedraw(true);
						break; 
					} 
					file->SetAutoDownPriority(false);
					file->SetDownPriority(PR_LOW);
					break;
				case MP_PRIONORMAL:
					if(selectedCount > 1)
					{
						SetRedraw(false);
						while(!selectedList.IsEmpty()) {
							selectedList.GetHead()->SetAutoDownPriority(false);
							selectedList.GetHead()->SetDownPriority(PR_NORMAL);
							selectedList.RemoveHead();
						}
						SetRedraw(true);
						break;
					} 
					file->SetAutoDownPriority(false);
					file->SetDownPriority(PR_NORMAL);
					break;
				case MP_PRIOAUTO:
					if(selectedCount > 1)
					{
						SetRedraw(false);
						while(!selectedList.IsEmpty()) {
							selectedList.GetHead()->SetAutoDownPriority(true);
							selectedList.GetHead()->SetDownPriority(PR_HIGH);
							selectedList.RemoveHead();
						}
						SetRedraw(true);
						break;
					} 
					file->SetAutoDownPriority(true);
					file->SetDownPriority(PR_HIGH);
					break;
				case MP_PAUSE:
					if(selectedCount > 1)
					{
						SetRedraw(false);
						while(!selectedList.IsEmpty()) { 
							selectedList.GetHead()->PauseFile(); 
							selectedList.RemoveHead(); 
						} 
						SetRedraw(true);
						break; 
					} 
					file->PauseFile();
					break;
				case MP_RESUME:
					if(selectedCount > 1)
					{
						SetRedraw(false);
						while(!selectedList.IsEmpty()) { 
							selectedList.GetHead()->ResumeFile(); 
							selectedList.RemoveHead(); 
						}
						SetRedraw(true);
						break; 
					} 
					file->ResumeFile();
					break;
				//EastShare Start - Only download complete files v2.1 by AndCycle
				case MP_FORCE://shadow#(onlydownloadcompletefiles)-start
					if(selectedCount > 1)
					{
						SetRedraw(false);
						while(!selectedList.IsEmpty()) {
							selectedList.GetHead()->lastseencomplete = 1025992800;
							selectedList.GetHead()->SavePartFile();
							selectedList.RemoveHead();
						}
						SetRedraw(true);
						break;
					}
					file->lastseencomplete = 1025992800;
					file->SavePartFile();
					break;//shadow#(onlydownloadcompletefiles)-end
				//EastShare End - Only download complete files v2.1 by AndCycle
				case MP_STOP:
					if(selectedCount > 1)
					{
						SetRedraw(false);
						while(!selectedList.IsEmpty()) { 
							CPartFile *selected = selectedList.GetHead();
							HideSources(selected);
							selected->StopFile(); 
							selectedList.RemoveHead(); 
						}
						SetRedraw(true);
						break; 
					} 
					HideSources(file);
					file->StopFile();
					break;
				case MP_CLEARCOMPLETED:
					//SetRedraw(false);//EastShare - modified by AndCycle - AutoClearComplete (NoamSon)
					ClearCompleted();
					//SetRedraw(true);//EastShare - modified by AndCycle - AutoClearComplete (NoamSon)
					break;
				/*case MP_ALL_A4AF_TO_THIS:
				{
					SetRedraw(false);
					if (selectedCount == 1 
					&& (file->GetStatus(false) == PS_READY || file->GetStatus(false) == PS_EMPTY))
					{
						theApp.downloadqueue->DisableAllA4AFAuto();

						POSITION pos1, pos2;
						for (pos1 = file->A4AFsrclist.GetHeadPosition();(pos2=pos1)!=NULL;)
						{
							file->A4AFsrclist.GetNext(pos1);
							CUpDownClient *cur_source = file->A4AFsrclist.GetAt(pos2);
							if( cur_source->GetDownloadState() != DS_DOWNLOADING
								&& cur_source->reqfile 
								&& ( (!cur_source->reqfile->IsA4AFAuto()) || cur_source->GetDownloadState() == DS_NONEEDEDPARTS)
								&& !cur_source->IsSwapSuspended(file) )
							{
								CPartFile* oldfile = cur_source->reqfile;
								if (cur_source->SwapToAnotherFile(true, false, false, file)){
									cur_source->DontSwapTo(oldfile);
								}
							}
						}

					}
					SetRedraw(true);
					this->UpdateItem(file);						
					break;
				}
				case MP_ALL_A4AF_AUTO:
					file->SetA4AFAuto(!file->IsA4AFAuto());
					break;
				case MP_ALL_A4AF_TO_OTHER:
				{
					SetRedraw(false);

					if (selectedCount == 1 
					&& (file->GetStatus(false) == PS_READY || file->GetStatus(false) == PS_EMPTY))
					{
						theApp.downloadqueue->DisableAllA4AFAuto();
						
						for (int sl=0;sl<SOURCESSLOTS;sl++)
						{
							if (!file->srclists[sl].IsEmpty())
							{
								POSITION pos1, pos2;
								for(pos1 = file->srclists[sl].GetHeadPosition(); (pos2 = pos1) != NULL;)
								{
									file->srclists[sl].GetNext(pos1);
									file->srclists[sl].GetAt(pos2)->SwapToAnotherFile(false, false, false, NULL);
								}
							}
						}
					}
					SetRedraw(true);
					break;
				}*/
				case MPG_F2: {
						InputBox inputbox;
						CString title=GetResString(IDS_RENAME);
						title.Remove('&');
						inputbox.SetLabels(title,GetResString(IDS_DL_FILENAME),file->GetFileName());
						inputbox.SetEditFilenameMode();
						inputbox.DoModal();
						CString nn=inputbox.GetInput();
						if (!inputbox.WasCancelled() && nn.GetLength()>0){
							file->SetFileName(nn,true);
							file->SavePartFile(); 
						}

						break;
					}
				case MPG_ALTENTER:
				case MP_METINFO:
					{
						CFileDetailDialog dialog(file);
						dialog.DoModal();
						break;
					}
				case MP_GETED2KLINK:
					if(selectedCount > 1)
					{
						CString str;
						while(!selectedList.IsEmpty()) { 
							str += theApp.CreateED2kLink(selectedList.GetHead()) + "\n"; 
							selectedList.RemoveHead(); 
						}
						theApp.CopyTextToClipboard(str);
						//AfxMessageBox(GetResString(IDS_COPIED2CB) + str);
						break; 
					} 
					theApp.CopyTextToClipboard(theApp.CreateED2kLink(file));
					break;
				case MP_GETHTMLED2KLINK:
					if(selectedCount > 1)
					{
						CString str;
						while(!selectedList.IsEmpty()) { 
							str += theApp.CreateHTMLED2kLink(selectedList.GetHead()) + "\n"; 
							selectedList.RemoveHead(); 
						}
						theApp.CopyTextToClipboard(str);
						//AfxMessageBox(GetResString(IDS_COPIED2CB) + str);
						break; 
					} 
					theApp.CopyTextToClipboard(theApp.CreateHTMLED2kLink(file));
					break;
				case MP_OPEN:{
					if(selectedCount > 1)
						break;
					char* buffer = new char[MAX_PATH];
					_snprintf(buffer,MAX_PATH,"%s",file->GetFullName());
					ShellOpenFile(buffer, NULL);
					delete[] buffer;
					break; 
				}
				case MP_PREVIEW:{
					if(selectedCount > 1)
						break;
					file->PreviewFile();
					break;
				}
				case MP_VIEWFILECOMMENTS:{
					CFileDetailDialog dialog(file, true);
					dialog.DoModal(); 
					break;
				}
				//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
  				case MP_FAKEREPORT:{ 
					if(selectedCount > 1)
					{
						//CString str;
						while(!selectedList.IsEmpty()) { 
							//str += theApp.CreateED2kLink(selectedList.GetHead()) + "\n"; 
							selectedList.RemoveHead(); 
						}
						//theApp.CopyTextToClipboard(str);
						//AfxMessageBox(GetResString(IDS_COPIED2CB) + str);
						break; 
					} 
					//theApp.CopyTextToClipboard(theApp.CreateED2kLink(file));
					ShellExecute(NULL, NULL, "http://edonkeyfakes.ath.cx/report/index.php?link2="+theApp.CreateED2kLink(file), NULL, theApp.glob_prefs->GetAppDir(), SW_SHOWDEFAULT);
					break;
				}
 				case MP_CHECKFAKE:{ 
					if(selectedCount > 1)
					{
						//CString str;
						while(!selectedList.IsEmpty()) { 
							//str += theApp.CreateED2kLink(selectedList.GetHead()) + "\n"; 
							selectedList.RemoveHead(); 
						}
						//theApp.CopyTextToClipboard(str);
						//AfxMessageBox(GetResString(IDS_COPIED2CB) + str);
						break; 
					} 
					//theApp.CopyTextToClipboard(theApp.CreateED2kLink(file));
					ShellExecute(NULL, NULL, "http://edonkeyfakes.ath.cx/fakecheck/update/fakecheck.php?ed2k="+theApp.CreateED2kLink(file), NULL, theApp.glob_prefs->GetAppDir(), SW_SHOWDEFAULT);
					break;
				}
				//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating

				//MORPH START - Added by IceCream, copy feedback feature
 				case MP_COPYFEEDBACK:
				{
					CString feed;
					feed.AppendFormat(GetResString(IDS_FEEDBACK_FROM), theApp.glob_prefs->GetUserNick(), MOD_VERSION);
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
					feed.AppendFormat("Feedback from %s ",theApp.glob_prefs->GetUserNick());
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
				case MP_SETFILEGROUP: {
					InputBox inputGroup;
					CString currGroup;

					currGroup.Format("%u", file->GetFileGroup());
					inputGroup.SetLabels(GetResString(IDS_CAT_SETFILEGROUP), GetResString(IDS_CAT_SETFILEGROUPMSG), currGroup);
					inputGroup.SetNumber(true);
					if (inputGroup.DoModal() == IDOK)
					{
						int newGroup = inputGroup.GetInputInt();
						if (newGroup < 1 || newGroup == file->GetFileGroup()) break;

						file->SetFileGroup(newGroup);
						Invalidate();
					}
					break;
				}
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
				//MORPH - Removed by SiRoB, Use Official ASSIGNCAT methode
				//case MP_ASSIGNCAT: {
				//	// Changed this to use SelCatDlg

				//	CSelCategoryDlg* getCatDlg = new CSelCategoryDlg((CWnd*)theApp.emuledlg);

				//	if (getCatDlg->DoModal() == IDCANCEL)
				//		break;

				//	uint8 useCat = getCatDlg->GetInput();
				//	delete getCatDlg;

				//	while(!selectedList.IsEmpty()) { 
				//		selectedList.GetHead()->SetCategory(useCat);
				//		selectedList.RemoveHead(); 
				//	}
				//	break;
				//}
				// khaos::categorymod-
				// khaos::kmod+
				case MP_FORCEA4AF: {

					if (selectedCount > 1)
						break;
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
			}
			//ChangeCategory(curTab); Commented lines for bug of A4AF changing cats (a part of catmod from khaos, no longer remains here...)
		}
		else{
			CUpDownClient* client = (CUpDownClient*)content->value;
			CPartFile* file = (CPartFile*)content->owner; // added by sivka
			switch (wParam){
				case MP_SHOWLIST:
					client->RequestSharedFileList();
					break;
				case MP_MESSAGE:
					theApp.emuledlg->chatwnd.StartSession(client);
					break;
				case MP_ADDFRIEND:{
					theApp.friendlist->AddFriend(client);
					break;
				}
				case MPG_ALTENTER:
				case MP_DETAIL: {
					CClientDetailDialog dialog(client);
					dialog.DoModal();
					break;
				}
				case MP_BOOT:{
					if(	theApp.kademlia->GetThreadID() && client->GetKadPort())
					{
						theApp.kademlia->Bootstrap(ntohl(client->GetIP()), client->GetKadPort());
					}
					break;
				}
				//MORPH - Removed by SiRoB, Due to khaos A4AF
				/*case MP_SWAP_A4AF_TO_THIS: { // added by sivka [enkeyDEV(Ottavio84) -A4AF-]
					if(file->GetStatus(false) == PS_READY || file->GetStatus(false) == PS_EMPTY)
					{
						if(!client->GetDownloadState() == DS_DOWNLOADING)
						{
							client->SwapToAnotherFile(true, true, false, file);
							UpdateItem(file);
						}
					}
					break;
				}
				case MP_SWAP_A4AF_TO_OTHER:
					if ((client != NULL)  && !(client->GetDownloadState() == DS_DOWNLOADING))
						client->SwapToAnotherFile(true, true, false, NULL);
					break;
				*/
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
		// cleanup multiselection
		selectedList.RemoveAll(); 

	}
	else /*nothing selected*/
	{
		switch (wParam){
			case MP_CLEARCOMPLETED:
				ClearCompleted();
				break;
		}

	}

	return true;
}

void CDownloadListCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult){
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableDownload);
	int userSort = (GetAsyncKeyState(VK_CONTROL) < 0) ? 0x8000:0;	// SLUGFILLER: DLsortFix - Ctrl sorts sources only
	bool m_oldSortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableDownload);
	bool sortAscending = (sortItem != pNMListView->iSubItem + userSort) ? (pNMListView->iSubItem == 0) : !m_oldSortAscending;	// SLUGFILLER: DLsortFix - descending by default for all but filename/username
	// Item is column clicked
	sortItem = pNMListView->iSubItem + userSort;	// SLUGFILLER: DLsortFix
	// Save new preferences
	theApp.glob_prefs->SetColumnSortItem(CPreferences::tableDownload, sortItem);
	theApp.glob_prefs->SetColumnSortAscending(CPreferences::tableDownload, sortAscending);
	// Sort table
	if (sortItem < 0x8000)	// SLUGFILLER: DLsortFix - Don't set arrow for source-only sorting(TODO: Seperate arrow?)
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));

	*pResult = 0;
}

int CDownloadListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){
	CtrlItem_Struct* item1 = (CtrlItem_Struct*)lParam1;
	CtrlItem_Struct* item2 = (CtrlItem_Struct*)lParam2;

	int sortMod = 1;
	if((lParamSort != 0xFFFF) && (lParamSort & 0x7FFF) >= 100) {	// SLUGFILLER: DLsortFix
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
		CUpDownClient* client1 = (CUpDownClient*)item1->value;
		CUpDownClient* client2 = (CUpDownClient*)item2->value;
		//MORPH START - Added by IceCream, SLUGFILLER: DLsortFix - never compare sources of different files
		if (item1->parent->value != item2->parent->value)
			return sortMod * Compare((CPartFile*)(item1->parent->value), (CPartFile*)(item2->parent->value), lParamSort);
		//MORPH END   - Added by IceCream, SLUGFILLER: DLsortFix
		if (item1->type != item2->type)
			return item1->type - item2->type;

		comp = Compare(client1, client2, lParamSort,sortMod);
	}

	return sortMod * comp;
}


void CDownloadListCtrl::ClearCompleted(bool ignorecats){
	SetRedraw(false);//EastShare - added by AndCycle - AutoClearComplete (NoamSon)
	// Search for completed file(s)
	for(ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); ){
		CtrlItem_Struct* cur_item = it->second;
		it++; // Already point to the next iterator. 
		if(cur_item->type == FILE_TYPE){
			CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);
			if(file->IsPartFile() == false && (CheckShowItemInGivenCat(file,curTab) || ignorecats) ){
				if (RemoveFile(file))
					it = m_ListItems.begin();
			}
		}
	}
	SetRedraw(true);//EastShare - added by AndCycle - AutoClearComplete (NoamSon)
}

void CDownloadListCtrl::SetStyle() {
	if (theApp.glob_prefs->IsDoubleClickEnabled())
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

int CDownloadListCtrl::Compare(CPartFile* file1, CPartFile* file2, LPARAM lParamSort) {
	// SLUGFILLER: DLsortFix
	if ((lParamSort & 0x8000) && (lParamSort != 0xFFFF))
		return 0;
	// SLUGFILLER: DLsortFix
	switch(lParamSort){
	case 0: //filename asc
		return file1->GetFileName().CompareNoCase(file2->GetFileName());	// SLUGFILLER: DLsortFix - Using CStrings
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
  //MORPH - Added by Yun.SF3, ZZ Upload System
       {
		    int comp =  file2->getPartfileStatusRang()-file1->getPartfileStatusRang();	//MORPH - Added by IceCream SLUGFILLER: DLsortFix - Low StatusRang<->Better status<->High status

            // Second sort order on filename
            if(comp == 0) {
                comp = strcmpi(file1->GetFileName(),file2->GetFileName());
            }

            return comp;
        }
		//return file1->getPartfileStatusRang()-file2->getPartfileStatusRang(); 
 //MORPH - Added by Yun.SF3, ZZ Upload System
	case 9: //Remaining Time asc 
		if (file1->GetDatarate())
			if (file2->GetDatarate())
                return file1->getTimeRemaining()-file2->getTimeRemaining() ;
			else
				return -1;
		else
			if (file2->GetDatarate())
				return 1;
			else
                return 0;
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
	case 14: // File Group
		if (file1->GetFileGroup() > file2->GetFileGroup())
			return 1;
		else if (file1->GetFileGroup() < file2->GetFileGroup())
			return -1;
		else
			return 0;
	case 15: // Remaining Bytes
		if ((file1->GetFileSize() - file1->GetCompletedSize()) > (file2->GetFileSize() - file2->GetCompletedSize()))
			return 1;
		else if ((file1->GetFileSize() - file1->GetCompletedSize()) < (file2->GetFileSize() - file2->GetCompletedSize()))
			return -1;
		else
			return 0;
	// khaos::categorymod-
	//MORPH START - Added by IceCream, SLUGFILLER: DLsortFix
	case 0xFFFF:
		return file1->GetPartMetFileName().CompareNoCase(file2->GetPartMetFileName());
	// SLUGFILLER: DLsortFix
	default:
		return 0;
	}
}

int CDownloadListCtrl::Compare(CUpDownClient *client1, CUpDownClient *client2, LPARAM lParamSort, int sortMod) {
	// SLUGFILLER: DLsortFix
	if (lParamSort != 0xFFFF)
		lParamSort &= 0x7FFF;
	// SLUGFILLER: DLsortFix
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
		return CompareUnsigned(client2->GetDownloadState(), client1->GetDownloadState());	// SLUGFILLER: DLsortFix - better status(downloading, on queue)<->bigger file
	case 2://transfered asc
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
				return strcmpi(client2->GetClientVerString(), client1->GetClientVerString());
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
		if ( client1->GetRemoteQueueRank() == 0 ){
			if ( client2->GetRemoteQueueRank() != 0 )
				return -1;
			if ( client1->GetDownloadState() != DS_ONQUEUE || client1->IsRemoteQueueFull() != true ){
				if ( client2->GetDownloadState() == DS_ONQUEUE && client2->IsRemoteQueueFull() == true )
					return -1;
				return 0;
			}
			else if ( client2->GetDownloadState() != DS_ONQUEUE || client2->IsRemoteQueueFull() != true )
			return 1;
			return 0;
		}
		else if ( client2->GetRemoteQueueRank() == 0 )
			return 1;

		return client2->GetRemoteQueueRank() - client1->GetRemoteQueueRank();
		//MORPH END   - Added by IceCream, SLUGFILLER: DLsortFix
	case 8:
		if( client1->GetDownloadState() == client2->GetDownloadState() ){
			//MORPH START - Added by IceCream, SLUGFILLER: DLsortFix - needed partial revamping
			if( client1->IsRemoteQueueFull() ){
				if( client2->IsRemoteQueueFull() )
					return 0;
				return -1;
			}
			else if( client2->IsRemoteQueueFull() )
				return 1;
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
				if (!theApp.glob_prefs->IsDoubleClickEnabled()){
					CPoint pt;
					::GetCursorPos(&pt);
					ScreenToClient(&pt);
					LVHITTESTINFO hit;
					hit.pt = pt;
					if (HitTest(&hit) >= 0 && (hit.flags & LVHT_ONITEM)){
						LVHITTESTINFO subhit;
						subhit.pt = pt;
						if (SubItemHitTest(&subhit) >= 0 && subhit.iSubItem == 0){
							CFileDetailDialog dialog((CPartFile*)content->value);
							dialog.DoModal();
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
	// khaos::kmod-
	if (m_FileMenu) VERIFY( m_FileMenu.DestroyMenu() );

	m_PrioMenu.CreateMenu();
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOLOW,GetResString(IDS_PRIOLOW));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIONORMAL,GetResString(IDS_PRIONORMAL));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOHIGH, GetResString(IDS_PRIOHIGH));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOAUTO, GetResString(IDS_PRIOAUTO));


	m_A4AFMenu.CreateMenu();
	//MORPH - Removed by SiRoB, Due ti Khaos A4AF
	/*m_A4AFMenu.AppendMenu(MF_STRING, MP_ALL_A4AF_TO_THIS, GetResString(IDS_ALL_A4AF_TO_THIS)); // sivka [Tarod]
	m_A4AFMenu.AppendMenu(MF_STRING, MP_ALL_A4AF_TO_OTHER, GetResString(IDS_ALL_A4AF_TO_OTHER)); // sivka
	m_A4AFMenu.AppendMenu(MF_STRING, MP_ALL_A4AF_AUTO, GetResString(IDS_ALL_A4AF_AUTO)); // sivka [Tarod]*/
	//MORPH START - Added by SiRoB, Advanced A4AF Flag from khaos
	m_A4AFMenu.AppendMenu(MF_STRING, MP_FORCEA4AFONFLAG, GetResString(IDS_A4AF_ONFLAG));
	m_A4AFMenu.AppendMenu(MF_STRING, MP_FORCEA4AFOFFFLAG, GetResString(IDS_A4AF_OFFFLAG));
	//MORPH END   - Added by SiRoB, Advanced A4AF Flag from khaos

	m_FileMenu.CreatePopupMenu();
	m_FileMenu.AddMenuTitle(GetResString(IDS_DOWNLOADMENUTITLE));
	// khaos::kmod+
	m_FileMenu.AppendMenu(MF_STRING, MP_FORCEA4AF, GetResString(IDS_A4AF_FORCEALL));
	m_FileMenu.AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_A4AFMenu.m_hMenu, GetResString(IDS_A4AF_FLAGS));
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	// khaos::kmod-
	m_FileMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PrioMenu.m_hMenu, GetResString(IDS_PRIORITY) );
	// khaos::kmod+
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	// khaos::kmod-
	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	m_FileMenu.AppendMenu(MF_STRING,MP_FAKEREPORT,GetResString(IDS_FAKEREPORT));
	m_FileMenu.AppendMenu(MF_STRING,MP_CHECKFAKE,GetResString(IDS_CHECKFAKE));
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating
	m_FileMenu.AppendMenu(MF_STRING,MP_CANCEL,GetResString(IDS_MAIN_BTN_CANCEL) );
	m_FileMenu.AppendMenu(MF_STRING,MP_STOP, GetResString(IDS_DL_STOP));
	m_FileMenu.AppendMenu(MF_STRING,MP_PAUSE, GetResString(IDS_DL_PAUSE));
	m_FileMenu.AppendMenu(MF_STRING,MP_RESUME, GetResString(IDS_DL_RESUME));
	//EastShare Start - Only download complete files v2.1 by AndCycle
	m_FileMenu.AppendMenu(MF_STRING,MP_FORCE, "Force Download");//shadow#(onlydownloadcompletefiles)
	//EastShare End - Only download complete files v2.1 by AndCycle
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	m_FileMenu.AppendMenu(MF_STRING,MP_OPEN, GetResString(IDS_DL_OPEN) );//<--9/21/02
	m_FileMenu.AppendMenu(MF_STRING,MP_PREVIEW, GetResString(IDS_DL_PREVIEW) );
	m_FileMenu.AppendMenu(MF_STRING,MP_METINFO, GetResString(IDS_DL_INFO) );//<--9/21/02
	m_FileMenu.AppendMenu(MF_STRING,MP_VIEWFILECOMMENTS, GetResString(IDS_CMT_SHOWALL) );

	m_FileMenu.AppendMenu(MF_SEPARATOR);
	m_FileMenu.AppendMenu(MF_STRING,MP_CLEARCOMPLETED, GetResString(IDS_DL_CLEAR));

	//MORPH - Removed by SiRoB, Due to Khaos A4AF
	/*if (theApp.glob_prefs->IsExtControlsEnabled()) m_FileMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_A4AFMenu.m_hMenu, GetResString(IDS_A4AF));*/

	m_FileMenu.AppendMenu(MF_STRING,MP_GETED2KLINK, GetResString(IDS_DL_LINK1) );
	m_FileMenu.AppendMenu(MF_STRING,MP_GETHTMLED2KLINK, GetResString(IDS_DL_LINK2));
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	//MORPH START - Added by IceCream, copy feedback feature
	m_FileMenu.AppendMenu(MF_STRING,MP_COPYFEEDBACK, GetResString(IDS_COPYFEEDBACK));
	m_FileMenu.AppendMenu(MF_STRING,MP_COPYFEEDBACK_US, GetResString(IDS_COPYFEEDBACK_US));
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	//MORPH END   - Added by IceCream, copy feedback feature

}

CString CDownloadListCtrl::getTextList() {
	CString out="";

	// Search for file(s)
	for(ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++){ // const is better
		CtrlItem_Struct* cur_item = it->second;
		if(cur_item->type == FILE_TYPE){
			CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);

			AddLogLine(false, _T("%s"), file->GetFileName());

			char buffer[50+1];
			ASSERT( !file->GetFileName().IsEmpty() );
			strncpy(buffer, file->GetFileName(), 50);
			buffer[50] = '\0';

			CString temp;
			temp.Format("\n%s\t [%.1f%%] %i/%i - %s",
						buffer,
						file->GetPercentCompleted(),
						file->GetTransferingSrcCount(),
						file->GetSourceCount(), 
						file->getPartfileStatus());
			
			out += temp;
		}
	}

	AddLogLine(false, _T("%s"), out);
	return out;
}

// khaos::categorymod+
void CDownloadListCtrl::ShowFilesCount() {
	CString counter;
	uint16	count=0;//theApp.downloadqueue->GetFileCount();
	uint16	totcnt=0;

	for(ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++){
		const CtrlItem_Struct* cur_item = it->second;
		if (cur_item->type == FILE_TYPE){
			CPartFile* file=(CPartFile*)cur_item->value;
			if (CheckShowItemInGivenCat(file, curTab)) count++; totcnt++;
		}
	}

	counter.Format("%s: %u (%u Total | %s)", GetResString(IDS_TW_DOWNLOADS),count,totcnt,theApp.glob_prefs->GetCategory(curTab)->viewfilters.bSuspendFilters ? GetResString(IDS_CAT_FILTERSSUSP) : GetResString(IDS_CAT_FILTERSACTIVE));
	theApp.emuledlg->transferwnd.GetDlgItem(IDC_DOWNLOAD_TEXT)->SetWindowText(counter);
}
// khaos::categorymod-

void CDownloadListCtrl::ShowSelectedFileDetails() {
	POINT point;
	::GetCursorPos(&point);
	CPoint p = point; 
    ScreenToClient(&p); 
    int it = HitTest(p); 
	SetSelectionMark(it);   // display selection mark correctly! 
    if (it == -1) return;

	CtrlItem_Struct* content = (CtrlItem_Struct*)this->GetItemData(GetSelectionMark());

	if (content->type == FILE_TYPE){
		CPartFile* file = (CPartFile*)content->value;

		if ((file->HasComment() || file->HasRating()) && p.x<13 ) {
			CFileDetailDialog dialog(file, true);
			dialog.DoModal();
		}
		else {
			CFileDetailDialog dialog(file);
			dialog.DoModal();
		}
	}else {
		CUpDownClient* client = (CUpDownClient*)content->value;
		CClientDetailDialog dialog(client);
		dialog.DoModal();
	}
}

void CDownloadListCtrl::ChangeCategory(int newsel){

	SetRedraw(FALSE);

	// remove all displayed files with a different cat and show the correct ones
	for(ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++){
		const CtrlItem_Struct* cur_item = it->second;
		if (cur_item->type == FILE_TYPE){
			CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);
			
			if ( !CheckShowItemInGivenCat(file,newsel)) HideFile(file);
				else ShowFile(file);

		}
	}

	SetRedraw(TRUE);
	curTab=newsel;
	ShowFilesCount();
}

void CDownloadListCtrl::HideFile(CPartFile* tohide){
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
	for(ListItems::const_iterator it = rangeIt.first; it != rangeIt.second; it++){
		CtrlItem_Struct* updateItem  = it->second;

		// Check if entry is already in the List
 		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)updateItem;
		sint16 result = FindItem(&find);
		if(result == (-1))
			InsertItem(LVIF_PARAM|LVIF_TEXT,GetItemCount(),LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)updateItem);

		return;
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
							_tcsncpy(pDispInfo->item.pszText, ((const CPartFile*)pItem->value)->GetFileName(), pDispInfo->item.cchTextMax);
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
				in_addr server;
				server.S_un.S_addr = client->GetServerIP();

				info.Format(GetResString(IDS_NICKNAME)+" %s\n"
					+GetResString(IDS_SERVER)+" %s:%d\n"
					+GetResString(IDS_SOURCEINFO),
							client->GetUserName(),
							inet_ntoa(server), client->GetServerPort(),
							client->GetAskedCountDown(), client->GetAvailablePartCount());

				if (content->type == 2)
				{	// normal client
					info += GetResString(IDS_CLIENTSOURCENAME) + CString(client->GetClientFilename());
				}
				else
				{	// client asked twice
					info += GetResString(IDS_ASKEDFAF);
				}
				//-For File Comment-// 
				try { 
					if (content->type==2){
						if (client->GetFileComment() != "") { 
							info += "\n" + GetResString(IDS_CMT_READ)  + " " + CString(client->GetFileComment()); 
						} 
						else { 
							//No comment entered 
							info += "\n" + GetResString(IDS_CMT_NONE); 
						} 
						info += "\n" + GetRateString(client->GetFileRate());
					}
				} catch(...) { 
					//Information not received = not connected or connecting 
					info += "\n" + GetResString(IDS_CMT_NOTCONNECTED); 
				} 
				//-End file comment-//
			}

			_tcsncpy(pGetInfoTip->pszText, info, pGetInfoTip->cchTextMax);
			pGetInfoTip->pszText[pGetInfoTip->cchTextMax-1] = _T('\0');
		}
	}
	*pResult = 0;
}
