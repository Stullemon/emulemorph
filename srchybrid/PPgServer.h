#pragma once

class CPPgServer : public CPropertyPage
{
	DECLARE_DYNAMIC(CPPgServer)

public:
	CPPgServer();
	virtual ~CPPgServer();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs; }

// Dialog Data
	enum { IDD = IDD_PPG_SERVER };
protected:
	CPreferences *app_prefs;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCheck1();
	virtual BOOL OnInitDialog();
private:
	void LoadSettings(void);
public:
	virtual BOOL OnApply();

	afx_msg void OnSettingsChange()					{ SetModified(); }
	void Localize(void);
	afx_msg void OnBnClickedEditadr();
};
