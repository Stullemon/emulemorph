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

BEGIN_MESSAGE_MAP(CPPgIRC, CPropertyPage)
	ON_BN_CLICKED(IDC_IRC_USECHANFILTER, OnBtnClickPerform)
	ON_BN_CLICKED(IDC_IRC_USEPERFORM, OnBtnClickPerform)
	ON_EN_CHANGE(IDC_IRC_NICK_BOX, OnEnChangeNick)
	ON_EN_CHANGE(IDC_IRC_PERFORM_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_SERVER_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_NAME_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_MINUSER_BOX, OnSettingsChange)
	ON_BN_CLICKED(IDC_IRC_LISTONCONNECT, OnSettingsChange)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_TREEOPTSCTRL_NOTIFY, OnTreeOptsCtrlNotify)
END_MESSAGE_MAP()

CPPgIRC::CPPgIRC()
	: CPropertyPage(CPPgIRC::IDD)
	, m_ctrlTreeOptions(theApp.m_iDfltImageListColorFlags)
{
	m_iTimeStamp = 0;
	m_iSoundEvents = 0;
	m_iInfoMessage = 0;
	m_iMiscMessage = 0;
	m_iJoinMessage = 0;
	m_iPartMessage = 0;
	m_iQuitMessage = 0;
	m_iEmuleProto = 0;
	m_iAcceptLinks = 0;
	m_iAcceptLinksFriends = 0;
	m_iHelpChannel = 0;
	m_bInitializedTreeOpts = false;
	m_htiTimeStamp = NULL;
	m_htiSoundEvents = NULL;
	m_htiInfoMessage = NULL;
	m_htiMiscMessage = NULL;
	m_htiJoinMessage = NULL;
	m_htiPartMessage = NULL;
	m_htiQuitMessage = NULL;
	m_htiEmuleProto = NULL;
	m_htiAcceptLinks = NULL;
	m_htiAcceptLinksFriends = NULL;
	m_htiHelpChannel = NULL;
}

CPPgIRC::~CPPgIRC()
{
}

void CPPgIRC::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MISC_IRC, m_ctrlTreeOptions);
	if (!m_bInitializedTreeOpts)
	{
		m_htiSoundEvents = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_SOUNDEVENTS), TVI_ROOT, m_iSoundEvents);
		m_htiHelpChannel = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_HELPCHANNEL), TVI_ROOT, m_iHelpChannel);
		m_htiTimeStamp = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_ADDTIMESTAMP), TVI_ROOT, m_iTimeStamp);
		m_htiInfoMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREINFOMESSAGE), TVI_ROOT, FALSE);
		m_htiMiscMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREMISCMESSAGE), m_htiInfoMessage, m_iMiscMessage);
		m_htiJoinMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREJOINMESSAGE), m_htiInfoMessage, m_iJoinMessage);
		m_htiPartMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREPARTMESSAGE), m_htiInfoMessage, m_iPartMessage);
		m_htiQuitMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREQUITMESSAGE), m_htiInfoMessage, m_iQuitMessage);
		m_htiEmuleProto = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_EMULEPROTO_IGNOREINFOMESSAGE), TVI_ROOT, m_iEmuleProto);
		m_htiAcceptLinks = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_ACCEPTLINKS), TVI_ROOT, m_iAcceptLinks);
		m_htiAcceptLinksFriends = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_ACCEPTLINKSFRIENDS), TVI_ROOT, m_iAcceptLinksFriends);

		m_ctrlTreeOptions.Expand(m_htiInfoMessage, TVE_EXPAND);

		m_ctrlTreeOptions.SendMessage(WM_VSCROLL, SB_TOP);

        m_bInitializedTreeOpts = true;
	}
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiTimeStamp, m_iTimeStamp);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiSoundEvents, m_iSoundEvents);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiMiscMessage, m_iMiscMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiJoinMessage, m_iJoinMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiPartMessage, m_iPartMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiQuitMessage, m_iQuitMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiEmuleProto, m_iEmuleProto);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiAcceptLinks, m_iAcceptLinks);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiAcceptLinksFriends, m_iAcceptLinksFriends);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiHelpChannel, m_iHelpChannel);

	m_ctrlTreeOptions.UpdateCheckBoxGroup(m_htiInfoMessage);
	m_ctrlTreeOptions.SetCheckBoxEnable(m_htiAcceptLinksFriends, m_iAcceptLinks);
}

