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
#include "emuledlg.h"
#include "SharedFilesCtrl.h"
#include "OtherFunctions.h"
#include "FileInfoDialog.h"
#include "MetaDataDlg.h"
#include "ED2kLinkDlg.h"
#include "CommentDialog.h"
#include "HighColorTab.hpp"
#include "ListViewWalkerPropertySheet.h"
#include "UserMsgs.h"
#include "ResizableLib/ResizableSheet.h"
#include "KnownFile.h"
#include "MapKey.h"
#include "SharedFileList.h"
#include "MemDC.h"
#include "PartFile.h"
#include "MenuCmds.h"
#include "IrcWnd.h"
#include "SharedFilesWnd.h"
#include "Opcodes.h"
#include "InputBox.h"
#include "WebServices.h"
#include "TransferWnd.h"
#include "ClientList.h"
#include "UpDownClient.h"
#include "Collection.h"
#include "CollectionCreateDialog.h"
#include "CollectionViewDialog.h"
#include "SharedDirsTreeCtrl.h"
#include "SearchParams.h"
#include "SearchDlg.h"
#include "SearchResultsWnd.h"
#include "uploadqueue.h" //MORPH - Added by SiRoB, 
#include "Log.h" //MORPH
// Mighty Knife: CRC32-Tag, Mass Rename
#include "AddCRC32TagDialog.h"
#include "MassRename.h"
// [end] Mighty Knife

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif



//////////////////////////////////////////////////////////////////////////////
// CSharedFileDetailsSheet

class CSharedFileDetailsSheet : public CListViewWalkerPropertySheet
{
	DECLARE_DYNAMIC(CSharedFileDetailsSheet)

public:
	CSharedFileDetailsSheet(CTypedPtrList<CPtrList, CKnownFile*>& aFiles, UINT uPshInvokePage = 0, CListCtrlItemWalk* pListCtrl = NULL);
	virtual ~CSharedFileDetailsSheet();

protected:
	CFileInfoDialog		m_wndMediaInfo;
	CMetaDataDlg		m_wndMetaData;
	CED2kLinkDlg		m_wndFileLink;
	CCommentDialog		m_wndFileComments;

	UINT m_uPshInvokePage;
	static LPCTSTR m_pPshStartPage;

	void UpdateTitle();

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
};

LPCTSTR CSharedFileDetailsSheet::m_pPshStartPage;

IMPLEMENT_DYNAMIC(CSharedFileDetailsSheet, CListViewWalkerPropertySheet)

BEGIN_MESSAGE_MAP(CSharedFileDetailsSheet, CListViewWalkerPropertySheet)
	ON_WM_DESTROY()
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
END_MESSAGE_MAP()

CSharedFileDetailsSheet::CSharedFileDetailsSheet(CTypedPtrList<CPtrList, CKnownFile*>& aFiles, UINT uPshInvokePage, CListCtrlItemWalk* pListCtrl)
	: CListViewWalkerPropertySheet(pListCtrl)
{
	m_uPshInvokePage = uPshInvokePage;
	POSITION pos = aFiles.GetHeadPosition();
	while (pos)
		m_aItems.Add(aFiles.GetNext(pos));
	m_psh.dwFlags &= ~PSH_HASHELP;

	m_wndMediaInfo.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndMediaInfo.m_psp.dwFlags |= PSP_USEICONID;
	m_wndMediaInfo.m_psp.pszIcon = _T("MEDIAINFO");
	m_wndMediaInfo.SetFiles(&m_aItems);
	AddPage(&m_wndMediaInfo);

	m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndMetaData.m_psp.dwFlags |= PSP_USEICONID;
	m_wndMetaData.m_psp.pszIcon = _T("METADATA");
	if (m_aItems.GetSize() == 1 && thePrefs.IsExtControlsEnabled()) {
		m_wndMetaData.SetFiles(&m_aItems);
		AddPage(&m_wndMetaData);
	}

	m_wndFileLink.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndFileLink.m_psp.dwFlags |= PSP_USEICONID;
	m_wndFileLink.m_psp.pszIcon = _T("ED2KLINK");
	m_wndFileLink.SetFiles(&m_aItems);
	AddPage(&m_wndFileLink);

	m_wndFileComments.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndFileComments.m_psp.dwFlags |= PSP_USEICONID;
	m_wndFileComments.m_psp.pszIcon = _T("FileComments");
	m_wndFileComments.SetFiles(&m_aItems);
	AddPage(&m_wndFileComments);

	LPCTSTR pPshStartPage = m_pPshStartPage;
	if (m_uPshInvokePage != 0)
		pPshStartPage = MAKEINTRESOURCE(m_uPshInvokePage);
	for (int i = 0; i < m_pages.GetSize(); i++)
	{
		CPropertyPage* pPage = GetPage(i);
		if (pPage->m_psp.pszTemplate == pPshStartPage)
		{
			m_psh.nStartPage = i;
			break;
		}
	}
}

CSharedFileDetailsSheet::~CSharedFileDetailsSheet()
{
}

void CSharedFileDetailsSheet::OnDestroy()
{
	if (m_uPshInvokePage == 0)
		m_pPshStartPage = GetPage(GetActiveIndex())->m_psp.pszTemplate;
	CListViewWalkerPropertySheet::OnDestroy();
}

BOOL CSharedFileDetailsSheet::OnInitDialog()
{
	EnableStackedTabs(FALSE);
	BOOL bResult = CListViewWalkerPropertySheet::OnInitDialog();
	HighColorTab::UpdateImageList(*this);
	InitWindowStyles(this);
	EnableSaveRestore(_T("SharedFileDetailsSheet")); // call this after(!) OnInitDialog
	UpdateTitle();
	return bResult;
}

LRESULT CSharedFileDetailsSheet::OnDataChanged(WPARAM, LPARAM)
{
	UpdateTitle();
	return 1;
}

void CSharedFileDetailsSheet::UpdateTitle()
{
	if (m_aItems.GetSize() == 1)
		SetWindowText(GetResString(IDS_DETAILS) + _T(": ") + STATIC_DOWNCAST(CKnownFile, m_aItems[0])->GetFileName());
	else
		SetWindowText(GetResString(IDS_DETAILS));
}

BOOL CSharedFileDetailsSheet::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_APPLY_NOW)
	{
		CSharedFilesCtrl* pSharedFilesCtrl = DYNAMIC_DOWNCAST(CSharedFilesCtrl, m_pListCtrl->GetListCtrl());
		if (pSharedFilesCtrl)
		{
			for (int i = 0; i < m_aItems.GetSize(); i++) {
				// so, and why does this not(!) work while the sheet is open ??
				pSharedFilesCtrl->UpdateFile(DYNAMIC_DOWNCAST(CKnownFile, m_aItems[i]));
			}
		}
	}

	return CListViewWalkerPropertySheet::OnCommand(wParam, lParam);
}


//////////////////////////////////////////////////////////////////////////////
// CSharedFilesCtrl

IMPLEMENT_DYNAMIC(CSharedFilesCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CSharedFilesCtrl, CMuleListCtrl)
	ON_WM_SYSCOLORCHANGE()
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
	ON_WM_KEYDOWN()
	// Mighty Knife: CRC32-Tag - Save rename
	ON_MESSAGE(UM_CRC32_RENAMEFILE,	OnCRC32RenameFile)
	ON_MESSAGE(UM_CRC32_UPDATEFILE, OnCRC32UpdateFile)
	// [end] Mighty Knife
END_MESSAGE_MAP()

CSharedFilesCtrl::CSharedFilesCtrl()
	: CListCtrlItemWalk(this)
{
	memset(&sortstat, 0, sizeof(sortstat));
	nAICHHashing = 0;
	m_pDirectoryFilter = NULL;
	SetGeneralPurposeFind(true, false);
}

CSharedFilesCtrl::~CSharedFilesCtrl()
{
}

void CSharedFilesCtrl::Init()
{
	SetName(_T("SharedFilesCtrl"));
	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy,theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	SetExtendedStyle(LVS_EX_FULLROWSELECT);
	ModifyStyle(LVS_SINGLESEL,0);

	InsertColumn(0, GetResString(IDS_DL_FILENAME) ,LVCFMT_LEFT,250,0);
	InsertColumn(1,GetResString(IDS_DL_SIZE),LVCFMT_LEFT,100,1);
	InsertColumn(2,GetResString(IDS_TYPE),LVCFMT_LEFT,50,2);
	InsertColumn(3,GetResString(IDS_PRIORITY),LVCFMT_LEFT,70,3);
	InsertColumn(4,GetResString(IDS_FILEID),LVCFMT_LEFT,220,4);
	InsertColumn(5,GetResString(IDS_SF_REQUESTS),LVCFMT_LEFT,100,5);
	InsertColumn(6,GetResString(IDS_SF_ACCEPTS),LVCFMT_LEFT,100,6);
	InsertColumn(7,GetResString(IDS_SF_TRANSFERRED),LVCFMT_LEFT,120,7);
	InsertColumn(8,GetResString(IDS_UPSTATUS),LVCFMT_LEFT,100,8);
	InsertColumn(9,GetResString(IDS_FOLDER),LVCFMT_LEFT,200,9);
	InsertColumn(10,GetResString(IDS_COMPLSOURCES),LVCFMT_LEFT,100,10);
	InsertColumn(11,GetResString(IDS_SHAREDTITLE),LVCFMT_LEFT,200,11);
	//MORPH START - Added by SiRoB, Keep Permission flag
	InsertColumn(12,GetResString(IDS_PERMISSION),LVCFMT_LEFT,100,12);
	//MORPH START - Added by SiRoB, ZZ Upload System
	InsertColumn(13,GetResString(IDS_POWERSHARE_COLUMN_LABEL),LVCFMT_LEFT,70,13);
	//MORPH END - Added by SiRoB, ZZ Upload System
	//MORPH START - Added by IceCream, SLUGFILLER: showComments
	//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars
	InsertColumn(14,GetResString(IDS_SF_UPLOADED_PARTS),LVCFMT_LEFT,170,14); // SF
	InsertColumn(15,GetResString(IDS_SF_TURN_PART),LVCFMT_LEFT,100,15); // SF
	InsertColumn(16,GetResString(IDS_SF_TURN_SIMPLE),LVCFMT_LEFT,100,16); // VQB
	InsertColumn(17,GetResString(IDS_SF_FULLUPLOAD),LVCFMT_LEFT,100,17); // SF
	//MORPH END   - Added & Modified by IceCream, SLUGFILLER: Spreadbars
	//MORPH START - Added by SiRoB, HIDEOS
    InsertColumn(18,GetResString(IDS_HIDEOS),LVCFMT_LEFT,100,18);
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	InsertColumn(19,GetResString(IDS_SHAREONLYTHENEED),LVCFMT_LEFT,100,19);
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
	// Mighty Knife: CRC32-Tag
	InsertColumn(20,GetResString(IDS_CRC_CALCULATED),LVCFMT_LEFT,120,20);
	InsertColumn(21,GetResString(IDS_CRC_CHECKOK),LVCFMT_LEFT,100,21);
	// [end] Mighty Knife

	SetAllIcons();
	CreateMenues();
	LoadSettings();

	// Barry - Use preferred sort order from preferences
	SetSortArrow();
	// Mighty Knife: CRC32-Tag - Indexes shifted by 10
	/*
	SortItems(SortProc, GetSortItem() + (GetSortAscending() ? 0:20));
	*/
	SortItems(SortProc, GetSortItem() + (GetSortAscending() ? 0:30));
	// [end] Mighty Knife
}

void CSharedFilesCtrl::OnSysColorChange()
{
	CMuleListCtrl::OnSysColorChange();
	SetAllIcons();
	CreateMenues();
}

void CSharedFilesCtrl::SetAllIcons()
{
	m_ImageList.DeleteImageList();
	m_ImageList.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	m_ImageList.SetBkColor(CLR_NONE);
	m_ImageList.Add(CTempIconLoader(_T("EMPTY")));
	m_ImageList.Add(CTempIconLoader(_T("FileSharedServer"), 16, 16));
	m_ImageList.Add(CTempIconLoader(_T("FileSharedKad"), 16, 16));
	m_ImageList.Add(CTempIconLoader(_T("Rating_NotRated")));
	m_ImageList.Add(CTempIconLoader(_T("Rating_Fake")));
	m_ImageList.Add(CTempIconLoader(_T("Rating_Poor")));
	m_ImageList.Add(CTempIconLoader(_T("Rating_Fair")));
	m_ImageList.Add(CTempIconLoader(_T("Rating_Good")));
	m_ImageList.Add(CTempIconLoader(_T("Rating_Excellent")));
	m_ImageList.SetOverlayImage(m_ImageList.Add(CTempIconLoader(_T("FileCommentsOvl"))), 1);
}

