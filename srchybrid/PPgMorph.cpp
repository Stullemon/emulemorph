// PpgMorph.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PPgMorph.h"
#include "emuledlg.h"
#include "serverWnd.h" //MORPH - Added by SiRoB
#include "OtherFunctions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


///////////////////////////////////////////////////////////////////////////////
// CPPgMorph dialog

IMPLEMENT_DYNAMIC(CPPgMorph, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgMorph, CPropertyPage)
	ON_WM_HSCROLL()
    ON_WM_DESTROY()
	ON_MESSAGE(WM_TREEOPTSCTRL_NOTIFY, OnTreeOptsCtrlNotify)
END_MESSAGE_MAP()

CPPgMorph::CPPgMorph()
	: CPropertyPage(CPPgMorph::IDD)
	, m_ctrlTreeOptions(theApp.m_iDfltImageListColorFlags)
{
	m_bInitializedTreeOpts = false;
	m_htiDM = NULL;
	m_htiUM = NULL;
	m_htiDYNUP = NULL;
	m_htiDynUpOFF = NULL;
	m_htiDynUpSUC = NULL;
	m_htiDynUpUSS = NULL;
	m_htiDynUpAutoSwitching = NULL;//MORPH - Added by Yun.SF3, Auto DynUp changing
	m_htiMaxConnectionsSwitchBorder = NULL;//MORPH - Added by Yun.SF3, Auto DynUp changing
	m_htiSUCLog = NULL;
	m_htiSUCHigh = NULL;
	m_htiSUCLow = NULL;
	m_htiSUCPitch = NULL;
	m_htiSUCDrift = NULL;
	m_htiUSSLog = NULL;
	m_htiUSSLimit = NULL; // EastShare - Added by TAHO , USS limit
	m_htiUSSPingLimit = NULL; // EastShare - Added by TAHO, USS limit
    m_htiUSSPingTolerance = NULL;
    m_htiUSSGoingUpDivider = NULL;
    m_htiUSSGoingDownDivider = NULL;
    m_htiUSSNumberOfPings = NULL;
	m_htiMinUpload = NULL;
	m_htiUpSecu = NULL;
	m_htiDlSecu = NULL;
	m_htiEnableZeroFilledTest = NULL;
	m_htiDisp = NULL;
	m_htiEnableDownloadInRed = NULL; //MORPH - Added by IceCream, show download in red
	m_htiEnableDownloadInBold = NULL; //MORPH - Added by SiRoB, show download in Bold
	m_htiEnableAntiLeecher = NULL; //MORPH - Added by IceCream, activate Anti-leecher
	m_htiEnableAntiCreditHack = NULL; //MORPH - Added by IceCream, activate Anti-CreditHack
	m_htiHideOS = NULL;	//MORPH - Added by SiRoB, SLUGFILLER: hideOS
	m_htiSelectiveShare = NULL;	//MORPH - Added by SiRoB, SLUGFILLER: hideOS
	m_htiShareOnlyTheNeed = NULL; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
	m_htiIsAutoPowershareNewDownloadFile = NULL; //MORPH - Added by SiRoB, Avoid misusing of powersharing	
	//MORPH START - Added by SiRoB, Show Permission
	m_htiPermissions = NULL;
	m_htiPermAll = NULL;
	m_htiPermFriend = NULL;
	m_htiPermNone = NULL;
	//MORPH END   - Added by SiRoB, Show Permission
	m_htiSCC = NULL;
	//MORPH START - Added by SiRoB, khaos::categorymod+
	m_htiShowCatNames = NULL;
	m_htiSelectCat = NULL;
	m_htiUseActiveCat = NULL;
	m_htiAutoSetResOrder = NULL;
	m_htiShowA4AFDebugOutput = NULL;
	m_htiSmartA4AFSwapping = NULL;
	m_htiAdvA4AFMode = NULL;
	m_htiBalanceSources = NULL;
	m_htiStackSources = NULL;
	m_htiDisableAdvA4AF = NULL;
	m_htiSmallFileDLPush = NULL;
	m_htiResumeFileInNewCat = NULL;
	m_htiUseAutoCat = NULL;
	m_htiUseSLS = NULL;
	// khaos::accuratetimerem+
	m_htiTimeRemainingMode = NULL;
	m_htiTimeRemBoth = NULL;
	m_htiTimeRemAverage = NULL;
	m_htiTimeRemRealTime = NULL;
	// khaos::accuratetimerem-
	//MORPH END - Added by SiRoB, khaos::categorymod+
	m_htiHighProcess = NULL; //MORPH - Added by IceCream, high process priority
	m_htiIsBoostFriends = NULL;//Added by Yun.SF3, boost friends
}

CPPgMorph::~CPPgMorph()
{
}

