// PPgServer.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgServer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CPPgServer dialog

IMPLEMENT_DYNAMIC(CPPgServer, CPropertyPage)
CPPgServer::CPPgServer()
	: CPropertyPage(CPPgServer::IDD)
{
}

CPPgServer::~CPPgServer()
{
}

void CPPgServer::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CPPgServer, CPropertyPage)
	ON_EN_CHANGE(IDC_SERVERRETRIES, OnSettingsChange)
	ON_BN_CLICKED(IDC_AUTOSERVER, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPDATESERVERCONNECT, OnSettingsChange)
	ON_BN_CLICKED(IDC_UPDATESERVERCLIENT, OnSettingsChange)
	ON_BN_CLICKED(IDC_SCORE, OnSettingsChange)
	ON_BN_CLICKED(IDC_REMOVEDEAD, OnSettingsChange)
	ON_BN_CLICKED(IDC_SMARTIDCHECK, OnSettingsChange)
	ON_BN_CLICKED(IDC_SAFESERVERCONNECT, OnSettingsChange)
	ON_BN_CLICKED(IDC_AUTOCONNECTSTATICONLY, OnSettingsChange)
	ON_BN_CLICKED(IDC_MANUALSERVERHIGHPRIO, OnSettingsChange)
	ON_BN_CLICKED(IDC_EDITADR, OnBnClickedEditadr)
END_MESSAGE_MAP()


// CPPgServer message handlers

void CPPgServer::OnBnClickedCheck1()
{
	// TODO: Add your control notification handler code here
}

BOOL CPPgServer::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgServer::LoadSettings(void)
{
//	switch(app_prefs->prefs->deadserver)
//	{
//		case 1:	this->CheckDlgButton(IDC_MARKDEAD,1);	break;
//		case 2:	this->CheckDlgButton(IDC_DELETEDEAD,1);	break;
//	}
	
	CString strBuffer;
	strBuffer.Format("%d", app_prefs->prefs->deadserverretries);
	GetDlgItem(IDC_SERVERRETRIES)->SetWindowText(strBuffer);
		

	if(app_prefs->IsSafeServerConnectEnabled())
		CheckDlgButton(IDC_SAFESERVERCONNECT,1);
	else
		CheckDlgButton(IDC_SAFESERVERCONNECT,0);

	if(app_prefs->prefs->m_bmanualhighprio)
		CheckDlgButton(IDC_MANUALSERVERHIGHPRIO,1);
	else
		CheckDlgButton(IDC_MANUALSERVERHIGHPRIO,0);

	if(app_prefs->GetSmartIdCheck())
		CheckDlgButton(IDC_SMARTIDCHECK,1);
	else
		CheckDlgButton(IDC_SMARTIDCHECK,0);

	if(app_prefs->prefs->deadserver)
		CheckDlgButton(IDC_REMOVEDEAD,1);
	else
		CheckDlgButton(IDC_REMOVEDEAD,0);
	
	if(app_prefs->prefs->autoserverlist)
		CheckDlgButton(IDC_AUTOSERVER,1);
	else
		CheckDlgButton(IDC_AUTOSERVER,0);

	if(app_prefs->prefs->addserversfromserver)
		CheckDlgButton(IDC_UPDATESERVERCONNECT, 1);
	else
		CheckDlgButton(IDC_UPDATESERVERCONNECT, 0);
	
	if(app_prefs->prefs->addserversfromclient)
		CheckDlgButton(IDC_UPDATESERVERCLIENT, 1);
	else
		CheckDlgButton(IDC_UPDATESERVERCLIENT, 0);

	if(app_prefs->prefs->scorsystem)
		CheckDlgButton(IDC_SCORE,1);
	else
		CheckDlgButton(IDC_SCORE,0);

	// Barry
	if(app_prefs->prefs->autoconnectstaticonly)
		CheckDlgButton(IDC_AUTOCONNECTSTATICONLY,1);
	else
		CheckDlgButton(IDC_AUTOCONNECTSTATICONLY,0);
}

