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
#include "SearchDlg.h"
#include "TransferWnd.h"
#include "OtherFunctions.h"
#include "ClientList.h"
#include "UploadQueue.h"
#include "DownloadQueue.h"
#include "emuledlg.h"
#include "MenuCmds.h"
#include "PartFile.h"
#include "CatDialog.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CTransferWnd dialog

IMPLEMENT_DYNAMIC(CTransferWnd, CDialog)
CTransferWnd::CTransferWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CTransferWnd::IDD, pParent)
{
	icon_download = NULL;
}

CTransferWnd::~CTransferWnd()
{
	if (icon_download)
		VERIFY( DestroyIcon(icon_download) );
	// khaos::categorymod+
	VERIFY(m_mnuCatPriority.DestroyMenu());
	VERIFY(m_mnuCatViewFilter.DestroyMenu());
	VERIFY(m_mnuCategory.DestroyMenu());
	// khaos::categorymod-
}

BEGIN_MESSAGE_MAP(CTransferWnd, CResizableDialog)
	ON_NOTIFY(LVN_HOTTRACK, IDC_UPLOADLIST, OnHoverUploadList)
	ON_NOTIFY(LVN_HOTTRACK, IDC_QUEUELIST, OnHoverUploadList)
	ON_NOTIFY(LVN_HOTTRACK, IDC_DOWNLOADLIST, OnHoverDownloadList)
	ON_NOTIFY(LVN_HOTTRACK, IDC_CLIENTLIST , OnHoverUploadList)
	ON_NOTIFY(TCN_SELCHANGE, IDC_DLTAB, OnTcnSelchangeDltab)
	ON_NOTIFY(NM_RCLICK, IDC_DLTAB, OnNMRclickDltab)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_DOWNLOADLIST, OnLvnBegindrag)
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_NOTIFY(LVN_KEYDOWN, IDC_DOWNLOADLIST, OnLvnKeydownDownloadlist)
	ON_NOTIFY(NM_TABMOVED, IDC_DLTAB, OnTabMovement)
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()


BOOL CTransferWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);
	windowtransferstate = 1;

	uploadlistctrl.Init();
	downloadlistctrl.Init();
	queuelistctrl.Init();
	clientlistctrl.Init();
	queuelistctrl.Hide();
	clientlistctrl.Hide();
	uploadlistctrl.Visable();

	SetAllIcons();
    Localize(); // i_a 

	m_uplBtn.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_uplBtn.SetFlat();
	m_uplBtn.SetLeftAlign(true); 
	
	AddAnchor(IDC_DOWNLOADLIST,TOP_LEFT,CSize(100, theApp.glob_prefs->GetSplitterbarPosition() ));
	AddAnchor(IDC_UPLOADLIST,CSize(0,theApp.glob_prefs->GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_QUEUELIST,CSize(0,theApp.glob_prefs->GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_CLIENTLIST,CSize(0,theApp.glob_prefs->GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_UPLOAD_ICO,CSize(0,theApp.glob_prefs->GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_QUEUECOUNT,BOTTOM_LEFT);
	AddAnchor(IDC_TSTATIC1,BOTTOM_LEFT);
	AddAnchor(IDC_QUEUE_REFRESH_BUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_DLTAB,CSize(50,0) ,TOP_RIGHT);

	// splitting functionality
	CRect rc,rcSpl,rcDown;

	GetWindowRect(rc);
	ScreenToClient(rc);

	rcSpl=rc; rcSpl.top=rc.bottom-100 ; rcSpl.bottom=rcSpl.top+5;rcSpl.left=55;
	m_wndSplitter.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER);
	SetInitLayout();
	
	//cats
	rightclickindex=-1;
	// khaos::categorymod+ Create the category right-click menu.
	CreateCategoryMenus();
	// khaos::categorymod-

	downloadlistactive=true;
	m_bIsDragging=false;

// khaos::categorymod+
	// show & cat-tabs
	for (int ix=0; ix < theApp.glob_prefs->GetCatCount(); ix++)
	{
		Category_Struct* curCat = theApp.glob_prefs->GetCategory(ix);
		// MORPH - TO WATCH FOR CATEGORY STATUS
		/*if (ix == 0 && curCat->viewfilters.nFromCats == 2)
			curCat->viewfilters.nFromCats = 0;
		else*/ if (curCat->viewfilters.nFromCats != 2 && ix != 0 && theApp.downloadqueue->GetCategoryFileCount(ix) != 0)
			curCat->viewfilters.nFromCats = 2;

		m_dlTab.InsertItem(ix,theApp.glob_prefs->GetCategory(ix)->title );
	}
	// khaos::categorymod-
	m_tooltip.Create(this, TTS_NOPREFIX);
	m_dlTab.SetToolTips(&m_tooltip);
	UpdateTabToolTips();
	m_tooltip.SendMessage(TTM_SETMAXTIPWIDTH, 0, SHRT_MAX); // recognize \n chars!
	m_tooltip.SetDelayTime(TTDT_AUTOPOP, 20000);
	m_tooltip.SetDelayTime(TTDT_INITIAL, 0);
	m_tooltip.Activate(TRUE);

	UpdateListCount(windowtransferstate);
	VerifyCatTabSize();

	return true;
}

void CTransferWnd::ShowQueueCount(uint32 number){
	char buffer[100];
	sprintf(buffer,"%u (%u "+ GetResString(IDS_BANNED).MakeLower() +")",number,theApp.clientlist->GetBannedCount() );
	this->GetDlgItem(IDC_QUEUECOUNT)->SetWindowText(buffer);
}

void CTransferWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_UPLOADLIST, uploadlistctrl);
	DDX_Control(pDX, IDC_DOWNLOADLIST, downloadlistctrl);
	DDX_Control(pDX, IDC_QUEUELIST, queuelistctrl);
	DDX_Control(pDX, IDC_CLIENTLIST, clientlistctrl);
	DDX_Control(pDX, IDC_UPLOAD_ICO, m_uplBtn);
	DDX_Control(pDX, IDC_DLTAB, m_dlTab);
}

