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


// ServerWnd.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "ServerWnd.h"
#include "HttpDownloadDlg.h"
#include "HTRichEditCtrl.h"
#include "ED2KLink.h"
#include "kademlia/kademlia/kademlia.h"
#include "kademlia/kademlia/prefs.h"
#include "kademlia/utils/MiscUtils.h"

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
	m_pacServerMetURL=NULL;
	m_uLangID = MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT);
	icon_srvlist = NULL;
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
}

BOOL CServerWnd::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	logbox.Init(GetResString(IDS_SV_LOG));
	debuglog.Init(SZ_DEBUG_LOG_TITLE);

	Localize();
	serverlistctrl.Init(theApp.serverlist);
	
	m_ctrlNewServerFrm.Init("AddServer");
	m_ctrlUpdateServerFrm.Init("ServerUpdateMET");
	m_ctrlMyInfo.Init("MyInfo");

	((CEdit*)GetDlgItem(IDC_SPORT))->SetLimitText(5);
	GetDlgItem(IDC_SPORT)->SetWindowText("4661");
	CRect rect;

	GetDlgItem(IDC_SERVMSG)->GetWindowRect(rect);
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rect, 2);
	if (servermsgbox->Create(WS_VISIBLE | WS_CHILD | WS_HSCROLL | WS_VSCROLL | ES_MULTILINE | ES_READONLY, rect, this, 123)){
		servermsgbox->ModifyStyleEx(0, WS_EX_STATICEDGE, SWP_FRAMECHANGED);
		servermsgbox->SetEventMask(servermsgbox->GetEventMask() | ENM_LINK);
		servermsgbox->SetFont(&theApp.emuledlg->m_fontHyperText);

		servermsgbox->AppendText(CString(CString("eMule v")+theApp.m_strCurVersionLong+CString("\n")));
		// MOD Note: Do not remove this part - Merkur
		m_strClickNewVersion = GetResString(IDS_EMULEW) + _T(" ") + GetResString(IDS_EMULEW3) + _T(" ") + _T(GetResString(IDS_EMULEW2));
		servermsgbox->AppendHyperLink(_T(""),_T(""),m_strClickNewVersion,_T(""),false);
		// MOD Note: end
		servermsgbox->AppendText(CString("\n\n"));
	}
	TCITEM newitem;
	CString name;
	name.Format("%s", GetResString(IDS_SV_SERVERINFO) );
	newitem.mask = TCIF_TEXT|TCIF_IMAGE;
	newitem.pszText = name.GetBuffer();
	newitem.cchTextMax = (int)name.GetLength()+1;
	newitem.iImage = 1;
	StatusSelector.InsertItem(StatusSelector.GetItemCount(),&newitem);
	name.Format("%s", GetResString(IDS_SV_LOG));
	newitem.mask = TCIF_TEXT|TCIF_IMAGE;
	newitem.pszText = name.GetBuffer();
	newitem.cchTextMax = (int)name.GetLength()+1;
	newitem.iImage = 0;
	StatusSelector.InsertItem(StatusSelector.GetItemCount(),&newitem);
	name=SZ_DEBUG_LOG_TITLE;
	newitem.mask = TCIF_TEXT|TCIF_IMAGE;
	newitem.pszText = name.GetBuffer();
	newitem.cchTextMax = (int)name.GetLength()+1;
	newitem.iImage = 0;
	StatusSelector.InsertItem(StatusSelector.GetItemCount(),&newitem);

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
	if (servermsgbox->m_hWnd)
		AddAnchor(*servermsgbox, CSize(0,50), BOTTOM_RIGHT);
	debug = true;
	ToggleDebugWindow();

	debuglog.ShowWindow(SW_HIDE);
	logbox.ShowWindow(SW_HIDE);
	if (servermsgbox->m_hWnd)
		servermsgbox->ShowWindow(SW_SHOW);

	CString nameCountStr; 
	MyInfoList = (CMuleListCtrl*)GetDlgItem(IDC_MYINFOLIST); 
	
	((CListCtrl*)GetDlgItem(IDC_MYINFOLIST))->SetExtendedStyle(LVS_EX_FULLROWSELECT);

	if (MyInfoList->GetHeaderCtrl()->GetItemCount() < 2) { 
		MyInfoList->DeleteColumn(0); 
		MyInfoList->InsertColumn(0, "", LVCFMT_LEFT, 60, -1); 
		MyInfoList->InsertColumn(1, "", LVCFMT_LEFT, 126, 1); 
	}

	if (theApp.glob_prefs->GetUseAutocompletion()){
		m_pacServerMetURL = new CCustomAutoComplete();
		m_pacServerMetURL->AddRef();
		if (m_pacServerMetURL->Bind(::GetDlgItem(m_hWnd, IDC_SERVERMETURL), ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST | ACO_FILTERPREFIXES ))
			m_pacServerMetURL->LoadList(CString(theApp.glob_prefs->GetConfigDir()) +  _T("\\") SERVERMET_STRINGS_PROFILE);
	}

	InitWindowStyles(this);

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
}

