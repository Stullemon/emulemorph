#include "stdafx.h"
#include "eMule.h"
#include "HTRichEditCtrl.h"

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
END_MESSAGE_MAP()

CHTRichEditCtrl::CHTRichEditCtrl(){
	m_bRichEdit = true;
	m_bAutoScroll = true;
	m_bNoPaint = false;
	m_bEnErrSpace = false;
	m_pPreparedText = NULL;
}

CHTRichEditCtrl::~CHTRichEditCtrl(){
}

void CHTRichEditCtrl::Localize(){
}

void CHTRichEditCtrl::Init(LPCTSTR pszTitle)
{
	m_LogMenu.CreatePopupMenu();
	m_LogMenu.AddMenuTitle(GetResString(IDS_LOGENTRY));
	m_LogMenu.AppendMenu(MF_STRING,MP_COPYSELECTED, GetResString(IDS_COPY));
	m_LogMenu.AppendMenu(MF_SEPARATOR);
	m_LogMenu.AppendMenu(MF_STRING,MP_SELECTALL, GetResString(IDS_SELECTALL));
	m_LogMenu.AppendMenu(MF_STRING,MP_REMOVEALL, GetResString(IDS_PW_RESET));
	m_LogMenu.AppendMenu(MF_SEPARATOR);
	m_LogMenu.AppendMenu(MF_STRING,MP_AUTOSCROLL, GetResString(IDS_AUTOSCROLL));

	VERIFY( SendMessage(EM_SETUNDOLIMIT, 0, 0) == 0 );
	int iMaxLogBuff = theApp.glob_prefs->GetMaxLogBuff();
	if (afxData.bWin95)
		LimitText(iMaxLogBuff > 0xFFFF ? 0xFFFF : iMaxLogBuff);
	else
		LimitText(iMaxLogBuff ? iMaxLogBuff : 128*1024);
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

void CHTRichEditCtrl::AddEntry(LPCTSTR pszMsg)
{
	CString strLine(pszMsg);
	strLine += _T("\n");
	if (m_hWnd == NULL){
		m_astrBuff.Add(strLine);
	}
	else{
		if (m_astrBuff.GetSize() > 0){ // flush buffer
			for (int i = 0; i < m_astrBuff.GetSize(); i++)
				AddLine(m_astrBuff[i]);
			m_astrBuff.RemoveAll();
		}
		AddLine(strLine);
	}
}

void CHTRichEditCtrl::AddLine(LPCTSTR pszMsg, bool bLink)
{
	int iMsgLen = _tcslen(pszMsg);
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
			SafeAddLine(iSize, pszMsg, iStartChar, iEndChar, bLink);
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
			SafeAddLine(iSize, pszMsg, iStartChar, iEndChar, bLink);
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
		SafeAddLine(iSize, pszMsg, iStartChar, iEndChar, bLink);
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

void CHTRichEditCtrl::SafeAddLine(int nPos, LPCTSTR pszLine, long& iStartChar, long& iEndChar, bool bLink)
{
	m_bEnErrSpace = false;
	SetSel(nPos, nPos);
	if (bLink){
		CHARFORMAT2 cf;
		MEMSET(&cf, 0, sizeof cf);
		GetSelectionCharFormat(cf);
		cf.dwMask |= CFM_LINK;
		cf.dwEffects |= CFE_LINK;
		SetSelectionCharFormat(cf);
	}
	ReplaceSel(pszLine);

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
			int iLine0Len = LineLength(0) + 2; // add NL character
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
			SetSel(nPos, nPos);
			m_bEnErrSpace = false;
			ReplaceSel(pszLine);

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

void CHTRichEditCtrl::OnContextMenu(CWnd* pWnd, CPoint point){
	long iSelStart, iSelEnd;
	GetSel(iSelStart, iSelEnd);
	int iTextLen = GetWindowTextLength();

	m_LogMenu.EnableMenuItem(MP_COPYSELECTED, iSelEnd > iSelStart ? MF_ENABLED : MF_GRAYED);
	m_LogMenu.EnableMenuItem(MP_REMOVEALL, iTextLen > 0 ? MF_ENABLED : MF_GRAYED);
	m_LogMenu.EnableMenuItem(MP_SELECTALL, iTextLen > 0 ? MF_ENABLED : MF_GRAYED);
	m_LogMenu.CheckMenuItem(MP_AUTOSCROLL, m_bAutoScroll ? MF_CHECKED : MF_UNCHECKED);
	m_LogMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
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
	case MP_AUTOSCROLL:
		m_bAutoScroll = !m_bAutoScroll;
		break;
	}
	return TRUE;
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
					AddLine(str);
				}

				// search next space or EOL
				int iLen = _tcscspn(psz, _T(" \n\r\t"));
				if (iLen == 0){
					AddLine(psz, true);
					psz += _tcslen(psz);
				}
				else{
					CString str(psz, iLen);
					AddLine(str, true);
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
		AddLine(pszStart);
}

void CHTRichEditCtrl::AppendHyperLink(const CString& sText, const CString& sTitle, const CString& sCommand, const CString& sDirectory, bool bInvalidate)
{
	ASSERT( sText.IsEmpty() );
	ASSERT( sTitle.IsEmpty() );
	ASSERT( sDirectory.IsEmpty() );
	AddLine(sCommand, true);
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
 
CPreparedRTFText* CHTRichEditCtrl::GetHyperText()
{
	return m_pPreparedText;
}

void CHTRichEditCtrl::SetHyperText(CPreparedRTFText* pPreparedText, bool bInvalidate)
{
	m_pPreparedText = pPreparedText;
	if (bInvalidate)
		UpdateSize(true);
}

void CHTRichEditCtrl::UpdateSize(bool bRepaint)
{
	if (m_pPreparedText == NULL)
		return;
	//SetWindowText(m_pPreparedText->GetText());

	m_pPreparedText->m_sText.AppendChar('}');
	SETTEXTEX st = {0};
	st.flags = ST_DEFAULT;
	st.codepage = CP_ACP;
	SendMessage(EM_SETTEXTEX, (WPARAM)&st, (LPARAM)(LPCTSTR)m_pPreparedText->GetText());
	m_pPreparedText->m_sText.Delete(m_pPreparedText->m_sText.GetLength()-1);
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


///////////////////////////////////////////////////////////////////////////////
// CPreparedRTFText

CPreparedRTFText::CPreparedRTFText()
{
	m_sText = _T("{\\rtf\\ansi");
}

CPreparedRTFText::~CPreparedRTFText()
{
}

void CPreparedRTFText::AppendKeyWord(const CString& str, COLORREF cr)
{
	if (!m_sText.IsEmpty() && m_sText[m_sText.GetLength()-1] == _T('\n'))
	{
		m_sText.Delete(m_sText.GetLength()-1);
		m_sText += _T("\\par");
	}
	m_sText.AppendFormat(_T("\\red%u\\green%u\\blue%u"), GetRValue(cr), GetGValue(cr), GetBValue(cr));
	m_sText += str;
}

void CPreparedRTFText::AppendText(const CString& str)
{
	if (!m_sText.IsEmpty() && m_sText[m_sText.GetLength()-1] == _T('\n'))
	{
		m_sText.Delete(m_sText.GetLength()-1);
		m_sText += _T("\\par");
	}
	m_sText += str;
}

const CString& CPreparedRTFText::GetText()
{
	return m_sText;
}
