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
#include "../io/IOException.h"
#include "emule.h"
#include "Preferences.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

void DebugSend(LPCTSTR pszMsg, uint32 ip, uint16 port);

SSearchHits ssh;

CString CIndexed::m_kfilename = "";
CString CIndexed::m_sfilename = "";

CIndexed::CIndexed()
{
	m_Keyword_map.InitHashTable(1031);
	m_sfilename = CMiscUtils::getAppDir();
	m_sfilename.Append(_T(CONFIGFOLDER));
	m_sfilename.Append("s_index.dat");
	m_kfilename = CMiscUtils::getAppDir();
	m_kfilename.Append(_T(CONFIGFOLDER));
	m_kfilename.Append("k_index.dat");
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
		time_t s_expire = time(NULL) - (KADEMLIAREPUBLISHTIMES);
		time_t k_expire = time(NULL) - (KADEMLIAREPUBLISHTIMEK);

		CBufferedFileIO k_file;
		if (k_file.Open(m_kfilename, CFile::modeRead | CFile::typeBinary))
		{
			setvbuf(k_file.m_pStream, NULL, _IOFBF, 32768);

			numKeys = k_file.readUInt32();
			CUInt128 id;
			while( numKeys )
			{
				CUInt128 keyID;
				k_file.readUInt128(&keyID);
				numSource = k_file.readUInt32();
				while( numSource )
				{
					CUInt128 sourceID;
					k_file.readUInt128(&sourceID);
					numName = k_file.readUInt32();
					while( numName )
					{
						Kademlia::CEntry* toaddN = new Kademlia::CEntry();
						toaddN->source = false;
						uint32 test = k_file.readUInt32();
						toaddN->lifetime = test;
						tagList = k_file.readByte();
						while( tagList )
						{
							CTag* tag = k_file.readTag();
							if (!tag->m_name.Compare(TAG_NAME))
							{
								toaddN->fileName = tag->GetStr();
								toaddN->fileName.MakeLower(); // make lowercase, the search code expects lower case strings!
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
						if( toaddN->lifetime < k_expire )	
						{
							delete toaddN;
						}
						else
						{
							toaddN->keyID.setValue(keyID);
							toaddN->sourceID.setValue(sourceID);
							if(AddKeyword(keyID, sourceID, toaddN))
								totalKeyword++;
						}
						numName--;
					}
					numSource--;
				}
				numKeys--;
			}
			k_file.Close();
		}

		CBufferedFileIO s_file;
		if (s_file.Open(m_sfilename, CFile::modeRead | CFile::typeBinary))
		{
			setvbuf(s_file.m_pStream, NULL, _IOFBF, 32768);

			numKeys = s_file.readUInt32();
			CUInt128 id;
			while( numKeys )
			{
				CUInt128 keyID;
				s_file.readUInt128(&keyID);
				numSource = s_file.readUInt32();
				while( numSource )
				{
					CUInt128 sourceID;
					s_file.readUInt128(&sourceID);
					numName = s_file.readUInt32();
					while( numName )
					{
						Kademlia::CEntry* toaddN = new Kademlia::CEntry();
						toaddN->source = true;
						uint32 test = s_file.readUInt32();
						toaddN->lifetime = test;
						tagList = s_file.readByte();
						while( tagList )
						{
							CTag* tag = s_file.readTag();
							if (!tag->m_name.Compare(TAG_SOURCEIP))
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
						if( toaddN->lifetime < s_expire )	
						{
							delete toaddN;
						}
						else
						{
							toaddN->keyID.setValue(keyID);
							toaddN->sourceID.setValue(sourceID);
							if(AddSources(keyID, sourceID, toaddN))
								totalSource++;
						}
						numName--;
					}
					numSource--;
				}
				numKeys--;
			}
			s_file.Close();

			m_totalIndexSource = totalSource;
			m_totalIndexKeyword = totalKeyword;
			CKademlia::debugMsg("Read %u source and %u keyword entries", totalSource, totalKeyword);
		}
	} 
	catch ( CIOException *ioe )
	{
		CKademlia::debugMsg("Exception in CIndexed::readFile (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CIndexed::readFile");
	}
}

CIndexed::~CIndexed()
{
	try
	{
		uint32 s_total = 0;
		uint32 k_total = 0;

		CBufferedFileIO s_file;
		if (s_file.Open(m_sfilename, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary))
		{
			setvbuf(s_file.m_pStream, NULL, _IOFBF, 32768);

			CCKey key;
			CCKey key2;

			s_file.writeUInt32(m_Sources_map.GetCount());
			POSITION pos = m_Sources_map.GetStartPosition();
			while( pos != NULL )
			{
				KeyHash* currKeyHash;
				m_Sources_map.GetNextAssoc( pos, key, currKeyHash );
				s_file.writeUInt128(currKeyHash->keyID);

				CSourceKeyMap& KeyHashSrcMap = currKeyHash->m_Source_map;
				s_file.writeUInt32(KeyHashSrcMap.GetCount());
				POSITION pos2 = KeyHashSrcMap.GetStartPosition();
				while( pos2 != NULL )
				{
					Source* currSource;
					KeyHashSrcMap.GetNextAssoc( pos2, key2, currSource );
					s_file.writeUInt128(currSource->sourceID);

					CKadEntryPtrList& SrcEntryList = currSource->entryList;
					s_file.writeUInt32(SrcEntryList.GetCount());
					for(POSITION pos5 = SrcEntryList.GetHeadPosition(); pos5 != NULL; )
					{
						Kademlia::CEntry* currName = SrcEntryList.GetNext(pos5);
						s_file.writeUInt32(currName->lifetime);
						s_file.writeTagList(currName->taglist);
						delete currName;
						s_total++;
					}
					delete currSource;
				}
				delete currKeyHash;
			}
			s_file.Close();
		}

		CBufferedFileIO k_file;
		if (k_file.Open(m_kfilename, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary))
		{
			setvbuf(k_file.m_pStream, NULL, _IOFBF, 32768);

			CCKey key;
			CCKey key2;

			k_file.writeUInt32(m_Keyword_map.GetCount());
			POSITION pos = m_Keyword_map.GetStartPosition();
			while( pos != NULL )
			{
				KeyHash* currKeyHash;
				m_Keyword_map.GetNextAssoc( pos, key, currKeyHash );
				k_file.writeUInt128(currKeyHash->keyID);

				CSourceKeyMap& KeyHashSrcMap = currKeyHash->m_Source_map;
				k_file.writeUInt32(KeyHashSrcMap.GetCount());
				POSITION pos2 = KeyHashSrcMap.GetStartPosition();
				while( pos2 != NULL )
				{
					Source* currSource;
					KeyHashSrcMap.GetNextAssoc( pos2, key2, currSource );
					k_file.writeUInt128(currSource->sourceID);
				
					CKadEntryPtrList& SrcEntryList = currSource->entryList;
					k_file.writeUInt32(SrcEntryList.GetCount());
					for(POSITION pos5 = SrcEntryList.GetHeadPosition(); pos5 != NULL; )
					{
						Kademlia::CEntry* currName = SrcEntryList.GetNext(pos5);
						k_file.writeUInt32(currName->lifetime);
						k_file.writeTagList(currName->taglist);
						delete currName;
						k_total++;
					}
					delete currSource;
				}
				delete currKeyHash;
			}
			k_file.Close();
		}
		CKademlia::debugMsg("Wrote %u source and %u keyword entries", s_total, k_total);
	} 
	catch ( CIOException *ioe )
	{
		CKademlia::debugMsg("Exception in CIndexed::~CIndexed (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CIndexed::~CIndexed");
	}
}

void CIndexed::clean(void)
{
	try
	{
		if( m_lastClean > time(NULL) )
		{
			return;
		}

		uint32 k_Removed = 0;
		uint32 s_Removed = 0;
		uint32 s_Total = 0;
		uint32 k_Total = 0;
		time_t s_expire = time(NULL) - (KADEMLIAREPUBLISHTIMES);
		time_t k_expire = time(NULL) - (KADEMLIAREPUBLISHTIMEK);

		POSITION pos = m_Keyword_map.GetStartPosition();
		while( pos != NULL )
		{
			KeyHash* currKeyHash;
			CCKey key;
			m_Keyword_map.GetNextAssoc( pos, key, currKeyHash );
			POSITION pos2 = currKeyHash->m_Source_map.GetStartPosition();
			while( pos2 != NULL )
			{
				Source* currSource;
				CCKey key2;
                currKeyHash->m_Source_map.GetNextAssoc( pos2, key2, currSource );
				for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; )
				{
					POSITION pos6 = pos5;
					Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
					k_Total++;
					if( !currName->source && currName->lifetime < k_expire)
					{
						k_Removed++;
						currSource->entryList.RemoveAt(pos6);
						delete currName;
					}
				}
				if( currSource->entryList.IsEmpty())
				{
					currKeyHash->m_Source_map.RemoveKey(key2);
					delete currSource;
				}
			}
			if( currKeyHash->m_Source_map.IsEmpty())
			{
				m_Keyword_map.RemoveKey(key);
				delete currKeyHash;
			}
		}

		pos = m_Sources_map.GetStartPosition();
		while( pos != NULL )
		{
			KeyHash* currKeyHash;
			CCKey key;
			m_Sources_map.GetNextAssoc( pos, key, currKeyHash );
			POSITION pos2 = currKeyHash->m_Source_map.GetStartPosition();
			while( pos2 != NULL )
			{
				Source* currSource;
				CCKey key2;
                currKeyHash->m_Source_map.GetNextAssoc( pos2, key2, currSource );
				for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; )
				{
					POSITION pos6 = pos5;
					Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
					s_Total++;
					if( currName->lifetime < s_expire)
					{
						s_Removed++;
						currSource->entryList.RemoveAt(pos6);
						delete currName;
					}
				}
				if( currSource->entryList.IsEmpty())
				{
					currKeyHash->m_Source_map.RemoveKey(key2);
					delete currSource;
				}
			}
			if( currKeyHash->m_Source_map.IsEmpty())
			{
				m_Sources_map.RemoveKey(key);
				delete currKeyHash;
			}
		}

		m_totalIndexSource = s_Total;
		m_totalIndexKeyword = k_Total;
		CKademlia::debugMsg("Removed %u keyword out of %u and %u source out of %u", k_Removed, k_Total, s_Removed, s_Total);
		m_lastClean = time(NULL) + (60*30);
	} 
	catch(...)
	{
		CKademlia::debugLine("Exception in CIndexed::clean");
		ASSERT(0);
	}
}

