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
#include "stdafx.h"
#include "DebugHelpers.h"
#include "emule.h"
#include "ListenSocket.h"
#include "PeerCacheSocket.h"
#include "opcodes.h"
#include "UpDownClient.h"
#include "ClientList.h"
#include "OtherFunctions.h"
#include "DownloadQueue.h"
#include "Statistics.h"
#include "IPFilter.h"
#include "SharedFileList.h"
#include "PartFile.h"
#include "SafeFile.h"
#include "Packets.h"
#include "UploadQueue.h"
#include "ServerList.h"
#include "Server.h"
#include "Sockets.h"
#include "emuledlg.h"
#include "TransferWnd.h"
#include "ClientListCtrl.h"
#include "ChatWnd.h"
#include "PeerCacheFinder.h"
#include "Exceptions.h"
#include "Kademlia/Utils/uint128.h"
#include "Kademlia/Kademlia/kademlia.h"
#include "Kademlia/Kademlia/prefs.h"
#include "ClientUDPSocket.h"
#include "SHAHashSet.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CClientReqSocket

IMPLEMENT_DYNCREATE(CClientReqSocket, CEMSocket)

CClientReqSocket::CClientReqSocket(CUpDownClient* in_client)
{
	SetClient(in_client);
	theApp.listensocket->AddSocket(this);
	ResetTimeOutTimer();
	deletethis = false;
	deltimer = 0;
	m_bPortTestCon=false;
	m_nOnConnect=SS_Other;
}

void CClientReqSocket::SetConState( SocketState val )
{
	//If no change, do nothing..
	if( val == m_nOnConnect )
		return;
	//Decrease count of old state..
	switch( m_nOnConnect )
	{
		case SS_Half:
			theApp.listensocket->m_nHalfOpen--;
			break;
		case SS_Complete:
			theApp.listensocket->m_nComp--;
	}
	//Set state to new state..
	m_nOnConnect = val;
	//Increase count of new state..
	switch( m_nOnConnect )
	{
		case SS_Half:
			theApp.listensocket->m_nHalfOpen++;
			break;
		case SS_Complete:
			theApp.listensocket->m_nComp++;
	}
}

void CClientReqSocket::WaitForOnConnect()
{
	SetConState(SS_Half);
}
	
CClientReqSocket::~CClientReqSocket()
{
	//This will update our statistics.
	SetConState(SS_Other);
	if (client)
		client->socket = 0;
	client = 0;
	theApp.listensocket->RemoveSocket(this);

	DEBUG_ONLY (theApp.clientlist->Debug_SocketDeleted(this));
}

void CClientReqSocket::SetClient(CUpDownClient* pClient)
{
	client = pClient;
	if (client)
		client->socket = this;
}

void CClientReqSocket::ResetTimeOutTimer(){
	timeout_timer = ::GetTickCount();
}

UINT CClientReqSocket::GetTimeOut()
{
	// PC-TODO
	// the PC socket may even already be disconnected and deleted and we still need to keep the
	// ed2k socket open because remote client may still be downloading from cache.
	if (client && client->IsUploadingToPeerCache() && (client->m_pPCUpSocket == NULL || !client->m_pPCUpSocket->IsConnected()))
	{
		// we are uploading (or at least allow uploading) but currently no socket
		return max(CEMSocket::GetTimeOut(), GetPeerCacheSocketUploadTimeout());
	}
	else if (client && client->m_pPCUpSocket && client->m_pPCUpSocket->IsConnected())
	{
		// we have an uploading PC socket, but that socket is not used (nor can it be closed)
		return max(CEMSocket::GetTimeOut(), client->m_pPCUpSocket->GetTimeOut());
	}
	else if (client && client->m_pPCDownSocket && client->m_pPCDownSocket->IsConnected())
	{
		// we have a downloading PC socket
		return max(CEMSocket::GetTimeOut(), client->m_pPCDownSocket->GetTimeOut());
	}
	else
		return CEMSocket::GetTimeOut();
}

bool CClientReqSocket::CheckTimeOut()
{
	if(m_nOnConnect == SS_Half)
	{
		//This socket is still in a half connection state.. Because of SP2, we don't know
		//if this socket is actually failing, or if this socket is just queued in SP2's new
		//protection queue. Therefore we give the socket a chance to either finally report
		//the connection error, or finally make it through SP2's new queued socket system..
		if (::GetTickCount() - timeout_timer > CEMSocket::GetTimeOut()*4){
			timeout_timer = ::GetTickCount();
			CString str;
			str.Format(_T("Timeout: State:%u"), m_nOnConnect);
			Disconnect(str);
			return true;
		}
		return false;
	}
	UINT uTimeout = GetTimeOut();
	if(client)
	{
		if (client->GetKadState() == KS_CONNECTED_BUDDY)
			return false;
		if (client->GetChatState()!=MS_NONE)
		uTimeout += CONNECTION_TIMEOUT;
	}
	if (::GetTickCount() - timeout_timer > uTimeout){
		timeout_timer = ::GetTickCount();
		CString str;
		str.Format(_T("Timeout: State:%u"), m_nOnConnect);
		Disconnect(str);
		return true;
	}
	return false;
}

void CClientReqSocket::OnClose(int nErrorCode){
	ASSERT (theApp.listensocket->IsValidSocket(this));
	CEMSocket::OnClose(nErrorCode);

	LPCTSTR pszReason;
	CString* pstrReason = NULL;
	if (nErrorCode == 0)
		pszReason = _T("Close");
	else if (thePrefs.GetVerbose()){
		pstrReason = new CString;
		*pstrReason = GetErrorMessage(nErrorCode, 1);
		pszReason = *pstrReason;
	}
	else
		pszReason = NULL;
	Disconnect(pszReason);
	delete pstrReason;
}

void CClientReqSocket::Disconnect(LPCTSTR pszReason){
	AsyncSelect(0);
	byConnected = ES_DISCONNECTED;
	if (!client)
		Safe_Delete();
	else
		if(client->Disconnected(pszReason, true)){
			CUpDownClient* temp = client;
			client->socket = NULL;
			client = NULL;
			delete temp;
			Safe_Delete();
		}
		else{
			client = NULL;
			Safe_Delete();
		}
};

void CClientReqSocket::Delete_Timed(){
// it seems that MFC Sockets call socketfunctions after they are deleted, even if the socket is closed
// and select(0) is set. So we need to wait some time to make sure this doesn't happens
	if (::GetTickCount() - deltimer > 10000)
		delete this;
}

void CClientReqSocket::Safe_Delete()
{
	ASSERT (theApp.listensocket->IsValidSocket(this));
	AsyncSelect(0);
	deltimer = ::GetTickCount();
	if (m_SocketData.hSocket != INVALID_SOCKET) // deadlake PROXYSUPPORT - changed to AsyncSocketEx
		ShutDown(SD_BOTH);
	if (client)
		client->socket = 0;
	client = 0;
	byConnected = ES_DISCONNECTED;
	deletethis = true;
}

