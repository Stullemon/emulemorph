#pragma once

class CPerfLog
{
public:
	CPerfLog();

	void Startup();
	void Shutdown();
	void LogSamples();

protected:
	enum ELogMode {
		None,
		OneSample
	} m_eMode;
	DWORD m_dwInterval;
	bool m_bInitialized;
	CString m_strFilePath;
	DWORD m_dwLastSampled;
	uint64 m_nLastSessionSentBytes;
	uint64 m_nLastSessionRecvBytes;
	uint64 m_nLastDnOH;
	uint64 m_nLastUpOH;

	void WriteSamples(UINT nCurDn, UINT nCurUp, UINT nCurDnOH, UINT uCurUpOH);
};

extern CPerfLog thePerfLog;
