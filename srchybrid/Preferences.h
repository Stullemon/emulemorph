//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once

#define	DFLT_TRANSFER_WND2	1

const CString strDefaultToolbar = _T("0099010203040506070899091011");

enum EViewSharedFilesAccess{
	vsfaEverybody = 0,
	vsfaFriends = 1,
	vsfaNobody = 2
};

// DO NOT EDIT VALUES like making a uint16 to uint32, or insert any value. ONLY append new vars
#pragma pack(1)
struct Preferences_Ext_Struct{
	uint8	version;
	uchar	userhash[16];
	WINDOWPLACEMENT EmuleWindowPlacement;
};
#pragma pack()

// deadlake PROXYSUPPORT
struct ProxySettings{
	uint16 type;
	uint16 port;
	TCHAR name[50];
	CHAR user[50];
	CHAR password[50];
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
	bool	bSeenComplet; //MORPH - Added by SiRoB, Seen Complet filter
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
	TCHAR	incomingpath[MAX_PATH];
	TCHAR	title[64];
	TCHAR	comment[255];
	DWORD	color;
	uint8	prio;
	// khaos::kmod+ Category Advanced A4AF Mode
	// 0 = Default, 1 = Balancing, 2 = Stacking
	uint8	iAdvA4AFMode;
	bool	bResumeFileOnlyInSameCat; //MORPH - Added by SiRoB, Resume file only in the same category
	// View Filter Struct
	CategoryViewFilter_Struct viewfilters;
	CategorySelectionCriteria_Struct selectioncriteria;
	// khaos::kmod-
	//MORPH - Removed by SiRoB, Due to Khaos Categorie
	/*
	CString autocat;
	*/
	BOOL    downloadInAlphabeticalOrder; // ZZ:DownloadManager
};
#pragma pack()
//MORPH START - Added by SiRoB, DynDNS
#pragma pack(1)
struct DynDNS_Struct{
	bool	Enabled;
	TCHAR	Username[256];
	TCHAR	Password[256];
	TCHAR	Hostname[256];
};
#pragma pack()
//MORPH END - Added by SiRoB, DynDNS

class CPreferences
{
public:
	static	CString	strNick;
	// ZZ:UploadSpeedSense -->
	static	uint16	minupload;
	// ZZ:UploadSpeedSense <--
	static	uint16	maxupload;
	static	uint16	maxdownload;
	static	uint16	port;
	static	uint16	udpport;
	static	uint16	nServerUDPPort;
	static	uint16	maxconnections;
	static	uint16	maxhalfconnections;
	static	uint8	reconnect;
	static	uint8	scorsystem;
	static	TCHAR	incomingdir[MAX_PATH];
	static	TCHAR	tempdir[MAX_PATH];
	static	uint8	ICH;
	static	uint8	autoserverlist;
	static	uint8	updatenotify;
	static	uint8	mintotray;
	static	uint8	autoconnect;
	static	uint8	autoconnectstaticonly; // Barry
	static	uint8	autotakeed2klinks;	   // Barry
	static	uint8	addnewfilespaused;	   // Barry
	static	uint8	depth3D;			   // Barry
	static	int		m_iStraightWindowStyles;
	static	TCHAR	m_szSkinProfile[MAX_PATH];
	static	TCHAR	m_szSkinProfileDir[MAX_PATH];
	static	uint8	addserversfromserver;
	static	uint8	addserversfromclient;
	static	uint16	maxsourceperfile;
	static	uint16	trafficOMeterInterval;
	static	uint16	statsInterval;
	static	uchar	userhash[16];
	static	WINDOWPLACEMENT EmuleWindowPlacement;
	static	int		maxGraphDownloadRate;
	static	int		maxGraphUploadRate;
	static	uint8	beepOnError;
	static	uint8	confirmExit;
	static	uint16	downloadColumnWidths[17]; /*13 Official+ 2 Khaos+1 WC+1 Country*/
	static	BOOL	downloadColumnHidden[17]; /*13 Official+ 2 Khaos+1 WC+1 Country*/
	static	INT		downloadColumnOrder[17];  /*13 Official+ 2 Khaos+1 WC+1 Country*/
	//SLAHAM: ADDED DownloadClientsCtrl =>
	static uint16	downloadClientsColumnWidths[12];
	static BOOL		downloadClientsColumnHidden[12];
	static INT		downloadClientsColumnOrder[12];
	//SLAHAM: ADDED DownloadClientsCtrl <=
	static	uint16	uploadColumnWidths[16]; /*8+1 MOD_VERSION+1 Upload/Download+1 Download Status+1 Slot+1 Compression+1 Community+1 Friend+1 Country*/
	static	BOOL	uploadColumnHidden[16]; /*8+1 MOD_VERSION+1 Upload/Download+1 Download Status+1 Slot+1 Compression+1 Community+1 Friend+1 Country*/
	static	INT		uploadColumnOrder[16];  /*8+1 MOD_VERSION+1 Upload/Download+1 Download Status+1 Slot+1 Compression+1 Community+1 Friend+1 Country*/
	static	uint16	queueColumnWidths[15];  /*10+1 MOD_VERSION+1 Community+1 Friend+1 Country+1 WC*/
	static	BOOL	queueColumnHidden[15];  /*10+1 MOD_VERSION+1 Community+1 Friend+1 Country+1 WC*/
	static	INT		queueColumnOrder[15];  /*10+1 MOD_VERSION+1 Community+1 Friend+1 Country+1 WC*/
	static	uint16	searchColumnWidths[15]; /*14+1 Fakecheck*/
	static	BOOL	searchColumnHidden[15]; /*14+1 Fakecheck*/
	static	INT		searchColumnOrder[15];  /*14+1 Fakecheck*/
	static	uint16	sharedColumnWidths[22]; /*12+1 PERMISSION+1 PWSHARE+4 Spreadbars+1 HIDEOS+1 Share_Only_TheNeed+2 CRC32-Tag*/
	static	BOOL	sharedColumnHidden[22]; /*12+1 PERMISSION+1 PWSHARE+4 Spreadbars+1 HIDEOS+1 Share_Only_TheNeed+2 CRC32-Tag*/
	static	INT		sharedColumnOrder[22];  /*12+1 PERMISSION+1 PWSHARE+4 Spreadbars+1 HIDEOS+1 Share_Only_TheNeed+2 CRC32-Tag*/
	static	uint16	serverColumnWidths[16]; /*14+ 1 Aux Port+1 Country*/
	static	BOOL	serverColumnHidden[16]; /*14+ 1 Aux Port+1 Country*/
	static	INT		serverColumnOrder[16];  /*14+ 1 Aux Port+1 Country*/
	static	uint16	clientListColumnWidths[11]; /*8+1 Community+1 Friend+1 Country*/
	static	BOOL	clientListColumnHidden[11]; /*8+1 Community+1 Friend+1 Country*/
	static	INT		clientListColumnOrder[11];  /*8+1 Community+1 Friend+1 Country*/
	static	uint16	FilenamesListColumnWidths[2];
	static	BOOL	FilenamesListColumnHidden[2];
	static	INT		FilenamesListColumnOrder[2];
	static	DWORD	statcolors[15];

	static	uint8	splashscreen;
	static	uint8	startupsound;//Commander - Added: Enable/Disable Startupsound
	static	uint8	sidebanner;//Commander - Added: Side Banner
	static	uint8	filterLANIPs;
	static	bool	m_bAllocLocalHostIP;
	static	uint8	onlineSig;

	// -khaos--+++> Struct Members for Storing Statistics

	// Saved stats for cumulative downline overhead...
	static	uint64	cumDownOverheadTotal;
	static	uint64	cumDownOverheadFileReq;
	static	uint64	cumDownOverheadSrcEx;
	static	uint64	cumDownOverheadServer;
	static	uint64	cumDownOverheadKad;
	static	uint64	cumDownOverheadTotalPackets;
	static	uint64	cumDownOverheadFileReqPackets;
	static	uint64	cumDownOverheadSrcExPackets;
	static	uint64	cumDownOverheadServerPackets;
	static	uint64	cumDownOverheadKadPackets;

	// Saved stats for cumulative upline overhead...
	static	uint64	cumUpOverheadTotal;
	static	uint64	cumUpOverheadFileReq;
	static	uint64	cumUpOverheadSrcEx;
	static	uint64	cumUpOverheadServer;
	static	uint64	cumUpOverheadKad;
	static	uint64	cumUpOverheadTotalPackets;
	static	uint64	cumUpOverheadFileReqPackets;
	static	uint64	cumUpOverheadSrcExPackets;
	static	uint64	cumUpOverheadServerPackets;
	static	uint64	cumUpOverheadKadPackets;

	// Saved stats for cumulative upline data...
	static	uint32	cumUpSuccessfulSessions;
	static	uint32	cumUpFailedSessions;
	static	uint32	cumUpAvgTime;
	// Cumulative client breakdown stats for sent bytes...
	static	uint64	cumUpData_EDONKEY;
	static	uint64	cumUpData_EDONKEYHYBRID;
	static	uint64	cumUpData_EMULE;
	static	uint64	cumUpData_MLDONKEY;
	static	uint64	cumUpData_AMULE;
	static	uint64	cumUpData_EMULECOMPAT;
	static	uint64	cumUpData_SHAREAZA;
	// Session client breakdown stats for sent bytes...
	static	uint64	sesUpData_EDONKEY;
	static	uint64	sesUpData_EDONKEYHYBRID;
	static	uint64	sesUpData_EMULE;
	static	uint64	sesUpData_MLDONKEY;
	static	uint64	sesUpData_AMULE;
	static	uint64	sesUpData_EMULECOMPAT;
	static	uint64	sesUpData_SHAREAZA;

	// Cumulative port breakdown stats for sent bytes...
	static	uint64	cumUpDataPort_4662;
	static	uint64	cumUpDataPort_OTHER;
	// Session port breakdown stats for sent bytes...
	static	uint64	sesUpDataPort_4662;
	static	uint64	sesUpDataPort_OTHER;

	// Cumulative source breakdown stats for sent bytes...
	static	uint64	cumUpData_File;
	static	uint64	cumUpData_Partfile;
	// Session source breakdown stats for sent bytes...
	static	uint64	sesUpData_File;
	static	uint64	sesUpData_Partfile;

	// Saved stats for cumulative downline data...
	static	uint32	cumDownCompletedFiles;
	static	uint32	cumDownSuccessfulSessions;
	static	uint32	cumDownFailedSessions;
	static	uint32	cumDownAvgTime;

	// Cumulative statistics for saved due to compression/lost due to corruption
	static	uint64	cumLostFromCorruption;
	static	uint64	cumSavedFromCompression;
	static	uint32	cumPartsSavedByICH;

	// Session statistics for download sessions
	static	uint32	sesDownSuccessfulSessions;
	static	uint32	sesDownFailedSessions;
	static	uint32	sesDownAvgTime;
	static	uint32	sesDownCompletedFiles;
	static	uint64	sesLostFromCorruption;
	static	uint64	sesSavedFromCompression;
	static	uint32	sesPartsSavedByICH;

	// Cumulative client breakdown stats for received bytes...
	static	uint64	cumDownData_EDONKEY;
	static	uint64	cumDownData_EDONKEYHYBRID;
	static	uint64	cumDownData_EMULE;
	static	uint64	cumDownData_MLDONKEY;
	static	uint64	cumDownData_AMULE;
	static	uint64	cumDownData_EMULECOMPAT;
	static	uint64	cumDownData_SHAREAZA;
	static	uint64	cumDownData_URL;
	static	uint64	cumDownData_WEBCACHE; //jp webcache statistics // MORPH - Added by Commander, WebCache 1.2e
	// Session client breakdown stats for received bytes...
	static	uint64	sesDownData_EDONKEY;
	static	uint64	sesDownData_EDONKEYHYBRID;
	static	uint64	sesDownData_EMULE;
	static	uint64	sesDownData_MLDONKEY;
	static	uint64	sesDownData_AMULE;
	static	uint64	sesDownData_EMULECOMPAT;
	static	uint64	sesDownData_SHAREAZA;
	static	uint64	sesDownData_URL;
	// MORPH START - Added by Commander, WebCache 1.2e
	static	uint64	sesDownData_WEBCACHE; //jp webcache statistics
	static  uint32	ses_WEBCACHEREQUESTS; //jp webcache statistics
	static	uint32	ses_successfull_WCDOWNLOADS;  //jp webcache statistics
	// MORPH END - Added by Commander, WebCache 1.2e

	// Cumulative port breakdown stats for received bytes...
	static	uint64	cumDownDataPort_4662;
	static	uint64	cumDownDataPort_OTHER;
	// Session port breakdown stats for received bytes...
	static	uint64	sesDownDataPort_4662;
	static	uint64	sesDownDataPort_OTHER;

	// Saved stats for cumulative connection data...
	static	float	cumConnAvgDownRate;
	static	float	cumConnMaxAvgDownRate;
	static	float	cumConnMaxDownRate;
	static	float	cumConnAvgUpRate;
	static	float	cumConnMaxAvgUpRate;
	static	float	cumConnMaxUpRate;
	static	uint64	cumConnRunTime;
	static	uint32	cumConnNumReconnects;
	static	uint32	cumConnAvgConnections;
	static	uint32	cumConnMaxConnLimitReached;
	static	uint32	cumConnPeakConnections;
	static	uint32	cumConnTransferTime;
	static	uint32	cumConnDownloadTime;
	static	uint32	cumConnUploadTime;
	static	uint32	cumConnServerDuration;

	// Saved records for servers / network...
	static	uint32	cumSrvrsMostWorkingServers;
	static	uint32	cumSrvrsMostUsersOnline;
	static	uint32	cumSrvrsMostFilesAvail;

	// Saved records for shared files...
	static	uint32	cumSharedMostFilesShared;
	static	uint64	cumSharedLargestShareSize;
	static	uint64	cumSharedLargestAvgFileSize;
	static	uint64	cumSharedLargestFileSize;

	// Save the date when the statistics were last reset...
	static	__int64 stat_datetimeLastReset;

	// Save new preferences for PPgStats
	static	uint8	statsConnectionsGraphRatio; // This will store the divisor, i.e. for 1:3 it will be 3, for 1:20 it will be 20.
	// Save the expanded branches of the stats tree
	static	TCHAR	statsExpandedTreeItems[256];

	static	UINT	statsSaveInterval;
	// <-----khaos- End Statistics Members


