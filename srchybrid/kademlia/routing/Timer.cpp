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
#include "Timer.h"
#include "../kademlia/Kademlia.h"
#include "../kademlia/Error.h"
#include "../kademlia/SearchManager.h"
#include "../kademlia/Defines.h"
#include "../kademlia/Prefs.h"
#include "../utils/ThreadName.h"
#include "RoutingZone.h"
#include "../kademlia/Search.h"
#include "../net/KademliaUDPListener.h"
#include "../../PartFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

HANDLE		CTimer::m_hThread = NULL;
HANDLE		CTimer::m_hStopEvent = NULL;
EventMap	CTimer::m_events;
time_t		CTimer::m_nextSearchJumpStart;
time_t		CTimer::m_nextSelfLookup;
DWORD		CTimer::m_dwThreadID = 0;
time_t		CTimer::m_statusUpdate;
time_t		CTimer::m_bigTimer;
time_t		CTimer::m_nextFirewallCheck;

void CTimer::start(void)
{
	if (m_hThread != NULL)
		return;

	m_nextSearchJumpStart = time(NULL);
	m_nextSelfLookup = time(NULL) + TEN_MINS/2;
	m_statusUpdate = time(NULL);
	m_bigTimer = time(NULL);
	m_nextFirewallCheck = time(NULL) + (HOUR);

	// SetTimer is designed for windows applications, not console applications.
	m_hThread = CreateThread(	NULL,				// no security attributes 
								0,					// use default stack size  
								timer,				// thread function 
								NULL,				// argument to thread function 
								0,					// use default creation flags 
								&m_dwThreadID);		// returns the thread identifier 
	if (m_hThread == NULL)
		CKademlia::reportError(ERR_CREATE_THREAD_FAILED, "Failed to create Kademlia timer thread.");
	else
		m_hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}

void CTimer::stop(void)
{
	if (m_hThread != NULL)
	{
		m_dwThreadID = 0;
		SetEvent(m_hStopEvent);
		WaitForSingleObject(m_hThread, INFINITE);
		CloseHandle(m_hThread);
		CloseHandle(m_hStopEvent);
		m_hThread = NULL;
		m_hStopEvent = NULL;
		m_events.clear();
	}
}

void CTimer::addEvent(CRoutingZone *zone)
{
	m_events[zone] = zone;
}

void CTimer::removeEvent(CRoutingZone *zone)
{
	m_events.erase(zone);
}

DWORD WINAPI CTimer::timer(LPVOID lpParam)
{
	Kademlia::SetThreadName("Kademlia Routing Timer");

	time_t now;
	CRoutingZone *zone;
	EventMap::const_iterator it;
	uint32 maxUsers = 0;
	while (true)
	{
		try
		{
			now = time(NULL);
			CPrefs *prefs = CKademlia::getPrefs();
			ASSERT(prefs != NULL); 
			if( m_statusUpdate <= now )
			{
				Kademlia::CKademlia::reportUpdateStatus(prefs->getStatus());
				m_statusUpdate = ONE_SEC + now;
			}
			prefs->setKademliaUsers(maxUsers);
			maxUsers = 0;
			if( m_nextFirewallCheck <= now)
			{
				prefs->setRecheckIP();
				m_nextFirewallCheck = HOUR + now;
			}
//			if ( prefs->getLastContact() == false && (m_nextSelfLookup > now + ONE_MIN ))
//				m_nextSelfLookup = (ONE_SEC*10) + now;
			if (m_nextSelfLookup <= now)
			{
				CUInt128 me;
				prefs->getClientID(&me);
				CSearchManager::findNodeComplete(me);
				m_nextSelfLookup = (4 * HOUR) + now;
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
							zone->m_nextBigTimer = HOUR + now;
							m_bigTimer = ONE_SEC*10 + now;
						}
					} catch (...) {}
				}
				if (zone->m_nextSmallTimer <= now)
				{
					try
					{
						zone->onSmallTimer();
					} catch (...) {}
					zone->m_nextSmallTimer = ONE_MIN + now;
				}

				// This is a convenient place to add this, although not related to routing
				if (m_nextSearchJumpStart <= now)
				{
					try
					{
						CSearchManager::jumpStart();
					}catch (...) {}
					m_nextSearchJumpStart += SEARCH_JUMPSTART;
				}
			}

//			if (WaitForSingleObject(m_hStopEvent, 1000) == WAIT_OBJECT_0)
//				break;
			DWORD dwEvent = MsgWaitForMultipleObjects(1, &m_hStopEvent, FALSE, 1000, QS_SENDMESSAGE | QS_POSTMESSAGE | QS_TIMER);
			if (dwEvent == WAIT_OBJECT_0)
				break;
			else if (dwEvent == -1){
				ASSERT(0);
				//break;
			}
			else if (dwEvent != WAIT_TIMEOUT)
			{
				MSG msg;
				while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				{
					if (msg.message == WM_KADEMLIA_STARTSEARCH)
					{
						CSearch* pSearch = (CSearch*)msg.lParam;
						CSearchManager::startSearch(pSearch);
					}
					else if (msg.message == WM_KADEMLIA_STOPSEARCH)
					{
						CSearchManager::stopSearch(msg.lParam);
					}
					else{
						TRACE("*** CTimer::timer; unknown message=0x%04x  wParam=%08x  lParam=%08x\n", msg.message, msg.wParam, msg.lParam);
					}
				}

				// TODO: If we need accurate time managment here, we have to update the 'dwTimeout' for the next
				// MsgWaitForMultipleObjects call, according the consumed time.
			}

		} catch (...) {}
	}
	return 0;
}

DWORD CTimer::getThreadID()
{
	return m_dwThreadID;
}
