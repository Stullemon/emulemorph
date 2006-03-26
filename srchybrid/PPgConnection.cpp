//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "emule.h"
#include "PPGtooltipped.h" //MORPH leuk_he addded tooltipped
#include "PPgConnection.h"
//MORPH START - Added by SiRoB, WebCache 1.2f
#include "WebCache\PPgWebcachesettings.h" //jp
#include "PreferencesDlg.h" //jp
#include "emuleDlg.h" // webcache
//MORPH END   - Added by SiRoB, WebCache 1.2f
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
#include "Firewallopener.h"
#include "ListenSocket.h"
#include "ClientUDPSocket.h"
#include "LastCommonRouteFinder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgConnection, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgConnection, CPropertyPage)
	ON_BN_CLICKED(IDC_STARTTEST, OnStartPortTest)
	ON_EN_CHANGE(IDC_DOWNLOAD_CAP, OnSettingsChange)
	ON_BN_CLICKED(IDC_UDPDISABLE, OnEnChangeUDPDisable)
	ON_EN_CHANGE(IDC_UDPPORT, OnEnChangeUDP)
	ON_EN_CHANGE(IDC_UPLOAD_CAP, OnSettingsChange)
	ON_EN_CHANGE(IDC_PORT, OnEnChangeTCP)
	ON_EN_CHANGE(IDC_MAXCON, OnSettingsChange)
	ON_EN_CHANGE(IDC_MAXSOURCEPERFILE, OnSettingsChange)
	ON_BN_CLICKED(IDC_AUTOCONNECT, OnSettingsChange)
	ON_BN_CLICKED(IDC_RECONN, OnSettingsChange)
	ON_BN_CLICKED(IDC_WIZARD, OnBnClickedWizard)
	ON_BN_CLICKED(IDC_NETWORK_ED2K, OnSettingsChange)
	ON_BN_CLICKED(IDC_SHOWOVERHEAD, OnSettingsChange)
	ON_BN_CLICKED(IDC_ULIMIT_LBL, OnLimiterChange)
	ON_BN_CLICKED(IDC_DLIMIT_LBL, OnLimiterChange)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_NETWORK_KADEMLIA, OnSettingsChange)
	ON_WM_HELPINFO()
	ON_BN_CLICKED(IDC_OPENPORTS, OnBnClickedOpenports)
	//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-]
	ON_BN_CLICKED(IDC_RANDOMPORTS, OnRandomPortsChange)
	//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-]
END_MESSAGE_MAP()


CPPgConnection::CPPgConnection()
// MORPRH START leuk_he tooltipped
/*
   : CPropertyPage(CPPgConnection::IDD)
*/
	: CPPgtooltipped(CPPgConnection::IDD)
// MORPRH END leuk_he tooltipped
{
	guardian=false;
}

CPPgConnection::~CPPgConnection()
{
}

void CPPgConnection::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MAXDOWN_SLIDER, m_ctlMaxDown);
	DDX_Control(pDX, IDC_MAXUP_SLIDER, m_ctlMaxUp);
	//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-]
	CString text;

	DDX_Control(pDX, IDC_MINPORT, m_minRndPort);
	m_minRndPort.GetWindowText(text);
	DDV_MinMaxInt(pDX,_ttoi(text),1,0xFFFF);

	DDX_Control(pDX, IDC_MAXPORT, m_maxRndPort);
	m_maxRndPort.GetWindowText(text);
	DDV_MinMaxInt(pDX,_ttoi(text),1,0xFFFF);

	DDX_Control(pDX, IDC_SPIN_MIN, m_minRndPortSpin);
	DDX_Control(pDX, IDC_SPIN_MAX, m_maxRndPortSpin);
	//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-]
}

void CPPgConnection::OnEnChangeTCP()
{
	OnEnChangePorts(true);
}

void CPPgConnection::OnEnChangeUDP()
{
	OnEnChangePorts(false);
}

void CPPgConnection::OnEnChangePorts(uint8 istcpport)
{
	// ports unchanged?
	CString buffer;
	GetDlgItem(IDC_PORT)->GetWindowText(buffer);
	uint16 tcp = (uint16)_tstoi(buffer);
	GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer);
	uint16 udp = (uint16)_tstoi(buffer);

	GetDlgItem(IDC_STARTTEST)->EnableWindow( 
		tcp==theApp.listensocket->GetConnectedPort() && 
		udp==theApp.clientudp->GetConnectedPort() 
	);

	if (istcpport==0)
		OnEnChangeUDPDisable();
	else if (istcpport==1)
		OnSettingsChange();
}

