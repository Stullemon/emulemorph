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
	m_uWnd2 = DFLT_TRANSFER_WND2;
	m_pLastMousePoint.x = -1;
	m_pLastMousePoint.y = -1;
	m_nLastCatTT = -1;
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

	uploadlistctrl.Init();
	downloadlistctrl.Init();
	queuelistctrl.Init();
	clientlistctrl.Init();
	downloadclientsctrl.Init(); //SLAHAM: ADDED DownloadClientsCtrl

	if (thePrefs.GetRestoreLastMainWndDlg())
		m_uWnd2 = thePrefs.GetTransferWnd2();
	//SLAHAM: ADDED DownloadClientsCtrl
	bKl = !thePrefs.m_bDisableKnownClientList;
	bQl = !thePrefs.m_bDisableQueueList;

	m_uWnd2 = 1;
	ShowWnd2(m_uWnd2);

	SetAllIcons();

    Localize(); // i_a 

	m_uplBtn.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_uplBtn.SetFlat();
	m_uplBtn.SetLeftAlign(true); 
	
	//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
	m_btnChangeView.SetIcon(_T("ViewCycle"));	
	m_btnChangeView.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnChangeView.SetLeftAlign(true); 

	m_btnDownUploads.SetIcon(_T("ViewUpDown"));
	m_btnDownUploads.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnDownUploads.SetLeftAlign(true);

	m_btnDownloads.SetIcon(_T("ViewDownload"));
	m_btnDownloads.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnDownloads.SetLeftAlign(true); 

	m_btnUploads.SetIcon(_T("ViewUpload"));
	m_btnUploads.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnUploads.SetLeftAlign(true); 

	m_btnClient.SetIcon(_T("ViewKnownOnly"));
	m_btnClient.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnClient.SetLeftAlign(true); 

	m_btnQueue.SetIcon(_T("ViewQueueOnly"));
	m_btnQueue.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnQueue.SetLeftAlign(true); 

	m_btnTransfers.SetIcon(_T("ViewTransfersOnly"));
	m_btnTransfers.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnTransfers.SetLeftAlign(true); 
	//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons <=

	//SLAHAM: ADDED Switch Lists Icons =>
	m_btnULChangeView.SetIcon(_T("ViewCycle"));
	m_btnULChangeView.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnULChangeView.SetLeftAlign(true);

	m_btnULClients.SetIcon(_T("ViewKnownOnly"));
	m_btnULClients.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnULClients.SetLeftAlign(true);

	m_btnULQueue.SetIcon(_T("ViewQueueOnly"));
	m_btnULQueue.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnULQueue.SetLeftAlign(true);

	m_btnULTransfers.SetIcon(_T("ViewTransfersOnly"));
	m_btnULTransfers.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnULTransfers.SetLeftAlign(true);

	m_btnULUploads.SetIcon(_T("ViewUpload"));
	m_btnULUploads.SetAlign(CButtonST::ST_ALIGN_HORIZ);
	m_btnULUploads.SetLeftAlign(true);
	//SLAHAM: ADDED Switch Lists Icons <=

	AddAnchor(IDC_DOWNLOADLIST,TOP_LEFT,CSize(100, thePrefs.GetSplitterbarPosition() ));
	AddAnchor(IDC_UPLOADLIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_QUEUELIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_CLIENTLIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_DOWNLOADCLIENTS,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT); //SLAHAM: ADDED DownloadClientsCtrl
	AddAnchor(IDC_UPLOAD_ICO,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_QUEUECOUNT,BOTTOM_LEFT);
    AddAnchor(IDC_QUEUE, BOTTOM_LEFT, BOTTOM_RIGHT); //Commander - Added: ClientQueueProgressBar
	AddAnchor(IDC_TSTATIC1,BOTTOM_LEFT);
	AddAnchor(IDC_QUEUE_REFRESH_BUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_DLTAB,CSize(50,0) ,TOP_RIGHT);

	// splitting functionality
	CRect rc,rcSpl,rcDown;

	GetWindowRect(rc);
	ScreenToClient(rc);

	rcSpl=rc; rcSpl.top=rc.bottom-100 ; rcSpl.bottom=rcSpl.top+5;rcSpl.left=330; //SLAHAM: MODIFIED Switch Lists Icons =>
	m_wndSplitter.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER);
	//SLAHAM: ADDED Switch Lists Icons =>
	m_btnULChangeView.ShowWindow(SW_SHOW);
	m_btnULClients.ShowWindow(SW_SHOW);
	m_btnULQueue.ShowWindow(SW_SHOW);
	m_btnULTransfers.ShowWindow(SW_SHOW);
	m_btnULUploads.ShowWindow(SW_SHOW);
	GetDlgItem(IDC_STATIC7)->ShowWindow(SW_SHOW);
	//SLAHAM: ADDED Switch Lists Icons <=

	OnBnClickedDownUploads(); //SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons
	
	//cats
	rightclickindex=-1;
	// khaos::categorymod+ Create the category right-click menu.
	CreateCategoryMenus();
	// khaos::categorymod-

	downloadlistactive=true;
	m_bIsDragging=false;

// khaos::categorymod+
	// show & cat-tabs
	for (int ix=0; ix < thePrefs.GetCatCount(); ix++)
	{
		Category_Struct* curCat = thePrefs.GetCategory(ix);
		// MORPH - TO WATCH FOR CATEGORY STATUS
		/*if (ix == 0 && curCat->viewfilters.nFromCats == 2)
			curCat->viewfilters.nFromCats = 0;
		else*/ if (curCat->viewfilters.nFromCats != 2 && ix != 0 && theApp.downloadqueue->GetCategoryFileCount(ix) != 0)
			curCat->viewfilters.nFromCats = 2;

		m_dlTab.InsertItem(ix,thePrefs.GetCategory(ix)->title );
	}
	// khaos::categorymod-
	m_tooltipCats.Create(this, TTS_NOPREFIX);
	m_dlTab.SetToolTips(&m_tooltipCats);
	UpdateTabToolTips();
	m_tooltipCats.SendMessage(TTM_SETMAXTIPWIDTH, 0, SHRT_MAX); // recognize \n chars!
	m_tooltipCats.SetDelayTime(TTDT_AUTOPOP, 20000);
	m_tooltipCats.SetDelayTime(TTDT_INITIAL, thePrefs.GetToolTipDelay()*1000);
	m_tooltipCats.Activate(TRUE);

	UpdateListCount(m_uWnd2);
	// Mighty Knife: Force category tab verification even if window is not visible
	VerifyCatTabSize(true);
	// [end] Mighty Knife
	//Commander - Added: ClientQueueProgressBar - Start
	bold.CreateFont(10,0,0,1,FW_BOLD,0,0,0,0,3,2,1,34,_T("Small Fonts"));

	queueBar.SetFont(&bold);
	queueBar.SetBkColor(GetSysColor(COLOR_WINDOW));
	queueBar.SetShowPercent();
	queueBar.SetGradientColors(RGB(0,255,0),RGB(255,0,0));
	if(thePrefs.IsInfiniteQueueEnabled() || !thePrefs.ShowClientQueueProgressBar()){
		GetDlgItem(IDC_QUEUE)->ShowWindow(SW_HIDE);
	}
	//Commander - Added: ClientQueueProgressBar - End

	return true;
}

void CTransferWnd::ShowQueueCount(uint32 number){
	TCHAR buffer[100];

	if(thePrefs.IsInfiniteQueueEnabled()){
		#ifdef _UNICODE
			TCHAR symbol[2] = _T("\x221E");
		#else
			TCHAR symbol[4] = "IFQ";
		#endif
		_stprintf(buffer,_T("%u / %s (%u ") + GetResString(IDS_BANNED).MakeLower() + _T(")"),number, symbol,theApp.clientlist->GetBannedCount()); // \x221E -> InfiniteSign
	}
	else {
		_stprintf(buffer,_T("%u / %u (%u ") + GetResString(IDS_BANNED).MakeLower() + _T(")"),number,(thePrefs.GetQueueSize() + max(thePrefs.GetQueueSize()/4, 200)),theApp.clientlist->GetBannedCount()); //Commander - Modified: ClientQueueProgressBar
	}
	
	GetDlgItem(IDC_QUEUECOUNT)->SetWindowText(buffer);

    //Commander - Added: ClientQueueProgressBar - Start
	if(thePrefs.IsInfiniteQueueEnabled() || !thePrefs.ShowClientQueueProgressBar()){
		GetDlgItem(IDC_QUEUE)->ShowWindow(SW_HIDE);
	}
	else{
		GetDlgItem(IDC_QUEUE)->ShowWindow(SW_SHOW);
		queueBar.SetRange32(0, (thePrefs.GetQueueSize() + max(thePrefs.GetQueueSize()/4, 200))); //Softlimit -> GetQueueSize | Hardlimit -> (GetQueueSize + (GetQueueSize/4))
		queueBar.SetPos(number);
	}
    //Commander - Added: ClientQueueProgressBar - End
}

void CTransferWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_UPLOADLIST, uploadlistctrl);
	DDX_Control(pDX, IDC_DOWNLOADLIST, downloadlistctrl);
	DDX_Control(pDX, IDC_DOWNLOADCLIENTS, downloadclientsctrl); //SLAHAM: ADDED DownloadClientsCtrl
	DDX_Control(pDX, IDC_QUEUELIST, queuelistctrl);
	DDX_Control(pDX, IDC_CLIENTLIST, clientlistctrl);
	DDX_Control(pDX, IDC_UPLOAD_ICO, m_uplBtn);
	DDX_Control(pDX, IDC_DLTAB, m_dlTab);
	DDX_Control(pDX, IDC_QUEUE, queueBar); //Commander - Added: ClientQueueProgressBar
	//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
	DDX_Control(pDX, IDC_DL_CHANGEVIEW, m_btnChangeView);
	DDX_Control(pDX, IDC_DL_DOWN_UPLOADS, m_btnDownUploads);
	DDX_Control(pDX, IDC_DL_DOWNLOADS, m_btnDownloads);
	DDX_Control(pDX, IDC_DL_UPLOADS, m_btnUploads);
	DDX_Control(pDX, IDC_DL_QUEUE, m_btnQueue);
	DDX_Control(pDX, IDC_DL_TRANSFERS, m_btnTransfers);
	DDX_Control(pDX, IDC_DL_CLIENT, m_btnClient);
	//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons <=

	//SLAHAM: ADDED Switch Lists Icons =>
	DDX_Control(pDX, IDC_UL_CHANGEVIEW, m_btnULChangeView);
	DDX_Control(pDX, IDC_UL_UPLOADS, m_btnULUploads);
	DDX_Control(pDX, IDC_UL_QUEUE, m_btnULQueue);
	DDX_Control(pDX, IDC_UL_CLIENTS, m_btnULClients);
	DDX_Control(pDX, IDC_UL_TRANSFERS, m_btnULTransfers);
	//SLAHAM: ADDED Switch Lists Icons <=
}

//SLAHAM: REMOVED [TPT] - TBH Transfer Window Buttons
/*void CTransferWnd::SetInitLayout() {
		CRect rcDown,rcSpl,rcW;
		CWnd* pWnd;

		GetWindowRect(rcW);
		ScreenToClient(rcW);

		LONG splitpos=(thePrefs.GetSplitterbarPosition()*rcW.Height())/100;

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
*/
void CTransferWnd::DoResize(int delta)
{
	CSplitterControl::ChangeHeight(&downloadlistctrl, delta);
	CSplitterControl::ChangeHeight(&uploadlistctrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(&queuelistctrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(&clientlistctrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(&downloadclientsctrl, -delta, CW_BOTTOMALIGN); //SLAHAM: ADDED DownloadClientsCtrl

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

	//SLAHAM: ADDED DownloadClientsCtrl =>
	pWnd = GetDlgItem(IDC_DOWNLOADCLIENTS);
	pWnd->GetWindowRect(rcUp);
	ScreenToClient(rcUp);
	//SLAHAM: ADDED DownloadClientsCtrl <=

		// [Comment by Mighty Knife:]
		// Calculate the percentage position of the splitter bar and save it.
		// Make sure to subtract the 5 pixel we added to the bottom position of
		// the downloadlist!
		// Furthermore we add another Height/2 before dividing by Height to perform
		// real rounding instead of just truncating - otherwise we might end up 1% too small!
		thePrefs.SetSplitterbarPosition(((rcDown.bottom-5)*100+rcW.Height()/2)/rcW.Height());

		RemoveAnchor(IDC_DOWNLOADLIST);
		RemoveAnchor(IDC_UPLOADLIST);
		RemoveAnchor(IDC_QUEUELIST);
		RemoveAnchor(IDC_CLIENTLIST);
	RemoveAnchor(IDC_DOWNLOADCLIENTS); //SLAHAM: ADDED DownloadClientsCtrl

		AddAnchor(IDC_DOWNLOADLIST,TOP_LEFT,CSize(100,thePrefs.GetSplitterbarPosition() ));
		AddAnchor(IDC_UPLOADLIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
		AddAnchor(IDC_QUEUELIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
		AddAnchor(IDC_CLIENTLIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_DOWNLOADCLIENTS,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT); //SLAHAM: ADDED DownloadClientsCtrl

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
					rcSpl.top=rcDown.bottom+8;rcSpl.bottom=rcSpl.top+5;rcSpl.left=330; //SLAHAM: MODIFIED Switch Lists Icons
					GetDlgItem(IDC_UPLOAD_ICO)->MoveWindow(10,rcSpl.top-4,170,18);
					//SLAHAM: ADDED Switch Lists Icons =>
					m_btnULChangeView.MoveWindow(211, rcSpl.top-4, 18, 18);
					GetDlgItem(IDC_STATIC7)->MoveWindow(233, rcSpl.top-2, 2, 15);
					m_btnULUploads.MoveWindow(238, rcSpl.top-4, 18, 18);
					m_btnULQueue.MoveWindow(259, rcSpl.top-4, 18, 18);
					m_btnULClients.MoveWindow(280, rcSpl.top-4, 18, 18);
					m_btnULTransfers.MoveWindow(301, rcSpl.top-4, 18, 18);
					//SLAHAM: ADDED Switch Lists Icons <=
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

					long splitpos=(thePrefs.GetSplitterbarPosition()*rcW.Height())/100;

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
			    //m_tooltipCats.Update();
				}
			}
	}

	if (pMsg->message==WM_MBUTTONUP) {
		if (downloadlistactive)
			downloadlistctrl.ShowSelectedFileDetails();
		else {
			switch (m_uWnd2){
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

//SLAHAM: ADDED DownloadClientsCtrl =>
void CTransferWnd::UpdateDownloadClientsCount(int count)
{
	if (showlist == IDC_DOWNLOADCLIENTS)
	{
		CString buffer;
		buffer.Format(_T(" (%i)"), count);
		GetDlgItem(IDC_DOWNLOAD_TEXT)->SetWindowText(GetResString(IDS_TW_TRANSFERS)+buffer);
	}
}
//SLAHAM: ADDED DownloadClientsCtrl <=

void CTransferWnd::UpdateListCount(uint8 listindex, int iCount /*=-1*/)
{
	if (m_uWnd2 != listindex)
		return;
	CString buffer;
	switch (m_uWnd2){
        case 1: {
            uint32 itemCount = iCount == -1 ? uploadlistctrl.GetItemCount() : iCount;
            uint32 activeCount = theApp.uploadqueue->GetActiveUploadsCount();
            if(activeCount >= itemCount) {
                buffer.Format(_T(" (%i)"), itemCount);
            } else {
                buffer.Format(_T(" (%i/%i)"), activeCount, itemCount);
            }
			GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_TW_UPLOADS)+buffer);
			break;
        }
		case 2:
			buffer.Format(_T(" (%i)"), iCount == -1 ? queuelistctrl.GetItemCount() : iCount);
					GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_ONQUEUE)+buffer);
				break;
			//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
		case 3: 
			{ 
			buffer.Format(_T(" (%i)"), iCount == -1 ? clientlistctrl.GetItemCount() : iCount);
					GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_CLIENTLIST)+buffer);
				}
			break;
		default:
			{
				buffer.Format(_T(" (%i)"), iCount == -1 ? downloadclientsctrl.GetItemCount() : iCount);					
				GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_TW_TRANSFERS)+buffer);
			}
			//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons <=
	}
	}

