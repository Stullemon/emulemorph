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
#include "SearchManager.h"
#include "Search.h"
#include "Kademlia.h"
#include "../../OpCodes.h"
#include "Defines.h"
#include "Tag.h"
#include "../routing/Contact.h"
#include "../utils/UInt128.h"
#include "../utils/MD4.h"
#include "../io/ByteIO.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

uint32 CSearchManager::m_nextID = 0;
SearchMap CSearchManager::m_searches;
CCriticalSection CSearchManager::m_critical;

void CSearchManager::stopSearch(uint32 searchID)
{
	m_critical.Lock();
	try
	{
		SearchMap::iterator it = m_searches.begin(); 
		while (it != m_searches.end())
		{
			if (it->second->m_searchID == searchID)
			{
				delete it->second;
				it = m_searches.erase(it);
			}
			else
				it++;
		}
	} catch (...) {}
	m_critical.Unlock();
}

bool CSearchManager::isNodeSearch(const CUInt128 &target)
{
//  Timer should be the only thing that can delete these..
//  Timer should be the only thing calling this...
//  Therefore there shouldn't be a sync issue at first glance.
//	m_critical.Lock();
	try
	{
		SearchMap::iterator it = m_searches.begin(); 
		while (it != m_searches.end())
		{
			if (it->second->m_target == target)
			{
				if( it->second->m_type == Kademlia::CSearch::NODE || it->second->m_type == Kademlia::CSearch::NODECOMPLETE )
					return true;
				else
					return false;
			}
			else
				it++;
		}
	} catch (...) {}
	return false;
//	m_critical.Unlock();
}

void CSearchManager::stopAllSearches(void)
{
	m_critical.Lock();
	try
	{
		SearchMap::iterator it;
		for (it = m_searches.begin(); it != m_searches.end(); it++)
			delete it->second;
		m_searches.clear();
	} catch (...) {}
	m_critical.Unlock();
}

void CSearchManager::startSearch(CSearch* pSearch)
{
	if (alreadySearchingFor(pSearch->m_target))
	{
		delete pSearch;
		return;
	}
	m_searches[pSearch->m_target] = pSearch;
	pSearch->go();
}

void CSearchManager::deleteSearch(CSearch* pSearch)
{
	delete pSearch;
}

CSearch* CSearchManager::prepareFindKeywords(SEARCH_KEYWORD_CALLBACK callback, uint32 numParams, LPCSTR keyword1, ...)
{
	//uint32 searchID = 0;
	CSearch *s = new CSearch;
	try
	{
		s->m_type = CSearch::KEYWORD;
		s->m_callbackKeyword = callback;

		getWords(keyword1, &s->m_words);
		if (numParams > 1)
		{
			va_list args;
			va_start(args, keyword1);
			while (--numParams > 0)
				getWords(va_arg(args, LPCSTR), &s->m_words);
			va_end(args);
		}
		if (s->m_words.size() == 0)
		{
			delete s;
			return 0;
		}

		CString k = s->m_words.front();

		CMD4::hash((byte*)k.GetBuffer(0), k.GetLength(), &s->m_target);
		//if (alreadySearchingFor(s->m_target)) //-- this may block!
		//{
		//	delete s;
		//	return 0;
		//}

		// Build keyword message (skip first keyword)
		int count = 0;
		byte keywords[4096];
		CByteIO kio(keywords, 4096);
		WordList::const_iterator it = s->m_words.begin();
		for (it++; it != s->m_words.end(); it++)
		{
			if (!(*it).Compare(SEARCH_IMAGE))
			{
				kio.writeTag("Image", "\x03");
				count++;
			}
			else if (!(*it).Compare(SEARCH_AUDIO))
			{
				kio.writeTag("Audio", "\x03");
				count++;
			}
			else if (!(*it).Compare(SEARCH_VIDEO))
			{
				kio.writeTag("Video", "\x03");
				count++;
			}
			else if (!(*it).Compare(SEARCH_DOC))
			{
				kio.writeTag("Doc", "\x03");
				count++;
			}
			else if (!(*it).Compare(SEARCH_PRO))
			{
				kio.writeTag("Pro", "\x03");
				count++;
			}
			else
			{
				kio.writeTag(1, *it);
				count++;
			}
		}

		// Write complete packet
		s->m_searchTerms = new byte[4096];
		CByteIO bio(s->m_searchTerms, 4096);
		bio.writeByte(OP_KADEMLIAHEADER);
		bio.writeByte(KADEMLIA_SEARCH_REQ);
		bio.writeUInt128(s->m_target);
		if (count == 0)
			bio.writeByte(0);
		else
		{
			bio.writeByte(1);
			for (int i=0; i<(count - 1); i++)
				bio.writeUInt16(0); // These are the operator / logical ANDs
			bio.writeArray(keywords, 4096 - kio.getAvailable());
		}

		s->m_lenSearchTerms = 4096 - bio.getAvailable();
		//m_searches[s->m_target] = s;

		//searchID = ++m_nextID;
		//s->m_searchID = searchID;
		// NOTE: the following line does only work because this function is (currently) invoked from emule thread only!
		// If that function once gets called from a different thread, the handling with 'm_nextID' has to by synchronized.
		s->m_searchID = ++m_nextID;

		//s->go(); //-- this may block!

	} catch (...) {
		delete s;
		s = NULL;
	}
	return s;
}

