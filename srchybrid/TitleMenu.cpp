//this file is part of eMule
//Copyright (C)2002-2004 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "TitleMenu.h"
#include "emule.h"
#include "preferences.h"
#include "otherfunctions.h"
#include "CxImage/xImage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define MP_TITLE	0xFFFE
#define ICONSIZE	16

#define MIIM_STRING      0x00000040
#define MIIM_BITMAP      0x00000080
#define MIIM_FTYPE       0x00000100
#define HBMMENU_CALLBACK ((HBITMAP) -1)

#define MIM_STYLE           0x00000010
#define MNS_CHECKORBMP      0x04000000

typedef struct tagMENUITEMINFOWEX
{
    UINT     cbSize;
    UINT     fMask;
    UINT     fType;         // used if MIIM_TYPE (4.0) or MIIM_FTYPE (>4.0)
    UINT     fState;        // used if MIIM_STATE
    UINT     wID;           // used if MIIM_ID
    HMENU    hSubMenu;      // used if MIIM_SUBMENU
    HBITMAP  hbmpChecked;   // used if MIIM_CHECKMARKS
    HBITMAP  hbmpUnchecked; // used if MIIM_CHECKMARKS
    ULONG_PTR dwItemData;   // used if MIIM_DATA
    LPWSTR   dwTypeData;    // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    UINT     cch;           // used if MIIM_TYPE (4.0) or MIIM_STRING (>4.0)
    HBITMAP  hbmpItem;      // used if MIIM_BITMAP
}   MENUITEMINFOEX, FAR *LPMENUITEMINFOEX;

TSetMenuInfo	CTitleMenu::SetMenuInfo;
TGetMenuInfo	CTitleMenu::GetMenuInfo;
HMODULE			CTitleMenu::m_hUSER32_DLL;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CTitleMenu::CTitleMenu()
{
	clLeft=::GetSysColor(COLOR_ACTIVECAPTION);
	clRight=::GetSysColor(27);  //COLOR_GRADIENTACTIVECAPTION
	clText=::GetSysColor(COLOR_CAPTIONTEXT);
	
	hinst_msimg32 = LoadLibrary(_T("msimg32.dll"));
	m_bCanDoGradientFill = FALSE;
	
	if(hinst_msimg32)
	{
		m_bCanDoGradientFill = TRUE;		
		dllfunc_GradientFill = ((LPFNDLLFUNC1) GetProcAddress(hinst_msimg32, "GradientFill"));
	}
	bDrawEdge=false;
	flag_edge=BDR_SUNKENINNER;
	m_bIconMenu = false;
	m_mapIconPos.InitHashTable(29);
}

CTitleMenu::~CTitleMenu()
{
	FreeLibrary( hinst_msimg32 );
}

