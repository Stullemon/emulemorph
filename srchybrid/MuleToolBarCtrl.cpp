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
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#if (_WIN32_IE < 0x0500)
#define TBN_INITCUSTOMIZE       (TBN_FIRST - 23)
#define    TBNRF_HIDEHELP       0x00000001
#endif

#define	EMULTB_BASEEXT		_T("eMuleToolbar.kad02")
#define	EMULSKIN_BASEEXT	_T("eMuleSkin")

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

static int _iPreviousHeight = 0;

// CMuleToolbarCtrl

IMPLEMENT_DYNAMIC(CMuleToolbarCtrl, CToolBarCtrl)
CMuleToolbarCtrl::CMuleToolbarCtrl()
{
	m_iLastPressedButton = -1;
	m_buttoncount=0;
	memset(TBButtons, 0, sizeof(TBButtons));
	memset(TBStrings, 0, sizeof(TBStrings));
	m_iToolbarLabelSettings = 0;
}

CMuleToolbarCtrl::~CMuleToolbarCtrl()
{
}


BEGIN_MESSAGE_MAP(CMuleToolbarCtrl, CToolBarCtrl)
	ON_WM_SIZE()
	ON_NOTIFY_REFLECT(NM_RCLICK, OnNMRclick)
	ON_NOTIFY_REFLECT(TBN_QUERYDELETE, OnTbnQueryDelete)
	ON_NOTIFY_REFLECT(TBN_QUERYINSERT, OnTbnQueryInsert)
	ON_NOTIFY_REFLECT(TBN_GETBUTTONINFO, OnTbnGetButtonInfo)
	ON_NOTIFY_REFLECT(TBN_TOOLBARCHANGE, OnTbnToolbarChange)
	ON_NOTIFY_REFLECT(TBN_RESET, OnTbnReset)
	ON_NOTIFY_REFLECT(TBN_INITCUSTOMIZE, OnTbnInitCustomize)
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()


void CMuleToolbarCtrl::Init(void)
{
	bitmappaths.RemoveAll();

	ModifyStyle(0, TBSTYLE_FLAT | CCS_ADJUSTABLE | TBSTYLE_TRANSPARENT | CCS_NODIVIDER);
	ChangeToolbarBitmap(thePrefs.GetToolbarBitmapSettings(), false);
	// add button-text:
	TCHAR cButtonStrings[2000];
	int lLen, lLen2;
	m_buttoncount=0;
	
	_tcscpy(cButtonStrings, GetResString(IDS_MAIN_BTN_CONNECT));
	lLen = _tcslen(GetResString(IDS_MAIN_BTN_CONNECT)) + 1;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_KADEMLIA)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_KADEMLIA), lLen2);
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_SERVER)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_SERVER), lLen2);
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_TRANS)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_TRANS), lLen2);
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_SEARCH)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_SEARCH), lLen2);
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_FILES)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_FILES), lLen2);
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_MESSAGES)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_MESSAGES), lLen2);
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_IRC)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_IRC), lLen2);
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_STATISTIC)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_STATISTIC), lLen2);
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_PREFS)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_PREFS), lLen2);
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_TOOLS)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_TOOLS), lLen2);
	lLen += lLen2;
	++m_buttoncount;

	lLen2 = _tcslen(GetResString(IDS_EM_HELP)) + 1;
	memcpy(cButtonStrings+lLen, GetResString(IDS_EM_HELP), lLen2);
	lLen += lLen2;
	++m_buttoncount;

	// terminate
	memcpy(cButtonStrings+lLen, "\0", 1);

	AddStrings(cButtonStrings);

	// initialize buttons:	
	for(int i = 0; i < m_buttoncount; i++)
	{		
		TBButtons[i].fsState	= TBSTATE_ENABLED;
		TBButtons[i].fsStyle	= TBSTYLE_CHECKGROUP;
		TBButtons[i].idCommand	= IDC_TOOLBARBUTTON + i;
		TBButtons[i].iString	= i;

		switch(i)
		{
		case 0:
		case 9:
		case 10:
		case 11:
			TBButtons[i].fsStyle = TBSTYLE_BUTTON;
			break;
		}
	}

	// set button image indices
	int iBitmap = 0;
	for(int i = 0; i < m_buttoncount; i++)
	{		
		TBButtons[i].iBitmap = iBitmap;
		if (i == 0) // 'Connect' button has 3 states
			iBitmap += 3;
		else
			iBitmap += 1;
	}
	
	TBBUTTON sepButton;
	sepButton.idCommand = 0;
	sepButton.fsStyle = TBSTYLE_SEP;
	sepButton.fsState = TBSTATE_ENABLED;
	sepButton.iString = -1;
	sepButton.iBitmap = -1;
	
	int iAddedButtons = 0;
	CString config = thePrefs.GetToolbarSettings();
	for(i=0;i<config.GetLength();i+=2)
	{
		int index = _tstoi(config.Mid(i,2));
		if(index==99)
		{
			AddButtons(1,&sepButton);
			continue;
		}
		AddButtons(1,&TBButtons[index]);
		iAddedButtons++;
	}

	// recalc toolbar-size:	
	Localize();		// at first we have to localize the button-text!!!
	m_iToolbarLabelSettings=4;
	ChangeTextLabelStyle(thePrefs.GetToolbarLabelSettings(), false);
	SetBtnWidth();		// then calc and set the button width
	AutoSize();		// and finally call the original (but maybe obsolete) function
	
	// add speed-meter to upper-right corner
	CRect rClient;
	GetClientRect(&rClient);
	rClient.DeflateRect(7,7);
	int iHeight = rClient.Height();
	rClient.left = rClient.right - iHeight * 2;

	// resize speed-meter	
	CRect rcWnd;
	GetWindowRect(&rcWnd);
	OnSize(0, rcWnd.Width(), rcWnd.Height());

	theApp.emuledlg->SetKadButtonState();

	GetWindowRect(&rcWnd);
	_iPreviousHeight = rcWnd.Height();
}