void CTransferWnd::SwitchUploadList()
{
	//SLAHAM: ADDED [TPT] - TBH Transfer Window
	bKl = !thePrefs.m_bDisableKnownClientList;
	bQl = !thePrefs.m_bDisableQueueList;
	//SLAHAM: ADDED [TPT] - TBH Transfer Window
	if( m_uWnd2 == 1){
		SetWnd2(2);
		if( thePrefs.IsQueueListDisabled()){
			SwitchUploadList();
			return;
		}
		uploadlistctrl.Hide();
		clientlistctrl.Hide();
		downloadclientsctrl.Hide(); //SLAHAM: ADDED DownloadClientsCtrl
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_SHOW);
		queuelistctrl.Visable();
		GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_ONQUEUE));
		//SLAHAM: ADDED Switch Lists Icons =>
		GetDlgItem(IDC_UL_CHANGEVIEW)->EnableWindow(true);
		GetDlgItem(IDC_UL_CLIENTS)->EnableWindow(bKl);
		GetDlgItem(IDC_UL_QUEUE)->EnableWindow(false);
		GetDlgItem(IDC_UL_TRANSFERS)->EnableWindow(true);
		GetDlgItem(IDC_UL_UPLOADS)->EnableWindow(true);
		//SLAHAM: ADDED Switch Lists Icons <=
	}
	else if( m_uWnd2 == 2){
		SetWnd2(3); //SLAHAM: ADDED DownloadClientsCtrl
		if( thePrefs.IsKnownClientListDisabled()){
			SwitchUploadList();
			return;
		}
		uploadlistctrl.Hide();
		queuelistctrl.Hide();
		downloadclientsctrl.Hide(); //SLAHAM: ADDED DownloadClientsCtrl
		clientlistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_CLIENTLIST));
		//SLAHAM: ADDED Switch Lists Icons =>
		GetDlgItem(IDC_UL_CHANGEVIEW)->EnableWindow(true);
		GetDlgItem(IDC_UL_CLIENTS)->EnableWindow(false);
		GetDlgItem(IDC_UL_QUEUE)->EnableWindow(bQl);
		GetDlgItem(IDC_UL_TRANSFERS)->EnableWindow(true);
		GetDlgItem(IDC_UL_UPLOADS)->EnableWindow(true);
		//SLAHAM: ADDED Switch Lists Icons <=
	}
	//SLAHAM: ADDED DownloadClientsCtrl =>
	else if( m_uWnd2 == 3){ 
		SetWnd2(0);
		queuelistctrl.Hide();
		clientlistctrl.Hide();
		uploadlistctrl.Hide();
		downloadclientsctrl.Show(); //SLAHAM: ADDED DownloadClientsCtrl
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_TW_TRANSFERS));
		//SLAHAM: ADDED Switch Lists Icons =>
		GetDlgItem(IDC_UL_CHANGEVIEW)->EnableWindow(true);
		GetDlgItem(IDC_UL_CLIENTS)->EnableWindow(bKl);
		GetDlgItem(IDC_UL_QUEUE)->EnableWindow(bQl);
		GetDlgItem(IDC_UL_TRANSFERS)->EnableWindow(false);
		GetDlgItem(IDC_UL_UPLOADS)->EnableWindow(true);
		//SLAHAM: ADDED Switch Lists Icons <=
	}
	//SLAHAM: ADDED DownloadClientsCtrl <=
	else{
		queuelistctrl.Hide();
		clientlistctrl.Hide();
		downloadclientsctrl.Hide(); //SLAHAM: ADDED DownloadClientsCtrl
		uploadlistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		SetWnd2(1);
		GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_TW_UPLOADS));
		//SLAHAM: ADDED Switch Lists Icons =>
		GetDlgItem(IDC_UL_CHANGEVIEW)->EnableWindow(true);
		GetDlgItem(IDC_UL_CLIENTS)->EnableWindow(bKl);
		GetDlgItem(IDC_UL_QUEUE)->EnableWindow(bQl);
		GetDlgItem(IDC_UL_TRANSFERS)->EnableWindow(true);
		GetDlgItem(IDC_UL_UPLOADS)->EnableWindow(false);
		//SLAHAM: ADDED Switch Lists Icons <=
	}
	UpdateListCount(m_uWnd2);
	SetWnd2Icon();
}

void CTransferWnd::ShowWnd2(uint8 uWnd2)
{
	//SLAHAM: ADDED [TPT] - TBH Transfer Window
	bKl = !thePrefs.m_bDisableKnownClientList;
	bQl = !thePrefs.m_bDisableQueueList;
	//SLAHAM: ADDED [TPT] - TBH Transfer Window

	if (uWnd2 == 2 && !thePrefs.IsQueueListDisabled())
	{
		uploadlistctrl.Hide();
		clientlistctrl.Hide();
		downloadclientsctrl.Hide(); //SLAHAM: ADDED DownloadClientsCtrl
		queuelistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_ONQUEUE));
		//SLAHAM: ADDED Switch Lists Icons =>
		GetDlgItem(IDC_UL_CHANGEVIEW)->EnableWindow(true);
		GetDlgItem(IDC_UL_CLIENTS)->EnableWindow(bKl);
		GetDlgItem(IDC_UL_QUEUE)->EnableWindow(false);
		GetDlgItem(IDC_UL_TRANSFERS)->EnableWindow(true);
		GetDlgItem(IDC_UL_UPLOADS)->EnableWindow(true);
		//SLAHAM: ADDED Switch Lists Icons <=
		SetWnd2(uWnd2);
	}
	else if (uWnd2 == 3 && !thePrefs.IsKnownClientListDisabled()) //SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons
	{
		uploadlistctrl.Hide();
		queuelistctrl.Hide();
		downloadclientsctrl.Hide(); //SLAHAM: ADDED DownloadClientsCtrl
		clientlistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_CLIENTLIST));
		//SLAHAM: ADDED Switch Lists Icons =>
		GetDlgItem(IDC_UL_CHANGEVIEW)->EnableWindow(true);
		GetDlgItem(IDC_UL_CLIENTS)->EnableWindow(false);
		GetDlgItem(IDC_UL_QUEUE)->EnableWindow(bQl);
		GetDlgItem(IDC_UL_TRANSFERS)->EnableWindow(true);
		GetDlgItem(IDC_UL_UPLOADS)->EnableWindow(true);
		//SLAHAM: ADDED Switch Lists Icons <=
		SetWnd2(uWnd2);
	}
	//SLAHAM: ADDED DownloadClientsCtrl =>
	else if (uWnd2 == 0) 
	{
		uploadlistctrl.Hide();
		queuelistctrl.Hide();		
		clientlistctrl.Hide();
		downloadclientsctrl.Show();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_TW_TRANSFERS));
		//SLAHAM: ADDED Switch Lists Icons =>
		GetDlgItem(IDC_UL_CHANGEVIEW)->EnableWindow(true);
		GetDlgItem(IDC_UL_CLIENTS)->EnableWindow(bKl);
		GetDlgItem(IDC_UL_QUEUE)->EnableWindow(bQl);
		GetDlgItem(IDC_UL_TRANSFERS)->EnableWindow(false);
		GetDlgItem(IDC_UL_UPLOADS)->EnableWindow(true);
		//SLAHAM: ADDED Switch Lists Icons <=
		SetWnd2(uWnd2);
	}
	//SLAHAM: ADDED DownloadClientsCtrl <=
	else
	{
		queuelistctrl.Hide();
		clientlistctrl.Hide();
		downloadclientsctrl.Hide(); //SLAHAM: ADDED DownloadClientsCtrl
		uploadlistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_UPLOAD_ICO)->SetWindowText(GetResString(IDS_TW_UPLOADS));
		//SLAHAM: ADDED Switch Lists Icons =>
		GetDlgItem(IDC_UL_CHANGEVIEW)->EnableWindow(true);
		GetDlgItem(IDC_UL_CLIENTS)->EnableWindow(bKl);
		GetDlgItem(IDC_UL_QUEUE)->EnableWindow(bQl);
		GetDlgItem(IDC_UL_TRANSFERS)->EnableWindow(true);
		GetDlgItem(IDC_UL_UPLOADS)->EnableWindow(false);
		//SLAHAM: ADDED Switch Lists Icons <=
		SetWnd2(1);
	}

	UpdateListCount(m_uWnd2); //SLAHAM: ADDED Switch Lists Icons
	SetWnd2Icon();
}

void CTransferWnd::SetWnd2(uint8 uWnd2)
{
	m_uWnd2 = uWnd2;
	thePrefs.SetTransferWnd2(m_uWnd2);
}

void CTransferWnd::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

//SLAHAM: MODIFIED Switch Lists Icons =>
void CTransferWnd::SetAllIcons()
{
	if (icon_download)
		VERIFY( DestroyIcon(icon_download) );
	if(showlist == IDC_DOWNLOADLIST)
		icon_download = theApp.LoadIcon(_T("ViewDownload"), 16, 16);
	else if(showlist == IDC_UPLOADLIST)
		icon_download = theApp.LoadIcon(_T("ViewUpload"), 16, 16);
	else if(showlist == IDC_QUEUELIST)
		icon_download = theApp.LoadIcon(_T("ClientsOnQueue"), 16, 16);
	else if(showlist == IDC_DOWNLOADCLIENTS)
		icon_download = theApp.LoadIcon(_T("ViewTransfersOnly"), 16, 16);
	else if(showlist == IDC_CLIENTLIST)
		icon_download = theApp.LoadIcon(_T("ViewKnownOnly"), 16, 16);
	else if(showlist == IDC_DOWNLOADLIST + IDC_UPLOADLIST)
		icon_download = theApp.LoadIcon(_T("ViewDownload"), 16, 16);
	else
	icon_download = theApp.LoadIcon(_T("Download"), 16, 16);
	((CStatic*)GetDlgItem(IDC_DOWNLOAD_ICO))->SetIcon(icon_download);
	SetWnd2Icon();
}