bool CIndexed::AddKeyword(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry, bool ignoreSize){
	try
	{
//		May add this a little later after testing a bit.
//		if( m_totalIndexKeyword > KADEMLIAMAXINDEX )
//		{
//			return false;
//		}

		KeyHash* currKeyHash;
		if(!m_Keyword_map.Lookup(CCKey(keyID.getData()), currKeyHash))
		{
			Source* currSource = new Source;
			currSource->sourceID.setValue(sourceID);
			currSource->entryList.AddHead(entry);
			currKeyHash = new KeyHash;
			currKeyHash->keyID.setValue(keyID);
			currKeyHash->m_Source_map.SetAt(CCKey(currSource->sourceID.getData()), currSource);
			m_Keyword_map.SetAt(CCKey(currKeyHash->keyID.getData()), currKeyHash);
			return true;
		}
		else
		{
//			//Only index so many files per keyword.
//			if( currKeyHash->m_Source_map.GetCount() > KADEMLIAMAXINDEX && !ignoreSize)
//			{
//				delete entry;
//				entry = NULL;
//				return false;
//			}
			Source* currSource;
			if(currKeyHash->m_Source_map.Lookup(CCKey(sourceID.getData()), currSource))
			{
				// FIXME: There should never be an entry in the map which does have an empty 'entryList'
				// We now ownly let the clean method remove entries. This should be fixed.
				if (currSource->entryList.GetCount() > 0)
				{
					delete currSource->entryList.GetHead();
					currSource->entryList.RemoveHead();
				}
				currSource->entryList.AddHead(entry);
				return true;
			}
			else
			{
				currSource = new Source;
				currSource->sourceID.setValue(sourceID);
				currSource->entryList.AddHead(entry);
				currKeyHash->m_Source_map.SetAt(CCKey(currSource->sourceID.getData()), currSource);
				return true;
			}
		}
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CIndexed::writeFile");
		ASSERT(0);
	}
	return false;

}

