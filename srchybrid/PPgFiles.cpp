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
#include "PPgFiles.h"
#include "Inputbox.h"
#include "OtherFunctions.h"
#include "TransferWnd.h"
#include "emuledlg.h"
#include "Preferences.h"
#include "HelpIDs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


IMPLEMENT_DYNAMIC(CPPgFiles, CPropertyPage)
CPPgFiles::CPPgFiles()
	: CPropertyPage(CPPgFiles::IDD)
{
}

CPPgFiles::~CPPgFiles()
{
}

void CPPgFiles::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPPgFiles, CPropertyPage)
	ON_BN_CLICKED(IDC_SEESHARE1, OnSettingsChange)
	ON_BN_CLICKED(IDC_SEESHARE2, OnSettingsChange)
	ON_BN_CLICKED(IDC_SEESHARE3, OnSettingsChange)
	ON_BN_CLICKED(IDC_PF_TIMECALC, OnSettingsChange)
	ON_BN_CLICKED(IDC_UAP, OnSettingsChange)
	ON_BN_CLICKED(IDC_DAP, OnSettingsChange)
	ON_BN_CLICKED(IDC_PREVIEWPRIO, OnSettingsChange)
	ON_BN_CLICKED(IDC_ADDNEWFILESPAUSED, OnSettingsChange)
	ON_BN_CLICKED(IDC_FULLCHUNKTRANS, OnSettingsChange)
	ON_BN_CLICKED(IDC_STARTNEXTFILE, OnSettingsChange)
	ON_BN_CLICKED(IDC_WATCHCB, OnSettingsChange)
	ON_BN_CLICKED(IDC_STARTNEXTFILECAT, OnSettingsChangeCat1)
	ON_BN_CLICKED(IDC_STARTNEXTFILECAT2, OnSettingsChangeCat2)
	ON_BN_CLICKED(IDC_FNCLEANUP, OnSettingsChange)
	ON_BN_CLICKED(IDC_FNC, OnSetCleanupFilter)
	ON_EN_CHANGE(IDC_VIDEOPLAYER, OnSettingsChange)
	ON_BN_CLICKED(IDC_VIDEOBACKUP, OnSettingsChange)
	ON_BN_CLICKED(IDC_BROWSEV, BrowseVideoplayer)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

BOOL CPPgFiles::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
	//MORPH START - Added by SiRoB, Allways use transfertfull chunk
	GetDlgItem(IDC_FULLCHUNKTRANS)->EnableWindow(0);
	//MORPH END   - Added by SiRoB, Allways use transfertfull chunk
	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgFiles::LoadSettings(void)
{	
	ASSERT( vsfaEverybody == 0 );
	ASSERT( vsfaFriends == 1 );
	ASSERT( vsfaNobody == 2 );
	CheckRadioButton(IDC_SEESHARE1, IDC_SEESHARE3, IDC_SEESHARE1 + thePrefs.m_iSeeShares);

	if(thePrefs.addnewfilespaused)
		CheckDlgButton(IDC_ADDNEWFILESPAUSED,1);
	else
		CheckDlgButton(IDC_ADDNEWFILESPAUSED,0);
	
	if(thePrefs.m_bUseOldTimeRemaining)
		CheckDlgButton(IDC_PF_TIMECALC,0);
	else
		CheckDlgButton(IDC_PF_TIMECALC,1);

	if(thePrefs.m_bpreviewprio)
		CheckDlgButton(IDC_PREVIEWPRIO,1);
	else
		CheckDlgButton(IDC_PREVIEWPRIO,0);

	if(thePrefs.m_bDAP)
		CheckDlgButton(IDC_DAP,1);
	else
		CheckDlgButton(IDC_DAP,0);

	if(thePrefs.m_bUAP)
		CheckDlgButton(IDC_UAP,1);
	else
		CheckDlgButton(IDC_UAP,0);

	if(thePrefs.m_btransferfullchunks)
		CheckDlgButton(IDC_FULLCHUNKTRANS,1);
	else
		CheckDlgButton(IDC_FULLCHUNKTRANS,0);

	CheckDlgButton( IDC_STARTNEXTFILECAT, FALSE);
	CheckDlgButton( IDC_STARTNEXTFILECAT2, FALSE);
	if(thePrefs.m_istartnextfile) {
		CheckDlgButton(IDC_STARTNEXTFILE,1);
		if (thePrefs.m_istartnextfile==2)
			CheckDlgButton( IDC_STARTNEXTFILECAT, TRUE);
		else if (thePrefs.m_istartnextfile==3)
			CheckDlgButton( IDC_STARTNEXTFILECAT2, TRUE);
	}
	else
		CheckDlgButton(IDC_STARTNEXTFILE,0);

	GetDlgItem(IDC_VIDEOPLAYER)->SetWindowText(thePrefs.VideoPlayer);
	if(thePrefs.moviePreviewBackup)
		CheckDlgButton(IDC_VIDEOBACKUP,1);
	else
		CheckDlgButton(IDC_VIDEOBACKUP,0);

	CheckDlgButton(IDC_FNCLEANUP, (uint8)thePrefs.AutoFilenameCleanup());

	if(thePrefs.watchclipboard)
		CheckDlgButton(IDC_WATCHCB,1);
	else
		CheckDlgButton(IDC_WATCHCB,0);
	
	GetDlgItem(IDC_STARTNEXTFILECAT)->EnableWindow(IsDlgButtonChecked(IDC_STARTNEXTFILE));
}

