#pragma once
#include "TreeOptionsCtrlEx.h"

// CPPgTweaks dialog

class CPPgTweaks : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgTweaks)

public:
	CPPgTweaks();
	virtual ~CPPgTweaks();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs;}

// Dialog Data
	enum { IDD = IDD_PPG_TWEAKS };
protected:
	CPreferences *app_prefs;
	uint8 m_iFileBufferSize;
	uint8 m_iQueueSize;
	int m_iMaxConnPerFive;
	int m_iAutoTakeEd2kLinks;
	int m_iVerbose;
	int m_iDebugSourceExchange;
	int m_iCreditSystem;
	int m_iLog2Disk;
	int m_iDebug2Disk;
	int m_iDateFileNameLog;//Morph - added by AndCycle, Date File Name Log
	int m_iCommitFiles;
	int m_iFilterLANIPs;
	int m_iExtControls;
	UINT m_uServerKeepAliveTimeout;
	int m_iCheckDiskspace;	// SLUGFILLER: checkDiskspace
	float m_fMinFreeDiskSpaceMB;
	CString m_sYourHostname;	// itsonlyme: hostnameSource
	
	// ZZ:UploadSpeedSense -->
    int m_iDynUpEnabled;
    int m_iDynUpMinUpload;
    int m_iDynUpPingTolerance;
    int m_iDynUpGoingUpDivider;
    int m_iDynUpGoingDownDivider;
    int m_iDynUpNumberOfPings;
	// ZZ:UploadSpeedSense <--

	CTreeOptionsCtrlEx m_ctrlTreeOptions;
	bool m_bInitializedTreeOpts;
	HTREEITEM m_htiMaxCon5Sec;
	HTREEITEM m_htiAutoTakeEd2kLinks;
	HTREEITEM m_htiVerbose;
	HTREEITEM m_htiDebugSourceExchange;
	HTREEITEM m_htiCreditSystem;
	HTREEITEM m_htiSaveLogs;
	HTREEITEM m_htiLog2Disk;
	HTREEITEM m_htiDebug2Disk;
	HTREEITEM m_htiDateFileNameLog;//Morph - added by AndCycle, Date File Name Log
	HTREEITEM m_htiCommit;
	HTREEITEM m_htiCommitNever;
	HTREEITEM m_htiCommitOnShutdown;
	HTREEITEM m_htiCommitAlways;
	HTREEITEM m_htiFilterLANIPs;
	HTREEITEM m_htiExtControls;
	HTREEITEM m_htiServerKeepAliveTimeout;
	HTREEITEM m_htiCheckDiskspace;	// SLUGFILLER: checkDiskspace
	HTREEITEM m_htiMinFreeDiskSpace;
	HTREEITEM m_htiYourHostname;	// itsonlyme: hostnameSource
	
	// ZZ:UploadSpeedSense -->
    HTREEITEM m_htiDynUp;
	HTREEITEM m_htiDynUpEnabled;
    HTREEITEM m_htiDynUpMinUpload;
    HTREEITEM m_htiDynUpPingTolerance;
    HTREEITEM m_htiDynUpGoingUpDivider;
    HTREEITEM m_htiDynUpGoingDownDivider;
    HTREEITEM m_htiDynUpNumberOfPings;
	// ZZ:UploadSpeedSense <--

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	void Localize(void);
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
};
