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
#pragma once
#include "../utils/UInt128.h"
#include "../io/ByteIO.h"

class CKnownFile;
class CSafeMemFile;


// Thread messages recognized by an UDP Socket Listener thread
#define WM_KADEMLIA_FIREWALLED_ACK		(WM_USER+0x200)

// params for WM_KADEMLIA_FIREWALLED_ACK (to be deleted by receiving thread)
typedef struct
{
	uint32 ip;
	uint16 port;
} KADEMLIAFIREWALLEDACK;


////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CSearch;

class CKademliaUDPListener
{
	friend class CSearch;

public:
	void bootstrap(LPCSTR ip, uint16 port);
	void bootstrap(uint32 ip, uint16 port);
	void firewalledCheck(uint32 ip, uint16 port);
	void sendMyDetails(byte opcode, uint32 ip, uint16 port);
	void publishPacket(uint32 ip, uint16 port, const CUInt128& targetID, const CUInt128& contactID, const TagList& tags);
	void sendNullPacket(byte opcode, uint32 ip, uint16 port);
	virtual void processPacket(const byte* data, uint32 lenData, uint32 ip, uint16 port);
	void sendPacket(const byte* data, uint32 lenData, uint32 destinationHost, uint16 destinationPort);
	void sendPacket(const byte *data, uint32 lenData, byte opcode, uint32 destinationHost, uint16 destinationPort);
	void sendPacket(CSafeMemFile* data, byte opcode, uint32 destinationHost, uint16 destinationPort);

private:
	void addContact (const byte* data, uint32 lenData, uint32 ip, uint16 port, uint16 tport = 0);
	void addContacts(const byte* data, uint32 lenData, uint16 numContacts);

	void processBootstrapRequest		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processBootstrapResponse		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processHelloRequest			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processHelloResponse			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processKademliaRequest			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processKademliaResponse		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processSearchRequest			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processSearchResponse			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processPublishRequest			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processPublishResponse			(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processFirewalledRequest		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processFirewalledResponse		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
	void processFirewalledResponse2		(const byte* packetData, uint32 lenPacket, uint32 ip, uint16 port);
};

} // End namespace