/*
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
#include "Indexed.h"
#include "Kademlia.h"
#include "../../OpCodes.h"
#include "Defines.h"
#include "Prefs.h"
#include "../routing/RoutingZone.h"
#include "../routing/Contact.h"
#include "../net/KademliaUDPListener.h"
#include "../utils/MiscUtils.h"
#include "../../OtherFunctions.h"
#include "../kademlia/tag.h"
#include "../io/FileIO.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////
CString CIndexed::m_filename = "";

CIndexed::CIndexed()
{
	m_filename = CMiscUtils::getAppDir();
	m_filename.Append(_T(CONFIGFOLDER));
	m_filename.Append("index.dat");
	m_lastClean = time(NULL) + (60*30);
	m_totalIndexSource = 0;
	m_totalIndexKeyword = 0;
	readFile();
}

void CIndexed::readFile(void)
{
	try
	{
		uint32 totalSource = 0;
		uint32 totalKeyword = 0;
		uint32 numKeys = 0;
		uint32 numSource = 0;
		uint32 numName = 0;
		uint32 tagList = 0;
		time_t now = time(NULL) - (KADEMLIAINDEXCLEAN);
		CFileIO file;
		if (file.Open(m_filename.GetBuffer(0), CFile::modeRead))
		{
			numKeys = file.readUInt32();
			CUInt128 id;
			while( numKeys )
			{
				CUInt128 keyID;
				file.readUInt128(&keyID);
				numSource = file.readUInt32();
				while( numSource )
				{
					CUInt128 sourceID;
					file.readUInt128(&sourceID);
					numName = file.readUInt32();
					while( numName )
					{
						Kademlia::CEntry* toaddN = new Kademlia::CEntry();
						uint32 test = file.readUInt32();
						toaddN->lifetime = test;
						//tagList = file.readUInt32LE();
						tagList = file.readByte();
						bool source = true;
						while( tagList )
						{
							CTag* tag = file.readTag();
							if (!tag->m_name.Compare(TAG_SOURCETYPE))
							{
								toaddN->source = true;
							}
							if (!tag->m_name.Compare(TAG_NAME))
							{
								toaddN->fileName = tag->GetStr();
								source = false;
								// NOTE: always add the 'name' tag, even if it's stored separately in 'fileName'. the tag is still needed for answering search request
								toaddN->taglist.push_back(tag);
							}
							else if (!tag->m_name.Compare(TAG_SIZE))
							{
								toaddN->size = tag->GetInt();
								// NOTE: always add the 'size' tag, even if it's stored separately in 'size'. the tag is still needed for answering search request
								toaddN->taglist.push_back(tag);
							}
							else if (!tag->m_name.Compare(TAG_SOURCEIP))
							{
								toaddN->ip = tag->GetInt();
								toaddN->taglist.push_back(tag);
							}
							else if (!tag->m_name.Compare(TAG_SOURCEPORT))
							{
								toaddN->tcpport = tag->GetInt();
								toaddN->taglist.push_back(tag);
							}
							else if (!tag->m_name.Compare(TAG_SOURCEUPORT))
							{
								toaddN->udpport = tag->GetInt();
								toaddN->taglist.push_back(tag);
							}
							else
							{
								toaddN->taglist.push_back(tag);
							}
							tagList--;
						}
						if( toaddN->lifetime < now)	
						{
							delete toaddN;
						}
						else
						{
							toaddN->keyID.setValue(keyID);
							toaddN->sourceID.setValue(sourceID);
							if(IndexedAdd(keyID, sourceID, toaddN))
							{
								if(toaddN->source)
								{
									totalSource++;
								}
								else
								{
									totalKeyword++;
								}
							}
						}
						numName--;
					}
					numSource--;
				}
				numKeys--;
			}
			file.Close();
			m_totalIndexSource = totalSource;
			m_totalIndexKeyword = totalKeyword;
			CKademlia::debugMsg("Read %u source and %u keyword entries", totalSource, totalKeyword);
		}
	} catch (...) {TRACE("\n************Error reading Entries!\n");ASSERT(0);}
}

void CIndexed::writeFile(void)
{
	try
	{
		uint32 total = 0;
		uint32 numKeys = 0;
		uint32 numSource = 0;
		uint32 numName = 0;
		CFileIO file;
		if (file.Open(m_filename.GetBuffer(0), CFile::modeWrite | CFile::modeCreate))
		{
			numKeys = this->keywordHashList.GetCount();
			file.writeUInt32(numKeys);
			for(POSITION pos = keywordHashList.GetHeadPosition(); pos != NULL; )
			{
				KeywordHash* currKeywordHash = keywordHashList.GetNext(pos);
				file.writeUInt128(currKeywordHash->keyID);
				numSource = currKeywordHash->sourceList.GetCount();
				file.writeUInt32(numSource);
				for(POSITION pos3 = currKeywordHash->sourceList.GetHeadPosition(); pos3 != NULL; )
				{
					Source* currSource = currKeywordHash->sourceList.GetNext(pos3);
					file.writeUInt128(currSource->sourceID);
					numName = currSource->entryList.GetCount();
					file.writeUInt32(numName);
					for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; )
					{
						Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
						uint32 test = (uint32)currName->lifetime;
						file.writeUInt32(test);
						file.writeTagList(currName->taglist);
						total++;
					}
				}
			}
			file.Close();
			CKademlia::debugMsg("Wrote %u indexed entries", total);
		}
	} 
	catch (...) {ASSERT(0);}
}

void CIndexed::clean(void)
{
	try
	{
		uint32 removed = 0;
		uint32 totalSource = 0;
		uint32 totalKeyword = 0;
		time_t expire = time(NULL) - (KADEMLIAINDEXCLEAN);
		if( m_lastClean > time(NULL) )
		{
			return;
		}
		for(POSITION pos = keywordHashList.GetHeadPosition(); pos != NULL; )
		{
			POSITION pos2 = pos;
			KeywordHash* currKeywordHash = keywordHashList.GetNext(pos);
			for(POSITION pos3 = currKeywordHash->sourceList.GetHeadPosition(); pos3 != NULL; )
			{
				POSITION pos4 = pos3;
				Source* currSource = currKeywordHash->sourceList.GetNext(pos3);
				for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; )
				{
					POSITION pos6 = pos5;
					Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
					if(currName->source)
					{
						totalSource++;
					}
					else
					{
						totalKeyword++;
					}
					if( currName->lifetime < expire)
					{
						removed++;
						currSource->entryList.RemoveAt(pos6);
						delete currName;
					}
				}
				if( currSource->entryList.IsEmpty())
				{
					currKeywordHash->sourceList.RemoveAt(pos4);
					delete currSource;
				}
			}
			if( currKeywordHash->sourceList.IsEmpty())
			{
				keywordHashList.RemoveAt(pos2);
				delete currKeywordHash;
			}
		}
		m_totalIndexSource = totalSource;
		m_totalIndexKeyword = totalKeyword;
		CKademlia::debugMsg("Active: Removed %u old indexed entries out of %u source %u keyword", removed, totalSource, totalKeyword);
		m_lastClean = time(NULL) + (60*30);
	} catch(...){ASSERT(0);}
}

CIndexed::~CIndexed()
{
	try
	{
		writeFile();
		while( !keywordHashList.IsEmpty()){
			KeywordHash* currKeywordHash = keywordHashList.GetHead();
			while( !currKeywordHash->sourceList.IsEmpty()){
				Source* currSource = currKeywordHash->sourceList.GetHead();
				while( !currSource->entryList.IsEmpty() ){
					Kademlia::CEntry* del1 = currSource->entryList.GetHead();
					currSource->entryList.RemoveHead();
					delete del1;
				}
				Source* del2 = currKeywordHash->sourceList.GetHead();
				currKeywordHash->sourceList.RemoveHead();
				delete del2;
			}
			KeywordHash* del3 = keywordHashList.GetHead();
			keywordHashList.RemoveHead();
			delete del3;
		}
	} catch(...){ASSERT(0);}
}

bool CIndexed::IndexedAdd(Kademlia::CUInt128 keyWordID, Kademlia::CUInt128 sourceID, Kademlia::CEntry* entry){
	try
	{
//		May add this a little later after testing a bit.
//		if( m_totalIndexKeyword > KADEMLIAMAXINDEX && !entry->source)
//		{
//			return false;
//		}
		if(keywordHashList.IsEmpty()){
			KeywordHash* toaddH = new KeywordHash;
			toaddH->keyID.setValue(keyWordID);
			Source* toaddS = new Source;
			toaddS->sourceID.setValue(sourceID);
			Kademlia::CEntry* toaddN = entry;
			toaddS->entryList.AddHead(toaddN);
			toaddH->sourceList.AddHead(toaddS);
			keywordHashList.AddHead(toaddH);
			return true;
		}
		for(POSITION pos = keywordHashList.GetHeadPosition(); pos != NULL; )
		{
			KeywordHash* currKeywordHash = keywordHashList.GetNext(pos);
			if (currKeywordHash->keyID == keyWordID){
				for(POSITION pos3 = currKeywordHash->sourceList.GetHeadPosition(); pos3 != NULL; ){
					Source* currSource = currKeywordHash->sourceList.GetNext(pos3);
					if (currSource->sourceID == sourceID){
						for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; ){
							POSITION pos6 = pos5;
							Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
							if ( !entry->source )
							{
								if(!currName->source)
								{
									currSource->entryList.RemoveAt(pos6);
									delete currName;
									currSource->entryList.AddHead(entry);
									return true;
								}
							}
							else if( entry->source )
							{
								if(currName->ip == entry->ip && currName->tcpport == entry->tcpport)
								{
									currSource->entryList.RemoveAt(pos6);
									delete currName;
									currSource->entryList.AddHead(entry);
									return true;
								}
							}
						}
						//New entry
						if(entry->source)
						{
							if( currSource->entryList.GetCount() > KADEMLIAMAXSOUCEPERFILE )
							{
								CKademlia::debugMsg("Rotating sources");
								Kademlia::CEntry* toremove = currSource->entryList.GetTail();
								currSource->entryList.RemoveTail();
								delete toremove;
							}
							Kademlia::CEntry* toadd = entry;
							currSource->entryList.AddHead(toadd);
							return true;
						}
					}
				}
				Source* toaddS = new Source;
				toaddS->sourceID.setValue(sourceID);
				Kademlia::CEntry* toaddN = entry;
				toaddS->entryList.AddHead(toaddN);
				currKeywordHash->sourceList.AddHead(toaddS);
				return true;
				//New Source
			}
		}
		KeywordHash* toaddH = new KeywordHash;
		toaddH->keyID.setValue(keyWordID);
		Source* toaddS = new Source;
		toaddS->sourceID.setValue(sourceID);
		Kademlia::CEntry* toaddN = entry;
		toaddS->entryList.AddHead(toaddN);
		toaddH->sourceList.AddHead(toaddS);
		keywordHashList.AddHead(toaddH);
		return true;
		//This is a new key!
	} catch(...){ASSERT(0);}
	return false;

}
bool SearchTermsMatch(const SSearchTerm* pSearchTerm, const Kademlia::CEntry* item)
{
#define ASSERT_RETURN(x)	if (!(x)) { ASSERT(0); return false; } // it's not worth that we crash here
	ASSERT_RETURN( pSearchTerm != NULL );
	ASSERT_RETURN( item != NULL );

	// boolean operators
	if (pSearchTerm->type == SSearchTerm::AND){
		ASSERT_RETURN( pSearchTerm->left != NULL && pSearchTerm->right != NULL );
		return SearchTermsMatch(pSearchTerm->left, item) && SearchTermsMatch(pSearchTerm->right, item);
	}
	if (pSearchTerm->type == SSearchTerm::OR){
		ASSERT_RETURN( pSearchTerm->left != NULL && pSearchTerm->right != NULL );
		return SearchTermsMatch(pSearchTerm->left, item) || SearchTermsMatch(pSearchTerm->right, item);
	}
	if (pSearchTerm->type == SSearchTerm::NAND){
		ASSERT_RETURN( pSearchTerm->left != NULL && pSearchTerm->right != NULL );
		return SearchTermsMatch(pSearchTerm->left, item) && !SearchTermsMatch(pSearchTerm->right, item);
	}

	// word which is to be searched in the file name (and in additional meta data as done by some ed2k servers???)
	if (pSearchTerm->type == SSearchTerm::String){
		ASSERT_RETURN( pSearchTerm->str != NULL );
		//TODO: Use a pre-tokenized list for better performance.
		int iPos = 0;
		CString strTok(item->fileName.Tokenize(" ()[]{}<>,._-!?", iPos));
		while (!strTok.IsEmpty()){
			if (stricmp(strTok, *(pSearchTerm->str)) == 0)
				return true;
			strTok = item->fileName.Tokenize(" ()[]{}<>,._-!?", iPos);
		}

		// search string value in all string meta tags (this includes also the filename)
		// although this would work, I am no longer sure if it's the correct way to process the search requests..
		/*const Kademlia::CTag *tag;
		TagList::const_iterator it;
		for (it = item->taglist.begin(); it != item->taglist.end(); it++)
		{
			tag = *it;
			if (tag->m_type == 2)
			{
				//TODO: Use a pre-tokenized list for better performance.
				int iPos = 0;
				CString strTok(static_cast<const CTagStr *>(tag)->m_value.Tokenize(" ()[]{}<>,._-!?", iPos));
				while (!strTok.IsEmpty()){
					if (stricmp(strTok, *(pSearchTerm->str)) == 0)
						return true;
					strTok = static_cast<const CTagStr *>(tag)->m_value.Tokenize(" ()[]{}<>,._-!?", iPos);
				}
			}
		}*/

		return false;
	}

	if (pSearchTerm->type == SSearchTerm::MetaTag){
		ASSERT_RETURN( pSearchTerm->tag != NULL );
		if (pSearchTerm->tag->m_type == 2){ // meta tags with string values
			const Kademlia::CTag *tag;
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				tag = *it;
				if (tag->IsStr() && pSearchTerm->tag->m_name == tag->m_name)
					return tag->GetStr().CompareNoCase(pSearchTerm->tag->GetStr()) == 0;
			}
		}
	}
	else if (pSearchTerm->type == SSearchTerm::Min){
		ASSERT_RETURN( pSearchTerm->tag != NULL );
		if (pSearchTerm->tag->IsInt()){ // meta tags with integer values
			const Kademlia::CTag *tag;
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name == tag->m_name)
					return tag->GetInt() >= pSearchTerm->tag->GetInt();
			}
		}
		else if (pSearchTerm->tag->IsFloat()){ // meta tags with float values
			const Kademlia::CTag *tag;
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name == tag->m_name)
					return tag->GetFloat() >= pSearchTerm->tag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->type == SSearchTerm::Max){
		ASSERT_RETURN( pSearchTerm->tag != NULL );
		if (pSearchTerm->tag->IsInt()){ // meta tags with integer values
			const Kademlia::CTag *tag;
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name == tag->m_name)
					return tag->GetInt() <= pSearchTerm->tag->GetInt();
			}
		}
		else if (pSearchTerm->tag->IsFloat()){ // meta tags with float values
			const Kademlia::CTag *tag;
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name == tag->m_name)
					return tag->GetFloat() <= pSearchTerm->tag->GetFloat();
			}
		}
	}

	//ASSERT(0);
	return false;
