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
#include "stdafx.h"
#include <io.h>
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
#include "MuleToolbarCtrl.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#include "StatisticsDlg.h"
#endif
#include "LastCommonRouteFinder.h" //MORPH - Added by SiRoB
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma pack(1)
struct Preferences_Import19c_Struct{
	uint8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	uint8	reconnect;
	uint8	deadserver;
	uint8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	uint8	ICH;
	uint8	autoserverlist;
	uint8	updatenotify;
	uint8	mintotray;
	uchar	userhash[16];
	uint8	autoconnect;
	uint8	addserversfromserver;
	uint8	addserversfromclient;
};
#pragma pack()

#pragma pack(1)
struct Preferences_Import20a_Struct{
	uint8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	uint8	reconnect;
	uint8	deadserver;
	uint16	deadserverretries;
	uint8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	uint8	ICH;
	uint8	autoserverlist;
	uint8	updatenotify;
	uint8	mintotray;
	uchar	userhash[16];
	uint8	autoconnect;
	uint8	addserversfromserver;
	uint8	addserversfromclient;
	uint16	maxsourceperfile;
	uint16	trafficOMeterInterval;
	uint32	totalDownloaded;
	uint32	totalUploaded;
	int		maxGraphDownloadRate;
	int		maxGraphUploadRate;
	uint8	beepOnError;
	uint8	confirmExit;
	WINDOWPLACEMENT EmuleWindowPlacement;
	int transferColumnWidths[9];
	int serverColumnWidths[8];
	uint8	splashscreen;
	uint8	filterLANIPs;
};
#pragma pack()

#pragma pack(1)
struct Preferences_Import20b_Struct{
	uint8	version;
	char	nick[50];
	uint16	maxupload;
	uint16	maxdownload;
	uint16	port;
	uint16	maxconnections;
	uint8	reconnect;
	uint8	deadserver;
	uint8	scorsystem;
	char	incomingdir[510];
	char	tempdir[510];
	uint8	ICH;
	uint8	autoserverlist;
	uint8	updatenotify;
	uint8	mintotray;
	uchar	userhash[16];
	uint8	autoconnect;
	uint8	addserversfromserver;
	uint8	addserversfromclient;
	uint16	maxsourceperfile;
	uint16	trafficOMeterInterval;
	uint32	totalDownloaded;	// outdated
	uint32	totalUploaded;		// outdated
	int		maxGraphDownloadRate;
	int		maxGraphUploadRate;
	uint8	beepOnError;
	uint8	confirmExit;
	WINDOWPLACEMENT EmuleWindowPlacement;
	int transferColumnWidths[9];
	int serverColumnWidths[8];
	uint8	splashscreen;
	uint8	filterLANIPs;
	uint64	totalDownloadedBytes;
	uint64	totalUploadedBytes;
};
#pragma pack()

CPreferences thePrefs;

int		CPreferences::m_iDbgHeap;
char	CPreferences::nick[50];
uint16	CPreferences::minupload;
uint16	CPreferences::maxupload;
uint16	CPreferences::maxdownload;
uint16	CPreferences::port;
uint16	CPreferences::udpport;
uint16	CPreferences::nServerUDPPort;
uint16	CPreferences::maxconnections;
uint8	CPreferences::reconnect;
uint8	CPreferences::deadserver;
uint8	CPreferences::scorsystem;
char	CPreferences::incomingdir[MAX_PATH];
char	CPreferences::tempdir[MAX_PATH];
uint8	CPreferences::ICH;
uint8	CPreferences::autoserverlist;
uint8	CPreferences::updatenotify;
uint8	CPreferences::mintotray;
uint8	CPreferences::autoconnect;
uint8	CPreferences::autoconnectstaticonly;
uint8	CPreferences::autotakeed2klinks;
uint8	CPreferences::addnewfilespaused;
uint8	CPreferences::depth3D;
int		CPreferences::m_iStraightWindowStyles;
TCHAR	CPreferences::m_szSkinProfile[MAX_PATH];
TCHAR	CPreferences::m_szSkinProfileDir[MAX_PATH];
uint8	CPreferences::addserversfromserver;
uint8	CPreferences::addserversfromclient;
uint16	CPreferences::maxsourceperfile;
uint16	CPreferences::trafficOMeterInterval;
uint16	CPreferences::statsInterval;
uchar	CPreferences::userhash[16];
WINDOWPLACEMENT CPreferences::EmuleWindowPlacement;
int		CPreferences::maxGraphDownloadRate;
int		CPreferences::maxGraphUploadRate;
uint8	CPreferences::beepOnError;
uint8	CPreferences::confirmExit;
uint16	CPreferences::downloadColumnWidths[13];
BOOL	CPreferences::downloadColumnHidden[13];
INT		CPreferences::downloadColumnOrder[13];
uint16	CPreferences::uploadColumnWidths[8];
BOOL	CPreferences::uploadColumnHidden[8];
INT		CPreferences::uploadColumnOrder[8];
uint16	CPreferences::queueColumnWidths[10];
BOOL	CPreferences::queueColumnHidden[10];
INT		CPreferences::queueColumnOrder[10];
uint16	CPreferences::searchColumnWidths[13];
BOOL	CPreferences::searchColumnHidden[13];
INT		CPreferences::searchColumnOrder[13];
uint16	CPreferences::sharedColumnWidths[12];
BOOL	CPreferences::sharedColumnHidden[12];
INT		CPreferences::sharedColumnOrder[12];
uint16	CPreferences::serverColumnWidths[13];
BOOL	CPreferences::serverColumnHidden[13];
INT		CPreferences::serverColumnOrder[13];
uint16	CPreferences::clientListColumnWidths[8];
BOOL	CPreferences::clientListColumnHidden[8];
INT		CPreferences::clientListColumnOrder[8];

uint16	CPreferences::FilenamesListColumnWidths[2];
BOOL	CPreferences::FilenamesListColumnHidden[2];
INT		CPreferences::FilenamesListColumnOrder[2];

DWORD	CPreferences::statcolors[15];
uint8	CPreferences::splashscreen;
uint8	CPreferences::filterLANIPs;
bool	CPreferences::m_bAllocLocalHostIP;
uint8	CPreferences::onlineSig;
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
uint64	CPreferences::cumUpData_CDONKEY;
uint64	CPreferences::cumUpData_XMULE;
uint64	CPreferences::cumUpData_SHAREAZA;
uint64	CPreferences::sesUpData_EDONKEY;
uint64	CPreferences::sesUpData_EDONKEYHYBRID;
uint64	CPreferences::sesUpData_EMULE;
uint64	CPreferences::sesUpData_MLDONKEY;
uint64	CPreferences::sesUpData_CDONKEY;
uint64	CPreferences::sesUpData_XMULE;
uint64	CPreferences::sesUpData_SHAREAZA;
uint64	CPreferences::cumUpDataPort_4662;
uint64	CPreferences::cumUpDataPort_OTHER;
uint64	CPreferences::sesUpDataPort_4662;
uint64	CPreferences::sesUpDataPort_OTHER;
uint64	CPreferences::cumUpData_File;
uint64	CPreferences::cumUpData_Partfile;
uint64	CPreferences::sesUpData_File;
uint64	CPreferences::sesUpData_Partfile;
uint32	CPreferences::cumDownCompletedFiles;
uint16	CPreferences::cumDownSuccessfulSessions;
uint16	CPreferences::cumDownFailedSessions;
uint32	CPreferences::cumDownAvgTime;
uint64	CPreferences::cumLostFromCorruption;
uint64	CPreferences::cumSavedFromCompression;
uint32	CPreferences::cumPartsSavedByICH;
uint16	CPreferences::sesDownSuccessfulSessions;
uint16	CPreferences::sesDownFailedSessions;
uint32	CPreferences::sesDownAvgTime;
uint16	CPreferences::sesDownCompletedFiles;
uint64	CPreferences::sesLostFromCorruption;
uint64	CPreferences::sesSavedFromCompression;
uint16	CPreferences::sesPartsSavedByICH;
uint64	CPreferences::cumDownData_EDONKEY;
uint64	CPreferences::cumDownData_EDONKEYHYBRID;
uint64	CPreferences::cumDownData_EMULE;
uint64	CPreferences::cumDownData_MLDONKEY;
uint64	CPreferences::cumDownData_CDONKEY;
uint64	CPreferences::cumDownData_XMULE;
uint64	CPreferences::cumDownData_SHAREAZA;
uint64	CPreferences::sesDownData_EDONKEY;
uint64	CPreferences::sesDownData_EDONKEYHYBRID;
uint64	CPreferences::sesDownData_EMULE;
uint64	CPreferences::sesDownData_MLDONKEY;
uint64	CPreferences::sesDownData_CDONKEY;
uint64	CPreferences::sesDownData_XMULE;
uint64	CPreferences::sesDownData_SHAREAZA;
uint64	CPreferences::cumDownDataPort_4662;
uint64	CPreferences::cumDownDataPort_OTHER;
uint64	CPreferences::sesDownDataPort_4662;
uint64	CPreferences::sesDownDataPort_OTHER;
float	CPreferences::cumConnAvgDownRate;
float	CPreferences::cumConnMaxAvgDownRate;
float	CPreferences::cumConnMaxDownRate;
float	CPreferences::cumConnAvgUpRate;
float	CPreferences::cumConnMaxAvgUpRate;
float	CPreferences::cumConnMaxUpRate;
uint64	CPreferences::cumConnRunTime;
uint16	CPreferences::cumConnNumReconnects;
uint16	CPreferences::cumConnAvgConnections;
uint16	CPreferences::cumConnMaxConnLimitReached;
uint16	CPreferences::cumConnPeakConnections;
uint32	CPreferences::cumConnTransferTime;
uint32	CPreferences::cumConnDownloadTime;
uint32	CPreferences::cumConnUploadTime;
uint32	CPreferences::cumConnServerDuration;
uint16	CPreferences::cumSrvrsMostWorkingServers;
uint32	CPreferences::cumSrvrsMostUsersOnline;
uint32	CPreferences::cumSrvrsMostFilesAvail;
uint16	CPreferences::cumSharedMostFilesShared;
uint64	CPreferences::cumSharedLargestShareSize;
uint64	CPreferences::cumSharedLargestAvgFileSize;
uint64	CPreferences::cumSharedLargestFileSize;
__int64 CPreferences::stat_datetimeLastReset;
uint8	CPreferences::statsConnectionsGraphRatio;
char	CPreferences::statsExpandedTreeItems[256];
uint64	CPreferences::totalDownloadedBytes;
uint64	CPreferences::totalUploadedBytes;
WORD	CPreferences::languageID;
uint8	CPreferences::transferDoubleclick;
EViewSharedFilesAccess CPreferences::m_iSeeShares;
uint8	CPreferences::m_iToolDelayTime;
uint8	CPreferences::bringtoforeground;
uint8	CPreferences::splitterbarPosition;
uint16	CPreferences::deadserverretries;
DWORD	CPreferences::m_dwServerKeepAliveTimeout;
uint16	CPreferences::statsMax;
uint8	CPreferences::statsAverageMinutes;
uint8	CPreferences::useDownloadNotifier;
uint8	CPreferences::useNewDownloadNotifier;
uint8	CPreferences::useChatNotifier;
uint8	CPreferences::useLogNotifier;
uint8	CPreferences::useSoundInNotifier;
uint8	CPreferences::notifierPopsEveryChatMsg;
uint8	CPreferences::notifierImportantError;
uint8	CPreferences::notifierNewVersion;
char	CPreferences::notifierSoundFilePath[510];
char	CPreferences::m_sircserver[50];
char	CPreferences::m_sircnick[30];
char	CPreferences::m_sircchannamefilter[50];
bool	CPreferences::m_bircaddtimestamp;
bool	CPreferences::m_bircusechanfilter;
uint16	CPreferences::m_iircchanneluserfilter;
char	CPreferences::m_sircperformstring[255];
bool	CPreferences::m_bircuseperform;
bool	CPreferences::m_birclistonconnect;
bool	CPreferences::m_bircacceptlinks;
bool	CPreferences::m_bircacceptlinksfriends;
bool	CPreferences::m_bircignoreinfomessage;
bool	CPreferences::m_bircignoreemuleprotoinfomessage;
bool	CPreferences::m_birchelpchannel;
bool	CPreferences::m_bRemove2bin;
bool	CPreferences::m_bpreviewprio;
bool	CPreferences::smartidcheck;
uint8	CPreferences::smartidstate;
bool	CPreferences::safeServerConnect;
bool	CPreferences::startMinimized;
bool	CPreferences::m_bRestoreLastMainWndDlg;
int		CPreferences::m_iLastMainWndDlgID;
bool	CPreferences::m_bRestoreLastLogPane;
int		CPreferences::m_iLastLogPaneID;
uint16	CPreferences::MaxConperFive;
int		CPreferences::checkDiskspace;
UINT	CPreferences::m_uMinFreeDiskSpace;
char	CPreferences::yourHostname[127];
bool	CPreferences::m_bVerbose;
bool	CPreferences::m_bDebugSourceExchange;
bool	CPreferences::m_bLogBannedClients;
bool	CPreferences::m_bLogRatingDescReceived;
bool	CPreferences::m_bLogSecureIdent;
bool	CPreferences::m_bLogFilteredIPs;
bool	CPreferences::m_bLogFileSaving;
int		CPreferences::m_iDebugServerTCPLevel;
int		CPreferences::m_iDebugServerUDPLevel;
int		CPreferences::m_iDebugServerSourcesLevel;
int		CPreferences::m_iDebugServerSearchesLevel;
int		CPreferences::m_iDebugClientTCPLevel;
int		CPreferences::m_iDebugClientUDPLevel;
int		CPreferences::m_iDebugClientKadUDPLevel;
bool	CPreferences::m_bupdatequeuelist;
bool	CPreferences::m_bmanualhighprio;
bool	CPreferences::m_btransferfullchunks;
bool	CPreferences::m_bstartnextfile;
bool	CPreferences::m_bshowoverhead;
bool	CPreferences::m_bDAP;
bool	CPreferences::m_bUAP;
bool	CPreferences::m_bDisableKnownClientList;
bool	CPreferences::m_bDisableQueueList;
bool	CPreferences::m_bExtControls;
bool	CPreferences::m_bTransflstRemain;
uint8	CPreferences::versioncheckdays;
int		CPreferences::tableSortItemDownload;
int		CPreferences::tableSortItemUpload;
int		CPreferences::tableSortItemQueue;
int		CPreferences::tableSortItemSearch;
int		CPreferences::tableSortItemShared;
int		CPreferences::tableSortItemServer;
int		CPreferences::tableSortItemClientList;
int		CPreferences::tableSortItemFilenames;
bool	CPreferences::tableSortAscendingDownload;
bool	CPreferences::tableSortAscendingUpload;
bool	CPreferences::tableSortAscendingQueue;
bool	CPreferences::tableSortAscendingSearch;
bool	CPreferences::tableSortAscendingShared;
bool	CPreferences::tableSortAscendingServer;
bool	CPreferences::tableSortAscendingClientList;
bool	CPreferences::tableSortAscendingFilenames;
bool	CPreferences::showRatesInTitle;
char	CPreferences::TxtEditor[256];
char	CPreferences::VideoPlayer[256];
bool	CPreferences::moviePreviewBackup;
int		CPreferences::m_iPreviewSmallBlocks;
bool	CPreferences::indicateratings;
bool	CPreferences::watchclipboard;
bool	CPreferences::filterserverbyip;
bool	CPreferences::m_bFirstStart;
bool	CPreferences::m_bCreditSystem;
bool	CPreferences::log2disk;
bool	CPreferences::debug2disk;
int		CPreferences::iMaxLogBuff;
UINT	CPreferences::uMaxLogFileSize;
bool	CPreferences::scheduler;
bool	CPreferences::dontcompressavi;
bool	CPreferences::msgonlyfriends;
bool	CPreferences::msgsecure;
uint8	CPreferences::filterlevel;
UINT	CPreferences::m_iFileBufferSize;
UINT	CPreferences::m_iQueueSize;
int		CPreferences::m_iCommitFiles;
uint16	CPreferences::maxmsgsessions;
uint32	CPreferences::versioncheckLastAutomatic;
char	CPreferences::messageFilter[512];
char	CPreferences::commentFilter[512];
char	CPreferences::filenameCleanups[512];
char	CPreferences::notifierConfiguration[510];
char	CPreferences::datetimeformat[64];
char	CPreferences::datetimeformat4log[64];
LOGFONT CPreferences::m_lfHyperText;
int		CPreferences::m_iExtractMetaData;
char	CPreferences::m_sWebPassword[256];
char	CPreferences::m_sWebLowPassword[256];
uint16	CPreferences::m_nWebPort;
bool	CPreferences::m_bWebEnabled;
bool	CPreferences::m_bWebUseGzip;
int		CPreferences::m_nWebPageRefresh;
bool	CPreferences::m_bWebLowEnabled;
char	CPreferences::m_sWebResDir[MAX_PATH];
char	CPreferences::m_sTemplateFile[MAX_PATH];
ProxySettings CPreferences::proxy;
bool	CPreferences::m_bIsASCWOP;
bool	CPreferences::m_bShowProxyErrors;
bool	CPreferences::showCatTabInfos;
bool	CPreferences::resumeSameCat;
bool	CPreferences::dontRecreateGraphs;
bool	CPreferences::autofilenamecleanup;
int		CPreferences::allcatType;
bool	CPreferences::m_bUseAutocompl;
bool	CPreferences::m_bShowDwlPercentage;
bool	CPreferences::m_bRemoveFinishedDownloads;
uint16	CPreferences::m_iMaxChatHistory;
int		CPreferences::m_iSearchMethod;
bool	CPreferences::m_bAdvancedSpamfilter;
bool	CPreferences::m_bUseSecureIdent;
char	CPreferences::m_sMMPassword[256];
bool	CPreferences::m_bMMEnabled;
uint16	CPreferences::m_nMMPort;
bool	CPreferences::networkkademlia;
bool	CPreferences::networked2k;
uint8	CPreferences::m_nToolbarLabels;
char	CPreferences::m_sToolbarBitmap[256];
char	CPreferences::m_sToolbarBitmapFolder[256];
char	CPreferences::m_sToolbarSettings[256];
bool	CPreferences::m_bPreviewEnabled;
bool	CPreferences::m_bDynUpEnabled;
int		CPreferences::m_iDynUpPingTolerance;
int		CPreferences::m_iDynUpGoingUpDivider;
int		CPreferences::m_iDynUpGoingDownDivider;
int		CPreferences::m_iDynUpNumberOfPings;
CStringList CPreferences::shareddir_list;
CStringList CPreferences::adresses_list;
CString CPreferences::appdir;
CString CPreferences::configdir;
CString CPreferences::m_strWebServerDir;
CString CPreferences::m_strLangDir;
Preferences_Ext_Struct* CPreferences::prefsExt;
WORD	CPreferences::m_wWinVer;
bool	CPreferences::m_UseProxyListenPort;
uint16	CPreferences::ListenPort;
CArray<Category_Struct*,Category_Struct*> CPreferences::catMap;


CPreferences::CPreferences()
{
#ifdef _DEBUG
	m_iDbgHeap = 1;
#endif
}

