#pragma once
#include "ResizableLib\ResizableDialog.h"
#include "MuleListCtrl.h"

class CIrcMain;
struct ChannelList;
struct Channel;
struct Nick;


///////////////////////////////////////////////////////////////////////////////
// CIrcNickListCtrl

class CIrcNickListCtrl : public CMuleListCtrl
{
	DECLARE_DYNAMIC(CIrcNickListCtrl)

public:
	CIrcNickListCtrl();

protected:
	friend class CIrcWnd;

	CIrcWnd* m_pParent;
	bool m_asc_sort[2];

	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
};


///////////////////////////////////////////////////////////////////////////////
// CIrcChannelListCtrl

class CIrcChannelListCtrl : public CMuleListCtrl
{
	DECLARE_DYNAMIC(CIrcChannelListCtrl)

public:
	CIrcChannelListCtrl();

protected:
	friend class CIrcWnd;

	CIrcWnd* m_pParent;
	bool m_asc_sort[3];

	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
};


///////////////////////////////////////////////////////////////////////////////
// CIrcWnd dialog

class CIrcWnd : public CResizableDialog
{
	DECLARE_DYNAMIC(CIrcWnd)

//IrcWnd
public:
	CIrcWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CIrcWnd();

	void		Localize();
	void		UpdateNickCount();
	bool		GetLoggedIn()				{return m_bLoggedIn;}
	void		SetLoggedIn( bool flag )	{m_bLoggedIn = flag;}
	void		SetSendFileString( CString in_file )	{m_sSendString = in_file;}
	CString		GetSendFileString()						{return m_sSendString;}
	bool		IsConnected()				{return m_bConnected;}
	void		UpdateFonts(CFont* pFont);
// Dialog Data
	enum { IDD = IDD_IRC };

protected:
	void SetAllIcons();
	virtual BOOL	OnInitDialog();
	virtual void	OnSize(UINT nType, int cx, int cy);
	virtual int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL	OnCommand(WPARAM wParam,LPARAM lParam );
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	afx_msg void	OnSysColorChange();
	afx_msg void	OnBnClickedBnIrcconnect();
	afx_msg void	OnBnClickedClosechat(int nItem=-1);
	afx_msg void	OnTcnSelchangeTab2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg LRESULT OnCloseTab(WPARAM wparam, LPARAM lparam);
	DECLARE_MESSAGE_MAP()

private:
	CImageList		m_imagelist;
	CIrcMain*		m_pIrcMain;
	CTabCtrl		channelselect;
	CString			m_sSendString;
	bool			m_bConnected;
	bool			m_bLoggedIn;
	Channel*		m_pCurrentChannel;

//Server Channel List
public:
	void		ResetServerChannelList();
	void		AddChannelToList( CString name, CString user, CString description );
	void		ScrollHistory(bool down);
private:
	CIrcChannelListCtrl serverChannelList;
	CPtrList		channelLPtrList;

//Nick List
public:
	Nick*		FindNickByName(CString channel, CString name);
	Nick*		NewNick(CString channel, CString nick);
	void		RefreshNickList( CString channel );
	bool		RemoveNick( CString channel, CString nick );
	void		DeleteAllNick( CString channel );
	void		DeleteNickInAll ( CString nick, CString message );
	bool		ChangeNick( CString channel, CString oldnick, CString newnick );
	bool		ChangeMode( CString channel, CString nick, CString mode );
	void		ParseChangeMode( CString channel, CString changer, CString commands, CString names );
	void		ChangeAllNick( CString oldnick, CString newnick );
//	void		SetNick( CString in_nick );
private:
	CIrcNickListCtrl nickList;

//Messages
public:
	void		AddStatus( CString recieved, ... );
	void		AddInfoMessage( CString channelName, CString recieved, ... );
	void		AddMessage( CString channelName, CString targetname, CString line,...);
	void		SetConnectStatus( bool connected );
	void		NoticeMessage( CString source, CString message );
	CString		StripMessageOfFontCodes( CString temp );
	CString		StripMessageOfColorCodes( CString temp );
	void		SetTitle( CString channel, CString title );
	void		SetActivity( CString channel, bool flag);
	void		SendString( CString send );
protected:
	afx_msg void OnBnClickedChatsend();
private:
	CEdit			titleWindow;
	CEdit			inputWindow;

//Channels
public:
	Channel*	FindChannelByName(CString name);
	Channel*	NewChannel(CString name, uint8 type);
	void		RemoveChannel( CString channel );
	void		DeleteAllChannel();
	void		JoinChannels();
protected:
private:
	CPtrList		channelPtrList;
};
