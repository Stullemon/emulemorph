#if !defined(AFX_TRAYMENUBTN_H__98CFE43E_EC13_4483_B464_6D98C9B51816__INCLUDED_)
#define AFX_TRAYMENUBTN_H__98CFE43E_EC13_4483_B464_6D98C9B51816__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TrayMenuBtn.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTrayMenuBtn window

class CTrayMenuBtn : public CWnd
{
	// Construction
public:
	CTrayMenuBtn();

	// Attributes
public:

	// Operations
public:

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrayMenuBtn)
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CTrayMenuBtn();

	// Generated message map functions
public:
	bool	m_bBold;
	bool	m_bMouseOver;
	bool	m_bNoHover;
	bool	m_bUseIcon;
	bool	m_bParentCapture;
	UINT	m_nBtnID;
	CSize	m_sIcon;
	HICON	m_hIcon;
	CString m_strText;
	CFont	m_cfFont;

	//{{AFX_MSG(CTrayMenuBtn)
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRAYMENUBTN_H__98CFE43E_EC13_4483_B464_6D98C9B51816__INCLUDED_)
