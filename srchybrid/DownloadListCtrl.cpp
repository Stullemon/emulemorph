//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "StringConversion.h"
#include "AddSourceDlg.h"
#include "ToolTipCtrlX.h"
#include "CollectionViewDialog.h"
#include "SearchDlg.h"
#include "SharedFileList.h"
#include "MassRename.h" //SLAHAM: ADDED MassRename DownloadList
#include "log.h" //MassRename DownloadList
#include "SR13-ImportParts.h"//MORPH - Added by SiRoB, Import Parts [SR13]

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CDownloadListCtrl

#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)
#define DLC_BARUPDATE 512

#define	FILE_ITEM_MARGIN_X	4
#define RATING_ICON_WIDTH	16


IMPLEMENT_DYNAMIC(CtrlItem_Struct, CObject)

IMPLEMENT_DYNAMIC(CDownloadListCtrl, CMuleListCtrl)

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
END_MESSAGE_MAP()

CDownloadListCtrl::CDownloadListCtrl()
	: CDownloadListListCtrlItemWalk(this)
{
	m_tooltip = new CToolTipCtrlX;
	SetGeneralPurposeFind(true, false);
}

CDownloadListCtrl::~CDownloadListCtrl()
{
	if (m_PrioMenu)
		VERIFY( m_PrioMenu.DestroyMenu() );
	if (m_SourcesMenu)
		VERIFY( m_SourcesMenu.DestroyMenu() );
	//MORPH START - Added by AndCycle, showSharePermissions
	if (m_PermMenu)
		VERIFY( m_PermMenu.DestroyMenu() );	// xMule_MOD: showSharePermissions
	//MORPH END   - Added by AndCycle, showSharePermissions
	//MORPH START - Added by SiRoB, Advanced A4AF derivated from Khaos
	if (m_A4AFMenuFlag)
		VERIFY( m_A4AFMenuFlag.DestroyMenu() );
	//MORPH END   - Added by SiRoB, Advanced A4AF derivated from Khaos
	if (m_FileMenu)
		VERIFY( m_FileMenu.DestroyMenu() );
	
	while (m_ListItems.empty() == false) {
		delete m_ListItems.begin()->second; // second = CtrlItem_Struct*
		m_ListItems.erase(m_ListItems.begin());
	}
	delete m_tooltip;
}

void CDownloadListCtrl::Init()
{
	SetName(_T("DownloadListCtrl"));
	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy, theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	SetStyle();
	ModifyStyle(LVS_SINGLESEL,0);
	
	CToolTipCtrl* tooltip = GetToolTips();
	if (tooltip){
		m_tooltip->SubclassWindow(*tooltip);
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
	lsctitle.Remove(_T(':'));
	InsertColumn(10, lsctitle,LVCFMT_LEFT, 220);
	lsctitle=GetResString(IDS_FD_LASTCHANGE);
	lsctitle.Remove(_T(':'));
	InsertColumn(11, lsctitle,LVCFMT_LEFT, 220);
	// khaos::categorymod+ Two new ResStrings, too.
	InsertColumn(12, GetResString(IDS_CAT_COLCATEGORY),LVCFMT_LEFT,60);
	InsertColumn(13, GetResString(IDS_CAT_COLORDER),LVCFMT_LEFT,60);
	// khaos::categorymod-
	// khaos::accuratetimerem+
	InsertColumn(14, GetResString(IDS_REMAININGSIZE), LVCFMT_LEFT, 80);
	// khaos::accuratetimerem-
	//MORPH START - Added by SiRoB, WebCache 1.2f
	InsertColumn(15, GetResString(IDS_WC_SOURCES) ,LVCFMT_LEFT, 100); //JP Webcache column
	//MORPH END   - Added by SiRoB, WebCache 1.2f
	//MORPH START - Added by SiRoB, IP2Country
	InsertColumn(16, GetResString(IDS_COUNTRY) ,LVCFMT_LEFT, 100);
	//MORPH END   - Added by SiRoB, IP2Country
	SetAllIcons();
	Localize();
	LoadSettings();
	curTab=0;

	//MORPH - Removed by SiRoB, Allways creat the font
	/*
	if (thePrefs.GetShowActiveDownloadsBold())
	{
	*/
		CFont* pFont = GetFont();
		LOGFONT lfFont = {0};
		pFont->GetLogFont(&lfFont);
		lfFont.lfWeight = FW_BOLD;
		m_fontBold.CreateFontIndirect(&lfFont);
	//} //MORPH - Removed by SiRoB, Allways creat the font
	//MORPH START - Added by SiRoB, Draw Client Percentage
	//m_fontBoldSmaller.CreateFont(12,0,0,1,FW_BOLD,0,0,0,0,3,2,1,34,_T("MS Serif"));
	lfFont.lfHeight = 11;
	m_fontBoldSmaller.CreateFontIndirect(&lfFont);
	//MORPH END   - Added by SiRoB, Draw Client Percentage

	// Barry - Use preferred sort order from preferences
	//MORPH START - Changed by SiRoB, Remain time and size Columns have been splited
	/*
	m_bRemainSort=thePrefs.TransferlistRemainSortStyle();

	uint8 adder=0;
	if (GetSortItem()!=9 || !m_bRemainSort)
		SetSortArrow();
	else {
		SetSortArrow(GetSortItem(), GetSortAscending()?arrowDoubleUp : arrowDoubleDown);
		adder=81;
	}
	
	SortItems(SortProc, GetSortItem() + (GetSortAscending()? 0:100) + adder);
	*/
	SetSortArrow();
	SortItems(SortProc, GetSortItem() + (GetSortAscending()? 0:100));
	//MORPH END - Changed by SiRoB, Remain time and size Columns have been splited
}

void CDownloadListCtrl::OnSysColorChange()
{
	CMuleListCtrl::OnSysColorChange();
	SetAllIcons();
	CreateMenues();
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
	m_ImageList.Add(CTempIconLoader(_T("ClientEDonkeyHybrid")));
	m_ImageList.Add(CTempIconLoader(_T("ClientShareaza")));
	m_ImageList.Add(CTempIconLoader(_T("Server")));
	m_ImageList.Add(CTempIconLoader(_T("ClientAMule")));
	m_ImageList.Add(CTempIconLoader(_T("ClientLPhant")));
	m_ImageList.Add(CTempIconLoader(_T("RATING_NOTRATED")));
	m_ImageList.Add(CTempIconLoader(_T("RATING_FAKE")));
	m_ImageList.Add(CTempIconLoader(_T("RATING_POOR")));
	m_ImageList.Add(CTempIconLoader(_T("RATING_FAIR")));
	m_ImageList.Add(CTempIconLoader(_T("RATING_GOOD")));
	m_ImageList.Add(CTempIconLoader(_T("RATING_EXCELLENT")));
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("ClientSecureOvl"))), 1);
	//MORPH START - Added by SiRoB, More client & Credit Overlay Icon
	m_ImageList.Add(CTempIconLoader(_T("ClientRightEdonkey"))); //21
	m_ImageList.Add(CTempIconLoader(_T("Morph"))); //22
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("ClientCreditOvl"))), 2); //23
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("ClientCreditSecureOvl"))), 3); //24
	//MORPH END   - Added by SiRoB, More client & Credit Overlay Icon
	//MORPH START - Added by SiRoB, WebCache 1.2f
	m_ImageList.Add(CTempIconLoader(_T("WEBCACHE"))); // 25// jp webcacheclient icon
	//MORPH END   - Added by SiRoB, WebCache 1.2
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

