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
#pragma once
#include "Loggable.h"
#include "UploadBandwidthThrottler.h" // ZZ:UploadBandWithThrottler (UDP)

class Packet;

#pragma pack(1)
struct UDPPack
{
	Packet* packet;
	uint32 dwIP;
	uint16 nPort;
	uint32 dwTime;
	//uint16 nPriority; We could add a priority system here to force some packets.
};
#pragma pack()

class CClientUDPSocket : public CAsyncSocket, public CLoggable, public ThrottledControlSocket // ZZ:UploadBandWithThrottler (UDP)
{
public:
	CClientUDPSocket();
	virtual ~CClientUDPSocket();

	bool	Create();
	bool	SendPacket(Packet* packet, uint32 dwIP, uint16 nPort);
    SocketSentBytes  Send(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket); // ZZ:UploadBandWithThrottler (UDP)

protected:
	bool	ProcessPacket(BYTE* packet, uint16 size, uint8 opcode, uint32 ip, uint16 port);
	
	virtual void	OnSend(int nErrorCode);	
	virtual void	OnReceive(int nErrorCode);

private:
	int		SendTo(char* lpBuf,int nBufLen,uint32 dwIP, uint16 nPort);
    bool	IsBusy() const { return m_bWouldBlock; }
	bool	m_bWouldBlock;

	CTypedPtrList<CPtrList, UDPPack*> controlpacket_queue;

    CCriticalSection sendLocker; // ZZ:UploadBandWithThrottler (UDP)
};
