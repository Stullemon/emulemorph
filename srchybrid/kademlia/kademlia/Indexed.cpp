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
#include "../utils/UInt128.h"
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

CString CIndexed::m_kfilename;
CString CIndexed::m_sfilename;

CIndexed::CIndexed()
{
	m_Keyword_map.InitHashTable(1031);
	m_sfilename = CMiscUtils::getAppDir();
	m_sfilename.Append(CONFIGFOLDER);
	m_sfilename.Append(_T("src_index.dat"));
	m_kfilename = CMiscUtils::getAppDir();
	m_kfilename.Append(CONFIGFOLDER);
	m_kfilename.Append(_T("key_index.dat"));
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
		
		CBufferedFileIO k_file;
		if (k_file.Open(m_kfilename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite))
		{
			setvbuf(k_file.m_pStream, NULL, _IOFBF, 32768);

			uint32 version = k_file.readUInt32();
			if( version > 1 )
				return;

			time_t savetime = k_file.readUInt32();
			if( savetime < time(NULL) - (KADEMLIAREPUBLISHTIMEK) )
				return;

			ASSERT(Kademlia::CKademlia::getPrefs() != NULL);

			CUInt128 id;

			k_file.readUInt128(&id);
			if( Kademlia::CKademlia::getPrefs()->getKadID().compareTo(id) )
				return;

			numKeys = k_file.readUInt32();
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
						uint32 expire = k_file.readUInt32();
						toaddN->lifetime = expire;
						tagList = k_file.readByte();
						while( tagList )
						{
							CTag* tag = k_file.readTag();
							if(tag)
							{
								if (!tag->m_name.Compare(TAG_NAME))
								{
									toaddN->fileName = tag->GetStr();
									KadTagStrMakeLower(toaddN->fileName); // make lowercase, the search code expects lower case strings!
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
							}
							tagList--;
						}
						toaddN->keyID.setValue(keyID);
						toaddN->sourceID.setValue(sourceID);
						uint8 load = 0;
						if(AddKeyword(keyID, sourceID, toaddN, load))
							totalKeyword++;
						else
							delete toaddN;
						numName--;
					}
					numSource--;
				}
				numKeys--;
			}
			k_file.Close();
		}

		CBufferedFileIO s_file;
		if (s_file.Open(m_sfilename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyWrite))
		{
			setvbuf(s_file.m_pStream, NULL, _IOFBF, 32768);

			uint32 version = s_file.readUInt32();
			if( version > 1 )
				return;

			time_t savetime = s_file.readUInt32();
			if( savetime < time(NULL) - (KADEMLIAREPUBLISHTIMES) )
				return;

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
							if(tag)
							{
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
							}
							tagList--;
						}
						toaddN->keyID.setValue(keyID);
						toaddN->sourceID.setValue(sourceID);
						uint8 load = 0;
						if(AddSources(keyID, sourceID, toaddN, load))
							totalSource++;
						else
							delete toaddN;
						numName--;
					}
					numSource--;
				}
				numKeys--;
			}
			s_file.Close();

			m_totalIndexSource = totalSource;
			m_totalIndexKeyword = totalKeyword;
			AddDebugLogLine( false, _T("Read %u source and %u keyword entries"), totalSource, totalKeyword);
		}
	} 
	catch ( CIOException *ioe )
	{
		AddDebugLogLine( false, _T("Exception in CIndexed::readFile (IO error(%i))"), ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		AddDebugLogLine(false, _T("Exception in CIndexed::readFile"));
	}
}