void CDownloadListCtrl::Localize()
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	strRes = GetResString(IDS_DL_FILENAME);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(0, &hdi);

	strRes = GetResString(IDS_DL_SIZE);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(1, &hdi);

	strRes = GetResString(IDS_DL_TRANSF);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(2, &hdi);

	strRes = GetResString(IDS_DL_TRANSFCOMPL);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(3, &hdi);

	strRes = GetResString(IDS_DL_SPEED);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(4, &hdi);

	strRes = GetResString(IDS_DL_PROGRESS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(5, &hdi);

	strRes = GetResString(IDS_DL_SOURCES);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(6, &hdi);

	strRes = GetResString(IDS_PRIORITY);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(7, &hdi);

	strRes = GetResString(IDS_STATUS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(8, &hdi);

	strRes = GetResString(IDS_DL_REMAINS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(9, &hdi);

	strRes = GetResString(IDS_LASTSEENCOMPL);
	strRes.Remove(_T(':'));
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(10, &hdi);

	strRes = GetResString(IDS_FD_LASTCHANGE);
	strRes.Remove(_T(':'));
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(11, &hdi);

	strRes = GetResString(IDS_CAT);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(12, &hdi);

	// khaos::categorymod+
	strRes = GetResString(IDS_CAT_COLCATEGORY);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(12, &hdi);

	strRes = GetResString(IDS_CAT_COLORDER);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(13, &hdi);
	// khaos::categorymod-

	// khaos::accuratetimerem+
	strRes = GetResString(IDS_REMAININGSIZE);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(14, &hdi);
	// khaos::accuratetimerem-
		
	//MORPH START - Added by SiRoB, WebCache 1.2f
	strRes = GetResString(IDS_WC_SOURCES);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(15, &hdi);
	//MORPH END   - Added by SiRoB, WebCache 1.2f

	//MORPH START - Added by SiRoB, IP2Country
	strRes = GetResString(IDS_COUNTRY);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(16, &hdi);
	//MORPH END   - Added by SiRoB, IP2Country

	CreateMenues();
	ShowFilesCount();
}

void CDownloadListCtrl::AddFile(CPartFile* toadd)
{
	// Create new Item
    CtrlItem_Struct* newitem = new CtrlItem_Struct;
    int itemnr = GetItemCount();
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
		int result = FindItem(&find);
		if (result != -1)
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
			int result = FindItem(&find);
			if (result != -1)
				DeleteItem(result);

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
			int result = FindItem(&find);
			if (result != -1)
				DeleteItem(result);

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
	//MORPH START - SiRoB, Don't Refresh item if not needed
	if( theApp.emuledlg->activewnd != theApp.emuledlg->transferwnd  || theApp.emuledlg->transferwnd->downloadlistctrl.IsWindowVisible() == false )
		return;
	//MORPH END   - SiRoB, Don't Refresh item if not needed
	
	// Retrieve all entries matching the source
	std::pair<ListItems::const_iterator, ListItems::const_iterator> rangeIt = m_ListItems.equal_range(toupdate);
	for(ListItems::const_iterator it = rangeIt.first; it != rangeIt.second; it++){
		CtrlItem_Struct* updateItem  = it->second;

		// Find entry in CListCtrl and update object
 		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)updateItem;
		int result = FindItem(&find);
		if (result != -1){
			updateItem->dwUpdated = 0;
			Update(result);
		}
	}
}

void CDownloadListCtrl::DrawFileItem(CDC *dc, int nColumn, LPCRECT lpRect, CtrlItem_Struct *lpCtrlItem)
{
	if(lpRect->left < lpRect->right)
	{
		CString buffer;
		/*const*/CPartFile *lpPartFile = (CPartFile*)lpCtrlItem->value;
		switch(nColumn)
		{
		case 0:{		// file name
			CRect rcDraw(lpRect);
			int iImage = theApp.GetFileTypeSystemImageIdx(lpPartFile->GetFileName());
			if (theApp.GetSystemImageList() != NULL)
				::ImageList_Draw(theApp.GetSystemImageList(), iImage, dc->GetSafeHdc(), rcDraw.left, rcDraw.top, ILD_NORMAL|ILD_TRANSPARENT);
			rcDraw.left += theApp.GetSmallSytemIconSize().cx;
			if ( thePrefs.ShowRatingIndicator() ){
				if  (lpPartFile->HasComment() || lpPartFile->HasRating()){
 					if (thePrefs.ShowRatingIndicator() && (lpPartFile->HasComment() || lpPartFile->HasRating()))
						m_ImageList.Draw(dc, lpPartFile->UserRating()+14, rcDraw.TopLeft(), ILD_NORMAL);
				}
				rcDraw.left += RATING_ICON_WIDTH;
			}

			rcDraw.left += 3;
			dc->DrawText(lpPartFile->GetFileName(), lpPartFile->GetFileName().GetLength(),&rcDraw, DLC_DT_TEXT);
			break;
		}

		case 1:		// size
			buffer = CastItoXBytes(lpPartFile->GetFileSize(), false, false);
			dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT | DT_RIGHT);
			break;

		case 2:		// transferred
			buffer = CastItoXBytes(lpPartFile->GetTransferred(), false, false);
			dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT | DT_RIGHT);
			break;

		case 3:		// transferred complete
			buffer = CastItoXBytes(lpPartFile->GetCompletedSize(), false, false);
			dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT | DT_RIGHT);
			break;

		case 4:		// speed
			if (lpPartFile->GetTransferringSrcCount())
				buffer.Format(_T("%s"), CastItoXBytes(lpPartFile->GetDatarate(), false, true));
			dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT | DT_RIGHT);
			break;

		case 5:		// progress
			{
				CRect rcDraw(*lpRect);
				rcDraw.bottom--;
				rcDraw.top++;

				// added
				int iWidth = rcDraw.Width();
				int iHeight = rcDraw.Height();
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

				dc->BitBlt(rcDraw.left, rcDraw.top, iWidth, iHeight,  &cdcStatus, 0, 0, SRCCOPY); 
				cdcStatus.SelectObject(hOldBitmap);
				//added end

				if (thePrefs.GetUseDwlPercentage()) {
					// HoaX_69: BEGIN Display percent in progress bar
					COLORREF oldclr = dc->SetTextColor(RGB(0,0,0)); //MORPH - Changed by SIRoB, Turn into black pen
					int iOMode = dc->SetBkMode(TRANSPARENT);
					buffer.Format(_T("%.1f%%"), lpPartFile->GetPercentCompleted());
					//MORPH START - Changed by SiRoB, Bold Percentage :) and right justify
					CFont *pOldFont = dc->SelectObject(&m_fontBold);
					
#define	DrawPercentText	dc->DrawText(buffer, buffer.GetLength(), &rcDraw, ((DLC_DT_TEXT | DT_RIGHT) & ~DT_LEFT) | DT_CENTER)
					DrawPercentText;
					rcDraw.left+=1;rcDraw.right+=1;
					DrawPercentText;
					rcDraw.left+=1;rcDraw.right+=1;
					DrawPercentText;
					
					rcDraw.top+=1;rcDraw.bottom+=1;
					DrawPercentText;
					rcDraw.top+=1;rcDraw.bottom+=1;
					DrawPercentText;
					
					rcDraw.left-=1;rcDraw.right-=1;
					DrawPercentText;
					rcDraw.left-=1;rcDraw.right-=1;
					DrawPercentText;
					
					rcDraw.top-=1;rcDraw.bottom-=1;
					DrawPercentText;
					
					rcDraw.left++;rcDraw.right++;
					dc->SetTextColor(RGB(255,255,255));
					DrawPercentText;
					dc->SelectObject(pOldFont); //MORPH - Added by SiRoB, Bold Percentage :)
					//MORPH END   - Changed by SiRoB, Bold Percentage :) and right justify
					dc->SetBkMode(iOMode);
					dc->SetTextColor(oldclr);
					// HoaX_69: END
				}
			}
			break;

		case 6:		// sources
			{
				UINT sc = lpPartFile->GetSourceCount();
				//MORPH START - Modified by IceCream, [sivka: -counter for A4AF in sources column-]
				buffer.Format(_T("%i/%i/%i (%i)"),
					lpPartFile->GetSrcA4AFCount(), //MORPH - Added by SiRoB, A4AF counter
					(lpPartFile->GetSrcStatisticsValue(DS_ONQUEUE) + lpPartFile->GetSrcStatisticsValue(DS_DOWNLOADING)), //MORPH - Modified by SiRoB
					sc, lpPartFile->GetTransferringSrcCount());
				//MORPH END   - Modified by IceCream, [sivka: -counter for A4AF in sources column-]
				if (thePrefs.IsExtControlsEnabled() && lpPartFile->GetPrivateMaxSources() != 0)
					buffer.AppendFormat(_T(" [%i]"), lpPartFile->GetPrivateMaxSources());
				dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT | DT_RIGHT);
			}
			break;

		case 7:		// prio
			switch(lpPartFile->GetDownPriority()) {
			case PR_LOW:
				if( lpPartFile->IsAutoDownPriority() )
					dc->DrawText(GetResString(IDS_PRIOAUTOLOW),GetResString(IDS_PRIOAUTOLOW).GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
				else
					dc->DrawText(GetResString(IDS_PRIOLOW),GetResString(IDS_PRIOLOW).GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
				break;
			case PR_NORMAL:
				if( lpPartFile->IsAutoDownPriority() )
					dc->DrawText(GetResString(IDS_PRIOAUTONORMAL),GetResString(IDS_PRIOAUTONORMAL).GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
				else
					dc->DrawText(GetResString(IDS_PRIONORMAL),GetResString(IDS_PRIONORMAL).GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
				break;
			case PR_HIGH:
				if( lpPartFile->IsAutoDownPriority() )
					dc->DrawText(GetResString(IDS_PRIOAUTOHIGH),GetResString(IDS_PRIOAUTOHIGH).GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
				else
					dc->DrawText(GetResString(IDS_PRIOHIGH),GetResString(IDS_PRIOHIGH).GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
				break;
			}
			break;

		case 8:		// <<--9/21/02
			buffer = lpPartFile->getPartfileStatus();
			dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
			break;

		// khaos::accuratetimerem+
		/*
		case 9:		// remaining time & size
			{
				if (lpPartFile->GetStatus()!=PS_COMPLETING && lpPartFile->GetStatus()!=PS_COMPLETE ){
					// time 
					time_t restTime;
					if (!thePrefs.UseSimpleTimeRemainingComputation())
						restTime = lpPartFile->getTimeRemaining();
					else
						restTime = lpPartFile->getTimeRemainingSimple();

					buffer.Format(_T("%s (%s)"), CastSecondsToHM(restTime), CastItoXBytes((lpPartFile->GetFileSize() - lpPartFile->GetCompletedSize()), false, false));
				}
				dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
			}
			break;
		*/
		case 9:		// remaining time NOT size
			{
				if (lpPartFile->GetStatus() != PS_COMPLETING && lpPartFile->GetStatus() != PS_COMPLETE)
				{
					switch (thePrefs.GetTimeRemainingMode()) {
					case 0:
						{
							sint32 curTime = lpPartFile->getTimeRemaining();
							sint32 avgTime = lpPartFile->GetTimeRemainingAvg();
							buffer.Format(_T("%s (%s)"), CastSecondsToHM(curTime), CastSecondsToHM(avgTime));
							break;
						}
					case 1:
						{
							sint32 curTime = lpPartFile->getTimeRemaining();
							buffer.Format(_T("%s"), CastSecondsToHM(curTime));
							break;
						}
					case 2:
						{
							sint32 avgTime = lpPartFile->GetTimeRemainingAvg();
							buffer.Format(_T("%s"), CastSecondsToHM(avgTime));
							break;
						}
					}
				}
				dc->DrawText(buffer, buffer.GetLength(), const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
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
					buffer.Format(_T("%s (%s)"),GetResString(IDS_NEVER),tempbuffer);
				else
					buffer.Format(_T("%s (%s)"),lpPartFile->lastseencomplete.Format( thePrefs.GetDateTimeFormat()),tempbuffer);
				dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
			}
			break;
		case 11: // last receive
			if (!IsColumnHidden(11)) {
				if(lpPartFile->GetFileDate()!=NULL && lpPartFile->GetCompletedSize() > (uint64)0)
					buffer=lpPartFile->GetCFileDate().Format( thePrefs.GetDateTimeFormat());
				else
					buffer.Format(_T("%s"),GetResString(IDS_NEVER));

				dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
			}
			break;
		// khaos::categorymod+
		case 12: // Category
			{
				if (!thePrefs.ShowCatNameInDownList())
					buffer.Format(_T("%u"), lpPartFile->GetCategory());
				else
					buffer.Format(_T("%s"), thePrefs.GetCategory(lpPartFile->GetCategory())->title);
				dc->DrawText(buffer, buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
				break;
			}
		case 13: // Resume Mod
			{
				buffer = _T("");
				//MORPH START - Added by SiRoB, ForcedA4AF
				if (thePrefs.UseSmartA4AFSwapping() && lpPartFile->GetStatus() != PS_PAUSED)
				{
					if (lpPartFile == theApp.downloadqueue->forcea4af_file) {
						if (buffer.IsEmpty() == false) buffer.Append(_T(", "));					
						buffer.AppendFormat(_T("%s"), GetResString(IDS_A4AF_FORCEALL));
					}
					if (lpPartFile->ForceA4AFOff()) {
						if (buffer.IsEmpty() == false) buffer.Append(_T(", "));
						buffer.AppendFormat(_T("%s"), GetResString(IDS_A4AF_OFFFLAG));
					}
					if (lpPartFile->ForceAllA4AF()) {
						if (buffer.IsEmpty() == false) buffer.Append(_T(", "));
						buffer.AppendFormat(_T("%s"), GetResString(IDS_A4AF_ONFLAG));
					}
				}
				//MORPH END   - Added by SiRoB, ForcedA4AF
				Category_Struct* ActiveCat=thePrefs.GetCategory(theApp.emuledlg->transferwnd->GetActiveCategory());
				Category_Struct* curCat=thePrefs.GetCategory(lpPartFile->GetCategory());
				if (curCat && ActiveCat && ActiveCat->viewfilters.nFromCats == 0) {
					if (buffer.IsEmpty() == false) buffer.Append(_T(", "));
					switch (curCat->prio) {
						case PR_LOW:
							buffer.AppendFormat(_T("%s"), GetResString(IDS_PRIOLOW));
							break;
						case PR_NORMAL:
							buffer.AppendFormat(_T("%s"), GetResString(IDS_PRIONORMAL));
							break;
						case PR_HIGH:
							buffer.AppendFormat(_T("%s"), GetResString(IDS_PRIOHIGH));
							break;
					}
					//MORPH START - Added by SiRoB, Avanced A4AF
					if (buffer.IsEmpty() == false) buffer.Append(_T(", "));
					UINT iA4AFMode = thePrefs.AdvancedA4AFMode();
					if (iA4AFMode && curCat->iAdvA4AFMode)
						iA4AFMode = curCat->iAdvA4AFMode;
					else
						buffer.Append(((CString)GetResString(IDS_DEFAULT)).Left(1) + _T(". "));
					switch (iA4AFMode) {
						case 0:
							buffer.AppendFormat(_T("%s"), GetResString(IDS_A4AF_DISABLED));
							break;
						case 1:
							buffer.AppendFormat(_T("%s"), GetResString(IDS_A4AF_BALANCE));
							break;
						case 2:
							buffer.AppendFormat(_T("%s=%u: %s"), GetResString(IDS_CAT_COLORDER), lpPartFile->GetCatResumeOrder(), GetResString(IDS_A4AF_STACK));
							break;
					}

					if (curCat->downloadInAlphabeticalOrder) {
						if (buffer.IsEmpty() == false) buffer.Append(_T(", "));
						buffer.AppendFormat(_T("%s"), GetResString(IDS_DOWNLOAD_ALPHABETICAL));
					}
				} else {
					if (buffer.IsEmpty() == false) buffer.Append(_T(", "));
					buffer.AppendFormat(_T("%s=%u"), GetResString(IDS_CAT_COLORDER), lpPartFile->GetCatResumeOrder());
				}
				//MORPH END   - Added by SiRoB, Avanced A4AF
				dc->DrawText(buffer, buffer.GetLength(), const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
				break;
			}
		// khaos::categorymod-
		// khaos::accuratetimerem+
		case 14:		// remaining size
			{
				if (lpPartFile->GetStatus()!=PS_COMPLETING && lpPartFile->GetStatus()!=PS_COMPLETE ){
					//size 
					uint64 remains;
					remains=lpPartFile->GetFileSize()-lpPartFile->GetCompletedSize();	//<<-- 09/27/2002, CML

					buffer.Format(_T("%s"), CastItoXBytes(remains, false, false));

				}
				dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
			}
			break;
		// khaos::accuratetimerem-
		//MORPH START - Added by SiRoB, WebCache 1.2f
		//JP Webcache START
		//JP added code from Gnaddelwarz
		case 15: //WebCache
			{
				UINT wcsc = lpPartFile->GetWebcacheSourceCount();
				UINT wcsc_our;
				UINT wcsc_not_our;
				if(thePrefs.IsExtControlsEnabled())
				{
				wcsc_our = lpPartFile->GetWebcacheSourceOurProxyCount();
				wcsc_not_our = lpPartFile->GetWebcacheSourceNotOurProxyCount();
				}

				UINT sc = lpPartFile->GetSourceCount() + lpPartFile->GetSrcA4AFCount();
				double PercentWCClients;
				if (sc !=0)
					PercentWCClients = (double) 100 * wcsc / sc;
				else
					PercentWCClients = 0;
                if(wcsc > 0 && !(lpPartFile->GetStatus() == PS_PAUSED && wcsc == 0) && lpPartFile->GetStatus() != PS_COMPLETE) {
					if(thePrefs.IsExtControlsEnabled())
						buffer.Format(_T("%i/%i/%i (%1.1f%%)"), wcsc, wcsc_our, wcsc_not_our, PercentWCClients);
					else
                    buffer.Format(_T("%i (%1.1f%%)"), wcsc, PercentWCClients);
                } else {
                    buffer = _T("");
				}
				dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT | DT_RIGHT);
			}
			break;
		//JP Webcache END
		//MORPH END   - Added by SiRoB, WebCache 1.2f
		}
	}
}

void CDownloadListCtrl::DrawSourceItem(CDC *dc, int nColumn, LPCRECT lpRect, CtrlItem_Struct *lpCtrlItem) {
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
					case DS_WAITCALLBACKKAD:
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
					case DS_ERROR:
						m_ImageList.Draw(dc, 3, point, ILD_NORMAL);
						break;
					case DS_TOOMANYCONNS:
					case DS_TOOMANYCONNSKAD:
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
				else*/ if (lpUpDownClient->GetClientSoft() == SO_EDONKEYHYBRID)
					m_ImageList.Draw(dc, 9, point2, ILD_NORMAL | uOvlImg);
				else if (lpUpDownClient->GetClientSoft() == SO_MLDONKEY)
					m_ImageList.Draw(dc, 8, point2, ILD_NORMAL | uOvlImg);
				else if (lpUpDownClient->GetClientSoft() == SO_SHAREAZA)
					m_ImageList.Draw(dc, 10, point2, ILD_NORMAL | uOvlImg);
				else if (lpUpDownClient->GetClientSoft() == SO_URL)
					m_ImageList.Draw(dc, 11, point2, ILD_NORMAL | uOvlImg);
				else if (lpUpDownClient->GetClientSoft() == SO_AMULE)
					m_ImageList.Draw(dc, 12, point2, ILD_NORMAL | uOvlImg);
				else if (lpUpDownClient->GetClientSoft() == SO_LPHANT)
					m_ImageList.Draw(dc, 13, point2, ILD_NORMAL | uOvlImg);
				//MORPH START - Added by SiRoB, WebCache 1.2f
				// jp webcacheclient icon START 
				else if (lpUpDownClient->GetClientSoft() == SO_WEBCACHE)
					m_ImageList.Draw(dc, 25, point2, ILD_NORMAL | uOvlImg);
				// jp webcacheclient icon END
				//MORPH END   - Added by SiRoB, WebCache 1.2f
				else if (lpUpDownClient->ExtProtocolAvailable())
					m_ImageList.Draw(dc, (lpUpDownClient->IsMorph())?22:5, point2, ILD_NORMAL | uOvlImg);
				else if ( lpUpDownClient->GetClientSoft() == SO_EDONKEY)
						m_ImageList.Draw(dc, 21, point2, ILD_NORMAL | uOvlImg);
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
				if(theApp.ip2country->ShowCountryFlag() && IsColumnHidden(16)){
					POINT point3= {cur_rec.left,cur_rec.top+1};
					theApp.ip2country->GetFlagImageList()->DrawIndirect(dc, lpUpDownClient->GetCountryFlagIndex(), point3, CSize(18,16), CPoint(0,0), ILD_NORMAL);
					cur_rec.left+=20;
				}
				//Morph End - added by AndCycle, IP to Country

				if (!lpUpDownClient->GetUserName())
					buffer = _T("?");
				else
					buffer = lpUpDownClient->GetUserName();
				//MORPH START - Added by IceCream, [sivka: -A4AF counter, ahead of user nickname-]
				CString tempStr;
				tempStr.Format(_T("(%i) %s"),lpUpDownClient->m_OtherRequests_list.GetCount()+1+lpUpDownClient->m_OtherNoNeeded_list.GetCount(),buffer);
				buffer = tempStr;
				//MORPH END   - Added by IceCream, [sivka: -A4AF counter, ahead of user nickname-]

				dc->DrawText(buffer,buffer.GetLength(),&cur_rec, DLC_DT_TEXT);
			}
			break;

		case 1:		// size
			switch(lpUpDownClient->GetSourceFrom()){
				case SF_SERVER:
					buffer = _T("eD2K Server");
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
					buffer = GetResString(IDS_SOURCE_LOADER_SAVER); //ADDED IDS by FrankyFive
					break;
				//MORPH END   - Added by SiRoB, Source Loader Saver [SLS]
			}
			dc->DrawText(buffer, buffer.GetLength(), const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
			break;

		case 2:// transferred
			//MORPH START - Changed by SiRoB, Download/Upload
			/*
			if (!IsColumnHidden(3)) {
				dc->DrawText(_T(""), 0, const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
				break;
			}
			*/
			if(lpUpDownClient->Credits() && (lpUpDownClient->Credits()->GetUploadedTotal() || lpUpDownClient->Credits()->GetDownloadedTotal())){
				buffer.Format( _T("%s/%s"),
				CastItoXBytes(lpUpDownClient->Credits()->GetDownloadedTotal(), false, false),
				CastItoXBytes(lpUpDownClient->Credits()->GetUploadedTotal(), false, false));
				dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
			}
			break;
			//MORPH END  - Changed by SiRoB, Download/Upload
		case 3:// completed
			// - 'Transferred' column: Show transferred data
			// - 'Completed' column: If 'Transferred' column is hidden, show the amount of transferred data
			//	  in 'Completed' column. This is plain wrong (at least when receiving compressed data), but
			//	  users seem to got used to it.
			if (nColumn == 2 || IsColumnHidden(2)) {
				if (lpCtrlItem->type == AVAILABLE_SOURCE && lpUpDownClient->GetTransferredDown()) {
					buffer = CastItoXBytes(lpUpDownClient->GetTransferredDown(), false, false);
					dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT | DT_RIGHT);
				}
			}
			break;

		case 4:		// speed
			if (lpCtrlItem->type == AVAILABLE_SOURCE && lpUpDownClient->GetDownloadDatarate()){
				if (lpUpDownClient->GetDownloadDatarate())
					buffer.Format(_T("%s"), CastItoXBytes(lpUpDownClient->GetDownloadDatarate(), false, true));
				dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT | DT_RIGHT);
			}
			break;

		case 5:		// file info
			{
				CRect rcDraw(*lpRect);
				rcDraw.bottom--; 
				rcDraw.top++; 

				int iWidth = rcDraw.Width();
				int iHeight = rcDraw.Height();
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
					/*
					lpUpDownClient->DrawStatusBar(&cdcStatus,  &rec_status,(lpCtrlItem->type == UNAVAILABLE_SOURCE), thePrefs.UseFlatBar()); 
					*/
					lpUpDownClient->DrawStatusBar(&cdcStatus,  &rec_status,(CPartFile*)lpCtrlItem->owner, thePrefs.UseFlatBar());
					//MORPH END   - Changed by SiRoB, Advanced A4AF derivated from Khaos

					//Commander - Added: Client percentage - Start
					if (thePrefs.GetUseClientPercentage() && lpUpDownClient->GetPartStatus() && lpCtrlItem->type == AVAILABLE_SOURCE)
					{
						float percent = (float)lpUpDownClient->GetAvailablePartCount() / (float)lpUpDownClient->GetPartCount()* 100.0f;
						if (percent > 0.05f)
						{
							//Commander - Added: Draw Client Percentage xored, caching before draw - Start
							COLORREF oldclr = cdcStatus.SetTextColor(RGB(0,0,0));
							int iOMode = cdcStatus.SetBkMode(TRANSPARENT);
							buffer.Format(_T("%.1f%%"), percent);
							CFont *pOldFont = cdcStatus.SelectObject(&m_fontBoldSmaller);
#define	DrawClientPercentText		cdcStatus.DrawText(buffer, buffer.GetLength(),&rec_status, ((DLC_DT_TEXT | DT_RIGHT) & ~DT_LEFT) | DT_CENTER)
							rec_status.top-=1;rec_status.bottom-=1;
							DrawClientPercentText;rec_status.left+=1;rec_status.right+=1;
							DrawClientPercentText;rec_status.left+=1;rec_status.right+=1;
							DrawClientPercentText;rec_status.top+=1;rec_status.bottom+=1;
							DrawClientPercentText;rec_status.top+=1;rec_status.bottom+=1;
							DrawClientPercentText;rec_status.left-=1;rec_status.right-=1;
							DrawClientPercentText;rec_status.left-=1;rec_status.right-=1;
							DrawClientPercentText;rec_status.top-=1;rec_status.bottom-=1;
							DrawClientPercentText;rec_status.left++;rec_status.right++;
							cdcStatus.SetTextColor(RGB(255,255,255));
							DrawClientPercentText;
							cdcStatus.SelectObject(pOldFont);
							cdcStatus.SetBkMode(iOMode);
							cdcStatus.SetTextColor(oldclr);
							//Commander - Added: Draw Client Percentage xored, caching before draw - End	
						}
					}
					//Commander - Added: Client percentage - End

					lpCtrlItem->dwUpdated = dwTicks + (rand() % 128); 
				} else 
					hOldBitmap = cdcStatus.SelectObject(lpCtrlItem->status); 

				dc->BitBlt(rcDraw.left, rcDraw.top, iWidth, iHeight,  &cdcStatus, 0, 0, SRCCOPY); 
				cdcStatus.SelectObject(hOldBitmap);
			}
			break;

		case 6:		// sources
		{
			buffer = lpUpDownClient->GetClientSoftVer();
			if (buffer.IsEmpty())
				buffer = GetResString(IDS_UNKNOWN);
			CRect rc(lpRect);
			dc->DrawText(buffer, buffer.GetLength(), &rc, DLC_DT_TEXT);
			break;
		}

		case 7:	{	// prio
				// EastShare START - Removed by TAHO, move to Status Column
				/*
			if (lpUpDownClient->GetDownloadState()==DS_ONQUEUE){
				if( lpUpDownClient->IsRemoteQueueFull() ){
					buffer = GetResString(IDS_QUEUEFULL);
					dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
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
						dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
						dc->SetTextColor(crOldTxtColor);
						//Morph - modified by AndCycle, DiffQR
					}
					else{
						dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
					}
				}
			}
			else {
				dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
				*/
				// EastShare END - Removed by TAHO, move to Status Column
				// EastShare START - Addeded by TAHO, last asked time
				uint32 lastAskedTime = lpUpDownClient->GetLastAskedTime();
				if ( lastAskedTime )
					buffer = CastSecondsToHM(( ::GetTickCount() - lastAskedTime) /1000);
				else
					buffer = "?";
				dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), (DLC_DT_TEXT | DT_RIGHT) & ~DT_LEFT);
				// EastShare END - Addeded by TAHO, last asked time
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
                    if(lpUpDownClient->IsInNoNeededList(lpCtrlItem->owner)) {
                        buffer += _T(" (") + GetResString(IDS_NONEEDEDPARTS) + _T(")");
                    } else if(lpUpDownClient->GetDownloadState() == DS_DOWNLOADING) {
                        buffer += _T(" (") + GetResString(IDS_TRANSFERRING) + _T(")");
                    } else if(lpUpDownClient->IsSwapSuspended(lpUpDownClient->GetRequestFile())) {
                        buffer += _T(" (") + GetResString(IDS_SOURCESWAPBLOCKED) + _T(")");
                    }

                    if (lpUpDownClient && lpUpDownClient->GetRequestFile() && lpUpDownClient->GetRequestFile()->GetFileName()){
                        buffer.AppendFormat(_T(": \"%s\""),lpUpDownClient->GetRequestFile()->GetFileName());
                    }
                }
			}

            if(thePrefs.IsExtControlsEnabled() && !lpUpDownClient->m_OtherRequests_list.IsEmpty()) {
                buffer.Append(_T("*"));
            }