void CMuleToolbarCtrl::Localize(void)
{	
	if(m_hWnd)
	{
		static const int TBStringIDs[]={
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
		tbi.cbSize = sizeof (TBBUTTONINFO);
		CString buffer;
		
		if(theApp.serverconnect->IsConnected())
			buffer = GetResString(IDS_MAIN_BTN_DISCONNECT);
		else if(theApp.serverconnect->IsConnecting())
			buffer = GetResString(IDS_MAIN_BTN_CANCEL);
		else
			buffer = GetResString(IDS_MAIN_BTN_CONNECT);

		_stprintf(TBStrings[0], _T("%s"), buffer);
		tbi.pszText = TBStrings[0];
		SetButtonInfo(IDC_TOOLBARBUTTON+0, &tbi);

		for (int i = 1; i < m_buttoncount; i++)
		{
			buffer = GetResString(TBStringIDs[i-1]); // EC
			_stprintf(TBStrings[i], _T("%s"), buffer);
			tbi.pszText = TBStrings[i];
			SetButtonInfo(IDC_TOOLBARBUTTON+i, &tbi);
		}

		AutoSize();
		SetBtnWidth();
	}
}

void CMuleToolbarCtrl::OnSize(UINT nType, int cx, int cy)
{
	CToolBarCtrl::OnSize(nType, cx, cy);
		
	SetBtnWidth();
	AutoSize();
}

void CMuleToolbarCtrl::SetBtnWidth()
{
	if(m_iToolbarLabelSettings==1)
	{
		CDC *pDC = GetDC();
		CFont *pFnt = GetFont();
		CFont *pOldFnt = pDC->SelectObject(pFnt);
		CRect r(0,0,0,0);

		// calculate the max. possible button-size
		int iCalcSize = 0;

		for(int i = 0; i < m_buttoncount ; i++)
			if(!IsButtonHidden(IDC_TOOLBARBUTTON+i))
			{
				pDC->DrawText(TBStrings[i], -1, r, DT_SINGLELINE | DT_CALCRECT);
 				if (r.Width() > iCalcSize)
					iCalcSize = r.Width();
			}

		iCalcSize += 10;

		pDC->SelectObject(pOldFnt);
		ReleaseDC(pDC); // FoRcHa

		GetClientRect(&r);
		int iMaxPossible = r.Width() / GetButtonCount();

		// if the buttons are to big, reduze their size
		if(iCalcSize > iMaxPossible)
			iCalcSize = iMaxPossible;

		SetButtonWidth(iCalcSize, iCalcSize);
	}
	else
	{
		SetButtonSize(CSize(0,0));
		SetButtonWidth(0,0);
	}
}

void CMuleToolbarCtrl::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (GetKeyState(VK_CONTROL) & 0x8000)
	{
		if (!thePrefs.GetToolbarBitmapSettings().IsEmpty())
			ChangeToolbarBitmap(thePrefs.GetToolbarBitmapSettings(), true);
		if (!CString(thePrefs.GetSkinProfile()).IsEmpty())
			theApp.ApplySkin(thePrefs.GetSkinProfile());

		*pResult = TRUE;
		return;
	}

	POINT point;
	GetCursorPos (&point);
	
	CMenu m_ToolbarMenu;
	m_ToolbarMenu.CreatePopupMenu();

	CMenu m_BitmapsMenu;
	m_BitmapsMenu.CreateMenu();
	m_BitmapsMenu.AppendMenu(MF_STRING,MP_SELECTTOOLBARBITMAP, GetResString(IDS_SELECTTOOLBARBITMAP));
	m_BitmapsMenu.AppendMenu(MF_STRING,MP_SELECTTOOLBARBITMAPDIR, GetResString(IDS_SELECTTOOLBARBITMAPDIR));
	m_BitmapsMenu.AppendMenu(MF_SEPARATOR);
	m_BitmapsMenu.AppendMenu(MF_STRING,MP_TOOLBARBITMAP,GetResString(IDS_DEFAULT));
	bitmappaths.RemoveAll();
	CString currentBitmapSettings = thePrefs.GetToolbarBitmapSettings();
	bool checked=false;
	if(currentBitmapSettings=="")
	{
		m_BitmapsMenu.CheckMenuItem(MP_TOOLBARBITMAP,MF_CHECKED);
		m_BitmapsMenu.EnableMenuItem(MP_TOOLBARBITMAP,MF_DISABLED);
		checked=true;
	}
	bitmappaths.Add(_T(""));
	int i = 1;
	if (!thePrefs.GetToolbarBitmapFolderSettings().IsEmpty())
	{
		for (int f = 0; f < ARRSIZE(_apszTBFiles); f++)
		{
			bool bFinished = false;
			WIN32_FIND_DATA FileData;
			HANDLE hSearch = FindFirstFile(thePrefs.GetToolbarBitmapFolderSettings() + CString(_T("\\")) + _apszTBFiles[f], &FileData);
			if (hSearch == INVALID_HANDLE_VALUE)
				bFinished = true;
			for (/**/; !bFinished && i < 50; i++)
			{
				CString bitmapFileName = FileData.cFileName;
				CString bitmapBaseName;
				int iExt = bitmapFileName.Find(EMULTB_BASEEXT);
				if (iExt > 0)
					bitmapBaseName = bitmapFileName.Left(iExt - 1);
				else
					bitmapBaseName = bitmapFileName;
				m_BitmapsMenu.AppendMenu(MF_STRING, MP_TOOLBARBITMAP + i, bitmapBaseName);
				bitmappaths.Add(thePrefs.GetToolbarBitmapFolderSettings() + CString(_T("\\")) + bitmapFileName);
				if (!checked && currentBitmapSettings == bitmappaths[i])
				{
					m_BitmapsMenu.CheckMenuItem(MP_TOOLBARBITMAP+i, MF_CHECKED);
					m_BitmapsMenu.EnableMenuItem(MP_TOOLBARBITMAP+i, MF_DISABLED);
					checked = true;
				}
				if (!FindNextFile(hSearch, &FileData))
					bFinished = true;
			}
			FindClose(hSearch);
		}
	}
	if(!checked)
	{
		m_BitmapsMenu.AppendMenu(MF_STRING,MP_TOOLBARBITMAP+i,currentBitmapSettings);
		m_BitmapsMenu.CheckMenuItem(MP_TOOLBARBITMAP+i,MF_CHECKED);
		m_BitmapsMenu.EnableMenuItem(MP_TOOLBARBITMAP+i,MF_DISABLED);
		bitmappaths.Add(currentBitmapSettings);
	}
	m_ToolbarMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_BitmapsMenu.m_hMenu, GetResString(IDS_TOOLBARSKINS));

	CMenu m_SkinsMenu;
	m_SkinsMenu.CreateMenu();
	m_SkinsMenu.AppendMenu(MF_STRING,MP_SELECT_SKIN_FILE, GetResString(IDS_SEL_SKIN));
	m_SkinsMenu.AppendMenu(MF_STRING,MP_SELECT_SKIN_DIR, GetResString(IDS_SEL_SKINDIR));
	m_SkinsMenu.AppendMenu(MF_SEPARATOR);
	m_SkinsMenu.AppendMenu(MF_STRING,MP_SKIN_PROFILE,GetResString(IDS_DEFAULT));
	aSkinPaths.RemoveAll();
	CString currentSkin = thePrefs.GetSkinProfile();
	checked=false;
	if(currentSkin=="")
	{
		m_SkinsMenu.CheckMenuItem(MP_SKIN_PROFILE,MF_CHECKED);
		m_SkinsMenu.EnableMenuItem(MP_SKIN_PROFILE,MF_DISABLED);
		checked=true;
	}
	aSkinPaths.Add(_T(""));
	i = 1;
	if (!thePrefs.GetSkinProfileDir().IsEmpty())
	{
		for (int f = 0; f < ARRSIZE(_apszSkinFiles); f++)
		{
			bool bFinished = false;
			WIN32_FIND_DATA FileData;
			HANDLE hSearch = FindFirstFile(thePrefs.GetSkinProfileDir() + CString(_T("\\")) + _apszSkinFiles[f], &FileData);
			if (hSearch == INVALID_HANDLE_VALUE)
				bFinished = true;
			for (/**/; !bFinished && i < 50; i++)
			{
				CString skinFileName = FileData.cFileName;
				CString skinBaseName;
				int iExt = skinFileName.Find(_T(".") EMULSKIN_BASEEXT _T(".ini"));
				if (iExt > 0)
					skinBaseName = skinFileName.Left(iExt);
				else
					skinBaseName = skinFileName;
				m_SkinsMenu.AppendMenu(MF_STRING, MP_SKIN_PROFILE + i, skinBaseName);
				aSkinPaths.Add(thePrefs.GetSkinProfileDir() + CString(_T("\\")) + skinFileName);
				if (!checked && currentSkin == aSkinPaths[i])
				{
					m_SkinsMenu.CheckMenuItem(MP_SKIN_PROFILE + i, MF_CHECKED);
					m_SkinsMenu.EnableMenuItem(MP_SKIN_PROFILE + i, MF_DISABLED);
					checked = true;
				}
				if (!FindNextFile(hSearch, &FileData))
					bFinished = true;
			}
			FindClose(hSearch);
		}
	}
	if(!checked)
	{
		m_SkinsMenu.AppendMenu(MF_STRING, MP_SKIN_PROFILE + i, currentSkin);
		m_SkinsMenu.CheckMenuItem(MP_SKIN_PROFILE + i, MF_CHECKED);
		m_SkinsMenu.EnableMenuItem(MP_SKIN_PROFILE + i, MF_DISABLED);
		aSkinPaths.Add(currentSkin);
	}
	m_ToolbarMenu.AppendMenu(MF_STRING | MF_POPUP, (UINT_PTR)m_SkinsMenu.m_hMenu, GetResString(IDS_SKIN_PROF));
	
	CMenu m_TextLabelsMenu;
	m_TextLabelsMenu.CreateMenu();
	m_TextLabelsMenu.AppendMenu(MF_STRING,MP_NOTEXTLABELS, GetResString(IDS_NOTEXTLABELS));
	m_TextLabelsMenu.AppendMenu(MF_STRING,MP_TEXTLABELS,GetResString(IDS_ENABLETEXTLABELS));
	m_TextLabelsMenu.AppendMenu(MF_STRING,MP_TEXTLABELSONRIGHT,GetResString(IDS_TEXTLABELSONRIGHT));
	m_TextLabelsMenu.CheckMenuItem(thePrefs.GetToolbarLabelSettings(),MF_BYPOSITION|MF_CHECKED);
	m_TextLabelsMenu.EnableMenuItem(thePrefs.GetToolbarLabelSettings(),MF_BYPOSITION|MF_DISABLED);
	m_ToolbarMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_TextLabelsMenu.m_hMenu, GetResString(IDS_TEXTLABELS));
	m_ToolbarMenu.AppendMenu(MF_STRING,MP_CUSTOMIZETOOLBAR, GetResString(IDS_CUSTOMIZETOOLBAR));
	m_ToolbarMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);

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
		*pResult = TRUE;
	}
}

