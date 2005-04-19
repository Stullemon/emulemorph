//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "stdafx.h"
#include "emule.h"
#include "ClientUDPSocket.h"
#include "Packets.h"
#include "DownloadQueue.h"
#include "Statistics.h"
#include "PartFile.h"
#include "SharedFileList.h"
#include "UploadQueue.h"
#include "UpDownClient.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "SafeFile.h"
#include "ClientList.h"
#include "Listensocket.h"
#include <zlib/zlib.h>
#include "kademlia/kademlia/Kademlia.h"
#include "kademlia/net/KademliaUDPListener.h"
#include "kademlia/io/IOException.h"
#include "IPFilter.h"
#include "Log.h"

#include "FirewallOpener.h" // emulEspaña: Added by MoNKi [MoNKi: -Random Ports-]

// MORPH START - Added by Commander, WebCache 1.2e
#include "WebCache/WebCachedBlock.h" // yonatan http
#include "SafeFile.h" // yonatan http (for udp ohcbs)
#include "WebCache/WebCachedBlockList.h" // Superlexx - managed OHCB list
// MORPH END - Added by Commander, WebCache 1.2e

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


// CClientUDPSocket

CClientUDPSocket::CClientUDPSocket()
{
	m_bWouldBlock = false;
	m_port=0;
}

CClientUDPSocket::~CClientUDPSocket()
{
    theApp.uploadBandwidthThrottler->RemoveFromAllQueues(this); // ZZ:UploadBandWithThrottler (UDP)

	POSITION pos = controlpacket_queue.GetHeadPosition();
	while (pos){
		UDPPack* p = controlpacket_queue.GetNext(pos);
		delete p->packet;
		delete p;
	}
}