void CSharedFilesCtrl::Localize()
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

	strRes = GetResString(IDS_TYPE);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(2, &hdi);

	strRes = GetResString(IDS_PRIORITY);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(3, &hdi);

	strRes = GetResString(IDS_FILEID);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(4, &hdi);

	strRes = GetResString(IDS_SF_REQUESTS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(5, &hdi);

	strRes = GetResString(IDS_SF_ACCEPTS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(6, &hdi);

	strRes = GetResString(IDS_SF_TRANSFERRED);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(7, &hdi);

	strRes = GetResString(IDS_SHARED_STATUS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(8, &hdi);

	strRes = GetResString(IDS_FOLDER);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(9, &hdi);

	strRes = GetResString(IDS_COMPLSOURCES);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(10, &hdi);

	strRes = GetResString(IDS_SHAREDTITLE);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(11, &hdi);

	//MORPH START - Added  by SiRoB, Keep Permission flag
	strRes = GetResString(IDS_PERMISSION);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(12, &hdi);
	//MORPH END   - Added  by SiRoB, Keep Permission flag

	//MORPH START - Added  by SiRoB, ZZ Upload System
	strRes = GetResString(IDS_POWERSHARE_COLUMN_LABEL);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(13, &hdi);
	//MORPH END - Added  by SiRoB, ZZ Upload System

	//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars
	strRes = GetResString(IDS_SF_UPLOADED_PARTS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(14, &hdi);

	strRes = GetResString(IDS_SF_TURN_PART);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(15, &hdi);

	strRes = GetResString(IDS_SF_TURN_SIMPLE);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(16, &hdi);

	strRes = GetResString(IDS_SF_FULLUPLOAD);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(17, &hdi);
	//MORPH END - Added by IceCream SLUGFILLER: Spreadbars
	//MORPH START - Added by SiRoB, HIDEOS
    strRes = GetResString(IDS_HIDEOS);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(18, &hdi);
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	strRes = GetResString(IDS_SHAREONLYTHENEED);
	hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
	pHeaderCtrl->SetItem(19, &hdi);
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
	
	CreateMenues();

	int iItems = GetItemCount();
	for (int i = 0; i < iItems; i++)
		Update(i);
}

void CSharedFilesCtrl::AddFile(const CKnownFile* file)
{
	if (!theApp.emuledlg->IsRunning())
		return;
	// check filter conditions if we should show this file right now
	if (m_pDirectoryFilter != NULL){
		CString strFilePath = file->GetPath();
		if (strFilePath.Right(1) == "\\"){
			strFilePath = strFilePath.Left(strFilePath.GetLength()-1);
		}
		switch(m_pDirectoryFilter->m_eItemType){
			case SDI_ALL:
				// No filter
				break;
			case SDI_FILESYSTEMPARENT:
				return; // no files
				break;
			case SDI_NO:
				// some shared directory
			case SDI_CATINCOMING:
				// Categories with special incoming dirs
			case SDI_UNSHAREDDIRECTORY:
				// Items from the whole filesystem tree
				if (strFilePath.CompareNoCase(m_pDirectoryFilter->m_strFullPath) != 0)
					return;
				break;
			case SDI_TEMP:
				// only tempfiles
				if (!file->IsPartFile())
					return;
				else if (m_pDirectoryFilter->m_nCatFilter != -1 && (UINT)m_pDirectoryFilter->m_nCatFilter != ((CPartFile*)file)->GetCategory())
					return;
				break;
			case SDI_DIRECTORY:
				// any userselected shared dir but not incoming or temp
				if (file->IsPartFile())
					return;
				if (strFilePath.CompareNoCase(thePrefs.GetIncomingDir()) == 0)
					return;
				break;
			case SDI_INCOMING:
				// Main incoming directory
				if (strFilePath.CompareNoCase(thePrefs.GetIncomingDir()) != 0)
					return;
				// Hmm should we show all incoming files dirs or only those from the main incoming dir here?
				// hard choice, will only show the main for now
				break;

		}
	}
	if (FindFile(file) != -1)
		return;
	int iItem = InsertItem(LVIF_TEXT|LVIF_PARAM, GetItemCount(), LPSTR_TEXTCALLBACK, 0, 0, 0, (LPARAM)file);
	if (iItem >= 0)
		Update(iItem);
}

void CSharedFilesCtrl::RemoveFile(const CKnownFile* file)
{
	int iItem = FindFile(file);
	if (iItem != -1)
	{
		DeleteItem(iItem);
		ShowFilesCount();
	}
}

void CSharedFilesCtrl::UpdateFile(const CKnownFile* file)
{
	if(!file || !theApp.emuledlg->IsRunning())
		return;
	int iItem = FindFile(file);
	if (iItem != -1)
	{
		Update(iItem);
		if (GetItemState(iItem, LVIS_SELECTED))
			theApp.emuledlg->sharedfileswnd->ShowSelectedFilesSummary();
	}
}

int CSharedFilesCtrl::FindFile(const CKnownFile* pFile)
{
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)pFile;
	return FindItem(&find);
}

void CSharedFilesCtrl::ReloadFileList()
{
	DeleteAllItems();
	//theApp.emuledlg->sharedfileswnd->ShowSelectedFilesSummary();

	CCKey bufKey;
	CKnownFile* cur_file;
	for (POSITION pos = theApp.sharedfiles->m_Files_map.GetStartPosition(); pos != 0; ){
		theApp.sharedfiles->m_Files_map.GetNextAssoc(pos, bufKey, cur_file);
		AddFile(cur_file);
	}
	ShowFilesCount();
}

void CSharedFilesCtrl::ShowFilesCount()
{
	CString str;
	if (theApp.sharedfiles->GetHashingCount() + nAICHHashing)
		str.Format(_T(" (%i, %s %i)"), theApp.sharedfiles->GetCount(), GetResString(IDS_HASHING), theApp.sharedfiles->GetHashingCount() + nAICHHashing);
	else
		str.Format(_T(" (%i)"), theApp.sharedfiles->GetCount());
	theApp.emuledlg->sharedfileswnd->GetDlgItem(IDC_TRAFFIC_TEXT)->SetWindowText(GetResString(IDS_SF_FILES) + str);
}

#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)

void CSharedFilesCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if( !theApp.emuledlg->IsRunning() )
		return;
	if( !lpDrawItemStruct->itemData)
		return;
	//MORPH START - Added by SiRoB, Don't draw hidden Rect
	RECT clientRect;
	GetClientRect(&clientRect);
	CRect cur_rec(lpDrawItemStruct->rcItem);
	if (cur_rec.top >= clientRect.bottom || cur_rec.bottom <= clientRect.top)
		return;
	//MORPH END   - Added by SiRoB, Don't draw hidden Rect
	CDC* odc = CDC::FromHandle(lpDrawItemStruct->hDC);
	BOOL bCtrlFocused = ((GetFocus() == this ) || (GetStyle() & LVS_SHOWSELALWAYS));
	if (lpDrawItemStruct->itemState & ODS_SELECTED) {
		if(bCtrlFocused)
			odc->SetBkColor(m_crHighlight);
		else
			odc->SetBkColor(m_crNoHighlight);
	}
	else
		odc->SetBkColor(GetBkColor());
	/*const*/ CKnownFile* file = (CKnownFile*)lpDrawItemStruct->itemData;
	CMemDC dc(odc, &lpDrawItemStruct->rcItem);
	CFont* pOldFont = dc.SelectObject(GetFont());
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

	CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	const int iMarginX = 4;
	cur_rec.right = cur_rec.left - iMarginX*2;
	cur_rec.left += iMarginX;
	CString buffer;
	int iIconDrawWidth = theApp.GetSmallSytemIconSize().cx + 3;
	for(int iCurrent = 0; iCurrent < iCount; iCurrent++){
		int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
		if( !IsColumnHidden(iColumn) ){
			UINT uDTFlags = DLC_DT_TEXT;
			cur_rec.right += GetColumnWidth(iColumn);
			//MORPH START - Added by SiRoB, Don't draw hidden columns
			if (cur_rec.left < clientRect.right && cur_rec.right > clientRect.left)
			{
			//MORPH END   - Added by SiRoB, Don't draw hidden columns
				//MORPH - Moved by SiRoB, due to the draw system change on hidden rect
				// xMule_MOD: showSharePermissions, modified by itsonlyme
				// display not finished files in navy, blocked files in red and friend-only files in orange
				int Perm = file->GetPermissions()>=0?file->GetPermissions():thePrefs.GetPermissions();
				if (iColumn==0){
					if (file->IsPartFile())
						dc->SetTextColor((COLORREF)RGB(0,0,192));
				}else if (Perm == PERM_NOONE)
					dc->SetTextColor((COLORREF)RGB(0,175,0));
				else if (Perm == PERM_FRIENDS)
					dc->SetTextColor((COLORREF)RGB(208,128,0));
				// Mighty Knife: Community visible filelist
				else if (Perm == PERM_COMMUNITY)
					dc->SetTextColor((COLORREF)RGB(240,0,128));
				// [end] Mighty Knife
				else if (Perm == PERM_ALL)
					dc->SetTextColor((COLORREF)RGB(240,0,0));
				// xMule_MOD: showSharePermissions
				switch(iColumn){
					case 0:{
						int iImage = theApp.GetFileTypeSystemImageIdx(file->GetFileName());
						if (theApp.GetSystemImageList() != NULL)
						::ImageList_Draw(theApp.GetSystemImageList(), iImage, dc.GetSafeHdc(), cur_rec.left, cur_rec.top, ILD_NORMAL|ILD_TRANSPARENT);
						if (!file->GetFileComment().IsEmpty() || file->GetFileRating())
							m_ImageList.Draw(dc, 0, CPoint(cur_rec.left, cur_rec.top), ILD_NORMAL | ILD_TRANSPARENT | INDEXTOOVERLAYMASK(1));
						cur_rec.left += (iIconDrawWidth-3);

						if (thePrefs.ShowRatingIndicator())
						{
							if ((file->HasComment() || file->HasRating()))
							{
							m_ImageList.Draw(dc, file->UserRating()+3, CPoint(cur_rec.left, cur_rec.top), ILD_NORMAL);
							}
							cur_rec.left += 16;
							iIconDrawWidth += 16;
						}

						cur_rec.left += 3;

						buffer = file->GetFileName();
						break;
					}
					case 1:
					buffer = CastItoXBytes(file->GetFileSize(), false, false);
						uDTFlags |= DT_RIGHT;
						break;
					case 2:
					buffer = file->GetFileTypeDisplayStr();
						break;
					case 3:{
						switch (file->GetUpPriority()) {
							case PR_VERYLOW :
								buffer = GetResString(IDS_PRIOVERYLOW);
								break;
							case PR_LOW :
								if( file->IsAutoUpPriority() )
									buffer = GetResString(IDS_PRIOAUTOLOW);
								else
									buffer = GetResString(IDS_PRIOLOW);
								break;
							case PR_NORMAL :
								if( file->IsAutoUpPriority() )
									buffer = GetResString(IDS_PRIOAUTONORMAL);
								else
									buffer = GetResString(IDS_PRIONORMAL);
								break;
							case PR_HIGH :
								if( file->IsAutoUpPriority() )
									buffer = GetResString(IDS_PRIOAUTOHIGH);
								else
									buffer = GetResString(IDS_PRIOHIGH);
								break;
							case PR_VERYHIGH :
								buffer = GetResString(IDS_PRIORELEASE);
								break;
							default:
								buffer.Empty();
						}
						//MORPH START - Added by SiRoB, Powershare State in prio colums
						if(file->GetPowerShared()) {
							CString tempString = GetResString(IDS_POWERSHARE_PREFIX);
							tempString.Append(_T(" "));
							tempString.Append(buffer);
							buffer.Empty();
							buffer = tempString;
						}
						//MORPH END   - Added by SiRoB, Powershare State in prio colums
						break;
					}
					case 4:
						buffer = md4str(file->GetFileHash());
						break;
					case 5:
                    buffer.Format(_T("%u (%u)"), file->statistic.GetRequests(), file->statistic.GetAllTimeRequests());
						break;
					case 6:
					buffer.Format(_T("%u (%u)"), file->statistic.GetAccepts(), file->statistic.GetAllTimeAccepts());
						break;
					case 7:
					buffer.Format(_T("%s (%s)"), CastItoXBytes(file->statistic.GetTransferred(), false, false), CastItoXBytes(file->statistic.GetAllTimeTransferred(), false, false));
						break;
					case 8:
						if( file->GetPartCount()){
							cur_rec.bottom--;
							cur_rec.top++;
							if(!file->IsPartFile())
								((CKnownFile*)lpDrawItemStruct->itemData)->DrawShareStatusBar(dc,&cur_rec,false,thePrefs.UseFlatBar());
							else
								((CPartFile*)lpDrawItemStruct->itemData)->DrawShareStatusBar(dc,&cur_rec,false,thePrefs.UseFlatBar());
							cur_rec.bottom++;
							cur_rec.top--;
						}
						break;
					case 9:
						buffer = file->GetPath();
						PathRemoveBackslash(buffer.GetBuffer());
						buffer.ReleaseBuffer();
						break;
					case 10:
                	    //MORPH START - Changed by SiRoB, Avoid misusing of powersharing
  						if (file->m_nCompleteSourcesCountLo == file->m_nCompleteSourcesCountHi)
							buffer.Format(_T("%u (%u)"), file->m_nCompleteSourcesCountLo, file->m_nVirtualCompleteSourcesCount);
                	    else if (file->m_nCompleteSourcesCountLo == 0)
							buffer.Format(_T("< %u (%u)"), file->m_nCompleteSourcesCountHi, file->m_nVirtualCompleteSourcesCount);
						else
							buffer.Format(_T("%u - %u (%u)"), file->m_nCompleteSourcesCountLo, file->m_nCompleteSourcesCountHi, file->m_nVirtualCompleteSourcesCount);
						//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
						break;
					case 11:{
						CPoint pt(cur_rec.left, cur_rec.top);
						m_ImageList.Draw(dc, file->GetPublishedED2K() ? 1 : 0, pt, ILD_NORMAL | ILD_TRANSPARENT);
						pt.x += 16;
						bool bSharedInKad;
						if ((uint32)time(NULL) < file->GetLastPublishTimeKadSrc())
						{
							if (theApp.IsFirewalled() && theApp.IsConnected())
							{
								if (theApp.clientlist->GetBuddy() && (file->GetLastPublishBuddy() == theApp.clientlist->GetBuddy()->GetIP()))
									bSharedInKad = true;
								else
									bSharedInKad = false;
							}
							else
								bSharedInKad = true;
						}
						else
							bSharedInKad = false;
						m_ImageList.Draw(dc, bSharedInKad ? 2 : 0, pt, ILD_NORMAL | ILD_TRANSPARENT);
						buffer.Empty();
						break;
					}
					//MORPH START - Added by SiRoB, Keep Permission flag
					case 12:{
						// xMule_MOD: showSharePermissions
						int Perm = file->GetPermissions()>=0?file->GetPermissions():thePrefs.GetPermissions();
						switch (Perm)
						{
							case PERM_ALL:
								buffer = GetResString(IDS_FSTATUS_PUBLIC);
								break;
							case PERM_NOONE: 
								buffer = GetResString(IDS_HIDDEN); 
								break;
							case PERM_FRIENDS: 
								buffer = GetResString(IDS_FSTATUS_FRIENDSONLY); 
								break;
							// Mighty Knife: Community visible filelist
							case PERM_COMMUNITY: 
								buffer = GetResString(IDS_COMMUNITY); 
								break;
							// [end] Mighty Knife
							default: 
								buffer = _T("?");
								break;
						}
						if (file->GetPermissions() < 0)
							buffer = ((CString)GetResString(IDS_DEFAULT)).Left(1) + _T(". ") + buffer;
						// xMule_MOD: showSharePermissions
						break;
					}
					//MORPH END   - Added by SiRoB, Keep Permission flag
					//MORPH START - Added by SiRoB, ZZ Upload System
					case 13:{
						//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
						int powersharemode;
						bool powershared = file->GetPowerShared();
						buffer = _T("[") + GetResString((powershared)?IDS_POWERSHARE_ON_LABEL:IDS_POWERSHARE_OFF_LABEL) + _T("] ");
						if (file->GetPowerSharedMode()>=0)
							powersharemode = file->GetPowerSharedMode();
						else {
							powersharemode = thePrefs.GetPowerShareMode();
							buffer.Append(_T(" ") + ((CString)GetResString(IDS_DEFAULT)).Left(1) + _T(". "));
						}
						if(powersharemode == 2)
							buffer.Append(GetResString(IDS_POWERSHARE_AUTO_LABEL));
						else if (powersharemode == 1)
							buffer.Append(GetResString(IDS_POWERSHARE_ACTIVATED_LABEL));
						//MORPH START - Added by SiRoB, POWERSHARE Limit
						else if (powersharemode == 3) {
							buffer.Append(GetResString(IDS_POWERSHARE_LIMITED));
							if (file->GetPowerShareLimit()<0)
								buffer.AppendFormat(_T(" %s. %i"), ((CString)GetResString(IDS_DEFAULT)).Left(1), thePrefs.GetPowerShareLimit());
							else
								buffer.AppendFormat(_T(" %i"), file->GetPowerShareLimit());
						}
						//MORPH END   - Added by SiRoB, POWERSHARE Limit
						else
							buffer.Append(GetResString(IDS_POWERSHARE_DISABLED_LABEL));
						buffer.Append(_T(" ("));
						if (file->GetPowerShareAuto())
							buffer.Append(GetResString(IDS_POWERSHARE_ADVISED_LABEL));
						//MORPH START - Added by SiRoB, POWERSHARE Limit
						else if (file->GetPowerShareLimited() && (powersharemode == 3))
							buffer.Append(GetResString(IDS_POWERSHARE_LIMITED));
						//MORPH END   - Added by SiRoB, POWERSHARE Limit
						else if (file->GetPowerShareAuthorized())
							buffer.Append(GetResString(IDS_POWERSHARE_AUTHORIZED_LABEL));
						else
							buffer.Append(GetResString(IDS_POWERSHARE_DENIED_LABEL));
						buffer.Append(_T(")"));
						//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
						break;
					}
					//MORPH END   - Added by SiRoB, ZZ Upload System
					// SLUGFILLER: Spreadbars
					case 14:
						cur_rec.bottom--;
						cur_rec.top++;
						((CKnownFile*)lpDrawItemStruct->itemData)->statistic.DrawSpreadBar(dc,&cur_rec,thePrefs.UseFlatBar());
						cur_rec.bottom++;
						cur_rec.top--;
						break;
					case 15:
						buffer.Format(_T("%.2f"),((CKnownFile*)lpDrawItemStruct->itemData)->statistic.GetSpreadSortValue());
						break;
					case 16:
						if (file->GetFileSize()>(uint64)0)
							buffer.Format(_T("%.2f"),((double)file->statistic.GetAllTimeTransferred())/((double)file->GetFileSize()));
						else
							buffer.Format(_T("%.2f"),0.0f);
						break;
					case 17:
						buffer.Format(_T("%.2f"),((CKnownFile*)lpDrawItemStruct->itemData)->statistic.GetFullSpreadCount());
						break;
					// SLUGFILLER: Spreadbars
					//MORPH START - Added by SiRoB, HIDEOS
					case 18:
					//MORPH START - Changed by SiRoB, Avoid misusing of HideOS
					{
						UINT hideOSInWork = file->HideOSInWork();
						buffer = _T("[") + GetResString((hideOSInWork>0)?IDS_POWERSHARE_ON_LABEL:IDS_POWERSHARE_OFF_LABEL) + _T("] ");
						if(file->GetHideOS()<0)
							buffer.Append(_T(" ") + ((CString)GetResString(IDS_DEFAULT)).Left(1) + _T(". "));
						hideOSInWork = (file->GetHideOS()>=0)?file->GetHideOS():thePrefs.GetHideOvershares();
						if (hideOSInWork>0)
							buffer.AppendFormat(_T("%i"), hideOSInWork);
						//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
						else if(file->GetSpreadbarSetStatus() == 0 || (file->GetSpreadbarSetStatus() == -1 && thePrefs.GetSpreadbarSetStatus() == 0))
							buffer.AppendFormat(_T("%s"), GetResString(IDS_SPREADBAR) + _T(" ") + GetResString(IDS_DISABLED));
						//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
						else
							buffer.AppendFormat(_T("%s"), GetResString(IDS_DISABLED));
						if (file->GetSelectiveChunk()>=0){
							if (file->GetSelectiveChunk())
								buffer.Append(_T(" + S"));
						}else
							if (thePrefs.IsSelectiveShareEnabled())
								buffer.Append(_T(" + ") + ((CString)GetResString(IDS_DEFAULT)).Left(1) + _T(". S"));
						break;
						//MORPH START - Changed by SiRoB, Avoid misusing of HideOS
					}
					//MORPH END   - Added by SiRoB, HIDEOS
					//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
					case 19:
						if(file->GetShareOnlyTheNeed()>=0) {
							if (file->GetShareOnlyTheNeed())
								buffer.Format(_T("%i") ,file->GetShareOnlyTheNeed());
							else
								buffer = GetResString(IDS_DISABLED);
						} else {
							buffer = ((CString)GetResString(IDS_DEFAULT)).Left(1) + _T(". ") + GetResString((thePrefs.GetShareOnlyTheNeed()>0)?IDS_ENABLED:IDS_DISABLED);
						}
						break;
					//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

					// Mighty Knife: CRC32-Tag
					case 20:
						if(file->IsCRC32Calculated()) {
							buffer.Format (_T("%s"),file->GetLastCalculatedCRC32());
						} else buffer = _T("");
						break;
					case 21:
						if(file->IsCRC32Calculated()) {
							CString FName = file->GetFileName();
							FName.MakeUpper (); // Uppercase the filename ! 
												// Our CRC is upper case...
							if (FName.Find (file->GetLastCalculatedCRC32()) != -1) {
								buffer.Format (_T("%s"),GetResString(IDS_YES));
							} else {
								buffer.Format (_T("%s"),GetResString(IDS_NO));
							}
						} else buffer = "";
						break;
					// [end] Mighty Knife
				}
				if(iColumn != 8 && iColumn!=14)
					dc.DrawText(buffer, buffer.GetLength(),&cur_rec,uDTFlags);
				if ( iColumn == 0 )
					cur_rec.left -= iIconDrawWidth;
			//MORPH - Added by SiRoB, Don't draw hidden coloms
			}
			//MORPH END   - Added by SiRoB, Don't draw hidden coloms
			cur_rec.left += GetColumnWidth(iColumn);
		}
	}
	ShowFilesCount();
	if (lpDrawItemStruct->itemState & ODS_SELECTED)
	{
		RECT outline_rec = lpDrawItemStruct->rcItem;

		outline_rec.top--;
		outline_rec.bottom++;
		dc.FrameRect(&outline_rec, &CBrush(m_crWindow));
		outline_rec.top++;
		outline_rec.bottom--;
		outline_rec.left++;
		outline_rec.right--;

		if (lpDrawItemStruct->itemID > 0 && GetItemState(lpDrawItemStruct->itemID - 1, LVIS_SELECTED))
			outline_rec.top--;

		if (lpDrawItemStruct->itemID + 1 < (UINT)GetItemCount() && GetItemState(lpDrawItemStruct->itemID + 1, LVIS_SELECTED))
			outline_rec.bottom++;

		if(bCtrlFocused)
			dc.FrameRect(&outline_rec, &CBrush(m_crFocusLine));
		else
			dc.FrameRect(&outline_rec, &CBrush(m_crNoFocusLine));
	}
	
	if (m_crWindowTextBk == CLR_NONE)
		dc.SetBkMode(iOldBkMode);
	dc.SelectObject(pOldFont);
	dc.SetTextColor(crOldTextColor);
}

void CSharedFilesCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
		// get merged settings
	bool bFirstItem = true;
	int iSelectedItems = GetSelectedCount();
	int iCompleteFileSelected = -1;
	CString buffer; //MORPH - added, for string format temp
	int iPowerShareLimit = -1; //MORPH - Added by SiRoB, POWERSHARE Limit
	int iHideOS = -1; //MORPH - Added by SiRoB, HIDEOS
	
	UINT uPrioMenuItem = 0;
	UINT uPermMenuItem = 0; //MORPH - Added by SiRoB, showSharePermissions
	UINT uPowershareMenuItem = 0; //MORPH - Added by SiRoB, Powershare
	UINT uPowerShareLimitMenuItem = 0; //MORPH - Added by SiRoB, POWERSHARE Limit
	
	UINT uSpreadbarMenuItem = 0; //MORPH	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	UINT uHideOSMenuItem = 0; //MORPH - Added by SiRoB, HIDEOS
	UINT uSelectiveChunkMenuItem = 0; //MORPH - Added by SiRoB, HIDEOS
	UINT uShareOnlyTheNeedMenuItem = 0; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
	//MORPH START - Added by SiRoB, WebCache 1.2f
	bool uWCReleaseItem = true; //JP webcache release
	bool uGreyOutWCRelease = true; //JP webcache release
	//MORPH END   - Added by SiRoB, WebCache 1.2f
	const CKnownFile* pSingleSelFile = NULL;
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		const CKnownFile* pFile = (CKnownFile*)GetItemData(GetNextSelectedItem(pos));
		if (bFirstItem)
			pSingleSelFile = pFile;
		else
			pSingleSelFile = NULL;

		int iCurCompleteFile = pFile->IsPartFile() ? 0 : 1;
		if (bFirstItem)
			iCompleteFileSelected = iCurCompleteFile;
		else if (iCompleteFileSelected != iCurCompleteFile)
			iCompleteFileSelected = -1;

		UINT uCurPrioMenuItem = 0;
		if (pFile->IsAutoUpPriority())
			uCurPrioMenuItem = MP_PRIOAUTO;
		else if (pFile->GetUpPriority() == PR_VERYLOW)
			uCurPrioMenuItem = MP_PRIOVERYLOW;
		else if (pFile->GetUpPriority() == PR_LOW)
			uCurPrioMenuItem = MP_PRIOLOW;
		else if (pFile->GetUpPriority() == PR_NORMAL)
			uCurPrioMenuItem = MP_PRIONORMAL;
		else if (pFile->GetUpPriority() == PR_HIGH)
			uCurPrioMenuItem = MP_PRIOHIGH;
		else if (pFile->GetUpPriority() == PR_VERYHIGH)
			uCurPrioMenuItem = MP_PRIOVERYHIGH;
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
		// Mighty Knife: Community visible filelist
		else if (pFile->GetPermissions() == PERM_COMMUNITY)
			uCurPermMenuItem = MP_PERMCOMMUNITY;
		// [end] Mighty Knife
		else if (pFile->GetPermissions() == PERM_NOONE)
			uCurPermMenuItem = MP_PERMNONE;
		else
			ASSERT(0);

		if (bFirstItem)
			uPermMenuItem = uCurPermMenuItem;
		else if (uPermMenuItem != uCurPermMenuItem)
			uPermMenuItem = 0;
		//MORPH END   - Added by SiRoB, showSharePermissions
		//MORPH START - Added by SiRoB, Avoid misusing of powershare
		UINT uCurPowershareMenuItem = 0;
		if (pFile->GetPowerSharedMode()==-1)
			uCurPowershareMenuItem = MP_POWERSHARE_DEFAULT;
		else
			uCurPowershareMenuItem = MP_POWERSHARE_DEFAULT+1 + pFile->GetPowerSharedMode();
		
		if (bFirstItem)
			uPowershareMenuItem = uCurPowershareMenuItem;
		else if (uPowershareMenuItem != uCurPowershareMenuItem)
			uPowershareMenuItem = 0;
		//MORPH END   - Added by SiRoB, Avoid misusing of powershare
		
		//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
		UINT uCurSpreadbarMenuItem = 0;
		if(pFile->GetSpreadbarSetStatus() == -1)
			uCurSpreadbarMenuItem = MP_SPREADBAR_DEFAULT;
		else if(pFile->GetSpreadbarSetStatus() == 0)
			uCurSpreadbarMenuItem = MP_SPREADBAR_OFF;
		else if(pFile->GetSpreadbarSetStatus() == 1)
			uCurSpreadbarMenuItem = MP_SPREADBAR_ON;
		else
			ASSERT(0);

		if (bFirstItem)
			uSpreadbarMenuItem = uCurSpreadbarMenuItem;
		else if (uSpreadbarMenuItem != uCurSpreadbarMenuItem)
			uSpreadbarMenuItem = 0;
		//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file

		//MORPH START - Added by SiRoB, POWERSHARE Limit
		UINT uCurPowerShareLimitMenuItem = 0;
		int iCurPowerShareLimit = pFile->GetPowerShareLimit();
		if (iCurPowerShareLimit==-1)
			uCurPowerShareLimitMenuItem = MP_POWERSHARE_LIMIT;
		else
			uCurPowerShareLimitMenuItem = MP_POWERSHARE_LIMIT_SET;
		
		if (bFirstItem)
		{
			uPowerShareLimitMenuItem = uCurPowerShareLimitMenuItem;
			iPowerShareLimit = iCurPowerShareLimit;
		}
		else if (uPowerShareLimitMenuItem != uCurPowerShareLimitMenuItem || iPowerShareLimit != iCurPowerShareLimit)
		{
			uPowerShareLimitMenuItem = 0;
			iPowerShareLimit = -1;
		}
		//MORPH END   - Added by SiRoB, POWERSHARE Limit
		
		//MORPH START - Added by SiRoB, HIDEOS
		UINT uCurHideOSMenuItem = 0;
		int iCurHideOS = pFile->GetHideOS();
		if (iCurHideOS == -1)
			uCurHideOSMenuItem = MP_HIDEOS_DEFAULT;
		else
			uCurHideOSMenuItem = MP_HIDEOS_SET;
		if (bFirstItem)
		{
			uHideOSMenuItem = uCurHideOSMenuItem;
			iHideOS = iCurHideOS;
		}
		else if (uHideOSMenuItem != uCurHideOSMenuItem || iHideOS != iCurHideOS)
		{
			uHideOSMenuItem = 0;
			iHideOS = -1;
		}
		
		UINT uCurSelectiveChunkMenuItem = 0;
		if (pFile->GetSelectiveChunk() == -1)
			uCurSelectiveChunkMenuItem = MP_SELECTIVE_CHUNK;
		else
			uCurSelectiveChunkMenuItem = MP_SELECTIVE_CHUNK+1 + pFile->GetSelectiveChunk();
		if (bFirstItem)
			uSelectiveChunkMenuItem = uCurSelectiveChunkMenuItem;
		else if (uSelectiveChunkMenuItem != uCurSelectiveChunkMenuItem)
			uSelectiveChunkMenuItem = 0;
		//MORPH END   - Added by SiRoB, HIDEOS

		//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
		UINT uCurShareOnlyTheNeedMenuItem = 0;
		if (pFile->GetShareOnlyTheNeed() == -1)
			uCurShareOnlyTheNeedMenuItem = MP_SHAREONLYTHENEED;
		else
			uCurShareOnlyTheNeedMenuItem = MP_SHAREONLYTHENEED+1 + pFile->GetShareOnlyTheNeed();
		if (bFirstItem)
			uShareOnlyTheNeedMenuItem = uCurShareOnlyTheNeedMenuItem ;
		else if (uShareOnlyTheNeedMenuItem != uCurShareOnlyTheNeedMenuItem)
			uShareOnlyTheNeedMenuItem = 0;
		//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

		//MORPH START - Added by SiRoB, WebCache 1.2f
		//jp webcache release start
		if (!pFile->ReleaseViaWebCache)
			uWCReleaseItem = false;
		if (!pFile->IsPartFile())
			uGreyOutWCRelease = false;
		//jp webcache release end
		//MORPH END  - Added by SiRoB, WebCache 1.2f

		bFirstItem = false;
	}

	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_PrioMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	m_PrioMenu.CheckMenuRadioItem(MP_PRIOVERYLOW, MP_PRIOAUTO, uPrioMenuItem, 0);

	//MORPH START - Added by SiRoB, WebCache 1.2f
	//jp webcache release START
	m_PrioMenu.EnableMenuItem(MP_PRIOWCRELEASE, (thePrefs.UpdateWebcacheReleaseAllowed() && !uGreyOutWCRelease) ? MF_ENABLED : MF_GRAYED);
	if (uWCReleaseItem && thePrefs.IsWebcacheReleaseAllowed()) //JP webcache release
		m_PrioMenu.CheckMenuItem(MP_PRIOWCRELEASE, MF_CHECKED);
	else
		m_PrioMenu.CheckMenuItem(MP_PRIOWCRELEASE, MF_UNCHECKED);
	//jp webcache relesae END
	//MORPH END   - Added by SiRoB, WebCache 1.2f


	bool bSingleCompleteFileSelected = (iSelectedItems == 1 && iCompleteFileSelected == 1);
	m_SharedFilesMenu.EnableMenuItem(MP_OPEN, bSingleCompleteFileSelected ? MF_ENABLED : MF_GRAYED);
	UINT uInsertedMenuItem = 0;
	static const TCHAR _szSkinPkgSuffix1[] = _T(".") EMULSKIN_BASEEXT _T(".zip");
	static const TCHAR _szSkinPkgSuffix2[] = _T(".") EMULSKIN_BASEEXT _T(".rar");
	if (bSingleCompleteFileSelected 
		&& pSingleSelFile 
		&& (   pSingleSelFile->GetFilePath().Right(ARRSIZE(_szSkinPkgSuffix1)-1).CompareNoCase(_szSkinPkgSuffix1) == 0
		    || pSingleSelFile->GetFilePath().Right(ARRSIZE(_szSkinPkgSuffix2)-1).CompareNoCase(_szSkinPkgSuffix2) == 0))
	{
		MENUITEMINFO mii = {0};
		mii.cbSize = sizeof mii;
		mii.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
		mii.fType = MFT_STRING;
		mii.fState = MFS_ENABLED;
		mii.wID = MP_INSTALL_SKIN;
		CString strBuff(GetResString(IDS_INSTALL_SKIN));
		mii.dwTypeData = const_cast<LPTSTR>((LPCTSTR)strBuff);
		if (::InsertMenuItem(m_SharedFilesMenu, MP_OPENFOLDER, FALSE, &mii))
			uInsertedMenuItem = mii.wID;
	}
	//MORPH - Changed by SIRoB, Open temp folder 
	/*
	m_SharedFilesMenu.EnableMenuItem(MP_OPENFOLDER, bSingleCompleteFileSelected ? MF_ENABLED : MF_GRAYED);
	*/
	m_SharedFilesMenu.EnableMenuItem(MP_OPENFOLDER, iSelectedItems==1 ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(MP_RENAME, bSingleCompleteFileSelected ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(MP_REMOVE, iCompleteFileSelected > 0 ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.SetDefaultItem(bSingleCompleteFileSelected ? MP_OPEN : -1);
	m_SharedFilesMenu.EnableMenuItem(MP_CMT, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(MP_DETAIL, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(thePrefs.GetShowCopyEd2kLinkCmd() ? MP_GETED2KLINK : MP_SHOWED2KLINK, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(MP_FIND, GetItemCount() > 0 ? MF_ENABLED : MF_GRAYED);

	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_SpreadbarMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	switch (thePrefs.GetSpreadbarSetStatus()){
		case 0:
			buffer.Format(_T(" (%s)"),GetResString(IDS_DISABLED));
			break;
		case 1:
			buffer.Format(_T(" (%s)"),GetResString(IDS_ENABLED));
			break;
		default:
			buffer = _T(" (?)");
			break;
	}
	m_SharedFilesMenu.ModifyMenu(MP_SPREADBAR_DEFAULT, MF_STRING, MP_SPREADBAR_DEFAULT, GetResString(IDS_DEFAULT) + buffer);
	m_SharedFilesMenu.CheckMenuRadioItem(MP_SPREADBAR_DEFAULT, MP_SPREADBAR_ON, uSpreadbarMenuItem, 0);
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	
	//MORPH START - Added by SiRoB, HIDEOS
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_HideOSMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_SelectiveChunkMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	if (thePrefs.GetHideOvershares()==0)
		buffer.Format(_T(" (%s)"),GetResString(IDS_DISABLED));
	else
		buffer.Format(_T(" (%u)"),thePrefs.GetHideOvershares());
	m_HideOSMenu.ModifyMenu(MP_HIDEOS_DEFAULT, MF_STRING,MP_HIDEOS_DEFAULT, GetResString(IDS_DEFAULT) + buffer);
	if (iHideOS==-1)
		buffer = GetResString(IDS_EDIT);
	else if (iHideOS==0)
		buffer = GetResString(IDS_DISABLED);
	else
		buffer.Format(_T("%i"), iHideOS);
	m_HideOSMenu.ModifyMenu(MP_HIDEOS_SET, MF_STRING,MP_HIDEOS_SET, buffer);
	m_HideOSMenu.CheckMenuRadioItem(MP_HIDEOS_DEFAULT, MP_HIDEOS_SET, uHideOSMenuItem, 0);
	buffer.Format(_T(" (%s)"),thePrefs.IsSelectiveShareEnabled()?GetResString(IDS_ENABLED):GetResString(IDS_DISABLED));
	m_SelectiveChunkMenu.ModifyMenu(MP_SELECTIVE_CHUNK, MF_STRING, MP_SELECTIVE_CHUNK, GetResString(IDS_DEFAULT) + buffer);
	m_SelectiveChunkMenu.CheckMenuRadioItem(MP_SELECTIVE_CHUNK, MP_SELECTIVE_CHUNK_1, uSelectiveChunkMenuItem, 0);
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_ShareOnlyTheNeedMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	buffer.Format(_T(" (%s)"),thePrefs.GetShareOnlyTheNeed()?GetResString(IDS_ENABLED):GetResString(IDS_DISABLED));
	m_ShareOnlyTheNeedMenu.ModifyMenu(MP_SHAREONLYTHENEED, MF_STRING, MP_SHAREONLYTHENEED, GetResString(IDS_DEFAULT) + buffer);
	m_ShareOnlyTheNeedMenu.CheckMenuRadioItem(MP_SHAREONLYTHENEED, MP_SHAREONLYTHENEED_1, uShareOnlyTheNeedMenuItem, 0);
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

	//MORPH START - Added by SiRoB, Show Share Permissions
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_PermMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
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
	m_PermMenu.CheckMenuRadioItem(MP_PERMDEFAULT,MP_PERMCOMMUNITY, uPermMenuItem, 0);
	// [end] Mighty Knife
	//MORPH END   - Added by SiRoB, Show Share Permissions

	//MORPH START - Added by SiRoB, Avoid misusing of powershare
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_PowershareMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	switch (thePrefs.GetPowerShareMode()){
		case 0:
			buffer.Format(_T(" (%s)"),GetResString(IDS_POWERSHARE_DISABLED));
			break;
		case 1:
			buffer.Format(_T(" (%s)"),GetResString(IDS_POWERSHARE_ACTIVATED));
			break;
		case 2:
			buffer.Format(_T(" (%s)"),GetResString(IDS_POWERSHARE_AUTO));
			break;
		case 3:
			buffer.Format(_T(" (%s)"),GetResString(IDS_POWERSHARE_LIMITED));
			break;
		default:
			buffer = _T(" (?)");
			break;
	}
	m_PowershareMenu.ModifyMenu(MP_POWERSHARE_DEFAULT, MF_STRING,MP_POWERSHARE_DEFAULT, GetResString(IDS_DEFAULT) + buffer);
	m_PowershareMenu.CheckMenuRadioItem(MP_POWERSHARE_DEFAULT, MP_POWERSHARE_LIMITED, uPowershareMenuItem, 0);
	//MORPH END   - Added by SiRoB, Avoid misusing of powershare
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	m_PowershareMenu.EnableMenuItem((UINT_PTR)m_PowerShareLimitMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	if (iPowerShareLimit==0)
		buffer.Format(_T(" (%s)"),GetResString(IDS_DISABLED));
	else
		buffer.Format(_T(" (%u)"),thePrefs.GetPowerShareLimit());
	m_PowerShareLimitMenu.ModifyMenu(MP_POWERSHARE_LIMIT, MF_STRING,MP_POWERSHARE_LIMIT, GetResString(IDS_DEFAULT) + buffer);
	if (iPowerShareLimit==-1)
		buffer = GetResString(IDS_EDIT);
	else if (iPowerShareLimit==0)
		buffer = GetResString(IDS_DISABLED);
	else
		buffer.Format(_T("%i"),iPowerShareLimit);
	m_PowerShareLimitMenu.ModifyMenu(MP_POWERSHARE_LIMIT_SET, MF_STRING,MP_POWERSHARE_LIMIT_SET, buffer);
	m_PowerShareLimitMenu.CheckMenuRadioItem(MP_POWERSHARE_LIMIT, MP_POWERSHARE_LIMIT_SET, uPowerShareLimitMenuItem, 0);
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	
	//MORPH START - Added by SiRoB, Mighty Knife: CRC32-Tag
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_CRC32Menu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	//MORPH END   - Added by SiRoB, [end] Mighty Knife
	//MORPH START - Added by SiRoB, Mighty Knife: Mass Rename
	m_SharedFilesMenu.EnableMenuItem(MP_MASSRENAME, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	//MORPH END   - Added by SiRoB, [end] Mighty Knife

	m_CollectionsMenu.EnableMenuItem(MP_MODIFYCOLLECTION, ( pSingleSelFile != NULL && pSingleSelFile->m_pCollection != NULL ) ? MF_ENABLED : MF_GRAYED);
	m_CollectionsMenu.EnableMenuItem(MP_VIEWCOLLECTION, ( pSingleSelFile != NULL && pSingleSelFile->m_pCollection != NULL ) ? MF_ENABLED : MF_GRAYED);
	m_CollectionsMenu.EnableMenuItem(MP_SEARCHAUTHOR, ( pSingleSelFile != NULL && pSingleSelFile->m_pCollection != NULL && !pSingleSelFile->m_pCollection->GetAuthorKeyHashString().IsEmpty()) ? MF_ENABLED : MF_GRAYED);
#if defined(_DEBUG)
	if (thePrefs.IsExtControlsEnabled()){
	//JOHNTODO: Not for release as we need kad lowID users in the network to see how well this work work. Also, we do not support these links yet.
	if (iSelectedItems > 0 && theApp.IsConnected() && theApp.IsFirewalled() && theApp.clientlist->GetBuddy())
		m_SharedFilesMenu.EnableMenuItem(MP_GETKADSOURCELINK, MF_ENABLED);
	else
		m_SharedFilesMenu.EnableMenuItem(MP_GETKADSOURCELINK, MF_GRAYED);
	}
#endif
	m_SharedFilesMenu.EnableMenuItem(Irc_SetSendLink, iSelectedItems == 1 && theApp.emuledlg->ircwnd->IsConnected() ? MF_ENABLED : MF_GRAYED);

	//MORPH START - Added by IceCream, copy feedback feature
	m_SharedFilesMenu.EnableMenuItem(MP_COPYFEEDBACK, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(MP_COPYFEEDBACK_US, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	//MORPH END   - Added by IceCream, copy feedback feature
	
	CTitleMenu WebMenu;
	WebMenu.CreateMenu();
	WebMenu.AddMenuTitle(NULL, true);
	int iWebMenuEntries = theWebServices.GetFileMenuEntries(&WebMenu);
	UINT flag2 = (iWebMenuEntries == 0 || iSelectedItems != 1) ? MF_GRAYED : MF_STRING;
	m_SharedFilesMenu.AppendMenu(flag2 | MF_POPUP, (UINT_PTR)WebMenu.m_hMenu, GetResString(IDS_WEBSERVICES), _T("WEB"));
	
	GetPopupMenuPos(*this, point);
	m_SharedFilesMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON,point.x,point.y,this);

	m_SharedFilesMenu.RemoveMenu(m_SharedFilesMenu.GetMenuItemCount()-1,MF_BYPOSITION);
	VERIFY( WebMenu.DestroyMenu() );
	if (uInsertedMenuItem)
		VERIFY( m_SharedFilesMenu.RemoveMenu(uInsertedMenuItem, MF_BYCOMMAND) );
}

BOOL CSharedFilesCtrl::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
	wParam = LOWORD(wParam);

	CTypedPtrList<CPtrList, CKnownFile*> selectedList;
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos != NULL){
		int index = GetNextSelectedItem(pos);
		if (index >= 0)
			selectedList.AddTail((CKnownFile*)GetItemData(index));
	}

	if (   wParam == MP_CREATECOLLECTION
		|| wParam == MP_FIND
		|| selectedList.GetCount() > 0)
	{
		CKnownFile* file = NULL;
		if (selectedList.GetCount() == 1)
			file = selectedList.GetHead();

		switch (wParam){
			case Irc_SetSendLink:
				if (file)
				theApp.emuledlg->ircwnd->SetSendFileString(CreateED2kLink(file));
				break;
			case MP_GETED2KLINK:{
				CString str;
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if (!str.IsEmpty())
						str += _T("\r\n");
					str += CreateED2kLink(file);
					}
				theApp.CopyTextToClipboard(str);
				break;
			}
			#if defined(_DEBUG)
			//JOHNTODO: Not for release as we need kad lowID users in the network to see how well this work work. Also, we do not support these links yet.
			case MP_GETKADSOURCELINK:{
				CString str;
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if (!str.IsEmpty())
						str += _T("\r\n");
					str += theApp.CreateKadSourceLink(file);
				}
				theApp.CopyTextToClipboard(str);
				break; 
			}
			#endif
			// file operations
			case MP_OPEN:
			case IDA_ENTER:
				if (file && !file->IsPartFile())
				OpenFile(file);
				break; 
			case MP_INSTALL_SKIN:
				if (file && !file->IsPartFile())
					InstallSkin(file->GetFilePath());
				break;
			case MP_OPENFOLDER:
				if (file && !file->IsPartFile())
					ShellExecute(NULL, _T("open"), file->GetPath(), NULL, NULL, SW_SHOW);
				break; 
			case MP_RENAME:
			case MPG_F2:
				if (file && !file->IsPartFile()){
					InputBox inputbox;
					CString title = GetResString(IDS_RENAME);
					title.Remove(_T('&'));
					inputbox.SetLabels(title, GetResString(IDS_DL_FILENAME), file->GetFileName());
					inputbox.SetEditFilenameMode();
					inputbox.DoModal();
					CString newname = inputbox.GetInput();
					if (!inputbox.WasCancelled() && newname.GetLength()>0)
					{
						// at least prevent users from specifying something like "..\dir\file"
						static const TCHAR _szInvFileNameChars[] = _T("\\/:*?\"<>|");
						if (newname.FindOneOf(_szInvFileNameChars) != -1){
							AfxMessageBox(GetErrorMessage(ERROR_BAD_PATHNAME));
							break;
						}

						CString newpath;
						PathCombine(newpath.GetBuffer(MAX_PATH), file->GetPath(), newname);
						newpath.ReleaseBuffer();
						if (_trename(file->GetFilePath(), newpath) != 0){
							CString strError;
							/*UNICODE FIX*/strError.Format(GetResString(IDS_ERR_RENAMESF), file->GetFilePath(), newpath, _tcserror(errno));
							AfxMessageBox(strError);
							break;
						}
						
						if (file->IsKindOf(RUNTIME_CLASS(CPartFile)))
							file->SetFileName(newname);
						else
						{
						theApp.sharedfiles->RemoveKeywords(file);
						file->SetFileName(newname);
						theApp.sharedfiles->AddKeywords(file);
						}
						file->SetFilePath(newpath);
						UpdateFile(file);
					}
				}
				else
					MessageBeep(MB_OK);
				break;
			case MP_REMOVE:
			case MPG_DELETE:
			{
				if (IDNO == AfxMessageBox(GetResString(IDS_CONFIRM_FILEDELETE),MB_ICONWARNING | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_YESNO))
					return TRUE;

				SetRedraw(FALSE);
				bool bRemovedItems = false;
				while (!selectedList.IsEmpty())
				{
					CKnownFile* myfile = selectedList.RemoveHead();
					if (!myfile || myfile->IsPartFile())
						continue;
					
					BOOL delsucc = FALSE;
					if (!PathFileExists(myfile->GetFilePath()))
						delsucc = TRUE;
					else{
						// Delete
						if (!thePrefs.GetRemoveToBin()){
							delsucc = DeleteFile(myfile->GetFilePath());
						}
						else{
							// delete to recycle bin :(
							TCHAR todel[MAX_PATH+1];
							memset(todel, 0, sizeof todel);
							_tcsncpy(todel, myfile->GetFilePath(), ARRSIZE(todel)-2);

							SHFILEOPSTRUCT fp = {0};
							fp.wFunc = FO_DELETE;
							fp.hwnd = theApp.emuledlg->m_hWnd;
							fp.pFrom = todel;
							fp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;// | FOF_NOERRORUI
							delsucc = (SHFileOperation(&fp) == 0);
						}
					}
					if (delsucc){
						theApp.sharedfiles->RemoveFile(myfile);
						bRemovedItems = true;
						if (myfile->IsKindOf(RUNTIME_CLASS(CPartFile)))
							theApp.emuledlg->transferwnd->downloadlistctrl.ClearCompleted(static_cast<CPartFile*>(myfile));
					}
					else{
						CString strError;
						strError.Format( GetResString(IDS_ERR_DELFILE) + _T("\r\n\r\n%s"), myfile->GetFilePath(), GetErrorMessage(GetLastError()));
						AfxMessageBox(strError);
					}
				}
				SetRedraw(TRUE);
				if (bRemovedItems)
					AutoSelectItem();
				break; 
			}
			case MP_CMT:
				ShowFileDialog(selectedList, IDD_COMMENT);
                break; 
			case MPG_ALTENTER:
			case MP_DETAIL:
				ShowFileDialog(selectedList);
				break;
			case MP_FIND:
				OnFindStart();
				break;
			case MP_CREATECOLLECTION:
			{
				CCollection* pCollection = new CCollection();
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					pCollection->AddFileToCollection(selectedList.GetNext(pos),true);
				}
				CCollectionCreateDialog dialog;
				dialog.SetCollection(pCollection,true);
				dialog.DoModal();
				//We delete this collection object because when the newly created
				//collection file is added to the sharedfile list, it is read and verified
				//and which creates the colleciton object that is attached to that file..
				delete pCollection;
				break;
			}
			case MP_SEARCHAUTHOR:
				if (selectedList.GetCount() == 1 && file->m_pCollection)
				{
					SSearchParams* pParams = new SSearchParams;
					pParams->strExpression = file->m_pCollection->GetCollectionAuthorKeyString();
					pParams->eType = SearchTypeKademlia;
					pParams->strFileType = ED2KFTSTR_EMULECOLLECTION;
					pParams->strSpecialTitle = file->m_pCollection->m_sCollectionAuthorName;
					if (pParams->strSpecialTitle.GetLength() > 50){
						pParams->strSpecialTitle = pParams->strSpecialTitle.Left(50) + _T("...");
					}
					theApp.emuledlg->searchwnd->m_pwndResults->StartSearch(pParams);
				}
				break;
			case MP_VIEWCOLLECTION:
				if (selectedList.GetCount() == 1 && file->m_pCollection)
				{
					CCollectionViewDialog dialog;
					dialog.SetCollection(file->m_pCollection);
					dialog.DoModal();
				}
				break;
			case MP_MODIFYCOLLECTION:
				if (selectedList.GetCount() == 1 && file->m_pCollection)
				{
					CCollectionCreateDialog dialog;
					CCollection* pCollection = new CCollection(file->m_pCollection);
					dialog.SetCollection(pCollection,false);
					dialog.DoModal();
					delete pCollection;				
				}
				break;
			case MP_SHOWED2KLINK:
				ShowFileDialog(selectedList, IDD_ED2KLINK);
				break;
			case MP_PRIOVERYLOW:
			case MP_PRIOLOW:
			case MP_PRIONORMAL:
			case MP_PRIOHIGH:
			case MP_PRIOVERYHIGH:
			//MORPH START - Added by SiRoB, WebCache 1.2f
			case MP_PRIOWCRELEASE: //JP webcache release
			//MORPH END   - Added by SiRoB, WebCache 1.2f
			case MP_PRIOAUTO:
				{
					//MORPH START - Added by SiRoB, WebCache 1.2f
					//jp webcache release START 
					// check if a click on MP_PRIOWCRELEASE should activate WC-release
					bool activateWCRelease = false;
					POSITION pos2 = selectedList.GetHeadPosition();
					CKnownFile* cur_file = NULL;
					while (pos2 != NULL)
					{
						cur_file = selectedList.GetNext(pos2);
						if (!cur_file->ReleaseViaWebCache)
							activateWCRelease = true;
					}
					//jp webcache release END
					//MORPH END   - Added by SiRoB, WebCache 1.2f

					POSITION pos = selectedList.GetHeadPosition();
					while (pos != NULL)
					{
						CKnownFile* file = selectedList.GetNext(pos);
						switch (wParam) {
							case MP_PRIOVERYLOW:
								file->SetAutoUpPriority(false);
								file->SetUpPriority(PR_VERYLOW);
								UpdateFile(file);
								break;
							case MP_PRIOLOW:
								file->SetAutoUpPriority(false);
								file->SetUpPriority(PR_LOW);
								UpdateFile(file);
								break;
							case MP_PRIONORMAL:
								file->SetAutoUpPriority(false);
								file->SetUpPriority(PR_NORMAL);
								UpdateFile(file);
								break;
							case MP_PRIOHIGH:
								file->SetAutoUpPriority(false);
								file->SetUpPriority(PR_HIGH);
								UpdateFile(file);
								break;
							case MP_PRIOVERYHIGH:
								file->SetAutoUpPriority(false);
								file->SetUpPriority(PR_VERYHIGH);
								UpdateFile(file);
								break;	
							case MP_PRIOAUTO:
								//MORPH START - Added by SiRoB, force savepart to update auto up flag since i removed the update in UpdateAutoUpPriority optimization
								if(file->IsPartFile() && !file->IsAutoUpPriority()){
									file->SetAutoUpPriority(true);
									((CPartFile*)file)->SavePartFile();
								}else
								//MORPH END   - Added by SiRoB, force savepart to update auto up flag since i removed the update in UpdateAutoUpPriority optimization
								file->SetAutoUpPriority(true);
								file->UpdateAutoUpPriority();
								UpdateFile(file); 
								break;
							//MORPH START - Added by SiRoB, WebCache 1.2f
							//jp webcache release start
							case MP_PRIOWCRELEASE:
								if (!file->IsPartFile())
									file->SetReleaseViaWebCache(activateWCRelease);
								else
									file->SetReleaseViaWebCache(false);
								break;
							//jp webcache release end
							//MORPH END   - Added by SiRoB, WebCache 1.2f

						}
					}
					break;

				}
			//MORPH START - Added by IceCream, copy feedback feature
 			case MP_COPYFEEDBACK:
			{
				CString feed;
				feed.AppendFormat(GetResString(IDS_FEEDBACK_FROM), thePrefs.GetUserNick(), theApp.m_strModLongVersion);
				feed.AppendFormat(_T(" \r\n"));
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					CKnownFile* file = selectedList.GetNext(pos);
					feed.Append(file->GetFeedback());
					feed.Append(_T(" \r\n"));
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

			//MORPH START - Added by SiRoB, ZZ Upload System
			case MP_POWERSHARE_ON:
			case MP_POWERSHARE_OFF:
			//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
			case MP_POWERSHARE_DEFAULT:
			case MP_POWERSHARE_AUTO:
			case MP_POWERSHARE_LIMITED: //MORPH - Added by SiRoB, POWERSHARE Limit
			{
				SetRedraw(FALSE);
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					switch (wParam) {
						case MP_POWERSHARE_DEFAULT:
							file->SetPowerShared(-1);
							break;
						case MP_POWERSHARE_ON:
							file->SetPowerShared(1);
							break;
						case MP_POWERSHARE_OFF:
							file->SetPowerShared(0);
							break;
						case MP_POWERSHARE_AUTO:
							file->SetPowerShared(2);
							break;
						//MORPH START - Added by SiRoB, POWERSHARE Limit
						case MP_POWERSHARE_LIMITED:
							file->SetPowerShared(3);
							break;
						//MORPH END   - Added by SiRoB, POWERSHARE Limit
					}
					UpdateFile(file);
				}
				SetRedraw(TRUE);
				theApp.uploadqueue->ReSortUploadSlots(true);
				break;
			}
			//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
			//MORPH END   - Added by SiRoB, ZZ Upload System
			//MORPH START - Added by SiRoB, POWERSHARE Limit
			case MP_POWERSHARE_LIMIT:
			case MP_POWERSHARE_LIMIT_SET:
			{
				POSITION pos = selectedList.GetHeadPosition();
				int newPowerShareLimit = -1;
				if (wParam==MP_POWERSHARE_LIMIT_SET)
				{
					InputBox inputbox;
					CString title=GetResString(IDS_POWERSHARE);
					CString currPowerShareLimit;
					if (file)
						currPowerShareLimit.Format(_T("%i"), (file->GetPowerShareLimit()>=0)?file->GetPowerShareLimit():thePrefs.GetPowerShareLimit());
					else
						currPowerShareLimit = _T("0");
					inputbox.SetLabels(GetResString(IDS_POWERSHARE), GetResString(IDS_POWERSHARE_LIMIT), currPowerShareLimit);
					inputbox.SetNumber(true);
					int result = inputbox.DoModal();
					if (result == IDCANCEL || (newPowerShareLimit = inputbox.GetInputInt()) < 0)
						break;
				}
				SetRedraw(FALSE);
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if  (newPowerShareLimit == file->GetPowerShareLimit()) break;
					file->SetPowerShareLimit(newPowerShareLimit);
					if (file->IsPartFile())
						((CPartFile*)file)->UpdatePartsInfo();
					else
						file->UpdatePartsInfo();
					UpdateFile(file);
				}
				SetRedraw(TRUE);
				break;
			}
			//MORPH END   - Added by SiRoB, POWERSHARE Limit
			//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
			case MP_SPREADBAR_DEFAULT:
			case MP_SPREADBAR_OFF:
			case MP_SPREADBAR_ON:
			{
				SetRedraw(FALSE);
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					switch (wParam) {
						case MP_SPREADBAR_DEFAULT:
							file->SetSpreadbarSetStatus(-1);
							break;
						case MP_SPREADBAR_OFF:
							file->SetSpreadbarSetStatus(0);
							break;
						case MP_SPREADBAR_ON:
							file->SetSpreadbarSetStatus(1);
							break;
						default:
							file->SetSpreadbarSetStatus(-1);
							break;
					}
					UpdateFile(file);
				}
				SetRedraw(TRUE);
				break;
			}
			case MP_SPREADBAR_RESET:
			{
				SetRedraw(FALSE);
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					file->statistic.ResetSpreadBar();
				}
				SetRedraw(TRUE);
			}
			//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
			//MORPH START - Added by SiRoB, HIDEOS
			case MP_HIDEOS_DEFAULT:
			case MP_HIDEOS_SET:
			{
				POSITION pos = selectedList.GetHeadPosition();
				int newHideOS = -1;
				if (wParam==MP_HIDEOS_SET)
				{
					InputBox inputbox;
					CString title=GetResString(IDS_HIDEOS);
					CString currHideOS;
					if (file)
						currHideOS.Format(_T("%i"), (file->GetHideOS()>=0)?file->GetHideOS():thePrefs.GetHideOvershares());
					else
						currHideOS = _T("0");
					inputbox.SetLabels(GetResString(IDS_HIDEOS), GetResString(IDS_HIDEOVERSHARES), currHideOS);
					inputbox.SetNumber(true);
					int result = inputbox.DoModal();
					if (result == IDCANCEL || (newHideOS = inputbox.GetInputInt()) < 0)
						break;
				}
				SetRedraw(FALSE);
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if  (newHideOS == file->GetHideOS()) break;
					file->SetHideOS(newHideOS);
					UpdateFile(file);
				}
				SetRedraw(TRUE);
				break;
			}
			//MORPH END   - Added by SiRoB, HIDEOS
			// xMule_MOD: showSharePermissions
			// with itsonlyme's sorting fix
			case MP_PERMDEFAULT:
			case MP_PERMNONE:
			case MP_PERMFRIENDS:
			// Mighty Knife: Community visible filelist
			case MP_PERMCOMMUNITY:
			// [end] Mighty Knife
			case MP_PERMALL: {
				SetRedraw(FALSE);
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
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
					UpdateFile(file);
				}
				SetRedraw(TRUE);
				Invalidate();
				break;
			}
		    // Mighty Knife: CRC32-Tag
			case MP_CRC32_RECALCULATE: 
				// Remove existing CRC32 tags from the selected files
				if (!selectedList.IsEmpty()){
					POSITION pos = selectedList.GetHeadPosition();
					while (pos != NULL) {
						CKnownFile* file = selectedList.GetAt (pos);
						file->SetLastCalculatedCRC32 (_T(""));
						//UpdateFile(file);
						selectedList.GetNext (pos);
					}
				}
				// Repaint the list 
				Invalidate();
				// !!! NO "break;" HERE !!!
				// This case branch must lead into the MP_CRC32_CALCULATE branch - 
				// so after removing the CRC's from the selected files they
				// are immediately recalculated!
			case MP_CRC32_CALCULATE: 
				if (!selectedList.IsEmpty()){
					// For every chosen file create a worker thread and add it
					// to the file processing thread
					POSITION pos = selectedList.GetHeadPosition();
					while (pos != NULL) {
						CCRC32CalcWorker* worker = new CCRC32CalcWorker;
						CKnownFile*  file = selectedList.GetAt (pos);
						const uchar* FileHash = file->GetFileHash ();
						worker->SetFileHashToProcess (FileHash);
						worker->SetFilePath (file->GetFilePath ());
						m_FileProcessingThread.AddFileProcessingWorker (worker);
						selectedList.GetNext (pos);
					}
					// Start the file processing thread to process the files.
					if (!m_FileProcessingThread.IsRunning ()) {
						// (Re-)start the thread, this will do the rest

						// If the thread object already exists, this will result in an
						// ASSERT - but that doesn't matter since we manually take care
						// that the thread does not run at this moment...
						m_FileProcessingThread.CreateThread ();
					}
				}
				break;
			case MP_CRC32_ABORT:
				// Message the File processing thread to stop any pending calculations
				if (m_FileProcessingThread.IsRunning ())
					m_FileProcessingThread.Terminate ();
				break;
			case MP_CRC32_TAG:
				if (!selectedList.IsEmpty()){
					AddCRC32InputBox AddCRCDialog;
					int result = AddCRCDialog.DoModal ();
					if (result == IDOK) {
						// For every chosen file create a worker thread and add it
						// to the file processing thread
						POSITION pos = selectedList.GetHeadPosition();
						while (pos != NULL) {
							// first create a worker thread that calculates the CRC
							// if it's not already calculated...
							CKnownFile*  file = selectedList.GetAt (pos);
							const uchar* FileHash = file->GetFileHash ();

							// But don't add the worker thread if the CRC32 should not
							// be calculated !
							if (!AddCRCDialog.GetDontAddCRC32 ()) {
								CCRC32CalcWorker* workercrc = new CCRC32CalcWorker;
								workercrc->SetFileHashToProcess (FileHash);
								workercrc->SetFilePath (file->GetFilePath ());
								m_FileProcessingThread.AddFileProcessingWorker (workercrc);
							}

							// Now add a worker thread that informs this window when
							// the calculation is completed.
							// The method OnCRC32RenameFilename will then rename the file
							CCRC32RenameWorker* worker = new CCRC32RenameWorker;
							worker->SetFileHashToProcess (FileHash);
							worker->SetFilePath (file->GetFilePath ());
							worker->SetFilenamePrefix (AddCRCDialog.GetCRC32Prefix ());
							worker->SetFilenameSuffix (AddCRCDialog.GetCRC32Suffix ());
							worker->SetDontAddCRCAndSuffix (AddCRCDialog.GetDontAddCRC32 ());
							worker->SetCRC32ForceUppercase (AddCRCDialog.GetCRC32ForceUppercase ());
							worker->SetCRC32ForceAdding (AddCRCDialog.GetCRC32ForceAdding ());
							m_FileProcessingThread.AddFileProcessingWorker (worker);

							// next file
							selectedList.GetNext (pos);
						}
						// Start the file processing thread to process the files.
						if (!m_FileProcessingThread.IsRunning ()) {
							// (Re-)start the thread, this will do the rest

							// If the thread object already exists, this will result in an
							// ASSERT - but that doesn't matter since we manually take care
							// that the thread does not run at this moment...
							m_FileProcessingThread.CreateThread ();
						}
					}
				}
				break;
			// [end] Mighty Knife

			// Mighty Knife: Mass Rename
			case MP_MASSRENAME: {
					CMassRenameDialog MRDialog;
					// Add the files to the dialog
					POSITION pos = selectedList.GetHeadPosition();
					while (pos != NULL) {
						CKnownFile*  file = selectedList.GetAt (pos);
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
							CKnownFile* file = selectedList.GetAt (pos);
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
									file->SetFileName(newname);
									theApp.sharedfiles->AddKeywords(file);
									file->SetFilePath(newpath);
									UpdateFile(file);
								} else {
									// Use the "Format"-Syntax of AddLogLine here instead of
									// CString.Format+AddLogLine, because if "%"-characters are
									// in the string they would be misinterpreted as control sequences!
									AddLogLine(false,_T("Successfully renamed .part file '%s' to '%s'"), file->GetFileName(), newname);
									file->SetFileName(newname, true); 
									((CPartFile*) file)->UpdateDisplayedInfo();
									((CPartFile*) file)->SavePartFile(); 
									UpdateFile(file);
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
			// xMule_MOD: showSharePermissions
			default:
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if (wParam>=MP_WEBURL && wParam<=MP_WEBURL+256){
						theWebServices.RunURL(file, wParam);
					}
					//MORPH START - Added by SiRoB, HIDEOS
					else if (wParam>=MP_SELECTIVE_CHUNK && wParam<=MP_SELECTIVE_CHUNK_1){
						file->SetSelectiveChunk(wParam==MP_SELECTIVE_CHUNK?-1:wParam-MP_SELECTIVE_CHUNK_0);
						UpdateFile(file);
					//MORPH END   - Added by SiRoB, HIDEOS
					//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
					}else if (wParam>=MP_SHAREONLYTHENEED && wParam<=MP_SHAREONLYTHENEED_1){
						file->SetShareOnlyTheNeed(wParam==MP_SHAREONLYTHENEED?-1:wParam-MP_SHAREONLYTHENEED_0);
						UpdateFile(file);
					//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
					}
				}
				break;
		}
	}
	return TRUE;
}