BOOL CPPgFiles::OnApply()
{
	CString buffer;

	if(IsDlgButtonChecked(IDC_SEESHARE1))
		thePrefs.m_iSeeShares = vsfaEverybody;
	else if(IsDlgButtonChecked(IDC_SEESHARE2))
		thePrefs.m_iSeeShares = vsfaFriends;
	else
		thePrefs.m_iSeeShares = vsfaNobody;

    bool bOldPreviewPrio = thePrefs.m_bpreviewprio;
	if(IsDlgButtonChecked(IDC_PREVIEWPRIO))
		thePrefs.m_bpreviewprio = true;
	else
		thePrefs.m_bpreviewprio = false;

    if(bOldPreviewPrio != thePrefs.m_bpreviewprio) {
		theApp.emuledlg->transferwnd->downloadlistctrl.CreateMenues();
    }

	if(IsDlgButtonChecked(IDC_DAP))
		thePrefs.m_bDAP = true;
	else
		thePrefs.m_bDAP = false;

	if(IsDlgButtonChecked(IDC_UAP))
		thePrefs.m_bUAP = true;
	else
		thePrefs.m_bUAP = false;

	if(IsDlgButtonChecked(IDC_STARTNEXTFILE)) {
		thePrefs.m_istartnextfile = 1;
		if (IsDlgButtonChecked(IDC_STARTNEXTFILECAT))
			thePrefs.m_istartnextfile = 2;
		else if (IsDlgButtonChecked(IDC_STARTNEXTFILECAT2))
			thePrefs.m_istartnextfile = 3;
	}
	else
		thePrefs.m_istartnextfile = 0;

//	thePrefs.resumeSameCat= (uint8)IsDlgButtonChecked(IDC_STARTNEXTFILECAT);

	if(IsDlgButtonChecked(IDC_FULLCHUNKTRANS))
		thePrefs.m_btransferfullchunks = true;
	else
		thePrefs.m_btransferfullchunks = false;


	if(IsDlgButtonChecked(IDC_WATCHCB))
		thePrefs.watchclipboard = true;
	else
		thePrefs.watchclipboard = false;

	thePrefs.addnewfilespaused = (uint8)IsDlgButtonChecked(IDC_ADDNEWFILESPAUSED);
	thePrefs.autofilenamecleanup=(uint8)IsDlgButtonChecked(IDC_FNCLEANUP);

	thePrefs.m_bUseOldTimeRemaining = !IsDlgButtonChecked(IDC_PF_TIMECALC);

	GetDlgItem(IDC_VIDEOPLAYER)->GetWindowText(buffer);
	_sntprintf(thePrefs.VideoPlayer, ARRSIZE(thePrefs.VideoPlayer), _T("%s"), buffer);

	thePrefs.moviePreviewBackup = IsDlgButtonChecked(IDC_VIDEOBACKUP);

	LoadSettings();

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgFiles::Localize(void)
{
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_FILES));
		GetDlgItem(IDC_PF_TIMECALC)->SetWindowText(GetResString(IDS_PF_ADVANCEDCALC));
		GetDlgItem(IDC_SEEMYSHARE_FRM)->SetWindowText(GetResString(IDS_PW_SHARE));
		GetDlgItem(IDC_SEESHARE1)->SetWindowText(GetResString(IDS_PW_EVER));
		GetDlgItem(IDC_SEESHARE2)->SetWindowText(GetResString(IDS_PW_FRIENDS));
		GetDlgItem(IDC_SEESHARE3)->SetWindowText(GetResString(IDS_PW_NOONE));
		GetDlgItem(IDC_UAP)->SetWindowText(GetResString(IDS_PW_UAP));
		GetDlgItem(IDC_DAP)->SetWindowText(GetResString(IDS_PW_DAP));
		GetDlgItem(IDC_PREVIEWPRIO)->SetWindowText(GetResString(IDS_DOWNLOADMOVIECHUNKS));
		GetDlgItem(IDC_ADDNEWFILESPAUSED)->SetWindowText(GetResString(IDS_ADDNEWFILESPAUSED));
		GetDlgItem(IDC_WATCHCB)->SetWindowText(GetResString(IDS_PF_WATCHCB));
		GetDlgItem(IDC_FULLCHUNKTRANS)->SetWindowText(GetResString(IDS_FULLCHUNKTRANS));
		GetDlgItem(IDC_STARTNEXTFILE)->SetWindowText(GetResString(IDS_STARTNEXTFILE));
		GetDlgItem(IDC_STARTNEXTFILECAT)->SetWindowText(GetResString(IDS_PREF_STARTNEXTFILECAT));
		GetDlgItem(IDC_STARTNEXTFILECAT2)->SetWindowText(GetResString(IDS_PREF_STARTNEXTFILECATONLY));
		GetDlgItem(IDC_FNC)->SetWindowText(GetResString(IDS_EDIT));
		GetDlgItem(IDC_ONND)->SetWindowText(GetResString(IDS_ONNEWDOWNLOAD));
		GetDlgItem(IDC_FNCLEANUP)->SetWindowText(GetResString(IDS_AUTOCLEANUPFN));

		GetDlgItem(IDC_STATICVIDEOPLAYER)->SetWindowText(GetResString(IDS_PW_VIDEOPLAYER));
		GetDlgItem(IDC_VIDEOBACKUP)->SetWindowText(GetResString(IDS_VIDEOBACKUP));		
		GetDlgItem(IDC_STATIC_EMPTY)->SetWindowText(GetResString(IDS_STATIC_EMPTY));
		GetDlgItem(IDC_BROWSEV)->SetWindowText(GetResString(IDS_PW_BROWSE));
	}
}

