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
#include "MuleToolbarCtrl.h"
#include "SearchDlg.h"
#include "KademliaWnd.h"
#include "EnBitmap.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "Sockets.h"
#include "MenuCmds.h"
#include "MuleStatusbarCtrl.h"
#include "ServerWnd.h"
#include "TransferWnd.h"
#include "SharedFilesWnd.h"
#include "ChatWnd.h"
#include "IrcWnd.h"
#include "StatisticsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	NUM_BUTTON_BITMAPS	14

#define	EMULTB_BASEEXT		_T("eMuleToolbar.kad02")

static const LPCTSTR _apszTBFiles[] = 
{
	_T("*.") EMULTB_BASEEXT _T(".bmp"),
	_T("*.") EMULTB_BASEEXT _T(".gif"),
	_T("*.") EMULTB_BASEEXT _T(".png")
};

static const LPCTSTR _apszSkinFiles[] = 
{
	_T("*.") EMULSKIN_BASEEXT _T(".ini"),
};

#define	MAX_TOOLBAR_FILES	100
#define	MAX_SKIN_FILES		100

// CMuleToolbarCtrl

IMPLEMENT_DYNAMIC(CMuleToolbarCtrl, CToolBarCtrl)
 
BEGIN_MESSAGE_MAP(CMuleToolbarCtrl, CToolBarCtrl)
	ON_WM_SIZE()
	ON_NOTIFY_REFLECT(NM_RCLICK, OnNMRclick)
	ON_NOTIFY_REFLECT(TBN_QUERYDELETE, OnTbnQueryDelete)
	ON_NOTIFY_REFLECT(TBN_QUERYINSERT, OnTbnQueryInsert)
	ON_NOTIFY_REFLECT(TBN_GETBUTTONINFO, OnTbnGetButtonInfo)
	ON_NOTIFY_REFLECT(TBN_TOOLBARCHANGE, OnTbnToolbarChange)
	ON_NOTIFY_REFLECT(TBN_RESET, OnTbnReset)
	ON_NOTIFY_REFLECT(TBN_INITCUSTOMIZE, OnTbnInitCustomize)
	ON_NOTIFY_REFLECT(TBN_ENDADJUST, OnTbnEndAdjust)
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

CMuleToolbarCtrl::CMuleToolbarCtrl()
{
	m_sizBtnBmp.cx = thePrefs.GetToolbarIconSize().cx;
	m_sizBtnBmp.cy = thePrefs.GetToolbarIconSize().cy;
	m_iPreviousHeight = 0;
	m_iLastPressedButton = -1;
	m_buttoncount = 0;
	memset(TBButtons, 0, sizeof(TBButtons));
	memset(TBStrings, 0, sizeof(TBStrings));
	m_eLabelType = NoLabels;
}

CMuleToolbarCtrl::~CMuleToolbarCtrl()
{
	if (m_bmpBack.m_hObject)
		VERIFY( m_bmpBack.DeleteObject() );
}

void CMuleToolbarCtrl::Init(void)
{
	m_astrToolbarPaths.RemoveAll();

	ModifyStyle(0, TBSTYLE_FLAT | TBSTYLE_ALTDRAG | CCS_ADJUSTABLE | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER);
	if (thePrefs.GetUseReBarToolbar())
	{
		ModifyStyle(0, CCS_NORESIZE);
		SetExtendedStyle(GetExtendedStyle() | TBSTYLE_EX_HIDECLIPPEDBUTTONS);
	}

	ChangeToolbarBitmap(thePrefs.GetToolbarBitmapSettings(), false);

	// add button-text:
	TCHAR cButtonStrings[2000];
	int lLen, lLen2;
	m_buttoncount = 0;
	
	_tcscpy(cButtonStrings, GetResString(IDS_MAIN_BTN_CONNECT));
	lLen = _tcslen(GetResString(IDS_MAIN_BTN_CONNECT)) + 1;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_KADEMLIA)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_KADEMLIA), lLen2*sizeof(TCHAR));
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_SERVER)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_SERVER), lLen2*sizeof(TCHAR));
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_TRANS)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_TRANS), lLen2*sizeof(TCHAR));
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_SEARCH)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_SEARCH), lLen2*sizeof(TCHAR));
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_FILES)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_FILES), lLen2*sizeof(TCHAR));
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_MESSAGES)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_MESSAGES), lLen2*sizeof(TCHAR));
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_IRC)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_IRC), lLen2*sizeof(TCHAR));
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_STATISTIC)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_STATISTIC), lLen2*sizeof(TCHAR));
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_PREFS)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_PREFS), lLen2*sizeof(TCHAR));
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_TOOLS)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_TOOLS), lLen2*sizeof(TCHAR));
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_HELP)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_HELP), lLen2*sizeof(TCHAR));
	lLen += lLen2;
	++m_buttoncount;

	// terminate
	memcpy(cButtonStrings+lLen, _T("\0"), sizeof(TCHAR));

	AddStrings(cButtonStrings);

	// initialize buttons:	
	for(int i = 0; i < m_buttoncount; i++)
	{		
		TBButtons[i].fsState	= TBSTATE_ENABLED;
		TBButtons[i].fsStyle	= TBSTYLE_CHECKGROUP;
		TBButtons[i].idCommand	= IDC_TOOLBARBUTTON + i;
		TBButtons[i].iString	= i;

		switch (TBButtons[i].idCommand)
		{
			case TBBTN_CONNECT:
			case TBBTN_OPTIONS:
			case TBBTN_TOOLS:
			case TBBTN_HELP:
				TBButtons[i].fsStyle = TBSTYLE_BUTTON;
				break;
		}
	}

	// set button image indices
	int iBitmap = 0;
	for (int i = 0; i < m_buttoncount; i++)
	{		
		TBButtons[i].iBitmap = iBitmap;
		if (TBButtons[i].idCommand == TBBTN_CONNECT) // 'Connect' button has 3 states
			iBitmap += 3;
		else
			iBitmap += 1;
	}
	
	TBBUTTON sepButton = {0};
	sepButton.idCommand = 0;
	sepButton.fsStyle = TBSTYLE_SEP;
	sepButton.fsState = TBSTATE_ENABLED;
	sepButton.iString = -1;
	sepButton.iBitmap = -1;
	
	CString config = thePrefs.GetToolbarSettings();
	for (i = 0; i < config.GetLength(); i += 2)
	{
		int index = _tstoi(config.Mid(i, 2));
		if (index == 99)
		{
			AddButtons(1, &sepButton);
			continue;
		}
		AddButtons(1, &TBButtons[index]);
	}

	// recalc toolbar-size
	SetAllButtonsStrings();
	ChangeTextLabelStyle(thePrefs.GetToolbarLabelSettings(), false, true);
	SetAllButtonsWidth();	// then calc and set the button width
	AutoSize();				// and finally call the original (but maybe obsolete) function
	SaveCurHeight();
}

