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
#include "ServerWnd.h"
#include "HttpDownloadDlg.h"
#include "HTRichEditCtrl.h"
#include "ED2KLink.h"
#include "kademlia/kademlia/kademlia.h"
#include "kademlia/kademlia/prefs.h"
#include "kademlia/utils/MiscUtils.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "WebServer.h"
#include "CustomAutoComplete.h"
#include "Server.h"
#include "ServerList.h"
#include "Sockets.h"
#include "MuleStatusBarCtrl.h"
#include "HelpIDs.h"

// Mighty Knife: Popup-Menu for editing news feeds
#include "MenuCmds.h"
#include "InputBox.h"
// [end] Mighty Knife

//MORPH START - Added by SiRoB, XML News [O²]
#define PUGAPI_VARIANT 0x58475550
#define PUGAPI_VERSION_MAJOR 1
#define PUGAPI_VERSION_MINOR 2
#include "pugxml.h"
#include ".\serverwnd.h"
//MORPH END   - Added by SiRoB, XML News [O²]

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define	SERVERMET_STRINGS_PROFILE	_T("AC_ServerMetURLs.dat")
#define SZ_DEBUG_LOG_TITLE			_T("Verbose")

// CServerWnd dialog

IMPLEMENT_DYNAMIC(CServerWnd, CDialog)
CServerWnd::CServerWnd(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CServerWnd::IDD, pParent)
{
	servermsgbox = new CHTRichEditCtrl;
	//MORPH START - Added by SiRoB, XML News [O²]
	newsmsgbox = new CHTRichEditCtrl; // Added by N_OxYdE: XML News
	//MORPH END   - Added by SiRoB, XML News [O²]
	m_pacServerMetURL=NULL;
	m_uLangID = MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT);
	icon_srvlist = NULL;
	memset(&m_cfDef, 0, sizeof m_cfDef);
	memset(&m_cfBold, 0, sizeof m_cfBold);
	StatusSelector.m_bCloseable = false;
}

CServerWnd::~CServerWnd()
{
	if (icon_srvlist)
		VERIFY( DestroyIcon(icon_srvlist) );
	if (m_pacServerMetURL){
		m_pacServerMetURL->Unbind();
		m_pacServerMetURL->Release();
	}
	delete servermsgbox;
	//MORPH START - Added by SiRoB, XML News [O²]
	delete newsmsgbox; // Added by N_OxYdE: XML News
	//MORPH END   - Added by SiRoB, XML News [O²]
}

BOOL CServerWnd::OnInitDialog()
{
#ifdef _UNICODE
	ReplaceRichEditCtrl(GetDlgItem(IDC_MYINFOLIST), this, GetDlgItem(IDC_SSTATIC)->GetFont());
#endif
	CResizableDialog::OnInitDialog();

	logbox.Init(GetResString(IDS_SV_LOG), _T("Log"));
	if (theApp.emuledlg->m_fontLog.m_hObject)
		logbox.SetFont(&theApp.emuledlg->m_fontLog);
	else{
		CFont* pFont = logbox.GetFont();
		if (pFont){
			LOGFONT lf;
			pFont->GetObject(sizeof lf, &lf);
			theApp.emuledlg->m_fontLog.CreateFontIndirect(&lf);
		}
	}
	logbox.ApplySkin();

	debuglog.Init(SZ_DEBUG_LOG_TITLE, _T("VerboseLog"));
	if (theApp.emuledlg->m_fontLog.m_hObject)
		debuglog.SetFont(&theApp.emuledlg->m_fontLog);
	debuglog.ApplySkin();

	SetAllIcons();
	Localize();
	serverlistctrl.Init(theApp.serverlist);

	((CEdit*)GetDlgItem(IDC_SPORT))->SetLimitText(5);
	GetDlgItem(IDC_SPORT)->SetWindowText(_T("4661"));
	CRect rect;

	GetDlgItem(IDC_SERVMSG)->GetWindowRect(rect);
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rect, 2);
	if (servermsgbox->Create(WS_VISIBLE | WS_CHILD | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_READONLY, rect, this, 123)){
		servermsgbox->SetProfileSkinKey(_T("ServerInfoLog"));
		servermsgbox->ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
		servermsgbox->SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
		servermsgbox->SetEventMask(servermsgbox->GetEventMask() | ENM_LINK);
		servermsgbox->SetFont(&theApp.emuledlg->m_fontHyperText);
		servermsgbox->ApplySkin();
		servermsgbox->SetTitle(GetResString(IDS_SV_SERVERINFO));

		servermsgbox->AppendText(CString(CString("eMule v")+theApp.m_strCurVersionLong+CString("\n")));
		// MOD Note: Do not remove this part - Merkur
		m_strClickNewVersion = GetResString(IDS_EMULEW) + _T(" ") + GetResString(IDS_EMULEW3) + _T(" ") + GetResString(IDS_EMULEW2);
		servermsgbox->AppendHyperLink(_T(""),_T(""),m_strClickNewVersion,_T(""),false);
		// MOD Note: end
		servermsgbox->AppendText(CString("\n\n"));
	}
	//MORPH START - Added by SiRoB, XML News [O²]
	if (newsmsgbox->Create(WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_READONLY, rect, this, 124)){
		newsmsgbox->ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
		newsmsgbox->SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
		newsmsgbox->SetEventMask(newsmsgbox->GetEventMask() | ENM_LINK);
		newsmsgbox->SetFont(&theApp.emuledlg->m_fontHyperText);
		newsmsgbox->SetTitle("News");
	}
	//MORPH END   - Added by SiRoB, XML News [O²]
	TCITEM newitem;
	CString name;
	name = GetResString(IDS_SV_SERVERINFO);
	newitem.mask = TCIF_TEXT|TCIF_IMAGE;
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
	newitem.iImage = 1;
	VERIFY( StatusSelector.InsertItem(StatusSelector.GetItemCount(), &newitem) == PaneServerInfo );

	name = GetResString(IDS_SV_LOG);
	newitem.mask = TCIF_TEXT|TCIF_IMAGE;
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
	newitem.iImage = 0;
	VERIFY( StatusSelector.InsertItem(StatusSelector.GetItemCount(), &newitem) == PaneLog );

	//MORPH START - Added by SiRoB, XML News [O²]
	name = "News";
	newitem.mask = TCIF_TEXT|TCIF_IMAGE;
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
	newitem.iImage = 0;
	VERIFY( StatusSelector.InsertItem(StatusSelector.GetItemCount(), &newitem) == PaneNews );
	//MORPH END   - Added by SiRoB, XML News [O²]

	name=SZ_DEBUG_LOG_TITLE;
	newitem.mask = TCIF_TEXT|TCIF_IMAGE;
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
	newitem.iImage = 0;
	VERIFY( StatusSelector.InsertItem(StatusSelector.GetItemCount(), &newitem) == PaneVerboseLog );

	AddAnchor(IDC_SERVLIST,TOP_LEFT, CSize(100,50));
	AddAnchor(IDC_LOGBOX, CSize(0,50), BOTTOM_RIGHT);
	AddAnchor(IDC_DEBUG_LOG, CSize(0,50), BOTTOM_RIGHT);
	AddAnchor(IDC_SSTATIC,TOP_RIGHT);
	AddAnchor(IDC_SSTATIC4,TOP_RIGHT);
	AddAnchor(IDC_SSTATIC7,TOP_RIGHT);
	AddAnchor(IDC_IPADDRESS,TOP_RIGHT);
	AddAnchor(IDC_SSTATIC3,TOP_RIGHT);
	AddAnchor(IDC_SNAME,TOP_RIGHT);
	AddAnchor(IDC_ADDSERVER,TOP_RIGHT );
	AddAnchor(IDC_SSTATIC5,TOP_RIGHT);
	AddAnchor(IDC_MYINFO,TOP_RIGHT ,CSize(100, 100));
	AddAnchor(IDC_MYINFOLIST,TOP_RIGHT,CSize(100,100));
	AddAnchor(IDC_SPORT,TOP_RIGHT);
	AddAnchor(IDC_SSTATIC6,TOP_RIGHT);
	AddAnchor(IDC_SERVERMETURL,TOP_RIGHT);
	AddAnchor(IDC_UPDATESERVERMETFROMURL,TOP_RIGHT);
	AddAnchor(IDC_TAB3,CSize(0,50), BOTTOM_RIGHT);
	AddAnchor(IDC_LOGRESET,CSize(100,50)); // avoid resizing GUI glitches with the tab control by adding this control as the last one (Z-order)
	AddAnchor(IDC_ED2KCONNECT,TOP_RIGHT);
	AddAnchor(IDC_DD,TOP_RIGHT);

	//MORPH START - Added by SiRoB, XML News [O²]
	AddAnchor(IDC_FEEDUPDATE, MIDDLE_RIGHT);
	AddAnchor(IDC_FEEDCHANGE, MIDDLE_RIGHT);
	AddAnchor(IDC_FEEDLIST, MIDDLE_LEFT, MIDDLE_RIGHT);
	//MORPH END   - Added by SiRoB, XML News [O²]
	
	if (servermsgbox->m_hWnd)
		AddAnchor(*servermsgbox, CSize(0,50), BOTTOM_RIGHT);
	debug = true;
	//MORPH START - Added by SiRoB, XML News [O²]
	if (newsmsgbox->m_hWnd)
		AddAnchor(*newsmsgbox, CSize(0,50), BOTTOM_RIGHT);
	news = true;
	//MORPH END   - Added by SiRoB, XML News [O²]	
	ToggleDebugWindow();

	debuglog.ShowWindow(SW_HIDE);
	logbox.ShowWindow(SW_HIDE);
	//MORPH START - Added by SiRoB, XML News [O²]
	newsmsgbox->ShowWindow(SW_HIDE); // Added by N_OxYdE: XML News
	//MORPH END   - Added by SiRoB, XML News [O²]

	// Mighty Knife: Context menu for editing news feeds
	if (m_FeedsMenu) VERIFY (m_FeedsMenu.DestroyMenu ());
	m_FeedsMenu.CreatePopupMenu();
	m_FeedsMenu.AppendMenu(MF_STRING,MP_NEWFEED,GetResString (IDS_FEEDNEW));
	m_FeedsMenu.AppendMenu(MF_STRING,MP_EDITFEED,GetResString (IDS_FEEDEDIT));
	m_FeedsMenu.AppendMenu(MF_STRING,MP_DELETEFEED,GetResString (IDS_FEEDDELETE));
	m_FeedsMenu.AppendMenu(MF_STRING|MF_SEPARATOR);
	m_FeedsMenu.AppendMenu(MF_STRING,MP_DELETEALLFEEDS,GetResString (IDS_FEEDDELETEALL));
	// [end] Mighty Knife

	if (servermsgbox->m_hWnd)
		servermsgbox->ShowWindow(SW_SHOW);

	// optional: restore last used log pane
	if (thePrefs.GetRestoreLastLogPane())
	{
		if (thePrefs.GetLastLogPaneID() >= 0 && thePrefs.GetLastLogPaneID() < StatusSelector.GetItemCount())
		{
			int iCurSel = StatusSelector.GetCurSel();
			StatusSelector.SetCurSel(thePrefs.GetLastLogPaneID());
			if (thePrefs.GetLastLogPaneID() == StatusSelector.GetCurSel())
				UpdateLogTabSelection();
			else
				StatusSelector.SetCurSel(iCurSel);
		}
	}

	m_MyInfo.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
	m_MyInfo.SetAutoURLDetect();
	m_MyInfo.SetEventMask(m_MyInfo.GetEventMask() | ENM_LINK);

	PARAFORMAT pf = {0};
	pf.cbSize = sizeof pf;
	if (m_MyInfo.GetParaFormat(pf)){
		pf.dwMask |= PFM_TABSTOPS;
		pf.cTabCount = 4;
		pf.rgxTabs[0] = 900;
		pf.rgxTabs[1] = 1000;
		pf.rgxTabs[2] = 1100;
		pf.rgxTabs[3] = 1200;
		m_MyInfo.SetParaFormat(pf);
	}

	m_cfDef.cbSize = sizeof m_cfDef;
	if (m_MyInfo.GetSelectionCharFormat(m_cfDef)){
		m_cfBold = m_cfDef;
		m_cfBold.dwMask |= CFM_BOLD;
		m_cfBold.dwEffects |= CFE_BOLD;
	}

	if (thePrefs.GetUseAutocompletion()){
		m_pacServerMetURL = new CCustomAutoComplete();
		m_pacServerMetURL->AddRef();
		if (m_pacServerMetURL->Bind(::GetDlgItem(m_hWnd, IDC_SERVERMETURL), ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST | ACO_FILTERPREFIXES ))
			m_pacServerMetURL->LoadList(CString(thePrefs.GetConfigDir()) +  _T("\\") SERVERMET_STRINGS_PROFILE);
		if (theApp.emuledlg->m_fontMarlett.m_hObject){
			GetDlgItem(IDC_DD)->SetFont(&theApp.emuledlg->m_fontMarlett);
			GetDlgItem(IDC_DD)->SetWindowText(_T("6")); // show a down-arrow
		}
	}
	else
		GetDlgItem(IDC_DD)->ShowWindow(SW_HIDE);

	InitWindowStyles(this);

	//MORPH START - Added by SiRoB, XML News [O²]
	ListFeeds(); // Added by O²: XML News
	//MORPH END   - Added by SiRoB, XML News [O²]
	return true;
}

