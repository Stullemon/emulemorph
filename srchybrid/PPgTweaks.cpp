// PgTweaks.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "SearchDlg.h"
#include "PPgTweaks.h"
#include "Scheduler.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define	DFLT_MAXCONPERFIVE	20


///////////////////////////////////////////////////////////////////////////////
// CPPgTweaks dialog

IMPLEMENT_DYNAMIC(CPPgTweaks, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgTweaks, CPropertyPage)
	ON_WM_HSCROLL()	
	ON_WM_DESTROY()
	ON_MESSAGE(WM_TREEOPTSCTRL_NOTIFY, OnTreeOptsCtrlNotify)
END_MESSAGE_MAP()

CPPgTweaks::CPPgTweaks()
	: CPropertyPage(CPPgTweaks::IDD)
	, m_ctrlTreeOptions(theApp.m_iDfltImageListColorFlags)
{
	m_bInitializedTreeOpts = false;
	m_htiMaxCon5Sec = NULL;
	m_htiAutoTakeEd2kLinks = NULL;
	m_htiVerbose = NULL;
	m_htiDebugSourceExchange = NULL;
	m_htiDebugSecuredConnection = NULL; //MORPH - Added by SiRoB, Debug Log option for Secured connection
	m_htiCreditSystem = NULL;
	m_htiSaveLogs = NULL;
	m_htiLog2Disk = NULL;
	m_htiDebug2Disk = NULL;
	m_htiCommit = NULL;
	m_htiCommitNever = NULL;
	m_htiCommitOnShutdown = NULL;
	m_htiCommitAlways = NULL;
	m_htiFilterLANIPs = NULL;
	m_htiExtControls = NULL;
	m_htiServerKeepAliveTimeout = NULL;
	m_htiCheckDiskspace = NULL;	// SLUGFILLER: checkDiskspace
	m_htiMinFreeDiskSpace = NULL;
	m_htiYourHostname = NULL;	// itsonlyme: hostnameSource
}

CPPgTweaks::~CPPgTweaks()
{
}

