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
#include <share.h>
#include "emule.h"
#include "HTRichEditCtrl.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "MenuCmds.h"
#include "Log.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CHTRichEditCtrl, CRichEditCtrl)

BEGIN_MESSAGE_MAP(CHTRichEditCtrl, CRichEditCtrl)
	ON_WM_CONTEXTMENU()
	ON_WM_KEYDOWN()
	ON_WM_PAINT()
	ON_CONTROL_REFLECT(EN_ERRSPACE, OnEnErrspace)
	ON_CONTROL_REFLECT(EN_MAXTEXT, OnEnMaxtext)
	ON_NOTIFY_REFLECT_EX(EN_LINK, OnEnLink)
	ON_WM_CREATE()
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

CHTRichEditCtrl::CHTRichEditCtrl()
{
	m_bRichEdit = true;
	m_bAutoScroll = true;
	m_bNoPaint = false;
	m_bEnErrSpace = false;
	m_bRestoreFormat = false;
	memset(&m_cfDefault, 0, sizeof m_cfDefault);
}

CHTRichEditCtrl::~CHTRichEditCtrl(){
}

void CHTRichEditCtrl::Localize(){
}

void CHTRichEditCtrl::Init(LPCTSTR pszTitle, LPCTSTR pszSkinKey)
{
	SetProfileSkinKey(pszSkinKey);
	SetTitle(pszTitle);

	VERIFY( SendMessage(EM_SETUNDOLIMIT, 0, 0) == 0 );
	int iMaxLogBuff = thePrefs.GetMaxLogBuff();
	if (afxData.bWin95)
		LimitText(iMaxLogBuff > 0xFFFF ? 0xFFFF : iMaxLogBuff);
	else
		LimitText(iMaxLogBuff ? iMaxLogBuff : 128*1024);
	m_iLimitText = GetLimitText();

	VERIFY( GetSelectionCharFormat(m_cfDefault) );

	// prevent the RE control to change the font height within single log lines (may happen with some Unicode chars)
	DWORD dwLangOpts = SendMessage(EM_GETLANGOPTIONS);
	SendMessage(EM_SETLANGOPTIONS, 0, dwLangOpts & ~IMF_AUTOFONT);
}

void CHTRichEditCtrl::SetProfileSkinKey(LPCTSTR pszSkinKey)
{
	m_strSkinKey = pszSkinKey;
}

void CHTRichEditCtrl::SetTitle(LPCTSTR pszTitle)
{
	m_strTitle = pszTitle;
}

int CHTRichEditCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CRichEditCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;
	Init(NULL);
	return 0;
}

LRESULT CHTRichEditCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_ERASEBKGND:
			if (m_bNoPaint)
				return TRUE;
		case WM_PAINT:
			if (m_bNoPaint)
				return TRUE;
	}
	return CRichEditCtrl::WindowProc(message, wParam, lParam);
}

COLORREF GetLogLineColor(UINT eMsgType)
{
	if (eMsgType == LOG_ERROR)
		return thePrefs.m_crLogError;
	if (eMsgType == LOG_WARNING)
		return thePrefs.m_crLogWarning;
	if (eMsgType == LOG_SUCCESS)
		return thePrefs.m_crLogSuccess;
	ASSERT( eMsgType == LOG_INFO );
	return CLR_DEFAULT;
}

void CHTRichEditCtrl::FlushBuffer()
{
	if (m_astrBuff.GetSize() > 0) // flush buffer
	{
		for (int i = 0; i < m_astrBuff.GetSize(); i++)
		{
			const CString& rstrLine = m_astrBuff[i];
			if (!rstrLine.IsEmpty())
			{
				if ((_TUCHAR)rstrLine[0] < 8)
					AddLine((LPCTSTR)rstrLine + 1, rstrLine.GetLength() - 1, false, GetLogLineColor((_TUCHAR)rstrLine[0]));
				else
					AddLine((LPCTSTR)rstrLine, rstrLine.GetLength());
			}
		}
		m_astrBuff.RemoveAll();
	}
}

void CHTRichEditCtrl::AddEntry(LPCTSTR pszMsg)
{
	CString strLine(pszMsg);
	strLine += _T("\n");
	if (m_hWnd == NULL){
		m_astrBuff.Add(strLine);
	}
	else{
		FlushBuffer();
		AddLine(strLine, strLine.GetLength());
	}
}

