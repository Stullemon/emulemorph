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

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

void CKademliaUDPListener::bootstrap(const LPCSTR ip, const uint16 port)
{
//	CKademlia::debugMsg("Sent UDP OpCode KADEMLIA_CONNECT 0x0A (%u)", ip);
	sendMyDetails(KADEMLIA_BOOTSTRAP_REQ, nameToIP(ip), port);
}

void CKademliaUDPListener::bootstrap(const uint32 ip, const uint16 port)
{
//	CKademlia::debugMsg("Sent UDP OpCode KADEMLIA_CONNECT (%u)", ip);
	sendMyDetails(KADEMLIA_BOOTSTRAP_REQ, ip, port);
}

void CKademliaUDPListener::sendMyDetails(const byte opcode, const uint32 ip, const uint16 port)
{
	byte packet[27];
	CByteIO bio(packet, sizeof(packet));
	bio.writeByte(OP_KADEMLIAHEADER);
	bio.writeByte(opcode);
	CUInt128 id;
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 
	prefs->getClientID(&id);
	bio.writeUInt128(id);
	bio.writeUInt32(prefs->getIPAddress());
	bio.writeUInt16(prefs->getUDPPort());
	bio.writeUInt16(prefs->getTCPPort());
	bio.writeByte(0);
//	CKademlia::debugMsg("Sent UDP OpCode MyDetails (%u)", opcode);
	sendPacket(packet, sizeof(packet), ip, port);
}

void CKademliaUDPListener::firewalledCheck(const uint32 ip, const uint16 port)
{
	byte packet[4];
	CByteIO bio(packet, sizeof(packet));
	bio.writeByte(OP_KADEMLIAHEADER);
	bio.writeByte(KADEMLIA_FIREWALLED_REQ);
	CPrefs *prefs = CKademlia::getPrefs();
	ASSERT(prefs != NULL); 
	bio.writeUInt16(prefs->getTCPPort());
//	CKademlia::debugMsg("Sent UDP OpCode KADEMLIA_FIREWALLED_REQ (%u)", ip);
	sendPacket(packet, sizeof(packet), ip, port);
}

void CKademliaUDPListener::sendNullPacket(byte opcode,uint32 ip, uint16 port)
{
	byte packet[2];
	CByteIO bio(packet, sizeof(packet));
	bio.writeByte(OP_KADEMLIAHEADER);
	bio.writeByte(opcode);
//	CKademlia::debugMsg("Sent UDP OpCode NullPacket (%u)", ip);
	sendPacket(packet, sizeof packet, ip, port);
}

void CKademliaUDPListener::publishPacket(const uint32 ip, const uint16 port, const Kademlia::CUInt128 &targetID, const Kademlia::CUInt128 &contactID, const TagList& tags)
{
	byte packet[1024];
	CByteIO bio(packet, sizeof(packet));
	bio.writeByte(OP_KADEMLIAHEADER);
	bio.writeByte(KADEMLIA_PUBLISH_REQ);
	bio.writeUInt128(targetID);
	//This is for future use.. This is the count of items to publish within the packet.
	//This way if you have several items with the same key, you can send them all in one packet.
	//This will be even greater use when compression is added..
	bio.writeUInt16(1); //TODO: OPT???
	bio.writeUInt128(contactID);
	bio.writeTagList(tags);
	uint32 len = sizeof(packet) - bio.getAvailable();
	sendPacket(packet, len,  ip, port);
}

LRESULT CKademliaUDPListener::OnMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_KADEMLIA_FIREWALLED_ACK)
	{
		KADEMLIAFIREWALLEDACK* fwack = (KADEMLIAFIREWALLEDACK*)lParam;
		sendNullPacket(KADEMLIA_FIREWALLED_ACK, fwack->ip, fwack->port);
		delete fwack;
		return 1;
	}

	CKademlia::debugMsg("*** %s; unknown message=0x%04x  wParam=%08x  lParam=%08x\n", __FUNCTION__, message, wParam, lParam);
	return 0;
}