void CServerWnd::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SERVLIST, serverlistctrl);
	DDX_Control(pDX, IDC_LOGBOX, logbox);
	DDX_Control(pDX, IDC_DEBUG_LOG, debuglog);
	DDX_Control(pDX, IDC_SSTATIC, m_ctrlNewServerFrm);
	DDX_Control(pDX, IDC_SSTATIC6, m_ctrlUpdateServerFrm);
	DDX_Control(pDX, IDC_MYINFO, m_ctrlMyInfo);
	DDX_Control(pDX, IDC_TAB3, StatusSelector);
	DDX_Control(pDX, IDC_MYINFOLIST, m_MyInfo);
	//MORPH START - Added by SiRoB, XML News [O²]
	DDX_Control(pDX, IDC_FEEDLIST, m_feedlist); // Added by O²: XML News
	//MORPH END   - Added by SiRoB, XML News [O²]
}

bool CServerWnd::UpdateServerMetFromURL(CString strURL) {
	if ((strURL==_T("")) || (strURL.Find(_T("://")) == -1))	// not a valid URL
	{
		AddLogLine(true, GetResString(IDS_INVALIDURL) );
		return false;
	}

	CString strTempFilename;
	strTempFilename.Format(_T("%stemp-%d-server.met"), thePrefs.GetConfigDir(), ::GetTickCount());

	// step2 - try to download server.met
	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_sURLToDownload = strURL;
	dlgDownload.m_sFileToDownloadInto = strTempFilename;
	if (dlgDownload.DoModal() != IDOK)
	{
		AddLogLine(true, GetResString(IDS_ERR_FAILEDDOWNLOADMET), strURL);
		return false;
	}

	if (m_pacServerMetURL && m_pacServerMetURL->IsBound())
		m_pacServerMetURL->AddItem(strURL,0);

	// step3 - add content of server.met to serverlist
	serverlistctrl.Hide();
	serverlistctrl.AddServermetToList(strTempFilename);
	serverlistctrl.Visable();
	_tremove(strTempFilename);
	return true;
}

void CServerWnd::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

void CServerWnd::SetAllIcons()
{
	m_ctrlNewServerFrm.Init(_T("AddServer"));
	m_ctrlUpdateServerFrm.Init(_T("ServerUpdateMET"));
	m_ctrlMyInfo.Init(_T("MyInfo"));

	CImageList iml;
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.Add(CTempIconLoader(_T("Log")));
	iml.Add(CTempIconLoader(_T("ServerInfo")));
	StatusSelector.SetImageList(&iml);
	m_imlLogPanes.DeleteImageList();
	m_imlLogPanes.Attach(iml.Detach());

	if (icon_srvlist)
		VERIFY( DestroyIcon(icon_srvlist) );
	icon_srvlist = theApp.LoadIcon(_T("ServerList"), 16, 16);
	((CStatic*)GetDlgItem(IDC_SERVLST_ICO))->SetIcon(icon_srvlist);
}

