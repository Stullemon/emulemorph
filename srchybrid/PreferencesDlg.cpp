//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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


IMPLEMENT_DYNAMIC(CPreferencesDlg, CPropertySheet)

BEGIN_MESSAGE_MAP(CPreferencesDlg, CPropertySheet)
	ON_WM_DESTROY()
		ON_MESSAGE(WM_SBN_SELCHANGED, OnSlideBarSelChanged) //MORPH - Changed by SiRoB, ePlus Group
	ON_WM_CTLCOLOR()
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
	m_wndProxy.m_psp.dwFlags &= ~PSH_HASHELP; // deadlake PROXYSUPPORT
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	m_wndDebug.m_psp.dwFlags &= ~PSH_HASHELP;
#endif
	m_wndMorph.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by IceCream, Morph Prefs
	m_wndMorphShare.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, Morph Prefs
	m_wndMorph2.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, Morph Prefs
	m_wndMorph3.m_psp.dwFlags &= ~PSH_HASHELP; //Commander - Added: Morph III
	m_wndBackup.m_psp.dwFlags &= ~PSH_HASHELP; //EastShare - Added by Pretender, TBH-AutoBackup
	m_wndEastShare.m_psp.dwFlags &= ~PSH_HASHELP; //EastShare - Added by Pretender, ES Prefs
	m_wndEmulespana.m_psp.dwFlags &= ~PSH_HASHELP; //MORPH - Added by SiRoB, emulEspaña preferency
	
	//	WARNING: Pages must be added with the same order as the slidebar group items.
	//General group
	AddPage(&m_wndGeneral);
	AddPage(&m_wndDisplay);
	AddPage(&m_wndDirectories);
	AddPage(&m_wndFiles);
	AddPage(&m_wndStats);
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	AddPage(&m_wndDebug);
#endif

	//Connexion group
	AddPage(&m_wndConnection);
	AddPage(&m_wndProxy);
	AddPage(&m_wndServer);

	//Advanced Official group
	AddPage(&m_wndIRC);
	AddPage(&m_wndNotify);
	AddPage(&m_wndWebServer);
	AddPage(&m_wndSecurity);
	AddPage(&m_wndScheduler);
	AddPage(&m_wndTweaks);

	//MORPH group
	AddPage(&m_wndBackup); //EastShare - Added by Pretender, TBH-AutoBackup
	AddPage(&m_wndMorph); //MORPH - Added by IceCream, Morph Prefs
	AddPage(&m_wndMorphShare); //MORPH - Added by SiRoB, Morph Prefs
	AddPage(&m_wndMorph2); //MORPH - Added by SiRoB, Morph Prefs
	AddPage(&m_wndMorph3); //Commander - Added: Morph III
	AddPage(&m_wndEastShare); //EastShare - Added by Pretender, ES Prefs
	AddPage(&m_wndEmulespana); //MORPH - Added by SiRoB, emulEspaña preferency
	m_nActiveWnd = 0;
	m_iPrevPage = -1;
}

CPreferencesDlg::~CPreferencesDlg()
{
}

void CPreferencesDlg::OnDestroy()
{
	CPropertySheet::OnDestroy();
	thePrefs.Save();
	m_nActiveWnd = GetActiveIndex();
}