void CPPgConnection::OnEnChangeUDPDisable()
{
	if (guardian)
		return;

		uint16 tempVal=0;
	CString strBuffer;
	TCHAR buffer[510];

	guardian=true;
	SetModified();

	//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-]
	/*
	GetDlgItem(IDC_UDPPORT)->EnableWindow(!IsDlgButtonChecked(IDC_UDPDISABLE));
	*/
	GetDlgItem(IDC_UDPPORT)->EnableWindow(!IsDlgButtonChecked(IDC_UDPDISABLE) && !IsDlgButtonChecked(IDC_RANDOMPORTS));
	//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-]

	if(GetDlgItem(IDC_UDPPORT)->GetWindowTextLength())
	{
		GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer,20);
		tempVal = (uint16)_tstoi(buffer);
	}

	if (IsDlgButtonChecked(IDC_UDPDISABLE) || (!IsDlgButtonChecked(IDC_UDPDISABLE) && tempVal == 0))
	{
		tempVal = (uint16)_tstoi(buffer) ? (uint16)(_tstoi(buffer)+10) : (uint16)(thePrefs.port+10);
		if ( IsDlgButtonChecked(IDC_UDPDISABLE))
			tempVal=0;
		strBuffer.Format(_T("%d"), tempVal);
		GetDlgItem(IDC_UDPPORT)->SetWindowText(strBuffer);
	}

	guardian=false;
}

BOOL CPPgConnection::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-]
	m_minRndPortSpin.SetBuddy(&m_minRndPort);
	m_minRndPortSpin.SetRange32(1,0xffff);
	m_maxRndPortSpin.SetBuddy(&m_maxRndPort);
	m_maxRndPortSpin.SetRange32(1,0xffff);
	//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-]

	LoadSettings();
	InitTooltips(); // MORPH leuk_he tooltipped
	Localize();

	OnEnChangePorts(2);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgConnection::LoadSettings(void)
{
	if(m_hWnd)
	{
		if( thePrefs.maxupload != 0 )
			thePrefs.maxdownload =thePrefs.GetMaxDownload();

		CString strBuffer;
		
		strBuffer.Format(_T("%d"), thePrefs.udpport);
		GetDlgItem(IDC_UDPPORT)->SetWindowText(strBuffer);
		CheckDlgButton(IDC_UDPDISABLE,(thePrefs.udpport==0));

		//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-]
		/*		
		GetDlgItem(IDC_UDPPORT)->EnableWindow(thePrefs.udpport>0);
		*/
		//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-]
	
		strBuffer.Format(_T("%d"), thePrefs.maxGraphDownloadRate);
		GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(strBuffer);

		m_ctlMaxDown.SetRange(1, thePrefs.maxGraphDownloadRate);
		SetRateSliderTicks(m_ctlMaxDown);

		if (thePrefs.maxGraphUploadRate != UNLIMITED)
		strBuffer.Format(_T("%d"), thePrefs.maxGraphUploadRate);
		else
			strBuffer = _T("0");
		GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(strBuffer);

		m_ctlMaxUp.SetRange(1, thePrefs.GetMaxGraphUploadRate(true));
		SetRateSliderTicks(m_ctlMaxUp);

		CheckDlgButton( IDC_DLIMIT_LBL, (thePrefs.maxdownload!=UNLIMITED) );
		CheckDlgButton( IDC_ULIMIT_LBL, (thePrefs.maxupload!=UNLIMITED) );

		m_ctlMaxDown.SetPos((thePrefs.maxdownload != UNLIMITED) ? thePrefs.maxdownload : thePrefs.maxGraphDownloadRate);
		m_ctlMaxUp.SetPos((thePrefs.maxupload != UNLIMITED) ? thePrefs.maxupload : thePrefs.GetMaxGraphUploadRate(true));

		strBuffer.Format(_T("%d"), thePrefs.port);
		GetDlgItem(IDC_PORT)->SetWindowText(strBuffer);

		strBuffer.Format(_T("%d"), thePrefs.maxconnections);
		GetDlgItem(IDC_MAXCON)->SetWindowText(strBuffer);

		if(thePrefs.maxsourceperfile == 0xFFFF)
			GetDlgItem(IDC_MAXSOURCEPERFILE)->SetWindowText(_T("0"));
		else{
			strBuffer.Format(_T("%d"), thePrefs.maxsourceperfile);
			GetDlgItem(IDC_MAXSOURCEPERFILE)->SetWindowText(strBuffer);
		}

		if (thePrefs.reconnect)
			CheckDlgButton(IDC_RECONN,1);
		else
			CheckDlgButton(IDC_RECONN,0);
		
		if (thePrefs.m_bshowoverhead)
			CheckDlgButton(IDC_SHOWOVERHEAD,1);
		else
			CheckDlgButton(IDC_SHOWOVERHEAD,0);

		if (thePrefs.autoconnect)
			CheckDlgButton(IDC_AUTOCONNECT,1);
		else
			CheckDlgButton(IDC_AUTOCONNECT,0);

		if (thePrefs.networkkademlia)
			CheckDlgButton(IDC_NETWORK_KADEMLIA,1);
		else
			CheckDlgButton(IDC_NETWORK_KADEMLIA,0);

		if (thePrefs.networked2k)
			CheckDlgButton(IDC_NETWORK_ED2K,1);
		else
			CheckDlgButton(IDC_NETWORK_ED2K,0);

		// don't try on XP SP2 or higher, not needed there anymore
		if (IsRunningXPSP2() == 0 && theApp.m_pFirewallOpener->DoesFWConnectionExist())
			GetDlgItem(IDC_OPENPORTS)->EnableWindow(true);
		else
			GetDlgItem(IDC_OPENPORTS)->EnableWindow(false);

		//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-]
		CString rndText;

		CheckDlgButton(IDC_RANDOMPORTS,thePrefs.GetUseRandomPorts());
		rndText.Format(_T("%u"), thePrefs.GetMinRandomPort());
		m_minRndPort.SetWindowText(rndText);
		rndText.Format(_T("%u"), thePrefs.GetMaxRandomPort());
		m_maxRndPort.SetWindowText(rndText);

		GetDlgItem(IDC_PORT)->EnableWindow(!thePrefs.GetUseRandomPorts());
		GetDlgItem(IDC_UDPPORT)->EnableWindow(!thePrefs.GetUseRandomPorts() && thePrefs.udpport>0);
		
		m_minRndPort.EnableWindow(thePrefs.GetUseRandomPorts());
		m_maxRndPort.EnableWindow(thePrefs.GetUseRandomPorts());
		m_minRndPortSpin.EnableWindow(thePrefs.GetUseRandomPorts());
		m_maxRndPortSpin.EnableWindow(thePrefs.GetUseRandomPorts());
		//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-]

		ShowLimitValues();
		OnLimiterChange();
	}
}