void CServerWnd::Localize()
{
	serverlistctrl.Localize();

	if (thePrefs.GetLanguageID() != m_uLangID){
		m_uLangID = thePrefs.GetLanguageID();
	    GetDlgItem(IDC_SERVLIST_TEXT)->SetWindowText(GetResString(IDS_SV_SERVERLIST));
	    GetDlgItem(IDC_SSTATIC)->SetWindowText(GetResString(IDS_SV_NEWSERVER));
	    m_ctrlNewServerFrm.SetText(GetResString(IDS_SV_NEWSERVER));
	    GetDlgItem(IDC_SSTATIC4)->SetWindowText(GetResString(IDS_SV_ADDRESS));
	    GetDlgItem(IDC_SSTATIC7)->SetWindowText(GetResString(IDS_SV_PORT));
	    GetDlgItem(IDC_SSTATIC3)->SetWindowText(GetResString(IDS_SW_NAME));
	    GetDlgItem(IDC_ADDSERVER)->SetWindowText(GetResString(IDS_SV_ADD));
	    GetDlgItem(IDC_SSTATIC6)->SetWindowText(GetResString(IDS_SV_MET));
	    m_ctrlUpdateServerFrm.SetText(GetResString(IDS_SV_MET));
	    GetDlgItem(IDC_UPDATESERVERMETFROMURL)->SetWindowText(GetResString(IDS_SV_UPDATE));
	    GetDlgItem(IDC_LOGRESET)->SetWindowText(GetResString(IDS_PW_RESET));
	    GetDlgItem(IDC_MYINFO)->SetWindowText(GetResString(IDS_MYINFO));
	    m_ctrlMyInfo.SetText(GetResString(IDS_MYINFO));

    	//MORPH START - Added by SiRoB, XML News [O²]
		GetDlgItem(IDC_FEEDUPDATE)->SetWindowText(GetResString(IDS_SF_RELOAD)); // Added by O²: XML News
		//MORPH END   - Added by SiRoB, XML News [O²]

		// Mighty Knife: Popup-Menu for editing news feeds
		GetDlgItem(IDC_LOGRESET)->SetWindowText(GetResString(IDS_FEEDBUTTON));
		// [end] Mighty Knife
    
	    TCITEM item;
	    CString name;
	    name = GetResString(IDS_SV_SERVERINFO);
	    item.mask = TCIF_TEXT;
		item.pszText = const_cast<LPTSTR>((LPCTSTR)name);
		StatusSelector.SetItem(PaneServerInfo, &item);

	    name = GetResString(IDS_SV_LOG);
	    item.mask = TCIF_TEXT;
		item.pszText = const_cast<LPTSTR>((LPCTSTR)name);
		StatusSelector.SetItem(PaneLog, &item);

	    //MORPH START - Added by SiRoB, XML News [O²]
		name = "News";
	    item.mask = TCIF_TEXT;
		item.pszText = const_cast<LPTSTR>((LPCTSTR)name);
		StatusSelector.SetItem(PaneNews, &item);
		//MORPH END   - Added by SiRoB, XML News [O²]

	    name = SZ_DEBUG_LOG_TITLE;
	    item.mask = TCIF_TEXT;
		item.pszText = const_cast<LPTSTR>((LPCTSTR)name);
		StatusSelector.SetItem(PaneVerboseLog, &item);
	}

	UpdateLogTabSelection();
	UpdateControlsState();
}

BEGIN_MESSAGE_MAP(CServerWnd, CResizableDialog)
	ON_BN_CLICKED(IDC_ADDSERVER, OnBnClickedAddserver)
	ON_BN_CLICKED(IDC_UPDATESERVERMETFROMURL, OnBnClickedUpdateservermetfromurl)
	ON_BN_CLICKED(IDC_LOGRESET, OnBnClickedResetLog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB3, OnTcnSelchangeTab3)
	ON_NOTIFY(EN_LINK, 123, OnEnLinkServerBox)
	ON_BN_CLICKED(IDC_ED2KCONNECT, OnBnConnect)
	ON_WM_SYSCOLORCHANGE()
	ON_BN_CLICKED(IDC_DD,OnDDClicked)
	ON_WM_HELPINFO()
	ON_EN_CHANGE(IDC_IPADDRESS, OnSvrTextChange)
	ON_EN_CHANGE(IDC_SPORT, OnSvrTextChange)
	ON_EN_CHANGE(IDC_SNAME, OnSvrTextChange)
	ON_EN_CHANGE(IDC_SERVERMETURL, OnSvrTextChange)
	//MORPH START - Added by SiRoB, XML News [O²]
	ON_NOTIFY(EN_LINK, 124, OnEnLinkNewsBox)
	ON_BN_CLICKED(IDC_FEEDUPDATE, DownloadFeed)
	ON_LBN_SELCHANGE(IDC_FEEDLIST, OnFeedListSelChange)
	ON_CBN_DROPDOWN(IDC_FEEDLIST, ListFeeds)
	//MORPH END   - Added by SiRoB, XML News [O²]
	// Mighty Knife: News feed edit button
	ON_BN_CLICKED(IDC_FEEDCHANGE, OnBnClickedFeedchange)
	// [end] Mighty Knife
END_MESSAGE_MAP()


// CServerWnd message handlers

void CServerWnd::OnBnClickedAddserver()
{
	CString serveraddr;
	if (!GetDlgItem(IDC_IPADDRESS)->GetWindowTextLength()){
		AfxMessageBox(GetResString(IDS_SRV_ADDR));
		return;
	}
	else
		GetDlgItem(IDC_IPADDRESS)->GetWindowText(serveraddr);

	UINT uPort = 0;
	if (_tcsncmp(serveraddr, _T("ed2k://"), 7) == 0){
		CED2KLink* pLink = NULL;
		try{
			pLink = CED2KLink::CreateLinkFromUrl(serveraddr);
			serveraddr.Empty();
			if (pLink && pLink->GetKind() == CED2KLink::kServer){
				CED2KServerLink* pServerLink = pLink->GetServerLink();
				if (pServerLink){
					uint32 nServerIP = pServerLink->GetIP();
					uPort = pServerLink->GetPort();
					serveraddr = ipstr(nServerIP);
					SetDlgItemText(IDC_IPADDRESS, serveraddr);
					SetDlgItemInt(IDC_SPORT, uPort, FALSE);
				}
			}
		}
		catch(CString strError){
			AfxMessageBox(strError);
			serveraddr.Empty();
		}
		delete pLink;
	}
	else{
		if (!GetDlgItem(IDC_SPORT)->GetWindowTextLength()){
			AfxMessageBox(GetResString(IDS_SRV_PORT));
			return;
		}

		BOOL bTranslated = FALSE;
		uPort = GetDlgItemInt(IDC_SPORT, &bTranslated, FALSE);
		if (!bTranslated){
			AfxMessageBox(GetResString(IDS_SRV_PORT));
			return;
		}
	}

	if (serveraddr.IsEmpty() || uPort == 0){
		AfxMessageBox(GetResString(IDS_SRV_ADDR));
		return;
	}

	CString strServerName;
	GetDlgItem(IDC_SNAME)->GetWindowText(strServerName);

	AddServer(uPort, serveraddr, strServerName);
}

void CServerWnd::PasteServerFromClipboard()
{
	CString strServer = theApp.CopyTextFromClipboard();
	strServer.Trim();
	if (strServer.IsEmpty())
		return;

	int nPos = 0;
	CString strTok = strServer.Tokenize(_T(" \t\r\n"), nPos);
	while (!strTok.IsEmpty())
	{
		uint32 nIP = 0;
		uint16 nPort = 0;
		CED2KLink* pLink = NULL;
		try{
			pLink = CED2KLink::CreateLinkFromUrl(strTok);
			if (pLink && pLink->GetKind() == CED2KLink::kServer){
				CED2KServerLink* pServerLink = pLink->GetServerLink();
				if (pServerLink){
					nIP = pServerLink->GetIP();
					nPort = pServerLink->GetPort();
				}
			}
		}
		catch(CString strError){
			AfxMessageBox(strError);
		}
		delete pLink;

		if (nIP == 0 || nPort == 0)
			break;

		(void)AddServer(nPort, ipstr(nIP), _T(""), false);
		strTok = strServer.Tokenize(_T(" \t\r\n"), nPos);
	}
}

