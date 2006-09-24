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
#include <share.h>
#include "emule.h"
#include "PPgMessages.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "emuledlg.h"
#include "HelpIDs.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



IMPLEMENT_DYNAMIC(CPPgMessages, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgMessages, CPropertyPage)
	ON_EN_CHANGE(IDC_FILTER, OnSettingsChange)
	ON_EN_CHANGE(IDC_COMMENTFILTER, OnSettingsChange)
	ON_BN_CLICKED(IDC_MSGONLYFRIENDS , OnSettingsChange) 
	ON_BN_CLICKED(IDC_MSGONLYSEC, OnSettingsChange)
	ON_BN_CLICKED(IDC_ADVSPAMFILTER , OnSettingsChange)
	ON_BN_CLICKED(IDC_INDICATERATINGS , OnSettingsChange)
	ON_WM_HELPINFO()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CPPgMessages::CPPgMessages()
	: CPropertyPage(CPPgMessages::IDD)
{
}

CPPgMessages::~CPPgMessages()
{
}

void CPPgMessages::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

void CPPgMessages::LoadSettings(void)
{
	CString strBuffer;

	if (thePrefs.msgonlyfriends)
		CheckDlgButton(IDC_MSGONLYFRIENDS,1);
	else
		CheckDlgButton(IDC_MSGONLYFRIENDS,0);

	if (thePrefs.msgsecure)
		CheckDlgButton(IDC_MSGONLYSEC,1);
	else
		CheckDlgButton(IDC_MSGONLYSEC,0);

	if (thePrefs.m_bAdvancedSpamfilter)
		CheckDlgButton(IDC_ADVSPAMFILTER,1);
	else
		CheckDlgButton(IDC_ADVSPAMFILTER,0);

	if(thePrefs.indicateratings)
		CheckDlgButton(IDC_INDICATERATINGS,1);
	else
		CheckDlgButton(IDC_INDICATERATINGS,0);

	GetDlgItem(IDC_FILTER)->SetWindowText(thePrefs.messageFilter);
	GetDlgItem(IDC_COMMENTFILTER)->SetWindowText(thePrefs.commentFilter);
}

BOOL CPPgMessages::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgMessages::OnApply()
{

	thePrefs.msgonlyfriends = IsDlgButtonChecked(IDC_MSGONLYFRIENDS)!=0;
	thePrefs.msgsecure = IsDlgButtonChecked(IDC_MSGONLYSEC)!=0;
	thePrefs.m_bAdvancedSpamfilter = IsDlgButtonChecked(IDC_ADVSPAMFILTER)!=0;
	thePrefs.indicateratings = IsDlgButtonChecked(IDC_INDICATERATINGS)!=0;

	GetDlgItem(IDC_FILTER)->GetWindowText(thePrefs.messageFilter);

	CString strCommentFilters;
	GetDlgItem(IDC_COMMENTFILTER)->GetWindowText(strCommentFilters);
	strCommentFilters.MakeLower();
	CString strNewCommentFilters;
	int curPos = 0;
	CString strFilter(strCommentFilters.Tokenize(_T("|"), curPos));
	while (!strFilter.IsEmpty())
	{
		strFilter.Trim();
		if (!strNewCommentFilters.IsEmpty())
			strNewCommentFilters += _T('|');
		strNewCommentFilters += strFilter;
		strFilter = strCommentFilters.Tokenize(_T("|"), curPos);
	}
	thePrefs.commentFilter = strNewCommentFilters;
	if (thePrefs.commentFilter != strCommentFilters)
		SetDlgItemText(IDC_COMMENTFILTER, thePrefs.commentFilter);

	LoadSettings();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgMessages::Localize(void)
{
	if (m_hWnd)
	{
		SetWindowText(GetResString(IDS_MESSAGESCOMMENTS));

		GetDlgItem(IDC_FILTERCOMMENTSLABEL)->SetWindowText(GetResString(IDS_FILTERCOMMENTSLABEL));
		GetDlgItem(IDC_STATIC_COMMENTS)->SetWindowText(GetResString(IDS_COMMENT));
		GetDlgItem(IDC_INDICATERATINGS)->SetWindowText(GetResString(IDS_INDICATERATINGS));

		GetDlgItem(IDC_FILTERLABEL)->SetWindowText(GetResString(IDS_FILTERLABEL));
		GetDlgItem(IDC_MSG)->SetWindowText(GetResString(IDS_CW_MESSAGES));

		GetDlgItem(IDC_MSGONLYFRIENDS)->SetWindowText(GetResString(IDS_MSGONLYFRIENDS));
		GetDlgItem(IDC_MSGONLYSEC)->SetWindowText(GetResString(IDS_MSGONLYSEC));

		GetDlgItem(IDC_ADVSPAMFILTER)->SetWindowText(GetResString(IDS_ADVSPAMFILTER));
	}
}


void CPPgMessages::OnDestroy()
{
	CPropertyPage::OnDestroy();
}

BOOL CPPgMessages::PreTranslateMessage(MSG* pMsg) 
{  
	return CPropertyPage::PreTranslateMessage(pMsg);
}


void CPPgMessages::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Security);
}

BOOL CPPgMessages::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgMessages::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}