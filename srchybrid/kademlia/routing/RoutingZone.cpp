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

/**
 * The *Zone* is just a node in a binary tree of *Zone*s.
 * Each zone is either an internal node or a leaf node.
 * Internal nodes have "bin == null" and "subZones[i] != null",
 * leaf nodes have "subZones[i] == null" and "bin != null".
 * 
 * All key unique id's are relative to the center (self), which
 * is considered to be 000..000
 */
#include "stdafx.h"
#include <math.h>
#include "RoutingZone.h"
#include "Contact.h"
#include "RoutingBin.h"
#include "../utils/UInt128.h"
#include "../utils/MiscUtils.h"
#include "../kademlia/Kademlia.h"
#include "../kademlia/Prefs.h"
#include "../kademlia/SearchManager.h"
#include "../kademlia/Defines.h"
#include "../kademlia/Error.h"
#include "../net/KademliaUDPListener.h"
#include "../../otherfunctions.h"
#include "../../Opcodes.h"
#include "emule.h"
#include "emuledlg.h"
#include "KadContactListCtrl.h"
#include "kademliawnd.h"
#include "SafeFile.h"
#include "Log.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

void DebugSend(LPCTSTR pszMsg, uint32 ip, uint16 port);

// This is just a safety precaution
#define CONTACT_FILE_LIMIT 5000

CString CRoutingZone::m_filename;
CUInt128 CRoutingZone::me = (ULONG)0;

CRoutingZone::CRoutingZone()
{
	// Can only create routing zone after prefs
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 
	prefs->getKadID(&me);
	m_filename = CMiscUtils::getAppDir();
	m_filename.Append(CONFIGFOLDER);
	m_filename.Append(_T("nodes.dat"));
	CUInt128 zero((ULONG)0);
	init(NULL, 0, zero);

}

CRoutingZone::CRoutingZone(LPCSTR filename)
{
	// Can only create routing zone after prefs
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 
	prefs->getKadID(&me);
	m_filename = filename;
	CUInt128 zero((ULONG)0);
	init(NULL, 0, zero);
}

CRoutingZone::CRoutingZone(CRoutingZone *super_zone, int level, const CUInt128 &zone_index)
{
	init(super_zone, level, zone_index);
}

void CRoutingZone::init(CRoutingZone *super_zone, int level, const CUInt128 &zone_index)
{
	m_superZone = super_zone;
	m_level = level;
	m_zoneIndex = zone_index;
	m_subZones[0] = NULL;
	m_subZones[1] = NULL;
	m_bin = new CRoutingBin();

	m_nextSmallTimer = time(NULL) + m_zoneIndex.get32BitChunk(3);

	if ((m_superZone == NULL) && (m_filename.GetLength() > 0))
		readFile();

	startTimer();
}

CRoutingZone::~CRoutingZone()
{
	if ((m_superZone == NULL) && (m_filename.GetLength() > 0))
	{
		theApp.emuledlg->kademliawnd->HideContacts();
		writeFile();
	}
	if (isLeaf())
		delete m_bin;
	else
	{
		delete m_subZones[0];
		delete m_subZones[1];
	}
	if (m_superZone == NULL)
		theApp.emuledlg->kademliawnd->ShowContacts();
}

void CRoutingZone::readFile(void)
{
	try
	{
		theApp.emuledlg->kademliawnd->HideContacts();
		uint32 numContacts = 0;
		CSafeBufferedFile file;
		CFileException fexp;
		if (file.Open(m_filename, CFile::modeRead | CFile::osSequentialScan|CFile::typeBinary|CFile::shareDenyWrite, &fexp))
		{
			setvbuf(file.m_pStream, NULL, _IOFBF, 32768);

			numContacts = file.ReadUInt32();

			CUInt128 id;
			uint32 ip;
			uint16 udpPort;
			uint16 tcpPort;
			byte type;
			for (uint32 i=0; i<numContacts; i++)
			{
				file.ReadUInt128(&id);
				ip		= file.ReadUInt32();
				udpPort	= file.ReadUInt16();
				tcpPort	= file.ReadUInt16();
				type	= file.ReadUInt8();
				if(::IsGoodIPPort(ntohl(ip),udpPort))
				{
					if( type < 2)
						add(id, ip, udpPort, tcpPort, type);
				}
			}
			file.Close();
			AddLogLine( false, GetResString(IDS_KADCONTACTSREAD), numContacts);
		}
		if (numContacts == 0)
			AddDebugLogLine( false, GetResString(IDS_ERR_KADCONTACTS));
	} 
	//TODO: Make this catch an CFileException..
	catch (...) 
	{
		AddDebugLogLine(false, _T("Exception in CRoutingZone::readFile"));
	}
	theApp.emuledlg->kademliawnd->ShowContacts();
}