bool CServerWnd::AddServer(uint16 nPort, CString strIP, CString strName, bool bShowErrorMB)
{
	CServer* toadd = new CServer(nPort, strIP);

	// Barry - Default all manually added servers to high priority
	if( thePrefs.GetManualHighPrio() )
		toadd->SetPreference(SRV_PR_HIGH);

	if (strName.IsEmpty())
		strName = strIP;
	toadd->SetListName(strName);

	if (!serverlistctrl.AddServer(toadd, true))
	{
		CServer* update = theApp.serverlist->GetServerByAddress(toadd->GetAddress(), toadd->GetPort());
		if(update)
		{
			update->SetListName(toadd->GetListName());
			serverlistctrl.RefreshServer(update);
		}
		delete toadd;
		if (bShowErrorMB)
		AfxMessageBox(GetResString(IDS_SRV_NOTADDED));
		return false;
	}
	else
	{
		AddLogLine(true,GetResString(IDS_SERVERADDED), toadd->GetListName());
		return true;
	}
}

void CServerWnd::OnBnClickedUpdateservermetfromurl()
{
	// step1 - get url
	CString strURL;
	bool bDownloaded=false;
	GetDlgItem(IDC_SERVERMETURL)->GetWindowText(strURL);
	
	if (strURL==_T("")){
		if (thePrefs.adresses_list.IsEmpty()){
			AddLogLine(true, GetResString(IDS_SRV_NOURLAV) );
			return;
		}
		else
		{
			POSITION Pos = thePrefs.adresses_list.GetHeadPosition(); 
			while ((!bDownloaded) && (Pos != NULL)){
				strURL = thePrefs.adresses_list.GetNext(Pos).GetBuffer(); 
				bDownloaded=UpdateServerMetFromURL(strURL);
			}
		}
	}
	else
		UpdateServerMetFromURL(strURL);
}

void CServerWnd::OnBnClickedResetLog()
{
	int cur_sel = StatusSelector.GetCurSel();
	if (cur_sel == -1)
		return;
	//MORPH START - Changed by SiRoB, XML News [O²]
	/*
	if (cur_sel == PaneVerboseLog)
	*/
	if( (cur_sel == 3 && news) || (cur_sel == 2 && !news) )
	//MORPH END   - Changed by SiRoB, XML News [O²]
	{
		theApp.emuledlg->ResetDebugLog();
		theApp.emuledlg->statusbar->SetText(_T(""),0,0);
	}
	if (cur_sel == PaneLog)
	{
		theApp.emuledlg->ResetLog();
		theApp.emuledlg->statusbar->SetText(_T(""),0,0);
	}
	if (cur_sel == PaneServerInfo)
	{
		servermsgbox->Reset();
		// the statusbar does not contain any server log related messages, so it's not cleared.
	}

	//MORPH START - Changed by SiRoB, XML News [O²]
	if( cur_sel == 2  && news )
	//if (cur_sel == PaneNews)
	{
		newsmsgbox->Reset();
		// the statusbar does not contain any server log related messages, so it's not cleared.
	}
	//MORPH END   - Changed by SiRoB, XML News [O²]
}

void CServerWnd::OnTcnSelchangeTab3(NMHDR *pNMHDR, LRESULT *pResult)
{
	UpdateLogTabSelection();
	*pResult = 0;
}

void CServerWnd::UpdateLogTabSelection()
{
	int cur_sel = StatusSelector.GetCurSel();
	if (cur_sel == -1)
		return;
	//MORPH START - Changed by SiRoB, XML News [O²]
	/*
	if (cur_sel == PaneVerboseLog)
	*/
	if( (cur_sel == 3 && news) || (cur_sel == 2 && !news) )
	//MORPH END   - Changed by SiRoB, XML News [O²]
	{
		servermsgbox->ShowWindow(SW_HIDE);
		logbox.ShowWindow(SW_HIDE);
		//MORPH START - Added by SiRoB, XML News [O²]
		newsmsgbox->ShowWindow(SW_HIDE); // added by O²: XML News
		//MORPH END   - Added by SiRoB, XML News [O²]
		debuglog.ShowWindow(SW_SHOW);
		StatusSelector.HighlightItem(cur_sel, FALSE);
	}
	if (cur_sel == PaneLog)
	{
		debuglog.ShowWindow(SW_HIDE);
		servermsgbox->ShowWindow(SW_HIDE);
		//MORPH START - Added by SiRoB, XML News [O²]
		newsmsgbox->ShowWindow(SW_HIDE); // added by O²: XML News
		//MORPH END   - Added by SiRoB, XML News [O²]
		logbox.ShowWindow(SW_SHOW);
		StatusSelector.HighlightItem(cur_sel, FALSE);
	}
	if (cur_sel == PaneServerInfo)
	{
		debuglog.ShowWindow(SW_HIDE);
		logbox.ShowWindow(SW_HIDE);
		//MORPH START - Added by SiRoB, XML News [O²]
		newsmsgbox->ShowWindow(SW_HIDE); // added by O²: XML News
		//MORPH END   - Added by SiRoB, XML News [O²]
		servermsgbox->ShowWindow(SW_SHOW);
		servermsgbox->Invalidate();
		StatusSelector.HighlightItem(cur_sel, FALSE);
	}
	
	//MORPH START - Added by SiRoB, XML News [O²]	
	if( cur_sel == 2  && news )
	//if (cur_sel == PaneNews)
	{
		debuglog.ShowWindow(SW_HIDE);
		logbox.ShowWindow(SW_HIDE);
		// eMule O²
		newsmsgbox->ShowWindow(SW_SHOW); // added by O²: XML News
		// END eMule O²
		servermsgbox->ShowWindow(SW_HIDE);
		StatusSelector.HighlightItem(cur_sel, FALSE);
	}
	//MORPH END   - Added by SiRoB, XML News [O²]
}

void CServerWnd::ToggleDebugWindow()
{
	int cur_sel = StatusSelector.GetCurSel();
	//MORPH START - Changed by SiRoB, XML News [O²]
	/*
	if (thePrefs.GetVerbose() && !debug)	
	{
		TCITEM newitem;
		CString name;
		name = SZ_DEBUG_LOG_TITLE;
		newitem.mask = TCIF_TEXT|TCIF_IMAGE;
		newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
		newitem.iImage = 0;
		StatusSelector.InsertItem(StatusSelector.GetItemCount(),&newitem);
		debug = true;
	}
	else if (!thePrefs.GetVerbose() && debug)
	{
		if (cur_sel == PaneVerboseLog)
		{
			StatusSelector.SetCurSel(PaneLog);
			StatusSelector.SetFocus();
		}
		debuglog.ShowWindow(SW_HIDE);
		servermsgbox->ShowWindow(SW_HIDE);
		logbox.ShowWindow(SW_SHOW);
		StatusSelector.DeleteItem(PaneVerboseLog);
		debug = false;
	}
	*/
	if( (cur_sel == 2) || (cur_sel == 3) ){
		// StatusSelector.SetCurSel(2);
		StatusSelector.SetCurSel(1);
		// END Added by O²: XML News
		// END eMule O²
		StatusSelector.SetFocus();
	}
	servermsgbox->ShowWindow(SW_HIDE);
	logbox.ShowWindow(SW_SHOW);
	newsmsgbox->ShowWindow(SW_HIDE);
	debuglog.ShowWindow(SW_HIDE);

	StatusSelector.DeleteItem(3);
	StatusSelector.DeleteItem(2);

	debug = false;
	news = false;

	GetDlgItem(IDC_FEEDUPDATE)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_FEEDLIST)->ShowWindow(SW_HIDE);
	if (thePrefs.GetNews()) {
		TCITEM newitem;
		CString name;
		name = "News";
		newitem.mask = TCIF_TEXT|TCIF_IMAGE;
		newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
		newitem.iImage = 0;
		StatusSelector.InsertItem(StatusSelector.GetItemCount(),&newitem);
		news = true; 

		GetDlgItem(IDC_FEEDUPDATE)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_FEEDLIST)->ShowWindow(SW_SHOW);
	}

	if (thePrefs.GetVerbose()) {
		TCITEM newitem;
		CString name;
		name = SZ_DEBUG_LOG_TITLE;
		newitem.mask = TCIF_TEXT|TCIF_IMAGE;
		newitem.pszText = const_cast<LPTSTR>((LPCTSTR)name);
		newitem.iImage = 0;
		StatusSelector.InsertItem(StatusSelector.GetItemCount(),&newitem);
		debug = true;
	}
	//MORPH END   - Changed by SiRoB, XML News [O²]
}

