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
#include "types.h"
#include "opcodes.h"
#include "MD5Sum.h"
#include "loggable.h"


const CString strDefaultToolbar = _T("0099010203040506070899091011");

// DO NOT EDIT VALUES like making a uint16 to uint32, or insert any value. ONLY append new vars
#pragma pack(1)
struct Preferences_Ext_Struct{
	int8	version;
	uchar	userhash[16];
	WINDOWPLACEMENT EmuleWindowPlacement;
};
#pragma pack()

// deadlake PROXYSUPPORT
struct ProxySettings{
	uint16 type;
	uint16 port;
	char name[50];
	char user[50];
	char password[50];
	bool EnablePassword;
	bool UseProxy;
};

// khaos::categorymod+ View Filter Struct
#pragma pack(1)
struct CategoryViewFilter_Struct{
	//		General View Filters
	uint8	nFromCats;  // 0 == All; 1 == Unassigned; 2 == This Cat Only
	bool	bSuspendFilters;
	//		File Type View Filters
	bool	bVideo;
	bool	bAudio;
	bool	bArchives;
	bool	bImages;
	//		File State View Filters
	bool	bWaiting;
	bool	bTransferring;
	bool	bPaused;
	bool	bStopped;
	bool	bComplete;
	bool	bHashing;
	bool	bErrorUnknown;
	bool	bCompleting;
	//		File Size View Filters
	uint32	nFSizeMin;
	uint32	nFSizeMax;
	uint32	nRSizeMin;
	uint32	nRSizeMax;
	//		Time Remaining Filters
	uint32	nTimeRemainingMin;
	uint32	nTimeRemainingMax;
	//		Source Count Filters
	uint16	nSourceCountMin;
	uint16	nSourceCountMax;
	uint16	nAvailSourceCountMin;
	uint16	nAvailSourceCountMax;
	//		Advanced Filter Mask
	CString	sAdvancedFilterMask;
};
#pragma pack()

// Criteria Selection Struct
#pragma pack(1)
struct CategorySelectionCriteria_Struct{
	bool	bFileSize;
	bool	bAdvancedFilterMask;
};
#pragma pack()
// khaos::categorymod-

#pragma pack(1)
struct Category_Struct{
	char	incomingpath[MAX_PATH];
	char	title[64];
	char	comment[255];
	DWORD	color;
	uint8	prio;
	// khaos::kmod+ Category Advanced A4AF Mode
	// 0 = Default, 1 = Balancing, 2 = Stacking
	uint8	iAdvA4AFMode;
	// View Filter Struct
	CategoryViewFilter_Struct viewfilters;
	CategorySelectionCriteria_Struct selectioncriteria;
	//MORPH - Removed by SiRoB, not used
	//char	autoCat[255];
	// khaos::kmod-
	//MORPH - Removed by SiRoB, Due to Khaos Categorie
	//CString autocat;
};

#pragma pack()

//EastShare Start - added by AndCycle, here is better then opcode.h, creditsystem integration
enum CreditSystemSelection {
	//becareful the sort order for the damn radio button in PPgEastShare.cpp
	CS_OFFICIAL = 0,	
	CS_LOVELACE,
//	CS_RATIO,
	CS_PAWCIO,
	CS_EASTSHARE
};
//EastShare End - added by AndCycle, here is better then opcode.h, creditsystem integration

//Morph Start - added by AndCycle, One-Queue-Per-File
enum EqualChanceForEachFileSelection{

	ECFEF_DISABLE = 0,
	ECFEF_ACCEPTED,				//accroading the file accepted
	ECFEF_ACCEPTED_COMPLETE,	//file accepted base, one complete file
	ECFEF_TRANSFERRED,			//accroading transfered
	ECFEF_TRANSFERRED_COMPLETE	//transfered base, one complete file

};
//Morph End - added by AndCycle, One-Queue-Per-File

#pragma pack(1)
struct Preferences_Struct{
//MORPH START - Added by IceCream, Defeat 0-filled Part Senders from Maella
	bool	enableZeroFilledTest;  // -Defeat 0-filled Part Senders- (Idea of xrmb)
//MORPH END   - Added by IceCream, Defeat 0-filled Part Senders from Maella
	bool	enableDownloadInRed; //MORPH - Added by IceCream, show download in red
	bool	enableChunkAvaibility; //MORPH - Added by IceCream, ChunkAvaibility
	bool	enableAntiLeecher; //MORPH - Added by IceCream, enableAntiLeecher
	bool	enableAntiCreditHack; //MORPH - Added by IceCream, enableAntiCreditHack
	bool	isZZRatioActivated;// Added By Yun.SF3, Option for Ratio Systems
	bool	isboostless;//Added by Yun.SF3, boost the less uploaded files
	CreditSystemSelection	creditSystemMode; // EastShare - Added by linekin, creditsystem integration
	EqualChanceForEachFileSelection equalChanceForEachFileMode;//Morph - added by AndCycle, Equal Chance For Each File
	bool	m_bECFEFallTime;//Morph - added by AndCycle, Equal Chance For Each File
	bool	isboostfriends;//Added by Yun.SF3, boost friends
	bool	isautodynupswitching;//MORPH - Added by Yun.SF3, Auto DynUp changing
	bool	m_bisautopowersharenewdownloadfile; //MORPH - Added by SiRoB, Avoid misusing of powersharing
	char	nick[255];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	udpport;
	uint16	nServerUDPPort;
	uint16	kadudpport;
	uint16	maxconnections;
	uint16	maxconnectionsswitchborder;
	int8	reconnect;
	int8	deadserver;
	int8	scorsystem;
	char	incomingdir[MAX_PATH];
	char	tempdir[MAX_PATH];
	int8	ICH;
	int8	autoserverlist;
	int8	updatenotify;
	int8	mintotray;
	int8	autoconnect;
	int8	autoconnectstaticonly; // Barry
	int8	autotakeed2klinks;     // Barry
	int8	addnewfilespaused;     // Barry
	int8	depth3D;			   // Barry
	int		m_iStraightWindowStyles;
	TCHAR	m_szSkinProfile[MAX_PATH];
	TCHAR	m_szSkinProfileDir[MAX_PATH];
	int8	addserversfromserver;
	int8	addserversfromclient;
	int16	maxsourceperfile;
	int16	trafficOMeterInterval;
	int16	statsInterval;
	uchar	userhash[16];
	WINDOWPLACEMENT EmuleWindowPlacement;
	int		maxGraphDownloadRate;
	int		maxGraphUploadRate;
	uint8	beepOnError;
	uint8	confirmExit;
	// khaos::categorymod+ More columns...
	int16	downloadColumnWidths[16];
	BOOL	downloadColumnHidden[16];
	INT		downloadColumnOrder[16];
	// khaos::categorymod-
	int16	uploadColumnWidths[13];
	BOOL	uploadColumnHidden[13];
	INT		uploadColumnOrder[13];
	int16	queueColumnWidths[11];
	BOOL	queueColumnHidden[11];
	INT		queueColumnOrder[11];
	int16	searchColumnWidths[12];//11+1/*Fakecheck*/
	BOOL	searchColumnHidden[12];//11+1/*Fakecheck*/
	INT		searchColumnOrder[12];//11+1/*Fakecheck*/
	//EastShare Start- Added by Pretender, TBH-AutoBackup
	bool	autobackup;
	bool	autobackup2;
	//EastShare End - Added by Pretender, TBH-AutoBackup
	// SLUGFILLER: Spreadbars
	int16	sharedColumnWidths[18];	//13+1/*PWSHARE*/+4/*Spreadbars*/
	BOOL	sharedColumnHidden[18];	//13+1/*PWSHARE*/+4/*Spreadbars*/
	INT		sharedColumnOrder[18]; //13+1/*PWSHARE*/+4/*Spreadbars*/
	// SLUGFILLER: Spreadbars
	int16	serverColumnWidths[13];
	BOOL	serverColumnHidden[13];
	INT 	serverColumnOrder[13];
	int16	clientListColumnWidths[8];
	BOOL	clientListColumnHidden[8];
	INT 	clientListColumnOrder[8];
	DWORD	statcolors[15];

	uint8	splashscreen;
	uint8	filterBadIP;
	uint8	onlineSig;

	// -khaos--+++> Struct Members for Storing Statistics
	
	// Saved stats for cumulative downline overhead...
	uint64	cumDownOverheadTotal;
	uint64	cumDownOverheadFileReq;
	uint64	cumDownOverheadSrcEx;
	uint64	cumDownOverheadServer;
	uint64	cumDownOverheadTotalPackets;
	uint64	cumDownOverheadFileReqPackets;
	uint64	cumDownOverheadSrcExPackets;
	uint64	cumDownOverheadServerPackets;

	// Saved stats for cumulative upline overhead...
	uint64	cumUpOverheadTotal;
	uint64	cumUpOverheadFileReq;
	uint64	cumUpOverheadSrcEx;
	uint64	cumUpOverheadServer;
	uint64	cumUpOverheadTotalPackets;
	uint64	cumUpOverheadFileReqPackets;
	uint64	cumUpOverheadSrcExPackets;
	uint64	cumUpOverheadServerPackets;

	// Saved stats for cumulative upline data...
	uint32	cumUpSuccessfulSessions;
	uint32	cumUpFailedSessions;
	uint32	cumUpAvgTime;
	// Cumulative client breakdown stats for sent bytes...
	uint64	cumUpData_EDONKEY;
	uint64	cumUpData_EDONKEYHYBRID;
	uint64	cumUpData_EMULE;
	uint64	cumUpData_MLDONKEY;
	uint64	cumUpData_CDONKEY;
	uint64	cumUpData_XMULE;
	uint64	cumUpData_SHAREAZA;
	// Session client breakdown stats for sent bytes...
	uint64	sesUpData_EDONKEY;
	uint64	sesUpData_EDONKEYHYBRID;
	uint64	sesUpData_EMULE;
	uint64	sesUpData_MLDONKEY;
	uint64	sesUpData_CDONKEY;
	uint64	sesUpData_XMULE;
	uint64	sesUpData_SHAREAZA;