BOOL CPPgServer::OnApply()
{	
	char buffer[510];
	
//	if(IsDlgButtonChecked(IDC_MARKDEAD))
//		app_prefs->prefs->deadserver = 1;
//	else
//		app_prefs->prefs->deadserver = 2;
	app_prefs->SetSafeServerConnectEnabled((int8)IsDlgButtonChecked(IDC_SAFESERVERCONNECT));

	if(IsDlgButtonChecked(IDC_SMARTIDCHECK))
		app_prefs->prefs->smartidcheck = true;
	else
		app_prefs->prefs->smartidcheck = false;

	if(IsDlgButtonChecked(IDC_MANUALSERVERHIGHPRIO))
		app_prefs->prefs->m_bmanualhighprio = true;
	else
		app_prefs->prefs->m_bmanualhighprio = false;

	app_prefs->prefs->deadserver = (int8)IsDlgButtonChecked(IDC_REMOVEDEAD);
	
	if(GetDlgItem(IDC_SERVERRETRIES)->GetWindowTextLength())
	{
		GetDlgItem(IDC_SERVERRETRIES)->GetWindowText(buffer,20);
		app_prefs->prefs->deadserverretries = (atoi(buffer)) ? atoi(buffer) : 5;
	}
	if(app_prefs->prefs->deadserverretries < 1) 
		app_prefs->prefs->deadserverretries = 5;

	app_prefs->prefs->scorsystem = (int8)IsDlgButtonChecked(IDC_SCORE);
	app_prefs->prefs->autoserverlist = (int8)IsDlgButtonChecked(IDC_AUTOSERVER);
	app_prefs->prefs->addserversfromserver = (int8)IsDlgButtonChecked(IDC_UPDATESERVERCONNECT);
	app_prefs->prefs->addserversfromclient = (int8)IsDlgButtonChecked(IDC_UPDATESERVERCLIENT);
	// Barry
	app_prefs->prefs->autoconnectstaticonly = (int8)IsDlgButtonChecked(IDC_AUTOCONNECTSTATICONLY);
	//	app_prefs->Save();
	LoadSettings();

	SetModified();
	return CPropertyPage::OnApply();
}

void CPPgServer::Localize(void)
{
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_SERVER));
		
		GetDlgItem(IDC_REMOVEDEAD)->SetWindowText(GetResString(IDS_PW_RDEAD));
		GetDlgItem(IDC_RETRIES_LBL)->SetWindowText(GetResString(IDS_PW_RETRIES));
		GetDlgItem(IDC_UPDATESERVERCONNECT)->SetWindowText(GetResString(IDS_PW_USC));
		GetDlgItem(IDC_UPDATESERVERCLIENT)->SetWindowText(GetResString(IDS_PW_UCC));
		GetDlgItem(IDC_AUTOSERVER)->SetWindowText(GetResString(IDS_PW_USS));
		GetDlgItem(IDC_SMARTIDCHECK)->SetWindowText(GetResString(IDS_SMARTLOWIDCHECK));
		GetDlgItem(IDC_SAFESERVERCONNECT)->SetWindowText(GetResString(IDS_PW_FASTSRVCON));
		GetDlgItem(IDC_SCORE)->SetWindowText(GetResString(IDS_PW_SCORE));
		GetDlgItem(IDC_MANUALSERVERHIGHPRIO)->SetWindowText(GetResString(IDS_MANUALSERVERHIGHPRIO));
		GetDlgItem(IDC_EDITADR)->SetWindowText(GetResString(IDS_EDITLIST));
		// Barry
		GetDlgItem(IDC_AUTOCONNECTSTATICONLY)->SetWindowText(GetResString(IDS_PW_AUTOCONNECTSTATICONLY));
	}
}

void CPPgServer::OnBnClickedEditadr()
{
	ShellExecute(NULL, "open", theApp.glob_prefs->GetTxtEditor(), "\""+CString(theApp.glob_prefs->GetConfigDir())+"adresses.dat\"", NULL, SW_SHOW); 
}