// Mighty Knife: CRC32-Tag - Save rename
// File will be renamed in this method at the time it can be renamed.
// Might be that the CRC had to be calculated before so the thread will inform
// this window when the CRC is ok and the file can be renamed...
afx_msg LRESULT CSharedFilesCtrl::OnCRC32RenameFile	(WPARAM /*wParam*/, LPARAM lParam) {
	// Get the worker thread
	CCRC32RenameWorker* worker = (CCRC32RenameWorker*) lParam;
	// In this case the worker thread is a helper thread for this routine !

	// We are in the "main" thread, so we can be sure that the filelist is not
	// deleted while we access it - so we try to get a pointer to the desired file
	// directly without worker->ValidateKnownFile !
	// This of course avoids possible deadlocks because we don't lock the list;
	// and we don't need to do an worker->UnlockSharedFilesList...
	CKnownFile* f = theApp.sharedfiles->GetFileByID (worker->GetFileHashToProcess ());
	if (f==NULL) {
		// File doesn't exist in the list; deleted and reloaded the shared files list in
		// the meantime ?
		// Let's hope the creator of this Worker thread has set the filename so we can
		// display it...
		if (worker->GetFilePath () == "") {
			AddLogLine (false,_T("Warning: A File that should be renamed is not shared anymore. Renaming skipped."));
		} else {
			AddLogLine (false,_T("Warning: File '%s' is not shared anymore. File is not renamed."),
				worker->GetFilePath ());
		}
		return 0;         
	}
	if (f->IsPartFile () && !worker->m_DontAddCRCAndSuffix) {     
		// We can't add a CRC suffix to files which are not complete
		AddLogLine (false,_T("Can't add CRC to file '%s'; file is a part file and not complete !"),
						   f->GetFileName ());
		return 0;
	}
	if (!worker->m_DontAddCRCAndSuffix && !f->IsCRC32Calculated ()) {
		// The CRC must have been calculate, otherwise we can't add it.
		// Normally this mesage is not shown because if the CRC is not calculated
		// the main thread creates a worker thread before to calculate it...
		AddLogLine (false,_T("Can't add CRC32 to file '%s'; CRC is not calculated !"),
						   f->GetFileName ());
		return 0;
	}

	// Declare the variables we'll need
	CString p3,p4;
	CString NewFn;

	// Split the old filename to name and extension
	CString fn = f->GetFileName ();
	// test if the file name already contained the CRC tag
	CString fnup = fn;
	fnup.MakeUpper();
	if( f->IsCRC32Calculated() && 
		(fnup.Find(f->GetLastCalculatedCRC32()) != -1) &&
		(!worker->m_CRC32ForceAdding) ){
		// Ok, the filename already contains the CRC. Normally we won't rename it, except for
		// we have to make sure it's uppercase
		if ((!worker->m_CRC32ForceUppercase) || (fn.Find(f->GetLastCalculatedCRC32()) != -1)) {
			AddLogLine (false, _T("File '%s' already contains the correct CRC32 tag, won't be renamed."), fn);
			return 0;
		} else {
			// This file contains a valid CRC, but not in uppercase - replace it!
			int i=fnup.Find(f->GetLastCalculatedCRC32());
			NewFn = fn;
			p3 = f->GetLastCalculatedCRC32();
			NewFn.Delete (i,p3.GetLength ());
			NewFn.Insert (i,p3);
		}
	} else {
		// We have to add the CRC32/Releaser tag to the filename.
		_tsplitpath (fn,NULL,NULL,p3.GetBuffer (MAX_PATH),p4.GetBuffer (MAX_PATH));
		p3.ReleaseBuffer();
		p4.ReleaseBuffer();

		// Create the new filename
		NewFn = p3;
		NewFn = NewFn + worker->m_FilenamePrefix;
		if (!worker->m_DontAddCRCAndSuffix) {
			NewFn = NewFn + f->GetLastCalculatedCRC32 () + worker->m_FilenameSuffix;
		}
		NewFn = NewFn + p4;
	}

	AddLogLine (false,_T("File '%s' will be renamed to '%s'..."),fn,NewFn);

	// Add the path of the old filename to the new one
	CString NewPath; 
	PathCombine(NewPath.GetBuffer(MAX_PATH), f->GetPath (), NewFn);
	NewPath.ReleaseBuffer();

	// Try to rename
	if ((!f->IsPartFile()) && (_trename(f->GetFilePath (), NewPath) != 0)) {
		AddLogLine (false,_T("Can't rename file '%s' ! Error: %hs"),fn,_tcserror(errno));
	} else {
		if (!f->IsPartFile()) {
			// Use the "Format"-Syntax of AddLogLine here instead of
			// CString.Format+AddLogLine, because if "%"-characters are
			// in the string they would be misinterpreted as control sequences!
			AddLogLine(false,_T("Successfully renamed file '%s' to '%s'"), f->GetFileName(), NewPath);

			theApp.sharedfiles->RemoveKeywords(f);
			f->SetFileName(NewFn);
			theApp.sharedfiles->AddKeywords(f);
			f->SetFilePath(NewPath);
			UpdateFile (f);
		} else {
			// Use the "Format"-Syntax of AddLogLine here instead of
			// CString.Format+AddLogLine, because if "%"-characters are
			// in the string they would be misinterpreted as control sequences!
			AddLogLine(false,_T("Successfully renamed .part file '%s' to '%s'"), f->GetFileName(), NewFn);

			f->SetFileName(NewFn, true); 
			((CPartFile*) f)->UpdateDisplayedInfo();
			((CPartFile*) f)->SavePartFile(); 
			UpdateFile(f);
		}
	}
	
	return 0;
}

