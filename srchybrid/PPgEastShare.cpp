// PpgEastShare.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgEastShare.h"
#include "OtherFunctions.h"

//EastShare Start - added by AndCycle, IP to Country
#include "ip2country.h"
//EastShare End - added by AndCycle, IP to Country

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

///////////////////////////////////////////////////////////////////////////////
// CPPgEastShare dialog

IMPLEMENT_DYNAMIC(CPPgEastShare, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgEastShare, CPropertyPage)
	ON_WM_HSCROLL()
    ON_WM_DESTROY()
	ON_MESSAGE(WM_TREEOPTSCTRL_NOTIFY, OnTreeOptsCtrlNotify)
END_MESSAGE_MAP()

CPPgEastShare::CPPgEastShare()
	: CPropertyPage(CPPgEastShare::IDD)
	, m_ctrlTreeOptions(theApp.m_iDfltImageListColorFlags)
{
	m_bInitializedTreeOpts = false;
	m_htiEnablePreferShareAll = NULL; //EastShare - PreferShareAll by AndCycle
	m_htiIsPayBackFirst = NULL; //EastShare - added by AndCycle, Pay Back First
	m_htiOnlyDownloadCompleteFiles = NULL;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	m_htiSaveUploadQueueWaitTime = NULL;//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)

	//EastShare Start - added by AndCycle, IP to Country
	m_htiIP2CountryName = NULL;
	m_htiIP2CountryName_DISABLE = NULL;
	m_htiIP2CountryName_SHORT = NULL;
	m_htiIP2CountryName_MID = NULL;
	m_htiIP2CountryName_LONG = NULL;
	m_htiIP2CountryShowFlag = NULL;
	//EastShare End - added by AndCycle, IP to Country

	//EastShare START - Added by Pretender
	m_htiCreditSystem = NULL;
	m_htiOfficialCredit = NULL;
	m_htiLovelaceCredit = NULL;
	m_htiRatioCredit = NULL;
	m_htiPawcioCredit = NULL;
	m_htiESCredit = NULL;
	//EastShare END - Added by Pretender

	//Morph - added by AndCycle, Equal Chance For Each File
	m_htiECFEF = NULL;
	m_htiECFEF_DISABLE = NULL;
	m_htiECFEF_ACCEPTED = NULL;
	m_htiECFEF_ACCEPTED_COMPLETE = NULL;
	m_htiECFEF_TRANSFERRED = NULL;
	m_htiECFEF_TRANSFERRED_COMPLETE = NULL;
	m_htiECFEF_ALLTIME = NULL;
	//Morph - added by AndCycle, Equal Chance For Each File

	//EastShare START - Added by TAHO, .met control
	m_htiMetControl = NULL;
	m_htiKnownMet = NULL;
	//EastShare END - Added by TAHO, .met control
}

CPPgEastShare::~CPPgEastShare()
{
}

void CPPgEastShare::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EASTSHARE_OPTS, m_ctrlTreeOptions);
	if (!m_bInitializedTreeOpts)
	{
		int iImgIP2Country = 8; //EastShare - added by AndCycle, IP to Country
		int iImgCS = 8; //EastShare Added by linekin, CreditSystem
		int iImgMETC = 8; //EastShare Added by TAHO, .met Control
		int iImgECFEF = 8; //Morph - added by AndCycle, Equal Chance For Each File
		CImageList* piml = m_ctrlTreeOptions.GetImageList(TVSIL_NORMAL);
		if (piml){
			iImgIP2Country = piml->Add(CTempIconLoader("SEARCHMETHOD_GLOBAL")); //EastShare - added by AndCycle, IP to Country
			iImgCS = piml->Add(CTempIconLoader("STATSCLIENTS")); // EastShare START - Added by Pretender, CS icon
			iImgMETC = piml->Add(CTempIconLoader("HARDDISK")); // EastShare START - Added by TAHO, .met control
			iImgECFEF = piml->Add(CTempIconLoader("EQFILE"));//Morph - added by AndCycle, Equal Chance For Each File
		}
		m_htiEnablePreferShareAll = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_PREFER_SHARE_ALL), TVI_ROOT, m_bEnablePreferShareAll);//EastShare - PreferShareAll by AndCycle
		m_htiOnlyDownloadCompleteFiles = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_ONLY_DOWNLOAD_COMPLETE_FILES), TVI_ROOT, m_bOnlyDownloadCompleteFiles);//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
		m_htiSaveUploadQueueWaitTime = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_SAVE_UPLOAD_QUEUE_WAIT_TIME), TVI_ROOT, m_bSaveUploadQueueWaitTime);//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)

		//EastShare Start - added by AndCycle, IP to Country
		m_htiIP2CountryName = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_IP2COUNTRY), iImgIP2Country, TVI_ROOT);
		m_htiIP2CountryName_DISABLE = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_DISABLED), m_htiIP2CountryName, m_iIP2CountryName == IP2CountryName_DISABLE);
		m_htiIP2CountryName_SHORT = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_COUNTRYNAME_SHORT), m_htiIP2CountryName, m_iIP2CountryName == IP2CountryName_SHORT);
		m_htiIP2CountryName_MID = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_COUNTRYNAME_MID), m_htiIP2CountryName, m_iIP2CountryName == IP2CountryName_MID);
		m_htiIP2CountryName_LONG = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_COUNTRYNAME_LONG), m_htiIP2CountryName, m_iIP2CountryName == IP2CountryName_LONG);
		m_htiIP2CountryShowFlag = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_COUNTRYNAME_SHOWFLAG), m_htiIP2CountryName, m_bIP2CountryShowFlag);
		//EastShare End - added by AndCycle, IP to Country

		// EastShare START - Added by linekin, new creditsystem by [lovelace]  // Modified by Pretender
		m_htiCreditSystem = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_CREDIT_SYSTEM), iImgCS, TVI_ROOT);
		//EastShare Start - CreditSystemSelection
		m_htiOfficialCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_OFFICIAL_CREDIT), m_htiCreditSystem, m_iCreditSystem == CS_OFFICIAL);
		m_htiLovelaceCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_LOVELACE_CREDIT), m_htiCreditSystem, m_iCreditSystem == CS_LOVELACE);
