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

#include "stdafx.h"
#include "KademliaUDPListener.h"
#include "../kademlia/Prefs.h"
#include "../kademlia/Kademlia.h"
#include "../kademlia/SearchManager.h"
#include "../routing/Contact.h"
#include "../routing/RoutingZone.h"
#include "../io/ByteIO.h"
#include "../io/IOException.h"
#include "../utils/MiscUtils.h"
#include "../../knownfile.h"
#include "../../knownfilelist.h"
#include "../../otherfunctions.h"
#include "../kademlia/Indexed.h"
#include "../kademlia/tag.h"
#include "../../OpCodes.h"
#include "../kademlia/Defines.h"
#include "emule.h"
#include "ClientUDPSocket.h"
#include "Packets.h"
#include "emuledlg.h"
#include "KadContactListCtrl.h"
#include "kademliawnd.h"
#include "clientlist.h"
#include "Statistics.h"
#include "SafeFile.h"
#include "updownclient.h"
#include "listensocket.h"
#include "StringConversion.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


extern LPCSTR _aszInvKadKeywordCharsA;
extern LPCWSTR _awszInvKadKeywordChars;

////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

void CKademliaUDPListener::bootstrap(LPCTSTR host, uint16 port)
{
	USES_CONVERSION;
	uint32 retVal = 0;
	if (_istalpha((_TUCHAR)host[0])) 
	{
		hostent *hp = gethostbyname(T2CA(host));
		if (hp == NULL) 
			return;
		memcpy (&retVal, hp->h_addr, sizeof(retVal));
	}
	else
		retVal = inet_addr(T2CA(host));
	if (thePrefs.GetDebugClientKadUDPLevel() > 0)
		DebugSend("KadBootstrapReq", ntohl(retVal), port);
	sendMyDetails(KADEMLIA_BOOTSTRAP_REQ, ntohl(retVal), port);
}

void CKademliaUDPListener::bootstrap(uint32 ip, uint16 port)
{
	if (thePrefs.GetDebugClientKadUDPLevel() > 0)
		DebugSend("KadBootstrapReq", ip, port);
	sendMyDetails(KADEMLIA_BOOTSTRAP_REQ, ip, port);
}

void CKademliaUDPListener::sendMyDetails(byte opcode, uint32 ip, uint16 port)
{
	ASSERT( CKademlia::getPrefs() != NULL );
	CSafeMemFile bio(25);
	bio.WriteUInt128(&CKademlia::getPrefs()->getKadID());
	bio.WriteUInt32(CKademlia::getPrefs()->getIPAddress());
	bio.WriteUInt16(thePrefs.GetUDPPort());
	bio.WriteUInt16(thePrefs.GetPort());
	bio.WriteUInt8(0);
	sendPacket(&bio, opcode, ip, port);
}

void CKademliaUDPListener::firewalledCheck(uint32 ip, uint16 port)
{
	CSafeMemFile bio(2);
	bio.WriteUInt16(thePrefs.GetPort());
	if (thePrefs.GetDebugClientKadUDPLevel() > 0)
		DebugSend("KadFirewalledReq", ip, port);
	sendPacket(&bio, KADEMLIA_FIREWALLED_REQ, ip, port);
}

void CKademliaUDPListener::sendNullPacket(byte opcode,uint32 ip, uint16 port)
{
	CSafeMemFile bio(0);
	if (thePrefs.GetDebugClientKadUDPLevel() > 0)
		DebugSend("KadNullPacket", ip, port);
	sendPacket(&bio, opcode, ip, port);
}

void CKademliaUDPListener::publishPacket(uint32 ip, uint16 port, const CUInt128 &targetID, const CUInt128 &contactID, const TagList& tags)
{
	//We need to get the tag lists working with CSafeMemFiles..
	byte packet[1024];
	CByteIO bio(packet, sizeof(packet));
	bio.writeByte(OP_KADEMLIAHEADER);
	bio.writeByte(KADEMLIA_PUBLISH_REQ);
	bio.writeUInt128(targetID);
	//We only use this for publishing sources now.. So we always send one here..
	bio.writeUInt16(1);
	bio.writeUInt128(contactID);
	bio.writeTagList(tags);
	uint32 len = sizeof(packet) - bio.getAvailable();
	if (thePrefs.GetDebugClientKadUDPLevel() > 0)
		DebugSend("KadPublishReq", ip, port);
	sendPacket(packet, len,  ip, port);
}

void CKademliaUDPListener::processPacket(const byte* data, uint32 lenData, uint32 ip, uint16 port)
{
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL);

	//Update constate only when it changes.
	bool curCon = prefs->getLastContact();
	prefs->setLastContact();
	if( curCon != prefs->getLastContact())
		theApp.emuledlg->ShowConnectionState();

	byte opcode = data[1];
	const byte *packetData = data + 2;
	uint32 lenPacket = lenData - 2;