void CHTRichEditCtrl::Add(LPCTSTR pszMsg, int iLen)
{
	if (m_hWnd == NULL){
		CString strLine(pszMsg);
		m_astrBuff.Add(strLine);
	}
	else{
		FlushBuffer();
		AddLine(pszMsg, iLen);
	}
}

void CHTRichEditCtrl::AddTyped(LPCTSTR pszMsg, int iLen, UINT eMsgType)
{
	if (m_hWnd == NULL)
	{
		CString strLine;
		strLine = (TCHAR)(eMsgType & LOGMSGTYPEMASK);
		strLine += pszMsg;
		m_astrBuff.Add(strLine);
	}
	else
	{
		FlushBuffer();
		AddLine(pszMsg, iLen, false, GetLogLineColor(eMsgType & LOGMSGTYPEMASK));
	}
}

void CHTRichEditCtrl::AddLine(LPCTSTR pszMsg, int iLen, bool bLink, COLORREF cr)
{
	int iMsgLen = (iLen == -1) ? _tcslen(pszMsg) : iLen;
	if (iMsgLen == 0)
		return;
#ifdef _DEBUG
//	if (pszMsg[iMsgLen - 1] == _T('\n'))
//		ASSERT( iMsgLen >= 2 && pszMsg[iMsgLen - 2] == _T('\r') );
#endif

	// Get Edit contents dimensions and cursor position
	long iStartChar, iEndChar;
	GetSel(iStartChar, iEndChar);
	long iSize = GetWindowTextLength();

	if (iStartChar == iSize && iSize == iEndChar)
	{
		// The cursor resides at the end of text
		SCROLLINFO si;
		si.cbSize = sizeof si;
		si.fMask = SIF_ALL;
		if (m_bAutoScroll && GetScrollInfo(SB_VERT, &si) && si.nPos >= (int)(si.nMax - si.nPage + 1))
		{
			// Not scrolled away
			SafeAddLine(iSize, pszMsg, iLen, iStartChar, iEndChar, bLink, cr);
			if (m_bAutoScroll && !IsWindowVisible())
				ScrollToLastLine();
		}
		else
		{
			// Reduce flicker by ignoring WM_PAINT
			m_bNoPaint = true;
			BOOL bIsVisible = IsWindowVisible();
			if (bIsVisible)
				SetRedraw(FALSE);

			// Remember where we are
			int nFirstLine = !m_bAutoScroll ? GetFirstVisibleLine() : 0;
		
			// Select at the end of text and replace the selection
			// This is a very fast way to add text to an edit control
			SafeAddLine(iSize, pszMsg, iLen, iStartChar, iEndChar, bLink, cr);
			//if (m_bAutoScroll && iStartChar == iEndChar)
			//	iStartChar = iEndChar = -1;
			SetSel(iStartChar, iEndChar); // Restore our previous selection

			if (!m_bAutoScroll)
				LineScroll(nFirstLine - GetFirstVisibleLine());
			else
				ScrollToLastLine();

			m_bNoPaint = false;
			if (bIsVisible){
				SetRedraw();
				if (m_bRichEdit)
					Invalidate();
			}
		}
	}
	else
	{
		// We should add the text anyway...

		// Reduce flicker by ignoring WM_PAINT
		m_bNoPaint = true;
		BOOL bIsVisible = IsWindowVisible();
		if (bIsVisible)
			SetRedraw(FALSE);

		// Remember where we are
		//int nFirstLine = !m_bAutoScroll ? GetFirstVisibleLine() : 0;
		POINT ptPos;
		if (!m_bAutoScroll)
			SendMessage(EM_GETSCROLLPOS, 0, (LPARAM)&ptPos);
	
		if (iStartChar != iEndChar)
		{
			// If we are currently selecting some text, we have to find out
			// if the caret is near the beginning of this block or near the end.
			// Note that this does not always work. Because of the EM_CHARFROMPOS
			// message returning only 16 bits this will fail if the user has selected 
			// a block with a length dividable by 64k.

			// NOTE: This may cause a lot of terrible CRASHES within the RichEdit control when used for a RichEdit control!?
			// To reproduce the crash: click in the RE control while it's drawing a line an start a selection!
			if (!m_bRichEdit){
			    CPoint pt;
			    ::GetCaretPos(&pt);
			    int nCaretPos = CharFromPos(pt);
			    if (abs((iStartChar % 0xffff - nCaretPos)) < abs((iEndChar % 0xffff - nCaretPos)))
			    {
				    nCaretPos = iStartChar;
				    iStartChar = iEndChar;
				    iEndChar = nCaretPos;
			    }
		    }
		}

		// Note: This will flicker, if someone has a good idea how to prevent this - let me know
		
		// Select at the end of text and replace the selection
		// This is a very fast way to add text to an edit control
		SafeAddLine(iSize, pszMsg, iLen, iStartChar, iEndChar, bLink, cr);
		//if (m_bAutoScroll && iStartChar == iEndChar)
		//	iStartChar = iEndChar = -1;
		SetSel(iStartChar, iEndChar); // Restore our previous selection

		if (!m_bAutoScroll){
			//LineScroll(nFirstLine - GetFirstVisibleLine());
			SendMessage(EM_SETSCROLLPOS, 0, (LPARAM)&ptPos);
		}
		else
			ScrollToLastLine();

		m_bNoPaint = false;
		if (bIsVisible){
			SetRedraw();
			if (m_bRichEdit)
				Invalidate();
		}
	}
}