	// Cumulative port breakdown stats for sent bytes...
	uint64	cumUpDataPort_4662;
	uint64	cumUpDataPort_OTHER;
	// Session port breakdown stats for sent bytes...
	uint64	sesUpDataPort_4662;
	uint64	sesUpDataPort_OTHER;

	// Cumulative source breakdown stats for sent bytes...
	uint64	cumUpData_File;
	uint64	cumUpData_Partfile;
	// Session source breakdown stats for sent bytes...
	uint64	sesUpData_File;
	uint64	sesUpData_Partfile;

	// Saved stats for cumulative downline data...
	uint32	cumDownCompletedFiles;
	uint16	cumDownSuccessfulSessions;
	uint16	cumDownFailedSessions;
	uint32	cumDownAvgTime;

	// Cumulative statistics for saved due to compression/lost due to corruption
	uint64	cumLostFromCorruption;
	uint64	cumSavedFromCompression;
	uint32	cumPartsSavedByICH;

	// Session stats for compression/corruption
	uint64	sesLostFromCorruption;
	uint64	sesSavedFromCompression;

	// Session statistics for download sessions
	uint16	sesDownSuccessfulSessions;
	uint16	sesDownFailedSessions;
	uint32	sesDownAvgTime;
	uint16	sesDownCompletedFiles;
	uint16	sesPartsSavedByICH;

	// Cumulative client breakdown stats for received bytes...
	uint64	cumDownData_EDONKEY;
	uint64	cumDownData_EDONKEYHYBRID;
	uint64	cumDownData_EMULE;
	uint64	cumDownData_MLDONKEY;
	uint64	cumDownData_CDONKEY;
	uint64	cumDownData_XMULE;
	uint64	cumDownData_SHAREAZA;
	// Session client breakdown stats for received bytes...
	uint64	sesDownData_EDONKEY;
	uint64	sesDownData_EDONKEYHYBRID;
	uint64	sesDownData_EMULE;
	uint64	sesDownData_MLDONKEY;
	uint64	sesDownData_CDONKEY;
	uint64	sesDownData_XMULE;
	uint64	sesDownData_SHAREAZA;

	// Cumulative port breakdown stats for received bytes...
	uint64	cumDownDataPort_4662;
	uint64	cumDownDataPort_OTHER;
	// Session port breakdown stats for received bytes...
	uint64	sesDownDataPort_4662;
	uint64	sesDownDataPort_OTHER;

	// Saved stats for cumulative connection data...
	float	cumConnAvgDownRate;
	float	cumConnMaxAvgDownRate;
	float	cumConnMaxDownRate;
	float	cumConnAvgUpRate;
	float	cumConnMaxAvgUpRate;
	float	cumConnMaxUpRate;
	uint64	cumConnRunTime;
	uint16	cumConnNumReconnects;
	uint16	cumConnAvgConnections;
	uint16	cumConnMaxConnLimitReached;
	uint16	cumConnPeakConnections;
	uint32	cumConnTransferTime;
	uint32	cumConnDownloadTime;
	uint32	cumConnUploadTime;
	uint32	cumConnServerDuration;

	// Saved records for servers / network...
	uint16	cumSrvrsMostWorkingServers;
	uint32	cumSrvrsMostUsersOnline;
	uint32	cumSrvrsMostFilesAvail;

	// Saved records for shared files...
    uint16	cumSharedMostFilesShared;
	uint64	cumSharedLargestShareSize;
	uint64	cumSharedLargestAvgFileSize;
	uint64	cumSharedLargestFileSize;

	// Save the date when the statistics were last reset...
	__int64	stat_datetimeLastReset;

	// Save new preferences for PPgStats
	uint8	statsConnectionsGraphRatio; // This will store the divisor, i.e. for 1:3 it will be 3, for 1:20 it will be 20.
	// Save the expanded branches of the stats tree
	char	statsExpandedTreeItems[256];

	// <-----khaos- End Statistics Members


	// Original Stats Stuff
	uint64  totalDownloadedBytes;
	uint64	totalUploadedBytes;
	// End Original Stats Stuff
	WORD	languageID;
	int8	transferDoubleclick;
	int8	m_iSeeShares;		// 0=everybody 1=friends only 2=noone
	int8	m_iToolDelayTime;	// tooltip delay time in seconds
	int8	bringtoforeground;
	int8	splitterbarPosition;
	uint16	deadserverretries;
	DWORD	m_dwServerKeepAliveTimeout;
	// -khaos--+++> Changed data type to avoid overflows
	uint16  statsMax;
	// <-----khaos-
	int8	statsAverageMinutes;

    int8    useDownloadNotifier;
	int8	useNewDownloadNotifier;
    int8    useChatNotifier;
    int8    useLogNotifier;	
    int8    useSoundInNotifier;
	int8    notifierPopsEveryChatMsg;
	int8	notifierImportantError;
	int8	notifierNewVersion;
    char    notifierSoundFilePath[510];

	char	m_sircserver[50];
	char	m_sircnick[30];
	char	m_sircchannamefilter[50];
	bool	m_bircaddtimestamp;
	bool	m_bircusechanfilter;
	uint16	m_iircchanneluserfilter;
	char	m_sircperformstring[255];
	bool	m_bircuseperform;
	bool	m_birclistonconnect;
	bool	m_bircacceptlinks;
	bool	m_bircacceptlinksfriends;
	bool	m_bircignoreinfomessage;
	bool	m_bircignoreemuleprotoinfomessage;
	bool	m_birchelpchannel;

	bool	m_bpreviewprio;
	bool	smartidcheck;
	uint8	smartidstate;
	bool	safeServerConnect;
	bool	startMinimized;
	uint16	MaxConperFive;
	int		checkDiskspace;	// SLUGFILLER: checkDiskspace
	UINT	m_uMinFreeDiskSpace;
	char	yourHostname[127];	// itsonlyme: hostnameSource

	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	uint8	LowIdRetries;
	uint8	LowIdRetried;
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry
	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	uint8	hideOS;
	uint8	selectiveShare;
	//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS

	//EastShare Start - PreferShareAll by AndCycle
	bool	shareall;	// SLUGFILLER: preferShareAll
	//EastShare End - PreferShareAll by AndCycle

	bool	m_bAutoClearComplete;//EastShare - added by AndCycle - AutoClearComplete (NoamSon)

	bool	m_bVerbose;
	bool	m_bDebugSourceExchange; // Sony April 23. 2003, button to keep source exchange msg out of verbose log
	bool	m_bDebugSecuredConnection; //MORPH - Added by SiRoB, Debug Log option for Secured Connection
	DWORD	m_dwDebugServerTCP;
	DWORD	m_dwDebugServerUDP;
	DWORD	m_dwDebugServerSources;
	DWORD	m_dwDebugServerSearches;
	bool	m_bupdatequeuelist;
	bool	m_bmanualhighprio;
	bool	m_btransferfullchunks;
	bool	m_bstartnextfile;
	bool	m_bshowoverhead;
	bool	m_bDAP;
	bool	m_bUAP;
	bool	m_bDisableKnownClientList;
	bool	m_bDisableQueueList;
	bool	m_bExtControls;

	int8	versioncheckdays;

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	// SLUGFILLER: multiSort - save multiple params
	// SLUGFILLER: DLsortFix - double, for client-only sorting
	int		tableSortItemDownload[26];
	BOOL	tableSortAscendingDownload[26];
	// SLUGFILLER: DLsortFix
	// itsonlyme: clientSoft +1
	// SLUGFILLER: upRemain +1
	// SLUGFILLER: BandwidthThrottler +1
	int		tableSortItemUpload[11];
	BOOL	tableSortAscendingUpload[11];
	// SLUGFILLER: BandwidthThrottler
	// SLUGFILLER: upRemain
	int		tableSortItemQueue[11];
	BOOL	tableSortAscendingQueue[11];
	// itsonlyme: clientSoft
	int		tableSortItemSearch[11];
	BOOL	tableSortAscendingSearch[11];
	// SLUGFILLER: Spreadbars (+4)
	int		tableSortItemShared[16];
	BOOL	tableSortAscendingShared[16];
	// SLUGFILLER: Spreadbars
	int		tableSortItemServer[13];
	BOOL	tableSortAscendingServer[13];
	int		tableSortItemClientList[8];
	BOOL	tableSortAscendingClientList[8];
	// SLUGFILLER: multiSort

	bool	showRatesInTitle;

	char	TxtEditor[256];
	char	VideoPlayer[256];
	char	UpdateURLFakeList[256];//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	char	UpdateURLIPFilter[256];//MORPH START added by Yun.SF3: Ipfilter.dat update
	bool	moviePreviewBackup;
	int		m_iPreviewSmallBlocks;
	bool	indicateratings;
	bool	watchclipboard;
	bool	filterserverbyip;
	bool	m_bFirstStart;
	bool	m_bCreditSystem;
	bool	m_bPayBackFirst;//EastShare - added by AndCycle, Pay Back First
	bool	m_bOnlyDownloadCompleteFiles;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	bool	m_bSaveUploadQueueWaitTime;//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	int		m_iKnownMetDays; // EastShare - Added by TAHO, .met file control

	bool	log2disk;
	bool	debug2disk;
	bool	DateFileNameLog;//Morph - added by AndCycle, Date File Name Log
	int		iMaxLogBuff;
	bool	scheduler;
	bool	dontcompressavi;
	bool	msgonlyfriends;
	bool	msgsecure;

	uint8	filterlevel;
	uint8	m_iFileBufferSize;
	uint8	m_iQueueSize;
	int		m_iCommitFiles;

	uint16	maxmsgsessions;
	uint32	versioncheckLastAutomatic;
	char messageFilter[512];
	char commentFilter[512];
	char filenameCleanups[512];
	char notifierConfiguration[510];
	char datetimeformat[64];
	char datetimeformat4log[64];
	LOGFONT m_lfHyperText;
	int		m_iExtractMetaData;

	// Web Server [kuchin]
	char		m_sWebPassword[256];
	char		m_sWebLowPassword[256];
	uint16		m_nWebPort;
	bool		m_bWebEnabled;
	bool		m_bWebUseGzip;
	int			m_nWebPageRefresh;
	bool		m_bWebLowEnabled;
	char		m_sWebResDir[MAX_PATH];

