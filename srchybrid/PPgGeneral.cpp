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
#include "SearchDlg.h"
#include "PreferencesDlg.h"
#include "ppggeneral.h"
#include "HttpDownloadDlg.h"
#include "version.h"
#include "Preferences.h"
#include "emuledlg.h"
#include "StatisticsDlg.h"
#include "ServerWnd.h"
#include "TransferWnd.h"
#include "ChatWnd.h"
#include "SharedFilesWnd.h"
#include "KademliaWnd.h"
#include "IrcWnd.h"
#include "WebServices.h"
#include "HelpIDs.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


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
	ON_CBN_SELCHANGE(IDC_LANGS, OnLangChange)
	ON_BN_CLICKED(IDC_ED2KFIX, OnBnClickedEd2kfix)
	ON_BN_CLICKED(IDC_WEBSVEDIT , OnBnClickedEditWebservices)
	ON_BN_CLICKED(IDC_ONLINESIG, OnSettingsChange)
	ON_BN_CLICKED(IDC_CHECK4UPDATE, OnBnClickedCheck4Update)
        //Commander - Added: Invisible Mode [TPT] - Start
	ON_CBN_SELCHANGE(IDC_INVISIBLE_MODE_SELECT_COMBO, OnSettingsChange)
	ON_CBN_SELCHANGE(IDC_INVISIBLE_MODE_KEY_COMBO, OnCbnSelchangeKeymodcombo)
	ON_BN_CLICKED(IDC_INVISIBLE_MODE, OnBoxesChange)
	//Commander - Added: Invisible Mode [TPT] - End
	ON_WM_HSCROLL()
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

void CPPgGeneral::LoadSettings(void)
{
	//Commander - Added: Invisible Mode [TPT] - Start
	m_iActualKeyModifier = thePrefs.GetInvisibleModeHKKeyModifier();
	((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_SELECT_COMBO))->SelectString(-1, CString(thePrefs.GetInvisibleModeHKKey()));
	if (!thePrefs.GetInvisibleMode()){
		GetDlgItem(IDC_INVISIBLE_MODE_SELECT_STATIC)->EnableWindow(false);
		GetDlgItem(IDC_INVISIBLE_MODE_MODIFIER_STATIC)->EnableWindow(false);
		GetDlgItem(IDC_INVISIBLE_MODE_KEY_STATIC)->EnableWindow(false);
		GetDlgItem(IDC_INVISIBLE_MODE_SYMBOL_STATIC)->EnableWindow(false);
		GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO)->EnableWindow(false);
		GetDlgItem(IDC_INVISIBLE_MODE_SELECT_COMBO)->EnableWindow(false);
		CheckDlgButton(IDC_INVISIBLE_MODE, 0);
	} else 
		CheckDlgButton(IDC_INVISIBLE_MODE, 1);
	//Commander - Added: Invisible Mode [TPT] - End

	USES_CONVERSION;
	GetDlgItem(IDC_NICK)->SetWindowText(A2CT(thePrefs.GetUserNick()));

	for(int i = 0; i < m_language.GetCount(); i++)
		if(m_language.GetItemData(i) == thePrefs.GetLanguageID())
			m_language.SetCurSel(i);
	
	if(thePrefs.startMinimized)
		CheckDlgButton(IDC_STARTMIN,1);
	else
		CheckDlgButton(IDC_STARTMIN,0);

	if (thePrefs.onlineSig)
		CheckDlgButton(IDC_ONLINESIG,1);
	else
		CheckDlgButton(IDC_ONLINESIG,0);
	
	if(thePrefs.beepOnError)
		CheckDlgButton(IDC_BEEPER,1);
	else
		CheckDlgButton(IDC_BEEPER,0);

	if(thePrefs.confirmExit)
		CheckDlgButton(IDC_EXIT,1);
	else
		CheckDlgButton(IDC_EXIT,0);

	if(thePrefs.splashscreen)
		CheckDlgButton(IDC_SPLASHON,1);
	else
		CheckDlgButton(IDC_SPLASHON,0);

	if(thePrefs.bringtoforeground)
		CheckDlgButton(IDC_BRINGTOFOREGROUND,1);
	else
		CheckDlgButton(IDC_BRINGTOFOREGROUND,0);

	if(thePrefs.updatenotify)
		CheckDlgButton(IDC_CHECK4UPDATE,1);
	else
		CheckDlgButton(IDC_CHECK4UPDATE,0);

	CString strBuffer;
	strBuffer.Format(_T("%i %s"),thePrefs.versioncheckdays,GetResString(IDS_DAYS2));
	GetDlgItem(IDC_DAYS)->SetWindowText(strBuffer);
}

