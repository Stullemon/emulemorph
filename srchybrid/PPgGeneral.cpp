// PPgGeneral.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "SearchDlg.h"
#include "KademliaWnd.h"
#include "PreferencesDlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CPPgGeneral dialog

IMPLEMENT_DYNAMIC(CPPgGeneral, CPropertyPage)
CPPgGeneral::CPPgGeneral()
	: CPropertyPage(CPPgGeneral::IDD)
{
}

CPPgGeneral::~CPPgGeneral()
{
}

void CPPgGeneral::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LANGS, m_language);
}


BEGIN_MESSAGE_MAP(CPPgGeneral, CPropertyPage)
	ON_BN_CLICKED(IDC_STARTMIN, OnSettingsChange)
	ON_EN_CHANGE(IDC_NICK, OnSettingsChange)
	ON_BN_CLICKED(IDC_BEEPER, OnSettingsChange)
	ON_BN_CLICKED(IDC_EXIT, OnSettingsChange)
	ON_BN_CLICKED(IDC_SPLASHON, OnSettingsChange)
	ON_BN_CLICKED(IDC_BRINGTOFOREGROUND, OnSettingsChange)
	ON_CBN_SELCHANGE(IDC_LANGS, OnSettingsChange)
	ON_BN_CLICKED(IDC_ED2KFIX, OnBnClickedEd2kfix)
	ON_BN_CLICKED(IDC_WEBSVEDIT , OnBnClickedEditWebservices)
	ON_BN_CLICKED(IDC_ONLINESIG, OnSettingsChange)
	ON_BN_CLICKED(IDC_CHECK4UPDATE, OnSettingsChange)
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

void CPPgGeneral::LoadSettings(void)
{
	GetDlgItem(IDC_NICK)->SetWindowText(app_prefs->prefs->nick);

	for(int i = 0; i < m_language.GetCount(); i++)
		if(m_language.GetItemData(i) == app_prefs->GetLanguageID())
			m_language.SetCurSel(i);
	
	if(app_prefs->prefs->startMinimized)
		CheckDlgButton(IDC_STARTMIN,1);
	else
		CheckDlgButton(IDC_STARTMIN,0);

	if (app_prefs->prefs->onlineSig)
		CheckDlgButton(IDC_ONLINESIG,1);
	else
		CheckDlgButton(IDC_ONLINESIG,0);
	
	if(app_prefs->prefs->beepOnError)
		CheckDlgButton(IDC_BEEPER,1);
	else
		CheckDlgButton(IDC_BEEPER,0);

	if(app_prefs->prefs->confirmExit)
		CheckDlgButton(IDC_EXIT,1);
	else
		CheckDlgButton(IDC_EXIT,0);

	if(app_prefs->prefs->splashscreen)
		CheckDlgButton(IDC_SPLASHON,1);
	else
		CheckDlgButton(IDC_SPLASHON,0);

	if(app_prefs->prefs->bringtoforeground)
		CheckDlgButton(IDC_BRINGTOFOREGROUND,1);
	else
		CheckDlgButton(IDC_BRINGTOFOREGROUND,0);

	if(app_prefs->prefs->updatenotify)
		CheckDlgButton(IDC_CHECK4UPDATE,1);
	else
		CheckDlgButton(IDC_CHECK4UPDATE,0);

	CString strBuffer;
	strBuffer.Format("%i %s",app_prefs->prefs->versioncheckdays ,GetResString(IDS_DAYS2));
	GetDlgItem(IDC_DAYS)->SetWindowText(strBuffer);
}

