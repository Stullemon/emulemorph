
// ColorFrameCtrl.h : header file
//

#ifndef __ColorFrameCtrl_H__
#define __ColorFrameCtrl_H__

/////////////////////////////////////////////////////////////////////////////
// CColorFrameCtrl window

class CColorFrameCtrl : public CWnd
{
// Construction
public:
	CColorFrameCtrl( );

// Attributes
public:
	void SetFrameColor(COLORREF color);
	void SetBackgroundColor(COLORREF color);

	// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CColorFrameCtrl)
	public:
	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID=NULL);
	//}}AFX_VIRTUAL

// Implementation
public:
	COLORREF m_crBackColor;        // background color
	COLORREF m_crFrameColor;       // frame color

	virtual ~CColorFrameCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CColorFrameCtrl)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy); 
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CRect  m_rectClient;
	CBrush m_brushBack;
	CBrush m_brushFrame;
};

/////////////////////////////////////////////////////////////////////////////
#endif
