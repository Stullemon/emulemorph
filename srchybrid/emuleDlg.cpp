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
#include <math.h>
#include <afxinet.h>
#include <Mmsystem.h>
#include <HtmlHelp.h>
#include <share.h>
#include "emule.h"
#include "emuleDlg.h"
#include "ServerWnd.h"
#include "KademliaWnd.h"
#include "TransferWnd.h"
#include "SearchResultsWnd.h"
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
#include "PeerCacheFinder.h"
#include "Statistics.h"
#include "FirewallOpener.h"
#include "StringConversion.h"
#include "aichsyncthread.h"

#include "fakecheck.h" //MORPH - Added by SiRoB
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country
// MORPH START - Added by Commander, Friendlinks [emulEspaña]
#include "Friend.h"
// MORPH END - Added by Commander, Friendlinks [emulEspaña]

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


BOOL (WINAPI *_TransparentBlt)(HDC, int, int, int, int, HDC, int, int, int, int, UINT)= NULL;
const static UINT UWM_ARE_YOU_EMULE = RegisterWindowMessage(EMULE_GUID);

IMPLEMENT_DYNAMIC(CMsgBoxException, CException)

//Commander - Added: Invisible Mode [TPT] - Start
// Allows "invisible mode" on multiple instances of eMule
#ifdef _DEBUG
#define EMULE_GUID_INVMODE				"EMULE-{4EADC6FC-516F-4b7c-9066-97D893649569}-DEBUG-INVISIBLEMODE"
#else
#define EMULE_GUID_INVMODE				"EMULE-{4EADC6FC-516F-4b7c-9066-97D893649569}-INVISIBLEMODE"
#endif
const static UINT UWM_RESTORE_WINDOW_IM=RegisterWindowMessage(_T(EMULE_GUID_INVMODE));
//Commander - Added: Invisible Mode [TPT] - End

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
	m_hIcon = AfxGetApp()->LoadIcon(_T("AAAEMULEAPP"));
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
	m_lasticoninfo=255;
	b_HideApp = false; //MORPH - Added by SiRoB, Toggle Show Hide window

    //Commander - Added: Blinking Tray Icon On Message Recieve [emulEspaña] - Start
	sourceTrayMessage = NULL;
	sourceTrayMessageLow = NULL;
	sourceTrayMessageGrey = NULL;
	//Commander - Added: Blinking Tray Icon On Message Recieve [emulEspaña] - End
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

	//Commander - Added: Blinking Tray Icon On Message Recieve [emulEspaña] - Start
	if (sourceTrayMessage) VERIFY( ::DestroyIcon(sourceTrayMessage) );
	if (sourceTrayMessageLow) VERIFY( ::DestroyIcon(sourceTrayMessageLow) );
	if (sourceTrayMessageGrey) VERIFY( ::DestroyIcon(sourceTrayMessageGrey) );
	//Commander - Added: Blinking Tray Icon On Message Recieve [emulEspaña] - End

	// already destroyed by windows?
	//VERIFY( m_menuUploadCtrl.DestroyMenu() );
	//VERIFY( m_menuDownloadCtrl.DestroyMenu() );
	//VERIFY( m_SysMenuOptions.DestroyMenu() );

	delete preferenceswnd;
	delete serverwnd;
	delete kademliawnd;
	delete transferwnd;
	delete sharedfileswnd;
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
	ON_WM_SHOWWINDOW()
	ON_MESSAGE(WM_HOTKEY, OnHotKey)	//Commander - Added: Invisible Mode [TPT]

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
	ON_REGISTERED_MESSAGE(UWM_RESTORE_WINDOW_IM, OnRestoreWindowInvisibleMode) //Commander - Added: Invisible Mode [TPT]
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

	// Version Check DNS
	ON_MESSAGE(WM_VERSIONCHECK_RESPONSE, OnVersionCheckResponse)
	//MORPH - Added by SiRoB, New Version check
	ON_MESSAGE(WM_MVERSIONCHECK_RESPONSE, OnMVersionCheckResponse)
	
	// PeerCache DNS
	ON_MESSAGE(WM_PEERCHACHE_RESPONSE, OnPeerCacheResponse)

	///////////////////////////////////////////////////////////////////////////
	// WM_APP messages
	//
	ON_MESSAGE(TM_FINISHEDHASHING,OnFileHashed)
	ON_MESSAGE(TM_FILEOPPROGRESS, OnFileOpProgress)
	ON_MESSAGE(TM_HASHFAILED,OnHashFailed)
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
	if( !m_bStartMinimized )
		m_bStartMinimized = theApp.DidWeAutoStart();

	// temporary disable the 'startup minimized' option, otherwise no window will be shown at all
	if (thePrefs.IsFirstStart())
		m_bStartMinimized = false;

	// show splashscreen as early as possible to "entertain" user while starting emule up
	if (thePrefs.UseSplashScreen() && !m_bStartMinimized)
		ShowSplash();

	//Commander - Added: Startupsound - Start
	if (thePrefs.UseStartupSound() && !m_bStartMinimized){
		if(PathFileExists(thePrefs.GetConfigDir() + _T("startup.wav"))) 
			PlaySound(thePrefs.GetConfigDir() + _T("startup.wav"), NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
		else
			AddLogLine(false,GetResString(IDS_MISSING_STARTUPSOUND));
	}
	//Commander - Added: Startupsound - End

	CTrayDialog::OnInitDialog();
	InitWindowStyles(this);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL){
		pSysMenu->AppendMenu(MF_SEPARATOR);

		ASSERT( (MP_ABOUTBOX & 0xFFF0) == MP_ABOUTBOX && MP_ABOUTBOX < 0xF000);
		pSysMenu->AppendMenu(MF_STRING, MP_ABOUTBOX, GetResString(IDS_ABOUTBOX));

		ASSERT( (MP_VERSIONCHECK & 0xFFF0) == MP_VERSIONCHECK && MP_VERSIONCHECK < 0xF000);
		pSysMenu->AppendMenu(MF_STRING, MP_VERSIONCHECK, GetResString(IDS_VERSIONCHECK));
		//MORPH START - Added by SiRoB, New Version check
		ASSERT( (MP_MVERSIONCHECK & 0xFFF0) == MP_MVERSIONCHECK && MP_MVERSIONCHECK < 0xF000);
		pSysMenu->AppendMenu(MF_STRING, MP_VERSIONCHECK, GetResString(IDS_MVERSIONCHECK));
		//MORPH END   - Added by SiRoB, New Version check
		
		// remaining system menu entries are created later...
	}

	SetIcon(m_hIcon, TRUE);			
	// this scales the 32x32 icon down to 16x16, does not look nice at least under WinXP
	//SetIcon(m_hIcon, FALSE);	

	toolbar->Create(WS_CHILD | WS_VISIBLE , CRect(0,0,0,0), this, IDC_TOOLBAR);
	toolbar->Init();

	//set title
	CString buffer = _T("eMule v"); 
	buffer += theApp.m_strCurVersionLong;

	//MORPH START - Added by SiRoB, [itsonlyme: -modname-]
	buffer += _T(" [") + theApp.m_strModLongVersion + _T("]");
	//MORPH END   - Added by SiRoB, [itsonlyme: -modname-]

	SetWindowText(buffer);

	//START - enkeyDEV(kei-kun) -TaskbarNotifier-
	m_wndTaskbarNotifier->Create(this);
	if (_tcscmp(thePrefs.GetNotifierConfiguration(),_T("")) == 0) {
		CString defaultTBN;
		defaultTBN.Format(_T("%sNotifier.ini"), thePrefs.GetAppDir());
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
	if (plfHyperText==NULL || plfHyperText->lfFaceName[0]==_T('\0') || !m_fontHyperText.CreateFontIndirect(plfHyperText))
		m_fontHyperText.CreatePointFont(100, _T("Times New Roman"));

	LPLOGFONT plfLog = thePrefs.GetLogFont();
	if (plfLog!=NULL && plfLog->lfFaceName[0]!=_T('\0'))
		m_fontLog.CreateFontIndirect(plfLog);

	// Why can't this font set via the font dialog??
//	HFONT hFontMono = CreateFont(10, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Lucida Console"));
//	m_fontLog.Attach(hFontMono);

	// create main window dialog pages
	serverwnd->Create(IDD_SERVER);
	sharedfileswnd->Create(IDD_FILES);
	searchwnd->Create(this);
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
	sourceTrayIcon = theApp.LoadIcon(_T("TrayConnected"), 16, 16);
	sourceTrayIconGrey = theApp.LoadIcon(_T("TrayNotConnected"), 16, 16);
	sourceTrayIconLow = theApp.LoadIcon(_T("TrayLowID"), 16, 16);

	//Commander - Added: Blinking Tray Icon On Message Recieve [emulEspaña] - Start
	sourceTrayMessage = theApp.LoadIcon(_T("TRAY_MESSAGE"), 16, 16);
	sourceTrayMessageLow = theApp.LoadIcon(_T("TRAY_MESSAGE_LOW"), 16, 16);
	sourceTrayMessageGrey = theApp.LoadIcon(_T("TRAY_MESSAGE_GREY"), 16, 16);
	//Commander - Added: Blinking Tray Icon On Message Recieve [emulEspaña] - End

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
	// ZZ:UploadSpeedSense -->
	ShowPing();
	// ZZ:UploadSpeedSense <--
	searchwnd->UpdateCatTabs();
	
	// Restore saved window placement
	WINDOWPLACEMENT wp = {0};
	wp.length = sizeof(wp);
	wp = thePrefs.GetEmuleWindowPlacement();
	SetWindowPlacement(&wp);

	if (thePrefs.GetWSIsEnabled())
		theApp.webserver->StartServer();
	theApp.mmserver->Init();

	//MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
	if (thePrefs.GetWapServerEnabled())
		theApp.wapserver->StartServer();
	//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]

	VERIFY( (m_hTimer = ::SetTimer(NULL, NULL, 300, StartupTimer)) != NULL );
	if (thePrefs.GetVerbose() && !m_hTimer)
		AddDebugLogLine(true,_T("Failed to create 'startup' timer - %s"),GetErrorMessage(GetLastError()));

	theStats.starttime = GetTickCount();

	if (thePrefs.IsFirstStart())
	{
		// temporary disable the 'startup minimized' option, otherwise no window will be shown at all
		m_bStartMinimized = false;

		//MORPH START - Added by SiROB, WebCache 1.2f
		thePrefs.detectWebcacheOnStartup = true; //jp detect webcache on startup
		//MORPH END   - Added by SiROB, WebCache 1.2f

		DestroySplash();

		extern BOOL FirstTimeWizard();
		if (FirstTimeWizard()){
			// start connection wizard
			Wizard conWizard;
			conWizard.DoModal();
		}
	}

	if(thePrefs.GetInvisibleMode()) RegisterInvisibleHotKey();//Commander - Added: Invisible Mode [TPT]
	VERIFY( m_pDropTarget->Register(this) );

	// initalize PeerCache
	theApp.m_pPeerCache->Init(thePrefs.GetPeerCacheLastSearch(), thePrefs.WasPeerCacheFound(), thePrefs.IsPeerCacheDownloadEnabled(), thePrefs.GetPeerCachePort());

	// start aichsyncthread
	AfxBeginThread(RUNTIME_CLASS(CAICHSyncThread), THREAD_PRIORITY_BELOW_NORMAL,0);

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
	if (WSAAsyncGetHostByName(m_hWnd, WM_VERSIONCHECK_RESPONSE, "vcdns2.emule-project.org", m_acVCDNSBuffer, sizeof(m_acVCDNSBuffer)) == 0){
		AddLogLine(true,GetResString(IDS_NEWVERSIONFAILED));
	}
}
//MORPH START - Added by SiRoB, New Version check
void CemuleDlg::DoMVersioncheck(bool manual) {

	if (!manual && thePrefs.GetLastVC()!=0) {
		CTime last(thePrefs.GetLastVC());
		time_t tLast=safe_mktime(last.GetLocalTm());
		time_t tNow=safe_mktime(CTime::GetCurrentTime().GetLocalTm());

		if ( (difftime(tNow,tLast) / 86400)<thePrefs.GetUpdateDays() )
			return;
	}
	if (WSAAsyncGetHostByName(m_hWnd, WM_MVERSIONCHECK_RESPONSE, "morphvercheck.dyndns.info", m_acMVCDNSBuffer, sizeof(m_acMVCDNSBuffer)) == 0){
		AddLogLine(true,GetResString(IDS_NEWVERSIONFAILED));
	}
}
//MORPH END   - Added by SiRoB, New Version check

