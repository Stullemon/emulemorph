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
#include "emuleDlg.h"
#include "SharedFilesWnd.h"
#include "OtherFunctions.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "KnownFile.h"
#include "UserMsgs.h"
#include "HelpIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define	SPLITTER_RANGE_MIN		100
#define	SPLITTER_RANGE_MAX		350

#define	SPLITTER_MARGIN			0
#define	SPLITTER_WIDTH			4


// CSharedFilesWnd dialog

IMPLEMENT_DYNAMIC(CSharedFilesWnd, CDialog)

BEGIN_MESSAGE_MAP(CSharedFilesWnd, CResizableDialog)
	ON_BN_CLICKED(IDC_RELOADSHAREDFILES, OnBnClickedReloadSharedFiles)
	ON_NOTIFY(LVN_ITEMACTIVATE, IDC_SFLIST, OnLvnItemActivateSharedFiles)
	ON_NOTIFY(NM_CLICK, IDC_SFLIST, OnNMClickSharedFiles)
	ON_WM_SYSCOLORCHANGE()
	ON_WM_CTLCOLOR()
	ON_STN_DBLCLK(IDC_FILES_ICO, OnStnDblClickFilesIco)
	ON_NOTIFY(TVN_SELCHANGED, IDC_SHAREDDIRSTREE, OnTvnSelChangedSharedDirsTree)
	ON_WM_SIZE()
	ON_MESSAGE(UM_DELAYED_EVALUATE, OnChangeFilter)
	ON_WM_HELPINFO()
	//MORPH START - Added, Downloaded History [Monki/Xman]
#ifndef NO_HISTORY
	ON_NOTIFY(LVN_ITEMACTIVATE, IDC_DOWNHISTORYLIST, OnLvnItemActivateHistorylist)
	ON_NOTIFY(NM_CLICK, IDC_DOWNHISTORYLIST, OnNMClickHistorylist)
	ON_WM_SHOWWINDOW() 
#endif
	//MORPH END   - Added, Downloaded History [Monki/Xman]
END_MESSAGE_MAP()

CSharedFilesWnd::CSharedFilesWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CSharedFilesWnd::IDD, pParent)
{
	icon_files = NULL;
	m_nFilterColumn = 0;
}

CSharedFilesWnd::~CSharedFilesWnd()
{
	m_ctlSharedListHeader.Detach();
	if (icon_files)
		VERIFY( DestroyIcon(icon_files) );
}

void CSharedFilesWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SFLIST, sharedfilesctrl);
	DDX_Control(pDX, IDC_POPBAR, pop_bar);
	DDX_Control(pDX, IDC_POPBAR2, pop_baraccept);
	DDX_Control(pDX, IDC_POPBAR3, pop_bartrans);
	DDX_Control(pDX, IDC_STATISTICS, m_ctrlStatisticsFrm);
	DDX_Control(pDX, IDC_SHAREDDIRSTREE, m_ctlSharedDirTree);
	DDX_Control(pDX, IDC_SHAREDFILES_FILTER, m_ctlFilter);
	//MORPH START - Added, Downloaded History [Monki/Xman]
#ifndef NO_HISTORY
	DDX_Control(pDX, IDC_DOWNHISTORYLIST, historylistctrl);
#endif
	//MORPH END   - Added, Downloaded History [Monki/Xman]
}

