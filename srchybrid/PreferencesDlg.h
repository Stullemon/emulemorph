#pragma once

#include "PPgGeneral.h"
#include "PPgConnection.h"
#include "PPgServer.h"
#include "PPgDirectories.h"
#include "PPgFiles.h"
#include "PPgStats.h"
#include "PPgNotify.h"
#include "PPgIRC.h"
#include "PPgTweaks.h"
#include "PPgDisplay.h"
#include "PPgSecurity.h"
#include "PPgWebServer.h"
#include "PPgScheduler.h"
#include "PPgProxy.h"
#include "otherfunctions.h"
#include "ListBoxST.h"
#include "PPgMorph.h" //MORPH - Added by IceCream, Morph Prefs
#include "PPgMorph2.h" //MORPH - Added by SiRoB, Morph Prefs
#include "PPgBackup.h" //EastShare - Added by Pretender, TBH-AutoBackup
#include "PPgEastShare.h" //EastShare - Added by Pretender, TBH-AutoBackup

// CPreferencesDlg

class CPreferencesDlg : public CPropertySheet
{
	DECLARE_DYNAMIC(CPreferencesDlg)

public:
	CPreferencesDlg();
	virtual ~CPreferencesDlg();
	
	CPPgGeneral		m_wndGeneral;
	CPPgConnection	m_wndConnection;
	CPPgServer		m_wndServer;
	CPPgDirectories	m_wndDirectories;
	CPPgFiles		m_wndFiles;
	CPPgStats		m_wndStats;
	CPPgNotify		m_wndNotify;
	CPPgIRC			m_wndIRC;
	CPPgTweaks		m_wndTweaks;
	CPPgDisplay		m_wndDisplay;
	CPPgSecurity	m_wndSecurity;
	CPPgWebServer	m_wndWebServer;
	CPPgScheduler	m_wndScheduler;
	CPPgProxy		m_wndProxy;
	CPPgMorph		m_wndMorph; //MORPH - Added by IceCream, Morph Prefs
	CPPgMorph2		m_wndMorph2; //MORPH - Added by SiRoB, Morph Prefs
	CPPgBackup		m_wndBackup; //EastShare - Added by Pretender, TBH-AutoBackup
	CPPgEastShare	m_wndEastShare; //EastShare - Added by Pretender, ES Prefs

	CPreferences	*app_prefs;
	CListBoxST		m_listbox;
	CButton			m_groupbox;
	CImageList		ImageList;
	int				m_iPrevPage;

	void Localize();
	void SetPrefs(CPreferences* in_prefs)
	{
		app_prefs = in_prefs;
		m_wndGeneral.SetPrefs(in_prefs);
		m_wndDisplay.SetPrefs(in_prefs);
		m_wndConnection.SetPrefs(in_prefs);
		m_wndServer.SetPrefs(in_prefs);
		m_wndDirectories.SetPrefs(in_prefs);
		m_wndFiles.SetPrefs(in_prefs);
		m_wndStats.SetPrefs(in_prefs);
		m_wndNotify.SetPrefs(in_prefs);
		m_wndIRC.SetPrefs(in_prefs);
		m_wndTweaks.SetPrefs(in_prefs);
		m_wndSecurity.SetPrefs(in_prefs);
		m_wndWebServer.SetPrefs(in_prefs);
		m_wndScheduler.SetPrefs(in_prefs);
		m_wndProxy.SetPrefs(in_prefs);
		m_wndMorph.SetPrefs(in_prefs); //MORPH - Added by IceCream, Morph Prefs
		m_wndMorph2.SetPrefs(in_prefs);	//MORPH - Added by SiRoB, Morph Prefs
		m_wndEastShare.SetPrefs(in_prefs);	//EastShare - Added by Pretender, ES Prefs
	}

protected:
	UINT m_nActiveWnd;

	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnSelChanged();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};