//		m_htiRatioCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_RATIO_CREDIT), m_htiCreditSystem, m_iCreditSystem == CS_RATIO);
		m_htiPawcioCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_PAWCIO_CREDIT), m_htiCreditSystem, m_iCreditSystem == CS_PAWCIO);
		m_htiESCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_EASTSHARE_CREDIT), m_htiCreditSystem, m_iCreditSystem == CS_EASTSHARE);
		//EastShare End - CreditSystemSelection
		m_htiIsPayBackFirst = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_PAYBACKFIRST), m_htiCreditSystem, m_bIsPayBackFirst);//EastShare - added by AndCycle, Pay Back First
		// EastShare END - Added by linekin, new creditsystem by [lovelace]

		//Morph - added by AndCycle, Equal Chance For Each File
		m_htiECFEF = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_ECFEF), iImgECFEF, TVI_ROOT);
		m_htiECFEF_DISABLE = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_DISABLED), m_htiECFEF, m_iEqualChanceForEachFile == ECFEF_DISABLE);
		m_htiECFEF_ACCEPTED = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_SF_ACCEPTS), m_htiECFEF, m_iEqualChanceForEachFile == ECFEF_ACCEPTED);
		m_htiECFEF_ACCEPTED_COMPLETE = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_SF_ACCEPTS) + "/" + GetResString(IDS_COMPLETE), m_htiECFEF, m_iEqualChanceForEachFile == ECFEF_ACCEPTED_COMPLETE);
		m_htiECFEF_TRANSFERRED = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_SF_TRANSFERRED), m_htiECFEF, m_iEqualChanceForEachFile == ECFEF_TRANSFERRED);
		m_htiECFEF_TRANSFERRED_COMPLETE = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_SF_TRANSFERRED) + "/" + GetResString(IDS_COMPLETE), m_htiECFEF, m_iEqualChanceForEachFile == ECFEF_TRANSFERRED_COMPLETE);
		m_htiECFEF_ALLTIME = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_ALL_TIME), m_htiECFEF, m_bECFEFallTime);
		//Morph - added by AndCycle, Equal Chance For Each File

		// EastShare START - Added by TAHO, .met control // Modified by Pretender
		m_htiMetControl = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_MET_FILE_CONTROL), iImgMETC, TVI_ROOT);
		m_htiKnownMet = m_ctrlTreeOptions.InsertItem(GetResString(IDS_EXPIRED_KNOWN), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiMetControl);
		m_ctrlTreeOptions.AddEditBox(m_htiKnownMet, RUNTIME_CLASS(CNumTreeOptionsEdit));
		// EastShare END - Added by TAHO, .met control

		// EastShare START - Added by Pretender
		m_ctrlTreeOptions.Expand(m_htiIP2CountryName, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiCreditSystem, TVE_EXPAND);//EastShare
		m_ctrlTreeOptions.Expand(m_htiECFEF, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiMetControl, TVE_EXPAND);
		// EastShare END - Added by Pretender
		m_ctrlTreeOptions.SendMessage(WM_VSCROLL, SB_TOP);
		m_bInitializedTreeOpts = true;
	}

	//this is bad using enum for radio button...need (int &) ^*&^#*^$(, by AndCycle

	//EastShare - added by AndCycle, IP to Country
	DDX_TreeRadio(pDX, IDC_EASTSHARE_OPTS, m_htiIP2CountryName, (int &)m_iIP2CountryName);
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiIP2CountryShowFlag, m_bIP2CountryShowFlag);
	//EastShare - added by AndCycle, IP to Country

	DDX_TreeRadio(pDX, IDC_EASTSHARE_OPTS, m_htiCreditSystem, (int &)m_iCreditSystem); //EastShare - added by linekin , CreditSystem

	DDX_TreeRadio(pDX, IDC_EASTSHARE_OPTS, m_htiECFEF, (int &)m_iEqualChanceForEachFile);//Morph - added by AndCycle, Equal Chance For Each File
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiECFEF_ALLTIME, m_bECFEFallTime);//Morph - added by AndCycle, Equal Chance For Each File

	// EastShare START - Added by TAHO, .met flies Control
	DDX_TreeEdit(pDX, IDC_EASTSHARE_OPTS, m_htiKnownMet, m_iKnownMetDays);
	// EastShare END - Added by TAHO, .met flies Control
	
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiEnablePreferShareAll, m_bEnablePreferShareAll);//EastShare - PreferShareAll by AndCycle
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiIsPayBackFirst, m_bIsPayBackFirst);//EastShare - added by AndCycle, Pay Back First
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiOnlyDownloadCompleteFiles, m_bOnlyDownloadCompleteFiles);//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiSaveUploadQueueWaitTime, m_bSaveUploadQueueWaitTime);//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
}