void CKademliaUDPListener::processPacket( byte *data, uint32 lenData, const sockaddr_in *senderAddress)
{
	try
	{
		CPrefs *prefs = CKademlia::getPrefs();
		ASSERT(prefs != NULL); 
		prefs->setLastContact();

		byte opcode = data[1];
		const byte *packetData = data + 2;
		uint32 lenPacket = lenData - 2;

//		CKademlia::debugMsg("Processing UDP Packet from %s port %ld : opcode length %ld", inet_ntoa(senderAddress->sin_addr), ntohs(senderAddress->sin_port), lenPacket);
//		CMiscUtils::debugHexDump(packetData, lenPacket);

		switch (opcode)
		{
			case KADEMLIA_BOOTSTRAP_REQ:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_BOOTSTRAP_REQ (%u)", ntohl(senderAddress->sin_addr.s_addr));
				processBootstrapRequest(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_BOOTSTRAP_RES:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_BOOTSTRAP_RES (%u)", ntohl(senderAddress->sin_addr.s_addr));
				processBootstrapResponse(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_HELLO_REQ:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_HELLO_REQ (%u)", ntohl(senderAddress->sin_addr.s_addr));
				processHelloRequest(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_HELLO_RES:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_HELLO_RES_ACK (%u)", ntohl(senderAddress->sin_addr.s_addr));
				processHelloResponse(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_REQ:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_REQ (%u)", ntohl(senderAddress->sin_addr.s_addr));
				processKademliaRequest(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_RES:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_RES (%u)", ntohl(senderAddress->sin_addr.s_addr));
				processKademliaResponse(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_SEARCH_REQ:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_SEARCH_REQ (%u)", ntohl(senderAddress->sin_addr.s_addr));
//				CMiscUtils::debugHexDump(packetData, lenPacket);
				processSearchRequest(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_SEARCH_RES:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_SEARCH_RES (%u)", ntohl(senderAddress->sin_addr.s_addr));
				processSearchResponse(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_PUBLISH_REQ:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_PUBLISH_REQ (%u)", ntohl(senderAddress->sin_addr.s_addr));
				processPublishRequest(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_PUBLISH_RES:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_PUBLISH_RES (%u)", ntohl(senderAddress->sin_addr.s_addr));
				processPublishResponse(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_FIREWALLED_REQ:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_FIREWALED_REQ_REQ (%u)", ntohl(senderAddress->sin_addr.s_addr));
				processFirewalledRequest(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_FIREWALLED_RES:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_FIREWALLED_RES (%u)", ntohl(senderAddress->sin_addr.s_addr));
				processFirewalledResponse(packetData, lenPacket, senderAddress);
				break;
			case KADEMLIA_FIREWALLED_ACK:
//				CKademlia::debugMsg("Handled UDP OpCode KADEMLIA_FIREWALLED_ACK (%u)", ntohl(senderAddress->sin_addr.s_addr));
				this->processFirewalledResponse2(packetData, lenPacket, senderAddress);
				break;
			default:
				CKademlia::debugMsg("*************************");
				CKademlia::debugMsg("Unhandled UDP OpCode 0x%02X (%u)", opcode, ntohl(senderAddress->sin_addr.s_addr));
				CKademlia::debugMsg("*************************");
		}
	} 
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processPacket");
	}
}

void CKademliaUDPListener::addContact(const byte *data, const uint32 lenData, const uint32 ip, const uint16 port, uint16 tport)
{
	try
	{
		CByteIO bio(data, lenData);
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 
		CUInt128 id;
		bio.readUInt128(&id);
		bio.readUInt32();
		bio.readUInt16();
		if( tport )
			bio.readUInt16();
		else
			tport = bio.readUInt16();
		byte type = bio.readByte();
		CString key;
		id.toHexString(&key);
		// Look for existing client
		CContact *contact = routingZone->getContact(id);
		if (contact != NULL)
		{
			contact->setIPAddress(ip);
			contact->setUDPPort(port);
			contact->setTCPPort(tport);
			Kademlia::CKademlia::reportContactRef(contact);
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
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::addContact (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::addContact");
	}
}

void CKademliaUDPListener::addContacts(const byte *data, const uint32 lenData, const uint16 numContacts)
{
	try
	{
		CByteIO bio(data, lenData);
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 
		CUInt128 id;
		for (uint16 i=0; i<numContacts; i++)
		{
			bio.readUInt128(&id);
			uint32 ip = bio.readUInt32(); // Not LE
			uint16 port = bio.readUInt16();
			uint16 tport = bio.readUInt16();
			byte type = bio.readByte();
			if(::IsGoodIPPort(ntohl(ip),port))
			{
				routingZone->add(id, ip, port, tport, type);
			}
		}
	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::addContacts (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::addContacts");
	}
}

//KADEMLIA_BOOTSTRAP_REQ
void CKademliaUDPListener::processBootstrapRequest (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		// Verify packet is expected size
		if (lenPacket != 25){
			ASSERT(0);
			return;
		}

		//Used pointers.
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 
		CPrefs *prefs = CKademlia::getPrefs();
		ASSERT(prefs != NULL); 

		// Add the sender to the list of contacts
		addContact(packetData, lenPacket, ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));

		// Get some contacts to return
		ContactList contacts;
		uint16 numContacts = 1 + (uint16)routingZone->getBootstrapContacts(&contacts, 20);

		// Create response packet
		uint32 lenResponse = 4 + numContacts * 25;
		byte *response = new byte[lenResponse];
		CByteIO bio(response, lenResponse);
		CUInt128 id;

		// Write packet info
		bio.writeByte(OP_KADEMLIAHEADER);
		bio.writeByte(KADEMLIA_BOOTSTRAP_RES);
		bio.writeUInt16(numContacts);//TODO: OPT??? ... (16+4+2+2+1)*255 = 6375 max.
		CContact *contact;
		ContactList::const_iterator it;
		for (it = contacts.begin(); it != contacts.end(); it++)
		{
			contact = *it;
			contact->getClientID(&id);
			bio.writeUInt128(id);
			bio.writeUInt32(contact->getIPAddress()); // Not LE
			bio.writeUInt16(contact->getUDPPort());
			bio.writeUInt16(contact->getTCPPort());
			bio.writeByte(contact->getType());
		}
		prefs->getClientID(&id);
		bio.writeUInt128(id);
		bio.writeUInt32(prefs->getIPAddress()); // Not LE
		bio.writeUInt16(prefs->getUDPPort());
		bio.writeUInt16(prefs->getTCPPort());
		bio.writeByte(0);

		// Send response
//		CKademlia::debugMsg("Sent UDP OpCode KADEMLIA_BOOTSTRAP_RES (%u)", ntohl(senderAddress->sin_addr.s_addr));
		sendPacket(response, lenResponse, ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));

		// Finished with memory
		delete [] response;
	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processBootstrapRequest (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processBootstrapRequest");
	}
}

//KADEMLIA_BOOTSTRAP_RES
void CKademliaUDPListener::processBootstrapResponse (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		// Verify packet is expected size
		if (lenPacket < 27){
			ASSERT(0);
			return;
		}

		//Used Pointers
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 

		// How many contacts were given
		CByteIO bio(packetData, lenPacket);
		uint16 numContacts = bio.readUInt16();

		// Verify packet is expected size
		if (lenPacket != (2 + 25*numContacts))
			return;

		// Add these contacts to the list.
		addContacts(packetData+2, lenPacket-2, numContacts);
		// Send sender to alive.
		routingZone->setAlive(ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));

	}
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processBootstrapResponse (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processBootstrapResponse");
	}
}

//KADEMLIA_HELLO_REQ
void CKademliaUDPListener::processHelloRequest (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		if (lenPacket != 25){
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processHelloRequest");
			return;
		}

		//Used Pointers
		CPrefs *prefs = CKademlia::getPrefs();
		ASSERT(prefs != NULL); 

		// Add the sender to the list of contacts
		addContact(packetData, lenPacket, ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));

		// Send response
//		CKademlia::debugMsg("Sent UDP OpCode KADEMLIA_HELLO_RES (%u)", ntohl(senderAddress->sin_addr.s_addr));
		sendMyDetails(KADEMLIA_HELLO_RES,ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));
		if(prefs->getRecheckIP())
		{
			// Check if firewalled
			firewalledCheck(ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));
		}	
	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processHelloRequest (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processHelloRequest");
	}
}

//KADEMLIA_HELLO_RES
void CKademliaUDPListener::processHelloResponse (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		if (lenPacket != 25){
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processHelloResponse");
			return;
		}
	
		//Used Pointers
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 
	
		// Add or Update contact.
		addContact(packetData, lenPacket, ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));

		// Set contact to alive.
		routingZone->setAlive(ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));
	
	}
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processHelloResponset (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processHelloResponse");
	}
}
//KADEMLIA_REQ
void CKademliaUDPListener::processKademliaRequest (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		// Verify packet is expected size
		if (lenPacket != 33)
		{
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processKademliaRequest");
			return;
		}

		//Used Pointers
		CPrefs *prefs = CKademlia::getPrefs();
		ASSERT(prefs != NULL); 
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 

		//RecheckIP and firewall status
		if(prefs->getRecheckIP())
		{
			firewalledCheck(ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));
			sendMyDetails(KADEMLIA_HELLO_REQ, ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));
		}

		// Get target and type
		CByteIO bio(packetData, lenPacket);
		byte type = bio.readByte();
//		bool flag1 = (type >> 6); //Reserved
//		bool flag2 = (type >> 7); //Reserved
//		bool flag3 = (type >> 8); //Reserved

		type = type & 0x1F;
		if( type == 0 )
		{
			CKademlia::debugLine("Recieved wrong type packet in CKademliaUDPListener::processKademliaRequest");
			return;
		}

		//This is the target node trying to be found.
		CUInt128 target;
		bio.readUInt128(&target);
		CUInt128 distance(prefs->getClientID());
		distance.xor(target);

		//This makes sure we are not mistaken identify. Some client may have fresh installed and have a new hash.
		CUInt128 check;
		bio.readUInt128(&check);
		if( prefs->getClientID().compareTo(check))
			return;

		// Get required number closest to target
		ContactMap results;
		routingZone->getClosestTo(0, distance, (int)type, &results);
		uint16 count = (uint16)results.size();

		// Write response
		uint32 lenResponse = 1+1+16+1 + (16+4+2+2+1)*count;
		byte response[1024];
		ASSERT(lenResponse <= sizeof(response));
		CByteIO bio2(response, lenResponse);
		bio2.writeByte(OP_KADEMLIAHEADER);
		bio2.writeByte(KADEMLIA_RES);
		bio2.writeUInt128(target);
		bio2.writeByte((byte)count);
		CContact *c;
		CUInt128 id;
		ContactMap::const_iterator it;
		for (it = results.begin(); it != results.end(); it++)
		{
			c = it->second;
			c->getClientID(&id);
			bio2.writeUInt128(id);
			bio2.writeUInt32(c->getIPAddress());
			bio2.writeUInt16(c->getUDPPort());
			bio2.writeUInt16(c->getTCPPort());
			bio2.writeByte(c->getType());
		}

//		CKademlia::debugMsg("Sent UDP OpCode KADEMLIA_RES (%u)", ntohl(senderAddress->sin_addr.s_addr));
		sendPacket(response, lenResponse, ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));

	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processKademliaRequest (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processKademliaRequest");
	}
}