void CMuleToolbarCtrl::SetAllButtonsStrings()
{
	static const int TBStringIDs[] =
	{
		IDS_EM_KADEMLIA, 
		IDS_EM_SERVER,
		IDS_EM_TRANS,
		IDS_EM_SEARCH,
		IDS_EM_FILES,
		IDS_EM_MESSAGES,
		IDS_IRC,
		IDS_EM_STATISTIC,
		IDS_EM_PREFS,
		IDS_TOOLS,
		IDS_EM_HELP
	};
	TBBUTTONINFO tbi;
	tbi.dwMask = TBIF_TEXT;
	tbi.cbSize = sizeof(TBBUTTONINFO);

	CString buffer;
	if (theApp.serverconnect->IsConnected())
		buffer = GetResString(IDS_MAIN_BTN_DISCONNECT);
	else if (theApp.serverconnect->IsConnecting())
		buffer = GetResString(IDS_MAIN_BTN_CANCEL);
	else
		buffer = GetResString(IDS_MAIN_BTN_CONNECT);

	_sntprintf(TBStrings[0], ARRSIZE(TBStrings[0]), _T("%s"), buffer);
	tbi.pszText = TBStrings[0];
	SetButtonInfo(IDC_TOOLBARBUTTON+0, &tbi);

	for (int i = 1; i < m_buttoncount; i++)
	{
		_sntprintf(TBStrings[i], ARRSIZE(TBStrings[0]), _T("%s"), GetResString(TBStringIDs[i-1]));
		tbi.pszText = TBStrings[i];
		SetButtonInfo(IDC_TOOLBARBUTTON+i, &tbi);
	}
}

void CMuleToolbarCtrl::Localize(void)
{
	if (m_hWnd)
	{
		SetAllButtonsStrings();
		SetAllButtonsWidth();
		AutoSize();
		UpdateIdealSize();
	}
}

void CMuleToolbarCtrl::OnSize(UINT nType, int cx, int cy)
{
	CToolBarCtrl::OnSize(nType, cx, cy);

	SetAllButtonsWidth();
	AutoSize();
}

void CMuleToolbarCtrl::SetAllButtonsWidth()
{
	if (GetButtonCount() == 0)
		return;

	if (m_eLabelType == LabelsBelow)
	{
		CDC *pDC = GetDC();
		CFont *pFnt = GetFont();
		CFont *pOldFnt = pDC->SelectObject(pFnt);
		CRect r(0,0,0,0);

		// calculate the max. possible button-size
		int iCalcSize = 0;
		for (int i = 0; i < m_buttoncount ; i++)
		{
			if (!IsButtonHidden(IDC_TOOLBARBUTTON + i))
			{
				pDC->DrawText(TBStrings[i], -1, r, DT_SINGLELINE | DT_CALCRECT);
 				if (r.Width() > iCalcSize)
					iCalcSize = r.Width();
			}
		}
		iCalcSize += 10;

		pDC->SelectObject(pOldFnt);
		ReleaseDC(pDC);

		if (!thePrefs.GetUseReBarToolbar())
		{
			GetClientRect(&r);
			int bc = GetButtonCount();
			if (bc == 0)
				bc = 1;
			int iMaxPossible = r.Width() / bc;

			// if the buttons are to big, reduze their size
			if (iCalcSize > iMaxPossible)
				iCalcSize = iMaxPossible;
		}
		else
		{
			if (iCalcSize < 56)
				iCalcSize = 56;
			else if (iCalcSize > 70)
				iCalcSize = 70;
		}
		SetButtonWidth(iCalcSize, iCalcSize);
	}
	else
	{
		const int iSmallIconsButtonHeight = 28;

		if (m_eLabelType == NoLabels)
		{
			DWORD dwSize = GetButtonSize();
			int iFixedButtonWidth;
			int iFixedButtonHeight = HIWORD(dwSize);
			if (m_sizBtnBmp.cx == 16)
			{
				iFixedButtonWidth = 28;
				iFixedButtonHeight = iSmallIconsButtonHeight;
			}
			else
			{
				iFixedButtonWidth = 56;
			}

			// it seems that the control updates itself more properly, if 'SetButtonWidth' id called *before* 'SetButtonSize'
			SetButtonWidth(iFixedButtonWidth, iFixedButtonWidth);
			SetButtonSize(CSize(iFixedButtonWidth, iFixedButtonHeight));
		}
		else
		{
			int iFixedButtonHeight = 0;
			if (m_sizBtnBmp.cx == 16)
				iFixedButtonHeight = iSmallIconsButtonHeight;

			// it seems that the control updates itself more properly, if 'SetButtonWidth' id called *before* 'SetButtonSize'
			SetButtonWidth(0, 0);
			SetButtonSize(CSize(0, iFixedButtonHeight));
		}
	}
}