	// Original Stats Stuff
	static	uint64	totalDownloadedBytes;
	static	uint64	totalUploadedBytes;
	// End Original Stats Stuff
	static	WORD	m_wLanguageID;
	static	uint8	transferDoubleclick;
	static	EViewSharedFilesAccess m_iSeeShares;
	static	uint8	m_iToolDelayTime;	// tooltip delay time in seconds
	static	uint8	bringtoforeground;
	static	uint8	splitterbarPosition;
	static	uint8	m_uTransferWnd2;
	//MORPH START - Added by SiRoB, Splitting Bar [O²]
	static	uint8	splitterbarPositionStat;
	static	uint8	splitterbarPositionStat_HL;
	static	uint8	splitterbarPositionStat_HR;
	static	uint16	splitterbarPositionFriend;
	static	uint16	splitterbarPositionIRC;
	//MORPH END - Added by SiRoB, Splitting Bar [O²]
	static	uint16	deadserverretries;
	static	DWORD	m_dwServerKeepAliveTimeout;
	// -khaos--+++> Changed data type to avoid overflows
	static	uint16	statsMax;
	// <-----khaos-
	static	uint8	statsAverageMinutes;

	static	uint8	useDownloadNotifier;
	static	uint8	useNewDownloadNotifier;
	static	uint8	useChatNotifier;
	static	uint8	useLogNotifier;
	static	uint8	useSoundInNotifier;
	static	uint8	notifierPopsEveryChatMsg;
	static	uint8	notifierImportantError;
	static	uint8	notifierNewVersion;
	static	TCHAR	notifierSoundFilePath[510];

	static	TCHAR	m_sircserver[50];
	static	TCHAR	m_sircnick[30];
	static	TCHAR	m_sircchannamefilter[50];
	static	bool	m_bircaddtimestamp;
	static	bool	m_bircusechanfilter;
	static	uint16	m_iircchanneluserfilter;
	static	TCHAR	m_sircperformstring[255];
	static	bool	m_bircuseperform;
	static	bool	m_birclistonconnect;
	static	bool	m_bircacceptlinks;
	static	bool	m_bircacceptlinksfriends;
	static	bool	m_bircsoundevents;
	static	bool	m_bircignoremiscmessage;
	static	bool	m_bircignorejoinmessage;
	static	bool	m_bircignorepartmessage;
	static	bool	m_bircignorequitmessage;
	static	bool	m_bircignoreemuleprotoaddfriend;
	static	bool	m_bircallowemuleprotoaddfriend;
	static	bool	m_bircignoreemuleprotosendlink;
	static	bool	m_birchelpchannel;

	static	bool	m_bRemove2bin;

	static	bool	m_bpreviewprio;
	static	bool	smartidcheck;
	static	uint8	smartidstate;
	static	bool	safeServerConnect;
	static	bool	startMinimized;
	static	bool	m_bAutoStart;
	static	bool	m_bRestoreLastMainWndDlg;
	static	int		m_iLastMainWndDlgID;
	static	bool	m_bRestoreLastLogPane;
	static	int		m_iLastLogPaneID;
	static	uint16	MaxConperFive;
	static	int		checkDiskspace; // SLUGFILLER: checkDiskspace
	static	UINT	m_uMinFreeDiskSpace;
	static	bool	m_bSparsePartFiles;
	static	TCHAR	yourHostname[127];	// itsonlyme: hostnameSource
	static	bool	m_bEnableVerboseOptions;
	static	bool	m_bVerbose;
	//MORPH START - Added by SiRoB, XML News [O²]
	static	bool	enableNEWS;
	//MORPH END   - Added by SiRoB, XML News [O²]
	static	bool	m_bFullVerbose;
	static	bool	m_bDebugSourceExchange; // Sony April 23. 2003, button to keep source exchange msg out of verbose log
	static	bool	m_bLogBannedClients;
	static	bool	m_bLogRatingDescReceived;
	static	bool	m_bLogSecureIdent;
	static	bool	m_bLogFilteredIPs;
	static	bool	m_bLogFileSaving;
    static  bool    m_bLogA4AF; // ZZ:DownloadManager
	static	bool	m_bLogUlDlEvents;
	// MORPH START - Added by Commander, WebCache 1.2f
	static	bool	m_bLogWebCacheEvents;//JP log webcache events
	static	bool	m_bLogICHEvents;//JP log ICH events
	// MORPH END   - Added by Commander, WebCache 1.2f
	static	bool	m_bUseDebugDevice;
	static	int		m_iDebugServerTCPLevel;
	static	int		m_iDebugServerUDPLevel;
	static	int		m_iDebugServerSourcesLevel;
	static	int		m_iDebugServerSearchesLevel;
	static	int		m_iDebugClientTCPLevel;
	static	int		m_iDebugClientUDPLevel;
	static	int		m_iDebugClientKadUDPLevel;
	static	bool	m_bupdatequeuelist;
	static	bool	m_bmanualhighprio;
	static	bool	m_btransferfullchunks;
	static	int		m_istartnextfile;
	static	bool	m_bshowoverhead;
	static	bool	m_bDAP;
	static	bool	m_bUAP;
	static	bool	m_bDisableKnownClientList;
	static	bool	m_bDisableQueueList;
	static	bool	m_bExtControls;
	static	bool	m_bTransflstRemain;

	static	uint8	versioncheckdays;

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	// SLUGFILLER: multiSort - save multiple params
	// SLUGFILLER: DLsortFix - double, for client-only sorting
	static	int		tableSortItemDownload[34];
	static	BOOL	tableSortAscendingDownload[34];
	// SLUGFILLER: DLsortFix
	//SLAHAM: ADDED DownloadClientsCtrl =>
	static	int		tableSortItemdownloadClients[8]; 	
	static	BOOL	tableSortAscendingdownloadClients[8]; 
	//SLAHAM: ADDED DownloadClientsCtrl <=
	static	int		tableSortItemUpload[16];
	static	BOOL	tableSortAscendingUpload[16];
	static	int		tableSortItemQueue[15];
	static	BOOL	tableSortAscendingQueue[15];
	static	int		tableSortItemSearch[15];
	static	BOOL	tableSortAscendingSearch[15];
	static	int		tableSortItemShared[22];
	static	BOOL	tableSortAscendingShared[22];
	static	int		tableSortItemServer[16];
	static	BOOL	tableSortAscendingServer[16];
	static	int		tableSortItemClientList[11];
	static	BOOL	tableSortAscendingClientList[11];
	static	int		tableSortItemFilenames[2];
	static	BOOL	tableSortAscendingFilenames[2];
	// SLUGFILLER: multiSort

	static	bool	showRatesInTitle;

	static	TCHAR	TxtEditor[256];
	static	TCHAR	VideoPlayer[256];
	static	bool	moviePreviewBackup;
	static	int		m_iPreviewSmallBlocks;
	static	int		m_iPreviewCopiedArchives;
	static	int		m_iInspectAllFileTypes;
	static	bool	indicateratings;
	static	bool	watchclipboard;
	static	bool	filterserverbyip;
	static	bool	m_bFirstStart;
	static	bool	m_bCreditSystem;

	static	bool	log2disk;
	static	bool	debug2disk;
	static	int		iMaxLogBuff;
	static	UINT	uMaxLogFileSize;
	static	bool	scheduler;
	static	bool	dontcompressavi;
	static	bool	msgonlyfriends;
	static	bool	msgsecure;

	static	uint8	filterlevel;
	static	UINT	m_iFileBufferSize;
	static	UINT	m_iQueueSize;
	static	int		m_iCommitFiles;

	static	uint16	maxmsgsessions;
	static	uint32	versioncheckLastAutomatic;
	//MORPH START - Added by SiRoB, New Version check
	static	uint32	mversioncheckLastAutomatic;
	//MORPH START - Added by SiRoB, New Version check
	static	TCHAR	messageFilter[512];
	static	CString	commentFilter;
	static	TCHAR	filenameCleanups[512];
	static	TCHAR	notifierConfiguration[510];
	static	TCHAR	datetimeformat[64];
	static	TCHAR	datetimeformat4log[64];
	static	LOGFONT m_lfHyperText;
	static	LOGFONT m_lfLogText;
	static	COLORREF m_crLogError;
	static	COLORREF m_crLogWarning;
	static	COLORREF m_crLogSuccess;
	//MORPH START - Added by SiRoB, Upload Splitting Class
	static	COLORREF m_crLogUSC;
	//MORPH END   - Added by SiRoB, Upload Splitting Class

	static	int		m_iExtractMetaData;
	static	bool	m_bAdjustNTFSDaylightFileTime;

	// Web Server [kuchin]
	static	TCHAR	m_sWebPassword[256];
	static	TCHAR	m_sWebLowPassword[256];
	static	uint16	m_nWebPort;
	static	bool	m_bWebEnabled;
	static	bool	m_bWebUseGzip;
	static	int		m_nWebPageRefresh;
	static	bool	m_bWebLowEnabled;
	static	TCHAR	m_sWebResDir[MAX_PATH];
	static	int		m_iWebTimeoutMins;

	static	TCHAR	m_sTemplateFile[MAX_PATH];
	static	ProxySettings proxy; // deadlake PROXYSUPPORT
	static	bool	m_bIsASCWOP;
	static	bool	m_bShowProxyErrors;

	static	bool	showCatTabInfos;
	static	bool	resumeSameCat;
	static	bool	dontRecreateGraphs;
	static	bool	autofilenamecleanup;
	// khaos::kmod+ Obsolete static	int		allcatType;
	static	bool	m_bUseAutocompl;
	static	bool	m_bShowDwlPercentage;
	static	bool	m_bShowClientPercentage; //Commander - Added: Client Percentage
	static	bool	m_bRemoveFinishedDownloads;
	static	uint16	m_iMaxChatHistory;
	static	bool	m_bShowActiveDownloadsBold;

	static	int		m_iSearchMethod;
	static	bool	m_bAdvancedSpamfilter;
	static	bool	m_bUseSecureIdent;
	// mobilemule
	static	TCHAR	m_sMMPassword[256];
	static	bool	m_bMMEnabled;
	static	uint16	m_nMMPort;

	static	bool	networkkademlia;
	static	bool	networked2k;

	// toolbar
	static	uint8	m_nToolbarLabels;
	static	TCHAR	m_sToolbarBitmap[256];
	static	TCHAR	m_sToolbarBitmapFolder[256];
	static	TCHAR	m_sToolbarSettings[256];

	//preview
	static	bool	m_bPreviewEnabled;

//MORPH START - Added by IceCream, Defeat 0-filled Part Senders from Maella
	static bool	enableZeroFilledTest;  // -Defeat 0-filled Part Senders- (Idea of xrmb)
//MORPH END   - Added by IceCream, Defeat 0-filled Part Senders from Maella
	static bool	enableDownloadInRed; //MORPH - Added by IceCream, show download in red
	static bool	enableAntiLeecher; //MORPH - Added by IceCream, enableAntiLeecher
	static bool	enableAntiCreditHack; //MORPH - Added by IceCream, enableAntiCreditHack
	static uint8	creditSystemMode; // EastShare - Added by linekin, creditsystem integration
	static bool	m_bEnableEqualChanceForEachFile;//Morph - added by AndCycle, Equal Chance For Each File
	static bool	isautodynupswitching;//MORPH - Added by Yun.SF3, Auto DynUp changing
	static uint8	m_iPowershareMode; //MORPH - Added by SiRoB, Avoid misusing of powersharing
	static bool m_bPowershareInternalPrio; //Morph - added by AndCyle, selective PS internal Prio
	static uint16	maxconnectionsswitchborder;
	//EastShare Start- Added by Pretender, TBH-AutoBackup
	static bool	autobackup;
	static bool	autobackup2;
	//EastShare End - Added by Pretender, TBH-AutoBackup
	//MORPH START - Added by SiRoB, Upload Splitting Class
	static uint16	globaldataratefriend;
	static uint16	globaldataratepowershare;
	static uint16	maxclientdataratefriend;
	static uint16	maxclientdataratepowershare;
	static uint16	maxclientdatarate;
	//MORPH END   - Added by SiRoB, Upload Splitting Class
	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	static uint8	LowIdRetries;
	static uint8	LowIdRetried;
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry
	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	static uint8	m_iSpreadbarSetStatus;
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	static uint8	hideOS;
	static uint8	selectiveShare;
	//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS

	static bool	infiniteQueue;	//Morph - added by AndCycle, SLUGFILLER: infiniteQueue

	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	static uint8	ShareOnlyTheNeed;
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	static uint8	PowerShareLimit;
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	
	//MORPH START - Added by SiRoB, Show Permissions
	static uint8	permissions;
	//MORPH END   - Added by SiRoB, Show Permissions
	
	//EastShare Start - PreferShareAll by AndCycle
	static bool	shareall;	// SLUGFILLER: preferShareAll
	//EastShare End - PreferShareAll by AndCycle

	//EastShare - Added by Pretender, Option for ChunkDots
	static bool	m_bEnableChunkDots;
	//EastShare - Added by Pretender, Option for ChunkDots

	static TCHAR UpdateURLFakeList[256];//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	static TCHAR UpdateURLIPFilter[256];//MORPH START added by Yun.SF3: Ipfilter.dat update
	static TCHAR UpdateURLIP2Country[256]; //Commander - Added: IP2Country auto-updating
	static TCHAR UpdateVerURLIP2Country[256];//Commander - Added: IP2Country auto-updating

    //MORPH START - Added by Commander, ClientQueueProgressBar
    static bool m_bClientQueueProgressBar;
	//MORPH END - Added by Commander, ClientQueueProgressBar
    
	//MORPH START - Added by Commander, Show WC stats
	static bool m_bCountWCSessionStats;
	//MORPH END - Added by Commander, Show WC stats

	//MORPH START - Added by Commander, FolderIcons
	static bool m_bShowFolderIcons;
	//MORPH END - Added by Commander, FolderIcons

	static bool	m_bPayBackFirst;//EastShare - added by AndCycle, Pay Back First
	static uint8	m_iPayBackFirstLimit;//MORPH - Added by SiRoB, Pay Back First Tweak
	static bool	m_bOnlyDownloadCompleteFiles;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	static bool	m_bSaveUploadQueueWaitTime;//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	static int		m_iKnownMetDays; // EastShare - Added by TAHO, .met file control
	static bool	m_bDateFileNameLog;//Morph - added by AndCycle, Date File Name Log
	static bool m_bDontRemoveSpareTrickleSlot;//Morph - added by AndCycle, Dont Remove Spare Trickle Slot
	
	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	static uint32		m_FakesDatVersion;
	static bool		UpdateFakeStartup;
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating

	//MORPH START added by Yun.SF3: Ipfilter.dat update
	static bool		AutoUpdateIPFilter; //added by milobac: Ipfilter.dat update
	static uint32		m_IPfilterVersion; //added by milobac: Ipfilter.dat update
	//MORPH END added by Yun.SF3: Ipfilter.dat update

	//Commander - Added: IP2Country Auto-updating - Start
	static bool		AutoUpdateIP2Country; 
	static uint32	m_IP2CountryVersion;
        //Commander - Added: IP2Country Auto-updating - End

	//EastShare - added by AndCycle, IP to Country
	static uint8	m_iIP2CountryNameMode;
	static bool		m_bIP2CountryShowFlag;
	//EastShare - added by AndCycle, IP to Country

	// khaos::categorymod+
	static bool		m_bValidSrcsOnly;
	static bool		m_bShowCatNames;
	static bool		m_bActiveCatDefault;
	static bool		m_bSelCatOnAdd;
	static bool		m_bAutoSetResumeOrder;
	static bool		m_bSmallFileDLPush;
	static uint8		m_iStartDLInEmptyCats;
	static bool		m_bRespectMaxSources;
	static bool		m_bUseAutoCat;
	// khaos::categorymod-
	// khaos::kmod+
	static bool		m_bShowA4AFDebugOutput;
	static bool		m_bSmartA4AFSwapping;
	static uint8		m_iAdvancedA4AFMode; // 0 = disabled, 1 = balance, 2 = stack
	static bool		m_bUseSaveLoadSources;
	// khaos::categorymod-
	// khaos::accuratetimerem+
	static uint8		m_iTimeRemainingMode; // 0 = both, 1 = real time, 2 = average
	// khaos::accuratetimerem-
	//MORPH START - Added by SiRoB, ICS Optional
	static bool		m_bUseIntelligentChunkSelection;
	//MORPH END   - Added by SiRoB, ICS Optional
	
	// Mighty Knife: Community Visualization, Report hashing files, Log friendlist activities
	static TCHAR	m_sCommunityName [256];
	static bool		m_bReportHashingFiles;
	static bool	    m_bLogFriendlistActivities;
	// [end] Mighty Knife

	// Mighty Knife: CRC32-Tag - not accessible in preferences dialog !
	static bool		m_bDontAddCRCToFilename;
	static bool		m_bCRC32ForceUppercase;
	static bool		m_bCRC32ForceAdding;
	static TCHAR		m_sCRC32Prefix [256];
	static TCHAR		m_sCRC32Suffix [256];
	// [end] Mighty Knife

	// Mighty Knife: Simple cleanup options
	static int      m_SimpleCleanupOptions;
	static CString  m_SimpleCleanupSearch;
	static CString  m_SimpleCleanupReplace;
	static CString  m_SimpleCleanupSearchChars;
	static CString  m_SimpleCleanupReplaceChars;
	// [end] Mighty Knife

	// Mighty Knife: Static server handling
	static bool		m_bDontRemoveStaticServers;
	// [end] Mighty Knife

