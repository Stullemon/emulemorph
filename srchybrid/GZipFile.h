#pragma once

typedef void* gzFile;

class CGZIPFile
{
public:
	CGZIPFile();

	bool Open(LPCTSTR pszFilePath);
	void Close();
	CString GetUncompressedFileName() const;
	CString GetUncompressedFilePath() const;
	bool Extract(LPCTSTR pszFilePath);

protected:
	CString m_strGzFilePath;
	gzFile m_gzFile;
};
