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
	m_htiPayBackFirstLimit = NULL; //MORPH - Added by SiRoB, Pay Back First Tweak
	m_htiOnlyDownloadCompleteFiles = NULL;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	m_htiSaveUploadQueueWaitTime = NULL;//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	m_htiEnableChunkDots = NULL; //EastShare - Added by Pretender, Option for ChunkDots

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
	m_htiEnableEqualChanceForEachFile = NULL;
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
		CImageList* piml = m_ctrlTreeOptions.GetImageList(TVSIL_NORMAL);
		if (piml){
			iImgIP2Country = piml->Add(CTempIconLoader(_T("SEARCHMETHOD_GLOBAL"))); //EastShare - added by AndCycle, IP to Country
			iImgCS = piml->Add(CTempIconLoader(_T("STATSCLIENTS"))); // EastShare START - Added by Pretender, CS icon
			iImgMETC = piml->Add(CTempIconLoader(_T("HARDDISK"))); // EastShare START - Added by TAHO, .met control
		}
		m_htiEnablePreferShareAll = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_PREFER_SHARE_ALL), TVI_ROOT, m_bEnablePreferShareAll);//EastShare - PreferShareAll by AndCycle
		m_htiOnlyDownloadCompleteFiles = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_ONLY_DOWNLOAD_COMPLETE_FILES), TVI_ROOT, m_bOnlyDownloadCompleteFiles);//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
		m_htiSaveUploadQueueWaitTime = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_SAVE_UPLOAD_QUEUE_WAIT_TIME), TVI_ROOT, m_bSaveUploadQueueWaitTime);//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
		m_htiEnableChunkDots = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_ENABLE_CHUNKDOTS), TVI_ROOT, m_bEnableChunkDots);//EastShare - Added by Pretender, Option for ChunkDots

		//Morph - added by AndCycle, Equal Chance For Each File
		m_htiEnableEqualChanceForEachFile = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_ECFEF), TVI_ROOT, m_iEnableEqualChanceForEachFile);
		//Morph - added by AndCycle, Equal Chance For Each File

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
		m_htiOfficialCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_OFFICIAL_CREDIT), m_htiCreditSystem, m_iCreditSystem == 0);
		m_htiLovelaceCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_LOVELACE_CREDIT), m_htiCreditSystem, m_iCreditSystem == 1);
		//m_htiRatioCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_RATIO_CREDIT), m_htiCreditSystem, m_iCreditSystem == CS_RATIO);
		m_htiPawcioCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_PAWCIO_CREDIT), m_htiCreditSystem, m_iCreditSystem == 2);
		m_htiESCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_EASTSHARE_CREDIT), m_htiCreditSystem, m_iCreditSystem == 3);
		//EastShare End - CreditSystemSelection
		m_htiIsPayBackFirst = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_PAYBACKFIRST), m_htiCreditSystem, m_bIsPayBackFirst);//EastShare - added by AndCycle, Pay Back First
		// EastShare END - Added by linekin, new creditsystem by [lovelace]
		//MORPH START - Added by SiRoB, Pay Back First Tweak
		m_htiPayBackFirstLimit = m_ctrlTreeOptions.InsertItem(GetResString(IDS_PAYBACKFIRSTLIMIT),TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT,m_htiIsPayBackFirst);
		m_ctrlTreeOptions.AddEditBox(m_htiPayBackFirstLimit, RUNTIME_CLASS(CNumTreeOptionsEdit));
		m_ctrlTreeOptions.Expand(m_htiIsPayBackFirst, TVE_EXPAND);
		//MORPH END   - Added by SiRoB, Pay Back First Tweak
		
		// EastShare START - Added by TAHO, .met control // Modified by Pretender
		m_htiMetControl = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_MET_FILE_CONTROL), iImgMETC, TVI_ROOT);
		m_htiKnownMet = m_ctrlTreeOptions.InsertItem(GetResString(IDS_EXPIRED_KNOWN), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiMetControl);
		m_ctrlTreeOptions.AddEditBox(m_htiKnownMet, RUNTIME_CLASS(CNumTreeOptionsEdit));
		// EastShare END - Added by TAHO, .met control

		// EastShare START - Added by Pretender
		m_ctrlTreeOptions.Expand(m_htiIP2CountryName, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiCreditSystem, TVE_EXPAND);//EastShare
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

	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiEnableEqualChanceForEachFile, m_iEnableEqualChanceForEachFile);//Morph - added by AndCycle, Equal Chance For Each File

	// EastShare START - Added by TAHO, .met flies Control
	DDX_TreeEdit(pDX, IDC_EASTSHARE_OPTS, m_htiKnownMet, m_iKnownMetDays);
	// EastShare END - Added by TAHO, .met flies Control
	
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiEnablePreferShareAll, m_bEnablePreferShareAll);//EastShare - PreferShareAll by AndCycle
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiIsPayBackFirst, m_bIsPayBackFirst);//EastShare - added by AndCycle, Pay Back First
	DDX_TreeEdit(pDX, IDC_EASTSHARE_OPTS, m_htiPayBackFirstLimit, m_iPayBackFirstLimit); //MORPH - Added by SiRoB, Pay Back First Tweak
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiOnlyDownloadCompleteFiles, m_bOnlyDownloadCompleteFiles);//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiSaveUploadQueueWaitTime, m_bSaveUploadQueueWaitTime);//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiEnableChunkDots, m_bEnableChunkDots);//EastShare - Added by Pretender, Option for ChunkDots
}