void CMuleToolbarCtrl::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (GetKeyState(VK_CONTROL) & 0x8000)
	{
		if (!thePrefs.GetToolbarBitmapSettings().IsEmpty())
			ChangeToolbarBitmap(thePrefs.GetToolbarBitmapSettings(), true);
		if (!thePrefs.GetSkinProfile().IsEmpty())
			theApp.ApplySkin(thePrefs.GetSkinProfile());

		*pResult = TRUE;
		return;
	}


	///////////////////////////////////////////////////////////////////////////
	// "Toolbar Bitmap" sub menu
	//
	CMenu menuBitmaps;
	menuBitmaps.CreateMenu();
	menuBitmaps.AppendMenu(MF_STRING, MP_SELECTTOOLBARBITMAP, GetResString(IDS_SELECTTOOLBARBITMAP));
	menuBitmaps.AppendMenu(MF_STRING, MP_SELECTTOOLBARBITMAPDIR, GetResString(IDS_SELECTTOOLBARBITMAPDIR));
	menuBitmaps.AppendMenu(MF_SEPARATOR);
	menuBitmaps.AppendMenu(MF_STRING, MP_TOOLBARBITMAP, GetResString(IDS_DEFAULT));
	
	m_astrToolbarPaths.RemoveAll();
	CString currentBitmapSettings = thePrefs.GetToolbarBitmapSettings();
	bool checked = false;
	if (currentBitmapSettings.IsEmpty())
	{
		menuBitmaps.CheckMenuItem(MP_TOOLBARBITMAP, MF_CHECKED);
		menuBitmaps.EnableMenuItem(MP_TOOLBARBITMAP, MF_DISABLED);
		checked = true;
	}
	m_astrToolbarPaths.Add(_T("")); // dummy entry for 'Default' menu item
	int i = 1;
	if (!thePrefs.GetToolbarBitmapFolderSettings().IsEmpty())
	{
		CStringArray astrToolbarFiles;
		for (int f = 0; f < ARRSIZE(_apszTBFiles); f++)
		{
			WIN32_FIND_DATA FileData;
			HANDLE hSearch = FindFirstFile(thePrefs.GetToolbarBitmapFolderSettings() + CString(_T("\\")) + _apszTBFiles[f], &FileData);
			if (hSearch != INVALID_HANDLE_VALUE)
			{
				do {
					astrToolbarFiles.Add(FileData.cFileName);
				}
				while (astrToolbarFiles.GetCount() < MAX_TOOLBAR_FILES && FindNextFile(hSearch, &FileData));
				FindClose(hSearch);
			}
		}

		if (astrToolbarFiles.GetCount() > 0)
		{
			Sort(astrToolbarFiles);
			for (int f = 0; f < astrToolbarFiles.GetCount(); f++)
			{
				const CString& bitmapFileName = astrToolbarFiles.GetAt(f);
				CString bitmapBaseName;
				LPCTSTR pszTbBaseExt = stristr(bitmapFileName, EMULTB_BASEEXT);
				if (pszTbBaseExt)
					bitmapBaseName = bitmapFileName.Left(pszTbBaseExt - (LPCTSTR)bitmapFileName - 1);
				else
					bitmapBaseName = bitmapFileName;
				menuBitmaps.AppendMenu(MF_STRING, MP_TOOLBARBITMAP + i, bitmapBaseName);
				m_astrToolbarPaths.Add(thePrefs.GetToolbarBitmapFolderSettings() + CString(_T("\\")) + bitmapFileName);
				if (!checked && currentBitmapSettings.CompareNoCase(m_astrToolbarPaths[i]) == 0)
				{
					menuBitmaps.CheckMenuItem(MP_TOOLBARBITMAP + i, MF_CHECKED);
					menuBitmaps.EnableMenuItem(MP_TOOLBARBITMAP + i, MF_DISABLED);
					checked = true;
				}
				i++;
			}
		}
		ASSERT( i-1 == astrToolbarFiles.GetCount() );
	}
	if (!checked)
	{
		menuBitmaps.AppendMenu(MF_STRING, MP_TOOLBARBITMAP + i, currentBitmapSettings);
		menuBitmaps.CheckMenuItem(MP_TOOLBARBITMAP + i, MF_CHECKED);
		menuBitmaps.EnableMenuItem(MP_TOOLBARBITMAP + i, MF_DISABLED);
		m_astrToolbarPaths.Add(currentBitmapSettings);
	}


	///////////////////////////////////////////////////////////////////////////
	// "Skin Profile" sub menu
	//
	CMenu menuSkins;
	menuSkins.CreateMenu();
	menuSkins.AppendMenu(MF_STRING, MP_SELECT_SKIN_FILE, GetResString(IDS_SEL_SKIN));
	menuSkins.AppendMenu(MF_STRING, MP_SELECT_SKIN_DIR, GetResString(IDS_SEL_SKINDIR));
	menuSkins.AppendMenu(MF_SEPARATOR);
	menuSkins.AppendMenu(MF_STRING, MP_SKIN_PROFILE,GetResString(IDS_DEFAULT));

	m_astrSkinPaths.RemoveAll();
	CString currentSkin = thePrefs.GetSkinProfile();
	checked = false;
	if (currentSkin.IsEmpty())
	{
		menuSkins.CheckMenuItem(MP_SKIN_PROFILE, MF_CHECKED);
		menuSkins.EnableMenuItem(MP_SKIN_PROFILE, MF_DISABLED);
		checked = true;
	}
	m_astrSkinPaths.Add(_T("")); // dummy entry for 'Default' menu item
	i = 1;
	if (!thePrefs.GetSkinProfileDir().IsEmpty())
	{
		CStringArray astrSkinFiles;
		for (int f = 0; f < ARRSIZE(_apszSkinFiles); f++)
		{
			WIN32_FIND_DATA FileData;
			HANDLE hSearch = FindFirstFile(thePrefs.GetSkinProfileDir() + CString(_T("\\")) + _apszSkinFiles[f], &FileData);
			if (hSearch != INVALID_HANDLE_VALUE)
			{
				do {
					astrSkinFiles.Add(FileData.cFileName);
				}
				while (astrSkinFiles.GetCount() < MAX_SKIN_FILES && FindNextFile(hSearch, &FileData));
				FindClose(hSearch);
			}
		}

		if (astrSkinFiles.GetCount() > 0)
		{
			Sort(astrSkinFiles);
			for (int f = 0; f < astrSkinFiles.GetCount(); f++)
			{
				const CString& skinFileName = astrSkinFiles.GetAt(f);
				CString skinBaseName;
				LPCTSTR pszSkinBaseExt = stristr(skinFileName, _T(".") EMULSKIN_BASEEXT _T(".ini"));
				if (pszSkinBaseExt)
					skinBaseName = skinFileName.Left(pszSkinBaseExt - (LPCTSTR)skinFileName);
				else
					skinBaseName = skinFileName;
				menuSkins.AppendMenu(MF_STRING, MP_SKIN_PROFILE + i, skinBaseName);
				m_astrSkinPaths.Add(thePrefs.GetSkinProfileDir() + CString(_T("\\")) + skinFileName);
				if (!checked && currentSkin.CompareNoCase(m_astrSkinPaths[i]) == 0)
				{
					menuSkins.CheckMenuItem(MP_SKIN_PROFILE + i, MF_CHECKED);
					menuSkins.EnableMenuItem(MP_SKIN_PROFILE + i, MF_DISABLED);
					checked = true;
				}
				i++;
			}
		}
		ASSERT( i-1 == astrSkinFiles.GetCount() );
	}
	if (!checked)
	{
		menuSkins.AppendMenu(MF_STRING, MP_SKIN_PROFILE + i, currentSkin);
		menuSkins.CheckMenuItem(MP_SKIN_PROFILE + i, MF_CHECKED);
		menuSkins.EnableMenuItem(MP_SKIN_PROFILE + i, MF_DISABLED);
		m_astrSkinPaths.Add(currentSkin);
	}
	

	///////////////////////////////////////////////////////////////////////////
	// "Text Label" sub menu
	//
	CMenu menuTextLabels;
	menuTextLabels.CreateMenu();
	ASSERT( MP_NOTEXTLABELS == MP_TEXTLABELS-1 && MP_NOTEXTLABELS == MP_TEXTLABELSONRIGHT-2 );
	ASSERT( MP_NOTEXTLABELS + (int)NoLabels == MP_NOTEXTLABELS );
	ASSERT( MP_NOTEXTLABELS + (int)LabelsBelow == MP_TEXTLABELS );
	ASSERT( MP_NOTEXTLABELS + (int)LabelsRight == MP_TEXTLABELSONRIGHT );
	menuTextLabels.AppendMenu(MF_STRING | MF_ENABLED, MP_NOTEXTLABELS, GetResString(IDS_NOTEXTLABELS));
	menuTextLabels.AppendMenu(MF_STRING | MF_ENABLED, MP_TEXTLABELS, GetResString(IDS_ENABLETEXTLABELS));
	menuTextLabels.AppendMenu(MF_STRING | MF_ENABLED, MP_TEXTLABELSONRIGHT, GetResString(IDS_TEXTLABELSONRIGHT));
	menuTextLabels.CheckMenuRadioItem(MP_NOTEXTLABELS, MP_TEXTLABELSONRIGHT, MP_NOTEXTLABELS + (int)thePrefs.GetToolbarLabelSettings(), MF_BYCOMMAND);
	menuTextLabels.EnableMenuItem(MP_NOTEXTLABELS + (int)thePrefs.GetToolbarLabelSettings(), MF_BYCOMMAND | MF_DISABLED);

	menuTextLabels.AppendMenu(MF_SEPARATOR);
	menuTextLabels.AppendMenu(MF_STRING, MP_LARGEICONS, GetResString(IDS_LARGEICONS));
	menuTextLabels.AppendMenu(MF_STRING, MP_SMALLICONS, GetResString(IDS_SMALLICONS));
	ASSERT( MP_LARGEICONS == MP_SMALLICONS-1 );
	menuTextLabels.CheckMenuRadioItem(MP_LARGEICONS, MP_SMALLICONS, m_sizBtnBmp.cx == 16 ? MP_SMALLICONS : MP_LARGEICONS, MF_BYCOMMAND);
	menuTextLabels.EnableMenuItem(m_sizBtnBmp.cx == 16 ? MP_SMALLICONS : MP_LARGEICONS, MF_BYCOMMAND | MF_DISABLED);


	///////////////////////////////////////////////////////////////////////////
	// Toolbar context menu
	//
	CMenu menuToolbar;
	menuToolbar.CreatePopupMenu();
	menuToolbar.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)menuBitmaps.m_hMenu, GetResString(IDS_TOOLBARSKINS));
	menuToolbar.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)menuSkins.m_hMenu, GetResString(IDS_SKIN_PROF));
	menuToolbar.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)menuTextLabels.m_hMenu, GetResString(IDS_TEXTLABELS));
	menuToolbar.AppendMenu(MF_STRING, MP_CUSTOMIZETOOLBAR, GetResString(IDS_CUSTOMIZETOOLBAR));
	CPoint point;
	GetCursorPos(&point);
	menuToolbar.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);

	*pResult = TRUE;
}