//	AddDebugLogLine( false, _T("Processing UDP Packet from %s port %ld : opcode length %ld", ipstr(senderAddress->sin_addr), ntohs(senderAddress->sin_port), lenPacket);
//	CMiscUtils::debugHexDump(packetData, lenPacket);

	switch (opcode)
	{
		case KADEMLIA_BOOTSTRAP_REQ:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadBootstrapReq", ip, port);
			processBootstrapRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_BOOTSTRAP_RES:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadBootstrapRes", ip, port);
			processBootstrapResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_HELLO_REQ:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadHelloReq", ip, port);
			processHelloRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_HELLO_RES:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadHelloRes", ip, port);
			processHelloResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_REQ:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadReq", ip, port);
			processKademliaRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_RES:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadRes", ip, port);
			processKademliaResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SEARCH_REQ:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadSearchReq", ip, port);
			processSearchRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SEARCH_RES:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadSearchRes", ip, port);
			processSearchResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUBLISH_REQ:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadPublishReq", ip, port);
			processPublishRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUBLISH_RES:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadPublishRes", ip, port);
			processPublishResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SRC_NOTES_REQ:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadSearchNotesReq", ip, port);
			processSearchNotesRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_SRC_NOTES_RES:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadSearchNotesRes", ip, port);
			processSearchNotesResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUB_NOTES_REQ:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadPublishNotesReq", ip, port);
			processPublishNotesRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_PUB_NOTES_RES:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadPublishNotesRes", ip, port);
			processPublishNotesResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FIREWALLED_REQ:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadFirewalledReq", ip, port);
			processFirewalledRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FIREWALLED_RES:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadFirewalledRes", ip, port);
			processFirewalledResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FIREWALLED_ACK:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadFirewalledAck", ip, port);
			processFirewalledResponse2(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FINDBUDDY_REQ:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadFindBuddyReq", ip, port);
			processFindBuddyRequest(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FINDBUDDY_RES:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadFindBuddyRes", ip, port);
			processFindBuddyResponse(packetData, lenPacket, ip, port);
			break;
		case KADEMLIA_FINDSOURCE_REQ:
			if (thePrefs.GetDebugClientKadUDPLevel() > 0)
				DebugRecv("KadFindSourceReq", ip, port);
			processFindSourceRequest(packetData, lenPacket, ip, port);
			break;
		default:
		{
			CString strError;
			strError.Format(_T("Unknown opcode %02x"), opcode);
			throw strError;
		}
	}
}

void CKademliaUDPListener::addContact( const byte *data, uint32 lenData, uint32 ip, uint16 port, uint16 tport)
{
	CSafeMemFile bio( data, lenData);
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 
	CUInt128 id;
	bio.ReadUInt128(&id);
	bio.ReadUInt32();
	bio.ReadUInt16();
	if( tport )
		bio.ReadUInt16();
	else
		tport = bio.ReadUInt16();
	byte type = bio.ReadUInt8();
	// Look for existing client
	CContact *contact = routingZone->getContact(id);
	if (contact != NULL)
	{
		contact->setIPAddress(ip);
		contact->setUDPPort(port);
		contact->setTCPPort(tport);
		theApp.emuledlg->kademliawnd->ContactRef(contact);
	}
	else
	{
		if(::IsGoodIPPort(ntohl(ip),port))
		{
			// Ignore stated ip and port, use the address the packet came from
			routingZone->add(id, ip, port, tport, type);
		}
	}
}

void CKademliaUDPListener::addContacts( const byte *data, uint32 lenData, uint16 numContacts)
{
	CSafeMemFile bio( data, lenData );
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 
	CUInt128 id;
	for (uint16 i=0; i<numContacts; i++)
	{
		bio.ReadUInt128(&id);
		uint32 ip = bio.ReadUInt32();
		uint16 port = bio.ReadUInt16();
		uint16 tport = bio.ReadUInt16();
		byte type = bio.ReadUInt8();
		if(::IsGoodIPPort(ntohl(ip),port))
		{
			routingZone->add(id, ip, port, tport, type);
		}
	}
}

//KADEMLIA_BOOTSTRAP_REQ
void CKademliaUDPListener::processBootstrapRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 25){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used pointers.
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 

	// Add the sender to the list of contacts
	addContact(packetData, lenPacket, ip, port);

	// Get some contacts to return
	ContactList contacts;
	uint16 numContacts = 1 + (uint16)routingZone->getBootstrapContacts(&contacts, 20);

	// Create response packet
	//We only collect a max of 20 contacts here.. Max size is 527.
	//2 + 25(20) + 15(1)
	CSafeMemFile bio(527);

	CUInt128 id;

	// Write packet info
	bio.WriteUInt16(numContacts);
	CContact *contact;
	ContactList::const_iterator it;
	for (it = contacts.begin(); it != contacts.end(); it++)
	{
		contact = *it;
		contact->getClientID(&id);
		bio.WriteUInt128(&id);
		bio.WriteUInt32(contact->getIPAddress());
		bio.WriteUInt16(contact->getUDPPort());
		bio.WriteUInt16(contact->getTCPPort());
		bio.WriteUInt8(contact->getType());
	}
	prefs->getKadID(&id);
	bio.WriteUInt128(&id);
	bio.WriteUInt32(prefs->getIPAddress());
	bio.WriteUInt16(thePrefs.GetUDPPort());
	bio.WriteUInt16(thePrefs.GetPort());
	bio.WriteUInt8(0);

	// Send response
	if (thePrefs.GetDebugClientKadUDPLevel() > 0)
		DebugSend("KadBootstrapRes", ip, port);

	sendPacket(&bio, KADEMLIA_BOOTSTRAP_RES, ip, port);
}