void CRoutingZone::writeFile(void)
{
	try
	{
		uint32 count = 0;
		CContact *c;
		CUInt128 id;
		CSafeBufferedFile file;
		CFileException fexp;
		if (file.Open(m_filename, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary|CFile::shareDenyWrite, &fexp))
		{
			setvbuf(file.m_pStream, NULL, _IOFBF, 32768);

			ContactList contacts;
			getBootstrapContacts(&contacts, 200);
			file.WriteUInt32((uint32)min(contacts.size(), CONTACT_FILE_LIMIT));
			ContactList::const_iterator it;
			for (it = contacts.begin(); it != contacts.end(); it++)
			{
				count++;
				c = *it;
				c->getClientID(&id);
				file.WriteUInt128(&id);
				file.WriteUInt32(c->getIPAddress());
				file.WriteUInt16(c->getUDPPort());
				file.WriteUInt16(c->getTCPPort());
				file.WriteUInt8(c->getType());
				if (count == CONTACT_FILE_LIMIT)
					break;
			}
			file.Close();
		}
		AddDebugLogLine( false, _T("Wrote %ld contact%s to file."), count, ((count == 1) ? _T("") : _T("s")));
	} 
	//TODO: Make this catch an CFileException..
	catch (...) 
	{
		AddDebugLogLine(false, _T("Exception in CRoutingZone::writeFile"));
	}
}

bool CRoutingZone::canSplit(void) const
{
	if (m_level >= 127)
		return false;
		
	/* Check if we are close to the center */
	if ( (m_zoneIndex < KK || m_level < KBASE) && m_bin->getSize() == K)
		return true;
	return false;
}

bool CRoutingZone::add(const CUInt128 &id, uint32 ip, uint16 port, uint16 tport, byte type)
{
	if (id == me)
		return false;

	bool retVal = false;
	CUInt128 distance(me);
	distance.xor(id);
	CContact *c = NULL;

	try
	{
		if (!isLeaf()) 
			retVal = m_subZones[distance.getBitNumber(m_level)]->add(id, ip, port, tport, type);
		else 
		{
			c = m_bin->getContact(id);
			if (c != NULL)
			{
				c->setIPAddress(ip);
				c->setUDPPort(port);
				c->setTCPPort(tport);
				retVal = true;
				theApp.emuledlg->kademliawnd->ContactRef(c);
			}
			else if (m_bin->getRemaining() > 0)
			{
				c = new CContact(id, ip, port, tport, type);
				retVal = m_bin->add(c);
				if(retVal)
				{
					if (theApp.emuledlg->kademliawnd->ContactAdd(c))
						c->setGuiRefs(true);
				}
			}
			else if (canSplit()) 
			{
				split();
				retVal = m_subZones[distance.getBitNumber(m_level)]->add(id, ip, port, tport, type);
			} 
			else 
			{
				merge();
				c = new CContact(id, ip, port, tport, type);
				retVal = m_bin->add(c);
				if(retVal)
				{
					if (theApp.emuledlg->kademliawnd->ContactAdd(c))
						c->setGuiRefs(true);
				}
			}

			if (!retVal)
			{
				if (c != NULL)
					delete c;
			}
		}
	} 
	catch (...) 
	{
		AddDebugLogLine(false, _T("Exception in CRoutingZone::add"));
	}
	return retVal;
}

void CRoutingZone::setAlive(uint32 ip, uint16 port)
{
	try
	{
		if (isLeaf())
			m_bin->setAlive(ip, port);
		else
		{
			m_subZones[0]->setAlive(ip, port);
			m_subZones[1]->setAlive(ip, port);
		}
	} 
	catch (...) 
	{
		AddDebugLogLine(false, _T("Exception in CRoutingZone::setAlive"));
	}
}

CContact *CRoutingZone::getContact(const CUInt128 &id) const
{
	if (isLeaf())
		return m_bin->getContact(id);
	else
		return m_subZones[id.getBitNumber(m_level)]->getContact(id);
}

