/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#include "stdafx.h"
#include "Search.h"
#include "Kademlia.h"
#include "../../OpCodes.h"
#include "Defines.h"
#include "Prefs.h"
#include "../io/IOException.h"
#include "../routing/RoutingZone.h"
#include "../routing/Contact.h"
#include "../net/KademliaUDPListener.h"
#include "../kademlia/tag.h"
#include "../../emule.h"
#include "../../sharedfilelist.h"
#include "../../otherfunctions.h"
#include "../../emuledlg.h"
#include "../../Packets.h"
#include "../../knownfile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CSearch::CSearch()
{
	m_created = time(NULL);
	m_lastSent = time(NULL);
	m_searchTerms = NULL;
	m_lenSearchTerms = 0;
	m_type = (uint32)-1;
	m_count = 0;
	m_countSent = 0;
	m_callbackID = NULL;
	m_callbackKeyword = NULL;
	m_searchID = (uint32)-1;
	m_keywordPublish = NULL;
	m_fileName = "";
	Kademlia::CKademlia::reportSearchAdd(this);
	bio1 = NULL;
	bio2 = NULL;
	bio3 = NULL;
}

CSearch::~CSearch()
{
	Kademlia::CKademlia::reportSearchRem(this);
	if (m_searchTerms != NULL)
		delete m_searchTerms;
	ContactList::const_iterator it;
	for (it = m_delete.begin(); it != m_delete.end(); it++)
		delete *it;
	Kademlia::CKademlia::reportSearchRem(this);
	if( bio1 )
		delete bio1;
	if( bio2 )
		delete bio2;
	if( bio3 )
		delete bio3;
}

void CSearch::go(void)
{
	Kademlia::CKademlia::reportSearchRef(this);
	// Start with a lot of possible contacts, this is a fallback in case search stalls due to dead contacts
	if (m_possible.empty())
	{
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 
		CPrefs *prefs = CKademlia::getPrefs();
		ASSERT(prefs != NULL); 
		CUInt128 distance(prefs->getClientID());
		distance.xor(m_target);
		routingZone->getClosestTo(1, distance, 50, &m_possible);
	}
	if (m_possible.empty())
		return;

	// Take top 3 possible
	int count = min(ALPHA_QUERY, (int)m_possible.size());
	CContact *c;
	ContactMap::iterator it;
	for (int i=0; i<count; i++)
	{
		it = m_possible.begin();
		c = it->second;
		// Move to tried
		m_tried[it->first] = c;
		m_possible.erase(it);
		// Send request
		c->setType(c->getType()+1);
		CUInt128 check;
		c->getClientID(&check);
		sendFindValue(m_target, check, c->getIPAddress(), c->getUDPPort());
		if(m_type == NODE)
		{
			break;
		}
	}
}

void CSearch::jumpStart(void)
{
	if (m_possible.empty())
	{
		if ( m_created + SEARCHNODE_LIFETIME - time(NULL) > SEC(15) )
			m_created = time(NULL) - SEARCHNODE_LIFETIME - SEC(15);
		return;
	}

	CUInt128 best;
	// Remove any obsolete possible contacts
	if (!m_responded.empty())
	{
		best = m_responded.begin()->first;
		ContactMap::iterator it = m_possible.begin();
		while (it != m_possible.end())
		{
			if (it->first < best)
				it = m_possible.erase(it);
			else
				it++;
		}
	}

	if (m_possible.empty())
		return;

	if (time(NULL)-m_lastSent < SEC(10))
		return;

	// Move to tried
	CContact *c = m_possible.begin()->second;
	m_tried[m_possible.begin()->first] = c;
	m_possible.erase(m_possible.begin());

	// Send request
	c->setType(c->getType()+1);
	CUInt128 check;
	c->getClientID(&check);
	sendFindValue(m_target, check, c->getIPAddress(), c->getUDPPort());
}