void CPPgMorph::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MORPH_OPTS, m_ctrlTreeOptions);
	if (!m_bInitializedTreeOpts)
	{
		int iImgUM = 8; // default icon
		int iImgDYNUP = 8; // default icon
		int iImgSUC = 8; // default icon
		int iImgUSS = 8;
		int iImgDM = 8;
		int iImgSFM = 8;
		int iImgPerm = 8;
		//MORPH START - Added by SiRoB, khaos::categorymod+
		int iImgSCC = 8;
		int iImgSAC = 8;
		int iImgA4AF = 8;
		int iImgTimeRem = 8;
		//MORPH END - Added by SiRoB, khaos::categorymod+
		int iImgSecu = 8;
		int iImgDisp = 8;
		CImageList* piml = m_ctrlTreeOptions.GetImageList(TVSIL_NORMAL);
		if (piml){
			iImgUM = piml->Add(CTempIconLoader("UPLOAD"));
			iImgDYNUP = piml->Add(CTempIconLoader("SUC"));
			iImgDM = piml->Add(CTempIconLoader("SEARCHDIRECTDOWNLOAD"));
			iImgSFM = piml->Add(CTempIconLoader("SHAREDFILES"));
			//MORPH START - Added by SiRoB, khaos::categorymod+
			iImgSCC = piml->Add(CTempIconLoader("PREF_FOLDERS"));
			iImgSAC = piml->Add(CTempIconLoader("ClientCompatible"));
			iImgA4AF = piml->Add(CTempIconLoader("SERVERLIST"));
			//MORPH START - Added by SiRoB, Show Permissions
			iImgPerm = piml->Add(CTempIconLoader("ClientCompatible"));
			//MORPH END   - Added by SiRoB, Show Permissions
			// khaos::accuratetimerem+
			iImgTimeRem = piml->Add(CTempIconLoader("PREF_SCHEDULER"));
			// khaos::accuratetimerem-
			//MORPH END - Added by SiRoB, khaos::categorymod+
			iImgSecu = piml->Add(CTempIconLoader("PREF_SECURITY"));
			iImgDisp = piml->Add(CTempIconLoader("PREF_DISPLAY"));
		}
		
		m_htiDM = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_DM), iImgDM, TVI_ROOT);
		//MORPH START - Added by SiRoB, khaos::categorymod+
		m_htiSCC = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_SCC), iImgSCC, m_htiDM);
		m_htiShowCatNames = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_CAT_SHOWCATNAME), m_htiSCC, m_iShowCatNames);
		m_htiSelectCat = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_CAT_SHOWSELCATDLG), m_htiSCC, m_iSelectCat);
		m_htiUseAutoCat = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_CAT_USEAUTOCAT), m_htiSCC, m_iUseAutoCat);
		m_htiUseActiveCat = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_CAT_USEACTIVE), m_htiSCC, m_iUseActiveCat);
		m_htiAutoSetResOrder = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_CAT_AUTORESUMEORD), m_htiSCC, m_iAutoSetResOrder);
		
		m_htiSAC = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_SAC), iImgSAC, m_htiDM);
		m_htiShowA4AFDebugOutput  = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_A4AF_SHOWDEBUG), m_htiSAC, m_iShowA4AFDebugOutput);
		m_htiSmallFileDLPush = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_CAT_SMALLFILEDLPUSH), m_htiSAC, m_iSmallFileDLPush);
		m_htiResumeFileInNewCat = m_ctrlTreeOptions.InsertItem(GetResString(IDS_CAT_STARTFILESONADD), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiSAC);
		m_ctrlTreeOptions.AddEditBox(m_htiResumeFileInNewCat, RUNTIME_CLASS(CNumTreeOptionsEdit));
		m_htiSmartA4AFSwapping = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_A4AF_SMARTSWAP), m_htiSAC, m_iSmartA4AFSwapping);
		m_htiAdvA4AFMode = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_DEFAULT) + " " + GetResString(IDS_A4AF_ADVMODE), iImgA4AF, m_htiSAC);
		m_htiDisableAdvA4AF = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_A4AF_DISABLED), m_htiAdvA4AFMode, m_iAdvA4AFMode == 0);
		m_htiBalanceSources = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_A4AF_BALANCE), m_htiAdvA4AFMode, m_iAdvA4AFMode == 1);
		m_htiStackSources = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_A4AF_STACK), m_htiAdvA4AFMode, m_iAdvA4AFMode == 2);
		
		//m_ctrlTreeOptions.Expand(m_htiSCC, TVE_EXPAND);
		//m_ctrlTreeOptions.Expand(m_htiSAC, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiAdvA4AFMode, TVE_EXPAND);
		//MORPH END - Added by SiRoB, khaos::categorymod+
		// khaos::accuratetimerem+
		m_htiTimeRemainingMode = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_REMTIMEAVRREAL), iImgTimeRem, m_htiDM);
		m_htiTimeRemBoth = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_BOTH), m_htiTimeRemainingMode, m_iTimeRemainingMode == 0);
		m_htiTimeRemRealTime = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_REALTIME), m_htiTimeRemainingMode, m_iTimeRemainingMode == 1);
		m_htiTimeRemAverage = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_AVG), m_htiTimeRemainingMode, m_iTimeRemainingMode == 2);
		//m_ctrlTreeOptions.Expand(m_htiTimeRemainingMode, TVE_EXPAND); // khaos::accuratetimerem+
		// khaos::accuratetimerem-
		m_htiDlSecu = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_SECURITY), iImgSecu, m_htiDM);
		m_htiEnableZeroFilledTest = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_ZERO_FILLED_TEST), m_htiDlSecu, m_bEnableZeroFilledTest);
		m_htiDisp = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_PW_DISPLAY), iImgDisp, m_htiDM);
		m_htiEnableDownloadInRed = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_DOWNLOAD_IN_RED), m_htiDisp, m_bEnableDownloadInRed); //MORPH - Added by SiRoB, show download in Bold
		m_htiEnableDownloadInBold = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_DOWNLOAD_IN_BOLD), m_htiDisp, m_bEnableDownloadInBold); //MORPH - Added by SiRoB, show download in Bold
				
		//MORPH START - Added by SiRoB, khaos::categorymod+
		m_htiUseSLS = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_SLS_USESLS), m_htiDM, m_iUseSLS);
		//MORPH END - Added by SiRoB, khaos::categorymod+
		
		CString Buffer;
		m_htiUM = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_UM), iImgUM, TVI_ROOT);
		m_htiDYNUP = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_DYNUPLOAD), iImgDYNUP, m_htiUM);
		
		m_htiDynUpOFF = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_DISABLED), m_htiDYNUP, m_iDynUpMode == 0);
		m_htiDynUpSUC = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_SUC), m_htiDYNUP, m_iDynUpMode == 1);
		m_htiSUCLog = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_SUC_LOG), m_htiDynUpSUC, m_iSUCLog);
		Buffer.Format(GetResString(IDS_SUC_HIGH),900);
		m_htiSUCHigh = m_ctrlTreeOptions.InsertItem(Buffer, TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUpSUC);
		m_ctrlTreeOptions.AddEditBox(m_htiSUCHigh, RUNTIME_CLASS(CNumTreeOptionsEdit));
		Buffer.Format(GetResString(IDS_SUC_LOW),600);
		m_htiSUCLow = m_ctrlTreeOptions.InsertItem(Buffer, TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUpSUC);
		m_ctrlTreeOptions.AddEditBox(m_htiSUCLow, RUNTIME_CLASS(CNumTreeOptionsEdit));
		Buffer.Format(GetResString(IDS_SUC_PITCH),3000);
		m_htiSUCPitch = m_ctrlTreeOptions.InsertItem(Buffer, TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUpSUC);
		m_ctrlTreeOptions.AddEditBox(m_htiSUCPitch, RUNTIME_CLASS(CNumTreeOptionsEdit));
		Buffer.Format(GetResString(IDS_SUC_DRIFT),50);
		m_htiSUCDrift = m_ctrlTreeOptions.InsertItem(Buffer, TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUpSUC);
		m_ctrlTreeOptions.AddEditBox(m_htiSUCDrift, RUNTIME_CLASS(CNumTreeOptionsEdit));
		
		m_htiDynUpUSS = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_USS), m_htiDYNUP, m_iDynUpMode == 2);
		m_htiUSSLog = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_USS_LOG), m_htiDynUpUSS, m_iUSSLog);

		// EastShare START - Added by TAHO, USS limit
		m_htiUSSLimit = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_USS_USEMAXPING), m_htiDynUpUSS, m_iUSSLimit);
		//Buffer.Format("Max ping value (ms): ",800); //modified by Pretender
		Buffer.Format(GetResString(IDS_USS_MAXPING),500); //Added by Pretender
		m_htiUSSPingLimit = m_ctrlTreeOptions.InsertItem(Buffer, TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUpUSS);
		m_ctrlTreeOptions.AddEditBox(m_htiUSSPingLimit, RUNTIME_CLASS(CNumTreeOptionsEdit));
		// EastShare END - Added by TAHO, USS limit

		Buffer.Format(GetResString(IDS_USS_PINGTOLERANCE),800);
		m_htiUSSPingTolerance = m_ctrlTreeOptions.InsertItem(Buffer, TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUpUSS);
		m_ctrlTreeOptions.AddEditBox(m_htiUSSPingTolerance, RUNTIME_CLASS(CNumTreeOptionsEdit));
		Buffer.Format(GetResString(IDS_USS_GOINGUPDIVIDER),1000);
		m_htiUSSGoingUpDivider = m_ctrlTreeOptions.InsertItem(Buffer, TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUpUSS);
		m_ctrlTreeOptions.AddEditBox(m_htiUSSGoingUpDivider, RUNTIME_CLASS(CNumTreeOptionsEdit));
		Buffer.Format(GetResString(IDS_USS_GOINGDOWNDIVIDER),1000);
		m_htiUSSGoingDownDivider = m_ctrlTreeOptions.InsertItem(Buffer, TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUpUSS);
		m_ctrlTreeOptions.AddEditBox(m_htiUSSGoingDownDivider, RUNTIME_CLASS(CNumTreeOptionsEdit));
		Buffer.Format(GetResString(IDS_USS_NUMBEROFPINGS),1);
		m_htiUSSNumberOfPings = m_ctrlTreeOptions.InsertItem(Buffer, TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUpUSS);
		m_ctrlTreeOptions.AddEditBox(m_htiUSSNumberOfPings, RUNTIME_CLASS(CNumTreeOptionsEdit));
		
		//MORPH START - Added by Yun.SF3, Auto DynUp changing
		m_htiDynUpAutoSwitching = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_AUTODYNUPSWITCHING), m_htiDYNUP, m_iDynUpMode == 3);
		Buffer.Format(GetResString(IDS_MAXCONNECTIONSSWITCHBORDER), 20);
		m_htiMaxConnectionsSwitchBorder = m_ctrlTreeOptions.InsertItem(Buffer, TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDynUpAutoSwitching);
		m_ctrlTreeOptions.AddEditBox(m_htiMaxConnectionsSwitchBorder, RUNTIME_CLASS(CNumTreeOptionsEdit));
		//MORPH END - Added by Yun.SF3, Auto DynUp changing
	
		m_htiMinUpload = m_ctrlTreeOptions.InsertItem(GetResString(IDS_MINUPLOAD), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiDYNUP);
		m_ctrlTreeOptions.AddEditBox(m_htiMinUpload, RUNTIME_CLASS(CNumTreeOptionsEdit));
		
		m_htiUpSecu = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_SECURITY), iImgSecu, m_htiUM);
		m_htiEnableAntiLeecher = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_ANTI_LEECHER), m_htiUpSecu, m_bEnableAntiLeecher); //MORPH - Added by IceCream, Enable Anti-leecher
		m_htiEnableAntiCreditHack = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_ANTI_CREDITHACK), m_htiUpSecu, m_bEnableAntiCreditHack); //MORPH - Added by IceCream, Enable Anti-CreditHack
		m_htiIsBoostFriends = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_BOOST_FRIENDS), m_htiUM, m_bIsBoostFriends);//Added by Yun.SF3, boost friends
		//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
		m_htiHideOS = m_ctrlTreeOptions.InsertItem(GetResString(IDS_HIDEOVERSHARES), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiUM);
		m_ctrlTreeOptions.AddEditBox(m_htiHideOS, RUNTIME_CLASS(CNumTreeOptionsEdit));
		m_htiSelectiveShare = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_SELECTIVESHARE), m_htiHideOS, m_iSelectiveShare);
		//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS
		m_htiSFM = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_SFM), iImgSFM, TVI_ROOT);
		//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
		m_htiShareOnlyTheNeed = m_ctrlTreeOptions.InsertItem(GetResString(IDS_SHAREONLYTHENEEDDEFAULT), TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT, m_htiSFM);
		m_ctrlTreeOptions.AddEditBox(m_htiShareOnlyTheNeed, RUNTIME_CLASS(CNumTreeOptionsEdit));
		//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
		//MORPH START - Added by SiRoB, Show Permission
		m_htiPermissions = m_ctrlTreeOptions.InsertGroup(GetResString(IDS_PERMISSION), iImgPerm, m_htiSFM);
		m_htiPermAll = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_FSTATUS_PUBLIC), m_htiPermissions, m_iPermissions == 0);
		m_htiPermFriend = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_FSTATUS_FRIENDSONLY), m_htiPermissions, m_iPermissions == 1);
		m_htiPermNone = m_ctrlTreeOptions.InsertRadioButton(GetResString(IDS_HIDDEN), m_htiPermissions, m_iPermissions == 2);

		// Mighty Knife: Community visible filelist
		m_htiPermCommunity = m_ctrlTreeOptions.InsertRadioButton("Community", m_htiPermissions, m_iPermissions == 3);
		// [end] Mighty Knife

		//MORPH END   - Added by SiRoB, Show Permission
		m_htiIsAutoPowershareNewDownloadFile = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_POWERSHARE_AUTONEWDOWNLOADFILE), m_htiDM, m_bIsAutoPowershareNewDownloadFile); //MORPH - Added by SiRoB, Avoid misusing of powersharing
		//MORPH START - Added by IceCream, high process priority
		m_htiHighProcess = m_ctrlTreeOptions.InsertCheckBox(GetResString(IDS_HIGHPROCESS), TVI_ROOT, m_iHighProcess);
		//MORPH END   - Added by IceCream, high process priority

		// #ifdef MIGHTY_SUMMERTIME
		// Mighty Knife: daylight saving patch
		m_htiDaylightSavingPatch = m_ctrlTreeOptions.InsertCheckBox("Enable daylight saving patch (Warning: will eventually rehash your files! Please restart EMule!)", TVI_ROOT, m_iDaylightSavingPatch);
		// #endif

		// Mighty Knife: Community visualization, Report hashing files, Log friendlist activities
		m_htiCommunityName = m_ctrlTreeOptions.InsertItem("Community Tags (separated by '|')", TREEOPTSCTRLIMG_EDIT, TREEOPTSCTRLIMG_EDIT);
		m_ctrlTreeOptions.AddEditBox(m_htiCommunityName, RUNTIME_CLASS(CTreeOptionsEdit));
		m_htiReportHashingFiles = m_ctrlTreeOptions.InsertCheckBox("Report file hashing activities", TVI_ROOT, m_bReportHashingFiles);
		m_htiLogFriendlistActivities = m_ctrlTreeOptions.InsertCheckBox("Report activities in friendlist", TVI_ROOT, m_bLogFriendlistActivities);
		// [end] Mighty Knife

		m_ctrlTreeOptions.Expand(m_htiDM, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiUM, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiSFM, TVE_EXPAND);
		m_ctrlTreeOptions.Expand(m_htiHideOS, TVE_EXPAND);
		m_ctrlTreeOptions.SendMessage(WM_VSCROLL, SB_TOP);
		m_bInitializedTreeOpts = true;
	}
	DDX_TreeRadio(pDX, IDC_MORPH_OPTS, m_htiDYNUP, m_iDynUpMode);
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiMaxConnectionsSwitchBorder, m_iMaxConnectionsSwitchBorder);//MORPH - Added by Yun.SF3, Auto DynUp changing
	DDV_MinMaxInt(pDX, m_iMaxConnectionsSwitchBorder, 20 , 60000);//MORPH - Added by Yun.SF3, Auto DynUp changing
	
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiSUCLog, m_iSUCLog);
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiSUCHigh, m_iSUCHigh);
	DDV_MinMaxInt(pDX, m_iSUCHigh, 350, 1000);
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiSUCLow, m_iSUCLow);
	DDV_MinMaxInt(pDX, m_iSUCLow, 350, m_iSUCHigh);
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiSUCPitch, m_iSUCPitch);
	DDV_MinMaxInt(pDX, m_iSUCPitch, 2500, 10000);
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiSUCDrift, m_iSUCDrift);
	DDV_MinMaxInt(pDX, m_iSUCDrift, 0, 100);
	
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiUSSLog, m_iUSSLog);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiUSSLimit, m_iUSSLimit); // EastShare - Added by TAHO, USS limit
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiUSSPingLimit, m_iUSSPingLimit); // EastShare - Added by TAHO, USS limit
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiUSSPingTolerance, m_iUSSPingTolerance);
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiUSSGoingUpDivider, m_iUSSGoingUpDivider);
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiUSSGoingDownDivider, m_iUSSGoingDownDivider);
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiUSSNumberOfPings, m_iUSSNumberOfPings);
	
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiMinUpload, m_iMinUpload);
	DDV_MinMaxInt(pDX, m_iMinUpload, 1, app_prefs->GetMaxGraphUploadRate());

	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiEnableZeroFilledTest, m_bEnableZeroFilledTest);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiEnableDownloadInRed, m_bEnableDownloadInRed); //MORPH - Added by IceCream, show download in red
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiEnableDownloadInBold, m_bEnableDownloadInBold); //MORPH - Added by SiRoB, show download in Bold
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiEnableAntiLeecher, m_bEnableAntiLeecher); //MORPH - Added by IceCream, enable Anti-leecher
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiEnableAntiCreditHack, m_bEnableAntiCreditHack); //MORPH - Added by IceCream, enable Anti-CreditHack
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiIsBoostFriends, m_bIsBoostFriends);//Added by Yun.SF3, boost friends
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiIsAutoPowershareNewDownloadFile, m_bIsAutoPowershareNewDownloadFile);//MORPH - Added by SiRoB, Avoid misusing of powersharing

	// Mighty Knife: Community visualization
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiCommunityName, m_sCommunityName);
	// [end] Mighty Knife

	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiHideOS, m_iHideOS);
	DDV_MinMaxInt(pDX, m_iHideOS, 0, INT_MAX);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiSelectiveShare, m_iSelectiveShare);
	//MORPH END - Added by SiRoB, SLUGFILLER: hideOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiShareOnlyTheNeed, m_iShareOnlyTheNeed);
	DDV_MinMaxInt(pDX, m_iShareOnlyTheNeed, 0, INT_MAX);
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
	//MORPH START - Added by SiRoB, Show Permission
	DDX_TreeRadio(pDX, IDC_MORPH_OPTS, m_htiPermissions, m_iPermissions);
	//MORPH END   - Added by SiRoB, Show Permission
	//MORPH START - Added by SiRoB, khaos::categorymod+
	DDX_TreeEdit(pDX, IDC_MORPH_OPTS, m_htiResumeFileInNewCat, m_iResumeFileInNewCat);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiShowCatNames, m_iShowCatNames);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiSelectCat, m_iSelectCat);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiUseActiveCat, m_iUseActiveCat);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiAutoSetResOrder, m_iAutoSetResOrder);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiSmallFileDLPush, m_iSmallFileDLPush);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiShowA4AFDebugOutput, m_iShowA4AFDebugOutput);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiSmartA4AFSwapping, m_iSmartA4AFSwapping);
	DDX_TreeRadio(pDX, IDC_MORPH_OPTS, m_htiAdvA4AFMode, m_iAdvA4AFMode);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiUseAutoCat, m_iUseAutoCat);
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiUseSLS, m_iUseSLS);
	// khaos::accuratetimerem+
	DDX_TreeRadio(pDX, IDC_MORPH_OPTS, m_htiTimeRemainingMode, m_iTimeRemainingMode);
	// khaos::accuratetimerem-
	//MORPH END - Added by SiRoB, khaos::categorymod+
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiHighProcess, m_iHighProcess); //MORPH - Added by IceCream, high process priority 
	// #ifdef MIGHTY_SUMMERTIME
	// Mighty Knife: daylight saving patch
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiDaylightSavingPatch, m_iDaylightSavingPatch); 
	// #endif

	// Mighty Knife: Report hashing files, Log friendlist activities
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiReportHashingFiles, m_bReportHashingFiles); 
	DDX_TreeCheck(pDX, IDC_MORPH_OPTS, m_htiLogFriendlistActivities, m_bLogFriendlistActivities); 
	// [end] Mighty Knife

}


