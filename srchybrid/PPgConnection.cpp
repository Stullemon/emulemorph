// PPgConnection.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgConnection.h"
#include "wizard.h"
#include "Scheduler.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CPPgConnection dialog

IMPLEMENT_DYNAMIC(CPPgConnection, CPropertyPage)
CPPgConnection::CPPgConnection()
	: CPropertyPage(CPPgConnection::IDD)
{
	guardian=false;
}

CPPgConnection::~CPPgConnection()
{
}

void CPPgConnection::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPPgConnection, CPropertyPage)

	ON_EN_CHANGE(IDC_DOWNLOAD_CAP, OnSettingsChange)
	ON_BN_CLICKED(IDC_UDPDISABLE, OnEnChangeUDPDisable)
	ON_EN_CHANGE(IDC_UDPPORT, OnEnChangeUDPDisable)
	ON_EN_CHANGE(IDC_KADUDPPORT, OnSettingsChange)
	ON_EN_CHANGE(IDC_UPLOAD_CAP, OnSettingsChange)
	ON_EN_CHANGE(IDC_MAXDOWN, OnSettingsChange)
	ON_EN_CHANGE(IDC_MAXUP, OnSettingsChange)
	ON_EN_CHANGE(IDC_PORT, OnSettingsChange)
	ON_EN_CHANGE(IDC_MAXCON, OnSettingsChange)
	ON_EN_CHANGE(IDC_MAXSOURCEPERFILE, OnSettingsChange)
	ON_BN_CLICKED(IDC_AUTOCONNECT, OnSettingsChange)
	ON_BN_CLICKED(IDC_RECONN, OnSettingsChange)
	ON_BN_CLICKED(IDC_WIZARD, OnBnClickedWizard)
	ON_BN_CLICKED(IDC_NETWORK_KADEMLIA, OnSettingsChange)
	ON_BN_CLICKED(IDC_NETWORK_ED2K, OnSettingsChange)
	ON_BN_CLICKED(IDC_SHOWOVERHEAD, OnSettingsChange)

	ON_BN_CLICKED(IDC_ULIMIT_LBL, OnLimiterChange)
	ON_BN_CLICKED(IDC_DLIMIT_LBL, OnLimiterChange)

	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// CPPgConnection message handlers

void CPPgConnection::OnEnChangeUDPDisable(){
		if (guardian) return;

		uint16 tempVal=0;
		CString strBuffer;
		char buffer[510];
		
		guardian=true;
		SetModified();

		GetDlgItem(IDC_UDPPORT)->EnableWindow(!IsDlgButtonChecked(IDC_UDPDISABLE));

		if(GetDlgItem(IDC_UDPPORT)->GetWindowTextLength())
		{
			GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer,20);
			tempVal= atoi(buffer);
		}

		
		if (IsDlgButtonChecked(IDC_UDPDISABLE) || (!IsDlgButtonChecked(IDC_UDPDISABLE) && tempVal==0)){
			tempVal= (atoi(buffer)) ? atoi(buffer)+10 : app_prefs->prefs->port+10;
			if ( IsDlgButtonChecked(IDC_UDPDISABLE)) tempVal=0;
			strBuffer.Format("%d", tempVal);
			GetDlgItem(IDC_UDPPORT)->SetWindowText(strBuffer);
		}

		guardian=false;
}

