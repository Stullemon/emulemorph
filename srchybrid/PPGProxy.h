#pragma once
#include "Preferences.h"

class CPPgProxy : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgProxy)

public:
	CPPgProxy();
	virtual ~CPPgProxy();

	// Dialog Data
	enum { IDD = IDD_PPG_PROXY };

	void Localize(void);

protected:
	ProxySettings proxy;

	void LoadSettings();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedEnableproxy();
	afx_msg void OnBnClickedEnableauth();
	afx_msg void OnCbnSelchangeProxytype();
	afx_msg void OnEnChangeProxyname(){SetModified(true);}
	afx_msg void OnEnChangeProxyport(){SetModified(true);}
	afx_msg void OnEnChangeUsername(){SetModified(true);}
	afx_msg void OnEnChangePassword(){SetModified(true);}
	afx_msg void OnBnClickedAscwop();
	afx_msg void OnHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
};
