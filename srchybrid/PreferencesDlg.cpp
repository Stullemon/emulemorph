//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "emule.h"
#include "PreferencesDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


IMPLEMENT_DYNAMIC(CPreferencesDlg, CTreePropSheet)

BEGIN_MESSAGE_MAP(CPreferencesDlg, CTreePropSheet)
	ON_WM_DESTROY()
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

CPreferencesDlg::CPreferencesDlg()
{
	m_psh.dwFlags &= ~PSH_HASHELP;
	m_wndGeneral.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndDisplay.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndConnection.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndServer.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndDirectories.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndFiles.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndStats.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndIRC.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndWebServer.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndTweaks.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndSecurity.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndScheduler.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndProxy.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndMessages.m_psp.dwFlags &= ~PSH_HASHELP;
	m_wndMorph.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by IceCream, Morph Prefs
	m_wndMorphShare.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, Morph Prefs
	m_wndMorph2.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, Morph Prefs
	m_wndBackup.m_psp.dwFlags &= ~PSH_HASHELP; //EastShare - Added by Pretender, TBH-AutoBackup
	m_wndEastShare.m_psp.dwFlags &= ~PSH_HASHELP; //EastShare - Added by Pretender, ES Prefs
	m_wndEmulespana.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, emulEspaña preferency
	m_wndWebcachesettings.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, WebCache 1.2f
    //m_wndIonixWebServer.m_psp.dwFlags &= ~PSH_HASHELP;  //ionix advanced webserver
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	m_wndDebug.m_psp.dwFlags &= ~PSH_HASHELP;
#endif

	CTreePropSheet::SetPageIcon(&m_wndGeneral, _T("Preferences"));
	CTreePropSheet::SetPageIcon(&m_wndDisplay, _T("DISPLAY"));
	CTreePropSheet::SetPageIcon(&m_wndConnection, _T("CONNECTION"));
	CTreePropSheet::SetPageIcon(&m_wndProxy, _T("PROXY"));
	CTreePropSheet::SetPageIcon(&m_wndServer, _T("SERVER"));
	CTreePropSheet::SetPageIcon(&m_wndDirectories, _T("FOLDERS"));
	CTreePropSheet::SetPageIcon(&m_wndFiles, _T("Transfer"));
	CTreePropSheet::SetPageIcon(&m_wndNotify, _T("NOTIFICATIONS"));
	CTreePropSheet::SetPageIcon(&m_wndStats, _T("STATISTICS"));
	CTreePropSheet::SetPageIcon(&m_wndIRC, _T("IRC"));
	CTreePropSheet::SetPageIcon(&m_wndSecurity, _T("SECURITY"));
	CTreePropSheet::SetPageIcon(&m_wndScheduler, _T("SCHEDULER"));
	CTreePropSheet::SetPageIcon(&m_wndWebServer, _T("WEB"));
	CTreePropSheet::SetPageIcon(&m_wndTweaks, _T("TWEAK"));
	CTreePropSheet::SetPageIcon(&m_wndMessages, _T("MESSAGES"));
	CTreePropSheet::SetPageIcon(&m_wndBackup, _T("BACKUP")); //EastShare - Added by Pretender, TBH-AutoBackup
	CTreePropSheet::SetPageIcon(&m_wndMorph, _T("MORPH")); //MORPH - Added by IceCream, Morph Prefs
	CTreePropSheet::SetPageIcon(&m_wndMorphShare, _T("MORPH")); //MORPH - Added by SiRoB, Morph Prefs
	CTreePropSheet::SetPageIcon(&m_wndMorph2, _T("MORPH")); //MORPH - Added by SiRoB, Morph Prefs
	CTreePropSheet::SetPageIcon(&m_wndEastShare, _T("EASTSHARE")); //EastShare - Added by Pretender, ES Prefs
	CTreePropSheet::SetPageIcon(&m_wndEmulespana, _T("EMULESPANA")); //MORPH - Added by SiRoB, emulEspaña preferency
	CTreePropSheet::SetPageIcon(&m_wndWebcachesettings, _T("WEBCACHE")); //MORPH - Added by SiRoB, WebCache 1.2f
	#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	CTreePropSheet::SetPageIcon(&m_wndDebug, _T("Preferences"));
    #endif
    //CTreePropSheet::SetPageIcon(&m_wndIonixWebServer, _T("MORPH"));
	
	AddPage(&m_wndGeneral);
	AddPage(&m_wndDisplay);
	AddPage(&m_wndConnection);
	AddPage(&m_wndProxy);
	AddPage(&m_wndServer);
	AddPage(&m_wndDirectories);
	AddPage(&m_wndFiles);
	AddPage(&m_wndNotify);
	AddPage(&m_wndStats);
	AddPage(&m_wndIRC);
	AddPage(&m_wndMessages);
	AddPage(&m_wndSecurity);
	AddPage(&m_wndScheduler);
	AddPage(&m_wndWebServer);
	AddPage(&m_wndTweaks);
	AddPage(&m_wndBackup); //EastShare - Added by Pretender, TBH-AutoBackup
	AddPage(&m_wndMorph); //MORPH - Added by IceCream, Morph Prefs
	AddPage(&m_wndMorphShare); //MORPH - Added by SiRoB, Morph Prefs
	AddPage(&m_wndMorph2); //MORPH - Added by SiRoB, Morph Prefs
	AddPage(&m_wndEastShare); //EastShare - Added by Pretender, ES Prefs
	AddPage(&m_wndEmulespana); //MORPH - Added by SiRoB, emulEspaña preferency
	AddPage(&m_wndWebcachesettings); //MORPH - Added by SiRoB, WebCache 1.2f
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	AddPage(&m_wndDebug);
#endif
     AddPage(&m_wndIonixWebServer); // Morph - ionix advanced webserver
	 AddPage(&m_wndNTService); // MORPH leuk_he:run as ntservice v1..


	SetTreeViewMode(TRUE, TRUE, TRUE);
	SetTreeWidth(170);

	m_pPshStartPage = NULL;
	m_bSaveIniFile = false;
	// MORPH start tabbed options [leuk_he]
	ActivePageWebServer			= 0;
	StartPageWebServer			= 0;
	NTService                   = 0; //MORPH leuk_he:run as ntservice v1..
	// end tabbed

}