CPreferences::~CPreferences()
{
	delete prefsExt;
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
	char buffer[490];
	::GetModuleFileName(0, buffer, 490);
	LPTSTR pszFileName = _tcsrchr(buffer, '\\') + 1;
	*pszFileName = '\0';

	appdir = buffer;
	configdir = appdir + CONFIGFOLDER;
	m_strWebServerDir = appdir + _T("webserver\\");
	m_strLangDir = appdir + _T("lang\\");

	::CreateDirectory(GetConfigDir(),0);

	// lets move config-files in the appdir to the configdir (for upgraders <0.29a to >=0.29a )
	if ( PathFileExists(appdir+"preferences.ini")) MoveFile(appdir+"preferences.ini",configdir+"preferences.ini");
	if ( PathFileExists(appdir+"preferences.dat")) MoveFile(appdir+"preferences.dat",configdir+"preferences.dat");
	if ( PathFileExists(appdir+"adresses.dat")) MoveFile(appdir+"adresses.dat",configdir+"adresses.dat");
	if ( PathFileExists(appdir+"Category.ini")) MoveFile(appdir+"Category.ini",configdir+"Category.ini");
	if ( PathFileExists(appdir+"clients.met")) MoveFile(appdir+"clients.met",configdir+"clients.met");
	if ( PathFileExists(appdir+"emfriends.met")) MoveFile(appdir+"emfriends.met",configdir+"emfriends.met");
	if ( PathFileExists(appdir+"fileinfo.ini")) MoveFile(appdir+"fileinfo.ini",configdir+"fileinfo.ini");
	if ( PathFileExists(appdir+"ipfilter.dat")) MoveFile(appdir+"ipfilter.dat",configdir+"ipfilter.dat");
	if ( PathFileExists(appdir+"known.met")) MoveFile(appdir+"known.met",configdir+"known.met");
	if ( PathFileExists(appdir+"server.met")) MoveFile(appdir+"server.met",configdir+"server.met");
	if ( PathFileExists(appdir+"shareddir.dat")) MoveFile(appdir+"shareddir.dat",configdir+"shareddir.dat");
	if ( PathFileExists(appdir+"staticservers.dat")) MoveFile(appdir+"staticservers.dat",configdir+"staticservers.dat");
	if ( PathFileExists(appdir+"webservices.dat")) MoveFile(appdir+"webservices.dat",configdir+"webservices.dat");

	CreateUserHash();

	// load preferences.dat or set standart values
	char* fullpath = new char[strlen(configdir)+16];
	sprintf(fullpath,"%spreferences.dat",configdir);
	FILE* preffile = fopen(fullpath,"rb");
	delete[] fullpath;

	LoadPreferences();

	if (!preffile){
		SetStandartValues();
		//if (Ask4RegFix(true)) Ask4RegFix(false);
	}
	else{
		fread(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		if (ferror(preffile))
			SetStandartValues();

		// import old pref-files
		if (prefsExt->version<20) {


			if (prefsExt->version>17) {// v0.20b+
				Preferences_Import20b_Struct* prefsImport20b;
				prefsImport20b=new Preferences_Import20b_Struct;
				memset(prefsImport20b,0,sizeof(Preferences_Import20b_Struct));
				fseek(preffile,0,0);
				fread(prefsImport20b,sizeof(Preferences_Import20b_Struct),1,preffile);

				md4cpy(userhash, prefsImport20b->userhash);
				memcpy(incomingdir, prefsImport20b->incomingdir, 510);
				memcpy(tempdir, prefsImport20b->tempdir, 510);
				_snprintf(nick, ARRSIZE(nick), "%s", prefsImport20b->nick);

				totalDownloadedBytes=prefsImport20b->totalDownloadedBytes;
				totalUploadedBytes=prefsImport20b->totalUploadedBytes;
				delete prefsImport20b;
			} else if (prefsExt->version>7) { // v0.20a
				Preferences_Import20a_Struct* prefsImport20a;
				prefsImport20a=new Preferences_Import20a_Struct;
				memset(prefsImport20a,0,sizeof(Preferences_Import20a_Struct));
				fseek(preffile,0,0);
				fread(prefsImport20a,sizeof(Preferences_Import20a_Struct),1,preffile);

				md4cpy(userhash, prefsImport20a->userhash);
				memcpy(incomingdir, prefsImport20a->incomingdir, 510);
				memcpy(tempdir, prefsImport20a->tempdir, 510);
				_snprintf(nick, ARRSIZE(nick), "%s", prefsImport20a->nick);

				totalDownloadedBytes=prefsImport20a->totalDownloaded;
				totalUploadedBytes=prefsImport20a->totalUploaded;
				delete prefsImport20a;
			} else {	//v0.19c-
				Preferences_Import19c_Struct* prefsImport19c;
				prefsImport19c=new Preferences_Import19c_Struct;
				memset(prefsImport19c,0,sizeof(Preferences_Import19c_Struct));

				fseek(preffile,0,0);
				fread(prefsImport19c,sizeof(Preferences_Import19c_Struct),1,preffile);

				if (prefsExt->version < 3)
					CreateUserHash();
				else
					md4cpy(userhash, prefsImport19c->userhash);
				memcpy(incomingdir, prefsImport19c->incomingdir, 510);
				memcpy(tempdir, prefsImport19c->tempdir, 510);
				_snprintf(nick, ARRSIZE(nick), "%s", prefsImport19c->nick);
				delete prefsImport19c;
			}
 		} else {
			md4cpy(userhash, prefsExt->userhash);
			EmuleWindowPlacement = prefsExt->EmuleWindowPlacement;
		}
		fclose(preffile);
		smartidstate = 0;
	}

	// shared directories
	fullpath = new char[strlen(configdir)+MAX_PATH]; // i_a
	sprintf(fullpath,"%sshareddir.dat",configdir);
	CStdioFile* sdirfile = new CStdioFile();
	if (sdirfile->Open(fullpath,CFile::modeRead)){
		CString toadd;
		while (sdirfile->ReadString(toadd))
		{
			TCHAR szFullPath[MAX_PATH];
			if (PathCanonicalize(szFullPath, toadd))
				toadd = szFullPath;

			if (IsInstallationDirectory(toadd))
				continue;

			if (_taccess(toadd, 0) == 0){ // only add directories which still exist
				if (toadd.Right(1) != _T('\\'))
					toadd.Append(_T("\\"));
				shareddir_list.AddHead(toadd);
			}
		}
		sdirfile->Close();
	}
	delete sdirfile;
	delete[] fullpath;
	
	//serverlist adresses
	fullpath = new char[strlen(configdir)+20];
	sprintf(fullpath,"%sadresses.dat",configdir);
	sdirfile = new CStdioFile();
	if (sdirfile->Open(fullpath,CFile::modeRead)){
		CString toadd;
		while (sdirfile->ReadString(toadd))
			adresses_list.AddHead(toadd);
		sdirfile->Close();
	}
	delete sdirfile;
	delete[] fullpath;	
	fullpath=NULL;

	userhash[5] = 14;
	userhash[14] = 111;

	// deadlake PROXYSUPPORT
	m_UseProxyListenPort = false;
	ListenPort = 0;

	// Explicitly inform the user about errors with incoming/temp folders!
	if (!PathFileExists(GetIncomingDir()) && !::CreateDirectory(GetIncomingDir(),0)) {
		CString strError;
		strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetIncomingDir(), GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
		sprintf(incomingdir,"%sincoming",appdir);
		if (!PathFileExists(GetIncomingDir()) && !::CreateDirectory(GetIncomingDir(),0)){
			strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_INCOMING), GetIncomingDir(), GetErrorMessage(GetLastError()));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}
	if (!PathFileExists(GetTempDir()) && !::CreateDirectory(GetTempDir(),0)) {
		CString strError;
		strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
		sprintf(tempdir,"%stemp",appdir);
		if (!PathFileExists(GetTempDir()) && !::CreateDirectory(GetTempDir(),0)){
			strError.Format(GetResString(IDS_ERR_CREATE_DIR), GetResString(IDS_PW_TEMP), GetTempDir(), GetErrorMessage(GetLastError()));
			AfxMessageBox(strError, MB_ICONERROR);
		}
	}
	// khaos::kmod+ Source Lists directory
	CString sSourceListsPath = CString(GetTempDir()) + "\\Source Lists";
	if (UseSaveLoadSources() && !PathFileExists(sSourceListsPath.GetBuffer()) && !::CreateDirectory(sSourceListsPath.GetBuffer(), 0)) {
		CString strError;
		strError.Format(_T("Failed to create source lists directory \"%s\" - %s"), sSourceListsPath, GetErrorMessage(GetLastError()));
		AfxMessageBox(strError, MB_ICONERROR);
	}
	// khaos::kmod-

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

//	Save();
}

bool CPreferences::IsTempFile(const CString& rstrDirectory, const CString& rstrName)
{
	if (CompareDirectories(rstrDirectory, GetTempDir()))
		return false;

	// do not share a file from the temp directory, if it matches one of the following patterns
	CString strNameLower(rstrName);
	strNameLower.MakeLower();
	strNameLower += _T("|"); // append an EOS character which we can query for
	static const LPCTSTR _apszNotSharedExts[] = {
		_T("%u.part") _T("%c"), 
		_T("%u.part.met") _T("%c"), 
		_T("%u.part.met") PARTMET_BAK_EXT _T("%c"), 
		_T("%u.part.met") PARTMET_TMP_EXT _T("%c") 
	};
	for (int i = 0; i < ARRSIZE(_apszNotSharedExts); i++){
		UINT uNum;
		TCHAR iChar;
		// "misuse" the 'scanf' function for a very simple pattern scanning.
		if (_stscanf(strNameLower, _apszNotSharedExts[i], &uNum, &iChar) == 2 && iChar == _T('|'))
			return true;
	}

	return false;
}

// SLUGFILLER: SafeHash
bool CPreferences::IsConfigFile(const CString& rstrDirectory, const CString& rstrName)
{
	if (CompareDirectories(rstrDirectory, configdir))
		return false;

	// do not share a file from the config directory, if it contains one of the following extensions
	static const LPCTSTR _apszNotSharedExts[] = { _T(".met.bak"), _T(".ini.old") };
	for (int i = 0; i < ARRSIZE(_apszNotSharedExts); i++){
		int iLen = _tcslen(_apszNotSharedExts[i]);
		if (rstrName.GetLength()>=iLen && rstrName.Right(iLen).CompareNoCase(_apszNotSharedExts[i])==0)
			return true;
	}

	// do not share following files from the config directory
	static const LPCTSTR _apszNotSharedFiles[] = 
	{
		_T("AC_SearchStrings.dat"),
		_T("AC_ServerMetURLs.dat"),
		_T("adresses.dat"),
		_T("category.ini"),
		_T("clients.met"),
		_T("cryptkey.dat"),
		_T("emfriends.met"),
		_T("fileinfo.ini"),
		_T("ipfilter.dat"),
		_T("known.met"),
		_T("preferences.dat"),
		_T("preferences.ini"),
		_T("server.met"),
		_T("server.met.new"),
		_T("server_met.download"),
		_T("server_met.old"),
		_T("shareddir.dat"),
		_T("sharedsubdir.dat"),
		_T("staticservers.dat"),
		_T("webservices.dat")
	};
	for (int i = 0; i < ARRSIZE(_apszNotSharedFiles); i++){
		if (rstrName.CompareNoCase(_apszNotSharedFiles[i])==0)
			return true;
	}

	return false;
}
// SLUGFILLER: SafeHash

//MORPH - Added by SiRoB, ZZ Ratio
bool CPreferences::IsZZRatioDoesWork(){
	
	/*
	if (theApp.stat_sessionSentBytesToFriend)
		return true;
	if (theApp.downloadqueue->IsFilesPowershared())
		return true;
	*/
	if (thePrefs.IsSUCEnabled())
		return theApp.uploadqueue->GetMaxVUR()<10240;
	else if (thePrefs.IsDynUpEnabled())
		return theApp.lastCommonRouteFinder->GetUpload()<10240;
	else
		return thePrefs.GetMaxUpload()<10;
}
//MORPH - Added by SiRoB, ZZ ratio

uint16 CPreferences::GetMaxDownload(){
	//dont be a Lam3r :)
	//MORPH START - Added by SiRoB, ZZ Upload system
	if (IsZZRatioDoesWork())
		return prefs->maxdownload;
	//MORPH END   - Added by SiRoB, ZZ Upload system
	uint16 maxup=(GetMaxUpload()==UNLIMITED)?GetMaxGraphUploadRate():GetMaxUpload();
	if( maxup < 4 )
		return (( (maxup < 10) && (maxup*3 < maxdownload) )? maxup*3 : maxdownload);
	return (( (maxup < 10) && (maxup*4 < maxdownload) )? maxup*4 : maxdownload);
}

// -khaos--+++> A whole bunch of methods!  Keep going until you reach the end tag.
void CPreferences::SaveStats(int bBackUp){
	// This function saves all of the new statistics in my addon.  It is also used to
	// save backups for the Reset Stats function, and the Restore Stats function (Which is actually LoadStats)
	// bBackUp = 0: DEFAULT; save to preferences.ini
	// bBackUp = 1: Save to statbkup.ini, which is used to restore after a reset
	// bBackUp = 2: Save to statbkuptmp.ini, which is temporarily created during a restore and then renamed to statbkup.ini

	CString buffer;
	char* fullpath = new char[strlen(configdir)+MAX_PATH]; // i_a
	if (bBackUp == 1) sprintf(fullpath,"%sstatbkup.ini",configdir);
	else if (bBackUp == 2) sprintf(fullpath,"%sstatbkuptmp.ini",configdir);
	else sprintf(fullpath,"%spreferences.ini",configdir);
	
	CIni ini(fullpath, "Statistics");

	delete[] fullpath;

	// Save cumulative statistics to preferences.ini, going in order as they appear in CStatisticsDlg::ShowStatistics.
	// We do NOT SET the values in prefs struct here.

    // Save Cum Down Data
	buffer.Format("%I64u",theApp.stat_sessionReceivedBytes+GetTotalDownloaded());
	ini.WriteString("TotalDownloadedBytes", buffer );
    // Save Complete Downloads - This is saved and incremented in partfile.cpp.
	// Save Successful Download Sessions
	ini.WriteInt("DownSuccessfulSessions", cumDownSuccessfulSessions);
	// Save Failed Download Sessions
	ini.WriteInt("DownFailedSessions", cumDownFailedSessions);
	ini.WriteInt("DownAvgTime", (GetDownC_AvgTime() + GetDownS_AvgTime()) / 2 );

	// Cumulative statistics for saved due to compression/lost due to corruption
	buffer.Format("%I64u", cumLostFromCorruption + sesLostFromCorruption);
	ini.WriteString("LostFromCorruption", buffer );

	buffer.Format("%I64u", sesSavedFromCompression + cumSavedFromCompression);
	ini.WriteString("SavedFromCompression", buffer );

	ini.WriteInt("PartsSavedByICH", cumPartsSavedByICH + sesPartsSavedByICH);

	// Save cumulative client breakdown stats for received bytes...
	buffer.Format("%I64u", GetCumDownData_EDONKEY() );
	ini.WriteString("DownData_EDONKEY", buffer );
	buffer.Format("%I64u", GetCumDownData_EDONKEYHYBRID() );
	ini.WriteString("DownData_EDONKEYHYBRID", buffer );
	buffer.Format("%I64u", GetCumDownData_EMULE() );
	ini.WriteString("DownData_EMULE", buffer );
	buffer.Format("%I64u", GetCumDownData_MLDONKEY() );
	ini.WriteString("DownData_MLDONKEY", buffer );
	buffer.Format("%I64u", GetCumDownData_XMULE() );
	ini.WriteString("DownData_LMULE", buffer );
	buffer.Format("%I64u", GetCumDownData_CDONKEY() );
	ini.WriteString("DownData_CDONKEY", buffer );
	buffer.Format("%I64u", GetCumDownData_SHAREAZA() );
	ini.WriteString("DownData_SHAREAZA", buffer );

	// Save cumulative port breakdown stats for received bytes
	buffer.Format("%I64u", GetCumDownDataPort_4662() );
	ini.WriteString("DownDataPort_4662", buffer );
	buffer.Format("%I64u", GetCumDownDataPort_OTHER() );
	ini.WriteString("DownDataPort_OTHER", buffer );

	// Save Cumulative Downline Overhead Statistics
	buffer.Format("%I64u",	theApp.downloadqueue->GetDownDataOverheadFileRequest() +
							theApp.downloadqueue->GetDownDataOverheadSourceExchange() +
							theApp.downloadqueue->GetDownDataOverheadServer() +
							theApp.downloadqueue->GetDownDataOverheadKad() +
							theApp.downloadqueue->GetDownDataOverheadOther() +
							GetDownOverheadTotal());
	ini.WriteString("DownOverheadTotal", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadFileRequest() + GetDownOverheadFileReq());
	ini.WriteString("DownOverheadFileReq", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadSourceExchange() + GetDownOverheadSrcEx());
	ini.WriteString("DownOverheadSrcEx", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadServer()+GetDownOverheadServer());
	ini.WriteString("DownOverheadServer", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadKad()+GetDownOverheadKad());
	ini.WriteString("DownOverheadKad", buffer );
	
	buffer.Format("%I64u",	theApp.downloadqueue->GetDownDataOverheadFileRequestPackets() + 
							theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets() + 
							theApp.downloadqueue->GetDownDataOverheadServerPackets() + 
							theApp.downloadqueue->GetDownDataOverheadKadPackets() + 
							theApp.downloadqueue->GetDownDataOverheadOtherPackets() + 
							GetDownOverheadTotalPackets());
	ini.WriteString("DownOverheadTotalPackets", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadFileRequestPackets() + GetDownOverheadFileReqPackets());
	ini.WriteString("DownOverheadFileReqPackets", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadSourceExchangePackets() + GetDownOverheadSrcExPackets());
	ini.WriteString("DownOverheadSrcExPackets", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadServerPackets() + GetDownOverheadServerPackets());
	ini.WriteString("DownOverheadServerPackets", buffer );
	buffer.Format("%I64u",theApp.downloadqueue->GetDownDataOverheadKadPackets() + GetDownOverheadKadPackets());
	ini.WriteString("DownOverheadKadPackets", buffer );

	// Save Cumulative Upline Statistics
	buffer.Format("%I64u",theApp.stat_sessionSentBytes+GetTotalUploaded());
	ini.WriteString("TotalUploadedBytes", buffer );
	ini.WriteInt("UpSuccessfulSessions", theApp.uploadqueue->GetSuccessfullUpCount()+GetUpSuccessfulSessions());
	ini.WriteInt("UpFailedSessions", theApp.uploadqueue->GetFailedUpCount()+GetUpFailedSessions());
	ini.WriteInt("UpAvgTime", (theApp.uploadqueue->GetAverageUpTime()+GetUpAvgTime())/2);

	// Save Cumulative Client Breakdown Stats For Sent Bytes
	buffer.Format("%I64u", GetCumUpData_EDONKEY() );
	ini.WriteString("UpData_EDONKEY", buffer );
	buffer.Format("%I64u", GetCumUpData_EDONKEYHYBRID() );
	ini.WriteString("UpData_EDONKEYHYBRID", buffer );
	buffer.Format("%I64u", GetCumUpData_EMULE() );
	ini.WriteString("UpData_EMULE", buffer );
	buffer.Format("%I64u", GetCumUpData_MLDONKEY() );
	ini.WriteString("UpData_MLDONKEY", buffer );
	buffer.Format("%I64u", GetCumUpData_XMULE() );
	ini.WriteString("UpData_LMULE", buffer );
	buffer.Format("%I64u", GetCumUpData_CDONKEY() );
	ini.WriteString("UpData_CDONKEY", buffer );
	buffer.Format("%I64u", GetCumUpData_SHAREAZA() );
	ini.WriteString("UpData_SHAREAZA", buffer );

	// Save cumulative port breakdown stats for sent bytes
	buffer.Format("%I64u", GetCumUpDataPort_4662() );
	ini.WriteString("UpDataPort_4662", buffer );
	buffer.Format("%I64u", GetCumUpDataPort_OTHER() );
	ini.WriteString("UpDataPort_OTHER", buffer );

	// Save cumulative source breakdown stats for sent bytes
	buffer.Format("%I64u", GetCumUpData_File() );
	ini.WriteString("UpData_File", buffer );
	buffer.Format("%I64u", GetCumUpData_Partfile() );
	ini.WriteString("UpData_Partfile", buffer );

	// Save Cumulative Upline Overhead Statistics
	buffer.Format("%I64u",	theApp.uploadqueue->GetUpDataOverheadFileRequest() + 
							theApp.uploadqueue->GetUpDataOverheadSourceExchange() + 
							theApp.uploadqueue->GetUpDataOverheadServer() + 
							theApp.uploadqueue->GetUpDataOverheadKad() + 
							theApp.uploadqueue->GetUpDataOverheadOther() + 
							GetUpOverheadTotal());
	ini.WriteString("UpOverheadTotal", buffer);
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadFileRequest() + GetUpOverheadFileReq());
	ini.WriteString("UpOverheadFileReq", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadSourceExchange() + GetUpOverheadSrcEx());
	ini.WriteString("UpOverheadSrcEx", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadServer() + GetUpOverheadServer());
	ini.WriteString("UpOverheadServer", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadKad() + GetUpOverheadKad());
	ini.WriteString("UpOverheadKad", buffer );

	buffer.Format("%I64u",	theApp.uploadqueue->GetUpDataOverheadFileRequestPackets() + 
							theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets() + 
							theApp.uploadqueue->GetUpDataOverheadServerPackets() + 
							theApp.uploadqueue->GetUpDataOverheadKadPackets() + 
							theApp.uploadqueue->GetUpDataOverheadOtherPackets() + 
							GetUpOverheadTotalPackets());
	ini.WriteString("UpOverheadTotalPackets", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadFileRequestPackets() + GetUpOverheadFileReqPackets());
	ini.WriteString("UpOverheadFileReqPackets", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadSourceExchangePackets() + GetUpOverheadSrcExPackets());
	ini.WriteString("UpOverheadSrcExPackets", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadServerPackets() + GetUpOverheadServerPackets());
	ini.WriteString("UpOverheadServerPackets", buffer );
	buffer.Format("%I64u",theApp.uploadqueue->GetUpDataOverheadKadPackets() + GetUpOverheadKadPackets());
	ini.WriteString("UpOverheadKadPackets", buffer );

	// Save Cumulative Connection Statistics
	float tempRate = 0;
	// Download Rate Average
	tempRate = theApp.statistics->GetAvgDownloadRate(2);
	ini.WriteFloat("ConnAvgDownRate", tempRate);
	// Max Download Rate Average
	if (tempRate>GetConnMaxAvgDownRate()) Add2ConnMaxAvgDownRate(tempRate);
	ini.WriteFloat("ConnMaxAvgDownRate", GetConnMaxAvgDownRate());
	// Max Download Rate
	tempRate = (float) theApp.downloadqueue->GetDatarate() / 1024;
	if (tempRate>GetConnMaxDownRate()) Add2ConnMaxDownRate(tempRate);
	ini.WriteFloat("ConnMaxDownRate", GetConnMaxDownRate());
	// Upload Rate Average
	tempRate = theApp.statistics->GetAvgUploadRate(2);
	ini.WriteFloat("ConnAvgUpRate", tempRate);
	// Max Upload Rate Average
	if (tempRate>GetConnMaxAvgUpRate()) Add2ConnMaxAvgUpRate(tempRate);
	ini.WriteFloat("ConnMaxAvgUpRate", GetConnMaxAvgUpRate());
	// Max Upload Rate
	tempRate = (float) theApp.uploadqueue->GetDatarate() / 1024;
	if (tempRate>GetConnMaxUpRate()) Add2ConnMaxUpRate(tempRate);
	ini.WriteFloat("ConnMaxUpRate", GetConnMaxUpRate());
	// Overall Run Time
	uint32 timeseconds = (GetTickCount()-theApp.stat_starttime)/1000;
	timeseconds+=GetConnRunTime();
	ini.WriteInt("ConnRunTime",timeseconds );
	// Number of Reconnects
	if (theApp.stat_reconnects>0) buffer.Format("%u",theApp.stat_reconnects - 1 + GetConnNumReconnects());
	else buffer.Format("%u",GetConnNumReconnects());
	ini.WriteString("ConnNumReconnects", buffer);
	// Average Connections
	if (theApp.serverconnect->IsConnected()){
		buffer.Format("%u",(uint32)(theApp.listensocket->GetAverageConnections()+cumConnAvgConnections)/2);
		ini.WriteString("ConnAvgConnections", buffer);
	}
	// Peak Connections
	if (theApp.listensocket->GetPeakConnections()>cumConnPeakConnections)
		cumConnPeakConnections = theApp.listensocket->GetPeakConnections();
	ini.WriteInt("ConnPeakConnections", cumConnPeakConnections);
	// Max Connection Limit Reached
	buffer.Format("%u",theApp.listensocket->GetMaxConnectionReached()+cumConnMaxConnLimitReached);
	if (atoi(buffer)>cumConnMaxConnLimitReached) ini.WriteString("ConnMaxConnLimitReached", buffer);
	// Time Stuff...
	ini.WriteInt("ConnTransferTime", GetConnTransferTime() + theApp.statistics->GetTransferTime());
	ini.WriteInt("ConnUploadTime", GetConnUploadTime() + theApp.statistics->GetUploadTime());
	ini.WriteInt("ConnDownloadTime", GetConnDownloadTime() + theApp.statistics->GetDownloadTime());
	ini.WriteInt("ConnServerDuration", GetConnServerDuration() + theApp.statistics->GetServerDuration());
	
	// Compare and Save Server Records
	uint32 servtotal, servfail, servuser, servfile, servtuser, servtfile; float servocc;
	theApp.serverlist->GetStatus( servtotal, servfail, servuser, servfile, servtuser, servtfile, servocc );
	
	if ((servtotal-servfail)>cumSrvrsMostWorkingServers)	cumSrvrsMostWorkingServers = servtotal-servfail;
	ini.WriteInt("SrvrsMostWorkingServers", cumSrvrsMostWorkingServers);
	if (servtuser>cumSrvrsMostUsersOnline) cumSrvrsMostUsersOnline = servtuser;
	ini.WriteInt("SrvrsMostUsersOnline", cumSrvrsMostUsersOnline);
	if (servtfile>cumSrvrsMostFilesAvail) cumSrvrsMostFilesAvail = servtfile;
	ini.WriteInt("SrvrsMostFilesAvail", cumSrvrsMostFilesAvail);

	// Compare and Save Shared File Records
	if (theApp.sharedfiles->GetCount()>cumSharedMostFilesShared)	cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
	ini.WriteInt("SharedMostFilesShared", cumSharedMostFilesShared);
	uint64 bytesLargestFile = 0;
	uint64 allsize=theApp.sharedfiles->GetDatasize(bytesLargestFile);
	if (allsize>cumSharedLargestShareSize) cumSharedLargestShareSize = allsize;
	buffer.Format("%I64u", cumSharedLargestShareSize);
	ini.WriteString("SharedLargestShareSize", buffer);
	if (bytesLargestFile>cumSharedLargestFileSize) cumSharedLargestFileSize = bytesLargestFile;
	buffer.Format("%I64u", cumSharedLargestFileSize);
	ini.WriteString("SharedLargestFileSize", buffer);
	if (theApp.sharedfiles->GetCount() != 0) {
		uint64 tempint = allsize/theApp.sharedfiles->GetCount();
		if (tempint>cumSharedLargestAvgFileSize)	cumSharedLargestAvgFileSize = tempint;
	}
	buffer.Format("%I64u",cumSharedLargestAvgFileSize);
	ini.WriteString("SharedLargestAvgFileSize", buffer);

	buffer.Format("%I64u",stat_datetimeLastReset);
	ini.WriteString("statsDateTimeLastReset", buffer);

	// If we are saving a back-up or a temporary back-up, return now.
	if (bBackUp != 0) return;

	// These aren't really statistics, but they're a part of my add-on, so we'll save them here and load them in LoadStats
	ini.WriteInt("statsConnectionsGraphRatio", statsConnectionsGraphRatio, "Statistics");
	ini.WriteString("statsExpandedTreeItems", statsExpandedTreeItems, "Statistics");

	// End SaveStats
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
	uint32 servtotal, servfail, servuser, servfile, servtuser, servtfile; float servocc;
	theApp.serverlist->GetStatus( servtotal, servfail, servuser, servfile, servtuser, servtfile, servocc );
	if ((servtotal-servfail)>cumSrvrsMostWorkingServers) cumSrvrsMostWorkingServers = (servtotal-servfail);
	if (servtuser>cumSrvrsMostUsersOnline) cumSrvrsMostUsersOnline = servtuser;
	if (servtfile>cumSrvrsMostFilesAvail) cumSrvrsMostFilesAvail = servtfile;

	// Shared Files
	if (theApp.sharedfiles->GetCount()>cumSharedMostFilesShared) cumSharedMostFilesShared = theApp.sharedfiles->GetCount();
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

	char* fullpath = new char[strlen(configdir)+MAX_PATH]; // i_a
	sprintf(fullpath,"%spreferences.ini",configdir);
	
	CIni ini( fullpath, "eMule" );

	delete[] fullpath;

	ini.WriteInt("DownCompletedFiles",			GetDownCompletedFiles(),		"Statistics");
	ini.WriteInt("DownSessionCompletedFiles",	GetDownSessionCompletedFiles(),	"Statistics");
} // SaveCompletedDownloadsStat()