BOOL CPPgGeneral::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	((CEdit*)GetDlgItem(IDC_NICK))->SetLimitText(MAX_NICK_LENGTH);
	
	CWordArray aLanguageIDs;
	app_prefs->GetLanguages(aLanguageIDs);
	for (int i = 0; i < aLanguageIDs.GetSize(); i++){
		TCHAR szLang[128];
		GetLocaleInfo(aLanguageIDs[i], LOCALE_SLANGUAGE, szLang, ARRSIZE(szLang));
		m_language.SetItemData(m_language.AddString(szLang), aLanguageIDs[i]);
	}

	GetDlgItem(IDC_ED2KFIX)->EnableWindow(Ask4RegFix(true));

	CSliderCtrl *sliderUpdate = (CSliderCtrl*)GetDlgItem(IDC_CHECKDAYS);
	sliderUpdate->SetRange(2, 7, true);
	sliderUpdate->SetPos(app_prefs->GetUpdateDays());
	
	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgGeneral::OnApply()
{
	if(GetDlgItem(IDC_NICK)->GetWindowTextLength())
	{
		GetDlgItem(IDC_NICK)->GetWindowText(app_prefs->prefs->nick,sizeof app_prefs->prefs->nick);
	}
	if (m_language.GetCurSel() != CB_ERR){
		WORD byNewLang =  m_language.GetItemData(m_language.GetCurSel());
		if (app_prefs->GetLanguageID() != byNewLang){
			app_prefs->SetLanguageID(byNewLang);
			
			theApp.glob_prefs->SetLanguage();

			theApp.emuledlg->preferenceswnd->Localize();
			theApp.emuledlg->statisticswnd.CreateMyTree();
			theApp.emuledlg->statisticswnd.Localize();
			theApp.emuledlg->statisticswnd.ShowStatistics(true);
			theApp.emuledlg->statisticswnd.RepaintMeters(); //MORPH - Added by SiRoB, Due to new Oscope
			theApp.emuledlg->serverwnd.Localize();
			theApp.emuledlg->transferwnd.Localize();
			theApp.emuledlg->transferwnd.UpdateCatTabTitles();
			theApp.emuledlg->searchwnd->Localize();
			theApp.emuledlg->sharedfileswnd.Localize();
			theApp.emuledlg->chatwnd.Localize();
			theApp.emuledlg->Localize();
			theApp.emuledlg->ircwnd.Localize();
			theApp.emuledlg->kademliawnd->Localize();
		}
	}

	app_prefs->prefs->startMinimized= (int8)IsDlgButtonChecked(IDC_STARTMIN);
	app_prefs->prefs->beepOnError= (int8)IsDlgButtonChecked(IDC_BEEPER);
	app_prefs->prefs->confirmExit= (int8)IsDlgButtonChecked(IDC_EXIT);
	app_prefs->prefs->splashscreen = (int8)IsDlgButtonChecked(IDC_SPLASHON);
	app_prefs->prefs->bringtoforeground = (int8)IsDlgButtonChecked(IDC_BRINGTOFOREGROUND);
	app_prefs->prefs->updatenotify = (int8)IsDlgButtonChecked(IDC_CHECK4UPDATE);
	app_prefs->prefs->onlineSig= (int8)IsDlgButtonChecked(IDC_ONLINESIG);
	app_prefs->prefs->versioncheckdays = ((CSliderCtrl*)GetDlgItem(IDC_CHECKDAYS))->GetPos();

	theApp.emuledlg->transferwnd.downloadlistctrl.SetStyle();
	LoadSettings();

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgGeneral::OnBnClickedEd2kfix()
{
	Ask4RegFix(false);
	GetDlgItem(IDC_ED2KFIX)->EnableWindow(Ask4RegFix(true));
}

void CPPgGeneral::Localize(void)
{
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_GENERAL));
		GetDlgItem(IDC_NICK_FRM)->SetWindowText(GetResString(IDS_PW_NICK));
		GetDlgItem(IDC_LANG_FRM)->SetWindowText(GetResString(IDS_PW_LANG));
		GetDlgItem(IDC_MISC_FRM)->SetWindowText(GetResString(IDS_PW_MISC));
		GetDlgItem(IDC_BEEPER)->SetWindowText(GetResString(IDS_PW_BEEP));
		GetDlgItem(IDC_EXIT)->SetWindowText(GetResString(IDS_PW_PROMPT));
		GetDlgItem(IDC_SPLASHON)->SetWindowText(GetResString(IDS_PW_SPLASH));
		GetDlgItem(IDC_BRINGTOFOREGROUND)->SetWindowText(GetResString(IDS_PW_FRONT));
		GetDlgItem(IDC_ONLINESIG)->SetWindowText(GetResString(IDS_PREF_ONLINESIG));	
		GetDlgItem(IDC_STARTMIN)->SetWindowText(GetResString(IDS_PREF_STARTMIN));	
		GetDlgItem(IDC_WEBSVEDIT)->SetWindowText(GetResString(IDS_WEBSVEDIT));
		GetDlgItem(IDC_ED2KFIX)->SetWindowText(GetResString(IDS_ED2KLINKFIX));
		GetDlgItem(IDC_CHECK4UPDATE)->SetWindowText(GetResString(IDS_CHECK4UPDATE));
		GetDlgItem(IDC_STARTUP)->SetWindowText(GetResString(IDS_STARTUP));
	}
}

void CPPgGeneral::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	SetModified(TRUE);

	if (pScrollBar==GetDlgItem(IDC_CHECKDAYS)) {
		CSliderCtrl* slider =(CSliderCtrl*)pScrollBar;
		CString text;
		text.Format("%i %s",slider->GetPos(),GetResString(IDS_DAYS2));
		GetDlgItem(IDC_DAYS)->SetWindowText(text);
	}

	UpdateData(false); 
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPgGeneral::OnBnClickedEditWebservices(){
	ShellExecute(NULL, "open", theApp.glob_prefs->GetTxtEditor(), "\""+CString(theApp.glob_prefs->GetConfigDir())+"webservices.dat\"", NULL, SW_SHOW); 
}