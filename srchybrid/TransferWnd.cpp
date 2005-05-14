//this file is part of eMule
//Copyright (C)2002-2005 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "InputBox.h"
#include "UserMsgs.h"
#include "SharedFileList.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

#define	WND_SPLITTER_YOFF	8
#define	WND_SPLITTER_HEIGHT	5

#define	WND1_BUTTON_XOFF	8
#define	WND1_BUTTON_WIDTH	170
#define	WND1_BUTTON_HEIGHT	22	// don't set the height do something different than 22 unless you know exactly what you are doing!
#define	NUM_WINA_BUTTONS	6

#define	WND2_BUTTON_XOFF	8
#define	WND2_BUTTON_WIDTH	170
#define	WND2_BUTTON_HEIGHT	22	// don't set the height do something different than 22 unless you know exactly what you are doing!

// CTransferWnd dialog

IMPLEMENT_DYNAMIC(CTransferWnd, CDialog)

BEGIN_MESSAGE_MAP(CTransferWnd, CResizableDialog)
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY(LVN_HOTTRACK, IDC_UPLOADLIST, OnHoverUploadList)
	ON_NOTIFY(LVN_HOTTRACK, IDC_QUEUELIST, OnHoverUploadList)
	ON_NOTIFY(LVN_HOTTRACK, IDC_DOWNLOADLIST, OnHoverDownloadList)
	ON_NOTIFY(LVN_HOTTRACK, IDC_DOWNLOADCLIENTS, OnHoverUploadList) //MORPH - Added by SiRoB, Fix
	ON_NOTIFY(LVN_HOTTRACK, IDC_CLIENTLIST , OnHoverUploadList)
	ON_NOTIFY(TCN_SELCHANGE, IDC_DLTAB, OnTcnSelchangeDltab)
	ON_NOTIFY(LVN_BEGINDRAG, IDC_DOWNLOADLIST, OnLvnBegindrag)
	ON_NOTIFY(LVN_KEYDOWN, IDC_DOWNLOADLIST, OnLvnKeydownDownloadlist)
	ON_NOTIFY(NM_RCLICK, IDC_DLTAB, OnNMRclickDltab)
	ON_NOTIFY(UM_TABMOVED, IDC_DLTAB, OnTabMovement)
	ON_NOTIFY(TBN_DROPDOWN, IDC_DOWNLOAD_ICO, OnWnd1BtnDropDown)
	ON_NOTIFY(TBN_DROPDOWN, IDC_UPLOAD_ICO, OnWnd2BtnDropDown)
END_MESSAGE_MAP()

CTransferWnd::CTransferWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CTransferWnd::IDD, pParent)
{
	m_uWnd2 = wnd2Uploading;
	m_dwShowListIDC = 0;
	m_pLastMousePoint.x = -1;
	m_pLastMousePoint.y = -1;
	m_nLastCatTT = -1;
}

CTransferWnd::~CTransferWnd()
{
	// khaos::categorymod+
	VERIFY(m_mnuCatPriority.DestroyMenu());
	VERIFY(m_mnuCatViewFilter.DestroyMenu());
	VERIFY(m_mnuCategory.DestroyMenu());
	// khaos::categorymod-
}

BOOL CTransferWnd::OnInitDialog()
{
	GetDlgItem(IDC_DOWNLOAD_ICO)->DestroyWindow(); // delete the control ID place holder dummy window

	CRect rc;
	ResetTransToolbar(thePrefs.IsTransToolbarEnabled(), false);	
	GetDlgItem(IDC_UPLOAD_ICO)->GetWindowRect(&rc);
	ScreenToClient(&rc);
	rc.top -= 1;
	GetDlgItem(IDC_UPLOAD_ICO)->DestroyWindow();
	rc.left = WND2_BUTTON_XOFF;
	rc.right = rc.left + WND2_BUTTON_WIDTH;
	rc.bottom = rc.top + WND2_BUTTON_HEIGHT;
	m_btnWnd2.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, rc, this, IDC_UPLOAD_ICO, true);

	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);

	uploadlistctrl.Init();
	downloadlistctrl.Init();
	queuelistctrl.Init();
	clientlistctrl.Init();
	downloadclientsctrl.Init();

	if (thePrefs.GetRestoreLastMainWndDlg())
		m_uWnd2 = (EWnd2)thePrefs.GetTransferWnd2();
	ShowWnd2(m_uWnd2);

	SetAllIcons();
    Localize();

	//AddAnchor(m_btnWnd1, TOP_LEFT);
	AddAnchor(IDC_DOWNLOADLIST,TOP_LEFT,CSize(100, thePrefs.GetSplitterbarPosition() ));
	AddAnchor(IDC_UPLOADLIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_QUEUELIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_CLIENTLIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(m_btnWnd2, CSize(0, thePrefs.GetSplitterbarPosition()), BOTTOM_RIGHT);
	AddAnchor(IDC_DOWNLOADCLIENTS, CSize(0, thePrefs.GetSplitterbarPosition()), BOTTOM_RIGHT);
	AddAnchor(IDC_QUEUECOUNT,BOTTOM_LEFT);
    AddAnchor(IDC_QUEUE, BOTTOM_LEFT, BOTTOM_RIGHT); //Commander - Added: ClientQueueProgressBar
	AddAnchor(IDC_TSTATIC1,BOTTOM_LEFT);
	AddAnchor(IDC_QUEUE_REFRESH_BUTTON, BOTTOM_RIGHT);
	AddAnchor(IDC_DLTAB,CSize(50,0) ,TOP_RIGHT);

	// splitting functionality
	GetWindowRect(rc);
	ScreenToClient(rc);
	CRect rcSpl;
	rcSpl.left = 55;
	rcSpl.right = rc.right;
	rcSpl.top = rc.bottom - 100;
	rcSpl.bottom = rcSpl.top + WND_SPLITTER_HEIGHT;
	m_wndSplitter.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER);
	OnBnClickedDownUploads(); // replaces SetInitLayout();
	
	//cats
	rightclickindex=-1;
	// khaos::categorymod+ Create the category right-click menu.
	CreateCategoryMenus();
	// khaos::categorymod-

	downloadlistactive=true;
	m_bIsDragging=false;

// khaos::categorymod+
	// show & cat-tabs
	/*
	_stprintf(thePrefs.GetCategory(0)->title, _T("%s"), GetCatTitle(thePrefs.GetCategory(0)->filter));
	_stprintf(thePrefs.GetCategory(0)->incomingpath, _T("%s"), thePrefs.GetIncomingDir());
	thePrefs.GetCategory(0)->care4all=true;

	for (int ix=0;ix<thePrefs.GetCatCount();ix++)
		m_dlTab.InsertItem(ix,thePrefs.GetCategory(ix)->title );
	*/
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
	// create tooltip control for download categories
	m_tooltipCats.Create(this, TTS_NOPREFIX);
	m_dlTab.SetToolTips(&m_tooltipCats);
	UpdateCatTabTitles();
	UpdateTabToolTips();
	m_tooltipCats.SendMessage(TTM_SETMAXTIPWIDTH, 0, SHRT_MAX); // recognize \n chars!
	m_tooltipCats.SetDelayTime(TTDT_AUTOPOP, 20000);
	m_tooltipCats.SetDelayTime(TTDT_INITIAL, thePrefs.GetToolTipDelay()*1000);
	m_tooltipCats.Activate(TRUE);

	UpdateListCount(m_uWnd2);
	// Mighty Knife: Force category tab verification even if window is not visible
	/*
	VerifyCatTabSize();
	*/
	VerifyCatTabSize(true);
	// [end] Mighty Knife
	//Commander - Added: ClientQueueProgressBar - Start
	bold.CreateFont(10,0,0,1,FW_BOLD,0,0,0,0,3,2,1,34,_T("Small Fonts"));

	queueBar.SetFont(&bold);
	queueBar.SetBkColor(GetSysColor(COLOR_WINDOW));
	queueBar.SetShowPercent();
	queueBar.SetGradientColors(GetSysColor(COLOR_WINDOW),GetSysColor(COLOR_WINDOW));
	if(thePrefs.IsInfiniteQueueEnabled() || !thePrefs.ShowClientQueueProgressBar()){
		GetDlgItem(IDC_QUEUE)->ShowWindow(SW_HIDE);
	}
	//Commander - Added: ClientQueueProgressBar - End

	return true;
}