void CTitleMenu::Init(){
	m_hUSER32_DLL = 0;
	SetMenuInfo = NULL;
	GetMenuInfo = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// CTitleMenu message handlers


//
// This function adds a title entry to a popup menu
//
void CTitleMenu::AddMenuTitle(LPCTSTR lpszTitle, bool bIsIconMenu)
{
	// insert an empty owner-draw item at top to serve as the title
	// note: item is not selectable (disabled) but not grayed
	if (lpszTitle != NULL){
		m_strTitle = lpszTitle;
		m_strTitle.Replace(_T("&"), _T(""));
		InsertMenu(0, MF_BYPOSITION | MF_OWNERDRAW | MF_STRING | MF_DISABLED, MP_TITLE);
	}
	if (bIsIconMenu && (thePrefs.GetWindowsVersion() == _WINVER_XP_ || thePrefs.GetWindowsVersion() == _WINVER_2K_) ){
		m_bIconMenu = true;
		m_ImageList.DeleteImageList();
		m_ImageList.Create(ICONSIZE,ICONSIZE,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
		m_ImageList.SetBkColor(CLR_NONE);
		if (LoadAPI()){
			MENUINFO mi;
			mi.fMask = MIM_STYLE;
			mi.cbSize = sizeof(mi);
			GetMenuInfo(m_hMenu, &mi);
			mi.dwStyle |= MNS_CHECKORBMP;
			SetMenuInfo(m_hMenu, &mi);
		}
	}
}

void CTitleMenu::MeasureItem(LPMEASUREITEMSTRUCT mi)
{
	if (mi->itemID == MP_TITLE){ 
		// get the screen dc to use for retrieving size information
		CDC dc;
		dc.Attach(::GetDC(NULL));
		// select the title font
		HFONT hfontOld = (HFONT)SelectObject(dc.m_hDC, (HFONT)theApp.m_fontDefaultBold);
		// compute the size of the title
		CSize size = dc.GetTextExtent(m_strTitle);
		// deselect the title font
		::SelectObject(dc.m_hDC, hfontOld);
		// add in the left margin for the menu item
		size.cx += GetSystemMetrics(SM_CXMENUCHECK)+8;


		//Return the width and height
		//+ include space for border
		const int nBorderSize = 2;
		mi->itemWidth = size.cx + nBorderSize;
		mi->itemHeight = size.cy + nBorderSize;
		
		// cleanup
		::ReleaseDC(NULL, dc.Detach());
	}
	else{
		CMenu::MeasureItem(mi);
		if (m_bIconMenu){
			mi->itemHeight = max(mi->itemHeight, 16);
			mi->itemWidth += 18;
		}
	}
}


void CTitleMenu::DrawItem(LPDRAWITEMSTRUCT di)
{
	if (di->itemID == MP_TITLE){
		COLORREF crOldBk = ::SetBkColor(di->hDC, clLeft);
		
		if(m_bCanDoGradientFill&&(clLeft!=clRight))
		{
 			TRIVERTEX rcVertex[2];
			di->rcItem.right--; // exclude this point, like FillRect does 
			di->rcItem.bottom--;
			rcVertex[0].x=di->rcItem.left;
			rcVertex[0].y=di->rcItem.top;
			rcVertex[0].Red=GetRValue(clLeft)<<8;	// color values from 0x0000 to 0xff00 !!!!
			rcVertex[0].Green=GetGValue(clLeft)<<8;
			rcVertex[0].Blue=GetBValue(clLeft)<<8;
			rcVertex[0].Alpha=0x0000;
			rcVertex[1].x=di->rcItem.right; 
			rcVertex[1].y=di->rcItem.bottom;
			rcVertex[1].Red=GetRValue(clRight)<<8;
			rcVertex[1].Green=GetGValue(clRight)<<8;
			rcVertex[1].Blue=GetBValue(clRight)<<8;
			rcVertex[1].Alpha=0;
			GRADIENT_RECT rect;
			rect.UpperLeft=0;
			rect.LowerRight=1;
			
			// fill the area 
			GradientFill( di->hDC,rcVertex,2,&rect,1,GRADIENT_FILL_RECT_H);
		}
		else
		{
			::ExtTextOut(di->hDC, 0, 0, ETO_OPAQUE, &di->rcItem, NULL, 0, NULL);
		}
		if(bDrawEdge)
			::DrawEdge(di->hDC, &di->rcItem, flag_edge, BF_RECT);
	 

		int modeOld = ::SetBkMode(di->hDC, TRANSPARENT);
		COLORREF crOld = ::SetTextColor(di->hDC, clText);
		// select font into the dc
		HFONT hfontOld = (HFONT)SelectObject(di->hDC, (HFONT)theApp.m_fontDefaultBold);

		// add the menu margin offset
		di->rcItem.left += GetSystemMetrics(SM_CXMENUCHECK)+8;

		// draw the text left aligned and vertically centered
		::DrawText(di->hDC, m_strTitle, -1, &di->rcItem, DT_SINGLELINE|DT_VCENTER|DT_LEFT);

		//Restore font and colors...
		::SelectObject(di->hDC, hfontOld);
		::SetBkMode(di->hDC, modeOld);
		::SetBkColor(di->hDC, crOldBk);
		::SetTextColor(di->hDC, crOld);
	}
	else{
		CDC* dc = CDC::FromHandle(di->hDC);
		int posY = di->rcItem.top + ((di->rcItem.bottom-di->rcItem.top)-ICONSIZE) / 2;
		int nIconPos;
		if (!m_mapIconPos.Lookup(di->itemID, nIconPos))
			return;

		if ((di->itemState & ODS_GRAYED) != 0)	
		{
			DrawMonoIcon(nIconPos, CPoint(di->rcItem.left,posY), dc);
			return;
		}

		// Draw the bitmap on the menu.
		m_ImageList.Draw(dc, nIconPos, CPoint(di->rcItem.left,posY), ILD_TRANSPARENT);
	}
}

BOOL CTitleMenu::GradientFill(HDC hdc, PTRIVERTEX pVertex, DWORD dwNumVertex, PVOID pMesh, DWORD dwNumMesh, DWORD dwMode)
{
	ASSERT(m_bCanDoGradientFill); 
	return dllfunc_GradientFill(hdc,pVertex,dwNumVertex,pMesh,dwNumMesh,dwMode); 
}

BOOL CTitleMenu::AppendMenu(UINT nFlags, UINT_PTR nIDNewItem, LPCTSTR lpszNewItem, LPCTSTR lpszIconName){
	bool bResult = CMenu::AppendMenu(nFlags, nIDNewItem, lpszNewItem);
	if (!m_bIconMenu || (nFlags & MF_SEPARATOR) != 0 || !(thePrefs.GetWindowsVersion() == _WINVER_XP_ || thePrefs.GetWindowsVersion() == _WINVER_2K_) ){
		if (m_bIconMenu && lpszIconName != NULL)
			ASSERT( false );
		return bResult;
	}

	// Those MFC warnings which are thrown when one opens certain context menus 
	// are because of sub menu items. All the IDs shown in the warnings are sub 
	// menu handles! Seems to be a bug in MFC. Look at '_AfxFindPopupMenuFromID'.
	// ---
	// Warning: unknown WM_MEASUREITEM for menu item 0x530601.
	// Warning: unknown WM_MEASUREITEM for menu item 0x4305E7.
	// ---
	//if (nFlags & MF_POPUP)
	//	TRACE(_T("TitleMenu: adding popup menu item id=%x  str=%s\n"), nIDNewItem, lpszNewItem);

	MENUITEMINFOEX info;
	ZeroMemory(&info, sizeof(info));
	info.fMask = MIIM_BITMAP;
	info.hbmpItem = HBMMENU_CALLBACK;
	info.cbSize = sizeof(info);
	VERIFY( SetMenuItemInfo(nIDNewItem, (MENUITEMINFO*)&info, FALSE) ); 
	
	if (lpszIconName != NULL){
		int nPos = m_ImageList.Add(CTempIconLoader(lpszIconName));
		if (nPos == (-1))
			ASSERT( false );
		else
			m_mapIconPos.SetAt(nIDNewItem, nPos);
	}
	return bResult;
}

void CTitleMenu::DrawMonoIcon(int nIconPos, CPoint nDrawPos, CDC *dc){
	CWindowDC windowDC(0);
	CDC colorDC;
	colorDC.CreateCompatibleDC(0);
	CBitmap bmpColor;
	bmpColor.CreateCompatibleBitmap(&windowDC, ICONSIZE, ICONSIZE);
	CBitmap* bmpOldColor = colorDC.SelectObject(&bmpColor);
	colorDC.FillSolidRect(0, 0, ICONSIZE, ICONSIZE, dc->GetBkColor());
	CxImage imgBk, imgGray;
	imgBk.CreateFromHBITMAP((HBITMAP)bmpColor);
	m_ImageList.Draw(&colorDC, nIconPos, CPoint(0,0), ILD_TRANSPARENT);
	imgGray.CreateFromHBITMAP((HBITMAP)bmpColor);
	if (imgGray.IsValid() && imgBk.IsValid()){
		imgGray.GrayScale();
		imgBk.GrayScale();
		imgGray.SetTransIndex(imgGray.GetNearestIndex(imgBk.GetPixelColor(0,0)));
		imgGray.Draw((HDC)*dc,nDrawPos.x,nDrawPos.y);
	}
	colorDC.SelectObject(bmpOldColor);
	colorDC.DeleteDC();
	bmpColor.DeleteObject();
}

bool CTitleMenu::LoadAPI(){
	if (m_hUSER32_DLL == 0)
		m_hUSER32_DLL = LoadLibrary(_T("User32.dll"));
    if (m_hUSER32_DLL == 0) {
        return false;
    }

	bool bSucceeded = true;
	bSucceeded = bSucceeded && (SetMenuInfo != NULL || (SetMenuInfo = (TSetMenuInfo) GetProcAddress(m_hUSER32_DLL,"SetMenuInfo")) != NULL);
	bSucceeded = bSucceeded && (GetMenuInfo != NULL || (GetMenuInfo = (TGetMenuInfo) GetProcAddress(m_hUSER32_DLL,"GetMenuInfo")) != NULL);

	if (!bSucceeded){
		FreeAPI();
		return false;
	}
	return true;
}

void CTitleMenu::FreeAPI(){
	if (m_hUSER32_DLL != 0){
		FreeLibrary(m_hUSER32_DLL);
		m_hUSER32_DLL = 0;
	}
	SetMenuInfo = NULL;
	GetMenuInfo = NULL;
}