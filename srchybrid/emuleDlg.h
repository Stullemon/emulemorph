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
//MORPH START - Added by SiRoB, New Systray Popup from fusion
#include "MuleSystrayDlg.h"
//MORPH END   - Added by SiRoB, New Systray Popup from fusion

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

// Elandal:ThreadSafeLogging -->
class LogItem {
public:
    bool addtostatusbar;
    CString line;
};
//MORPH END   - Added by SiRoB, ZZ Upload System ZZUL-20030807-1911

// emuleapp <-> emuleapp
#define OP_ED2KLINK				12000
#define OP_CLCOMMAND			12001

class CemuleDlg : public CTrayDialog
{
// Konstruktion
public:
	CemuleDlg(CWnd* pParent = NULL);	// Standardkonstruktor
	~CemuleDlg();
	enum { IDD = IDD_EMULE_DIALOG };

	void			AddLogText(bool addtostatusbar,const CString& txt, bool bDebug);
	void			AddServerMessageLine(LPCTSTR line);
	void			ShowConnectionState();
	void			ShowNotifier(CString Text, int MsgType, bool ForceSoundOFF = false); 
	void			ShowUserCount();
	void			ShowMessageState(uint8 iconnr);
	void			SetActiveDialog(CDialog* dlg);
	void			ShowTransferRate(bool forceAll=false);
	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	void			ShowPing();
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
	void			Localize();
	void			ResetLog();
	void			ResetDebugLog();
	void			OnCancel();
	void			StopTimer();
	// Barry - To find out if app is running or shutting/shut down
	bool			IsRunning();
	void			DoVersioncheck(bool manual);
	CString			GetLastLogEntry();
	CString			GetLastDebugLogEntry();
	CString			GetAllLogEntries();
	CString			GetAllDebugLogEntries();
	void			ApplyHyperTextFont(LPLOGFONT pFont);
	void			SetKadButtonState();
	void			ProcessED2KLink(LPCTSTR pszData);

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

	CDialog*		activewnd;
	uint8			status;
	CFont			m_fontHyperText;
	//MORPH START - Added by SiRoB, ZZ Upload System ZZUL-20030807-1911
	// thread safe log calls
	void   QueueDebugLogLine(bool addtostatusbar,CString line,...);
	void   HandleDebugLogQueue();
	void   ClearDebugLogQueue();
	//MORPH END   - Added by SiRoB, ZZ Upload System ZZUL-20030807-1911
protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnTrayRButtonDown(CPoint pt);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

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

	//Framegrabbing
	afx_msg LRESULT OnFrameGrabFinished(WPARAM wParam,LPARAM lParam);

	afx_msg LRESULT OnAreYouEmule(WPARAM, LPARAM);