void CTransferWnd::ShowQueueCount(uint32 number)
{
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
		UINT iMaxQueueSize = thePrefs.GetQueueSize() + max(thePrefs.GetQueueSize()/4, 200);
		queueBar.SetRange32(0, iMaxQueueSize); //Softlimit -> GetQueueSize | Hardlimit -> (GetQueueSize + (GetQueueSize/4))
		if (number<=thePrefs.GetQueueSize()){
			queueBar.SetGradientColors(RGB(0, 192, 0), RGB(255, 255, 0));
		}else {
			queueBar.SetGradientColors(RGB(255, 255, 0), RGB(255, 0, 0));
		}
		queueBar.SetPos(number);
	}
    //Commander - Added: ClientQueueProgressBar - End
}

void CTransferWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_UPLOADLIST, uploadlistctrl);
	DDX_Control(pDX, IDC_DOWNLOADLIST, downloadlistctrl);
	DDX_Control(pDX, IDC_QUEUELIST, queuelistctrl);
	DDX_Control(pDX, IDC_CLIENTLIST, clientlistctrl);
	DDX_Control(pDX, IDC_DOWNLOADCLIENTS, downloadclientsctrl);
	DDX_Control(pDX, IDC_UPLOAD_ICO, m_btnWnd2);
	DDX_Control(pDX, IDC_DLTAB, m_dlTab);
	DDX_Control(pDX, IDC_QUEUE, queueBar); //Commander - Added: ClientQueueProgressBar
}

