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
	int m_bEnableDownloadInRed; //MORPH - Added by IceCream, show download in red
	int m_bEnableDownloadInBold; //MORPH - Added by SiRoB, show download in Bold
	int m_bShowClientPercentage;
	int m_bEnableAntiLeecher; //MORPH - Added by IceCream, enable Anti-leecher
	int m_bEnableAntiCreditHack; //MORPH - Added by IceCream, enable Anti-CreditHack
	int m_iInfiniteQueue;	//Morph - added by AndCycle, SLUGFILLER: infiniteQueue
	int m_iDontRemoveSpareTrickleSlot; //Morph - added by AndCycle, Dont Remove Spare Trickle Slot
	int m_iFunnyNick; //MORPH - Added by SiRoB, Optionnal funnynick display
	int m_iClientQueueProgressBar; // MORPH - Added by Commander, ClientQueueProgressBar
	int m_iCountWCSessionStats; // MORPH - Added by Commander, Show WC stats

	//MORPH START - Added by SiRoB, Datarate Average Time Management
	int m_iDownloadDataRateAverageTime;
	int m_iUploadDataRateAverageTime;
	//MORPH END   - Added by SiRoB, Datarate Average Time Management
	
	//MORPH START - Added by SiRoB, Upload Splitting Class
	int m_iGlobalDataRateFriend;
	int m_iMaxClientDataRateFriend;
	int m_iGlobalDataRatePowerShare;
	int m_iMaxClientDataRatePowerShare;
	int m_iMaxClientDataRate;
	//MORPH END  - Added by SiRoB, Upload Splitting Class
	int	m_iDynUpMode;//MORPH - Added by Yun.SF3, Auto DynUp changing
	int	m_iMaxConnectionsSwitchBorder;//MORPH - Added by Yun.SF3, Auto DynUp changing
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
	int m_iUseICS; //MORPH - Added by SIRoB, ICS Optional
	int m_iHighProcess; //MORPH - Added by IceCream, high process priority

	CTreeOptionsCtrlEx m_ctrlTreeOptions;
	bool m_bInitializedTreeOpts;
	HTREEITEM m_htiDM;
	HTREEITEM m_htiUM;
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
	HTREEITEM m_htiEnableDownloadInRed; //MORPH - Added by IceCream, show download in red
	HTREEITEM m_htiEnableDownloadInBold; //MORPH - Added by SiRoB, show download in Bold
	HTREEITEM m_htiShowClientPercentage;
	HTREEITEM m_htiEnableAntiLeecher; //MORPH - Added by IceCream, enable Anti-leecher
	HTREEITEM m_htiEnableAntiCreditHack; //MORPH - Added by IceCream, enable Anti-CreditHack
	HTREEITEM m_htiInfiniteQueue;	//Morph - added by AndCycle, SLUGFILLER: infiniteQueue
	HTREEITEM m_htiDontRemoveSpareTrickleSlot; //Morph - added by AndCycle, Dont Remove Spare Trickle Slot
	HTREEITEM m_htiDisplayFunnyNick;//MORPH - Added by SiRoB, Optionnal funnynick display
	HTREEITEM m_htiCountWCSessionStats;//MORPH - Added by Commander, Show WC stats
	HTREEITEM m_htiClientQueueProgressBar; //MORPH - Added by Commander, ClientQueueProgressBar

	//MORPH START - Added by SiRoB, Datarate Average Time Management
	HTREEITEM m_htiDownloadDataRateAverageTime;
	HTREEITEM m_htiUploadDataRateAverageTime;
	//MORPH END   - Added by SiRoB, Datarate Average Time Management
	
	//MORPH START - Added by SiRoB, Upload Splitting Class
	HTREEITEM m_htiFriend;
	HTREEITEM m_htiGlobalDataRateFriend;
	HTREEITEM m_htiMaxClientDataRateFriend;
	HTREEITEM m_htiPowerShare;
	HTREEITEM m_htiGlobalDataRatePowerShare;
	HTREEITEM m_htiMaxClientDataRatePowerShare;
	HTREEITEM m_htiMaxClientDataRate;
	//MORPH END   - Added by SiRoB, Upload Splitting Class
	HTREEITEM m_htiUpDisplay; //MORPH - Added by Commander, UM -> Display

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
	HTREEITEM m_htiUseICS; //MORPH - Added by SIRoB, ICS Optional
	HTREEITEM m_htiHighProcess; //MORPH - Added by IceCream, high process priority

	// Mighty Knife: Community visualization, Report hashing files, Log friendlist activities
	CString   m_sCommunityName;
	HTREEITEM m_htiCommunityName;
	BOOL      m_bReportHashingFiles;
	HTREEITEM m_htiReportHashingFiles;
	BOOL	  m_bLogFriendlistActivities;
	HTREEITEM m_htiLogFriendlistActivities;
	// [end] Mighty Knife

	// Mighty Knife: Static server handling
	BOOL	  m_bDontRemoveStaticServers;
	HTREEITEM m_htiDontRemoveStaticServers;
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