	//MORPH START - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	static bool		m_bSUCEnabled;
	static uint16		m_iSUCHigh;
	static uint16		m_iSUCLow;
	static uint16		m_iSUCPitch;
	static uint16		m_iSUCDrift;
	static bool		m_bSUCLog;
	//MORPH END - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]

	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	static	bool	m_bDynUpEnabled;
	static	int		m_iDynUpPingTolerance;
	static	int		m_iDynUpGoingUpDivider;
	static	int		m_iDynUpGoingDownDivider;
	static	int		m_iDynUpNumberOfPings;
	static  int		m_iDynUpPingToleranceMilliseconds;
	static  bool	m_bDynUpUseMillisecondPingTolerance;
	static bool		m_bDynUpLog;
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)

	static bool		m_bSolidGraph; //MORPH - Added by SiRoB, New Graph

    //Commander - Added: Invisible Mode [TPT] - Start
    static bool		m_bInvisibleMode;		
	static UINT		m_iInvisibleModeHotKeyModifier;
	static char		m_cInvisibleModeHotKey;
    //Commander - Added: Invisible Mode [TPT] - End

        static bool     m_bA4AFSaveCpu; // ZZ:DownloadManager
        
	//MORPH START - Added by SiRoB, [MoNKi: -UPnPNAT Support-]
	static bool		m_bUPnPNat;
	static bool		m_bUPnPNatWeb;
	static uint16	m_iUPnPTCPExternal;
	static uint16	m_iUPnPUDPExternal;
	static uint16	m_iUPnPTCPInternal;
	static uint16	m_iUPnPUDPInternal;
	static bool		m_bUPnPTryRandom;
	//MORPH END   - Added by SiRoB, [MoNKi: -UPnPNAT Support-]

	//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-]
	static bool		m_bRndPorts;
	static uint16	m_iMinRndPort;
	static uint16	m_iMaxRndPort;
	//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-]

	//MORPH START - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
	static bool		m_bICFSupport;
	static bool		m_bICFSupportFirstTime;
	static bool		m_bICFSupportStatusChanged;
	static bool		m_bICFSupportServerUDP;
	//MORPH END   - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]

    //MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
	static TCHAR	m_sWapTemplateFile[MAX_PATH];
	static bool		m_bWapEnabled;
	static uint16	m_nWapPort;
	static uint8	m_iWapGraphWidth;
	static uint8	m_iWapGraphHeight;
	static bool		m_bWapFilledGraphs;
	static int		m_iWapMaxItemsInPages;
	static bool		m_bWapSendImages;
	static bool		m_bWapSendGraphs;
	static bool		m_bWapSendProgressBars;
	static bool		m_bWapAllwaysSendBWImages;
	static UINT		m_iWapLogsSize;
	static CString	m_strWapServerDir;
	static CString	m_sWapPassword;
	static CString	m_sWapLowPassword;
	static bool		m_bWapLowEnabled;
	//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]

	static	CStringList shareddir_list;
	static	CStringList adresses_list;

	static	int		m_iDbgHeap;
	static	uint8	m_nWebMirrorAlertLevel;
	static	bool	m_bRunAsUser;

	static  bool	m_bUseOldTimeRemaining;

	// PeerCache
	static	uint32	m_uPeerCacheLastSearch;
	static	bool	m_bPeerCacheWasFound;
	static	bool	m_bPeerCacheEnabled;
	static	uint16	m_nPeerCachePort;

	// Firewall settings
	static bool		m_bOpenPortsOnStartUp;

	//AICH Options
	static bool		m_bTrustEveryHash;

	static uint8	m_byLogLevel;

	// MORPH START - Added by Commander, WebCache 1.2f
	static	bool	m_bHighIdPossible; // JP detect fake HighID (from netfinity)
	static	bool	WebCacheDisabledThisSession; //JP temp disabler
	static	uint32	WebCachePingSendTime;//jp check proxy config
	static	bool	expectingWebCachePing;//jp check proxy config
	static	bool	IsWebCacheTestPossible(); //jp check proxy config
	static	CString	webcacheName;		//jp move these to private?? and make member functions to set and change them??
	static	uint16	webcachePort;
	static	bool	webcacheReleaseAllowed; //jp webcache release
	static	bool	IsWebcacheReleaseAllowed() {return webcacheReleaseAllowed;}//jp webcache release
	static	bool	UpdateWebcacheReleaseAllowed();//jp webcache release
	static	bool	WebCacheIsTransparent() {return webcacheName.GetLength() > 15 && webcacheName.Left(12) == "transparent@";}
	static	uint16	webcacheBlockLimit;
	static	void	SetWebCacheBlockLimit(uint16 limit) {webcacheBlockLimit = limit;}
	static	uint16	GetWebCacheBlockLimit() {return webcacheBlockLimit;}
	static	bool	webcacheExtraTimeout;
	static	bool	PersistentConnectionsForProxyDownloads;
	static	void	SetWebCacheExtraTimeout(bool value) {webcacheExtraTimeout = value;}
	static	bool	GetWebCacheExtraTimeout() {return webcacheExtraTimeout;}
	static	bool	webcacheCachesLocalTraffic;
	static	void	SetWebCacheCachesLocalTraffic(bool value) {webcacheCachesLocalTraffic = value;}
	static	bool	GetWebCacheCachesLocalTraffic() {return webcacheCachesLocalTraffic;}
	static	bool	webcacheEnabled;
	static	bool	IsWebCacheDownloadEnabled() {return webcacheEnabled && !WebCacheDisabledThisSession;} //jp
	static	bool	UsesCachedTCPPort();	//jp
	static	bool	detectWebcacheOnStartup; // jp detect webcache on startup
	static	uint32	webcacheLastSearch;
	static	void	SetWebCacheLastSearch(uint32 time) {webcacheLastSearch = time;}
	static	uint32	GetWebCacheLastSearch() {return webcacheLastSearch;}
	static	uint32	webcacheLastGlobalIP;
	static	void	SetWebCacheLastGlobalIP(uint32 IP) {webcacheLastGlobalIP = IP;}
	static	uint32	GetWebCacheLastGlobalIP() {return webcacheLastGlobalIP;}
	static	CString	webcacheLastResolvedName;
	static	void	SetLastResolvedName(CString name) {webcacheLastResolvedName = name;}
	static	CString	GetLastResolvedName()	{return webcacheLastResolvedName;}
	// MORPH END - Added by Commander, WebCache 1.2f

	enum Table
	{
		tableDownload, 
		tabledownloadClients,  //SLAHAM: ADDED DownloadClientsCtrl
		tableUpload, 
		tableQueue, 
		tableSearch,
		tableShared, 
		tableServer, 
		tableClientList,
		tableFilenames
	};

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
	friend class CPPgDebug;
	friend class CPPgMorph; //MORPH - Added by SiRoB, Morph Prefs
	friend class CPPgMorph2; //MORPH - Added by SiRoB, Morph Prefs
	//friend class CPPgMorph3; //Commander - Added: Morph III
	friend class CPPgEastShare; //EastShare - Added by Pretender, ES Prefs
	
	CPreferences();
	~CPreferences();

	static	void	Init();
	static	void	Uninit();

	static	const CString& GetAppDir()				{return appdir;}
	static	LPCTSTR GetIncomingDir()				{return incomingdir;}
	static	LPCTSTR GetTempDir()					{return tempdir;}
	static	const CString& GetConfigDir()			{return configdir;}
	static	const CString& GetWebServerDir()		{return m_strWebServerDir;}
	static	const CString& GetFileCommentsFilePath(){return m_strFileCommentsFilePath;}
	static	const CString& GetLogDir()				{return m_strLogDir;}

	//MORPH START - Added by SiRoB, XML News [O²]
	static	const CString& GetFeedsDir()			{return m_strFeedsDir;} // Added by N_OxYdE: XML News
	//MORPH END   - Added by SiRoB, XML News [O²]

	static	bool	IsTempFile(const CString& rstrDirectory, const CString& rstrName);
	static	bool	IsConfigFile(const CString& rstrDirectory, const CString& rstrName);
	static	bool	IsShareableDirectory(const CString& rstrDirectory);
	static	bool	IsInstallationDirectory(const CString& rstrDir);

	static	bool	Save();
	static	void	SaveCats();

	static	uint8	Score()							{return scorsystem;}
	static	bool	Reconnect()						{return reconnect;}
	static	const CString& GetUserNick()			{return strNick;}
	static	void	SetUserNick(LPCTSTR pszNick);
	static	int		GetMaxUserNickLength()			{return 50;}

	//MORPH START - Changed by SiRoB, [MoNKi: -UPnPNAT Support-] [MoNKi: -Random Ports-]
	// emulEspaña: Modified by MoNKi [MoNKi: -UPnPNAT Support-]
	/*
	static	uint16	GetPort()		{return port;}
	static	uint16	GetUDPPort()	{return udpport;}
	*/
		// emulEspaña: Modified by MoNKi [MoNKi: -Random Ports-]
		/*
		static	uint16	GetPort();
		static	uint16	GetUDPPort();
		*/
		static	uint16	GetPort(bool newPort = false, bool original = false, bool reset = false);
		static	uint16	GetUDPPort(bool newPort = false, bool original = false, bool reset = false);
	//MORPH END   - Changed by SiRoB, [MoNKi: -UPnPNAT Support-] [MoNKi: -Random Ports-]

	static	uint16	GetServerUDPPort(){return nServerUDPPort;}
	static	uchar*	GetUserHash()	{return userhash;}
	// ZZ:UploadSpeedSense -->
	static	uint16	GetMinUpload()	{return minupload;}
	// ZZ:UploadSpeedSense <--
	static	uint16	GetMaxUpload()	{return maxupload;}
	static	bool	IsICHEnabled()	{return ICH;}
	static	bool	AutoServerlist(){return autoserverlist;}
	static	bool	UpdateNotify()	{return updatenotify;}
	static	bool	DoMinToTray()	{return mintotray;}
	static	bool	DoAutoConnect() {return autoconnect;}
	static	void	SetAutoConnect( bool inautoconnect) {autoconnect = inautoconnect;}
	static	bool	AddServersFromServer()		{return addserversfromserver;}
	static	bool	AddServersFromClient()		{return addserversfromclient;}
	static	uint8*	GetMinTrayPTR() {return &mintotray;}
	static	uint16	GetTrafficOMeterInterval() { return trafficOMeterInterval;}
	static	void	SetTrafficOMeterInterval(uint16 in) { trafficOMeterInterval=in;}
	static	uint16	GetStatsInterval() { return statsInterval;}
	static	void	SetStatsInterval(uint16 in) { statsInterval=in;}
	static	void	Add2TotalDownloaded(uint64 in) {totalDownloadedBytes+=in;}
	static	void	Add2TotalUploaded(uint64 in) {totalUploadedBytes+=in;}

	// -khaos--+++> Many, many, many, many methods.
	static	void	SaveStats(int bBackUp = 0);
	static	void	SetRecordStructMembers();
	static	void	SaveCompletedDownloadsStat();
	static	bool	LoadStats(int loadBackUp = 0);
	static	void	ResetCumulativeStatistics();

	//		Functions from base code that update original cumulative stats, now obsolete. (KHAOS)
	//void	Add2TotalDownloaded(uint64 in) {totalDownloadedBytes+=in;}
	//void	Add2TotalUploaded(uint64 in) {totalUploadedBytes+=in;}
	//		End functions from base code.

	//		Add to, increment and replace functions.  They're all named Add2 for the sake of some kind of naming
	//		convention.
	static	void	Add2DownCompletedFiles()			{ cumDownCompletedFiles++; }
	static	void	SetConnMaxAvgDownRate(float in)		{ cumConnMaxAvgDownRate = in; }
	static	void	SetConnMaxDownRate(float in)		{ cumConnMaxDownRate = in; }
	static	void	SetConnAvgUpRate(float in)			{ cumConnAvgUpRate = in; }
	static	void	SetConnMaxAvgUpRate(float in)		{ cumConnMaxAvgUpRate = in; }
	static	void	SetConnMaxUpRate(float in)			{ cumConnMaxUpRate = in; }
	static	void	SetConnPeakConnections(int in)		{ cumConnPeakConnections = in; }
	static	void	SetUpAvgTime(int in)				{ cumUpAvgTime = in; }
	static	void	Add2DownSAvgTime(int in)			{ sesDownAvgTime += in; }
	static	void	SetDownCAvgTime(int in)				{ cumDownAvgTime = in; }
	static	void	Add2ConnTransferTime(int in)		{ cumConnTransferTime += in; }
	static	void	Add2ConnDownloadTime(int in)		{ cumConnDownloadTime += in; }
	static	void	Add2ConnUploadTime(int in)			{ cumConnUploadTime += in; }
	static	void	Add2DownSessionCompletedFiles()		{ sesDownCompletedFiles++; }
	static	void	Add2SessionTransferData				( uint8 uClientID , uint16 uClientPort , BOOL bFromPF, BOOL bUpDown, uint32 bytes, bool sentToFriend = false);
	static	void	Add2DownSuccessfulSessions()		{ sesDownSuccessfulSessions++;
														  cumDownSuccessfulSessions++; }
	static	void	Add2DownFailedSessions()			{ sesDownFailedSessions++;
														  cumDownFailedSessions++; }
	static	void	Add2LostFromCorruption(uint64 in)	{ sesLostFromCorruption += in;}
	static	void	Add2SavedFromCompression(uint64 in) { sesSavedFromCompression += in;}
	static	void	Add2SessionPartsSavedByICH(int in)	{ sesPartsSavedByICH += in;}

	//		Functions that return stats stuff...
	//		Saved stats for cumulative downline overhead
	static	uint64	GetDownOverheadTotal()			{ return cumDownOverheadTotal;}
	static	uint64	GetDownOverheadFileReq()		{ return cumDownOverheadFileReq;}
	static	uint64	GetDownOverheadSrcEx()			{ return cumDownOverheadSrcEx;}
	static	uint64	GetDownOverheadServer()			{ return cumDownOverheadServer;}
	static	uint64	GetDownOverheadKad()			{ return cumDownOverheadKad;}
	static	uint64	GetDownOverheadTotalPackets()	{ return cumDownOverheadTotalPackets;}
	static	uint64	GetDownOverheadFileReqPackets() { return cumDownOverheadFileReqPackets;}
	static	uint64	GetDownOverheadSrcExPackets()	{ return cumDownOverheadSrcExPackets;}
	static	uint64	GetDownOverheadServerPackets()	{ return cumDownOverheadServerPackets;}
	static	uint64	GetDownOverheadKadPackets()		{ return cumDownOverheadKadPackets;}

	//		Saved stats for cumulative upline overhead
	static	uint64	GetUpOverheadTotal()			{ return cumUpOverheadTotal;}
	static	uint64	GetUpOverheadFileReq()			{ return cumUpOverheadFileReq;}
	static	uint64	GetUpOverheadSrcEx()			{ return cumUpOverheadSrcEx;}
	static	uint64	GetUpOverheadServer()			{ return cumUpOverheadServer;}
	static	uint64	GetUpOverheadKad()				{ return cumUpOverheadKad;}
	static	uint64	GetUpOverheadTotalPackets()		{ return cumUpOverheadTotalPackets;}
	static	uint64	GetUpOverheadFileReqPackets()	{ return cumUpOverheadFileReqPackets;}
	static	uint64	GetUpOverheadSrcExPackets()		{ return cumUpOverheadSrcExPackets;}
	static	uint64	GetUpOverheadServerPackets()	{ return cumUpOverheadServerPackets;}
	static	uint64	GetUpOverheadKadPackets()		{ return cumUpOverheadKadPackets;}

	//		Saved stats for cumulative upline data
	static	uint32	GetUpSuccessfulSessions()		{ return cumUpSuccessfulSessions;}
	static	uint32	GetUpFailedSessions()			{ return cumUpFailedSessions;}
	static	uint32	GetUpAvgTime()					{ return cumUpAvgTime;}

	//		Saved stats for cumulative downline data
	static	uint32	GetDownCompletedFiles()			{ return cumDownCompletedFiles;}
	static	uint32	GetDownC_SuccessfulSessions()	{ return cumDownSuccessfulSessions;}
	static	uint32	GetDownC_FailedSessions()		{ return cumDownFailedSessions;}
	static	uint32	GetDownC_AvgTime()				{ return cumDownAvgTime;}
	//		Session download stats
	static	uint32	GetDownSessionCompletedFiles()	{ return sesDownCompletedFiles;}
	static	uint32	GetDownS_SuccessfulSessions()	{ return sesDownSuccessfulSessions;}
	static	uint32	GetDownS_FailedSessions()		{ return sesDownFailedSessions;}
	static	uint32	GetDownS_AvgTime()				{ return GetDownS_SuccessfulSessions()?sesDownAvgTime/GetDownS_SuccessfulSessions():0;}

	//		Saved stats for corruption/compression
	static	uint64	GetCumLostFromCorruption()			{ return cumLostFromCorruption;}
	static	uint64	GetCumSavedFromCompression()		{ return cumSavedFromCompression;}
	static	uint64	GetSesLostFromCorruption()			{ return sesLostFromCorruption;}
	static	uint64	GetSesSavedFromCompression()		{ return sesSavedFromCompression;}
	static	uint32	GetCumPartsSavedByICH()				{ return cumPartsSavedByICH;}
	static	uint32	GetSesPartsSavedByICH()				{ return sesPartsSavedByICH;}

	// Cumulative client breakdown stats for sent bytes
	static	uint64	GetUpTotalClientData()			{ return   GetCumUpData_EDONKEY()
															  + GetCumUpData_EDONKEYHYBRID()
															  + GetCumUpData_EMULE()
															  + GetCumUpData_MLDONKEY()
															 + GetCumUpData_AMULE()
															 + GetCumUpData_EMULECOMPAT()
															 + GetCumUpData_SHAREAZA(); }
	static	uint64	GetCumUpData_EDONKEY()			{ return (cumUpData_EDONKEY +		sesUpData_EDONKEY );}
	static	uint64	GetCumUpData_EDONKEYHYBRID()	{ return (cumUpData_EDONKEYHYBRID +	sesUpData_EDONKEYHYBRID );}
	static	uint64	GetCumUpData_EMULE()			{ return (cumUpData_EMULE +			sesUpData_EMULE );}
	static	uint64	GetCumUpData_MLDONKEY()			{ return (cumUpData_MLDONKEY +		sesUpData_MLDONKEY );}
	static	uint64	GetCumUpData_AMULE()			{ return (cumUpData_AMULE +			sesUpData_AMULE );}
	static	uint64	GetCumUpData_EMULECOMPAT()		{ return (cumUpData_EMULECOMPAT +	sesUpData_EMULECOMPAT );}
	static	uint64	GetCumUpData_SHAREAZA()			{ return (cumUpData_SHAREAZA +			sesUpData_SHAREAZA );}
	// Session client breakdown stats for sent bytes
	static	uint64	GetUpSessionClientData()		{ return   sesUpData_EDONKEY 
															  +	sesUpData_EDONKEYHYBRID 
															  + sesUpData_EMULE 
															  +	sesUpData_MLDONKEY 
															 + sesUpData_AMULE
															 + sesUpData_EMULECOMPAT
															 + sesUpData_SHAREAZA; }
	static	uint64	GetUpData_EDONKEY()				{ return sesUpData_EDONKEY;}
	static	uint64	GetUpData_EDONKEYHYBRID()		{ return sesUpData_EDONKEYHYBRID;}
	static	uint64	GetUpData_EMULE()				{ return sesUpData_EMULE;}
	static	uint64	GetUpData_MLDONKEY()			{ return sesUpData_MLDONKEY;}
	static	uint64	GetUpData_AMULE()				{ return sesUpData_AMULE;}
	static	uint64	GetUpData_EMULECOMPAT()			{ return sesUpData_EMULECOMPAT;}
	static	uint64	GetUpData_SHAREAZA()				{ return sesUpData_SHAREAZA;}

	// Cumulative port breakdown stats for sent bytes...
	static	uint64	GetUpTotalPortData()			{ return (GetCumUpDataPort_4662() +			GetCumUpDataPort_OTHER() );}
	static	uint64	GetCumUpDataPort_4662()			{ return (cumUpDataPort_4662 +		sesUpDataPort_4662 );}
	static	uint64	GetCumUpDataPort_OTHER()		{ return (cumUpDataPort_OTHER +		sesUpDataPort_OTHER );}
	// Session port breakdown stats for sent bytes...
	static	uint64	GetUpSessionPortData()			{ return (sesUpDataPort_4662 +		sesUpDataPort_OTHER );}
	static	uint64	GetUpDataPort_4662()			{ return sesUpDataPort_4662;}
	static	uint64	GetUpDataPort_OTHER()			{ return sesUpDataPort_OTHER;}

	// Cumulative DS breakdown stats for sent bytes...
	static	uint64	GetUpTotalDataFile()			{ return (GetCumUpData_File() +				GetCumUpData_Partfile() );}
	static	uint64	GetCumUpData_File()				{ return (cumUpData_File +			sesUpData_File );}
	static	uint64	GetCumUpData_Partfile()			{ return (sesUpData_Partfile +		sesUpData_Partfile );}
	// Session DS breakdown stats for sent bytes...
	static	uint64	GetUpSessionDataFile()			{ return (sesUpData_File +			sesUpData_Partfile );}
	static	uint64	GetUpData_File()				{ return sesUpData_File;}
	static	uint64	GetUpData_Partfile()			{ return sesUpData_Partfile;}

	// Cumulative client breakdown stats for received bytes
	static	uint64	GetDownTotalClientData()		{ return   GetCumDownData_EDONKEY() 
															  + GetCumDownData_EDONKEYHYBRID() 
															  + GetCumDownData_EMULE() 
															  +	GetCumDownData_MLDONKEY() 
															 + GetCumDownData_AMULE()
															 + GetCumDownData_EMULECOMPAT()
															 + GetCumDownData_SHAREAZA()
// WebCache ////////////////////////////////////////////////////////////////////////////////////
															 + GetCumDownData_URL()
															  + GetCumDownData_WEBCACHE(); } // jp webcache statistics
	static	uint64	GetCumDownData_EDONKEY()		{ return (cumDownData_EDONKEY +			sesDownData_EDONKEY);}
	static	uint64	GetCumDownData_EDONKEYHYBRID()	{ return (cumDownData_EDONKEYHYBRID +	sesDownData_EDONKEYHYBRID);}
	static	uint64	GetCumDownData_EMULE()			{ return (cumDownData_EMULE +			sesDownData_EMULE);}
	static	uint64	GetCumDownData_MLDONKEY()		{ return (cumDownData_MLDONKEY +			sesDownData_MLDONKEY);}
	static	uint64	GetCumDownData_AMULE()			{ return (cumDownData_AMULE +			sesDownData_AMULE);}
	static	uint64	GetCumDownData_EMULECOMPAT()	{ return (cumDownData_EMULECOMPAT +		sesDownData_EMULECOMPAT);}
	static	uint64	GetCumDownData_SHAREAZA()		{ return (cumDownData_SHAREAZA +			sesDownData_SHAREAZA );}
	static	uint64	GetCumDownData_URL()			{ return (cumDownData_URL +				sesDownData_URL);}
	// MORPH START - Added by Commander, WebCache 1.2e
	static	uint64	GetCumDownData_WEBCACHE()		{ return (cumDownData_WEBCACHE +		sesDownData_WEBCACHE);} //jp webcache statistics
	// MORPH END - Added by Commander, WebCache 1.2e
	// Session client breakdown stats for received bytes
	static	uint64	GetDownSessionClientData()		{ return   sesDownData_EDONKEY 
															  + sesDownData_EDONKEYHYBRID 
															  + sesDownData_EMULE 
															  +	sesDownData_MLDONKEY 
															 + sesDownData_AMULE
															 + sesDownData_EMULECOMPAT
															 + sesDownData_SHAREAZA
// WebCache ////////////////////////////////////////////////////////////////////////////////////
															  + sesDownData_URL
															  + sesDownData_WEBCACHE; } // jp webcache statistics
	static	uint64	GetDownData_EDONKEY()			{ return sesDownData_EDONKEY;}
	static	uint64	GetDownData_EDONKEYHYBRID()		{ return sesDownData_EDONKEYHYBRID;}
	static	uint64	GetDownData_EMULE()				{ return sesDownData_EMULE;}
	static	uint64	GetDownData_MLDONKEY()			{ return sesDownData_MLDONKEY;}
	static	uint64	GetDownData_AMULE()				{ return sesDownData_AMULE;}
	static	uint64	GetDownData_EMULECOMPAT()		{ return sesDownData_EMULECOMPAT;}
	static	uint64	GetDownData_SHAREAZA()			{ return sesDownData_SHAREAZA;}
	static	uint64	GetDownData_URL()				{ return sesDownData_URL;}
	// MORPH START - Added by Commander, WebCache 1.2e
	static	uint64	GetDownData_WEBCACHE()			{ return sesDownData_WEBCACHE;} //jp webcache statistics
	// MORPH END - Added by Commander, WebCache 1.2e

	// Cumulative port breakdown stats for received bytes...
	static	uint64	GetDownTotalPortData()			{ return (GetCumDownDataPort_4662() +			GetCumDownDataPort_OTHER() );}
	static	uint64	GetCumDownDataPort_4662()		{ return (cumDownDataPort_4662 +			sesDownDataPort_4662 );}
	static	uint64	GetCumDownDataPort_OTHER()		{ return (cumDownDataPort_OTHER +		sesDownDataPort_OTHER );}
	// Session port breakdown stats for received bytes...
	static	uint64	GetDownSessionDataPort()		{ return (sesDownDataPort_4662 +			sesDownDataPort_OTHER );}
	static	uint64	GetDownDataPort_4662()			{ return sesDownDataPort_4662;}
	static	uint64	GetDownDataPort_OTHER()			{ return sesDownDataPort_OTHER;}

	//		Saved stats for cumulative connection data
	static	float	GetConnAvgDownRate()			{ return cumConnAvgDownRate;}
	static	float	GetConnMaxAvgDownRate()			{ return cumConnMaxAvgDownRate;}
	static	float	GetConnMaxDownRate()			{ return cumConnMaxDownRate;}
	static	float	GetConnAvgUpRate()				{ return cumConnAvgUpRate;}
	static	float	GetConnMaxAvgUpRate()			{ return cumConnMaxAvgUpRate;}
	static	float	GetConnMaxUpRate()				{ return cumConnMaxUpRate;}
	static	uint64	GetConnRunTime()				{ return cumConnRunTime;}
	static	uint32	GetConnNumReconnects()			{ return cumConnNumReconnects;}
	static	uint32	GetConnAvgConnections()			{ return cumConnAvgConnections;}
	static	uint32	GetConnMaxConnLimitReached()	{ return cumConnMaxConnLimitReached;}
	static	uint32	GetConnPeakConnections()		{ return cumConnPeakConnections;}
	static	uint32	GetConnTransferTime()			{ return cumConnTransferTime;}
	static	uint32	GetConnDownloadTime()			{ return cumConnDownloadTime;}
	static	uint32	GetConnUploadTime()				{ return cumConnUploadTime;}
	static	uint32	GetConnServerDuration()			{ return cumConnServerDuration;}

	//		Saved records for servers / network
	static	uint32	GetSrvrsMostWorkingServers()	{ return cumSrvrsMostWorkingServers;}
	static	uint32	GetSrvrsMostUsersOnline()		{ return cumSrvrsMostUsersOnline;}
	static	uint32	GetSrvrsMostFilesAvail()		{ return cumSrvrsMostFilesAvail;}

	//		Saved records for shared files
	static	uint32	GetSharedMostFilesShared()		{ return cumSharedMostFilesShared;}
	static	uint64	GetSharedLargestShareSize()		{ return cumSharedLargestShareSize;}
	static	uint64	GetSharedLargestAvgFileSize()	{ return cumSharedLargestAvgFileSize;}
	static	uint64	GetSharedLargestFileSize()		{ return cumSharedLargestFileSize;}

	//		Get the long date/time when the stats were last reset
	static	__int64 GetStatsLastResetLng()			{ return stat_datetimeLastReset;}
	static	CString GetStatsLastResetStr(bool formatLong = true);
	static	UINT	GetStatsSaveInterval()			{ return statsSaveInterval; }

	//		Get and Set our new preferences
	static	void	SetStatsMax(uint16 in)						{ statsMax = in; }
	static	void	SetStatsConnectionsGraphRatio(uint8 in)		{ statsConnectionsGraphRatio = in; }
	static	uint8	GetStatsConnectionsGraphRatio()				{ return statsConnectionsGraphRatio; }
	static	void	SetExpandedTreeItems(CString in)			{ _stprintf(statsExpandedTreeItems,_T("%s"),in); }
	static	CString GetExpandedTreeItems()						{ return statsExpandedTreeItems; }
	// <-----khaos- End Statistics Methods

	//		Original Statistics Functions
	static	uint64	GetTotalDownloaded()		{return totalDownloadedBytes;}
	static	uint64	GetTotalUploaded()			{return totalUploadedBytes;}
	//		End Original Statistics Functions
	static	bool	IsErrorBeepEnabled()		{return beepOnError;}
	static	bool	IsConfirmExitEnabled()		{return confirmExit;}
	static	bool	UseSplashScreen()			{return splashscreen;}
	static  bool	UseStartupSound()			{return startupsound;}//Commander - Added: Enable/Disable Startupsound
	static  bool	UseSideBanner()			    {return sidebanner;}//Commander - Added: Side Banner	
	static	bool	FilterLANIPs()				{return filterLANIPs;}
	static	bool	GetAllowLocalHostIP()		{return m_bAllocLocalHostIP;}
	static	bool	IsOnlineSignatureEnabled()	{return onlineSig;}
	static	int		GetMaxGraphUploadRate()		{return maxGraphUploadRate;}
	static	int		GetMaxGraphDownloadRate()		{return maxGraphDownloadRate;}
	static	void	SetMaxGraphUploadRate(int in)	{maxGraphUploadRate	=(in)?in:16;}
	static	void	SetMaxGraphDownloadRate(int in) {maxGraphDownloadRate=(in)?in:96;}

	static	uint16	GetMaxDownload();
	static	uint64	GetMaxDownloadInBytesPerSec(bool dynamic = false);
	static	uint16	GetMaxConnections()			{return maxconnections;}
	static	uint16	GetMaxHalfConnections()		{return maxhalfconnections;}
	static	uint16	GetMaxSourcePerFile()		{return maxsourceperfile;}
	static	uint16	GetMaxSourcePerFileSoft();
	static	uint16	GetMaxSourcePerFileUDP();
	static	uint16	GetDeadserverRetries()		{return deadserverretries;}
	static	DWORD	GetServerKeepAliveTimeout() {return m_dwServerKeepAliveTimeout;}

	static	int		GetColumnWidth (Table t, int index);
	static	BOOL	GetColumnHidden(Table t, int index);
	static	int		GetColumnOrder (Table t, int index);
	static	void	SetColumnWidth (Table t, int index, int width);
	static	void	SetColumnHidden(Table t, int index, BOOL bHidden);
	static	void	SetColumnOrder (Table t, INT *piOrder);

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	//MORPH START - Changed by SiRoB, SLUGFILLER: multiSort
	static	int		GetColumnSortItem (Table t, int column = 0);
	static	bool	GetColumnSortAscending (Table t, int column = 0);
	static	int		GetColumnSortCount(Table t);
	//MORPH END   - Changed by SiRoB, multiSort
	static	void	SetColumnSortItem (Table t, int sortItem);
	static	void	SetColumnSortAscending (Table t, bool sortAscending);

	static	WORD	GetLanguageID();
	static	void	SetLanguageID(WORD lid);
	static	void	GetLanguages(CWordArray& aLanguageIDs);
	static	void	SetLanguage();
	static	const CString& GetLangDir()					{return m_strLangDir;}
	static	bool	IsLanguageSupported(LANGID lidSelected, bool bUpdateBefore);
	static	CString GetLangDLLNameByID(LANGID lidSelected);
	static	void	InitThreadLocale();
	static	void	SetRtlLocale(LCID lcid);
	static	CString GetHtmlCharset();

	static	uint8	IsDoubleClickEnabled()				{return transferDoubleclick;}
	static	EViewSharedFilesAccess CanSeeShares(void) {return m_iSeeShares;}
	static	uint8	GetToolTipDelay(void)				{return m_iToolDelayTime;}
	static	uint8	IsBringToFront()					{return bringtoforeground;}

	static	uint8	GetSplitterbarPosition()			{return splitterbarPosition;}
	static	void	SetSplitterbarPosition(uint8 pos)	{splitterbarPosition=pos;}
	static	uint8	GetTransferWnd2()					{return m_uTransferWnd2;}
	static	void	SetTransferWnd2(uint8 uWnd2)		{m_uTransferWnd2 = uWnd2;}
	//MORPH START - Added by SiRoB, Splitting Bar [O²]
	static	uint8   GetSplitterbarPositionStat()	{return splitterbarPositionStat;}
	static	void	SetSplitterbarPositionStat(uint8 pos) {splitterbarPositionStat=pos;}
	static	uint8   GetSplitterbarPositionStat_HL()	{return splitterbarPositionStat_HL;}
	static	void	SetSplitterbarPositionStat_HL(uint8 pos) {splitterbarPositionStat_HL=pos;}
	static	uint8   GetSplitterbarPositionStat_HR()	{return splitterbarPositionStat_HR;}
	static	void	SetSplitterbarPositionStat_HR(uint8 pos) {splitterbarPositionStat_HR=pos;}
	static	uint16   GetSplitterbarPositionFriend()	{return splitterbarPositionFriend;}
	static	void	SetSplitterbarPositionFriend(uint16 pos) {splitterbarPositionFriend=pos;}
	static	uint16  GetSplitterbarPositionIRC()	{return splitterbarPositionIRC;}
	static	void	SetSplitterbarPositionIRC(uint16 pos) {splitterbarPositionIRC=pos;}
	//MORPH END   - Added by SiRoB, Splitting Bar [O²]
	// -khaos--+++> Changed datatype to avoid overflows
	static	uint16	GetStatsMax()						{return statsMax;}
	// <-----khaos-
	static	uint8	UseFlatBar()						{return (depth3D==0);}
	static	int		GetStraightWindowStyles()			{return m_iStraightWindowStyles;}
	static	LPCTSTR GetSkinProfile()					{return m_szSkinProfile;}
	static	CString GetSkinProfileDir()					{return m_szSkinProfileDir;}
	static	void	SetSkinProfile(LPCTSTR pszProfile)	{_sntprintf(m_szSkinProfile, ARRSIZE(m_szSkinProfile), _T("%s"), pszProfile); }
	static	void	SetSkinProfileDir(LPCTSTR pszDir)	{_sntprintf(m_szSkinProfileDir, ARRSIZE(m_szSkinProfileDir), _T("%s"), pszDir); }
	static	uint8	GetStatsAverageMinutes()			{return statsAverageMinutes;}
	static	void	SetStatsAverageMinutes(uint8 in)	{statsAverageMinutes=in;}

	static	bool	GetUseDownloadNotifier()			{return useDownloadNotifier;}
	static	bool	GetUseNewDownloadNotifier()			{return useNewDownloadNotifier;}
	static	bool	GetUseChatNotifier()				{return useChatNotifier;}
	static	bool	GetUseLogNotifier()					{return useLogNotifier;}
	static	bool	GetUseSoundInNotifier()				{return useSoundInNotifier;}
	static	bool	GetNotifierPopsEveryChatMsg()		{return notifierPopsEveryChatMsg;}
	static	bool	GetNotifierPopOnImportantError()	{return notifierImportantError;}
	static	bool	GetNotifierPopOnNewVersion()		{return notifierNewVersion;}
	static	TCHAR*	GetNotifierWavSoundPath()			{return notifierSoundFilePath;}

	static	CString GetIRCNick()						{return m_sircnick;}
	static	void	SetIRCNick( TCHAR in_nick[] )		{ _tcscpy(m_sircnick,in_nick);}
	static	CString GetIRCServer()						{return m_sircserver;}
	static	bool	GetIRCAddTimestamp()				{return m_bircaddtimestamp;}
	static	CString GetIRCChanNameFilter()				{return m_sircchannamefilter;}
	static	bool	GetIRCUseChanFilter()				{return m_bircusechanfilter;}
	static	uint16	GetIRCChannelUserFilter()			{return m_iircchanneluserfilter;}
	static	CString GetIrcPerformString()				{return m_sircperformstring;}
	static	bool	GetIrcUsePerform()					{return m_bircuseperform;}
	static	bool	GetIRCListOnConnect()				{return m_birclistonconnect;}
	static	bool	GetIrcAcceptLinks()					{return m_bircacceptlinks;}
	static	bool	GetIrcAcceptLinksFriends()			{return m_bircacceptlinksfriends;}
	static	bool	GetIrcSoundEvents()					{return m_bircsoundevents;}
	static	bool	GetIrcIgnoreMiscMessage()			{return m_bircignoremiscmessage;}
	static	bool	GetIrcIgnoreJoinMessage()			{return m_bircignorejoinmessage;}
	static	bool	GetIrcIgnorePartMessage()			{return m_bircignorepartmessage;}
	static	bool	GetIrcIgnoreQuitMessage()			{return m_bircignorequitmessage;}
	static	bool	GetIrcIgnoreEmuleProtoAddFriend()	{return m_bircignoreemuleprotoaddfriend;}
	static	bool	GetIrcAllowEmuleProtoAddFriend()	{return m_bircallowemuleprotoaddfriend;}
	static	bool	GetIrcIgnoreEmuleProtoSendLink()	{return m_bircignoreemuleprotosendlink;}
	static	bool	GetIrcHelpChannel()					{return m_birchelpchannel;}
	static	WORD	GetWindowsVersion();
	static	bool	GetStartMinimized()					{return startMinimized;}
	static	void	SetStartMinimized( bool instartMinimized) {startMinimized = instartMinimized;}
	static	bool	GetAutoStart()						{return m_bAutoStart;}
	static	void	SetAutoStart( bool val)				{m_bAutoStart = val;}

	static	bool	GetRestoreLastMainWndDlg()			{return m_bRestoreLastMainWndDlg;}
	static	int		GetLastMainWndDlgID()				{return m_iLastMainWndDlgID;}
	static	void	SetLastMainWndDlgID(int iID)		{m_iLastMainWndDlgID = iID;}

	static	bool	GetRestoreLastLogPane()				{return m_bRestoreLastLogPane;}
	static	int		GetLastLogPaneID()					{return m_iLastLogPaneID;}
	static	void	SetLastLogPaneID(int iID)			{m_iLastLogPaneID = iID;}

	static	bool	GetSmartIdCheck()					{return smartidcheck;}
	static	void	SetSmartIdCheck(bool in_smartidcheck) {smartidcheck = in_smartidcheck;}
	static	uint8	GetSmartIdState()					{return smartidstate;}
	static	void	SetSmartIdState(uint8 in_smartidstate) {smartidstate = in_smartidstate;}
	//MORPH START - Added by SiRoB, XML News [O²]
	static	bool	GetNews()							{return enableNEWS;}
	//MORPH END   - Added by SiRoB, XML News [O²]
	static	bool	GetPreviewPrio()					{return m_bpreviewprio;}
	static	void	SetPreviewPrio(bool in)				{m_bpreviewprio=in;}
	static	bool	GetUpdateQueueList()				{return m_bupdatequeuelist;}
	static	bool	GetManualHighPrio()					{return m_bmanualhighprio;}
	static	bool	TransferFullChunks()				{return m_btransferfullchunks;}
	static	void	SetTransferFullChunks( bool m_bintransferfullchunks )				{m_btransferfullchunks = m_bintransferfullchunks;}
	static	int		StartNextFile()						{return m_istartnextfile;}
	static	bool	ShowOverhead()						{return m_bshowoverhead;}
	static	void	SetNewAutoUp(bool m_bInUAP)			{m_bUAP = m_bInUAP;}
	static	bool	GetNewAutoUp()						{return m_bUAP;}
	static	void	SetNewAutoDown(bool m_bInDAP)		{m_bDAP = m_bInDAP;}
	static	bool	GetNewAutoDown()					{return m_bDAP;}
	static	bool	IsKnownClientListDisabled()			{return m_bDisableKnownClientList;}
	static	bool	IsQueueListDisabled()				{return m_bDisableQueueList;}
	static	bool	IsFirstStart()						{return m_bFirstStart;}
	static	bool	UseCreditSystem()					{return m_bCreditSystem;}
	static	void	SetCreditSystem(bool m_bInCreditSystem)	{m_bCreditSystem = m_bInCreditSystem;}

	static	TCHAR*	GetTxtEditor()						{return TxtEditor;}
	static	CString	GetVideoPlayer()					{if (_tcslen(VideoPlayer)==0) return _T(""); else return CString(VideoPlayer);}

	static	UINT	GetFileBufferSize()					{return m_iFileBufferSize;}
	static	UINT	GetQueueSize()						{return m_iQueueSize;}
	static	int		GetCommitFiles()					{return m_iCommitFiles;}

	// Barry
	static	uint16	Get3DDepth() { return depth3D;}
	static	bool	AutoTakeED2KLinks() {return autotakeed2klinks;}
	static	bool	AddNewFilesPaused() {return addnewfilespaused;}

	static	bool	TransferlistRemainSortStyle()	{ return m_bTransflstRemain;}
	static	void	TransferlistRemainSortStyle(bool in)	{ m_bTransflstRemain=in;}

	static	void	SetStatsColor(int index,DWORD value) {statcolors[index]=value;}
	static	DWORD	GetStatsColor(int index)			{return statcolors[index];}
	static	void	SetMaxConsPerFive(int in)			{MaxConperFive=in;}
	static	LPLOGFONT GetHyperTextLogFont()				{return &m_lfHyperText;}
	static	void	SetHyperTextFont(LPLOGFONT plf)		{m_lfHyperText = *plf;}
	static	LPLOGFONT GetLogFont()						{return &m_lfLogText;}
	static	void	SetLogFont(LPLOGFONT plf)			{m_lfLogText = *plf;}
	static	COLORREF GetLogErrorColor()					{return m_crLogError;}
	static	COLORREF GetLogWarningColor()				{return m_crLogWarning;}
	static	COLORREF GetLogSuccessColor()				{return m_crLogSuccess;}
	//MORPH START - Added by SiRoB, Upload Splitting Class
	static	COLORREF GetLogUSCColor()					{return m_crLogUSC;}
	//MORPH END   - Added by SiRoB, Upload Splitting Class

	static	uint16	GetMaxConperFive()					{return MaxConperFive;}
	static	uint16	GetDefaultMaxConperFive();

	static	void	ResetStatsColor(int index);
	static	bool	IsSafeServerConnectEnabled()		{return safeServerConnect;}
	static	void	SetSafeServerConnectEnabled(bool in){safeServerConnect=in;}
	static	bool	IsMoviePreviewBackup()				{return moviePreviewBackup;}
	static	int		GetPreviewSmallBlocks()				{return m_iPreviewSmallBlocks;}
	static	int		GetPreviewCopiedArchives()			{return m_iPreviewCopiedArchives;}
	static	int		GetInspectAllFileTypes()			{return m_iInspectAllFileTypes;}
	static	int		GetExtractMetaData()				{return m_iExtractMetaData;}
	static	bool	GetAdjustNTFSDaylightFileTime()		{return m_bAdjustNTFSDaylightFileTime;}

	// itsonlyme: hostnameSource
	static	TCHAR*	GetYourHostname()					{return yourHostname;}
	static	void	SetYourHostname(CString in)			{_stprintf(yourHostname,_T("%s"),in);}
	// itsonlyme: hostnameSource
	static	bool	IsCheckDiskspaceEnabled()			{return checkDiskspace != 0;}	// SLUGFILLER: checkDiskspace
	static	UINT	GetMinFreeDiskSpace()				{return m_uMinFreeDiskSpace;}
	static	bool	GetSparsePartFiles()				{return m_bSparsePartFiles;}
	static	void	SetSparsePartFiles(bool bEnable)	{m_bSparsePartFiles = bEnable;}

	static	void	SetMaxUpload(uint16 in);
	static	void	SetMaxDownload(uint16 in);

	static	WINDOWPLACEMENT GetEmuleWindowPlacement() {return EmuleWindowPlacement; }
	static	void	SetWindowLayout(WINDOWPLACEMENT in) {EmuleWindowPlacement=in; }

	static	uint8	AutoConnectStaticOnly() {return autoconnectstaticonly;}
	static	uint8	GetUpdateDays()			{return versioncheckdays;}
	static	uint32	GetLastVC()				{return versioncheckLastAutomatic;}
	static	void	UpdateLastVC();
	//MORPH START - Added by SiRoB, New Version check
	static	uint32	GetLastMVC()				{return mversioncheckLastAutomatic;}
	static	void	UpdateLastMVC();
	//MORPH END   - Added by SiRoB, New Version check
	static	int		GetIPFilterLevel()		{ return filterlevel;}
	static	CString GetMessageFilter()		{ return CString(messageFilter);}
	static	const CString& GetCommentFilter(){ return commentFilter; }
	static	CString GetFilenameCleanups()	{ return CString(filenameCleanups);}

	static	bool	ShowRatesOnTitle()		{ return showRatesInTitle;}
	static	TCHAR*	GetNotifierConfiguration()	  {return notifierConfiguration;}; //<<-- enkeyDEV(kei-kun) -skinnable notifier-
	static	void	SetNotifierConfiguration(CString configFullPath) {_stprintf(notifierConfiguration,_T("%s"),configFullPath); } //<<-- enkeyDEV(kei-kun) -skinnable notifier-
	static	void	LoadCats();
	static	CString GetDateTimeFormat()		{ return CString(datetimeformat);}
	static	CString GetDateTimeFormat4Log() { return CString(datetimeformat4log);}
	// Download Categories (Ornis)
	static	int		AddCat(Category_Struct* cat) { catMap.Add(cat); return catMap.GetCount()-1;}
	static	bool	MoveCat(UINT from, UINT to);
	static	void	RemoveCat(int index);
	static	int		GetCatCount()			{ return catMap.GetCount();}
	/*// khaos::kmod+ Obsolete 
	static  bool	SetCatFilter(int index, int filter);
	static  int		GetCatFilter(int index);
	static	bool	GetCatFilterNeg(int index);
	static	void	SetCatFilterNeg(int index, bool val);
	*/
	static	Category_Struct* GetCategory(int index) { if (index>=0 && index<catMap.GetCount()) return catMap.GetAt(index); else return NULL;}
	static	TCHAR*	GetCatPath(uint8 index) { return catMap.GetAt(index)->incomingpath;}
	static	DWORD	GetCatColor(uint8 index)	{ if (index>=0 && index<catMap.GetCount()) return catMap.GetAt(index)->color; else return 0;}

	static	bool	ShowRatingIndicator()	{ return indicateratings;}
	// khaos::kmod+ Obsolete static	int		GetAllcatType()			{ return allcatType;}
	// khaos::kmod+ Obsolete static	void	SetAllcatType(int in)	{ allcatType=in; }

	static	bool	WatchClipboard4ED2KLinks()	{ return watchclipboard;}
	static	bool	GetRemoveToBin()			{ return m_bRemove2bin;}
	static	bool	FilterServerByIP()		{ return filterserverbyip;}

	static	bool	GetLog2Disk()							{ return log2disk;}
	static	bool	GetDebug2Disk()							{ return m_bVerbose && debug2disk;}
	static	int		GetMaxLogBuff()							{ return iMaxLogBuff;}
	static	UINT	GetMaxLogFileSize()						{ return uMaxLogFileSize; }

	// WebServer
	static	uint16	GetWSPort()								{ return m_nWebPort; }
	static	void	SetWSPort(uint16 uPort)					{ m_nWebPort=uPort; }
	static	CString GetWSPass()								{ return CString(m_sWebPassword); }
	static	void	SetWSPass(CString strNewPass);
	static	bool	GetWSIsEnabled()						{ return m_bWebEnabled; }
	static	void	SetWSIsEnabled(bool bEnable)			{ m_bWebEnabled=bEnable; }
	static	bool	GetWebUseGzip()							{ return m_bWebUseGzip; }
	static	void	SetWebUseGzip(bool bUse)				{ m_bWebUseGzip=bUse; }
	static	int		GetWebPageRefresh()						{ return m_nWebPageRefresh; }
	static	void	SetWebPageRefresh(int nRefresh)			{ m_nWebPageRefresh=nRefresh; }
	static	bool	GetWSIsLowUserEnabled()					{ return m_bWebLowEnabled; }
	static	void	SetWSIsLowUserEnabled(bool in)			{ m_bWebLowEnabled=in; }
	static	CString GetWSLowPass()							{ return CString(m_sWebLowPassword); }
	static	int		GetWebTimeoutMins()						{ return m_iWebTimeoutMins;}
	static	void	SetWSLowPass(CString strNewPass);

	static	void	SetMaxSourcesPerFile(uint16 in)			{ maxsourceperfile=in;}
	static	void	SetMaxConnections(uint16 in)			{ maxconnections =in;}
	static	void	SetMaxHalfConnections(uint16 in)		{ maxhalfconnections =in;}
	static	bool	IsSchedulerEnabled()					{ return scheduler;}
	static	void	SetSchedulerEnabled(bool in)			{ scheduler=in;}
	static	bool	GetDontCompressAvi()					{ return dontcompressavi;}

	static	bool	MsgOnlyFriends()						{ return msgonlyfriends;}
	static	bool	MsgOnlySecure()							{ return msgsecure;}
	static	uint16	GetMsgSessionsMax()						{ return maxmsgsessions;}
	static	bool	IsSecureIdentEnabled()					{ return m_bUseSecureIdent;} // use clientcredits->CryptoAvailable() to check if crypting is really available and not this function
	static	bool	IsAdvSpamfilterEnabled()				{ return m_bAdvancedSpamfilter;}
	static	CString GetTemplate()							{ return CString(m_sTemplateFile);}
	static	void	SetTemplate(CString in)					{ _stprintf(m_sTemplateFile,_T("%s"),in);}
	static	bool	GetNetworkKademlia()					{ return networkkademlia;}
	static	void	SetNetworkKademlia(bool val);
	static	bool	GetNetworkED2K()						{ return networked2k;}
	static	void	SetNetworkED2K(bool val)				{ networked2k = val;}

	// mobileMule
	static	CString GetMMPass()								{ return CString(m_sMMPassword); }
	static	void	SetMMPass(CString strNewPass);
	static	bool	IsMMServerEnabled()						{ return m_bMMEnabled; }
	static	void	SetMMIsEnabled(bool bEnable)			{ m_bMMEnabled=bEnable; }
	static	uint16	GetMMPort()								{ return m_nMMPort; }
	static	void	SetMMPort(uint16 uPort)					{ m_nMMPort=uPort; }

	// deadlake PROXYSUPPORT
	static	const ProxySettings& GetProxy()					{ return proxy; }
	static	void	SetProxySettings(const ProxySettings& proxysettings) { proxy = proxysettings; }

	//MORPH START - Changed by SiRoB, [MoNKi: -UPnPNAT Support-] [MoNKi: -Random Ports-]
	/*
	static	uint16	GetListenPort()							{ if (m_UseProxyListenPort) return ListenPort; else return port; }
	*/
	static	uint16	GetListenPort()							{ if (m_UseProxyListenPort) return ListenPort; else return GetPort(); }
	//MORPH END   - Changed by SiRoB, [MoNKi: -UPnPNAT Support-] [MoNKi: -Random Ports-]

	static	void	SetListenPort(uint16 uPort)				{ ListenPort = uPort; m_UseProxyListenPort = true; }
	static	void	ResetListenPort()						{ ListenPort = 0; m_UseProxyListenPort = false; }
	static	void	SetUseProxy(bool in)					{ proxy.UseProxy=in;}
	static	bool	GetShowProxyErrors()					{ return m_bShowProxyErrors; }
	static	void	SetShowProxyErrors(bool bEnable)		{ m_bShowProxyErrors = bEnable; }

	static	bool	IsProxyASCWOP()							{ return m_bIsASCWOP;}
	static	void	SetProxyASCWOP(bool in)					{ m_bIsASCWOP=in;}

	static	bool	ShowCatTabInfos()						{ return showCatTabInfos;}
	static	void	ShowCatTabInfos(bool in)				{ showCatTabInfos=in;}

	static	bool	AutoFilenameCleanup()						{ return autofilenamecleanup;}
	static	void	AutoFilenameCleanup(bool in)				{ autofilenamecleanup=in;}
	static	void	SetFilenameCleanups(CString in)				{ _stprintf(filenameCleanups,_T("%s"),in);}

	static	bool	GetResumeSameCat()							{ return resumeSameCat;}
	static	bool	IsGraphRecreateDisabled()					{ return dontRecreateGraphs;}
	static	bool	IsExtControlsEnabled()						{ return m_bExtControls;}
	static	void	SetExtControls(bool in)						{ m_bExtControls=in;}
	static	bool	GetRemoveFinishedDownloads()				{ return m_bRemoveFinishedDownloads;}

	static	uint16	GetMaxChatHistoryLines()					{ return m_iMaxChatHistory;}
	static	bool	GetUseAutocompletion()						{ return m_bUseAutocompl;}
	static	bool	GetUseDwlPercentage()						{ return m_bShowDwlPercentage;}
	static	void	SetUseDwlPercentage(bool in)				{ m_bShowDwlPercentage=in;}
	static	bool	GetShowActiveDownloadsBold()				{ return m_bShowActiveDownloadsBold; }        

    //Commander - Added: Client Percentage - Start
	static	bool	GetUseClientPercentage()					{ return m_bShowClientPercentage;}
	static	void	SetUseClientPercentage(bool in)				{ m_bShowClientPercentage=in;}
	//Commander - Added: Client Percentage - End

	//MORPH START - Added by Commander, ClientQueueProgressBar
	static bool ShowClientQueueProgressBar()					{ return m_bClientQueueProgressBar;}
    //MORPH END - Added by Commander, ClientQueueProgressBar

    //MORPH START - Added by Commander, Show WC stats
	static bool CountWCSessionStats()							{ return m_bCountWCSessionStats;}
    //MORPH END - Added by Commander, Show WC stats

	//MORPH START - Added by Commander, FolderIcons
	static bool ShowFolderIcons()								{ return m_bShowFolderIcons;}
	//MORPH END - Added by Commander, FolderIcons

	//Toolbar
	static	CString GetToolbarSettings()						{ return m_sToolbarSettings; }
	static	void	SetToolbarSettings(CString in)				{ _stprintf(m_sToolbarSettings,_T("%s"),in);}
	static	CString GetToolbarBitmapSettings()					{ return m_sToolbarBitmap; }
	static	void	SetToolbarBitmapSettings(CString path)		{ _stprintf(m_sToolbarBitmap,_T("%s"),path);}
	static	CString GetToolbarBitmapFolderSettings()			{ return m_sToolbarBitmapFolder; }
	static	void	SetToolbarBitmapFolderSettings(CString path){ _stprintf(m_sToolbarBitmapFolder,_T("%s"),path); }
	static	uint8	GetToolbarLabelSettings()					{ return m_nToolbarLabels; }
	static	void	SetToolbarLabelSettings(uint8 settings)		{ m_nToolbarLabels= settings; }

	static	int		GetSearchMethod()							{ return m_iSearchMethod; }
	static	void	SetSearchMethod(int iMethod)				{ m_iSearchMethod = iMethod; }

	// ZZ:UploadSpeedSense -->
	static	bool	IsDynUpEnabled()							{ return m_bDynUpEnabled; }
	static	void	SetDynUpEnabled(bool newValue)				{ m_bDynUpEnabled = newValue; }
	static	int		GetDynUpPingTolerance()						{ return m_iDynUpPingTolerance; }
	static	int		GetDynUpGoingUpDivider()					{ return m_iDynUpGoingUpDivider; }
	static	int		GetDynUpGoingDownDivider()					{ return m_iDynUpGoingDownDivider; }
	static	int		GetDynUpNumberOfPings()						{ return m_iDynUpNumberOfPings; }
    static  bool	IsDynUpUseMillisecondPingTolerance()        { return m_bDynUpUseMillisecondPingTolerance;} // EastShare - Added by TAHO, USS limit
	static  int		GetDynUpPingToleranceMilliseconds()         { return m_iDynUpPingToleranceMilliseconds; } // EastShare - Added by TAHO, USS limit
	static  void	SetDynUpPingToleranceMilliseconds(int in)   { m_iDynUpPingToleranceMilliseconds = in; }
	// ZZ:UploadSpeedSense <--

    static bool     GetA4AFSaveCpu()                            { return m_bA4AFSaveCpu; } // ZZ:DownloadManager
	// ZZ:UploadSpeedSense <--
	static	CString	GetHomepageBaseURL()						{ return GetHomepageBaseURLForLevel(GetWebMirrorAlertLevel()); }
	static	CString	GetVersionCheckBaseURL();					
	static	void	SetWebMirrorAlertLevel(uint8 newValue)		{ m_nWebMirrorAlertLevel = newValue; }
	static bool	IsDefaultNick(const CString strCheck);
	static	uint8	GetWebMirrorAlertLevel();
	static bool		UseSimpleTimeRemainingComputation()			{ return m_bUseOldTimeRemaining;}

	static	bool	IsRunAsUserEnabled();

	// PeerCache
	static	bool	IsPeerCacheDownloadEnabled()				{ return m_bPeerCacheEnabled; }
	static	uint32	GetPeerCacheLastSearch()					{ return m_uPeerCacheLastSearch; }
	static	bool	WasPeerCacheFound()							{ return m_bPeerCacheWasFound; }
	static	void	SetPeerCacheLastSearch(uint32 dwLastSearch) { m_uPeerCacheLastSearch = dwLastSearch; }
	static	void	SetPeerCacheWasFound(bool bFound)			{ m_bPeerCacheWasFound = bFound; }
	static	uint16	GetPeerCachePort()							{ return m_nPeerCachePort; }
	static	void	SetPeerCachePort(uint16 nPort)				{ m_nPeerCachePort = nPort; }

	// Verbose log options
	static	bool	GetEnableVerboseOptions()			{return m_bEnableVerboseOptions;}
	static	bool	GetVerbose()						{return m_bVerbose;}
	static	bool	GetFullVerbose()					{return m_bVerbose && m_bFullVerbose;}
	static	bool	GetDebugSourceExchange()			{return m_bVerbose && m_bDebugSourceExchange;}
	static	bool	GetLogBannedClients()				{return m_bVerbose && m_bLogBannedClients;}
	static	bool	GetLogRatingDescReceived()			{return m_bVerbose && m_bLogRatingDescReceived;}
	static	bool	GetLogSecureIdent()					{return m_bVerbose && m_bLogSecureIdent;}
	static	bool	GetLogFilteredIPs()					{return m_bVerbose && m_bLogFilteredIPs;}
	static	bool	GetLogFileSaving()					{return m_bVerbose && m_bLogFileSaving;}
    static	bool	GetLogA4AF()    					{return m_bVerbose && m_bLogA4AF;} // ZZ:DownloadManager
	static	bool	GetLogUlDlEvents()					{return m_bVerbose && m_bLogUlDlEvents;} // MORPH - Added by Commander, WebCache 1.2e
	static	bool	GetLogWebCacheEvents()				{return m_bVerbose && m_bLogWebCacheEvents;}//JP log webcache events
	static	bool	GetLogICHEvents()					{return m_bVerbose && m_bLogICHEvents;}//JP log ICH events
	static	bool	GetUseDebugDevice()					{return m_bUseDebugDevice;}
	static	int		GetDebugServerTCPLevel()			{return m_iDebugServerTCPLevel;}
	static	int		GetDebugServerUDPLevel() 			{return m_iDebugServerUDPLevel;}
	static	int		GetDebugServerSourcesLevel()		{return m_iDebugServerSourcesLevel;}
	static	int		GetDebugServerSearchesLevel()		{return m_iDebugServerSearchesLevel;}
	static	int		GetDebugClientTCPLevel()			{return m_iDebugClientTCPLevel;}
	static	int		GetDebugClientUDPLevel()			{return m_iDebugClientUDPLevel;}
	static	int		GetDebugClientKadUDPLevel()			{return m_iDebugClientKadUDPLevel;}
	static	uint8	GetVerboseLogPriority()	{return	m_byLogLevel;} // hard coded now, will of course be selectable later

	// Firewall settings
	static  bool	IsOpenPortsOnStartupEnabled()		{return m_bOpenPortsOnStartUp; }
	//AICH Hash
	static	bool	IsTrustingEveryHash()				{return m_bTrustEveryHash;} // this is a debug option

	static	bool	IsUSSLog() {return m_bDynUpLog;} //MORPH - Added by SiRoB, ZZ Upload system (USS)

	//EastShare START - Added by Pretender, add USS settings in scheduler tab
	static	void	SetDynUpGoingUpDivider(int in) { m_iDynUpGoingUpDivider = in; }
	static	void	SetDynUpGoingDownDivider(int in) { m_iDynUpGoingDownDivider = in; }
	//EastShare END - Added by Pretender, add USS settings in scheduler tab

	//MORPH START - Added by IceCream, Defeat 0-filled Part Senders from Maella	// Maella -Defeat 0-filled Part Senders- (Idea of xrmb)
	static	bool	GetEnableZeroFilledTest() { return enableZeroFilledTest; }
	//MORPH END   - Added by IceCream, Defeat 0-filled Part Senders from Maella

	//MORPH START - Added by IceCream, high process priority
	static	bool	enableHighProcess;
	static	int		GetEnableHighProcess()					{ return enableHighProcess; }
	static	void	SetEnableHighProcess(bool enablehigh);
	//MORPH END   - Added by IceCream, high process priority

	static	bool	GetEnableAntiCreditHack()					{ return enableAntiCreditHack; }//MORPH - Added by IceCream, enable AntiCreditHack

	static	bool GetEnableDownloadInRed()	{ return enableDownloadInRed; } //MORPH - Added by IceCream, show download in red
	static	bool GetEnableAntiLeecher()		{ return enableAntiLeecher; } //MORPH - Added by IceCream, enable Anti-leecher
	
	static	uint8	GetCreditSystem()	{return creditSystemMode;} // EastShare - Added by linekin, creditsystem integration
	static	bool	IsEqualChanceEnable()	{ return m_bEnableEqualChanceForEachFile;}	//Morph - added by AndCycle, Equal Chance For Each File
	static	int  GetKnownMetDays()	{return m_iKnownMetDays;} // EastShare - Added by TAHO, .met file control
	static	bool IsAutoDynUpSwitching()	{return isautodynupswitching;}//MORPH - Added by Yun.SF3, Auto DynUp changing
	static	uint8  GetPowerShareMode()	{return m_iPowershareMode;} //MORPH - Added by SiRoB, Avoid misusing of powersharing
	static	bool	IsPSinternalPrioEnable()	{return m_bPowershareInternalPrio;} //Morph - added by AndCyle, selective PS internal Prio
	//EastShare START - Pretender, TBH-AutoBackup
	static	bool    GetAutoBackup()	{ return autobackup;}
	static	bool    GetAutoBackup2()	{ return autobackup2;}
	static	void    SetAutoBackup(bool in) { autobackup = in;}
	static	void    SetAutoBackup2(bool in) { autobackup2 = in;}
	//EastShare END - Pretender, TBH-AutoBackup
	static	uint16	MaxConnectionsSwitchBorder() {return maxconnectionsswitchborder;}//MORPH - Added by Yun.SF3, Auto DynUp changing
	//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	static	bool	IsSUCDoesWork();
	static	bool	IsSUCEnabled()				{return m_bSUCEnabled;}
	//MORPH START - Added by Yun.SF3, Auto DynUp changing
	static	void	SetSUCEnabled(bool newValue){m_bSUCEnabled = newValue;}
	//MORPH END - Added by Yun.SF3, Auto DynUp changing
	static	bool	IsSUCLog()					{return m_bSUCLog;}
	static	uint16	GetSUCHigh()				{return m_iSUCHigh;}
	static	uint16	GetSUCLow()					{return m_iSUCLow;}
	static	uint16	GetSUCDrift()				{return m_iSUCDrift;}
	static	uint16	GetSUCPitch()				{return m_iSUCPitch;}
	//MORPH END - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	//MORPH START - Added by SiRoB, ZZ Ratio
	static	bool	IsZZRatioDoesWork();
	//MORPH END - Added by SiRoB, ZZ Ratio
	static	void	SetKnownMetDays(int m_iInKnownMetDays)	{m_iKnownMetDays = m_iInKnownMetDays;}	//EastShare - Added by TAHO, .met file control
	static	bool	IsPayBackFirst()					{return m_bPayBackFirst;}	//EastShare - added by AndCycle, Pay Back First
	static	uint8	GetPayBackFirstLimit()				{return m_iPayBackFirstLimit;}	//MOPRH - Added by SiRoB, Pay Back First Tweak
	static	bool	OnlyDownloadCompleteFiles()			{return m_bOnlyDownloadCompleteFiles;} //EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	static	bool	SaveUploadQueueWaitTime()			{return m_bSaveUploadQueueWaitTime;}//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	static	bool	DoRemoveSpareTrickleSlot()			{return !m_bDontRemoveSpareTrickleSlot;}//Morph - added by AndCycle, Dont Remove Spare Trickle Slot
	static	CString	GetUpdateURLFakeList()				{return CString(UpdateURLFakeList);}		//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	static	CString	GetUpdateURLIPFilter()				{return CString(UpdateURLIPFilter);}//MORPH START added by Yun.SF3: Ipfilter.dat update
	static  CString GetUpdateURLIP2Country()			{return CString(UpdateURLIP2Country);}//Commander - Added: IP2Country auto-updating
	static  CString GetUpdateVerURLIP2Country()			{return CString(UpdateVerURLIP2Country);}//Commander - Added: IP2Country auto-updating
	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	static	uint32	GetFakesDatVersion()				{return m_FakesDatVersion;}
	static	void	SetFakesDatVersion(uint32 version)	{m_FakesDatVersion = version;} 
	static	bool	IsUpdateFakeStartupEnabled()		{ return UpdateFakeStartup; }
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating

	//MORPH START added by Yun.SF3: Ipfilter.dat update
	static	uint32	GetIPfilterVersion()				{return m_IPfilterVersion;}
	static	void	SetIpfilterVersion(uint32 version)	{m_IPfilterVersion = version;}
	//MORPH END added by Yun.SF3: Ipfilter.dat update

	static uint32   GetIP2CountryVersion()				{return m_IP2CountryVersion;}//Commander - Added: IP2Country auto-updating
	static void		SetIP2CountryVersion(uint32 version){m_IP2CountryVersion = version;}//Commander - Added: IP2Country auto-updating
	//EastShare - added by AndCycle, IP to Country
	uint8	GetIP2CountryNameMode()	{return m_iIP2CountryNameMode;}
	static bool	IsIP2CountryShowFlag()				{return m_bIP2CountryShowFlag;}
	//EastShare - added by AndCycle, IP to Country
	static	void	SetMinUpload(uint16 in); //MORPH - Added by SiRoB, (SUC) & (USS)
	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	static	void	SetLowIdRetried()	{LowIdRetried--;}
	static	void	ResetLowIdRetried()	{LowIdRetried = LowIdRetries;}
	static	uint8	GetLowIdRetried()	{return LowIdRetried;}
	static	uint8	GetLowIdRetries()	{return LowIdRetries;}
	//MORPH END - Added by SiRoB, SLUGFILLER: lowIdRetry
	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	static	uint8	GetSpreadbarSetStatus()	{return m_iSpreadbarSetStatus;}
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	static	uint8	GetHideOvershares()		{return hideOS;}
	static	uint8	IsSelectiveShareEnabled()	{return selectiveShare;}
	//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS
	static	bool	IsInfiniteQueueEnabled()		{return infiniteQueue;}	//Morph - added by AndCycle, SLUGFILLER: infiniteQueue
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	static	uint8	GetShareOnlyTheNeed()	{return ShareOnlyTheNeed;}
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	static	uint8	GetPowerShareLimit() {return PowerShareLimit;}
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	//MORPH START - Added by SiRoB, Show Permissions
	static	uint8	GetPermissions()	{return permissions;}
	//MORPH END   - Added by SiRoB, Show Permissions
	//EastShare Start - PreferShareAll by AndCycle
	static	bool	ShareAll()			{return shareall;}	// SLUGFILLER: preferShareAll
	//EastShare End - PreferShareAll by AndCycle
	//EastShare - Added by Pretender, Option for ChunkDots
	static	bool	EnableChunkDots()			{return m_bEnableChunkDots;}
	//EastShare - Added by Pretender, Option for ChunkDots
	//EastShare - Added by Pretender, Invisible Mode
	static	bool	InvisibleMode()			{return m_bInvisibleMode;}
	//EastShare - Added by Pretender, Invisible Mode

	static	bool	DateFileNameLog()	{ return m_bDateFileNameLog;}//Morph - added by AndCycle, Date File Name Log

	// Mighty Knife: Community visualization, report hashing files, Log friendlist activities
	static	CString GetCommunityName ()						{ return m_sCommunityName; }
	static	void	SetCommunityName (CString _CommName)	{ _stprintf(m_sCommunityName,_T("%s"),_CommName); }
	static	bool	IsCommunityEnabled()					{ return m_sCommunityName [0] != '\0' ? true : false; }
	static	bool    GetReportHashingFiles ()				{ return m_bReportHashingFiles; }
	static	void	SetReportHashingFiles (bool _b)			{ m_bReportHashingFiles = _b; }
	static	bool    GetLogFriendlistActivities ()			{ return m_bLogFriendlistActivities; }
	static	void	SetLogFriendlistActivities (bool _b)	{ m_bLogFriendlistActivities = _b; }
	// [end] Mighty Knife

	// Mighty Knife: CRC32-Tag - not accessible in preferences dialog !
	static	bool    GetDontAddCRCToFilename ()				{ return m_bDontAddCRCToFilename; }
	static	void	SetDontAddCRCToFilename (bool _b)		{ m_bDontAddCRCToFilename = _b; }
	static	bool    GetCRC32ForceUppercase ()				{ return m_bCRC32ForceUppercase; }
	static	void	SetCRC32ForceUppercase (bool _b)		{ m_bCRC32ForceUppercase = _b; }
	static	bool    GetCRC32ForceAdding ()					{ return m_bCRC32ForceAdding; }
	static	void	SetCRC32ForceAdding (bool _b)			{ m_bCRC32ForceAdding = _b; }
	static	CString GetCRC32Prefix ()						{ return m_sCRC32Prefix; }
	static	void	SetCRC32Prefix (CString _s)				{ _stprintf (m_sCRC32Prefix,_T("%s"),_s); }
	static	CString GetCRC32Suffix ()						{ return m_sCRC32Suffix; }
	static	void	SetCRC32Suffix (CString _s)				{ _stprintf (m_sCRC32Suffix,_T("%s"),_s); }
	// [end] Mighty Knife

	// Mighty Knife: Simple cleanup options
	static	void	SetSimpleCleanupOptions (int _i)	      { m_SimpleCleanupOptions = _i; }
	static	int 	GetSimpleCleanupOptions ()			      { return m_SimpleCleanupOptions; }
	static	void	SetSimpleCleanupSearch (CString _s)	      { m_SimpleCleanupSearch = _s; }
	static	CString	GetSimpleCleanupSearch ()			      { return m_SimpleCleanupSearch; }
	static	void	SetSimpleCleanupReplace (CString _s)	  { m_SimpleCleanupReplace = _s; }
	static	CString	GetSimpleCleanupReplace ()				  { return m_SimpleCleanupReplace; }
	static	void	SetSimpleCleanupSearchChars (CString _s)  { m_SimpleCleanupSearchChars = _s; }
	static	CString	GetSimpleCleanupSearchChars ()			  { return m_SimpleCleanupSearchChars; }
	static	void	SetSimpleCleanupReplaceChars (CString _s) { m_SimpleCleanupReplaceChars = _s; }
	static	CString	GetSimpleCleanupReplaceChars ()			  { return m_SimpleCleanupReplaceChars; }
	// [end] Mighty Knife

	// Mighty Knife: Static server handling
	static	bool    GetDontRemoveStaticServers ()			  { return m_bDontRemoveStaticServers; }
	static	void	SetDontRemoveStaticServers (bool _b)	  { m_bDontRemoveStaticServers = _b; }
	// [end] Mighty Knife

	static bool	IsAutoUPdateIPFilterEnabled()		{ return AutoUpdateIPFilter; } //MORPH START added by Yun.SF3: Ipfilter.dat update
	static bool IsAutoUPdateIP2CountryEnabled()    { return AutoUpdateIP2Country; } //Commander - Added: IP2Country Auto-updating
	static bool	IsSolidGraph()	{ return m_bSolidGraph;} //MORPH - Added by SiRoB, New Graph

	// khaos::categorymod+
	static	bool	ShowValidSrcsOnly()		{ return m_bValidSrcsOnly; }
	static	bool	ShowCatNameInDownList()	{ return m_bShowCatNames; }
	static	bool	SelectCatForNewDL()		{ return m_bSelCatOnAdd; }
	static	bool	UseActiveCatForLinks()	{ return m_bActiveCatDefault; }
	static	bool	AutoSetResumeOrder()	{ return m_bAutoSetResumeOrder; }
	static	bool	SmallFileDLPush()		{ return m_bSmallFileDLPush; }
	static	uint8	StartDLInEmptyCats()	{ return m_iStartDLInEmptyCats; } // 0 = disabled, otherwise num to resume
	static	bool	UseAutoCat()			{ return m_bUseAutoCat; }
	// khaos::categorymod-
	// khaos::kmod+
	static	bool	UseSmartA4AFSwapping()	{ return m_bSmartA4AFSwapping; } // only for NNP swaps and file completes, stops, cancels, etc.
	static	uint8	AdvancedA4AFMode()		{ return m_iAdvancedA4AFMode; } // 0 = disabled, 1 = balance, 2 = stack -- controls the balancing routines for on queue sources
	static	bool	RespectMaxSources()		{ return m_bRespectMaxSources; }
	static	bool	ShowA4AFDebugOutput()	{ return m_bShowA4AFDebugOutput; }
	static	bool	UseSaveLoadSources()	{ return m_bUseSaveLoadSources; }
	// khaos::categorymod-
	// khaos::accuratetimerem+
	static	uint8	GetTimeRemainingMode()	{ return m_iTimeRemainingMode; }
	// khaos::accuratetimerem-
	//MORPH START - Added by SiRoB, ICS Optional
	static	bool	UseICS()	{ return m_bUseIntelligentChunkSelection; }
	//MORPH END   - Added by SiRoB, ICS Optional

    //Commander - Added: Invisible Mode [TPT] - Start
    static	bool GetInvisibleMode() { return m_bInvisibleMode; }
	static	UINT GetInvisibleModeHKKeyModifier() { return m_iInvisibleModeHotKeyModifier; }
	static	char GetInvisibleModeHKKey() { return m_cInvisibleModeHotKey; }
	static	void SetInvisibleMode(bool on, UINT keymodifier, char key);
   //Commander - Added: Invisible Mode [TPT] - End

	//MORPH START - Added by SiRoB, Upload Splitting Class
	static	uint32	GetGlobalDataRateFriend();
	static	uint32	GetGlobalDataRatePowerShare();
	static	uint32	GetMaxClientDataRateFriend();
	static	uint32	GetMaxClientDataRatePowerShare();
	static	uint32	GetMaxClientDataRate();
	//MORPH END   - Added by SiRoB, Upload Splitting Class

	//MORPH START - Added by SiRoB, DynDNS
	static	void	SaveDynDNS();
	static	void	LoadDynDNS();
	static	int		AddDynDNSAccount(DynDNS_Struct* DynDNSAccount) { DynDNSMap.Add(DynDNSAccount); return DynDNSMap.GetCount()-1;}
	static	DynDNS_Struct* GetDynDNSAccount(int index) { if (index>=0 && index<DynDNSMap.GetCount()) return DynDNSMap.GetAt(index); else return NULL;}
	static	void	RemoveDynDNSAccount(int index);
	static	int		GetDynDNSCount()			{ return DynDNSMap.GetCount();}
	//MORPH END   - Added by SiRoB, DynDNS