//KADEMLIA_RES
void CKademliaUDPListener::processKademliaResponse (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		// Verify packet is expected size
		if (lenPacket < 17){
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processKademliaResponse");
			return;
		}

		//Used Pointers
		CPrefs *prefs = CKademlia::getPrefs();
		ASSERT(prefs != NULL); 
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 

		if(prefs->getRecheckIP())
		{
			firewalledCheck(ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));
			sendMyDetails(KADEMLIA_HELLO_REQ, ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));
		}

		// What search does this relate to
		CByteIO bio(packetData, lenPacket);
		CUInt128 target;
		bio.readUInt128(&target);
		uint16 numContacts = bio.readByte();

		// Verify packet is expected size
		if (lenPacket != 16+1 + (16+4+2+2+1)*numContacts){
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processKademliaResponse");
			return;
		}

		// Set contact to alive.
		routingZone->setAlive(ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));

		CUInt128 id;
		ContactList *results = new ContactList;
		for (uint16 i=0; i<numContacts; i++)
		{
			bio.readUInt128(&id);
			uint32 ip = bio.readUInt32(); // Not LE
			uint16 port = bio.readUInt16();
			uint16 tport = bio.readUInt16();
			byte type = bio.readByte();
			if(::IsGoodIPPort(ntohl(ip),port))
			{
				routingZone->add(id, ip, port, tport, type);
				results->push_back(new CContact(id, ip, port, tport, type, target));
			}
		}
		CSearchManager::processResponse(target, 
										ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port), 
										results);
	}
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processKademliaResponse (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processKademliaResponse");
	}
}