BOOL CPPgIRC::OnInitDialog()
{
	m_iTimeStamp = thePrefs.GetIRCAddTimestamp();
	m_iInfoMessage = 0;
	m_iSoundEvents = thePrefs.GetIrcSoundEvents();
	m_iMiscMessage = thePrefs.GetIrcIgnoreMiscMessage();
	m_iJoinMessage = thePrefs.GetIrcIgnoreJoinMessage();
	m_iPartMessage = thePrefs.GetIrcIgnorePartMessage();
	m_iQuitMessage = thePrefs.GetIrcIgnoreQuitMessage();
	m_iEmuleProto = thePrefs.GetIrcIgnoreEmuleProtoInfoMessage();
	m_iAcceptLinks = thePrefs.GetIrcAcceptLinks();
	m_iAcceptLinksFriends = thePrefs.GetIrcAcceptLinksFriends();
	m_iHelpChannel = thePrefs.GetIrcHelpChannel();

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

	UpdateControls();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgIRC::OnKillActive()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	return CPropertyPage::OnKillActive();
}

void CPPgIRC::LoadSettings(void)
{	
	if(thePrefs.m_bircusechanfilter)
		CheckDlgButton(IDC_IRC_USECHANFILTER,1);
	else
		CheckDlgButton(IDC_IRC_USECHANFILTER,0);
	
	if(thePrefs.m_bircuseperform)
		CheckDlgButton(IDC_IRC_USEPERFORM,1);
	else
		CheckDlgButton(IDC_IRC_USEPERFORM,0);
	
	if(thePrefs.m_birclistonconnect)
		CheckDlgButton(IDC_IRC_LISTONCONNECT,1);
	else
		CheckDlgButton(IDC_IRC_LISTONCONNECT,0);
	
	GetDlgItem(IDC_IRC_SERVER_BOX)->SetWindowText(thePrefs.m_sircserver);
	GetDlgItem(IDC_IRC_NICK_BOX)->SetWindowText(thePrefs.m_sircnick);
	GetDlgItem(IDC_IRC_NAME_BOX)->SetWindowText(thePrefs.m_sircchannamefilter);
	GetDlgItem(IDC_IRC_PERFORM_BOX)->SetWindowText(thePrefs.m_sircperformstring);
	CString strBuffer;
	strBuffer.Format("%d", thePrefs.m_iircchanneluserfilter);
	GetDlgItem(IDC_IRC_MINUSER_BOX)->SetWindowText(strBuffer);
}