void CALLBACK CemuleDlg::StartupTimer(HWND hwnd, UINT uiMsg, UINT idEvent, DWORD dwTime)
{
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	try
	{
		switch(theApp.emuledlg->status){
			case 0:
				theApp.emuledlg->status++;
				theApp.emuledlg->ready = true;
				theApp.sharedfiles->SetOutputCtrl(&theApp.emuledlg->sharedfileswnd->sharedfilesctrl);
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
				
				//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-] [MoNKi: -UPnPNAT Support-]
				theApp.emuledlg->serverwnd->UpdateMyInfo();
				//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-] [MoNKi: -UPnPNAT Support-]

				if (!bError) // show the success msg, only if we had no serious error
					AddLogLine(true, GetResString(IDS_MAIN_READY) + _T(" %s"),theApp.m_strCurVersionLong + _T(" [") + theApp.m_strModLongVersion + _T("]"),GetResString(IDS_TRANSVERSION));  //MORPH - Changed by milobac, Translation version info

				if(thePrefs.DoAutoConnect())
					theApp.emuledlg->OnBnClickedButton2();
				theApp.emuledlg->status++;
				break;
			}
			case 5:
				break;
			default:
				theApp.emuledlg->StopTimer();
		}
	}
	CATCH_DFLT_EXCEPTIONS(_T("CemuleDlg::StartupTimer"))
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
	//MORPH START - Added by SiRoB, New Version check
	DoMVersioncheck(false);
	//MORPH END   - Added by SiRoB, New Version check
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
		//MORPH START - Added by SiRoB, New Version check
		case MP_MVERSIONCHECK:
			DoMVersioncheck(true);
			break;
		//MORPH END   - Added by SiRoB, New Version check
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
		//I'm not a Gui person.. If you know a better way to do this, go for it.. :)
		if (m_bStartMinimized)
		{
			if(theApp.DidWeAutoStart())
			{
				if (thePrefs.mintotray == false)
				{
					thePrefs.mintotray = true;
					OnCancel();
					thePrefs.mintotray = false;
				}
				else
			OnCancel();
	}
			else
			{
				OnCancel();
			}
		}
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
	Debug(_T("%s\n"), txt);