void CTransferWnd::SetInitLayout() {
		CRect rcDown,rcSpl,rcW;
		CWnd* pWnd;

		GetWindowRect(rcW);
		ScreenToClient(rcW);

		LONG splitpos=(theApp.glob_prefs->GetSplitterbarPosition()*rcW.Height())/100;

		pWnd = GetDlgItem(IDC_DOWNLOADLIST);
		pWnd->GetWindowRect(rcDown);
		ScreenToClient(rcDown);
		rcDown.right=rcW.right-7;
		rcDown.bottom=splitpos-5;
		downloadlistctrl.MoveWindow(rcDown);
		
		pWnd = GetDlgItem(IDC_UPLOADLIST);
		pWnd->GetWindowRect(rcDown);
		ScreenToClient(rcDown);
		rcDown.right=rcW.right-7;
		rcDown.bottom=rcW.bottom-20;
		rcDown.top=splitpos+20;
		uploadlistctrl.MoveWindow(rcDown);

		pWnd = GetDlgItem(IDC_QUEUELIST);
		pWnd->GetWindowRect(rcDown);
		ScreenToClient(rcDown);
		rcDown.right=rcW.right-7;
		rcDown.bottom=rcW.bottom-20;
		rcDown.top=splitpos+20;
		queuelistctrl.MoveWindow(rcDown);

		pWnd = GetDlgItem(IDC_CLIENTLIST);
		pWnd->GetWindowRect(rcDown);
		ScreenToClient(rcDown);
		rcDown.right=rcW.right-7;
		rcDown.bottom=rcW.bottom-20;
		rcDown.top=splitpos+20;
		clientlistctrl.MoveWindow(rcDown);

		rcSpl=rcDown;
		rcSpl.top=rcDown.bottom+4;rcSpl.bottom=rcSpl.top+7;rcSpl.left=(rcDown.right/2)-50;rcSpl.right=rcSpl.left+100;
		m_wndSplitter.MoveWindow(rcSpl,true);

		DoResize(0);
}

void CTransferWnd::DoResize(int delta)
{
	CSplitterControl::ChangeHeight(&downloadlistctrl, delta);
	CSplitterControl::ChangeHeight(&uploadlistctrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(&queuelistctrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(&clientlistctrl, -delta, CW_BOTTOMALIGN);

	UpdateSplitterRange();

	Invalidate();
	UpdateWindow();
}

// setting splitter range limits
void CTransferWnd::UpdateSplitterRange()
{
		CRect rcDown,rcUp,rcW,rcSpl;
		CWnd* pWnd;

		GetWindowRect(rcW);
		ScreenToClient(rcW);

		pWnd = GetDlgItem(IDC_DOWNLOADLIST);
		pWnd->GetWindowRect(rcDown);
		ScreenToClient(rcDown);

		pWnd = GetDlgItem(IDC_UPLOADLIST);
		pWnd->GetWindowRect(rcUp);
		ScreenToClient(rcUp);

		pWnd = GetDlgItem(IDC_QUEUELIST);
		pWnd->GetWindowRect(rcUp);
		ScreenToClient(rcUp);

		pWnd = GetDlgItem(IDC_CLIENTLIST);
		pWnd->GetWindowRect(rcUp);
		ScreenToClient(rcUp);

		theApp.glob_prefs->SetSplitterbarPosition((rcDown.bottom*100)/rcW.Height());

		RemoveAnchor(IDC_DOWNLOADLIST);
		RemoveAnchor(IDC_UPLOADLIST);
		RemoveAnchor(IDC_QUEUELIST);
		RemoveAnchor(IDC_CLIENTLIST);
		AddAnchor(IDC_DOWNLOADLIST,TOP_LEFT,CSize(100,theApp.glob_prefs->GetSplitterbarPosition() ));
		AddAnchor(IDC_UPLOADLIST,CSize(0,theApp.glob_prefs->GetSplitterbarPosition()),BOTTOM_RIGHT);
		AddAnchor(IDC_QUEUELIST,CSize(0,theApp.glob_prefs->GetSplitterbarPosition()),BOTTOM_RIGHT);
		AddAnchor(IDC_CLIENTLIST,CSize(0,theApp.glob_prefs->GetSplitterbarPosition()),BOTTOM_RIGHT);

		m_wndSplitter.SetRange(rcDown.top+50 , rcUp.bottom-40);

}


LRESULT CTransferWnd::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message) {
		// arrange transferwindow layout
		case WM_PAINT:
			if (m_wndSplitter) {
				CRect rcDown,rcSpl,rcW;
				CWnd* pWnd;


				GetWindowRect(rcW);
				ScreenToClient(rcW);

				pWnd = GetDlgItem(IDC_DOWNLOADLIST);
				pWnd->GetWindowRect(rcDown);
				ScreenToClient(rcDown);

				if (rcW.Height()>0) {
					// splitter paint update
					rcSpl=rcDown;
					rcSpl.top=rcDown.bottom+8;rcSpl.bottom=rcSpl.top+5;rcSpl.left=190;
					GetDlgItem(IDC_UPLOAD_ICO)->MoveWindow(10,rcSpl.top-4,170,18);
					m_wndSplitter.MoveWindow(rcSpl,true);
					UpdateSplitterRange();
				}
			}
			break;
		case WM_NOTIFY:
			if (wParam == IDC_SPLITTER)
			{	
				SPC_NMHDR* pHdr = (SPC_NMHDR*) lParam;
				DoResize(pHdr->delta);
			}
			break;
		case WM_WINDOWPOSCHANGED : 
			{
				CRect rcW;
				GetWindowRect(rcW);
				ScreenToClient(rcW);

				if (m_wndSplitter && rcW.Height()>0) Invalidate();
				break;
			}
		case WM_SIZE:
			if (m_wndSplitter) {
				CRect rcDown,rcSpl,rcW;
				CWnd* pWnd;

				GetWindowRect(rcW);
				ScreenToClient(rcW);

				if (rcW.Height()>0){
					pWnd = GetDlgItem(IDC_DOWNLOADLIST);
					pWnd->GetWindowRect(rcDown);
					ScreenToClient(rcDown);

					long splitpos=(theApp.glob_prefs->GetSplitterbarPosition()*rcW.Height())/100;

					rcSpl.right=rcDown.right;rcSpl.top=splitpos+10;rcSpl.bottom=rcSpl.top+7;rcSpl.left=(rcDown.right/2)-50;rcSpl.right=rcSpl.left+100;
					m_wndSplitter.MoveWindow(rcSpl,true);
				}

			}
			break;
	}

	return CResizableDialog::DefWindowProc(message, wParam, lParam);
}



