#pragma once
#include "WapServer/WebWapDefinitions.h"	//MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]

class CWebSocket;
class CUpDownClient;

#define WEB_GRAPH_HEIGHT		120
#define WEB_GRAPH_WIDTH			500

// emulEspaña: Removed by MoNKi, now in WebWapDefinitions.h [MoNKi: -Wap Server-]
/*
#define SHORT_FILENAME_LENGTH	40	// Max size of file name.

typedef struct { float download; float upload;  long connections; } UpDown;

typedef struct { CTime startTime; long lSession; bool admin;} Session;

typedef struct
{
	CString	sFileName;
	CString	sFileStatus;
	uint32	lFileSize;
	uint32	lFileTransferred;
	uint32	lFileSpeed;
	uint16	lSourceCount;
	uint16	lNotCurrentSourceCount;
	uint16	lTransferringSourceCount;
	float	fCompleted;
	int		nFileStatus;
	int		nFilePrio;
	CString	sFileHash;
	CString	sED2kLink;
	CString	sFileInfo;
} DownloadFiles;

typedef struct
{
	CString	sFileName;
	uint64	lFileSize;
	uint64	nFileTransferred;//Morph - modify by AndCycle, [fix] webserver uint sizes(itsonlyme)
    uint64	nFileAllTimeTransferred;
	uint16	nFileRequests;
	uint16	nFileAllTimeRequests;//Morph - modify by AndCycle, [fix] webserver uint sizes(itsonlyme)
	uint16	nFileAccepts;
	uint32	nFileAllTimeAccepts;
	uint8	nFilePriority;
	CString sFilePriority;
	bool	bFileAutoPriority;
	CString sFileHash;
	CString	sED2kLink;
} SharedFiles;

typedef enum
{
	DOWN_SORT_NAME,
	DOWN_SORT_SIZE,
	DOWN_SORT_TRANSFERRED,
	DOWN_SORT_SPEED,
	DOWN_SORT_PROGRESS
} DownloadSort;

typedef enum
{
	SHARED_SORT_NAME,
	SHARED_SORT_SIZE,
	SHARED_SORT_TRANSFERRED,
	SHARED_SORT_ALL_TIME_TRANSFERRED,
	SHARED_SORT_REQUESTS,
	SHARED_SORT_ALL_TIME_REQUESTS,
	SHARED_SORT_ACCEPTS,
    SHARED_SORT_ALL_TIME_ACCEPTS,
	SHARED_SORT_PRIORITY
} SharedSort;

typedef struct
{
	CString	sServerName;
	CString	sServerDescription;
	int		nServerPort;
	CString	sServerIP;
	int		nServerUsers;
	int		nServerMaxUsers;
	int		nServerFiles;
} ServerEntry;

typedef enum
{
	SERVER_SORT_NAME,
	SERVER_SORT_DESCRIPTION,
	SERVER_SORT_IP,
	SERVER_SORT_USERS,
	SERVER_SORT_FILES
} ServerSort;

struct BadLogin {
	uint32	datalen;
	DWORD	timestamp;
};

typedef struct
{
	uint32			nUsers;
	DownloadSort	DownloadSort;
	bool			bDownloadSortReverse;
	ServerSort		ServerSort;
	bool			bServerSortReverse;
	SharedSort		SharedSort;
	bool			bSharedSortReverse;	
	bool			bShowUploadQueue;

	CArray<UpDown, UpDown>		PointsForWeb;
	CArray<Session, Session>	Sessions;
	CArray<BadLogin, BadLogin> badlogins;	//TransferredData= IP : time
	
	CString sLastModified;
	CString	sETag;
} GlobalParams;
*/
typedef struct
{
	CString			sURL;
	in_addr			inadr;
	void			*pThis;
	CWebSocket		*pSocket;
} ThreadData;

typedef struct
{
	CString	sHeader;
	CString	sHeaderMetaRefresh;
	CString	sHeaderStylesheet;
	CString	sFooter;
	CString	sServerList;
	CString	sServerLine;
	CString	sTransferImages;
	CString	sTransferList;
	CString	sTransferDownHeader;
	CString	sTransferDownFooter;
	CString	sTransferDownLine;
	CString	sTransferDownLineGood;
	CString	sTransferUpHeader;
	CString	sTransferUpFooter;
	CString	sTransferUpLine;
	CString	sTransferUpQueueShow;
	CString	sTransferUpQueueHide;
	CString	sTransferUpQueueLine;
	CString	sTransferBadLink;
	CString	sDownloadLink;
	CString	sSharedList;
	CString	sSharedLine;
	CString	sSharedLineChanged;
	CString	sGraphs;
	CString	sLog;
	CString	sServerInfo;
	CString sDebugLog;
	CString sStats;
	CString sPreferences;
	CString	sLogin;
	//MORPH START - Added by SiRoB, Login Failed from eMule+
	CString sFailedLogin;
 	//MORPH END   - Added by SiRoB, Login Failed from eMule+
	CString	sConnectedServer;
	CString	sAddServerBox;
	CString	sWebSearch;
	CString	sSearch;
	CString	sProgressbarImgs;
	CString sProgressbarImgsPercent;
	uint16	iProgressbarWidth;
	CString sSearchResultLine;
	CString sSearchHeader;
	CString sClearCompleted;
	CString sCatArrow;
	CString sBootstrapLine;
	CString sKad;
} WebTemplates;

