#pragma once
#include "mulelistctrl.h"

class CIrcChannelListCtrl : public CMuleListCtrl
{
	DECLARE_DYNAMIC(CIrcChannelListCtrl)

public:
	CIrcChannelListCtrl();
	virtual ~CIrcChannelListCtrl();
	void ResetServerChannelList( bool b_shutdown = false );
	void AddChannelToList( CString name, CString user, CString description );
	void JoinChannels();
	void Localize();
	void Init();

protected:
	friend class CIrcWnd;

	CIrcWnd* m_pParent;
	bool m_asc_sort[3];

	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL OnCommand(WPARAM wParam,LPARAM lParam );

private:
	CPtrList channelLPtrList;

};
