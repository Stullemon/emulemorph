//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
// HistoryListCtrl. emulEspa�a Mod: Added by MoNKi
//	modified and adapted by Xman-Xtreme Mod
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

// HistoryListCtrl.cpp: archivo de implementaci�n
//

#include "stdafx.h"
#include "emule.h"
#include "Preferences.h"
#include "emuledlg.h"
#include "MemDc.h"
#include "knownfilelist.h"
#include "SharedFileList.h"
#include "menucmds.h"
#include "OtherFunctions.h"
#include "IrcWnd.h"
#include "WebServices.h"
#include "HistoryListCtrl.h"
#include "CommentDialog.h"
#include "FileInfoDialog.h"
#include "MetaDataDlg.h"
#include "ResizableLib/ResizableSheet.h"
#include "ED2kLinkDlg.h"
#include "Log.h"
#include "ListViewWalkerPropertySheet.h"
#include "UserMsgs.h"
#include "SharedFilesWnd.h"
#include "HighColorTab.hpp"
#include "PartFile.h"
#include "TransferWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////////////
// CHistoryFileDetailsSheet
#ifndef NO_HISTORY
class CHistoryFileDetailsSheet : public CListViewWalkerPropertySheet
{
	DECLARE_DYNAMIC(CHistoryFileDetailsSheet)

public:
	CHistoryFileDetailsSheet(CTypedPtrList<CPtrList, CKnownFile*>& aFiles, UINT uPshInvokePage = 0, CListCtrlItemWalk* pListCtrl = NULL);
	virtual ~CHistoryFileDetailsSheet();

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

LPCTSTR CHistoryFileDetailsSheet::m_pPshStartPage;

IMPLEMENT_DYNAMIC(CHistoryFileDetailsSheet, CListViewWalkerPropertySheet)

BEGIN_MESSAGE_MAP(CHistoryFileDetailsSheet, CListViewWalkerPropertySheet)
	ON_WM_DESTROY()
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
END_MESSAGE_MAP()

CHistoryFileDetailsSheet::CHistoryFileDetailsSheet(CTypedPtrList<CPtrList, CKnownFile*>& aFiles, UINT uPshInvokePage, CListCtrlItemWalk* pListCtrl)
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

CHistoryFileDetailsSheet::~CHistoryFileDetailsSheet()
{
}

void CHistoryFileDetailsSheet::OnDestroy()
{
	if (m_uPshInvokePage == 0)
		m_pPshStartPage = GetPage(GetActiveIndex())->m_psp.pszTemplate;
	CListViewWalkerPropertySheet::OnDestroy();
}

BOOL CHistoryFileDetailsSheet::OnInitDialog()
{		
	EnableStackedTabs(FALSE);
	BOOL bResult = CListViewWalkerPropertySheet::OnInitDialog();
	HighColorTab::UpdateImageList(*this);
	InitWindowStyles(this);
	EnableSaveRestore(_T("HistoryFileDetailsSheet")); // call this after(!) OnInitDialog
	UpdateTitle();
	return bResult;
}

LRESULT CHistoryFileDetailsSheet::OnDataChanged(WPARAM, LPARAM)
{
	UpdateTitle();
	return 1;
}

void CHistoryFileDetailsSheet::UpdateTitle()
{
	if (m_aItems.GetSize() == 1)
		SetWindowText(GetResString(IDS_DETAILS) + _T(": ") + STATIC_DOWNCAST(CKnownFile, m_aItems[0])->GetFileName());
	else
		SetWindowText(GetResString(IDS_DETAILS));
}

BOOL CHistoryFileDetailsSheet::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_APPLY_NOW)
		{
		CHistoryListCtrl* pHistoryListCtrl = DYNAMIC_DOWNCAST(CHistoryListCtrl, m_pListCtrl->GetListCtrl());
		if (pHistoryListCtrl)
			{
			for (int i = 0; i < m_aItems.GetSize(); i++) {
				// so, and why does this not(!) work while the sheet is open ??
				pHistoryListCtrl->UpdateFile(DYNAMIC_DOWNCAST(CKnownFile, m_aItems[i]));
			}
		}
	}

	return CListViewWalkerPropertySheet::OnCommand(wParam, lParam);
}

//////////////////////////////
// CHistoryListCtrl

#define MLC_DT_TEXT (DT_SINGLELINE | DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS)

IMPLEMENT_DYNAMIC(CHistoryListCtrl, CListCtrl)
CHistoryListCtrl::CHistoryListCtrl()
	: CListCtrlItemWalk(this)
{
	SetGeneralPurposeFind(true, false);
}

CHistoryListCtrl::~CHistoryListCtrl()
{
	if (m_HistoryOpsMenu) VERIFY( m_HistoryOpsMenu.DestroyMenu() );
	if (m_HistoryMenu)  VERIFY( m_HistoryMenu.DestroyMenu() );
}