// ZZ:DownloadManager <--

			COLORREF crOldTxtColor = dc->SetTextColor((COLORREF)RGB(200,80,200));; // EastShare - Moddified by TAHO, color
			if (lpCtrlItem->type == AVAILABLE_SOURCE){
				switch (lpUpDownClient->GetDownloadState()) {
					case DS_CONNECTING:
						crOldTxtColor = dc->SetTextColor((COLORREF)RGB(210,210,10));
						break;
					case DS_CONNECTED:
						crOldTxtColor = dc->SetTextColor((COLORREF)RGB(210,210,10));
						break;
					case DS_WAITCALLBACK:
						crOldTxtColor = dc->SetTextColor((COLORREF)RGB(210,210,10));
						break;
					case DS_ONQUEUE:
						if( lpUpDownClient->IsRemoteQueueFull() ){
							crOldTxtColor = dc->SetTextColor((COLORREF)RGB(10,130,160));
						}
						else {
							if ( lpUpDownClient->GetRemoteQueueRank()){
								int	m_iDifference = lpUpDownClient->GetDiffQR();
								if(m_iDifference == 0){
									crOldTxtColor = dc->SetTextColor((COLORREF)RGB(60,10,240));
								}
								else if(m_iDifference > 0){
									crOldTxtColor = dc->SetTextColor((COLORREF)RGB(240,125,10));
								}
								else if(m_iDifference < 0){
									crOldTxtColor = dc->SetTextColor((COLORREF)RGB(10,180,50));
								}
							}
							else{
								crOldTxtColor = dc->SetTextColor((COLORREF)RGB(50,80,140));
							}
						}
						break;
					case DS_DOWNLOADING:
						crOldTxtColor = dc->SetTextColor((COLORREF)RGB(192,0,0));
						break;
					case DS_REQHASHSET:
						crOldTxtColor = dc->SetTextColor((COLORREF)RGB(245,240,100));
						break;
					case DS_NONEEDEDPARTS:
						crOldTxtColor = dc->SetTextColor((COLORREF)RGB(30,200,240)); 
						break;
					case DS_LOWTOLOWIP:
						crOldTxtColor = dc->SetTextColor((COLORREF)RGB(135,135,135)); 
						break;
					case DS_TOOMANYCONNS:
						crOldTxtColor = dc->SetTextColor((COLORREF)RGB(135,135,135)); 
						break;
					default:
						crOldTxtColor = dc->SetTextColor((COLORREF)RGB(135,135,135)); 
				}
			}
			// EastShare END - Added by TAHO, color
			dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
			dc->SetTextColor(crOldTxtColor); // EastShare - Added by TAHO, color
			break;
				}
		//SLAHAM: ADDED Show Downloading Time =>
		case 9:		// remaining time & size
			buffer.Format(_T("DT: %s (%s)[%u]"), CastSecondsToHM(lpUpDownClient->dwSessionDLTime/1000), CastSecondsToHM(lpUpDownClient->dwTotalDLTime/1000), lpUpDownClient->uiStartDLCount);
			dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
			break;
			//SLAHAM: ADDED Show Downloading Time <=
			//SLAHAM: ADDED Known Since =>
		case 10:	// last seen complete
			if( lpUpDownClient->dwThisClientIsKnownSince )
				buffer.Format(GetResString(IDS_KNOWN_SINCE) + _T(" %s"), CastSecondsToHM((::GetTickCount()-lpUpDownClient->dwThisClientIsKnownSince)/1000));
			else
				buffer.Format(_T("WHO IS THAT???"));
			dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
			break;
			//SLAHAM: ADDED Known Since <=
		case 11:	// last received
			break;
		case 12:	// category
			break;
		case 13:	// linear priority
			break;
		case 14:	// remaining size
			break;
		//MORPH START - Added by SiRoB, WebCache 1.2f
		//JP Webcache START
		case 15: {
			if (lpUpDownClient->SupportsWebCache())
			{
				buffer = lpUpDownClient->GetWebCacheName();
				if (lpUpDownClient->IsBehindOurWebCache())
					dc->SetTextColor(RGB(0, 180, 0)); //if is behind our webcache display green
				else if (buffer != "")
					dc->SetTextColor(RGB(255, 0, 0)); // if webcache info is there but not our own set red
				else
					buffer = GetResString(IDS_WEBCACHE_NOPROXY);	// if no webcache info colour is black
			}
			else
				buffer = "";
			dc->DrawText(buffer,buffer.GetLength(),const_cast<LPRECT>(lpRect), DLC_DT_TEXT);
 			dc->SetTextColor(RGB(0, 0, 0));
			break;
		}
		//JP Webcache END
		//MORPH END   - Added by SiRoB, WebCache 1.2f
		//MORPH START - Added by SiRoB, IP2Country
		case 16: {
			RECT cur_rec = *lpRect;
			if(theApp.ip2country->ShowCountryFlag()){
				POINT point3= {cur_rec.left,cur_rec.top+1};
				theApp.ip2country->GetFlagImageList()->DrawIndirect(dc, lpUpDownClient->GetCountryFlagIndex(), point3, CSize(18,16), CPoint(0,0), ILD_NORMAL);
				cur_rec.left+=20;
			}
			buffer.Format(_T("%s"), lpUpDownClient->GetCountryName());
			dc->DrawText(buffer,buffer.GetLength(),&cur_rec,DLC_DT_TEXT);
			if(theApp.ip2country->ShowCountryFlag()){
				cur_rec.left-=20;
			}
			break;
		}
		//MORPH END   - Added by SiRoB, IP2Country
		}
	}
}

void CDownloadListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if (!theApp.emuledlg->IsRunning())
		return;
	if (!lpDrawItemStruct->itemData)
		return;
	//MORPH START - Added by SiRoB, Don't draw hidden Rect
	RECT clientRect;
	GetClientRect(&clientRect);
	CRect cur_rec(lpDrawItemStruct->rcItem);
	if (cur_rec.top >= clientRect.bottom || cur_rec.bottom <= clientRect.top)
		return;
	//MORPH END   - Added by SiRoB, Don't draw hidden Rect
	CDC* odc = CDC::FromHandle(lpDrawItemStruct->hDC);
	BOOL bCtrlFocused = ((GetFocus() == this) || (GetStyle() & LVS_SHOWSELALWAYS));
	if (lpDrawItemStruct->itemState & ODS_SELECTED) {
		if (bCtrlFocused)
			odc->SetBkColor(m_crHighlight);
		else
			odc->SetBkColor(m_crNoHighlight);
	}
	else
		odc->SetBkColor(GetBkColor());
	CtrlItem_Struct* content = (CtrlItem_Struct*)lpDrawItemStruct->itemData;
	CMemDC dc(odc, &lpDrawItemStruct->rcItem);
	CFont* pOldFont;
	//MORPH START - Changed by SiRoB, Show Downloading file in bold
	/*
	if (m_fontBold.m_hObject){
	*/
	if (m_fontBold.m_hObject && thePrefs.GetShowActiveDownloadsBold()){
	//MORPH END   - Changed by SiRoB, Show Downloading file in bold
		if (content->type == FILE_TYPE){
			if (((const CPartFile*)content->value)->GetTransferringSrcCount())
				pOldFont = dc.SelectObject(&m_fontBold);
			else
				pOldFont = dc.SelectObject(GetFont());
		}
		else if (content->type == UNAVAILABLE_SOURCE || content->type == AVAILABLE_SOURCE){
			if (((const CUpDownClient*)content->value)->GetDownloadState() == DS_DOWNLOADING)
				pOldFont = dc.SelectObject(&m_fontBold);
			else
				pOldFont = dc.SelectObject(GetFont());
		}
		else
			pOldFont = dc.SelectObject(GetFont());
	}
	else
		pOldFont = dc.SelectObject(GetFont());
	//MORPH - Moved by SiRoB, Don't draw hidden Rect
	/*
	CRect cur_rec(lpDrawItemStruct->rcItem);
	*/
	COLORREF crOldTextColor = dc.SetTextColor((lpDrawItemStruct->itemState & ODS_SELECTED) ? m_crHighlightText : m_crWindowText);

	int iOldBkMode;
	if (m_crWindowTextBk == CLR_NONE){
		DefWindowProc(WM_ERASEBKGND, (WPARAM)(HDC)dc, 0);
		iOldBkMode = dc.SetBkMode(TRANSPARENT);
	}
	else
		iOldBkMode = OPAQUE;

	BOOL notLast = lpDrawItemStruct->itemID + 1 != (UINT)GetItemCount();
	BOOL notFirst = lpDrawItemStruct->itemID != 0;
	int tree_start=0;
	int tree_end=0;

	//offset was 4, now it's the standard 2 spaces
	int iTreeOffset = dc.GetTextExtent(_T(" "), 1 ).cx*2;
	CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	cur_rec.right = cur_rec.left;
	cur_rec.right -= FILE_ITEM_MARGIN_X;
	cur_rec.left += FILE_ITEM_MARGIN_X;

	if (content->type == FILE_TYPE)
	{
		if (!g_bLowColorDesktop || (lpDrawItemStruct->itemState & ODS_SELECTED) == 0) {
			DWORD dwCatColor = thePrefs.GetCatColor(((/*const*/ CPartFile*)content->value)->GetCategory());
			if (dwCatColor > 0)
				dc.SetTextColor(dwCatColor);
			//MORPH START - Added by IceCream, show download in red
			if(thePrefs.GetEnableDownloadInRed() && ((/*const*/ CPartFile*)content->value)->GetTransferringSrcCount())
				dc.SetTextColor(RGB(192,0,0));
			//MORPH END   - Added by IceCream, show download in red
			//MORPH START - Added by FrankyFive, show paused files in gray
			if(((/*const*/ CPartFile*)content->value)->GetStatus() == PS_PAUSED)
				dc.SetTextColor(RGB(128,128,128));
		    //MORPH END   - Added by FrankyFive, show paused files in gray
		}

		for(int iCurrent = 0; iCurrent < iCount; iCurrent++)
		{
			int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
			int cx = CListCtrl::GetColumnWidth(iColumn);
			if(iColumn == 5) {
				int iNextLeft = cur_rec.left + cx;
				//set up tree vars
				cur_rec.left = cur_rec.right + iTreeOffset;
				cur_rec.right = cur_rec.left + min(8, cx);
				tree_start = cur_rec.left + 1;
				tree_end = cur_rec.right;
				//normal column stuff
				cur_rec.left = cur_rec.right + 1;
				cur_rec.right = tree_start + cx - iTreeOffset;
				//MORPH START - Added by SiRoB, Don't draw hidden columns
				if (cur_rec.left < clientRect.right && cur_rec.right > clientRect.left)
				//MORPH END   - Added by SiRoB, Don't draw hidden columns
				DrawFileItem(dc, 5, &cur_rec, content);
				cur_rec.left = iNextLeft;
			} else {
				cur_rec.right += cx;
				//MORPH START - Added by SiRoB, Don't draw hidden columns
				if (cur_rec.left < clientRect.right && cur_rec.right > clientRect.left)
				//MORPH END   - Added by SiRoB, Don't draw hidden columns
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
				cur_rec.left = cur_rec.right + iTreeOffset;
				cur_rec.right = cur_rec.left + min(8, cx);
				tree_start = cur_rec.left + 1;
				tree_end = cur_rec.right;
				//normal column stuff
				cur_rec.left = cur_rec.right + 1;
				cur_rec.right = tree_start + cx - iTreeOffset;
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
	if (content->type == FILE_TYPE && (lpDrawItemStruct->itemState & ODS_SELECTED))
	{
		RECT outline_rec = lpDrawItemStruct->rcItem;

		outline_rec.top--;
		outline_rec.bottom++;
		dc.FrameRect(&outline_rec, &CBrush(GetBkColor()));
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
			dc.FrameRect(&outline_rec, &CBrush(m_crFocusLine));
		else
			dc.FrameRect(&outline_rec, &CBrush(m_crNoFocusLine));
	}
	//draw focus rectangle around non-highlightable items when they have the focus
	else if (((lpDrawItemStruct->itemState & ODS_FOCUS) == ODS_FOCUS) && (GetFocus() == this))
	{
		RECT focus_rec;
		focus_rec.top    = lpDrawItemStruct->rcItem.top;
		focus_rec.bottom = lpDrawItemStruct->rcItem.bottom;
		focus_rec.left   = lpDrawItemStruct->rcItem.left + 1;
		focus_rec.right  = lpDrawItemStruct->rcItem.right - 1;
		dc.FrameRect(&focus_rec, &CBrush(m_crNoFocusLine));
	}

	//draw tree last so it draws over selected and focus (looks better)
	if(tree_start < tree_end) {
		//set new bounds
		RECT tree_rect;
		tree_rect.top    = lpDrawItemStruct->rcItem.top;
		tree_rect.bottom = lpDrawItemStruct->rcItem.bottom;
		tree_rect.left   = tree_start;
		tree_rect.right  = tree_end;
		dc.SetBoundsRect(&tree_rect, DCB_DISABLE);

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
		oldpn = dc.SelectObject(&pn);

		if(isChild) {
			//draw the line to the status bar
			dc.MoveTo(tree_end, middle);
			dc.LineTo(tree_start + 3, middle);

			//draw the line to the child node
			if(hasNext) {
				dc.MoveTo(treeCenter, middle);
				dc.LineTo(treeCenter, cur_rec.bottom + 1);
			}
		} else if(isOpenRoot) {
			//draw circle
			RECT circle_rec;
			COLORREF crBk = dc.GetBkColor();
			circle_rec.top    = middle - 2;
			circle_rec.bottom = middle + 3;
			circle_rec.left   = treeCenter - 2;
			circle_rec.right  = treeCenter + 3;
			dc.FrameRect(&circle_rec, &CBrush(m_crWindowText));
			dc.SetPixelV(circle_rec.left,      circle_rec.top,    crBk);
			dc.SetPixelV(circle_rec.right - 1, circle_rec.top,    crBk);
			dc.SetPixelV(circle_rec.left,      circle_rec.bottom - 1, crBk);
			dc.SetPixelV(circle_rec.right - 1, circle_rec.bottom - 1, crBk);
			//draw the line to the child node
			if(hasNext) {
				dc.MoveTo(treeCenter, middle + 3);
				dc.LineTo(treeCenter, cur_rec.bottom + 1);
			}
		} /*else if(isExpandable) {
			//draw a + sign
			dc.MoveTo(treeCenter, middle - 2);
			dc.LineTo(treeCenter, middle + 3);
			dc.MoveTo(treeCenter - 2, middle);
			dc.LineTo(treeCenter + 3, middle);
		}*/

		//draw the line back up to parent node
		if(notFirst && isChild) {
			dc.MoveTo(treeCenter, middle);
			dc.LineTo(treeCenter, cur_rec.top - 1);
		}

		//put the old pen back
		dc.SelectObject(oldpn);
		pn.DeleteObject();
	}

	//put the original objects back
	if (m_crWindowTextBk == CLR_NONE)
		dc.SetBkMode(iOldBkMode);
	dc.SelectObject(pOldFont);
	dc.SetTextColor(crOldTextColor);
}

void CDownloadListCtrl::HideSources(CPartFile* toCollapse)
{
	SetRedraw(false);
	int pre = 0;
	int post = 0;
	for (int i = 0; i < GetItemCount(); i++)
	{
		CtrlItem_Struct* item = (CtrlItem_Struct*)GetItemData(i);
		if (item->owner == toCollapse)
		{
			pre++;
			item->dwUpdated = 0;
			item->status.DeleteObject();
			DeleteItem(i--);
			post++;
		}
	}
	if (pre - post == 0)
		toCollapse->srcarevisible = false;
	SetRedraw(true);
}

void CDownloadListCtrl::ExpandCollapseItem(int iItem, int iAction, bool bCollapseSource)
{
	if (iItem == -1)
		return;
	CtrlItem_Struct* content = (CtrlItem_Struct*)GetItemData(iItem);

	// to collapse/expand files when one of its source is selected
	if (bCollapseSource && content->parent != NULL)
	{
		content=content->parent;
		
 		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)content;
		iItem = FindItem(&find);
		if (iItem == -1)
			return;
	}

	if (!content || content->type != FILE_TYPE)
		return;
	
	CPartFile* partfile = reinterpret_cast<CPartFile*>(content->value);
	if (!partfile)
		return;

	if (partfile->CanOpenFile()) {
		partfile->OpenFile();
		return;
	}

	// Check if the source branch is disable
	if (!partfile->srcarevisible)
	{
		if (iAction > COLLAPSE_ONLY)
		{
			SetRedraw(false);
			
			// Go throught the whole list to find out the sources for this file
			// Remark: don't use GetSourceCount() => UNAVAILABLE_SOURCE
			for (ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++)
			{
				const CtrlItem_Struct* cur_item = it->second;
				if (cur_item->owner == partfile)
				{
					partfile->srcarevisible = true;
					InsertItem(LVIF_PARAM|LVIF_TEXT, iItem+1, LPSTR_TEXTCALLBACK, 0, 0, 0, (LPARAM)cur_item);
				}
			}

			SetRedraw(true);
		}
	}
	else {
		if (iAction == EXPAND_COLLAPSE || iAction == COLLAPSE_ONLY)
		{
			if (GetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED) != (LVIS_SELECTED | LVIS_FOCUSED))
			{
				SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				SetSelectionMark(iItem);
			}
			HideSources(partfile);
		}
	}
}

void CDownloadListCtrl::OnItemActivate(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (thePrefs.IsDoubleClickEnabled() || pNMIA->iSubItem > 0)
		ExpandCollapseItem(pNMIA->iItem, EXPAND_COLLAPSE);
	*pResult = 0;
}

void CDownloadListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED);
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
            int iFilesGetPreviewParts = 0;
            int iFilesPreviewType = 0;
			int iFilesToPreview = 0;
			int iFilesToCancel = 0;
			int iFileForceA4AF = 0; //MORPH - Added by SiRoB, A4AF
			int iFileForceAllA4AF = 0; //MORPH - Added by SiRoB, A4AF
			int iFileForceA4AFOff = 0; //MORPH - Added by SiRoB, A4AF
			int iFileNotSeenCompleteSource = 0; //MORPH - Added by SiRoB, Only download complete files v2.1 (shadow)
			int	iFilesToImport = 0; //MORPH START - Added by SiRoB, Import Part

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
				iFilesToCancel += pFile->GetStatus() != PS_COMPLETING ? 1 : 0;
				 //MORPH START - Added by SiRoB, Import Part
				iFilesToImport += pFile->GetFileOp() == PFOP_SR13_IMPORTPARTS ? 1 : 0;
				 //MORPH END   - Added by SiRoB, Import Part
				iFilesNotDone += !bFileDone ? 1 : 0;
				iFilesToStop += pFile->CanStopFile() ? 1 : 0;
				iFilesToPause += pFile->CanPauseFile() ? 1 : 0;
				iFilesToResume += pFile->CanResumeFile() ? 1 : 0;
				iFilesToOpen += pFile->CanOpenFile() ? 1 : 0;
                iFilesGetPreviewParts += pFile->GetPreviewPrio() ? 1 : 0;
                iFilesPreviewType += pFile->IsPreviewableFileType() ? 1 : 0;
				iFilesToPreview += pFile->IsReadyForPreview() ? 1 : 0;
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
			if (thePrefs.IsExtControlsEnabled()) {
			    m_SourcesMenu.EnableMenuItem(MP_FORCEA4AF, thePrefs.UseSmartA4AFSwapping() && iSelectedItems == 1 && iFilesNotDone == 1? MF_ENABLED : MF_GRAYED);
			    m_SourcesMenu.CheckMenuItem(MP_FORCEA4AF,  iFileForceAllA4AF > 0 && iSelectedItems == 1 ? MF_CHECKED : MF_UNCHECKED); // todo
			
			    m_SourcesMenu.EnableMenuItem((UINT_PTR)m_A4AFMenuFlag.m_hMenu, iFilesNotDone > 0 && (thePrefs.AdvancedA4AFMode() || thePrefs.UseSmartA4AFSwapping())? MF_ENABLED : MF_GRAYED);
			     m_A4AFMenuFlag.ModifyMenu(MP_FORCEA4AFONFLAG, (iFileForceA4AF > 0 && iSelectedItems == 1 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, MP_FORCEA4AFONFLAG, ((GetSelectedCount() > 1) ? GetResString(IDS_INVERT) + _T(" ") : _T("")) + GetResString(IDS_A4AF_ONFLAG));
			    m_A4AFMenuFlag.ModifyMenu(MP_FORCEA4AFOFFFLAG, (iFileForceA4AFOff > 0 && iSelectedItems == 1 ? MF_CHECKED : MF_UNCHECKED) | MF_STRING, MP_FORCEA4AFOFFFLAG, ((GetSelectedCount() > 1) ? GetResString(IDS_INVERT) + _T(" ") :  _T("")) + GetResString(IDS_A4AF_OFFFLAG));
			}
			// khaos::kmod-

			// enable commands if there is at least one item which can be used for the action
			m_FileMenu.EnableMenuItem(MP_CANCEL, iFilesToCancel > 0 ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_STOP, iFilesToStop > 0 ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_PAUSE, iFilesToPause > 0 ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_RESUME, iFilesToResume > 0 ? MF_ENABLED : MF_GRAYED);

			//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
			m_FileMenu.EnableMenuItem(MP_FORCE,iFileNotSeenCompleteSource > 0 ? MF_ENABLED : MF_GRAYED);
			//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)

			bool bOpenEnabled = (iSelectedItems == 1 && iFilesToOpen == 1);
			m_FileMenu.EnableMenuItem(MP_OPEN, bOpenEnabled ? MF_ENABLED : MF_GRAYED);
            if(thePrefs.IsExtControlsEnabled() && !thePrefs.GetPreviewPrio()) {
			    m_FileMenu.EnableMenuItem(MP_TRY_TO_GET_PREVIEW_PARTS, (iSelectedItems == 1 && iFilesPreviewType == 1 && iFilesToPreview == 0 && iFilesNotDone == 1) ? MF_ENABLED : MF_GRAYED);
			    m_FileMenu.CheckMenuItem(MP_TRY_TO_GET_PREVIEW_PARTS, (iSelectedItems == 1 && iFilesGetPreviewParts == 1) ? MF_CHECKED : MF_UNCHECKED);
            }
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
			m_FileMenu.EnableMenuItem(MP_VIEWFILECOMMENTS, (iSelectedItems >= 1 /*&& iFilesNotDone == 1*/) ? MF_ENABLED : MF_GRAYED);
			
			//MORPH START - Added by SiRoB, Import Parts [SR13]
			m_FileMenu.EnableMenuItem(MP_SR13_ImportParts, (iSelectedItems == 1 && iFilesNotDone == 1) ? MF_ENABLED : MF_GRAYED);
			//CTitleMenu::SetMenuBitmap(MF_STRING, MP_SR13_ImportParts,(iFilesToImport > 0) ? GetResString(IDS_IMPORTPARTS_STOP) :GetResString(IDS_IMPORTPARTS), _T("FILEIMPORTPARTS"));
			
			//m_FileMenu.EnableMenuItem(MP_SR13_InitiateRehash, (iSelectedItems == 1 && iFilesNotDone == 1) ? MF_ENABLED : MF_GRAYED);
			//MORPH END   - Added by SiRoB, Import Parts [SR13]

			int total;
			m_FileMenu.EnableMenuItem(MP_CLEARCOMPLETED, GetCompleteDownloads(curTab, total) > 0 ? MF_ENABLED : MF_GRAYED);

			if (m_SourcesMenu && thePrefs.IsExtControlsEnabled()) {
				m_FileMenu.EnableMenuItem((UINT_PTR)m_SourcesMenu.m_hMenu, MF_ENABLED);
				m_SourcesMenu.EnableMenuItem(MP_ADDSOURCE, (iSelectedItems == 1 && iFilesToStop == 1) ? MF_ENABLED : MF_GRAYED);
				m_SourcesMenu.EnableMenuItem(MP_SETSOURCELIMIT, (iFilesNotDone == iSelectedItems) ? MF_ENABLED : MF_GRAYED);
			}

			m_FileMenu.EnableMenuItem(thePrefs.GetShowCopyEd2kLinkCmd() ? MP_GETED2KLINK : MP_SHOWED2KLINK, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_PASTE, theApp.IsEd2kFileLinkInClipboard() ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_FIND, GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_SEARCHRELATED, iSelectedItems == 1 && theApp.emuledlg->searchwnd->CanSearchRelatedFiles() ? MF_ENABLED : MF_GRAYED);

			//MORPH START - Added by SiRoB, Show Share Permissions
			m_FileMenu.EnableMenuItem((UINT_PTR)m_PermMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
			CString buffer;
			switch (thePrefs.GetPermissions()){
				case PERM_ALL:
				buffer.Format(_T(" (%s)"),GetResString(IDS_FSTATUS_PUBLIC));
					break;
				case PERM_FRIENDS:
				buffer.Format(_T(" (%s)"),GetResString(IDS_FSTATUS_FRIENDSONLY));
					break;
				case PERM_NOONE:
				buffer.Format(_T(" (%s)"),GetResString(IDS_HIDDEN));
					break;
				// Mighty Knife: Community visible filelist
				case PERM_COMMUNITY:
				buffer.Format(_T(" (%s)"),GetResString(IDS_COMMUNITY));
					break;
				// [end] Mighty Knife
				default:
				buffer = _T(" (?)");
					break;
			}
			m_PermMenu.ModifyMenu(MP_PERMDEFAULT, MF_STRING, MP_PERMDEFAULT, GetResString(IDS_DEFAULT) + buffer);
			// Mighty Knife: Community visible filelist
			m_PermMenu.CheckMenuRadioItem(MP_PERMDEFAULT, MP_PERMCOMMUNITY, uPermMenuItem, 0);
			// [end] Mighty Knife
			//MORPH END   - Added by SiRoB, Show Share Permissions

			m_FileMenu.EnableMenuItem(MP_COPYFEEDBACK, iSelectedItems > 0? MF_ENABLED : MF_GRAYED);
			m_FileMenu.EnableMenuItem(MP_COPYFEEDBACK_US, iSelectedItems > 0? MF_ENABLED : MF_GRAYED);
            m_FileMenu.EnableMenuItem(MP_MASSRENAME, iSelectedItems > 0? MF_ENABLED : MF_GRAYED); //Commander - Added: MassRename [Dragon]
	
			CTitleMenu WebMenu;
			WebMenu.CreateMenu();
			WebMenu.AddMenuTitle(NULL, true);
			int iWebMenuEntries = theWebServices.GetFileMenuEntries(&WebMenu);
			UINT flag = (iWebMenuEntries == 0 || iSelectedItems != 1) ? MF_GRAYED : MF_ENABLED;
			m_FileMenu.AppendMenu(MF_POPUP | flag, (UINT_PTR)WebMenu.m_hMenu, GetResString(IDS_WEBSERVICES), _T("WEB"));

			// create cat-submenue
			CMenu CatsMenu;
			CatsMenu.CreateMenu();
			flag = (thePrefs.GetCatCount() == 1) ? MF_GRAYED : MF_ENABLED;
			CString label;
			if (thePrefs.GetCatCount()>1) {
				for (int i = 0; i < thePrefs.GetCatCount(); i++){
					//MORPH START - Changed By SiRoB, Khaos Category
					/*
					if (i>0) {
						label=thePrefs.GetCategory(i)->title;
						label.Replace(_T("&"), _T("&&") );
					}
					CatsMenu.AppendMenu(MF_STRING,MP_ASSIGNCAT+i, (i==0)?GetResString(IDS_CAT_UNASSIGN):label);
					*/
					label=thePrefs.GetCategory(i)->title;
					label.Replace(_T("&"), _T("&&") );
					CatsMenu.AppendMenu(MF_STRING,MP_ASSIGNCAT+i,label);
					//MORPH END   - Changed By SiRoB, Khaos Category
				}
			}
			m_FileMenu.AppendMenu(MF_POPUP | flag, (UINT_PTR)CatsMenu.m_hMenu, GetResString(IDS_TOCAT), _T("CATEGORY"));
			
			// khaos::categorymod+			
			CTitleMenu mnuOrder;
			if (this->GetSelectedCount() > 1) {
				mnuOrder.CreatePopupMenu();
				mnuOrder.AddMenuTitle(GetResString(IDS_CAT_SETORDER));
				mnuOrder.AppendMenu(MF_STRING, MP_CAT_ORDERAUTOINC, GetResString(IDS_CAT_MNUAUTOINC));
				mnuOrder.AppendMenu(MF_STRING, MP_CAT_ORDERSTEPTHRU, GetResString(IDS_CAT_MNUSTEPTHRU));
				mnuOrder.AppendMenu(MF_STRING, MP_CAT_ORDERALLSAME, GetResString(IDS_CAT_MNUALLSAME));
				m_FileMenu.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)mnuOrder.m_hMenu, GetResString(IDS_CAT_SETORDER), _T("FILELINEARPRIO"));
			}
			else {
				m_FileMenu.AppendMenu(MF_STRING, MP_CAT_SETRESUMEORDER, GetResString(IDS_CAT_SETORDER), _T("FILELINEARPRIO"));
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
		else{
			const CUpDownClient* client = (CUpDownClient*)content->value;
			CTitleMenu ClientMenu;
			ClientMenu.CreatePopupMenu();
			ClientMenu.AddMenuTitle(GetResString(IDS_CLIENTS), true);
			ClientMenu.AppendMenu(MF_STRING, MP_DETAIL, GetResString(IDS_SHOWDETAILS), _T("CLIENTDETAILS"));
			ClientMenu.SetDefaultItem(MP_DETAIL);
			ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && !client->IsFriend()) ? MF_ENABLED : MF_GRAYED), MP_ADDFRIEND, GetResString(IDS_ADDFRIEND), _T("ADDFRIEND"));
			//MORPH START - Added by SiRoB, Friend Addon
			ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsFriend()) ? MF_ENABLED : MF_GRAYED), MP_REMOVEFRIEND, GetResString(IDS_REMOVEFRIEND), _T("DELETEFRIEND"));
			ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->IsFriend()) ? MF_ENABLED  | ((!client->HasLowID() && client->IsFriend() && client->GetFriendSlot())?MF_CHECKED : MF_UNCHECKED) : MF_GRAYED), MP_FRIENDSLOT, GetResString(IDS_FRIENDSLOT), _T("FRIENDSLOT"));
			//MORPH END - Added by SiRoB, Friend Addon
			ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient()) ? MF_ENABLED : MF_GRAYED), MP_MESSAGE, GetResString(IDS_SEND_MSG), _T("SENDMESSAGE"));
			ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetViewSharedFilesSupport()) ? MF_ENABLED : MF_GRAYED), MP_SHOWLIST, GetResString(IDS_VIEWFILES), _T("VIEWFILES"));
			if (Kademlia::CKademlia::IsRunning() && !Kademlia::CKademlia::IsConnected())
				ClientMenu.AppendMenu(MF_STRING | ((client && client->IsEd2kClient() && client->GetKadPort()!=0) ? MF_ENABLED : MF_GRAYED), MP_BOOT, GetResString(IDS_BOOTSTRAP));
			ClientMenu.AppendMenu(MF_STRING | (GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED), MP_FIND, GetResString(IDS_FIND), _T("Search"));

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
			ClientMenu.AppendMenu(MF_STRING,MP_LIST_REQUESTED_FILES, GetResString(IDS_LISTREQUESTED), _T("FILEREQUESTED")); // Added by sivka
			//MORPH END - Added by Yun.SF3, List Requested Files

			GetPopupMenuPos(*this, point);
			ClientMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
			
			VERIFY( A4AFMenu.DestroyMenu() );
			VERIFY( ClientMenu.DestroyMenu() );
		}
	}
	else{	// nothing selected
		int total;
		m_FileMenu.EnableMenuItem((UINT_PTR)m_PrioMenu.m_hMenu, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_CANCEL, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_PAUSE, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_STOP, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_RESUME, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_OPEN, MF_GRAYED);

		if (thePrefs.IsExtControlsEnabled() && !thePrefs.GetPreviewPrio()) {
			m_FileMenu.EnableMenuItem(MP_TRY_TO_GET_PREVIEW_PARTS, MF_GRAYED);
			m_FileMenu.CheckMenuItem(MP_TRY_TO_GET_PREVIEW_PARTS, MF_UNCHECKED);
        }
		//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
		m_FileMenu.EnableMenuItem(MP_FORCE,MF_GRAYED);//shadow#(onlydownloadcompletefiles)
		//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
		//MORPH START - Added by SiRoB, ShowPermissions
		m_FileMenu.EnableMenuItem((UINT_PTR)m_PermMenu.m_hMenu, MF_GRAYED);
		//MORPH END   - Added by SiRoB, ShowPermissions
		//MORPH START - Added by SiRoB, copy feedback feature
		m_FileMenu.EnableMenuItem(MP_COPYFEEDBACK, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_COPYFEEDBACK_US, MF_GRAYED);
		//MORPH END   - Added by SiRoB, copy feedback feature
		m_FileMenu.EnableMenuItem(MP_MASSRENAME,MF_GRAYED);//Commander - Added: MassRename
		m_FileMenu.EnableMenuItem(MP_PREVIEW, MF_GRAYED);
		//MORPH START - Added by SiRoB, Import Parts [SR13]
		m_FileMenu.EnableMenuItem(MP_SR13_ImportParts,MF_GRAYED);
		//m_FileMenu.EnableMenuItem(MP_SR13_InitiateRehash,MF_GRAYED);
		//MORPH END   - Added by SiRoB, Import Parts [SR13]
		m_FileMenu.EnableMenuItem(MP_METINFO, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_VIEWFILECOMMENTS, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_CLEARCOMPLETED, GetCompleteDownloads(curTab,total) > 0 ? MF_ENABLED : MF_GRAYED);
		m_FileMenu.EnableMenuItem(thePrefs.GetShowCopyEd2kLinkCmd() ? MP_GETED2KLINK : MP_SHOWED2KLINK, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_PASTE, theApp.IsEd2kFileLinkInClipboard() ? MF_ENABLED : MF_GRAYED);
		m_FileMenu.SetDefaultItem((UINT)-1);
		if (m_SourcesMenu)
			m_FileMenu.EnableMenuItem((UINT_PTR)m_SourcesMenu.m_hMenu, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_SEARCHRELATED, MF_GRAYED);
		m_FileMenu.EnableMenuItem(MP_FIND, GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED);

		// also show the "Web Services" entry, even if its disabled and therefore not useable, it though looks a little 
		// less confusing this way.
		CTitleMenu WebMenu;
		WebMenu.CreateMenu();
		WebMenu.AddMenuTitle(NULL, true);
		theWebServices.GetFileMenuEntries(&WebMenu);
		m_FileMenu.AppendMenu(MF_POPUP | MF_GRAYED, (UINT_PTR)WebMenu.m_hMenu, GetResString(IDS_WEBSERVICES), _T("WEB"));

		GetPopupMenuPos(*this, point);
		m_FileMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
		m_FileMenu.RemoveMenu(m_FileMenu.GetMenuItemCount() - 1, MF_BYPOSITION);
		VERIFY( WebMenu.DestroyMenu() );
	}
}