void CPPgFiles::OnSetCleanupFilter()
{
	CString prompt=GetResString(IDS_FILTERFILENAMEWORD);
	InputBox inputbox;
	inputbox.SetLabels(GetResString(IDS_FNFILTERTITLE),prompt,thePrefs.GetFilenameCleanups());
	inputbox.DoModal();
	if (!inputbox.WasCancelled())
		thePrefs.SetFilenameCleanups(inputbox.GetInput());
}

void CPPgFiles::BrowseVideoplayer()
{
	CString strPlayerPath;
	GetDlgItemText(IDC_VIDEOPLAYER, strPlayerPath);
	CFileDialog dlgFile(TRUE, _T("exe"), strPlayerPath,OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, _T("Executable (*.exe)|*.exe||"), NULL, 0);
	if (dlgFile.DoModal()==IDOK){
		GetDlgItem(IDC_VIDEOPLAYER)->SetWindowText(dlgFile.GetPathName());
		SetModified();
	}
}

void CPPgFiles::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Files);
}

BOOL CPPgFiles::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgFiles::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}

void CPPgFiles::OnSettingsChange() {
		SetModified();
		GetDlgItem(IDC_STARTNEXTFILECAT)->EnableWindow( IsDlgButtonChecked(IDC_STARTNEXTFILE) );
		GetDlgItem(IDC_STARTNEXTFILECAT2)->EnableWindow( IsDlgButtonChecked(IDC_STARTNEXTFILE) );
}

void CPPgFiles::OnSettingsChangeCat(uint8 index) {

	bool on=IsDlgButtonChecked( index==1?IDC_STARTNEXTFILECAT:IDC_STARTNEXTFILECAT2 );
	if (on)
		CheckDlgButton( index==1?IDC_STARTNEXTFILECAT2:IDC_STARTNEXTFILECAT , FALSE);

	OnSettingsChange();
}
