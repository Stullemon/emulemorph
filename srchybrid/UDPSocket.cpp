//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "Statistics.h"
#include "Server.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "ServerList.h"
#include "Opcodes.h"
#include "SafeFile.h"
#include "PartFile.h"
#include "Packets.h"
#include "IPFilter.h"
#include "emuledlg.h"
#include "ServerWnd.h"
#include "SearchDlg.h"
#include "Log.h"
#include "FirewallOpener.h" //MORPH - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
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

#define WM_DNSLOOKUPDONE	(WM_USER+0x101)		// does not need to be placed in "UserMsgs.h"

CUDPSocketWnd::CUDPSocketWnd(){
}

BEGIN_MESSAGE_MAP(CUDPSocketWnd, CWnd)
	ON_MESSAGE(WM_DNSLOOKUPDONE, OnDNSLookupDone)
END_MESSAGE_MAP()

LRESULT CUDPSocketWnd::OnDNSLookupDone(WPARAM wParam,LPARAM lParam){
	m_pOwner->DnsLookupDone(wParam,lParam);
	return true;
};

CUDPSocket::CUDPSocket(){
	m_hWndResolveMessage = NULL;
	m_sendbuffer = NULL;
	m_cur_server = NULL;
	m_DnsTaskHandle = NULL;
	m_bWouldBlock = false;
}

CUDPSocket::~CUDPSocket(){
    theApp.uploadBandwidthThrottler->RemoveFromAllQueues(this); // ZZ:UploadBandWithThrottler (UDP)

    delete m_cur_server;
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
	if (thePrefs.GetServerUDPPort()){
		VERIFY( m_udpwnd.CreateEx(0, AfxRegisterWndClass(0), _T("eMule Async DNS Resolve Socket Wnd #1"), WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL));
	    m_hWndResolveMessage = m_udpwnd.m_hWnd;
	    m_udpwnd.m_pOwner = this;
		if (!CAsyncSocket::Create(thePrefs.GetServerUDPPort()==0xFFFF ? 0 : thePrefs.GetServerUDPPort(), SOCK_DGRAM, FD_READ | FD_WRITE)){
			LogError(LOG_STATUSBAR, _T("Error: Server UDP socket: Failed to create server UDP socket on port - %s"), GetErrorMessage(GetLastError()));
			return false;
		}

		// emulEspaña: Added by MoNKi [MoNKi: -UPnPNAT Support-]
		// Don't add UPnP port mapping if is a random port and we don't want
		// to clear mappings on close
		if(thePrefs.IsUPnPEnabled() &&
			(!(thePrefs.GetServerUDPPort()==0xFFFF && !thePrefs.GetUPnPClearOnClose())))
		{
			CString client;
			UINT port;
			GetSockName(client, port);
			
			//MORPH START -, Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]
			if(thePrefs.GetICFSupport() && thePrefs.GetICFSupportServerUDP()){
				if (theApp.m_pFirewallOpener->OpenPort(port, NAT_PROTOCOL_UDP, EMULE_DEFAULTRULENAME_SERVERUDP, thePrefs.IsOpenPortsOnStartupEnabled() || thePrefs.GetServerUDPPort()==0xFFFF))
					Log(GetResString(IDS_FO_TEMPUDP_S), port);
				else
					Log(GetResString(IDS_FO_TEMPUDP_F), port);
			}
			//MORPH END   - Added by SiRoB, [MoNKi: -Improved ICS-Firewall support-]

			theApp.m_UPnP_IGDControlPoint->AddPortMapping(port,
				CUPnP_IGDControlPoint::UNAT_UDP,
				_T("Server UDP Port"));
		}
		// End -UPnPNAT Support-

		return true;
	}
	return false;
}