// CTransferWnd message handlers
BOOL CTransferWnd::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message== WM_LBUTTONDBLCLK && pMsg->hwnd== GetDlgItem(IDC_DLTAB)->m_hWnd) {
		OnDblclickDltab();
		return TRUE;
	}

	if (pMsg->message==WM_MOUSEMOVE) {
			POINT point;
			::GetCursorPos(&point);
			if (point.x!=m_pLastMousePoint.x || point.y!=m_pLastMousePoint.y) {
				m_pLastMousePoint=point;
				// handle tooltip updating, when mouse is moved from one item to another
				CPoint pt(point);
				m_nDropIndex=GetTabUnderMouse(&pt);
				if (m_nDropIndex!=m_nLastCatTT) {
					m_nLastCatTT=m_nDropIndex;
				    if (m_nDropIndex!=-1)
					    UpdateTabToolTips(m_nDropIndex);
				    m_tooltip.Update();
				}
			}
	}

	if (pMsg->message==WM_MBUTTONUP) {
		if (downloadlistactive) downloadlistctrl.ShowSelectedFileDetails();
		else {
			switch(windowtransferstate){
				case 2:
					queuelistctrl.ShowSelectedUserDetails();
					break;
				case 1:
					uploadlistctrl.ShowSelectedUserDetails();
					break;
				case 0:
					clientlistctrl.ShowSelectedUserDetails();
					break;
			}
		}
		return TRUE;
	}

	return CResizableDialog::PreTranslateMessage(pMsg);
}

int CTransferWnd::GetItemUnderMouse(CListCtrl* ctrl)
{
	CPoint pt;
	::GetCursorPos(&pt);
	ctrl->ScreenToClient(&pt);
	LVHITTESTINFO hit, subhit;
	hit.pt = pt;
	subhit.pt = pt;
	ctrl->SubItemHitTest(&subhit);
	int sel = ctrl->HitTest(&hit);
	if (sel != LB_ERR && (hit.flags & LVHT_ONITEM))
	{
		if (subhit.iSubItem == 0)
			return sel;
	}
	return LB_ERR;
}

void CTransferWnd::UpdateListCount(uint8 listindex) {
	CString buffer;
	if (windowtransferstate!=listindex) return;

	switch (windowtransferstate) {
		case 1: {
					buffer.Format(" (%i)",uploadlistctrl.GetItemCount());
					GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_TW_UPLOADS)+buffer);
				}
				break;
		case 2: {
					buffer.Format(" (%i)",queuelistctrl.GetItemCount());
					GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_ONQUEUE)+buffer);
				}
				break;
		default:{
					buffer.Format(" (%i)",clientlistctrl.GetItemCount());
					GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_CLIENTLIST)+buffer);
				}
	}
}

void CTransferWnd::SwitchUploadList()
{
	if( windowtransferstate == 1){
		windowtransferstate = 2;		
		if( theApp.glob_prefs->IsQueueListDisabled()){
			SwitchUploadList();
			return;
		}
		uploadlistctrl.Hide();
		clientlistctrl.Hide();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_SHOW);
		queuelistctrl.Visable();
		GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_ONQUEUE));
	}
	else if( windowtransferstate == 2){
		windowtransferstate = 0;
		if( theApp.glob_prefs->IsKnownClientListDisabled()){
			SwitchUploadList();
			return;
		}
		uploadlistctrl.Hide();
		queuelistctrl.Hide();
		clientlistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_CLIENTLIST));
	}
	else{
		queuelistctrl.Hide();
		clientlistctrl.Hide();
		uploadlistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		windowtransferstate = 1;
		GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_TW_UPLOADS));
	}
	UpdateListCount(windowtransferstate);
}

void CTransferWnd::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

void CTransferWnd::SetAllIcons()
{
	if (icon_download)
		VERIFY( DestroyIcon(icon_download) );
	icon_download = theApp.LoadIcon("SearchDirectDownload", 16, 16);
	((CStatic*)GetDlgItem(IDC_DOWNLOAD_ICO))->SetIcon(icon_download);
	m_uplBtn.SetIcon("Upload");
}

void CTransferWnd::Localize()
{
	GetDlgItem(IDC_DOWNLOAD_TEXT)->SetWindowText(GetResString(IDS_TW_DOWNLOADS));
	GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_TW_UPLOADS));
	GetDlgItem(IDC_TSTATIC1)->SetWindowText(GetResString(IDS_TW_QUEUE));

	GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->SetWindowText(GetResString(IDS_SV_UPDATE));

	// khaos::categorymod+ Rebuild menus...
	CreateCategoryMenus();
	// khaos::categorymod-

	uploadlistctrl.Localize();
	queuelistctrl.Localize();
	downloadlistctrl.Localize();
	clientlistctrl.Localize();
}

void CTransferWnd::OnBnClickedQueueRefreshButton()
{
	CUpDownClient* update = theApp.uploadqueue->GetNextClient(NULL);

	while( update ){
		theApp.emuledlg->transferwnd->queuelistctrl.RefreshClient( update);
		update = theApp.uploadqueue->GetNextClient(update);
	}
}

void CTransferWnd::OnHoverUploadList(NMHDR *pNMHDR, LRESULT *pResult)
{
	downloadlistactive=false;
	*pResult = 0;
}

void CTransferWnd::OnHoverDownloadList(NMHDR *pNMHDR, LRESULT *pResult)
{
	downloadlistactive=true;
	*pResult = 0;
}

