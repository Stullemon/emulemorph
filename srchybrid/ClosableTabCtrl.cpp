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
#include "ClosableTabCtrl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CClosableTabCtrl
// by enkeyDEV(Ottavio84)

IMPLEMENT_DYNAMIC(CClosableTabCtrl, CTabCtrl)

BEGIN_MESSAGE_MAP(CClosableTabCtrl, CTabCtrl)
	ON_WM_LBUTTONUP()
	ON_WM_CREATE()
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

CClosableTabCtrl::CClosableTabCtrl()
{
}

CClosableTabCtrl::~CClosableTabCtrl()
{
}

void CClosableTabCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	int ntabs = GetItemCount();
	CRect clsbutrect;
	for (int i = 0; i < ntabs; i++) {
		GetItemRect(i, clsbutrect);
		clsbutrect.DeflateRect(2, 2, clsbutrect.right - clsbutrect.left - 16, 0);
		if (clsbutrect.PtInRect(point)) {
			GetParent()->SendMessage(WM_CLOSETAB, (WPARAM) i);
			return; 
		}
	}
	
	CTabCtrl::OnLButtonDown(nFlags, point);
}

void CClosableTabCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	CRect rect = lpDrawItemStruct->rcItem;
	int nTabIndex = lpDrawItemStruct->itemID;
	if (nTabIndex < 0)
		return;
	BOOL bSelected = (nTabIndex == GetCurSel());

	TCHAR szLabel[64];
	TC_ITEM tci;
	tci.mask = TCIF_TEXT|TCIF_IMAGE;
	tci.pszText = szLabel;
	tci.cchTextMax = ARRSIZE(szLabel);
	if (!GetItem(nTabIndex, &tci))
		return;

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	if (!pDC)
		return;
	int nSavedDC = pDC->SaveDC();

	rect.top += ::GetSystemMetrics(SM_CYEDGE);

	pDC->SetBkMode(TRANSPARENT);

	// Draw image
	if (tci.iImage >= 0) {
		rect.left += pDC->GetTextExtent(_T(" ")).cx;		// Margin

		// Get height of image so we 
		IMAGEINFO info;
		m_ImgLst.GetImageInfo(0, &info);
		CRect ImageRect(info.rcImage);
		int nYpos = rect.top;

		m_ImgLst.Draw(pDC, 0, CPoint(rect.left, nYpos), ILD_TRANSPARENT);
		rect.left += ImageRect.Width();
	}

	if (bSelected) {
		rect.top -= ::GetSystemMetrics(SM_CYEDGE);
		pDC->DrawText(szLabel, rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_NOPREFIX);
		rect.top += ::GetSystemMetrics(SM_CYEDGE);
	}
	else {
		pDC->DrawText(szLabel, rect, DT_SINGLELINE|DT_BOTTOM|DT_CENTER|DT_NOPREFIX);
	}

	if (nSavedDC)
		pDC->RestoreDC(nSavedDC);
}

void CClosableTabCtrl::PreSubclassWindow()
{
	CTabCtrl::PreSubclassWindow();
	ModifyStyle(0, TCS_OWNERDRAWFIXED);
	SetAllIcons();
}

int CClosableTabCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTabCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	ModifyStyle(0, TCS_OWNERDRAWFIXED);
	SetAllIcons();
	return 0;
}

void CClosableTabCtrl::OnSysColorChange()
{
	CTabCtrl::OnSysColorChange();
	SetAllIcons();
}

void CClosableTabCtrl::SetAllIcons()
{
	m_ImgLst.DeleteImageList();
	m_ImgLst.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	m_ImgLst.SetBkColor(CLR_NONE);
	m_ImgLst.Add(CTempIconLoader("CloseTab"));
	Invalidate();
}
