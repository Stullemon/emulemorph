//this file is part of eMule
//Copyright (C)2002-2007 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
static char THIS_FILE[] = __FILE__; 
#endif

#define MAX_SORTORDERHISTORY 4
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

BEGIN_MESSAGE_MAP(CMuleListCtrl, CListCtrl)
	ON_WM_DRAWITEM()
	ON_WM_KEYDOWN()
	ON_WM_ERASEBKGND()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
END_MESSAGE_MAP()

CMuleListCtrl::CMuleListCtrl(PFNLVCOMPARE pfnCompare, DWORD dwParamSort)
{
	m_SortProc = pfnCompare;
	m_dwParamSort = dwParamSort;
	UpdateSortHistory(m_dwParamSort, 0);	// SLUGFILLER: multiSort - fail-safe, ensure it's in the sort history(no inverse check)

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
    m_crHighlightText = m_crWindowText;
	m_crGlow=0;
    m_crFocusLine = 0;
    m_crNoHighlight = 0;
    m_crNoFocusLine = 0;
	m_bGeneralPurposeFind = false;
	m_bCanSearchInAllColumns = false;
    m_bFindMatchCase = false;
    m_iFindDirection = 1;
    m_iFindColumn = 0;
	m_hAccel = NULL;
	m_uIDAccel = IDR_LISTVIEW;
	m_eUpdateMode = lazy;
    // not for server list and download list	
	// MORPH START leuk_he:run as ntservice v1..
	if (theApp.IsRunningAsService(SVC_SVR_OPT )) return; // THIS SHOULD BE SVC_LIST_OPT for other than server	TODO.
	// MORPH END leuk_he:run as ntservice v1..
	//MORPH START - UpdateItemThread
	m_updatethread = (CUpdateItemThread*) AfxBeginThread(RUNTIME_CLASS(CUpdateItemThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
	m_updatethread->ResumeThread();
	m_updatethread->SetListCtrl(this);
	//MORPH END   - UpdateItemThread
}

CMuleListCtrl::~CMuleListCtrl() {
	delete[] m_aColumns;
	// MORPH START leuk_he:run as ntservice v1..
   	if (theApp.IsRunningAsService(SVC_SVR_OPT)) return;
	// MORPH END leuk_he:run as ntservice v1..
		m_updatethread->EndThread(); //MORPH - UpdateItemThread
}

int CMuleListCtrl::SortProc(LPARAM /*lParam1*/, LPARAM /*lParam2*/, LPARAM /*lParamSort*/)
{
	return 0;
}

void CMuleListCtrl::SetName(LPCTSTR lpszName) {
	m_Name = lpszName;
}

void CMuleListCtrl::PreSubclassWindow()
{
	SetColors();
	CListCtrl::PreSubclassWindow();
	// Win98: Explicitly set to Unicode to receive Unicode notifications.
	SendMessage(CCM_SETUNICODEFORMAT, TRUE);
	SetExtendedStyle(LVS_EX_HEADERDRAGDROP);

	// If we want to handle the VK_RETURN key, we have to do that via accelerators!
	if (m_uIDAccel != (UINT)-1) {
		m_hAccel = ::LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(m_uIDAccel));
		ASSERT(m_hAccel);
	}
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

void CMuleListCtrl::SaveSettings()
{
	ASSERT(!m_Name.IsEmpty());
	
	ASSERT(GetHeaderCtrl()->GetItemCount() == m_iColumnsTracked);

	if (m_Name.IsEmpty() || GetHeaderCtrl()->GetItemCount() != m_iColumnsTracked)
		return;

	CIni ini(thePrefs.GetConfigFile(), _T("ListControlSetup"));

	ShowWindow(SW_HIDE);

	// SLUGFILLER: multiSort - store unlimited sorts
	int i;
	CString strSortHist;
	POSITION pos = m_liSortHistory.GetTailPosition();
	if (pos != NULL) {
		strSortHist.Format(_T("%d"), m_liSortHistory.GetPrev(pos));
		while (pos != NULL) {
			strSortHist.AppendChar(_T(','));
			strSortHist.AppendFormat(_T("%d"), m_liSortHistory.GetPrev(pos));
		}
	}
	ini.WriteString(m_Name + _T("SortHistory"), strSortHist);
	// SLUGFILLER: multiSort
	// store additional settings
	ini.WriteInt( m_Name + _T("TableSortItem"), GetSortItem() );
	ini.WriteInt( m_Name + _T("TableSortAscending"), GetSortType( m_atSortArrow ));

	int* piColWidths = new int[m_iColumnsTracked];
	int* piColHidden = new int[m_iColumnsTracked];
	INT *piColOrders = new INT[m_iColumnsTracked];
	for(i = 0; i < m_iColumnsTracked; i++)
	{
		piColWidths[i] = GetColumnWidth(i);
		piColHidden[i] = IsColumnHidden(i);
		ShowColumn(i);
	}

	GetHeaderCtrl()->GetOrderArray(piColOrders, m_iColumnsTracked);
	ini.SerGet(false, piColWidths, m_iColumnsTracked, m_Name + _T("ColumnWidths"));
	ini.SerGet(false, piColHidden, m_iColumnsTracked, m_Name + _T("ColumnHidden"));
	ini.SerGet(false, piColOrders, m_iColumnsTracked, m_Name + _T("ColumnOrders"));

	for(i = 0; i < m_iColumnsTracked; i++)
		if (piColHidden[i]==1)
			HideColumn(i);
	
	ShowWindow(SW_SHOW);

	// SLUGFILLER: multiSort remove - unused
	delete[] piColOrders;
	delete[] piColWidths;
	delete[] piColHidden;
}

int		CMuleListCtrl::GetSortType(ArrowType at){
	switch(at) {
		case arrowDown	: return 0;
		case arrowUp	: return 1;
		case arrowDoubleDown	: return 2;
		case arrowDoubleUp		: return 3;
	}
	return 0;
}

CMuleListCtrl::ArrowType CMuleListCtrl::GetArrowType(int iat) {
	switch (iat){
		case 0: return arrowDown;
		case 1: return arrowUp;
		case 2: return arrowDoubleDown;
		case 3: return arrowDoubleUp;
	}
	return arrowDown;
}

void CMuleListCtrl::LoadSettings()
{
	ASSERT(!m_Name.IsEmpty());
	if (m_Name.IsEmpty())
		return;

	CIni ini(thePrefs.GetConfigFile(), _T("ListControlSetup"));
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();

	// sort history
	// SLUGFILLER: multiSort - read unlimited sorts
	CString strSortHist = ini.GetString(m_Name + _T("SortHistory"));
	int nOffset = 0;
	CString strTemp;
	nOffset = ini.Parse(strSortHist, nOffset, strTemp);
	while (!strTemp.IsEmpty()) {
		UpdateSortHistory((int)_tstoi(strTemp), 0);	// avoid duplicates(cannot detect inverse, but it does half the job)
		nOffset = ini.Parse(strSortHist, nOffset, strTemp);
	}
	// SLUGFILLER: multiSort
	
	m_iCurrentSortItem= ini.GetInt( m_Name + _T("TableSortItem"), 0);
	m_atSortArrow = GetArrowType(ini.GetInt(m_Name + _T("TableSortAscending"), 1));
	if (m_liSortHistory.IsEmpty())
		m_liSortHistory.AddTail(m_iCurrentSortItem);

	// columns settings
	int* piColWidths = new int[m_iColumnsTracked];
	int* piColHidden = new int[m_iColumnsTracked];
	INT* piColOrders = new int[m_iColumnsTracked];
	ini.SerGet(true, piColWidths, m_iColumnsTracked, m_Name + _T("ColumnWidths"));
	ini.SerGet(true, piColHidden, m_iColumnsTracked, m_Name + _T("ColumnHidden"));
	ini.SerGet(true, piColOrders, m_iColumnsTracked, m_Name + _T("ColumnOrders"));
	
	// apply columnwidths and verify sortorder
	INT *piArray = new INT[m_iColumnsTracked];
	for (int i = 0; i < m_iColumnsTracked; i++)
	{
		piArray[i] = i;

		if (piColWidths[i] >= 2) // don't allow column widths of 0 and 1 -- just because it looks very confusing in GUI
			SetColumnWidth(i, piColWidths[i]);

		int iOrder = piColOrders[i];
		if (i>0 && iOrder > 0 && iOrder < m_iColumnsTracked && iOrder != i)
			piArray[i] = iOrder;
		m_aColumns[i].iLocation = piArray[i];
	}
	piArray[0] = 0;

	for(int i = 0; i < m_iColumnsTracked; i++)
		m_aColumns[piArray[i]].iLocation = i;
	pHeaderCtrl->SetOrderArray(m_iColumnsTracked, piArray);

	for(int i = 1; i < m_iColumnsTracked; i++) {
		if (piColHidden[i])
			HideColumn(i);
	}

	delete[] piArray;
	delete[] piColOrders;
	delete[] piColWidths;
	delete[] piColHidden;
	// SLUGFILLER: multiSort remove - unused
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
		GetPrivateProfileString(_T("Colors"), strKey + _T("BkImg"), _T(""), szColor, _countof(szColor), pszSkinProfile);
		if (szColor[0] == _T('\0'))
			GetPrivateProfileString(_T("Colors"), _T("DefLvBkImg"), _T(""), szColor, _countof(szColor), pszSkinProfile);
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
		if (ExpandEnvironmentStrings(strBkImage, szExpSkinRes, _countof(szExpSkinRes)) != 0)
			strBkImage = szExpSkinRes;

		// create absolute path to icon resource file
		TCHAR szFullResPath[MAX_PATH];
		if (PathIsRelative(strBkImage))
		{
			TCHAR szSkinResFolder[MAX_PATH];
			_tcsncpy(szSkinResFolder, pszSkinProfile, _countof(szSkinResFolder));
			szSkinResFolder[_countof(szSkinResFolder)-1] = _T('\0');
			PathRemoveFileSpec(szSkinResFolder);
			_tmakepathlimit(szFullResPath, NULL, szSkinResFolder, strBkImage, NULL);
		}
		else
		{
			_tcsncpy(szFullResPath, strBkImage, _countof(szFullResPath));
			szFullResPath[_countof(szFullResPath)-1] = _T('\0');
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

	m_crFocusLine = crHighlight;
	if (g_bLowColorDesktop) {
		m_crNoHighlight		= crHighlight;
		m_crNoFocusLine		= crHighlight;
		m_crHighlight		= crHighlight;
		m_crHighlightText	= GetSysColor(COLOR_HIGHLIGHTTEXT);
		m_crGlow			= crHighlight;
	} else {
		m_crNoHighlight		= MLC_RGBBLEND(crHighlight, m_crWindow, 8);
		m_crNoFocusLine		= MLC_RGBBLEND(crHighlight, m_crWindow, 2);
		m_crHighlight		= MLC_RGBBLEND(crHighlight, m_crWindow, 4);
		m_crHighlightText	= m_crWindowText;
		m_crGlow			= MLC_RGBBLEND(crHighlight, m_crWindow, 3);
	}
}

void CMuleListCtrl::SetSortArrow(int iColumn, ArrowType atType) {
	HDITEM headerItem;
	headerItem.mask = HDI_FORMAT;
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();

	if(iColumn != m_iCurrentSortItem) {
		pHeaderCtrl->GetItem(m_iCurrentSortItem & 63, &headerItem); //MORPH - Changed by SiRoB, DLsortFix: it's ok until we have not more than 28 columns
		headerItem.fmt &= ~(HDF_IMAGE | HDF_BITMAP_ON_RIGHT);
		pHeaderCtrl->SetItem(m_iCurrentSortItem & 63, &headerItem); //MORPH - Changed by SiRoB, DLsortFix: it's ok until we have not more than 28 columns
		m_iCurrentSortItem = iColumn;
		m_imlHeaderCtrl.DeleteImageList();
	}

	//place new arrow unless we were given an invalid column
	if(iColumn >= 0 && pHeaderCtrl->GetItem(iColumn & 63, &headerItem)) { //MORPH - Changed by SiRoB, DLsortFix: it's ok until we have not more than 28 columns
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
		pHeaderCtrl->SetItem(iColumn & 63, &headerItem); //MORPH - Changed by SiRoB, DLsortFix: it's ok until we have not more than 28 columns
	}
}

// move item in list, returns index of new item
int CMuleListCtrl::MoveItem(int iOldIndex, int iNewIndex)
{
	if(iNewIndex > iOldIndex)
		iNewIndex--;

	// netfinity start: Don't move item if new index is the same as the old one
	if(iNewIndex == iOldIndex)
		return iNewIndex;
  // netf end
	// copy item
	LVITEM lvi;
	TCHAR szText[256];
	lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM | LVIF_INDENT | LVIF_IMAGE | LVIF_NORECOMPUTE;
	lvi.stateMask = (UINT)-1;
	lvi.iItem = iOldIndex;
	lvi.iSubItem = 0;
	lvi.pszText = szText;
	lvi.cchTextMax = _countof(szText);
	lvi.iIndent = 0;
	if (!GetItem(&lvi))
		return -1;

	// copy strings of sub items
	CSimpleArray<void*> aSubItems;
	DWORD Style = GetStyle();
	if((Style & LVS_OWNERDATA) == 0) {
		TCHAR szText[256];
		LVITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_NORECOMPUTE;
		lvi.iItem = iOldIndex;
		for(int i = 1; i < m_iColumnsTracked; i++){
			lvi.iSubItem = i;
			lvi.cchTextMax = _countof(szText);
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

	//do the move
	SetRedraw(FALSE);
	DeleteItem(iOldIndex);
	lvi.iItem = iNewIndex;
	iNewIndex = InsertItem(&lvi);

	// restore strings of sub items
	if((Style & LVS_OWNERDATA) == 0) {
		for(int i = 1; i < m_iColumnsTracked; i++) {
			LVITEM lvi;
			lvi.iSubItem = i;
			void* pstrSubItem = aSubItems[i-1];
			if (pstrSubItem != NULL){
				if (pstrSubItem == LPSTR_TEXTCALLBACK)
					lvi.pszText = LPSTR_TEXTCALLBACK;
				else
					lvi.pszText = const_cast<LPTSTR>((LPCTSTR)*((CString *)pstrSubItem));
				DefWindowProc(LVM_SETITEMTEXT, iNewIndex, (LPARAM)&lvi);
				if (pstrSubItem != LPSTR_TEXTCALLBACK)
					delete (CString*)pstrSubItem;
			}
		}
	}

	SetRedraw(TRUE);

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
	if (pos == NULL)
		return 0;
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

				CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
				int iCount = pHeaderCtrl->GetItemCount();
				for(int iCurrent = 1; iCurrent < iCount; iCurrent++) {
					HDITEM item;
					TCHAR text[255];
					item.pszText = text;
					item.mask = HDI_TEXT;
					item.cchTextMax = _countof(text);
					pHeaderCtrl->GetItem(iCurrent, &item);

					tmColumnMenu.AppendMenu(MF_STRING | (m_aColumns[iCurrent].bHidden ? 0 : MF_CHECKED),
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

	case LVM_INSERTCOLUMNA:
	case LVM_INSERTCOLUMNW:
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
		{
			POSITION pos = m_Params.FindIndex(((LPLVITEM)lParam)->iItem);
			if(pos) {
				m_Params.SetAt(pos, MLC_MAGIC);
			if (m_eUpdateMode == lazy)
				PostMessage(LVM_UPDATE, ((LPLVITEM)lParam)->iItem);
			else if (m_eUpdateMode == direct)
				UpdateLocation(((LPLVITEM)lParam)->iItem);
			}
		break;
	}

	case LVN_KEYDOWN:
		break;

	case LVM_SETITEMTEXT:
		//need to check for movement
		*pResult = DefWindowProc(message, wParam, lParam);
		if (*pResult) {
			if (m_eUpdateMode == lazy)
				PostMessage(WM_COMMAND, MLC_IDC_UPDATE, wParam);
			else if (m_eUpdateMode == direct)
				UpdateLocation(wParam);
		}
		return *pResult;

	case LVM_SORTITEMS:
		m_dwParamSort = (LPARAM)wParam;
		UpdateSortHistory(m_dwParamSort, 0);	// SLUGFILLER: multiSort - fail-safe, ensure it's in the sort history(no inverse check)
		m_SortProc = (PFNLVCOMPARE)lParam;
		// SLUGFILLER: multiSort - hook our own callback for automatic layered sorting
		lParam = (LPARAM)MultiSortCallback;
		wParam = (WPARAM)this;
		// SLUGFILLER: multiSort
		for(POSITION pos = m_Params.GetHeadPosition(); pos != NULL; m_Params.GetNext(pos))
			m_Params.SetAt(pos, MLC_MAGIC);
		break;

	case LVM_DELETEALLITEMS:
		if(!CListCtrl::OnWndMsg(message, wParam, lParam, pResult) && DefWindowProc(message, wParam, lParam)) 
			m_Params.RemoveAll();
		return *pResult = TRUE;

	case LVM_DELETEITEM:
		MLC_ASSERT(m_Params.GetAt(m_Params.FindIndex(wParam)) == CListCtrl::GetItemData(wParam));
		if(!CListCtrl::OnWndMsg(message, wParam, lParam, pResult) && DefWindowProc(message, wParam, lParam))
				m_Params.RemoveAt(m_Params.FindIndex(wParam));
		return *pResult = TRUE;

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

	case WM_DESTROY:
        // MORPH START leuk_he temp workarround to prevent a chrash on shutdown.
		// Crash on beta's
		try
		{
		// orginal line:
	     SaveSettings();
		}
		catch(...)
		{
			ASSERT(false);
			//nope should not happen. Just silent... 
		}
		// MORPH ENDT leuk_he temp workarround to prevent a chrash on shutdown.
		break;

	case LVM_UPDATE:
		//better fix for old problem... normally Update(int) causes entire list to redraw
		if (wParam == (UINT)UpdateLocation(wParam)) { //no need to invalidate rect if item moved
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
	else if (nChar == 'V' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		// Ctrl+V: Paste keycombo
		SendMessage(WM_COMMAND, MP_PASTE);
	}
	else if (nChar == 'X' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		// Ctrl+X: Paste keycombo
		SendMessage(WM_COMMAND, MP_CUT);
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

	DrawItem((LPDRAWITEMSTRUCT)lParam);
	return TRUE;
}

void CMuleListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) {
	//MORPH START - Added by SiRoB, Don't draw hidden Rect
	RECT clientRect;
	GetClientRect(&clientRect);
	CRect rcItem(lpDrawItemStruct->rcItem);
	if (rcItem.top >= clientRect.bottom || rcItem.bottom <= clientRect.top)
		return;
	//MORPH END   - Added by SiRoB, Don't draw hidden Rect

	//set up our flicker free drawing
	//MORPH - Moved by SiRoB, Don't draw hidden Rect
	/*
	CRect rcItem(lpDrawItemStruct->rcItem);
	*/
	CDC *oDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	COLORREF crOldDCBkColor = oDC->SetBkColor(m_crWindow);
	CMemDC pDC(oDC, &rcItem);
	CFont *pOldFont = pDC->SelectObject(GetFont());

	int iOffset = 6;
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

	COLORREF crOldTextColor;
	if (m_bCustomDraw) {
		if (bHighlight)
			crOldTextColor = pDC->SetTextColor(g_bLowColorDesktop ? m_crHighlightText : m_lvcd.clrText);
		else
			crOldTextColor = pDC->SetTextColor(m_lvcd.clrText);
	}
	else {
		if (bHighlight)
			crOldTextColor = pDC->SetTextColor(m_crHighlightText);
		else
			crOldTextColor = pDC->SetTextColor(m_crWindowText);
	}

	if (m_crWindowTextBk == CLR_NONE){
		DefWindowProc(WM_ERASEBKGND, (WPARAM)pDC->m_hDC, 0);
	}

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
	if (GetItemCount() == 0) {
		MessageBeep(MB_OK);
		return;
	}

	CListViewSearchDlg dlg;
	dlg.m_pListView = this;
	dlg.m_strFindText = m_strFindText;
	dlg.m_bCanSearchInAllColumns = m_bCanSearchInAllColumns;
	dlg.m_iSearchColumn = m_iFindColumn;
	if (dlg.DoModal() != IDOK || dlg.m_strFindText.IsEmpty())
		return;
	m_strFindText = dlg.m_strFindText;
	m_iFindColumn = dlg.m_iSearchColumn;

	DoFindNext(TRUE/*bShowError*/);
}

void CMuleListCtrl::OnFindNext()
{
	if (GetItemCount() == 0) {
		MessageBeep(MB_OK);
		return;
	}

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
	if (GetItemCount() == 0) {
		MessageBeep(MB_OK);
		return;
	}

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

	if (m_hAccel != NULL)
	{
		if (pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST)
		{
			// If we want to handle the VK_RETURN key, we have to do that via accelerators!
			if (TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
				return TRUE;
		}
	}

	// Catch the "Ctrl+<NumPad_Plus_Key>" shortcut. CMuleListCtrl can not handle this.
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ADD && GetAsyncKeyState(VK_CONTROL)<0) {
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

void CMuleListCtrl::UpdateSortHistory(int dwNewOrder, int dwInverseValue){
	int dwInverse = (dwNewOrder >= dwInverseValue) ? (dwNewOrder-dwInverseValue) : (dwNewOrder+dwInverseValue);	// SLUGFILLER: multiSort - changed to >= for sort #0
	// delete the value (or its inverse sorting value) if it appears already in the list
	POSITION pos1, pos2;
	for (pos1 = m_liSortHistory.GetHeadPosition();( pos2 = pos1 ) != NULL;)
	{
		m_liSortHistory.GetNext(pos1);
		if (m_liSortHistory.GetAt(pos2) == dwNewOrder || m_liSortHistory.GetAt(pos2) == dwInverse)
			m_liSortHistory.RemoveAt(pos2);
	}
	m_liSortHistory.AddHead(dwNewOrder);
	// SLUGFILLER: multiSort remove - do not limit, unlimited saving and loading available
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
//	ASSERT( false );
	return -1;
}

CMuleListCtrl::EUpdateMode CMuleListCtrl::SetUpdateMode(EUpdateMode eUpdateMode)
{
	EUpdateMode eCurUpdateMode = m_eUpdateMode;
	m_eUpdateMode = eUpdateMode;
	return eCurUpdateMode;
}

void CMuleListCtrl::OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	// NOTE: Using 'Info Tips' for owner drawn list view controls (like almost all instances
	// of the CMuleListCtrl) gives potentially *wrong* results. One may and will experience
	// several situations where a tooltip should be shown and none will be shown. This is
	// because the Windows list view control code does not know anything about what the
	// owner drawn list view control was actually drawing. So, the Windows list view control
	// code is just *assuming* that the owner drawn list view control instance is using the
	// same drawing metrics as the Windows control. Because our owner drawn list view controls
	// almost always draw an additional icon before the actual item text and because the
	// Windows control does not know that, the calculations performed by the Windows control
	// regarding folded/unfolded items are in couple of cases wrong. E.g. because the Windows
	// control does not know about the additional icon and thus about the reduced space used
	// for drawing the item text, we may show folded item texts while the Windows control is
	// still assuming that we show the full text -> thus we will not receive a precomputed
	// info tip which contains the unfolded item text. Result: We would have to implement
	// our own info tip processing.
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	if (pGetInfoTip->iSubItem == 0)
	{
		LVHITTESTINFO hti = {0};
		::GetCursorPos(&hti.pt);
		ScreenToClient(&hti.pt);
		if (SubItemHitTest(&hti) == -1 || hti.iItem != pGetInfoTip->iItem || hti.iSubItem != 0)
		{
			// Don't show the default label tip for the main item, if the mouse is not over 
			// the main item.
			if ((pGetInfoTip->dwFlags & LVGIT_UNFOLDED) == 0 && pGetInfoTip->cchTextMax > 0 && pGetInfoTip->pszText[0] != _T('\0'))
			{
				// For any reason this does not work with Win98 (COMCTL32 v5.8). Even when 
				// the info tip text is explicitly set to empty, the list view control may 
				// display the unfolded text for the 1st item. It though works for WinXP.
				pGetInfoTip->pszText[0] = _T('\0');
			}
			return;
		}
	}
	*pResult = 0;
}

//MORPH START - UpdateItemThread
IMPLEMENT_DYNCREATE(CUpdateItemThread, CWinThread)
void CUpdateItemThread::SetListCtrl(CListCtrl* listctrl) {
	m_listctrl = listctrl;
}
void CUpdateItemThread::AddItemToUpdate(LPARAM item) {
	queueditemlocker.Lock();
	queueditem.AddTail(item);
	queueditemlocker.Unlock();
	newitemEvent.SetEvent();
}

void CUpdateItemThread::AddItemUpdated(LPARAM item) {
	updateditemlocker.Lock();
	updateditem.AddTail(item);
	updateditemlocker.Unlock();
}

CUpdateItemThread::CUpdateItemThread() {
	threadEndedEvent = new CEvent(0, 1);
	doRun = true;
}
void CUpdateItemThread::EndThread() {
	doRun = false;
	newitemEvent.SetEvent();
}

CUpdateItemThread::~CUpdateItemThread() {
	// wait for the thread to signal that it has stopped looping.
    threadEndedEvent->Lock();
	delete threadEndedEvent;
}

int CUpdateItemThread::Run() {
	DbgSetThreadName("CUpdateItemThread");
	
	InitThreadLocale();
	
	newitemEvent.Lock();
	while(doRun) {
		queueditemlocker.Lock();
		while (queueditem.GetCount()) {
			LPARAM item = queueditem.RemoveHead();
			queueditemlocker.Unlock();
			update_info_struct* update_info;
			if (ListItems.Lookup(item, update_info)) {
				update_info->bNeedToUpdate = true;
			} else {
				update_info = new update_info_struct;
				update_info->dwUpdate = 0;
				update_info->bNeedToUpdate = true;
				ListItems.SetAt(item, update_info);
			}
			queueditemlocker.Lock();
		}
		queueditemlocker.Unlock();

		updateditemlocker.Lock();
		while (updateditem.GetCount()) {
			LPARAM item = updateditem.RemoveHead();
			updateditemlocker.Unlock();
			update_info_struct* update_info;
			if (!ListItems.Lookup(item, update_info)) {
				update_info = new update_info_struct;
				update_info->bNeedToUpdate = false;
				ListItems.SetAt(item, update_info);
			}
			update_info->dwUpdate = GetTickCount()+MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+(uint32)(rand()/(RAND_MAX/1000));
			updateditemlocker.Lock();
		}
		updateditemlocker.Unlock();
		DWORD wecanwait = (DWORD)-1;
		POSITION pos = ListItems.GetStartPosition();
		LPARAM item;
		update_info_struct* update_info;
		while (pos != NULL)
		{
			ListItems.GetNextAssoc( pos, item, update_info );
			if (update_info->dwUpdate < GetTickCount() && update_info->bNeedToUpdate) {
				if (update_info->dwUpdate + MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE > GetTickCount()) { //check if not too much time occured before to prevent overload
				LVFINDINFO find;
				find.flags = LVFI_PARAM;
				find.lParam = (LPARAM)item;
				int found = m_listctrl->FindItem(&find);   // assert on shutdown? 
				if (found != -1)
					m_listctrl->Update(found);
				update_info->dwUpdate = GetTickCount()+MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+(uint32)(rand()/(RAND_MAX/1000));
				update_info->bNeedToUpdate = false;
				wecanwait = min(wecanwait,1000);
				} else { //we couldn't process it before du to cpu load, so delay the update
					update_info->dwUpdate = GetTickCount()+MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE;
				}
			} else if (update_info->dwUpdate > GetTickCount()) {
				wecanwait = min(wecanwait,update_info->dwUpdate-GetTickCount());
			} else {
				ListItems.RemoveKey(item);
				delete update_info;
			}
		}
		if(doRun) {
			if ((ListItems.GetCount() == 0) || (theApp.m_app_state == APP_STATE_SHUTTINGDOWN))
				newitemEvent.Lock();
			else
				newitemEvent.Lock(wecanwait);
		}
	}

	POSITION pos = ListItems.GetStartPosition();
	LPARAM item;
	update_info_struct* update_info;
	while (pos != NULL)
	{
		ListItems.GetNextAssoc( pos, item, update_info );
		delete update_info;
	}
	ListItems.RemoveAll();
	threadEndedEvent->SetEvent();
	return 0;
}
//MORPH END    - UpdateItemThread