#undef ASSERT_RETURN
}

void CIndexed::SendValidResult( Kademlia::CUInt128 keyWordID, const SSearchTerm* pSearchTerms, const sockaddr_in *senderAddress, bool source ){
	try
	{
		CKademliaUDPListener *udpListner = CKademlia::getUDPListener();
		ASSERT(udpListner != NULL); 
		byte packet[1024*50];
		CByteIO bio(packet,sizeof(packet));
		bio.writeByte(OP_KADEMLIAHEADER);
		bio.writeByte(KADEMLIA_SEARCH_RES);
		bio.writeUInt128(keyWordID);
		bio.writeUInt16(50);
		time_t now = time(NULL) - (KADEMLIAINDEXCLEAN);
		uint16 maxResults = 300;
		uint16 count = 0;
		uint32 removed = 0;
		for(POSITION pos = keywordHashList.GetHeadPosition(); pos != NULL; )
		{
			KeywordHash* currKeywordHash = keywordHashList.GetNext(pos);
			if (currKeywordHash->keyID == keyWordID)
			{
				for(POSITION pos3 = currKeywordHash->sourceList.GetHeadPosition(); pos3 != NULL; )
				{
					Source* currSource = currKeywordHash->sourceList.GetNext(pos3);
					for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; )
					{
						POSITION pos6 = pos5;
						Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
						if( currName->lifetime < now )
						{
							removed++;
							currSource->entryList.RemoveAt(pos6);
							delete currName;
							currName = NULL;
						}
						else
						{
							if( source )
							{
								if( currName->source )
								{
									if( count < maxResults )
									{
										bio.writeUInt128(currName->sourceID);
										bio.writeTagList(currName->taglist);
										count++;
										if( count % 50 == 0 )
										{
											uint32 len = sizeof(packet)-bio.getAvailable();
											TRACE("Search %u\n", count);
											udpListner->sendPacket(packet, len, ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));
											bio.reset();
											bio.writeByte(OP_KADEMLIAHEADER);
											bio.writeByte(KADEMLIA_SEARCH_RES);
											bio.writeUInt128(keyWordID);
											bio.writeUInt16(50);
										}
									}
								}
							}
							else
							{
								if( !currName->source)
								{
									if ( !pSearchTerms || SearchTermsMatch(pSearchTerms, currName) )
									{
										if( count < maxResults ){
											bio.writeUInt128(currName->sourceID);
											bio.writeTagList(currName->taglist);
											count++;
											if( count % 50 == 0 )
											{
												uint32 len = sizeof(packet)-bio.getAvailable();
												TRACE("Search %u\n", count);
												udpListner->sendPacket(packet, len, ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));
												bio.reset();
												bio.writeByte(OP_KADEMLIAHEADER);
												bio.writeByte(KADEMLIA_SEARCH_RES);
												bio.writeUInt128(keyWordID);
												bio.writeUInt16(50);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		int16 ccount = count % 50;
		if( ccount )
		{
			uint32 len = sizeof(packet)-bio.getAvailable();
//			CKademlia::debugMsg("Sent UDP OpCode KADEMLIA_SEARCH_RESULT (Keyword)(%u)", ntohl(senderAddress->sin_addr.s_addr));
//			CMiscUtils::debugHexDump(packet, len);
			TRACE("Search %u\n", count);
			memcpy(packet+18, &ccount, 2);
			udpListner->sendPacket(packet, len, ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));
		}
		if(removed)
		{
			CKademlia::debugMsg("Passive: Removed %u old indexed entries", removed);
		}
		clean();
	} catch(...){ASSERT(0);}
}

SSearchTerm::SSearchTerm()
{
	type = AND;
	tag = NULL;
	left = NULL;
	right = NULL;
}

SSearchTerm::~SSearchTerm()
{
	if (type == String)
		delete str;
	delete tag;
}
