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
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
#include "stdafx.h"
#include <locale.h>
#include <io.h>
#include <share.h>
#include "secrunasuser.h" // yonatan - moved up... // MORPH - Modified by Commander, WebCache 1.2e
#include "emule.h"
#include "opcodes.h"
#include "mdump.h"
#include "Scheduler.h"
#include "SearchList.h"
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/kademlia/Prefs.h"
#include "kademlia/kademlia/Error.h"
#include "kademlia/utils/UInt128.h"
#include "PerfLog.h"
#include <..\src\mfc\sockimpl.h>
#include <..\src\mfc\afximpl.h>
#include "LastCommonRouteFinder.h"
#include "UploadBandwidthThrottler.h"
#include "ClientList.h"
#include "FriendList.h"
#include "ClientUDPSocket.h"
#include "DownloadQueue.h"
#include "IPFilter.h"
#include "MMServer.h"
#include "Statistics.h"
#include "OtherFunctions.h"
#include "WebServer.h"
#include "WapServer/WapServer.h" //MORPH - Added by SiRoB / Commander, Wapserver [emulEspaña]
#include "UploadQueue.h"
#include "SharedFileList.h"
#include "ServerList.h"
#include "Sockets.h"
#include "ListenSocket.h"
#include "ClientCredits.h"
#include "KnownFileList.h"
#include "Server.h"
#include "UpDownClient.h"
#include "ED2KLink.h"
#include "Preferences.h"
//#include "secrunasuser.h" // MORPH - Modified by Commander, WebCache 1.2e
#include "SafeFile.h"
#include "PeerCacheFinder.h"
#include "WebCache/WebCache.h" // jp detect webcache on startup // MORPH - Added by Commander, WebCache 1.2e
#include "WebCache/WebCacheMFRList.h"	// Superlexx - MFR
#include "emuleDlg.h"
#include "SearchDlg.h"
#include "enbitmap.h"
#include "FirewallOpener.h"
#include "StringConversion.h"
#include "Log.h"
#include "Collection.h"
#include "fakecheck.h" //MORPH - Added by SiRoB
// Commander - Added: Custom incoming folder icon [emulEspaña] - Start
#include "Ini2.h"
// Commander - Added: Custom incoming folder icon [emulEspaña] - End

CLogFile theLog;
CLogFile theVerboseLog;
bool g_bLowColorDesktop = false;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//#define USE_16COLOR_ICONS


///////////////////////////////////////////////////////////////////////////////
// MSLU (Microsoft Layer for Unicode) support - UnicoWS
// 
HMODULE g_hUnicoWS = NULL;
bool g_bUnicoWS = false;

void ShowUnicowsError()
{
	// NOTE: Do *NOT* use any MFC nor W-functions here!
	// NOTE: Do *NOT* use eMule's localization functions here!
	MessageBoxA(NULL,
				"This eMule version requires the \"Microsoft(R) Layer for Unicode(TM) on Windows(R) 95/98/ME Systems\".\r\n"
				"\r\n"
				"Download the MSLU package from Microsoft(R) here:\r\n"
				"        http://www.microsoft.com/downloads/details.aspx?FamilyId=73BA7BD7-ED06-4F0D-80A4-2A7EEAEE17E2\r\n"
				"or\r\n"
				"        visit the eMule Project Download Page http://www.emule-project.net/home/perl/general.cgi?rm=download\r\n"
				"or\r\n"
				"        search the Microsoft(R) Download Center http://www.microsoft.com/downloads/ for \"MSLU\" or \"unicows\"."
				"\r\n"
				"\r\n"
				"\r\n"
				"After downloading the MSLU package, run the \"unicows.exe\" program and specify your eMule installation folder "
				"where to place the extracted files from the package.\r\n"
				"\r\n"
				"Ensure that the file \"unicows.dll\" was placed in your eMule installation folder and start eMule again.",
				"eMule",
				MB_ICONSTOP | MB_SYSTEMMODAL | MB_SETFOREGROUND);
}

extern "C" HMODULE __stdcall ExplicitPreLoadUnicows()
{
#ifdef _AFXDLL
	// UnicoWS support *requires* statically linked MFC and C-RTL.

	// NOTE: Do *NOT* use any MFC nor W-functions here!
	// NOTE: Do *NOT* use eMule's localization functions here!
	MessageBoxA(NULL, 
				"This eMule version (Unicode, MSLU, shared MFC) does not run with this version of Windows.", 
				"eMule", 
				MB_ICONSTOP | MB_SYSTEMMODAL | MB_SETFOREGROUND);
	exit(1);
#endif

	// Pre-Load UnicoWS -- needed for proper initialization of MFC/C-RTL
	HMODULE g_hUnicoWS = LoadLibraryA("unicows.dll");
	if (g_hUnicoWS == NULL)
	{
		ShowUnicowsError();
		exit(1);
	}

	g_bUnicoWS = true;
	return g_hUnicoWS;
}

// NOTE: Do *NOT* change the name of this function. It *HAS* to be named "_PfnLoadUnicows" !
extern "C" HMODULE (__stdcall *_PfnLoadUnicows)(void) = &ExplicitPreLoadUnicows;


///////////////////////////////////////////////////////////////////////////////
// C-RTL Memory Debug Support
// 
#ifdef _DEBUG
static CMemoryState oldMemState, newMemState, diffMemState;

_CRT_ALLOC_HOOK g_pfnPrevCrtAllocHook = NULL;
CMap<const unsigned char*, const unsigned char*, UINT, UINT> g_allocations;
int eMuleAllocHook(int mode, void* pUserData, size_t nSize, int nBlockUse, long lRequest, const unsigned char* pszFileName, int nLine);

//CString _strCrtDebugReportFilePath(_T("eMule CRT Debug Log.txt"));
// don't use a CString for that memory - it will not be available on application termination!
#define APP_CRT_DEBUG_LOG_FILE _T("eMule CRT Debug Log.txt")
static TCHAR _szCrtDebugReportFilePath[MAX_PATH] = APP_CRT_DEBUG_LOG_FILE;
#endif //_DEBUG


struct SLogItem
{
	UINT uFlags;
    CString line;
};

void CALLBACK myErrHandler(Kademlia::CKademliaError *error)
{
	CString msg;
	msg.Format(_T("\r\nError 0x%08X : %hs\r\n"), error->m_ErrorCode, error->m_ErrorDescription);
	if(theApp.emuledlg && theApp.emuledlg->IsRunning())
		theApp.QueueDebugLogLine(false, _T("%s"), msg);
}

void CALLBACK myDebugAndLogHandler(LPCSTR lpMsg)
{
	if(theApp.emuledlg && theApp.emuledlg->IsRunning())
		theApp.QueueDebugLogLine(false, _T("%hs"), lpMsg);
}

void CALLBACK myLogHandler(LPCSTR lpMsg)
{
	if(theApp.emuledlg && theApp.emuledlg->IsRunning())
		theApp.QueueLogLine(false, _T("%hs"), lpMsg);
}

const static UINT UWM_ARE_YOU_EMULE=RegisterWindowMessage(EMULE_GUID);

// CemuleApp

BEGIN_MESSAGE_MAP(CemuleApp, CWinApp)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


CemuleApp::CemuleApp(LPCTSTR lpszAppName)
	:CWinApp(lpszAppName)
{
	srand(time(NULL));
	m_dwPublicIP = 0;
	m_bAutoStart = false;

	m_ullComCtrlVer = MAKEDLLVERULL(4,0,0,0);
	m_hSystemImageList = NULL;
	m_sizSmallSystemIcon.cx = 16;
	m_sizSmallSystemIcon.cy = 16;
	m_iDfltImageListColorFlags = ILC_COLOR;

// MOD Note: Do not change this part - Merkur

	// this is the "base" version number <major>.<minor>.<update>.<build>
	m_dwProductVersionMS = MAKELONG(CemuleApp::m_nVersionMin, CemuleApp::m_nVersionMjr);
	m_dwProductVersionLS = MAKELONG(CemuleApp::m_nVersionBld, CemuleApp::m_nVersionUpd);

	// create a string version (e.g. "0.30a")
	ASSERT( CemuleApp::m_nVersionUpd + 'a' <= 'f' );
#ifdef _DEBUG
	m_strCurVersionLong.Format(_T("%u.%u%c.%u"), CemuleApp::m_nVersionMjr, CemuleApp::m_nVersionMin, _T('a') + CemuleApp::m_nVersionUpd, CemuleApp::m_nVersionBld);
#else
	m_strCurVersionLong.Format(_T("%u.%u%c"), CemuleApp::m_nVersionMjr, CemuleApp::m_nVersionMin, _T('a') + CemuleApp::m_nVersionUpd);
#endif

#ifdef _DEBUG
	m_strCurVersionLong += _T(" DEBUG");
#endif
#ifdef _BETA
	m_strCurVersionLong += _T(" BETA");
#endif

	// create the protocol version number
	CString strTmp;
	strTmp.Format(_T("0x%u"), m_dwProductVersionMS);
	VERIFY( _stscanf(strTmp, _T("0x%x"), &m_uCurVersionShort) == 1 );
	ASSERT( m_uCurVersionShort < 0x99 );

	// create the version check number
	strTmp.Format(_T("0x%u%c"), m_dwProductVersionMS, _T('A') + CemuleApp::m_nVersionUpd);
	VERIFY( _stscanf(strTmp, _T("0x%x"), &m_uCurVersionCheck) == 1 );
	ASSERT( m_uCurVersionCheck < 0x999 );
// MOD Note: end

	m_bGuardClipboardPrompt = false;

	EnableHtmlHelp();

	//MORPH START - Added by SiRoB, [-modname-]
	m_strModVersion = CemuleApp::m_szMVersion;
	m_strModVersion.AppendFormat(_T(" %u.%u"), CemuleApp::m_nMVersionMjr, CemuleApp::m_nMVersionMin);
	if (CemuleApp::m_szMMVersion[0]!=0)
		m_strModVersion.AppendFormat(_T(" %s"), CemuleApp::m_szMMVersion);
	m_strModLongVersion = CemuleApp::m_szMVersionLong;
	m_strModLongVersion.AppendFormat(_T("%u.%u"), CemuleApp::m_nMVersionMjr, CemuleApp::m_nMVersionMin);
	if (CemuleApp::m_szMMVersion[0]!=0)
		m_strModLongVersion.AppendFormat(_T(" %s"), CemuleApp::m_szMMVersion);
	//MORPH END   - Added by SiRoB, [-modname-]
	//MORPH START - Added by SiRoB, [MoNKi: -UPnPNAT Support-]
	m_UPnP_IGDControlPoint = CUPnP_IGDControlPoint::GetInstance();
	//MORPH END   - Added by SiRoB, [MoNKi: -UPnPNAT Support-]
}


