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
#include "PPgIRC.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "Preferences.h"
#include "IrcWnd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CPPgIRC, CPropertyPage)
CPPgIRC::CPPgIRC()
	: CPropertyPage(CPPgIRC::IDD)
{
}

CPPgIRC::~CPPgIRC()
{
}

void CPPgIRC::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPPgIRC, CPropertyPage)
	ON_BN_CLICKED(IDC_IRC_TIMESTAMP, OnSettingsChange)
	ON_BN_CLICKED(IDC_IRC_USECHANFILTER, OnBtnClickPerform)
	ON_BN_CLICKED(IDC_IRC_USEPERFORM, OnBtnClickPerform)
	ON_BN_CLICKED(IDC_IRC_ACCEPTLINKS, OnSettingsChange)
	ON_BN_CLICKED(IDC_IRC_ACCEPTLINKSFRIENDS, OnSettingsChange)
	ON_BN_CLICKED(IDC_IRC_INFOMESSAGE, OnSettingsChange)
	ON_BN_CLICKED(IDC_IRC_EMULEPROTO_INFOMESSAGE, OnSettingsChange)
	ON_BN_CLICKED(IDC_IRC_HELPCHANNEL, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_NICK_BOX, OnEnChangeNick)
	ON_EN_CHANGE(IDC_IRC_PERFORM_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_SERVER_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_NAME_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_MINUSER_BOX, OnSettingsChange)
	ON_BN_CLICKED(IDC_IRC_LISTONCONNECT, OnSettingsChange)
END_MESSAGE_MAP()


BOOL CPPgIRC::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
	((CEdit*)GetDlgItem(IDC_IRC_NICK_BOX))->SetLimitText(20);
	((CEdit*)GetDlgItem(IDC_IRC_MINUSER_BOX))->SetLimitText(6);
	((CEdit*)GetDlgItem(IDC_IRC_SERVER_BOX))->SetLimitText(40);
	((CEdit*)GetDlgItem(IDC_IRC_NAME_BOX))->SetLimitText(40);
	((CEdit*)GetDlgItem(IDC_IRC_PERFORM_BOX))->SetLimitText(250);
	LoadSettings();
	Localize();
	m_bnickModified = false;

	GetDlgItem(IDC_IRC_PERFORM_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USEPERFORM) );
	GetDlgItem(IDC_IRC_NAME_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USECHANFILTER) );
	GetDlgItem(IDC_IRC_MINUSER_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USECHANFILTER) );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgIRC::LoadSettings(void)
{	//What am I doing wrong? why can't I use the thePrefs.prefs like the other tabs?
	if(thePrefs.m_bircaddtimestamp)
		this->CheckDlgButton(IDC_IRC_TIMESTAMP,1);
	else
		this->CheckDlgButton(IDC_IRC_TIMESTAMP,0);
	if(thePrefs.m_bircignoreinfomessage)
		this->CheckDlgButton(IDC_IRC_INFOMESSAGE,1);
	else
		this->CheckDlgButton(IDC_IRC_INFOMESSAGE,0);
	if(thePrefs.m_bircignoreemuleprotoinfomessage)
		this->CheckDlgButton(IDC_IRC_EMULEPROTO_INFOMESSAGE,1);
	else
		this->CheckDlgButton(IDC_IRC_EMULEPROTO_INFOMESSAGE,0);
	if(thePrefs.m_bircacceptlinks)
		this->CheckDlgButton(IDC_IRC_ACCEPTLINKS,1);
	else
		this->CheckDlgButton(IDC_IRC_ACCEPTLINKS,0);
	if(thePrefs.m_bircacceptlinksfriends)
		this->CheckDlgButton(IDC_IRC_ACCEPTLINKSFRIENDS,1);
	else
		this->CheckDlgButton(IDC_IRC_ACCEPTLINKSFRIENDS,0);
	if(thePrefs.m_bircusechanfilter)
		this->CheckDlgButton(IDC_IRC_USECHANFILTER,1);
	else
		this->CheckDlgButton(IDC_IRC_USECHANFILTER,0);
	if(thePrefs.m_bircuseperform)
		this->CheckDlgButton(IDC_IRC_USEPERFORM,1);
	else
		this->CheckDlgButton(IDC_IRC_USEPERFORM,0);
	if(thePrefs.m_birclistonconnect)
		this->CheckDlgButton(IDC_IRC_LISTONCONNECT,1);
	else
		this->CheckDlgButton(IDC_IRC_LISTONCONNECT,0);
	if(thePrefs.m_birchelpchannel)
		this->CheckDlgButton(IDC_IRC_HELPCHANNEL,1);
	else
		this->CheckDlgButton(IDC_IRC_HELPCHANNEL,0);
	GetDlgItem(IDC_IRC_SERVER_BOX)->SetWindowText(thePrefs.m_sircserver);
	GetDlgItem(IDC_IRC_NICK_BOX)->SetWindowText(thePrefs.m_sircnick);
	GetDlgItem(IDC_IRC_NAME_BOX)->SetWindowText(thePrefs.m_sircchannamefilter);
	GetDlgItem(IDC_IRC_PERFORM_BOX)->SetWindowText(thePrefs.m_sircperformstring);
	CString strBuffer;
	strBuffer.Format("%d", thePrefs.m_iircchanneluserfilter);
	GetDlgItem(IDC_IRC_MINUSER_BOX)->SetWindowText(strBuffer);
}


