//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "stdafx.h"
#include "emule.h"
#include "UDPSocket.h"
#include "SearchList.h"
#include "DownloadQueue.h"
#include "Server.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "ServerList.h"
#include "Opcodes.h"
#include "SafeFile.h"
#include "PartFile.h"
#include "Packets.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#include "ServerWnd.h"
#include "SearchDlg.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma pack(1)
struct SServerUDPPacket
{
	uint8* packet;
	int size;
	uint32 dwIP;
	uint16 nPort;
};
#pragma pack()


CUDPSocketWnd::CUDPSocketWnd(){
}

BEGIN_MESSAGE_MAP(CUDPSocketWnd, CWnd)
	ON_MESSAGE(WM_DNSLOOKUPDONE, OnDNSLookupDone)
END_MESSAGE_MAP()

LRESULT CUDPSocketWnd::OnDNSLookupDone(WPARAM wParam,LPARAM lParam){
	m_pOwner->DnsLookupDone(wParam,lParam);
	return true;
};

CUDPSocket::CUDPSocket(CServerConnect* in_serverconnect){
	m_hWndResolveMessage = NULL;
	m_sendbuffer = NULL;
	m_cur_server = NULL;
	m_serverconnect = in_serverconnect;
	m_DnsTaskHandle = NULL;
	m_bWouldBlock = false;
}

CUDPSocket::~CUDPSocket(){
	if (m_cur_server)
		delete m_cur_server;
	if (m_sendbuffer)
		delete[] m_sendbuffer;
	POSITION pos = controlpacket_queue.GetHeadPosition();
	while (pos){
		SServerUDPPacket* p = controlpacket_queue.GetNext(pos);
		delete[] p->packet;
		delete p;
	}
	m_udpwnd.DestroyWindow();
}

bool CUDPSocket::Create(){
	if (theApp.glob_prefs->GetServerUDPPort()){
	    VERIFY( m_udpwnd.CreateEx(0, AfxRegisterWndClass(0),_T("Emule Socket Wnd"),WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL));
	    m_hWndResolveMessage = m_udpwnd.m_hWnd;
	    m_udpwnd.m_pOwner = this;
		if (!CAsyncSocket::Create(theApp.glob_prefs->GetServerUDPPort()==0xFFFF ? 0 : theApp.glob_prefs->GetServerUDPPort(), SOCK_DGRAM, FD_READ | FD_WRITE)){
			AddLogLine(true, _T("Error: Server UDP socket: Failed to create server UDP socket on port - %s"), GetErrorMessage(GetLastError()));
			return false;
		}
		return true;
	}
	return false;
}

void CUDPSocket::OnReceive(int nErrorCode){
	if (nErrorCode)
		AddDebugLogLine(false, _T("Error: Server UDP socket: Receive failed - %s"), GetErrorMessage(nErrorCode, 1));

	uint8 buffer[5000];
	CString host;
	uint32 nUDPPort;
	int length = ReceiveFrom(buffer,sizeof buffer,host,nUDPPort);
	if (length != SOCKET_ERROR)
	{
		if (buffer[0] == OP_EDONKEYPROT)
			ProcessPacket(buffer+2, length-2, buffer[1], host, nUDPPort);
		else if (theApp.glob_prefs->GetDebugServerUDP())
			Debug("***NOTE: ServerUDPMessage from %s:%u - Unknown protocol 0x%02x\n", host, nUDPPort-4, buffer[0]);
	}
}