	char		m_sTemplateFile[MAX_PATH];
	ProxySettings proxy; // deadlake PROXYSUPPORT
	bool		m_bIsASCWOP;
	bool		m_bShowProxyErrors;

	bool		showCatTabInfos;
	bool		resumeSameCat;
	bool		dontRecreateGraphs;
	bool		autofilenamecleanup;
	// khaos::kmod+ Obsolete int			allcatType;
	bool		m_bUseAutocompl;
	bool		m_bShowDwlPercentage;
	uint16		m_iMaxChatHistory;

	int			m_iSearchMethod;
	bool		m_bAdvancedSpamfilter;
	bool		m_bUseSecureIdent;
	// mobilemule
	char		m_sMMPassword[256];
	bool		m_bMMEnabled;
	uint16		m_nMMPort;
	bool		networkkademlia;
	bool		networked2k;

	// toolbar
	int8		m_nToolbarLabels;
	char		m_sToolbarBitmap[256];
	char		m_sToolbarBitmapFolder[256];
	char		m_sToolbarSettings[256];

	//preview
	bool		m_bPreviewEnabled;

	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	uint32	m_FakesDatVersion;
	bool	UpdateFakeStartup;
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating
	//MORPH START added by Yun.SF3: Ipfilter.dat update
	bool		AutoUpdateIPFilter; //added by milobac: Ipfilter.dat update
	uint32		m_IPfilterVersion; //added by milobac: Ipfilter.dat update
	//MORPH END added by Yun.SF3: Ipfilter.dat update
	// khaos::categorymod+
	bool		m_bValidSrcsOnly;
	bool		m_bShowCatNames;
	bool		m_bActiveCatDefault;
	bool		m_bSelCatOnAdd;
	bool		m_bAutoSetResumeOrder;
	bool		m_bSmallFileDLPush;
	uint8		m_iStartDLInEmptyCats;
	bool		m_bRespectMaxSources;
	bool		m_bUseAutoCat;
	// khaos::categorymod-
	// khaos::kmod+
	bool		m_bShowA4AFDebugOutput;
	bool		m_bSmartA4AFSwapping;
	uint8		m_iAdvancedA4AFMode; // 0 = disabled, 1 = balance, 2 = stack
	bool		m_bUseSaveLoadSources;
	// khaos::categorymod-
	// khaos::accuratetimerem+
	uint8		m_iTimeRemainingMode; // 0 = both, 1 = real time, 2 = average
	// khaos::accuratetimerem-
	
	//MORPH START - Added by SiRoB, (SUC) & (USS)
	uint16		m_iMinUpload;
	//MORPH END   - Added by SiRoB, (SUC) & (USS)
	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	bool		m_bDynUpEnabled;
	int			m_iDynUpPingLimit; // EastShare - Added by TAHO, USS limit
	int			m_iDynUpPingTolerance;
	int			m_iDynUpGoingUpDivider;
	int			m_iDynUpGoingDownDivider;
	int			m_iDynUpNumberOfPings;
	bool		m_bIsUSSLimit; // EastShare - Added by linekin, USS limit applied?
	bool		m_bDynUpLog;
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
	//MORPH START - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	bool		m_bSUCEnabled;
	uint16		m_iSUCHigh;
    uint16		m_iSUCLow;
    uint16		m_iSUCPitch;
	uint16		m_iSUCDrift;
	bool		m_bSUCLog;
	//MORPH END - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
};
#pragma pack()

#pragma pack(1)
struct Preferences_Import19c_Struct{
	int8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	int8	reconnect;
	int8	deadserver;
	int8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	int8	ICH;
	int8	autoserverlist;
	int8	updatenotify;
	int8	mintotray;
	uchar	userhash[16];
	int8	autoconnect;
	int8	addserversfromserver;
	int8	addserversfromclient;
};
#pragma pack()

#pragma pack(1)
struct Preferences_Import20a_Struct{
	int8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	int8	reconnect;
	int8	deadserver;
	uint16	deadserverretries;
	int8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	int8	ICH;
	int8	autoserverlist;
	int8	updatenotify;
	int8	mintotray;
	uchar	userhash[16];
	int8	autoconnect;
	int8	addserversfromserver;
	int8	addserversfromclient;
	int16	maxsourceperfile;
	int16	trafficOMeterInterval;
	int32   totalDownloaded;
	int32	totalUploaded;
	int		maxGraphDownloadRate;
	int		maxGraphUploadRate;
	uint8	beepOnError;
	uint8	confirmExit;
	WINDOWPLACEMENT EmuleWindowPlacement;
	int transferColumnWidths[9];
	int serverColumnWidths[8];
	uint8	splashscreen;
	uint8	filterBadIP;
};
#pragma pack()

#pragma pack(1)
struct Preferences_Import20b_Struct{
	int8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	int8	reconnect;
	int8	deadserver;
	int8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	int8	ICH;
	int8	autoserverlist;
	int8	updatenotify;
	int8	mintotray;
	uchar	userhash[16];
	int8	autoconnect;
	int8	addserversfromserver;
	int8	addserversfromclient;
	int16	maxsourceperfile;
	int16	trafficOMeterInterval;
	int32   totalDownloaded;	// outdated
	int32	totalUploaded;		// outdated
	int		maxGraphDownloadRate;
	int		maxGraphUploadRate;
	uint8	beepOnError;
	uint8	confirmExit;
	WINDOWPLACEMENT EmuleWindowPlacement;
	int transferColumnWidths[9];
	int serverColumnWidths[8];
	uint8	splashscreen;
	uint8	filterBadIP;
	int64   totalDownloadedBytes;
	int64	totalUploadedBytes;
};
#pragma pack()

class CPreferences: public CLoggable
{
public:
	enum Table { tableDownload, tableUpload, tableQueue, tableSearch,
		tableShared, tableServer, tableClientList };

	friend class CPreferencesWnd;
	friend class CPPgGeneral;
	friend class CPPgConnection;
	friend class CPPgServer;
	friend class CPPgDirectories;
	friend class CPPgFiles;
	friend class CPPgNotify;
	friend class CPPgIRC;
	friend class Wizard;
	friend class CPPgTweaks;
	friend class CPPgDisplay;
	friend class CPPgSecurity;
	friend class CPPgScheduler;
    friend class CPPgMorph; //MORPH - Added by SiRoB, Morph Prefs
	friend class CPPgMorph2; //MORPH - Added by SiRoB, Morph Prefs
	friend class CPPgEastShare; //EastShare - Added by Pretender, ES Prefs
//MORPH START - Added by IceCream, Defeat 0-filled Part Senders from Maella	// Maella -Defeat 0-filled Part Senders- (Idea of xrmb)
	// Maella -Defeat 0-filled Part Senders- (Idea of xrmb)
	bool	GetEnableZeroFilledTest() const { return prefs->enableZeroFilledTest; }
	//void	SetEnableZeroFilledTest(bool flag) { prefs->enableZeroFilledTest = flag; }
	// Maella end
//MORPH END   - Added by IceCream, Defeat 0-filled Part Senders from Maella

	//MORPH START - Added by IceCream, high process priority
	bool	enableHighProcess;
	int		GetEnableHighProcess()					{ return enableHighProcess; }
	void	SetEnableHighProcess(bool enablehigh);
	//MORPH END   - Added by IceCream, high process priority

	bool	GetEnableAntiCreditHack()					{ return prefs->enableAntiCreditHack; }//MORPH - Added by IceCream, enable AntiCreditHack
	bool	GetChunkAvaibility()						{ return prefs->enableChunkAvaibility; }//MORPH - Added by IceCream, enable ChunkAvaibility

	bool GetEnableDownloadInRed () const { return prefs->enableDownloadInRed; } //MORPH - Added by IceCream, show download in red
	bool GetEnableAntiLeecher () const { return prefs->enableAntiLeecher; } //MORPH - Added by IceCream, enable Anti-leecher
	bool IsBoostLess() const {return prefs->isboostless;}//Added by Yun.SF3, boost the less uploaded files

	CreditSystemSelection  GetCreditSystem() const {return prefs->creditSystemMode;} // EastShare - Added by linekin, creditsystem integration
	EqualChanceForEachFileSelection	GetEqualChanceForEachFileMode() const {return prefs->equalChanceForEachFileMode;}	//Morph - added by AndCycle, Equal Chance For Each File
	bool	IsECFEFallTime()	const	{return prefs->m_bECFEFallTime;}//Morph - added by AndCycle, Equal Chance For Each File
	int  GetKnownMetDays() const {return prefs->m_iKnownMetDays;} // EastShare - Added by TAHO, .met file control
	bool IsBoostFriends() const {return prefs->isboostfriends;}//Added by Yun.SF3, boost friends
	bool IsAutoDynUpSwitching() const {return prefs->isautodynupswitching;}//MORPH - Added by Yun.SF3, Auto DynUp changing
	bool IsAutoPowershareNewDownloadFile() const {return prefs->m_bisautopowersharenewdownloadfile;} //MORPH - Added by SiRoB, Avoid misusing of powersharing
	CPreferences();
	~CPreferences();

	const CString& GetAppDir() const		{return appdir;}
	LPCTSTR	GetIncomingDir() const			{return prefs->incomingdir;}
	LPCTSTR	GetTempDir() const				{return prefs->tempdir;}
	const CString& GetConfigDir() const		{return configdir;}
	const CString& GetWebServerDir() const	{return m_strWebServerDir;}

	bool	IsTempFile(const CString& rstrDirectory, const CString& rstrName) const;
	bool	IsConfigFile(const CString& rstrDirectory, const CString& rstrName) const; // SLUGFILLER: SafeHash
	bool	IsShareableDirectory(const CString& rstrDirectory) const;
	bool	IsInstallationDirectory(const CString& rstrDir) const;

	bool	Save();
	void	SaveCats();
	//EastShare START - Pretender, TBH-AutoBackup
	bool    GetAutoBackup()	{ return prefs->autobackup;}
	bool    GetAutoBackup2()	{ return prefs->autobackup2;}
	void    SetAutoBackup(bool in) { prefs->autobackup = in;}
	void    SetAutoBackup2(bool in) { prefs->autobackup2 = in;}
	//EastShare END - Pretender, TBH-AutoBackup