void CMuleToolbarCtrl::OnTbnQueryDelete(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = TRUE;
}

void CMuleToolbarCtrl::OnTbnQueryInsert(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = TRUE;
}

void CMuleToolbarCtrl::OnTbnGetButtonInfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTOOLBAR pNMTB = reinterpret_cast<LPNMTOOLBAR>(pNMHDR);
	if (pNMTB->iItem >= ARRSIZE(TBButtons))
	{
		*pResult = FALSE;
	}
	else
	{
		CString strText = TBStrings[pNMTB->iItem];
		strText.Remove(_T('&'));
		_tcsncpy(pNMTB->pszText, strText, pNMTB->cchText - 1);
		pNMTB->pszText[pNMTB->cchText - 1] = _T('\0');
		pNMTB->tbButton = TBButtons[pNMTB->iItem];
		if (m_eLabelType == LabelsRight)
			pNMTB->tbButton.fsStyle |= TBSTYLE_AUTOSIZE;
		*pResult = TRUE;
	}
}

void CMuleToolbarCtrl::OnTbnToolbarChange(NMHDR *pNMHDR, LRESULT *pResult)
{
	CString config;
	for (int i = 0; i < GetButtonCount();i++)
	{
		TBBUTTON buttoninfo;
		if (GetButton(i, &buttoninfo))
			config.AppendFormat(_T("%02i"), (buttoninfo.idCommand != 0) ? buttoninfo.idCommand - IDC_TOOLBARBUTTON : 99);
	}

	thePrefs.SetToolbarSettings(config);
	Localize();

	theApp.emuledlg->ShowConnectionState();

	SetAllButtonsWidth();
	AutoSize();

	*pResult = 0;
}