BOOL CPPgConnection::OnApply()
{
	TCHAR buffer[510];
	int lastmaxgu = thePrefs.maxGraphUploadRate;
	int lastmaxgd = thePrefs.maxGraphDownloadRate;
	bool bRestartApp = false;

	if(GetDlgItem(IDC_DOWNLOAD_CAP)->GetWindowTextLength())
	{ 
		GetDlgItem(IDC_DOWNLOAD_CAP)->GetWindowText(buffer,20);
		thePrefs.SetMaxGraphDownloadRate(_tstoi(buffer));
	}

	m_ctlMaxDown.SetRange(1, thePrefs.GetMaxGraphDownloadRate(), TRUE);
	SetRateSliderTicks(m_ctlMaxDown);

	if(GetDlgItem(IDC_UPLOAD_CAP)->GetWindowTextLength())
	{
		GetDlgItem(IDC_UPLOAD_CAP)->GetWindowText(buffer,20);
		thePrefs.SetMaxGraphUploadRate(_tstoi(buffer));
	}

	m_ctlMaxUp.SetRange(1, thePrefs.GetMaxGraphUploadRate(true), TRUE);
	SetRateSliderTicks(m_ctlMaxUp);

    {
        uint16 ulSpeed;

		if (!IsDlgButtonChecked(IDC_ULIMIT_LBL))
		    ulSpeed = UNLIMITED;
		else
		    ulSpeed = (uint16)m_ctlMaxUp.GetPos();

	    if (thePrefs.GetMaxGraphUploadRate(true) < ulSpeed && ulSpeed != UNLIMITED)
		    ulSpeed = (uint16)(thePrefs.GetMaxGraphUploadRate(true) * 0.8);

        if(ulSpeed > thePrefs.GetMaxUpload()) {
            // make USS go up to higher ul limit faster
            theApp.lastCommonRouteFinder->InitiateFastReactionPeriod();
        }

        thePrefs.SetMaxUpload(ulSpeed);
    }

	if (thePrefs.GetMaxUpload()!=UNLIMITED)
		m_ctlMaxUp.SetPos(thePrefs.GetMaxUpload());
	
	if (!IsDlgButtonChecked(IDC_DLIMIT_LBL))
		thePrefs.SetMaxDownload(UNLIMITED);
	else
		thePrefs.SetMaxDownload(m_ctlMaxDown.GetPos());

	if( thePrefs.GetMaxGraphDownloadRate() < thePrefs.GetMaxDownload() && thePrefs.GetMaxDownload()!=UNLIMITED )
		thePrefs.SetMaxDownload((uint16)(thePrefs.GetMaxGraphDownloadRate() * 0.8));

	if (thePrefs.GetMaxDownload()!=UNLIMITED)
		m_ctlMaxDown.SetPos(thePrefs.GetMaxDownload());

	if(GetDlgItem(IDC_PORT)->GetWindowTextLength())
	{
		GetDlgItem(IDC_PORT)->GetWindowText(buffer,20);
		uint16 nNewPort = ((uint16)_tstoi(buffer)) ? (uint16)_tstoi(buffer) : (uint16)DEFAULT_TCP_PORT;
		if (nNewPort != thePrefs.port){
			thePrefs.port = nNewPort;
			if (theApp.IsPortchangeAllowed())
				theApp.listensocket->Rebind();
			else
				bRestartApp = true;
			//MORPH START - Added by SiRoB, WebCache 1.2f
			// yonatan WC-TODO: check out Rebind()
			// jp webcachesettings
			// this part crashes if Webcachesettings has not been active page at least once see PreferencesDlg.cpp (103)
			if	(((!thePrefs.UsesCachedTCPPort())	// not a good port for webcace
			&& thePrefs.IsWebCacheDownloadEnabled()		// webcache enabled
			&& theApp.emuledlg->preferenceswnd->m_wndWebcachesettings.IsDlgButtonChecked(IDC_Activatewebcachedownloads))  //if webcache was disabled but the change was not saved yet, no need for the message because it will be saved now
			|| (!thePrefs.UsesCachedTCPPort()		// not a good port for webcache
				&& theApp.emuledlg->preferenceswnd->m_wndWebcachesettings.IsDlgButtonChecked(IDC_Activatewebcachedownloads))) //webcache enabled but not yet saved to thePrefs. would be saved now but shouldn't
			{
				AfxMessageBox(GetResString(IDS_WrongPortforWebcache),MB_OK | MB_ICONINFORMATION,0);
				thePrefs.webcacheEnabled=false;			// disable webcache
			}
			
			theApp.emuledlg->preferenceswnd->m_wndWebcachesettings.LoadSettings();
			// jp end
			//MORPH END   - Added by SiRoB, WebCache 1.2f
			//MORPH START - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
			theApp.m_pFirewallOpener->ClearMappingsAtEnd();
			//MORPH END   - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
		}
	}
	
	if(GetDlgItem(IDC_MAXSOURCEPERFILE)->GetWindowTextLength())
	{
		GetDlgItem(IDC_MAXSOURCEPERFILE)->GetWindowText(buffer,20);
		thePrefs.maxsourceperfile = (_tstoi(buffer)) ? _tstoi(buffer) : 1;
	}

	if(GetDlgItem(IDC_UDPPORT)->GetWindowTextLength())
	{
		GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer,20);
		uint16 nNewPort = ((uint16)_tstoi(buffer) && !IsDlgButtonChecked(IDC_UDPDISABLE)) ? (uint16)_tstoi(buffer) : (uint16)0;
		if (nNewPort != thePrefs.udpport){
			thePrefs.udpport = nNewPort;
			if (theApp.IsPortchangeAllowed())
				theApp.clientudp->Rebind();
			else 
				bRestartApp = true;
			//MORPH START - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
			theApp.m_pFirewallOpener->ClearMappingsAtEnd();
			//MORPH START - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
		}
	}

	if(IsDlgButtonChecked(IDC_SHOWOVERHEAD)){
		if (!thePrefs.m_bshowoverhead){
			// reset overhead data counters before starting to meassure!
			theStats.ResetDownDatarateOverhead();
			theStats.ResetUpDatarateOverhead();
		}
		thePrefs.m_bshowoverhead = true;
	}
	else{
		if (thePrefs.m_bshowoverhead){
			// free memory used by overhead computations
			theStats.ResetDownDatarateOverhead();
			theStats.ResetUpDatarateOverhead();
		}
		thePrefs.m_bshowoverhead = false;
	}

	if(IsDlgButtonChecked(IDC_NETWORK_KADEMLIA))
		thePrefs.SetNetworkKademlia(true);
	else
		thePrefs.SetNetworkKademlia(false);

	if(IsDlgButtonChecked(IDC_NETWORK_ED2K))
		thePrefs.SetNetworkED2K(true);
	else
		thePrefs.SetNetworkED2K(false);

	GetDlgItem(IDC_UDPPORT)->EnableWindow(!IsDlgButtonChecked(IDC_UDPDISABLE));

	thePrefs.autoconnect = IsDlgButtonChecked(IDC_AUTOCONNECT)!=0;
	thePrefs.reconnect = IsDlgButtonChecked(IDC_RECONN)!=0;
		
	if(lastmaxgu != thePrefs.maxGraphUploadRate) 
		theApp.emuledlg->statisticswnd->SetARange(false, thePrefs.GetMaxGraphUploadRate(true));
	if(lastmaxgd!=thePrefs.maxGraphDownloadRate)
		theApp.emuledlg->statisticswnd->SetARange(true,thePrefs.maxGraphDownloadRate);

	UINT tempcon = thePrefs.maxconnections;
	if(GetDlgItem(IDC_MAXCON)->GetWindowTextLength())
	{
		GetDlgItem(IDC_MAXCON)->GetWindowText(buffer,20);
		tempcon = (_tstoi(buffer)) ? _tstoi(buffer) : CPreferences::GetRecommendedMaxConnections();
	}

	if(tempcon > (unsigned)::GetMaxWindowsTCPConnections())
	{
		CString strMessage;
		strMessage.Format(GetResString(IDS_PW_WARNING), GetResString(IDS_PW_MAXC), ::GetMaxWindowsTCPConnections());
		int iResult = AfxMessageBox(strMessage, MB_ICONWARNING | MB_YESNO);
		if(iResult != IDYES)
		{
			//TODO: set focus to max connection?
			strMessage.Format(_T("%d"), thePrefs.maxconnections);
			GetDlgItem(IDC_MAXCON)->SetWindowText(strMessage);
			tempcon = ::GetMaxWindowsTCPConnections();
		}
	}
	thePrefs.maxconnections = tempcon;
	theApp.scheduler->SaveOriginals();

	//MORPH START - Added by SiRoB, [MoNKi: [MoNKi: -Random Ports-]
	bool oldUseRandom = thePrefs.GetUseRandomPorts();
	thePrefs.SetUseRandomPorts(IsDlgButtonChecked(IDC_RANDOMPORTS)!=0);

	unsigned long rndValue1, rndValue2, oldRndMin, oldRndMax;
	CString rndText;
	m_minRndPort.GetWindowText(rndText);
	rndValue1 = _tstoi(rndText);
	m_maxRndPort.GetWindowText(rndText);
	rndValue2 = _tstoi(rndText);

	if (rndValue1 > 0xFFFF) rndValue1 = 0xFFFF;
	if (rndValue1 < 1) rndValue1 = 1;
	if (rndValue2 > 0xFFFF) rndValue2 = 0xFFFF;
	if (rndValue2 < 1) rndValue2 = 1;
	if (rndValue1 > rndValue2){
		int tmp = rndValue2;
		rndValue2 = rndValue1;
		rndValue1 = tmp;
	}

	oldRndMin = thePrefs.GetMinRandomPort();
	oldRndMax = thePrefs.GetMaxRandomPort();

	if(rndValue1 != oldRndMin || rndValue2 != oldRndMax){
		thePrefs.SetMinRandomPort((uint16)rndValue1);
		thePrefs.SetMaxRandomPort((uint16)rndValue2);
	}

	if((IsDlgButtonChecked(IDC_RANDOMPORTS)!=0) != oldUseRandom){
		if (theApp.IsPortchangeAllowed()){
			theApp.listensocket->Rebind();
			theApp.clientudp->Rebind();
		}
		else{
			bRestartApp = true;
			thePrefs.SetRandomPortsResetOnRestart(true);
		}
		// Added by MoNKi [MoNKi: -Improved ICS-Firewall support-]
		theApp.m_pFirewallOpener->ClearMappingsAtEnd();
		// End -Improved ICS-Firewall support-
	}
	else if(oldUseRandom){
		if(rndValue1 != oldRndMin || rndValue2 != oldRndMax){
			if (theApp.IsPortchangeAllowed()){
				theApp.listensocket->Rebind();
				theApp.clientudp->Rebind();
			}
			else {
				bRestartApp = true;
				thePrefs.SetRandomPortsResetOnRestart(true);
			}

			// Added by MoNKi [MoNKi: -Improved ICS-Firewall support-]
			theApp.m_pFirewallOpener->ClearMappingsAtEnd();
			// End -Improved ICS-Firewall support-
		}
	}
	// End emulEspa�a

	SetModified(FALSE);
	LoadSettings();

	theApp.emuledlg->ShowConnectionState();

	if (bRestartApp)
		AfxMessageBox(GetResString(IDS_NOPORTCHANGEPOSSIBLE));

	OnEnChangePorts(2);

	return CPropertyPage::OnApply();
}

