#pragma once

class CLogEditCtrl : public CEdit
{
	DECLARE_DYNAMIC(CLogEditCtrl)
public:
	CLogEditCtrl();
	virtual ~CLogEditCtrl();

	void Init(LPCTSTR pszTitle);
	void Localize();

	void AddEntry(LPCTSTR pszMsg);
	void Reset();
	CString GetLastLogEntry();
	CString GetAllLogEntries();

protected:
	bool m_bRichEdit;
	CTitleMenu m_LogMenu;
	int m_iMaxLogMessages;
	bool m_bAutoScroll;
	CStringArray m_astrBuff;
	bool m_bNoPaint;
	bool m_bEnErrSpace;

	void AddLine(LPCTSTR pszMsg);
	void SelectAllItems();
	void CopySelectedItems();
	int GetMaxSize();
	void SafeAddLine(int nPos, LPCTSTR pszLine, int& nStartChar, int& nEndChar);
	void FlushBuffer();
	void ScrollToLastLine();

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEnErrspace();
	afx_msg void OnEnMaxtext();
};
