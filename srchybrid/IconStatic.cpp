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
#include "IconStatic.h"
#include "VisualStylesXP.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIconStatic

CIconStatic::CIconStatic()
{
	m_strText = "";
	m_pszIconID = NULL;
}

CIconStatic::~CIconStatic()
{
	m_MemBMP.DeleteObject();
}


BEGIN_MESSAGE_MAP(CIconStatic, CStatic)
	//{{AFX_MSG_MAP(CIconStatic)
		ON_WM_SYSCOLORCHANGE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten CIconStatic 
bool CIconStatic::Init(LPCTSTR pszIconID)
{
	m_pszIconID = pszIconID;

	CString strText;	
	GetWindowText(strText);
	SetWindowText(_T(""));
	if(!strText.IsEmpty())
		m_strText = strText;

	CRect rRect;
	GetClientRect(rRect);

	CDC *pDC = GetDC();
	CDC MemDC;
	CBitmap *pOldBMP;
	
	VERIFY( MemDC.CreateCompatibleDC(pDC) );

	CFont *pOldFont = MemDC.SelectObject(GetFont());

	CRect rCaption(0,0,0,0);
	MemDC.DrawText(m_strText, rCaption, DT_CALCRECT);
	ASSERT( rCaption.Width() >= 0 );
	ASSERT( rCaption.Height() >= 0 );
	if (rCaption.Height() < 16)
		rCaption.bottom = rCaption.top + 16;
	rCaption.right += 25;
	if (rCaption.Width() > rRect.Width() - 16)
		rCaption.right = rCaption.left + rRect.Width() - 16;

	if (m_MemBMP.m_hObject)
		VERIFY( m_MemBMP.DeleteObject() );
	VERIFY( m_MemBMP.CreateCompatibleBitmap(pDC, rCaption.Width(), rCaption.Height()) );
	pOldBMP = MemDC.SelectObject(&m_MemBMP);

	MemDC.FillSolidRect(rCaption, GetSysColor(COLOR_BTNFACE));
	
	VERIFY( DrawState( MemDC.m_hDC, NULL, NULL, (LPARAM)(HICON)CTempIconLoader(m_pszIconID, 16, 16), NULL, 3, 0, 16, 16, DST_ICON | DSS_NORMAL) );

	// clear all alpha channel data
	BITMAP bmMem;
	if (m_MemBMP.GetObject(sizeof bmMem, &bmMem) >= sizeof bmMem && bmMem.bmBitsPixel == 32)
	{
		DWORD dwSize = m_MemBMP.GetBitmapBits(0, NULL);
		if (dwSize)
		{
			LPBYTE pPixels = (LPBYTE)malloc(dwSize);
			if (pPixels)
			{
				if (m_MemBMP.GetBitmapBits(dwSize, pPixels) == dwSize)
				{
					LPBYTE pLine = pPixels;
					int iLines = bmMem.bmHeight;
					while (iLines-- > 0)
					{
						LPDWORD pdwPixel = (LPDWORD)pLine;
						for (int x = 0; x < bmMem.bmWidth; x++)
							*pdwPixel++ &= 0x00FFFFFF;
						pLine += bmMem.bmWidthBytes;
					}
					m_MemBMP.SetBitmapBits(dwSize, pPixels);
				}
				free(pPixels);
			}
		}
	}

	rCaption.left += 22;
	
	if(g_xpStyle.IsThemeActive() && g_xpStyle.IsAppThemed())
    {
		HTHEME hTheme = g_xpStyle.OpenThemeData(NULL, L"BUTTON"); 
		USES_CONVERSION;
		LPCWSTR oleText = T2CW(m_strText); 
		g_xpStyle.DrawThemeText(hTheme, MemDC.m_hDC, BP_GROUPBOX, GBS_NORMAL, oleText, ocslen (oleText), 
			DT_WORDBREAK | DT_CENTER | DT_WORD_ELLIPSIS, NULL, &rCaption); 
		g_xpStyle.CloseThemeData(hTheme);
	}
	else
	{	
		MemDC.SetTextColor(pDC->GetTextColor());
		MemDC.DrawText(m_strText, rCaption, DT_SINGLELINE | DT_LEFT | DT_END_ELLIPSIS);
	}

	ReleaseDC( pDC );

	MemDC.SelectObject(pOldBMP);
	MemDC.SelectObject(pOldFont);
	
	if(m_wndPicture.m_hWnd == NULL)
		m_wndPicture.Create(NULL, WS_CHILD | WS_VISIBLE | SS_BITMAP, CRect(0,0,0,0), this);

	m_wndPicture.SetWindowPos(NULL, rRect.left+8, rRect.top, rCaption.Width()+22, rCaption.Height(), SWP_SHOWWINDOW);
	m_wndPicture.SetBitmap(m_MemBMP);

	CWnd *pParent = GetParent();
	if(pParent == NULL)
		pParent = GetDesktopWindow();
	
	CRect r;
	GetWindowRect(r);
	r.bottom = r.top + 20;
	GetParent()->ScreenToClient(&r);
	GetParent()->RedrawWindow(r);

	return true;
}

bool CIconStatic::SetText(CString strText)
{
	m_strText = strText;
	return Init(m_pszIconID);
}

bool CIconStatic::SetIcon(LPCTSTR pszIconID)
{
	return Init(pszIconID);
}

void CIconStatic::OnSysColorChange()
{
	CStatic::OnSysColorChange();
	if (m_pszIconID)
		Init(m_pszIconID);
}
