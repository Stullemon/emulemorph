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

#include "stdafx.h"
#include "Contact.h"
#include "../kademlia/Prefs.h"
#include "../kademlia/Kademlia.h"
#include "../utils/MiscUtils.h"
#include "../io/ByteIO.h"
#include "../../OpCodes.h"
#include "../net/KademliaUDPListener.h"
#include "../kademlia/Defines.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CContact::~CContact()
{
	Kademlia::CKademlia::reportContactRem(this);
}

CContact::CContact()
{
	m_clientID = 0;
	m_ip = 0;
	m_udpPort = 0;
	m_tcpPort = 0;
	m_type = 1;
	m_expires = 0;
	m_madeContact = false;
	m_lastTypeSet = time(NULL);
}

CContact::CContact(const CUInt128 &clientID, const uint32 ip, const uint16 udpPort, const uint16 tcpPort, const byte type)
{
	m_clientID = clientID;
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 
	prefs->getClientID(&m_distance);
	m_distance.xor(clientID);
	m_ip = ip;
	m_udpPort = udpPort;
	m_tcpPort = tcpPort;
	m_type = 1;//type; Set all new contacts to 1 to avoid spreading dead contacts..
	m_expires = 0;
	m_madeContact = false;
	m_lastTypeSet = time(NULL);
}

CContact::CContact(const CUInt128 &clientID, const uint32 ip, const uint16 udpPort, const uint16 tcpPort, const byte type, const CUInt128 &target)
{
	m_clientID = clientID;
	CPrefs *prefs = CKademlia::getPrefs();
	m_distance.setValue(target);
	m_distance.xor(clientID);
	m_ip = ip;
	m_udpPort = udpPort;
	m_tcpPort = tcpPort;
	m_type = 1;//type; Set all new contacts to 1 to avoid spreading dead contacts..
	m_expires = 0;
	m_madeContact = false;
	m_lastTypeSet = time(NULL);
}

/*CContact::CContact(const CUInt128 &clientID, const uint32 ip, const uint16 udpPort, const byte type, const uint16 tcpPort)
{
	m_clientID = clientID;
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 
	prefs->getClientID(&m_distance);
	m_distance.xor(clientID);
	m_ip = ip;
	m_tcpPort = tcpPort;
	m_udpPort = udpPort;
	m_type = type;
	m_expires = 0;
	m_madeContact = false;
	m_lastTypeSet = time(NULL);
//	Kademlia::CKademlia::reportContactAdd(this);
}
*/
void CContact::getClientID(CUInt128 *id)
{
	id->setValue(m_clientID);
}

void CContact::getClientID(CString *id)
{
	m_clientID.toHexString(id);
}

void CContact::setClientID(const CUInt128 &clientID)
{
	m_clientID = clientID;
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 
	prefs->getClientID(&m_distance);
	m_distance.xor(clientID);
}

void CContact::getDistance(CUInt128 *distance)
{
	distance->setValue(m_distance);
}

void CContact::getDistance(CString *distance)
{
	m_distance.toBinaryString(distance);
}

uint32 CContact::getIPAddress(void)
{
	return m_ip;
}

void CContact::getIPAddress(CString *ip)
{
	CMiscUtils::ipAddressToString(m_ip, ip);
}

void CContact::setIPAddress(const uint32 ip)
{
	m_ip = ip;
}

uint16 CContact::getTCPPort(void)
{
	return m_tcpPort;
}

void CContact::getTCPPort(CString *port)
{
	port->Format("%ld", m_tcpPort);
}

void CContact::setTCPPort(const uint16 port)
{
	m_tcpPort = port;
}

uint16 CContact::getUDPPort(void)
{
	return m_udpPort;
}

void CContact::getUDPPort(CString *port)
{
	port->Format("%ld", m_udpPort);
}

void CContact::setUDPPort(const uint16 port)
{
	m_udpPort = port;
}

byte CContact::getType(void)
{
	return m_type;
}

void CContact::setType(const byte type)
{
	if(type != 0 && time(NULL) - m_lastTypeSet < 10 )
	{
		return;
	}
	if(type > 1 )
	{
		if( m_expires == 0 ) // Just in case..
			m_expires = time(NULL) + ONE_MIN*3;
		else if( m_type == 1 )
			m_expires = time(NULL) + ONE_MIN*3;
		m_type = 2; //Just in case in case again..
		Kademlia::CKademlia::reportContactRef(this);
		return;
	}
	m_lastTypeSet = time(NULL);
	m_type = type;
	if( m_type == 0 )
		m_expires = time(NULL) + HOUR*2;
	else 
		m_expires = time(NULL) + HOUR*1;
	Kademlia::CKademlia::reportContactRef(this);
}

bool CContact::madeContact(void)
{
	return m_madeContact;
}

void CContact::madeContact(bool val)
{
	m_madeContact = val;
	if( m_madeContact == true )
		setType(0);
}