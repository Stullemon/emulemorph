//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
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
	m_wndMorph.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by IceCream, Morph Prefs
	m_wndMorphShare.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, Morph Prefs
	m_wndMorph2.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, Morph Prefs
//	m_wndMorph3.m_psp.dwFlags &= ~PSH_HASHELP; //Commander - Added: Morph III
	m_wndBackup.m_psp.dwFlags &= ~PSH_HASHELP; //EastShare - Added by Pretender, TBH-AutoBackup
	m_wndEastShare.m_psp.dwFlags &= ~PSH_HASHELP; //EastShare - Added by Pretender, ES Prefs
	m_wndEmulespana.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, emulEspaña preferency
	m_wndWebcachesettings.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, WebCache 1.2f
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	m_wndDebug.m_psp.dwFlags &= ~PSH_HASHELP;
#endif

	CTreePropSheet::SetPageIcon(&m_wndGeneral, _T("Preferences"));
	CTreePropSheet::SetPageIcon(&m_wndDisplay, _T("DISPLAY"));
	CTreePropSheet::SetPageIcon(&m_wndConnection, _T("CONNECTION"));
	CTreePropSheet::SetPageIcon(&m_wndProxy, _T("PROXY"));
	CTreePropSheet::SetPageIcon(&m_wndServer, _T("SERVER"));
	CTreePropSheet::SetPageIcon(&m_wndDirectories, _T("FOLDERS"));
	CTreePropSheet::SetPageIcon(&m_wndFiles, _T("SharedFiles"));
	CTreePropSheet::SetPageIcon(&m_wndNotify, _T("NOTIFICATIONS"));
	CTreePropSheet::SetPageIcon(&m_wndStats, _T("STATISTICS"));
	CTreePropSheet::SetPageIcon(&m_wndIRC, _T("IRC"));
	CTreePropSheet::SetPageIcon(&m_wndSecurity, _T("SECURITY"));
	CTreePropSheet::SetPageIcon(&m_wndScheduler, _T("SCHEDULER"));
	CTreePropSheet::SetPageIcon(&m_wndWebServer, _T("WEB"));
	CTreePropSheet::SetPageIcon(&m_wndTweaks, _T("TWEAK"));
	CTreePropSheet::SetPageIcon(&m_wndBackup, _T("BACKUP")); //EastShare - Added by Pretender, TBH-AutoBackup
	CTreePropSheet::SetPageIcon(&m_wndMorph, _T("MORPH")); //MORPH - Added by IceCream, Morph Prefs
	CTreePropSheet::SetPageIcon(&m_wndMorphShare, _T("MORPH")); //MORPH - Added by SiRoB, Morph Prefs
	CTreePropSheet::SetPageIcon(&m_wndMorph2, _T("MORPH")); //MORPH - Added by SiRoB, Morph Prefs
//	CTreePropSheet::SetPageIcon(&m_wndMorph3, _T("MORPH")); //Commander - Added: Morph III
	CTreePropSheet::SetPageIcon(&m_wndEastShare, _T("EASTSHARE")); //EastShare - Added by Pretender, ES Prefs
	CTreePropSheet::SetPageIcon(&m_wndEmulespana, _T("EMULESPANA")); //MORPH - Added by SiRoB, emulEspaña preferency
	CTreePropSheet::SetPageIcon(&m_wndWebcachesettings, _T("WEBCACHE")); //MORPH - Added by SiRoB, WebCache 1.2f
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	CTreePropSheet::SetPageIcon(&m_wndDebug, _T("Preferences"));
#endif
	
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
	AddPage(&m_wndSecurity);
	AddPage(&m_wndScheduler);
	AddPage(&m_wndWebServer);
	AddPage(&m_wndTweaks);
	AddPage(&m_wndBackup); //EastShare - Added by Pretender, TBH-AutoBackup
	AddPage(&m_wndMorph); //MORPH - Added by IceCream, Morph Prefs
	AddPage(&m_wndMorphShare); //MORPH - Added by SiRoB, Morph Prefs
	AddPage(&m_wndMorph2); //MORPH - Added by SiRoB, Morph Prefs
//	AddPage(&m_wndMorph3); //Commander - Added: Morph III
	AddPage(&m_wndEastShare); //EastShare - Added by Pretender, ES Prefs
	AddPage(&m_wndEmulespana); //MORPH - Added by SiRoB, emulEspaña preferency
	AddPage(&m_wndWebcachesettings); //MORPH - Added by SiRoB, WebCache 1.2f
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	AddPage(&m_wndDebug);
#endif
	SetTreeViewMode(TRUE, TRUE, TRUE);
	SetTreeWidth(170);

	m_nActiveWnd = 0;
	m_iPrevPage = -1;
}

