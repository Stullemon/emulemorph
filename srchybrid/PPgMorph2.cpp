// PpgMorph2.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgMorph2.h"
#include "emuleDlg.h"
#include "OtherFunctions.h"
#include "serverWnd.h"
#include "Fakecheck.h"
#include "IPFilter.h"
#include "IP2Country.h"

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
	//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	ON_BN_CLICKED(IDC_UPDATEFAKELISTSTART, OnSettingsChange)
	ON_BN_CLICKED(IDC_RESETFAKESURL, OnBnClickedResetfakes)
	ON_BN_CLICKED(IDC_UPDATEFAKES, OnBnClickedUpdatefakes)
	ON_EN_CHANGE(IDC_UPDATE_URL_FAKELIST, OnSettingsChange)
	//MORPH END - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	//MORPH START added by Yun.SF3: Ipfilter.dat update
	ON_EN_CHANGE(IDC_UPDATE_URL_IPFILTER, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPDATEIPFURL, OnBnClickedUpdateipfurl)
	ON_BN_CLICKED(IDC_RESETIPFURL, OnBnClickedResetipfurl)
	ON_BN_CLICKED(IDC_AUTOUPIPFILTER , OnSettingsChange)
	//MORPH END added by Yun.SF3: Ipfilter.dat update
	//Commander - Added: IP2Country Auto-updating - Start
	ON_EN_CHANGE(IDC_UPDATE_URL_IP2COUNTRY, OnSettingsChange)
    ON_EN_CHANGE(IDC_UPDATE_VER_URL_IP2COUNTRY, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPDATEIPCURL, OnBnClickedUpdateipcurl)
	ON_BN_CLICKED(IDC_RESETIPCURL, OnBnClickedResetipcurl)
	ON_BN_CLICKED(IDC_AUTOUPIP2COUNTRY , OnSettingsChange)
	//Commander - Added: IP2Country Auto-updating - End
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

	//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	GetDlgItem(IDC_UPDATE_URL_FAKELIST)->SetWindowText(thePrefs.UpdateURLFakeList);
	if(thePrefs.UpdateFakeStartup)
		CheckDlgButton(IDC_UPDATEFAKELISTSTART,1);
	else
		CheckDlgButton(IDC_UPDATEFAKELISTSTART,0);
	//MORPH END - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating

	//MORPH START added by Yun.SF3: Ipfilter.dat update
	GetDlgItem(IDC_UPDATE_URL_IPFILTER)->SetWindowText(thePrefs.UpdateURLIPFilter);
	if(thePrefs.AutoUpdateIPFilter)
		CheckDlgButton(IDC_AUTOUPIPFILTER,1);
	else
		CheckDlgButton(IDC_AUTOUPIPFILTER,0);
	//MORPH END added by Yun.SF3: Ipfilter.dat update

	//Commander - Added: IP2Country Auto-updating - Start
	GetDlgItem(IDC_UPDATE_URL_IP2COUNTRY)->SetWindowText(thePrefs.UpdateURLIP2Country);
	if(thePrefs.AutoUpdateIP2Country)
		CheckDlgButton(IDC_AUTOUPIP2COUNTRY,1);
	else
		CheckDlgButton(IDC_AUTOUPIP2COUNTRY,0);
	//Commander - Added: IP2Country Auto-updating - End

	//Commander - Added: IP2Country Auto-updating - Start
	GetDlgItem(IDC_UPDATE_VER_URL_IP2COUNTRY)->SetWindowText(thePrefs.UpdateVerURLIP2Country);
	//Commander - Added: IP2Country Auto-updating - End

	//Commander - Added: IP2Country Auto-updating - Start
	strBuffer.Format(_T("v.%u"), thePrefs.GetIP2CountryVersion());
	GetDlgItem(IDC_IP2COUNTRY_VERSION)->SetWindowText(strBuffer);
	//Commander - Added: IP2Country Auto-updating - End

	//Commander - Added: IP2Country Auto-updating - Start
	strBuffer.Format(_T("v.%u"), thePrefs.GetFakesDatVersion());
	GetDlgItem(IDC_FAKELIST_VERSION)->SetWindowText(strBuffer);
	//Commander - Added: IP2Country Auto-updating - End

	//Commander - Added: IP2Country Auto-updating - Start
	strBuffer.Format(_T("v.%u"), thePrefs.GetIPfilterVersion());
	GetDlgItem(IDC_IPFILTER_VERSION)->SetWindowText(strBuffer);
	//Commander - Added: IP2Country Auto-updating - End
}