BEGIN_MESSAGE_MAP(CHistoryListCtrl, CMuleListCtrl)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()


void CHistoryListCtrl::Init(void)
{
	SetName(_T("HistoryListCtrl"));
	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy,theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	ModifyStyle(LVS_SINGLESEL,0);
	
	InsertColumn(0,GetResString(IDS_DL_FILENAME),LVCFMT_LEFT, 260);
	InsertColumn(1,GetResString(IDS_DL_SIZE),LVCFMT_RIGHT,70);
	InsertColumn(2,GetResString(IDS_TYPE),LVCFMT_LEFT,100);
	InsertColumn(3,GetResString(IDS_FILEID),LVCFMT_LEFT, 220);
	InsertColumn(4,GetResString(IDS_DATE),LVCFMT_LEFT, 120);
	InsertColumn(5,GetResString(IDS_DOWNHISTORY_SHARED),LVCFMT_LEFT, 65);
	InsertColumn(6,GetResString(IDS_COMMENT),LVCFMT_LEFT, 260);
	
	//EastShare START - Added by Pretender
	InsertColumn(7,GetResString(IDS_SF_TRANSFERRED),LVCFMT_RIGHT,120);
	InsertColumn(8,GetResString(IDS_SF_REQUESTS),LVCFMT_RIGHT,100);
	InsertColumn(9,GetResString(IDS_SF_ACCEPTS),LVCFMT_RIGHT,100);
	//EastShare END
	//MORPH START - SLUGFILLER: Spreadbars //Fafner: added to history - 080318
	InsertColumn(10,GetResString(IDS_SF_UPLOADED_PARTS),LVCFMT_LEFT,170,14); // SF
	InsertColumn(11,GetResString(IDS_SF_TURN_PART),LVCFMT_LEFT,100,15); // SF
	InsertColumn(12,GetResString(IDS_SF_TURN_SIMPLE),LVCFMT_LEFT,100,16); // VQB
	InsertColumn(13,GetResString(IDS_SF_FULLUPLOAD),LVCFMT_LEFT,100,17); // SF
	//MORPH END   - SLUGFILLER: Spreadbars

	LoadSettings();

	Reload();

	SetSortArrow();
	SortItems(SortProc, GetSortItem() + (GetSortAscending() ? 0:20));
}

void CHistoryListCtrl::AddFile(CKnownFile* toadd){
	uint32 itemnr = GetItemCount();

	LVFINDINFO info;
	info.flags = LVFI_PARAM;
	info.lParam = (LPARAM)toadd;
	int nItem = FindItem(&info);
	if(nItem == -1){
		if(!theApp.sharedfiles->IsFilePtrInList(toadd) || (theApp.sharedfiles->IsFilePtrInList(toadd) && thePrefs.GetShowSharedInHistory()))
		{
			InsertItem(LVIF_PARAM|LVIF_TEXT,itemnr,toadd->GetFileName(),0,0,0,(LPARAM)toadd);
			if(IsWindowVisible())
			{
				CString str;
				str.Format(_T(" (%i)"),this->GetItemCount());
				theApp.emuledlg->sharedfileswnd->GetDlgItem(IDC_TRAFFIC_TEXT)->SetWindowText(GetResString(IDS_DOWNHISTORY) + str);
			}
		}
	}
}

void CHistoryListCtrl::Reload(void)
{
	CKnownFile * cur_file;

	SetRedraw(false);

	DeleteAllItems();

	//Xman 4.8
	//don't know exactly what happend, but a few users (with old known.met) had a crash
	if(theApp.knownfiles->GetKnownFiles().IsEmpty()==false)
	{
		if(thePrefs.GetShowSharedInHistory()){
			POSITION pos = theApp.knownfiles->GetKnownFiles().GetStartPosition();					
			while(pos){
				CCKey key;
				theApp.knownfiles->GetKnownFiles().GetNextAssoc( pos, key, cur_file );
				InsertItem(LVIF_PARAM|LVIF_TEXT,GetItemCount(),cur_file->GetFileName(),0,0,0,(LPARAM)cur_file);
			}		
		}
		else{
			CKnownFilesMap *files = NULL;
			files=theApp.knownfiles->GetDownloadedFiles();
			POSITION pos = files->GetStartPosition();					
			while(pos){
				CCKey key;
				files->GetNextAssoc( pos, key, cur_file );
				InsertItem(LVIF_PARAM|LVIF_TEXT,GetItemCount(),cur_file->GetFileName(),0,0,0,(LPARAM)cur_file);
			}		
			delete files;
		}
	}
	//Xman end
      
	SetRedraw(true);

	if(IsWindowVisible())
	{
		CString str;
		str.Format(_T(" (%i)"),this->GetItemCount());
		theApp.emuledlg->sharedfileswnd->GetDlgItem(IDC_TRAFFIC_TEXT)->SetWindowText(GetResString(IDS_DOWNHISTORY) + str);
	}
}

void CHistoryListCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) {
	if (!theApp.emuledlg->IsRunning())
		return;
	if( !lpDrawItemStruct->itemData)
		return;

	//MORPH START - Added by SiRoB, Don't draw hidden Rect
	RECT clientRect;
	GetClientRect(&clientRect);
	CRect rcItem(lpDrawItemStruct->rcItem);
	if (rcItem.top >= clientRect.bottom || rcItem.bottom <= clientRect.top)
		return;
	//MORPH END   - Added by SiRoB, Don't draw hidden Rect

	//set up our ficker free drawing
	
	//MORPH - Moved by SiRoB, Don't draw hidden Rect
	/*
	CRect rcItem(lpDrawItemStruct->rcItem);
	*/
	CDC *oDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	COLORREF crOldDCBkColor = oDC->SetBkColor(m_crWindow);
	CMemDC pDC(oDC, &rcItem);	
	CFont *pOldFont = pDC.SelectObject(GetFont());
	COLORREF crOldTextColor;

	//Fafner: possible exception in history - 070626
	//Fafner: note: I got this when replacing known.met (e.g., with backup) and some of
	//Fafner: note: the files are still shared. After return and reload it is fine.
	CString sText;
	try {
		CKnownFile* file = (CKnownFile*)lpDrawItemStruct->itemData;
		
		if(m_bCustomDraw)
			crOldTextColor = pDC.SetTextColor(m_lvcd.clrText);
		else
			crOldTextColor = pDC.SetTextColor(m_crWindowText);

		int iOffset = pDC.GetTextExtent(_T(" "), 1 ).cx*2;
		int iItem = lpDrawItemStruct->itemID;
		CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();

		//gets the item image and state info
		LV_ITEM lvi;
		lvi.mask = LVIF_IMAGE | LVIF_STATE;
		lvi.iItem = iItem;
		lvi.iSubItem = 0;
		lvi.stateMask = LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED;
		GetItem(&lvi);

		//see if the item be highlighted
		BOOL bHighlight = ((lvi.state & LVIS_DROPHILITED) || (lvi.state & LVIS_SELECTED));
		BOOL bCtrlFocused = ((GetFocus() == this) || (GetStyle() & LVS_SHOWSELALWAYS));

		//get rectangles for drawing
		CRect rcBounds, rcLabel, rcIcon;
		GetItemRect(iItem, rcBounds, LVIR_BOUNDS);
		GetItemRect(iItem, rcLabel, LVIR_LABEL);
		GetItemRect(iItem, rcIcon, LVIR_ICON);
		CRect rcCol(rcBounds);

		//the label!
		CString sLabel = GetItemText(iItem, 0);
		sText = sLabel;
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
				pDC.FillRect(rcHighlight, &CBrush(m_crHighlight));
				crOldBckColor = pDC.SetBkColor(m_crHighlight);
			} 
			else 
			{
				pDC.FillRect(rcHighlight, &CBrush(m_crNoHighlight));
				crOldBckColor = pDC.SetBkColor(m_crNoHighlight);
			}
		} 
		else 
		{
			pDC.FillRect(rcHighlight, &CBrush(m_crWindow));
			crOldBckColor = pDC.SetBkColor(GetBkColor());
		}

		//update column
		rcCol.right = rcCol.left + GetColumnWidth(0);

		//draw the item's icon
		int iIconPosY = (rcCol.Height() > theApp.GetSmallSytemIconSize().cy) ? ((rcCol.Height() - theApp.GetSmallSytemIconSize().cy) / 2) : 0;
		int iImage = theApp.GetFileTypeSystemImageIdx( file->GetFileName() );
		if (theApp.GetSystemImageList() != NULL)
			::ImageList_Draw(theApp.GetSystemImageList(), iImage, pDC, rcCol.left + 4, rcCol.top + iIconPosY, ILD_NORMAL|ILD_TRANSPARENT);

		//draw item label (column 0)
		sLabel = file->GetFileName();
		rcLabel.left += 16;
		rcLabel.left += iOffset / 2;
		rcLabel.right -= iOffset;
		pDC.DrawText(sLabel, -1, rcLabel, MLC_DT_TEXT | DT_LEFT | DT_NOCLIP);

		//draw labels for remaining columns
		LV_COLUMN lvc;
		lvc.mask = LVCF_FMT | LVCF_WIDTH;
		rcBounds.right = rcHighlight.right > rcBounds.right ? rcHighlight.right : rcBounds.right;

		int iCount = pHeaderCtrl->GetItemCount();
		for(int iCurrent = 0; iCurrent < iCount; iCurrent++) 
		{
			int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);

			if(iColumn == 0)
				continue;

			GetColumn(iColumn, &lvc);
			//don't draw anything with 0 width
			if(lvc.cx == 0)
				continue;

			rcCol.left = rcCol.right;
			rcCol.right += lvc.cx;

			//EastShare START - Added by Pretender
			CString buffer;
			//EastShare END
			switch(iColumn){
				case 1:
					sLabel = CastItoXBytes(file->GetFileSize());
					break;
				case 2:
					sLabel = file->GetFileTypeDisplayStr();
					break;
				case 3:
					sLabel = EncodeBase16(file->GetFileHash(),16);
					break;
				case 4:
					sLabel = file->GetUtcCFileDate().Format("%x %X");
					break;
				case 5:
					if (theApp.sharedfiles->IsFilePtrInList(file))
						sLabel=GetResString(IDS_YES);
					else
						sLabel=GetResString(IDS_NO);
					break;
				case 6:
					sLabel = file->GetFileComment();
					break;
				
				//EastShare START - Added by Pretender
				case 7:
					sLabel = CastItoXBytes(file->statistic.GetAllTimeTransferred());
					break;
				case 8:
					buffer.Format(_T("%u"), file->statistic.GetAllTimeRequests());
					sLabel = buffer;
					break;
				case 9:
					buffer.Format(_T("%u"), file->statistic.GetAllTimeAccepts());
					sLabel = buffer;
					break;
				//EastShare

				// SLUGFILLER: Spreadbars //Fafner: added to history - 080318
				case 10:
					rcCol.bottom--;
					rcCol.top++;
					file->statistic.DrawSpreadBar(pDC,&rcCol,thePrefs.UseFlatBar());
					rcCol.bottom++;
					rcCol.top--;
					break;
				case 11:
					buffer.Format(_T("%.2f"),file->statistic.GetSpreadSortValue());
					sLabel = buffer;
					break;
				case 12:
					if (file->GetFileSize()>(uint64)0)
						buffer.Format(_T("%.2f"),((double)file->statistic.GetAllTimeTransferred())/((double)file->GetFileSize()));
					else
						buffer.Format(_T("%.2f"),0.0f);
					sLabel = buffer;
					break;
				case 13:
					buffer.Format(_T("%.2f"),file->statistic.GetFullSpreadCount());
					sLabel = buffer;
					break;
				// SLUGFILLER: Spreadbars
			}
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

			if (iColumn != 10) //Fafner: added to history - 080318
				pDC.DrawText(sLabel, -1, rcLabel, MLC_DT_TEXT | nJustify);
		}

		//draw focus rectangle if item has focus
		if((lvi.state & LVIS_FOCUSED) && (bCtrlFocused || (lvi.state & LVIS_SELECTED))) 
		{
			if(!bCtrlFocused || !(lvi.state & LVIS_SELECTED))
				pDC.FrameRect(rcHighlight, &CBrush(m_crNoFocusLine));
			else
				pDC.FrameRect(rcHighlight, &CBrush(m_crFocusLine));
		}

		//Xman Code Improvement
		//not needed
		//pDC.Flush();
		//restore old font
		pDC.SelectObject(pOldFont);
		pDC.SetTextColor(crOldTextColor);
		pDC.SetBkColor(crOldBckColor);
		oDC->SetBkColor(crOldDCBkColor);
		if (!theApp.IsRunningAsService(SVC_LIST_OPT)) // MORPH leuk_he:run as ntservice v1..
			m_updatethread->AddItemUpdated((LPARAM)file); //MORPH - UpdateItemThread
	}
	catch (...) {
		if (!theApp.knownfiles->bReloadHistory)
			LogError(LOG_STATUSBAR, _T("CHistoryListCtrl::DrawItem: exception - %s."), sText); //just once
		theApp.knownfiles->bReloadHistory = true;
	}
}

void CHistoryListCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult){
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	bool sortAscending = (GetSortItem()!= pNMListView->iSubItem) ? true : !GetSortAscending();

	// Sort table
	UpdateSortHistory(pNMListView->iSubItem + (sortAscending ? 0:20), 20);
	SetSortArrow(pNMListView->iSubItem, sortAscending);
	SortItems(SortProc, pNMListView->iSubItem + (sortAscending ? 0:20));

	*pResult = 0;
}

int CHistoryListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){
	/*const*/ CKnownFile* item1 = (CKnownFile*)lParam1;
	/*const*/ CKnownFile* item2 = (CKnownFile*)lParam2;	

	int iResult=0;

	try { //Fafner: possible exception in history - 070626
		switch(lParamSort){
			case 0: //filename asc
				iResult= _tcsicmp(item1->GetFileName(),item2->GetFileName());
				break;
			case 20: //filename desc
				iResult= _tcsicmp(item2->GetFileName(),item1->GetFileName());
				break;
			case 1: //filesize asc
				iResult= item1->GetFileSize()==item2->GetFileSize()?0:(item1->GetFileSize()>item2->GetFileSize()?1:-1);
				break;
			case 21: //filesize desc
				iResult= item1->GetFileSize()==item2->GetFileSize()?0:(item2->GetFileSize()>item1->GetFileSize()?1:-1);
				break;
			case 2: //filetype asc
				iResult= item1->GetFileType().CompareNoCase(item2->GetFileType());
				break;
			case 22: //filetype desc
				iResult= item2->GetFileType().CompareNoCase(item1->GetFileType());
				break;
			case 3: //file ID
				iResult= memcmp(item1->GetFileHash(),item2->GetFileHash(),16);
				break;
			case 23:
				iResult= memcmp(item2->GetFileHash(),item1->GetFileHash(),16);
				break;
			case 4: //date
				iResult= CompareUnsigned(item1->GetUtcFileDate(),item2->GetUtcFileDate());
				break;
			case 24:
				iResult= CompareUnsigned(item2->GetUtcFileDate(),item1->GetUtcFileDate());
				break;
			case 5: //Shared?
				{
					bool shared1, shared2;
					shared1 = theApp.sharedfiles->IsFilePtrInList(item1);
					shared2 = theApp.sharedfiles->IsFilePtrInList(item2);
					iResult= shared1==shared2 ? 0 : (shared1 && !shared2 ? 1 : -1);
				}
				break;
			case 25:
				{
					bool shared1, shared2;
					shared1 = theApp.sharedfiles->IsFilePtrInList(item1);
					shared2 = theApp.sharedfiles->IsFilePtrInList(item2);
					iResult= shared1==shared2 ? 0 : (shared2 && !shared1 ? 1 : -1);
				}
				break;
			case 6: //comment
				iResult= _tcsicmp(item1->GetFileComment(),item2->GetFileComment());
				break;
			case 26:
				iResult= _tcsicmp(item2->GetFileComment(),item1->GetFileComment());
				break;
			//EastShare START - Added by Pretender
			case 7: //all transferred asc
				iResult=item1->statistic.GetAllTimeTransferred()==item2->statistic.GetAllTimeTransferred()?0:(item1->statistic.GetAllTimeTransferred()>item2->statistic.GetAllTimeTransferred()?1:-1);
				break;
			case 27: //all transferred desc
				iResult=item2->statistic.GetAllTimeTransferred()==item1->statistic.GetAllTimeTransferred()?0:(item2->statistic.GetAllTimeTransferred()>item1->statistic.GetAllTimeTransferred()?1:-1);
				break;

			case 8: //acc requests asc
				iResult=item1->statistic.GetAllTimeAccepts() - item2->statistic.GetAllTimeAccepts();
				break;
			case 28: //acc requests desc
				iResult=item2->statistic.GetAllTimeAccepts() - item1->statistic.GetAllTimeAccepts();
				break;
			
			case 9: //acc accepts asc
				iResult=item1->statistic.GetAllTimeAccepts() - item2->statistic.GetAllTimeAccepts();
				break;
			case 29: //acc accepts desc
				iResult=item2->statistic.GetAllTimeAccepts() - item1->statistic.GetAllTimeAccepts();
				break;
			//EastShare END
			//MORPH START - SLUGFILLER: Spreadbars //Fafner: added to history - 080318
			case 10: //spread asc
			case 11:
				iResult=CompareFloat(item1->statistic.GetSpreadSortValue(),item2->statistic.GetSpreadSortValue());
				break;
			case 30: //spread desc
			case 31:
				iResult=CompareFloat(item2->statistic.GetSpreadSortValue(),item1->statistic.GetSpreadSortValue());
				break;

			case 12: // VQB:  Simple UL asc
			case 32: //VQB:  Simple UL desc
				{
					float x1 = ((float)item1->statistic.GetAllTimeTransferred())/((float)item1->GetFileSize());
					float x2 = ((float)item2->statistic.GetAllTimeTransferred())/((float)item2->GetFileSize());
					if (lParamSort == 12) iResult=CompareFloat(x1,x2); else iResult=CompareFloat(x2,x1);
				break;
				}
			case 13: // SF:  Full Upload Count asc
				iResult=CompareFloat(item1->statistic.GetFullSpreadCount(),item2->statistic.GetFullSpreadCount());
				break;
			case 33: // SF:  Full Upload Count desc
				iResult=CompareFloat(item2->statistic.GetFullSpreadCount(),item1->statistic.GetFullSpreadCount());
				break;
			//MORPH END   - SLUGFILLER: Spreadbars
			default: 
				iResult= 0;
		}
	}
	catch (...) {
		if (!theApp.knownfiles->bReloadHistory)
			LogError(LOG_STATUSBAR, _T("CHistoryListCtrl::SortProc: exception.")); //just once
		theApp.knownfiles->bReloadHistory = true;
		iResult = 0;
	}
	// SLUGFILLER: multiSort remove - handled in parent class
	/*
	int dwNextSort;
	//call secondary sortorder, if this one results in equal
	//(Note: yes I know this call is evil OO wise, but better than changing a lot more code, while we have only one instance anyway - might be fixed later)
	if (iResult == 0 && (dwNextSort = theApp.emuledlg->sharedfileswnd->historylistctrl.GetNextSortOrder(lParamSort)) != (-1)){
		iResult= SortProc(lParam1, lParam2, dwNextSort);
	}
	*/
	return iResult;
}