CemuleApp theApp(_T("eMule"));


// Workaround for buggy 'AfxSocketTerm' (needed at least for MFC 7.0)
#if _MFC_VER==0x0700 || _MFC_VER==0x0710
void __cdecl __AfxSocketTerm()
{
#if defined(_AFXDLL) && (_MFC_VER==0x0700 || _MFC_VER==0x0710)
	VERIFY( WSACleanup() == 0 );
#else
	_AFX_SOCK_STATE* pState = _afxSockState.GetData();
	if (pState->m_pfnSockTerm != NULL){
		VERIFY( WSACleanup() == 0 );
		pState->m_pfnSockTerm = NULL;
	}
#endif
}
#else
#error "You are using an MFC version which may require a special version of the above function!"
#endif

// CemuleApp Initialisierung

BOOL CemuleApp::InitInstance()
{
#ifdef _DEBUG
	// set Floating Point Processor to throw several exceptions, in particular the 'Floating point devide by zero'
	UINT uEmCtrlWord = _control87(0, 0) & _MCW_EM;
	_control87(uEmCtrlWord & ~(/*_EM_INEXACT |*/ _EM_UNDERFLOW | _EM_OVERFLOW | _EM_ZERODIVIDE | _EM_INVALID), _MCW_EM);

	// output all ASSERT messages to debug device
	_CrtSetReportMode(_CRT_ASSERT, _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_REPORT_MODE) | _CRTDBG_MODE_DEBUG);
#endif

	TCHAR szAppDir[MAX_PATH];
	VERIFY( GetModuleFileName(m_hInstance, szAppDir, ARRSIZE(szAppDir)) );
	VERIFY( PathRemoveFileSpec(szAppDir) );
	
	TCHAR szConfigDir[MAX_PATH];
	PathCombine(szConfigDir, szAppDir, CONFIGFOLDER);
	::CreateDirectory(szConfigDir, NULL);
	
	TCHAR szPrefFilePath[MAX_PATH];
	PathCombine(szPrefFilePath, szAppDir, CONFIGFOLDER _T("preferences.ini"));
	if (m_pszProfileName)
		free((void*)m_pszProfileName);
	m_pszProfileName = _tcsdup(szPrefFilePath);


#ifdef _DEBUG
	oldMemState.Checkpoint();
	// Installing that memory debug code works fine in Debug builds when running within VS Debugger,
	// but some other test applications don't like that all....
	//g_pfnPrevCrtAllocHook = _CrtSetAllocHook(&eMuleAllocHook);
#endif
	//afxMemDF = allocMemDF | delayFreeMemDF;


	///////////////////////////////////////////////////////////////////////////
	// Install crash dump creation
	//
#ifndef _BETA
	if (GetProfileInt(_T("eMule"), _T("CreateCrashDump"), 0))
#endif
		theCrashDumper.Enable(_T("eMule ") + m_strCurVersionLong, true);


	///////////////////////////////////////////////////////////////////////////
	// Locale initialization -- BE VERY CAREFUL HERE!!!
	//
	_tsetlocale(LC_ALL, _T(""));		// set all categories of locale to user-default ANSI code page obtained from the OS.
	_tsetlocale(LC_NUMERIC, _T("C"));	// set numeric category to 'C'
	//_tsetlocale(LC_CTYPE, _T("C"));		// set character types category to 'C' (VERY IMPORTANT, we need binary string compares!)

	AfxOleInit();

	pstrPendingLink = NULL;
	if (ProcessCommandline())
		return false;

	extern bool CheckThreadLocale();
	if (!CheckThreadLocale())
		return false;

	///////////////////////////////////////////////////////////////////////////
	// Common Controls initialization
	//
	InitCommonControls();
	DWORD dwComCtrlMjr = 4;
	DWORD dwComCtrlMin = 0;
	AtlGetCommCtrlVersion(&dwComCtrlMjr, &dwComCtrlMin);
	m_ullComCtrlVer = MAKEDLLVERULL(dwComCtrlMjr,dwComCtrlMin,0,0);
	if (m_ullComCtrlVer < MAKEDLLVERULL(5,8,0,0))
	{
		if (GetProfileInt(_T("eMule"), _T("CheckComctl32"), 1)) // just in case some user's can not install that package and have to survive without it..
		{
			if (AfxMessageBox(GetResString(IDS_COMCTRL32_DLL_TOOOLD), MB_ICONSTOP | MB_YESNO) == IDYES)
				ShellOpenFile(_T("http://www.microsoft.com/downloads/details.aspx?FamilyID=cb2cf3a2-8025-4e8f-8511-9b476a8d35d2"));

			// No need to exit eMule, it will most likely work as expected but it will have some GUI glitches here and there..
		}
	}

	DWORD dwShellMjr = 4;
	DWORD dwShellMin = 0;
	AtlGetShellVersion(&dwShellMjr, &dwShellMin);
	ULONGLONG ullShellVer = MAKEDLLVERULL(dwShellMjr,dwShellMin,0,0);
	if (ullShellVer < MAKEDLLVERULL(4,7,0,0))
	{
		if (GetProfileInt(_T("eMule"), _T("CheckShell32"), 1)) // just in case some user's can not install that package and have to survive without it..
		{
			AfxMessageBox(_T("Windows Shell library (SHELL32.DLL) is too old!\r\n\r\neMule detected a version of the \"Windows Shell library (SHELL32.DLL)\" which is too old to be properly used by eMule. To ensure full and flawless functionality of eMule we strongly recommend to update the \"Windows Shell library (SHELL32.DLL)\" to at least version 4.7.\r\n\r\nDownload and install an update of the \"Windows Shell library (SHELL32.DLL)\" at Microsoft (R) Download Center."), MB_ICONSTOP);

			// No need to exit eMule, it will most likely work as expected but it will have some GUI glitches here and there..
		}
	}

	m_sizSmallSystemIcon.cx = GetSystemMetrics(SM_CXSMICON);
	m_sizSmallSystemIcon.cy = GetSystemMetrics(SM_CYSMICON);
	UpdateDesktopColorDepth();

	if (g_bLowColorDesktop)
	{
		m_iDfltImageListColorFlags = ILC_COLOR4;
	}
	else
	{
		m_iDfltImageListColorFlags = GetAppImageListColorFlag();
		// don't use 32bit color resources if not supported by commctl
		if (m_iDfltImageListColorFlags == ILC_COLOR32 && m_ullComCtrlVer < MAKEDLLVERULL(6,0,0,0))
			m_iDfltImageListColorFlags = ILC_COLOR16;
		// don't use >8bit color resources with OSs with restricted memory for GDI resources
		if (afxData.bWin95)
			m_iDfltImageListColorFlags = ILC_COLOR8;
	}

	CWinApp::InitInstance();

	if (!AfxSocketInit())
	{
		AfxMessageBox(GetResString(IDS_SOCKETS_INIT_FAILED));
		return FALSE;
	}
#if _MFC_VER==0x0700 || _MFC_VER==0x0710
	atexit(__AfxSocketTerm);
#else
#error "You are using an MFC version which may require a special version of the above function!"
#endif
	AfxEnableControlContainer();
	if (!AfxInitRichEdit2()){
		if (!AfxInitRichEdit())
			AfxMessageBox(_T("Fatal Error: No Rich Edit control library found!")); // should never happen..
	}

	if (!Kademlia::CKademlia::initUnicode(AfxGetInstanceHandle())){
		AfxMessageBox(_T("Fatal Error: Failed to load Unicode character tables for Kademlia!")); // should never happen..
		return FALSE; // DO *NOT* START !!!
	}

	extern bool SelfTest();
	if (!SelfTest())
		return FALSE; // DO *NOT* START !!!

	// create & initalize all the important stuff 
	thePrefs.Init();
	theStats.Init();

	// check if we have to restart eMule as Secure user
	if (thePrefs.IsRunAsUserEnabled()){
		CSecRunAsUser rau;
		eResult res = rau.RestartSecure();
		if (res == RES_OK_NEED_RESTART)
			return FALSE; // emule restart as secure user, kill this instance
		else if (res == RES_FAILED){
			// something went wrong
			theApp.QueueLogLine(false, GetResString(IDS_RAU_FAILED), rau.GetCurrentUserW()); 
		}
	}

	if (thePrefs.GetRTLWindowsLayout())
		EnableRTLWindowsLayout();

	//MORPH START - Added by IceCream, high process priority
	if (thePrefs.GetEnableHighProcess())
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	//MORPH END   - Added by IceCream, high process priority

#ifdef _DEBUG
	_sntprintf(_szCrtDebugReportFilePath, ARRSIZE(_szCrtDebugReportFilePath), _T("%s%s"), thePrefs.GetLogDir(), APP_CRT_DEBUG_LOG_FILE);
