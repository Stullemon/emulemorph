#pragma once

class Wizard : public CDialog
{
	DECLARE_DYNAMIC(Wizard)

public:
	Wizard(CWnd* pParent = NULL);   // standard constructor
	virtual ~Wizard();
	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }
	void Localize();
	virtual BOOL OnInitDialog();

	int m_iOS;
	int m_iTotalDownload;
	int m_iBitByte;

// Dialog Data
	enum { IDD = IDD_WIZARD };
protected:
	CPreferences* app_prefs;
	void SetCustomItemsActivation();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedApply();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedWizRadioOsNtxp();
	afx_msg void OnBnClickedWizRadioUs98me();
	afx_msg void OnBnClickedWizLowdownloadRadio();
	afx_msg void OnBnClickedWizMediumdownloadRadio();
	afx_msg void OnBnClickedWizHighdownloadRadio();
	afx_msg void OnBnClickedWizResetButton();
	afx_msg void OnNMClickProviders(NMHDR *pNMHDR, LRESULT *pResult);
	CListCtrl m_provider;
};