void CTransferWnd::SetWnd2Icon()
{
	//SLAHAM: MODIFIED Switch Lists Icons =>
	if (m_uWnd2 == 2)
		m_uplBtn.SetIcon(_T("ViewQueueOnly"));
	else if (m_uWnd2 == 1)
		m_uplBtn.SetIcon(_T("ViewUpload"));
	else if (m_uWnd2 == 3)
		m_uplBtn.SetIcon(_T("ViewKnownOnly"));
	else
		m_uplBtn.SetIcon(_T("ViewTransfersOnly"));
	//SLAHAM: MODIFIED Switch Lists Icons <=
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
	downloadclientsctrl.Localize(); //SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons

	//UpdateListCount(m_uWnd2);
}

void CTransferWnd::OnBnClickedQueueRefreshButton()
{
	CUpDownClient* update = theApp.uploadqueue->GetNextClient(NULL);

	while( update ){
		theApp.emuledlg->transferwnd->queuelistctrl.RefreshClient( update);
		update = theApp.uploadqueue->GetNextClient(update);
	}
}

//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
void CTransferWnd::OnBnClickedChangeView() {
	
	bKl = thePrefs.m_bDisableKnownClientList;
	bQl = thePrefs.m_bDisableQueueList;

	if(showlist == IDC_DOWNLOADLIST)
		ShowList(IDC_UPLOADLIST);
	else if(showlist == IDC_UPLOADLIST){
		if(bQl && bKl)
			  ShowList(IDC_DOWNLOADCLIENTS);
		else if(bQl && !bKl)
			ShowList(IDC_CLIENTLIST);
		else
			ShowList(IDC_QUEUELIST);
	}
	else if(showlist == IDC_QUEUELIST){
		if(!bKl)
			ShowList(IDC_CLIENTLIST);
		else
			ShowList(IDC_DOWNLOADCLIENTS);
	}
	else if(showlist == IDC_CLIENTLIST)
		ShowList(IDC_DOWNLOADCLIENTS);
	else if(showlist == IDC_DOWNLOADCLIENTS)
		OnBnClickedDownUploads();
	else if(showlist == IDC_UPLOADLIST+IDC_DOWNLOADLIST)
		ShowList(IDC_DOWNLOADLIST);
}
//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons <=

//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
void CTransferWnd::ShowList(uint16 list) {
	CRect rcDown,rcW;
	CWnd* pWnd;
	CString buffer;

	GetWindowRect(rcW);
	ScreenToClient(rcW);
	pWnd = GetDlgItem(list);
	pWnd->GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.bottom=rcW.bottom-20;
	rcDown.top=28;
	m_wndSplitter.DestroyWindow();
	m_btnULChangeView.ShowWindow(SW_HIDE);
	m_btnULClients.ShowWindow(SW_HIDE);
	m_btnULQueue.ShowWindow(SW_HIDE);
	m_btnULTransfers.ShowWindow(SW_HIDE);
	m_btnULUploads.ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC7)->ShowWindow(SW_HIDE);
	RemoveAnchor(list);
	GetDlgItem(IDC_UPLOAD_ICO)->ShowWindow(SW_HIDE);

	showlist = list;

	bKl = !thePrefs.m_bDisableKnownClientList;
	bQl = !thePrefs.m_bDisableQueueList;

	switch(list){
		case IDC_DOWNLOADLIST:
			m_dlTab.ShowWindow(SW_SHOW); //MORPH - Added by SiRoB, Show/Hide dlTab
			downloadlistctrl.MoveWindow(rcDown);
			uploadlistctrl.ShowWindow(SW_HIDE);
			queuelistctrl.ShowWindow(SW_HIDE);
			downloadclientsctrl.ShowWindow(SW_HIDE);
			clientlistctrl.ShowWindow(SW_HIDE);
			downloadlistctrl.ShowWindow(SW_SHOW);
			downloadlistctrl.ShowFilesCount();
			GetDlgItem(IDC_DL_DOWN_UPLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_UPLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_DOWNLOADS)->EnableWindow(false);
			GetDlgItem(IDC_DL_QUEUE)->EnableWindow(bQl);
			GetDlgItem(IDC_DL_TRANSFERS)->EnableWindow(true);
			GetDlgItem(IDC_DL_CLIENT)->EnableWindow(bKl);
			break;
		case IDC_UPLOADLIST:
			m_dlTab.ShowWindow(SW_HIDE); //MORPH - Added by SiRoB, Show/Hide dlTab
			uploadlistctrl.MoveWindow(rcDown);
			downloadlistctrl.ShowWindow(SW_HIDE);
			queuelistctrl.ShowWindow(SW_HIDE);
			clientlistctrl.ShowWindow(SW_HIDE);
			downloadclientsctrl.ShowWindow(SW_HIDE);
			uploadlistctrl.ShowWindow(SW_SHOW);
			buffer.Format(_T(" (%i)"),uploadlistctrl.GetItemCount());
			GetDlgItem(IDC_DOWNLOAD_TEXT)->SetWindowText(GetResString(IDS_TW_UPLOADS)+buffer);
			GetDlgItem(IDC_DL_DOWN_UPLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_UPLOADS)->EnableWindow(false);
			GetDlgItem(IDC_DL_DOWNLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_QUEUE)->EnableWindow(bQl);			
			GetDlgItem(IDC_DL_TRANSFERS)->EnableWindow(true);
			GetDlgItem(IDC_DL_CLIENT)->EnableWindow(bKl);
			break;
		case IDC_QUEUELIST:
			m_dlTab.ShowWindow(SW_HIDE); //MORPH - Added by SiRoB, Show/Hide dlTab
			queuelistctrl.MoveWindow(rcDown);
			uploadlistctrl.ShowWindow(SW_HIDE);
			downloadlistctrl.ShowWindow(SW_HIDE);
			clientlistctrl.ShowWindow(SW_HIDE);
			downloadclientsctrl.ShowWindow(SW_HIDE);
			queuelistctrl.ShowWindow(SW_SHOW);
			buffer.Format(_T(" (%i)"),queuelistctrl.GetItemCount());
			GetDlgItem(IDC_DOWNLOAD_TEXT)->SetWindowText(GetResString(IDS_ONQUEUE)+buffer);
			GetDlgItem(IDC_DL_DOWN_UPLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_UPLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_DOWNLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_QUEUE)->EnableWindow(false);
			GetDlgItem(IDC_DL_TRANSFERS)->EnableWindow(true);
			GetDlgItem(IDC_DL_CLIENT)->EnableWindow(bKl);
			break;
		case IDC_DOWNLOADCLIENTS:
			m_dlTab.ShowWindow(SW_HIDE); //MORPH - Added by SiRoB, Show/Hide dlTab
			downloadclientsctrl.MoveWindow(rcDown);
			uploadlistctrl.ShowWindow(SW_HIDE);
			downloadlistctrl.ShowWindow(SW_HIDE);
			queuelistctrl.ShowWindow(SW_HIDE);
			clientlistctrl.ShowWindow(SW_HIDE);
			downloadclientsctrl.ShowWindow(SW_SHOW);
			buffer.Format(_T(" (%i)"),downloadclientsctrl.GetItemCount());
			GetDlgItem(IDC_DOWNLOAD_TEXT)->SetWindowText(GetResString(IDS_TW_TRANSFERS)+buffer);
			GetDlgItem(IDC_DL_DOWN_UPLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_UPLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_DOWNLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_QUEUE)->EnableWindow(bQl);
			GetDlgItem(IDC_DL_TRANSFERS)->EnableWindow(false);
			GetDlgItem(IDC_DL_CLIENT)->EnableWindow(bKl);
			break;
		case IDC_CLIENTLIST:
			m_dlTab.ShowWindow(SW_HIDE); //MORPH - Added by SiRoB, Show/Hide dlTab
			clientlistctrl.MoveWindow(rcDown);
			uploadlistctrl.ShowWindow(SW_HIDE);
			downloadlistctrl.ShowWindow(SW_HIDE);
			queuelistctrl.ShowWindow(SW_HIDE);
			downloadclientsctrl.ShowWindow(SW_HIDE);
			clientlistctrl.ShowWindow(SW_SHOW);
			buffer.Format(_T(" (%i)"),clientlistctrl.GetItemCount());
			GetDlgItem(IDC_DOWNLOAD_TEXT)->SetWindowText(GetResString(IDS_CLIENTLIST)+buffer);
			GetDlgItem(IDC_DL_DOWN_UPLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_UPLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_DOWNLOADS)->EnableWindow(true);
			GetDlgItem(IDC_DL_QUEUE)->EnableWindow(bQl);
			GetDlgItem(IDC_DL_TRANSFERS)->EnableWindow(true);
			GetDlgItem(IDC_DL_CLIENT)->EnableWindow(false);
			break;
	}
	AddAnchor(list,TOP_LEFT,BOTTOM_RIGHT);
	SetAllIcons(); //SLAHAM: ADDED
}
//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons <=

