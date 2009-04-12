//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#pragma once
#include "ClosableTabCtrl.h"
#include "HTRichEditCtrl.h"
#include "SplitterControl.h"

#define	IRC_TITLE_WND_DFLT_HEIGHT	36	// min. space for 2 lines with "MS Sans Serif" or "Verdana" at 10pt
#define	IRC_TITLE_WND_MIN_HEIGHT	(IRC_TITLE_WND_DFLT_HEIGHT/2)
#define	IRC_TITLE_WND_MAX_HEIGHT	(IRC_TITLE_WND_MIN_HEIGHT*6)
#define	IRC_CHANNEL_SPLITTER_HEIGHT	4

struct Nick;

struct Channel
{
	CString	m_sName;
	CString m_sModesA;
	CString m_sModesB;
	CString m_sModesC;
	CString m_sModesD;
	CHTRichEditCtrl m_wndTitle;
	CSplitterControl m_wndSplitter;
	CHTRichEditCtrl m_wndLog;
	CString m_sTitle;
	CTypedPtrList<CPtrList, Nick*> m_lstNicks;
	CStringArray m_astrHistory;
	int m_iHistoryPos;
	// Type is mainly so that we can use this for IRC and the eMule Messages..
	// 1-Status, 2-Channel list, 4-Channel, 5-Private Channel, 6-eMule Message(Add later)
	enum EType {
		ctStatus = 1,
		ctChannelList = 2,
		ctNormal = 4,
		ctPrivate = 5
	} m_eType;

	void Show();
	void Hide();
};

class CIrcChannelTabCtrl : public CClosableTabCtrl
{
	DECLARE_DYNAMIC(CIrcChannelTabCtrl)

public:
	CIrcChannelTabCtrl();
	virtual ~CIrcChannelTabCtrl();

	void Init();
	void Localize();
	Channel* FindChannelByName(const CString& sName);
	Channel* NewChannel(const CString& sName, Channel::EType uType);
	void RemoveChannel(const CString& sChannel);
	void SelectChannel(const Channel *pChannel);
	void DeleteAllChannels();
	bool ChangeChanMode(const CString& sChannel, const CString& sParam, const CString& sDir, const CString& sCommand);
	void ScrollHistory(bool bDown);
	void ChatSend(CString sSend);
	void SetActivity(Channel *pChannel, bool bFlag);
	void EnableSmileys(bool bEnable);

	CString m_sChannelModeSettingsTypeA;
	CString m_sChannelModeSettingsTypeB;
	CString m_sChannelModeSettingsTypeC;
	CString m_sChannelModeSettingsTypeD;
	CTypedPtrList<CPtrList, Channel*> m_lstChannels;
	Channel* m_pCurrentChannel;
	Channel* m_pChanStatus;
	Channel* m_pChanList;

protected:
	friend class CIrcWnd;

	CIrcWnd* m_pParent;
	CImageList m_imlistIRC;

	void SetAllIcons();
	int FindChannel(const Channel *pChannel);
	void SelectChannel(int iItem);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnTcnSelChange(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSysColorChange();
	afx_msg LRESULT OnCloseTab(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnQueryTab(WPARAM wParam, LPARAM lParam);
};