bool CClientReqSocket::ProcessPacket(char* packet, uint32 size, UINT opcode)
{
	try
	{
		try
		{
			if (!client && opcode != OP_HELLO)
			{
				theStats.AddDownDataOverheadOther(size);
				throw GetResString(IDS_ERR_NOHELLO);
			}
			else if (client && opcode != OP_HELLO && opcode != OP_HELLOANSWER)
				client->CheckHandshakeFinished(OP_EDONKEYPROT, opcode);
			switch(opcode)
			{
				case OP_HELLOANSWER:
				{
					//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
					if(thePrefs.IsSUCDoesWork())
						SmartUploadControl();
					//MORPH END - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
					theStats.AddDownDataOverheadOther(size);
					client->ProcessHelloAnswer(packet,size);
					if (thePrefs.GetDebugClientTCPLevel() > 0){
						DebugRecv("OP_HelloAnswer", client);
						Debug(_T("  %s\n"), client->DbgGetHelloInfo());
					}

					// start secure identification, if
					//  - we have received OP_EMULEINFO and OP_HELLOANSWER (old eMule)
					//	- we have received eMule-OP_HELLOANSWER (new eMule)
					if (client->GetInfoPacketsReceived() == IP_BOTH)
						client->InfoPacketsReceived();

					if (client)
					{
						client->ConnectionEstablished();
						theApp.emuledlg->transferwnd->clientlistctrl.RefreshClient(client);
					}
					break;
				}
				case OP_HELLO:
				{
					theStats.AddDownDataOverheadOther(size);
					bool bNewClient = !client;
					if (bNewClient)
					{
						// create new client to save standart informations
						client = new CUpDownClient(this);
					}

					bool bIsMuleHello = false;
					try
					{
						bIsMuleHello = client->ProcessHelloPacket(packet,size);
					}
					catch(...)
					{
						if (bNewClient)
						{
							// Don't let CUpDownClient::Disconnected be processed for a client which is not in the list of clients.
							delete client;
							client = NULL;
						}
						throw;
					}

					if (thePrefs.GetDebugClientTCPLevel() > 0){
						DebugRecv("OP_Hello", client);
						Debug(_T("  %s\n"), client->DbgGetHelloInfo());
					}

					// now we check if we know this client already. if yes this socket will
					// be attached to the known client, the new client will be deleted
					// and the var. "client" will point to the known client.
					// if not we keep our new-constructed client ;)
					if (theApp.clientlist->AttachToAlreadyKnown(&client,this))
					{
						// update the old client informations
						bIsMuleHello = client->ProcessHelloPacket(packet,size);
					}
					else 
					{
						theApp.clientlist->AddClient(client);
						client->SetCommentDirty();
					}
					theApp.emuledlg->transferwnd->clientlistctrl.RefreshClient(client);

					// send a response packet with standart informations
					if (client->GetHashType() == SO_EMULE && !bIsMuleHello)
						client->SendMuleInfoPacket(false);

					client->SendHelloAnswer();
					if (client)
						client->ConnectionEstablished();
					// start secure identification, if
					//	- we have received eMule-OP_HELLO (new eMule)
					ASSERT( client );
					if(client)
					{
						// start secure identification, if
						//	- we have received eMule-OP_HELLO (new eMule)
						if (client->GetInfoPacketsReceived() == IP_BOTH)
							client->InfoPacketsReceived();
					}
					break;
				}
				case OP_REQUESTFILENAME:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_FileRequest", client, packet);
					theStats.AddDownDataOverheadFileRequest(size);

					if (size >= 16)
					{
						// EastShare START - Marked by TAHO, modified SUQWT
						//if (!client->GetWaitStartTime())
						//	client->SetWaitStartTime();
						// EastShare END - Marked by TAHO, modified SUQWT
						CSafeMemFile data_in((BYTE*)packet,size);
						uchar reqfilehash[16];
						data_in.ReadHash16(reqfilehash);
						CKnownFile* reqfile;
						if ( (reqfile = theApp.sharedfiles->GetFileByID(reqfilehash)) == NULL ){
							if ( !((reqfile = theApp.downloadqueue->GetFileByID(reqfilehash)) != NULL
								&& reqfile->GetFileSize() > PARTSIZE ) )
							{
								client->CheckFailedFileIdReqs(reqfilehash);
								break;
							}
						}
						// if we are downloading this file, this could be a new source
						// no passive adding of files with only one part
						if (reqfile->IsPartFile() && reqfile->GetFileSize() > PARTSIZE)
						{
							if (thePrefs.GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount())
								theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile, client, true);
					}
						// check to see if this is a new file they are asking for
						if (md4cmp(client->GetUploadFileID(), reqfilehash) != 0)
							client->SetCommentDirty();
						client->SetUploadFileID(reqfile);
						client->ProcessExtendedInfo(&data_in, reqfile);
						// send filename etc
						CSafeMemFile data_out(128);
						data_out.WriteHash16(reqfile->GetFileHash());
						data_out.WriteString(reqfile->GetFileName(), client->GetUnicodeSupport());
						Packet* packet = new Packet(&data_out);
						packet->opcode = OP_REQFILENAMEANSWER;
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__FileReqAnswer", client, (char*)reqfile->GetFileHash());
						theStats.AddUpDataOverheadFileRequest(packet->size);
						SendPacket(packet, true);
						client->SendCommentInfo(reqfile);
						break;
					}
					throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
					break;
				}
				case OP_SETREQFILEID:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_SetReqFileID", client, (size == 16) ? packet : NULL);
					theStats.AddDownDataOverheadFileRequest(size);

					if (size == 16){
						// EastShare START - Marked by TAHO, modified SUQWT
						//if (!client->GetWaitStartTime())
						//	client->SetWaitStartTime();
						// EastShare END - Marked by TAHO, modified SUQWT
						CKnownFile* reqfile;
						if ( (reqfile = theApp.sharedfiles->GetFileByID((uchar*)packet)) == NULL ){
							if ( !((reqfile = theApp.downloadqueue->GetFileByID((uchar*)packet)) != NULL
								&& reqfile->GetFileSize() > PARTSIZE ) )
							{
							// send file request no such file packet (0x48)
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugSend("OP__FileReqAnsNoFil", client, packet);
								Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
								md4cpy(replypacket->pBuffer, packet);
								theStats.AddUpDataOverheadFileRequest(replypacket->size);
								SendPacket(replypacket, true);
								client->CheckFailedFileIdReqs((uchar*)packet);
							break;
						}
						}
						// check to see if this is a new file they are asking for
						if(md4cmp(client->GetUploadFileID(), packet) != 0)
							client->SetCommentDirty();
						client->SetUploadFileID(reqfile);
						// send filestatus
						CSafeMemFile data(16+16);
						data.WriteHash16(reqfile->GetFileHash());
						if (reqfile->IsPartFile())
							((CPartFile*)reqfile)->WritePartStatus(&data, client);	// SLUGFILLER: hideOS
						else if (!reqfile->ShareOnlyTheNeed(&data, client)) //wistily SOTN
							if (!reqfile->HideOvershares(&data, client))	//Slugfiller: HideOS
								data.WriteUInt16(0);
						Packet* packet = new Packet(&data);
						packet->opcode = OP_FILESTATUS;
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__FileStatus", client, (char*)reqfile->GetFileHash());
						theStats.AddUpDataOverheadFileRequest(packet->size);
						SendPacket(packet,true);
						//Morph Start - added by AndCycle, ICS
					    // enkeyDEV: ICS - Send incomplete parts
					    if (client->GetIncompletePartVersion())
					    {
						    CSafeMemFile data(16+16);
						    data.WriteHash16(reqfile->GetFileHash());
						    if (reqfile->IsPartFile())
							    ((CPartFile*)reqfile)->WriteIncPartStatus(&data);
						    else
							    data.WriteUInt16(0);
						    Packet* packet = new Packet(&data, OP_EMULEPROT);
						    packet->opcode = OP_FILEINCSTATUS;
						    theStats.AddUpDataOverheadFileRequest(packet->size);
						    SendPacket(packet,true);
					    }
					    // <--- enkeyDEV: ICS
					    //Morph End - added by AndCycle, ICS
						break;
					}
					throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
					break;
				}
				case OP_FILEREQANSNOFIL:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_FileReqAnsNoFil", client, packet);
					theStats.AddDownDataOverheadFileRequest(size);
					if (size == 16)
					{
						CPartFile* reqfile = theApp.downloadqueue->GetFileByID((uchar*)packet);
						if (!reqfile){
							client->CheckFailedFileIdReqs((uchar*)packet);
							break;
						}
						else
							reqfile->m_DeadSourceList.AddDeadSource(client);
						// if that client does not have my file maybe has another different
						// we try to swap to another file ignoring no needed parts files
						switch (client->GetDownloadState())
						{
							case DS_CONNECTED:
							case DS_ONQUEUE:
							case DS_NONEEDEDPARTS:
                                client->DontSwapTo(client->GetRequestFile()); // ZZ:DownloadManager
                                if (!client->SwapToAnotherFile(_T("Source says it doesn't have the file. CClientReqSocket::ProcessPacket()"), true, true, true, NULL, false, false)) { // ZZ:DownloadManager
									theApp.downloadqueue->RemoveSource(client);
                                }
							break;
						}
						break;
					}
					throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
					break;
				}
				case OP_REQFILENAMEANSWER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_FileReqAnswer", client, packet);
					theStats.AddDownDataOverheadFileRequest(size);
					CSafeMemFile data((BYTE*)packet,size);
					uchar cfilehash[16];
					data.ReadHash16(cfilehash);
					CPartFile* file = theApp.downloadqueue->GetFileByID(cfilehash);
					if (file == NULL)
						client->CheckFailedFileIdReqs(cfilehash);
					client->ProcessFileInfo(&data, file);
					break;
				}
				case OP_FILESTATUS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_FileStatus", client, packet);
					theStats.AddDownDataOverheadFileRequest(size);
					CSafeMemFile data((BYTE*)packet,size);
					uchar cfilehash[16];
					data.ReadHash16(cfilehash);
					CPartFile* file = theApp.downloadqueue->GetFileByID(cfilehash);
					if (file == NULL)
						client->CheckFailedFileIdReqs(cfilehash);
					client->ProcessFileStatus(false, &data, file);
					break;
				}
				case OP_STARTUPLOADREQ:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_StartUpLoadReq", client);
					theStats.AddDownDataOverheadFileRequest(size);
					if (!client->CheckHandshakeFinished(OP_EDONKEYPROT, opcode))
						break;
					if (size == 16)
					{
						CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)packet);
						if (reqfile)
						{
							if(md4cmp(client->GetUploadFileID(), packet) != 0)
								client->SetCommentDirty();
							client->SetUploadFileID(reqfile);
							client->SendCommentInfo(reqfile);
							theApp.uploadqueue->AddClientToQueue(client);
						}
						else
							client->CheckFailedFileIdReqs((uchar*)packet);
					}
					break;
				}
				case OP_QUEUERANK:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_QueueRank", client);
					theStats.AddDownDataOverheadFileRequest(size);
					client->ProcessEdonkeyQueueRank(packet, size);
					break;
				}
				case OP_ACCEPTUPLOADREQ:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0){
						DebugRecv("OP_AcceptUploadReq", client, size >= 16 ? packet : NULL);
						if (size > 0)
							Debug(_T("  ***NOTE: Packet contains %u additional bytes\n"), size);
						Debug(_T("  QR=%d\n"), client->IsRemoteQueueFull() ? (UINT)-1 : (UINT)client->GetRemoteQueueRank());
					}
					theStats.AddDownDataOverheadFileRequest(size);
					client->ProcessAcceptUpload();
					break;
				}
				case OP_REQUESTPARTS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_RequestParts", client, packet);
					theStats.AddDownDataOverheadFileRequest(size);

					CSafeMemFile data((BYTE*)packet,size);
					uchar reqfilehash[16];
					data.ReadHash16(reqfilehash);

					uint32 auStartOffsets[3];
					auStartOffsets[0] = data.ReadUInt32();
					auStartOffsets[1] = data.ReadUInt32();
					auStartOffsets[2] = data.ReadUInt32();

					uint32 auEndOffsets[3];
					auEndOffsets[0] = data.ReadUInt32();
					auEndOffsets[1] = data.ReadUInt32();
					auEndOffsets[2] = data.ReadUInt32();

					if (thePrefs.GetDebugClientTCPLevel() > 0){
						Debug(_T("  Start1=%u  End1=%u  Size=%u\n"), auStartOffsets[0], auEndOffsets[0], auEndOffsets[0] - auStartOffsets[0]);
						Debug(_T("  Start2=%u  End2=%u  Size=%u\n"), auStartOffsets[1], auEndOffsets[1], auEndOffsets[1] - auStartOffsets[1]);
						Debug(_T("  Start3=%u  End3=%u  Size=%u\n"), auStartOffsets[2], auEndOffsets[2], auEndOffsets[2] - auStartOffsets[2]);
					}
					//MORPH START - Added by SiRoB, ZZ Upload System 20030723-0133
					uchar tempfileid[16];
                    md4clr(tempfileid);
					//MORPH START - Added by SiRoB, Anti-leecher feature
					if(client->IsLeecher()) {
						// Flag blocks to delete
                        auStartOffsets[0] = 0; auEndOffsets[0] = 0; 
                        auStartOffsets[1] = 0; auEndOffsets[1] = 0; 
                        auStartOffsets[2] = 0; auEndOffsets[2] = 0; 

						// Remove client from the upload queue
						theApp.uploadqueue->RemoveFromUploadQueue(client,GetResString(IDS_UPSTOPPEDLEECHER), true, true);
						AddDebugLogLine(false, GetResString(IDS_LEECHERDETREM));
						
						theApp.uploadqueue->AddClientToQueue(client);
						AddDebugLogLine(false, GetResString(IDS_LEECHERPUTBACK));
						client->SetUploadFileID(theApp.sharedfiles->GetFileByID(reqfilehash));
					}
					//MORPH END   - Added by SiRoB, Anti-leecher feature
					// Remark: There is a security leak that a leecher mod might exploit here.
					//         A client might send reqblock for another file than the one it 
					//         was granted to download. As long as the file ID in reqblock
					//         is the same in all reqblocks, it won't be rejected.  
					//         With this a client might be in a waiting queue with a high 
					//         priority but download block of a file set to a lower priority.
					else if(md4cmp(reqfilehash, client->GetUploadFileID()) != 0 && md4cmp(reqfilehash, tempfileid) != 0 && client->GetSessionUp() == 0){
						// client requested another file than it queued up for. Try to decide if the swith should be allowed.
						bool allowSwitch = false;

	                    CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(reqfilehash);
                        if(!currequpfile) {
                            // save original file id asked for, to be able to log it
							uchar uploadFileId[16];
							md4cpy(uploadFileId, client->GetUploadFileID());

							if(client->HasLowID() && (client->IsFriend() && client->GetFriendSlot()) == true) {
								allowSwitch = true;
							} else {
								client->SetUploadFileID(currequpfile);

                                // we need to compare with the client that would get to replace this if we kick it out
								CUpDownClient* bestQueuedClient = theApp.uploadqueue->FindBestClientInQueue();

                                // This checks if which of client and bestClient would be on top on the queue, if client is put back.
                                // Allow switch of file if bestClient wouldn't be on top. Clients score is calculated with the new file
                                // it requested, since we used SetUploadFileID above.
								if(bestQueuedClient == NULL || !theApp.uploadqueue->RightClientIsBetter(client, client->GetScore(false), bestQueuedClient, bestQueuedClient->GetScore(false))) {
									allowSwitch = true;
								}
							}
							if(allowSwitch == false) {
                                // Flag blocks to delete
                                auStartOffsets[0] = 0; auEndOffsets[0] = 0; 
                                auStartOffsets[1] = 0; auEndOffsets[1] = 0; 
                                auStartOffsets[2] = 0; auEndOffsets[2] = 0; 

								// Put client back on queue and trace some info
								AddDebugLogLine(false, GetResString(IDS_TRIEDDLOTHERFILE), 
									client->GetUserName(), md4str(uploadFileId), md4str(reqfilehash));

								// Remove client from the upload queue
								theApp.uploadqueue->RemoveFromUploadQueue(client, GetResString(IDS_IPSTOPPEDOTHFILE), true, true);
	
								// Put back with wating time intact
								theApp.uploadqueue->AddClientToQueue(client, true);

								AddDebugLogLine(false, GetResString(IDS_CLIENTPUTBACK));
						
								client->SetUploadFileID(currequpfile);
							}
						}
					}
					//MORPH END - Added by SiRoB, ZZ Upload System
					for (int i = 0; i < ARRSIZE(auStartOffsets); i++)
					{
						if (auEndOffsets[i] > auStartOffsets[i])
						{
							Requested_Block_Struct* reqblock = new Requested_Block_Struct;
							reqblock->StartOffset = auStartOffsets[i];
							reqblock->EndOffset = auEndOffsets[i];
							md4cpy(reqblock->FileID, reqfilehash);
							reqblock->transferred = 0;
							client->AddReqBlock(reqblock);
						}
						else
						{
							if (thePrefs.GetVerbose())
							{
								if (auEndOffsets[i] != 0 || auStartOffsets[i] != 0)
									AddDebugLogLine(false, _T("Client requests invalid %u. file block %u-%u (%d bytes): %s"), i, auStartOffsets[i], auEndOffsets[i], auEndOffsets[i] - auStartOffsets[i], client->DbgGetClientInfo());
							}
						}
					}
					break;
				}
				case OP_CANCELTRANSFER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_CancelTransfer", client);
					theStats.AddDownDataOverheadFileRequest(size);
					theApp.uploadqueue->RemoveFromUploadQueue(client, _T("Remote client canceled transfer."));
					break;
				}
				case OP_END_OF_DOWNLOAD:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_EndOfDownload", client, packet);
					theStats.AddDownDataOverheadFileRequest(size);
					if (size>=16 && !md4cmp(client->GetUploadFileID(),packet))
					{
						if (theApp.uploadqueue->RemoveFromUploadQueue(client, _T("Remote client ended transfer."))){
							if (thePrefs.GetLogUlDlEvents())
								AddDebugLogLine(false, DLP_LOW, "%s: Upload session ended due to ended transfer.", client->GetUserName());
						}
					}
					else
						client->CheckFailedFileIdReqs((uchar*)packet);
					break;
				}
				case OP_HASHSETREQUEST:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_HashSetReq", client, packet);
					theStats.AddDownDataOverheadFileRequest(size);

					if (size != 16)
						throw GetResString(IDS_ERR_WRONGHPACKAGESIZE);
					client->SendHashsetPacket(packet);
					break;
				}
				case OP_HASHSETANSWER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_HashSetAnswer", client, packet);
					theStats.AddDownDataOverheadFileRequest(size);
					client->ProcessHashSet(packet,size);
					break;
				}
				case OP_SENDINGPART:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 1)
						DebugRecv("OP_SendingPart", client, packet);
					theStats.AddDownDataOverheadFileRequest(24);
					//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
					/*
					if (client->GetRequestFile() && !client->GetRequestFile()->IsStopped() && (client->GetRequestFile()->GetStatus()==PS_READY || client->GetRequestFile()->GetStatus()==PS_EMPTY))
					*/
					if (client->GetRequestFile() && !client->GetRequestFile()->IsStopped() && (client->GetRequestFile()->GetStatus()==PS_READY || client->GetRequestFile()->GetStatus()==PS_EMPTY) && !client->GetRequestFile()->notSeenCompleteSource())
					//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
					{
						client->ProcessBlockPacket(packet,size);
						//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
						/*
						if (client->GetRequestFile()->IsStopped() || client->GetRequestFile()->GetStatus()==PS_PAUSED || client->GetRequestFile()->GetStatus()==PS_ERROR)
						*/
						if (client->GetRequestFile()->IsStopped() || client->GetRequestFile()->GetStatus()==PS_PAUSED || client->GetRequestFile()->GetStatus()==PS_ERROR || client->GetRequestFile()->notSeenCompleteSource())
						//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)	
						{
							client->SendCancelTransfer();
							client->SetDownloadState(client->GetRequestFile()->IsStopped() ? DS_NONE : DS_ONQUEUE);
						}
					}
					else
					{
						client->SendCancelTransfer();
						client->SetDownloadState((client->GetRequestFile()==NULL || client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
					}
					break;
				}
				case OP_OUTOFPARTREQS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_OutOfPartReqs", client);
					theStats.AddDownDataOverheadFileRequest(size);
					if (client->GetDownloadState() == DS_DOWNLOADING)
					{
						client->SetDownloadState(DS_ONQUEUE);
					}
					break;
				}
				case OP_CHANGE_CLIENT_ID:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_ChangedClientID", client);
					theStats.AddDownDataOverheadOther(size);
					CSafeMemFile data((BYTE*)packet, size);
					uint32 nNewUserID = data.ReadUInt32();
					uint32 nNewServerIP = data.ReadUInt32();
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						Debug(_T("  NewUserID=%u (%08x, %s)  NewServerIP=%u (%08x, %s)\n"), nNewUserID, nNewUserID, ipstr(nNewUserID), nNewServerIP, nNewServerIP, ipstr(nNewServerIP));
					if (IsLowID(nNewUserID))
					{	// client changed server and has a LowID
						CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
						if (pNewServer != NULL)
						{
							client->SetUserIDHybrid(nNewUserID); // update UserID only if we know the server
							client->SetServerIP(nNewServerIP);
							client->SetServerPort(pNewServer->GetPort());
						}
					}
					else if (nNewUserID == client->GetIP())
					{	// client changed server and has a HighID(IP)
						client->SetUserIDHybrid(ntohl(nNewUserID));
						CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
						if (pNewServer != NULL)
						{
							client->SetServerIP(nNewServerIP);
							client->SetServerPort(pNewServer->GetPort());
						}
					}
					else{
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							Debug(_T("***NOTE: OP_ChangedClientID unknown contents\n"));
					}
					UINT uAddData = data.GetLength() - data.GetPosition();
					if (uAddData > 0){
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							Debug(_T("***NOTE: OP_ChangedClientID contains add. data %s\n"), DbgGetHexDump((uint8*)packet + data.GetPosition(), uAddData));
					}
					break;
				}
				case OP_CHANGE_SLOT:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_ChangeSlot", client, size>=16 ? packet : NULL);
					theStats.AddDownDataOverheadOther(size);
					// sometimes sent by Hybrid
					break;
				}
				case OP_MESSAGE:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_Message", client);
					theStats.AddDownDataOverheadOther(size);
					if (size < 2)
						throw CString(_T("invalid message packet"));
					CSafeMemFile data((BYTE*)packet, size);
					UINT length = data.ReadUInt16();
					if (length+2 != size)
						throw CString(_T("invalid message packet"));
					//filter me?
					//MORPH START - Changed by SiRoB, originaly in ChatSelector::IsSpam(), Added by IceCream, third fixed criteria: leechers who try to afraid other morph/lovelave/blackrat users (NOS, Darkmule ...)
					/*
					if ( (thePrefs.MsgOnlyFriends() && !client->IsFriend()) || (thePrefs.MsgOnlySecure() && client->GetUserName()==NULL) )
					*/
					if ( (thePrefs.MsgOnlyFriends() && !client->IsFriend()) || (thePrefs.MsgOnlySecure() && client->GetUserName()==NULL) || (thePrefs.GetEnableAntiLeecher() && (client->IsLeecher() || client->TestLeecher())))
					//MORPH END - Changed by SiRoB, originaly in ChatSelector::IsSpam(), Added by IceCream, third fixed criteria: leechers who try to afraid other morph/lovelave/blackrat users (NOS, Darkmule ...)
					{
						if (!client->GetMessageFiltered()){
							if (thePrefs.GetVerbose())
							//MORPH START - Changed by SiRoB, Just Add client soft version
								AddDebugLogLine(false,_T("Filtered Message from '%s' (IP:%s) (%s)"), client->GetUserName(), ipstr(client->GetConnectIP()), client->GetClientSoftVer());
							//MORPH END   - Changed by SiRoB, Just Add client soft version
						}
						client->SetMessageFiltered(true);
						break;
					}

					if (length > MAX_CLIENT_MSG_LEN){
						if (thePrefs.GetVerbose())
							AddDebugLogLine(false, _T("Message from '%s' (IP:%s) exceeds limit by %u chars, truncated."), client->GetUserName(), ipstr(client->GetConnectIP()), length - MAX_CLIENT_MSG_LEN);
						length = MAX_CLIENT_MSG_LEN;
					}

					AddLogLine(true,GetResString(IDS_NEWMSG), client->GetUserName(), ipstr(client->GetConnectIP()));

					CString strMessage(data.ReadString(client->GetUnicodeSupport(), length));
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						Debug(_T("  %s\n"), strMessage);
					theApp.emuledlg->chatwnd->chatselector.ProcessMessage(client, strMessage);
					break;
				}
				case OP_ASKSHAREDFILES:
				{	
					// client wants to know what we have in share, let's see if we allow him to know that
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedFiles", client);
					theStats.AddDownDataOverheadOther(size);
					// IP banned, no answer for this request

					CPtrList list;
					if (thePrefs.CanSeeShares()==vsfaEverybody || (thePrefs.CanSeeShares()==vsfaFriends && client->IsFriend()))
					{
						CCKey bufKey;
						CKnownFile* cur_file;
						for (POSITION pos = theApp.sharedfiles->m_Files_map.GetStartPosition();pos != 0;)
						{
							theApp.sharedfiles->m_Files_map.GetNextAssoc(pos,bufKey,cur_file);
							// xMule_MOD: showSharePermissions
							// only show the files that you agree to show (because sometimes people would like to show
							// most of their files but hesitate to show a few ones (for some reasons :))
							uint8 Perm = cur_file->GetPermissions()>=0?cur_file->GetPermissions():thePrefs.GetPermissions();
							// Mighty Knife: Community visible filelist
							if ( (Perm == PERM_ALL)
								|| ((Perm == PERM_FRIENDS) && client->IsFriend())
								|| ((Perm == PERM_COMMUNITY) && (client->IsCommunity()) || client->IsFriend()) )
								list.AddTail((void*&)cur_file);
							// [end] Mighty Knife
							// xMule_MOD: showSharePermissions
						}
						AddLogLine(true,GetResString(IDS_REQ_SHAREDFILES),client->GetUserName(),client->GetUserIDHybrid(),GetResString(IDS_ACCEPTED) );
					}
					else
					{
						AddLogLine(true,GetResString(IDS_REQ_SHAREDFILES),client->GetUserName(),client->GetUserIDHybrid(),GetResString(IDS_DENIED) );
					}

					// now create the memfile for the packet
					uint32 iTotalCount = list.GetCount();
					CSafeMemFile tempfile(80);
					tempfile.WriteUInt32(iTotalCount);
					while (list.GetCount())
					{
						theApp.sharedfiles->CreateOfferedFilePacket((CKnownFile*)list.GetHead(), &tempfile, NULL, client);
						list.RemoveHead();
					}

					// create a packet and send it
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugSend("OP__AskSharedFilesAnswer", client);
					Packet* replypacket = new Packet(&tempfile);
					replypacket->opcode = OP_ASKSHAREDFILESANSWER;
					theStats.AddUpDataOverheadOther(replypacket->size);
					SendPacket(replypacket, true, true);
					break;
				}
				case OP_ASKSHAREDFILESANSWER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedFilesAnswer", client);
					theStats.AddDownDataOverheadOther(size);
					client->ProcessSharedFileList(packet,size);
					break;
				}
                case OP_ASKSHAREDDIRS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedDirectories", client);
                    theStats.AddDownDataOverheadOther(size);
					ASSERT( size == 0 );
					
                    if (thePrefs.CanSeeShares()==vsfaEverybody || (thePrefs.CanSeeShares()==vsfaFriends && client->IsFriend()))
					{
						AddLogLine(true,GetResString(IDS_SHAREDREQ1),client->GetUserName(),client->GetUserIDHybrid(),GetResString(IDS_ACCEPTED) );
 
						/*
						//TODO: Don't send shared directories which do not contain any files
						// add shared directories
						CString strDir;
						CStringArray arFolders;
                        POSITION pos = thePrefs.shareddir_list.GetHeadPosition();
                        while (pos)
						{
                            strDir = thePrefs.shareddir_list.GetNext(pos);
                            PathRemoveBackslash(strDir.GetBuffer());
                            strDir.ReleaseBuffer();
							
							bool bFoundFolder=false;
							for (int i = 0; i < arFolders.GetCount(); i++)
							{
								if (strDir.CompareNoCase(arFolders.GetAt(i)) == 0)
								{
									bFoundFolder=true;
									break;
								}
							}
							if (!bFoundFolder)
								arFolders.Add(strDir);
						}

						// add incoming folders
                       	for (int iCat = 0; iCat < thePrefs.GetCatCount(); iCat++)
						{
							strDir = thePrefs.GetCategory(iCat)->incomingpath;
							PathRemoveBackslash(strDir.GetBuffer());
							strDir.ReleaseBuffer();
							
							bool bFoundFolder=false;
							for (int i = 0; i < arFolders.GetCount(); i++)
							{
								if (strDir.CompareNoCase(arFolders.GetAt(i)) == 0)
								{
									bFoundFolder=true;
									break;
								}
							}
							if (!bFoundFolder)
								arFolders.Add(strDir);
						}

						// add temporary folder
						strDir = OP_INCOMPLETE_SHARED_FILES;
						bool bFoundFolder = false;
						for (int i = 0; i < arFolders.GetCount(); i++)
						{
							if (strDir.CompareNoCase(arFolders.GetAt(i)) == 0)
                        {
								bFoundFolder = true;
								break;
                        }
						}
						if (!bFoundFolder)
							arFolders.Add(strDir);

						// build packet
                        CSafeMemFile tempfile(80);
                        tempfile.WriteUInt32(arFolders.GetCount());
						for (int i = 0; i < arFolders.GetCount(); i++)
                            tempfile.WriteString(arFolders.GetAt(i), client->GetUnicodeSupport());
						*/
						
						// SLUGFILLER: shareSubdir - enumerate according to shared files
						CStringList toSend;		// String list, because it's easier and faster
						CCKey bufKey;
						CKnownFile* cur_file;
						for (POSITION pos = theApp.sharedfiles->m_Files_map.GetStartPosition();pos != 0;){
							theApp.sharedfiles->m_Files_map.GetNextAssoc(pos,bufKey,cur_file);
							// xMule_MOD: showSharePermissions - don't send dir names that are empty
							// due to file browse permissions
							uint8 Perm = cur_file->GetPermissions()>=0?cur_file->GetPermissions():thePrefs.GetPermissions();
							// Mighty Knife: Community visible filelist
							if ( Perm == PERM_NOONE 
								|| (Perm == PERM_COMMUNITY && !(client->IsCommunity() || client->IsFriend()) ) 
								|| (Perm == PERM_FRIENDS && !client->IsFriend()) )
								continue;
							// [end] Mighty Knife
							// xMule_MOD: showSharePermissions
							CString path = cur_file->GetPath();
							path.MakeLower();
							if (toSend.Find(path) == NULL)
								toSend.AddTail(path);
						}
						
						//build packet
						CSafeMemFile tempfile(80);
						tempfile.WriteUInt32(toSend.GetCount());
						for (POSITION pos = toSend.GetHeadPosition();pos != 0;toSend.GetNext(pos))
							tempfile.WriteString((LPCTSTR)toSend.GetAt(pos), client->GetUnicodeSupport());
						// SLUGFILLER: shareSubdir

						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__AskSharedDirsAnswer", client);
						Packet* replypacket = new Packet(&tempfile);
                        replypacket->opcode = OP_ASKSHAREDDIRSANS;
                        theStats.AddUpDataOverheadOther(replypacket->size);
                        SendPacket(replypacket, true, true);
					}
					else
					{
						AddLogLine(true,GetResString(IDS_SHAREDREQ1),client->GetUserName(),client->GetUserIDHybrid(),GetResString(IDS_DENIED) );
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__AskSharedDeniedAnswer", client);
                        Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
						theStats.AddUpDataOverheadOther(replypacket->size);
                        SendPacket(replypacket, true, true);
                    }
                    break;
                }
                case OP_ASKSHAREDFILESDIR:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedFilesInDirectory", client);
                    theStats.AddDownDataOverheadOther(size);
					// IP banned, no answer for this request

                    CSafeMemFile data((uchar*)packet, size);
                    CString strReqDir = data.ReadString(client->GetUnicodeSupport());
                    PathRemoveBackslash(strReqDir.GetBuffer());
                    strReqDir.ReleaseBuffer();
                    if (thePrefs.CanSeeShares()==vsfaEverybody || (thePrefs.CanSeeShares()==vsfaFriends && client->IsFriend()))
					{
						AddLogLine(true,GetResString(IDS_SHAREDREQ2),client->GetUserName(),client->GetUserIDHybrid(),strReqDir,GetResString(IDS_ACCEPTED) );
                        ASSERT( data.GetPosition() == data.GetLength() );
 
                        CTypedPtrList<CPtrList, CKnownFile*> list;
						if (strReqDir == OP_INCOMPLETE_SHARED_FILES)
						{
							// get all shared files from download queue
							int iQueuedFiles = theApp.downloadqueue->GetFileCount();
							for (int i = 0; i < iQueuedFiles; i++)
							{
								CPartFile* pFile = theApp.downloadqueue->GetFileByIndex(i);
								if (pFile == NULL || pFile->GetStatus(true) != PS_READY)
									break;
								// xMule_MOD: showSharePermissions
								// only show the files that you agree to show (because sometimes people would like to show
								// most of their files but hesitate to show a few ones (for some reasons :))
								uint8 Perm = pFile->GetPermissions()>=0?pFile->GetPermissions():thePrefs.GetPermissions();
								// Migty Knife: Community visible filelist
								if (( (Perm==PERM_ALL)
									|| ((Perm==PERM_COMMUNITY) && (client->IsCommunity() || client->IsFriend()) )
									|| ((Perm==PERM_FRIENDS) && client->IsFriend()) ) )
										list.AddTail(pFile);
								// [end] Mighty Knife
								// xMule_MOD: showSharePermissions
							}
						}
						else
						{
							// get all shared files from requested directory
							for (POSITION pos = theApp.sharedfiles->m_Files_map.GetStartPosition();pos != 0;)
							{
                            CCKey bufKey;
                            CKnownFile* cur_file;
                            theApp.sharedfiles->m_Files_map.GetNextAssoc(pos, bufKey, cur_file);
							CString strSharedFileDir(cur_file->GetPath());
							PathRemoveBackslash(strSharedFileDir.GetBuffer());
							strSharedFileDir.ReleaseBuffer();
							// xMule_MOD: showSharePermissions
							// only show the files that you agree to show (because sometimes people would like to show
							// most of their files but hesitate to show a few ones (for some reasons :))
							uint8 Perm = cur_file->GetPermissions()>=0?cur_file->GetPermissions():thePrefs.GetPermissions();
							// Migty Knife: Community visible filelist
							if ( strReqDir.CompareNoCase(strSharedFileDir) == 0 &&
								( (Perm==PERM_ALL)
								|| ((Perm==PERM_COMMUNITY) && (client->IsCommunity() || client->IsFriend()) )
								|| ((Perm==PERM_FRIENDS) && client->IsFriend()) ) )
                                list.AddTail(cur_file);
							// [end] Mighty Knife
							// xMule_MOD: showSharePermissions
                       	}
						}

						// Currently we are sending each shared directory, even if it does not contain any files.
						// Because of this we also have to send an empty shared files list..
                            CSafeMemFile tempfile(80);
						tempfile.WriteString(strReqDir, client->GetUnicodeSupport());
						tempfile.WriteUInt32(list.GetCount());
						while (list.GetCount())
						{
							theApp.sharedfiles->CreateOfferedFilePacket(list.GetHead(), &tempfile, NULL, client);
							list.RemoveHead();
						}
 
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__AskSharedFilesInDirectoryAnswer", client);
                            Packet* replypacket = new Packet(&tempfile);
                            replypacket->opcode = OP_ASKSHAREDFILESDIRANS;
						theStats.AddUpDataOverheadOther(replypacket->size);
                            SendPacket(replypacket, true, true);
                        }
                    else
					{
						AddLogLine(true,GetResString(IDS_SHAREDREQ2),client->GetUserName(),client->GetUserIDHybrid(),strReqDir,GetResString(IDS_DENIED) );
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__AskSharedDeniedAnswer", client);
                        Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
                        theStats.AddUpDataOverheadOther(replypacket->size);
                        SendPacket(replypacket, true, true);
                    }
                    break;
                }
				case OP_ASKSHAREDDIRSANS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedDirectoriesAnswer", client);
                    theStats.AddDownDataOverheadOther(size);
                    if (client->GetFileListRequested() == 1)
					{
                        CSafeMemFile data((uchar*)packet, size);
						UINT uDirs = data.ReadUInt32();
                        for (UINT i = 0; i < uDirs; i++)
						{
                            CString strDir = data.ReadString(client->GetUnicodeSupport());
							// Better send the received and untouched directory string back to that client
							//PathRemoveBackslash(strDir.GetBuffer());
							//strDir.ReleaseBuffer();
							AddLogLine(true,GetResString(IDS_SHAREDANSW),client->GetUserName(),client->GetUserIDHybrid(),strDir);

							if (thePrefs.GetDebugClientTCPLevel() > 0)
								DebugSend("OP__AskSharedFilesInDirectory", client);
                            CSafeMemFile tempfile(80);
							tempfile.WriteString(strDir, client->GetUnicodeSupport());
                            Packet* replypacket = new Packet(&tempfile);
                            replypacket->opcode = OP_ASKSHAREDFILESDIR;
                            theStats.AddUpDataOverheadOther(replypacket->size);
                            SendPacket(replypacket, true, true);
                        }
                        ASSERT( data.GetPosition() == data.GetLength() );
                        client->SetFileListRequested(uDirs);
                    }
					else
						AddLogLine(true,GetResString(IDS_SHAREDANSW2),client->GetUserName(),client->GetUserIDHybrid());
                    break;
                }
                case OP_ASKSHAREDFILESDIRANS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedFilesInDirectoryAnswer", client);
                    theStats.AddDownDataOverheadOther(size);
                    CSafeMemFile data((uchar*)packet, size);
                    CString strDir = data.ReadString(client->GetUnicodeSupport());
					PathRemoveBackslash(strDir.GetBuffer());
					strDir.ReleaseBuffer();
                    if (client->GetFileListRequested() > 0)
					{
						AddLogLine(true,GetResString(IDS_SHAREDINFO1),client->GetUserName(),client->GetUserIDHybrid(),strDir);
						client->ProcessSharedFileList(packet + data.GetPosition(), size - data.GetPosition(), strDir);
						if (client->GetFileListRequested() == 0)
							AddLogLine(true,GetResString(IDS_SHAREDINFO2),client->GetUserName(),client->GetUserIDHybrid());
                    }
					else
						AddLogLine(true,GetResString(IDS_SHAREDANSW3),client->GetUserName(),client->GetUserIDHybrid(),strDir);
                    break;
                }
                case OP_ASKSHAREDDENIEDANS:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AskSharedDeniedAnswer", client);
                    theStats.AddDownDataOverheadOther(size);
                    ASSERT( size == 0 );
					AddLogLine(true,GetResString(IDS_SHAREDREQDENIED),client->GetUserName(),client->GetUserIDHybrid());
					client->SetFileListRequested(0);
                    break;
                }

				default:
					theStats.AddDownDataOverheadOther(size);
					PacketToDebugLogLine(_T("eDonkey"), packet, size, opcode, DLP_LOW);
					break;
			}
		}
		catch(CFileException* error)
		{
			error->Delete();	//mf
			throw GetResString(IDS_ERR_INVALIDPACKAGE);
		}
		catch(CMemoryException* error)
		{
			error->Delete();
			throw CString(_T("Memory exception"));
		}
	}
	catch(CClientException* ex) // nearly same as the 'CString' exception but with optional deleting of the client
	{
		if (thePrefs.GetVerbose() && !ex->m_strMsg.IsEmpty())
			AddDebugLogLine(false, _T("%s - while processing eDonkey packet: opcode=%s  size=%u; %s"), ex->m_strMsg, DbgGetDonkeyClientTCPOpcode(opcode), size, DbgGetClientInfo());
		if (client && ex->m_bDelete)
			client->SetDownloadState(DS_ERROR);
		Disconnect(ex->m_strMsg);
		ex->Delete();
		return false;
	}
	catch(CString error)
	{
		if (thePrefs.GetVerbose() && !error.IsEmpty()){
			if (opcode == OP_REQUESTFILENAME /*low priority for OP_REQUESTFILENAME*/)
				AddDebugLogLine(DLP_LOW, false, _T("%s - while processing eDonkey packet: opcode=%s  size=%u; %s"), error, DbgGetDonkeyClientTCPOpcode(opcode), size, DbgGetClientInfo());
			else
				AddDebugLogLine(DLP_HIGH, false, _T("%s - while processing eDonkey packet: opcode=%s  size=%u; %s"), error, DbgGetDonkeyClientTCPOpcode(opcode), size, DbgGetClientInfo());
		}
		if (client)
			client->SetDownloadState(DS_ERROR);	
		Disconnect(_T("Error when processing packet.") + error);
		return false;
	}
	return true;
}