bool CUDPSocket::ProcessPacket(uint8* packet, UINT size, uint8 opcode, LPCTSTR host, uint16 nUDPPort){
	try{
		CServer* update = theApp.serverlist->GetServerByAddress( host, nUDPPort-4 );
		if( update ){
			update->ResetFailedCount();
			theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer( update );
		}
		switch(opcode){
			case OP_GLOBSEARCHRES:{
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile data(packet, size);
				// process all search result packets
				int iLeft;
				int iPacket = 1;
				do{
					if (theApp.glob_prefs->GetDebugServerUDP()){
						uint32 nClientID = *((uint32*)(packet+16));
						uint16 nClientPort = *((uint16*)(packet+20));
						Debug("ServerUDPMessage from %s:%u - OP_GlobSearchResult(%u); %s\n", host, nUDPPort-4, iPacket++, DbgGetFileInfo((uchar*)packet), DbgGetClientID(nClientID), nClientPort);
					}
					uint16 uResultCount = theApp.searchlist->ProcessUDPSearchanswer(data, inet_addr(host), nUDPPort-4);
					theApp.emuledlg->searchwnd->AddUDPResult(uResultCount);

					// check if there is another source packet
					iLeft = (int)(data.GetLength() - data.GetPosition());
					if (iLeft >= 2){
						uint8 protocol;
						data.Read(&protocol, 1);
						iLeft--;
						if (protocol != OP_EDONKEYPROT){
							data.Seek(-1, SEEK_CUR);
							iLeft += 1;
							break;
						}

						uint8 opcode;
						data.Read(&opcode, 1);
						iLeft--;
						if (opcode != OP_GLOBSEARCHRES){
							data.Seek(-2, SEEK_CUR);
							iLeft += 2;
							break;
						}
					}
				}
				while (iLeft > 0);

				if (iLeft > 0 && theApp.glob_prefs->GetDebugServerUDP()){
					Debug("OP_GlobSearchResult contains %d additionl bytes\n", iLeft);
					if (theApp.glob_prefs->GetDebugServerUDP() > 1)
						DebugHexDump(data);
				}
				break;
			}
			case OP_GLOBFOUNDSOURCES:{
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile data(packet, size);
				// process all source packets
				int iLeft;
				int iPacket = 1;
				do{
					uchar fileid[16];
					data.Read(fileid,16);
					if (theApp.glob_prefs->GetDebugServerUDP())
						Debug("ServerUDPMessage from %s:%u - OP_GlobFoundSources(%u); %s\n", host, nUDPPort-4, iPacket++, DbgGetFileInfo(fileid));
					if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid))
						file->AddSources(&data, inet_addr(host), nUDPPort-4);
					else{
						// skip sources for that file
						uint8 count;
						data.Read(&count, 1);
						data.Seek(count*(4+2), SEEK_SET);
					}

					// check if there is another source packet
					iLeft = (int)(data.GetLength() - data.GetPosition());
					if (iLeft >= 2){
						uint8 protocol;
						data.Read(&protocol, 1);
						iLeft--;
						if (protocol != OP_EDONKEYPROT){
							data.Seek(-1, SEEK_CUR);
							iLeft += 1;
							break;
						}

						uint8 opcode;
						data.Read(&opcode, 1);
						iLeft--;
						if (opcode != OP_GLOBFOUNDSOURCES){
							data.Seek(-2, SEEK_CUR);
							iLeft += 2;
							break;
						}
					}
				}
				while (iLeft > 0);

				if (iLeft > 0 && theApp.glob_prefs->GetDebugServerUDP()){
					Debug("OP_GlobFoundSources contains %d additionl bytes\n", iLeft);
					if (theApp.glob_prefs->GetDebugServerUDP() > 1)
						DebugHexDump(data);
				}
				break;
			}
 			case OP_GLOBSERVSTATRES:{
				if (theApp.glob_prefs->GetDebugServerUDP())
					Debug("ServerUDPMessage from %s:%u - OP_GlobServStatRes\n", host, nUDPPort-4);
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				if( size < 12 || update == NULL )
					return true;
#define get_uint32(p)	*((uint32*)(p))
				uint32 challenge = get_uint32(packet);
				if( challenge != update->GetChallenge() )
					return true; 
				uint32 cur_user = get_uint32(packet+4);
				uint32 cur_files = get_uint32(packet+8);
				uint32 cur_maxusers = 0;
				uint32 cur_softfiles = 0;
				uint32 cur_hardfiles = 0;
				uint32 uUDPFlags = 0;
				if( size >= 16 ){
					cur_maxusers = get_uint32(packet+12);
				}
				if( size >= 24 ){
					cur_softfiles = get_uint32(packet+16);
					cur_hardfiles = get_uint32(packet+20);
				}
				if( size >= 28 ){
					uUDPFlags = get_uint32(packet+24);
					if (theApp.glob_prefs->GetDebugServerUDP())
						Debug(" UDP flags=0x%08x\n", uUDPFlags);
				}
				if (theApp.glob_prefs->GetDebugServerUDP()){
					if( size > 28 ){
					    Debug("***NOTE: ServerUDPMessage from %s:%u - OP_GlobServStatRes:  ***AddData: %s\n", host, nUDPPort-4, GetHexDump(packet+24, size-24));
					}
				}
				if( update ){
					update->SetPing( ::GetTickCount() - update->GetLastPinged() );
					update->SetUserCount( cur_user );
					update->SetFileCount( cur_files );
					update->SetMaxUsers( cur_maxusers );
					update->SetSoftFiles( cur_softfiles );
					update->SetHardFiles( cur_hardfiles );
					// if the received UDP flags do not match any already stored UDP flags, 
					// reset the server version string because the version (which was determined by last connecting to
					// that server) is most likely not accurat any longer.
					// this may also give 'false' results because we don't know the UDP flags when connecting to a server
					// with TCP.
					//if (update->GetUDPFlags() != uUDPFlags)
					//	update->SetVersion(_T(""));
					update->SetUDPFlags( uUDPFlags );
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer( update );
				}
#undef get_uint32
				break;
			}

 			case OP_SERVER_DESC_RES:{
				if (theApp.glob_prefs->GetDebugServerUDP())
					Debug("ServerUDPMessage from %s:%u - OP_ServerDescRes\n", host, nUDPPort-4);
				if(!update)
					return true;
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CSafeMemFile srvinfo(packet,size);
				uint16 stringlen;
				srvinfo.Read(&stringlen,2);
				CString strName;
				srvinfo.Read(strName.GetBuffer(stringlen),stringlen);
				strName.ReleaseBuffer(stringlen);
				
				srvinfo.Read(&stringlen,2);
				CString strDesc;
				srvinfo.Read(strDesc.GetBuffer(stringlen),stringlen);
				strDesc.ReleaseBuffer(stringlen);

				if (theApp.glob_prefs->GetDebugServerUDP()){
					UINT uAddData = srvinfo.GetLength() - srvinfo.GetPosition();
					if (uAddData)
						Debug("***NOTE: ServerUDPMessage from %s:%u - OP_ServerDescRes:  ***AddData: %s\n", host, nUDPPort-4, GetHexDump(packet + srvinfo.GetPosition(), uAddData));
				}
				if(update){
					update->SetDescription(strDesc);
					update->SetListName(strName);
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer( update );
				}
				break;
			}
			default:
				Debug("***NOTE: ServerUDPMessage from %s:%u - Unknown packet: opcode=0x%02X  %s\n", host, nUDPPort-4, opcode, GetHexDump(packet, size));
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				return false;
		}

		return true;
	}
	catch(CFileException* error){
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		error->m_strFileName = _T("server UDP packet");
		if (!error->GetErrorMessage(szError, ARRSIZE(szError)))
			szError[0] = _T('\0');
		ProcessPacketError(size, opcode, host, nUDPPort-4, szError);
		error->Delete();
		if (opcode==OP_GLOBSEARCHRES || opcode==OP_GLOBFOUNDSOURCES)
			return true;
	}
	catch(CMemoryException* error){
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (!error->GetErrorMessage(szError, ARRSIZE(szError)))
			szError[0] = _T('\0');
		ProcessPacketError(size, opcode, host, nUDPPort-4, szError);
		error->Delete();
		if (opcode==OP_GLOBSEARCHRES || opcode==OP_GLOBFOUNDSOURCES)
			return true;
	}
	catch(CString error){
		ProcessPacketError(size, opcode, host, nUDPPort-4, error);
	}
	catch(...){
		ProcessPacketError(size, opcode, host, nUDPPort-4, _T("Unknown exception"));
		ASSERT(0);
	}

	return false;
}