void CPPgTweaks::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EXT_OPTS, m_ctrlTreeOptions);
	if (!m_bInitializedTreeOpts)
	{
		int iImgBackup = 8; // default icon
		int iImgLog = 8; // default icon
		CImageList* piml = m_ctrlTreeOptions.GetImageList(TVSIL_NORMAL);
		if (piml){
			iImgBackup = piml->Add(CTempIconLoader("SafeFileWrite"));
			iImgLog = piml->Add(CTempIconLoader("Log"));
		}

		m_htiMaxCon5Sec = m_ctrlTreeOptions.InsertItem(GetResString(IDS_MAXCON5SECLABEL), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, TVI_ROOT);
		m_ctrlTreeOptions.AddEditBox(m_htiMaxCon5Sec, RUNTIME_CLASS(CNumTreeOptionsEdit));
		m_htiAutoTakeEd2kLinks = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_AUTOTAKEED2KLINKS), TVI_ROOT, m_iAutoTakeEd2kLinks);
		m_htiVerbose = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_VERBOSE), TVI_ROOT, m_iVerbose);
		m_htiDebugSourceExchange = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_DEBUG_SOURCE_EXCHANGE), TVI_ROOT, m_iDebugSourceExchange);
		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebugSourceExchange, m_iVerbose);
		//MORPH START - Added by SiRoB, Debug Log option for Secured connection
		m_htiDebugSecuredConnection = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_SHOWSECUREDCONNECTION), TVI_ROOT, m_iDebugSecuredConnection);
		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebugSecuredConnection, m_iVerbose);
		//MORPH END - Added by SiRoB, Debug Log option for Secured connection
		m_htiCreditSystem = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_USECREDITSYSTEM), TVI_ROOT, m_iCreditSystem);
		m_ctrlTreeOptions.SetCheckBoxEnable(m_htiCreditSystem,false); //MORPH - Added by SiRoB, Credit System Allways Used
		
		m_htiFilterLANIPs = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_PW_FILTER), TVI_ROOT, m_iFilterLANIPs);
		m_htiExtControls = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_SHOWEXTSETTINGS), TVI_ROOT, m_iExtControls);
		m_htiServerKeepAliveTimeout = m_ctrlTreeOptions.InsertItem(GetResString(IDS_SERVERKEEPALIVETIMEOUT), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, TVI_ROOT);
		m_ctrlTreeOptions.AddEditBox(m_htiServerKeepAliveTimeout, RUNTIME_CLASS(CNumTreeOptionsEdit));

		m_htiSaveLogs = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_LOG2DISKFRAME), iImgLog, TVI_ROOT);
		m_htiLog2Disk = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_LOG2DISK), m_htiSaveLogs, m_iLog2Disk);
		m_htiDebug2Disk = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_DEBUG2DISK), m_htiSaveLogs, m_iDebug2Disk);

		m_htiCommit = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_COMMITFILES), iImgBackup, TVI_ROOT);
		m_htiCommitNever = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_NEVER), m_htiCommit, m_iCommitFiles == 0);
		m_htiCommitOnShutdown = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_ONSHUTDOWN), m_htiCommit, m_iCommitFiles == 1);
		m_htiCommitAlways = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_ALWAYS), m_htiCommit, m_iCommitFiles == 2);

		m_htiCheckDiskspace = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_CHECKDISKSPACE), TVI_ROOT, m_iCheckDiskspace);	// SLUGFILLER: checkDiskspace
		m_htiMinFreeDiskSpace = m_ctrlTreeOptions.InsertItem(GetResString(IDS_MINFREEDISKSPACE), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiCheckDiskspace);
		m_ctrlTreeOptions.AddEditBox(m_htiMinFreeDiskSpace, RUNTIME_CLASS(CNumTreeOptionsEdit));
		// itsonlyme: hostnameSource
		m_htiYourHostname = m_ctrlTreeOptions.InsertItem(GetResString(IDS_YOURHOSTNAME), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, TVI_ROOT);
		m_ctrlTreeOptions.AddEditBox(m_htiYourHostname, RUNTIME_CLASS(CTreeOptionsEdit));
		// itsonlyme: hostnameSource

		m_ctrlTreeOptions.Expand(m_htiSaveLogs, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiCommit, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiCheckDiskspace, TVE_EXPAND);
		m_ctrlTreeOptions.SendMessage(WM_VSCROLL, SB_TOP);
		m_bInitializedTreeOpts = true;
	}

	DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiMaxCon5Sec, m_iMaxConnPerFive);
	DDV_MinMaxInt(pDX, m_iMaxConnPerFive, 1, INT_MAX);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiAutoTakeEd2kLinks, m_iAutoTakeEd2kLinks);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiVerbose, m_iVerbose);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiDebugSourceExchange, m_iDebugSourceExchange);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiDebugSecuredConnection, m_iDebugSecuredConnection); //MORPH - Added by SiRoB, Debug Log option for Secured connection
	//MORPH START - Changed by SiRoB, Hot fix to show correct disabled checkbox
	//MORPH - Removed by SiRoB, Hot fix to show correct disabled checkbox
	//DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiCreditSystem, m_iCreditSystem);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiLog2Disk, m_iLog2Disk);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiDebug2Disk, m_iDebug2Disk);
	DDX_TreeRadio(pDX, IDC_EXT_OPTS, m_htiCommit, m_iCommitFiles);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiFilterLANIPs, m_iFilterLANIPs);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiExtControls, m_iExtControls);
	DDX_Text(pDX, IDC_EXT_OPTS, m_htiServerKeepAliveTimeout, m_uServerKeepAliveTimeout);
	DDX_TreeCheck(pDX, IDC_EXT_OPTS, m_htiCheckDiskspace, m_iCheckDiskspace);	// SLUGFILLER: checkDiskspace
	DDX_Text(pDX, IDC_EXT_OPTS, m_htiMinFreeDiskSpace, m_fMinFreeDiskSpaceMB);
	DDX_TreeEdit(pDX, IDC_EXT_OPTS, m_htiYourHostname, m_sYourHostname);	// itsonlyme: hostnameSource

	m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebugSourceExchange, m_iVerbose);
}
BOOL CPPgTweaks::OnInitDialog()
{
	m_iMaxConnPerFive = app_prefs->GetMaxConperFive();
	m_iAutoTakeEd2kLinks = app_prefs->prefs->autotakeed2klinks;
	m_iVerbose = app_prefs->prefs->m_bVerbose;
	m_iDebugSourceExchange = app_prefs->prefs->m_bDebugSourceExchange;
	m_iDebugSecuredConnection = app_prefs->prefs->m_bDebugSecuredConnection;  //MORPH - Added by SiRoB, Debug Log option for Secured Connection
	m_iLog2Disk = app_prefs->prefs->log2disk;
	m_iDebug2Disk = app_prefs->prefs->debug2disk;
	m_iCreditSystem = app_prefs->prefs->m_bCreditSystem;
	m_iCommitFiles = app_prefs->prefs->m_iCommitFiles;
	m_iFilterLANIPs = app_prefs->prefs->filterBadIP;
	m_iExtControls = app_prefs->prefs->m_bExtControls;
	m_uServerKeepAliveTimeout = app_prefs->prefs->m_dwServerKeepAliveTimeout / 60000;
	m_iCheckDiskspace = app_prefs->prefs->checkDiskspace;	// SLUGFILLER: checkDiskspace
	m_fMinFreeDiskSpaceMB = (float)(app_prefs->prefs->m_uMinFreeDiskSpace / (1024.0 * 1024.0));
	m_sYourHostname = app_prefs->GetYourHostname();	// itsonlyme: hostnameSource

	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);

	((CSliderCtrl*)GetDlgItem(IDC_FILEBUFFERSIZE))->SetRange(1,100,true);
	((CSliderCtrl*)GetDlgItem(IDC_FILEBUFFERSIZE))->SetPos(app_prefs->prefs->m_iFileBufferSize);
	m_iFileBufferSize = app_prefs->prefs->m_iFileBufferSize;

	((CSliderCtrl*)GetDlgItem(IDC_QUEUESIZE))->SetRange(20,100,true);
	((CSliderCtrl*)GetDlgItem(IDC_QUEUESIZE))->SetPos(app_prefs->prefs->m_iQueueSize);
	m_iQueueSize = app_prefs->prefs->m_iQueueSize;

	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgTweaks::OnKillActive()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	return CPropertyPage::OnKillActive();
}

