#pragma once
#include "TreeOptionsCtrlEx.h"

class CPreferences;

#define	MAX_DEBUG_ITEMS	6

class CPPgDebug : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgDebug)

public:
	CPPgDebug();
	virtual ~CPPgDebug();

	void SetPrefs(CPreferences* in_prefs) { app_prefs = in_prefs;}

// Dialog Data
	enum { IDD = IDD_PPG_DEBUG };

protected:
	CPreferences *app_prefs;
	HTREEITEM m_htiServer;
	HTREEITEM m_htiClient;
	HTREEITEM m_cb[MAX_DEBUG_ITEMS];
	HTREEITEM m_lv[MAX_DEBUG_ITEMS];
	BOOL m_checks[MAX_DEBUG_ITEMS];
	int m_levels[MAX_DEBUG_ITEMS];
	CTreeOptionsCtrlEx m_ctrlTreeOptions;
	bool m_bInitializedTreeOpts;

	void ClearAllMembers();

	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnKillActive();
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg LRESULT OnTreeOptsCtrlNotify(WPARAM wParam, LPARAM lParam);
};