void Free(SSearchTerm* pSearchTerms)
{
	if (pSearchTerms->left)
		Free(pSearchTerms->left);
	if (pSearchTerms->right)
		Free(pSearchTerms->right);
	delete pSearchTerms;
}

SSearchTerm* CreateSearchExpressionTree(CByteIO& bio, int iLevel)
{
	// the max. depth has to match our own limit for creating the search expression 
	// (see also 'ParsedSearchExpression' and 'GetSearchPacket')
	if (iLevel >= 16){
		CKademlia::debugLine("*** Search expression tree exceeds depth limit!");
		return NULL;
	}
	iLevel++;

	uint8 op = bio.readByte();
	if (op == 0x00){
		uint8 boolop = bio.readByte();
		if (boolop == 0x00){ // AND
			SSearchTerm* pSearchTerm = new SSearchTerm;
			pSearchTerm->type = SSearchTerm::AND;
			CKademlia::debugLine(" AND");
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
		else if (boolop == 0x01){ // OR
			SSearchTerm* pSearchTerm = new SSearchTerm;
			pSearchTerm->type = SSearchTerm::OR;
			CKademlia::debugLine(" OR");
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
		else if (boolop == 0x02){ // NAND
			SSearchTerm* pSearchTerm = new SSearchTerm;
			pSearchTerm->type = SSearchTerm::NAND;
			CKademlia::debugLine(" NAND");
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
			CKademlia::debugMsg("*** Unknown boolean search operator 0x%02x (CreateSearchExpressionTree)", boolop);
			return NULL;
		}
	}
	else if (op == 0x01){ // String
		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = SSearchTerm::String;
		uint16 len = bio.readUInt16();
		pSearchTerm->str = new CString;
		bio.readArray(pSearchTerm->str->GetBuffer(len), len);
		pSearchTerm->str->ReleaseBuffer(len);
		CKademlia::debugMsg(" \"%s\"", *(pSearchTerm->str));
		return pSearchTerm;
	}
	else if (op == 0x02){ // Meta tag
		SSearchTerm* pSearchTerm = new SSearchTerm;
		pSearchTerm->type = SSearchTerm::MetaTag;

		// read tag value
		CString strValue;
		uint16 lenValue = bio.readUInt16();
		bio.readArray(strValue.GetBuffer(lenValue), lenValue);
		strValue.ReleaseBuffer(lenValue);

		// read tag name
		CString strTagName;
		uint16 lenTagName = bio.readUInt16();
		bio.readArray(strTagName.GetBuffer(lenTagName), lenTagName);
		strTagName.ReleaseBuffer(lenTagName);

		pSearchTerm->tag = new Kademlia::CTagStr(strTagName, strValue);
		if (lenTagName == 1)
			CKademlia::debugMsg(" Tag%02x=\"%s\"", strTagName[0], strValue);
		else
			CKademlia::debugMsg(" Tag\"%s\"=\"%s\"", strTagName, strValue);
		return pSearchTerm;
	}
	else if (op == 0x03){ // Min/Max
		SSearchTerm* pSearchTerm = new SSearchTerm;

		// read tag value
		uint32 uValue = bio.readUInt32();

		// read min/max operator
		uint8 mmop = bio.readByte();
		pSearchTerm->type = (mmop == 0x01) ? SSearchTerm::Min : SSearchTerm::Max;

		// read tag name
		CString strTagName;
		uint16 lenTagName = bio.readUInt16();
		bio.readArray(strTagName.GetBuffer(lenTagName), lenTagName);
		strTagName.ReleaseBuffer(lenTagName);

		pSearchTerm->tag = new Kademlia::CTagUInt32(strTagName, uValue);
		if (lenTagName == 1)
			CKademlia::debugMsg(" %s(Tag%02x)=%u", pSearchTerm->type == SSearchTerm::Min ? "Min" : "Max", strTagName[0], uValue);
		else
			CKademlia::debugMsg(" %s(Tag\"%s\")=%u", pSearchTerm->type == SSearchTerm::Min ? "Min" : "Max", strTagName, uValue);
		return pSearchTerm;
	}
	else{
		CKademlia::debugMsg("*** Unknown search op=0x%02x (CreateSearchExpressionTree)", op);
		return NULL;
	}
}

//KADEMLIA_SEARCH_REQ
void CKademliaUDPListener::processSearchRequest (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		if (lenPacket < 17){
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processSearchRequest");
			return;
		}

		//Used Pointers
		CIndexed *indexed = CKademlia::getIndexed();
		ASSERT(indexed != NULL); 

		CByteIO bio(packetData, lenPacket);
		CUInt128 target;
		bio.readUInt128(&target);
		uint8 restrictive = bio.readByte();

		if(lenPacket == 17 ){
			if(restrictive)
			{
				//Source request
				indexed->SendValidResult(target, NULL, senderAddress, true );
			}
			else
			{
				//Single keyword request
				indexed->SendValidResult(target, NULL, senderAddress, false );
			}
		}
		else if(lenPacket > 17){

			SSearchTerm* pSearchTerms = NULL;
			if (restrictive){
				pSearchTerms = CreateSearchExpressionTree(bio, 0);
			}
			//Keyword request with added options.
			indexed->SendValidResult(target, pSearchTerms, senderAddress, false ); 
			Free(pSearchTerms);
		}
	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processSearchRequest (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processSearchRequest");
	}
}

//KADEMLIA_SEARCH_RES
void CKademliaUDPListener::processSearchResponse (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		// Verify packet is expected size
		if (lenPacket < 37){
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processSearchResponse");
			return;
		}

		//Used Pointers
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 

		// Set contact to alive.
		routingZone->setAlive(ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));

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
			TagList *tags = bio.readTagList();
			CSearchManager::processResult(	target, 
											ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port), 
											answer,
											tags);
			count--;
		}
	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processSearchResponse (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processSearchResponse");
	}
}

