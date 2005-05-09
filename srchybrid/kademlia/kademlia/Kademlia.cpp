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
#include "Kademlia.h"
#include "Prefs.h"
#include "Error.h"
#include "SearchManager.h"
#include "Indexed.h"
#include "../net/KademliaUDPListener.h"
#include "../routing/RoutingZone.h"
#include "../utils/MiscUtils.h"
#include "../../sharedfilelist.h"
#include "../routing/contact.h"
#include "emule.h"
#include "emuledlg.h"
#include "opcodes.h"
#include "defines.h"
#include "Preferences.h"
#include "Log.h"
#include "MD4.h"
#include "StringConversion.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CKademlia	*CKademlia::instance = NULL;
EventMap	CKademlia::m_events;
time_t		CKademlia::m_nextSearchJumpStart;
time_t		CKademlia::m_nextSelfLookup;
time_t		CKademlia::m_statusUpdate;
time_t		CKademlia::m_bigTimer;
time_t		CKademlia::m_nextFirewallCheck;
time_t		CKademlia::m_nextFindBuddy;
time_t		CKademlia::m_bootstrap;
bool		CKademlia::m_running = false;

void CKademlia::start(void)
{
	if (instance != NULL)
	{
		return;
	}
	start(new CPrefs());
}

void CKademlia::start(CPrefs *prefs)
{
	try
	{
		if( m_running )
		{
			delete prefs;
			return;
		}

		AddDebugLogLine(false, _T("Starting Kademlia"));

		m_nextSearchJumpStart = time(NULL);
		m_nextSelfLookup = time(NULL) + MIN2S(3);
		m_statusUpdate = time(NULL);
		m_bigTimer = time(NULL);
		m_nextFirewallCheck = time(NULL) + (HR2S(1));
		//Frist Firewall check is done on connect, find a buddy after the first 10min of starting
		//the client to try to allow it to settle down..
		m_nextFindBuddy = time(NULL) + (MIN2S(5));
		m_bootstrap = 0;

		srand((UINT)time(NULL));
		instance = new CKademlia();	
		instance->m_prefs = prefs;
		instance->m_udpListener = NULL;
		instance->m_routingZone = NULL;
		instance->m_indexed = new CIndexed();
		instance->m_routingZone = new CRoutingZone();
		instance->m_udpListener = new CKademliaUDPListener();
		m_running = true;
	}
	catch (CException *e)
	{
		TCHAR err[512];
		e->GetErrorMessage(err, 512);
		AddDebugLogLine( false, _T("%s"), err);
		e->Delete();
	}
}

void CKademlia::stop()
{
	if( !m_running )
		return;

	AddDebugLogLine(false, _T("Stopping Kademlia"));
	m_running = false;

	CSearchManager::stopAllSearches();

	delete instance->m_udpListener;
	instance->m_udpListener = NULL;

	delete instance->m_routingZone;
	instance->m_routingZone = NULL;

	delete instance->m_indexed;
	instance->m_indexed = NULL;

	delete instance->m_prefs;
	instance->m_prefs = NULL;

	delete instance;
	instance = NULL;

	m_events.clear();
}

void CKademlia::process()
{
	if( instance == NULL || !m_running)
		return;
	ASSERT(instance != NULL);
	time_t now;
	CRoutingZone *zone;
	EventMap::const_iterator it;
	uint32 maxUsers = 0;
	uint32 tempUsers = 0;
	uint32 lastContact = 0;
	bool updateUserFile = false;
	try
	{
		now = time(NULL);
		ASSERT(instance->m_prefs != NULL);
		lastContact = instance->m_prefs->getLastContact();
		CSearchManager::updateStats();
		if( m_statusUpdate <= now )
		{
			updateUserFile = true;
			m_statusUpdate = MIN2S(1) + now;
		}
		if( m_nextFirewallCheck <= now)
		{
			RecheckFirewalled();
		}
		if (m_nextSelfLookup <= now)
		{
			CUInt128 me;
			instance->m_prefs->getKadID(&me);
			CSearchManager::findNodeComplete(me);
			m_nextSelfLookup = HR2S(4) + now;
		}
		if (m_nextFindBuddy <= now)
		{
			instance->m_prefs->setFindBuddy();
			m_nextFindBuddy = MIN2S(5) + m_nextFirewallCheck;
		}
		for (it = m_events.begin(); it != m_events.end(); it++)
		{
			zone = it->first;
			if( updateUserFile )
			{
				tempUsers = zone->estimateCount();
				if( maxUsers < tempUsers )
					maxUsers = tempUsers;
			}
			if (m_bigTimer <= now)
			{
				try
				{
					if( zone->m_nextBigTimer <= now )
					{
						if(zone->onBigTimer())
						{
							zone->m_nextBigTimer = HR2S(1) + now;
							m_bigTimer = SEC(10) + now;
						}
					} 
					else
					{
						if( lastContact && ( (now - lastContact) > (KADEMLIADISCONNECTDELAY-MIN2S(5))))
						{
							if(zone->onBigTimer())
							{
								zone->m_nextBigTimer = HR2S(1) + now;
								m_bigTimer = SEC(10) + now;
							}
						} 
					}
				}
				catch (...) 
				{
					AddDebugLogLine(false, _T("Exception in Kademlia::Process(1)"));
				}
			}
			if (zone->m_nextSmallTimer <= now)
			{
				try
				{
					zone->onSmallTimer();
				}
				catch (...) 
				{
					AddDebugLogLine(false, _T("Exception in Kademlia::Process(2)"));
				}
				zone->m_nextSmallTimer = MIN2S(1) + now;
			}
				// This is a convenient place to add this, although not related to routing
			if (m_nextSearchJumpStart <= now)
			{
				try
				{
					CSearchManager::jumpStart();
				}
				catch (...) 
				{
					AddDebugLogLine(false, _T("Exception in Kademlia::Process(3)"));
				}
				m_nextSearchJumpStart += SEARCH_JUMPSTART;
			}
		}

		//Update user count only if changed.
		if( updateUserFile )
		{
			if( maxUsers != instance->m_prefs->getKademliaUsers())
			{
				instance->m_prefs->setKademliaUsers(maxUsers);
				instance->m_prefs->setKademliaFiles();
				theApp.emuledlg->ShowUserCount();
			}
		}
	}
	catch (...) 
	{
		AddDebugLogLine(false, _T("Exception in Kademlia::Process(4)"));
	}
}

