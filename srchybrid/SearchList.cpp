//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

#include "StdAfx.h"
#include "SearchList.h"
#include "SearchDlg.h"
#include "CxImage/xImage.h"

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
	if (pTag->tag.specialtag == 0 && pTag->tag.tagname != NULL)
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
			{ FT_MEDIA_ARTIST,  2, FT_ED2K_MEDIA_ARTIST },
			{ FT_MEDIA_ALBUM,   2, FT_ED2K_MEDIA_ALBUM },
			{ FT_MEDIA_TITLE,   2, FT_ED2K_MEDIA_TITLE },
			{ FT_MEDIA_LENGTH,  2, FT_ED2K_MEDIA_LENGTH },
			{ FT_MEDIA_BITRATE, 3, FT_ED2K_MEDIA_BITRATE },
			{ FT_MEDIA_CODEC,   2, FT_ED2K_MEDIA_CODEC }
		};

		for (int j = 0; j < ARRSIZE(_aEmuleToED2KMetaTagsMap); j++)
		{
			if (stricmp(pTag->tag.tagname, _aEmuleToED2KMetaTagsMap[j].pszED2KName) == 0)
			{
				if (pTag->tag.type == _aEmuleToED2KMetaTagsMap[j].nED2KType)
				{
					if (pTag->tag.type == 2)
					{
						if (_aEmuleToED2KMetaTagsMap[j].nID == FT_MEDIA_LENGTH)
						{
							UINT nMediaLength = 0;
							UINT hour = 0, min = 0, sec = 0;
							if (sscanf(pTag->tag.stringvalue, "%u : %u : %u", &hour, &min, &sec) == 3)
								nMediaLength = hour * 3600 + min * 60 + sec;
							else if (sscanf(pTag->tag.stringvalue, "%u : %u", &min, &sec) == 2)
								nMediaLength = min * 60 + sec;
							else if (sscanf(pTag->tag.stringvalue, "%u", &sec) == 1)
								nMediaLength = sec;

							CTag* tag = (nMediaLength != 0) ? new CTag(_aEmuleToED2KMetaTagsMap[j].nID, nMediaLength) : NULL;
							delete pTag;
							pTag = tag;
						}
						else
						{
							CTag* tag = (pTag->tag.stringvalue[0] != '\0') 
										  ? new CTag(_aEmuleToED2KMetaTagsMap[j].nID, pTag->tag.stringvalue) 
										  : NULL;
							delete pTag;
							pTag = tag;
						}
					}
					else if (pTag->tag.type == 3)
					{
						CTag* tag = (pTag->tag.intvalue != 0) 
									  ? new CTag(_aEmuleToED2KMetaTagsMap[j].nID, pTag->tag.intvalue) 
									  : NULL;
						delete pTag;
						pTag = tag;
					}
				}
				break;
			}
		}
	}
}


///////////////////////////////////////////////////////////////////////////////
// CSearchFile
	
CSearchFile::CSearchFile(CSearchFile* copyfrom)
{
	MD4COPY(m_abyFileHash, copyfrom->GetFileHash());
	SetFileSize(copyfrom->GetIntTagValue(FT_FILESIZE));
	SetFileName(copyfrom->GetStrTagValue(FT_FILENAME));
	m_nClientServerIP = copyfrom->GetClientServerIP();
	m_nClientID = copyfrom->GetClientID();
	m_nClientPort = copyfrom->GetClientPort();
	m_nClientServerPort = copyfrom->GetClientServerPort();
	m_pszDirectory = copyfrom->GetDirectory()? nstrdup(copyfrom->GetDirectory()) : NULL;
	m_nSearchID = copyfrom->GetSearchID();
	m_nKademlia = copyfrom->IsKademlia();
	for (int i = 0; i < copyfrom->GetTags().GetCount(); i++)
		taglist.Add(new CTag(*copyfrom->GetTags().GetAt(i)));
	for (i = 0; i < copyfrom->GetServers().GetSize(); i++){
		SServer server = copyfrom->GetServer(i);
		AddServer(server);
	}

	m_list_bExpanded = false;
	m_list_parent = copyfrom;
	m_list_childcount = 0;
	m_bPreviewPossible = false;
}

