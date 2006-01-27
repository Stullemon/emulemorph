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
#include <string.h>
#include "emule.h"
#include "PPgWebcachesettings.h"
#include "wizard.h"
#include "Scheduler.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "Preferences.h"
#include "Opcodes.h"
#include "StatisticsDlg.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "HelpIDs.h"
#include "Statistics.h"
#include "webcache.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgWebcachesettings, CPropertyPage)
CPPgWebcachesettings::CPPgWebcachesettings()
	: CPropertyPage(CPPgWebcachesettings::IDD)
{
	guardian=false;
	bCreated = false;
	bCreated2 = false;
	showadvanced = false;
}

CPPgWebcachesettings::~CPPgWebcachesettings()
{
}

void CPPgWebcachesettings::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPPgWebcachesettings, CPropertyPage)
	ON_EN_CHANGE(IDC_webcacheName, OnSettingsChange) 
	ON_EN_CHANGE(IDC_webcachePort, OnSettingsChange)
	ON_EN_CHANGE(IDC_BLOCKS, OnSettingsChange)
	ON_BN_CLICKED(IDC_Activatewebcachedownloads, OnEnChangeActivatewebcachedownloads)
	ON_BN_CLICKED(IDC_DETECTWEBCACHE, OnBnClickedDetectWebCache)
	ON_BN_CLICKED(IDC_EXTRATIMEOUT, OnSettingsChange)
	ON_BN_CLICKED(IDC_LOCALTRAFFIC, OnSettingsChange)
	ON_BN_CLICKED(IDC_PERSISTENT_PROXY_CONNS, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPDATE_WCSETTINGS, OnSettingsChange)
	ON_BN_CLICKED(IDC_ADVANCEDCONTROLS, OnBnClickedAdvancedcontrols)
	ON_BN_CLICKED(IDC_TestProxy, OnBnClickedTestProxy)//JP TMP
	ON_WM_HSCROLL()
	ON_WM_HELPINFO()
END_MESSAGE_MAP()


// CPPgWebcachesettings message handlers

BOOL CPPgWebcachesettings::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	LoadSettings();
	Localize();

	// create hyperlinks 	
	// note: there are better classes to create a pure hyperlink, however since it is only needed here
	//		 we rather use an already existing class

	// Create email-proxy-submission link. The current settings will be included in the email.
	// Superlexx - email proxy submissions are not available anymore, use the web form

// Create website-proxy-submission link
	CRect rect2;
	GetDlgItem(IDC_WEBCACHELINK2)->GetWindowRect(rect2);
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rect2, 2);
	m_wndSubmitWebcacheLink2.CreateEx(NULL,0,_T("MsgWnd"),WS_BORDER | WS_VISIBLE | WS_CHILD | HTC_WORDWRAP | HTC_UNDERLINE_HOVER,rect2.left,rect2.top,rect2.Width(),rect2.Height(),m_hWnd,0);
	m_wndSubmitWebcacheLink2.SetBkColor(::GetSysColor(COLOR_3DFACE)); // still not the right color, will fix this later (need to merge the .rc file before it changes ;) )
	m_wndSubmitWebcacheLink2.SetFont(GetFont());
	if (!bCreated2){
		bCreated2 = true;
		CString URL = _T("http://ispcachingforemule.de.vu/index.php?show=submitProxy");
		CString proxyName, proxyPort, hostName;
		proxyName.Format(_T("%s"), thePrefs.webcacheName);
		proxyPort.Format(_T("%i"), thePrefs.webcachePort);
		hostName.Format(_T("%s"), thePrefs.GetLastResolvedName());

		URL += _T("&hostName=") + hostName + _T("&proxyName=") + proxyName + _T("&proxyPort=") + proxyPort;
		m_wndSubmitWebcacheLink2.AppendText(GetResString(IDS_WC_LINK));
		m_wndSubmitWebcacheLink2.AppendHyperLink(GetResString(IDS_WC_SUBMIT_WEB),0,URL,0,0);
	}

	//JP hide advanced settings
	if (showadvanced)
	{
		GetDlgItem(IDC_ADVANCEDCONTROLS)->SetWindowText(GetResString(IDS_WC_HIDE_ADV));
		GetDlgItem(IDC_EXTRATIMEOUT)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_LOCALTRAFFIC)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BLOCKS)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_NRBLOCKS)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATIC_BLOCKS)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_PERSISTENT_PROXY_CONNS)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_UPDATE_WCSETTINGS)->ShowWindow(SW_SHOW);
	}
	else
	{
		GetDlgItem(IDC_ADVANCEDCONTROLS)->SetWindowText(GetResString(IDS_WC_ADVANCED));
	GetDlgItem(IDC_EXTRATIMEOUT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_LOCALTRAFFIC)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BLOCKS)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_NRBLOCKS)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_STATIC_BLOCKS)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_PERSISTENT_PROXY_CONNS)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_UPDATE_WCSETTINGS)->ShowWindow(SW_HIDE);
	}


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
void CPPgWebcachesettings::OnEnChangeActivatewebcachedownloads(){
		
		if (guardian) return;

		guardian=true;

		SetModified();
		GetDlgItem(IDC_webcacheName)->EnableWindow(IsDlgButtonChecked(IDC_Activatewebcachedownloads));  //disable all other windows if webcachedownload disabled
		GetDlgItem(IDC_webcachePort)->EnableWindow(IsDlgButtonChecked(IDC_Activatewebcachedownloads));
		GetDlgItem(IDC_DETECTWEBCACHE)->EnableWindow(IsDlgButtonChecked(IDC_Activatewebcachedownloads));
		GetDlgItem(IDC_BLOCKS)->EnableWindow(IsDlgButtonChecked(IDC_Activatewebcachedownloads));
		GetDlgItem(IDC_EXTRATIMEOUT)->EnableWindow(IsDlgButtonChecked(IDC_Activatewebcachedownloads));
		GetDlgItem(IDC_LOCALTRAFFIC)->EnableWindow(IsDlgButtonChecked(IDC_Activatewebcachedownloads));
		GetDlgItem(IDC_PERSISTENT_PROXY_CONNS)->EnableWindow(IsDlgButtonChecked(IDC_Activatewebcachedownloads));
		GetDlgItem(IDC_UPDATE_WCSETTINGS)->EnableWindow(IsDlgButtonChecked(IDC_Activatewebcachedownloads));
		GetDlgItem(IDC_ADVANCEDCONTROLS)->EnableWindow(IsDlgButtonChecked(IDC_Activatewebcachedownloads));
		guardian=false;
}