class CWebServer
{
	friend class CWebSocket;

public:
	CWebServer(void);
	~CWebServer(void);

	int	 UpdateSessionCount();
	void StartServer(void);
	void RestartServer();
	void AddStatsLine(UpDown line);
	void ReloadTemplates();
	uint16	GetSessionCount()	{ return m_Params.Sessions.GetCount();}
	bool IsRunning()	{ return m_bServerWorking;}
	CArray<UpDown, UpDown>* GetPointsForWeb()	{return &m_Params.PointsForWeb;} // MobileMule
protected:
	static void		ProcessURL(ThreadData);
	static void		ProcessFileReq(ThreadData);

private:
	static CString	_GetHeader(ThreadData, long lSession);
	static CString	_GetFooter(ThreadData);
	static CString	_GetServerList(ThreadData);
	static CString	_GetTransferList(ThreadData);
	static CString	_GetDownloadLink(ThreadData);
	static CString	_GetSharedFilesList(ThreadData);
	static CString	_GetGraphs(ThreadData);
	static CString	_GetLog(ThreadData);
	static CString	_GetServerInfo(ThreadData);
	static CString	_GetDebugLog(ThreadData);
	static CString	_GetStats(ThreadData);
	static CString	_GetPreferences(ThreadData);
	static CString	_GetLoginScreen(ThreadData);
	//MORPH START - Added by SiRoB, Login Failed from eMule+
	static CString  _GetFailedLoginScreen(ThreadData);
	//MORPH END   - Added by SiRoB, Login Failed from eMule+
	static CString	_GetConnectedServer(ThreadData);
	static CString 	_GetAddServerBox(ThreadData Data);
	static void		_RemoveServer(CString sIP, int nPort);
    static CString	_GetWebSearch(ThreadData Data);
    static CString	_GetSearch(ThreadData Data);
	static CString	_GetKadPage(ThreadData Data);

    static CString  _GetRemoteLinkAddedOk(ThreadData Data);
    static CString  _GetRemoteLinkAddedFailed(ThreadData Data);

	static CString	_ParseURL(CString URL, CString fieldname); 
	static CString	_ParseURLArray(CString URL, CString fieldname);
	static void		_ConnectToServer(CString sIP, int nPort);
	static bool		_IsLoggedIn(ThreadData Data, long lSession);
	static void		_RemoveTimeOuts(ThreadData Data, long lSession);
	static bool		_RemoveSession(ThreadData Data, long lSession);
	static bool		_GetFileHash(CString sHash, uchar *FileHash);
	static CString	_SpecialChars(CString str);
#ifdef USE_STRING_IDS
	static CString	__GetPlainResString(RESSTRIDTYPE nID, bool noquote = false);
#define _GetPlainResString(id)			__GetPlainResString(#id, false)
#define _GetPlainResStringNoQuote(id)	__GetPlainResString(#id, true)
#else
	static CString	_GetPlainResString(RESSTRIDTYPE nID, bool noquote = false);
	static CString	_GetPlainResStringNoQuote(RESSTRIDTYPE nID) { return _GetPlainResString(nID, true); }
#endif
	static int		_GzipCompress(BYTE* dest, ULONG* destLen, const BYTE* source, ULONG sourceLen, int level);
	static void		_SetSharedFilePriority(CString hash, uint8 priority);
	static CString	_GetWebCharSet();
	CString			_LoadTemplate(CString sAll, CString sTemplateName);
	static Session	GetSessionByID(ThreadData Data,long sessionID);
	static bool		IsSessionAdmin(ThreadData Data,CString SsessionID);
	static CString	GetPermissionDenied();
	static CString	_GetDownloadGraph(ThreadData Data,CString filehash);
	static void		InsertCatBox(CString &Out,int preselect,CString boxlabel, bool jump=false,bool extraCats=false);
	static CString	GetSubCatLabel(int cat);
	// Common data
	GlobalParams	m_Params;
	WebTemplates	m_Templates;
	bool			m_bServerWorking;
	int				m_iSearchSortby;
	bool			m_bSearchAsc;

	// Elandal: Moved from CUpDownClient
	static CString	GetUploadFileInfo(CUpDownClient* client);
};