void CTransferWnd::DoResize(int delta)
{
	CSplitterControl::ChangeHeight(&downloadlistctrl, delta);
	CSplitterControl::ChangeHeight(&uploadlistctrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(&queuelistctrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(&clientlistctrl, -delta, CW_BOTTOMALIGN);
	CSplitterControl::ChangeHeight(&downloadclientsctrl, -delta, CW_BOTTOMALIGN);

	UpdateSplitterRange();

	Invalidate();
	UpdateWindow();
}

void CTransferWnd::UpdateSplitterRange()
{
	CRect rcWnd;
	GetWindowRect(rcWnd);

	CRect rcDown;
	downloadlistctrl.GetWindowRect(rcDown);
	ScreenToClient(rcDown);

	CRect rcUp;
	downloadclientsctrl.GetWindowRect(rcUp);
	ScreenToClient(rcUp);

	// [Comment by Mighty Knife:]
	// Calculate the percentage position of the splitter bar and save it.
	// Make sure to subtract the 5 pixel we added to the bottom position of
	// the downloadlist!
	// Furthermore we add another Height/2 before dividing by Height to perform
	// real rounding instead of just truncating - otherwise we might end up 1% too small!
	/*
	thePrefs.SetSplitterbarPosition((rcDown.bottom * 100) / rcWnd.Height());
	*/
	thePrefs.SetSplitterbarPosition(((rcDown.bottom - 5) * 100+rcWnd.Height() / 2) / rcWnd.Height());

	RemoveAnchor(IDC_DOWNLOADLIST);
	RemoveAnchor(IDC_UPLOADLIST);
	RemoveAnchor(IDC_QUEUELIST);
	RemoveAnchor(IDC_CLIENTLIST);
	RemoveAnchor(IDC_DOWNLOADCLIENTS);

	AddAnchor(IDC_DOWNLOADLIST,TOP_LEFT,CSize(100,thePrefs.GetSplitterbarPosition() ));
	AddAnchor(IDC_UPLOADLIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_QUEUELIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_CLIENTLIST,CSize(0,thePrefs.GetSplitterbarPosition()),BOTTOM_RIGHT);
	AddAnchor(IDC_DOWNLOADCLIENTS, CSize(0, thePrefs.GetSplitterbarPosition()), BOTTOM_RIGHT);

	m_wndSplitter.SetRange(rcDown.top+50 , rcUp.bottom-40);
}

LRESULT CTransferWnd::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message)
	{
		// arrange transferwindow layout
		case WM_PAINT:
			if (m_wndSplitter)
			{
				CRect rcWnd;
				GetWindowRect(rcWnd);
				if (rcWnd.Height() > 0)
				{
					CRect rcDown;
					downloadlistctrl.GetWindowRect(rcDown);
					ScreenToClient(rcDown);

					// splitter paint update
					CRect rcSpl;
					rcSpl.left = WND2_BUTTON_XOFF + WND2_BUTTON_WIDTH + 8;
					rcSpl.right = rcDown.right;
					rcSpl.top = rcDown.bottom + WND_SPLITTER_YOFF;
					rcSpl.bottom = rcSpl.top + WND_SPLITTER_HEIGHT;
					m_wndSplitter.MoveWindow(rcSpl, TRUE);
					m_btnWnd2.MoveWindow(WND2_BUTTON_XOFF, rcSpl.top - (WND_SPLITTER_YOFF - 1), WND2_BUTTON_WIDTH, WND2_BUTTON_HEIGHT);
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
			if (m_wndSplitter)
			{
				CRect rcWnd;
				GetWindowRect(rcWnd);
				if (rcWnd.Height() > 0)
					Invalidate();
			}
			break;
	}

	return CResizableDialog::DefWindowProc(message, wParam, lParam);
}

// CTransferWnd message handlers
BOOL CTransferWnd::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message== WM_LBUTTONDBLCLK && pMsg->hwnd == m_dlTab.m_hWnd) {
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

	if (pMsg->message == WM_MBUTTONUP)
	{
		if (downloadlistactive)
			downloadlistctrl.ShowSelectedFileDetails();
		//MORPH START - Added by SiRoB, DownloadClientCtrl
		else if (m_dwShowListIDC != IDC_DOWNLOADLIST + IDC_UPLOADLIST)
			switch(m_dwShowListIDC){
				case IDC_UPLOADLIST:
					uploadlistctrl.ShowSelectedUserDetails();
					break;
				case IDC_QUEUELIST:
					queuelistctrl.ShowSelectedUserDetails();
					break;
				case IDC_CLIENTLIST:
					clientlistctrl.ShowSelectedUserDetails();
					break;
				case IDC_DOWNLOADCLIENTS:
					downloadclientsctrl.ShowSelectedUserDetails();
					break;
			}
		//MORPH END   - Added by SiRoB, DownloadClientsCtrl
		else
		{
			switch (m_uWnd2)
			{
				case wnd2OnQueue:
					queuelistctrl.ShowSelectedUserDetails();
					break;
				case wnd2Uploading:
					uploadlistctrl.ShowSelectedUserDetails();
					break;
				case wnd2Clients:
					clientlistctrl.ShowSelectedUserDetails();
					break;
				case wnd2Downloading:
					downloadclientsctrl.ShowSelectedUserDetails(); //MORPH - Added by SiRoB, DownloadClientsCtrl
					break;
				default:
					ASSERT(0);
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

void CTransferWnd::UpdateFilesCount(int iCount)
{
	if (m_dwShowListIDC == IDC_DOWNLOADLIST || m_dwShowListIDC == IDC_DOWNLOADLIST + IDC_UPLOADLIST)
	{
		CString strBuffer;
		strBuffer.Format(_T("%s (%u)"), GetResString(IDS_TW_DOWNLOADS), iCount);
		m_btnWnd1.SetWindowText(strBuffer);
	}
}

void CTransferWnd::UpdateListCount(EWnd2 listindex, int iCount /*=-1*/)
{
	switch (m_dwShowListIDC)
	{
		case IDC_DOWNLOADLIST + IDC_UPLOADLIST: {
			if (m_uWnd2 != listindex)
				return;
			CString strBuffer;
			switch (m_uWnd2)
			{
				case wnd2Uploading: {
					uint32 itemCount = iCount == -1 ? uploadlistctrl.GetItemCount() : iCount;
					uint32 activeCount = theApp.uploadqueue->GetActiveUploadsCount();
					if (activeCount >= itemCount)
						strBuffer.Format(_T(" (%i)"), itemCount);
					else
						strBuffer.Format(_T(" (%i/%i)"), activeCount, itemCount);
		            //Upload Speed Sens
					strBuffer.AppendFormat(_T(" %i Max"), theApp.uploadqueue->GetActiveUploadsCountLongPerspective());

					m_btnWnd2.SetWindowText(GetResString(IDS_UPLOADING) + strBuffer);
					break;
				}
				case wnd2OnQueue:
					strBuffer.Format(_T(" (%i)"), iCount == -1 ? queuelistctrl.GetItemCount() : iCount);
					m_btnWnd2.SetWindowText(GetResString(IDS_ONQUEUE) + strBuffer);
					break;
				case wnd2Clients:
					strBuffer.Format(_T(" (%i)"), iCount == -1 ? clientlistctrl.GetItemCount() : iCount);
					m_btnWnd2.SetWindowText(GetResString(IDS_CLIENTLIST) + strBuffer);
					break;
				case wnd2Downloading:
					strBuffer.Format(_T(" (%i)"), iCount == -1 ? downloadclientsctrl.GetItemCount() : iCount);
					m_btnWnd2.SetWindowText(GetResString(IDS_DOWNLOADING) + strBuffer);
					break;
				default:
					ASSERT(0);
			}
			break;
		}

		case IDC_DOWNLOADLIST:
			break;

		case IDC_UPLOADLIST:
			if (listindex == wnd2Uploading)
			{
				CString strBuffer;
            	uint32 itemCount = iCount == -1 ? uploadlistctrl.GetItemCount() : iCount;
            	uint32 activeCount = theApp.uploadqueue->GetActiveUploadsCount();
				if (activeCount >= itemCount)
					strBuffer.Format(_T(" (%i)"), itemCount);
				else
					strBuffer.Format(_T(" (%i/%i)"), activeCount, itemCount);
				//Upload Speed Sens
				strBuffer.AppendFormat(_T(" %i Max"), theApp.uploadqueue->GetActiveUploadsCountLongPerspective());
				m_btnWnd1.SetWindowText(GetResString(IDS_UPLOADING) + strBuffer);
            }
			break;

		case IDC_QUEUELIST:
			if (listindex == wnd2OnQueue)
			{
				CString strBuffer;
				strBuffer.Format(_T(" (%i)"), iCount == -1 ? queuelistctrl.GetItemCount() : iCount);
				m_btnWnd1.SetWindowText(GetResString(IDS_ONQUEUE) + strBuffer);
			}
			break;
		
		case IDC_CLIENTLIST:
			if (listindex == wnd2Clients)
			{
				CString strBuffer;
				strBuffer.Format(_T(" (%i)"), iCount == -1 ? clientlistctrl.GetItemCount() : iCount);
				m_btnWnd1.SetWindowText(GetResString(IDS_CLIENTLIST) + strBuffer);
        	}
			break;

		case IDC_DOWNLOADCLIENTS:
			if (listindex == wnd2Downloading)
			{
				CString strBuffer;
				strBuffer.Format(_T(" (%i)"), iCount == -1 ? downloadclientsctrl.GetItemCount() : iCount);
				m_btnWnd1.SetWindowText(GetResString(IDS_DOWNLOADING) + strBuffer);
			}
			break;

		default:
			//ASSERT(0);
			;
	}
	}

void CTransferWnd::SwitchUploadList()
{
	if (m_uWnd2 == wnd2Uploading)
	{
		SetWnd2(wnd2OnQueue);
		if( thePrefs.IsQueueListDisabled()){
			SwitchUploadList();
			return;
		}
		uploadlistctrl.Hide();
		clientlistctrl.Hide();
		downloadclientsctrl.Hide();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_SHOW);
		queuelistctrl.Visable();
		m_btnWnd2.SetWindowText(GetResString(IDS_ONQUEUE));
	}
	else if (m_uWnd2 == wnd2OnQueue)
	{
		SetWnd2(wnd2Clients);
		if( thePrefs.IsKnownClientListDisabled()){
			SwitchUploadList();
			return;
		}
		uploadlistctrl.Hide();
		queuelistctrl.Hide();
		downloadclientsctrl.Hide();
		clientlistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		m_btnWnd2.SetWindowText(GetResString(IDS_CLIENTLIST));
	}
	else if (m_uWnd2 == wnd2Clients)
	{
		SetWnd2(wnd2Downloading);
		queuelistctrl.Hide();
		clientlistctrl.Hide();
		uploadlistctrl.Hide();
		downloadclientsctrl.Show();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		m_btnWnd2.SetWindowText(GetResString(IDS_DOWNLOADING));
	}
	else
	{
		queuelistctrl.Hide();
		clientlistctrl.Hide();
		downloadclientsctrl.Hide();
		uploadlistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		SetWnd2(wnd2Uploading);
		m_btnWnd2.SetWindowText(GetResString(IDS_UPLOADING));
	}
	UpdateListCount(m_uWnd2);
	SetWnd2Icon();
}

void CTransferWnd::ShowWnd2(EWnd2 uWnd2)
{
	if (uWnd2 == wnd2OnQueue && !thePrefs.IsQueueListDisabled())
	{
		uploadlistctrl.Hide();
		clientlistctrl.Hide();
		downloadclientsctrl.Hide();
		queuelistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_SHOW);
		m_btnWnd2.SetWindowText(GetResString(IDS_ONQUEUE));
		SetWnd2(uWnd2);
	}
	else if (uWnd2 == wnd2Clients && !thePrefs.IsKnownClientListDisabled())
	{
		uploadlistctrl.Hide();
		queuelistctrl.Hide();
		downloadclientsctrl.Hide();
		clientlistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		m_btnWnd2.SetWindowText(GetResString(IDS_CLIENTLIST));
		SetWnd2(uWnd2);
	}
	else if (uWnd2 == wnd2Downloading) 
	{
		uploadlistctrl.Hide();
		queuelistctrl.Hide();		
		clientlistctrl.Hide();
		downloadclientsctrl.Show();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		m_btnWnd2.SetWindowText(GetResString(IDS_DOWNLOADING));
		SetWnd2(uWnd2);
	}
	else
	{
		queuelistctrl.Hide();
		clientlistctrl.Hide();
		downloadclientsctrl.Hide();
		uploadlistctrl.Visable();
		GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow(SW_HIDE);
		m_btnWnd2.SetWindowText(GetResString(IDS_UPLOADING));
		SetWnd2(wnd2Uploading);
	}
	UpdateListCount(m_uWnd2);
	SetWnd2Icon();
}

void CTransferWnd::SetWnd2(EWnd2 uWnd2)
{
	m_uWnd2 = uWnd2;
	thePrefs.SetTransferWnd2(m_uWnd2);
}

void CTransferWnd::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
	m_btnWnd1.Invalidate();
}

void CTransferWnd::SetAllIcons()
{
	SetWnd1Icons();
	SetWnd2Icon();
}

void CTransferWnd::SetWnd1Icons()
{
	CImageList iml;
	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 1, 1);
	iml.Add(CTempIconLoader(_T("SplitWindow")));
	iml.Add(CTempIconLoader(_T("DownloadFiles")));
	iml.Add(CTempIconLoader(_T("Upload")));
	iml.Add(CTempIconLoader(_T("Download")));
	iml.Add(CTempIconLoader(_T("ClientsOnQueue")));
	iml.Add(CTempIconLoader(_T("ClientsKnown")));
	CImageList* pImlOld = m_btnWnd1.SetImageList(&iml);
	iml.Detach();
	if (pImlOld)
		pImlOld->DeleteImageList();
}

void CTransferWnd::SetWnd2Icon()
{
	if (m_uWnd2 == wnd2OnQueue)
		m_btnWnd2.SetIcon(_T("ClientsOnQueue"));
	else if (m_uWnd2 == wnd2Uploading)
		m_btnWnd2.SetIcon(_T("Upload"));
	else if (m_uWnd2 == wnd2Clients)
		m_btnWnd2.SetIcon(_T("ClientsKnown"));
	else if (m_uWnd2 == wnd2Downloading)
		m_btnWnd2.SetIcon(_T("Download"));
	else
		ASSERT(0);
}

void CTransferWnd::Localize()
{
	m_btnWnd1.SetWindowText(GetResString(IDS_TW_DOWNLOADS));
	m_btnWnd1.SetBtnText(MP_VIEW1_SPLIT_WINDOW, GetResString(IDS_SPLIT_WINDOW));
	m_btnWnd1.SetBtnText(MP_VIEW1_DOWNLOADS, GetResString(IDS_TW_DOWNLOADS));
	m_btnWnd1.SetBtnText(MP_VIEW1_UPLOADING, GetResString(IDS_UPLOADING));
	m_btnWnd1.SetBtnText(MP_VIEW1_DOWNLOADING, GetResString(IDS_DOWNLOADING));
	m_btnWnd1.SetBtnText(MP_VIEW1_ONQUEUE, GetResString(IDS_ONQUEUE));
	m_btnWnd1.SetBtnText(MP_VIEW1_CLIENTS, GetResString(IDS_CLIENTLIST));
	m_btnWnd2.SetWindowText(GetResString(IDS_UPLOADING));
	GetDlgItem(IDC_TSTATIC1)->SetWindowText(GetResString(IDS_TW_QUEUE));
	GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->SetWindowText(GetResString(IDS_SV_UPDATE));

	// khaos::categorymod+ Rebuild menus...
	CreateCategoryMenus();
	// khaos::categorymod-

	uploadlistctrl.Localize();
	queuelistctrl.Localize();
	downloadlistctrl.Localize();
	clientlistctrl.Localize();
	downloadclientsctrl.Localize();

	UpdateListCount(m_uWnd2);
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
	if (rightclickindex == -1)
		return;

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

		// If the current category is '0'...  Well, we can't very well delete the default category, now can we...
		/*
		if (m_nDropIndex>0 && thePrefs.GetCategory(m_nDropIndex)->care4all)	// not droppable
			m_dlTab.SetCurSel(-1);
		else
		*/
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
			downloadlistctrl.UpdateCurrentCategoryView();

			UpdateCatTabTitles();

		} else m_dlTab.SetCurSel(downloadlistctrl.curTab);
		downloadlistctrl.Invalidate();
	}
}