bool CIndexed::AddSources(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry)
{
	try
	{
		KeyHash* currKeyHash;
		if(!m_Sources_map.Lookup(CCKey(keyID.getData()), currKeyHash))
		{
			Source* currSource = new Source;
			currSource->sourceID.setValue(sourceID);
			currSource->entryList.AddHead(entry);
			currKeyHash = new KeyHash;
			currKeyHash->keyID.setValue(keyID);
			currKeyHash->m_Source_map.SetAt(CCKey(currSource->sourceID.getData()), currSource);
			m_Sources_map.SetAt(CCKey(currKeyHash->keyID.getData()), currKeyHash);
			return true;
		}
		else
		{
			Source* currSource;
			if(currKeyHash->m_Source_map.Lookup(CCKey(sourceID.getData()), currSource))
			{
				for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; )
				{
					POSITION pos6 = pos5;
					Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
					if(currName->ip == entry->ip && currName->tcpport == entry->tcpport)
					{
						currSource->entryList.RemoveAt(pos6);
						delete currName;
						currSource->entryList.AddHead(entry);
						return true;
					}
				}
				//New entry
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
			else
			{
				currSource = new Source;
				currSource->sourceID.setValue(sourceID);
				currSource->entryList.AddHead(entry);
				currKeyHash->m_Source_map.SetAt(CCKey(currSource->sourceID.getData()), currSource);
				return true;
			}
		}
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CIndexed::AddSource");
	}
	return false;
}

