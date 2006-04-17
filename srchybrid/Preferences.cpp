//this file is part of eMule
//Copyright (C)2002-2005 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "stdafx.h"
#include <io.h>
#include <share.h>
#include "emule.h"
#include "Preferences.h"
#include "Opcodes.h"
#include "OtherFunctions.h"
#include "Ini2.h"
#include "DownloadQueue.h"
#include "UploadQueue.h"
#include "Statistics.h"
#include "MD5Sum.h"
#include "PartFile.h"
#include "Sockets.h"
#include "ListenSocket.h"
#include "ServerList.h"
#include "SharedFileList.h"
#include "UpDownClient.h"
#include "SafeFile.h"
#include "emuledlg.h"
#include "StatisticsDlg.h"
#include "Log.h"
#include "MuleToolbarCtrl.h"
#include "LastCommonRouteFinder.h" //MORPH - Added by SiRoB
#include "friendlist.h" //MORPH - Added by SiRoB, There is one slot friend or more

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

CPreferences thePrefs;

int		CPreferences::m_iDbgHeap;
CString	CPreferences::strNick;
uint16	CPreferences::minupload;
uint16	CPreferences::maxupload;
uint16	CPreferences::maxdownload;
LPCSTR	CPreferences::m_pszBindAddrA;
CStringA CPreferences::m_strBindAddrA;
LPCWSTR	CPreferences::m_pszBindAddrW;
CStringW CPreferences::m_strBindAddrW;
// MORPH START leuk_he upnp bindaddr
DWORD	 CPreferences::m_dwUpnpBindAddr;
bool     CPreferences::m_bBindAddrIsDhcp;
// MORPH End leuk_he upnp bindaddr

uint16	CPreferences::port;
uint16	CPreferences::udpport;
uint16	CPreferences::nServerUDPPort;
UINT	CPreferences::maxconnections;
UINT	CPreferences::maxhalfconnections;
bool	CPreferences::m_bConditionalTCPAccept;
bool	CPreferences::reconnect;
bool	CPreferences::m_bUseServerPriorities;
TCHAR	CPreferences::incomingdir[MAX_PATH];
CStringArray CPreferences::tempdir;
bool	CPreferences::ICH;
bool	CPreferences::m_bAutoUpdateServerList;
bool	CPreferences::updatenotify;
bool	CPreferences::mintotray;
bool	CPreferences::autoconnect;
bool	CPreferences::m_bAutoConnectToStaticServersOnly;
bool	CPreferences::autotakeed2klinks;
bool	CPreferences::addnewfilespaused;
UINT	CPreferences::depth3D;
bool	CPreferences::m_bEnableMiniMule;
int		CPreferences::m_iStraightWindowStyles;
bool	CPreferences::m_bRTLWindowsLayout;
CString	CPreferences::m_strSkinProfile;
CString	CPreferences::m_strSkinProfileDir;
bool	CPreferences::m_bAddServersFromServer;
bool	CPreferences::m_bAddServersFromClients;
UINT	CPreferences::maxsourceperfile;
UINT	CPreferences::trafficOMeterInterval;
UINT	CPreferences::statsInterval;
uchar	CPreferences::userhash[16];
WINDOWPLACEMENT CPreferences::EmuleWindowPlacement;
int		CPreferences::maxGraphDownloadRate;
int		CPreferences::maxGraphUploadRate;
uint32	CPreferences::maxGraphUploadRateEstimated = 0;
bool	CPreferences::beepOnError;
bool	CPreferences::m_bIconflashOnNewMessage;
bool	CPreferences::confirmExit;
DWORD	CPreferences::m_adwStatsColors[16]; //MORPH - Changed by SiRoB, Powershare display
bool	CPreferences::splashscreen;
bool	CPreferences::startupsound;//Commander - Added: Enable/Disable Startupsound
bool	CPreferences::sidebanner;//Commander - Added: Side Banner
bool	CPreferences::filterLANIPs;
bool	CPreferences::m_bAllocLocalHostIP;
bool	CPreferences::onlineSig;
uint64	CPreferences::cumDownOverheadTotal;
uint64	CPreferences::cumDownOverheadFileReq;
uint64	CPreferences::cumDownOverheadSrcEx;
uint64	CPreferences::cumDownOverheadServer;
uint64	CPreferences::cumDownOverheadKad;
uint64	CPreferences::cumDownOverheadTotalPackets;
uint64	CPreferences::cumDownOverheadFileReqPackets;
uint64	CPreferences::cumDownOverheadSrcExPackets;
uint64	CPreferences::cumDownOverheadServerPackets;
uint64	CPreferences::cumDownOverheadKadPackets;
uint64	CPreferences::cumUpOverheadTotal;
uint64	CPreferences::cumUpOverheadFileReq;
uint64	CPreferences::cumUpOverheadSrcEx;
uint64	CPreferences::cumUpOverheadServer;
uint64	CPreferences::cumUpOverheadKad;
uint64	CPreferences::cumUpOverheadTotalPackets;
uint64	CPreferences::cumUpOverheadFileReqPackets;
uint64	CPreferences::cumUpOverheadSrcExPackets;
uint64	CPreferences::cumUpOverheadServerPackets;
uint64	CPreferences::cumUpOverheadKadPackets;
uint32	CPreferences::cumUpSuccessfulSessions;
uint32	CPreferences::cumUpFailedSessions;
uint32	CPreferences::cumUpAvgTime;
uint64	CPreferences::cumUpData_EDONKEY;
uint64	CPreferences::cumUpData_EDONKEYHYBRID;
uint64	CPreferences::cumUpData_EMULE;
uint64	CPreferences::cumUpData_MLDONKEY;
uint64	CPreferences::cumUpData_AMULE;
uint64	CPreferences::cumUpData_EMULECOMPAT;
uint64	CPreferences::cumUpData_SHAREAZA;
uint64	CPreferences::sesUpData_EDONKEY;
uint64	CPreferences::sesUpData_EDONKEYHYBRID;
uint64	CPreferences::sesUpData_EMULE;
uint64	CPreferences::sesUpData_MLDONKEY;
uint64	CPreferences::sesUpData_AMULE;
uint64	CPreferences::sesUpData_EMULECOMPAT;
uint64	CPreferences::sesUpData_SHAREAZA;
uint64	CPreferences::cumUpDataPort_4662;
uint64	CPreferences::cumUpDataPort_OTHER;
uint64	CPreferences::cumUpDataPort_PeerCache;
uint64	CPreferences::sesUpDataPort_4662;
uint64	CPreferences::sesUpDataPort_OTHER;
uint64	CPreferences::sesUpDataPort_PeerCache;
uint64	CPreferences::cumUpData_File;
uint64	CPreferences::cumUpData_Partfile;
uint64	CPreferences::sesUpData_File;
uint64	CPreferences::sesUpData_Partfile;
uint32	CPreferences::cumDownCompletedFiles;
uint32	CPreferences::cumDownSuccessfulSessions;
uint32	CPreferences::cumDownFailedSessions;
uint32	CPreferences::cumDownAvgTime;
uint64	CPreferences::cumLostFromCorruption;
uint64	CPreferences::cumSavedFromCompression;
uint32	CPreferences::cumPartsSavedByICH;
uint32	CPreferences::sesDownSuccessfulSessions;
uint32	CPreferences::sesDownFailedSessions;
uint32	CPreferences::sesDownAvgTime;
uint32	CPreferences::sesDownCompletedFiles;
uint64	CPreferences::sesLostFromCorruption;
uint64	CPreferences::sesSavedFromCompression;
uint32	CPreferences::sesPartsSavedByICH;
uint64	CPreferences::cumDownData_EDONKEY;
uint64	CPreferences::cumDownData_EDONKEYHYBRID;
uint64	CPreferences::cumDownData_EMULE;
uint64	CPreferences::cumDownData_MLDONKEY;
uint64	CPreferences::cumDownData_AMULE;
uint64	CPreferences::cumDownData_EMULECOMPAT;
uint64	CPreferences::cumDownData_SHAREAZA;
uint64	CPreferences::cumDownData_URL;
uint64	CPreferences::cumDownData_WEBCACHE; //jp webcache statistics // MORPH - Added by Commander, WebCache 1.2e
uint64	CPreferences::sesDownData_EDONKEY;
uint64	CPreferences::sesDownData_EDONKEYHYBRID;
uint64	CPreferences::sesDownData_EMULE;
uint64	CPreferences::sesDownData_MLDONKEY;
uint64	CPreferences::sesDownData_AMULE;
uint64	CPreferences::sesDownData_EMULECOMPAT;
uint64	CPreferences::sesDownData_SHAREAZA;
uint64	CPreferences::sesDownData_URL;
// MORPH START - Added by Commander, WebCache 1.2e
uint64	CPreferences::sesDownData_WEBCACHE; //jp webcache statistics
uint32	CPreferences::ses_WEBCACHEREQUESTS; //jp webcache statistics needs to be uint32 or the statistics won't work
uint32	CPreferences::ses_PROXYREQUESTS; //jp webcache statistics
uint32	CPreferences::ses_successfullPROXYREQUESTS; //jp webcache statistics
uint32	CPreferences::ses_successfull_WCDOWNLOADS; //jp webcache statistics needs to be uint32 or the statistics won't work
// MORPH END - Added by Commander, WebCache 1.2e
uint64	CPreferences::cumDownDataPort_4662;
uint64	CPreferences::cumDownDataPort_OTHER;
uint64	CPreferences::cumDownDataPort_PeerCache;
uint64	CPreferences::sesDownDataPort_4662;
uint64	CPreferences::sesDownDataPort_OTHER;
uint64	CPreferences::sesDownDataPort_PeerCache;
float	CPreferences::cumConnAvgDownRate;
float	CPreferences::cumConnMaxAvgDownRate;
float	CPreferences::cumConnMaxDownRate;
float	CPreferences::cumConnAvgUpRate;
float	CPreferences::cumConnMaxAvgUpRate;
float	CPreferences::cumConnMaxUpRate;
time_t	CPreferences::cumConnRunTime;
uint32	CPreferences::cumConnNumReconnects;
uint32	CPreferences::cumConnAvgConnections;
uint32	CPreferences::cumConnMaxConnLimitReached;
uint32	CPreferences::cumConnPeakConnections;
uint32	CPreferences::cumConnTransferTime;
uint32	CPreferences::cumConnDownloadTime;
uint32	CPreferences::cumConnUploadTime;
uint32	CPreferences::cumConnServerDuration;
uint32	CPreferences::cumSrvrsMostWorkingServers;
uint32	CPreferences::cumSrvrsMostUsersOnline;
uint32	CPreferences::cumSrvrsMostFilesAvail;
uint32	CPreferences::cumSharedMostFilesShared;
uint64	CPreferences::cumSharedLargestShareSize;
uint64	CPreferences::cumSharedLargestAvgFileSize;
uint64	CPreferences::cumSharedLargestFileSize;
time_t	CPreferences::stat_datetimeLastReset;
UINT	CPreferences::statsConnectionsGraphRatio;
UINT	CPreferences::statsSaveInterval;
TCHAR	CPreferences::statsExpandedTreeItems[256];
bool	CPreferences::m_bShowVerticalHourMarkers;
uint64	CPreferences::totalDownloadedBytes;
uint64	CPreferences::totalUploadedBytes;
WORD	CPreferences::m_wLanguageID;
bool	CPreferences::transferDoubleclick;
EViewSharedFilesAccess CPreferences::m_iSeeShares;
UINT	CPreferences::m_iToolDelayTime;
bool	CPreferences::bringtoforeground;
UINT	CPreferences::splitterbarPosition;
UINT	CPreferences::splitterbarPositionSvr;
UINT	CPreferences::splitterbarPositionStat;
UINT	CPreferences::splitterbarPositionStat_HL;
UINT	CPreferences::splitterbarPositionStat_HR;
UINT	CPreferences::splitterbarPositionFriend;
UINT	CPreferences::splitterbarPositionIRC;
UINT	CPreferences::splitterbarPositionShared;
UINT	CPreferences::m_uTransferWnd1;
UINT	CPreferences::m_uTransferWnd2;
UINT	CPreferences::m_uDeadServerRetries;
DWORD	CPreferences::m_dwServerKeepAliveTimeout;
UINT	CPreferences::statsMax;
UINT	CPreferences::statsAverageMinutes;
CString	CPreferences::notifierConfiguration;
bool	CPreferences::notifierOnDownloadFinished;
bool	CPreferences::notifierOnNewDownload;
bool	CPreferences::notifierOnChat;
bool	CPreferences::notifierOnLog;
bool	CPreferences::notifierOnImportantError;
bool	CPreferences::notifierOnEveryChatMsg;
bool	CPreferences::notifierOnNewVersion;
ENotifierSoundType CPreferences::notifierSoundType = ntfstNoSound;
CString	CPreferences::notifierSoundFile;
TCHAR	CPreferences::m_sircserver[50];
TCHAR	CPreferences::m_sircnick[30];
TCHAR	CPreferences::m_sircchannamefilter[50];
bool	CPreferences::m_bircaddtimestamp;
bool	CPreferences::m_bircusechanfilter;
UINT	CPreferences::m_iircchanneluserfilter;
TCHAR	CPreferences::m_sircperformstring[255];
bool	CPreferences::m_bircuseperform;
bool	CPreferences::m_birclistonconnect;
bool	CPreferences::m_bircacceptlinks;
bool	CPreferences::m_bircacceptlinksfriends;
bool	CPreferences::m_bircsoundevents;
bool	CPreferences::m_bircignoremiscmessage;
bool	CPreferences::m_bircignorejoinmessage;
bool	CPreferences::m_bircignorepartmessage;
bool	CPreferences::m_bircignorequitmessage;
bool	CPreferences::m_bircignoreemuleprotoaddfriend;
bool	CPreferences::m_bircallowemuleprotoaddfriend;
bool	CPreferences::m_bircignoreemuleprotosendlink;
bool	CPreferences::m_birchelpchannel;
bool	CPreferences::m_bRemove2bin;
bool	CPreferences::m_bShowCopyEd2kLinkCmd;
bool	CPreferences::m_bpreviewprio;
bool	CPreferences::m_bSmartServerIdCheck;
uint8	CPreferences::smartidstate;
bool	CPreferences::m_bSafeServerConnect;
bool	CPreferences::startMinimized;
bool	CPreferences::m_bAutoStart;
bool	CPreferences::m_bRestoreLastMainWndDlg;
int		CPreferences::m_iLastMainWndDlgID;
bool	CPreferences::m_bRestoreLastLogPane;
int		CPreferences::m_iLastLogPaneID;
UINT	CPreferences::MaxConperFive;
bool	CPreferences::checkDiskspace;
UINT	CPreferences::m_uMinFreeDiskSpace;
bool	CPreferences::m_bSparsePartFiles;
CString	CPreferences::m_strYourHostname;
bool	CPreferences::m_bEnableVerboseOptions;
bool	CPreferences::m_bVerbose;
bool	CPreferences::m_bFullVerbose;
bool	CPreferences::m_bDebugSourceExchange;
bool	CPreferences::m_bLogBannedClients;
bool	CPreferences::m_bLogRatingDescReceived;
bool	CPreferences::m_bLogSecureIdent;
bool	CPreferences::m_bLogFilteredIPs;
bool	CPreferences::m_bLogFileSaving;
bool	CPreferences::m_bLogA4AF; // ZZ:DownloadManager
bool	CPreferences::m_bLogUlDlEvents;
// MORPH START - Added by Commander, WebCache 1.2e
bool	CPreferences::m_bLogWebCacheEvents;//JP log webcache events
bool	CPreferences::m_bLogICHEvents;//JP log ICH events
// MORPH END - Added by Commander, WebCache 1.2e
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
bool	CPreferences::m_bUseDebugDevice = true;
#else
bool	CPreferences::m_bUseDebugDevice = false;
#endif
int		CPreferences::m_iDebugServerTCPLevel;
int		CPreferences::m_iDebugServerUDPLevel;
int		CPreferences::m_iDebugServerSourcesLevel;
int		CPreferences::m_iDebugServerSearchesLevel;
int		CPreferences::m_iDebugClientTCPLevel;
int		CPreferences::m_iDebugClientUDPLevel;
int		CPreferences::m_iDebugClientKadUDPLevel;
int		CPreferences::m_iDebugSearchResultDetailLevel;
bool	CPreferences::m_bupdatequeuelist;
bool	CPreferences::m_bManualAddedServersHighPriority;
bool	CPreferences::m_btransferfullchunks;
int		CPreferences::m_istartnextfile;
bool	CPreferences::m_bshowoverhead;
bool	CPreferences::m_bDAP;
bool	CPreferences::m_bUAP;
bool	CPreferences::m_bDisableKnownClientList;
bool	CPreferences::m_bDisableQueueList;
bool	CPreferences::m_bExtControls;
bool	CPreferences::m_bTransflstRemain;
UINT	CPreferences::versioncheckdays;
bool	CPreferences::showRatesInTitle;
TCHAR	CPreferences::TxtEditor[256];
TCHAR	CPreferences::VideoPlayer[256];
bool	CPreferences::moviePreviewBackup;
int		CPreferences::m_iPreviewSmallBlocks;
bool	CPreferences::m_bPreviewCopiedArchives;
int		CPreferences::m_iInspectAllFileTypes;
bool	CPreferences::m_bPreviewOnIconDblClk;
bool	CPreferences::indicateratings;
bool	CPreferences::watchclipboard;
bool	CPreferences::filterserverbyip;
bool	CPreferences::m_bFirstStart;
bool	CPreferences::m_bCreditSystem;
bool	CPreferences::log2disk;
bool	CPreferences::debug2disk;
int		CPreferences::iMaxLogBuff;
UINT	CPreferences::uMaxLogFileSize;
ELogFileFormat CPreferences::m_iLogFileFormat = Unicode;
bool	CPreferences::scheduler;
bool	CPreferences::dontcompressavi;
bool	CPreferences::msgonlyfriends;
bool	CPreferences::msgsecure;
UINT	CPreferences::filterlevel;
UINT	CPreferences::m_iFileBufferSize;
UINT	CPreferences::m_iQueueSize;
int		CPreferences::m_iCommitFiles;
UINT	CPreferences::maxmsgsessions;
uint32	CPreferences::versioncheckLastAutomatic;
//MORPH START - Added by SiRoB, New Version check
uint32	CPreferences::mversioncheckLastAutomatic;
//MORPH START - Added by SiRoB, New Version check
CString	CPreferences::messageFilter;
CString	CPreferences::commentFilter;
CString	CPreferences::filenameCleanups;
TCHAR	CPreferences::datetimeformat[64];
TCHAR	CPreferences::datetimeformat4log[64];
LOGFONT CPreferences::m_lfHyperText;
LOGFONT CPreferences::m_lfLogText;
COLORREF CPreferences::m_crLogError = RGB(255, 0, 0);
COLORREF CPreferences::m_crLogWarning = RGB(128, 0, 128);
COLORREF CPreferences::m_crLogSuccess = RGB(0, 0, 255);
//MORPH START - Added by SiRoB, Upload Splitting Class
COLORREF CPreferences::m_crLogUSC = RGB(10,180,50);
//MORPH END   - Added by SiRoB, Upload Splitting Class
	
int		CPreferences::m_iExtractMetaData;
bool	CPreferences::m_bAdjustNTFSDaylightFileTime = true;
TCHAR	CPreferences::m_sWebPassword[256];
TCHAR	CPreferences::m_sWebLowPassword[256];
CUIntArray CPreferences::m_aAllowedRemoteAccessIPs;
uint16	CPreferences::m_nWebPort;
bool	CPreferences::m_bWebEnabled;
bool	CPreferences::m_bWebUseGzip;
int		CPreferences::m_nWebPageRefresh;
bool	CPreferences::m_bWebLowEnabled;
TCHAR	CPreferences::m_sWebResDir[MAX_PATH];
int		CPreferences::m_iWebTimeoutMins;
int		CPreferences::m_iWebFileUploadSizeLimitMB;
bool	CPreferences::m_bAllowAdminHiLevFunc;
TCHAR	CPreferences::m_sTemplateFile[MAX_PATH];
ProxySettings CPreferences::proxy;
bool	CPreferences::showCatTabInfos;
bool	CPreferences::resumeSameCat;
bool	CPreferences::dontRecreateGraphs;
bool	CPreferences::autofilenamecleanup;
bool	CPreferences::m_bUseAutocompl;
bool	CPreferences::m_bShowDwlPercentage;
bool    CPreferences::m_bShowClientPercentage;  //Commander - Added: Client Percentage
bool	CPreferences::m_bRemoveFinishedDownloads;
UINT	CPreferences::m_iMaxChatHistory;
bool	CPreferences::m_bShowActiveDownloadsBold;
int		CPreferences::m_iSearchMethod;
bool	CPreferences::m_bAdvancedSpamfilter;
bool	CPreferences::m_bUseSecureIdent;
TCHAR	CPreferences::m_sMMPassword[256];
bool	CPreferences::m_bMMEnabled;
uint16	CPreferences::m_nMMPort;
bool	CPreferences::networkkademlia;
bool	CPreferences::networked2k;
EToolbarLabelType CPreferences::m_nToolbarLabels;
CString	CPreferences::m_sToolbarBitmap;
CString	CPreferences::m_sToolbarBitmapFolder;
CString	CPreferences::m_sToolbarSettings;
bool	CPreferences::m_bReBarToolbar;
CSize	CPreferences::m_sizToolbarIconSize;
bool	CPreferences::m_bPreviewEnabled;
bool	CPreferences::m_bDynUpEnabled;
int	CPreferences::m_iDynUpPingTolerance;
int	CPreferences::m_iDynUpGoingUpDivider;
int	CPreferences::m_iDynUpGoingDownDivider;
int	CPreferences::m_iDynUpNumberOfPings;
int		CPreferences::m_iDynUpPingToleranceMilliseconds;
bool	CPreferences::m_bDynUpUseMillisecondPingTolerance;
//MORPH START - Added by SiRoB, USSLog
bool	CPreferences::m_bDynUpLog;
//MORPH END   - Added by SiRoB, USSLog
bool	CPreferences::m_bUSSUDP; //MORPH - Added by SiRoB, USS UDP preferency
//MORPH START - Added by Commander, ClientQueueProgressBar
bool CPreferences::m_bClientQueueProgressBar;
//MORPH END - Added by Commander, ClientQueueProgressBar

//MORPH START - Added by Commander, Show WC stats
bool CPreferences::m_bCountWCSessionStats;
//MORPH END - Added by Commander, Show WC stats

//MORPH START - Added by Commander, FolderIcons
bool CPreferences::m_bShowFolderIcons;
//MORPH END - Added by Commander, FolderIcons

// ==> Slot Limit - Stulle
bool	CPreferences::m_bSlotLimitThree;
bool	CPreferences::m_bSlotLimitNum;
uint8	CPreferences::m_iSlotLimitNum;
// <== Slot Limit - Stulle

bool	CPreferences::enableHighProcess;//MORPH - Added by IceCream, high process priority
bool	CPreferences::enableDownloadInRed; //MORPH - Added by IceCream, show download in red
bool	CPreferences::enableAntiLeecher; //MORPH - Added by IceCream, enableAntiLeecher
bool	CPreferences::enableAntiCreditHack; //MORPH - Added by IceCream, enableAntiCreditHack
int	CPreferences::creditSystemMode; // EastShare - Added by linekin, creditsystem integration
bool	CPreferences::m_bEnableEqualChanceForEachFile;//Morph - added by AndCycle, Equal Chance For Each File
bool	CPreferences::isautodynupswitching;//MORPH - Added by Yun.SF3, Auto DynUp changing
int	CPreferences::m_iPowershareMode; //MORPH - Added by SiRoB, Avoid misusing of powersharing
bool	CPreferences::m_bPowershareInternalPrio; //Morph - added by AndCyle, selective PS internal Prio
int	CPreferences::maxconnectionsswitchborder;
//EastShare Start- Added by Pretender, TBH-AutoBackup
bool	CPreferences::autobackup;
bool	CPreferences::autobackup2;
//EastShare End - Added by Pretender, TBH-AutoBackup

//MORPH START - Added by SiRoB, Datarate Average Time Management
int	CPreferences::m_iDownloadDataRateAverageTime;
int	CPreferences::m_iUploadDataRateAverageTime;
//MORPH END   - Added by SiRoB, Datarate Average Time Management

//MORPH START - Added by SiRoB, Upload Splitting Class
int	CPreferences::globaldataratefriend;
int	CPreferences::maxglobaldataratefriend;
int	CPreferences::globaldataratepowershare;
int	CPreferences::maxglobaldataratepowershare;
int	CPreferences::maxclientdataratefriend;
int	CPreferences::maxclientdataratepowershare;
int	CPreferences::maxclientdatarate;
//MORPH END   - Added by SiRoB, Upload Splitting Class
//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
int	CPreferences::LowIdRetries;
int	CPreferences::LowIdRetried;
//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry
//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
int	CPreferences::m_iSpreadbarSetStatus;
//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
int	CPreferences::hideOS;
bool	CPreferences::selectiveShare;
//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS

bool	CPreferences::infiniteQueue;	//Morph - added by AndCycle, SLUGFILLER: infiniteQueue

int	CPreferences::ShareOnlyTheNeed; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED Wistily idea
int	CPreferences::PowerShareLimit; //MORPH - Added by SiRoB, POWERSHARE Limit
int	CPreferences::permissions;//MORPH - Added by SiRoB, Show Permissions

//EastShare Start - PreferShareAll by AndCycle
bool	CPreferences::shareall;	// SLUGFILLER: preferShareAll
//EastShare End - PreferShareAll by AndCycle

bool	CPreferences::m_bEnableChunkDots;
//EastShare - Added by Pretender, Option for ChunkDots




TCHAR	CPreferences::UpdateURLFakeList[256];//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
TCHAR	CPreferences::UpdateURLIPFilter[256];//MORPH START added by Yun.SF3: Ipfilter.dat update
TCHAR	CPreferences::UpdateURLIP2Country[256];//Commander - Added: IP2Country auto-updating

bool	CPreferences::m_bPayBackFirst;//EastShare - added by AndCycle, Pay Back First
uint8	CPreferences::m_iPayBackFirstLimit;//MORPH - Added by SiRoB, Pay Back First Tweak
bool	CPreferences::m_bOnlyDownloadCompleteFiles;//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
bool	CPreferences::m_bSaveUploadQueueWaitTime;//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
int	CPreferences::m_iKnownMetDays; // EastShare - Added by TAHO, .met file control
bool	CPreferences::m_bDateFileNameLog;//Morph - added by AndCycle, Date File Name Log
bool	CPreferences::m_bDontRemoveSpareTrickleSlot;//Morph - added by AndCycle, Dont Remove Spare Trickle Slot
bool	CPreferences::m_bFunnyNick;//MORPH - Added by SiRoB, Optionnal funnynick display

//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
SYSTEMTIME	CPreferences::m_FakesDatVersion;
bool	CPreferences::UpdateFakeStartup;
//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating
//MORPH START added by Yun.SF3: Ipfilter.dat update
bool	CPreferences::AutoUpdateIPFilter; //added by milobac: Ipfilter.dat update
SYSTEMTIME	CPreferences::m_IPfilterVersion; //added by milobac: Ipfilter.dat update
//MORPH END added by Yun.SF3: Ipfilter.dat update

//Commander - Added: IP2Country Auto-updating - Start
bool	CPreferences::AutoUpdateIP2Country;
SYSTEMTIME	CPreferences::m_IP2CountryVersion;
//Commander - Added: IP2Country Auto-updating - End

//EastShare - added by AndCycle, IP to Country
int	CPreferences::m_iIP2CountryNameMode;
bool	CPreferences::m_bIP2CountryShowFlag;
//EastShare - added by AndCycle, IP to Country

// khaos::categorymod+
bool	CPreferences::m_bValidSrcsOnly;
bool	CPreferences::m_bShowCatNames;
bool	CPreferences::m_bActiveCatDefault;
bool	CPreferences::m_bSelCatOnAdd;
bool	CPreferences::m_bAutoSetResumeOrder;
bool	CPreferences::m_bSmallFileDLPush;
uint8	CPreferences::m_iStartDLInEmptyCats;
bool	CPreferences::m_bRespectMaxSources;
bool	CPreferences::m_bUseAutoCat;
// khaos::categorymod-
// khaos::kmod+
bool	CPreferences::m_bShowA4AFDebugOutput;
bool	CPreferences::m_bSmartA4AFSwapping;
uint8	CPreferences::m_iAdvancedA4AFMode; // 0 = disabled, 1 = balance, 2 = stack
bool	CPreferences::m_bUseSaveLoadSources;
// khaos::categorymod-
// khaos::accuratetimerem+
uint8	CPreferences::m_iTimeRemainingMode; // 0 = both, 1 = real time, 2 = average
// khaos::accuratetimerem-

//MORPH START - Added by SiRoB, ICS Optional
bool	CPreferences::m_bUseIntelligentChunkSelection;
//MORPH END   - Added by SiRoB, ICS Optional

// Mighty Knife: Community Visualization, Report hashing files, Log friendlist activities
TCHAR	CPreferences::m_sCommunityName [256];
bool	CPreferences::m_bReportHashingFiles;
bool	CPreferences::m_bLogFriendlistActivities;
// [end] Mighty Knife

