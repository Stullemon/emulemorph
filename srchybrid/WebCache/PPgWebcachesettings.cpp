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
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
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
	ON_BN_CLICKED(IDC_STATIC_CONTROLS, OnBnClickedAdvancedcontrols)
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
	CRect rect;
	GetDlgItem(IDC_WEBCACHELINK)->GetWindowRect(rect);
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rect, 2);
	m_wndSubmitWebcacheLink.CreateEx(NULL,0,_T("MsgWnd"),WS_BORDER | WS_VISIBLE | WS_CHILD | HTC_WORDWRAP | HTC_UNDERLINE_HOVER,rect.left,rect.top,rect.Width(),rect.Height(),m_hWnd,0);
	m_wndSubmitWebcacheLink.SetBkColor(::GetSysColor(COLOR_3DFACE)); // still not the right color, will fix this later (need to merge the .rc file before it changes ;) )
	m_wndSubmitWebcacheLink.SetFont(GetFont());
	// make the relevant information into strings
	CString proxyname, proxyport, limit, lastresolvedname, blocklimit, timeout, localtraffic, persistentconns;
		proxyname.Format(_T("%s"), thePrefs.webcacheName);
		proxyport.Format(_T("%i"), thePrefs.webcachePort);
		limit.Format(_T("%i"), thePrefs.GetWebCacheBlockLimit());
		lastresolvedname.Format(_T("%s"), thePrefs.GetLastResolvedName());
		if (lastresolvedname == _T("")) 
			lastresolvedname = _T("PLEASE USE 'AUTODETECT WEBCACHE' AT LEAST ONCE AND RESTART EMULE FOR THIS INFORMATION TO BE INCLUDED");
		if (thePrefs.GetWebCacheExtraTimeout())timeout.Format(_T("True"));
		else timeout.Format(_T("False"));
		if (thePrefs.GetWebCacheCachesLocalTraffic()) localtraffic.Format(_T("False"));
		else localtraffic.Format(_T("True"));
		if (thePrefs.PersistentConnectionsForProxyDownloads) persistentconns.Format(_T("True"));
		else persistentconns.Format(_T("False"));
	// create hyperlink string
	CString hyperlink;
	hyperlink = _T("mailto:proxydata@arcor.de?Subject=Submit%20webcache&Body=Server%3A%20")
				+ proxyname
				+ _T("%0D%0APort%3A%20") 
				+ proxyport
				+ _T("%0D%0AISP%20Identifyer%3A%20") 
				+ lastresolvedname
				+ _T("%0D%0ANumber%20of%20blocks%3A%20") 
				+ limit
				+ _T("%0D%0AExtra%20timeout%3A%20") 
				+ timeout
				+ _T("%0D%0ADoesn%27t%20cache%20local%3A%20") 
				+ localtraffic
				+ _T("%0D%0ASupports%20persistent%20connections%20for%20proxy%20downloads%3A%20") 
				+ persistentconns
				+ _T("%0D%0A%0D%0AISP%3A%20PLEASE%20ADD%20YOUR%20ISP%20NAME%20AND%20COUNTRY%20HERE%0D%0ACOMMENTS%3A%20PLEASE%20ADD%20ANY%20COMMENTS%20YOU%20MIGHT%20HAVE%20HERE");
	if (!bCreated){
		bCreated = true;
		m_wndSubmitWebcacheLink.AppendText(_T("email ") + GetResString(IDS_WC_LINK));
		m_wndSubmitWebcacheLink.AppendHyperLink(GetResString(IDS_WC_SUBMIT_EMAIL),0,hyperlink,0,0);
	}