BOOL CTransferWnd::OnCommand(WPARAM wParam,LPARAM lParam )
{ 
	// khaos::categorymod+
	/*
	// category filter menuitems
	if (wParam>=MP_CAT_SET0 && wParam<=MP_CAT_SET0+99)
	{
		if (wParam==MP_CAT_SET0+17)
		{
			thePrefs.GetCategory(m_isetcatmenu)->care4all=!thePrefs.GetCategory(m_isetcatmenu)->care4all;
		}
		else if (wParam==MP_CAT_SET0+19) // negate
		{
			thePrefs.SetCatFilterNeg(m_isetcatmenu, (!thePrefs.GetCatFilterNeg(m_isetcatmenu)) );
		}
		else // set the view filter
		{
			if (wParam-MP_CAT_SET0<1)	// dont negate all filter
				thePrefs.SetCatFilterNeg(m_isetcatmenu, false);
			thePrefs.SetCatFilter(m_isetcatmenu,wParam-MP_CAT_SET0);
			m_nLastCatTT=-1;
		}

		// set to regexp but none is set for that category?
		if (wParam==MP_CAT_SET0+18 && thePrefs.GetCategory(m_isetcatmenu)->regexp.IsEmpty())
		{
			m_nLastCatTT=-1;
			CCatDialog dialog(rightclickindex);
			dialog.DoModal();

			// still no regexp?
			if (thePrefs.GetCategory(m_isetcatmenu)->regexp.IsEmpty())
				thePrefs.SetCatFilter(m_isetcatmenu,0);
		}

		downloadlistctrl.UpdateCurrentCategoryView();
		EditCatTabLabel(m_isetcatmenu);
		thePrefs.SaveCats();
		return TRUE;
	}
	*/
	Category_Struct* curCat;
	curCat = thePrefs.GetCategory(rightclickindex);
	// khaos::categorymod-

	switch (wParam)
	{
		case MP_CAT_ADD: {
			m_nLastCatTT=-1;
			int newindex=AddCategory(_T("?"),thePrefs.GetIncomingDir(),_T(""),_T(""),false);
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
			theApp.emuledlg->transferwnd->downloadlistctrl.UpdateCurrentCategoryView();
			thePrefs.SaveCats();
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
			theApp.emuledlg->transferwnd->downloadlistctrl.UpdateCurrentCategoryView();
			thePrefs.SaveCats();
			break;
		}
		// khaos::categorymod-
		case MP_CAT_REMOVE: {
			m_nLastCatTT=-1;
			bool toreload=( _tcsicmp(thePrefs.GetCatPath(rightclickindex),thePrefs.GetIncomingDir())!=0);
			theApp.downloadqueue->ResetCatParts(rightclickindex);
			thePrefs.RemoveCat(rightclickindex);
			m_dlTab.DeleteItem(rightclickindex);
			m_dlTab.SetCurSel(0);
			downloadlistctrl.ChangeCategory(0);
			thePrefs.SaveCats();
			/*
			if (thePrefs.GetCatCount()==1)
				thePrefs.GetCategory(0)->filter=0;
			*/
			theApp.emuledlg->searchwnd->UpdateCatTabs();
			VerifyCatTabSize();
			if (toreload)
				theApp.sharedfiles->Reload();
			break;
		}
		case MP_PRIOLOW:
            thePrefs.GetCategory(rightclickindex)->prio = PR_LOW;
            thePrefs.SaveCats();
			break;
		case MP_PRIONORMAL:
            thePrefs.GetCategory(rightclickindex)->prio = PR_NORMAL;
            thePrefs.SaveCats();
			break;
		case MP_PRIOHIGH:
            thePrefs.GetCategory(rightclickindex)->prio = PR_HIGH;
			thePrefs.SaveCats();
			break;

		case MP_PAUSE:
			theApp.downloadqueue->SetCatStatus(rightclickindex,MP_PAUSE);
			break;
		case MP_STOP:
			theApp.downloadqueue->SetCatStatus(rightclickindex,MP_STOP);
			break;
		case MP_CANCEL:
			if (AfxMessageBox(GetResString(IDS_Q_CANCELDL),MB_ICONQUESTION|MB_YESNO) == IDYES)
				theApp.downloadqueue->SetCatStatus(rightclickindex,MP_CANCEL);
			break;
		case MP_RESUME:
			theApp.downloadqueue->SetCatStatus(rightclickindex,MP_RESUME);
			break;
		case MP_RESUMENEXT:
			theApp.downloadqueue->StartNextFile(rightclickindex,false);
			break;

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

		case IDC_UPLOAD_ICO:
			SwitchUploadList();
			break;
		case MP_VIEW2_UPLOADING:
			ShowWnd2(wnd2Uploading);
			break;
		case MP_VIEW2_DOWNLOADING:
			ShowWnd2(wnd2Downloading);
			break;
		case MP_VIEW2_ONQUEUE:
			ShowWnd2(wnd2OnQueue);
			break;
		case MP_VIEW2_CLIENTS:
			ShowWnd2(wnd2Clients);
			break;
		case IDC_QUEUE_REFRESH_BUTTON:
			OnBnClickedQueueRefreshButton();
			break;

		case IDC_DOWNLOAD_ICO:
			OnBnClickedChangeView();
			break;
		case MP_VIEW1_SPLIT_WINDOW:
			OnBnClickedDownUploads();
			break;
		case MP_VIEW1_DOWNLOADS:
			ShowList(IDC_DOWNLOADLIST);
			break;
		case MP_VIEW1_UPLOADING:
			ShowList(IDC_UPLOADLIST);
			break;
		case MP_VIEW1_DOWNLOADING:
			ShowList(IDC_DOWNLOADCLIENTS);
			break;
		case MP_VIEW1_ONQUEUE:
			ShowList(IDC_QUEUELIST);
			break;
		case MP_VIEW1_CLIENTS:
			ShowList(IDC_CLIENTLIST);
			break;

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

void CTransferWnd::UpdateCatTabTitles(bool force)
{
	CPoint pt;
	::GetCursorPos(&pt);
	if (!force && GetTabUnderMouse(&pt)!=-1)		// avoid cat tooltip jumping
		return;

	for (uint8 i=0;i<m_dlTab.GetItemCount();i++)
		//MORPH START - Changed by SiRoB, Due to Khaos Category
		//EditCatTabLabel(i,/*(i==0)? GetCatTitle( thePrefs.GetCategory(0)->filter ):*/thePrefs.GetCategory(i)->title);
		EditCatTabLabel(i, thePrefs.GetCategory(i)->title);
		//MORPH END   - Changed by SiRoB, Due to Khaos Category
}

void CTransferWnd::EditCatTabLabel(int i)
{
	//MORPH	- Changed by SiRoB, Khaos Category
	//	EditCatTabLabel(i,/*(i==0)? GetCatTitle( thePrefs.GetAllcatType() ):*/thePrefs.GetCategory(i)->title);
	EditCatTabLabel(i,thePrefs.GetCategory(i)->title);
}

void CTransferWnd::EditCatTabLabel(int index,CString newlabel)
{
	TCITEM tabitem;
	tabitem.mask = TCIF_PARAM;
	m_dlTab.GetItem(index,&tabitem);
	tabitem.mask = TCIF_TEXT;

	newlabel.Replace(_T("&"),_T("&&"));

	//MORPH - Removed by SIROB, Due to Khaos Cat
	/*
	if (!index)
		newlabel.Empty();

	if (!index || (index && thePrefs.GetCatFilter(index)>0)) {

		if (index)
			newlabel.Append(_T(" (")) ;
		
		if (thePrefs.GetCatFilterNeg(index))
			newlabel.Append(_T("!"));			
 
		if (thePrefs.GetCatFilter(index)==18)
			newlabel.Append( _T("\"") + thePrefs.GetCategory(index)->regexp + _T("\"") );
		else
        	newlabel.Append( GetCatTitle(thePrefs.GetCatFilter(index)));

		if (index)
			newlabel.Append( _T(")") );
	}
	*/

	int count,dwl;
	if (thePrefs.ShowCatTabInfos()) {
		CPartFile* cur_file;
		count=dwl=0;
		for (int i=0;i<theApp.downloadqueue->GetFileCount();i++) {
			cur_file=theApp.downloadqueue->GetFileByIndex(i);
			if (cur_file==0) continue;
			if (cur_file->CheckShowItemInGivenCat(index)) {
				if (cur_file->GetTransferringSrcCount()>0) ++dwl;
			}
		}
		CString title=newlabel;
		theApp.emuledlg->transferwnd->downloadlistctrl.GetCompleteDownloads(index,count);
		newlabel.Format(_T("%s %i/%i"),title,dwl,count);
	}

	tabitem.pszText = newlabel.LockBuffer();
	m_dlTab.SetItem(index,&tabitem);
	newlabel.UnlockBuffer();

	VerifyCatTabSize();
}

int CTransferWnd::AddCategory(CString newtitle,CString newincoming,CString newcomment, CString newautocat, bool addTab){
	Category_Struct* newcat=new Category_Struct;

	_stprintf(newcat->title,newtitle);
	newcat->prio=PR_NORMAL;
	// khaos::kmod+ Category Advanced A4AF Mode and Auto Cat
	newcat->iAdvA4AFMode = 0;
	// khaos::kmod-
	_stprintf(newcat->incomingpath,newincoming);
	_stprintf(newcat->comment,newcomment);
	/*
	newcat->regexp.Empty();
	newcat->ac_regexpeval=false;
	newcat->autocat=newautocat;//replaced by viewfilters.sAdvancedFilterMask
	newcat->filter=0;
	newcat->filterNeg=false;
	newcat->care4all=false;
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

int CTransferWnd::GetTabUnderMouse(CPoint* point)
{
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
	else
		return -1;
}

void CTransferWnd::OnLvnKeydownDownloadlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	int iItem = downloadlistctrl.GetSelectionMark();
	if (iItem != -1)
	{
		bool bAltKey = GetAsyncKeyState(VK_MENU) < 0;
		int iAction = EXPAND_COLLAPSE;
		if (pLVKeyDow->wVKey==VK_ADD || (bAltKey && pLVKeyDow->wVKey==VK_RIGHT))
			iAction = EXPAND_ONLY;
		else if (pLVKeyDow->wVKey==VK_SUBTRACT || (bAltKey && pLVKeyDow->wVKey==VK_LEFT))
			iAction = COLLAPSE_ONLY;
		if (iAction < EXPAND_COLLAPSE)
			downloadlistctrl.ExpandCollapseItem(iItem, iAction, true);
	}
	*pResult = 0;
}

void CTransferWnd::UpdateTabToolTips(int tab)
{
	if (tab == -1)
	{
		for (int i = 0; i < m_tooltipCats.GetToolCount(); i++)
			m_tooltipCats.DelTool(&m_dlTab,i+1);

		for (int i = 0; i < m_dlTab.GetItemCount(); i++)
		{
			CRect r;
			m_dlTab.GetItemRect(i, &r);
			VERIFY(m_tooltipCats.AddTool(&m_dlTab, GetTabStatistic(i), &r, i+1));
		}
	}
	else
	{
		CRect r;
		m_dlTab.GetItemRect(tab, &r);
		m_tooltipCats.DelTool(&m_dlTab,tab+1);
		VERIFY(m_tooltipCats.AddTool(&m_dlTab, GetTabStatistic(tab), &r, tab+1));
	}
}

CString CTransferWnd::GetTabStatistic(uint8 tab)
{
	uint16 count, dwl, err, paus;
	count = dwl = err = paus = 0;
	float speed=0;
	uint64 size=0;
	uint64 trsize=0;
	uint64 disksize=0;
	for (int i = 0; i < theApp.downloadqueue->GetFileCount(); i++)
	{
		/*const*/ CPartFile* cur_file = theApp.downloadqueue->GetFileByIndex(i);
		if (cur_file == 0)
			continue;
		if (cur_file->CheckShowItemInGivenCat(tab))
		{
			count++;
			if (cur_file->GetTransferringSrcCount() > 0)
				dwl++;
			speed += cur_file->GetDatarate() / 1024.0F;
			size+=cur_file->GetFileSize();
			trsize+=cur_file->GetCompletedSize();
			disksize+=cur_file->GetRealFileSize();
			if (cur_file->GetStatus() == PS_ERROR)
				err++;
			if (cur_file->GetStatus() == PS_PAUSED)
				paus++;
		}
	}

	int total;
	int compl = theApp.emuledlg->transferwnd->downloadlistctrl.GetCompleteDownloads(tab, total);

    CString prio;
    switch (thePrefs.GetCategory(tab)->prio)
	{
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

	CString title;
	title.Format(_T("%s: %i\n\n%s: %i\n%s: %i\n%s: %i\n%s: %i\n\n%s: %s\n\n%s: %.1f %s\n%s: %s/%s\n%s%s"),
		GetResString(IDS_FILES), count+compl,
		GetResString(IDS_DOWNLOADING), dwl,
		GetResString(IDS_PAUSED) ,paus,
		GetResString(IDS_ERRORLIKE) ,err,
		GetResString(IDS_DL_TRANSFCOMPL) ,compl,
        GetResString(IDS_PRIORITY), prio,
		GetResString(IDS_DL_SPEED) ,speed,GetResString(IDS_KBYTESEC),
		GetResString(IDS_DL_SIZE),CastItoXBytes(trsize, false, false),CastItoXBytes(size, false, false),
		GetResString(IDS_ONDISK),CastItoXBytes(disksize, false, false));
	return title;
}


void CTransferWnd::OnDblclickDltab()
{
	POINT point;
	::GetCursorPos(&point);
	CPoint pt(point);
	int tab=GetTabUnderMouse(&pt);
	if (tab < 1)
		return;
	rightclickindex=tab;
	OnCommand(MP_CAT_EDIT,0);
}

void CTransferWnd::OnTabMovement(NMHDR *pNMHDR, LRESULT *pResult)
{
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

void CTransferWnd::VerifyCatTabSize(bool _forceverify)
{
	//MORPH - Added by SiRoB, Show/Hide dlTab
	// MightyKnife: Forcing of the verification added
	if ((m_dwShowListIDC != IDC_DOWNLOADLIST && m_dwShowListIDC != IDC_UPLOADLIST + IDC_DOWNLOADLIST) && (!_forceverify))
		return;
	// [end] Mighty Knife
	//MORPH - Added by SiRoB, Show/Hide dlTab
			
	int size=0;
	for (int i = 0; i < m_dlTab.GetItemCount(); i++)
	{
		CRect rect;
		m_dlTab.GetItemRect(i,&rect);
		size+= rect.Width();
	}
	size += 4;

	int right;
	WINDOWPLACEMENT wp;
	downloadlistctrl.GetWindowPlacement(&wp);
	right = wp.rcNormalPosition.right;
	m_dlTab.GetWindowPlacement(&wp);
	if (wp.rcNormalPosition.right < 0)
		return;
	wp.rcNormalPosition.right = right;

	int left = wp.rcNormalPosition.right - size;
	CRect rcBtnWnd1;
	m_btnWnd1.GetWindowRect(rcBtnWnd1);
	ScreenToClient(rcBtnWnd1);
	if (left < rcBtnWnd1.right + 10)
		left = rcBtnWnd1.right + 10;
	wp.rcNormalPosition.left = left;

	RemoveAnchor(m_dlTab);
	m_dlTab.SetWindowPlacement(&wp);
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
		case 8 : return GetResString(IDS_SEENCOMPL);
		case 10 : return GetResString(IDS_VIDEO);
		case 11 : return GetResString(IDS_AUDIO);
		case 12 : return GetResString(IDS_SEARCH_ARC);
		case 13 : return GetResString(IDS_SEARCH_CDIMG);
		case 14 : return GetResString(IDS_SEARCH_DOC);
		case 15 : return GetResString(IDS_SEARCH_PICS);
		case 16 : return GetResString(IDS_SEARCH_PRG);
//		case 18 : return GetResString(IDS_REGEXPRESSION);
	}
	return _T("?");
}
*/

void CTransferWnd::OnBnClickedChangeView()
{
	switch (m_dwShowListIDC)
	{
		case IDC_DOWNLOADLIST:
		ShowList(IDC_UPLOADLIST);
			break;
		case IDC_UPLOADLIST:
			  ShowList(IDC_DOWNLOADCLIENTS);
			break;
		case IDC_DOWNLOADCLIENTS:
			if (!thePrefs.IsQueueListDisabled()){
			ShowList(IDC_QUEUELIST);
				break;
			}
		case IDC_QUEUELIST:
			if (!thePrefs.IsKnownClientListDisabled()){
			ShowList(IDC_CLIENTLIST);
				break;
			}
		case IDC_CLIENTLIST:
		OnBnClickedDownUploads();
			break;
		case IDC_UPLOADLIST + IDC_DOWNLOADLIST:
		ShowList(IDC_DOWNLOADLIST);
			break;
	}
}

void CTransferWnd::ChangeDlIcon(EWnd1Icon iIcon)
{
	TBBUTTONINFO tbbi = {0};
	tbbi.cbSize = sizeof tbbi;
	tbbi.dwMask = TBIF_IMAGE;
	tbbi.iImage = iIcon;
	m_btnWnd1.SetButtonInfo(GetWindowLong(m_btnWnd1, GWL_ID), &tbbi);
}

void CTransferWnd::ShowList(uint32 dwListIDC)
{
	CRect rcWnd;
	GetWindowRect(rcWnd);
	ScreenToClient(rcWnd);

	CRect rcDown;
	GetDlgItem(dwListIDC)->GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.bottom = rcWnd.bottom - 20;
	rcDown.top=28;
	m_wndSplitter.DestroyWindow();
	RemoveAnchor(dwListIDC);
	m_btnWnd2.ShowWindow(SW_HIDE);

	m_dwShowListIDC = dwListIDC;
	uploadlistctrl.ShowWindow((m_dwShowListIDC == IDC_UPLOADLIST) ? SW_SHOW : SW_HIDE);
	queuelistctrl.ShowWindow((m_dwShowListIDC == IDC_QUEUELIST) ? SW_SHOW : SW_HIDE);
	downloadclientsctrl.ShowWindow((m_dwShowListIDC == IDC_DOWNLOADCLIENTS) ? SW_SHOW : SW_HIDE);
	clientlistctrl.ShowWindow((m_dwShowListIDC == IDC_CLIENTLIST) ? SW_SHOW : SW_HIDE);
	downloadlistctrl.ShowWindow((m_dwShowListIDC == IDC_DOWNLOADLIST) ? SW_SHOW : SW_HIDE);
	m_dlTab.ShowWindow((m_dwShowListIDC == IDC_DOWNLOADLIST) ? SW_SHOW : SW_HIDE);

	GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow((m_dwShowListIDC == IDC_QUEUELIST) ? SW_SHOW : SW_HIDE);

	switch (dwListIDC)
	{
		case IDC_DOWNLOADLIST:
			downloadlistctrl.MoveWindow(rcDown);
			downloadlistctrl.ShowFilesCount();
			m_btnWnd1.CheckButton(MP_VIEW1_DOWNLOADS);
			ChangeDlIcon(w1iDownloadFiles);
			break;
		case IDC_UPLOADLIST:
			uploadlistctrl.MoveWindow(rcDown);
			UpdateListCount(wnd2Uploading);
			m_btnWnd1.CheckButton(MP_VIEW1_UPLOADING);
			ChangeDlIcon(w1iUploading);
			break;
		case IDC_QUEUELIST:
			queuelistctrl.MoveWindow(rcDown);
			UpdateListCount(wnd2OnQueue);
			m_btnWnd1.CheckButton(MP_VIEW1_ONQUEUE);
			ChangeDlIcon(w1iOnQueue);
			break;
		case IDC_DOWNLOADCLIENTS:
			downloadclientsctrl.MoveWindow(rcDown);
			UpdateListCount(wnd2Downloading);
			m_btnWnd1.CheckButton(MP_VIEW1_DOWNLOADING);
			ChangeDlIcon(w1iDownloading);
			break;
		case IDC_CLIENTLIST:
			clientlistctrl.MoveWindow(rcDown);
			UpdateListCount(wnd2Clients);
			m_btnWnd1.CheckButton(MP_VIEW1_CLIENTS);
			ChangeDlIcon(w1iClientsKnown);
			break;
	}
	AddAnchor(dwListIDC, TOP_LEFT, BOTTOM_RIGHT);
}

void CTransferWnd::OnBnClickedDownUploads(bool bReDraw) 
{
	m_dlTab.ShowWindow(SW_SHOW);
	if (!bReDraw && m_dwShowListIDC == IDC_DOWNLOADLIST + IDC_UPLOADLIST)
		return;

	m_btnWnd1.CheckButton(MP_VIEW1_SPLIT_WINDOW);
	ChangeDlIcon(w1iDownloadFiles);

	CRect rcWnd;
	GetWindowRect(rcWnd);
	ScreenToClient(rcWnd);

	// [Comment by Mighty Knife:]
	//
	// thePrefs.GetSplitterbarPosition() retrieves the SplitterBar position.
	// This is a percentage value (0-100%) of height of the whole window.
	// Therefore the "splitpos" variable calculates the vertical pixel-position
	// of the SplitterBar relative to the height of the whole window, i.e.
	// the Transfer-window!
	LONG splitpos = (thePrefs.GetSplitterbarPosition() * rcWnd.Height()) / 100;

	CRect rcDown;
	downloadlistctrl.GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.right = rcWnd.right - 7;
	rcDown.bottom = splitpos + 5;

	// [Comment by Mighty Knife:]
	// In UpdateSplitterRange we used this downloadlist here to recalculate the
	// percentage splitter position from the current absolute pixel position
	// on the screen. We added 5 pixel here to the splitpos to get the bottom
	// position of the downloadlist. So in UpdateSplitterRange we have to remember
	// subtracting these 5 pixel again!
	downloadlistctrl.MoveWindow(rcDown);

	uploadlistctrl.GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.right = rcWnd.right - 7;
	rcDown.bottom = rcWnd.bottom - 20;
	rcDown.top = splitpos + 30;
	uploadlistctrl.MoveWindow(rcDown);

	queuelistctrl.GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.right = rcWnd.right - 7;
	rcDown.bottom = rcWnd.bottom - 20;
	rcDown.top = splitpos + 30;
	queuelistctrl.MoveWindow(rcDown);

	clientlistctrl.GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.right = rcWnd.right - 7;
	rcDown.bottom = rcWnd.bottom - 20;
	rcDown.top = splitpos + 30;
	clientlistctrl.MoveWindow(rcDown);

	downloadclientsctrl.GetWindowRect(rcDown);
	ScreenToClient(rcDown);
	rcDown.right = rcWnd.right - 7;
	rcDown.bottom = rcWnd.bottom - 20;
	rcDown.top = splitpos + 30;
	downloadclientsctrl.MoveWindow(rcDown);

	CRect rcSpl;
	rcSpl.left = WND2_BUTTON_XOFF + WND2_BUTTON_WIDTH + 8;
	rcSpl.right = rcDown.right;
	rcSpl.top = splitpos + WND_SPLITTER_YOFF;
	rcSpl.bottom = rcSpl.top + WND_SPLITTER_HEIGHT;
	if (!m_wndSplitter)
		m_wndSplitter.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER);
	else
		m_wndSplitter.MoveWindow(rcSpl, TRUE);
	DoResize(0);

	m_dwShowListIDC = IDC_DOWNLOADLIST + IDC_UPLOADLIST;
	downloadlistctrl.ShowFilesCount();
	m_btnWnd2.ShowWindow(SW_SHOW);

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
	uploadlistctrl.ShowWindow((m_uWnd2 == wnd2Uploading) ? SW_SHOW : SW_HIDE);
	queuelistctrl.ShowWindow((m_uWnd2 == wnd2OnQueue) ? SW_SHOW : SW_HIDE);
	downloadclientsctrl.ShowWindow((m_uWnd2 == wnd2Downloading) ? SW_SHOW : SW_HIDE);
	clientlistctrl.ShowWindow((m_uWnd2 == wnd2Clients) ? SW_SHOW : SW_HIDE);

	GetDlgItem(IDC_QUEUE_REFRESH_BUTTON)->ShowWindow((m_uWnd2 == wnd2OnQueue) ? SW_SHOW : SW_HIDE);

	UpdateListCount(m_uWnd2);
}

void CTransferWnd::OnWnd1BtnDropDown(NMHDR *pNMHDR, LRESULT *pResult)
{
	CTitleMenu menu;
	menu.CreatePopupMenu();
	menu.EnableIcons();

	menu.AppendMenu(MF_STRING | (m_dwShowListIDC == IDC_DOWNLOADLIST + IDC_UPLOADLIST ? MF_GRAYED : 0), MP_VIEW1_SPLIT_WINDOW, GetResString(IDS_SPLIT_WINDOW), _T("SplitWindow"));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING | (m_dwShowListIDC == IDC_DOWNLOADLIST ? MF_GRAYED : 0), MP_VIEW1_DOWNLOADS, GetResString(IDS_TW_DOWNLOADS), _T("DownloadFiles"));
	menu.AppendMenu(MF_STRING | (m_dwShowListIDC == IDC_UPLOADLIST ? MF_GRAYED : 0), MP_VIEW1_UPLOADING, GetResString(IDS_UPLOADING), _T("Upload"));
	menu.AppendMenu(MF_STRING | (m_dwShowListIDC == IDC_DOWNLOADCLIENTS ? MF_GRAYED : 0), MP_VIEW1_DOWNLOADING, GetResString(IDS_DOWNLOADING), _T("Download"));
	if (!thePrefs.IsQueueListDisabled())
		menu.AppendMenu(MF_STRING | (m_dwShowListIDC == IDC_QUEUELIST ? MF_GRAYED : 0), MP_VIEW1_ONQUEUE, GetResString(IDS_ONQUEUE), _T("ClientsOnQueue"));
	if (!thePrefs.IsKnownClientListDisabled())
		menu.AppendMenu(MF_STRING | (m_dwShowListIDC == IDC_CLIENTLIST ? MF_GRAYED : 0), MP_VIEW1_CLIENTS, GetResString(IDS_CLIENTLIST), _T("ClientsKnown"));

	CRect rc;
	m_btnWnd1.GetWindowRect(&rc);
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, rc.left, rc.bottom, this);
}

