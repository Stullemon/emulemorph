// ------------------------------------------------------------
//  CDialogMinTrayBtn template class
//  MFC CDialog with minimize to systemtray button (0.04)
//  Supports WinXP styles (thanks to David Yuheng Zhao for CVisualStylesXP - yuheng_zhao@yahoo.com)
// ------------------------------------------------------------
//  DialogMinTrayBtn.h
//  zegzav - 2002,2003 - eMule project (http://www.emule-project.net)
// ------------------------------------------------------------
#pragma once
#define HTMINTRAYBUTTON         65
#define SC_MINIMIZETRAY         0xE000

//bluecow/sony: moved out of class for VC 2003 compatiblity; zegzav: made extern for proper look (thanks)
extern BOOL (WINAPI *_TransparentBlt)(HDC, int, int, int, int, HDC, int, int, int, int, UINT);

template <class BASE= CDialog> class CDialogMinTrayBtn : public BASE
{
public:
    // constructor
    CDialogMinTrayBtn();
    CDialogMinTrayBtn(LPCTSTR lpszTemplateName, CWnd* pParentWnd = NULL);
    CDialogMinTrayBtn(UINT nIDTemplate, CWnd* pParentWnd = NULL);

    // methods
    void MinTrayBtnShow();
    void MinTrayBtnHide();
    BOOL MinTrayBtnIsVisible() const;
    void MinTrayBtnEnable();
    void MinTrayBtnDisable();
    BOOL MinTrayBtnIsEnabled() const;

	void SetWindowText(LPCTSTR lpszString);

protected:
    // messages
    virtual BOOL OnInitDialog();
    afx_msg void OnNcPaint();
    afx_msg BOOL OnNcActivate(BOOL bActive);
    afx_msg UINT OnNcHitTest(CPoint point);
    afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
    afx_msg void OnNcRButtonDown(UINT nHitTest, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT _OnThemeChanged();
    DECLARE_MESSAGE_MAP()

private:
    // internal methods
    void MinTrayBtnInit();
    void MinTrayBtnDraw();
    BOOL MinTrayBtnHitTest(CPoint point) const;
    void MinTrayBtnUpdatePosAndSize();

    void MinTrayBtnSetUp();
    void MinTrayBtnSetDown();

    const CPoint &MinTrayBtnGetPos() const;
    const CSize &MinTrayBtnGetSize() const;
    CRect MinTrayBtnGetRect() const;

    BOOL IsWindowsClassicStyle() const;
	INT GetVisualStylesXPColor() const;

	BOOL MinTrayBtnInitBitmap();

    // data members
    CPoint m_MinTrayBtnPos;
    CSize  m_MinTrayBtnSize;
    BOOL   m_bMinTrayBtnVisible; 
    BOOL   m_bMinTrayBtnEnabled; 
    BOOL   m_bMinTrayBtnUp;
    BOOL   m_bMinTrayBtnCapture;
    BOOL   m_bMinTrayBtnActive;
    BOOL   m_bMinTrayBtnHitTest;
    UINT_PTR m_nMinTrayBtnTimerId;
	CBitmap m_bmMinTrayBtnBitmap;
	BOOL	m_bMinTrayBtnWindowsClassicStyle;
	static const CHAR *m_pszMinTrayBtnBmpName[];
};

template <class BASE> inline const CPoint &CDialogMinTrayBtn<BASE>::MinTrayBtnGetPos() const
{
    return m_MinTrayBtnPos;
}

template <class BASE> inline const CSize &CDialogMinTrayBtn<BASE>::MinTrayBtnGetSize() const
{
    return m_MinTrayBtnSize;
}

template <class BASE> inline CRect CDialogMinTrayBtn<BASE>::MinTrayBtnGetRect() const
{
    return CRect(MinTrayBtnGetPos(), MinTrayBtnGetSize());
}

template <class BASE> inline BOOL CDialogMinTrayBtn<BASE>::MinTrayBtnIsVisible() const
{
    return m_bMinTrayBtnVisible;
}

template <class BASE> inline BOOL CDialogMinTrayBtn<BASE>::MinTrayBtnIsEnabled() const
{
    return m_bMinTrayBtnEnabled;
}

#include "DialogMinTrayBtn.hpp"