void CSearch::processResponse(const CUInt128 &target, uint32 fromIP, uint16 fromPort, ContactList *results)
{
	// Remember the contacts to be deleted when finished
	ContactList::iterator response;
	for (response = results->begin(); response != results->end(); response++)
		m_delete.push_back(*response);

	// Not interested in responses for FIND_NODE, will be added to contacts by udp listener
	if (m_type == NODE)
	{
		m_count++;
		m_possible.clear();
		delete results;
		CKademlia::reportSearchRef(this);
		return;
	}

	ContactMap::const_iterator tried;
	CContact *c;
	CContact *from;
	CUInt128 distance;
	CUInt128 fromDistance;
	bool returnedCloser;

	try
	{
		// Find the person who sent this
		returnedCloser = false;
		for (tried = m_tried.begin(); tried != m_tried.end(); tried++)
		{
			fromDistance = tried->first;
			from = tried->second;


			if ((from->getIPAddress() == fromIP) && (from->getUDPPort() == fromPort))
			{
				// Add to list of people who responded
				m_responded[fromDistance] = from;

				returnedCloser = false;

				// Loop through their responses
				for (response = results->begin(); response != results->end(); response++)
				{
					c = *response;

					// Only interested in type 0
//					if (c->getType() != 0)
//						continue;

					c->getClientID(&distance);
					distance.xor(target);

					// Ignore if already tried
					if (m_tried.count(distance) > 0)	
						continue;

					if (distance < fromDistance)
					{
						returnedCloser = true;
						// If in top 3 responses
						bool top = false;
						if (m_best.size() < ALPHA_QUERY)
							top = true;
						else
						{
							ContactMap::iterator it = m_best.end();
							it--;
							if (distance < it->first)
							{
								m_best.erase(it);
								m_best[distance] = c;
								top = true;
							}
						}
						
						if (top)
						{

							// Add to tried
							m_tried[distance] = c;
							// Send request
							c->setType(c->getType()+1);
							CUInt128 check;
							c->getClientID(&check);
							sendFindValue(m_target, check, c->getIPAddress(), c->getUDPPort());
						}
						else
						{
							// Add to possible
							m_possible[distance] = c;
						}
					}

//					break;
				}

				// We don't want anything from these people, so just increment the counter.
				if( m_type == NODECOMPLETE )
				{
					m_count++;
					CKademlia::reportSearchRef(this);
				}
				// Ask 'from' for the file if closest
				else if (!returnedCloser && fromDistance.get32BitChunk(0) < SEARCHTOLERANCE)
				{
					switch(m_type){
						case FILE:
						case KEYWORD:
						{
							CKademliaUDPListener *udpListner = CKademlia::getUDPListener();
							ASSERT(udpListner != NULL); 
							udpListner->sendPacket(m_searchTerms, m_lenSearchTerms, from->getIPAddress(), from->getUDPPort());
							break;
						}
						case STOREFILE:
						{
							CString fileID;
							target.toHexString(&fileID);
							uchar fileid[16];
							DecodeBase16(fileID.GetBuffer(),fileID.GetLength(),fileid);
							::CSharedFileList *sharedFileList = CKademlia::getSharedFileList();
							ASSERT(sharedFileList != NULL); 
							CKnownFile* file = sharedFileList->GetFileByID(fileid);
							if( file ){
								m_fileName = file->GetFileName();
								file->SetPublishedKadSrc();
//								m_count++;
								CUInt128 id;
								CPrefs *prefs = CKademlia::getPrefs();
								ASSERT(prefs != NULL); 
								prefs->getClientHash(&id);
								CKademliaUDPListener *udpListner = CKademlia::getUDPListener();
								ASSERT(udpListner != NULL);

								TagList taglist;

								//We can use type for different types of sources. 1 is reserved for highID sources..
								taglist.push_back(new CTagUInt8(TAG_SOURCETYPE, 1));
//								taglist.push_back(new CTagUInt32(TAG_SERVERIP, 0));
//								taglist.push_back(new CTagUInt16(TAG_SERVERPORT, 0));
								taglist.push_back(new CTagUInt16(TAG_SOURCEPORT, prefs->getTCPPort()));

								udpListner->publishPacket(from->getIPAddress(), from->getUDPPort(),target,id, taglist);
								Kademlia::CKademlia::reportSearchRef(this);
								TagList::const_iterator it;
								for (it = taglist.begin(); it != taglist.end(); it++)
									delete *it;
							}
							break;
						}
						case STOREKEYWORD:
						{
							CKademliaUDPListener *udpListner = CKademlia::getUDPListener();
							ASSERT(udpListner != NULL); 
							if( bio1 )
							{
								udpListner->sendPacket( packet1, ((1024*50)-bio1->getAvailable()), from->getIPAddress(), from->getUDPPort() );
								Kademlia::CKademlia::reportSearchRef(this);
							}
							if( bio2 )
							{
								udpListner->sendPacket( packet2, ((1024*50)-bio2->getAvailable()), from->getIPAddress(), from->getUDPPort() );
								Kademlia::CKademlia::reportSearchRef(this);
							}
							if( bio3 )
							{
								udpListner->sendPacket( packet3, ((1024*50)-bio3->getAvailable()), from->getIPAddress(), from->getUDPPort() );
								Kademlia::CKademlia::reportSearchRef(this);
							}
						}
					}
				}
			}
		}
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearch::processResponse");
	}
	delete results;
}