BOOL CPPgGeneral::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	((CEdit*)GetDlgItem(IDC_NICK))->SetLimitText(thePrefs.GetMaxUserNickLength());

	CWordArray aLanguageIDs;
	thePrefs.GetLanguages(aLanguageIDs);
	for (int i = 0; i < aLanguageIDs.GetSize(); i++){
		TCHAR szLang[128];
		GetLocaleInfo(aLanguageIDs[i], LOCALE_SLANGUAGE, szLang, ARRSIZE(szLang));
		m_language.SetItemData(m_language.AddString(szLang), aLanguageIDs[i]);
	}

	UpdateEd2kLinkFixCtrl();

	CSliderCtrl *sliderUpdate = (CSliderCtrl*)GetDlgItem(IDC_CHECKDAYS);
	sliderUpdate->SetRange(2, 7, true);
	sliderUpdate->SetPos(thePrefs.GetUpdateDays());
	
	//Commander - Added: Invisible Mode [TPT] - Start
	// Add keys to ComboBox
	for(int i='A'; i<='Z'; i++)
		((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_SELECT_COMBO))->AddString(_T(CString((char)(i))));
	for(int i='0'; i<='9'; i++)
		((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_SELECT_COMBO))->AddString(_T(CString((char)(i))));
	//Commander - Added: Invisible Mode [TPT] - End

	LoadSettings();
	Localize();
	GetDlgItem(IDC_CHECKDAYS)->ShowWindow( IsDlgButtonChecked(IDC_CHECK4UPDATE) ? SW_SHOW : SW_HIDE );
	GetDlgItem(IDC_DAYS)->ShowWindow( IsDlgButtonChecked(IDC_CHECK4UPDATE) ? SW_SHOW : SW_HIDE );

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgGeneral::OnApply()
{
	//Commander - Added: Invisible Mode [TPT] - Start
	CString sKey;
	((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_SELECT_COMBO))->GetLBText(((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_SELECT_COMBO))->GetCurSel(), sKey);
	if (IsDlgButtonChecked(IDC_INVISIBLE_MODE))
		thePrefs.SetInvisibleMode(true,m_iActualKeyModifier,sKey[0]);
	else
		thePrefs.SetInvisibleMode(false,m_iActualKeyModifier,sKey[0]);
	//Commander - Added: Invisible Mode [TPT] - End

	CString strNick;
	GetDlgItem(IDC_NICK)->GetWindowText(strNick);
	strNick.Trim();
	if (strNick.IsEmpty())
	{
		strNick = DEFAULT_NICK;
		GetDlgItem(IDC_NICK)->SetWindowText(strNick);
	}
	thePrefs.SetUserNick(strNick);

	if (m_language.GetCurSel() != CB_ERR){
		WORD byNewLang =  m_language.GetItemData(m_language.GetCurSel());
		if (thePrefs.GetLanguageID() != byNewLang){
			thePrefs.SetLanguageID(byNewLang);
		
			thePrefs.SetLanguage();

			theApp.emuledlg->preferenceswnd->Localize();
			theApp.emuledlg->statisticswnd->CreateMyTree();
			theApp.emuledlg->statisticswnd->Localize();
			theApp.emuledlg->statisticswnd->ShowStatistics(true);
			theApp.emuledlg->statisticswnd->RepaintMeters(); //MORPH - Added by SiRoB, Due to new graph
			theApp.emuledlg->serverwnd->Localize();
			theApp.emuledlg->transferwnd->Localize();
			theApp.emuledlg->transferwnd->UpdateCatTabTitles();
			theApp.emuledlg->searchwnd->Localize();
			theApp.emuledlg->sharedfileswnd->Localize();
			theApp.emuledlg->chatwnd->Localize();
			theApp.emuledlg->Localize();
			theApp.emuledlg->ircwnd->Localize();
			theApp.emuledlg->kademliawnd->Localize();
		}
	}

	thePrefs.startMinimized= (uint8)IsDlgButtonChecked(IDC_STARTMIN);
	thePrefs.beepOnError= (uint8)IsDlgButtonChecked(IDC_BEEPER);
	thePrefs.confirmExit= (uint8)IsDlgButtonChecked(IDC_EXIT);
	thePrefs.splashscreen = (uint8)IsDlgButtonChecked(IDC_SPLASHON);
	thePrefs.bringtoforeground = (uint8)IsDlgButtonChecked(IDC_BRINGTOFOREGROUND);
	thePrefs.updatenotify = (uint8)IsDlgButtonChecked(IDC_CHECK4UPDATE);
	thePrefs.onlineSig= (uint8)IsDlgButtonChecked(IDC_ONLINESIG);
	thePrefs.versioncheckdays = ((CSliderCtrl*)GetDlgItem(IDC_CHECKDAYS))->GetPos();

	theApp.emuledlg->transferwnd->downloadlistctrl.SetStyle();
	LoadSettings();

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgGeneral::UpdateEd2kLinkFixCtrl()
{
	GetDlgItem(IDC_ED2KFIX)->EnableWindow(HaveEd2kRegAccess() && Ask4RegFix(true));
}

BOOL CPPgGeneral::OnSetActive()
{
	UpdateEd2kLinkFixCtrl();
	return __super::OnSetActive();
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
		
		//Commander - Added: Invisible Mode [TPT] - Start
		// Add key modifiers to ComboBox
		((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO))->ResetContent();
		((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO))->AddString(GetResString(IDS_CTRLKEY));
		((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO))->AddString(GetResString(IDS_ALTKEY));
		((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO))->AddString(GetResString(IDS_SHIFTKEY));
		((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO))->AddString(GetResString(IDS_CTRLKEY) + " + " + GetResString(IDS_ALTKEY));
		((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO))->AddString(GetResString(IDS_CTRLKEY) + " + " + GetResString(IDS_SHIFTKEY));
		((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO))->AddString(GetResString(IDS_ALTKEY) + " + " + GetResString(IDS_SHIFTKEY));
		((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO))->AddString(GetResString(IDS_CTRLKEY) + " + " + GetResString(IDS_ALTKEY) + " + " + GetResString(IDS_SHIFTKEY));

		CString key_modifier;
		if (m_iActualKeyModifier & MOD_CONTROL)
			key_modifier=GetResString(IDS_CTRLKEY);
		if (m_iActualKeyModifier & MOD_ALT){
			if (!key_modifier.IsEmpty()) key_modifier += " + ";
			key_modifier+=GetResString(IDS_ALTKEY);
		}
		if (m_iActualKeyModifier & MOD_SHIFT){
			if (!key_modifier.IsEmpty()) key_modifier += " + ";
			key_modifier+=GetResString(IDS_SHIFTKEY);
		}
		((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO))->SelectString(-1,key_modifier);
		
		GetDlgItem(IDC_INVISIBLE_MODE_GROUP_BOX)->SetWindowText(GetResString(IDS_INVMODE_GROUP));
		GetDlgItem(IDC_INVISIBLE_MODE)->SetWindowText(GetResString(IDS_INVMODE));
		GetDlgItem(IDC_INVISIBLE_MODE_SELECT_STATIC)->SetWindowText(GetResString(IDS_INVMODE_HOTKEY));
		GetDlgItem(IDC_INVISIBLE_MODE_MODIFIER_STATIC)->SetWindowText(GetResString(IDS_INVMODE_MODKEY));
		GetDlgItem(IDC_INVISIBLE_MODE_KEY_STATIC)->SetWindowText(GetResString(IDS_INVMODE_VKEY));
		//Commander - Added: Invisible Mode [TPT] - End
	}
}