//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
void CTransferWnd::OnBnClickedDownUploads() 
{
	m_dlTab.ShowWindow(SW_SHOW); //MORPH - Added by SiRoB, Show/Hide dlTab
	if (showlist == IDC_DOWNLOADLIST+IDC_UPLOADLIST)
		return;

	CRect rcDown,rcUp,rcSpl,rcW;
	CWnd* pWnd;

	GetWindowRect(rcW);
	ScreenToClient(rcW);

	// [Comment by Mighty Knife:]
	//
	// thePrefs.GetSplitterbarPosition() retrieves the SplitterBar position.
	// This is a percentage value (0-100%) of height of the whole window.
	// Therefore the "splitpos" variable calculates the vertical pixel-position
	// of the SplitterBar relative to the height of the whole window, i.e.
	// the Transfer-window!

	LONG splitpos=(thePrefs.GetSplitterbarPosition()*rcW.Height())/100;

	pWnd = GetDlgItem(IDC_DOWNLOADLIST);
	pWnd->GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.right=rcW.right-7;
	rcDown.bottom=splitpos+5;
	// [Comment by Mighty Knife:]
	// In UpdateSplitterRange we used this downloadlist here to recalculate the
	// percentage splitter position from the current absolute pixel position
	// on the screen. We added 5 pixel here to the splitpos to get the bottom
	// position of the downloadlist. So in UpdateSplitterRange we have to remember
	// subtracting these 5 pixel again!
	downloadlistctrl.MoveWindow(rcDown);

	pWnd = GetDlgItem(IDC_UPLOADLIST);
	pWnd->GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.right=rcW.right-7;
	rcDown.bottom=rcW.bottom-20;
	rcDown.top=splitpos+30;
	uploadlistctrl.MoveWindow(rcDown);

	pWnd = GetDlgItem(IDC_QUEUELIST);
	pWnd->GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.right=rcW.right-7;
	rcDown.bottom=rcW.bottom-20;
	rcDown.top=splitpos+30;
	queuelistctrl.MoveWindow(rcDown);

	pWnd = GetDlgItem(IDC_CLIENTLIST);
	pWnd->GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.right=rcW.right-7;
	rcDown.bottom=rcW.bottom-20;
	rcDown.top=splitpos+30;
	clientlistctrl.MoveWindow(rcDown);

	pWnd = GetDlgItem(IDC_DOWNLOADCLIENTS);
	pWnd->GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.right=rcW.right-7;
	rcDown.bottom=rcW.bottom-20;
	rcDown.top=splitpos+30;
	downloadclientsctrl.MoveWindow(rcDown);

	rcSpl=rcDown;
	rcSpl.top=rcDown.bottom+4;rcSpl.bottom=rcSpl.top+5;rcSpl.left=110;
	if (!m_wndSplitter)
		m_wndSplitter.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER);
	else
		m_wndSplitter.MoveWindow(rcSpl,true);

	DoResize(0);

	showlist=IDC_DOWNLOADLIST+IDC_UPLOADLIST;

	downloadlistctrl.ShowFilesCount();
	GetDlgItem(IDC_UPLOAD_ICO)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_STATIC7)->ShowWindow(SW_SHOW);
	m_btnULChangeView.ShowWindow(SW_SHOW);
	m_btnULClients.ShowWindow(SW_SHOW);
	m_btnULQueue.ShowWindow(SW_SHOW);
	m_btnULTransfers.ShowWindow(SW_SHOW);
	m_btnULUploads.ShowWindow(SW_SHOW);

	RemoveAnchor(IDC_DOWNLOADLIST);
	RemoveAnchor(IDC_UPLOADLIST);
	RemoveAnchor(IDC_QUEUELIST);
	RemoveAnchor(IDC_DOWNLOADCLIENTS);
	RemoveAnchor(IDC_CLIENTLIST);
	RemoveAnchor(IDC_UPLOAD_ICO);

	AddAnchor(IDC_DOWNLOADLIST,TOP_LEFT,CSize(100, thePrefs.GetSplitterbarPosition() ));
	AddAnchor(IDC_UPLOADLIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_QUEUELIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_CLIENTLIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_DOWNLOADCLIENTS,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_UPLOAD_ICO,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);

	downloadlistctrl.ShowWindow(SW_SHOW);

	switch (m_uWnd2)
	{
	case 0:
		{
			uploadlistctrl.ShowWindow(SW_HIDE);
			queuelistctrl.ShowWindow(SW_HIDE);
			clientlistctrl.ShowWindow(SW_HIDE);
			GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
			downloadclientsctrl.ShowWindow(SW_SHOW);
			break;
		}
	case 1:
		{
			queuelistctrl.ShowWindow(SW_HIDE);
			clientlistctrl.ShowWindow(SW_HIDE);
			downloadclientsctrl.ShowWindow(SW_HIDE);
			GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
			uploadlistctrl.ShowWindow(SW_SHOW);
			break;
		}
	case 2:
		{
			uploadlistctrl.ShowWindow(SW_HIDE);
			clientlistctrl.ShowWindow(SW_HIDE);
			downloadclientsctrl.ShowWindow(SW_HIDE);
			GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_SHOW);
			queuelistctrl.ShowWindow(SW_SHOW);
			break;
		}	
	case 3:
		{
			uploadlistctrl.ShowWindow(SW_HIDE);
			queuelistctrl.ShowWindow(SW_HIDE);
			downloadclientsctrl.ShowWindow(SW_HIDE);
			GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
			clientlistctrl.ShowWindow(SW_SHOW);
			break;
		}	
	default:
		{
			queuelistctrl.ShowWindow(SW_HIDE);
			clientlistctrl.ShowWindow(SW_HIDE);
			downloadclientsctrl.ShowWindow(SW_HIDE);
			GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
			uploadlistctrl.ShowWindow(SW_SHOW);
			m_uWnd2 = 1;
		}
	}

	bKl = !thePrefs.m_bDisableKnownClientList;
	bQl = !thePrefs.m_bDisableQueueList;

	GetDlgItem(IDC_DL_DOWN_UPLOADS)->EnableWindow(false);
	GetDlgItem(IDC_DL_UPLOADS)->EnableWindow(true);
	GetDlgItem(IDC_DL_DOWNLOADS)->EnableWindow(true);
	GetDlgItem(IDC_DL_QUEUE)->EnableWindow(bQl);
	GetDlgItem(IDC_DL_TRANSFERS)->EnableWindow(true);
	GetDlgItem(IDC_DL_CLIENT)->EnableWindow(bKl);

	UpdateListCount(m_uWnd2);
	SetAllIcons(); //SLAHAM: ADDED
}
//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons <=

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
	//MORPH - Removed by SiRoB, Khaos Category
	/*
	// Menu for category
	CTitleMenu menu;
	*/
	POINT point;
	::GetCursorPos(&point);

	CPoint pt(point);
	rightclickindex=GetTabUnderMouse(&pt);

	// If the current category is '0'...  Well, we can't very well delete the default category, now can we...
	// Nor can we merge it.
	m_mnuCategory.EnableMenuItem(MP_CAT_REMOVE, rightclickindex == 0 ? MF_GRAYED : MF_ENABLED);
	m_mnuCategory.EnableMenuItem(MP_CAT_MERGE, rightclickindex == 0 ? MF_GRAYED : MF_ENABLED);
	m_mnuCategory.EnableMenuItem(8, (thePrefs.AdvancedA4AFMode() ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION);

	Category_Struct* curCat = thePrefs.GetCategory(rightclickindex);
	if (curCat) { //MORPH - HOTFIX by SiRoB, Possible crash when NULL is returned by GetCategory()
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
		
		m_mnuCatViewFilter.CheckMenuItem(MP_CAT_SET0+16, (curCat->viewfilters.bSeenComplet) ? MF_CHECKED : MF_UNCHECKED); //MORPH - Added by SiRoB, Seen Complet filter

		// Check the appropriate menu item for the Prio menu...
	    m_mnuCatPriority.CheckMenuRadioItem(MP_PRIOLOW, MP_PRIOHIGH, MP_PRIOLOW+curCat->prio,0);
		// Check the appropriate menu item for the A4AF menu...
		m_mnuCatA4AF.CheckMenuRadioItem(MP_CAT_A4AF, MP_CAT_A4AF+2, MP_CAT_A4AF+curCat->iAdvA4AFMode,0);
	    m_mnuCatA4AF.CheckMenuItem(MP_DOWNLOAD_ALPHABETICAL, curCat->downloadInAlphabeticalOrder ? MF_CHECKED : MF_UNCHECKED);
		m_mnuCategory.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	}
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

	// A4AF Menu
	m_mnuCatA4AF.CreateMenu();
	m_mnuCatA4AF.AddMenuTitle(GetResString(IDS_CAT_ADVA4AF));
	m_mnuCatA4AF.AppendMenu(MF_STRING, MP_CAT_A4AF, GetResString(IDS_DEFAULT));
	m_mnuCatA4AF.AppendMenu(MF_STRING, MP_CAT_A4AF+1, GetResString(IDS_A4AF_BALANCE));
	m_mnuCatA4AF.AppendMenu(MF_STRING, MP_CAT_A4AF+2, GetResString(IDS_A4AF_STACK));
    m_mnuCatA4AF.AppendMenu(MF_STRING,MP_DOWNLOAD_ALPHABETICAL, GetResString(IDS_DOWNLOAD_ALPHABETICAL));

	// View Filter Menu
	m_mnuCatViewFilter.CreateMenu();
	m_mnuCatViewFilter.AddMenuTitle(GetResString(IDS_CHANGECATVIEW),_T("SEARCHPARAMS"));
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
	
	//MORPH START - Added by SiRoB, Seen Complet filter
	m_mnuCatViewFilter.AppendMenu(MF_SEPARATOR);
	CString strtemp = GetResString(IDS_LASTSEENCOMPL);
	strtemp.Remove(':');
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+16, strtemp);	
	//MORPH END - Added by SiRoB, Seen Complet filter

	m_mnuCatViewFilter.AppendMenu(MF_SEPARATOR);
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+11, GetResString(IDS_VIDEO));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+12, GetResString(IDS_AUDIO));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+13, GetResString(IDS_SEARCH_ARC));
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+14, GetResString(IDS_SEARCH_CDIMG));
	
	m_mnuCatViewFilter.AppendMenu(MF_SEPARATOR);
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+15, GetResString(IDS_CAT_SUSPENDFILTERS));
	//MORPH - Changed by SiRoB, Seen Complet filter
	/*
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+16, GetResString(IDS_COL_MORECOLORS));
	*/
	m_mnuCatViewFilter.AppendMenu(MF_STRING, MP_CAT_SET0+17, GetResString(IDS_COL_MORECOLORS));

	// Create the main menu...
	m_mnuCategory.CreatePopupMenu();
	m_mnuCategory.AddMenuTitle(GetResString(IDS_CAT),true);

	m_mnuCategory.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)m_mnuCatViewFilter.m_hMenu, GetResString(IDS_CHANGECATVIEW),_T("SEARCHPARAMS"));
	m_mnuCategory.AppendMenu(MF_SEPARATOR);
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_ADD, GetResString(IDS_CAT_ADD),_T("CATADD"));
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_EDIT, GetResString(IDS_CAT_EDIT),_T("CATEDIT"));
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_MERGE, GetResString(IDS_CAT_MERGE),_T("CATMERGE"));
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_REMOVE, GetResString(IDS_CAT_REMOVE),_T("CATREMOVE"));
	m_mnuCategory.AppendMenu(MF_SEPARATOR);
	m_mnuCategory.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)m_mnuCatA4AF.m_hMenu, GetResString(IDS_CAT_ADVA4AF),_T("ADVA4AF"));
	m_mnuCategory.AppendMenu(MF_SEPARATOR);
	m_mnuCategory.AppendMenu(MF_STRING,MP_HM_OPENINC, GetResString(IDS_OPENINC), _T("FOLDERS") );
	m_mnuCategory.AppendMenu(MF_SEPARATOR);
	m_mnuCategory.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)m_mnuCatPriority.m_hMenu, GetResString(IDS_PRIORITY), _T("FILEPRIORITY"));
	m_mnuCategory.AppendMenu(MF_STRING, MP_CANCEL, GetResString(IDS_MAIN_BTN_CANCEL), _T("DELETE"));
	m_mnuCategory.AppendMenu(MF_STRING, MP_STOP, GetResString(IDS_DL_STOP), _T("STOP"));
	m_mnuCategory.AppendMenu(MF_STRING, MP_PAUSE, GetResString(IDS_DL_PAUSE), _T("PAUSE"));
	m_mnuCategory.AppendMenu(MF_STRING, MP_RESUME, GetResString(IDS_DL_RESUME), _T("RESUME"));
	m_mnuCategory.AppendMenu(MF_SEPARATOR);
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_STOPLAST, GetResString(IDS_CAT_STOPLAST), _T("CATSTOPLAST"));	
	m_mnuCategory.AppendMenu(MF_STRING, MP_CAT_PAUSELAST, GetResString(IDS_CAT_PAUSELAST), _T("CATPAUSELAST"));	
	m_mnuCategory.AppendMenu(MF_STRING, MP_RESUMENEXT, GetResString(IDS_CAT_RESUMENEXT), _T("CATRESUMENEXT"));    	

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
		
		if (m_nDropIndex>-1 && (downloadlistctrl.curTab==0 ||
				(downloadlistctrl.curTab>0 && m_nDropIndex!=downloadlistctrl.curTab) )) {

			CPartFile* file;
			
			// for multiselections
			CTypedPtrList <CPtrList,CPartFile*> selectedList; 
			POSITION pos = downloadlistctrl.GetFirstSelectedItemPosition(); 
			while(pos != NULL) 
			{ 
				int index = downloadlistctrl.GetNextSelectedItem(pos);
				if(index > -1 && (((CtrlItem_Struct*)downloadlistctrl.GetItemData(index))->type == FILE_TYPE))
					selectedList.AddTail( (CPartFile*)((CtrlItem_Struct*)downloadlistctrl.GetItemData(index))->value );
			}

			while (!selectedList.IsEmpty())
				{
				file = selectedList.GetHead();
				selectedList.RemoveHead();
						file->SetCategory(m_nDropIndex);
					}


			m_dlTab.SetCurSel(downloadlistctrl.curTab);
			//if (m_dlTab.GetCurSel()>0 || (thePrefs.GetAllcatType()==1 && m_dlTab.GetCurSel()==0) )
			downloadlistctrl.ChangeCategory(m_dlTab.GetCurSel());

			UpdateCatTabTitles();

		} else m_dlTab.SetCurSel(downloadlistctrl.curTab);
		downloadlistctrl.Invalidate();
	}
}