CSearchFile::CSearchFile(CFile* in_data, uint32 nSearchID, uint32 nServerIP, uint16 nServerPort, LPCTSTR pszDirectory, bool nKademlia)
{
	m_nKademlia = nKademlia;
	m_nSearchID = nSearchID;
	in_data->Read(&m_abyFileHash,16);
	in_data->Read(&m_nClientID,4);
	in_data->Read(&m_nClientPort,2);
	if ((m_nClientID || m_nClientPort) && !IsValidClientIPPort(m_nClientID, m_nClientPort)){
		if (theApp.glob_prefs->GetDebugServerSearches() > 1)
			Debug("Filtered source from search result %s:%u\n", DbgGetClientID(m_nClientID), m_nClientPort);
		m_nClientID = 0;
		m_nClientPort = 0;
	}
	uint32 tagcount;
	in_data->Read(&tagcount,4);
	// NSERVER2.EXE (lugdunum v16.38 patched for Win32) returns the ClientIP+Port of the client which offered that
	// file, even if that client has not filled the according fields in the OP_OFFERFILES packet with its IP+Port.
	//
	// 16.38.p73 (lugdunum) (propenprinz)
	//  *) does not return ClientIP+Port if the OP_OFFERFILES packet does not also contain it.
	//  *) if the OP_OFFERFILES packet does contain our HighID and Port the server returns that data at least when
	//     returning search results via TCP.
	if (theApp.glob_prefs->GetDebugServerSearches() > 1)
		Debug("Search Result: %s  Client=%u.%u.%u.%u:%u  Tags=%u\n", md4str(m_abyFileHash), (uint8)m_nClientID,(uint8)(m_nClientID>>8),(uint8)(m_nClientID>>16),(uint8)(m_nClientID>>24), m_nClientPort, tagcount);

	for (int i = 0;i != tagcount; i++){
		CTag* toadd = new CTag(in_data);
		if (theApp.glob_prefs->GetDebugServerSearches() > 1)
			Debug("  %s\n", toadd->GetFullInfo());
		ConvertED2KTag(toadd);
		if (toadd)
		taglist.Add(toadd);
	}

	// here we have two choices
	//	- if the server/client sent us a filetype, we could use it (though it could be wrong)
	//	- we always trust our filetype list and determine the filetype by the extension of the file
	SetFileName(GetStrTagValue(FT_FILENAME));
	SetFileSize(GetIntTagValue(FT_FILESIZE));
	m_nClientServerIP = nServerIP;
	m_nClientServerPort = nServerPort;
	if (m_nClientServerIP && m_nClientServerPort){
		SServer server(m_nClientServerIP, m_nClientServerPort);
		server.m_uAvail = GetIntTagValue(FT_SOURCES);
		AddServer(server);
	}
	m_pszDirectory = pszDirectory ? nstrdup(pszDirectory) : NULL;
	
	m_list_bExpanded = false;
	m_list_parent = NULL;
	m_list_childcount = 0;
	m_bPreviewPossible = false;
}

CSearchFile::CSearchFile(uint32 nSearchID, const uchar* pucFileHash, uint32 uFileSize, LPCTSTR pszFileName, int iFileType, int iAvailability)
{
	m_nSearchID = nSearchID;
	MD4COPY(m_abyFileHash, pucFileHash);
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
	m_list_bExpanded = false;
	m_list_parent = NULL;
	m_list_childcount = 0;
	m_bPreviewPossible = false;
}

CSearchFile::~CSearchFile(){
	for (int i = 0; i < taglist.GetSize();i++)
		safe_delete(taglist[i]);
	taglist.RemoveAll();
	taglist.SetSize(0);
	delete[] m_pszDirectory;
	for (int i = 0; i != m_listImages.GetSize(); i++)
		safe_delete(m_listImages[i]);
}