void CMuleToolbarCtrl::OnTbnToolbarChange(NMHDR *pNMHDR, LRESULT *pResult)
{
	TBBUTTON buttoninfo;
	CString config;
	CString buffer;

	for(int i=0;i<GetButtonCount();i++)
		if(GetButton(i, &buttoninfo))
		{
			buffer.Format("%02i", (buttoninfo.idCommand!=0)?buttoninfo.idCommand-IDC_TOOLBARBUTTON:99);
			config.Append(buffer);
		}

	thePrefs.SetToolbarSettings(config);
	Localize();

	theApp.emuledlg->ShowConnectionState();

	SetBtnWidth();
	AutoSize();

	*pResult = 0;
}

void CMuleToolbarCtrl::ChangeToolbarBitmap(CString path, bool refresh)
{
	bool bResult = false;
	CImageList ImageList;
	CEnBitmap Bitmap;
	if (!path.IsEmpty() && Bitmap.LoadImage(path))
	{
		BITMAP bm = {0};
		Bitmap.GetObject(sizeof(bm), &bm);
		bool bAlpha = bm.bmBitsPixel > 24;
		if (ImageList.Create(32, bm.bmHeight, bAlpha ? ILC_COLOR32 : (theApp.m_iDfltImageListColorFlags | ILC_MASK), 0, 1))
		{
			ImageList.Add(&Bitmap,  bAlpha ? 0xFF000000 : RGB(255, 0, 255));
			CImageList* pimlOld = SetImageList(&ImageList);
			ImageList.Detach();
			if (pimlOld)
				pimlOld->DeleteImageList();
			bResult = true;
		}
		Bitmap.DeleteObject();
	}

	// if image file loading or image list creation failed, create default image list.
	if (!bResult)
	{
		// load from icon ressources
		ImageList.Create(32, 32, theApp.m_iDfltImageListColorFlags | ILC_MASK, 0, 1);
		ImageList.Add(CTempIconLoader("BN_CONNECT", 32, 32));
		ImageList.Add(CTempIconLoader("BN_DISCONNECT", 32, 32));
		ImageList.Add(CTempIconLoader("BN_STOPCONNECTING", 32, 32));
		ImageList.Add(CTempIconLoader("BN_KADEMLIA", 32, 32));
		ImageList.Add(CTempIconLoader("BN_SERVER", 32, 32));
		ImageList.Add(CTempIconLoader("BN_DOWNLOAD", 32, 32));
		ImageList.Add(CTempIconLoader("BN_SEARCH", 32, 32));
		ImageList.Add(CTempIconLoader("BN_FILES", 32, 32));
		ImageList.Add(CTempIconLoader("BN_MESSAGES", 32, 32));
		ImageList.Add(CTempIconLoader("BN_IRC", 32, 32));
		ImageList.Add(CTempIconLoader("BN_STATISTICS", 32, 32));
		ImageList.Add(CTempIconLoader("BN_PREFERENCES", 32, 32));
		ImageList.Add(CTempIconLoader("BN_TOOLS", 32, 32));
		ImageList.Add(CTempIconLoader("BN_HELP", 32, 32));
		CImageList* pimlOld = SetImageList(&ImageList);
		ImageList.Detach();
		if (pimlOld)
			pimlOld->DeleteImageList();
	}

	if (refresh)
		Refresh();
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

		case MP_NOTEXTLABELS:
			ChangeTextLabelStyle(0,TRUE);
			thePrefs.SetToolbarLabelSettings(0);
			break;

		case MP_TEXTLABELS:
			ChangeTextLabelStyle(1,TRUE);
			thePrefs.SetToolbarLabelSettings(1);
			break;

		case MP_TEXTLABELSONRIGHT:
			ChangeTextLabelStyle(2,TRUE);
			thePrefs.SetToolbarLabelSettings(2);
			break;

		case MP_SELECT_SKIN_DIR:{
			TCHAR buffer[MAX_PATH];
			_sntprintf(buffer,ARRSIZE(buffer),_T("%s"), thePrefs.GetSkinProfileDir());
			if(SelectDir(m_hWnd, buffer, "Select skin profile directory"))
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
			if (IDOK == dialog.DoModal())
			{
				if(thePrefs.GetSkinProfile()!=dialog.GetPathName())
					theApp.ApplySkin(dialog.GetPathName());
			}
			break;
		}

		default:
			if(wParam >= MP_TOOLBARBITMAP && wParam < MP_TOOLBARBITMAP + 50)
			{
				if(thePrefs.GetToolbarBitmapSettings()!=bitmappaths[wParam-MP_TOOLBARBITMAP])
				{
					ChangeToolbarBitmap(bitmappaths[wParam-MP_TOOLBARBITMAP], true);
					thePrefs.SetToolbarBitmapSettings(bitmappaths[wParam-MP_TOOLBARBITMAP]);
				}
			}
			else if (wParam >= MP_SKIN_PROFILE && wParam < MP_SKIN_PROFILE + 50)
			{
				if (thePrefs.GetSkinProfile() != aSkinPaths[wParam - MP_SKIN_PROFILE])
					theApp.ApplySkin(aSkinPaths[wParam - MP_SKIN_PROFILE]);
			}
	}

	return true;
}

