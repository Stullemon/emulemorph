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
#include "MemDC.h"
#include "MuleListCtrl.h"
#include "Ini2.h"
#include "SharedFilesCtrl.h"
#include "SearchListCtrl.h"
#include "KadContactListCtrl.h"
#include "KadSearchListCtrl.h"
#include "DownloadListCtrl.h"
#include "DownloadClientsCtrl.h" //SLAHAM: ADDED DownloadClientsCtrl
#include "UploadListCtrl.h"
#include "QueueListCtrl.h"
#include "ClientListCtrl.h"
#include "FriendListCtrl.h"
#include "ServerListCtrl.h"
#include "MenuCmds.h"
#include "OtherFunctions.h"
#include "ListViewSearchDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


#define MLC_BLEND(A, B, X) ((A + B * (X-1) + ((X+1)/2)) / X)

#define MLC_RGBBLEND(A, B, X) (                   \
	RGB(MLC_BLEND(GetRValue(A), GetRValue(B), X), \
	MLC_BLEND(GetGValue(A), GetGValue(B), X),     \
	MLC_BLEND(GetBValue(A), GetBValue(B), X))     \
)

#define MLC_DT_TEXT (DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS)

#define MLC_IDC_MENU	4875
#define MLC_IDC_UPDATE	(MLC_IDC_MENU - 1)

//a value that's not a multiple of 4 and uncommon
#define MLC_MAGIC 0xFEEBDEEF

//used for very slow assertions
//#define MLC_ASSERT(f)	ASSERT(f)
#define MLC_ASSERT(f)	((void)0)

//////////////////////////////////
// CMuleListCtrl

IMPLEMENT_DYNAMIC(CMuleListCtrl, CListCtrl)
CMuleListCtrl::CMuleListCtrl(PFNLVCOMPARE pfnCompare, DWORD dwParamSort) {
	m_SortProc = pfnCompare;
	m_dwParamSort.AddHead(dwParamSort);	// SLUGFILLER: multiSort

	m_bCustomDraw = false;
	m_iCurrentSortItem = -1;
	m_iColumnsTracked = 0;
	m_aColumns = NULL;
	m_iRedrawCount = 0;

	//just in case
    m_crWindow = 0;
    m_crWindowText = 0;
	m_crWindowTextBk = m_crWindow;
    m_crHighlight = 0;
	m_crGlow=0;
    m_crFocusLine = 0;
    m_crNoHighlight = 0;
    m_crNoFocusLine = 0;
	m_bGeneralPurposeFind = false;
    m_bFindMatchCase = false;
    m_iFindDirection = 1;
    m_iFindColumn = 0;
}

CMuleListCtrl::~CMuleListCtrl() {
	if(m_aColumns != NULL)
		delete[] m_aColumns;
}

int CMuleListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
	return 0;
}

void CMuleListCtrl::SetName(LPCTSTR lpszName) {
	m_Name = lpszName;
}

void CMuleListCtrl::PreSubclassWindow()
{
	SetColors();
	CListCtrl::PreSubclassWindow();
	SendMessage(CCM_SETUNICODEFORMAT, TRUE);
	ModifyStyle(LVS_SINGLESEL|LVS_LIST|LVS_ICON|LVS_SMALLICON,LVS_REPORT|LVS_SINGLESEL|LVS_REPORT);
	SetExtendedStyle(LVS_EX_HEADERDRAGDROP);
}

int CMuleListCtrl::IndexToOrder(CHeaderCtrl* pHeader, int iIndex) {
	int iCount = pHeader->GetItemCount();
	int *piArray = new int[iCount];
	Header_GetOrderArray( pHeader->m_hWnd, iCount, piArray);
	for(int i=0; i < iCount; i++ ) {
		if(piArray[i] == iIndex) {
			delete[] piArray;
			return i;
		}
	}
	delete[] piArray;
	return -1;
}

void CMuleListCtrl::HideColumn(int iColumn) {
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	if(iColumn < 1 || iColumn >= iCount || m_aColumns[iColumn].bHidden)
		return;

	//stop it from redrawing
	SetRedraw(FALSE);

	//shrink width to 0
	HDITEM item;
	item.mask = HDI_WIDTH;
	pHeaderCtrl->GetItem(iColumn, &item);
	m_aColumns[iColumn].iWidth = item.cxy;
	item.cxy = 0;
	pHeaderCtrl->SetItem(iColumn, &item);

	//move to front of list
	INT *piArray = new INT[m_iColumnsTracked];
	pHeaderCtrl->GetOrderArray(piArray, m_iColumnsTracked);

	int iFrom = m_aColumns[iColumn].iLocation;
	for(int i = 0; i < m_iColumnsTracked; i++)
		if(m_aColumns[i].iLocation > m_aColumns[iColumn].iLocation && m_aColumns[i].bHidden)
			iFrom++;

	for(int i = iFrom; i > 0; i--)
		piArray[i] = piArray[i - 1];
	piArray[0] = iColumn;
	pHeaderCtrl->SetOrderArray(m_iColumnsTracked, piArray);
	delete[] piArray;

	//update entry
	m_aColumns[iColumn].bHidden = true;

	//redraw
	SetRedraw(TRUE);
	Invalidate(FALSE);
}

void CMuleListCtrl::ShowColumn(int iColumn) {
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	if(iColumn < 1 || iColumn >= iCount || !m_aColumns[iColumn].bHidden)
		return;

	//stop it from redrawing
	SetRedraw(FALSE);

	//restore position in list
	INT *piArray = new INT[m_iColumnsTracked];
	pHeaderCtrl->GetOrderArray(piArray, m_iColumnsTracked);
	int iCurrent = IndexToOrder(pHeaderCtrl, iColumn);

	for(; iCurrent < IndexToOrder(pHeaderCtrl, 0) && iCurrent < m_iColumnsTracked - 1; iCurrent++ )
		piArray[iCurrent] = piArray[iCurrent + 1];
	for(; m_aColumns[iColumn].iLocation > m_aColumns[pHeaderCtrl->OrderToIndex(iCurrent + 1)].iLocation &&
	      iCurrent < m_iColumnsTracked - 1; iCurrent++)
		piArray[iCurrent] = piArray[iCurrent + 1];
	piArray[iCurrent] = iColumn;
	pHeaderCtrl->SetOrderArray(m_iColumnsTracked, piArray);
	delete[] piArray;

	//and THEN restore original width
	HDITEM item;
	item.mask = HDI_WIDTH;
	item.cxy = m_aColumns[iColumn].iWidth;
	pHeaderCtrl->SetItem(iColumn, &item);

	//update entry
	m_aColumns[iColumn].bHidden = false;

	//redraw
	SetRedraw(TRUE);
	Invalidate(FALSE);
}

