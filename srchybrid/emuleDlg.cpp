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


// emuleDlg.cpp : Implementierungsdatei
//

#include "stdafx.h"
#include "afxinet.h"
#include "emule.h"
#include "emuleDlg.h"
#include "SearchDlg.h"
#include "sockets.h"
#include "knownfilelist.h"
#include "knownfile.h"
#include "serverlist.h"
#include "opcodes.h"
#include "sharedfilelist.h"
#include "creditsdlg.h"
#include "ED2KLink.h"
#include "math.h"
#include "Splashscreen.h"
#include "PartFileConvert.h"

#pragma comment(lib, "winmm.lib")
#include "Mmsystem.h"
#include "enbitmap.h"
#include "wizard.h"
#include "InputBox.h"
#include "Exceptions.h"
#include "PreferencesDlg.h"
#include "SearchList.h"
#include "HTRichEditCtrl.h"
#include "FrameGrabThread.h"
#include "kademlia/kademlia/kademlia.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/routing/RoutingZone.h"
#include "kademlia/routing/contact.h"
#include "kademlia/kademlia/prefs.h"
#include "KademliaWnd.h"
#include "KadSearchListCtrl.h"
#include "KadContactListCtrl.h"
#include "LastCommonRouteFinder.h" //MORPH - Added by SiRoB, ZZ Upload system (USS)

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


BOOL (WINAPI *_TransparentBlt)(HDC, int, int, int, int, HDC, int, int, int, int, UINT)= NULL;
const static UINT UWM_ARE_YOU_EMULE=RegisterWindowMessage(_T(EMULE_GUID));
// CemuleDlg Dialog

CemuleDlg::CemuleDlg(CWnd* pParent /*=NULL*/)
	: CTrayDialog(CemuleDlg::IDD, pParent){
	preferenceswnd = new CPreferencesDlg;
	searchwnd = new CSearchDlg;
	kademliawnd = new CKademliaWnd;
	// NOTE: the application icon name is prefixed with "AAA" to make sure it's alphabetically sorted by the
	// resource compiler as the 1st icon in the resource table!
	m_hIcon = AfxGetApp()->LoadIcon("AAAEMULEAPP");
	theApp.m_app_state	=	APP_STATE_RUNNING;
	ready = false; 
	startUpMinimized=false;
	lastuprate = 0;
	lastdownrate = 0;
	status = 0;
	activewnd = NULL;
	connicons[0] = NULL;
	connicons[1] = NULL;
	connicons[2] = NULL;
	transicons[0] = NULL;
	transicons[1] = NULL;
	transicons[2] = NULL;
	transicons[3] = NULL;
	imicons[0] = NULL;
	imicons[1] = NULL;
	imicons[2] = NULL;
	sourceTrayIcon = NULL;
	sourceTrayIconGrey = NULL;
	sourceTrayIconLow = NULL;
	usericon = NULL;
	mytrayIcon = NULL;
	m_hTimer = 0;
	notifierenabled = false;
	m_pSystrayDlg = NULL;//MORPH - Added by SiRoB, New Systray Popup from Fusion

}

CemuleDlg::~CemuleDlg()
{
	if (mytrayIcon) VERIFY( DestroyIcon(mytrayIcon) );
	if (m_hIcon) VERIFY( ::DestroyIcon(m_hIcon) );
	if (connicons[0]) VERIFY( ::DestroyIcon(connicons[0]) );
	if (connicons[1]) VERIFY( ::DestroyIcon(connicons[1]) );
	if (connicons[2]) VERIFY( ::DestroyIcon(connicons[2]) );
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
	delete searchwnd;
	delete kademliawnd;
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
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_MENUCHAR()
	ON_WM_QUERYENDSESSION()
	ON_MESSAGE(WM_COPYDATA, OnWMData)

	///////////////////////////////////////////////////////////////////////////
	// WM_COMMAND messages
	//
	ON_COMMAND(MP_CONNECT, StartConnection)
	ON_COMMAND(MP_DISCONNECT, CloseConnection)
	ON_COMMAND(MP_EXIT, OnClose )
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

	// Kademlia thread messages
	ON_MESSAGE(WM_KAD_SEARCHADD,	OnKademliaSearchAdd)
	ON_MESSAGE(WM_KAD_SEARCHREM,	OnKademliaSearchRem)
	ON_MESSAGE(WM_KAD_SEARCHREF,	OnKademliaSearchRef)
	ON_MESSAGE(WM_KAD_CONTACTADD,	OnKademliaContactAdd)
	ON_MESSAGE(WM_KAD_CONTACTREM,	OnKademliaContactRem)
	ON_MESSAGE(WM_KAD_CONTACTREF,	OnKademliaContactRef)
	ON_MESSAGE(WM_KAD_RESULTFILE,	OnKademliaResultFile)
	ON_MESSAGE(WM_KAD_RESULTKEYWORD,OnKademliaResultKeyword)
	ON_MESSAGE(WM_KAD_REQUESTTCP,	OnKademliaRequestTCP)
	ON_MESSAGE(WM_KAD_UPDATESTATUS,	OnKademliaUpdateStatus)
	ON_MESSAGE(WM_KAD_OVERHEADSEND,	OnKademliaOverheadSend)
	ON_MESSAGE(WM_KAD_OVERHEADRECV,	OnKademliaOverheadRecv)

	///////////////////////////////////////////////////////////////////////////
	// WM_APP messages
	//
	// SLUGFILLER: SafeHash
	ON_MESSAGE(TM_FINISHEDHASHING,OnFileHashed)
	ON_MESSAGE(TM_HASHFAILED,OnHashFailed)

	// Framegrabbing
	ON_MESSAGE(TM_FRAMEGRABFINISHED,OnFrameGrabFinished)

	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

// CemuleDlg eventhandler

LRESULT CemuleDlg::OnAreYouEmule(WPARAM, LPARAM)
{
  return UWM_ARE_YOU_EMULE;
} 

BOOL CemuleDlg::OnInitDialog()
{
	CTrayDialog::OnInitDialog();
	InitWindowStyles(this);

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL){
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, GetResString(IDS_ABOUTBOX));
			pSysMenu->AppendMenu(MF_STRING, MP_VERSIONCHECK, GetResString(IDS_VERSIONCHECK));

			switch (theApp.glob_prefs->GetWindowsVersion()){
				case _WINVER_98_:
				case _WINVER_95_:	
				case _WINVER_ME_:
					break;
				default: {

					CMenu optionsM;
					optionsM.CreateMenu();
					AddSpeedSelectorSys(&optionsM);

					// set connect/disconnect to enabled and disabled
					optionsM.AppendMenu(MF_STRING ,MP_CONNECT, GetResString(IDS_CONNECTTOANYSERVER));
					optionsM.AppendMenu(MF_STRING ,MP_DISCONNECT,   GetResString(IDS_MAIN_BTN_DISCONNECT)); 
					
					pSysMenu->AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)optionsM.m_hMenu, GetResString(IDS_EM_PREFS));
				 }
			}
	}

	SetIcon(m_hIcon, TRUE);			
	SetIcon(m_hIcon, FALSE);

	toolbar.Create(WS_CHILD | WS_VISIBLE , CRect(0,0,0,0), this, IDC_TOOLBAR);
	toolbar.Init();


	//set title
	CString buffer = "eMule v"; 
	buffer += theApp.m_strCurVersionLong;

//Morph - added by AndCycle, eMulePlus CPU optimize
	CString optimizeType = " ";
#if !defined(MMX) && !defined(AMD) && !defined(SSE)
	switch (get_cpu_type())
	{
		case 1:
			AddLogLine(false,"FPU optimizations active");
			optimizeType += "FPU";
			break;
		case 2:
			AddLogLine(false,"MMX optimizations active");
			optimizeType += "MMX";
			break;
		case 3:
			AddLogLine(false,"AMD optimizations active");
			optimizeType += "AMD";
			break;
		case 4:
		case 5:
			AddLogLine(false,"SSE optimizations active");
			optimizeType += "SSE";
			break;
	}
#else
	#ifdef MMX
		AddLogLine(false,"MMX optimizations active");
		optimizeType += "MMX";
	#endif MMX
	#ifdef AMD
		AddLogLine(false,"AMD optimizations active");
		optimizeType += "AMD";
	#endif AMD
	#ifdef SSE
		AddLogLine(false,"SSE optimizations active");
		optimizeType += "SSE";
	#endif SSE
#endif
	buffer += optimizeType;