//KADEMLIA_BOOTSTRAP_RES
void CKademliaUDPListener::processBootstrapResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 27){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 

	// How many contacts were given
	CSafeMemFile bio( packetData, lenPacket);
	uint16 numContacts = bio.ReadUInt16();

	// Verify packet is expected size
	if (lenPacket != (2 + 25*numContacts))
		return;

	// Add these contacts to the list.
	addContacts(packetData+2, lenPacket-2, numContacts);
	// Send sender to alive.
	routingZone->setAlive(ip, port);
}

//KADEMLIA_HELLO_REQ
void CKademliaUDPListener::processHelloRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 25){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 

	// Add the sender to the list of contacts
	addContact(packetData, lenPacket, ip, port);

	// Send response
	if (thePrefs.GetDebugClientKadUDPLevel() > 0)
		DebugSend("KadHelloRes", ip, port);
	sendMyDetails(KADEMLIA_HELLO_RES, ip, port);

	// Check if firewalled
	if(prefs->getRecheckIP())
	{
		firewalledCheck(ip, port);
	}
}

//KADEMLIA_HELLO_RES
void CKademliaUDPListener::processHelloResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 25){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 

	// Add or Update contact.
	addContact(packetData, lenPacket, ip, port);

	// Set contact to alive.
	routingZone->setAlive(ip, port);
}

//KADEMLIA_REQ
void CKademliaUDPListener::processKademliaRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 33){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 

	//RecheckIP and firewall status
	if(prefs->getRecheckIP())
	{
		firewalledCheck(ip, port);
		if (thePrefs.GetDebugClientKadUDPLevel() > 0)
			DebugSend("KadHelloReq", ip, port);
		sendMyDetails(KADEMLIA_HELLO_REQ, ip, port);
	}

	// Get target and type
	CSafeMemFile bio( packetData, lenPacket);
	byte type = bio.ReadUInt8();
//		bool flag1 = (type >> 6); //Reserved
//		bool flag2 = (type >> 7); //Reserved
//		bool flag3 = (type >> 8); //Reserved

	type = type & 0x1F;
	if( type == 0 )
	{
		CString strError;
		strError.Format(_T("***NOTE: Received wrong type (0x%02x) in %hs"), type, __FUNCTION__);
		throw strError;
	}

	//This is the target node trying to be found.
	CUInt128 target;
	bio.ReadUInt128(&target);
	CUInt128 distance(prefs->getKadID());
	distance.xor(target);

	//This makes sure we are not mistaken identify. Some client may have fresh installed and have a new hash.
	CUInt128 check;
	bio.ReadUInt128(&check);
	if( prefs->getKadID().compareTo(check))
		return;

	// Get required number closest to target
	ContactMap results;
	routingZone->getClosestTo(0, distance, (int)type, &results);
	uint16 count = (uint16)results.size();

	// Write response
	// Max count is 32. size 817.. 
	// 16 + 1 + 25(32)
	CSafeMemFile bio2( 817 );
	bio2.WriteUInt128(&target);
	bio2.WriteUInt8((byte)count);
	CContact *c;
	CUInt128 id;
	ContactMap::const_iterator it;
	for (it = results.begin(); it != results.end(); it++)
	{
		c = it->second;
		c->getClientID(&id);
		bio2.WriteUInt128(&id);
		bio2.WriteUInt32(c->getIPAddress());
		bio2.WriteUInt16(c->getUDPPort());
		bio2.WriteUInt16(c->getTCPPort());
		bio2.WriteUInt8(c->getType());
	}

	if (thePrefs.GetDebugClientKadUDPLevel() > 0)
		DebugSendF("KadRes", ip, port, _T("Count=%u"), count);

	sendPacket(&bio2, KADEMLIA_RES, ip, port);
}