bool CClientReqSocket::ProcessExtPacket(char* packet, uint32 size, UINT opcode, UINT uRawSize)
{
	try
	{
		try
		{
			if (!client && opcode!=OP_PORTTEST)
			{
				theStats.AddDownDataOverheadOther(uRawSize);
				throw GetResString(IDS_ERR_UNKNOWNCLIENTACTION);
			}
			if (thePrefs.m_iDbgHeap >= 2 && opcode!=OP_PORTTEST)
				ASSERT_VALID(client);
			switch(opcode)
			{
				case OP_MULTIPACKET:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_MultiPacket", client, packet);
					theStats.AddDownDataOverheadFileRequest(size);

					client->CheckHandshakeFinished(OP_EMULEPROT, opcode);
					CSafeMemFile data_in((BYTE*)packet,size);
					uchar reqfilehash[16];
					data_in.ReadHash16(reqfilehash);
					CKnownFile* reqfile;

					if ( (reqfile = theApp.sharedfiles->GetFileByID(reqfilehash)) == NULL ){
						if ( !((reqfile = theApp.downloadqueue->GetFileByID(reqfilehash)) != NULL
								&& reqfile->GetFileSize() > PARTSIZE ) )
						{
							// send file request no such file packet (0x48)
							if (thePrefs.GetDebugClientTCPLevel() > 0)
								DebugSend("OP__FileReqAnsNoFil", client, packet);
							Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
							md4cpy(replypacket->pBuffer, packet);
							theStats.AddUpDataOverheadFileRequest(replypacket->size);
							SendPacket(replypacket, true);
							client->CheckFailedFileIdReqs(reqfilehash);
							break;
						}
					}

					// EastShare START - Marked by TAHO, modified SUQWT
					//if (!client->GetWaitStartTime())
					//	client->SetWaitStartTime();
					// EastShare END - Marked by TAHO, modified SUQWT
					// if we are downloading this file, this could be a new source
					// no passive adding of files with only one part
					if (reqfile->IsPartFile() && reqfile->GetFileSize() > PARTSIZE)
					{
						if (thePrefs.GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount())
							theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile, client, true);
					}
					// check to see if this is a new file they are asking for
					if (md4cmp(client->GetUploadFileID(), reqfilehash) != 0)
						client->SetCommentDirty();
					client->SetUploadFileID(reqfile);
					uint8 opcode_in;
					CSafeMemFile data_out(128);
					data_out.WriteHash16(reqfile->GetFileHash());
					while(data_in.GetLength()-data_in.GetPosition())
					{
						opcode_in = data_in.ReadUInt8();
						switch(opcode_in)
						{
							case OP_REQUESTFILENAME:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPReqFileName", client, packet);

								client->ProcessExtendedInfo(&data_in, reqfile);
								data_out.WriteUInt8(OP_REQFILENAMEANSWER);
								data_out.WriteString(reqfile->GetFileName(), client->GetUnicodeSupport());
								break;
							}
							case OP_AICHFILEHASHREQ:
							{
								if (client->IsSupportingAICH() && reqfile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE
									&& reqfile->GetAICHHashset()->HasValidMasterHash())
								{
									data_out.WriteUInt8(OP_AICHFILEHASHANS);
									reqfile->GetAICHHashset()->GetMasterHash().Write(&data_out);
								}
								break;
							}
							case OP_SETREQFILEID:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPSetReqFileID", client, packet);

								data_out.WriteUInt8(OP_FILESTATUS);
								if (reqfile->IsPartFile())
									((CPartFile*)reqfile)->WritePartStatus(&data_out, client);	// SLUGFILLER: hideOS
								else if (!reqfile->ShareOnlyTheNeed(&data_out, client)) //wistily SOTN
									if (!reqfile->HideOvershares(&data_out, client))	//Slugfiller: HideOS
										data_out.WriteUInt16(0);
								
								//Morph Start - added by AndCycle, ICS
								// enkeyDev: ICS - Send incomplete parts
								if (client->GetIncompletePartVersion())
								{
									data_out.WriteUInt8(OP_FILEINCSTATUS);
									if (reqfile->IsPartFile())
										((CPartFile*)reqfile)->WriteIncPartStatus(&data_out);
									else
										data_out.WriteUInt16(0);
								}
								// <--- enkeyDev: ICS
								//Morph End - added by AndCycle, ICS
										
								break;
							}
							//We still send the source packet seperately.. 
							//We could send it within this packet.. If agreeded, I will fix it..
							case OP_REQUESTSOURCES:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPReqSources", client, packet);

								if (thePrefs.GetDebugSourceExchange())
									AddDebugLogLine(false, _T("SXRecv: Client source request; %s, File=\"%s\""), client->DbgGetClientInfo(), reqfile->GetFileName());

								//Although this shouldn't happen, it's a just in case to any Mods that mess with version numbers.
								if (client->GetSourceExchangeVersion() > 1)
								{
									//data_out.WriteUInt8(OP_ANSWERSOURCES);
									DWORD dwTimePassed = ::GetTickCount() - client->GetLastSrcReqTime() + CONNECTION_LATENCY;
									bool bNeverAskedBefore = client->GetLastSrcReqTime() == 0;
									if( 
										//if not complete and file is rare
										(    reqfile->IsPartFile()
										&& (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS)
										&& ((CPartFile*)reqfile)->GetSourceCount() <= RARE_FILE
										) ||
										//OR if file is not rare or if file is complete
										( (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS * MINCOMMONPENALTY) )
										) 
									{
										client->SetLastSrcReqTime();
										Packet* tosend = reqfile->CreateSrcInfoPacket(client);
										if(tosend)
										{
											if (thePrefs.GetDebugClientTCPLevel() > 0)
												DebugSend("OP__AnswerSources", client, (char*)reqfile->GetFileHash());
											theStats.AddUpDataOverheadSourceExchange(tosend->size);
											SendPacket(tosend, true);
										}
									}
//									else
//									{
//										if (thePrefs.GetVerbose())
//											AddDebugLogLine(false, "RCV: Source Request to fast. (This is testing the new timers to see how much older client will not receive this)");
//									}
								}
								break;
							}

							default:
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									Debug(_T("***NOTE: Invalid sub opcode 0x%02x with OP_MultiPacket received; %s"), opcode_in, client->DbgGetClientInfo());
						}
					}
					if( data_out.GetLength() > 16 )
					{
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("OP__MulitPacketAns", client, (char*)reqfile->GetFileHash());
						Packet* reply = new Packet(&data_out, OP_EMULEPROT);
						reply->opcode = OP_MULTIPACKETANSWER;
						theStats.AddUpDataOverheadFileRequest(reply->size);
						SendPacket(reply, true);
					}
					break;
				}
				case OP_MULTIPACKETANSWER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_MultiPacketAns", client, packet);
					theStats.AddDownDataOverheadFileRequest(size);

					client->CheckHandshakeFinished(OP_EMULEPROT, opcode);
					CSafeMemFile data_in((BYTE*)packet,size);
					uchar reqfilehash[16];
					data_in.ReadHash16(reqfilehash);
					CPartFile* reqfile = theApp.downloadqueue->GetFileByID(reqfilehash);
					//Make sure we are downloading this file.
					if (reqfile==NULL){
						client->CheckFailedFileIdReqs(reqfilehash);
						throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (OP_MULTIPACKETANSWER; reqfile==NULL)");
					}
					if (client->GetRequestFile()==NULL)
						throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (OP_MULTIPACKETANSWER; client->GetRequestFile()==NULL)");
					if (reqfile != client->GetRequestFile())
						throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (OP_MULTIPACKETANSWER; reqfile!=client->GetRequestFile())");
					uint8 opcode_in;
					while(data_in.GetLength()-data_in.GetPosition())
					{
						opcode_in = data_in.ReadUInt8();
						switch(opcode_in)
						{
							case OP_REQFILENAMEANSWER:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPReqFileNameAns", client, packet);

								client->ProcessFileInfo(&data_in, reqfile);
								break;
							}
							case OP_FILESTATUS:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPFileStatus", client, packet);

								client->ProcessFileStatus(false, &data_in, reqfile);
								break;
							}
							case OP_AICHFILEHASHANS:
							{
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									DebugRecv("OP_MPAICHFILEHASH", client);
								client->ProcessAICHFileHash(&data_in, reqfile);
								break;
							}
							//Morph Start - added by AndCycle, ICS
							// enkeyDEV: ICS
							case OP_FILEINCSTATUS:
							{
								theStats.AddDownDataOverheadFileRequest(size);
								client->ProcessFileIncStatus(&data_in,size);
								break;
							}
							// <--- enkeyDEV: ICS
							//Morph End - added by AndCycle, ICS
							default:
								if (thePrefs.GetDebugClientTCPLevel() > 0)
									Debug(_T("***NOTE: Invalid sub opcode 0x%02x with OP_MultiPacketAns received; %s"), opcode_in, client->DbgGetClientInfo());
						}
					}
					break;
				}
				case OP_EMULEINFO:
				{
					theStats.AddDownDataOverheadOther(uRawSize);
					client->ProcessMuleInfoPacket(packet,size);
					if (thePrefs.GetDebugClientTCPLevel() > 0){
						DebugRecv("OP_EmuleInfo", client);
						Debug(_T("  %s\n"), client->DbgGetMuleInfo());
					}

					// start secure identification, if
					//  - we have received eD2K and eMule info (old eMule)
					if (client->GetInfoPacketsReceived() == IP_BOTH)
						client->InfoPacketsReceived();

					client->SendMuleInfoPacket(true);
					break;
				}
				case OP_EMULEINFOANSWER:
				{
					theStats.AddDownDataOverheadOther(uRawSize);
					client->ProcessMuleInfoPacket(packet,size);
					if (thePrefs.GetDebugClientTCPLevel() > 0){
						DebugRecv("OP_EmuleInfoAnswer", client);
						Debug(_T("  %s\n"), client->DbgGetMuleInfo());
					}

					// start secure identification, if
					//  - we have received eD2K and eMule info (old eMule)
					if (client->GetInfoPacketsReceived() == IP_BOTH)
						client->InfoPacketsReceived();
					break;
				}
				case OP_SECIDENTSTATE:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_SecIdentState", client);
					theStats.AddDownDataOverheadOther(uRawSize);
					client->ProcessSecIdentStatePacket((uchar*)packet,size);
					if (client->GetSecureIdentState() == IS_SIGNATURENEEDED)
						client->SendSignaturePacket();
					else if (client->GetSecureIdentState() == IS_KEYANDSIGNEEDED)
					{
						client->SendPublicKeyPacket();
						client->SendSignaturePacket();
					}
					break;
				}
				case OP_PUBLICKEY:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_PublicKey", client);
					theStats.AddDownDataOverheadOther(uRawSize);

					client->ProcessPublicKeyPacket((uchar*)packet,size);
					break;
				}
  				case OP_SIGNATURE:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_Signature", client);
					theStats.AddDownDataOverheadOther(uRawSize);
					client->ProcessSignaturePacket((uchar*)packet,size);
					break;
				}
				case OP_COMPRESSEDPART:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 1)
						DebugRecv("OP_CompressedPart", client, packet);
					theStats.AddDownDataOverheadFileRequest(24);
					client->CheckHandshakeFinished(OP_EMULEPROT, opcode);
					if (client->GetRequestFile() && !client->GetRequestFile()->IsStopped() && (client->GetRequestFile()->GetStatus()==PS_READY || client->GetRequestFile()->GetStatus()==PS_EMPTY))
					{
						client->ProcessBlockPacket(packet,size,true);
						if (client->GetRequestFile()->IsStopped() || client->GetRequestFile()->GetStatus()==PS_PAUSED || client->GetRequestFile()->GetStatus()==PS_ERROR)
						{
							client->SendCancelTransfer();
							client->SetDownloadState(client->GetRequestFile()->IsStopped() ? DS_NONE : DS_ONQUEUE);
						}
					}
					else
					{
						client->SendCancelTransfer();
						client->SetDownloadState((client->GetRequestFile()==NULL || client->GetRequestFile()->IsStopped()) ? DS_NONE : DS_ONQUEUE);
					}
					break;
				}
				case OP_QUEUERANKING:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_QueueRanking", client);
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					client->CheckHandshakeFinished(OP_EMULEPROT, opcode);
					client->ProcessEmuleQueueRank(packet, size);
					break;
				}
  				case OP_REQUESTSOURCES:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_RequestSources", client, packet);
					theStats.AddDownDataOverheadSourceExchange(uRawSize);
					client->CheckHandshakeFinished(OP_EMULEPROT, opcode);
					if (client->GetSourceExchangeVersion() > 1)
					{
						if(size != 16)
							throw GetResString(IDS_ERR_BADSIZE);

						if (thePrefs.GetDebugSourceExchange())
							AddDebugLogLine(false, _T("SXRecv: Client source request; %s, %s"), client->DbgGetClientInfo(), DbgGetFileInfo((uchar*)packet));

						//first check shared file list, then download list
						CKnownFile* file;
						if ((file = theApp.sharedfiles->GetFileByID((uchar*)packet)) != NULL ||
							(file = theApp.downloadqueue->GetFileByID((uchar*)packet)) != NULL)
						{
							DWORD dwTimePassed = ::GetTickCount() - client->GetLastSrcReqTime() + CONNECTION_LATENCY;
							bool bNeverAskedBefore = client->GetLastSrcReqTime() == 0;
							if( 
								//if not complete and file is rare
								(    file->IsPartFile()
								  && (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS)
								  && ((CPartFile*)file)->GetSourceCount() <= RARE_FILE
								) ||
								//OR if file is not rare or if file is complete
								( (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASKS * MINCOMMONPENALTY) )
							) 
							{
								client->SetLastSrcReqTime();
								Packet* tosend = file->CreateSrcInfoPacket(client);
								if(tosend)
								{
									if (thePrefs.GetDebugClientTCPLevel() > 0)
										DebugSend("OP__AnswerSources", client, (char*)file->GetFileHash());
									theStats.AddUpDataOverheadSourceExchange(tosend->size);
									SendPacket(tosend, true, true);
								}
							}
						}
						else
							client->CheckFailedFileIdReqs((uchar*)packet);
					}
					break;
				}
 				case OP_ANSWERSOURCES:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AnswerSources", client, packet);
					theStats.AddDownDataOverheadSourceExchange(uRawSize);
					client->CheckHandshakeFinished(OP_EMULEPROT, opcode);
					CSafeMemFile data((BYTE*)packet,size);
					uchar hash[16];
					data.ReadHash16(hash);
					CKnownFile* file = theApp.downloadqueue->GetFileByID(hash);
					if(file)
					{
						if (file->IsPartFile())
						{
							//set the client's answer time
							client->SetLastSrcAnswerTime();
							//and set the file's last answer time
							((CPartFile*)file)->SetLastAnsweredTime();
							((CPartFile*)file)->AddClientSources(&data, client->GetSourceExchangeVersion(), client);
						}
					}
					else
						client->CheckFailedFileIdReqs(hash);
					break;
				}
				case OP_FILEDESC:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_FileDesc", client);
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					client->CheckHandshakeFinished(OP_EMULEPROT, opcode);
					client->ProcessMuleCommentPacket(packet,size);
					break;
				}
				case OP_REQUESTPREVIEW:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_RequestPreView", client);
					theStats.AddDownDataOverheadOther(uRawSize);
					client->CheckHandshakeFinished(OP_EMULEPROT, opcode);

					if (thePrefs.CanSeeShares()==vsfaEverybody || (thePrefs.CanSeeShares()==vsfaFriends && client->IsFriend()))	
					{
						client->ProcessPreviewReq(packet,size);	
						if (thePrefs.GetVerbose())
							AddDebugLogLine(true,_T("Client '%s' (%s) requested Preview - accepted"), client->GetUserName(), ipstr(client->GetConnectIP()));
					}
					else
					{
						// we don't send any answer here, because the client should know that he was not allowed to ask
						if (thePrefs.GetVerbose())
							AddDebugLogLine(true,_T("Client '%s' (%s) requested Preview - denied"), client->GetUserName(), ipstr(client->GetConnectIP()));
					}
					break;
				}
				case OP_PREVIEWANSWER:
				{
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_PreviewAnswer", client);
					theStats.AddDownDataOverheadOther(uRawSize);
					client->CheckHandshakeFinished(OP_EMULEPROT, opcode);
					client->ProcessPreviewAnswer(packet, size);
					break;
				}
				case OP_PEERCACHE_QUERY:
				{
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					if (!client->ProcessPeerCacheQuery(packet, size))
					{
						CSafeMemFile dataSend(128);
						dataSend.WriteUInt8(PCPCK_VERSION);
						dataSend.WriteUInt8(PCOP_NONE);
						if (thePrefs.GetDebugClientTCPLevel() > 0){
							DebugSend("OP__PeerCacheAnswer", client);
							Debug(_T("  %s\n"), _T("Not supported"));
						}
						Packet* pEd2kPacket = new Packet(&dataSend, OP_EMULEPROT, OP_PEERCACHE_ANSWER);
						theStats.AddUpDataOverheadFileRequest(pEd2kPacket->size);
						SendPacket(pEd2kPacket);
					}
					break;
				}
				case OP_PEERCACHE_ANSWER:
				{
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					if ( (!client->ProcessPeerCacheAnswer(packet, size)) && client->GetDownloadState() != DS_NONEEDEDPARTS)
					{
						// We have sent a PeerCache Query to the remote client, for any reason the remote client
						// can not process it -> fall back to ed2k download.
						client->SetPeerCacheDownState(PCDS_NONE);
						ASSERT( client->m_pPCDownSocket == NULL );

						// PC-TODO: Check client state.
						ASSERT( client->GetDownloadState() == DS_DOWNLOADING );
						client->SetDownloadState(DS_ONQUEUE); // clear block requests
						if (client)
							client->StartDownload();
					}
					break;
				}
				case OP_PEERCACHE_ACK:
				{
					theStats.AddDownDataOverheadFileRequest(uRawSize);
					client->ProcessPeerCacheAcknowledge(packet, size);
					break;
				}
				case OP_PUBLICIP_ANSWER:
				{
					theStats.AddDownDataOverheadOther(uRawSize);
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_PublicIPAns", client);
					client->ProcessPublicIPAnswer((BYTE*)packet,size);
					break;
				}
				case OP_PUBLICIP_REQ:
				{
					theStats.AddDownDataOverheadOther(uRawSize);
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_PublicIPReq", client);
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugSend("OP__PublicIPAns", client);
					Packet* pPacket = new Packet(OP_PUBLICIP_ANSWER, 4, OP_EMULEPROT);
					PokeUInt32(pPacket->pBuffer, client->GetIP());
					theStats.AddUpDataOverheadOther(pPacket->size);
					SendPacket(pPacket);
					break;
				}
				case OP_PORTTEST:
				{
					theStats.AddDownDataOverheadOther(uRawSize);
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_PortTest", client);
					m_bPortTestCon=true;
					Packet* replypacket = new Packet(OP_PORTTEST, 1);
					replypacket->pBuffer[0]=0x12;
					SendPacket(replypacket);
					break;
				}
				case OP_CALLBACK:
				{
					if(!Kademlia::CKademlia::getPrefs())
						break;
					theStats.AddDownDataOverheadOther(size);
					CSafeMemFile data((BYTE*)packet,size);
					Kademlia::CUInt128 check;
					data.ReadUInt128(&check);
					check.xor(Kademlia::CUInt128(true));
					if( check.compareTo(Kademlia::CKademlia::getPrefs()->getKadID()))
						break;
					Kademlia::CUInt128 fileid;
					data.ReadUInt128(&fileid);
					uchar fileid2[16];
					fileid.toByteArray(fileid2);
					CKnownFile* reqfile;
					if ( (reqfile = theApp.sharedfiles->GetFileByID(fileid2)) == NULL )
					{
						if ( (reqfile = theApp.downloadqueue->GetFileByID(fileid2)) == NULL)
							break;
					}

					uint32 ip = data.ReadUInt32();
					uint16 tcp = data.ReadUInt16();
					CUpDownClient* callback;
					callback = theApp.clientlist->FindClientByIP(ntohl(ip), tcp);
					if( callback == NULL )
					{
						callback = new CUpDownClient(NULL,tcp,ip,0,0);
						theApp.clientlist->AddClient(callback);
					}
					callback->TryToConnect(true);
					break;
				}
				case OP_REASKCALLBACKTCP:
				{
					CUpDownClient* buddy = theApp.clientlist->GetBuddy();
					if( buddy != client )
						//This callback was not from our buddy.. Ignore.
						break;
					theStats.AddDownDataOverheadFileRequest(size);
					CSafeMemFile data_in((uchar*)packet, size);
					uint32 destip = data_in.ReadUInt32();
					uint16 destport = data_in.ReadUInt16();
					uchar reqfilehash[16];
					data_in.ReadHash16(reqfilehash);
					CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfilehash);
					if (!reqfile)
					{
						if (thePrefs.GetDebugClientUDPLevel() > 0)
						{
							DebugRecv("OP_REASKCALLBACKTCP", NULL, (char*)reqfilehash, destip);
							DebugSend("OP__FileNotFound", NULL);
						}

						Packet* response = new Packet(OP_FILENOTFOUND,0,OP_EMULEPROT);
						theStats.AddUpDataOverheadFileRequest(response->size);
						theApp.clientudp->SendPacket(response, destip, destport);
						break;
					}
					CUpDownClient* sender = theApp.uploadqueue->GetWaitingClientByIP_UDP(destip, destport);
					if (sender)
					{
						if (thePrefs.GetDebugClientUDPLevel() > 0)
							DebugRecv("OP_REASKCALLBACKTCP", sender, (char*)reqfilehash, destip);

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
									reqfile->UpdatePartsInfo();
								}
							}
							CSafeMemFile data_out(128);
							if(sender->GetUDPVersion() > 3)
							{
								if (reqfile->IsPartFile())
									((CPartFile*)reqfile)->WritePartStatus(&data_out);
								else
									data_out.WriteUInt16(0);
							}
							data_out.WriteUInt16(theApp.uploadqueue->GetWaitingPosition(sender));
							if (thePrefs.GetDebugClientUDPLevel() > 0)
								DebugSend("OP__ReaskAck", sender);
							Packet* response = new Packet(&data_out, OP_EMULEPROT);
							response->opcode = OP_REASKACK;
							theStats.AddUpDataOverheadFileRequest(response->size);
							theApp.clientudp->SendPacket(response, destip, destport);
						}
						else
						{
							AddDebugLogLine(false, _T("Client UDP socket; OP_REASKCALLBACKTCP; reqfile does not match"));
							TRACE(_T("reqfile:         %s\n"), DbgGetFileInfo(reqfile->GetFileHash()));
							TRACE(_T("sender->GetRequestFile(): %s\n"), sender->GetRequestFile() ? DbgGetFileInfo(sender->GetRequestFile()->GetFileHash()) : _T("(null)"));
						}
					}
					else
					{
						if (thePrefs.GetDebugClientUDPLevel() > 0)
							DebugRecv("OP_REASKCALLBACKTCP", NULL, (char*)reqfilehash, destip);
		
						if (((uint32)theApp.uploadqueue->GetWaitingUserCount() + 50) > thePrefs.GetQueueSize())
						{
							if (thePrefs.GetDebugClientUDPLevel() > 0)
								DebugSend("OP_REASKCALLBACKTCP", NULL);
							Packet* response = new Packet(OP_QUEUEFULL,0,OP_EMULEPROT);
							theStats.AddUpDataOverheadFileRequest(response->size);
							theApp.clientudp->SendPacket(response, destip, destport);
						}
					}
					break;
				}
				case OP_AICHANSWER:
				{
					theStats.AddDownDataOverheadOther(uRawSize);
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AICHANSWER", client);
					client->ProcessAICHAnswer(packet,size);
					break;
				}
				case OP_AICHREQUEST:
				{
					theStats.AddDownDataOverheadOther(uRawSize);
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AICHREQUEST", client);
					client->ProcessAICHRequest(packet,size);
					break;
				}
				case OP_AICHFILEHASHANS:
				{
					// those should not be received normally, since we should only get those in MULTIPACKET
					theStats.AddDownDataOverheadOther(uRawSize);
					if (thePrefs.GetDebugClientTCPLevel() > 0)
						DebugRecv("OP_AICHFILEHASH", client);
					CSafeMemFile data((BYTE*)packet, size);
					client->ProcessAICHFileHash(&data, NULL);
					break;
				}
				case OP_AICHFILEHASHREQ:
				{
					// those should not be received normally, since we should only get those in MULTIPACKET
					CSafeMemFile data((BYTE*)packet, size);
					uchar abyHash[16];
					data.ReadHash16(abyHash);
					CKnownFile* pPartFile = theApp.sharedfiles->GetFileByID(abyHash);
					if (pPartFile == NULL){
						client->CheckFailedFileIdReqs(abyHash);
						break;
					}
					if (client->IsSupportingAICH() && pPartFile->GetAICHHashset()->GetStatus() == AICH_HASHSETCOMPLETE
						&& pPartFile->GetAICHHashset()->HasValidMasterHash())
					{
						CSafeMemFile data_out;
						data_out.WriteHash16(abyHash);
						pPartFile->GetAICHHashset()->GetMasterHash().Write(&data_out);
						SendPacket(new Packet(&data_out, OP_EMULEPROT, OP_AICHFILEHASHANS));
					}
					break;
				}
				//Morph Start - added by AndCycle, ICS
			    // enkeyDEV: ICS
			    case OP_FILEINCSTATUS:{
				    CSafeMemFile data((BYTE*)packet,size);
				    theStats.AddDownDataOverheadFileRequest(size);
				    client->ProcessFileIncStatus(&data,size,true);
				    break;
			    }
			    // <--- enkeyDEV: ICS
			    //Morph End - added by AndCycle, ICS

				default:
					theStats.AddDownDataOverheadOther(uRawSize);
					PacketToDebugLogLine(_T("eMule"), packet, size, opcode, DLP_DEFAULT);
					break;
			}
		}
		catch(CFileException* error)
		{
			error->Delete();
			throw GetResString(IDS_ERR_INVALIDPACKAGE);
		}
		catch(CMemoryException* error)
		{
			error->Delete();
			throw CString(_T("Memory exception"));
		}
	}
	catch(CClientException* ex) // nearly same as the 'CString' exception but with optional deleting of the client
	{
		if (thePrefs.GetVerbose() && !ex->m_strMsg.IsEmpty())
			AddDebugLogLine(false, _T("%s - while processing eMule packet: opcode=%s  size=%u; %s"), ex->m_strMsg, DbgGetMuleClientTCPOpcode(opcode), size, DbgGetClientInfo());
		if (client && ex->m_bDelete)
			client->SetDownloadState(DS_ERROR);
		Disconnect(ex->m_strMsg);
		ex->Delete();
		return false;
	}
	catch(CString error)
	{
		if (thePrefs.GetVerbose() && !error.IsEmpty())
			AddDebugLogLine(false, _T("%s - while processing eMule packet: opcode=%s  size=%u; %s"), error, DbgGetMuleClientTCPOpcode(opcode), size, DbgGetClientInfo());
		if (client)
			client->SetDownloadState(DS_ERROR);
		Disconnect(_T("ProcessExtPacket error. ") + error);
		return false;
	}
	return true;
}

