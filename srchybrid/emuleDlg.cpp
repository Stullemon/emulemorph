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
#include <math.h>
#include <afxinet.h>
#pragma comment(lib, "winmm.lib")
#include <Mmsystem.h>
#include "emule.h"
#include "emuleDlg.h"
#include "ServerWnd.h"
#include "KademliaWnd.h"
#include "TransferWnd.h"
#include "SearchDlg.h"
#include "SharedFilesWnd.h"
#include "ChatWnd.h"
#include "IrcWnd.h"
#include "StatisticsDlg.h"
#include "CreditsDlg.h"
#include "PreferencesDlg.h"
#include "Sockets.h"
#include "KnownFileList.h"
#include "ServerList.h"
#include "Opcodes.h"
#include "SharedFileList.h"
#include "ED2KLink.h"
#include "Splashscreen.h"
#include "PartFileConvert.h"
#include "EnBitmap.h"
#include "Wizard.h"
#include "Exceptions.h"
#include "SearchList.h"
#include "HTRichEditCtrl.h"
#include "FrameGrabThread.h"
#include "kademlia/kademlia/kademlia.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/routing/RoutingZone.h"
#include "kademlia/routing/contact.h"
#include "kademlia/kademlia/prefs.h"
#include "KadSearchListCtrl.h"
#include "KadContactListCtrl.h"
#include "PerfLog.h"
#include "version.h"
#include "DropTarget.h"
#include "LastCommonRouteFinder.h"
#include "WebServer.h"
#include "MMServer.h"
#include "DownloadQueue.h"
#include "ClientUDPSocket.h"
#include "UploadQueue.h"
#include "ClientList.h"
#include "UploadBandwidthThrottler.h"
#include "FriendList.h"
#include "IPFilter.h"
#include "Statistics.h"
#include "MuleToolbarCtrl.h"
#include "TaskbarNotifier.h"
#include "MuleStatusbarCtrl.h"
#include "ListenSocket.h"
#include "Server.h"
#include "PartFile.h"
#include "Scheduler.h"
#include "ClientCredits.h"
#include "MenuCmds.h"
#include "MuleSystrayDlg.h"
#include "IPFilterDlg.h"
#include "WebServices.h"
#include "DirectDownloadDlg.h"

#include "fakecheck.h" //MORPH - Added by SiRoB
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


struct SLogItem
{
    bool addtostatusbar;
    CString line;
};

BOOL (WINAPI *_TransparentBlt)(HDC, int, int, int, int, HDC, int, int, int, int, UINT)= NULL;
const static UINT UWM_ARE_YOU_EMULE=RegisterWindowMessage(_T(EMULE_GUID));

// CemuleDlg Dialog

CemuleDlg::CemuleDlg(CWnd* pParent /*=NULL*/)
	: CTrayDialog(CemuleDlg::IDD, pParent)
{
	preferenceswnd = new CPreferencesDlg;
	serverwnd = new CServerWnd;
	kademliawnd = new CKademliaWnd;
	transferwnd = new CTransferWnd;
	sharedfileswnd = new CSharedFilesWnd;
	searchwnd = new CSearchDlg;
	chatwnd = new CChatWnd;
	ircwnd = new CIrcWnd;
	statisticswnd = new CStatisticsDlg;
	toolbar = new CMuleToolbarCtrl;
	statusbar = new CMuleStatusBarCtrl;
	m_wndTaskbarNotifier = new CTaskbarNotifier;

	// NOTE: the application icon name is prefixed with "AAA" to make sure it's alphabetically sorted by the
	// resource compiler as the 1st icon in the resource table!
	m_hIcon = AfxGetApp()->LoadIcon("AAAEMULEAPP");
	theApp.m_app_state = APP_STATE_RUNNING;
	ready = false; 
	startUpMinimizedChecked = false;
	m_bStartMinimized = false;
	lastuprate = 0;
	lastdownrate = 0;
	status = 0;
	activewnd = NULL;
	for (int i = 0; i < ARRSIZE(connicons); i++)
		connicons[i] = NULL;
	transicons[0] = NULL;
	transicons[1] = NULL;
	transicons[2] = NULL;
	transicons[3] = NULL;
	imicons[0] = NULL;
	imicons[1] = NULL;
	imicons[2] = NULL;
	m_iMsgIcon = 0;
	sourceTrayIcon = NULL;
	sourceTrayIconGrey = NULL;
	sourceTrayIconLow = NULL;
	usericon = NULL;
	mytrayIcon = NULL;
	m_hTimer = 0;
	notifierenabled = false;
	m_pDropTarget = new CMainFrameDropTarget;
	m_pSplashWnd = NULL;
	m_dwSplashTime = (DWORD)-1;
	m_pSystrayDlg = NULL;
}

CemuleDlg::~CemuleDlg()
{
	if (mytrayIcon) VERIFY( DestroyIcon(mytrayIcon) );
	if (m_hIcon) VERIFY( ::DestroyIcon(m_hIcon) );
	for (int i = 0; i < ARRSIZE(connicons); i++){
		if (connicons[i]) VERIFY( ::DestroyIcon(connicons[i]) );
	}
	if (transicons[0]) VERIFY( ::DestroyIcon(transicons[0]) );
	if (transicons[1]) VERIFY( ::DestroyIcon(transicons[1]) );
	if (transicons[2]) VERIFY( ::DestroyIcon(transicons[2]) );
	if (transicons[3]) VERIFY( ::DestroyIcon(transicons[3]) );
	if (imicons[0]) VERIFY( ::DestroyIcon(imicons[0]) );
	if (imicons[1]) VERIFY( ::DestroyIcon(imicons[1]) );
	if (imicons[2]) VERIFY( ::DestroyIcon(imicons[2]) );
	if (sourceTrayIcon) VERIFY( ::DestroyIcon(sourceTrayIcon) );
	if (sourceTrayIconGrey) VERIFY( ::DestroyIcon(sourceTrayIconGrey) );
	if (sourceTrayIconLow) VERIFY( ::DestroyIcon(sourceTrayIconLow) );
	if (usericon) VERIFY( ::DestroyIcon(usericon) );

	delete preferenceswnd;
	delete serverwnd;
	delete kademliawnd;
	delete transferwnd;
	delete sharedfileswnd;
	delete searchwnd;
	delete chatwnd;
	delete ircwnd;
	delete statisticswnd;
	delete toolbar;
	delete statusbar;
	delete m_wndTaskbarNotifier;

	delete m_pDropTarget;
}

void CemuleDlg::DoDataExchange(CDataExchange* pDX){
	CTrayDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CemuleDlg, CTrayDialog)
	///////////////////////////////////////////////////////////////////////////
	// Windows messages
	//
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_ENDSESSION()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_MENUCHAR()
	ON_WM_QUERYENDSESSION()
	ON_WM_SYSCOLORCHANGE()
	ON_MESSAGE(WM_COPYDATA, OnWMData)
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)

	///////////////////////////////////////////////////////////////////////////
	// WM_COMMAND messages
	//
	ON_COMMAND(MP_CONNECT, StartConnection)
	ON_COMMAND(MP_DISCONNECT, CloseConnection)
	ON_COMMAND(MP_EXIT, OnClose)
	ON_COMMAND(MP_RESTORE, RestoreWindow)
	// quick-speed changer -- 
	ON_COMMAND_RANGE(MP_QS_U10, MP_QS_UP10, QuickSpeedUpload)
	ON_COMMAND_RANGE(MP_QS_D10, MP_QS_DC, QuickSpeedDownload)
	//--- quickspeed - paralize all ---
	ON_COMMAND_RANGE(MP_QS_PA, MP_QS_UA, QuickSpeedOther)
	// quick-speed changer -- based on xrmb	

	ON_REGISTERED_MESSAGE(UWM_ARE_YOU_EMULE, OnAreYouEmule)
	ON_BN_CLICKED(IDC_HOTMENU, OnBnClickedHotmenu)

	///////////////////////////////////////////////////////////////////////////
	// WM_USER messages
	//
	ON_MESSAGE(WM_TASKBARNOTIFIERCLICKED, OnTaskbarNotifierClicked)
	//Webserver [kuchin]
	ON_MESSAGE(WEB_CONNECT_TO_SERVER, OnWebServerConnect)
	ON_MESSAGE(WEB_DISCONNECT_SERVER, OnWebServerDisonnect)
	ON_MESSAGE(WEB_REMOVE_SERVER, OnWebServerRemove)
	ON_MESSAGE(WEB_SHARED_FILES_RELOAD, OnWebSharedFilesReload)

	// Jigle SOAP service
	ON_MESSAGE(WM_JIGLE_SEARCH_RESPONSE, OnJigleSearchResponse)

	// Version Check DNS
	ON_MESSAGE(WM_VERSIONCHECK_RESPONSE, OnVersionCheckResponse)

	///////////////////////////////////////////////////////////////////////////
	// WM_APP messages
	//
	// SLUGFILLER: SafeHash
	ON_MESSAGE(TM_FINISHEDHASHING,OnFileHashed)
	ON_MESSAGE(TM_HASHFAILED,OnHashFailed)
	ON_MESSAGE(TM_PARTHASHEDOK,OnPartHashedOK)
	ON_MESSAGE(TM_PARTHASHEDCORRUPT,OnPartHashedCorrupt)
	// SLUGFILLER: SafeHash

	ON_MESSAGE(TM_FRAMEGRABFINISHED,OnFrameGrabFinished)
	ON_MESSAGE(TM_FILEALLOCEXC, OnFileAllocExc)
	ON_MESSAGE(TM_FILECOMPLETED, OnFileCompleted)
END_MESSAGE_MAP()

// CemuleDlg eventhandler

LRESULT CemuleDlg::OnAreYouEmule(WPARAM, LPARAM)
{
  return UWM_ARE_YOU_EMULE;
} 

BOOL CemuleDlg::OnInitDialog()
{
	m_fontMarlett.CreatePointFont(10 * 10, _T("Marlett"));

	m_bStartMinimized = thePrefs.GetStartMinimized();
	// temporary disable the 'startup minimized' option, otherwise no window will be shown at all
	if (thePrefs.IsFirstStart())
		m_bStartMinimized = false;

	// show splashscreen as early as possible to "entertain" user while starting emule up
	if (thePrefs.UseSplashScreen() && !m_bStartMinimized)
		ShowSplash();

	CTrayDialog::OnInitDialog();
	InitWindowStyles(this);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL){
		pSysMenu->AppendMenu(MF_SEPARATOR);

		ASSERT( (MP_ABOUTBOX & 0xFFF0) == MP_ABOUTBOX && MP_ABOUTBOX < 0xF000);
		pSysMenu->AppendMenu(MF_STRING, MP_ABOUTBOX, GetResString(IDS_ABOUTBOX));

		ASSERT( (MP_VERSIONCHECK & 0xFFF0) == MP_VERSIONCHECK && MP_VERSIONCHECK < 0xF000);
		pSysMenu->AppendMenu(MF_STRING, MP_VERSIONCHECK, GetResString(IDS_VERSIONCHECK));

		switch (thePrefs.GetWindowsVersion()){
			case _WINVER_98_:
			case _WINVER_95_:	
			case _WINVER_ME_:
				break;
			default: {
				CMenu optionsM;
				optionsM.CreateMenu();
				AddSpeedSelectorSys(&optionsM);

				// set connect/disconnect to enabled and disabled
				optionsM.AppendMenu(MF_STRING, MP_CONNECT, GetResString(IDS_MAIN_BTN_CONNECT));
				optionsM.AppendMenu(MF_STRING, MP_DISCONNECT, GetResString(IDS_MAIN_BTN_DISCONNECT)); 
				
				pSysMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)optionsM.m_hMenu, GetResString(IDS_EM_PREFS));
			}
		}
	}

	SetIcon(m_hIcon, TRUE);			
	SetIcon(m_hIcon, FALSE);

	toolbar->Create(WS_CHILD | WS_VISIBLE , CRect(0,0,0,0), this, IDC_TOOLBAR);
	toolbar->Init();

	//set title
	CString buffer = "eMule v"; 
	buffer += theApp.m_strCurVersionLong;
	SetWindowText(buffer);

	//START - enkeyDEV(kei-kun) -TaskbarNotifier-
	m_wndTaskbarNotifier->Create(this);
	if (_tcscmp(thePrefs.GetNotifierConfiguration(),"") == 0) {
		CString defaultTBN;
		defaultTBN.Format("%sNotifier.ini", thePrefs.GetAppDir());
		LoadNotifier(defaultTBN);
		thePrefs.SetNotifierConfiguration(defaultTBN);
	}
	else
		LoadNotifier(thePrefs.GetNotifierConfiguration());
	//END - enkeyDEV(kei-kun) -TaskbarNotifier-

	// set statusbar
	statusbar->Create(WS_CHILD|WS_VISIBLE|CCS_BOTTOM,CRect(0,0,0,0), this, IDC_STATUSBAR);
	statusbar->EnableToolTips(true);
	SetStatusBarPartsSize();

	LPLOGFONT plfHyperText = thePrefs.GetHyperTextLogFont();
	if (plfHyperText==NULL || plfHyperText->lfFaceName[0]=='\0' || !m_fontHyperText.CreateFontIndirect(plfHyperText))
		m_fontHyperText.CreatePointFont(100, _T("Times New Roman"));

	LPLOGFONT plfLog = thePrefs.GetLogFont();
	if (plfLog!=NULL && plfLog->lfFaceName[0]!='\0')
		m_fontLog.CreateFontIndirect(plfLog);

	// Why can't this font set via the font dialog??
