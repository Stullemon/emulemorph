//this file is part of eMule
//Copyright (C)2005 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "DropDownButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define	MAX_BTN_WIDTH_XOFF	0

IMPLEMENT_DYNAMIC(CDropDownButton, CToolBarCtrl)

BEGIN_MESSAGE_MAP(CDropDownButton, CToolBarCtrl)
	ON_WM_SIZE()
END_MESSAGE_MAP()

CDropDownButton::CDropDownButton()
{
	m_bSingleDropDownBtn = true;
}

CDropDownButton::~CDropDownButton()
{
}

BOOL CDropDownButton::Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, bool bSingleDropDownBtn)
{
	m_bSingleDropDownBtn = bSingleDropDownBtn;
	dwStyle |= CCS_NOMOVEY
			   | CCS_NOPARENTALIGN
			   | CCS_NORESIZE		// prevent adjusting of specified width & height(!) by Create func..
			   | CCS_NODIVIDER
			   | TBSTYLE_LIST
			   | TBSTYLE_FLAT
			   | TBSTYLE_TRANSPARENT
			   | 0;

	if (!CToolBarCtrl::Create(dwStyle, rect, pParentWnd, nID))
		return FALSE;

	// use the following only if the button has a very small height
	//DWORD dwBtnSize = GetButtonSize();
	//SetButtonSize(CSize(LOWORD(dwBtnSize), rect.bottom - rect.top));
	//DWORD dwPadding = SendMessage(TB_GETPADDING);
	//SendMessage(TB_SETPADDING, 0, MAKELPARAM(LOWORD(dwPadding), 3));

	TBBUTTON atb[1] = {0};
	atb[0].iBitmap = -1;
	atb[0].idCommand = GetWindowLong(m_hWnd, GWL_ID);
	atb[0].fsState = TBSTATE_ENABLED;
	atb[0].fsStyle = m_bSingleDropDownBtn ? BTNS_DROPDOWN : BTNS_BUTTON;
	atb[0].iString = -1;
	AddButtons(1, atb);
	if (m_bSingleDropDownBtn)
	{
		ResizeToMaxWidth();
		SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);
	}
	return TRUE;
}

int CDropDownButton::GetBtnWidth(int nID)
{
	TBBUTTONINFO tbbi = {0};
	tbbi.cbSize = sizeof tbbi;
	tbbi.dwMask = TBIF_SIZE;
	(void)GetButtonInfo(nID, &tbbi);
	return tbbi.cx;
}

void CDropDownButton::SetBtnWidth(int nID, int iWidth)
{
	TBBUTTONINFO tbbi = {0};
	tbbi.cbSize = sizeof tbbi;
	tbbi.dwMask = TBIF_SIZE;
	tbbi.cx = iWidth;
	SetButtonInfo(nID, &tbbi);
}

CString CDropDownButton::GetBtnText(int nID)
{
	TCHAR szString[512];
	TBBUTTONINFO tbbi = {0};
	tbbi.cbSize = sizeof tbbi;
	tbbi.dwMask = TBIF_TEXT;
	tbbi.pszText = szString;
	tbbi.cchText = _countof(szString);
	GetButtonInfo(nID, &tbbi);
	return szString;
}

void CDropDownButton::SetBtnText(int nID, LPCTSTR pszString)
{
	TBBUTTONINFO tbbi = {0};
	tbbi.cbSize = sizeof tbbi;
	tbbi.dwMask = TBIF_TEXT;
	tbbi.pszText = const_cast<LPTSTR>(pszString);
	SetButtonInfo(nID, &tbbi);
}

void CDropDownButton::SetWindowText(LPCTSTR pszString)
{
	int cx = 0;
	if (!m_bSingleDropDownBtn)
		cx = GetBtnWidth(GetWindowLong(m_hWnd, GWL_ID));

	TBBUTTONINFO tbbi = {0};
	tbbi.cbSize = sizeof tbbi;
	tbbi.dwMask = TBIF_TEXT;
	tbbi.pszText = const_cast<LPTSTR>(pszString);
	SetButtonInfo(GetWindowLong(m_hWnd, GWL_ID), &tbbi);

	if (cx)
		SetBtnWidth(GetWindowLong(m_hWnd, GWL_ID), cx);
}

void CDropDownButton::SetIcon(LPCTSTR pszResourceID)
{
	if (!m_bSingleDropDownBtn)
		return;

	CImageList iml;
	iml.Create(16, 16, theApp.m_iDfltImageListColorFlags | ILC_MASK, 1, 1);
	iml.Add(CTempIconLoader(pszResourceID));
	CImageList* pImlOld = SetImageList(&iml);
	iml.Detach();
	if (pImlOld)
		pImlOld->DeleteImageList();

	TBBUTTONINFO tbbi = {0};
	tbbi.cbSize = sizeof tbbi;
	tbbi.dwMask = TBIF_IMAGE;
	tbbi.iImage = 0;
	SetButtonInfo(GetWindowLong(m_hWnd, GWL_ID), &tbbi);
}

void CDropDownButton::ResizeToMaxWidth()
{
	if (!m_bSingleDropDownBtn)
		return;

	CRect rcWnd;
	GetWindowRect(&rcWnd);
	if (rcWnd.Width() > 0)
	{
	    TBBUTTONINFO tbbi = {0};
	    tbbi.cbSize = sizeof tbbi;
	    tbbi.dwMask = TBIF_SIZE;
	    tbbi.cx = rcWnd.Width() - MAX_BTN_WIDTH_XOFF;
	    SetButtonInfo(GetWindowLong(m_hWnd, GWL_ID), &tbbi);
	}
}

void CDropDownButton::OnSize(UINT nType, int cx, int cy)
{
	CToolBarCtrl::OnSize(nType, cx, cy);

	if (cx > 0 && cy > 0)
		ResizeToMaxWidth();
}