	int8	Score()			{return prefs->scorsystem;}
	bool	Reconnect()		{return prefs->reconnect;}
	int8	DeadServer()	{return prefs->deadserver;}
	char*	GetUserNick()	{return prefs->nick;}
	void	SetUserNick(CString in)	{sprintf(prefs->nick,"%s",in);}

	uint16	GetPort()		{return prefs->port;}
	uint16	GetUDPPort()	{return prefs->udpport;}
	uint16	GetServerUDPPort(){return prefs->nServerUDPPort;}
	uint16	GetKadUDPPort()	{return prefs->kadudpport;}
	char*	GetUserHash()	{return userhash;}
	uint16	GetMaxUpload()	{return	prefs->maxupload;}
	bool	IsICHEnabled()	{return prefs->ICH;}
	bool	AutoServerlist(){return prefs->autoserverlist;}
	bool	UpdateNotify()	{return prefs->updatenotify;}
	bool	DoMinToTray()	{return prefs->mintotray;}
	bool	DoAutoConnect() {return prefs->autoconnect;}
	void	SetAutoConnect( bool inautoconnect)	{prefs->autoconnect = inautoconnect;}
	bool	AddServersFromServer()		{return prefs->addserversfromserver;}
	bool	AddServersFromClient()		{return prefs->addserversfromclient;}
	int8*	GetMinTrayPTR() {return &prefs->mintotray;}
	uint16	GetTrafficOMeterInterval() { return prefs->trafficOMeterInterval;}
	void	SetTrafficOMeterInterval(int16 in) { prefs->trafficOMeterInterval=in;}
	uint16	GetStatsInterval() { return prefs->statsInterval;}
	void	SetStatsInterval(int16 in) { prefs->statsInterval=in;}
	void	Add2TotalDownloaded(uint64 in) {prefs->totalDownloadedBytes+=in;}
	void	Add2TotalUploaded(uint64 in) {prefs->totalUploadedBytes+=in;}

	// -khaos--+++> Many, many, many, many methods.
	void	SaveStats(int bBackUp = 0);
	void	SetRecordStructMembers();
	void	SaveCompletedDownloadsStat();
	bool	LoadStats(int loadBackUp = 0);
	void	ResetCumulativeStatistics();

	//		Functions from base code that update original cumulative stats, now obsolete. (KHAOS)
	//void	Add2TotalDownloaded(uint64 in) {prefs->totalDownloadedBytes+=in;}
	//void	Add2TotalUploaded(uint64 in) {prefs->totalUploadedBytes+=in;}
	//		End functions from base code.

	//		Add to, increment and replace functions.  They're all named Add2 for the sake of some kind of naming
	//		convention.
	void	Add2DownCompletedFiles()			{ prefs->cumDownCompletedFiles++; }
	void	Add2ConnMaxAvgDownRate(float in)	{ prefs->cumConnMaxAvgDownRate = in; }
	void	Add2ConnMaxDownRate(float in)		{ prefs->cumConnMaxDownRate = in; }
	void	Add2ConnAvgUpRate(float in)			{ prefs->cumConnAvgUpRate = in; }
	void	Add2ConnMaxAvgUpRate(float in)		{ prefs->cumConnMaxAvgUpRate = in; }
	void	Add2ConnMaxUpRate(float in)			{ prefs->cumConnMaxUpRate = in; }
	void	Add2ConnPeakConnections(int in)		{ prefs->cumConnPeakConnections = in; }
	void	Add2UpAvgTime(int in)				{ prefs->cumUpAvgTime = in; }
	void	Add2DownSAvgTime(int in)			{ prefs->sesDownAvgTime += in; }
	void	Add2DownCAvgTime(int in)			{ prefs->cumDownAvgTime = in; }
	void	Add2ConnTransferTime(int in)		{ prefs->cumConnTransferTime += in; }
	void	Add2ConnDownloadTime(int in)		{ prefs->cumConnDownloadTime += in; }
	void	Add2ConnUploadTime(int in)			{ prefs->cumConnUploadTime += in; }
	void	Add2DownSessionCompletedFiles()		{ prefs->sesDownCompletedFiles++; }
	//MORPH - Added by Yun.SF3, ZZ Upload System
	void	Add2SessionTransferData				( uint8 uClientID , uint16 uClientPort , BOOL bFromPF, BOOL bUpDown, uint32 bytes, bool sentToFriend);
	//MORPH - Added by Yun.SF3, ZZ Upload System
	void	Add2DownSuccessfulSessions()		{ prefs->sesDownSuccessfulSessions++;
												  prefs->cumDownSuccessfulSessions++; }
	void	Add2DownFailedSessions()			{ prefs->sesDownFailedSessions++;
												  prefs->cumDownFailedSessions++; }
	void	Add2LostFromCorruption(uint64 in)	{ prefs->sesLostFromCorruption += in;}
	void	Add2SavedFromCompression(uint64 in)	{ prefs->sesSavedFromCompression += in;}
	void	Add2SessionPartsSavedByICH(int in)	{ prefs->sesPartsSavedByICH += in;}

	//		Functions that return stats stuff...
	//		Saved stats for cumulative downline overhead
	uint64	GetDownOverheadTotal()			{ return prefs->cumDownOverheadTotal;}
	uint64	GetDownOverheadFileReq()		{ return prefs->cumDownOverheadFileReq;}
	uint64	GetDownOverheadSrcEx()			{ return prefs->cumDownOverheadSrcEx;}
	uint64	GetDownOverheadServer()			{ return prefs->cumDownOverheadServer;}
	uint64	GetDownOverheadTotalPackets()	{ return prefs->cumDownOverheadTotalPackets;}
	uint64	GetDownOverheadFileReqPackets() { return prefs->cumDownOverheadFileReqPackets;}
	uint64	GetDownOverheadSrcExPackets()	{ return prefs->cumDownOverheadSrcExPackets;}
	uint64	GetDownOverheadServerPackets()	{ return prefs->cumDownOverheadServerPackets;}

	//		Saved stats for cumulative upline overhead
	uint64	GetUpOverheadTotal()			{ return prefs->cumUpOverheadTotal;}
	uint64	GetUpOverheadFileReq()			{ return prefs->cumUpOverheadFileReq;}
	uint64	GetUpOverheadSrcEx()			{ return prefs->cumUpOverheadSrcEx;}
	uint64	GetUpOverheadServer()			{ return prefs->cumUpOverheadServer;}
	uint64	GetUpOverheadTotalPackets()		{ return prefs->cumUpOverheadTotalPackets;}
	uint64	GetUpOverheadFileReqPackets()	{ return prefs->cumUpOverheadFileReqPackets;}
	uint64	GetUpOverheadSrcExPackets()		{ return prefs->cumUpOverheadSrcExPackets;}
	uint64	GetUpOverheadServerPackets()	{ return prefs->cumUpOverheadServerPackets;}

	//		Saved stats for cumulative upline data
	uint32	GetUpSuccessfulSessions()		{ return prefs->cumUpSuccessfulSessions;}
	uint32	GetUpFailedSessions()			{ return prefs->cumUpFailedSessions;}
	uint32	GetUpAvgTime()					{ return prefs->cumUpAvgTime;}

	//		Saved stats for cumulative downline data
	uint32	GetDownCompletedFiles()			{ return prefs->cumDownCompletedFiles;}
	uint16	GetDownC_SuccessfulSessions()	{ return prefs->cumDownSuccessfulSessions;}
	uint16	GetDownC_FailedSessions()		{ return prefs->cumDownFailedSessions;}
	uint32	GetDownC_AvgTime()				{ return prefs->cumDownAvgTime;}
	//		Session download stats
	uint16	GetDownSessionCompletedFiles()	{ return prefs->sesDownCompletedFiles;}
	uint16	GetDownS_SuccessfulSessions()	{ return prefs->sesDownSuccessfulSessions;}
	uint16	GetDownS_FailedSessions()		{ return prefs->sesDownFailedSessions;}
	uint32	GetDownS_AvgTime()				{ return GetDownS_SuccessfulSessions()?prefs->sesDownAvgTime/GetDownS_SuccessfulSessions():0;}

	//		Saved stats for corruption/compression
	uint64	GetCumLostFromCorruption()			{ return prefs->cumLostFromCorruption;}
	uint64	GetCumSavedFromCompression()		{ return prefs->cumSavedFromCompression;}
	uint32	GetPartsSavedByICH()			{ return prefs->cumPartsSavedByICH;}
	//		Session stats for corruption/compression
	uint32	GetSesLostFromCorruption()		{ return prefs->sesLostFromCorruption;}
	uint32	GetSesSavedFromCompression()	{ return prefs->sesSavedFromCompression;}
	uint16	GetSesPartsSavedByICH()			{ return prefs->sesPartsSavedByICH;}

