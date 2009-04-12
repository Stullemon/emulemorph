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

#pragma once
#include <afxinet.h>
#include "ClientVersionInfo.h"

enum EPeerCacheStatus{
	// permanent failure 20..11
	PCS_NOTFOUND = 20,
	PCS_NOTVERIFIED = 19,
	PCS_DISABLED	= 18,

	// still trying to find & valdite 10..1
	PCS_DOINGLOOKUPS = 10,
	PCS_VALDITING	 = 9,
	PCS_NOINIT		 = 8,
	PCS_OWNIPUNKNOWN = 7,

	// success 0
	PCS_READY		= 0
};

enum EPCLookUpState{
	LUS_NONE = 0,
	LUS_BASEPCLOCATION,
	LUS_MYHOSTNAME,
	LUS_EXTPCLOCATION,
	LUS_FINISHED
};

////////////////////////////////////////////////////////////////////////////////////
/// CPeerCacheFinder

class CPeerCacheFinder
{
	friend class CPCValditeThread;
public:
	CPeerCacheFinder();
	~CPeerCacheFinder(void);

	void	Init(uint32 dwLastSearch, bool bLastSearchSuccess, bool bEnabled, uint16 nPort);
	void	Save();
	void	FoundMyPublicIPAddress(uint32 dwIP);
	bool	IsCacheAvailable() const;
	uint32	GetCacheIP()						const	{ return m_dwPCIP; }
	uint16	GetCachePort()						const	{ return m_nPCPort;}
	void	DownloadAttemptStarted()					{ m_nDownloadAttempts++; }
	void	DownloadAttemptFailed();
	void	AddBannedVersion(CClientVersionInfo cviVersion);
	void	AddAllowedVersion(CClientVersionInfo cviVersion);
	bool	IsClientPCCompatible(uint32 dwTagVersionInfo, UINT nClientSoft);
	bool	IsClientPCCompatible(const CClientVersionInfo& cviToCheck);
	LRESULT OnPeerCacheCheckResponse(WPARAM wParam, LPARAM lParam);

protected:
	void	DoLookUp(CStringA strHostname);
	void	DoReverseLookUp(uint32 dwIP);
	void	SearchForPC();
	void	ValditeDescriptorFile();

private:
	EPeerCacheStatus	m_PCStatus;
	EPCLookUpState		m_PCLUState;	
	uint32	m_dwPCIP;
	uint32	m_dwMyIP;
	CString m_strMyHostname;
	int		m_posCurrentLookUp;
	bool	m_bValdited;
	bool	m_bNotReSearched;
	bool	m_bNotReValdited;
	uint16	m_nPCPort;
	uint32	m_nDownloadAttempts;
	uint32	m_nFailedDownloads;
	CArray<CClientVersionInfo> liBannedVersions;
	CArray<CClientVersionInfo> liAllowedVersions;
	CMutex	m_SettingsMutex;

};

///////////////////////////////////////////////////////////////////////////////////////
/// CPCValditeThread

class CPCValditeThread : public CWinThread
{
	DECLARE_DYNCREATE(CPCValditeThread)

protected:
	CPCValditeThread();           // protected constructor used by dynamic creation
	virtual ~CPCValditeThread();
	DECLARE_MESSAGE_MAP()
	bool	Valdite();

public:
	virtual	BOOL	InitInstance();
	virtual int		Run();
	void	SetValues(CPeerCacheFinder* in_pOwner, uint32 dwPCIP, uint32 dwMyIP);

private:

	uint32 m_dwPCIP;
	uint32 m_dwMyIP;
	CPeerCacheFinder* m_pOwner;
	uint16 m_nPCPort;
};

///////////////////////////////////////////////////////////////////////////////////////
/// CPCReverseDnsThread

class CPCReverseDnsThread : public CWinThread
{
	DECLARE_DYNCREATE(CPCReverseDnsThread)

public:
	CPCReverseDnsThread()			{m_dwIP = 0; m_hwndAsyncResult = NULL;}
	BOOL InitInstance();

	DWORD m_dwIP;
	HWND m_hwndAsyncResult;
};