void CMuleToolbarCtrl::ChangeToolbarBitmap(const CString& path, bool bRefresh)
{
	bool bResult = false;
	CImageList ImageList;
	CEnBitmap Bitmap;
	if (!path.IsEmpty() && Bitmap.LoadImage(path))
	{
		BITMAP bm = {0};
		Bitmap.GetObject(sizeof(bm), &bm);
		if (bm.bmWidth == NUM_BUTTON_BITMAPS*m_sizBtnBmp.cx && bm.bmHeight == m_sizBtnBmp.cy)
		{
			bool bAlpha = bm.bmBitsPixel > 24;
			if (ImageList.Create(m_sizBtnBmp.cx, bm.bmHeight, bAlpha ? ILC_COLOR32 : (theApp.m_iDfltImageListColorFlags | ILC_MASK), 0, 1))
			{
				ImageList.Add(&Bitmap,  bAlpha ? 0xFF000000 : RGB(255, 0, 255));
				CImageList* pimlOld = SetImageList(&ImageList);
				ImageList.Detach();
				if (pimlOld)
					pimlOld->DeleteImageList();
				bResult = true;
			}
		}
		Bitmap.DeleteObject();
	}

	// if image file loading or image list creation failed, create default image list.
	if (!bResult)
	{
		// load from icon ressources
		ImageList.Create(m_sizBtnBmp.cx, m_sizBtnBmp.cy, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
		ImageList.Add(CTempIconLoader(_T("CONNECT"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("DISCONNECT"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("STOPCONNECTING"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("KADEMLIA"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("SERVER"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("TRANSFER"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("SEARCH"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("SharedFiles"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("MESSAGES"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("IRC"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("STATISTICS"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("PREFERENCES"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("TOOLS"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ImageList.Add(CTempIconLoader(_T("HELP"), m_sizBtnBmp.cx, m_sizBtnBmp.cy));
		ASSERT( ImageList.GetImageCount() == NUM_BUTTON_BITMAPS );
		CImageList* pimlOld = SetImageList(&ImageList);
		ImageList.Detach();
		if (pimlOld)
			pimlOld->DeleteImageList();
	}

	if (bRefresh)
	{
		UpdateBackground();
		Invalidate();
		Refresh();
	}
}

BOOL CMuleToolbarCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
		case MP_SELECTTOOLBARBITMAPDIR:{
			TCHAR buffer[MAX_PATH];
			_sntprintf(buffer,ARRSIZE(buffer),_T("%s"), thePrefs.GetToolbarBitmapFolderSettings());
			if(SelectDir(m_hWnd, buffer, GetResString(IDS_SELECTTOOLBARBITMAPDIR)))
				thePrefs.SetToolbarBitmapFolderSettings(buffer);
			break;
		}
		case MP_CUSTOMIZETOOLBAR:
			Customize();
			break;

		case MP_SELECTTOOLBARBITMAP:
		{
			// we could also load "*.jpg" here, but because of the typical non solid background of JPGs this
			// doesn't make sense here.
			CString strFilter(_T("eMule Toolbar Bitmap Files ("));
			for (int f = 0; f < ARRSIZE(_apszTBFiles); f++){
				if (f > 0)
					strFilter += _T(';');
				strFilter += _apszTBFiles[f];
			}
			strFilter += _T(")|");
			for (int f = 0; f < ARRSIZE(_apszTBFiles); f++){
				if (f > 0)
					strFilter += _T(';');
				strFilter += _apszTBFiles[f];
			}
			strFilter += _T("||");
			CFileDialog dialog(TRUE, EMULTB_BASEEXT _T(".bmp"), NULL, OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, strFilter, NULL, 0);
			if (IDOK == dialog.DoModal())
				if(thePrefs.GetToolbarBitmapSettings()!=dialog.GetPathName())
				{
					ChangeToolbarBitmap(dialog.GetPathName(), true);
					thePrefs.SetToolbarBitmapSettings(dialog.GetPathName());
				}
			break;
		}

		case MP_LARGEICONS:
			m_sizBtnBmp.cx = m_sizBtnBmp.cy = 32;
			ForceRecalcLayout();
			ChangeToolbarBitmap(thePrefs.GetToolbarBitmapSettings(), true);
			thePrefs.SetToolbarIconSize(m_sizBtnBmp);
			break;

       	case MP_SMALLICONS:
			m_sizBtnBmp.cx = m_sizBtnBmp.cy = 16;
			ForceRecalcLayout();
			ChangeToolbarBitmap(thePrefs.GetToolbarBitmapSettings(), true);
			thePrefs.SetToolbarIconSize(m_sizBtnBmp);
			break;

		case MP_NOTEXTLABELS:
			ForceRecalcLayout();
			ChangeTextLabelStyle(NoLabels, true);
			thePrefs.SetToolbarLabelSettings(NoLabels);
			break;

		case MP_TEXTLABELS:
			ForceRecalcLayout();
			ChangeTextLabelStyle(LabelsBelow, true);
			thePrefs.SetToolbarLabelSettings(LabelsBelow);
			break;

		case MP_TEXTLABELSONRIGHT:
			ForceRecalcLayout();
			ChangeTextLabelStyle(LabelsRight, true);
			thePrefs.SetToolbarLabelSettings(LabelsRight);
			break;

		case MP_SELECT_SKIN_DIR:{
			TCHAR buffer[MAX_PATH];
			_sntprintf(buffer,ARRSIZE(buffer),_T("%s"), thePrefs.GetSkinProfileDir());
			if(SelectDir(m_hWnd, buffer, GetResString(IDS_SELSKINPROFILEDIR)))
				thePrefs.SetSkinProfileDir(buffer);
			break;
		}
		case MP_SELECT_SKIN_FILE:
		{
			CString strFilter(_T("eMule Skin Files ("));
			for (int f = 0; f < ARRSIZE(_apszSkinFiles); f++){
				if (f > 0)
					strFilter += _T(';');
				strFilter += _apszSkinFiles[f];
			}
			strFilter += _T(")|");
			for (int f = 0; f < ARRSIZE(_apszSkinFiles); f++){
				if (f > 0)
					strFilter += _T(';');
				strFilter += _apszSkinFiles[f];
			}
			strFilter += _T("||");
			CFileDialog dialog(TRUE, EMULSKIN_BASEEXT _T(".ini"), NULL, OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST, strFilter, NULL, 0);
			if (dialog.DoModal() == IDOK)
			{
				if (thePrefs.GetSkinProfile().CompareNoCase(dialog.GetPathName()) != 0)
					theApp.ApplySkin(dialog.GetPathName());
			}
			break;
		}

		default:
			if (wParam >= MP_TOOLBARBITMAP && wParam < MP_TOOLBARBITMAP + MAX_TOOLBAR_FILES)
			{
				if (thePrefs.GetToolbarBitmapSettings().CompareNoCase(m_astrToolbarPaths[wParam-MP_TOOLBARBITMAP]) != 0)
				{
					ChangeToolbarBitmap(m_astrToolbarPaths[wParam-MP_TOOLBARBITMAP], true);
					thePrefs.SetToolbarBitmapSettings(m_astrToolbarPaths[wParam-MP_TOOLBARBITMAP]);
				}
			}
			else if (wParam >= MP_SKIN_PROFILE && wParam < MP_SKIN_PROFILE + MAX_SKIN_FILES)
			{
				if (thePrefs.GetSkinProfile().CompareNoCase(m_astrSkinPaths[wParam - MP_SKIN_PROFILE]) != 0)
					theApp.ApplySkin(m_astrSkinPaths[wParam - MP_SKIN_PROFILE]);
			}
	}

	return true;
}

void CMuleToolbarCtrl::ChangeTextLabelStyle(EToolbarLabelType eLabelType, bool bRefresh, bool bForceUpdateButtons)
{
	if (m_eLabelType != eLabelType || bForceUpdateButtons)
	{
		switch (eLabelType)
		{
			case NoLabels:
				SetStyle(GetStyle() & ~TBSTYLE_LIST);
				SetMaxTextRows(0);
				break;
			case LabelsBelow:
				SetStyle(GetStyle() & ~TBSTYLE_LIST);
				SetMaxTextRows(1);
				break;
			case LabelsRight:
				SetStyle(GetStyle() | TBSTYLE_LIST);
				SetMaxTextRows(1);
				break;
		}

		for (int i = 0; i < m_buttoncount; i++)
		{	
			TBBUTTONINFO tbbi = {0};
			tbbi.cbSize = sizeof(tbbi);
			tbbi.dwMask = TBIF_STYLE;
			GetButtonInfo(IDC_TOOLBARBUTTON + i, &tbbi);
			if (eLabelType == LabelsRight)
				tbbi.fsStyle |= TBSTYLE_AUTOSIZE;
			else
				tbbi.fsStyle &= ~TBSTYLE_AUTOSIZE;
			SetButtonInfo(IDC_TOOLBARBUTTON + i, &tbbi);
		}

		m_eLabelType = eLabelType;
		if (bRefresh)
			Refresh();
	}
}

void CMuleToolbarCtrl::Refresh()
{
	SetAllButtonsWidth();
	AutoSize();	// Causes a toolbar to be resized.

	if (theApp.emuledlg->m_ctlMainTopReBar.m_hWnd)
	{
	    theApp.emuledlg->RemoveAnchor(theApp.emuledlg->m_ctlMainTopReBar.m_hWnd);

	    REBARBANDINFO rbbi = {0};
	    CSize sizeBar;
	    GetMaxSize(&sizeBar);
		ASSERT( sizeBar.cx != 0 && sizeBar.cy != 0 );
	    rbbi.cbSize = sizeof(rbbi);
	    rbbi.fMask = RBBIM_CHILDSIZE | RBBIM_IDEALSIZE;
	    rbbi.cxMinChild = sizeBar.cy;
	    rbbi.cyMinChild = sizeBar.cy;
	    rbbi.cxIdeal = sizeBar.cx;
	    VERIFY( theApp.emuledlg->m_ctlMainTopReBar.SetBandInfo(MULE_TOOLBAR_BAND_NR, &rbbi) );

	    theApp.emuledlg->AddAnchor(theApp.emuledlg->m_ctlMainTopReBar.m_hWnd, TOP_LEFT, TOP_RIGHT);
	}

	CRect rToolbarRect;
	GetWindowRect(&rToolbarRect);

	if (m_iPreviousHeight == rToolbarRect.Height())
	{
		Invalidate();
		RedrawWindow();
	}
	else
	{
		m_iPreviousHeight = rToolbarRect.Height();

		CRect rClientRect;
		theApp.emuledlg->GetClientRect(&rClientRect);

		CRect rStatusbarRect;
		theApp.emuledlg->statusbar->GetWindowRect(&rStatusbarRect);

		rClientRect.top += rToolbarRect.Height();
		rClientRect.bottom -= rStatusbarRect.Height();

		CWnd* wnds[] =
		{
			theApp.emuledlg->serverwnd,
			theApp.emuledlg->kademliawnd,
			theApp.emuledlg->transferwnd,
			theApp.emuledlg->sharedfileswnd,
			theApp.emuledlg->searchwnd,
			theApp.emuledlg->chatwnd,
			theApp.emuledlg->ircwnd,
			theApp.emuledlg->statisticswnd
		};
		for (int i = 0; i < ARRSIZE(wnds); i++)
		{
			wnds[i]->SetWindowPos(NULL, rClientRect.left, rClientRect.top, rClientRect.Width(), rClientRect.Height(), SWP_NOZORDER);
			theApp.emuledlg->RemoveAnchor(wnds[i]->m_hWnd);
			theApp.emuledlg->AddAnchor(wnds[i]->m_hWnd, TOP_LEFT, BOTTOM_RIGHT);
		}
		theApp.emuledlg->Invalidate();
		theApp.emuledlg->RedrawWindow();
	}
}

void CMuleToolbarCtrl::OnTbnReset(NMHDR *pNMHDR, LRESULT *pResult)
{
	// First get rid of old buttons
	// while saving their states
	for (int i = GetButtonCount()-1; i >= 0; i--)
	{
		TBBUTTON Button;
		GetButton(i, &Button);
		for (int j = 0; j < m_buttoncount ; j++)
		{
			if (TBButtons[j].idCommand == Button.idCommand)
			{
				TBButtons[j].fsState = Button.fsState;
				TBButtons[j].fsStyle = Button.fsStyle;
				TBButtons[j].iString = Button.iString;
			}
		}
		DeleteButton(i);
	}

	TBBUTTON sepButton;
	sepButton.idCommand = 0;
	sepButton.fsStyle = TBSTYLE_SEP;
	sepButton.fsState = TBSTATE_ENABLED;
	sepButton.iString = -1;
	sepButton.iBitmap = -1;
	
	// set default configuration 
	CString config = strDefaultToolbar;
	for (i = 0; i <config.GetLength(); i += 2)
	{
		int index = _tstoi(config.Mid(i, 2));
		if (index == 99)
		{
			AddButtons(1, &sepButton);
			continue;
		}
		AddButtons(1, &TBButtons[index]);
	}

	// save new (default) configuration 
	thePrefs.SetToolbarSettings(config);

	Localize();		// we have to localize the button-text

	theApp.emuledlg->ShowConnectionState();

	ChangeTextLabelStyle(thePrefs.GetToolbarLabelSettings(), false, true);
	SetAllButtonsWidth();	// then calc and set the button width
	AutoSize();
}

void CMuleToolbarCtrl::OnTbnInitCustomize(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = TBNRF_HIDEHELP;
}

void CMuleToolbarCtrl::OnTbnEndAdjust(NMHDR*, LRESULT* pResult)
{
	UpdateIdealSize();
	*pResult = 0; // return value is ignored
}

void CMuleToolbarCtrl::OnSysColorChange()
{
	CToolBarCtrl::OnSysColorChange();
	ChangeToolbarBitmap(thePrefs.GetToolbarBitmapSettings(), true);
}

void CMuleToolbarCtrl::PressMuleButton(int nID)
{
	// Customization might splits up the button-group, so we have to (un-)press them on our own
	if (m_iLastPressedButton != -1)
		CheckButton(m_iLastPressedButton, FALSE);
	CheckButton(nID, TRUE);
	m_iLastPressedButton = nID;
}

void CMuleToolbarCtrl::UpdateIdealSize()
{
	if (theApp.emuledlg->m_ctlMainTopReBar.m_hWnd)
	{
		// let the rebar know what's our new current ideal size, so the chevron is handled correctly..
		CSize sizeBar;
		GetMaxSize(&sizeBar);
		ASSERT( sizeBar.cx != 0 && sizeBar.cy != 0 );

	    REBARBANDINFO rbbi = {0};
	    rbbi.cbSize = sizeof(rbbi);
	    rbbi.fMask = RBBIM_IDEALSIZE;
		rbbi.cxIdeal = sizeBar.cx;
	    VERIFY( theApp.emuledlg->m_ctlMainTopReBar.SetBandInfo(MULE_TOOLBAR_BAND_NR, &rbbi) );
	}
}

void CMuleToolbarCtrl::ForceRecalcLayout()
{
	// Force a recalc of the toolbar's layout to work around a comctl bug
	int iTextRows = GetMaxTextRows();
	SetRedraw(FALSE);
	SetMaxTextRows(iTextRows+1);
	SetMaxTextRows(iTextRows);
	SetRedraw(TRUE);
}

#ifdef _DEBUG

void CMuleToolbarCtrl::Dump()
{
	TRACE("---\n");
	CRect rcWnd;
	GetWindowRect(&rcWnd);
	TRACE("Wnd =%4d,%4d-%4d,%4d (%4d x %4d)\n", rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom, rcWnd.Width(), rcWnd.Height());

	CRect rcClnt;
	GetClientRect(&rcClnt);
	TRACE("Clnt=%4d,%4d-%4d,%4d (%4d x %4d)\n", rcClnt.left, rcClnt.top, rcClnt.right, rcClnt.bottom, rcClnt.Width(), rcClnt.Height());

	// Total size of all of the visible buttons and separators in the toolbar.
	CSize siz;
	GetMaxSize(&siz);
	TRACE("MaxSize=                  %4d x %4d\n", siz.cx, siz.cy);

	int iButtons = GetButtonCount();	// Count of the buttons currently in the toolbar.
	int iRows = GetRows();				// Number of rows of buttons in a toolbar with the TBSTYLE_WRAPABLE style
	int iMaxTextRows = GetMaxTextRows();// Maximum number of text rows that can be displayed on a toolbar button.
	TRACE("ButtonCount=%d  Rows=%d  MaxTextRows=%d\n", iButtons, iRows, iMaxTextRows);

	// Current width and height of toolbar buttons, in pixels.
	DWORD dwButtonSize = GetButtonSize();
	TRACE("ButtonSize=%dx%d\n", LOWORD(dwButtonSize), HIWORD(dwButtonSize));

	// Padding for a toolbar control.
	DWORD dwPadding = SendMessage(TB_GETPADDING);
	TRACE("Padding=%dx%d\n", LOWORD(dwPadding), HIWORD(dwPadding));

	DWORD dwBitmapFlags = GetBitmapFlags(); // TBBF_LARGE=0x0001
	TRACE("BitmapFlags=%u\n", dwBitmapFlags);

	// Bounding rectangle of a button in a toolbar.
	TRACE("ItemRects:");
	for (int i = 0; i < iButtons; i++)
	{
		CRect rcButton(0,0,0,0);
		GetItemRect(i, &rcButton);
		TRACE(" %2dx%2d", rcButton.Width(), rcButton.Height());
	}
	TRACE("\n");

	// Bounding rectangle for a specified toolbar button.
	TRACE("Rects    :");
	for (int i = 0; i < iButtons; i++)
	{
		CRect rcButton(0,0,0,0);
		GetRect(IDC_TOOLBARBUTTON + i, &rcButton);
		TRACE(" %2dx%2d", rcButton.Width(), rcButton.Height());
	}
	TRACE("\n");

	TRACE("Info     :");
	for (int i = 0; i < iButtons; i++)
	{
		TCHAR szLabel[256];
		TBBUTTONINFO tbi = {0};
		tbi.cbSize = sizeof(tbi);
		tbi.dwMask |= TBIF_BYINDEX | TBIF_COMMAND | TBIF_IMAGE | TBIF_LPARAM | TBIF_SIZE | TBIF_STATE | TBIF_STYLE | TBIF_TEXT;
		tbi.cchText = ARRSIZE(szLabel);
		tbi.pszText = szLabel;
		GetButtonInfo(i, &tbi);
		TRACE(" %2d ", tbi.cx);
	}
	TRACE("\n");
}
#endif

void CMuleToolbarCtrl::AutoSize()
{
	CToolBarCtrl::AutoSize();
#ifdef _DEBUG
	//Dump();
#endif
}

void CMuleToolbarCtrl::SaveCurHeight()
{
	CRect rcWnd;
	GetWindowRect(&rcWnd);
	m_iPreviousHeight = rcWnd.Height();
}

void CMuleToolbarCtrl::UpdateBackground()
{
	if (theApp.emuledlg->m_ctlMainTopReBar)
	{
		HBITMAP hbmp = theApp.LoadImage(_T("MainToolBarBk"), _T("BMP"));
		if (hbmp)
		{
			REBARBANDINFO rbbi = {0};
			rbbi.cbSize = sizeof(rbbi);
			rbbi.fMask = RBBIM_STYLE;
			if (theApp.emuledlg->m_ctlMainTopReBar.GetBandInfo(MULE_TOOLBAR_BAND_NR, &rbbi))
			{
				rbbi.fMask = RBBIM_STYLE | RBBIM_BACKGROUND;
				rbbi.fStyle |= RBBS_FIXEDBMP;
				rbbi.hbmBack = hbmp;
				if (theApp.emuledlg->m_ctlMainTopReBar.SetBandInfo(MULE_TOOLBAR_BAND_NR, &rbbi))
				{
					if (m_bmpBack.m_hObject)
						VERIFY( m_bmpBack.DeleteObject() );
					m_bmpBack.Attach(hbmp);
					hbmp = NULL;
				}
			}
			if (hbmp)
				VERIFY( DeleteObject(hbmp) );
		}
		else
		{
			REBARBANDINFO rbbi = {0};
			rbbi.cbSize = sizeof(rbbi);
			rbbi.fMask = RBBIM_STYLE;
			if (theApp.emuledlg->m_ctlMainTopReBar.GetBandInfo(MULE_TOOLBAR_BAND_NR, &rbbi))
			{
				rbbi.fMask = RBBIM_STYLE | RBBIM_BACKGROUND;
				rbbi.fStyle &= ~RBBS_FIXEDBMP;
				rbbi.hbmBack = NULL;
				if (theApp.emuledlg->m_ctlMainTopReBar.SetBandInfo(MULE_TOOLBAR_BAND_NR, &rbbi))
				{
					if (m_bmpBack.m_hObject)
						VERIFY( m_bmpBack.DeleteObject() );
				}
			}
		}
	}
}
