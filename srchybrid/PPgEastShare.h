#pragma once

#include "preferences.h"
#include "wizard.h"
#include "TreeOptionsCtrlEx.h"
// CPPgEastShare dialog

class CPPgEastShare : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgEastShare)

public:
	CPPgEastShare();
	virtual ~CPPgEastShare();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs;}

// Dialog Data
	enum { IDD = IDD_PPG_EASTSHARE };
protected:
	CPreferences *app_prefs;
	
	int m_bEnablePreferShareAll;//EastShare - PreferShareAll by AndCycle
	HTREEITEM m_htiEnablePreferShareAll;//EastShare - PreferShareAll by AndCycle

	int m_bIsPayBackFirst;//EastShare - added by AndCycle, Pay Back First
	HTREEITEM m_htiIsPayBackFirst; //EastShare - added by AndCycle, Pay Back First

	int m_bAutoClearComplete;//EastShare - added by AndCycle - AutoClearComplete (NoamSon)
	HTREEITEM m_htiAutoClearComplete;//EastShare - added by AndCycle - AutoClearComplete (NoamSon)

	int m_bOnlyDownloadCompleteFiles;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	HTREEITEM m_htiOnlyDownloadCompleteFiles;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)

	int m_bSaveUploadQueueWaitTime;//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	HTREEITEM m_htiSaveUploadQueueWaitTime;//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)

	// EastShare START - Added by linekin, new creditsystem by [lovelace]
	CreditSystemSelection m_iCreditSystem;
	HTREEITEM m_htiCreditSystem;
	HTREEITEM m_htiOfficialCredit;
	HTREEITEM m_htiLovelaceCredit;
	HTREEITEM m_htiRatioCredit;
	HTREEITEM m_htiPawcioCredit;
	HTREEITEM m_htiESCredit;
	// EastShare START - Added by linekin, new creditsystem by [lovelace]

	//Morph - added by AndCycle, Equal Chance For Each File
	EqualChanceForEachFileSelection m_iEqualChanceForEachFile;
	HTREEITEM m_htiECFEF;
	HTREEITEM m_htiECFEF_DISABLE;
	HTREEITEM m_htiECFEF_ACCEPTED;
	HTREEITEM m_htiECFEF_ACCEPTED_COMPLETE;
	HTREEITEM m_htiECFEF_TRANSFERRED;
	HTREEITEM m_htiECFEF_TRANSFERRED_COMPLETE;
	//Morph - added by AndCycle, Equal Chance For Each File

	// EastShare START - Added by TAHO, .met control
	HTREEITEM m_htiMetControl;
	int m_iKnownMetDays;
	HTREEITEM m_htiKnownMet;
	// EastShare END - Added by TAHO, .met control

	CTreeOptionsCtrlEx m_ctrlTreeOptions;
	bool m_bInitializedTreeOpts;

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