CIndexed::~CIndexed()
{
	try
	{
		uint32 s_total = 0;
		uint32 k_total = 0;

		CBufferedFileIO s_file;
		if (s_file.Open(m_sfilename, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary | CFile::shareDenyWrite))
		{
			setvbuf(s_file.m_pStream, NULL, _IOFBF, 32768);

			uint32 version = 1;
			s_file.writeUInt32(version);

			s_file.writeUInt32(time(NULL));

			CCKey key;
			CCKey key2;

			s_file.writeUInt32(m_Sources_map.GetCount());
			POSITION pos = m_Sources_map.GetStartPosition();
			while( pos != NULL )
			{
				SrcHash* currSrcHash;
				m_Sources_map.GetNextAssoc( pos, key, currSrcHash );
				s_file.writeUInt128(currSrcHash->keyID);

				CKadSourcePtrList& KeyHashSrcMap = currSrcHash->m_Source_map;
				s_file.writeUInt32(KeyHashSrcMap.GetCount());
				POSITION pos2 = KeyHashSrcMap.GetHeadPosition();
				while( pos2 != NULL )
				{
					Source* currSource = KeyHashSrcMap.GetNext(pos2);
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
				delete currSrcHash;
			}
			s_file.Close();
		}

		CBufferedFileIO k_file;
		if (k_file.Open(m_kfilename, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary | CFile::shareDenyWrite))
		{
			setvbuf(k_file.m_pStream, NULL, _IOFBF, 32768);

			CCKey key;
			CCKey key2;

			uint32 version = 1;
			k_file.writeUInt32(version);

			k_file.writeUInt32(time(NULL));

			ASSERT(Kademlia::CKademlia::getPrefs() != NULL);
			k_file.writeUInt128(Kademlia::CKademlia::getPrefs()->getKadID());

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
		AddDebugLogLine( false, _T("Wrote %u source and %u keyword entries"), s_total, k_total);

		CCKey key;
		CCKey key2;
		POSITION pos = m_Notes_map.GetStartPosition();
		while( pos != NULL )
		{
			KeyHash* currKeyHash;
			m_Notes_map.GetNextAssoc( pos, key, currKeyHash );
			POSITION pos2 = currKeyHash->m_Source_map.GetStartPosition();
			while( pos2 != NULL )
			{
				Source* currSource;
				currKeyHash->m_Source_map.GetNextAssoc( pos2, key2, currSource );
				CKadEntryPtrList& SrcEntryList = currSource->entryList;
				for(POSITION pos5 = SrcEntryList.GetHeadPosition(); pos5 != NULL; )
				{
					Kademlia::CEntry* currName = SrcEntryList.GetNext(pos5);
					delete currName;
				}
				delete currSource;
			}
			delete currKeyHash;
		} 
	}
	catch ( CIOException *ioe )
	{
		AddDebugLogLine( false, _T("Exception in CIndexed::~CIndexed (IO error(%i))"), ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		AddDebugLogLine(false, _T("Exception in CIndexed::~CIndexed"));
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
			SrcHash* currSrcHash;
			CCKey key;
			m_Sources_map.GetNextAssoc( pos, key, currSrcHash );
			for(POSITION pos2 = currSrcHash->m_Source_map.GetHeadPosition(); pos2 != NULL; )
			{
				POSITION pos3 = pos2;
				Source* currSource = currSrcHash->m_Source_map.GetNext(pos2);
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
					currSrcHash->m_Source_map.RemoveAt(pos3);
					delete currSource;
				}
			}
			if( currSrcHash->m_Source_map.IsEmpty())
			{
				m_Sources_map.RemoveKey(key);
				delete currSrcHash;
			}
		}

		m_totalIndexSource = s_Total;
		m_totalIndexKeyword = k_Total;
		AddDebugLogLine( false, _T("Removed %u keyword out of %u and %u source out of %u"), k_Removed, k_Total, s_Removed, s_Total);
		m_lastClean = time(NULL) + (60*30);
	} 
	catch(...)
	{
		AddDebugLogLine(false, _T("Exception in CIndexed::clean"));
		ASSERT(0);
	}
}

bool CIndexed::AddKeyword(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8& load){
	try
	{
		if( !entry )
			return false;

		if( m_totalIndexKeyword > KADEMLIAMAXENTRIES )
		{
			load = 100;
			return false;
		}

		if( entry->size == 0 || entry->fileName.IsEmpty() || entry->taglist.size() == 0 || entry->lifetime < time(NULL) - (KADEMLIAREPUBLISHTIMEK))
			return false;

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
			load = 1;
			m_totalIndexKeyword++;
			return true;
		}
		else
		{
			uint32 indexTotal = currKeyHash->m_Source_map.GetCount();
			if ( indexTotal > KADEMLIAMAXINDEX )
			{
				load = 100;
				//Too many entries for this Keyword..
				return false;
			}
			Source* currSource;
			if(currKeyHash->m_Source_map.Lookup(CCKey(sourceID.getData()), currSource))
			{
				if (currSource->entryList.GetCount() > 0)
				{
					if( indexTotal > KADEMLIAMAXINDEX - 5000 )
					{
						load = 100;
						//We are in a hot node.. If we continued to update all the publishes
						//while this index is full, popular files will be the only thing you index.
						return false;
					}
					delete currSource->entryList.GetHead();
					currSource->entryList.RemoveHead();
				}
				else
					m_totalIndexKeyword++;
				load = (indexTotal*100)/KADEMLIAMAXINDEX;
				currSource->entryList.AddHead(entry);
				return true;
			}
			else
			{
				currSource = new Source;
				currSource->sourceID.setValue(sourceID);
				currSource->entryList.AddHead(entry);
				currKeyHash->m_Source_map.SetAt(CCKey(currSource->sourceID.getData()), currSource);
				m_totalIndexKeyword++;
				load = (indexTotal*100)/KADEMLIAMAXINDEX;
				return true;
			}
		}
	}
	catch(...)
	{
		AddDebugLogLine(false, _T("Exception in CIndexed::writeFile"));
		ASSERT(0);
	}
	return false;

}

