//#pragma once
#include "stdafx.h"
#include "WebCacheOHCBManager.h"
#include "PartFile.h"
#include "UpDownClient.h"
#include "otherfunctions.h"
#include "opcodes.h"

CWebCacheOHCBManager WC_OHCBManager;	// global

CWebCacheOHCBManager::CWebCacheOHCBManager()
{
	lastCleanupTime = GetTickCount();
}

CWebCacheOHCBManager::~CWebCacheOHCBManager()
{
	if (managedOHCBList.IsEmpty())
		return;
	POSITION pos = managedOHCBList.GetHeadPosition();
	while (pos)
	{
		delete managedOHCBList.GetNext(pos);
		managedOHCBList.RemoveHead();
	}
}

POSITION CWebCacheOHCBManager::AddWCBlock(
									  uint32 proxyIP,
									  uint32 clientIP,
									  uint16 clientPort,
									  const byte* fileHash,
									  uint32 startOffset,
									  uint32 endOffset,
									  byte* key)
{
	CleanupOHCBListIfNeeded();

	CManagedOHCB* newOHCB = new CManagedOHCB(
											proxyIP,
											clientIP,
											clientPort,
											fileHash,
											startOffset,
											endOffset,
											key);
	managedOHCBList.AddTail(newOHCB);
	return managedOHCBList.GetTailPosition();
}