void CMuleListCtrl::SaveSettings(CPreferences::Table tID) {
	INT *piArray = new INT[m_iColumnsTracked];

	for(int i = 0; i < m_iColumnsTracked; i++) {
		thePrefs.SetColumnWidth(tID, i, GetColumnWidth(i));
		thePrefs.SetColumnHidden(tID, i, IsColumnHidden(i));
		piArray[i] = m_aColumns[i].iLocation;
	}

	thePrefs.SetColumnOrder(tID, piArray);
	delete[] piArray;
}

void CMuleListCtrl::SaveSettings(CIni* ini, LPCTSTR pszLVName)
{
	int* piColWidths = new int[m_iColumnsTracked];
	int* piColHidden = new int[m_iColumnsTracked];
	INT *piColOrders = new INT[m_iColumnsTracked];

	for(int i = 0; i < m_iColumnsTracked; i++)
	{
		piColWidths[i] = GetColumnWidth(i);
		piColHidden[i] = IsColumnHidden(i);
		piColOrders[i] = m_aColumns[i].iLocation;
	}

	ini->SerGet(false, piColWidths, m_iColumnsTracked, CString(pszLVName) + _T("ColumnWidths"));
	ini->SerGet(false, piColHidden, m_iColumnsTracked, CString(pszLVName) + _T("ColumnHidden"));
	ini->SerGet(false, piColOrders, m_iColumnsTracked, CString(pszLVName) + _T("ColumnOrders"));

	delete[] piColOrders;
	delete[] piColWidths;
	delete[] piColHidden;
}

void CMuleListCtrl::LoadSettings(CPreferences::Table tID) {
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();

	INT *piArray = new INT[m_iColumnsTracked];
	for(int i = 0; i < m_iColumnsTracked; i++)
		piArray[i] = i;

	for(int i = 0; i < m_iColumnsTracked; i++) {
		int iWidth = thePrefs.GetColumnWidth(tID, i);
		if(iWidth >= 2) // don't allow column widths of 0 and 1 -- just because it looks very confusing in GUI
			SetColumnWidth(i, iWidth);
		if(i == 0) {
			piArray[0] = 0;
		} else {
			int iOrder = thePrefs.GetColumnOrder(tID, i);
			if(iOrder > 0 && iOrder < m_iColumnsTracked && iOrder != i)
				piArray[iOrder] = i;
		}
	}

	pHeaderCtrl->SetOrderArray(m_iColumnsTracked, piArray);
	pHeaderCtrl->GetOrderArray(piArray, m_iColumnsTracked);
	for(int i = 0; i < m_iColumnsTracked; i++)
		m_aColumns[piArray[i]].iLocation = i;

	delete[] piArray;

	for(int i = 1; i < m_iColumnsTracked; i++) {
		if(thePrefs.GetColumnHidden(tID, i))
			HideColumn(i);
	}
}

void CMuleListCtrl::LoadSettings(CIni* ini, LPCTSTR pszLVName)
{
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();

	int* piColWidths = new int[m_iColumnsTracked];
	int* piColHidden = new int[m_iColumnsTracked];
	int* piColOrders = new int[m_iColumnsTracked];
	ini->SerGet(true, piColWidths, m_iColumnsTracked, CString(pszLVName) + _T("ColumnWidths"));
	ini->SerGet(true, piColHidden, m_iColumnsTracked, CString(pszLVName) + _T("ColumnHidden"));
	ini->SerGet(true, piColOrders, m_iColumnsTracked, CString(pszLVName) + _T("ColumnOrders"));

	INT *piArray = new INT[m_iColumnsTracked];
	for (int i = 0; i < m_iColumnsTracked; i++)
		piArray[i] = i;

	for (int i = 0; i < m_iColumnsTracked; i++)
	{
		int iWidth = piColWidths[i];
		if (iWidth >= 2) // don't allow column widths of 0 and 1 -- just because it looks very confusing in GUI
			SetColumnWidth(i, iWidth);
		if (i == 0) {
			piArray[0] = 0;
		}
		else {
			int iOrder = piColOrders[i];
			if (iOrder > 0 && iOrder < m_iColumnsTracked && iOrder != i)
				piArray[iOrder] = i;
		}
	}

	pHeaderCtrl->SetOrderArray(m_iColumnsTracked, piArray);
	pHeaderCtrl->GetOrderArray(piArray, m_iColumnsTracked);
	for(int i = 0; i < m_iColumnsTracked; i++)
		m_aColumns[piArray[i]].iLocation = i;

	delete[] piArray;

	for(int i = 1; i < m_iColumnsTracked; i++) {
		if (piColHidden[i])
			HideColumn(i);
	}

	delete[] piColOrders;
	delete[] piColWidths;
	delete[] piColHidden;
}