	//Webserver [kuchin]
	afx_msg LRESULT OnWebServerConnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWebServerDisonnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWebServerRemove(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnWebSharedFilesReload(WPARAM wParam, LPARAM lParam);

	// Jigle SOAP service
	afx_msg LRESULT OnJigleSearchResponse(WPARAM wParam, LPARAM lParam);

	// VersionCheck DNS
	afx_msg LRESULT OnVersionCheckResponse(WPARAM wParam, LPARAM lParam);

	// Kademlia message
	afx_msg LRESULT OnKademliaSearchAdd		(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaSearchRem		(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaSearchRef		(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaContactAdd	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaContactRem	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaContactRef	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaContactUpdate	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaIndexedAdd	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaIndexedRem	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaIndexedRef	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaResultFile	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaResultKeyword	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaRequestTCP	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaUpdateStatus	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaOverheadSend	(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnKademliaOverheadRecv	(WPARAM wParam, LPARAM lParam);

	void OnOK() {}
	void OnClose();
	bool CanClose();

private:
	bool			ready;
	bool			startUpMinimized;
	HICON			connicons[3];
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
	CMainFrameDropTarget* m_pDropTarget;

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
	int	 IsNewVersionAvailable();
	void LoadNotifier(CString configuration); //<<--enkeyDEV(kei-kun) -TaskbarNotifier-
	bool notifierenabled;					  //<<-- enkeyDEV(kei-kun) -Quick disable/enable notifier-
	void ShowToolPopup(bool toolsonly=false);
	void SetStatusBarPartsSize();
	void SetAllIcons();

	char m_acVCDNSBuffer[MAXGETHOSTSTRUCT];

	//MORPH START - Added by SiRoB, New Systray Popup from fusion
	CMuleSystrayDlg *m_pSystrayDlg;
	//MORPH END   - Added by SiRoB, New Systray Popup from fusion
	//MORPH START - Added by SiRoB, ZZ Upload System ZZUL-20030807-1911
	// thread safe log calls
	CCriticalSection queueLock;
	CTypedPtrList<CPtrList, LogItem*> m_LogQueue;
	//MORPH END   - Added by SiRoB, ZZ Upload System ZZUL-20030807-1911
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

	WM_JIGLE_SEARCH_RESPONSE,

	// VC
	WM_VERSIONCHECK_RESPONSE,

    // Messages sent to main app window from within Kademlia threads
    WM_KAD_SEARCHADD,
    WM_KAD_SEARCHREM,
    WM_KAD_SEARCHREF,
    WM_KAD_CONTACTADD,
    WM_KAD_CONTACTREM,
    WM_KAD_CONTACTREF,
    WM_KAD_CONTACTUPDATE,
    WM_KAD_INDEXEDADD,
    WM_KAD_INDEXEDREM,
    WM_KAD_INDEXEDREF,
    WM_KAD_RESULTFILE,
    WM_KAD_RESULTKEYWORD,
	WM_KAD_REQUESTTCP,
	WM_KAD_UPDATESTATUS,
	WM_KAD_OVERHEADSEND,
	WM_KAD_OVERHEADRECV
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
	TM_FRAMEGRABFINISHED
};



typedef struct
{
	uint32	searchID;
	const Kademlia::CUInt128* pcontactID;
	uint8	type;
	uint32	ip;
	uint16	tcp;
	uint16	udp;
	uint32	serverip;
	uint16	serverport;
} KADFILERESULT;

typedef struct
{
	uint32	searchID;
	const Kademlia::CUInt128* pfileID;
	LPCSTR	name;
	uint32	size;
	LPCSTR	type;
	uint16	numProperties;
	va_list	args;
} KADKEYWORDRESULT;

void CALLBACK KademliaSearchAddCallback		(Kademlia::CSearch* search);
void CALLBACK KademliaSearchRemCallback		(Kademlia::CSearch* search);
void CALLBACK KademliaSearchRefCallback		(Kademlia::CSearch* search);
void CALLBACK KademliaContactAddCallback	(Kademlia::CContact* contact);
void CALLBACK KademliaContactRemCallback	(Kademlia::CContact* contact);
void CALLBACK KademliaContactRefCallback	(Kademlia::CContact* contact);
void CALLBACK KademliaContactUpdateCallback	(Kademlia::CContact* contact);
void CALLBACK KademliaIndexedAddCallback	(Kademlia::CEntry* contact);
void CALLBACK KademliaIndexedRemCallback	(Kademlia::CEntry* contact);
void CALLBACK KademliaIndexedRefCallback	(Kademlia::CEntry* contact);
void CALLBACK KademliaResultFileCallback	(uint32 searchID, Kademlia::CUInt128 contactID, uint8 type, uint32 ip, uint16 tcp, uint16 udp, uint32 serverip, uint16 serverport);
void CALLBACK KademliaResultKeywordCallback	(uint32 searchID, Kademlia::CUInt128 fileID, LPCSTR name, uint32 size, LPCSTR type, uint16 numProperties, ...);
void CALLBACK KademliaRequestTCPCallback	(Kademlia::CContact* contact);
void CALLBACK KademliaUpdateStatusCallback	(Status* status);
void CALLBACK KademliaOverheadSendCallback	(uint32 size);
void CALLBACK KademliaOverheadRecvCallback	(uint32 size);

void KademliaSearchFile(uint32 searchID, const Kademlia::CUInt128* pcontactID, uint8 type, uint32 ip, uint16 tcp, uint16 udp, uint32 serverip, uint16 serverport);
void KademliaSearchKeyword(uint32 searchID, const Kademlia::CUInt128* pfileID, LPCSTR name, uint32 size, LPCSTR type, uint16 numProperties, va_list args);