void CClientUDPSocket::OnReceive(int nErrorCode)
{
	if (nErrorCode)
	{
		if (thePrefs.GetVerbose())
			DebugLogError(_T("Error: Client UDP socket, error on receive event: %s"), GetErrorMessage(nErrorCode, 1));
	}

	BYTE buffer[5000];
	SOCKADDR_IN sockAddr = {0};
	int iSockAddrLen = sizeof sockAddr;
	int length = ReceiveFrom(buffer, sizeof buffer, (SOCKADDR*)&sockAddr, &iSockAddrLen);
	if (length >= 1 && !(theApp.ipfilter->IsFiltered(sockAddr.sin_addr.S_un.S_addr) || theApp.clientlist->IsBannedClient(sockAddr.sin_addr.S_un.S_addr)))
    {
		CString strError;
		try
		{
			switch (buffer[0])
			{
				case OP_EMULEPROT:
				{
					if (length >= 2)
						ProcessPacket(buffer+2, length-2, buffer[1], sockAddr.sin_addr.S_un.S_addr, ntohs(sockAddr.sin_port));
					else
						throw CString(_T("Packet too short"));
					break;
				}
				case OP_KADEMLIAPACKEDPROT:
				{
					theStats.AddDownDataOverheadKad(length);
					if (length >= 2)
					{
						uint32 nNewSize = length*10+300;
						byte* unpack = new byte[nNewSize];
						uLongf unpackedsize = nNewSize-2;
						uint16 result = uncompress(unpack+2, &unpackedsize, buffer+2, length-2);
						if (result == Z_OK)
						{
							unpack[0] = OP_KADEMLIAHEADER;
							unpack[1] = buffer[1];
							try
							{
								Kademlia::CKademlia::processPacket(unpack, unpackedsize+2, ntohl(sockAddr.sin_addr.S_un.S_addr), ntohs(sockAddr.sin_port));
							}
							catch(...)
							{
								delete[] unpack;
								throw;
							}
						}
						else
						{
							delete[] unpack;
							throw CString(_T("Failed to uncompress Kademlia packet"));
						}
						delete[] unpack;
					}
					else
						throw CString(_T("Packet too short"));
					break;
				}
				case OP_KADEMLIAHEADER:
				{
					theStats.AddDownDataOverheadKad(length);
					if (length >= 2)
						Kademlia::CKademlia::processPacket(buffer, length, ntohl(sockAddr.sin_addr.S_un.S_addr), ntohs(sockAddr.sin_port));
					else
						throw CString(_T("Packet too short"));
					break;
				}
//MORPH START - Added by SiRoB, WebCache 1.2f
// WebCache ////////////////////////////////////////////////////////////////////////////////////
				case OP_WEBCACHEPACKEDPROT:	// Superlexx - packed WC protocol
				{ // taken from above, update this code once the source is beautified
					if (length >= 2)
					{
						uint32 nNewSize = length*10+300;
						byte* unpack = new byte[nNewSize];
						uLongf unpackedsize = nNewSize-2;
						uint16 result = uncompress(unpack+2, &unpackedsize, buffer+2, length-2);
						if (result == Z_OK)
						{
							unpack[0] = OP_WEBCACHEPROT;
							unpack[1] = buffer[1];
							ProcessWebCachePacket(unpack+2, unpackedsize, unpack[1], sockAddr.sin_addr.S_un.S_addr, ntohs(sockAddr.sin_port));
						}
						else
						{
							delete[] unpack;
							throw CString(_T("Failed to uncompress WebCache packet"));
						}
						delete[] unpack;
					}
					else
						throw CString(_T("Packet too short"));
					break;
				}
				//JP WEBCACHE START
				case OP_WEBCACHEPROT:
				{
					if (length >= 2)
						ProcessWebCachePacket(buffer+2, length-2, buffer[1], sockAddr.sin_addr.S_un.S_addr, ntohs(sockAddr.sin_port));
					else
						throw CString(_T("Packet too short"));
					break;
				}
				//JP WEBCACHE END
// MORPH END   - Added by SiRoB, WebCache 1.2f
				default:
				{
					CString strError;
					strError.Format(_T("Unknown protocol %02x"), buffer[0]);
					throw strError;
				}
			}
		}
		catch(CFileException* error)
		{
			error->Delete();
			strError = _T("Invalid packet received");
		}
		catch(CMemoryException* error)
		{
			error->Delete();
			strError = _T("Memory exception");
		}
		catch(CString error)
		{
			strError = error;
		}
		catch(Kademlia::CIOException* error)
		{
			error->Delete();
			strError = _T("Invalid packet received");
		}
		catch(CException* error)
		{
			error->Delete();
			strError = _T("General packet error");
		}
		catch(...)
		{
			strError = _T("Unknown exception");
			ASSERT(0);
		}

		if (thePrefs.GetVerbose() && !strError.IsEmpty())
		{
			CString strClientInfo;
			CUpDownClient* client;
			if (buffer[0] == OP_EMULEPROT)
				client = theApp.clientlist->FindClientByIP_UDP(sockAddr.sin_addr.S_un.S_addr, ntohs(sockAddr.sin_port));
			else
				client = theApp.clientlist->FindClientByIP_KadPort(sockAddr.sin_addr.S_un.S_addr, ntohs(sockAddr.sin_port));
			if (client)
				strClientInfo = client->DbgGetClientInfo();
			else
				strClientInfo.Format(_T("%s:%u"), ipstr(sockAddr.sin_addr), ntohs(sockAddr.sin_port));

			DebugLogWarning(_T("Client UDP socket: prot=%02x  opcode=%02x  %s: %s"), buffer[0], buffer[1], strError, strClientInfo);
		}
    }
	else if (length == SOCKET_ERROR)
	{
		DWORD dwError = WSAGetLastError();
		if (dwError == WSAECONNRESET)
		{
			// Depending on local and remote OS and depending on used local (remote?) router we may receive
			// WSAECONNRESET errors. According some KB articels, this is a special way of winsock to report 
			// that a sent UDP packet was not received by the remote host because it was not listening on 
			// the specified port -> no eMule running there.
			//
			// TODO: So, actually we should to something with this information and drop the related Kad node 
			// or eMule client...
			;
		}
		if (thePrefs.GetVerbose() && dwError != WSAECONNRESET)
		{
				CString strClientInfo;
				if (iSockAddrLen > 0 && sockAddr.sin_addr.S_un.S_addr != 0 && sockAddr.sin_addr.S_un.S_addr != INADDR_NONE)
						strClientInfo.Format(_T(" from %s:%u"), ipstr(sockAddr.sin_addr), ntohs(sockAddr.sin_port));
			DebugLogError(_T("Error: Client UDP socket, failed to receive data%s: %s"), strClientInfo, GetErrorMessage(dwError, 1));
		}
	}
}

