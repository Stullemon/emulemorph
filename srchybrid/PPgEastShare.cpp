// PpgEastShare.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgEastShare.h"

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
//	m_htiIsBoostLess = NULL;//Added by Yun.SF3, boost the less uploaded files //EastShare removed by linekin for CreditSystem integration
	m_htiEnablePreferShareAll = NULL; //EastShare - PreferShareAll by AndCycle
	m_htiIsPayBackFirst = NULL; //EastShare - added by AndCycle, Pay Back First
	//EastShare START - Added by Pretender
	m_htiCreditSystem = NULL;
	m_htiOfficialCredit = NULL;
	m_htiLovelaceCredit = NULL;
	m_htiRatioCredit = NULL;
	m_htiPawcioCredit = NULL;
	m_htiESCredit = NULL;
	m_htiBoostLess = NULL;
	//EastShare END - Added by Pretender
	//EastShare START - Added by TAHO, .met control
	m_htiMetControl = NULL;
//	m_htiClientsMet = NULL;//EastShare - AndCycle, this official setting shoudlnt be change by user
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
		int iImgCS = 8; //EastShare Added by linekin, CreditSystem
		int iImgMETC = 8; //EastShare Added by TAHO, .met Control
		CImageList* piml = m_ctrlTreeOptions.GetImageList(TVSIL_NORMAL);
		if (piml){
			iImgCS = piml->Add(CTempIconLoader("STATSCLIENTS")); // EastShare START - Added by Pretender, CS icon
			iImgMETC = piml->Add(CTempIconLoader("HARDDISK")); // EastShare START - Added by TAHO, .met control
		}
		m_htiEnablePreferShareAll = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_PREFER_SHARE_ALL), TVI_ROOT, m_bEnablePreferShareAll);//EastShare - PreferShareAll by AndCycle
		
		// EastShare START - Added by linekin, new creditsystem by [lovelace]  // Modified by Pretender
		m_htiCreditSystem = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_CREDIT_SYSTEM), iImgCS, TVI_ROOT);
		//EastShare Start - CreditSystemSelection
		m_htiOfficialCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_OFFICIAL_CREDIT), m_htiCreditSystem, m_iCreditSystem == CS_OFFICIAL);
		m_htiLovelaceCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_LOVELACE_CREDIT), m_htiCreditSystem, m_iCreditSystem == CS_LOVELACE);
		m_htiRatioCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_RATIO_CREDIT), m_htiCreditSystem, m_iCreditSystem == CS_RATIO);
		m_htiPawcioCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_PAWCIO_CREDIT), m_htiCreditSystem, m_iCreditSystem == CS_PAWCIO);
		m_htiESCredit = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_EASTSHARE_CREDIT), m_htiCreditSystem, m_iCreditSystem == CS_EASTSHARE);
		//EastShare End - CreditSystemSelection
		m_htiIsPayBackFirst = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_PAYBACKFIRST), m_htiCreditSystem, m_bIsPayBackFirst);//EastShare - added by AndCycle, Pay Back First
		// EastShare END - Added by linekin, new creditsystem by [lovelace]

		// EastShare START - Added by TAHO, .met control // Modified by Pretender
		m_htiMetControl = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_MET_FILE_CONTROL), iImgMETC, TVI_ROOT);
