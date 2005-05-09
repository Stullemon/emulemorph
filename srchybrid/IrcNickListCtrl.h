#pragma once
#include "mulelistctrl.h"

struct Nick;

class CIrcNickListCtrl : public CMuleListCtrl
{
	DECLARE_DYNAMIC(CIrcNickListCtrl)

public:
	CIrcNickListCtrl();
	void Init();
	Nick* FindNickByName(CString channel, CString name);
	Nick* NewNick(CString channel, CString nick);
	void RefreshNickList( CString channel );
	bool RemoveNick( CString channel, CString nick );
	void DeleteAllNick( CString channel );
	void DeleteNickInAll ( CString nick, CString message );
	bool ChangeNick( CString channel, CString oldnick, CString newnick );
	bool ChangeNickMode( CString channel, CString nick, CString mode );
	void ChangeAllNick( CString oldnick, CString newnick );
	void UpdateNickCount();
	void Localize();
	CString m_sUserModeSettings;
	CString m_sUserModeSymbols;

protected:
	friend class CIrcWnd;
	uint8	m_sortindex;
	bool	m_sortorder;

	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL OnCommand(WPARAM wParam,LPARAM lParam );

private:
	CIrcWnd* m_pParent;

};