bool CClientUDPSocket::ProcessPacket(BYTE* packet, uint16 size, uint8 opcode, uint32 ip, uint16 port)
{
	switch(opcode)
	{
		case OP_REASKCALLBACKUDP:
		{
			theStats.AddDownDataOverheadOther(size);
			CUpDownClient* buddy = theApp.clientlist->GetBuddy();
			if( buddy )
			{
				if( size < 17 || buddy->socket == NULL )
					break;
				uchar check[16];
				memcpy(&check, packet, 16);
				if( !memcmp(&check, buddy->GetBuddyID(), 16) )
				{
					memcpy(packet+10, &ip, 4);
					memcpy(packet+14, &port, 2);
					Packet* response = new Packet(OP_EMULEPROT);
					response->opcode = OP_REASKCALLBACKTCP;
					response->pBuffer = new char[size];
					memcpy(response->pBuffer, packet+10, size-10);
					response->size = size-10;
					buddy->socket->SendPacket(response);
				}
			}
			break;
		}
		case OP_REASKFILEPING:
		{
			theStats.AddDownDataOverheadFileRequest(size);
			CSafeMemFile data_in(packet, size);
			uchar reqfilehash[16];
			data_in.ReadHash16(reqfilehash);
			CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfilehash);
			if (!reqfile)
			{
				if (thePrefs.GetDebugClientUDPLevel() > 0)
				{
					DebugRecv("OP_ReaskFilePing", NULL, (char*)reqfilehash, ip);
					DebugSend("OP__FileNotFound", NULL);
				}

				Packet* response = new Packet(OP_FILENOTFOUND,0,OP_EMULEPROT);
				theStats.AddUpDataOverheadFileRequest(response->size);
				SendPacket(response, ip, port);
				break;
			}
			CUpDownClient* sender = theApp.uploadqueue->GetWaitingClientByIP_UDP(ip, port);
			if (sender)
			{
				if (thePrefs.GetDebugClientUDPLevel() > 0)
					DebugRecv("OP_ReaskFilePing", sender, (char*)reqfilehash, ip);

				//Make sure we are still thinking about the same file
				if (md4cmp(reqfilehash, sender->GetUploadFileID()) == 0)
				{
					sender->AddAskedCount();
					sender->SetLastUpRequest();
					//I messed up when I first added extended info to UDP
					//I should have originally used the entire ProcessExtenedInfo the first time.
					//So now I am forced to check UDPVersion to see if we are sending all the extended info.
					//For now on, we should not have to change anything here if we change
					//anything to the extended info data as this will be taken care of in ProcessExtendedInfo()
					//Update extended info. 
					if (sender->GetUDPVersion() > 3)
					{
						sender->ProcessExtendedInfo(&data_in, reqfile);
					}
					//Update our complete source counts.
					else if (sender->GetUDPVersion() > 2)
					{
						uint16 nCompleteCountLast= sender->GetUpCompleteSourcesCount();
						uint16 nCompleteCountNew = data_in.ReadUInt16();
						sender->SetUpCompleteSourcesCount(nCompleteCountNew);
						if (nCompleteCountLast != nCompleteCountNew)
						{
							//Morph Start - added by AndCycle, ICS
							if(reqfile->IsPartFile()) //  enkeyDEV: ICS
								((CPartFile*)reqfile)->NewSrcIncPartsInfo();
							//Morph End - added by AndCycle, ICS
							//MORPH START - Added by SiRoB, UpdatePartsInfo -Fix-
							if(reqfile->IsPartFile())
								((CPartFile*)reqfile)->UpdatePartsInfo();
							else
							//MORPH END   - Added by SiRoB, UpdatePartsInfo -Fix-
								reqfile->UpdatePartsInfo();
						}
					}
					CSafeMemFile data_out(128);
					if(sender->GetUDPVersion() > 3)
					{
						if (reqfile->IsPartFile())
							((CPartFile*)reqfile)->WritePartStatus(&data_out, sender);	// SLUGFILLER: hideOS
						else if (!reqfile->ShareOnlyTheNeed(&data_out, sender)) //wistily SOTN
							if (!reqfile->HideOvershares(&data_out, sender))	//Slugfiller: HideOS
								data_out.WriteUInt16(0);
					}
					data_out.WriteUInt16(theApp.uploadqueue->GetWaitingPosition(sender));
					if (thePrefs.GetDebugClientUDPLevel() > 0)
						DebugSend("OP__ReaskAck", sender);
					Packet* response = new Packet(&data_out, OP_EMULEPROT);
					response->opcode = OP_REASKACK;
					theStats.AddUpDataOverheadFileRequest(response->size);
					theApp.clientudp->SendPacket(response, ip, port);
				}
				else
				{
					DebugLogError(_T("Client UDP socket; ReaskFilePing; reqfile does not match"));
					TRACE(_T("reqfile:         %s\n"), DbgGetFileInfo(reqfile->GetFileHash()));
					TRACE(_T("sender->GetRequestFile(): %s\n"), sender->GetRequestFile() ? DbgGetFileInfo(sender->GetRequestFile()->GetFileHash()) : _T("(null)"));
				}
			}
			else
			{
				if (thePrefs.GetDebugClientUDPLevel() > 0)
					DebugRecv("OP_ReaskFilePing", NULL, (char*)reqfilehash, ip);

				//Morph - modified by AndCycle, SLUGFILLER: infiniteQueue
				/*
				if (((uint32)theApp.uploadqueue->GetWaitingUserCount() + 50) > thePrefs.GetQueueSize())
				*/
				if (((uint32)theApp.uploadqueue->GetWaitingUserCount() + 50) > thePrefs.GetQueueSize() && !thePrefs.IsInfiniteQueueEnabled())
				{
					if (thePrefs.GetDebugClientUDPLevel() > 0)
						DebugSend("OP__QueueFull", NULL);
					Packet* response = new Packet(OP_QUEUEFULL,0,OP_EMULEPROT);
					theStats.AddUpDataOverheadFileRequest(response->size);
					SendPacket(response, ip, port);
				}
			}
			break;
		}
		case OP_QUEUEFULL:
		{
			theStats.AddDownDataOverheadFileRequest(size);
			CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(ip, port);
			if (thePrefs.GetDebugClientUDPLevel() > 0)
				DebugRecv("OP_QueueFull", sender, NULL, ip);
			if (sender){
				sender->SetRemoteQueueFull(true);
				sender->UDPReaskACK(0);
			}
			break;
		}
		case OP_REASKACK:
		{
			theStats.AddDownDataOverheadFileRequest(size);
			CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(ip, port);
			if (thePrefs.GetDebugClientUDPLevel() > 0)
				DebugRecv("OP_ReaskAck", sender, NULL, ip);
			if (sender){
				CSafeMemFile data_in((BYTE*)packet,size);
				if ( sender->GetUDPVersion() > 3 )
				{
					sender->ProcessFileStatus(true, &data_in, sender->GetRequestFile());
				}
				uint16 nRank = data_in.ReadUInt16();
				sender->SetRemoteQueueFull(false);
				sender->UDPReaskACK(nRank);
				sender->AddAskedCountDown();
			}
			break;
		}
		case OP_FILENOTFOUND:
		{
			theStats.AddDownDataOverheadFileRequest(size);
			CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(ip, port);
			if (thePrefs.GetDebugClientUDPLevel() > 0)
				DebugRecv("OP_FileNotFound", sender, NULL, ip);
			if (sender){
				sender->UDPReaskFNF(); // may delete 'sender'!
				sender = NULL;
			}
			break;
		}
		case OP_PORTTEST:
		{
			theStats.AddDownDataOverheadOther(size);
			if (size == 1){
				if (packet[0]==0x12) {
					bool ret=theApp.listensocket->SendPortTestReply('1',true);
					AddDebugLogLine(true,_T("UDP Portcheck packet arrived - ACK sent back (status=%i)"),ret );
				}
			}
			break;
		}
                // MORPH START - Added by Commander, WebCache 1.2e
		case OP_HTTP_CACHED_BLOCK:
		{
			theStats.AddDownDataOverheadOther(size);
			uint32 *id = (uint32*)((BYTE*)packet+50);
//			if (thePrefs.GetLogWebCacheEvents())
//				AddDebugLogLine( false, _T("Recv UDP-WCB: %d"), *id );
			CUpDownClient* sender = theApp.clientlist->FindClientByWebCacheUploadId( *id );

			if( sender ) 
			{
				if (thePrefs.GetDebugClientUDPLevel() > 0)
					DebugRecv("OP__Http_Cached_Block (UDP)", sender, NULL, ip);
				if( thePrefs.IsWebCacheDownloadEnabled() )
				{
					if (thePrefs.GetLogWebCacheEvents())
					AddDebugLogLine( false, _T("Received WCBlock - UDP") );
					CWebCachedBlock* newblock = new CWebCachedBlock( (char*)packet, size, sender ); // Starts DL or places block on queue
				}
			} 
			else 
				if (thePrefs.GetLogWebCacheEvents())
				AddDebugLogLine( false, _T("Received cached block info from unknown client (UDP)") );

			break;
		}
                // MORPH END - Added by Commander, WebCache 1.2e
		default:
			theStats.AddDownDataOverheadOther(size);
			if (thePrefs.GetDebugClientUDPLevel() > 0)
			{
				CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(ip, port);
				Debug(_T("Unknown client UDP packet: host=%s:%u (%s) opcode=0x%02x  size=%u\n"), ipstr(ip), port, sender ? sender->DbgGetClientInfo() : _T(""), opcode, size);
			}
			return false;
	}
	return true;
}
// MORPH START - Added by SiRoB, WebCache 1.2f
// WebCache ////////////////////////////////////////////////////////////////////////////////////
// WebCache START
bool CClientUDPSocket::ProcessWebCachePacket(BYTE* packet, uint16 size, uint8 opcode, uint32 ip, uint16 port)
{
	switch(opcode)
	{
		case OP_HTTP_CACHED_BLOCK:
		{
			theStats.AddDownDataOverheadOther(size);
			uint32 *id = (uint32*)((BYTE*)packet+50);
			CUpDownClient* sender = theApp.clientlist->FindClientByWebCacheUploadId( *id );

			if( sender ) 
			{
				if (thePrefs.GetDebugClientUDPLevel() > 0)
					DebugRecv("OP__Http_Cached_Block (UDP)", sender, NULL, ip);
				if( thePrefs.IsWebCacheDownloadEnabled() )
				{
					if (thePrefs.GetLogWebCacheEvents())
					AddDebugLogLine( false, _T("Received WCBlock - UDP") );
					CWebCachedBlock* newblock = new CWebCachedBlock( (char*)packet, size, sender ); // Starts DL or places block on queue
				}
			} 
			else 
				if (thePrefs.GetLogWebCacheEvents())
				AddDebugLogLine( false, _T("Received cached block info from unknown client (UDP)") );

			break;
		}
		//JP for a future version
		case OP_RESUME_SEND_OHCBS:
		{
			theStats.AddDownDataOverheadOther(size);
			uint32 *id = (uint32*)((BYTE*)packet);
			CUpDownClient* sender = theApp.clientlist->FindClientByWebCacheUploadId( *id );

			if( sender ) 
			{
				sender->m_bIsAcceptingOurOhcbs = true;
				if (thePrefs.GetLogWebCacheEvents())
					AddDebugLogLine( false, _T("Received OP_RESUME_SEND_OHCBS from %s "), sender->DbgGetClientInfo() );
			} 
			else 
				if (thePrefs.GetLogWebCacheEvents())
				AddDebugLogLine( false, _T("Received OP_RESUME_SEND_OHCBS from unknown client") );
			break;
		}
		case OP_XPRESS_MULTI_HTTP_CACHED_BLOCKS:
		case OP_MULTI_HTTP_CACHED_BLOCKS:
			{
				CSafeMemFile data((BYTE*)packet,size);
				CUpDownClient* sender = theApp.clientlist->FindClientByWebCacheUploadId( data.ReadUInt32() ); // data.ReadUInt32() is the uploadID
				if (!sender) 
					return false;
				DebugRecv("OP__Multi_Http_Cached_Blocks", sender);
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					theStats.AddDownDataOverheadOther(size);
				if( thePrefs.IsWebCacheDownloadEnabled() && sender->SupportsWebCache() ) 
				{
					// CHECK HANDSHAKE?
					if (thePrefs.GetLogWebCacheEvents())
						AddDebugLogLine( false, _T("Received MultiWCBlocks - UDP") );
					return WebCachedBlockList.ProcessWCBlocks((char*)packet, size, opcode, sender);
				}
				break;
			}
		case OP_MULTI_FILE_REASK:
			{
				theStats.AddDownDataOverheadFileRequest(size);
				CSafeMemFile data_in(packet, size);
				uchar reqfilehash[16];
				data_in.ReadHash16(reqfilehash);
				CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfilehash);
				if (!reqfile)
				{
					if (thePrefs.GetDebugClientUDPLevel() > 0)
					{
						DebugRecv("OP_MultiFileReask", NULL, (char*)reqfilehash, ip);
						DebugSend("OP__FileNotFound", NULL);
					}

					Packet* response = new Packet(OP_FILENOTFOUND,0,OP_EMULEPROT);
					theStats.AddUpDataOverheadFileRequest(response->size);
					SendPacket(response, ip, port);
					break;
				}
				CUpDownClient* sender = theApp.uploadqueue->GetWaitingClientByIP_UDP(ip, port);
				if (sender)
				{
					if (thePrefs.GetDebugClientUDPLevel() > 0)
						DebugRecv("OP_MultiFileReask", sender, (char*)reqfilehash, ip);

					//Make sure we are still thinking about the same file
					if (md4cmp(reqfilehash, sender->GetUploadFileID()) == 0)
					{
						sender->AddAskedCount();
						sender->SetLastUpRequest();
						sender->ProcessExtendedInfo(&data_in, reqfile);

						sender->requestedFiles.AddFiles(&data_in, sender);

						CSafeMemFile data_out(128);

						if (reqfile->IsPartFile())
							((CPartFile*)reqfile)->WritePartStatus(&data_out, sender);	// SLUGFILLER: hideOS
						else if (!reqfile->ShareOnlyTheNeed(&data_out, sender)) //wistily SOTN
							if (!reqfile->HideOvershares(&data_out, sender))	//Slugfiller: HideOS
								data_out.WriteUInt16(0);

						data_out.WriteUInt16(theApp.uploadqueue->GetWaitingPosition(sender));
						if (thePrefs.GetDebugClientUDPLevel() > 0)
							DebugSend("OP__ReaskAck", sender);
						Packet* response = new Packet(&data_out, OP_EMULEPROT);
						response->opcode = OP_REASKACK;
						theStats.AddUpDataOverheadFileRequest(response->size);
						theApp.clientudp->SendPacket(response, ip, port);
					}
					else
					{
						AddDebugLogLine(false, _T("Client UDP socket; MultiFileReask; reqfile does not match"));
						TRACE(_T("reqfile:         %s\n"), DbgGetFileInfo(reqfile->GetFileHash()));
						TRACE(_T("sender->GetRequestFile(): %s\n"), sender->GetRequestFile() ? DbgGetFileInfo(sender->GetRequestFile()->GetFileHash()) : _T("(null)"));
					}
				}
				else
				{
					if (thePrefs.GetDebugClientUDPLevel() > 0)
						DebugRecv("OP_MultiFileReask", NULL, (char*)reqfilehash, ip);

					if (((uint32)theApp.uploadqueue->GetWaitingUserCount() + 50) > thePrefs.GetQueueSize())
					{
						if (thePrefs.GetDebugClientUDPLevel() > 0)
							DebugSend("OP__QueueFull", NULL);
						Packet* response = new Packet(OP_QUEUEFULL,0,OP_EMULEPROT);
						theStats.AddUpDataOverheadFileRequest(response->size);
						SendPacket(response, ip, port);
					}
				}
				break;
			}
		default:
			theStats.AddDownDataOverheadOther(size);
			if (thePrefs.GetDebugClientUDPLevel() > 0)
			{
				CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(ip, port);
				Debug(_T("Unknown client UDP packet: host=%s:%u (%s) opcode=0x%02x  size=%u\n"), ipstr(ip), port, sender ? sender->DbgGetClientInfo() : _T(""), opcode, size);
			}
			return false;
	}
	return true;
}