//	HFONT hFontMono = CreateFont(10, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Lucida Console"));
//	m_fontLog.Attach(hFontMono);

	// create main window dialog pages
	serverwnd->Create(IDD_SERVER);
	sharedfileswnd->Create(IDD_FILES);
	searchwnd->Create(IDD_SEARCH);
	chatwnd->Create(IDD_CHAT);
	transferwnd->Create(IDD_TRANSFER);
	statisticswnd->Create(IDD_STATISTICS);
	kademliawnd->Create(IDD_KADEMLIAWND);
	ircwnd->Create(IDD_IRC);

	// optional: restore last used main window dialog
	if (thePrefs.GetRestoreLastMainWndDlg()){
		switch (thePrefs.GetLastMainWndDlgID()){
		case IDD_SERVER:
			SetActiveDialog(serverwnd);
			break;
		case IDD_FILES:
			SetActiveDialog(sharedfileswnd);
			break;
		case IDD_SEARCH:
			SetActiveDialog(searchwnd);
			break;
		case IDD_CHAT:
			SetActiveDialog(chatwnd);
			break;
		case IDD_TRANSFER:
			SetActiveDialog(transferwnd);
			break;
		case IDD_STATISTICS:
			SetActiveDialog(statisticswnd);
			break;
		case IDD_KADEMLIAWND:
			SetActiveDialog(kademliawnd);
			break;
		case IDD_IRC:
			SetActiveDialog(ircwnd);
			break;
		}
	}

	// if still no active window, activate server window
	if (activewnd == NULL)
		SetActiveDialog(serverwnd);

	// no support for changing traybar icons on-the-fly
	sourceTrayIcon = theApp.LoadIcon("TrayConnected", 16, 16);
	sourceTrayIconGrey = theApp.LoadIcon("TrayNotConnected", 16, 16);
	sourceTrayIconLow = theApp.LoadIcon("TrayLowID", 16, 16);
	SetAllIcons();
	Localize();

	// set updateintervall of graphic rate display (in seconds)
	//ShowConnectionState(false);

	// adjust all main window sizes for toolbar height and maximize the child windows
	CRect rcClient, rcToolbar, rcStatusbar;
	GetClientRect(&rcClient);
	toolbar->GetWindowRect(&rcToolbar);
	statusbar->GetWindowRect(&rcStatusbar);
	rcClient.top += rcToolbar.Height();
	rcClient.bottom -= rcStatusbar.Height();

	CWnd* apWnds[] =
	{
		serverwnd,
		kademliawnd,
		transferwnd,
		sharedfileswnd,
		searchwnd,
		chatwnd,
		ircwnd,
		statisticswnd
	};
	for (int i = 0; i < ARRSIZE(apWnds); i++)
		apWnds[i]->SetWindowPos(NULL, rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(), SWP_NOZORDER);

	// anchors
	AddAnchor(*serverwnd,		TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*kademliawnd,		TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*transferwnd,		TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*sharedfileswnd,	TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(*searchwnd,		TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(*chatwnd,			TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*ircwnd,			TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*statisticswnd,	TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(*toolbar,			TOP_LEFT, TOP_RIGHT);
	AddAnchor(*statusbar,		BOTTOM_LEFT, BOTTOM_RIGHT);

	statisticswnd->ShowInterval();

	// tray icon
	TraySetMinimizeToTray(thePrefs.GetMinTrayPTR());
	TrayMinimizeToTrayChange();

	ShowTransferRate(true);
	
	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	ShowPing();
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
	// khaos::categorymod+ obsolete
	//searchwnd->UpdateCatTabs();
	
	// Restore saved window placement
	WINDOWPLACEMENT wp = {0};
	wp.length = sizeof(wp);
	wp = thePrefs.GetEmuleWindowPlacement();
	SetWindowPlacement(&wp);

	if (thePrefs.GetWSIsEnabled())
		theApp.webserver->StartServer();
	theApp.mmserver->Init();

	VERIFY( (m_hTimer = ::SetTimer(NULL, NULL, 300, StartupTimer)) != NULL );
	if (thePrefs.GetVerbose() && !m_hTimer)
		AddDebugLogLine(true,_T("Failed to create 'startup' timer - %s"),GetErrorMessage(GetLastError()));

	theApp.stat_starttime = GetTickCount();

	if (thePrefs.IsFirstStart())
	{
		// temporary disable the 'startup minimized' option, otherwise no window will be shown at all
		m_bStartMinimized = false;
		DestroySplash();

		extern BOOL FirstTimeWizard();
		if (FirstTimeWizard()){
			// start connection wizard
			Wizard conWizard;
			conWizard.DoModal();
		}
	}

	VERIFY( m_pDropTarget->Register(this) );

	return TRUE;
}

// modders: dont remove or change the original versioncheck! (additionals are ok)
void CemuleDlg::DoVersioncheck(bool manual) {

	if (!manual && thePrefs.GetLastVC()!=0) {
		CTime last(thePrefs.GetLastVC());
		time_t tLast=safe_mktime(last.GetLocalTm());
		time_t tNow=safe_mktime(CTime::GetCurrentTime().GetLocalTm());

		if ( (difftime(tNow,tLast) / 86400)<thePrefs.GetUpdateDays() )
			return;
	}
	if (WSAAsyncGetHostByName(m_hWnd, WM_VERSIONCHECK_RESPONSE, _T("vcdns2.emule-project.org"), m_acVCDNSBuffer, sizeof(m_acVCDNSBuffer)) == 0){
		AddLogLine(true,GetResString(IDS_NEWVERSIONFAILED));
	}
}


void CALLBACK CemuleDlg::StartupTimer(HWND hwnd, UINT uiMsg, UINT idEvent, DWORD dwTime)
{
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	try
	{
		switch(theApp.emuledlg->status){
			case 0:
				theApp.emuledlg->status++;
				theApp.emuledlg->ready = true;
				// SLUGFILLER: SafeHash remove - moved down
				theApp.emuledlg->status++;
				break;
			case 1:
				break;
			case 2:
				theApp.emuledlg->status++;
				try{
					theApp.serverlist->Init();
				}
				catch(...){
					ASSERT(0);
					AddLogLine(true,_T("Failed to initialize server list - Unknown exception"));
				}
				theApp.emuledlg->status++;
				break;
			case 3:
				break;
			case 4:{
				bool bError = false;
				theApp.emuledlg->status++;

				// NOTE: If we have an unhandled exception in CDownloadQueue::Init, MFC will silently catch it
				// and the creation of the TCP and the UDP socket will not be done -> client will get a LowID!
				try{
					theApp.downloadqueue->Init();
				}
				catch(...){
					ASSERT(0);
					AddLogLine(true,_T("Failed to initialize download queue - Unknown exception"));
					bError = true;
				}
				if(!theApp.listensocket->StartListening()){
					AddLogLine(true, GetResString(IDS_MAIN_SOCKETERROR),thePrefs.GetPort());
					bError = true;
				}
				if(!theApp.clientudp->Create()){
				    AddLogLine(true, GetResString(IDS_MAIN_SOCKETERROR),thePrefs.GetUDPPort());
					bError = true;
				}
				
				if (!bError) // show the success msg, only if we had no serious error
					AddLogLine(true, GetResString(IDS_MAIN_READY)+" "+GetResString(IDS_TRANSVERSION),theApp.m_strCurVersionLong); //MORPH - Added by milobac, Translation version info
				

				// SLUGFILLER: SafeHash remove - moved down
				theApp.emuledlg->status++;
				break;
			}
			case 5:
				break;
			// SLUGFILLER: SafeHash - delay load shared files
			case 6:
				theApp.emuledlg->status++;
				theApp.sharedfiles->SetOutputCtrl(&theApp.emuledlg->sharedfileswnd->sharedfilesctrl);
				theApp.emuledlg->status++;
				break;
			case 7:
				break;
			// SLUGFILLER: SafeHash				
			default:
				// SLUGFILLER: SafeHash - autoconnect only after emule loaded completely
				if(thePrefs.DoAutoConnect())
					theApp.emuledlg->OnBnClickedButton2();
				// SLUGFILLER: SafeHash
				theApp.emuledlg->StopTimer();
		}
	}
	CATCH_DFLT_EXCEPTIONS("CemuleDlg::StartupTimer")
}

void CemuleDlg::StopTimer(){
	try	{
		if (m_hTimer){
			::KillTimer(NULL, m_hTimer);
			m_hTimer = 0;
		}
	}
	catch (...){
		ASSERT(0);
	}
	if (thePrefs.UpdateNotify()) DoVersioncheck(false);
	if (theApp.pendinglink){
		OnWMData(NULL,(LPARAM) &theApp.sendstruct);//changed by Cax2 28/10/02
		delete theApp.pendinglink;
	}
}

void CemuleDlg::OnSysCommand(UINT nID, LPARAM lParam){
	// Systemmenu-Speedselector
	if (nID>=MP_QS_U10 && nID<=10512) {
		QuickSpeedUpload(nID);
		return;
	}
	if (nID>=MP_QS_D10 && nID<=10531) {
		QuickSpeedDownload(nID);
		return;
	}
	if (nID==MP_QS_PA || nID==MP_QS_UA) {
		QuickSpeedOther(nID);
		return;
	}
	
	switch (nID /*& 0xFFF0*/){
		case MP_ABOUTBOX : {
			CCreditsDlg dlgAbout;
			dlgAbout.DoModal();
			break;
		}
		case MP_VERSIONCHECK:
			DoVersioncheck(true);
			break;
		case MP_CONNECT : {
			StartConnection();
			break;
		}
		case MP_DISCONNECT : {
			CloseConnection();
			break;
		}
		default:{
			CTrayDialog::OnSysCommand(nID, lParam);
		}
	}

	if (
		(nID & 0xFFF0) == SC_MINIMIZE ||
		(nID & 0xFFF0) == SC_MINIMIZETRAY ||
		(nID & 0xFFF0) == SC_RESTORE ||
		(nID & 0xFFF0) == SC_MAXIMIZE ) { 
			ShowTransferRate(true);
			//MORPH START - Added by SiRoB, ZZ Upload system (USS)
			ShowPing();
			//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
			transferwnd->UpdateCatTabTitles();
		}
}

void CemuleDlg::OnPaint()
{
	if (!startUpMinimizedChecked){
		//TODO: Use full initialized 'WINDOWPLACEMENT' and remove the 'OnCancel' call...
		startUpMinimizedChecked = true;
		if (m_bStartMinimized)
			OnCancel();
	}

	if (IsIconic()){
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else{
		CTrayDialog::OnPaint();
	}
}


HCURSOR CemuleDlg::OnQueryDragIcon(){
	return static_cast<HCURSOR>(m_hIcon);
}

void CemuleDlg::OnBnClickedButton2(){
	if (!theApp.IsConnected())
		//connect if not currently connected
		if (!theApp.serverconnect->IsConnecting() && !Kademlia::CKademlia::isRunning() ){
			StartConnection();
		}
		else {
			CloseConnection();
		}
	else{
		//disconnect if currently connected
		CloseConnection();
	}
}

void CemuleDlg::ResetLog(){
	serverwnd->logbox.Reset();
}

void CemuleDlg::ResetDebugLog(){
	serverwnd->debuglog.Reset();
}

void CemuleDlg::AddLogText(bool addtostatusbar, const CString& txt, bool bDebug)
{
	if (addtostatusbar)
	{
        if (statusbar->m_hWnd /*&& ready*/)
		{
			if (theApp.m_app_state != APP_STATE_SHUTINGDOWN)
			{
				statusbar->SetText(txt,0,0);
				statusbar->SetTipText(0,txt);
			}
		}
		else
			AfxMessageBox(txt);
	}
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
		Debug("%s\n", txt);
#endif

	if (bDebug && !thePrefs.GetVerbose())
		return;

	TCHAR temp[1060];
	int iLen = _sntprintf(temp, ARRSIZE(temp), _T("%s: %s\r\n"), CTime::GetCurrentTime().Format(thePrefs.GetDateTimeFormat4Log()), txt);
	if (iLen >= 0)
	{
		if (!bDebug)
		{
			serverwnd->logbox.Add(temp, iLen);
			if (IsWindow(serverwnd->StatusSelector) && serverwnd->StatusSelector.GetCurSel() != CServerWnd::PaneLog)
				serverwnd->StatusSelector.HighlightItem(CServerWnd::PaneLog, TRUE);
			if (ready)
				ShowNotifier(txt, TBN_LOG);
			if (thePrefs.GetLog2Disk())
				theLog.Log(temp, iLen);
		}

		if (thePrefs.GetVerbose() && (bDebug || thePrefs.GetFullVerbose()))
		{
				serverwnd->debuglog.Add(temp, iLen);
			if (IsWindow(serverwnd->StatusSelector) && serverwnd->StatusSelector.GetCurSel() != CServerWnd::PaneVerboseLog)
				serverwnd->StatusSelector.HighlightItem(CServerWnd::PaneVerboseLog, TRUE);

			if (thePrefs.GetDebug2Disk())
				theVerboseLog.Log(temp, iLen);
		}
	}
}

// Elandal:ThreadSafeLogging -->
void CemuleDlg::QueueDebugLogLine(bool addtostatusbar, LPCTSTR line, ...)
{
	if (!thePrefs.GetVerbose())
       return;

   m_queueLock.Lock();

   TCHAR bufferline[1000];

   va_list argptr;
   va_start(argptr, line);
	_vsntprintf(bufferline, ARRSIZE(bufferline), line, argptr);
   va_end(argptr);

	SLogItem* newItem = new SLogItem;
   newItem->addtostatusbar = addtostatusbar;
   newItem->line = bufferline;
	m_QueueDebugLog.AddTail(newItem);

	m_queueLock.Unlock();
}

void CemuleDlg::QueueLogLine(bool addtostatusbar, LPCTSTR line,...)
{
	if (!thePrefs.GetVerbose())
		return;

	m_queueLock.Lock();

	TCHAR bufferline[1000];

	va_list argptr;
	va_start(argptr, line);
	_vsnprintf(bufferline, ARRSIZE(bufferline), line, argptr);
	va_end(argptr);

	SLogItem* newItem = new SLogItem;
	newItem->addtostatusbar = addtostatusbar;
	newItem->line = bufferline;
	m_QueueLog.AddTail(newItem);

	m_queueLock.Unlock();
}

void CemuleDlg::HandleDebugLogQueue()
{
	m_queueLock.Lock();
	while(!m_QueueDebugLog.IsEmpty()) {
		const SLogItem* newItem = m_QueueDebugLog.RemoveHead();
		if (thePrefs.GetVerbose())
			AddDebugLogLine(newItem->addtostatusbar, newItem->line);
		delete newItem;
	}
	m_queueLock.Unlock();
}

void CemuleDlg::HandleLogQueue()
{
	m_queueLock.Lock();
	while(!m_QueueLog.IsEmpty()) {
		const SLogItem* newItem = m_QueueLog.RemoveHead();
		AddLogLine(newItem->addtostatusbar, newItem->line);
		delete newItem;
	}
	m_queueLock.Unlock();
}

void CemuleDlg::ClearDebugLogQueue(bool bDebugPendingMsgs)
{
	m_queueLock.Lock();
	while(!m_QueueDebugLog.IsEmpty())
	{
		if (bDebugPendingMsgs)
			TRACE("Queued dbg log msg: %s\n", m_QueueDebugLog.GetHead()->line);
		delete m_QueueDebugLog.RemoveHead();
	}
	m_queueLock.Unlock();
}

void CemuleDlg::ClearLogQueue(bool bDebugPendingMsgs)
{
	m_queueLock.Lock();
	while(!m_QueueLog.IsEmpty())
	{
		if (bDebugPendingMsgs)
			TRACE("Queued log msg: %s\n", m_QueueLog.GetHead()->line);
		delete m_QueueLog.RemoveHead();
	}
	m_queueLock.Unlock();
}
// Elandal:ThreadSafeLogging <--

CString CemuleDlg::GetLastLogEntry()
{
	return serverwnd->logbox.GetLastLogEntry();
}

CString CemuleDlg::GetAllLogEntries()
{
	return serverwnd->logbox.GetAllLogEntries();
}

CString CemuleDlg::GetLastDebugLogEntry()
{
	return serverwnd->debuglog.GetLastLogEntry();
}

CString CemuleDlg::GetAllDebugLogEntries()
{
	return serverwnd->debuglog.GetAllLogEntries();
}

void CemuleDlg::AddServerMessageLine(LPCTSTR pszLine)
{
	serverwnd->servermsgbox->AppendText(pszLine + CString(_T('\n')));
	if (IsWindow(serverwnd->StatusSelector) && serverwnd->StatusSelector.GetCurSel() != CServerWnd::PaneServerInfo)
		serverwnd->StatusSelector.HighlightItem(CServerWnd::PaneServerInfo, TRUE);
}

void CemuleDlg::ShowConnectionStateIcon()
{
	if (theApp.serverconnect->IsConnected() && !Kademlia::CKademlia::isConnected())
	{
		if (theApp.serverconnect->IsLowID())
			statusbar->SetIcon(3, connicons[3]); // LowNot
		else 
			statusbar->SetIcon(3, connicons[6]); // HighNot
	}
	else if (!theApp.serverconnect->IsConnected() && Kademlia::CKademlia::isConnected())
	{
		if (Kademlia::CKademlia::isFirewalled())
			statusbar->SetIcon(3, connicons[1]); // NotLow
		else
			statusbar->SetIcon(3, connicons[2]); // NotHigh
	}
	else if (theApp.serverconnect->IsConnected() && Kademlia::CKademlia::isConnected())
	{
		if (theApp.serverconnect->IsLowID() && Kademlia::CKademlia::isFirewalled())
			statusbar->SetIcon(3, connicons[4]); // LowLow
		else if (theApp.serverconnect->IsLowID())
			statusbar->SetIcon(3, connicons[5]); // LowHigh
		else if (Kademlia::CKademlia::isFirewalled())
			statusbar->SetIcon(3, connicons[7]); // HighLow
		else
			statusbar->SetIcon(3, connicons[8]); // HighHigh
	}
	else
	{
		statusbar->SetIcon(3, connicons[0]); // NotNot
	}
}

void CemuleDlg::ShowConnectionState()
{
	theApp.downloadqueue->OnConnectionState(theApp.IsConnected());
	serverwnd->UpdateMyInfo();
	serverwnd->UpdateControlsState();
	kademliawnd->UpdateControlsState();

	ShowConnectionStateIcon();
	
	CString status;

	//MORPH START - Changed by SiRoB, Don't know why but arceling reporting
	//Please keep this modif or we will get again an arceling reporting :p
	/*
	if(theApp.serverconnect->IsConnected())
		status = "eD2K:"+GetResString(IDS_CONNECTED);
	else if (theApp.serverconnect->IsConnecting())
		status = "eD2K:"+GetResString(IDS_CONNECTING);
	else
		status = "eD2K:"+GetResString(IDS_NOTCONNECTED);

	//Most likley needs a rewrite
	if(Kademlia::CKademlia::isConnected())
		status += "|Kad:"+GetResString(IDS_CONNECTED);
	else if (Kademlia::CKademlia::isRunning())
		status += "|Kad:"+GetResString(IDS_CONNECTING);
	else
		status += "|Kad:"+GetResString(IDS_NOTCONNECTED);
	*/
	if(theApp.serverconnect->IsConnected())
		status = "ED2K";
	else if (theApp.serverconnect->IsConnecting())
		status = "ed2k";
	else
		status = "";

	if(Kademlia::CKademlia::isConnected())
		status += status.IsEmpty()?"KAD":" | KAD";
	else if (Kademlia::CKademlia::isRunning())
		status += status.IsEmpty()?"kad":" | kad";
	//MORPH END   - Changed by SiRoB, Don't know why but arceling reporting

	statusbar->SetTipText(3,status);
	statusbar->SetText(status,3,0);

	if (theApp.IsConnected())
	{
		char szBuf[200];
		sprintf(szBuf, "%s", GetResString(IDS_MAIN_BTN_DISCONNECT));
		LPSTR pszBuf;
		TBBUTTONINFO tbi;
		LPTBBUTTONINFO lptbbi;
		pszBuf = szBuf;
		lptbbi = &tbi;
		tbi.dwMask = TBIF_IMAGE | TBIF_TEXT;
		tbi.cbSize = sizeof (TBBUTTONINFO);
		tbi.iImage = 1;
		tbi.pszText = pszBuf;
		tbi.cchText = sizeof (szBuf);
		toolbar->SetButtonInfo(IDC_TOOLBARBUTTON+0, lptbbi);
		//TOOLTIP: GetResString(IDS_MAIN_BTN_DISCONNECT_TOOLTIP)
	}
	else
	{
		if (theApp.serverconnect->IsConnecting() || Kademlia::CKademlia::isRunning()) 
		{
			char szBuf[200];
			sprintf(szBuf, "%s", GetResString(IDS_MAIN_BTN_CANCEL));
			LPSTR pszBuf;
			TBBUTTONINFO tbi;
			LPTBBUTTONINFO lptbbi;
			pszBuf = szBuf;
			lptbbi = &tbi;
			tbi.dwMask = TBIF_IMAGE | TBIF_TEXT;
			tbi.cbSize = sizeof (TBBUTTONINFO);
			tbi.iImage = 2;
			tbi.pszText = pszBuf;
			tbi.cchText = sizeof (szBuf);
			toolbar->SetButtonInfo(IDC_TOOLBARBUTTON+0, lptbbi);
			//TOOLTIP: GetResString(IDS_MAIN_BTN_CONNECT_TOOLTIP)
			ShowUserCount();
		} 
		else 
		{
			char szBuf[200];
			sprintf(szBuf, "%s", GetResString(IDS_MAIN_BTN_CONNECT));
			LPSTR pszBuf;
			TBBUTTONINFO tbi;
			LPTBBUTTONINFO lptbbi;
			pszBuf = szBuf;
			lptbbi = &tbi;
			tbi.dwMask = TBIF_IMAGE | TBIF_TEXT;
			tbi.cbSize = sizeof (TBBUTTONINFO);
			tbi.iImage = 0;
			tbi.pszText = pszBuf;
			tbi.cchText = sizeof (szBuf);
			toolbar->SetButtonInfo(IDC_TOOLBARBUTTON+0, lptbbi);
			//TOOLTIP: GetResString(IDS_CONNECTTOANYSERVER)
			//toolbar->AutoSize();
			ShowUserCount();
		}

	}
}

void CemuleDlg::ShowUserCount(){
	uint32 totaluser, totalfile;
	totaluser = totalfile = 0;
	theApp.serverlist->GetUserFileStatus( totaluser, totalfile );
	CString buffer;
	buffer.Format( "%s: %s(%s) | %s: %s", GetResString(IDS_UUSERS), CastItoIShort(totaluser), CastItoIShort(Kademlia::CKademlia::getKademliaUsers()), GetResString(IDS_FILES), CastItoIShort(totalfile));
	statusbar->SetText(buffer,1,0);
}

void CemuleDlg::ShowMessageState(uint8 iconnr)
{
	m_iMsgIcon = iconnr;
	statusbar->SetIcon(4, imicons[m_iMsgIcon]);
}

void CemuleDlg::ShowTransferStateIcon()
{
	if (lastuprate && lastdownrate)
		statusbar->SetIcon(2,transicons[3]);
	else if (lastuprate)
		statusbar->SetIcon(2,transicons[2]);
	else if (lastdownrate)
		statusbar->SetIcon(2,transicons[1]);
	else
		statusbar->SetIcon(2,transicons[0]);
}

void CemuleDlg::ShowTransferRate(bool forceAll){
	char buffer[50];

	/*lastuprate = (theApp.uploadqueue->Getavg()) ?  theApp.uploadqueue->Getavgupload()/theApp.uploadqueue->Getavg() : theApp.downloadqueue->GetDatarate();
	lastdownrate= (theApp.uploadqueue->Getavg()) ? theApp.uploadqueue->Getavgdownload()/theApp.uploadqueue->Getavg():theApp.downloadqueue->GetDatarate();
	if (theApp.uploadqueue->Getavg()) theApp.uploadqueue->Zeroavg();
	*/
	lastdownrate=theApp.downloadqueue->GetDatarate();
	lastuprate=theApp.uploadqueue->GetDatarate();

	int lastuprateoverhead = theApp.uploadqueue->GetUpDatarateOverhead();
	int lastdownrateoverhead = theApp.downloadqueue->GetDownDatarateOverhead();

	if( thePrefs.ShowOverhead() )
		sprintf(buffer,GetResString(IDS_UPDOWN),(float)lastuprate/1024, (float)lastuprateoverhead/1024, (float)lastdownrate/1024, (float)lastdownrateoverhead/1024);
	else
		sprintf(buffer,GetResString(IDS_UPDOWNSMALL),(float)lastuprate/1024, (float)lastdownrate/1024);
	
	if (TrayIsVisible() || forceAll){
		char buffer2[100];
		// set trayicon-icon
		int DownRateProcent=(int)ceil ( (lastdownrate/10.24)/ thePrefs.GetMaxGraphDownloadRate());
		if (DownRateProcent>100) DownRateProcent=100;
			UpdateTrayIcon(DownRateProcent);

		//MORPH START - Added by IceCream, Correct the bug of the download speed shown in the systray
		if (theApp.IsConnected())
			_snprintf(buffer2,sizeof buffer2,"[%s] (%s)\r\n%s",MOD_VERSION,GetResString(IDS_CONNECTED),buffer);
		else 
			_snprintf(buffer2,sizeof buffer2,"[%s] (%s)\r\n%s",MOD_VERSION,GetResString(IDS_DISCONNECTED),buffer);
		//MORPH END   - Added by IceCream, Correct the bug of the download speed shown in the systray

		buffer2[63]=0;
		TraySetToolTip(buffer2);
	}

	if (IsWindowVisible() || forceAll) {
		//MORPH START - Added by SiRoB, Show zz ratio activation
		if (thePrefs.IsZZRatioDoesWork()){
			char buffer2[100];		
			_snprintf(buffer2,sizeof buffer2,"%s r",buffer);
			statusbar->SetText(buffer2,2,0);
		}else
		//MORPH END   - Added by SiRoB, Show zz ratio activation
			statusbar->SetText(buffer,2,0);
		ShowTransferStateIcon();
	}
	if (!TrayIsVisible() && thePrefs.ShowRatesOnTitle()) {
		_snprintf(buffer,sizeof buffer,"(U:%.1f D:%.1f) eMule v%s",(float)lastuprate/1024, (float)lastdownrate/1024,theApp.m_strCurVersionLong);
		SetWindowText(buffer);
	}
}
//MORPH START - Added by SiRoB, ZZ Upload system (USS)
void CemuleDlg::ShowPing() {
	if(IsWindowVisible()) {
        CurrentPingStruct lastPing = theApp.lastCommonRouteFinder->GetCurrentPing();

        CString buffer = "";

        if(thePrefs.IsDynUpEnabled()) {
            if(lastPing.state.GetLength() == 0) {
				if (!thePrefs.IsUSSLimit()){
					if(lastPing.lowest > 0 ) {
						buffer.Format("USS %i ms %i%% %.1f%s/s",lastPing.latency, lastPing.latency*100/lastPing.lowest, (float)theApp.lastCommonRouteFinder->GetUpload()/1024,GetResString(IDS_KBYTES));
					} else {
						buffer.Format("USS %i ms %.1f%s/s",lastPing.latency, (float)theApp.lastCommonRouteFinder->GetUpload()/1024,GetResString(IDS_KBYTES));
					}
				} else
					buffer.Format("USS %i ms %i ms %.1f%s/s",lastPing.latency, thePrefs.GetDynUpPingLimit(), (float)theApp.lastCommonRouteFinder->GetUpload()/1024,GetResString(IDS_KBYTES));
			} else {
                buffer.SetString(lastPing.state);
            }
		//MORPH START - Added by SiRoB, Related to SUC
		} else if (thePrefs.IsSUCDoesWork()){
			buffer.Format("SUC r:%i vur:%.1f%s/s",theApp.uploadqueue->GetAvgRespondTime(0), (float)theApp.uploadqueue->GetMaxVUR()/1024,GetResString(IDS_KBYTES));
		//MORPH END   - Added by SiRoB, Related to SUC
		}
		statusbar->SetText(buffer,4,0);
    }
}
//MORPH END   - Added by SiRoB, ZZ Upload system (USS)

void CemuleDlg::OnCancel()
{
	if (*thePrefs.GetMinTrayPTR()){
		TrayShow();
		ShowWindow(SW_HIDE);
	}
	else{
		ShowWindow(SW_MINIMIZE);
	}
	ShowTransferRate();
	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
    ShowPing();
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
}

void CemuleDlg::SetActiveDialog(CDialog* dlg)
{
	if (dlg == activewnd)
		return;
	if (activewnd)
		activewnd->ShowWindow(SW_HIDE);
	dlg->ShowWindow(SW_SHOW);
	dlg->SetFocus();
	activewnd = dlg;
	if (dlg == transferwnd){
		if (thePrefs.ShowCatTabInfos())
			transferwnd->UpdateCatTabTitles();
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+3);
	}
	else if (dlg == serverwnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+2);
	}
	else if (dlg == chatwnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+6);
		chatwnd->chatselector.ShowChat();
	}
	else if (dlg == ircwnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+7);
	}
	else if (dlg == sharedfileswnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+5);
	}
	else if (dlg == searchwnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+4);
	}
	else if (dlg == statisticswnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+8);
		statisticswnd->ShowStatistics();
	}
	else if	(dlg == kademliawnd){
		toolbar->PressMuleButton(IDC_TOOLBARBUTTON+1);
	}
}