// Create website-proxy-submission link
	CRect rect2;
	GetDlgItem(IDC_WEBCACHELINK2)->GetWindowRect(rect2);
	::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rect2, 2);
	m_wndSubmitWebcacheLink2.CreateEx(NULL,0,_T("MsgWnd"),WS_BORDER | WS_VISIBLE | WS_CHILD | HTC_WORDWRAP | HTC_UNDERLINE_HOVER,rect2.left,rect2.top,rect2.Width(),rect2.Height(),m_hWnd,0);
	m_wndSubmitWebcacheLink2.SetBkColor(::GetSysColor(COLOR_3DFACE)); // still not the right color, will fix this later (need to merge the .rc file before it changes ;) )
	m_wndSubmitWebcacheLink2.SetFont(GetFont());
	if (!bCreated2){
		bCreated2 = true;
		m_wndSubmitWebcacheLink2.AppendText(GetResString(IDS_WC_LINK));
		m_wndSubmitWebcacheLink2.AppendHyperLink(GetResString(IDS_WC_SUBMIT_WEB),0,CString(_T("http://ispcachingforemule.de.vu/submitproxy.html")),0,0);
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
	thePrefs.webcacheEnabled = (uint8)IsDlgButtonChecked(IDC_Activatewebcachedownloads);
	
	
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
	thePrefs.SetWebCacheExtraTimeout((uint8)IsDlgButtonChecked(IDC_EXTRATIMEOUT));

	// set thePrefs.WebCacheCachesLocalTraffic
	uint8 cachestraffic;
	cachestraffic = (uint8)IsDlgButtonChecked(IDC_LOCALTRAFFIC);
	if (cachestraffic == 1) thePrefs.SetWebCacheCachesLocalTraffic(0);
	else thePrefs.SetWebCacheCachesLocalTraffic(1);

	// set thePrefs.PersistentConnectionsForProxyDownloads
	thePrefs.PersistentConnectionsForProxyDownloads = ((uint8)IsDlgButtonChecked(IDC_PERSISTENT_PROXY_CONNS));

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
	}
}
void CPPgWebcachesettings::OnBnClickedDetectWebCache()
{
	/*CString temp, WCNametemp, ISPNametemp, WCPorttemp;
	int pos=0;

	try
	{
		temp=DetectWebCache();
	}
	catch(CString strError)
	{
		AfxMessageBox(strError ,MB_OK | MB_ICONINFORMATION,0);
		return;
	}
	catch (...)
	{
		AfxMessageBox(_T("Autodetection failed") ,MB_OK | MB_ICONINFORMATION,0);
		return;
	}

	WCNametemp=temp.Tokenize(_T(":"),pos);
	WCPorttemp=temp.Tokenize(_T(":"),pos);
	ISPNametemp=temp.Tokenize(_T(":"),pos);
 if (AfxMessageBox((_T("Your ISP is:\t\t")+ISPNametemp+_T("\nYour Proxy Name is:\t")+WCNametemp+_T("\nThe Proxy-Port is:\t\t")+WCPorttemp),MB_OKCANCEL | MB_ICONINFORMATION,0) == IDCANCEL) return;
	thePrefs.webcacheName=WCNametemp;
	thePrefs.webcachePort=atoi(WCPorttemp);
	LoadSettings();
	AfxMessageBox(GetResString(IDS_SETTINGCHANGED_RESTART));*/

    //CString WCNametemp, ISPNametemp, WCPorttemp;
	WCInfo_Struct* detectedWebcache = new WCInfo_Struct();
	//int pos=0;
	bool reaskedDNS;	// tells if a DNS backward lookup has been performed during detection

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

	if (AfxMessageBox((_T("Your ISP is:\t\t") + detectedWebcache->isp + _T("\n") +
		_T("Your proxy name is:\t") + detectedWebcache->webcache + _T("\n") +
		_T("The proxy port is:\t\t") + detectedWebcache->port + _T("\n\n") +
		_T("The block limit is:\t\t") + detectedWebcache->blockLimit + _T("\n") +
		_T("extra timeout needed:\t") + detectedWebcache->extraTimeout + _T("\n") +
		_T("caches local traffic:\t\t") + detectedWebcache->cachesLocal + _T("\n") +
		_T("Use persistent connections:\t") + detectedWebcache->persistentconns + _T("\n\n") +
		_T("reverse DNS lookup performed:\t") + (reaskedDNS?_T("yes"):_T("no"))
		),MB_OKCANCEL | MB_ICONINFORMATION,0) == IDCANCEL)
	{
		delete detectedWebcache;
		return;
	}

	thePrefs.webcacheName=detectedWebcache->webcache;
	thePrefs.webcachePort=(uint16)_tstol(detectedWebcache->port);
	thePrefs.SetWebCacheBlockLimit((uint16)_tstol(detectedWebcache->blockLimit));
	thePrefs.SetWebCacheExtraTimeout(detectedWebcache->extraTimeout == _T("yes") ? true : false);
	thePrefs.SetWebCacheCachesLocalTraffic(detectedWebcache->cachesLocal == _T("yes") ? true : false);
	thePrefs.PersistentConnectionsForProxyDownloads = (detectedWebcache->persistentconns == _T("yes") ? true : false);
	delete detectedWebcache;
	LoadSettings();
	AfxMessageBox(GetResString(IDS_SETTINGCHANGED_RESTART));

}


BOOL CPPgWebcachesettings::OnCommand(WPARAM wParam, LPARAM lParam)
{
	/*if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}*/
	return __super::OnCommand(wParam, lParam);
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
		AfxMessageBox(_T("No Test Performed. Not all requirements met.\n Requirements:\n1. You have to be connected to a server\n2. You need to have a valid public IP\n3. You need to have a high ID"));
}