#endif
	VERIFY( theLog.SetFilePath(thePrefs.GetLogDir() + _T("eMule.log")) );
	VERIFY( theVerboseLog.SetFilePath(thePrefs.GetLogDir() + _T("eMule_Verbose.log")) );
	theLog.SetMaxFileSize(thePrefs.GetMaxLogFileSize());
	theLog.SetFileFormat(thePrefs.GetLogFileFormat());
	theVerboseLog.SetMaxFileSize(thePrefs.GetMaxLogFileSize());
	theVerboseLog.SetFileFormat(thePrefs.GetLogFileFormat());
	if (thePrefs.GetLog2Disk()){
		theLog.Open();
		theLog.Log(_T("\r\n"));
	}
	if (thePrefs.GetDebug2Disk()){
		theVerboseLog.Open();
		theVerboseLog.Log(_T("\r\n"));
	}
	Log(_T("Starting eMule v%s"), m_strCurVersionLong);

	CemuleDlg dlg;
	emuledlg = &dlg;
	m_pMainWnd = &dlg;

	//MORPH START - Added by Commander, Custom incoming / temp folder icon [emulEspaña]
	if(thePrefs.ShowFolderIcons()){
		theApp.AddIncomingFolderIcon();
		theApp.AddTempFolderIcon();
	}
	//MORPH END   - Added by Commander, Custom incoming / temp folder icon [emulEspaña]

	// Barry - Auto-take ed2k links
	if (thePrefs.AutoTakeED2KLinks())
		Ask4RegFix(false, true, false);

	if (thePrefs.GetAutoStart())
		::AddAutoStart();
	else
		::RemAutoStart();

	m_pFirewallOpener = new CFirewallOpener();
	m_pFirewallOpener->Init(true); // we need to init it now (even if we may not use it yet) because of CoInitializeSecurity - which kinda ruins the sense of the class interface but ooohh well :P

	//MORPH START - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
	if(!thePrefs.GetICFSupport() && thePrefs.GetICFSupportFirstTime() && m_pFirewallOpener->DoesFWConnectionExist()){ 	 
		if(MessageBox(NULL, GetResString(IDS_ICFSUPPORTFIRST), _T("eMule"), MB_YESNO | MB_ICONQUESTION) == IDYES){ 	 
			thePrefs.SetICFSupport(true); 	 
        } 	 
		thePrefs.SetICFSupportFirstTime(false); 	 
	}
	//MORPH END   - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]

	// Open WinXP firewallports if set in preferences and possible
	//MORPH START - Changed by SiRoB, [MoNKi: -Random Ports-]
	/*
	if (thePrefs.IsOpenPortsOnStartupEnabled()){
	*/
	if (thePrefs.IsOpenPortsOnStartupEnabled() || thePrefs.GetUseRandomPorts()){
	//MORPH END   - Changed by SiRoB, [MoNKi: -Random Ports-]
		if (m_pFirewallOpener->DoesFWConnectionExist()){
			//MORPH START - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
			/*
			// delete old rules added by eMule
			m_pFirewallOpener->RemoveRule(EMULE_DEFAULTRULENAME_UDP);
			m_pFirewallOpener->RemoveRule(EMULE_DEFAULTRULENAME_TCP);
			*/
			// delete old rules added by eMule
			m_pFirewallOpener->ClearOld();
			//MORPH END   - Changed by SiRoB, [MoNKi: -Improved ICS-Firewall support-]

			//MORPH START - Changed by SiRoB, [MoNKi: -Random Ports-]
			/*
			// open port for this session
			if (m_pFirewallOpener->OpenPort(thePrefs.GetPort(), NAT_PROTOCOL_TCP, EMULE_DEFAULTRULENAME_TCP, true))
				QueueLogLine(false, GetResString(IDS_FO_TEMPTCP_S), thePrefs.GetPort());
			else
				QueueLogLine(false, GetResString(IDS_FO_TEMPTCP_F), thePrefs.GetPort());

			if (thePrefs.GetUDPPort()){
				// open port for this session
				if (m_pFirewallOpener->OpenPort(thePrefs.GetUDPPort(), NAT_PROTOCOL_UDP, EMULE_DEFAULTRULENAME_UDP, true))
					QueueLogLine(false, GetResString(IDS_FO_TEMPUDP_S), thePrefs.GetUDPPort());
				else
					QueueLogLine(false, GetResString(IDS_FO_TEMPUDP_F), thePrefs.GetUDPPort());
			}
			*/
			//MORPH END   - Changed by SiRoB, [MoNKi: -Random Ports-]
		}
	}

	// emulEspaña: Added by MoNKi [MoNKi: -UPnPNAT Support-]
	if(m_UPnP_IGDControlPoint != NULL && thePrefs.IsUPnPEnabled()){
		m_UPnP_IGDControlPoint->Init(thePrefs.GetUPnPLimitToFirstConnection());
		if(thePrefs.GetUPnPClearOnClose() /*|| thePrefs.GetUseRandomPorts()*/)
			m_UPnP_IGDControlPoint->DeleteAllPortMappingsOnClose();
	}
	// End -UPnPNAT Support-

	// ZZ:UploadSpeedSense -->
    lastCommonRouteFinder = new LastCommonRouteFinder();
    uploadBandwidthThrottler = new UploadBandwidthThrottler();
	// ZZ:UploadSpeedSense <--

	clientlist = new CClientList();
	friendlist = new CFriendList();
	searchlist = new CSearchList();
	knownfiles = new CKnownFileList();
	serverlist = new CServerList();
	serverconnect = new CServerConnect(serverlist);
	sharedfiles = new CSharedFileList(serverconnect);
	listensocket = new CListenSocket();
	clientudp	= new CClientUDPSocket();
	clientcredits = new CClientCreditsList();
	downloadqueue = new CDownloadQueue();	// bugfix - do this before creating the uploadqueue
	uploadqueue = new CUploadQueue();
	ipfilter 	= new CIPFilter();
	ip2country = new CIP2Country(); //EastShare - added by AndCycle, IP to Country
	FakeCheck 	= new CFakecheck(); //MORPH - Added by milobac, FakeCheck, FakeReport, Auto-updating
	webserver = new CWebServer(); // Webserver [kuchin]
	wapserver = new CWapServer(); //MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
	mmserver = new CMMServer();
	scheduler = new CScheduler();
	m_pPeerCache = new CPeerCacheFinder();
	
	thePerfLog.Startup();
	dlg.DoModal();

	DisableRTLWindowsLayout();

	// Barry - Restore old registry if required
	if (thePrefs.AutoTakeED2KLinks())
		RevertReg();

	::CloseHandle(m_hMutexOneInstance);
#ifdef _DEBUG
	if (g_pfnPrevCrtAllocHook)
		_CrtSetAllocHook(g_pfnPrevCrtAllocHook);

	newMemState.Checkpoint();
	if (diffMemState.Difference(oldMemState, newMemState))
	{
		TRACE("Memory usage:\n");
		diffMemState.DumpStatistics();
	}
	//_CrtDumpMemoryLeaks();
#endif //_DEBUG

	emuledlg = NULL;

	ClearDebugLogQueue(true);
	ClearLogQueue(true);

	AddDebugLogLine(DLP_VERYLOW, _T("%hs: returning: FALSE"), __FUNCTION__);
	return FALSE;
}

int CemuleApp::ExitInstance()
{
	AddDebugLogLine(DLP_VERYLOW, _T("%hs"), __FUNCTION__);
	return CWinApp::ExitInstance();
}

#ifdef _DEBUG
int CrtDebugReportCB(int reportType, char* message, int* returnValue)
{
	FILE* fp = _tfsopen(_szCrtDebugReportFilePath, _T("a"), _SH_DENYWR);
	if (fp){
		time_t tNow = time(NULL);
		TCHAR szTime[40];
		_tcsftime(szTime, ARRSIZE(szTime), _T("%H:%M:%S"), localtime(&tNow));
		_ftprintf(fp, _T("%s  %u  %hs"), szTime, reportType, message);
		fclose(fp);
	}
	*returnValue = 0; // avoid invokation of 'AfxDebugBreak' in ASSERT macros
	return TRUE; // avoid further processing of this debug report message by the CRT
}

// allocation hook - for memory statistics gatering
int eMuleAllocHook(int mode, void* pUserData, size_t nSize, int nBlockUse, long lRequest, const unsigned char* pszFileName, int nLine)
{
	UINT count = 0;
	g_allocations.Lookup(pszFileName, count);
	if (mode == _HOOK_ALLOC) {
		_CrtSetAllocHook(g_pfnPrevCrtAllocHook);
		g_allocations.SetAt(pszFileName, count + 1);
		_CrtSetAllocHook(&eMuleAllocHook);
	}
	else if (mode == _HOOK_FREE){
		_CrtSetAllocHook(g_pfnPrevCrtAllocHook);
		g_allocations.SetAt(pszFileName, count - 1);
		_CrtSetAllocHook(&eMuleAllocHook);
	}
	return g_pfnPrevCrtAllocHook(mode, pUserData, nSize, nBlockUse, lRequest, pszFileName, nLine);
}
#endif