void CSearch::processResult(const CUInt128 &target, uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagList *info)
{
//	m_count++;
	switch(m_type)
	{
		case FILE:
//			m_searchType = 1;
			processResultFile(target, fromIP, fromPort, answer, info);
			break;
		case KEYWORD:
//			m_searchType = 2;
			processResultKeyword(target, fromIP, fromPort, answer, info);
			break;
	}
	Kademlia::CKademlia::reportSearchRef(this);
}

void CSearch::processResultFile(const CUInt128 &target, uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagList *info)
{
	CString temp;
	uint8  type = 0;
	uint32 ip = 0;
	uint16 tcp = 0;
	uint16 udp = 0;
	uint32 serverip = 0;
	uint16 serverport = 0;
	CTag *tag;
	TagList::const_iterator it;
	for (it = info->begin(); it != info->end(); it++)
	{
		tag = *it;
		if (!tag->m_name.Compare(TAG_SOURCETYPE))
			type		= tag->GetInt();
		else if (!tag->m_name.Compare(TAG_SOURCEIP))
			ip			= tag->GetInt();
		else if (!tag->m_name.Compare(TAG_SOURCEPORT))
			tcp			= tag->GetInt();
		else if (!tag->m_name.Compare(TAG_SOURCEUPORT))
			udp			= tag->GetInt();
		else if (!tag->m_name.Compare(TAG_SERVERIP))
			serverip	= tag->GetInt();
		else if (!tag->m_name.Compare(TAG_SERVERPORT))
			serverport	= tag->GetInt();

		delete tag;
	}
	delete info;
	if (m_callbackID && type && ip && tcp)
	{
		m_count++;
		(*m_callbackID)(m_searchID, answer, type, ip, tcp, udp, serverip, serverport);
	}
}

void CSearch::processResultKeyword(const CUInt128 &target, uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagList *info)
{
	bool interested = false;
	CString name = "";
	uint32 size = 0;
	CString type = "";
	CString format = "";
	CString artist = "";
	CString album = "";
	CString title = "";
	uint32 length = 0;
	CString codec;
	uint32 bitrate = 0;
	uint32 availability = 0;

	CTag *tag;
	TagList::const_iterator it;
	for (it = info->begin(); it != info->end(); it++)
	{
		tag = *it;

		if (!tag->m_name.Compare(TAG_NAME))
		{
			name			= tag->GetStr();
			if( name != "" )
			{
				interested = true;
			}
		}
		else if (!tag->m_name.Compare(TAG_SIZE))
			size			= tag->GetInt();
		else if (!tag->m_name.Compare(TAG_TYPE))
			type			= tag->GetStr();
		else if (!tag->m_name.Compare(TAG_FORMAT))
			format			= tag->GetStr();
		else if (!tag->m_name.CompareNoCase(TAG_MEDIA_ARTIST))
			artist			= tag->GetStr();
		else if (!tag->m_name.CompareNoCase(TAG_MEDIA_ALBUM))
			album			= tag->GetStr();
		else if (!tag->m_name.CompareNoCase(TAG_MEDIA_TITLE))
			title			= tag->GetStr();
		else if (!tag->m_name.CompareNoCase(TAG_MEDIA_LENGTH))
			length			= tag->GetInt();
		else if (!tag->m_name.CompareNoCase(TAG_MEDIA_BITRATE))
			bitrate			= tag->GetInt();
		else if (!tag->m_name.CompareNoCase(TAG_MEDIA_CODEC))
			codec			= tag->GetStr();
		else if (!tag->m_name.CompareNoCase(TAG_AVAILABILITY))
		{
			availability	= tag->GetInt();
			if( availability > 65500 )
				availability = 0;
		}
		delete tag;
	}
	delete info;

	if( !interested )
		return;

	// Check that it matches original criteria
	WordList testWords;
	CSearchManager::getWords(name.GetBuffer(0), &testWords);
	
	WordList::const_iterator mine;
	WordList::const_iterator test;
	CString keyword;
	for (mine = m_words.begin(); mine != m_words.end(); mine++)
	{
		keyword = *mine;
		if (keyword.GetAt(0) == '-')
		{
			if (type.CompareNoCase(keyword.GetBuffer(0) + 1))
			{
				interested = false;
				break;
			}
		}
		else
		{
			interested = false;
			for (test = testWords.begin(); test != testWords.end(); test++)
			{
				if (!keyword.CompareNoCase(*test))
				{
					interested = true;
					break;
				}
			}
		}
		if (!interested)
			return;
	}

	if (interested && m_callbackKeyword)
	{
		m_count++;
		if (format.GetLength() == 0)
		{
			(*m_callbackKeyword)(m_searchID, answer, name.GetBuffer(0), size, type.GetBuffer(0), 0);
		}
		else if (length != 0 || !codec.IsEmpty() || bitrate != 0)
		{
			(*m_callbackKeyword)(m_searchID, answer, name.GetBuffer(0), size, type.GetBuffer(0), 8, 
					2, TAG_FORMAT, format.GetBuffer(0), 
					2, TAG_MEDIA_ARTIST, artist, 
					2, TAG_MEDIA_ALBUM, album, 
					2, TAG_MEDIA_TITLE, title, 
					3, TAG_MEDIA_LENGTH, length, 
					3, TAG_MEDIA_BITRATE, bitrate, 
					2, TAG_MEDIA_CODEC, codec, 
					3, TAG_AVAILABILITY, availability);
		}
		else if ((!type.Compare("Image")) && (artist.GetLength() > 0))
		{
			(*m_callbackKeyword)(m_searchID, answer, name.GetBuffer(0), size, type.GetBuffer(0), 3, 
					2, TAG_FORMAT, format.GetBuffer(0), 
					2, TAG_MEDIA_ARTIST, artist, 
					3, TAG_AVAILABILITY, availability);
		}
		else
		{
			(*m_callbackKeyword)(m_searchID, answer, name.GetBuffer(0), size, type.GetBuffer(0), 2, 
					2, TAG_FORMAT, format.GetBuffer(0), 
					3, TAG_AVAILABILITY, availability);
		}
	}
}