void CTransferWnd::OnTcnSelchangeDltab(NMHDR *pNMHDR, LRESULT *pResult)
{
	downloadlistctrl.ChangeCategory(m_dlTab.GetCurSel());
	*pResult = 0;
}

// Ornis' download categories
void CTransferWnd::OnNMRclickDltab(NMHDR *pNMHDR, LRESULT *pResult)
{
	POINT point;
	::GetCursorPos(&point);

	CPoint pt(point);
	rightclickindex=GetTabUnderMouse(&pt);

	// If the current category is '0'...  Well, we can't very well delete the default category, now can we...
	// Nor can we merge it.
	m_mnuCategory.EnableMenuItem(MP_CAT_REMOVE, rightclickindex == 0 ? MF_GRAYED : MF_ENABLED);
	m_mnuCategory.EnableMenuItem(MP_CAT_MERGE, rightclickindex == 0 ? MF_GRAYED : MF_ENABLED);
	m_mnuCategory.EnableMenuItem(8, (theApp.glob_prefs->AdvancedA4AFMode() ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);

	Category_Struct* curCat = theApp.glob_prefs->GetCategory(rightclickindex);

	// Check and enable the appropriate menu items in Select View Filter
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0, (curCat->viewfilters.nFromCats == 0) ? MF_CHECKED : MF_UNCHECKED);
	//m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+1, (curCat->viewfilters.nFromCats == 1) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+2, (curCat->viewfilters.nFromCats == 2) ? MF_CHECKED : MF_UNCHECKED);

	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+3, (curCat->viewfilters.bComplete) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+4, (curCat->viewfilters.bCompleting) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+5, (curCat->viewfilters.bTransferring) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+6, (curCat->viewfilters.bWaiting) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+7, (curCat->viewfilters.bPaused) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+8, (curCat->viewfilters.bStopped) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+9, (curCat->viewfilters.bHashing) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+10, (curCat->viewfilters.bErrorUnknown) ? MF_CHECKED : MF_UNCHECKED);

	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+11, (curCat->viewfilters.bVideo) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+12, (curCat->viewfilters.bAudio) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+13, (curCat->viewfilters.bArchives) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+14, (curCat->viewfilters.bImages) ? MF_CHECKED : MF_UNCHECKED);
	m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+15, (curCat->viewfilters.bSuspendFilters) ? MF_CHECKED : MF_UNCHECKED);
		
	// Check the appropriate menu item for the A4AF menu...
	for (int i = 0; i < 3; i++)
		m_mnuCatA4AF.CheckMenuItem(MP_CAT_A4AF + i, (curCat->iAdvA4AFMode == i) ? MF_CHECKED : MF_UNCHECKED);

	m_mnuCategory.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	
	*pResult = 0;
	}

void CTransferWnd::CreateCategoryMenus()
{
	if (m_mnuCatPriority) VERIFY( m_mnuCatPriority.DestroyMenu() );
	if (m_mnuCatViewFilter) VERIFY( m_mnuCatViewFilter.DestroyMenu() );
	if (m_mnuCatA4AF) VERIFY( m_mnuCatA4AF.DestroyMenu() );
	if (m_mnuCategory) VERIFY( m_mnuCategory.DestroyMenu() );
	
	// Create sub-menus first...

	// Priority Menu
	m_mnuCatPriority.CreateMenu();
	m_mnuCatPriority.AddMenuTitle(GetResString(IDS_PRIORITY));
	m_mnuCatPriority.AppendMenu(MF_STRING, MP_PRIOLOW, GetResString(IDS_PRIOLOW));
	m_mnuCatPriority.AppendMenu(MF_STRING, MP_PRIONORMAL, GetResString(IDS_PRIONORMAL));
	m_mnuCatPriority.AppendMenu(MF_STRING, MP_PRIOHIGH, GetResString(IDS_PRIOHIGH));
	m_mnuCatPriority.AppendMenu(MF_STRING, MP_PRIOAUTO, GetResString(IDS_PRIOAUTO));

	// A4AF Menu
	m_mnuCatA4AF.CreateMenu();
	m_mnuCatA4AF.AddMenuTitle(GetResString(IDS_CAT_ADVA4AF));
	m_mnuCatA4AF.AppendMenu(MF_STRING, MP_CAT_A4AF, GetResString(IDS_DEFAULT));
	m_mnuCatA4AF.AppendMenu(MF_STRING, MP_CAT_A4AF+1, GetResString(IDS_A4AF_BALANCE));
	m_mnuCatA4AF.AppendMenu(MF_STRING, MP_CAT_A4AF+2, GetResString(IDS_A4AF_STACK));

	// View Filter Menu
	m_mnuCatViewFilter.CreateMenu();
	m_mnuCatViewFilter.AddMenuTitle(GetResString(IDS_CHANGECATVIEW));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0, GetResString(IDS_ALL) );
	//m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+1, GetResString(IDS_ALLOTHERS) );
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+2, GetResString(IDS_CAT_THISCAT) );
		
	m_mnuCatViewFilter.AppendMenu(MF_SEPARATOR);
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+3, GetResString(IDS_COMPLETE));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+4, GetResString(IDS_COMPLETING));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+5, GetResString(IDS_DOWNLOADING));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+6, GetResString(IDS_WAITING));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+7, GetResString(IDS_PAUSED));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+8, GetResString(IDS_STOPPED));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+9, GetResString(IDS_HASHING));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+10, GetResString(IDS_ERRORLIKE));	
	
	m_mnuCatViewFilter.AppendMenu(MF_SEPARATOR);
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+11, GetResString(IDS_VIDEO));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+12, GetResString(IDS_AUDIO));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+13, GetResString(IDS_SEARCH_ARC));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+14, GetResString(IDS_SEARCH_CDIMG));
	
	m_mnuCatViewFilter.AppendMenu(MF_SEPARATOR);
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+15, GetResString(IDS_CAT_SUSPENDFILTERS));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+16, GetResString(IDS_COL_MORECOLORS));

	// Create the main menu...
	m_mnuCategory.CreatePopupMenu();
	m_mnuCategory.AddMenuTitle(GetResString(IDS_CAT));

	m_mnuCategory.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)m_mnuCatViewFilter.m_hMenu, GetResString(IDS_CHANGECATVIEW));
	m_mnuCategory.AppendMenu(MF_SEPARATOR);
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_ADD, GetResString(IDS_CAT_ADD));
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_EDIT, GetResString(IDS_CAT_EDIT));
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_MERGE, GetResString(IDS_CAT_MERGE));
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_REMOVE, GetResString(IDS_CAT_REMOVE));
	m_mnuCategory.AppendMenu(MF_SEPARATOR);
	m_mnuCategory.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)m_mnuCatA4AF.m_hMenu, GetResString(IDS_CAT_ADVA4AF));
	m_mnuCategory.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)m_mnuCatPriority.m_hMenu, GetResString(IDS_PRIORITY));
	m_mnuCategory.AppendMenu(MF_SEPARATOR);
	m_mnuCategory.AppendMenu(MF_STRING, MP_CANCEL, GetResString(IDS_MAIN_BTN_CANCEL));
	m_mnuCategory.AppendMenu(MF_STRING, MP_STOP, GetResString(IDS_DL_STOP));
	m_mnuCategory.AppendMenu(MF_STRING, MP_PAUSE, GetResString(IDS_DL_PAUSE));
	m_mnuCategory.AppendMenu(MF_STRING, MP_RESUME, GetResString(IDS_DL_RESUME));
	m_mnuCategory.AppendMenu(MF_SEPARATOR);
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_STOPLAST, GetResString(IDS_CAT_STOPLAST));	
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_PAUSELAST, GetResString(IDS_CAT_PAUSELAST));	
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_RESUMENEXT, GetResString(IDS_CAT_RESUMENEXT));    	

}