bool CServerWnd::UpdateServerMetFromURL(CString strURL) {
	if ((strURL=="") || (strURL.Find("://") == -1))	// not a valid URL
	{
		AddLogLine(true, GetResString(IDS_INVALIDURL) );
		return false;
	}

	CString strTempFilename;
	strTempFilename.Format("%stemp-%d-server.met", theApp.glob_prefs->GetConfigDir(), ::GetTickCount());

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
	remove(strTempFilename);
	return true;
}

void CServerWnd::OnSysColorChange()
{
	Localize();
	CResizableDialog::OnSysColorChange();
}

void CServerWnd::Localize()
{
	CImageList iml;
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.Add(CTempIconLoader("Log"));
	iml.Add(CTempIconLoader("ServerInfo"));
	CImageList* piml = StatusSelector.SetImageList(&iml);
	if (piml)
		piml->DeleteImageList();
	iml.Detach();

	if (icon_srvlist)
		VERIFY( DestroyIcon(icon_srvlist) );
	icon_srvlist = theApp.LoadIcon("ServerList", 16, 16);
	((CStatic*)GetDlgItem(IDC_SERVLST_ICO))->SetIcon(icon_srvlist);

	serverlistctrl.Localize();

	if (theApp.glob_prefs->GetLanguageID() != m_uLangID){
		m_uLangID = theApp.glob_prefs->GetLanguageID();
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
    
	    TCITEM item;
	    CString name;
	    name = GetResString(IDS_SV_SERVERINFO);
	    item.mask = TCIF_TEXT;
	    item.pszText = name.GetBuffer();
	    StatusSelector.SetItem( 0, &item);
	    name = GetResString(IDS_SV_LOG);
	    item.mask = TCIF_TEXT;
	    item.pszText = name.GetBuffer();
	    StatusSelector.SetItem( 1, &item);
	    name = SZ_DEBUG_LOG_TITLE;
	    item.mask = TCIF_TEXT;
	    item.pszText = name.GetBuffer();
	    StatusSelector.SetItem( 2, &item);
	}

	UpdateLogTabSelection();
	UpdateControlsState();

}

BEGIN_MESSAGE_MAP(CServerWnd, CResizableDialog)
	ON_BN_CLICKED(IDC_ADDSERVER, OnBnClickedAddserver)
	ON_BN_CLICKED(IDC_UPDATESERVERMETFROMURL, OnBnClickedUpdateservermetfromurl)
	ON_BN_CLICKED(IDC_LOGRESET, OnBnClickedResetLog)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB3, OnTcnSelchangeTab3)
	ON_WM_CTLCOLOR()
	ON_NOTIFY(EN_LINK, 123, OnEnLinkServerBox)
	ON_BN_CLICKED(IDC_ED2KCONNECT, OnBnConnect)
	ON_WM_SYSCOLORCHANGE()
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
					serveraddr = inet_ntoa(*(in_addr*)&nServerIP);
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

	CServer* toadd = new CServer(uPort,serveraddr.GetBuffer());

	// Barry - Default all manually added servers to high priority
	if( theApp.glob_prefs->GetManualHighPrio() )
		toadd->SetPreference(SRV_PR_HIGH);

	int32 namelen = GetDlgItem(IDC_SNAME)->GetWindowTextLength();
	if (namelen){
		char* servername = new char[namelen+2];
		GetDlgItem(IDC_SNAME)->GetWindowText(servername,namelen+1);
		toadd->SetListName(servername);
		delete[] servername;
	}
	else{
		toadd->SetListName(serveraddr.GetBuffer());
	}
	if (!serverlistctrl.AddServer(toadd,true)){
		CServer* update = theApp.serverlist->GetServerByAddress(toadd->GetAddress(), toadd->GetPort());
		if(update){
			update->SetListName(toadd->GetListName());
			serverlistctrl.RefreshServer(update);
		}
		delete toadd;
		AfxMessageBox(GetResString(IDS_SRV_NOTADDED));
	}
	else
		AddLogLine(true,GetResString(IDS_SERVERADDED), toadd->GetListName());
}