BOOL CDownloadListCtrl::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
	wParam = LOWORD(wParam);

	switch (wParam)
	{
		case MP_PASTE:
			if (theApp.IsEd2kFileLinkInClipboard())
				theApp.PasteClipboard(curTab);
			return TRUE;
		case MP_FIND:
			OnFindStart();
			return TRUE;
	}

	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel == -1)
		iSel = GetNextItem(-1, LVIS_SELECTED);
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
				case MPG_DELETE: // keyboard del will continue to remove completed files from the screen while cancel will now also be available for complete files
				case MP_CANCEL:
				{
					if (selectedCount > 0)
					{
						SetRedraw(false);
						CString fileList;
						bool validdelete = false;
						bool removecompl = false;
						for (pos = selectedList.GetHeadPosition(); pos != 0; )
						{
							CPartFile* cur_file = selectedList.GetNext(pos);
							if (cur_file->GetStatus() != PS_COMPLETING && (cur_file->GetStatus() != PS_COMPLETE || wParam == MP_CANCEL) ){
								validdelete = true;
								if (selectedCount < 50)
									fileList.Append(_T("\n") + CString(cur_file->GetFileName()));
							}
							else if (cur_file->GetStatus() == PS_COMPLETE)
								removecompl = true;

						}
						CString quest;
						if (selectedCount == 1)
							quest = GetResString(IDS_Q_CANCELDL2);
						else
							quest = GetResString(IDS_Q_CANCELDL);
						if ((removecompl && !validdelete) || (validdelete && AfxMessageBox(quest + fileList, MB_DEFBUTTON2 | MB_ICONQUESTION | MB_YESNO) == IDYES))
						{
							bool bRemovedItems = false;
							while (!selectedList.IsEmpty())
							{
								HideSources(selectedList.GetHead());
								switch (selectedList.GetHead()->GetStatus())
								{
									case PS_WAITINGFORHASH:
									case PS_HASHING:
									case PS_COMPLETING:
										selectedList.RemoveHead();
										bRemovedItems = true;
										break;
									case PS_COMPLETE:
										if (wParam == MP_CANCEL){
											BOOL delsucc = FALSE;
											if (!PathFileExists(selectedList.GetHead()->GetFilePath()))
												delsucc = TRUE;
											else{
												// Delete
												if (!thePrefs.GetRemoveToBin()){
													delsucc = DeleteFile(selectedList.GetHead()->GetFilePath());
												}
												else{
													// delete to recycle bin :(
													TCHAR todel[MAX_PATH+1];
													memset(todel, 0, sizeof todel);
													_tcsncpy(todel, selectedList.GetHead()->GetFilePath(), ARRSIZE(todel)-2);

													SHFILEOPSTRUCT fp = {0};
													fp.wFunc = FO_DELETE;
													fp.hwnd = theApp.emuledlg->m_hWnd;
													fp.pFrom = todel;
													fp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;// | FOF_NOERRORUI
													delsucc = (SHFileOperation(&fp) == 0);
												}
											}
											if (delsucc){
												theApp.sharedfiles->RemoveFile(selectedList.GetHead());
											}
											else{
												CString strError;
												strError.Format( GetResString(IDS_ERR_DELFILE) + _T("\r\n\r\n%s"), selectedList.GetHead()->GetFilePath(), GetErrorMessage(GetLastError()));
												AfxMessageBox(strError);
											}
										}
										RemoveFile(selectedList.GetHead());
										selectedList.RemoveHead();
										bRemovedItems = true;
										break;
									case PS_PAUSED:
										selectedList.GetHead()->DeleteFile();
										selectedList.RemoveHead();
										bRemovedItems = true;
										break;
									default:
										if (selectedList.GetHead()->GetCategory())
											theApp.downloadqueue->StartNextFileIfPrefs(selectedList.GetHead()->GetCategory());
										selectedList.GetHead()->DeleteFile();
										selectedList.RemoveHead();
										bRemovedItems = true;
								}
							}
							if (bRemovedItems)
							{
								AutoSelectItem();
								theApp.emuledlg->transferwnd->UpdateCatTabTitles();
							}
						}
						SetRedraw(true);
					}
					break;
				}
				case MP_PRIOHIGH:
					SetRedraw(false);
					while (!selectedList.IsEmpty()){
						CPartFile* partfile = selectedList.GetHead();
						partfile->SetAutoDownPriority(false);
						partfile->SetDownPriority(PR_HIGH);
						selectedList.RemoveHead();
					}
					SetRedraw(true);
					break;
				case MP_PRIOLOW:
					SetRedraw(false);
					while (!selectedList.IsEmpty()){
						CPartFile* partfile = selectedList.GetHead();
						partfile->SetAutoDownPriority(false);
						partfile->SetDownPriority(PR_LOW);
						selectedList.RemoveHead();
					}
					SetRedraw(true);
					break;
				case MP_PRIONORMAL:
					SetRedraw(false);
					while (!selectedList.IsEmpty()){
						CPartFile* partfile = selectedList.GetHead();
						partfile->SetAutoDownPriority(false);
						partfile->SetDownPriority(PR_NORMAL);
						selectedList.RemoveHead();
					}
					SetRedraw(true);
					break;
				case MP_PRIOAUTO:
					SetRedraw(false);
					while (!selectedList.IsEmpty()){
						CPartFile* partfile = selectedList.GetHead();
						partfile->SetAutoDownPriority(true);
						partfile->SetDownPriority(PR_HIGH);
						selectedList.RemoveHead();
					}
					SetRedraw(true);
					break;
                //Commander - Added: MassRename [Dragon] - Start
				// Mighty Knife: Mass Rename
			case MP_MASSRENAME: {
				CMassRenameDialog MRDialog;
				// Add the files to the dialog
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL) {
					CPartFile*  file = selectedList.GetAt (pos);
					MRDialog.m_FileList.AddTail (file);
					selectedList.GetNext (pos);
				}
				int result = MRDialog.DoModal ();
				if (result == IDOK) {
					// The user has successfully entered new filenames. Now we have
					// to rename all the files...
					POSITION pos = selectedList.GetHeadPosition();
					int i=0;
					while (pos != NULL) {
						CString newname = MRDialog.m_NewFilenames.at (i);
						CString newpath = MRDialog.m_NewFilePaths.at (i);
						CPartFile* file = selectedList.GetAt (pos);
						// .part files could be renamed by simply changing the filename
						// in the CKnownFile object.
						if ((!file->IsPartFile()) && (_trename(file->GetFilePath(), newpath) != 0)){
							// Use the "Format"-Syntax of AddLogLine here instead of
							// CString.Format+AddLogLine, because if "%"-characters are
							// in the string they would be misinterpreted as control sequences!
							AddLogLine(false,_T("Failed to rename '%s' to '%s', Error: %hs"), file->GetFilePath(), newpath, _tcserror(errno));
						} else {
							CString strres;
							if (!file->IsPartFile()) {
								// Use the "Format"-Syntax of AddLogLine here instead of
								// CString.Format+AddLogLine, because if "%"-characters are
								// in the string they would be misinterpreted as control sequences!
								AddLogLine(false,_T("Successfully renamed '%s' to '%s'"), file->GetFilePath(), newpath);
								theApp.sharedfiles->RemoveKeywords(file);
								((CPartFile*) file)->SetFileName(newname); //Changed by SiRoB, change the part file name to be properly displayed after rename
								theApp.sharedfiles->AddKeywords(file);
							} else {
								// Use the "Format"-Syntax of AddLogLine here instead of
								// CString.Format+AddLogLine, because if "%"-characters are
								// in the string they would be misinterpreted as control sequences!
								AddLogLine(false,_T("Successfully renamed .part file '%s' to '%s'"), file->GetFileName(), newname);
								file->SetFileName(newname, true); 
								((CPartFile*) file)->UpdateDisplayedInfo();
								((CPartFile*) file)->SavePartFile(); 
								file->UpdateDisplayedInfo();
							}
						}

						// Next item
						selectedList.GetNext (pos);
						i++;
					}
				}
								}
								break;
				// [end] Mighty Knife
				//Commander - Added: MassRename [Dragon] - End
				case MP_PAUSE:
					SetRedraw(false);
					while (!selectedList.IsEmpty()){
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
					while (!selectedList.IsEmpty()){
						CPartFile *partfile = selectedList.GetHead();
						if (partfile->CanStopFile()){
							HideSources(partfile);
							partfile->StopFile();
						}
						selectedList.RemoveHead();
					}
					SetRedraw(true);
					theApp.emuledlg->transferwnd->UpdateCatTabTitles();
					break;
				case MP_CLEARCOMPLETED:
					SetRedraw(false);
					ClearCompleted();
					SetRedraw(true);
					break;
				case MPG_F2:
					if (GetAsyncKeyState(VK_CONTROL) < 0 || selectedCount > 1) {
						// when ctrl is pressed -> filename cleanup
						if (IDYES==AfxMessageBox(GetResString(IDS_MANUAL_FILENAMECLEANUP),MB_YESNO))
							while (!selectedList.IsEmpty()){
								CPartFile *partfile = selectedList.GetHead();
								if (partfile->IsPartFile()) {
									partfile->SetFileName(CleanupFilename(partfile->GetFileName()));
								}
								selectedList.RemoveHead();
							}
					} else {
						if (file->GetStatus() != PS_COMPLETE && file->GetStatus() != PS_COMPLETING)
						{
							InputBox inputbox;
							CString title = GetResString(IDS_RENAME);
							title.Remove(_T('&'));
							inputbox.SetLabels(title, GetResString(IDS_DL_FILENAME), file->GetFileName());
							inputbox.SetEditFilenameMode();
							if (inputbox.DoModal()==IDOK && !inputbox.GetInput().IsEmpty() && IsValidEd2kString(inputbox.GetInput()))
							{
								file->SetFileName(inputbox.GetInput(), true);
								file->UpdateDisplayedInfo();
								file->SavePartFile();
							}
						}
						else
							MessageBeep(MB_OK);
					}
					break;
				case MP_METINFO:
				case MPG_ALTENTER:
					ShowFileDialog(0);
					break;
				case MP_COPYSELECTED:
				case MP_GETED2KLINK:{
					CString str;
					while (!selectedList.IsEmpty()){
						if (!str.IsEmpty())
							str += _T("\r\n");
						str += CreateED2kLink(selectedList.GetHead());
						selectedList.RemoveHead();
					}
					theApp.CopyTextToClipboard(str);
					break;
				}
				case MP_SEARCHRELATED:
					if (selectedCount > 1)
						break;
					theApp.emuledlg->searchwnd->SearchRelatedFiles(file);
					theApp.emuledlg->SetActiveDialog(theApp.emuledlg->searchwnd);
					break;
				case MP_OPEN:
				case IDA_ENTER:
					if (selectedCount > 1)
						break;
					if (file->CanOpenFile())
					file->OpenFile();
					break;
				case MP_TRY_TO_GET_PREVIEW_PARTS:
					if (selectedCount > 1)
						break;
                    file->SetPreviewPrio(!file->GetPreviewPrio());
                    break;
				case MP_PREVIEW:
					if (selectedCount > 1)
						break;
					file->PreviewFile();
					break;
				case MP_VIEWFILECOMMENTS:
					ShowFileDialog(IDD_COMMENTLST);
					break;
				//MORPH START - Added by SiRoB, Import Parts [SR13]
				case MP_SR13_ImportParts:
					file->SR13_ImportParts();
					break;
				/*
				case MP_SR13_InitiateRehash:
					SR13_InitiateRehash(file);
					break;
				*/
				//MORPH END   - Added by SiRoB, Import Parts [SR13]
				case MP_SHOWED2KLINK:
					ShowFileDialog(IDD_ED2KLINK);
					break;
				case MP_SETSOURCELIMIT: {
					CString temp;
					temp.Format(_T("%u"),file->GetPrivateMaxSources());
					InputBox inputbox;
					CString title = GetResString(IDS_SETPFSLIMIT);
					inputbox.SetLabels(title, GetResString(IDS_SETPFSLIMITEXPLAINED), temp );

					if (inputbox.DoModal()==IDOK  )
					{
						temp=inputbox.GetInput();
						int newlimit = _tstoi(temp);
						while (!selectedList.IsEmpty()){
							CPartFile *partfile = selectedList.GetHead();
							partfile->SetPrivateMaxSources(newlimit);
							selectedList.RemoveHead();
							partfile->UpdateDisplayedInfo(true);
						}
					}
					break;
				}
				case MP_ADDSOURCE: {
					if (selectedCount > 1)
						break;
					CAddSourceDlg as;
					as.SetFile(file);
					as.DoModal();
					break;
				}
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
					feed.AppendFormat(GetResString(IDS_FEEDBACK_FROM), thePrefs.GetUserNick(), theApp.m_strModLongVersion);
					feed.Append(_T(" \r\n"));
					POSITION pos = selectedList.GetHeadPosition();
					while (pos != NULL)
					{			
						CKnownFile* file = selectedList.GetNext(pos);
						feed.Append(file->GetFeedback());
						feed.Append(_T("\r\n"));
					}
					//Todo: copy all the comments too
					theApp.CopyTextToClipboard(feed);
					break;
				}
				case MP_COPYFEEDBACK_US:
				{
					CString feed;
					feed.AppendFormat(_T("Feedback from %s on [%s]\r\n"),thePrefs.GetUserNick(),theApp.m_strModLongVersion);
					POSITION pos = selectedList.GetHeadPosition();
					while (pos != NULL)
					{
						CKnownFile* file = selectedList.GetNext(pos);
						feed.Append(file->GetFeedback(true));
						feed.Append(_T("\r\n"));
					}
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

					currOrder.Format(_T("%u"), file->GetCatResumeOrder());
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
						
					inputOrder.SetLabels(GetResString(IDS_CAT_SETORDER), GetResString(IDS_CAT_EXPAUTOINC), _T("0"));
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
						currOrder.Format(_T("%u"), selectedList.GetHead()->GetCatResumeOrder());
						currFile = selectedList.GetHead()->GetFileName();
                        if (currFile.GetLength() > 50) currFile = currFile.Mid(0,47) + _T("...");
						currInstructions.Format(_T("%s %s"), GetResString(IDS_CAT_EXPSTEPTHRU), currFile);
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

					inputOrder.SetLabels(GetResString(IDS_CAT_SETORDER), GetResString(IDS_CAT_EXPALLSAME), _T("0"));
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
						UpdateCurrentCategoryView();
						if (thePrefs.ShowCatTabInfos())
							theApp.emuledlg->transferwnd->UpdateCatTabTitles();
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
						theApp.friendlist->ShowFriends();
						UpdateItem(client);
					}
					//MORPH END - Modified by SIRoB, Added by Yun.SF3, ZZ Upload System
					break;
				}
				case MP_REMOVEFRIEND:{//LSD
					if (client && client->IsFriend())
					{					
						theApp.friendlist->RemoveFriend(client->m_Friend);
						UpdateItem(client);
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
				case MP_DETAIL:
				case MPG_ALTENTER:
					ShowClientDialog(client);
					break;
				case MP_BOOT:
					if (client->GetKadPort())
						Kademlia::CKademlia::Bootstrap(ntohl(client->GetIP()), client->GetKadPort());
					break;
// ZZ:DownloadManager -->
				case MP_A4AF_CHECK_THIS_NOW:
					if (file->GetStatus(false) == PS_READY || file->GetStatus(false) == PS_EMPTY)
					{
						if (client->GetDownloadState() != DS_DOWNLOADING)
						{
							client->SwapToAnotherFile(_T("Manual init of source check. Test to be like ProcessA4AFClients(). CDownloadListCtrl::OnCommand() MP_SWAP_A4AF_DEBUG_THIS"), false, false, false, NULL, true, true, true); // ZZ:DownloadManager
							UpdateItem(file);
						}
					}
					break;
// <-- ZZ:DownloadManager
				//MORPH START - Added by Yun.SF3, List Requested Files
				case MP_LIST_REQUESTED_FILES: { // added by sivka
					if (client != NULL)
						client->ShowRequestedFiles(); //Changed by SiRoB
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
	int sortItem = GetSortItem();
	bool m_oldSortAscending = GetSortAscending();
	int userSort = (GetAsyncKeyState(VK_CONTROL) < 0) ? 64:0;	// SLUGFILLER: DLsortFix Ctrl sorts sources only

	//MORPH START - Removed by SiRoB, Remain time and size Columns have been splited
	/*
	if (sortItem==9) {
		m_bRemainSort=(sortItem != pNMListView->iSubItem) ? false : (m_oldSortAscending?m_bRemainSort:!m_bRemainSort);
	}
	*/
	//MORPH END   - Removed by SiRoB, Remain time and size Columns have been splited
	bool sortAscending = (sortItem != pNMListView->iSubItem + userSort) ? (pNMListView->iSubItem == 0) : !m_oldSortAscending;	// SLUGFILLER: DLsortFix - descending by default for all but filename/username

	// Item is column clicked
	sortItem = pNMListView->iSubItem + userSort;	//MORPH - Changed by SiRoB, DLsortFix Ctrl sorts sources only
	UpdateSortHistory(sortItem + (sortAscending ? 0:100), 100);

	// Save new preferences
	//MORPH START - Changed by SiRoB, Remain time and size Columns have been splited
	/*
	thePrefs.TransferlistRemainSortStyle(m_bRemainSort);
	
	// Sort table
	uint8 adder=0;
	if (sortItem!=9 || !m_bRemainSort)
		SetSortArrow(sortItem, sortAscending);
	else {
        SetSortArrow(sortItem, sortAscending?arrowDoubleUp : arrowDoubleDown);
		adder=81;
	}
	

	SortItems(SortProc, sortItem + (sortAscending ? 0:100) + adder );
	*/
	SetSortArrow(sortItem, sortAscending);
	SortItems(SortProc, sortItem + (sortAscending ? 0:100));
	//MORPH END  - Changed by SiRoB, Remain time and size Columns have been splited
	*pResult = 0;
}

int CDownloadListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){
	CtrlItem_Struct* item1 = (CtrlItem_Struct*)lParam1;
	CtrlItem_Struct* item2 = (CtrlItem_Struct*)lParam2;

	int dwOrgSort = lParamSort;

	int sortMod = 1;
	if(lParamSort >= 100) 
	{
		sortMod = -1;
		lParamSort -= 100;
	}

	int comp;

	if(item1->type == FILE_TYPE && item2->type != FILE_TYPE) 
	{
		if(item1->value == item2->parent->value)
			return -1;
		comp = Compare((CPartFile*)item1->value, (CPartFile*)(item2->parent->value), lParamSort);
	}
	else if(item2->type == FILE_TYPE && item1->type != FILE_TYPE) 
	{
		if(item1->parent->value == item2->value)
			return 1;
		comp = Compare((CPartFile*)(item1->parent->value), (CPartFile*)item2->value, lParamSort);
	}
	else if (item1->type == FILE_TYPE) 
	{
		CPartFile* file1 = (CPartFile*)item1->value;
		CPartFile* file2 = (CPartFile*)item2->value;
		comp = Compare(file1, file2, lParamSort);
	}
	else
	{
		if (item1->parent->value != item2->parent->value)
			return SortProc((LPARAM)item1->parent, (LPARAM)item2->parent, (LPARAM)dwOrgSort);	// SLUGFILLER: DLsortFix - preserve recursion
		if (item1->type != item2->type)
			return item1->type - item2->type;

		CUpDownClient* client1 = (CUpDownClient*)item1->value;
		CUpDownClient* client2 = (CUpDownClient*)item2->value;
		comp = Compare(client1, client2, lParamSort);
	}

	// SLUGFILLER: DLsortFix - last-chance sort, detect and use
	if (comp == 0 && dwOrgSort != 99 && theApp.emuledlg->transferwnd->downloadlistctrl.GetNextSortOrder(dwOrgSort) == -1)
		return SortProc(lParam1, lParam2, 99);	// Perhaps a bit hackity, but we only have one last-chance, and this beats saving it in the preferences
	// SLUGFILLER: DLsortFix

	return sortMod * comp;
}

void CDownloadListCtrl::ClearCompleted(int incat){
	if (incat==-2)
		incat=curTab;

	// Search for completed file(s)
	for(ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); ){
		CtrlItem_Struct* cur_item = it->second;
		it++; // Already point to the next iterator. 
		if(cur_item->type == FILE_TYPE){
			CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);
			if(file->IsPartFile() == false && (file->CheckShowItemInGivenCat(incat) || incat==-1) ){
				if (RemoveFile(file))
					it = m_ListItems.begin();
			}
		}
	}
	if (thePrefs.ShowCatTabInfos())
		theApp.emuledlg->transferwnd->UpdateCatTabTitles();
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

