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
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
#include "PPgDebug.h"
#endif
#include "otherfunctions.h"
#include "TreePropSheet.h"
#include "PPgMorph.h" //MORPH - Added by IceCream, Morph Prefs
#include "PPgMorphShare.h" //MORPH - Added by SiRoB, Morph Prefs
#include "PPgMorph2.h" //MORPH - Added by SiRoB, Morph Prefs
//#include "PPgMorph3.h" //Commander - Added: Morph III
#include "PPgBackup.h" //EastShare - Added by Pretender, TBH-AutoBackup
#include "PPgEastShare.h" //EastShare - Added by Pretender
#include "PPgEmulEspana.h" //MORPH - Added by SiRoB, eMulEspana Preferency
#include "WebCache\PPgWebcachesettings.h" //MORPH - Added by SiRoB, WebCache 1.2f
#include "KCSideBannerWnd.h" //Commander - Added: Preferences Banner [TPT]
#include "SlideBar.h" //MORPH - Added by SiRoB, ePLus Group

class CPreferencesDlg : public CTreePropSheet
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
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	CPPgDebug		m_wndDebug;
#endif
	CPPgMorph		m_wndMorph; //MORPH - Added by IceCream, Morph Prefs
	CPPgMorphShare	m_wndMorphShare; //MORPH - Added by SiRoB, Morph Prefs
	CPPgMorph2		m_wndMorph2; //MORPH - Added by SiRoB, Morph Prefs
	CPPgBackup		m_wndBackup; //EastShare - Added by Pretender, TBH-AutoBackup
	CPPgEastShare	m_wndEastShare; //EastShare - Added by Pretender, ES Prefs
	CPPgEmulespana	m_wndEmulespana; //MORPH - Added by SiRoB, emulEspaña preferency
	CPPgWebcachesettings	m_wndWebcachesettings; //MORPH - Added by SiRoB, WebCache 1.2f
	
	void Localize();
	void SetStartPage(UINT uStartPageID);

protected:
	LPCTSTR m_pPshStartPage;
	bool m_bSaveIniFile;

	virtual BOOL OnInitDialog();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg void OnHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
public:
	CKCSideBannerWnd m_banner;	//Commander - Added: Preferences Banner [TPT]	
};