//KADEMLIA_RES
void CKademliaUDPListener::processKademliaResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 17){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 

	if(prefs->getRecheckIP())
	{
		firewalledCheck(ip, port);
		if (thePrefs.GetDebugClientKadUDPLevel() > 0)
			DebugSend("KadHelloReq", ip, port);
		sendMyDetails(KADEMLIA_HELLO_REQ, ip, port);
	}

	// What search does this relate to
	CSafeMemFile bio( packetData, lenPacket);
	CUInt128 target;
	bio.ReadUInt128(&target);
	uint16 numContacts = bio.ReadUInt8();

	// Verify packet is expected size
	if (lenPacket != 16+1 + (16+4+2+2+1)*numContacts){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	// Set contact to alive.
	routingZone->setAlive(ip, port);

	CUInt128 id;
	ContactList *results = new ContactList;
	try
	{
		for (uint16 i=0; i<numContacts; i++)
		{
			bio.ReadUInt128(&id);
			uint32 ip = bio.ReadUInt32();
			uint16 port = bio.ReadUInt16();
			uint16 tport = bio.ReadUInt16();
			byte type = bio.ReadUInt8();
			if(::IsGoodIPPort(ntohl(ip),port))
			{
				routingZone->add(id, ip, port, tport, type);
				results->push_back(new CContact(id, ip, port, tport, type, target));
			}
		}
	}
	catch(...)
	{
		delete results;
		throw;
	}
	CSearchManager::processResponse(target, ip, port, results);
}

void CKademliaUDPListener::Free(SSearchTerm* pSearchTerms)
{
	if (pSearchTerms->left)
		Free(pSearchTerms->left);
	if (pSearchTerms->right)
		Free(pSearchTerms->right);
	delete pSearchTerms;
}

SSearchTerm* CKademliaUDPListener::CreateSearchExpressionTree(CSafeMemFile& bio, int iLevel)
{
	// the max. depth has to match our own limit for creating the search expression 
	// (see also 'ParsedSearchExpression' and 'GetSearchPacket')
	if (iLevel >= 24){
		AddDebugLogLine(false, _T("***NOTE: Search expression tree exceeds depth limit!"));
		return NULL;
	}
	iLevel++;

	uint8 op = bio.ReadUInt8();
	if (op == 0x00)
	{
		uint8 boolop = bio.ReadUInt8();
		if (boolop == 0x00) // AND
		{
			SSearchTerm* pSearchTerm = new SSearchTerm;
			pSearchTerm->type = SSearchTerm::AND;
			TRACE(" AND");
			if ((pSearchTerm->left = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				ASSERT(0);
				delete pSearchTerm;
				return NULL;
			}
			if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				ASSERT(0);
				Free(pSearchTerm->left);
				delete pSearchTerm;
				return NULL;
			}
			return pSearchTerm;
		}
		else if (boolop == 0x01) // OR
		{
			SSearchTerm* pSearchTerm = new SSearchTerm;
			pSearchTerm->type = SSearchTerm::OR;
			TRACE(" OR");
			if ((pSearchTerm->left = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				ASSERT(0);
				delete pSearchTerm;
				return NULL;
			}
			if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				ASSERT(0);
				Free(pSearchTerm->left);
				delete pSearchTerm;
				return NULL;
			}
			return pSearchTerm;
		}
		else if (boolop == 0x02) // NAND
		{
			SSearchTerm* pSearchTerm = new SSearchTerm;
			pSearchTerm->type = SSearchTerm::NAND;
			TRACE(" NAND");
			if ((pSearchTerm->left = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				ASSERT(0);
				delete pSearchTerm;
				return NULL;
			}
			if ((pSearchTerm->right = CreateSearchExpressionTree(bio, iLevel)) == NULL){
				ASSERT(0);
				Free(pSearchTerm->left);
				delete pSearchTerm;
				return NULL;
			}
			return pSearchTerm;
		}
		else{
			AddDebugLogLine(false, _T("*** Unknown boolean search operator 0x%02x (CreateSearchExpressionTree)"), boolop);
			return NULL;
		}
	}
	else if (op == 0x01) // String
	{
		CTagValueString str(bio.ReadStringUTF8());

		KadTagStrMakeLower(str); // make lowercase, the search code expects lower case strings!
		TRACE(" \"%ls\"", str);

		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = SSearchTerm::String;
		pSearchTerm->astr = new CStringWArray;

		// pre-tokenize the string term
		int iPosTok = 0;
		CTagValueString strTok(str.Tokenize(_awszInvKadKeywordChars, iPosTok));
		while (!strTok.IsEmpty())
		{
			pSearchTerm->astr->Add(strTok);
			strTok = str.Tokenize(_awszInvKadKeywordChars, iPosTok);
		}

		return pSearchTerm;
	}
	else if (op == 0x02) // Meta tag
	{
		// read tag value
		CTagValueString strValue(bio.ReadStringUTF8());

		KadTagStrMakeLower(strValue); // make lowercase, the search code expects lower case strings!

		// read tag name
		CStringA strTagName;
		uint16 lenTagName = bio.ReadUInt16();
		bio.Read(strTagName.GetBuffer(lenTagName), lenTagName);
		strTagName.ReleaseBuffer(lenTagName);

		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = SSearchTerm::MetaTag;
		pSearchTerm->tag = new Kademlia::CTagStr(strTagName, strValue);
		if (lenTagName == 1)
			TRACE(" Tag%02x=\"%ls\"", strTagName[0], strValue);
		else
			TRACE(" \"%s\"=\"%ls\"", strTagName, strValue);
		return pSearchTerm;
	}
	else if (op == 0x03) // Min/Max
	{
		static const struct {
			SSearchTerm::ESearchTermType eSearchTermOp;
			LPCSTR pszOp;
		} _aOps[] =
		{
			{ SSearchTerm::OpEqual,			"="		}, // mmop=0x00
			{ SSearchTerm::OpGreaterEqual,	">="	}, // mmop=0x01
			{ SSearchTerm::OpLessEqual,		"<="	}, // mmop=0x02
			{ SSearchTerm::OpGreater,		">"		}, // mmop=0x03
			{ SSearchTerm::OpLess,			"<"		}, // mmop=0x04
			{ SSearchTerm::OpNotEqual,		"!="	}  // mmop=0x05
		};

		// read tag value
		uint32 uValue = bio.ReadUInt32();

		// read integer operator
		uint8 mmop = bio.ReadUInt8();
		if (mmop >= ARRSIZE(_aOps)){
			AddDebugLogLine(false, _T("*** Unknown integer search op=0x%02x (CreateSearchExpressionTree)"), mmop);
			return NULL;
		}

		// read tag name
		CStringA strTagName;
		uint16 lenTagName = bio.ReadUInt16();
		bio.Read(strTagName.GetBuffer(lenTagName), lenTagName);
		strTagName.ReleaseBuffer(lenTagName);

		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = _aOps[mmop].eSearchTermOp;
		pSearchTerm->tag = new Kademlia::CTagUInt32(strTagName, uValue);

		if (lenTagName == 1)
			TRACE(" Tag%02x%s%u", (BYTE)strTagName[0], _aOps[mmop].pszOp, uValue);
		else
			TRACE(" \"%s\"%s%u", strTagName, _aOps[mmop].pszOp, uValue);

		return pSearchTerm;
	}
	else{
		AddDebugLogLine(false, _T("*** Unknown search op=0x%02x (CreateSearchExpressionTree)"), op);
		return NULL;
	}
}

//KADEMLIA_SEARCH_REQ
void CKademliaUDPListener::processSearchRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 17){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CIndexed *indexed = CKademlia::getIndexed();
	ASSERT(indexed != NULL); 

	CSafeMemFile bio( packetData, lenPacket);
	CUInt128 target;
	bio.ReadUInt128(&target);
	uint8 restrictive = bio.ReadUInt8();

#ifdef _DEBUG
	DWORD dwNow = GetTickCount();
#endif
	if(lenPacket == 17 )
	{
		if(restrictive)
		{
			//Source request
			indexed->SendValidSourceResult(target, ip, port);
			//DEBUG_ONLY( Debug("SendValidSourceResult: Time=%.2f sec\n", (GetTickCount() - dwNow) / 1000.0) );
		}
		else
		{
			//Single keyword request
			indexed->SendValidKeywordResult(target, NULL, ip, port );
			//DEBUG_ONLY( Debug("SendValidKeywordResult (Single): Time=%.2f sec\n", (GetTickCount() - dwNow) / 1000.0) );
		}
	}
	else if(lenPacket > 17)
	{
		SSearchTerm* pSearchTerms = NULL;
		if (restrictive)
		{
			try
			{
				pSearchTerms = CreateSearchExpressionTree(bio, 0);
				TRACE("\n");
			}
			catch(...)
			{
				Free(pSearchTerms);
				throw;
			}
		}
		//Keyword request with added options.
		indexed->SendValidKeywordResult(target, pSearchTerms, ip, port); 
		Free(pSearchTerms);
		//DEBUG_ONLY( Debug("SendValidKeywordResult: Time=%.2f sec\n", (GetTickCount() - dwNow) / 1000.0) );
	}
}

//KADEMLIA_SEARCH_RES
void CKademliaUDPListener::processSearchResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 37){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 

	// Set contact to alive.
	routingZone->setAlive(ip, port);

	// What search does this relate to
	CByteIO bio(packetData, lenPacket);
	CUInt128 target;
	bio.readUInt128(&target);

	// How many results.. Not supported yet..
	uint16 count = bio.readUInt16();
	while( count > 0 )
	{
		// What is the answer
		CUInt128 answer;
		bio.readUInt128(&answer);

		// Get info about answer
		// NOTE: this is the one and only place in Kad where we allow string conversion to local code page in
		// case we did not receive an UTF8 string. this is for backward compatibility for search results which are 
		// supposed to be 'viewed' by user only and not feed into the Kad engine again!
		// If that tag list is once used for something else than for viewing, special care has to be taken for any
		// string conversion!
		TagList *tags = bio.readTagList(true/*bOptACP*/);
		CSearchManager::processResult(target, ip, port, answer,tags);
		count--;
	}
}

