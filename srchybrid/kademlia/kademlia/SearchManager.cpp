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
#include "resource.h"
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
#include "../io/IOException.h"
#include "../kademlia/prefs.h"
#include "SafeFile.h"
#include "OtherFunctions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


LPCTSTR _aszInvKadKeywordChars = " ()[]{}<>,._-!?";

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

uint32 CSearchManager::m_nextID = 0;
SearchMap CSearchManager::m_searches;
CCriticalSection CSearchManager::m_critical;

void CSearchManager::stopSearch(uint32 searchID, bool delayDelete)
{
	m_critical.Lock();
	try
	{
		SearchMap::iterator it = m_searches.begin(); 
		while (it != m_searches.end())
		{
			if (it->second->m_searchID == searchID)
			{
				if(delayDelete)
				{
					it->second->prepareToStop();
					it++;
				}
				else
				{
					delete it->second;
					it = m_searches.erase(it);
				}
			}
			else
				it++;
		}
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::stopSearch");
	}
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
				if( it->second->m_type == CSearch::NODE || it->second->m_type == CSearch::NODECOMPLETE )
					return true;
				else
					return false;
			}
			else
				it++;
		}
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::isNodeSearch");
	}
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
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::stopAllSearches");
	}
	m_critical.Unlock();
}

bool CSearchManager::startSearch(CSearch* pSearch)
{
	if (alreadySearchingFor(pSearch->m_target))
	{
		delete pSearch;
		return false;
	}
	m_searches[pSearch->m_target] = pSearch;
	pSearch->go();
	return true;
}

void CSearchManager::deleteSearch(CSearch* pSearch)
{
	delete pSearch;
}

CSearch* CSearchManager::prepareFindKeywords(uint32 type, bool start, LPCSTR keyword1, UINT uSearchTermsSize, LPBYTE pucSearchTermsData)
{
	CSearch *s = new CSearch;
	try
	{
		s->m_type = type;

		getWords(keyword1, &s->m_words);
		if (s->m_words.size() == 0)
		{
			delete s;
			throw GetResString(IDS_KAD_SEARCH_KEYWORD_TOO_SHORT);
		}

		CString k = s->m_words.front();
		CMD4::hash((byte*)k.GetBuffer(0), k.GetLength(), &s->m_target);
		if (alreadySearchingFor(s->m_target))
		{
			delete s;
			CString strError;
			strError.Format(GetResString(IDS_KAD_SEARCH_KEYWORD_ALREADY_SEARCHING), k);
			throw strError;
		}

		// Write complete packet
		s->m_searchTerms = new CSafeMemFile();
		s->m_searchTerms->WriteUInt128(&s->m_target);
		if (uSearchTermsSize == 0)
			s->m_searchTerms->WriteUInt8(0);
		else
		{
			s->m_searchTerms->WriteUInt8(1);
			s->m_searchTerms->Write(pucSearchTermsData, uSearchTermsSize);
		}

		s->m_searchID = ++m_nextID;
		if( start )
		{
			m_searches[s->m_target] = s;
			s->go();
		}
	} 
	catch (CIOException* ioe)
	{
		CString strError;
		strError.Format(_T("IO-Exception in %s: Error %u"), __FUNCTION__, ioe->m_cause);
		ioe->Delete();
		delete s;
		throw strError;
	}
	catch (CFileException* e)
	{
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		e->m_strFileName = "search packet";
		e->GetErrorMessage(szError, ARRSIZE(szError));
		CString strError;
		strError.Format(_T("Exception in %s: %s"), __FUNCTION__, szError);
		e->Delete();
		delete s;
		throw strError;
	}
	catch (CString strException)
	{
		throw strException;
	}
	catch (...) 
	{
		CString strError;
		strError.Format(_T("Unknown exception in %s"), __FUNCTION__);
		delete s;
		throw strError;
	}
	return s;
}

