// PpgMorph2.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgMorph2.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CPPgMorph dialog

IMPLEMENT_DYNAMIC(CPPgMorph2, CPropertyPage)
CPPgMorph2::CPPgMorph2()
: CPropertyPage(CPPgMorph2::IDD)
{
}

CPPgMorph2::~CPPgMorph2()
{
}

void CPPgMorph2::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPPgMorph2, CPropertyPage)
	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	ON_EN_CHANGE(IDC_LOWIDRETRY, OnSettingsChange)
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry
	//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	ON_BN_CLICKED(IDC_UPDATEFAKELISTSTART, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPDATEFAKES, OnBnClickedUpdatefakes)
	ON_EN_CHANGE(IDC_UPDATE_URL_FAKELIST, OnSettingsChange)
	//MORPH END - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	//MORPH START added by Yun.SF3: Ipfilter.dat update
	ON_EN_CHANGE(IDC_UPDATE_URL_IPFILTER, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPDATEIPFURL, OnBnClickedUpdateipfurl)
	ON_BN_CLICKED(IDC_AUTOUPIPFILTER , OnSettingsChange)
	//MORPH END added by Yun.SF3: Ipfilter.dat update
END_MESSAGE_MAP()


// CPPgMorph message handlers

BOOL CPPgMorph2::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgMorph2::LoadSettings(void)
{
	CString strBuffer;

	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	strBuffer.Format("%d", app_prefs->GetLowIdRetries());
	GetDlgItem(IDC_LOWIDRETRY)->SetWindowText(strBuffer);
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry

	//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	GetDlgItem(IDC_UPDATE_URL_FAKELIST)->SetWindowText(app_prefs->prefs->UpdateURLFakeList);
	if(app_prefs->prefs->UpdateFakeStartup)
		CheckDlgButton(IDC_UPDATEFAKELISTSTART,1);
	else
		CheckDlgButton(IDC_UPDATEFAKELISTSTART,0);
	//MORPH END - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating

	//MORPH START added by Yun.SF3: Ipfilter.dat update
	GetDlgItem(IDC_UPDATE_URL_IPFILTER)->SetWindowText(app_prefs->prefs->UpdateURLIPFilter);
	if(app_prefs->prefs->AutoUpdateIPFilter)
		CheckDlgButton(IDC_AUTOUPIPFILTER,1);
	else
		CheckDlgButton(IDC_AUTOUPIPFILTER,0);
	//MORPH END added by Yun.SF3: Ipfilter.dat update
}

BOOL CPPgMorph2::OnApply()
{

	CString buffer;
	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	if(GetDlgItem(IDC_LOWIDRETRY)->GetWindowTextLength())
	{
		GetDlgItem(IDC_LOWIDRETRY)->GetWindowText(buffer);
		app_prefs->SetLowIdRetries(atoi(buffer));
	}
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry

	//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	GetDlgItem(IDC_UPDATE_URL_FAKELIST)->GetWindowText(buffer);
	strcpy(app_prefs->prefs->UpdateURLFakeList, buffer);
	app_prefs->prefs->UpdateFakeStartup = IsDlgButtonChecked(IDC_UPDATEFAKELISTSTART);
	//MORPH END   - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating

	//MORPH START - Added by Yun.SF3: Ipfilter.dat update
	GetDlgItem(IDC_UPDATE_URL_IPFILTER)->GetWindowText(buffer);
	strcpy(app_prefs->prefs->UpdateURLIPFilter, buffer);
	app_prefs->prefs->AutoUpdateIPFilter = IsDlgButtonChecked(IDC_AUTOUPIPFILTER);
	//MORPH END   - Added by Yun.SF3: Ipfilter.dat update

	LoadSettings();
	SetModified(FALSE);
	
	return CPropertyPage::OnApply();
}
void CPPgMorph2::Localize(void)
{
	if(m_hWnd)
	{
		//SetWindowText(GetResString(IDS_MORPH2));
		
		//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
		GetDlgItem(IDC_LOWIDRETRYLABEL)->SetWindowText(GetResString(IDS_LOWIDRETRYLABEL));
		//MORPH END - Added by SiRoB, SLUGFILLER: lowIdRetry
		
		//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
		GetDlgItem(IDC_UPDATEFAKES)->SetWindowText(GetResString(IDS_UPDATEFAKES));
		GetDlgItem(IDC_UPDATEFAKELISTSTART)->SetWindowText(GetResString(IDS_UPDATEFAKECHECKONSTART));
		GetDlgItem(IDC_URL_FOR_UPDATING)->SetWindowText(GetResString(IDS_URL_FOR_UPDATING));
		//MORPH END   - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
		
		//MORPH START - Added by Yun.SF3: Ipfilter.dat update
		GetDlgItem(IDC_AUTOUPIPFILTER)->SetWindowText(GetResString(IDS_UPDATEIPFILTERONSTART));
		GetDlgItem(IDC_UPDATEIPFURL)->SetWindowText(GetResString(IDS_UPDATEIPFILTER));
		GetDlgItem(IDC_URL_FOR_UPDATING)->SetWindowText(GetResString(IDS_URL_FOR_UPDATING));
		//MORPH END   - Added by Yun.SF3: Ipfilter.dat update

	}
}

void CPPgMorph2::OnBnClickedUpdatefakes()
{
	if(!theApp.FakeCheck->DownloadFakeList())
		theApp.emuledlg->AddLogLine(true, GetResString(IDS_FAKECHECKUPERROR));
}

//MORPH START added by Yun.SF3: Ipfilter.dat update
void CPPgMorph2::OnBnClickedUpdateipfurl()
{
	theApp.ipfilter->UpdateIPFilterURL();
}
//MORPH END added by Yun.SF3: Ipfilter.dat update