#endif

#ifdef _DEBUG
	// we have some emtpy log messages somewhere.. remove that check, after it fired..
	ASSERT( !txt.IsEmpty() );
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
		status = _T("eD2K:")+GetResString(IDS_CONNECTED);
	else if (theApp.serverconnect->IsConnecting())
		status = _T("eD2K:")+GetResString(IDS_CONNECTING);
	else
		status = _T("eD2K:")+GetResString(IDS_NOTCONNECTED);

	//Most likley needs a rewrite
	if(Kademlia::CKademlia::isConnected())
		status += _T("|Kad:")+GetResString(IDS_CONNECTED);
	else if (Kademlia::CKademlia::isRunning())
		status += _T("|Kad:")+GetResString(IDS_CONNECTING);
	else
		status += _T("|Kad:")+GetResString(IDS_NOTCONNECTED);
	*/
	if(theApp.serverconnect->IsConnected())
		status = _T("ED2K");
	else if (theApp.serverconnect->IsConnecting())
		status = _T("ed2k");
	else
		status = _T("");

	if(Kademlia::CKademlia::isConnected())
		status += status.IsEmpty()?_T("KAD"):_T(" | KAD");
	else if (Kademlia::CKademlia::isRunning())
		status += status.IsEmpty()?_T("kad"):_T(" | kad");
	//MORPH END   - Changed by SiRoB, Don't know why but arceling reporting

	statusbar->SetTipText(3,status);
	statusbar->SetText(status,3,0);

	if (theApp.IsConnected())
	{
		TCHAR szBuf[200];
		_stprintf(szBuf, _T("%s"), GetResString(IDS_MAIN_BTN_DISCONNECT));
		LPTSTR pszBuf;
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
			TCHAR szBuf[200];
			_stprintf(szBuf, _T("%s"), GetResString(IDS_MAIN_BTN_CANCEL));
			LPTSTR pszBuf;
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
			TCHAR szBuf[200];
			_stprintf(szBuf, _T("%s"), GetResString(IDS_MAIN_BTN_CONNECT));
			LPTSTR pszBuf;
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
	buffer.Format(_T("%s:%s(%s)|%s:%s(%s)"), GetResString(IDS_UUSERS), CastItoIShort(totaluser, false, 1), CastItoIShort(Kademlia::CKademlia::getKademliaUsers(), false, 1), GetResString(IDS_FILES), CastItoIShort(totalfile, false, 1), CastItoIShort(Kademlia::CKademlia::getKademliaFiles(), false, 1));
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
	TCHAR buffer[50];

	if (forceAll)
		m_lasticoninfo=255;

	lastdownrate=theApp.downloadqueue->GetDatarate();
	lastuprate=theApp.uploadqueue->GetDatarate();

	if( thePrefs.ShowOverhead() )
		_stprintf(buffer, GetResString(IDS_UPDOWN),
				  (float)lastuprate/1024, 
				  (float)theStats.GetUpDatarateOverhead()/1024, 
				  (float)lastdownrate/1024, 
				  (float)theStats.GetDownDatarateOverhead()/1024);
	else
		_stprintf(buffer,GetResString(IDS_UPDOWNSMALL),
				  (float)lastuprate/1024, 
				  (float)lastdownrate/1024);
	
	if (TrayIsVisible() || forceAll){
		TCHAR buffer2[100];
		// set trayicon-icon
		int DownRateProcent=(int)ceil ( (lastdownrate/10.24)/ thePrefs.GetMaxGraphDownloadRate());
		if (DownRateProcent>100)
			DownRateProcent=100;
			UpdateTrayIcon(DownRateProcent);

		//MORPH START - Added by IceCream, Correct the bug of the download speed shown in the systray
		if (theApp.IsConnected())
			_sntprintf(buffer2,ARRSIZE(buffer2),_T("[%s] (%s)\r\n%s"),theApp.m_strModLongVersion,GetResString(IDS_CONNECTED),buffer);
		else 
			_sntprintf(buffer2,ARRSIZE(buffer2),_T("[%s] (%s)\r\n%s"),theApp.m_strModLongVersion,GetResString(IDS_DISCONNECTED),buffer);
		//MORPH END   - Added by IceCream, Correct the bug of the download speed shown in the systray

		buffer2[63]=0;
		TraySetToolTip(buffer2);
	}

	if (IsWindowVisible() || forceAll) {
		//MORPH START - Added by SiRoB, Show zz ratio activation
		if (thePrefs.IsZZRatioDoesWork()){
			TCHAR buffer2[100];		
			_sntprintf(buffer2,ARRSIZE(buffer2),theApp.downloadqueue->IsZZRatioInWork()?_T("%s R"):_T("%s r"),buffer);
			statusbar->SetText(buffer2,2,0);
		}else
		//MORPH END   - Added by SiRoB, Show zz ratio activation
		statusbar->SetText(buffer,2,0);
		ShowTransferStateIcon();
	}
	if (IsWindowVisible() && thePrefs.ShowRatesOnTitle()) {
		 //MORPH START - Changed by SiRoB, [itsonlyme: -modname-]
		/*
		_sntprintf(buffer,ARRSIZE(buffer),_T("(U:%.1f D:%.1f) eMule v%s [%s]"),(float)lastuprate/1024, (float)lastdownrate/1024,theApp.m_strCurVersionLong,theApp.m_strModLongVersion);
		*/
		_sntprintf(buffer,ARRSIZE(buffer),_T("(U:%.1f D:%.1f) eMule v%s [%s]"),(float)lastuprate/1024, (float)lastdownrate/1024,theApp.m_strCurVersionLong,theApp.m_strModLongVersion);
		 //MORPH END   - Changed by SiRoB, [itsonlyme: -modname-]
		SetWindowText(buffer);
	}
}

// ZZ:UploadSpeedSense -->
void CemuleDlg::ShowPing() {
	if(IsWindowVisible()) {
        CurrentPingStruct lastPing = theApp.lastCommonRouteFinder->GetCurrentPing();

        CString buffer;

		//MORPH START - Changed by SiRoB, Related to SUC &  USS
        if(thePrefs.IsDynUpEnabled()) {
            if(lastPing.state.GetLength() == 0) {
				if(lastPing.lowest > 0 && !thePrefs.IsDynUpUseMillisecondPingTolerance()) {
					buffer.Format(_T("%s | %ims | %i%%"),CastItoXBytes(lastPing.currentLimit,false,true),lastPing.latency, lastPing.latency*100/lastPing.lowest);
				} else {
					buffer.Format(_T("%s | %ims | %ims"),CastItoXBytes(lastPing.currentLimit,false,true),lastPing.latency, thePrefs.GetDynUpPingToleranceMilliseconds());
				}
			} else {
                buffer.SetString(lastPing.state);
            }
		} else if (thePrefs.IsSUCDoesWork())
			buffer.Format(_T("vur:%s r:%i"),CastItoXBytes(theApp.uploadqueue->GetMaxVUR(),false,true),theApp.uploadqueue->GetAvgRespondTime(0));
		//MORPH END   - Changed by SiRoB, Related to SUC &  USS
		statusbar->SetText(buffer,4,0);
    }
}
// ZZ:UploadSpeedSense <--

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

