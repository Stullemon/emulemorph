#pragma once

class CPPgGeneral : public CPropertyPage, CLoggable
{
	DECLARE_DYNAMIC(CPPgGeneral)

public:
	CPPgGeneral();
	virtual ~CPPgGeneral();

// Dialog Data
	enum { IDD = IDD_PPG_GENERAL };
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
	afx_msg void OnSettingsChange()					{ SetModified(); }
	afx_msg void OnBnClickedEd2kfix();
	afx_msg void OnBnClickedEditWebservices();
	afx_msg void OnLangChange();
	afx_msg void OnBnClickedCheck4Update();

protected:
	CComboBox m_language;
public:
	void Localize(void);
	afx_msg void OnCbnCloseupLangs();
};

