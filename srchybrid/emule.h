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
#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif
#include ".\Optimizer\cpu_info.h" //Commander - Added: Optimizer [ePlus]
#include "resource.h"
#include "loggable.h"
#include "UPnPNat.h" //MORPH - Added by SiRoB, [MoNKi: -UPnPNAT Support-]
#include "WapServer/WapServer.h" //MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]

#define	DEFAULT_NICK		thePrefs.GetHomepageBaseURL()
#define	DEFAULT_TCP_PORT	4662
#define	DEFAULT_UDP_PORT	(DEFAULT_TCP_PORT+10)

class CSearchList;
class CUploadQueue;
class CListenSocket;
class CDownloadQueue;
class CScheduler;
class UploadBandwidthThrottler;
class LastCommonRouteFinder;
class CemuleDlg;
class CClientList;
class CKnownFileList;
class CServerConnect;
class CServerList;
class CSharedFileList;
class CClientCreditsList;
class CFriendList;
class CClientUDPSocket;
class CIPFilter;
class CWebServer;
class CMMServer;
class CAbstractFile;
class CUpDownClient;
class CPeerCacheFinder;
class CFirewallOpener;

struct SLogItem;
class CFakecheck; //MORPH - Added by milobac, FakeCheck, FakeReport, Auto-updating
#include "PPgBackup.h" //EastShare - Added by Pretender, TBH-AutoBackup
class CIP2Country; //EastShare - added by AndCycle, IP to Country

enum AppState{
	APP_STATE_RUNNING=0,
   	APP_STATE_SHUTINGDOWN,
	APP_STATE_DONE
};

class CemuleApp : public CWinApp, public CLoggable
{
public:
	CemuleApp(LPCTSTR lpszAppName = NULL);
    CPUInfo 		cpu; //Commander - Added: Optimizer [ePlus]	
	//MORPH - Added by SiRoB Yun.SF3, ZZ Upload system (USS)
	UploadBandwidthThrottler* uploadBandwidthThrottler; 
	LastCommonRouteFinder*    lastCommonRouteFinder; 
	//MORPH - Added by SiRoB Yun.SF3, ZZ Upload system (USS)
	CemuleDlg*		emuledlg;
	CClientList*		clientlist;
	CKnownFileList*		knownfiles;
	CServerConnect*		serverconnect;
	CServerList*		serverlist;	
	CSharedFileList*	sharedfiles;
	CSearchList*		searchlist;
	CListenSocket*		listensocket;
	CUploadQueue*		uploadqueue;
	CDownloadQueue*		downloadqueue;
	CClientCreditsList*	clientcredits;
	CFriendList*		friendlist;
	CClientUDPSocket*	clientudp;
	CIPFilter*			ipfilter;
	CWebServer*			webserver;
	CScheduler*			scheduler;
	CMMServer*			mmserver;
	CPeerCacheFinder*	m_pPeerCache;
	CFirewallOpener*	m_pFirewallOpener;

	CFakecheck*			FakeCheck; //MORPH - Added by milobac, FakeCheck, FakeReport, Auto-updating
	CPPgBackup*			ppgbackup; //EastShare - Added by Pretender, TBH-AutoBackup
	CIP2Country*		ip2country; //EastShare - added by AndCycle, IP to Country


	HANDLE				m_hMutexOneInstance;
	int					m_iDfltImageListColorFlags;
	DWORD				m_dwProductVersionMS;
	DWORD				m_dwProductVersionLS;
	CString				m_strCurVersionLong;
	UINT				m_uCurVersionShort;
	UINT				m_uCurVersionCheck;
	ULONGLONG			m_ullComCtrlVer;
	AppState			m_app_state; // defines application state for shutdown 
	CMutex				hashing_mut;
	CString*			pendinglink;
	COPYDATASTRUCT		sendstruct;

// Implementierung
	virtual BOOL InitInstance();

	// ed2k link functions
	//MORPH START - Changed by SiRoB, Selection category support khaos::categorymod+
	/*
	void		AddEd2kLinksToDownload(CString strLinks, uint8 cat);
	*/
	void		AddEd2kLinksToDownload(CString strLinks, int cat);
	//MORPH END   - Changed by SiRoB, Selection category support khaos::categorymod+
	void		SearchClipboard();
	void		IgnoreClipboardLinks(CString strLinks) {m_strLastClipboardContents = strLinks;}
	//MORPH START - Changed by SiRoB, Selection category support khaos::categorymod+
	/*
	void		PasteClipboard(uint8 uCategory = 0);
	*/
	void		PasteClipboard(int Cat = -1);
	//MORPH END   - Changed by SiRoB, Selection category support khaos::categorymod+
	bool		IsEd2kFileLinkInClipboard();
	bool		IsEd2kServerLinkInClipboard();
	bool		IsEd2kLinkInClipboard(LPCSTR pszLinkType, int iLinkTypeLen);

	CString		CreateED2kSourceLink(const CAbstractFile* f);
//	CString		CreateED2kHostnameSourceLink(const CAbstractFile* f);
	CString		CreateKadSourceLink(const CAbstractFile* f);

