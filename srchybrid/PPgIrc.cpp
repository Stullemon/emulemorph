// PPgIRC.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgIRC.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CPPgIRC dialog

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
	ON_BN_CLICKED(IDC_IRC_USECHANFILTER, OnSettingsChange)
	ON_BN_CLICKED(IDC_IRC_USEPERFORM, OnSettingsChange)
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

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgIRC::LoadSettings(void)
{	//What am I doing wrong? why can't I use the app_prefs->prefs like the other tabs?
	if(app_prefs->prefs->m_bircaddtimestamp)
		this->CheckDlgButton(IDC_IRC_TIMESTAMP,1);
	else
		this->CheckDlgButton(IDC_IRC_TIMESTAMP,0);
	if(app_prefs->prefs->m_bircignoreinfomessage)
		this->CheckDlgButton(IDC_IRC_INFOMESSAGE,1);
	else
		this->CheckDlgButton(IDC_IRC_INFOMESSAGE,0);
	if(app_prefs->prefs->m_bircignoreemuleprotoinfomessage)
		this->CheckDlgButton(IDC_IRC_EMULEPROTO_INFOMESSAGE,1);
	else
		this->CheckDlgButton(IDC_IRC_EMULEPROTO_INFOMESSAGE,0);
	if(app_prefs->prefs->m_bircacceptlinks)
		this->CheckDlgButton(IDC_IRC_ACCEPTLINKS,1);
	else
		this->CheckDlgButton(IDC_IRC_ACCEPTLINKS,0);
	if(app_prefs->prefs->m_bircacceptlinksfriends)
		this->CheckDlgButton(IDC_IRC_ACCEPTLINKSFRIENDS,1);
	else
		this->CheckDlgButton(IDC_IRC_ACCEPTLINKSFRIENDS,0);
	if(app_prefs->prefs->m_bircusechanfilter)
		this->CheckDlgButton(IDC_IRC_USECHANFILTER,1);
	else
		this->CheckDlgButton(IDC_IRC_USECHANFILTER,0);
	if(app_prefs->prefs->m_bircuseperform)
		this->CheckDlgButton(IDC_IRC_USEPERFORM,1);
	else
		this->CheckDlgButton(IDC_IRC_USEPERFORM,0);
	if(app_prefs->prefs->m_birclistonconnect)
		this->CheckDlgButton(IDC_IRC_LISTONCONNECT,1);
	else
		this->CheckDlgButton(IDC_IRC_LISTONCONNECT,0);
	if(app_prefs->prefs->m_birchelpchannel)
		this->CheckDlgButton(IDC_IRC_HELPCHANNEL,1);
	else
		this->CheckDlgButton(IDC_IRC_HELPCHANNEL,0);
	GetDlgItem(IDC_IRC_SERVER_BOX)->SetWindowText(app_prefs->prefs->m_sircserver);
	GetDlgItem(IDC_IRC_NICK_BOX)->SetWindowText(app_prefs->prefs->m_sircnick);
	GetDlgItem(IDC_IRC_NAME_BOX)->SetWindowText(app_prefs->prefs->m_sircchannamefilter);
	GetDlgItem(IDC_IRC_PERFORM_BOX)->SetWindowText(app_prefs->prefs->m_sircperformstring);
	CString strBuffer;
	strBuffer.Format("%d", app_prefs->prefs->m_iircchanneluserfilter);
	GetDlgItem(IDC_IRC_MINUSER_BOX)->SetWindowText(strBuffer);
}


BOOL CPPgIRC::OnApply()
{   //What am I doing wrong? why can't I use the app_prefs->prefs like the other tabs?
	if(IsDlgButtonChecked(IDC_IRC_TIMESTAMP))
		app_prefs->prefs->m_bircaddtimestamp = true;
	else
		app_prefs->prefs->m_bircaddtimestamp = false;
	if(IsDlgButtonChecked(IDC_IRC_INFOMESSAGE))
		app_prefs->prefs->m_bircignoreinfomessage = true;
	else
		app_prefs->prefs->m_bircignoreinfomessage = false;
	if(IsDlgButtonChecked(IDC_IRC_EMULEPROTO_INFOMESSAGE))
		app_prefs->prefs->m_bircignoreemuleprotoinfomessage = true;
	else
		app_prefs->prefs->m_bircignoreemuleprotoinfomessage = false;
	if(IsDlgButtonChecked(IDC_IRC_ACCEPTLINKS))
		app_prefs->prefs->m_bircacceptlinks = true;
	else
		app_prefs->prefs->m_bircacceptlinks = false;
	if(IsDlgButtonChecked(IDC_IRC_ACCEPTLINKSFRIENDS))
		app_prefs->prefs->m_bircacceptlinksfriends = true;
	else
		app_prefs->prefs->m_bircacceptlinksfriends = false;
	if(IsDlgButtonChecked(IDC_IRC_LISTONCONNECT))
		app_prefs->prefs->m_birclistonconnect = true;
	else
		app_prefs->prefs->m_birclistonconnect = false;
	if(IsDlgButtonChecked(IDC_IRC_USECHANFILTER))
		app_prefs->prefs->m_bircusechanfilter = true;
	else
		app_prefs->prefs->m_bircusechanfilter = false;
	if(IsDlgButtonChecked(IDC_IRC_USEPERFORM))
		app_prefs->prefs->m_bircuseperform = true;
	else
		app_prefs->prefs->m_bircuseperform = false;
	if(IsDlgButtonChecked(IDC_IRC_HELPCHANNEL))
		app_prefs->prefs->m_birchelpchannel = true;
	else
		app_prefs->prefs->m_birchelpchannel = false;
	char buffer[510];
	if(GetDlgItem(IDC_IRC_NICK_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_NICK_BOX)->GetWindowText(buffer,20);
		strcpy(app_prefs->prefs->m_sircnick,buffer);
		if( theApp.emuledlg->ircwnd.GetLoggedIn() && m_bnickModified == true){
			m_bnickModified = false;
			theApp.emuledlg->ircwnd.SendString( (CString)"NICK " + (CString)buffer );
		}
	}

	if(GetDlgItem(IDC_IRC_SERVER_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_SERVER_BOX)->GetWindowText(buffer,40);
		strcpy(app_prefs->prefs->m_sircserver,buffer);
	}

	if(GetDlgItem(IDC_IRC_NAME_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_NAME_BOX)->GetWindowText(buffer,40);
		strcpy(app_prefs->prefs->m_sircchannamefilter,buffer);
	}

	if(GetDlgItem(IDC_IRC_PERFORM_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_PERFORM_BOX)->GetWindowText(buffer,250);
		strcpy(app_prefs->prefs->m_sircperformstring,buffer);
	}
	else
	{
		sprintf( buffer, " " );
		strcpy(app_prefs->prefs->m_sircperformstring,buffer);
	}

	if(GetDlgItem(IDC_IRC_MINUSER_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_MINUSER_BOX)->GetWindowText(buffer,6);
		app_prefs->prefs->m_iircchanneluserfilter = atoi(buffer);
	}
	else{
		app_prefs->prefs->m_iircchanneluserfilter = 0;
	}
//	app_prefs->Save();
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