void CServerWnd::UpdateMyInfo() {
	CString buffer;

	m_MyInfo.SetRedraw(FALSE);
	m_MyInfo.SetWindowText(_T(""));

	///////////////////////////////////////////////////////////////////////////
	// ED2K
	///////////////////////////////////////////////////////////////////////////
	m_MyInfo.SetSelectionCharFormat(m_cfBold);
	m_MyInfo << _T("eD2K ") << GetResString(IDS_NETWORK) << _T("\r\n");
	m_MyInfo.SetSelectionCharFormat(m_cfDef);

	m_MyInfo << GetResString(IDS_STATUS) << _T(":\t");
	if (theApp.serverconnect->IsConnected())
		m_MyInfo << GetResString(IDS_CONNECTED);
	else if(theApp.serverconnect->IsConnecting())
		m_MyInfo << GetResString(IDS_CONNECTING);
	else 
		m_MyInfo << GetResString(IDS_DISCONNECTED);
	m_MyInfo << _T("\r\n");

	if (theApp.serverconnect->IsConnected()){
		m_MyInfo << GetResString(IDS_IP) << _T(":") << GetResString(IDS_PORT);
		if (theApp.serverconnect->IsLowID())
			buffer = GetResString(IDS_UNKNOWN);
		else
			buffer.Format(_T("%s:%i"), ipstr(theApp.serverconnect->GetClientID()), thePrefs.GetPort());
		m_MyInfo << _T("\t") << buffer << _T("\r\n");

		m_MyInfo << GetResString(IDS_ID) << _T("\t");
		if (theApp.serverconnect->IsConnected()){
			buffer.Format(_T("%u"),theApp.serverconnect->GetClientID());
			m_MyInfo << buffer;
		}
		m_MyInfo << _T("\r\n");

		m_MyInfo << _T("\t");
		if (theApp.serverconnect->IsLowID())
			m_MyInfo << GetResString(IDS_IDLOW);
		else
			m_MyInfo << GetResString(IDS_IDHIGH);
		m_MyInfo << _T("\r\n");

		CServer* cur_server = theApp.serverconnect->GetCurrentServer();
		CServer* srv = cur_server ? theApp.serverlist->GetServerByAddress(cur_server->GetAddress(), cur_server->GetPort()) : NULL;
		if (srv){
			m_MyInfo << _T("\r\n");
			m_MyInfo.SetSelectionCharFormat(m_cfBold);
			m_MyInfo << _T("eD2K ") << GetResString(IDS_SERVER) << _T("\r\n");
			m_MyInfo.SetSelectionCharFormat(m_cfDef);

			m_MyInfo << GetResString(IDS_SW_NAME) << _T(":\t") << srv->GetListName() << _T("\r\n");
			m_MyInfo << GetResString(IDS_DESCRIPTION) << _T(":\t") << srv->GetDescription() << _T("\r\n");
			m_MyInfo << GetResString(IDS_IP) << _T(":\t") << srv->GetAddress() << _T(":") << srv->GetPort() << "\r\n";
			//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
//			if (srv->GetConnPort() != srv->GetPort())  
			m_MyInfo << GetResString(IDS_AUXPORTS) << _T(":\t") << srv->GetConnPort() << _T("\r\n"); 
			//Morph End - added by AndCycle, aux Ports, by lugdunummaster
			m_MyInfo << GetResString(IDS_VERSION) << _T(":\t") << srv->GetVersion() << _T("\r\n");
			m_MyInfo << GetResString(IDS_UUSERS) << _T(":\t") << GetFormatedUInt(srv->GetUsers()) << _T("\r\n");
			m_MyInfo << GetResString(IDS_PW_FILES) << _T(":\t") << GetFormatedUInt(srv->GetFiles()) << _T("\r\n");
		}
	}
	m_MyInfo << _T("\r\n");

	///////////////////////////////////////////////////////////////////////////
	// Kademlia
	///////////////////////////////////////////////////////////////////////////
	m_MyInfo.SetSelectionCharFormat(m_cfBold);
	m_MyInfo << GetResString(IDS_KADEMLIA) << _T(" ") << GetResString(IDS_NETWORK) << _T("\r\n");
	m_MyInfo.SetSelectionCharFormat(m_cfDef);
	
	m_MyInfo << GetResString(IDS_STATUS) << _T(":\t");
	if(Kademlia::CKademlia::isConnected()){
		if(Kademlia::CKademlia::isFirewalled())
			m_MyInfo << GetResString(IDS_FIREWALLED);
		else
			m_MyInfo << GetResString(IDS_KADOPEN);
		m_MyInfo << _T("\r\n");

		CString IP;
		Kademlia::CMiscUtils::ipAddressToString(Kademlia::CKademlia::getPrefs()->getIPAddress(),&IP);
		buffer.Format(_T("%s:%i"), IP, thePrefs.GetUDPPort());
		m_MyInfo << GetResString(IDS_IP) << _T(":") << GetResString(IDS_PORT) << _T("\t") << buffer << _T("\r\n");

		buffer.Format(_T("%u"),Kademlia::CKademlia::getPrefs()->getIPAddress());
		m_MyInfo << GetResString(IDS_ID) << _T("\t") << buffer << _T("\r\n");
	}
	else if (Kademlia::CKademlia::isRunning())
		m_MyInfo << GetResString(IDS_CONNECTING) << _T("\r\n");
	else
		m_MyInfo << GetResString(IDS_DISCONNECTED) << _T("\r\n");
	m_MyInfo << _T("\r\n");

	///////////////////////////////////////////////////////////////////////////
	// Web Interface
	///////////////////////////////////////////////////////////////////////////
	m_MyInfo.SetSelectionCharFormat(m_cfBold);
	m_MyInfo << GetResString(IDS_WEBSRV) << _T("\r\n");
	m_MyInfo.SetSelectionCharFormat(m_cfDef);
	m_MyInfo << GetResString(IDS_STATUS) << _T(":\t");
	m_MyInfo << (theApp.webserver->IsRunning() ? GetResString(IDS_ENABLED) : GetResString(IDS_DISABLED)) << _T("\r\n");
	if (thePrefs.GetWSIsEnabled()){
		CString count;
		count.Format(_T("%i %s"),theApp.webserver->GetSessionCount(),GetResString(IDS_ACTSESSIONS));
		m_MyInfo << _T("\t") << count << _T("\r\n");
		uint32 nLocalIP = theApp.serverconnect->GetLocalIP();
		m_MyInfo << _T("URL:\t") << _T("http://") << ipstr(nLocalIP) << _T(":") << thePrefs.GetWSPort() << _T("/\r\n");
	}
	//MORPH START - Added by SiRoB, Mighty Knife: display complete userhash in status window
	m_MyInfo << _T("\r\n");
	buffer.Format(_T("%s"),(LPCTSTR)(md4str((uchar*)thePrefs.GetUserHash())));
	m_MyInfo << GetResString(IDS_CD_UHASH) << _T("\t") << buffer.Left (16) << _T("-");
	m_MyInfo << _T("\r\n\t") << buffer.Mid (16,255);
	//MORPH END   - Added by SiRoB, [end] Mighty Knife

	m_MyInfo.SetRedraw(TRUE);
	m_MyInfo.Invalidate();
}