bool CemuleApp::ProcessCommandline()
{
	bool bIgnoreRunningInstances = (GetProfileInt(_T("eMule"), _T("IgnoreInstances"), 0) != 0);

	for (int i = 1; i < __argc; i++){
		LPCTSTR pszParam = __targv[i];
		if (pszParam[0] == _T('-') || pszParam[0] == _T('/')){
			pszParam++;
#ifdef _DEBUG
			if (_tcscmp(pszParam, _T("assertfile")) == 0)
				_CrtSetReportHook(CrtDebugReportCB);
#endif
			if (_tcscmp(pszParam, _T("ignoreinstances")) == 0)
				bIgnoreRunningInstances = true;

			if (_tcscmp(pszParam, _T("AutoStart")) == 0)
				m_bAutoStart = true;
		}
	}

	CCommandLineInfo cmdInfo;
    ParseCommandLine(cmdInfo);
    
	// If we create our TCP listen socket with SO_REUSEADDR, we have to ensure that there are
	// not 2 emules are running using the same port.
	// NOTE: This will not prevent from some other application using that port!
	UINT uTcpPort = GetProfileInt(_T("eMule"), _T("Port"), DEFAULT_TCP_PORT);
	CString strMutextName;
	strMutextName.Format(_T("%s:%u"), EMULE_GUID, uTcpPort);
	m_hMutexOneInstance = ::CreateMutex(NULL, FALSE, strMutextName);
	
	HWND maininst = NULL;
	bool bAlreadyRunning = false;
	if (!bIgnoreRunningInstances){
		bAlreadyRunning = ( ::GetLastError() == ERROR_ALREADY_EXISTS ||::GetLastError() == ERROR_ACCESS_DENIED);
    	if ( bAlreadyRunning ) EnumWindows(SearchEmuleWindow, (LPARAM)&maininst);
	}

    if (cmdInfo.m_nShellCommand == CCommandLineInfo::FileOpen) {
		CString* command = new CString(cmdInfo.m_strFileName);
		if (command->Find(_T("://"))>0) {
			sendstruct.cbData = (command->GetLength() + 1)*sizeof(TCHAR);
			sendstruct.dwData = OP_ED2KLINK; 
			sendstruct.lpData = command->GetBuffer(); 
    		if (maininst){
      			SendMessage(maininst, WM_COPYDATA, (WPARAM)0, (LPARAM)(PCOPYDATASTRUCT)&sendstruct);
				delete command;
      			return true; 
			} 
    		else 
      			pstrPendingLink = command;
		}
		else if (CCollection::HasCollectionExtention(*command)){
			sendstruct.cbData = (command->GetLength() + 1)*sizeof(TCHAR);
			sendstruct.dwData = OP_COLLECTION; 
			sendstruct.lpData = command->GetBuffer(); 
    		if (maininst){
      			SendMessage(maininst, WM_COPYDATA, (WPARAM)0, (LPARAM)(PCOPYDATASTRUCT)&sendstruct);
      			delete command;
				return true; 
			} 
    		else 
      			pstrPendingLink = command;
		}
		else {
			sendstruct.cbData = (command->GetLength() + 1)*sizeof(TCHAR);
			sendstruct.dwData = OP_CLCOMMAND;
			sendstruct.lpData = command->GetBuffer(); 
    		if (maininst){
      			SendMessage(maininst, WM_COPYDATA, (WPARAM)0, (LPARAM)(PCOPYDATASTRUCT)&sendstruct);
      			delete command;
				return true; 
			}
		}
    }
    return (maininst || bAlreadyRunning);
}

BOOL CALLBACK CemuleApp::SearchEmuleWindow(HWND hWnd, LPARAM lParam){
	DWORD dwMsgResult;
	LRESULT res = ::SendMessageTimeout(hWnd,UWM_ARE_YOU_EMULE,0, 0,SMTO_BLOCK |SMTO_ABORTIFHUNG,10000,&dwMsgResult);
	if(res == 0)
		return TRUE;
	if(dwMsgResult == UWM_ARE_YOU_EMULE){ 
		HWND * target = (HWND *)lParam;
		*target = hWnd;
		return FALSE; 
	} 
	return TRUE; 
} 


void CemuleApp::UpdateReceivedBytes(uint32 bytesToAdd) {
	SetTimeOnTransfer();
	theStats.sessionReceivedBytes+=bytesToAdd;
}

void CemuleApp::UpdateSentBytes(uint32 bytesToAdd, bool sentToFriend) {
	SetTimeOnTransfer();
	theStats.sessionSentBytes+=bytesToAdd;

    if(sentToFriend == true) {
	    theStats.sessionSentBytesToFriend += bytesToAdd;
    }
}

void CemuleApp::SetTimeOnTransfer() {
	if (theStats.transferStarttime>0) return;
	
	theStats.transferStarttime=GetTickCount();
}

CString CemuleApp::CreateED2kSourceLink(const CAbstractFile* f)
{
	if (!IsConnected() || IsFirewalled()){
		LogWarning(LOG_STATUSBAR, GetResString(IDS_SOURCELINKFAILED));
		return _T("");
	}
	uint32 dwID = GetID();

	CString strLink;
	strLink.Format(_T("ed2k://|file|%s|%u|%s|/|sources,%i.%i.%i.%i:%i|/"),
		EncodeUrlUtf8(StripInvalidFilenameChars(f->GetFileName(), false)),
		f->GetFileSize(),
		EncodeBase16(f->GetFileHash(),16),
		(uint8)dwID,(uint8)(dwID>>8),(uint8)(dwID>>16),(uint8)(dwID>>24), thePrefs.GetPort() );
	return strLink;
}

CString CemuleApp::CreateKadSourceLink(const CAbstractFile* f)
{
	CString strLink;
	if( Kademlia::CKademlia::isConnected() && theApp.clientlist->GetBuddy() && theApp.IsFirewalled() )
	{
		CString KadID;
		Kademlia::CKademlia::getPrefs()->getKadID().xor(Kademlia::CUInt128(true)).toHexString(&KadID);
		strLink.Format(_T("ed2k://|file|%s|%u|%s|/|kadsources,%s:%s|/"),
			EncodeUrlUtf8(StripInvalidFilenameChars(f->GetFileName(), false)),
			f->GetFileSize(),
			EncodeBase16(f->GetFileHash(),16),
			md4str(thePrefs.GetUserHash()), KadID);
	}
	return strLink;
}

//TODO: Move to emule-window
bool CemuleApp::CopyTextToClipboard(CString strText)
{
	if (strText.IsEmpty())
		return false;

	HGLOBAL hGlobalT = GlobalAlloc(GHND | GMEM_SHARE, (strText.GetLength() + 1) * sizeof(TCHAR));
	if (hGlobalT != NULL)
	{
		LPTSTR pGlobalT = static_cast<LPTSTR>(GlobalLock(hGlobalT));
		if (pGlobalT != NULL)
		{
			_tcscpy(pGlobalT, strText);
			GlobalUnlock(hGlobalT);
		}
		else
		{
			GlobalFree(hGlobalT);
			hGlobalT = NULL;
		}
	}

	CStringA strTextA(strText);
	HGLOBAL hGlobalA = GlobalAlloc(GHND | GMEM_SHARE, (strTextA.GetLength() + 1) * sizeof(CHAR));
	if (hGlobalA != NULL)
	{
		LPSTR pGlobalA = static_cast<LPSTR>(GlobalLock(hGlobalA));
		if (pGlobalA != NULL)
		{
			strcpy(pGlobalA, strTextA);
			GlobalUnlock(hGlobalA);
		}
		else
		{
			GlobalFree(hGlobalA);
			hGlobalA = NULL;
		}
	}

	if (hGlobalT == NULL && hGlobalA == NULL)
		return false;

	int iCopied = 0;
	if (OpenClipboard(NULL))
	{
		if (EmptyClipboard())
		{
			if (hGlobalT){
				if (SetClipboardData(CF_UNICODETEXT, hGlobalT) != NULL){
					iCopied++;
				}
				else{
					GlobalFree(hGlobalT);
					hGlobalT = NULL;
				}
			}
			if (hGlobalA){
				if (SetClipboardData(CF_TEXT, hGlobalA) != NULL){
					iCopied++;
				}
				else{
					GlobalFree(hGlobalA);
					hGlobalA = NULL;
				}
			}
		}
		CloseClipboard();
	}

	if (iCopied == 0)
	{
		if (hGlobalT){
			GlobalFree(hGlobalT);
			hGlobalT = NULL;
		}
		if (hGlobalA){
			GlobalFree(hGlobalA);
			hGlobalA = NULL;
		}
	}
	else
		IgnoreClipboardLinks(strText); // this is so eMule won't think the clipboard has ed2k links for adding

	return (iCopied != 0);
}

//TODO: Move to emule-window
CString CemuleApp::CopyTextFromClipboard()
{
	if (IsClipboardFormatAvailable(CF_UNICODETEXT))
	{
		if (OpenClipboard(NULL))
		{
			bool bResult = false;
			CString strClipboard;
			HGLOBAL hMem = GetClipboardData(CF_UNICODETEXT);
			if (hMem)
			{
				LPCWSTR pwsz = (LPCWSTR)GlobalLock(hMem);
				if (pwsz)
				{
					strClipboard = pwsz;
					GlobalUnlock(hMem);
					bResult = true;
				}
			}
			CloseClipboard();
			if (bResult)
				return strClipboard;
		}
	}

	if (!IsClipboardFormatAvailable(CF_TEXT))
		return _T("");
	if (!OpenClipboard(NULL))
		return _T("");

	CString	retstring;
	HGLOBAL	hglb = GetClipboardData(CF_TEXT);
	if (hglb != NULL)
	{
		LPCSTR lptstr = (LPCSTR)GlobalLock(hglb);
		if (lptstr != NULL)
		{
			retstring = lptstr;
			GlobalUnlock(hglb);
		}
	}
	CloseClipboard();

	return retstring;
}