// Mighty Knife: CRC32-Tag - not accessible in preferences dialog !
bool	CPreferences::m_bDontAddCRCToFilename;
bool	CPreferences::m_bCRC32ForceUppercase;
bool	CPreferences::m_bCRC32ForceAdding;
TCHAR	CPreferences::m_sCRC32Prefix [256];
TCHAR	CPreferences::m_sCRC32Suffix [256];
// [end] Mighty Knife

// Mighty Knife: Simple cleanup options
int      CPreferences::m_SimpleCleanupOptions;
CString  CPreferences::m_SimpleCleanupSearch;
CString  CPreferences::m_SimpleCleanupReplace;
CString  CPreferences::m_SimpleCleanupSearchChars;
CString  CPreferences::m_SimpleCleanupReplaceChars;
// [end] Mighty Knife

// Mighty Knife: Static server handling
bool	CPreferences::m_bDontRemoveStaticServers;
// [end] Mighty Knife

//MORPH START - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
bool	CPreferences::m_bSUCEnabled;
int		CPreferences::m_iSUCHigh;
int		CPreferences::m_iSUCLow;
int		CPreferences::m_iSUCPitch;
int		CPreferences::m_iSUCDrift;
bool	CPreferences::m_bSUCLog;
//MORPH END - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
bool	CPreferences::m_bSolidGraph; //MORPH - Added by SiRoB, New Graph
//Commander - Added: Invisible Mode [TPT] - Start
bool	CPreferences::m_bInvisibleMode;		
UINT	CPreferences::m_iInvisibleModeHotKeyModifier;
char	CPreferences::m_cInvisibleModeHotKey;
//Commander - Added: Invisible Mode [TPT] - End

bool    CPreferences::m_bAllocFull;
// ZZ:DownloadManager -->
bool    CPreferences::m_bA4AFSaveCpu;
// ZZ:DownloadManager <--
bool    CPreferences::m_bHighresTimer;
CStringList CPreferences::shareddir_list;
CStringList CPreferences::addresses_list;
CString CPreferences::appdir;
CString CPreferences::configdir;
CString CPreferences::m_strWebServerDir;
CString CPreferences::m_strLangDir;
CString CPreferences::m_strFileCommentsFilePath;
CString	CPreferences::m_strLogDir;
Preferences_Ext_Struct* CPreferences::prefsExt;
WORD	CPreferences::m_wWinVer;
CArray<Category_Struct*,Category_Struct*> CPreferences::catMap;
UINT	CPreferences::m_nWebMirrorAlertLevel;
bool	CPreferences::m_bRunAsUser;
bool	CPreferences::m_bPreferRestrictedOverUser;
bool	CPreferences::m_bUseOldTimeRemaining;
uint32	CPreferences::m_uPeerCacheLastSearch;
bool	CPreferences::m_bPeerCacheWasFound;
bool	CPreferences::m_bPeerCacheEnabled;
uint16	CPreferences::m_nPeerCachePort;
bool	CPreferences::m_bPeerCacheShow;

bool	CPreferences::m_bOpenPortsOnStartUp;
int		CPreferences::m_byLogLevel;
bool	CPreferences::m_bTrustEveryHash;
bool	CPreferences::m_bRememberCancelledFiles;
bool	CPreferences::m_bRememberDownloadedFiles;

bool	CPreferences::m_bNotifierSendMail;
CString	CPreferences::m_strNotifierMailServer;
CString	CPreferences::m_strNotifierMailSender;
CString	CPreferences::m_strNotifierMailReceiver;

bool	CPreferences::m_bWinaTransToolbar;

//MORPH START - Added by SiRoB, XML News [O²]
CString	CPreferences::m_strFeedsDir;
bool	CPreferences::enableNEWS;
//MORPH END   - Added by SiRoB, XML News [O²]

//MORPH START - Added by SiRoB, [MoNKi: -UPnPNAT Support-]
	bool	CPreferences::m_bUPnPNat;
	bool	CPreferences::m_bUPnPNatWeb;
	bool	CPreferences::m_bUPnPVerboseLog;
	uint16	CPreferences::m_iUPnPPort;
	bool	CPreferences::m_bUPnPLimitToFirstConnection;
	bool	CPreferences::m_bUPnPClearOnClose;
	int     CPreferences::m_iDetectuPnP; //leuk_he autodetect in startup wizard
	// End MoNKi
//MORPH END   - Added by SiRoB, [MoNKi: -UPnPNAT Support-]

//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-]
	bool	CPreferences::m_bRndPorts;
	uint16	CPreferences::m_iMinRndPort;
	uint16	CPreferences::m_iMaxRndPort;
	bool	CPreferences::m_bRndPortsResetOnRestart;
	uint16	CPreferences::m_iRndPortsSafeResetOnRestartTime;
	uint16	CPreferences::m_iCurrentTCPRndPort;
	uint16	CPreferences::m_iCurrentUDPRndPort;
//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-]

//MORPH START - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
	bool	CPreferences::m_bICFSupport;
	bool	CPreferences::m_bICFSupportFirstTime;
	bool	CPreferences::m_bICFSupportStatusChanged;
	bool	CPreferences::m_bICFSupportServerUDP;
//MORPH END   - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]

//MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
	TCHAR	CPreferences::m_sWapTemplateFile[MAX_PATH];
	bool	CPreferences::m_bWapEnabled;
	uint16	CPreferences::m_nWapPort;
	int		CPreferences::m_iWapGraphWidth;
	int		CPreferences::m_iWapGraphHeight;
	bool	CPreferences::m_bWapFilledGraphs;
	int		CPreferences::m_iWapMaxItemsInPages;
	bool	CPreferences::m_bWapSendImages;
	bool	CPreferences::m_bWapSendGraphs;
	bool	CPreferences::m_bWapSendProgressBars;
	bool	CPreferences::m_bWapAllwaysSendBWImages;
	UINT	CPreferences::m_iWapLogsSize;
	CString CPreferences::m_strWapServerDir;
	CString	CPreferences::m_sWapPassword;
	CString	CPreferences::m_sWapLowPassword;
	bool	CPreferences::m_bWapLowEnabled;
	//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]

// MORPH START - Added by Commander, WebCache 1.2e
CString	CPreferences::webcacheName;
uint16	CPreferences::webcachePort;
bool	CPreferences::webcacheReleaseAllowed; //jp webcache release
uint16	CPreferences::webcacheBlockLimit;
bool	CPreferences::PersistentConnectionsForProxyDownloads; //jp persistent proxy connections
bool	CPreferences::WCAutoupdate; //jp WCAutoupdate
bool	CPreferences::webcacheExtraTimeout;
bool	CPreferences::webcacheCachesLocalTraffic;
bool	CPreferences::webcacheEnabled;
bool	CPreferences::detectWebcacheOnStart; //jp detect webcache on startup
uint32	CPreferences::webcacheLastSearch;
CString	CPreferences::webcacheLastResolvedName;
uint32	CPreferences::webcacheLastGlobalIP;
bool	CPreferences::UsesCachedTCPPort()  //jp
{
	//MORPH - Changed by SiRoB,
	/*
	if ((thePrefs.GetPort()==80) || (thePrefs.port==21) || (thePrefs.port==443) || (thePrefs.port==563) || (thePrefs.port==70) || (thePrefs.port==210) || ((thePrefs.port>=1025) && (thePrefs.port<=65535))) return true;
	*/
	if ((thePrefs.GetPort()==80) || (thePrefs.GetPort()==21) || (thePrefs.GetPort()==443) || (thePrefs.GetPort()==563) || (thePrefs.GetPort()==70) || (thePrefs.GetPort()==210) || ((thePrefs.GetPort()>=1025) && (thePrefs.GetPort()<=65535))) return true;
	else return false;
}
//JP proxy configuration test start
bool	CPreferences::m_bHighIdPossible;
//JP proxy configuration test start
bool	CPreferences::WebCacheDisabledThisSession;//jp temp disabled
uint32	CPreferences::WebCachePingSendTime;//jp check proxy config
bool	CPreferences::expectingWebCachePing;//jp check proxy config
bool	CPreferences::IsWebCacheTestPossible()//jp check proxy config
{
	return (theApp.GetPublicIP() != 0 //we have a public IP
		&& theApp.serverconnect->IsConnected() //connected to a server
		&& !theApp.serverconnect->IsLowID()//don't have LowID
		&& m_bHighIdPossible
		&& !theApp.listensocket->TooManySockets());// no fake high ID
}
//JP proxy configuration test end
// WebCache ////////////////////////////////////////////////////////////////////////////////////
uint8	CPreferences::webcacheTrustLevel;
//JP webcache release START
bool	CPreferences::UpdateWebcacheReleaseAllowed()
{
	webcacheReleaseAllowed = true;
	if (theApp.downloadqueue->ContainsUnstoppedFiles())
		webcacheReleaseAllowed = false;
	return webcacheReleaseAllowed;
}
//JP webcache release END
// MORPH END - Added by Commander, WebCache 1.2e

//MORPH START - Added by Stulle, Global Source Limit
UINT	CPreferences::m_uGlobalHL; 
bool	CPreferences::m_bGlobalHL;
//MORPH END   - Added by Stulle, Global Source Limit

CPreferences::CPreferences()
{
#ifdef _DEBUG
	m_iDbgHeap = 1;
#endif
// MORPH START - Added by Commander, WebCache 1.2e
//JP set standard values for stuff that doesn't need to be saved. This should probably be somewhere else START
expectingWebCachePing = false;
WebCachePingSendTime = 0;
WebCacheDisabledThisSession = false;
webcacheReleaseAllowed = true; //jp webcache release
m_bHighIdPossible = false; // JP detect fake HighID (from netfinity)
//JP set standard values for stuff that doesn't need to be saved. This should probably be somewhere else END
// MORPH END - Added by Commander, WebCache 1.2e
}

CPreferences::~CPreferences()
{
	delete prefsExt;
}

LPCTSTR CPreferences::GetConfigFile()
{
	return theApp.m_pszProfileName;
}