void CTransferWnd::OnWnd2BtnDropDown(NMHDR *pNMHDR, LRESULT *pResult)
{
	CTitleMenu menu;
	menu.CreatePopupMenu();
	menu.EnableIcons();

	menu.AppendMenu(MF_STRING | (m_uWnd2 == wnd2Uploading ? MF_GRAYED : 0), MP_VIEW2_UPLOADING, GetResString(IDS_UPLOADING), _T("Upload"));
	menu.AppendMenu(MF_STRING | (m_uWnd2 == wnd2Downloading ? MF_GRAYED : 0), MP_VIEW2_DOWNLOADING, GetResString(IDS_DOWNLOADING), _T("Download"));
	if (!thePrefs.IsQueueListDisabled())
		menu.AppendMenu(MF_STRING | (m_uWnd2 == wnd2OnQueue ? MF_GRAYED : 0), MP_VIEW2_ONQUEUE, GetResString(IDS_ONQUEUE), _T("ClientsOnQueue"));
	if (!thePrefs.IsKnownClientListDisabled())
		menu.AppendMenu(MF_STRING | (m_uWnd2 == wnd2Clients ? MF_GRAYED : 0), MP_VIEW2_CLIENTS, GetResString(IDS_CLIENTLIST), _T("ClientsKnown"));

	CRect rc;
	m_btnWnd2.GetWindowRect(&rc);
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, rc.left, rc.bottom, this);
}