void CemuleDlg::SetStatusBarPartsSize()
{
	CRect rect;
	statusbar->GetClientRect(&rect);
	int ussShift = 0;
	//MORPH START - Added by SiRoB, Related to SUC
	if(thePrefs.IsDynUpEnabled() || thePrefs.IsSUCEnabled())
	{
		ussShift = 150;
	}
	//MORPH END   - Added by SiRoB, Related to SUC
	int aiWidths[5] = { rect.right-525-ussShift, rect.right-315-ussShift, rect.right-125-ussShift, rect.right-25-ussShift, -1 };
	statusbar->SetParts(5, aiWidths);
}

void CemuleDlg::OnSize(UINT nType, int cx, int cy)
{
	CTrayDialog::OnSize(nType, cx, cy);
	SetStatusBarPartsSize();
	transferwnd->VerifyCatTabSize();
}

void CemuleDlg::ProcessED2KLink(LPCTSTR pszData)
{
	try {
		CString link2;
		CString link;
		link2 = pszData;
		link2.Replace("%7c","|");
		link=URLDecode(link2);
		CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(link);
		_ASSERT( pLink !=0 );
		switch (pLink->GetKind()) {
		case CED2KLink::kFile:
			{
				//MORPH START - Changed by SiRoB, Selection category support khaos::categorymod+
				CED2KFileLink* pFileLink = (CED2KFileLink*)CED2KLink::CreateLinkFromUrl(link.Trim());
				_ASSERT(pFileLink !=0);
				theApp.downloadqueue->AddFileLinkToDownload(pFileLink, -1, true);
				//MORPH END   - Changed by SiRoB, Selection category support khaos::categorymod+
			}
			break;
		case CED2KLink::kServerList:
			{
				CED2KServerListLink* pListLink = pLink->GetServerListLink(); 
				_ASSERT( pListLink !=0 ); 
				CString strAddress = pListLink->GetAddress(); 
				if(strAddress.GetLength() != 0)
					serverwnd->UpdateServerMetFromURL(strAddress);
			}
			break;
		case CED2KLink::kServer:
			{
				CString defName;
				CED2KServerLink* pSrvLink = pLink->GetServerLink();
				_ASSERT( pSrvLink !=0 );
				in_addr host;
				host.S_un.S_addr = pSrvLink->GetIP();
				CServer* pSrv = new CServer(pSrvLink->GetPort(), inet_ntoa(host));
				_ASSERT( pSrv !=0 );
				pSrvLink->GetDefaultName(defName);
				pSrv->SetListName(defName.GetBuffer());

				// Barry - Default all new servers to high priority
				pSrv->SetPreference(PR_HIGH);

				if (!serverwnd->serverlistctrl.AddServer(pSrv,true)) 
					delete pSrv; 
				else
					AddLogLine(true,GetResString(IDS_SERVERADDED), pSrv->GetListName());
			}
			break;
		default:
			break;
		}
		delete pLink;
	}
	catch(...){
		AddLogLine(true, GetResString(IDS_LINKNOTADDED));
		ASSERT(0);
	}
}

