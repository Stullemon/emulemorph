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
#include "PPgNotify.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "Preferences.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CPPgNotify, CPropertyPage)

CPPgNotify::CPPgNotify()
	: CPropertyPage(CPPgNotify::IDD)
{
}

CPPgNotify::~CPPgNotify()
{
}

void CPPgNotify::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

void CPPgNotify::LoadSettings(void)
{
	if (app_prefs->prefs->useDownloadNotifier) 
		CheckDlgButton(IDC_CB_TBN_ONDOWNLOAD, BST_CHECKED);
	if (app_prefs->prefs->useNewDownloadNotifier) 
		CheckDlgButton(IDC_CB_TBN_ONNEWDOWNLOAD, BST_CHECKED);
	if (app_prefs->prefs->useChatNotifier)  
		CheckDlgButton(IDC_CB_TBN_ONCHAT, BST_CHECKED);
	if (app_prefs->prefs->useSoundInNotifier)
		CheckDlgButton(IDC_CB_TBN_USESOUND, BST_CHECKED);
	if (app_prefs->prefs->useLogNotifier)
		CheckDlgButton(IDC_CB_TBN_ONLOG, BST_CHECKED);
	if (app_prefs->prefs->notifierPopsEveryChatMsg)
		CheckDlgButton(IDC_CB_TBN_POP_ALWAYS, BST_CHECKED);
	if (app_prefs->prefs->notifierImportantError) 
		CheckDlgButton(IDC_CB_TBN_IMPORTATNT, BST_CHECKED);
	if (app_prefs->prefs->notifierNewVersion) 
		CheckDlgButton(IDC_CB_TBN_ONNEWVERSION, BST_CHECKED);
	
	CButton* btnPTR = (CButton*) GetDlgItem(IDC_CB_TBN_POP_ALWAYS);
	btnPTR->EnableWindow(IsDlgButtonChecked(IDC_CB_TBN_ONCHAT));
	CEdit* editPtr = (CEdit*) GetDlgItem(IDC_EDIT_TBN_WAVFILE);
	editPtr->SetWindowText(LPCTSTR(app_prefs->prefs->notifierSoundFilePath));
	GetDlgItem(IDC_EDIT_TBN_WAVFILE)->EnableWindow(IsDlgButtonChecked(IDC_CB_TBN_USESOUND));
	GetDlgItem(IDC_BTN_BROWSE_WAV)->EnableWindow(IsDlgButtonChecked(IDC_CB_TBN_USESOUND));
}

void CPPgNotify::Localize(void)
{
	if (m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_EKDEV_OPTIONS));
		GetDlgItem(IDC_CB_TBN_USESOUND)->SetWindowText(GetResString(IDS_PW_TBN_USESOUND));
		GetDlgItem(IDC_BTN_BROWSE_WAV)->SetWindowText(GetResString(IDS_PW_BROWSE));
		GetDlgItem(IDC_CB_TBN_ONLOG)->SetWindowText(GetResString(IDS_PW_TBN_ONLOG));
		GetDlgItem(IDC_CB_TBN_ONCHAT)->SetWindowText(GetResString(IDS_PW_TBN_ONCHAT));
		GetDlgItem(IDC_CB_TBN_POP_ALWAYS)->SetWindowText(GetResString(IDS_PW_TBN_POP_ALWAYS));
		GetDlgItem(IDC_CB_TBN_ONDOWNLOAD)->SetWindowText(GetResString(IDS_PW_TBN_ONDOWNLOAD));
		GetDlgItem(IDC_CB_TBN_ONNEWDOWNLOAD)->SetWindowText(GetResString(IDS_TBN_ONNEWDOWNLOAD));
		GetDlgItem(IDC_TASKBARNOTIFIER)->SetWindowText(GetResString(IDS_PW_TASKBARNOTIFIER));
		GetDlgItem(IDC_CB_TBN_IMPORTATNT)->SetWindowText(GetResString(IDS_PS_TBN_IMPORTANT));
		GetDlgItem(IDC_CB_TBN_ONNEWVERSION)->SetWindowText(GetResString(IDS_CB_TBN_ONNEWVERSION));
		GetDlgItem(IDC_TBN_OPTIONS)->SetWindowText(GetResString(IDS_PW_TBN_OPTIONS));
	}
}