// MORPH END - Added by SiRoB, WebCache 1.2f
void CClientUDPSocket::OnSend(int nErrorCode){
	if (nErrorCode){
		if (thePrefs.GetVerbose())
			DebugLogError(_T("Error: Client UDP socket, error on send event: %s"), GetErrorMessage(nErrorCode, 1));
		return;
	}

// ZZ:UploadBandWithThrottler (UDP) -->
    sendLocker.Lock();
	m_bWouldBlock = false;

    if(!controlpacket_queue.IsEmpty()) {
	    theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
    }
	sendLocker.Unlock();
// <-- ZZ:UploadBandWithThrottler (UDP)
}

SocketSentBytes CClientUDPSocket::SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize){ // ZZ:UploadBandWithThrottler (UDP)
// ZZ:UploadBandWithThrottler (UDP) -->
	// NOTE: *** This function is invoked from a *different* thread!
    sendLocker.Lock();

    uint32 sentBytes = 0;
// <-- ZZ:UploadBandWithThrottler (UDP)

	while (!controlpacket_queue.IsEmpty() && !IsBusy() && sentBytes < maxNumberOfBytesToSend){ // ZZ:UploadBandWithThrottler (UDP)
		UDPPack* cur_packet = controlpacket_queue.GetHead();
		if( GetTickCount() - cur_packet->dwTime < UDPMAXQUEUETIME )
		{
			char* sendbuffer = new char[cur_packet->packet->size+2];
			memcpy(sendbuffer,cur_packet->packet->GetUDPHeader(),2);
			memcpy(sendbuffer+2,cur_packet->packet->pBuffer,cur_packet->packet->size);

			if (!SendTo(sendbuffer, cur_packet->packet->size+2, cur_packet->dwIP, cur_packet->nPort)){
                sentBytes += cur_packet->packet->size+2; // ZZ:UploadBandWithThrottler (UDP)

				controlpacket_queue.RemoveHead();
				delete cur_packet->packet;
				delete cur_packet;
			}
			delete[] sendbuffer;
		}
		else
		{
			controlpacket_queue.RemoveHead();
			delete cur_packet->packet;
			delete cur_packet;
		}
	}

// ZZ:UploadBandWithThrottler (UDP) -->
    if(!IsBusy() && !controlpacket_queue.IsEmpty()) {
	    theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
    }
    sendLocker.Unlock();

    SocketSentBytes returnVal = { true, 0, sentBytes };
    return returnVal;
// <-- ZZ:UploadBandWithThrottler (UDP)
}