void CDownloadListCtrl::SetStyle()
{
	if (thePrefs.IsDoubleClickEnabled())
		SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	else
		SetExtendedStyle(LVS_EX_ONECLICKACTIVATE | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
}

void CDownloadListCtrl::OnListModified(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	NM_LISTVIEW *pNMListView = (NM_LISTVIEW*)pNMHDR;

	//this works because true is equal to 1 and false equal to 0
	BOOL notLast = pNMListView->iItem + 1 != GetItemCount();
	BOOL notFirst = pNMListView->iItem != 0;
	RedrawItems(pNMListView->iItem - notFirst, pNMListView->iItem + notLast);
}

int CDownloadListCtrl::Compare(const CPartFile* file1, const CPartFile* file2, LPARAM lParamSort)
{
	// SLUGFILLER: DLsortFix - client-only sorting
	if (lParamSort > 63)
		return 0;
	// SLUGFILLER: DLsortFix
	int comp=0;
	switch(lParamSort)
	{
		case 0: //filename asc
			comp=CompareLocaleStringNoCase(file1->GetFileName(),file2->GetFileName());
			break;
		case 1: //size asc
			comp=CompareUnsigned64(file1->GetFileSize(), file2->GetFileSize());
			break;
		case 2: //transferred asc
			comp=CompareUnsigned64(file1->GetTransferred(), file2->GetTransferred());
			break;
		case 3: //completed asc
			comp=CompareUnsigned64(file1->GetCompletedSize(), file2->GetCompletedSize());
			break;
		case 4: //speed asc
			comp=CompareUnsigned(file1->GetDatarate(), file2->GetDatarate());
			break;
		case 5: //progress asc
			comp = CompareFloat(file1->GetPercentCompleted(), file2->GetPercentCompleted());
			break;
		case 6: //sources asc
			comp=CompareUnsigned(file1->GetSourceCount(), file2->GetSourceCount());
			break;
		case 7: //priority asc
			comp=CompareUnsigned(file1->GetDownPriority(), file2->GetDownPriority());
			break;
		case 8: //Status asc 
			//MORPH 
			/*
			comp=CompareUnsigned(file1->getPartfileStatusRang(),file2->getPartfileStatusRang());
			*/
			comp=CompareUnsigned(file2->getPartfileStatusRang(),file1->getPartfileStatusRang());
			break;
		case 9: //Remaining Time asc
		{
			//Make ascending sort so we can have the smaller remaining time on the top 
			//instead of unknowns so we can see which files are about to finish better..
			//MORPH - Changed by SiRoB, Sort by AverageRemainingTime
			/*
			time_t f1 = file1->getTimeRemaining();
			time_t f2 = file2->getTimeRemaining();
			*/
			time_t f1 = (thePrefs.GetTimeRemainingMode()!=2)?file1->getTimeRemaining():file1->GetTimeRemainingAvg();
			time_t f2 = (thePrefs.GetTimeRemainingMode()!=2)?file2->getTimeRemaining():file2->GetTimeRemainingAvg();
			//Same, do nothing.
			if( f1 == f2 ) 
			{
				comp=0;
				break;
			}
			//If descending, put first on top as it is unknown
			//If ascending, put first on bottom as it is unknown
			if( f1 == -1 ) 
			{
				comp=1;
				break;
			}
			//If descending, put second on top as it is unknown
			//If ascending, put second on bottom as it is unknown
			if( f2 == -1 ) 
			{
				comp=-1;
				break;
			}
			//If descending, put first on top as it is bigger.
			//If ascending, put first on bottom as it is bigger.
			comp = CompareUnsigned(f1, f2);
			break;
		}
		//MORPH - Removed by SiRoB, Remain time and size Columns have been splited
		/*
		case 90: //Remaining SIZE asc
			comp=CompareUnsigned64(file1->GetFileSize()-file1->GetCompletedSize(), file2->GetFileSize()-file2->GetCompletedSize());
			break;
		*/
		case 10: //last seen complete asc
			if (file1->lastseencomplete > file2->lastseencomplete)
				comp=1;
			else if(file1->lastseencomplete < file2->lastseencomplete)
				comp=-1;
			else
				comp=0;
			break;
		case 11: //last received Time asc
			if (file1->GetFileDate() > file2->GetFileDate())
				comp=1;
			else if(file1->GetFileDate() < file2->GetFileDate())
				comp=-1;
			else
				comp=0;
			break;
		// khaos::categorymod+
		case 12: // Cat
			if (file1->GetCategory() > file2->GetCategory())
				comp=1;
			else if (file1->GetCategory() < file2->GetCategory())
				comp=-1;
			else
				comp=0;
			break;
		case 13: // Mod
			if (CPartFile::RightFileHasHigherPrio(file2, file1))
				comp=1;
			else if (CPartFile::RightFileHasHigherPrio(file1, file2))
				comp=-1;
			else
				comp=0;
			break;
		case 14: // Remaining Bytes
			if ((file1->GetFileSize() - file1->GetCompletedSize()) > (file2->GetFileSize() - file2->GetCompletedSize()))
				comp=1;
			else if ((file1->GetFileSize() - file1->GetCompletedSize()) < (file2->GetFileSize() - file2->GetCompletedSize()))
				comp=-1;
			else
				comp=0;
			break;
		// khaos::categorymod-
		//MORPH START - Added by SiRoB, WebCache 1.2f
		//JP Webcache
		case 15:
			comp=file1->GetWebcacheSourceCount() - file2->GetWebcacheSourceCount();
			break;
		//MORPH END   - Added by SiRoB, WebCache 1.2f
		// SLUGFILLER: DLsortFix
		case 99:	// met file name asc, only available as last-resort sort to make sure no two files are equal
			comp=CompareLocaleStringNoCase(file1->GetFullName(), file2->GetFullName());
		// SLUGFILLER: DLsortFix
		default:
			comp=0;
	}
	return comp;
}

int CDownloadListCtrl::Compare(const CUpDownClient *client1, const CUpDownClient *client2, LPARAM lParamSort)
{
	lParamSort &= 63;	// SLUGFILLER: DLsortFix
	switch (lParamSort)
	{
	case 0: //name asc
		if (client1->GetUserName() == client2->GetUserName())
			return 0;
		else if (!client1->GetUserName())	// place clients with no usernames at bottom
			return 1;
		else if (!client2->GetUserName())	// place clients with no usernames at bottom
			return -1;
		return CompareLocaleStringNoCase(client1->GetUserName(), client2->GetUserName());

	case 1: //size but we use status asc
		return client1->GetSourceFrom() - client2->GetSourceFrom();

	case 2://transferred asc
		//MORPH START - Added By SiRoB, Download/Upload
		if (!client1->Credits())
			return 1;
		else if (!client2->Credits())
			return -1;
		return CompareUnsigned64(client2->Credits()->GetDownloadedTotal(), client1->Credits()->GetDownloadedTotal());
		//MORPH END - Added By SiRoB, Download/Upload
	case 3://completed asc
		return CompareUnsigned(client1->GetTransferredDown(), client2->GetTransferredDown());

	case 4: //speed asc
		return CompareUnsigned(client1->GetDownloadDatarate(), client2->GetDownloadDatarate());

	case 5: //progress asc
		return CompareUnsigned(client1->GetAvailablePartCount(), client2->GetAvailablePartCount());

	case 6:
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
		// Maella -Support for tag ET_MOD_VERSION 0x55-
		if (client1->GetClientSoft() == client2->GetClientSoft())
			if(client2->GetVersion() == client1->GetVersion() && client1->GetClientSoft() == SO_EMULE){
				return CompareOptLocaleStringNoCase(client2->GetClientSoftVer(), client1->GetClientSoftVer());
			}
			else {
			return client2->GetVersion() - client1->GetVersion();
			}
		else
		return client1->GetClientSoft() - client2->GetClientSoft();
		// Maella end

//EastShare START - Modified by TAHO, @@
/*
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
*/
		case 7: {//priority asc
			uint32 lastAskedTime1 = client1->GetLastAskedTime();
			uint32 lastAskedTime2 = client2->GetLastAskedTime();
			if ( lastAskedTime1 != 0){
				if ( lastAskedTime2 != 0){
					if (lastAskedTime1 > lastAskedTime2){
						return 1;
					} else if (lastAskedTime1 < lastAskedTime2) {
						return -1;
					} else {
						return 0;
					}
				}
				return 1;
			}
			else
				return (lastAskedTime2 == 0) ? -1 : 0;
		}
		case 8: {//Status asc
			EDownloadState clientState1 = client1->GetDownloadState();
			EDownloadState clientState2 = client2->GetDownloadState();

			if ( clientState1 == DS_DOWNLOADING ){
				if ( clientState2 == DS_DOWNLOADING) {
					return CompareUnsigned(client1->GetDownloadDatarate(), client2->GetDownloadDatarate());
				}
				return 1;
			} else if ( clientState2 == DS_DOWNLOADING) {
				return -1;
			}

			if ( clientState1 == DS_ONQUEUE ){
				if ( clientState2 == DS_ONQUEUE ) {
					if ( client1->IsRemoteQueueFull() ){
						return (client2->IsRemoteQueueFull()) ? 0 : -1;
					}
					else if ( client2->IsRemoteQueueFull() ){
						return 1;
					}

					if ( client1->GetRemoteQueueRank() ){
						return (client2->GetRemoteQueueRank()) ? client2->GetRemoteQueueRank() - client1->GetRemoteQueueRank() : 1;
					}
					return (client2->GetRemoteQueueRank()) ? -1 : 0;
				}
				return 1;
			} else if ( clientState2 == DS_ONQUEUE ){
				return -1;
			}

			if ( clientState1 == DS_NONEEDEDPARTS && clientState2 != DS_NONEEDEDPARTS)
				return 1;

			if ( clientState1 == DS_TOOMANYCONNS && clientState2 != DS_TOOMANYCONNS)
				return -1;

			return 0;
		}

//EastShare END - Modified by TAHO, @@
			//SLAHAM: ADDED Show Downloading Time =>
	case 9: 
		return client1->dwSessionDLTime - client2->dwSessionDLTime;
		//SLAHAM: ADDED Show Downloading Time <=
		//SLAHAM: ADDED Known Since =>
	case 10:
		{
			uint32 known1 = client2->dwThisClientIsKnownSince;
			uint32 known2 = client1->dwThisClientIsKnownSince;
			return known1==known2? 0 : known1<known2? -1 : 1;
		}
	//SLAHAM: ADDED Known Since <=
	//MORPH START - Added by SiRoB, WebCache 1.2f
	//JP Webcache START 
	case 15:
		if (client1->SupportsWebCache() && client2->SupportsWebCache() )
		return CompareLocaleStringNoCase(client1->GetWebCacheName(),client2->GetWebCacheName());
		else
			return client1->SupportsWebCache() - client2->SupportsWebCache();
	//JP Webcache END
	//MORPH END   - Added by SiRoB, WebCache 1.2f
	// Commander - Added: IP2Country column - Start
    case 16:
		if(client1->GetCountryName(true) && client2->GetCountryName(true))
			return CompareLocaleStringNoCase(client1->GetCountryName(true), client2->GetCountryName(true));
		else if(client1->GetCountryName(true))
			return 1;
		else
			return -1;
	// Commander - Added: IP2Country column - End
		
	default:
		return 0;
	}
}

void CDownloadListCtrl::OnNMDblclkDownloadlist(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	int iSel = GetSelectionMark();
	if (iSel != -1)
	{
		const CtrlItem_Struct* content = (CtrlItem_Struct*)GetItemData(iSel);
		if (content && content->value)
		{
			if (content->type == FILE_TYPE)
			{
				if (!thePrefs.IsDoubleClickEnabled())
				{
					CPoint pt;
					::GetCursorPos(&pt);
					ScreenToClient(&pt);
					LVHITTESTINFO hit;
					hit.pt = pt;
					if (HitTest(&hit) >= 0 && (hit.flags & LVHT_ONITEM))
					{
						LVHITTESTINFO subhit;
						subhit.pt = pt;
						if (SubItemHitTest(&subhit) >= 0 && subhit.iSubItem == 0)
						{
							CPartFile* file = (CPartFile*)content->value;
							if (thePrefs.ShowRatingIndicator() 
								&& (file->HasComment() || file->HasRating()) 
								&& pt.x >= FILE_ITEM_MARGIN_X+theApp.GetSmallSytemIconSize().cx 
								&& pt.x <= FILE_ITEM_MARGIN_X+theApp.GetSmallSytemIconSize().cx+RATING_ICON_WIDTH)
								ShowFileDialog(IDD_COMMENTLST);
							else if (thePrefs.GetPreviewOnIconDblClk()
									 && pt.x >= FILE_ITEM_MARGIN_X 
									 && pt.x < FILE_ITEM_MARGIN_X+theApp.GetSmallSytemIconSize().cx) {
								if (file->IsReadyForPreview())
									file->PreviewFile();
								else
									MessageBeep(MB_OK);
							}
							else
								ShowFileDialog(0);
						}
					}
				}
			}
			else
			{
				ShowClientDialog((CUpDownClient*)content->value);
			}
		}
	}
	
	*pResult = 0;
}

void CDownloadListCtrl::CreateMenues()
{
	if (m_PrioMenu)
		VERIFY( m_PrioMenu.DestroyMenu() );
	if (m_SourcesMenu)
		VERIFY( m_SourcesMenu.DestroyMenu() );
	if (m_PermMenu)	VERIFY( m_PermMenu.DestroyMenu() );	// xMule_MOD: showSharePermissions
	//MORPH START - Added by SiRoB, Advanced A4AF Flag derivated from Khaos
	if (m_A4AFMenuFlag)
		VERIFY( m_A4AFMenuFlag.DestroyMenu() );
	//MORPH END   - Added by SiRoB, Advanced A4AF Flag derivated from Khaos
	if (m_FileMenu)
		VERIFY( m_FileMenu.DestroyMenu() );

	m_FileMenu.CreatePopupMenu();
	m_FileMenu.AddMenuTitle(GetResString(IDS_DOWNLOADMENUTITLE), true);

	// Add 'Download Priority' sub menu
	//
	m_PrioMenu.CreateMenu();
	m_PrioMenu.AddMenuTitle(NULL, true);
	m_PrioMenu.AppendMenu(MF_STRING, MP_PRIOLOW, GetResString(IDS_PRIOLOW));
	m_PrioMenu.AppendMenu(MF_STRING, MP_PRIONORMAL, GetResString(IDS_PRIONORMAL));
	m_PrioMenu.AppendMenu(MF_STRING, MP_PRIOHIGH, GetResString(IDS_PRIOHIGH));
	m_PrioMenu.AppendMenu(MF_STRING, MP_PRIOAUTO, GetResString(IDS_PRIOAUTO));

	// xMule_MOD: showSharePermissions
	m_PermMenu.CreateMenu();
	m_PermMenu.AddMenuTitle(NULL, true);
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMDEFAULT,	GetResString(IDS_DEFAULT));
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMNONE,	GetResString(IDS_HIDDEN));
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMFRIENDS,	GetResString(IDS_FSTATUS_FRIENDSONLY));
	// Mighty Knife: Community visible filelist
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMCOMMUNITY,GetResString(IDS_COMMUNITY));
	// [end] Mighty Knife
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMALL,		GetResString(IDS_FSTATUS_PUBLIC));
	// xMule_MOD: showSharePermissions

	m_FileMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PermMenu.m_hMenu, GetResString(IDS_PERMISSION), _T("FILEPERMISSION"));	// xMule_MOD: showSharePermissions
	m_FileMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PrioMenu.m_hMenu, GetResString(IDS_PRIORITY) + _T(" (") + GetResString(IDS_DOWNLOAD) + _T(")"), _T("FILEPRIORITY"));

	// Add file commands
	//
	m_FileMenu.AppendMenu(MF_STRING, MP_PAUSE, GetResString(IDS_DL_PAUSE), _T("PAUSE"));
	m_FileMenu.AppendMenu(MF_STRING, MP_STOP, GetResString(IDS_DL_STOP), _T("STOP"));
	m_FileMenu.AppendMenu(MF_STRING, MP_RESUME, GetResString(IDS_DL_RESUME), _T("RESUME"));
	m_FileMenu.AppendMenu(MF_STRING, MP_CANCEL, GetResString(IDS_MAIN_BTN_CANCEL), _T("DELETE"));
	//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
	if(thePrefs.OnlyDownloadCompleteFiles())
		m_FileMenu.AppendMenu(MF_STRING,MP_FORCE, GetResString(IDS_DL_FORCE));//shadow#(onlydownloadcompletefiles)
	//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	m_FileMenu.AppendMenu(MF_STRING, MP_OPEN, GetResString(IDS_DL_OPEN), _T("OPENFILE"));
	if (thePrefs.IsExtControlsEnabled() && !thePrefs.GetPreviewPrio())
    	m_FileMenu.AppendMenu(MF_STRING, MP_TRY_TO_GET_PREVIEW_PARTS, GetResString(IDS_DL_TRY_TO_GET_PREVIEW_PARTS), _T("FILEDOWNLOADPREVIEWFIRST"));
	m_FileMenu.AppendMenu(MF_STRING, MP_PREVIEW, GetResString(IDS_DL_PREVIEW), _T("PREVIEW"));
	m_FileMenu.AppendMenu(MF_STRING, MP_METINFO, GetResString(IDS_DL_INFO), _T("FILEINFO"));
	m_FileMenu.AppendMenu(MF_STRING,MP_VIEWFILECOMMENTS, GetResString(IDS_CMT_SHOWALL), _T("FILECOMMENTS"));
	//MORPH START - Added by SiRoB, Import Parts [SR13]
	m_FileMenu.AppendMenu(MF_STRING,MP_SR13_ImportParts, GetResString(IDS_IMPORTPARTS), _T("FILEIMPORTPARTS"));
 	//m_FileMenu.AppendMenu(MF_STRING,MP_SR13_InitiateRehash, GetResString(IDS_INITIATEREHASH), _T("FILEINITIATEREHASH"));
	//MORPH END   - Added by SiRoB, Import Parts [SR13]
	if (thePrefs.IsExtControlsEnabled()) m_FileMenu.AppendMenu(MF_STRING,MP_MASSRENAME, GetResString(IDS_MR), _T("FILEMASSRENAME"));//Commander - Added: MassRename [Dragon]
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	m_FileMenu.AppendMenu(MF_STRING, MP_CLEARCOMPLETED, GetResString(IDS_DL_CLEAR), _T("CLEARCOMPLETE"));

	// Add (extended user mode) 'Source Handling' sub menu
	//
	if (thePrefs.IsExtControlsEnabled()) {
		m_SourcesMenu.CreateMenu();
		m_SourcesMenu.AddMenuTitle(NULL, true);
		m_SourcesMenu.AppendMenu(MF_STRING, MP_ADDSOURCE, GetResString(IDS_ADDSRCMANUALLY), _T("FILEADDSRC"));
		m_SourcesMenu.AppendMenu(MF_STRING, MP_SETSOURCELIMIT, GetResString(IDS_SETPFSLIMIT));
		//MORPH START - Added by SiRoB, Advanced A4AF Flag derivated from Khaos
		m_A4AFMenuFlag.CreateMenu();
		m_A4AFMenuFlag.AppendMenu(MF_STRING, MP_FORCEA4AFONFLAG, GetResString(IDS_A4AF_ONFLAG));
		m_A4AFMenuFlag.AppendMenu(MF_STRING, MP_FORCEA4AFOFFFLAG, GetResString(IDS_A4AF_OFFFLAG));

		m_SourcesMenu.AppendMenu(MF_SEPARATOR);
		m_SourcesMenu.AppendMenu(MF_STRING, MP_FORCEA4AF, GetResString(IDS_A4AF_FORCEALL));
		m_SourcesMenu.AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_A4AFMenuFlag.m_hMenu, GetResString(IDS_A4AF_FLAGS), _T("ADVA4AF"));
		//MORPH END   - Added by SiRoB, Advanced A4AF Flag derivated from Khaos
		m_FileMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_SourcesMenu.m_hMenu, GetResString(IDS_A4AF));
	}
	m_FileMenu.AppendMenu(MF_SEPARATOR);

	// Add 'Copy & Paste' commands
	//
	if (thePrefs.GetShowCopyEd2kLinkCmd())
		m_FileMenu.AppendMenu(MF_STRING, MP_GETED2KLINK, GetResString(IDS_DL_LINK1), _T("ED2KLINK"));
	else
		m_FileMenu.AppendMenu(MF_STRING, MP_SHOWED2KLINK, GetResString(IDS_DL_SHOWED2KLINK), _T("ED2KLINK"));
	m_FileMenu.AppendMenu(MF_STRING, MP_PASTE, GetResString(IDS_SW_DIRECTDOWNLOAD), _T("PASTELINK"));
	m_FileMenu.AppendMenu(MF_SEPARATOR);

	//MORPH START - Added by IceCream, copy feedback feature
	m_FileMenu.AppendMenu(MF_STRING,MP_COPYFEEDBACK, GetResString(IDS_COPYFEEDBACK), _T("COPY"));
	m_FileMenu.AppendMenu(MF_STRING,MP_COPYFEEDBACK_US, GetResString(IDS_COPYFEEDBACK_US), _T("COPY"));
	m_FileMenu.AppendMenu(MF_SEPARATOR);
	//MORPH END   - Added by IceCream, copy feedback feature

	// Search commands
	//
	m_FileMenu.AppendMenu(MF_STRING, MP_FIND, GetResString(IDS_FIND), _T("Search"));
	m_FileMenu.AppendMenu(MF_STRING, MP_SEARCHRELATED, GetResString(IDS_SEARCHRELATED), _T("KadFileSearch"));
	// Web-services and categories will be added on-the-fly..
}

