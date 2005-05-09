//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#pragma once
#include "KnownFile.h"
#include "QArray.h"


typedef struct
{
	CString	m_strFileName;
	CString	m_strFileType;
	CString	m_strFileHash;
	CString	m_strIndex;
	uint32	m_uFileSize;
	uint32	m_uSourceCount;
	uint32	m_dwCompleteSourceCount;
} SearchFileStruct;


class CFileDataIO;

class CSearchFile : public CAbstractFile
{
	DECLARE_DYNAMIC(CSearchFile)

	friend class CPartFile;
	friend class CSearchListCtrl;
public:
	CSearchFile(CFileDataIO* in_data, bool bOptUTF8, uint32 nSearchID,
				uint32 nServerIP=0, uint16 nServerPort=0,
				LPCTSTR pszDirectory = NULL, 
				bool nKademlia = false);
	CSearchFile(const CSearchFile* copyfrom);
	CSearchFile(uint32 nSearchID, const uchar* pucFileHash, uint32 uFileSize, LPCTSTR pszFileName, int iFileType, int iAvailability);
	virtual ~CSearchFile();

	bool	IsKademlia() const { return m_nKademlia; }
	uint32	AddSources(uint32 count);
	uint32	GetSourceCount() const;
	uint32	AddCompleteSources(uint32 count);
	uint32	GetCompleteSourceCount() const;
	int		IsComplete() const;
	int		IsComplete(UINT uSources, UINT uCompleteSources) const;
	time_t	GetLastSeenComplete() const;
	uint32	GetSearchID() const { return m_nSearchID; }
	LPCTSTR	GetFakeComment() const { return m_pszIsFake; } //MORPH - Added by SiRoB, FakeCheck, FakeReport, Auto-updating
	LPCTSTR GetDirectory() const { return m_pszDirectory; }

	uint32	GetClientID() const				{ return m_nClientID; }
	void	SetClientID(uint32 nClientID)	{ m_nClientID = nClientID; }
	uint16	GetClientPort() const			{ return m_nClientPort; }
	void	SetClientPort(uint16 nPort)		{ m_nClientPort = nPort; }
	uint32	GetClientServerIP() const		{ return m_nClientServerIP; }
	void	SetClientServerIP(uint32 uIP)   { m_nClientServerIP = uIP; }
	uint16	GetClientServerPort() const		{ return m_nClientServerPort; }
	void	SetClientServerPort(uint16 nPort) { m_nClientServerPort = nPort; }
	int		GetClientsCount() const			{ return ((GetClientID() && GetClientPort()) ? 1 : 0) + m_aClients.GetSize(); }

	// GUI helpers
	CSearchFile* GetListParent() const		{ return m_list_parent; }
	void		 SetListParent(CSearchFile* parent) { m_list_parent = parent; }
	uint16		 GetListChildCount() const	{ return m_list_childcount;}
	void		 SetListChildCount(int cnt)	{ m_list_childcount = cnt; }
	void		 AddListChildCount(int cnt) { m_list_childcount += cnt; }
	bool		 IsListExpanded() const		{ return m_list_bExpanded; }
	void		 SetListExpanded(bool val)	{ m_list_bExpanded = val; }

	struct SClient {
		SClient() {
			m_nIP = m_nPort = m_nServerIP = m_nServerPort = 0;
		}
		SClient(uint32 nIP, UINT nPort, uint32 nServerIP, UINT nServerPort) {
			m_nIP = nIP;
			m_nPort = nPort;
			m_nServerIP = nServerIP;
			m_nServerPort = nServerPort;
		}
		uint32 m_nIP;
		uint32 m_nServerIP;
		uint16 m_nPort;
		uint16 m_nServerPort;
	};
	void AddClient(const SClient& client) { m_aClients.Add(client); }
	const CSimpleArray<SClient>& GetClients() const { return m_aClients; }

	struct SServer {
		SServer() {
			m_nIP = m_nPort = 0;
			m_uAvail = 0;
		}
		SServer(uint32 nIP, UINT nPort) {
			m_nIP = nIP;
			m_nPort = nPort;
			m_uAvail = 0;
		}
		uint32 m_nIP;
		uint16 m_nPort;
		UINT   m_uAvail;
	};
	void AddServer(const SServer& server) { m_aServers.Add(server); }
	const CSimpleArray<SServer>& GetServers() const { return m_aServers; }
	SServer& GetServerAt(int iServer) { return m_aServers[iServer]; }
	