protected:
	static	CString appdir;
	static	CString configdir;
	static	CString m_strWebServerDir;
	static	CString m_strLangDir;
	static	CString m_strFileCommentsFilePath;
	static	CString m_strLogDir;
	//MORPH START - Added by SiRoB, XML News [O²]
	static	CString m_strFeedsDir; // Added by N_OxYdE: XML News
	//MORPH END   - Added by SiRoB, XML News [O²]
	static	Preferences_Ext_Struct* prefsExt;
	static	WORD m_wWinVer;
	static	bool m_UseProxyListenPort;
	static	uint16	ListenPort;
	static	CArray<Category_Struct*,Category_Struct*> catMap;
	
	static void	CreateUserHash();
	static void	SetStandartValues();
	static int	GetRecommendedMaxConnections();
	static void LoadPreferences();
	static void SavePreferences();
	static CString GetHomepageBaseURLForLevel(uint8 nLevel);
	//MORPH START -  Added by SiRoB, DynDNS
	static	CArray<DynDNS_Struct*,DynDNS_Struct*> DynDNSMap;
	//MORPH END   -  Added by SiRoB, DynDNS
public:
	//MORPH START - Added by SiRoB [MoNKi: -UPnPNAT Support-]
	static	bool	GetUPnPNat()						{ return m_bUPnPNat; }
	static	void	SetUPnPNat(bool on)					{ m_bUPnPNat = on; }
	static	bool	GetUPnPNatWeb()						{ return m_bUPnPNatWeb; }
	static	void	SetUPnPNatWeb(bool on)				{ m_bUPnPNatWeb = on; }
	static	void	SetUPnPTCPExternal(uint16 port)		{ m_iUPnPTCPExternal = port; }
	static	void	SetUPnPUDPExternal(uint16 port)		{ m_iUPnPUDPExternal = port; }
	static	bool	GetUPnPNatTryRandom()				{ return m_bUPnPTryRandom; }
	static	void	SetUPnPNatTryRandom(bool on)		{ m_bUPnPTryRandom = on; }
	static	void	SetUPnPTCPInternal(uint16 port)		{ m_iUPnPTCPInternal = port; }
	static	uint16	GetUPnPTCPInternal()				{ return m_iUPnPTCPInternal; }
	static	void	SetUPnPUDPInternal(uint16 port)		{ m_iUPnPUDPInternal = port; }
	static	uint16	GetUPnPUDPInternal()				{ return m_iUPnPUDPInternal; }
	//MORPH END   - Added by SiRoB [MoNKi: -UPnPNAT Support-]

	//MORPH START - Added by SiRoB [MoNKi: -Random Ports-]
	static	bool	GetUseRandomPorts()				{ return m_bRndPorts; }
	static	void	SetUseRandomPorts(bool on)		{ m_bRndPorts = on; }
	static	uint16	GetMinRandomPort()				{ return m_iMinRndPort; }
	static	void	SetMinRandomPort(uint16 min)	{ m_iMinRndPort = min; }
	static	uint16	GetMaxRandomPort()				{ return m_iMaxRndPort; }
	static	void	SetMaxRandomPort(uint16 max)	{ m_iMaxRndPort = max; }
	//MORPH END   - Added by SiRoB [MoNKi: -Random Ports-]

	//MORPH START - Added by SiRoB [MoNKi: -Improved ICS-Firewall support-]
	static	bool	GetICFSupport() { return m_bICFSupport; }
	static	void	SetICFSupport(bool on) { m_bICFSupport = on; }
	static	bool	GetICFSupportFirstTime() { return m_bICFSupportFirstTime; }
	static	void	SetICFSupportFirstTime(bool on) { m_bICFSupportFirstTime = on; }
	static	bool	GetICFSupportServerUDP() { return m_bICFSupportServerUDP; }
	static	void	SetICFSupportServerUDP(bool on) { m_bICFSupportServerUDP = on; }
	//MORPH END   - Added by SiRoB [MoNKi: -Improved ICS-Firewall support-]

	//MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
	static CString	GetWapTemplate()				{ return CString(m_sWapTemplateFile);}
	static void		SetWapTemplate(CString in)		{ _stprintf(m_sWapTemplateFile,_T("%s"),in);}
	static bool		GetWapServerEnabled()			{ return m_bWapEnabled; }
	static void		SetWapServerEnabled(bool on)	{ m_bWapEnabled=on; }
	static uint16	GetWapPort()					{ return m_nWapPort; }
	static void		SetWapPort(uint16 uPort)		{ m_nWapPort=uPort; }
	static const CString& GetWapServerDir() 		{return m_strWapServerDir;}
	static uint8	GetWapGraphWidth()				{ return m_iWapGraphWidth; }
	static uint8	GetWapGraphHeight()				{ return m_iWapGraphHeight; }
	static void		SetWapGraphWidth(int width)		{ m_iWapGraphWidth=width; }
	static void		SetWapGraphHeight(int height)	{ m_iWapGraphHeight=height; }
	static bool		GetWapGraphsFilled()			{ return m_bWapFilledGraphs; }
	static void		SetWapGraphsFilled(bool on)		{ m_bWapFilledGraphs=on; }
	static void		SetWapMaxItemsInPages(int n)    { m_iWapMaxItemsInPages=n; }
	static int		GetWapMaxItemsInPages()		    { return m_iWapMaxItemsInPages; }
	static void		SetWapSendImages(bool on)		{ m_bWapSendImages=on; }
	static bool		GetWapSendImages()				{ return m_bWapSendImages; }
	static void		SetWapSendGraphs(bool on)		{ m_bWapSendGraphs=on; }
	static bool		GetWapSendGraphs()				{ return m_bWapSendGraphs; }
	static void		SetWapSendProgressBars(bool on)	{ m_bWapSendProgressBars=on; }
	static bool		GetWapSendProgressBars()		{ return m_bWapSendProgressBars; }
	static bool		GetWapAllwaysSendBWImages()		{ return m_bWapAllwaysSendBWImages; }
	static void		SetWapAllwaysSendBWImages(bool on)	{ m_bWapAllwaysSendBWImages=on; }
	static UINT		GetWapLogsSize()				{ return m_iWapLogsSize; }
	static void		SetWapLogsSize(UINT size)		{ m_iWapLogsSize=size; }
	static const	CString& GetWapPass()				{ return m_sWapPassword; }
	static void		SetWapPass(CString strNewPass);
	static bool		GetWapIsLowUserEnabled()		{ return m_bWapLowEnabled; }
	static void		SetWapIsLowUserEnabled(bool in)	{ m_bWapLowEnabled=in; }
	static const	CString& GetWapLowPass()			{ return m_sWapLowPassword; }
	static void		SetWapLowPass(CString strNewPass);
	//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]
};

extern CPreferences thePrefs;