void CPPgWebcachesettings::LoadSettings(void)
{
	if(m_hWnd)
	{
		CString strBuffer;
		if (!(thePrefs.UsesCachedTCPPort()))	// if the user doesn't use a cacheable port, disable everything and disable webcachedownload
		{
			// remove check from DialogeBox
			CheckDlgButton(IDC_Activatewebcachedownloads,(false));
			GetDlgItem(IDC_Activatewebcachedownloads)->EnableWindow(false);
			
			// enter name in webcachename and disable window
			strBuffer.Format(_T("%s"), thePrefs.webcacheName);
			GetDlgItem(IDC_webcacheName)->SetWindowText(strBuffer);
			GetDlgItem(IDC_webcacheName)->EnableWindow(false);

			// enter port in webcacheport and disable window
			strBuffer.Format(_T("%d"), thePrefs.webcachePort);
			GetDlgItem(IDC_webcachePort)->SetWindowText(strBuffer);
			GetDlgItem(IDC_webcachePort)->EnableWindow(false);

			// display wrong port warning
			GetDlgItem(IDC_WrongPortWarning)->ShowWindow(SW_SHOW);
			thePrefs.webcacheEnabled=false;

			// disable autodetection window
			GetDlgItem(IDC_DETECTWEBCACHE)->EnableWindow(false);

			// disable reconnect settings
			GetDlgItem(IDC_BLOCKS)->EnableWindow(false);

			// disable extratimeout
			GetDlgItem(IDC_EXTRATIMEOUT)->EnableWindow(false);

			// disable localtraffic
			GetDlgItem(IDC_LOCALTRAFFIC)->EnableWindow(false);

			// disable persistentproxyconns
			GetDlgItem(IDC_PERSISTENT_PROXY_CONNS)->EnableWindow(false);

			// disable advanced settings button
			GetDlgItem(IDC_ADVANCEDCONTROLS)->EnableWindow(false);

			// disable WC autoupdate
			GetDlgItem(IDC_UPDATE_WCSETTINGS)->EnableWindow(false);

			return;
		}

		// check/uncheck webcache
		GetDlgItem(IDC_Activatewebcachedownloads)->EnableWindow(true);
		CheckDlgButton(IDC_Activatewebcachedownloads,(thePrefs.webcacheEnabled==true));
		
		// enable webcacheName box and enter name 
		strBuffer.Format(_T("%s"), thePrefs.webcacheName);
		GetDlgItem(IDC_webcacheName)->EnableWindow(thePrefs.webcacheEnabled==true);
		GetDlgItem(IDC_webcacheName)->SetWindowText(strBuffer);

		// enable webcachePort box and enter Port
		strBuffer.Format(_T("%d"), thePrefs.webcachePort);
		GetDlgItem(IDC_webcachePort)->EnableWindow(thePrefs.webcacheEnabled==true);
		GetDlgItem(IDC_webcachePort)->SetWindowText(strBuffer);

		// hide wrong port warning
		GetDlgItem(IDC_WrongPortWarning)->ShowWindow(SW_HIDE);

		// enable autodetection
		GetDlgItem(IDC_DETECTWEBCACHE)->EnableWindow(thePrefs.webcacheEnabled==true);

		// load parts to download before reconnect
		strBuffer.Format(_T("%d"), thePrefs.GetWebCacheBlockLimit());
		GetDlgItem(IDC_BLOCKS)->EnableWindow(thePrefs.webcacheEnabled==true);
		GetDlgItem(IDC_BLOCKS)->SetWindowText(strBuffer);

		// load extratimeoutsetting
		GetDlgItem(IDC_EXTRATIMEOUT)->EnableWindow(thePrefs.webcacheEnabled==true);
		CheckDlgButton(IDC_EXTRATIMEOUT,(thePrefs.GetWebCacheExtraTimeout()==true));

		// load localtrafficsettings
		GetDlgItem(IDC_LOCALTRAFFIC)->EnableWindow(thePrefs.webcacheEnabled==true);
		CheckDlgButton(IDC_LOCALTRAFFIC,(thePrefs.GetWebCacheCachesLocalTraffic()==false));

		// load persistent proxy conns
		GetDlgItem(IDC_PERSISTENT_PROXY_CONNS)->EnableWindow(thePrefs.webcacheEnabled==true);
		CheckDlgButton(IDC_PERSISTENT_PROXY_CONNS,(thePrefs.PersistentConnectionsForProxyDownloads==true));

		// show advanced settings button
		GetDlgItem(IDC_ADVANCEDCONTROLS)->EnableWindow(thePrefs.webcacheEnabled==true);

		// load autoupdate
		GetDlgItem(IDC_UPDATE_WCSETTINGS)->EnableWindow(thePrefs.webcacheEnabled==true);
		CheckDlgButton(IDC_UPDATE_WCSETTINGS,(thePrefs.WCAutoupdate==true));

	}
}