//KADEMLIA_PUBLISH_REQ
void CKademliaUDPListener::processPublishRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	//There are different types of publishing..
	//Keyword and File are Stored..
	// Verify packet is expected size
	if (lenPacket < 37){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CIndexed *indexed = CKademlia::getIndexed();
	ASSERT(indexed != NULL);
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 

	if( prefs->getFirewalled() )
		//We are firewalled. We should not index this entry and give publisher a false report.
		return;

	CByteIO bio(packetData, lenPacket);
	CUInt128 file;
	bio.readUInt128(&file);

	CUInt128 distance;
	prefs->getKadID(&distance);
	distance.xor(file);

	if( thePrefs.FilterLANIPs() && distance.get32BitChunk(0) > SEARCHTOLERANCE)
		return;

	bool bDbgInfo = thePrefs.GetDebugClientKadUDPLevel() > 0;
	CString strInfo;
	uint16 count = bio.readUInt16();
	bool flag = false;
	uint8 load = 0;
	while( count > 0 )
	{
		strInfo.Empty();

		CUInt128 target;
		bio.readUInt128(&target);

		Kademlia::CEntry* entry = new Kademlia::CEntry();
		try
		{
			entry->ip = ip;
			entry->udpport = port;
			entry->keyID.setValue(file);
			entry->sourceID.setValue(target);
			uint32 tags = bio.readByte();
			while(tags > 0)
			{
				CTag* tag = bio.readTag();
				if(tag)
				{
					if (!tag->m_name.Compare(TAG_SOURCETYPE) && tag->m_type == 9)
					{
						if( entry->source == false )
						{
							entry->taglist.push_back(new CTagUInt32(TAG_SOURCEIP, entry->ip));
							entry->taglist.push_back(new CTagUInt16(TAG_SOURCEUPORT, entry->udpport));
						}
						else
						{
							//More then one sourcetype tag found.
							delete tag;
						}
						entry->source = true;
					}
					
					if (!tag->m_name.Compare(TAG_NAME))
					{
						if ( entry->fileName.IsEmpty() )
						{
							entry->fileName = tag->GetStr();
							KadTagStrMakeLower(entry->fileName); // make lowercase, the search code expects lower case strings!
							if (bDbgInfo)
								strInfo.AppendFormat(_T("  Name=\"%ls\""), entry->fileName);
							// NOTE: always add the 'name' tag, even if it's stored separately in 'fileName'. the tag is still needed for answering search request
							entry->taglist.push_back(tag);
						}
						else
						{
							//More then one Name tag found.
							delete tag;
						}
					}
					else if (!tag->m_name.Compare(TAG_SIZE))
					{
						if( entry->size == 0 )
						{
							entry->size = tag->GetInt();
							if (bDbgInfo)
								strInfo.AppendFormat(_T("  Size=%u"), entry->size);
							// NOTE: always add the 'size' tag, even if it's stored separately in 'size'. the tag is still needed for answering search request
							entry->taglist.push_back(tag);
						}
						else
						{
							//More then one size tag found
							delete tag;
						}
					}
					else if (!tag->m_name.Compare(TAG_SOURCEPORT))
					{
						if( entry->tcpport == 0 )
						{
							entry->tcpport = tag->GetInt();
							entry->taglist.push_back(tag);
						}
						else
						{
							//More then one port tag found
							delete tag;
						}
					}
					else
					{
						//TODO: Filter tags
						entry->taglist.push_back(tag);
					}
				}
				tags--;
			}
			if (bDbgInfo && !strInfo.IsEmpty())
				Debug(_T("%s\n"), strInfo);
		}
		catch(...)
		{
			delete entry;
			throw;
		}

		if( entry->source == true )
		{
			if( indexed->AddSources(file, target, entry, load ) )
				flag = true;
			else
			{
				delete entry;
				entry = NULL;
			}
		}
		else
		{
			if( indexed->AddKeyword(file, target, entry, load) )
			{
				//This makes sure we send a publish response.. This also makes sure we index all the files for this keyword.
				flag = true;
			}
			else
			{
				//We already indexed the maximum number of keywords.
				//We do not index anymore but we still send a success..
				//Reason: Because if a VERY busy node tells the publisher it failed,
				//this busy node will spread to all the surrounding nodes causing popular
				//keywords to be stored on MANY nodes..
				//So, once we are full, we will periodically clean our list until we can
				//begin storing again..
				flag = true;
				delete entry;
				entry = NULL;
			}
		}
		count--;
	}	
	if( flag )
	{
		CSafeMemFile bio2(17);
		bio2.WriteUInt128(&file);
		bio2.WriteUInt8(load);
		if (thePrefs.GetDebugClientKadUDPLevel() > 0)
			DebugSend("KadPublishRes", ip, port);

		sendPacket( &bio2, KADEMLIA_PUBLISH_RES, ip, port);
	}
}