int CClientUDPSocket::SendTo(char* lpBuf,int nBufLen,uint32 dwIP, uint16 nPort){
	// NOTE: *** This function is invoked from a *different* thread!
	uint32 result = CAsyncSocket::SendTo(lpBuf,nBufLen,nPort,ipstr(dwIP));
	if (result == (uint32)SOCKET_ERROR){
		uint32 error = GetLastError();
		if (error == WSAEWOULDBLOCK){
			m_bWouldBlock = true;
			return -1;
		}
		if (thePrefs.GetVerbose())
			theApp.QueueDebugLogLine(false, _T("Error: Client UDP socket, failed to send data to %s:%u: %s"), ipstr(dwIP), nPort, GetErrorMessage(error, 1));
	}
	return 0;
}

bool CClientUDPSocket::SendPacket(Packet* packet, uint32 dwIP, uint16 nPort){
	UDPPack* newpending = new UDPPack;
	newpending->dwIP = dwIP;
	newpending->nPort = nPort;
	newpending->packet = packet;
	newpending->dwTime = GetTickCount();
// ZZ:UploadBandWithThrottler (UDP) -->
    sendLocker.Lock();
		controlpacket_queue.AddTail(newpending);
    sendLocker.Unlock();

    theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this);
		return true;
// <-- ZZ:UploadBandWithThrottler (UDP)
}