void CPPgGeneral::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	SetModified(TRUE);

	if (pScrollBar==GetDlgItem(IDC_CHECKDAYS)) {
		CSliderCtrl* slider =(CSliderCtrl*)pScrollBar;
		CString text;
		text.Format(_T("%i %s"),slider->GetPos(),GetResString(IDS_DAYS2));
		GetDlgItem(IDC_DAYS)->SetWindowText(text);
	}

	UpdateData(false); 
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPgGeneral::OnBnClickedEditWebservices()
{
	theWebServices.Edit();
}

void CPPgGeneral::OnLangChange()
{
#define MIRRORS_URL	_T("http://langmirror%i.emule-project.org/lang/%i%i%i%i/")

	WORD byNewLang =  m_language.GetItemData(m_language.GetCurSel());
	if (thePrefs.GetLanguageID() != byNewLang){
		if	(!thePrefs.IsLanguageSupported(byNewLang, false)){
			if (AfxMessageBox(GetResString(IDS_ASKDOWNLOADLANGCAP) + _T("\r\n\r\n") + GetResString(IDS_ASKDOWNLOADLANG), MB_ICONQUESTION | MB_YESNO) == IDYES){
				// download file
				// create url, use random mirror for load balancing
				uint16 nRand = (rand()/(RAND_MAX/3))+1;
				CString strUrl;
				strUrl.Format(MIRRORS_URL,nRand, VERSION_MJR, VERSION_MIN, VERSION_UPDATE, VERSION_BUILD);
				strUrl += thePrefs.GetLangDLLNameByID(byNewLang);
				// safeto
				CString strFilename = thePrefs.GetLangDir();
				if (!PathFileExists(strFilename))
					CreateDirectory(strFilename,0);
				strFilename.Append(thePrefs.GetLangDLLNameByID(byNewLang));
				// start
				CHttpDownloadDlg dlgDownload;
				dlgDownload.m_strTitle = _T("Downloading language file");
				dlgDownload.m_sURLToDownload = strUrl;
				dlgDownload.m_sFileToDownloadInto = strFilename;
				if (dlgDownload.DoModal() == IDOK && thePrefs.IsLanguageSupported(byNewLang, true))
				{
					// everything ok, new language downloaded and working
					OnSettingsChange();
					return;
				}
				CString strErr;
				strErr.Format(GetResString(IDS_ERR_FAILEDDOWNLOADLANG), strUrl);
				AddLogLine(true, strErr);
				AfxMessageBox(strErr, MB_ICONERROR | MB_OK);
			}
			// undo change selection
			for(int i = 0; i < m_language.GetCount(); i++)
				if(m_language.GetItemData(i) == thePrefs.GetLanguageID())
					m_language.SetCurSel(i);
		}
		else
			OnSettingsChange();
	}
}