BOOL CPPgMorph::OnInitDialog()
{
	if (app_prefs->prefs->isautodynupswitching)
		m_iDynUpMode = 3;
	else if (app_prefs->prefs->m_bSUCEnabled)
		m_iDynUpMode = 1;
	else if (app_prefs->prefs->m_bDynUpEnabled)
		m_iDynUpMode = 2;
	else
		m_iDynUpMode = 0;
	m_iMaxConnectionsSwitchBorder = app_prefs->prefs->maxconnectionsswitchborder;//MORPH - Added by Yun.SF3, Auto DynUp changing
	m_iSUCLog =  app_prefs->prefs->m_bSUCLog;
	m_iSUCHigh = app_prefs->prefs->m_iSUCHigh;
	m_iSUCLow = app_prefs->prefs->m_iSUCLow;
	m_iSUCPitch = app_prefs->prefs->m_iSUCPitch;
	m_iSUCDrift = app_prefs->prefs->m_iSUCDrift;;
	m_iUSSLog = app_prefs->prefs->m_bDynUpLog;
	m_iUSSLimit = app_prefs->prefs->m_bIsUSSLimit; // EastShare - Added by TAHO, USS limit
	m_iUSSPingLimit = app_prefs->prefs->m_iDynUpPingLimit; // EastShare - Added by TAHO, USS limit
    m_iUSSPingTolerance = app_prefs->prefs->m_iDynUpPingTolerance;
    m_iUSSGoingUpDivider = app_prefs->prefs->m_iDynUpGoingUpDivider;
    m_iUSSGoingDownDivider = app_prefs->prefs->m_iDynUpGoingDownDivider;
    m_iUSSNumberOfPings = app_prefs->prefs->m_iDynUpNumberOfPings;
	m_iMinUpload = app_prefs->prefs->minupload;
	m_bEnableZeroFilledTest = app_prefs->prefs->enableZeroFilledTest;
	m_bEnableDownloadInRed = app_prefs->prefs->enableDownloadInRed; //MORPH - Added by IceCream, show download in red
	m_bEnableDownloadInBold = app_prefs->prefs->enableDownloadInBold; //MORPH - Added by SiRoB, show download in Bold
	m_bEnableAntiLeecher = app_prefs->prefs->enableAntiLeecher; //MORPH - Added by IceCream, enabnle Anti-leecher
	m_bEnableAntiCreditHack = app_prefs->prefs->enableAntiCreditHack; //MORPH - Added by IceCream, enabnle Anti-CreditHack
	m_bIsBoostFriends = app_prefs->prefs->isboostfriends;//Added by Yun.SF3, boost friends
	m_bIsAutoPowershareNewDownloadFile = app_prefs->prefs->m_bisautopowersharenewdownloadfile;//MORPH - Added by SiRoB, Avoid misusing of powersharing
	m_iHideOS = app_prefs->prefs->hideOS; //MORPH - Added by SiRoB, SLUGFILLER: hideOS
	m_iSelectiveShare = app_prefs->prefs->selectiveShare; //MORPH - Added by SiRoB, SLUGFILLER: hideOS
	m_iShareOnlyTheNeed = app_prefs->prefs->ShareOnlyTheNeed; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
	m_iPermissions = app_prefs->prefs->permissions; //MORPH - Added by SiRoB, Show Permission
	
	// Mighty Knife: Community visualization
	m_sCommunityName = app_prefs->prefs->m_sCommunityName;
	// [end] Mighty Knife

	//MORPH START - Added by SiRoB, khaos::categorymod+
	m_iShowCatNames = app_prefs->ShowCatNameInDownList();
	m_iSelectCat = app_prefs->SelectCatForNewDL();
	m_iUseActiveCat = app_prefs->UseActiveCatForLinks();
	m_iAutoSetResOrder = app_prefs->AutoSetResumeOrder();
	m_iShowA4AFDebugOutput = app_prefs->prefs->m_bShowA4AFDebugOutput;
	m_iSmartA4AFSwapping = app_prefs->UseSmartA4AFSwapping();
	m_iAdvA4AFMode = app_prefs->AdvancedA4AFMode();
	m_iSmallFileDLPush = app_prefs->SmallFileDLPush();
	m_iResumeFileInNewCat = app_prefs->StartDLInEmptyCats();
	m_iUseAutoCat = app_prefs->UseAutoCat();
	m_iUseSLS = app_prefs->UseSaveLoadSources();
	// khaos::accuratetimerem+
	m_iTimeRemainingMode = app_prefs->GetTimeRemainingMode();
	// khaos::accuratetimerem-
	//MORPH END - Added by SiRoB, khaos::categorymod+
	//MORPH START - Added by IceCream, high process priority
	m_iHighProcess = app_prefs->GetEnableHighProcess();
	//MORPH END   - Added by IceCream, high process priority
	
	// #ifdef MIGHTY_SUMMERTIME
	// Mighty Knife: daylight saving patch
	m_iDaylightSavingPatch = app_prefs->GetDaylightSavingPatch();
	// #endif

	// Mighty Knife: Report hashing files, Log friendlist activities
	m_bReportHashingFiles = app_prefs->GetReportHashingFiles ();
	m_bLogFriendlistActivities = app_prefs->GetLogFriendlistActivities ();
	// [end] Mighty Knife

	CPropertyPage::OnInitDialog();
	Localize();
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CPPgMorph::OnKillActive()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	return CPropertyPage::OnKillActive();
}