void CHistoryListCtrl::Localize() {
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	for(int i=0; i<=6;i++){
		switch(i){
			case 0:
				strRes = GetResString(IDS_DL_FILENAME);
				break;
			case 1:
				strRes = GetResString(IDS_DL_SIZE);
				break;
			case 2:
				strRes = GetResString(IDS_TYPE);
				break;
			case 3:
				strRes = GetResString(IDS_FILEID);
				break;
			case 4:
				strRes = GetResString(IDS_DATE);
				break;
			case 5:
				strRes = GetResString(IDS_DOWNHISTORY_SHARED);
				break;
			case 6:
				strRes = GetResString(IDS_COMMENT);
				break;

			//EastShare
			case 7:
				strRes = GetResString(IDS_SF_TRANSFERRED);
				break;
			case 8:
				strRes = GetResString(IDS_SF_REQUESTS);
				break;
			case 9:
				strRes = GetResString(IDS_SF_ACCEPTS);
				break;
			//EastShare

			//MORPH START - SLUGFILLER: Spreadbars //Fafner: added to history - 080318
			case 10:
				strRes = GetResString(IDS_SF_UPLOADED_PARTS);
				break;
			case 11:
				strRes = GetResString(IDS_SF_TURN_PART);
				break;
			case 12:
				strRes = GetResString(IDS_SF_TURN_SIMPLE);
				break;
			case 13:
				strRes = GetResString(IDS_SF_FULLUPLOAD);
			//MORPH END - SLUGFILLER: Spreadbars
				break;

			default:
				strRes = "No Text!!";
		}

		hdi.pszText = const_cast<LPTSTR>((LPCTSTR)strRes);
		pHeaderCtrl->SetItem(i, &hdi);
	}

	CreateMenues();
}