/*
////////////////////////////////////////////////////////////////////////////////////
//	adds packed OHCBs to the CSafeMemFile
//	format:
//	
//	starts with <nrOfBlockPackets 2>, this is the number of packets of packed blocks; it continues with:
//
//	<proxyIP 4><clientIP 4><clientPort 2><fileHash 16><startOffset 4><endOffset 4><key 16><packedOHCBs 1><...>
//
//	the last one says how many OHCBs are packed in the following data;
//	those OHCBs have same proxyIP, clientIP, clientPort and the fileHash and therefore we only need to send
//
//	<startOffset 4><endOffset 4><key 16><startOffset 4><endOffset 4><key 16><startOffset 4><...>
//	
//	this saves 26 bytes with every packed OHCB

void CWebCacheOHCBManager::GetWCBlocksForClient(CSafeMemFile* data, CUpDownClient* recipient)
{
	PurgeOHCBListIfNeeded();

	ULONGLONG dataStart = data->GetPosition();
	data->WriteUInt32(0);	// we will return here if we have something to send and change this number
	uint32 nrOfBlockPackets = 0;

	CToSendOHCBList toSend;

	const byte* reqfileHash = recipient->GetUploadFileID();	// the file that this client is requesting
	POSITION pos = managedOHCBList.GetHeadPosition();
	while (pos != NULL)	// OHCB search loop
	{
		CManagedOHCB* currentOHCB = managedOHCBList.GetAt(pos);
		if (!md4cmp(currentOHCB->fileHash, reqfileHash)	// same file hash
			&& !recipient->IsPartAvailable( currentOHCB->startOffset / PARTSIZE ))	// missing this chunk
		{
			POSITION pos2 = currentOHCB->recipients.GetHeadPosition();
			const byte* userHash = recipient->GetUserHash();
			bool alreadySent = false;
			while (pos2 != NULL && !alreadySent)	// client search loop, we check if we have already sent this OHCB to this client
			{
				if (!md4cmp(userHash, currentOHCB->recipients.GetAt(pos2)))	// found this client in the list
					alreadySent = true;
				currentOHCB->recipients.GetNext(pos2);
			}

			if (!alreadySent)	// this client has not received this OHCB yet, but now it's time
			{
				byte hashToAdd[16];
				md4cpy(hashToAdd, userHash);
				currentOHCB->recipients.AddTail(hashToAdd);	// add this user to the recipient list

				// add this OHCB to the toSend-list
				POSITION pos3 = toSend.GetHeadPosition();
				bool foundSimilarOHCBs = false;
				while (pos3 != NULL && !foundSimilarOHCBs)
				{
					if (toSend.GetAt(pos3)->clientIP == currentOHCB->clientIP
						&& toSend.GetAt(pos3)->proxyIP == currentOHCB->proxyIP
						&& toSend.GetAt(pos3)->clientPort == currentOHCB->clientPort
						&& !md4cmp(toSend.GetAt(pos3)->fileHash, currentOHCB->fileHash))
                        foundSimilarOHCBs = true;
					if (!foundSimilarOHCBs)
						toSend.GetNext(pos3);
				}

				if (foundSimilarOHCBs)
				{
					toSend.GetAt(pos3)->compressibleList->AddTail(currentOHCB);
					toSend.GetAt(pos3)->listLength++;
				}
				else
				{
					toSend.AddTail(new CToSendOHCBListMember(currentOHCB->proxyIP, currentOHCB->clientIP, currentOHCB->clientPort, currentOHCB->fileHash));
					toSend.GetTail()->compressibleList->AddTail(new CManagedOHCB(currentOHCB->proxyIP, currentOHCB->clientIP, currentOHCB->clientPort, currentOHCB->fileHash, currentOHCB->startOffset, currentOHCB->endOffset, currentOHCB->key));
				}
			}
		}
		managedOHCBList.GetNext(pos);
	}

	if (!toSend.IsEmpty())	// we've got something to send
	{
		POSITION pos4 = toSend.GetHeadPosition();
		do
		{
			CToSendOHCBListMember* currentNode = toSend.GetAt(pos4);
			CManagedOHCBList* currentOHCBList = currentNode->compressibleList;	// list with similar OHCBs
			uint16 nrOfSimilarOHCBs = currentNode->listLength;
			
			POSITION pos5 = currentOHCBList->GetHeadPosition();
			do
			{
				CManagedOHCB* currentOHCB = currentOHCBList->GetAt(pos5);
				nrOfBlockPackets++;
				if (thePrefs.WebCacheIsTransparent())
					data->WriteUInt32( 0 );
				else
					data->WriteUInt32( currentOHCB->proxyIP	);
				data->WriteUInt32( currentOHCB->clientIP	);
				data->WriteUInt16( currentOHCB->clientPort	);
				data->WriteHash16( currentOHCB->fileHash	);
				data->WriteUInt32( currentOHCB->startOffset	);
				data->WriteUInt32( currentOHCB->endOffset	);
				data->Write( currentOHCB->key, WC_KEYLENGTH	);
				nrOfSimilarOHCBs--;
				currentOHCBList->GetNext(pos5);

				byte nrOfOHCBsToPack;	// number of OHCBs that will be packed at this run, up to 255
				nrOfOHCBsToPack = nrOfSimilarOHCBs > 255 ? 255 : nrOfSimilarOHCBs;
				nrOfSimilarOHCBs -= nrOfOHCBsToPack;
				data->WriteUInt8(nrOfOHCBsToPack);

				while (nrOfOHCBsToPack > 0)
				{
					currentOHCB = currentOHCBList->GetAt(pos5);
					data->WriteUInt32( currentOHCB->startOffset	);
					data->WriteUInt32( currentOHCB->endOffset	);
					data->Write( currentOHCB->key, WC_KEYLENGTH	);

					nrOfOHCBsToPack--;
					currentOHCBList->GetNext(pos5);
				}
			}
			while (nrOfSimilarOHCBs > 0);

			toSend.GetNext(pos4);
		}
		while (pos4 != NULL);

		// now write how many block packets we have made
		data->Seek(dataStart, CFile::begin);
		data->WriteUInt32(nrOfBlockPackets);
		data->SeekToEnd();
	}
}
*/

