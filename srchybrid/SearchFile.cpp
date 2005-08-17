// parts of this file are based on work from pan One (http://home-3.tiscali.nl/~meost/pms/)
//this file is part of eMule
//Copyright (C)2002-2004 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "SearchFile.h"
#include "OtherFunctions.h"
#include "opcodes.h"
#include "Packets.h"
#include "Preferences.h"
#include "CxImage/xImage.h"
#include "Kademlia/Kademlia/Entry.h"
#include "emule.h"
#include "emuledlg.h"
#include "Searchdlg.h"
#include "Fakecheck.h" //MORPH - Added by SiRoB

#ifdef _DEBUG
#include "DebugHelpers.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


bool IsValidSearchResultClientIPPort(uint32 nIP, uint16 nPort)
{
	return	   nIP != 0
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

IMPLEMENT_DYNAMIC(CSearchFile, CAbstractFile)

CSearchFile::CSearchFile(const CSearchFile* copyfrom) : CAbstractFile(copyfrom)
{
	UpdateFileRatingCommentAvail();

	m_nClientServerIP = copyfrom->GetClientServerIP();
	m_nClientServerPort = copyfrom->GetClientServerPort();
	m_nClientID = copyfrom->GetClientID();
	m_nClientPort = copyfrom->GetClientPort();
	m_pszDirectory = copyfrom->GetDirectory()? _tcsdup(copyfrom->GetDirectory()) : NULL;
	m_pszIsFake = copyfrom->GetFakeComment()? _tcsdup(copyfrom->GetFakeComment()) : NULL; //MORPH - Added by SiRoB, FakeCheck, FakeReport, Auto-updating
	m_nSearchID = copyfrom->GetSearchID();
	m_nKademlia = copyfrom->IsKademlia();
	
	const CSimpleArray<SClient>& clients = copyfrom->GetClients();
	for (int i = 0; i < clients.GetSize(); i++)
		AddClient(clients[i]);
	
	const CSimpleArray<SServer>& servers = copyfrom->GetServers();
	for (int i = 0; i < servers.GetSize(); i++)
		AddServer(servers[i]);

	m_list_bExpanded = false;
	m_list_parent = const_cast<CSearchFile*>(copyfrom);
	m_list_childcount = 0;
	m_bPreviewPossible = false;
	m_eKnown = copyfrom->m_eKnown;
}

CSearchFile::CSearchFile(CFileDataIO* in_data, bool bOptUTF8, 
						 uint32 nSearchID, uint32 nServerIP, uint16 nServerPort, LPCTSTR pszDirectory, bool nKademlia)
{
	m_nKademlia = nKademlia;
	m_nSearchID = nSearchID;
	in_data->ReadHash16(m_abyFileHash);
	m_nClientID = in_data->ReadUInt32();
	m_nClientPort = in_data->ReadUInt16();
	if ((m_nClientID || m_nClientPort) && !IsValidSearchResultClientIPPort(m_nClientID, m_nClientPort)){
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
	if (m_pszDirectory)
		free(m_pszDirectory);
	//MORPH START - Added by SiRoB, FakeCheck, FakeReport, Auto-updating
	if (m_pszIsFake)
		free(m_pszIsFake);
	//MORPH END   - Added by SiRoB, FakeCheck, FakeReport, Auto-updating
	for (int i = 0; i < m_listImages.GetSize(); i++)
		delete m_listImages[i];
}

void CSearchFile::UpdateFileRatingCommentAvail()
{
	bool bOldHasComment = m_bHasComment;
	uint32 uOldUserRatings = m_uUserRating;

	m_bHasComment = false;
	uint32 uRatings = 0;
	uint32 uUserRatings = 0;

	for(POSITION pos = m_kadNotes.GetHeadPosition(); pos != NULL; )
	{
		Kademlia::CEntry* entry = m_kadNotes.GetNext(pos);
		if (!m_bHasComment && !entry->GetStrTagValue(TAG_DESCRIPTION).IsEmpty())
			m_bHasComment = true;
		uint16 rating = entry->GetIntTagValue(TAG_FILERATING);
		if(rating!=0)
		{
			uRatings++;
			uUserRatings += rating;
		}
	}

	if(uRatings)
		m_uUserRating = uUserRatings / uRatings;
	else
		m_uUserRating = 0;

	if (bOldHasComment != m_bHasComment || uOldUserRatings != m_uUserRating)
		theApp.emuledlg->searchwnd->UpdateSearch(this);
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
				if (count > pTag->GetInt())
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
				if (count > pTag->GetInt())
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