void CSearch::sendFindValue(const CUInt128 &target, const CUInt128 &check, uint32 ip, uint16 port)
{
	try
	{
		byte packet[35];
		CByteIO bio(packet, 35);
		bio.writeByte(OP_KADEMLIAHEADER);
		bio.writeByte(KADEMLIA_REQ);
		switch(m_type){
			case NODE:
			case NODECOMPLETE:
				bio.writeByte(KADEMLIA_FIND_NODE);
				break;
			case FILE:
			case KEYWORD:
				bio.writeByte(KADEMLIA_FIND_VALUE);
				break;
			case STOREFILE:
			case STOREKEYWORD:
				bio.writeByte(KADEMLIA_STORE);
				break;
			default:
				CKademlia::debugLine("Invalid search type. (CSearch::sendFindValue)");
				return;
		}
		bio.writeUInt128(target);
		bio.writeUInt128(check);
//		CKademlia::debugMsg("Sent UDP OpCode KADEMLIA_REQUEST(%u)", ip);
		CKademliaUDPListener *udpListner = CKademlia::getUDPListener();
		ASSERT(udpListner != NULL); 
		m_countSent++;
		m_lastSent = time(NULL);
		Kademlia::CKademlia::reportSearchRef(this);
		udpListner->sendPacket(packet, 35, ip, port);
	} 
	catch ( CIOException *ioe )
	{
		CKademlia::debugMsg("Exception in CSearch::sendFindValue (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearch::sendFindValue");
	}
}

void CSearch::addFileID(const CUInt128& id)
{
	m_fileIDs.push_back(id);
}

