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
#include "ListBoxST.h"
#include "PPgMorph.h" //MORPH - Added by IceCream, Morph Prefs
#include "PPgMorph2.h" //MORPH - Added by SiRoB, Morph Prefs
#include "PPgBackup.h" //EastShare - Added by Pretender, TBH-AutoBackup
#include "PPgEastShare.h" //EastShare - Added by Pretender, TBH-AutoBackup
#include "KCSideBannerWnd.h" //Commander - Added: Preferences Banner [TPT]
#include "SlideBar.h" //MORPH - Added by SiRoB, ePLus Group

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
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	CPPgDebug		m_wndDebug;
#endif
	CPPgMorph		m_wndMorph; //MORPH - Added by IceCream, Morph Prefs
	CPPgMorph2		m_wndMorph2; //MORPH - Added by SiRoB, Morph Prefs
	CPPgBackup		m_wndBackup; //EastShare - Added by Pretender, TBH-AutoBackup
	CPPgEastShare	m_wndEastShare; //EastShare - Added by Pretender, ES Prefs

	CListBoxST		m_listbox;
	CButton			m_groupbox;
	CImageList		ImageList;
	int				m_iPrevPage;
	//MORPH START - Changed by SiRoB, ePlus Group
	CSlideBar	 	m_slideBar;
	//MORPH END   - Changed by SiRoB, ePlus Group
	
	void Localize();
	void OpenPage(UINT uResourceID);

protected:
	UINT m_nActiveWnd;

	virtual BOOL OnInitDialog();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	//MORPH START - Changed by SiRoB, ePlus Group
	/*
	afx_msg void OnSelChanged();
	*/
	afx_msg LRESULT		OnSlideBarSelChanged(WPARAM wParam, LPARAM lParam);
	//MORPH END   - Changed by SiRoB, ePlus Group

	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnHelp();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
public:
	CKCSideBannerWnd m_banner;	//Commander - Added: Preferences Banner [TPT]	


};