CSearch* CSearchManager::prepareFindKeywords(SEARCH_KEYWORD_CALLBACK callback, LPCSTR keyword1, UINT uSearchTermsSize, LPBYTE pucSearchTermsData)
{
	CSearch *s = new CSearch;
	try
	{
		s->m_type = CSearch::KEYWORD;
		s->m_callbackKeyword = callback;

		getWords(keyword1, &s->m_words);
		if (s->m_words.size() == 0)
		{
			delete s;
			return 0;
		}
		CString k = s->m_words.front();
		CMD4::hash((byte*)k.GetBuffer(0), k.GetLength(), &s->m_target);

		// Write complete packet
		s->m_searchTerms = new byte[4096];
		CByteIO bio(s->m_searchTerms, 4096);
		bio.writeByte(OP_KADEMLIAHEADER);
		bio.writeByte(KADEMLIA_SEARCH_REQ);
		bio.writeUInt128(s->m_target);
		if (uSearchTermsSize == 0)
			bio.writeByte(0);
		else
		{
			bio.writeByte(1);
			bio.writeArray(pucSearchTermsData, uSearchTermsSize);
		}

		s->m_lenSearchTerms = 4096 - bio.getAvailable();
		s->m_searchID = ++m_nextID;
	} catch (...) {
		delete s;
		s = NULL;
	}
	return s;
}

CSearch* CSearchManager::prepareFindFile(SEARCH_ID_CALLBACK callback, const CUInt128 &id)
{
	//if (alreadySearchingFor(id)) //-- this may block!
	//	return 0;

	//uint32 searchID = 0;
	CSearch *s = new CSearch;
	try
	{
		s->m_type = CSearch::FILE;
		s->m_target = id;
		s->m_callbackID = callback;

		// Write complete packet
		s->m_searchTerms = new byte[4096];
		CByteIO bio(s->m_searchTerms, 4096);
		bio.writeByte(OP_KADEMLIAHEADER);
		bio.writeByte(KADEMLIA_SEARCH_REQ);
		bio.writeUInt128(s->m_target);
		bio.writeByte(1);

		s->m_lenSearchTerms = 4096 - bio.getAvailable();
		//m_searches[s->m_target] = s;

		//searchID = ++m_nextID;
		//s->m_searchID = searchID;
		// NOTE: the following line does only work because this function is (currently) invoked from emule thread only!
		// If that function once gets called from a different thread, the handling with 'm_nextID' has to by synchronized.
		s->m_searchID = ++m_nextID;

		//s->go(); //-- this may block!

	} catch (...) {
		delete s;
		s = NULL;
	}
	return s;
}

