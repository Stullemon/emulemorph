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
#include "UDPSocketListener.h"
#include <mswsock.h>
#include "../kademlia/Kademlia.h"
#include "../kademlia/Error.h"
#include "../kademlia/SearchManager.h"
#include "../utils/ThreadName.h"
#include "../utils/MiscUtils.h"
#include "NetException.h"
#include "OtherFunctions.h"
#include "../../OpCodes.h"
#include <zlib/zlib.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CUDPSocketListener::CUDPSocketListener()
{
	timer = ::GetTickCount();
}

CUDPSocketListener::~CUDPSocketListener()
{
	WaitingUDPData *w;
	UDPQueue::const_iterator it;
	m_critical.Lock();
	try
	{
		for (it = m_sendQueue.begin(); it != m_sendQueue.end(); it++)
		{
			w = *it;
			delete w->addr;
			delete [] w->data;
			delete w;
		}
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in ~CUDPSocketListener");
	}
	m_sendQueue.clear();
	m_critical.Unlock();
}

int CUDPSocketListener::start(uint16 port)
{
	return CSocketListener::start(port, SOCK_DGRAM);
}

DWORD CUDPSocketListener::listeningImpl(LPVOID lpParam)
{
	try
	{
		// Get the parameters passed
		SocketListenerThreadParam *tp = (SocketListenerThreadParam *)lpParam;
		uint16			port			= tp->port;
		int				type			= tp->type;
		HANDLE			hStopEvent		= tp->hStopEvent;

		// Give this thread a meaningful name to help identify it
		SetThreadName("UDP Listener: Port %ld", port);

		// Try to create the socket
		if (!create(port, type))
		{
			tp->nReturnValue = ERR_CREATE_SOCKET_FAILED;
			SetEvent(tp->hReturnEvent);
			return 1;
		}

		// Set to non-blocking
		u_long argp = 1;
		ioctlsocket(m_hSocket, FIONBIO, &argp);

		// Try to avoid the Win2000/XP problem where recvfrom reports 
		// WSAECONNRESET after sendto gets "ICMP port unreachable" 
		// when sent to port that wasn't listening.
		// See MSDN - Q263823
		tryConnResetFix();

		// Notify the createListenerThread() method of result
		tp->nReturnValue = ERR_SUCCESS;
		SetEvent(tp->hReturnEvent);

		// Loop waiting for packets to arrive
		fd_set readfds;
		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;
		byte *buffer;
		uint32 lenBuf;
		uint32 lenPacket;
		sockaddr_in senderAddress;
		int lenAddress = sizeof(senderAddress);
		memset(&senderAddress, 0, lenAddress);
		int err;
		uint32 delay = 100;
		uint32 sendQueueSize = 0;
		for (;;)
		{
			try
			{
				m_critical.Lock(); // only lock the 'm_sendQueue'
				sendQueueSize = (uint32)m_sendQueue.size();
				// Send all data in the queue
				if (sendQueueSize && (GetTickCount() - timer > delay))
				{
					WaitingUDPData *w = m_sendQueue.front();

//					CKademlia::debugMsg("Sending UDP Packet to %s port %ld", inet_ntoa(w->addr->sin_addr), ntohs(w->addr->sin_port));
//					CMiscUtils::debugHexDump(w->data, w->lenData);

					try
					{
						if(sendQueueSize > 9)
						{
							delay = 1000/sendQueueSize;
						}
						timer += delay;
						if (sendto(m_hSocket, (char*)w->data, w->lenData, 0, (sockaddr*)w->addr, sizeof(*w->addr)) == SOCKET_ERROR){
							CKademlia::logMsg("Failed to send UDP packet to %s port %ld. %s", inet_ntoa(w->addr->sin_addr), ntohs(w->addr->sin_port), GetErrorMessage(WSAGetLastError(), 1));
						}
					} 
					catch (...) 
					{
						CKademlia::debugLine("Exception in CUDPSocketListener::listeningImpl(1)");
					}

					delete w->addr;
					delete [] w->data;
					delete w;
					m_sendQueue.pop_front();
				}
				m_critical.Unlock();

				// Test for any data waiting to be read
				readfds.fd_count = 1;
				readfds.fd_array[0] = m_hSocket;
				if (select(0, &readfds, NULL, NULL, &timeout) == 1)
				{
					lenBuf = 1024;
					do
					{	
						err = 0;
						lenBuf *= 2;
						buffer = new byte[lenBuf];
						lenPacket = recvfrom(m_hSocket, (char*)buffer, lenBuf, MSG_PEEK, (sockaddr*)&senderAddress, &lenAddress);
						if (lenPacket == SOCKET_ERROR)
						{
							err = WSAGetLastError();
							if (err == WSAECONNRESET)
							{
								// No hope, must close and re-open the socket
								closesocket(m_hSocket);
								create(port, type);
							}
							delete [] buffer;
						}
					} while (err == WSAEMSGSIZE);

					// Process the packet
					if (lenPacket != SOCKET_ERROR)
					{
						lenPacket = recvfrom(m_hSocket, (char*)buffer, lenBuf, 0, (sockaddr*)&senderAddress, &lenAddress);
						if ((int)lenPacket > 1)
						{
							CKademlia::reportOverheadRecv(lenPacket);

							// If we don't know the opcode, we could save some CPU cycles with ignoring that packet
							if (buffer[0] == OP_KADEMLIAPACKEDPROT /*&& IsKnownKadOpcode(buffer[1])*/)
							{
								uint32 nNewSize = lenPacket*10+300;
								byte* unpack = new byte[nNewSize];
								uLongf unpackedsize = nNewSize-2;
								uint16 result = uncompress(unpack+2,&unpackedsize,(BYTE*)buffer+2,lenPacket-2);
								if (result == Z_OK)
								{
									unpack[0] = OP_KADEMLIAHEADER;
									unpack[1] = buffer[1];
									processPacket(unpack, unpackedsize+2, &senderAddress);
								}
								else{
									CKademlia::debugLine("Error: Failed to uncompress Kademlia packet");
								}
								delete[] unpack;
							}
							else if ( buffer[0] == OP_KADEMLIAHEADER )
							{
								processPacket(buffer, lenPacket, &senderAddress);
							}
						}
						delete[] buffer;
					}
				}
			} 
			catch (...) 
			{
				CKademlia::debugLine("Exception in CUDPSocketListener::listeningImpl(2)");
			}

			// Is the thread supposed to finish
//			if (WaitForSingleObject(hStopEvent, 100) == WAIT_OBJECT_0)
//			{
//				break;
//			}
			DWORD dwEvent = MsgWaitForMultipleObjects(1, &hStopEvent, FALSE, 100, QS_SENDMESSAGE | QS_POSTMESSAGE | QS_TIMER);
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
					OnMessage(msg.message, msg.wParam, msg.lParam);
				}

				// TODO: If we need accurate time managment here, we have to update the 'dwTimeout' for the next
				// MsgWaitForMultipleObjects call, according the consumed time.
			}
		}

		closesocket(m_hSocket);
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CUDPSocketListener::listeningImpl(3)");
	}
	return 0;
}

