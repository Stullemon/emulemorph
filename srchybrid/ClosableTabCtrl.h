#pragma once

#define WM_CLOSETAB		(WM_USER + 0x101)
#define	WM_QUERYTAB		(WM_USER + 0x102)
#define	WM_DBLCLICKTAB	(WM_USER + 0x103)

class CClosableTabCtrl : public CTabCtrl
{
	DECLARE_DYNAMIC(CClosableTabCtrl)

public:
	CClosableTabCtrl();
	virtual ~CClosableTabCtrl();

	bool m_bCloseable;

protected:
	CImageList m_ImgLstCloseButton;
	IMAGEINFO m_iiCloseButton;
	CPoint m_ptCtxMenu;

	void SetAllIcons();
	void GetCloseButtonRect(const CRect& rcItem, CRect& rcCloseButton);

	virtual void PreSubclassWindow();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSysColorChange();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
};
