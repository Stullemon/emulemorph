// Apfelteig Patch :-)

#pragma once

#include "preferences.h"
#include "afxwin.h"
#include "HypertextCtrl.h"
#include "loggable.h"

// CPPgWebServer dialog

class CPPgWebServer : public CPropertyPage, public CLoggable
{

	DECLARE_DYNAMIC(CPPgWebServer)

public:
	CPPgWebServer();
	virtual ~CPPgWebServer();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }
	enum { IDD = IDD_PPG_WEBSRV };
	virtual BOOL OnInitDialog();
	virtual BOOL OnApply();
	
protected:
	CPreferences *app_prefs;
	
	bool m_bModified;
	void SetModified(BOOL bChanged = TRUE)
	{
		m_bModified = bChanged;
		CPropertyPage::SetModified(bChanged);
	}

	virtual void DoDataExchange(CDataExchange* pDX);
	DECLARE_MESSAGE_MAP()

private:
	void LoadSettings(void);
	CHyperTextCtrl	m_wndMobileLink;
	bool			bCreated;
	
public:
	void Localize(void);
	afx_msg void OnDataChange()		{SetModified();}
	afx_msg void OnEnChangeWSEnabled();
	afx_msg void OnEnChangeMMEnabled();
	afx_msg void OnReloadTemplates();
	afx_msg void OnBnClickedTmplbrowse();
};