BOOL CPPgEastShare::OnInitDialog()
{

	m_bEnablePreferShareAll = app_prefs->prefs->shareall;//EastShare - PreferShareAll by AndCycle
	m_bIsPayBackFirst = app_prefs->prefs->m_bPayBackFirst;//EastShare - added by AndCycle, Pay Back First
	m_bOnlyDownloadCompleteFiles = app_prefs->prefs->m_bOnlyDownloadCompleteFiles;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	m_bSaveUploadQueueWaitTime = app_prefs->prefs->m_bSaveUploadQueueWaitTime;//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)

	//EastShare Start - added by AndCycle, IP to Country
	m_iIP2CountryName = app_prefs->GetIP2CountryNameMode(); 
	m_bIP2CountryShowFlag = app_prefs->IsIP2CountryShowFlag();
	//EastShare End - added by AndCycle, IP to Country

	m_iCreditSystem = app_prefs->GetCreditSystem(); //EastShare - Added by linekin , CreditSystem 
	m_iEqualChanceForEachFile = app_prefs->GetEqualChanceForEachFileMode();//Morph - added by AndCycle, Equal Chance For Each File
	m_bECFEFallTime = app_prefs->IsECFEFallTime();//Morph - added by AndCycle, Equal Chance For Each File

	m_iKnownMetDays = app_prefs->GetKnownMetDays(); //EastShare - Added by TAHO , .met file control
	
	CPropertyPage::OnInitDialog();
	Localize();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgEastShare::OnKillActive()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	return CPropertyPage::OnKillActive();
}

BOOL CPPgEastShare::OnApply()
{

	bool bRestartApp = false;

	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	
	if (!UpdateData())
		return FALSE;

	app_prefs->prefs->shareall = m_bEnablePreferShareAll;//EastShare - PreferShareAll by AndCycle
	app_prefs->prefs->m_bPayBackFirst = m_bIsPayBackFirst;//EastShare - added by AndCycle, Pay Back First
	app_prefs->prefs->m_bOnlyDownloadCompleteFiles = m_bOnlyDownloadCompleteFiles;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)

	//EastShare Start - added by AndCycle, IP to Country
	if(	(app_prefs->prefs->m_iIP2CountryNameMode != IP2CountryName_DISABLE || app_prefs->prefs->m_bIP2CountryShowFlag) !=
		((IP2CountryNameSelection)m_iIP2CountryName != IP2CountryName_DISABLE || m_bIP2CountryShowFlag)	){
		//check if need to load or unload DLL and ip table
		if((IP2CountryNameSelection)m_iIP2CountryName != IP2CountryName_DISABLE || m_bIP2CountryShowFlag){
			theApp.ip2country->Load();
		}
		else{
			theApp.ip2country->Unload();
		}
	}
	app_prefs->prefs->m_iIP2CountryNameMode = m_iIP2CountryName;
	app_prefs->prefs->m_bIP2CountryShowFlag = m_bIP2CountryShowFlag;
	theApp.ip2country->Refresh();//refresh passive windows
	//EastShare End - added by AndCycle, IP to Country

	//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	if((bool)m_bSaveUploadQueueWaitTime != app_prefs->prefs->m_bSaveUploadQueueWaitTime)	bRestartApp = true;
	app_prefs->prefs->m_bSaveUploadQueueWaitTime = m_bSaveUploadQueueWaitTime;
	//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)

