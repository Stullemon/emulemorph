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
#include "emuleDlg.h"
#include "knownfilelist.h"
#include "preferences.h"
#include "sockets.h"
#include "serverlist.h"
#include "sharedfilelist.h"
#include "listensocket.h"
#include "uploadqueue.h"
#include "downloadqueue.h"
#include "clientlist.h"
#include "clientcredits.h"
#include "friendlist.h"
#include "clientudpsocket.h"
#include "IPFilter.h"
#include <afxmt.h>
#include "Webserver.h"
#include "mmserver.h"
#include "KademliaMain.h"
#include "loggable.h"
#include "UploadBandwidthThrottler.h" //MORPH - Added by Yun.SF3, ZZ Upload System
#include "LastCommonRouteFinder.h" //MORPH - Added by SiRoB, ZZ Upload system (USS)
#include "fakecheck.h" //MORPH - Added by milobac, FakeCheck, FakeReport, Auto-updating
#include "PPgBackup.h" //EastShare - Added by Pretender, TBH-AutoBackup

#define MAX_NICK_LENGTH 49 // max. length of nick without trailing NUL char

class CSearchList;
class CUploadQueue;
class CListenSocket;
class CDownloadQueue;
class CScheduler;

enum AppState{
	APP_STATE_RUNNING=0,
   	APP_STATE_SHUTINGDOWN,
	APP_STATE_DONE
};

class CemuleApp : public CWinApp, public CLoggable
{
public:
	CemuleApp(LPCTSTR lpszAppName = NULL);
	UploadBandwidthThrottler* uploadBandwidthThrottler; //MORPH - Added by Yun.SF3, ZZ Upload System
	LastCommonRouteFinder*    lastCommonRouteFinder; //MORPH - Added by SiRoB, ZZ Upload system (USS)
	CKademliaMain*		kademlia;
	CemuleDlg*		emuledlg;
	CClientList*		clientlist;
	CKnownFileList*		knownfiles;
	CPreferences*		glob_prefs;
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
	CMutex				hashing_mut;
	virtual BOOL		InitInstance();
	CString*			pendinglink;
	tagCOPYDATASTRUCT  sendstruct; //added by Cax2 28/10/02 
	CIPFilter*			ipfilter;
	CWebServer*			webserver; // Webserver [kuchin]
	CScheduler*			scheduler;
	CMMServer*			mmserver;
	CFakecheck*			FakeCheck; //MORPH - Added by milobac, FakeCheck, FakeReport, Auto-updating
	CPPgBackup*			ppgbackup; //EastShare - Added by Pretender, TBH-AutoBackup

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
	ULONGLONG			m_ullComCtrlVer;
	int					m_iDfltImageListColorFlags;
	DWORD				m_dwProductVersionMS;
	DWORD				m_dwProductVersionLS;
	CString				m_strCurVersionLong;
	UINT				m_uCurVersionShort;
	UINT				m_uCurVersionCheck;

	CArray<CString,CString> webservices;
	AppState	m_app_state; // defines application state for shutdown 
// Implementierung
	// ed2k link functions
	CString		StripInvalidFilenameChars(CString strText, bool bKeepSpaces = true);
	CString		CreateED2kLink( CAbstractFile* f );
	CString		CreateED2kSourceLink( CAbstractFile* f );
	CString		CreateED2kHostnameSourceLink( CAbstractFile* f );	// itsonlyme: hostnameSource
	CString		CreateHTMLED2kLink( CAbstractFile* f );
	bool		CopyTextToClipboard( CString strText );
	CString		CopyTextFromClipboard();
	void		OnlineSig(); 
	void		UpdateReceivedBytes(int32 bytesToAdd);
	void		UpdateSentBytes(int32 bytesToAdd, bool sentToFriend = false); //MORPH - Added by Yun.SF3, ZZ Upload System

	int			GetFileTypeSystemImageIdx(LPCTSTR pszFilePath, int iLength = -1);
	HIMAGELIST	GetSystemImageList() { return m_hSystemImageList; }
	CSize		GetSmallSytemIconSize() { return m_sizSmallSystemIcon; }
	bool		IsConnected();
	uint32		GetID();
	bool		IsFirewalled();

	// because nearly all icons we are loading are 16x16, the default size is specified as 16 and not as 32 nor LR_DEFAULTSIZE
	HICON		LoadIcon(LPCTSTR lpszResourceName, int cx = 16, int cy = 16, UINT uFlags = LR_DEFAULTCOLOR) const;
	HICON		LoadIcon(UINT nIDResource) const;
	void		ApplySkin(LPCTSTR pszSkinProfile);

	DECLARE_MESSAGE_MAP()
protected:
	bool ProcessCommandline();
	void SetTimeOnTransfer();
	static BOOL CALLBACK SearchEmuleWindow(HWND hWnd, LPARAM lParam);
	void OnHelp();

	HIMAGELIST m_hSystemImageList;
	CMapStringToPtr m_aExtToSysImgIdx;
	CSize m_sizSmallSystemIcon;
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