void CUDPSocket::OnReceive(int nErrorCode)
{
	if (nErrorCode)
	{
		if (thePrefs.GetDebugServerUDPLevel() > 0)
			Debug(_T("Error: Server UDP socket: Receive failed - %s\n"), GetErrorMessage(nErrorCode, 1));
		if (thePrefs.GetVerbose())
			DebugLogError(_T("Error: Server UDP socket: Receive failed - %s"), GetErrorMessage(nErrorCode, 1));
	}

	uint8 buffer[5000];
	SOCKADDR_IN sockAddr = {0};
	int iSockAddrLen = sizeof sockAddr;
	int length = ReceiveFrom(buffer, sizeof buffer, (SOCKADDR*)&sockAddr, &iSockAddrLen);
	if (length != SOCKET_ERROR)
	{
		if (buffer[0] == OP_EDONKEYPROT)
			ProcessPacket(buffer+2, length-2, buffer[1], sockAddr.sin_addr.S_un.S_addr, ntohs(sockAddr.sin_port));
		else if (thePrefs.GetDebugServerUDPLevel() > 0)
			Debug(_T("***NOTE: ServerUDPMessage from %s:%u - Unknown protocol 0x%02x\n"), ipstr(sockAddr.sin_addr), ntohs(sockAddr.sin_port)-4, buffer[0]);
	}
	else
	{
		DWORD dwError = WSAGetLastError();
		if (thePrefs.GetDebugServerUDPLevel() > 0) {
			CString strServerInfo;
			if (iSockAddrLen > 0 && sockAddr.sin_addr.S_un.S_addr != 0 && sockAddr.sin_addr.S_un.S_addr != INADDR_NONE)
				strServerInfo.Format(_T(" from %s:%u"), ipstr(sockAddr.sin_addr), ntohs(sockAddr.sin_port)-4);
			Debug(_T("Error: Server UDP socket: Failed to receive data%s: %s\n"), strServerInfo, GetErrorMessage(dwError, 1));
		}
		if (dwError == WSAECONNRESET)
		{
			// Depending on local and remote OS and depending on used local (remote?) router we may receive
			// WSAECONNRESET errors. According some KB articels, this is a special way of winsock to report 
			// that a sent UDP packet was not received by the remote host because it was not listening on 
			// the specified port -> no server running there.
			//

			// If we are not currently pinging this server, increase the failure counter
			CServer* pServer = theApp.serverlist->GetServerByAddress(ipstr(sockAddr.sin_addr), ntohs(sockAddr.sin_port)-4);
			if (pServer && GetTickCount() - pServer->GetLastPinged() >= SEC2MS(30))
			{
				pServer->AddFailedCount();
				theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(pServer);
			}
		}
		else if (thePrefs.GetVerbose())
		{
			CString strServerInfo;
			if (iSockAddrLen > 0 && sockAddr.sin_addr.S_un.S_addr != 0 && sockAddr.sin_addr.S_un.S_addr != INADDR_NONE)
				strServerInfo.Format(_T(" from %s:%u"), ipstr(sockAddr.sin_addr), ntohs(sockAddr.sin_port)-4);
			DebugLogError(_T("Error: Server UDP socket: Failed to receive data%s: %s"), strServerInfo, GetErrorMessage(dwError, 1));
		}
	}
}