void CMuleListCtrl::SetColors(LPCTSTR pszLvKey) {
	m_crWindow      = ::GetSysColor(COLOR_WINDOW);
	m_crWindowText  = ::GetSysColor(COLOR_WINDOWTEXT);
	m_crWindowTextBk = m_crWindow;

	COLORREF crHighlight = ::GetSysColor(COLOR_HIGHLIGHT);

	CString strBkImage;
	LPCTSTR pszSkinProfile = thePrefs.GetSkinProfile();
	if (pszSkinProfile != NULL && pszSkinProfile[0] != _T('\0'))
	{
		CString strKey;
		if (pszLvKey != NULL && pszLvKey[0] != _T('\0'))
			strKey = pszLvKey;
		else if (IsKindOf(RUNTIME_CLASS(CServerListCtrl)))
			strKey = _T("ServersLv");
		else if (IsKindOf(RUNTIME_CLASS(CSearchListCtrl)))
			strKey = _T("SearchResultsLv");
		else if (IsKindOf(RUNTIME_CLASS(CDownloadListCtrl)))
			strKey = _T("DownloadsLv");
		// MORPH START - Added by Commander, Crash fix
		else if (IsKindOf(RUNTIME_CLASS(CDownloadClientsCtrl)))	
			strKey = _T("DownloadsClientsLv");
		// MORPH END - Added by Commander, Crash fix
		else if (IsKindOf(RUNTIME_CLASS(CUploadListCtrl)))
			strKey = _T("UploadsLv");
		else if (IsKindOf(RUNTIME_CLASS(CQueueListCtrl)))
			strKey = _T("QueuedLv");
		else if (IsKindOf(RUNTIME_CLASS(CClientListCtrl)))
			strKey = _T("ClientsLv");
		else if (IsKindOf(RUNTIME_CLASS(CFriendListCtrl)))
			strKey = _T("FriendsLv");
		else if (IsKindOf(RUNTIME_CLASS(CSharedFilesCtrl)))
			strKey = _T("SharedFilesLv");
		else if (IsKindOf(RUNTIME_CLASS(CKadContactListCtrl)))
			strKey = _T("KadContactsLv");
		else if (IsKindOf(RUNTIME_CLASS(CKadSearchListCtrl)))
			strKey = _T("KadActionsLv");
		else
			GetWindowText(strKey);

		if (strKey.IsEmpty())
			strKey = _T("DefLv");

		if (theApp.LoadSkinColorAlt(strKey + _T("Bk" ), _T("DefLvBk"), m_crWindow))
				m_crWindowTextBk = m_crWindow;
		theApp.LoadSkinColorAlt(strKey + _T("Fg"), _T("DefLvFg"), m_crWindowText);
		theApp.LoadSkinColorAlt(strKey + _T("Hl"), _T("DefLvHl"), crHighlight);

		TCHAR szColor[MAX_PATH];
		GetPrivateProfileString(_T("Colors"), strKey + _T("BkImg"), _T(""), szColor, ARRSIZE(szColor), pszSkinProfile);
		if (szColor[0] == _T('\0'))
			GetPrivateProfileString(_T("Colors"), _T("DefLvBkImg"), _T(""), szColor, ARRSIZE(szColor), pszSkinProfile);
		if (szColor[0] != _T('\0'))
			strBkImage = szColor;
	}

	SetBkColor(m_crWindow);
	SetTextBkColor(m_crWindowTextBk);
	SetTextColor(m_crWindowText);
	LVBKIMAGE lvimg = {0};
	lvimg.ulFlags = LVBKIF_SOURCE_NONE;
	SetBkImage(&lvimg);
	if (!strBkImage.IsEmpty())
	{
		// expand any optional available environment strings
		TCHAR szExpSkinRes[MAX_PATH];
		if (ExpandEnvironmentStrings(strBkImage, szExpSkinRes, ARRSIZE(szExpSkinRes)) != 0)
			strBkImage = szExpSkinRes;

		// create absolute path to icon resource file
		TCHAR szFullResPath[MAX_PATH];
		if (PathIsRelative(strBkImage))
		{
			TCHAR szSkinResFolder[MAX_PATH];
			_tcsncpy(szSkinResFolder, pszSkinProfile, ARRSIZE(szSkinResFolder));
			szSkinResFolder[ARRSIZE(szSkinResFolder)-1] = _T('\0');
			PathRemoveFileSpec(szSkinResFolder);
			_tmakepath(szFullResPath, NULL, szSkinResFolder, strBkImage, NULL);
		}
		else
		{
			_tcsncpy(szFullResPath, strBkImage, ARRSIZE(szFullResPath));
			szFullResPath[ARRSIZE(szFullResPath)-1] = _T('\0');
		}

		CString strUrl(_T("file://"));
		strUrl += szFullResPath;
		//if (SetBkImage(const_cast<LPTSTR>((LPCTSTR)strUrl), FALSE, 0, 0))
		if (SetBkImage(const_cast<LPTSTR>((LPCTSTR)strUrl), FALSE, 100, 0))
		{
			m_crWindowTextBk = CLR_NONE;
			SetTextBkColor(m_crWindowTextBk);
		}
	}

	m_crFocusLine   = crHighlight;
	m_crNoHighlight = MLC_RGBBLEND(crHighlight, m_crWindow, 8);
	m_crNoFocusLine = MLC_RGBBLEND(crHighlight, m_crWindow, 2);
	m_crHighlight   = MLC_RGBBLEND(crHighlight, m_crWindow, 4);
	m_crGlow		= MLC_RGBBLEND(crHighlight, m_crWindow, 3);
}

void CMuleListCtrl::SetSortArrow(int iColumn, ArrowType atType) {
	HDITEM headerItem;
	headerItem.mask = HDI_FORMAT;
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();

	if(iColumn != m_iCurrentSortItem) {
		pHeaderCtrl->GetItem(m_iCurrentSortItem, &headerItem);
		headerItem.fmt &= ~(HDF_IMAGE | HDF_BITMAP_ON_RIGHT);
		pHeaderCtrl->SetItem(m_iCurrentSortItem, &headerItem);
		m_iCurrentSortItem = iColumn;
		m_imlHeaderCtrl.DeleteImageList();
	}

	//place new arrow unless we were given an invalid column
	if(iColumn >= 0 && pHeaderCtrl->GetItem(iColumn, &headerItem)) {
		m_atSortArrow = atType;

		HINSTANCE hInstRes = AfxFindResourceHandle(MAKEINTRESOURCE(m_atSortArrow), RT_BITMAP);
		if (hInstRes != NULL){
			HBITMAP hbmSortStates = (HBITMAP)::LoadImage(hInstRes, MAKEINTRESOURCE(m_atSortArrow), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS);
			if (hbmSortStates != NULL){
				CBitmap bmSortStates;
				bmSortStates.Attach(hbmSortStates);

				CImageList imlSortStates;
				if (imlSortStates.Create(14, 14, theApp.m_iDfltImageListColorFlags | ILC_MASK, 1, 0)){
					VERIFY( imlSortStates.Add(&bmSortStates, RGB(255, 0, 255)) != -1 );

					// To avoid drawing problems (which occure only with an image list *with* a mask) while
					// resizing list view columns which have the header control bitmap right aligned, set
					// the background color of the image list.
					if (theApp.m_ullComCtrlVer < MAKEDLLVERULL(6,0,0,0))
						imlSortStates.SetBkColor(GetSysColor(COLOR_BTNFACE));

					// When setting the image list for the header control for the first time we'll get
					// the image list of the listview control!! So, better store the header control imagelist separate.
					(void)pHeaderCtrl->SetImageList(&imlSortStates);
					m_imlHeaderCtrl.DeleteImageList();
					m_imlHeaderCtrl.Attach(imlSortStates.Detach());

					// Use smaller bitmap margins -- this saves some pixels which may be required for 
					// rather small column titles.
					if (theApp.m_ullComCtrlVer >= MAKEDLLVERULL(5,8,0,0)){
						int iBmpMargin = pHeaderCtrl->GetBitmapMargin();
					    int iNewBmpMargin = GetSystemMetrics(SM_CXEDGE) + GetSystemMetrics(SM_CXEDGE)/2;
					    if (iNewBmpMargin < iBmpMargin)
							pHeaderCtrl->SetBitmapMargin(iNewBmpMargin);
					}
				}
			}
		}
		headerItem.mask |= HDI_IMAGE;
		headerItem.fmt |= HDF_IMAGE | HDF_BITMAP_ON_RIGHT;
		headerItem.iImage = 0;
		pHeaderCtrl->SetItem(iColumn, &headerItem);
	}
}

