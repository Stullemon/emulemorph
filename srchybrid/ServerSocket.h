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

// Client to Server communication

#pragma once

#include "types.h"
#include "server.h"
#include "packets.h"
#include "Preferences.h"
#include "sockets.h"
#include "preferences.h"
#include "emsocket.h"
#include "AsyncProxySocketLayer.h" // deadlake PROXYSUPPORT
#include "ServerWnd.h"
#include "StatisticsDlg.h"

class CServerSocket : public CEMSocket
{
	friend class CServerConnect;
public:
	CServerSocket(CServerConnect* in_serverconnect);
	~CServerSocket();

	void	ConnectToServer(CServer* server);
	sint8	GetConnectionState()	{return connectionstate;} 
	DWORD	GetLastTransmission() const { return m_dwLastTransmission; }
	bool	SendPacket(Packet* packet, bool delpacket = true, bool controlpacket = true);

	CString info;
protected:
	void	OnClose(int nErrorCode);
	void	OnConnect(int nErrorCode);
	void	OnReceive(int nErrorCode);
	void	OnError(int nErrorCode);
	void	PacketReceived(Packet* packet);
private:
	bool	ProcessPacket(char* packet, int32 size,int8 opcode);
	void	SetConnectionState(sint8 newstate);
	CServerConnect*	serverconnect; 
	sint8	connectionstate;
	CServer*	cur_server;
	bool	headercomplete;
	int32	sizetoget;
	int32	sizereceived;
	char*	rbuffer;
	bool	m_bIsDeleting;	// true: socket is already in deletion phase, don't destroy it in ::StopConnectionTry
	DWORD	m_dwLastTransmission;
};