//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

// ClientUDPSocket.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "ClientUDPSocket.h"
#include "opcodes.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CClientUDPSocket

CClientUDPSocket::CClientUDPSocket()
{
	m_bWouldBlock = false;
}

CClientUDPSocket::~CClientUDPSocket()
{
}

void CClientUDPSocket::OnReceive(int nErrorCode){
	char buffer[5000];
	CString serverbuffer;
	uint32 port;
	int32 length = ReceiveFrom(buffer,5000,serverbuffer,port);
	if (((uint8)buffer[0] == OP_EMULEPROT) && length != SOCKET_ERROR)
		ProcessPacket(buffer+2,length-2,buffer[1],serverbuffer.GetBuffer(),port);
	
}

bool CClientUDPSocket::ProcessPacket(char* packet, int16 size, int8 opcode, char* host, uint16 port){
	try{
		switch(opcode){
			case OP_REASKFILEPING:
			{
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				if (size < 16)
					break;

				CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)packet);
				if (!reqfile)
				{
					Packet* response = new Packet(OP_FILENOTFOUND,0,OP_EMULEPROT);
					theApp.uploadqueue->AddUpDataOverheadFileRequest(response->size + 8);
					SendPacket(response,inet_addr(host),port);
					break;
				}
				CUpDownClient* sender = theApp.uploadqueue->GetWaitingClientByIP_UDP(inet_addr(host), port);
				if (sender)
				{
					sender->AddAskedCount();
					sender->SetLastUpRequest();
					sender->UDPFileReasked();
					if ((sender->GetUDPVersion() > 2) && (size > 17))
					{
						uint16 nCompleteCountLast= sender->GetUpCompleteSourcesCount();
						uint16 nCompleteCountNew= *(uint16*)(packet+16);
						sender->SetUpCompleteSourcesCount(nCompleteCountNew);
						if (nCompleteCountLast != nCompleteCountNew)
						{
							if(reqfile->IsPartFile())
							{
								((CPartFile*)reqfile)->NewSrcPartsInfo();
							}
							else
							{
								reqfile->NewAvailPartsInfo();
							}
						}
					}
					break;
				}
				else{
					if ((uint32)theApp.uploadqueue->GetWaitingUserCount() + 50) > theApp.glob_prefs->GetQueueSize()){
						Packet* response = new Packet(OP_QUEUEFULL,0,OP_EMULEPROT);
						theApp.uploadqueue->AddUpDataOverheadFileRequest(response->size + 8);
						SendPacket(response,inet_addr(host),port);
					}
					break;
				}
				break;
			}
			case OP_QUEUEFULL:
			{
				theApp.downloadqueue->AddDownDataOverheadOther(size);
				CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(inet_addr(host), port);
				if (sender){
					//TRACE("CClientUDPSocket: OP_QUEUEFULL from client %s UDP:%u\n", host, port);
					sender->SetRemoteQueueFull(true);
					sender->UDPReaskACK(0);
				}
//			#ifdef _DEBUG
//				else{
//					CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP(inet_addr(host));
//					if (sender)
//						TRACE("*** CClientUDPSocket: OP_QUEUEFULL from unknown client %s UDP:%u (found client with same IP but diff. UDP port %u)\n", host, port, sender->GetUDPPort());
//					else
//						TRACE("*** CClientUDPSocket: OP_QUEUEFULL from unknown client %s UDP:%u\n", host, port);
//				}
//			#endif
				break;
			}
			case OP_REASKACK:
			{
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(inet_addr(host), port);
				if (sender){
					if (size != 2)
						break;

					uint16 nRank;
					memcpy(&nRank,packet,2);
					sender->SetRemoteQueueFull(false);
					sender->UDPReaskACK(nRank);
					sender->AddAskedCountDown();
				}
				break;
			}
			case OP_FILENOTFOUND:
			{
				theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				CUpDownClient* sender = theApp.downloadqueue->GetDownloadClientByIP_UDP(inet_addr(host), port);
				if (sender){
					sender->UDPReaskFNF();
				}
				break;
			}
			default:
				return false;
		}

		return true;
	}
	catch(CFileException* error){ // preventive, just if someone uses a CSafeMemFile in the future..
		OUTPUT_DEBUG_TRACE();
		error->Delete();
		AddDebugLogLine(false,GetResString(IDS_ERR_UDP_MISCONF) + _T(" - CFileException"));
	}
	catch(CMemoryException* error){
		OUTPUT_DEBUG_TRACE();
		error->Delete();
		AddDebugLogLine(false,GetResString(IDS_ERR_UDP_MISCONF) + _T(" - CMemoryException"));
	}
	catch(CString error){
		OUTPUT_DEBUG_TRACE();
		AddDebugLogLine(false,GetResString(IDS_ERR_UDP_MISCONF) + _T(" - ") + error);
	}
	catch(...){
		OUTPUT_DEBUG_TRACE();
		AddDebugLogLine(false,GetResString(IDS_ERR_UDP_MISCONF) + _T(" - Unknown exception"));
	}

	return false;
}

void CClientUDPSocket::OnSend(int nErrorCode){
	if (nErrorCode){
		return;
	}
	m_bWouldBlock = false;
	while (controlpacket_queue.GetHeadPosition() != 0 && !IsBusy()){
		UDPPack* cur_packet = controlpacket_queue.GetHead();
		char* sendbuffer = new char[cur_packet->packet->size+2];
		memcpy(sendbuffer,cur_packet->packet->GetUDPHeader(),2);
		memcpy(sendbuffer+2,cur_packet->packet->pBuffer,cur_packet->packet->size);
		if (!SendTo(sendbuffer, cur_packet->packet->size+2, cur_packet->dwIP, cur_packet->nPort) ){
			controlpacket_queue.RemoveHead();
			delete cur_packet->packet;
			delete cur_packet;
		}
		delete[] sendbuffer;
	}

}

int CClientUDPSocket::SendTo(char* lpBuf,int nBufLen,uint32 dwIP, uint16 nPort){
	in_addr host;
	host.S_un.S_addr = dwIP;
	uint32 result = CAsyncSocket::SendTo(lpBuf,nBufLen,nPort,inet_ntoa(host));
	if (result == (uint32)SOCKET_ERROR){
		uint32 error = GetLastError();
		if (error == WSAEWOULDBLOCK)
			m_bWouldBlock = true;
		return -1;
	}
	return 0;
}


bool CClientUDPSocket::SendPacket(Packet* packet, uint32 dwIP, uint16 nPort){
	UDPPack* newpending = new UDPPack;
	newpending->dwIP = dwIP;
	newpending->nPort = nPort;
	newpending->packet = packet;
	if ( IsBusy() ){
		controlpacket_queue.AddTail(newpending);
		return true;
	}
	char* sendbuffer = new char[packet->size+2];
	memcpy(sendbuffer,packet->GetUDPHeader(),2);
	memcpy(sendbuffer+2,packet->pBuffer,packet->size);
	if (SendTo(sendbuffer, packet->size+2, dwIP, nPort)){
		controlpacket_queue.AddTail(newpending);
	}
	else{
		delete newpending->packet;
		delete newpending;
	}
	delete[] sendbuffer;
	return true;
}

bool  CClientUDPSocket::Create(){
	if (theApp.glob_prefs->GetUDPPort())
		return CAsyncSocket::Create(theApp.glob_prefs->GetUDPPort(),SOCK_DGRAM,FD_READ|FD_WRITE);
	else
		return true;
}