void CServerWnd::OnBnClickedUpdateservermetfromurl()
{
	// step1 - get url
	CString strURL;
	bool bDownloaded=false;
	GetDlgItem(IDC_SERVERMETURL)->GetWindowText(strURL);
	
	if (strURL==""){
		if (theApp.glob_prefs->adresses_list.IsEmpty()){
			AddLogLine(true, GetResString(IDS_SRV_NOURLAV) );
			return;
		}
		else
		{
			POSITION Pos = theApp.glob_prefs->adresses_list.GetHeadPosition(); 
			while ((!bDownloaded) && (Pos != NULL)){
				strURL = theApp.glob_prefs->adresses_list.GetNext(Pos).GetBuffer(); 
				bDownloaded=UpdateServerMetFromURL(strURL);
			}
		}
	}
	else
		UpdateServerMetFromURL(strURL);
}

void CServerWnd::OnBnClickedResetLog() {
	int cur_sel = StatusSelector.GetCurSel();
	if (cur_sel == (-1))
		return;
	if( cur_sel == 2 ){
		theApp.emuledlg->ResetDebugLog();
		theApp.emuledlg->statusbar.SetText(_T(""),0,0);
	}
	if( cur_sel == 1 ){
		theApp.emuledlg->ResetLog();
		theApp.emuledlg->statusbar.SetText(_T(""),0,0);
	}
	if( cur_sel == 0 ){
		servermsgbox->Reset();
		// the statusbar does not contain any server log related messages, so it's not cleared.
	}
}

void CServerWnd::OnTcnSelchangeTab3(NMHDR *pNMHDR, LRESULT *pResult)
{
	UpdateLogTabSelection();
	*pResult = 0;
}

void CServerWnd::UpdateLogTabSelection() {
	int cur_sel = StatusSelector.GetCurSel();
	if (cur_sel == (-1))
		return;
	if( cur_sel == 2 ){
		servermsgbox->ShowWindow(SW_HIDE);
		logbox.ShowWindow(SW_HIDE);
		debuglog.ShowWindow(SW_SHOW);
	}
	if( cur_sel == 1 ){
		debuglog.ShowWindow(SW_HIDE);
		servermsgbox->ShowWindow(SW_HIDE);
		logbox.ShowWindow(SW_SHOW);
	}
	if( cur_sel == 0 ){
		debuglog.ShowWindow(SW_HIDE);
		logbox.ShowWindow(SW_HIDE);
		servermsgbox->ShowWindow(SW_SHOW);
		servermsgbox->Invalidate();
	}
}

void CServerWnd::ToggleDebugWindow(){
	int cur_sel = StatusSelector.GetCurSel();
	if( theApp.glob_prefs->GetVerbose() && debug == false ){
		TCITEM newitem;
		CString name;
		name = SZ_DEBUG_LOG_TITLE;
		newitem.mask = TCIF_TEXT|TCIF_IMAGE;
		newitem.pszText = name.GetBuffer();
		newitem.cchTextMax = (int)name.GetLength()+1;
		newitem.iImage = 0;
		StatusSelector.InsertItem(StatusSelector.GetItemCount(),&newitem);
		debug = true;
	}
	else if( !theApp.glob_prefs->GetVerbose() && debug == true ) {
		if( cur_sel == 2 ){
			StatusSelector.SetCurSel(1);
			StatusSelector.SetFocus();
		}
		debuglog.ShowWindow(SW_HIDE);
		servermsgbox->ShowWindow(SW_HIDE);
		logbox.ShowWindow(SW_SHOW);
		StatusSelector.DeleteItem(2);
		debug = false;
	}
}