bool SearchTermsMatch(const SSearchTerm* pSearchTerm, const Kademlia::CEntry* item, CStringArray& astrFileNameTokens)
{
	// boolean operators
	if (pSearchTerm->type == SSearchTerm::AND)
		return SearchTermsMatch(pSearchTerm->left, item, astrFileNameTokens) && SearchTermsMatch(pSearchTerm->right, item, astrFileNameTokens);
	
	if (pSearchTerm->type == SSearchTerm::OR)
		return SearchTermsMatch(pSearchTerm->left, item, astrFileNameTokens) || SearchTermsMatch(pSearchTerm->right, item, astrFileNameTokens);
	
	if (pSearchTerm->type == SSearchTerm::NAND)
		return SearchTermsMatch(pSearchTerm->left, item, astrFileNameTokens) && !SearchTermsMatch(pSearchTerm->right, item, astrFileNameTokens);

	// word which is to be searched in the file name (and in additional meta data as done by some ed2k servers???)
	if (pSearchTerm->type == SSearchTerm::String)
	{
		int iStrSearchTerms = pSearchTerm->astr->GetCount();
		if (iStrSearchTerms == 0)
			return false;
#if 0
		//TODO: Use a pre-tokenized list for better performance.
		// tokenize the filename (very expensive) only once per search expression and only if really needed
		if (astrFileNameTokens.GetCount() == 0)
		{
			int iPosTok = 0;
			CString strTok(item->fileName.Tokenize(" ()[]{}<>,._-!?", iPosTok));
			while (!strTok.IsEmpty())
			{
				ssh.iFileNameSplits++;
				astrFileNameTokens.Add(strTok);
				strTok = item->fileName.Tokenize(" ()[]{}<>,._-!?", iPosTok);
			}
		}
		if (astrFileNameTokens.GetCount() == 0)
			return false;

		// if there are more than one search strings specified (e.g. "aaa bbb ccc") the entire string is handled
		// like "aaa AND bbb AND ccc". search all strings from the string search term in the tokenized list of
		// the file name. all strings of string search term have to be found (AND)
		for (int iSearchTerm = 0; iSearchTerm < iStrSearchTerms; iSearchTerm++)
		{
			bool bFoundSearchTerm = false;
			for (int i = 0; i < astrFileNameTokens.GetCount(); i++)
			{
				ssh.iStringCmps++;
				// the file name string was already stored in lowercase
				// the string search term was already stored in lowercase
				if (strcmp(astrFileNameTokens.GetAt(i), pSearchTerm->astr->GetAt(iSearchTerm)) == 0)
				{
					bFoundSearchTerm = true;
					break;
				}
			}
			if (!bFoundSearchTerm)
				return false;
		}
#else
		// if there are more than one search strings specified (e.g. "aaa bbb ccc") the entire string is handled
		// like "aaa AND bbb AND ccc". search all strings from the string search term in the tokenized list of
		// the file name. all strings of string search term have to be found (AND)
		for (int iSearchTerm = 0; iSearchTerm < iStrSearchTerms; iSearchTerm++)
		{
			// this will not give the same results as when tokenizing the filename string, but it is 20 times faster.
			if (strstr(item->fileName, pSearchTerm->astr->GetAt(iSearchTerm)) == NULL)
				return false;
		}
#endif
		return true;

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
		}
		return false;*/
	}

	if (pSearchTerm->type == SSearchTerm::MetaTag)
	{
		if (pSearchTerm->tag->m_type == 2) // meta tags with string values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsStr() && pSearchTerm->tag->m_name == tag->m_name)
					return tag->GetStr().CompareNoCase(pSearchTerm->tag->GetStr()) == 0;
			}
		}
	}
	else if (pSearchTerm->type == SSearchTerm::Min)
	{
		if (pSearchTerm->tag->IsInt()) // meta tags with integer values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name == tag->m_name)
					return tag->GetInt() >= pSearchTerm->tag->GetInt();
			}
		}
		else if (pSearchTerm->tag->IsFloat()) // meta tags with float values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name == tag->m_name)
					return tag->GetFloat() >= pSearchTerm->tag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->type == SSearchTerm::Max)
	{
		if (pSearchTerm->tag->IsInt()) // meta tags with integer values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name == tag->m_name)
					return tag->GetInt() <= pSearchTerm->tag->GetInt();
			}
		}
		else if (pSearchTerm->tag->IsFloat()) // meta tags with float values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name == tag->m_name)
					return tag->GetFloat() <= pSearchTerm->tag->GetFloat();
			}
		}
	}

	return false;
}