void CSearch::PreparePacketForTags( CByteIO *bio, CKnownFile *file)
{
	try
	{
		if( file && bio )
		{
			TagList taglist;
			
			// Name, Size
			taglist.push_back(new CTagStr(TAG_NAME, file->GetFileName()));
			taglist.push_back(new CTagUInt(TAG_SIZE, file->GetFileSize()));
			taglist.push_back(new CTagUInt(TAG_AVAILABILITY, (uint32)file->m_nCompleteSourcesCount));
			
			// eD2K file type (Audio, Video, ...)
			CString strED2KFileType(GetED2KFileTypeSearchTerm(GetED2KFileTypeID(file->GetFileName())));
			if (!strED2KFileType.IsEmpty())
				taglist.push_back(new CTagStr(TAG_TYPE, strED2KFileType));
			
			// file format (filename extension)
			int iExt = file->GetFileName().ReverseFind(_T('.'));
			if (iExt != -1)
			{
				CString strExt(file->GetFileName().Mid(iExt));
				if (!strExt.IsEmpty())
				{
					strExt = strExt.Mid(1);
					if (!strExt.IsEmpty())
						taglist.push_back(new CTagStr(TAG_FORMAT, strExt));
				}
			}

			// additional meta data (Artist, Album, Codec, Length, ...)
			// only send verified meta data to nodes
			if (file->GetMetaDataVer() > 0)
			{
				static const struct{
					uint8	nName;
					uint8	nType;
				} _aMetaTags[] = 
				{
					{ FT_MEDIA_ARTIST,  2 },
					{ FT_MEDIA_ALBUM,   2 },
					{ FT_MEDIA_TITLE,   2 },
					{ FT_MEDIA_LENGTH,  3 },
					{ FT_MEDIA_BITRATE, 3 },
					{ FT_MEDIA_CODEC,   2 }
				};
				for (int i = 0; i < ARRSIZE(_aMetaTags); i++)
				{
					const ::CTag* pTag = file->GetTag(_aMetaTags[i].nName, _aMetaTags[i].nType);
					if (pTag)
					{
						// skip string tags with empty string values
						if (pTag->tag.type == 2 && (pTag->tag.stringvalue == NULL || pTag->tag.stringvalue[0] == '\0'))
							continue;
						// skip integer tags with '0' values
						if (pTag->tag.type == 3 && pTag->tag.intvalue == 0)
							continue;
						char szKadTagName[2];
						szKadTagName[0] = pTag->tag.specialtag;
						szKadTagName[1] = '\0';
						if (pTag->tag.type == 2)
							taglist.push_back(new CTagStr(szKadTagName, pTag->tag.stringvalue));
						else
							taglist.push_back(new CTagUInt(szKadTagName, pTag->tag.intvalue));
					}
				}
			}
			bio->writeTagList(taglist);
			TagList::const_iterator it;
			for (it = taglist.begin(); it != taglist.end(); it++)
				delete *it;
		}
		else
		{
			//If we get here.. Bad things happen.. Will fix this later if it is a real issue.
			ASSERT(0);
		}
	}
	catch ( CIOException *ioe )
	{
		CKademlia::debugMsg("Exception in CSearch::PreparePacketForTags (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearch::PreparePacketForTags");
	}
}

void CSearch::PreparePacket(void)
{
	try
	{
		//TODO Move the tag reading to a seperate method to reduce code.
		::CSharedFileList *sharedFileList = CKademlia::getSharedFileList();
		ASSERT(sharedFileList != NULL); 
		int count = m_fileIDs.size();
		CString fileID;
		uchar fileid[16];
		CKnownFile* file = NULL;
		if( count > 150 )
			count = 150;
		if( count > 100 )
		{
			bio3 = new CByteIO(packet3,sizeof(packet3));
			bio3->writeByte(OP_KADEMLIAHEADER);
			bio3->writeByte(KADEMLIA_PUBLISH_REQ);
			bio3->writeUInt128(m_target);
			bio3->writeUInt16(count-100);
			while ( count > 100 )
			{
				count--;
				bio3->writeUInt128(m_fileIDs.front());
				m_fileIDs.front().toHexString(&fileID);
				m_fileIDs.pop_front();
				DecodeBase16(fileID.GetBuffer(),fileID.GetLength(),fileid);
				file = sharedFileList->GetFileByID(fileid);
				PreparePacketForTags( bio3, file );
			}
		}
		if( count > 50 )
		{
			bio2 = new CByteIO(packet2,sizeof(packet2));
			bio2->writeByte(OP_KADEMLIAHEADER);
			bio2->writeByte(KADEMLIA_PUBLISH_REQ);
			bio2->writeUInt128(m_target);
			bio2->writeUInt16(count-50);
			while ( count > 50 )
			{
				count--;
				bio2->writeUInt128(m_fileIDs.front());
				m_fileIDs.front().toHexString(&fileID);
				m_fileIDs.pop_front();
				DecodeBase16(fileID.GetBuffer(),fileID.GetLength(),fileid);
				file = sharedFileList->GetFileByID(fileid);
				PreparePacketForTags( bio2, file );
			}
		}
		if( count > 0 )
		{
			bio1 = new CByteIO(packet1,sizeof(packet1));
			bio1->writeByte(OP_KADEMLIAHEADER);
			bio1->writeByte(KADEMLIA_PUBLISH_REQ);
			bio1->writeUInt128(m_target);
			bio1->writeUInt16(count);
			while ( count > 0 )
			{
				count--;
				bio1->writeUInt128(m_fileIDs.front());
				m_fileIDs.front().toHexString(&fileID);
				m_fileIDs.pop_front();
				DecodeBase16(fileID.GetBuffer(),fileID.GetLength(),fileid);
				file = sharedFileList->GetFileByID(fileid);
				PreparePacketForTags( bio1, file );
			}
		}
	}
	catch ( CIOException *ioe )
	{
		CKademlia::debugMsg("Exception in CSearch::PreparePacket (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearch::PreparePacket");
	}
}