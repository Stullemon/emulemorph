#pragma once

// CPPgStats dialog

#include "preferences.h"
#include "ColorButton.h"

class CPPgStats : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgStats)

public:
	CPPgStats();
	virtual ~CPPgStats();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }

// Dialog Data
	enum { IDD = IDD_PPG_STATS };
protected:
	CPreferences *app_prefs;
	//{{AFX_MSG(CSpinToolBar)
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnCbnSelchangeColorselector();
	afx_msg LONG OnSelChange(UINT lParam, LONG wParam);
	//}}AFX_MSG
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
	// -khaos--+++> Added connection ratio combo box
	CComboBox m_colors, m_cratio;
	// <-----khaos-
	CColorButton m_ctlColor;
	void ShowInterval();
public:
	virtual BOOL OnInitDialog();
private:
	int mystats1,mystats2,mystats3;
public:
	// -khaos--+++> Events for the new pref ctrls
	afx_msg void OnEnChangeCGraphScale()		{SetModified();}
	afx_msg void OnCbnSelchangeCRatio()			{SetModified();}
	// <-----khaos-

	virtual BOOL OnApply();
	void Localize(void);
};
