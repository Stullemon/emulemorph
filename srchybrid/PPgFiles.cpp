// PPgFiles.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgFiles.h"
#include "Inputbox.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CPPgFiles dialog

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
	ON_BN_CLICKED(IDC_ICH, OnSettingsChange)
	ON_BN_CLICKED(IDC_UAP, OnSettingsChange)
	ON_BN_CLICKED(IDC_DAP, OnSettingsChange)
	ON_BN_CLICKED(IDC_PREVIEWPRIO, OnSettingsChange)
	ON_BN_CLICKED(IDC_ADDNEWFILESPAUSED, OnSettingsChange)
	ON_BN_CLICKED(IDC_FULLCHUNKTRANS, OnSettingsChange)
	ON_BN_CLICKED(IDC_STARTNEXTFILE, OnSettingsChange)
	ON_BN_CLICKED(IDC_WATCHCB, OnSettingsChange)
	ON_BN_CLICKED(IDC_STARTNEXTFILECAT, OnSettingsChange)
	ON_BN_CLICKED(IDC_FNCLEANUP, OnSettingsChange)
	ON_BN_CLICKED(IDC_FNC, OnSetCleanupFilter)
	ON_EN_CHANGE(IDC_VIDEOPLAYER, OnSettingsChange)
	ON_BN_CLICKED(IDC_VIDEOBACKUP, OnSettingsChange)
	ON_BN_CLICKED(IDC_BROWSEV, BrowseVideoplayer)
END_MESSAGE_MAP()