BOOL CPPgEastShare::OnInitDialog()
{

	m_bEnablePreferShareAll = thePrefs.shareall;//EastShare - PreferShareAll by AndCycle
	m_bIsPayBackFirst = thePrefs.m_bPayBackFirst;//EastShare - added by AndCycle, Pay Back First
	m_iPayBackFirstLimit = thePrefs.m_iPayBackFirstLimit;//MORPH - Added by SiRoB, Pay Back First Tweak
	m_bOnlyDownloadCompleteFiles = thePrefs.m_bOnlyDownloadCompleteFiles;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	m_bSaveUploadQueueWaitTime = thePrefs.m_bSaveUploadQueueWaitTime;//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	m_bEnableChunkDots = thePrefs.m_bEnableChunkDots;//EastShare - Added by Pretender, Option for ChunkDots

	//EastShare Start - added by AndCycle, IP to Country
	m_iIP2CountryName = thePrefs.GetIP2CountryNameMode(); 
	m_bIP2CountryShowFlag = thePrefs.IsIP2CountryShowFlag();
	//EastShare End - added by AndCycle, IP to Country

	m_iCreditSystem = thePrefs.GetCreditSystem(); //EastShare - Added by linekin , CreditSystem 
	m_iEnableEqualChanceForEachFile = thePrefs.IsEqualChanceEnable();//Morph - added by AndCycle, Equal Chance For Each File

	m_iKnownMetDays = thePrefs.GetKnownMetDays(); //EastShare - Added by TAHO , .met file control
	
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

	thePrefs.shareall = m_bEnablePreferShareAll;//EastShare - PreferShareAll by AndCycle
	thePrefs.m_bPayBackFirst = m_bIsPayBackFirst;//EastShare - added by AndCycle, Pay Back First
	thePrefs.m_iPayBackFirstLimit = m_iPayBackFirstLimit;//MORPH - Added by SiRoB, Pay Back First Tweak
	thePrefs.m_bOnlyDownloadCompleteFiles = m_bOnlyDownloadCompleteFiles;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	thePrefs.m_bEnableChunkDots = m_bEnableChunkDots;//EastShare - Added by Pretender, Option for ChunkDots

	//EastShare Start - added by AndCycle, IP to Country
	if(	(thePrefs.m_iIP2CountryNameMode != IP2CountryName_DISABLE || thePrefs.m_bIP2CountryShowFlag) !=
		(m_iIP2CountryName != IP2CountryName_DISABLE || m_bIP2CountryShowFlag)	){
		//check if need to load or unload DLL and ip table
		if(m_iIP2CountryName != IP2CountryName_DISABLE || m_bIP2CountryShowFlag){
			theApp.ip2country->Load();
		}
		else{
			theApp.ip2country->Unload();
		}
	}
	thePrefs.m_iIP2CountryNameMode = m_iIP2CountryName;
	thePrefs.m_bIP2CountryShowFlag = m_bIP2CountryShowFlag;
	theApp.ip2country->Refresh();//refresh passive windows
	//EastShare End - added by AndCycle, IP to Country

	//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	if((bool)m_bSaveUploadQueueWaitTime != thePrefs.m_bSaveUploadQueueWaitTime)	bRestartApp = true;
	thePrefs.m_bSaveUploadQueueWaitTime = m_bSaveUploadQueueWaitTime;
	//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)

/*	theApp.emuledlg->serverwnd->ToggleDebugWindow();
	theApp.emuledlg->serverwnd->UpdateLogTabSelection(); */


	thePrefs.creditSystemMode = m_iCreditSystem; //EastShare - Added by linekin , CreditSystem 

	//Morph - added by AndCycle, Equal Chance For Each File
	thePrefs.m_bEnableEqualChanceForEachFile = m_iEnableEqualChanceForEachFile;
	//Morph - added by AndCycle, Equal Chance For Each File

	thePrefs.SetKnownMetDays( m_iKnownMetDays); //EastShare - Added by TAHO , .met file control

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
		if (m_htiPayBackFirstLimit) m_ctrlTreeOptions.SetEditLabel(m_htiPayBackFirstLimit, GetResString(IDS_PAYBACKFIRSTLIMIT));//MORPH - Added by SiRoB, Pay Back First Tweak
		if (m_htiOnlyDownloadCompleteFiles) m_ctrlTreeOptions.SetItemText(m_htiOnlyDownloadCompleteFiles,GetResString(IDS_ONLY_DOWNLOAD_COMPLETE_FILES));//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
		if (m_htiSaveUploadQueueWaitTime) m_ctrlTreeOptions.SetItemText(m_htiSaveUploadQueueWaitTime,GetResString(IDS_SAVE_UPLOAD_QUEUE_WAIT_TIME));//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
		if (m_htiEnableChunkDots) m_ctrlTreeOptions.SetItemText(m_htiEnableChunkDots, GetResString(IDS_ENABLE_CHUNKDOTS));//EastShare - Added by Pretender, Option for ChunkDots

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
	m_htiPayBackFirstLimit = NULL; //MORPH - Added by SiRoB, Pay Back First Tweak
	m_htiOnlyDownloadCompleteFiles = NULL;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	m_htiSaveUploadQueueWaitTime = NULL;//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	m_htiEnableChunkDots = NULL; //EastShare - Added by Pretender, Option for ChunkDots

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
	m_htiEnableEqualChanceForEachFile = NULL;
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