void CPreferences::Add2SessionTransferData(uint8 uClientID, uint16 uClientPort, BOOL bFromPF, BOOL bUpDown, uint32 bytes, bool sentToFriend = false){ //MORPH - Added by Yun.SF3, ZZ Upload System


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
				case SO_EDONKEY:		sesUpData_EDONKEY+=bytes;		break;
				case SO_EDONKEYHYBRID:	sesUpData_EDONKEYHYBRID+=bytes;	break;
				case SO_OLDEMULE:
				case SO_EMULE:			sesUpData_EMULE+=bytes;			break;
				case SO_MLDONKEY:		sesUpData_MLDONKEY+=bytes;		break;
				case SO_CDONKEY:		sesUpData_CDONKEY+=bytes;		break;
				case SO_XMULE:			sesUpData_XMULE+=bytes;			break;
				case SO_SHAREAZA:		sesUpData_SHAREAZA+=bytes;		break;
			}

			switch (uClientPort){
				// Update session port breakdown stats for sent bytes...
				case 4662:				sesUpDataPort_4662+=bytes;		break;
				default:				sesUpDataPort_OTHER+=bytes;		break;
			}

			if (bFromPF)				sesUpData_Partfile+=bytes;
			else						sesUpData_File+=bytes;

			//	Add to our total for sent bytes...
			theApp.UpdateSentBytes(bytes, sentToFriend); //MORPH - Added by Yun.SF3, ZZ Upload System


			break;

		case false:
			// Downline Data

			switch (uClientID){
                // Update session client breakdown stats for received bytes...
			if (bFromPF)				sesUpData_Partfile+=bytes;
			else						sesUpData_File+=bytes;

			//	Add to our total for sent bytes...
			theApp.UpdateSentBytes(bytes);

			break;

		case false:
			// Downline Data

			switch (uClientID){
                // Update session client breakdown stats for received bytes...
				case SO_EDONKEY:		sesDownData_EDONKEY+=bytes;		break;
				case SO_EDONKEYHYBRID:	sesDownData_EDONKEYHYBRID+=bytes;break;
				case SO_OLDEMULE:
				case SO_EMULE:			sesDownData_EMULE+=bytes;		break;
				case SO_MLDONKEY:		sesDownData_MLDONKEY+=bytes;		break;
				case SO_CDONKEY:		sesDownData_CDONKEY+=bytes;		break;
				case SO_XMULE:			sesDownData_XMULE+=bytes;		break;
				case SO_SHAREAZA:		sesDownData_SHAREAZA+=bytes;		break;
			}

			switch (uClientPort){
				// Update session port breakdown stats for received bytes...
				// For now we are only going to break it down by default and non-default.
				// A statistical analysis of all data sent from every single port/domain is
				// beyond the scope of this add-on.
				case 4662:				sesDownDataPort_4662+=bytes;		break;
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
	cumUpData_CDONKEY=0;
	cumUpData_XMULE=0;
	cumUpData_SHAREAZA=0;
	cumUpDataPort_4662=0;
	cumUpDataPort_OTHER=0;
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
	cumDownData_CDONKEY=0;
	cumDownData_XMULE=0;
	cumDownData_SHAREAZA=0;
	cumDownDataPort_4662=0;
	cumDownDataPort_OTHER=0;
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
	time_t	timeNow;time(&timeNow);stat_datetimeLastReset = (__int64) timeNow;

	// Save the reset stats
	SaveStats();
	theApp.emuledlg->statisticswnd->ShowStatistics(true);

	// End Reset Statistics

}


// Load Statistics
// This used to be integrated in LoadPreferences, but it has been altered
// so that it can be used to load the backup created when the stats are reset.
// Last Modified: 2-22-03 by Khaos

bool CPreferences::LoadStats(int loadBackUp){
	// loadBackUp is 0 by default
	// loadBackUp = 0: Load the stats normally like we used to do in LoadPreferences
	// loadBackUp = 1: Load the stats from statbkup.ini and create a backup of the current stats.  Also, do not initialize session variables.
	// loadBackUp = 2: Load the stats from preferences.ini.old because the version has changed.
	char buffer[200];
	CString sINI;
	//uint64 temp64; moved
	CFileFind findBackUp;

	switch (loadBackUp) {
		case 0:
			sINI.Format("%spreferences.ini", configdir);
			break;
		case 1:
			sINI.Format("%sstatbkup.ini", configdir);
			if (!findBackUp.FindFile(sINI))
				return false;

			SaveStats(2); // Save our temp backup of current values to statbkuptmp.ini, we will be renaming it at the end of this function.
			break;
		case 2:
			sINI.Format("%spreferences.ini.old",configdir);
			break;
	}

	bool fileex=PathFileExists(sINI);
	CIni ini(sINI, "Statistics");

	sprintf(buffer , "%s", ini.GetString(			"TotalDownloadedBytes"			, 0 ) );
	totalDownloadedBytes=			_atoi64( buffer );

	sprintf(buffer , "%s", ini.GetString(			"TotalUploadedBytes"			, 0 ) );
	totalUploadedBytes=				_atoi64( buffer );

	// Load stats for cumulative downline overhead
	sprintf(buffer,"%s", ini.GetString(			"DownOverheadTotal"				, 0	) );
	cumDownOverheadTotal=			_atoi64( buffer );
	sprintf(buffer,"%s", ini.GetString(			"DownOverheadFileReq"			, 0	) );
	cumDownOverheadFileReq=			_atoi64( buffer );
	sprintf(buffer,"%s", ini.GetString(			"DownOverheadSrcEx"				, 0	) );
	cumDownOverheadSrcEx=			_atoi64( buffer );
	sprintf(buffer,"%s", ini.GetString(			"DownOverheadServer"			, 0	) );
	cumDownOverheadServer=			_atoi64( buffer );
	sprintf(buffer,"%s", ini.GetString(			"DownOverheadKad"				, 0	) );
	cumDownOverheadKad=				_atoi64( buffer );
	sprintf(buffer,"%s", ini.GetString(			"DownOverheadTotalPackets"		, 0 ) );
	cumDownOverheadTotalPackets=		_atoi64( buffer );
	sprintf(buffer,"%s", ini.GetString(			"DownOverheadFileReqPackets"	, 0 ) );
	cumDownOverheadFileReqPackets=	_atoi64( buffer );
	sprintf(buffer,"%s", ini.GetString(			"DownOverheadSrcExPackets"		, 0 ) );
	cumDownOverheadSrcExPackets=		_atoi64( buffer );
	sprintf(buffer,"%s", ini.GetString(			"DownOverheadServerPackets"		, 0 ) );
	cumDownOverheadServerPackets=	_atoi64( buffer );
	sprintf(buffer,"%s", ini.GetString(			"DownOverheadKadPackets"		, 0 ) );
	cumDownOverheadKadPackets=		_atoi64( buffer );

	// Load stats for cumulative upline overhead
	sprintf(buffer , "%s", ini.GetString(			"UpOverHeadTotal"				, 0 ) );
	cumUpOverheadTotal=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpOverheadFileReq"				, 0 ) );
	cumUpOverheadFileReq=			_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpOverheadSrcEx"				, 0 ) );
	cumUpOverheadSrcEx=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpOverheadServer"				, 0 ) );
	cumUpOverheadServer=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpOverheadKad"					, 0 ) );
	cumUpOverheadKad=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpOverHeadTotalPackets"		, 0 ) );
	cumUpOverheadTotalPackets=		_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpOverheadFileReqPackets"		, 0 ) );
	cumUpOverheadFileReqPackets=		_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpOverheadSrcExPackets"		, 0 ) );
	cumUpOverheadSrcExPackets=		_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpOverheadServerPackets"		, 0 ) );
	cumUpOverheadServerPackets=		_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpOverheadKadPackets"			, 0 ) );
	cumUpOverheadKadPackets=			_atoi64( buffer );

	// Load stats for cumulative upline data
	cumUpSuccessfulSessions =	ini.GetInt("UpSuccessfulSessions"	, 0 );
	cumUpFailedSessions =		ini.GetInt("UpFailedSessions"		, 0 );
	cumUpAvgTime =				ini.GetInt("UpAvgTime"				, 0 );

	// Load cumulative client breakdown stats for sent bytes
	sprintf(buffer , "%s", ini.GetString(			"UpData_EDONKEY"				, 0 ) );
	cumUpData_EDONKEY=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpData_EDONKEYHYBRID"			, 0 ) );
	cumUpData_EDONKEYHYBRID=			_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpData_EMULE"					, 0 ) );
	cumUpData_EMULE=					_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpData_MLDONKEY"				, 0 ) );
	cumUpData_MLDONKEY=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpData_LMULE"					, 0 ) );
	cumUpData_XMULE=					_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpData_CDONKEY"				, 0 ) );
	cumUpData_CDONKEY=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpData_SHAREAZA"				, 0 ) );
	cumUpData_SHAREAZA=				_atoi64( buffer );

	// Load cumulative port breakdown stats for sent bytes
	sprintf(buffer , "%s", ini.GetString(			"UpDataPort_4662"				, 0 ) );
	cumUpDataPort_4662=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpDataPort_OTHER"				, 0 ) );
	cumUpDataPort_OTHER=				_atoi64( buffer );

	// Load cumulative source breakdown stats for sent bytes
	sprintf(buffer , "%s", ini.GetString(			"UpData_File"					, 0 ) );
	cumUpData_File=					_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"UpData_Partfile"				, 0 ) );
	cumUpData_Partfile=				_atoi64( buffer );

	// Load stats for cumulative downline data
	cumDownCompletedFiles =		ini.GetInt("DownCompletedFiles"		, 0 );
	cumDownSuccessfulSessions=	ini.GetInt("DownSuccessfulSessions"	, 0 );
	cumDownFailedSessions=		ini.GetInt("DownFailedSessions"		, 0 );
	cumDownAvgTime=				ini.GetInt("DownAvgTime"			, 0 );

	// Cumulative statistics for saved due to compression/lost due to corruption
	sprintf(buffer , "%s", ini.GetString(			"LostFromCorruption"			, 0 ) );
	cumLostFromCorruption=			_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"SavedFromCompression"			, 0 ) );
	cumSavedFromCompression=			_atoi64( buffer );
	cumPartsSavedByICH=				ini.GetInt("PartsSavedByICH"		, 0 );

	// Load cumulative client breakdown stats for received bytes
	sprintf(buffer , "%s", ini.GetString(			"DownData_EDONKEY"				, 0 ) );
	cumDownData_EDONKEY=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"DownData_EDONKEYHYBRID"		, 0 ) );
	cumDownData_EDONKEYHYBRID=		_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"DownData_EMULE"				, 0 ) );
	cumDownData_EMULE=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"DownData_MLDONKEY"				, 0 ) );
	cumDownData_MLDONKEY=			_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"DownData_LMULE"				, 0 ) );
	cumDownData_XMULE=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"DownData_CDONKEY"				, 0 ) );
	cumDownData_CDONKEY=				_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"DownData_SHAREAZA"				, 0 ) );
	cumDownData_SHAREAZA=				_atoi64( buffer );

	// Load cumulative port breakdown stats for received bytes
	sprintf(buffer , "%s", ini.GetString(			"DownDataPort_4662"				, 0 ) );
	cumDownDataPort_4662=			_atoi64( buffer );
	sprintf(buffer , "%s", ini.GetString(			"DownDataPort_OTHER"			, 0 ) );
	cumDownDataPort_OTHER=			_atoi64( buffer );

	// Load stats for cumulative connection data
	cumConnAvgDownRate =		ini.GetFloat(	"ConnAvgDownRate"		, 0 );
	cumConnMaxAvgDownRate =	ini.GetFloat(	"ConnMaxAvgDownRate"	, 0 );
	cumConnMaxDownRate =		ini.GetFloat(	"ConnMaxDownRate"		, 0 );
	cumConnAvgUpRate =		ini.GetFloat(	"ConnAvgUpRate"			, 0 );
	cumConnMaxAvgUpRate =	ini.GetFloat(	"ConnMaxAvgUpRate"		, 0 );
	cumConnMaxUpRate =		ini.GetFloat(	"ConnMaxUpRate"			, 0 );

	sprintf(buffer , "%s", ini.GetString(			"ConnRunTime"					, 0 ) );
	cumConnRunTime=					_atoi64(buffer);

	cumConnTransferTime=			ini.GetInt(	"ConnTransferTime"			, 0 );
	cumConnDownloadTime=			ini.GetInt(	"ConnDownloadTime"			, 0 );
	cumConnUploadTime=			ini.GetInt(	"ConnUploadTime"			, 0 );
	cumConnServerDuration=		ini.GetInt( "ConnServerDuration"		, 0 );
	cumConnNumReconnects =		ini.GetInt(	"ConnNumReconnects"			, 0 );
	cumConnAvgConnections =		ini.GetInt(	"ConnAvgConnections"		, 0 );
	cumConnMaxConnLimitReached=	ini.GetInt(	"ConnMaxConnLimitReached"	, 0 );
	cumConnPeakConnections =		ini.GetInt(	"ConnPeakConnections"		, 0 );

	// Load date/time of last reset
	sprintf(buffer , "%s", ini.GetString(			"statsDateTimeLastReset"		, 0 ) );
	stat_datetimeLastReset=			_atoi64( buffer );

	// Smart Load For Restores - Don't overwrite records that are greater than the backed up ones
	if (loadBackUp == 1) {
		uint64 temp64;
		// Load records for servers / network
		if (ini.GetInt("SrvrsMostWorkingServers", 0) > cumSrvrsMostWorkingServers)
			cumSrvrsMostWorkingServers = ini.GetInt(		"SrvrsMostWorkingServers"	, 0 );
		if (ini.GetInt("SrvrsMostUsersOnline", 0) > (int)cumSrvrsMostUsersOnline)
			cumSrvrsMostUsersOnline =	ini.GetInt(		"SrvrsMostUsersOnline"		, 0 );
		if (ini.GetInt("SrvrsMostFilesAvail", 0) > (int)cumSrvrsMostFilesAvail)
			cumSrvrsMostFilesAvail =		ini.GetInt(		"SrvrsMostFilesAvail"		, 0 );

		// Load records for shared files
		if (ini.GetInt("SharedMostFilesShared ", 0, "Statistics") > cumSharedMostFilesShared)
			cumSharedMostFilesShared =	ini.GetInt(		"SharedMostFilesShared"		, 0 );

		sprintf(buffer , "%s", ini.GetString(	"SharedLargestShareSize" , 0 ) );
		temp64 = _atoi64( buffer );
		if (temp64 > cumSharedLargestShareSize) cumSharedLargestShareSize = temp64;
		sprintf(buffer , "%s", ini.GetString( "SharedLargestAvgFileSize" , 0 ) );
		temp64 = _atoi64( buffer );
		if (temp64 > cumSharedLargestAvgFileSize) cumSharedLargestAvgFileSize = temp64;
		sprintf(buffer , "%s", ini.GetString( "SharedLargestFileSize" , 0 ) );
		temp64 = _atoi64( buffer );
		if (temp64 > cumSharedLargestFileSize) cumSharedLargestFileSize = temp64;

		// Check to make sure the backup of the values we just overwrote exists.  If so, rename it to the backup file.
		// This allows us to undo a restore, so to speak, just in case we don't like the restored values...
		CString sINIBackUp;
		sINIBackUp.Format("%sstatbkuptmp.ini", configdir);
		if (findBackUp.FindFile(sINIBackUp)) {		
			CFile::Remove(sINI); // Remove the backup that we just restored from
			CFile::Rename(sINIBackUp, sINI); // Rename our temporary backup to the normal statbkup.ini filename.
		}

		// Since we know this is a restore, now we should call ShowStatistics to update the data items to the new ones we just loaded.
		// Otherwise user is left waiting around for the tick counter to reach the next automatic update (Depending on setting in prefs)
		theApp.emuledlg->statisticswnd->ShowStatistics();
		
	}
	// Stupid Load -> Just load the values.
	else {
		// Load records for servers / network
		cumSrvrsMostWorkingServers = ini.GetInt(		"SrvrsMostWorkingServers"	, 0 );
		cumSrvrsMostUsersOnline =	ini.GetInt(		"SrvrsMostUsersOnline"		, 0 );
		cumSrvrsMostFilesAvail =		ini.GetInt(		"SrvrsMostFilesAvail"		, 0 );

		// Load records for shared files
		cumSharedMostFilesShared =	ini.GetInt(		"SharedMostFilesShared"		, 0 );

		sprintf(buffer , "%s", ini.GetString(		"SharedLargestShareSize"					, 0 ) );
		cumSharedLargestShareSize=	_atoi64( buffer );
		sprintf(buffer , "%s", ini.GetString(		"SharedLargestAvgFileSize"					, 0 ) );
		cumSharedLargestAvgFileSize=	_atoi64( buffer );
		sprintf(buffer , "%s", ini.GetString(		"SharedLargestFileSize"						, 0 ) );
		cumSharedLargestFileSize =	_atoi64( buffer );

		// These are not stats, but they're part of my mod, so we will load them here anyway.
		statsConnectionsGraphRatio =		ini.GetInt("statsConnectionsGraphRatio"	, 3	, "Statistics");
		sprintf(statsExpandedTreeItems,"%s",ini.GetString("statsExpandedTreeItems","111000000100000110000010000011110000010010","Statistics"));

		// Initialize new session statistic variables...
		sesDownCompletedFiles =		0;
		sesUpData_EDONKEY =			0;
		sesUpData_EDONKEYHYBRID =	0;
		sesUpData_EMULE =			0;
		sesUpData_MLDONKEY =			0;
		sesUpData_CDONKEY =			0;
		sesUpDataPort_4662 =			0;
		sesUpDataPort_OTHER =		0;
		sesDownData_EDONKEY =		0;
		sesDownData_EDONKEYHYBRID =	0;
		sesDownData_EMULE =			0;
		sesDownData_MLDONKEY =		0;
		sesDownData_CDONKEY =		0;
		sesDownData_SHAREAZA =		0;
		sesDownDataPort_4662 =		0;
		sesDownDataPort_OTHER =		0;
		sesDownSuccessfulSessions=	0;
		sesDownFailedSessions=		0;
		sesPartsSavedByICH=			0;
	}

	if (!fileex) {time_t	timeNow;time(&timeNow);stat_datetimeLastReset = (__int64) timeNow;}
	
	return true;

	// End Load Stats
}

