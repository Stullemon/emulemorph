#pragma once

class CMuleStatusBarCtrl : public CStatusBarCtrl
{
	DECLARE_DYNAMIC(CMuleStatusBarCtrl)

public:
	CMuleStatusBarCtrl();
	virtual ~CMuleStatusBarCtrl();

	void Init(void);

protected:
	int GetPaneAtPosition(CPoint& point);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLButtonDblClk(UINT nFlags,CPoint point);
};