void CTransferWnd::OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult)
{
    int iSel = downloadlistctrl.GetSelectionMark();
	if (iSel==-1) return;
	if (((CtrlItem_Struct*)downloadlistctrl.GetItemData(iSel))->type != FILE_TYPE) return;
	
	m_bIsDragging = true;

	POINT pt;
	::GetCursorPos(&pt);

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	m_nDragIndex = pNMLV->iItem;
	m_pDragImage = downloadlistctrl.CreateDragImage( downloadlistctrl.GetSelectionMark() ,&pt);
    m_pDragImage->BeginDrag( 0, CPoint(0,0) );
    m_pDragImage->DragEnter( GetDesktopWindow(), pNMLV->ptAction );
    SetCapture();
	m_nDropIndex = -1;

	*pResult = 0;
}

void CTransferWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	if( !(nFlags & MK_LBUTTON) ) m_bIsDragging = false;

	if (m_bIsDragging){
		CPoint pt(point);           //get our current mouse coordinates
		ClientToScreen(&pt);        //convert to screen coordinates

		m_nDropIndex=GetTabUnderMouse(&pt);
		m_dlTab.SetCurSel(m_nDropIndex);
		m_dlTab.Invalidate();
		
		::GetCursorPos(&pt);
		pt.y-=10;
		m_pDragImage->DragMove(pt); //move the drag image to those coordinates
	}
}

void CTransferWnd::OnLButtonUp(UINT nFlags, CPoint point)
{

	if (m_bIsDragging)
	{
		ReleaseCapture ();
		m_bIsDragging = false;
		m_pDragImage->DragLeave (GetDesktopWindow ());
		m_pDragImage->EndDrag ();
		delete m_pDragImage;
		

		if (m_nDropIndex > -1)
		{
			Category_Struct* newCat = theApp.glob_prefs->GetCategory(m_nDropIndex);
			
			if (newCat->viewfilters.nFromCats != 1)
			{
			CPartFile* file;
			int index = -1; 
			POSITION pos = downloadlistctrl.GetFirstSelectedItemPosition(); 
			while(pos != NULL) 
			{ 
				index = downloadlistctrl.GetNextSelectedItem(pos); 
				if(index > -1) 
				{
					if (((CtrlItem_Struct*)downloadlistctrl.GetItemData(index))->type == FILE_TYPE) {
						file=(CPartFile*)((CtrlItem_Struct*)downloadlistctrl.GetItemData(index))->value;
						file->SetCategory(m_nDropIndex);
					}

				} 
			}
			m_dlTab.SetCurSel(downloadlistctrl.curTab);
				downloadlistctrl.ChangeCategory(m_dlTab.GetCurSel());
			UpdateCatTabTitles();
			}
			else
				MessageBox(GetResString(IDS_CAT_FROMCATSINFO), GetResString(IDS_CAT_FROMCATSCAP));

		}
		else
			m_dlTab.SetCurSel(downloadlistctrl.curTab);
		downloadlistctrl.Invalidate();
	}
}