	void	AddPreviewImg(CxImage* img)	{	m_listImages.Add(img); }
	const CSimpleArray<CxImage*>& GetPreviews() const { return m_listImages; }
	bool	IsPreviewPossible() const { return m_bPreviewPossible;}
	void	SetPreviewPossible(bool in)	{ m_bPreviewPossible = in; }

	enum EKnownType
	{
		NotDetermined,
		Shared,
		Downloading,
		Downloaded,
		Cancelled,
		Unknown
	};

	EKnownType GetKnownType() const { return m_eKnown; }
	void SetKnownType(EKnownType eType) { m_eKnown = eType; }

private:
	bool	m_nKademlia;
	uint32	m_nClientID;
	uint16	m_nClientPort;
	uint32	m_nSearchID;
	uint32	m_nClientServerIP;
	uint16	m_nClientServerPort;
	CSimpleArray<SClient> m_aClients;
	CSimpleArray<SServer> m_aServers;
	CSimpleArray<CxImage*> m_listImages;
	LPTSTR m_pszDirectory;
	LPTSTR m_pszIsFake; //MORPH - Added by SiRoB, FakeCheck, FakeReport, Auto-updating

	// GUI helpers
	bool		 m_bPreviewPossible;
	bool		 m_list_bExpanded;
	uint16		 m_list_childcount;
	CSearchFile* m_list_parent;
	EKnownType	m_eKnown;
};

__inline bool __stdcall operator==(const CSearchFile::SServer& s1, const CSearchFile::SServer& s2)
{
	return s1.m_nIP==s2.m_nIP && s1.m_nPort==s2.m_nPort;
}

__inline bool __stdcall operator==(const CSearchFile::SClient& c1, const CSearchFile::SClient& c2)
{
	return c1.m_nIP==c2.m_nIP && c1.m_nPort==c2.m_nPort &&
		   c1.m_nServerIP==c2.m_nServerIP && c1.m_nServerPort==c2.m_nServerPort;
}


class CSearchList
{
friend class CSearchListCtrl;
public:
	CSearchList();
	~CSearchList();
	void	Clear();
	void	NewSearch(CSearchListCtrl* in_wnd, CStringA strResultFileType, uint32 nSearchID, bool MobilMuleSearch = false);
	uint16	ProcessSearchAnswer(const uchar* packet, uint32 size, CUpDownClient* Sender, bool* pbMoreResultsAvailable, LPCTSTR pszDirectory = NULL);
	uint16	ProcessSearchAnswer(const uchar* packet, uint32 size, bool bOptUTF8, uint32 nServerIP, uint16 nServerPort, bool* pbMoreResultsAvailable);
	uint16	ProcessUDPSearchAnswer(CFileDataIO& packet, bool bOptUTF8, uint32 nServerIP, uint16 nServerPort);
	uint16	GetResultCount() const;
	uint16	GetResultCount(uint32 nSearchID) const;
	void	AddResultCount(uint32 nSearchID, const uchar* hash, UINT nCount);
	void	SetOutputWnd(CSearchListCtrl* in_wnd)		{outputwnd = in_wnd;}
	void	RemoveResults(  uint32 nSearchID );
	void	RemoveResult(CSearchFile* todel);
	void	ShowResults(uint32 nSearchID);
	void	GetWebList(CQArray<SearchFileStruct, SearchFileStruct> *SearchFileArray, int iSortBy) const;
	void	AddFileToDownloadByHash(const uchar* hash)		{AddFileToDownloadByHash(hash,0);}
	void	AddFileToDownloadByHash(const uchar* hash, uint8 cat);
	bool	AddToList(CSearchFile* toadd, bool bClientResponse = false);
	CSearchFile* GetSearchFileByHash(const uchar* hash) const;
	void	KademliaSearchKeyword(uint32 searchID, const Kademlia::CUInt128* pfileID, LPCTSTR name, uint32 size, LPCTSTR type, uint16 numProperties, ...);

	uint16	GetFoundFiles(uint32 searchID) const {
		uint16 returnVal=0;
		VERIFY( m_foundFilesCount.Lookup(searchID,returnVal) );
		return returnVal;
	}
	// mobilemule
	CSearchFile*	DetachNextFile(uint32 nSearchID);

private:
	CTypedPtrList<CPtrList, CSearchFile*> list;
	CMap<uint32, uint32, uint16, uint16> m_foundFilesCount;
	CMap<uint32, uint32, uint16, uint16> m_foundSourcesCount;

	CSearchListCtrl*	outputwnd;
	CString m_strResultFileType;
	uint32	m_nCurSearchID;
	bool	m_MobilMuleSearch;
public:
	//MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
	CString GetWapList(CString linePattern,int sortby,bool asc, int start, int max, bool &more) const;
	//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]
};
