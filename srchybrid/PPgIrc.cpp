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
#include "emule.h"
#include "PPgIRC.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "Preferences.h"
#include "IrcWnd.h"
#include "HelpIDs.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CPPgIRC, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgIRC, CPropertyPage)
	ON_BN_CLICKED(IDC_IRC_USECHANFILTER, OnBtnClickPerform)
	ON_BN_CLICKED(IDC_IRC_USEPERFORM, OnBtnClickPerform)
	ON_EN_CHANGE(IDC_IRC_NICK_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_PERFORM_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_SERVER_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_NAME_BOX, OnSettingsChange)
	ON_EN_CHANGE(IDC_IRC_MINUSER_BOX, OnSettingsChange)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_TREEOPTSCTRL_NOTIFY, OnTreeOptsCtrlNotify)
	ON_WM_HELPINFO()
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
	m_iEmuleAllowAddFriend = 0;
	m_iEmuleAddFriend = 0;
	m_iEmuleSendLink = 0;
	m_iAcceptLinks = 0;
	m_iAcceptLinksFriends = 0;
	m_iHelpChannel = 0;
	m_iChannelsOnConnect = 0;
	m_bInitializedTreeOpts = false;
	m_htiTimeStamp = NULL;
	m_htiSoundEvents = NULL;
	m_htiInfoMessage = NULL;
	m_htiMiscMessage = NULL;
	m_htiJoinMessage = NULL;
	m_htiPartMessage = NULL;
	m_htiQuitMessage = NULL;
	m_htiEmuleProto = NULL;
	m_htiEmuleAddFriend = NULL;
	m_htiEmuleAllowAddFriend = NULL;
	m_htiEmuleSendLink = NULL;
	m_htiAcceptLinks = NULL;
	m_htiAcceptLinksFriends = NULL;
	m_htiHelpChannel = NULL;
	m_htiChannelsOnConnect = NULL;
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
		m_htiChannelsOnConnect = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_LOADCHANNELLISTONCON), TVI_ROOT, m_iChannelsOnConnect);
		m_htiTimeStamp = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_ADDTIMESTAMP), TVI_ROOT, m_iTimeStamp);
		m_htiInfoMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREINFOMESSAGE), TVI_ROOT, FALSE);
		m_htiMiscMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREMISCMESSAGE), m_htiInfoMessage, m_iMiscMessage);
		m_htiJoinMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREJOINMESSAGE), m_htiInfoMessage, m_iJoinMessage);
		m_htiPartMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREPARTMESSAGE), m_htiInfoMessage, m_iPartMessage);
		m_htiQuitMessage = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_IGNOREQUITMESSAGE), m_htiInfoMessage, m_iQuitMessage);
		m_htiEmuleProto = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_EMULEPROTO_IGNOREINFOMESSAGE), TVI_ROOT, FALSE);
		m_htiEmuleAddFriend = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_EMULEPROTO_IGNOREADDFRIEND), m_htiEmuleProto, m_iEmuleAddFriend);
		m_htiEmuleSendLink = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_EMULEPROTO_IGNORESENDLINK), m_htiEmuleProto, m_iEmuleSendLink);
		m_htiEmuleAllowAddFriend = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_EMULEPROTO_ALLOWADDFRIEND), TVI_ROOT, m_iEmuleAllowAddFriend);
		m_htiAcceptLinks = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_ACCEPTLINKS), TVI_ROOT, m_iAcceptLinks);
		m_htiAcceptLinksFriends = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_IRC_ACCEPTLINKSFRIENDS), TVI_ROOT, m_iAcceptLinksFriends);

		m_ctrlTreeOptions.Expand(m_htiInfoMessage, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiEmuleProto, TVE_EXPAND);

		m_ctrlTreeOptions.SendMessage(WM_VSCROLL, SB_TOP);

        m_bInitializedTreeOpts = true;
	}
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiTimeStamp, m_iTimeStamp);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiSoundEvents, m_iSoundEvents);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiMiscMessage, m_iMiscMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiJoinMessage, m_iJoinMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiPartMessage, m_iPartMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiQuitMessage, m_iQuitMessage);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiEmuleAddFriend, m_iEmuleAddFriend);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiEmuleAllowAddFriend, m_iEmuleAllowAddFriend);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiEmuleSendLink, m_iEmuleSendLink);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiAcceptLinks, m_iAcceptLinks);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiAcceptLinksFriends, m_iAcceptLinksFriends);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiHelpChannel, m_iHelpChannel);
	DDX_TreeCheck(pDX, IDC_MISC_IRC, m_htiChannelsOnConnect, m_iChannelsOnConnect);

	m_ctrlTreeOptions.UpdateCheckBoxGroup(m_htiEmuleProto);
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
	m_iEmuleAddFriend = thePrefs.GetIrcIgnoreEmuleProtoAddFriend();
	m_iEmuleAllowAddFriend = thePrefs.GetIrcAllowEmuleProtoAddFriend();
	m_iEmuleSendLink = thePrefs.GetIrcIgnoreEmuleProtoSendLink();
	m_iAcceptLinks = thePrefs.GetIrcAcceptLinks();
	m_iAcceptLinksFriends = thePrefs.GetIrcAcceptLinksFriends();
	m_iHelpChannel = thePrefs.GetIrcHelpChannel();
	m_iChannelsOnConnect = thePrefs.GetIRCListOnConnect();

	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
	((CEdit*)GetDlgItem(IDC_IRC_NICK_BOX))->SetLimitText(20);
	((CEdit*)GetDlgItem(IDC_IRC_MINUSER_BOX))->SetLimitText(6);
	((CEdit*)GetDlgItem(IDC_IRC_SERVER_BOX))->SetLimitText(40);
	((CEdit*)GetDlgItem(IDC_IRC_NAME_BOX))->SetLimitText(40);
	((CEdit*)GetDlgItem(IDC_IRC_PERFORM_BOX))->SetLimitText(250);
	LoadSettings();
	Localize();

	UpdateControls();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgIRC::OnKillActive()
{
	// if prop page is closed by pressing ENTER we have to explicitly commit any possibly pending
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
	
	GetDlgItem(IDC_IRC_SERVER_BOX)->SetWindowText(thePrefs.m_sircserver);
	GetDlgItem(IDC_IRC_NICK_BOX)->SetWindowText(thePrefs.m_sircnick);
	GetDlgItem(IDC_IRC_NAME_BOX)->SetWindowText(thePrefs.m_sircchannamefilter);
	GetDlgItem(IDC_IRC_PERFORM_BOX)->SetWindowText(thePrefs.m_sircperformstring);
	CString strBuffer;
	strBuffer.Format(_T("%d"), thePrefs.m_iircchanneluserfilter);
	GetDlgItem(IDC_IRC_MINUSER_BOX)->SetWindowText(strBuffer);
}

