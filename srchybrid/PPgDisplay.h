#pragma once

class CPPgDisplay : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgDisplay)

public:
	CPPgDisplay();
	virtual ~CPPgDisplay();

// Dialog Data
	enum { IDD = IDD_PPG_DISPLAY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	void LoadSettings(void);
public:
	virtual BOOL OnApply();
	void Localize(void);
	afx_msg void OnSettingsChange()					{ SetModified(); }
	afx_msg void OnBnClickedSelectHypertextFont();
};