void CServerWnd::UpdateMyInfo() {
	CString buffer;
	int item;
	MyInfoList->DeleteAllItems();
	MyInfoList->InsertItem(0,"ED2K");
	MyInfoList->InsertItem(1,GetResString(IDS_STATUS)+":");
	if (theApp.serverconnect->IsConnected())
		MyInfoList->SetItemText(1, 1, GetResString(IDS_CONNECTED));
	else
		if(theApp.serverconnect->IsConnecting())
			MyInfoList->SetItemText(1, 1, GetResString(IDS_CONNECTING)); 
		else 
			MyInfoList->SetItemText(1, 1, GetResString(IDS_DISCONNECTED)); 

	if (theApp.serverconnect->IsConnected()) {
		MyInfoList->InsertItem(2,GetResString(IDS_IP) +":"+GetResString(IDS_PORT) );
		if (theApp.serverconnect->IsLowID())
			buffer=GetResString(IDS_UNKNOWN);
		else {
			uint32 myid=theApp.serverconnect->GetClientID();
			uint8 d=myid/(256*256*256);myid-=d*(256*256*256);
			uint8 c=myid/(256*256);myid-=c*256*256;
			uint8 b=myid/(256);myid-=b*256;
			buffer.Format("%i.%i.%i.%i:%i",myid,b,c,d,theApp.glob_prefs->GetPort());
		}
		MyInfoList->SetItemText(2,1,buffer);

		buffer.Format("%u",theApp.serverconnect->GetClientID());
		MyInfoList->InsertItem(3,GetResString(IDS_ID));
		if (theApp.serverconnect->IsConnected()) 
			MyInfoList->SetItemText(3, 1, buffer); 

		MyInfoList->InsertItem(4,"");
		if (theApp.serverconnect->IsLowID())
			MyInfoList->SetItemText(4, 1,GetResString(IDS_IDLOW));
		else MyInfoList->SetItemText(4, 1,GetResString(IDS_IDHIGH));
	}

	MyInfoList->InsertItem(5,"");
	MyInfoList->InsertItem(6,"KADEMLIA");
	item = MyInfoList->InsertItem(7,GetResString(IDS_STATUS)+":");
//	if( Kademlia::CTimer::getThreadID()){
		if(theApp.kademlia->isConnected()){
			if(theApp.kademlia->isFirewalled())
				MyInfoList->SetItemText(item,1,"Firewalled");
			else
				MyInfoList->SetItemText(item,1,"Open");
			item=MyInfoList->InsertItem(8,GetResString(IDS_IP) +":"+GetResString(IDS_PORT) );
			CString IP;
			Kademlia::CMiscUtils::ipAddressToString(theApp.kademlia->getIP(),&IP);
			buffer.Format("%s:%i", IP, theApp.kademlia->getUdpPort());
			MyInfoList->SetItemText(item,1,buffer);
			item=MyInfoList->InsertItem(9,GetResString(IDS_ID));
			buffer.Format("%u",theApp.kademlia->getIP());
			MyInfoList->SetItemText(item,1,buffer);
		}
		else if (Kademlia::CTimer::getThreadID())
			MyInfoList->SetItemText(item,1,GetResString(IDS_CONNECTING));
		else
			MyInfoList->SetItemText(item,1,GetResString(IDS_DISCONNECTED));
//	}
	MyInfoList->InsertItem(10,"");
	item=MyInfoList->InsertItem(11,GetResString(IDS_WEBSRV));
	MyInfoList->SetItemText(item,1,(theApp.webserver->IsRunning())?GetResString(IDS_ENABLED):GetResString(IDS_DISABLED));
	if (theApp.glob_prefs->GetWSIsEnabled()){
		item=MyInfoList->InsertItem(12 ,"");
		CString count;
		count.Format("%i %s",theApp.webserver->GetSessionCount(),GetResString(IDS_ACTSESSIONS));
		MyInfoList->SetItemText(item,1,count);
	}

	//MORPH START - Added by IceCream, Mighty Knife: display complete userhash in status window
	item=MyInfoList->InsertItem(12,GetResString(IDS_CD_UHASH));
	buffer.Format("%s",(LPCTSTR)(md4str((uchar*)theApp.glob_prefs->GetUserHash())));
	MyInfoList->SetItemText(item, 1, buffer.Left (16)+"-");

	item=MyInfoList->InsertItem(7,"");
	MyInfoList->SetItemText(item, 1, buffer.Mid (16,255));
	//MORPH END   - Added by IceCream, [end] Mighty Knife
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
	return m_pacServerMetURL->SaveList(CString(theApp.glob_prefs->GetConfigDir()) + _T("\\") SERVERMET_STRINGS_PROFILE);
}