void CemuleApp::OnlineSig() // Added By Bouc7 
{ 
	if (!thePrefs.IsOnlineSignatureEnabled())
		return;

	static const TCHAR _szFileName[] = _T("onlinesig.dat");
	CString strFullPath = thePrefs.GetAppDir();
	strFullPath += _szFileName;

	// The 'onlinesig.dat' is potentially read by other applications at more or less frequent intervals.
	//	 -	Set the file shareing mode to allow other processes to read the file while we are writing
	//		it (see also next point).
	//	 -	Try to write the hole file data at once, so other applications are always reading 
	//		a consistent amount of file data. C-RTL uses a 4 KB buffer, this is large enough to write
	//		those 2 lines into the onlinesig.dat file with one IO operation.
	//	 -	Although this file is a text file, we set the file mode to 'binary' because of backward 
	//		compatibility with older eMule versions.
    CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(strFullPath, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite | CFile::typeBinary, &fexp)){
		CString strError = GetResString(IDS_ERROR_SAVEFILE) + _T(" ") + CString(_szFileName);
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		fexp.GetErrorMessage(szError, ARRSIZE(szError));
		strError += _T(" - ");
		strError += szError;
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		return;
    }

	try
	{
		char buffer[20];
		CStringA strBuff;
		if (IsConnected()){
			file.Write("1",1); 
			file.Write("|",1);
			if(serverconnect->IsConnected()){
				strBuff = serverconnect->GetCurrentServer()->GetListName();
				file.Write(strBuff, strBuff.GetLength()); 
			}
			else{
				strBuff = "Kademlia";
				file.Write(strBuff,strBuff.GetLength()); 
			}

			file.Write("|",1); 
			if(serverconnect->IsConnected()){
				strBuff = serverconnect->GetCurrentServer()->GetFullIP();
				file.Write(strBuff,strBuff.GetLength()); 
			}
			else{
				strBuff = "0.0.0.0";
				file.Write(strBuff,strBuff.GetLength()); 
			}
			file.Write("|",1); 
			if(serverconnect->IsConnected()){
				itoa(serverconnect->GetCurrentServer()->GetPort(),buffer,10); 
				file.Write(buffer,strlen(buffer));
			}
			else{
				strBuff = "0";
				file.Write(strBuff,strBuff.GetLength());
			}
		} 
		else 
			file.Write("0",1); 

		file.Write("\n",1); 
		sprintf(buffer,"%.1f",(float)downloadqueue->GetDatarate()/1024); 
		file.Write(buffer,strlen(buffer)); 
		file.Write("|",1); 
		sprintf(buffer,"%.1f",(float)uploadqueue->GetDatarate()/1024); 
		file.Write(buffer,strlen(buffer)); 
		file.Write("|",1); 
		itoa(uploadqueue->GetWaitingUserCount(),buffer,10); 
		file.Write(buffer,strlen(buffer)); 

		file.Close(); 
	}
	catch (CFileException* ex)
	{
		CString strError = GetResString(IDS_ERROR_SAVEFILE) + _T(" ") + CString(_szFileName);
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		ex->GetErrorMessage(szError, ARRSIZE(szError));
		strError += _T(" - ");
		strError += szError;
		LogError(LOG_STATUSBAR, _T("%s"), strError);
		ex->Delete();
	}
} //End Added By Bouc7

bool CemuleApp::GetLangHelpFilePath(CString& strResult)
{
	// Change extension for help file
	CString strHelpFile = m_pszHelpFilePath;
	CFileFind ff;
	bool bFound;
	if (thePrefs.GetLanguageID() != MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT))
	{
		int pos = strHelpFile.ReverseFind(_T('\\'));   //CML
		CString temp;
		temp.Format(_T("%s\\eMule.%u.chm"), strHelpFile.Left(pos), thePrefs.GetLanguageID());
		if (pos>0)
			strHelpFile = temp;

		// if not exists, use original help (english)
		if (!ff.FindFile(strHelpFile, 0)){
			strHelpFile = m_pszHelpFilePath;
			bFound = false;
		}
		else
			bFound = true;
		strHelpFile.Replace(_T(".HLP"), _T(".chm"));
	}
	else{
		int pos = strHelpFile.ReverseFind(_T('\\'));
		CString temp;
		temp.Format(_T("%s\\eMule.chm"), strHelpFile.Left(pos));
		strHelpFile = temp;
		strHelpFile.Replace(_T(".HLP"), _T(".chm"));
		if (!ff.FindFile(strHelpFile, 0))
			bFound = false;
		else
			bFound = true;
	}
	ff.Close();
	strResult = strHelpFile;
	return bFound;
}

void CemuleApp::SetHelpFilePath(LPCTSTR pszHelpFilePath)
{
	if (m_pszHelpFilePath)
	{
		free((void*)m_pszHelpFilePath);
		m_pszHelpFilePath = NULL;
	}

	m_pszHelpFilePath = _tcsdup(pszHelpFilePath);
}

void CemuleApp::OnHelp()
{
	if (m_dwPromptContext != 0)
	{
		// do not call WinHelp when the error is failing to lauch help
		if (m_dwPromptContext != HID_BASE_PROMPT+AFX_IDP_FAILED_TO_LAUNCH_HELP)
			ShowHelp(m_dwPromptContext);
		return;
	}
	ShowHelp(0, HELP_CONTENTS);
}

void CemuleApp::ShowHelp(UINT uTopic, UINT uCmd)
{
	CString strHelpFilePath;
	bool bResult = GetLangHelpFilePath(strHelpFilePath);
	if (!bResult){
		if (ShowWebHelp())
			return;
	}
	SetHelpFilePath(strHelpFilePath);
	WinHelpInternal(uTopic, uCmd);
}

bool CemuleApp::ShowWebHelp(){
	uint16 nWebLanguage;
	switch(thePrefs.GetLanguageID()){
		// languages for we have a onlinehelp
		case 1031: nWebLanguage = 2; break;
        case 1036: nWebLanguage = 13; break;
        case 1033: nWebLanguage = 1; break;
		case 1028: nWebLanguage = 16; break;
        case 1034: nWebLanguage = 17; break;
        case 1040: nWebLanguage = 18; break;
        case 1043: nWebLanguage = 29; break;
        case 1046: nWebLanguage = 30; break;
		default:
			return false;
	}
	// onlinehelp unfortunatly doesnt supports context based help yet, since the topic IDs
	// differ for each language, maybe improved in later versions
	CString strHelpURL;
	strHelpURL.Format(_T("%s/home/perl/help.cgi?l=%u"), thePrefs.GetHomepageBaseURL(), nWebLanguage); 
	ShellExecute(NULL, NULL, strHelpURL, NULL, thePrefs.GetAppDir(), SW_SHOWDEFAULT);
	return true;
}