CPreferencesDlg::~CPreferencesDlg()
{
}

void CPreferencesDlg::OnDestroy()
{
	CTreePropSheet::OnDestroy();
	thePrefs.Save();
	m_nActiveWnd = GetActiveIndex();
}

BOOL CPreferencesDlg::OnInitDialog()
{		
	BOOL bResult = CTreePropSheet::OnInitDialog();
	InitWindowStyles(this);
	SetActivePage(m_nActiveWnd);
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

	m_iPrevPage = GetActiveIndex();
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
	m_wndMorph.Localize();//MORPH - Added by IceCream, Morph Prefs
	m_wndMorphShare.Localize();//MORPH - Added by SiRoB, Morph Prefs
	m_wndMorph2.Localize();//MORPH - Added by SiRoB, Morph Prefs
//	m_wndMorph3.Localize(); //Commander - Added: Morph III
	m_wndEastShare.Localize();
	m_wndEmulespana.Localize(); //MORPH - Added by SiRoB, emulEspaña preferency
	m_wndWebcachesettings.Localize(); //MORPH - Added by SiRoB, WebCache 1.2f

	CTreeCtrl* pTree = GetPageTreeControl();
	if (pTree)
	{
		pTree->SetItemText(GetPageTreeItem(0), RemoveAmbersand(GetResString(IDS_PW_GENERAL)));
		pTree->SetItemText(GetPageTreeItem(1), RemoveAmbersand(GetResString(IDS_PW_DISPLAY))); 
		pTree->SetItemText(GetPageTreeItem(2), RemoveAmbersand(GetResString(IDS_PW_CONNECTION))); 
		pTree->SetItemText(GetPageTreeItem(3), RemoveAmbersand(GetResString(IDS_PW_PROXY))); 
		pTree->SetItemText(GetPageTreeItem(4), RemoveAmbersand(GetResString(IDS_PW_SERVER))); 
		pTree->SetItemText(GetPageTreeItem(5), RemoveAmbersand(GetResString(IDS_PW_DIR))); 
		pTree->SetItemText(GetPageTreeItem(6), RemoveAmbersand(GetResString(IDS_PW_FILES))); 
		pTree->SetItemText(GetPageTreeItem(7), RemoveAmbersand(GetResString(IDS_PW_EKDEV_OPTIONS))); 
		pTree->SetItemText(GetPageTreeItem(8), RemoveAmbersand(GetResString(IDS_STATSSETUPINFO))); 
		pTree->SetItemText(GetPageTreeItem(9), RemoveAmbersand(GetResString(IDS_IRC)));
		pTree->SetItemText(GetPageTreeItem(10), RemoveAmbersand(GetResString(IDS_SECURITY))); 
		pTree->SetItemText(GetPageTreeItem(11), RemoveAmbersand(GetResString(IDS_SCHEDULER)));
		pTree->SetItemText(GetPageTreeItem(12), RemoveAmbersand(GetResString(IDS_PW_WS)));
		pTree->SetItemText(GetPageTreeItem(13), RemoveAmbersand(GetResString(IDS_PW_TWEAK)));
		//	MOD group
		pTree->SetItemText(GetPageTreeItem(14), RemoveAmbersand(GetResString(IDS_BACKUP)));
		pTree->SetItemText(GetPageTreeItem(15), RemoveAmbersand(_T("Morph")));
		pTree->SetItemText(GetPageTreeItem(16), RemoveAmbersand(_T("Morph Share")));
		pTree->SetItemText(GetPageTreeItem(17), RemoveAmbersand(_T("Morph Update")));
		//pTree->SetItemText(GetPageTreeItem(??), RemoveAmbersand(_T("Morph DynDNS")));
		pTree->SetItemText(GetPageTreeItem(18), RemoveAmbersand(_T("EastShare")));
		pTree->SetItemText(GetPageTreeItem(19), RemoveAmbersand(_T("emulEspaña")));
		pTree->SetItemText(GetPageTreeItem(20), RemoveAmbersand(GetResString(IDS_PW_WEBCACHE)));  //MORPH - Added by SiRoB, WebCache 1.2f
	#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
		pTree->SetItemText(GetPageTreeItem(21), _T("Debug"));
	#endif
	}

	m_banner.UpdateSize(); //Commander - Added: Preferences Banner [TPT]	
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
	return __super::OnCommand(wParam, lParam);
}

BOOL CPreferencesDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}