void CPreferences::Init()
{
	srand((uint32)time(0)); // we need random numbers sometimes
	// khaos::kmod+ We need _better_ random numbers...  Sometimes.
	InitRandGen();
	// khaos::kmod-

	prefsExt=new Preferences_Ext_Struct;
	memset(prefsExt, 0, sizeof *prefsExt);

	//get application start directory
	TCHAR buffer[490];
	::GetModuleFileName(0, buffer, 490);
	LPTSTR pszFileName = _tcsrchr(buffer, L'\\') + 1;
	*pszFileName = L'\0';

	appdir = buffer;
	configdir = appdir + CONFIGFOLDER;
	//emulEspaña: Modified by MoNKi [MoNKi -Web/Wap Debug Dirs-]
	/*
	m_strWebServerDir = appdir + L"webserver\\";
	*/
#ifndef _DEBUG
	m_strWebServerDir = appdir + L"webserver\\";
	m_strWapServerDir = appdir + L"wapserver\\";	//MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
#else
	m_strWebServerDir = appdir + L"..\\webserver\\";
	m_strWapServerDir = appdir + L"..\\wapserver\\";	//MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
#endif
	//End emulEspaña
	m_strLangDir = appdir + L"lang\\";
	m_strFileCommentsFilePath = configdir + L"fileinfo.ini";
	m_strLogDir = appdir + L"logs\\";

	//MORPH START - Added by SiRoB, XML News [O²]
	m_strFeedsDir = appdir + L"feeds\\"; // Added by N_OxYdE: XML News
	//MORPH END   - Added by SiRoB, XML News [O²]

	///////////////////////////////////////////////////////////////////////////
	// Create 'config' directory (and optionally move files from application directory)
	//
	::CreateDirectory(GetConfigDir(),0);

	//MORPH START - Added by SiRoB, XML News [O²]
	::CreateDirectory(GetFeedsDir(),0); // Added by N_OxYdE: XML News
	//MORPH END   - Added by SiRoB, XML News [O²]

	///////////////////////////////////////////////////////////////////////////
	// Create 'logs' directory (and optionally move files from application directory)
	//
	::CreateDirectory(GetLogDir(), 0);
	CFileFind ff;
	BOOL bFoundFile = ff.FindFile(GetAppDir() + L"eMule*.log", 0);
	while (bFoundFile)
	{
		bFoundFile = ff.FindNextFile();
		if (ff.IsDots() || ff.IsSystem() || ff.IsDirectory() || ff.IsHidden())
			continue;
		MoveFile(ff.GetFilePath(), GetLogDir() + ff.GetFileName());
	}


	CreateUserHash();

	// load preferences.dat or set standart values
	TCHAR* fullpath = new TCHAR[_tcslen(configdir)+16];
	_stprintf(fullpath,L"%spreferences.dat",configdir);
	FILE* preffile = _tfsopen(fullpath,L"rb", _SH_DENYWR);
	delete[] fullpath;

	LoadPreferences();

	if (!preffile){
		SetStandartValues();
	}
	else{
		fread(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		if (ferror(preffile))
			SetStandartValues();

		md4cpy(userhash, prefsExt->userhash);
		EmuleWindowPlacement = prefsExt->EmuleWindowPlacement;

		fclose(preffile);
		smartidstate = 0;
	}

	// shared directories
	fullpath = new TCHAR[_tcslen(configdir) + MAX_PATH];
	_stprintf(fullpath, L"%sshareddir.dat", configdir);
	CStdioFile* sdirfile = new CStdioFile();
	bool bIsUnicodeFile = IsUnicodeFile(fullpath); // check for BOM
	// open the text file either in ANSI (text) or Unicode (binary), this way we can read old and new files
	// with nearly the same code..
	if (sdirfile->Open(fullpath, CFile::modeRead | CFile::shareDenyWrite | (bIsUnicodeFile ? CFile::typeBinary : 0)))
	{
		try {
			if (bIsUnicodeFile)
				sdirfile->Seek(sizeof(WORD), SEEK_CUR); // skip BOM

		CString toadd;
		while (sdirfile->ReadString(toadd))
		{
				toadd.Trim(L"\r\n"); // need to trim '\r' in binary mode
			TCHAR szFullPath[MAX_PATH];
			if (PathCanonicalize(szFullPath, toadd))
				toadd = szFullPath;

				// SLUGFILLER: SafeHash remove - removed installation dir unsharing
				/*
				if (!IsShareableDirectory(toadd) )
					continue;
				*/

			if (_taccess(toadd, 0) == 0){ // only add directories which still exist
					if (toadd.Right(1) != L'\\')
						toadd.Append(L"\\");
				shareddir_list.AddHead(toadd);
			}
		}
		}
		catch (CFileException* ex) {
			ASSERT(0);
			ex->Delete();
		}
		sdirfile->Close();
	}
	delete sdirfile;
	delete[] fullpath;
	
	// serverlist addresses
	// filename update to reasonable name
	if (PathFileExists( configdir + L"adresses.dat") ) {
		if (PathFileExists( configdir + L"addresses.dat") )
			DeleteFile( configdir + L"adresses.dat");
		else 
			MoveFile( configdir + L"adresses.dat", configdir + L"addresses.dat");
	}

	fullpath = new TCHAR[_tcslen(configdir)+20];
	_stprintf(fullpath, L"%saddresses.dat", configdir);
	sdirfile = new CStdioFile();
	bIsUnicodeFile = IsUnicodeFile(fullpath);
	if (sdirfile->Open(fullpath, CFile::modeRead | CFile::shareDenyWrite | (bIsUnicodeFile ? CFile::typeBinary : 0)))
	{
		try {
			if (bIsUnicodeFile)
				sdirfile->Seek(sizeof(WORD), SEEK_CUR); // skip BOM

		CString toadd;
		while (sdirfile->ReadString(toadd))
			{
				toadd.Trim(L"\r\n"); // need to trim '\r' in binary mode
				addresses_list.AddHead(toadd);
			}
		}
		catch (CFileException* ex) {
			ASSERT(0);
			ex->Delete();
		}
		sdirfile->Close();
	}
	delete sdirfile;
	delete[] fullpath;	
	fullpath=NULL;

	userhash[5] = 14;
	userhash[14] = 111;

	// Explicitly inform the user about errors with incoming/temp folders!
	if (!PathFileExists(GetIncomingDir()) && !::CreateDirectory(GetIncomingDir(),0)) {
		CString strError;
		strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetIncomingDir(), GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
		_stprintf(incomingdir,L"%sincoming",appdir);
		if (!PathFileExists(GetIncomingDir()) && !::CreateDirectory(GetIncomingDir(),0)){
			strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetIncomingDir(), GetErrorMessage(GetLastError()));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}
	if (!PathFileExists(GetTempDir()) && !::CreateDirectory(GetTempDir(),0)) {
		CString strError;
		strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
		
		tempdir.SetAt(0,appdir + L"temp" );
		if (!PathFileExists(GetTempDir()) && !::CreateDirectory(GetTempDir(),0)){
			strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}
	// khaos::kmod+ Source Lists directory
	for (int i=0;i<thePrefs.tempdir.GetCount();i++) { // leuk_he: multiple temp dirs for save sources. 
	   CString sSourceListsPath = CString(thePrefs.GetTempDir(i)) + _T("\\Source Lists");
	   if (UseSaveLoadSources() && !PathFileExists(sSourceListsPath.GetBuffer()) && !::CreateDirectory(sSourceListsPath.GetBuffer(), 0)) {
	  	  CString strError;
		  strError.Format(_T("Failed to create source lists directory \"%s\" - %s"), sSourceListsPath, GetErrorMessage(GetLastError()));
		  AfxMessageBox(strError, MB_ICONERROR);
	   }
	}
	// khaos::kmod-

	// Create 'skins' directory
	if (!PathFileExists(GetSkinProfileDir()) && !CreateDirectory(GetSkinProfileDir(), 0)) {
		m_strSkinProfileDir = appdir + L"skins";
		CreateDirectory(GetSkinProfileDir(), 0);
	}

	// Create 'toolbars' directory
	if (!PathFileExists(GetToolbarBitmapFolderSettings()) && !CreateDirectory(GetToolbarBitmapFolderSettings(), 0)) {
		m_sToolbarBitmapFolder = appdir + L"skins";
		CreateDirectory(GetToolbarBitmapFolderSettings(), 0);
	}


	if (((int*)userhash[0]) == 0 && ((int*)userhash[1]) == 0 && ((int*)userhash[2]) == 0 && ((int*)userhash[3]) == 0)
		CreateUserHash();
}

void CPreferences::Uninit()
{
	while (!catMap.IsEmpty())
	{
		Category_Struct* delcat = catMap.GetAt(0); 
		catMap.RemoveAt(0); 
		delete delcat;
	}
}

void CPreferences::SetStandartValues()
{
	CreateUserHash();

	WINDOWPLACEMENT defaultWPM;
	defaultWPM.length = sizeof(WINDOWPLACEMENT);
	defaultWPM.rcNormalPosition.left=10;defaultWPM.rcNormalPosition.top=10;
	defaultWPM.rcNormalPosition.right=700;defaultWPM.rcNormalPosition.bottom=500;
	defaultWPM.showCmd=0;
	EmuleWindowPlacement=defaultWPM;
	versioncheckLastAutomatic=0;
	//MORPH START - Added by SiRoB, New Version check
	mversioncheckLastAutomatic=0;
	//MORPH END   - Added by SiRoB, New Version check
//	Save();
}

// SLUGFILLER: SafeHash remove - global form of IsTempFile unnececery
/*
bool CPreferences::IsTempFile(const CString& rstrDirectory, const CString& rstrName)
{
	bool bFound = false;
	for (int i=0;i<tempdir.GetCount() && !bFound;i++)
		if (CompareDirectories(rstrDirectory, GetTempDir(i))==0)
			bFound = true; //ok, found a directory
	
	if(!bFound) //found nowhere - not a tempfile...
			return false;

	// do not share a file from the temp directory, if it matches one of the following patterns
	CString strNameLower(rstrName);
	strNameLower.MakeLower();
	strNameLower += L"|"; // append an EOS character which we can query for
	static const LPCTSTR _apszNotSharedExts[] = {
		L"%u.part" L"%c", 
		L"%u.part.met" L"%c", 
		L"%u.part.met" PARTMET_BAK_EXT L"%c", 
		L"%u.part.met" PARTMET_TMP_EXT L"%c" 
	};
	for (int i = 0; i < ARRSIZE(_apszNotSharedExts); i++){
		UINT uNum;
		TCHAR iChar;
		// "misuse" the 'scanf' function for a very simple pattern scanning.
		if (_stscanf(strNameLower, _apszNotSharedExts[i], &uNum, &iChar) == 2 && iChar == L'|')
			return true;
	}

	return false;
}
*/

// SLUGFILLER: SafeHash
bool CPreferences::IsConfigFile(const CString& rstrDirectory, const CString& rstrName)
{
	if (CompareDirectories(rstrDirectory, configdir))
		return false;

	// do not share a file from the config directory, if it contains one of the following extensions
	static const LPCTSTR _apszNotSharedExts[] = { L".met.bak", L".ini.old" };
	for (int i = 0; i < ARRSIZE(_apszNotSharedExts); i++){
		int iLen = _tcslen(_apszNotSharedExts[i]);
		if (rstrName.GetLength()>=iLen && rstrName.Right(iLen).CompareNoCase(_apszNotSharedExts[i])==0)
			return true;
	}

	// do not share following files from the config directory
	static const LPCTSTR _apszNotSharedFiles[] = 
	{
		L"AC_SearchStrings.dat",
		L"AC_ServerMetURLs.dat",
		L"addresses.dat",
		L"category.ini",
		L"clients.met",
		L"cryptkey.dat",
		L"emfriends.met",
		L"fileinfo.ini",
		L"ipfilter.dat",
		L"known.met",
		L"preferences.dat",
		L"preferences.ini",
		L"server.met",
		L"server.met.new",
		L"server_met.download",
		L"server_met.old",
		L"shareddir.dat",
		L"sharedsubdir.dat",
		L"staticservers.dat",
		L"webservices.dat"
	};
	for (int i = 0; i < ARRSIZE(_apszNotSharedFiles); i++){
		if (rstrName.CompareNoCase(_apszNotSharedFiles[i])==0)
			return true;
	}

	return false;
}
// SLUGFILLER: SafeHash

//MORPH - Added by SiRoB, ZZ Ratio
uint8 CPreferences::IsZZRatioDoesWork(){
	uint8 ret = 0;
	if (theApp.downloadqueue->IsFilesPowershared())
		ret |= 1;
	if (theApp.friendlist->IsFriendSlot() && (thePrefs.GetGlobalDataRateFriend() > 3*1024 || thePrefs.GetGlobalDataRateFriend() == 0))
		ret |= 2;
	if (GetMaxUpload()<10)
		ret |= 4;
	if (theStats.GetAvgUploadRate(0)<10)
		ret |= 8;
	return ret;
}
//MORPH - Added by SiRoB, ZZ ratio

uint16 CPreferences::GetMaxDownload(){
    return (uint16)(GetMaxDownloadInBytesPerSec()/1024);
}

uint64 CPreferences::GetMaxDownloadInBytesPerSec(bool dynamic){
//dont be a Lam3r :)
	//MORPH START - Added by SiRoB, ZZ Upload system
	if (IsZZRatioDoesWork() || (dynamic && (thePrefs.IsDynUpEnabled() || thePrefs.IsSUCDoesWork())))
		return maxdownload*1024;
	//MORPH END   - Added by SiRoB, ZZ Upload system
    uint64 maxup;
    if(dynamic && thePrefs.IsDynUpEnabled() && theApp.uploadqueue->GetWaitingUserCount() != 0 && theApp.uploadqueue->GetDatarate() != 0) {
        maxup = theApp.uploadqueue->GetDatarate();
    } else {
        maxup = GetMaxUpload()*1024;
    }

	if( maxup < 4*1024 )
		return (( (maxup < 10*1024) && (maxup*3 < maxdownload*1024) )? maxup*3 : maxdownload*1024);
	return (( (maxup < 10*1024) && (maxup*4 < maxdownload*1024) )? maxup*4 : maxdownload*1024);
}

//MORPH START - Added by SiRoB, Upload Splitting Class
uint32	CPreferences::GetGlobalDataRateFriend()
{
	return globaldataratefriend*1024;//_UI32_MAX;
}
uint32	CPreferences::GetMaxGlobalDataRateFriend()
{
	return maxglobaldataratefriend;
}
uint32	CPreferences::GetMaxClientDataRateFriend()
{
	return maxclientdataratefriend*1024;
}
uint32	CPreferences::GetGlobalDataRatePowerShare()
{
	return globaldataratepowershare*1024;//_UI32_MAX;
}
uint32	CPreferences::GetMaxGlobalDataRatePowerShare()
{
	return maxglobaldataratepowershare;
}
uint32	CPreferences::GetMaxClientDataRatePowerShare()
{
	return maxclientdataratepowershare*1024;
}
uint32	CPreferences::GetMaxClientDataRate()
{
	return maxclientdatarate*1024;
}
//MORPH END   - Added by SiRoB, Upload Splitting Class
// -khaos--+++> A whole bunch of methods!  Keep going until you reach the end tag.
void CPreferences::SaveStats(int bBackUp){
	// This function saves all of the new statistics in my addon.  It is also used to
	// save backups for the Reset Stats function, and the Restore Stats function (Which is actually LoadStats)
	// bBackUp = 0: DEFAULT; save to statistics.ini
	// bBackUp = 1: Save to statbkup.ini, which is used to restore after a reset
	// bBackUp = 2: Save to statbkuptmp.ini, which is temporarily created during a restore and then renamed to statbkup.ini

	CString fullpath(configdir);
	if (bBackUp == 1)
		fullpath += L"statbkup.ini";
	else if (bBackUp == 2)
		fullpath += L"statbkuptmp.ini";
	else
		fullpath += L"statistics.ini";
	
	CIni ini(fullpath, L"Statistics");

	// Save cumulative statistics to preferences.ini, going in order as they appear in CStatisticsDlg::ShowStatistics.
	// We do NOT SET the values in prefs struct here.

    // Save Cum Down Data
		ini.WriteUInt64(L"TotalDownloadedBytes", theStats.sessionReceivedBytes + GetTotalDownloaded());
	ini.WriteInt(L"DownSuccessfulSessions", cumDownSuccessfulSessions);
	ini.WriteInt(L"DownFailedSessions", cumDownFailedSessions);
	ini.WriteInt(L"DownAvgTime", (GetDownC_AvgTime() + GetDownS_AvgTime()) / 2);
	ini.WriteUInt64(L"LostFromCorruption", cumLostFromCorruption + sesLostFromCorruption);
	ini.WriteUInt64(L"SavedFromCompression", sesSavedFromCompression + cumSavedFromCompression);
	ini.WriteInt(L"PartsSavedByICH", cumPartsSavedByICH + sesPartsSavedByICH);

	ini.WriteUInt64(L"DownData_EDONKEY", GetCumDownData_EDONKEY());
	ini.WriteUInt64(L"DownData_EDONKEYHYBRID", GetCumDownData_EDONKEYHYBRID());
	ini.WriteUInt64(L"DownData_EMULE", GetCumDownData_EMULE());
	ini.WriteUInt64(L"DownData_MLDONKEY", GetCumDownData_MLDONKEY());
	ini.WriteUInt64(L"DownData_LMULE", GetCumDownData_EMULECOMPAT());
	ini.WriteUInt64(L"DownData_AMULE", GetCumDownData_AMULE());
	ini.WriteUInt64(L"DownData_SHAREAZA", GetCumDownData_SHAREAZA());
	ini.WriteUInt64(L"DownData_URL", GetCumDownData_URL());
	ini.WriteUInt64(L"DownDataPort_4662", GetCumDownDataPort_4662());
	ini.WriteUInt64(L"DownDataPort_OTHER", GetCumDownDataPort_OTHER());
	ini.WriteUInt64(L"DownDataPort_PeerCache", GetCumDownDataPort_PeerCache());
	// MORPH START - Added by Commander, WebCache 1.2e
	ini.WriteUInt64(L"DownData_WEBCACHE", GetCumDownData_WEBCACHE()); // Superlexx - webcache - statistics
	// MORPH END - Added by Commander, WebCache 1.2e
	ini.WriteUInt64(_T("DownDataPort_4662"), GetCumDownDataPort_4662());
	ini.WriteUInt64(_T("DownDataPort_OTHER"), GetCumDownDataPort_OTHER());
	ini.WriteUInt64(_T("DownDataPort_PeerCache"), GetCumDownDataPort_PeerCache());

	ini.WriteUInt64(L"DownOverheadTotal",theStats.GetDownDataOverheadFileRequest() +
										theStats.GetDownDataOverheadSourceExchange() +
										theStats.GetDownDataOverheadServer() +
										theStats.GetDownDataOverheadKad() +
										theStats.GetDownDataOverheadOther() +
										GetDownOverheadTotal());
	ini.WriteUInt64(L"DownOverheadFileReq", theStats.GetDownDataOverheadFileRequest() + GetDownOverheadFileReq());
	ini.WriteUInt64(L"DownOverheadSrcEx", theStats.GetDownDataOverheadSourceExchange() + GetDownOverheadSrcEx());
	ini.WriteUInt64(L"DownOverheadServer", theStats.GetDownDataOverheadServer() + GetDownOverheadServer());
	ini.WriteUInt64(L"DownOverheadKad", theStats.GetDownDataOverheadKad() + GetDownOverheadKad());
	
	ini.WriteUInt64(L"DownOverheadTotalPackets", theStats.GetDownDataOverheadFileRequestPackets() + 
												theStats.GetDownDataOverheadSourceExchangePackets() + 
												theStats.GetDownDataOverheadServerPackets() + 
												theStats.GetDownDataOverheadKadPackets() + 
												theStats.GetDownDataOverheadOtherPackets() + 
												GetDownOverheadTotalPackets());
	ini.WriteUInt64(L"DownOverheadFileReqPackets", theStats.GetDownDataOverheadFileRequestPackets() + GetDownOverheadFileReqPackets());
	ini.WriteUInt64(L"DownOverheadSrcExPackets", theStats.GetDownDataOverheadSourceExchangePackets() + GetDownOverheadSrcExPackets());
	ini.WriteUInt64(L"DownOverheadServerPackets", theStats.GetDownDataOverheadServerPackets() + GetDownOverheadServerPackets());
	ini.WriteUInt64(L"DownOverheadKadPackets", theStats.GetDownDataOverheadKadPackets() + GetDownOverheadKadPackets());

	// Save Cumulative Upline Statistics
	ini.WriteUInt64(L"TotalUploadedBytes", theStats.sessionSentBytes + GetTotalUploaded());
	ini.WriteInt(L"UpSuccessfulSessions", theApp.uploadqueue->GetSuccessfullUpCount() + GetUpSuccessfulSessions());
	ini.WriteInt(L"UpFailedSessions", theApp.uploadqueue->GetFailedUpCount() + GetUpFailedSessions());
	ini.WriteInt(L"UpAvgTime", (theApp.uploadqueue->GetAverageUpTime() + GetUpAvgTime())/2);
	ini.WriteUInt64(L"UpData_EDONKEY", GetCumUpData_EDONKEY());
	ini.WriteUInt64(L"UpData_EDONKEYHYBRID", GetCumUpData_EDONKEYHYBRID());
	ini.WriteUInt64(L"UpData_EMULE", GetCumUpData_EMULE());
	ini.WriteUInt64(L"UpData_MLDONKEY", GetCumUpData_MLDONKEY());
	ini.WriteUInt64(L"UpData_LMULE", GetCumUpData_EMULECOMPAT());
	ini.WriteUInt64(L"UpData_AMULE", GetCumUpData_AMULE());
	ini.WriteUInt64(L"UpData_SHAREAZA", GetCumUpData_SHAREAZA());
	ini.WriteUInt64(L"UpDataPort_4662", GetCumUpDataPort_4662());
	ini.WriteUInt64(L"UpDataPort_OTHER", GetCumUpDataPort_OTHER());
	ini.WriteUInt64(L"UpDataPort_PeerCache", GetCumUpDataPort_PeerCache());
	ini.WriteUInt64(L"UpData_File", GetCumUpData_File());
	ini.WriteUInt64(L"UpData_Partfile", GetCumUpData_Partfile());

	ini.WriteUInt64(L"UpOverheadTotal", theStats.GetUpDataOverheadFileRequest() + 
										theStats.GetUpDataOverheadSourceExchange() + 
										theStats.GetUpDataOverheadServer() + 
										theStats.GetUpDataOverheadKad() + 
										theStats.GetUpDataOverheadOther() + 
										GetUpOverheadTotal());
	ini.WriteUInt64(L"UpOverheadFileReq", theStats.GetUpDataOverheadFileRequest() + GetUpOverheadFileReq());
	ini.WriteUInt64(L"UpOverheadSrcEx", theStats.GetUpDataOverheadSourceExchange() + GetUpOverheadSrcEx());
	ini.WriteUInt64(L"UpOverheadServer", theStats.GetUpDataOverheadServer() + GetUpOverheadServer());
	ini.WriteUInt64(L"UpOverheadKad", theStats.GetUpDataOverheadKad() + GetUpOverheadKad());

	ini.WriteUInt64(L"UpOverheadTotalPackets", theStats.GetUpDataOverheadFileRequestPackets() + 
										theStats.GetUpDataOverheadSourceExchangePackets() + 
										theStats.GetUpDataOverheadServerPackets() + 
										theStats.GetUpDataOverheadKadPackets() + 
										theStats.GetUpDataOverheadOtherPackets() + 
										GetUpOverheadTotalPackets());
	ini.WriteUInt64(L"UpOverheadFileReqPackets", theStats.GetUpDataOverheadFileRequestPackets() + GetUpOverheadFileReqPackets());
	ini.WriteUInt64(L"UpOverheadSrcExPackets", theStats.GetUpDataOverheadSourceExchangePackets() + GetUpOverheadSrcExPackets());
	ini.WriteUInt64(L"UpOverheadServerPackets", theStats.GetUpDataOverheadServerPackets() + GetUpOverheadServerPackets());
	ini.WriteUInt64(L"UpOverheadKadPackets", theStats.GetUpDataOverheadKadPackets() + GetUpOverheadKadPackets());

	// Save Cumulative Connection Statistics
	float tempRate = 0.0F;

	// Download Rate Average
	tempRate = theStats.GetAvgDownloadRate(AVG_TOTAL);
	ini.WriteFloat(L"ConnAvgDownRate", tempRate);
	
	// Max Download Rate Average
	if (tempRate > GetConnMaxAvgDownRate())
		SetConnMaxAvgDownRate(tempRate);
	ini.WriteFloat(L"ConnMaxAvgDownRate", GetConnMaxAvgDownRate());
	
	// Max Download Rate
	tempRate = (float)theApp.downloadqueue->GetDatarate() / 1024;
	if (tempRate > GetConnMaxDownRate())
		SetConnMaxDownRate(tempRate);
	ini.WriteFloat(L"ConnMaxDownRate", GetConnMaxDownRate());
	
	// Upload Rate Average
	tempRate = theStats.GetAvgUploadRate(AVG_TOTAL);
	ini.WriteFloat(L"ConnAvgUpRate", tempRate);
	
	// Max Upload Rate Average
	if (tempRate > GetConnMaxAvgUpRate())
		SetConnMaxAvgUpRate(tempRate);
	ini.WriteFloat(L"ConnMaxAvgUpRate", GetConnMaxAvgUpRate());
	
	// Max Upload Rate
	tempRate = (float)theApp.uploadqueue->GetDatarate() / 1024;
	if (tempRate > GetConnMaxUpRate())
		SetConnMaxUpRate(tempRate);
	ini.WriteFloat(L"ConnMaxUpRate", GetConnMaxUpRate());
	
	// Overall Run Time
	ini.WriteInt(L"ConnRunTime", (UINT)((GetTickCount() - theStats.starttime)/1000 + GetConnRunTime()));
	
	// Number of Reconnects
	ini.WriteInt(L"ConnNumReconnects", (theStats.reconnects>0) ? (theStats.reconnects - 1 + GetConnNumReconnects()) : GetConnNumReconnects());
	
	// Average Connections
	if (theApp.serverconnect->IsConnected())
		ini.WriteInt(L"ConnAvgConnections", (UINT)((theApp.listensocket->GetAverageConnections() + cumConnAvgConnections)/2));
	
	// Peak Connections
	if (theApp.listensocket->GetPeakConnections() > cumConnPeakConnections)
		cumConnPeakConnections = theApp.listensocket->GetPeakConnections();
	ini.WriteInt(L"ConnPeakConnections", cumConnPeakConnections);
	
	// Max Connection Limit Reached
	if (theApp.listensocket->GetMaxConnectionReached() + cumConnMaxConnLimitReached > cumConnMaxConnLimitReached)
		ini.WriteInt(L"ConnMaxConnLimitReached", theApp.listensocket->GetMaxConnectionReached() + cumConnMaxConnLimitReached);
	
	// Time Stuff...
	ini.WriteInt(L"ConnTransferTime", GetConnTransferTime() + theStats.GetTransferTime());
	ini.WriteInt(L"ConnUploadTime", GetConnUploadTime() + theStats.GetUploadTime());
	ini.WriteInt(L"ConnDownloadTime", GetConnDownloadTime() + theStats.GetDownloadTime());
	ini.WriteInt(L"ConnServerDuration", GetConnServerDuration() + theStats.GetServerDuration());
	
	// Compare and Save Server Records
	uint32 servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile;
	float servocc;
	theApp.serverlist->GetStatus(servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile, servocc);
	
	if (servtotal - servfail > cumSrvrsMostWorkingServers)
		cumSrvrsMostWorkingServers = servtotal - servfail;
	ini.WriteInt(L"SrvrsMostWorkingServers", cumSrvrsMostWorkingServers);

	if (servtuser > cumSrvrsMostUsersOnline)
		cumSrvrsMostUsersOnline = servtuser;
	ini.WriteInt(L"SrvrsMostUsersOnline", cumSrvrsMostUsersOnline);

	if (servtfile > cumSrvrsMostFilesAvail)
		cumSrvrsMostFilesAvail = servtfile;
	ini.WriteInt(L"SrvrsMostFilesAvail", cumSrvrsMostFilesAvail);

	// Compare and Save Shared File Records
	if ((UINT)theApp.sharedfiles->GetCount() > cumSharedMostFilesShared)
		cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
	ini.WriteInt(L"SharedMostFilesShared", cumSharedMostFilesShared);

	uint64 bytesLargestFile = 0;
	uint64 allsize = theApp.sharedfiles->GetDatasize(bytesLargestFile);
	if (allsize > cumSharedLargestShareSize)
		cumSharedLargestShareSize = allsize;
	ini.WriteUInt64(L"SharedLargestShareSize", cumSharedLargestShareSize);
	if (bytesLargestFile > cumSharedLargestFileSize)
		cumSharedLargestFileSize = bytesLargestFile;
	ini.WriteUInt64(L"SharedLargestFileSize", cumSharedLargestFileSize);

	if (theApp.sharedfiles->GetCount() != 0) {
		uint64 tempint = allsize/theApp.sharedfiles->GetCount();
		if (tempint > cumSharedLargestAvgFileSize)
			cumSharedLargestAvgFileSize = tempint;
	}

	ini.WriteUInt64(L"SharedLargestAvgFileSize", cumSharedLargestAvgFileSize);
	ini.WriteInt(L"statsDateTimeLastReset", stat_datetimeLastReset);

	// If we are saving a back-up or a temporary back-up, return now.
	if (bBackUp != 0)
		return;
}

void CPreferences::SetRecordStructMembers() {

	// The purpose of this function is to be called from CStatisticsDlg::ShowStatistics()
	// This was easier than making a bunch of functions to interface with the record
	// members of the prefs struct from ShowStatistics.

	// This function is going to compare current values with previously saved records, and if
	// the current values are greater, the corresponding member of prefs will be updated.
	// We will not write to INI here, because this code is going to be called a lot more often
	// than SaveStats()  - Khaos

	CString buffer;

	// Servers
	uint32 servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile;
	float servocc;
	theApp.serverlist->GetStatus( servtotal, servfail, servuser, servfile, servlowiduser, servtuser, servtfile, servocc );
	if ((servtotal-servfail)>cumSrvrsMostWorkingServers) cumSrvrsMostWorkingServers = (servtotal-servfail);
	if (servtuser>cumSrvrsMostUsersOnline) cumSrvrsMostUsersOnline = servtuser;
	if (servtfile>cumSrvrsMostFilesAvail) cumSrvrsMostFilesAvail = servtfile;

	// Shared Files
	if ((UINT)theApp.sharedfiles->GetCount() > cumSharedMostFilesShared)
		cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
	uint64 bytesLargestFile = 0;
	uint64 allsize=theApp.sharedfiles->GetDatasize(bytesLargestFile);
	if (allsize>cumSharedLargestShareSize) cumSharedLargestShareSize = allsize;
	if (bytesLargestFile>cumSharedLargestFileSize) cumSharedLargestFileSize = bytesLargestFile;
	if (theApp.sharedfiles->GetCount() != 0) {
		uint64 tempint = allsize/theApp.sharedfiles->GetCount();
		if (tempint>cumSharedLargestAvgFileSize) cumSharedLargestAvgFileSize = tempint;
	}
} // SetRecordStructMembers()

void CPreferences::SaveCompletedDownloadsStat(){

	// This function saves the values for the completed
	// download members to INI.  It is called from
	// CPartfile::PerformFileComplete ...   - Khaos

	TCHAR* fullpath = new TCHAR[_tcslen(configdir)+MAX_PATH]; // i_a
	_stprintf(fullpath,L"%sstatistics.ini",configdir);
	
	CIni ini( fullpath, L"Statistics" );

	delete[] fullpath;

	ini.WriteInt(L"DownCompletedFiles",			GetDownCompletedFiles());
	ini.WriteInt(L"DownSessionCompletedFiles",	GetDownSessionCompletedFiles());
} // SaveCompletedDownloadsStat()

void CPreferences::Add2SessionTransferData(UINT uClientID, UINT uClientPort, BOOL bFromPF, 
										   BOOL bUpDown, uint32 bytes, bool sentToFriend)
{
	//	This function adds the transferred bytes to the appropriate variables,
	//	as well as to the totals for all clients. - Khaos
	//	PARAMETERS:
	//	uClientID - The identifier for which client software sent or received this data, eg SO_EMULE
	//	uClientPort - The remote port of the client that sent or received this data, eg 4662
	//	bFromPF - Applies only to uploads.  True is from partfile, False is from non-partfile.
	//	bUpDown - True is Up, False is Down
	//	bytes - Number of bytes sent by the client.  Subtract header before calling.

	switch (bUpDown){
		case true:
			//	Upline Data
			switch (uClientID){
				// Update session client breakdown stats for sent bytes...
				case SO_EMULE:
				case SO_OLDEMULE:		sesUpData_EMULE+=bytes;			break;
				case SO_EDONKEYHYBRID:	sesUpData_EDONKEYHYBRID+=bytes;	break;
				case SO_EDONKEY:		sesUpData_EDONKEY+=bytes;		break;
				case SO_MLDONKEY:		sesUpData_MLDONKEY+=bytes;		break;
				case SO_AMULE:			sesUpData_AMULE+=bytes;			break;
				case SO_SHAREAZA:		sesUpData_SHAREAZA+=bytes;		break;
				case SO_CDONKEY:
				case SO_LPHANT:
				case SO_XMULE:			sesUpData_EMULECOMPAT+=bytes;	break;
			}

			switch (uClientPort){
				// Update session port breakdown stats for sent bytes...
				case 4662:				sesUpDataPort_4662+=bytes;		break;
				case (UINT)-1:			sesUpDataPort_PeerCache+=bytes;	break;
				//case (UINT)-2:		sesUpDataPort_URL+=bytes;		break;
				default:				sesUpDataPort_OTHER+=bytes;		break;
			}

			if (bFromPF)				sesUpData_Partfile+=bytes;
			else						sesUpData_File+=bytes;

			//	Add to our total for sent bytes...
			theApp.UpdateSentBytes(bytes, sentToFriend);

			break;

		case false:
			// Downline Data
			switch (uClientID){
                // Update session client breakdown stats for received bytes...
				case SO_EMULE:
				case SO_OLDEMULE:		sesDownData_EMULE+=bytes;		break;
				case SO_EDONKEYHYBRID:	sesDownData_EDONKEYHYBRID+=bytes;break;
				case SO_EDONKEY:		sesDownData_EDONKEY+=bytes;		break;
				case SO_MLDONKEY:		sesDownData_MLDONKEY+=bytes;	break;
				case SO_AMULE:			sesDownData_AMULE+=bytes;		break;
				case SO_SHAREAZA:		sesDownData_SHAREAZA+=bytes;	break;
				case SO_CDONKEY:
				case SO_LPHANT:
				case SO_XMULE:			sesDownData_EMULECOMPAT+=bytes;	break;
				case SO_URL:			sesDownData_URL+=bytes;			break;

				// MORPH START - Added by Commander, WebCache 1.2e
				case SO_WEBCACHE:		sesDownData_WEBCACHE+=bytes;	break; // Superlexx - webcache - statistics
				// MORPH END - Added by Commander, WebCache 1.2e
			}

			switch (uClientPort){
				// Update session port breakdown stats for received bytes...
				// For now we are only going to break it down by default and non-default.
				// A statistical analysis of all data sent from every single port/domain is
				// beyond the scope of this add-on.
				case 4662:				sesDownDataPort_4662+=bytes;	break;
				case (UINT)-1:			sesDownDataPort_PeerCache+=bytes;break;
				//case (UINT)-2:		sesDownDataPort_URL+=bytes;		break;
				default:				sesDownDataPort_OTHER+=bytes;	break;
			}

			//	Add to our total for received bytes...
			theApp.UpdateReceivedBytes(bytes);
	}
}

// Reset Statistics by Khaos

void CPreferences::ResetCumulativeStatistics(){

	// Save a backup so that we can undo this action
	SaveStats(1);

	// SET ALL CUMULATIVE STAT VALUES TO 0  :'-(

	totalDownloadedBytes=0;
	totalUploadedBytes=0;
	cumDownOverheadTotal=0;
	cumDownOverheadFileReq=0;
	cumDownOverheadSrcEx=0;
	cumDownOverheadServer=0;
	cumDownOverheadKad=0;
	cumDownOverheadTotalPackets=0;
	cumDownOverheadFileReqPackets=0;
	cumDownOverheadSrcExPackets=0;
	cumDownOverheadServerPackets=0;
	cumDownOverheadKadPackets=0;
	cumUpOverheadTotal=0;
	cumUpOverheadFileReq=0;
	cumUpOverheadSrcEx=0;
	cumUpOverheadServer=0;
	cumUpOverheadKad=0;
	cumUpOverheadTotalPackets=0;
	cumUpOverheadFileReqPackets=0;
	cumUpOverheadSrcExPackets=0;
	cumUpOverheadServerPackets=0;
	cumUpOverheadKadPackets=0;
	cumUpSuccessfulSessions=0;
	cumUpFailedSessions=0;
	cumUpAvgTime=0;
	cumUpData_EDONKEY=0;
	cumUpData_EDONKEYHYBRID=0;
	cumUpData_EMULE=0;
	cumUpData_MLDONKEY=0;
	cumUpData_AMULE=0;
	cumUpData_EMULECOMPAT=0;
	cumUpData_SHAREAZA=0;
	cumUpDataPort_4662=0;
	cumUpDataPort_OTHER=0;
	cumUpDataPort_PeerCache=0;
	cumDownCompletedFiles=0;
	cumDownSuccessfulSessions=0;
	cumDownFailedSessions=0;
	cumDownAvgTime=0;
	cumLostFromCorruption=0;
	cumSavedFromCompression=0;
	cumPartsSavedByICH=0;
	cumDownData_EDONKEY=0;
	cumDownData_EDONKEYHYBRID=0;
	cumDownData_EMULE=0;
	cumDownData_MLDONKEY=0;
	cumDownData_AMULE=0;
	cumDownData_EMULECOMPAT=0;
	cumDownData_SHAREAZA=0;
	cumDownData_URL=0;
	// MORPH START - Added by Commander, WebCache 1.2e
	cumDownData_WEBCACHE=0; // Superlexx - webcache - statistics
	// MORPH END - Added by Commander, WebCache 1.2e
	cumDownDataPort_4662=0;
	cumDownDataPort_OTHER=0;
	cumDownDataPort_PeerCache=0;
	cumConnAvgDownRate=0;
	cumConnMaxAvgDownRate=0;
	cumConnMaxDownRate=0;
	cumConnAvgUpRate=0;
	cumConnRunTime=0;
	cumConnNumReconnects=0;
	cumConnAvgConnections=0;
	cumConnMaxConnLimitReached=0;
	cumConnPeakConnections=0;
	cumConnDownloadTime=0;
	cumConnUploadTime=0;
	cumConnTransferTime=0;
	cumConnServerDuration=0;
	cumConnMaxAvgUpRate=0;
	cumConnMaxUpRate=0;
	cumSrvrsMostWorkingServers=0;
	cumSrvrsMostUsersOnline=0;
	cumSrvrsMostFilesAvail=0;
    cumSharedMostFilesShared=0;
	cumSharedLargestShareSize=0;
	cumSharedLargestAvgFileSize=0;

	// Set the time of last reset...
	time_t timeNow;
	time(&timeNow);
	stat_datetimeLastReset = timeNow;

	// Save the reset stats
	SaveStats();
	theApp.emuledlg->statisticswnd->ShowStatistics(true);
}


// Load Statistics
// This used to be integrated in LoadPreferences, but it has been altered
// so that it can be used to load the backup created when the stats are reset.
// Last Modified: 2-22-03 by Khaos
bool CPreferences::LoadStats(int loadBackUp)
{
	// loadBackUp is 0 by default
	// loadBackUp = 0: Load the stats normally like we used to do in LoadPreferences
	// loadBackUp = 1: Load the stats from statbkup.ini and create a backup of the current stats.  Also, do not initialize session variables.
	CString sINI;
	CFileFind findBackUp;

	switch (loadBackUp) {
		case 0:{
			// for transition...
			if(PathFileExists(configdir+L"statistics.ini"))
				sINI.Format(L"%sstatistics.ini", configdir);
			else
				sINI.Format(L"%spreferences.ini", configdir);

			break;
			   }
		case 1:
			sINI.Format(L"%sstatbkup.ini", configdir);
			if (!findBackUp.FindFile(sINI))
				return false;
			SaveStats(2); // Save our temp backup of current values to statbkuptmp.ini, we will be renaming it at the end of this function.
			break;
	}

	BOOL fileex = PathFileExists(sINI);
	CIni ini(sINI, L"Statistics");

	totalDownloadedBytes			= ini.GetUInt64(L"TotalDownloadedBytes");
	totalUploadedBytes				= ini.GetUInt64(L"TotalUploadedBytes");

	// Load stats for cumulative downline overhead
	cumDownOverheadTotal			= ini.GetUInt64(L"DownOverheadTotal");
	cumDownOverheadFileReq			= ini.GetUInt64(L"DownOverheadFileReq");
	cumDownOverheadSrcEx			= ini.GetUInt64(L"DownOverheadSrcEx");
	cumDownOverheadServer			= ini.GetUInt64(L"DownOverheadServer");
	cumDownOverheadKad				= ini.GetUInt64(L"DownOverheadKad");
	cumDownOverheadTotalPackets		= ini.GetUInt64(L"DownOverheadTotalPackets");
	cumDownOverheadFileReqPackets	= ini.GetUInt64(L"DownOverheadFileReqPackets");
	cumDownOverheadSrcExPackets		= ini.GetUInt64(L"DownOverheadSrcExPackets");
	cumDownOverheadServerPackets	= ini.GetUInt64(L"DownOverheadServerPackets");
	cumDownOverheadKadPackets		= ini.GetUInt64(L"DownOverheadKadPackets");

	// Load stats for cumulative upline overhead
	cumUpOverheadTotal				= ini.GetUInt64(L"UpOverHeadTotal");
	cumUpOverheadFileReq			= ini.GetUInt64(L"UpOverheadFileReq");
	cumUpOverheadSrcEx				= ini.GetUInt64(L"UpOverheadSrcEx");
	cumUpOverheadServer				= ini.GetUInt64(L"UpOverheadServer");
	cumUpOverheadKad				= ini.GetUInt64(L"UpOverheadKad");
	cumUpOverheadTotalPackets		= ini.GetUInt64(L"UpOverHeadTotalPackets");
	cumUpOverheadFileReqPackets		= ini.GetUInt64(L"UpOverheadFileReqPackets");
	cumUpOverheadSrcExPackets		= ini.GetUInt64(L"UpOverheadSrcExPackets");
	cumUpOverheadServerPackets		= ini.GetUInt64(L"UpOverheadServerPackets");
	cumUpOverheadKadPackets			= ini.GetUInt64(L"UpOverheadKadPackets");

	// Load stats for cumulative upline data
	cumUpSuccessfulSessions			= ini.GetInt(L"UpSuccessfulSessions");
	cumUpFailedSessions				= ini.GetInt(L"UpFailedSessions");
	cumUpAvgTime					= ini.GetInt(L"UpAvgTime");

	// Load cumulative client breakdown stats for sent bytes
	cumUpData_EDONKEY				= ini.GetUInt64(L"UpData_EDONKEY");
	cumUpData_EDONKEYHYBRID			= ini.GetUInt64(L"UpData_EDONKEYHYBRID");
	cumUpData_EMULE					= ini.GetUInt64(L"UpData_EMULE");
	cumUpData_MLDONKEY				= ini.GetUInt64(L"UpData_MLDONKEY");
	cumUpData_EMULECOMPAT			= ini.GetUInt64(L"UpData_LMULE");
	cumUpData_AMULE					= ini.GetUInt64(L"UpData_AMULE");
	cumUpData_SHAREAZA				= ini.GetUInt64(L"UpData_SHAREAZA");

	// Load cumulative port breakdown stats for sent bytes
	cumUpDataPort_4662				= ini.GetUInt64(L"UpDataPort_4662");
	cumUpDataPort_OTHER				= ini.GetUInt64(L"UpDataPort_OTHER");
	cumUpDataPort_PeerCache			= ini.GetUInt64(L"UpDataPort_PeerCache");

	// Load cumulative source breakdown stats for sent bytes
	cumUpData_File					= ini.GetUInt64(L"UpData_File");
	cumUpData_Partfile				= ini.GetUInt64(L"UpData_Partfile");

	// Load stats for cumulative downline data
	cumDownCompletedFiles			= ini.GetInt(L"DownCompletedFiles");
	cumDownSuccessfulSessions		= ini.GetInt(L"DownSuccessfulSessions");
	cumDownFailedSessions			= ini.GetInt(L"DownFailedSessions");
	cumDownAvgTime					= ini.GetInt(L"DownAvgTime");

	// Cumulative statistics for saved due to compression/lost due to corruption
	cumLostFromCorruption			= ini.GetUInt64(L"LostFromCorruption");
	cumSavedFromCompression			= ini.GetUInt64(L"SavedFromCompression");
	cumPartsSavedByICH				= ini.GetInt(L"PartsSavedByICH");

	// Load cumulative client breakdown stats for received bytes
	cumDownData_EDONKEY				= ini.GetUInt64(L"DownData_EDONKEY");
	cumDownData_EDONKEYHYBRID		= ini.GetUInt64(L"DownData_EDONKEYHYBRID");
	cumDownData_EMULE				= ini.GetUInt64(L"DownData_EMULE");
	cumDownData_MLDONKEY			= ini.GetUInt64(L"DownData_MLDONKEY");
	cumDownData_EMULECOMPAT			= ini.GetUInt64(L"DownData_LMULE");
	cumDownData_AMULE				= ini.GetUInt64(L"DownData_AMULE");
	cumDownData_SHAREAZA			= ini.GetUInt64(L"DownData_SHAREAZA");
	cumDownData_URL					= ini.GetUInt64(L"DownData_URL");
	// MORPH START - Added by Commander, WebCache 1.2e
	cumDownData_WEBCACHE			= ini.GetUInt64(_T("DownData_WEBCACHE")); // Superlexx - webcache - statistics
	// MORPH END - Added by Commander, WebCache 1.2e

	// Load cumulative port breakdown stats for received bytes
	cumDownDataPort_4662			= ini.GetUInt64(L"DownDataPort_4662");
	cumDownDataPort_OTHER			= ini.GetUInt64(L"DownDataPort_OTHER");
	cumDownDataPort_PeerCache		= ini.GetUInt64(L"DownDataPort_PeerCache");

	// Load stats for cumulative connection data
	cumConnAvgDownRate				= ini.GetFloat(L"ConnAvgDownRate");
	cumConnMaxAvgDownRate			= ini.GetFloat(L"ConnMaxAvgDownRate");
	cumConnMaxDownRate				= ini.GetFloat(L"ConnMaxDownRate");
	cumConnAvgUpRate				= ini.GetFloat(L"ConnAvgUpRate");
	cumConnMaxAvgUpRate				= ini.GetFloat(L"ConnMaxAvgUpRate");
	cumConnMaxUpRate				= ini.GetFloat(L"ConnMaxUpRate");
	cumConnRunTime					= ini.GetInt(L"ConnRunTime");
	cumConnTransferTime				= ini.GetInt(L"ConnTransferTime");
	cumConnDownloadTime				= ini.GetInt(L"ConnDownloadTime");
	cumConnUploadTime				= ini.GetInt(L"ConnUploadTime");
	cumConnServerDuration			= ini.GetInt(L"ConnServerDuration");
	cumConnNumReconnects			= ini.GetInt(L"ConnNumReconnects");
	cumConnAvgConnections			= ini.GetInt(L"ConnAvgConnections");
	cumConnMaxConnLimitReached		= ini.GetInt(L"ConnMaxConnLimitReached");
	cumConnPeakConnections			= ini.GetInt(L"ConnPeakConnections");

	// Load date/time of last reset
	stat_datetimeLastReset			= ini.GetInt(L"statsDateTimeLastReset");

	// Smart Load For Restores - Don't overwrite records that are greater than the backed up ones
	if (loadBackUp == 1)
	{
		// Load records for servers / network
		if ((UINT)ini.GetInt(L"SrvrsMostWorkingServers") > cumSrvrsMostWorkingServers)
			cumSrvrsMostWorkingServers = ini.GetInt(L"SrvrsMostWorkingServers");

		if ((UINT)ini.GetInt(L"SrvrsMostUsersOnline") > cumSrvrsMostUsersOnline)
			cumSrvrsMostUsersOnline = ini.GetInt(L"SrvrsMostUsersOnline");

		if ((UINT)ini.GetInt(L"SrvrsMostFilesAvail") > cumSrvrsMostFilesAvail)
			cumSrvrsMostFilesAvail = ini.GetInt(L"SrvrsMostFilesAvail");

		// Load records for shared files
		if ((UINT)ini.GetInt(L"SharedMostFilesShared") > cumSharedMostFilesShared)
			cumSharedMostFilesShared =	ini.GetInt(L"SharedMostFilesShared");

		uint64 temp64 = ini.GetUInt64(L"SharedLargestShareSize");
		if (temp64 > cumSharedLargestShareSize)
			cumSharedLargestShareSize = temp64;

		temp64 = ini.GetUInt64(L"SharedLargestAvgFileSize");
		if (temp64 > cumSharedLargestAvgFileSize)
			cumSharedLargestAvgFileSize = temp64;

		temp64 = ini.GetUInt64(L"SharedLargestFileSize");
		if (temp64 > cumSharedLargestFileSize)
			cumSharedLargestFileSize = temp64;

		// Check to make sure the backup of the values we just overwrote exists.  If so, rename it to the backup file.
		// This allows us to undo a restore, so to speak, just in case we don't like the restored values...
		CString sINIBackUp;
		sINIBackUp.Format(L"%sstatbkuptmp.ini", configdir);
		if (findBackUp.FindFile(sINIBackUp)){
			CFile::Remove(sINI);				// Remove the backup that we just restored from
			CFile::Rename(sINIBackUp, sINI);	// Rename our temporary backup to the normal statbkup.ini filename.
		}

		// Since we know this is a restore, now we should call ShowStatistics to update the data items to the new ones we just loaded.
		// Otherwise user is left waiting around for the tick counter to reach the next automatic update (Depending on setting in prefs)
		theApp.emuledlg->statisticswnd->ShowStatistics();
	}
	// Stupid Load -> Just load the values.
	else
	{
		// Load records for servers / network
		cumSrvrsMostWorkingServers	= ini.GetInt(L"SrvrsMostWorkingServers");
		cumSrvrsMostUsersOnline		= ini.GetInt(L"SrvrsMostUsersOnline");
		cumSrvrsMostFilesAvail		= ini.GetInt(L"SrvrsMostFilesAvail");

		// Load records for shared files
		cumSharedMostFilesShared	= ini.GetInt(L"SharedMostFilesShared");
		cumSharedLargestShareSize	= ini.GetUInt64(L"SharedLargestShareSize");
		cumSharedLargestAvgFileSize = ini.GetUInt64(L"SharedLargestAvgFileSize");
		cumSharedLargestFileSize	= ini.GetUInt64(L"SharedLargestFileSize");

		// Initialize new session statistic variables...
		sesDownCompletedFiles		= 0;
		
		sesUpData_EDONKEY			= 0;
		sesUpData_EDONKEYHYBRID		= 0;
		sesUpData_EMULE				= 0;
		sesUpData_MLDONKEY			= 0;
		sesUpData_AMULE				= 0;
		sesUpData_EMULECOMPAT		= 0;
		sesUpData_SHAREAZA			= 0;
		sesUpDataPort_4662			= 0;
		sesUpDataPort_OTHER			= 0;
		sesUpDataPort_PeerCache		= 0;

		sesDownData_EDONKEY			= 0;
		sesDownData_EDONKEYHYBRID	= 0;
		sesDownData_EMULE			= 0;
		sesDownData_MLDONKEY		= 0;
		sesDownData_AMULE			= 0;
		sesDownData_EMULECOMPAT		= 0;
		sesDownData_SHAREAZA		= 0;
		sesDownData_URL				= 0;

		// MORPH START - Added by Commander, WebCache 1.2e
		sesDownData_WEBCACHE		= 0; // Superlexx - webcache - statistics
		ses_WEBCACHEREQUESTS		= 0; //jp webcache statistics (from proxy)
		ses_successfull_WCDOWNLOADS	= 0; //jp webcache statistics (from proxy)
		ses_PROXYREQUESTS           = 0; //jp webcache statistics (via proxy)
		ses_successfullPROXYREQUESTS= 0; //jp webcache statistics (via proxy)
		// MORPH END - Added by Commander, WebCache 1.2e

		sesDownDataPort_4662		= 0;
		sesDownDataPort_OTHER		= 0;
		sesDownDataPort_PeerCache	= 0;

		sesDownSuccessfulSessions	= 0;
		sesDownFailedSessions		= 0;
		sesPartsSavedByICH			= 0;
	}

	if (!fileex || (stat_datetimeLastReset==0 && totalDownloadedBytes==0 && totalUploadedBytes==0))
	{
		time_t timeNow;
		time(&timeNow);
		stat_datetimeLastReset = timeNow;
	}

	return true;
}

// This formats the UTC long value that is saved for stat_datetimeLastReset
// If this value is 0 (Never reset), then it returns Unknown.
CString CPreferences::GetStatsLastResetStr(bool formatLong)
{
	// formatLong dictates the format of the string returned.
	// For example...
	// true: DateTime format from the .ini
	// false: DateTime format from the .ini for the log
	CString	returnStr;
	if (GetStatsLastResetLng()) {
		tm *statsReset;
		TCHAR szDateReset[128];
		time_t lastResetDateTime = (time_t) GetStatsLastResetLng();
		statsReset = localtime(&lastResetDateTime);
		if (statsReset){
			_tcsftime(szDateReset, ARRSIZE(szDateReset), formatLong ? GetDateTimeFormat() : L"%c", statsReset);
			returnStr = szDateReset;
		}
	}
	if (returnStr.IsEmpty())
		returnStr = GetResString(IDS_UNKNOWN);
	return returnStr;
}

// <-----khaos-

bool CPreferences::Save(){

	bool error = false;
	TCHAR* fullpath = new TCHAR[_tcslen(configdir)+MAX_PATH]; // i_a
	_stprintf(fullpath,L"%spreferences.dat",configdir);

	FILE* preffile = _tfsopen(fullpath,L"wb", _SH_DENYWR);
	delete[] fullpath;
	prefsExt->version = PREFFILE_VERSION;
	if (preffile){
		prefsExt->version=PREFFILE_VERSION;
		prefsExt->EmuleWindowPlacement=EmuleWindowPlacement;
		md4cpy(prefsExt->userhash, userhash);

		error = fwrite(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile)!=1;
		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			fflush(preffile); // flush file stream buffers to disk buffers
			(void)_commit(_fileno(preffile)); // commit disk buffers to disk
		}
		fclose(preffile);
	}
	else
		error = true;

	SavePreferences();
		SaveStats();
	// khaos::categorymod+ We need to SaveCats() each time we exit eMule.
	SaveCats();
	// khaos::categorymod-

	fullpath = new TCHAR[_tcslen(configdir)+14];
	_stprintf(fullpath, L"%sshareddir.dat", configdir);
	CStdioFile sdirfile;
	if (sdirfile.Open(fullpath, CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite | CFile::typeBinary))
	{
		try{
			// write Unicode byte-order mark 0xFEFF
			WORD wBOM = 0xFEFF;
			sdirfile.Write(&wBOM, sizeof(wBOM));

			for (POSITION pos = shareddir_list.GetHeadPosition();pos != 0;){
				sdirfile.WriteString(shareddir_list.GetNext(pos));
				sdirfile.Write(L"\r\n", sizeof(TCHAR)*2);
			}
			if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
				sdirfile.Flush(); // flush file stream buffers to disk buffers
				if (_commit(_fileno(sdirfile.m_pStream)) != 0) // commit disk buffers to disk
					AfxThrowFileException(CFileException::hardIO, GetLastError(), sdirfile.GetFileName());
			}
			sdirfile.Close();
		}
		catch(CFileException* error){
			TCHAR buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,ARRSIZE(buffer));
			if (thePrefs.GetVerbose())
				AddDebugLogLine(true,L"Failed to save %s - %s", fullpath, buffer);
			error->Delete();
		}
	}
	else
		error = true;
	delete[] fullpath;
	fullpath=NULL;

	::CreateDirectory(GetIncomingDir(),0);
	::CreateDirectory(GetTempDir(),0);
	return error;
}

