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
#include "PPgServer.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "HelpIDs.h"
#include "Opcodes.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CPPgServer, CPropertyPage)

CPPgServer::CPPgServer()
	: CPropertyPage(CPPgServer::IDD)
{
}

CPPgServer::~CPPgServer()
{
}

void CPPgServer::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPPgServer, CPropertyPage)
	ON_EN_CHANGE(IDC_SERVERRETRIES, OnSettingsChange)
	ON_BN_CLICKED(IDC_AUTOSERVER, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPDATESERVERCONNECT, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPDATESERVERCLIENT, OnSettingsChange)
	ON_BN_CLICKED(IDC_SCORE, OnSettingsChange)
	ON_BN_CLICKED(IDC_SMARTIDCHECK, OnSettingsChange)
	ON_BN_CLICKED(IDC_SAFESERVERCONNECT, OnSettingsChange)
	ON_BN_CLICKED(IDC_AUTOCONNECTSTATICONLY, OnSettingsChange)
	ON_BN_CLICKED(IDC_MANUALSERVERHIGHPRIO, OnSettingsChange)
	ON_BN_CLICKED(IDC_EDITADR, OnBnClickedEditadr)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()


// CPPgServer message handlers

BOOL CPPgServer::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgServer::LoadSettings(void)
{
	SetDlgItemInt(IDC_SERVERRETRIES, thePrefs.deadserverretries, FALSE);

	if(thePrefs.IsSafeServerConnectEnabled())
		CheckDlgButton(IDC_SAFESERVERCONNECT,1);
	else
		CheckDlgButton(IDC_SAFESERVERCONNECT,0);

	if(thePrefs.m_bmanualhighprio)
		CheckDlgButton(IDC_MANUALSERVERHIGHPRIO,1);
	else
		CheckDlgButton(IDC_MANUALSERVERHIGHPRIO,0);

	if(thePrefs.GetSmartIdCheck())
		CheckDlgButton(IDC_SMARTIDCHECK,1);
	else
		CheckDlgButton(IDC_SMARTIDCHECK,0);

	if(thePrefs.autoserverlist)
		CheckDlgButton(IDC_AUTOSERVER,1);
	else
		CheckDlgButton(IDC_AUTOSERVER,0);

	if(thePrefs.addserversfromserver)
		CheckDlgButton(IDC_UPDATESERVERCONNECT, 1);
	else
		CheckDlgButton(IDC_UPDATESERVERCONNECT, 0);
	
	if(thePrefs.addserversfromclient)
		CheckDlgButton(IDC_UPDATESERVERCLIENT, 1);
	else
		CheckDlgButton(IDC_UPDATESERVERCLIENT, 0);

	if(thePrefs.scorsystem)
		CheckDlgButton(IDC_SCORE,1);
	else
		CheckDlgButton(IDC_SCORE,0);

	if(thePrefs.autoconnectstaticonly)
		CheckDlgButton(IDC_AUTOCONNECTSTATICONLY,1);
	else
		CheckDlgButton(IDC_AUTOCONNECTSTATICONLY,0);
}

BOOL CPPgServer::OnApply()
{	
	thePrefs.SetSafeServerConnectEnabled((uint8)IsDlgButtonChecked(IDC_SAFESERVERCONNECT));

	if(IsDlgButtonChecked(IDC_SMARTIDCHECK))
		thePrefs.smartidcheck = true;
	else
		thePrefs.smartidcheck = false;

	if(IsDlgButtonChecked(IDC_MANUALSERVERHIGHPRIO))
		thePrefs.m_bmanualhighprio = true;
	else
		thePrefs.m_bmanualhighprio = false;

	thePrefs.deadserverretries = GetDlgItemInt(IDC_SERVERRETRIES, NULL, FALSE);
	if (thePrefs.deadserverretries < 1)
		thePrefs.deadserverretries = 1;
	else if (thePrefs.deadserverretries > MAX_SERVERFAILCOUNT)
		thePrefs.deadserverretries = MAX_SERVERFAILCOUNT;

	thePrefs.scorsystem = (uint8)IsDlgButtonChecked(IDC_SCORE);
	thePrefs.autoserverlist = (uint8)IsDlgButtonChecked(IDC_AUTOSERVER);
	thePrefs.addserversfromserver = (uint8)IsDlgButtonChecked(IDC_UPDATESERVERCONNECT);
	thePrefs.addserversfromclient = (uint8)IsDlgButtonChecked(IDC_UPDATESERVERCLIENT);
	thePrefs.autoconnectstaticonly = (uint8)IsDlgButtonChecked(IDC_AUTOCONNECTSTATICONLY);

	LoadSettings();

	SetModified();
	return CPropertyPage::OnApply();
}

void CPPgServer::Localize(void)
{
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_SERVER));
		
		GetDlgItem(IDC_REMOVEDEAD)->SetWindowText(GetResString(IDS_PW_RDEAD));
		GetDlgItem(IDC_RETRIES_LBL)->SetWindowText(GetResString(IDS_PW_RETRIES));
		GetDlgItem(IDC_UPDATESERVERCONNECT)->SetWindowText(GetResString(IDS_PW_USC));
		GetDlgItem(IDC_UPDATESERVERCLIENT)->SetWindowText(GetResString(IDS_PW_UCC));
		GetDlgItem(IDC_AUTOSERVER)->SetWindowText(GetResString(IDS_PW_USS));
		GetDlgItem(IDC_SMARTIDCHECK)->SetWindowText(GetResString(IDS_SMARTLOWIDCHECK));
		GetDlgItem(IDC_SAFESERVERCONNECT)->SetWindowText(GetResString(IDS_PW_FASTSRVCON));
		GetDlgItem(IDC_SCORE)->SetWindowText(GetResString(IDS_PW_SCORE));
		GetDlgItem(IDC_MANUALSERVERHIGHPRIO)->SetWindowText(GetResString(IDS_MANUALSERVERHIGHPRIO));
		GetDlgItem(IDC_EDITADR)->SetWindowText(GetResString(IDS_EDITLIST));
		
		// Barry
		GetDlgItem(IDC_AUTOCONNECTSTATICONLY)->SetWindowText(GetResString(IDS_PW_AUTOCONNECTSTATICONLY));
	}
}

void CPPgServer::OnBnClickedEditadr()
{
	ShellExecute(NULL, _T("open"), thePrefs.GetTxtEditor(), _T("\"") + CString(thePrefs.GetConfigDir()) + _T("adresses.dat\""), NULL, SW_SHOW); 
}

void CPPgServer::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Server);
}

BOOL CPPgServer::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgServer::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}