void CClientReqSocket::PacketToDebugLogLine(LPCTSTR protocol, const char* packet, uint32 size, UINT opcode, EDebugLogPriority dlpPriority)
{
	if (thePrefs.GetVerbose())
	{
		CString buffer; 
	    buffer.Format(_T("Unknown %s Protocol Opcode: 0x%02x, Size=%u, Data=["), protocol, opcode, size);
		for (uint32 i = 0; i < size && i < 50; i++){
			if (i > 0)
				buffer += _T(' ');
			TCHAR temp[3];
			_stprintf(temp, _T("%02x"), (uint8)packet[i]);
			buffer += temp;
		}
		buffer += (i == size) ? _T("]") : _T("..]");
		DbgAppendClientInfo(buffer);
		AddDebugLogLine(dlpPriority, false, _T("%s"), buffer);
	}
}

CString CClientReqSocket::DbgGetClientInfo()
{
	CString str;
	SOCKADDR_IN sockAddr = {0};
	int nSockAddrLen = sizeof(sockAddr);
	GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
	if (sockAddr.sin_addr.S_un.S_addr != 0 && (client == NULL || sockAddr.sin_addr.S_un.S_addr != client->GetIP()))
		str.AppendFormat(_T("IP=%s"), ipstr(sockAddr.sin_addr));
	if (client){
		if (!str.IsEmpty())
			str += _T("; ");
		str += _T("Client=") + client->DbgGetClientInfo();
	}
	return str;
}

