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
#include "SocketListener.h"
#include "../kademlia/Kademlia.h"
#include "NetException.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

bool CSocketListener::m_bSocketInit = false;

CSocketListener::CSocketListener()
{
	// No harm calling this more than once, but don't waste time
	if (!m_bSocketInit)
	{
		WSADATA wsaData;
		if (WSAStartup(0x0101, &wsaData) != 0)
			throw new CNetException(ERR_WINSOCK);
		if (wsaData.wVersion != 0x0101)
		{
			WSACleanup();
			throw new CNetException(ERR_WINSOCK);
		}
		m_bSocketInit = true;
	}

	m_port = 0;
	m_type = 0;
	m_hSocket = NULL;
	m_bRunning = false;
	//m_hThread = NULL;
	m_pThread = NULL;
	m_hStopEvent = NULL;
}

CSocketListener::~CSocketListener()
{
	// do not call the 'stop' function in the dtor!
	ASSERT( !m_bRunning );
	stop(false);
}

int CSocketListener::start(uint16 nSocketPort, int nSocketType)
{
	int retVal = ERR_SUCCESS;
	try
	{
		m_bRunning = false;
		m_port = nSocketPort;
		m_type = nSocketType;
		m_hStopEvent = NULL;

		// Wrap info required to be passed to thread
		SocketListenerThreadParam tp;
		tp.listener = this;
		tp.port = nSocketPort;
		tp.type = nSocketType;
		tp.hStopEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		tp.hReturnEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		tp.nReturnValue = ERR_SUCCESS;
		if ((tp.hStopEvent == NULL) || (tp.hReturnEvent == NULL))
		{
			CKademlia::logMsg("CSocketListener::start() - Could not create event handles");
			throw new CNetException(ERR_CREATE_HANDLE_FAILED);
		}
		m_hStopEvent = tp.hStopEvent;

		// Create a new thread for the listener
		ASSERT( m_pThread == NULL );
		m_pThread = new CWinThread(listening, (LPVOID)&tp);
		m_pThread->m_bAutoDelete = FALSE;
		if (!m_pThread->CreateThread())
		{
			CKademlia::logMsg("CSocketListener::start() - Could not create thread");
			delete m_pThread;
			m_pThread = NULL;
			throw new CNetException(ERR_CREATE_THREAD_FAILED);
		}

		// Wait for the thread to signal success/failure
		WaitForSingleObject(tp.hReturnEvent, INFINITE);
		CloseHandle(tp.hReturnEvent);

		if (tp.nReturnValue == ERR_SUCCESS)
			m_bRunning = true;
		else
			throw new CNetException(tp.nReturnValue);
	}
	catch (CNetException *ne)
	{
		retVal = ne->m_cause;
		ne->Delete();
	}
	catch (...)
	{
		retVal = ERR_UNKNOWN;
	}

	if (retVal == ERR_SUCCESS)
		CKademlia::debugMsg("New listener thread created (%s port %ld)", (nSocketType == SOCK_STREAM ? "TCP" : "UDP"), nSocketPort);
	else
		CKademlia::logMsg("Failed to create new listener thread (%s port %ld) [Error 0x%04X%ld]", (nSocketType == SOCK_STREAM ? "TCP" : "UDP"), nSocketPort, retVal);

	return retVal;
}

void CSocketListener::stop(bool bAppShutdown)
{
	if (!m_bRunning)
		return;

	if (m_hStopEvent != NULL)
		SetEvent(m_hStopEvent);

	// Wait for the thread to finish
	if (m_pThread != NULL) 
	{
		if (m_pThread->m_hThread != NULL)
		{
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
						TRACE("%s: Error in MsgWaitForMultipleObjects: %08x\n", __FUNCTION__, GetLastError());
						ASSERT(0);
					}
					else if (dwEvent == WAIT_OBJECT_0 + iNumEvents)
					{
						CWinThread *pThread = AfxGetThread();
						MSG* pMsg = AfxGetCurrentMessage();
						while (::PeekMessage(pMsg, NULL, NULL, NULL, PM_NOREMOVE))
						{
							TRACE("%s: Message %08x arrived while waiting on thread shutdown\n", __FUNCTION__, pMsg->message);
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
		}
		delete m_pThread;
		m_pThread = NULL;
	}
	if (m_hStopEvent != NULL)
	{
		CloseHandle(m_hStopEvent);		
		m_hStopEvent = NULL;
	}

	m_bRunning = false;
}

UINT AFX_CDECL CSocketListener::listening(LPVOID lpParam)
{
	return ((SocketListenerThreadParam *)lpParam)->listener->listeningImpl(lpParam);
}

bool CSocketListener::create(uint16 nSocketPort, int nSocketType)
{
	m_hSocket = socket(AF_INET, nSocketType, 0);
	if (m_hSocket == INVALID_SOCKET)
	{
		CKademlia::logMsg("Failed to create new %s socket on port %ld - Error %ld", (nSocketType == SOCK_DGRAM ? "UDP" : "TCP"), nSocketPort, WSAGetLastError());
		return false;
	}
	
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(nSocketPort);
	if (bind(m_hSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		CKademlia::logMsg("Failed to bind new %s socket on port %ld - Error %ld", (nSocketType == SOCK_DGRAM ? "UDP" : "TCP"), nSocketPort, WSAGetLastError());
		return false;
	}

	return true;
}

uint32 CSocketListener::nameToIP(LPCSTR host)
{
	uint32 retVal = 0;
	try
	{
		if (isalpha(host[0])) 
		{
			hostent *hp = gethostbyname(host);
			if (hp == NULL) 
				return false;
			memcpy (&retVal, hp->h_addr, sizeof(retVal));
		}
		else
			retVal = inet_addr(host);
	} catch (...) {}
	return ntohl(retVal);
}