#pragma once

class CSafeMemFile;


///////////////////////////////////////////////////////////////////////////////
// ESearchType

enum ESearchType
{
	//NOTE: The numbers are *equal* to the entries in the comboxbox -> TODO: use item data
	SearchTypeEd2kServer = 0,
	SearchTypeEd2kGlobal,
	SearchTypeKademlia,
	SearchTypeFileDonkey
};


///////////////////////////////////////////////////////////////////////////////
// SSearchParams

struct SSearchParams
{
	SSearchParams()
	{
		dwSearchID = (DWORD)-1;
		eType = SearchTypeEd2kServer;
		bClientSharedFiles = false;
		ulMinSize = 0;
		ulMaxSize = 0;
		uAvailability = 0;
		uComplete = 0;
		ulMinBitrate = 0;
		ulMinLength = 0;
		bMatchKeywords = false;
	}
	DWORD dwSearchID;
	bool bClientSharedFiles;
	CString strExpression;
	CString strKeyword;
	CString strBooleanExpr;
	ESearchType eType;
	CString strFileType;
	CString strMinSize;
	ULONG ulMinSize;
	CString strMaxSize;
	ULONG ulMaxSize;
	UINT uAvailability;
	CString strExtension;
	UINT uComplete;
	CString strCodec;
	ULONG ulMinBitrate;
	ULONG ulMinLength;
	CString strTitle;
	CString strAlbum;
	CString strArtist;
	bool bMatchKeywords;
};

bool GetSearchPacket(CSafeMemFile* data, SSearchParams* pParams);