void CClientReqSocket::DbgAppendClientInfo(CString& str)
{
	CString strClientInfo(DbgGetClientInfo());
	if (!strClientInfo.IsEmpty()){
		if (!str.IsEmpty())
			str += _T("; ");
		str += strClientInfo;
	}
}

void CClientReqSocket::OnInit(){
	//uint8 tv = 1;
	//SetSockOpt(SO_DONTLINGER,&tv,sizeof(BOOL));
}

void CClientReqSocket::OnConnect(int nErrorCode)
{
	SetConState(SS_Complete);
	CEMSocket::OnConnect(nErrorCode);
	if (nErrorCode)
	{
	    CString strTCPError;
		if (thePrefs.GetVerbose())
		{
		    strTCPError = GetErrorMessage(nErrorCode, 1);
		    if (nErrorCode != WSAECONNREFUSED && nErrorCode != WSAETIMEDOUT)
			    AddDebugLogLine(false, _T("Client TCP socket error (OnConnect): %s; %s"), strTCPError, DbgGetClientInfo());
		}
	}
	else
	{
		//This socket may have been delayed by SP2 protection, lets make sure it doesn't time out instantly.
		ResetTimeOutTimer();
	}
}

void CClientReqSocket::OnSend(int nErrorCode){
	ResetTimeOutTimer();
	CEMSocket::OnSend(nErrorCode);
}