// as long as this function is only called from the CTimer thread itself, there should be no
// sync/deadlocks problems. if it is called from the emule thread it has to be changed in
// the same way as findFile and findKeywords.
void CSearchManager::findNode(const CUInt128 &id)
{
	if (alreadySearchingFor(id))
		return;

	CString idStr;
	id.toHexString(&idStr);
//	CKademlia::debugMsg("Find Node: %s", idStr);

	try
	{
		CSearch *s = new CSearch;
		s->m_type = CSearch::NODE;
		s->m_target = id;
		s->m_searchTerms = NULL;
		s->m_lenSearchTerms = 0;
		s->m_searchID = 0;
		m_searches[s->m_target] = s;
		s->go();
	} catch (...) {}
	return;
}

void CSearchManager::findNodeComplete(const CUInt128 &id)
{
	if (alreadySearchingFor(id))
		return;

	CString idStr;
	id.toHexString(&idStr);
//	CKademlia::debugMsg("Find Node: %s", idStr);

	try
	{
		CSearch *s = new CSearch;
		s->m_type = CSearch::NODECOMPLETE;
		s->m_target = id;
		s->m_searchTerms = NULL;
		s->m_lenSearchTerms = 0;
		s->m_searchID = 0;
		m_searches[s->m_target] = s;
		s->go();
	} catch (...) {}
	return;
}

bool CSearchManager::alreadySearchingFor(const CUInt128 &target)
{
	bool retVal = false;
	m_critical.Lock();
	try
	{
		retVal = (m_searches.count(target) > 0);
	} catch (...) {}
	m_critical.Unlock();
	return retVal;
}

void CSearchManager::getWords(LPCSTR str, WordList *words)
{
	LPSTR s = (LPSTR)str;
	int len;
	CString word;
	while (strlen(s) > 0)
	{
		len = (int)strcspn(s, " ()[]{}<>,._-!?");
		if (len > 2)
		{
			word = s;
			word.Truncate(len);
			word.MakeLower();
			words->push_back(word);
		}
//		else if ((len == 0) && (s[0] == '-'))
//		{
//			len = (int)strcspn(s, " ");
//			if ((!strnicmp(s, SEARCH_IMAGE, len))
//			||  (!strnicmp(s, SEARCH_AUDIO, len))
//			||  (!strnicmp(s, SEARCH_VIDEO, len))
//			||  (!strnicmp(s, SEARCH_DOC  , len))
//			||  (!strnicmp(s, SEARCH_PRO  , len)))
//			{
//				word = s;
//				word.Truncate(len);
//				word.MakeLower();
//				words->push_back(word);
///			}
//			else 
//				len = 0;
//		}
		if (len < (int)strlen(s))
			len++;
		s += len;
	}
	if(words->size() > 1 && len == 3)
	{
		words->pop_back();
	}
}

