#pragma once

enum EDebugLogPriority{
	DLP_VERYLOW = 0,
	DLP_LOW,
	DLP_DEFAULT,
	DLP_HIGH,
	DLP_VERYHIGH
};

// Log message type enumeration
#define	LOG_INFO		0
#define	LOG_WARNING		1
#define	LOG_ERROR		2
#define	LOG_SUCCESS		3
#define	LOGMSGTYPEMASK	0x03

// Log message targets flags
#define	LOG_DEFAULT		0x00
#define	LOG_DEBUG		0x10
#define	LOG_STATUSBAR	0x20


void Log(LPCTSTR pszLine, ...);
void LogError(LPCTSTR pszLine, ...);
void LogWarning(LPCTSTR pszLine, ...);

void Log(UINT uFlags, LPCTSTR pszLine, ...);
void LogError(UINT uFlags, LPCTSTR pszLine, ...);
void LogWarning(UINT uFlags, LPCTSTR pszLine, ...);

void DebugLog(LPCTSTR pszLine, ...);
void DebugLogError(LPCTSTR pszLine, ...);
void DebugLogWarning(LPCTSTR pszLine, ...);

void DebugLog(UINT uFlags, LPCTSTR pszLine, ...);
void DebugLogError(UINT uFlags, LPCTSTR pszLine, ...);
void DebugLogWarning(UINT uFlags, LPCTSTR pszLine, ...);

void LogV(UINT uFlags, LPCTSTR pszFmt, va_list argp);

void AddLogLine(bool bAddToStatusBar, LPCTSTR pszLine, ...);
void AddDebugLogLine(bool bAddToStatusBar, LPCTSTR pszLine, ...);
void AddDebugLogLine(EDebugLogPriority Priority, bool bAddToStatusBar, LPCTSTR pszLine, ...);

void AddLogTextV(UINT uFlags, EDebugLogPriority dlpPriority, LPCTSTR pszLine, va_list argp);


///////////////////////////////////////////////////////////////////////////////
// CLogFile

class CLogFile
{
public:
	CLogFile();
	~CLogFile();

	bool IsOpen() const;
	const CString& GetFilePath() const;
	bool SetFilePath(LPCTSTR pszFilePath);
	void SetMaxFileSize(UINT uMaxFileSize);

	bool Create(LPCTSTR pszFilePath, UINT uMaxFileSize = 1024*1024);
	bool Open();
	bool Close();
	bool Log(LPCTSTR psz, int iLen = -1);
	void StartNewLogFile();

protected:
	FILE* m_fp;
	time_t m_tStarted;
	CString m_strFilePath;
	UINT m_uBytesWritten;
	UINT m_uMaxFileSize;
	bool m_bInOpenCall;
//Morph START - added by AndCycle, Date File Name Log
private:
	CString m_strOriginFileName;
	DWORD	m_dwNextRenameTick;
//Morph END - added by AndCycle, Date File Name Log
};

extern CLogFile theLog;
extern CLogFile theVerboseLog;