CPreferencesDlg::~CPreferencesDlg()
{
}

void CPreferencesDlg::OnDestroy()
{
	CTreePropSheet::OnDestroy();
	if (m_bSaveIniFile)
	{
		thePrefs.Save();
		m_bSaveIniFile = false;
	}
	m_pPshStartPage = GetPage(GetActiveIndex())->m_psp.pszTemplate;
}

BOOL CPreferencesDlg::OnInitDialog()
{		
	ASSERT( !m_bSaveIniFile );
	BOOL bResult = CTreePropSheet::OnInitDialog();
	InitWindowStyles(this);

	for (int i = 0; i < m_pages.GetSize(); i++)
	{
		if (GetPage(i)->m_psp.pszTemplate == m_pPshStartPage)
		{
			// MORPH start tabbed options [leuk_he]
			if ((i==Multiwebserver)||(i==NTService) ) //MORPH leuk_he:run as ntservice v1..
			{
					SetActivePage(Webserver);
				m_wndWebServer.InitTab(false,StartPageWebServer);
				m_wndIonixWebServer.InitTab(false,StartPageWebServer);
			    m_wndNTService.InitTab(false,StartPageWebServer);//MORPH leuk_he:run as ntservice v1..
				break;
			}
			else
			{
				SetActivePage(i);
				break;
			}
			/*   commented out morph :
			SetActivePage(i);
			break;*/
		  // MORPH end tabbed options [leuk_he]
		}
		
			
	}

	Localize();	

	//MORPH START - Added by SiRoB, Load a jpg
	CBitmap bmp;
	VERIFY( bmp.Attach(theApp.LoadImage(_T("BANNER"), _T("JPG"))) );
	if (bmp.GetSafeHandle())
	{
	//MORPH END   - Added by SiRoB, Load a jpg
		//Commander - Added: Preferences Banner [TPT] - Start			
		m_banner.SetTexture((HBITMAP)bmp.Detach());	
		m_banner.SetFillFlag(KCSB_FILL_TEXTURE);
		m_banner.SetSize(thePrefs.sidebanner?70:0);
		m_banner.SetTitle(_T(""));
		m_banner.SetCaption(_T(""));
		m_banner.Attach(this, KCSB_ATTACH_RIGHT);
		//Commander - Added: Preferences Banner [TPT] - End
	}
	return bResult;
}