CSearch* CSearchManager::prepareFindFile(uint32 type, bool start, const CUInt128 &id)
{
	if (alreadySearchingFor(id))
		return 0;

	CSearch *s = new CSearch;
	try
	{
		s->m_type = type;
		s->m_target = id;

		// Write complete packet
		s->m_searchTerms = new CSafeMemFile();
		s->m_searchTerms->WriteUInt128(&s->m_target);
		s->m_searchTerms->WriteUInt8(1);

		s->m_searchID = ++m_nextID;
		if( start )
		{
			m_searches[s->m_target] = s;
			s->go();
		}
	} 
	catch ( CIOException *ioe )
	{
		CKademlia::debugMsg("Exception in CSearchManager::prepareFindFile (IO error(%i))", ioe->m_cause);
		ioe->Delete();
		delete s;
		s = NULL;
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::prepareFindFile");
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

//	CString idStr;
//	id.toHexString(&idStr);
//	CKademlia::debugMsg("Find Node: %s", idStr);

	try
	{
		CSearch *s = new CSearch;
		s->m_type = CSearch::NODE;
		s->m_target = id;
		s->m_searchTerms = NULL;
		s->m_searchID = 0;
		m_searches[s->m_target] = s;
		s->go();
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::findNode");
	}
	return;
}

void CSearchManager::findNodeComplete(const CUInt128 &id)
{
	if (alreadySearchingFor(id))
		return;

//	CString idStr;
//	id.toHexString(&idStr);
//	CKademlia::debugMsg("Find Node: %s", idStr);

	try
	{
		CSearch *s = new CSearch;
		s->m_type = CSearch::NODECOMPLETE;
		s->m_target = id;
		s->m_searchTerms = NULL;
		s->m_searchID = 0;
		m_searches[s->m_target] = s;
		s->go();
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::findNodeComplete");
	}
	return;
}

bool CSearchManager::alreadySearchingFor(const CUInt128 &target)
{
	bool retVal = false;
	m_critical.Lock();
	try
	{
		retVal = (m_searches.count(target) > 0);
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::alreadySearchingFor");
	}
	m_critical.Unlock();
	return retVal;
}

void CSearchManager::getWords(LPCSTR str, WordList *words)
{
	LPSTR s = (LPSTR)str;
	int len = 0;
	CString word;
	CString wordtemp;
	uint32 i;
	while (strlen(s) > 0)
	{
		len = (int)strcspn(s, _aszInvKadKeywordChars);
		if (len > 2)
		{
			word = s;
			word.Truncate(len);
			word.MakeLower();
			for( i = 0; i < words->size(); i++)
			{
				wordtemp = words->front();
				words->pop_front();
				if(  wordtemp != word )
					words->push_back(wordtemp);
			}
			words->push_back(word);
		}
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
					if (it->second->m_created + SEARCHFILE_LIFETIME < now)
					{
						delete it->second;
						it = m_searches.erase(it);
					}
					else if (it->second->getCount() > SEARCHFILE_TOTAL || it->second->m_created + SEARCHFILE_LIFETIME - SEC(20) < now)
					{
						it->second->prepareToStop();
						it++;
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
					if (it->second->m_created + SEARCHKEYWORD_LIFETIME < now)
					{
						delete it->second;
						it = m_searches.erase(it);
					}
					else if (it->second->getCount() > SEARCHKEYWORD_TOTAL || it->second->m_created + SEARCHKEYWORD_LIFETIME - SEC(20) < now)
					{
						it->second->prepareToStop();
						it++;
					}
					else
					{
						it->second->jumpStart();
						it++;
					}
					break;
				}
				case CSearch::NODE:
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
				case CSearch::NODECOMPLETE:
				{
					if (it->second->m_created + SEARCHNODE_LIFETIME < now)
					{
						CPrefs *prefs = CKademlia::getPrefs();
						if(prefs)
							prefs->setPublish(true);
						else
							CKademlia::debugLine("No preference object found CSearchManager::jumpStart");
						delete it->second;
						it = m_searches.erase(it);
					}
					else if ((it->second->m_created + SEARCHNODECOMP_LIFETIME < now) && (it->second->getCount() > SEARCHNODECOMP_TOTAL))
					{
						CPrefs *prefs = CKademlia::getPrefs();
						if(prefs)
							prefs->setPublish(true);
						else
							CKademlia::debugLine("No preference object found CSearchManager::jumpStart");
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
					if (it->second->m_created + SEARCHSTOREFILE_LIFETIME < now)
					{
						delete it->second;
						it = m_searches.erase(it);
					}
					else if (it->second->getCount() > SEARCHSTOREFILE_TOTAL || it->second->m_created + SEARCHSTOREFILE_LIFETIME - SEC(20) < now)
					{
						it->second->prepareToStop();
						it++;
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
					if (it->second->m_created + SEARCHSTOREKEYWORD_LIFETIME < now)
					{
						delete it->second;
						it = m_searches.erase(it);
					}
					else if (it->second->getCount() > SEARCHSTOREKEYWORD_TOTAL || it->second->m_created + SEARCHSTOREKEYWORD_LIFETIME - SEC(20)< now)
					{
						it->second->prepareToStop();
						it++;
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
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::jumpStart");
	}
	m_critical.Unlock();
}

void CSearchManager::updateStats(void)
{
	uint8 m_totalFile = 0;
	uint8 m_totalStoreSrc = 0;
	uint8 m_totalStoreKey = 0;
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
				{
					m_totalStoreSrc++;
					it++;
					break;
				}
				case CSearch::STOREKEYWORD:
				{
					m_totalStoreKey++;
					it++;
					break;
				}
				default:
					it++;
					break;
			}
		}
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::updateStats");
	}
	m_critical.Unlock();
	CPrefs *prefs = CKademlia::getPrefs();
	if(prefs != NULL)
	{
		prefs->setTotalFile(m_totalFile);
		prefs->setTotalStoreSrc(m_totalStoreSrc);
		prefs->setTotalStoreKey(m_totalStoreKey);
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
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::processPublishResults");
	}
	m_critical.Unlock();

	if (s == NULL)
	{
//		CKademlia::debugLine("Search either never existed or receiving late results (CSearchManager::processPublishResults)");
		return;
	}
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
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::processResponse");
	}
	m_critical.Unlock();

	if (s == NULL)
	{
//		CKademlia::debugLine("Search either never existed or receiving late results (CSearchManager::processResponse)");
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
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CSearchManager::processResult");
	}
	m_critical.Unlock();

	if (s == NULL)
	{
//		CKademlia::debugLine("Search either never existed or receiving late results (CSearchManager::processResult)");
		TagList::const_iterator it;
		for (it = info->begin(); it != info->end(); it++)
			delete *it;
		delete info;
	}
	else
		s->processResult(target, fromIP, fromPort, answer, info);
}