BOOL CTransferWnd::OnCommand(WPARAM wParam,LPARAM lParam ){ 

	// khaos::categorymod+
	Category_Struct* curCat;
	curCat = theApp.glob_prefs->GetCategory(rightclickindex);
	/*if (wParam >= MP_CAT_SET0 && wParam <= MP_CAT_SET0 + 20) {
		curCat = theApp.glob_prefs->GetCategory(rightclickindex);
		theApp.glob_prefs->SetAllcatType(wParam-MP_CAT_SET0);
		m_nLastCatTT=-1;
		m_dlTab.SetCurSel(0);
		downloadlistctrl.ChangeCategory(0);
		EditCatTabLabel(0,GetCatTitle( theApp.glob_prefs->GetAllcatType()));*/
	//}
	// khaos::categorymod-

	switch (wParam){ 
		case MP_CAT_ADD: {
			m_nLastCatTT=-1;
			int newindex=AddCategorie("?",theApp.glob_prefs->GetIncomingDir(),"","",false);
			m_dlTab.InsertItem(newindex,theApp.glob_prefs->GetCategory(newindex)->title);
			CCatDialog dialog(newindex);
			dialog.DoModal();
			// khaos::categorymod+ obsolete //theApp.emuledlg->searchwnd.UpdateCatTabs();
			EditCatTabLabel(newindex,theApp.glob_prefs->GetCategory(newindex)->title);
			theApp.glob_prefs->SaveCats();
			break;
		}
		// khaos::categorymod+						 
		case MP_CAT_SET0+16:
		case MP_CAT_EDIT: {
			m_nLastCatTT=-1;
			CCatDialog dialog(rightclickindex);
			dialog.DoModal();

			CString csName;
			csName.Format("%s", theApp.glob_prefs->GetCategory(rightclickindex)->title );
			EditCatTabLabel(rightclickindex,csName);
		
			// khaos::categorymod+ obsolete //theApp.emuledlg->searchwnd.UpdateCatTabs();
			theApp.glob_prefs->SaveCats();
			if (m_dlTab.GetCurSel() == rightclickindex)
				downloadlistctrl.ChangeCategory(rightclickindex);
			break;
		}
		// khaos::categorymod+ Merge Category
		case MP_CAT_MERGE: {
			uint8 useCat;

			CSelCategoryDlg* getCatDlg = new CSelCategoryDlg((CWnd*)theApp.emuledlg);
			int nResult = getCatDlg->DoModal();

			if (nResult == IDCANCEL)
				break;

			useCat = getCatDlg->GetInput();
			delete getCatDlg;

			if (useCat == rightclickindex) break;
			m_nLastCatTT=-1;

			if (useCat > rightclickindex) useCat--;

			theApp.downloadqueue->ResetCatParts(rightclickindex, useCat);
			theApp.glob_prefs->RemoveCat(rightclickindex);
			m_dlTab.DeleteItem(rightclickindex);
			m_dlTab.SetCurSel(useCat);
			downloadlistctrl.ChangeCategory(useCat);
			theApp.glob_prefs->SaveCats();
			break;
		}
		// khaos::categorymod-
		case MP_CAT_REMOVE: {
			m_nLastCatTT=-1;
			theApp.downloadqueue->ResetCatParts(rightclickindex);
			theApp.glob_prefs->RemoveCat(rightclickindex);
			m_dlTab.DeleteItem(rightclickindex);
			m_dlTab.SetCurSel(0);
			downloadlistctrl.ChangeCategory(0);
			theApp.glob_prefs->SaveCats();
			// khaos::categorymod+ obsolete //theApp.emuledlg->searchwnd.UpdateCatTabs();
			break;
		}
		case MP_PRIOLOW: {
			theApp.downloadqueue->SetCatPrio(rightclickindex,PR_LOW);
			break;
		}
		case MP_PRIONORMAL: {
			theApp.downloadqueue->SetCatPrio(rightclickindex,PR_NORMAL);
			break;
		}
		case MP_PRIOHIGH: {
			theApp.downloadqueue->SetCatPrio(rightclickindex,PR_HIGH);
			break;
		}
		case MP_PRIOAUTO: {
			theApp.downloadqueue->SetCatPrio(rightclickindex,PR_AUTO);
			break;
		}
		case MP_PAUSE: {
			theApp.downloadqueue->SetCatStatus(rightclickindex,MP_PAUSE);
			break;
		}
		case MP_STOP : {
				theApp.downloadqueue->SetCatStatus(rightclickindex,MP_STOP);
			break;
		}
		case MP_CANCEL: {
			if (AfxMessageBox(GetResString(IDS_Q_CANCELDL),MB_ICONQUESTION|MB_YESNO) == IDYES)
				theApp.downloadqueue->SetCatStatus(rightclickindex,MP_CANCEL);
			break;
		}
		case MP_RESUME: {
			theApp.downloadqueue->SetCatStatus(rightclickindex,MP_RESUME);
			break;
		}
		// khaos::categorymod+
		/*case MP_RESUMENEXT: {
			theApp.downloadqueue->StartNextFile(rightclickindex);
			break;
		}*/
		case MP_CAT_RESUMENEXT: {
			theApp.downloadqueue->StartNextFile(rightclickindex);
			break;
		}
		case MP_CAT_STOPLAST: {
			theApp.downloadqueue->StopPauseLastFile(MP_STOP, rightclickindex);
			break;
		}
		case MP_CAT_PAUSELAST: {
			theApp.downloadqueue->StopPauseLastFile(MP_PAUSE, rightclickindex);
			break;
		}
		// khaos::categorymod-

		case IDC_UPLOAD_ICO: {
			SwitchUploadList();
			break;
		}
		case IDC_QUEUE_REFRESH_BUTTON: {
			OnBnClickedQueueRefreshButton();
			break;
		}
		// khaos::categorymod+ Handle the new view filter menu.
		case MP_CAT_SET0+1:
			if (rightclickindex != 0 && theApp.downloadqueue->GetCategoryFileCount(rightclickindex))
			{
				MessageBox(GetResString(IDS_CAT_FROMCATSINFO), GetResString(IDS_CAT_FROMCATSCAP));
				curCat->viewfilters.nFromCats = 2;
				EditCatTabLabel(rightclickindex, CString(curCat->title));
				break;
			}
		case MP_CAT_SET0:
		case MP_CAT_SET0+2: {
			curCat->viewfilters.nFromCats = wParam - MP_CAT_SET0;
			break;
		}
		case MP_CAT_SET0+3: {
			curCat->viewfilters.bComplete = curCat->viewfilters.bComplete ? false : true;
			break;
		}
		case MP_CAT_SET0+4: {
			curCat->viewfilters.bCompleting = curCat->viewfilters.bCompleting ? false : true;
			break;
		}
		case MP_CAT_SET0+5: {
			curCat->viewfilters.bTransferring = curCat->viewfilters.bTransferring ? false : true;
			break;
		}
		case MP_CAT_SET0+6: {
			curCat->viewfilters.bWaiting = curCat->viewfilters.bWaiting ? false : true;
			break;
		}
		case MP_CAT_SET0+7: {
			curCat->viewfilters.bPaused = curCat->viewfilters.bPaused ? false : true;
			break;
		}
		case MP_CAT_SET0+8: {
			curCat->viewfilters.bStopped = curCat->viewfilters.bStopped ? false : true;
			break;
		}
		case MP_CAT_SET0+9: {
			curCat->viewfilters.bHashing = curCat->viewfilters.bHashing ? false : true;
			break;
		}
		case MP_CAT_SET0+10: {
			curCat->viewfilters.bErrorUnknown = curCat->viewfilters.bErrorUnknown ? false : true;
			break;
		}
		case MP_CAT_SET0+11: {
			curCat->viewfilters.bVideo = curCat->viewfilters.bVideo ? false : true;
			break;
		}
		case MP_CAT_SET0+12: {
			curCat->viewfilters.bAudio = curCat->viewfilters.bAudio ? false : true;
			break;
		}
		case MP_CAT_SET0+13: {
			curCat->viewfilters.bArchives = curCat->viewfilters.bArchives ? false : true;
			break;
		}
		case MP_CAT_SET0+14: {
			curCat->viewfilters.bImages = curCat->viewfilters.bImages ? false : true;
			break;
		}
		case MP_CAT_SET0+15: {
			curCat->viewfilters.bSuspendFilters = curCat->viewfilters.bSuspendFilters ? false : true;
			break;
		}
	}

	if (wParam >= MP_CAT_SET0 && wParam <= MP_CAT_SET0 + 15)
		downloadlistctrl.ChangeCategory(m_dlTab.GetCurSel());
	if (wParam >= MP_CAT_A4AF && wParam <= MP_CAT_A4AF + 2)
		curCat->iAdvA4AFMode = wParam - MP_CAT_A4AF;
	// khaos::categorymod-
	
	return TRUE;
}