int CemuleApp::GetFileTypeSystemImageIdx(LPCTSTR pszFilePath, int iLength /* = -1 */)
{
	//TODO: This has to be MBCS aware..
	DWORD dwFileAttributes;
	LPCTSTR pszCacheExt = NULL;
	if (iLength == -1)
		iLength = _tcslen(pszFilePath);
	if (iLength > 0 && (pszFilePath[iLength - 1] == _T('\\') || pszFilePath[iLength - 1] == _T('/'))){
		// it's a directory
		pszCacheExt = _T("\\");
		dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
	}
	else{
		dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
		// search last '.' character *after* the last '\\' character
		for (int i = iLength - 1; i >= 0; i--){
			if (pszFilePath[i] == _T('\\') || pszFilePath[i] == _T('/'))
				break;
			if (pszFilePath[i] == _T('.')) {
				// point to 1st character of extension (skip the '.')
				pszCacheExt = &pszFilePath[i+1];
				break;
			}
		}
		if (pszCacheExt == NULL)
			pszCacheExt = _T("");	// empty extension
	}

	// Search extension in "ext->idx" cache.
	LPVOID vData;
	if (!m_aExtToSysImgIdx.Lookup(pszCacheExt, vData)){
		// Get index for the system's small icon image list
		SHFILEINFO sfi;
		DWORD dwResult = SHGetFileInfo(pszFilePath, dwFileAttributes, &sfi, sizeof(sfi),
									   SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
		if (dwResult == 0)
			return 0;
		ASSERT( m_hSystemImageList == NULL || m_hSystemImageList == (HIMAGELIST)dwResult );
		m_hSystemImageList = (HIMAGELIST)dwResult;

		// Store icon index in local cache
		m_aExtToSysImgIdx.SetAt(pszCacheExt, (LPVOID)sfi.iIcon);
		return sfi.iIcon;
	}
	// Return already cached value
	// Elandal: Assumes sizeof(void*) == sizeof(int)
	return (int)vData;
}

bool CemuleApp::IsConnected()
{
	return (theApp.serverconnect->IsConnected() || Kademlia::CKademlia::isConnected());
}

bool CemuleApp::IsPortchangeAllowed() {
	return ( theApp.clientlist->GetClientCount()==0 && !IsConnected() );
}

uint32 CemuleApp::GetID(){
	uint32 ID;
	if( Kademlia::CKademlia::isConnected() && !Kademlia::CKademlia::isFirewalled() )
		ID = ntohl(Kademlia::CKademlia::getIPAddress());
	else if( theApp.serverconnect->IsConnected() )
		ID = theApp.serverconnect->GetClientID();
	else if ( Kademlia::CKademlia::isConnected() && Kademlia::CKademlia::isFirewalled() )
		ID = 1;
	else
		ID = 0;
	return ID;
}

uint32 CemuleApp::GetPublicIP() const {
	if (m_dwPublicIP == 0 && Kademlia::CKademlia::isConnected() && Kademlia::CKademlia::getIPAddress() )
		return ntohl(Kademlia::CKademlia::getIPAddress());
	return m_dwPublicIP;
}

void CemuleApp::SetPublicIP(const uint32 dwIP){
	if (dwIP != 0){
		ASSERT ( !IsLowID(dwIP));
		ASSERT ( m_pPeerCache );
		if ( GetPublicIP() == 0)
			AddDebugLogLine(DLP_VERYLOW, false, _T("My public IP Address is: %s"),ipstr(dwIP));
		else if (Kademlia::CKademlia::isConnected() && Kademlia::CKademlia::getPrefs()->getIPAddress())
			if(ntohl(Kademlia::CKademlia::getIPAddress()) != dwIP)
				AddDebugLogLine(DLP_DEFAULT, false,  _T("Public IP Address reported from Kademlia (%s) differs from new found (%s)"),ipstr(ntohl(Kademlia::CKademlia::getIPAddress())),ipstr(dwIP));
		m_pPeerCache->FoundMyPublicIPAddress(dwIP);	
	}
	else
		AddDebugLogLine(DLP_VERYLOW, false, _T("Deleted public IP"));
	
	m_dwPublicIP = dwIP;

// WebCache ////////////////////////////////////////////////////////////////////////////////////
	// jp detect Webcache on Startup START
	if (thePrefs.WCAutoupdate
		&& m_dwPublicIP != 0 )
	{
		thePrefs.detectWebcacheOnStart = false;
		AutodetectWebcache();
	}
	// jp detect Webcache on Startup END
}


bool CemuleApp::IsFirewalled()
{
	if (theApp.serverconnect->IsConnected() && !theApp.serverconnect->IsLowID())
		return false; // we have an eD2K HighID -> not firewalled

	if (Kademlia::CKademlia::isConnected() && !Kademlia::CKademlia::isFirewalled())
		return false; // we have an Kad HighID -> not firewalled

	return true; // firewalled
}

bool CemuleApp::DoCallback( CUpDownClient *client )
{
	if(Kademlia::CKademlia::isConnected())
	{
		if(theApp.serverconnect->IsConnected())
		{
			if(theApp.serverconnect->IsLowID())
			{
				if(Kademlia::CKademlia::isFirewalled())
				{
					//Both Connected - Both Firewalled
					return false;
				}
				else
				{
					if(client->GetServerIP() == theApp.serverconnect->GetCurrentServer()->GetIP() && client->GetServerPort() == theApp.serverconnect->GetCurrentServer()->GetPort())
					{
						//Both Connected - Server lowID, Kad Open - Client on same server
						//We prevent a callback to the server as this breaks the protocol and will get you banned.
						return false;
					}
					else
					{
						//Both Connected - Server lowID, Kad Open - Client on remote server
						return true;
					}
				}
			}
			else
			{
				//Both Connected - Server HighID, Kad don't care
				return true;
			}
		}
		else
		{
			if(Kademlia::CKademlia::isFirewalled())
			{
				//Only Kad Connected - Kad Firewalled
				return false;
			}
			else
			{
				//Only Kad Conected - Kad Open
				return true;
			}
		}
	}
	else
	{
		if( theApp.serverconnect->IsConnected() )
		{
			if( theApp.serverconnect->IsLowID() )
			{
				//Only Server Connected - Server LowID
				return false;
			}
			else
			{
				//Only Server Connected - Server HighID
				return true;
			}
		}
		else
		{
			//We are not connected at all!
			return false;
		}
	}
}

HICON CemuleApp::LoadIcon(UINT nIDResource) const
{
	// use string resource identifiers!!
	return CWinApp::LoadIcon(nIDResource);
}

HICON CemuleApp::LoadIcon(LPCTSTR lpszResourceName, int cx, int cy, UINT uFlags) const
{
	// Test using of 16 color icons. If 'LR_VGACOLOR' is specified _and_ the icon resource
	// contains a 16 color version, that 16 color version will be loaded. If there is no
	// 16 color version available, Windows will use the next (better) color version found.
#ifdef _DEBUG
	if (g_bLowColorDesktop)
		uFlags |= LR_VGACOLOR;
#endif

	HICON hIcon = NULL;
	LPCTSTR pszSkinProfile = thePrefs.GetSkinProfile();
	if (pszSkinProfile != NULL && pszSkinProfile[0] != _T('\0'))
	{
		// load icon resource file specification from skin profile
		TCHAR szSkinResource[MAX_PATH];
		GetPrivateProfileString(_T("Icons"), lpszResourceName, _T(""), szSkinResource, ARRSIZE(szSkinResource), pszSkinProfile);
		if (szSkinResource[0] != _T('\0'))
		{
			// expand any optional available environment strings
			TCHAR szExpSkinRes[MAX_PATH];
			if (ExpandEnvironmentStrings(szSkinResource, szExpSkinRes, ARRSIZE(szExpSkinRes)) != 0)
			{
				_tcsncpy(szSkinResource, szExpSkinRes, ARRSIZE(szSkinResource));
				szSkinResource[ARRSIZE(szSkinResource)-1] = _T('\0');
			}

			// create absolute path to icon resource file
			TCHAR szFullResPath[MAX_PATH];
			if (PathIsRelative(szSkinResource))
			{
				TCHAR szSkinResFolder[MAX_PATH];
				_tcsncpy(szSkinResFolder, pszSkinProfile, ARRSIZE(szSkinResFolder));
				szSkinResFolder[ARRSIZE(szSkinResFolder)-1] = _T('\0');
				PathRemoveFileSpec(szSkinResFolder);
				_tmakepath(szFullResPath, NULL, szSkinResFolder, szSkinResource, NULL);
			}
			else
			{
				_tcsncpy(szFullResPath, szSkinResource, ARRSIZE(szFullResPath));
				szFullResPath[ARRSIZE(szFullResPath)-1] = _T('\0');
			}

			// check for optional icon index or resource identifier within the icon resource file
			bool bExtractIcon = false;
			CString strFullResPath = szFullResPath;
			int iIconIndex = 0;
			int iComma = strFullResPath.ReverseFind(_T(','));
			if (iComma != -1){
				if (_stscanf((LPCTSTR)strFullResPath + iComma + 1, _T("%d"), &iIconIndex) == 1)
					bExtractIcon = true;
				strFullResPath = strFullResPath.Left(iComma);
			}

			if (bExtractIcon)
			{
				if (uFlags != 0 || !(cx == cy && (cx == 16 || cx == 32)))
				{
					static UINT (WINAPI *_pfnPrivateExtractIcons)(LPCTSTR, int, int, int, HICON*, UINT*, UINT, UINT) = (UINT (WINAPI *)(LPCTSTR, int, int, int, HICON*, UINT*, UINT, UINT))GetProcAddress(GetModuleHandle(_T("user32")), _TWINAPI("PrivateExtractIcons"));
					if (_pfnPrivateExtractIcons)
					{
						UINT uIconId;
						(*_pfnPrivateExtractIcons)(strFullResPath, iIconIndex, cx, cy, &hIcon, &uIconId, 1, uFlags);
					}
				}

				if (hIcon == NULL)
				{
					HICON aIconsLarge[1] = {0};
					HICON aIconsSmall[1] = {0};
					int iExtractedIcons = ExtractIconEx(strFullResPath, iIconIndex, aIconsLarge, aIconsSmall, 1);
					if (iExtractedIcons > 0) // 'iExtractedIcons' is 2(!) if we get a large and a small icon
					{
						// alway try to return the icon size which was requested
						if (cx == 16 && aIconsSmall[0] != NULL)
						{
							hIcon = aIconsSmall[0];
							aIconsSmall[0] = NULL;
						}
						else if (cx == 32 && aIconsLarge[0] != NULL)
						{
							hIcon = aIconsLarge[0];
							aIconsLarge[0] = NULL;
						}
						else
						{
							if (aIconsSmall[0] != NULL)
							{
								hIcon = aIconsSmall[0];
								aIconsSmall[0] = NULL;
							}
							else if (aIconsLarge[0] != NULL)
							{
								hIcon = aIconsLarge[0];
								aIconsLarge[0] = NULL;
							}
						}

						for (int i = 0; i < ARRSIZE(aIconsLarge); i++)
						{
							if (aIconsLarge[i] != NULL)
								VERIFY( DestroyIcon(aIconsLarge[i]) );
							if (aIconsSmall[i] != NULL)
								VERIFY( DestroyIcon(aIconsSmall[i]) );
						}
					}
				}
			}
			else
			{
				// WINBUG???: 'ExtractIcon' does not work well on ICO-files when using the color 
				// scheme 'Windows-Standard (extragroß)' -> always try to use 'LoadImage'!
				//
				// If the ICO file contains a 16x16 icon, 'LoadImage' will though return a 32x32 icon,
				// if LR_DEFAULTSIZE is specified! -> always specify the requested size!
				hIcon = (HICON)::LoadImage(NULL, szFullResPath, IMAGE_ICON, cx, cy, uFlags | LR_LOADFROMFILE);
			}
		}
	}

	if (hIcon == NULL)
	{
		if (cx != LR_DEFAULTSIZE || cy != LR_DEFAULTSIZE || uFlags != LR_DEFAULTCOLOR)
			hIcon = (HICON)::LoadImage(AfxGetResourceHandle(), lpszResourceName, IMAGE_ICON, cx, cy, uFlags);
		if (hIcon == NULL)
			hIcon = CWinApp::LoadIcon(lpszResourceName);
	}
	return hIcon;
}

HBITMAP CemuleApp::LoadImage(LPCTSTR lpszResourceName, LPCTSTR pszResourceType) const
{
	LPCTSTR pszSkinProfile = thePrefs.GetSkinProfile();
	if (pszSkinProfile != NULL && pszSkinProfile[0] != _T('\0'))
	{
		// load resource file specification from skin profile
		TCHAR szSkinResource[MAX_PATH];
		GetPrivateProfileString(_T("Bitmaps"), lpszResourceName, _T(""), szSkinResource, ARRSIZE(szSkinResource), pszSkinProfile);
		if (szSkinResource[0] != _T('\0'))
		{
			// expand any optional available environment strings
			TCHAR szExpSkinRes[MAX_PATH];
			if (ExpandEnvironmentStrings(szSkinResource, szExpSkinRes, ARRSIZE(szExpSkinRes)) != 0)
			{
				_tcsncpy(szSkinResource, szExpSkinRes, ARRSIZE(szSkinResource));
				szSkinResource[ARRSIZE(szSkinResource)-1] = _T('\0');
			}

			// create absolute path to resource file
			TCHAR szFullResPath[MAX_PATH];
			if (PathIsRelative(szSkinResource))
			{
				TCHAR szSkinResFolder[MAX_PATH];
				_tcsncpy(szSkinResFolder, pszSkinProfile, ARRSIZE(szSkinResFolder));
				szSkinResFolder[ARRSIZE(szSkinResFolder)-1] = _T('\0');
				PathRemoveFileSpec(szSkinResFolder);
				_tmakepath(szFullResPath, NULL, szSkinResFolder, szSkinResource, NULL);
			}
			else
			{
				_tcsncpy(szFullResPath, szSkinResource, ARRSIZE(szFullResPath));
				szFullResPath[ARRSIZE(szFullResPath)-1] = _T('\0');
			}

			CEnBitmap bmp;
			if (bmp.LoadImage(szFullResPath))
				return (HBITMAP)bmp.Detach();
		}
	}

	CEnBitmap bmp;
	if (bmp.LoadImage(lpszResourceName, pszResourceType))
		return (HBITMAP)bmp.Detach();
	return NULL;
}

CString CemuleApp::GetSkinFileItem(LPCTSTR lpszResourceName, LPCTSTR pszResourceType) const
{
	LPCTSTR pszSkinProfile = thePrefs.GetSkinProfile();
	if (pszSkinProfile != NULL && pszSkinProfile[0] != _T('\0'))
	{
		// load resource file specification from skin profile
		TCHAR szSkinResource[MAX_PATH];
		GetPrivateProfileString(pszResourceType, lpszResourceName, _T(""), szSkinResource, ARRSIZE(szSkinResource), pszSkinProfile);
		if (szSkinResource[0] != _T('\0'))
		{
			// expand any optional available environment strings
			TCHAR szExpSkinRes[MAX_PATH];
			if (ExpandEnvironmentStrings(szSkinResource, szExpSkinRes, ARRSIZE(szExpSkinRes)) != 0)
			{
				_tcsncpy(szSkinResource, szExpSkinRes, ARRSIZE(szSkinResource));
				szSkinResource[ARRSIZE(szSkinResource)-1] = _T('\0');
			}

			// create absolute path to resource file
			TCHAR szFullResPath[MAX_PATH];
			if (PathIsRelative(szSkinResource))
			{
				TCHAR szSkinResFolder[MAX_PATH];
				_tcsncpy(szSkinResFolder, pszSkinProfile, ARRSIZE(szSkinResFolder));
				szSkinResFolder[ARRSIZE(szSkinResFolder)-1] = _T('\0');
				PathRemoveFileSpec(szSkinResFolder);
				_tmakepath(szFullResPath, NULL, szSkinResFolder, szSkinResource, NULL);
			}
			else
			{
				_tcsncpy(szFullResPath, szSkinResource, ARRSIZE(szFullResPath));
				szFullResPath[ARRSIZE(szFullResPath)-1] = _T('\0');
			}

			return szFullResPath;
		}
	}
	return _T("");
}

bool CemuleApp::LoadSkinColor(LPCTSTR pszKey, COLORREF& crColor) const
{
	LPCTSTR pszSkinProfile = thePrefs.GetSkinProfile();
	if (pszSkinProfile != NULL && pszSkinProfile[0] != _T('\0'))
	{
		TCHAR szColor[MAX_PATH];
		GetPrivateProfileString(_T("Colors"), pszKey, _T(""), szColor, ARRSIZE(szColor), pszSkinProfile);
		if (szColor[0] != _T('\0'))
		{
			UINT red, grn, blu;
			int iVals = _stscanf(szColor, _T("%i , %i , %i"), &red, &grn, &blu);
			if (iVals == 3)
			{
				crColor = RGB(red, grn, blu);
				return true;
			}
		}
	}
	return false;
}

bool CemuleApp::LoadSkinColorAlt(LPCTSTR pszKey, LPCTSTR pszAlternateKey, COLORREF& crColor) const
{
	if (LoadSkinColor(pszKey, crColor))
		return true;
	return LoadSkinColor(pszAlternateKey, crColor);
}

void CemuleApp::ApplySkin(LPCTSTR pszSkinProfile)
{
	thePrefs.SetSkinProfile(pszSkinProfile);
	AfxGetMainWnd()->SendMessage(WM_SYSCOLORCHANGE);
}

CTempIconLoader::CTempIconLoader(LPCTSTR pszResourceID, int cx, int cy, UINT uFlags)
{
	m_hIcon = theApp.LoadIcon(pszResourceID, cx, cy, uFlags);
}

CTempIconLoader::CTempIconLoader(UINT uResourceID, int cx, int cy, UINT uFlags)
{
	ASSERT( uFlags == 0 );
	m_hIcon = theApp.LoadIcon(uResourceID);
}

CTempIconLoader::~CTempIconLoader()
{
	if (m_hIcon)
		VERIFY( DestroyIcon(m_hIcon) );
}

//MORPH START - Changed by SiRoB, Selection category support khaos::categorymod+
/*
void CemuleApp::AddEd2kLinksToDownload(CString strlink, uint8 cat)
*/
void CemuleApp::AddEd2kLinksToDownload(CString strlink, int cat)
//MORPH END   - Changed by SiRoB, Selection category support khaos::categorymod+
{
	int curPos = 0;
	CString resToken = strlink.Tokenize(_T("\t\n\r"), curPos);
	while (resToken != _T(""))
	{
		if (resToken.Right(1) != _T("/"))
			resToken += _T("/");
		try
		{
			CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(resToken.Trim());
			if (pLink)
			{
				if (pLink->GetKind() == CED2KLink::kFile)
				{
					//MORPH START - Changed by SiRoB, Selection category support khaos::categorymod+
					// pFileLink IS NOT A LEAK, DO NOT DELETE.
					CED2KFileLink* pFileLink = (CED2KFileLink*)CED2KLink::CreateLinkFromUrl(resToken.Trim());
					downloadqueue->AddFileLinkToDownload(pFileLink, cat, true);
					//MORPH END   - Changed by SiRoB, Selection category support khaos::categorymod-
				}
				else
				{
					delete pLink;
					throw CString(_T("bad link"));
				}
				delete pLink;
			}
		}
		catch(CString error)
		{
			TCHAR szBuffer[200];
			_sntprintf(szBuffer, ARRSIZE(szBuffer), GetResString(IDS_ERR_INVALIDLINK), error);
			LogError(LOG_STATUSBAR, GetResString(IDS_ERR_LINKERROR), szBuffer);
		}
		resToken = strlink.Tokenize(_T("\t\n\r"), curPos);
	}
}

void CemuleApp::SearchClipboard()
{
	if (m_bGuardClipboardPrompt)
		return;

	CString strLinks = CopyTextFromClipboard();
	if (strLinks.IsEmpty())
		return;

	if (strLinks.Compare(m_strLastClipboardContents) == 0)
		return;

	if (strLinks.Left(13).CompareNoCase(_T("ed2k://|file|")) == 0)
	{
		m_bGuardClipboardPrompt = true;
		if (AfxMessageBox(GetResString(IDS_ED2KLINKFIX) + _T("\r\n\r\n") + GetResString(IDS_ADDDOWNLOADSFROMCB)+_T("\r\n") + strLinks, MB_YESNO | MB_TOPMOST) == IDYES)
			//MORPH START - Changed by SiRoB, Selection category support khaos::categorymod+
			/*
			AddEd2kLinksToDownload(strLinks, 0);
			*/
			AddEd2kLinksToDownload(strLinks, -1);
			/**/
			//MORPH END  - Changed by SiRoB, Selection category support khaos::categorymod+
	}
	m_strLastClipboardContents = strLinks;
	m_bGuardClipboardPrompt = false;
}

//MORPH START - Changed by SiRoB, Selection category support khaos::categorymod+
/*
void CemuleApp::PasteClipboard(uint8 uCategory)
*/
void CemuleApp::PasteClipboard(int Cat)
//MORPH END   - Changed by SiRoB, Selection category support khaos::categorymod+
{
	CString strLinks = CopyTextFromClipboard();
	strLinks.Trim();
	if (strLinks.IsEmpty())
		return;

	//MORPH START - Changed by SiRoB, Selection category support khaos::categorymod+
	/*
	AddEd2kLinksToDownload(strLinks, uCategory);
	*/
	AddEd2kLinksToDownload(strLinks, Cat);
	/**/
	//MORPH END   - Changed by SiRoB, Selection category support khaos::categorymod+
}

bool CemuleApp::IsEd2kLinkInClipboard(LPCSTR pszLinkType, int iLinkTypeLen)
{
	bool bFoundLink = false;
	if (IsClipboardFormatAvailable(CF_TEXT))
	{
		if (OpenClipboard(NULL))
		{
			HGLOBAL	hText = GetClipboardData(CF_TEXT);
			if (hText != NULL)
			{ 
				// Use the ANSI string
				LPCSTR pszText = (LPCSTR)GlobalLock(hText);
				if (pszText != NULL)
				{
					while (*pszText == ' ' || *pszText == '\t' || *pszText == '\r' || *pszText == '\n')
						pszText++;
					bFoundLink = (strncmp(pszText, pszLinkType, iLinkTypeLen) == 0);
					GlobalUnlock(hText);
				}
			}
			CloseClipboard();
		}
	}

	return bFoundLink;
}

bool CemuleApp::IsEd2kFileLinkInClipboard()
{
	static const CHAR _szEd2kFileLink[] = "ed2k://|file|"; // Use the ANSI string
	return IsEd2kLinkInClipboard(_szEd2kFileLink, ARRSIZE(_szEd2kFileLink)-1);
}

bool CemuleApp::IsEd2kServerLinkInClipboard()
{
	static const CHAR _szEd2kServerLink[] = "ed2k://|server|"; // Use the ANSI string
	return IsEd2kLinkInClipboard(_szEd2kServerLink, ARRSIZE(_szEd2kServerLink)-1);
}

// Elandal:ThreadSafeLogging -->
void CemuleApp::QueueDebugLogLine(bool bAddToStatusbar, LPCTSTR line, ...)
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
	newItem->uFlags = LOG_DEBUG | (bAddToStatusbar ? LOG_STATUSBAR : 0);
	newItem->line = bufferline;
	m_QueueDebugLog.AddTail(newItem);

	m_queueLock.Unlock();
}