BOOL CPreferencesDlg::OnInitDialog()
{		
	EnableStackedTabs(FALSE);
	BOOL bResult = CPropertySheet::OnInitDialog();
	//MORPH START - Changed by SIRoB, ePlus Group
	/*
	m_listbox.CreateEx(WS_EX_CLIENTEDGE,_T("Listbox"),0,WS_CHILD|WS_VISIBLE|LBS_NOTIFY|WS_TABSTOP|LBS_HASSTRINGS|LBS_OWNERDRAWVARIABLE|WS_BORDER,CRect(0,0,0,0),this,111);
	::SendMessage(m_listbox.m_hWnd, WM_SETFONT, (WPARAM) ::GetStockObject(DEFAULT_GUI_FONT),0);
	m_groupbox.Create(0,BS_GROUPBOX|WS_CHILD|WS_VISIBLE|BS_FLAT,CRect(0,0,0,0),this,666);
	::SendMessage(m_groupbox.m_hWnd, WM_SETFONT, (WPARAM) ::GetStockObject(DEFAULT_GUI_FONT),0);
	*/
	m_slideBar.CreateEx(WS_EX_CLIENTEDGE, WS_CHILD | WS_VISIBLE | WS_BORDER, CRect(0, 0, 0, 0), this, 111);
	m_slideBar.SetImageList(&ImageList);
	m_slideBar.SetHAlignCaption(DT_CENTER);
	m_groupbox.Create(0,BS_GROUPBOX|WS_CHILD|WS_VISIBLE|BS_FLAT,CRect(0,0,0,0),this,666);
	::SendMessage(m_groupbox.m_hWnd, WM_SETFONT, (WPARAM) ::GetStockObject(DEFAULT_GUI_FONT),0);

	//sets a bold font for the group buttons
	CFont* pGroupFont = m_slideBar.GetGroupFont();
	ASSERT_VALID(pGroupFont);
	LOGFONT logFont;
	pGroupFont->GetLogFont(&logFont);
	logFont.lfWeight *= 2;
	if (logFont.lfWeight > FW_BLACK)
	{
		logFont.lfWeight = FW_BLACK;
	}
	pGroupFont->DeleteObject();
	pGroupFont->CreateFontIndirect(&logFont);
	ASSERT_VALID(pGroupFont);

	//MORPH END - Changed by SIRoB, ePlus Group

	InitWindowStyles(this);

	SetActivePage(m_nActiveWnd);
	Localize();	
	//MORPH START - Changed by SiRoB, ePlus Group
	/*
	m_listbox.SetFocus();
	CString currenttext;
	int curSel=m_listbox.GetCurSel();
	m_listbox.GetText(curSel,currenttext);
	m_groupbox.SetWindowText(currenttext);
	m_iPrevPage = curSel;
    */
	m_slideBar.SetFocus();
	//MORPH END - Changed by SiRoB, ePlus Group

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

//MORPH START - Changed by SIRoB, ePlus Group
/*
void CPreferencesDlg::OnSelChanged()
{
	int curSel=m_listbox.GetCurSel();
	if (!SetActivePage(curSel)){
		if (m_iPrevPage != -1){
			m_listbox.SetCurSel(m_iPrevPage);
			return;
		}
	}
	CString currenttext;
	m_listbox.GetText(curSel,currenttext);
	m_groupbox.SetWindowText(currenttext);
	m_listbox.SetFocus();
	m_iPrevPage = curSel;
}
*/
LRESULT CPreferencesDlg::OnSlideBarSelChanged(WPARAM wParam, LPARAM lParam)
{
	int iCurrentGlobalSel	= m_slideBar.GetGlobalSelectedItem();

	SetActivePage(iCurrentGlobalSel);

	CListBoxST* pListBox = m_slideBar.GetGroupListBox(m_slideBar.GetSelectedGroupIndex());
	ASSERT_VALID(pListBox);

	CString strCurrentItemText;
	pListBox->GetText(pListBox->GetCurSel(), strCurrentItemText);

	CString strCurrentGroupText = m_slideBar.GetGroupName(m_slideBar.GetSelectedGroupIndex());
	strCurrentGroupText.Remove('&');

	CString strTitle = GetResString(IDS_EM_PREFS);
	strTitle.Remove('&');
	SetWindowText(strTitle /*+ _T(" -> ") + strCurrentGroupText + _T(" -> ") + strCurrentItemText*/);

	m_groupbox.SetWindowText(strCurrentItemText);

	pListBox->SetFocus();

	return true;
}
//MORPH END   - Changed by SIRoB, ePlus Group

void CPreferencesDlg::Localize()
{
	ImageList.DeleteImageList();
	ImageList.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
	ImageList.Add(CTempIconLoader(_T("PREF_GENERAL")));			//0
	ImageList.Add(CTempIconLoader(_T("PREF_DISPLAY")));			//1
	ImageList.Add(CTempIconLoader(_T("PREF_CONNECTION")));		//2
	ImageList.Add(CTempIconLoader(_T("PREF_PROXY")));			//3
	ImageList.Add(CTempIconLoader(_T("PREF_SERVER")));			//4
	ImageList.Add(CTempIconLoader(_T("PREF_FOLDERS")));			//5
	ImageList.Add(CTempIconLoader(_T("PREF_FILES")));			//6
	ImageList.Add(CTempIconLoader(_T("PREF_NOTIFICATIONS")));	//7
	ImageList.Add(CTempIconLoader(_T("PREF_STATISTICS")));		//8
	ImageList.Add(CTempIconLoader(_T("PREF_IRC")));				//9
	ImageList.Add(CTempIconLoader(_T("PREF_SECURITY")));		//10
	ImageList.Add(CTempIconLoader(_T("PREF_SCHEDULER")));		//11
	ImageList.Add(CTempIconLoader(_T("PREF_WEBSERVER")));		//12
	ImageList.Add(CTempIconLoader(_T("PREF_TWEAK")));			//13
	//MORPH group
	ImageList.Add(CTempIconLoader(_T("PREF_BACKUP"))); //EastShare - Added by Pretender, TBH-AutoBackup
	ImageList.Add(CTempIconLoader(_T("CLIENTMORPH")));  //MORPH - Added by IceCream, Morph Prefs
	ImageList.Add(CTempIconLoader(_T("CLIENTMORPH")));  //MORPH - Added by SiRoB, Morph Prefs
	ImageList.Add(CTempIconLoader(_T("CLIENTMORPH")));  //MORPH - Added by SiRoB, Morph Prefs
	ImageList.Add(CTempIconLoader(_T("CLIENTMORPH"))); //Commander - Added: Morph III
	ImageList.Add(CTempIconLoader(_T("CLIENTEASTSHARE")));  //MORPH - Added by IceCream, Morph Prefs  //EastShare - Modified by Pretender
	ImageList.Add(CTempIconLoader(_T("PREF_EMULESPANA")));  //MORPH - Added by IceCream, eMulEspaña Preferency
	m_slideBar.SetImageList(&ImageList);

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
	m_wndMorph3.Localize(); //Commander - Added: Morph III
	m_wndEastShare.Localize();
	m_wndEmulespana.Localize(); //MORPH - Added by SiRoB, emulEspaña preferency

	m_slideBar.ResetContent();

//	General group
	int iGroup = m_slideBar.AddGroup(GetResString(IDS_PW_GENERAL)/*, 1*/);
	m_slideBar.AddGroupItem(GetResString(IDS_PW_GENERAL), iGroup, 0);
	m_slideBar.AddGroupItem(GetResString(IDS_PW_DISPLAY), iGroup, 1);
	m_slideBar.AddGroupItem(GetResString(IDS_PW_DIR), iGroup, 5);
	m_slideBar.AddGroupItem(GetResString(IDS_PW_FILES), iGroup, 6);
	m_slideBar.AddGroupItem(GetResString(IDS_STATSSETUPINFO), iGroup, 8);
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	m_slideBar.AddGroupItem(_T("Debug"), iGroup, 13);
#endif

	//	Connexion group
	iGroup = m_slideBar.AddGroup(GetResString(IDS_PW_CONNECTION)/*, 1*/);
	m_slideBar.AddGroupItem(GetResString(IDS_PW_CONNECTION), iGroup, 2);
	m_slideBar.AddGroupItem(GetResString(IDS_PW_PROXY), iGroup, 3);
	m_slideBar.AddGroupItem(GetResString(IDS_PW_SERVER), iGroup, 4);

	//	Advanced Official Group
	iGroup = m_slideBar.AddGroup(GetResString(IDS_ADVANCED));
	m_slideBar.AddGroupItem(GetResString(IDS_IRC), iGroup, 9);
	m_slideBar.AddGroupItem(GetResString(IDS_PW_EKDEV_OPTIONS), iGroup, 7);
	m_slideBar.AddGroupItem(GetResString(IDS_PW_WS), iGroup, 12);
	m_slideBar.AddGroupItem(GetResString(IDS_SECURITY), iGroup, 10);
	m_slideBar.AddGroupItem(GetResString(IDS_SCHEDULER), iGroup, 11);
	m_slideBar.AddGroupItem(GetResString(IDS_PW_TWEAK), iGroup, 13);

	//	Advanced group
	iGroup = m_slideBar.AddGroup(_T("MOD"));
	m_slideBar.AddGroupItem(GetResString(IDS_BACKUP), iGroup, 14);
	m_slideBar.AddGroupItem(_T("Morph"), iGroup, 15);
	m_slideBar.AddGroupItem(_T("Morph Share"), iGroup, 16);
	m_slideBar.AddGroupItem(_T("Morph Update"), iGroup, 17);
	m_slideBar.AddGroupItem(_T("Morph DynDns"), iGroup, 18); //Commander - Added: Morph III
	m_slideBar.AddGroupItem(_T("EastShare"), iGroup, 19);
	m_slideBar.AddGroupItem(_T("emulEspaña"), iGroup, 20); //MORPH - Added by SiRoB, emulEspaña preferency

	//	Determines the width needed to the slidebar, and its position
	int width = m_slideBar.GetGreaterStringWidth();
	CTabCtrl* tab = GetTabControl();
	width += 60;
	CRect rectOld;

	m_slideBar.GetWindowRect(rectOld);

	int xoffset, yoffset;
	if(IsWindowVisible())
	{
		yoffset=0;
		xoffset=width-rectOld.Width();
	}
	else
	{
		xoffset=width-rectOld.Width()+10;
		GetActivePage()->GetWindowRect(rectOld);
		tab->GetItemRect(0,rectOld);
		yoffset=-rectOld.Height();
	}
	GetWindowRect(rectOld);
	SetWindowPos(NULL,0,0,rectOld.Width()+xoffset,rectOld.Height()+yoffset,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
	tab->GetWindowRect (rectOld);
	ScreenToClient (rectOld);
	tab->SetWindowPos(NULL,rectOld.left+xoffset,rectOld.top+yoffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	CPropertyPage* activepage = GetActivePage();
	activepage->GetWindowRect(rectOld);
	ScreenToClient (rectOld);
	activepage->SetWindowPos(NULL,rectOld.left+xoffset,rectOld.top+yoffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	activepage->GetWindowRect(rectOld);
	ScreenToClient (rectOld);
	m_groupbox.SetWindowPos(NULL,rectOld.left,2,rectOld.Width()+4,rectOld.Height()+10,SWP_NOZORDER|SWP_NOACTIVATE);
	m_groupbox.GetWindowRect(rectOld);
	ScreenToClient(rectOld);
	GetClientRect(rectOld);
	m_slideBar.SetWindowPos(NULL, 6, 6, width, rectOld.Height() - 12, SWP_NOZORDER | SWP_NOACTIVATE);
	int _PropSheetButtons[] = {IDOK, IDCANCEL, ID_APPLY_NOW, IDHELP };
	CWnd* PropSheetButton;
	for (int i = 0; i < sizeof (_PropSheetButtons) / sizeof(_PropSheetButtons[0]); i++)
	{
		if ((PropSheetButton = GetDlgItem(_PropSheetButtons[i])) != NULL)
		{
			PropSheetButton->GetWindowRect (rectOld);
			ScreenToClient (rectOld);
			PropSheetButton->SetWindowPos (NULL, rectOld.left+xoffset,rectOld.top+yoffset,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
		}
	}
	tab->ShowWindow(SW_HIDE);
	m_slideBar.SelectGlobalItem(GetActiveIndex());
	OnSlideBarSelChanged(NULL, NULL);
	m_banner.UpdateSize(); //Commander - Added: Preferences Banner [TPT]	
	CenterWindow();
	Invalidate();
	RedrawWindow();
}

HBRUSH CPreferencesDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CPropertySheet::OnCtlColor(pDC, pWnd, nCtlColor);
	if (m_groupbox.m_hWnd == pWnd->m_hWnd) 
	{
		pDC->SetBkColor(GetSysColor(COLOR_BTNFACE));
		hbr = GetSysColorBrush(COLOR_BTNFACE);
	}
	return hbr;
}
void CPreferencesDlg::OnHelp()
{
	int iCurSel = m_listbox.GetCurSel();
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
void CPreferencesDlg::OpenPage(UINT uResourceID)
{
	int iCurActiveWnd = m_nActiveWnd;
	for (int i = 0; i < m_pages.GetSize(); i++)
	{
		CPropertyPage* pPage = GetPage(i);
		if (pPage->m_psp.pszTemplate == MAKEINTRESOURCE(uResourceID))
		{
			m_nActiveWnd = i;
			break;
		}
	}
	DoModal();
	m_nActiveWnd = iCurActiveWnd;
}