BOOL CPPgMorph2::OnApply()
{

	CString buffer;
	
	//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	GetDlgItem(IDC_UPDATE_URL_FAKELIST)->GetWindowText(buffer);
	_tcscpy(thePrefs.UpdateURLFakeList, buffer);
	thePrefs.UpdateFakeStartup = IsDlgButtonChecked(IDC_UPDATEFAKELISTSTART);
	//MORPH END   - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating

	//MORPH START - Added by Yun.SF3: Ipfilter.dat update
	GetDlgItem(IDC_UPDATE_URL_IPFILTER)->GetWindowText(buffer);
	_tcscpy(thePrefs.UpdateURLIPFilter, buffer);
	thePrefs.AutoUpdateIPFilter = IsDlgButtonChecked(IDC_AUTOUPIPFILTER);
	//MORPH END   - Added by Yun.SF3: Ipfilter.dat update

    //Commander - Added: IP2Country Auto-updating - Start
    GetDlgItem(IDC_UPDATE_URL_IP2COUNTRY)->GetWindowText(buffer);
	_tcscpy(thePrefs.UpdateURLIP2Country, buffer);
	thePrefs.AutoUpdateIP2Country = IsDlgButtonChecked(IDC_AUTOUPIP2COUNTRY);
	//Commander - Added: IP2Country Auto-updating - End

	//Commander - Added: IP2Country Auto-updating - Start
	GetDlgItem(IDC_UPDATE_VER_URL_IP2COUNTRY)->GetWindowText(buffer);
	_tcscpy(thePrefs.UpdateVerURLIP2Country, buffer);
	//thePrefs.AutoVerUpdateIP2Country = IsDlgButtonChecked(IDC_AUTOUPIP2COUNTRY);
	//Commander - Added: IP2Country Auto-updating - End

	LoadSettings();
	SetModified(FALSE);
	
	return CPropertyPage::OnApply();
}
void CPPgMorph2::Localize(void)
{
	if(m_hWnd)
	{
		//SetWindowText(GetResString(IDS_MORPH2));
		
		//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
		GetDlgItem(IDC_MORPH2_FILE)->SetWindowText(GetResString(IDS_FILES));
		GetDlgItem(IDC_UPDATEFAKELISTSTART)->SetWindowText(GetResString(IDS_UPDATEFAKECHECKONSTART));
		GetDlgItem(IDC_UPDATEFAKES)->SetWindowText(GetResString(IDS_UPDATEFAKES));
		GetDlgItem(IDC_URL_FOR_UPDATING)->SetWindowText(GetResString(IDS_URL_FOR_UPDATING));
		GetDlgItem(IDC_RESETFAKESURL)->SetWindowText(GetResString(IDS_RESET));
		//MORPH END   - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
		
		//MORPH START - Added by Yun.SF3: Ipfilter.dat update
		GetDlgItem(IDC_MORPH2_SECURITY)->SetWindowText(GetResString(IDS_SECURITY));
		GetDlgItem(IDC_AUTOUPIPFILTER)->SetWindowText(GetResString(IDS_UPDATEIPFILTERONSTART));
		GetDlgItem(IDC_UPDATEIPFURL)->SetWindowText(GetResString(IDS_UPDATEIPCURL));
		GetDlgItem(IDC_URL_FOR_UPDATING2)->SetWindowText(GetResString(IDS_URL_FOR_UPDATING));
		GetDlgItem(IDC_RESETIPFURL)->SetWindowText(GetResString(IDS_RESET));
		//MORPH END   - Added by Yun.SF3: Ipfilter.dat update

		//MORPH START - Added by Commander: IP2Country update
		GetDlgItem(IDC_MORPH2_COUNTRY)->SetWindowText(GetResString(IDS_COUNTRY));
		GetDlgItem(IDC_AUTOUPIP2COUNTRY)->SetWindowText(GetResString(IDS_AUTOUPIP2COUNTRY));
		GetDlgItem(IDC_UPDATEIPCURL)->SetWindowText(GetResString(IDS_UPDATEIPCURL));
		GetDlgItem(IDC_URL_FOR_UPDATING_IP2COUNTRY)->SetWindowText(GetResString(IDS_URL_FOR_UPDATING_IP2COUNTRY));
		GetDlgItem(IDC_URL_FOR_UPDATING_IP2COUNTRY_VERFILE)->SetWindowText(GetResString(IDS_URL_FOR_UPDATING_IP2COUNTRY_VERFILE));
		GetDlgItem(IDC_RESETIPCURL)->SetWindowText(GetResString(IDS_RESET));
		//MORPH END - Added by Commander: IP2Country update

	}
}