BOOL CServerWnd::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN){

		if (pMsg->wParam == VK_ESCAPE)
			return FALSE;

		if( m_pacServerMetURL && m_pacServerMetURL->IsBound() && ((pMsg->wParam == VK_DELETE) && (pMsg->hwnd == GetDlgItem(IDC_SERVERMETURL)->m_hWnd) && (GetAsyncKeyState(VK_MENU)<0 || GetAsyncKeyState(VK_CONTROL)<0)) )
			m_pacServerMetURL->Clear();

		if (pMsg->wParam == VK_RETURN){
			if (   pMsg->hwnd == GetDlgItem(IDC_IPADDRESS)->m_hWnd
				|| pMsg->hwnd == GetDlgItem(IDC_SPORT)->m_hWnd
				|| pMsg->hwnd == GetDlgItem(IDC_SNAME)->m_hWnd){

				OnBnClickedAddserver();
				return TRUE;
			}
			else if (pMsg->hwnd == GetDlgItem(IDC_SERVERMETURL)->m_hWnd){
				if (m_pacServerMetURL && m_pacServerMetURL->IsBound() ){
					CString strText;
					GetDlgItem(IDC_SERVERMETURL)->GetWindowText(strText);
					if (!strText.IsEmpty()){
						GetDlgItem(IDC_SERVERMETURL)->SetWindowText(_T("")); // this seems to be the only chance to let the dropdown list to disapear
						GetDlgItem(IDC_SERVERMETURL)->SetWindowText(strText);
						((CEdit*)GetDlgItem(IDC_SERVERMETURL))->SetSel(strText.GetLength(), strText.GetLength());
					}
				}
				OnBnClickedUpdateservermetfromurl();
				return TRUE;
			}
		}
	}
   
	return CResizableDialog::PreTranslateMessage(pMsg);
}

BOOL CServerWnd::SaveServerMetStrings()
{
	if (m_pacServerMetURL== NULL)
		return FALSE;
	return m_pacServerMetURL->SaveList(CString(thePrefs.GetConfigDir()) + _T("\\") SERVERMET_STRINGS_PROFILE);
}

void CServerWnd::ShowServerInfo() {
	CString buffer;

	CServer* cur_server = theApp.serverconnect ? theApp.serverconnect->GetCurrentServer() : NULL;
	CServer* server = cur_server ? theApp.serverlist->GetServerByAddress(cur_server->GetAddress(), cur_server->GetPort()) : NULL;

	if (!theApp.serverconnect->IsConnected() || server==NULL)
		buffer=GetResString(IDS_ERR_NOTCONNECTED);
	else {
		CString buffer2;
		buffer2.Format(_T("%s:\n    %s\n\n"),GetResString(IDS_SL_SERVERNAME),server->GetListName());
		buffer.Append(buffer2);

		buffer2.Format(_T("%s:\n    %s\n\n"),GetResString(IDS_DESCRIPTION),server->GetDescription());
		buffer.Append(buffer2);

		buffer2.Format(_T("%s:\n    %s\n\n"),GetResString(IDS_VERSION),server->GetVersion() );
		buffer.Append(buffer2);

		if (thePrefs.IsExtControlsEnabled()){
			buffer2.Format(_T("%s:\n    "), GetResString(IDS_SRV_TCPCOMPR));
			if (server->GetTCPFlags() & SRV_TCPFLG_COMPRESSION)
				buffer2 += GetResString(IDS_YES);
			else
				buffer2 += GetResString(IDS_NO);
			buffer.Append(buffer2 + _T("\n\n"));
		}
		if (thePrefs.IsExtControlsEnabled()){
			buffer2.Format(_T("%s:\n    "), GetResString(IDS_SRV_UDPSR));
			if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETSOURCES)
				buffer2 += GetResString(IDS_YES);
			else
				buffer2 += GetResString(IDS_NO);
			buffer.Append(buffer2 + _T("\n\n"));

			buffer2.Format(_T("%s:\n    "), GetResString(IDS_SRV_UDPFR));
			if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)
				buffer2 += GetResString(IDS_YES);
			else
				buffer2 += GetResString(IDS_NO);
			buffer.Append(buffer2 + _T("\n\n"));
		}

		buffer2.Format(_T("%s%s:\n    %s:%u\n\n"),GetResString(IDS_CD_UIP),GetResString(IDS_PORT),server->GetFullIP(),server->GetPort() );
		buffer.Append(buffer2);

		buffer2.Format(_T("%s:\n    %u\n\n"),GetResString(IDS_PW_FILES),server->GetFiles());
		buffer.Append(buffer2);

		buffer2.Format(_T("%s:\n    %u / %u\n\n"),GetResString(IDS_SERVER_LIMITS),server->GetSoftFiles(),server->GetHardFiles());
		buffer.Append(buffer2);

		buffer2.Format(_T("%s:\n    %u / %u\n\n"),GetResString(IDS_UUSERS),server->GetUsers(),server->GetMaxUsers());
		buffer.Append(buffer2);

		buffer2.Format(_T("%s:\n    %u ms\n\n"),GetResString(IDS_PING),server->GetPing());
		buffer.Append(buffer2);
		
	}
	MessageBox(buffer, GetResString(IDS_SERVERINFO));
}

void CServerWnd::OnEnLinkServerBox(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	ENLINK* pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	if (pEnLink && pEnLink->msg == WM_LBUTTONDOWN)
	{
		CString strUrl;
		servermsgbox->GetTextRange(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax, strUrl);
		if (strUrl == m_strClickNewVersion){
			// MOD Note: Do not remove this part - Merkur
					strUrl.Format(_T("/en/version_check.php?version=%i&language=%i"),theApp.m_uCurVersionCheck,thePrefs.GetLanguageID());
					strUrl = thePrefs.GetVersionCheckBaseURL()+strUrl;
			// MOD Note: end
		}
		ShellExecute(NULL, NULL, strUrl, NULL, NULL, SW_SHOWDEFAULT);
		*pResult = 1;
	}
}

void CServerWnd::UpdateControlsState()
{
	CString strLabel;
	if( theApp.serverconnect->IsConnected() )
		strLabel = GetResString(IDS_MAIN_BTN_DISCONNECT);
	else if( theApp.serverconnect->IsConnecting() )
		strLabel = GetResString(IDS_MAIN_BTN_CANCEL);
	else
		strLabel = GetResString(IDS_MAIN_BTN_CONNECT);
	strLabel.Remove(_T('&'));
	GetDlgItem(IDC_ED2KCONNECT)->SetWindowText(strLabel);
}

void CServerWnd::OnBnConnect()
{
	if (theApp.serverconnect->IsConnected())
		theApp.serverconnect->Disconnect();
	else if (theApp.serverconnect->IsConnecting() )
		theApp.serverconnect->StopConnectionTry();
	else
		theApp.serverconnect->ConnectToAnyServer();
}

void CServerWnd::SaveAllSettings()
{
	thePrefs.SetLastLogPaneID(StatusSelector.GetCurSel());
	serverlistctrl.SaveSettings(CPreferences::tableServer);
	SaveServerMetStrings();
}

void CServerWnd::OnDDClicked() {
	
	CWnd* box=GetDlgItem(IDC_SERVERMETURL);
	box->SetFocus();
	box->SetWindowText(_T(""));
	box->SendMessage(WM_KEYDOWN,VK_DOWN,0x00510001);
	
}
void CServerWnd::ResetHistory() {

	if (m_pacServerMetURL==NULL) return;

	GetDlgItem(IDC_SERVERMETURL)->SendMessage(WM_KEYDOWN, VK_ESCAPE, 0x00510001);
		m_pacServerMetURL->Clear();
}
BOOL CServerWnd::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.ShowHelp(eMule_FAQ_Update_Server);
	return TRUE;
}