BOOL CPPgWebcachesettings::OnApply()
{
	bool bRestartApp = false;
	CString buffer;

	// set thePrefs.webcacheName
	if(GetDlgItem(IDC_webcacheName)->GetWindowTextLength())
	{
		CString nNewwebcache;
		GetDlgItem(IDC_webcacheName)->GetWindowText(nNewwebcache);
		if (thePrefs.webcacheName != nNewwebcache){
			thePrefs.webcacheName = nNewwebcache;
			bRestartApp = true;
		}
	}

	// set thePrefs.webcachePort
	if(GetDlgItem(IDC_webcachePort)->GetWindowTextLength())
	{
		GetDlgItem(IDC_webcachePort)->GetWindowText(buffer);
		uint16 nNewPort = (uint16)_tstol(buffer);
		if (!nNewPort) nNewPort=0;
		if (nNewPort != thePrefs.webcachePort){
			thePrefs.webcachePort = nNewPort;
		}
	}
	
	// set thePrefs.webcacheEnabled
	thePrefs.webcacheEnabled = IsDlgButtonChecked(IDC_Activatewebcachedownloads)!=0;
	
	
	// set thePrefs.webcacheBlockLimit
	if(GetDlgItem(IDC_BLOCKS)->GetWindowTextLength())
	{
		GetDlgItem(IDC_BLOCKS)->GetWindowText(buffer);
		uint16 nNewBlocks = (uint16)_tstol(buffer);
		if ((!nNewBlocks) || (nNewBlocks > 50000) || (nNewBlocks < 0)) nNewBlocks=0;
		if (nNewBlocks != thePrefs.GetWebCacheBlockLimit()){
			thePrefs.SetWebCacheBlockLimit(nNewBlocks);
		}
	}
	
	// set thePrefs.WebCacheExtraTimeout
	thePrefs.SetWebCacheExtraTimeout(IsDlgButtonChecked(IDC_EXTRATIMEOUT)!=0);

	// set thePrefs.WebCacheCachesLocalTraffic
	uint8 cachestraffic;
	cachestraffic = (uint8)IsDlgButtonChecked(IDC_LOCALTRAFFIC);
	if (cachestraffic == 1) thePrefs.SetWebCacheCachesLocalTraffic(0);
	else thePrefs.SetWebCacheCachesLocalTraffic(1);

	// set thePrefs.PersistentConnectionsForProxyDownloads
	thePrefs.PersistentConnectionsForProxyDownloads = IsDlgButtonChecked(IDC_PERSISTENT_PROXY_CONNS)!=0;

	// set thePrefs.WCAutoupdate
	thePrefs.WCAutoupdate = IsDlgButtonChecked(IDC_UPDATE_WCSETTINGS)!=0;

	SetModified(FALSE);
	LoadSettings();

	if (bRestartApp)
	{
		AfxMessageBox(GetResString(IDS_SETTINGCHANGED_RESTART));
		thePrefs.WebCacheDisabledThisSession = true;
	}
	return CPropertyPage::OnApply();
}

