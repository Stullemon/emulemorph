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
#include "ServerSocket.h"
#include "SearchList.h"
#include "DownloadQueue.h"
#include "ClientList.h"
#include "Server.h"
#include "ServerList.h"
#include "Sockets.h"
#include "OtherFunctions.h"
#include "Opcodes.h"
#include "Preferences.h"
#include "SafeFile.h"
#include "PartFile.h"
#include "Packets.h"
#include "UpDownClient.h"
#ifndef _CONSOLE
#include "emuleDlg.h"
#include "ServerWnd.h"
#include "SearchDlg.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma pack(1)
struct LoginAnswer_Struct {
	uint32	clientid;
};
#pragma pack()


CServerSocket::CServerSocket(CServerConnect* in_serverconnect){
	serverconnect = in_serverconnect;
	connectionstate = 0;
	cur_server = 0;
	m_bIsDeleting = false;
	info="";
	m_dwLastTransmission = 0;
}

CServerSocket::~CServerSocket(){
	if (cur_server)
		delete cur_server;
	cur_server = NULL;
}

void CServerSocket::OnConnect(int nErrorCode){
	CAsyncSocketEx::OnConnect(nErrorCode); // deadlake PROXYSUPPORT - changed to AsyncSocketEx
	switch (nErrorCode){
		case 0:{
			if (cur_server->HasDynIP()){
				SOCKADDR_IN sockAddr = {0};
				int nSockAddrLen = sizeof(sockAddr);
				GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
				cur_server->SetID(sockAddr.sin_addr.S_un.S_addr);
				theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort())->SetID(sockAddr.sin_addr.S_un.S_addr);
			}
			SetConnectionState(CS_WAITFORLOGIN);
			break;
		}
		case WSAEADDRNOTAVAIL:
		case WSAECONNREFUSED:
		case WSAENETUNREACH:
		case WSAETIMEDOUT: 	
		case WSAEADDRINUSE:
			m_bIsDeleting = true;
			SetConnectionState(CS_SERVERDEAD);
			serverconnect->DestroySocket(this);
			return;
		// deadlake PROXYSUPPORT
		case WSAECONNABORTED:
			if (m_ProxyConnectFailed)
			{
				m_ProxyConnectFailed = false;
				m_bIsDeleting = true;
				SetConnectionState(CS_SERVERDEAD);
				serverconnect->DestroySocket(this);
				return;
			}
		default:	
			m_bIsDeleting = true;
			SetConnectionState(CS_FATALERROR);
			serverconnect->DestroySocket(this);
			return;
	}	 
}

void CServerSocket::OnReceive(int nErrorCode){
	if (connectionstate != CS_CONNECTED && !this->serverconnect->IsConnecting()){
		serverconnect->DestroySocket(this);
		return;
	}
	CEMSocket::OnReceive(nErrorCode);
	m_dwLastTransmission = GetTickCount();
}