bool SearchTermsMatch(const SSearchTerm* pSearchTerm, const Kademlia::CEntry* item)
{
	ssh.iSearchTerms++;
	// tokenize the filename (very expensive) only once per search expression and only if really needed
	CStringArray astrFileNameTokens;
	return SearchTermsMatch(pSearchTerm, item, astrFileNameTokens);
}

void CIndexed::SendValidKeywordResult(const CUInt128& keyID, const SSearchTerm* pSearchTerms, uint32 ip, uint16 port)
{
	try
	{
		KeyHash* currKeyHash;
		if(m_Keyword_map.Lookup(CCKey(keyID.getData()), currKeyHash))
		{
			CKademliaUDPListener *udpListner = CKademlia::getUDPListener();
			ASSERT(udpListner != NULL); 
			byte packet[1024*50];
			CByteIO bio(packet,sizeof(packet));
			bio.writeByte(OP_KADEMLIAHEADER);
			bio.writeByte(KADEMLIA_SEARCH_RES);
			bio.writeUInt128(keyID);
			bio.writeUInt16(50);
			uint16 maxResults = 300;
			uint16 count = 0;
			POSITION pos2 = currKeyHash->m_Source_map.GetStartPosition();
			while( pos2 != NULL )
			{
				ssh.iSourceMaps++;
				Source* currSource;
				CCKey key2;
				currKeyHash->m_Source_map.GetNextAssoc( pos2, key2, currSource );
				for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; )
				{
					ssh.iSourceMapsEntries++;
					POSITION pos6 = pos5;
					Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
					if ( !pSearchTerms || SearchTermsMatch(pSearchTerms, currName) )
					{
						if( count < maxResults ){
							bio.writeUInt128(currName->sourceID);
							bio.writeTagList(currName->taglist);
							count++;
							if( count % 50 == 0 )
							{
								uint32 len = sizeof(packet)-bio.getAvailable();
								if (thePrefs.GetDebugClientKadUDPLevel() > 0)
									DebugSend("KadSearchRes", ip, port);
								udpListner->sendPacket(packet, len, ip, port);
								bio.reset();
								bio.writeByte(OP_KADEMLIAHEADER);
								bio.writeByte(KADEMLIA_SEARCH_RES);
								bio.writeUInt128(keyID);
								bio.writeUInt16(50);
							}
						}
					}
				}
			}
			uint16 ccount = count % 50;
			if( ccount )
			{
				uint32 len = sizeof(packet)-bio.getAvailable();
				memcpy(packet+18, &ccount, 2);
				if (thePrefs.GetDebugClientKadUDPLevel() > 0)
					DebugSend("KadSearchRes", ip, port);
				udpListner->sendPacket(packet, len, ip, port);
			}
			clean();
		}
	} 
	catch(...)
	{
		CKademlia::debugLine("Exception in CIndexed::SendValidKeywordResult");
	}
}