void CPPgWebcachesettings::Localize(void)
{	
	//if(m_hWnd)
	//{
	//	SetWindowText(GetResString(IDS_PW_CONNECTION));
		
		//JP just an example of how it is done GetDlgItem(IDC_CAPACITIES_FRM)->SetWindowText(GetResString(IDS_PW_CON_CAPFRM));

	//}
	if (m_hWnd){
		GetDlgItem(IDC_Webcache)->SetWindowText( GetResString(IDS_WEBCACHE_ISP) );
		GetDlgItem(IDC_STATIC_PORT)->SetWindowText( GetResString(IDS_WC_PORT) );
		GetDlgItem(IDC_STATIC_ADDRESS)->SetWindowText( GetResString(IDS_WC_ADDRESS) );
		GetDlgItem(IDC_STATIC_CONTROLS)->SetWindowText( GetResString(IDS_WC_CONTROLS) );
		GetDlgItem(IDC_Activatewebcachedownloads)->SetWindowText( GetResString(IDS_WC_ENABLE) );
		GetDlgItem(IDC_WrongPortWarning)->SetWindowText( GetResString(IDS_WC_WRONGPORT) );
		GetDlgItem(IDC_DETECTWEBCACHE)->SetWindowText( GetResString(IDS_WC_AUTO) );
		GetDlgItem(IDC_STATIC_NRBLOCKS)->SetWindowText( GetResString(IDS_WC_NRBLOCKS) );
		GetDlgItem(IDC_STATIC_BLOCKS)->SetWindowText( GetResString(IDS_WC_BLOCK) );
		GetDlgItem(IDC_EXTRATIMEOUT)->SetWindowText( GetResString(IDS_WC_TIMEOUT) );
		GetDlgItem(IDC_LOCALTRAFFIC)->SetWindowText( GetResString(IDS_WC_LOCAL) );
		GetDlgItem(IDC_PERSISTENT_PROXY_CONNS)->SetWindowText( GetResString(IDS_WC_PERSISTENT) );
		GetDlgItem(IDC_ADVANCEDCONTROLS)->SetWindowText( GetResString(IDS_WC_ADVANCED) );
		GetDlgItem(IDC_TestProxy)->SetWindowText( GetResString(IDS_WC_TEST) );
		GetDlgItem(IDC_UPDATE_WCSETTINGS)->SetWindowText( GetResString(IDS_WC_UPDATESETTING) );
	}
}
void CPPgWebcachesettings::OnBnClickedDetectWebCache()
{

	WCInfo_Struct* detectedWebcache = new WCInfo_Struct();
	bool reaskedDNS;	// tells if a DNS reverse lookup has been performed during detection; unneeded since we don't show it anymore

	try
	{
		reaskedDNS=DetectWebCache(detectedWebcache);
	}
	catch(CString strError)
	{
		delete detectedWebcache;
		AfxMessageBox(strError ,MB_OK | MB_ICONINFORMATION,0);
		return;
	}
	catch (...)
	{
		delete detectedWebcache;
		AfxMessageBox(_T("Autodetection failed") ,MB_OK | MB_ICONINFORMATION,0);
		return;
	}

	CString comment = detectedWebcache->comment;
	for (int i=1; i*45 < comment.GetLength(); i++) // some quick-n-dirty beautifying  
		comment = comment.Left(i*45) + _T(" \n\t\t\t") + comment.Right(comment.GetLength() - i*45);

	CString message =	_T("Your ISP is:\t\t") + detectedWebcache->isp + _T(", ") + detectedWebcache->country + _T("\n") +
		_T("Your proxy name is:\t") + detectedWebcache->webcache + _T("\n") +
						_T("The proxy port is:\t\t") + detectedWebcache->port + _T("\n") +
						(comment != _T("") ? _T("comment: \t\t") + comment : _T(""));
	if (detectedWebcache->active == "0")
		message += _T("\n\ndue to detection results, webcache downloading has been deactivated;\nsee the comment for more details");

	if (AfxMessageBox(message, MB_OKCANCEL | MB_ICONINFORMATION,0) == IDCANCEL)
	{
		delete detectedWebcache;
		return;
	}

	CheckDlgButton(IDC_Activatewebcachedownloads, detectedWebcache->active == "1" ? BST_CHECKED : BST_UNCHECKED);
	GetDlgItem(IDC_webcacheName)->SetWindowText(detectedWebcache->webcache);
	GetDlgItem(IDC_webcachePort)->SetWindowText(detectedWebcache->port);
	GetDlgItem(IDC_BLOCKS)->SetWindowText(detectedWebcache->blockLimit);
	CheckDlgButton(IDC_EXTRATIMEOUT, detectedWebcache->extraTimeout == "1" ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_LOCALTRAFFIC, detectedWebcache->cachesLocal == "0" ? BST_CHECKED : BST_UNCHECKED);
	CheckDlgButton(IDC_PERSISTENT_PROXY_CONNS, detectedWebcache->persistentconns == "1" ? BST_CHECKED : BST_UNCHECKED);

	delete detectedWebcache;
}

