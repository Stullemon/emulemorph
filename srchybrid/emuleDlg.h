//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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
#include "TrayDialog.h"
#include "MeterIcon.h"
#include "TitleMenu.h"

namespace Kademlia {
	class CSearch;
	class CContact;
	class CEntry;
	class CUInt128;
};

class CChatWnd;
class CIrcWnd;
class CKademliaWnd;
class CKnownFileList; 
class CMainFrameDropTarget;
class CMuleStatusBarCtrl;
class CMuleToolbarCtrl;
class CPreferencesDlg;
class CSearchDlg;
class CServerWnd;
class CSharedFilesWnd;
class CStatisticsDlg;
class CTaskbarNotifier;
class CTransferWnd;
struct Status;
class CSplashScreen;
class CMuleSystrayDlg;

// emuleapp <-> emuleapp
#define OP_ED2KLINK				12000
#define OP_CLCOMMAND			12001

#define	EMULE_HOTMENU_ACCEL		'x'
#define	EMULSKIN_BASEEXT		_T("eMuleSkin")

class CemuleDlg : public CTrayDialog, public CLoggable
{
	friend class CMuleToolbarCtrl;
// Konstruktion
public:
	CemuleDlg(CWnd* pParent = NULL);	// Standardkonstruktor
	~CemuleDlg();
	enum { IDD = IDD_EMULE_DIALOG };

	void			AddServerMessageLine(LPCTSTR line);
	void			ShowConnectionState();
	void			ShowNotifier(CString Text, int MsgType, bool ForceSoundOFF = false); 
	void			ShowUserCount();
	void			ShowMessageState(uint8 iconnr);
	void			SetActiveDialog(CWnd* dlg);
	void			ShowTransferRate(bool forceAll=false);
	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	void			ShowPing();
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
	void			Localize();

	// Logging
	void			AddLogText(bool addtostatusbar,const CString& txt, bool bDebug);
	void			ResetLog();
	void			ResetDebugLog();
	CString			GetLastLogEntry();
	CString			GetLastDebugLogEntry();
	CString			GetAllLogEntries();
	CString			GetAllDebugLogEntries();

	void			OnCancel();
	void			StopTimer();
	// Barry - To find out if app is running or shutting/shut down
	bool			IsRunning();
	void			DoVersioncheck(bool manual);
	void			ApplyHyperTextFont(LPLOGFONT pFont);
	void			ApplyLogFont(LPLOGFONT pFont);
	void			SetKadButtonState();
	void			ProcessED2KLink(LPCTSTR pszData);
	void			SetStatusBarPartsSize();

	virtual void HtmlHelp(DWORD_PTR dwData, UINT nCmd = 0x000F);

	CTransferWnd*	transferwnd;
	CServerWnd*		serverwnd;
	CPreferencesDlg* preferenceswnd;
	CSharedFilesWnd* sharedfileswnd;
	CSearchDlg*		searchwnd;
	CChatWnd*		chatwnd;
	CMuleStatusBarCtrl* statusbar;
	CStatisticsDlg*  statisticswnd;
	CIrcWnd*		ircwnd;
	CTaskbarNotifier* m_wndTaskbarNotifier;
	CMuleToolbarCtrl* toolbar;
	CKademliaWnd*	kademliawnd;
	CWnd*			activewnd;
	uint8			status;
	CFont			m_fontHyperText;
	CFont			m_fontMarlett;
	CFont			m_fontLog;
	//EastShare, Added by linekin HotKey
	LRESULT			OnHotKey(WPARAM wParam, LPARAM lParam); 
	//EastShare, Added by linekin HotKey
protected:
	HICON m_hIcon;

	CSplashScreen *m_pSplashWnd;
	DWORD m_dwSplashTime;
	void ShowSplash();
	void DestroySplash();

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnTrayRButtonUp(CPoint pt);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSize(UINT nType,int cx,int cy);
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButton2();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnBnClickedHotmenu();
	afx_msg LRESULT OnMenuChar(UINT nChar, UINT nFlags, CMenu* pMenu);
	afx_msg void OnSysColorChange();
	afx_msg BOOL OnQueryEndSession();
	afx_msg void OnEndSession(BOOL bEnding);
	afx_msg LRESULT OnKickIdle(UINT nWhy, long lIdleCount);

	// quick-speed changer -- based on xrmb
	afx_msg void QuickSpeedUpload(UINT nID);
	afx_msg void QuickSpeedDownload(UINT nID);
	afx_msg void QuickSpeedOther(UINT nID);
	// end of quick-speed changer
	