int CRoutingZone::getClosestTo(int maxType, const CUInt128 &target, int maxRequired, ContactMap *result, bool emptyFirst, bool inUse) const
{
	// If leaf zone, do it here
	if (isLeaf())
		return m_bin->getClosestTo(maxType, target, maxRequired, result, emptyFirst, inUse);
	
	// otherwise, recurse in the closer-to-the-target subzone first
	int closer = target.getBitNumber(m_level);
	int found = m_subZones[closer]->getClosestTo(maxType, target, maxRequired, result, emptyFirst, inUse);

	// if still not enough tokens found, recurse in the other subzone too
	if (found < maxRequired)
		found += m_subZones[1-closer]->getClosestTo(maxType, target, maxRequired-found, result, false, inUse);
	
	return found;
}

void CRoutingZone::getAllEntries(ContactList *result, bool emptyFirst)
{
	if (isLeaf())
	{
		try
		{
			m_bin->getEntries(result, emptyFirst);
		} 
		catch (...) 
		{
			AddDebugLogLine(false, _T("Exception in CRoutingZone::getAllEntries"));
		}
	}
	else
	{
		m_subZones[0]->getAllEntries(result, emptyFirst);
		m_subZones[1]->getAllEntries(result, false);			
	}
}

void CRoutingZone::topDepth(int depth, ContactList *result, bool emptyFirst)
{
	if (isLeaf())
	{
		try
		{
			m_bin->getEntries(result, emptyFirst);
		} 
		catch (...) 
		{
			AddDebugLogLine(false, _T("Exception in CRoutingZone::topDepth"));
		}
	}
	else if (depth <= 0)
		randomBin(result, emptyFirst);
	else
	{
		m_subZones[0]->topDepth(depth-1, result, emptyFirst);
		m_subZones[1]->topDepth(depth-1, result, false);
	}
}

void CRoutingZone::randomBin(ContactList *result, bool emptyFirst)
{
	if (isLeaf())
	{
		try
		{
			m_bin->getEntries(result, emptyFirst);
		} 
		catch (...) 
		{
			AddDebugLogLine(false, _T("Exception in CRoutingZone::randomBin"));
		}
	}
	else
		m_subZones[rand()&1]->randomBin(result, emptyFirst);
}

uint32 CRoutingZone::getMaxDepth(void) const
{
	if (isLeaf())
		return 0;
	return 1 + max(m_subZones[0]->getMaxDepth(), m_subZones[1]->getMaxDepth());
}

void CRoutingZone::split(void)
{
	try
	{
		stopTimer();
		
		m_subZones[0] = genSubZone(0);
		m_subZones[1] = genSubZone(1);

		ContactList entries;
		m_bin->getEntries(&entries);
		ContactList::const_iterator it;
		for (it = entries.begin(); it != entries.end(); it++)
		{
			int sz = (*it)->m_distance.getBitNumber(m_level);
			m_subZones[sz]->m_bin->add(*it);
		}
		m_bin->m_dontDeleteContacts = true;
		delete m_bin;
		m_bin = NULL;
	} 
	catch (...)
	{
		AddDebugLogLine(false, _T("Exception in CRoutingZone::split"));
	}
}

void CRoutingZone::merge(void)
{
	try
	{
        if (isLeaf() && m_superZone != NULL)
			m_superZone->merge();
		else if ((!isLeaf())
			&& (m_subZones[0]->isLeaf() && m_subZones[1]->isLeaf()) 
			&&	(getNumContacts()) < (K/2) )
		{
			m_bin = new CRoutingBin();
			
			m_subZones[0]->stopTimer();
			m_subZones[1]->stopTimer();

			if (getNumContacts() > 0)
			{
				ContactList list0;
				ContactList list1;
				m_subZones[0]->m_bin->getEntries(&list0);
				m_subZones[1]->m_bin->getEntries(&list1);
				ContactList::const_iterator it;
				for (it = list0.begin(); it != list0.end(); it++)
					m_bin->add(*it);
				for (it = list1.begin(); it != list1.end(); it++)
					m_bin->add(*it);
			}

			m_subZones[0]->m_superZone = NULL;
			m_subZones[1]->m_superZone = NULL;

			delete m_subZones[0];
			delete m_subZones[1];

			m_subZones[0] = NULL;
			m_subZones[1] = NULL;

			startTimer();
			
			if (m_superZone != NULL)
				m_superZone->merge();
		}
	} 
	catch (...) 
	{
		AddDebugLogLine(false, _T("Exception in CRoutingZone::merge"));
	}
}