BOOL CTransferWnd::OnCommand(WPARAM wParam,LPARAM lParam ){ 

	// khaos::categorymod+
	Category_Struct* curCat;
	curCat = thePrefs.GetCategory(rightclickindex);
	/*
	if (wParam>=MP_CAT_SET0 && wParam<=MP_CAT_SET0+20) {
		if (wParam==MP_CAT_SET0+20) {
			// negate
			thePrefs.SetCatFilterNeg(m_isetcatmenu, (!thePrefs.GetCatFilterNeg(m_isetcatmenu)) );


		} else {
			// dont negate all/uncat filter
			if (wParam-MP_CAT_SET0<=1)
				thePrefs.SetCatFilterNeg(m_isetcatmenu, false);

			thePrefs.SetCatFilter(m_isetcatmenu,wParam-MP_CAT_SET0);
			m_nLastCatTT=-1;
		}
		EditCatTabLabel(m_isetcatmenu);
		downloadlistctrl.ChangeCategory( m_dlTab.GetCurSel() );
		thePrefs.SaveCats();
	}
	*/
	// khaos::categorymod-

	switch (wParam){ 
		case MP_CAT_ADD: {
			m_nLastCatTT=-1;
			int newindex=AddCategorie(_T("?"),thePrefs.GetIncomingDir(),_T(""),_T(""),false);
			CCatDialog dialog(newindex);
			dialog.DoModal();
			if (dialog.WasCancelled())
				thePrefs.RemoveCat(newindex);
			else {
				theApp.emuledlg->searchwnd->UpdateCatTabs();
				m_dlTab.InsertItem(newindex,thePrefs.GetCategory(newindex)->title);
				EditCatTabLabel(newindex);
				thePrefs.SaveCats();
				VerifyCatTabSize();
			}
			break;
		}
		// khaos::categorymod+						 
		//MORPH - Changed by SiRoB, Seen Complet filter
		/*
		case MP_CAT_SET0+16:
		*/
		case MP_CAT_SET0+17:
		case MP_CAT_EDIT: {
			m_nLastCatTT=-1;
			CCatDialog dialog(rightclickindex);
			dialog.DoModal();

			CString csName;
			csName.Format(_T("%s"), thePrefs.GetCategory(rightclickindex)->title );
			EditCatTabLabel(rightclickindex,csName);
		
			theApp.emuledlg->searchwnd->UpdateCatTabs();
			thePrefs.SaveCats();
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
			thePrefs.RemoveCat(rightclickindex);
			m_dlTab.DeleteItem(rightclickindex);
			m_dlTab.SetCurSel(useCat);
			downloadlistctrl.ChangeCategory(useCat);
			thePrefs.SaveCats();
			break;
		}
		// khaos::categorymod-
		case MP_CAT_REMOVE: {
			m_nLastCatTT=-1;
			theApp.downloadqueue->ResetCatParts(rightclickindex);
			thePrefs.RemoveCat(rightclickindex);
			m_dlTab.DeleteItem(rightclickindex);
			m_dlTab.SetCurSel(0);
			downloadlistctrl.ChangeCategory(0);
			thePrefs.SaveCats();
			/*
			if (thePrefs.GetCatCount()==1) thePrefs.SetAllcatType(0);
			*/
			theApp.emuledlg->searchwnd->UpdateCatTabs();
			VerifyCatTabSize();
			break;
		}
// ZZ:DownloadManager -->
		case MP_PRIOLOW: {
            thePrefs.GetCategory(rightclickindex)->prio = PR_LOW;
			
            //CString csName;
            //csName.Format(_T("%s"), thePrefs.GetCategory(rightclickindex)->title );
            //EditCatTabLabel(rightclickindex,csName);

            //theApp.emuledlg->searchwnd->UpdateCatTabs();
			thePrefs.SaveCats();
			break;
		}
		case MP_PRIONORMAL: {
            thePrefs.GetCategory(rightclickindex)->prio = PR_NORMAL;
			
            //CString csName;
            //csName.Format(_T("%s"), thePrefs.GetCategory(rightclickindex)->title );
            //EditCatTabLabel(rightclickindex,csName);

            //theApp.emuledlg->searchwnd->UpdateCatTabs();
			thePrefs.SaveCats();
			break;
		}
		case MP_PRIOHIGH: {
            thePrefs.GetCategory(rightclickindex)->prio = PR_HIGH;
			
            //CString csName;
            //csName.Format(_T("%s"), thePrefs.GetCategory(rightclickindex)->title );
            //EditCatTabLabel(rightclickindex,csName);

            //theApp.emuledlg->searchwnd->UpdateCatTabs();
			thePrefs.SaveCats();
			break;
		}
// <-- ZZ:DownloadManager

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
		case MP_RESUMENEXT: {
			theApp.downloadqueue->StartNextFile(rightclickindex,false);
			break;
		}

// ZZ:DownloadManager -->
		case MP_DOWNLOAD_ALPHABETICAL: {
            BOOL newSetting = !thePrefs.GetCategory(rightclickindex)->downloadInAlphabeticalOrder;
            thePrefs.GetCategory(rightclickindex)->downloadInAlphabeticalOrder = newSetting;
			thePrefs.SaveCats();
            if(newSetting) {
                // any auto prio files will be set to normal now.
                theApp.downloadqueue->RemoveAutoPrioInCat(rightclickindex, PR_NORMAL);
            }

            break;
		}
// <-- ZZ:DownloadManager
		// khaos::categorymod+
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
//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
		case IDC_DL_CHANGEVIEW: 
			{
				OnBnClickedChangeView();
				break;
			}
		case IDC_DL_DOWN_UPLOADS: 
			{
				OnBnClickedDownUploads();
				break;
			}
		case IDC_DL_DOWNLOADS: 
			{
				ShowList(IDC_DOWNLOADLIST);
				break;
			}
		case IDC_DL_UPLOADS: {
			ShowList(IDC_UPLOADLIST);
			break;
							 }
		case IDC_DL_QUEUE: 
			{
				ShowList(IDC_QUEUELIST);
				break;
			}
		case IDC_DL_TRANSFERS: 
			{
				ShowList(IDC_DOWNLOADCLIENTS);
				break;
			}
		case IDC_DL_CLIENT: 
			{
				ShowList(IDC_CLIENTLIST);
				break;
			}
			//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons <=

			//SLAHAM: ADDED Switch Lists Icons =>
		case IDC_UL_CHANGEVIEW: 
			{
				SwitchUploadList();
				break;
			}
		case IDC_UL_UPLOADS: 
			{
				ShowWnd2(m_uWnd2 = 1);
			break;
			}
		case IDC_UL_QUEUE: 
			{
				ShowWnd2(m_uWnd2 = 2);
				break;
			}
		case IDC_UL_CLIENTS: 
			{
				ShowWnd2(m_uWnd2 = 3);
				break;
			}
		case IDC_UL_TRANSFERS: 
			{
				ShowWnd2(m_uWnd2 = 0);
				break;
			}
//SLAHAM: ADDED Switch Lists Icons <=

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
		//MORPH START - Added by SiRoB, Seen Complet filter
		case MP_CAT_SET0+16: {
			curCat->viewfilters.bSeenComplet = curCat->viewfilters.bSeenComplet ? false : true;
			break;
		}
		//MORPH END   - Added by SiRoB, Seen Complet filter
	    //MORPH START - Added by Commander, Open Incoming Folder Fix
		case MP_HM_OPENINC:
			ShellExecute(NULL, _T("open"), curCat->incomingpath,NULL, NULL, SW_SHOW);
			break;
		//MORPH END - Added by Commander, Open Incoming Folder Fix
	}

	//MORPH - Changed by SiRoB, Seen Complet filter
	/*
	if (wParam >= MP_CAT_SET0 && wParam <= MP_CAT_SET0 + 15)
	*/
	if (wParam >= MP_CAT_SET0 && wParam <= MP_CAT_SET0 + 16)
		downloadlistctrl.ChangeCategory(m_dlTab.GetCurSel());
	if (wParam >= MP_CAT_A4AF && wParam <= MP_CAT_A4AF + 2)
		curCat->iAdvA4AFMode = wParam - MP_CAT_A4AF;
	// khaos::categorymod-
	
	return TRUE;
}