void CHTRichEditCtrl::OnEnErrspace()
{
	m_bEnErrSpace = true;
}

void CHTRichEditCtrl::OnEnMaxtext()
{
	m_bEnErrSpace = true;
}

void CHTRichEditCtrl::ScrollToLastLine()
{
	// WM_VSCROLL does not work correctly under Win98 (or older version of comctl.dll)
	SendMessage(WM_VSCROLL, SB_BOTTOM);
	if (afxData.bWin95){
		// older version of comctl.dll seem to need this to properly update the display
		int iPos = GetScrollPos(SB_VERT);
		SendMessage(WM_VSCROLL, MAKELONG(SB_THUMBPOSITION, iPos));
		SendMessage(WM_VSCROLL, SB_ENDSCROLL);
	}
}

void CHTRichEditCtrl::AddString(int nPos, LPCTSTR pszString, bool bLink, COLORREF cr)
{
	bool bRestoreFormat = false;
	m_bEnErrSpace = false;
	SetSel(nPos, nPos);
	if (bLink)
	{
		CHARFORMAT2 cf;
		memset(&cf, 0, sizeof cf);
		GetSelectionCharFormat(cf);
		cf.dwMask |= CFM_LINK;
		cf.dwEffects |= CFE_LINK;
		SetSelectionCharFormat(cf);
	}
	else if (cr != CLR_DEFAULT)
	{
		CHARFORMAT cf;
		GetSelectionCharFormat(cf);
		cf.dwMask |= CFM_COLOR;
		cf.dwEffects &= ~CFE_AUTOCOLOR;
		cf.crTextColor = cr;
		SetSelectionCharFormat(cf);
		bRestoreFormat = true;
	}
	else if (m_bRestoreFormat)
	{
		SetSelectionCharFormat(m_cfDefault);
	}
	ReplaceSel(pszString);
	m_bRestoreFormat = bRestoreFormat;
}

void CHTRichEditCtrl::SafeAddLine(int nPos, LPCTSTR pszLine, int iLen, long& iStartChar, long& iEndChar, bool bLink, COLORREF cr)
{
	// EN_ERRSPACE and EN_MAXTEXT are not working for rich edit control (at least not same as for standard control),
	// need to explicitly check the log buffer limit..
	int iCurSize = nPos;
	if (iCurSize + iLen >= m_iLimitText)
{
		bool bOldNoPaint = m_bNoPaint;
		m_bNoPaint = true;
		BOOL bIsVisible = IsWindowVisible();
		if (bIsVisible)
			SetRedraw(FALSE);

		while (iCurSize > 0 && iCurSize + iLen > m_iLimitText)
		{
			// delete 1st line
			int iLine0Len = LineLength(0) + 1; // add NL character
			SetSel(0, iLine0Len);
			ReplaceSel(_T(""));

			// update any possible available selection
			iStartChar -= iLine0Len;
			if (iStartChar < 0)
				iStartChar = 0;
			iEndChar -= iLine0Len;
			if (iEndChar < 0)
				iEndChar = 0;

			iCurSize = GetWindowTextLength();
		}

		m_bNoPaint = bOldNoPaint;
		if (bIsVisible && !m_bNoPaint){
			SetRedraw();
			if (m_bRichEdit)
				Invalidate();
		}
	}

	AddString(nPos, pszLine, bLink, cr);

	if (m_bEnErrSpace)
	{
		bool bOldNoPaint = m_bNoPaint;
		m_bNoPaint = true;
		BOOL bIsVisible = IsWindowVisible();
		if (bIsVisible)
			SetRedraw(FALSE);

		// remove the first line as long as we are capable of adding the new line
		int iSafetyCounter = 0;
		while (m_bEnErrSpace && iSafetyCounter < 10)
		{
			// delete the previous partially added line
			SetSel(nPos, -1);
			ReplaceSel(_T(""));

			// delete 1st line
			int iLine0Len = LineLength(0) + 1; // add NL character
			SetSel(0, iLine0Len);
			ReplaceSel(_T(""));

			// update any possible available selection
			iStartChar -= iLine0Len;
			if (iStartChar < 0)
				iStartChar = 0;
			iEndChar -= iLine0Len;
			if (iEndChar < 0)
				iEndChar = 0;

			// add the new line again
			nPos = GetWindowTextLength();
			AddString(nPos, pszLine, bLink, cr);

			if (m_bEnErrSpace && nPos == 0){
				// should never happen: if we tried to add the line another time in the 1st line, there 
				// will be no chance to add the line at all -> avoid endless loop!
				break;
			}
			iSafetyCounter++; // never ever create an endless loop!
		}
		m_bNoPaint = bOldNoPaint;
		if (bIsVisible && !m_bNoPaint){
			SetRedraw();
			if (m_bRichEdit)
				Invalidate();
		}
	}
}