void CClientReqSocket::OnError(int nErrorCode)
{
	CString strTCPError;
	if (thePrefs.GetVerbose())
	{
		if (nErrorCode == ERR_WRONGHEADER)
			strTCPError = _T("Wrong header");
		else if (nErrorCode == ERR_TOOBIG)
			strTCPError = _T("Too much data sent");
		else
			strTCPError = GetErrorMessage(nErrorCode);
		AddDebugLogLine(false, _T("Client TCP socket error: %s; %s"), strTCPError, DbgGetClientInfo());
	}

	Disconnect(strTCPError);
}

bool CClientReqSocket::PacketReceivedCppEH(Packet* packet)
{
	bool bResult;
	UINT uRawSize = packet->size;
	switch (packet->prot){
		case OP_EDONKEYPROT:
			bResult = ProcessPacket(packet->pBuffer,packet->size,packet->opcode);
			break;
		case OP_PACKEDPROT:
			if (!packet->UnPackPacket()){
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false, _T("Failed to decompress client TCP packet; %s; %s"), DbgGetClientTCPPacket(packet->prot, packet->opcode, packet->size), DbgGetClientInfo());
				bResult = false;
				break;
			}
		case OP_EMULEPROT:
			bResult = ProcessExtPacket(packet->pBuffer, packet->size, packet->opcode, uRawSize);
			break;
		default:{
			theStats.AddDownDataOverheadOther(uRawSize);
			if (thePrefs.GetVerbose())
				AddDebugLogLine(false, _T("Received unknown client TCP packet; %s; %s"), DbgGetClientTCPPacket(packet->prot, packet->opcode, packet->size), DbgGetClientInfo());

			if (client)
				client->SetDownloadState(DS_ERROR);
			Disconnect(_T("Unknown protocol"));
			bResult = false;
		}
	}
	return bResult;
}