LRESULT CemuleDlg::OnWMData(WPARAM wParam,LPARAM lParam)
{
	tagCOPYDATASTRUCT* data = (tagCOPYDATASTRUCT*)lParam;
	if (data->dwData == OP_ED2KLINK)
	{
		FlashWindow(true);
		if (thePrefs.IsBringToFront())
		{
			if (IsIconic())
				ShowWindow(SW_SHOWNORMAL);
			else if (TrayHide())
				ShowWindow(SW_SHOW);
			else
				SetForegroundWindow();
		}
		ProcessED2KLink((LPCTSTR)data->lpData);
	}
	else if (data->dwData == OP_CLCOMMAND){
		// command line command received
		CString clcommand;
		clcommand= (char*)data->lpData;
		clcommand.MakeLower();
		AddLogLine(true,"CLI: %s",clcommand);

		if (clcommand=="connect") {StartConnection(); return true;}
		if (clcommand=="disconnect") {theApp.serverconnect->Disconnect(); return true;}
		if (clcommand=="resume") {theApp.downloadqueue->StartNextFile(); return true;}
		if (clcommand=="exit") {OnClose(); return true;}
		if (clcommand=="restore") {RestoreWindow();return true;}
		if (clcommand.Left(7).MakeLower()=="limits=" && clcommand.GetLength()>8) {
			CString down="";
			CString up=clcommand.Mid(7);
			int pos=up.Find(',');
			if (pos>0) {
				down=up.Mid(pos+1);
				up=up.Left(pos);
			}
			if (down.GetLength()>0) thePrefs.SetMaxDownload(atoi(down));
			if (up.GetLength()>0) thePrefs.SetMaxUpload(atoi(up));

			return true;
		}

		if (clcommand=="help" || clcommand=="/?") {
			// show usage
			return true;
		}

		if (clcommand=="status") {
			CString strBuff;
			strBuff.Format("%sstatus.log",thePrefs.GetAppDir());
			FILE* file = fopen(strBuff, "wt");
			if (file){
				if (theApp.serverconnect->IsConnected())
					strBuff = GetResString(IDS_CONNECTED);
				else if (theApp.serverconnect->IsConnecting())
					strBuff = GetResString(IDS_CONNECTING);
				else
					strBuff = GetResString(IDS_DISCONNECTED);
				fprintf(file, "%s\n", strBuff);

				strBuff.Format(GetResString(IDS_UPDOWNSMALL), (float)theApp.uploadqueue->GetDatarate()/1024, (float)theApp.downloadqueue->GetDatarate()/1024);
				fprintf(file, "%s", strBuff); // next string (getTextList) is already prefixed with '\n'!
				fprintf(file, "%s\n", transferwnd->downloadlistctrl.getTextList());
				
				fclose(file);
			}
			return true;
		}
		// show "unknown command";
	}
	return true;
}

