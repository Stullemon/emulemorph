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

#include "../../Types.h"
#include "Maps.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CUInt128;
class CRoutingZone;

class CRoutingBin
{
	friend class CRoutingZone;

public:

	~CRoutingBin();

private:

	CRoutingBin();
	bool add(CContact *contact);
	void setAlive(uint32 ip, uint16 port);
	void setTCPPort(uint32 ip, uint16 port, uint16 tcpPort);
	void remove(CContact *contact);
	bool contains(const CUInt128 &id);
	CContact *getContact(const CUInt128 &id);
	CContact *getOldest(void);

	UINT getSize() const;
	UINT getRemaining(void) const;
	void getEntries(ContactList *result, bool emptyFirst = true);
	int getClosestTo(int maxType, const CUInt128 &target, int maxRequired, ContactMap *result, bool emptyFirst = true);

	// Debug purposes.
	void dumpContents(void);

	bool m_dontDeleteContacts;
	ContactList m_entries;
};

} // End namespace