BOOL CPPgTweaks::OnApply()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();

	if (!UpdateData())
		return FALSE;

	app_prefs->SetMaxConsPerFive(m_iMaxConnPerFive ? m_iMaxConnPerFive : DFLT_MAXCONPERFIVE);
	theApp.scheduler->original_cons5s = app_prefs->GetMaxConperFive();

	if (app_prefs->prefs->autotakeed2klinks != m_iAutoTakeEd2kLinks){
		app_prefs->prefs->autotakeed2klinks = m_iAutoTakeEd2kLinks;
		RevertReg();
	}

	app_prefs->prefs->m_bVerbose = m_iVerbose;
	app_prefs->prefs->m_bDebugSourceExchange = m_iDebugSourceExchange;
	app_prefs->prefs->m_bDebugSecuredConnection = m_iDebugSecuredConnection; //MORPH - Added by SiRoB, Debug Log option for Secured Connection
	app_prefs->prefs->m_bCreditSystem = m_iCreditSystem;
	app_prefs->prefs->log2disk = m_iLog2Disk;
	app_prefs->prefs->debug2disk = m_iDebug2Disk;
	app_prefs->prefs->m_iCommitFiles = m_iCommitFiles;
	app_prefs->prefs->filterBadIP = m_iFilterLANIPs;
	app_prefs->prefs->m_iFileBufferSize = m_iFileBufferSize;
	app_prefs->prefs->m_iQueueSize = m_iQueueSize;
	if (app_prefs->prefs->m_bExtControls != (bool)m_iExtControls) {
		app_prefs->prefs->m_bExtControls = m_iExtControls;
		theApp.emuledlg->transferwnd.downloadlistctrl.CreateMenues();
		theApp.emuledlg->searchwnd->searchlistctrl.CreateMenues();
		theApp.emuledlg->sharedfileswnd.sharedfilesctrl.CreateMenues();
	}
	app_prefs->prefs->m_dwServerKeepAliveTimeout = m_uServerKeepAliveTimeout * 60000;
	app_prefs->prefs->checkDiskspace = m_iCheckDiskspace;	// SLUGFILLER: checkDiskspace
	app_prefs->prefs->m_uMinFreeDiskSpace = (UINT)(m_fMinFreeDiskSpaceMB * (1024 * 1024));
	app_prefs->SetYourHostname(m_sYourHostname);	// itsonlyme: hostnameSource

	theApp.emuledlg->serverwnd.ToggleDebugWindow();
	theApp.emuledlg->serverwnd.UpdateLogTabSelection();
	theApp.downloadqueue->CheckDiskspace();

	SetModified(FALSE);
	return CPropertyPage::OnApply();
}

