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

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
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
bool		CKademlia::m_running = false;

void CKademlia::start(void)
{
	if (instance != NULL)
	{
		return;
	}
	start(new CPrefs());
//	Kademlia::CKademlia::getRoutingZone()->selfTest();

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

		debugMsg("Starting Kademlia");

		m_nextSearchJumpStart = time(NULL);
		m_nextSelfLookup = time(NULL) + MIN2S(5);
		m_statusUpdate = time(NULL);
		m_bigTimer = time(NULL);
		m_nextFirewallCheck = time(NULL) + (HR2S(1));

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
		char err[512];
		e->GetErrorMessage(err, 512);
		reportError(ERR_UNKNOWN, err);
		e->Delete();
	}
}

void CKademlia::stop()
{
	if( !m_running )
		return;

	debugMsg("Stopping Kademlia");
	m_running = false;

	CSearchManager::stopAllSearches();
	try
	{
		delete instance->m_udpListener;
		instance->m_udpListener = NULL;
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::stop(1)");
		ASSERT(0);
	}

	try
	{
		delete instance->m_routingZone;
		instance->m_routingZone = NULL;
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::stop(2)");
		ASSERT(0);
	}

	try
	{
		delete instance->m_prefs;
		instance->m_prefs = NULL;
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::stop(3)");
		ASSERT(0);
	}

	try
	{
		delete instance->m_indexed;
		instance->m_indexed = NULL;
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::stop(4)");
		ASSERT(0);
	}

	try
	{
		delete instance;
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::stop(5)");
		ASSERT(0);
	}
	m_events.clear();
	instance = NULL;
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
	try
	{
		now = time(NULL);
		ASSERT(instance->m_prefs != NULL); 
		if( m_statusUpdate <= now )
		{
			CSearchManager::updateStats();
			m_statusUpdate = SEC(1) + now;
		}
		if( m_nextFirewallCheck <= now)
		{
			instance->m_prefs->setRecheckIP();
			m_nextFirewallCheck = HR2S(1) + now;
		}
		if (m_nextSelfLookup <= now)
		{
			CUInt128 me;
			instance->m_prefs->getClientID(&me);
			CSearchManager::findNodeComplete(me);
			m_nextSelfLookup = HR2S(4) + now;
		}
		for (it = m_events.begin(); it != m_events.end(); it++)
		{
			zone = it->first;
			if( zone->estimateCount() > maxUsers)
				maxUsers = zone->estimateCount();
			if (zone->m_nextBigTimer <= now && m_bigTimer <= now)
			{
				try
				{
					if(zone->onBigTimer())
					{
						zone->m_nextBigTimer = HR2S(1) + now;
						m_bigTimer = SEC(10) + now;
					}
				} 
				catch (...) 
				{
					CKademlia::debugLine("Exception in Kademlia::Process(1)");
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
					CKademlia::debugLine("Exception in Kademlia::Process(2)");
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
					CKademlia::debugLine("Exception in Kademlia::Process(3)");
				}
				m_nextSearchJumpStart += SEARCH_JUMPSTART;
			}
		}

		//Update user count only if changed.
		if( maxUsers != instance->m_prefs->getKademliaUsers())
		{
			instance->m_prefs->setKademliaUsers(maxUsers);
			theApp.emuledlg->ShowUserCount();
		}
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in Kademlia::Process(4)");
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
		return instance->m_prefs->getLastContact();
//	else
//		CKademlia::debugLine("Exception in CKademlia::isConnected");
	return false;
}

bool CKademlia::isFirewalled(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getFirewalled(); 
//	else
//		CKademlia::debugLine("Exception in CKademlia::isFirewalled");
	return true;
}

uint32 CKademlia::getKademliaUsers(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getKademliaUsers();
//	else
//		CKademlia::debugLine("Exception in Kademlia::getKademliaUsers");
	return 0;
}

uint32 CKademlia::getTotalStoreKey(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getTotalStoreKey();
//	else
//		CKademlia::debugLine("Exception in CKademlia::getTotalStoreKey");
	return 0;
}

uint32 CKademlia::getTotalStoreSrc(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getTotalStoreSrc();
//	else
//		CKademlia::debugLine("Exception in CKademlia::getTotalStoreSrc");
	return 0;
}

uint32 CKademlia::getTotalFile(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getTotalFile();
//	else
//		CKademlia::debugLine("Exception in CKademlia::getTotalFile");
	return 0;
}

uint32 CKademlia::getIPAddress(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getIPAddress();
//	else
//		CKademlia::debugLine("Exception in CKademlia::getIPAddress");
	return 0;
}

void CKademlia::processPacket(const byte *data, uint32 lenData, uint32 ip, uint16 port)
{
	if( instance && instance->m_udpListener )
		instance->m_udpListener->processPacket( data, lenData, ip, port);
//	else
//		CKademlia::debugLine("Exception in CKademlia::processPacket");
}

bool CKademlia::getPublish(void)
{
	if( instance && instance->m_prefs )
		return instance->m_prefs->getPublish();
//	else
//		CKademlia::debugLine("Exception in CKademlia::getKeywordPublish");
	return 0;
}

void CKademlia::bootstrap(LPCSTR host, uint16 port)
{
	if( instance && instance->m_udpListener )
		instance->m_udpListener->bootstrap( host, port);
//	else
//		CKademlia::debugLine("Exception in CKademlia::bootstrap");
}

void CKademlia::bootstrap(uint32 ip, uint16 port)
{
	if( instance && instance->m_udpListener )
		instance->m_udpListener->bootstrap( ip, port);
//	else
//		CKademlia::debugLine("Exception in CKademlia::bootstrap");
}

void CKademlia::logMsg(LPCSTR lpMsg, ...)
{
	try
	{
		CString msg;
		va_list args;
		va_start(args, lpMsg);
		msg.FormatV(lpMsg, args);
		va_end(args);
		theApp.AddLogLine(false, msg);
	}
	catch(...)
	{
		TRACE("Exception in CKademlia::logMsg");
		return;
	}
}

void CKademlia::logLine(LPCSTR lpLine)
{
	try
	{
		theApp.AddLogLine(false, lpLine);
	}
	catch(...)
	{
		TRACE("Exception in CKademlia::logLine");
		return;
	}
}

void CKademlia::debugMsg(LPCSTR lpMsg, ...)
{
	try
	{
		if (thePrefs.GetVerbose())
		{
			CString msg;
			va_list args;
			va_start(args, lpMsg);
			msg.FormatV(lpMsg, args);
			va_end(args);
			theApp.AddDebugLogLine( false, msg );
		}
	}
	catch(...)
	{
		TRACE("Exception in CKademlia::debugMsg");
		return;
	}
}

void CKademlia::debugLine(LPCSTR lpLine)
{
	try
	{
		if (thePrefs.GetVerbose())
			theApp.AddDebugLogLine(false, lpLine);
	}
	catch(...)
	{
		TRACE("Exception in CKademlia::debugLine");
		return;
	}
}
void CKademlia::reportError(int errorCode, LPCSTR errorDescription, ...)
{
	try
	{
		if (thePrefs.GetVerbose())
		{
			CString msg;
			va_list args;
			va_start(args, errorDescription);
			msg.FormatV(errorDescription, args);
			va_end(args);
			CKademliaError error(errorCode, msg.GetBuffer(0));
			theApp.AddDebugLogLine(false, "(%i) : %u", errorCode, msg);
		}
	}
	catch(...)
	{
		TRACE("Exception in CKademlia::reportError");
		return;
	}
}

CPrefs *CKademlia::getPrefs(void)
{
	try
	{
		if (instance == NULL || instance->m_prefs == NULL)
		{
			return NULL;
		}
	}
	catch(...)
	{ 
		CKademlia::debugLine("Exception in CKademlia::getPrefs");
		return NULL; 
	}
	return instance->m_prefs;
}

CKademliaUDPListener *CKademlia::getUDPListener(void)
{
	try
	{
		if (instance == NULL || instance->m_udpListener == NULL)
		{
			return NULL;
		}
	}
	catch(...)
	{ 
		CKademlia::debugLine("Exception in CKademlia::getUDPListener");
		return NULL; 
	}
	return instance->m_udpListener;
}

CRoutingZone *CKademlia::getRoutingZone(void)
{
	try
	{
		if (instance == NULL || instance->m_routingZone == NULL)
		{
			return NULL;
		}
	}
	catch(...)
	{ 
		CKademlia::debugLine("Exception in CKademlia::reportRoutingZone");
		return NULL; 
	}
	return instance->m_routingZone;
}

CIndexed *CKademlia::getIndexed(void)
{
	try
	{
		if ( instance == NULL || instance->m_indexed == NULL)
		{
			return NULL;
		}
	}
	catch(...)
	{ 
		CKademlia::debugLine("Exception in CKademlia::getIndexed");
		return NULL; 
	}
	return instance->m_indexed;
}