void CIndexed::SendValidSourceResult(const CUInt128& keyID, uint32 ip, uint16 port)
{
	try
	{
		KeyHash* currKeyHash;
		if(m_Sources_map.Lookup(CCKey(keyID.getData()), currKeyHash))
		{
			CKademliaUDPListener *udpListner = CKademlia::getUDPListener();
			ASSERT(udpListner != NULL); 
			byte packet[1024*50];
			CByteIO bio(packet,sizeof(packet));
			bio.writeByte(OP_KADEMLIAHEADER);
			bio.writeByte(KADEMLIA_SEARCH_RES);
			bio.writeUInt128(keyID);
			bio.writeUInt16(50);
			uint16 maxResults = 300;
			uint16 count = 0;
			POSITION pos2 = currKeyHash->m_Source_map.GetStartPosition();
			while( pos2 != NULL )
			{
				Source* currSource;
				CCKey key2;
				currKeyHash->m_Source_map.GetNextAssoc( pos2, key2, currSource );
				for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; )
				{
					POSITION pos6 = pos5;
					Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
					if( count < maxResults )
					{
						bio.writeUInt128(currName->sourceID);
						bio.writeTagList(currName->taglist);
						count++;
						if( count % 50 == 0 )
						{
							uint32 len = sizeof(packet)-bio.getAvailable();
							if (thePrefs.GetDebugClientKadUDPLevel() > 0)
								DebugSend("KadSearchRes", ip, port);
							udpListner->sendPacket(packet, len, ip, port);
							bio.reset();
							bio.writeByte(OP_KADEMLIAHEADER);
							bio.writeByte(KADEMLIA_SEARCH_RES);
							bio.writeUInt128(keyID);
							bio.writeUInt16(50);
						}
					}
				}
			}
			uint16 ccount = count % 50;
			if( ccount )
			{
				uint32 len = sizeof(packet)-bio.getAvailable();
				memcpy(packet+18, &ccount, 2);
				if (thePrefs.GetDebugClientKadUDPLevel() > 0)
					DebugSend("KadSearchRes", ip, port);
				udpListner->sendPacket(packet, len, ip, port);
			}
			clean();
		}
	} 
	catch(...)
	{
		CKademlia::debugLine("Exception in CIndexed::SendValidSourceResult");
	}
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
		delete astr;
	delete tag;
}