/*Packet* CWebCacheOHCBManager::GetWCBlocksForClient(CUpDownClient* recipient)
{
	CleanupOHCBListIfNeeded();

	if( !recipient->IsBehindOurWebCache()
		|| recipient->IsProxy()
		|| !recipient->m_bIsAcceptingOurOhcbs)
		return NULL;

	CManagedOHCBList toSend;

	const byte* reqfileHash = recipient->GetUploadFileID();	// the file that this client is requesting
	POSITION pos = managedOHCBList.GetHeadPosition();
	while (pos != NULL)	// OHCB search loop
	{
		//CManagedOHCB* currentOHCB = managedOHCBList.GetAt(pos);
		CManagedOHCB* currentOHCB = managedOHCBList.GetNext(pos);
		if (!md4cmp(currentOHCB->fileHash, reqfileHash)	// same file hash
			&& !recipient->IsPartAvailable( currentOHCB->startOffset / PARTSIZE ))	// missing this chunk
		{
			POSITION pos2 = currentOHCB->recipients.GetHeadPosition();
			const byte* userHash = recipient->GetUserHash();
			bool alreadySent = false;
			while (pos2 != NULL && !alreadySent)	// client search loop, we check if we have already sent this OHCB to this client
			{
				if (!md4cmp(userHash, currentOHCB->recipients.GetAt(pos2)))	// found this client in the list
					alreadySent = true;
				currentOHCB->recipients.GetNext(pos2);
			}

			if (!alreadySent)	// this client has not received this OHCB yet, but now it's time
			{
				byte* hashToAdd = new byte[16];
				md4cpy(hashToAdd, userHash);
				currentOHCB->recipients.AddTail(hashToAdd);	// add this user to the recipient list

				// add this OHCB to the toSend-list
				toSend.AddTail(currentOHCB);
			}
		}
		//managedOHCBList.GetNext(pos);
	}

	if (toSend.IsEmpty())
		return NULL;
	
	// we've got something to send

	CSafeMemFile data(5000);
	data.WriteUInt32(recipient->m_uWebCacheDownloadId);
	data.WriteUInt32(toSend.GetSize());
	POSITION pos3 = toSend.GetHeadPosition();
	do
	{
		CManagedOHCB* currentOHCB = toSend.GetAt(pos3);
		
		if (thePrefs.WebCacheIsTransparent())
			data.WriteUInt32( 0 );
		else
			data.WriteUInt32( currentOHCB->proxyIP	);
		data.WriteUInt32( currentOHCB->clientIP	);
		data.WriteUInt16( currentOHCB->clientPort	);
		data.WriteHash16( currentOHCB->fileHash	);
		data.WriteUInt32( currentOHCB->startOffset	);
		data.WriteUInt32( currentOHCB->endOffset	);
		data.Write( currentOHCB->key, WC_KEYLENGTH	);
		
		toSend.GetNext(pos3);
		toSend.RemoveHead();
	}
	while (pos3 != NULL);

	Packet* result = new Packet(&data, OP_WEBCACHEPROT);
	result->opcode = OP_MULTI_HTTP_CACHED_BLOCKS;

	uint32 unpackedSize = result->size;
	result->PackPacket();
	if (unpackedSize <= WC_OHCB_PACKET_SIZE*3 + 8	// check if we benefit from the compression; if we packed more than 3 OHCBs, we surely do
		&& result->size > unpackedSize)
		result->UnPackPacket(unpackedSize);

	return result;
}*/

Packet* CWebCacheOHCBManager::GetWCBlocksForClient(CUpDownClient* recipient)
{
	uint32 nrOfOHCBsInThePacket;
	return GetWCBlocksForClient(recipient, nrOfOHCBsInThePacket, NULL);
}