void CUDPSocket::ProcessPacketError(UINT size, uint8 opcode, LPCTSTR host, uint16 nTCPPort, LPCTSTR pszError)
{
	CString strName;
	CServer* pServer = theApp.serverlist->GetServerByAddress(host, nTCPPort);
	if (pServer)
		strName = _T(" (") + CString(pServer->GetListName()) + _T(")");
	AddDebugLogLine(false, _T("Error: Failed to process server UDP packet from %s:%u%s opcode=0x%02x size=%u - %s"), host, nTCPPort, strName, opcode, size, pszError);
}

void CUDPSocket::AsyncResolveDNS(LPCTSTR lpszHostAddress, UINT nHostPort){
	m_lpszHostAddress = lpszHostAddress;
	m_nHostPort = nHostPort;
	if (m_DnsTaskHandle)
		WSACancelAsyncRequest(m_DnsTaskHandle);
	m_DnsTaskHandle = NULL;

	// see if we have a ip already
	USES_CONVERSION;
	SOCKADDR_IN sockAddr;
	memset(&sockAddr,0,sizeof(sockAddr));
	LPSTR lpszAscii = T2A((LPTSTR)m_lpszHostAddress);
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(lpszAscii);
	sockAddr.sin_port = htons((u_short)m_nHostPort);

	// backup for send socket
	m_SaveAddr = sockAddr;

	if (sockAddr.sin_addr.s_addr == INADDR_NONE){
		/* Resolve hostname "hostname" asynchronously */ 
		memset(m_DnsHostBuffer, 0, sizeof(m_DnsHostBuffer));

		m_DnsTaskHandle = WSAAsyncGetHostByName(
			m_hWndResolveMessage,
			WM_DNSLOOKUPDONE,
			lpszHostAddress,
			m_DnsHostBuffer,
			MAXGETHOSTSTRUCT);

		if (m_DnsTaskHandle == NULL){
			delete[] m_sendbuffer;
			m_sendbuffer = NULL;
			delete m_cur_server;
			m_cur_server = NULL;
#ifdef _DEBUG
			AfxMessageBox("LOOKUPERROR DNSTASKHANDLE = 0");
#endif
		}
	}
	else{
		SendBuffer();
	}
}