	// clipboard (text)
	bool		CopyTextToClipboard( CString strText );
	CString		CopyTextFromClipboard();
	void		OnlineSig();
	void		UpdateReceivedBytes(uint32 bytesToAdd);
	void		UpdateSentBytes(uint32 bytesToAdd, bool sentToFriend = false);
	int			GetFileTypeSystemImageIdx(LPCTSTR pszFilePath, int iLength = -1);
	HIMAGELIST	GetSystemImageList() { return m_hSystemImageList; }
	CSize		GetSmallSytemIconSize() { return m_sizSmallSystemIcon; }
	bool		IsPortchangeAllowed();
	bool		IsConnected();
	bool		IsFirewalled();
	bool		DoCallback( CUpDownClient *client );
	uint32		GetID();
	uint32		GetPublicIP() const;	// return current (valid) public IP or 0 if unknown
	void		SetPublicIP(const uint32 dwIP);

	// because nearly all icons we are loading are 16x16, the default size is specified as 16 and not as 32 nor LR_DEFAULTSIZE
	HICON		LoadIcon(LPCTSTR lpszResourceName, int cx = 16, int cy = 16, UINT uFlags = LR_DEFAULTCOLOR) const;
	HICON		LoadIcon(UINT nIDResource) const;
	HBITMAP		LoadImage(LPCTSTR lpszResourceName, LPCTSTR pszResourceType) const;
	HBITMAP		LoadImage(UINT nIDResource, LPCTSTR pszResourceType) const;
	bool		LoadSkinColor(LPCTSTR pszKey, COLORREF& crColor);
	void		ApplySkin(LPCTSTR pszSkinProfile);

	CString		GetLangHelpFilePath();
	void		SetHelpFilePath(LPCTSTR pszHelpFilePath);
	void		ShowHelp(UINT uTopic, UINT uCmd = HELP_CONTEXT);

    // Elandal:ThreadSafeLogging -->
    // thread safe log calls
    void			QueueDebugLogLine(bool addtostatusbar, LPCTSTR line,...);
    void			HandleDebugLogQueue();
    void			ClearDebugLogQueue(bool bDebugPendingMsgs = false);
    void			QueueLogLine(bool addtostatusbar, LPCTSTR line,...);
    void			HandleLogQueue();
    void			ClearLogQueue(bool bDebugPendingMsgs = false);
    // Elandal:ThreadSafeLogging <--

	bool			DidWeAutoStart() { return m_bAutoStart; }

protected:
	bool ProcessCommandline();
	void SetTimeOnTransfer();
	static BOOL CALLBACK SearchEmuleWindow(HWND hWnd, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnHelp();

	HIMAGELIST m_hSystemImageList;
	CMapStringToPtr m_aExtToSysImgIdx;
	CSize m_sizSmallSystemIcon;

	bool		m_bGuardClipboardPrompt;
	CString		m_strLastClipboardContents;

    // Elandal:ThreadSafeLogging -->
    // thread safe log calls
    CCriticalSection m_queueLock;
    CTypedPtrList<CPtrList, SLogItem*> m_QueueDebugLog;
    CTypedPtrList<CPtrList, SLogItem*> m_QueueLog;
    // Elandal:ThreadSafeLogging <--

	uint32 m_dwPublicIP;
	bool m_bAutoStart;
public:
	void OptimizerInfo(void); // Commander - Added: Optimizer [ePlus]
       // Commander - Added: Custom incoming folder icon [emulEspaña] - Start
	void	AddIncomingFolderIcon();
	void	RemoveIncomingFolderIcon();
       // Commander - Added: Custom incoming folder icon [emulEspaña] - End

	//MORPH START - Added by SiRoB, [MoNKi: -UPnPNAT Support-]
public:
	CUPnPNat	m_UPnPNat;
	BOOL		AddUPnPNatPort(CUPnPNat::UPNPNAT_MAPPING *mapping, bool tryRandom = false);
	BOOL		RemoveUPnPNatPort(CUPnPNat::UPNPNAT_MAPPING *mapping);
	//MORPH END   - Added by SiRoB, [MoNKi: -UPnPNAT Support-]

	//MORPH START - Added by SiRoB, [itsonlyme: -modname-]
	public:
		CString		m_strModVersion;
		CString		m_strModLongVersion;
	//MORPH END   - Added by SiRoB, [itsonlyme: -modname-]

//MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
public:
	CWapServer*		wapserver;
//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]
// Added by Announ [Announ: -Friend eLinks-]
	public:
		bool	IsEd2kFriendLinkInClipboard();
// End -Friend eLinks-
};

extern CemuleApp theApp;


//////////////////////////////////////////////////////////////////////////////
// CTempIconLoader

class CTempIconLoader
{
public:
	// because nearly all icons we are loading are 16x16, the default size is specified as 16 and not as 32 nor LR_DEFAULTSIZE
	CTempIconLoader(LPCTSTR pszResourceID, int cx = 16, int cy = 16, UINT uFlags = LR_DEFAULTCOLOR);
	~CTempIconLoader();

	operator HICON() const{
		return this == NULL ? NULL : m_hIcon;
	}

protected:
	HICON m_hIcon;
};

extern CLog theLog;
extern CLog theVerboseLog;