int CMuleListCtrl::MoveItem(int iOldIndex, int iNewIndex) { //move item in list, returns index of new item
	if(iNewIndex > iOldIndex)
		iNewIndex--;

	//save substrings
	CSimpleArray<void*> aSubItems;
	DWORD Style = GetStyle();
	if((Style & LVS_OWNERDATA) == 0) {
		TCHAR szText[256];
		LVITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_NORECOMPUTE;
		lvi.iItem = iOldIndex;
		for(int i = 1; i < m_iColumnsTracked; i++){
			lvi.iSubItem = i;
			lvi.cchTextMax = ARRSIZE(szText);
			lvi.pszText = szText;
			void* pstrSubItem = NULL;
			if (GetItem(&lvi)){
				if (lvi.pszText == LPSTR_TEXTCALLBACK)
					pstrSubItem = LPSTR_TEXTCALLBACK;
				else
					pstrSubItem = new CString(lvi.pszText);
			}
			aSubItems.Add(pstrSubItem);
		}
	}

	//copy item
	LVITEM lvi;
	TCHAR szText[256];
	lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM | LVIF_INDENT | LVIF_IMAGE | LVIF_NORECOMPUTE;
	lvi.stateMask = (UINT)-1;
	lvi.iItem = iOldIndex;
	lvi.iSubItem = 0;
	lvi.pszText = szText;
	lvi.cchTextMax = sizeof(szText) / sizeof(szText[0]);
	lvi.iIndent = 0;
	if(GetItem(&lvi) == 0)
		return -1;
	lvi.iItem = iNewIndex;

	//do the move
	SetRedraw(FALSE);
	//SetItemData(iOldIndex, 0);  //should do this to be safe?
	DeleteItem(iOldIndex);
	iNewIndex = InsertItem(&lvi);
	SetRedraw(TRUE);

	//restore substrings
	if((Style & LVS_OWNERDATA) == 0) {
		for(int i = 1; i < m_iColumnsTracked; i++) {
			LVITEM lvi;
			lvi.iSubItem = i;
			void* pstrSubItem = aSubItems[i-1];
			if (pstrSubItem != NULL){
				if (pstrSubItem == LPSTR_TEXTCALLBACK)
					lvi.pszText = LPSTR_TEXTCALLBACK;
				else
					lvi.pszText = (LPTSTR)((LPCTSTR)*((CString*)pstrSubItem));
				DefWindowProc(LVM_SETITEMTEXT, iNewIndex, (LPARAM)&lvi);
				if (pstrSubItem != LPSTR_TEXTCALLBACK)
					delete (CString*)pstrSubItem;
			}
		}
	}

	return iNewIndex;
}

int CMuleListCtrl::UpdateLocation(int iItem) {
	int iItemCount = GetItemCount();
	if(iItem >= iItemCount || iItem < 0)
		return iItem;

	BOOL notLast = iItem + 1 < iItemCount;
	BOOL notFirst = iItem > 0;

	DWORD_PTR dwpItemData = GetItemData(iItem);
	if(dwpItemData == NULL)
		return iItem;

	if(notFirst) {
		int iNewIndex = iItem - 1;
		POSITION pos = m_Params.FindIndex(iNewIndex);
		int iResult = MultiSortProc(dwpItemData, GetParamAt(pos, iNewIndex));	// SLUGFILLER: multiSort
		if(iResult < 0) {
			POSITION posPrev = pos;
			int iDist = iNewIndex / 2;
			while(iDist > 1) {
				for(int i = 0; i < iDist; i++)
					m_Params.GetPrev(posPrev);

				if(MultiSortProc(dwpItemData, GetParamAt(posPrev, iNewIndex - iDist)) < 0) {	// SLUGFILLER: multiSort
					iNewIndex = iNewIndex - iDist;
					pos = posPrev;
				} else {
					posPrev = pos;
				}
				iDist /= 2;
			}
			while(--iNewIndex >= 0) {
				m_Params.GetPrev(pos);
				if(MultiSortProc(dwpItemData, GetParamAt(pos, iNewIndex)) >= 0)	// SLUGFILLER: multiSort
					break;
			}
			MoveItem(iItem, iNewIndex + 1);
			return iNewIndex + 1;
		}
	}

	if(notLast) {
		int iNewIndex = iItem + 1;
		POSITION pos = m_Params.FindIndex(iNewIndex);
		int iResult = MultiSortProc(dwpItemData, GetParamAt(pos, iNewIndex));	// SLUGFILLER: multiSort
		if(iResult > 0) {
			POSITION posNext = pos;
			int iDist = (GetItemCount() - iNewIndex) / 2;
			while(iDist > 1) {
				for(int i = 0; i < iDist; i++)
					m_Params.GetNext(posNext);

				if(MultiSortProc(dwpItemData, GetParamAt(posNext, iNewIndex + iDist)) > 0) {	// SLUGFILLER: multiSort
					iNewIndex = iNewIndex + iDist;
					pos = posNext;
				} else {
					posNext = pos;
				}
				iDist /= 2;
			}
			while(++iNewIndex < iItemCount) {
				m_Params.GetNext(pos);
				if(MultiSortProc(dwpItemData, GetParamAt(pos, iNewIndex)) <= 0)	// SLUGFILLER: multiSort
					break;
			}
			MoveItem(iItem, iNewIndex);
			return iNewIndex;
		}
	}

	return iItem;
}

DWORD_PTR CMuleListCtrl::GetItemData(int iItem) {
	POSITION pos = m_Params.FindIndex(iItem);
	LPARAM lParam = GetParamAt(pos, iItem);

	MLC_ASSERT(lParam == CListCtrl::GetItemData(iItem));
	return lParam;
}

