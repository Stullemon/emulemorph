#pragma once

class api__searchResponse;

#define WM_JIGLE_SEARCH_REQUEST		(WM_USER + 0x101)

typedef struct
{
	// Jigle search paramters
	CString strPhrase;
	CString strExt;
	int iAvail;
	__int64 llMinSize;
	__int64 llMaxSize;
	int iOptions;
	int iOffset;
	int iTotal;

	// Helpers
	HWND hWnd;
	int iTotalSearchResults;
	ULONG nSearchID;
} JIGLE_SEARCH_REQUEST;

typedef struct
{
	int iResult;
	api__searchResponse* res;
	struct soap* soap;
	JIGLE_SEARCH_REQUEST* pReq;
} JIGLE_SEARCH_RESPONSE;


class CJigleSOAPThread : public CWinThread
{
	DECLARE_DYNAMIC(CJigleSOAPThread)

public:
	CJigleSOAPThread();

	virtual BOOL InitInstance();
};
