#pragma once

// CPPgFiles dialog

#include "preferences.h"

class CPPgFiles : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgFiles)

public:
	CPPgFiles();
	virtual ~CPPgFiles();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }

// Dialog Data
	enum { IDD = IDD_PPG_FILES };
protected:
	CPreferences *app_prefs;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
private:
	void LoadSettings(void);
	CListBox m_uncfolders;
public:
	virtual BOOL OnApply();
	afx_msg void OnSetCleanupFilter();
	afx_msg void BrowseVideoplayer();
	afx_msg void OnSettingsChange() {
		SetModified();
		GetDlgItem(IDC_STARTNEXTFILECAT)->EnableWindow(IsDlgButtonChecked(IDC_STARTNEXTFILE));
	}
	void Localize(void);
};