#ifdef USE_CLIENT_TCP_CATCH_ALL_HANDLER
int FilterSE(DWORD dwExCode, LPEXCEPTION_POINTERS pExPtrs, CClientReqSocket* reqsock, Packet* packet)
{
	if (thePrefs.GetVerbose())
	{
		CString strExError;
		if (pExPtrs){
			const EXCEPTION_RECORD* er = pExPtrs->ExceptionRecord;
			strExError.Format(_T("Error: Unknown exception %08x in CClientReqSocket::PacketReceived at 0x%08x"), er->ExceptionCode, er->ExceptionAddress);
		}
		else
			strExError.Format(_T("Error: Unknown exception %08x in CClientReqSocket::PacketReceived"), dwExCode);

		// we already had an unknown exception, better be prepared for dealing with invalid data -> use another exception handler
		try{
			CString strError = strExError;
			strError.AppendFormat(_T("; %s"), DbgGetClientTCPPacket(packet?packet->prot:0, packet?packet->opcode:0, packet?packet->size:0));
			reqsock->DbgAppendClientInfo(strError);
			CemuleApp::AddDebugLogLine(false, _T("%s"), strError);
		}
		catch(...){
			ASSERT(0);
			CemuleApp::AddDebugLogLine(false, _T("%s"), strExError);
		}
	}
	
	// this searches the next exception handler -> catch(...) in 'CAsyncSocketExHelperWindow::WindowProc'
	// as long as I do not know where and why we are crashing, I prefere to have it handled that way which
	// worked fine in 28a/b.
	//
	// 03-J�n-2004 [bc]: Returning the execution to the catch-all handler in 'CAsyncSocketExHelperWindow::WindowProc'
	// can make things even worse, because in some situations, the socket will continue fireing received events. And
	// because the processed packet (which has thrown the exception) was not removed from the EMSocket buffers, it would
	// be processed again and again.
	//return EXCEPTION_CONTINUE_SEARCH;

	// this would continue the program "as usual" -> return execution to the '__except' handler
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

#ifdef USE_CLIENT_TCP_CATCH_ALL_HANDLER
int CClientReqSocket::PacketReceivedSEH(Packet* packet)
{
	int iResult;
	// this function is only here to get a chance of determining the crash address via SEH
	__try{
		iResult = PacketReceivedCppEH(packet);
	}
	__except(FilterSE(GetExceptionCode(), GetExceptionInformation(), this, packet)){
		iResult = -1;
	}
	return iResult;
}
#endif

bool CClientReqSocket::PacketReceived(Packet* packet)
{
	bool bResult;
#ifdef USE_CLIENT_TCP_CATCH_ALL_HANDLER
	int iResult = PacketReceivedSEH(packet);
	if (iResult < 0)
	{
		if (client)
			client->SetDownloadState(DS_ERROR);
		Disconnect(_T("Unknown Exception"));
		bResult = false;
	}
	else
		bResult = (bool)iResult;
#else
	bResult = PacketReceivedCppEH(packet);
#endif
	return bResult;
}

void CClientReqSocket::OnReceive(int nErrorCode){
	ResetTimeOutTimer();
	CEMSocket::OnReceive(nErrorCode);
}

bool CClientReqSocket::Create(){
	theApp.listensocket->AddConnection();
	BOOL result = CAsyncSocketEx::Create(0,SOCK_STREAM,FD_WRITE|FD_READ|FD_CLOSE|FD_CONNECT); // deadlake PROXYSUPPORT - changed to AsyncSocketEx
	OnInit();
	return result;
}

SocketSentBytes CClientReqSocket::SendControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend) {
    SocketSentBytes returnStatus = CEMSocket::SendControlData(maxNumberOfBytesToSend, overchargeMaxBytesToSend);

    if(returnStatus.success && (returnStatus.sentBytesControlPackets > 0 || returnStatus.sentBytesStandardPackets > 0)) {
        ResetTimeOutTimer();
    }

    return returnStatus;
}

SocketSentBytes CClientReqSocket::SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 overchargeMaxBytesToSend) {
    SocketSentBytes returnStatus = CEMSocket::SendFileAndControlData(maxNumberOfBytesToSend, overchargeMaxBytesToSend);

    if(returnStatus.success && (returnStatus.sentBytesControlPackets > 0 || returnStatus.sentBytesStandardPackets > 0)) {
        ResetTimeOutTimer();
    }

    return returnStatus;
}

// CListenSocket
void CClientReqSocket::SendPacket(Packet* packet, bool delpacket, bool controlpacket, uint32 actualPayloadSize){
	ResetTimeOutTimer();
	CEMSocket::SendPacket(packet,delpacket,controlpacket, actualPayloadSize);
}

bool CListenSocket::SendPortTestReply(char result,bool disconnect) {
	POSITION pos2;
	for(POSITION pos1 = socket_list.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
		socket_list.GetNext(pos1);
		CClientReqSocket* cur_sock = socket_list.GetAt(pos2);

		if (cur_sock->m_bPortTestCon) {
			Packet* replypacket = new Packet(OP_PORTTEST, 1);
			replypacket->pBuffer[0]=result;
			cur_sock->SendPacket(replypacket);
			if (disconnect)
				//cur_sock->Disconnect(_T("Porttest over"));
				cur_sock->m_bPortTestCon=false;
			return true;
		}
	}
	return false;
}

// CListenSocket
// CListenSocket member functions
CListenSocket::CListenSocket()
{
	bListening = false;
	opensockets = 0;
	maxconnectionreached = 0;
	m_OpenSocketsInterval = 0;
	m_nPendingConnections = 0;
	memset(m_ConnectionStates, 0, sizeof m_ConnectionStates);
	peakconnections = 0;
	totalconnectionchecks = 0;
	averageconnections = 0.0;
	activeconnections = 0;
	m_port=0;
	m_nHalfOpen = 0;
	m_nComp = 0;
	per5average = 0; //MORPH - Added by Yun.SF3, Auto DynUp changing
}

CListenSocket::~CListenSocket(){
	Close();
	KillAllSockets();
}

bool CListenSocket::Rebind(){
	if (thePrefs.GetPort()==m_port)
		return false;

	Close();
	KillAllSockets();
	
	return StartListening();
}

