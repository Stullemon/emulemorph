#pragma once

#include "TitleMenu.h"

class CHTRichEditCtrl : public CRichEditCtrl
{
	DECLARE_DYNAMIC(CHTRichEditCtrl)

public:
	CHTRichEditCtrl();
	virtual ~CHTRichEditCtrl();

	void Init(LPCTSTR pszTitle);
	void SetTitle(LPCTSTR pszTitle);
	void Localize();

	void AddEntry(LPCTSTR pszMsg);
	void Add(LPCTSTR pszMsg, int iLen = -1);
	void Reset();
	CString GetLastLogEntry();
	CString GetAllLogEntries();
	bool SaveLog(LPCTSTR pszDefName = NULL);

	void AppendText(const CString& sText, bool bInvalidate = true);
	void AppendHyperLink(const CString& sText, const CString& sTitle, const CString& sCommand, const CString& sDirectory, bool bInvalidate = true);
	void AppendKeyWord(const CString& sText, COLORREF cr);
	void AppendColoredText(LPCTSTR pszText, COLORREF cr);

	CString GetText() const;

	void SetFont(CFont* pFont, BOOL bRedraw = TRUE);
	CFont* GetFont() const;

protected:
	bool m_bRichEdit;
	CTitleMenu m_LogMenu;
	int m_iMaxLogMessages;
	bool m_bAutoScroll;
	CStringArray m_astrBuff;
	bool m_bNoPaint;
	bool m_bEnErrSpace;
	CString m_strTitle;
	bool m_bRestoreFormat;
	CHARFORMAT m_cfDefault;

	void AddLine(LPCTSTR pszMsg, int iLen = -1, bool bLink = false, COLORREF cr = CLR_DEFAULT);
	void SelectAllItems();
	void CopySelectedItems();
	int GetMaxSize();
	void SafeAddLine(int nPos, LPCTSTR pszLine, long& nStartChar, long& nEndChar, bool bLink, COLORREF cr);
	void FlushBuffer();
	void AddString(int nPos, LPCTSTR pszString, bool bLink, COLORREF cr);
	void ScrollToLastLine();

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEnErrspace();
	afx_msg void OnEnMaxtext();
	afx_msg BOOL OnEnLink(NMHDR *pNMHDR, LRESULT *pResult);
};