BOOL CPPgMorph::OnApply()
{
	// if prop page is closed by pressing VK_ENTER we have to explicitly commit any possibly pending
	// data from an open edit control
	m_ctrlTreeOptions.HandleChildControlLosingFocus();
	
	if (!UpdateData())
		return FALSE;
	if (m_iDynUpMode == 3)
		app_prefs->prefs->isautodynupswitching = true;//MORPH - Added by Yun.SF3, Auto DynUp changing
	else{
		app_prefs->prefs->isautodynupswitching = false;
		app_prefs->prefs->m_bSUCEnabled = (m_iDynUpMode == 1);
		app_prefs->prefs->m_bDynUpEnabled = (m_iDynUpMode == 2);
	}
	app_prefs->prefs->maxconnectionsswitchborder = m_iMaxConnectionsSwitchBorder;//MORPH - Added by Yun.SF3, Auto DynUp changing
	
	app_prefs->prefs->m_bSUCLog = m_iSUCLog;
	app_prefs->prefs->m_iSUCHigh = m_iSUCHigh;
	app_prefs->prefs->m_iSUCLow = m_iSUCLow;
	app_prefs->prefs->m_iSUCPitch = m_iSUCPitch;
	app_prefs->prefs->m_iSUCDrift = m_iSUCDrift;
	app_prefs->prefs->m_bDynUpLog = m_iUSSLog;
	app_prefs->prefs->m_bIsUSSLimit = m_iUSSLimit; // EastShare - Added by TAHO, USS limit
	app_prefs->prefs->m_iDynUpPingLimit = m_iUSSPingLimit; // EastShare - Added by TAHO, USS limit
    app_prefs->prefs->m_iDynUpPingTolerance = m_iUSSPingTolerance;
    app_prefs->prefs->m_iDynUpGoingUpDivider = m_iUSSGoingUpDivider;
    app_prefs->prefs->m_iDynUpGoingDownDivider = m_iUSSGoingDownDivider;
    app_prefs->prefs->m_iDynUpNumberOfPings = m_iUSSNumberOfPings;
	app_prefs->SetMinUpload(m_iMinUpload);
	app_prefs->prefs->enableZeroFilledTest = m_bEnableZeroFilledTest;
	app_prefs->prefs->enableDownloadInRed = m_bEnableDownloadInRed; //MORPH - Added by IceCream, show download in red
	app_prefs->prefs->enableDownloadInBold = m_bEnableDownloadInBold; //MORPH - Added by SiRoB, show download in Bold
	app_prefs->prefs->enableAntiLeecher = m_bEnableAntiLeecher; //MORPH - Added by IceCream, enable Anti-leecher
	app_prefs->prefs->enableAntiCreditHack = m_bEnableAntiCreditHack; //MORPH - Added by IceCream, enable Anti-CreditHack
	app_prefs->prefs->isboostfriends = m_bIsBoostFriends;//Added by Yun.SF3, boost friends
	app_prefs->prefs->m_bisautopowersharenewdownloadfile = m_bIsAutoPowershareNewDownloadFile;//MORPH - Added by SiRoB, Avoid misusing of powersharing
	app_prefs->prefs->hideOS = m_iHideOS;	//MORPH - Added by SiRoB, SLUGFILLER: hideOS
	app_prefs->prefs->selectiveShare = m_iSelectiveShare; //MORPH - Added by SiRoB, SLUGFILLER: hideOS
	app_prefs->prefs->ShareOnlyTheNeed = m_iShareOnlyTheNeed; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
	app_prefs->prefs->permissions = m_iPermissions; //MORPH - Added by SiRoB, Show Permission
	theApp.emuledlg->serverwnd->ToggleDebugWindow();
	theApp.emuledlg->serverwnd->UpdateLogTabSelection();

	// Mighty Knife: Community visualization
	sprintf (app_prefs->prefs->m_sCommunityName,"%s", m_sCommunityName);
	// [end] Mighty Knife

	//MORPH START - Added by SiRoB, khaos::categorymod+
	app_prefs->prefs->m_bShowCatNames = m_iShowCatNames;
	app_prefs->prefs->m_bSelCatOnAdd = m_iSelectCat;
	app_prefs->prefs->m_bActiveCatDefault = m_iUseActiveCat;
	app_prefs->prefs->m_bAutoSetResumeOrder = m_iAutoSetResOrder;
	app_prefs->prefs->m_bShowA4AFDebugOutput = m_iShowA4AFDebugOutput;
	app_prefs->prefs->m_bSmartA4AFSwapping = m_iSmartA4AFSwapping;
	app_prefs->prefs->m_iAdvancedA4AFMode = m_iAdvA4AFMode;
	app_prefs->prefs->m_bSmallFileDLPush = m_iSmallFileDLPush;
	app_prefs->prefs->m_iStartDLInEmptyCats = m_iResumeFileInNewCat;
	app_prefs->prefs->m_bUseAutoCat = m_iUseAutoCat;
	app_prefs->prefs->m_bUseSaveLoadSources = m_iUseSLS;
	// khaos::accuratetimerem+
	app_prefs->prefs->m_iTimeRemainingMode = m_iTimeRemainingMode;
	// khaos::accuratetimerem-
	//MORPH END - Added by SiRoB, khaos::categorymod+
	//MORPH START - Added by IceCream, high process priority
	app_prefs->SetEnableHighProcess(m_iHighProcess);
	//MORPH END   - Added by IceCream, high process priority

	// #ifdef MIGHTY_SUMMERTIME
	// Mighty Knife: daylight saving patch
	app_prefs->SetDaylightSavingPatch(m_iDaylightSavingPatch);
	// #endif

	// Mighty Knife: Report hashing files, Log friendlist activities
	app_prefs->SetReportHashingFiles (m_bReportHashingFiles);
	app_prefs->SetLogFriendlistActivities (m_bLogFriendlistActivities);
	// [end] Mighty Knife

	SetModified(FALSE);


	return CPropertyPage::OnApply();
}