void CPPgConnection::Localize(void)
{	
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_CONNECTION));
		GetDlgItem(IDC_CAPACITIES_FRM)->SetWindowText(GetResString(IDS_PW_CON_CAPFRM));
		GetDlgItem(IDC_DCAP_LBL)->SetWindowText(GetResString(IDS_PW_CON_DOWNLBL));
		GetDlgItem(IDC_UCAP_LBL)->SetWindowText(GetResString(IDS_PW_CON_UPLBL));
		GetDlgItem(IDC_LIMITS_FRM)->SetWindowText(GetResString(IDS_PW_CON_LIMITFRM));
		GetDlgItem(IDC_DLIMIT_LBL)->SetWindowText(GetResString(IDS_PW_DOWNL));
		GetDlgItem(IDC_ULIMIT_LBL)->SetWindowText(GetResString(IDS_PW_UPL));
		GetDlgItem(IDC_CONNECTION_NETWORK)->SetWindowText(GetResString(IDS_NETWORK));
		GetDlgItem(IDC_KBS2)->SetWindowText(GetResString(IDS_KBYTESPERSEC));
		GetDlgItem(IDC_KBS3)->SetWindowText(GetResString(IDS_KBYTESPERSEC));
		ShowLimitValues();
		GetDlgItem(IDC_MAXCONN_FRM)->SetWindowText(GetResString(IDS_PW_CONLIMITS));
		GetDlgItem(IDC_MAXCONLABEL)->SetWindowText(GetResString(IDS_PW_MAXC));
		GetDlgItem(IDC_SHOWOVERHEAD)->SetWindowText(GetResString(IDS_SHOWOVERHEAD));
		GetDlgItem(IDC_CLIENTPORT_FRM)->SetWindowText(GetResString(IDS_PW_CLIENTPORT));
		GetDlgItem(IDC_MAXSRC_FRM)->SetWindowText(GetResString(IDS_PW_MAXSOURCES));
		GetDlgItem(IDC_AUTOCONNECT)->SetWindowText(GetResString(IDS_PW_AUTOCON));
		GetDlgItem(IDC_RECONN)->SetWindowText(GetResString(IDS_PW_RECON));
		GetDlgItem(IDC_MAXSRCHARD_LBL)->SetWindowText(GetResString(IDS_HARDLIMIT));
		GetDlgItem(IDC_WIZARD)->SetWindowText(GetResString(IDS_WIZARD));
		GetDlgItem(IDC_UDPDISABLE)->SetWindowText(GetResString(IDS_UDPDISABLED));
		GetDlgItem(IDC_OPENPORTS)->SetWindowText(GetResString(IDS_FO_PREFBUTTON));
		SetDlgItemText(IDC_STARTTEST, GetResString(IDS_STARTTEST) );
		//MORPH START - Added by SiRoB, [MoNKi: [MoNKi: -Random Ports-]
		GetDlgItem(IDC_RANDOMPORTS)->SetWindowText(GetResString(IDS_RANDOMPORTS));
		GetDlgItem(IDC_LBL_MIN)->SetWindowText(GetResString(IDS_MINPORT));
		GetDlgItem(IDC_LBL_MAX)->SetWindowText(GetResString(IDS_MAXPORT));
		//MORPH END   - Added by SiRoB, [MoNKi: [MoNKi: -Random Ports-]
        //MORPH START leuk_he tooltipped
		SetTool(IDC_CAPACITIES_FRM,IDS_PW_CON_CAPFRM_TIP);
		SetTool(IDC_DCAP_LBL,IDS_PW_CON_DOWNLBL_TIP);
		SetTool(IDC_UCAP_LBL,IDS_PW_CON_UPLBL_TIP);
		SetTool(IDC_LIMITS_FRM,IDS_PW_CON_LIMITFRM_TIP);
		SetTool(IDC_DLIMIT_LBL,IDS_PW_DOWNL_TIP);
		SetTool(IDC_ULIMIT_LBL,IDS_PW_UPL_TIP);
		SetTool(IDC_CONNECTION_NETWORK,IDS_NETWORK_TIP);
		SetTool(IDC_KBS2,IDS_KBYTESPERSEC2_TIP);
		SetTool(IDC_KBS3,IDS_KBYTESPERSEC3_TIP);
		SetTool(IDC_SHOWOVERHEAD,IDS_SHOWOVERHEAD_TIP);
		SetTool(IDC_CLIENTPORT_FRM,IDS_PW_CLIENTPORT_TIP);
		SetTool(IDC_MAXSRC_FRM,IDC_MAXSOURCEPERFILE_TIP );
		SetTool(IDC_AUTOCONNECT,IDS_PW_AUTOCON_TIP);
		SetTool(IDC_RECONN,IDS_PW_RECON_TIP);
		SetTool(IDC_WIZARD,IDS_WIZARD_TIP);
		SetTool(IDC_UDPDISABLE,IDS_UDPDISABLED_TIP);
		SetTool(IDC_STARTTEST, IDS_STARTTEST_TIP) ;
		SetTool(IDC_RANDOMPORTS,IDS_RANDOMPORTS_TIP);
		SetTool(IDC_LBL_MIN,IDS_MINPORT_TIP);
		SetTool(IDC_LBL_MAX,IDS_MAXPORT_TIP);
		SetTool(IDC_DOWNLOAD_CAP,IDC_DOWNLOAD_CAP_TIP);
		SetTool(IDC_UPLOAD_CAP,IDC_UPLOAD_CAP_TIP);
		SetTool(IDC_MAXDOWN_SLIDER,IDS_PW_DOWNL_TIP);
		SetTool(IDC_MAXUP_SLIDER,IDS_PW_UPL_TIP);
		SetTool(IDC_PORT,IDC_PORT_TIP);
		SetTool(IDC_UDPPORT,IDC_UDPPORT_TIP);
		SetTool(IDC_MAXSOURCEPERFILE,IDC_MAXSOURCEPERFILE_TIP );
		SetTool(IDC_MAXCON,IDC_MAXCON_TIP);
		SetTool(IDC_MINPORT,IDS_MINPORT_TIP);
		SetTool(IDC_MAXPORT,IDS_MAXPORT_TIP);
		SetTool(IDC_NETWORK_KADEMLIA,IDC_NETWORK_KADEMLIA_TIP);
		SetTool(IDC_NETWORK_ED2K,IDC_NETWORK_ED2K_TIP);
        //MORPH END leuk_he tooltipped
	}
}