void CServerWnd::OnSvrTextChange()
{
	GetDlgItem(IDC_ADDSERVER)->EnableWindow(GetDlgItem(IDC_IPADDRESS)->GetWindowTextLength());
	GetDlgItem(IDC_UPDATESERVERMETFROMURL)->EnableWindow( GetDlgItem(IDC_SERVERMETURL)->GetWindowTextLength()>0 );
}

//MORPH START - Added by SiRoB, XML News [O²]
void CServerWnd::ListFeeds()
{
	while (m_feedlist.GetCount()>0)
		m_feedlist.DeleteString(0);
	aFeedUrls.RemoveAll();
	int counter=0;
	CString sbuffer;
	char buffer[1024];
	int lenBuf = 1024;

	FILE* readFile= fopen(CString(thePrefs.GetConfigDir())+"XMLNews.dat", "r");
	if (readFile!=NULL)
	{
		GetDlgItem(IDC_FEEDLIST)->EnableWindow();
		while (!feof(readFile))
		{
			if (fgets(buffer,lenBuf,readFile)==0)
				break;
			sbuffer=buffer;
			
			// ignore comments & too short lines
			if (sbuffer.GetAt(0) == '#' || sbuffer.GetAt(0) == '/' || sbuffer.GetLength()<5)
				continue;
			
			int pos=sbuffer.Find(',');
			if (pos>0 && pos<sbuffer.GetLength())
			{
				counter++;
				m_feedlist.AddString(sbuffer.Left(pos).Trim());
				aFeedUrls.Add(sbuffer.Right(sbuffer.GetLength()-pos-1).Trim());
			}
		}
		fclose(readFile);
	}
	else
	{
		GetDlgItem(IDC_FEEDUPDATE)->EnableWindow(false);
		GetDlgItem(IDC_FEEDLIST)->EnableWindow(false);
	}
}

void CServerWnd::DownloadFeed()
{
	CString sbuffer;
	int numero = m_feedlist.GetCurSel();
	// if no item is selected there's nothing to do...
	if (numero==CB_ERR) return;
	// Get the URL to download
	CString strURL = aFeedUrls.GetAt(numero);
	CString strTempFilename; 
	// Delete the old temporary copy in the "feeds" subdirectory
	// of this feed if necessary
	strTempFilename.Format("%s%d.xml",thePrefs.GetFeedsDir(),numero);
	FILE* readFile = fopen(strTempFilename, "r");
	if (readFile!=NULL)
	{
		fclose(readFile);
		remove(strTempFilename);
	}
	readFile = fopen(strTempFilename, "r");
	// Start the download dialog and retrieve the file
	CHttpDownloadDlg dlgDownload;
	dlgDownload.m_strTitle = _T("Download RSS feed file");
	dlgDownload.m_sURLToDownload = strURL;
	dlgDownload.m_sFileToDownloadInto = strTempFilename;
	if (dlgDownload.DoModal() != IDOK)
	{
		theApp.emuledlg->AddLogLine(true, "Error downloading %s", strURL);
		return;
	}
	// Parse it
	ParseNewsFile(strTempFilename);
}

// Parses a node of the news file.
// Add all "item" nodes to the news messagebox of the server window.
// Don't add the message _xmlbuffer - that was already added since it
// represents the description of the news page itself.
void CServerWnd::ParseNewsNode(pug::xml_node _node, CString _xmlbuffer) {
	CString sbuffer;
	CString sxmlbuffer;
	using namespace pug;
	xml_node_list item;
	for(xml_node::child_iterator i = _node.children_begin(); i < _node.children_end(); ++i) {
		CString c = CString(i->name());
		if (CString(i->name()) == CString("item")) {
			aXMLUrls.Add(i->first_element_by_path("./link").child(0).value());
			sbuffer = i->first_element_by_path("./title").child(0).value();
			HTMLParse(sbuffer);
			newsmsgbox->AppendText("\n• ");
			newsmsgbox->AppendHyperLink(_T(""),_T(""),sbuffer,_T(""),false);
			aXMLNames.Add(sbuffer);
			if (!i->first_element_by_path("./author").child(0).empty())
			{
				sxmlbuffer = i->first_element_by_path("./author").child(0).value();
				newsmsgbox->AppendText(CString(" - Par: ")+sxmlbuffer);
			}
			CString buffer = i->first_element_by_path("./description").child(0).value();
			HTMLParse(buffer);
			if (buffer != _xmlbuffer && !buffer.IsEmpty())
			{
				if (sxmlbuffer.IsEmpty())
					newsmsgbox->AppendText("\n");
				int index = 0;
				while (buffer.Find("<a href=\"") != -1)
				{
					index = buffer.Find("<a href=\"");
					newsmsgbox->AppendText(buffer.Left(index));
					buffer = buffer.Mid(index+9);
					index = buffer.Find("\"");
					sbuffer = buffer.Left(index);
					aXMLUrls.Add(sbuffer);
					buffer = buffer.Mid(index+1);
					index = buffer.Find(">");
					buffer = buffer.Mid(index+1);
					index = buffer.Find("</a>");
					sbuffer = buffer.Left(index);
					aXMLNames.Add(sbuffer);
					newsmsgbox->AppendHyperLink(_T(""),_T(""),sbuffer,_T(""),false);
					buffer = buffer.Mid(index+4);
				}
				newsmsgbox->AppendText(buffer+"\n");
			}
		}
	}
}

void CServerWnd::ParseNewsFile(CString strTempFilename)
{
	CString sbuffer;
	newsmsgbox->Reset();
	aXMLUrls.RemoveAll();
	aXMLNames.RemoveAll();

	// Look if the news file exists in the "feed" subdirectory
	if (!PathFileExists(strTempFilename)){
		StatusSelector.SetCurSel(2);
		UpdateLogTabSelection();
		return;
	}

	// Generate an XML parser. THat thing can be found in the "pugxml" library.
	// It uses the namespace "pug".
	using namespace pug;
	xml_parser* xml = new xml_parser();

	// Load and parse the XML file
	xml->parse_file(strTempFilename);

	// Create two XML nodes. One node represents the root and one represets
	// the "channel" section in the file.
	xml_node itelem;
	xml_node itelemroot;
	if (!xml->document().first_element_by_path("./rss").empty()) {
		itelemroot = xml->document().first_element_by_path("./rss");
		itelem = xml->document().first_element_by_path("./rss/channel");
	} else if (!xml->document().first_element_by_path("./rdf:RDF").empty()) {
		itelemroot = xml->document().first_element_by_path("./rdf:RDF");
		itelem = xml->document().first_element_by_path("./rdf:RDF/channel");
	} else {
		delete xml;
		return;
	}

	// We'll only continue if we find the "channel" section.
	if(!itelem.empty()) {
		// Add the data in this section to the News box. 
		// It represents the title of the news channel and so on...
		aXMLUrls.Add(itelem.first_element_by_path("./link").child(0).value());
		sbuffer = itelem.first_element_by_path("./title").child(0).value();
		HTMLParse(sbuffer);
		sbuffer.Replace("'","`");
		newsmsgbox->AppendHyperLink(_T(""),_T(""),sbuffer,_T(""),false);
		aXMLNames.Add(sbuffer);
		// The xmlbuffer stores the description of the newsfile itself.
		// We pass this to ParseNewsNode to prevent this description to be
		// added twice: Some news pages put the most important message
		// in both the page description and the first item.
		CString xmlbuffer = itelem.first_element_by_path("./description").child(0).value();
		HTMLParse(xmlbuffer);
		newsmsgbox->AddEntry(CString("\n	")+xmlbuffer);
		// News-items can be found either in the "channel"-node...
		ParseNewsNode (itelem, xmlbuffer);
		// ...and in the root node of the xml file. 
		ParseNewsNode (itelemroot, xmlbuffer);
		// On which node they can fe found depends on the one who generates 
		// the XML file.
		newsmsgbox->AppendText("\n");
	}
	delete xml;
	newsmsgbox->ScrollToFirstLine();
	StatusSelector.SetCurSel(2);
	UpdateLogTabSelection();
}