//KADEMLIA_PUBLISH_ACK
void CKademliaUDPListener::processPublishResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 16){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 

	// Set contact to alive.
	routingZone->setAlive(ip, port);

	CSafeMemFile bio(packetData, lenPacket);
	CUInt128 file;
	bio.ReadUInt128(&file);
	CSearchManager::processPublishResult(file);
}

//KADEMLIA_SRC_NOTES_REQ
void CKademliaUDPListener::processSearchNotesRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 33){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CIndexed *indexed = CKademlia::getIndexed();
	ASSERT(indexed != NULL); 

	CSafeMemFile bio( packetData, lenPacket);
	CUInt128 target;
	bio.ReadUInt128(&target);
	CUInt128 source;
	bio.ReadUInt128(&source);

	indexed->SendValidNoteResult(target, source, ip, port);
}

//KADEMLIA_SRC_NOTES_RES
void CKademliaUDPListener::processSearchNotesResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 37){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 

	// Set contact to alive.
	routingZone->setAlive(ip, port);

	// What search does this relate to
	CByteIO bio(packetData, lenPacket);
	CUInt128 target;
	bio.readUInt128(&target);

	// How many results.. Not supported yet..
	uint16 count = bio.readUInt16();
	while( count > 0 )
	{
		// What is the answer
		CUInt128 answer;
		bio.readUInt128(&answer);

		// Get info about answer
		// NOTE: this is the one and only place in Kad where we allow string conversion to local code page in
		// case we did not receive an UTF8 string. this is for backward compatibility for search results which are 
		// supposed to be 'viewed' by user only and not feed into the Kad engine again!
		// If that tag list is once used for something else than for viewing, special care has to be taken for any
		// string conversion!
		TagList *tags = bio.readTagList(true/*bOptACP*/);
		CSearchManager::processResult(target, ip, port, answer,tags);
		count--;
	}
}