bool CServerSocket::ProcessPacket(char* packet, uint32 size, uint8 opcode){
	try{
		switch(opcode){
			case OP_SERVERMESSAGE:{
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug("ServerMsg - OP_ServerMessage\n");

				CStringA strMessages;
				if (size >= 2){
					UINT uLen = *(uint16*)packet;
					if (uLen > size-2)
						uLen = size-2;
					memcpy(strMessages.GetBuffer(uLen), packet + sizeof uint16, uLen);
					strMessages.ReleaseBuffer(uLen);
				}

				// 16.40 servers do not send separate OP_SERVERMESSAGE packets for each line;
				// instead of this they are sending all text lines with one OP_SERVERMESSAGE packet.
				int iPos = 0;
				CString message = strMessages.Tokenize("\r\n", iPos);
				while (!message.IsEmpty())
				{
					bool bOutputMessage = true;
					if (strnicmp(message, "server version", 14) == 0){
						CString strVer = message.Mid(14);
						strVer.Trim();
						strVer = strVer.Left(64); // truncate string to avoid misuse by servers in showing ads
						CServer* eserver = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						if (eserver){
							eserver->SetVersion(strVer);
							theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(eserver);
							theApp.emuledlg->serverwnd->UpdateMyInfo();
						}
						if (thePrefs.GetDebugServerTCPLevel() > 0)
							Debug("%s\n", message);
					}
					else if (strncmp(message, "ERROR", 5) == 0){
						CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						AddLogLine(true, _T("%s %s (%s:%u) - %s"), 
							GetResString(IDS_ERROR),
							pServer ? pServer->GetListName() : GetResString(IDS_PW_SERVER), 
							cur_server->GetAddress(), cur_server->GetPort(), message.Mid(5).Trim(_T(" :")));
						bOutputMessage = false;
					}
					else if (strncmp(message, "WARNING", 7) == 0){
						CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
						AddLogLine(true, _T("%s %s (%s:%u) - %s"), 
							GetResString(IDS_WARNING),
							pServer ? pServer->GetListName() : GetResString(IDS_PW_SERVER), 
							cur_server->GetAddress(), cur_server->GetPort(), message.Mid(7).Trim(_T(" :")));
						bOutputMessage = false;
					}

					if (message.Find("[emDynIP: ") != (-1) && message.Find("]") != (-1) && message.Find("[emDynIP: ") < message.Find("]")){
						CString dynip = message.Mid(message.Find("[emDynIP: ")+10,message.Find("]") - (message.Find("[emDynIP: ")+10));
						dynip.Trim(" ");
						if ( dynip.GetLength() && dynip.GetLength() < 51){
							CServer* eserver = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort());
							if (eserver){
								eserver->SetDynIP(dynip.GetBuffer());
								cur_server->SetDynIP(dynip.GetBuffer());
								theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(eserver);
								theApp.emuledlg->serverwnd->UpdateMyInfo();
							}
						}
					}
					if (bOutputMessage)
						theApp.emuledlg->AddServerMessageLine(message);

					message = strMessages.Tokenize("\r\n", iPos);
				}
				break;
			}
			case OP_IDCHANGE:{
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug("ServerMsg - OP_IDChange\n");
				if (size < sizeof(LoginAnswer_Struct)){
					throw GetResString(IDS_ERR_BADSERVERREPLY);
				}
				LoginAnswer_Struct* la = (LoginAnswer_Struct*)packet;

				// save TCP flags in 'cur_server'
				ASSERT( cur_server );
				//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
				uint32 ConnPort = 0;
				uint32 rport = cur_server->GetConnPort();
				//Morph End - added by AndCycle, aux Ports, by lugdunummaster
				if (cur_server){
					if (size >= sizeof(LoginAnswer_Struct)+4){
						if (thePrefs.GetDebugServerTCPLevel() > 0)
							Debug("  Flags=%08x\n", *((uint32*)(packet + sizeof(LoginAnswer_Struct))));
						cur_server->SetTCPFlags(*((uint32*)(packet + sizeof(LoginAnswer_Struct))));
						//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
						if (size >= sizeof(LoginAnswer_Struct)+8) {
							// aux port login : we should use the 'standard' port of this server to advertize to other clients
							ConnPort = *((uint32*)(packet + sizeof(LoginAnswer_Struct) + 4)) ;
							cur_server->SetPort(ConnPort) ;
						}
						//Morph End - added by AndCycle, aux Ports, by lugdunummaster
					}
					else
						cur_server->SetTCPFlags(0);

					// copy TCP flags into the server in the server list
					//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
					/*
					CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(), cur_server->GetPort());
					if (pServer)
						pServer->SetTCPFlags(cur_server->GetTCPFlags());
					*/
					CServer* pServer = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(), rport);
					if (pServer) {
 						pServer->SetTCPFlags(cur_server->GetTCPFlags());
						if (ConnPort) pServer->SetPort(ConnPort);
							theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(pServer);
							theApp.emuledlg->serverwnd->UpdateMyInfo();
					}
					//Morph End - added by AndCycle, aux Ports, by lugdunummaster
				}

				if (la->clientid == 0){
					uint8 state = thePrefs.GetSmartIdState();
					if ( state > 0 ){
						state++;
						if( state > 3 )
							thePrefs.SetSmartIdState(0);
						else
							thePrefs.SetSmartIdState(state);
					}
					break;
				}
				if( thePrefs.GetSmartIdCheck() ){
					if (!IsLowID(la->clientid))
						thePrefs.SetSmartIdState(1);
					else{
						uint8 state = thePrefs.GetSmartIdState();
						if ( state > 0 ){
							state++;
							if( state > 3 )
								thePrefs.SetSmartIdState(0);
							else
								thePrefs.SetSmartIdState(state);
							break;
						}
					}
				}
				//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
				if (thePrefs.GetLowIdRetried()){
					if (la->clientid < 16777216 ){
						SetConnectionState(CS_ERROR);
						AddLogLine(true,GetResString(IDS_LOWIDRETRYING),thePrefs.GetLowIdRetried());
						thePrefs.SetLowIdRetried();
						break;
					}
				}
				//MORPH END - Added by SiRoB, SLUGFILLER: lowIdRetry
				// we need to know our client's HighID when sending our shared files (done indirectly on SetConnectionState)
				serverconnect->clientid = la->clientid;

				if (connectionstate != CS_CONNECTED) {
					SetConnectionState(CS_CONNECTED);
					theApp.OnlineSig();       // Added By Bouc7 
				}
				serverconnect->SetClientID(la->clientid);
				AddLogLine(false,GetResString(IDS_NEWCLIENTID),la->clientid);

				theApp.downloadqueue->ResetLocalServerRequests();
				break;
			}
			case OP_SEARCHRESULT:{
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug("ServerMsg - OP_SearchResult\n");
				CServer* cur_srv = (serverconnect) ? serverconnect->GetCurrentServer() : NULL;
				bool bMoreResultsAvailable;
				uint16 uSearchResults = theApp.searchlist->ProcessSearchanswer(packet, size, cur_srv ? cur_srv->GetIP():0, cur_srv ? cur_srv->GetPort() : 0, &bMoreResultsAvailable);
				theApp.emuledlg->searchwnd->LocalSearchEnd(uSearchResults, bMoreResultsAvailable);
				break;
			}
			case OP_FOUNDSOURCES:{
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug("ServerMsg - OP_FoundSources; Sources=%u  %s\n", (UINT)(uchar)packet[16], DbgGetFileInfo((uchar*)packet));
				CSafeMemFile sources((BYTE*)packet,size);
				uchar fileid[16];
				sources.ReadHash16(fileid);
				if (CPartFile* file = theApp.downloadqueue->GetFileByID(fileid))
					file->AddSources(&sources,cur_server->GetIP(), cur_server->GetPort());
				break;
			}
			case OP_SERVERSTATUS:{
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug("ServerMsg - OP_ServerStatus\n");
				// FIXME some statuspackets have a different size -> why? structur?
				if (size < 8)
					break;//throw "Invalid status packet";
				uint32 cur_user = PeekUInt32(packet);
				uint32 cur_files = PeekUInt32(packet+4);
				CServer* update = theApp.serverlist->GetServerByAddress( cur_server->GetAddress(), cur_server->GetPort() );
				if (update){
					update->SetUserCount(cur_user); 
					update->SetFileCount(cur_files);
					theApp.emuledlg->ShowUserCount();
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer( update );
					theApp.emuledlg->serverwnd->UpdateMyInfo();
				}
				if (thePrefs.GetDebugServerTCPLevel() > 0){
					if (size > 8){
						Debug("*** NOTE: OP_ServerStatus: ***AddData: %u bytes\n", size - 8);
						DebugHexDump((uint8*)packet + 8, size - 8);
					}
				}
				break;
			}
			case OP_SERVERIDENT:{
				// OP_SERVERIDENT - this is sent by the server only if we send a OP_GETSERVERLIST
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug("ServerMsg - OP_ServerIdent\n");
				if (size<16+4+2+4) {
					if (thePrefs.GetVerbose())
						AddDebugLogLine(false,GetResString(IDS_ERR_KNOWNSERVERINFOREC)); 
					break;// throw "Invalid server info received"; 
				} 


				CString strInfo;
				CSafeMemFile data((BYTE*)packet, size);
				
				uint8 aucHash[16];
				data.ReadHash16(aucHash);
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					strInfo.AppendFormat("Hash=%s (%s)", md4str(aucHash), DbgGetHashTypeString(aucHash));
				uint32 nServerIP = data.ReadUInt32();
				uint16 nServerPort = data.ReadUInt16();
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					strInfo.AppendFormat("  IP=%s:%u", inet_ntoa(*(in_addr*)&nServerIP), nServerPort);
				UINT nTags = data.ReadUInt32();
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					strInfo.AppendFormat("  Tags=%u", nTags);

				CString strName;
				CString strDescription;
				for (UINT i = 0; i < nTags; i++){
					CTag tag(&data);
					if (tag.tag.specialtag == ST_SERVERNAME){
						if (tag.tag.type == 2){
							strName = tag.tag.stringvalue;
							if (thePrefs.GetDebugServerTCPLevel() > 0)
								strInfo.AppendFormat("  Name=%s", strName);
						}
					}
					else if (tag.tag.specialtag == ST_DESCRIPTION){
						if (tag.tag.type == 2){
							strDescription = tag.tag.stringvalue;
							if (thePrefs.GetDebugServerTCPLevel() > 0)
								strInfo.AppendFormat("  Desc=%s", strDescription);
						}
					}
					else if (thePrefs.GetDebugServerTCPLevel() > 0)
						strInfo.AppendFormat("  ***UnkTag: 0x%02x=%u", tag.tag.specialtag, tag.tag.intvalue);
				}
				if (thePrefs.GetDebugServerTCPLevel() > 0){
					strInfo += _T('\n');
					Debug(strInfo);

					UINT uAddData = data.GetLength() - data.GetPosition();
					if (uAddData > 0){
						Debug("*** NOTE: OP_ServerIdent: ***AddData: %u bytes\n", uAddData);
						DebugHexDump((uint8*)packet + data.GetPosition(), uAddData);
					}
				}

				ASSERT( cur_server );
				if (cur_server){
					CServer* update = theApp.serverlist->GetServerByAddress(cur_server->GetAddress(),cur_server->GetPort()); 
					ASSERT( update );
					if (update){
						update->SetListName(strName);
						update->SetDescription(strDescription);
						if (((uint32*)aucHash)[0] == 0x2A2A2A2A){
							const CString& rstrVersion = update->GetVersion();
							if (!rstrVersion.IsEmpty())
								update->SetVersion(_T("eFarm ") + rstrVersion);
							else
								update->SetVersion(_T("eFarm"));
						}
						theApp.emuledlg->ShowConnectionState(); 
						theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(update); 
						theApp.emuledlg->serverwnd->UpdateMyInfo();
					}
				}
				break;
			} 
			// tecxx 1609 2002 - add server's serverlist to own serverlist
			case OP_SERVERLIST:{
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug("ServerMsg - OP_ServerList\n");
				try{
					CSafeMemFile servers((BYTE*)packet,size);
					UINT count = servers.ReadUInt8();
					// check if packet is valid
					if (1 + count*(4+2) > size)
						count = 0;
					int addcount = 0;
					while(count)
					{
						uint32 ip = servers.ReadUInt32();
						uint16 port = servers.ReadUInt16();
						in_addr host;
						host.S_un.S_addr = ip;
						CServer* srv = new CServer(port, inet_ntoa(host));
						srv->SetListName(srv->GetFullIP());
						if (!theApp.emuledlg->serverwnd->serverlistctrl.AddServer(srv, true))
							delete srv;
						else
							addcount++;
						count--;
					}
					if (addcount)
						AddLogLine(false,GetResString(IDS_NEWSERVERS), addcount);
					if (thePrefs.GetDebugServerTCPLevel() > 0){
						UINT uAddData = servers.GetLength() - servers.GetPosition();
						if (uAddData > 0){
							Debug("*** NOTE: OP_ServerList: ***AddData: %u bytes\n", uAddData);
							DebugHexDump((uint8*)packet + servers.GetPosition(), uAddData);
						}
					}
				}
				catch(CFileException* error){
					if (thePrefs.GetVerbose())
						AddDebugLogLine(false,GetResString(IDS_ERR_BADSERVERLISTRECEIVED));
					error->Delete();
				}
				break;
			}
			case OP_CALLBACKREQUESTED:{
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug("ServerMsg - OP_CallbackRequested\n");
				if (size == 6){
					uint32 dwIP = PeekUInt32(packet);
					uint16 nPort = PeekUInt16(packet+4);
					CUpDownClient* client = theApp.clientlist->FindClientByIP(dwIP,nPort);
					if (client)
						client->TryToConnect();
					else{
						client = new CUpDownClient(0,nPort,dwIP,0,0,true);
						theApp.clientlist->AddClient(client);
						client->TryToConnect();
					}
				}
				break;
			}
			case OP_CALLBACK_FAIL:{
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug("ServerMsg - OP_Callback_Fail %s\n", GetHexDump((uint8*)packet, size));
				break;
			}
			case OP_REJECT:{
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug("ServerMsg - OP_Reject %s\n", GetHexDump((uint8*)packet, size));
				// this could happen if we send a command with the wrong protocol (e.g. sending a compressed packet to
				// a server which does not support that protocol).
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false, _T("Server rejected last command"));
				break;
			}
			default:
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug("***NOTE: ServerMsg - Unknown message; opcode=0x%02x  %s\n", opcode, GetHexDump((uint8*)packet, size));
				;
		}

		return true;
	}
	catch(CFileException* error)
	{
		if (thePrefs.GetVerbose())
		{
			char szError[MAX_CFEXP_ERRORMSG];
			error->m_strFileName = "server packet";
			error->GetErrorMessage(szError,sizeof(szError));
			AddDebugLogLine(false,GetResString(IDS_ERR_PACKAGEHANDLING),szError);
		}
		error->Delete();
		ASSERT(0);
		if (opcode==OP_SEARCHRESULT || opcode==OP_FOUNDSOURCES)
			return true;
	}
	catch(CMemoryException* error)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false,GetResString(IDS_ERR_PACKAGEHANDLING),_T("CMemoryException"));
		error->Delete();
		ASSERT(0);
		if (opcode==OP_SEARCHRESULT || opcode==OP_FOUNDSOURCES)
			return true;
	}
	catch(CString error)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false,GetResString(IDS_ERR_PACKAGEHANDLING),error.GetBuffer());
		ASSERT(0);
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false,GetResString(IDS_ERR_PACKAGEHANDLING),_T("Unknown exception"));
		ASSERT(0);
	}

	SetConnectionState(CS_DISCONNECTED);
	return false;
}