// Update the file which CRC was just calculated.
// The LPARAM parameter is a pointer to the hash of the file to be updated.
LRESULT CSharedFilesCtrl::OnCRC32UpdateFile	(WPARAM /*wParam*/, LPARAM lParam) {
	uchar* filehash = (uchar*) lParam;
	// We are in the "main" thread, so we can be sure that the filelist is not
	// deleted while we access it - so we try to get a pointer to the desired file
	// directly without worker->ValidateKnownFile !
	// This of course avoids possible deadlocks because we don't lock the list;
	// and we don't need to do an worker->UnlockSharedFilesList...
	CKnownFile* file = theApp.sharedfiles->GetFileByID (filehash);
	if (file != NULL)		// Update the file if it exists
		UpdateFile (file);
	return 0;
}
// [end] Mighty Knife

void CSharedFilesCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column

	bool sortAscending = (GetSortItem() != pNMListView->iSubItem) ? true : !GetSortAscending();

	// Ornis 4-way-sorting
	int adder=0;
	if (pNMListView->iSubItem>=5 && pNMListView->iSubItem<=7)
	{
		ASSERT( pNMListView->iSubItem - 5 < ARRSIZE(sortstat) );
		if (!sortAscending)
			sortstat[pNMListView->iSubItem - 5] = !sortstat[pNMListView->iSubItem - 5];
		adder = sortstat[pNMListView->iSubItem-5] ? 0 : 100;
	}
	else if (pNMListView->iSubItem==11)
	{
		ASSERT( 3 < ARRSIZE(sortstat) );
		if (!sortAscending)
			sortstat[3] = !sortstat[3];
		adder = sortstat[3] ? 0 : 100;
	}

	// Sort table
	if (adder==0)	
		SetSortArrow(pNMListView->iSubItem, sortAscending); 
	else
		SetSortArrow(pNMListView->iSubItem, sortAscending ? arrowDoubleUp : arrowDoubleDown);
	
	UpdateSortHistory(pNMListView->iSubItem + adder + (sortAscending ? 0:20),20);
	// Mighty Knife: CRC32-Tag - Indexes shifted by 10
	/*
	SortItems(SortProc, pNMListView->iSubItem + adder + (sortAscending ? 0:20));
	*/
	SortItems(SortProc, pNMListView->iSubItem + adder + (sortAscending ? 0:30));
	// [end] Mighty Knife
	*pResult = 0;
}

int CSharedFilesCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	// Mighty Knife: CRC32-Tag - Indexes shifted by 10 !
	const CKnownFile* item1 = (CKnownFile*)lParam1;
	const CKnownFile* item2 = (CKnownFile*)lParam2;	
	
	int iResult=0;
	switch(lParamSort){
		case 0: //filename asc
			iResult=CompareLocaleStringNoCase(item1->GetFileName(),item2->GetFileName());
			break;
		case 30: //filename desc
			iResult=CompareLocaleStringNoCase(item2->GetFileName(),item1->GetFileName());
			break;

		case 1: //filesize asc
			iResult=item1->GetFileSize()==item2->GetFileSize()?0:(item1->GetFileSize()>item2->GetFileSize()?1:-1);
			break;

		case 31: //filesize desc
			iResult=item1->GetFileSize()==item2->GetFileSize()?0:(item2->GetFileSize()>item1->GetFileSize()?1:-1);
			break;

		case 2: //filetype asc
			iResult=item1->GetFileTypeDisplayStr().Compare(item2->GetFileTypeDisplayStr());
			break;
		case 32: //filetype desc
			iResult=item2->GetFileTypeDisplayStr().Compare(item1->GetFileTypeDisplayStr());
			break;

		case 3: //prio asc
			//MORPH START - Changed by SiRoB, Powerstate in prio colums
			if (!item1->GetPowerShared() && item2->GetPowerShared())
				iResult=-1;			
			else if (item1->GetPowerShared() && !item2->GetPowerShared())
				iResult=1;
			else			
				if(item1->GetUpPriority() == PR_VERYLOW && item2->GetUpPriority() != PR_VERYLOW)
					iResult=-1;
				else if (item1->GetUpPriority() != PR_VERYLOW && item2->GetUpPriority() == PR_VERYLOW)
					iResult=1;
				else
					iResult=item1->GetUpPriority()-item2->GetUpPriority();
			//MORPH END   - Changed by SiRoB, Powerstate in prio colums
			break;
		case 33: //prio desc
			//MORPH START - Changed by SiRoB, Powerstate in prio colums
			if (!item2->GetPowerShared() && item1->GetPowerShared())
				iResult=-1;			
			else if (item2->GetPowerShared() && !item1->GetPowerShared())
				iResult=1;
			else		
				if(item2->GetUpPriority() == PR_VERYLOW && item1->GetUpPriority() != PR_VERYLOW )
					iResult=-1;
				else if (item2->GetUpPriority() != PR_VERYLOW && item1->GetUpPriority() == PR_VERYLOW)
					iResult=1;
				else
					iResult=item2->GetUpPriority()-item1->GetUpPriority();
			//MORPH END  - Changed by SiRoB, Powerstate in prio colums
			break;
		case 4: //fileID asc
			iResult=memcmp(item1->GetFileHash(), item2->GetFileHash(), 16);
			break;
		case 34: //fileID desc
			iResult=memcmp(item2->GetFileHash(), item1->GetFileHash(), 16);
			break;

		case 5: //requests asc
			iResult=item1->statistic.GetRequests() - item2->statistic.GetRequests();
			break;
		case 35: //requests desc
			iResult=item2->statistic.GetRequests() - item1->statistic.GetRequests();
			break;
		
		case 6: //acc requests asc
			iResult=item1->statistic.GetAccepts() - item2->statistic.GetAccepts();
			break;
		case 36: //acc requests desc
			iResult=item2->statistic.GetAccepts() - item1->statistic.GetAccepts();
			break;
		
		case 7: //all transferred asc
			iResult=item1->statistic.GetTransferred()==item2->statistic.GetTransferred()?0:(item1->statistic.GetTransferred()>item2->statistic.GetTransferred()?1:-1);
			break;
		case 37: //all transferred desc
			iResult=item1->statistic.GetTransferred()==item2->statistic.GetTransferred()?0:(item2->statistic.GetTransferred()>item1->statistic.GetTransferred()?1:-1);
			break;

		case 9: //folder asc
			iResult=CompareLocaleStringNoCase(item1->GetPath(),item2->GetPath());
			break;
		case 39: //folder desc
			iResult=CompareLocaleStringNoCase(item2->GetPath(),item1->GetPath());
			break;

		case 10: //complete sources asc
			iResult=CompareUnsigned(item1->m_nCompleteSourcesCount, item2->m_nCompleteSourcesCount);
			break;
		case 40: //complete sources desc
			iResult=CompareUnsigned(item2->m_nCompleteSourcesCount, item1->m_nCompleteSourcesCount);
			break;

		case 11: //ed2k shared asc
			iResult=item1->GetPublishedED2K() - item2->GetPublishedED2K();
			break;
		case 41: //ed2k shared desc
			iResult=item2->GetPublishedED2K() - item1->GetPublishedED2K();
			break;

		case 12: //permission asc
			iResult=item2->GetPermissions()-item1->GetPermissions();
			break;
		case 42: //permission desc
			iResult=item1->GetPermissions()-item2->GetPermissions();
			break;

		
		//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
		//MORPH START - Added by SiRoB, ZZ Upload System
		case 13:
			if (!item1->GetPowerShared() && item2->GetPowerShared())
				iResult=-1;
			else if (item1->GetPowerShared() && !item2->GetPowerShared())
				iResult=1;
			else
				if (item1->GetPowerSharedMode() != item2->GetPowerSharedMode())
					iResult=item1->GetPowerSharedMode() - item2->GetPowerSharedMode();
				else
					if (!item1->GetPowerShareAuthorized() && item2->GetPowerShareAuthorized())
						iResult=-1;
					else if (item1->GetPowerShareAuthorized() && !item2->GetPowerShareAuthorized())
						iResult=1;
					else
						if (!item1->GetPowerShareAuto() && item2->GetPowerShareAuto())
							iResult=-1;
						else if (item1->GetPowerShareAuto() && !item2->GetPowerShareAuto())
							iResult=1;
						else
							//MORPH START - Added by SiRoB, POWERSHARE Limit
							if (!item1->GetPowerShareLimited() && item2->GetPowerShareLimited())
								iResult=-1;
							else if (item1->GetPowerShareLimited() && !item2->GetPowerShareLimited())
								iResult=1;
							else
							//MORPH END   - Added by SiRoB, POWERSHARE Limit
								iResult=0;
			break;
		case 43:
			if (!item2->GetPowerShared() && item1->GetPowerShared())
				iResult=-1;
			else if (item2->GetPowerShared() && !item1->GetPowerShared())
				iResult=1;
			else
				if (item1->GetPowerSharedMode() != item2->GetPowerSharedMode())
					iResult=item2->GetPowerSharedMode() - item1->GetPowerSharedMode();
				else
					if (!item2->GetPowerShareAuthorized() && item1->GetPowerShareAuthorized())
						iResult=-1;
					else if (item2->GetPowerShareAuthorized() && !item1->GetPowerShareAuthorized())
						iResult=1;
					else
						if (!item2->GetPowerShareAuto() && item1->GetPowerShareAuto())
							iResult=-1;
						else if (item2->GetPowerShareAuto() && !item1->GetPowerShareAuto())
							iResult=1;
						else
							//MORPH START - Added by SiRoB, POWERSHARE Limit
							if (!item2->GetPowerShareLimited() && item1->GetPowerShareLimited())
								iResult=-1;
							else if (item2->GetPowerShareLimited() && !item1->GetPowerShareLimited())
								iResult=1;
							else
							//MORPH END   - Added by SiRoB, POWERSHARE Limit
								iResult=0;
		//MORPH END - Added by SiRoB, ZZ Upload System
		//MORPH END - Changed by SiRoB, Avoid misusing of powersharing
			break;
		//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars
		case 14: //spread asc
		case 15:
			iResult=CompareFloat(((CKnownFile*)lParam1)->statistic.GetSpreadSortValue(),((CKnownFile*)lParam2)->statistic.GetSpreadSortValue());
			break;
		case 44: //spread desc
		case 45:
			iResult=CompareFloat(((CKnownFile*)lParam2)->statistic.GetSpreadSortValue(),((CKnownFile*)lParam1)->statistic.GetSpreadSortValue());
			break;

		case 16: // VQB:  Simple UL asc
		case 46: //VQB:  Simple UL desc
			{
				float x1 = ((float)item1->statistic.GetAllTimeTransferred())/((float)item1->GetFileSize());
				float x2 = ((float)item2->statistic.GetAllTimeTransferred())/((float)item2->GetFileSize());
				if (lParamSort == 16) iResult=CompareFloat(x1,x2); else iResult=CompareFloat(x2,x1);
			break;
			}
		case 17: // SF:  Full Upload Count asc
			iResult=CompareFloat(((CKnownFile*)lParam1)->statistic.GetFullSpreadCount(),((CKnownFile*)lParam2)->statistic.GetFullSpreadCount());
			break;
		case 47: // SF:  Full Upload Count desc
			iResult=CompareFloat(((CKnownFile*)lParam2)->statistic.GetFullSpreadCount(),((CKnownFile*)lParam1)->statistic.GetFullSpreadCount());
			break;
		//MORPH END   - Added by IceCream, SLUGFILLER: Spreadbars
		//MORPH START - Added by SiRoB, HIDEOS
		case 18:
			if (item1->GetHideOS() == item2->GetHideOS())
				iResult=item1->GetSelectiveChunk() - item2->GetSelectiveChunk();
			else
				iResult=item1->GetHideOS() - item2->GetHideOS();
			break;
		case 38:
			if (item2->GetHideOS() == item1->GetHideOS())
				iResult=item2->GetSelectiveChunk() - item1->GetSelectiveChunk();
			else
				iResult=item2->GetHideOS() - item1->GetHideOS();
			break;
		//MORPH END   - Added by SiRoB, HIDEOS
		//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
		case 19:
			iResult=item1->GetShareOnlyTheNeed() - item2->GetShareOnlyTheNeed();
			break;
		case 49:
			iResult=item2->GetShareOnlyTheNeed() - item1->GetShareOnlyTheNeed();
			break;
		//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
		case 105: //all requests asc
			iResult=CompareUnsigned(item1->statistic.GetAllTimeRequests(), item2->statistic.GetAllTimeRequests());
			break;
		case 135: //all requests desc
			iResult=CompareUnsigned(item2->statistic.GetAllTimeRequests(), item1->statistic.GetAllTimeRequests());
			break;

		case 106: //all acc requests asc
			iResult=CompareUnsigned(item1->statistic.GetAllTimeAccepts(), item2->statistic.GetAllTimeAccepts());
			break;

		case 136: //all acc requests desc
			iResult=CompareUnsigned(item2->statistic.GetAllTimeAccepts(), item1->statistic.GetAllTimeAccepts());
			break;

		case 107: //all transferred asc
			iResult=item1->statistic.GetAllTimeTransferred()==item2->statistic.GetAllTimeTransferred()?0:(item1->statistic.GetAllTimeTransferred()>item2->statistic.GetAllTimeTransferred()?1:-1);
			break;

		case 137: //all transferred desc
			iResult=item1->statistic.GetAllTimeTransferred()==item2->statistic.GetAllTimeTransferred()?0:(item2->statistic.GetAllTimeTransferred()>item1->statistic.GetAllTimeTransferred()?1:-1);
			break;

		case 111:{ //kad shared asc
			uint32 tNow = time(NULL);
			int i1 = (tNow < item1->GetLastPublishTimeKadSrc()) ? 1 : 0;
			int i2 = (tNow < item2->GetLastPublishTimeKadSrc()) ? 1 : 0;
			iResult=i1 - i2;
			break;
		}
		case 141:{ //kad shared desc
			uint32 tNow = time(NULL);
			int i1 = (tNow < item1->GetLastPublishTimeKadSrc()) ? 1 : 0;
			int i2 = (tNow < item2->GetLastPublishTimeKadSrc()) ? 1 : 0;
			iResult=i2 - i1;
			break;
		}
		default: 
			iResult=0;
			break;
	}

	// SLUGFILLER: multiSort remove - handled in parent class
	/*
	int dwNextSort;
	//call secondary sortorder, if this one results in equal
	//(Note: yes I know this call is evil OO wise, but better than changing a lot more code, while we have only one instance anyway - might be fixed later)
	if (iResult == 0 && (dwNextSort = theApp.emuledlg->sharedfileswnd->sharedfilesctrl.GetNextSortOrder(lParamSort)) != (-1)){
		iResult= SortProc(lParam1, lParam2, dwNextSort);
	}
	*/
	return iResult;

}