bool CUDPSocket::ProcessPacket(const BYTE* packet, UINT size, UINT opcode, uint32 nIP, uint16 nUDPPort)
{
	try
	{
		theStats.AddDownDataOverheadServer(size);
		CServer* update = theApp.serverlist->GetServerByAddress(ipstr(nIP), nUDPPort-4);
		if( update ){
			update->ResetFailedCount();
			theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer( update );
		}

		switch (opcode)
		{
			case OP_GLOBSEARCHRES:{
				CSafeMemFile data(packet, size);
				// process all search result packets
				int iLeft;
				int iDbgPacket = 1;
				do{
					if (thePrefs.GetDebugServerUDPLevel() > 0){
						if (data.GetLength() - data.GetPosition() >= 16+4+2){
							const BYTE* pDbgPacket = data.GetBuffer() + data.GetPosition();
							Debug(_T("ServerUDPMessage from %-21s - OP_GlobSearchResult(%u); %s\n"), ipstr(nIP, nUDPPort-4), iDbgPacket++, DbgGetFileInfo(pDbgPacket), DbgGetClientID(PeekUInt32(pDbgPacket+16)), PeekUInt16(pDbgPacket+20));
						}
					}
					uint16 uResultCount = theApp.searchlist->ProcessUDPSearchAnswer(data, true/*update->GetUnicodeSupport()*/, nIP, nUDPPort-4);
					theApp.emuledlg->searchwnd->AddUDPResult(uResultCount);

					// check if there is another source packet
					iLeft = (int)(data.GetLength() - data.GetPosition());
					if (iLeft >= 2){
						uint8 protocol = data.ReadUInt8();
						iLeft--;
						if (protocol != OP_EDONKEYPROT){
							data.Seek(-1, SEEK_CUR);
							iLeft += 1;
							break;
						}

						uint8 opcode = data.ReadUInt8();
						iLeft--;
						if (opcode != OP_GLOBSEARCHRES){
							data.Seek(-2, SEEK_CUR);
							iLeft += 2;
							break;
						}
					}
				}
				while (iLeft > 0);

				if (iLeft > 0 && thePrefs.GetDebugServerUDPLevel() > 0){
					Debug(_T("***NOTE: OP_GlobSearchResult contains %d additional bytes\n"), iLeft);
					if (thePrefs.GetDebugServerUDPLevel() > 1)
						DebugHexDump(data);
				}
				break;
			}
			case OP_GLOBFOUNDSOURCES:{
				CSafeMemFile data(packet, size);
				// process all source packets
				int iLeft;
				int iDbgPacket = 1;
				do{
					uchar fileid[16];
					data.ReadHash16(fileid);
					if (thePrefs.GetDebugServerUDPLevel() > 0)
						Debug(_T("ServerUDPMessage from %-21s - OP_GlobFoundSources(%u); %s\n"), ipstr(nIP, nUDPPort-4), iDbgPacket++, DbgGetFileInfo(fileid));
					if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid))
						file->AddSources(&data, nIP, nUDPPort-4);
					else{
						// skip sources for that file
						UINT count = data.ReadUInt8();
						data.Seek(count*(4+2), SEEK_CUR);
					}

					// check if there is another source packet
					iLeft = (int)(data.GetLength() - data.GetPosition());
					if (iLeft >= 2){
						uint8 protocol = data.ReadUInt8();
						iLeft--;
						if (protocol != OP_EDONKEYPROT){
							data.Seek(-1, SEEK_CUR);
							iLeft += 1;
							break;
						}

						uint8 opcode = data.ReadUInt8();
						iLeft--;
						if (opcode != OP_GLOBFOUNDSOURCES){
							data.Seek(-2, SEEK_CUR);
							iLeft += 2;
							break;
						}
					}
				}
				while (iLeft > 0);

				if (iLeft > 0 && thePrefs.GetDebugServerUDPLevel() > 0){
					Debug(_T("***NOTE: OP_GlobFoundSources contains %d additional bytes\n"), iLeft);
					if (thePrefs.GetDebugServerUDPLevel() > 1)
						DebugHexDump(data);
				}
				break;
			}
 			case OP_GLOBSERVSTATRES:{
				if (thePrefs.GetDebugServerUDPLevel() > 0)
					Debug(_T("ServerUDPMessage from %-21s - OP_GlobServStatRes\n"), ipstr(nIP, nUDPPort-4));
				if( size < 12 || update == NULL )
					return true;
				uint32 challenge = PeekUInt32(packet);
				if (challenge != update->GetChallenge()){
					if (thePrefs.GetDebugServerUDPLevel() > 0)
						Debug(_T("***NOTE: Received unexpected challenge %08x (waiting on packet with challenge %08x)\n"), challenge, update->GetChallenge());
					return true;
				}
				update->SetChallenge(0);
				uint32 cur_user = PeekUInt32(packet+4);
				uint32 cur_files = PeekUInt32(packet+8);
				uint32 cur_maxusers = 0;
				uint32 cur_softfiles = 0;
				uint32 cur_hardfiles = 0;
				uint32 uUDPFlags = 0;
				uint32 uLowIDUsers = 0;
				if( size >= 16 ){
					cur_maxusers = PeekUInt32(packet+12);
				}
				if( size >= 24 ){
					cur_softfiles = PeekUInt32(packet+16);
					cur_hardfiles = PeekUInt32(packet+20);
				}
				if( size >= 28 ){
					uUDPFlags = PeekUInt32(packet+24);
					if (thePrefs.GetDebugServerUDPLevel() > 0){
						CString strInfo;
						const DWORD dwKnownBits = SRV_UDPFLG_EXT_GETSOURCES | SRV_UDPFLG_EXT_GETFILES | SRV_UDPFLG_NEWTAGS | SRV_UDPFLG_UNICODE | SRV_UDPFLG_EXT_GETSOURCES2;
						if (uUDPFlags & ~dwKnownBits)
							strInfo.AppendFormat(_T("  ***UnkUDPFlags=0x%08x"), uUDPFlags & ~dwKnownBits);
						if (uUDPFlags & SRV_UDPFLG_EXT_GETSOURCES)
							strInfo.AppendFormat(_T("  ExtGetSources=1"));
						if (uUDPFlags & SRV_UDPFLG_EXT_GETSOURCES2)
							strInfo.AppendFormat(_T("  ExtGetSources2=1"));
						if (uUDPFlags & SRV_UDPFLG_EXT_GETFILES)
							strInfo.AppendFormat(_T("  ExtGetFiles=1"));
						if (uUDPFlags & SRV_UDPFLG_NEWTAGS)
							strInfo.AppendFormat(_T("  NewTags=1"));
						if (uUDPFlags & SRV_UDPFLG_UNICODE)
							strInfo.AppendFormat(_T("  Unicode=1"));
						Debug(_T("%s\n"), strInfo);
					}
				}
				if( size >= 32 ){
					uLowIDUsers = PeekUInt32(packet+28);
				}
				if (thePrefs.GetDebugServerUDPLevel() > 0){
					if( size > 32 ){
						Debug(_T("***NOTE: OP_GlobServStatRes contains %d additional bytes\n"), size-32);
						if (thePrefs.GetDebugServerUDPLevel() > 1)
							DbgGetHexDump(packet+32, size-32);
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
					update->SetLowIDUsers( uLowIDUsers );
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer( update );
				}
				break;
			}

 			case OP_SERVER_DESC_RES:{
				if (thePrefs.GetDebugServerUDPLevel() > 0)
					Debug(_T("ServerUDPMessage from %-21s - OP_ServerDescRes\n"), ipstr(nIP, nUDPPort-4));
				if (!update)
					return true;

				// old packet: <name_len 2><name name_len><desc_len 2 desc_en>
				// new packet: <challenge 4><taglist>
				//
				// NOTE: To properly distinguish between the two packets which are both useing the same opcode...
				// the first two bytes of <challenge> (in network byte order) have to be an invalid <name_len> at least.

				CSafeMemFile srvinfo(packet, size);
				if (size >= 8 && PeekUInt16(packet) == INV_SERV_DESC_LEN)
				{
					if (update->GetDescReqChallenge() != 0 && PeekUInt32(packet) == update->GetDescReqChallenge())
					{
						update->SetDescReqChallenge(0);
						(void)srvinfo.ReadUInt32(); // skip challenge
						UINT uTags = srvinfo.ReadUInt32();
						for (UINT i = 0; i < uTags; i++)
						{
							CTag tag(&srvinfo, true/*update->GetUnicodeSupport()*/);
							if (tag.GetNameID() == ST_SERVERNAME && tag.IsStr())
								update->SetListName(tag.GetStr());
							else if (tag.GetNameID() == ST_DESCRIPTION && tag.IsStr())
								update->SetDescription(tag.GetStr());
							else if (tag.GetNameID() == ST_DYNIP && tag.IsStr())
								update->SetDynIP(tag.GetStr());
							else if (tag.GetNameID() == ST_VERSION && tag.IsStr())
								update->SetVersion(tag.GetStr());
							else if (tag.GetNameID() == ST_VERSION && tag.IsInt()){
								CString strVersion;
								strVersion.Format(_T("%u.%u"), tag.GetInt() >> 16, tag.GetInt() & 0xFFFF);
								update->SetVersion(strVersion);
							}
							else if (tag.GetNameID() == ST_AUXPORTSLIST && tag.IsStr())
								// currently not implemented.
								; // <string> = <port> [, <port>...]
							else{
								if (thePrefs.GetDebugServerUDPLevel() > 0)
									Debug(_T("***NOTE: Unknown tag in OP_ServerDescRes: %s\n"), tag.GetFullInfo());
							}
						}
					}
					else
					{
						// A server sent us a new server description packet (including a challenge) although we did not
						// ask for it. This may happen, if there are multiple servers running on the same machine with
						// multiple IPs. If such a server is asked for a description, the server will answer 2 times,
						// but with the same IP.

						if (thePrefs.GetDebugServerUDPLevel() > 0)
							Debug(_T("***NOTE: Received unexpected new format OP_ServerDescRes from %s with challenge %08x (waiting on packet with challenge %08x)\n"), ipstr(nIP, nUDPPort-4), PeekUInt32(packet), update->GetDescReqChallenge());
						; // ignore this packet
					}
				}
				else
				{
					CString strName = srvinfo.ReadString(true/*update->GetUnicodeSupport()*/);
					CString strDesc = srvinfo.ReadString(true/*update->GetUnicodeSupport()*/);
					update->SetDescription(strDesc);
					update->SetListName(strName);
				}

				if (thePrefs.GetDebugServerUDPLevel() > 0){
					int iAddData = (int)(srvinfo.GetLength() - srvinfo.GetPosition());
					if (iAddData > 0){
						Debug(_T("***NOTE: OP_ServerDescRes contains %d additional bytes\n"), iAddData);
						if (thePrefs.GetDebugServerUDPLevel() > 1)
							DebugHexDump(srvinfo);
					}
				}
				theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(update);
				break;
			}
			default:
				if (thePrefs.GetDebugServerUDPLevel() > 0)
					Debug(_T("***NOTE: ServerUDPMessage from %s - Unknown packet: opcode=0x%02X  %s\n"), ipstr(nIP, nUDPPort-4), opcode, DbgGetHexDump(packet, size));
				return false;
		}

		return true;
	}
	catch(CFileException* error){
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		error->m_strFileName = _T("server UDP packet");
		if (!error->GetErrorMessage(szError, ARRSIZE(szError)))
			szError[0] = _T('\0');
		ProcessPacketError(size, opcode, nIP, nUDPPort-4, szError);
		error->Delete();
		//ASSERT(0);
		if (opcode==OP_GLOBSEARCHRES || opcode==OP_GLOBFOUNDSOURCES)
			return true;
	}
	catch(CMemoryException* error){
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (!error->GetErrorMessage(szError, ARRSIZE(szError)))
			szError[0] = _T('\0');
		ProcessPacketError(size, opcode, nIP, nUDPPort-4, szError);
		error->Delete();
		//ASSERT(0);
		if (opcode==OP_GLOBSEARCHRES || opcode==OP_GLOBFOUNDSOURCES)
			return true;
	}
	catch(CString error){
		ProcessPacketError(size, opcode, nIP, nUDPPort-4, error);
		//ASSERT(0);
	}
	catch(...){
		ProcessPacketError(size, opcode, nIP, nUDPPort-4, _T("Unknown exception"));
		ASSERT(0);
	}

	return false;
}

