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
#include "StdAfx.h"
#include "partfile.h"
#include "opcodes.h"
#include "types.h"
#include "sharedfilelist.h"
#include "preferences.h"
#include "loggable.h"
// khaos::categorymod+
#include "SelCategoryDlg.h"
// khaos::categorymod-

// SLUGFILLER: hostnameSources
#define WM_HOSTNAMERESOLVED		(WM_USER + 0x101)

class CSourceHostnameResolveWnd : public CWnd
{
// Construction
public:
	CSourceHostnameResolveWnd();
	virtual ~CSourceHostnameResolveWnd();

	void AddToResolve(const uchar* fileid, LPCTSTR pszHostname, uint16 port);

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnHostnameResolved(WPARAM wParam, LPARAM lParam);

private:
	struct Hostname_Entry {
		uchar fileid[16];
		CStringA strHostname;
		uint16 port;
	};
	CTypedPtrList<CPtrList, Hostname_Entry*> m_toresolve;
	char m_aucHostnameBuffer[MAXGETHOSTSTRUCT];
};
// SLUGFILLER: hostnameSources
class CDownloadQueue: public CLoggable
{
	friend class CAddFileThread;
	friend class CServerSocket;
public:
	CDownloadQueue(CPreferences* in_prefs,CSharedFileList* in_sharedfilelist);
	~CDownloadQueue();
	void	Process();
	void	Init();
	// khaos::categorymod+ Modified these three functions by adding and in some cases removing params.
	void	AddSearchToDownload(CSearchFile* toadd, uint8 cat = 0, uint16 useOrder = 0);
	void	AddSearchToDownload(CString link, uint8 cat = 0, uint16 useOrder = 0);
	void	AddFileLinkToDownload(class CED2KFileLink* pLink, bool AllocatedLink = false, bool SkipQueue = false);
	// khaos::categorymod-
	bool	IsFileExisting(const uchar* fileid);
	bool	IsPartFile(void* totest);
	bool	IsTempFile(const CString& rstrDirectory, const CString& rstrName) const;	// SLUGFILLER: SafeHash
	CPartFile*	GetFileByID(const uchar* filehash);
	CPartFile*	GetFileByIndex(int index);
	CPartFile*	GetFileByKadFileSearchID(uint32 ID );
	void    CheckAndAddSource(CPartFile* sender,CUpDownClient* source);
	void    CheckAndAddKnownSource(CPartFile* sender,CUpDownClient* source);
	bool	RemoveSource(CUpDownClient* toremove, bool updatewindow = true, bool bDoStatsUpdate = true); // delete later ->{ return RemoveSource(toremove,NULL,updatewindow);}
	void	DeleteAll();
	void	RemoveFile(CPartFile* toremove);
	uint32	GetDatarate()			{return datarate;}
	void	SortByPriority();
	void	CheckDiskspace();	// SLUGFILLER: checkDiskspace
	void	StopUDPRequests();
	CServer*	cur_udpserver;
	void	GetDownloadStats(int results[]);
	void	GetDownloadStats(int results[],uint64& pui64TotFileSize,uint64& pui64TotBytesLeftToTransfer,uint64& pui64TotNeededSpace);
	void	AddPartFilesToShare();
	// SLUGFILLER: mergeKnown - include part files in known.met
	int		SavePartFilesToKnown(CFile* file);
	uint32	GetPartFilesCount();
	// SLUGFILLER: mergeKnown
	void	AddDownload(CPartFile* newfile);
	CUpDownClient* GetDownloadClientByIP_UDP(uint32 dwIP, uint16 nUDPPort);
	void	UpdateDisplayedInfo(boolean force=false);
	// khaos::categorymod+
	bool	StartNextFile(int Category = -1, bool forceResume = false);
	void	StopPauseLastFile(int Mode = MP_PAUSE, int Category = -1);
	uint16	GetMaxCatResumeOrder(uint8 iCategory = 0);
	void	GetCategoryFileCounts(uint8 iCategory, int cntFiles[]);
	uint16	GetCategoryFileCount(uint8 iCategory);
	uint16	GetHighestAvailableSourceCount(int nCat = -1);
	uint16	GetCatActiveFileCount(uint8 iCategory);
	uint8	GetAutoCat(CString sFullName, ULONG nFileSize);
	bool	ApplyFilterMask(CString sFullName, uint8 nCat);
	// khaos::categorymod-
	void	AddDownDataOverheadSourceExchange(uint32 data)	{ m_nDownDataRateMSOverhead += data;
															  m_nDownDataOverheadSourceExchange += data;
															  m_nDownDataOverheadSourceExchangePackets++;}
	void	AddDownDataOverheadFileRequest(uint32 data)		{ m_nDownDataRateMSOverhead += data;
															  m_nDownDataOverheadFileRequest += data;
															  m_nDownDataOverheadFileRequestPackets++;}
	void	AddDownDataOverheadServer(uint32 data)			{ m_nDownDataRateMSOverhead += data;
															  m_nDownDataOverheadServer += data;
															  m_nDownDataOverheadServerPackets++;}
	void	AddDownDataOverheadOther(uint32 data)			{ m_nDownDataRateMSOverhead += data;
															  m_nDownDataOverheadOther += data;
															  m_nDownDataOverheadOtherPackets++;}
	uint32	GetDownDatarateOverhead()			{return m_nDownDatarateOverhead;}
	uint64	GetDownDataOverheadSourceExchange()	{return m_nDownDataOverheadSourceExchange;}
	uint64	GetDownDataOverheadFileRequest()	{return m_nDownDataOverheadFileRequest;}
	uint64	GetDownDataOverheadServer()			{return m_nDownDataOverheadServer;}
	uint64	GetDownDataOverheadOther()			{return m_nDownDataOverheadOther;}
	uint64	GetDownDataOverheadSourceExchangePackets()	{return m_nDownDataOverheadSourceExchangePackets;}
	uint64	GetDownDataOverheadFileRequestPackets()		{return m_nDownDataOverheadFileRequestPackets;}
	uint64	GetDownDataOverheadServerPackets()			{return m_nDownDataOverheadServerPackets;}
	uint64	GetDownDataOverheadOtherPackets()			{return m_nDownDataOverheadOtherPackets;}
	void	CompDownDatarateOverhead();
	int		GetFileCount()						{return filelist.GetCount();}
	void	ResetCatParts(int cat, uint8 useCat = 0);
	void	SavePartFiles(bool del = false);	// InterCeptor
	void	SetCatPrio(int cat, uint8 newprio);
	void	SetCatStatus(int cat, int newstatus);
	void	MoveCat(uint8 from, uint8 to);
	uint16	GetDownloadingFileCount();
	uint16	GetPausedFileCount();
	void	DisableAllA4AFAuto(void);
	//MORPH START - Removed by SiRoB, Due to Khaos Categorie
	//void	SetAutoCat(CPartFile* newfile);
	//MORPH END   - Removed by SiRoB, Due to Khaos Categorie
	void	SendLocalSrcRequest(CPartFile* sender);
	void	SetLastKademliaFileRequest()	{lastkademliafilerequest = ::GetTickCount();}
	bool	DoKademliaFileRequest()	{return ((::GetTickCount() - lastkademliafilerequest) > KADEMLIAASKTIME);}
	void	UpdatePNRFile(CPartFile * ppfChanged = NULL);							//<<-- enkeyDEV(ColdShine) -PartfileNameRecovery-
	void	BuildPNRRecord(CPartFile * ppf, char * pszBuff, unsigned cchBuffMax);	//<<-- enkeyDEV(ColdShine) -PartfileNameRecovery-