bool CClientUDPSocket::Create(){
	// emulEspaña: Modified by MoNKi [MoNKi: -UPnPNAT Support-]
	// emulEspaña: Modified by MoNKi [MoNKi: -Random Ports-]
	/*
	bool ret=true;

	if (thePrefs.GetUDPPort()) {
		ret=CAsyncSocket::Create(thePrefs.GetUDPPort(),SOCK_DGRAM,FD_READ|FD_WRITE);
		if (ret)
			m_port=thePrefs.GetUDPPort();
	}

	if (ret)
		m_port=thePrefs.GetUDPPort();

	return ret;
	*/
	bool ret = true;
	WORD rndPort;
	int retries=0;
	int maxRetries = 50;

	static bool bNotFirstRun = false;

	if (thePrefs.GetUDPPort(false, false, bNotFirstRun)){
		if(thePrefs.GetUseRandomPorts()){
			do{
				retries++;
				rndPort = thePrefs.GetUDPPort(bNotFirstRun);
				if((retries < (maxRetries / 2)) && ((thePrefs.GetICFSupport() && !theApp.m_pFirewallOpener->DoesRuleExist(rndPort, NAT_PROTOCOL_UDP))
					|| !thePrefs.GetICFSupport()))
				{
					ret = CAsyncSocket::Create(rndPort,SOCK_DGRAM,FD_READ|FD_WRITE);
				}
				else if (retries >= (maxRetries / 2))
					ret = CAsyncSocket::Create(rndPort,SOCK_DGRAM,FD_READ|FD_WRITE);
			}while(!ret && retries<maxRetries);
		}
		else
			ret = CAsyncSocket::Create(thePrefs.GetUDPPort(false, true),SOCK_DGRAM,FD_READ|FD_WRITE);

		if(ret){
			m_port=thePrefs.GetUDPPort();
		
			if(thePrefs.GetICFSupport()){
				if (theApp.m_pFirewallOpener->OpenPort(thePrefs.GetUDPPort(), NAT_PROTOCOL_UDP, EMULE_DEFAULTRULENAME_UDP, thePrefs.IsOpenPortsOnStartupEnabled() || thePrefs.GetUseRandomPorts()))
					theApp.QueueLogLine(false, GetResString(IDS_FO_TEMPUDP_S), thePrefs.GetUDPPort());
				else
					theApp.QueueLogLine(false, GetResString(IDS_FO_TEMPUDP_F), thePrefs.GetUDPPort());
			}

			if(thePrefs.GetUPnPNat()){
				CUPnP_IGDControlPoint::UPNPNAT_MAPPING mapping;

				mapping.internalPort = mapping.externalPort = thePrefs.GetUDPPort();
				mapping.protocol = CUPnP_IGDControlPoint::UNAT_UDP;
				mapping.description = _T("UDP Port");
				
				theApp.AddUPnPNatPort(&mapping);
				
				thePrefs.SetUPnPUDPExternal(mapping.externalPort);
				thePrefs.SetUPnPUDPInternal(mapping.internalPort);
			}
			else{
				thePrefs.SetUPnPUDPExternal(thePrefs.GetUDPPort());
				thePrefs.SetUPnPUDPInternal(thePrefs.GetUDPPort());
			}
		}
	}

	bNotFirstRun = true;
	
	if (ret)
		m_port=thePrefs.GetUDPPort();

	return ret;
	// End emulEspaña	
}

bool CClientUDPSocket::Rebind(){
	
	// emulEspaña: Modified by MoNKi [MoNKi: -Random Ports-]
	/*
	if (thePrefs.GetUDPPort()==m_port)
		return false;
	*/
	if (!thePrefs.GetUseRandomPorts() && thePrefs.GetUDPPort(false, true)==m_port)
		return false;

	// Modified by MoNKi [MoNKi: -UPnPNAT Support-]
	if(thePrefs.GetUPnPNat()){
		CUPnP_IGDControlPoint::UPNPNAT_MAPPING mapping;
		mapping.internalPort = thePrefs.GetUPnPUDPInternal();
		mapping.externalPort = thePrefs.GetUPnPUDPExternal();
		mapping.protocol = CUPnP_IGDControlPoint::UNAT_UDP;
		mapping.description = _T("UDP Port");
		theApp.RemoveUPnPNatPort(&mapping);
	}
	// End -UPnPNAT Support-
	// End emulEspaña

	Close();

	return Create();
}