void CServerSocket::ConnectToServer(CServer* server){
	if (cur_server){
		ASSERT(0);
		delete cur_server;
		cur_server = NULL;
	}

	cur_server = new CServer(server);
	AddLogLine(false,GetResString(IDS_CONNECTINGTO),cur_server->GetListName(),cur_server->GetFullIP(),cur_server->GetPort());

	if (thePrefs.IsProxyASCWOP() )
	{
		if (thePrefs.GetProxy().UseProxy == true)
		{
			thePrefs.SetProxyASCWOP(true);
			thePrefs.SetUseProxy(false);
			AddLogLine(false,GetResString(IDS_ASCWOP_PROXYSUPPORT)+GetResString(IDS_DISABLED));
		}
		else
			thePrefs.SetProxyASCWOP(false);
	}

	SetConnectionState(CS_CONNECTING);
	//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
	/*
	if (!this->Connect(server->GetAddress(),server->GetPort())){
	*/
	if (!this->Connect(server->GetAddress(),server->GetConnPort())){
	//Morph End - added by AndCycle, aux Ports, by lugdunummaster
		int error = this->GetLastError();
		if ( error != WSAEWOULDBLOCK){
			AddLogLine(false,GetResString(IDS_ERR_CONNECTIONERROR),cur_server->GetListName(),cur_server->GetFullIP(),cur_server->GetPort(), GetErrorMessage(error, 1)); 
			SetConnectionState(CS_FATALERROR);
			return;
		}
	}
	info=server->GetListName();
	SetConnectionState(CS_CONNECTING);
}