uint32 CSearchFile::AddSources(uint32 count){
	for (int i = 0; i < taglist.GetSize(); i++){
		CTag* pTag = taglist[i];
		if (pTag->tag.specialtag == FT_SOURCES){
			if(m_nKademlia){
				if( pTag->tag.intvalue < count)
					pTag->tag.intvalue = count;
			}
			else
			pTag->tag.intvalue += count;
			return pTag->tag.intvalue;
		}
	}
	return 0;
}

uint32 CSearchFile::GetSourceCount() const {
	return GetIntTagValue(FT_SOURCES);
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

void CSearchList::RemoveResults( uint32 nSearchID){
	// this will not delete the item from the window, make sure your code does it if you call this
	ASSERT( outputwnd );
	for (POSITION pos = list.GetHeadPosition(); pos != NULL;){
		POSITION posLast = pos;
		CSearchFile* cur_file =	list.GetNext(pos);
		if( cur_file->GetSearchID() == nSearchID ){
			list.RemoveAt(posLast);
			delete cur_file;
		}
	}
}

void CSearchList::ShowResults( uint32 nSearchID){
	ASSERT( outputwnd );
	outputwnd->SetRedraw(false);
	for (POSITION pos = list.GetHeadPosition(); pos != 0;){
		CSearchFile* cur_file = list.GetNext(pos);
		if( cur_file->GetSearchID() == nSearchID && cur_file->GetListParent()==NULL)
			outputwnd->AddResult(cur_file);
	}
	outputwnd->SetRedraw(true);
}

void CSearchList::RemoveResults( CSearchFile* todel ){
	for (POSITION pos = list.GetHeadPosition(); pos !=0;){
		POSITION posLast = pos;
		CSearchFile* cur_file = list.GetNext(pos);
		if( cur_file == todel ){
			theApp.emuledlg->searchwnd->searchlistctrl.RemoveResult( todel );
			list.RemoveAt(posLast);
			delete todel;
			return;
		}
	}
}

void CSearchList::NewSearch(CSearchListCtrl* in_wnd, CString resTypes, uint32 nSearchID, bool MobilMuleSearch){
	if(in_wnd)
		outputwnd = in_wnd;

	m_strResultType = resTypes;
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
	if (theApp.emuledlg->searchwnd->CreateNewTab(pParams)){
		m_foundFilesCount.SetAt(nSearchID,0);
		m_foundSourcesCount.SetAt(nSearchID,0);
	}
	else{
		delete pParams;
		pParams = NULL;
	}

	CSafeMemFile packet((BYTE*)in_packet,size);
	uint32 results;
	packet.Read(&results,4);

	for (int i = 0; i != results; i++){
		CSearchFile* toadd = new CSearchFile(&packet, nSearchID, 0, 0, pszDirectory);
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
			toadd->SetPreviewPossible( Sender->SupportsPreview() && ED2KFT_VIDEO == GetED2KFileTypeID(toadd->GetFileName()) );
		}
		AddToList(toadd, true);
	}
	
	if (pbMoreResultsAvailable)
		*pbMoreResultsAvailable = false;
	int iAddData = (int)(packet.GetLength() - packet.GetPosition());
	if (iAddData == 1){
		uint8 ucMore;
		packet.Read(&ucMore, 1);
		if (ucMore == 0x00 || ucMore == 0x01){
			if (pbMoreResultsAvailable)
				*pbMoreResultsAvailable = (bool)ucMore;
		}
	}

	packet.Close();
	return GetResultCount(nSearchID);
}

