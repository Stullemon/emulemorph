#pragma once

enum EDebugLogPriority{
	DLP_VERYLOW = 0,
	DLP_LOW,
	DLP_DEFAULT,
	DLP_HIGH,
	DLP_VERYHIGH
};
///////////////////////////////////////////////////////////////////////////////
// CLoggable

class CLoggable
{
public:
	static void AddLogLine(bool addtostatusbar,UINT nID,...);
	static void AddLogLine(bool addtostatusbar,LPCTSTR line,...);
	static void AddDebugLogLine(bool addtostatusbar, UINT nID,...);
	static void AddDebugLogLine(bool addtostatusbar, LPCTSTR line,...);
	static void AddDebugLogLine(EDebugLogPriority Priority, bool addtostatusbar, UINT nID,...);
	static void AddDebugLogLine(EDebugLogPriority Priority, bool addtostatusbar, LPCTSTR line,...);

	void PacketToDebugLogLine(LPCTSTR info, char * packet, uint32 size, uint8 opcode, EDebugLogPriority dlpPriority) const;
	void TagToDebugLogLine(LPCTSTR info, LPCTSTR tag, uint32 size, uint8 opcode) const;

private:
	static void AddLogText(bool debug, bool addtostatusbar, EDebugLogPriority dlpPriority, LPCTSTR line, va_list argptr);	
};


///////////////////////////////////////////////////////////////////////////////
// CLog

class CLog
{
public:
	CLog();
	~CLog();

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
