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
#include "resource.h"
#include "MenuCmds.h"
#include "RichEditCtrlX.h"
#include "OtherFunctions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// CRichEditCtrlX

BEGIN_MESSAGE_MAP(CRichEditCtrlX, CRichEditCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_KEYDOWN()
	ON_WM_GETDLGCODE()
	ON_NOTIFY_REFLECT_EX(EN_LINK, OnEnLink)
END_MESSAGE_MAP()

CRichEditCtrlX::CRichEditCtrlX()
{
}

CRichEditCtrlX& CRichEditCtrlX::operator<<(LPCTSTR psz)
{
	ReplaceSel(psz);
	return *this;
}

CRichEditCtrlX& CRichEditCtrlX::operator<<(char* psz)
{
	ReplaceSel(psz);
	return *this;
}

CRichEditCtrlX& CRichEditCtrlX::operator<<(UINT uVal)
{
	CString strVal;
	strVal.Format(_T("%u"), uVal);
	ReplaceSel(strVal);
	return *this;
}

CRichEditCtrlX& CRichEditCtrlX::operator<<(int iVal)
{
	CString strVal;
	strVal.Format(_T("%d"), iVal);
	ReplaceSel(strVal);
	return *this;
}

CRichEditCtrlX& CRichEditCtrlX::operator<<(double fVal)
{
	CString strVal;
	strVal.Format(_T("%.3f"), fVal);
	ReplaceSel(strVal);
	return *this;
}

UINT CRichEditCtrlX::OnGetDlgCode() 
{
	// Avoid that the edit control will select the entire contents, if the
	// focus is moved via tab into the edit control
	//
	// DLGC_WANTALLKEYS is needed, if the control is within a wizard property
	// page and the user presses the Enter key to invoke the default button of the property sheet!
	return CRichEditCtrl::OnGetDlgCode() & ~(DLGC_HASSETSEL | DLGC_WANTALLKEYS);
}

BOOL CRichEditCtrlX::OnEnLink(NMHDR *pNMHDR, LRESULT *pResult)
{
	BOOL bMsgHandled = FALSE;
	*pResult = 0;
	ENLINK* pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	if (pEnLink && pEnLink->msg == WM_LBUTTONDOWN)
	{
		CString strUrl;
		GetTextRange(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax, strUrl);

		ShellExecute(NULL, NULL, strUrl, NULL, NULL, SW_SHOWDEFAULT);
		*pResult = 1;
		bMsgHandled = TRUE; // do not route this message to any parent
	}
	return bMsgHandled;
}

void CRichEditCtrlX::OnContextMenu(CWnd* pWnd, CPoint point)
{
	long iSelStart, iSelEnd;
	GetSel(iSelStart, iSelEnd);
	int iTextLen = GetWindowTextLength();

	CMenu menu;
	menu.CreatePopupMenu();
	menu.AppendMenu(MF_STRING, MP_COPYSELECTED, GetResString(IDS_COPY));
	menu.AppendMenu(MF_SEPARATOR);
	menu.AppendMenu(MF_STRING, MP_SELECTALL, GetResString(IDS_SELECTALL));
	menu.EnableMenuItem(MP_COPYSELECTED, iSelEnd > iSelStart ? MF_ENABLED : MF_GRAYED);
	menu.EnableMenuItem(MP_SELECTALL, iTextLen > 0 ? MF_ENABLED : MF_GRAYED);
	if (point.x == -1 && point.y == -1){
		point.x = 16;
		point.y = 32;
		ClientToScreen(&point);
	}
	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
}

BOOL CRichEditCtrlX::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (wParam) {
	case MP_COPYSELECTED:
		Copy();
		break;
	case MP_SELECTALL:
		SetSel(0, -1);
		break;
	}
	return TRUE;
}

void CRichEditCtrlX::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == 'A' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		//////////////////////////////////////////////////////////////////
		// Ctrl+A: Select all
		SetSel(0, -1);
	}
	else if (nChar == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		//////////////////////////////////////////////////////////////////
		// Ctrl+C: Copy selected contents to clipboard
		Copy();
	}

	CRichEditCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}