void CMuleToolbarCtrl::ChangeTextLabelStyle(int settings, bool refresh)
{
	if(m_iToolbarLabelSettings!=settings)
	{
		switch(settings)
		{
			case 0:
				SetStyle(GetStyle() & ~TBSTYLE_LIST);
				SetMaxTextRows(0);
				break;
			case 1:
				SetStyle(GetStyle() & ~TBSTYLE_LIST);
				SetMaxTextRows(1);
				break;
			case 2:
				SetStyle(GetStyle() | TBSTYLE_LIST);
				SetMaxTextRows(1);
				break;
		}
		if((m_iToolbarLabelSettings+settings)!=2) //if not changing between no labels and labels on right 
		{
			for(int i = 0; i < m_buttoncount; i++)
			{	
				TBBUTTONINFO buttonInfo;
				buttonInfo.cbSize=sizeof(buttonInfo);
				buttonInfo.dwMask=TBIF_STYLE;
				GetButtonInfo(IDC_TOOLBARBUTTON + i, &buttonInfo);
				if(settings==1)
					buttonInfo.fsStyle &= ~TBSTYLE_AUTOSIZE;
				else
					buttonInfo.fsStyle |= TBSTYLE_AUTOSIZE;
				SetButtonInfo(IDC_TOOLBARBUTTON + i, &buttonInfo);
			}
		}
		m_iToolbarLabelSettings=settings;
		if(refresh)
			Refresh();
	}
}

