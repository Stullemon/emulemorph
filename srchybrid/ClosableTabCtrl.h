#pragma once

#define WM_CLOSETAB		(WM_USER + 0x101)

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

	void SetAllIcons();
	void GetCloseButtonRect(const CRect& rcItem, CRect& rcCloseButton);

	virtual void PreSubclassWindow();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSysColorChange();
};
