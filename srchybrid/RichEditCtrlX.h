#pragma once

/////////////////////////////////////////////////////////////////////////////
// CRichEditCtrlX window

class CRichEditCtrlX : public CRichEditCtrl
{
public:
	CRichEditCtrlX();

	void SetDisableSelectOnFocus(bool bDisable = true);
	void SetSyntaxColoring(const LPCTSTR* ppszKeywords = NULL, LPCTSTR pszSeperators = NULL);

	CRichEditCtrlX& operator<<(LPCTSTR psz);
	CRichEditCtrlX& operator<<(char* psz);
	CRichEditCtrlX& operator<<(UINT uVal);
	CRichEditCtrlX& operator<<(int iVal);
	CRichEditCtrlX& operator<<(double fVal);

protected:
	bool m_bDisableSelectOnFocus;

	bool m_bSelfUpdate;
	CStringArray m_astrKeywords;
	CString m_strSeperators;
	CHARFORMAT m_cfDef;
	CHARFORMAT m_cfKeyword;

	void UpdateSyntaxColoring();

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg UINT OnGetDlgCode();
	afx_msg BOOL OnEnLink(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnEnChange();
};