void CUDPSocket::ProcessPacketError(UINT size, UINT opcode, uint32 nIP, uint16 nTCPPort, LPCTSTR pszError)
{
	if (thePrefs.GetVerbose())
	{
		CString strName;
		CServer* pServer = theApp.serverlist->GetServerByAddress(ipstr(nIP), nTCPPort);
		if (pServer)
			strName = _T(" (") + pServer->GetListName() + _T(")");
		DebugLogWarning(false, _T("Error: Failed to process server UDP packet from %s:%u%s opcode=0x%02x size=%u - %s"), ipstr(nIP), nTCPPort, strName, opcode, size, pszError);
	}
}

void CUDPSocket::AsyncResolveDNS(LPCSTR lpszHostAddressA, UINT nHostPort)
{
	if (m_DnsTaskHandle){
		WSACancelAsyncRequest(m_DnsTaskHandle);
		m_DnsTaskHandle = NULL;
	}

	// see if we have a ip already
	SOCKADDR_IN sockAddr = {0};
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_addr.s_addr = inet_addr(lpszHostAddressA);
	sockAddr.sin_port = htons((u_short)nHostPort);

	// backup for send socket
	m_SaveAddr = sockAddr;

	if (sockAddr.sin_addr.s_addr == INADDR_NONE){
		/* Resolve hostname "hostname" asynchronously */ 
		memset(m_DnsHostBuffer, 0, sizeof(m_DnsHostBuffer));

		m_DnsTaskHandle = WSAAsyncGetHostByName(
			m_hWndResolveMessage,
			WM_DNSLOOKUPDONE,
			lpszHostAddressA,
			m_DnsHostBuffer,
			MAXGETHOSTSTRUCT);

		if (m_DnsTaskHandle == NULL){
			if (thePrefs.GetVerbose())
				DebugLogWarning(_T("Error: Server UDP socket: Failed to resolve address for '%hs' - %s"), lpszHostAddressA, GetErrorMessage(GetLastError(), 1));
			delete[] m_sendbuffer;
			m_sendbuffer = NULL;
			delete m_cur_server;
			m_cur_server = NULL;
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
		if (thePrefs.GetVerbose())
			DebugLogWarning(_T("Error: Server UDP socket: Failed to resolve address for server '%s' (%s) - %s"), m_cur_server ? m_cur_server->GetListName() : _T(""), m_cur_server ? m_cur_server->GetAddress() : _T(""), GetErrorMessage(WSAGETASYNCERROR(lp), 1));
		delete[] m_sendbuffer;
		m_sendbuffer = NULL;
		delete m_cur_server;
		m_cur_server = NULL;
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
			bool bRemoveServer = false;
			if (!IsGoodIP(m_SaveAddr.sin_addr.s_addr)){
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Resolved IP for server '%s' is %s - Invalid IP or LAN address, server deleted."), m_cur_server->GetListName(), ipstr(m_SaveAddr.sin_addr.s_addr));
				bRemoveServer = true;
			}
			if (!bRemoveServer && theApp.ipfilter->IsFiltered(m_SaveAddr.sin_addr.s_addr)){
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Resolved IP for server '%s' is %s - Found in IP filter, server deleted."), m_cur_server->GetListName(), ipstr(m_SaveAddr.sin_addr.s_addr));
				bRemoveServer = true;
			}

			if (!bRemoveServer){
				CServer* update = theApp.serverlist->GetServerByAddress(m_cur_server->GetAddress(),m_cur_server->GetPort());
			    if (update)
				    update->SetIP(m_SaveAddr.sin_addr.S_un.S_addr);
				SendBuffer();
			}
			else{
				CServer* todel = theApp.serverlist->GetServerByAddress(m_cur_server->GetAddress(), m_cur_server->GetPort());
				if (todel)
					theApp.emuledlg->serverwnd->serverlistctrl.RemoveServer(todel);
				delete m_cur_server;
				m_cur_server = NULL;
				delete[] m_sendbuffer;
				m_sendbuffer = NULL;
				m_sendblen = 0;
			}
		}
		else{
			// still no valid IP for this server - delete packet
			if (thePrefs.GetVerbose())
				DebugLogWarning(_T("Error: Server UDP socket: Failed to resolve address for server '%s' (%s)"), m_cur_server->GetListName(), m_cur_server->GetAddress());
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
		if (thePrefs.GetVerbose())
			DebugLogError(_T("Error: Server UDP socket: Failed to send packet - %s"), GetErrorMessage(nErrorCode, 1));
		return;
	}
	//m_bWouldBlock = false; //MORPH - Removed by SiRoB, moved after the lock -Fix-

// ZZ:UploadBandWithThrottler (UDP) -->
    sendLocker.Lock();
	m_bWouldBlock = false; //MORPH - Added by SiRoB, moved after the lock -Fix-
	if(!controlpacket_queue.IsEmpty()) {
	    theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
    }
    sendLocker.Unlock();
// <-- ZZ:UploadBandWithThrottler (UDP)
}

SocketSentBytes CUDPSocket::SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize) { // ZZ:UploadBandWithThrottler (UDP)
// ZZ:UploadBandWithThrottler (UDP) -->
	// NOTE: *** This function is invoked from a *different* thread!
    sendLocker.Lock();

    uint32 sentBytes = 0;
// <-- ZZ:UploadBandWithThrottler (UDP)
    while (controlpacket_queue.GetHeadPosition() != 0 && !IsBusy() && sentBytes < maxNumberOfBytesToSend) // ZZ:UploadBandWithThrottler (UDP)
	{
		SServerUDPPacket* packet = controlpacket_queue.GetHead();
        int sendSuccess = SendTo(packet->packet, packet->size, packet->dwIP, packet->nPort);
		if (sendSuccess >= 0){
            if(sendSuccess > 0) {
                sentBytes += packet->size; // ZZ:UploadBandWithThrottler (UDP)
            }

			controlpacket_queue.RemoveHead();
			delete[] packet->packet;
			delete packet;
		}
	}

// ZZ:UploadBandWithThrottler (UDP) -->
    if(!IsBusy() && !controlpacket_queue.IsEmpty()) {
        theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
    }
    sendLocker.Unlock();
    
    SocketSentBytes returnVal = { true, 0, sentBytes };
    return returnVal;
// <-- ZZ:UploadBandWithThrottler (UDP)
}