bool CIndexed::AddSources(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8& load)
{
	if( !entry )
		return false;
	if( entry->ip == 0 || entry->tcpport == 0 || entry->udpport == 0 || entry->taglist.size() == 0 || entry->lifetime < time(NULL) - (KADEMLIAREPUBLISHTIMES))
		return false;
	try
	{
		SrcHash* currSrcHash;
		if(!m_Sources_map.Lookup(CCKey(keyID.getData()), currSrcHash))
		{
			Source* currSource = new Source;
			currSource->sourceID.setValue(sourceID);
			currSource->entryList.AddHead(entry);
			currSrcHash = new SrcHash;
			currSrcHash->keyID.setValue(keyID);
			currSrcHash->m_Source_map.AddHead(currSource);
			m_Sources_map.SetAt(CCKey(currSrcHash->keyID.getData()), currSrcHash);
			m_totalIndexSource++;
			load = 1;
			return true;
		}
		else
		{
			uint32 size = currSrcHash->m_Source_map.GetSize();
			for(POSITION pos2 = currSrcHash->m_Source_map.GetHeadPosition(); pos2 != NULL; )
			{
				Source* currSource = currSrcHash->m_Source_map.GetNext(pos2);
				if( currSource->entryList.GetSize() )
				{
					CEntry* currEntry = currSource->entryList.GetHead();
					ASSERT(currEntry!=NULL);
					if( currEntry->ip == entry->ip && ( currEntry->tcpport == entry->tcpport || currEntry->udpport == entry->udpport ))
					{
						//Currently we only allow one entry per Hash.
						Kademlia::CEntry* currName = currSource->entryList.RemoveHead();
						delete currName;
						currSource->entryList.AddHead(entry);
						load = (size*100)/KADEMLIAMAXSOUCEPERFILE;
						return true;
					}
				}
				else
				{
					//This should never happen!
					currSource->entryList.AddHead(entry);
					ASSERT(0);
					load = (size*100)/KADEMLIAMAXSOUCEPERFILE;
					return true;
				}
			}
			if( size > KADEMLIAMAXSOUCEPERFILE )
			{
				Source* currSource = currSrcHash->m_Source_map.RemoveTail();
				Kademlia::CEntry* currName = currSource->entryList.RemoveTail();
				if( currName )
					delete currName;
				else
					ASSERT(0);
				if( !currSource )
					ASSERT(0);
				currSource->sourceID.setValue(sourceID);
				currSource->entryList.AddHead(entry);
				currSrcHash->m_Source_map.AddHead(currSource);
				load = 100;
				return true;
			}
			else
			{
				Source* currSource = new Source;
				currSource->sourceID.setValue(sourceID);
				currSource->entryList.AddHead(entry);
				currSrcHash->m_Source_map.AddHead(currSource);
				m_totalIndexSource++;
				load = (size*100)/KADEMLIAMAXSOUCEPERFILE;
				return true;
			}
		}
	}
	catch(...)
	{
		AddDebugLogLine(false, _T("Exception in CIndexed::AddSource"));
	}
	return false;
}