//KADEMLIA_PUBLISH_REQ
void CKademliaUDPListener::processPublishRequest (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	//There are different types of publishing..
	//Keyword and File are Stored..
	try
	{
		if (lenPacket < 37){
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processPublishRequest");
			return;
		}

		//Used Pointers
		CIndexed *indexed = CKademlia::getIndexed();
		ASSERT(indexed != NULL);
		CPrefs *prefs = CKademlia::getPrefs();
		ASSERT(prefs != NULL); 

		CByteIO bio(packetData, lenPacket);
		CUInt128 file;
		bio.readUInt128(&file);

		CUInt128 distance;
		prefs->getClientID(&distance);
		distance.xor(file);

		if(distance.get32BitChunk(0) > SEARCHTOLERANCE)
//		{
//			CKademlia::debugLine("Dropped index");
			return;
//		}
//		else
//			CKademlia::debugLine("Insert index");

		uint16 count = bio.readUInt16();
		bool flag = false;
		while( count > 0 )
		{
			CUInt128 target;
			bio.readUInt128(&target);
	
			Kademlia::CEntry* entry = new Kademlia::CEntry();
			entry->ip = ntohl(senderAddress->sin_addr.s_addr);
			entry->udpport = ntohs(senderAddress->sin_port);
			entry->keyID.setValue(file);
			entry->sourceID.setValue(target);
			//uint32 tags = bio.readUInt32LE();
			uint32 tags = bio.readByte();
			while(tags > 0)
			{
				CTag* tag = bio.readTag();
				if (!tag->m_name.Compare(TAG_SOURCETYPE) && tag->m_type == 9)
				{
					if( entry->source == false )
					{
						entry->taglist.push_back(new CTagUInt32(TAG_SOURCEIP, entry->ip));
						entry->taglist.push_back(new CTagUInt16(TAG_SOURCEUPORT, entry->udpport));
					}
					entry->source = true;
				}
				
				if (!tag->m_name.Compare(TAG_NAME)){
					entry->fileName = tag->GetStr();
					// NOTE: always add the 'name' tag, even if it's stored separately in 'fileName'. the tag is still needed for answering search request
					entry->taglist.push_back(tag);
				}
				else if (!tag->m_name.Compare(TAG_SIZE)){
					entry->size = tag->GetInt();
					// NOTE: always add the 'size' tag, even if it's stored separately in 'size'. the tag is still needed for answering search request
					entry->taglist.push_back(tag);
				}
				else if (!tag->m_name.Compare(TAG_SOURCEPORT)){
					entry->tcpport = tag->GetInt();
					entry->taglist.push_back(tag);
				}
				else
				{
					//TODO: Filter tags
					entry->taglist.push_back(tag);
				}
				tags--;
			}
			if( !entry->source && (entry->fileName == "" || entry->size == 0 ))
			{
				delete entry;
				entry = NULL;
				CKademlia::debugLine("Invalid entry CKademliaUDPListener::processPublishRequest");
			}
			else
			{
				if(	indexed->IndexedAdd(file, target, entry) )
				{
					flag = true;
				}
				else
				{
					CKademlia::debugMsg("INDEX FULL Source %u Keyword %u (CKademliaUDPListener::processPublishRequest)", indexed->m_totalIndexSource, indexed->m_totalIndexKeyword);
					delete entry;
					entry = NULL;
				}
			}
			count--;
		}	
//		CKademlia::debugMsg("Sent UDP OpCode KADEMLIA_PUBLISH_RES (%u)", ntohl(senderAddress->sin_addr.s_addr));
		if( flag )
		{
			byte response[18];
			CByteIO bio2(response, sizeof(response));
			bio2.writeByte(OP_KADEMLIAHEADER);
			bio2.writeByte(KADEMLIA_PUBLISH_RES);
			bio2.writeUInt128(file);
			sendPacket(response, sizeof(response), ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));
		}
	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processPublishRequest (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processPublishRequest");
	}
}