	// Cumulative client breakdown stats for sent bytes
	uint64	GetUpTotalClientData()			{ return (GetCumUpData_EDONKEY() +			GetCumUpData_EDONKEYHYBRID() +
													  GetCumUpData_EMULE() +			GetCumUpData_MLDONKEY() +
													  GetCumUpData_CDONKEY() );}
	uint64	GetCumUpData_EDONKEY()			{ return (prefs->cumUpData_EDONKEY +		prefs->sesUpData_EDONKEY );}
	uint64	GetCumUpData_EDONKEYHYBRID()	{ return (prefs->cumUpData_EDONKEYHYBRID +	prefs->sesUpData_EDONKEYHYBRID );}
	uint64	GetCumUpData_EMULE()			{ return (prefs->cumUpData_EMULE +			prefs->sesUpData_EMULE );}
	uint64	GetCumUpData_MLDONKEY()			{ return (prefs->cumUpData_MLDONKEY +		prefs->sesUpData_MLDONKEY );}
	uint64	GetCumUpData_CDONKEY()			{ return (prefs->cumUpData_CDONKEY +		prefs->sesUpData_CDONKEY );}
	uint64	GetCumUpData_XMULE()			{ return (prefs->cumUpData_XMULE +			prefs->sesUpData_XMULE );}
	uint64	GetCumUpData_SHAREAZA()			{ return (prefs->cumUpData_SHAREAZA +			prefs->sesUpData_SHAREAZA );}
	// Session client breakdown stats for sent bytes
	uint64	GetUpSessionClientData()		{ return (prefs->sesUpData_EDONKEY +		prefs->sesUpData_EDONKEYHYBRID +
													  prefs->sesUpData_EMULE +			prefs->sesUpData_MLDONKEY +
													  prefs->sesUpData_CDONKEY ); }
	uint64	GetUpData_EDONKEY()				{ return prefs->sesUpData_EDONKEY;}
	uint64	GetUpData_EDONKEYHYBRID()		{ return prefs->sesUpData_EDONKEYHYBRID;}
	uint64	GetUpData_EMULE()				{ return prefs->sesUpData_EMULE;}
	uint64	GetUpData_MLDONKEY()			{ return prefs->sesUpData_MLDONKEY;}
	uint64	GetUpData_CDONKEY()				{ return prefs->sesUpData_CDONKEY;}
	uint64	GetUpData_XMULE()				{ return prefs->sesUpData_XMULE;}
	uint64	GetUpData_SHAREAZA()				{ return prefs->sesUpData_SHAREAZA;}
	
	// Cumulative port breakdown stats for sent bytes...
	uint64	GetUpTotalPortData()			{ return (GetCumUpDataPort_4662() +			GetCumUpDataPort_OTHER() );}
	uint64	GetCumUpDataPort_4662()			{ return (prefs->cumUpDataPort_4662 +		prefs->sesUpDataPort_4662 );}
	uint64	GetCumUpDataPort_OTHER()		{ return (prefs->cumUpDataPort_OTHER +		prefs->sesUpDataPort_OTHER );}
	// Session port breakdown stats for sent bytes...
	uint64	GetUpSessionPortData()			{ return (prefs->sesUpDataPort_4662 +		prefs->sesUpDataPort_OTHER );}
	uint64	GetUpDataPort_4662()			{ return prefs->sesUpDataPort_4662;}
	uint64	GetUpDataPort_OTHER()			{ return prefs->sesUpDataPort_OTHER;}

	// Cumulative DS breakdown stats for sent bytes...
	uint64	GetUpTotalDataFile()			{ return (GetCumUpData_File() +				GetCumUpData_Partfile() );}
	uint64	GetCumUpData_File()				{ return (prefs->cumUpData_File +			prefs->sesUpData_File );}
	uint64	GetCumUpData_Partfile()			{ return (prefs->sesUpData_Partfile +		prefs->sesUpData_Partfile );}
	// Session DS breakdown stats for sent bytes...
	uint64	GetUpSessionDataFile()			{ return (prefs->sesUpData_File +			prefs->sesUpData_Partfile );}
	uint64	GetUpData_File()				{ return prefs->sesUpData_File;}
	uint64	GetUpData_Partfile()			{ return prefs->sesUpData_Partfile;}

	// Cumulative client breakdown stats for received bytes
	uint64	GetDownTotalClientData()		{ return (GetCumDownData_EDONKEY() +			GetCumDownData_EDONKEYHYBRID() +
													  GetCumDownData_EMULE() +				GetCumDownData_MLDONKEY() +
													  GetCumDownData_CDONKEY() ); }
	uint64	GetCumDownData_EDONKEY()		{ return (prefs->cumDownData_EDONKEY +			prefs->sesDownData_EDONKEY);}
	uint64	GetCumDownData_EDONKEYHYBRID()	{ return (prefs->cumDownData_EDONKEYHYBRID +	prefs->sesDownData_EDONKEYHYBRID);}
	uint64	GetCumDownData_EMULE()			{ return (prefs->cumDownData_EMULE +			prefs->sesDownData_EMULE);}
	uint64	GetCumDownData_MLDONKEY()		{ return (prefs->cumDownData_MLDONKEY +			prefs->sesDownData_MLDONKEY);}
	uint64	GetCumDownData_CDONKEY()		{ return (prefs->cumDownData_CDONKEY +			prefs->sesDownData_CDONKEY);}
	uint64	GetCumDownData_XMULE()			{ return (prefs->cumDownData_XMULE +			prefs->sesDownData_XMULE );}
	uint64	GetCumDownData_SHAREAZA()		{ return (prefs->cumDownData_SHAREAZA +			prefs->sesDownData_SHAREAZA );}
	// Session client breakdown stats for received bytes
	uint64	GetDownSessionClientData()		{ return (prefs->sesDownData_EDONKEY +			prefs->sesDownData_EDONKEYHYBRID +
													  prefs->sesDownData_EMULE +			prefs->sesDownData_MLDONKEY +
													  prefs->sesDownData_CDONKEY ); }
	uint64	GetDownData_EDONKEY()			{ return prefs->sesDownData_EDONKEY;}
	uint64	GetDownData_EDONKEYHYBRID()		{ return prefs->sesDownData_EDONKEYHYBRID;}
	uint64	GetDownData_EMULE()				{ return prefs->sesDownData_EMULE;}
	uint64	GetDownData_MLDONKEY()			{ return prefs->sesDownData_MLDONKEY;}
	uint64	GetDownData_CDONKEY()			{ return prefs->sesDownData_CDONKEY;}
	uint64	GetDownData_XMULE()				{ return prefs->sesDownData_XMULE;}
	uint64	GetDownData_SHAREAZA()			{ return prefs->sesDownData_SHAREAZA;}

	// Cumulative port breakdown stats for received bytes...
	uint64	GetDownTotalPortData()			{ return (GetCumDownDataPort_4662() +			GetCumDownDataPort_OTHER() );}
	uint64	GetCumDownDataPort_4662()		{ return (prefs->cumDownDataPort_4662 +			prefs->sesDownDataPort_4662 );}
	uint64	GetCumDownDataPort_OTHER()		{ return (prefs->cumDownDataPort_OTHER +		prefs->sesDownDataPort_OTHER );}
	// Session port breakdown stats for received bytes...
	uint64	GetDownSessionDataPort()		{ return (prefs->sesDownDataPort_4662 +			prefs->sesDownDataPort_OTHER );}
	uint64	GetDownDataPort_4662()			{ return prefs->sesDownDataPort_4662;}
	uint64	GetDownDataPort_OTHER()			{ return prefs->sesDownDataPort_OTHER;}

	//		Saved stats for cumulative connection data
	float	GetConnAvgDownRate()			{ return prefs->cumConnAvgDownRate;}
	float	GetConnMaxAvgDownRate()			{ return prefs->cumConnMaxAvgDownRate;}
	float	GetConnMaxDownRate()			{ return prefs->cumConnMaxDownRate;}
	float	GetConnAvgUpRate()				{ return prefs->cumConnAvgUpRate;}
	float	GetConnMaxAvgUpRate()			{ return prefs->cumConnMaxAvgUpRate;}
	float	GetConnMaxUpRate()				{ return prefs->cumConnMaxUpRate;}
	uint64	GetConnRunTime()				{ return prefs->cumConnRunTime;}
	uint16	GetConnNumReconnects()			{ return prefs->cumConnNumReconnects;}
	uint16	GetConnAvgConnections()			{ return prefs->cumConnAvgConnections;}
	uint16	GetConnMaxConnLimitReached()	{ return prefs->cumConnMaxConnLimitReached;}
	uint16	GetConnPeakConnections()		{ return prefs->cumConnPeakConnections;}
	uint32	GetConnTransferTime()			{ return prefs->cumConnTransferTime;}
	uint32	GetConnDownloadTime()			{ return prefs->cumConnDownloadTime;}
	uint32	GetConnUploadTime()				{ return prefs->cumConnUploadTime;}
	uint32	GetConnServerDuration()			{ return prefs->cumConnServerDuration;}

	//		Saved records for servers / network
	uint16	GetSrvrsMostWorkingServers()	{ return prefs->cumSrvrsMostWorkingServers;}
	uint32	GetSrvrsMostUsersOnline()		{ return prefs->cumSrvrsMostUsersOnline;}
	uint32	GetSrvrsMostFilesAvail()		{ return prefs->cumSrvrsMostFilesAvail;}

	//		Saved records for shared files
	uint16	GetSharedMostFilesShared()		{ return prefs->cumSharedMostFilesShared;}
	uint64	GetSharedLargestShareSize()		{ return prefs->cumSharedLargestShareSize;}
	uint64	GetSharedLargestAvgFileSize()	{ return prefs->cumSharedLargestAvgFileSize;}
	uint64	GetSharedLargestFileSize()		{ return prefs->cumSharedLargestFileSize;}

	//		Get the long date/time when the stats were last reset
	__int64	GetStatsLastResetLng()			{ return prefs->stat_datetimeLastReset;}
	CString	GetStatsLastResetStr(bool formatLong = true);

	//		Get and Set our new preferences
	void	SetStatsMax(uint16 in)						{ prefs->statsMax = in; }
	void	SetStatsConnectionsGraphRatio(uint8 in)		{ prefs->statsConnectionsGraphRatio = in; }
	uint8	GetStatsConnectionsGraphRatio()				{ return prefs->statsConnectionsGraphRatio; }
	void	SetExpandedTreeItems(CString in)			{ sprintf(prefs->statsExpandedTreeItems,"%s",in); }
	CString GetExpandedTreeItems()						{ return (CString)prefs->statsExpandedTreeItems; }
	// <-----khaos- End Statistics Methods