BOOL CPPgIRC::OnApply()
{
	// if prop page is closed by pressing ENTER we have to explicitly commit any possibly pending
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
	thePrefs.m_bircignoreemuleprotoaddfriend = m_iEmuleAddFriend;
	thePrefs.m_bircallowemuleprotoaddfriend = m_iEmuleAllowAddFriend;
	thePrefs.m_bircignoreemuleprotosendlink = m_iEmuleSendLink;
	thePrefs.m_bircacceptlinks = m_iAcceptLinks;
	thePrefs.m_bircacceptlinksfriends = m_iAcceptLinksFriends;
	thePrefs.m_birchelpchannel = m_iHelpChannel;
	thePrefs.m_birclistonconnect = m_iChannelsOnConnect;

	if(IsDlgButtonChecked(IDC_IRC_USECHANFILTER))
		thePrefs.m_bircusechanfilter = true;
	else
		thePrefs.m_bircusechanfilter = false;

	if(IsDlgButtonChecked(IDC_IRC_USEPERFORM))
		thePrefs.m_bircuseperform = true;
	else
		thePrefs.m_bircuseperform = false;

	TCHAR buffer[510];
	if(GetDlgItem(IDC_IRC_NICK_BOX)->GetWindowTextLength() )
	{
		GetDlgItem(IDC_IRC_NICK_BOX)->GetWindowText(buffer,20);
		if( _tcscmp(buffer, thePrefs.m_sircnick ) )
		{
			CString input;
			input.Format(_T("%s"), buffer);
			input.Trim();
			input = input.SpanExcluding(_T(" !@#$%^&*():;<>,.?{}~`+=-"));
			if( input != "" )
			{
				theApp.emuledlg->ircwnd->SendString( (CString)_T("NICK ") + input );
				_tcscpy(thePrefs.m_sircnick,input.GetBuffer());
			}
		}
	}

	if(GetDlgItem(IDC_IRC_SERVER_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_SERVER_BOX)->GetWindowText(buffer,40);
		_tcscpy(thePrefs.m_sircserver,buffer);
	}

	if(GetDlgItem(IDC_IRC_NAME_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_NAME_BOX)->GetWindowText(buffer,40);
		_tcscpy(thePrefs.m_sircchannamefilter,buffer);
	}

	if(GetDlgItem(IDC_IRC_PERFORM_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_PERFORM_BOX)->GetWindowText(buffer,250);
		_tcscpy(thePrefs.m_sircperformstring,buffer);
	}
	else
	{
		_stprintf( buffer, _T(" ") );
		_tcscpy(thePrefs.m_sircperformstring,buffer);
	}

	if(GetDlgItem(IDC_IRC_MINUSER_BOX)->GetWindowTextLength())
	{
		GetDlgItem(IDC_IRC_MINUSER_BOX)->GetWindowText(buffer,6);
		thePrefs.m_iircchanneluserfilter = _tstoi(buffer);
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
		if (m_htiEmuleAddFriend) m_ctrlTreeOptions.SetItemText(m_htiEmuleAddFriend, GetResString(IDS_IRC_EMULEPROTO_IGNOREADDFRIEND));
		if (m_htiEmuleAllowAddFriend) m_ctrlTreeOptions.SetItemText(m_htiEmuleAllowAddFriend, GetResString(IDS_IRC_EMULEPROTO_ALLOWADDFRIEND));
		if (m_htiEmuleSendLink) m_ctrlTreeOptions.SetItemText(m_htiEmuleSendLink, GetResString(IDS_IRC_EMULEPROTO_IGNORESENDLINK));
		if (m_htiAcceptLinks) m_ctrlTreeOptions.SetItemText(m_htiAcceptLinks, GetResString(IDS_IRC_ACCEPTLINKS));
		if (m_htiAcceptLinksFriends) m_ctrlTreeOptions.SetItemText(m_htiAcceptLinksFriends, GetResString(IDS_IRC_ACCEPTLINKSFRIENDS));
		if (m_htiHelpChannel) m_ctrlTreeOptions.SetItemText(m_htiHelpChannel, GetResString(IDS_IRC_HELPCHANNEL));
		if (m_htiChannelsOnConnect) m_ctrlTreeOptions.SetItemText(m_htiChannelsOnConnect, GetResString(IDS_IRC_LOADCHANNELLISTONCON));
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
	m_htiEmuleAddFriend = NULL;
	m_htiEmuleAllowAddFriend = NULL;
	m_htiEmuleSendLink = NULL;
	m_htiHelpChannel = NULL;
	m_htiChannelsOnConnect = NULL;
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

void CPPgIRC::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_IRC);
}

BOOL CPPgIRC::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgIRC::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}