//KADEMLIA_PUB_NOTES_REQ
void CKademliaUDPListener::processPublishNotesRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 37){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CIndexed *indexed = CKademlia::getIndexed();
	ASSERT(indexed != NULL);
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 

	if( prefs->getFirewalled() )
		//We are firewalled. We should not index this entry and give publisher a false report.
		return;

	CByteIO bio(packetData, lenPacket);
	CUInt128 target;
	bio.readUInt128(&target);

	CUInt128 distance;
	prefs->getKadID(&distance);
	distance.xor(target);

	if( thePrefs.FilterLANIPs() && distance.get32BitChunk(0) > SEARCHTOLERANCE)
		return;

	bool bDbgInfo = thePrefs.GetDebugClientKadUDPLevel() > 0;

	CUInt128 source;
	bio.readUInt128(&source);

	Kademlia::CEntry* entry = new Kademlia::CEntry();
	try
	{
		entry->ip = ip;
		entry->udpport = port;
		entry->keyID.setValue(target);
		entry->sourceID.setValue(source);
		uint32 tags = bio.readByte();
		bio.readTagList( &(entry->taglist) );
		entry->source = false;
	}
	catch(...)
	{
		delete entry;
		throw;
	}

	if( entry == NULL )
	{
		throw;
	}
	else if( entry->taglist.size() == 0 || entry->taglist.size() > 2)
	{
		delete entry;
		throw;
	}

	if( indexed->AddNotes(target, source, entry ) )
	{
		CSafeMemFile bio2(16);
		bio2.WriteUInt128(&target);
		if (thePrefs.GetDebugClientKadUDPLevel() > 0)
			DebugSend("KadPublishNotesRes", ip, port);

		sendPacket( &bio2, KADEMLIA_PUB_NOTES_RES, ip, port);
	}
	else
		delete entry;
}

//KADEMLIA_PUB_NOTES_ACK
void CKademliaUDPListener::processPublishNotesResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 16){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}
	//Used Pointers
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 

	// Set contact to alive.
	routingZone->setAlive(ip, port);
}

//KADEMLIA_FIREWALLED_REQ
void CKademliaUDPListener::processFirewalledRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 2){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	CSafeMemFile bio(packetData, lenPacket);
	uint16 tcpport = bio.ReadUInt16();

	CContact contact;
	contact.setIPAddress(ip);
	contact.setTCPPort(tcpport);
	contact.setUDPPort(port);
	theApp.clientlist->RequestTCP(&contact);

	// Send response
	CSafeMemFile bio2(4);
	bio2.WriteUInt32(ip);
	if (thePrefs.GetDebugClientKadUDPLevel() > 0)
		DebugSend("KadFirewalledRes", ip, port);

	sendPacket(&bio2, KADEMLIA_FIREWALLED_RES, ip, port);
}

//KADEMLIA_FIREWALLED_RES
void CKademliaUDPListener::processFirewalledResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 4){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 

	// Set contact to alive.
	routingZone->setAlive(ip, port);

	CSafeMemFile bio(packetData, lenPacket);
	uint32 firewalledIP = bio.ReadUInt32();
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL);

	//Update con state only if something changes.
	if( prefs->getIPAddress() != firewalledIP )
	{
		prefs->setIPAddress(firewalledIP);
		theApp.emuledlg->ShowConnectionState();
	}
	prefs->incRecheckIP();
}

//KADEMLIA_FIREWALLED_ACK
void CKademliaUDPListener::processFirewalledResponse2 (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket != 0){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 
	CRoutingZone *routingZone = CKademlia::getRoutingZone();
	ASSERT(routingZone != NULL); 

	// Set contact to alive.
	routingZone->setAlive(ip, port);

	prefs->incFirewalled();
}

