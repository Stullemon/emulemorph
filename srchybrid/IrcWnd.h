#pragma once
#include "ResizableLib\ResizableDialog.h"
#include "afxcmn.h"
#include "afxwin.h"
#include "resource.h"
#include "ircmain.h"
#include "hypertextctrl.h"
#include "MuleListCtrl.h"
#include "ClosableTabCtrl.h"

// CIrcWnd dialog
#pragma pack(1) //???
struct ChannelList{
	CString name;
	CString users;
	CString desc;
};
#pragma pack()

#pragma pack(1)
struct Channel{
	CString	name;
	CPreparedHyperText log;
	CString title;
	CPtrList nicks;
	uint8 type;
	CStringArray history;
	uint16 history_pos;
	// Type is mainly so that we can use this for IRC and the eMule Messages..
	// 1-Status, 2-Channel list, 4-Channel, 5-Private Channel, 6-eMule Message(Add later)
};
#pragma pack()

#pragma pack(1)
struct Nick{
	CString nick;
	CString op;
	CString hop;
	CString voice;
	CString uop;
	CString owner;
	CString protect;
};
#pragma pack()

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
	virtual BOOL	OnInitDialog();
	virtual void	OnSize(UINT nType, int cx, int cy);
	virtual int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL	OnCommand(WPARAM wParam,LPARAM lParam );
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	afx_msg void	OnBnClickedBnIrcconnect();
	afx_msg void	OnBnClickedClosechat(int nItem=-1);
	afx_msg void	OnTcnSelchangeTab2(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()
private:
	CImageList		imagelist;
	bool			asc_sort[8];	 
	CIrcMain*		m_pIrcMain;
	CClosableTabCtrl		channelselect;
	CString			m_sSendString;
	bool			m_bConnected;	
	bool			m_bLoggedIn;
	Channel*		m_pCurrentChannel;

//Server Channel List
public:
	void		ResetServerChannelList();
	void		AddChannelToList( CString name, CString user, CString description );
	void		ScrollHistory(bool down);
protected:
	static	int		CALLBACK SortProcChanL(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	afx_msg	void	OnColumnClickChanL( NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg	void	OnNMRclickChanL(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnNMDblclkserverChannelList(NMHDR *pNMHDR, LRESULT *pResult);
private:
	CMuleListCtrl	serverChannelList;
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
protected:
	static	int		CALLBACK SortProcNick(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	afx_msg	void	OnColumnClickNick( NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg	void	OnNMRclickNick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnNMDblclkNickList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnNMClickNicklist(NMHDR *pNMHDR, LRESULT *pResult);
private:
	CMuleListCtrl	nickList;

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
//	afx_msg	void	OnNMRclickStatusWindow(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnBnClickedChatsend();
	LRESULT		OnCloseTab(WPARAM wparam, LPARAM lparam);
private:
	CHyperTextCtrl	statusWindow;
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