void CHTRichEditCtrl::Reset(){
	m_astrBuff.RemoveAll();
	SetRedraw(FALSE);
	SetWindowText(_T(""));
	SetRedraw();
	if (m_bRichEdit)
		Invalidate();
}

void CHTRichEditCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	long iSelStart, iSelEnd;
	GetSel(iSelStart, iSelEnd);

	// ugly, simulate a left click to get around the text cursor problem when right clicking.
	if (point.x != -1 && point.y != -1 && iSelStart == iSelEnd)
	{
		CPoint ptMouse(point);
		ScreenToClient(&ptMouse);
		SendMessage(WM_LBUTTONDOWN, MK_LBUTTON, MAKELONG(ptMouse.x, ptMouse.y));
		SendMessage(WM_LBUTTONUP, MK_LBUTTON, MAKELONG(ptMouse.x, ptMouse.y));
	}

	int iTextLen = GetWindowTextLength();

	// create menu here for runtime localisation

	m_LogMenu.CreatePopupMenu();
	m_LogMenu.AddMenuTitle(GetResString(IDS_LOGENTRY));
	m_LogMenu.AppendMenu(MF_STRING | (iSelEnd > iSelStart ? MF_ENABLED : MF_GRAYED), MP_COPYSELECTED, GetResString(IDS_COPY));
	m_LogMenu.AppendMenu(MF_SEPARATOR);
	m_LogMenu.AppendMenu(MF_STRING | (iTextLen > 0 ? MF_ENABLED : MF_GRAYED), MP_SELECTALL, GetResString(IDS_SELECTALL));
	m_LogMenu.AppendMenu(MF_STRING | (iTextLen > 0 ? MF_ENABLED : MF_GRAYED), MP_REMOVEALL , GetResString(IDS_PW_RESET));
	m_LogMenu.AppendMenu(MF_STRING | (iTextLen > 0 ? MF_ENABLED : MF_GRAYED), MP_SAVELOG, GetResString(IDS_SAVELOG) + _T("..."));
	m_LogMenu.AppendMenu(MF_SEPARATOR);
	m_LogMenu.AppendMenu(MF_STRING, MP_AUTOSCROLL, GetResString(IDS_AUTOSCROLL));

	m_LogMenu.CheckMenuItem(MP_AUTOSCROLL, m_bAutoScroll ? MF_CHECKED : MF_UNCHECKED);

	if (point.x == -1 && point.y == -1)
	{
		point.x = 16;
		point.y = 32;
		ClientToScreen(&point);
	}
	m_LogMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);

	m_LogMenu.DestroyMenu();
}

BOOL CHTRichEditCtrl::OnCommand(WPARAM wParam, LPARAM lParam){
	switch (wParam) {
	case MP_COPYSELECTED:
		CopySelectedItems();
		break;
	case MP_SELECTALL:
		SelectAllItems();
		break;
	case MP_REMOVEALL:
		Reset();
		break;
	case MP_SAVELOG:
		SaveLog();
		break;
	case MP_AUTOSCROLL:
		m_bAutoScroll = !m_bAutoScroll;
		break;
	}
	return TRUE;
}