BOOL CPPgIRC::OnApply()
{   //What am I doing wrong? why can't I use the thePrefs.prefs like the other tabs?
	if(IsDlgButtonChecked(IDC_IRC_TIMESTAMP))
		thePrefs.m_bircaddtimestamp = true;
	else
		thePrefs.m_bircaddtimestamp = false;
	if(IsDlgButtonChecked(IDC_IRC_INFOMESSAGE))
		thePrefs.m_bircignoreinfomessage = true;
	else
		thePrefs.m_bircignoreinfomessage = false;
	if(IsDlgButtonChecked(IDC_IRC_EMULEPROTO_INFOMESSAGE))
		thePrefs.m_bircignoreemuleprotoinfomessage = true;
	else
		thePrefs.m_bircignoreemuleprotoinfomessage = false;
	if(IsDlgButtonChecked(IDC_IRC_ACCEPTLINKS))
		thePrefs.m_bircacceptlinks = true;
	else
		thePrefs.m_bircacceptlinks = false;
	if(IsDlgButtonChecked(IDC_IRC_ACCEPTLINKSFRIENDS))
		thePrefs.m_bircacceptlinksfriends = true;
	else
		thePrefs.m_bircacceptlinksfriends = false;
	if(IsDlgButtonChecked(IDC_IRC_LISTONCONNECT))
		thePrefs.m_birclistonconnect = true;
	else
		thePrefs.m_birclistonconnect = false;
	if(IsDlgButtonChecked(IDC_IRC_USECHANFILTER))
		thePrefs.m_bircusechanfilter = true;
	else
		thePrefs.m_bircusechanfilter = false;
	if(IsDlgButtonChecked(IDC_IRC_USEPERFORM))
		thePrefs.m_bircuseperform = true;
	else
		thePrefs.m_bircuseperform = false;
	if(IsDlgButtonChecked(IDC_IRC_HELPCHANNEL))
		thePrefs.m_birchelpchannel = true;
	else
		thePrefs.m_birchelpchannel = false;
	char buffer[510];
	if(GetDlgItem(IDC_IRC_NICK_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_NICK_BOX)->GetWindowText(buffer,20);
		strcpy(thePrefs.m_sircnick,buffer);
		if( theApp.emuledlg->ircwnd->GetLoggedIn() && m_bnickModified == true){
			m_bnickModified = false;
			theApp.emuledlg->ircwnd->SendString( (CString)"NICK " + (CString)buffer );
		}
	}

	if(GetDlgItem(IDC_IRC_SERVER_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_SERVER_BOX)->GetWindowText(buffer,40);
		strcpy(thePrefs.m_sircserver,buffer);
	}

	if(GetDlgItem(IDC_IRC_NAME_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_NAME_BOX)->GetWindowText(buffer,40);
		strcpy(thePrefs.m_sircchannamefilter,buffer);
	}

	if(GetDlgItem(IDC_IRC_PERFORM_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_PERFORM_BOX)->GetWindowText(buffer,250);
		strcpy(thePrefs.m_sircperformstring,buffer);
	}
	else
	{
		sprintf( buffer, " " );
		strcpy(thePrefs.m_sircperformstring,buffer);
	}

	if(GetDlgItem(IDC_IRC_MINUSER_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_MINUSER_BOX)->GetWindowText(buffer,6);
		thePrefs.m_iircchanneluserfilter = atoi(buffer);
	}
	else{
		thePrefs.m_iircchanneluserfilter = 0;
	}
//	thePrefs.Save();
	LoadSettings();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}