void CTransferWnd::UpdateCatTabTitles(bool force) {


	CPoint pt;
	::GetCursorPos(&pt);
	if (!force && GetTabUnderMouse(&pt)!=-1)		// avoid cat tooltip jumping
		return;

	for (uint8 i=0;i<m_dlTab.GetItemCount();i++)
		//MORPH START - Changed by SiRoB, Due to Khaos Category
		/*
		EditCatTabLabel(i,(i==0)? GetCatTitle( thePrefs.GetAllcatType() ):thePrefs.GetCategory(i)->title);
		*/
		EditCatTabLabel(i, thePrefs.GetCategory(i)->title);
		//MORPH END   - Changed by SiRoB, Due to Khaos Category
}

void CTransferWnd::EditCatTabLabel(int i) {
	//MORPH	- Changed by SiRoB, Khaos Category
	/*
	EditCatTabLabel(i,(i==0)? GetCatTitle( thePrefs.GetAllcatType() ):thePrefs.GetCategory(i)->title);
	*/
	EditCatTabLabel(i,thePrefs.GetCategory(i)->title);
}

void CTransferWnd::EditCatTabLabel(int index,CString newlabel) {

	TCITEM tabitem;
	tabitem.mask = TCIF_PARAM;
	m_dlTab.GetItem(index,&tabitem);
	tabitem.mask = TCIF_TEXT;

	newlabel.Replace(_T("&"),_T("&&"));
	//MORPH - Removed by SIROB, Due to Khaos Cat
	/*
	// add filter label
	if (index && thePrefs.GetCatFilter(index)>0) {
		newlabel.Append(_T(" (")) ;
		if (thePrefs.GetCatFilterNeg(index))
			newlabel.Append(_T("!"));			
			newlabel.Append( GetCatTitle(thePrefs.GetCatFilter(index)) + _T(")") );
	} else
		if (!index && thePrefs.GetCatFilterNeg(index)  )
			newlabel=_T("!") + newlabel;
	*/

	int count,dwl;

// ZZ:DownloadManager -->
    //CString prioStr;
    //switch(thePrefs.GetCategory(index)->prio) {
    //    case PR_LOW:
    //        prioStr = _T(" ") + GetResString(IDS_PR_SHORT_LOW);
    //        break;

    //    case PR_HIGH:
    //        prioStr = _T(" ") + GetResString(IDS_PR_SHORT_HIGH);
    //        break;

    //    default:
    //        prioStr = _T("");
    //        break;
    //}
// <-- ZZ:DownloadManager

	if (thePrefs.ShowCatTabInfos()) {
		CPartFile* cur_file;
		count=dwl=0;
		for (int i=0;i<theApp.downloadqueue->GetFileCount();i++) {
			cur_file=theApp.downloadqueue->GetFileByIndex(i);
			if (cur_file==0) continue;
			if (cur_file->CheckShowItemInGivenCat(index)) {
				if (cur_file->GetTransferingSrcCount()>0) ++dwl;
			}
		}
		CString title=newlabel;
		int compl= theApp.emuledlg->transferwnd->downloadlistctrl.GetCompleteDownloads(index,count);
		newlabel.Format(_T("%s %i/%i"),title,dwl,count); // ZZ:DownloadManager
		//newlabel.Format(_T("%s%s %i/%i"),title, prioStr,dwl,count); // ZZ:DownloadManager

// ZZ:DownloadManager -->
    //} else {
    //    newlabel += prioStr;
// <-- ZZ:DownloadManager
	}

	tabitem.pszText = newlabel.LockBuffer();
	m_dlTab.SetItem(index,&tabitem);
	newlabel.UnlockBuffer();

	VerifyCatTabSize();
}

