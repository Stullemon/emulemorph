#pragma once
#include "afxcmn.h"
#include "loggable.h"

#define WM_CLOSETAB		(WM_USER + 0x101)

// CClosableTabCtrl

class CClosableTabCtrl : public CTabCtrl, public CLoggable
{
	DECLARE_DYNAMIC(CClosableTabCtrl)

public:
	CClosableTabCtrl();
	virtual ~CClosableTabCtrl();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
protected:
	virtual void PreSubclassWindow();
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
private:
	CImageList m_pImgLst;
public:
};