void CPreferences::CreateUserHash()
{
	for (int i = 0; i < 8; i++)
	{
		uint16 random = GetRandomUInt16();
		memcpy(&userhash[i*2], &random, 2);
	}

	// mark as emule client. that will be need in later version
	userhash[5] = 14;
	userhash[14] = 111;
}

int CPreferences::GetRecommendedMaxConnections() {
	int iRealMax = ::GetMaxWindowsTCPConnections();
	if(iRealMax == -1 || iRealMax > 520)
		return 500;

	if(iRealMax < 20)
		return iRealMax;

	if(iRealMax <= 256)
		return iRealMax - 10;

	return iRealMax - 20;
}

void CPreferences::SavePreferences()
{
	CString buffer;
	
	CIni ini(GetConfigFile(), L"eMule");
	//---
	//MORPH START - Added by SiRoB, [itsonlyme: -modname-]
	/*
	ini.WriteString(L"AppVersion", theApp.m_strCurVersionLong);
	*/
	ini.WriteString(L"AppVersion", theApp.m_strCurVersionLong + L" [" + theApp.m_strModLongVersion + L"]");
	//MORPH END   - Added by SiRoB, [itsonlyme: -modname-]
	//---

#ifdef _DEBUG
	ini.WriteInt(L"DebugHeap", m_iDbgHeap);
#endif

	ini.WriteStringUTF8(L"Nick", strNick);
	ini.WriteString(L"IncomingDir", incomingdir);
	
	ini.WriteString(L"TempDir", tempdir.GetAt(0));

	CString tempdirs;
	for (int i=1;i<tempdir.GetCount();i++) {
		tempdirs.Append(tempdir.GetAt(i) );
		if (i+1<tempdir.GetCount())
			tempdirs.Append(L"|");
	}
	ini.WriteString(L"TempDirs", tempdirs);

    ini.WriteInt(L"MinUpload", minupload);
	ini.WriteInt(L"MaxUpload",maxupload);
	ini.WriteInt(L"MaxDownload",maxdownload);
	ini.WriteInt(L"MaxConnections",maxconnections);
	ini.WriteInt(L"MaxHalfConnections",maxhalfconnections);
	ini.WriteBool(L"ConditionalTCPAccept", m_bConditionalTCPAccept);
	ini.WriteInt(L"Port",port);
	ini.WriteInt(L"UDPPort",udpport);
	ini.WriteInt(L"ServerUDPPort", nServerUDPPort);
	// MORPH START - Added by Commander, WebCache 1.2e
	ini.WriteString(L"webcacheName", webcacheName);
	ini.WriteInt(L"webcachePort", webcachePort);
	ini.WriteInt(L"WebCacheBlockLimit", webcacheBlockLimit);
	ini.WriteBool(L"PersistentConnectionsForProxyDownloads", PersistentConnectionsForProxyDownloads); //JP persistent proxy connections
	ini.WriteBool(L"WCAutoupdate", WCAutoupdate); //JP WCAutoupdate
	ini.WriteBool(L"WebCacheExtraTimeout", webcacheExtraTimeout);
	ini.WriteBool(L"WebCacheCachesLocalTraffic", webcacheCachesLocalTraffic);
	ini.WriteBool(L"WebCacheEnabled", webcacheEnabled);
	ini.WriteBool(L"detectWebcacheOnStart", detectWebcacheOnStart); // jp detect webcache on startup
	ini.WriteUInt64(L"WebCacheLastSearch", (uint64)webcacheLastSearch);
	ini.WriteUInt64(L"WebCacheLastGlobalIP", (uint64)webcacheLastGlobalIP);
	ini.WriteString(L"WebCacheLastResolvedName", webcacheLastResolvedName);
	ini.WriteUInt64(L"webcacheTrustLevel", (uint64)webcacheTrustLevel);
// yonatan http end ////////////////////////////////////////////////////////////////////////////
	// MORPH END - Added by Commander, WebCache 1.2e
	ini.WriteInt(L"MaxSourcesPerFile",maxsourceperfile );
	ini.WriteWORD(L"Language",m_wLanguageID);
	ini.WriteInt(L"SeeShare",m_iSeeShares);
	ini.WriteInt(L"ToolTipDelay",m_iToolDelayTime);
	ini.WriteInt(L"StatGraphsInterval",trafficOMeterInterval);
	ini.WriteInt(L"StatsInterval",statsInterval);
	ini.WriteInt(L"DownloadCapacity",maxGraphDownloadRate);
	ini.WriteInt(L"UploadCapacityNew",maxGraphUploadRate);
	ini.WriteInt(L"DeadServerRetry",m_uDeadServerRetries);
	ini.WriteInt(L"ServerKeepAliveTimeout",m_dwServerKeepAliveTimeout);
	ini.WriteString(L"BindAddr",m_pszBindAddrW); //MORPH leuk_he bindaddr
	// MORPH START leuk_he upnp bindaddr
	ini.WriteString(L"UpnpBindAddr", ipstr(htonl(GetUpnpBindAddr())));
	ini.WriteBool(L"UpnpBindAddrDhcp",GetUpnpBindDhcp());
    // MORPH END leuk_he upnp bindaddr
	ini.WriteInt(L"SplitterbarPosition",splitterbarPosition+2);
	// Mighty Knife: What's the reason for this line ?!?!?
	// Why is 2 added here ?!?
	// ini.WriteInt(L"SplitterbarPosition",splitterbarPosition+2);
	ini.WriteInt(L"SplitterbarPosition",splitterbarPosition);
	// [end] Mighty Knife
	ini.WriteInt(L"SplitterbarPositionServer",splitterbarPositionSvr);
	ini.WriteInt(L"SplitterbarPositionStat",splitterbarPositionStat+1);
	ini.WriteInt(L"SplitterbarPositionStat_HL",splitterbarPositionStat_HL+1);
	ini.WriteInt(L"SplitterbarPositionStat_HR",splitterbarPositionStat_HR+1);
	ini.WriteInt(L"SplitterbarPositionFriend",splitterbarPositionFriend);
	ini.WriteInt(L"SplitterbarPositionIRC",splitterbarPositionIRC+2);
	ini.WriteInt(L"SplitterbarPositionShared",splitterbarPositionShared);
	ini.WriteInt(L"TransferWnd1",m_uTransferWnd1);
	ini.WriteInt(L"TransferWnd2",m_uTransferWnd2);
	ini.WriteInt(L"VariousStatisticsMaxValue",statsMax);
	ini.WriteInt(L"StatsAverageMinutes",statsAverageMinutes);
	ini.WriteInt(L"MaxConnectionsPerFiveSeconds",MaxConperFive);
	ini.WriteInt(L"Check4NewVersionDelay",versioncheckdays);

	ini.WriteBool(L"Reconnect",reconnect);
	ini.WriteBool(L"Scoresystem",m_bUseServerPriorities);
	ini.WriteBool(L"Serverlist",m_bAutoUpdateServerList);
	ini.WriteBool(L"UpdateNotifyTestClient",updatenotify);
	ini.WriteBool(L"MinToTray",mintotray);
	ini.WriteBool(L"AddServersFromServer",m_bAddServersFromServer);
	ini.WriteBool(L"AddServersFromClient",m_bAddServersFromClients);
	ini.WriteBool(L"Splashscreen",splashscreen);
	ini.WriteBool(L"Startupsound",startupsound);//Commander - Added: Enable/Disable Startupsound
	ini.WriteBool(L"Sidebanner",sidebanner);//Commander - Added: Side Banner
	ini.WriteBool(L"BringToFront",bringtoforeground);
	ini.WriteBool(L"TransferDoubleClick",transferDoubleclick);
	ini.WriteBool(L"BeepOnError",beepOnError);
	ini.WriteBool(L"ConfirmExit",confirmExit);
	ini.WriteBool(L"FilterBadIPs",filterLANIPs);
    ini.WriteBool(L"Autoconnect",autoconnect);
	ini.WriteBool(L"OnlineSignature",onlineSig);
	ini.WriteBool(L"StartupMinimized",startMinimized);
	ini.WriteBool(L"AutoStart",m_bAutoStart);
	ini.WriteInt(L"LastMainWndDlgID",m_iLastMainWndDlgID);
	ini.WriteInt(L"LastLogPaneID",m_iLastLogPaneID);
	ini.WriteBool(L"SafeServerConnect",m_bSafeServerConnect);
	ini.WriteBool(L"ShowRatesOnTitle",showRatesInTitle);
	ini.WriteBool(L"IndicateRatings",indicateratings);
	ini.WriteBool(L"WatchClipboard4ED2kFilelinks",watchclipboard);
	ini.WriteInt(L"SearchMethod",m_iSearchMethod);
	ini.WriteBool(L"CheckDiskspace",checkDiskspace);
	ini.WriteInt(L"MinFreeDiskSpace",m_uMinFreeDiskSpace);
	ini.WriteBool(L"SparsePartFiles",m_bSparsePartFiles);
	ini.WriteString(L"YourHostname",m_strYourHostname);

	// Barry - New properties...
    ini.WriteBool(L"AutoConnectStaticOnly", m_bAutoConnectToStaticServersOnly);
	ini.WriteBool(L"AutoTakeED2KLinks", autotakeed2klinks);
    ini.WriteBool(L"AddNewFilesPaused", addnewfilespaused);
    ini.WriteInt (L"3DDepth", depth3D);  

	ini.WriteString(L"NotifierConfiguration", notifierConfiguration);
	ini.WriteBool(L"NotifyOnDownload", notifierOnDownloadFinished);
	ini.WriteBool(L"NotifyOnNewDownload", notifierOnNewDownload);
	ini.WriteBool(L"NotifyOnChat", notifierOnChat);
	ini.WriteBool(L"NotifyOnLog", notifierOnLog);
	ini.WriteBool(L"NotifyOnImportantError", notifierOnImportantError);
	ini.WriteBool(L"NotifierPopEveryChatMessage", notifierOnEveryChatMsg);
	ini.WriteBool(L"NotifierPopNewVersion", notifierOnNewVersion);
	ini.WriteInt(L"NotifierUseSound", (int)notifierSoundType);
	ini.WriteString(L"NotifierSoundPath", notifierSoundFile);

	ini.WriteString(L"TxtEditor",TxtEditor);
	ini.WriteString(L"VideoPlayer",VideoPlayer);
	ini.WriteString(L"MessageFilter",messageFilter);
	ini.WriteString(L"CommentFilter",commentFilter);
	ini.WriteString(L"DateTimeFormat",GetDateTimeFormat());
	ini.WriteString(L"DateTimeFormat4Log",GetDateTimeFormat4Log());
	ini.WriteString(L"WebTemplateFile",m_sTemplateFile);
	ini.WriteString(L"FilenameCleanups",filenameCleanups);
	ini.WriteInt(L"ExtractMetaData",m_iExtractMetaData);

	ini.WriteString(L"DefaultIRCServerNew",m_sircserver);
	ini.WriteString(L"IRCNick",m_sircnick);
	ini.WriteBool(L"IRCAddTimestamp", m_bircaddtimestamp);
	ini.WriteString(L"IRCFilterName", m_sircchannamefilter);
	ini.WriteInt(L"IRCFilterUser", m_iircchanneluserfilter);
	ini.WriteBool(L"IRCUseFilter", m_bircusechanfilter);
	ini.WriteString(L"IRCPerformString", m_sircperformstring);
	ini.WriteBool(L"IRCUsePerform", m_bircuseperform);
	ini.WriteBool(L"IRCListOnConnect", m_birclistonconnect);
	ini.WriteBool(L"IRCAcceptLink", m_bircacceptlinks);
	ini.WriteBool(L"IRCAcceptLinkFriends", m_bircacceptlinksfriends);
	ini.WriteBool(L"IRCSoundEvents", m_bircsoundevents);
	ini.WriteBool(L"IRCIgnoreMiscMessages", m_bircignoremiscmessage);
	ini.WriteBool(L"IRCIgnoreJoinMessages", m_bircignorejoinmessage);
	ini.WriteBool(L"IRCIgnorePartMessages", m_bircignorepartmessage);
	ini.WriteBool(L"IRCIgnoreQuitMessages", m_bircignorequitmessage);
	ini.WriteBool(L"IRCIgnoreEmuleProtoAddFriend", m_bircignoreemuleprotoaddfriend);
	ini.WriteBool(L"IRCAllowEmuleProtoAddFriend", m_bircallowemuleprotoaddfriend);
	ini.WriteBool(L"IRCIgnoreEmuleProtoSendLink", m_bircignoreemuleprotosendlink);
	ini.WriteBool(L"IRCHelpChannel", m_birchelpchannel);
	ini.WriteBool(L"SmartIdCheck", m_bSmartServerIdCheck);
	ini.WriteBool(L"Verbose", m_bVerbose);
	ini.WriteBool(L"DebugSourceExchange", m_bDebugSourceExchange);	// do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogBannedClients", m_bLogBannedClients);			// do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogRatingDescReceived", m_bLogRatingDescReceived);// do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogSecureIdent", m_bLogSecureIdent);				// do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogFilteredIPs", m_bLogFilteredIPs);				// do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogFileSaving", m_bLogFileSaving);				// do *not* use the according 'Get...' function here!
    ini.WriteBool(L"LogA4AF", m_bLogA4AF);                           // do *not* use the according 'Get...' function here!
	ini.WriteBool(L"LogUlDlEvents", m_bLogUlDlEvents);
	// MORPH START - Added by Commander, WebCache 1.2f
	ini.WriteBool(L"LogWebCacheEvents", m_bLogWebCacheEvents);//JP log webcache events
	ini.WriteBool(L"LogICHEvents", m_bLogICHEvents);//JP log ICH events
	// MORPH END - Added by Commander, WebCache 1.2f
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	// following options are for debugging or when using an external debug device viewer only.
	ini.WriteInt(L"DebugServerTCP",m_iDebugServerTCPLevel);
	ini.WriteInt(L"DebugServerUDP",m_iDebugServerUDPLevel);
	ini.WriteInt(L"DebugServerSources",m_iDebugServerSourcesLevel);
	ini.WriteInt(L"DebugServerSearches",m_iDebugServerSearchesLevel);
	ini.WriteInt(L"DebugClientTCP",m_iDebugClientTCPLevel);
	ini.WriteInt(L"DebugClientUDP",m_iDebugClientUDPLevel);
	ini.WriteInt(L"DebugClientKadUDP",m_iDebugClientKadUDPLevel);
#endif
	ini.WriteBool(L"PreviewPrio", m_bpreviewprio);
	ini.WriteBool(L"UpdateQueueListPref", m_bupdatequeuelist);
	ini.WriteBool(L"ManualHighPrio", m_bManualAddedServersHighPriority);
	ini.WriteBool(L"FullChunkTransfers", m_btransferfullchunks);
	ini.WriteBool(L"ShowOverhead", m_bshowoverhead);
	ini.WriteBool(L"VideoPreviewBackupped", moviePreviewBackup);
	ini.WriteInt(L"StartNextFile", m_istartnextfile);

	ini.DeleteKey(L"FileBufferSizePref"); // delete old 'file buff size' setting
	ini.WriteInt(L"FileBufferSize", m_iFileBufferSize);

	ini.DeleteKey(L"QueueSizePref"); // delete old 'queue size' setting
	ini.WriteInt(L"QueueSize", m_iQueueSize);

	ini.WriteInt(L"CommitFiles", m_iCommitFiles);
	ini.WriteBool(L"DAPPref", m_bDAP);
	ini.WriteBool(L"UAPPref", m_bUAP);
	ini.WriteBool(L"FilterServersByIP",filterserverbyip);
	ini.WriteBool(L"DisableKnownClientList",m_bDisableKnownClientList);
	ini.WriteBool(L"DisableQueueList",m_bDisableQueueList);
	ini.WriteBool(L"UseCreditSystem",m_bCreditSystem);
	ini.WriteBool(L"SaveLogToDisk",log2disk);
	ini.WriteBool(L"SaveDebugToDisk",debug2disk);
	ini.WriteBool(L"EnableScheduler",scheduler);
	ini.WriteBool(L"MessagesFromFriendsOnly",msgonlyfriends);
	ini.WriteBool(L"MessageFromValidSourcesOnly",msgsecure);
	ini.WriteBool(L"ShowInfoOnCatTabs",showCatTabInfos);
	ini.WriteBool(L"DontRecreateStatGraphsOnResize",dontRecreateGraphs);
	ini.WriteBool(L"AutoFilenameCleanup",autofilenamecleanup);
	ini.WriteBool(L"ShowExtControls",m_bExtControls);
	ini.WriteBool(L"UseAutocompletion",m_bUseAutocompl);
	ini.WriteBool(L"NetworkKademlia",networkkademlia);
	ini.WriteBool(L"NetworkED2K",networked2k);
	ini.WriteBool(L"AutoClearCompleted",m_bRemoveFinishedDownloads);
	ini.WriteBool(L"TransflstRemainOrder",m_bTransflstRemain);
	ini.WriteBool(L"UseSimpleTimeRemainingcomputation",m_bUseOldTimeRemaining);
	ini.WriteBool(L"AllocateFullFile",m_bAllocFull);

	ini.WriteInt(L"VersionCheckLastAutomatic", versioncheckLastAutomatic);
	//MORPH START - Added by SiRoB, New Version check
	ini.WriteInt(L"MVersionCheckLastAutomatic", mversioncheckLastAutomatic);
	//MORPH END   - Added by SiRoB, New Version check
	ini.WriteInt(L"FilterLevel",filterlevel);

	ini.WriteBool(L"SecureIdent", m_bUseSecureIdent);// change the name in future version to enable it by default
	ini.WriteBool(L"AdvancedSpamFilter",m_bAdvancedSpamfilter);
	ini.WriteBool(L"ShowDwlPercentage",m_bShowDwlPercentage);
	ini.WriteBool(L"RemoveFilesToBin",m_bRemove2bin);
	//ini.WriteBool(L"ShowCopyEd2kLinkCmd",m_bShowCopyEd2kLinkCmd);

	// Toolbar
	ini.WriteString(L"ToolbarSetting", m_sToolbarSettings);
	ini.WriteString(L"ToolbarBitmap", m_sToolbarBitmap );
	ini.WriteString(L"ToolbarBitmapFolder", m_sToolbarBitmapFolder);
	ini.WriteInt(L"ToolbarLabels", m_nToolbarLabels);
	ini.WriteInt(L"ToolbarIconSize", m_sizToolbarIconSize.cx);
	ini.WriteString(L"SkinProfile", m_strSkinProfile);
	ini.WriteString(L"SkinProfileDir", m_strSkinProfileDir);

	ini.WriteBinary(L"HyperTextFont", (LPBYTE)&m_lfHyperText, sizeof m_lfHyperText);
	ini.WriteBinary(L"LogTextFont", (LPBYTE)&m_lfLogText, sizeof m_lfLogText);

	// ZZ:UploadSpeedSense -->
    ini.WriteBool(L"USSEnabled", m_bDynUpEnabled);
    ini.WriteBool(L"USSUseMillisecondPingTolerance", m_bDynUpUseMillisecondPingTolerance);
    ini.WriteInt(L"USSPingTolerance", m_iDynUpPingTolerance);
	ini.WriteInt(L"USSPingToleranceMilliseconds", m_iDynUpPingToleranceMilliseconds); // EastShare - Add by TAHO, USS limit
    ini.WriteInt(L"USSGoingUpDivider", m_iDynUpGoingUpDivider);
    ini.WriteInt(L"USSGoingDownDivider", m_iDynUpGoingDownDivider);
    ini.WriteInt(L"USSNumberOfPings", m_iDynUpNumberOfPings);
	// ZZ:UploadSpeedSense <--

    ini.WriteBool(L"A4AFSaveCpu", m_bA4AFSaveCpu); // ZZ:DownloadManager
    ini.WriteBool(L"HighresTimer", m_bHighresTimer);
	ini.WriteInt(L"WebMirrorAlertLevel", m_nWebMirrorAlertLevel);
	ini.WriteBool(L"RunAsUnprivilegedUser", m_bRunAsUser);
	ini.WriteBool(L"OpenPortsOnStartUp", m_bOpenPortsOnStartUp);
	ini.WriteInt(L"DebugLogLevel", m_byLogLevel);
	ini.WriteInt(L"WinXPSP2", IsRunningXPSP2());
	ini.WriteBool(L"RememberCancelledFiles", m_bRememberCancelledFiles);
	ini.WriteBool(L"RememberDownloadedFiles", m_bRememberDownloadedFiles);

	ini.WriteBool(L"NotifierSendMail", m_bNotifierSendMail);
	ini.WriteString(L"NotifierMailSender", m_strNotifierMailSender);
	ini.WriteString(L"NotifierMailServer", m_strNotifierMailServer);
	ini.WriteString(L"NotifierMailRecipient", m_strNotifierMailReceiver);

	ini.WriteBool(L"WinaTransToolbar", m_bWinaTransToolbar);




    // ==> Slot Limit - Stulle
	ini.WriteBool(_T("SlotLimitThree"), m_bSlotLimitThree);
	ini.WriteBool(_T("SlotLimitNumB"), m_bSlotLimitNum);
	ini.WriteInt(_T("SlotLimitNum"), m_iSlotLimitNum);
	// <== Slot Limit - Stulle

	///////////////////////////////////////////////////////////////////////////
	// Section: "Proxy"
	//
	ini.WriteBool(L"ProxyEnablePassword",proxy.EnablePassword,L"Proxy");
	ini.WriteBool(L"ProxyEnableProxy",proxy.UseProxy,L"Proxy");
	ini.WriteString(L"ProxyName",CStringW(proxy.name),L"Proxy");
	ini.WriteString(L"ProxyPassword",CStringW(proxy.password),L"Proxy");
	ini.WriteString(L"ProxyUser",CStringW(proxy.user),L"Proxy");
	ini.WriteInt(L"ProxyPort",proxy.port,L"Proxy");
	ini.WriteInt(L"ProxyType",proxy.type,L"Proxy");


	///////////////////////////////////////////////////////////////////////////
	// Section: "Statistics"
	//
	ini.WriteInt(L"statsConnectionsGraphRatio", statsConnectionsGraphRatio,L"Statistics");
	ini.WriteString(L"statsExpandedTreeItems", statsExpandedTreeItems);
	CString buffer2;
	for (int i=0;i<GetNumStatsColors();i++) { //MORPH - Changed by SiRoB, Powershare display
		buffer.Format(L"0x%06x",GetStatsColor(i));
		buffer2.Format(L"StatColor%i",i);
		ini.WriteString(buffer2,buffer,L"Statistics" );
	}


	///////////////////////////////////////////////////////////////////////////
	// Section: "WebServer"
	//
	ini.WriteString(L"Password", GetWSPass(), L"WebServer");
	ini.WriteString(L"PasswordLow", GetWSLowPass());
	ini.WriteInt(L"Port", m_nWebPort);
	ini.WriteBool(L"Enabled", m_bWebEnabled);
	ini.WriteBool(L"UseGzip", m_bWebUseGzip);
	ini.WriteInt(L"PageRefreshTime", m_nWebPageRefresh);
	ini.WriteBool(L"UseLowRightsUser", m_bWebLowEnabled);
	ini.WriteBool(L"AllowAdminHiLevelFunc",m_bAllowAdminHiLevFunc);
	ini.WriteInt(L"WebTimeoutMins", m_iWebTimeoutMins);


	///////////////////////////////////////////////////////////////////////////
	// Section: "MobileMule"
	//
	ini.WriteString(L"Password", GetMMPass(), L"MobileMule");
	ini.WriteBool(L"Enabled", m_bMMEnabled);
	ini.WriteInt(L"Port", m_nMMPort);


	///////////////////////////////////////////////////////////////////////////
	// Section: "PeerCache"
	//
	ini.WriteInt(L"LastSearch", m_uPeerCacheLastSearch, L"PeerCache");
	ini.WriteBool(L"Found", m_bPeerCacheWasFound);
	ini.WriteBool(L"Enabled", m_bPeerCacheEnabled);
	ini.WriteInt(L"PCPort", m_nPeerCachePort);
        
	//MORPH START - Added by SiRoB, [MoNKi: -UPnPNAT Support-]
	ini.WriteBool(_T("UPnPNAT"), m_bUPnPNat, _T("eMule"));
	ini.WriteBool(_T("UPnPNAT_Web"), m_bUPnPNatWeb, _T("eMule"));
	ini.WriteBool(_T("UPnPVerbose"), m_bUPnPVerboseLog, _T("eMule"));
	ini.WriteInt(_T("UPnPPort"), m_iUPnPPort, _T("eMule"));
	ini.WriteBool(_T("UPnPClearOnClose"), m_bUPnPClearOnClose, _T("eMule"));
	ini.WriteBool(_T("UPnPLimitToFirstConnection"), m_bUPnPLimitToFirstConnection, _T("eMule"));
	ini.WriteInt(_T("UPnPDetect"), m_iDetectuPnP, _T("eMule")); // 
	//MORPH END   - Added by SiRoB, [MoNKi: -UPnPNAT Support-]
  
	//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-]
	ini.WriteBool(_T("RandomPorts"), m_bRndPorts, _T("eMule"));
	ini.WriteInt(_T("MinRandomPort"), m_iMinRndPort, _T("eMule"));
	ini.WriteInt(_T("MaxRandomPort"), m_iMaxRndPort, _T("eMule"));
	ini.WriteBool(_T("RandomPortsReset"), m_bRndPortsResetOnRestart, _T("eMule"));
	ini.WriteInt(_T("RandomPortsSafeResetOnRestartTime"), m_iRndPortsSafeResetOnRestartTime, _T("eMule"));
	
	ini.WriteInt(_T("OldTCPRandomPort"), m_iCurrentTCPRndPort, _T("eMule"));
	ini.WriteInt(_T("OldUDPRandomPort"), m_iCurrentUDPRndPort, _T("eMule"));
	ini.WriteUInt64(_T("RandomPortsLastRun"), CTime::GetCurrentTime().GetTime() , _T("eMule"));
	//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-]

	//MORPH START - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
	ini.WriteBool(_T("ICFSupportFirstTime"), m_bICFSupportFirstTime, _T("eMule"));
	ini.WriteBool(_T("ICFSupport"), m_bICFSupport, _T("eMule"));
	ini.WriteBool(_T("ICFSupportServerUDP"), m_bICFSupportServerUDP , _T("eMule"));
	//MORPH END   - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]

    //MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
	ini.WriteBool(_T("WapEnabled"), m_bWapEnabled, _T("WapServer"));
	ini.WriteString(_T("WapTemplateFile"),m_sWapTemplateFile, _T("WapServer"));
	ini.WriteInt(_T("WapPort"), m_nWapPort, _T("WapServer"));
	ini.WriteInt(_T("WapGraphWidth"), m_iWapGraphWidth, _T("WapServer"));
	ini.WriteInt(_T("WapGraphHeight"), m_iWapGraphHeight, _T("WapServer"));
	ini.WriteBool(_T("WapFilledGraphs"), m_bWapFilledGraphs, _T("WapServer"));
	ini.WriteInt(_T("WapMaxItemsInPage"), m_iWapMaxItemsInPages, _T("WapServer"));
	ini.WriteBool(_T("WapSendImages"), m_bWapSendImages, _T("WapServer"));
	ini.WriteBool(_T("WapSendGraphs"), m_bWapSendGraphs, _T("WapServer"));
	ini.WriteBool(_T("WapSendProgressBars"), m_bWapSendProgressBars, _T("WapServer"));
	ini.WriteBool(_T("WapSendBWImages"), m_bWapAllwaysSendBWImages, _T("WapServer"));
	ini.WriteInt(_T("WapLogsSize"), m_iWapLogsSize, _T("WapServer"));
	ini.WriteString(_T("WapPassword"), m_sWapPassword, _T("WapServer"));
	ini.WriteString(_T("WapPasswordLow"), m_sWapLowPassword, _T("WapServer"));
	ini.WriteBool(_T("WapLowEnable"), m_bWapLowEnabled, _T("WapServer"));
	//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]        

	//MORPH START - Added by Commander, ClientQueueProgressBar  
	ini.WriteBool(_T("ClientQueueProgressBar"),m_bClientQueueProgressBar, _T("eMule"));
	//MORPH END - Added by Commander, ClientQueueProgressBar

	//MORPH START - Added by Commander, Show WC stats
	ini.WriteBool(_T("CountWCSessionStats"),m_bCountWCSessionStats, _T("eMule"));
    //MORPH END - Added by Commander, Show WC stats

	//MORPH START - Added by Commander, FolderIcons  
	ini.WriteBool(_T("ShowFolderIcons"),m_bShowFolderIcons, _T("eMule"));
	//MORPH END - Added by Commander, FolderIcons

	ini.WriteBool(_T("InfiniteQueue"),infiniteQueue,_T("eMule"));	//Morph - added by AndCycle, SLUGFILLER: infiniteQueue

	ini.WriteBool(_T("AutoDynUpSwitching"),isautodynupswitching,_T("eMule"));//MORPH - Added by Yun.SF3, Auto DynUp changing
	ini.WriteInt(_T("PowershareMode"),m_iPowershareMode,_T("eMule")); //MORPH - Added by SiRoB, Avoid misusing of powersharing
	ini.WriteBool(_T("PowershareInternalPrio"),m_bPowershareInternalPrio,_T("eMule"));//Morph - added by AndCyle, selective PS internal Prio

	ini.WriteBool(_T("EnableHighProcess"), enableHighProcess,_T("eMule")); //MORPH - Added by IceCream, high process priority

	ini.WriteBool(_T("EnableDownloadInRed"), enableDownloadInRed,_T("eMule")); //MORPH - Added by IceCream, show download in red
	ini.WriteBool(_T("EnableDownloadInBold"), m_bShowActiveDownloadsBold,_T("eMule")); //MORPH - Added by SiRoB, show download in Bold
	ini.WriteBool(_T("EnableAntiLeecher"), enableAntiLeecher,_T("eMule")); //MORPH - Added by IceCream, enable AntiLeecher
	ini.WriteBool(_T("EnableAntiCreditHack"), enableAntiCreditHack,_T("eMule")); //MORPH - Added by IceCream, enable AntiCreditHack
	ini.WriteInt(_T("CreditSystemMode"), creditSystemMode,_T("eMule"));// EastShare - Added by linekin, ES CreditSystem
	ini.WriteBool(_T("EqualChanceForEachFile"), m_bEnableEqualChanceForEachFile, _T("eMule"));	//Morph - added by AndCycle, Equal Chance For Each File

	//MORPH START - Added by SiRoB, Datarate Average Time Management
	ini.WriteInt(_T("DownloadDataRateAverageTime"),max(1,m_iDownloadDataRateAverageTime/1000),_T("eMule"));
	ini.WriteInt(_T("UPloadDataRateAverageTime"),max(1,m_iUploadDataRateAverageTime/1000),_T("eMule"));
	//MORPH END   - Added by SiRoB, Datarate Average Time Management

	//MORPH START - Added by SiRoB, Upload Splitting Class
	ini.WriteInt(_T("GlobalDataRateFriend"),globaldataratefriend,_T("eMule"));
	ini.WriteInt(_T("MaxGlobalDataRateFriend"),maxglobaldataratefriend,_T("eMule"));
	ini.WriteInt(_T("GlobalDataRatePowerShare"),globaldataratepowershare,_T("eMule"));
	ini.WriteInt(_T("MaxGlobalDataRatePowerShare"),maxglobaldataratepowershare,_T("eMule"));
	ini.WriteInt(_T("MaxClientDataRateFriend"),maxclientdataratefriend,_T("eMule"));
	ini.WriteInt(_T("MaxClientDataRatePowerShare"),maxclientdataratepowershare,_T("eMule"));
	ini.WriteInt(_T("MaxClientDataRate"),maxclientdatarate,_T("eMule"));
	//MORPH END   - Added by SiRoB, Upload Splitting Class

	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	ini.WriteInt(_T("ReconnectOnLowIdRetries"),LowIdRetries,_T("eMule"));	// SLUGFILLER: lowIdRetry
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry
	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	ini.WriteInt(_T("SpreadbarSetStatus"), m_iSpreadbarSetStatus, _T("eMule"));
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	ini.WriteInt(_T("HideOvershares"),hideOS,_T("eMule"));
	ini.WriteBool(_T("SelectiveShare"),selectiveShare,_T("eMule"));
	//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	ini.WriteInt(_T("ShareOnlyTheNeed"),ShareOnlyTheNeed,_T("eMule"));
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	ini.WriteInt(_T("PowerShareLimit"),PowerShareLimit,_T("eMule"));
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	//MORPH START - Added by SiRoB, Show Permissions
	ini.WriteInt(_T("ShowSharePermissions"),permissions,_T("eMule"));
	//MORPH END   - Added by SiRoB, Show Permissions

    //MORPH START added by Yun.SF3: Ipfilter.dat update
	ini.WriteBinary(_T("IPfilterVersion"), (LPBYTE)&m_IPfilterVersion, sizeof(m_IPfilterVersion),_T("eMule")); 
	ini.WriteBool(_T("AutoUPdateIPFilter"),AutoUpdateIPFilter,_T("eMule"));
    //MORPH END added by Yun.SF3: Ipfilter.dat update

	//Commander - Added: IP2Country Auto-updating - Start
	ini.WriteBinary(_T("IP2CountryVersion"), (LPBYTE)&m_IP2CountryVersion, sizeof(m_IP2CountryVersion),_T("eMule")); 
	ini.WriteBool(_T("AutoUPdateIP2Country"),AutoUpdateIP2Country,_T("eMule"));
	//Commander - Added: IP2Country Auto-updating - End

	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	ini.WriteBinary(_T("FakesDatVersion"), (LPBYTE)&m_FakesDatVersion, sizeof(m_FakesDatVersion),_T("eMule")); 
	ini.WriteBool(_T("UpdateFakeStartup"),UpdateFakeStartup,_T("eMule"));
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating

	ini.WriteString(_T("UpdateURLFakeList"),UpdateURLFakeList,_T("eMule"));		//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	ini.WriteString(_T("UpdateURLIPFilter"),UpdateURLIPFilter,_T("eMule"));//MORPH START added by Yun.SF3: Ipfilter.dat update
    ini.WriteString(_T("UpdateURLIP2Country"),UpdateURLIP2Country,_T("eMule"));//Commander - Added: IP2Country auto-updating

	//EastShare Start - PreferShareAll by AndCycle
	ini.WriteBool(_T("ShareAll"),shareall,_T("eMule"));	// SLUGFILLER: preferShareAll
	//EastShare END - PreferShareAll by AndCycle
	// EastShare START - Added by TAHO, .met file control
	ini.WriteInt(_T("KnownMetDays"), m_iKnownMetDays,_T("eMule"));
	// EastShare END - Added by TAHO, .met file control
	//EastShare - Added by Pretender, Option for ChunkDots
	ini.WriteInt(_T("EnableChunkDots"), m_bEnableChunkDots,_T("eMule"));
	//EastShare - Added by Pretender, Option for ChunkDots

	//EastShare - added by AndCycle, IP to Country
	ini.WriteInt(_T("IP2Country"), m_iIP2CountryNameMode,_T("eMule")); 
	ini.WriteBool(_T("IP2CountryShowFlag"), m_bIP2CountryShowFlag,_T("eMule"));
	//EastShare - added by AndCycle, IP to Country

	// khaos::categorymod+ Save Preferences
	ini.WriteBool(_T("ValidSrcsOnly"), m_bValidSrcsOnly,_T("eMule"));
	ini.WriteBool(_T("ShowCatName"), m_bShowCatNames,_T("eMule"));
	ini.WriteBool(_T("ActiveCatDefault"), m_bActiveCatDefault,_T("eMule"));
	ini.WriteBool(_T("SelCatOnAdd"), m_bSelCatOnAdd,_T("eMule"));
	ini.WriteBool(_T("AutoSetResumeOrder"), m_bAutoSetResumeOrder,_T("eMule"));
	ini.WriteBool(_T("SmallFileDLPush"), m_bSmallFileDLPush,_T("eMule"));
	ini.WriteInt(_T("StartDLInEmptyCats"), m_iStartDLInEmptyCats,_T("eMule"));
	ini.WriteBool(_T("UseAutoCat"), m_bUseAutoCat,_T("eMule"));
	// khaos::categorymod-
	// khaos::kmod+
	ini.WriteBool(_T("SmartA4AFSwapping"), m_bSmartA4AFSwapping,_T("eMule"));
	ini.WriteInt(_T("AdvancedA4AFMode"), m_iAdvancedA4AFMode,_T("eMule"));
	ini.WriteBool(_T("ShowA4AFDebugOutput"), m_bShowA4AFDebugOutput,_T("eMule"));
	ini.WriteBool(_T("RespectMaxSources"), m_bRespectMaxSources,_T("eMule"));
	ini.WriteBool(_T("UseSaveLoadSources"), m_bUseSaveLoadSources,_T("eMule"));
	// khaos::categorymod-
	// khaos::accuratetimerem+
	ini.WriteInt(_T("TimeRemainingMode"), m_iTimeRemainingMode,_T("eMule"));
	// khaos::accuratetimerem-
	//MORPH START - Added by SiRoB, ICS Optional
	ini.WriteBool(_T("UseIntelligentChunkSelection"), m_bUseIntelligentChunkSelection,_T("eMule"));
	//MORPH END   - Added by SiRoB, ICS Optional
	//MORPH START - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	ini.WriteBool(_T("SUCEnabled"),m_bSUCEnabled,_T("eMule"));
	ini.WriteInt(_T("SUCLog"),m_bSUCLog,_T("eMule"));
	ini.WriteInt(_T("SUCHigh"),m_iSUCHigh,_T("eMule"));
	ini.WriteInt(_T("SUCLow"),m_iSUCLow,_T("eMule"));
	ini.WriteInt(_T("SUCDrift"),m_iSUCDrift,_T("eMule"));
	ini.WriteInt(_T("SUCPitch"),m_iSUCPitch,_T("eMule"));
	//MORPH END - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	ini.WriteInt(_T("MaxConnectionsSwitchBorder"),maxconnectionsswitchborder,_T("eMule"));//MORPH - Added by Yun.SF3, Auto DynUp changing

	ini.WriteBool(_T("IsPayBackFirst"),m_bPayBackFirst,_T("eMule"));//EastShare - added by AndCycle, Pay Back First
	ini.WriteInt(_T("PayBackFirstLimit"),m_iPayBackFirstLimit,_T("eMule"));//MORPH - Added by SiRoB, Pay Back First Tweak
	ini.WriteBool(_T("OnlyDownloadCompleteFiles"), m_bOnlyDownloadCompleteFiles,_T("eMule"));//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	ini.WriteBool(_T("SaveUploadQueueWaitTime"), m_bSaveUploadQueueWaitTime,_T("eMule"));//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	ini.WriteBool(_T("DateFileNameLog"), m_bDateFileNameLog,_T("eMule"));//Morph - added by AndCycle, Date File Name Log
	ini.WriteBool(_T("DontRemoveSpareTrickleSlot"), m_bDontRemoveSpareTrickleSlot,_T("eMule"));//Morph - added by AndCycle, Dont Remove Spare Trickle Slot
	ini.WriteBool(_T("DisplayFunnyNick"), m_bFunnyNick,_T("eMule"));//MORPH - Added by SiRoB, Optionnal funnynick display
	//EastShare Start - Added by Pretender, TBH-AutoBackup
	ini.WriteBool(_T("AutoBackup"),autobackup,_T("eMule"));
	ini.WriteBool(_T("AutoBackup2"),autobackup2,_T("eMule"));
	//EastShare End - Added by Pretender, TBH-AutoBackup

	// Mighty Knife: Community visualization, Report hashing files, Log friendlist activities
	ini.WriteString(_T("CommunityName"), m_sCommunityName,_T("eMule"));
	ini.WriteBool (_T("ReportHashingFiles"),m_bReportHashingFiles,_T("eMule"));
	ini.WriteBool (_T("LogFriendlistActivities"),m_bLogFriendlistActivities,_T("eMule"));
	// [end] Mighty Knife

	// Mighty Knife: CRC32-Tag
	ini.WriteBool (_T("DontAddCRC32ToFilename"),m_bDontAddCRCToFilename,_T("eMule"));
	ini.WriteBool (_T("ForceCRC32Uppercase"),m_bCRC32ForceUppercase,_T("eMule"));
	ini.WriteBool (_T("ForceCRC32Adding"),m_bCRC32ForceAdding,_T("eMule"));
	CString temp;
	// Encapsule these strings by "" because space characters are allowed at the
	// beginning/end of the prefix/suffix !
	temp.Format (_T("\"%s\""),m_sCRC32Prefix);
	ini.WriteString(_T("LastCRC32Prefix"),temp,_T("eMule"));
	temp.Format (_T("\"%s\""),m_sCRC32Suffix);
	ini.WriteString(_T("LastCRC32Suffix"),temp,_T("eMule"));
	// [end] Mighty Knife

	// Mighty Knife: Simple cleanup options
	ini.WriteInt (_T("SimpleCleanupOptions"),m_SimpleCleanupOptions);
	// Enclose the strings with '"' before writing them to the file.
	// These will be filtered if the string is read again
	ini.WriteString (_T("SimpleCleanupSearch"),CString ('\"')+m_SimpleCleanupSearch+'\"');
	ini.WriteString (_T("SimpleCleanupReplace"),CString ('\"')+m_SimpleCleanupReplace+'\"');
	ini.WriteString (_T("SimpleCleanupSearchChars"),CString ('\"')+m_SimpleCleanupSearchChars+'\"');
	ini.WriteString (_T("SimpleCleanupReplaceChars"),CString ('\"')+m_SimpleCleanupReplaceChars+'\"');
	// [end] Mighty Knife

	// Mighty Knife: Static server handling
	ini.WriteBool (_T("DontRemoveStaticServers"),m_bDontRemoveStaticServers,_T("eMule"));
	// [end] Mighty Knife

	ini.WriteBool(_T("SolidGraph"), m_bSolidGraph,_T("eMule")); //MORPH - Added by SiRoB, New Graph
	//MORPH START - Added by SiRoB,  ZZ dynamic upload (USS)
	ini.WriteBool(_T("USSLog"), m_bDynUpLog,_T("eMule"));
	//MORPH END    - Added by SiRoB,  ZZ dynamic upload (USS)
	ini.WriteBool(_T("USSUDP_FORCE"), m_bUSSUDP,_T("eMule")); //MORPH - Added by SiRoB, USS UDP preferency
    ini.WriteBool(_T("ShowClientPercentage"),m_bShowClientPercentage);  //Commander - Added: Client Percentage

    //Commander - Added: Invisible Mode [TPT] - Start
    ini.WriteBool(_T("InvisibleMode"), m_bInvisibleMode);
	ini.WriteInt(_T("InvisibleModeHKKey"), (int)m_cInvisibleModeHotKey);
	ini.WriteInt(_T("InvisibleModeHKKeyModifier"), m_iInvisibleModeHotKeyModifier);
    //Commander - Added: Invisible Mode [TPT] - End        

	//MORPH START - Added by Stulle, Global Source Limit
	ini.WriteBool(_T("GlobalHL"), m_bGlobalHL);
	ini.WriteInt(_T("GlobalHLvalue"), m_uGlobalHL);
	//MORPH END   - Added by Stulle, Global Source Limit
}

