#pragma once
#include "TreeOptionsCtrlEx.h"
// CPPgMorph dialog

class CPPgMorph : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgMorph)

public:
	CPPgMorph();
	virtual ~CPPgMorph();

// Dialog Data
	enum { IDD = IDD_PPG_MORPH };
protected:

	int m_iSUCLog;
	int m_iUSSLimit; // EastShare - Added by TAHO, USS limit
	int m_iSUCHigh;
	int m_iSUCLow;
	int m_iSUCPitch;
	int m_iSUCDrift;
	int m_iUSSLog;
	int m_iUSSPingLimit; // EastShare - Added by TAHO, USS limit
    int m_iUSSPingTolerance;
    int m_iUSSGoingUpDivider;
    int m_iUSSGoingDownDivider;
    int m_iUSSNumberOfPings;
	int m_iMinUpload;
	int m_bEnableZeroFilledTest;
	int m_bEnableDownloadInRed; //MORPH - Added by IceCream, show download in red
	int m_bEnableDownloadInBold; //MORPH - Added by SiRoB, show download in Bold
	int m_bEnableAntiLeecher; //MORPH - Added by IceCream, enable Anti-leecher
	int m_bEnableAntiCreditHack; //MORPH - Added by IceCream, enable Anti-CreditHack
	int m_bIsBoostFriends;//Added by Yun.SF3, boost friends
	int m_iInfiniteQueue;	//Morph - added by AndCycle, SLUGFILLER: infiniteQueue
	int	m_iDynUpMode;//MORPH - Added by Yun.SF3, Auto DynUp changing
	int	m_iMaxConnectionsSwitchBorder;//MORPH - Added by Yun.SF3, Auto DynUp changing
	int m_bIsAutoPowershareNewDownloadFile; //MORPH - Added by SiRoB, Avoid misusing of powersharing
	int m_iHideOS;	//MORPH - Added by SiRoB, SLUGFILLER: hideOS
	int m_iSelectiveShare;  //MORPH - Added by SiRoB, SLUGFILLER: hideOS
	int m_iShareOnlyTheNeed; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
	int m_iPermissions; //MORPH - Added by SiRoB, Show Permissions
	//MORPH START - Added by SiRoB, khaos::categorymod+
	int m_iShowCatNames;
	int m_iSelectCat;
	int m_iUseActiveCat;
	int m_iAutoSetResOrder;
	int m_iShowA4AFDebugOutput;
	int m_iSmartA4AFSwapping;
	int m_iAdvA4AFMode;
	int m_iSmallFileDLPush;
	int m_iResumeFileInNewCat;
	int m_iUseAutoCat;
	int m_iUseSLS;
	// khaos::accuratetimerem+
	int m_iTimeRemainingMode;
	// khaos::accuratetimerem-
	//MORPH END - Added by SiRoB, khaos::categorymod+
	int m_iHighProcess; //MORPH - Added by IceCream, high process priority

	CTreeOptionsCtrlEx m_ctrlTreeOptions;
	bool m_bInitializedTreeOpts;
	HTREEITEM m_htiDM;
	HTREEITEM m_htiUM;
	HTREEITEM m_htiSFM;
	HTREEITEM m_htiDYNUP;
	HTREEITEM m_htiDynUpOFF;
	HTREEITEM m_htiDynUpSUC;
	HTREEITEM m_htiDynUpUSS;
	HTREEITEM m_htiDynUpAutoSwitching;//MORPH - Added by Yun.SF3, Auto DynUp changing
	HTREEITEM m_htiMaxConnectionsSwitchBorder;//MORPH - Added by Yun.SF3, Auto DynUp changing
	HTREEITEM m_htiSUCLog;
	HTREEITEM m_htiSUCHigh;
	HTREEITEM m_htiSUCLow;
	HTREEITEM m_htiSUCPitch;
	HTREEITEM m_htiSUCDrift;
	HTREEITEM m_htiUSSLog;
	HTREEITEM m_htiUSSLimit; // EastShare - Added by TAHO, USS limit
	HTREEITEM m_htiUSSPingLimit; // EastShare - Added by TAHO, USS limit
    HTREEITEM m_htiUSSPingTolerance;
    HTREEITEM m_htiUSSGoingUpDivider;
    HTREEITEM m_htiUSSGoingDownDivider;
    HTREEITEM m_htiUSSNumberOfPings;
	HTREEITEM m_htiMinUpload;
	HTREEITEM m_htiEnableZeroFilledTest;
	HTREEITEM m_htiEnableDownloadInRed; //MORPH - Added by IceCream, show download in red
	HTREEITEM m_htiEnableDownloadInBold; //MORPH - Added by SiRoB, show download in Bold
	HTREEITEM m_htiEnableAntiLeecher; //MORPH - Added by IceCream, enable Anti-leecher
	HTREEITEM m_htiEnableAntiCreditHack; //MORPH - Added by IceCream, enable Anti-CreditHack
	HTREEITEM m_htiIsBoostFriends;//Added by Yun.SF3, boost friends
	HTREEITEM m_htiInfiniteQueue;	//Morph - added by AndCycle, SLUGFILLER: infiniteQueue
	HTREEITEM m_htiIsAutoPowershareNewDownloadFile;//MORPH - Added by SiRoB, Avoid misusing of powersharing
	HTREEITEM m_htiHideOS;	//MORPH - Added by SiRoB, SLUGFILLER: hideOS
	HTREEITEM m_htiSelectiveShare; //MORPH - Added by SiRoB, SLUGFILLER: hideOS
	HTREEITEM m_htiShareOnlyTheNeed; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
	//MORPH START - Added by SiRoB, Show Permission
	HTREEITEM m_htiPermissions;
	HTREEITEM m_htiPermAll;
	HTREEITEM m_htiPermFriend;
	HTREEITEM m_htiPermNone;
	//MORPH END   - Added by SiRoB, Show Permission
	
	HTREEITEM m_htiSCC;
	HTREEITEM m_htiSAC;
	HTREEITEM m_htiDisp;
	HTREEITEM m_htiDlSecu;
	HTREEITEM m_htiUpSecu;

	//MORPH START - Added by SiRoB, khaos::categorymod+
	HTREEITEM m_htiShowCatNames;
	HTREEITEM m_htiSelectCat;
	HTREEITEM m_htiUseActiveCat;
	HTREEITEM m_htiAutoSetResOrder;
	HTREEITEM m_htiShowA4AFDebugOutput;
	HTREEITEM m_htiSmartA4AFSwapping;
	HTREEITEM m_htiAdvA4AFMode;
	HTREEITEM m_htiBalanceSources;
	HTREEITEM m_htiStackSources;
	HTREEITEM m_htiDisableAdvA4AF;
	HTREEITEM m_htiSmallFileDLPush;
	HTREEITEM m_htiResumeFileInNewCat;
	HTREEITEM m_htiUseAutoCat;
	HTREEITEM m_htiUseSLS;
	// khaos::accuratetimerem+
	HTREEITEM m_htiTimeRemainingMode;
	HTREEITEM m_htiTimeRemBoth;
	HTREEITEM m_htiTimeRemAverage;
	HTREEITEM m_htiTimeRemRealTime;
	// khaos::accuratetimerem-
	//MORPH END - Added by SiRoB, khaos::categorymod+
	HTREEITEM m_htiHighProcess; //MORPH - Added by IceCream, high process priority

	// #ifdef MIGHTY_SUMMERTIME
	// Mighty Knife: daylight saving patch
	HTREEITEM m_htiDaylightSavingPatch;
	int m_iDaylightSavingPatch;
	// #endif

	// Mighty Knife: Community visualization, Report hashing files, Log friendlist activities
	CString   m_sCommunityName;
	HTREEITEM m_htiCommunityName;
	BOOL      m_bReportHashingFiles;
	HTREEITEM m_htiReportHashingFiles;
	BOOL	  m_bLogFriendlistActivities;
	HTREEITEM m_htiLogFriendlistActivities;
	// [end] Mighty Knife

	// Mighty Knife: Community visible filelist
	HTREEITEM m_htiPermCommunity;
	// [end] Mighty Knife

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	void Localize(void);	
	void LoadSettings(void);
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	virtual BOOL OnKillActive();
	afx_msg void OnSettingsChange()			{ SetModified(); }
};