//lower level than everything else so poorly overriden functions don't break us
BOOL CMuleListCtrl::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) {
	//lets look for the important messages that are essential to handle
	switch(message) {
	case WM_NOTIFY:
		if(wParam == 0) {
			if(((NMHDR*)lParam)->code == NM_RCLICK) {
				//handle right click on headers and show column menu

				POINT point;
				GetCursorPos (&point);

				CTitleMenu tmColumnMenu;
				tmColumnMenu.CreatePopupMenu();
				if(m_Name.GetLength() != 0)
					tmColumnMenu.AddMenuTitle(m_Name);

				CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
				int iCount = pHeaderCtrl->GetItemCount();
				for(int iCurrent = 1; iCurrent < iCount; iCurrent++) {
					HDITEM item;
					TCHAR text[255];
					item.pszText = text;
					item.mask = HDI_TEXT;
					item.cchTextMax = ARRSIZE(text);
					pHeaderCtrl->GetItem(iCurrent, &item);

					tmColumnMenu.AppendMenu(MF_STRING | m_aColumns[iCurrent].bHidden ? 0 : MF_CHECKED,
						MLC_IDC_MENU + iCurrent, item.pszText);
				}
				tmColumnMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
				VERIFY( tmColumnMenu.DestroyMenu() );

				return *pResult = TRUE;

			} else if(((NMHDR*)lParam)->code == HDN_BEGINTRACKA || ((NMHDR*)lParam)->code == HDN_BEGINTRACKW) {
				//stop them from changeing the size of anything "before" first column

				HD_NOTIFY *pHDN = (HD_NOTIFY*)lParam;
				if(m_aColumns[pHDN->iItem].bHidden)
					return *pResult = TRUE;

			} else if(((NMHDR*)lParam)->code == HDN_ENDDRAG) {
				//stop them from moving first column

				NMHEADER *pHeader = (NMHEADER*)lParam;
				if(pHeader->iItem != 0 && pHeader->pitem->iOrder != 0) {

					int iNewLoc = pHeader->pitem->iOrder - GetHiddenColumnCount();
					if(iNewLoc > 0) {

						if(m_aColumns[pHeader->iItem].iLocation != iNewLoc) {

							if(m_aColumns[pHeader->iItem].iLocation > iNewLoc) {
								int iMax = m_aColumns[pHeader->iItem].iLocation;
								int iMin = iNewLoc;
								for(int i = 0; i < m_iColumnsTracked; i++) {
									if(m_aColumns[i].iLocation >= iMin && m_aColumns[i].iLocation < iMax)
										m_aColumns[i].iLocation++;
								}
							}

							else if(m_aColumns[pHeader->iItem].iLocation < iNewLoc) {
								int iMin = m_aColumns[pHeader->iItem].iLocation;
								int iMax = iNewLoc;
								for(int i = 0; i < m_iColumnsTracked; i++) {
									if(m_aColumns[i].iLocation > iMin && m_aColumns[i].iLocation <= iMax)
										m_aColumns[i].iLocation--;
								}
							}

							m_aColumns[pHeader->iItem].iLocation = iNewLoc;

							Invalidate(FALSE);
							break;
						}
					}
				}

				return *pResult = 1;
			} else if(((NMHDR*)lParam)->code == HDN_DIVIDERDBLCLICKA || ((NMHDR*)lParam)->code == HDN_DIVIDERDBLCLICKW) {
				if (GetStyle() & LVS_OWNERDRAWFIXED) {
					NMHEADER *pHeader = (NMHEADER*)lParam;
					// As long as we do not handle the HDN_DIVIDERDBLCLICK according the actual
					// listview item contents it's better to resize to the header width instead of
					// resizing to zero width. The complete solution for this would require a lot
					// of rewriting in the owner drawn listview controls...
					SetColumnWidth(pHeader->iItem, LVSCW_AUTOSIZE_USEHEADER);
					return *pResult = 1;
				}
			}
		}
		break;

	case WM_COMMAND:
		//deal with menu clicks

		if(wParam == MLC_IDC_UPDATE) {
			UpdateLocation(lParam);
			return *pResult = 1;

		} else if(wParam >= MLC_IDC_MENU) {

			CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
			int iCount = pHeaderCtrl->GetItemCount();

			int iToggle = wParam - MLC_IDC_MENU;
			if(iToggle >= iCount)
				break;

			if(m_aColumns[iToggle].bHidden)
				ShowColumn(iToggle);
			else
				HideColumn(iToggle);

			return *pResult = 1;
		}
		break;

	case LVM_DELETECOLUMN:
		//book keeping!
		if(m_aColumns != NULL) {
			for(int i = 0; i < m_iColumnsTracked; i++)
				if(m_aColumns[i].bHidden)
					ShowColumn(i);

			delete[] m_aColumns;
			m_aColumns = NULL; // 'new' may throw an exception
		}
		m_aColumns = new MULE_COLUMN[--m_iColumnsTracked];
		for(int i = 0; i < m_iColumnsTracked; i++) {
			m_aColumns[i].iLocation = i;
			m_aColumns[i].bHidden = false;
		}
		break;

	//case LVM_INSERTCOLUMN:
	case LVM_INSERTCOLUMNA:
	case LVM_INSERTCOLUMNW:
		//book keeping!

		if(m_aColumns != NULL) {
			for(int i = 0; i < m_iColumnsTracked; i++)
				if(m_aColumns[i].bHidden)
					ShowColumn(i);

			delete[] m_aColumns;
			m_aColumns = NULL; // 'new' may throw an exception
		}
		m_aColumns = new MULE_COLUMN[++m_iColumnsTracked];
		for(int i = 0; i < m_iColumnsTracked; i++) {
			m_aColumns[i].iLocation = i;
			m_aColumns[i].bHidden = false;
		}
		break;

	case LVM_SETITEM:
		//book keeping
		{
			POSITION pos = m_Params.FindIndex(((LPLVITEM)lParam)->iItem);
			if(pos) {
				m_Params.SetAt(pos, MLC_MAGIC);
				PostMessage(LVM_UPDATE, ((LPLVITEM)lParam)->iItem);
			}
		}
		break;
	case LVN_KEYDOWN:{
		break;}
	case LVM_SETITEMTEXT:
		//need to check for movement

		*pResult = DefWindowProc(message, wParam, lParam);
		if(*pResult)
			PostMessage(WM_COMMAND, MLC_IDC_UPDATE, wParam);
		return *pResult;

	case LVM_SORTITEMS:
		//book keeping...

		// SLUGFILLER: multiSort
		if (m_SortProc == (PFNLVCOMPARE)lParam) {
			if (m_dwParamSort.Find((LPARAM)wParam) != NULL)
				m_dwParamSort.RemoveAt(m_dwParamSort.Find((LPARAM)wParam));
		}
		else {
			while (!m_dwParamSort.IsEmpty())
				m_dwParamSort.RemoveHead();
		}
		m_dwParamSort.AddHead((LPARAM)wParam);
		// SLUGFILLER: multiSort
		m_SortProc = (PFNLVCOMPARE)lParam;
		for(POSITION pos = m_Params.GetHeadPosition(); pos != NULL; m_Params.GetNext(pos))
			m_Params.SetAt(pos, MLC_MAGIC);
		break;
	case LVM_DELETEALLITEMS:
		//book keeping...

		if(!CListCtrl::OnWndMsg(message, wParam, lParam, pResult) && DefWindowProc(message, wParam, lParam)) 
			m_Params.RemoveAll();
		return *pResult = TRUE;

	case LVM_DELETEITEM:
		//book keeping.....

		MLC_ASSERT(m_Params.GetAt(m_Params.FindIndex(wParam)) == CListCtrl::GetItemData(wParam));
		if(!CListCtrl::OnWndMsg(message, wParam, lParam, pResult) && DefWindowProc(message, wParam, lParam))
				m_Params.RemoveAt(m_Params.FindIndex(wParam));
		return *pResult = TRUE;

	//case LVM_INSERTITEM:
	case LVM_INSERTITEMA:
	case LVM_INSERTITEMW:
		//try to fix position of inserted items
		{
			LPLVITEM pItem = (LPLVITEM)lParam;
			int iItem = pItem->iItem;
			int iItemCount = GetItemCount();
			BOOL notLast = iItem < iItemCount;
			BOOL notFirst = iItem > 0;

			if(notFirst) {
				int iNewIndex = iItem - 1;
				POSITION pos = m_Params.FindIndex(iNewIndex);
				int iResult = MultiSortProc(pItem->lParam, GetParamAt(pos, iNewIndex));	// SLUGFILLER: multiSort
				if(iResult < 0) {
					POSITION posPrev = pos;
					int iDist = iNewIndex / 2;
					while(iDist > 1) {
						for(int i = 0; i < iDist; i++)
							m_Params.GetPrev(posPrev);

						if(MultiSortProc(pItem->lParam, GetParamAt(posPrev, iNewIndex - iDist)) < 0) {	// SLUGFILLER: multiSort
							iNewIndex = iNewIndex - iDist;
							pos = posPrev;
						} else {
							posPrev = pos;
						}
						iDist /= 2;
					}
					while(--iNewIndex >= 0) {
						m_Params.GetPrev(pos);
						if(MultiSortProc(pItem->lParam, GetParamAt(pos, iNewIndex)) >= 0)	// SLUGFILLER: multiSort
							break;
					}
					pItem->iItem = iNewIndex + 1;
					notLast = false;
				}
			}

			if(notLast) {
				int iNewIndex = iItem;
				POSITION pos = m_Params.FindIndex(iNewIndex);
				int iResult = MultiSortProc(pItem->lParam, GetParamAt(pos, iNewIndex));	// SLUGFILLER: multiSort
				if(iResult > 0) {
					POSITION posNext = pos;
					int iDist = (GetItemCount() - iNewIndex) / 2;
					while(iDist > 1) {
						for(int i = 0; i < iDist; i++)
							m_Params.GetNext(posNext);

						if(MultiSortProc(pItem->lParam, GetParamAt(posNext, iNewIndex + iDist)) > 0) {	// SLUGFILLER: multiSort
							iNewIndex = iNewIndex + iDist;
							pos = posNext;
						} else {
							posNext = pos;
						}
						iDist /= 2;
					}
					while(++iNewIndex < iItemCount) {
						m_Params.GetNext(pos);
						if(MultiSortProc(pItem->lParam, GetParamAt(pos, iNewIndex)) <= 0)	// SLUGFILLER: multiSort
							break;
					}
					pItem->iItem = iNewIndex;
				}
			}

			if(pItem->iItem == 0) {
				m_Params.AddHead(pItem->lParam);
				return FALSE;
			}

			LRESULT lResult = DefWindowProc(message, wParam, lParam);
			if(lResult != -1) {
				if(lResult >= GetItemCount())
					m_Params.AddTail(pItem->lParam);
				else if(lResult == 0)
					m_Params.AddHead(pItem->lParam);
				else
					m_Params.InsertAfter(m_Params.FindIndex(lResult - 1), pItem->lParam);
			}
			return *pResult = lResult;
		}
		break;

	case LVM_UPDATE:
		//better fix for old problem... normally Update(int) causes entire list to redraw

		if(wParam == UpdateLocation(wParam)) { //no need to invalidate rect if item moved
			RECT rcItem;
			BOOL bResult = GetItemRect(wParam, &rcItem, LVIR_BOUNDS);
			if(bResult)
				InvalidateRect(&rcItem, FALSE);
			return *pResult = bResult;
		}
		return *pResult = TRUE;
	}

	return CListCtrl::OnWndMsg(message, wParam, lParam, pResult);
}

