//emulEspaña - Added by MoNKi [MoNKi: -Wap Server-]
#pragma once

#define SESSION_TIMEOUT_SECS	300	// 5 minutes session expiration
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

//emulEspaña: Modified by MoNKi [itsonlyme: -Fix: webserver uint sizes-]
/*
typedef struct
{
	CString	sFileName;
	uint32	lFileSize;
	uint32	nFileTransferred;
    uint64	nFileAllTimeTransferred;
	uint16	nFileRequests;
	uint32	nFileAllTimeRequests;
	uint16	nFileAccepts;
	uint32	nFileAllTimeAccepts;
	uint8	nFilePriority;
	CString sFilePriority;
	bool	bFileAutoPriority;
	CString sFileHash;
	CString	sED2kLink;
} SharedFiles;
*/

typedef struct
{
	CString	sFileName;
	uint32	lFileSize;
	uint64	nFileTransferred;
    uint64	nFileAllTimeTransferred;
	uint16	nFileRequests;
	uint16	nFileAllTimeRequests;
	uint16	nFileAccepts;
	uint16	nFileAllTimeAccepts;
	uint8	nFilePriority;
	CString sFilePriority;
	bool	bFileAutoPriority;
	CString sFileHash;
	CString	sED2kLink;
} SharedFiles;
//End emulEspaña

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