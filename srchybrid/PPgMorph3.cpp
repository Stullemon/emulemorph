// PpgMorph3.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgMorph3.h"
#include "OtherFunctions.h"
#include "Preferences.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// added by MoNKi [MoNKi: -Wap Server-]
#include "TreeOptsPrefs\PassTreeOptionsEdit.h"
#include "emuledlg.h"
#include "serverwnd.h"
#define HIDDEN_PASSWORD _T("*****")
// End MoNKi


///////////////////////////////////////////////////////////////////////////////
// CPPgMorph3 dialog

BEGIN_MESSAGE_MAP(CPPgMorph3, CPropertyPage)
	ON_MESSAGE(WM_TREEOPTSCTRL_NOTIFY, OnTreeOptsCtrlNotify)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

IMPLEMENT_DYNAMIC(CPPgMorph3, CPropertyPage)
CPPgMorph3::CPPgMorph3()
: CPropertyPage(CPPgMorph3::IDD)
, m_ctrlTreeOptions(theApp.m_iDfltImageListColorFlags)	
{
	m_bInitializedTreeOpts = false;


	// Added by MoNKi [ MoNKi: -Wap Server- ]
	m_htiWapRoot = NULL;
	m_htiWapEnable = NULL;
	m_htiWapPort = NULL;
	m_htiWapTemplate = NULL;
	m_htiWapPass = NULL;
	m_htiWapLowEnable = NULL;
	m_htiWapLowPass = NULL;
	m_bWapEnable = false;
	m_iWapPort = 0;
	m_sWapTemplate = "";
	m_sWapPass = "";
	m_bWapLowEnable = false;
	m_sWapLowPass = "";
	// End MoNKi

}

CPPgMorph3::~CPPgMorph3()
{
}

void CPPgMorph3::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MORPH3_OPTS, m_ctrlTreeOptions);
	if ( !m_bInitializedTreeOpts )
	{
		CImageList* piml = m_ctrlTreeOptions.GetImageList(TVSIL_NORMAL);


		// Added by MoNKi [MoNKi: -Wap server-]
		int iImgWap = 8; // default icon
		if (piml){
			iImgWap = piml->Add(CTempIconLoader(_T("MOBILE")));
		}
		m_htiWapRoot = m_ctrlTreeOptions.InsertItem(_T("Wap Interface"), iImgWap, iImgWap, TVI_ROOT);
		m_htiWapEnable  = m_ctrlTreeOptions.InsertCheckBox(_T("Enable Wap Interface"), m_htiWapRoot, m_bWapEnable);
		m_htiWapTemplate = m_ctrlTreeOptions.InsertItem(_T("Template"), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiWapRoot);
		m_ctrlTreeOptions.AddFileEditBox(m_htiWapTemplate,RUNTIME_CLASS(CTreeOptionsEdit), RUNTIME_CLASS(CTreeOptionsBrowseButton));
		m_htiWapPort = m_ctrlTreeOptions.InsertItem(_T("Port"), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiWapRoot);
		m_ctrlTreeOptions.AddEditBox(m_htiWapPort, RUNTIME_CLASS(CNumTreeOptionsEdit));
		m_htiWapPass = m_ctrlTreeOptions.InsertItem(_T("Pass"), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiWapRoot);
		m_ctrlTreeOptions.AddPassEditBox(m_htiWapPass, RUNTIME_CLASS(CPassTreeOptionsEdit));
		m_htiWapLowEnable  = m_ctrlTreeOptions.InsertCheckBox(_T("Low Enable"), m_htiWapRoot, m_bWapLowEnable);
		m_htiWapLowPass = m_ctrlTreeOptions.InsertItem(_T("Low Pass"), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiWapRoot);
		m_ctrlTreeOptions.AddPassEditBox(m_htiWapLowPass, RUNTIME_CLASS(CPassTreeOptionsEdit));

		m_ctrlTreeOptions.Expand(m_htiWapRoot, TVE_EXPAND);
		// End MoNKi

		m_ctrlTreeOptions.SelectItem(m_ctrlTreeOptions.GetRootItem());
		m_bInitializedTreeOpts = true;
	}

	// Added by MoNKi [ MoNKi: -Wap Server- ]
	DDX_TreeCheck(pDX, IDC_MORPH3_OPTS, m_htiWapEnable, m_bWapEnable);
	DDX_TreeEdit(pDX, IDC_MORPH3_OPTS, m_htiWapPort, m_iWapPort);
	DDV_MinMaxInt(pDX, m_iWapPort, 0, 0xFFFF);
	DDX_TreeEdit(pDX, IDC_MORPH3_OPTS, m_htiWapTemplate, m_sWapTemplate);
	DDX_TreeEdit(pDX, IDC_MORPH3_OPTS, m_htiWapPass, m_sWapPass);
	DDX_TreeCheck(pDX, IDC_MORPH3_OPTS, m_htiWapLowEnable, m_bWapLowEnable);
	DDX_TreeEdit(pDX, IDC_MORPH3_OPTS, m_htiWapLowPass, m_sWapLowPass);
	// end MoNKi
}

