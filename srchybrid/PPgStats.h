#pragma once
#include "ColorButton.h"

class CPPgStats : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgStats)

public:
	CPPgStats();
	virtual ~CPPgStats();

	// Dialog Data
	enum { IDD = IDD_PPG_STATS };

	void Localize(void);

protected:
	CComboBox m_colors, m_cratio;
	CColorButton m_ctlColor;
	int mystats1, mystats2, mystats3;
	BOOL m_bModified;

	void ShowInterval();
	void SetModified(BOOL bChanged = TRUE);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnCbnSelchangeColorselector();
	afx_msg LONG OnColorPopupSelChange(UINT lParam, LONG wParam);
	afx_msg void OnEnChangeCGraphScale() { SetModified(); }
	afx_msg void OnCbnSelchangeCRatio()	{ SetModified(); }
	afx_msg void OnHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
};
