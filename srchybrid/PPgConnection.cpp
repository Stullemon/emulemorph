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
#include "emule.h"
#include "PPgConnection.h"
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

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CPPgConnection, CPropertyPage)
CPPgConnection::CPPgConnection()
	: CPropertyPage(CPPgConnection::IDD)
{
	guardian=false;
}

CPPgConnection::~CPPgConnection()
{
}

void CPPgConnection::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

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
END_MESSAGE_MAP()


// CPPgConnection message handlers
void CPPgConnection::OnEnChangeTCP(){
	OnEnChangePorts(true);
}

void CPPgConnection::OnEnChangeUDP(){
	OnEnChangePorts(false);
}

void CPPgConnection::OnEnChangePorts(uint8 istcpport){


	// ports unchanged?
	CString buffer;
	GetDlgItem(IDC_PORT)->GetWindowText(buffer);
	uint16 tcp= _tstoi(buffer);
	GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer);
	uint16 udp= _tstoi(buffer);

	GetDlgItem(IDC_STARTTEST)->EnableWindow( 
		tcp==theApp.listensocket->GetConnectedPort() && 
		udp==theApp.clientudp->GetConnectedPort() 
	);


	if (istcpport==0)
		OnEnChangeUDPDisable();
	else if (istcpport==1)
		OnSettingsChange();
}