//KADEMLIA_PUBLISH_ACK
void CKademliaUDPListener::processPublishResponse (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		if (lenPacket != 16){
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processPublishResponse");
			return;
		}

		//Used Pointers
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 

		// Set contact to alive.
		routingZone->setAlive(ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));

		CByteIO bio(packetData, lenPacket);
		CUInt128 file;
		bio.readUInt128(&file);
		CSearchManager::processPublishResult(file);
	}
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processPublishResponse (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processPublishResponse");
	}
}
//KADEMLIA_FIREWALLED_REQ
void CKademliaUDPListener::processFirewalledRequest (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		if (lenPacket != 2){
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processFirewalledRequest");
			return;
		}

		CByteIO bio(packetData, lenPacket);
		uint16 tcpport = bio.readUInt16();
		uint16 udpport = ntohs(senderAddress->sin_port);
		uint32 ip = ntohl(senderAddress->sin_addr.s_addr);

		CContact* contact = new CContact();
		contact->setIPAddress(ip);
		contact->setTCPPort(tcpport);
		contact->setUDPPort(udpport);
		Kademlia::CKademlia::reportRequestTcp(contact);

		// Send response
		byte response[6];
		CByteIO bio2(response, sizeof(response));
		bio2.writeByte(OP_KADEMLIAHEADER);
		bio2.writeByte(KADEMLIA_FIREWALLED_RES);
		bio2.writeUInt32(ip);
//		CKademlia::debugMsg("Sent UDP OpCode KADEMLIA_FIREWALLED_RES (%u)", ip);
		sendPacket(response, sizeof(response), ip, udpport);
	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processFirewalledRequest (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processFirewalledRequest");
	}
}