	//		Original Statistics Functions
	uint64  GetTotalDownloaded()			{return prefs->totalDownloadedBytes;}
	uint64	GetTotalUploaded()				{return prefs->totalUploadedBytes;}
	//		End Original Statistics Functions
	bool	IsErrorBeepEnabled()		{return prefs->beepOnError;}
	bool	IsConfirmExitEnabled()		{return prefs->confirmExit;}
	bool	UseSplashScreen()			{return prefs->splashscreen;}
	bool	FilterBadIPs()				{return prefs->filterBadIP;}
	bool	IsOnlineSignatureEnabled()  {return prefs->onlineSig;}
	int		GetMaxGraphUploadRate()		{return prefs->maxGraphUploadRate;}
	int		GetMaxGraphDownloadRate()		{return prefs->maxGraphDownloadRate;}
	void	SetMaxGraphUploadRate(int in)	{prefs->maxGraphUploadRate  =(in)?in:16;}
	void	SetMaxGraphDownloadRate(int in)	{prefs->maxGraphDownloadRate=(in)?in:96;}
	uint16	MaxConnectionsSwitchBorder() const {return prefs->maxconnectionsswitchborder;}//MORPH - Added by Yun.SF3, Auto DynUp changing
	//MORPH START - Added by SiRoB, (SUC) & (USS)
	uint16	GetMinUpload()				{return prefs->m_iMinUpload;}
	//MORPH END   - Added by SiRoB, (SUC) & (USS)
	//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	bool	IsSUCDoesWork()				{return prefs->m_iMinUpload<prefs->maxupload && prefs->maxupload != UNLIMITED && prefs->m_bSUCEnabled;}
	bool	IsSUCEnabled()				{return prefs->m_bSUCEnabled;}
	//MORPH START - Added by Yun.SF3, Auto DynUp changing
	void	SetSUCEnabled(bool newValue){prefs->m_bSUCEnabled = newValue;}
	//MORPH END - Added by Yun.SF3, Auto DynUp changing
	bool	IsSUCLog()					{return prefs->m_bSUCLog;}
	uint16	GetSUCHigh()				{return prefs->m_iSUCHigh;}
	uint16	GetSUCLow()					{return prefs->m_iSUCLow;}
	uint16	GetSUCDrift()				{return prefs->m_iSUCDrift;}
	uint16	GetSUCPitch()				{return prefs->m_iSUCPitch;}
	//MORPH END - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	//MORPH START - Added by SiRoB, ZZ Ratio
	bool	IsZZRatioDoesWork();
	//MORPH END - Added by SiRoB, ZZ Ratio
	uint16	GetMaxDownload();
	uint16	GetMaxConnections()			{return	prefs->maxconnections;}
	uint16	GetMaxSourcePerFile()		{return prefs->maxsourceperfile;}
	uint16	GetMaxSourcePerFileSoft()	{	// Elandal: typesafe, integer math only
											uint16 temp = (uint16)((prefs->maxsourceperfile * 9L) / 10);
											if( temp > 500 )
												return 500;
                                            return temp;
										}
	uint16	GetMaxSourcePerFileUDP()	{	// Elandal: typesafe, integer math only
											uint16 temp = (uint16)((prefs->maxsourceperfile * 3L) / 4);
											if( temp > 50 )
												return 50;
                                            return temp;
										}
	uint16	GetDeadserverRetries()		{return prefs->deadserverretries;}
	DWORD	GetServerKeepAliveTimeout()	{return prefs->m_dwServerKeepAliveTimeout;}

	int     GetColumnWidth (Table t, int index) const;
	BOOL    GetColumnHidden(Table t, int index) const;
	int     GetColumnOrder (Table t, int index) const;
	void	SetColumnWidth (Table t, int index, int width);
	void	SetColumnHidden(Table t, int index, BOOL bHidden);
	void	SetColumnOrder (Table t, INT *piOrder);

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	// SLUGFILLER: multiSort
	int		GetColumnSortItem (Table t, int column = 0) const;
	bool	GetColumnSortAscending (Table t, int column = 0) const;
	int		GetColumnSortCount(Table t) const;
	// SLUGFILLER: multiSort
	void	SetColumnSortItem (Table t, int sortItem);
	void	SetColumnSortAscending (Table t, bool sortAscending);

	WORD	GetLanguageID() const;
	void	SetLanguageID(WORD lid);
	void	GetLanguages(CWordArray& aLanguageIDs) const;
	void	SetLanguage();
	const CString& GetLangDir() const {return m_strLangDir;}

	int8	IsDoubleClickEnabled()		{return prefs->transferDoubleclick;}
	int8	CanSeeShares(void)			{return prefs->m_iSeeShares;}
	int8	ServePreview(void)			{return prefs->m_iSeeShares;}
	int8	GetToolTipDelay(void)		{return prefs->m_iToolDelayTime;}
	int8	IsBringToFront()			{return prefs->bringtoforeground;}

	int8    GetSplitterbarPosition()	{return prefs->splitterbarPosition;}
	void	SetSplitterbarPosition(int8 pos) {prefs->splitterbarPosition=pos;}
	// -khaos--+++> Changed datatype to avoid overflows
	uint16	GetStatsMax()				{return prefs->statsMax;}
	// <-----khaos-
	int8	UseFlatBar()				{return (prefs->depth3D==0);}
	int		GetStraightWindowStyles()	{return prefs->m_iStraightWindowStyles;}
	LPCTSTR GetSkinProfile()			{return prefs->m_szSkinProfile;}
	CString GetSkinProfileDir()			{return prefs->m_szSkinProfileDir;}
	void	SetSkinProfile(LPCTSTR pszProfile) { _sntprintf(prefs->m_szSkinProfile, ARRSIZE(prefs->m_szSkinProfile), _T("%s"), pszProfile); }
	void	SetSkinProfileDir(LPCTSTR pszDir) { _sntprintf(prefs->m_szSkinProfileDir, ARRSIZE(prefs->m_szSkinProfileDir), _T("%s"), pszDir); }
	int8	GetStatsAverageMinutes()	{return prefs->statsAverageMinutes;}
	void	SetStatsAverageMinutes(int8 in)	{prefs->statsAverageMinutes=in;}

    bool    GetUseDownloadNotifier() {return prefs->useDownloadNotifier;}
	bool	GetUseNewDownloadNotifier(){return prefs->useNewDownloadNotifier;}
    bool    GetUseChatNotifier()	 {return prefs->useChatNotifier;}
    bool    GetUseLogNotifier()		 {return prefs->useLogNotifier;}
    bool    GetUseSoundInNotifier()  {return prefs->useSoundInNotifier;}
	bool    GetNotifierPopsEveryChatMsg() {return prefs->notifierPopsEveryChatMsg;}
	bool	GetNotifierPopOnImportantError()	{return prefs->notifierImportantError;}
	bool	GetNotifierPopOnNewVersion()	{return prefs->notifierNewVersion;}
	char*   GetNotifierWavSoundPath() {return prefs->notifierSoundFilePath;}

	CString	GetIRCNick()						{return (CString)prefs->m_sircnick;}
	void	SetIRCNick( char in_nick[] )		{strcpy(prefs->m_sircnick,in_nick);}
	CString	GetIRCServer()						{return (CString)prefs->m_sircserver;}
	bool	GetIRCAddTimestamp()				{return prefs->m_bircaddtimestamp;}
	CString	GetIRCChanNameFilter()				{return (CString)prefs->m_sircchannamefilter;}
	bool	GetIRCUseChanFilter()				{return prefs->m_bircusechanfilter;}
	uint16	GetIRCChannelUserFilter()			{return	prefs->m_iircchanneluserfilter;}
	CString	GetIrcPerformString()				{return (CString)prefs->m_sircperformstring;}
	bool	GetIrcUsePerform()					{return prefs->m_bircuseperform;}
	bool	GetIRCListOnConnect()				{return prefs->m_birclistonconnect;}
	bool	GetIrcAcceptLinks()					{return prefs->m_bircacceptlinks;}
	bool	GetIrcAcceptLinksFriends()			{return prefs->m_bircacceptlinksfriends;}
	bool	GetIrcIgnoreInfoMessage()			{return prefs->m_bircignoreinfomessage;}
	bool	GetIrcIgnoreEmuleProtoInfoMessage()	{return prefs->m_bircignoreemuleprotoinfomessage;}
	bool	GetIrcHelpChannel()					{return prefs->m_birchelpchannel;}
	WORD	GetWindowsVersion();
	bool	GetStartMinimized()					{return prefs->startMinimized;}
	void	SetStartMinimized( bool instartMinimized){prefs->startMinimized = instartMinimized;}
	bool	GetSmartIdCheck()					{return prefs->smartidcheck;}
	void	SetSmartIdCheck( bool in_smartidcheck){prefs->smartidcheck = in_smartidcheck;}
	uint8	GetSmartIdState()					{return prefs->smartidstate;}
	void	SetSmartIdState( uint8 in_smartidstate){prefs->smartidstate = in_smartidstate;}
	bool	GetVerbose()						{return prefs->m_bVerbose;}
	bool	GetDebugSourceExchange()			{return prefs->m_bDebugSourceExchange;}
	DWORD	GetDebugServerTCP()					{return prefs->m_dwDebugServerTCP;}
	DWORD	GetDebugServerUDP()					{return prefs->m_dwDebugServerUDP;}
	DWORD	GetDebugServerSources()				{return prefs->m_dwDebugServerSources;}
	DWORD	GetDebugServerSearches()			{return prefs->m_dwDebugServerSearches;}
	bool	GetDebugSecuredConnection()			{return prefs->m_bDebugSecuredConnection;}  //MORPH - Added by SiRoB, Debug Log option for Secured Connection
	bool	GetPreviewPrio()					{return prefs->m_bpreviewprio;}
	void	SetPreviewPrio(bool in)				{prefs->m_bpreviewprio=in;}
	bool	GetUpdateQueueList()				{return prefs->m_bupdatequeuelist;}
	bool	GetManualHighPrio()					{return prefs->m_bmanualhighprio;}
	bool	TransferFullChunks()				{return prefs->m_btransferfullchunks;}
	void	SetTransferFullChunks( bool m_bintransferfullchunks )				{prefs->m_btransferfullchunks = m_bintransferfullchunks;}
	bool	StartNextFile()						{return prefs->m_bstartnextfile;}
	bool	ShowOverhead()						{return prefs->m_bshowoverhead;}
	void	SetNewAutoUp(bool m_bInUAP)			{prefs->m_bUAP = m_bInUAP;}
	bool	GetNewAutoUp()						{return prefs->m_bUAP;}
	void	SetNewAutoDown(bool m_bInDAP)		{prefs->m_bDAP = m_bInDAP;}
	bool	GetNewAutoDown()					{return prefs->m_bDAP;}
	bool	IsKnownClientListDisabled()			{return prefs->m_bDisableKnownClientList;}
	bool	IsQueueListDisabled()				{return prefs->m_bDisableQueueList;}
	bool	IsFirstStart()						{return prefs->m_bFirstStart;}
	bool	UseCreditSystem()					{return true;} // EastShare - Fixed by linekin
	void	SetCreditSystem(bool m_bInCreditSystem)	{prefs->m_bCreditSystem = m_bInCreditSystem;}	//EastShare - Credit System select
	void	SetKnownMetDays(int m_iInKnownMetDays)	{prefs->m_iKnownMetDays = m_iInKnownMetDays;}	//EastShare - Added by TAHO, .met file control
	bool	IsPayBackFirst()					{return prefs->m_bPayBackFirst;}	//EastShare - added by AndCycle, Pay Back First
	bool	IsAutoClearComplete()				{return prefs->m_bAutoClearComplete; }	//EastShare - added by AndCycle - AutoClearComplete (NoamSon)
	bool	OnlyDownloadCompleteFiles()			{return prefs->m_bOnlyDownloadCompleteFiles;} //EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	bool	SaveUploadQueueWaitTime()			{return prefs->m_bSaveUploadQueueWaitTime;}//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)

