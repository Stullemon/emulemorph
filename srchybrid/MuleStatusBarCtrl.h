#pragma once

// CMuleStatusbarCtrl

class CMuleStatusBarCtrl : public CStatusBarCtrl
{
	DECLARE_DYNAMIC(CMuleStatusBarCtrl)

public:
	CMuleStatusBarCtrl();
	virtual ~CMuleStatusBarCtrl();

protected:
	DECLARE_MESSAGE_MAP()
public:
//	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnLButtonDblClk(UINT nFlags,CPoint point);
	void Init(void);
//	void Localize(void);
private:
	int GetPaneAtPosition(CPoint& point);
};