void CPreferences::ResetStatsColor(int index)
{
	switch(index)
	{
		case 0 : m_adwStatsColors[0]=RGB(0,0,0);break;  //MORPH - HotFix by SiRoB & IceCream, Default Black color for BackGround
		case 1 : m_adwStatsColors[1]=RGB(192,192,255);break;
		case 2 : m_adwStatsColors[2]=RGB(128, 255, 128);break;
		case 3 : m_adwStatsColors[3]=RGB(0, 255, 255);break;
		case 4 : m_adwStatsColors[4]=RGB(255, 255, 255);break;
		case 5 : m_adwStatsColors[5]=RGB(255, 0, 0);break;
		case 6 : m_adwStatsColors[6]=RGB(0, 255, 255);break;
		case 7 : m_adwStatsColors[7]=RGB(255, 255, 255);break;
		case 8 : m_adwStatsColors[8]=RGB(150, 150, 255);break;
		case 9 : m_adwStatsColors[9]=RGB(255, 255, 128);break; //MORPH - Added by Yun.SF3, ZZ Upload System
		case 10 : m_adwStatsColors[10]=RGB(0, 255, 0);break;
		case 11 : m_adwStatsColors[11]=RGB(0, 0, 0);break; //MORPH - HotFix by SiRoB & IceCream, Default Black color for SystrayBar
		case 12 : m_adwStatsColors[12]=RGB(192,   0, 192);break; //MORPH - Added by Yun.SF3, ZZ Upload System
		case 13 : m_adwStatsColors[13]=RGB(128, 128, 255);break; //MORPH - Added by Yun.SF3, ZZ Upload System
		case 14 : m_adwStatsColors[14]=RGB(192, 192, 0);break;
		case 15 : m_adwStatsColors[15]=RGB(255, 0, 255);break; //MORPH - Added by SiRoB, Powershare display
	}
}

void CPreferences::GetAllStatsColors(int iCount, LPDWORD pdwColors)
{
	memset(pdwColors, 0, sizeof(*pdwColors) * iCount);
	memcpy(pdwColors, m_adwStatsColors, sizeof(*pdwColors) * min(ARRSIZE(m_adwStatsColors), iCount));
}

bool CPreferences::SetAllStatsColors(int iCount, const DWORD* pdwColors)
{
	bool bModified = false;
	int iMin = min(ARRSIZE(m_adwStatsColors), iCount);
	for (int i = 0; i < iMin; i++)
	{
		if (m_adwStatsColors[i] != pdwColors[i])
		{
			m_adwStatsColors[i] = pdwColors[i];
			bModified = true;
		}
	}
	return bModified;
}

void CPreferences::IniCopy(CString si, CString di) {
	CIni ini(GetConfigFile(), L"eMule");
	
	CString s=ini.GetString(si);

	ini.SetSection(L"ListControlSetup");
	
	ini.WriteString(di,s);
}

// Imports the tablesetups of emuleversions (.ini) <0.46b		- temporary
void CPreferences::ImportOldTableSetup() {

	IniCopy(L"DownloadColumnHidden" ,	L"DownloadListCtrlColumnHidden" );
	IniCopy(L"DownloadColumnWidths" ,	L"DownloadListCtrlColumnWidths" );
	IniCopy(L"DownloadColumnOrder" ,		L"DownloadListCtrlColumnOrders" );
	IniCopy(L"TableSortItemDownload" ,	L"DownloadListCtrlTableSortItem" );
	IniCopy(L"TableSortAscendingDownload" , L"DownloadListCtrlTableSortAscending" );

	IniCopy(L"ONContactListCtrlColumnHidden" ,	L"ONContactListCtrlColumnHidden" );
	IniCopy(L"ONContactListCtrlColumnWidths" ,	L"ONContactListCtrlColumnWidths" );
	IniCopy(L"ONContactListCtrlColumnOrders" ,		L"ONContactListCtrlColumnOrders" );

	IniCopy(L"KadSearchListCtrlColumnHidden" ,	L"KadSearchListCtrlColumnHidden" );
	IniCopy(L"KadSearchListCtrlColumnWidths" ,	L"KadSearchListCtrlColumnWidths" );
	IniCopy(L"KadSearchListCtrlColumnOrders" ,		L"KadSearchListCtrlColumnOrders" );
	
	IniCopy(L"UploadColumnHidden" ,		L"UploadListCtrlColumnHidden" );
	IniCopy(L"UploadColumnWidths" ,		L"UploadListCtrlColumnWidths" );
	IniCopy(L"UploadColumnOrder" ,		L"UploadListCtrlColumnOrders" );
	IniCopy(L"TableSortItemUpload" ,		L"UploadListCtrlTableSortItem" );
	IniCopy(L"TableSortAscendingUpload", L"UploadListCtrlTableSortAscending" );

	IniCopy(L"QueueColumnHidden" ,		L"QueueListCtrlColumnHidden" );
	IniCopy(L"QueueColumnWidths" ,		L"QueueListCtrlColumnWidths" );
	IniCopy(L"QueueColumnOrder" ,		L"QueueListCtrlColumnOrders" );
	IniCopy(L"TableSortItemQueue" ,		L"QueueListCtrlTableSortItem" );
	IniCopy(L"TableSortAscendingQueue" , L"QueueListCtrlTableSortAscending" );

	IniCopy(L"SearchColumnHidden" ,		L"SearchListCtrlColumnHidden" );
	IniCopy(L"SearchColumnWidths" ,		L"SearchListCtrlColumnWidths" );
	IniCopy(L"SearchColumnOrder" ,		L"SearchListCtrlColumnOrders" );
	IniCopy(L"TableSortItemSearch" ,		L"SearchListCtrlTableSortItem" );
	IniCopy(L"TableSortAscendingSearch", L"SearchListCtrlTableSortAscending" );

	IniCopy(L"SharedColumnHidden" ,		L"SharedFilesCtrlColumnHidden" );
	IniCopy(L"SharedColumnWidths" ,		L"SharedFilesCtrlColumnWidths" );
	IniCopy(L"SharedColumnOrder" ,		L"SharedFilesCtrlColumnOrders" );
	IniCopy(L"TableSortItemShared" ,		L"SharedFilesCtrlTableSortItem" );
	IniCopy(L"TableSortAscendingShared", L"SharedFilesCtrlTableSortAscending" );

	IniCopy(L"ServerColumnHidden" ,		L"ServerListCtrlColumnHidden" );
	IniCopy(L"ServerColumnWidths" ,		L"ServerListCtrlColumnWidths" );
	IniCopy(L"ServerColumnOrder" ,		L"ServerListCtrlColumnOrders" );
	IniCopy(L"TableSortItemServer" ,		L"ServerListCtrlTableSortItem" );
	IniCopy(L"TableSortAscendingServer", L"ServerListCtrlTableSortAscending" );

	IniCopy(L"ClientListColumnHidden" ,		L"ClientListCtrlColumnHidden" );
	IniCopy(L"ClientListColumnWidths" ,		L"ClientListCtrlColumnWidths" );
	IniCopy(L"ClientListColumnOrder" ,		L"ClientListCtrlColumnOrders" );
	IniCopy(L"TableSortItemClientList" ,		L"ClientListCtrlTableSortItem" );
	IniCopy(L"TableSortAscendingClientList", L"ClientListCtrlTableSortAscending" );

	IniCopy(L"FilenamesListColumnHidden" ,	L"FileDetailDlgNameColumnHidden" );
	IniCopy(L"FilenamesListColumnWidths" ,	L"FileDetailDlgNameColumnWidths" );
	IniCopy(L"FilenamesListColumnOrder" ,	L"FileDetailDlgNameColumnOrders" );
	IniCopy(L"TableSortItemFilenames" ,		L"FileDetailDlgNameTableSortItem" );
	IniCopy(L"TableSortAscendingFilenames",  L"FileDetailDlgNameTableSortAscending" );

	IniCopy(L"IrcMainColumnHidden" ,		L"IrcNickListCtrlColumnHidden" );
	IniCopy(L"IrcMainColumnWidths" ,		L"IrcNickListCtrlColumnWidths" );
	IniCopy(L"IrcMainColumnOrder" ,		L"IrcNickListCtrlColumnOrders" );
	IniCopy(L"TableSortItemIrcMain" ,	L"IrcNickListCtrlTableSortItem" );
	IniCopy(L"TableSortAscendingIrcMain",L"IrcNickListCtrlTableSortAscending" );

	IniCopy(L"IrcChannelsColumnHidden" ,		L"IrcChannelListCtrlColumnHidden" );
	IniCopy(L"IrcChannelsColumnWidths" ,		L"IrcChannelListCtrlColumnWidths" );
	IniCopy(L"IrcChannelsColumnOrder" ,		L"IrcChannelListCtrlColumnOrders" );
	IniCopy(L"TableSortItemIrcChannels" ,	L"IrcChannelListCtrlTableSortItem" );
	IniCopy(L"TableSortAscendingIrcChannels",L"IrcChannelListCtrlTableSortAscending" );

	IniCopy(L"DownloadClientsColumnHidden" ,		L"DownloadClientsCtrlColumnHidden" );
	IniCopy(L"DownloadClientsColumnWidths" ,		L"DownloadClientsCtrlColumnWidths" );
	IniCopy(L"DownloadClientsColumnOrder" ,		L"DownloadClientsCtrlColumnOrders" );
}