void CPPgConnection::OnBnClickedWizard()
{
	CConnectionWizardDlg conWizard;
	conWizard.DoModal();
}

void CPPgConnection::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	SetModified(TRUE);

	if (pScrollBar->GetSafeHwnd() == m_ctlMaxUp.m_hWnd)
	{
		uint32 maxup = m_ctlMaxUp.GetPos();
		uint32 maxdown = m_ctlMaxDown.GetPos();
		if( maxup < 4 && maxup*3 < maxdown)
		{
			m_ctlMaxDown.SetPos(maxup*3);
		}
		if( maxup < 10 && maxup*4 < maxdown)
		{
			m_ctlMaxDown.SetPos(maxup*4);
		}
	}
	else if (pScrollBar->GetSafeHwnd() == m_ctlMaxDown.m_hWnd)
	{
		uint32 maxup = m_ctlMaxUp.GetPos();
		uint32 maxdown = m_ctlMaxDown.GetPos();
		if( maxdown < 13 && maxup*3 < maxdown)
		{
			m_ctlMaxUp.SetPos((int)ceil((double)maxdown/3));
		}
		if( maxdown < 41 && maxup*4 < maxdown)
		{
			m_ctlMaxUp.SetPos((int)ceil((double)maxdown/4));
		}
	}

	ShowLimitValues();

	UpdateData(false); 
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPgConnection::ShowLimitValues()
{
	CString buffer;

	if (!IsDlgButtonChecked(IDC_ULIMIT_LBL))
		buffer=_T("");
	else
		buffer.Format(_T("%u %s"), m_ctlMaxUp.GetPos(), GetResString(IDS_KBYTESPERSEC));
	GetDlgItem(IDC_KBS4)->SetWindowText(buffer);
	
	if (!IsDlgButtonChecked(IDC_DLIMIT_LBL))
		buffer=_T("");
	else
		buffer.Format(_T("%u %s"), m_ctlMaxDown.GetPos(), GetResString(IDS_KBYTESPERSEC));
	GetDlgItem(IDC_KBS1)->SetWindowText(buffer);
}

