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
#include "stdafx.h"
#include "emule.h"
#include "SearchList.h"
#include "SearchParams.h"
#include "Packets.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "UpDownClient.h"
#include "SafeFile.h"
#include "MMServer.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "DownloadQueue.h"
#include "PartFile.h"
#include "CxImage/xImage.h"
#include "kademlia/utils/uint128.h"
#include "emuledlg.h"
#include "SearchDlg.h"
#include "SearchListCtrl.h"
#include "Fakecheck.h" //MORPH - Added by SiRoB
#include "Log.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


bool IsValidClientIPPort(uint32 nIP, uint16 nPort)
{
	return     nIP != 0
			&& nPort != 0
		    && (ntohl(nIP) != nPort)		// this filters most of the false data
			&& ((nIP & 0x000000FF) != 0)
			&& ((nIP & 0x0000FF00) != 0)
			&& ((nIP & 0x00FF0000) != 0)
			&& ((nIP & 0xFF000000) != 0);
}

void ConvertED2KTag(CTag*& pTag)
{
	if (pTag->GetNameID() == 0 && pTag->GetName() != NULL)
	{
		static const struct
		{
			uint8	nID;
			uint8	nED2KType;
			LPCSTR	pszED2KName;
		} _aEmuleToED2KMetaTagsMap[] = 
		{
			// Artist, Album and Title are disabled because they should be already part of the filename
			// and would therefore be redundant information sent to the servers.. and the servers count the
			// amount of sent data!
			{ FT_MEDIA_ARTIST,  TAGTYPE_STRING, FT_ED2K_MEDIA_ARTIST },
			{ FT_MEDIA_ALBUM,   TAGTYPE_STRING, FT_ED2K_MEDIA_ALBUM },
			{ FT_MEDIA_TITLE,   TAGTYPE_STRING, FT_ED2K_MEDIA_TITLE },
			{ FT_MEDIA_LENGTH,  TAGTYPE_STRING, FT_ED2K_MEDIA_LENGTH },
			{ FT_MEDIA_LENGTH,  TAGTYPE_UINT32, FT_ED2K_MEDIA_LENGTH },
			{ FT_MEDIA_BITRATE, TAGTYPE_UINT32, FT_ED2K_MEDIA_BITRATE },
			{ FT_MEDIA_CODEC,   TAGTYPE_STRING, FT_ED2K_MEDIA_CODEC }
		};

		for (int j = 0; j < ARRSIZE(_aEmuleToED2KMetaTagsMap); j++)
		{
			if (    CmpED2KTagName(pTag->GetName(), _aEmuleToED2KMetaTagsMap[j].pszED2KName) == 0
				&& (   (pTag->IsStr() && _aEmuleToED2KMetaTagsMap[j].nED2KType == TAGTYPE_STRING)
					|| (pTag->IsInt() && _aEmuleToED2KMetaTagsMap[j].nED2KType == TAGTYPE_UINT32)))
			{
				if (pTag->IsStr())
					{
						if (_aEmuleToED2KMetaTagsMap[j].nID == FT_MEDIA_LENGTH)
						{
							UINT nMediaLength = 0;
							UINT hour = 0, min = 0, sec = 0;
						if (_stscanf(pTag->GetStr(), _T("%u : %u : %u"), &hour, &min, &sec) == 3)
								nMediaLength = hour * 3600 + min * 60 + sec;
						else if (_stscanf(pTag->GetStr(), _T("%u : %u"), &min, &sec) == 2)
								nMediaLength = min * 60 + sec;
						else if (_stscanf(pTag->GetStr(), _T("%u"), &sec) == 1)
								nMediaLength = sec;

							CTag* tag = (nMediaLength != 0) ? new CTag(_aEmuleToED2KMetaTagsMap[j].nID, nMediaLength) : NULL;
							delete pTag;
							pTag = tag;
						}
						else
						{
						CTag* tag = (!pTag->GetStr().IsEmpty()) 
										? new CTag(_aEmuleToED2KMetaTagsMap[j].nID, pTag->GetStr()) 
										  : NULL;
							delete pTag;
							pTag = tag;
						}
					}
					else if (pTag->IsInt())
					{
					CTag* tag = (pTag->GetInt() != 0) 
									? new CTag(_aEmuleToED2KMetaTagsMap[j].nID, pTag->GetInt()) 
									  : NULL;
						delete pTag;
						pTag = tag;
					}
				break;
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// CSearchFile
	
CSearchFile::CSearchFile(const CSearchFile* copyfrom)
{
	md4cpy(m_abyFileHash, copyfrom->GetFileHash());
	SetFileSize(copyfrom->GetIntTagValue(FT_FILESIZE));
	SetFileName(copyfrom->GetStrTagValue(FT_FILENAME), false, false);
	SetFileType(copyfrom->GetFileType());
	m_nClientServerIP = copyfrom->GetClientServerIP();
	m_nClientID = copyfrom->GetClientID();
	m_nClientPort = copyfrom->GetClientPort();
	m_nClientServerPort = copyfrom->GetClientServerPort();
	m_pszDirectory = copyfrom->GetDirectory()? _tcsdup(copyfrom->GetDirectory()) : NULL;
	m_pszIsFake = copyfrom->GetFakeComment()? _tcsdup(copyfrom->GetFakeComment()) : NULL; //MORPH - Added by SiRoB, FakeCheck, FakeReport, Auto-updating
	m_nSearchID = copyfrom->GetSearchID();
	m_nKademlia = copyfrom->IsKademlia();
	for (int i = 0; i < copyfrom->GetTags().GetCount(); i++)
		taglist.Add(new CTag(*copyfrom->GetTags().GetAt(i)));
	const CSimpleArray<SServer>& servers = copyfrom->GetServers();
	for (i = 0; i < servers.GetSize(); i++)
		AddServer(servers[i]);

	m_list_bExpanded = false;
	m_list_parent = const_cast<CSearchFile*>(copyfrom);
	m_list_childcount = 0;
	m_bPreviewPossible = false;
	m_eKnown = copyfrom->m_eKnown;
}

CSearchFile::CSearchFile(CFileDataIO* in_data, bool bOptUTF8, uint32 nSearchID, uint32 nServerIP, uint16 nServerPort, LPCTSTR pszDirectory, bool nKademlia)
{
	m_nKademlia = nKademlia;
	m_nSearchID = nSearchID;
	in_data->ReadHash16(m_abyFileHash);
	m_nClientID = in_data->ReadUInt32();
	m_nClientPort = in_data->ReadUInt16();
	if ((m_nClientID || m_nClientPort) && !IsValidClientIPPort(m_nClientID, m_nClientPort)){
		if (thePrefs.GetDebugServerSearchesLevel() > 1)
			Debug(_T("Filtered source from search result %s:%u\n"), DbgGetClientID(m_nClientID), m_nClientPort);
		m_nClientID = 0;
		m_nClientPort = 0;
	}
	UINT tagcount = in_data->ReadUInt32();
	// NSERVER2.EXE (lugdunum v16.38 patched for Win32) returns the ClientIP+Port of the client which offered that
	// file, even if that client has not filled the according fields in the OP_OFFERFILES packet with its IP+Port.
	//
	// 16.38.p73 (lugdunum) (propenprinz)
	//  *) does not return ClientIP+Port if the OP_OFFERFILES packet does not also contain it.
	//  *) if the OP_OFFERFILES packet does contain our HighID and Port the server returns that data at least when
	//     returning search results via TCP.
	if (thePrefs.GetDebugServerSearchesLevel() > 1)
		Debug(_T("Search Result: %s  Client=%u.%u.%u.%u:%u  Tags=%u\n"), md4str(m_abyFileHash), (uint8)m_nClientID,(uint8)(m_nClientID>>8),(uint8)(m_nClientID>>16),(uint8)(m_nClientID>>24), m_nClientPort, tagcount);

	for (UINT i = 0; i < tagcount; i++){
		CTag* toadd = new CTag(in_data, bOptUTF8);
		if (thePrefs.GetDebugServerSearchesLevel() > 1)
			Debug(_T("  %s\n"), toadd->GetFullInfo());
		ConvertED2KTag(toadd);
		if (toadd)
			taglist.Add(toadd);
	}

	// here we have two choices
	//	- if the server/client sent us a filetype, we could use it (though it could be wrong)
	//	- we always trust our filetype list and determine the filetype by the extension of the file
	//
	// if we received a filetype from server, we use it.
	// if we did not receive a filetype, we determine it by examining the file's extension.
	//
	// but, in no case, we will use the receive file type when adding this search result to the download queue, to avoid
	// that we are using 'wrong' file types in part files. (this has to be handled when creating the part files)
	const CString& rstrFileType = GetStrTagValue(FT_FILETYPE);
	SetFileName(GetStrTagValue(FT_FILENAME), false, rstrFileType.IsEmpty());
	SetFileSize(GetIntTagValue(FT_FILESIZE));
	if (!rstrFileType.IsEmpty())
	{
		if (_tcscmp(rstrFileType, _T(ED2KFTSTR_PROGRAM))==0)
		{
			CString strDetailFileType = GetFileTypeByName(GetFileName());
			if (!strDetailFileType.IsEmpty())
				SetFileType(strDetailFileType);
			else
				SetFileType(rstrFileType);
		}
		else
			SetFileType(rstrFileType);
	}

	m_nClientServerIP = nServerIP;
	m_nClientServerPort = nServerPort;
	if (m_nClientServerIP && m_nClientServerPort){
		SServer server(m_nClientServerIP, m_nClientServerPort);
		server.m_uAvail = GetIntTagValue(FT_SOURCES);
		AddServer(server);
	}
	m_pszDirectory = pszDirectory ? _tcsdup(pszDirectory) : NULL;
	m_pszIsFake = theApp.FakeCheck->IsFake(m_abyFileHash,GetFileSize()) ? _tcsdup(theApp.FakeCheck->GetLastHit()) : NULL;; //MORPH - Added by SiRoB, FakeCheck, FakeReport, Auto-updating
	m_list_bExpanded = false;
	m_list_parent = NULL;
	m_list_childcount = 0;
	m_bPreviewPossible = false;
	m_eKnown = NotDetermined;
}

CSearchFile::CSearchFile(uint32 nSearchID, const uchar* pucFileHash, uint32 uFileSize, LPCTSTR pszFileName, int iFileType, int iAvailability)
{
	m_nSearchID = nSearchID;
	md4cpy(m_abyFileHash, pucFileHash);
	taglist.Add(new CTag(FT_FILESIZE, uFileSize));
	taglist.Add(new CTag(FT_FILENAME, pszFileName));
	taglist.Add(new CTag(FT_SOURCES, iAvailability));
	SetFileName(pszFileName);
	SetFileSize(uFileSize);

	m_nKademlia = 0;
	m_nClientID = 0;
	m_nClientPort = 0;
	m_nClientServerIP = 0;
	m_nClientServerPort = 0;
	m_pszDirectory = NULL;
	m_pszIsFake = theApp.FakeCheck->IsFake(m_abyFileHash,uFileSize) ? _tcsdup(theApp.FakeCheck->GetLastHit()) : NULL; //MORPH - Added by SiRoB, FakeCheck, FakeReport, Auto-updating
	m_list_bExpanded = false;
	m_list_parent = NULL;
	m_list_childcount = 0;
	m_bPreviewPossible = false;
	m_eKnown = NotDetermined;
}

CSearchFile::~CSearchFile()
{
	for (int i = 0; i < taglist.GetSize();i++)
		delete taglist[i];
	if (m_pszDirectory)
		free(m_pszDirectory);
	//MORPH START - Added by SiRoB, FakeCheck, FakeReport, Auto-updating
	if (m_pszIsFake)
		free(m_pszIsFake);
	//MORPH END   - Added by SiRoB, FakeCheck, FakeReport, Auto-updating
	for (int i = 0; i < m_listImages.GetSize(); i++)
		delete m_listImages[i];
}

uint32 CSearchFile::AddSources(uint32 count)
{
	for (int i = 0; i < taglist.GetSize(); i++)
	{
		CTag* pTag = taglist[i];
		if (pTag->GetNameID() == FT_SOURCES)
		{
			if(m_nKademlia)
			{
				if (pTag->GetInt() < count)
					pTag->SetInt(count);
			}
			else
				pTag->SetInt(pTag->GetInt() + count);
			return pTag->GetInt();
		}
	}

	// FT_SOURCES is not yet supported by clients, we may have to create such a tag..
	CTag* pTag = new CTag(FT_SOURCES, count);
	taglist.Add(pTag);
	return count;
}

uint32 CSearchFile::GetSourceCount() const
{
	return GetIntTagValue(FT_SOURCES);
}

uint32 CSearchFile::AddCompleteSources(uint32 count)
{
	for (int i = 0; i < taglist.GetSize(); i++)
	{
		CTag* pTag = taglist[i];
		if (pTag->GetNameID() == FT_COMPLETE_SOURCES)
		{
			if (m_nKademlia)
			{
				if (pTag->GetInt() < count)
					pTag->SetInt(count);
			}
			else
				pTag->SetInt(pTag->GetInt() + count);
			return pTag->GetInt();
		}
	}

	// FT_COMPLETE_SOURCES is not yet supported by all servers, we may have to create such a tag..
	CTag* pTag = new CTag(FT_COMPLETE_SOURCES, count);
	taglist.Add(pTag);
	return count;
}

uint32 CSearchFile::GetCompleteSourceCount() const
{
	return GetIntTagValue(FT_COMPLETE_SOURCES);
}

int CSearchFile::IsComplete() const
{
	return IsComplete(GetSourceCount(), GetIntTagValue(FT_COMPLETE_SOURCES));
}

int CSearchFile::IsComplete(UINT uSources, UINT uCompleteSources) const
{
	if (IsKademlia())
		return -1;		// unknown
	else if (uSources > 0 && uCompleteSources > 0)
		return 1;		// complete
	else
		return 0;		// not complete
}

time_t CSearchFile::GetLastSeenComplete() const
{
	return GetIntTagValue(FT_LASTSEENCOMPLETE);
}


///////////////////////////////////////////////////////////////////////////////
// CSearchList

CSearchList::CSearchList(){
	outputwnd = 0;
	m_MobilMuleSearch = false;
}

CSearchList::~CSearchList(){
	Clear();
}

void CSearchList::Clear(){
	for(POSITION pos = list.GetHeadPosition(); pos != NULL; )
		delete list.GetNext(pos);
	list.RemoveAll();
}

void CSearchList::RemoveResults( uint32 nSearchID)
{
	// this will not delete the item from the window, make sure your code does it if you call this
	ASSERT( outputwnd );
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;)
	{
		POSITION posLast = pos;
		CSearchFile* cur_file =	list.GetNext(pos);
		if (cur_file->GetSearchID() == nSearchID)
		{
			list.RemoveAt(posLast);
			delete cur_file;
		}
	}
}

void CSearchList::ShowResults(uint32 nSearchID)
{
	ASSERT( outputwnd );
	outputwnd->SetRedraw(false);
	for (POSITION pos = list.GetHeadPosition(); pos != 0;)
	{
		const CSearchFile* cur_file = list.GetNext(pos);
		if( cur_file->GetSearchID() == nSearchID && cur_file->GetListParent()==NULL)
			outputwnd->AddResult(cur_file);
	}
	outputwnd->SetRedraw(true);
}

void CSearchList::RemoveResult(CSearchFile* todel)
{
	for (POSITION pos = list.GetHeadPosition(); pos != 0;)
	{
		POSITION posLast = pos;
		CSearchFile* cur_file = list.GetNext(pos);
		if (cur_file == todel)
		{
			theApp.emuledlg->searchwnd->RemoveResult(todel);
			list.RemoveAt(posLast);
			delete todel;
			return;
		}
	}
}

void CSearchList::NewSearch(CSearchListCtrl* in_wnd, CStringA strResultFileType, uint32 nSearchID, bool MobilMuleSearch)
{
	if(in_wnd)
		outputwnd = in_wnd;

	m_strResultFileType = strResultFileType;
	m_nCurrentSearch = nSearchID;
	m_foundFilesCount.SetAt(nSearchID,0);
	m_foundSourcesCount.SetAt(nSearchID,0);
	m_MobilMuleSearch = MobilMuleSearch;
}

uint16 CSearchList::ProcessSearchanswer(char* in_packet, uint32 size, 
										CUpDownClient* Sender, bool* pbMoreResultsAvailable, LPCTSTR pszDirectory)
{
	ASSERT( Sender != NULL );
	// Elandal: Assumes sizeof(void*) == sizeof(uint32)
	uint32 nSearchID = (uint32)Sender;
	SSearchParams* pParams = new SSearchParams;
	pParams->strExpression = Sender->GetUserName();
	pParams->dwSearchID = nSearchID;
	pParams->bClientSharedFiles = true;
	if (theApp.emuledlg->searchwnd->CreateNewTab(pParams)){
		m_foundFilesCount.SetAt(nSearchID,0);
		m_foundSourcesCount.SetAt(nSearchID,0);
	}
	else{
		delete pParams;
		pParams = NULL;
	}

	CSafeMemFile packet((BYTE*)in_packet,size);
	UINT results = packet.ReadUInt32();
	for (UINT i = 0; i < results; i++){
		CSearchFile* toadd = new CSearchFile(&packet, Sender ? Sender->GetUnicodeSupport() : false, nSearchID, 0, 0, pszDirectory);
		if (Sender){
			toadd->SetClientID(Sender->GetIP());
			toadd->SetClientPort(Sender->GetUserPort());
			toadd->SetClientServerIP(Sender->GetServerIP());
			toadd->SetClientServerPort(Sender->GetServerPort());
			if (Sender->GetServerIP() && Sender->GetServerPort()){
				CSearchFile::SServer server(Sender->GetServerIP(), Sender->GetServerPort());
				server.m_uAvail = 1;
				toadd->AddServer(server);
			}			
			toadd->SetPreviewPossible( Sender->GetPreviewSupport() && ED2KFT_VIDEO == GetED2KFileTypeID(toadd->GetFileName()) );
			//Morph Start - added by AndCycle, SLUGFILLER: searchCatch,itsonlyme: cacheUDPsearchResults
			// SLUGFILLER: searchCatch
			CPartFile *file = theApp.downloadqueue->GetFileByID(toadd->GetFileHash());
			if (file){
				if (toadd->GetClientID() && toadd->GetClientPort()){
					// pre-filter sources which would be dropped in CPartFile::AddSources
					if (CPartFile::CanAddSource(toadd->GetClientID(), toadd->GetClientPort(), toadd->GetClientServerIP(), toadd->GetClientServerPort())){
						CSafeMemFile sources(1+4+2);
						uint8 uSources = 1;
						uint32 nIP = toadd->GetClientID();
						uint16 nPort = toadd->GetClientPort();
						sources.Write(&uSources, 1);
						sources.Write(&nIP, 4);
						sources.Write(&nPort, 2);
						sources.SeekToBegin();
						file->AddSources(&sources,toadd->GetClientServerIP(),toadd->GetClientServerPort());
					}
				}
				// itsonlyme: cacheUDPsearchResults
				CPartFile::SServer tmpServer(toadd->GetClientServerIP(), toadd->GetClientServerPort());
				tmpServer.m_uAvail = toadd->GetIntTagValue(FT_SOURCES);
				file->AddAvailServer(tmpServer);
				AddDebugLogLine(false, _T("Caching server with %i sources for %s"), toadd->GetIntTagValue(FT_SOURCES), file->GetFileName());
				// itsonlyme: cacheUDPsearchResults
			}
			// SLUGFILLER: searchCatch
			//Morph End - added by AndCycle, SLUGFILLER: searchCatch,itsonlyme: cacheUDPsearchResults
		}
		AddToList(toadd, true);
	}
	
	if (pbMoreResultsAvailable)
		*pbMoreResultsAvailable = false;
	int iAddData = (int)(packet.GetLength() - packet.GetPosition());
	if (iAddData == 1){
		uint8 ucMore = packet.ReadUInt8();
		if (ucMore == 0x00 || ucMore == 0x01){
			if (pbMoreResultsAvailable)
				*pbMoreResultsAvailable = (bool)ucMore;
			if (thePrefs.GetDebugClientTCPLevel() > 0)
				Debug(_T("  Client search answer(%s): More=%u\n"), Sender->GetUserName(), ucMore);
		}
		else{
			if (thePrefs.GetDebugClientTCPLevel() > 0)
				Debug(_T("*** NOTE: Client ProcessSearchanswer(%s): ***AddData: 1 byte: 0x%02x\n"), Sender->GetUserName(), ucMore);
		}
	}
	else if (iAddData > 0){
		if (thePrefs.GetDebugClientTCPLevel() > 0){
			Debug(_T("*** NOTE: Client ProcessSearchanswer(%s): ***AddData: %u bytes\n"), Sender->GetUserName(), iAddData);
			DebugHexDump((uint8*)in_packet + packet.GetPosition(), iAddData);
		}
	}

	packet.Close();
	return GetResultCount(nSearchID);
}

uint16 CSearchList::ProcessSearchanswer(char* in_packet, uint32 size, bool bOptUTF8,
										uint32 nServerIP, uint16 nServerPort, bool* pbMoreResultsAvailable)
{
	CSafeMemFile packet((BYTE*)in_packet,size);
	UINT results = packet.ReadUInt32();
	for (UINT i = 0; i < results; i++){
		CSearchFile* toadd = new CSearchFile(&packet, bOptUTF8, m_nCurrentSearch);
		toadd->SetClientServerIP(nServerIP);
		toadd->SetClientServerPort(nServerPort);
		if (nServerIP && nServerPort){
			CSearchFile::SServer server(nServerIP, nServerPort);
			server.m_uAvail = toadd->GetIntTagValue(FT_SOURCES);
			toadd->AddServer(server);
		}
		AddToList(toadd, false);
	}
	if (m_MobilMuleSearch)
		theApp.mmserver->SearchFinished(false);
	m_MobilMuleSearch = false;

	if (pbMoreResultsAvailable)
		*pbMoreResultsAvailable = false;
	int iAddData = (int)(packet.GetLength() - packet.GetPosition());
	if (iAddData == 1){
		uint8 ucMore = packet.ReadUInt8();
		if (ucMore == 0x00 || ucMore == 0x01){
			if (pbMoreResultsAvailable)
				*pbMoreResultsAvailable = (bool)ucMore;
			if (thePrefs.GetDebugServerTCPLevel() > 0)
				Debug(_T("  Search answer(Server %s:%u): More=%u\n"), ipstr(nServerIP), nServerPort, ucMore);
		}
		else{
			if (thePrefs.GetDebugServerTCPLevel() > 0)
				Debug(_T("*** NOTE: ProcessSearchanswer(Server %s:%u): ***AddData: 1 byte: 0x%02x\n"), ipstr(nServerIP), nServerPort, ucMore);
		}
	}
	else if (iAddData > 0){
		if (thePrefs.GetDebugServerTCPLevel() > 0){
			Debug(_T("*** NOTE: ProcessSearchanswer(Server %s:%u): ***AddData: %u bytes\n"), ipstr(nServerIP), nServerPort, iAddData);
			DebugHexDump((uint8*)in_packet + packet.GetPosition(), iAddData);
		}
	}

	packet.Close();
	return GetResultCount();
}

uint16 CSearchList::ProcessUDPSearchanswer(CFileDataIO& packet, bool bOptUTF8, uint32 nServerIP, uint16 nServerPort)
{
	CSearchFile* toadd = new CSearchFile(&packet, bOptUTF8, m_nCurrentSearch, nServerIP, nServerPort);
	AddToList(toadd);
	return GetResultCount();
}

uint16 CSearchList::GetResultCount(uint32 nSearchID) const
{
	uint16 nSources = 0;
	VERIFY( m_foundSourcesCount.Lookup(nSearchID, nSources) );
	return nSources;
}

uint16 CSearchList::GetResultCount() const {
	return GetResultCount(m_nCurrentSearch);
}

CString CSearchList::GetWebList(CString linePattern,int sortby,bool asc) const {
	CString buffer;
	CString temp;
	CArray<CSearchFile*, CSearchFile*> sortarray;
	int swap = 0;
	bool inserted;

	// insertsort
	CSearchFile* sf1;
	CSearchFile* sf2;
	for (POSITION pos = list.GetHeadPosition(); pos !=0;) {
		inserted=false;
		sf1 = list.GetNext(pos);
		
		if (sf1->GetListParent()!=NULL) continue;
		
		for (uint16 i1=0;i1<sortarray.GetCount();++i1) {
			sf2 = sortarray.GetAt(i1);
			
			switch (sortby) {
				case 0: swap=sf1->GetFileName().CompareNoCase(sf2->GetFileName()); break;
				case 1: swap=sf1->GetFileSize()-sf2->GetFileSize();break;
				case 2: swap=CString(sf1->GetFileHash()).CompareNoCase(CString(sf2->GetFileHash())); break;
				case 3: swap=sf1->GetSourceCount()-sf2->GetSourceCount(); break;
			}
			if (!asc) swap=0-swap;
			if (swap<0) {inserted=true; sortarray.InsertAt(i1,sf1);break;}
		}
		if (!inserted) sortarray.Add(sf1);
	}
	
	for (uint16 i=0;i<sortarray.GetCount();++i) {
		const CSearchFile* sf = sortarray.GetAt(i);

		// colorize
		CString coloraddon;
		CString coloraddonE;
		CKnownFile* sameFile = theApp.sharedfiles->GetFileByID(sf->GetFileHash());
		if (!sameFile)
			sameFile = theApp.downloadqueue->GetFileByID(sf->GetFileHash());

		if (sameFile) {
			if (sameFile->IsPartFile())
				coloraddon = _T("<font color=\"#FF0000\">");
			else
				coloraddon = _T("<font color=\"#00FF00\">");
		}
		if (coloraddon.GetLength()>0)
			coloraddonE = _T("</font>");

		CString strHash(EncodeBase16(sf->GetFileHash(),16));
		temp.Format(linePattern,
					coloraddon + StringLimit(sf->GetFileName(),70) + coloraddonE,
					CastItoXBytes(sf->GetFileSize(), false, false),
					strHash,
					sf->GetSourceCount(),
					strHash);
		buffer.Append(temp);
	}
	return buffer;
}

void CSearchList::AddFileToDownloadByHash(const uchar* hash, uint8 cat)
{
	for (POSITION pos = list.GetHeadPosition(); pos != 0; )
	{
		CSearchFile* sf = list.GetNext(pos);
		if (!md4cmp(hash, sf->GetFileHash()))
		{
			theApp.downloadqueue->AddSearchToDownload(sf,2,cat);
			break;
		}
	}
}

// mobilemule

CSearchFile* CSearchList::DetachNextFile(uint32 nSearchID) {
	// the files are NOT deleted, make sure you do this if you call this function
	// find, removes and returns the searchresult with most Sources	
	uint32 nHighSource = 0;
	POSITION resultpos = 0;
	for (POSITION pos = list.GetHeadPosition(); pos != NULL; ){
		POSITION cur_pos = pos;
		CSearchFile* cur_file = list.GetNext(pos);
		if( cur_file->GetSearchID() == nSearchID ){
			if(cur_file->GetIntTagValue(FT_SOURCES) >= nHighSource){
				nHighSource = cur_file->GetIntTagValue(FT_SOURCES);
				resultpos = cur_pos;
			}
		}
	}
	if (resultpos == 0){
		ASSERT ( false );
		return NULL;
	}
	CSearchFile* result = list.GetAt(resultpos);
	list.RemoveAt(resultpos);
	return result;
}

bool CSearchList::AddToList(CSearchFile* toadd, bool bClientResponse)
{
	//Morph Start - added by AndCycle, SLUGFILLER: searchCatch,itsonlyme: cacheUDPsearchResults
	// SLUGFILLER: searchCatch
	CPartFile *file = theApp.downloadqueue->GetFileByID(toadd->GetFileHash());
	if (file){
		if (toadd->GetClientID() && toadd->GetClientPort()){
			// pre-filter sources which would be dropped in CPartFile::AddSources
			if (CPartFile::CanAddSource(toadd->GetClientID(), toadd->GetClientPort(), toadd->GetClientServerIP(), toadd->GetClientServerPort())){
				CSafeMemFile sources(1+4+2);
				uint8 uSources = 1;
				uint32 nIP = toadd->GetClientID();
				uint16 nPort = toadd->GetClientPort();
				sources.Write(&uSources, 1);
				sources.Write(&nIP, 4);
				sources.Write(&nPort, 2);
				sources.SeekToBegin();
				file->AddSources(&sources,toadd->GetClientServerIP(),toadd->GetClientServerPort());
			}
		}
		// itsonlyme: cacheUDPsearchResults
		CPartFile::SServer tmpServer(toadd->GetClientServerIP(), toadd->GetClientServerPort());
		tmpServer.m_uAvail = toadd->GetIntTagValue(FT_SOURCES);
		file->AddAvailServer(tmpServer);
		AddDebugLogLine(false, _T("Caching server with %i sources for %s"), toadd->GetIntTagValue(FT_SOURCES), file->GetFileName());
		// itsonlyme: cacheUDPsearchResults
	}
	// SLUGFILLER: searchCatch
	//Morph End - added by AndCycle, SLUGFILLER: searchCatch,itsonlyme: cacheUDPsearchResults
	if (!bClientResponse && !m_strResultFileType.IsEmpty() && _tcscmp(m_strResultFileType, toadd->GetFileType()) != 0)
	{
		delete toadd;
		return false;
	}

	for (POSITION pos = list.GetHeadPosition(); pos != NULL; )
	{
		CSearchFile* cur_file = list.GetNext(pos);
		if ( (!md4cmp(toadd->GetFileHash(),cur_file->GetFileHash())) 
			&& cur_file->GetSearchID() ==  toadd->GetSearchID()
			&& cur_file->GetListParent()==NULL
			)
		{
			UINT uAvail;
			if (bClientResponse)
			{
				// If this is a response from a client ("View Shared Files"), we set the "Availability" at least to 1.
				if (!toadd->GetIntTagValue(FT_SOURCES, uAvail) || uAvail==0)
					uAvail = 1;
			}
			else
				uAvail = toadd->GetIntTagValue(FT_SOURCES);
			
			// already a result in list (parent exists)
			cur_file->AddSources(uAvail);
			AddResultCount(cur_file->GetSearchID(), toadd->GetFileHash(), uAvail);

			uint32 uCompleteSources = -1;
			if (toadd->GetIntTagValue(FT_COMPLETE_SOURCES, uCompleteSources))
				cur_file->AddCompleteSources(uCompleteSources);

			// check if child with same filename exists
			bool found = false;
			for (POSITION pos2 = list.GetHeadPosition(); pos2 != NULL && !found ; )
			{
				CSearchFile* cur_file2 = list.GetNext(pos2);
				if ( cur_file2!=toadd												// not the same object
					 && cur_file2->GetListParent()==cur_file								// is a child of our result (one filehash)
					 && !toadd->GetFileName().CompareNoCase(cur_file2->GetFileName()))	// same name
				{
					if( toadd->IsKademlia() )
					{
						if (uAvail == cur_file2->GetIntTagValue(FT_SOURCES))
						{
							delete toadd;
							return false;
						}
					}
					else
					{
						// yes: add the counter of that child
						found=true;
						cur_file2->SetListAddChildCount(uAvail);
						cur_file2->AddSources(uAvail);
						if (uCompleteSources != -1)
							cur_file2->AddCompleteSources(uCompleteSources);
	
						// copy servers to child item -- this is not really needed because the servers are also stored
						// in the parent item, but it gives a correct view of the server list within the server property page
						for (int s = 0; s < toadd->GetServers().GetSize(); s++)
						{
							CSearchFile::SServer server = toadd->GetServerAt(s);
							int iFound = cur_file2->GetServers().Find(server);
							if (iFound == -1)
								cur_file2->AddServer(server);
							else
								cur_file2->GetServerAt(iFound).m_uAvail += server.m_uAvail;
						}
						break;
					}
				}
			}
			if (!found) 
			{
				// no:  add child
				toadd->SetListParent(cur_file);
				toadd->SetListChildCount(uAvail);
				cur_file->SetListAddChildCount(1);
				list.AddHead(toadd);
			}
			outputwnd->UpdateSources(cur_file);

			if ((toadd->GetClientID() && toadd->GetClientPort()))
			{
				if (IsValidClientIPPort(toadd->GetClientID(), toadd->GetClientPort()))
				{
					// pre-filter sources which would be dropped in CPartFile::AddSources
					if (CPartFile::CanAddSource(toadd->GetClientID(), toadd->GetClientPort(), toadd->GetClientServerIP(), toadd->GetClientServerPort()))
					{
						CSearchFile::SClient client(toadd->GetClientID(), toadd->GetClientPort(),
													toadd->GetClientServerIP(), toadd->GetClientServerPort());
						if (cur_file->GetClients().Find(client) == -1)
							cur_file->AddClient(client);
					}
				}
				else
				{
					if (thePrefs.GetDebugServerSearchesLevel() > 1)
					{
						uint32 nIP = toadd->GetClientID();
						Debug(_T("Filtered source from search result %s:%u\n"), DbgGetClientID(nIP), toadd->GetClientPort());
					}
				}

			}
			// will be used in future
			if (toadd->GetClientServerIP() && toadd->GetClientServerPort())
			{
				CSearchFile::SServer server(toadd->GetClientServerIP(), toadd->GetClientServerPort());
				int iFound = cur_file->GetServers().Find(server);
				if (iFound == -1)
				{
					server.m_uAvail = uAvail;
					cur_file->AddServer(server);
				}
				else
					cur_file->GetServerAt(iFound).m_uAvail += uAvail;
			}
			if (outputwnd && !m_MobilMuleSearch)
				outputwnd->UpdateSources(cur_file);

			if (found)
				delete toadd;
			return true;
		}
	}
	
	// no bounded result found yet -> add as parent to list
	toadd->SetListParent(NULL);
	if (list.AddTail(toadd))
	{
		uint16 tempValue = 0;
		VERIFY( m_foundFilesCount.Lookup(toadd->GetSearchID(),tempValue) );
		m_foundFilesCount.SetAt(toadd->GetSearchID(),tempValue+1);

		// new search result entry (no parent); add to result count for search result limit
		UINT uAvail;
		if (bClientResponse)
		{
			// If this is a response from a client ("View Shared Files"), we set the "Availability" at least to 1.
			if (!toadd->GetIntTagValue(FT_SOURCES, uAvail) || uAvail==0)
				uAvail = 1;
			toadd->AddSources(uAvail);
		}
		else
			uAvail = toadd->GetIntTagValue(FT_SOURCES);
		AddResultCount(toadd->GetSearchID(), toadd->GetFileHash(), uAvail);

		CSearchFile* neu = new CSearchFile(toadd);
		neu->SetListParent(toadd);
		neu->SetListChildCount(uAvail);
		list.AddTail(neu);
		toadd->SetListChildCount(1);
	}
	if (outputwnd && !m_MobilMuleSearch)
		outputwnd->AddResult(toadd);
	return true;
}

CSearchFile* CSearchList::GetSearchFileByHash(const uchar* hash) const
{
	for (POSITION pos = list.GetHeadPosition(); pos != 0; )
	{
		CSearchFile* sf = list.GetNext(pos);
		if (!md4cmp(hash, sf->GetFileHash()))
			return sf;
	}
	return NULL;
}

void CSearchList::AddResultCount(uint32 nSearchID, const uchar* hash, UINT nCount)
{
	// do not count already available or downloading files for the search result limit
	if (theApp.sharedfiles->GetFileByID(hash) || theApp.downloadqueue->GetFileByID(hash))
		return;

	uint16 tempValue = 0;
	VERIFY( m_foundSourcesCount.Lookup(nSearchID, tempValue) );
	m_foundSourcesCount.SetAt(nSearchID, tempValue + nCount);
}

void CSearchList::KademliaSearchKeyword(uint32 searchID, const Kademlia::CUInt128* fileID, 
										LPCTSTR name, uint32 size, LPCTSTR type, uint16 numProperties, ...)
{
	va_list args;
	va_start(args, numProperties);

#ifdef _UNICODE
	EUtf8Str eStrEncode = utf8strRaw;
#else
	EUtf8Str eStrEncode = utf8strNone;
#endif
	CSafeMemFile* temp = new CSafeMemFile(250);
	uchar fileid[16];
	fileID->toByteArray(fileid);
	temp->WriteHash16(fileid);
	
	temp->WriteUInt32(0);	// client IP
	temp->WriteUInt16(0);	// client port
	
	// write tag list
	UINT uFilePosTagCount = temp->GetPosition();
	uint32 tagcount = 0;
	temp->WriteUInt32(tagcount); // dummy tag count, will be filled later

	// standard tags
	CTag tagName(FT_FILENAME, name);
	tagName.WriteTagToFile(temp, eStrEncode);
	tagcount++;

	CTag tagSize(FT_FILESIZE, size);
	tagSize.WriteTagToFile(temp, eStrEncode);
	tagcount++;

	if (type != NULL && type[0] != _T('\0'))
	{
		CTag tagType(FT_FILETYPE, type);
		tagType.WriteTagToFile(temp, eStrEncode);
		tagcount++;
	}

	// additional tags
	while (numProperties-- > 0)
	{
		UINT uPropType = va_arg(args, UINT);
		LPCSTR pszPropName = va_arg(args, LPCSTR);
		LPVOID pvPropValue = va_arg(args, LPVOID);
		if (uPropType == 2 /*TAGTYPE_STRING*/)
		{
			if ((LPCTSTR)pvPropValue != NULL && ((LPCTSTR)pvPropValue)[0] != _T('\0'))
			{
				if (strlen(pszPropName) == 1)
				{
					CTag tagProp((uint8)*pszPropName, (LPCTSTR)pvPropValue);
					tagProp.WriteTagToFile(temp, eStrEncode);
				}
				else
				{
					CTag tagProp(pszPropName, (LPCTSTR)pvPropValue);
					tagProp.WriteTagToFile(temp, eStrEncode);
				}
				tagcount++;
			}
		}
		else if (uPropType == 3 /*TAGTYPE_UINT32*/)
		{
			if ((uint32)pvPropValue != 0)
			{
				CTag tagProp(pszPropName, (uint32)pvPropValue);
				tagProp.WriteTagToFile(temp, eStrEncode);
			tagcount++;
		}
		}
		else
		{
			ASSERT(0);
		}
	}
	va_end(args);
	temp->Seek(uFilePosTagCount, SEEK_SET);
	temp->WriteUInt32(tagcount);
	
	temp->SeekToBegin();
	CSearchFile* tempFile = new CSearchFile(temp, eStrEncode == utf8strRaw, searchID, 0, 0, 0, true);
	AddToList(tempFile);
	
	delete temp;
}

//MORPH START - Added by SiRoB / Commander, Wapserver [emulEspaña]
CString CSearchList::GetWapList(CString linePattern,int sortby,bool asc, int start, int max, bool &more) const {
	CString buffer;
	CString temp;
	CArray<CSearchFile*, CSearchFile*> sortarray;
	int swap;
	bool inserted;

	// insertsort
	CSearchFile* sf1;
	CSearchFile* sf2;
	for (POSITION pos = list.GetHeadPosition(); pos !=0;) {
		inserted=false;
		sf1 = list.GetNext(pos);
		
		if (sf1->GetListParent()!=NULL) continue;		
		
		for (uint16 i1=0;i1<sortarray.GetCount();++i1) {
			sf2 = sortarray.GetAt(i1);
			
			switch (sortby) {
				case 0: swap=sf1->GetFileName().CompareNoCase(sf2->GetFileName()); break;
				case 1: swap=sf1->GetFileSize()-sf2->GetFileSize();break;
				case 2: swap=CString(sf1->GetFileHash()).CompareNoCase(CString(sf2->GetFileHash())); break;
				case 3: swap=sf1->GetSourceCount()-sf2->GetSourceCount(); break;
			}
			if (!asc) swap=0-swap;
			if (swap<0) {inserted=true; sortarray.InsertAt(i1,sf1);break;}
		}
		if (!inserted) sortarray.Add(sf1);
	}
	
	int endpos;

	endpos = start + max;

	if (endpos>sortarray.GetCount())
		endpos=sortarray.GetCount();

	if (start>sortarray.GetCount()){
		start=endpos-max;
		if (start<0) start=0;
	}

	for (uint16 i=start;i<endpos;++i) {
		const CSearchFile* sf = sortarray.GetAt(i);

		// colorize
		CString coloraddon;
		CString coloraddonE;
		CKnownFile* sameFile = theApp.sharedfiles->GetFileByID(sf->GetFileHash());
		if (!sameFile)
			sameFile = theApp.downloadqueue->GetFileByID(sf->GetFileHash());

		if (sameFile) {
			if (sameFile->IsPartFile())
				coloraddon = _T("<b>");
			else
				coloraddon = _T("");
		}
		if (coloraddon.GetLength()>0)
			coloraddonE = _T("</b>");

		CString strHash(EncodeBase16(sf->GetFileHash(),16));
		temp.Format(linePattern,
					coloraddon + StringLimit(sf->GetFileName(),70) + coloraddonE,
					CastItoXBytes(sf->GetFileSize()),
					strHash,
					sf->GetSourceCount(),
					strHash);

		buffer.Append(temp);
	}
	
	if(endpos<sortarray.GetCount())
		more=true;
	else
		more=false;

	return buffer;
}
//MORPH END - Added by SiRoB / Commander, Wapserver [emulEspaña]