void CMuleListCtrl::OnKeyDown(UINT nChar,UINT nRepCnt,UINT nFlags)
{
	if (nChar == 'A' && ::GetAsyncKeyState(VK_CONTROL)<0)
	{
		// Ctrl+A: Select all items
		LV_ITEM theItem;
		theItem.mask= LVIF_STATE;
		theItem.iItem= -1;
		theItem.iSubItem= 0;
		theItem.state= LVIS_SELECTED;
		theItem.stateMask= 2;
		SetItemState(-1, &theItem);
	}
	else if (nChar==VK_DELETE)
		PostMessage(WM_COMMAND, MPG_DELETE, 0);
	else if (nChar==VK_F2)
		PostMessage(WM_COMMAND, MPG_F2, 0);
	else if (nChar == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		// Ctrl+C: Copy keycombo
		SendMessage(WM_COMMAND, MP_COPYSELECTED);
	}
	if (nChar == 'V' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		// Ctrl+V: Paste keycombo
		SendMessage(WM_COMMAND, MP_PASTE);
	}
	else if (m_bGeneralPurposeFind){
		if (nChar == 'F' && (GetKeyState(VK_CONTROL) & 0x8000)){
			// Ctrl+F: Search item
			OnFindStart();
		}
		else if (nChar == VK_F3){
			if (GetKeyState(VK_SHIFT) & 0x8000){
				// Shift+F3: Search previous
				OnFindPrev();
			}
			else{
				// F3: Search next
				OnFindNext();
			}
		}
	}

	return CListCtrl::OnKeyDown(nChar,nRepCnt,nFlags);
}

BOOL CMuleListCtrl::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	if(message != WM_DRAWITEM) {
		//catch the prepaint and copy struct
		if(message == WM_NOTIFY && ((NMHDR*)lParam)->code == NM_CUSTOMDRAW &&
		  ((LPNMLVCUSTOMDRAW)lParam)->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {

			m_bCustomDraw = CListCtrl::OnChildNotify(message, wParam, lParam, pResult);
			if(m_bCustomDraw)
				m_lvcd = *((LPNMLVCUSTOMDRAW)lParam);

			return m_bCustomDraw;
		}

		return CListCtrl::OnChildNotify(message, wParam, lParam, pResult);
	}

	ASSERT(pResult == NULL); // no return value expected
	UNUSED(pResult);         // unused in release builds

	DrawItem((LPDRAWITEMSTRUCT)lParam);
	return TRUE;
}

//////////////////////////////////
// CMuleListCtrl message map

BEGIN_MESSAGE_MAP(CMuleListCtrl, CListCtrl)
	ON_WM_DRAWITEM()
	ON_WM_KEYDOWN()
	ON_WM_ERASEBKGND()
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

//////////////////////////////////
// CMuleListCtrl message handlers

void CMuleListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) {
	//set up our flicker free drawing
	CRect rcItem(lpDrawItemStruct->rcItem);
	CDC *oDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	COLORREF crOldDCBkColor = oDC->SetBkColor(m_crWindow);
	CMemDC pDC(oDC, &rcItem);	
	CFont *pOldFont = pDC->SelectObject(GetFont());
	COLORREF crOldTextColor;
	if(m_bCustomDraw)
		crOldTextColor = pDC->SetTextColor(m_lvcd.clrText);
	else
		crOldTextColor = pDC->SetTextColor(m_crWindowText);

	if (m_crWindowTextBk == CLR_NONE){
		DefWindowProc(WM_ERASEBKGND, (WPARAM)pDC->m_hDC, 0);
	}

	int iOffset = pDC->GetTextExtent(_T(" "), 1 ).cx*2;
	int iItem = lpDrawItemStruct->itemID;
	CImageList* pImageList;
	CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();

	//gets the item image and state info
	LV_ITEM lvi;
	lvi.mask = LVIF_IMAGE | LVIF_STATE;
	lvi.iItem = iItem;
	lvi.iSubItem = 0;
	lvi.stateMask = LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED | LVIS_GLOW; 
	GetItem(&lvi);

	//see if the item be highlighted
	BOOL bHighlight = ((lvi.state & LVIS_DROPHILITED) || (lvi.state & LVIS_SELECTED));
	BOOL bCtrlFocused = ((GetFocus() == this) || (GetStyle() & LVS_SHOWSELALWAYS));
	BOOL bGlowing = ( lvi.state & LVIS_GLOW );

	//get rectangles for drawing
	CRect rcBounds, rcLabel, rcIcon;
	GetItemRect(iItem, rcBounds, LVIR_BOUNDS);
	GetItemRect(iItem, rcLabel, LVIR_LABEL);
	GetItemRect(iItem, rcIcon, LVIR_ICON);
	CRect rcCol(rcBounds);

	//the label!
	CString sLabel = GetItemText(iItem, 0);
	//labels are offset by a certain amount
	//this offset is related to the width of a space character
	CRect rcHighlight;
	CRect rcWnd;

	//should I check (GetExtendedStyle() & LVS_EX_FULLROWSELECT) ?
	rcHighlight.top    = rcBounds.top;
	rcHighlight.bottom = rcBounds.bottom;
	rcHighlight.left   = rcBounds.left  + 1;
	rcHighlight.right  = rcBounds.right - 1;

	COLORREF crOldBckColor;
	//draw the background color
	if(bHighlight) 
	{
		if(bCtrlFocused) 
		{
			pDC->FillRect(rcHighlight, &CBrush(m_crHighlight));
			crOldBckColor = pDC->SetBkColor(m_crHighlight);
		}
		else if(bGlowing)
		{
			pDC->FillRect(rcHighlight, &CBrush(m_crGlow));
			crOldBckColor = pDC->SetBkColor(m_crGlow);
		}
		else 
		{
			pDC->FillRect(rcHighlight, &CBrush(m_crNoHighlight));
			crOldBckColor = pDC->SetBkColor(m_crNoHighlight);
		}
	} 
	else
	{
		if(bGlowing)
		{
			pDC->FillRect(rcHighlight, &CBrush(m_crGlow));
			crOldBckColor = pDC->SetBkColor(m_crGlow);
		}
		else
		{
			if (m_crWindowTextBk != CLR_NONE)
				pDC->FillRect(rcHighlight, &CBrush(m_crWindow)); // was already done with WM_ERASEBKGND
			crOldBckColor = pDC->SetBkColor(m_crWindow);
		}
	}

	//update column
	rcCol.right = rcCol.left + GetColumnWidth(0);

	//draw state icon
	if(lvi.state & LVIS_STATEIMAGEMASK) 
	{
		int nImage = ((lvi.state & LVIS_STATEIMAGEMASK)>>12) - 1;
		pImageList = GetImageList(LVSIL_STATE);
		if(pImageList) 
		{
			COLORREF crOld = pImageList->SetBkColor(CLR_NONE);
			pImageList->Draw(pDC, nImage, rcCol.TopLeft(), ILD_NORMAL);
			pImageList->SetBkColor(crOld);
		}
	}

	//draw the item's icon
	pImageList = GetImageList(LVSIL_SMALL);
	if(pImageList) 
	{
		COLORREF crOld = pImageList->SetBkColor(CLR_NONE);
		pImageList->Draw(pDC, lvi.iImage, rcIcon.TopLeft(), ILD_NORMAL);
		pImageList->SetBkColor(crOld);
	}

	int iOldBkMode = (m_crWindowTextBk == CLR_NONE) ? pDC->SetBkMode(TRANSPARENT) : OPAQUE;

	//draw item label (column 0)
	rcLabel.left += iOffset / 2;
	rcLabel.right -= iOffset;
	pDC->DrawText(sLabel, -1, rcLabel, MLC_DT_TEXT | DT_LEFT | DT_NOCLIP);

	//draw labels for remaining columns
	LV_COLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH;
	rcBounds.right = rcHighlight.right > rcBounds.right ? rcHighlight.right : rcBounds.right;

	int iCount = pHeaderCtrl->GetItemCount();
	for(int iCurrent = 1; iCurrent < iCount; iCurrent++) 
	{
		int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
		//don't draw column 0 again
		if(iColumn == 0)
			continue;

		GetColumn(iColumn, &lvc);
		//don't draw anything with 0 width
		if(lvc.cx == 0)
			continue;

		rcCol.left = rcCol.right;
		rcCol.right += lvc.cx;

		sLabel = GetItemText(iItem, iColumn);
		if (sLabel.GetLength() == 0)
			continue;

		//get the text justification
		UINT nJustify = DT_LEFT;
		switch(lvc.fmt & LVCFMT_JUSTIFYMASK) 
		{
			case LVCFMT_RIGHT:
				nJustify = DT_RIGHT;
				break;
			case LVCFMT_CENTER:
				nJustify = DT_CENTER;
				break;
			default:
				break;
		}

		rcLabel = rcCol;
		rcLabel.left += iOffset;
		rcLabel.right -= iOffset;

		pDC->DrawText(sLabel, -1, rcLabel, MLC_DT_TEXT | nJustify);
	}

	//draw focus rectangle if item has focus
	if((lvi.state & LVIS_FOCUSED) && (bCtrlFocused || (lvi.state & LVIS_SELECTED))) 
	{
		if(!bCtrlFocused || !(lvi.state & LVIS_SELECTED))
			pDC->FrameRect(rcHighlight, &CBrush(m_crNoFocusLine));
		else
			pDC->FrameRect(rcHighlight, &CBrush(m_crFocusLine));
	}

	pDC->Flush();
	if (m_crWindowTextBk == CLR_NONE)
		pDC->SetBkMode(iOldBkMode);
	pDC->SelectObject(pOldFont);
	pDC->SetTextColor(crOldTextColor);
	pDC->SetBkColor(crOldBckColor);
	oDC->SetBkColor(crOldDCBkColor);
}

BOOL CMuleListCtrl::OnEraseBkgnd(CDC* pDC)
{
//	if (m_crWindowTextBk == CLR_NONE) // this creates a lot screen flickering
//		return CListCtrl::OnEraseBkgnd(pDC);

	int itemCount = GetItemCount();
	if (!itemCount)
		return CListCtrl::OnEraseBkgnd(pDC);

	RECT clientRect;
	RECT itemRect;
	int topIndex = GetTopIndex();
	int maxItems = GetCountPerPage();
	int drawnItems = itemCount < maxItems ? itemCount : maxItems;
	CRect rcClip;

	//draw top portion
	GetClientRect(&clientRect);
	rcClip = clientRect;
	GetItemRect(topIndex, &itemRect, LVIR_BOUNDS);
	clientRect.bottom = itemRect.top;
	if (m_crWindowTextBk != CLR_NONE)
		pDC->FillSolidRect(&clientRect,GetBkColor());
	else
		rcClip.top = itemRect.top;

	//draw bottom portion if we have to
	if(topIndex + maxItems >= itemCount) {
		GetClientRect(&clientRect);
		GetItemRect(topIndex + drawnItems - 1, &itemRect, LVIR_BOUNDS);
		clientRect.top = itemRect.bottom;
		rcClip.bottom = itemRect.bottom;
		if (m_crWindowTextBk != CLR_NONE)
			pDC->FillSolidRect(&clientRect, GetBkColor());
	}

	//draw right half if we need to
	if (itemRect.right < clientRect.right) {
		GetClientRect(&clientRect);
		clientRect.left = itemRect.right;
		rcClip.right = itemRect.right;
		if (m_crWindowTextBk != CLR_NONE)
			pDC->FillSolidRect(&clientRect, GetBkColor());
	}

	if (m_crWindowTextBk == CLR_NONE){
		CRect rcClipBox;
		pDC->GetClipBox(&rcClipBox);
		rcClipBox.SubtractRect(&rcClipBox, &rcClip);
		if (!rcClipBox.IsRectEmpty()){
			pDC->ExcludeClipRect(&rcClip);
			CListCtrl::OnEraseBkgnd(pDC);
			InvalidateRect(&rcClip, FALSE);
		}
	}
	return TRUE;
}

