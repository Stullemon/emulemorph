#pragma once
#include "loggable.h"

#define WM_CLOSETAB		(WM_USER + 0x101)

class CClosableTabCtrl : public CTabCtrl, public CLoggable
{
	DECLARE_DYNAMIC(CClosableTabCtrl)

public:
	CClosableTabCtrl();
	virtual ~CClosableTabCtrl();

protected:
	CImageList m_ImgLst;

	void SetAllIcons();

	virtual void PreSubclassWindow();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSysColorChange();
};