void CMuleToolbarCtrl::Refresh()
{
	SetBtnWidth();
	AutoSize();

	CRect rToolbarRect;
	GetWindowRect(&rToolbarRect);

	if (_iPreviousHeight == rToolbarRect.Height())
	{
		Invalidate();
		RedrawWindow();
	}
	else
	{
		_iPreviousHeight = rToolbarRect.Height();

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
	for ( int i = GetButtonCount()-1; i >= 0 ; i-- )
	{
		TBBUTTON Button;
		GetButton(i,&Button);
		for ( int j= 0; j <m_buttoncount ; j++ )
		{
			if ( TBButtons[j].idCommand == Button.idCommand )
			{
				TBButtons[j].fsState	=	Button.fsState;
				TBButtons[j].fsStyle	=	Button.fsStyle;
				TBButtons[j].iString	=	Button.iString;
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
	for(i=0;i<config.GetLength();i+=2)
	{
		int index = _tstoi(config.Mid(i,2));
		if(index==99)
		{
			AddButtons(1,&sepButton);
			continue;
		}
		AddButtons(1,&TBButtons[index]);
	}
	// save new (default) configuration 
	thePrefs.SetToolbarSettings(config.GetBuffer(256));
	config.ReleaseBuffer();

	Localize();		// we have to localize the button-text

	theApp.emuledlg->ShowConnectionState();

	m_iToolbarLabelSettings=4;
	ChangeTextLabelStyle(thePrefs.GetToolbarLabelSettings(), false);
	SetBtnWidth();		// then calc and set the button width
	AutoSize();		
}

void CMuleToolbarCtrl::OnTbnInitCustomize(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = TBNRF_HIDEHELP;
}

void CMuleToolbarCtrl::OnSysColorChange()
{
	CToolBarCtrl::OnSysColorChange();
	ChangeToolbarBitmap(thePrefs.GetToolbarBitmapSettings(), true);
}

void CMuleToolbarCtrl::ReloadConfig(){
	while (GetButtonCount() != 0)
		DeleteButton(0);
	Init();
}