bool CRoutingZone::isLeaf(void) const
{
	return (m_bin != NULL);
}

CRoutingZone *CRoutingZone::genSubZone(int side) 
{
	CUInt128 newIndex(m_zoneIndex);
	newIndex.shiftLeft(1);
	if (side != 0)
		newIndex.add(1);
	CRoutingZone *retVal = new CRoutingZone(this, m_level+1, newIndex);
	return retVal;
}

void CRoutingZone::startTimer(void)
{
	time_t now = time(NULL);
	// Start filling the tree, closest bins first.
	m_nextBigTimer = now + (MIN2S(1)*m_zoneIndex.get32BitChunk(3)) + SEC(10);
	CKademlia::addEvent(this);
}

void CRoutingZone::stopTimer(void)
{
	CKademlia::removeEvent(this);
}

bool CRoutingZone::onBigTimer(void)
{
	if (!isLeaf())
		return false;

	if ( (m_zoneIndex < KK || m_level < KBASE || m_bin->getRemaining() >= (K*.4)))
	{
		randomLookup();
		return true;
	}

	return false;
}

//This is used when we find a leaf and want to know what this sample looks like.
//We fall back two levels and take a sample to try to minimize any areas of the 
//tree that will give very bad results.
uint32 CRoutingZone::estimateCount()
{
	if( !isLeaf() )
		return 0;
	if( m_level < KBASE )
		return (pow(2, m_level)*10);
	CRoutingZone* curZone = m_superZone->m_superZone->m_superZone;

	float modify = ((float)curZone->getNumContacts())/20.0F;
	return (pow( 2, m_level-2))*10*(modify);
}

void CRoutingZone::onSmallTimer(void)
{
	if (!isLeaf())
		return;

	CString test;
	m_zoneIndex.toBinaryString(&test);

	CContact *c = NULL;
	time_t now = time(NULL);
	ContactList entries;
	ContactList::iterator it;

	try
	{
		// Remove dead entries
		m_bin->getEntries(&entries);
		for (it = entries.begin(); it != entries.end(); it++)
		{
			c = *it;
			if ( c->getType() > 1)
			{
				if (((c->m_expires > 0) && (c->m_expires <= now)))
				{
					if(!c->inUse())
					{
						m_bin->remove(c);
						delete c;
					}
					continue;
				}
			}
			if(c->m_expires == 0 && c->madeContact() == false)
				c->m_expires = now;
		}
		c = NULL;
		//Ping only contacts that are in the branches that meet the set level and are not close to our ID.
		//The other contacts are checked with the big timer.
		if( m_bin->getRemaining() < (K*(.4)) )
			c = m_bin->getOldest();
		if( c != NULL )
		{
			if ( c->m_expires >= now || c->getType() == 2)
			{
				m_bin->remove(c);
				m_bin->m_entries.push_back(c);
				c = NULL;
			}
		}
	} 
	catch (...) 
	{
		AddDebugLogLine(false, _T("Exception in CRoutingZone::onSmallTimer"));
	}
	if(c != NULL)
	{
		c->setType(c->getType()+1);
		CKademliaUDPListener *udpListner = CKademlia::getUDPListener();
		ASSERT(udpListner != NULL); 
		if (thePrefs.GetDebugClientKadUDPLevel() > 0)
			DebugSend("KadHelloReq", c->getIPAddress(), c->getUDPPort());
		udpListner->sendMyDetails(KADEMLIA_HELLO_REQ, c->getIPAddress(), c->getUDPPort());
	}
}

void CRoutingZone::randomLookup(void) 
{
	// Look-up a random client in this zone
	CUInt128 prefix(m_zoneIndex);
	prefix.shiftLeft(128 - m_level);
	CUInt128 random(prefix, m_level);
	random.xor(me);
	CSearchManager::findNode(random);
}

uint32 CRoutingZone::getNumContacts(void) const
{
	if (isLeaf())
		return m_bin->getSize();
	else
		return m_subZones[0]->getNumContacts() + m_subZones[1]->getNumContacts();
}