void CUDPSocket::DnsLookupDone(WPARAM wp, LPARAM lp){
	m_DnsTaskHandle = NULL;

	/* An asynchronous database routine completed. */
	if (WSAGETASYNCERROR(lp) != 0){
		if (m_sendbuffer){
			delete[] m_sendbuffer;
			m_sendbuffer = NULL;
		}
		if (m_cur_server){
			delete m_cur_server;
			m_cur_server = NULL;
		}
		return;
	}
	if (m_SaveAddr.sin_addr.s_addr == INADDR_NONE){
		// get the structure length
		int iBufLen = WSAGETASYNCBUFLEN(lp);
		if (iBufLen >= sizeof(HOSTENT)){
			LPHOSTENT pHost = (LPHOSTENT)m_DnsHostBuffer;
			if (pHost->h_length == 4 && pHost->h_addr_list && pHost->h_addr_list[0])
				m_SaveAddr.sin_addr.s_addr = ((LPIN_ADDR)(pHost->h_addr_list[0]))->s_addr;
		}
		// also reset the receive buffer
		memset(m_DnsHostBuffer, 0, sizeof(m_DnsHostBuffer));
	}
	if (m_cur_server){
		if (m_SaveAddr.sin_addr.s_addr != INADDR_NONE){
			CServer* update = theApp.serverlist->GetServerByAddress(m_cur_server->GetAddress(),m_cur_server->GetPort());
		    if (update)
			    update->SetID(m_SaveAddr.sin_addr.S_un.S_addr);
			SendBuffer();
		}
		else{
			// still no valid IP for this server - delete packet
			delete m_cur_server;
			m_cur_server = NULL;
			delete[] m_sendbuffer;
			m_sendbuffer = NULL;
			m_sendblen = 0;
		}
	}
}

void CUDPSocket::OnSend(int nErrorCode){
	if (nErrorCode){
		AddDebugLogLine(false, _T("Error: Server UDP socket: Failed to send packet - %s"), GetErrorMessage(nErrorCode, 1));
		return;
	}
	m_bWouldBlock = false;
	while (controlpacket_queue.GetHeadPosition() != 0 && !IsBusy())
	{
		SServerUDPPacket* packet = controlpacket_queue.GetHead();
		if (SendTo(packet->packet, packet->size, packet->dwIP, packet->nPort) > 0){
			controlpacket_queue.RemoveHead();
			delete[] packet->packet;
			delete packet;
		}
	}
}

int CUDPSocket::SendTo(uint8* lpBuf, int nBufLen, uint32 dwIP, uint16 nPort){
	in_addr host;
	host.S_un.S_addr = dwIP;
	int iResult = CAsyncSocket::SendTo(lpBuf, nBufLen, nPort, inet_ntoa(host));
	if (iResult == SOCKET_ERROR){
		DWORD dwError = GetLastError();
		if (dwError == WSAEWOULDBLOCK){
			m_bWouldBlock = true;
			return -1; // blocked
		}
		else{
			AddDebugLogLine(false, _T("Error: Server UDP socket: Failed to send packet to %s:%u - %s"), inet_ntoa(host), nPort, GetErrorMessage(dwError, 1));
			return 0; // error
		}
	}
	return 1; // success
}

void CUDPSocket::SendBuffer(){
	if(m_cur_server && m_sendbuffer){
		u_short nPort = ntohs(m_SaveAddr.sin_port);
		if (IsBusy() || SendTo(m_sendbuffer, m_sendblen, m_SaveAddr.sin_addr.s_addr, nPort) < 0){
			SServerUDPPacket* newpending = new SServerUDPPacket;
			newpending->dwIP = m_SaveAddr.sin_addr.s_addr;
			newpending->nPort = nPort;
			newpending->packet = m_sendbuffer;
			newpending->size = m_sendblen;
			controlpacket_queue.AddTail(newpending);
		}
		else{
			// on success or error, delete the packet
			delete[] m_sendbuffer;
		}
		m_sendbuffer = NULL;
		m_sendblen = 0;
		delete m_cur_server;
		m_cur_server = NULL;
	}
}

void CUDPSocket::SendPacket(Packet* packet,CServer* host){
	// if the last DNS query did not yet return, we may still have a packet queued - delete it
	delete m_cur_server;
	m_cur_server = NULL;
	delete[] m_sendbuffer;
	m_sendbuffer = NULL;
	m_sendblen = 0;

	m_cur_server = new CServer(host);
	m_sendbuffer = new uint8[packet->size+2];
	memcpy(m_sendbuffer,packet->GetUDPHeader(),2);
	memcpy(m_sendbuffer+2,packet->pBuffer,packet->size);
	m_sendblen = packet->size+2;
	AsyncResolveDNS(m_cur_server->GetAddress(),m_cur_server->GetPort()+4);
}