void CPPgMorph::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	SetModified(TRUE);
	CSliderCtrl* slider =(CSliderCtrl*)pScrollBar;
	CString temp;
}


void CPPgMorph::Localize(void)
{	
	if(m_hWnd)
	{
		GetDlgItem(IDC_WARNINGMORPH)->SetWindowText(GetResString(IDS_WARNINGMORPH));
		CString Buffer;
		//MORPH START - Added by Yun.SF3, Auto DynUp changing
		if (m_htiDynUpAutoSwitching) m_ctrlTreeOptions.SetItemText(m_htiDynUpAutoSwitching, GetResString(IDS_AUTODYNUPSWITCHING));
		if (m_htiMaxConnectionsSwitchBorder){
			Buffer.Format(GetResString(IDS_MAXCONNECTIONSSWITCHBORDER),100);
			m_ctrlTreeOptions.SetEditLabel(m_htiMaxConnectionsSwitchBorder, Buffer);
		}
		//MORPH END - Added by Yun.SF3, Auto DynUp changing
		if (m_htiDynUpSUC) m_ctrlTreeOptions.SetItemText(m_htiDynUpSUC, GetResString(IDS_SUC));
		if (m_htiDynUpUSS) m_ctrlTreeOptions.SetItemText(m_htiDynUpUSS, GetResString(IDS_USS));

		if (m_htiSUCLog) m_ctrlTreeOptions.SetItemText(m_htiSUCLog, GetResString(IDS_SUC_LOG));
		if (m_htiSUCHigh){
			Buffer.Format(GetResString(IDS_SUC_HIGH),900);
			m_ctrlTreeOptions.SetEditLabel(m_htiSUCHigh, Buffer);
		}
		if (m_htiSUCLow){
			Buffer.Format(GetResString(IDS_SUC_LOW),600);
			m_ctrlTreeOptions.SetEditLabel(m_htiSUCLow, Buffer);
		}
		if (m_htiSUCPitch){
			Buffer.Format(GetResString(IDS_SUC_PITCH),3000);
			m_ctrlTreeOptions.SetEditLabel(m_htiSUCPitch, Buffer);
		}
		if (m_htiSUCDrift){
			Buffer.Format(GetResString(IDS_SUC_DRIFT),50);
			m_ctrlTreeOptions.SetEditLabel(m_htiSUCDrift, Buffer);
		}
		if (m_htiUSSLog) m_ctrlTreeOptions.SetItemText(m_htiUSSLog, GetResString(IDS_USS_LOG));
		if (m_htiUSSPingTolerance){
			Buffer.Format(GetResString(IDS_USS_PINGTOLERANCE),800);
			m_ctrlTreeOptions.SetEditLabel(m_htiUSSPingTolerance, Buffer);
		}
		if (m_htiUSSGoingUpDivider){
			Buffer.Format(GetResString(IDS_USS_GOINGUPDIVIDER),1000);
			m_ctrlTreeOptions.SetEditLabel(m_htiUSSGoingUpDivider, Buffer);
		}
		if (m_htiUSSGoingDownDivider){
			Buffer.Format(GetResString(IDS_USS_GOINGDOWNDIVIDER),1000);
			m_ctrlTreeOptions.SetEditLabel(m_htiUSSGoingDownDivider, Buffer);
		}
		if (m_htiUSSNumberOfPings){
			Buffer.Format(GetResString(IDS_USS_NUMBEROFPINGS),1);
			m_ctrlTreeOptions.SetEditLabel(m_htiUSSNumberOfPings, Buffer);
		}
		if (m_htiMinUpload) m_ctrlTreeOptions.SetEditLabel(m_htiMinUpload, GetResString(IDS_MINUPLOAD));		
		if (m_htiEnableZeroFilledTest) m_ctrlTreeOptions.SetItemText(m_htiEnableZeroFilledTest, GetResString(IDS_ZERO_FILLED_TEST));
		if (m_htiEnableDownloadInRed) m_ctrlTreeOptions.SetItemText(m_htiEnableDownloadInRed, GetResString(IDS_DOWNLOAD_IN_RED)); //MORPH - Added by IceCream, show download in red
		if (m_htiEnableDownloadInBold) m_ctrlTreeOptions.SetItemText(m_htiEnableDownloadInBold, GetResString(IDS_DOWNLOAD_IN_BOLD)); //MORPH - Added by SiRoB, show download in Bold
		if (m_htiEnableAntiLeecher) m_ctrlTreeOptions.SetItemText(m_htiEnableAntiLeecher, GetResString(IDS_ANTI_LEECHER)); //MORPH - Added by IceCream, enable Anti-leecher
		if (m_htiEnableAntiCreditHack) m_ctrlTreeOptions.SetItemText(m_htiEnableAntiCreditHack, GetResString(IDS_ANTI_CREDITHACK)); //MORPH - Added by IceCream, enable Anti-CreditHack
		if (m_htiIsBoostFriends) m_ctrlTreeOptions.SetItemText(m_htiIsBoostFriends, GetResString(IDS_BOOST_FRIENDS));//Added by Yun.SF3, boost friends
		if (m_htiHideOS) m_ctrlTreeOptions.SetEditLabel(m_htiHideOS, GetResString(IDS_HIDEOVERSHARES));//MORPH - Added by SiRoB, SLUGFILLER: hideOS
		if (m_htiSelectiveShare) m_ctrlTreeOptions.SetItemText(m_htiSelectiveShare, GetResString(IDS_SELECTIVESHARE));//MORPH - Added by SiRoB, SLUGFILLER: hideOS
		if (m_htiShareOnlyTheNeed) m_ctrlTreeOptions.SetEditLabel(m_htiShareOnlyTheNeed, GetResString(IDS_SHAREONLYTHENEEDDEFAULT));//MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
		if (m_htiIsAutoPowershareNewDownloadFile) m_ctrlTreeOptions.SetItemText(m_htiIsAutoPowershareNewDownloadFile, GetResString(IDS_POWERSHARE_AUTONEWDOWNLOADFILE)); //MORPH - Added by SiRoB, Avoid misusing of powersharing
		//MORPH START - Added by SiRoB, Show Permission
		if (m_htiPermissions) m_ctrlTreeOptions.SetItemText(m_htiPermissions, GetResString(IDS_PERMISSION));
		if (m_htiPermAll) m_ctrlTreeOptions.SetItemText(m_htiPermAll, GetResString(IDS_FSTATUS_PUBLIC));
		if (m_htiPermFriend) m_ctrlTreeOptions.SetItemText(m_htiPermFriend, GetResString(IDS_FSTATUS_FRIENDSONLY));
		if (m_htiPermNone) m_ctrlTreeOptions.SetItemText(m_htiPermNone, GetResString(IDS_HIDDEN));
		//MORPH END   - Added by SiRoB, Show Permission
		//MORPH START - Added by SiRoB, khaos::categorymod+
		if (m_htiShowCatNames) m_ctrlTreeOptions.SetItemText(m_htiShowCatNames, GetResString(IDS_CAT_SHOWCATNAME));
		if (m_htiSelectCat) m_ctrlTreeOptions.SetItemText(m_htiSelectCat, GetResString(IDS_CAT_SHOWSELCATDLG));
		if (m_htiUseAutoCat) m_ctrlTreeOptions.SetItemText(m_htiUseAutoCat, GetResString(IDS_CAT_USEAUTOCAT));
		if (m_htiUseActiveCat) m_ctrlTreeOptions.SetItemText(m_htiUseActiveCat, GetResString(IDS_CAT_USEACTIVE));
		if (m_htiAutoSetResOrder) m_ctrlTreeOptions.SetItemText(m_htiAutoSetResOrder, GetResString(IDS_CAT_AUTORESUMEORD));
		if (m_htiSmallFileDLPush) m_ctrlTreeOptions.SetItemText(m_htiSmallFileDLPush, GetResString(IDS_CAT_SMALLFILEDLPUSH));
		if (m_htiResumeFileInNewCat) m_ctrlTreeOptions.SetEditLabel(m_htiResumeFileInNewCat, GetResString(IDS_CAT_RESUMENEXT));
		if (m_htiSmartA4AFSwapping) m_ctrlTreeOptions.SetItemText(m_htiSmartA4AFSwapping, GetResString(IDS_A4AF_SMARTSWAP));
		if (m_htiShowA4AFDebugOutput) m_ctrlTreeOptions.SetItemText(m_htiShowA4AFDebugOutput, GetResString(IDS_A4AF_SHOWDEBUG));
		if (m_htiAdvA4AFMode) m_ctrlTreeOptions.SetItemText(m_htiAdvA4AFMode, /*GetResString(IDS_DEFAULT) + " " +*/ GetResString(IDS_A4AF_ADVMODE));
		if (m_htiDisableAdvA4AF) m_ctrlTreeOptions.SetItemText(m_htiDisableAdvA4AF, GetResString(IDS_A4AF_DISABLED));
		if (m_htiBalanceSources) m_ctrlTreeOptions.SetItemText(m_htiBalanceSources, GetResString(IDS_A4AF_BALANCE));
		if (m_htiStackSources) m_ctrlTreeOptions.SetItemText(m_htiStackSources, GetResString(IDS_A4AF_STACK));
		if (m_htiUseSLS) m_ctrlTreeOptions.SetItemText(m_htiUseSLS, GetResString(IDS_SLS_USESLS));
		// khaos::accuratetimerem+
		if (m_htiTimeRemainingMode) m_ctrlTreeOptions.SetItemText(m_htiTimeRemainingMode, GetResString(IDS_REMTIMEAVRREAL));
		if (m_htiTimeRemBoth) m_ctrlTreeOptions.SetItemText(m_htiTimeRemBoth, GetResString(IDS_BOTH));
		if (m_htiTimeRemRealTime) m_ctrlTreeOptions.SetItemText(m_htiTimeRemRealTime, GetResString(IDS_REALTIME));
		if (m_htiTimeRemAverage) m_ctrlTreeOptions.SetItemText(m_htiTimeRemAverage, GetResString(IDS_AVG));
		// khaos::accuratetimerem-
		//MORPH END - Added by SiRoB, khaos::categorymod+
		//MORPH START - Added by IceCream, high process priority
		if (m_htiHighProcess) m_ctrlTreeOptions.SetItemText(m_htiHighProcess, GetResString(IDS_HIGHPROCESS));
		//MORPH END   - Added by IceCream, high process priority
	}

}

