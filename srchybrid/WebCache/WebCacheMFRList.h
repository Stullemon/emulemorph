#pragma once
#include "SafeFile.h"
//#include "UpDownClient.h"

class CWebCacheMFRReqFile
{
public:
	CWebCacheMFRReqFile(void) {}
	~CWebCacheMFRReqFile(void) {}
	byte		fileID[16];
	uint16		partCount;
	CArray<uint8, uint8>	partStatus;
};

typedef CTypedPtrList<CPtrList, CWebCacheMFRReqFile*> CReqFileList;

class CWebCacheMFRList
{
public:
	CWebCacheMFRList(void) { length = 0; client = NULL; }
	~CWebCacheMFRList(void);
	void AddFiles(CSafeMemFile* data, CUpDownClient* client = NULL);
	bool IsPartAvailable(uint16 part, const byte* fileID);
	void RemoveAll();
private:
	void CheckExpiration(); // calls RemoveAll() if the MFR is older than WC_MAX_MFR_AGE
	CReqFileList reqFiles;
	uint32 expirationTick;
	CUpDownClient* client;
	uint32 length;
};