	char*	GetTxtEditor()						{return prefs->TxtEditor;}
	CString	GetVideoPlayer()					{if (strlen(prefs->VideoPlayer)==0) return ""; else return CString(prefs->VideoPlayer);}
	CString	GetUpdateURLFakeList()				{return CString(prefs->UpdateURLFakeList);}		//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	CString	GetUpdateURLIPFilter()				{return CString(prefs->UpdateURLIPFilter);}//MORPH START added by Yun.SF3: Ipfilter.dat update


	uint32	GetFileBufferSize()					{return prefs->m_iFileBufferSize*15000;}
	uint32	GetQueueSize()						{return prefs->m_iQueueSize*100;}
	int		GetCommitFiles()					{return prefs->m_iCommitFiles;}
	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	uint32	GetFakesDatVersion()				{return prefs->m_FakesDatVersion;}
	void	SetFakesDatVersion(uint32 version)	{prefs->m_FakesDatVersion = version;} 
	bool	IsUpdateFakeStartupEnabled()		{ return prefs->UpdateFakeStartup; }
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating
	//MORPH START added by Yun.SF3: Ipfilter.dat update
	uint32	GetIPfilterVersion()				{return prefs->m_IPfilterVersion;}
	void	SetIpfilterVersion(uint32 version)	{prefs->m_IPfilterVersion = version;}
	//MORPH END added by Yun.SF3: Ipfilter.dat update
	// Barry
	uint16	Get3DDepth() { return prefs->depth3D;}
	bool	AutoTakeED2KLinks() {return prefs->autotakeed2klinks;}
	bool	AddNewFilesPaused() {return prefs->addnewfilespaused;}

	void	SetStatsColor(int index,DWORD value) {prefs->statcolors[index]=value;}
	DWORD	GetStatsColor(int index) {return prefs->statcolors[index];}
	void	SetMaxConsPerFive(int in) {prefs->MaxConperFive=in;}
	LPLOGFONT GetHyperTextLogFont() { return &prefs->m_lfHyperText; }
	void	SetHyperTextFont(LPLOGFONT plf) { prefs->m_lfHyperText = *plf; }

	uint16	GetMaxConperFive()					{return prefs->MaxConperFive;}
	uint16	GetDefaultMaxConperFive();

	void	ResetStatsColor(int index);
	bool	IsSafeServerConnectEnabled()		{return prefs->safeServerConnect;}
	void	SetSafeServerConnectEnabled(bool in){prefs->safeServerConnect=in;}
	bool	IsMoviePreviewBackup()				{return prefs->moviePreviewBackup;}
	int		GetPreviewSmallBlocks()				{return prefs->m_iPreviewSmallBlocks;}
	int		GetExtractMetaData() const			{return prefs->m_iExtractMetaData;}

	// itsonlyme: hostnameSource
	char*	GetYourHostname()	{return prefs->yourHostname;}
	void	SetYourHostname(CString in)	{sprintf(prefs->yourHostname,"%s",in);}
	// itsonlyme: hostnameSource
	bool	IsCheckDiskspaceEnabled()		{return prefs->checkDiskspace != 0;}	// SLUGFILLER: checkDiskspace
	UINT	GetMinFreeDiskSpace()			{return prefs->m_uMinFreeDiskSpace;}

	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	void	SetLowIdRetries(uint8 in)	{prefs->LowIdRetries=in;}
	void	SetLowIdRetried()	{prefs->LowIdRetried--;}
	void	ResetLowIdRetried()	{prefs->LowIdRetried = prefs->LowIdRetries;}
	uint8	GetLowIdRetried()	{return prefs->LowIdRetried;}
	uint8	GetLowIdRetries()	{return prefs->LowIdRetries;}
	//MORPH END - Added by SiRoB, SLUGFILLER: lowIdRetry
	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	uint8	GetHideOvershares()		{return prefs->hideOS;}
	uint8	IsSelectiveShareEnabled()	{return prefs->selectiveShare;}
	//MORPH END - Added by SiRoB, SLUGFILLER: hideOS

	//EastShare Start - PreferShareAll by AndCycle
	bool	ShareAll()			{return prefs->shareall;}	// SLUGFILLER: preferShareAll
	//EastShare End - PreferShareAll by AndCycle

	void	SetMinUpload(uint16 in) {  prefs->m_iMinUpload = in; } //MORPH - Added by SiRoB, (SUC) & (USS)
	void	SetMaxUpload(uint16 in) {  prefs->maxupload = (in) ? in : 0xffff; }
	void	SetMaxDownload(uint16 in) {prefs->maxdownload=(in) ? in : 0xffff; }

	WINDOWPLACEMENT GetEmuleWindowPlacement() {return prefs->EmuleWindowPlacement; }
	void SetWindowLayout(WINDOWPLACEMENT in) {prefs->EmuleWindowPlacement=in; }

	CStringList shareddir_list;
	CStringList adresses_list;

	int8 AutoConnectStaticOnly()	{return prefs->autoconnectstaticonly;}	
	int8 GetUpdateDays()			{return prefs->versioncheckdays;}
	uint32 GetLastVC()				{return prefs->versioncheckLastAutomatic;}
	void   UpdateLastVC()			{prefs->versioncheckLastAutomatic=mktime(CTime::GetCurrentTime().GetLocalTm());}
	int	GetIPFilterLevel()			{ return prefs->filterlevel;}
	CString GetMessageFilter()		{ return CString(prefs->messageFilter);}
	CString GetCommentFilter()		{ return CString(prefs->commentFilter);}
	CString GetFilenameCleanups()	{ return CString(prefs->filenameCleanups);}

	bool	ShowRatesOnTitle()		{ return prefs->showRatesInTitle;}
    char*   GetNotifierConfiguration()    {return prefs->notifierConfiguration;}; //<<-- enkeyDEV(kei-kun) -skinnable notifier-
    void    SetNotifierConfiguration(CString configFullPath) {sprintf(prefs->notifierConfiguration,configFullPath); } //<<-- enkeyDEV(kei-kun) -skinnable notifier-
	void	LoadCats();
	CString	GetDateTimeFormat()		{ return CString(prefs->datetimeformat);}
	CString GetDateTimeFormat4Log()	{ return CString(prefs->datetimeformat4log);}
	// Download Categories (Ornis)
	int		AddCat(Category_Struct* cat) { catMap.Add(cat); return catMap.GetCount()-1;}
	bool	MoveCat(UINT from, UINT to);
	void	RemoveCat(int index);
	int		GetCatCount()			{ return catMap.GetCount();}
	Category_Struct* GetCategory(int index) { if (index>=0 && index<catMap.GetCount()) return catMap.GetAt(index); else return NULL;}
	char*	GetCatPath(uint8 index)	{ return catMap.GetAt(index)->incomingpath;}
	DWORD	GetCatColor(uint8 index)	{ if (index>=0 && index<catMap.GetCount()) return catMap.GetAt(index)->color; else return 0;}

	bool	ShowRatingIndicator()	{ return prefs->indicateratings;}
	// khaos::kmod+ Obsolete int		GetAllcatType()			{ return prefs->allcatType;}
	// khaos::kmod+ Obsolete void	SetAllcatType(int in)   { prefs->allcatType=in; }

	bool	WatchClipboard4ED2KLinks()	{ return prefs->watchclipboard;}
	bool	FilterServerByIP()		{ return prefs->filterserverbyip;}

	bool	Log2Disk()	{ return prefs->log2disk;}
	bool	Debug2Disk()	{ return prefs->debug2disk;}
	bool	DateFileNameLog()	{ return prefs->DateFileNameLog;}//Morph - added by AndCycle, Date File Name Log
	int		GetMaxLogBuff() { return prefs->iMaxLogBuff;}

	// WebServer
	uint16	GetWSPort()								{ return prefs->m_nWebPort; }
	void	SetWSPort(uint16 uPort)					{ prefs->m_nWebPort=uPort; }
	CString	GetWSPass()								{ return CString(prefs->m_sWebPassword); }
	void	SetWSPass(CString strNewPass)			{ sprintf(prefs->m_sWebPassword,"%s",MD5Sum(strNewPass).GetHash().GetBuffer(0)); }
	bool	GetWSIsEnabled()						{ return prefs->m_bWebEnabled; }
	void	SetWSIsEnabled(bool bEnable)			{ prefs->m_bWebEnabled=bEnable; }
	bool	GetWebUseGzip()							{ return prefs->m_bWebUseGzip; }
	void	SetWebUseGzip(bool bUse)				{ prefs->m_bWebUseGzip=bUse; }
	int		GetWebPageRefresh()						{ return prefs->m_nWebPageRefresh; }
	void	SetWebPageRefresh(int nRefresh)			{ prefs->m_nWebPageRefresh=nRefresh; }
	bool	GetWSIsLowUserEnabled()					{ return prefs->m_bWebLowEnabled; }
	void	SetWSIsLowUserEnabled(bool in)			{ prefs->m_bWebLowEnabled=in; }
	CString	GetWSLowPass()							{ return CString(prefs->m_sWebLowPassword); }
	void	SetWSLowPass(CString strNewPass)		{ sprintf(prefs->m_sWebLowPassword,"%s",MD5Sum(strNewPass).GetHash().GetBuffer(0)); }