void CPPgIRC::Localize(void)
{
	if(m_hWnd)
	{
		GetDlgItem(IDC_IRC_SERVER_FRM)->SetWindowText(GetResString(IDS_PW_SERVER));
		GetDlgItem(IDC_IRC_MISC_FRM)->SetWindowText(GetResString(IDS_PW_MISC));
		GetDlgItem(IDC_IRC_TIMESTAMP)->SetWindowText(GetResString(IDS_IRC_ADDTIMESTAMP));
		GetDlgItem(IDC_IRC_NICK_FRM)->SetWindowText(GetResString(IDS_PW_NICK));
		GetDlgItem(IDC_IRC_NAME_TEXT)->SetWindowText(GetResString(IDS_IRC_NAME));
		GetDlgItem(IDC_IRC_MINUSER_TEXT)->SetWindowText(GetResString(IDS_UUSERS));
		GetDlgItem(IDC_IRC_FILTER_FRM)->SetWindowText(GetResString(IDS_IRC_CHANNELLIST));
		GetDlgItem(IDC_IRC_USECHANFILTER)->SetWindowText(GetResString(IDS_IRC_USEFILTER));
		GetDlgItem(IDC_IRC_PERFORM_FRM)->SetWindowText(GetResString(IDS_IRC_PERFORM));
		GetDlgItem(IDC_IRC_USEPERFORM)->SetWindowText(GetResString(IDS_IRC_USEPERFORM));
		GetDlgItem(IDC_IRC_LISTONCONNECT)->SetWindowText(GetResString(IDS_IRC_LOADCHANNELLISTONCON));
		GetDlgItem(IDC_IRC_ACCEPTLINKS)->SetWindowText(GetResString(IDS_IRC_ACCEPTLINKS));
		GetDlgItem(IDC_IRC_ACCEPTLINKSFRIENDS)->SetWindowText(GetResString(IDS_IRC_ACCEPTLINKSFRIENDS));
		GetDlgItem(IDC_IRC_INFOMESSAGE)->SetWindowText(GetResString(IDS_IRC_IGNOREINFOMESSAGE));
		GetDlgItem(IDC_IRC_EMULEPROTO_INFOMESSAGE)->SetWindowText(GetResString(IDS_IRC_EMULEPROTO_IGNOREINFOMESSAGE));
		GetDlgItem(IDC_IRC_HELPCHANNEL)->SetWindowText(GetResString(IDS_IRC_HELPCHANNEL));
	}
}

void CPPgIRC::OnBtnClickPerform() {
	SetModified();
	GetDlgItem(IDC_IRC_PERFORM_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USEPERFORM) );

	GetDlgItem(IDC_IRC_NAME_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USECHANFILTER) );
	GetDlgItem(IDC_IRC_MINUSER_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USECHANFILTER) );
}