LRESULT CemuleDlg::OnFileHashed(WPARAM wParam, LPARAM lParam)
{
	if (theApp.m_app_state	== APP_STATE_SHUTINGDOWN)
		return false;
	CKnownFile* result = (CKnownFile*)lParam;
	ASSERT( result->IsKindOf(RUNTIME_CLASS(CKnownFile)) );

	if (wParam)
	{
		// File hashing finished for a part file when:
		// - part file just completed
		// - part file was rehashed at startup because the file date of part.met did not match the part file date

		CPartFile* requester = (CPartFile*)wParam;
		ASSERT( requester->IsKindOf(RUNTIME_CLASS(CPartFile)) );

		// SLUGFILLER: SafeHash - could have been canceled
		if (theApp.downloadqueue->IsPartFile(requester))
			requester->PartFileHashFinished(result);
		else
			delete result;
		// SLUGFILLER: SafeHash
	}
	else
	{
		ASSERT( !result->IsKindOf(RUNTIME_CLASS(CPartFile)) );

		// File hashing finished for a shared file (none partfile)
		//	- reading shared directories at startup and hashing files which were not found in known.met
		//	- reading shared directories during runtime (user hit Reload button, added a shared directory, ...)
		theApp.sharedfiles->FileHashingFinished(result);
	}
	return true;
}

// SLUGFILLER: SafeHash
LRESULT CemuleDlg::OnHashFailed(WPARAM wParam, LPARAM lParam)
{
	theApp.sharedfiles->HashFailed((UnknownFile_Struct*)lParam);
	return 0;
}

LRESULT CemuleDlg::OnPartHashedOK(WPARAM wParam,LPARAM lParam){
	CPartFile* pOwner = (CPartFile*)lParam;
	if (theApp.downloadqueue->IsPartFile(pOwner))	// could have been canceled
		pOwner->PartHashFinished((uint16)wParam, false);
	return 0;
}

LRESULT CemuleDlg::OnPartHashedCorrupt(WPARAM wParam,LPARAM lParam){
	CPartFile* pOwner = (CPartFile*)lParam;
	if (theApp.downloadqueue->IsPartFile(pOwner))	// could have been canceled
		pOwner->PartHashFinished((uint16)wParam, true);
	return 0;
}
// SLUGFILLER: SafeHash

LRESULT CemuleDlg::OnFileAllocExc(WPARAM wParam,LPARAM lParam)
{
	if (lParam==0)
		((CPartFile*)wParam)->FlushBuffersExceptionHandler();
	else
		((CPartFile*)wParam)->FlushBuffersExceptionHandler( (CFileException*)lParam );
	return 0;
}

LRESULT CemuleDlg::OnFileCompleted(WPARAM wParam, LPARAM lParam)
{
	CPartFile* partfile = (CPartFile*)lParam;
	ASSERT( partfile != NULL );
	if (partfile)
		partfile->PerformFileCompleteEnd(wParam);
	return 0;
}

bool CemuleDlg::CanClose()
{
	if (theApp.m_app_state == APP_STATE_RUNNING && thePrefs.IsConfirmExitEnabled())
	{
		if (AfxMessageBox(GetResString(IDS_MAIN_EXIT), MB_YESNO | MB_DEFBUTTON2) == IDNO)
			return false;
	}
	return true;
}

void CemuleDlg::OnClose()
{
	if (!CanClose())
		return;

	m_pDropTarget->Revoke();
	theApp.m_app_state = APP_STATE_SHUTINGDOWN;
	theApp.serverconnect->Disconnect();
	theApp.OnlineSig(); // Added By Bouc7 

	// get main window placement
	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);
	thePrefs.SetWindowLayout(wp);

	// get active main window dialog
	if (activewnd){
		if (activewnd->IsKindOf(RUNTIME_CLASS(CServerWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_SERVER);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CSharedFilesWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_FILES);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CSearchDlg)))
			thePrefs.SetLastMainWndDlgID(IDD_SEARCH);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CChatWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_CHAT);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CTransferWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_TRANSFER);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CStatisticsDlg)))
			thePrefs.SetLastMainWndDlgID(IDD_STATISTICS);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CKademliaWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_KADEMLIAWND);
		else if (activewnd->IsKindOf(RUNTIME_CLASS(CIrcWnd)))
			thePrefs.SetLastMainWndDlgID(IDD_IRC);
		else{
			ASSERT(0);
			thePrefs.SetLastMainWndDlgID(0);
		}
	}

	Kademlia::CKademlia::stop();

	// saving data & stuff
	theApp.emuledlg->preferenceswnd->m_wndSecurity.DeleteDDB();

	theApp.knownfiles->Save();
	transferwnd->downloadlistctrl.SaveSettings(CPreferences::tableDownload);
	transferwnd->uploadlistctrl.SaveSettings(CPreferences::tableUpload);
	transferwnd->queuelistctrl.SaveSettings(CPreferences::tableQueue);
	transferwnd->clientlistctrl.SaveSettings(CPreferences::tableClientList);
	searchwnd->SaveAllSettings();
	sharedfileswnd->sharedfilesctrl.SaveSettings(CPreferences::tableShared);
	serverwnd->SaveAllSettings();
	kademliawnd->SaveAllSettings();

	theApp.scheduler->RestoreOriginals();
	thePrefs.Save();
	thePerfLog.Shutdown();

	//EastShare START - Pretender, TBH-AutoBackup
	if (thePrefs.GetAutoBackup2())
		theApp.ppgbackup->Backup3();
	if (thePrefs.GetAutoBackup())
	{
		theApp.ppgbackup->Backup("*.ini", false);
		theApp.ppgbackup->Backup("*.dat", false);
		theApp.ppgbackup->Backup("*.met", false);
	}
	//EastShare END - Pretender, TBH-AutoBackup

	// Barry - Restore old registry if required
	if (thePrefs.AutoTakeED2KLinks())
		RevertReg();

	// explicitly delete all listview items which may hold ptrs to objects which will get deleted
	// by the dtors (some lines below) to avoid potential problems during application shutdown.
	transferwnd->downloadlistctrl.DeleteAllItems();
	chatwnd->chatselector.DeleteAllItems();
	theApp.clientlist->DeleteAll();
	searchwnd->searchlistctrl.DeleteAllItems();
	sharedfileswnd->sharedfilesctrl.DeleteAllItems();
    transferwnd->queuelistctrl.DeleteAllItems();
	transferwnd->clientlistctrl.DeleteAllItems();
	transferwnd->uploadlistctrl.DeleteAllItems();
	
	CPartFileConvert::CloseGUI();
	CPartFileConvert::RemoveAllJobs();

	//MORPH - Added by Yun.SF3, ZZ Upload System
	theApp.uploadBandwidthThrottler->EndThread();
	//MORPH - Added by SiRoB, ZZ Upload system (USS)
	theApp.lastCommonRouteFinder->EndThread();

	theApp.sharedfiles->DeletePartFileInstances();

    // NOTE: Do not move those dtors into 'CemuleApp::InitInstance' (althought they should be there). The
	// dtors are indirectly calling functions which access several windows which would not be available 
	// after we have closed the main window -> crash!
	delete theApp.mmserver;			theApp.mmserver = NULL;
	delete theApp.listensocket;		theApp.listensocket = NULL;
	delete theApp.clientudp;		theApp.clientudp = NULL;
	delete theApp.sharedfiles;		theApp.sharedfiles = NULL;
	delete theApp.serverconnect;	theApp.serverconnect = NULL;
	delete theApp.serverlist;		theApp.serverlist = NULL;
	delete theApp.knownfiles;		theApp.knownfiles = NULL;
	delete theApp.searchlist;		theApp.searchlist = NULL;
	delete theApp.clientcredits;	theApp.clientcredits = NULL;
	delete theApp.downloadqueue;	theApp.downloadqueue = NULL;
	delete theApp.uploadqueue;		theApp.uploadqueue = NULL;
	delete theApp.clientlist;		theApp.clientlist = NULL;
	delete theApp.friendlist;		theApp.friendlist = NULL;
	delete theApp.scheduler;		theApp.scheduler = NULL;
	delete theApp.ipfilter;			theApp.ipfilter = NULL;
	delete theApp.webserver;		theApp.webserver = NULL;
	delete theApp.statistics;		theApp.statistics = NULL;
	//EastShare Start - added by AndCycle, IP to Country
	delete theApp.ip2country;		theApp.ip2country = NULL;
	//EastShare End   - added by AndCycle, IP to Country
	//MORPH - Added by Yun.SF3, ZZ Upload System
	delete theApp.uploadBandwidthThrottler; theApp.uploadBandwidthThrottler = NULL;
	delete theApp.lastCommonRouteFinder; theApp.lastCommonRouteFinder = NULL;
	//MORPH - Added by SiRoB, ZZ Upload system (USS)

	//MORPH - Added by SiRoB, More clean :|
	delete theApp.FakeCheck;		theApp.FakeCheck = NULL;

	ClearDebugLogQueue(true);
	ClearLogQueue(true);
	
	thePrefs.Uninit();
	theApp.m_app_state = APP_STATE_DONE;
	CTrayDialog::OnCancel();
}

void CemuleDlg::OnTrayRButtonUp(CPoint pt)
{
	if(m_pSystrayDlg)
	{
		m_pSystrayDlg->BringWindowToTop();
		return;
	}

	m_pSystrayDlg = new CMuleSystrayDlg(this,pt, 
		thePrefs.GetMaxGraphUploadRate(), 
		thePrefs.GetMaxGraphDownloadRate(),
		thePrefs.GetMaxUpload(),
		thePrefs.GetMaxDownload());
	if(m_pSystrayDlg)
	{
		UINT nResult = m_pSystrayDlg->DoModal();
		delete m_pSystrayDlg;
		m_pSystrayDlg = NULL;
		switch(nResult)
		{
		case IDC_TOMAX:
			QuickSpeedOther(MP_QS_UA);
			break;
		/*
		case IDC_TOMIN:
			QuickSpeedOther(MP_QS_PA);
			break;
		*/
		case IDC_RESTORE:
			RestoreWindow();
			break;
		case IDC_CONNECT:
			StartConnection();
			break;
		case IDC_DISCONNECT:
			CloseConnection();
			break;
		case IDC_TRAY_EXIT:
			OnClose();
			break;
		case IDC_TRAYRELOADSHARE:
			theApp.sharedfiles->Reload();
			break;
		case IDC_PREFERENCES:
			{	
				static int iOpen = 0;
				if(!iOpen)
				{
					iOpen = 1;
					preferenceswnd->DoModal();
					iOpen = 0;
				}
				break;
			}
		default:
			break;
		}
	}
//MORPH END - Added by SiRoB, New Systray Popup From Fusion
}

void CemuleDlg::AddSpeedSelectorSys(CMenu* addToMenu)
{
	CMenu trayUploadPopup;
	CMenu trayDownloadPopup;
	CString text;

	// creating UploadPopup Menu
	trayUploadPopup.CreateMenu();
	//trayUploadPopup.AddMenuTitle(GetResString(IDS_PW_TIT_UP));
	text.Format("20%%\t%i %s",  (uint16)(thePrefs.GetMaxGraphUploadRate()*0.2),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U20,  text);
	text.Format("40%%\t%i %s",  (uint16)(thePrefs.GetMaxGraphUploadRate()*0.4),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U40,  text);
	text.Format("60%%\t%i %s",  (uint16)(thePrefs.GetMaxGraphUploadRate()*0.6),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U60,  text);
	text.Format("80%%\t%i %s",  (uint16)(thePrefs.GetMaxGraphUploadRate()*0.8),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U80,  text);
	text.Format("100%%\t%i %s", (uint16)(thePrefs.GetMaxGraphUploadRate()),GetResString(IDS_KBYTESEC));		trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U100, text);
	trayUploadPopup.AppendMenu(MF_SEPARATOR);
	
	if (GetRecMaxUpload()>0) {
		text.Format(GetResString(IDS_PW_MINREC)+GetResString(IDS_KBYTESEC),GetRecMaxUpload());
		trayUploadPopup.AppendMenu(MF_STRING, MP_QS_UP10, text );
	}

//	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_UPC, GetResString(IDS_PW_UNLIMITED));

	// creating DownloadPopup Menu
	trayDownloadPopup.CreateMenu();
	//trayDownloadPopup.AddMenuTitle(GetResString(IDS_PW_TIT_DOWN));
	text.Format("20%%\t%i %s",  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.2),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D20,  text);
	text.Format("40%%\t%i %s",  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.4),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D40,  text);
	text.Format("60%%\t%i %s",  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.6),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D60,  text);
	text.Format("80%%\t%i %s",  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.8),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D80,  text);
	text.Format("100%%\t%i %s", (uint16)(thePrefs.GetMaxGraphDownloadRate()),GetResString(IDS_KBYTESEC));		trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D100, text);
