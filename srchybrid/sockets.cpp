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
#include "packets.h"
#include "sockets.h"
#include "emuleDlg.h"
#include "SearchDlg.h"
#include "opcodes.h"
#include "searchlist.h"
#include <time.h>
#include <afxmt.h>
#include "UDPSocket.h"
#include "Exceptions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CServerConnect

void CServerConnect::TryAnotherConnectionrequest(){
	if ( connectionattemps.GetCount()<((app_prefs->IsSafeServerConnectEnabled()) ? 1 : 2) ) {

		CServer*  next_server = used_list->GetNextServer();

		if (!next_server)
		{
			if (connectionattemps.GetCount()==0){
				AddLogLine(true,GetResString(IDS_OUTOFSERVERS));
				ConnectToAnyServer(lastStartAt);
			}
			return;
		}

		// Barry - Only auto-connect to static server option
		if (theApp.glob_prefs->AutoConnectStaticOnly())
		{
			if (next_server->IsStaticMember())
                ConnectToServer(next_server,true);
		}
		else
			ConnectToServer(next_server,true);
	}
}

void CServerConnect::ConnectToAnyServer(uint32 startAt,bool prioSort,bool isAuto){
	lastStartAt=startAt;
	StopConnectionTry();
	Disconnect();
	theApp.emuledlg->ShowConnectionState();
	connecting = true;
	singleconnecting = false;

	// Barry - Only auto-connect to static server option
	if (theApp.glob_prefs->AutoConnectStaticOnly() && isAuto)
	{
		bool anystatic = false;
		CServer *next_server; 
		used_list->SetServerPosition( startAt );
		while ((next_server = used_list->GetNextServer()) != NULL)
		{
			if (next_server->IsStaticMember())
			{
				anystatic = true;
				break;
			}
		}
		if (!anystatic)
		{
			connecting = false;
			AddLogLine(true,GetResString(IDS_ERR_NOVALIDSERVERSFOUND));
			return;
		}
	}

	used_list->SetServerPosition( startAt );
	if( theApp.glob_prefs->Score() && prioSort ) used_list->Sort();

	if (used_list->GetServerCount()==0 ){
		connecting = false;
		AddLogLine(true,GetResString(IDS_ERR_NOVALIDSERVERSFOUND));
		return;
	}
	theApp.listensocket->Process();

	TryAnotherConnectionrequest();
}

void CServerConnect::ConnectToServer(CServer* server, bool multiconnect){
	
	if (!multiconnect) {
		StopConnectionTry();
		Disconnect();
	}
	connecting = true;
	singleconnecting = !multiconnect;

	CServerSocket* newsocket = new CServerSocket(this);
	m_lstOpenSockets.AddTail((void*&)newsocket);
	newsocket->Create(0,SOCK_STREAM,FD_READ|FD_WRITE|FD_CLOSE|FD_CONNECT,NULL);
	newsocket->ConnectToServer(server);
	ULONG x=GetTickCount();
	connectionattemps.SetAt(x,newsocket);
}

void CServerConnect::StopConnectionTry(){
	connectionattemps.RemoveAll();
	connecting = false;
	singleconnecting = false;

	if (m_idRetryTimer) 
	{ 
		KillTimer(NULL, m_idRetryTimer); 
		m_idRetryTimer= 0; 
	} 

	// close all currenty opened sockets except the one which is connected to our current server
	for( POSITION pos = m_lstOpenSockets.GetHeadPosition(); pos != NULL; )
	{
		CServerSocket* pSck = (CServerSocket*)m_lstOpenSockets.GetNext(pos);
		if (pSck == connectedsocket)		// don't destroy socket which is connected to server
			continue;
		if (pSck->m_bIsDeleting == false)	// don't destroy socket if it is going to destroy itself later on
			DestroySocket(pSck);
	}
	//MORPH START - Added by SiRoB, SLUGFILLER: lowIdRetry
	theApp.glob_prefs->ResetLowIdRetried();
	//MORPH END   - Added by SiRoB, SLUGFILLER: lowIdRetry
}

