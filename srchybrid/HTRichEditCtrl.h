#pragma once

class CPreparedRTFText
{
public:
	CPreparedRTFText();
	~CPreparedRTFText();

	const CString& GetText();

	void AppendText(const CString& sText);
	void AppendKeyWord(const CString& sText, COLORREF iColor);

//protected:
	CString m_sText;
};

class CHTRichEditCtrl : public CRichEditCtrl
{
	DECLARE_DYNAMIC(CHTRichEditCtrl)

public:
	CHTRichEditCtrl();
	virtual ~CHTRichEditCtrl();

	void Init(LPCTSTR pszTitle);
	void Localize();

	void AddEntry(LPCTSTR pszMsg);
	void Reset();
	CString GetLastLogEntry();
	CString GetAllLogEntries();

	void AppendText(const CString& sText, bool bInvalidate = true);
	void AppendHyperLink(const CString& sText, const CString& sTitle, const CString& sCommand, const CString& sDirectory, bool bInvalidate = true);

	CString GetText() const;
	CPreparedRTFText* GetHyperText();
	void SetHyperText(CPreparedRTFText*, bool bInvalidate = true);
	void UpdateSize(bool bRepaint);

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
	CPreparedRTFText* m_pPreparedText;

	void AddLine(LPCTSTR pszMsg, bool bLink = false);
	void SelectAllItems();
	void CopySelectedItems();
	int GetMaxSize();
	void SafeAddLine(int nPos, LPCTSTR pszLine, long& nStartChar, long& nEndChar, bool bLink);
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