void CPreferences::LoadPreferences()
{
	TCHAR buffer[256];

	CIni ini(GetConfigFile(), L"eMule");

	// import old (<0.46b) table setups - temporary
	if (ini.GetInt(L"SearchListCtrlTableSortItem",-1,L"ListControlSetup")==-1)
		ImportOldTableSetup();
	ini.SetSection(L"eMule");

	CString strCurrVersion, strPrefsVersion;

	//MORPH START - Added by SiRoB, [itsonlyme: -modname-]
	/*
	strCurrVersion = theApp.m_strCurVersionLong;
	*/
	strCurrVersion = theApp.m_strCurVersionLong + _T(" [") + theApp.m_strModLongVersion + _T("]");
	//MORPH END   - Added by SiRoB, [itsonlyme: -modname-]
	
	strPrefsVersion = ini.GetString(L"AppVersion");

	m_bFirstStart = false;

	if (strCurrVersion != strPrefsVersion){
		m_bFirstStart = true;
	}

#ifdef _DEBUG
	m_iDbgHeap = ini.GetInt(L"DebugHeap", 1);
#else
	m_iDbgHeap = 0;
#endif

	m_nWebMirrorAlertLevel = ini.GetInt(L"WebMirrorAlertLevel",0);
	updatenotify=ini.GetBool(L"UpdateNotifyTestClient",true);

	SetUserNick(ini.GetStringUTF8(L"Nick", DEFAULT_NICK));
	if (strNick.IsEmpty() || IsDefaultNick(strNick))
		SetUserNick(DEFAULT_NICK);

	_stprintf(buffer,L"%sIncoming",appdir);
	_stprintf(incomingdir,L"%s",ini.GetString(L"IncomingDir",buffer ));
	MakeFoldername(incomingdir);

	// load tempdir(s) setting
	_stprintf(buffer,L"%sTemp",appdir);

	CString tempdirs;
	tempdirs=ini.GetString(L"TempDir",buffer);
	tempdirs+= L"|" + ini.GetString(L"TempDirs");

	int curPos=0;
	bool doubled;
	CString atmp=tempdirs.Tokenize(L"|", curPos);
	while (!atmp.IsEmpty())
	{
		atmp.Trim();
		if (!atmp.IsEmpty()) {
			MakeFoldername(atmp.GetBuffer(MAX_PATH));
			atmp.ReleaseBuffer();
			doubled=false;
			for (int i=0;i<tempdir.GetCount();i++)	// avoid double tempdirs
				if (atmp.CompareNoCase(GetTempDir(i))==0) {
					doubled=true;
					break;
				}
			if (!doubled) {
				if (PathFileExists(atmp)==FALSE) {
					CreateDirectory(atmp,NULL);
					if (PathFileExists(atmp)==TRUE || tempdir.GetCount()==0)
						tempdir.Add(atmp);
				}
				else
					tempdir.Add(atmp);
			}
		}
		atmp = tempdirs.Tokenize(L"|", curPos);
	}

	maxGraphDownloadRate=ini.GetInt(L"DownloadCapacity",96);
	if (maxGraphDownloadRate==0)
		maxGraphDownloadRate=96;
	
	maxGraphUploadRate = ini.GetInt(L"UploadCapacityNew",-1);
	if (maxGraphUploadRate == 0)
		maxGraphUploadRate = UNLIMITED;
	else if (maxGraphUploadRate == -1){
		// converting value from prior versions
		int nOldUploadCapacity = ini.GetInt(L"UploadCapacity", 16);
		if (nOldUploadCapacity == 16 && ini.GetInt(L"MaxUpload",12) == 12){
			// either this is a complete new install, or the prior version used the default value
			// in both cases, set the new default values to unlimited
			maxGraphUploadRate = UNLIMITED;
			ini.WriteInt(L"MaxUpload",UNLIMITED, L"eMule");
		}
		else
			maxGraphUploadRate = nOldUploadCapacity; // use old custoum value
	}

	minupload=(uint16)ini.GetInt(L"MinUpload", 1);

	//MORPH START - Added by SiRoB, (SUC) & (USS)
	minupload = (uint16)min(max(minupload,1),maxGraphUploadRate);
	//MORPH END   - Added by SiRoB, (SUC) & (USS)
	maxupload=(uint16)ini.GetInt(L"MaxUpload",UNLIMITED);
	if (maxupload > maxGraphUploadRate && maxupload != UNLIMITED)
		maxupload = (uint16)(maxGraphUploadRate * .8);
	
	maxdownload=(uint16)ini.GetInt(L"MaxDownload", UNLIMITED);
	if (maxdownload > maxGraphDownloadRate && maxdownload != UNLIMITED)
		maxdownload = (uint16)(maxGraphDownloadRate * .8);
	maxconnections=ini.GetInt(L"MaxConnections",GetRecommendedMaxConnections());
	maxhalfconnections=ini.GetInt(L"MaxHalfConnections",9);
	m_bConditionalTCPAccept = ini.GetBool(L"ConditionalTCPAccept", false);

	// reset max halfopen to a default if OS changed to SP2 or away
	int dwSP2 = ini.GetInt(L"WinXPSP2", -1);
	int dwCurSP2 = IsRunningXPSP2();
	if (dwSP2 != dwCurSP2){
		if (dwCurSP2 == 0)
			maxhalfconnections = 50;
		else if (dwCurSP2 == 1)
			maxhalfconnections = 9;
	}
  // MORPH START leuk_he upnp bindaddr
	// abuse m_strBindAddrW will be overwriten in a sec...
	m_strBindAddrW = ini.GetString(L"upnpBindAddr");
	m_strBindAddrW.Trim();

  	SetUpnpBindAddr(ntohl(inet_addr((LPCSTR)(CStringA)m_strBindAddrW  )));
  // MORPH END leuk_he upnp bindaddr

	m_strBindAddrW = ini.GetString(L"BindAddr");
	m_strBindAddrW.Trim();
	m_pszBindAddrW = m_strBindAddrW.IsEmpty() ? NULL : (LPCWSTR)m_strBindAddrW;
	m_strBindAddrA = m_strBindAddrW;
	m_pszBindAddrA = m_strBindAddrA.IsEmpty() ? NULL : (LPCSTR)m_strBindAddrA;
	port = (uint16)ini.GetInt(L"Port", DEFAULT_TCP_PORT);
	udpport = (uint16)ini.GetInt(L"UDPPort",port+10);
	nServerUDPPort = (uint16)ini.GetInt(L"ServerUDPPort", -1); // 0 = Don't use UDP port for servers, -1 = use a random port (for backward compatibility)
	// MORPH START - Added by Commander, WebCache 1.2e
	// Superlexx - webcache
	/*char tmpWebcacheName[100];
	sprintf(tmpWebcacheName,"%s",ini.GetString(_T("webcacheName"),_T("")));
	webcacheName = tmpWebcacheName; // TODO: something more elegant*/
	webcacheName = ini.GetString(_T("webcacheName"), _T(""));
	webcachePort=(uint16)ini.GetInt(_T("webcachePort"),0);
	webcacheBlockLimit=(uint16)ini.GetInt(_T("webcacheBlockLimit"));
	webcacheExtraTimeout=ini.GetBool(_T("webcacheExtraTimeout"));
	PersistentConnectionsForProxyDownloads=ini.GetBool(_T("PersistentConnectionsForProxyDownloads"), false);
	WCAutoupdate=ini.GetBool(_T("WCAutoupdate"), true);
	webcacheCachesLocalTraffic=ini.GetBool(_T("webcacheCachesLocalTraffic"), true);
	webcacheEnabled=ini.GetBool(_T("webcacheEnabled"),false); //webcache disabled on first start so webcache detection on start gets called.
	detectWebcacheOnStart=ini.GetBool(_T("detectWebcacheOnStart"), true); // jp detect webcache on startup
	webcacheLastSearch=(uint32)ini.GetUInt64(_T("webcacheLastSearch"));
	webcacheLastGlobalIP=(uint32)ini.GetUInt64(_T("webcacheLastGlobalIP"));
	webcacheLastResolvedName=ini.GetString(_T("webcacheLastResolvedName"),0);
	webcacheTrustLevel=(uint8)ini.GetUInt64(_T("webcacheTrustLevel"),30);
	// webcache end
        // MORPH END - Added by Commander, WebCache 1.2e
	maxsourceperfile=ini.GetInt(L"MaxSourcesPerFile",400 );
	m_wLanguageID=ini.GetWORD(L"Language",0);
	m_iSeeShares=(EViewSharedFilesAccess)ini.GetInt(L"SeeShare",vsfaNobody);
	m_iToolDelayTime=ini.GetInt(L"ToolTipDelay",1);
	trafficOMeterInterval=ini.GetInt(L"StatGraphsInterval",3);
	statsInterval=ini.GetInt(L"statsInterval",5);
	dontcompressavi=ini.GetBool(L"DontCompressAvi",false);
	
	m_uDeadServerRetries=ini.GetInt(L"DeadServerRetry",1);
	if (m_uDeadServerRetries > MAX_SERVERFAILCOUNT)
		m_uDeadServerRetries = MAX_SERVERFAILCOUNT;
	m_dwServerKeepAliveTimeout=ini.GetInt(L"ServerKeepAliveTimeout",0);
	splitterbarPosition=ini.GetInt(L"SplitterbarPosition",75);
	if (splitterbarPosition < 9)
		splitterbarPosition = 9;
	else if (splitterbarPosition > 93)
		splitterbarPosition = 93;
	splitterbarPositionStat=ini.GetInt(L"SplitterbarPositionStat",30);
	splitterbarPositionStat_HL=ini.GetInt(L"SplitterbarPositionStat_HL",66);
	splitterbarPositionStat_HR=ini.GetInt(L"SplitterbarPositionStat_HR",33);
	if (splitterbarPositionStat_HR+1>=splitterbarPositionStat_HL){
		splitterbarPositionStat_HL = 66;
		splitterbarPositionStat_HR = 33;
	}
	splitterbarPositionFriend=ini.GetInt(L"SplitterbarPositionFriend",300);
	splitterbarPositionShared=ini.GetInt(L"SplitterbarPositionShared",179);
	splitterbarPositionIRC=ini.GetInt(L"SplitterbarPositionIRC",200);
	splitterbarPositionSvr=ini.GetInt(L"SplitterbarPositionServer",75);
	if (splitterbarPositionSvr>90 || splitterbarPositionSvr<10)
		splitterbarPositionSvr=75;

	m_uTransferWnd1 = ini.GetInt(L"TransferWnd1",0);
	m_uTransferWnd2 = ini.GetInt(L"TransferWnd2",1);

	statsMax=ini.GetInt(L"VariousStatisticsMaxValue",100);
	statsAverageMinutes=ini.GetInt(L"StatsAverageMinutes",5);
	MaxConperFive=ini.GetInt(L"MaxConnectionsPerFiveSeconds",GetDefaultMaxConperFive());

	reconnect = ini.GetBool(L"Reconnect", true);
	m_bUseServerPriorities = ini.GetBool(L"Scoresystem", true);
	ICH = ini.GetBool(L"ICH", true);
	m_bAutoUpdateServerList = ini.GetBool(L"Serverlist", false);

	mintotray=ini.GetBool(L"MinToTray",false);
	m_bAddServersFromServer=ini.GetBool(L"AddServersFromServer",false); // MORPH leuk_he default to false to fight false servers
	m_bAddServersFromClients=ini.GetBool(L"AddServersFromClient",false);
	splashscreen=ini.GetBool(L"Splashscreen",true);
	startupsound=ini.GetBool(L"Startupsound",true);//Commander - Added: Enable/Disable Startupsound
	sidebanner=ini.GetBool(L"Sidebanner",true);//Commander - Added: Side Banner
	bringtoforeground=ini.GetBool(L"BringToFront",true);
	transferDoubleclick=ini.GetBool(L"TransferDoubleClick",true);
	beepOnError=ini.GetBool(L"BeepOnError",true);
	confirmExit=ini.GetBool(L"ConfirmExit",true);
	filterLANIPs=ini.GetBool(L"FilterBadIPs",true);
	m_bAllocLocalHostIP=ini.GetBool(L"AllowLocalHostIP",false);
	autoconnect=ini.GetBool(L"Autoconnect",false);
	showRatesInTitle=ini.GetBool(L"ShowRatesOnTitle",false);
	m_bIconflashOnNewMessage=ini.GetBool(L"IconflashOnNewMessage",true);

	onlineSig=ini.GetBool(L"OnlineSignature",false);
	startMinimized=ini.GetBool(L"StartupMinimized",false);
	m_bAutoStart=ini.GetBool(L"AutoStart",false);
	m_bRestoreLastMainWndDlg=ini.GetBool(L"RestoreLastMainWndDlg",false);
	m_iLastMainWndDlgID=ini.GetInt(L"LastMainWndDlgID",0);
	m_bRestoreLastLogPane=ini.GetBool(L"RestoreLastLogPane",false);
	m_iLastLogPaneID=ini.GetInt(L"LastLogPaneID",0);
	m_bSafeServerConnect =ini.GetBool(L"SafeServerConnect",false);

	m_bTransflstRemain =ini.GetBool(L"TransflstRemainOrder",false);
	filterserverbyip=ini.GetBool(L"FilterServersByIP",true); //MORPH leuk_he Changed default from false to true to fight fake servers
	filterlevel=ini.GetInt(L"FilterLevel",127);
	checkDiskspace=ini.GetBool(L"CheckDiskspace",false);
	m_uMinFreeDiskSpace=ini.GetInt(L"MinFreeDiskSpace",20*1024*1024);
	m_bSparsePartFiles=ini.GetBool(L"SparsePartFiles",false);
	m_strYourHostname=ini.GetString(L"YourHostname", L"");

	// Barry - New properties...
	m_bAutoConnectToStaticServersOnly = ini.GetBool(L"AutoConnectStaticOnly",false); 
	autotakeed2klinks = ini.GetBool(L"AutoTakeED2KLinks",true); 
	addnewfilespaused = ini.GetBool(L"AddNewFilesPaused",false); 
	depth3D = ini.GetInt(L"3DDepth", 5);
	m_bEnableMiniMule = ini.GetBool(L"MiniMule", true);

	// Notifier
	notifierConfiguration = ini.GetString(L"NotifierConfiguration", GetConfigDir() + L"Notifier.ini");
    notifierOnDownloadFinished = ini.GetBool(L"NotifyOnDownload");
	notifierOnNewDownload = ini.GetBool(L"NotifyOnNewDownload");
    notifierOnChat = ini.GetBool(L"NotifyOnChat");
    notifierOnLog = ini.GetBool(L"NotifyOnLog");
	notifierOnImportantError = ini.GetBool(L"NotifyOnImportantError");
	notifierOnEveryChatMsg = ini.GetBool(L"NotifierPopEveryChatMessage");
	notifierOnNewVersion = ini.GetBool(L"NotifierPopNewVersion");
    notifierSoundType = (ENotifierSoundType)ini.GetInt(L"NotifierUseSound", ntfstNoSound);
	notifierSoundFile = ini.GetString(L"NotifierSoundPath");

	_stprintf(datetimeformat,L"%s",ini.GetString(L"DateTimeFormat",L"%A, %c"));
	if (_tcslen(datetimeformat)==0) _tcscpy(datetimeformat,L"%A, %c");
	_stprintf(datetimeformat4log,L"%s",ini.GetString(L"DateTimeFormat4Log",L"%c"));
	if (_tcslen(datetimeformat4log)==0) _tcscpy(datetimeformat4log,L"%c");

	_stprintf(m_sircserver,L"%s",ini.GetString(L"DefaultIRCServerNew",L"ircchat.emule-project.net"));
	_stprintf(m_sircnick,L"%s",ini.GetString(L"IRCNick"));
	m_bircaddtimestamp=ini.GetBool(L"IRCAddTimestamp",true);
	_stprintf(m_sircchannamefilter,L"%s",ini.GetString(L"IRCFilterName", L"" ));
	m_bircusechanfilter=ini.GetBool(L"IRCUseFilter", false);
	m_iircchanneluserfilter=ini.GetInt(L"IRCFilterUser", 0);
	_stprintf(m_sircperformstring,L"%s",ini.GetString(L"IRCPerformString", L"" ));
	m_bircuseperform=ini.GetBool(L"IRCUsePerform", false);
	m_birclistonconnect=ini.GetBool(L"IRCListOnConnect", true);
	m_bircacceptlinks=ini.GetBool(L"IRCAcceptLink", true);
	m_bircacceptlinksfriends=ini.GetBool(L"IRCAcceptLinkFriends", true);
	m_bircsoundevents=ini.GetBool(L"IRCSoundEvents", false);
	m_bircignoremiscmessage=ini.GetBool(L"IRCIgnoreMiscMessages", false);
	m_bircignorejoinmessage=ini.GetBool(L"IRCIgnoreJoinMessages", true);
	m_bircignorepartmessage=ini.GetBool(L"IRCIgnorePartMessages", true);
	m_bircignorequitmessage=ini.GetBool(L"IRCIgnoreQuitMessages", true);
	m_bircignoreemuleprotoaddfriend=ini.GetBool(L"IRCIgnoreEmuleProtoAddFriend", false);
	m_bircallowemuleprotoaddfriend=ini.GetBool(L"IRCAllowEmuleProtoAddFriend", true);
	m_bircignoreemuleprotosendlink=ini.GetBool(L"IRCIgnoreEmuleProtoSendLink", false);
	m_birchelpchannel=ini.GetBool(L"IRCHelpChannel",true);
	m_bSmartServerIdCheck=ini.GetBool(L"SmartIdCheck",true);

	log2disk = ini.GetBool(L"SaveLogToDisk",false);
	uMaxLogFileSize = ini.GetInt(L"MaxLogFileSize", 1024*1024);
	iMaxLogBuff = ini.GetInt(L"MaxLogBuff",64) * 1024;
	m_iLogFileFormat = (ELogFileFormat)ini.GetInt(L"LogFileFormat", Unicode);
	m_bEnableVerboseOptions=ini.GetBool(L"VerboseOptions", true);
	if (m_bEnableVerboseOptions)
	{
		m_bVerbose=ini.GetBool(L"Verbose",false);
		m_bFullVerbose=ini.GetBool(L"FullVerbose",false);
		debug2disk=ini.GetBool(L"SaveDebugToDisk",false);
		m_bDebugSourceExchange=ini.GetBool(L"DebugSourceExchange",false);
		m_bLogBannedClients=ini.GetBool(L"LogBannedClients", true);
		m_bLogRatingDescReceived=ini.GetBool(L"LogRatingDescReceived",true);
		m_bLogSecureIdent=ini.GetBool(L"LogSecureIdent",true);
		m_bLogFilteredIPs=ini.GetBool(L"LogFilteredIPs",true);
		m_bLogFileSaving=ini.GetBool(L"LogFileSaving",false);
        m_bLogA4AF=ini.GetBool(L"LogA4AF",false); // ZZ:DownloadManager
		m_bLogUlDlEvents=ini.GetBool(L"LogUlDlEvents",true);

		// MORPH START - Added by Commander, WebCache 1.2e
		m_bLogWebCacheEvents=ini.GetBool(_T("LogWebCacheEvents"),true);//JP log webcache events
		m_bLogICHEvents=ini.GetBool(_T("LogICHEvents"),true);//JP log ICH events
		// MORPH END - Added by Commander, WebCache 1.2e
	}
	else
	{
		if (m_bRestoreLastLogPane && m_iLastLogPaneID>=2)
			m_iLastLogPaneID = 1;
	}
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	// following options are for debugging or when using an external debug device viewer only.
	m_iDebugServerTCPLevel = ini.GetInt(L"DebugServerTCP", 0);
	m_iDebugServerUDPLevel = ini.GetInt(L"DebugServerUDP", 0);
	m_iDebugServerSourcesLevel = ini.GetInt(L"DebugServerSources", 0);
	m_iDebugServerSearchesLevel = ini.GetInt(L"DebugServerSearches", 0);
	m_iDebugClientTCPLevel = ini.GetInt(L"DebugClientTCP", 0);
	m_iDebugClientUDPLevel = ini.GetInt(L"DebugClientUDP", 0);
	m_iDebugClientKadUDPLevel = ini.GetInt(L"DebugClientKadUDP", 0);
	m_iDebugSearchResultDetailLevel = ini.GetInt(L"DebugSearchResultDetailLevel", 0);
#else
	// for normal release builds ensure that those options are all turned off
	m_iDebugServerTCPLevel=0;
	m_iDebugServerUDPLevel=0;
	m_iDebugServerSourcesLevel=0;
	m_iDebugServerSearchesLevel=0;
	m_iDebugClientTCPLevel=0;
	m_iDebugClientUDPLevel=0;
	m_iDebugClientKadUDPLevel=0;
	m_iDebugSearchResultDetailLevel = 0;
#endif

	m_bpreviewprio=ini.GetBool(L"PreviewPrio",false);
	m_bupdatequeuelist=ini.GetBool(L"UpdateQueueListPref",false);
	m_bManualAddedServersHighPriority=ini.GetBool(L"ManualHighPrio",false);
	m_btransferfullchunks=ini.GetBool(L"FullChunkTransfers",true);
	m_istartnextfile=ini.GetInt(L"StartNextFile",0);
	m_bshowoverhead=ini.GetBool(L"ShowOverhead",false);
	moviePreviewBackup=ini.GetBool(L"VideoPreviewBackupped",true);
	m_iPreviewSmallBlocks=ini.GetInt(L"PreviewSmallBlocks", 0);
	m_bPreviewCopiedArchives=ini.GetBool(L"PreviewCopiedArchives", true);
	m_iInspectAllFileTypes=ini.GetInt(L"InspectAllFileTypes", 0);
	m_bAllocFull=ini.GetBool(L"AllocateFullFile",0);

	// read file buffer size (with backward compatibility)
	m_iFileBufferSize=ini.GetInt(L"FileBufferSizePref",0); // old setting
	if (m_iFileBufferSize == 0)
		m_iFileBufferSize = 256*1024;
	else
		m_iFileBufferSize = ((m_iFileBufferSize*15000 + 512)/1024)*1024;
	m_iFileBufferSize=ini.GetInt(L"FileBufferSize",m_iFileBufferSize);

	// read queue size (with backward compatibility)
	m_iQueueSize=ini.GetInt(L"QueueSizePref",0); // old setting
	if (m_iQueueSize == 0)
		m_iQueueSize = 50*100;
	else
		m_iQueueSize = m_iQueueSize*100;
	m_iQueueSize=ini.GetInt(L"QueueSize",m_iQueueSize);
	
	m_iCommitFiles=ini.GetInt(L"CommitFiles", 1); // 1 = "commit" on application shut down; 2 = "commit" on each file saveing
	versioncheckdays=ini.GetInt(L"Check4NewVersionDelay",5);
	m_bDAP=ini.GetBool(L"DAPPref",true);
	m_bUAP=ini.GetBool(L"UAPPref",true);
	m_bPreviewOnIconDblClk=ini.GetBool(L"PreviewOnIconDblClk",false);
	indicateratings=ini.GetBool(L"IndicateRatings",true);
	watchclipboard=ini.GetBool(L"WatchClipboard4ED2kFilelinks",false);
	m_iSearchMethod=ini.GetInt(L"SearchMethod",0);

	showCatTabInfos=ini.GetBool(L"ShowInfoOnCatTabs",false);
//	resumeSameCat=ini.GetBool(L"ResumeNextFromSameCat",false);
	dontRecreateGraphs =ini.GetBool(L"DontRecreateStatGraphsOnResize",false);
	m_bExtControls =ini.GetBool(L"ShowExtControls",false);

	versioncheckLastAutomatic=ini.GetInt(L"VersionCheckLastAutomatic",0);
	//MORPH START - Added by SiRoB, New Version check
	mversioncheckLastAutomatic=ini.GetInt(_T("MVersionCheckLastAutomatic"),0);
	//MORPH END   - Added by SiRoB, New Version check
	m_bDisableKnownClientList=ini.GetBool(L"DisableKnownClientList",false);
	m_bDisableQueueList=ini.GetBool(L"DisableQueueList",false);
	/*
	m_bCreditSystem=ini.GetBool(L"UseCreditSystem",true);
	*/
	m_bCreditSystem=true; //MORPH - Changed by SiRoB, CreditSystem allways used
	scheduler=ini.GetBool(L"EnableScheduler",false);
	msgonlyfriends=ini.GetBool(L"MessagesFromFriendsOnly",false);
	msgsecure=ini.GetBool(L"MessageFromValidSourcesOnly",true);
	autofilenamecleanup=ini.GetBool(L"AutoFilenameCleanup",false);
	m_bUseAutocompl=ini.GetBool(L"UseAutocompletion",true);
	m_bShowDwlPercentage=ini.GetBool(L"ShowDwlPercentage",false);
	networkkademlia=ini.GetBool(L"NetworkKademlia",false);
	networked2k=ini.GetBool(L"NetworkED2K",true);
	m_bRemove2bin=ini.GetBool(L"RemoveFilesToBin",true);
	m_bShowCopyEd2kLinkCmd=ini.GetBool(L"ShowCopyEd2kLinkCmd",false);

	m_iMaxChatHistory=ini.GetInt(L"MaxChatHistoryLines",100);
	if (m_iMaxChatHistory < 1)
		m_iMaxChatHistory = 100;
	maxmsgsessions=ini.GetInt(L"MaxMessageSessions",50);
	m_bShowActiveDownloadsBold = ini.GetBool(L"ShowActiveDownloadsBold", false);

	_stprintf(TxtEditor,L"%s",ini.GetString(L"TxtEditor",L"notepad.exe"));
	_stprintf(VideoPlayer,L"%s",ini.GetString(L"VideoPlayer",L""));
	
	_stprintf(m_sTemplateFile,L"%s",ini.GetString(L"WebTemplateFile", GetConfigDir()+L"eMule.tmpl"));

	messageFilter=ini.GetStringLong(L"MessageFilter",L"Your client has an infinite queue|Your client is connecting too fast|fastest download speed|DI-Emule|eMule FX|ZamBoR 2"); // leuk_he: add some known spammers
	commentFilter = ini.GetStringLong(L"CommentFilter",L"http://|https://|www.");
	commentFilter.MakeLower();
	filenameCleanups=ini.GetStringLong(L"FilenameCleanups",L"http|www.|.com|.de|.org|.net|shared|powered|sponsored|sharelive|filedonkey|");
	m_iExtractMetaData = ini.GetInt(L"ExtractMetaData", 1); // 0=disable, 1=mp3, 2=MediaDet
	if (m_iExtractMetaData > 1)
		m_iExtractMetaData = 1;
	m_bAdjustNTFSDaylightFileTime=ini.GetBool(L"AdjustNTFSDaylightFileTime", true);

	m_bUseSecureIdent=ini.GetBool(L"SecureIdent",true);
	m_bAdvancedSpamfilter=ini.GetBool(L"AdvancedSpamFilter",true);
	m_bRemoveFinishedDownloads=ini.GetBool(L"AutoClearCompleted",false);
	m_bUseOldTimeRemaining= ini.GetBool(L"UseSimpleTimeRemainingcomputation",false);

	// Toolbar
	m_sToolbarSettings = ini.GetString(L"ToolbarSetting", strDefaultToolbar);
	m_sToolbarBitmap = ini.GetString(L"ToolbarBitmap", L"");
	m_sToolbarBitmapFolder = ini.GetString(L"ToolbarBitmapFolder", appdir + L"skins");
	m_nToolbarLabels = (EToolbarLabelType)ini.GetInt(L"ToolbarLabels", CMuleToolbarCtrl::GetDefaultLabelType());
	m_bReBarToolbar = ini.GetBool(L"ReBarToolbar", 1);
	m_sizToolbarIconSize.cx = m_sizToolbarIconSize.cy = ini.GetInt(L"ToolbarIconSize", 32);
	m_iStraightWindowStyles=ini.GetInt(L"StraightWindowStyles",0);
	m_bRTLWindowsLayout = ini.GetBool(L"RTLWindowsLayout");
	m_strSkinProfile = ini.GetString(L"SkinProfile", L"");
	m_strSkinProfileDir = ini.GetString(L"SkinProfileDir", appdir + L"skins");


    //Commander - Added: Invisible Mode [TPT] - Start
    m_bInvisibleMode = ini.GetBool(_T("InvisibleMode"), false);
	m_iInvisibleModeHotKeyModifier = ini.GetInt(_T("InvisibleModeHKKeyModifier"), MOD_CONTROL | MOD_SHIFT | MOD_ALT);
	m_cInvisibleModeHotKey = (char)ini.GetInt(_T("InvisibleModeHKKey"),(int)'E');
    SetInvisibleMode(m_bInvisibleMode  ,m_iInvisibleModeHotKeyModifier ,m_cInvisibleModeHotKey );
	//Commander - Added: Invisible Mode [TPT] - End

    //MORPH START - Added by Commander, ClientQueueProgressBar
	m_bClientQueueProgressBar=ini.GetBool(_T("ClientQueueProgressBar"),false);
    //MORPH END - Added by Commander, ClientQueueProgressBar
	
	//MORPH START - Added by Commander, Show WC stats
	m_bCountWCSessionStats=ini.GetBool(_T("CountWCSessionStats"),false);
    //MORPH END - Added by Commander, Show WC stats

	//MORPH START - Added by Commander, FolderIcons
	m_bShowFolderIcons=ini.GetBool(_T("ShowFolderIcons"),false);
	//MORPH END - Added by Commander, FolderIcons

    m_bShowClientPercentage=ini.GetBool(_T("ShowClientPercentage"),false);  //Commander - Added: Client Percentage
	enableDownloadInRed = ini.GetBool(_T("EnableDownloadInRed"), true); //MORPH - Added by IceCream, show download in red
	m_bShowActiveDownloadsBold = ini.GetBool(_T("EnableDownloadInBold"), true); //MORPH - Added by SiRoB, show download in Bold
	enableAntiLeecher = ini.GetBool(_T("EnableAntiLeecher"), true); //MORPH - Added by IceCream, enable AntiLeecher
	enableAntiCreditHack = ini.GetBool(_T("EnableAntiCreditHack"), true); //MORPH - Added by IceCream, enable AntiCreditHack
	enableHighProcess = ini.GetBool(_T("EnableHighProcess"), false); //MORPH - Added by IceCream, high process priority
	creditSystemMode = ini.GetInt(_T("CreditSystemMode"), 0/*Officiel*/); // EastShare - Added by linekin, ES CreditSystem
	m_bEnableEqualChanceForEachFile = ini.GetBool(_T("EqualChanceForEachFile"), false);//Morph - added by AndCycle, Equal Chance For Each File
        
	//MORPH START added by Yun.SF3: Ipfilter.dat update
	LPBYTE pst = NULL;
	UINT usize = sizeof m_IPfilterVersion;
	if (ini.GetBinary(_T("IPfilterVersion"), &pst, &usize) && usize == sizeof m_IPfilterVersion)
		memcpy(&m_IPfilterVersion, pst, sizeof m_IPfilterVersion);
	else
		memset(&m_IPfilterVersion, 0, sizeof m_IPfilterVersion);
	delete[] pst;
	AutoUpdateIPFilter=ini.GetBool(_T("AutoUPdateIPFilter"),false); //added by milobac: Ipfilter.dat update
	//MORPH END added by Yun.SF3: Ipfilter.dat update
    
    //Commander - Added: IP2Country Auto-updating - Start
	pst = NULL;
	usize = sizeof m_IP2CountryVersion;
	if (ini.GetBinary(_T("IP2CountryVersion"), &pst, &usize) && usize == sizeof m_IP2CountryVersion)
		memcpy(&m_IP2CountryVersion, pst, sizeof m_IP2CountryVersion);
	else
		memset(&m_IP2CountryVersion, 0, sizeof m_IP2CountryVersion);
	delete[] pst;
	AutoUpdateIP2Country=ini.GetBool(_T("AutoUPdateIP2Country"),false);
    //Commander - Added: IP2Country Auto-updating - End

	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	pst = NULL;
	usize = sizeof m_FakesDatVersion;
	if (ini.GetBinary(_T("FakesDatVersion"), &pst, &usize) && usize == sizeof m_FakesDatVersion)
		memcpy(&m_FakesDatVersion, pst, sizeof m_FakesDatVersion);
	else
		memset(&m_FakesDatVersion, 0, sizeof m_FakesDatVersion);
	delete[] pst;
	UpdateFakeStartup=ini.GetBool(_T("UpdateFakeStartup"),false);
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating

	//EastShare - added by AndCycle, IP to Country
	m_iIP2CountryNameMode = ini.GetInt(_T("IP2Country"), IP2CountryName_DISABLE); 
	m_bIP2CountryShowFlag = ini.GetBool(_T("IP2CountryShowFlag"), false);
	//EastShare - added by AndCycle, IP to Country
	
	//MORPH START - Added by SiRoB, Datarate Average Time Management
	m_iDownloadDataRateAverageTime = 1000*max(1, (uint8)ini.GetInt(_T("DownloadDataRateAverageTime"),30));
	m_iUploadDataRateAverageTime = 1000*max(1, (uint8)ini.GetInt(_T("UploadDataRateAverageTime"),30));
	//MORPH END   - Added by SiRoB, Datarate Average Time Management

	//MORPH START - Added by SiRoB, Upload Splitting Class
	globaldataratefriend=ini.GetInt(_T("GlobalDataRateFriend"),3);
	maxglobaldataratefriend=ini.GetInt(_T("MaxGlobalDataRateFriend"),100);
	globaldataratepowershare=ini.GetInt(_T("GlobalDataRatePowerShare"),0);
	maxglobaldataratepowershare=ini.GetInt(_T("MaxGlobalDataRatePowerShare"),100);
	maxclientdataratefriend=ini.GetInt(_T("MaxClientDataRateFriend"),0);
	maxclientdataratepowershare=ini.GetInt(_T("MaxClientDataRatePowerShare"),0);
	maxclientdatarate=ini.GetInt(_T("MaxClientDataRate"),0);
	//MORPH END   - Added by SiRoB, Upload Splitting Class
    // ==> Slot Limit - Stulle
	m_bSlotLimitThree = ini.GetBool(_T("SlotLimitThree"),true); // default
	if (!m_bSlotLimitThree)
		m_bSlotLimitNum = ini.GetBool(_T("SlotLimitNumB"),false);
	else
		m_bSlotLimitNum = false;
	int temp = ini.GetInt(_T("SlotLimitNum"),100);
	m_iSlotLimitNum = (uint8)((temp >= 60 && temp <= 255) ? temp : 100);
	// <== Slot Limit - Stulle


	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	LowIdRetries=ini.GetInt(_T("ReconnectOnLowIdRetries"),3);	// SLUGFILLER: lowIdRetry
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry
	//MORPH	Start	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	m_iSpreadbarSetStatus = ini.GetInt(_T("SpreadbarSetStatus"), 0);
	//MORPH	End	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	hideOS=ini.GetInt(_T("HideOvershares"),0/*5*/);
	selectiveShare=ini.GetBool(_T("SelectiveShare"),false);
	//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	ShareOnlyTheNeed=ini.GetInt(_T("ShareOnlyTheNeed"),false);
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	PowerShareLimit=ini.GetInt(_T("PowerShareLimit"),0);
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	//MORPH START - Added by SiRoB, Show Permissions
	permissions=ini.GetInt(_T("ShowSharePermissions"),0);
	//MORPH END   - Added by SiRoB, Show Permissions
	//EastShare - Added by Pretender, TBH-AutoBackup
	autobackup = ini.GetBool(_T("AutoBackup"),true);
	autobackup2 = ini.GetBool(_T("AutoBackup2"),true);
	//EastShare - Added by Pretender, TBH-AutoBackup
	m_bSolidGraph = ini.GetBool(_T("SolidGraph"), false); //MORPH - Added by SiRoB, New Graph
	infiniteQueue=ini.GetBool(_T("InfiniteQueue"),false);	//Morph - added by AndCycle, SLUGFILLER: infiniteQueue
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	m_iPowershareMode=ini.GetInt(_T("PowershareMode"),2);
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	m_bPowershareInternalPrio = ini.GetBool(_T("PowershareInternalPrio"),false);//Morph - added by AndCyle, selective PS internal Prio

	//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	m_bSUCEnabled = ini.GetBool(_T("SUCEnabled"),false);
	m_bSUCLog =  ini.GetBool(_T("SUCLog"),false);
	m_iSUCHigh = ini.GetInt(_T("SUCHigh"),900);
	m_iSUCHigh = min(max(m_iSUCHigh,350),1000);
	m_iSUCLow = ini.GetInt(_T("SUCLow"),600);
	m_iSUCLow = min(max(m_iSUCLow,350),m_iSUCHigh);
	m_iSUCPitch = ini.GetInt(_T("SUCPitch"),3000);
	m_iSUCPitch = min(max(m_iSUCPitch,2500),10000);
	m_iSUCDrift = ini.GetInt(_T("SUCDrift"),50);
	m_iSUCDrift = min(max(m_iSUCDrift,0),100);
	//MORPH END - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	maxconnectionsswitchborder = ini.GetInt(_T("MaxConnectionsSwitchBorder"),100);//MORPH - Added by Yun.SF3, Auto DynUp changing
	maxconnectionsswitchborder = min(max(maxconnectionsswitchborder,50),60000);//MORPH - Added by Yun.SF3, Auto DynUp changing

	//EastShare Start - PreferShareAll by AndCycle
	shareall=ini.GetBool(_T("ShareAll"),true);	// SLUGFILLER: preferShareAll
	//EastShare END - PreferShareAll by AndCycle
	// EastShare START - Added by TAHO, .met file control
	m_iKnownMetDays = ini.GetInt(_T("KnownMetDays"), 90);
	if (m_iKnownMetDays == 0) m_iKnownMetDays = 90;
	// EastShare END - Added by TAHO, .met file control
	//EastShare - Added by Pretender, Option for ChunkDots
	m_bEnableChunkDots=ini.GetBool(_T("EnableChunkDots"),true);
	//EastShare - Added by Pretender, Option for ChunkDots

	isautodynupswitching=ini.GetBool(_T("AutoDynUpSwitching"),false);
	m_bDateFileNameLog=ini.GetBool(_T("DateFileNameLog"), true);//Morph - added by AndCycle, Date File Name Log
	m_bPayBackFirst=ini.GetBool(_T("IsPayBackFirst"),false);//EastShare - added by AndCycle, Pay Back First
	m_iPayBackFirstLimit=(uint8)min(ini.GetInt(_T("PayBackFirstLimit"),10),255);//MORPH - Added by SiRoB, Pay Back First Tweak
	m_bOnlyDownloadCompleteFiles = ini.GetBool(_T("OnlyDownloadCompleteFiles"), false);//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	m_bSaveUploadQueueWaitTime = ini.GetBool(_T("SaveUploadQueueWaitTime"), false/*true changed by sirob*/);//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	m_bDontRemoveSpareTrickleSlot = ini.GetBool(_T("DontRemoveSpareTrickleSlot"), true);//Morph - added by AndCycle, Dont Remove Spare Trickle Slot
	m_bFunnyNick = ini.GetBool(_T("DisplayFunnyNick"), true);//MORPH - Added by SiRoB, Optionnal funnynick display
	_stprintf(UpdateURLFakeList,_T("%s"),ini.GetString(_T("UpdateURLFakeList"),_T("http://emulepawcio.sourceforge.net/nieuwe_site/Ipfilter_fakes/fakes.zip")));		//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	if (!_tcsicmp(UpdateURLFakeList, _T("http://emulepawcio.sourceforge.net/nieuwe_site/Ipfilter_fakes/fakes")))
		_stprintf(UpdateURLFakeList ,_T("%s"), _T("http://emulepawcio.sourceforge.net/nieuwe_site/Ipfilter_fakes/fakes.zip"));
	_stprintf(UpdateURLIPFilter,_T("%s"),ini.GetString(_T("UpdateURLIPFilter"),_T("http://emulepawcio.sourceforge.net/nieuwe_site/Ipfilter_fakes/ipfilter.zip")));//MORPH START added by Yun.SF3: Ipfilter.dat update
	if (!_tcsicmp(UpdateURLIPFilter, _T("http://emulepawcio.sourceforge.net/nieuwe_site/Ipfilter_fakes/ipfilter")))
		_stprintf(UpdateURLIPFilter ,_T("%s"), _T("http://emulepawcio.sourceforge.net/nieuwe_site/Ipfilter_fakes/ipfilter.zip"));
	_stprintf(UpdateURLIP2Country,_T("%s"),ini.GetString(_T("UpdateURLIP2Country"),_T("http://ip-to-country.webhosting.info/downloads/ip-to-country.csv.zip")));//Commander - Added: IP2Country auto-updating

	// khaos::categorymod+ Load Preferences
	m_bShowCatNames=ini.GetBool(_T("ShowCatName"),true);
	m_bValidSrcsOnly=ini.GetBool(_T("ValidSrcsOnly"), false);
	m_bActiveCatDefault=ini.GetBool(_T("ActiveCatDefault"), true);
	m_bSelCatOnAdd=ini.GetBool(_T("SelCatOnAdd"), true);
	m_bAutoSetResumeOrder=ini.GetBool(_T("AutoSetResumeOrder"), true);
	m_bSmallFileDLPush=ini.GetBool(_T("SmallFileDLPush"), true);
	m_iStartDLInEmptyCats=(uint8)ini.GetInt(_T("StartDLInEmptyCats"), 0);
	m_bUseAutoCat=ini.GetBool(_T("UseAutoCat"), true);
	// khaos::categorymod-
	// khaos::kmod+
	m_bUseSaveLoadSources=ini.GetBool(_T("UseSaveLoadSources"), true);
	m_bRespectMaxSources=ini.GetBool(_T("RespectMaxSources"), true);
	m_bSmartA4AFSwapping=ini.GetBool(_T("SmartA4AFSwapping"), true);
	m_iAdvancedA4AFMode=(uint8)ini.GetInt(_T("AdvancedA4AFMode"), 1);
	m_bShowA4AFDebugOutput=ini.GetBool(_T("ShowA4AFDebugOutput"), false);
	// khaos::accuratetimerem+
	m_iTimeRemainingMode=(uint8)ini.GetInt(_T("TimeRemainingMode"), 0);
	// khaos::accuratetimerem-
	//MORPH START - Added by SiRoB, ICS Optional
	m_bUseIntelligentChunkSelection=ini.GetBool(_T("UseIntelligentChunkSelection"), true);
	//MORPH END   - Added by SiRoB, ICS Optional
	//MORPH START - Added by SiRoB, XML News [O²]
	enableNEWS=ini.GetBool(_T("ShowNews"), 1);
	//MORPH END   - Added by SiRoB, XML News [O²]

	LPBYTE pData = NULL;
	UINT uSize = sizeof m_lfHyperText;
	if (ini.GetBinary(L"HyperTextFont", &pData, &uSize) && uSize == sizeof m_lfHyperText)
		memcpy(&m_lfHyperText, pData, sizeof m_lfHyperText);
	else
		memset(&m_lfHyperText, 0, sizeof m_lfHyperText);
	delete[] pData;

	pData = NULL;
	uSize = sizeof m_lfLogText;
	if (ini.GetBinary(L"LogTextFont", &pData, &uSize) && uSize == sizeof m_lfLogText)
		memcpy(&m_lfLogText, pData, sizeof m_lfLogText);
	else
		memset(&m_lfLogText, 0, sizeof m_lfLogText);
	delete[] pData;

	m_crLogError = ini.GetColRef(L"LogErrorColor", m_crLogError);
	m_crLogWarning = ini.GetColRef(L"LogWarningColor", m_crLogWarning);
	m_crLogSuccess = ini.GetColRef(L"LogSuccessColor", m_crLogSuccess);
	//MORPH START - Added by SiRoB, Upload Splitting Class
	m_crLogUSC = ini.GetColRef(_T("LogUploadSplittingClassColor"), m_crLogUSC);
	//MORPH END   - Added by SiRoB, Upload Splitting Class

	if (statsAverageMinutes < 1)
		statsAverageMinutes = 5;

	// ZZ:UploadSpeedSense -->
	if (!m_bSUCEnabled) 	//MORPH START - Added by SiRoB,  Morph parameter transfer (USS)
	m_bDynUpEnabled = ini.GetBool(L"USSEnabled", false);
    m_bDynUpUseMillisecondPingTolerance = ini.GetBool(L"USSUseMillisecondPingTolerance", false);
    m_iDynUpPingTolerance = ini.GetInt(L"USSPingTolerance", 500);
	m_iDynUpPingToleranceMilliseconds = ini.GetInt(L"USSPingToleranceMilliseconds", 200);
	if( minupload < 1 )
		minupload = 1;
	m_iDynUpGoingUpDivider = ini.GetInt(L"USSGoingUpDivider", 1000);
    m_iDynUpGoingDownDivider = ini.GetInt(L"USSGoingDownDivider", 1000);
    m_iDynUpNumberOfPings = ini.GetInt(L"USSNumberOfPings", 1);
	// ZZ:UploadSpeedSense <--

	//MORPH START - Added by SiRoB,  USS log flag
	m_bDynUpLog = ini.GetBool(_T("USSLog"), true);
	//MORPH END   - Added by SiRoB,  USS log flag
	m_bUSSUDP = ini.GetBool(_T("USSUDP_FORCE"), false); //MORPH - Added by SiRoB, USS UDP preferency
	m_bA4AFSaveCpu = ini.GetBool(L"A4AFSaveCpu", false); // ZZ:DownloadManager
    m_bHighresTimer = ini.GetBool(L"HighresTimer", false);
	m_bRunAsUser = ini.GetBool(L"RunAsUnprivilegedUser", false);
	m_bPreferRestrictedOverUser = ini.GetBool(L"PreferRestrictedOverUser", false);
	m_bOpenPortsOnStartUp = ini.GetBool(L"OpenPortsOnStartUp", false);
	m_byLogLevel = ini.GetInt(L"DebugLogLevel", DLP_VERYLOW);
	m_bTrustEveryHash = ini.GetBool(L"AICHTrustEveryHash", false);
	m_bRememberCancelledFiles = ini.GetBool(L"RememberCancelledFiles", true);
	m_bRememberDownloadedFiles = ini.GetBool(L"RememberDownloadedFiles", true);

	m_bNotifierSendMail = ini.GetBool(L"NotifierSendMail", false);
#if _ATL_VER >= 0x0710
	if (!IsRunningXPSP2())
		m_bNotifierSendMail = false;
#endif
	m_strNotifierMailSender = ini.GetString(L"NotifierMailSender", L"");
	m_strNotifierMailServer = ini.GetString(L"NotifierMailServer", L"");
	m_strNotifierMailReceiver = ini.GetString(L"NotifierMailRecipient", L"");

	m_bWinaTransToolbar = ini.GetBool(L"WinaTransToolbar", false);

	// Mighty Knife: Community visualization, Report hashing files, Log friendlist activities
	_stprintf (m_sCommunityName,_T("%s"),ini.GetString (_T("CommunityName")));
	m_bReportHashingFiles = ini.GetBool (_T("ReportHashingFiles"),true);
	m_bLogFriendlistActivities = ini.GetBool (_T("LogFriendlistActivities"),true);
	// [end] Mighty Knife

	// Mighty Knife: CRC32-Tag
	SetDontAddCRCToFilename (ini.GetBool (_T("DontAddCRC32ToFilename"),false));
	SetCRC32ForceUppercase (ini.GetBool (_T("ForceCRC32Uppercase"),false));
	SetCRC32ForceAdding (ini.GetBool (_T("ForceCRC32Adding"),false));
	// From the prefix/suffix delete the leading/trailing "".
	SetCRC32Prefix (ini.GetString(_T("LastCRC32Prefix"),_T("\" [\"")).Trim (_T("\"")));
	SetCRC32Suffix (ini.GetString(_T("LastCRC32Suffix"),_T("\"]\"")).Trim (_T("\"")));
	// [end] Mighty Knife

	// Mighty Knife: Simple cleanup options
	SetSimpleCleanupOptions (ini.GetInt (_T("SimpleCleanupOptions"),3));
	SetSimpleCleanupSearch (ini.GetString (_T("SimpleCleanupSearch")));
	SetSimpleCleanupReplace (ini.GetString (_T("SimpleCleanupReplace")));
	// Format of the preferences string for character replacement:
	//      "str";"str";"str";...;"str"
	// Every "str" in SimpleCleanupSearchChars corresponds to a "str"
	// in SimpleCleanupReplaceChars at the same position.
	SetSimpleCleanupSearchChars (ini.GetString (_T("SimpleCleanupSearchChars"),
								 _T("\"\xE4\";\"\xF6\";\"\xFC\";\"\xC4\";\"\xD6\";\"\xDC\";\"\xDF\"")));/*ISO 8859-4*/
	SetSimpleCleanupReplaceChars (ini.GetString (_T("SimpleCleanupReplaceChars"),
								 _T("\"ae\";\"oe\";\"ue\";\"Ae\";\"Oe\";\"Ue\";\"ss\"")));
	// [end] Mighty Knife

	// Mighty Knife: Static server handling
	SetDontRemoveStaticServers (ini.GetBool (_T("DontRemoveStaticServers"),false));
	// [end] Mighty Knife

	///////////////////////////////////////////////////////////////////////////
	// Section: "Proxy"
	//
	proxy.EnablePassword = ini.GetBool(L"ProxyEnablePassword",false,L"Proxy");
	proxy.UseProxy = ini.GetBool(L"ProxyEnableProxy",false,L"Proxy");
	proxy.name = CStringA(ini.GetString(L"ProxyName", L"", L"Proxy"));
	proxy.user = CStringA(ini.GetString(L"ProxyUser", L"", L"Proxy"));
	proxy.password = CStringA(ini.GetString(L"ProxyPassword", L"", L"Proxy"));
	proxy.port = (uint16)ini.GetInt(L"ProxyPort",1080,L"Proxy");
	proxy.type = (uint16)ini.GetInt(L"ProxyType",PROXYTYPE_NOPROXY,L"Proxy");


	///////////////////////////////////////////////////////////////////////////
	// Section: "Statistics"
	//
	statsSaveInterval = ini.GetInt(L"SaveInterval", 60, L"Statistics");
	statsConnectionsGraphRatio = ini.GetInt(L"statsConnectionsGraphRatio", 3, L"Statistics");
	_stprintf(statsExpandedTreeItems,L"%s",ini.GetString(L"statsExpandedTreeItems",L"111000000100000110000010000011110000010010",L"Statistics"));
	CString buffer2;
	for (int i=0;i<ARRSIZE(m_adwStatsColors);i++) {
		buffer2.Format(L"StatColor%i", i);
		_stprintf(buffer, L"%s", ini.GetString(buffer2, L"", L"Statistics"));
		m_adwStatsColors[i] = 0;
		if (_stscanf(buffer, L"%i", &m_adwStatsColors[i]) != 1)
			ResetStatsColor(i);
	}
	m_bShowVerticalHourMarkers = ini.GetBool(L"ShowVerticalHourMarkers", true, L"Statistics");

	// -khaos--+++> Load Stats
	// I changed this to a seperate function because it is now also used
	// to load the stats backup and to load stats from preferences.ini.old.
	LoadStats();
	// <-----khaos-

	///////////////////////////////////////////////////////////////////////////
	// Section: "WebServer"
	//
	_stprintf(m_sWebPassword,L"%s",ini.GetString(L"Password", L"",L"WebServer"));
	_stprintf(m_sWebLowPassword,L"%s",ini.GetString(L"PasswordLow", L""));
	m_nWebPort=(uint16)ini.GetInt(L"Port", 4711);
	m_bWebEnabled=ini.GetBool(L"Enabled", false);
	m_bWebUseGzip=ini.GetBool(L"UseGzip", true);
	m_bWebLowEnabled=ini.GetBool(L"UseLowRightsUser", false);
	m_nWebPageRefresh=ini.GetInt(L"PageRefreshTime", 120);
	m_iWebTimeoutMins=ini.GetInt(L"WebTimeoutMins", 5 );
	m_iWebFileUploadSizeLimitMB=ini.GetInt(L"MaxFileUploadSizeMB", 5 );
	m_bAllowAdminHiLevFunc=ini.GetBool(L"AllowAdminHiLevelFunc", false);
	buffer2 = ini.GetString(L"AllowedIPs");
	int iPos = 0;
	CString strIP = buffer2.Tokenize(L";", iPos);
	while (!strIP.IsEmpty())
	{
		u_long nIP = inet_addr(CStringA(strIP));
		if (nIP != INADDR_ANY && nIP != INADDR_NONE)
			m_aAllowedRemoteAccessIPs.Add(nIP);
		strIP = buffer2.Tokenize(L";", iPos);
	}

	///////////////////////////////////////////////////////////////////////////
	// Section: "MobileMule"
	//
	_stprintf(m_sMMPassword,L"%s",ini.GetString(L"Password", L"",L"MobileMule"));
	m_bMMEnabled = ini.GetBool(L"Enabled", false);
	m_nMMPort = (uint16)ini.GetInt(L"Port", 80);

	///////////////////////////////////////////////////////////////////////////
	// Section: "PeerCache"
	//
	m_uPeerCacheLastSearch = ini.GetInt(L"LastSearch", 0, L"PeerCache");
	m_bPeerCacheWasFound = ini.GetBool(L"Found", false);
	m_bPeerCacheEnabled = ini.GetBool(L"Enabled", true);
	m_nPeerCachePort = (uint16)ini.GetInt(L"PCPort", 0);
	m_bPeerCacheShow = ini.GetBool(L"Show", false);

    //MORPH START - Added by SiRoB, [MoNKi: -UPnPNAT Support-]
	m_bUPnPNat = ini.GetBool(_T("UPnPNAT"), false, _T("eMule"));
	m_bUPnPNatWeb = ini.GetBool(_T("UPnPNAT_Web"), false, _T("eMule"));
	m_bUPnPVerboseLog = ini.GetBool(_T("UPnPVerbose"), true, _T("eMule"));
	m_iUPnPPort = (uint16)ini.GetInt(_T("UPnPPort"), 0, _T("eMule"));
	m_bUPnPLimitToFirstConnection = ini.GetBool(_T("UPnPLimitToFirstConnection"), false, _T("eMule"));
	m_bUPnPClearOnClose = ini.GetBool(_T("UPnPClearOnClose"), true, _T("eMule"));
    SetUpnpDetect(ini.GetInt(_T("uPnPDetect"), UPNP_DO_AUTODETECT, _T("eMule"))); //leuk_he autodetect upnp in wizard
	//MORPH END   - Added by SiRoB, [MoNKi: -UPnPNAT Support-]

	//MORPH START - Added by SiRoB, [MoNKi: -Random Ports-]
	m_bRndPorts = ini.GetBool(_T("RandomPorts"), false, _T("eMule"));
	m_iMinRndPort = (uint16)ini.GetInt(_T("MinRandomPort"), 3000, _T("eMule"));
	m_iMaxRndPort = (uint16)ini.GetInt(_T("MaxRandomPort"), 0xFFFF, _T("eMule"));
	m_bRndPortsResetOnRestart = ini.GetBool(_T("RandomPortsReset"), false, _T("eMule"));
	m_iRndPortsSafeResetOnRestartTime = (uint16)ini.GetInt(_T("RandomPortsSafeResetOnRestartTime"), 0, _T("eMule"));
	
	uint16 iOldRndTCPPort = (uint16)ini.GetInt(_T("OldTCPRandomPort"), 0, _T("eMule"));
	uint16 iOldRndUDPPort = (uint16)ini.GetInt(_T("OldUDPRandomPort"), 0, _T("eMule"));
	__time64_t iRndPortsLastRun = ini.GetUInt64(_T("RandomPortsLastRun"), 0, _T("eMule"));
	
	m_iCurrentTCPRndPort = 0;
	m_iCurrentUDPRndPort = 0;

	if(m_bRndPorts && !m_bRndPortsResetOnRestart &&
		m_iRndPortsSafeResetOnRestartTime != 0)
	{
		CTime tNow = CTime::GetCurrentTime();
		CTime tOld = CTime(iRndPortsLastRun);
		CTimeSpan ts = tNow - tOld;
		if(ts.GetTimeSpan() <= m_iRndPortsSafeResetOnRestartTime){
			m_iCurrentTCPRndPort = iOldRndTCPPort;
			m_iCurrentUDPRndPort = iOldRndUDPPort;
		}
	}
	m_bRndPortsResetOnRestart = false;
	//MORPH END   - Added by SiRoB, [MoNKi: -Random Ports-]

	//MORPH START - Added by by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
	m_bICFSupportStatusChanged = false;
	m_bICFSupport = ini.GetBool(_T("ICFSupport"), false, _T("eMule"));
	m_bICFSupportFirstTime = ini.GetBool(_T("ICFSupportFirstTime"), true, _T("eMule"));
	m_bICFSupportServerUDP = ini.GetBool(_T("ICFSupportServerUDP"), false, _T("eMule"));
	//MORPH END   - Added by by SiRoB, [MoNKi: -Improved ICS-Firewall support-]

	///////////////////////////////////////////////////////////////////////////
	// Section: "WapServer"
	//
    //MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
	m_bWapEnabled=ini.GetBool(_T("WapEnabled"), false, _T("WapServer"));
	_stprintf(m_sWapTemplateFile,_T("%s"),ini.GetString(_T("WapTemplateFile"),_T("eMule_Wap.tmpl"),_T("WapServer")));
	m_nWapPort=(uint16)ini.GetInt(_T("WapPort"), 80, _T("WapServer"));
	m_iWapGraphWidth=ini.GetInt(_T("WapGraphWidth"), 60, _T("WapServer"));
	m_iWapGraphHeight=ini.GetInt(_T("WapGraphHeight"), 45, _T("WapServer"));
	m_bWapFilledGraphs=ini.GetBool(_T("WapFilledGraphs"), false, _T("WapServer"));
	m_iWapMaxItemsInPages = ini.GetInt(_T("WapMaxItemsInPage"), 5, _T("WapServer"));
	m_bWapSendImages=ini.GetBool(_T("WapSendImages"), true, _T("WapServer"));
	m_bWapSendGraphs=ini.GetBool(_T("WapSendGraphs"), true, _T("WapServer"));
	m_bWapSendProgressBars=ini.GetBool(_T("WapSendProgressBars"), true, _T("WapServer"));
	m_bWapAllwaysSendBWImages=ini.GetBool(_T("WapSendBWImages"), true, _T("WapServer"));
	m_iWapLogsSize=ini.GetInt(_T("WapLogsSize"), 1024, _T("WapServer"));
	m_sWapPassword = ini.GetString(_T("WapPassword"), _T("WapServer"), _T("WapServer"));
	m_sWapLowPassword = ini.GetString(_T("WapPasswordLow"), _T(""), _T("WapServer"));
	m_bWapLowEnabled = ini.GetBool(_T("WapLowEnable"), false, _T("WapServer"));
	//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]

	//MORPH START - Added by Stulle, Global Source Limit
	m_bGlobalHL = ini.GetBool(_T("GlobalHL"), false, _T("eMule"));
	uint32 m_uGlobalHlStandard = 3500;
	if (maxGraphUploadRate != UNLIMITED)
	{
		m_uGlobalHlStandard = (uint32)(maxGraphUploadRate*0.9f);
		m_uGlobalHlStandard = (uint32)((m_uGlobalHlStandard*400 - (m_uGlobalHlStandard-10.0f)*100)*0.65f);
		m_uGlobalHlStandard = max(1000,min(m_uGlobalHlStandard,MAX_GSL));
	}
	uint32 m_uTemp = ini.GetInt(_T("GlobalHLvalue"), m_uGlobalHlStandard);
	m_uGlobalHL = (m_uTemp >= 1000 && m_uTemp <= MAX_GSL) ? m_uTemp : m_uGlobalHlStandard;
	//MORPH END   - Added by Stulle, Global Source Limit

    LoadCats();
	SetLanguage();
}