void CPPgConnection::OnEnChangeUDPDisable(){
		if (guardian) return;

		uint16 tempVal=0;
		CString strBuffer;
		TCHAR buffer[510];
		
		guardian=true;
		SetModified();

		GetDlgItem(IDC_UDPPORT)->EnableWindow(!IsDlgButtonChecked(IDC_UDPDISABLE));

		if(GetDlgItem(IDC_UDPPORT)->GetWindowTextLength())
		{
			GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer,20);
			tempVal= _tstoi(buffer);
		}

		
		if (IsDlgButtonChecked(IDC_UDPDISABLE) || (!IsDlgButtonChecked(IDC_UDPDISABLE) && tempVal==0)){
			tempVal= (_tstoi(buffer)) ? _tstoi(buffer)+10 : thePrefs.port+10;
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

	LoadSettings();
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

		GetDlgItem(IDC_UDPPORT)->EnableWindow(thePrefs.udpport>0);
	
		strBuffer.Format(_T("%d"), thePrefs.maxGraphDownloadRate);
		GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(strBuffer);

		((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetRange(1, thePrefs.maxGraphDownloadRate);

		strBuffer.Format(_T("%d"), thePrefs.maxGraphUploadRate);
		GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(strBuffer);

		((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetRange(1, thePrefs.maxGraphUploadRate);


		CheckDlgButton( IDC_DLIMIT_LBL, (thePrefs.maxdownload!=UNLIMITED) );
		CheckDlgButton( IDC_ULIMIT_LBL, (thePrefs.maxupload!=UNLIMITED) );

		((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetPos(
			(thePrefs.maxdownload!=UNLIMITED)?thePrefs.maxdownload:thePrefs.maxGraphDownloadRate
		);
		
		((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetPos(
			(thePrefs.maxupload!=UNLIMITED)?thePrefs.maxupload:thePrefs.maxGraphUploadRate
		);

		strBuffer.Format(_T("%d"), thePrefs.port);
		GetDlgItem(IDC_PORT)->SetWindowText(strBuffer);

		strBuffer.Format(_T("%d"), thePrefs.maxconnections);
		GetDlgItem(IDC_MAXCON)->SetWindowText(strBuffer);

		if(thePrefs.maxsourceperfile == 0xFFFF)
			GetDlgItem(IDC_MAXSOURCEPERFILE)->SetWindowText(_T("0"));
		else
		{
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

		if (theApp.m_pFirewallOpener->DoesFWConnectionExist())
			GetDlgItem(IDC_OPENPORTS)->EnableWindow(true);
		else
			GetDlgItem(IDC_OPENPORTS)->EnableWindow(false);

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

	((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetRange(1, thePrefs.GetMaxGraphDownloadRate(), TRUE);

	if(GetDlgItem(IDC_UPLOAD_CAP)->GetWindowTextLength())
	{
		GetDlgItem(IDC_UPLOAD_CAP)->GetWindowText(buffer,20);
		thePrefs.SetMaxGraphUploadRate(_tstoi(buffer));
	}

	((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetRange(1, thePrefs.GetMaxGraphUploadRate(), TRUE);

	if (IsDlgButtonChecked(IDC_ULIMIT_LBL)==FALSE)
		thePrefs.SetMaxUpload(UNLIMITED);
	else
		thePrefs.SetMaxUpload(((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->GetPos());

	if( thePrefs.GetMaxGraphUploadRate() < thePrefs.GetMaxUpload() && thePrefs.GetMaxUpload()!=UNLIMITED  )
		thePrefs.SetMaxUpload(thePrefs.GetMaxGraphUploadRate()*.8);

	if (thePrefs.GetMaxUpload()!=UNLIMITED)
		((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetPos(thePrefs.GetMaxUpload());
	
	if (IsDlgButtonChecked(IDC_DLIMIT_LBL)==FALSE)
		thePrefs.SetMaxDownload(UNLIMITED);
	else
		thePrefs.SetMaxDownload(((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->GetPos());

	if( thePrefs.GetMaxGraphDownloadRate() < thePrefs.GetMaxDownload() && thePrefs.GetMaxDownload()!=UNLIMITED )
		thePrefs.SetMaxDownload(thePrefs.GetMaxGraphDownloadRate()*.8);

	if (thePrefs.GetMaxDownload()!=UNLIMITED)
		((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetPos(thePrefs.GetMaxDownload());

	if(GetDlgItem(IDC_PORT)->GetWindowTextLength())
	{
		GetDlgItem(IDC_PORT)->GetWindowText(buffer,20);
		uint16 nNewPort = (_tstoi(buffer)) ? _tstoi(buffer) : 4662;
		if (nNewPort != thePrefs.port){
			thePrefs.port = nNewPort;
			if (theApp.IsPortchangeAllowed())
				theApp.listensocket->Rebind();
			else
				bRestartApp = true;
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
		uint16 nNewPort = (_tstoi(buffer) && !IsDlgButtonChecked(IDC_UDPDISABLE) ) ? _tstoi(buffer) : 0;
		if (nNewPort != thePrefs.udpport){
			thePrefs.udpport = nNewPort;
			if (theApp.IsPortchangeAllowed())
				theApp.clientudp->Rebind();
			else 
				bRestartApp = true;
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

	//	if(IsDlgButtonChecked(IDC_UDPDISABLE)) thePrefs.udpport=0;
	GetDlgItem(IDC_UDPPORT)->EnableWindow(!IsDlgButtonChecked(IDC_UDPDISABLE));

	thePrefs.autoconnect = (uint8)IsDlgButtonChecked(IDC_AUTOCONNECT);
	thePrefs.reconnect = (uint8)IsDlgButtonChecked(IDC_RECONN);
		
	if(lastmaxgu != thePrefs.maxGraphUploadRate) 
		theApp.emuledlg->statisticswnd->SetARange(false,thePrefs.maxGraphUploadRate);
	if(lastmaxgd!=thePrefs.maxGraphDownloadRate)
		theApp.emuledlg->statisticswnd->SetARange(true,thePrefs.maxGraphDownloadRate);

	uint16 tempcon = thePrefs.maxconnections;
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

	//if (thePrefs.maxGraphDownloadRate<thePrefs.maxdownload) thePrefs.maxdownload=UNLIMITED;
	//if (thePrefs.maxGraphUploadRate<thePrefs.maxupload) thePrefs.maxupload=UNLIMITED;

	SetModified(FALSE);
	LoadSettings();

	theApp.emuledlg->ShowConnectionState();

//	thePrefs.Save();
//	theApp.emuledlg->preferenceswnd->m_wndTweaks.LoadSettings();

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

//		GetDlgItem(IDC_KBS1)->SetWindowText(GetResString(IDS_KBYTESEC));
		GetDlgItem(IDC_KBS2)->SetWindowText(GetResString(IDS_KBYTESEC));
		GetDlgItem(IDC_KBS3)->SetWindowText(GetResString(IDS_KBYTESEC));
//		GetDlgItem(IDC_KBS4)->SetWindowText(GetResString(IDS_KBYTESEC));

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
	}
}

void CPPgConnection::OnBnClickedWizard()
{
	Wizard conWizard;
	conWizard.DoModal();
}

void CPPgConnection::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	SetModified(TRUE);

	if( pScrollBar == (CScrollBar*)GetDlgItem(IDC_MAXUP_SLIDER))
	{
		uint32 maxup = ((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->GetPos();
		uint32 maxdown = ((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->GetPos();
		if( maxup < 4 && maxup*3 < maxdown)
		{
			((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetPos(maxup*3);
		}
		if( maxup < 10 && maxup*4 < maxdown)
		{
			((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetPos(maxup*4);
		}
	}
	else if (pScrollBar == (CScrollBar*)GetDlgItem(IDC_MAXDOWN_SLIDER))
	{
		uint32 maxup = ((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->GetPos();
		uint32 maxdown = ((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->GetPos();
		if( maxdown < 13 && maxup*3 < maxdown)
		{
			((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetPos(ceil((double)maxdown/3));
		}
		if( maxdown < 41 && maxup*4 < maxdown)
		{
			((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetPos(ceil((double)maxdown/4));
		}
	}

	ShowLimitValues();

	UpdateData(false); 
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPgConnection::ShowLimitValues() {
	CString buffer;

	if (IsDlgButtonChecked(IDC_ULIMIT_LBL)==FALSE)
		buffer=_T("");
	else
		buffer.Format(_T("%u %s"), ((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->GetPos(), GetResString(IDS_KBYTESEC));
	GetDlgItem(IDC_KBS4)->SetWindowText(buffer);
	
	if (IsDlgButtonChecked(IDC_DLIMIT_LBL)==FALSE)
		buffer=_T("");
	else
		buffer.Format(_T("%u %s"), ((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->GetPos(), GetResString(IDS_KBYTESEC));
	GetDlgItem(IDC_KBS1)->SetWindowText(buffer);
}

void CPPgConnection::OnLimiterChange() {
	
	GetDlgItem(IDC_MAXDOWN_SLIDER)->ShowWindow( IsDlgButtonChecked(IDC_DLIMIT_LBL)?SW_SHOW:SW_HIDE);
	GetDlgItem(IDC_MAXUP_SLIDER)->ShowWindow( IsDlgButtonChecked(IDC_ULIMIT_LBL)?SW_SHOW:SW_HIDE );

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

BOOL CPPgConnection::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}

void CPPgConnection::OnBnClickedOpenports()
{
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

void CPPgConnection::OnStartPortTest() {

	CString buffer;

	GetDlgItem(IDC_PORT)->GetWindowText(buffer);
	uint16 tcp= _tstoi(buffer);

	GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer);
	uint16 udp= _tstoi(buffer);

	TriggerPortTest(tcp,udp);
}