//KADEMLIA_FINDBUDDY_REQ
void CKademliaUDPListener::processFindBuddyRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 34){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 

	if( prefs->getFirewalled() )
		//We are firewalled but somehow we still got this packet.. Don't send a response..
		return;

	CSafeMemFile bio(packetData, lenPacket);
	CUInt128 BuddyID;
	bio.ReadUInt128(&BuddyID);
	CUInt128 userID;
	bio.ReadUInt128(&userID);
	uint16 tcpport = bio.ReadUInt16();

	CContact contact;
	contact.setIPAddress(ip);
	contact.setTCPPort(tcpport);
	contact.setUDPPort(port);
	contact.setClientID(userID);
	theApp.clientlist->IncomingBuddy(&contact, &BuddyID);

	CSafeMemFile bio2(34);
	bio2.WriteUInt128(&BuddyID);
	bio2.WriteUInt128(&prefs->getClientHash());
	bio2.WriteUInt16(thePrefs.GetPort());
	if (thePrefs.GetDebugClientKadUDPLevel() > 0)
		DebugSend("KadFindBuddyRes", ip, port);

	sendPacket(&bio2, KADEMLIA_FINDBUDDY_RES, ip, port);
}

//KADEMLIA_FINDBUDDY_RES
void CKademliaUDPListener::processFindBuddyResponse (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 34){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL);

	CSafeMemFile bio(packetData, lenPacket);
	CUInt128 check;
	bio.ReadUInt128(&check);
	check.xor(CUInt128(true));
	if( prefs->getKadID().compareTo(check))
		return;
	CUInt128 userID;
	bio.ReadUInt128(&userID);
	uint16 tcpport = bio.ReadUInt16();

	CContact contact;
	contact.setIPAddress(ip);
	contact.setTCPPort(tcpport);
	contact.setUDPPort(port);
	contact.setClientID(userID);
	theApp.clientlist->RequestBuddy(&contact);
}

//KADEMLIA_FINDSOURCE_REQ
void CKademliaUDPListener::processFindSourceRequest (const byte *packetData, uint32 lenPacket, uint32 ip, uint16 port)
{
	// Verify packet is expected size
	if (lenPacket < 34){
		CString strError;
		strError.Format(_T("***NOTE: Received wrong size (%u) packet in %hs"), lenPacket, __FUNCTION__);
		throw strError;
	}

	//Used Pointers
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL);

	CUpDownClient* buddy = theApp.clientlist->GetBuddy();
	if( buddy != NULL )
	{
		CSafeMemFile bio(packetData, lenPacket);
		CUInt128 check;
		bio.ReadUInt128(&check);
		CUInt128 bud(buddy->GetBuddyID());
		CUInt128 file;
		bio.ReadUInt128(&file);
		uint16 tcp = bio.ReadUInt16();
		CSafeMemFile bio2(lenPacket+6);
		bio2.WriteUInt128(&check);
		bio2.WriteUInt128(&file);
		bio2.WriteUInt32(ip);
		bio2.WriteUInt16(tcp);
		Packet* packet = new Packet(&bio2, OP_EMULEPROT, OP_CALLBACK);
		if( buddy->socket )
            buddy->socket->SendPacket(packet);
		else
			ASSERT(0);
		if (thePrefs.GetDebugClientKadUDPLevel() > 0)
			DebugSend("KadCallback", ip, port);
	}
}

void CKademliaUDPListener::sendPacket(const byte *data, uint32 lenData, uint32 destinationHost, uint16 destinationPort)
{
	//This is temp.. The entire Kad code will be rewritten using CMemFile and send a Packet object directly.
	Packet* packet = new Packet(OP_KADEMLIAHEADER);
	packet->opcode = data[1];
	packet->pBuffer = new char[lenData+8];
	memcpy(packet->pBuffer, data+2, lenData-2);
	packet->size = lenData-2;
	if( lenData > 200 )
		packet->PackPacket();
	theStats.AddUpDataOverheadKad(packet->size);
	theApp.clientudp->SendPacket(packet, ntohl(destinationHost), destinationPort);
}

void CKademliaUDPListener::sendPacket(const byte *data, uint32 lenData, byte opcode, uint32 destinationHost, uint16 destinationPort)
{
	Packet* packet = new Packet(OP_KADEMLIAHEADER);
	packet->opcode = opcode;
	packet->pBuffer = new char[lenData];
	memcpy(packet->pBuffer, data, lenData);
	packet->size = lenData;
	if( lenData > 200 )
		packet->PackPacket();
	theStats.AddUpDataOverheadKad(packet->size);
	theApp.clientudp->SendPacket(packet, ntohl(destinationHost), destinationPort);
}

void CKademliaUDPListener::sendPacket(CSafeMemFile *data, byte opcode, uint32 destinationHost, uint16 destinationPort)
{
	Packet* packet = new Packet(data, OP_KADEMLIAHEADER);
	packet->opcode = opcode;
	if( packet->size > 200 )
		packet->PackPacket();
	theStats.AddUpDataOverheadKad(packet->size);
	theApp.clientudp->SendPacket(packet, ntohl(destinationHost), destinationPort);
}