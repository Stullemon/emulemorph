#pragma once


// CPPgProxy dialog
#include "Preferences.h"

class CPPgProxy : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgProxy)

public:
	CPPgProxy();
	virtual ~CPPgProxy();

	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }

	// Dialog Data
	enum { IDD = IDD_PPG_PROXY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedEnableproxy();
	afx_msg void OnBnClickedEnableauth();
	afx_msg void OnCbnSelchangeProxytype();
	afx_msg void OnEnChangeProxyname(){SetModified(true);}
	afx_msg void OnEnChangeProxyport(){SetModified(true);}
	afx_msg void OnEnChangeUsername(){SetModified(true);}
	afx_msg void OnEnChangePassword(){SetModified(true);}
	afx_msg void OnBnClickedAscwop();
	void Localize(void);
private:
	CPreferences* app_prefs;
	ProxySettings proxy;
	void LoadSettings();
};