int CTransferWnd::AddCategorie(CString newtitle,CString newincoming,CString newcomment,CString newautocat,bool addTab){
	Category_Struct* newcat=new Category_Struct;

	_stprintf(newcat->title,newtitle);
	newcat->prio=PR_NORMAL; // ZZ:DownloadManager
	// khaos::kmod+ Category Advanced A4AF Mode and Auto Cat
	newcat->iAdvA4AFMode = 0;
	// khaos::kmod-
	_stprintf(newcat->incomingpath,newincoming);
	_stprintf(newcat->comment,newcomment);
	/*
	autocat=newautocat; //replaced by viewfilters.sAdvancedFilterMask
	newcat->autocat=newautocat;
	newcat->filter=0;
	newcat->filterNeg=false;
	*/
    newcat->downloadInAlphabeticalOrder = FALSE; // ZZ:DownloadManager

	// khaos::categorymod+ Initialize View Filter Variables
	newcat->viewfilters.bArchives = true;
	newcat->viewfilters.bAudio = true;
	newcat->viewfilters.bComplete = true;
	newcat->viewfilters.bCompleting = true;
	newcat->viewfilters.bSeenComplet = true; //MORPH START - Added by SiRoB, Seen Complet filter
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
	newcat->bResumeFileOnlyInSameCat = false;	//MORPH - Added by SiRoB, Resume file only in the same category
	// khaos::categorymod-
	int index=thePrefs.AddCat(newcat);
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

		for (i=0;i<m_tooltipCats.GetToolCount();i++)
			m_tooltipCats.DelTool(&m_dlTab,i+1);

		for (i = 0; i < m_dlTab.GetItemCount(); i++)
		{
			CRect r;
			m_dlTab.GetItemRect(i, &r);
			VERIFY(m_tooltipCats.AddTool(&m_dlTab, GetTabStatistic(i), &r, i+1));
		}
	} else {
			CRect r;
			m_dlTab.GetItemRect(tab, &r);

		m_tooltipCats.DelTool(&m_dlTab,tab+1);
		VERIFY(m_tooltipCats.AddTool(&m_dlTab, GetTabStatistic(tab), &r, tab+1));
	}
}

CString CTransferWnd::GetTabStatistic(uint8 tab) {
	uint16 count,dwl,err,compl,paus;
	count=dwl=err=compl=paus=0;
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
			if (cur_file->GetStatus()==PS_ERROR) ++err;
			if (cur_file->GetStatus()==PS_PAUSED) ++paus;
		}
	}

	int total;
	compl=theApp.emuledlg->transferwnd->downloadlistctrl.GetCompleteDownloads(tab,total);

// ZZ:DownloadManager -->
    CString prio;
    switch(thePrefs.GetCategory(tab)->prio) {
        case PR_LOW:
            prio = GetResString(IDS_PRIOLOW);
            break;

        case PR_HIGH:
            prio = GetResString(IDS_PRIOHIGH);
            break;

        default:
            prio = GetResString(IDS_PRIONORMAL);
            break;
    }
// ZZ:DownloadManager <--

	CString title;
	title.Format(_T("%s: %i\n\n%s: %i\n%s: %i\n%s: %i\n%s: %i\n\n%s: %s\n\n%s: %.1f %s\n%s: %s/%s\n%s%s"), // ZZ:DownloadManager
		
		GetResString(IDS_FILES), count+compl,
		GetResString(IDS_DOWNLOADING), dwl,
		GetResString(IDS_PAUSED) ,paus,
		GetResString(IDS_ERRORLIKE) ,err,
		GetResString(IDS_DL_TRANSFCOMPL) ,compl,

        GetResString(IDS_PRIORITY), prio, // ZZ:DownloadManager

		GetResString(IDS_DL_SPEED) ,speed,GetResString(IDS_KBYTESEC),


		GetResString(IDS_DL_SIZE),CastItoXBytes(trsize, false, false),CastItoXBytes(size, false, false),
		GetResString(IDS_ONDISK),CastItoXBytes(disksize, false, false));
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
	if (!thePrefs.MoveCat(from,to)) return;

	// update partfile-stored assignment
	theApp.downloadqueue->MoveCat((uint8)from,(uint8)to);

	// move category of completed files
	downloadlistctrl.MoveCompletedfilesCat((uint8)from,(uint8)to);

	// of the tabcontrol itself
	m_dlTab.ReorderTab(from,to);

	UpdateCatTabTitles();
	theApp.emuledlg->searchwnd->UpdateCatTabs();

	if (to>from) --to;
	m_dlTab.SetCurSel(to);
	downloadlistctrl.ChangeCategory(to);
}

// VerifyCatTabSize 
//
// [Comment by Mighty Knife:]
//
// Verify the proper positioning of the download tab's. If the
// positioning is wrong, the tab's are repositioned.
//
// If the download-list is not visible, no repositioning of
// the tab's is performed. That's the case if the download list
// is hidden by the category buttons next to the download tab's.
// If repositioning took place, the download tab's would be made
// visible, what's undesired if the download list is not visible!
//
// Unfortunately if the dialog is build up in the InitDialog
// routine, the download window is not visible, but the resizing must
// be done. In such a case if _forceverify==true, the resizing is
// forced, even if the download window is not visible!

void CTransferWnd::VerifyCatTabSize(bool _forceverify) {
	//MORPH - Added by SiRoB, Show/Hide dlTab
	// MightyKnife: Forcing of the verification added
	if ((downloadlistctrl.IsWindowVisible() == false) && (!_forceverify))
		return;
	// [end] Mighty Knife
	//MORPH - Added by SiRoB, Show/Hide dlTab
			
	CRect rect;
	int size=0;
	int right;

	for (int i=0;i<m_dlTab.GetItemCount();i++) {
		m_dlTab.GetItemRect(i,&rect);
		size+= rect.Width();
	}
	size+=20;

	WINDOWPLACEMENT wpTabWinPos;

	downloadlistctrl.GetWindowPlacement(&wpTabWinPos);
	right=wpTabWinPos.rcNormalPosition.right;

	m_dlTab.GetWindowPlacement(&wpTabWinPos);
	if (wpTabWinPos.rcNormalPosition.right<0) return;

	wpTabWinPos.rcNormalPosition.right=right;
	int left=wpTabWinPos.rcNormalPosition.right-size;
	//MORPH START - Changed by SiRoB, Due to Khaos Categorie
	//if (left<200) left=200;
	//if (left<260) left=260;
	if (left<425) left=425; //SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons
	//MORPH END   - Changed by SiRoB, Due to Khaos Categorie
	wpTabWinPos.rcNormalPosition.left=left;

			RemoveAnchor(m_dlTab);
			m_dlTab.SetWindowPlacement(&wpTabWinPos);
			AddAnchor(m_dlTab,TOP_RIGHT);
}
//MORPH - Removed by SiRoB, Due to Khaos Categorie
/*
CString CTransferWnd::GetCatTitle(int catid)
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
	return _T("?");
}
*/