/*	theApp.emuledlg->serverwnd->ToggleDebugWindow();
	theApp.emuledlg->serverwnd->UpdateLogTabSelection(); */


	app_prefs->prefs->creditSystemMode = m_iCreditSystem; //EastShare - Added by linekin , CreditSystem 

	//Morph - added by AndCycle, Equal Chance For Each File
	app_prefs->prefs->equalChanceForEachFileMode = m_iEqualChanceForEachFile;
	app_prefs->prefs->m_bECFEFallTime = m_bECFEFallTime;
	//Morph - added by AndCycle, Equal Chance For Each File

	app_prefs->SetKnownMetDays( m_iKnownMetDays); //EastShare - Added by TAHO , .met file control

	SetModified(FALSE);

	if (bRestartApp){
		AfxMessageBox(GetResString(IDS_SETTINGCHANGED_RESTART));
	}

	return CPropertyPage::OnApply();
}

void CPPgEastShare::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	SetModified(TRUE);
	CSliderCtrl* slider =(CSliderCtrl*)pScrollBar;
	CString temp;
}

void CPPgEastShare::Localize(void)
{	
	if(m_hWnd)
	{
		GetDlgItem(IDC_WARNINGEASTSHARE)->SetWindowText(GetResString(IDS_WARNINGMORPH));

		if (m_htiEnablePreferShareAll) m_ctrlTreeOptions.SetItemText(m_htiEnablePreferShareAll, GetResString(IDS_PREFER_SHARE_ALL));//EastShare - PreferShareAll by AndCycle
		if (m_htiIsPayBackFirst) m_ctrlTreeOptions.SetItemText(m_htiIsPayBackFirst, GetResString(IDS_PAYBACKFIRST));//EastShare - added by AndCycle, Pay Back First
		if (m_htiOnlyDownloadCompleteFiles) m_ctrlTreeOptions.SetItemText(m_htiOnlyDownloadCompleteFiles,GetResString(IDS_ONLY_DOWNLOAD_COMPLETE_FILES));//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
		if (m_htiSaveUploadQueueWaitTime) m_ctrlTreeOptions.SetItemText(m_htiSaveUploadQueueWaitTime,GetResString(IDS_SAVE_UPLOAD_QUEUE_WAIT_TIME));//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)

		//EastShare START - Added By TAHO, .met file control // Modified by Pretender
		if (m_htiKnownMet) m_ctrlTreeOptions.SetEditLabel(m_htiKnownMet, (GetResString(IDS_EXPIRED_KNOWN)));
		//EastShare END - Added By TAHO, .met file control

	}

}

void CPPgEastShare::OnDestroy()
{
	m_ctrlTreeOptions.DeleteAllItems();
	m_ctrlTreeOptions.DestroyWindow();
	m_bInitializedTreeOpts = false;

	m_htiEnablePreferShareAll = NULL; //EastShare - PreferShareAll by AndCycle
	m_htiIsPayBackFirst = NULL; //EastShare - added by AndCycle, Pay Back First
	m_htiOnlyDownloadCompleteFiles = NULL;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	m_htiSaveUploadQueueWaitTime = NULL;//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)

	//EastShare Start - added by AndCycle, IP to Country
	m_htiIP2CountryName = NULL;
	m_htiIP2CountryName_DISABLE = NULL;
	m_htiIP2CountryName_SHORT = NULL;
	m_htiIP2CountryName_MID = NULL;
	m_htiIP2CountryName_LONG = NULL;
	m_htiIP2CountryShowFlag = NULL;
	//EastShare End - added by AndCycle, IP to Country

	//EastShare START - Added by Pretender
	m_htiCreditSystem = NULL;
	m_htiOfficialCredit = NULL;
	m_htiLovelaceCredit = NULL;
	m_htiRatioCredit = NULL;
	m_htiPawcioCredit = NULL;
	m_htiESCredit = NULL;
	//EastShare END - Added by Pretender

	//Morph - added by AndCycle, Equal Chance For Each File
	m_htiECFEF = NULL;
	m_htiECFEF_DISABLE = NULL;
	m_htiECFEF_ACCEPTED = NULL;
	m_htiECFEF_ACCEPTED_COMPLETE = NULL;
	m_htiECFEF_TRANSFERRED = NULL;
	m_htiECFEF_TRANSFERRED_COMPLETE = NULL;
	m_htiECFEF_ALLTIME = NULL;
	//Morph - added by AndCycle, Equal Chance For Each File

	//EastShare START - Added by TAHO, .met control
	m_htiMetControl = NULL;
	m_htiKnownMet = NULL;
	//EastShare END - Added by TAHO, .met control
	
	CPropertyPage::OnDestroy();
}
LRESULT CPPgEastShare::OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDC_EASTSHARE_OPTS){
		TREEOPTSCTRLNOTIFY* pton = (TREEOPTSCTRLNOTIFY*)lParam;
		SetModified();
	}
	return 0;
}