// PPgSecurity.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgSecurity.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CPPgSecurity dialog

IMPLEMENT_DYNAMIC(CPPgSecurity, CPropertyPage)
CPPgSecurity::CPPgSecurity()
	: CPropertyPage(CPPgSecurity::IDD)
{
}

CPPgSecurity::~CPPgSecurity()
{
}

void CPPgSecurity::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPPgSecurity, CPropertyPage)
	ON_BN_CLICKED(IDC_FILTERSERVERBYIPFILTER , OnSettingsChange)
	ON_BN_CLICKED(IDC_RELOADFILTER, OnReloadIPFilter)
	ON_BN_CLICKED(IDC_EDITFILTER, OnEditIPFilter)

	ON_EN_CHANGE(IDC_FILTERLEVEL, OnSettingsChange)
	ON_EN_CHANGE(IDC_FILTER, OnSettingsChange)
	ON_EN_CHANGE(IDC_COMMENTFILTER, OnSettingsChange)
	ON_BN_CLICKED(IDC_MSGONLYFRIENDS , OnSettingsChange) 
	ON_BN_CLICKED(IDC_MSGONLYSEC, OnSettingsChange)
	ON_BN_CLICKED(IDC_ADVSPAMFILTER , OnSettingsChange)
	ON_BN_CLICKED(IDC_USESECIDENT, OnSettingsChange)
END_MESSAGE_MAP()

void CPPgSecurity::LoadSettings(void)
{
	CString strBuffer;
	
	strBuffer.Format("%i",app_prefs->prefs->filterlevel);
	GetDlgItem(IDC_FILTERLEVEL)->SetWindowText(strBuffer);

	if(app_prefs->prefs->filterserverbyip)
		CheckDlgButton(IDC_FILTERSERVERBYIPFILTER,1);
	else
		CheckDlgButton(IDC_FILTERSERVERBYIPFILTER,0);

	if(app_prefs->prefs->msgonlyfriends)
		CheckDlgButton(IDC_MSGONLYFRIENDS,1);
	else
		CheckDlgButton(IDC_MSGONLYFRIENDS,0);

	if(app_prefs->prefs->msgsecure)
		CheckDlgButton(IDC_MSGONLYSEC,1);
	else
		CheckDlgButton(IDC_MSGONLYSEC,0);

	if(app_prefs->prefs->m_bAdvancedSpamfilter)
		CheckDlgButton(IDC_ADVSPAMFILTER,1);
	else
		CheckDlgButton(IDC_ADVSPAMFILTER,0);

	if(app_prefs->prefs->m_bUseSecureIdent)
		CheckDlgButton(IDC_USESECIDENT,1);
	else
		CheckDlgButton(IDC_USESECIDENT,0);

	GetDlgItem(IDC_FILTER)->SetWindowText(app_prefs->prefs->messageFilter);
	GetDlgItem(IDC_COMMENTFILTER)->SetWindowText(app_prefs->prefs->commentFilter);
}

BOOL CPPgSecurity::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
	//MORPH START - Added by SiRoB, Allways use securedid
	GetDlgItem(IDC_USESECIDENT)->EnableWindow(0);
	//MORPH END   - Added by SiRoB, Allways use securedid
	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgSecurity::OnApply()
{
	char buffer[510];
	if(GetDlgItem(IDC_FILTERLEVEL)->GetWindowTextLength())
	{
		GetDlgItem(IDC_FILTERLEVEL)->GetWindowText(buffer,4);
		app_prefs->prefs->filterlevel=atoi(buffer);
	}

	app_prefs->prefs->filterserverbyip = (int8)IsDlgButtonChecked(IDC_FILTERSERVERBYIPFILTER);
	app_prefs->prefs->msgonlyfriends = (int8)IsDlgButtonChecked(IDC_MSGONLYFRIENDS);
	app_prefs->prefs->msgsecure = (int8)IsDlgButtonChecked(IDC_MSGONLYSEC);
	app_prefs->prefs->m_bAdvancedSpamfilter = IsDlgButtonChecked(IDC_ADVSPAMFILTER);
	app_prefs->prefs->m_bUseSecureIdent = IsDlgButtonChecked(IDC_USESECIDENT);

	GetDlgItem(IDC_FILTER)->GetWindowText(app_prefs->prefs->messageFilter,511);
	GetDlgItem(IDC_COMMENTFILTER)->GetWindowText(app_prefs->prefs->commentFilter,511);

	LoadSettings();
	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgSecurity::Localize(void)
{
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_SECURITY));
		GetDlgItem(IDC_STATIC_IPFILTER)->SetWindowText(GetResString(IDS_IPFILTER));
		GetDlgItem(IDC_RELOADFILTER)->SetWindowText(GetResString(IDS_SF_RELOAD));
		GetDlgItem(IDC_EDITFILTER)->SetWindowText(GetResString(IDS_EDIT));
		GetDlgItem(IDC_STATIC_FILTERLEVEL)->SetWindowText(GetResString(IDS_FILTERLEVEL)+":");
		GetDlgItem(IDC_FILTERSERVERBYIPFILTER)->SetWindowText(GetResString(IDS_FILTERSERVERBYIPFILTER));

		GetDlgItem(IDC_FILTERCOMMENTSLABEL)->SetWindowText(GetResString(IDS_FILTERCOMMENTSLABEL));
		GetDlgItem(IDC_STATIC_COMMENTS)->SetWindowText(GetResString(IDS_COMMENT));

		GetDlgItem(IDC_FILTERLABEL)->SetWindowText(GetResString(IDS_FILTERLABEL));
		GetDlgItem(IDC_MSG)->SetWindowText(GetResString(IDS_CW_MESSAGES));

		GetDlgItem(IDC_MSGONLYFRIENDS)->SetWindowText(GetResString(IDS_MSGONLYFRIENDS));
		GetDlgItem(IDC_MSGONLYSEC)->SetWindowText(GetResString(IDS_MSGONLYSEC));

		GetDlgItem(IDC_ADVSPAMFILTER)->SetWindowText(GetResString(IDS_ADVSPAMFILTER));
		GetDlgItem(IDC_SEC_MISC)->SetWindowText(GetResString(IDS_PW_MISC));
		GetDlgItem(IDC_USESECIDENT)->SetWindowText(GetResString(IDS_USESECIDENT));
	}
}

void CPPgSecurity::OnReloadIPFilter() {
	int count=theApp.ipfilter->LoadFromFile();
	AddLogLine(true,GetResString(IDS_IPFILTERLOADED),count);
}

void CPPgSecurity::OnEditIPFilter() {
	ShellExecute(NULL, "open", theApp.glob_prefs->GetTxtEditor(), "\""+CString(theApp.glob_prefs->GetConfigDir())+"IPfilter.dat\"", NULL, SW_SHOW); 
}