bool CListenSocket::StartListening(){
	bListening = true;
	// Creating the socket with SO_REUSEADDR may solve LowID issues if emule was restarted
	// quickly or started after a crash, but(!) it will also create another problem. If the
	// socket is already used by some other application (e.g. a 2nd emule), we though bind
	// to that socket leading to the situation that 2 applications are listening at the same
	// port!

	bool ret=(Create(thePrefs.GetPort(), SOCK_STREAM, FD_ACCEPT, NULL, FALSE/*TRUE*/) && Listen());
	if (ret)
		m_port=thePrefs.GetPort();

	return ret;
}

void CListenSocket::ReStartListening(){
	bListening = true;
	if (m_nPendingConnections){
		m_nPendingConnections--;
		OnAccept(0);
	}
}

void CListenSocket::StopListening(){
	bListening = false;
	maxconnectionreached++;
}

void CListenSocket::OnAccept(int nErrorCode){
	if (!nErrorCode){
		m_nPendingConnections++;
		if (m_nPendingConnections < 1){
			ASSERT ( false );
			m_nPendingConnections = 1;
		}
		if (TooManySockets(true) && !theApp.serverconnect->IsConnecting()){
			StopListening();
			return;
		}
		else if ( bListening == false )
			ReStartListening(); //If the client is still at maxconnections, this will allow it to go above it.. But if you don't, you will get a lowID on all servers.
	
		for (/**/; m_nPendingConnections; AddConnection())
		{
			m_nPendingConnections--;

			CClientReqSocket* newclient = new CClientReqSocket();
			SOCKADDR_IN SockAddr = {0};
			int iSockAddrLen = sizeof SockAddr;
			if (!Accept(*newclient, (SOCKADDR*)&SockAddr, &iSockAddrLen)){
				newclient->Safe_Delete();
				continue;
			}

			if (SockAddr.sin_addr.S_un.S_addr == 0) // for safety..
			{
				iSockAddrLen = sizeof SockAddr;
				newclient->GetPeerName((SOCKADDR*)&SockAddr, &iSockAddrLen);
				AddDebugLogLine(false, _T("***NOTE: SockAddr.sin_addr.S_un.S_addr == 0;  GetPeerName returned %s"), ipstr(SockAddr.sin_addr.S_un.S_addr));
		}
//		if (TooManySockets(true) && !theApp.serverconnect->IsConnecting())
			ASSERT( SockAddr.sin_addr.S_un.S_addr != 0 && SockAddr.sin_addr.S_un.S_addr != INADDR_NONE );

			if (theApp.ipfilter->IsFiltered(SockAddr.sin_addr.S_un.S_addr)){
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Rejecting connection attempt (IP=%s) - IP filter (%s)"), ipstr(SockAddr.sin_addr.S_un.S_addr), theApp.ipfilter->GetLastHit());
				newclient->Safe_Delete();
				theStats.filteredclients++;
				continue;
			}

			if (theApp.clientlist->IsBannedClient(SockAddr.sin_addr.S_un.S_addr)){
				if (thePrefs.GetLogBannedClients()){
					CUpDownClient* pClient = theApp.clientlist->FindClientByIP(SockAddr.sin_addr.S_un.S_addr);
					AddDebugLogLine(false, _T("Rejecting connection attempt of banned client %s %s"), ipstr(SockAddr.sin_addr.S_un.S_addr), pClient->DbgGetClientInfo());
				}
				newclient->Safe_Delete();
				continue;
			}
//			StopListening();
			newclient->AsyncSelect(FD_WRITE|FD_READ|FD_CLOSE);
			newclient->OnInit();
		}
	}
}

void CListenSocket::Process(){
	POSITION pos2;
	m_OpenSocketsInterval = 0;
	opensockets = 0;
	//MORPH START - Added by Yun.SF3, Auto DynUp changing
	if (per5average)
		per5average /= 2;
	//MORPH END - Added by Yun.SF3, Auto DynUp changing
	for(POSITION pos1 = socket_list.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
		socket_list.GetNext(pos1);
		CClientReqSocket* cur_sock = socket_list.GetAt(pos2);
		opensockets++;

		if (cur_sock->deletethis){
			if (cur_sock->m_SocketData.hSocket != INVALID_SOCKET){ // deadlake PROXYSUPPORT - changed to AsyncSocketEx
				cur_sock->Close();			// calls 'closesocket'
			}
			else{
				cur_sock->Delete_Timed();	// may delete 'cur_sock'
			}
		}
		else{
			cur_sock->CheckTimeOut();		// may call 'shutdown'
		}
	}
	if ( (GetOpenSockets()+5 < thePrefs.GetMaxConnections() || theApp.serverconnect->IsConnecting()) && !bListening)
	   ReStartListening();
}

void CListenSocket::RecalculateStats(){
	memset(m_ConnectionStates,0,sizeof m_ConnectionStates);
	for (POSITION pos = socket_list.GetHeadPosition(); pos != NULL; ){
		switch (socket_list.GetNext(pos)->GetConState()){
			case ES_DISCONNECTED:
				m_ConnectionStates[0]++;
				break;
			case ES_NOTCONNECTED:
				m_ConnectionStates[1]++;
				break;
			case ES_CONNECTED:
				m_ConnectionStates[2]++;
				break;
		}
   }
}

void CListenSocket::AddSocket(CClientReqSocket* toadd){
	socket_list.AddTail(toadd);
}

void CListenSocket::RemoveSocket(CClientReqSocket* todel){
	for (POSITION pos = socket_list.GetHeadPosition(); pos != NULL; ){
		POSITION posLast = pos;
		if ( socket_list.GetNext(pos) == todel )
			socket_list.RemoveAt(posLast);
   }
}

void CListenSocket::KillAllSockets(){
	for (POSITION pos = socket_list.GetHeadPosition();pos != 0;pos = socket_list.GetHeadPosition()){
		CClientReqSocket* cur_socket = socket_list.GetAt(pos);
		if (cur_socket->client)
			delete cur_socket->client;
		else
			delete cur_socket;
	}
}

void CListenSocket::AddConnection(){
	//MORPH START - Added by Yun.SF3, Auto DynUp changing
	per5average++;
	//MORPH END - Added by Yun.SF3, Auto DynUp changing
	m_OpenSocketsInterval++;
	opensockets++;
//MORPH START - Added by Yun.SF3, Auto DynUp changing
	if (thePrefs.IsAutoDynUpSwitching())
		if (per5average >= thePrefs.MaxConnectionsSwitchBorder() && !thePrefs.IsSUCEnabled())
			SwitchSUC(true);
		else if (per5average <= 5 && !thePrefs.IsDynUpEnabled())
			SwitchSUC(false);
//MORPH END - Added by Yun.SF3, Auto DynUp changing
}

bool CListenSocket::TooManySockets(bool bIgnoreInterval){
	if (GetOpenSockets() > thePrefs.GetMaxConnections() 
		|| (m_OpenSocketsInterval > (thePrefs.GetMaxConperFive()*GetMaxConperFiveModifier()) && !bIgnoreInterval)
		|| (m_nHalfOpen >= thePrefs.GetMaxHalfConnections() && !bIgnoreInterval))
	{
		return true;
	}
	else
		return false;
}

bool CListenSocket::IsValidSocket(CClientReqSocket* totest){
	return socket_list.Find(totest);
}

#ifdef _DEBUG
void CListenSocket::Debug_ClientDeleted(CUpDownClient* deleted){
	for (POSITION pos = socket_list.GetHeadPosition(); pos != NULL;){
		CClientReqSocket* cur_sock = socket_list.GetNext(pos);
		if (!AfxIsValidAddress(cur_sock, sizeof(CClientReqSocket))) {
			AfxDebugBreak(); 
		}
		if (thePrefs.m_iDbgHeap >= 2)
			ASSERT_VALID(cur_sock);
		if (cur_sock->client == deleted){
			AfxDebugBreak();
		}
	}
}
#endif

void CListenSocket::UpdateConnectionsStatus()
{
	activeconnections = GetOpenSockets();
	if( peakconnections < activeconnections )
		peakconnections = activeconnections;
	// -khaos--+++>
	if (peakconnections>thePrefs.GetConnPeakConnections())
		thePrefs.SetConnPeakConnections(peakconnections);
	// <-----khaos-
	if( theApp.IsConnected() ){
		totalconnectionchecks++;
		float percent;
		percent = (float)(totalconnectionchecks-1)/(float)totalconnectionchecks;
		if( percent > .99f )
			percent = .99f;
		averageconnections = (averageconnections*percent) + (float)activeconnections*(1.0f-percent);
	}
}

float CListenSocket::GetMaxConperFiveModifier(){
	//This is a alpha test.. Will clean up for b version.
	float SpikeSize = GetOpenSockets() - averageconnections ;
	if ( SpikeSize < 1 )
		return 1;
	float SpikeTolerance = 25.0f*(float)thePrefs.GetMaxConperFive()/10.0f;
	if ( SpikeSize > SpikeTolerance )
		return 0;
	float Modifier = (1.0f-(SpikeSize/SpikeTolerance));
	return Modifier;
}

//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
void CClientReqSocket::SmartUploadControl()
{
  // Smart Upload Control v2 (SUC) Version 17.3.2003 by [lovelace]
  // MaxVUR: maximum variable upload rate
  uint32 VUR=theApp.uploadqueue->GetMaxVUR();
  uint32 checkrespond=(::GetTickCount() - client->GetAskTime());
  
  uint32 VURstep=(((thePrefs.GetMaxUpload()*1024-theApp.uploadqueue->GetDatarate())>10000)
         ? (thePrefs.GetMaxUpload()*1024-theApp.uploadqueue->GetDatarate())/2 
         : UPLOAD_CHECK_CLIENT_DR/2);

  if (checkrespond<theApp.uploadqueue->GetAvgRespondTime(1))
    theApp.uploadqueue->SetAvgRespondTime(0,(uint32)(0.95*theApp.uploadqueue->GetAvgRespondTime(0)));
  else
    theApp.uploadqueue->SetAvgRespondTime(0,(uint32)((19*theApp.uploadqueue->GetAvgRespondTime(0)+1000)*0.05));
  if (checkrespond<thePrefs.GetSUCPitch())
    theApp.uploadqueue->SetAvgRespondTime(1,(uint32)((499*theApp.uploadqueue->GetAvgRespondTime(1)+checkrespond)*0.002));

  if (theApp.uploadqueue->GetAvgRespondTime(0)>thePrefs.GetSUCHigh())
    //lower
	theApp.uploadqueue->SetMaxVUR((uint32)(theApp.uploadqueue->GetDatarate()-UPLOAD_CHECK_CLIENT_DR/2));
  else if (theApp.uploadqueue->GetAvgRespondTime(0)<thePrefs.GetSUCLow())
    {
    //higher
    if (VUR<(theApp.uploadqueue->GetDatarate()+VURstep))
	  theApp.uploadqueue->SetMaxVUR(theApp.uploadqueue->GetDatarate()+VURstep);
	}
  else
  { // slightly drift higher
    if (VUR<theApp.uploadqueue->GetDatarate()+thePrefs.GetSUCDrift() && thePrefs.GetSUCDrift())
	  theApp.uploadqueue->SetMaxVUR(theApp.uploadqueue->GetDatarate()+thePrefs.GetSUCDrift());
	}
  if (thePrefs.IsSUCLog ()) {
	AddDebugLogLine(false,_T("Smart Upload Control: (time:%ims, clip:%ims, ratio:%i) VUR:%iB/s"),
		checkrespond,
		theApp.uploadqueue->GetAvgRespondTime(1),
		theApp.uploadqueue->GetAvgRespondTime(0),
		theApp.uploadqueue->GetMaxVUR());
  }
}
//MORPH END - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]

//MORPH START - Added by Yun.SF3, Auto DynUp changing
void CListenSocket::SwitchSUC(bool bSetSUCOn)
{
	if (bSetSUCOn){
		thePrefs.SetDynUpEnabled(false);
		AddDebugLogLine(false,GetResString(IDS_SWITCHSUC));
		thePrefs.SetSUCEnabled(true);
	}
	else{
		thePrefs.SetSUCEnabled(false);
		AddDebugLogLine(false,GetResString(IDS_SWITCHDYNUP));
		thePrefs.SetDynUpEnabled(true);

	}
}
//MORPH END - Added by Yun.SF3, Auto DynUp changing