void CPreferencesDlg::Localize()
{
	SetTitle(RemoveAmbersand(GetResString(IDS_EM_PREFS))); 

	m_wndGeneral.Localize();
	m_wndDisplay.Localize();
	m_wndConnection.Localize();
	m_wndServer.Localize();
	m_wndDirectories.Localize();
	m_wndFiles.Localize();
	m_wndStats.Localize();
	m_wndNotify.Localize();
	m_wndIRC.Localize();
	m_wndSecurity.Localize();
	m_wndTweaks.Localize();
	m_wndWebServer.Localize();
	m_wndScheduler.Localize();
	m_wndProxy.Localize();
	m_wndMessages.Localize();
	m_wndMorph.Localize();//MORPH - Added by IceCream, Morph Prefs
	m_wndMorphShare.Localize();//MORPH - Added by SiRoB, Morph Prefs
	m_wndMorph2.Localize();//MORPH - Added by SiRoB, Morph Prefs
	m_wndEastShare.Localize();
	m_wndEmulespana.Localize(); //MORPH - Added by SiRoB, emulEspaña preferency
	m_wndWebcachesettings.Localize(); //MORPH - Added by SiRoB, WebCache 1.2f
    m_wndIonixWebServer.Localize(); //MORPH ionix advanced webserver
	m_wndNTService.Localize(); //MORPH leuk_he:run as ntservice v1..
	int c = 0;

	CTreeCtrl* pTree = GetPageTreeControl();
	if (pTree)
	{
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_PW_GENERAL)));
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_PW_DISPLAY))); 
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_PW_CONNECTION))); 
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_PW_PROXY))); 
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_PW_SERVER))); 
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_PW_DIR))); 
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_PW_FILES))); 
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_PW_EKDEV_OPTIONS))); 
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_STATSSETUPINFO))); 
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_IRC)));
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_MESSAGESCOMMENTS)));
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_SECURITY))); 
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_SCHEDULER)));
		pTree->SetItemText(GetPageTreeItem(Webserver=c++), RemoveAmbersand(GetResString(IDS_PW_WS)));
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_PW_TWEAK)));
		//	MOD group
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_BACKUP)));
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(_T("Morph")));
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(_T("Morph Share")));
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(_T("Morph Update")));
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(_T("EastShare")));
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(_T("emulEspaña")));
		pTree->SetItemText(GetPageTreeItem(c++), RemoveAmbersand(GetResString(IDS_PW_WEBCACHE)));  //MORPH - Added by SiRoB, WebCache 1.2f
    	#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
		pTree->SetItemText(GetPageTreeItem(c++), _T("Debug"));
	     #endif
		pTree->SetItemText(GetPageTreeItem(Multiwebserver=c++), RemoveAmbersand(_T(" ")));	// MORPH ionix advanced webserver must be last!
		pTree->SetItemText(GetPageTreeItem(NTService=c++), RemoveAmbersand(_T(" "))); // MORPH leuk_he:run as ntservice v1..
    //  
	
	}
	m_banner.UpdateSize(); //Commander - Added: Preferences Banner [TPT]	
	//MORPH START - Added by SiRoB, Adjust tabHeigh
	CRect rectTab,rectClient;
	pTree->GetWindowRect(rectTab);
	GetClientRect(rectClient);
	pTree->SetWindowPos(NULL,-1,-1,rectTab.Width(),rectClient.Height()-13,SWP_NOZORDER | SWP_NOMOVE);
	//MORPH END   - Added by SiRoB, Adjust tabHeigh
	UpdateCaption();
}

void CPreferencesDlg::OnHelp()
{
	int iCurSel = GetActiveIndex();
	if (iCurSel >= 0)
	{
		CPropertyPage* pPage = GetPage(iCurSel);
		if (pPage)
		{
			HELPINFO hi = {0};
			hi.cbSize = sizeof hi;
			hi.iContextType = HELPINFO_WINDOW;
			hi.iCtrlId = 0;
			hi.hItemHandle = pPage->m_hWnd;
			hi.dwContextId = 0;
			pPage->SendMessage(WM_HELP, 0, (LPARAM)&hi);
			return;
		}
	}

	theApp.ShowHelp(0, HELP_CONTENTS);
}

BOOL CPreferencesDlg::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	if (wParam == IDOK || wParam == ID_APPLY_NOW)
		m_bSaveIniFile = true;
	return __super::OnCommand(wParam, lParam);
}

BOOL CPreferencesDlg::OnHelpInfo(HELPINFO* /*pHelpInfo*/)
{
	OnHelp();
	return TRUE;
}

void CPreferencesDlg::SetStartPage(UINT uStartPageID)
{
	m_pPshStartPage = MAKEINTRESOURCE(uStartPageID);
}


// MORPH start tabbed option [leuk_he]
void CPreferencesDlg::SwitchTab(int Page)
{
	
	if(m_hWnd && IsWindowVisible()){
		CPropertyPage* activepage = GetActivePage();
								   
		// webServer 1-2 -3
		if (activepage == &m_wndWebServer || activepage == &m_wndIonixWebServer || activepage == &m_wndNTService){
			if (Page == 0) {
				SetActivePage(&m_wndWebServer);
				ActivePageWebServer = 0;
				StartPageWebServer = 0;
				m_wndWebServer.InitTab(false,0);
			}
			if (Page == 1) {
				SetActivePage(&m_wndIonixWebServer);
				ActivePageWebServer = Multiwebserver;
				StartPageWebServer = 1;
				m_wndIonixWebServer.InitTab(false,1);
			}			
			// MORPH leuk_he:run as ntservice v1..
			if (Page == 2) {
				SetActivePage(&m_wndNTService);
				ActivePageWebServer = NTService;
				StartPageWebServer = 2;
				m_wndNTService.InitTab(false,2);
			}		
			// MORPH leuk_he:run as ntservice v1..
		}
	}
}
// MORPH end tabbed option [leuk_he]