//This file is part of the eMule WebCache mod
//http://ispcachingforemule.de.vu
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
#include "WebCache.h"
#include "SafeFile.h"
#include "UpDownClient.h"
#include "otherfunctions.h"
#include "packets.h"

#define	CLEANUPTIME		MIN2MS(10)	// don't cleanup the list more often than once in 10 minutes to save CPU time
#define MAX_OHCB_AGE	MIN2MS(30)	// OHCBs older than this time will be removed from the list on cleanup
#define	OHCB_ASK_BONUS	MIN2MS(5)	// every OHCB reask gives it extra 5 minutes to live

typedef CTypedPtrList<CPtrList, byte*> CClientHashList;

class CManagedOHCB
{
public:
	CManagedOHCB(
		uint32 proxyIP,
		uint32 clientIP,
		uint16 clientPort,
		const byte* fileHash,
		uint32 startOffset,
		uint32 endOffset,
		byte* key)
	{
		this->proxyIP = proxyIP;
		this->clientIP = clientIP;
		this->clientPort = clientPort;
		memcpy(this->fileHash, fileHash, 16);
		this->startOffset = startOffset;
		this->endOffset = endOffset;
		memcpy(this->key, key, WC_KEYLENGTH);
		this->creationTime = GetTickCount();
	}

	~CManagedOHCB()
	{
		if (recipients.IsEmpty())
			return;

		for (POSITION pos = recipients.GetHeadPosition(); pos; recipients.GetNext(pos))
			delete[] recipients.GetAt(pos);
		recipients.RemoveAll();
	}
	uint32 proxyIP;
	uint32 clientIP;
	uint16 clientPort;
	byte fileHash[16];
	uint32 startOffset;
	uint32 endOffset;
	byte key[WC_KEYLENGTH];
	uint32 creationTime;	// tick at which the block has been initially loaded via the proxy
	CClientHashList recipients;	// list of client hashes that this OHCB has been already sent to
};

typedef CTypedPtrList<CPtrList, CManagedOHCB*> CManagedOHCBList;

/*class CToSendOHCBListMember	// used for comression
{
public:
	CToSendOHCBListMember(
		uint32 proxyIP,
		uint32 clientIP,
		uint16 clientPort,
		byte* fileHash)
	{
		this->proxyIP = proxyIP;
		this->clientIP = clientIP;
		this->clientPort = clientPort;
		md4cpy(this->fileHash, fileHash);
		listLength = 1;
	}
	uint32 proxyIP;
	uint32 clientIP;
	uint16 clientPort;
	byte fileHash[16];
	CManagedOHCBList* compressibleList;	// list of ManagedOHCBs that have same proxyIP, clientIP, clientPort and fileHash
	uint32 listLength;	// length of the compressibleList
};

typedef CTypedPtrList<CPtrList, CToSendOHCBListMember*> CToSendOHCBList;
*/

class CWebCacheOHCBManager
{
public:
	CWebCacheOHCBManager();
	~CWebCacheOHCBManager();
	POSITION AddWCBlock(	// adds a WCBlock with the parameters to the list
		uint32 proxyIP,
		uint32 clientIP,
		uint16 clientPort,
		const byte* fileHash,
		uint32 startOffset,
		uint32 endOffset,
		byte* key);
	Packet* GetWCBlocksForClient(CUpDownClient* recipient);
	Packet* GetWCBlocksForClient(CUpDownClient* recipient, uint32 &nrOfOHCBsInThePacket, POSITION OHCBpos);
	bool AddRecipient(POSITION OHCBpos, CUpDownClient* client);	// adds the client to the recepient list of this OHCB
	void CleanupOHCBListIfNeeded();	// removes old OHCBs from the list
private:
	CManagedOHCBList managedOHCBList;
	uint32 lastCleanupTime;
};

extern CWebCacheOHCBManager WC_OHCBManager;