BOOL CPPgIRC::OnApply()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();

	if (!UpdateData())
		return FALSE;

	thePrefs.m_bircaddtimestamp = m_iTimeStamp;
	thePrefs.m_bircsoundevents = m_iSoundEvents;
	thePrefs.m_bircignoremiscmessage = m_iMiscMessage;
	thePrefs.m_bircignorejoinmessage = m_iJoinMessage;
	thePrefs.m_bircignorepartmessage = m_iPartMessage;
	thePrefs.m_bircignorequitmessage = m_iQuitMessage;
	thePrefs.m_bircignoreemuleprotoinfomessage = m_iEmuleProto;
	thePrefs.m_bircacceptlinks = m_iAcceptLinks;
	thePrefs.m_bircacceptlinksfriends = m_iAcceptLinksFriends;
	thePrefs.m_birchelpchannel = m_iHelpChannel;

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
		GetDlgItem(IDC_IRC_NICK_FRM)->SetWindowText(GetResString(IDS_PW_NICK));
		GetDlgItem(IDC_IRC_NAME_TEXT)->SetWindowText(GetResString(IDS_IRC_NAME));
		GetDlgItem(IDC_IRC_MINUSER_TEXT)->SetWindowText(GetResString(IDS_UUSERS));
		GetDlgItem(IDC_IRC_FILTER_FRM)->SetWindowText(GetResString(IDS_IRC_CHANNELLIST));
		GetDlgItem(IDC_IRC_USECHANFILTER)->SetWindowText(GetResString(IDS_IRC_USEFILTER));
		GetDlgItem(IDC_IRC_PERFORM_FRM)->SetWindowText(GetResString(IDS_IRC_PERFORM));
		GetDlgItem(IDC_IRC_USEPERFORM)->SetWindowText(GetResString(IDS_IRC_USEPERFORM));

		if (m_htiSoundEvents) m_ctrlTreeOptions.SetItemText(m_htiSoundEvents, GetResString(IDS_IRC_SOUNDEVENTS));
		if (m_htiTimeStamp) m_ctrlTreeOptions.SetItemText(m_htiTimeStamp, GetResString(IDS_IRC_ADDTIMESTAMP));
		if (m_htiInfoMessage) m_ctrlTreeOptions.SetItemText(m_htiInfoMessage, GetResString(IDS_IRC_IGNOREINFOMESSAGE));
		if (m_htiMiscMessage) m_ctrlTreeOptions.SetItemText(m_htiMiscMessage, GetResString(IDS_IRC_IGNOREMISCMESSAGE));
		if (m_htiJoinMessage) m_ctrlTreeOptions.SetItemText(m_htiJoinMessage, GetResString(IDS_IRC_IGNOREJOINMESSAGE));
		if (m_htiPartMessage) m_ctrlTreeOptions.SetItemText(m_htiPartMessage, GetResString(IDS_IRC_IGNOREPARTMESSAGE));
		if (m_htiQuitMessage) m_ctrlTreeOptions.SetItemText(m_htiQuitMessage, GetResString(IDS_IRC_IGNOREQUITMESSAGE));
		if (m_htiEmuleProto) m_ctrlTreeOptions.SetItemText(m_htiEmuleProto, GetResString(IDS_IRC_EMULEPROTO_IGNOREINFOMESSAGE));
		if (m_htiAcceptLinks) m_ctrlTreeOptions.SetItemText(m_htiAcceptLinks, GetResString(IDS_IRC_ACCEPTLINKS));
		if (m_htiAcceptLinksFriends) m_ctrlTreeOptions.SetItemText(m_htiAcceptLinksFriends, GetResString(IDS_IRC_ACCEPTLINKSFRIENDS));
		if (m_htiHelpChannel) m_ctrlTreeOptions.SetItemText(m_htiHelpChannel, GetResString(IDS_IRC_HELPCHANNEL));
	}
}

void CPPgIRC::OnBtnClickPerform()
{
	SetModified();
	UpdateControls();
}

void CPPgIRC::UpdateControls()
{
	GetDlgItem(IDC_IRC_PERFORM_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USEPERFORM) );
	GetDlgItem(IDC_IRC_NAME_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USECHANFILTER) );
	GetDlgItem(IDC_IRC_MINUSER_BOX)->EnableWindow( IsDlgButtonChecked(IDC_IRC_USECHANFILTER) );
}

void CPPgIRC::OnDestroy()
{
	m_ctrlTreeOptions.DeleteAllItems();
	m_ctrlTreeOptions.DestroyWindow();
	m_bInitializedTreeOpts = false;
	m_htiAcceptLinks = NULL;
	m_htiAcceptLinksFriends = NULL;
	m_htiEmuleProto = NULL;
	m_htiHelpChannel = NULL;
	m_htiSoundEvents = NULL;
	m_htiInfoMessage = NULL;
	m_htiMiscMessage = NULL;
	m_htiJoinMessage = NULL;
	m_htiPartMessage = NULL;
	m_htiQuitMessage = NULL;
	m_htiTimeStamp = NULL;
    CPropertyPage::OnDestroy();
}

LRESULT CPPgIRC::OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDC_MISC_IRC){
		TREEOPTSCTRLNOTIFY* pton = (TREEOPTSCTRLNOTIFY*)lParam;
		if (pton->hItem == m_htiAcceptLinks){
			BOOL bCheck;
			if (m_ctrlTreeOptions.GetCheckBox(m_htiAcceptLinks, bCheck))
				m_ctrlTreeOptions.SetCheckBoxEnable(m_htiAcceptLinksFriends, bCheck);
		}
		SetModified();
	}
	return 0;
}