CString CDownloadListCtrl::getTextList()
{
	CString out;

	for (ListItems::iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++)
	{
		const CtrlItem_Struct* cur_item = it->second;
		if (cur_item->type == FILE_TYPE)
		{
			const CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);

			CString temp;
			temp.Format(_T("\n%s\t [%.1f%%] %i/%i - %s"),
						file->GetFileName(),
						file->GetPercentCompleted(),
						file->GetTransferringSrcCount(),
						file->GetSourceCount(), 
						file->getPartfileStatus());

			out += temp;
		}
	}

	return out;
}

int CDownloadListCtrl::GetFilesCountInCurCat()
{
	int iCount = 0;
	for (ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++)
	{
		CtrlItem_Struct* cur_item = it->second;
		if (cur_item->type == FILE_TYPE)
		{
			CPartFile* file = (CPartFile*)cur_item->value;
			if (file->CheckShowItemInGivenCat(curTab))
				iCount++;
		}
	}
	return iCount;
}

void CDownloadListCtrl::ShowFilesCount()
{
	theApp.emuledlg->transferwnd->UpdateFilesCount(GetFilesCountInCurCat());
}

void CDownloadListCtrl::ShowSelectedFileDetails()
{
	POINT point;
	::GetCursorPos(&point);
	CPoint pt = point; 
    ScreenToClient(&pt); 
    int it = HitTest(pt);
    if (it == -1)
		return;

	SetItemState(-1, 0, LVIS_SELECTED);
	SetItemState(it, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	SetSelectionMark(it);   // display selection mark correctly! 

	CtrlItem_Struct* content = (CtrlItem_Struct*)GetItemData(GetSelectionMark());
	if (content->type == FILE_TYPE)
	{
		CPartFile* file = (CPartFile*)content->value;
		if (thePrefs.ShowRatingIndicator() 
			&& (file->HasComment() || file->HasRating()) 
			&& pt.x >= FILE_ITEM_MARGIN_X+theApp.GetSmallSytemIconSize().cx 
			&& pt.x <= FILE_ITEM_MARGIN_X+theApp.GetSmallSytemIconSize().cx+RATING_ICON_WIDTH)
			ShowFileDialog(IDD_COMMENTLST);
		else
			ShowFileDialog(0);
	}
	else
	{
		ShowClientDialog((CUpDownClient*)content->value);
	}
}

int CDownloadListCtrl::GetCompleteDownloads(int cat, int& total)
{
	total = 0;
	int count = 0;
	for (ListItems::const_iterator it = m_ListItems.begin(); it != m_ListItems.end(); it++)
	{
		const CtrlItem_Struct* cur_item = it->second;
		if (cur_item->type == FILE_TYPE)
		{
			/*const*/ CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);
			if (file->CheckShowItemInGivenCat(cat) || cat==-1)
			{
				total++;
				if (file->GetStatus() == PS_COMPLETE)
					count++;
			}
		}
	}
	return count;
}

void CDownloadListCtrl::UpdateCurrentCategoryView(){
	ChangeCategory(curTab);
}

void CDownloadListCtrl::UpdateCurrentCategoryView(CPartFile* thisfile) {

	ListItems::const_iterator it = m_ListItems.find(thisfile);
	if (it != m_ListItems.end()) {
		const CtrlItem_Struct* cur_item = it->second;
		if (cur_item->type == FILE_TYPE){
			CPartFile* file = reinterpret_cast<CPartFile*>(cur_item->value);
			
			if (!file->CheckShowItemInGivenCat(curTab))
				HideFile(file);
			else
				ShowFile(file);
		}
	}

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
		int result = FindItem(&find);
		if (result != -1){
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
		int result = FindItem(&find);
		if (result == -1)
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

void CDownloadListCtrl::MoveCompletedfilesCat(uint8 from, uint8 to)
{
	int mycat;

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
						if (from<to)
							file->SetCategory(mycat-1);
						else
							file->SetCategory(mycat+1);
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

		const CtrlItem_Struct* content = (CtrlItem_Struct*)GetItemData(pGetInfoTip->iItem);
		if (content && pGetInfoTip->pszText && pGetInfoTip->cchTextMax > 0)
		{
			CString info;

			// build info text and display it
			if (content->type == 1) // for downloading files
			{
				const CPartFile* partfile = (CPartFile*)content->value;
				info = partfile->GetInfoSummary();
			}
			else if (content->type == 3 || content->type == 2) // for sources
			{
				const CUpDownClient* client = (CUpDownClient*)content->value;
				if (client->IsEd2kClient())
				{
					in_addr server;
					server.S_un.S_addr = client->GetServerIP();
					info.Format(GetResString(IDS_USERINFO)
								+ GetResString(IDS_SERVER) + _T(":%s:%u\n\n")
								+ GetResString(IDS_NEXT_REASK) + _T(":%s"),
								client->GetUserName() ? client->GetUserName() : _T("?"),
								ipstr(server), client->GetServerPort(),
								CastSecondsToHM(client->GetTimeUntilReask(client->GetRequestFile()) / 1000));
					if (thePrefs.IsExtControlsEnabled())
						info.AppendFormat(_T(" (%s)"), CastSecondsToHM(client->GetTimeUntilReask(content->owner) / 1000));
					info += _T('\n');
					info.AppendFormat(GetResString(IDS_SOURCEINFO), client->GetAskedCountDown(), client->GetAvailablePartCount());
					info += _T('\n');

					if (content->type == 2)
					{
						info += GetResString(IDS_CLIENTSOURCENAME) + (!client->GetClientFilename().IsEmpty() ? client->GetClientFilename() : _T("-"));
						if (!client->GetFileComment().IsEmpty())
							info += _T('\n') + GetResString(IDS_CMT_READ) + _T(' ') + client->GetFileComment();
						if (client->GetFileRating())
							info += _T('\n') + GetResString(IDS_QL_RATING) + _T(':') + GetRateString(client->GetFileRating());
					}
					else
					{	// client asked twice
						info += GetResString(IDS_ASKEDFAF);
                        if (client->GetRequestFile() && client->GetRequestFile()->GetFileName())
                            info.AppendFormat(_T(":%s"), client->GetRequestFile()->GetFileName());
					}

                    if (thePrefs.IsExtControlsEnabled() && !client->m_OtherRequests_list.IsEmpty())
					{
						CSimpleArray<const CString*> apstrFileNames;
						POSITION pos = client->m_OtherRequests_list.GetHeadPosition();
						while (pos)
							apstrFileNames.Add(&client->m_OtherRequests_list.GetNext(pos)->GetFileName());
						Sort(apstrFileNames);
						if (content->type == 2)
							info += _T('\n');
						info += _T('\n');
						info += GetResString(IDS_A4AF_FILES);
						info += _T(':');
						for (int i = 0; i < apstrFileNames.GetSize(); i++)
						{
							const CString* pstrFileName = apstrFileNames[i];
							if (info.GetLength() + (i > 0 ? 2 : 0) + pstrFileName->GetLength() >= pGetInfoTip->cchTextMax) {
								static const TCHAR szEllipses[] = _T("\n:...");
								if (info.GetLength() + (int)ARRSIZE(szEllipses) - 1 < pGetInfoTip->cchTextMax)
									info += szEllipses;
								break;
							}
							if (i > 0)
								info += _T("\n:");
							info += *pstrFileName;
						}
                    }
				}
				else
				{
					info.Format(_T("URL:%s\nAvailable parts:%u"), client->GetUserName(), client->GetAvailablePartCount());
				}
			}

			_tcsncpy(pGetInfoTip->pszText, info, pGetInfoTip->cchTextMax);
			pGetInfoTip->pszText[pGetInfoTip->cchTextMax-1] = _T('\0');
		}
	}
	*pResult = 0;
}

void CDownloadListCtrl::ShowFileDialog(UINT uInvokePage)
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
		CDownloadListListCtrlItemWalk::SetItemType(FILE_TYPE);
		CFileDetailDialog dialog(&aFiles, uInvokePage, this);
		dialog.DoModal();
	}
}

CDownloadListListCtrlItemWalk::CDownloadListListCtrlItemWalk(CDownloadListCtrl* pListCtrl)
	: CListCtrlItemWalk(pListCtrl)
{
	m_pDownloadListCtrl = pListCtrl;
	m_eItemType = (ItemType)-1;
}

CObject* CDownloadListListCtrlItemWalk::GetPrevSelectableItem()
{
	ASSERT( m_pDownloadListCtrl != NULL );
	if (m_pDownloadListCtrl == NULL)
		return NULL;
	ASSERT( m_eItemType != (ItemType)-1 );

	int iItemCount = m_pDownloadListCtrl->GetItemCount();
	if (iItemCount >= 2)
	{
		POSITION pos = m_pDownloadListCtrl->GetFirstSelectedItemPosition();
		if (pos)
		{
			int iItem = m_pDownloadListCtrl->GetNextSelectedItem(pos);
			int iCurSelItem = iItem;
			while (iItem-1 >= 0)
			{
				iItem--;

				const CtrlItem_Struct* ctrl_item = (CtrlItem_Struct*)m_pDownloadListCtrl->GetItemData(iItem);
				if (ctrl_item->type == m_eItemType || (m_eItemType != FILE_TYPE && ctrl_item->type != FILE_TYPE))
				{
					m_pDownloadListCtrl->SetItemState(iCurSelItem, 0, LVIS_SELECTED | LVIS_FOCUSED);
					m_pDownloadListCtrl->SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
					m_pDownloadListCtrl->SetSelectionMark(iItem);
					m_pDownloadListCtrl->EnsureVisible(iItem, FALSE);
					return STATIC_DOWNCAST(CObject, (CObject*)ctrl_item->value);
				}
			}
		}
	}
	return NULL;
}

CObject* CDownloadListListCtrlItemWalk::GetNextSelectableItem()
{
	ASSERT( m_pDownloadListCtrl != NULL );
	if (m_pDownloadListCtrl == NULL)
		return NULL;
	ASSERT( m_eItemType != (ItemType)-1 );

	int iItemCount = m_pDownloadListCtrl->GetItemCount();
	if (iItemCount >= 2)
	{
		POSITION pos = m_pDownloadListCtrl->GetFirstSelectedItemPosition();
		if (pos)
		{
			int iItem = m_pDownloadListCtrl->GetNextSelectedItem(pos);
			int iCurSelItem = iItem;
			while (iItem+1 < iItemCount)
			{
				iItem++;

				const CtrlItem_Struct* ctrl_item = (CtrlItem_Struct*)m_pDownloadListCtrl->GetItemData(iItem);
				if (ctrl_item->type == m_eItemType || (m_eItemType != FILE_TYPE && ctrl_item->type != FILE_TYPE))
				{
					m_pDownloadListCtrl->SetItemState(iCurSelItem, 0, LVIS_SELECTED | LVIS_FOCUSED);
					m_pDownloadListCtrl->SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
					m_pDownloadListCtrl->SetSelectionMark(iItem);
					m_pDownloadListCtrl->EnsureVisible(iItem, FALSE);
					return STATIC_DOWNCAST(CObject, (CObject*)ctrl_item->value);
				}
			}
		}
	}
	return NULL;
}

void CDownloadListCtrl::ShowClientDialog(CUpDownClient* pClient)
{
	CDownloadListListCtrlItemWalk::SetItemType(AVAILABLE_SOURCE); // just set to something !=FILE_TYPE
	CClientDetailDialog dialog(pClient, this);
	dialog.DoModal();
}