void CServerWnd::OnEnLinkNewsBox(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	ENLINK* pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	if (pEnLink && pEnLink->msg == WM_LBUTTONDOWN)
	{
		CString strUrl;
		newsmsgbox->GetTextRange(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax, strUrl);
		for (int i=0;i<aXMLNames.GetCount();i++)
			if (aXMLNames[i] == strUrl)
			{
				strUrl = aXMLUrls[i];
				break;
			}
		ShellExecute(NULL, NULL, strUrl, NULL, NULL, SW_SHOWDEFAULT);
		*pResult = 1;
	}
}

void CServerWnd::OnFeedListSelChange()
{
	GetDlgItem(IDC_FEEDUPDATE)->EnableWindow();
	CString strTempFilename;
	strTempFilename.Format("%s%d.xml",thePrefs.GetFeedsDir(),m_feedlist.GetCurSel());
	ParseNewsFile(strTempFilename);
}
//MORPH END - Added by SiRoB, XML News [O²]

// Mighty Knife: News feed edit button
void CServerWnd::OnBnClickedFeedchange()
{
	// Generate a popup menu directly next to the button.
	// The user can change the news-feeds with the items in this menu.
	CWnd* FeedButton = GetDlgItem (IDC_FEEDCHANGE);
	CRect R;
	FeedButton->GetWindowRect (R);
	R.left += R.Width ();
	m_FeedsMenu.TrackPopupMenu (TPM_LEFTALIGN,R.left,R.top,this);
}

BOOL CServerWnd::OnCommand(WPARAM wParam, LPARAM lParam) {
	switch (wParam) {
		case MP_NEWFEED: {
			// Show two input boxes so that the user can enter the URL
			// and optionally a name for that feed
			InputBox inp;
			inp.SetLabels (GetResString (IDS_ADDNEWSFEED),
						   GetResString (IDS_FEEDURL),
						   "");
			inp.DoModal ();
			CString url = inp.GetInput ();
			if ((!inp.WasCancelled()) && (url != "")) {
				// Create a 2nd Input box because the default implementation
				// of this class does not reset the m_Cancel variable!
				InputBox inp2;
				inp2.SetLabels (GetResString (IDS_ADDNEWSFEED),
								GetResString (IDS_FEEDNAME),
							    "");
				inp2.DoModal ();
				if (!inp2.WasCancelled()) {
					CString name = inp2.GetInput ();
					// Reload the XML list
					CStringList names;
					CStringList urls;
					ReadXMLList (names,urls);
					// Append the new adresses
					names.AddTail (name);
					urls.AddTail (url);
					// Rewrite the XML News file
					WriteXMLList (names,urls);
					ListFeeds ();
				}
			}
			return true;
		} break;
		case MP_EDITFEED: {
			// Show two input boxes so that the user can edit the URL
			// and the name for that feed
			int i=m_feedlist.GetCurSel ();
			if (i != CB_ERR) {
				// Reload the XML list
				CStringList names;
				CStringList urls;
				ReadXMLList (names,urls);
				// Find our entry
				if (i < names.GetCount ()) {
					// But first we have to walk to it
					POSITION namepos = names.GetHeadPosition ();
					POSITION urlpos = urls.GetHeadPosition ();
					while (i > 0) {
						names.GetNext (namepos);
						urls.GetNext (urlpos);
						i--;
					}
					// Create an input box
					InputBox inp;
					inp.SetLabels (GetResString (IDS_EDITNEWSFEED),
								GetResString (IDS_FEEDURL),
								urls.GetAt (urlpos));
					inp.DoModal ();
					CString url = inp.GetInput ();
					if ((!inp.WasCancelled()) && (url != "")) {
					    // Create a 2nd Input box because the default implementation
						// of this class does not reset the m_Cancel variable!
						InputBox inp2;
						inp2.SetLabels (GetResString (IDS_EDITNEWSFEED),
										GetResString (IDS_FEEDNAME),
										names.GetAt (namepos));
						inp2.DoModal ();
						if (!inp2.WasCancelled()) {
							CString name = inp2.GetInput ();
							// Append the new adresses
							names.SetAt (namepos,name);
							urls.SetAt (urlpos,url);
							// Rewrite the XML News file
							WriteXMLList (names,urls);
							ListFeeds ();
							m_feedlist.SetCurSel (-1);
						}
					}
				}
			}
			return true;
		} break;
		case MP_DELETEFEED: {
			int sel=m_feedlist.GetCurSel ();
			if (sel != CB_ERR) {
				// Reload the XML list
				CStringList names;
				CStringList urls;
				ReadXMLList (names,urls);
				// Remove the entry
				int i=sel;
				if (i < names.GetCount ()) {
					// But first we have to walk to it
					POSITION namepos = names.GetHeadPosition ();
					POSITION urlpos = urls.GetHeadPosition ();
					while (i > 0) {
						names.GetNext (namepos);
						urls.GetNext (urlpos);
						i--;
					}
					// Got it - throw it away
					names.RemoveAt (namepos);
					urls.RemoveAt (urlpos);
					// Rewrite the XML News file
					WriteXMLList (names,urls);
					// The last thing we have to do is to rename all 
					// files in the "feed" subdirectory (temporary stored
					// news) so that their numbers fit again to the
					// corresponding number of the entrys in the ComboBox.
					for (int j=sel; j < names.GetSize (); j++) {
						CString Source, Dest;
						Source.Format("%s%d.xml",thePrefs.GetFeedsDir(),j+1);
						Dest.Format("%s%d.xml",thePrefs.GetFeedsDir(),j);
						DeleteFile ((const char*) Dest);
						rename ((const char*) Source,(const char*) Dest);
					}

				}
			}
			// Reload the list, change the ComboBox to "nothing", remove the
			// content of the "news" window.
			ListFeeds();
			m_feedlist.SetCurSel (-1);
			GetDlgItem(IDC_FEEDUPDATE)->EnableWindow(false);
			newsmsgbox->Reset();
			return true;
		} break;
		case MP_DELETEALLFEEDS: {
			// Delete the file and reload the feeds for the ComboBox
			DeleteFile (CString(thePrefs.GetConfigDir())+"XMLNews.dat");
			ListFeeds ();
			return true;
		} break;
		default: return CResizableDialog::OnCommand (wParam, lParam);
	}
	return true;
}

// Read the content of the XMLNews.dat file. The file is constructed as:
//    name, url
//    name, url
//    name, url
// ...
// There's no "," sign allowed in the name!
void CServerWnd::ReadXMLList (CStringList& _names, CStringList& _urls) {
	FILE* readfile = fopen(CString(thePrefs.GetConfigDir())+"XMLNews.dat", "r");
	if (readfile == NULL) return; 
	while (!feof (readfile)) {
		// Read the current line
		CString url;
		char* pbuf = url.GetBuffer (1024);
		fgets (pbuf,1024,readfile);
		url.ReleaseBuffer ();
		// Remove all LF characters
		url = url.SpanExcluding ("\n");
		// Split the string on the place of the ","
		int i=url.Find (',');
		if (i != -1) {
			CString name = url.Left (i);
			url.Delete (0,i+1);
			// If the name is empty, use the URL as name
			if (name.IsEmpty ()) 
				name = url;
			_names.AddTail (name);
			_urls.AddTail (url);
		}
	}
	fclose (readfile);
}

// Rewrite the XMLNews.dat file.
// Filter all "," characters if some exist
void CServerWnd::WriteXMLList (CStringList& _names, CStringList& _urls) {
	FILE* writefile = fopen(CString(thePrefs.GetConfigDir())+"XMLNews.dat", "w");
	if (writefile == NULL) return; 
	POSITION posnames = _names.GetHeadPosition ();
	POSITION posurls = _urls.GetHeadPosition ();
	while ((posnames != NULL) && (posurls != NULL)) {
		CString name = _names.GetNext (posnames);
		CString url = _urls.GetNext (posurls);
		// If the name is empty, use the URL as name
		if (name.IsEmpty ()) 
			name = url;
		// Replace all "," by " "
		name.Replace (',',' ');
		// Write the info; append CR/LF characters to the end of the line
		fprintf (writefile,"%s,%s\n",(const char*) name,(const char*) url);
	}
	fclose (writefile);
}

// [end] Mighty Knife
