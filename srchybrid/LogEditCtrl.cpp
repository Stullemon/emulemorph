#include "stdafx.h"
#include "emule.h"
#include "LogEditCtrl.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CLogEditCtrl, CEdit)

BEGIN_MESSAGE_MAP(CLogEditCtrl, CEdit)
	ON_WM_CONTEXTMENU()
	ON_WM_KEYDOWN()
	ON_WM_PAINT()
	ON_CONTROL_REFLECT(EN_ERRSPACE, OnEnErrspace)
	ON_CONTROL_REFLECT(EN_MAXTEXT, OnEnMaxtext)
END_MESSAGE_MAP()

CLogEditCtrl::CLogEditCtrl(){
	m_bRichEdit = false;
	m_bAutoScroll = true;
	m_bNoPaint = false;
	m_bEnErrSpace = false;
}

CLogEditCtrl::~CLogEditCtrl(){
}

void CLogEditCtrl::Localize(){
}

void CLogEditCtrl::Init(LPCTSTR pszTitle)
{
	TCHAR szClassName[MAX_PATH];
	GetClassName(*this, szClassName, ARRSIZE(szClassName));
	m_bRichEdit = _tcsicmp(szClassName, _T("EDIT")) != 0;

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
	FlushBuffer();
}

LRESULT CLogEditCtrl::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
		case WM_ERASEBKGND:
			if (m_bNoPaint)
				return TRUE;
		case WM_PAINT:
			if (m_bNoPaint)
				return TRUE;
	}
	return CEdit::WindowProc(message, wParam, lParam);
}

void CLogEditCtrl::FlushBuffer()
{
	if (m_astrBuff.GetSize() > 0){ // flush buffer
		for (int i = 0; i < m_astrBuff.GetSize(); i++)
			AddLine(m_astrBuff[i]);
		m_astrBuff.RemoveAll();
	}
}

void CLogEditCtrl::AddEntry(LPCTSTR pszMsg)
{
	CString strLine(pszMsg);
	strLine += _T("\r\n");
	if (m_hWnd == NULL){
		m_astrBuff.Add(strLine);
	}
	else{
		FlushBuffer();
		AddLine(strLine);
	}
}

//////////////////////////////////////////////////////////////////////////////
// This function is based on Daniel Lohmann's article "CEditLog - fast logging
// into an edit control with cout" at http://www.codeproject.com
void CLogEditCtrl::AddLine(LPCTSTR pszMsg)
{
	int iMsgLen = _tcslen(pszMsg);
	if (iMsgLen == 0)
		return;
#ifdef _DEBUG
	if (pszMsg[iMsgLen - 1] == _T('\n'))
		ASSERT( iMsgLen >= 2 && pszMsg[iMsgLen - 2] == _T('\r') );
#endif

	// Get Edit contents dimensions and cursor position
	int iStartChar, iEndChar;
	GetSel(iStartChar, iEndChar);
	int iSize = GetWindowTextLength();

	if (iStartChar == iSize && iSize == iEndChar)
	{
		// The cursor resides at the end of text
		SCROLLINFO si;
		si.cbSize = sizeof si;
		si.fMask = SIF_ALL;
		if (m_bAutoScroll && GetScrollInfo(SB_VERT, &si) && si.nPos >= (int)(si.nMax - si.nPage + 1))
		{
			// Not scrolled away
			SafeAddLine(iSize, pszMsg, iStartChar, iEndChar);
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
			SafeAddLine(iSize, pszMsg, iStartChar, iEndChar);
			SetSel(iStartChar, iEndChar, TRUE); // Restore our previous selection

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
		int nFirstLine = !m_bAutoScroll ? GetFirstVisibleLine() : 0;
	
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
		SafeAddLine(iSize, pszMsg, iStartChar, iEndChar);
		SetSel(iStartChar, iEndChar, TRUE); // Restore our previous selection

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

void CLogEditCtrl::OnEnErrspace()
{
	m_bEnErrSpace = true;
}

void CLogEditCtrl::OnEnMaxtext()
{
	m_bEnErrSpace = true;
}

void CLogEditCtrl::ScrollToLastLine()
{
	// WM_VSCROLL does not work correctly under Win98 (or older version of comctl.dll)
	//SendMessage(WM_VSCROLL, SB_BOTTOM, 0);
	LineScroll(GetLineCount());
}

void CLogEditCtrl::SafeAddLine(int nPos, LPCTSTR pszLine, int& iStartChar, int& iEndChar)
{
	m_bEnErrSpace = false;
	SetSel(nPos, nPos, TRUE);
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
			SetSel(nPos, -1, TRUE);
			ReplaceSel(_T(""));

			// delete 1st line
			int iLine0Len = LineLength(0) + 2; // add NL character
			SetSel(0, iLine0Len, TRUE);
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
			SetSel(nPos, nPos, TRUE);
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

void CLogEditCtrl::Reset(){
	m_astrBuff.RemoveAll();
	SetRedraw(FALSE);
	SetWindowText(_T(""));
	SetRedraw();
	if (m_bRichEdit)
		Invalidate();
}

void CLogEditCtrl::OnContextMenu(CWnd* pWnd, CPoint point){
	int iSelStart, iSelEnd;
	GetSel(iSelStart, iSelEnd);
	int iTextLen = GetWindowTextLength();

	m_LogMenu.EnableMenuItem(MP_COPYSELECTED, iSelEnd > iSelStart ? MF_ENABLED : MF_GRAYED);
	m_LogMenu.EnableMenuItem(MP_REMOVEALL, iTextLen > 0 ? MF_ENABLED : MF_GRAYED);
	m_LogMenu.EnableMenuItem(MP_SELECTALL, iTextLen > 0 ? MF_ENABLED : MF_GRAYED);
	m_LogMenu.CheckMenuItem(MP_AUTOSCROLL, m_bAutoScroll ? MF_CHECKED : MF_UNCHECKED);
	m_LogMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
}

BOOL CLogEditCtrl::OnCommand(WPARAM wParam, LPARAM lParam){
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

CString CLogEditCtrl::GetLastLogEntry(){
	CString strLog;
	int iLastLine = GetLineCount() - 2;
	if (iLastLine >= 0){
		GetLine(iLastLine, strLog.GetBuffer(1024), 1024);
		strLog.ReleaseBuffer();
	}
	return strLog;
}

CString CLogEditCtrl::GetAllLogEntries(){
	CString strLog;
	GetWindowText(strLog);
	return strLog;
}

void CLogEditCtrl::SelectAllItems()
{
	SetSel(0, -1, TRUE);
}

void CLogEditCtrl::CopySelectedItems()
{
	Copy();
}

void CLogEditCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
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

	CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}