//		m_htiClientsMet = m_ctrlTreeOptions.InsertItem(GetResString(IDS_EXPIRED_CLIENTS), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiMetControl);//EastShare - AndCycle, this official setting shoudlnt be change by user
//		m_ctrlTreeOptions.AddEditBox(m_htiClientsMet, RUNTIME_CLASS(CNumTreeOptionsEdit));//EastShare - AndCycle, this official setting shoudlnt be change by user
		m_htiKnownMet = m_ctrlTreeOptions.InsertItem(GetResString(IDS_EXPIRED_KNOWN), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiMetControl);
		m_ctrlTreeOptions.AddEditBox(m_htiKnownMet, RUNTIME_CLASS(CNumTreeOptionsEdit));
		// EastShare END - Added by TAHO, .met control

		// EastShare START - Added by Pretender
		m_ctrlTreeOptions.Expand(m_htiCreditSystem, TVE_EXPAND);//EastShare - AndCycle, this official setting shoudlnt be change by user
		m_ctrlTreeOptions.Expand(m_htiMetControl, TVE_EXPAND);
		// EastShare END - Added by Pretender
		m_ctrlTreeOptions.SendMessage(WM_VSCROLL, SB_TOP);
		m_bInitializedTreeOpts = true;
	}

	//this is bad using enum for radio button...need (int &) ^*&^#*^$(, by AndCycle
	DDX_TreeRadio(pDX, IDC_EASTSHARE_OPTS, m_htiCreditSystem, (int &)m_iCreditSystem); //EastShare - added by linekin , CreditSystem
	
	// EastShare START - Added by TAHO, .met flies Control
//	DDX_TreeEdit(pDX, IDC_EASTSHARE_OPTS, m_htiClientsMet, m_iClientsMetDays);//EastShare - AndCycle, this official setting shoudlnt be change by user
	DDX_TreeEdit(pDX, IDC_EASTSHARE_OPTS, m_htiKnownMet, m_iKnownMetDays);
	// EastShare END - Added by TAHO, .met flies Control
	
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiEnablePreferShareAll, m_bEnablePreferShareAll);//EastShare - PreferShareAll by AndCycle
	DDX_TreeCheck(pDX, IDC_EASTSHARE_OPTS, m_htiIsPayBackFirst, m_bIsPayBackFirst);//EastShare - added by AndCycle, Pay Back First
}


BOOL CPPgEastShare::OnInitDialog()
{

	m_bEnablePreferShareAll = app_prefs->prefs->shareall;//EastShare - PreferShareAll by AndCycle
	m_bIsPayBackFirst = app_prefs->prefs->m_bPayBackFirst;//EastShare - added by AndCycle, Pay Back First
	m_iCreditSystem = app_prefs->GetCreditSystem(); //EastShare - Added by linekin , CreditSystem 
//	m_iClientsMetDays = app_prefs->GetClientsMetDays(); //EastShare - Added by TAHO , .met file control//EastShare - AndCycle, this official setting shoudlnt be change by user
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
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	
	if (!UpdateData())
		return FALSE;

	app_prefs->prefs->shareall = m_bEnablePreferShareAll;//EastShare - PreferShareAll by AndCycle
	app_prefs->prefs->m_bPayBackFirst = m_bIsPayBackFirst;//EastShare - added by AndCycle, Pay Back First

/*	theApp.emuledlg->serverwnd.ToggleDebugWindow();
	theApp.emuledlg->serverwnd.UpdateLogTabSelection(); */


	app_prefs->prefs->creditSystemMode = m_iCreditSystem; //EastShare - Added by linekin , CreditSystem 
//	app_prefs->SetClientsMetDays( m_iClientsMetDays); //EastShare - Added by TAHO , .met file control//EastShare - AndCycle, this official setting shoudlnt be change by user
	app_prefs->SetKnownMetDays( m_iKnownMetDays); //EastShare - Added by TAHO , .met file control

	SetModified(FALSE);

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

		//EastShare START - Added By TAHO, .met file control // Modified by Pretender
//		if (m_htiClientsMet) m_ctrlTreeOptions.SetEditLabel(m_htiClientsMet, (GetResString(IDS_EXPIRED_CLIENTS)));//EastShare - AndCycle, this official setting shoudlnt be change by user
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

	//EastShare START - Added by Pretender
	m_htiCreditSystem = NULL;
	m_htiOfficialCredit = NULL;
	m_htiLovelaceCredit = NULL;
	m_htiRatioCredit = NULL;
	m_htiPawcioCredit = NULL;
	m_htiBoostLess = NULL;
	m_htiESCredit = NULL;
	//EastShare END - Added by Pretender

	//EastShare START - Added by TAHO, .met control
	m_htiMetControl = NULL;
//	m_htiClientsMet = NULL;//EastShare - AndCycle, this official setting shoudlnt be change by user
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