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
#include "../kademlia/tag.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CEntry
{
public:
	CEntry()
	{
		ip = 0;
		tcpport = 0;
		udpport = 0;
		(void)fileName;
		size = 0;
		lifetime = time(NULL);
		source = false;
	}
	~CEntry()
	{
		TagList::const_iterator it;
		for (it = taglist.begin(); it != taglist.end(); it++)
			delete *it;
	}
	uint32 ip;
	uint16 tcpport;
	uint16 udpport;
	CUInt128 keyID;
	CUInt128 sourceID;
	CTagValueString fileName; // NOTE: this always holds the string in LOWERCASE!!!
	uint32	size;
	TagList taglist;
	time_t lifetime;
	bool source;
};

}