void CemuleApp::QueueLogLine(bool bAddToStatusbar, LPCTSTR line, ...)
{
	m_queueLock.Lock();

	TCHAR bufferline[1000];

	va_list argptr;
	va_start(argptr, line);
	_vsntprintf(bufferline, ARRSIZE(bufferline), line, argptr);
	va_end(argptr);

	SLogItem* newItem = new SLogItem;
	newItem->uFlags = bAddToStatusbar ? LOG_STATUSBAR : 0;
	newItem->line = bufferline;
	m_QueueLog.AddTail(newItem);

	m_queueLock.Unlock();
}

void CemuleApp::QueueDebugLogLineEx(UINT uFlags, LPCTSTR line, ...)
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
	newItem->uFlags = uFlags | LOG_DEBUG;
	newItem->line = bufferline;
	m_QueueDebugLog.AddTail(newItem);

	m_queueLock.Unlock();
}

void CemuleApp::QueueLogLineEx(UINT uFlags, LPCTSTR line, ...)
{
	m_queueLock.Lock();

	TCHAR bufferline[1000];

	va_list argptr;
	va_start(argptr, line);
	_vsntprintf(bufferline, ARRSIZE(bufferline), line, argptr);
	va_end(argptr);

	SLogItem* newItem = new SLogItem;
	newItem->uFlags = uFlags;
	newItem->line = bufferline;
	m_QueueLog.AddTail(newItem);

	m_queueLock.Unlock();
}