int CUDPSocket::SendTo(uint8* lpBuf, int nBufLen, uint32 dwIP, uint16 nPort){
	// NOTE: *** This function is invoked from a *different* thread!
	int iResult = CAsyncSocket::SendTo(lpBuf, nBufLen, nPort, ipstr(dwIP));
	if (iResult == SOCKET_ERROR){
		DWORD dwError = GetLastError();
		if (dwError == WSAEWOULDBLOCK){
			m_bWouldBlock = true;
			return -1; // blocked
		}
		else{
			if (thePrefs.GetVerbose())
				theApp.QueueDebugLogLine(false, _T("Error: Server UDP socket: Failed to send packet to %s:%u - %s"), ipstr(dwIP), nPort, GetErrorMessage(dwError, 1));
			return 0; // error
		}
	}
	return 1; // success
}

void CUDPSocket::SendBuffer(){
	if(m_cur_server && m_sendbuffer){
		u_short nPort = ntohs(m_SaveAddr.sin_port);
// ZZ:UploadBandWithThrottler (UDP) -->
		SServerUDPPacket* newpending = new SServerUDPPacket;
		newpending->dwIP = m_SaveAddr.sin_addr.s_addr;
		newpending->nPort = nPort;
		newpending->packet = m_sendbuffer;
		newpending->size = m_sendblen;
        sendLocker.Lock();
		controlpacket_queue.AddTail(newpending);
        sendLocker.Unlock();
        theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
// <-- ZZ:UploadBandWithThrottler (UDP)

		m_sendbuffer = NULL;
		m_sendblen = 0;
		delete m_cur_server;
		m_cur_server = NULL;
	}
}

void CUDPSocket::SendPacket(Packet* packet,CServer* host){
	USES_CONVERSION;
	// if the last DNS query did not yet return, we may still have a packet queued - delete it
	if (thePrefs.GetVerbose() && m_cur_server)
		DebugLogWarning(_T("Warning: Server UDP socket: Timeout occured when trying to resolve address for server '%s' (%s)"), m_cur_server->GetListName(), m_cur_server->GetAddress());
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
	AsyncResolveDNS(T2CA(m_cur_server->GetAddress()),m_cur_server->GetPort()+4);
}
