#pragma once
#include "closabletabctrl.h"

struct Channel;

class CIrcChannelTabCtrl : public CClosableTabCtrl
{
	DECLARE_DYNAMIC(CIrcChannelTabCtrl)
public:
	CIrcChannelTabCtrl();
	virtual ~CIrcChannelTabCtrl();
	void Init();
	void Localize();
	Channel* FindChannelByName(CString name);
	Channel* NewChannel(CString name, uint8 type);
	void RemoveChannel( CString channel );
	void DeleteAllChannel();
	bool ChangeChanMode( CString channel, CString nick, CString mode );
	void ScrollHistory(bool down);
	void Chatsend( CString send );
	CString m_sChannelModeSettingsTypeA;
	CString m_sChannelModeSettingsTypeB;
	CString m_sChannelModeSettingsTypeC;
	CString m_sChannelModeSettingsTypeD;
	CPtrList channelPtrList;
	Channel* m_pCurrentChannel;
protected:
	friend class CIrcWnd;

	DECLARE_MESSAGE_MAP()
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnTcnSelchangeTab2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSysColorChange();
	virtual BOOL OnCommand(WPARAM wParam,LPARAM lParam );
private:
	void SetActivity( CString channel, bool flag);
	int	GetTabUnderMouse(CPoint point);
	void SetAllIcons();
	CIrcWnd* m_pParent;
	CImageList m_imlIRC;
};
