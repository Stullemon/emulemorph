#pragma once

#include "preferences.h"
#include "afxwin.h"
#include "loggable.h"
// CPPgMorph dialog

class CPPgMorph2 : public CPropertyPage, public CLoggable
{
	DECLARE_DYNAMIC(CPPgMorph2)

public:
	CPPgMorph2();
	virtual ~CPPgMorph2();

	void SetPrefs(CPreferences* in_prefs) {	app_prefs = in_prefs;}

// Dialog Data
	enum { IDD = IDD_PPG_MORPH2 };
protected:
	CPreferences *app_prefs;


	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void LoadSettings(void);
	virtual BOOL OnApply();


	void Localize(void);

private:

public:
	afx_msg void OnBnClickedUpdatefakes();//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	afx_msg void OnBnClickedUpdateipfurl();//MORPH START added by Yun.SF3: Ipfilter.dat update
};