void CTransferWnd::UpdateCatTabTitles() {
	for (uint8 i=0;i<m_dlTab.GetItemCount();i++)
		//MORPH START - Changed by SiRoB, Due to Khaos Category
		//EditCatTabLabel(i,(i==0)? GetCatTitle( theApp.glob_prefs->GetAllcatType() ):theApp.glob_prefs->GetCategory(i)->title);
		EditCatTabLabel(i, theApp.glob_prefs->GetCategory(i)->title);
		//MORPH END   - Changed by SiRoB, Due to Khaos Category
}

void CTransferWnd::EditCatTabLabel(int index,CString newlabel) {

	TCITEM tabitem;
	tabitem.mask = TCIF_PARAM;
	m_dlTab.GetItem(index,&tabitem);
	tabitem.mask = TCIF_TEXT;

	if (theApp.glob_prefs->ShowCatTabInfos()) {
		CPartFile* cur_file;
		uint16 count,dwl;
		count=dwl=0;
		for (int i=0;i<theApp.downloadqueue->GetFileCount();i++) {
			cur_file=theApp.downloadqueue->GetFileByIndex(i);
			if (cur_file==0) continue;
			if (cur_file->CheckShowItemInGivenCat(index)) {
				count++;
				if (cur_file->GetTransferingSrcCount()>0) dwl++;
//				speed+=cur_file->GetDatarate()/1024.0f;
//				size+=cur_file->GetFileSize();
//				trsize+=cur_file->GetCompletedSize();
			}
		}
		CString title=newlabel;
		newlabel.Format("%s (%i/%i)",title,dwl,count);
	}

	tabitem.pszText = newlabel.LockBuffer();
	m_dlTab.SetItem(index,&tabitem);
	newlabel.UnlockBuffer();

	VerifyCatTabSize();
}

int CTransferWnd::AddCategorie(CString newtitle,CString newincoming,CString newcomment,CString newautocat,bool addTab){
	Category_Struct* newcat=new Category_Struct;

	sprintf(newcat->title,newtitle);
	newcat->prio=0;
	// khaos::kmod+ Category Advanced A4AF Mode and Auto Cat
	newcat->iAdvA4AFMode = 0;
	// khaos::kmod-
	sprintf(newcat->incomingpath,newincoming);
	sprintf(newcat->comment,newcomment);
	// khaos::categorymod+ Initialize View Filter Variables
	newcat->viewfilters.bArchives = true;
	newcat->viewfilters.bAudio = true;
	newcat->viewfilters.bComplete = true;
	newcat->viewfilters.bCompleting = true;
	newcat->viewfilters.bErrorUnknown = true;
	newcat->viewfilters.bHashing = true;
	newcat->viewfilters.bImages = true;
	newcat->viewfilters.bPaused = true;
	newcat->viewfilters.bStopped = true;
	newcat->viewfilters.bSuspendFilters = false;
	newcat->viewfilters.bTransferring = true;
	newcat->viewfilters.bVideo = true;
	newcat->viewfilters.bWaiting = true;
	newcat->viewfilters.nAvailSourceCountMax = 0;
	newcat->viewfilters.nAvailSourceCountMin = 0;
	newcat->viewfilters.nFromCats = 2;
	newcat->viewfilters.nFSizeMax = 0;
	newcat->viewfilters.nFSizeMin = 0;
	newcat->viewfilters.nRSizeMax = 0;
	newcat->viewfilters.nRSizeMin = 0;
	newcat->viewfilters.nSourceCountMax = 0;
	newcat->viewfilters.nSourceCountMin = 0;
	newcat->viewfilters.nTimeRemainingMax = 0;
	newcat->viewfilters.nTimeRemainingMin = 0;
	newcat->viewfilters.sAdvancedFilterMask = "";
	newcat->selectioncriteria.bAdvancedFilterMask = true;
	newcat->selectioncriteria.bFileSize = true;
	// khaos::categorymod-
	int index=theApp.glob_prefs->AddCat(newcat);
	
	if (addTab) m_dlTab.InsertItem(index,newtitle);

	VerifyCatTabSize();

	return index;
}

int CTransferWnd::GetTabUnderMouse(CPoint* point) {
		TCHITTESTINFO hitinfo;
		CRect rect;
		m_dlTab.GetWindowRect(&rect);
		point->Offset(0-rect.left,0-rect.top);
		hitinfo.pt = *point;

		if( m_dlTab.GetItemRect( 0, &rect ) )
			if (hitinfo.pt.y< rect.top+30 && hitinfo.pt.y >rect.top-30)
				hitinfo.pt.y = rect.top;

		// Find the destination tab...
		unsigned int nTab = m_dlTab.HitTest( &hitinfo );
		if( hitinfo.flags != TCHT_NOWHERE )
			return nTab;
		else return -1;
}