//	trayDownloadPopup.AppendMenu(MF_SEPARATOR);
//	trayDownloadPopup.AppendMenu(MF_STRING, MP_QS_DC, GetResString(IDS_PW_UNLIMITED));

	// Show UploadPopup Menu
	text.Format("%s:", GetResString(IDS_PW_UPL));
	addToMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)trayUploadPopup.m_hMenu, text);

	// Show DownloadPopup Menu
		text.Format("%s:", GetResString(IDS_PW_DOWNL));

	addToMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)trayDownloadPopup.m_hMenu, text);

	addToMenu->AppendMenu(MF_SEPARATOR);
//////////////////////////
}


void CemuleDlg::StartConnection(){
	if (!Kademlia::CKademlia::isRunning() && !theApp.serverconnect->IsConnecting()){
		AddLogLine(true, GetResString(IDS_CONNECTING));
		if( thePrefs.GetNetworkED2K() ){
			if ( serverwnd->serverlistctrl.GetSelectedCount()>1 )
			{
				serverwnd->serverlistctrl.PostMessage(WM_COMMAND,MP_CONNECTTO,0L);
			}
			else
			{
				theApp.serverconnect->ConnectToAnyServer();
			}
		}
		if( thePrefs.GetNetworkKademlia() )
		{
			Kademlia::CKademlia::start();
		}
		ShowConnectionState();
	}
}

void CemuleDlg::CloseConnection(){
	
	if (theApp.serverconnect->IsConnected()){
		theApp.serverconnect->Disconnect();
	}

	if (theApp.serverconnect->IsConnecting()){
		theApp.serverconnect->StopConnectionTry();
	}
	Kademlia::CKademlia::stop();
	theApp.OnlineSig(); // Added By Bouc7 
	ShowConnectionState();
}

void CemuleDlg::RestoreWindow(){
	if (TrayIsVisible())
		TrayHide();	
	
	ShowWindow(SW_SHOW);
}

void CemuleDlg::UpdateTrayIcon(int procent)
{
	// set the limits of where the bar color changes (low-high)
	int pLimits16[1] = {100};

	// set the corresponding color for each level
	COLORREF pColors16[1] = {thePrefs.GetStatsColor(11)};

	// start it up
	if (theApp.IsConnected()){
		if (theApp.IsFirewalled())
			trayIcon.Init(sourceTrayIconLow,100,1,1,16,16,thePrefs.GetStatsColor(11));
		else 
			trayIcon.Init(sourceTrayIcon,100,1,1,16,16,thePrefs.GetStatsColor(11));
	}
	else
		trayIcon.Init(sourceTrayIconGrey,100,1,1,16,16,thePrefs.GetStatsColor(11));

	// load our limit and color info
	trayIcon.SetColorLevels(pLimits16,pColors16,1);

	// generate the icon (destroy these icon using DestroyIcon())
	int pVals16[1] = {procent};

//	if (mytrayIcon)
//		VERIFY( DestroyIcon(mytrayIcon) );
	mytrayIcon = trayIcon.Create(pVals16);
	ASSERT (mytrayIcon != NULL);
	if (mytrayIcon)
		TraySetIcon(mytrayIcon,true);
	TrayUpdate();
}

int CemuleDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	return CTrayDialog::OnCreate(lpCreateStruct);
}

//START - enkeyDEV(kei-kun) -TaskbarNotifier-
void CemuleDlg::ShowNotifier(CString Text, int MsgType, bool ForceSoundOFF)
{
	if (!notifierenabled)
		return;

	LPCTSTR pszSoundEvent = NULL;
	int iSoundPrio = 0;
	bool ShowIt = false;
	switch(MsgType) {
		case TBN_CHAT:
            if (thePrefs.GetUseChatNotifier()) {
				m_wndTaskbarNotifier->Show(Text, MsgType);
				ShowIt = true;
				pszSoundEvent = _T("eMule_Chat");
				iSoundPrio = 1;
			}
			break;
		case TBN_DLOAD:
            if (thePrefs.GetUseDownloadNotifier()) {
				m_wndTaskbarNotifier->Show(Text, MsgType);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_DownloadFinished");
				iSoundPrio = 1;
			}
			break;
		case TBN_DLOADADDED:
            if (thePrefs.GetUseNewDownloadNotifier()) {
				m_wndTaskbarNotifier->Show(Text, MsgType);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_DownloadAdded");
				iSoundPrio = 1;
			}
			break;
		case TBN_LOG:
            if (thePrefs.GetUseLogNotifier()) {
				m_wndTaskbarNotifier->Show(Text, MsgType);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_LogEntryAdded");
			}
			break;
		case TBN_IMPORTANTEVENT:
			if (thePrefs.GetNotifierPopOnImportantError()) {
				m_wndTaskbarNotifier->Show(Text, MsgType);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_Urgent");
				iSoundPrio = 1;
			}
			break;	// added by InterCeptor (bugfix) 27.11.02
		case TBN_NEWVERSION:
			if (thePrefs.GetNotifierPopOnNewVersion()) {
				m_wndTaskbarNotifier->Show(Text, MsgType);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_NewVersion");
				iSoundPrio = 1;
			}
			break;
		case TBN_NULL:
            m_wndTaskbarNotifier->Show(Text, MsgType);
			ShowIt = true;			
			break;
	}
	
	if (ShowIt && !ForceSoundOFF){
		if (thePrefs.GetUseSoundInNotifier()){
        PlaySound(thePrefs.GetNotifierWavSoundPath(), NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
		}
		else if (pszSoundEvent){
			// use 'SND_NOSTOP' only for low priority events, otherwise the 'Log message' event may overrule a more important
			// event which is fired nearly at the same time.
			PlaySound(pszSoundEvent, NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT | ((iSoundPrio > 0) ? 0 : SND_NOSTOP));
		}
	}
}

void CemuleDlg::LoadNotifier(CString configuration) {
	notifierenabled = m_wndTaskbarNotifier->LoadConfiguration(configuration);
}

LRESULT CemuleDlg::OnTaskbarNotifierClicked(WPARAM wParam,LPARAM lParam)
{
	switch(m_wndTaskbarNotifier->GetMessageType()){
		case TBN_CHAT:
			RestoreWindow();
			SetActiveDialog(chatwnd);
			break;
		case TBN_DLOAD:
		case TBN_DLOADADDED:
			RestoreWindow();
			SetActiveDialog(transferwnd);
			break;
		case TBN_IMPORTANTEVENT:
			RestoreWindow();
			SetActiveDialog(serverwnd);	
			break;
		case TBN_LOG:
			RestoreWindow();
			SetActiveDialog(serverwnd);	
			break;
		case TBN_NEWVERSION:{
			CString theUrl;
			theUrl.Format("/en/version_check.php?version=%i&language=%i",theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
			theUrl = thePrefs.GetVersionCheckBaseURL()+theUrl;
			ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
			break;
		}
	}
    return 0;
}
//END - enkeyDEV(kei-kun) -TaskbarNotifier-

void CemuleDlg::OnSysColorChange()
{
	CTrayDialog::OnSysColorChange();
	SetAllIcons();
}

void CemuleDlg::SetAllIcons()
{
	// connection state
	for (int i = 0; i < ARRSIZE(connicons); i++){
		if (connicons[i]) VERIFY( ::DestroyIcon(connicons[i]) );
	}
	connicons[0] = theApp.LoadIcon("ConnectedNotNot", 16, 16);
	connicons[1] = theApp.LoadIcon("ConnectedNotLow", 16, 16);
	connicons[2] = theApp.LoadIcon("ConnectedNotHigh", 16, 16);
	connicons[3] = theApp.LoadIcon("ConnectedLowNot", 16, 16);
	connicons[4] = theApp.LoadIcon("ConnectedLowLow", 16, 16);
	connicons[5] = theApp.LoadIcon("ConnectedLowHigh", 16, 16);
	connicons[6] = theApp.LoadIcon("ConnectedHighNot", 16, 16);
	connicons[7] = theApp.LoadIcon("ConnectedHighLow", 16, 16);
	connicons[8] = theApp.LoadIcon("ConnectedHighHigh", 16, 16);
	ShowConnectionStateIcon();

	// transfer state
	if (transicons[0]) VERIFY( ::DestroyIcon(transicons[0]) );
	if (transicons[1]) VERIFY( ::DestroyIcon(transicons[1]) );
	if (transicons[2]) VERIFY( ::DestroyIcon(transicons[2]) );
	if (transicons[3]) VERIFY( ::DestroyIcon(transicons[3]) );
	transicons[0] = theApp.LoadIcon("UP0DOWN0", 16, 16);
	transicons[1] = theApp.LoadIcon("UP0DOWN1", 16, 16);
	transicons[2] = theApp.LoadIcon("UP1DOWN0", 16, 16);
	transicons[3] = theApp.LoadIcon("UP1DOWN1", 16, 16);
	ShowTransferStateIcon();

	// users state
	if (usericon) VERIFY( ::DestroyIcon(usericon) );
	usericon = theApp.LoadIcon("StatsClients", 16, 16);
	ShowUserStateIcon();

	// traybar icons: no support for changing traybar icons on-the-fly
//	if (sourceTrayIcon) VERIFY( ::DestroyIcon(sourceTrayIcon) );
//	if (sourceTrayIconGrey) VERIFY( ::DestroyIcon(sourceTrayIconGrey) );
//	if (sourceTrayIconLow) VERIFY( ::DestroyIcon(sourceTrayIconLow) );
//	sourceTrayIcon = theApp.LoadIcon("TrayConnected", 16, 16);
//	sourceTrayIconGrey = theApp.LoadIcon("TrayNotConnected", 16, 16);
//	sourceTrayIconLow = theApp.LoadIcon("TrayLowID", 16, 16);

	if (imicons[0]) VERIFY( ::DestroyIcon(imicons[0]) );
	if (imicons[1]) VERIFY( ::DestroyIcon(imicons[1]) );
	if (imicons[2]) VERIFY( ::DestroyIcon(imicons[2]) );
	imicons[0] = NULL;
	imicons[1] = theApp.LoadIcon("Message", 16, 16);
	imicons[2] = theApp.LoadIcon("MessagePending", 16, 16);
	ShowMessageState(m_iMsgIcon);
}

void CemuleDlg::Localize()
{
	ShowUserStateIcon();
	toolbar->Localize();
	ShowConnectionState();
	ShowTransferRate(true);
	ShowUserCount();
}

void CemuleDlg::ShowUserStateIcon()
{
	statusbar->SetIcon(1,usericon);
}

void CemuleDlg::QuickSpeedOther(UINT nID)
{
	switch (nID) {
		case MP_QS_PA: thePrefs.SetMaxUpload((uint16)(1));
			thePrefs.SetMaxDownload((uint16)(1));
			break ;
		case MP_QS_UA: 
			thePrefs.SetMaxUpload((uint16)(thePrefs.GetMaxGraphUploadRate()));
			thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate()));
			break ;
	}
}


void CemuleDlg::QuickSpeedUpload(UINT nID)
{
	switch (nID) {
		case MP_QS_U10: thePrefs.SetMaxUpload((uint16)(thePrefs.GetMaxGraphUploadRate()*0.1)); break ;
		case MP_QS_U20: thePrefs.SetMaxUpload((uint16)(thePrefs.GetMaxGraphUploadRate()*0.2)); break ;
		case MP_QS_U30: thePrefs.SetMaxUpload((uint16)(thePrefs.GetMaxGraphUploadRate()*0.3)); break ;
		case MP_QS_U40: thePrefs.SetMaxUpload((uint16)(thePrefs.GetMaxGraphUploadRate()*0.4)); break ;
		case MP_QS_U50: thePrefs.SetMaxUpload((uint16)(thePrefs.GetMaxGraphUploadRate()*0.5)); break ;
		case MP_QS_U60: thePrefs.SetMaxUpload((uint16)(thePrefs.GetMaxGraphUploadRate()*0.6)); break ;
		case MP_QS_U70: thePrefs.SetMaxUpload((uint16)(thePrefs.GetMaxGraphUploadRate()*0.7)); break ;
		case MP_QS_U80: thePrefs.SetMaxUpload((uint16)(thePrefs.GetMaxGraphUploadRate()*0.8)); break ;
		case MP_QS_U90: thePrefs.SetMaxUpload((uint16)(thePrefs.GetMaxGraphUploadRate()*0.9)); break ;
		case MP_QS_U100: thePrefs.SetMaxUpload(thePrefs.GetMaxGraphUploadRate()); break ;
//		case MP_QS_UPC: thePrefs.SetMaxUpload(UNLIMITED); break ;
		case MP_QS_UP10: thePrefs.SetMaxUpload(GetRecMaxUpload()); break ;
	}
}