BEGIN_MESSAGE_MAP(CPPgNotify, CPropertyPage)
	ON_BN_CLICKED(IDC_CB_TBN_USESOUND, OnBnClickedUseSound)
	ON_BN_CLICKED(IDC_CB_TBN_ONLOG, OnSettingsChange)
	ON_BN_CLICKED(IDC_CB_TBN_ONCHAT, OnBnClickedCbTbnOnchat)
	ON_BN_CLICKED(IDC_CB_TBN_POP_ALWAYS, OnSettingsChange)
	ON_BN_CLICKED(IDC_CB_TBN_ONDOWNLOAD, OnSettingsChange)
	ON_BN_CLICKED(IDC_CB_TBN_IMPORTATNT , OnSettingsChange)
	ON_BN_CLICKED(IDC_BTN_BROWSE_WAV, OnBnClickedBtnBrowseWav)
	ON_BN_CLICKED(IDC_CB_TBN_ONNEWVERSION, OnSettingsChange)
	ON_EN_CHANGE(IDC_EDIT_TBN_WAVFILE, OnSettingsChange)
	ON_BN_CLICKED(IDC_CB_TBN_ONNEWDOWNLOAD, OnSettingsChange)
END_MESSAGE_MAP()


BOOL CPPgNotify::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgNotify::OnApply()
{
    app_prefs->prefs->useDownloadNotifier = IsDlgButtonChecked(IDC_CB_TBN_ONDOWNLOAD);
    app_prefs->prefs->useNewDownloadNotifier = IsDlgButtonChecked(IDC_CB_TBN_ONNEWDOWNLOAD);
    app_prefs->prefs->useChatNotifier = IsDlgButtonChecked(IDC_CB_TBN_ONCHAT);
    app_prefs->prefs->useLogNotifier = IsDlgButtonChecked(IDC_CB_TBN_ONLOG);        
    app_prefs->prefs->useSoundInNotifier = IsDlgButtonChecked(IDC_CB_TBN_USESOUND);
    app_prefs->prefs->notifierPopsEveryChatMsg = IsDlgButtonChecked(IDC_CB_TBN_POP_ALWAYS);
	app_prefs->prefs->notifierImportantError = IsDlgButtonChecked(IDC_CB_TBN_IMPORTATNT);
	app_prefs->prefs->notifierNewVersion = IsDlgButtonChecked(IDC_CB_TBN_ONNEWVERSION);

	GetDlgItemText(IDC_EDIT_TBN_WAVFILE, app_prefs->prefs->notifierSoundFilePath, ARRSIZE(app_prefs->prefs->notifierSoundFilePath));
    
	//SaveConfiguration();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgNotify::OnBnClickedCbTbnOnchat()
{
    GetDlgItem(IDC_CB_TBN_POP_ALWAYS)->EnableWindow(IsDlgButtonChecked(IDC_CB_TBN_ONCHAT));	
	SetModified();
}

void CPPgNotify::OnBnClickedBtnBrowseWav()
{
	CString strWavPath;
	GetDlgItemText(IDC_EDIT_TBN_WAVFILE, strWavPath);
	CString buffer;
    if (DialogBrowseFile(buffer, _T("Audio-Wav (*.wav)|*.wav||"), strWavPath)){
		SetDlgItemText(IDC_EDIT_TBN_WAVFILE, buffer);
		SetModified();
	}
}

void CPPgNotify::OnBnClickedUseSound()
{
	GetDlgItem(IDC_EDIT_TBN_WAVFILE)->EnableWindow(IsDlgButtonChecked(IDC_CB_TBN_USESOUND));
	GetDlgItem(IDC_BTN_BROWSE_WAV)->EnableWindow(IsDlgButtonChecked(IDC_CB_TBN_USESOUND));
}