void CKademlia::addEvent(CRoutingZone *zone)
{
	m_events[zone] = zone;
}

void CKademlia::removeEvent(CRoutingZone *zone)
{
	m_events.erase(zone);
}

bool CKademlia::isConnected(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->hasHadContact();
	return false;
}

bool CKademlia::isFirewalled(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getFirewalled(); 
	return true;
}

uint32 CKademlia::getKademliaUsers(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getKademliaUsers();
	return 0;
}

uint32 CKademlia::getKademliaFiles(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getKademliaFiles();
	return 0;
}

uint32 CKademlia::getTotalStoreKey(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getTotalStoreKey();
	return 0;
}

uint32 CKademlia::getTotalStoreSrc(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getTotalStoreSrc();
	return 0;
}

uint32 CKademlia::getTotalStoreNotes(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getTotalStoreNotes();
	return 0;
}

uint32 CKademlia::getTotalFile(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getTotalFile();
	return 0;
}

uint32 CKademlia::getIPAddress(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getIPAddress();
	return 0;
}

void CKademlia::processPacket(const byte *data, uint32 lenData, uint32 ip, uint16 port)
{
	if( instance && instance->m_udpListener )
		instance->m_udpListener->processPacket( data, lenData, ip, port);
}

bool CKademlia::getPublish(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getPublish();
	return 0;
}

void CKademlia::bootstrap(LPCTSTR host, uint16 port)
{
	if( instance && instance->m_udpListener && !isConnected() && time(NULL) - m_bootstrap > MIN2S(1) )
		instance->m_udpListener->bootstrap( host, port);
}

void CKademlia::bootstrap(uint32 ip, uint16 port)
{
	if( instance && instance->m_udpListener && !isConnected() && time(NULL) - m_bootstrap > MIN2S(1) )
		instance->m_udpListener->bootstrap( ip, port);
}

void CKademlia::RecheckFirewalled()
{
	if( instance && instance->getPrefs() )
	{
		instance->m_prefs->setFindBuddy(false);
		instance->m_prefs->setRecheckIP();
		m_nextFindBuddy = MIN2S(5) + m_nextFirewallCheck;
		m_nextFirewallCheck = HR2S(1) + time(NULL);
	}
}

CPrefs *CKademlia::getPrefs(void)
{
	if (instance == NULL || instance->m_prefs == NULL)
	{
		ASSERT(0);
		return NULL;
	}
	return instance->m_prefs;
}

CKademliaUDPListener *CKademlia::getUDPListener(void)
{
	if (instance == NULL || instance->m_udpListener == NULL)
	{
		ASSERT(0);
		return NULL;
	}
	return instance->m_udpListener;
}

CRoutingZone *CKademlia::getRoutingZone(void)
{
	if (instance == NULL || instance->m_routingZone == NULL)
	{
		ASSERT(0);
		return NULL;
	}
	return instance->m_routingZone;
}

CIndexed *CKademlia::getIndexed(void)
{
	if ( instance == NULL || instance->m_indexed == NULL)
	{
		ASSERT(0);
		return NULL;
	}
	return instance->m_indexed;
}

void KadGetKeywordHash(const CStringA& rstrKeywordA, Kademlia::CUInt128* pKadID)
{
	CMD4 md4;
	md4.Add((byte*)(LPCSTR)rstrKeywordA, rstrKeywordA.GetLength());
	md4.Finish();
	pKadID->setValueBE(md4.GetHash());
}

CStringA KadGetKeywordBytes(const CStringW& rstrKeywordW)
{
	return CStringA(wc2utf8(rstrKeywordW));
}

void KadGetKeywordHash(const CStringW& rstrKeywordW, Kademlia::CUInt128* pKadID)
{
	KadGetKeywordHash(KadGetKeywordBytes(rstrKeywordW), pKadID);
}