void CemuleDlg::QuickSpeedDownload(UINT nID)
{
	switch (nID) {
		case MP_QS_D10: thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate()*0.1)); break ;
		case MP_QS_D20: thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate()*0.2)); break ;
		case MP_QS_D30: thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate()*0.3)); break ;
		case MP_QS_D40: thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate()*0.4)); break ;
		case MP_QS_D50: thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate()*0.5)); break ;
		case MP_QS_D60: thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate()*0.6)); break ;
		case MP_QS_D70: thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate()*0.7)); break ;
		case MP_QS_D80: thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate()*0.8)); break ;
		case MP_QS_D90: thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate()*0.9)); break ;
		case MP_QS_D100: thePrefs.SetMaxDownload(thePrefs.GetMaxGraphDownloadRate()); break ;
//		case MP_QS_DC: thePrefs.SetMaxDownload(UNLIMITED); break ;
	}
}
// quick-speed changer -- based on xrmb

int CemuleDlg::GetRecMaxUpload() {
	
	if (thePrefs.GetMaxGraphUploadRate()<7) return 0;
	if (thePrefs.GetMaxGraphUploadRate()<15) return thePrefs.GetMaxGraphUploadRate()-3;
	return (thePrefs.GetMaxGraphUploadRate()-4);
}

BOOL CemuleDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{	
		case IDC_TOOLBARBUTTON + 0:
			OnBnClickedButton2();
			break;
		case MP_HM_KAD:
		case IDC_TOOLBARBUTTON +1:
			SetActiveDialog(kademliawnd);
			break;
		case IDC_TOOLBARBUTTON + 2:
		case MP_HM_SRVR:
			SetActiveDialog(serverwnd);
			break;
		case IDC_TOOLBARBUTTON + 3:
		case MP_HM_TRANSFER:
			SetActiveDialog(transferwnd);
			break;
		case IDC_TOOLBARBUTTON + 4:
		case MP_HM_SEARCH:
			SetActiveDialog(searchwnd);
			break;
		case IDC_TOOLBARBUTTON + 5:
		case MP_HM_FILES:
			SetActiveDialog(sharedfileswnd);
			break;
		case IDC_TOOLBARBUTTON + 6:
		case MP_HM_MSGS:
			SetActiveDialog(chatwnd);
			break;
		case IDC_TOOLBARBUTTON + 7:
		case MP_HM_IRC:
			SetActiveDialog(ircwnd);
			break;
		case IDC_TOOLBARBUTTON + 8:
		case MP_HM_STATS:
			SetActiveDialog(statisticswnd);
			break;
		case IDC_TOOLBARBUTTON + 9:
		case MP_HM_PREFS:
			toolbar->CheckButton(IDC_TOOLBARBUTTON+9,TRUE);
			preferenceswnd->DoModal();
			toolbar->CheckButton(IDC_TOOLBARBUTTON+9,FALSE);
			break;
		case IDC_TOOLBARBUTTON + 10:
			ShowToolPopup(true);
			break;
		case MP_HM_OPENINC:
			ShellExecute(NULL, "open", thePrefs.GetIncomingDir(),NULL, NULL, SW_SHOW); 
			break;
		case MP_HM_HELP:
		case IDC_TOOLBARBUTTON + 11:
			wParam = ID_HELP;
			break;
		case MP_HM_CON:
			OnBnClickedButton2();
			break;
		case MP_HM_EXIT:
			OnClose();
			break;
		case MP_HM_LINK1: // MOD: dont remove!
			ShellExecute(NULL, NULL, thePrefs.GetHomepageBaseURL(), NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
			break;
		case MP_HM_LINK2:
			ShellExecute(NULL, NULL, thePrefs.GetHomepageBaseURL()+ CString(_T("/faq/")), NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
			break;
		case MP_HM_LINK3: {
			CString theUrl;
			theUrl.Format( thePrefs.GetVersionCheckBaseURL() + CString(_T("/en/version_check.php?version=%i&language=%i")),theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
			ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
			break;
		}
		case MP_WEBSVC_EDIT:
			theWebServices.Edit();
			break;
		case MP_HM_CONVERTPF:
			CPartFileConvert::ShowGUI();
			break;
		case MP_HM_SCHEDONOFF:
			thePrefs.SetSchedulerEnabled(!thePrefs.IsSchedulerEnabled());
			theApp.scheduler->Check(true);
			break;
		case MP_HM_1STSWIZARD:
			extern BOOL FirstTimeWizard();
			if (FirstTimeWizard()){
				// start connection wizard
				Wizard conWizard;
				conWizard.DoModal();
			}
			break;
		case MP_HM_IPFILTER:{
			CIPFilterDlg dlg;
			dlg.DoModal();
			break;
	}	
		case MP_HM_DIRECT_DOWNLOAD:{
			CDirectDownloadDlg dlg;
			dlg.DoModal();
			break;
	}
	}	
	if (wParam>=MP_WEBURL && wParam<=MP_WEBURL+99) {
		theWebServices.RunURL(NULL, wParam);
	}
	else if (wParam>=MP_SCHACTIONS && wParam<=MP_SCHACTIONS+99) {
		theApp.scheduler->ActivateSchedule(wParam-MP_SCHACTIONS);
		theApp.scheduler->SaveOriginals(); // use the new settings as original
	}

	return CTrayDialog::OnCommand(wParam, lParam);
}

LRESULT CemuleDlg::OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu)
{
	UINT nCmdID;
	if (toolbar->MapAccelerator(nChar, &nCmdID)){
		OnCommand(nCmdID, 0);
		return MAKELONG(0,MNC_CLOSE);
	}
	return CTrayDialog::OnMenuChar(nChar, nFlags, pMenu);
}

BOOL CemuleDlg::OnQueryEndSession()
{
	if (!CTrayDialog::OnQueryEndSession())
		return FALSE;

	return TRUE;
}

void CemuleDlg::OnEndSession(BOOL bEnding)
{
	if (bEnding && theApp.m_app_state == APP_STATE_RUNNING)
	{
		theApp.m_app_state = APP_STATE_SHUTINGDOWN;
		OnClose();
	}

	CTrayDialog::OnEndSession(bEnding);
}

// Barry - To find out if app is running or shutting/shut down
bool CemuleDlg::IsRunning()
{
	return (theApp.m_app_state == APP_STATE_RUNNING);
}


void CemuleDlg::OnBnClickedHotmenu()
{
	ShowToolPopup(false);
}

void CemuleDlg::ShowToolPopup(bool toolsonly)
{
	POINT point;

	::GetCursorPos(&point);

	CTitleMenu menu;
	menu.CreatePopupMenu();
	if (!toolsonly)
		menu.AddMenuTitle(GetResString(IDS_HOTMENU));
	else
		menu.AddMenuTitle(GetResString(IDS_TOOLS));

	CMenu Links;
	Links.CreateMenu();
	Links.AppendMenu(MF_STRING,MP_HM_LINK1, GetResString(IDS_HM_LINKHP));
	Links.AppendMenu(MF_STRING,MP_HM_LINK2, GetResString(IDS_HM_LINKFAQ));
	Links.AppendMenu(MF_STRING,MP_HM_LINK3, GetResString(IDS_HM_LINKVC));
	theWebServices.GetGeneralMenuEntries(Links);
	Links.InsertMenu(3, MF_BYPOSITION | MF_SEPARATOR);
	Links.AppendMenu(MF_STRING, MP_WEBSVC_EDIT, GetResString(IDS_WEBSVEDIT));

	CMenu scheduler;
	scheduler.CreateMenu();
	CString schedonoff= (!thePrefs.IsSchedulerEnabled())?GetResString(IDS_HM_SCHED_ON):GetResString(IDS_HM_SCHED_OFF);

	scheduler.AppendMenu(MF_STRING,MP_HM_SCHEDONOFF, schedonoff);
	if (theApp.scheduler->GetCount()>0) {
		scheduler.AppendMenu(MF_SEPARATOR);
		for (int i=0; i<theApp.scheduler->GetCount();i++)
			scheduler.AppendMenu(MF_STRING,MP_SCHACTIONS+i, theApp.scheduler->GetSchedule(i)->title );
	}

	if (!toolsonly) {
		if (theApp.serverconnect->IsConnected())
			menu.AppendMenu(MF_STRING,MP_HM_CON,GetResString(IDS_MAIN_BTN_DISCONNECT));
		else if (theApp.serverconnect->IsConnecting())
			menu.AppendMenu(MF_STRING,MP_HM_CON,GetResString(IDS_MAIN_BTN_CANCEL));
		else
			menu.AppendMenu(MF_STRING,MP_HM_CON,GetResString(IDS_MAIN_BTN_CONNECT));

		menu.AppendMenu(MF_STRING,MP_HM_KAD, GetResString(IDS_EM_KADEMLIA) );
		menu.AppendMenu(MF_STRING,MP_HM_SRVR, GetResString(IDS_EM_SERVER) );
		menu.AppendMenu(MF_STRING,MP_HM_TRANSFER, GetResString(IDS_EM_TRANS));
		menu.AppendMenu(MF_STRING,MP_HM_SEARCH, GetResString(IDS_EM_SEARCH));
		menu.AppendMenu(MF_STRING,MP_HM_FILES, GetResString(IDS_EM_FILES));
		menu.AppendMenu(MF_STRING,MP_HM_MSGS, GetResString(IDS_EM_MESSAGES));
		menu.AppendMenu(MF_STRING,MP_HM_IRC, GetResString(IDS_IRC));
		menu.AppendMenu(MF_STRING,MP_HM_STATS, GetResString(IDS_EM_STATISTIC));
		menu.AppendMenu(MF_STRING,MP_HM_PREFS, GetResString(IDS_EM_PREFS));
		menu.AppendMenu(MF_STRING,MP_HM_HELP, GetResString(IDS_EM_HELP));
		menu.AppendMenu(MF_SEPARATOR);
	}

	menu.AppendMenu(MF_STRING,MP_HM_OPENINC, GetResString(IDS_OPENINC) + _T("..."));
	menu.AppendMenu(MF_STRING,MP_HM_CONVERTPF, GetResString(IDS_IMPORTSPLPF) + _T("..."));
	menu.AppendMenu(MF_STRING,MP_HM_1STSWIZARD, GetResString(IDS_WIZ1) + _T("..."));
	menu.AppendMenu(MF_STRING,MP_HM_IPFILTER, GetResString(IDS_IPFILTER) + _T("..."));
	menu.AppendMenu(MF_STRING,MP_HM_DIRECT_DOWNLOAD, GetResString(IDS_SW_DIRECTDOWNLOAD) + _T("..."));

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)Links.m_hMenu, GetResString(IDS_LINKS) );
	menu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)scheduler.m_hMenu, GetResString(IDS_SCHEDULER) );

	if (!toolsonly) {
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING,MP_HM_EXIT, GetResString(IDS_EXIT));
	}
	menu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( Links.DestroyMenu() );
	VERIFY( scheduler.DestroyMenu() );
	VERIFY( menu.DestroyMenu() );
}

LRESULT CemuleDlg::OnWebServerConnect(WPARAM wParam, LPARAM lParam)
{
	CServer* server=(CServer*)wParam;
	if (server==NULL) theApp.serverconnect->ConnectToAnyServer();
		else theApp.serverconnect->ConnectToServer(server);
	
	return 0;
}

LRESULT CemuleDlg::OnWebServerDisonnect(WPARAM wParam, LPARAM lParam)
{
	theApp.serverconnect->Disconnect();
	
	return 0;
}

LRESULT CemuleDlg::OnWebServerRemove(WPARAM wParam, LPARAM lParam)
{
	serverwnd->serverlistctrl.RemoveServer((CServer*)wParam); // sivka's bugfix
	return 0;
}

LRESULT CemuleDlg::OnWebSharedFilesReload(WPARAM wParam, LPARAM lParam)
{
	theApp.sharedfiles->Reload();
	return 0;
}

