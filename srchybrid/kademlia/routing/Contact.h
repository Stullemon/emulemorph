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


This work is based on the java implementation of the Kademlia protocol.
Kademlia: Peer-to-peer routing based on the XOR metric
Copyright (C) 2002  Petar Maymounkov [petar@post.harvard.edu]
http://kademlia.scs.cs.nyu.edu
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

#include "../../stdafx.h"
#include "../../Types.h"
#include "../utils/UInt128.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CRoutingZone;
class CRoutingBin;

class CContact
{
	friend class CRoutingZone;
	friend class CRoutingBin;

public:

	~CContact();
	CContact();
	CContact(const CUInt128 &clientID, const uint32 ip, const uint16 udpPort, const uint16 tcpPort, const byte type);
	CContact(const CUInt128 &clientID, const uint32 ip, const uint16 udpPort, const uint16 tcpPort, const byte type, const CUInt128 &target);
//	CContact(const CUInt128 &clientID, const uint32 ip, const uint16 udpPort, const byte type, const uint16 tcpPort);

	void getClientID(CUInt128 *id);
	void getClientID(CString *id);
	void setClientID(const CUInt128 &clientID);

	void getDistance(CUInt128 *distance);
	void getDistance(CString *distance);

	uint32 getIPAddress(void);
	void getIPAddress(CString *ip);
	void setIPAddress(const uint32 ip);

	uint16 getTCPPort(void);
	void getTCPPort(CString *port);
	void setTCPPort(const uint16 port);

	uint16 getUDPPort(void);
	void getUDPPort(CString *port);
	void setUDPPort(const uint16 port);

	byte getType(void);
	void setType(const byte type);

	bool madeContact(void);
	void madeContact(bool val);

private:
	CUInt128	m_clientID;
	CUInt128	m_distance;
	uint32		m_ip;
	uint16		m_tcpPort;
	uint16		m_udpPort;
	byte		m_type;
	time_t		m_lastTypeSet;
	time_t		m_expires;
	bool		m_madeContact;
};

} // End namespace