void CPPgWebcachesettings::OnHelp()
{
	//theApp.ShowHelp(0);
}

BOOL CPPgWebcachesettings::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgWebcachesettings::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}

void CPPgWebcachesettings::OnBnClickedAdvancedcontrols(){

showadvanced = !showadvanced;
if (showadvanced)
{
	GetDlgItem(IDC_ADVANCEDCONTROLS)->SetWindowText(GetResString(IDS_WC_HIDE_ADV));
	GetDlgItem(IDC_EXTRATIMEOUT)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_LOCALTRAFFIC)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_BLOCKS)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_STATIC_NRBLOCKS)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_STATIC_BLOCKS)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_PERSISTENT_PROXY_CONNS)->ShowWindow(SW_SHOW);
	GetDlgItem(IDC_UPDATE_WCSETTINGS)->ShowWindow(SW_SHOW);
}
else
{
	GetDlgItem(IDC_ADVANCEDCONTROLS)->SetWindowText(GetResString(IDS_WC_ADVANCED));
	GetDlgItem(IDC_EXTRATIMEOUT)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_LOCALTRAFFIC)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BLOCKS)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_NRBLOCKS)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_STATIC_BLOCKS)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_PERSISTENT_PROXY_CONNS)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_UPDATE_WCSETTINGS)->ShowWindow(SW_HIDE);
}
}
//JP proxy configuration test
void CPPgWebcachesettings::OnBnClickedTestProxy()
{
	if (thePrefs.IsWebCacheTestPossible())
	{
		if (!thePrefs.expectingWebCachePing)
		{
			//get webcache name from IDC_webcacheName
			CString cur_WebCacheName;
			GetDlgItem(IDC_webcacheName)->GetWindowText(cur_WebCacheName);
			if (cur_WebCacheName.GetLength() > 15 && cur_WebCacheName.Left(12) == "transparent@") //doesn't work for transparent proxies
			{
				AfxMessageBox(_T("Proxy Test can not test Transparent proxies. Test Canceled!"));
				return;
			}
			//get webcache port from IDC_webcachePort
			CString buffer;			
			GetDlgItem(IDC_webcachePort)->GetWindowText(buffer);
			uint16 cur_WebCachePort = (uint16)_tstol(buffer);
			if (PingviaProxy(cur_WebCacheName, cur_WebCachePort))
			{
				thePrefs.WebCachePingSendTime = ::GetTickCount();
				thePrefs.expectingWebCachePing = true;
				AfxMessageBox(_T("Performing Proxy Test! Please check the log in the serverwindow for the results!"));
			}
			else
				AfxMessageBox(_T("Proxy Test Error!"));
		}
		else 
			AfxMessageBox(_T("No New Test Started. There is already a Test in progress"));
	}
	else
		AfxMessageBox(_T("No Test Performed. Not all requirements met.\n Requirements:\n1. You have to be connected to a server\n2. You need to have a valid public IP\n3. You need to have a high ID\n4. Test does not work if you have too many open connections"));
}