void CemuleDlg::SetActiveDialog(CWnd* dlg)
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
	//MORPH START - Changed by SiRoB, Related to SUC
	/*
	if(thePrefs.IsDynUpEnabled())
	{
        if(thePrefs.IsDynUpUseMillisecondPingTolerance()) {
            ussShift = 45;
        } else {
            ussShift = 90;
        }
	}
	int aiWidths[5] = { rect.right-675-ussShift, rect.right-440-ussShift, rect.right-250-ussShift,rect.right-25-ussShift, -1 };
	*/
	if(thePrefs.IsDynUpEnabled() || thePrefs.IsSUCEnabled())
	{
		ussShift = 150;
	}
	int aiWidths[5] = { rect.right-525-ussShift, rect.right-315-ussShift, rect.right-115-ussShift, rect.right-25-ussShift, -1 };
	//MORPH END   - Added by SiRoB, Related to SUC
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
		link2.Replace(_T("%7c"),_T("|"));
		link = OptUtf8ToStr(URLDecode(link2));
		CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(link);
		_ASSERT( pLink !=0 );
		switch (pLink->GetKind()) {
		case CED2KLink::kFile:
			{
				//MORPH START - Changed by SiRoB, Selection category support khaos::categorymod+
				/*
				CED2KFileLink* pFileLink = pLink->GetFileLink();
				_ASSERT(pFileLink !=0);
				theApp.downloadqueue->AddFileLinkToDownload(pFileLink,searchwnd->GetSelectedCat());
				*/
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
				CServer* pSrv = new CServer(pSrvLink->GetPort(), ipstr(pSrvLink->GetIP()));
				_ASSERT( pSrv !=0 );
				pSrvLink->GetDefaultName(defName);
				pSrv->SetListName(defName.GetBuffer());

				// Barry - Default all new servers to high priority
				if (thePrefs.GetManualHighPrio())
					pSrv->SetPreference(SRV_PR_HIGH);

				if (!serverwnd->serverlistctrl.AddServer(pSrv,true)) 
					delete pSrv; 
				else
					AddLogLine(true,GetResString(IDS_SERVERADDED), pSrv->GetListName());
			}
			break;
		// MORPH START - Added by Commander, Friendlinks [emulEspaña]
		case CED2KLink::kFriend:
			{
				// Better with dynamic_cast, but no RTTI enabled in the project
				CED2KFriendLink* pFriendLink = static_cast<CED2KFriendLink*>(pLink);
				uchar userHash[16];
				pFriendLink->GetUserHash(userHash);

				if ( ! theApp.friendlist->IsAlreadyFriend(userHash) )
					theApp.friendlist->AddFriend(userHash, 0U, 0U, 0U, 0U, pFriendLink->GetUserName(), 1U);
				else
				{
					CString msg;
					msg.Format(GetResString(IDS_USER_ALREADY_FRIEND), pFriendLink->GetUserName());
					theApp.AddLogLine(true, msg);
				}
			}
			break;
		case CED2KLink::kFriendList:
			{
				// Better with dynamic_cast, but no RTTI enabled in the project
				CED2KFriendListLink* pFrndLstLink = static_cast<CED2KFriendListLink*>(pLink);
				CString sAddress = pFrndLstLink->GetAddress(); 
				if ( !sAddress.IsEmpty() )
					chatwnd->UpdateEmfriendsMetFromURL(sAddress);
			}
			break;
		// MORPH END - Added by Commander, Friendlinks [emulEspaña]
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
	PCOPYDATASTRUCT data = (PCOPYDATASTRUCT)lParam;
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
		CString clcommand((LPCTSTR)data->lpData);
		clcommand.MakeLower();
		AddLogLine(true,_T("CLI: %s"),clcommand);

		if (clcommand==_T("connect")) {StartConnection(); return true;}
		if (clcommand==_T("disconnect")) {theApp.serverconnect->Disconnect(); return true;}
		if (clcommand==_T("resume")) {theApp.downloadqueue->StartNextFile(); return true;}
		if (clcommand==_T("exit")) {OnClose(); return true;}
		if (clcommand==_T("restore")) {RestoreWindow();return true;}
		if (clcommand.Left(7).MakeLower()==_T("limits=") && clcommand.GetLength()>8) {
			CString down;
			CString up=clcommand.Mid(7);
			int pos=up.Find(_T(','));
			if (pos>0) {
				down=up.Mid(pos+1);
				up=up.Left(pos);
			}
			if (down.GetLength()>0) thePrefs.SetMaxDownload(_tstoi(down));
			if (up.GetLength()>0) thePrefs.SetMaxUpload(_tstoi(up));

			return true;
		}

		if (clcommand==_T("help") || clcommand==_T("/?")) {
			// show usage
			return true;
		}

		if (clcommand==_T("status")) {
			CString strBuff;
			strBuff.Format(_T("%sstatus.log"),thePrefs.GetAppDir());
			FILE* file = _tfsopen(strBuff, _T("wt"), _SH_DENYWR);
			if (file){
				if (theApp.serverconnect->IsConnected())
					strBuff = GetResString(IDS_CONNECTED);
				else if (theApp.serverconnect->IsConnecting())
					strBuff = GetResString(IDS_CONNECTING);
				else
					strBuff = GetResString(IDS_DISCONNECTED);
				_ftprintf(file, _T("%s\n"), strBuff);

				strBuff.Format(GetResString(IDS_UPDOWNSMALL), (float)theApp.uploadqueue->GetDatarate()/1024, (float)theApp.downloadqueue->GetDatarate()/1024);
				_ftprintf(file, _T("%s"), strBuff); // next string (getTextList) is already prefixed with '\n'!
				_ftprintf(file, _T("%s\n"), transferwnd->downloadlistctrl.getTextList());
				
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
		return FALSE;

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
	return TRUE;
}

LRESULT CemuleDlg::OnFileOpProgress(WPARAM wParam, LPARAM lParam)
{
	if (theApp.m_app_state == APP_STATE_SHUTINGDOWN)
		return FALSE;

	CKnownFile* pKnownFile = (CKnownFile*)lParam;
	ASSERT( pKnownFile->IsKindOf(RUNTIME_CLASS(CKnownFile)) );

	if (pKnownFile->IsKindOf(RUNTIME_CLASS(CPartFile)))
	{
		CPartFile* pPartFile = static_cast<CPartFile*>(pKnownFile);
		pPartFile->SetFileOpProgress(wParam);
		pPartFile->UpdateDisplayedInfo(true);
	}

	return 0;
}

// SLUGFILLER: SafeHash
LRESULT CemuleDlg::OnHashFailed(WPARAM wParam, LPARAM lParam)
{
	theApp.sharedfiles->HashFailed((UnknownFile_Struct*)lParam);
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

	// try to wait untill the hashing thread notices that we are shutting down
	CSingleLock sLock1(&theApp.hashing_mut); // only one filehash at a time
	sLock1.Lock(2000);

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

	theApp.m_pPeerCache->Save();
	theApp.scheduler->RestoreOriginals();
	thePrefs.Save();
	thePerfLog.Shutdown();

	//EastShare START - Pretender, TBH-AutoBackup
	if (thePrefs.GetAutoBackup2())
		theApp.ppgbackup->Backup3();
	if (thePrefs.GetAutoBackup())
	{
		theApp.ppgbackup->Backup(_T("*.ini"), false);
		theApp.ppgbackup->Backup(_T("*.dat"), false);
		theApp.ppgbackup->Backup(_T("*.met"), false);
	}
	//EastShare END - Pretender, TBH-AutoBackup

	if(thePrefs.GetInvisibleMode()) UnRegisterInvisibleHotKey(); //Commander - Added: Invisible Mode [TPT]

	// explicitly delete all listview items which may hold ptrs to objects which will get deleted
	// by the dtors (some lines below) to avoid potential problems during application shutdown.
	transferwnd->downloadlistctrl.DeleteAllItems();
	chatwnd->chatselector.DeleteAllItems();
	theApp.clientlist->DeleteAll();
	searchwnd->DeleteAllSearchListCtrlItems();
	sharedfileswnd->sharedfilesctrl.DeleteAllItems();
    transferwnd->queuelistctrl.DeleteAllItems();
	transferwnd->clientlistctrl.DeleteAllItems();
	transferwnd->uploadlistctrl.DeleteAllItems();
	
	CPartFileConvert::CloseGUI();
	CPartFileConvert::RemoveAllJobs();

	theApp.uploadBandwidthThrottler->EndThread();
	// ZZ:UploadSpeedSense -->
	theApp.lastCommonRouteFinder->EndThread();
	// ZZ:UploadSpeedSense <--

	theApp.sharedfiles->DeletePartFileInstances();

	searchwnd->SendMessage(WM_CLOSE);

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
	delete theApp.m_pPeerCache;		theApp.m_pPeerCache = NULL;
	delete theApp.m_pFirewallOpener;theApp.m_pFirewallOpener = NULL;
	//EastShare Start - added by AndCycle, IP to Country
	delete theApp.ip2country;		theApp.ip2country = NULL;
	//EastShare End   - added by AndCycle, IP to Country
	//MORPH - Added by Yun.SF3, ZZ Upload System
	delete theApp.uploadBandwidthThrottler; theApp.uploadBandwidthThrottler = NULL;
	delete theApp.lastCommonRouteFinder; theApp.lastCommonRouteFinder = NULL;
	//MORPH - Added by SiRoB, ZZ Upload system (USS)

	//MORPH - Added by SiRoB, More clean :|
	delete theApp.FakeCheck;		theApp.FakeCheck = NULL;
        
    //MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
	delete theApp.wapserver;		theApp.wapserver = NULL;
	//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]

	thePrefs.Uninit();
	theApp.m_app_state = APP_STATE_DONE;
	CTrayDialog::OnCancel();
	//EastShare, Added by linekin HotKey
	UnregisterHotKey(this->m_hWnd,100); 
	//EastShare, Added by linekin HotKey
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
		case IDC_EXIT:
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
	CString text;

	// creating UploadPopup Menu
	ASSERT( m_menuUploadCtrl.m_hMenu == NULL );
	if (m_menuUploadCtrl.CreateMenu())
	{
		//m_menuUploadCtrl.AddMenuTitle(GetResString(IDS_PW_TIT_UP));
		text.Format(_T("20%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphUploadRate()*0.2),GetResString(IDS_KBYTESEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U20,  text);
		text.Format(_T("40%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphUploadRate()*0.4),GetResString(IDS_KBYTESEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U40,  text);
		text.Format(_T("60%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphUploadRate()*0.6),GetResString(IDS_KBYTESEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U60,  text);
		text.Format(_T("80%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphUploadRate()*0.8),GetResString(IDS_KBYTESEC));	m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U80,  text);
		text.Format(_T("100%%\t%i %s"), (uint16)(thePrefs.GetMaxGraphUploadRate()),GetResString(IDS_KBYTESEC));		m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_U100, text);
		m_menuUploadCtrl.AppendMenu(MF_SEPARATOR);
	
		if (GetRecMaxUpload()>0) {
			text.Format(GetResString(IDS_PW_MINREC)+GetResString(IDS_KBYTESEC),GetRecMaxUpload());
			m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_UP10, text );
		}

		//m_menuUploadCtrl.AppendMenu(MF_STRING, MP_QS_UPC, GetResString(IDS_PW_UNLIMITED));

		text.Format(_T("%s:"), GetResString(IDS_PW_UPL));
		addToMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_menuUploadCtrl.m_hMenu, text);
	}

	// creating DownloadPopup Menu
	ASSERT( m_menuDownloadCtrl.m_hMenu == NULL );
	if (m_menuDownloadCtrl.CreateMenu())
	{
		//m_menuDownloadCtrl.AddMenuTitle(GetResString(IDS_PW_TIT_DOWN));
		text.Format(_T("20%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.2),GetResString(IDS_KBYTESEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D20,  text);
		text.Format(_T("40%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.4),GetResString(IDS_KBYTESEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D40,  text);
		text.Format(_T("60%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.6),GetResString(IDS_KBYTESEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D60,  text);
		text.Format(_T("80%%\t%i %s"),  (uint16)(thePrefs.GetMaxGraphDownloadRate()*0.8),GetResString(IDS_KBYTESEC));	m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D80,  text);
		text.Format(_T("100%%\t%i %s"), (uint16)(thePrefs.GetMaxGraphDownloadRate()),GetResString(IDS_KBYTESEC));		m_menuDownloadCtrl.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D100, text);
		//m_menuDownloadCtrl.AppendMenu(MF_SEPARATOR);
		//m_menuDownloadCtrl.AppendMenu(MF_STRING, MP_QS_DC, GetResString(IDS_PW_UNLIMITED));

		// Show DownloadPopup Menu
		text.Format(_T("%s:"), GetResString(IDS_PW_DOWNL));
		addToMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_menuDownloadCtrl.m_hMenu, text);
	}
	addToMenu->AppendMenu(MF_SEPARATOR);
	addToMenu->AppendMenu(MF_STRING, MP_CONNECT, GetResString(IDS_MAIN_BTN_CONNECT));
	addToMenu->AppendMenu(MF_STRING, MP_DISCONNECT, GetResString(IDS_MAIN_BTN_DISCONNECT)); 
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

void CemuleDlg::CloseConnection()
{
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

void CemuleDlg::RestoreWindow()
{
	if (TrayIsVisible())
		TrayHide();	
	
	ShowWindow(SW_SHOW);
}

void CemuleDlg::UpdateTrayIcon(int procent)
{
	// compute an id of the icon to be generated
	uint8 m_newiconinfo=(procent>0)?(16-((procent*15/100)+1)):0;

	if (theApp.IsConnected()){
		if (!theApp.IsFirewalled())
			m_newiconinfo += 50;
	} else
		m_newiconinfo += 100;

	// dont update if the same icon as displayed would be generated
	//MORPH START - Changed by SiRoB, Blinking Tray Icon On Message Recieve
	/*
	if ( m_lasticoninfo==m_newiconinfo)
		return;
	*/
	static bool messageIcon = false;
	if ( m_lasticoninfo==m_newiconinfo && m_iMsgIcon == 0 && messageIcon)
		return;
	//MORPH START - Changed by SiRoB, Blinking Tray Icon On Message Recieve


	m_lasticoninfo=m_newiconinfo;

    //Commander - Added: Blinking Tray Icon On Message Recieve [emulEspaña] - Start
	//static bool messageIcon = false;
	if(m_iMsgIcon == 0 || !messageIcon){
	//Commander - Added: Blinking Tray Icon On Message Recieve [emulEspaña] - End
	if (theApp.IsConnected()){
		if (theApp.IsFirewalled())
			trayIcon.Init(sourceTrayIconLow,100,1,1,16,16,thePrefs.GetStatsColor(11));
		else 
			trayIcon.Init(sourceTrayIcon,100,1,1,16,16,thePrefs.GetStatsColor(11));
		}
	else
		trayIcon.Init(sourceTrayIconGrey,100,1,1,16,16,thePrefs.GetStatsColor(11));
	//Commander - Added: Blinking Tray Icon On Message Recieve [emulEspaña] - Start
	}
	else {
		if (theApp.IsConnected()){
			if (theApp.IsFirewalled())
				trayIcon.Init(sourceTrayMessageLow,100,1,1,16,16,thePrefs.GetStatsColor(11));
			else 
				trayIcon.Init(sourceTrayMessage,100,1,1,16,16,thePrefs.GetStatsColor(11));
		}
		else
			trayIcon.Init(sourceTrayMessageGrey,100,1,1,16,16,thePrefs.GetStatsColor(11));
	}
	messageIcon = !messageIcon;
	//Commander - Added: Blinking Tray Icon On Message Recieve [emulEspaña] - End

	// load our limit and color info
	int pLimits16[1] = {100}; // set the limits of where the bar color changes (low-high)
	COLORREF pColors16[1] = {thePrefs.GetStatsColor(11)}; // set the corresponding color for each level
	trayIcon.SetColorLevels(pLimits16,pColors16,1);

	// generate the icon (destroy these icon using DestroyIcon())
	int pVals16[1] = {procent};
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

void CemuleDlg::OnShowWindow( BOOL bShow, UINT nStatus) {
	if (IsRunning())
		ShowTransferRate(true);
}

void CemuleDlg::ShowNotifier(CString Text, int MsgType, LPCTSTR pszLink, bool bForceSoundOFF)
{
	if (!notifierenabled)
		return;

	LPCTSTR pszSoundEvent = NULL;
	int iSoundPrio = 0;
	bool ShowIt = false;
	switch (MsgType)
	{
		case TBN_CHAT:
            if (thePrefs.GetUseChatNotifier())
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;
				pszSoundEvent = _T("eMule_Chat");
				iSoundPrio = 1;
			}
			break;
		case TBN_DLOAD:
            if (thePrefs.GetUseDownloadNotifier())
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_DownloadFinished");
				iSoundPrio = 1;
			}
			break;
		case TBN_DLOADADDED:
            if (thePrefs.GetUseNewDownloadNotifier())
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_DownloadAdded");
				iSoundPrio = 1;
			}
			break;
		case TBN_LOG:
            if (thePrefs.GetUseLogNotifier())
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_LogEntryAdded");
			}
			break;
		case TBN_IMPORTANTEVENT:
			if (thePrefs.GetNotifierPopOnImportantError())
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_Urgent");
				iSoundPrio = 1;
			}
			break;

		case TBN_NEWVERSION:
			if (thePrefs.GetNotifierPopOnNewVersion())
			{
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;			
				pszSoundEvent = _T("eMule_NewVersion");
				iSoundPrio = 1;
			}
			break;
		//MORPH START - Added by SiRoB, New Version Check
		case TBN_NEWMVERSION:
			{	
				m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
				ShowIt = true;
				pszSoundEvent = _T("startup.wav");
				iSoundPrio = 1;
			}
			break;
		//MORPH END   - Added by SiRoB, New Version Check
		case TBN_NULL:
            m_wndTaskbarNotifier->Show(Text, MsgType, pszLink);
			ShowIt = true;			
			break;
	}
	
	if (ShowIt && !bForceSoundOFF)
	{
		if (thePrefs.GetUseSoundInNotifier())
		{
        PlaySound(thePrefs.GetNotifierWavSoundPath(), NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
		}
		else if (pszSoundEvent)
		{
			// use 'SND_NOSTOP' only for low priority events, otherwise the 'Log message' event may overrule a more important
			// event which is fired nearly at the same time.
			PlaySound(pszSoundEvent, NULL, SND_APPLICATION | SND_ASYNC | SND_NODEFAULT | SND_NOWAIT | ((iSoundPrio > 0) ? 0 : SND_NOSTOP));
		}
	}
}

void CemuleDlg::LoadNotifier(CString configuration)
{
	notifierenabled = m_wndTaskbarNotifier->LoadConfiguration(configuration);
}

LRESULT CemuleDlg::OnTaskbarNotifierClicked(WPARAM wParam,LPARAM lParam)
{
	if (lParam)
	{
		LPTSTR pszLink = (LPTSTR)lParam;
		ShellOpenFile(pszLink);
		free(pszLink);
		pszLink = NULL;
	}

	switch (m_wndTaskbarNotifier->GetMessageType())
	{
		case TBN_CHAT:
			RestoreWindow();
			SetActiveDialog(chatwnd);
			break;

		case TBN_DLOAD:
			// if we had a link and opened the downloaded file and if we currently in traybar, dont restore the app window
			if (lParam==0 || !TrayIsVisible())
			{
				RestoreWindow();
				SetActiveDialog(transferwnd);
			}
			break;

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

		case TBN_NEWVERSION:
		{
			CString theUrl;
			theUrl.Format(_T("/en/version_check.php?version=%i&language=%i"),theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
			theUrl = thePrefs.GetVersionCheckBaseURL()+theUrl;
			ShellExecute(NULL, NULL, theUrl, NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
			break;
		}
		//MORPH START - Added by SiRoB, New Version Check
		case TBN_NEWMVERSION:
		{
			ShellExecute(NULL, NULL, _T("http://emulemorph.sourceforge.net/"), NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
			break;
		}
		//MORPH END   - Added by SiRoB, New Version Check
	}
    return 0;
}

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
	connicons[0] = theApp.LoadIcon(_T("ConnectedNotNot"), 16, 16);
	connicons[1] = theApp.LoadIcon(_T("ConnectedNotLow"), 16, 16);
	connicons[2] = theApp.LoadIcon(_T("ConnectedNotHigh"), 16, 16);
	connicons[3] = theApp.LoadIcon(_T("ConnectedLowNot"), 16, 16);
	connicons[4] = theApp.LoadIcon(_T("ConnectedLowLow"), 16, 16);
	connicons[5] = theApp.LoadIcon(_T("ConnectedLowHigh"), 16, 16);
	connicons[6] = theApp.LoadIcon(_T("ConnectedHighNot"), 16, 16);
	connicons[7] = theApp.LoadIcon(_T("ConnectedHighLow"), 16, 16);
	connicons[8] = theApp.LoadIcon(_T("ConnectedHighHigh"), 16, 16);
	ShowConnectionStateIcon();

	// transfer state
	if (transicons[0]) VERIFY( ::DestroyIcon(transicons[0]) );
	if (transicons[1]) VERIFY( ::DestroyIcon(transicons[1]) );
	if (transicons[2]) VERIFY( ::DestroyIcon(transicons[2]) );
	if (transicons[3]) VERIFY( ::DestroyIcon(transicons[3]) );
	transicons[0] = theApp.LoadIcon(_T("UP0DOWN0"), 16, 16);
	transicons[1] = theApp.LoadIcon(_T("UP0DOWN1"), 16, 16);
	transicons[2] = theApp.LoadIcon(_T("UP1DOWN0"), 16, 16);
	transicons[3] = theApp.LoadIcon(_T("UP1DOWN1"), 16, 16);
	ShowTransferStateIcon();

	// users state
	if (usericon) VERIFY( ::DestroyIcon(usericon) );
	usericon = theApp.LoadIcon(_T("StatsClients"), 16, 16);
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
	imicons[1] = theApp.LoadIcon(_T("Message"), 16, 16);
	imicons[2] = theApp.LoadIcon(_T("MessagePending"), 16, 16);
	ShowMessageState(m_iMsgIcon);
}

void CemuleDlg::Localize()
{
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu)
	{
		VERIFY( pSysMenu->ModifyMenu(MP_ABOUTBOX, MF_BYCOMMAND | MF_STRING, MP_ABOUTBOX, GetResString(IDS_ABOUTBOX)) );
		VERIFY( pSysMenu->ModifyMenu(MP_VERSIONCHECK, MF_BYCOMMAND | MF_STRING, MP_VERSIONCHECK, GetResString(IDS_VERSIONCHECK)) );
		//MORPH START - Added by SiRoB, New Version check
		VERIFY( pSysMenu->ModifyMenu(MP_MVERSIONCHECK, MF_BYCOMMAND | MF_STRING, MP_MVERSIONCHECK, GetResString(IDS_MVERSIONCHECK)) );
		//MORPH END   - Added by SiRoB, New Version check

		switch (thePrefs.GetWindowsVersion()){
		case _WINVER_98_:
		case _WINVER_95_:
		case _WINVER_ME_:
			// NOTE: I think the reason why the old version of the following code crashed under Win9X was because
			// of the menus were destroyed right after they were added to the system menu. New code should work
			// under Win9X too but I can't test it.
			break;
		default:{
			// localize the 'speed control' sub menus by deleting the current menus and creating a new ones.

			// remove any already available 'speed control' menus from system menu
			UINT uOptMenuPos = pSysMenu->GetMenuItemCount() - 1;
			CMenu* pAccelMenu = pSysMenu->GetSubMenu(uOptMenuPos);
			if (pAccelMenu)
			{
				ASSERT( pAccelMenu->m_hMenu == m_SysMenuOptions.m_hMenu );
				VERIFY( pSysMenu->RemoveMenu(uOptMenuPos, MF_BYPOSITION) );
				pAccelMenu = NULL;
			}

			// destroy all 'speed control' menus
			if (m_menuUploadCtrl)
				VERIFY( m_menuUploadCtrl.DestroyMenu() );
			if (m_menuDownloadCtrl)
				VERIFY( m_menuDownloadCtrl.DestroyMenu() );
			if (m_SysMenuOptions)
				VERIFY( m_SysMenuOptions.DestroyMenu() );

			// create new 'speed control' menus
			if (m_SysMenuOptions.CreateMenu())
			{
				AddSpeedSelectorSys(&m_SysMenuOptions);
				pSysMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)m_SysMenuOptions.m_hMenu, GetResString(IDS_EM_PREFS));
			}
		  }
		}
	}

	ShowUserStateIcon();
	toolbar->Localize();
	ShowConnectionState();
	ShowTransferRate(true);
	ShowUserCount();
	CPartFileConvert::Localize();
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
			ShellExecute(NULL, _T("open"), thePrefs.GetIncomingDir(),NULL, NULL, SW_SHOW); 
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

	CHAR szClassName[MAX_PATH];
	if (::GetClassNameA(*pWnd, szClassName, ARRSIZE(szClassName)))
	{
		bool bButton = (__ascii_stricmp(szClassName, "Button") == 0);

		if (   (__ascii_stricmp(szClassName, "EDIT") == 0 && (pWnd->GetExStyle() & WS_EX_STATICEDGE))
			|| __ascii_stricmp(szClassName, "SysListView32") == 0
			|| __ascii_stricmp(szClassName, "msctls_trackbar32") == 0
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

	CHAR szClassName[MAX_PATH];
	if (::GetClassNameA(*pWnd, szClassName, ARRSIZE(szClassName)))
	{
		bool bButton = (__ascii_stricmp(szClassName, "Button") == 0);

//		if (   !bButton
//			//|| (__ascii_stricmp(szClassName, "SysListView32") == 0 && (pWnd->GetStyle() & WS_BORDER) == 0)
//			|| __ascii_stricmp(szClassName, "msctls_trackbar32") == 0
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
							theUrl.Format(_T("/en/version_check.php?version=%i&language=%i"),theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
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
//MORPH START - Added by SiRoB, New Version check
LRESULT CemuleDlg::OnMVersionCheckResponse(WPARAM wParam, LPARAM lParam)
{
	if (WSAGETASYNCERROR(lParam) == 0)
	{
		int iBufLen = WSAGETASYNCBUFLEN(lParam);
		if (iBufLen >= sizeof(HOSTENT))
		{
			LPHOSTENT pHost = (LPHOSTENT)m_acMVCDNSBuffer;
			if (pHost->h_length == 4 && pHost->h_addr_list && pHost->h_addr_list[0])
			{
				uint32 dwResult = ((LPIN_ADDR)(pHost->h_addr_list[0]))->s_addr;
				uint8 abyCurVer[4] = { 1, MOD_VERSION_MIN, MOD_VERSION_MJR, 0};
				dwResult &= 0x00FFFFFF;
				if (dwResult > *(uint32*)abyCurVer){
					thePrefs.UpdateLastVC();
					SetActiveWindow();
					AddLogLine(true,GetResString(IDS_NEWMVERSIONAVL));
					ShowNotifier(GetResString(IDS_NEWMVERSIONAVLPOPUP), TBN_NEWMVERSION);
					if (AfxMessageBox(GetResString(IDS_NEWMVERSIONAVL)+GetResString(IDS_VISITMVERSIONCHECK),MB_YESNO)==IDYES) {
						ShellExecute(NULL, NULL, _T("http://emulemorph.sourceforge.net/"), NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
					}
				}
				else{
					thePrefs.UpdateLastVC();
					AddLogLine(true,GetResString(IDS_NONEWMVERVERSION));
				}
				return 0;
			}
		}
	}
	AddLogLine(true,GetResString(IDS_NEWVERSIONFAILED));
	return 0;
}
//MORPH END   - Added by SiRoB, New Version check

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

	if (searchwnd && searchwnd->m_hWnd)
	{
//		searchwnd->SendMessage(WM_IDLEUPDATECMDUI);

		if (theApp.m_app_state != APP_STATE_SHUTINGDOWN)
		{
			//extern void Mfc_IdleUpdateCmdUiTopLevelFrameList(CWnd* pMainFrame);
			//Mfc_IdleUpdateCmdUiTopLevelFrameList(this);
			theApp.OnIdle(0/*lIdleCount*/);	// NOTE: DO **NOT** CALL THIS WITH 'lIdleCount>0'
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

void CemuleDlg::HtmlHelp(DWORD_PTR dwData, UINT nCmd)
{
	CWinApp* pApp = AfxGetApp();
	ASSERT_VALID(pApp);
	ASSERT(pApp->m_pszHelpFilePath != NULL);
	// to call HtmlHelp the m_fUseHtmlHelp must be set in
	// the application's constructor
	ASSERT(pApp->m_eHelpType == afxHTMLHelp);

	CWaitCursor wait;

	PrepareForHelp();

	// need to use top level parent (for the case where m_hWnd is in DLL)
	CWnd* pWnd = GetTopLevelParent();

	TRACE(traceAppMsg, 0, _T("HtmlHelp: pszHelpFile = '%s', dwData: $%lx, fuCommand: %d.\n"), pApp->m_pszHelpFilePath, dwData, nCmd);

	bool bHelpError = false;
	CString strHelpError;
	int iTry = 0;
	while (iTry++ < 2)
	{
		if (!AfxHtmlHelp(pWnd->m_hWnd, pApp->m_pszHelpFilePath, nCmd, dwData))
		{
			bHelpError = true;
			strHelpError.LoadString(AFX_IDP_FAILED_TO_LAUNCH_HELP);

			typedef struct tagHH_LAST_ERROR
			{
				int      cbStruct;
				HRESULT  hr;
				BSTR     description;
			} HH_LAST_ERROR;
			HH_LAST_ERROR hhLastError = {0};
			hhLastError.cbStruct = sizeof hhLastError;
			HWND hwndResult = AfxHtmlHelp(pWnd->m_hWnd, NULL, HH_GET_LAST_ERROR, reinterpret_cast<DWORD>(&hhLastError));
			if (hwndResult != 0)
			{
				if (FAILED(hhLastError.hr))
				{
					if (hhLastError.description)
					{
						USES_CONVERSION;
						strHelpError = OLE2T(hhLastError.description);
						::SysFreeString(hhLastError.description);
					}
					if (   hhLastError.hr == 0x8004020A  /*no topics IDs available in Help file*/
						|| hhLastError.hr == 0x8004020B) /*requested Help topic ID not found*/
					{
						// try opening once again without help topic ID
						if (nCmd != HH_DISPLAY_TOC)
						{
							nCmd = HH_DISPLAY_TOC;
							dwData = 0;
							continue;
						}
					}
				}
			}
			break;
		}
		else
		{
			bHelpError = false;
			strHelpError.Empty();
			break;
		}
	}

	if (bHelpError)
	{
		if (AfxMessageBox(CString(pApp->m_pszHelpFilePath) + _T("\n\n") + strHelpError + _T("\n\n") + GetResString(IDS_ERR_NOHELP), MB_YESNO | MB_ICONERROR) == IDYES)
		{
			CString strUrl = thePrefs.GetHomepageBaseURL() + _T("/home/perl/help.cgi");
			ShellExecute(NULL, NULL, strUrl, NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
		}
	}
}

LRESULT CemuleDlg::OnPeerCacheResponse(WPARAM wParam, LPARAM lParam)
{
	return theApp.m_pPeerCache->OnPeerCacheCheckResponse(wParam,lParam);
}


//Commander - Added: Invisible Mode [TPT] - Start
LRESULT CemuleDlg::OnHotKey(WPARAM wParam, LPARAM lParam)
{
	//MORPH START - Changed by SiRoB, Toggle Show Hide window
	/*
	if(wParam == HOTKEY_INVISIBLEMODE_ID) RestoreWindow();
	// Allows "invisible mode" on multiple instances of eMule
	// Restore the rest of hidden emules
	EnumWindows(AskEmulesForInvisibleMode, INVMODE_RESTOREWINDOW);
	*/
	if(wParam == HOTKEY_INVISIBLEMODE_ID)
		if(b_HideApp = !b_HideApp)
		{
			EnumWindows(AskEmulesForInvisibleMode, INVMODE_HIDEWINDOW);
		}
		else
		{
			EnumWindows(AskEmulesForInvisibleMode, INVMODE_RESTOREWINDOW);
		}
	//MORPH END   - Changed by SiRoB, Toggle Show Hide window
	return 0;
}
//MORPH - Added by SiRoB, Toggle Show Hide window
void CemuleDlg::ToggleHide()
{
	b_HideApp = true;
	b_TrayWasVisible = TrayIsVisible();
	TrayHide();
	ShowWindow(SW_HIDE);
}
void CemuleDlg::ToggleShow()
{
	b_HideApp = false;
	if(b_TrayWasVisible)
		TrayShow();
	else
		ShowWindow(SW_SHOW);
}
//MORPH - Added by SiRoB, Toggle Show Hide window
BOOL CemuleDlg::RegisterInvisibleHotKey()
{
	if(m_hWnd && IsRunning()){
		bool res = RegisterHotKey( this->m_hWnd, HOTKEY_INVISIBLEMODE_ID ,
						   thePrefs.GetInvisibleModeHKKeyModifier(),
						   thePrefs.GetInvisibleModeHKKey());
		return res;
	} else
		return false;
}

BOOL CemuleDlg::UnRegisterInvisibleHotKey()
{
	if(m_hWnd){
		bool res = !(UnregisterHotKey(this->m_hWnd, HOTKEY_INVISIBLEMODE_ID));

		// Allows "invisible mode" on multiple instances of eMule
		// Only one app (eMule) can register the hotkey, if we unregister, we need
		// to register the hotkey in other emule.
		EnumWindows(AskEmulesForInvisibleMode, INVMODE_REGISTERHOTKEY);
		return res;
	} else
		return false;
}

// Allows "invisible mode" on multiple instances of eMule
// LOWORD(WPARAM) -> HotKey KeyModifier
// HIWORD(WPARAM) -> HotKey VirtualKey
// LPARAM		  -> int:	INVMODE_RESTOREWINDOW	-> Restores the window
//							INVMODE_REGISTERHOTKEY	-> Registers the hotkey
LRESULT CemuleDlg::OnRestoreWindowInvisibleMode(WPARAM wParam, LPARAM lParam)
{
	if (thePrefs.GetInvisibleMode() &&
		(UINT)LOWORD(wParam) == thePrefs.GetInvisibleModeHKKeyModifier() &&
		(char)HIWORD(wParam) == thePrefs.GetInvisibleModeHKKey()) {
			switch(lParam){
				case INVMODE_RESTOREWINDOW:
					//MORPH - Changed by SiRoB, Toggle Show Hide window
					/*
					RestoreWindow();
					*/
					ToggleShow();
					break;
				case INVMODE_REGISTERHOTKEY:
					RegisterInvisibleHotKey();
					break;
				//MORPH START - Added by SiRoB, Toggle Show Hide window
				case INVMODE_HIDEWINDOW:
					ToggleHide();
				//MORPH END   - Added by SiRoB, Toggle Show Hide window
			
			}
			return UWM_RESTORE_WINDOW_IM;
	} else
		return false;
} 

// Allows "invisible mode" on multiple instances of eMule
BOOL CALLBACK CemuleDlg::AskEmulesForInvisibleMode(HWND hWnd, LPARAM lParam){
	DWORD dwMsgResult;
	WPARAM msgwParam;

	msgwParam=MAKEWPARAM(thePrefs.GetInvisibleModeHKKeyModifier(),
				thePrefs.GetInvisibleModeHKKey());

	LRESULT res = ::SendMessageTimeout(hWnd,UWM_RESTORE_WINDOW_IM, msgwParam, lParam,
				SMTO_BLOCK |SMTO_ABORTIFHUNG,10000,&dwMsgResult);
	
	return res; 
} 
//Commander - Added: Invisible Mode [TPT] - End