void CemuleDlg::ApplyHyperTextFont(LPLOGFONT plf)
{
	m_fontHyperText.DeleteObject();
	if (m_fontHyperText.CreateFontIndirect(plf))
	{
		thePrefs.SetHyperTextFont(plf);
		serverwnd->servermsgbox->SetFont(&m_fontHyperText);
		chatwnd->chatselector.UpdateFonts(&m_fontHyperText);
		ircwnd->UpdateFonts(&m_fontHyperText);
	}
}

void CemuleDlg::ApplyLogFont(LPLOGFONT plf)
{
	m_fontLog.DeleteObject();
	if (m_fontLog.CreateFontIndirect(plf))
	{
		thePrefs.SetLogFont(plf);
		serverwnd->logbox.SetFont(&m_fontLog);
		serverwnd->debuglog.SetFont(&m_fontLog);
	}
}

LRESULT CemuleDlg::OnJigleSearchResponse(WPARAM wParam, LPARAM lParam)
{
	if ( !IsRunning() )
		return 1;
	searchwnd->ProcessJigleSearchResponse(wParam, lParam);
	return 0;
}

void CemuleDlg::SetKadButtonState() {
	//I'm temp removing this.. Will most likely split the status window from the servers.
	//This have a true Server & Kademlia window.. We can then add and remove the buttons then..
	//Also, I think it's best to hide the button, not disable..
	if( Kademlia::CKademlia::isRunning() )
		toolbar->HideButton(IDC_TOOLBARBUTTON+1, false);
	else
		toolbar->HideButton(IDC_TOOLBARBUTTON+1, !thePrefs.GetNetworkKademlia());
}

LRESULT CemuleDlg::OnFrameGrabFinished(WPARAM wParam,LPARAM lParam){
	CKnownFile* pOwner = (CKnownFile*)wParam;
	FrameGrabResult_Struct* result = (FrameGrabResult_Struct*)lParam;
	
	if (theApp.knownfiles->IsKnownFile(pOwner) || theApp.downloadqueue->IsPartFile(pOwner) ){
		pOwner->GrabbingFinished(result->imgResults,result->nImagesGrabbed, result->pSender);
	}
	else{
		ASSERT ( false );
	}

	delete result;
	return 0;
}

void StraightWindowStyles(CWnd* pWnd)
{
	CWnd* pWndChild = pWnd->GetWindow(GW_CHILD);
	while (pWndChild)
	{
		StraightWindowStyles(pWndChild);
		pWndChild = pWndChild->GetNextWindow();
	}

	TCHAR szClassName[MAX_PATH];
	if (::GetClassName(*pWnd, szClassName, ARRSIZE(szClassName)))
	{
		bool bButton = (_tcsicmp(szClassName, _T("Button")) == 0);

		if (   (_tcsicmp(szClassName, _T("EDIT")) == 0 && (pWnd->GetExStyle() & WS_EX_STATICEDGE))
			//|| (bButton && (pWnd->GetStyle() & BS_GROUPBOX) == 0)
			|| _tcsicmp(szClassName, _T("SysListView32")) == 0
			//|| _tcsnicmp(szClassName, _T("RichEdit20"), 10) == 0
			|| _tcsicmp(szClassName, _T("msctls_trackbar32")) == 0
			)
		{
			pWnd->ModifyStyleEx(WS_EX_STATICEDGE, WS_EX_CLIENTEDGE);
		}

		if (bButton)
			pWnd->ModifyStyle(BS_FLAT, 0);
	}
}

void FlatWindowStyles(CWnd* pWnd)
{
	CWnd* pWndChild = pWnd->GetWindow(GW_CHILD);
	while (pWndChild)
	{
		FlatWindowStyles(pWndChild);
		pWndChild = pWndChild->GetNextWindow();
	}

	TCHAR szClassName[MAX_PATH];
	if (::GetClassName(*pWnd, szClassName, ARRSIZE(szClassName)))
	{
		bool bButton = (_tcsicmp(szClassName, _T("Button")) == 0);

//		if (   !bButton
//			//|| (_tcsicmp(szClassName, _T("SysListView32")) == 0 && (pWnd->GetStyle() & WS_BORDER) == 0)
//			|| _tcsnicmp(szClassName, _T("RichEdit20"), 10) == 0
//			|| _tcsicmp(szClassName, _T("msctls_trackbar32")) == 0
//			)
//		{
//			pWnd->ModifyStyleEx(WS_EX_CLIENTEDGE, WS_EX_STATICEDGE);
//		}

		if (bButton)
			pWnd->ModifyStyle(0, BS_FLAT);
	}
}

void InitWindowStyles(CWnd* pWnd)
{
	if (thePrefs.GetStraightWindowStyles() < 0)
		return;
	else if (thePrefs.GetStraightWindowStyles() > 0)
		StraightWindowStyles(pWnd);
	else
		FlatWindowStyles(pWnd);
}

LRESULT CemuleDlg::OnVersionCheckResponse(WPARAM wParam, LPARAM lParam)
{
	if (WSAGETASYNCERROR(lParam) == 0)
	{
		int iBufLen = WSAGETASYNCBUFLEN(lParam);
		if (iBufLen >= sizeof(HOSTENT))
		{
			LPHOSTENT pHost = (LPHOSTENT)m_acVCDNSBuffer;
			if (pHost->h_length == 4 && pHost->h_addr_list && pHost->h_addr_list[0])
			{
				uint32 dwResult = ((LPIN_ADDR)(pHost->h_addr_list[0]))->s_addr;
				// last byte contains informations about mirror urls, to avoid effects of future DDoS Attacks against eMules Homepage
				thePrefs.SetWebMirrorAlertLevel((uint8)(dwResult >> 24));
				uint8 abyCurVer[4] = { VERSION_BUILD + 1, VERSION_UPDATE, VERSION_MIN, 0};
				dwResult &= 0x00FFFFFF;
				if (dwResult > *(uint32*)abyCurVer){
					thePrefs.UpdateLastVC();
					SetActiveWindow();
					AddLogLine(true,GetResString(IDS_NEWVERSIONAVL));
					ShowNotifier(GetResString(IDS_NEWVERSIONAVLPOPUP), TBN_NEWVERSION);
					if (!thePrefs.GetNotifierPopOnNewVersion()){
						if (AfxMessageBox(GetResString(IDS_NEWVERSIONAVL)+GetResString(IDS_VISITVERSIONCHECK),MB_YESNO)==IDYES) {
							CString theUrl;
							theUrl.Format("/en/version_check.php?version=%i&language=%i",theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
							theUrl = thePrefs.GetVersionCheckBaseURL()+theUrl;
							ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
						}
					}
				}
				else{
					thePrefs.UpdateLastVC();
					AddLogLine(true,GetResString(IDS_NONEWERVERSION));
				}
				return 0;
			}
		}
	}
	AddLogLine(true,GetResString(IDS_NEWVERSIONFAILED));
	return 0;
}

void CemuleDlg::ShowSplash()
{
	ASSERT( m_pSplashWnd == NULL );
	if (m_pSplashWnd == NULL)
	{
		m_pSplashWnd = new CSplashScreen;
		if (m_pSplashWnd != NULL)
		{
			ASSERT(m_hWnd);
			if (m_pSplashWnd->Create(CSplashScreen::IDD, this))
			{
				m_pSplashWnd->ShowWindow(SW_SHOW);
				m_pSplashWnd->UpdateWindow();
				m_dwSplashTime = ::GetCurrentTime();
			}
			else
			{
				delete m_pSplashWnd;
				m_pSplashWnd = NULL;
			}
		}
	}
}

void CemuleDlg::DestroySplash()
{
	if (m_pSplashWnd != NULL)
	{
		m_pSplashWnd->DestroyWindow();
		delete m_pSplashWnd;
		m_pSplashWnd = NULL;
	}
}

LRESULT CemuleDlg::OnKickIdle(UINT nWhy, long lIdleCount)
{
	LRESULT lResult = 0;

	if (m_pSplashWnd)
	{
		if (::GetCurrentTime() - m_dwSplashTime > 2500)
		{
			// timeout expired, destroy the splash window
			DestroySplash();
			UpdateWindow();
		}
		else
		{
			// check again later...
			lResult = 1;
		}
	}

	return lResult;
}

BOOL CemuleDlg::PreTranslateMessage(MSG* pMsg)
{
	BOOL bResult = CTrayDialog::PreTranslateMessage(pMsg);

	if (m_pSplashWnd && m_pSplashWnd->m_hWnd != NULL &&
		(pMsg->message == WM_KEYDOWN	   ||
		 pMsg->message == WM_SYSKEYDOWN	   ||
		 pMsg->message == WM_LBUTTONDOWN   ||
		 pMsg->message == WM_RBUTTONDOWN   ||
		 pMsg->message == WM_MBUTTONDOWN   ||
		 pMsg->message == WM_NCLBUTTONDOWN ||
		 pMsg->message == WM_NCRBUTTONDOWN ||
		 pMsg->message == WM_NCMBUTTONDOWN))
	{
		DestroySplash();
		UpdateWindow();
	} 
	return bResult;
}

// MightyKnife: extended debug logging

#define MAX_MESSAGES 500

/* Name:     AddExtDebugMessage
   Function: This function adds the message "line" to the debug list m_ExtDebugMessages.
			 This list contains MAX_MESSAGES extended debug messages that are invisible to 
			 the user. If the method OutputExtDebugMessages is called, these debug messages 
			 are printed to the visible debug log.
   Return:   -
   Remarks:  -
   Author:	 MightyKnife
*/
void CemuleDlg::AddExtDebugMessage (CString _line,...) {


	char temp[1060]; 
	char bufferline[1000];

	va_list argptr;
	va_start(argptr, _line);
	_vsnprintf(bufferline, 1000, _line, argptr);
	va_end(argptr);

	CTime nowT=CTime::GetCurrentTime();
	_snprintf(temp,sizeof temp,"%s/%d: %s",nowT.Format(thePrefs.GetDateTimeFormat4Log()),
			  ++m_ExtDebugMessagesCount,bufferline);
	m_ExtDebugMessages.AddTail (temp);

	// remove some old debug messages if necessary
	while (m_ExtDebugMessages.GetSize () > MAX_MESSAGES) 
		m_ExtDebugMessages.RemoveHead ();
}

/* Name:     AddExtDebugDump
   Function: This method works comparable AddExtDebugMessage. It first prints the
			 _headline to the extended debug message (if it is != ""). Afterwards it 
			 transforms the buffer _data into a hexadecimal string and prints it on 
			 the  extended debug messages, too. So it's good for dumping data blocks.
			 The _subscript is added afterwards, too, if it's != "".
   Return:   -
   Remarks:  -
   Author:   Mighty Knife
*/
void CemuleDlg::AddExtDebugDump (CString _headline, const char* _data, int _size, CString _subscript) {


	// Add timestamp to the headline
	if (_headline != "") AddExtDebugMessage ("%s",(const char*) _headline);

	// Only dump the first 256 bytes
	if (_size > 256) _size = 256;

	// output as hex string
	CString tmp,tmp2;
	for (int itmp=0; itmp < _size; itmp++) {
		tmp2.Format (" %02X",(unsigned) (BYTE) (_data [itmp]));
		tmp += tmp2;
	}
	m_ExtDebugMessages.AddTail (tmp);

	// output as C++-string for inserting into the sourcecode for testing
	tmp="\"";
	for (int itmp=0; itmp < _size; itmp++) {
		tmp2.Format ("\\x%02X",(unsigned) (BYTE) (_data [itmp]));
		if ((_data [itmp] >= 32) && (_data [itmp] <= 'z') && 
			(_data [itmp] != '\"') && (_data [itmp] != '\\'))
			tmp += (unsigned char) _data [itmp];
		else tmp += tmp2;
	}
	tmp += "\"";
	m_ExtDebugMessages.AddTail (tmp);
	
	if (_subscript != "") m_ExtDebugMessages.AddTail (_subscript);

	// remove some old debug messages if necessary
	while (m_ExtDebugMessages.GetSize () > MAX_MESSAGES) 
		m_ExtDebugMessages.RemoveHead ();
}

/* Name:     OutputExtDebugMessages
   Function: Prints all messages in the m_ExtDebugMessages list to the debug log.
			 Removes all messages from the list so that messages are only printed once.
   Return:   -
   Remarks:  -
   Author:   Mighty Knife
*/
void CemuleDlg::OutputExtDebugMessages () {

	while (!m_ExtDebugMessages.IsEmpty ()) {
		CString temp = m_ExtDebugMessages.RemoveHead ();
		serverwnd->debuglog.AddEntry(temp);
	}
}
// [end] Mighty Knife