void CServerConnect::ConnectionEstablished(CServerSocket* sender){
	if (theApp.glob_prefs->IsProxyASCWOP())
	{
		theApp.glob_prefs->SetUseProxy(true);
		AddLogLine(false,GetResString(IDS_ASCWOP_PROXYSUPPORT)+GetResString(IDS_ENABLED));
	}

	if (connecting == false)
	{
		// we are already connected to another server
		DestroySocket(sender);
		return;
	}
	
	InitLocalIP();
	if (sender->GetConnectionState() == CS_WAITFORLOGIN){
		AddLogLine(false,GetResString(IDS_CONNECTEDTOREQ),sender->cur_server->GetListName(),sender->cur_server->GetFullIP(),sender->cur_server->GetPort());
		//send loginpacket
		CServer* update = theApp.serverlist->GetServerByAddress( sender->cur_server->GetAddress(), sender->cur_server->GetPort() );
		if (update){
			update->ResetFailedCount();
			theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer( update );
		}

		CSafeMemFile data(256);
		data.Write(theApp.glob_prefs->GetUserHash(),16);
		uint32 clientid = GetClientID();
		data.Write(&clientid,4);
		uint16 port = app_prefs->GetPort();
		data.Write(&port,2);

		uint32 tagcount = 4;
		data.Write(&tagcount,4);
		CTag tagName(CT_NAME,app_prefs->GetUserNick());
		tagName.WriteTagToFile(&data);
		CTag tagVersion(CT_VERSION,EDONKEYVERSION);
		tagVersion.WriteTagToFile(&data);
		CTag tagPort(CT_PORT,app_prefs->GetPort());
		tagPort.WriteTagToFile(&data);

		CTag tagFlags(0x20,0x00000001);
		tagFlags.WriteTagToFile(&data);

		Packet* packet = new Packet(&data);
		packet->opcode = OP_LOGINREQUEST;
		if (theApp.glob_prefs->GetDebugServerTCP())
			Debug(">>> Sending OP__LoginRequest\n");
		theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
		SendPacket(packet,true,sender);
	}
	else if (sender->GetConnectionState() == CS_CONNECTED){	
		theApp.stat_reconnects++;
		theApp.stat_serverConnectTime=GetTickCount();
		connected = true;
		AddLogLine(true,GetResString(IDS_CONNECTEDTO),sender->cur_server->GetListName());
		theApp.emuledlg->ShowConnectionState();
		connectedsocket = sender;
		StopConnectionTry();
		theApp.sharedfiles->ClearED2KPublishInfo();
		theApp.sharedfiles->SendListToServer();
		theApp.emuledlg->serverwnd.serverlistctrl.RemoveDeadServer();
		// tecxx 1609 2002 - serverlist update
		if (theApp.glob_prefs->AddServersFromServer())
		{
			Packet* packet = new Packet(OP_GETSERVERLIST,0);
			if (theApp.glob_prefs->GetDebugServerTCP())
				Debug(">>> Sending OP__GetServerList\n");
			theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
			SendPacket(packet,true);
		}
	}
}
bool CServerConnect::SendPacket(Packet* packet,bool delpacket, CServerSocket* to){
	if (!to){
		if (connected){
			connectedsocket->SendPacket(packet,delpacket,true);
		}
		else{
			if (delpacket)
				delete packet;
			return false;
		}
	}
	else{
		to->SendPacket(packet,delpacket,true);
	}
	return true;
}

bool CServerConnect::SendUDPPacket(Packet* packet,CServer* host,bool delpacket){
	if (theApp.IsConnected()){
		if (udpsocket != NULL)
			udpsocket->SendPacket(packet,host);
	}
	if (delpacket)
		delete packet;
	return true;
}

