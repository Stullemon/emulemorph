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
#include "resource.h"
#include "loggable.h"

#define	DEFAULT_NICK		thePrefs.GetHomepageBaseURL()
#define	DEFAULT_TCP_PORT	4662

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
class CStatistics;
class CAbstractFile;
class CUpDownClient;
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

	//MORPH - Added by SiRoB Yun.SF3, ZZ Upload system (USS)
	UploadBandwidthThrottler* uploadBandwidthThrottler; 
	LastCommonRouteFinder*    lastCommonRouteFinder; 
	//MORPH - Added by SiRoB Yun.SF3, ZZ Upload system (USS)
	CemuleDlg*			emuledlg;
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
	CStatistics*		statistics;
	CFakecheck*			FakeCheck; //MORPH - Added by milobac, FakeCheck, FakeReport, Auto-updating
	CPPgBackup*			ppgbackup; //EastShare - Added by Pretender, TBH-AutoBackup
	CIP2Country*		ip2country; //EastShare - added by AndCycle, IP to Country

	uint64				stat_sessionReceivedBytes;
	uint64				stat_sessionSentBytes;
	uint64				stat_sessionSentBytesToFriend; //MORPH - Added by Yun.SF3, ZZ Upload System

	uint16				stat_reconnects;
	DWORD				stat_transferStarttime;
	DWORD				stat_serverConnectTime;
	DWORD				stat_starttime;
	HANDLE				m_hMutexOneInstance;
	uint16				stat_filteredclients;
	uint32				stat_leecherclients; //MORPH - Added by IceCream, Anti-leecher feature
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
	tagCOPYDATASTRUCT	sendstruct;

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
	void		PasteClipboard();
	bool		IsEd2kFileLinkInClipboard();
	bool		IsEd2kServerLinkInClipboard();
	bool		IsEd2kLinkInClipboard(LPCTSTR pszLinkType, int iLinkTypeLen);

	CString		CreateED2kSourceLink(const CAbstractFile* f);
	CString		CreateED2kHostnameSourceLink(const CAbstractFile* f);

	// clipboard (text)
	bool		CopyTextToClipboard( CString strText );
	CString		CopyTextFromClipboard();
	void		OnlineSig();
	void		UpdateReceivedBytes(uint32 bytesToAdd);
	void		UpdateSentBytes(uint32 bytesToAdd, bool sentToFriend = false); //MORPH - Added by Yun.SF3, ZZ Upload System
	int			GetFileTypeSystemImageIdx(LPCTSTR pszFilePath, int iLength = -1);
	HIMAGELIST	GetSystemImageList() { return m_hSystemImageList; }
	CSize		GetSmallSytemIconSize() { return m_sizSmallSystemIcon; }
	bool		IsConnected();
	bool		IsFirewalled();
	bool		DoCallback( CUpDownClient *client );
	uint32		GetID();

	// because nearly all icons we are loading are 16x16, the default size is specified as 16 and not as 32 nor LR_DEFAULTSIZE
	HICON		LoadIcon(LPCTSTR lpszResourceName, int cx = 16, int cy = 16, UINT uFlags = LR_DEFAULTCOLOR) const;
	HICON		LoadIcon(UINT nIDResource) const;
	HBITMAP		LoadImage(LPCTSTR lpszResourceName, LPCTSTR pszResourceType) const;
	HBITMAP		LoadImage(UINT nIDResource, LPCTSTR pszResourceType) const;
	void		ApplySkin(LPCTSTR pszSkinProfile);

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