void CPPgTweaks::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	SetModified(TRUE);
	
	CSliderCtrl* slider =(CSliderCtrl*)pScrollBar;
	CString temp;
	if (pScrollBar==GetDlgItem(IDC_FILEBUFFERSIZE)) {
		m_iFileBufferSize = slider->GetPos();
		temp.Format( GetResString(IDS_FILEBUFFERSIZE), m_iFileBufferSize*15000 );
		GetDlgItem(IDC_FILEBUFFERSIZE_STATIC)->SetWindowText(temp);
	}
	else if (pScrollBar==GetDlgItem(IDC_QUEUESIZE)) {
		m_iQueueSize = slider->GetPos();
		temp.Format( GetResString(IDS_QUEUESIZE), m_iQueueSize*100 );
		GetDlgItem(IDC_QUEUESIZE_STATIC)->SetWindowText(temp);
	}
}

void CPPgTweaks::Localize(void)
{	
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_TWEAK));
		GetDlgItem(IDC_WARNING)->SetWindowText(GetResString(IDS_TWEAKS_WARNING));

		if (m_htiMaxCon5Sec) m_ctrlTreeOptions.SetEditLabel(m_htiMaxCon5Sec, GetResString(IDS_MAXCON5SECLABEL));
		if (m_htiAutoTakeEd2kLinks) m_ctrlTreeOptions.SetItemText(m_htiAutoTakeEd2kLinks, GetResString(IDS_AUTOTAKEED2KLINKS));
		if (m_htiVerbose) m_ctrlTreeOptions.SetItemText(m_htiVerbose, GetResString(IDS_VERBOSE));
		if (m_htiDebugSourceExchange) m_ctrlTreeOptions.SetItemText(m_htiDebugSourceExchange, GetResString(IDS_DEBUG_SOURCE_EXCHANGE));
		if (m_htiDebugSecuredConnection) m_ctrlTreeOptions.SetItemText(m_htiDebugSecuredConnection, GetResString(IDS_SHOWSECUREDCONNECTION)); //MORPH - Added by SiRoB, Debug Log option for Secured connection
			
		if (m_htiCreditSystem) m_ctrlTreeOptions.SetItemText(m_htiCreditSystem, GetResString(IDS_USECREDITSYSTEM));

		if (m_htiSaveLogs) m_ctrlTreeOptions.SetItemText(m_htiSaveLogs, GetResString(IDS_LOG2DISKFRAME));
		if (m_htiLog2Disk) m_ctrlTreeOptions.SetItemText(m_htiLog2Disk, GetResString(IDS_LOG2DISK));
		if (m_htiDebug2Disk) m_ctrlTreeOptions.SetItemText(m_htiDebug2Disk, GetResString(IDS_DEBUG2DISK));

		if (m_htiCommit) m_ctrlTreeOptions.SetItemText(m_htiCommit, GetResString(IDS_COMMITFILES));
		if (m_htiCommitNever) m_ctrlTreeOptions.SetItemText(m_htiCommitNever, GetResString(IDS_NEVER));
		if (m_htiCommitOnShutdown) m_ctrlTreeOptions.SetItemText(m_htiCommitOnShutdown, GetResString(IDS_ONSHUTDOWN));
		if (m_htiCommitAlways) m_ctrlTreeOptions.SetItemText(m_htiCommitAlways, GetResString(IDS_ALWAYS));
		if (m_htiFilterLANIPs) m_ctrlTreeOptions.SetItemText(m_htiFilterLANIPs, GetResString(IDS_PW_FILTER));
		if (m_htiExtControls) m_ctrlTreeOptions.SetItemText(m_htiExtControls, GetResString(IDS_SHOWEXTSETTINGS));
		if (m_htiServerKeepAliveTimeout) m_ctrlTreeOptions.SetEditLabel(m_htiServerKeepAliveTimeout, GetResString(IDS_SERVERKEEPALIVETIMEOUT));
		if (m_htiCheckDiskspace) m_ctrlTreeOptions.SetItemText(m_htiCheckDiskspace, GetResString(IDS_CHECKDISKSPACE));	// SLUGFILLER: checkDiskspace
		if (m_htiMinFreeDiskSpace) m_ctrlTreeOptions.SetEditLabel(m_htiMinFreeDiskSpace, GetResString(IDS_MINFREEDISKSPACE));
		if (m_htiYourHostname) m_ctrlTreeOptions.SetEditLabel(m_htiYourHostname, GetResString(IDS_YOURHOSTNAME));	// itsonlyme: hostnameSource

		CString temp;
		temp.Format( GetResString(IDS_FILEBUFFERSIZE), m_iFileBufferSize*15000 );
		GetDlgItem(IDC_FILEBUFFERSIZE_STATIC)->SetWindowText(temp);
		temp.Format( GetResString(IDS_QUEUESIZE), m_iQueueSize*100 );
		GetDlgItem(IDC_QUEUESIZE_STATIC)->SetWindowText(temp);
	}
}