void CServerConnect::ConnectionFailed(CServerSocket* sender){
	if (connecting == false && sender != connectedsocket)
	{
		// just return, cleanup is done by the socket itself
		return;
	}
	//messages
	CServer* update;
	switch (sender->GetConnectionState()){
		case CS_FATALERROR:
			AddLogLine(true,GetResString(IDS_ERR_FATAL));
			break;
		case CS_DISCONNECTED:{
			theApp.sharedfiles->ClearED2KPublishInfo();
			AddLogLine(true,GetResString(IDS_ERR_LOSTC),sender->cur_server->GetListName(),sender->cur_server->GetFullIP(),sender->cur_server->GetPort());
			break;
		}
		case CS_SERVERDEAD:
			AddLogLine(true,GetResString(IDS_ERR_DEAD),sender->cur_server->GetListName(),sender->cur_server->GetFullIP(),sender->cur_server->GetPort()); //<<--
			update = theApp.serverlist->GetServerByAddress( sender->cur_server->GetAddress(), sender->cur_server->GetPort() );
			if(update){
				update->AddFailedCount();
				theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer( update );
			}
			break;
		case CS_ERROR:
			break;
		case CS_SERVERFULL:
			AddLogLine(true,GetResString(IDS_ERR_FULL),sender->cur_server->GetListName(),sender->cur_server->GetFullIP(),sender->cur_server->GetPort());
			break;
		case CS_NOTCONNECTED:; 
			break; 
	}

	// IMPORTANT: mark this socket not to be deleted in StopConnectionTry(), 
	// because it will delete itself after this function!
	sender->m_bIsDeleting = true;

	switch (sender->GetConnectionState()){
		case CS_FATALERROR:{
			bool autoretry= !singleconnecting;
			StopConnectionTry();
			if ((app_prefs->Reconnect()) && (autoretry) && (!m_idRetryTimer)){ 
				AddLogLine(false,GetResString(IDS_RECONNECT), CS_RETRYCONNECTTIME); 
				VERIFY( (m_idRetryTimer= SetTimer(NULL, 0, 1000*CS_RETRYCONNECTTIME, RetryConnectTimer)) );
				if (!m_idRetryTimer)
					AddDebugLogLine(true,_T("Failed to create 'server connect retry' timer - %s"),GetErrorMessage(GetLastError()));
			}
			break;
		}
		case CS_DISCONNECTED:{
			theApp.sharedfiles->ClearED2KPublishInfo();
			connected = false;
			if (connectedsocket) 
				connectedsocket->Close();
			connectedsocket = NULL;
			theApp.emuledlg->searchwnd->OnBnClickedCancels();
			if (app_prefs->Reconnect() && !connecting){
				ConnectToAnyServer();		
			}
			if (theApp.glob_prefs->GetNotifierPopOnImportantError()) {
				theApp.emuledlg->ShowNotifier(GetResString(IDS_CONNECTIONLOST), TBN_IMPORTANTEVENT, false);
			}
			break;
		}
		case CS_ERROR:
		case CS_NOTCONNECTED:{
			if (!connecting)
				break;
		}
		case CS_SERVERDEAD:
		case CS_SERVERFULL:{
			if (!connecting)
				break;
			if (singleconnecting){
				StopConnectionTry();
				break;
			}

			DWORD tmpkey;
			CServerSocket* tmpsock;
			POSITION pos = connectionattemps.GetStartPosition();
			while (pos){
				connectionattemps.GetNextAssoc(pos,tmpkey,tmpsock);
				if (tmpsock==sender) {
					connectionattemps.RemoveKey(tmpkey);
					break;
				}
			}			
			TryAnotherConnectionrequest();
		}
	}
	theApp.emuledlg->ShowConnectionState();
}

// 09/28/02, by zegzav
VOID CALLBACK CServerConnect::RetryConnectTimer(HWND hWnd, UINT nMsg, UINT nId, DWORD dwTime) 
{ 
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	try
	{
		CServerConnect *_this= theApp.serverconnect; 
		ASSERT(_this); 
		_this->StopConnectionTry();
		if (_this->IsConnected())
			return; 

		_this->ConnectToAnyServer();
	}
	CATCH_DFLT_EXCEPTIONS("CServerConnect::RetryConnectTimer")
}

void CServerConnect::CheckForTimeout()
{ 
	//DWORD maxage=GetTickCount() - CONSERVTIMEOUT;	// this gives us problems when TickCount < TIMEOUT (may occure right after system start)
	DWORD dwCurTick = GetTickCount();
	DWORD tmpkey;
	CServerSocket* tmpsock;
	POSITION pos = connectionattemps.GetStartPosition();
	while (pos){
		connectionattemps.GetNextAssoc(pos,tmpkey,tmpsock);
		if (!tmpsock) {
			AddDebugLogLine(false,"Error: Socket invalid at timeoutcheck" );
			connectionattemps.RemoveKey(tmpkey);
			return;
		}

		//if (tmpkey<=maxage) {
		if (dwCurTick - tmpkey > CONSERVTIMEOUT){
			if (!tmpsock->info && !tmpsock->cur_server) //MORPH - Added by SiRoB, Temporary Patch while finding why info & cur_server could be empty and crash emule
				theApp.emuledlg->AddLogLine(false,GetResString(IDS_ERR_CONTIMEOUT),tmpsock->info, tmpsock->cur_server->GetFullIP(), tmpsock->cur_server->GetPort() );
			connectionattemps.RemoveKey(tmpkey);
			TryAnotherConnectionrequest();
			DestroySocket(tmpsock);
		}
	}
}

bool CServerConnect::Disconnect(){
	if (connected && connectedsocket){
		theApp.sharedfiles->ClearED2KPublishInfo();
		DestroySocket(connectedsocket);
		connectedsocket = NULL;
		connected = false;
		theApp.emuledlg->ShowConnectionState();
		theApp.emuledlg->AddServerMessageLine(_T(""));
		theApp.emuledlg->AddServerMessageLine(_T(""));
		theApp.emuledlg->AddServerMessageLine(_T(""));
		theApp.emuledlg->AddServerMessageLine(_T(""));
		theApp.stat_serverConnectTime=0;
		// -khaos--+++> Tell our total server duration thinkymajig to update...
		theApp.emuledlg->statisticswnd.Add2TotalServerDuration();
		// <-----khaos-
		return true;
	}
	else
		return false;
}