	// khaos::kmod+ Advanced A4AF: Brute Force
	CPartFile* forcea4af_file;
	// khaos::kmod-

protected:
	bool	SendNextUDPPacket();
	void	ProcessLocalRequests();
	int		GetMaxFilesPerUDPServerPacket() const;
	bool	SendGlobGetSourcesUDPPacket(CSafeMemFile& data);

private:
	// SLUGFILLER: checkDiskspace
	bool	CompareParts(POSITION pos1, POSITION pos2);
	void	SwapParts(POSITION pos1, POSITION pos2);
	void	HeapSort(uint16 first, uint16 last);
	// SLUGFILLER: checkDiskspace
	CTypedPtrList<CPtrList, CPartFile*> filelist;
	CTypedPtrList<CPtrList, CPartFile*> m_localServerReqQueue;
	CSharedFileList* sharedfilelist;
	CPreferences*	 app_prefs;
	uint16	filesrdy;
	uint32	datarate;
	
	CPartFile*	lastfile;
	uint32		lastcheckdiskspacetime;	// SLUGFILLER: checkDiskspace
	uint32		lastudpsearchtime;
	uint32		lastudpstattime;
	uint8		udcounter;
	uint8		m_cRequestsSentToServer;
	uint32		m_dwNextTCPSrcReq;
	int			m_iSearchedServers;
	uint32		lastkademliafilerequest;

	uint64		m_datarateMS;
	uint32		m_nDownDatarateOverhead;
	uint32		m_nDownDataRateMSOverhead;
	uint64		m_nDownDataOverheadSourceExchange;
	uint64		m_nDownDataOverheadSourceExchangePackets;
	uint64		m_nDownDataOverheadFileRequest;
	uint64		m_nDownDataOverheadFileRequestPackets;
	uint64		m_nDownDataOverheadServer;
	uint64		m_nDownDataOverheadServerPackets;
	uint64		m_nDownDataOverheadOther;
	uint64		m_nDownDataOverheadOtherPackets;

	// By BadWolf - Accurate Speed Measurement
	//CList<TransferredData,TransferredData> avarage_dr_list; //MORPH - Added by Yun.SF3, ZZ Upload System

	CList<TransferredData,TransferredData>	m_AvarageDDRO_list;
	uint32 sumavgDDRO;
	// END By BadWolf - Accurate Speed Measurement

	// khaos::categorymod+ For queuing ED2K link additions.
	bool		m_bBusyPurgingLinks;
	bool		PurgeED2KLinkQueue();
	uint32		m_iLastLinkQueuedTick;

	CTypedPtrList<CPtrList, CED2KFileLink*> m_ED2KLinkQueue;
	// khaos::categorymod-

	DWORD m_lastRefreshedDLDisplay;
	CSourceHostnameResolveWnd m_srcwnd;		// SLUGFILLER: hostnameSources
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	uint32 m_random_update_wait;
	//MORPH END   - Added by Yun.SF3, ZZ Upload System
};