void CSharedFilesCtrl::OpenFile(const CKnownFile* file)
{
	if(file->m_pCollection)
	{
		CCollectionViewDialog dialog;
		dialog.SetCollection(file->m_pCollection);
		dialog.DoModal();
	}
	else
		ShellOpenFile(file->GetFilePath(), NULL);
}

void CSharedFilesCtrl::OnNMDblclk(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1)
	{
		CKnownFile* file = (CKnownFile*)GetItemData(iSel);
		if (file)
		{
			//MORPH Changed by SiRoB - Double click uncomplet files in SharedFile window display FileDetail
			/*
			if (GetKeyState(VK_MENU) & 0x8000){
			*/
			if (GetKeyState(VK_MENU) & 0x8000 || file->IsPartFile())
			{
				CTypedPtrList<CPtrList, CKnownFile*> aFiles;
				aFiles.AddHead(file);
				ShowFileDialog(aFiles);
			}
			else if (!file->IsPartFile())
				OpenFile(file);
		}
	}
	*pResult = 0;
}

void CSharedFilesCtrl::CreateMenues()
{
	if (m_PrioMenu) VERIFY( m_PrioMenu.DestroyMenu() );
	//MORPH START - Added by SiRoB, Keep Prermission Flag
	if (m_PermMenu) VERIFY( m_PermMenu.DestroyMenu() );
	//MORPH END   - Added by SiRoB, Keep Prermission Flag
	//MORPH START - Added by SiRoB, ZZ Upload System
	if (m_PowershareMenu) VERIFY( m_PowershareMenu.DestroyMenu() );
	//MORPH END - Added by SiRoB, ZZ Upload System
	//MORPH START - Added by SiRoB, POWERSHARE LImit
	if (m_PowerShareLimitMenu) VERIFY( m_PowerShareLimitMenu.DestroyMenu() );
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	if(m_SpreadbarMenu)	VERIFY(m_SpreadbarMenu.DestroyMenu());
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	//MORPH START - Added by SiRoB, HIDEOS
	if (m_HideOSMenu) VERIFY( m_HideOSMenu.DestroyMenu() );
	if (m_SelectiveChunkMenu) VERIFY( m_SelectiveChunkMenu.DestroyMenu() );
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	if (m_ShareOnlyTheNeedMenu) VERIFY( m_ShareOnlyTheNeedMenu.DestroyMenu() );
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
	//MORPH START - Added by SiRoB, CRC32-Tag
	if (m_CRC32Menu) VERIFY( m_CRC32Menu.DestroyMenu() );
	//MORPH END   - Added by SiRoB, CRC32-Tag
	if (m_CollectionsMenu) VERIFY( m_CollectionsMenu.DestroyMenu() );
	if (m_SharedFilesMenu) VERIFY( m_SharedFilesMenu.DestroyMenu() );


	m_PrioMenu.CreateMenu();
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOVERYLOW,GetResString(IDS_PRIOVERYLOW));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOLOW,GetResString(IDS_PRIOLOW));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIONORMAL,GetResString(IDS_PRIONORMAL));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOHIGH, GetResString(IDS_PRIOHIGH));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOVERYHIGH, GetResString(IDS_PRIORELEASE));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOAUTO, GetResString(IDS_PRIOAUTO));//UAP
	//MORPH START - Added by SiRoB, WebCache 1.2f
	m_PrioMenu.AppendMenu(MF_STRING|MF_SEPARATOR);//jp webcache release
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOWCRELEASE, _T("WC-Release"));//jp webcache release
	//MORPH END   - Added by SiRoB, WebCache 1.2f

	//MORPH START - Added by SiRoB, Show Permissions
	m_PermMenu.CreateMenu();
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMDEFAULT,	GetResString(IDS_DEFAULT));
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMNONE,	GetResString(IDS_HIDDEN));
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMFRIENDS,	GetResString(IDS_FSTATUS_FRIENDSONLY));
	// Mighty Knife: Community visible filelist
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMCOMMUNITY,GetResString(IDS_COMMUNITY));
	// [end] Mighty Knife
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMALL,		GetResString(IDS_FSTATUS_PUBLIC));
	//MORPH END   - Added by SiRoB, Show Permissions

	//MORPH START - Added by SiRoB, ZZ Upload System
	// add powershare switcher
	m_PowershareMenu.CreateMenu();
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_DEFAULT,GetResString(IDS_DEFAULT));
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_OFF,GetResString(IDS_POWERSHARE_DISABLED));
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_ON,GetResString(IDS_POWERSHARE_ACTIVATED));
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_AUTO,GetResString(IDS_POWERSHARE_AUTO));
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_LIMITED,GetResString(IDS_POWERSHARE_LIMITED)); 
	m_PowerShareLimitMenu.CreateMenu();
	m_PowerShareLimitMenu.AppendMenu(MF_STRING,MP_POWERSHARE_LIMIT,	GetResString(IDS_DEFAULT));
	m_PowerShareLimitMenu.AppendMenu(MF_STRING,MP_POWERSHARE_LIMIT_SET,	GetResString(IDS_DISABLED));
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	//MORPH END   - Added by SiRoB, ZZ Upload System

	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	m_SpreadbarMenu.CreateMenu();
	m_SpreadbarMenu.AppendMenu(MF_STRING,MP_SPREADBAR_DEFAULT, GetResString(IDS_DEFAULT));
	m_SpreadbarMenu.AppendMenu(MF_STRING,MP_SPREADBAR_OFF, GetResString(IDS_DISABLED));
	m_SpreadbarMenu.AppendMenu(MF_STRING,MP_SPREADBAR_ON, GetResString(IDS_ENABLED));
	m_SpreadbarMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
	m_SpreadbarMenu.AppendMenu(MF_STRING,MP_SPREADBAR_RESET, GetResString(IDS_RESET));
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file

	//MORPH START - Added by SiRoB, HIDEOS
	m_HideOSMenu.CreateMenu();
	m_HideOSMenu.AppendMenu(MF_STRING,MP_HIDEOS_DEFAULT, GetResString(IDS_DEFAULT));
	m_HideOSMenu.AppendMenu(MF_STRING,MP_HIDEOS_SET, GetResString(IDS_DISABLED));
	m_SelectiveChunkMenu.CreateMenu();
	m_SelectiveChunkMenu.AppendMenu(MF_STRING,MP_SELECTIVE_CHUNK,	GetResString(IDS_DEFAULT));
	m_SelectiveChunkMenu.AppendMenu(MF_STRING,MP_SELECTIVE_CHUNK_0,	GetResString(IDS_DISABLED));
	m_SelectiveChunkMenu.AppendMenu(MF_STRING,MP_SELECTIVE_CHUNK_1,	GetResString(IDS_ENABLED));
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	m_ShareOnlyTheNeedMenu.CreateMenu();
	m_ShareOnlyTheNeedMenu.AppendMenu(MF_STRING,MP_SHAREONLYTHENEED,	GetResString(IDS_DEFAULT));
	m_ShareOnlyTheNeedMenu.AppendMenu(MF_STRING,MP_SHAREONLYTHENEED_0,	GetResString(IDS_DISABLED));
	m_ShareOnlyTheNeedMenu.AppendMenu(MF_STRING,MP_SHAREONLYTHENEED_1,	GetResString(IDS_ENABLED));
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

	//MORPH START - Changed by SiRoB, Mighty Knife: CRC32-Tag
	m_CRC32Menu.CreateMenu();
	m_CRC32Menu.AppendMenu(MF_STRING,MP_CRC32_CALCULATE,GetResString(IDS_CRC32_CALCULATE));
	m_CRC32Menu.AppendMenu(MF_STRING,MP_CRC32_RECALCULATE,GetResString(IDS_CRC32_RECALCULATE));
	m_CRC32Menu.AppendMenu(MF_STRING,MP_CRC32_TAG,GetResString(IDS_CRC32_TAG));
	m_CRC32Menu.AppendMenu(MF_STRING,MP_CRC32_ABORT,GetResString(IDS_CRC32_ABORT));
	//MORPH END   - Changed by SiRoB, [end] Mighty Knife

	m_CollectionsMenu.CreateMenu();
	m_CollectionsMenu.AddMenuTitle(NULL, true);
	m_CollectionsMenu.AppendMenu(MF_STRING,MP_CREATECOLLECTION, GetResString(IDS_CREATECOLLECTION), _T("COLLECTION_ADD"));
	m_CollectionsMenu.AppendMenu(MF_STRING,MP_MODIFYCOLLECTION, GetResString(IDS_MODIFYCOLLECTION), _T("COLLECTION_EDIT"));
	m_CollectionsMenu.AppendMenu(MF_STRING,MP_VIEWCOLLECTION, GetResString(IDS_VIEWCOLLECTION), _T("COLLECTION_VIEW"));
	m_CollectionsMenu.AppendMenu(MF_STRING,MP_SEARCHAUTHOR, GetResString(IDS_SEARCHAUTHORCOLLECTION), _T("COLLECTION_SEARCH"));

	m_SharedFilesMenu.CreatePopupMenu();
	m_SharedFilesMenu.AddMenuTitle(GetResString(IDS_SHAREDFILES), true);

	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_OPEN, GetResString(IDS_OPENFILE), _T("OPENFILE"));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_OPENFOLDER, GetResString(IDS_OPENFOLDER), _T("OPENFOLDER"));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_RENAME, GetResString(IDS_RENAME) + _T("..."), _T("FILERENAME"));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_REMOVE, GetResString(IDS_DELETE), _T("DELETE"));
	if (thePrefs.IsExtControlsEnabled())
		m_SharedFilesMenu.AppendMenu(MF_STRING,Irc_SetSendLink,GetResString(IDS_IRC_ADDLINKTOIRC), _T("IRCCLIPBOARD"));

	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR);
	//MOPRH START - Added by SiRoB, Keep permission flag	
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PermMenu.m_hMenu, GetResString(IDS_PERMISSION), _T("FILEPERMISSION"));	// xMule_MOD: showSharePermissions - done
	//MOPRH END   - Added by SiRoB, Keep permission flag
	//MORPH START - Added by SiRoB, ZZ Upload System
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PowershareMenu.m_hMenu, GetResString(IDS_POWERSHARE), _T("FILEPOWERSHARE"));
	//MORPH END - Added by SiRoB, ZZ Upload System
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	m_PowershareMenu.AppendMenu(MF_STRING|MF_SEPARATOR);
	m_PowershareMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PowerShareLimitMenu.m_hMenu, GetResString(IDS_POWERSHARE_LIMIT));
	//MORPH END   - Added by SiRoB, POWERSHARE Limit

	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_SpreadbarMenu.m_hMenu, GetResString(IDS_SPREADBAR), _T("FILESPREADBAR"));
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file

	//MORPH START - Added by SiRoB, HIDEOS
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_HideOSMenu.m_hMenu, GetResString(IDS_HIDEOS), _T("FILEHIDEOS"));
	m_HideOSMenu.AppendMenu(MF_STRING|MF_SEPARATOR);
	m_HideOSMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_SelectiveChunkMenu.m_hMenu, GetResString(IDS_SELECTIVESHARE));
	//MORPH END   - Added by SiRoB, HIDEOS

	//MORPH START - Added by SiRoB,	SHARE_ONLY_THE_NEED
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_ShareOnlyTheNeedMenu.m_hMenu, GetResString(IDS_SHAREONLYTHENEED), _T("FILESHAREONLYTHENEED"));
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PrioMenu.m_hMenu, GetResString(IDS_PRIORITY) + _T(" (") + GetResString(IDS_PW_CON_UPLBL) + _T(")"), _T("FILEPRIORITY"));
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR);

	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_CollectionsMenu.m_hMenu, GetResString(IDS_META_COLLECTION), _T("COLLECTION"));
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR);
	
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_DETAIL, GetResString(IDS_SHOWDETAILS), _T("FILEINFO"));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_CMT, GetResString(IDS_CMT_ADD), _T("FILECOMMENTS")); 
	if (thePrefs.GetShowCopyEd2kLinkCmd())
		m_SharedFilesMenu.AppendMenu(MF_STRING,MP_GETED2KLINK, GetResString(IDS_DL_LINK1), _T("ED2KLINK") );
	else
		m_SharedFilesMenu.AppendMenu(MF_STRING,MP_SHOWED2KLINK, GetResString(IDS_DL_SHOWED2KLINK), _T("ED2KLINK") );
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_FIND, GetResString(IDS_FIND), _T("Search"));
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
	
	//MORPH START - Changed by SiRoB, Mighty Knife: CRC32-Tag
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_CRC32Menu.m_hMenu, GetResString(IDS_CRC32), _T("FILECRC32"));
	//MORPH START - Changed by SiRoB, [end] Mighty Knife

	// Mighty Knife: Mass Rename
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_MASSRENAME,GetResString(IDS_MR), _T("FILEMASSRENAME"));
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
	// [end] Mighty Knife