void CPPgTweaks::OnDestroy()
{
	m_ctrlTreeOptions.DeleteAllItems();
	m_ctrlTreeOptions.DestroyWindow();
	m_bInitializedTreeOpts = false;
	m_htiMaxCon5Sec = NULL;
	m_htiAutoTakeEd2kLinks = NULL;
	m_htiVerbose = NULL;
	m_htiDebugSourceExchange = NULL;
	m_htiDebugSecuredConnection = NULL; //MORPH - Added by SiRoB, Debug Log option for Secured Connection
	m_htiCreditSystem = NULL;
	m_htiSaveLogs = NULL;
	m_htiLog2Disk = NULL;
	m_htiDebug2Disk = NULL;
	m_htiCommit = NULL;
	m_htiCommitNever = NULL;
	m_htiCommitOnShutdown = NULL;
	m_htiCommitAlways = NULL;
	m_htiFilterLANIPs = NULL;
	m_htiExtControls = NULL;
	m_htiServerKeepAliveTimeout = NULL;
	m_htiCheckDiskspace = NULL;	// SLUGFILLER: checkDiskspace
	m_htiMinFreeDiskSpace = NULL;
	m_htiYourHostname = NULL;	// itsonlyme: hostnameSource
	CPropertyPage::OnDestroy();
}

LRESULT CPPgTweaks::OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDC_EXT_OPTS){
		TREEOPTSCTRLNOTIFY* pton = (TREEOPTSCTRLNOTIFY*)lParam;
		if (pton->hItem == m_htiVerbose){
			BOOL bCheck;
			if (m_ctrlTreeOptions.GetCheckBox(m_htiVerbose, bCheck)){
				m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebugSourceExchange, bCheck);
				m_ctrlTreeOptions.SetCheckBoxEnable(m_htiDebugSecuredConnection, bCheck); //MORPH - Added by SiRoB, Debug Log option for Secured Connection
			}
		}
		SetModified();
	}
	return 0;
}
