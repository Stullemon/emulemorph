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
#include "Maps.h"
#include "../utils/UInt128.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CRoutingBin;
class CPing;
class CContact;

/**
 * The *Zone* is just a node in a binary tree of *Zone*s.
 * Each zone is either an internal node or a leaf node.
 * Internal nodes have "bin == null" and "subZones[i] != null",
 * leaf nodes have "subZones[i] == null" and "bin != null".
 * 
 * All key pseudoaddresses are relative to the center (self), which
 * is considered to be 000..000
 */
class CRoutingZone : public CCriticalSection
{
public:

	CRoutingZone();
	CRoutingZone(LPCSTR filename);
	~CRoutingZone();


	time_t m_nextBigTimer;
	time_t m_nextSmallTimer;
	bool onBigTimer(void);
	void onSmallTimer(void);
	uint32 estimateCount(void);

	bool add(const CUInt128 &id, uint32 ip, uint16 port, uint16 tport, byte type);
	void setAlive(uint32 ip, uint16 port);

	bool contains(const CUInt128 &id) const;
	CContact *getContact(const CUInt128 &id) const;
	UINT getNumContacts(void) const;

	// Returns a list of all contacts in all leafs of this zone.
	void getAllEntries(ContactList *result, bool emptyFirst = true);

	// Returns the *maxRequired* tokens that are closest to the target within this zone's subtree.
	int getClosestTo(int maxType, const CUInt128 &target, int maxRequired, ContactMap *result, bool emptyFirst = true) const;

	// Ideally: Returns all contacts that are in buckets of common range between us and the asker.
	// In practice: returns the contacts from the top (2^{logBase+1}) buckets.
	UINT getBootstrapContacts(ContactList *results, UINT maxRequired);

	/** Debugging. */
	void dumpContents(LPCTSTR prefix = NULL) const;
	void selfTest(void);

	uint64 getApproximateNodeCount(UINT ourLevel) const;

private:

	CRoutingZone(CRoutingZone *super_zone, int level, const CUInt128 &zone_index);
	void init(CRoutingZone *super_zone, int level, const CUInt128 &zone_index);

	/**
	 * Zone pair is an array of two. Either both entries are null, which
	 * means that *this* is a leaf zone, or both are non-null which means
	 * that *this* has been split into equally sized finer zones.
	 * The zone with index 0 is the one closer to our *self* token.
	 */
	CRoutingZone *m_subZones[2];
	CRoutingZone *m_superZone;

	static CString m_filename;
	static CUInt128 me;

	void readFile(void);
	void writeFile(void);

	bool isLeaf(void) const;
	bool canSplit(void) const;
	bool areWeInFirstSubtreeOfEnoughSize(void) const;
	void consolidate(void);

	// Returns all contacts from this zone tree that are no deeper than *depth* from the current zone.
	void topDepth(int depth, ContactList *result, bool emptyFirst = true);

	// Returns the maximum depth of the tree as the number of edges of the longest path to a leaf.
	UINT getMaxDepth(void) const;

	void randomBin(ContactList *result, bool emptyFirst = true);

	/**
	 * The level indicates what size chunk of the address space
	 * this zone is representing. Level 0 is the whole space,
	 * level 1 is 1/2 of the space, level 2 is 1/4, etc.
	 */
	UINT m_level;

	/**
	 * This is the distance in number of zones from the zone at this level
	 * that contains the center of the system; distance is wrt the XOR metric.
	 */
	CUInt128 m_zoneIndex;

	/** List of contacts, if this zone is a leaf zone. */
	CRoutingBin *m_bin;
	
	/**
	 * Generates a new TokenBin for this zone. Used when the current zone is becoming a leaf zone.
	 * Must be deleted by caller
	 */
	CRoutingZone *genSubZone(int side);

	void split(void);
	void merge(void);

	void startTimer(void);
	void stopTimer(void);

	void randomLookup(void);
};

} // End namespace