void CemuleApp::HandleDebugLogQueue()
{
	m_queueLock.Lock();
	while (!m_QueueDebugLog.IsEmpty())
	{
		const SLogItem* newItem = m_QueueDebugLog.RemoveHead();
		if (thePrefs.GetVerbose())
			Log(newItem->uFlags, _T("%s"), newItem->line);
		delete newItem;
	}
	m_queueLock.Unlock();
}

void CemuleApp::HandleLogQueue()
{
	m_queueLock.Lock();
	while (!m_QueueLog.IsEmpty())
	{
		const SLogItem* newItem = m_QueueLog.RemoveHead();
		Log(newItem->uFlags, _T("%s"), newItem->line);
		delete newItem;
	}
	m_queueLock.Unlock();
}

void CemuleApp::ClearDebugLogQueue(bool bDebugPendingMsgs)
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

void CemuleApp::ClearLogQueue(bool bDebugPendingMsgs)
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

void CemuleApp::CreateAllFonts()
{
	///////////////////////////////////////////////////////////////////////////
	// Symbol font
	//
	VERIFY( m_fontSymbol.CreatePointFont(10 * 10, _T("Marlett")) );


	///////////////////////////////////////////////////////////////////////////
	// Log-, Message- and IRC-Window fonts
	//
	LPLOGFONT plfHyperText = thePrefs.GetHyperTextLogFont();
	if (plfHyperText==NULL || plfHyperText->lfFaceName[0]==_T('\0') || !m_fontHyperText.CreateFontIndirect(plfHyperText))
		m_fontHyperText.CreatePointFont(10 * 10, _T("MS Shell Dlg"));

	LPLOGFONT plfLog = thePrefs.GetLogFont();
	if (plfLog!=NULL && plfLog->lfFaceName[0]!=_T('\0'))
		m_fontLog.CreateFontIndirect(plfLog);

	// Why can't this font set via the font dialog??
//	HFONT hFontMono = CreateFont(10, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Lucida Console"));
//	m_fontLog.Attach(hFontMono);


	///////////////////////////////////////////////////////////////////////////
	// Default GUI Font (Bold)
	//
	// OEM_FIXED_FONT		Terminal
	// ANSI_FIXED_FONT		Courier
	// ANSI_VAR_FONT		MS Sans Serif
	// SYSTEM_FONT			System
	// DEVICE_DEFAULT_FONT	System
	// SYSTEM_FIXED_FONT	Fixedsys
	// DEFAULT_GUI_FONT		MS Shell Dlg

	CFont* pFont = CFont::FromHandle((HFONT)GetStockObject(DEFAULT_GUI_FONT));
	if (pFont)
	{
		LOGFONT lf;
		pFont->GetLogFont(&lf);
		lf.lfWeight = FW_BOLD;
		VERIFY( m_fontDefaultBold.CreateFontIndirect(&lf) );
	}
}

void CemuleApp::CreateBackwardDiagonalBrush()
{
	static const WORD awBackwardDiagonalBrushPattern[8] = { 0x0f, 0x1e, 0x3c, 0x78, 0xf0, 0xe1, 0xc3, 0x87 };
	CBitmap bm;
	if (bm.CreateBitmap(8, 8, 1, 1, awBackwardDiagonalBrushPattern))
	{
		LOGBRUSH logBrush = {0};
		logBrush.lbStyle = BS_PATTERN;
		logBrush.lbHatch = (int)bm.GetSafeHandle();
		logBrush.lbColor = RGB(0, 0, 0);
		VERIFY( m_brushBackwardDiagonal.CreateBrushIndirect(&logBrush) );
	}
}

void CemuleApp::UpdateDesktopColorDepth()
{
	g_bLowColorDesktop = (GetDesktopColorDepth() <= 8);
#ifdef _DEBUG
	if (!g_bLowColorDesktop)
		g_bLowColorDesktop = (GetProfileInt(_T("eMule"), _T("LowColorRes"), 0) != 0);
#endif
}

// Commander - Added: Custom incoming / temp folder icon [emulEspaña] - Start
void CemuleApp::AddIncomingFolderIcon(){
	CString desktopFile, exePath;
	
	desktopFile = CString(thePrefs.GetIncomingDir()) + _T("\\Desktop.ini");
	exePath = thePrefs.GetAppDir() + CString(theApp.m_pszExeName) + _T(".exe");

	CIni desktopIni(desktopFile, _T(".ShellClassInfo"));
	
	desktopIni.WriteString(_T("IconFile"),exePath);
	desktopIni.WriteInt(_T("IconIndex"),1);

	SetFileAttributes(desktopFile, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
	PathMakeSystemFolder(thePrefs.GetIncomingDir());
}

void CemuleApp::RemoveIncomingFolderIcon()
{
	if(!IsCustomIncomingFolderIcon()){
		CString desktopFile;
	
		desktopFile = CString(thePrefs.GetIncomingDir()) + _T("\\Desktop.ini");

		CIni desktopIni(desktopFile, _T(".ShellClassInfo"));
	
		desktopIni.DeleteKey(_T("IconFile"));
		desktopIni.DeleteKey(_T("IconIndex"));
	}
}
// Commander - Added: Custom incoming / temp folder icon [emulEspaña] - End

// Commander - Added: Custom incoming / temp folder icon [emulEspaña] - Start
void CemuleApp::AddTempFolderIcon(){
	CString desktopFile, exePath;

	desktopFile = CString(thePrefs.GetTempDir()) + _T("\\Desktop.ini");
	exePath = thePrefs.GetAppDir() + CString(theApp.m_pszExeName) + _T(".exe");

	CIni desktopIni(desktopFile, _T(".ShellClassInfo"));

	desktopIni.WriteString(_T("IconFile"),exePath);
	desktopIni.WriteInt(_T("IconIndex"),1);

	SetFileAttributes(desktopFile, FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
	PathMakeSystemFolder(thePrefs.GetTempDir());
}

void CemuleApp::RemoveTempFolderIcon(){
	if(!IsCustomIncomingFolderIcon()){
		CString desktopFile;
		desktopFile = CString(thePrefs.GetTempDir()) + _T("\\Desktop.ini");

		CIni desktopIni(desktopFile, _T(".ShellClassInfo"));

		desktopIni.DeleteKey(_T("IconFile"));
		desktopIni.DeleteKey(_T("IconIndex"));
	}
}
// Commander - Added: Custom incoming / temp folder icon [emulEspaña] - End
BOOL CemuleApp::IsCustomIncomingFolderIcon(){
	CString desktopFile;
	
	desktopFile = CString(thePrefs.GetIncomingDir()) + _T("\\Desktop.ini");

	if(CFileFind().FindFile(desktopFile) == TRUE){
		CIni desktopIni(desktopFile, _T(".ShellClassInfo"));
		
		CString iconFile, exePath;
		int iconIndex;
		iconFile = desktopIni.GetString(_T("IconFile"), _T(""), _T(".ShellClassInfo"));
		iconIndex = desktopIni.GetInt(_T("IconIndex"),0,_T(".ShellClassInfo"));
		exePath = thePrefs.GetAppDir() + CString(theApp.m_pszExeName) + _T(".exe");
		
		iconFile.MakeLower();
		exePath.MakeLower();
		if (iconFile == exePath){
			if(iconIndex == 1)
				return false;
			else
				return true;
		}else{
			if(iconFile == _T(""))
				return false;
			else
				return true;
		}
	}
	else
		return false;
}
// End MoNKi
// Commander - Added: FriendLinks [emulEspaña] - Start

bool CemuleApp::IsEd2kFriendLinkInClipboard()
{
	static const CHAR _szEd2kFriendLink[] = "ed2k://|friend|";
	return IsEd2kLinkInClipboard(_szEd2kFriendLink, ARRSIZE(_szEd2kFriendLink)-1);
}
// Commander - Added: FriendLinks [emulEspaña] - End