// This formats the UCT long value that is saved for stat_datetimeLastReset
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
			_tcsftime(szDateReset, ARRSIZE(szDateReset), formatLong ? GetDateTimeFormat() : GetDateTimeFormat4Log(), statsReset);
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
	char* fullpath = new char[strlen(configdir)+MAX_PATH]; // i_a
	sprintf(fullpath,"%spreferences.dat",configdir);

	FILE* preffile = fopen(fullpath,"wb");
	prefsExt->version = PREFFILE_VERSION;

	// -khaos--+++> Don't save stats if preferences.ini doesn't exist yet (Results in unhandled exception).
	sprintf(fullpath,"%spreferences.ini",configdir);
	bool bSaveStats = true;
	if (!PathFileExists(fullpath))
		bSaveStats = false;
	// <-----khaos-

	delete[] fullpath;
	if (preffile){
		prefsExt->version=PREFFILE_VERSION;
		prefsExt->EmuleWindowPlacement=EmuleWindowPlacement;
		md4cpy(prefsExt->userhash, userhash);

		error = fwrite(prefsExt,sizeof(Preferences_Ext_Struct),1,preffile);
		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			fflush(preffile); // flush file stream buffers to disk buffers
			(void)_commit(_fileno(preffile)); // commit disk buffers to disk
		}
		fclose(preffile);
	}
	else
		error = true;

	SavePreferences();
	// -khaos--+++> SaveStats is now called here instead of from SavePreferences...
	if (bSaveStats)
		SaveStats();
	// <-----khaos-
	// khaos::categorymod+ We need to SaveCats() each time we exit eMule.
	SaveCats();
	// khaos::categorymod-

	fullpath = new char[strlen(configdir)+14];
	sprintf(fullpath,"%sshareddir.dat",configdir);
	CStdioFile sdirfile;
	if (sdirfile.Open(fullpath,CFile::modeCreate|CFile::modeWrite))
	{
		try{
			for (POSITION pos = shareddir_list.GetHeadPosition();pos != 0;){
				sdirfile.WriteString(shareddir_list.GetNext(pos).GetBuffer());
				sdirfile.Write("\n",1);
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
				AddDebugLogLine(true,_T("Failed to save %s - %s"), fullpath, buffer);
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
		uint16 random = rand();
		memcpy(&userhash[i*2], &random, 2);
	}

	// mark as emule client. that will be need in later version
	userhash[5] = 14;
	userhash[14] = 111;
}

int CPreferences::GetColumnWidth(Table t, int index)
{
	switch(t) {
	case tableDownload:
		return downloadColumnWidths[index];
	case tableUpload:
		return uploadColumnWidths[index];
	case tableQueue:
		return queueColumnWidths[index];
	case tableSearch:
		return searchColumnWidths[index];
	case tableShared:
		return sharedColumnWidths[index];
	case tableServer:
		return serverColumnWidths[index];
	case tableClientList:
		return clientListColumnWidths[index];
	case tableFilenames:
		return FilenamesListColumnWidths[index];
	}
	return 0;
}

void CPreferences::SetColumnWidth(Table t, int index, int width) {
	switch(t) {
	case tableDownload:
		downloadColumnWidths[index] = width;
		break;
	case tableUpload:
		uploadColumnWidths[index] = width;
		break;
	case tableQueue:
		queueColumnWidths[index] = width;
		break;
	case tableSearch:
		searchColumnWidths[index] = width;
		break;
	case tableShared:
		sharedColumnWidths[index] = width;
		break;
	case tableServer:
		serverColumnWidths[index] = width;
		break;
	case tableClientList:
		clientListColumnWidths[index] = width;
		break;
	case tableFilenames:
		FilenamesListColumnWidths[index] = width;
		break;
	}
}

BOOL CPreferences::GetColumnHidden(Table t, int index)
{
	switch(t) {
	case tableDownload:
		return downloadColumnHidden[index];
	case tableUpload:
		return uploadColumnHidden[index];
	case tableQueue:
		return queueColumnHidden[index];
	case tableSearch:
		return searchColumnHidden[index];
	case tableShared:
		return sharedColumnHidden[index];
	case tableServer:
		return serverColumnHidden[index];
	case tableClientList:
		return clientListColumnHidden[index];
	case tableFilenames:
		return FilenamesListColumnHidden[index];
	}
	return FALSE;
}

void CPreferences::SetColumnHidden(Table t, int index, BOOL bHidden) {
	switch(t) {
	case tableDownload:
		downloadColumnHidden[index] = bHidden;
		break;
	case tableUpload:
		uploadColumnHidden[index] = bHidden;
		break;
	case tableQueue:
		queueColumnHidden[index] = bHidden;
		break;
	case tableSearch:
		searchColumnHidden[index] = bHidden;
		break;
	case tableShared:
		sharedColumnHidden[index] = bHidden;
		break;
	case tableServer:
		serverColumnHidden[index] = bHidden;
		break;
	case tableClientList:
		clientListColumnHidden[index] = bHidden;
		break;
	case tableFilenames:
		FilenamesListColumnHidden[index] = bHidden;
		break;
	}
}

int CPreferences::GetColumnOrder(Table t, int index)
{
	switch(t) {
	case tableDownload:
		return downloadColumnOrder[index];
	case tableUpload:
		return uploadColumnOrder[index];
	case tableQueue:
		return queueColumnOrder[index];
	case tableSearch:
		return searchColumnOrder[index];
	case tableShared:
		return sharedColumnOrder[index];
	case tableServer:
		return serverColumnOrder[index];
	case tableClientList:
		return clientListColumnOrder[index];
	case tableFilenames:
		return FilenamesListColumnOrder[index];
	}
	return 0;
}

void CPreferences::SetColumnOrder(Table t, INT *piOrder) {
	switch(t) {
	case tableDownload:
		memcpy(downloadColumnOrder, piOrder, sizeof(downloadColumnOrder));
		break;
	case tableUpload:
		memcpy(uploadColumnOrder, piOrder, sizeof(uploadColumnOrder));
		break;
	case tableQueue:
		memcpy(queueColumnOrder, piOrder, sizeof(queueColumnOrder));
		break;
	case tableSearch:
		memcpy(searchColumnOrder, piOrder, sizeof(searchColumnOrder));
		break;
	case tableShared:
		memcpy(sharedColumnOrder, piOrder, sizeof(sharedColumnOrder));
		break;
	case tableServer:
		memcpy(serverColumnOrder, piOrder, sizeof(serverColumnOrder));
		break;
	case tableClientList:
		memcpy(clientListColumnOrder, piOrder, sizeof(clientListColumnOrder));
		break;
	case tableFilenames:
		memcpy(FilenamesListColumnOrder, piOrder, sizeof(FilenamesListColumnOrder));
		break;
	}
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

void CPreferences::SavePreferences(){
	CString buffer;
	char* fullpath = new char[strlen(configdir)+MAX_PATH]; // i_a
	sprintf(fullpath,"%spreferences.ini",configdir);
	
	CIni ini( fullpath, "eMule" );
	delete[] fullpath;
	fullpath=NULL;
	//---
	ini.WriteString("AppVersion", theApp.m_strCurVersionLong);
	//---

#ifdef _DEBUG
	ini.WriteInt("DebugHeap", m_iDbgHeap);
#endif

	ini.WriteString("Nick", nick);
	ini.WriteString("IncomingDir", incomingdir);
	ini.WriteString("TempDir", tempdir);

	// ZZ:UploadSpeedSense -->
    	ini.WriteInt("MinUpload", minupload);
	// ZZ:UploadSpeedSense <--
	ini.WriteInt("MaxUpload",maxupload);
	ini.WriteInt("MaxDownload",maxdownload);
	ini.WriteInt("MaxConnections",maxconnections);
	ini.WriteInt("RemoveDeadServer",deadserver);
	ini.WriteInt("Port",port);
	ini.WriteInt("UDPPort",udpport);
	ini.WriteInt("ServerUDPPort", nServerUDPPort);
	ini.WriteInt("MaxSourcesPerFile",maxsourceperfile );
	ini.WriteWORD("Language",languageID);
	ini.WriteInt("SeeShare",m_iSeeShares);
	ini.WriteInt("ToolTipDelay",m_iToolDelayTime);
	ini.WriteInt("StatGraphsInterval",trafficOMeterInterval);
	ini.WriteInt("StatsInterval",statsInterval);
	ini.WriteInt("DownloadCapacity",maxGraphDownloadRate);
	ini.WriteInt("UploadCapacity",maxGraphUploadRate);
	ini.WriteInt("DeadServerRetry",deadserverretries);
	ini.WriteInt("ServerKeepAliveTimeout",m_dwServerKeepAliveTimeout);
	ini.WriteInt("SplitterbarPosition",splitterbarPosition+2);
	ini.WriteInt("VariousStatisticsMaxValue",statsMax);
	ini.WriteInt("StatsAverageMinutes",statsAverageMinutes);
	ini.WriteInt("MaxConnectionsPerFiveSeconds",MaxConperFive);
	ini.WriteInt("Check4NewVersionDelay",versioncheckdays);

	ini.WriteBool("Reconnect",reconnect);
	ini.WriteBool("Scoresystem",scorsystem);
	ini.WriteBool("ICH",ICH);
	ini.WriteBool("Serverlist",autoserverlist);
	ini.WriteBool("UpdateNotifyTestClient",updatenotify);
	ini.WriteBool("MinToTray",mintotray);
	ini.WriteBool("AddServersFromServer",addserversfromserver);
	ini.WriteBool("AddServersFromClient",addserversfromclient);
	ini.WriteBool("Splashscreen",splashscreen);
	ini.WriteBool("BringToFront",bringtoforeground);
	ini.WriteBool("TransferDoubleClick",transferDoubleclick);
	ini.WriteBool("BeepOnError",beepOnError);
	ini.WriteBool("ConfirmExit",confirmExit);
	ini.WriteBool("FilterBadIPs",filterLANIPs);
    ini.WriteBool("Autoconnect",autoconnect);
	ini.WriteBool("OnlineSignature",onlineSig);
	ini.WriteBool("StartupMinimized",startMinimized);
	ini.WriteInt("LastMainWndDlgID",m_iLastMainWndDlgID);
	ini.WriteInt("LastLogPaneID",m_iLastLogPaneID);
	ini.WriteBool("SafeServerConnect",safeServerConnect);
	ini.WriteBool("ShowRatesOnTitle",showRatesInTitle);
	ini.WriteBool("IndicateRatings",indicateratings);
	ini.WriteBool("WatchClipboard4ED2kFilelinks",watchclipboard);
	ini.WriteInt("SearchMethod",m_iSearchMethod);
	ini.WriteBool("CheckDiskspace",checkDiskspace);	// SLUGFILLER: checkDiskspace
	ini.WriteInt("MinFreeDiskSpace",m_uMinFreeDiskSpace);
	// itsonlyme: hostnameSource
	buffer.Format("%s",yourHostname);
	ini.WriteString("YourHostname",buffer);
	// itsonlyme: hostnameSource

	// Barry - New properties...
    ini.WriteBool("AutoConnectStaticOnly", autoconnectstaticonly);  
	ini.WriteBool("AutoTakeED2KLinks", autotakeed2klinks);  
    ini.WriteBool("AddNewFilesPaused", addnewfilespaused);  
    ini.WriteInt ("3DDepth", depth3D);  

	ini.WriteBool("NotifyOnDownload",useDownloadNotifier); // Added by enkeyDEV
	ini.WriteBool("NotifyOnNewDownload",useNewDownloadNotifier);
	ini.WriteBool("NotifyOnChat",useChatNotifier);		  
	ini.WriteBool("NotifyOnLog",useLogNotifier);
	ini.WriteBool("NotifierUseSound",useSoundInNotifier);
	ini.WriteBool("NotifierPopEveryChatMessage",notifierPopsEveryChatMsg);
	ini.WriteBool("NotifierPopNewVersion",notifierNewVersion);
	ini.WriteBool("NotifyOnImportantError", notifierImportantError);
	ini.WriteString("NotifierSoundPath",notifierSoundFilePath);
	ini.WriteString("NotifierConfiguration",notifierConfiguration);

	ini.WriteString("TxtEditor",TxtEditor);
	ini.WriteString("VideoPlayer",VideoPlayer);
	ini.WriteString("MessageFilter",messageFilter);
	ini.WriteString("CommentFilter",commentFilter);
	ini.WriteString("DateTimeFormat",GetDateTimeFormat());
	ini.WriteString("DateTimeFormat4Log",GetDateTimeFormat4Log());
	ini.WriteString("WebTemplateFile",m_sTemplateFile);
	ini.WriteString("FilenameCleanups",filenameCleanups);
	ini.WriteInt("ExtractMetaData",m_iExtractMetaData);

	ini.WriteString("DefaultIRCServer",m_sircserver);
	ini.WriteString("IRCNick",m_sircnick);
	ini.WriteBool("IRCAddTimestamp", m_bircaddtimestamp);
	ini.WriteString("IRCFilterName", m_sircchannamefilter);
	ini.WriteInt("IRCFilterUser", m_iircchanneluserfilter);
	ini.WriteBool("IRCUseFilter", m_bircusechanfilter);
	ini.WriteString("IRCPerformString", m_sircperformstring);
	ini.WriteBool("IRCUsePerform", m_bircuseperform);
	ini.WriteBool("IRCListOnConnect", m_birclistonconnect);
	ini.WriteBool("IRCAcceptLink", m_bircacceptlinks);
	ini.WriteBool("IRCAcceptLinkFriends", m_bircacceptlinksfriends);
	ini.WriteBool("IRCIgnoreInfoMessage", m_bircignoreinfomessage);
	ini.WriteBool("IRCIgnoreEmuleProtoInfoMessage", m_bircignoreemuleprotoinfomessage);
	ini.WriteBool("IRCHelpChannel", m_birchelpchannel);
	ini.WriteBool("SmartIdCheck", smartidcheck);
	ini.WriteBool("Verbose", m_bVerbose);
	ini.WriteBool("DebugSourceExchange", m_bDebugSourceExchange);	// do *not* use the according 'Get...' function here!
	ini.WriteBool("LogBannedClients", m_bLogBannedClients);			// do *not* use the according 'Get...' function here!
	ini.WriteBool("LogRatingDescReceived", m_bLogRatingDescReceived);// do *not* use the according 'Get...' function here!
	ini.WriteBool("LogSecureIdent", m_bLogSecureIdent);				// do *not* use the according 'Get...' function here!
	ini.WriteBool("LogFilteredIPs", m_bLogFilteredIPs);				// do *not* use the according 'Get...' function here!
	ini.WriteBool("LogFileSaving", m_bLogFileSaving);				// do *not* use the according 'Get...' function here!
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	// following options are for debugging or when using an external debug device viewer only.
	ini.WriteInt("DebugServerTCP",m_iDebugServerTCPLevel);
	ini.WriteInt("DebugServerUDP",m_iDebugServerUDPLevel);
	ini.WriteInt("DebugServerSources",m_iDebugServerSourcesLevel);
	ini.WriteInt("DebugServerSearches",m_iDebugServerSearchesLevel);
	ini.WriteInt("DebugClientTCP",m_iDebugClientTCPLevel);
	ini.WriteInt("DebugClientUDP",m_iDebugClientUDPLevel);
	ini.WriteInt("DebugClientKadUDP",m_iDebugClientKadUDPLevel);
#endif
	ini.WriteBool("PreviewPrio", m_bpreviewprio);
	ini.WriteBool("UpdateQueueListPref", m_bupdatequeuelist);
	ini.WriteBool("ManualHighPrio", m_bmanualhighprio);
	ini.WriteBool("FullChunkTransfers", m_btransferfullchunks);
	ini.WriteBool("StartNextFile", m_bstartnextfile);
	ini.WriteBool("ShowOverhead", m_bshowoverhead);
	ini.WriteBool("VideoPreviewBackupped", moviePreviewBackup);
	ini.WriteInt("PreviewSmallBlocks", m_iPreviewSmallBlocks);

	ini.DeleteKey("FileBufferSizePref"); // delete old 'file buff size' setting
	ini.WriteInt("FileBufferSize", m_iFileBufferSize);

	ini.DeleteKey("QueueSizePref"); // delete old 'queue size' setting
	ini.WriteInt("QueueSize", m_iQueueSize);

	ini.WriteInt("CommitFiles", m_iCommitFiles);
	ini.WriteBool("DAPPref", m_bDAP);
	ini.WriteBool("UAPPref", m_bUAP);
	// khaos::kmod+ Obsolete ini.WriteInt("AllcatType", allcatType);
	ini.WriteBool("FilterServersByIP",filterserverbyip);
	ini.WriteBool("DisableKnownClientList",m_bDisableKnownClientList);
	ini.WriteBool("DisableQueueList",m_bDisableQueueList);
	ini.WriteBool("UseCreditSystem",m_bCreditSystem);
	ini.WriteBool("SaveLogToDisk",log2disk);
	ini.WriteBool("SaveDebugToDisk",debug2disk);
	ini.WriteBool("EnableScheduler",scheduler);
	ini.WriteBool("MessagesFromFriendsOnly",msgonlyfriends);
	ini.WriteBool("MessageFromValidSourcesOnly",msgsecure);
	ini.WriteBool("ShowInfoOnCatTabs",showCatTabInfos);
	ini.WriteBool("ResumeNextFromSameCat",resumeSameCat);
	ini.WriteBool("DontRecreateStatGraphsOnResize",dontRecreateGraphs);
	ini.WriteBool("AutoFilenameCleanup",autofilenamecleanup);
	ini.WriteBool("ShowExtControls",m_bExtControls);
	ini.WriteBool("UseAutocompletion",m_bUseAutocompl);
	ini.WriteBool("NetworkKademlia",networkkademlia);
	ini.WriteBool("NetworkED2K",networked2k);
	ini.WriteBool("AutoClearCompleted",m_bRemoveFinishedDownloads);
	ini.WriteBool("TransflstRemainOrder",m_bTransflstRemain);

	ini.WriteInt("VersionCheckLastAutomatic", versioncheckLastAutomatic);
	ini.WriteInt("FilterLevel",filterlevel);

	ini.WriteBool("SecureIdent", m_bUseSecureIdent);// change the name in future version to enable it by default
	ini.WriteBool("AdvancedSpamFilter",m_bAdvancedSpamfilter);
	ini.WriteBool("ShowDwlPercentage",m_bShowDwlPercentage);		
	ini.WriteBool("RemoveFilesToBin",m_bRemove2bin);

	// Toolbar
	ini.WriteString("ToolbarSetting", m_sToolbarSettings);
	ini.WriteString("ToolbarBitmap", m_sToolbarBitmap );
	ini.WriteString("ToolbarBitmapFolder", m_sToolbarBitmapFolder);
	ini.WriteInt("ToolbarLabels", m_nToolbarLabels);
	ini.WriteString("SkinProfile", m_szSkinProfile);
	ini.WriteString("SkinProfileDir", m_szSkinProfileDir);

	
	ini.SerGet(false, downloadColumnWidths,
		ARRSIZE(downloadColumnWidths), "DownloadColumnWidths");
	ini.SerGet(false, downloadColumnHidden,
		ARRSIZE(downloadColumnHidden), "DownloadColumnHidden");
	ini.SerGet(false, downloadColumnOrder,
		ARRSIZE(downloadColumnOrder), "DownloadColumnOrder");
	ini.SerGet(false, uploadColumnWidths,
		ARRSIZE(uploadColumnWidths), "UploadColumnWidths");
	ini.SerGet(false, uploadColumnHidden,
		ARRSIZE(uploadColumnHidden), "UploadColumnHidden");
	ini.SerGet(false, uploadColumnOrder,
		ARRSIZE(uploadColumnOrder), "UploadColumnOrder");
	ini.SerGet(false, queueColumnWidths,
		ARRSIZE(queueColumnWidths), "QueueColumnWidths");
	ini.SerGet(false, queueColumnHidden,
		ARRSIZE(queueColumnHidden), "QueueColumnHidden");
	ini.SerGet(false, queueColumnOrder,
		ARRSIZE(queueColumnOrder), "QueueColumnOrder");
	ini.SerGet(false, searchColumnWidths,
		ARRSIZE(searchColumnWidths), "SearchColumnWidths");
	ini.SerGet(false, searchColumnHidden,
		ARRSIZE(searchColumnHidden), "SearchColumnHidden");
	ini.SerGet(false, searchColumnOrder,
		ARRSIZE(searchColumnOrder), "SearchColumnOrder");
	ini.SerGet(false, sharedColumnWidths,
		ARRSIZE(sharedColumnWidths), "SharedColumnWidths");
	ini.SerGet(false, sharedColumnHidden,
		ARRSIZE(sharedColumnHidden), "SharedColumnHidden");
	ini.SerGet(false, sharedColumnOrder,
		ARRSIZE(sharedColumnOrder), "SharedColumnOrder");
	ini.SerGet(false, serverColumnWidths,
		ARRSIZE(serverColumnWidths), "ServerColumnWidths");
	ini.SerGet(false, serverColumnHidden,
		ARRSIZE(serverColumnHidden), "ServerColumnHidden");
	ini.SerGet(false, serverColumnOrder,
		ARRSIZE(serverColumnOrder), "ServerColumnOrder");
	ini.SerGet(false, clientListColumnWidths,
		ARRSIZE(clientListColumnWidths), "ClientListColumnWidths");
	ini.SerGet(false, clientListColumnHidden,
		ARRSIZE(clientListColumnHidden), "ClientListColumnHidden");
	ini.SerGet(false, clientListColumnOrder,
		ARRSIZE(clientListColumnOrder), "ClientListColumnOrder");

	ini.SerGet(false, FilenamesListColumnWidths,
		ARRSIZE(FilenamesListColumnWidths), "FilenamesListColumnWidths");
	ini.SerGet(false, FilenamesListColumnHidden,
		ARRSIZE(FilenamesListColumnHidden), "FilenamesListColumnHidden");
	ini.SerGet(false, FilenamesListColumnOrder,
		ARRSIZE(FilenamesListColumnOrder), "FilenamesListColumnOrder");

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	// SLUGFILLER: multiSort - save multiple params
	ini.SerGet(false, tableSortItemDownload,
		GetColumnSortCount(tableDownload), "TableSortItemDownload");
	ini.SerGet(false, tableSortItemUpload,
		GetColumnSortCount(tableUpload), "TableSortItemUpload");
	ini.SerGet(false, tableSortItemQueue,
		GetColumnSortCount(tableQueue), "TableSortItemQueue");
	ini.SerGet(false, tableSortItemSearch,
		GetColumnSortCount(tableSearch), "TableSortItemSearch");
	ini.SerGet(false, tableSortItemShared,
		GetColumnSortCount(tableShared), "TableSortItemShared");
	ini.SerGet(false, tableSortItemServer,
		GetColumnSortCount(tableServer), "TableSortItemServer");
	ini.SerGet(false, tableSortItemClientList,
		GetColumnSortCount(tableClientList), "TableSortItemClientList");
	ini.SerGet(false, tableSortItemFilenames,
		GetColumnSortCount(tableFilenames), "TableSortItemFilenames");
	ini.SerGet(false, tableSortAscendingDownload,
		GetColumnSortCount(tableDownload), "TableSortAscendingDownload");
	ini.SerGet(false, tableSortAscendingUpload,
		GetColumnSortCount(tableUpload), "TableSortAscendingUpload");
	ini.SerGet(false, tableSortAscendingQueue,
		GetColumnSortCount(tableQueue), "TableSortAscendingQueue");
	ini.SerGet(false, tableSortAscendingSearch,
		GetColumnSortCount(tableSearch), "TableSortAscendingSearch");
	ini.SerGet(false, tableSortAscendingShared,
		GetColumnSortCount(tableShared), "TableSortAscendingShared");
	ini.SerGet(false, tableSortAscendingServer,
		GetColumnSortCount(tableServer), "TableSortAscendingServer");
	ini.SerGet(false, tableSortAscendingClientList,
		GetColumnSortCount(tableClientList), "TableSortAscendingClientList");
	ini.SerGet(false, tableSortAscendingFilenames,
		GetColumnSortCount(tableFilenames), "TableSortAscendingFilenames");
	// SLUGFILLER: multiSort
	ini.WriteBinary("HyperTextFont", (LPBYTE)&m_lfHyperText, sizeof m_lfHyperText);

	// deadlake PROXYSUPPORT
	ini.WriteBool("ProxyEnablePassword",proxy.EnablePassword,"Proxy");
	ini.WriteBool("ProxyEnableProxy",proxy.UseProxy,"Proxy");
	ini.WriteString("ProxyName",proxy.name,"Proxy");
	ini.WriteString("ProxyPassword",proxy.password,"Proxy");
	ini.WriteString("ProxyUser",proxy.user,"Proxy");
	ini.WriteInt("ProxyPort",proxy.port,"Proxy");
	ini.WriteInt("ProxyType",proxy.type,"Proxy");
	ini.WriteBool("ConnectWithoutProxy",m_bIsASCWOP,"Proxy");
	ini.WriteBool("ShowErrors",m_bShowProxyErrors,"Proxy");

	CString buffer2;
	for (int i=0;i<15;i++) {
		buffer.Format("0x%06x",GetStatsColor(i));
		buffer2.Format("StatColor%i",i);
		ini.WriteString(buffer2,buffer,"Statistics");
	}

	// -khaos--+++>
	/* Original stat saves from base code now obsolete (KHAOS)
	buffer.Format("%I64u",totalDownloadedBytes);
	ini.WriteString("TotalDownloadedBytes",buffer ,"Statistics");

	buffer.Format("%I64u",totalUploadedBytes);
	ini.WriteString("TotalUploadedBytes",buffer ,"Statistics");
	// End original stat saves from base code. */
	// <-----khaos--

	// Web Server
	ini.WriteString("Password", GetWSPass(), "WebServer");
	ini.WriteString("PasswordLow", GetWSLowPass());
	ini.WriteInt("Port", m_nWebPort);
	ini.WriteBool("Enabled", m_bWebEnabled);
	ini.WriteBool("UseGzip", m_bWebUseGzip);
	ini.WriteInt("PageRefreshTime", m_nWebPageRefresh);
	ini.WriteBool("UseLowRightsUser", m_bWebLowEnabled);

	//mobileMule
	ini.WriteString("Password", GetMMPass(), "MobileMule");
	ini.WriteBool("Enabled", m_bMMEnabled);
	ini.WriteInt("Port", m_nMMPort);

	ini.WriteBool("InfiniteQueue",prefs->infiniteQueue);	//Morph - added by AndCycle, SLUGFILLER: infiniteQueue
	ini.WriteBool("MultipleInstance",prefs->multipleInstance);	//Morph - added by AndCycle, VQB: multipleInstance

	ini.WriteBool("AutoDynUpSwitching",prefs->isautodynupswitching);//MORPH - Added by Yun.SF3, Auto DynUp changing
	ini.WriteBool("AutoPowershareNewDownloadFile",prefs->m_bisautopowersharenewdownloadfile); //MORPH - Added by SiRoB, Avoid misusing of powersharing

//MORPH START - Added by IceCream, Defeat 0-filled Part Senders from Maella
	// Maella -Defeat 0-filled Part Senders- (Idea of xrmb)
	ini.WriteBool("EnableZeroFilledTest", prefs->enableZeroFilledTest);
	// Maella end
//MORPH END   - Added by IceCream, Defeat 0-filled Part Senders from Maella

	ini.WriteBool("EnableHighProcess", enableHighProcess); //MORPH - Added by IceCream, high process priority

	ini.WriteBool("EnableDownloadInRed", prefs->enableDownloadInRed); //MORPH - Added by IceCream, show download in red
	ini.WriteBool("EnableDownloadInBold", prefs->enableDownloadInBold); //MORPH - Added by SiRoB, show download in Bold
	ini.WriteBool("EnableAntiLeecher", prefs->enableAntiLeecher); //MORPH - Added by IceCream, enable AntiLeecher
	ini.WriteBool("EnableAntiCreditHack", prefs->enableAntiCreditHack); //MORPH - Added by IceCream, enable AntiCreditHack
	ini.WriteInt("CreditSystemMode", prefs->creditSystemMode);// EastShare - Added by linekin, ES CreditSystem
	ini.WriteInt("EqualChanceForEachFileMode", prefs->equalChanceForEachFileMode);//Morph - added by AndCycle, Equal Chance For Each File
	ini.WriteBool("ECFEFallTime", prefs->m_bECFEFallTime);//Morph - added by AndCycle, Equal Chance For Each File
	ini.WriteBool("IsUSSLimit", prefs->m_bIsUSSLimit); // EastShare - Added by TAHO, does USS limit
	ini.WriteBool("IsBoostFriends", prefs->isboostfriends);//Added by Yun.SF3, boost friends

	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	ini.WriteInt("ReconnectOnLowIdRetries",prefs->LowIdRetries);	// SLUGFILLER: lowIdRetry
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry
	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	ini.WriteInt("HideOvershares",prefs->hideOS);
	ini.WriteBool("SelectiveShare",prefs->selectiveShare);
	//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	ini.WriteBool("ShareOnlyTheNeed",prefs->ShareOnlyTheNeed);
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
	//MORPH START - Added by SiRoB, Show Permissions
	ini.WriteBool("ShowSharePermissions",prefs->permissions);
	//MORPH END   - Added by SiRoB, Show Permissions

//MORPH START added by Yun.SF3: Ipfilter.dat update
	ini.WriteInt("IPfilterVersion",prefs->m_IPfilterVersion); //added by milobac: Ipfilter.dat update
	ini.WriteBool("AutoUPdateIPFilter",prefs->AutoUpdateIPFilter); //added by milobac: Ipfilter.dat update
//MORPH END added by Yun.SF3: Ipfilter.dat update

	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	ini.WriteInt("FakesDatVersion",prefs->m_FakesDatVersion);
	ini.WriteBool("UpdateFakeStartup",prefs->UpdateFakeStartup);
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating

	ini.WriteString("UpdateURLFakeList",prefs->UpdateURLFakeList);		//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	ini.WriteString("UpdateURLIPFilter",prefs->UpdateURLIPFilter);//MORPH START added by Yun.SF3: Ipfilter.dat update

	//EastShare Start - PreferShareAll by AndCycle
	ini.WriteBool("ShareAll",prefs->shareall);	// SLUGFILLER: preferShareAll
	//EastShare END - PreferShareAll by AndCycle
	// EastShare START - Added by TAHO, .met file control
	ini.WriteInt("KnownMetDays", prefs->m_iKnownMetDays);
	// EastShare END - Added by TAHO, .met file control

	//EastShare - added by AndCycle, IP to Country
	ini.WriteInt("IP2Country", prefs->m_iIP2CountryNameMode); 
	ini.WriteBool("IP2CountryShowFlag", prefs->m_bIP2CountryShowFlag);
	//EastShare - added by AndCycle, IP to Country

	// khaos::categorymod+ Save Preferences
	ini.WriteBool("ValidSrcsOnly", m_bValidSrcsOnly);
	ini.WriteBool("ShowCatName", m_bShowCatNames);
	ini.WriteBool("ActiveCatDefault", m_bActiveCatDefault);
	ini.WriteBool("SelCatOnAdd", m_bSelCatOnAdd);
	ini.WriteBool("AutoSetResumeOrder", m_bAutoSetResumeOrder);
	ini.WriteBool("SmallFileDLPush", m_bSmallFileDLPush);
	ini.WriteInt("StartDLInEmptyCats", m_iStartDLInEmptyCats);
	ini.WriteBool("UseAutoCat", m_bUseAutoCat);
	// khaos::categorymod-
	// khaos::kmod+
	ini.WriteBool("SmartA4AFSwapping", m_bSmartA4AFSwapping);
	ini.WriteInt("AdvancedA4AFMode", m_iAdvancedA4AFMode);
	ini.WriteBool("ShowA4AFDebugOutput", m_bShowA4AFDebugOutput);
	ini.WriteBool("RespectMaxSources", m_bRespectMaxSources);
	ini.WriteBool("UseSaveLoadSources", m_bUseSaveLoadSources);
	// khaos::categorymod-
	// khaos::accuratetimerem+
	ini.WriteInt("TimeRemainingMode", m_iTimeRemainingMode);
	// khaos::accuratetimerem-
	//MORPH START - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	ini.WriteBool("SUCEnabled",m_bSUCEnabled);
	ini.WriteInt("SUCLog",m_bSUCLog);
	ini.WriteInt("SUCHigh",m_iSUCHigh);
	ini.WriteInt("SUCLow",m_iSUCLow);
	ini.WriteInt("SUCDrift",m_iSUCDrift);
	ini.WriteInt("SUCPitch",m_iSUCPitch);
	//MORPH END - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	ini.WriteInt("MaxConnectionsSwitchBorder",maxconnectionsswitchborder);//MORPH - Added by Yun.SF3, Auto DynUp changing

	ini.WriteBool("IsPayBackFirst",m_bPayBackFirst);//EastShare - added by AndCycle, Pay Back First
	ini.WriteBool("OnlyDownloadCompleteFiles", m_bOnlyDownloadCompleteFiles);//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	ini.WriteBool("SaveUploadQueueWaitTime", m_bSaveUploadQueueWaitTime);//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	ini.WriteBool("DateFileNameLog", DateFileNameLog);//Morph - added by AndCycle, Date File Name Log

	// #ifdef MIGHTY_SUMMERTIME
	// Mighty Knife: daylight saving patch
	ini.WriteBool("DaylightSavingPatchEnabled",m_iDaylightSavingPatch);
	// #endif

	//EastShare Start - Added by Pretender, TBH-AutoBackup
	ini.WriteBool("AutoBackup",autobackup);
	ini.WriteBool("AutoBackup2",autobackup2);
	//EastShare End - Added by Pretender, TBH-AutoBackup

	// Mighty Knife: Community visualization, Report hashing files, Log friendlist activities
	ini.WriteString("CommunityName",GetCommunityName());
	ini.WriteBool ("ReportHashingFiles",GetReportHashingFiles ());
	ini.WriteBool ("LogFriendlistActivities",GetLogFriendlistActivities ());
	// [end] Mighty Knife

	// Mighty Knife: CRC32-Tag
	ini.WriteBool ("DontAddCRC32ToFilename",GetDontAddCRCToFilename ());
	CString temp;
	// Encapsule these strings by "" because space characters are allowed at the
	// beginning/end of the prefix/suffix !
	temp.Format ("\"%s\"",GetCRC32Prefix ());
	ini.WriteString("LastCRC32Prefix",temp);
	temp.Format ("\"%s\"",GetCRC32Suffix());
	ini.WriteString("LastCRC32Suffix",temp);
	// [end] Mighty Knife

	//MORPH START - Added by SiRoB,  ZZ dynamic upload (USS)
	ini.WriteBool("DynUpEnabled", m_bDynUpEnabled);
	ini.WriteInt("DynUpPingTolerance", m_iDynUpPingTolerance);
	ini.WriteInt("DynUpGoingUpDivider", m_iDynUpGoingUpDivider);
	ini.WriteInt("DynUpGoingDownDivider", m_iDynUpGoingDownDivider);
	ini.WriteInt("DynUpNumberOfPings", m_iDynUpNumberOfPings);
	ini.WriteBool("DynUpLog", m_bDynUpLog);
	ini.WriteInt("DynUpPingLimit", m_iDynUpPingLimit); // EastShare - Add by TAHO, USS limit
	//MORPH END    - Added by SiRoB,  ZZ dynamic upload (USS)
	// ZZ:UploadSpeedSense -->
	ini.WriteBool("USSEnabled", m_bDynUpEnabled, "eMule");
	ini.WriteInt("USSPingTolerance", m_iDynUpPingTolerance, "eMule");
	ini.WriteInt("USSGoingUpDivider", m_iDynUpGoingUpDivider, "eMule");
	ini.WriteInt("USSGoingDownDivider", m_iDynUpGoingDownDivider, "eMule");
	ini.WriteInt("USSNumberOfPings", m_iDynUpNumberOfPings, "eMule");
	// ZZ:UploadSpeedSense <--
}

void CPreferences::SaveCats(){

	// Cats
	CString catinif,ixStr,buffer;
	catinif.Format("%sCategory.ini",configdir);
	remove(catinif);

		CIni catini( catinif, "Category" );
		catini.WriteInt("Count",catMap.GetCount()-1,"General");
		catini.WriteInt("CategoryVersion", 2, "General"); // khaos::categorymod+
		for (int ix=0;ix<catMap.GetCount();ix++){
			ixStr.Format("Cat#%i",ix);
			catini.WriteString("Title",catMap.GetAt(ix)->title,ixStr);
			catini.WriteString("Incoming",catMap.GetAt(ix)->incomingpath,ixStr);
			catini.WriteString("Comment",catMap.GetAt(ix)->comment,ixStr);
			buffer.Format("%lu",catMap.GetAt(ix)->color,ixStr);
			catini.WriteString("Color",buffer,ixStr);
			catini.WriteInt("Priority",catMap.GetAt(ix)->prio,ixStr);
			// khaos::kmod+ Category Advanced A4AF Mode and Auto Cat
			catini.WriteInt("AdvancedA4AFMode", catMap.GetAt(ix)->iAdvA4AFMode, ixStr);
			//catini.WriteString("AutoCatString", catMap.GetAt(ix)->autocat, ixStr);
			// khaos::kmod-
		// khaos::categorymod+ Save View Filters
		catini.WriteInt("vfFromCats", catMap.GetAt(ix)->viewfilters.nFromCats, ixStr);
		catini.WriteBool("vfVideo", catMap.GetAt(ix)->viewfilters.bVideo, ixStr);
		catini.WriteBool("vfAudio", catMap.GetAt(ix)->viewfilters.bAudio, ixStr);
		catini.WriteBool("vfArchives", catMap.GetAt(ix)->viewfilters.bArchives, ixStr);
		catini.WriteBool("vfImages", catMap.GetAt(ix)->viewfilters.bImages, ixStr);
		catini.WriteBool("vfWaiting", catMap.GetAt(ix)->viewfilters.bWaiting, ixStr);
		catini.WriteBool("vfTransferring", catMap.GetAt(ix)->viewfilters.bTransferring, ixStr);
		catini.WriteBool("vfPaused", catMap.GetAt(ix)->viewfilters.bPaused, ixStr);
		catini.WriteBool("vfStopped", catMap.GetAt(ix)->viewfilters.bStopped, ixStr);
		catini.WriteBool("vfComplete", catMap.GetAt(ix)->viewfilters.bComplete, ixStr);
		catini.WriteBool("vfHashing", catMap.GetAt(ix)->viewfilters.bHashing, ixStr);
		catini.WriteBool("vfErrorUnknown", catMap.GetAt(ix)->viewfilters.bErrorUnknown, ixStr);
		catini.WriteBool("vfCompleting", catMap.GetAt(ix)->viewfilters.bCompleting, ixStr);
		catini.WriteInt("vfFSizeMin", catMap.GetAt(ix)->viewfilters.nFSizeMin, ixStr);
		catini.WriteInt("vfFSizeMax", catMap.GetAt(ix)->viewfilters.nFSizeMax, ixStr);
		catini.WriteInt("vfRSizeMin", catMap.GetAt(ix)->viewfilters.nRSizeMin, ixStr);
		catini.WriteInt("vfRSizeMax", catMap.GetAt(ix)->viewfilters.nRSizeMax, ixStr);
		catini.WriteInt("vfTimeRemainingMin", catMap.GetAt(ix)->viewfilters.nTimeRemainingMin, ixStr);
		catini.WriteInt("vfTimeRemainingMax", catMap.GetAt(ix)->viewfilters.nTimeRemainingMax, ixStr);
		catini.WriteInt("vfSourceCountMin", catMap.GetAt(ix)->viewfilters.nSourceCountMin, ixStr);
		catini.WriteInt("vfSourceCountMax", catMap.GetAt(ix)->viewfilters.nSourceCountMax, ixStr);
		catini.WriteInt("vfAvailSourceCountMin", catMap.GetAt(ix)->viewfilters.nAvailSourceCountMin, ixStr);
		catini.WriteInt("vfAvailSourceCountMax", catMap.GetAt(ix)->viewfilters.nAvailSourceCountMax, ixStr);
		catini.WriteString("AdvancedFilterMask", catMap.GetAt(ix)->viewfilters.sAdvancedFilterMask, ixStr);
		// Save Selection Criteria
		catini.WriteBool("scFileSize", catMap.GetAt(ix)->selectioncriteria.bFileSize, ixStr);
		catini.WriteBool("scAdvancedFilterMask", catMap.GetAt(ix)->selectioncriteria.bAdvancedFilterMask, ixStr);
		// khaos::categorymod-
		}
}

void CPreferences::ResetStatsColor(int index){
	switch(index) {
		case 0 : statcolors[0]=RGB(0,0,0);break;  //MORPH - HotFix by SiRoB & IceCream, Default Black color for BackGround
		case 1 : statcolors[1]=RGB(192,192,255);break;
		case 2 : statcolors[2]=RGB(128, 255, 128);break;
		case 3 : statcolors[3]=RGB(0, 210, 0);break;
		case 4 : statcolors[4]=RGB(0, 128, 0);break;
		case 5 : statcolors[5]=RGB(255, 128, 128);break;
		case 6 : statcolors[6]=RGB(200, 0, 0);break;
		case 7 : statcolors[7]=RGB(140, 0, 0);break;
		case 8 : statcolors[8]=RGB(150, 150, 255);break;
		case 9 : statcolors[9]=RGB(255, 255, 128);break; //MORPH - Added by Yun.SF3, ZZ Upload System
		case 10 : statcolors[10]=RGB(0, 255, 0);break;
		case 11 : statcolors[11]=RGB(0, 0, 0);break; //MORPH - HotFix by SiRoB & IceCream, Default Black color for SystrayBar
		case 12 : statcolors[12]=RGB(192,   0, 192);break; //MORPH - Added by Yun.SF3, ZZ Upload System
		case 13 : statcolors[13]=RGB(0, 0, 255);break; //MORPH - Added by Yun.SF3, ZZ Upload System
		case 14 : statcolors[14]=RGB(200, 200, 0);break;

		default:break;
	}
}

void CPreferences::LoadPreferences(){
	char buffer[200];
	// -khaos--+++> Fix to stats being lost when version changes!
	int loadstatsFromOld = 0;
	// <-----khaos-

	//--- Quick hack to add version tag to preferences.ini-file and solve the issue with the FlatStatusBar tag...
	CString strFileName;
	strFileName.Format("%spreferences.ini", configdir);
	CIni* pIni = new CIni(strFileName, "eMule");

	CString strCurrVersion, strPrefsVersion;

	strCurrVersion = theApp.m_strCurVersionLong;
	strPrefsVersion = pIni->GetString("AppVersion");
	delete pIni;
	m_bFirstStart = false;

	CFileFind findFileName;

	if (strCurrVersion != strPrefsVersion){
//MORPH START - Added by IceCream, No more wizard at launch if you upgrade your Morph version to an other Morph
		if (!StrStrI(strPrefsVersion,"morph"))
			m_bFirstStart = true;
//MORPH END  - Added by IceCream, No more wizard at launch if you upgrade your Morph version to an other Morph
		// don't use this; it'll delete all read-only settings from the current pref.ini
//		if(findFileName.FindFile(strFileName)){
//			CFile file;
//			CFileFind findNewName;
//			CString strNewName;
//			strNewName.Format("%spreferences.ini.old", configdir);
//	
//			if (findNewName.FindFile(strNewName))
//				file.Remove(strNewName);
//	
//			file.Rename(strFileName, strNewName);
//			strFileName = strNewName;
//			// -khaos--+++> Set this to 2 so that LoadStats will load 'em from ini.old
//			loadstatsFromOld = 2;
//			// <-----khaos-
//		}
	}
	CIni ini(strFileName, "eMule");
	//--- end Ozon :)

#ifdef _DEBUG
	m_iDbgHeap = ini.GetInt("DebugHeap", 1);
#else
	m_iDbgHeap = 0;
#endif

	_snprintf(nick, ARRSIZE(nick), "%s", ini.GetString("Nick", DEFAULT_NICK));
	if (nick[0] == '\0' || strcmp(nick, "http://www.emule-project.net") == 0)
		_snprintf(nick, ARRSIZE(nick), "%s", DEFAULT_NICK);

	sprintf(buffer,"%sIncoming",appdir);
	sprintf(incomingdir,"%s",ini.GetString("IncomingDir",buffer ));
	MakeFoldername(incomingdir);

	sprintf(buffer,"%sTemp",appdir);
	sprintf(tempdir,"%s",ini.GetString("TempDir",buffer));
	MakeFoldername(tempdir);

	maxGraphDownloadRate=ini.GetInt("DownloadCapacity",96);
	if (maxGraphDownloadRate==0) maxGraphDownloadRate=96;
	maxGraphUploadRate=ini.GetInt("UploadCapacity",16);
	if (maxGraphUploadRate==0) maxGraphUploadRate=16;
	// ZZ:UploadSpeedSense -->
    	minupload=ini.GetInt("MinUpload", 1);
	//MORPH START - Added by SiRoB, (SUC) & (USS)
	minupload = min(max(minupload,1),maxGraphUploadRate);
	//MORPH END   - Added by SiRoB, (SUC) & (USS)
	// ZZ:UploadSpeedSense <--
	maxupload=ini.GetInt("MaxUpload",12);
	if (maxupload>maxGraphUploadRate && maxupload!=UNLIMITED) maxupload=maxGraphUploadRate*.8;
	maxdownload=ini.GetInt("MaxDownload",76);
	if (maxdownload>maxGraphDownloadRate && maxdownload!=UNLIMITED) maxdownload=maxGraphDownloadRate*.8;
	maxconnections=ini.GetInt("MaxConnections",GetRecommendedMaxConnections());
	deadserver=ini.GetInt("RemoveDeadServer",2);
	port=ini.GetInt("Port",4662);
	udpport=ini.GetInt("UDPPort",port+10);
	nServerUDPPort = ini.GetInt("ServerUDPPort", -1); // 0 = Don't use UDP port for servers, -1 = use a random port (for backward compatibility)
	maxsourceperfile=ini.GetInt("MaxSourcesPerFile",400 );
	languageID=ini.GetWORD("Language",0);
	m_iSeeShares=(EViewSharedFilesAccess)ini.GetInt("SeeShare",vsfaNobody);
	m_iToolDelayTime=ini.GetInt("ToolTipDelay",1);
	trafficOMeterInterval=ini.GetInt("StatGraphsInterval",3);
	statsInterval=ini.GetInt("statsInterval",5);
	
	deadserverretries=ini.GetInt("DeadServerRetry",1);
	m_dwServerKeepAliveTimeout=ini.GetInt("ServerKeepAliveTimeout",0);
	splitterbarPosition=ini.GetInt("SplitterbarPosition",75);
	if (splitterbarPosition>93 || splitterbarPosition<10) splitterbarPosition=75;

	statsMax=ini.GetInt("VariousStatisticsMaxValue",100);
	statsAverageMinutes=ini.GetInt("StatsAverageMinutes",5);
	MaxConperFive=ini.GetInt("MaxConnectionsPerFiveSeconds",GetDefaultMaxConperFive());

	reconnect=ini.GetBool("Reconnect",true);
	scorsystem=ini.GetBool("Scoresystem",true);
	ICH=ini.GetBool("ICH",true);
	autoserverlist=ini.GetBool("Serverlist",false);
	updatenotify=ini.GetBool("UpdateNotifyTestClient",true);
	mintotray=ini.GetBool("MinToTray",false);
	addserversfromserver=ini.GetBool("AddServersFromServer",true);
	addserversfromclient=ini.GetBool("AddServersFromClient",true);
	splashscreen=ini.GetBool("Splashscreen",true);
	bringtoforeground=ini.GetBool("BringToFront",true);
	transferDoubleclick=ini.GetBool("TransferDoubleClick",true);
	beepOnError=ini.GetBool("BeepOnError",true);
	confirmExit=ini.GetBool("ConfirmExit",false);
	filterLANIPs=ini.GetBool("FilterBadIPs",true);
	m_bAllocLocalHostIP=ini.GetBool("AllowLocalHostIP",false);
	autoconnect=ini.GetBool("Autoconnect",false);
	showRatesInTitle=ini.GetBool("ShowRatesOnTitle",false);

	onlineSig=ini.GetBool("OnlineSignature",false);
	startMinimized=ini.GetBool("StartupMinimized",false);
	m_bRestoreLastMainWndDlg=ini.GetBool("RestoreLastMainWndDlg",false);
	m_iLastMainWndDlgID=ini.GetInt("LastMainWndDlgID",0);
	m_bRestoreLastLogPane=ini.GetBool("RestoreLastLogPane",false);
	m_iLastLogPaneID=ini.GetInt("LastLogPaneID",0);
	safeServerConnect =ini.GetBool("SafeServerConnect",false);

	m_bTransflstRemain =ini.GetBool("TransflstRemainOrder",false);
	filterserverbyip=ini.GetBool("FilterServersByIP",false);
	filterlevel=ini.GetInt("FilterLevel",127);
	checkDiskspace=ini.GetBool("CheckDiskspace",false);	// SLUGFILLER: checkDiskspace
	m_uMinFreeDiskSpace=ini.GetInt("MinFreeDiskSpace",20*1024*1024);
	sprintf(yourHostname,"%s",ini.GetString("YourHostname",""));	// itsonlyme: hostnameSource

	// Barry - New properties...
	autoconnectstaticonly = ini.GetBool("AutoConnectStaticOnly",false); 
	autotakeed2klinks = ini.GetBool("AutoTakeED2KLinks",true); 
	addnewfilespaused = ini.GetBool("AddNewFilesPaused",false); 
	depth3D = ini.GetInt("3DDepth", 0);

	// as temporarial converter for previous versions
	if (strPrefsVersion < "0.25a") // before 0.25a
		if (ini.GetBool("FlatStatusBar",false))
			depth3D = 0;
		else 
			depth3D = 5;

    useDownloadNotifier=ini.GetBool("NotifyOnDownload",false);	// Added by enkeyDEV
	useNewDownloadNotifier=ini.GetBool("NotifyOnNewDownload",false);
    useChatNotifier=ini.GetBool("NotifyOnChat",false);
    useLogNotifier=ini.GetBool("NotifyOnLog",false);
    useSoundInNotifier=ini.GetBool("NotifierUseSound",false);
	notifierPopsEveryChatMsg=ini.GetBool("NotifierPopEveryChatMessage",false);
	notifierNewVersion=ini.GetBool("NotifierPopNewVersion",false);
	notifierImportantError=ini.GetBool("NotifyOnImportantError",false);
	sprintf(notifierSoundFilePath,"%s",ini.GetString("NotifierSoundPath",""));
	sprintf(notifierConfiguration,"%s",ini.GetString("NotifierConfiguration","")); // Added by enkeyDEV
	sprintf(datetimeformat,"%s",ini.GetString("DateTimeFormat","%A, %x, %X"));
	if (strlen(datetimeformat)==0) strcpy(datetimeformat,"%A, %x, %X");

	sprintf(datetimeformat4log,"%s",ini.GetString("DateTimeFormat4Log","%c"));
	if (strlen(datetimeformat4log)==0) strcpy(datetimeformat4log,"%c");

	sprintf(m_sircserver,"%s",ini.GetString("DefaultIRCServer","irc.emule-project.net"));
	sprintf(m_sircnick,"%s",ini.GetString("IRCNick","eMule"));
	m_bircaddtimestamp=ini.GetBool("IRCAddTimestamp",true);
	sprintf(m_sircchannamefilter,"%s",ini.GetString("IRCFilterName", "" ));
	m_bircusechanfilter=ini.GetBool("IRCUseFilter", false);
	m_iircchanneluserfilter=ini.GetInt("IRCFilterUser", 0);
	sprintf(m_sircperformstring,"%s",ini.GetString("IRCPerformString", "" ));
	m_bircuseperform=ini.GetBool("IRCUsePerform", false);
	m_birclistonconnect=ini.GetBool("IRCListOnConnect", true);
	m_bircacceptlinks=ini.GetBool("IRCAcceptLink", true);
	m_bircacceptlinksfriends=ini.GetBool("IRCAcceptLinkFriends", true);
	m_bircignoreinfomessage=ini.GetBool("IRCIgnoreInfoMessage", false);
	m_bircignoreemuleprotoinfomessage=ini.GetBool("IRCIgnoreEmuleProtoInfoMessage", true);
	m_birchelpchannel=ini.GetBool("IRCHelpChannel",true);
	smartidcheck=ini.GetBool("SmartIdCheck",true);
	m_bVerbose=ini.GetBool("Verbose",false);
	m_bDebugSourceExchange=ini.GetBool("DebugSourceExchange",false);
	m_bLogBannedClients=ini.GetBool("LogBannedClients", true);
	m_bLogRatingDescReceived=ini.GetBool("LogRatingDescReceived",true);
	m_bLogSecureIdent=ini.GetBool("LogSecureIdent",true);
	m_bLogFilteredIPs=ini.GetBool("LogFilteredIPs",true);
	m_bLogFileSaving=ini.GetBool("LogFileSaving",false);
#if defined(_DEBUG) || defined(USE_DEBUG_DEVICE)
	// following options are for debugging or when using an external debug device viewer only.
	m_iDebugServerTCPLevel=ini.GetInt("DebugServerTCP",0);
	m_iDebugServerUDPLevel=ini.GetInt("DebugServerUDP",0);
	m_iDebugServerSourcesLevel=ini.GetInt("DebugServerSources",0);
	m_iDebugServerSearchesLevel=ini.GetInt("DebugServerSearches",0);
	m_iDebugClientTCPLevel=ini.GetInt("DebugClientTCP",0);
	m_iDebugClientUDPLevel=ini.GetInt("DebugClientUDP",0);
	m_iDebugClientKadUDPLevel=ini.GetInt("DebugClientKadUDP",0);
#else
	// for normal release builds ensure that those options are all turned off
	m_iDebugServerTCPLevel=0;
	m_iDebugServerUDPLevel=0;
	m_iDebugServerSourcesLevel=0;
	m_iDebugServerSearchesLevel=0;
	m_iDebugClientTCPLevel=0;
	m_iDebugClientUDPLevel=0;
	m_iDebugClientKadUDPLevel=0;
#endif
	m_bpreviewprio=ini.GetBool("PreviewPrio",false);
	m_bupdatequeuelist=ini.GetBool("UpdateQueueListPref",false);
	m_bmanualhighprio=ini.GetBool("ManualHighPrio",false);
	m_btransferfullchunks=ini.GetBool("FullChunkTransfers",true);
	m_bstartnextfile=ini.GetBool("StartNextFile",false);
	m_bshowoverhead=ini.GetBool("ShowOverhead",false);
	moviePreviewBackup=ini.GetBool("VideoPreviewBackupped",true);
	m_iPreviewSmallBlocks=ini.GetInt("PreviewSmallBlocks", 0);

	// read file buffer size (with backward compatibility)
	m_iFileBufferSize=ini.GetInt("FileBufferSizePref",0); // old setting
	if (m_iFileBufferSize == 0)
		m_iFileBufferSize = 256*1024;
	else
		m_iFileBufferSize = ((m_iFileBufferSize*15000 + 512)/1024)*1024;
	m_iFileBufferSize=ini.GetInt("FileBufferSize",m_iFileBufferSize);

	// read queue size (with backward compatibility)
	m_iQueueSize=ini.GetInt("QueueSizePref",0); // old setting
	if (m_iQueueSize == 0)
		m_iQueueSize = 50*100;
	else
		m_iQueueSize = m_iQueueSize*100;
	m_iQueueSize=ini.GetInt("QueueSize",m_iQueueSize);
	
	m_iCommitFiles=ini.GetInt("CommitFiles", 1); // 1 = "commit" on application shut down; 2 = "commit" on each file saveing
	versioncheckdays=ini.GetInt("Check4NewVersionDelay",5);
	m_bDAP=ini.GetBool("DAPPref",true);
	m_bUAP=ini.GetBool("UAPPref",true);
	indicateratings=ini.GetBool("IndicateRatings",true);
	// khaos::kmod+ Obsolete prefs->allcatType=ini.GetInt("AllcatType",0);
	watchclipboard=ini.GetBool("WatchClipboard4ED2kFilelinks",false);
	m_iSearchMethod=ini.GetInt("SearchMethod",0);
	
	log2disk=ini.GetBool("SaveLogToDisk",false);
	debug2disk=ini.GetBool("SaveDebugToDisk",false);
	uMaxLogFileSize = ini.GetInt("MaxLogFileSize", 1024*1024);
	iMaxLogBuff = ini.GetInt("MaxLogBuff",64) * 1024;
	showCatTabInfos=ini.GetBool("ShowInfoOnCatTabs",false);
	resumeSameCat=ini.GetBool("ResumeNextFromSameCat",false);
	dontRecreateGraphs =ini.GetBool("DontRecreateStatGraphsOnResize",false);
	m_bExtControls =ini.GetBool("ShowExtControls",false);
	
	versioncheckLastAutomatic=ini.GetInt("VersionCheckLastAutomatic",0);
	m_bDisableKnownClientList=ini.GetInt("DisableKnownClientList",false);
	m_bDisableQueueList=ini.GetInt("DisableQueueList",false);
	m_bCreditSystem=ini.GetInt("UseCreditSystem",true);
	m_bCreditSystem=true; //MORPH - Changed by SiRoB, CreditSystem allways used
	scheduler=ini.GetBool("EnableScheduler",false);
	msgonlyfriends=ini.GetBool("MessagesFromFriendsOnly",false);
	msgsecure=ini.GetBool("MessageFromValidSourcesOnly",true);
	autofilenamecleanup=ini.GetBool("AutoFilenameCleanup",false);
	m_bUseAutocompl=ini.GetBool("UseAutocompletion",true);
	m_bShowDwlPercentage=ini.GetBool("ShowDwlPercentage",false);
	networkkademlia=ini.GetBool("NetworkKademlia",false);
	networked2k=ini.GetBool("NetworkED2K",true);
	m_bRemove2bin=ini.GetBool("RemoveFilesToBin",true);

	m_iMaxChatHistory=ini.GetInt("MaxChatHistoryLines",100);
	if (m_iMaxChatHistory<1) m_iMaxChatHistory=100;

	maxmsgsessions=ini.GetInt("MaxMessageSessions",50);

	sprintf(TxtEditor,"%s",ini.GetString("TxtEditor","notepad.exe"));
	sprintf(VideoPlayer,"%s",ini.GetString("VideoPlayer",""));
	
	sprintf(m_sTemplateFile,"%s",ini.GetString("WebTemplateFile","eMule.tmpl"));

	sprintf(messageFilter,"%s",ini.GetString("MessageFilter","Your client has an infinite queue"));
	sprintf(commentFilter,"%s",ini.GetString("CommentFilter","http://|www."));
	sprintf(filenameCleanups,"%s",ini.GetString("FilenameCleanups","http|www.|.com|shared|powered|sponsored|sharelive|filedonkey|saugstube|eselfilme|eseldownloads|emulemovies|spanishare|eselpsychos.de|saughilfe.de|goldesel.6x.to|freedivx.org|elitedivx|deviance|-ftv|ftv|-flt|flt"));
	m_iExtractMetaData=ini.GetInt("ExtractMetaData",2); // 0=disable, 1=mp3+avi, 2=MediaDet
		
	m_bUseSecureIdent=ini.GetBool("SecureIdent",true);
	m_bAdvancedSpamfilter=ini.GetBool("AdvancedSpamFilter",true);
	m_bRemoveFinishedDownloads=ini.GetBool("AutoClearCompleted",false);

	// Toolbar
	sprintf(m_sToolbarSettings,"%s", ini.GetString("ToolbarSetting", strDefaultToolbar));
	sprintf(m_sToolbarBitmap,"%s", ini.GetString("ToolbarBitmap", ""));
	sprintf(m_sToolbarBitmapFolder,"%s", ini.GetString("ToolbarBitmapFolder", incomingdir));
	m_nToolbarLabels = ini.GetInt("ToolbarLabels",1);
	m_iStraightWindowStyles=ini.GetInt("StraightWindowStyles",0);
	_sntprintf(m_szSkinProfile, ARRSIZE(m_szSkinProfile), "%s", ini.GetString(_T("SkinProfile"), _T("")));
	_sntprintf(m_szSkinProfileDir, ARRSIZE(m_szSkinProfileDir), "%s", ini.GetString(_T("SkinProfileDir"), _T("")));

	//MORPH START added by Yun.SF3: Ipfilter.dat update
	m_IPfilterVersion=ini.GetInt("IPfilterVersion",0); //added by milobac: Ipfilter.dat update
	AutoUpdateIPFilter=ini.GetBool("AutoUPdateIPFilter",false); //added by milobac: Ipfilter.dat update
	//MORPH END added by Yun.SF3: Ipfilter.dat update

	//EastShare - added by AndCycle, IP to Country
	m_iIP2CountryNameMode = (IP2CountryNameSelection)ini.GetInt("IP2Country", IP2CountryName_DISABLE); 
	m_bIP2CountryShowFlag = ini.GetBool("IP2CountryShowFlag", false);
	//EastShare - added by AndCycle, IP to Country

	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	LowIdRetries=ini.GetInt("ReconnectOnLowIdRetries",3);	// SLUGFILLER: lowIdRetry
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry
	//MORPH START - Added by SiRoB, SLUGFILLER: hideOS
	hideOS=ini.GetInt("HideOvershares",0/*5*/);
	selectiveShare=ini.GetBool("SelectiveShare",false);
	//MORPH END   - Added by SiRoB, SLUGFILLER: hideOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	ShareOnlyTheNeed=ini.GetInt("ShareOnlyTheNeed",0);
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
	//MORPH START - Added by SiRoB, Show Permissions
	permissions=ini.GetInt("ShowSharePermissions",0);
	//MORPH END   - Added by SiRoB, Show Permissions
	//EastShare - Added by Pretender, TBH-AutoBackup
	autobackup = ini.GetBool("AutoBackup",true);
	autobackup2 = ini.GetBool("AutoBackup2",true);
	//EastShare - Added by Pretender, TBH-AutoBackup

	infiniteQueue=ini.GetBool("InfiniteQueue",false);	//Morph - added by AndCycle, SLUGFILLER: infiniteQueue
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	m_bisautopowersharenewdownloadfile=ini.GetBool("AutoPowershareNewDownloadFile",true);
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	m_FakesDatVersion=ini.GetInt("FakesDatVersion",0);
	UpdateFakeStartup=ini.GetBool("UpdateFakeStartup",false);
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating
	//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	m_bSUCEnabled = ini.GetBool("SUCEnabled",false);
	m_bSUCLog =  ini.GetBool("SUCLog",false);
	m_iSUCHigh = ini.GetInt("SUCHigh",900);
	m_iSUCHigh = min(max(prefs->m_iSUCHigh,350),1000);
	m_iSUCLow = ini.GetInt("SUCLow",600);
	m_iSUCLow = min(max(prefs->m_iSUCLow,350),prefs->m_iSUCHigh);
	m_iSUCPitch = ini.GetInt("SUCPitch",3000);
	m_iSUCPitch = min(max(prefs->m_iSUCPitch,2500),10000);
	m_iSUCDrift = ini.GetInt("SUCDrift",50);
	m_iSUCDrift = min(max(prefs->m_iSUCDrift,0),100);
	//MORPH END - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	maxconnectionsswitchborder = ini.GetInt("MaxConnectionsSwitchBorder",100);//MORPH - Added by Yun.SF3, Auto DynUp changing
	maxconnectionsswitchborder = min(max(prefs->maxconnectionsswitchborder,50),60000);//MORPH - Added by Yun.SF3, Auto DynUp changing

	// #ifdef MIGHTY_SUMMERTIME
	// Mighty Knife: daylight saving patch
	m_iDaylightSavingPatch = ini.GetBool("DaylightSavingPatchEnabled",false);
	// #endif

	//EastShare Start - PreferShareAll by AndCycle
	shareall=ini.GetBool("ShareAll",true);	// SLUGFILLER: preferShareAll
	//EastShare END - PreferShareAll by AndCycle
	// EastShare START - Added by TAHO, .met file control
	m_iKnownMetDays = ini.GetInt("KnownMetDays", 0);
	// EastShare END - Added by TAHO, .met file control
	isautodynupswitching=ini.GetBool("AutoDynUpSwitching",false);
	DateFileNameLog=ini.GetBool("DateFileNameLog", true);//Morph - added by AndCycle, Date File Name Log
	m_bPayBackFirst=ini.GetBool("IsPayBackFirst",false);//EastShare - added by AndCycle, Pay Back First
	m_bOnlyDownloadCompleteFiles = ini.GetBool("OnlyDownloadCompleteFiles", false);//EastShare - Added by AndCycle, Only download complete files v2.1 (shadow)
	m_bSaveUploadQueueWaitTime = ini.GetBool("SaveUploadQueueWaitTime", true);//Morph - added by AndCycle, Save Upload Queue Wait Time (MSUQWT)
	sprintf(UpdateURLFakeList,"%s",ini.GetString("UpdateURLFakeList","http://www.emuleitor.com/downloads/Morph/fakes.txt"));		//MORPH START - Added by milobac and Yun.SF3, FakeCheck, FakeReport, Auto-updating
	sprintf(UpdateURLIPFilter,"%s",ini.GetString("UpdateURLIPFilter","http://www.emuleitor.com/downloads/Morph/ipfilter.txt"));//MORPH START added by Yun.SF3: Ipfilter.dat update

	// khaos::categorymod+ Load Preferences
	m_bShowCatNames=ini.GetBool("ShowCatName",true);
	m_bValidSrcsOnly=ini.GetBool("ValidSrcsOnly", false);
	m_bActiveCatDefault=ini.GetBool("ActiveCatDefault", true);
	m_bSelCatOnAdd=ini.GetBool("SelCatOnAdd", true);
	m_bAutoSetResumeOrder=ini.GetBool("AutoSetResumeOrder", true);
	m_bSmallFileDLPush=ini.GetBool("SmallFileDLPush", true);
	m_iStartDLInEmptyCats=ini.GetInt("StartDLInEmptyCats", 0);
	m_bUseAutoCat=ini.GetBool("UseAutoCat", true);
	// khaos::categorymod-
	// khaos::kmod+
	m_bUseSaveLoadSources=ini.GetBool("UseSaveLoadSources", true);
	m_bRespectMaxSources=ini.GetBool("RespectMaxSources", true);
	m_bSmartA4AFSwapping=ini.GetBool("SmartA4AFSwapping", true);
	m_iAdvancedA4AFMode=ini.GetInt("AdvancedA4AFMode", 1);
	m_bShowA4AFDebugOutput=ini.GetBool("ShowA4AFDebugOutput", false);
	// khaos::accuratetimerem+
	m_iTimeRemainingMode=ini.GetInt("TimeRemainingMode", 0);
	// khaos::accuratetimerem-

	//if (maxGraphDownloadRate<maxdownload) maxdownload=UNLIMITED;
	//if (maxGraphUploadRate<maxupload) maxupload=UNLIMITED;

	ini.SerGet(true, downloadColumnWidths,
		ARRSIZE(downloadColumnWidths), "DownloadColumnWidths");
	ini.SerGet(true, downloadColumnHidden,
		ARRSIZE(downloadColumnHidden), "DownloadColumnHidden");
	ini.SerGet(true, downloadColumnOrder,
		ARRSIZE(downloadColumnOrder), "DownloadColumnOrder");
	ini.SerGet(true, uploadColumnWidths,
		ARRSIZE(uploadColumnWidths), "UploadColumnWidths");
	ini.SerGet(true, uploadColumnHidden,
		ARRSIZE(uploadColumnHidden), "UploadColumnHidden");
	ini.SerGet(true, uploadColumnOrder,
		ARRSIZE(uploadColumnOrder), "UploadColumnOrder");
	ini.SerGet(true, queueColumnWidths,
		ARRSIZE(queueColumnWidths), "QueueColumnWidths");
	ini.SerGet(true, queueColumnHidden,
		ARRSIZE(queueColumnHidden), "QueueColumnHidden");
	ini.SerGet(true, queueColumnOrder,
		ARRSIZE(queueColumnOrder), "QueueColumnOrder");
	ini.SerGet(true, searchColumnWidths,
		ARRSIZE(searchColumnWidths), "SearchColumnWidths");
	ini.SerGet(true, searchColumnHidden,
		ARRSIZE(searchColumnHidden), "SearchColumnHidden");
	ini.SerGet(true, searchColumnOrder,
		ARRSIZE(searchColumnOrder), "SearchColumnOrder");
	ini.SerGet(true, sharedColumnWidths,
		ARRSIZE(sharedColumnWidths), "SharedColumnWidths");
	ini.SerGet(true, sharedColumnHidden,
		ARRSIZE(sharedColumnHidden), "SharedColumnHidden");
	ini.SerGet(true, sharedColumnOrder,
		ARRSIZE(sharedColumnOrder), "SharedColumnOrder");
	ini.SerGet(true, serverColumnWidths,
		ARRSIZE(serverColumnWidths), "ServerColumnWidths");
	ini.SerGet(true, serverColumnHidden,
		ARRSIZE(serverColumnHidden), "ServerColumnHidden");
	ini.SerGet(true, serverColumnOrder,
		ARRSIZE(serverColumnOrder), "ServerColumnOrder");
	ini.SerGet(true, clientListColumnWidths,
		ARRSIZE(clientListColumnWidths), "ClientListColumnWidths");
	ini.SerGet(true, clientListColumnHidden,
		ARRSIZE(clientListColumnHidden), "ClientListColumnHidden");
	ini.SerGet(true, clientListColumnOrder,
		ARRSIZE(clientListColumnOrder), "ClientListColumnOrder");

	ini.SerGet(true, FilenamesListColumnWidths,
		ARRSIZE(FilenamesListColumnWidths), "FilenamesListColumnWidths");
	ini.SerGet(true, FilenamesListColumnHidden,
		ARRSIZE(FilenamesListColumnHidden), "FilenamesListColumnHidden");
	ini.SerGet(true, FilenamesListColumnOrder,
		ARRSIZE(FilenamesListColumnOrder), "FilenamesListColumnOrder");

	// Barry - Provide a mechanism for all tables to store/retrieve sort order
	// SLUGFILLER: multiSort - load multiple params
	ini.SerGet(true, tableSortItemDownload,
		ARRSIZE(tableSortItemDownload), "TableSortItemDownload", NULL, -1);
	ini.SerGet(true, tableSortItemUpload,
		ARRSIZE(tableSortItemUpload), "TableSortItemUpload", NULL, -1);
	ini.SerGet(true, tableSortItemQueue,
		ARRSIZE(tableSortItemQueue), "TableSortItemQueue", NULL, -1);
	ini.SerGet(true, tableSortItemSearch,
		ARRSIZE(tableSortItemSearch), "TableSortItemSearch", NULL, -1);
	ini.SerGet(true, tableSortItemShared,
		ARRSIZE(tableSortItemShared), "TableSortItemShared", NULL, -1);
	ini.SerGet(true, tableSortItemServer,
		ARRSIZE(tableSortItemServer), "TableSortItemServer", NULL, -1);
	ini.SerGet(true, tableSortItemClientList,
		ARRSIZE(tableSortItemClientList), "TableSortItemClientList", NULL, -1);
	ini.SerGet(true, tableSortItemFilenames,
		ARRSIZE(tableSortItemFilenames), "TableSortItemFilenames", NULL, -1);
	ini.SerGet(true, tableSortAscendingDownload,
		ARRSIZE(tableSortAscendingDownload), "TableSortAscendingDownload");
	ini.SerGet(true, tableSortAscendingUpload,
		ARRSIZE(tableSortAscendingUpload), "TableSortAscendingUpload");
	ini.SerGet(true, tableSortAscendingQueue,
		ARRSIZE(tableSortAscendingQueue), "TableSortAscendingQueue");
	ini.SerGet(true, tableSortAscendingSearch,
		ARRSIZE(tableSortAscendingSearch), "TableSortAscendingSearch");
	ini.SerGet(true, tableSortAscendingShared,
		ARRSIZE(tableSortAscendingShared), "TableSortAscendingShared");
	ini.SerGet(true, tableSortAscendingServer,
		ARRSIZE(tableSortAscendingServer), "TableSortAscendingServer");
	ini.SerGet(true, tableSortAscendingClientList,
		ARRSIZE(tableSortAscendingClientList), "TableSortAscendingClientList");
	ini.SerGet(true, tableSortAscendingFilenames,
		ARRSIZE(tableSortAscendingFilenames), "TableSortAscendingFilenames");
	// topmost must be valid
	if (tableSortItemDownload[0] == -1) {
		tableSortItemDownload[0] = 0;
		tableSortAscendingDownload[0] = true;
	}
	if (tableSortItemUpload[0] == -1) {
		tableSortItemUpload[0] = 0;
		tableSortAscendingUpload[0] = true;
	}
	if (tableSortItemQueue[0] == -1) {
		tableSortItemQueue[0] = 0;
		tableSortAscendingQueue[0] = true;
	}
	if (tableSortItemSearch[0] == -1) {
		tableSortItemSearch[0] = 0;
		tableSortAscendingSearch[0] = true;
	}
	if (tableSortItemShared[0] == -1) {
		tableSortItemShared[0] = 0;
		tableSortAscendingShared[0] = true;
	}
	if (tableSortItemServer[0] == -1) {
		tableSortItemServer[0] = 0;
		tableSortAscendingServer[0] = true;
	}
	if (tableSortItemClientList[0] == -1) {
		tableSortItemClientList[0] = 0;
		tableSortAscendingClientList[0] = true;
	}
	if (tableSortItemFilenames[0] == -1) {
		tableSortItemFilenames[0] = 0;
		tableSortAscendingFilenames[0] = true;
	}
	// SLUGFILLER: multiSort

	LPBYTE pData = NULL;
	UINT uSize = sizeof m_lfHyperText;
	if (ini.GetBinary("HyperTextFont", &pData, &uSize) && uSize == sizeof m_lfHyperText)
		memcpy(&m_lfHyperText, pData, sizeof m_lfHyperText);
	else
		memset(&m_lfHyperText, 0, sizeof m_lfHyperText);
	delete[] pData;

	if (statsAverageMinutes<1) statsAverageMinutes=5;

	// deadlake PROXYSUPPORT
	proxy.EnablePassword = ini.GetBool("ProxyEnablePassword",false,"Proxy");
	proxy.UseProxy = ini.GetBool("ProxyEnableProxy",false,"Proxy");
	sprintf(buffer,"");
	sprintf(proxy.name,"%s",ini.GetString("ProxyName",buffer,"Proxy"));
	sprintf(proxy.password,"%s",ini.GetString("ProxyPassword",buffer,"Proxy"));
	sprintf(proxy.user,"%s",ini.GetString("ProxyUser",buffer,"Proxy"));
	proxy.port = ini.GetInt("ProxyPort",1080,"Proxy");
	proxy.type = ini.GetInt("ProxyType",PROXYTYPE_NOPROXY,"Proxy");
	m_bIsASCWOP = ini.GetBool("ConnectWithoutProxy",false,"Proxy");
	m_bShowProxyErrors = ini.GetBool("ShowErrors",false,"Proxy");

	CString buffer2;
	for (int i=0;i<ARRSIZE(statcolors);i++) {
		buffer2.Format("StatColor%i",i);
		sprintf(buffer,"%s",ini.GetString(buffer2,"0","Statistics"));
		statcolors[i] = 0;
		if (sscanf(buffer, "%i", &statcolors[i]) != 1 || statcolors[i] == 0)
			ResetStatsColor(i);
	}

	// -khaos--+++> Load Stats
	// I changed this to a seperate function because it is now also used
	// to load the stats backup and to load stats from preferences.ini.old.
	LoadStats(loadstatsFromOld);
	// <-----khaos-

	// Web Server
	sprintf(m_sWebPassword,"%s",ini.GetString("Password", "","WebServer"));
	sprintf(m_sWebLowPassword,"%s",ini.GetString("PasswordLow", ""));
	m_nWebPort=ini.GetInt("Port", 4711);
	m_bWebEnabled=ini.GetBool("Enabled", false);
	m_bWebUseGzip=ini.GetBool("UseGzip", true);
	m_bWebLowEnabled=ini.GetBool("UseLowRightsUser", false);
	m_nWebPageRefresh=ini.GetInt("PageRefreshTime", 120);

	dontcompressavi=ini.GetBool("DontCompressAvi",false);

	// mobilemule
	sprintf(m_sMMPassword,"%s",ini.GetString("Password", "","MobileMule"));
	m_bMMEnabled = ini.GetBool("Enabled", false);
	m_nMMPort = ini.GetInt("Port", 80);

/*
	// ZZ:UploadSpeedSense -->
    m_bDynUpEnabled = ini.GetBool("USSEnabled", false, "eMule");
    m_iDynUpPingTolerance = ini.GetInt("USSPingTolerance", 800, "eMule");
    m_iDynUpGoingUpDivider = ini.GetInt("USSGoingUpDivider", 1000, "eMule");
    m_iDynUpGoingDownDivider = ini.GetInt("USSGoingDownDivider", 1000, "eMule");
    m_iDynUpNumberOfPings = ini.GetInt("USSNumberOfPings", 1, "eMule");
	// ZZ:UploadSpeedSense <--
*/
	// Mighty Knife: Community visualization, Report hashing files, Log friendlist activities
	sprintf (m_sCommunityName,"%s",ini.GetString ("CommunityName"));
	m_bReportHashingFiles = ini.GetBool ("ReportHashingFiles",true);
	m_bLogFriendlistActivities = ini.GetBool ("LogFriendlistActivities",true);
	// [end] Mighty Knife

	// Mighty Knife: CRC32-Tag
	SetDontAddCRCToFilename (ini.GetBool ("DontAddCRC32ToFilename",false));
	// From the prefix/suffix delete the leading/trailing "".
	SetCRC32Prefix (ini.GetString("LastCRC32Prefix","\" [\"").Trim ("\""));
	SetCRC32Suffix (ini.GetString("LastCRC32Suffix","\"]\"").Trim ("\""));
	// [end] Mighty Knife

	//MORPH START - Added by SiRoB,  ZZ dynamic upload (USS)
	if (!m_bSUCEnabled) bDynUpEnabled = ini.GetBool("DynUpEnabled", false);

	m_iDynUpPingTolerance = ini.GetInt("DynUpPingTolerance", 800);
	m_iDynUpGoingUpDivider = ini.GetInt("DynUpGoingUpDivider", 1000);
	m_iDynUpGoingDownDivider = ini.GetInt("DynUpGoingDownDivider", 1000);
	m_iDynUpNumberOfPings = ini.GetInt("DynUpNumberOfPings", 1);
	m_bDynUpLog = ini.GetBool("DynUpLog", false);
	m_iDynUpPingLimit = ini.GetInt("DynUpPingLimit", 200); // EastShare - Added by TAHO, USS limit
	//MORPH END   - Added by SiRoB,  ZZ dynamic upload (USS)

    LoadCats();
	//MORPH - Khaos Obsolete //if (GetCatCount()==1) SetAllcatType(0);

	SetLanguage();
	if (loadstatsFromOld == 2) SavePreferences();
}

// khaos::categorymod+
void CPreferences::LoadCats() {
	CString ixStr,catinif;//,cat_a,cat_b,cat_c;
	//char buffer[100];

	catinif.Format("%sCategory.ini", configdir);
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
		catini.SetSection("General");
		if (catini.GetInt("CategoryVersion") == 0)
			bCreateDefault = true;
	}

	if (bCreateDefault)
	{
		Category_Struct* defcat=new Category_Struct;

		sprintf(defcat->title,"Default");
		defcat->prio=0;
		defcat->iAdvA4AFMode = 0;
		sprintf(defcat->incomingpath, incomingdir);
		sprintf(defcat->comment, "The default category.  It can't be merged or deleted.");
		defcat->color = 0;
		defcat->viewfilters.bArchives = true;
		defcat->viewfilters.bAudio = true;
		defcat->viewfilters.bComplete = true;
		defcat->viewfilters.bCompleting = true;
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
		AddCat(defcat);
		if (bSkipLoad)
		{
			SaveCats();
			return;
		}
	}

	int max = catini.GetInt("Count", 0, "General");

	for (int ix = bCreateDefault ? 1 : 0; ix <= max; ix++)
	{
		ixStr.Format("Cat#%i",ix);
        	catini.SetSection(ixStr);

		Category_Struct* newcat = new Category_Struct;

		sprintf(newcat->title,"%s",catini.GetString("Title", ix == 0 ? "Default" : "?"));
		sprintf(newcat->incomingpath, catini.GetString("Incoming", ix == 0 ? CString(incomingdir) : ""));
		MakeFoldername(newcat->incomingpath);
		sprintf(newcat->comment, catini.GetString("Comment", ""));
		newcat->prio = catini.GetInt("Priority", 0);
		CString sBuff = catini.GetString("Color", "0");
		newcat->color = _atoi64(sBuff.GetBuffer());
		// khaos::kmod+ Category Advanced A4AF Mode
		newcat->iAdvA4AFMode = catini.GetInt("AdvancedA4AFMode", 0);
		//newcat->autocat = catini.GetString("AutoCatString","",ixStr);
		// khaos::kmod-
		// Load View Filters
		newcat->viewfilters.nFromCats = catini.GetInt("vfFromCats", ix==0?0:2);
		newcat->viewfilters.bSuspendFilters = false;
		newcat->viewfilters.bVideo = catini.GetBool("vfVideo", true);
		newcat->viewfilters.bAudio = catini.GetBool("vfAudio", true);
		newcat->viewfilters.bArchives = catini.GetBool("vfArchives", true);
		newcat->viewfilters.bImages = catini.GetBool("vfImages", true);
		newcat->viewfilters.bWaiting = catini.GetBool("vfWaiting", true);
		newcat->viewfilters.bTransferring = catini.GetBool("vfTransferring", true);
		newcat->viewfilters.bPaused = catini.GetBool("vfPaused", true);
		newcat->viewfilters.bStopped = catini.GetBool("vfStopped", true);
		newcat->viewfilters.bComplete = catini.GetBool("vfComplete", true);
		newcat->viewfilters.bHashing = catini.GetBool("vfHashing", true);
		newcat->viewfilters.bErrorUnknown = catini.GetBool("vfErrorUnknown", true);
		newcat->viewfilters.bCompleting = catini.GetBool("vfCompleting", true);
		newcat->viewfilters.nFSizeMin = catini.GetInt("vfFSizeMin", 0);
		newcat->viewfilters.nFSizeMax = catini.GetInt("vfFSizeMax", 0);
		newcat->viewfilters.nRSizeMin = catini.GetInt("vfRSizeMin", 0);
		newcat->viewfilters.nRSizeMax = catini.GetInt("vfRSizeMax", 0);
		newcat->viewfilters.nTimeRemainingMin = catini.GetInt("vfTimeRemainingMin", 0);
		newcat->viewfilters.nTimeRemainingMax = catini.GetInt("vfTimeRemainingMax", 0);
		newcat->viewfilters.nSourceCountMin = catini.GetInt("vfSourceCountMin", 0);
		newcat->viewfilters.nSourceCountMax = catini.GetInt("vfSourceCountMax", 0);
		newcat->viewfilters.nAvailSourceCountMin = catini.GetInt("vfAvailSourceCountMin", 0);
		newcat->viewfilters.nAvailSourceCountMax = catini.GetInt("vfAvailSourceCountMax", 0);
		newcat->viewfilters.sAdvancedFilterMask = catini.GetString("vfAdvancedFilterMask", "");
		// Load Selection Criteria
		newcat->selectioncriteria.bFileSize = catini.GetBool("scFileSize", true);
		newcat->selectioncriteria.bAdvancedFilterMask = catini.GetBool("scAdvancedFilterMask", true);

		AddCat(newcat);
		if (!PathFileExists(newcat->incomingpath)) ::CreateDirectory(newcat->incomingpath, 0);
	}
}
// khaos::categorymod-

WORD CPreferences::GetWindowsVersion(){
	static bool bWinVerAlreadyDetected = false;
	if(!bWinVerAlreadyDetected)
	{	
		bWinVerAlreadyDetected = true;
		m_wWinVer = DetectWinVersion();	
	}	
	return m_wWinVer;
}

uint16 CPreferences::GetDefaultMaxConperFive(){
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

// Barry - Provide a mechanism for all tables to store/retrieve sort order
int CPreferences::GetColumnSortItem(Table t, int column) const	// SLUGFILLER: multiSort
{
	switch(t) 
	{
		case tableDownload:
			return tableSortItemDownload[column];	// SLUGFILLER: multiSort
		case tableUpload:
			return tableSortItemUpload[column];	// SLUGFILLER: multiSort
		case tableQueue:
			return tableSortItemQueue[column];	// SLUGFILLER: multiSort
		case tableSearch:
			return tableSortItemSearch[column];	// SLUGFILLER: multiSort
		case tableShared:
			return tableSortItemShared[column];	// SLUGFILLER: multiSort
		case tableServer:
			return tableSortItemServer[column];	// SLUGFILLER: multiSort
		case tableClientList:
			return tableSortItemClientList[column];	// SLUGFILLER: multiSort
		case tableFilenames:
			return tableSortItemFilenames[column];	// SLUGFILLER: multiSort
	}
	return 0;
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
bool CPreferences::GetColumnSortAscending(Table t, int column) const	// SLUGFILLER: multiSort
{
	switch(t) 
	{
		case tableDownload:
			return tableSortAscendingDownload[column];	// SLUGFILLER: multiSort
		case tableUpload:
			return tableSortAscendingUpload[column];	// SLUGFILLER: multiSort
		case tableQueue:
			return tableSortAscendingQueue[column];	// SLUGFILLER: multiSort
		case tableSearch:
			return tableSortAscendingSearch[column];	// SLUGFILLER: multiSort
		case tableShared:
			return tableSortAscendingShared[column];	// SLUGFILLER: multiSort
		case tableServer:
			return tableSortAscendingServer[column];	// SLUGFILLER: multiSort
		case tableClientList:
			return tableSortAscendingClientList[column];	// SLUGFILLER: multiSort
		case tableFilenames:
			return tableSortAscendingFilenames[column];	// SLUGFILLER: multiSort
	}
	return true;
}

// SLUGFILLER: multiSort
int CPreferences::GetColumnSortCount(Table t) const
{
	int i;
	switch(t)
	{
		case tableDownload:
			for (i = 0; i < ARRSIZE(tableSortItemDownload); i++)
				if (tableSortItemDownload[i] == -1)
					break;
			return i;
		case tableUpload:
			for (i = 0; i < ARRSIZE(tableSortItemUpload); i++)
				if (tableSortItemUpload[i] == -1)
					break;
			return i;
		case tableQueue:
			for (i = 0; i < ARRSIZE(tableSortItemQueue); i++)
				if (tableSortItemQueue[i] == -1)
					break;
			return i;
		case tableSearch:
			for (i = 0; i < ARRSIZE(tableSortItemSearch); i++)
				if (tableSortItemSearch[i] == -1)
					break;
			return i;
		case tableShared:
			for (i = 0; i < ARRSIZE(tableSortItemShared); i++)
				if (tableSortItemShared[i] == -1)
					break;
			return i;
		case tableServer:
			for (i = 0; i < ARRSIZE(tableSortItemServer); i++)
				if (tableSortItemServer[i] == -1)
					break;
			return i;
		case tableClientList:
			for (i = 0; i < ARRSIZE(tableSortItemClientList); i++)
				if (tableSortItemClientList[i] == -1)
					break;
			return i;
		case tableFilenames:
			for (i = 0; i < ARRSIZE(tableSortItemFilenames); i++)
				if (tableSortItemFilenames[i] == -1)
					break;
			return i;
	}
	return 0;
}
// SLUGFILLER: multiSort

// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortItem(Table t, int sortItem)
{
	int i;	// SLUGFILLER: multiSort - roll params
	switch(t) 
	{
		case tableDownload:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ARRSIZE(tableSortItemDownload)-1; i++)
				if (tableSortItemDownload[i] == sortItem ||
					tableSortItemDownload[i] == -1)
					break;
			for (; i > 0; i--) {
				tableSortItemDownload[i] = tableSortItemDownload[i-1];
				tableSortAscendingDownload[i] = tableSortAscendingDownload[i-1];
			}
			tableSortItemDownload[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableUpload:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ARRSIZE(tableSortItemUpload)-1; i++)
				if (tableSortItemUpload[i] == sortItem ||
					tableSortItemUpload[i] == -1)
					break;
			for (; i > 0; i--) {
				tableSortItemUpload[i] = tableSortItemUpload[i-1];
				tableSortAscendingUpload[i] = tableSortAscendingUpload[i-1];
			}
			tableSortItemUpload[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableQueue:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ARRSIZE(tableSortItemQueue)-1; i++)
				if (tableSortItemQueue[i] == sortItem ||
					tableSortItemQueue[i] == -1)
					break;
			for (; i > 0; i--) {
				tableSortItemQueue[i] = tableSortItemQueue[i-1];
				tableSortAscendingQueue[i] = tableSortAscendingQueue[i-1];
			}
			tableSortItemQueue[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableSearch:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ARRSIZE(tableSortItemSearch)-1; i++)
				if (tableSortItemSearch[i] == sortItem ||
					tableSortItemSearch[i] == -1)
					break;
			for (; i > 0; i--) {
				tableSortItemSearch[i] = tableSortItemSearch[i-1];
				tableSortAscendingSearch[i] = tableSortAscendingSearch[i-1];
			}
			tableSortItemSearch[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableShared:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ARRSIZE(tableSortItemShared)-1; i++)
				if (tableSortItemShared[i] == sortItem ||
					tableSortItemShared[i] == -1)
					break;
			for (; i > 0; i--) {
				tableSortItemShared[i] = tableSortItemShared[i-1];
				tableSortAscendingShared[i] = tableSortAscendingShared[i-1];
			}
			tableSortItemShared[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableServer:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ARRSIZE(tableSortItemServer)-1; i++)
				if (tableSortItemServer[i] == sortItem ||
					tableSortItemServer[i] == -1)
					break;
			for (; i > 0; i--) {
				tableSortItemServer[i] = tableSortItemServer[i-1];
				tableSortAscendingServer[i] = tableSortAscendingServer[i-1];
			}
			tableSortItemServer[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableClientList:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ARRSIZE(tableSortItemClientList)-1; i++)
				if (tableSortItemClientList[i] == sortItem ||
					tableSortItemClientList[i] == -1)
					break;
			for (; i > 0; i--) {
				tableSortItemClientList[i] = tableSortItemClientList[i-1];
				tableSortAscendingClientList[i] = tableSortAscendingClientList[i-1];
			}
			tableSortItemClientList[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
		case tableFilenames:
			// SLUGFILLER: multiSort - roll params
			for (i = 0; i < ARRSIZE(tableSortItemFilenames)-1; i++)
				if (tableSortItemFilenames[i] == sortItem ||
					tableSortItemFilenames[i] == -1)
					break;
			for (; i > 0; i--) {
				tableSortItemFilenames[i] = tableSortItemFilenames[i-1];
				tableSortAscendingFilenames[i] = tableSortAscendingFilenames[i-1];
			}
			tableSortItemFilenames[0] = sortItem;
			// SLUGFILLER: multiSort
			break;
	}
}

// Barry - Provide a mechanism for all tables to store/retrieve sort order
void CPreferences::SetColumnSortAscending(Table t, bool sortAscending)
{
	switch(t) 
	{
		case tableDownload:
			tableSortAscendingDownload[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableUpload:
			tableSortAscendingUpload[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableQueue:
			tableSortAscendingQueue[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableSearch:
			tableSortAscendingSearch[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableShared:
			tableSortAscendingShared[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableServer:
			tableSortAscendingServer[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableClientList:
			tableSortAscendingClientList[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
		case tableFilenames:
			tableSortAscendingFilenames[0] = sortAscending;	// SLUGFILLER: multiSort - change for newest param only
			break;
	}
}

void CPreferences::RemoveCat(int index)	{
	if (index>=0 && index<catMap.GetCount()) { 
		Category_Struct* delcat;
		delcat=catMap.GetAt(index); 
		catMap.RemoveAt(index); 
		delete delcat;
	}
}

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

bool CPreferences::IsInstallationDirectory(const CString& rstrDir)
{
	CString strFullPath;
	if (PathCanonicalize(strFullPath.GetBuffer(MAX_PATH), rstrDir))
		strFullPath.ReleaseBuffer();
	else
		strFullPath = rstrDir;
	
	// skip sharing of several special eMule folders
	if (!CompareDirectories(strFullPath, GetAppDir()))			// ".\eMule"
		return true;
	if (!CompareDirectories(strFullPath, GetConfigDir()))		// ".\eMule\config"
		return true;
	if (!CompareDirectories(strFullPath, GetWebServerDir()))	// ".\eMule\webserver"
		return true;
	if (!CompareDirectories(strFullPath, GetLangDir()))			// ".\eMule\lang"
		return true;

	return false;
}

bool CPreferences::IsShareableDirectory(const CString& rstrDir)
{
	if (IsInstallationDirectory(rstrDir))
		return false;

	CString strFullPath;
	if (PathCanonicalize(strFullPath.GetBuffer(MAX_PATH), rstrDir))
		strFullPath.ReleaseBuffer();
	else
		strFullPath = rstrDir;
	
	// skip sharing of several special eMule folders
	if (!CompareDirectories(strFullPath, GetTempDir()))			// ".\eMule\temp"
		return false;

	return true;
}

void CPreferences::UpdateLastVC()
{
	versioncheckLastAutomatic = safe_mktime(CTime::GetCurrentTime().GetLocalTm());
}

void CPreferences::SetWSPass(CString strNewPass)
{
	sprintf(m_sWebPassword,"%s",MD5Sum(strNewPass).GetHash().GetBuffer(0));
}

void CPreferences::SetWSLowPass(CString strNewPass)
{
	sprintf(m_sWebLowPassword,"%s",MD5Sum(strNewPass).GetHash().GetBuffer(0));
}

void CPreferences::SetMMPass(CString strNewPass)
{
	sprintf(m_sMMPassword,"%s",MD5Sum(strNewPass).GetHash().GetBuffer(0));
}

void CPreferences::SetMaxUpload(uint16 in)
{
	maxupload = (in) ? in : UNLIMITED;
}

void CPreferences::SetMaxDownload(uint16 in)
{
	maxdownload=(in) ? in : UNLIMITED;
}

uint16 CPreferences::GetMaxSourcePerFileSoft()
{
	UINT temp = ((UINT)maxsourceperfile * 9L) / 10;
	if (temp > MAX_SOURCES_FILE_SOFT)
		return MAX_SOURCES_FILE_SOFT;
	return temp;
}

uint16 CPreferences::GetMaxSourcePerFileUDP()
{	
	UINT temp = ((UINT)maxsourceperfile * 3L) / 4;
	if (temp > MAX_SOURCES_FILE_UDP)
		return MAX_SOURCES_FILE_UDP;
	return temp;
}

void CPreferences::SetNetworkKademlia(bool val)	{ 
	networkkademlia = val; 
//	theApp.emuledlg->toolbar->ReloadConfig();// TODO: Remove this line as soon as we always show the kadbutton
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
	return prefs->minupload<prefs->maxupload && prefs->maxupload != UNLIMITED && prefs->m_bSUCEnabled;
}
//MORPH END  - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]

//MORPH START - Added by SiRoB, (SUC) & (USS)
void	CPreferences::SetMinUpload(uint16 in)
{
	prefs->minupload = (in) ? in : UNLIMITED;
}
//MORPH END  - Added by SiRoB, (SUC) & (USS)
