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
#include "../../opcodes.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CWinThread* CTimer::m_pThread = NULL;
DWORD		CTimer::m_dwThreadID = 0;
HANDLE		CTimer::m_hStopEvent = NULL;
EventMap	CTimer::m_events;
time_t		CTimer::m_nextSearchJumpStart;
time_t		CTimer::m_nextSelfLookup;
time_t		CTimer::m_statusUpdate;
time_t		CTimer::m_bigTimer;
time_t		CTimer::m_nextFirewallCheck;

void CTimer::start(void)
{
	if (m_pThread != NULL && m_pThread->m_hThread != NULL)
		return;

	m_nextSearchJumpStart = time(NULL);
	m_nextSelfLookup = time(NULL) + MIN2S(10);
	m_statusUpdate = time(NULL);
	m_bigTimer = time(NULL);
	m_nextFirewallCheck = time(NULL) + (HR2S(1));

	ASSERT( m_pThread == NULL );
	m_pThread = new CWinThread(timer, NULL);
	m_pThread->m_bAutoDelete = FALSE;
	if (!m_pThread->CreateThread())
	{
		delete m_pThread;
		m_pThread = NULL;
		CKademlia::reportError(ERR_CREATE_THREAD_FAILED, "Failed to create Kademlia timer thread.");
	}
	else
	{
		m_dwThreadID = m_pThread->m_nThreadID; // stored thread ID in sync save member var
		m_hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	}
}

void CTimer::stop(bool bAppShutdown)
{
	if (m_pThread != NULL)
	{
		if (m_pThread->m_hThread != NULL)
		{
			m_dwThreadID = 0; // don't all other threads to post messages to the thread during thread termination
			SetEvent(m_hStopEvent);

			if (bAppShutdown)
			{
				// NOTE: This code is to be invoked from within the main thread *only*!
				bool bQuit = false;
				while (!bQuit)
				{
					const int iNumEvents = 1;
					DWORD dwEvent = MsgWaitForMultipleObjects(iNumEvents, &m_pThread->m_hThread, FALSE, INFINITE, QS_ALLINPUT);
					if (dwEvent == -1)
					{
						CKademlia::debugMsg("%s: Error in MsgWaitForMultipleObjects: %08x (CTimer::stop)\n", __FUNCTION__, GetLastError());
						ASSERT(0);
					}
					else if (dwEvent == WAIT_OBJECT_0 + iNumEvents)
					{
						CWinThread *pThread = AfxGetThread();
						MSG* pMsg = AfxGetCurrentMessage();
						while (::PeekMessage(pMsg, NULL, NULL, NULL, PM_NOREMOVE))
						{
							CKademlia::debugMsg("%s: Message %08x arrived while waiting on thread shutdown (CTimer::stop)\n", __FUNCTION__, pMsg->message);
							// pump message, but quit on WM_QUIT
							if (!pThread->PumpMessage()) {
								AfxPostQuitMessage(0);
								bQuit = true;
								break;
							}
						}
					}
					else if (dwEvent == WAIT_OBJECT_0 + 0)
					{
						// thread has finished
						break;
					}
					else
					{
						ASSERT(0);
					}
				}
			}
			else
			{
				WaitForSingleObject(m_pThread->m_hThread, INFINITE);
			}

			CloseHandle(m_hStopEvent);
			m_hStopEvent = NULL;
			m_events.clear();
		}
		delete m_pThread;
		m_pThread = NULL;
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

UINT AFX_CDECL CTimer::timer(LPVOID lpParam)
{
	Kademlia::SetThreadName("Kademlia Routing Timer");

	time_t now;
	CRoutingZone *zone;
	EventMap::const_iterator it;
	uint32 maxUsers = 0;
	for (;;)
	{
		try
		{
			now = time(NULL);
			CPrefs *prefs = CKademlia::getPrefs();
			ASSERT(prefs != NULL); 
			if( m_statusUpdate <= now )
			{
				Kademlia::CKademlia::reportUpdateStatus(prefs->getStatus());
				m_statusUpdate = SEC(1) + now;
			}
			prefs->setKademliaUsers(maxUsers);
			maxUsers = 0;
			if( m_nextFirewallCheck <= now)
			{
				prefs->setRecheckIP();
				m_nextFirewallCheck = HR2S(1) + now;
			}
			if (m_nextSelfLookup <= now)
			{
				CUInt128 me;
				prefs->getClientID(&me);
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
						CKademlia::debugLine("Exception in CTimer::timer(1)");
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
						CKademlia::debugLine("Exception in CTimer::timer(2)");
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
						CKademlia::debugLine("Exception in CTimer::timer(3)");
					}
					m_nextSearchJumpStart += SEARCH_JUMPSTART;
				}
			}

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
						CKademlia::debugMsg("*** CTimer::timer; unknown message=0x%04x  wParam=%08x  lParam=%08x\n", msg.message, msg.wParam, msg.lParam);
					}
				}

				// TODO: If we need accurate time managment here, we have to update the 'dwTimeout' for the next
				// MsgWaitForMultipleObjects call, according the consumed time.
			}

		} 
		catch (...) 
		{
			CKademlia::debugLine("Exception in CTimer::timer(4)");
		}
	}
	return 0;
}

DWORD CTimer::getThreadID()
{
	// this function is allowed to be called from any thread -> do not use 'm_pThread->m_nThreadID'
	return m_dwThreadID;
}