void CPPgGeneral::OnBnClickedCheck4Update()
{
	SetModified();
	GetDlgItem(IDC_CHECKDAYS)->ShowWindow( IsDlgButtonChecked(IDC_CHECK4UPDATE)?SW_SHOW:SW_HIDE );
	GetDlgItem(IDC_DAYS)->ShowWindow( IsDlgButtonChecked(IDC_CHECK4UPDATE)?SW_SHOW:SW_HIDE );
}

void CPPgGeneral::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_General);
}

BOOL CPPgGeneral::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return __super::OnCommand(wParam, lParam);
}

BOOL CPPgGeneral::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}

//Commander - Added: Invisible Mode [TPT] - Start
void CPPgGeneral::SetBoxes()
{	
	bool bImode = IsDlgButtonChecked(IDC_INVISIBLE_MODE);

	GetDlgItem(IDC_INVISIBLE_MODE_SELECT_STATIC)->EnableWindow(bImode);
	GetDlgItem(IDC_INVISIBLE_MODE_MODIFIER_STATIC)->EnableWindow(bImode);
	GetDlgItem(IDC_INVISIBLE_MODE_KEY_STATIC)->EnableWindow(bImode);
	GetDlgItem(IDC_INVISIBLE_MODE_SYMBOL_STATIC)->EnableWindow(bImode);
	GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO)->EnableWindow(bImode);
	GetDlgItem(IDC_INVISIBLE_MODE_SELECT_COMBO)->EnableWindow(bImode);

	SetModified();
}

void CPPgGeneral::OnCbnSelchangeKeymodcombo()
{
	CString sKeyMod;
	
	((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO))->GetLBText(((CComboBox*)GetDlgItem(IDC_INVISIBLE_MODE_KEY_COMBO))->GetCurSel(), sKeyMod);
	m_iActualKeyModifier = 0;
	if (sKeyMod.Find(GetResString(IDS_CTRLKEY))!=-1)
		m_iActualKeyModifier |= MOD_CONTROL;
	if (sKeyMod.Find(GetResString(IDS_ALTKEY))!=-1)
		m_iActualKeyModifier |= MOD_ALT;
	if (sKeyMod.Find(GetResString(IDS_SHIFTKEY))!=-1)
		m_iActualKeyModifier |= MOD_SHIFT;
	
	SetModified();
}
//Commander - Added: Invisible Mode [TPT] - End