void CPPgConnection::OnLimiterChange()
{
	m_ctlMaxDown.ShowWindow(IsDlgButtonChecked(IDC_DLIMIT_LBL) ? SW_SHOW : SW_HIDE);
	m_ctlMaxUp.ShowWindow(IsDlgButtonChecked(IDC_ULIMIT_LBL) ? SW_SHOW : SW_HIDE);

	ShowLimitValues();
	SetModified(TRUE);	
}

void CPPgConnection::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Connection);
}

BOOL CPPgConnection::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgConnection::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}

void CPPgConnection::OnBnClickedOpenports()
{
	//MORPH START - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
	thePrefs.SetICFSupport(true);
	thePrefs.m_bICFSupportStatusChanged = true;
	//MORPH END   - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]

	OnApply();
	theApp.m_pFirewallOpener->RemoveRule(EMULE_DEFAULTRULENAME_UDP);
	theApp.m_pFirewallOpener->RemoveRule(EMULE_DEFAULTRULENAME_TCP);
	bool bAlreadyExisted = false;
	if (theApp.m_pFirewallOpener->DoesRuleExist(thePrefs.GetPort(), NAT_PROTOCOL_TCP) || theApp.m_pFirewallOpener->DoesRuleExist(thePrefs.GetUDPPort(), NAT_PROTOCOL_UDP)){
		bAlreadyExisted = true;
	}
	bool bResult = theApp.m_pFirewallOpener->OpenPort(thePrefs.GetPort(), NAT_PROTOCOL_TCP, EMULE_DEFAULTRULENAME_TCP, false);
	if (thePrefs.GetUDPPort() != 0)
		bResult = bResult && theApp.m_pFirewallOpener->OpenPort(thePrefs.GetUDPPort(), NAT_PROTOCOL_UDP, EMULE_DEFAULTRULENAME_UDP, false);
	if (bResult){
		if (!bAlreadyExisted)
			AfxMessageBox(GetResString(IDS_FO_PREF_SUCCCEEDED), MB_ICONINFORMATION | MB_OK);
		else
			// TODO: actually we could offer the user to remove existing rules
			AfxMessageBox(GetResString(IDS_FO_PREF_EXISTED), MB_ICONINFORMATION | MB_OK);
	}
	else
		AfxMessageBox(GetResString(IDS_FO_PREF_FAILED), MB_ICONSTOP | MB_OK);
}