bool CIndexed::AddNotes(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry)
{
	try
	{
		KeyHash* currKeyHash;
		if(!m_Notes_map.Lookup(CCKey(keyID.getData()), currKeyHash))
		{
			Source* currSource = new Source;
			currSource->sourceID.setValue(sourceID);
			currSource->entryList.AddHead(entry);
			currKeyHash = new KeyHash;
			currKeyHash->keyID.setValue(keyID);
			currKeyHash->m_Source_map.SetAt(CCKey(currSource->sourceID.getData()), currSource);
			m_Notes_map.SetAt(CCKey(currKeyHash->keyID.getData()), currKeyHash);
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
				if( currSource->entryList.GetCount() > KADEMLIAMAXNOTESPERETRY )
				{
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
		AddDebugLogLine(false, _T("Exception in CIndexed::AddSource"));
	}
	return false;
}

bool SearchTermsMatch(const SSearchTerm* pSearchTerm, const Kademlia::CEntry* item/*, CStringArray& astrFileNameTokens*/)
{
	// boolean operators
	if (pSearchTerm->type == SSearchTerm::AND)
		return SearchTermsMatch(pSearchTerm->left, item/*, astrFileNameTokens*/) && SearchTermsMatch(pSearchTerm->right, item/*, astrFileNameTokens*/);
	
	if (pSearchTerm->type == SSearchTerm::OR)
		return SearchTermsMatch(pSearchTerm->left, item/*, astrFileNameTokens*/) || SearchTermsMatch(pSearchTerm->right, item/*, astrFileNameTokens*/);
	
	if (pSearchTerm->type == SSearchTerm::NAND)
		return SearchTermsMatch(pSearchTerm->left, item/*, astrFileNameTokens*/) && !SearchTermsMatch(pSearchTerm->right, item/*, astrFileNameTokens*/);

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
			CString strTok(item->fileName.Tokenize(_aszInvKadKeywordChars, iPosTok));
			while (!strTok.IsEmpty())
			{
				astrFileNameTokens.Add(strTok);
				strTok = item->fileName.Tokenize(_aszInvKadKeywordChars, iPosTok);
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
			if (wcsstr(item->fileName, pSearchTerm->astr->GetAt(iSearchTerm)) == NULL)
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
				CString strTok(static_cast<const CTagStr *>(tag)->m_value.Tokenize(_aszInvKadKeywordChars, iPos));
				while (!strTok.IsEmpty()){
					if (stricmp(strTok, *(pSearchTerm->str)) == 0)
						return true;
					strTok = static_cast<const CTagStr *>(tag)->m_value.Tokenize(_aszInvKadKeywordChars, iPos);
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
				if (tag->IsStr() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetStr().CompareNoCase(pSearchTerm->tag->GetStr()) == 0;
			}
		}
	}
	else if (pSearchTerm->type == SSearchTerm::OpGreaterEqual)
	{
		if (pSearchTerm->tag->IsInt()) // meta tags with integer values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetInt() >= pSearchTerm->tag->GetInt();
			}
		}
		else if (pSearchTerm->tag->IsFloat()) // meta tags with float values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetFloat() >= pSearchTerm->tag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->type == SSearchTerm::OpLessEqual)
	{
		if (pSearchTerm->tag->IsInt()) // meta tags with integer values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetInt() <= pSearchTerm->tag->GetInt();
			}
		}
		else if (pSearchTerm->tag->IsFloat()) // meta tags with float values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetFloat() <= pSearchTerm->tag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->type == SSearchTerm::OpGreater)
	{
		if (pSearchTerm->tag->IsInt()) // meta tags with integer values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetInt() > pSearchTerm->tag->GetInt();
			}
		}
		else if (pSearchTerm->tag->IsFloat()) // meta tags with float values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetFloat() > pSearchTerm->tag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->type == SSearchTerm::OpLess)
	{
		if (pSearchTerm->tag->IsInt()) // meta tags with integer values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetInt() < pSearchTerm->tag->GetInt();
			}
		}
		else if (pSearchTerm->tag->IsFloat()) // meta tags with float values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetFloat() < pSearchTerm->tag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->type == SSearchTerm::OpEqual)
	{
		if (pSearchTerm->tag->IsInt()) // meta tags with integer values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetInt() == pSearchTerm->tag->GetInt();
			}
		}
		else if (pSearchTerm->tag->IsFloat()) // meta tags with float values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetFloat() == pSearchTerm->tag->GetFloat();
			}
		}
	}
	else if (pSearchTerm->type == SSearchTerm::OpNotEqual)
	{
		if (pSearchTerm->tag->IsInt()) // meta tags with integer values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsInt() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetInt() != pSearchTerm->tag->GetInt();
			}
		}
		else if (pSearchTerm->tag->IsFloat()) // meta tags with float values
		{
			TagList::const_iterator it;
			for (it = item->taglist.begin(); it != item->taglist.end(); it++)
			{
				const Kademlia::CTag* tag = *it;
				if (tag->IsFloat() && pSearchTerm->tag->m_name.Compare(tag->m_name) == 0)
					return tag->GetFloat() != pSearchTerm->tag->GetFloat();
			}
		}
	}

	return false;
}