void CSearchManager::jumpStart(void)
{
	time_t now = time(NULL);
	m_critical.Lock();
	try
	{
		SearchMap::iterator it = m_searches.begin(); 
		while (it != m_searches.end())
		{
			switch(it->second->getSearchTypes()){
				case CSearch::FILE:
				{
					if ((it->second->m_created + SEARCHFILE_LIFETIME < now)||(it->second->getCount() > SEARCHFILE_TOTAL))
					{
						delete it->second;
						it = m_searches.erase(it);
					}
					else
					{
						it->second->jumpStart();
						it++;
					}
					break;
				}
				case CSearch::KEYWORD:
				{
					if ((it->second->m_created + SEARCHKEYWORD_LIFETIME < now)||(it->second->getCount() > SEARCHKEYWORD_TOTAL))
					{
						delete it->second;
						it = m_searches.erase(it);
					}
					else
					{
						it->second->jumpStart();
						it++;
					}
					break;
				}
				case CSearch::NODE:
				case CSearch::NODECOMPLETE:
				{
					if (it->second->m_created + SEARCHNODE_LIFETIME < now)
					{
						delete it->second;
						it = m_searches.erase(it);
					}
					else
					{
						it->second->jumpStart();
						it++;
					}
					break;
				}
				case CSearch::STOREFILE:
				{
					if ((it->second->m_created + SEARCHSTOREFILE_LIFETIME < now)||(it->second->getCount() > SEARCHSTOREFILE_TOTAL))
					{
						delete it->second;
						it = m_searches.erase(it);
					}
					else
					{
						it->second->jumpStart();
						it++;
					}
					break;
				}
				case CSearch::STOREKEYWORD:
				{
					if ((it->second->m_created + SEARCHSTOREKEYWORD_LIFETIME < now)||(it->second->getCount() > SEARCHSTOREKEYWORD_TOTAL))
					{
						delete it->second;
						it = m_searches.erase(it);
					}
					else
					{
						it->second->jumpStart();
						it++;
					}
					break;
				}
				default:
				{
					if (it->second->m_created + SEARCH_LIFETIME < now)
					{
						delete it->second;
						it = m_searches.erase(it);
					}
					else
					{
						it->second->jumpStart();
						it++;
					}
					break;
				}
			}
		}
	} catch (...) {}
	m_critical.Unlock();
}

void CSearchManager::updateStats(void)
{
	uint8 m_totalFile = 0;
	uint8 m_totalStore = 0;
	m_critical.Lock();
	try
	{
		SearchMap::iterator it = m_searches.begin(); 
		while (it != m_searches.end())
		{
			switch(it->second->getSearchTypes()){
				case CSearch::FILE:
				{
					m_totalFile++;
					it++;
					break;
				}
				case CSearch::STOREFILE:
				case CSearch::STOREKEYWORD:
				{
					m_totalStore++;
					it++;
					break;
				}
				default:
					it++;
					break;
			}
		}
	} catch (...) {}
	m_critical.Unlock();
	CPrefs *prefs = CKademlia::getPrefs();
	if(prefs != NULL)
	{
		prefs->setTotalFile(m_totalFile);
		prefs->setTotalStore(m_totalStore);
	}
}

void CSearchManager::processPublishResult(const CUInt128 &target)
{
	CSearch *s = NULL;
	m_critical.Lock();
	try
	{
		SearchMap::const_iterator it = m_searches.find(target);
		if (it != m_searches.end())
			s = it->second;
	} catch (...) {}
	m_critical.Unlock();

	if (s == NULL)
		return;
	s->m_count++;
}


void CSearchManager::processResponse(const CUInt128 &target, uint32 fromIP, uint16 fromPort, ContactList *results)
{
	CSearch *s = NULL;
	m_critical.Lock();
	try
	{
		SearchMap::const_iterator it = m_searches.find(target);
		if (it != m_searches.end())
			s = it->second;
	} catch (...) {}
	m_critical.Unlock();

	if (s == NULL)
	{
		ContactList::const_iterator it2;
		for (it2 = results->begin(); it2 != results->end(); it2++)
			delete (*it2);
		delete results;
		return;
	}
	else
		s->processResponse(target, fromIP, fromPort, results);
}

void CSearchManager::processResult(const CUInt128 &target, uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagList *info)
{
	CSearch *s = NULL;
	m_critical.Lock();
	try
	{
		SearchMap::const_iterator it = m_searches.find(target);
		if (it != m_searches.end())
			s = it->second;
	} catch (...) {}
	m_critical.Unlock();

	if (s == NULL)
	{
		TagList::const_iterator it;
		for (it = info->begin(); it != info->end(); it++)
			delete *it;
		delete info;
	}
	else
		s->processResult(target, fromIP, fromPort, answer, info);
}