uint16 CSearchList::ProcessSearchanswer(char* in_packet, uint32 size, 
										uint32 nServerIP, uint16 nServerPort, bool* pbMoreResultsAvailable)
{
	CSafeMemFile packet((BYTE*)in_packet,size);
	uint32 results;
	packet.Read(&results,4);

	for (int i = 0; i != results; i++){
		CSearchFile* toadd = new CSearchFile(&packet, m_nCurrentSearch);
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
		uint8 ucMore;
		packet.Read(&ucMore, 1);
		if (ucMore == 0x00 || ucMore == 0x01){
			if (pbMoreResultsAvailable)
				*pbMoreResultsAvailable = (bool)ucMore;
			if (theApp.glob_prefs->GetDebugServerTCP())
				Debug("  Search answer(Server %s:%u): More=%u\n", inet_ntoa(*(in_addr*)&nServerIP), nServerPort, ucMore);
		}
		else{
			if (theApp.glob_prefs->GetDebugServerTCP())
				Debug("*** NOTE: ProcessSearchanswer(Server %s:%u): ***AddData: 1 byte: 0x%02x\n", inet_ntoa(*(in_addr*)&nServerIP), nServerPort, ucMore);
		}
	}
	else if (iAddData > 0){
		if (theApp.glob_prefs->GetDebugServerTCP()){
			Debug("*** NOTE: ProcessSearchanswer(Server %s:%u): ***AddData: %u bytes\n", inet_ntoa(*(in_addr*)&nServerIP), nServerPort, iAddData);
			DebugHexDump((uint8*)in_packet + packet.GetPosition(), iAddData);
		}
	}

	packet.Close();
	return GetResultCount();
}

uint16 CSearchList::ProcessUDPSearchanswer(CFile& packet, uint32 nServerIP, uint16 nServerPort)
{
	CSearchFile* toadd = new CSearchFile(&packet, m_nCurrentSearch, nServerIP, nServerPort);
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
					CastItoXBytes(sf->GetFileSize()),
					strHash,
					sf->GetSourceCount(),
					strHash);
		buffer.Append(temp);
	}
	return buffer;
}

void CSearchList::AddFileToDownloadByHash(const uchar* hash,uint8 cat) {
	for (POSITION pos = list.GetHeadPosition(); pos !=0; ){
		CSearchFile* sf=list.GetNext(pos);//->GetSearchID() == nSearchID ){
		if (!md4cmp(hash,sf->GetFileHash())) {
			theApp.downloadqueue->AddSearchToDownload(sf,cat);
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

bool CSearchList::AddToList(CSearchFile* toadd, bool bClientResponse){
	//TODO: Optimize this "GetResString(IDS_SEARCH_ANY)"!!!
	if (!bClientResponse && !(m_strResultType==GetResString(IDS_SEARCH_ANY) || toadd->GetFileType()==m_strResultType))
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
			UINT uAvail = toadd->GetIntTagValue(FT_SOURCES);
			
			// already a result in list (parent exists)
			cur_file->AddSources(uAvail);
			AddResultCount(cur_file->GetSearchID(), toadd->GetFileHash(), uAvail);
			bool found=false;

			// check if child with same filename exists
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
	
						// copy servers to child item -- this is not really needed because the servers are also stored
						// in the parent item, but it gives a correct view of the server list within the server property page
						for (int s = 0; s < toadd->GetServers().GetSize(); s++)
						{
							CSearchFile::SServer server = toadd->GetServer(s);
							int iFound = cur_file2->GetServers().Find(server);
							if (iFound == -1)
								cur_file2->AddServer(server);
							else
								cur_file2->GetServer(iFound).m_uAvail += server.m_uAvail;
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
					if (theApp.glob_prefs->GetDebugServerSearches() > 1)
					{
						uint32 nIP = toadd->GetClientID();
						Debug("Filtered source from search result %s:%u\n", DbgGetClientID(nIP), toadd->GetClientPort());
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
					cur_file->GetServer(iFound).m_uAvail += uAvail;
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
	if (list.AddTail(toadd)) {	
		uint16 tempValue = 0;
		VERIFY( m_foundFilesCount.Lookup(toadd->GetSearchID(),tempValue) );
		m_foundFilesCount.SetAt(toadd->GetSearchID(),tempValue+1);

		// new search result entry (no parent); add to result count for search result limit
		UINT uAvail = toadd->GetIntTagValue(FT_SOURCES);
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

CSearchFile* CSearchList::GetSearchFileByHash(const uchar* hash) {
	for (POSITION pos = list.GetHeadPosition(); pos !=0; ){
		CSearchFile* sf=list.GetNext(pos);//->GetSearchID() == nSearchID ){
		if (!md4cmp(hash,sf->GetFileHash())) {
			return sf;
		}
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