uint32 CRoutingZone::getBootstrapContacts(ContactList *results, uint32 maxRequired)
{
	ASSERT(m_superZone == NULL);

	results->clear();

	uint32 retVal = 0;
	try
	{
		ContactList top;
		topDepth(LOG_BASE_EXPONENT, &top);
		if (top.size() > 0)
		{
			ContactList::const_iterator it;
			for (it = top.begin(); it != top.end(); it++)
			{
				results->push_back(*it);
				retVal++;
				if (retVal == maxRequired)
					break;
			}
		}
	} 
	catch (...) 
	{
		AddDebugLogLine(false, _T("Exception in CRoutingZone::getBoostStrapContacts"));
	}
	return retVal;
}

/*
uint64 CRoutingZone::getApproximateNodeCount(uint32 ourLevel) const
{
	if (isLeaf()) 
		return ((uint64)1 << ourLevel) * (uint64)m_bin->getSize();
	return (m_subZones[0]->getApproximateNodeCount(ourLevel+1) + m_subZones[1]->getApproximateNodeCount(ourLevel+1)) / 2;
}
*/

/*
void CRoutingZone::selfTest(void)
{
	// This is not intended to be a conclusive test of the routing zone, 
	// just a place to put some simple debugging tests

	CString msg;

	// Try storing a lot of random keys
	CUInt128 id;
	for (int i=0; i<100000; i++)
	{
		id.setValueRandom();
id.toHexString(&msg);
OutputDebugString(msg);
OutputDebugString(_T("\r\n"));

		if (!add(id, 0xC0A80001, 0x1234, 0x4321, 0))
			break;
//		if (i%20 == 0)
//			dumpContents();
	}
	dumpContents();

	// Should have good dispersion now

	// Try some close to me, should keep splitting
	CUInt128 *close = new CUInt128;
	for (int i=0; i<100000; i++)
	{
		delete close;
		close = new CUInt128(me, 1+i%128);
close->toHexString(&msg);
OutputDebugString(msg);
OutputDebugString(_T("\r\n"));
		if (!add(*close, 0xC0A80001, 0x1234, 0x4321, 0))
			break;
//		if (i%20 == 0)
//			root->dumpContents();
	}
	dumpContents();
	delete close;

	// Try to find the nearest

	id.setValueRandom();
	id.toHexString(&msg);
	OutputDebugString(_T("Trying to find nearest to : "));
	OutputDebugString(msg);
	OutputDebugString(_T("\r\n"));
	CUInt128 x(me);
	x.xor(id);
	OutputDebugString(_T("Distance from me                 : "));
	x.toBinaryString(&msg);
	OutputDebugString(msg);
	OutputDebugString(_T("\r\n"));

	ContactMap result;
	getClosestTo(0, id, 20, &result);

	CString line;
	CString hex;
	CString distance;
	CContact *c;
	ContactMap::const_iterator it;
	for (it = result.begin(); it != result.end(); it++)
	{
		c = it->second;
		c->m_clientID.toHexString(&hex);
		c->getDistance(&distance);
		line.Format(_T("%s : %s\r\n"), hex, distance);
		OutputDebugString(line);
	}
}
*/
/*
//Don't delete, just keep for future reference..
void CRoutingZone::dumpContents(LPCTSTR prefix) const
{
#ifdef DEBUG
	CString msg;
	CString ziStr;
	m_zoneIndex.toBinaryString(&ziStr, true);

	if (prefix == NULL)
		OutputDebugString(_T("------------------------------------------------------\r\n"));
	if (isLeaf()) 
	{
		msg.Format(_T("Zone level: %ld\tZone prefix: %s\tContacts: %ld\tZoneIndex: %s\r\n"), 
			m_level, (prefix == NULL) ? _T("ROOT") : prefix, getNumContacts(), ziStr);
		OutputDebugString(msg);
		m_bin->dumpContents();
	} 
	else 
	{
		msg.Format(_T("Zone level: %ld\tZone prefix: %s\tContacts: %ld\tZoneIndex: %s NODE\r\n"), 
					m_level, (prefix == NULL) ? _T("ROOT") : prefix, getNumContacts(), ziStr);
		OutputDebugString(msg);
		msg.Format(_T("%s0"), (prefix == NULL) ? _T("") : prefix);
		m_subZones[0]->dumpContents(msg.GetBuffer(0));
		msg.Format(_T("%s1"), (prefix == NULL) ? _T("") : prefix);
		m_subZones[1]->dumpContents(msg.GetBuffer(0));
	}
#endif
}
*/