BOOL CSharedFilesWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);
	SetAllIcons();
	sharedfilesctrl.Init();
	m_ctlSharedDirTree.Initalize(&sharedfilesctrl);
	if (thePrefs.GetUseSystemFontForMainControls())
		m_ctlSharedDirTree.SendMessage(WM_SETFONT, NULL, FALSE);

	m_ctlSharedListHeader.Attach(sharedfilesctrl.GetHeaderCtrl()->Detach());
	CArray<int, int> aIgnore; // ignored no-text columns for filter edit
	aIgnore.Add(8); // shared parts
	aIgnore.Add(11); // shared ed2k/kad
	m_ctlFilter.OnInit(&m_ctlSharedListHeader, &aIgnore);
	
	pop_bar.SetGradientColors(RGB(255,255,240),RGB(255,255,0));
	pop_bar.SetTextColor(RGB(20,70,255));
	pop_baraccept.SetGradientColors(RGB(255,255,240),RGB(255,255,0));
	pop_baraccept.SetTextColor(RGB(20,70,255));
	pop_bartrans.SetGradientColors(RGB(255,255,240),RGB(255,255,0));
	pop_bartrans.SetTextColor(RGB(20,70,255));

	CRect rcSpl;
	m_ctlSharedDirTree.GetWindowRect(rcSpl);
	ScreenToClient(rcSpl);
	rcSpl.left = rcSpl.right + SPLITTER_MARGIN;
	rcSpl.right = rcSpl.left + SPLITTER_WIDTH;
	m_wndSplitter.Create(WS_CHILD | WS_VISIBLE, rcSpl, this, IDC_SPLITTER_SHAREDFILES);

	AddAnchor(m_wndSplitter, TOP_LEFT);
	AddAnchor(sharedfilesctrl, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(m_ctrlStatisticsFrm, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CURSESSION_LBL, BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC4, BOTTOM_LEFT);
	AddAnchor(IDC_SREQUESTED, BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC5, BOTTOM_LEFT);
	AddAnchor(IDC_SACCEPTED, BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC6, BOTTOM_LEFT);
	AddAnchor(IDC_STRANSFERRED, BOTTOM_LEFT);
	AddAnchor(m_ctlSharedDirTree, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(pop_bar, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(pop_baraccept, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(pop_bartrans, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(m_ctlFilter, TOP_LEFT);
	AddAnchor(IDC_FILES_ICO, TOP_LEFT);
	AddAnchor(IDC_RELOADSHAREDFILES, TOP_RIGHT);
	AddAnchor(IDC_TOTAL_LBL, BOTTOM_RIGHT);
	AddAnchor(IDC_SREQUESTED2,BOTTOM_RIGHT);
	AddAnchor(IDC_FSTATIC7,BOTTOM_RIGHT);
	AddAnchor(IDC_FSTATIC8,BOTTOM_RIGHT);
	AddAnchor(IDC_FSTATIC9,BOTTOM_RIGHT);
	AddAnchor(IDC_STRANSFERRED2,BOTTOM_RIGHT);
	AddAnchor(IDC_SACCEPTED2,BOTTOM_RIGHT);
	AddAnchor(IDC_TRAFFIC_TEXT, TOP_LEFT);
	//MORPH START - Added, Downloaded History [Monki/Xman]
#ifndef NO_HISTORY
	{
		CRect tmpRect;
		sharedfilesctrl.GetWindowRect(tmpRect);
		ScreenToClient(tmpRect);
		historylistctrl.MoveWindow(tmpRect,true);
		historylistctrl.ShowWindow(false);
		//historylistctrl.Init(); //Xman SLUGFILLER: SafeHash - moved
		AddAnchor(IDC_DOWNHISTORYLIST,TOP_LEFT,BOTTOM_RIGHT);
	}
#else
	GetDlgItem(IDC_DOWNHISTORYLIST)->ShowWindow(SW_HIDE); //Fafner: otherwise I see the not used control - 080731
#endif
	//MORPH END   - Added, Downloaded History [Monki/Xman]

	int iPosStatInit = rcSpl.left;
	int iPosStatNew = thePrefs.GetSplitterbarPositionShared();
	if (iPosStatNew > SPLITTER_RANGE_MAX)
		iPosStatNew = SPLITTER_RANGE_MAX;
	else if (iPosStatNew < SPLITTER_RANGE_MIN)
		iPosStatNew = SPLITTER_RANGE_MIN;
	rcSpl.left = iPosStatNew;
	rcSpl.right = iPosStatNew + SPLITTER_WIDTH;
	if (iPosStatNew != iPosStatInit)
	{
		m_wndSplitter.MoveWindow(rcSpl);
		DoResize(iPosStatNew - iPosStatInit);
	}

	Localize();

	GetDlgItem(IDC_CURSESSION_LBL)->SetFont(&theApp.m_fontDefaultBold);
	GetDlgItem(IDC_TOTAL_LBL)->SetFont(&theApp.m_fontDefaultBold);

	return TRUE;
}

void CSharedFilesWnd::DoResize(int iDelta)
{
	CSplitterControl::ChangeWidth(&m_ctlSharedDirTree, iDelta);
	CSplitterControl::ChangeWidth(&m_ctlFilter, iDelta);
	CSplitterControl::ChangePos(GetDlgItem(IDC_CURSESSION_LBL), -iDelta, 0);
	CSplitterControl::ChangePos(GetDlgItem(IDC_FSTATIC4), -iDelta, 0);
	CSplitterControl::ChangePos(GetDlgItem(IDC_SREQUESTED), -iDelta, 0);
	CSplitterControl::ChangePos(GetDlgItem(IDC_FSTATIC5), -iDelta, 0);
	CSplitterControl::ChangePos(GetDlgItem(IDC_SACCEPTED), -iDelta, 0);
	CSplitterControl::ChangePos(GetDlgItem(IDC_FSTATIC6), -iDelta, 0);
	CSplitterControl::ChangePos(GetDlgItem(IDC_STRANSFERRED), -iDelta, 0);
	CSplitterControl::ChangePos(&m_ctrlStatisticsFrm, -iDelta, 0);
	CSplitterControl::ChangePos(&sharedfilesctrl, -iDelta, 0);
	CSplitterControl::ChangeWidth(&m_ctrlStatisticsFrm, -iDelta);
	CSplitterControl::ChangeWidth(&sharedfilesctrl, -iDelta);
	//MORPH START - Added, Downloaded History [Monki/Xman]
#ifndef NO_HISTORY
	CSplitterControl::ChangePos(GetDlgItem(IDC_DOWNHISTORYLIST), -iDelta, 0);
	CSplitterControl::ChangeWidth(GetDlgItem(IDC_DOWNHISTORYLIST), -iDelta);
#endif
	//MORPH END   - Added, Downloaded History [Monki/Xman]
	CSplitterControl::ChangePos(&pop_bar, -iDelta, 0);
	CSplitterControl::ChangePos(&pop_baraccept, -iDelta, 0);
	CSplitterControl::ChangePos(&pop_bartrans, -iDelta, 0);
	CSplitterControl::ChangeWidth(&pop_bar, -iDelta);
	CSplitterControl::ChangeWidth(&pop_baraccept, -iDelta);
	CSplitterControl::ChangeWidth(&pop_bartrans, -iDelta);

	CRect rcSpl;
	m_wndSplitter.GetWindowRect(rcSpl);
	ScreenToClient(rcSpl);
	thePrefs.SetSplitterbarPositionShared(rcSpl.left);

	RemoveAnchor(m_wndSplitter);
	AddAnchor(m_wndSplitter, TOP_LEFT);

	//MORPH START - Added, Downloaded History [Monki/Xman]
#ifndef NO_HISTORY
	RemoveAnchor(IDC_DOWNHISTORYLIST);
#endif
	//MORPH END   - Added, Downloaded History [Monki/Xman]
	RemoveAnchor(sharedfilesctrl);
	RemoveAnchor(m_ctrlStatisticsFrm);
	RemoveAnchor(IDC_CURSESSION_LBL);
	RemoveAnchor(IDC_FSTATIC4);
	RemoveAnchor(IDC_SREQUESTED);
	RemoveAnchor(pop_bar);
	RemoveAnchor(IDC_FSTATIC5);
	RemoveAnchor(IDC_SACCEPTED);
	RemoveAnchor(pop_baraccept);
	RemoveAnchor(IDC_FSTATIC6);
	RemoveAnchor(IDC_STRANSFERRED);
	RemoveAnchor(pop_bartrans);
	RemoveAnchor(m_ctlSharedDirTree);
	RemoveAnchor(m_ctlFilter);

	//MORPH START - Added, Downloaded History [Monki/Xman]
#ifndef NO_HISTORY
	AddAnchor(IDC_DOWNHISTORYLIST,TOP_LEFT,BOTTOM_RIGHT);
#endif
	//MORPH END   - Added, Downloaded History [Monki/Xman]
	AddAnchor(sharedfilesctrl, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(m_ctrlStatisticsFrm, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_CURSESSION_LBL, BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC4, BOTTOM_LEFT);
	AddAnchor(IDC_SREQUESTED,BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC5,BOTTOM_LEFT);
	AddAnchor(IDC_SACCEPTED,BOTTOM_LEFT);
	AddAnchor(IDC_FSTATIC6,BOTTOM_LEFT);
	AddAnchor(IDC_STRANSFERRED,BOTTOM_LEFT);
	AddAnchor(m_ctlSharedDirTree, TOP_LEFT, BOTTOM_LEFT);
	AddAnchor(pop_bar, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(pop_baraccept, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(pop_bartrans, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(m_ctlFilter, TOP_LEFT);

	CRect rcWnd;
	GetWindowRect(rcWnd);
	ScreenToClient(rcWnd);
	m_wndSplitter.SetRange(rcWnd.left + SPLITTER_RANGE_MIN, rcWnd.left + SPLITTER_RANGE_MAX);

	Invalidate();
	UpdateWindow();
}


void CSharedFilesWnd::Reload()
{	
	sharedfilesctrl.SetDirectoryFilter(NULL, false);
	m_ctlSharedDirTree.Reload();
	sharedfilesctrl.SetDirectoryFilter(m_ctlSharedDirTree.GetSelectedFilter(), false);
	theApp.sharedfiles->Reload();

	ShowSelectedFilesSummary();
}

void CSharedFilesWnd::OnStnDblClickFilesIco()
{
	theApp.emuledlg->ShowPreferences(IDD_PPG_DIRECTORIES);
}

void CSharedFilesWnd::OnBnClickedReloadSharedFiles()
{
	CWaitCursor curWait;
	Reload();
}

void CSharedFilesWnd::OnLvnItemActivateSharedFiles(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	ShowSelectedFilesSummary();
}

//MORPH START - Changed, Downloaded History [Monki/Xman]
#ifdef NO_HISTORY
void CSharedFilesWnd::ShowSelectedFilesSummary()
#else
void CSharedFilesWnd::ShowSelectedFilesSummary(bool bHistory /*=false*/)
#endif
//MORPH END   - Changed, Downloaded History [Monki/Xman]
{
	const CKnownFile* pTheFile = NULL;
	int iFiles = 0;
	uint64 uTransferred = 0;
	UINT uRequests = 0;
	UINT uAccepted = 0;
	uint64 uAllTimeTransferred = 0;
	UINT uAllTimeRequests = 0;
	UINT uAllTimeAccepted = 0;
	//MORPH START - Changed, Downloaded History [Monki/Xman]
#ifdef NO_HISTORY
	POSITION pos = sharedfilesctrl.GetFirstSelectedItemPosition();
	while (pos)
	{
		int iItem = sharedfilesctrl.GetNextSelectedItem(pos);
		const CKnownFile* pFile = (CKnownFile*)sharedfilesctrl.GetItemData(iItem);
#else
	POSITION pos;
	if(bHistory){
		pos = historylistctrl.GetFirstSelectedItemPosition();
	}
	else{
		pos = sharedfilesctrl.GetFirstSelectedItemPosition();
	}
	while (pos)
	{
		int iItem;
		const CKnownFile* pFile;
		if(bHistory){
			iItem = historylistctrl.GetNextSelectedItem(pos);
			pFile = (CKnownFile*)historylistctrl.GetItemData(iItem);
		}
		else{
			iItem = sharedfilesctrl.GetNextSelectedItem(pos);
			pFile = (CKnownFile*)sharedfilesctrl.GetItemData(iItem);
		}
#endif
	//MORPH END   - Changed, Downloaded History [Monki/Xman]
		iFiles++;
		if (iFiles == 1)
			pTheFile = pFile;

		uTransferred += pFile->statistic.GetTransferred();
		uRequests += pFile->statistic.GetRequests();
		uAccepted += pFile->statistic.GetAccepts();

		uAllTimeTransferred += pFile->statistic.GetAllTimeTransferred();
		uAllTimeRequests += pFile->statistic.GetAllTimeRequests();
		uAllTimeAccepted += pFile->statistic.GetAllTimeAccepts();
	}

	if (iFiles != 0)
	{
		pop_bartrans.SetRange32(0, (int)(theApp.knownfiles->transferred/1024));
		pop_bartrans.SetPos((int)(uTransferred/1024));
		pop_bartrans.SetShowPercent();
		SetDlgItemText(IDC_STRANSFERRED, CastItoXBytes(uTransferred, false, false));

		pop_bar.SetRange32(0, theApp.knownfiles->requested);
		pop_bar.SetPos(uRequests);
		pop_bar.SetShowPercent();
		SetDlgItemInt(IDC_SREQUESTED, uRequests, FALSE);

		pop_baraccept.SetRange32(0, theApp.knownfiles->accepted);
		pop_baraccept.SetPos(uAccepted);
		pop_baraccept.SetShowPercent();
		SetDlgItemInt(IDC_SACCEPTED, uAccepted, FALSE);

		SetDlgItemText(IDC_STRANSFERRED2, CastItoXBytes(uAllTimeTransferred, false, false));
		SetDlgItemInt(IDC_SREQUESTED2, uAllTimeRequests, FALSE);
		SetDlgItemInt(IDC_SACCEPTED2, uAllTimeAccepted, FALSE);

		CString str(GetResString(IDS_SF_STATISTICS));
		if (iFiles == 1 && pTheFile != NULL)
			str += _T(" (") + MakeStringEscaped(pTheFile->GetFileName()) +_T(")");
		m_ctrlStatisticsFrm.SetWindowText(str);
	}
	else
	{
		pop_bartrans.SetRange32(0, 100);
		pop_bartrans.SetPos(0);
		pop_bartrans.SetTextFormat(_T(""));
		SetDlgItemText(IDC_STRANSFERRED, _T("-"));

		pop_bar.SetRange32(0, 100);
		pop_bar.SetPos(0);
		pop_bar.SetTextFormat(_T(""));
		SetDlgItemText(IDC_SREQUESTED, _T("-"));

		pop_baraccept.SetRange32(0, 100);
		pop_baraccept.SetPos(0);
		pop_baraccept.SetTextFormat(_T(""));
		SetDlgItemText(IDC_SACCEPTED, _T("-"));

		SetDlgItemText(IDC_STRANSFERRED2, _T("-"));
		SetDlgItemText(IDC_SREQUESTED2, _T("-"));
		SetDlgItemText(IDC_SACCEPTED2, _T("-"));

		m_ctrlStatisticsFrm.SetWindowText(GetResString(IDS_SF_STATISTICS));
	}
}

void CSharedFilesWnd::OnNMClickSharedFiles(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnLvnItemActivateSharedFiles(pNMHDR, pResult);
	*pResult = 0;
}

BOOL CSharedFilesWnd::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		// Don't handle Ctrl+Tab in this window. It will be handled by main window.
		if (pMsg->wParam == VK_TAB && GetAsyncKeyState(VK_CONTROL) < 0)
			return FALSE;
		if (pMsg->wParam == VK_ESCAPE)
			return FALSE;
	}
	else if (pMsg->message == WM_KEYUP)
	{
		if (pMsg->hwnd == sharedfilesctrl.m_hWnd)
			OnLvnItemActivateSharedFiles(0, 0);
	}
	else if (!thePrefs.GetStraightWindowStyles() && pMsg->message == WM_MBUTTONUP)
	{
		POINT point;
		::GetCursorPos(&point);
		CPoint p = point; 
		sharedfilesctrl.ScreenToClient(&p); 
		int it = sharedfilesctrl.HitTest(p); 
		if (it == -1)
			return FALSE;

		sharedfilesctrl.SetItemState(-1, 0, LVIS_SELECTED);
		sharedfilesctrl.SetItemState(it, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		sharedfilesctrl.SetSelectionMark(it);   // display selection mark correctly!
		sharedfilesctrl.ShowComments((CKnownFile*)sharedfilesctrl.GetItemData(it));
		return TRUE;
	}

	return CResizableDialog::PreTranslateMessage(pMsg);
}

void CSharedFilesWnd::OnSysColorChange()
{
	pop_bar.SetBkColor(GetSysColor(COLOR_3DFACE));
	pop_baraccept.SetBkColor(GetSysColor(COLOR_3DFACE));
	pop_bartrans.SetBkColor(GetSysColor(COLOR_3DFACE));
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

void CSharedFilesWnd::SetAllIcons()
{
	m_ctrlStatisticsFrm.SetIcon(_T("StatsDetail"));

	if (icon_files)
		VERIFY( DestroyIcon(icon_files) );
	//MORPH START - Changed, Downloaded History [Monki/Xman]
#ifdef NO_HISTORY
	icon_files = theApp.LoadIcon(_T("SharedFilesList"), 16, 16);
	((CStatic*)GetDlgItem(IDC_FILES_ICO))->SetIcon(icon_files);
#else
	if(!historylistctrl.IsWindowVisible())
	{
		icon_files = theApp.LoadIcon(_T("SharedFilesList"), 16, 16);
		((CStatic*)GetDlgItem(IDC_FILES_ICO))->SetIcon(icon_files);
	}
	else
	{
		icon_files = theApp.LoadIcon(_T("DOWNLOAD"), 16, 16);
		((CStatic*)GetDlgItem(IDC_FILES_ICO))->SetIcon(icon_files);
	}
#endif
	//MORPH END   - Changed, Downloaded History [Monki/Xman]
}

void CSharedFilesWnd::Localize()
{
	sharedfilesctrl.Localize();
	//MORPH START - Added, Downloaded History [Monki/Xman]
#ifndef NO_HISTORY
	historylistctrl.Localize();
#endif
	//MORPH END   - Added, Downloaded History [Monki/Xman]
	m_ctlSharedDirTree.Localize();
	sharedfilesctrl.SetDirectoryFilter(NULL,true);

	//MORPH START - Added, Downloaded History [Monki/Xman]
#ifdef NO_HISTORY
	GetDlgItem(IDC_TRAFFIC_TEXT)->SetWindowText(GetResString(IDS_SF_FILES));
#endif
	//MORPH END   - Added, Downloaded History [Monki/Xman]
	GetDlgItem(IDC_RELOADSHAREDFILES)->SetWindowText(GetResString(IDS_SF_RELOAD));
	m_ctrlStatisticsFrm.SetWindowText(GetResString(IDS_SF_STATISTICS));
	GetDlgItem(IDC_CURSESSION_LBL)->SetWindowText(GetResString(IDS_SF_CURRENT));
	GetDlgItem(IDC_TOTAL_LBL)->SetWindowText(GetResString(IDS_SF_TOTAL));
	GetDlgItem(IDC_FSTATIC6)->SetWindowText(GetResString(IDS_SF_TRANS));
	GetDlgItem(IDC_FSTATIC5)->SetWindowText(GetResString(IDS_SF_ACCEPTED));
	GetDlgItem(IDC_FSTATIC4)->SetWindowText(GetResString(IDS_SF_REQUESTS)+_T(":"));
	GetDlgItem(IDC_FSTATIC9)->SetWindowText(GetResString(IDS_SF_TRANS));
	GetDlgItem(IDC_FSTATIC8)->SetWindowText(GetResString(IDS_SF_ACCEPTED));
	GetDlgItem(IDC_FSTATIC7)->SetWindowText(GetResString(IDS_SF_REQUESTS)+_T(":"));
}

void CSharedFilesWnd::OnTvnSelChangedSharedDirsTree(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	//MORPH START - Changed, Downloaded History [Monki/Xman]
#ifdef NO_HISTORY
	sharedfilesctrl.SetDirectoryFilter(m_ctlSharedDirTree.GetSelectedFilter(), !m_ctlSharedDirTree.IsCreatingTree());
#else
	if(m_ctlSharedDirTree.GetSelectedFilter() == m_ctlSharedDirTree.pHistory)
	{
		if(!historylistctrl.IsWindowVisible())
		{
			sharedfilesctrl.ShowWindow(false);
			historylistctrl.ShowWindow(true);
			SetAllIcons();
			CString str;
			str.Format(_T(" (%i)"),historylistctrl.GetItemCount());
			GetDlgItem(IDC_TRAFFIC_TEXT)->SetWindowText(GetResString(IDS_DOWNHISTORY) + str);
			GetDlgItem(IDC_RELOADSHAREDFILES)->ShowWindow(false);
		}
	}
	else
	{
		if(!sharedfilesctrl.IsWindowVisible()) 
		{
			sharedfilesctrl.ShowWindow(true);
			historylistctrl.ShowWindow(false);
			SetAllIcons();
			GetDlgItem(IDC_RELOADSHAREDFILES)->ShowWindow(true);
			sharedfilesctrl.ShowFilesCount(); //MORPH - Added, Code Improvement for ShowFilesCount [Xman]
		}
		sharedfilesctrl.SetDirectoryFilter(m_ctlSharedDirTree.GetSelectedFilter(), !m_ctlSharedDirTree.IsCreatingTree());
	}
#endif
	//MORPH END   - Changed, Downloaded History [Monki/Xman]
	*pResult = 0;
}

LRESULT CSharedFilesWnd::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
	switch (message)
	{
	case WM_PAINT:
		if (m_wndSplitter)
		{
				CRect rcWnd;
				GetWindowRect(rcWnd);
				if (rcWnd.Width() > 0)
			{
				CRect rcSpl;
					m_ctlSharedDirTree.GetWindowRect(rcSpl);
					ScreenToClient(rcSpl);
					rcSpl.left = rcSpl.right + SPLITTER_MARGIN;
					rcSpl.right = rcSpl.left + SPLITTER_WIDTH;

					CRect rcFilter;
					m_ctlFilter.GetWindowRect(rcFilter);
					ScreenToClient(rcFilter);
					rcSpl.top = rcFilter.top;
					m_wndSplitter.MoveWindow(rcSpl, TRUE);
			}
		}
		break;

	case WM_NOTIFY:
		if (wParam == IDC_SPLITTER_SHAREDFILES)
		{ 
			SPC_NMHDR* pHdr = (SPC_NMHDR*)lParam;
			DoResize(pHdr->delta);
		}
		break;

		case WM_WINDOWPOSCHANGED: {
			CRect rcWnd;
			GetWindowRect(rcWnd);
			if (m_wndSplitter && rcWnd.Width() > 0)
				Invalidate();
			break;
		}
	case WM_SIZE:
		if (m_wndSplitter)
		{
				CRect rcWnd;
				GetWindowRect(rcWnd);
				ScreenToClient(rcWnd);
				m_wndSplitter.SetRange(rcWnd.left + SPLITTER_RANGE_MIN, rcWnd.left + SPLITTER_RANGE_MAX);
		}
		break;
	}
	return CResizableDialog::DefWindowProc(message, wParam, lParam);
}

void CSharedFilesWnd::OnSize(UINT nType, int cx, int cy){
	CResizableDialog::OnSize(nType, cx, cy);
}

LRESULT CSharedFilesWnd::OnChangeFilter(WPARAM wParam, LPARAM lParam)
{
	CWaitCursor curWait; // this may take a while

	bool bColumnDiff = (m_nFilterColumn != (uint32)wParam);
	m_nFilterColumn = (uint32)wParam;

	CStringArray astrFilter;
	CString strFullFilterExpr = (LPCTSTR)lParam;
	int iPos = 0;
	CString strFilter(strFullFilterExpr.Tokenize(_T(" "), iPos));
	while (!strFilter.IsEmpty()) {
		if (strFilter != _T("-"))
			astrFilter.Add(strFilter);
		strFilter = strFullFilterExpr.Tokenize(_T(" "), iPos);
	}

	bool bFilterDiff = (astrFilter.GetSize() != m_astrFilter.GetSize());
	if (!bFilterDiff) {
		for (int i = 0; i < astrFilter.GetSize(); i++) {
			if (astrFilter[i] != m_astrFilter[i]) {
				bFilterDiff = true;
				break;
			}
		}
	}

	if (!bColumnDiff && !bFilterDiff)
		return 0;
	m_astrFilter.RemoveAll();
	m_astrFilter.Append(astrFilter);

	sharedfilesctrl.ReloadFileList();
	return 0;
}

BOOL CSharedFilesWnd::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	theApp.ShowHelp(eMule_FAQ_GUI_SharedFiles);
	return TRUE;
}

HBRUSH CSharedFilesWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = theApp.emuledlg->GetCtlColor(pDC, pWnd, nCtlColor);
	if (hbr)
		return hbr;
	return __super::OnCtlColor(pDC, pWnd, nCtlColor);
}
//MORPH START - Added, Downloaded History [Monki/Xman]
#ifndef NO_HISTORY
void CSharedFilesWnd::OnNMClickHistorylist(NMHDR *pNMHDR, LRESULT *pResult){
	OnLvnItemActivateHistorylist(pNMHDR,pResult);
	*pResult = 0;
}

void CSharedFilesWnd::OnLvnItemActivateHistorylist(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	ShowSelectedFilesSummary(true);
}

void CSharedFilesWnd::OnShowWindow( BOOL bShow,UINT /*nStatus*/ )
{
	if(bShow && m_ctlSharedDirTree.GetSelectedFilter() == m_ctlSharedDirTree.pHistory)
	{
		CString str;
		str.Format(_T(" (%i)"),historylistctrl.GetItemCount());
		GetDlgItem(IDC_TRAFFIC_TEXT)->SetWindowText(GetResString(IDS_DOWNHISTORY) + str);
	}
	//MORPH START - Added, Code Improvement for ShowFilesCount [Xman]
	else if(bShow)
		sharedfilesctrl.ShowFilesCount();
	//MORPH END   - Added, Code Improvement for ShowFilesCount [Xman]
}
#endif
//MORPH END   - Added, Downloaded History [Monki/Xman]