	afx_msg LRESULT OnTaskbarNotifierClicked(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnWMData(WPARAM wParam,LPARAM lParam);
	// SLUGFILLER: SafeHash
	afx_msg LRESULT OnFileHashed(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnHashFailed(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnPartHashedOK(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnPartHashedCorrupt(WPARAM wParam,LPARAM lParam);
	// SLUGFILLER: SafeHash
	afx_msg LRESULT OnFileAllocExc(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnFileCompleted(WPARAM wParam,LPARAM lParam);
	afx_msg LRESULT OnFileOpProgress(WPARAM wParam,LPARAM lParam);

	//Framegrabbing
	afx_msg LRESULT OnFrameGrabFinished(WPARAM wParam,LPARAM lParam);

	afx_msg LRESULT OnAreYouEmule(WPARAM, LPARAM);

	//Webserver [kuchin]
	afx_msg LRESULT OnWebServerConnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWebServerDisonnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWebServerRemove(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWebSharedFilesReload(WPARAM wParam, LPARAM lParam);

	// VersionCheck DNS
	afx_msg LRESULT OnVersionCheckResponse(WPARAM wParam, LPARAM lParam);

	// Peercache DNS
	afx_msg LRESULT OnPeerCacheResponse(WPARAM wParam, LPARAM lParam);
	
	void OnOK() {}
	void OnClose();
	bool CanClose();

private:
	bool			ready;
	bool			startUpMinimizedChecked;
	bool			m_bStartMinimized;
	HICON			connicons[9];
	HICON			transicons[4];
	HICON			imicons[3];
	HICON			mytrayIcon;
	HICON			usericon;
	CMeterIcon		trayIcon;
	HICON			sourceTrayIcon;		// do not use those icons for anything else than the traybar!!!
	HICON			sourceTrayIconGrey;	// do not use those icons for anything else than the traybar!!!
	HICON			sourceTrayIconLow;	// do not use those icons for anything else than the traybar!!!
	int				m_iMsgIcon;

	uint32			lastuprate;
	uint32			lastdownrate;
	CImageList		imagelist;
	CTitleMenu		trayPopup;
	CMuleSystrayDlg* m_pSystrayDlg;
	CMainFrameDropTarget* m_pDropTarget;
	CMenu			m_SysMenuOptions;
	CMenu			m_menuUploadCtrl;
	CMenu			m_menuDownloadCtrl;

	UINT_PTR m_hTimer;
	static void CALLBACK StartupTimer(HWND hwnd, UINT uiMsg, UINT idEvent, DWORD dwTime);

	void StartConnection();
	void CloseConnection();
	void RestoreWindow();
	void UpdateTrayIcon(int procent);
	void ShowConnectionStateIcon();
	void ShowTransferStateIcon();
	void ShowUserStateIcon();
	void AddSpeedSelectorSys(CMenu* addToMenu);
	int  GetRecMaxUpload();
	void LoadNotifier(CString configuration); //<<--enkeyDEV(kei-kun) -TaskbarNotifier-
	bool notifierenabled;					  //<<-- enkeyDEV(kei-kun) -Quick disable/enable notifier-
	void ShowToolPopup(bool toolsonly=false);
	void SetAllIcons();

	char m_acVCDNSBuffer[MAXGETHOSTSTRUCT];

	// Mighty Knife: extended debug logging
private:
	int m_ExtDebugMessagesCount;
	CStringList m_ExtDebugMessages;
public:
	void AddExtDebugMessage (CString _line,...);
	void AddExtDebugDump (CString _headline, const char* _data, int _size, CString _subscript="");
	void OutputExtDebugMessages ();
	// [end] Mighty Knife
};


// ALL emuledlg WM_USER messages are to be declared here!!
enum EEmuleUserMsgs
{
	// Do *NOT* use any WM_USER messages in the range WM_USER - WM_USER+0x100!

	// Taskbar
	WM_TASKBARNOTIFIERCLICKED = WM_USER + 0x101,
	WM_TRAY_ICON_NOTIFY_MESSAGE,

	// Webserver
	WEB_CONNECT_TO_SERVER,
	WEB_DISCONNECT_SERVER,
	WEB_REMOVE_SERVER,
	WEB_SHARED_FILES_RELOAD,

	// VC
	WM_VERSIONCHECK_RESPONSE,

	// PC
	WM_PEERCHACHE_RESPONSE,

	// Mighty Knife: CRC32-Tag - save rename
	WM_CRC32_RENAMEFILE,
	WM_CRC32_UPDATEFILE
	// [end] Mighty Knife
};

enum EEmlueAppMsgs
{
	//thread messages
	TM_FINISHEDHASHING = WM_APP + 10,
	// SLUGFILLER: SafeHash - new handling
	TM_HASHFAILED,
	TM_PARTHASHEDOK,
	TM_PARTHASHEDCORRUPT,
	// SLUGFILLER: SafeHash
	TM_FRAMEGRABFINISHED,
	TM_FILEALLOCEXC,
	TM_FILECOMPLETED,
	TM_FILEOPPROGRESS
};