// CPPgMorph3 message handlers

BOOL CPPgMorph3::OnInitDialog()
{
        // Added by by MoNKi [MoNKi: -Wap Server-]
	m_bWapEnable = thePrefs.GetWapServerEnabled();
	m_iWapPort = thePrefs.GetWapPort();
	m_sWapTemplate = thePrefs.GetWapTemplate();
	m_sWapPass = HIDDEN_PASSWORD;
	m_bWapLowEnable = thePrefs.GetWapIsLowUserEnabled();
	m_sWapLowPass = HIDDEN_PASSWORD;
	// End MoNKi

	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgMorph3::OnKillActive()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	return CPropertyPage::OnKillActive();
}

BOOL CPPgMorph3::OnApply()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();

	if (!UpdateData())
		return FALSE;

	bool bRestartApp = false;


	// Added by by MoNKi [MoNKi: -Wap Server-]
	thePrefs.SetWapServerEnabled(m_bWapEnable);
	thePrefs.SetWapIsLowUserEnabled(m_bWapLowEnable);
	if(m_sWapPass != CString(HIDDEN_PASSWORD))
		thePrefs.SetWapPass(m_sWapPass);
	if(m_sWapLowPass != CString(HIDDEN_PASSWORD))
		thePrefs.SetWapLowPass(m_sWapLowPass);
	if(m_sWapTemplate != thePrefs.GetWapTemplate()){
		thePrefs.SetWapTemplate(m_sWapTemplate);
		theApp.wapserver->ReloadTemplates();
	}
	if(m_iWapPort != thePrefs.GetWapPort()){
		thePrefs.SetWapPort(m_iWapPort);
		theApp.wapserver->RestartServer();
	}
	theApp.wapserver->StartServer();
	theApp.emuledlg->serverwnd->UpdateMyInfo();
	// End MoNKi
	

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}
void CPPgMorph3::Localize(void)
{
	if ( m_hWnd )
	{
		SetWindowText(_T("Morph III"));

		// Added by MoNKi [ MoNKi: -Wap Server- ]
		if (m_htiWapRoot)		m_ctrlTreeOptions.SetItemText(m_htiWapRoot, GetResString(IDS_PW_WAP));
		if (m_htiWapEnable)		m_ctrlTreeOptions.SetItemText(m_htiWapEnable, GetResString(IDS_ENABLED));
		if (m_htiWapPort)		m_ctrlTreeOptions.SetEditLabel(m_htiWapPort, GetResString(IDS_PORT));
		if (m_htiWapTemplate)	m_ctrlTreeOptions.SetEditLabel(m_htiWapTemplate, GetResString(IDS_WS_RELOAD_TMPL));
		if (m_htiWapPass)		m_ctrlTreeOptions.SetEditLabel(m_htiWapPass, GetResString(IDS_WS_PASS));
		if (m_htiWapLowEnable)	m_ctrlTreeOptions.SetItemText(m_htiWapLowEnable, GetResString(IDS_WEB_LOWUSER));
		if (m_htiWapLowPass)	m_ctrlTreeOptions.SetEditLabel(m_htiWapLowPass, GetResString(IDS_WS_PASS));
		// End MoNKi
	}
}

LRESULT CPPgMorph3::OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDC_MORPH3_OPTS)
	{
		TREEOPTSCTRLNOTIFY* pton = (TREEOPTSCTRLNOTIFY*)lParam;

		SetModified();
	}
	return 0;
}

void CPPgMorph3::OnDestroy()
{
	m_ctrlTreeOptions.DeleteAllItems();
	m_ctrlTreeOptions.DestroyWindow();
	m_bInitializedTreeOpts = false;

	// Added by MoNKi [ MoNKi: -Wap Server- ]
	m_htiWapRoot = NULL;
	m_htiWapEnable = NULL;
	m_htiWapPort = NULL;
	m_htiWapTemplate = NULL;
	m_htiWapPass = NULL;
	m_htiWapLowEnable = NULL;
	m_htiWapLowPass = NULL;
	// End MoNKi

	CPropertyPage::OnDestroy();
}