void CTransferWnd::ResetTransToolbar(bool bShowToolbar, bool bResetLists){
	if (m_btnWnd1.m_hWnd){
		RemoveAnchor(m_btnWnd1);
		m_btnWnd1.DestroyWindow();
	}
	CRect rc;
	rc.top = 5;
	rc.left = WND1_BUTTON_XOFF;
	rc.right = rc.left + WND1_BUTTON_WIDTH + (bShowToolbar ? NUM_WINA_BUTTONS*27 : 0);
	rc.bottom = rc.top + WND1_BUTTON_HEIGHT;
	m_btnWnd1.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP, rc, this, IDC_DOWNLOAD_ICO, !bShowToolbar);
	SetWnd1Icons();
	if (bShowToolbar)
	{
		m_btnWnd1.ModifyStyle(0, TBSTYLE_TOOLTIPS);
		m_btnWnd1.SetExtendedStyle(m_btnWnd1.GetExtendedStyle() | TBSTYLE_EX_MIXEDBUTTONS);

		TBBUTTONINFO tbbi = {0};
		tbbi.cbSize = sizeof tbbi;
		tbbi.dwMask = TBIF_SIZE | TBIF_STYLE;
		tbbi.cx = WND1_BUTTON_WIDTH;
		tbbi.fsStyle = BTNS_BUTTON | BTNS_SHOWTEXT;
		m_btnWnd1.SetButtonInfo(IDC_DOWNLOAD_ICO, &tbbi);

		TBBUTTON atb[NUM_WINA_BUTTONS] = {0};
		atb[0].iBitmap = w1iSplitWindow;
		atb[0].idCommand = MP_VIEW1_SPLIT_WINDOW;
		atb[0].fsState = TBSTATE_ENABLED;
		atb[0].fsStyle = BTNS_BUTTON | BTNS_CHECKGROUP | BTNS_AUTOSIZE;
		atb[0].iString = -1;

		atb[1].iBitmap = w1iDownloadFiles;
		atb[1].idCommand = MP_VIEW1_DOWNLOADS;
		atb[1].fsState = TBSTATE_ENABLED;
		atb[1].fsStyle = BTNS_BUTTON | BTNS_CHECKGROUP | BTNS_AUTOSIZE;
		atb[1].iString = -1;

		atb[2].iBitmap = w1iUploading;
		atb[2].idCommand = MP_VIEW1_UPLOADING;
		atb[2].fsState = TBSTATE_ENABLED;
		atb[2].fsStyle = BTNS_BUTTON | BTNS_CHECKGROUP | BTNS_AUTOSIZE;
		atb[2].iString = -1;

		atb[3].iBitmap = w1iDownloading;
		atb[3].idCommand = MP_VIEW1_DOWNLOADING;
		atb[3].fsState = TBSTATE_ENABLED;
		atb[3].fsStyle = BTNS_BUTTON | BTNS_CHECKGROUP | BTNS_AUTOSIZE;
		atb[3].iString = -1;

		atb[4].iBitmap = w1iOnQueue;
		atb[4].idCommand = MP_VIEW1_ONQUEUE;
		atb[4].fsState = thePrefs.IsQueueListDisabled() ? 0 : TBSTATE_ENABLED;
		atb[4].fsStyle = BTNS_BUTTON | BTNS_CHECKGROUP | BTNS_AUTOSIZE;
		atb[4].iString = -1;

		atb[5].iBitmap = w1iClientsKnown;
		atb[5].idCommand = MP_VIEW1_CLIENTS;
		atb[5].fsState = thePrefs.IsKnownClientListDisabled() ? 0 : TBSTATE_ENABLED;
		atb[5].fsStyle = BTNS_BUTTON | BTNS_CHECKGROUP | BTNS_AUTOSIZE;
		atb[5].iString = -1;
		m_btnWnd1.AddButtons(ARRSIZE(atb), atb);

		for (int i = 0; i < ARRSIZE(atb); i++)
		{
			tbbi.dwMask = TBIF_SIZE | TBIF_BYINDEX;
			tbbi.cx = 4+16+4;
			m_btnWnd1.SetButtonInfo(i + 1, &tbbi);
		}

		CSize size;
		m_btnWnd1.GetMaxSize(&size);
		m_btnWnd1.GetWindowRect(&rc);
		ScreenToClient(&rc);
		m_btnWnd1.MoveWindow(rc.left, rc.top, size.cx, rc.Height());
	}
	AddAnchor(m_btnWnd1, TOP_LEFT);
	if (bResetLists){
		OnBnClickedDownUploads(true);
	}
}