void CTransferWnd::OnLvnKeydownDownloadlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	if (downloadlistctrl.GetSelectionMark()!=-1) {
		uint8 action=EXPAND_COLLAPSE;
		if (pLVKeyDow->wVKey==VK_ADD || pLVKeyDow->wVKey==VK_RIGHT) action=EXPAND_ONLY;
		else if ( pLVKeyDow->wVKey==VK_SUBTRACT || pLVKeyDow->wVKey==VK_LEFT ) action=COLLAPSE_ONLY;
		if (action<EXPAND_COLLAPSE) downloadlistctrl.ExpandCollapseItem(downloadlistctrl.GetSelectionMark(),action,true);
	}
	*pResult = 0;
}

void CTransferWnd::UpdateTabToolTips(int tab)
{
	uint8 i;

	if (tab==-1) {

		for (i=0;i<m_tooltip.GetToolCount();i++)
			m_tooltip.DelTool(&m_dlTab,i+1);

		for (i = 0; i < m_dlTab.GetItemCount(); i++)
		{
			CRect r;
			m_dlTab.GetItemRect(i, &r);
			VERIFY(m_tooltip.AddTool(&m_dlTab, GetTabStatistic(i), &r, i+1));
		}
	} else {
			CRect r;
			m_dlTab.GetItemRect(tab, &r);

		m_tooltip.DelTool(&m_dlTab,tab+1);
		VERIFY(m_tooltip.AddTool(&m_dlTab, GetTabStatistic(tab), &r, tab+1));
	}
}

CString CTransferWnd::GetTabStatistic(uint8 tab) {
	uint16 count,dwl;
	count=dwl=0;
	float speed=0;
	uint64 size=0;
	uint64 trsize=0;
	uint64 disksize=0;

	CPartFile* cur_file;

	for (int i=0;i<theApp.downloadqueue->GetFileCount();++i) {
		cur_file=theApp.downloadqueue->GetFileByIndex(i);
		if (cur_file==0) continue;
		if (cur_file->CheckShowItemInGivenCat(tab)) {
			count++;
			if (cur_file->GetTransferingSrcCount()>0) ++dwl;
			speed+=cur_file->GetDatarate()/1024.0f;
			size+=cur_file->GetFileSize();
			trsize+=cur_file->GetCompletedSize();
			disksize+=cur_file->GetRealFileSize();
		}
	}

	CString title;
	title.Format("%s: %i/%i\n%s: %.1f %s\n%s: %s/%s\n%s%s",
		GetResString(IDS_DOWNLOADING), dwl,count,GetResString(IDS_DL_SPEED) ,speed,GetResString(IDS_KBYTESEC),
		GetResString(IDS_DL_SIZE),CastItoXBytes(trsize),CastItoXBytes(size),
		GetResString(IDS_ONDISK),CastItoXBytes(disksize)		);
	return title;
}


void CTransferWnd::OnDblclickDltab(){
	POINT point;
	::GetCursorPos(&point);
	CPoint pt(point);
	int tab=GetTabUnderMouse(&pt);
	if (tab<1) return;
	rightclickindex=tab;
	OnCommand(MP_CAT_EDIT,0);
}

void CTransferWnd::OnTabMovement(NMHDR *pNMHDR, LRESULT *pResult) {
	UINT from=m_dlTab.GetLastMovementSource();
	UINT to=m_dlTab.GetLastMovementDestionation();

	if (from==0 || to==0 || from==to-1) return;

	// do the reorder
	
	// rearrange the cat-map
	if (!theApp.glob_prefs->MoveCat(from,to)) return;

	// update partfile-stored assignment
	theApp.downloadqueue->MoveCat((uint8)from,(uint8)to);

	// move category of completed files
	downloadlistctrl.MoveCompletedfilesCat((uint8)from,(uint8)to);

	// of the tabcontrol itself
	m_dlTab.ReorderTab(from,to);

	UpdateCatTabTitles();
	// khaos::categorymod+ obsolete theApp.emuledlg->searchwnd.UpdateCatTabs();

	if (to>from) --to;
	m_dlTab.SetCurSel(to);
	downloadlistctrl.ChangeCategory(to);
}

void CTransferWnd::VerifyCatTabSize() {
	CRect rect;
	int size=0;

	for (int i=0;i<m_dlTab.GetItemCount();i++) {
		m_dlTab.GetItemRect(i,&rect);
		size+= rect.Width();
	}
	size+=20;

			WINDOWPLACEMENT wpTabWinPos;
			m_dlTab.GetWindowPlacement(&wpTabWinPos);
	if (wpTabWinPos.rcNormalPosition.right<0) return;

	int left=wpTabWinPos.rcNormalPosition.right-size;
	if (left<200) left=200;
	wpTabWinPos.rcNormalPosition.left=left;

			RemoveAnchor(m_dlTab);

			m_dlTab.SetWindowPlacement(&wpTabWinPos);
			AddAnchor(m_dlTab,TOP_RIGHT);
}
//MORPH - Removed by SiRoB, Due to Khaos Categorie
/*CString CTransferWnd::GetCatTitle(int catid)
{
	switch (catid) {
		case 0 : return GetResString(IDS_ALL);
		case 1 : return GetResString(IDS_ALLOTHERS);
		case 2 : return GetResString(IDS_STATUS_NOTCOMPLETED);
		case 3 : return GetResString(IDS_DL_TRANSFCOMPL);
		case 4 : return GetResString(IDS_WAITING);
		case 5 : return GetResString(IDS_DOWNLOADING);
		case 6 : return GetResString(IDS_ERRORLIKE);
		case 7 : return GetResString(IDS_PAUSED);
		case 8 : return GetResString(IDS_STOPPED);
		case 10 : return GetResString(IDS_VIDEO);
		case 11 : return GetResString(IDS_AUDIO);
		case 12 : return GetResString(IDS_SEARCH_ARC);
		case 13 : return GetResString(IDS_SEARCH_CDIMG);
	}
	return "?";
}*/