void CPPgConnection::OnStartPortTest()
{
	CString buffer;

	GetDlgItem(IDC_PORT)->GetWindowText(buffer);
	uint16 tcp = (uint16)_tstoi(buffer);

	GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer);
	uint16 udp = (uint16)_tstoi(buffer);

	TriggerPortTest(tcp,udp);
}

void CPPgConnection::SetRateSliderTicks(CSliderCtrl& rRate)
{
	rRate.ClearTics();
	int iMin = 0, iMax = 0;
	rRate.GetRange(iMin, iMax);
	int iDiff = iMax - iMin;
	if (iDiff > 0)
	{
		CRect rc;
		rRate.GetWindowRect(&rc);
		if (rc.Width() > 0)
		{
			int iTic;
			int iPixels = rc.Width() / iDiff;
			if (iPixels >= 6)
				iTic = 1;
			else
			{
				iTic = 10;
				while (rc.Width() / (iDiff / iTic) < 8)
					iTic *= 10;
			}
			if (iTic)
			{
				for (int i = ((iMin+(iTic-1))/iTic)*iTic; i < iMax; /**/)
				{
					rRate.SetTic(i);
					i += iTic;
				}
			}
			rRate.SetPageSize(iTic);
		}
	}
}

//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-]
void CPPgConnection::OnRandomPortsChange() {
	CString udpStr;
	unsigned int udp;

	GetDlgItem(IDC_UDPPORT)->GetWindowText(udpStr);
	udp = _tstoi(udpStr);

	GetDlgItem(IDC_UDPPORT)->EnableWindow(!IsDlgButtonChecked(IDC_RANDOMPORTS) && udp>0);
	GetDlgItem(IDC_PORT)->EnableWindow(!IsDlgButtonChecked(IDC_RANDOMPORTS));
	m_minRndPort.EnableWindow(IsDlgButtonChecked(IDC_RANDOMPORTS));
	m_maxRndPort.EnableWindow(IsDlgButtonChecked(IDC_RANDOMPORTS));
	m_minRndPortSpin.EnableWindow(IsDlgButtonChecked(IDC_RANDOMPORTS));
	m_maxRndPortSpin.EnableWindow(IsDlgButtonChecked(IDC_RANDOMPORTS));

	SetModified(TRUE);	
}
//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-]