void CPPgMorph2::OnBnClickedUpdatefakes()
{
	OnApply();
	theApp.FakeCheck->DownloadFakeList();
	CString strBuffer;
	strBuffer.Format(_T("v.%u"), thePrefs.GetFakesDatVersion());
	GetDlgItem(IDC_FAKELIST_VERSION)->SetWindowText(strBuffer);
}

void CPPgMorph2::OnBnClickedResetfakes()
{
	CString strBuffer = _T("http://emulepawcio.sourceforge.net/nieuwe_site/Ipfilter_fakes/fakes.dat");
	GetDlgItem(IDC_UPDATE_URL_FAKELIST)->SetWindowText(strBuffer);
	thePrefs.m_FakesDatVersion = 0;
	strBuffer.Format(_T("v.%u"), thePrefs.GetFakesDatVersion());
	GetDlgItem(IDC_FAKELIST_VERSION)->SetWindowText(strBuffer);
}

//MORPH START added by Yun.SF3: Ipfilter.dat update
void CPPgMorph2::OnBnClickedUpdateipfurl()
{
	OnApply();
	theApp.ipfilter->UpdateIPFilterURL();
	CString strBuffer;
	strBuffer.Format(_T("v.%u"), thePrefs.GetIPfilterVersion());
	GetDlgItem(IDC_IPFILTER_VERSION)->SetWindowText(strBuffer);
}
//MORPH END added by Yun.SF3: Ipfilter.dat update

void CPPgMorph2::OnBnClickedResetipfurl()
{
	CString strBuffer = _T("http://emulepawcio.sourceforge.net/nieuwe_site/Ipfilter_fakes/ipfilter.zip");
	GetDlgItem(IDC_UPDATE_URL_IPFILTER)->SetWindowText(strBuffer);
	thePrefs.m_IPfilterVersion = 0;
	strBuffer.Format(_T("v.%u"), thePrefs.GetIPfilterVersion());
	GetDlgItem(IDC_IPFILTER_VERSION)->SetWindowText(strBuffer);
}

//Commander - Added: IP2Country Auto-updating - Start
void CPPgMorph2::OnBnClickedUpdateipcurl()
{
	OnApply();
	theApp.ip2country->UpdateIP2CountryURL();
	CString strBuffer;
	strBuffer.Format(_T("v.%u"), thePrefs.GetIP2CountryVersion());
	GetDlgItem(IDC_IP2COUNTRY_VERSION)->SetWindowText(strBuffer);
}

//Commander - Added: IP2Country Auto-updating - End
void CPPgMorph2::OnBnClickedResetipcurl()
{
	CString strBuffer = _T("http://ip-to-country.webhosting.info/downloads/ip-to-country.csv.zip");
	GetDlgItem(IDC_UPDATE_URL_IP2COUNTRY)->SetWindowText(strBuffer);
	strBuffer = _T("http://ip-to-country.webhosting.info/downloads/latest");
	GetDlgItem(IDC_UPDATE_VER_URL_IP2COUNTRY)->SetWindowText(strBuffer);
	thePrefs.m_IP2CountryVersion = 0;
	strBuffer.Format(_T("v.%u"), thePrefs.GetIP2CountryVersion());
	GetDlgItem(IDC_IP2COUNTRY_VERSION)->SetWindowText(strBuffer);
}