void CHistoryListCtrl::CreateMenues()
{
	if (m_HistoryOpsMenu) VERIFY( m_HistoryOpsMenu.DestroyMenu() );
	if (m_HistoryMenu) VERIFY(m_HistoryMenu.DestroyMenu());

	m_HistoryOpsMenu.CreateMenu();
	m_HistoryOpsMenu.AddMenuTitle(NULL, true);
	m_HistoryOpsMenu.AppendMenu(MF_STRING,MP_CLEARHISTORY,GetResString(IDS_DOWNHISTORY_CLEAR), _T("CLEARCOMPLETE"));

	m_HistoryMenu.CreatePopupMenu();
	m_HistoryMenu.AddMenuTitle(GetResString(IDS_DOWNHISTORY), true);
	m_HistoryMenu.AppendMenu(MF_STRING,MP_OPEN, GetResString(IDS_OPENFILE), _T("OPENFILE"));
	
	m_HistoryMenu.AppendMenu(MF_STRING,MP_CMT, GetResString(IDS_CMT_ADD), _T("FILECOMMENTS")); 
	m_HistoryMenu.AppendMenu(MF_STRING,MP_DETAIL, GetResString(IDS_SHOWDETAILS), _T("FILEINFO"));
	m_HistoryMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 

	m_HistoryMenu.AppendMenu(MF_STRING,MP_VIEWSHAREDFILES,GetResString(IDS_DOWNHISTORY_SHOWSHARED));
	m_HistoryMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 

	m_HistoryMenu.AppendMenu(MF_STRING,MP_SHOWED2KLINK, GetResString(IDS_DL_SHOWED2KLINK), _T("ED2KLINK"));
	m_HistoryMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
	
	m_HistoryMenu.AppendMenu(MF_STRING,MP_REMOVESELECTED, GetResString(IDS_DOWNHISTORY_REMOVE), _T("DELETESELECTED"));
	m_HistoryMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 

	m_HistoryMenu.AppendMenu(MF_STRING,Irc_SetSendLink,GetResString(IDS_IRC_ADDLINKTOIRC), _T("IRCCLIPBOARD"));
	m_HistoryMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 

	m_HistoryMenu.AppendMenu(MF_POPUP,(UINT_PTR)m_HistoryOpsMenu.m_hMenu, GetResString(IDS_DOWNHISTORY_ACTIONS));
	m_HistoryMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
}

void CHistoryListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CKnownFile* file = NULL;

	int iSelectedItems = GetSelectedCount();
	if (GetSelectionMark()!=-1) file=(CKnownFile*)GetItemData(GetSelectionMark());

	if(file && theApp.sharedfiles->IsFilePtrInList(file)){
		m_HistoryMenu.EnableMenuItem(MP_OPEN, MF_ENABLED);
		m_HistoryMenu.EnableMenuItem(MP_REMOVESELECTED, MF_GRAYED);
	}
	else {
		m_HistoryMenu.EnableMenuItem(MP_OPEN, MF_GRAYED);
		if(file && GetSelectedCount()>0)
            m_HistoryMenu.EnableMenuItem(MP_REMOVESELECTED, MF_ENABLED);
        else
		    m_HistoryMenu.EnableMenuItem(MP_REMOVESELECTED, MF_GRAYED);
    }

	if(thePrefs.GetShowSharedInHistory())
		m_HistoryMenu.CheckMenuItem(MP_VIEWSHAREDFILES, MF_CHECKED);
	else
		m_HistoryMenu.CheckMenuItem(MP_VIEWSHAREDFILES, MF_UNCHECKED);

	m_HistoryMenu.EnableMenuItem(Irc_SetSendLink, iSelectedItems == 1 && theApp.emuledlg->ircwnd->IsConnected() ? MF_ENABLED : MF_GRAYED);

	CTitleMenu WebMenu;
	WebMenu.CreateMenu();
	WebMenu.AddMenuTitle(NULL, true);
	int iWebMenuEntries = theWebServices.GetFileMenuEntries(&WebMenu);
	UINT flag = (iWebMenuEntries == 0 || iSelectedItems != 1) ? MF_GRAYED : MF_ENABLED;
	m_HistoryMenu.AppendMenu(MF_POPUP | flag, (UINT_PTR)WebMenu.m_hMenu, GetResString(IDS_WEBSERVICES), _T("SEARCHMETHOD_GLOBAL"));

	m_HistoryMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON,point.x,point.y,this);

	m_HistoryMenu.RemoveMenu(m_HistoryMenu.GetMenuItemCount()-1,MF_BYPOSITION);
	VERIFY( WebMenu.DestroyMenu() );
}