BOOL CPPgConnection::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgConnection::LoadSettings(void)
{
	if(m_hWnd)
	{
		//MORPH START - Added by SiRoB, Avoid change of the MaxDownloadLimit when we are in dyn upload
		if( app_prefs->prefs->maxupload != 0 && !(app_prefs->prefs->m_bDynUpEnabled || app_prefs->prefs->m_bSUCEnabled || app_prefs->prefs->isautodynupswitching))
		//MORPH END   - Added by SiRoB, Avoid change of the MaxDownloadLimit when we are in dyn upload
			app_prefs->prefs->maxdownload = app_prefs->GetMaxDownload();

		CString strBuffer;
		
		strBuffer.Format("%d", app_prefs->prefs->udpport);
		GetDlgItem(IDC_UDPPORT)->SetWindowText(strBuffer);
		CheckDlgButton(IDC_UDPDISABLE,(app_prefs->prefs->udpport==0));

		GetDlgItem(IDC_UDPPORT)->EnableWindow(app_prefs->prefs->udpport>0);
	
		strBuffer.Format("%d", app_prefs->prefs->kadudpport);
		GetDlgItem(IDC_KADUDPPORT)->SetWindowText(strBuffer);


		strBuffer.Format("%d", app_prefs->prefs->maxGraphDownloadRate);
		GetDlgItem(IDC_DOWNLOAD_CAP)->SetWindowText(strBuffer);
		
		((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetRange(1, app_prefs->prefs->maxGraphDownloadRate);

		strBuffer.Format("%d", app_prefs->prefs->maxGraphUploadRate);
		GetDlgItem(IDC_UPLOAD_CAP)->SetWindowText(strBuffer);

		((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetRange(1, app_prefs->prefs->maxGraphUploadRate);


		CheckDlgButton( IDC_DLIMIT_LBL, (app_prefs->prefs->maxdownload!=UNLIMITED) );
		CheckDlgButton( IDC_ULIMIT_LBL, (app_prefs->prefs->maxupload!=UNLIMITED) );

		((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetPos(
			(app_prefs->prefs->maxdownload!=UNLIMITED)?app_prefs->prefs->maxdownload:app_prefs->prefs->maxGraphDownloadRate
		);
		
		((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetPos(
			(app_prefs->prefs->maxupload!=UNLIMITED)?app_prefs->prefs->maxupload:app_prefs->prefs->maxGraphUploadRate
		);

		strBuffer.Format("%d", app_prefs->prefs->port);
		GetDlgItem(IDC_PORT)->SetWindowText(strBuffer);

		strBuffer.Format("%d", app_prefs->prefs->maxconnections);
		GetDlgItem(IDC_MAXCON)->SetWindowText(strBuffer);

		if(app_prefs->prefs->maxsourceperfile == 0xFFFF)
			GetDlgItem(IDC_MAXSOURCEPERFILE)->SetWindowText("0");
		else
		{
			strBuffer.Format("%d", app_prefs->prefs->maxsourceperfile);
			GetDlgItem(IDC_MAXSOURCEPERFILE)->SetWindowText(strBuffer);
		}

		if (app_prefs->prefs->reconnect)
			CheckDlgButton(IDC_RECONN,1);
		else
			CheckDlgButton(IDC_RECONN,0);
		
		if (app_prefs->prefs->m_bshowoverhead)
			CheckDlgButton(IDC_SHOWOVERHEAD,1);
		else
			CheckDlgButton(IDC_SHOWOVERHEAD,0);

		if (app_prefs->prefs->autoconnect)
			CheckDlgButton(IDC_AUTOCONNECT,1);
		else
			CheckDlgButton(IDC_AUTOCONNECT,0);

		if (app_prefs->prefs->networkkademlia)
			CheckDlgButton(IDC_NETWORK_KADEMLIA,1);
		else
			CheckDlgButton(IDC_NETWORK_KADEMLIA,0);

		if (app_prefs->prefs->networked2k)
			CheckDlgButton(IDC_NETWORK_ED2K,1);
		else
			CheckDlgButton(IDC_NETWORK_ED2K,0);

		ShowLimitValues();
		OnLimiterChange();
	}
}

BOOL CPPgConnection::OnApply()
{
	char buffer[510];
	int lastmaxgu = app_prefs->prefs->maxGraphUploadRate;
	int lastmaxgd = app_prefs->prefs->maxGraphDownloadRate;
	bool bRestartApp = false;

	if(GetDlgItem(IDC_DOWNLOAD_CAP)->GetWindowTextLength())
	{ 
		GetDlgItem(IDC_DOWNLOAD_CAP)->GetWindowText(buffer,20);
		app_prefs->SetMaxGraphDownloadRate(atoi(buffer));
	}

	((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetRange(1, app_prefs->GetMaxGraphDownloadRate(), TRUE);

	if(GetDlgItem(IDC_UPLOAD_CAP)->GetWindowTextLength())
	{
		GetDlgItem(IDC_UPLOAD_CAP)->GetWindowText(buffer,20);
		app_prefs->SetMaxGraphUploadRate(atoi(buffer));
	}

	((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetRange(1, app_prefs->GetMaxGraphUploadRate(), TRUE);

	if (IsDlgButtonChecked(IDC_ULIMIT_LBL)==FALSE) app_prefs->SetMaxUpload(UNLIMITED);
	else app_prefs->SetMaxUpload(((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->GetPos());

	if( app_prefs->GetMaxGraphUploadRate() < app_prefs->GetMaxUpload() && app_prefs->GetMaxUpload()!=UNLIMITED  )
		app_prefs->SetMaxUpload(app_prefs->GetMaxGraphUploadRate()*.8);

	if (app_prefs->GetMaxUpload()!=UNLIMITED)
	((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetPos(app_prefs->GetMaxUpload());
	
	if (IsDlgButtonChecked(IDC_DLIMIT_LBL)==FALSE) app_prefs->SetMaxDownload(UNLIMITED);
	else app_prefs->SetMaxDownload(((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->GetPos());

	if( app_prefs->GetMaxGraphDownloadRate() < app_prefs->GetMaxDownload() && app_prefs->GetMaxDownload()!=UNLIMITED )
		app_prefs->SetMaxDownload(app_prefs->GetMaxGraphDownloadRate()*.8);

	if (app_prefs->GetMaxDownload()!=UNLIMITED)
	((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetPos(app_prefs->GetMaxDownload());
	
	if(GetDlgItem(IDC_PORT)->GetWindowTextLength())
	{
		GetDlgItem(IDC_PORT)->GetWindowText(buffer,20);
		uint16 nNewPort = (atoi(buffer)) ? atoi(buffer) : 4662;
		if (nNewPort != app_prefs->prefs->port){
			app_prefs->prefs->port = nNewPort;
			bRestartApp = true;
		}
	}
	
	if(GetDlgItem(IDC_MAXSOURCEPERFILE)->GetWindowTextLength())
	{
		GetDlgItem(IDC_MAXSOURCEPERFILE)->GetWindowText(buffer,20);
		app_prefs->prefs->maxsourceperfile = (atoi(buffer)) ? atoi(buffer) : 1;
	}

	if(GetDlgItem(IDC_UDPPORT)->GetWindowTextLength())
	{
		GetDlgItem(IDC_UDPPORT)->GetWindowText(buffer,20);
		uint16 nNewPort = (atoi(buffer) && !IsDlgButtonChecked(IDC_UDPDISABLE) ) ? atoi(buffer) : 0;
		if (nNewPort != app_prefs->prefs->udpport){
			app_prefs->prefs->udpport = nNewPort;
			bRestartApp = true;
		}
	}

	if(GetDlgItem(IDC_KADUDPPORT)->GetWindowTextLength())
	{
		GetDlgItem(IDC_KADUDPPORT)->GetWindowText(buffer,20);
		uint16 nNewPort = (atoi(buffer)) ? atoi(buffer) : 4673;
		if (nNewPort != app_prefs->prefs->kadudpport){
			app_prefs->prefs->kadudpport = nNewPort;
			bRestartApp = true;
		}
	}

	if(IsDlgButtonChecked(IDC_SHOWOVERHEAD))
		app_prefs->prefs->m_bshowoverhead = true;
	else
		app_prefs->prefs->m_bshowoverhead = false;

	if(IsDlgButtonChecked(IDC_NETWORK_KADEMLIA))
		app_prefs->prefs->networkkademlia = true;
	else
		app_prefs->prefs->networkkademlia = false;

	theApp.emuledlg->SetKadButtonState();

	if(IsDlgButtonChecked(IDC_NETWORK_ED2K))
		app_prefs->prefs->networked2k = true;
	else
		app_prefs->prefs->networked2k = false;
	
	//	if(IsDlgButtonChecked(IDC_UDPDISABLE)) app_prefs->prefs->udpport=0;
	GetDlgItem(IDC_UDPPORT)->EnableWindow(!IsDlgButtonChecked(IDC_UDPDISABLE));

	app_prefs->prefs->autoconnect = (int8)IsDlgButtonChecked(IDC_AUTOCONNECT);
	app_prefs->prefs->reconnect = (int8)IsDlgButtonChecked(IDC_RECONN);
		
	if(lastmaxgu != app_prefs->prefs->maxGraphUploadRate) 
		theApp.emuledlg->statisticswnd.SetARange(false,app_prefs->prefs->maxGraphUploadRate);
	if(lastmaxgd!=app_prefs->prefs->maxGraphDownloadRate)
		theApp.emuledlg->statisticswnd.SetARange(true,app_prefs->prefs->maxGraphDownloadRate);

	uint16 tempcon = app_prefs->prefs->maxconnections;
	if(GetDlgItem(IDC_MAXCON)->GetWindowTextLength())
	{
		GetDlgItem(IDC_MAXCON)->GetWindowText(buffer,20);
		tempcon = (atoi(buffer)) ? atoi(buffer) : CPreferences::GetRecommendedMaxConnections();
	}

	if(tempcon > (unsigned)::GetMaxConnections())
	{
		CString strMessage;
		strMessage.Format(GetResString(IDS_PW_WARNING), GetResString(IDS_PW_MAXC), ::GetMaxConnections());
		int iResult = AfxMessageBox(strMessage, MB_ICONWARNING | MB_YESNO);
		if(iResult != IDYES)
		{
			//TODO: set focus to max connection?
			strMessage.Format("%d", app_prefs->prefs->maxconnections);
			GetDlgItem(IDC_MAXCON)->SetWindowText(strMessage);
			tempcon = ::GetMaxConnections();
		}
	}
	app_prefs->prefs->maxconnections = tempcon;
	theApp.scheduler->SaveOriginals();

	//if (app_prefs->prefs->maxGraphDownloadRate<app_prefs->prefs->maxdownload) app_prefs->prefs->maxdownload=UNLIMITED;
	//if (app_prefs->prefs->maxGraphUploadRate<app_prefs->prefs->maxupload) app_prefs->prefs->maxupload=UNLIMITED;

	SetModified(FALSE);
	LoadSettings();

	theApp.emuledlg->ShowConnectionState();

//	app_prefs->Save();
//	theApp.emuledlg->preferenceswnd->m_wndTweaks.LoadSettings();

	if (bRestartApp)
		AfxMessageBox(GetResString(IDS_SETTINGCHANGED_RESTART));
	return CPropertyPage::OnApply();
}

void CPPgConnection::Localize(void)
{	
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_CONNECTION));
		
		GetDlgItem(IDC_CAPACITIES_FRM)->SetWindowText(GetResString(IDS_PW_CON_CAPFRM));
		GetDlgItem(IDC_DCAP_LBL)->SetWindowText(GetResString(IDS_PW_CON_DOWNLBL));
		GetDlgItem(IDC_UCAP_LBL)->SetWindowText(GetResString(IDS_PW_CON_UPLBL));
		
		GetDlgItem(IDC_LIMITS_FRM)->SetWindowText(GetResString(IDS_PW_CON_LIMITFRM));

		GetDlgItem(IDC_DLIMIT_LBL)->SetWindowText(GetResString(IDS_PW_DOWNL));
		GetDlgItem(IDC_ULIMIT_LBL)->SetWindowText(GetResString(IDS_PW_UPL));

//		GetDlgItem(IDC_KBS1)->SetWindowText(GetResString(IDS_KBYTESEC));
		GetDlgItem(IDC_KBS2)->SetWindowText(GetResString(IDS_KBYTESEC));
		GetDlgItem(IDC_KBS3)->SetWindowText(GetResString(IDS_KBYTESEC));
//		GetDlgItem(IDC_KBS4)->SetWindowText(GetResString(IDS_KBYTESEC));

		ShowLimitValues();

		GetDlgItem(IDC_MAXCONN_FRM)->SetWindowText(GetResString(IDS_PW_CONLIMITS));
		GetDlgItem(IDC_MAXCONLABEL)->SetWindowText(GetResString(IDS_PW_MAXC));

		GetDlgItem(IDC_SHOWOVERHEAD)->SetWindowText(GetResString(IDS_SHOWOVERHEAD));
		
		GetDlgItem(IDC_CLIENTPORT_FRM)->SetWindowText(GetResString(IDS_PW_CLIENTPORT));
		GetDlgItem(IDC_MAXSRC_FRM)->SetWindowText(GetResString(IDS_PW_MAXSOURCES));

		GetDlgItem(IDC_AUTOCONNECT)->SetWindowText(GetResString(IDS_PW_AUTOCON));
		GetDlgItem(IDC_RECONN)->SetWindowText(GetResString(IDS_PW_RECON));
		GetDlgItem(IDC_MAXSRCHARD_LBL)->SetWindowText(GetResString(IDS_HARDLIMIT));
		GetDlgItem(IDC_WIZARD)->SetWindowText(GetResString(IDS_WIZARD));
		GetDlgItem(IDC_UDPDISABLE)->SetWindowText(GetResString(IDS_UDPDISABLED));
	}
}

void CPPgConnection::OnBnClickedWizard()
{
	Wizard conWizard;
	conWizard.SetPrefs( app_prefs );
	conWizard.DoModal();
}

void CPPgConnection::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	SetModified(TRUE);

	if( pScrollBar == (CScrollBar*)GetDlgItem(IDC_MAXUP_SLIDER))
	{
		uint32 maxup = ((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->GetPos();
		uint32 maxdown = ((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->GetPos();
		if( maxup < 4 && maxup*3 < maxdown)
		{
			((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetPos(maxup*3);
		}
		if( maxup < 10 && maxup*4 < maxdown)
		{
			((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->SetPos(maxup*4);
		}
	}
	else if (pScrollBar == (CScrollBar*)GetDlgItem(IDC_MAXDOWN_SLIDER))
	{
		uint32 maxup = ((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->GetPos();
		uint32 maxdown = ((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->GetPos();
		if( maxdown < 13 && maxup*3 < maxdown)
		{
			((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetPos(ceil((double)maxdown/3));
		}
		if( maxdown < 41 && maxup*4 < maxdown)
		{
			((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->SetPos(ceil((double)maxdown/4));
		}
	}

	ShowLimitValues();

	UpdateData(false); 
	CPropertyPage::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CPPgConnection::ShowLimitValues() {
	CString buffer;

	if (IsDlgButtonChecked(IDC_ULIMIT_LBL)==FALSE) buffer="";
	else buffer.Format("%u %s", ((CSliderCtrl*)GetDlgItem(IDC_MAXUP_SLIDER))->GetPos(), GetResString(IDS_KBYTESEC));
	GetDlgItem(IDC_KBS4)->SetWindowText(buffer);
	
	if (IsDlgButtonChecked(IDC_DLIMIT_LBL)==FALSE) buffer="";
	else buffer.Format("%u %s", ((CSliderCtrl*)GetDlgItem(IDC_MAXDOWN_SLIDER))->GetPos(), GetResString(IDS_KBYTESEC));
	GetDlgItem(IDC_KBS1)->SetWindowText(buffer);
}

void CPPgConnection::OnLimiterChange() {
	
	GetDlgItem(IDC_MAXDOWN_SLIDER)->ShowWindow( IsDlgButtonChecked(IDC_DLIMIT_LBL)?SW_SHOW:SW_HIDE);
	GetDlgItem(IDC_MAXUP_SLIDER)->ShowWindow( IsDlgButtonChecked(IDC_ULIMIT_LBL)?SW_SHOW:SW_HIDE );

	ShowLimitValues();
	SetModified(TRUE);	
}