BOOL CPPgFiles::OnInitDialog()
{
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
	
	//MORPH START - Added by SiRoB, Allways use Fullchunk
	GetDlgItem(IDC_FULLCHUNKTRANS)->EnableWindow(0);
	//MORPH END   - Added by SiRoB, Allways use Fullchunk

	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgFiles::LoadSettings(void)
{	
	CheckRadioButton(IDC_SEESHARE1, IDC_SEESHARE3, IDC_SEESHARE1 + app_prefs->prefs->m_iSeeShares);

	if(app_prefs->prefs->addnewfilespaused)
		CheckDlgButton(IDC_ADDNEWFILESPAUSED,1);
	else
		CheckDlgButton(IDC_ADDNEWFILESPAUSED,0);
	
	if(app_prefs->prefs->ICH)
		CheckDlgButton(IDC_ICH,1);
	else
		CheckDlgButton(IDC_ICH,0);

	if(app_prefs->prefs->m_bpreviewprio)
		CheckDlgButton(IDC_PREVIEWPRIO,1);
	else
		CheckDlgButton(IDC_PREVIEWPRIO,0);

	if(app_prefs->prefs->m_bDAP)
		CheckDlgButton(IDC_DAP,1);
	else
		CheckDlgButton(IDC_DAP,0);

	if(app_prefs->prefs->m_bUAP)
		CheckDlgButton(IDC_UAP,1);
	else
		CheckDlgButton(IDC_UAP,0);

	if(app_prefs->prefs->m_btransferfullchunks)
		CheckDlgButton(IDC_FULLCHUNKTRANS,1);
	else
		CheckDlgButton(IDC_FULLCHUNKTRANS,0);

	if(app_prefs->prefs->m_bstartnextfile)
		CheckDlgButton(IDC_STARTNEXTFILE,1);
	else
		CheckDlgButton(IDC_STARTNEXTFILE,0);

	GetDlgItem(IDC_VIDEOPLAYER)->SetWindowText(app_prefs->prefs->VideoPlayer);
	if(app_prefs->prefs->moviePreviewBackup)
		CheckDlgButton(IDC_VIDEOBACKUP,1);
	else
		CheckDlgButton(IDC_VIDEOBACKUP,0);

	CheckDlgButton(IDC_STARTNEXTFILECAT, (uint8)app_prefs->GetResumeSameCat() );
	CheckDlgButton(IDC_FNCLEANUP, (uint8)app_prefs->AutoFilenameCleanup());

	if(app_prefs->prefs->watchclipboard)
		CheckDlgButton(IDC_WATCHCB,1);
	else
		CheckDlgButton(IDC_WATCHCB,0);
	GetDlgItem(IDC_STARTNEXTFILECAT)->EnableWindow(IsDlgButtonChecked(IDC_STARTNEXTFILE));
}

BOOL CPPgFiles::OnApply()
{
	CString buffer;

	if(IsDlgButtonChecked(IDC_SEESHARE1))
		app_prefs->prefs->m_iSeeShares = 0;
	else if(IsDlgButtonChecked(IDC_SEESHARE2))
		app_prefs->prefs->m_iSeeShares = 1;
	else
		app_prefs->prefs->m_iSeeShares = 2;

	if(IsDlgButtonChecked(IDC_PREVIEWPRIO))
		app_prefs->prefs->m_bpreviewprio = true;
	else
		app_prefs->prefs->m_bpreviewprio = false;

	if(IsDlgButtonChecked(IDC_DAP))
		app_prefs->prefs->m_bDAP = true;
	else
		app_prefs->prefs->m_bDAP = false;

	if(IsDlgButtonChecked(IDC_UAP))
		app_prefs->prefs->m_bUAP = true;
	else
		app_prefs->prefs->m_bUAP = false;

	if(IsDlgButtonChecked(IDC_STARTNEXTFILE))
		app_prefs->prefs->m_bstartnextfile = true;
	else
		app_prefs->prefs->m_bstartnextfile = false;

	app_prefs->prefs->resumeSameCat= (int8)IsDlgButtonChecked(IDC_STARTNEXTFILECAT);


	if(IsDlgButtonChecked(IDC_FULLCHUNKTRANS))
		app_prefs->prefs->m_btransferfullchunks = true;
	else
		app_prefs->prefs->m_btransferfullchunks = false;


	if(IsDlgButtonChecked(IDC_WATCHCB))
		app_prefs->prefs->watchclipboard = true;
	else
		app_prefs->prefs->watchclipboard = false;
	app_prefs->prefs->addnewfilespaused = (int8)IsDlgButtonChecked(IDC_ADDNEWFILESPAUSED);
	app_prefs->prefs->autofilenamecleanup=(int8)IsDlgButtonChecked(IDC_FNCLEANUP);

	app_prefs->prefs->ICH = (int8)IsDlgButtonChecked(IDC_ICH);

	GetDlgItem(IDC_VIDEOPLAYER)->GetWindowText(buffer);
	_snprintf(app_prefs->prefs->VideoPlayer, sizeof app_prefs->prefs->VideoPlayer, "%s", buffer);

	app_prefs->prefs->moviePreviewBackup = IsDlgButtonChecked(IDC_VIDEOBACKUP);

	LoadSettings();

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgFiles::Localize(void)
{
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_FILES));
		GetDlgItem(IDC_ICH_FRM)->SetWindowText(GetResString(IDS_PW_ICH));
		GetDlgItem(IDC_ICH)->SetWindowText(GetResString(IDS_PW_FILE_ICH));
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
	inputbox.SetLabels(GetResString(IDS_FNFILTERTITLE),prompt,theApp.glob_prefs->GetFilenameCleanups());
	inputbox.DoModal();
	if (!inputbox.WasCancelled())
		theApp.glob_prefs->SetFilenameCleanups(inputbox.GetInput());
}

void CPPgFiles::BrowseVideoplayer()
{
	CString strPlayerPath;
	GetDlgItemText(IDC_VIDEOPLAYER, strPlayerPath);
	CFileDialog dlgFile(TRUE, "exe", strPlayerPath,OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY,"Executable (*.exe)|*.exe||", NULL, 0);
	if (dlgFile.DoModal()==IDOK){
		GetDlgItem(IDC_VIDEOPLAYER)->SetWindowText(dlgFile.GetPathName());
		SetModified();
	}
}