#if defined(_DEBUG)
	if (thePrefs.IsExtControlsEnabled()){
		//JOHNTODO: Not for release as we need kad lowID users in the network to see how well this work work. Also, we do not support these links yet.
		m_SharedFilesMenu.AppendMenu(MF_STRING,MP_GETKADSOURCELINK, _T("Copy eD2K Links To Clipboard (Kad)"));
		m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 	
	}
#endif

	//MORPH START - Added by SiRoB, copy feedback feature
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_COPYFEEDBACK, GetResString(IDS_COPYFEEDBACK), _T("COPY"));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_COPYFEEDBACK_US, GetResString(IDS_COPYFEEDBACK_US), _T("COPY"));
	m_SharedFilesMenu.AppendMenu(MF_SEPARATOR);
	//MORPH END   - Added by SiRoB, copy feedback feature
}

void CSharedFilesCtrl::ShowComments(CKnownFile* file)
{
	if (file)
	{
		CTypedPtrList<CPtrList, CKnownFile*> aFiles;
		aFiles.AddHead(file);
		ShowFileDialog(aFiles, IDD_COMMENT);
	}
}

void CSharedFilesCtrl::OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
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
			const CKnownFile* pFile = reinterpret_cast<CKnownFile*>(pDispInfo->item.lParam);
			if (pFile != NULL){
				switch (pDispInfo->item.iSubItem){
					case 0:
						if (pDispInfo->item.cchTextMax > 0){
							_tcsncpy(pDispInfo->item.pszText, pFile->GetFileName(), pDispInfo->item.cchTextMax);
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

void CSharedFilesCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		// Ctrl+C: Copy listview items to clipboard
		SendMessage(WM_COMMAND, MP_GETED2KLINK);
		return;
	}
	else if (nChar == VK_F5)
		ReloadFileList();

	CMuleListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CSharedFilesCtrl::ShowFileDialog(CTypedPtrList<CPtrList, CKnownFile*>& aFiles, UINT uPshInvokePage)
{
	if (aFiles.GetSize() > 0)
	{
		CSharedFileDetailsSheet dialog(aFiles, uPshInvokePage, this);
		dialog.DoModal();
	}
}

void CSharedFilesCtrl::SetDirectoryFilter(CDirectoryItem* pNewFilter, bool bRefresh){
	if (m_pDirectoryFilter == pNewFilter)
		return;
	m_pDirectoryFilter = pNewFilter;
	if (bRefresh)
		ReloadFileList();
}