void CUDPSocketListener::tryConnResetFix(void)
{
	try
	{
		DWORD bNewBehavior = FALSE;
		int result = ioctlsocket(m_hSocket, SIO_UDP_CONNRESET, &bNewBehavior);
		if (result == SOCKET_ERROR)
		{
			CKademlia::debugMsg("Cound not resolve WSAECONNRESET problem (ioctlsocket returned %ld) (CUDPSocketListener::tryConnResetFix).", result);
		}
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CUDPSocketListener::tryConnResetFix");
	}
}

void CUDPSocketListener::sendPacket(byte *data, uint32 lenData, const uint32 destinationHost, const uint16 destinationPort)
{
	if (!isRunning())
	{
		return;
	}
	uLongf newsize = lenData+300;
	BYTE* output = new BYTE[newsize];
	if( lenData > 200 )
	{
		uint32 lenRawData = lenData-2;
		uint16 result = compress2(output+2,&newsize,(BYTE*)data+2,lenRawData,Z_BEST_COMPRESSION);
		if (result != Z_OK || lenRawData <= newsize)
		{
			if (result != Z_OK)
			{
				CKademlia::debugLine("Error: Failed to compress Kademlia packet");
			}
			memcpy(output, data, lenData);
		}
		else
		{
			output[0] = OP_KADEMLIAPACKEDPROT;	// change protocol code to indicate compression
			output[1] = data[1];				// leave the packet opcode unchanged
			lenData = newsize+2;
		}
	}
	else
	{
		memcpy(output, data, lenData);
	}
	CKademlia::reportOverheadSend(lenData);
	try
	{
		WaitingUDPData *w = new WaitingUDPData;
		w->addr = new sockaddr_in;
		memset(w->addr, 0, sizeof(w->addr));
		w->addr->sin_family = AF_INET;
		w->addr->sin_addr.s_addr = htonl(destinationHost);
		w->addr->sin_port = htons(destinationPort);
		w->data = output;
		w->lenData = lenData;
		m_critical.Lock();
		try
		{
			m_sendQueue.push_back(w);
		} 
		catch (...) 
		{
			CKademlia::debugLine("Exception in CUDPSocketListener::sendPacket(1)");
		}
		m_critical.Unlock();
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CUDPSocketListener::sendPacket(2)");
	}
}

void CUDPSocketListener::sendPacket( byte *data, uint32 lenData, LPCSTR destinationHost, const uint16 destinationPort)
{
	return sendPacket(data, lenData, nameToIP(destinationHost), destinationPort);
}