Packet* CWebCacheOHCBManager::GetWCBlocksForClient(CUpDownClient* recipient, uint32 &nrOfOHCBsInThePacket, POSITION OHCBpos)
{
	CleanupOHCBListIfNeeded();

	if( managedOHCBList.IsEmpty()
		|| !recipient->IsBehindOurWebCache()
		|| recipient->IsProxy()
		|| !recipient->m_bIsAcceptingOurOhcbs)
		return NULL;

	CSafeMemFile data(5000);
	data.WriteUInt32(0); // data.WriteUInt32(recipient->m_uWebCacheDownloadId); comes later here
	data.WriteUInt32(0); // data.WriteUInt32(nrOfOHCBsInThePacket); comes later here

	nrOfOHCBsInThePacket = 0;
	const byte* userHash = recipient->GetUserHash();

	if (OHCBpos)
	{
		CManagedOHCB* XpressOHCB = managedOHCBList.GetAt(OHCBpos);
		byte* hashToAdd = new byte[16];
		md4cpy(hashToAdd, userHash);
		XpressOHCB->recipients.AddTail(hashToAdd);	// add this user to the recipient list
		nrOfOHCBsInThePacket++;

		if (thePrefs.WebCacheIsTransparent())
			data.WriteUInt32( 0 );
		else
			data.WriteUInt32( XpressOHCB->proxyIP	);
		data.WriteUInt32( XpressOHCB->clientIP	);
		data.WriteUInt16( XpressOHCB->clientPort	);
		data.WriteHash16( XpressOHCB->fileHash	);
		data.WriteUInt32( XpressOHCB->startOffset	);
		data.WriteUInt32( XpressOHCB->endOffset	);
		data.Write( XpressOHCB->key, WC_KEYLENGTH	);
	}

	for (POSITION pos = managedOHCBList.GetHeadPosition(); pos;) // OHCB search loop
	{
		CManagedOHCB* currentOHCB = managedOHCBList.GetNext(pos);
		if (!recipient->IsPartAvailable(currentOHCB->startOffset / PARTSIZE, currentOHCB->fileHash))	// missing this chunk - WRONG file, TODO: write this for the requested file(s)
		{
			POSITION pos2 = currentOHCB->recipients.GetHeadPosition();
			bool alreadySent = false;
			while (pos2 != NULL && !alreadySent)	// client search loop, we check if we have already sent this OHCB to this client
			{
				if (!md4cmp(userHash, currentOHCB->recipients.GetAt(pos2)))	// found this client in the list
					alreadySent = true;
				currentOHCB->recipients.GetNext(pos2);
			}

			if (!alreadySent)	// this client has not received this OHCB yet, but now it's time
			{
				byte* hashToAdd = new byte[16];
				md4cpy(hashToAdd, userHash);
				currentOHCB->recipients.AddTail(hashToAdd);	// add this user to the recipient list
				nrOfOHCBsInThePacket++;
				
				if (thePrefs.WebCacheIsTransparent())
					data.WriteUInt32( 0 );
				else
					data.WriteUInt32( currentOHCB->proxyIP	);
				data.WriteUInt32( currentOHCB->clientIP	);
				data.WriteUInt16( currentOHCB->clientPort	);
				data.WriteHash16( currentOHCB->fileHash	);
				data.WriteUInt32( currentOHCB->startOffset	);
				data.WriteUInt32( currentOHCB->endOffset	);
				data.Write( currentOHCB->key, WC_KEYLENGTH	);
			}
		}
	}

	if (nrOfOHCBsInThePacket == 0)
		return NULL;
	
	data.SeekToBegin();
	data.WriteUInt32(recipient->m_uWebCacheDownloadId);
	data.WriteUInt32(nrOfOHCBsInThePacket);

	Packet* result = new Packet(&data, OP_WEBCACHEPROT);
	result->opcode = OHCBpos ? OP_XPRESS_MULTI_HTTP_CACHED_BLOCKS : OP_MULTI_HTTP_CACHED_BLOCKS;

	uint32 unpackedSize = result->size;
	result->PackPacket();
	if (unpackedSize <= WC_OHCB_PACKET_SIZE*3 + 8	// check if we benefit from the compression; if we packed more than 3 OHCBs, we surely do
		&& result->size > unpackedSize)
		result->UnPackPacket(unpackedSize);

	return result;
}

void CWebCacheOHCBManager::CleanupOHCBListIfNeeded()
{
	uint32 now = GetTickCount();
	if (now - lastCleanupTime < CLEANUPTIME)	// don't clean up too often, this function gets called on every occasion
		return;
	else
		lastCleanupTime = now;

	if (managedOHCBList.IsEmpty())
		return;

	uint32 maxCreationTime = now - MAX_OHCB_AGE;	// we will delete all OHCBs that are older than this
	POSITION toDelete;
	for (POSITION pos = managedOHCBList.GetHeadPosition(); pos;)
	{
		if (managedOHCBList.GetAt(pos)->creationTime < maxCreationTime)
		{
			delete managedOHCBList.GetAt(pos);
			toDelete = pos;
			managedOHCBList.GetNext(pos);
			managedOHCBList.RemoveAt(toDelete);
		}
		else
			managedOHCBList.GetNext(pos);
	}
}

bool CWebCacheOHCBManager::AddRecipient(POSITION OHCBpos, CUpDownClient* client)
{
	try
	{
		CManagedOHCB* currentOHCB = managedOHCBList.GetAt(OHCBpos);
		byte* hashToAdd = new byte[16];
		md4cpy(hashToAdd, client->GetUserHash());
		currentOHCB->recipients.AddTail(hashToAdd);
	}
	catch (...)
	{
		return false;
	}
	return true;
}