WORD CPreferences::GetWindowsVersion(){
	static bool bWinVerAlreadyDetected = false;
	if(!bWinVerAlreadyDetected)
	{	
		bWinVerAlreadyDetected = true;
		m_wWinVer = DetectWinVersion();	
	}	
	return m_wWinVer;
}

UINT CPreferences::GetDefaultMaxConperFive(){
	switch (GetWindowsVersion()){
		case _WINVER_98_:
			return 5;
		case _WINVER_95_:	
		case _WINVER_ME_:
			return MAXCON5WIN9X;
		case _WINVER_2K_:
		case _WINVER_XP_:
			return MAXCONPER5SEC;
		default:
			return MAXCONPER5SEC;
	}
}

//////////////////////////////////////////////////////////
// category implementations
//////////////////////////////////////////////////////////

void CPreferences::SaveCats(){

	// Cats
	CString catinif,ixStr,buffer;
	catinif.Format(L"%sCategory.ini",configdir);
	_tremove(catinif);

	CIni catini( catinif, L"Category" );
	catini.WriteInt(L"Count",catMap.GetCount()-1,L"General");
	catini.WriteInt(_T("CategoryVersion"), 2, _T("General")); // khaos::categorymod+
	for (int ix=0;ix<catMap.GetCount();ix++){
		ixStr.Format(L"Cat#%i",ix);
		catini.WriteString(L"Title",catMap.GetAt(ix)->title,ixStr);
		catini.WriteString(L"Incoming",catMap.GetAt(ix)->incomingpath,ixStr);
		catini.WriteString(L"Comment",catMap.GetAt(ix)->comment,ixStr);
		buffer.Format(_T("%lu"),catMap.GetAt(ix)->color,ixStr);
		catini.WriteString(_T("Color"),buffer,ixStr);
		catini.WriteInt(_T("a4afPriority"),catMap.GetAt(ix)->prio,ixStr);
		catini.WriteInt(_T("AdvancedA4AFMode"), catMap.GetAt(ix)->iAdvA4AFMode, ixStr);
		/*Removed by SiRoB
		catini.WriteString(_T("AutoCat"),catMap.GetAt(ix)->autocat,ixStr); 
		*/
        catini.WriteBool(_T("downloadInAlphabeticalOrder"), catMap.GetAt(ix)->downloadInAlphabeticalOrder!=FALSE, ixStr);

		// khaos::categorymod+ Save View Filters
		catini.WriteInt(_T("vfFromCats"), catMap.GetAt(ix)->viewfilters.nFromCats, ixStr);
		catini.WriteBool(_T("vfVideo"), catMap.GetAt(ix)->viewfilters.bVideo!=FALSE, ixStr);
		catini.WriteBool(_T("vfAudio"), catMap.GetAt(ix)->viewfilters.bAudio!=FALSE, ixStr);
		catini.WriteBool(_T("vfArchives"), catMap.GetAt(ix)->viewfilters.bArchives!=FALSE, ixStr);
		catini.WriteBool(_T("vfImages"), catMap.GetAt(ix)->viewfilters.bImages!=FALSE, ixStr);
		catini.WriteBool(_T("vfWaiting"), catMap.GetAt(ix)->viewfilters.bWaiting!=FALSE, ixStr);
		catini.WriteBool(_T("vfTransferring"), catMap.GetAt(ix)->viewfilters.bTransferring!=FALSE, ixStr);
		catini.WriteBool(_T("vfPaused"), catMap.GetAt(ix)->viewfilters.bPaused!=FALSE, ixStr);
		catini.WriteBool(_T("vfStopped"), catMap.GetAt(ix)->viewfilters.bStopped!=FALSE, ixStr);
		catini.WriteBool(_T("vfComplete"), catMap.GetAt(ix)->viewfilters.bComplete!=FALSE, ixStr);
		catini.WriteBool(_T("vfHashing"), catMap.GetAt(ix)->viewfilters.bHashing!=FALSE, ixStr);
		catini.WriteBool(_T("vfErrorUnknown"), catMap.GetAt(ix)->viewfilters.bErrorUnknown!=FALSE, ixStr);
		catini.WriteBool(_T("vfCompleting"), catMap.GetAt(ix)->viewfilters.bCompleting!=FALSE, ixStr);
		catini.WriteBool(_T("vfSeenComplet"), catMap.GetAt(ix)->viewfilters.bSeenComplet!=FALSE, ixStr); //MORPH - Added by SiRoB, Seen Complet filter
		catini.WriteUInt64(_T("vfFSizeMin"), catMap.GetAt(ix)->viewfilters.nFSizeMin, ixStr);
		catini.WriteUInt64(_T("vfFSizeMax"), catMap.GetAt(ix)->viewfilters.nFSizeMax, ixStr);
		catini.WriteUInt64(_T("vfRSizeMin"), catMap.GetAt(ix)->viewfilters.nRSizeMin, ixStr);
		catini.WriteUInt64(_T("vfRSizeMax"), catMap.GetAt(ix)->viewfilters.nRSizeMax, ixStr);
		catini.WriteInt(_T("vfTimeRemainingMin"), catMap.GetAt(ix)->viewfilters.nTimeRemainingMin, ixStr);
		catini.WriteInt(_T("vfTimeRemainingMax"), catMap.GetAt(ix)->viewfilters.nTimeRemainingMax, ixStr);
		catini.WriteInt(_T("vfSourceCountMin"), catMap.GetAt(ix)->viewfilters.nSourceCountMin, ixStr);
		catini.WriteInt(_T("vfSourceCountMax"), catMap.GetAt(ix)->viewfilters.nSourceCountMax, ixStr);
		catini.WriteInt(_T("vfAvailSourceCountMin"), catMap.GetAt(ix)->viewfilters.nAvailSourceCountMin, ixStr);
		catini.WriteInt(_T("vfAvailSourceCountMax"), catMap.GetAt(ix)->viewfilters.nAvailSourceCountMax, ixStr);
		catini.WriteString(_T("vfAdvancedFilterMask"), catMap.GetAt(ix)->viewfilters.sAdvancedFilterMask, ixStr);
		// Save Selection Criteria
		catini.WriteBool(_T("scFileSize"), catMap.GetAt(ix)->selectioncriteria.bFileSize!=FALSE, ixStr);
		catini.WriteBool(_T("scAdvancedFilterMask"), catMap.GetAt(ix)->selectioncriteria.bAdvancedFilterMask!=FALSE, ixStr);
		// khaos::categorymod-
		catini.WriteBool(_T("ResumeFileOnlyInSameCat"), catMap.GetAt(ix)->bResumeFileOnlyInSameCat!=FALSE, ixStr); //MORPH - Added by SiRoB, Resume file only in the same category
	}
}
// khaos::categorymod+
void CPreferences::LoadCats() {
	CString ixStr,catinif;//,cat_a,cat_b,cat_c;
	TCHAR buffer[100];

	catinif.Format(_T("%sCategory.ini"),configdir);
	CIni catini;
	
	bool bCreateDefault = false;
	bool bSkipLoad = false;
	if (!PathFileExists(catinif))
	{
		bCreateDefault = true;
		bSkipLoad = true;
	}
	else
	{
		catini.SetFileName(catinif);
		catini.SetSection(_T("General"));
		if (catini.GetInt(_T("CategoryVersion")) == 0)
			bCreateDefault = true;
	}

	if (bCreateDefault)
	{
		Category_Struct* defcat=new Category_Struct;

		_stprintf(defcat->title,_T("Default"));
    	defcat->prio=PR_NORMAL; // ZZ:DownloadManager
		defcat->iAdvA4AFMode = 0;
		_stprintf(defcat->incomingpath, incomingdir);
		_stprintf(defcat->comment, _T("The default category.  It can't be merged or deleted."));
		defcat->color = 0;
		defcat->viewfilters.bArchives = true;
		defcat->viewfilters.bAudio = true;
		defcat->viewfilters.bComplete = true;
		defcat->viewfilters.bCompleting = true;
		defcat->viewfilters.bSeenComplet = true; //MORPH - Added by SiRoB, Seen Complet filter
		defcat->viewfilters.bErrorUnknown = true;
		defcat->viewfilters.bHashing = true;
		defcat->viewfilters.bImages = true;
		defcat->viewfilters.bPaused = true;
		defcat->viewfilters.bStopped = true;
		defcat->viewfilters.bSuspendFilters = false;
		defcat->viewfilters.bTransferring = true;
		defcat->viewfilters.bVideo = true;
		defcat->viewfilters.bWaiting = true;
		defcat->viewfilters.nAvailSourceCountMax = 0;
		defcat->viewfilters.nAvailSourceCountMin = 0;
		defcat->viewfilters.nFromCats = 2;
		defcat->viewfilters.nFSizeMax = 0;
		defcat->viewfilters.nFSizeMin = 0;
		defcat->viewfilters.nRSizeMax = 0;
		defcat->viewfilters.nRSizeMin = 0;
		defcat->viewfilters.nSourceCountMax = 0;
		defcat->viewfilters.nSourceCountMin = 0;
		defcat->viewfilters.nTimeRemainingMax = 0;
		defcat->viewfilters.nTimeRemainingMin = 0;
		defcat->viewfilters.sAdvancedFilterMask = "";
		defcat->selectioncriteria.bAdvancedFilterMask = true;
		defcat->selectioncriteria.bFileSize = true;
		defcat->bResumeFileOnlyInSameCat = false; //MORPH - Added by SiRoB, Resume file only in the same category
		AddCat(defcat);
		if (bSkipLoad)
		{
			SaveCats();
			return;
		}
	}

	int max=catini.GetInt(_T("Count"),0,_T("General"));

	for (int ix = bCreateDefault ? 1 : 0; ix <= max; ix++)
	{
		ixStr.Format(_T("Cat#%i"),ix);
        catini.SetSection(ixStr);

		Category_Struct* newcat = new Category_Struct;
		_stprintf(newcat->title,_T("%s"),catini.GetString(_T("Title"),_T(""),ixStr));
		_stprintf(newcat->incomingpath,_T("%s"),catini.GetString(_T("Incoming"),_T(""),ixStr));
		MakeFoldername(newcat->incomingpath);
		// SLUGFILLER: SafeHash remove - removed installation dir unsharing
		_stprintf(newcat->comment,_T("%s"),catini.GetString(_T("Comment"),_T(""),ixStr));
		newcat->prio =catini.GetInt(_T("a4afPriority"),PR_NORMAL,ixStr); // ZZ:DownloadManager
		_stprintf(buffer,_T("%s"),catini.GetString(_T("Color"),_T("0"),ixStr));
		newcat->color=(DWORD)_tstoi64(buffer);
		/*
		newcat->autocat=catini.GetString(_T("Autocat"),_T(""),ixStr);
		*/
        newcat->downloadInAlphabeticalOrder = catini.GetBool(_T("downloadInAlphabeticalOrder"), FALSE, ixStr); // ZZ:DownloadManager

		// khaos::kmod+ Category Advanced A4AF Mode
		newcat->iAdvA4AFMode = catini.GetInt(_T("AdvancedA4AFMode"), 0);
		//newcat->autocat = catini.GetString(_T("AutoCatString"),_T(""),ixStr);
		// khaos::kmod-
		// Load View Filters
		newcat->viewfilters.nFromCats = catini.GetInt(_T("vfFromCats"), ix==0?0:2);
		newcat->viewfilters.bSuspendFilters = false;
		newcat->viewfilters.bVideo = catini.GetBool(_T("vfVideo"), true);
		newcat->viewfilters.bAudio = catini.GetBool(_T("vfAudio"), true);
		newcat->viewfilters.bArchives = catini.GetBool(_T("vfArchives"), true);
		newcat->viewfilters.bImages = catini.GetBool(_T("vfImages"), true);
		newcat->viewfilters.bWaiting = catini.GetBool(_T("vfWaiting"), true);
		newcat->viewfilters.bTransferring = catini.GetBool(_T("vfTransferring"), true);
		newcat->viewfilters.bPaused = catini.GetBool(_T("vfPaused"), true);
		newcat->viewfilters.bStopped = catini.GetBool(_T("vfStopped"), true);
		newcat->viewfilters.bComplete = catini.GetBool(_T("vfComplete"), true);
		newcat->viewfilters.bHashing = catini.GetBool(_T("vfHashing"), true);
		newcat->viewfilters.bErrorUnknown = catini.GetBool(_T("vfErrorUnknown"), true);
		newcat->viewfilters.bCompleting = catini.GetBool(_T("vfCompleting"), true);
		newcat->viewfilters.bSeenComplet = catini.GetBool(_T("vfSeenComplet"), true); //MORPH - Added by SiRoB, Seen Complet filter
		newcat->viewfilters.nFSizeMin = catini.GetInt(_T("vfFSizeMin"), 0);
		newcat->viewfilters.nFSizeMax = catini.GetInt(_T("vfFSizeMax"), 0);
		newcat->viewfilters.nRSizeMin = catini.GetInt(_T("vfRSizeMin"), 0);
		newcat->viewfilters.nRSizeMax = catini.GetInt(_T("vfRSizeMax"), 0);
		newcat->viewfilters.nTimeRemainingMin = catini.GetInt(_T("vfTimeRemainingMin"), 0);
		newcat->viewfilters.nTimeRemainingMax = catini.GetInt(_T("vfTimeRemainingMax"), 0);
		newcat->viewfilters.nSourceCountMin = catini.GetInt(_T("vfSourceCountMin"), 0);
		newcat->viewfilters.nSourceCountMax = catini.GetInt(_T("vfSourceCountMax"), 0);
		newcat->viewfilters.nAvailSourceCountMin = catini.GetInt(_T("vfAvailSourceCountMin"), 0);
		newcat->viewfilters.nAvailSourceCountMax = catini.GetInt(_T("vfAvailSourceCountMax"), 0);
		newcat->viewfilters.sAdvancedFilterMask = catini.GetString(_T("vfAdvancedFilterMask"), _T(""));
		// Load Selection Criteria
		newcat->selectioncriteria.bFileSize = catini.GetBool(_T("scFileSize"), true);
		newcat->selectioncriteria.bAdvancedFilterMask = catini.GetBool(_T("scAdvancedFilterMask"), true);
		newcat->bResumeFileOnlyInSameCat = catini.GetBool(_T("ResumeFileOnlyInSameCat"), false); //MORPH - Added by SiRoB, Resume file only in the same category

		AddCat(newcat);
		if (!PathFileExists(newcat->incomingpath))
			::CreateDirectory(newcat->incomingpath, 0);
	}
}
// khaos::categorymod-