void CServerSocket::OnError(int nErrorCode)
{
	SetConnectionState(CS_DISCONNECTED);
	if (thePrefs.GetVerbose())
		AddDebugLogLine(false,GetResString(IDS_ERR_SOCKET),cur_server->GetListName(),cur_server->GetFullIP(),cur_server->GetPort(), GetErrorMessage(nErrorCode, 1));
}

bool CServerSocket::PacketReceived(Packet* packet)
{
	try
	{
		theApp.downloadqueue->AddDownDataOverheadServer(packet->size);
		if (packet->prot == OP_PACKEDPROT)
		{
			uint32 uComprSize = packet->size;
			if (!packet->UnPackPacket(250000)){
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false,_T("Failed to decompress server TCP packet: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->prot : 0, packet ? packet->opcode : 0, packet ? packet->size : 0);
				return true;
			}
			packet->prot = OP_EDONKEYPROT;
			if (thePrefs.GetDebugServerTCPLevel() > 1)
				Debug("Received compressed server TCP packet; opcode=0x%02x  size=%u  uncompr size=%u\n", packet->opcode, uComprSize, packet->size);
		}

		if (packet->prot == OP_EDONKEYPROT)
		{
			ProcessPacket(packet->pBuffer,packet->size,packet->opcode);
		}
		else
		{
			if (thePrefs.GetVerbose())
				AddDebugLogLine(false,_T("Received server TCP packet with unknown protocol: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->prot : 0, packet ? packet->opcode : 0, packet ? packet->size : 0);
		}
	}
	catch(...)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false,_T("Error: Unhandled exception while processing server TCP packet: protocol=0x%02x  opcode=0x%02x  size=%u"), packet ? packet->prot : 0, packet ? packet->opcode : 0, packet ? packet->size : 0);
		ASSERT(0);
		return false;
	}
	return true;
}

void CServerSocket::OnClose(int nErrorCode){
	CEMSocket::OnClose(0);
	if (connectionstate == CS_WAITFORLOGIN){	 	
		SetConnectionState(CS_SERVERFULL);
	}
	else if (connectionstate == CS_CONNECTED){
		SetConnectionState(CS_DISCONNECTED);		
	}
	else{
		SetConnectionState(CS_NOTCONNECTED);
	}
	serverconnect->DestroySocket(this);	
}

void CServerSocket::SetConnectionState(sint8 newstate){
	connectionstate = newstate;
	if (newstate < 1){
		serverconnect->ConnectionFailed(this);
	}
	else if (newstate == CS_CONNECTED || newstate == CS_WAITFORLOGIN){
		if (serverconnect)
			serverconnect->ConnectionEstablished(this);
	}
}

//MORPH START - Changed by SiRoB,	zz Upload System
void CServerSocket::SendPacket(Packet* packet, bool delpacket, bool controlpacket, uint32 actualPayloadSize){
	m_dwLastTransmission = GetTickCount();
	CEMSocket::SendPacket(packet, delpacket, controlpacket, actualPayloadSize);
}
//MORPH END  - Changed by SiRoB,	zz Upload System