void CHistoryListCtrl::OnNMDblclk(NMHDR* /*pNMHDR*/, LRESULT *pResult)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1)
	{
		CKnownFile* file = (CKnownFile*)GetItemData(iSel);
		if (file)
		{
			if (GetKeyState(VK_MENU) & 0x8000)
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

BOOL CHistoryListCtrl::OnCommand(WPARAM wParam, LPARAM /*lParam*/)
{
	UINT selectedCount = this->GetSelectedCount(); 
	int iSel = GetSelectionMark();

	CTypedPtrList<CPtrList, CKnownFile*> selectedList;
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos != NULL){
		int index = GetNextSelectedItem(pos);
		if (index >= 0)
			selectedList.AddTail((CKnownFile*)GetItemData(index));
	}

	if (selectedCount>0){
		CKnownFile* file = (CKnownFile*)GetItemData(iSel);
		if (wParam>=MP_WEBURL && wParam<=MP_WEBURL+256) {
			theWebServices.RunURL(file, wParam);
		}

		switch (wParam){
			case Irc_SetSendLink:
			{
				theApp.emuledlg->ircwnd->SetSendFileString(CreateED2kLink(file));
				break;
			}
			case MP_SHOWED2KLINK:
			{
				ShowFileDialog(selectedList, IDD_ED2KLINK);
				break;
			}
			case MP_OPEN:
				if(theApp.sharedfiles->IsFilePtrInList(file))
					OpenFile(file);
				break; 
			case MP_CMT: 
				ShowFileDialog(selectedList, IDD_COMMENT);
                break; 
			case MPG_ALTENTER:
			case MP_DETAIL:{
				ShowFileDialog(selectedList);
				break;
			}
			case MP_REMOVESELECTED:
				{
					UINT i, uSelectedCount = GetSelectedCount();
					int  nItem = -1;

					if (uSelectedCount > 1)
					{
						if(MessageBox(GetResString(IDS_DOWNHISTORY_REMOVE_QUESTION_MULTIPLE),NULL,MB_YESNO) == IDYES){
							for (i=0;i < uSelectedCount;i++)
							{
								nItem = GetNextItem(nItem, LVNI_SELECTED);
								ASSERT(nItem != -1);
								CKnownFile *item_File = (CKnownFile *)GetItemData(nItem);
								if(item_File && theApp.sharedfiles->GetFileByID(item_File->GetFileHash())==NULL ){
									RemoveFile(item_File);
									nItem--;
								}
							}
						}
					}
					else
					{
						if(file && theApp.sharedfiles->GetFileByID(file->GetFileHash())==NULL){ //Xman 4.2 crashfix
							CString msg;
							msg.Format(GetResString(IDS_DOWNHISTORY_REMOVE_QUESTION),file->GetFileName());
							if(MessageBox(msg,NULL,MB_YESNO) == IDYES)
								RemoveFile(file);
						}
					}
				}
				break;
		}
	}

	switch(wParam){
		case MP_CLEARHISTORY:
			ClearHistory();
			break;
		case MP_VIEWSHAREDFILES:
			thePrefs.SetShowSharedInHistory(!thePrefs.GetShowSharedInHistory());
			Reload();
			break;
	}

	return true;
}

void CHistoryListCtrl::ShowComments(CKnownFile* file) {
	if (file)
	{
		CTypedPtrList<CPtrList, CKnownFile*> aFiles;
		aFiles.AddHead(file);
		ShowFileDialog(aFiles, IDD_COMMENT);
	}
}

void CHistoryListCtrl::OpenFile(CKnownFile* file){
	TCHAR* buffer = new TCHAR[MAX_PATH];
	_sntprintf(buffer,MAX_PATH,_T("%s\\%s"),file->GetPath(),file->GetFileName());
	AddLogLine( false, _T("%s\\%s"),file->GetPath(),file->GetFileName());
	ShellOpenFile(buffer, NULL);
	delete[] buffer;
}

void CHistoryListCtrl::RemoveFile(CKnownFile *toRemove) {
	if(theApp.sharedfiles->IsFilePtrInList(toRemove))
		return;

	if (toRemove->IsKindOf(RUNTIME_CLASS(CPartFile)))
		theApp.emuledlg->transferwnd->downloadlistctrl.ClearCompleted(static_cast<CPartFile*>(toRemove));

	if(theApp.knownfiles->RemoveKnownFile(toRemove)){
		LVFINDINFO info;
		info.flags = LVFI_PARAM;
		info.lParam = (LPARAM)toRemove;
		int nItem = FindItem(&info);
		if(nItem != -1)
		{
			DeleteItem(nItem);
			if(IsWindowVisible())
			{
				CString str;
				str.Format(_T(" (%i)"),this->GetItemCount());
				theApp.emuledlg->sharedfileswnd->GetDlgItem(IDC_TRAFFIC_TEXT)->SetWindowText(GetResString(IDS_DOWNHISTORY) + str);
			}
		}
	}
}
//Xman
//only used for removing duplicated files to avoid a crash
void CHistoryListCtrl::RemoveFileFromView(CKnownFile* toRemove)
{
	LVFINDINFO info;
	info.flags = LVFI_PARAM;
	info.lParam = (LPARAM)toRemove;
	int nItem = FindItem(&info);
	if(nItem != -1)
	{
		DeleteItem(nItem);
	}
	//remark: no need to update the count-info, because we only replace files
}
//Xman end

void CHistoryListCtrl::ClearHistory() {
	if(MessageBox(GetResString(IDS_DOWNHISTORY_CLEAR_QUESTION),GetResString(IDS_DOWNHISTORY),MB_YESNO)==IDYES)
	{
		theApp.knownfiles->ClearHistory();
		Reload();
	}
}


//MORPH START- UpdateItemThread
/*
void CHistoryListCtrl::UpdateFile(const CKnownFile* file)
{
	if (!file || !theApp.emuledlg->IsRunning())
		return;
	int iItem = FindFile(file);
	if (iItem != -1)
	{
		Update(iItem);
		if (GetItemState(iItem, LVIS_SELECTED))
			theApp.emuledlg->sharedfileswnd->ShowSelectedFilesSummary();
	}
}
int CHistoryListCtrl::FindFile(const CKnownFile* pFile)
{
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)pFile;
	return FindItem(&find);
}
*/
void CHistoryListCtrl::UpdateFile(const CKnownFile* file)
{
	if (theApp.IsRunningAsService(SVC_LIST_OPT)) return;// MORPH leuk_he:run as ntservice v1..
	
	if(!file || !theApp.emuledlg->IsRunning())
		return;

	//MORPH START - SiRoB, Don't Refresh item if not needed
	if( theApp.emuledlg->activewnd != theApp.emuledlg->sharedfileswnd || IsWindowVisible() == FALSE )
		return;
	//MORPH END   - SiRoB, Don't Refresh item if not needed

	m_updatethread->AddItemToUpdate((LPARAM)file);
	theApp.emuledlg->sharedfileswnd->ShowSelectedFilesSummary();
}
//MORPH END - UpdateItemThread

void CHistoryListCtrl::ShowFileDialog(CTypedPtrList<CPtrList, CKnownFile*>& aFiles, UINT uPshInvokePage)
{
	if (aFiles.GetSize() > 0)
	{
		CHistoryFileDetailsSheet dialog(aFiles, uPshInvokePage, this);
		dialog.DoModal();
	}
}
#endif