void CPreferences::RemoveCat(int index)	{
	if (index>=0 && index<catMap.GetCount()) { 
		Category_Struct* delcat;
		delcat=catMap.GetAt(index); 
		catMap.RemoveAt(index); 
		delete delcat;
	}
}

/*
bool CPreferences::SetCatFilter(int index, int filter){
	if (index>=0 && index<catMap.GetCount()) { 
		Category_Struct* cat;
		cat=catMap.GetAt(index); 
		cat->filter=filter;
		return true;
	} 
	
	return false;
}

int CPreferences::GetCatFilter(int index){
	if (index>=0 && index<catMap.GetCount()) {
		return catMap.GetAt(index)->filter;
	}
	
    return 0;
}

bool CPreferences::GetCatFilterNeg(int index){
	if (index>=0 && index<catMap.GetCount()) {
		return catMap.GetAt(index)->filterNeg;
	}
	
    return false;
}

void CPreferences::SetCatFilterNeg(int index, bool val) {
	if (index>=0 && index<catMap.GetCount()) {
		catMap.GetAt(index)->filterNeg=val;
	}
}
*/

bool CPreferences::MoveCat(UINT from, UINT to){
	if (from>=(UINT)catMap.GetCount() || to >=(UINT)catMap.GetCount()+1 || from==to) return false;

	Category_Struct* tomove;

	tomove=catMap.GetAt(from);

	if (from < to) {
		catMap.RemoveAt(from);
		catMap.InsertAt(to-1,tomove);
	} else {
		catMap.InsertAt(to,tomove);
		catMap.RemoveAt(from+1);
	}
	
	SaveCats();

	return true;
}


void CPreferences::UpdateLastVC()
{
	versioncheckLastAutomatic = safe_mktime(CTime::GetCurrentTime().GetLocalTm());
}
//MORPH START - Added by SiRoB, New Version check
void CPreferences::UpdateLastMVC()
{
	mversioncheckLastAutomatic = safe_mktime(CTime::GetCurrentTime().GetLocalTm());
}
//MORPH END   - Added by SiRoB, New Version check

void CPreferences::SetWSPass(CString strNewPass)
{
	_stprintf(m_sWebPassword, L"%s", MD5Sum(strNewPass).GetHash().GetBuffer(0));
}

void CPreferences::SetWSLowPass(CString strNewPass)
{
	_stprintf(m_sWebLowPassword, L"%s", MD5Sum(strNewPass).GetHash().GetBuffer(0));
}

void CPreferences::SetMMPass(CString strNewPass)
{
	_stprintf(m_sMMPassword, L"%s", MD5Sum(strNewPass).GetHash().GetBuffer(0));
}

void CPreferences::SetMaxUpload(UINT in)
{
	uint16 oldMaxUpload = (uint16)in;
	maxupload = (oldMaxUpload) ? oldMaxUpload : (uint16)UNLIMITED;
}

void CPreferences::SetMaxDownload(UINT in)
{
	uint16 oldMaxDownload = (uint16)in;
	maxdownload = (oldMaxDownload) ? oldMaxDownload : (uint16)UNLIMITED;
}

void CPreferences::SetNetworkKademlia(bool val)
{
	networkkademlia = val; 
}

CString CPreferences::GetHomepageBaseURLForLevel(int nLevel){
	CString tmp;
	if (nLevel == 0)
		tmp = L"http://emule-project.net";
	else if (nLevel == 1)
		tmp = L"http://www.emule-project.org";
	else if (nLevel == 2)
		tmp = L"http://www.emule-project.com";
	else if (nLevel < 100)
		tmp.Format(L"http://www%i.emule-project.net",nLevel-2);
	else if (nLevel < 150)
		tmp.Format(L"http://www%i.emule-project.org",nLevel);
	else if (nLevel < 200)
		tmp.Format(L"http://www%i.emule-project.com",nLevel);
	else if (nLevel == 200)
		tmp = L"http://emule.sf.net";
	else if (nLevel == 201)
		tmp = L"http://www.emuleproject.net";
	else if (nLevel == 202)
		tmp = L"http://sourceforge.net/projects/emule/";
	else
		tmp = L"http://www.emule-project.net";
	return tmp;
}

CString CPreferences::GetVersionCheckBaseURL(){
	CString tmp;
	UINT nWebMirrorAlertLevel = GetWebMirrorAlertLevel();
	if (nWebMirrorAlertLevel < 100)
		tmp = L"http://vcheck.emule-project.net";
	else if (nWebMirrorAlertLevel < 150)
		tmp.Format(L"http://vcheck%i.emule-project.org",nWebMirrorAlertLevel);
	else if (nWebMirrorAlertLevel < 200)
		tmp.Format(L"http://vcheck%i.emule-project.com",nWebMirrorAlertLevel);
	else if (nWebMirrorAlertLevel == 200)
		tmp = L"http://emule.sf.net";
	else if (nWebMirrorAlertLevel == 201)
		tmp = L"http://www.emuleproject.net";
	else
		tmp = L"http://vcheck.emule-project.net";
	return tmp;
}

bool CPreferences::IsDefaultNick(const CString strCheck){
	// not fast, but this function is called often
	for (int i = 0; i != 255; i++){
		if (GetHomepageBaseURLForLevel(i) == strCheck)
			return true;
	}
	return ( strCheck == L"http://emule-project.net" );
}

void CPreferences::SetUserNick(LPCTSTR pszNick)
{
	strNick = pszNick;
}

UINT CPreferences::GetWebMirrorAlertLevel(){
	// Known upcoming DDoS Attacks
	if (m_nWebMirrorAlertLevel == 0){
		// no threats known at this time
	}
	// end
	if (UpdateNotify())
		return m_nWebMirrorAlertLevel;
	else
		return 0;
}

bool CPreferences::IsRunAsUserEnabled(){
	return (GetWindowsVersion() == _WINVER_XP_ || GetWindowsVersion() == _WINVER_2K_) && m_bRunAsUser;
}

bool CPreferences::GetUseReBarToolbar()
{
	return GetReBarToolbar() && theApp.m_ullComCtrlVer >= MAKEDLLVERULL(5,8,0,0);
}

int	CPreferences::GetMaxGraphUploadRate(bool bEstimateIfUnlimited){
	if (maxGraphUploadRate != UNLIMITED || !bEstimateIfUnlimited){
		return maxGraphUploadRate;
	}
	else{
		if (maxGraphUploadRateEstimated != 0){
			return maxGraphUploadRateEstimated +4;
		}
		else
			return 16;
	}
}

void CPreferences::EstimateMaxUploadCap(uint32 nCurrentUpload){
	if (maxGraphUploadRateEstimated+1 < nCurrentUpload){
		maxGraphUploadRateEstimated = nCurrentUpload;
		if (maxGraphUploadRate == UNLIMITED && theApp.emuledlg && theApp.emuledlg->statisticswnd)
			theApp.emuledlg->statisticswnd->SetARange(false, thePrefs.GetMaxGraphUploadRate(true));
	}
}

void CPreferences::SetMaxGraphUploadRate(int in){
	maxGraphUploadRate	=(in) ? in : UNLIMITED;
}

bool CPreferences::IsDynUpEnabled()	{
	return m_bDynUpEnabled || maxGraphUploadRate == UNLIMITED;
}

bool CPreferences::CanFSHandleLargeFiles()	{
	bool bResult = false;
	for (int i = 0; i != tempdir.GetCount(); i++){
		if (!IsFileOnFATVolume(tempdir.GetAt(i))){
			bResult = true;
			break;
		}
	}
	return bResult && !IsFileOnFATVolume(GetIncomingDir());
}
//MORPH START - Added by IceCream, high process priority
void CPreferences::SetEnableHighProcess(bool enablehigh) 			
{
	enableHighProcess = enablehigh;
	if (enablehigh)
		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	else
		SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
}
//MORPH END   - Added by IceCream, high process priority

//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
bool	CPreferences::IsSUCDoesWork()
{
	return minupload<maxupload && maxupload != UNLIMITED && m_bSUCEnabled;
}
//MORPH END  - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]

//MORPH START - Added by SiRoB, (SUC) & (USS)
void	CPreferences::SetMinUpload(UINT in)
{
	minupload = (uint16)((in) ? in : UNLIMITED);
}
//MORPH END  - Added by SiRoB, (SUC) & (USS)

//Commander - Added: Invisible Mode [TPT] - Start
void CPreferences::SetInvisibleMode(bool on, UINT keymodifier, char key) 
{
	m_bInvisibleMode = on;
	m_iInvisibleModeHotKeyModifier = keymodifier;
	m_cInvisibleModeHotKey = key;
	if(theApp.emuledlg!=NULL){
		//Always unregister, the keys could be different.
		theApp.emuledlg->UnRegisterInvisibleHotKey();
		if(m_bInvisibleMode)	theApp.emuledlg->RegisterInvisibleHotKey();
	}
}
//Commander - Added: Invisible Mode [TPT] - End

//MORPH START - Added by SiRoB, [MoNKi: -UPnPNAT Support-]
// emulEspaña: Modified by MoNKi [MoNKi: -Random Ports-]
/*
uint16	CPreferences::GetPort(){
*/
uint16	CPreferences::GetPort(bool newPort, bool original, bool reset){
// End -Random Ports-

	// emulEspaña: Added by MoNKi [MoNKi: -Random Ports-]
	if(original)
		return port;

	if(reset){
		m_iCurrentTCPRndPort = 0;
	}
// End -Random Ports-

	// emulEspaña: Added by MoNKi [MoNKi: -Random Ports-]
	if (m_iCurrentTCPRndPort == 0 || newPort){
		if(GetUseRandomPorts())
			do{
				m_iCurrentTCPRndPort = GetMinRandomPort() + (uint16)((((float)rand() / RAND_MAX) * (GetMaxRandomPort() - GetMinRandomPort())));
			}while(m_iCurrentTCPRndPort==GetUDPPort() && ((GetMaxRandomPort() - GetMinRandomPort())>0));
		else
			m_iCurrentTCPRndPort = port;
	}
	return m_iCurrentTCPRndPort;
	// End -Random Ports-
}

// emulEspaña: Modified by MoNKi [MoNKi: -Random Ports-]
/*
uint16	CPreferences::GetUDPPort(){
*/
uint16	CPreferences::GetUDPPort(bool newPort, bool original, bool reset){
// End -Random Ports-

	// emulEspaña: Added by MoNKi [MoNKi: -Random Ports-]
	if(original)
		return udpport;

	if(reset){
		m_iCurrentUDPRndPort = 0;
	}
// End -Random Ports-

	if(udpport == 0)
		return 0;
	
	// emulEspaña: Added by MoNKi [MoNKi: -Random Ports-]
	if (m_iCurrentUDPRndPort == 0 || newPort){
		if(GetUseRandomPorts())
			do{
				m_iCurrentUDPRndPort = GetMinRandomPort() + (uint16)(((float)rand() / RAND_MAX) * (GetMaxRandomPort() - GetMinRandomPort()));
			}while(m_iCurrentUDPRndPort==GetPort() && ((GetMaxRandomPort() - GetMinRandomPort())>0));
		else
			m_iCurrentUDPRndPort = udpport;
	}
	return m_iCurrentUDPRndPort;
	// End -Random Ports-
}
//MORPH END   - Added by SiRoB, [MoNKi: -UPnPNAT Support-]

//MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
void CPreferences::SetWapPass(CString strNewPass)
{
	m_sWapPassword = MD5Sum(strNewPass).GetHash();
}

void CPreferences::SetWapLowPass(CString strNewPass)
{
	m_sWapLowPassword = MD5Sum(strNewPass).GetHash();
}
//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]

 // MORPH START leuk_he bindaddr
void CPreferences::SetBindAddr(CStringW bindip)
{
	m_strBindAddrW = bindip;
	m_strBindAddrW.Trim();
	m_pszBindAddrW = m_strBindAddrW.IsEmpty() ? NULL : (LPCWSTR)m_strBindAddrW;
	m_strBindAddrA = m_strBindAddrW;
	m_pszBindAddrA = m_strBindAddrA.IsEmpty() ? NULL : (LPCSTR)m_strBindAddrA;
}
 // MORPH END leuk_he bindaddr
 // MORPH START leuk_he upnp bindaddr
void CPreferences::SetUpnpBindAddr(DWORD bindip) {
		if (bindip== ntohl(inet_addr(GetBindAddrA())))
			m_dwUpnpBindAddr =0;
		else 
	    	m_dwUpnpBindAddr= bindip;
	}
	// MORPH END leuk_he upnp bindaddr
 