//Morph - added by AndCycle, eMulePlus CPU optimize

	SetWindowText(buffer);

	//START - enkeyDEV(kei-kun) -TaskbarNotifier-
	m_wndTaskbarNotifier.Create(this);
	if (_tcscmp(theApp.glob_prefs->GetNotifierConfiguration(),"") == 0) {
			CString defaultTBN;
			defaultTBN.Format("%sNotifier.ini", theApp.glob_prefs->GetAppDir());
			LoadNotifier(defaultTBN);
			theApp.glob_prefs->SetNotifierConfiguration(defaultTBN);
	}
	else
			LoadNotifier(theApp.glob_prefs->GetNotifierConfiguration());
	//END - enkeyDEV(kei-kun) -TaskbarNotifier-

	// set statusbar
	statusbar.Create(WS_CHILD|WS_VISIBLE|CCS_BOTTOM,CRect(0,0,0,0), this, IDC_STATUSBAR) ;
	statusbar.EnableToolTips(true);
	CRect rect;
	statusbar.GetClientRect(&rect);
	//MORPH START - Changed by SiRoB, Related to SUC
	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	//int widths[5] = { rect.right-650, rect.right-440, rect.right-250,rect.right-25, -1 };
	int widths[5] = { rect.right-725, rect.right-515, rect.right-325,rect.right-150/*100*/, -1 };
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
	//MORPH END   - Changed by SiRoB, Related to SUC
	statusbar.SetParts(5, widths);

	LPLOGFONT plfHyperText = theApp.glob_prefs->GetHyperTextLogFont();
	if (plfHyperText==NULL || plfHyperText->lfFaceName[0]=='\0' || !m_fontHyperText.CreateFontIndirect(plfHyperText))
		m_fontHyperText.CreatePointFont(100, _T("Times New Roman"));

	// create dialog pages
	preferenceswnd->SetPrefs(theApp.glob_prefs);
	serverwnd.Create(IDD_SERVER);
	sharedfileswnd.Create(IDD_FILES);
	searchwnd->Create(IDD_SEARCH);
	chatwnd.Create(IDD_CHAT);
	transferwnd.Create(IDD_TRANSFER);
	statisticswnd.Create(IDD_STATISTICS);
	kademliawnd->Create(IDD_KADEMLIAWND);
	ircwnd.Create(IDD_IRC);
	serverwnd.ShowWindow(SW_SHOW);
	activewnd = &serverwnd;

	Localize();

	// set updateintervall of graphic rate display (in seconds)
	//ShowConnectionState(false);

	// anchors
	AddAnchor(serverwnd,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(transferwnd,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(sharedfileswnd,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(*searchwnd,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(chatwnd,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(ircwnd,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(statusbar,BOTTOM_LEFT,BOTTOM_RIGHT);
	AddAnchor(statisticswnd,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(*kademliawnd,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(toolbar,TOP_LEFT, TOP_RIGHT);

	statisticswnd.ShowInterval();
	// tray icon
	TraySetMinimizeToTray(theApp.glob_prefs->GetMinTrayPTR());
	TrayMinimizeToTrayChange();
	ShowTransferRate(true);
	// khaos::categorymod+ obsolete
	//searchwnd.UpdateCatTabs();
	
	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	ShowPing();
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
	// Restore saved window placement
	WINDOWPLACEMENT wp;wp.length=sizeof(wp);
	wp=theApp.glob_prefs->GetEmuleWindowPlacement();
	SetWindowPlacement(&wp);

	if (theApp.glob_prefs->GetWSIsEnabled())
		theApp.webserver->StartServer();
	theApp.mmserver->Init();


	VERIFY( (m_hTimer = ::SetTimer(NULL, NULL, 300, StartupTimer)) );
	if (!m_hTimer)
		AddDebugLogLine(true,_T("Failed to create 'startup' timer - %s"),GetErrorMessage(GetLastError()));

	// splashscreen
	if (theApp.glob_prefs->UseSplashScreen() && !theApp.glob_prefs->GetStartMinimized()){
		CSplashScreen splash(this);
		splash.DoModal();
	}

	theApp.stat_starttime=GetTickCount();

	if( theApp.glob_prefs->IsFirstStart()){
		::DeleteFile(theApp.glob_prefs->GetConfigDir() + _T("PNRecovery.dat"));	//<<-- enkeyDEV(ColdShine) -PartfileNameRecovery- Avoid clashes with previous file format.
		theApp.downloadqueue->UpdatePNRFile();									//<<-- enkeyDEV(ColdShine) -PartfileNameRecovery- Force full rewrite.
		extern BOOL FirstTimeWizard();
		if (FirstTimeWizard()){
			// start connection wizard
			Wizard conWizard;
			conWizard.SetPrefs(theApp.glob_prefs);
			conWizard.DoModal();
		}
	}

	return TRUE;
}

void CemuleDlg::DoVersioncheck(bool manual) {

	if (!manual && theApp.glob_prefs->GetLastVC()!=0) {
		CTime last(theApp.glob_prefs->GetLastVC());
		time_t tLast=mktime(last.GetLocalTm());
		time_t tNow=mktime(CTime::GetCurrentTime().GetLocalTm());

		if ( (difftime(tNow,tLast) / 86400)<theApp.glob_prefs->GetUpdateDays() )
			return;
	}

	switch (IsNewVersionAvailable()){
		case 1 :
			theApp.glob_prefs->UpdateLastVC();
			SetActiveWindow();
			AddLogLine(true,GetResString(IDS_NEWVERSIONAVL));
			ShowNotifier(GetResString(IDS_NEWVERSIONAVLPOPUP), TBN_NEWVERSION);
			if (!theApp.glob_prefs->GetNotifierPopOnNewVersion())
				if (AfxMessageBox(GetResString(IDS_NEWVERSIONAVL)+GetResString(IDS_VISITVERSIONCHECK),MB_YESNO)==IDYES) {
					CString theUrl;
					theUrl.Format("http://emule-project.net");
					ShellExecute(NULL, NULL, theUrl, NULL, theApp.glob_prefs->GetAppDir(), SW_SHOWDEFAULT);
				}
			break;
		case 0:
			theApp.glob_prefs->UpdateLastVC();
			if (manual) AddLogLine(true,GetResString(IDS_NONEWERVERSION));
			break;
		case -1 :
			AddLogLine(true,GetResString(IDS_NEWVERSIONFAILED));
			break;
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
				theApp.sharedfiles->SetOutputCtrl(&theApp.emuledlg->sharedfileswnd.sharedfilesctrl);
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
					AddLogLine(true, GetResString(IDS_MAIN_SOCKETERROR),theApp.glob_prefs->GetPort());
					bError = true;
				}
				if(!theApp.clientudp->Create()){
				    AddLogLine(true, GetResString(IDS_MAIN_SOCKETERROR),theApp.glob_prefs->GetUDPPort());
					bError = true;
				}
				
				if (!bError) // show the success msg, only if we had no serious error
					AddLogLine(true, GetResString(IDS_MAIN_READY)+" "+GetResString(IDS_TRANSVERSION),theApp.m_strCurVersionLong); //MORPH - Added by milobac, Translation version info
				

				if(theApp.glob_prefs->DoAutoConnect())
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
	CATCH_DFLT_EXCEPTIONS("CemuleDlg::StartupTimer")
}

void CemuleDlg::StopTimer(){
	try	{
		if (m_hTimer){
			::KillTimer(NULL, m_hTimer);
			m_hTimer = 0;
		}
	} catch (...) {}
	if (theApp.glob_prefs->UpdateNotify()) DoVersioncheck(false);
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

		case  IDM_ABOUTBOX : {
			CCreditsDlg dlgAbout;
			dlgAbout.DoModal();
			break;
		}
		case MP_VERSIONCHECK:
			DoVersioncheck(true);
			break;
		case MP_CONNECT : {
			theApp.serverconnect->ConnectToAnyServer();
			break;
		}
		case MP_DISCONNECT : {
			theApp.serverconnect->Disconnect();
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
			theApp.emuledlg->transferwnd.UpdateCatTabTitles();
		}
}

void CemuleDlg::OnPaint() {

	if (!startUpMinimized) {
		startUpMinimized=true;
		if (theApp.glob_prefs->GetStartMinimized()) OnCancel();
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
		if (!theApp.serverconnect->IsConnecting() && !Kademlia::CTimer::getThreadID() ){
			StartConnection();
		}
		else {
			CloseConnection();
	}
  else
    //disconnect if currently connected
    CloseConnection();
}

void CemuleDlg::ResetLog(){
	serverwnd.logbox.Reset();
}

void CemuleDlg::ResetDebugLog(){
	serverwnd.debuglog.Reset();
}

void CemuleDlg::AddLogText(bool addtostatusbar,const CString& txt, bool bDebug) {
	if (addtostatusbar)
        if (statusbar.m_hWnd /*&& ready*/)
		{
			if (theApp.m_app_state != APP_STATE_SHUTINGDOWN){
				statusbar.SetText(txt,0,0);
				statusbar.SetTipText(0,txt);
			}
		}
		else
			AfxMessageBox(txt);
#ifdef _DEBUG
	if (bDebug)
		Debug("%s\n", txt);
#endif
	
	char temp[1060]; //<<--9/21/02
	CTime nowT=CTime::GetCurrentTime();
	_snprintf(temp,sizeof temp,"%s: %s",nowT.Format(theApp.glob_prefs->GetDateTimeFormat4Log()),txt);//<<--9/21/02 
	
	if (bDebug){
	if (theApp.glob_prefs->GetVerbose())
		serverwnd.debuglog.AddEntry(temp);
	
	if (theApp.glob_prefs->Debug2Disk()) {
		CString logfile;

		if(theApp.glob_prefs->DateFileNameLog()){//Morph - added by AndCycle, Date File Name Log
			logfile.Format(_T("%s")_T("%s")_T("%s")_T("%s"), theApp.glob_prefs->GetAppDir(), "eMule_Verbose.", nowT.Format("%Y%m%d"), ".log");
		}
		else{
			/*CString*/ logfile = theApp.glob_prefs->GetAppDir() + CString("eMule_Verbose.log");//original
		}
		FILE* file = fopen(logfile , "a");
		if (file){
			fputs(temp, file);
			fputc('\n', file);
			fclose(file);
			}
		}
	}
	else{
		serverwnd.logbox.AddEntry(temp);
		if (ready)
			ShowNotifier(txt, TBN_LOG, false);
		if (theApp.glob_prefs->Log2Disk()) {
			CString logfile;

			if(theApp.glob_prefs->DateFileNameLog()){//Morph - added by AndCycle, Date File Name Log
				logfile.Format(_T("%s")_T("%s")_T("%s")_T("%s"), theApp.glob_prefs->GetAppDir(), "eMule.", nowT.Format("%Y%m%d"), ".log");
			}
			else{
				/*CString*/ logfile = theApp.glob_prefs->GetAppDir() + CString("eMule.log");//original
			}
			FILE* file = fopen(logfile , "a");
			if (file){
				fputs(temp, file);
				fputc('\n', file);
				fclose(file);
			}
		}
	}
}

//MORPH START - Added by SiRoB, ZZ Upload System ZZUL-20030807-1911
void CemuleDlg::QueueDebugLogLine(bool addtostatusbar,CString line,...) {
   ASSERT( theApp.glob_prefs!=NULL );
   if( theApp.glob_prefs==NULL || (!theApp.glob_prefs->GetVerbose() && !theApp.glob_prefs->Debug2Disk()) )
       return;

   queueLock.Lock();

   char bufferline[1000];

   va_list argptr;
   va_start(argptr, line);
   _vsnprintf(bufferline, 1000, line, argptr);
   va_end(argptr);

   LogItem* newItem = new LogItem;
   newItem->addtostatusbar = addtostatusbar;
   newItem->line = bufferline;
   m_LogQueue.AddTail(newItem);

   queueLock.Unlock();
}

void CemuleDlg::HandleDebugLogQueue() {
   queueLock.Lock();
   while(!m_LogQueue.IsEmpty()) {
       LogItem* newItem = m_LogQueue.RemoveHead();
       AddDebugLogLine(newItem->addtostatusbar, newItem->line);
       delete newItem;
   }
   queueLock.Unlock();
}

void CemuleDlg::ClearDebugLogQueue() {
   queueLock.Lock();
   while(!m_LogQueue.IsEmpty()) {
       delete m_LogQueue.RemoveHead();
   }
   queueLock.Unlock();
}
	//MORPH END   - Added by SiRoB, ZZ Upload System ZZUL-20030807-1911

CString CemuleDlg::GetLastLogEntry(){
	return serverwnd.logbox.GetLastLogEntry();
}

CString CemuleDlg::GetAllLogEntries(){
	return serverwnd.logbox.GetAllLogEntries();
}

CString CemuleDlg::GetLastDebugLogEntry(){
	return serverwnd.debuglog.GetLastLogEntry();
}

CString CemuleDlg::GetAllDebugLogEntries(){
	return serverwnd.debuglog.GetAllLogEntries();
}

void CemuleDlg::AddServerMessageLine(LPCTSTR line){
	serverwnd.servermsgbox->AppendText(line+CString(_T('\n')));
}

void CemuleDlg::ShowConnectionState(){
	
	theApp.emuledlg->serverwnd.UpdateMyInfo();
	theApp.emuledlg->serverwnd.UpdateControlsState();
	theApp.emuledlg->kademliawnd->UpdateControlsState();

	//if( theApp.IsConnected()){ We can use this once we handle firewalled users in Kademlia.
	if( theApp.serverconnect->IsConnected() || theApp.kademlia->isConnected() ){
		if ( !theApp.IsFirewalled()) 
			statusbar.SetIcon(3,connicons[2]);
		else 
			statusbar.SetIcon(3,connicons[1]);
} 
	else{
		statusbar.SetIcon(3,connicons[0]);
	}
	
	CString status;

	if(theApp.serverconnect->IsConnected())
		status = "ED2K:"+GetResString(IDS_CONNECTED);
	else if (theApp.serverconnect->IsConnecting())
		status = "ED2K:"+GetResString(IDS_CONNECTING);
	else
		status = "ED2K:"+GetResString(IDS_NOTCONNECTED);
 
	if(theApp.kademlia->isConnected())
		status += "|KAD:"+GetResString(IDS_CONNECTED);
	else if (Kademlia::CTimer::getThreadID())
		status += "|KAD:"+GetResString(IDS_CONNECTING);
	else
		status += "|KAD:"+GetResString(IDS_NOTCONNECTED);

	statusbar.SetTipText(3,status);
	statusbar.SetText(status,3,0);

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
		toolbar.SetButtonInfo(IDC_TOOLBARBUTTON+0, lptbbi);
		//TOOLTIP: GetResString(IDS_MAIN_BTN_DISCONNECT_TOOLTIP)
	}
	else
	{
		if (theApp.serverconnect->IsConnecting() || Kademlia::CTimer::getThreadID()) 
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
			toolbar.SetButtonInfo(IDC_TOOLBARBUTTON+0, lptbbi);
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
			toolbar.SetButtonInfo(IDC_TOOLBARBUTTON+0, lptbbi);
			//TOOLTIP: GetResString(IDS_CONNECTTOANYSERVER)
			//toolbar.AutoSize();
			ShowUserCount();
		}

	}
}

void CemuleDlg::ShowUserCount(){
	uint32 totaluser, totalfile;
	totaluser = totalfile = 0;
	theApp.serverlist->GetUserFileStatus( totaluser, totalfile );
	CString buffer;
	buffer.Format( "%s: %s(%s) | %s: %s", GetResString(IDS_UUSERS), CastItoIShort(totaluser), CastItoIShort(theApp.kademlia->getStatus()->m_kademliaUsers), GetResString(IDS_FILES), CastItoIShort(totalfile));
	statusbar.SetText(buffer,1,0);
}

void CemuleDlg::ShowMessageState(uint8 iconnr){
	statusbar.SetIcon(4,imicons[iconnr]);
	//statusbar.SetDlgItemText(4,"!");	// there is no control with this ID in the statusbar!? (BC)
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
	
	if( theApp.glob_prefs->ShowOverhead() )
		sprintf(buffer,GetResString(IDS_UPDOWN),(float)lastuprate/1024, (float)lastuprateoverhead/1024, (float)lastdownrate/1024, (float)lastdownrateoverhead/1024);
	else
	sprintf(buffer,GetResString(IDS_UPDOWNSMALL),(float)lastuprate/1024, (float)lastdownrate/1024);
	
	if (TrayIsVisible() || forceAll){
		char buffer2[100];
		// set trayicon-icon
		int DownRateProcent=(int)ceil ( (lastdownrate/10.24)/ theApp.glob_prefs->GetMaxGraphDownloadRate());
		if (DownRateProcent>100) DownRateProcent=100;
			UpdateTrayIcon(DownRateProcent);

		//MORPH START - Added by IceCream, Correct the bug of the download speed shown in the systray
		if (theApp.IsConnected())
			_snprintf(buffer2,sizeof buffer2,"[%s] (%s)\r\n%s","MorphKad",GetResString(IDS_CONNECTED),buffer);
		else 
			_snprintf(buffer2,sizeof buffer2,"[%s] (%s)\r\n%s","MorphKad",GetResString(IDS_DISCONNECTED),buffer);
		//MORPH END   - Added by IceCream, Correct the bug of the download speed shown in the systray

		buffer2[63]=0;
		TraySetToolTip(buffer2);
	}

	if (IsWindowVisible() || forceAll) {
		statusbar.SetText(buffer,2,0);
		if (lastuprate && lastdownrate)
			statusbar.SetIcon(2,transicons[3]);
		else if (lastuprate)
			statusbar.SetIcon(2,transicons[2]);
		else if (lastdownrate)
			statusbar.SetIcon(2,transicons[1]);
		else
			statusbar.SetIcon(2,transicons[0]);
	}
	if (!TrayIsVisible() && theApp.glob_prefs->ShowRatesOnTitle()) {
		_snprintf(buffer,sizeof buffer,"(U:%.1f D:%.1f) eMule v%s",(float)lastuprate/1024, (float)lastdownrate/1024,theApp.m_strCurVersionLong);
		SetWindowText(buffer);
	}
}
//MORPH START - Added by SiRoB, ZZ Upload system (USS)
void CemuleDlg::ShowPing() {
    if(IsWindowVisible()) {
        CurrentPingStruct lastPing = theApp.lastCommonRouteFinder->GetCurrentPing();

        char buffer[50];

        if(theApp.glob_prefs->IsDynUpEnabled()) {
            if(lastPing.latency > 0) {
				if (!theApp.glob_prefs->IsUSSLimit()){
					if(lastPing.lowest > 0) {
						sprintf(buffer,"USS %i ms %i%% %.1f%s/s",lastPing.latency, lastPing.latency*100/lastPing.lowest, (float)theApp.lastCommonRouteFinder->GetUpload()/1024,GetResString(IDS_KBYTES));
					} else {
						sprintf(buffer,"USS %i ms %.1f%s/s",lastPing.latency, (float)theApp.lastCommonRouteFinder->GetUpload()/1024,GetResString(IDS_KBYTES));
					}
				} else
					sprintf(buffer,"USS %i ms %i ms %.1f%s/s",lastPing.latency, theApp.glob_prefs->GetDynUpPingLimit(), (float)theApp.lastCommonRouteFinder->GetUpload()/1024,GetResString(IDS_KBYTES));
			} else {
                sprintf(buffer,"Preparing...",lastPing);
            }
		//MORPH START - Added by SiRoB, Related to SUC
		} else if (theApp.glob_prefs->IsSUCDoesWork()){
			sprintf(buffer,"SUC r:%i vur:%.1f%s/s",theApp.uploadqueue->GetAvgRespondTime(0), (float)theApp.uploadqueue->GetMaxVUR()/1024,GetResString(IDS_KBYTES));
		//MORPH END   - Added by SiRoB, Related to SUC
		} else {
            sprintf(buffer,"");
        }
		statusbar.SetText(buffer,4,0);
    }
}
//MORPH END   - Added by SiRoB, ZZ Upload system (USS)

void CemuleDlg::OnCancel(){
	if (*theApp.glob_prefs->GetMinTrayPTR()) {
			TrayShow();
			ShowWindow(SW_HIDE);
	} else {
			ShowWindow(SW_MINIMIZE);
	}
	ShowTransferRate();
	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	ShowPing();
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
}

void CemuleDlg::SetActiveDialog(CDialog* dlg){

	if(dlg == activewnd)
		return;

	activewnd->ShowWindow(SW_HIDE);
	dlg->ShowWindow(SW_SHOW);
	dlg->SetFocus();
	activewnd = dlg;
	if (dlg==&transferwnd) { 
		if (theApp.glob_prefs->ShowCatTabInfos()) theApp.emuledlg->transferwnd.UpdateCatTabTitles();
		theApp.emuledlg->toolbar.PressMuleButton(IDC_TOOLBARBUTTON+3);
	}
	else if (dlg==&serverwnd) theApp.emuledlg->toolbar.PressMuleButton(IDC_TOOLBARBUTTON+2);
	else if (dlg==&chatwnd) {theApp.emuledlg->toolbar.PressMuleButton(IDC_TOOLBARBUTTON+6);theApp.emuledlg->chatwnd.chatselector.ShowChat();}
	else if (dlg==&ircwnd) theApp.emuledlg->toolbar.PressMuleButton(IDC_TOOLBARBUTTON+7);
	else if (dlg==&sharedfileswnd) theApp.emuledlg->toolbar.PressMuleButton(IDC_TOOLBARBUTTON+5);
	else if (dlg==searchwnd) theApp.emuledlg->toolbar.PressMuleButton(IDC_TOOLBARBUTTON+4);
	else if (dlg==&statisticswnd) {theApp.emuledlg->toolbar.PressMuleButton(IDC_TOOLBARBUTTON+8);	statisticswnd.ShowStatistics();}
	else if	(dlg==kademliawnd) {theApp.emuledlg->toolbar.PressMuleButton(IDC_TOOLBARBUTTON+1);}
}

void CemuleDlg::OnSize(UINT nType,int cx,int cy){
	CTrayDialog::OnSize(nType,cx,cy);
	CRect rect;
	statusbar.GetClientRect(&rect);
	//MORPH START - Changed by SiRoB, Related to SUC
	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	//int widths[5] = { rect.right-650, rect.right-440, rect.right-250,rect.right-25, -1 };
	int widths[5] = { rect.right-750, rect.right-540, rect.right-350,rect.right-150/*100*/, -1 };
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
	//MORPH END   - Changed by SiRoB, Related to SUC
	statusbar.SetParts(5, widths);
	transferwnd.VerifyCatTabSize();
}

//extern void URLDecode(CString& result, const char* buff);

LRESULT CemuleDlg::OnWMData(WPARAM wParam,LPARAM lParam){

	tagCOPYDATASTRUCT* data = (tagCOPYDATASTRUCT*)lParam;
	if (data->dwData == OP_ED2KLINK){
		FlashWindow(true);
		if( theApp.glob_prefs->IsBringToFront() ){
			if (IsIconic())
				ShowWindow(SW_SHOWNORMAL);
			else if (TrayHide()){
				ShowWindow(SW_SHOW);
			}
		else
			SetForegroundWindow();
		}
		try {
			CString link2;
			CString link;
			link2 = (char*)data->lpData;
			link2.Replace("%7c","|");
			link=URLDecode(link2);
			CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(link);
			_ASSERT( pLink !=0 );
			switch (pLink->GetKind()) {
			case CED2KLink::kFile:
				{
					// khaos::categorymod+ Need to allocate memory so that our pointer
					// remains valid until the link is added and removed from queue...
					//MORPH START - HotFix by SiRoB, Khaos 14.6 Tempory Patch
					//CED2KFileLink* pFileLink = new CED2KFileLink(pLink); 
					CED2KFileLink* pFileLink = (CED2KFileLink*)CED2KLink::CreateLinkFromUrl(link.Trim());
					//MORPH END - HotFix by SiRoB, Khaos 14.6 Tempory Patch
					_ASSERT(pFileLink !=0);
					theApp.downloadqueue->AddFileLinkToDownload(pFileLink, true);
					// This memory will be deallocated in CDownloadQueue::AddFileLinkToDownload
					// or CDownloadQueue::PurgeED2KFileLinkQueue.
					// khaos::categorymod-
				}
				break;
			case CED2KLink::kServerList:
				{
					CED2KServerListLink* pListLink = pLink->GetServerListLink(); 
					_ASSERT( pListLink !=0 ); 
					CString strAddress = pListLink->GetAddress(); 
					if(strAddress.GetLength() != 0)
						theApp.emuledlg->serverwnd.UpdateServerMetFromURL(strAddress);
				}
				break;
			case CED2KLink::kServer:
				{
					CString defName;
					CED2KServerLink* pSrvLink = pLink->GetServerLink();
					_ASSERT( pSrvLink !=0 );
					in_addr host;
					host.S_un.S_addr = pSrvLink->GetIP();
					CServer* pSrv = new CServer(pSrvLink->GetPort(),inet_ntoa(host));
					_ASSERT( pSrv !=0 );
					pSrvLink->GetDefaultName(defName);
					pSrv->SetListName(defName.GetBuffer());

					// Barry - Default all new servers to high priority
					pSrv->SetPreference(PR_HIGH);

					if (!theApp.emuledlg->serverwnd.serverlistctrl.AddServer(pSrv,true)) 
						delete pSrv; 
					else
						AddLogLine(true,GetResString(IDS_SERVERADDED), pSrv->GetListName());
				}
				break;
			default:
				break;
			}
			delete pLink;
		} catch(...) {
			OUTPUT_DEBUG_TRACE();
			AddLogLine(true, GetResString(IDS_LINKNOTADDED));
		}
	} else if (data->dwData==OP_CLCOMMAND){
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
			if (down.GetLength()>0) theApp.glob_prefs->SetMaxDownload(atoi(down));
			if (up.GetLength()>0) theApp.glob_prefs->SetMaxUpload(atoi(up));

			return true;
		}

		if (clcommand=="help" || clcommand=="/?") {
			// show usage

			return true;
		}

		if (clcommand=="status") {
			CString pString;

			// todo: output to console
			
			pString.Format("%sstatus.log",theApp.glob_prefs->GetAppDir());
			FILE* file = fopen(pString , "w");
			if (file){
				if (theApp.serverconnect->IsConnected()) pString=GetResString(IDS_CONNECTED); else
					if (theApp.serverconnect->IsConnecting()) pString=GetResString(IDS_CONNECTING); else pString=GetResString(IDS_DISCONNECTED);
				fprintf(file, pString);


				pString.Format("\n"+GetResString(IDS_UPDOWNSMALL),(float)theApp.uploadqueue->GetDatarate()/1024, (float)theApp.downloadqueue->GetDatarate()/1024);
				fprintf(file, pString);

				pString="\n"+transferwnd.downloadlistctrl.getTextList();
				fprintf(file, pString);
				
				fclose(file);
			}
			return true;
		}
		// show "unknown command";
	}
	return true;
}

LRESULT CemuleDlg::OnFileHashed(WPARAM wParam,LPARAM lParam){
	if (theApp.m_app_state	== APP_STATE_SHUTINGDOWN)
		return false;
	CKnownFile* result = (CKnownFile*)lParam;
	if (wParam){
		CPartFile* requester = (CPartFile*)wParam;
		// SLUGFILLER: SafeHash - could have been canceled
		if (theApp.downloadqueue->IsPartFile(requester))
		requester->PartFileHashFinished(result);
		else
			delete result;
		// SLUGFILLER: SafeHash
	}
	else{
		theApp.sharedfiles->filelist->SafeAddKFile(result);
		theApp.sharedfiles->SafeAddKFile(result);	
	}
	return true;
}

// SLUGFILLER: SafeHash
LRESULT CemuleDlg::OnHashFailed(WPARAM wParam,LPARAM lParam){
	theApp.sharedfiles->HashFailed((UnknownFile_Struct*)lParam);
	return 0;
}
// SLUGFILLER: SafeHash

void CemuleDlg::OnClose()
{
	if ( theApp.m_app_state == APP_STATE_RUNNING )
		if (theApp.glob_prefs->IsConfirmExitEnabled() )
			if (IDNO==AfxMessageBox(GetResString(IDS_MAIN_EXIT),MB_YESNO | MB_DEFBUTTON2)) return;

	theApp.m_app_state	= APP_STATE_SHUTINGDOWN;
	theApp.serverconnect->Disconnect();
	theApp.OnlineSig(); // Added By Bouc7 

	WINDOWPLACEMENT wp;wp.length=sizeof(wp);GetWindowPlacement(&wp);
	theApp.glob_prefs->SetWindowLayout(wp);

	theApp.kademlia->DisConnect();

	// saving data & stuff
	theApp.knownfiles->Save();
	transferwnd.downloadlistctrl.SaveSettings(CPreferences::tableDownload);
	transferwnd.uploadlistctrl.SaveSettings(CPreferences::tableUpload);
	transferwnd.queuelistctrl.SaveSettings(CPreferences::tableQueue);
	transferwnd.clientlistctrl.SaveSettings(CPreferences::tableClientList);
	searchwnd->searchlistctrl.SaveSettings(CPreferences::tableSearch);
	sharedfileswnd.sharedfilesctrl.SaveSettings(CPreferences::tableShared);
	serverwnd.serverlistctrl.SaveSettings(CPreferences::tableServer);
	kademliawnd->SaveAllSettings();

	theApp.scheduler->RestoreOriginals();
	theApp.glob_prefs->Save();
	searchwnd->SaveSearchStrings();
	serverwnd.SaveServerMetStrings();

	//EastShare START - Pretender, TBH-AutoBackup
	if (theApp.glob_prefs->GetAutoBackup2())
		theApp.ppgbackup->Backup3();
	if (theApp.glob_prefs->GetAutoBackup())
	{
		theApp.ppgbackup->Backup("*.ini", false);
		theApp.ppgbackup->Backup("*.dat", false);
		theApp.ppgbackup->Backup("*.met", false);
	}
	//EastShare END - Pretender, TBH-AutoBackup


	// Barry - Restore old registry if required
	if (theApp.glob_prefs->AutoTakeED2KLinks())
		RevertReg();

	// explicitly delete all listview items which may hold ptrs to objects which will get deleted
	// by the dtors (some lines below) to avoid potential problems during application shutdown.
	transferwnd.downloadlistctrl.DeleteAllItems();
	chatwnd.chatselector.DeleteAllItems();
	theApp.clientlist->DeleteAll();
	searchwnd->searchlistctrl.DeleteAllItems();
	sharedfileswnd.sharedfilesctrl.DeleteAllItems();
    transferwnd.queuelistctrl.DeleteAllItems();
	transferwnd.clientlistctrl.DeleteAllItems();
	transferwnd.uploadlistctrl.DeleteAllItems();

	CPartFileConvert::CloseGUI();
	CPartFileConvert::RemoveAllJobs();

	theApp.uploadBandwidthThrottler->EndThread(); //MORPH - Added by Yun.SF3, ZZ Upload System
	theApp.lastCommonRouteFinder->EndThread(); //MORPH - Added by SiRoB, ZZ Upload system (USS)

	// NOTE: Do not move those dtors into 'CemuleApp::InitInstance' (althought they should be there). The
	// dtors are indirectly calling functions which access several windows which would not be available 
	// after we have closed the main window -> crash!
	delete theApp.mmserver;
	delete theApp.listensocket;
	delete theApp.clientudp;
	delete theApp.sharedfiles;
	delete theApp.serverconnect;
	delete theApp.serverlist;
	delete theApp.knownfiles;
	delete theApp.searchlist;
	delete theApp.clientcredits;
	delete theApp.downloadqueue;
	delete theApp.uploadqueue;
	delete theApp.clientlist;
	delete theApp.friendlist;
	delete theApp.scheduler;
	delete theApp.ipfilter;
	delete theApp.webserver;
	delete theApp.uploadBandwidthThrottler; //MORPH - Added by Yun.SF3, ZZ Upload System
	delete theApp.lastCommonRouteFinder; //MORPH - Added by SiRoB, ZZ Upload system (USS)
	delete theApp.FakeCheck; //MORPH - Added by SiRoB, More clean :|
	delete theApp.glob_prefs;
	delete theApp.kademlia;

	theApp.m_app_state	=	APP_STATE_DONE;
	CTrayDialog::OnCancel();
}

void CemuleDlg::OnTrayRButtonDown(CPoint pt){
//MORPH START - Added by SiRoB, New Systray Popup from Fusion
/*	if (trayPopup) return;

	trayPopup.CreatePopupMenu(); 
	UINT flagsC;
	UINT flagsD;

	// set connect/disconnect to enabled and disabled
	if (!theApp.serverconnect->IsConnected()) {
		flagsD=MF_STRING || MF_DISABLED; flagsC=MF_STRING;
	} else {
		flagsC=MF_STRING || MF_DISABLED; flagsD=MF_STRING;
	}
	trayPopup.AddMenuTitle((CString)"eMule v"+(CString)CURRENT_VERSION_LONG);
	trayPopup.AppendMenu(	MF_STRING ,MP_RESTORE, GetResString(IDS_MAIN_POPUP_RESTORE) ); 
	trayPopup.SetDefaultItem(MP_RESTORE);
	trayPopup.AppendMenu(MF_SEPARATOR);

	// Show QuickSpeedOthers
	if ((theApp.glob_prefs->GetMaxDownload() == 1) && (theApp.glob_prefs->GetMaxUpload() == 1)) {
		trayPopup.AppendMenu(MF_STRING, MP_QS_UA, GetResString(IDS_PW_UA));
	} else {
		trayPopup.AppendMenu(MF_STRING, MP_QS_PA, GetResString(IDS_PW_PA));
	};
	trayPopup.AppendMenu(MF_SEPARATOR);
	

	//AddSpeedSelector(&trayPopup);
	CTitleMenu trayUploadPopup;
	CTitleMenu trayDownloadPopup;
	CString text;

	// creating UploadPopup Menu
	trayUploadPopup.CreateMenu();
	trayUploadPopup.AddMenuTitle(GetResString(IDS_PW_TIT_UP));
	text.Format("20%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.2),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U20,  text);
	text.Format("40%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.4),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U40,  text);
	text.Format("60%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.6),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U60,  text);
	text.Format("80%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.8),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U80,  text);
	text.Format("100%%\t%i %s", (uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()),GetResString(IDS_KBYTESEC));		trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U100, text);
	trayUploadPopup.AppendMenu(MF_SEPARATOR);
	
	if (GetRecMaxUpload()>0) {
		text.Format(GetResString(IDS_PW_MINREC)+GetResString(IDS_KBYTESEC),GetRecMaxUpload());
		trayUploadPopup.AppendMenu(MF_STRING, MP_QS_UP10,text );
	}
	
	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_UPC, GetResString(IDS_PW_UNLIMITED));

	// creating DownloadPopup Menu
	trayDownloadPopup.CreateMenu();
	trayDownloadPopup.AddMenuTitle(GetResString(IDS_PW_TIT_DOWN));
	text.Format("20%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.2),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D20,  text);
	text.Format("40%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.4),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D40,  text);
	text.Format("60%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.6),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D60,  text);
	text.Format("80%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.8),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D80,  text);
	text.Format("100%%\t%i %s", (uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()),GetResString(IDS_KBYTESEC));		trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D100, text);
	trayDownloadPopup.AppendMenu(MF_SEPARATOR);
	trayDownloadPopup.AppendMenu(MF_STRING, MP_QS_DC, GetResString(IDS_PW_UNLIMITED));

	// Show UploadPopup Menu
	if(theApp.glob_prefs->GetMaxUpload()==UNLIMITED)
		text.Format("%s:\t%s (%i %s)", GetResString(IDS_PW_UPL),GetResString(IDS_PW_UNLIMITED), theApp.glob_prefs->GetMaxGraphUploadRate(),GetResString(IDS_KBYTESEC));
	else
		text.Format("%s:\t%i %s (%i %s)", GetResString(IDS_PW_UPL), theApp.glob_prefs->GetMaxUpload(),GetResString(IDS_KBYTESEC) ,theApp.glob_prefs->GetMaxGraphUploadRate(),GetResString(IDS_KBYTESEC));

	trayPopup.AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)trayUploadPopup.m_hMenu, text);
	

	// Show DownloadPopup Menu
	if(theApp.glob_prefs->GetMaxDownload()==UNLIMITED)
		text.Format("%s:\t%s (%i %s)", GetResString(IDS_PW_DOWNL),GetResString(IDS_PW_UNLIMITED), theApp.glob_prefs->GetMaxGraphDownloadRate(),GetResString(IDS_KBYTESEC));
	else
		text.Format("%s:\t%i %s (%i %s)", GetResString(IDS_PW_DOWNL), theApp.glob_prefs->GetMaxDownload(),GetResString(IDS_KBYTESEC), theApp.glob_prefs->GetMaxGraphDownloadRate(),GetResString(IDS_KBYTESEC));
	trayPopup.AppendMenu(MF_STRING|MF_POPUP, (UINT_PTR)trayDownloadPopup.m_hMenu, text);

	trayPopup.AppendMenu(MF_SEPARATOR);

	trayPopup.AppendMenu(flagsC,MP_CONNECT, GetResString(IDS_CONNECTTOANYSERVER));
	trayPopup.AppendMenu(flagsD ,MP_DISCONNECT,   GetResString(IDS_MAIN_BTN_DISCONNECT)); 

	trayPopup.AppendMenu(MF_STRING,MP_EXIT, GetResString(IDS_EXIT)); 
	SetForegroundWindow();
	trayPopup.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, pt.x, pt.y, this); 
	PostMessage(WM_NULL, 0, 0);
	VERIFY( trayUploadPopup.DestroyMenu() );
	VERIFY( trayDownloadPopup.DestroyMenu() );
	VERIFY( trayPopup.DestroyMenu() );
*/
/////////////////////////////////////////////////////////////
	if(m_pSystrayDlg)
	{
		m_pSystrayDlg->BringWindowToTop();
		return;
	}

	m_pSystrayDlg = new CMuleSystrayDlg(this,pt, 
		theApp.glob_prefs->GetMaxGraphUploadRate(), 
		theApp.glob_prefs->GetMaxGraphDownloadRate(),
		theApp.glob_prefs->GetMaxUpload(),
		theApp.glob_prefs->GetMaxDownload());
	
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
		case IDC_RESTORE:
			RestoreWindow();
			break;
		case IDC_CONNECT:
			StartConnection();
			break;
		case IDC_DISCONNECT:
			CloseConnection();
			break;
		case IDC_TRAYEXIT:
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
	text.Format("20%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.2),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U20,  text);
	text.Format("40%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.4),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U40,  text);
	text.Format("60%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.6),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U60,  text);
	text.Format("80%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.8),GetResString(IDS_KBYTESEC));	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U80,  text);
	text.Format("100%%\t%i %s", (uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()),GetResString(IDS_KBYTESEC));		trayUploadPopup.AppendMenu(MF_STRING, MP_QS_U100, text);
	trayUploadPopup.AppendMenu(MF_SEPARATOR);
	
	if (GetRecMaxUpload()>0) {
		text.Format(GetResString(IDS_PW_MINREC)+GetResString(IDS_KBYTESEC),GetRecMaxUpload());
		trayUploadPopup.AppendMenu(MF_STRING, MP_QS_UP10, text );
	}

//	trayUploadPopup.AppendMenu(MF_STRING, MP_QS_UPC, GetResString(IDS_PW_UNLIMITED));

	// creating DownloadPopup Menu
	trayDownloadPopup.CreateMenu();
	//trayDownloadPopup.AddMenuTitle(GetResString(IDS_PW_TIT_DOWN));
	text.Format("20%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.2),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D20,  text);
	text.Format("40%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.4),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D40,  text);
	text.Format("60%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.6),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D60,  text);
	text.Format("80%%\t%i %s",  (uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.8),GetResString(IDS_KBYTESEC));	trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D80,  text);
	text.Format("100%%\t%i %s", (uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()),GetResString(IDS_KBYTESEC));		trayDownloadPopup.AppendMenu(MF_STRING|MF_POPUP, MP_QS_D100, text);
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
	AddLogLine(true, GetResString(IDS_CONNECTING));
	if( theApp.glob_prefs->GetNetworkED2K() ){
		if ( serverwnd.serverlistctrl.GetSelectedCount()>1 )
		{
			serverwnd.serverlistctrl.PostMessage(WM_COMMAND,MP_CONNECTTO,0L);
		}
		else
		{
			theApp.serverconnect->ConnectToAnyServer();
		}
	}
	if( theApp.glob_prefs->GetNetworkKademlia() ){
		theApp.kademlia->Connect();
	}
	ShowConnectionState();
}

void CemuleDlg::CloseConnection(){
	
	if (theApp.serverconnect->IsConnected()){
		theApp.serverconnect->Disconnect();
	}

	if (theApp.serverconnect->IsConnecting()){
		theApp.serverconnect->StopConnectionTry();
	}
	theApp.kademlia->DisConnect();
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
	COLORREF pColors16[1] = {theApp.glob_prefs->GetStatsColor(11)};

	// start it up
	if (theApp.IsConnected()){
		if (theApp.IsFirewalled())
			trayIcon.Init(sourceTrayIconLow,100,1,1,16,16,theApp.glob_prefs->GetStatsColor(11));
		else 
			trayIcon.Init(sourceTrayIcon,100,1,1,16,16,theApp.glob_prefs->GetStatsColor(11));
	}
	else
		trayIcon.Init(sourceTrayIconGrey,100,1,1,16,16,theApp.glob_prefs->GetStatsColor(11));

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

int CemuleDlg::OnCreate(LPCREATESTRUCT lpCreateStruct){
	CTrayDialog::OnCreate(lpCreateStruct);
	/*if (theApp.glob_prefs->UseSplashScreen()){
		ModifyStyleEx(0,WS_EX_LAYERED);
		SetLayeredWindowAttributes(0, 0, LWA_ALPHA);
	}*/
	
	return 0;

}

//START - enkeyDEV(kei-kun) -TaskbarNotifier-
void CemuleDlg::ShowNotifier(CString Text, int MsgType, bool ForceSoundOFF) {
	if (!notifierenabled) return;
	bool ShowIt = false;

	switch(MsgType) {
		case TBN_CHAT:
            if (theApp.glob_prefs->GetUseChatNotifier()) {
				m_wndTaskbarNotifier.Show(Text, MsgType);
				ShowIt = true;
			}
			break;
		case TBN_DLOAD:
            if (theApp.glob_prefs->GetUseDownloadNotifier()) {
				m_wndTaskbarNotifier.Show(Text, MsgType);
				ShowIt = true;			
			}
			break;
		case TBN_DLOADADDED:
            if (theApp.glob_prefs->GetUseNewDownloadNotifier()) {
				m_wndTaskbarNotifier.Show(Text, MsgType);
				ShowIt = true;			
			}
			break;
		case TBN_LOG:
            if (theApp.glob_prefs->GetUseLogNotifier()) {
				m_wndTaskbarNotifier.Show(Text, MsgType);
				ShowIt = true;			
			}
			break;
		case TBN_IMPORTANTEVENT:
			if (theApp.glob_prefs->GetNotifierPopOnImportantError()) {
				m_wndTaskbarNotifier.Show(Text, MsgType);
				ShowIt = true;			
			}
			break;	// added by InterCeptor (bugfix) 27.11.02
		case TBN_NEWVERSION:
			if (theApp.glob_prefs->GetNotifierPopOnNewVersion()) {
				m_wndTaskbarNotifier.Show(Text, MsgType);
				ShowIt = true;			
			}
			break;
		case TBN_NULL:
            m_wndTaskbarNotifier.Show(Text, MsgType);
			ShowIt = true;			
			break;
	}
	
    if (theApp.glob_prefs->GetUseSoundInNotifier() && !ForceSoundOFF && ShowIt == true)
        PlaySound(theApp.glob_prefs->GetNotifierWavSoundPath(), NULL, SND_FILENAME | SND_NOSTOP | SND_NOWAIT | SND_ASYNC);
}

void CemuleDlg::LoadNotifier(CString configuration) {
	notifierenabled = m_wndTaskbarNotifier.LoadConfiguration(configuration);
}

LRESULT CemuleDlg::OnTaskbarNotifierClicked(WPARAM wParam,LPARAM lParam)
{
	int msgType = TBN_NULL;
	msgType = m_wndTaskbarNotifier.GetMessageType();
	
	switch(msgType)
	{
	case TBN_CHAT:
		RestoreWindow();
		SetActiveDialog(&chatwnd);
		break;
	case TBN_DLOAD:
	case TBN_DLOADADDED:
		RestoreWindow();
		SetActiveDialog(&transferwnd);
	//	{TODO: aprire la cartella incoming?}
		break;
	case TBN_IMPORTANTEVENT:
		RestoreWindow();
		SetActiveDialog(&serverwnd);	
		break;
	case TBN_LOG:
		RestoreWindow();
		SetActiveDialog(&serverwnd);	
		break;
	case TBN_NEWVERSION:{
		CString theUrl;
		theUrl.Format("http://emule-project.net");
		ShellExecute(NULL, NULL, theUrl, NULL, theApp.glob_prefs->GetAppDir(), SW_SHOWDEFAULT);
		break;
		}
	}
    return 0;
}
//END - enkeyDEV(kei-kun) -TaskbarNotifier-

void CemuleDlg::OnSysColorChange()
{
	Localize();
	CTrayDialog::OnSysColorChange();
}

void CemuleDlg::Localize()
{
	if (connicons[0]) VERIFY( ::DestroyIcon(connicons[0]) );
	if (connicons[1]) VERIFY( ::DestroyIcon(connicons[1]) );
	if (connicons[2]) VERIFY( ::DestroyIcon(connicons[2]) );
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

	connicons[0] = theApp.LoadIcon("NotConnected", 16, 16);
	connicons[1] = theApp.LoadIcon("Connected", 16, 16);
	connicons[2] = theApp.LoadIcon("ConnectedHighID", 16, 16);
	transicons[0] = theApp.LoadIcon("UploadDownload", 16, 16);
	transicons[1] = theApp.LoadIcon("UP0DOWN1", 16, 16);
	transicons[2] = theApp.LoadIcon("UP1DOWN0", 16, 16);
	transicons[3] = theApp.LoadIcon("UP1DOWN1", 16, 16);
	imicons[0] = NULL;
	imicons[1] = theApp.LoadIcon("Message", 16, 16);
	imicons[2] = theApp.LoadIcon("MessagePending", 16, 16);
	sourceTrayIcon = theApp.LoadIcon("TrayConnected", 16, 16);
	sourceTrayIconGrey = theApp.LoadIcon("TrayNotConnected", 16, 16);
	sourceTrayIconLow = theApp.LoadIcon("TrayLowID", 16, 16);
	usericon = theApp.LoadIcon("StatsClients", 16, 16);

	statusbar.SetIcon(1,usericon);

	toolbar.Localize();
	ShowConnectionState();
	ShowTransferRate(true);
	ShowUserCount();
}


// quick-speed changer -- 

void CemuleDlg::QuickSpeedOther(UINT nID)
{
	switch (nID) {
		case MP_QS_PA: theApp.glob_prefs->SetMaxUpload((uint16)(1));
			theApp.glob_prefs->SetMaxDownload((uint16)(1));
			break ;
		case MP_QS_UA: 
			theApp.glob_prefs->SetMaxUpload((uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()));
			theApp.glob_prefs->SetMaxDownload((uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()));
			break ;
	}
}


void CemuleDlg::QuickSpeedUpload(UINT nID)
{
	switch (nID) {
		case MP_QS_U10: theApp.glob_prefs->SetMaxUpload((uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.1)); break ;
		case MP_QS_U20: theApp.glob_prefs->SetMaxUpload((uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.2)); break ;
		case MP_QS_U30: theApp.glob_prefs->SetMaxUpload((uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.3)); break ;
		case MP_QS_U40: theApp.glob_prefs->SetMaxUpload((uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.4)); break ;
		case MP_QS_U50: theApp.glob_prefs->SetMaxUpload((uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.5)); break ;
		case MP_QS_U60: theApp.glob_prefs->SetMaxUpload((uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.6)); break ;
		case MP_QS_U70: theApp.glob_prefs->SetMaxUpload((uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.7)); break ;
		case MP_QS_U80: theApp.glob_prefs->SetMaxUpload((uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.8)); break ;
		case MP_QS_U90: theApp.glob_prefs->SetMaxUpload((uint16)(theApp.glob_prefs->GetMaxGraphUploadRate()*0.9)); break ;
		case MP_QS_U100: theApp.glob_prefs->SetMaxUpload(theApp.glob_prefs->GetMaxGraphUploadRate()); break ;
//		case MP_QS_UPC: theApp.glob_prefs->SetMaxUpload(UNLIMITED); break ;
		case MP_QS_UP10: theApp.glob_prefs->SetMaxUpload(GetRecMaxUpload()); break ;
	}
}

void CemuleDlg::QuickSpeedDownload(UINT nID)
{
	switch (nID) {
		case MP_QS_D10: theApp.glob_prefs->SetMaxDownload((uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.1)); break ;
		case MP_QS_D20: theApp.glob_prefs->SetMaxDownload((uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.2)); break ;
		case MP_QS_D30: theApp.glob_prefs->SetMaxDownload((uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.3)); break ;
		case MP_QS_D40: theApp.glob_prefs->SetMaxDownload((uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.4)); break ;
		case MP_QS_D50: theApp.glob_prefs->SetMaxDownload((uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.5)); break ;
		case MP_QS_D60: theApp.glob_prefs->SetMaxDownload((uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.6)); break ;
		case MP_QS_D70: theApp.glob_prefs->SetMaxDownload((uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.7)); break ;
		case MP_QS_D80: theApp.glob_prefs->SetMaxDownload((uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.8)); break ;
		case MP_QS_D90: theApp.glob_prefs->SetMaxDownload((uint16)(theApp.glob_prefs->GetMaxGraphDownloadRate()*0.9)); break ;
		case MP_QS_D100: theApp.glob_prefs->SetMaxDownload(theApp.glob_prefs->GetMaxGraphDownloadRate()); break ;
//		case MP_QS_DC: theApp.glob_prefs->SetMaxDownload(UNLIMITED); break ;
	}
}
// quick-speed changer -- based on xrmb

int CemuleDlg::GetRecMaxUpload() {
	
	if (theApp.glob_prefs->GetMaxGraphUploadRate()<7) return 0;
	if (theApp.glob_prefs->GetMaxGraphUploadRate()<15) return theApp.glob_prefs->GetMaxGraphUploadRate()-3;
	return (theApp.glob_prefs->GetMaxGraphUploadRate()-4);
}

BOOL CemuleDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{	
		case IDC_TOOLBARBUTTON + 0:
			OnBnClickedButton2();
			break;
		case IDC_TOOLBARBUTTON + 1:
			SetActiveDialog(kademliawnd);
			break;
		case IDC_TOOLBARBUTTON + 2:
		case MP_HM_SRVR:
			SetActiveDialog(&serverwnd);
			break;
		case IDC_TOOLBARBUTTON + 3:
		case MP_HM_TRANSFER:
			SetActiveDialog(&transferwnd);
			break;
		case IDC_TOOLBARBUTTON + 4:
		case MP_HM_SEARCH:
			SetActiveDialog(searchwnd);
			break;
		case IDC_TOOLBARBUTTON + 5:
		case MP_HM_FILES:
			SetActiveDialog(&sharedfileswnd);
			break;
		case IDC_TOOLBARBUTTON + 6:
		case MP_HM_MSGS:
			SetActiveDialog(&chatwnd);
			break;
		case IDC_TOOLBARBUTTON + 7:
		case MP_HM_IRC:
			SetActiveDialog(&ircwnd);
			break;
		case IDC_TOOLBARBUTTON + 8:
		case MP_HM_STATS:
			SetActiveDialog(&statisticswnd);
			break;
		case IDC_TOOLBARBUTTON + 9:
		case MP_HM_PREFS:
			toolbar.CheckButton(IDC_TOOLBARBUTTON+9,TRUE);
			preferenceswnd->DoModal();
			toolbar.CheckButton(IDC_TOOLBARBUTTON+9,FALSE);
			break;
		case IDC_TOOLBARBUTTON + 10:
			ShowToolPopup(true);
			break;

		case MP_HM_OPENINC:
			ShellExecute(NULL, "open", theApp.glob_prefs->GetIncomingDir(),NULL, NULL, SW_SHOW); 
			break;
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
			ShellExecute(NULL, NULL, "http://www.emule-project.net", NULL, theApp.glob_prefs->GetAppDir(), SW_SHOWDEFAULT);
			break;
		case MP_HM_LINK2:
			ShellExecute(NULL, NULL, "http://www.emule-project.net/faq/", NULL, theApp.glob_prefs->GetAppDir(), SW_SHOWDEFAULT);
			break;
		case MP_HM_LINK3: {
				CString theUrl;
				theUrl.Format("http://emule-project.net");
				ShellExecute(NULL, NULL, theUrl, NULL, theApp.glob_prefs->GetAppDir(), SW_SHOWDEFAULT);
			break;
		}
		case MP_HM_CONVERTPF:
			CPartFileConvert::ShowGUI();
			break;
		case MP_HM_SCHEDONOFF:
			theApp.glob_prefs->SetSchedulerEnabled(!theApp.glob_prefs->IsSchedulerEnabled());
			theApp.scheduler->Check(true);
			break;
	}
	if (wParam>=MP_WEBURL && wParam<=MP_WEBURL+99) {
		RunURL(NULL, theApp.webservices.GetAt(wParam-MP_WEBURL) );
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
	if (toolbar.MapAccelerator(nChar, &nCmdID)){
		OnCommand(nCmdID, 0);
		return MAKELONG(0,MNC_CLOSE);
	}
	return CTrayDialog::OnMenuChar(nChar, nFlags, pMenu);
}

BOOL CemuleDlg::OnQueryEndSession()
{
	if (!CTrayDialog::OnQueryEndSession())
		return FALSE;

	if ( theApp.m_app_state == APP_STATE_DONE )
	{
		return TRUE;
	}
	else if ( theApp.m_app_state == APP_STATE_RUNNING )
	{
		theApp.m_app_state = APP_STATE_SHUTINGDOWN;
		OnClose();
		return TRUE;
	}
	return FALSE;
}

// Barry - To find out if app is running or shutting/shut down
bool CemuleDlg::IsRunning()
{
	return (theApp.m_app_state == APP_STATE_RUNNING);
}

// modders: dont remove or change the original versioncheck! (additionals are ok)
int CemuleDlg::IsNewVersionAvailable(){
	CString current;
	current.Format("%i",theApp.m_uCurVersionCheck);

	// this first block does the actual work
	CInternetSession session;
	CInternetFile* file = NULL;
	try
	{
		// try to connect to the URL
		file = (CInternetFile*) session.OpenURL("http://www.emule-project.net/currentversionhybrid");
	}
	catch (CInternetException* m_pException)
	{
		// set file to NULL if there's an error
		file = NULL;
		m_pException->Delete();
		return -1;
	}

	if (file)
	{
		CString data;
		int comp=-1;

		if (file->ReadString(data) != NULL)
			comp=data.Compare(current);

		file->Close();
		delete file;
		return comp;
	}
	else
	{
		return -1;
	}
	return -1;
}

void CemuleDlg::OnBnClickedHotmenu()
{
	ShowToolPopup(false);
}

void CemuleDlg::ShowToolPopup(bool toolsonly) {
	int counter;
	POINT point;

	::GetCursorPos(&point);

	CTitleMenu menu;
	menu.CreatePopupMenu();
	if (!toolsonly) menu.AddMenuTitle(GetResString(IDS_HOTMENU));
		else menu.AddMenuTitle(GetResString(IDS_TOOLS));

	CMenu m_Links;
	m_Links.CreateMenu();
	m_Links.AppendMenu(MF_STRING,MP_HM_LINK1, GetResString(IDS_HM_LINKHP));
	m_Links.AppendMenu(MF_STRING,MP_HM_LINK2, GetResString(IDS_HM_LINKFAQ));
	m_Links.AppendMenu(MF_STRING,MP_HM_LINK3, GetResString(IDS_HM_LINKVC));

	CMenu m_scheduler;
	m_scheduler.CreateMenu();
	CString schedonoff= (!theApp.glob_prefs->IsSchedulerEnabled())?GetResString(IDS_HM_SCHED_ON):GetResString(IDS_HM_SCHED_OFF);

	m_scheduler.AppendMenu(MF_STRING,MP_HM_SCHEDONOFF, schedonoff);
	if (theApp.scheduler->GetCount()>0) {
		m_scheduler.AppendMenu(MF_SEPARATOR);
		for (int i=0; i<theApp.scheduler->GetCount();i++)
			m_scheduler.AppendMenu(MF_STRING,MP_SCHACTIONS+i, theApp.scheduler->GetSchedule(i)->title );
	}

	CMenu m_Web;
	m_Web.CreateMenu();
	UpdateURLMenu(m_Web,counter);
	UINT flag2;
	flag2=(counter==0) ? MF_GRAYED:MF_STRING;

	if (!toolsonly) {
		if (theApp.serverconnect->IsConnected()) menu.AppendMenu(MF_STRING,MP_HM_CON,GetResString(IDS_MAIN_BTN_DISCONNECT));
		else 
			if (theApp.serverconnect->IsConnecting()) menu.AppendMenu(MF_STRING,MP_HM_CON,GetResString(IDS_MAIN_BTN_CANCEL));
			else menu.AppendMenu(MF_STRING,MP_HM_CON,GetResString(IDS_MAIN_BTN_CONNECT));

		menu.AppendMenu(MF_STRING,MP_HM_SRVR, GetResString(IDS_EM_SERVER) );
		menu.AppendMenu(MF_STRING,MP_HM_TRANSFER, GetResString(IDS_EM_TRANS));
		menu.AppendMenu(MF_STRING,MP_HM_SEARCH, GetResString(IDS_EM_SEARCH));
		menu.AppendMenu(MF_STRING,MP_HM_FILES, GetResString(IDS_EM_FILES));
		menu.AppendMenu(MF_STRING,MP_HM_MSGS, GetResString(IDS_EM_MESSAGES));
		menu.AppendMenu(MF_STRING,MP_HM_IRC, GetResString(IDS_IRC));
		menu.AppendMenu(MF_STRING,MP_HM_STATS, GetResString(IDS_EM_STATISTIC));
		menu.AppendMenu(MF_STRING,MP_HM_PREFS, GetResString(IDS_EM_PREFS));
		menu.AppendMenu(MF_SEPARATOR);
	}
	menu.AppendMenu(MF_STRING,MP_HM_OPENINC, GetResString(IDS_OPENINC));
	menu.AppendMenu(MF_STRING,MP_HM_CONVERTPF, GetResString(IDS_IMPORTSPLPF));

	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_Links.m_hMenu, GetResString(IDS_LINKS) );
	menu.AppendMenu(flag2|MF_POPUP,(UINT_PTR)m_Web.m_hMenu, GetResString(IDS_WEBSERVICES) );
	menu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_scheduler.m_hMenu, GetResString(IDS_SCHEDULER) );

	if (!toolsonly) {
		menu.AppendMenu(MF_SEPARATOR);
		menu.AppendMenu(MF_STRING,MP_HM_EXIT, GetResString(IDS_EXIT));
	}
	menu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( m_Web.DestroyMenu() );
	VERIFY( m_Links.DestroyMenu() );
	VERIFY( m_scheduler.DestroyMenu() );
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
	theApp.emuledlg->serverwnd.serverlistctrl.RemoveServer((CServer*)wParam); // sivka's bugfix
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
		theApp.glob_prefs->SetHyperTextFont(plf);
		theApp.emuledlg->serverwnd.servermsgbox->SetFont(&m_fontHyperText);
		theApp.emuledlg->chatwnd.chatselector.SetFont(&m_fontHyperText);
		theApp.emuledlg->ircwnd.UpdateFonts(&m_fontHyperText);
	}
}

LRESULT CemuleDlg::OnJigleSearchResponse(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	theApp.emuledlg->searchwnd->ProcessJigleSearchResponse(wParam, lParam);
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Kademlia message handlers

LRESULT CemuleDlg::OnKademliaSearchAdd(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	theApp.emuledlg->kademliawnd->searchList->SearchAdd((Kademlia::CSearch*)lParam);
	return 0;
}

LRESULT CemuleDlg::OnKademliaSearchRem(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	theApp.emuledlg->kademliawnd->searchList->SearchRem((Kademlia::CSearch*)lParam);
	return 0;
}

LRESULT CemuleDlg::OnKademliaSearchRef(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	theApp.emuledlg->kademliawnd->searchList->SearchRef((Kademlia::CSearch*)lParam);
	return 0;
}

LRESULT CemuleDlg::OnKademliaContactAdd(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	theApp.emuledlg->kademliawnd->contactList->ContactAdd((Kademlia::CContact*)lParam);
	return 0;
}

LRESULT CemuleDlg::OnKademliaContactRem(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	theApp.emuledlg->kademliawnd->contactList->ContactRem((Kademlia::CContact*)lParam);
	return 0;
}

LRESULT CemuleDlg::OnKademliaContactRef(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	theApp.emuledlg->kademliawnd->contactList->ContactRef((Kademlia::CContact*)lParam);
	return 0;
}

LRESULT CemuleDlg::OnKademliaResultFile(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	const KADFILERESULT* lpkadfr = (KADFILERESULT*)lParam;
	KademliaSearchFile(lpkadfr->searchID, lpkadfr->pcontactID, lpkadfr->type, lpkadfr->ip, lpkadfr->tcp, lpkadfr->udp, lpkadfr->serverip, lpkadfr->serverport);
	return 0;
}

LRESULT CemuleDlg::OnKademliaResultKeyword(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	const KADKEYWORDRESULT* kadkwr = (KADKEYWORDRESULT*)lParam;
	KademliaSearchKeyword(kadkwr->searchID, kadkwr->pfileID, kadkwr->name, kadkwr->size, kadkwr->type, kadkwr->numProperties, kadkwr->args);
	return 0;
}

LRESULT CemuleDlg::OnKademliaRequestTCP(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	theApp.clientlist->RequestTCP((Kademlia::CContact*)lParam);
	return 0;
}

LRESULT CemuleDlg::OnKademliaUpdateStatus(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	theApp.kademlia->setStatus((Status*)lParam);
	return 0;
}

LRESULT CemuleDlg::OnKademliaOverheadSend(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	theApp.uploadqueue->AddUpDataOverheadServer((uint32)lParam);
	return 0;
}

LRESULT CemuleDlg::OnKademliaOverheadRecv(WPARAM wParam, LPARAM lParam)
{
	if ( !theApp.emuledlg->IsRunning() )
		return 1;
	theApp.downloadqueue->AddDownDataOverheadServer((uint32)lParam);
	return 0;
}

void CemuleDlg::SetKadButtonState() {
	//I'm temp removing this.. Will most likely split the status window from the servers.
	//This have a true Server & Kademlia window.. We can then add and remove the buttons then..
	//Also, I think it's best to hide the button, not disable..
//	toolbar.EnableButton(IDC_TOOLBARBUTTON+1, theApp.glob_prefs->GetNetworkKademlia());
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

		if (   _tcsicmp(szClassName, _T("EDIT")) == 0
			|| (bButton && (pWnd->GetStyle() & BS_GROUPBOX) == 0)
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
	if (theApp.glob_prefs->GetStraightWindowStyles() < 0)
		return;
	else if (theApp.glob_prefs->GetStraightWindowStyles() > 0)
		StraightWindowStyles(pWnd);
	else
		FlatWindowStyles(pWnd);
}