//KADEMLIA_FIREWALLED_RES
void CKademliaUDPListener::processFirewalledResponse (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		if (lenPacket != 4){
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processFirewalledResponse");
			return;
		}

		//Used Pointers
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 

		// Set contact to alive.
		routingZone->setAlive(ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));

		CByteIO bio(packetData, lenPacket);
		uint32 ip = bio.readUInt32();
		CPrefs *prefs = CKademlia::getPrefs();
		ASSERT(prefs != NULL); 
		prefs->setIPAddress(ip);
		CString ipStr;
		CMiscUtils::ipAddressToString(ip, &ipStr);
	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processFirewalledResponse (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processFirewalledResponse");
	}
}

//KADEMLIA_FIREWALLED_ACK
void CKademliaUDPListener::processFirewalledResponse2 (const byte *packetData, const uint32 lenPacket, const sockaddr_in *senderAddress)
{
	try
	{
		if (lenPacket != 0){
			CKademlia::debugLine("Recieved wrong size packet in CKademliaUDPListener::processFirewalledResponse2");
			return;
		}

		//Used Pointers
		CPrefs *prefs = CKademlia::getPrefs();
		ASSERT(prefs != NULL); 
		CRoutingZone *routingZone = CKademlia::getRoutingZone();
		ASSERT(routingZone != NULL); 

		// Set contact to alive.
		routingZone->setAlive(ntohl(senderAddress->sin_addr.s_addr), ntohs(senderAddress->sin_port));

		prefs->incFirewalled();
	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CKademliaUDPListener::processFirewalledResponse2 (IO error(%i))", ioe->m_cause);
		ioe->Delete();
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CKademliaUDPListener::processFirewalledResponse2");
	}
}