void CPPgMorph::OnDestroy()
{
	m_ctrlTreeOptions.DeleteAllItems();
	m_ctrlTreeOptions.DestroyWindow();
	m_bInitializedTreeOpts = false;
	m_htiDM = NULL;
	m_htiUM = NULL;
	m_htiDYNUP = NULL;
	m_htiDynUpOFF = NULL;
	m_htiDynUpSUC = NULL;
	m_htiDynUpUSS = NULL;
	m_htiDynUpAutoSwitching = NULL;//MORPH - Added by Yun.SF3, Auto DynUp changing
	m_htiMaxConnectionsSwitchBorder = NULL;//MORPH - Added by Yun.SF3, Auto DynUp changing
	
	m_htiSUCLog = NULL;
	m_htiSUCHigh = NULL;
	m_htiSUCLow = NULL;
	m_htiSUCPitch = NULL;
	m_htiSUCDrift = NULL;
	m_htiUSSLog = NULL;
	m_htiUSSLimit = NULL; // EastShare - Added by TAHO, USS limit
	m_htiUSSPingLimit = NULL;
    m_htiUSSPingTolerance = NULL;
    m_htiUSSGoingUpDivider = NULL;
    m_htiUSSGoingDownDivider = NULL;
    m_htiUSSNumberOfPings = NULL;
	m_htiMinUpload = NULL;
	m_htiDlSecu = NULL;
	m_htiEnableZeroFilledTest = NULL;
	m_htiDisp = NULL;
	m_htiEnableDownloadInRed = NULL; //MORPH - Added by IceCream, show download in red
	m_htiEnableDownloadInBold = NULL; //MORPH - Added by SiRoB, show download in Bold
	m_htiUpSecu = NULL;
	m_htiEnableAntiLeecher = NULL; //MORPH - Added by IceCream, enable Anti-leecher
	m_htiEnableAntiCreditHack = NULL; //MORPH - Added by IceCream, enable Anti-CreditHack
	m_htiSCC = NULL;
	//MORPH START - Added by SiRoB, khaos::categorymod+
	m_htiShowCatNames = NULL;
	m_htiSelectCat = NULL;
	m_htiUseActiveCat = NULL;
	m_htiAutoSetResOrder = NULL;
	m_htiSmartA4AFSwapping = NULL;
	m_htiAdvA4AFMode = NULL;
	m_htiBalanceSources = NULL;
	m_htiStackSources = NULL;
	m_htiShowA4AFDebugOutput = NULL;
	m_htiDisableAdvA4AF = NULL;
	m_htiSmallFileDLPush = NULL;
	m_htiResumeFileInNewCat = NULL;
	m_htiUseAutoCat = NULL;
	m_htiUseSLS = NULL;
	// khaos::accuratetimerem+
	m_htiTimeRemainingMode = NULL;
	m_htiTimeRemBoth = NULL;
	m_htiTimeRemAverage = NULL;
	m_htiTimeRemRealTime = NULL;
	// khaos::accuratetimerem-
	//MORPH END - Added by SiRoB, khaos::categorymod+
	m_htiHighProcess = NULL; //MORPH - Added by IceCream, high process priority
	m_htiIsBoostFriends = NULL;//MORPH - Added by Yun.SF3, boost friends
	m_htiHideOS = NULL;	//MORPH - Added by SiRoB, SLUGFILLER: hideOS
	m_htiSelectiveShare = NULL;	//MORPH - Added by SiRoB, SLUGFILLER: hideOS
	m_htiShareOnlyTheNeed = NULL; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
	m_htiIsAutoPowershareNewDownloadFile = NULL; //MORPH - Added by SiRoB, Avoid misusing of powersharing	
	//MORPH START - Added by SiRoB, Show Permission
	m_htiPermissions = NULL;
	m_htiPermAll = NULL;
	m_htiPermFriend = NULL;
	m_htiPermNone = NULL;
	//MORPH END   - Added by SiRoB, Show Permission
	CPropertyPage::OnDestroy();
}
LRESULT CPPgMorph::OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam)
{
	if (wParam == IDC_MORPH_OPTS){
		TREEOPTSCTRLNOTIFY* pton = (TREEOPTSCTRLNOTIFY*)lParam;
		//		if (bCheck && m_ctrlTreeOptions.GetCheckBox(m_htiUSSEnabled,bCheck))
		//			if (bCheck) m_ctrlTreeOptions.SetCheckBox(m_htiUSSEnabled,false);
		//}else if (pton->hItem == m_htiUSSEnabled){
		//	BOOL bCheck;
		//	if (m_ctrlTreeOptions.GetCheckBox(m_htiUSSEnabled, bCheck))
		//		if (bCheck && m_ctrlTreeOptions.GetCheckBox(m_htiSUCEnabled,bCheck))
		//			if (bCheck) m_ctrlTreeOptions.SetCheckBox(m_htiSUCEnabled,false);
		//	
		//}
		SetModified();
	}
	return 0;
}