void CMuleListCtrl::OnSysColorChange()
{
	//adjust colors
	CListCtrl::OnSysColorChange();
	SetColors();
	
	//redraw the up/down sort arrow (if it's there)
	if(m_iCurrentSortItem >= 0)
		SetSortArrow(m_iCurrentSortItem, (ArrowType)m_atSortArrow);
}

HIMAGELIST CMuleListCtrl::ApplyImageList(HIMAGELIST himl)
{
	HIMAGELIST himlOld = (HIMAGELIST)SendMessage(LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)himl);
	if (m_imlHeaderCtrl.m_hImageList != NULL){
		// Must *again* set the image list for the header control, because LVM_SETIMAGELIST
		// always resets any already specified header control image lists!
		GetHeaderCtrl()->SetImageList(&m_imlHeaderCtrl);
	}
	return himlOld;
}

void CMuleListCtrl::DoFind(int iStartItem, int iDirection /*1=down, 0 = up*/, BOOL bShowError)
{
	CWaitCursor curHourglass;

	if (iStartItem < 0) {
		MessageBeep(MB_OK);
		return;
	}

	int iNumItems = iDirection ? GetItemCount() : 0;
	int iItem = iStartItem;
	while ( iDirection ? iItem < iNumItems : iItem >= 0 )
	{
		CString strItemText(GetItemText(iItem, m_iFindColumn));
		if (!strItemText.IsEmpty())
		{
			if ( m_bFindMatchCase
				   ? _tcsstr(strItemText, m_strFindText) != NULL
				   : stristr(strItemText, m_strFindText) != NULL )
			{
				// Deselect all listview entries
				SetItemState(-1, 0, LVIS_SELECTED);

				// Select the found listview entry
				SetItemState(iItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
				SetSelectionMark(iItem);
				EnsureVisible(iItem, FALSE/*bPartialOK*/);
				SetFocus();

				return;
			}
		}

		if (iDirection)
			iItem++;
		else
			iItem--;
	}

	if (bShowError)
		AfxMessageBox(GetResString(IDS_SEARCH_NORESULT), MB_ICONINFORMATION);
	else
		MessageBeep(MB_OK);
}

void CMuleListCtrl::OnFindStart()
{
	CListViewSearchDlg dlg;
	dlg.m_pListView = this;
	dlg.m_strFindText = m_strFindText;
	dlg.m_iSearchColumn = m_iFindColumn;
	if (dlg.DoModal() != IDOK || dlg.m_strFindText.IsEmpty())
		return;
	m_strFindText = dlg.m_strFindText;
	m_iFindColumn = dlg.m_iSearchColumn;

	DoFindNext(TRUE/*bShowError*/);
}

void CMuleListCtrl::OnFindNext()
{
	DoFindNext(FALSE/*bShowError*/);
}

void CMuleListCtrl::DoFindNext(BOOL bShowError)
{
	int iStartItem = GetNextItem(-1, LVNI_SELECTED | LVNI_FOCUSED);
	if (iStartItem == -1)
		iStartItem = 0;
	else
		iStartItem = iStartItem + (m_iFindDirection ? 1 : -1);
	DoFind(iStartItem, m_iFindDirection, bShowError);
}

void CMuleListCtrl::OnFindPrev()
{
	int iStartItem = GetNextItem(-1, LVNI_SELECTED | LVNI_FOCUSED);
	if (iStartItem == -1)
		iStartItem = 0;
	else
		iStartItem = iStartItem + (!m_iFindDirection ? 1 : -1);

	DoFind(iStartItem, !m_iFindDirection, FALSE/*bShowError*/);
}

BOOL CMuleListCtrl::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_SYSKEYDOWN && pMsg->wParam == VK_RETURN && GetAsyncKeyState(VK_MENU)<0) {
		PostMessage(WM_COMMAND, MPG_ALTENTER, 0);
		return TRUE;
	}

	return CListCtrl::PreTranslateMessage(pMsg);
}

void CMuleListCtrl::AutoSelectItem()
{
	int iItem = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iItem == -1)
	{
		iItem = GetNextItem(-1, LVIS_FOCUSED);
		if (iItem != -1)
		{
			SetItemState(iItem, LVIS_SELECTED, LVIS_SELECTED);
			SetSelectionMark(iItem);
		}
	}
}

/*
void CMuleListCtrl::UpdateSortHistory(int dwNewOrder, int dwInverseValue){
	int dwInverse = (dwNewOrder > dwInverseValue) ? (dwNewOrder-dwInverseValue) : (dwNewOrder+dwInverseValue);
	// delete the value (or its inverse sorting value) if it appears already in the list
	POSITION pos1, pos2;
	for (pos1 = m_liSortHistory.GetHeadPosition();( pos2 = pos1 ) != NULL;)
	{
		m_liSortHistory.GetNext(pos1);
		if (m_liSortHistory.GetAt(pos2) == dwNewOrder || m_liSortHistory.GetAt(pos2) == dwInverse)
			m_liSortHistory.RemoveAt(pos2);
	}
	m_liSortHistory.AddHead(dwNewOrder);
	// limit it to 4 entries for now, just for performance
	if (m_liSortHistory.GetSize() > 4)
		m_liSortHistory.RemoveTail();
}

int	CMuleListCtrl::GetNextSortOrder(int dwCurrentSortOrder) const{
	POSITION pos1, pos2;
	for (pos1 = m_liSortHistory.GetHeadPosition();( pos2 = pos1 ) != NULL;)
	{
		m_liSortHistory.GetNext(pos1);
		if (m_liSortHistory.GetAt(pos2) == dwCurrentSortOrder){
			if (pos1 == NULL)
				return -1; // there is no further sortorder stored
			else
				return m_liSortHistory.GetAt(pos1);
		}
	}
	// current one not found, shouldn't happen
	ASSERT( false );
	return -1;
}*/