HBRUSH CServerWnd::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	if (nCtlColor == CTLCOLOR_STATIC && (pWnd->GetDlgCtrlID() == IDC_LOGBOX || pWnd->GetDlgCtrlID() == IDC_DEBUG_LOG))
	{
		// explicitly set the text color -- needed for some contrast window color schemes
		pDC->SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
		pDC->SetBkColor(GetSysColor(COLOR_WINDOW));
		return GetSysColorBrush(COLOR_WINDOW);
	}
	return CResizableDialog::OnCtlColor(pDC, pWnd, nCtlColor);
}

void CServerWnd::ShowServerInfo() {
	CString buffer;

	CServer* cur_server = theApp.serverconnect ? theApp.serverconnect->GetCurrentServer() : NULL;
	CServer* server = cur_server ? theApp.serverlist->GetServerByAddress(cur_server->GetAddress(), cur_server->GetPort()) : NULL;

	if (!theApp.serverconnect->IsConnected() || server==NULL)
		buffer=GetResString(IDS_ERR_NOTCONNECTED);
	else {
		CString buffer2;
		buffer2.Format("%s:\n    %s\n\n",GetResString(IDS_SL_SERVERNAME),server->GetListName());
		buffer.Append(buffer2);

		buffer2.Format("%s:\n    %s\n\n",GetResString(IDS_DESCRIPTION),server->GetDescription());
		buffer.Append(buffer2);

		buffer2.Format("%s:\n    %s\n\n",GetResString(IDS_VERSION),server->GetVersion() );
		buffer.Append(buffer2);

		if (theApp.glob_prefs->IsExtControlsEnabled()){
			buffer2.Format("%s:\n    ", GetResString(IDS_SRV_TCPCOMPR));
			if (server->GetTCPFlags() & SRV_TCPFLG_COMPRESSION)
				buffer2 += GetResString(IDS_YES);
			else
				buffer2 += GetResString(IDS_NO);
			buffer.Append(buffer2 + _T("\n\n"));
		}
		if (theApp.glob_prefs->IsExtControlsEnabled()){
			buffer2.Format("%s:\n    ", GetResString(IDS_SRV_UDPSR));
			if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETSOURCES)
				buffer2 += GetResString(IDS_YES);
			else
				buffer2 += GetResString(IDS_NO);
			buffer.Append(buffer2 + _T("\n\n"));

			buffer2.Format("%s:\n    ", GetResString(IDS_SRV_UDPFR));
			if (server->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)
				buffer2 += GetResString(IDS_YES);
			else
				buffer2 += GetResString(IDS_NO);
			buffer.Append(buffer2 + _T("\n\n"));
		}

		buffer2.Format("%s%s:\n    %s:%u\n\n",GetResString(IDS_CD_UIP),GetResString(IDS_PORT),server->GetFullIP(),server->GetPort() );
		buffer.Append(buffer2);

		buffer2.Format("%s:\n    %u\n\n",GetResString(IDS_PW_FILES),server->GetFiles());
		buffer.Append(buffer2);

		buffer2.Format("%s:\n    %u / %u\n\n",GetResString(IDS_SERVER_LIMITS),server->GetSoftFiles(),server->GetHardFiles());
		buffer.Append(buffer2);

		buffer2.Format("%s:\n    %u / %u\n\n",GetResString(IDS_UUSERS),server->GetUsers(),server->GetMaxUsers());
		buffer.Append(buffer2);

		buffer2.Format("%s:\n    %u ms\n\n",GetResString(IDS_PING),server->GetPing());
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
			strUrl.Format(_T("http://emule-project.net"));
			// MOD Note: end
		}
		ShellExecute(NULL, NULL, strUrl, NULL, NULL, SW_SHOWDEFAULT);
		*pResult = 1;
	}
}

void CServerWnd::UpdateControlsState() {
	if( theApp.serverconnect->IsConnected() )
	{
		GetDlgItem(IDC_ED2KCONNECT)->SetWindowText( GetResString(IDS_MAIN_BTN_DISCONNECT ) );
	}
	else if( theApp.serverconnect->IsConnecting() )
	{
		GetDlgItem(IDC_ED2KCONNECT)->SetWindowText( GetResString(IDS_MAIN_BTN_CANCEL));
	}
	else
	{
		GetDlgItem(IDC_ED2KCONNECT)->SetWindowText( GetResString(IDS_MAIN_BTN_CONNECT ) );
	}
}

void CServerWnd::OnBnConnect() {
	if (theApp.serverconnect->IsConnected())
		theApp.serverconnect->Disconnect();
	else
		theApp.serverconnect->ConnectToAnyServer();
}
