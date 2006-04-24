/// jp webcachesettings GUI


#pragma once
#include "HypertextCtrl.h"

// MORPH START  leuk_he tooltipped
#include "PPGtooltipped.h" 

/*
class CPPgWebcachesettings : public CPropertyPage
*/
class CPPgWebcachesettings : public CPPgtooltipped  
// MORPH END leuk_he tooltipped
{
	DECLARE_DYNAMIC(CPPgWebcachesettings)

public:
	CPPgWebcachesettings();
	virtual ~CPPgWebcachesettings();

// Dialog Data
	enum { IDD = IDD_PPG_WEBCACHESETTINGS };

	void Localize(void);
	void LoadSettings(void);

protected:
	bool guardian;
	bool showadvanced;
	void ShowLimitValues();
	CHyperTextCtrl m_wndSubmitWebcacheLink, m_wndSubmitWebcacheLink2;
	bool bCreated, bCreated2;
	virtual BOOL OnApply();
	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnEnChangeActivatewebcachedownloads();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSettingsChange()					{ SetModified(); }
	afx_msg void OnBnClickedDetectWebCache();
	afx_msg void OnBnClickedAdvancedcontrols();
	afx_msg void OnBnClickedTestProxy(); //JP Proxy Configuration Test
};