//bool SearchTermsMatch(const SSearchTerm* pSearchTerm, const Kademlia::CEntry* item)
//{
//	// tokenize the filename (very expensive) only once per search expression and only if really needed
//	CStringArray astrFileNameTokens;
//	return SearchTermsMatch(pSearchTerm, item, astrFileNameTokens);
//}

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
				Source* currSource;
				CCKey key2;
				currKeyHash->m_Source_map.GetNextAssoc( pos2, key2, currSource );
				for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; )
				{
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
		AddDebugLogLine(false, _T("Exception in CIndexed::SendValidKeywordResult"));
	}
}

void CIndexed::SendValidSourceResult(const CUInt128& keyID, uint32 ip, uint16 port)
{
	try
	{
		SrcHash* currSrcHash;
		if(m_Sources_map.Lookup(CCKey(keyID.getData()), currSrcHash))
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
			for(POSITION pos2 = currSrcHash->m_Source_map.GetHeadPosition(); pos2 != NULL; )
			{
				Source* currSource = currSrcHash->m_Source_map.GetNext(pos2);
				if( currSource->entryList.GetSize() )
				{
					Kademlia::CEntry* currName = currSource->entryList.GetHead();
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
		AddDebugLogLine(false, _T("Exception in CIndexed::SendValidSourceResult"));
	}
}

void CIndexed::SendValidNoteResult(const CUInt128& keyID, const CUInt128& sourceID, uint32 ip, uint16 port)
{
	try
	{
		KeyHash* currKeyHash;
		if(m_Notes_map.Lookup(CCKey(keyID.getData()), currKeyHash))
		{
			CKademliaUDPListener *udpListner = CKademlia::getUDPListener();
			ASSERT(udpListner != NULL); 
			uint16 count = 0;
			Source* currSource;
			if(currKeyHash->m_Source_map.Lookup(CCKey(sourceID.getData()), currSource))
			{
				if( currSource->entryList.GetCount() == 0 )
					return;
				byte packet[1024*50];
				CByteIO bio(packet,sizeof(packet));
				bio.writeByte(OP_KADEMLIAHEADER);
				bio.writeByte(KADEMLIA_SRC_NOTES_RES);
				bio.writeUInt128(keyID);
				bio.writeUInt16(currSource->entryList.GetCount());
				for(POSITION pos5 = currSource->entryList.GetHeadPosition(); pos5 != NULL; )
				{
					POSITION pos6 = pos5;
					Kademlia::CEntry* currName = currSource->entryList.GetNext(pos5);
					bio.writeUInt128(currName->sourceID);
					bio.writeTagList(currName->taglist);
				}
				if (thePrefs.GetDebugClientKadUDPLevel() > 0)
					DebugSend("KadNotesRes", ip, port);
				uint32 len = sizeof(packet)-bio.getAvailable();
				udpListner->sendPacket(packet, len, ip, port);
			}
			if( count )
			{
			}
			clean();
		}
	} 
	catch(...)
	{
		AddDebugLogLine(false, _T("Exception in CIndexed::SendValidSourceResult"));
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
