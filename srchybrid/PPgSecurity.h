#pragma once

#include "preferences.h"
#include "afxwin.h"
#include "loggable.h"

// CPPgDisplay dialog

class CPPgSecurity : public CPropertyPage, public CLoggable
{
	DECLARE_DYNAMIC(CPPgSecurity)

public:
	CPPgSecurity();
	virtual ~CPPgSecurity();
	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }

// Dialog Data
	enum { IDD = IDD_PPG_SECURITY };
protected:
	CPreferences *app_prefs;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	void LoadSettings(void);
public:
	virtual BOOL OnApply();
	void Localize(void);
	afx_msg void OnSettingsChange()					{ SetModified(); }
	afx_msg void OnReloadIPFilter();
	afx_msg void OnEditIPFilter();
};

