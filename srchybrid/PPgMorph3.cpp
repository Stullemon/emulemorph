// PpgMorph3.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgMorph3.h"
#include "OtherFunctions.h"
#include "Preferences.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
// CPPgMorph3 dialog

BEGIN_MESSAGE_MAP(CPPgMorph3, CPropertyPage)
	ON_EN_CHANGE(IDC_EDIT_DYNDNS_USERNAME, OnDataChange)
	ON_EN_CHANGE(IDC_EDIT_DYNDNS_PASSWORD, OnDataChange)
	ON_EN_CHANGE(IDC_EDIT_DYNDNS_HOSTNAME, OnDataChange)
	ON_EN_CHANGE(IDC_CHECK_DYNDNS_ENABLED, OnDataChange)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CPPgMorph3, CPropertyPage)
CPPgMorph3::CPPgMorph3()
: CPropertyPage(CPPgMorph3::IDD)
{
}

CPPgMorph3::~CPPgMorph3()
{
}

void CPPgMorph3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

// CPPgMorph3 message handlers

BOOL CPPgMorph3::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
    /*
	//MORPH START - Added by Commander, Various Symbols
	if ( theApp.emuledlg->m_fontMarlett.m_hObject )
	{
		GetDlgItem(IDC_BUTTON_DYNDNS_PREVIOUS)->SetFont(&theApp.emuledlg->m_fontMarlett);
		GetDlgItem(IDC_BUTTON_DYNDNS_PREVIOUS)->SetWindowText(_T("3"));

		GetDlgItem(IDC_BUTTON_DYNDNS_NEXT)->SetFont(&theApp.emuledlg->m_fontMarlett);
		GetDlgItem(IDC_BUTTON_DYNDNS_NEXT)->SetWindowText(_T("4"));
        
		GetDlgItem(IDC_STATIC_DYNDNS_CURRENTIP_ARROW)->SetFont(&theApp.emuledlg->m_fontMarlett);
		GetDlgItem(IDC_STATIC_DYNDNS_CURRENTIP_ARROW)->SetWindowText(_T("4"));

		GetDlgItem(IDC_STATIC_DYNDNS_LASTUPDATE_ARROW)->SetFont(&theApp.emuledlg->m_fontMarlett);
		GetDlgItem(IDC_STATIC_DYNDNS_LASTUPDATE_ARROW)->SetWindowText(_T("4"));

		GetDlgItem(IDC_BUTTON_DYNDNS_HELP)->SetFont(&theApp.emuledlg->m_fontMarlett);
		GetDlgItem(IDC_BUTTON_DYNDNS_HELP)->SetWindowText(_T("s"));
	}
	//MORPH END - Added by Commander, Various Symbols

	CString strBuffer;

	strBuffer.Format(_T("%s"), thePrefs.GetDynDNSUsername());
	GetDlgItem(IDC_EDIT_DYNDNS_USERNAME)->SetWindowText(strBuffer);

    GetDlgItem(IDC_EDIT_DYNDNS_PASSWORD)->SetWindowText(HIDDEN_PASSWORD);

	strBuffer.Format(_T("%s"), thePrefs.GetDynDNSHostname());
	GetDlgItem(IDC_EDIT_DYNDNS_HOSTNAME)->SetWindowText(strBuffer);

	if(thePrefs.GetDynDNSIsEnabled())
		CheckDlgButton(IDC_CHECK_DYNDNS_ENABLED,1);
	else
		CheckDlgButton(IDC_CHECK_DYNDNS_ENABLED,0);
	*/
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgMorph3::OnApply()
{
	if(m_bModified)
	{   
		/*
		CString sBuf;
		CString oldUsername=thePrefs.GetDynDNSUsername();
		CString oldHostname=thePrefs.GetDynDNSHostname();
        int oldState=thePrefs.GetDynDNSIsEnabled();

		GetDlgItem(IDC_EDIT_DYNDNS_USERNAME)->GetWindowText(sBuf);
		if(sBuf != oldUsername )
			thePrefs.SetDynDNSUsername(sBuf);

		GetDlgItem(IDC_EDIT_DYNDNS_PASSWORD)->GetWindowText(sBuf);
		if(sBuf != HIDDEN_PASSWORD)
			thePrefs.SetDynDNSPassword(sBuf);

		GetDlgItem(IDC_EDIT_DYNDNS_HOSTNAME)->GetWindowText(sBuf);
		if(sBuf != oldHostname )
			thePrefs.SetDynDNSHostname(sBuf);

		GetDlgItem(IDC_CHECK_DYNDNS_ENABLED)->GetWindowText(sBuf);
		if (_tstoi(sBuf)!=oldState)
			thePrefs.SetWSPort(_tstoi(sBuf));
		*/
	}

	if (!UpdateData())
		return FALSE;

	bool bRestartApp = false;

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}
void CPPgMorph3::Localize(void)
{
	if ( m_hWnd )
	{
		SetWindowText(_T("Morph III"));

        GetDlgItem(IDC_CHECK_DYNDNS_ENABLED)->SetWindowText(GetResString(IDS_CHECK_DYNDNS_ENABLED));
		GetDlgItem(IDC_STATIC_DYNDNS_USERNAME)->SetWindowText(GetResString(IDS_STATIC_DYNDNS_USERNAME));
		GetDlgItem(IDC_STATIC_DYNDNS_PASSWORD)->SetWindowText(GetResString(IDS_STATIC_DYNDNS_PASSWORD));
		GetDlgItem(IDC_STATIC_DYNDNS_HOSTNAME)->SetWindowText(GetResString(IDS_STATIC_DYNDNS_HOSTNAME));
		GetDlgItem(IDC_BUTTON_DYNDNS_UPDATE)->SetWindowText(GetResString(IDS_BUTTON_DYNDNS_UPDATE));
		GetDlgItem(IDC_BUTTON_DYNDNS_RESET)->SetWindowText(GetResString(IDS_BUTTON_DYNDNS_RESET));

	}
}

void CPPgMorph3::OnEnChangeDynDNSEnabled()
{
	GetDlgItem(IDC_EDIT_DYNDNS_USERNAME)->EnableWindow(IsDlgButtonChecked(IDC_CHECK_DYNDNS_ENABLED));	
	GetDlgItem(IDC_EDIT_DYNDNS_PASSWORD)->EnableWindow(IsDlgButtonChecked(IDC_CHECK_DYNDNS_ENABLED));	
	GetDlgItem(IDC_EDIT_DYNDNS_HOSTNAME)->EnableWindow(IsDlgButtonChecked(IDC_CHECK_DYNDNS_ENABLED));	
	GetDlgItem(IDC_BUTTON_DYNDNS_UPDATE)->EnableWindow(IsDlgButtonChecked(IDC_CHECK_DYNDNS_ENABLED));	
	GetDlgItem(IDC_BUTTON_DYNDNS_RESET)->EnableWindow(IsDlgButtonChecked(IDC_CHECK_DYNDNS_ENABLED));	

	SetModified();
}

void CPPgMorph3::OnDestroy()
{
	CPropertyPage::OnDestroy();
}