bool CHTRichEditCtrl::SaveLog(LPCTSTR pszDefName)
{
	bool bResult = false;
	CFileDialog dlg(FALSE, _T("log"), pszDefName ? pszDefName : (LPCTSTR)m_strTitle, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Log Files (*.log)|*.log||"), this, 0);
	if (dlg.DoModal() == IDOK)
	{
		FILE* fp = _tfsopen(dlg.GetPathName(), _T("wb"), _SH_DENYWR);
		if (fp)
		{
#ifdef _UNICODE
			// write Unicode byte-order mark 0xFEFF
			fputwc(0xFEFF, fp);
#endif
			CString strText;
			GetWindowText(strText);
			fwrite(strText, sizeof(TCHAR), strText.GetLength(), fp);
			if (ferror(fp)){
				CString strError;
				strError.Format(_T("Failed to write log file \"%s\" - %s"), dlg.GetPathName(), _tcserror(errno));
				AfxMessageBox(strError, MB_ICONERROR);
			}
			else
				bResult = true;
			fclose(fp);
		}
		else{
			CString strError;
			strError.Format(_T("Failed to create log file \"%s\" - %s"), dlg.GetPathName(), _tcserror(errno));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}
	return bResult;
}

CString CHTRichEditCtrl::GetLastLogEntry(){
	CString strLog;
	int iLastLine = GetLineCount() - 2;
	if (iLastLine >= 0){
		GetLine(iLastLine, strLog.GetBuffer(1024), 1024);
		strLog.ReleaseBuffer();
	}
	return strLog;
}

CString CHTRichEditCtrl::GetAllLogEntries(){
	CString strLog;
	GetWindowText(strLog);
	return strLog;
}

void CHTRichEditCtrl::SelectAllItems()
{
	SetSel(0, -1);
}

void CHTRichEditCtrl::CopySelectedItems()
{
	Copy();
}

void CHTRichEditCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == 'A' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		//////////////////////////////////////////////////////////////////
		// Ctrl+A: Select all items
		SelectAllItems();
	}
	else if (nChar == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		//////////////////////////////////////////////////////////////////
		// Ctrl+C: Copy listview items to clipboard
		CopySelectedItems();
	}

	CRichEditCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

static const struct
{
	LPCTSTR pszScheme;
	int iLen;
} _apszSchemes[] = 
{
	{ _T("ed2k://"),  7 },
	{ _T("http://"),  7 },
	{ _T("https://"), 8 },
	{ _T("ftp://"),   6 },
	{ _T("www."),     4 },
	{ _T("ftp."),     4 },
	{ _T("mailto:"),  7 }
};

void CHTRichEditCtrl::AppendText(const CString& sText, bool bInvalidate)
{
	LPCTSTR psz = sText;
	LPCTSTR pszStart = psz;
	while (*psz != _T('\0'))
	{
		bool bFoundScheme = false;
		for (int i = 0; i < ARRSIZE(_apszSchemes); i++)
		{
			if (_tcsncmp(psz, _apszSchemes[i].pszScheme, _apszSchemes[i].iLen) == 0)
			{
				// output everything before the URL
				if (psz - pszStart > 0){
					CString str(pszStart, psz - pszStart);
					AddLine(str, str.GetLength());
				}

				// search next space or EOL
				int iLen = _tcscspn(psz, _T(" \n\r\t"));
				if (iLen == 0){
					AddLine(psz, -1, true);
					psz += _tcslen(psz);
				}
				else{
					CString str(psz, iLen);
					AddLine(str, str.GetLength(), true);
					psz += iLen;
				}
				pszStart = psz;
				bFoundScheme = true;
				break;
			}
		}
		if (!bFoundScheme)
			psz = _tcsinc(psz);
	}

	if (*pszStart != _T('\0'))
		AddLine(pszStart, -1);
}

void CHTRichEditCtrl::AppendHyperLink(const CString& sText, const CString& sTitle, const CString& sCommand, const CString& sDirectory, bool bInvalidate)
{
	ASSERT( sText.IsEmpty() );
	ASSERT( sTitle.IsEmpty() );
	ASSERT( sDirectory.IsEmpty() );
	AddLine(sCommand, sCommand.GetLength(), true);
}

void CHTRichEditCtrl::AppendColoredText(LPCTSTR pszText, COLORREF cr)
{
	AddLine(pszText, -1, false, cr);
}

void CHTRichEditCtrl::AppendKeyWord(const CString& str, COLORREF cr)
{
	AppendColoredText(str, cr);
}

BOOL CHTRichEditCtrl::OnEnLink(NMHDR *pNMHDR, LRESULT *pResult)
{
	BOOL bMsgHandled = FALSE;
	*pResult = 0;
	ENLINK* pEnLink = reinterpret_cast<ENLINK *>(pNMHDR);
	if (pEnLink && pEnLink->msg == WM_LBUTTONDOWN)
	{
		CString strUrl;
		GetTextRange(pEnLink->chrg.cpMin, pEnLink->chrg.cpMax, strUrl);

		// check if that "URL" has a valid URL scheme. if it does not have, pass that notification up to the
		// parent window which may interpret that "URL" in some other way.
		for (int i = 0; i < ARRSIZE(_apszSchemes); i++){
			if (_tcsncmp(strUrl, _apszSchemes[i].pszScheme, _apszSchemes[i].iLen) == 0){
				ShellExecute(NULL, NULL, strUrl, NULL, NULL, SW_SHOWDEFAULT);
				*pResult = 1;
				bMsgHandled = TRUE; // do not route this message to any parent
				break;
			}
		}
	}
	return bMsgHandled;
}

CString CHTRichEditCtrl::GetText() const
{
	CString strText;
	GetWindowText(strText);
	return strText;
}
 
void CHTRichEditCtrl::SetFont(CFont* pFont, BOOL bRedraw)
{
	LOGFONT lf = {0};
	pFont->GetLogFont(&lf);

	CHARFORMAT cf = {0};
	cf.cbSize = sizeof cf;

	cf.dwMask |= CFM_BOLD;
	cf.dwEffects |= (lf.lfWeight == FW_BOLD) ? CFE_BOLD : 0;

	cf.dwMask |= CFM_ITALIC;
	cf.dwEffects |= (lf.lfItalic) ? CFE_ITALIC : 0;

	cf.dwMask |= CFM_UNDERLINE;
	cf.dwEffects |= (lf.lfUnderline) ? CFE_UNDERLINE : 0;

	cf.dwMask |= CFM_STRIKEOUT;
	cf.dwEffects |= (lf.lfStrikeOut) ? CFE_STRIKEOUT : 0;

	cf.dwMask |= CFM_SIZE;
	HDC hDC = ::GetDC(NULL);
	int iPointSize = -MulDiv(lf.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
	cf.yHeight = iPointSize * 20;
	::ReleaseDC(NULL, hDC);

	cf.dwMask |= CFM_FACE;
	cf.bPitchAndFamily = lf.lfPitchAndFamily;
	_tcsncpy(cf.szFaceName, lf.lfFaceName, ARRSIZE(cf.szFaceName));
	cf.szFaceName[ARRSIZE(cf.szFaceName) - 1] = _T('\0');

	// although this should work correctly (according SDK) it may give false results (e.g. the "click here..." text
	// which is shown in the server info window may not be entirely used as a hyperlink???)
//	cf.dwMask |= CFM_CHARSET;
//	cf.bCharSet = lf.lfCharSet;

	cf.yOffset = 0;
	VERIFY( SetDefaultCharFormat(cf) );
	VERIFY( GetSelectionCharFormat(m_cfDefault) );

	if (bRedraw){
		Invalidate();
		UpdateWindow();
	}
}

CFont* CHTRichEditCtrl::GetFont() const
{
	ASSERT(0);
	return NULL;
}

void CHTRichEditCtrl::OnSysColorChange()
{
	CRichEditCtrl::OnSysColorChange();
	ApplySkin();
}

void CHTRichEditCtrl::ApplySkin()
{
	if (!m_strSkinKey.IsEmpty())
	{
		COLORREF cr;
		if (theApp.LoadSkinColor(m_strSkinKey + _T("Bk"), cr))
		{
			SetBackgroundColor(FALSE, cr);
		}
		else
		{
			SetBackgroundColor(TRUE, GetSysColor(COLOR_WINDOW));
		}
	}
}

//MORPH START - Added by SiRoB, XML News [O²]
void CHTRichEditCtrl::ScrollToFirstLine()
{
	// WM_VSCROLL does not work correctly under Win98 (or older version of comctl.dll)
	SendMessage(WM_VSCROLL, SB_TOP);
	if (afxData.bWin95){
		// older version of comctl.dll seem to need this to properly update the display
		int iPos = GetScrollPos(SB_VERT);
		SendMessage(WM_VSCROLL, MAKELONG(SB_THUMBPOSITION, iPos));
		SendMessage(WM_VSCROLL, SB_ENDSCROLL);
	}
}
//MORPH END   - Added by SiRoB, XML News [O²]