	void	SetMaxSourcesPerFile(uint16 in)			{ prefs->maxsourceperfile=in;}
	void	SetMaxConnections(uint16 in)			{ prefs->maxconnections =in;}
	bool	IsSchedulerEnabled()					{ return prefs->scheduler;}
	void	SetSchedulerEnabled(bool in)			{ prefs->scheduler=in;}
	bool	GetDontCompressAvi()					{ return prefs->dontcompressavi;}
	
	bool	MsgOnlyFriends()						{ return prefs->msgonlyfriends;}
	bool	MsgOnlySecure()							{ return prefs->msgsecure;}
	uint16	GetMsgSessionsMax()						{ return prefs->maxmsgsessions;}
	bool	IsSecureIdentEnabled()					{ return prefs->m_bUseSecureIdent;} // use clientcredits->CryptoAvailable() to check if crypting is really available and not this function
	bool	IsAdvSpamfilterEnabled()				{ return prefs->m_bAdvancedSpamfilter;}
	CString	GetTemplate()							{ return CString(prefs->m_sTemplateFile);}
	void	SetTemplate(CString in)					{ sprintf(prefs->m_sTemplateFile,"%s",in);}
	bool	GetNetworkKademlia()					{ return prefs->networkkademlia;}
	void	SetNetworkKademlia(bool val)			{ prefs->networkkademlia = val;}
	bool	GetNetworkED2K()						{ return prefs->networked2k;}
	void	SetNetworkED2K(bool val)				{ prefs->networked2k = val;}

	// mobileMule
	CString	GetMMPass()								{ return CString(prefs->m_sMMPassword); }
	void	SetMMPass(CString strNewPass)			{ sprintf(prefs->m_sMMPassword,"%s",MD5Sum(strNewPass).GetHash().GetBuffer(0)); }
	bool	IsMMServerEnabled()						{ return prefs->m_bMMEnabled; }
	void	SetMMIsEnabled(bool bEnable)			{ prefs->m_bMMEnabled=bEnable; }
	uint16	GetMMPort()								{ return prefs->m_nMMPort; }
	void	SetMMPort(uint16 uPort)					{ prefs->m_nMMPort=uPort; }

	// deadlake PROXYSUPPORT
	const ProxySettings& GetProxy()								{return prefs->proxy;}
	void SetProxySettings(const ProxySettings& proxysettings)	{prefs->proxy = proxysettings;}
	uint16	GetListenPort()				{if (m_UseProxyListenPort) return ListenPort; else return prefs->port;}
	void	SetListenPort(uint16 uPort)	{ListenPort = uPort; m_UseProxyListenPort = true;}
	void	ResetListenPort()			{ListenPort = 0; m_UseProxyListenPort = false;}
	void	SetUseProxy(bool in)		{ prefs->proxy.UseProxy=in;}
	bool	GetShowProxyErrors()		{ return prefs->m_bShowProxyErrors; }
	void	SetShowProxyErrors(bool bEnable){ prefs->m_bShowProxyErrors = bEnable; }

	bool	IsProxyASCWOP()				{ return prefs->m_bIsASCWOP;}
	void	SetProxyASCWOP(bool in)		{ prefs->m_bIsASCWOP=in;}

	bool	ShowCatTabInfos()			{ return prefs->showCatTabInfos;}
	void	ShowCatTabInfos(bool in)	{ prefs->showCatTabInfos=in;}

	bool	AutoFilenameCleanup()			{ return prefs->autofilenamecleanup;}
	void	AutoFilenameCleanup(bool in)	{ prefs->autofilenamecleanup=in;}
	void	SetFilenameCleanups(CString in) { sprintf(prefs->filenameCleanups,"%s",in);}

	bool	GetResumeSameCat()			{ return prefs->resumeSameCat;}
	bool	IsGraphRecreateDisabled()	{ return prefs->dontRecreateGraphs;}
bool	IsExtControlsEnabled()		{ return prefs->m_bExtControls;}
	void	SetExtControls(bool in)		{ prefs->m_bExtControls=in;}

	uint16	GetMaxChatHistoryLines()	{ return prefs->m_iMaxChatHistory;}
	bool	GetUseAutocompletion()		{ return prefs->m_bUseAutocompl;}
	bool	GetUseDwlPercentage()		{ return prefs->m_bShowDwlPercentage;}
	void	SetUseDwlPercentage(bool in){ prefs->m_bShowDwlPercentage=in;}

	//Toolbar
	CString GetToolbarSettings() 						{ return prefs->m_sToolbarSettings; }
	void	SetToolbarSettings(CString in) 				{ sprintf(prefs->m_sToolbarSettings,in);}
	CString GetToolbarBitmapSettings() 					{ return prefs->m_sToolbarBitmap; }
	void	SetToolbarBitmapSettings(CString path)		{ sprintf(prefs->m_sToolbarBitmap,path);}
	CString GetToolbarBitmapFolderSettings() 			{ return prefs->m_sToolbarBitmapFolder; }
	void	SetToolbarBitmapFolderSettings(CString path){ sprintf(prefs->m_sToolbarBitmapFolder,path); }
	int8	GetToolbarLabelSettings()					{ return prefs->m_nToolbarLabels; }
	void	SetToolbarLabelSettings(int8 settings)		{ prefs->m_nToolbarLabels= settings; }

	//preview
	bool	IsPreviewEnabled()			{ return true; }
	void	SetPreview(bool in)			{ prefs->m_bPreviewEnabled = in; }

	int		GetSearchMethod() const		{ return prefs->m_iSearchMethod; }
	void	SetSearchMethod(int iMethod){ prefs->m_iSearchMethod = iMethod; }

	bool	IsAutoUPdateIPFilterEnabled()		{ return prefs->AutoUpdateIPFilter; } //MORPH START added by Yun.SF3: Ipfilter.dat update

	// khaos::categorymod+
	bool	ShowValidSrcsOnly()		{ return prefs->m_bValidSrcsOnly; }
	bool	ShowCatNameInDownList()	{ return prefs->m_bShowCatNames; }
	bool	SelectCatForNewDL()		{ return prefs->m_bSelCatOnAdd; }
	bool	UseActiveCatForLinks()	{ return prefs->m_bActiveCatDefault; }
	bool	AutoSetResumeOrder()	{ return prefs->m_bAutoSetResumeOrder; }
	bool	SmallFileDLPush()		{ return prefs->m_bSmallFileDLPush; }
	uint8	StartDLInEmptyCats()	{ return prefs->m_iStartDLInEmptyCats; } // 0 = disabled, otherwise num to resume
	bool	UseAutoCat()			{ return prefs->m_bUseAutoCat; }
	// khaos::categorymod-
	// khaos::kmod+
	bool	UseSmartA4AFSwapping()	{ return prefs->m_bSmartA4AFSwapping; } // only for NNP swaps and file completes, stops, cancels, etc.
	uint8	AdvancedA4AFMode()		{ return prefs->m_iAdvancedA4AFMode; } // 0 = disabled, 1 = balance, 2 = stack -- controls the balancing routines for on queue sources
	bool	RespectMaxSources()		{ return prefs->m_bRespectMaxSources; }
	bool	ShowA4AFDebugOutput()	{ return prefs->m_bShowA4AFDebugOutput; }
	bool	UseSaveLoadSources()	{ return prefs->m_bUseSaveLoadSources; }
	// khaos::categorymod-
	// khaos::accuratetimerem+
	uint8	GetTimeRemainingMode()	{ return prefs->m_iTimeRemainingMode; }
	// khaos::accuratetimerem-

	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	bool	IsDynUpEnabled() { return prefs->m_bDynUpEnabled; }
	bool	IsUSSLog() {return prefs->m_bDynUpLog;}
	bool	IsUSSLimit() { return prefs->m_bIsUSSLimit;} // EastShare - Added by TAHO, USS limit
	void	SetDynUpEnabled(bool newValue) { prefs->m_bDynUpEnabled = newValue; }
	//EastShare START - Added by Pretender, add USS settings in scheduler tab
	void	SetDynUpPingLimit(int in) { prefs->m_iDynUpPingLimit = in; }
	void	SetDynUpGoingUpDivider(int in) { prefs->m_iDynUpGoingUpDivider = in; }
	void	SetDynUpGoingDownDivider(int in) { prefs->m_iDynUpGoingDownDivider = in; }
	//EastShare END - Added by Pretender, add USS settings in scheduler tab
	int		GetDynUpPingLimit() { return prefs->m_iDynUpPingLimit; } // EastShare - Added by TAHO, USS limit
	int		GetDynUpPingTolerance() { return prefs->m_iDynUpPingTolerance; }
	int		GetDynUpGoingUpDivider() { return prefs->m_iDynUpGoingUpDivider; }
	int		GetDynUpGoingDownDivider() { return prefs->m_iDynUpGoingDownDivider; }
	int		GetDynUpNumberOfPings() { return prefs->m_iDynUpNumberOfPings; }
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
protected:
	void	CreateUserHash();
	void	SetStandartValues();
	static int GetRecommendedMaxConnections();

private:
	CString appdir;
	CString configdir;
	CString m_strWebServerDir;
	CString m_strLangDir;
	Preferences_Struct* prefs;
	Preferences_Ext_Struct* prefsExt;

	Preferences_Import19c_Struct* prefsImport19c;
	Preferences_Import20a_Struct* prefsImport20a;
	Preferences_Import20b_Struct* prefsImport20b;
	
	char userhash[16];
	WORD m_wWinVer;

	void LoadPreferences();
	void SavePreferences();

	CArray<Category_Struct*,Category_Struct*> catMap;

	// deadlake PROXYSUPPORT
	bool m_UseProxyListenPort;
	uint16	ListenPort;
};