CServerConnect::CServerConnect(CServerList* in_serverlist, CPreferences* in_prefs){
	connectedsocket = NULL;
	app_prefs = in_prefs;
	used_list = in_serverlist;
	max_simcons = (app_prefs->IsSafeServerConnectEnabled()) ? 1 : 2;
	connecting = false;
	connected = false;
	clientid = 0;
	singleconnecting = false;
	if (in_prefs->GetServerUDPPort() != 0){
	udpsocket = new CUDPSocket(this); // initalize socket for udp packets
		if (!udpsocket->Create()){
			delete udpsocket;
			udpsocket = NULL;
		}
	}
	else
		udpsocket = NULL;
	m_idRetryTimer= 0;
	lastStartAt=0;
	InitLocalIP();
}

CServerConnect::~CServerConnect(){
	// stop all connections
	StopConnectionTry();
	// close connected socket, if any
	DestroySocket(connectedsocket);
	connectedsocket = NULL;
	// close udp socket
	if (udpsocket){
		udpsocket->Close();
		delete udpsocket;
	}
}

CServer* CServerConnect::GetCurrentServer(){
	if (IsConnected() && connectedsocket)
		return connectedsocket->cur_server;
	return NULL;
}

void CServerConnect::SetClientID(uint32 newid){
	clientid = newid;
	theApp.emuledlg->ShowConnectionState();
}

void CServerConnect::DestroySocket(CServerSocket* pSck){
	if (pSck == NULL)
		return;
	// remove socket from list of opened sockets
	for( POSITION pos = m_lstOpenSockets.GetHeadPosition(); pos != NULL; )
	{
		POSITION posDel = pos;
		CServerSocket* pTestSck = (CServerSocket*)m_lstOpenSockets.GetNext(pos);
		if (pTestSck == pSck)
		{
			m_lstOpenSockets.RemoveAt(posDel);
			break;
		}
	}
	if (pSck->m_SocketData.hSocket != INVALID_SOCKET){ // deadlake PROXYSUPPORT - changed to AsyncSocketEx
		pSck->AsyncSelect(0);
		pSck->Close();
	}

	delete pSck;
}

bool CServerConnect::IsLocalServer(uint32 dwIP, uint16 nPort){
	if (IsConnected()){
		if (connectedsocket->cur_server->GetIP() == dwIP && connectedsocket->cur_server->GetPort() == nPort)
			return true;
	}
	return false;
}

void CServerConnect::InitLocalIP(){
	m_nLocalIP = 0;
	// Don't use 'gethostbyname(NULL)'. The winsock DLL may be replaced by a DLL from a third party
	// which is not fully compatible to the original winsock DLL. ppl reported crash with SCORSOCK.DLL
	// when using 'gethostbyname(NULL)'.
	try{
		char szHost[256];
		if (gethostname(szHost, sizeof szHost) == 0){
			hostent* pHostEnt = gethostbyname(szHost);
			if (pHostEnt != NULL && pHostEnt->h_length == 4 && pHostEnt->h_addr_list[0] != NULL)
				m_nLocalIP = *((uint32*)pHostEnt->h_addr_list[0]);
		}
	}
	catch(...){
		// at least two ppl reported crashs when using 'gethostbyname' with third party winsock DLLs
		AddDebugLogLine(false, _T("Unknown exception in CServerConnect::InitLocalIP"));
	}
}

void CServerConnect::KeepConnectionAlive()
{
	DWORD dwServerKeepAliveTimeout = theApp.glob_prefs->GetServerKeepAliveTimeout();
	if (dwServerKeepAliveTimeout && connected && connectedsocket && connectedsocket->connectionstate == CS_CONNECTED &&
		GetTickCount() - connectedsocket->GetLastTransmission() >= dwServerKeepAliveTimeout)
	{
		// "Ping" the server if the TCP connection was not used for the specified interval with 
		// an empty publish files packet -> recommended by lugdunummaster himself!
		CSafeMemFile files(4);
		uint32 nFiles = 0;
		files.Write(&nFiles,4);
		Packet* packet = new Packet(&files);
		packet->opcode = OP_OFFERFILES;
		theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
		AddDebugLogLine(false, _T("Refreshing server connection"));
		if (theApp.glob_prefs->GetDebugServerTCP())
			Debug(">>> Sending OP__OfferFiles(KeepAlive) to server\n");
		connectedsocket->SendPacket(packet,true);
	}
}
