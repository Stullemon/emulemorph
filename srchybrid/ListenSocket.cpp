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
// ListenSocket.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "ListenSocket.h"
#include "opcodes.h"
#include "KnownFile.h"
#include "sharedfilelist.h"
#include "uploadqueue.h"
#include "updownclient.h"
#include "clientlist.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CClientReqSocket
CClientReqSocket::CClientReqSocket(CPreferences* in_prefs,CUpDownClient* in_client){
	app_prefs = in_prefs;
	client = in_client;
	if (in_client)
		client->socket = this;
	theApp.listensocket->AddSocket(this);
	ResetTimeOutTimer();
	deletethis = false;
	deltimer = 0;
}


CClientReqSocket::~CClientReqSocket(){
	if (client)
		client->socket = 0;
	client = 0;
	theApp.listensocket->RemoveSocket(this);

	DEBUG_ONLY (theApp.clientlist->Debug_SocketDeleted(this));
}

void CClientReqSocket::ResetTimeOutTimer(){
	timeout_timer = ::GetTickCount();
};

bool CClientReqSocket::CheckTimeOut(){
	if(client && client->GetChatState()!=MS_NONE){
		if (::GetTickCount() - timeout_timer > CONNECTION_TIMEOUT*2){
			timeout_timer = ::GetTickCount();
			//MORPH START - Added by SiRoB, ZZ Upload System,  Kademlia 40c13
			Disconnect(GetResString(IDS_CNXTIMEOUT) + " 2");
			//MORPH END   - Added by SiRoB, ZZ Upload System,  Kademlia 40c13
			return true;
		}
	}
	else{
		if (::GetTickCount() - timeout_timer > CONNECTION_TIMEOUT){
			timeout_timer = ::GetTickCount();
			//MORPH START - Added by SiRoB, ZZ Upload System,  Kademlia 40c13
			Disconnect(GetResString(IDS_CNXTIMEOUT)+ " 1");
			//MORPH END   - Added by SiRoB, ZZ Upload System,  Kademlia 40c13
			return true;
		}
	}
	return false;
};

void CClientReqSocket::OnClose(int nErrorCode){
	ASSERT (theApp.listensocket->IsValidSocket(this));
	CEMSocket::OnClose(nErrorCode);
	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	Disconnect("OnClose()");
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
};

//MORPH START - Added by SiRoB, ZZ Upload System, Kademlia 40c13
void CClientReqSocket::Disconnect(CString reason){
    if(reason) {
        reason.Insert(0, GetResString(IDS_SKTDISREASON));
    } else {
        CString temp = GetResString(IDS_SKTDISUNKREASON);
        reason = temp;
    }

	AsyncSelect(0);
	byConnected = ES_DISCONNECTED;
	if (!client)
		Safe_Delete();
	else
		if(client->Disconnected(reason,true)){
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
//MORPH END   - Added by SiRoB, ZZ Upload System,  Kademlia 40c13

void CClientReqSocket::Delete_Timed(){
// it seems that MFC Sockets call socketfunctions after they are deleted, even if the socket is closed
// and select(0) is set. So we need to wait some time to make sure this doesn't happens
	if (::GetTickCount() - deltimer > 10000)
		delete this;
}

void CClientReqSocket::Safe_Delete(){
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

bool CClientReqSocket::ProcessPacket(char* packet, uint32 size, UINT opcode){
	try{
		try{
			if (!client && opcode != OP_HELLO){
				//theApp.downloadqueue->AddDownDataOverheadOther(size);
				throw GetResString(IDS_ERR_NOHELLO);
			}
			switch(opcode){
				case OP_HELLOANSWER:{
					//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
					if(theApp.glob_prefs->IsSUCDoesWork())
						SmartUploadControl();
					//MORPH END - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					client->ProcessHelloAnswer(packet,size);
// start secure identification, if
					//  - we have received OP_EMULEINFO and OP_HELLOANSWER (old eMule)
					//	- we have received eMule-OP_HELLOANSWER (new eMule)
					if (client->GetInfoPacketsReceived() == IP_BOTH)
						client->InfoPacketsReceived();

					if (client){
						client->ConnectionEstablished();
						theApp.emuledlg->transferwnd.clientlistctrl.RefreshClient(client);
					}
					break;
				}
				case OP_HELLO:{
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					bool bNewClient = !client;
					if (bNewClient){
						// create new client to save standart informations
						client = new CUpDownClient(this);
					}

					bool bIsMuleHello = false;
					try{
						bIsMuleHello = client->ProcessHelloPacket(packet,size);
					}
					catch(...){
						if (bNewClient){
							// Don't let CUpDownClient::Disconnected be processed for a client which is not in the list of clients.
							delete client;
							client = NULL;
						}
						throw;
					}

					// if IP is filtered, dont reply but disconnect...
					if (theApp.ipfilter->IsFiltered(client->GetIP())) {
						AddDebugLogLine(true,GetResString(IDS_IPFILTERED),client->GetFullIP(),theApp.ipfilter->GetLastHit());
						theApp.stat_filteredclients++;
						if (bNewClient){
							delete client;
							client = NULL;
							//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
							Disconnect(GetResString(IDS_IPISFILTERED));
							//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
						}
						else{
							//MORPH START - Added by SiRoB, ZZ Upload System, Kademlia 40c13
							Disconnect(GetResString(IDS_IPISFILTERED)+" 2");
							//MORPH END   - Added by SiRoB, ZZ Upload System, Kademlia 40c13
						}
						break;
					}

					// now we check if we know this client already. if yes this socket will
					// be attached to the known client, the new client will be deleted
					// and the var. "client" will point to the known client.
					// if not we keep our new-constructed client ;)
					if (theApp.clientlist->AttachToAlreadyKnown(&client,this)){
						// update the old client informations
						bIsMuleHello = client->ProcessHelloPacket(packet,size);
						client->DisableL2HAC(); //<<-- enkeyDEV(th1) -L2HAC- lowid side
					}
					else {
						theApp.clientlist->AddClient(client);
						client->SetCommentDirty();
					}
					
					theApp.emuledlg->transferwnd.clientlistctrl.RefreshClient(client);

					// send a response packet with standart informations
					if (client->GetHashType() == SO_EMULE && !bIsMuleHello)
						client->SendMuleInfoPacket(false);
					
					client->SendHelloAnswer();
					if (client)
						client->ConnectionEstablished();

					// start secure identification, if
					//	- we have received eMule-OP_HELLO (new eMule)
					if (client->GetInfoPacketsReceived() == IP_BOTH)
						client->InfoPacketsReceived();
					break;
				}
				case OP_FILEREQUEST:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					
					// IP banned, no answer for this request
					if (client->IsBanned() ){
						break;						
					}

					if (size == 16 || (size > 16 && client->GetExtendedRequestsVersion() > 0)){
						if (!client->GetWaitStartTime())
							client->SetWaitStartTime();
						uchar reqfileid[16];
						md4cpy(reqfileid,packet);
						CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfileid);
						if (!reqfile){
							// if we've just started a download we may want to use that client as a source
							CKnownFile* partfile = theApp.downloadqueue->GetFileByID(reqfileid);
							if (partfile && partfile->IsPartFile())
								if( theApp.glob_prefs->GetMaxSourcePerFile() > ((CPartFile*)partfile)->GetSourceCount() ) //<<--
									theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)partfile,client);
						}
						if (!reqfile){
							// 26-Jul-2003: removed sending of OP_FILEREQANSNOFIL when receiving OP_FILEREQUEST
							//	for better compatibility with ed2k protocol (eDonkeyHybrid) and to save some traffic
							//
							//	*) The OP_FILEREQANSNOFIL _has_ to be sent on receiving OP_SETREQFILEID.
							//	*) The OP_FILEREQANSNOFIL _may_ be sent on receiving OP_FILEREQUEST but in almost all cases 
							//	   it's not needed because the remote client will also send a OP_SETREQFILEID.

							// DbT:FileRequest
							// send file request no such file packet (0x48)
//							Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
//							md4cpy(replypacket->pBuffer, packet);
//							theApp.uploadqueue->AddUpDataOverheadFileRequest(replypacket->size);
//							SendPacket(replypacket, true);
							// DbT:End
							break;
						}
						// if wer are downloading this file, this could be a new source
						if (reqfile->IsPartFile())
							if( theApp.glob_prefs->GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount() ) //<<--
								theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile,client);
						
						// check to see if this is a new file they are asking for
						if(md4cmp(client->GetUploadFileID(), packet) != 0)
							client->SetCommentDirty();

//						CKnownFile* clientreqfile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
//						if( clientreqfile )
//							clientreqfile->SubQueuedCount();
//						reqfile->AddQueuedCount();
						// send filename etc
						client->SetUploadFileID((uchar*)packet);
//						md4cpy(client->reqfileid,packet);
						CSafeMemFile data(128);
						data.Write(reqfile->GetFileHash(),16);
						uint16 namelength = (uint16)strlen(reqfile->GetFileName());
						data.Write(&namelength,2);
						data.Write(reqfile->GetFileName(),namelength);
						// TODO: Don't let 'ProcessUpFileStatus' re-process the entire packet and search the fileid
						// again in 'sharedfiles' -> waste of time.
						client->ProcessUpFileStatus(packet,size);
						Packet* packet = new Packet(&data);
						packet->opcode = OP_FILEREQANSWER;
						theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
						SendPacket(packet,true);
						client->SendCommentInfo(reqfile);
						break;
					}
					throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
					break;
				}
				case OP_FILEREQANSNOFIL:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					// DbT:FileRequest
					if (size == 16) {
						// if that client do not have my file maybe has another different
						CPartFile* reqfile = theApp.downloadqueue->GetFileByID((uchar*)packet);
						if (!reqfile)
							break;

						// we try to swap to another file ignoring no needed parts files
						if (client){
							switch (client->GetDownloadState()) {
								case DS_CONNECTED:
							case DS_ONQUEUE:
							case DS_NONEEDEDPARTS:
								if (!client->SwapToAnotherFile(true, true, true, NULL)) {
									theApp.downloadqueue->RemoveSource(client, true);
							}
							break;
						}
						}
						break;
					}
					throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
					break;
					// DbT:End
				}
				case OP_FILEREQANSWER:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					client->ProcessFileInfo(packet,size);
					break;
				}
				case OP_FILESTATUS:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					client->ProcessFileStatus(packet,size);
					break;
				}
				case OP_STARTUPLOADREQ:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					if( size == 16 ){
						uchar reqfileid[16];
						md4cpy(reqfileid,packet);
						CKnownFile* reqfile = theApp.sharedfiles->GetFileByID(reqfileid);
						if (reqfile){
							if (reqfile->IsPartFile())
								if( theApp.glob_prefs->GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount() ) //<<--
									theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile,client);
							if(md4cmp(client->GetUploadFileID(), packet) != 0)
								client->SetCommentDirty();
							client->SetUploadFileID((uchar*)packet);
							client->SendCommentInfo(reqfile);
						}
					}
					theApp.uploadqueue->AddClientToQueue(client);
					break;
				}
				case OP_QUEUERANK:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					CSafeMemFile data((BYTE*)packet,size);
					uint32 rank;
					data.Read(&rank,4);
					client->SetRemoteQueueRank(rank);
					break;
				}
				case OP_ACCEPTUPLOADREQ:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
				//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
					//if (client->reqfile && !client->reqfile->IsStopped() && 
					if (client->reqfile && !client->reqfile->IsStopped() && (client->reqfile->lastseencomplete!=NULL || !app_prefs->OnlyDownloadCompleteFiles()) /*shadow#(onlydownloadcompletefiles)*/ &&
				//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
						(client->reqfile->GetStatus()==PS_READY || client->reqfile->GetStatus()==PS_EMPTY)){
						if (client->GetDownloadState() == DS_ONQUEUE){
							client->SetDownloadState(DS_DOWNLOADING);
							// -khaos--+++> Set our bool to false.  When (if) we receive a block from this client, it will be set to true.
							//				When the download state is changed from DS_DOWNLOADING to something else, we will check this
							//				bool and if it's true and new state != DS_ERROR, then it was a successful session.  Else failed.
							client->InitTransferredDownMini();
							client->SetDownStartTime();
							// <-----khaos- End Statistics Stuff
							
							client->m_lastPartAsked = 0xffff; // Reset current downloaded Chunk // Maella -Enhanced Chunk Selection- (based on jicxicmic)

							client->SendBlockRequests();
						}
					}
					else{
						Packet* packet = new Packet(OP_CANCELTRANSFER,0);
						theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
						client->socket->SendPacket(packet,true,true);
						client->SetDownloadState((client->reqfile==NULL || client->reqfile->IsStopped()) ? DS_NONE : DS_ONQUEUE);
					}
					break;
				}
				case OP_REQUESTPARTS:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					CSafeMemFile data((BYTE*)packet,size);
					uchar reqfilehash[16];
					data.Read(reqfilehash,16);
					Requested_Block_Struct* reqblock1 = new Requested_Block_Struct;
					Requested_Block_Struct* reqblock2 = new Requested_Block_Struct;
					Requested_Block_Struct* reqblock3 = new Requested_Block_Struct;
					data.Read(&reqblock1->StartOffset,4);
					data.Read(&reqblock2->StartOffset,4);
					data.Read(&reqblock3->StartOffset,4);
					data.Read(&reqblock1->EndOffset,4);
					data.Read(&reqblock2->EndOffset,4);
					data.Read(&reqblock3->EndOffset,4);
					md4cpy(&reqblock1->FileID,reqfilehash);
					md4cpy(&reqblock2->FileID,reqfilehash);
					md4cpy(&reqblock3->FileID,reqfilehash);
					
					//MORPH START - Added by SiRoB, ZZ Upload System 20030723-0133
					uchar tempfileid[16];
                    			md4clr(tempfileid);
					//MORPH START - Added by SiRoB, Anti-leecher feature
					if(client->IsLeecher()) {
						// Flag blocks to delete
						reqblock1->StartOffset = 0; reqblock1->EndOffset = 0; 
						reqblock2->StartOffset = 0; reqblock2->EndOffset = 0; 
						reqblock3->StartOffset = 0; reqblock3->EndOffset = 0; 
					
						// Remove client from the upload queue
						theApp.uploadqueue->RemoveFromUploadQueue(client,GetResString(IDS_UPSTOPPEDLEECHER), true, true);
						theApp.emuledlg->AddDebugLogLine(false, GetResString(IDS_LEECHERDETREM));
						
						theApp.uploadqueue->AddClientToQueue(client);
						theApp.emuledlg->AddDebugLogLine(false, GetResString(IDS_LEECHERPUTBACK));
					
						client->SetUploadFileID(reqblock1->FileID);
					}
					
					//MORPH END   - Added by SiRoB, Anti-leecher feature
					
					// Remark: There is a security leak that a leecher mod might exploit here.
					//         A client might send reqblock for another file than the one it 
					//         was granted to download. As long as the file ID in reqblock
					//         is the same in all reqblocks, it won't be rejected.  
					//         With this a client might be in a waiting queue with a high 
					//         priority but download block of a file set to a lower priority.
					else if(md4cmp(reqblock1->FileID, client->GetUploadFileID()) != 0 && md4cmp(reqblock1->FileID, tempfileid) != 0 && client->GetSessionUp() == 0){
						// client requested another file than it queued up for. Try to decide if the swith should be allowed.
						bool allowSwitch = false;

						// save original file id asked for, to be able to log it
						uchar uploadFileId[16];
						md4cpy(uploadFileId, client->GetUploadFileID());

						if(client->HasLowID() && (client->IsFriend() && client->GetFriendSlot()) == true) {
							allowSwitch = true;
						} else {
							client->SetUploadFileID(reqblock1->FileID);

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
							reqblock1->StartOffset = 0; reqblock1->EndOffset = 0; 
							reqblock2->StartOffset = 0; reqblock2->EndOffset = 0; 
							reqblock3->StartOffset = 0; reqblock3->EndOffset = 0; 
						
							// Ban client and trace some info
							theApp.emuledlg->AddDebugLogLine(false, GetResString(IDS_TRIEDDLOTHERFILE), 
								client->GetUserName(), md4str(uploadFileId), md4str(reqblock1->FileID));

							// Remove client from the upload queue
							theApp.uploadqueue->RemoveFromUploadQueue(client, GetResString(IDS_IPSTOPPEDOTHFILE), true, true);
	
							// Put back with wating time intact
							theApp.uploadqueue->AddClientToQueue(client, true, true);

							theApp.emuledlg->AddDebugLogLine(false, GetResString(IDS_CLIENTPUTBACK));
						
							client->SetUploadFileID(reqblock1->FileID);
						}
					}
					//MORPH END - Added by SiRoB, ZZ Upload System
						
					if (reqblock1->EndOffset-reqblock1->StartOffset == 0)			
						delete reqblock1;
					else
						client->AddReqBlock(reqblock1);

					if (reqblock2->EndOffset-reqblock2->StartOffset == 0)			
						delete reqblock2;
					else
						client->AddReqBlock(reqblock2);

					if (reqblock3->EndOffset-reqblock3->StartOffset == 0)			
						delete reqblock3;
					else
						client->AddReqBlock(reqblock3);
					break;
				}
				case OP_CANCELTRANSFER:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					theApp.uploadqueue->RemoveFromUploadQueue(client,false);
					//MORPH START - Changed by SiRoB, ZZ Patch
					uint32 ip = client->GetIP();
					uchar tempfileid[16];
					md4clr(tempfileid);
					if(md4cmp(client->GetUploadFileID(), tempfileid) != 0) { // to prevent spam in log from broken client
					AddDebugLogLine(false, "%s: Upload session ended due canceled transfer. %s %s:%i", client->GetUserName(), md4str(client->GetUploadFileID()), inet_ntoa(*(in_addr*)&ip), client->GetUserPort());
					client->SetUploadFileID(NULL);
					}
					//MORPH END - Changed by SiRoB, ZZ Patch
					break;
				}
				case OP_END_OF_DOWNLOAD:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					if (size>=16 && !md4cmp(client->GetUploadFileID(),packet)){
						theApp.uploadqueue->RemoveFromUploadQueue(client, GetResString(IDS_REMULUSEREND));
						AddDebugLogLine(false, GetResString(IDS_ULENDENDTRANS), client->GetUserName());
						client->SetUploadFileID(NULL);
					}
					break;
				}
				case OP_HASHSETREQUEST:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					if (size != 16)
						throw GetResString(IDS_ERR_WRONGHPACKAGESIZE);
					client->SendHashsetPacket(packet);
					break;
				}
				case OP_HASHSETANSWER:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					client->ProcessHashSet(packet,size);
					break;
				}
				case OP_SENDINGPART:{
//					theApp.downloadqueue->AddDownDataOverheadOther(0, 24);
					//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
					//if (client->reqfile && !client->reqfile->IsStopped() && 
					if (client->reqfile && !client->reqfile->IsStopped() && (client->reqfile->lastseencomplete!=NULL || !app_prefs->OnlyDownloadCompleteFiles())/*shadow#(onlydownloadcompletefiles)*/ &&
					//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
					(client->reqfile->GetStatus()==PS_READY || client->reqfile->GetStatus()==PS_EMPTY)){
						client->ProcessBlockPacket(packet,size);
						//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
						if (client->reqfile->IsStopped() || client->reqfile->GetStatus()==PS_PAUSED || client->reqfile->GetStatus()==PS_ERROR || (client->reqfile->lastseencomplete==NULL && app_prefs->OnlyDownloadCompleteFiles())){
						//if (client->reqfile->IsStopped() || client->reqfile->GetStatus()==PS_PAUSED || client->reqfile->GetStatus()==PS_ERROR){
						//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)		
							Packet* packet = new Packet(OP_CANCELTRANSFER,0);
							theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
							client->socket->SendPacket(packet,true,true);
							client->SetDownloadState(client->reqfile->IsStopped() ? DS_NONE : DS_ONQUEUE);
						}
					}
					else{
						Packet* packet = new Packet(OP_CANCELTRANSFER,0);
						theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
						client->socket->SendPacket(packet,true,true);
						client->SetDownloadState((client->reqfile==NULL || client->reqfile->IsStopped()) ? DS_NONE : DS_ONQUEUE);
					}
					break;
				}
				case OP_OUTOFPARTREQS:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					if (client->GetDownloadState() == DS_DOWNLOADING)
						client->SetDownloadState(DS_ONQUEUE);
					break;
				}
				case OP_SETREQFILEID:{
					theApp.downloadqueue->AddDownDataOverheadFileRequest(size);
					// IP banned, no answer for this request
					if (client->IsBanned() ){
						break;						
					}
					// DbT:FileRequest
					if (size == 16){
						if (!client->GetWaitStartTime())
							client->SetWaitStartTime();

						CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)packet);
						if (!reqfile){
							// send file request no such file packet (0x48)
							Packet* replypacket = new Packet(OP_FILEREQANSNOFIL, 16);
							md4cpy(replypacket->pBuffer, packet);
							theApp.uploadqueue->AddUpDataOverheadFileRequest(replypacket->size);
							SendPacket(replypacket, true);
							break;
						}

						// if we are downloading this file, this could be a new source
						if (reqfile->IsPartFile())
							if( theApp.glob_prefs->GetMaxSourcePerFile() > ((CPartFile*)reqfile)->GetSourceCount() )
								theApp.downloadqueue->CheckAndAddKnownSource((CPartFile*)reqfile,client);
					            
						// check to see if this is a new file they are asking for
						if(md4cmp(client->GetUploadFileID(), packet) != 0)
							client->SetCommentDirty();

//						CKnownFile* clientreqfile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
//						if( clientreqfile )
//							clientreqfile->SubQueuedCount();
//						reqfile->AddQueuedCount();

						//send filestatus
						client->SetUploadFileID((uchar*)packet);
//						md4cpy(client->reqfileid,packet);
						CSafeMemFile data(16+16);
						data.Write(reqfile->GetFileHash(),16);
						if (reqfile->IsPartFile())
							((CPartFile*)reqfile)->WritePartStatus(&data, client);	// SLUGFILLER: hideOS
						else if (!reqfile->ShareOnlyTheNeed(&data)){ //wistily SOTN
							if (!reqfile->HideOvershares(&data, client)){	//Slugfiller: HideOS
								uint32 null = 0;
								data.Write(&null,3);
							}
						}
						Packet* packet = new Packet(&data);
						packet->opcode = OP_FILESTATUS;
						theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
						SendPacket(packet,true);
						break;
					}
					throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
					break;
					// DbT:End
				}
				case OP_CHANGE_CLIENT_ID:{
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					CSafeMemFile data((BYTE*)packet, size);
					uint32 nNewUserID;
					data.Read(&nNewUserID, 4);
					uint32 nNewServerIP;
					data.Read(&nNewServerIP, 4);
					if (IsLowIDED2K(nNewUserID)){ // client changed server and gots a LowID
						CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
						if (pNewServer != NULL){
							client->SetUserIDHybrid(nNewUserID); // update UserID only if we know the server
							client->SetServerIP(nNewServerIP);
							client->SetServerPort(pNewServer->GetPort());
						}
					}
					else if (ntohl(nNewUserID) == client->GetIP()){ // client changed server and gots a HighID(IP)
						client->SetUserIDHybrid(ntohl(nNewUserID));
						CServer* pNewServer = theApp.serverlist->GetServerByIP(nNewServerIP);
						if (pNewServer != NULL){
							client->SetServerIP(nNewServerIP);
							client->SetServerPort(pNewServer->GetPort());
						}
					}
					break;
				}
				case OP_CHANGE_SLOT:{
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					// sometimes sent by Hybrid
					break;
				}
				case OP_MESSAGE:{
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					if (size < 2)
						throw CString(_T("invalid message packet"));
					uint16 length;
					memcpy(&length,packet,2);
					if ((UINT)(length+2) != size)
						throw CString(_T("invalid message packet"));

					AddDebugLogLine(true,GetResString(IDS_NEWMSGFROM),client->GetUserName(), client->GetFullIP());
					//filter me?
					if ( (theApp.glob_prefs->MsgOnlyFriends() && !client->IsFriend()) ||
						 (theApp.glob_prefs->MsgOnlySecure() && client->GetUserName()==NULL) ) {
						if (!client->m_bMsgFiltered)
							AddDebugLogLine(false,GetResString(IDS_FILTEREDFROM),client->GetUserName(), client->GetFullIP());
						client->m_bMsgFiltered=true;
						break;
					}
					char* message = new char[length+1];
					memcpy(message,packet+2,length);
					message[length] = '\0';
					theApp.emuledlg->chatwnd.chatselector.ProcessMessage(client,message);
					delete[] message;
					break;
				}
				case OP_ASKSHAREDFILES:	{	// client wants to know what we have in share, let's see if we allow him to know that
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					
					// IP banned, no answer for this request
					if (client->IsBanned() ){
						break;						
					}

					CPtrList list;
					if (theApp.glob_prefs->CanSeeShares() == 0 ||					// everybody
						(theApp.glob_prefs->CanSeeShares() == 1 && client->IsFriend() ) )	// friend
					{
						CCKey bufKey;
						CKnownFile* cur_file;
						for (POSITION pos = theApp.sharedfiles->m_Files_map.GetStartPosition();pos != 0;){
							theApp.sharedfiles->m_Files_map.GetNextAssoc(pos,bufKey,cur_file);
							list.AddTail((void*&)cur_file);
						}
						AddLogLine(true,GetResString(IDS_REQ_SHAREDFILES),client->GetUserName(),client->GetUserIDHybrid(),GetResString(IDS_ACCEPTED) );
					} else AddLogLine(true,GetResString(IDS_REQ_SHAREDFILES),client->GetUserName(),client->GetUserIDHybrid(),GetResString(IDS_DENIED) );

					// now create the memfile for the packet
					uint32 iTotalCount = list.GetCount();
					CSafeMemFile tempfile(80);
					tempfile.Write(&iTotalCount, 4);
					while (list.GetCount())
					{
						theApp.sharedfiles->CreateOfferedFilePacket((CKnownFile*)list.GetHead(), &tempfile, false);
						list.RemoveHead();
					}
					// create a packet and send it
					Packet* replypacket = new Packet(&tempfile);
					replypacket->opcode = OP_ASKSHAREDFILESANSWER;
					theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
					SendPacket(replypacket, true, true);
					break;
				}
				case OP_ASKSHAREDFILESANSWER:{
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					client->ProcessSharedFileList(packet,size);
					break;
				}
                case OP_ASKSHAREDDIRS:{
                    theApp.downloadqueue->AddDownDataOverheadOther(size);
                    ASSERT( size == 0 );
					
					// IP banned, no answer for this request
					if (client->IsBanned() ){
						break;						
					}

                    if (theApp.glob_prefs->CanSeeShares()==0 || (theApp.glob_prefs->CanSeeShares()==1 && client->IsFriend())){
						AddLogLine(true,GetResString(IDS_SHAREDREQ1),client->GetUserName(),client->GetUserIDHybrid(),GetResString(IDS_ACCEPTED) );
 
						CString strDir,strTest;

						//TODO: Don't send shared directories which do not contain any files

						// collect folders
						CArray<CString, CString&> arFolders;
                        POSITION pos = theApp.glob_prefs->shareddir_list.GetHeadPosition();
                        while (pos){
                            strDir = theApp.glob_prefs->shareddir_list.GetNext(pos);
                            PathRemoveBackslash(strDir.GetBuffer());
                            strDir.ReleaseBuffer();
							
							bool bFoundFolder=false;
							for (int ix=0;ix<arFolders.GetCount();ix++) {
								strTest=arFolders.GetAt(ix);
								if (strDir.CompareNoCase(strTest)==0) {
									bFoundFolder=true;
									break;
								}
							}
							if (!bFoundFolder)
								arFolders.Add(strDir);
						}
						
						// and the incoming folders
                       	for (int iCat=0;iCat<theApp.glob_prefs->GetCatCount();iCat++) {
							strDir = theApp.glob_prefs->GetCategory(iCat)->incomingpath;
							PathRemoveBackslash(strDir.GetBuffer());
							strDir.ReleaseBuffer();
							
							bool bFoundFolder=false;
							for (int ix=0;ix<arFolders.GetCount();ix++) {
								strTest=arFolders.GetAt(ix);
								if (strDir.CompareNoCase(strTest)==0) {
									bFoundFolder=true;
									break;
								}
							}
							if (!bFoundFolder)
								arFolders.Add(strDir);
						}
						
						//build packet
                        CSafeMemFile tempfile(80);
						uint32 uDirs = arFolders.GetCount();
                        tempfile.Write(&uDirs, 4);
						for (int ix=0;ix<arFolders.GetCount();ix++)
                        {
							strDir = arFolders.GetAt(ix);
                            uint16 cnt = strDir.GetLength();
                            tempfile.Write(&cnt, 2);
                            tempfile.Write((LPCTSTR)strDir, cnt);
                        }

						Packet* replypacket = new Packet(&tempfile);
                        replypacket->opcode = OP_ASKSHAREDDIRSANS;
                        theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
                        SendPacket(replypacket, true, true);
					}
					else{
						AddLogLine(true,GetResString(IDS_SHAREDREQ1),client->GetUserName(),client->GetUserIDHybrid(),GetResString(IDS_DENIED) );
                        Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
						theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
                        SendPacket(replypacket, true, true);
                    }
                    break;
                }
                case OP_ASKSHAREDFILESDIR:{
                    theApp.downloadqueue->AddDownDataOverheadOther(size);
					
					// IP banned, no answer for this request
					if (client->IsBanned() ){
						break;						
					}

                    CSafeMemFile data((uchar*)packet, size);
                    uint16 cnt;
                    data.Read(&cnt, 2);
                    CString strReqDir;
                    data.Read(strReqDir.GetBuffer(cnt), cnt);
                    strReqDir.ReleaseBuffer(cnt);
                    PathRemoveBackslash(strReqDir.GetBuffer());
                    strReqDir.ReleaseBuffer();
 
                    if (theApp.glob_prefs->CanSeeShares()==0 || (theApp.glob_prefs->CanSeeShares()==1 && client->IsFriend())){
						AddLogLine(true,GetResString(IDS_SHAREDREQ2),client->GetUserName(),client->GetUserIDHybrid(),strReqDir,GetResString(IDS_ACCEPTED) );
                        ASSERT( data.GetPosition() == data.GetLength() );
 
                        CTypedPtrList<CPtrList, CKnownFile*> list;
                        for (POSITION pos = theApp.sharedfiles->m_Files_map.GetStartPosition();pos != 0;){
                            CCKey bufKey;
                            CKnownFile* cur_file;
                            theApp.sharedfiles->m_Files_map.GetNextAssoc(pos, bufKey, cur_file);
							CString strSharedFileDir(cur_file->GetPath());
							PathRemoveBackslash(strSharedFileDir.GetBuffer());
							strSharedFileDir.ReleaseBuffer();
							if (strReqDir.CompareNoCase(strSharedFileDir) == 0)
                                list.AddTail(cur_file);
                        }
 
						// Currently we are sending each shared directory, even if it does not contain any files.
						// Because of this we also have to send an empty shared files list..
						/*if (list.GetCount())*/{
                            CSafeMemFile tempfile(80);
                            uint16 cnt = strReqDir.GetLength();
                            tempfile.Write(&cnt, 2);
                            tempfile.Write((LPCTSTR)strReqDir, cnt);
                            uint32 uFiles = list.GetCount();
                            tempfile.Write(&uFiles, 4);
                            while (list.GetCount()){
                                theApp.sharedfiles->CreateOfferedFilePacket(list.GetHead(), &tempfile, false);
                                list.RemoveHead();
                            }
 
                            Packet* replypacket = new Packet(&tempfile);
                            replypacket->opcode = OP_ASKSHAREDFILESDIRANS;
                            theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
                            SendPacket(replypacket, true, true);
                        }
                    }
                    else{
						AddLogLine(true,GetResString(IDS_SHAREDREQ2),client->GetUserName(),client->GetUserIDHybrid(),strReqDir,GetResString(IDS_DENIED) );
                        Packet* replypacket = new Packet(OP_ASKSHAREDDENIEDANS, 0);
                        theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
                        SendPacket(replypacket, true, true);
                    }
                    break;
                }
                case OP_ASKSHAREDDIRSANS:{
                    theApp.downloadqueue->AddDownDataOverheadOther(size);
                    if (client->GetFileListRequested() == 1){
                        CSafeMemFile data((uchar*)packet, size);
                        uint32 uDirs;
                        data.Read(&uDirs, 4);
                        for (UINT i = 0; i < uDirs; i++){
                            uint16 cnt;
                            data.Read(&cnt, 2);
                            CString strDir;
                            data.Read(strDir.GetBuffer(cnt), cnt);
                            strDir.ReleaseBuffer(cnt);
							// Better send the received and untouched directory string back to that client
							//PathRemoveBackslash(strDir.GetBuffer());
							//strDir.ReleaseBuffer();
							AddLogLine(true,GetResString(IDS_SHAREDANSW),client->GetUserName(),client->GetUserIDHybrid(),strDir);

                            CSafeMemFile tempfile(80);
                            cnt = strDir.GetLength();
                            tempfile.Write(&cnt, 2);
                            tempfile.Write((LPCTSTR)strDir, cnt);
                            Packet* replypacket = new Packet(&tempfile);
                            replypacket->opcode = OP_ASKSHAREDFILESDIR;
                            theApp.uploadqueue->AddUpDataOverheadOther(replypacket->size);
                            SendPacket(replypacket, true, true);
                        }
                        ASSERT( data.GetPosition() == data.GetLength() );
                        client->SetFileListRequested(uDirs);
                    }
					else
						AddLogLine(true,GetResString(IDS_SHAREDANSW2),client->GetUserName(),client->GetUserIDHybrid());
                    break;
                }
                case OP_ASKSHAREDFILESDIRANS:{
                    theApp.downloadqueue->AddDownDataOverheadOther(size);
                    CSafeMemFile data((uchar*)packet, size);
                    uint16 cnt;
                    data.Read(&cnt, 2);
                    CString strDir;
                    data.Read(strDir.GetBuffer(cnt), cnt);
                    strDir.ReleaseBuffer(cnt);
					PathRemoveBackslash(strDir.GetBuffer());
					strDir.ReleaseBuffer();
                    if (client->GetFileListRequested() > 0){
						AddLogLine(true,GetResString(IDS_SHAREDINFO1),client->GetUserName(),client->GetUserIDHybrid(),strDir);
						client->ProcessSharedFileList(packet + data.GetPosition(), size - data.GetPosition(), strDir);
						if (client->GetFileListRequested() == 0)
							AddLogLine(true,GetResString(IDS_SHAREDINFO2),client->GetUserName(),client->GetUserIDHybrid());
                    }
					else
						AddLogLine(true,GetResString(IDS_SHAREDANSW3),client->GetUserName(),client->GetUserIDHybrid(),strDir);
                    break;
                }
                case OP_ASKSHAREDDENIEDANS:{
                    theApp.downloadqueue->AddDownDataOverheadOther(size);
                    ASSERT( size == 0 );
					AddLogLine(true,GetResString(IDS_SHAREDREQDENIED),client->GetUserName(),client->GetUserIDHybrid());
					client->SetFileListRequested(0);
                    break;
                }

				default:
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					PacketToDebugLogLine("eDonkey", packet, size, opcode);
					break;
			}
		}
		catch(CFileException* error){
			OUTPUT_DEBUG_TRACE();
			error->Delete();	//mf
			throw GetResString(IDS_ERR_INVALIDPACKAGE);
		}
		catch(CMemoryException* error){
			OUTPUT_DEBUG_TRACE();
			error->Delete();
			throw CString(_T("Memory exception"));
		}
	}
	catch(CString error){
		OUTPUT_DEBUG_TRACE();
		if (client){
			client->SetDownloadState(DS_ERROR);	
			AddDebugLogLine(false,GetResString(IDS_ERR_CLIENTERROR),client->GetUserName(),client->GetFullIP(),error.GetBuffer());
		}
		else
			AddDebugLogLine(false,GetResString(IDS_ERR_BADCLIENTACTION),error.GetBuffer());
		//MORPH START - Added by SiRoB, ZZ Upload system 200308181923
		Disconnect(GetResString(IDS_ERRORPROCESSING) + error);
		//MORPH END   - Added by SiRoB, ZZ Upload system 200308181923
		return false;
	}
	return true;
}

bool CClientReqSocket::ProcessExtPacket(char* packet, uint32 size, UINT opcode){
	try{
		try{
			if (!client){
				//theApp.downloadqueue->AddDownDataOverheadOther(size);
				throw GetResString(IDS_ERR_UNKNOWNCLIENTACTION);
			}
			switch(opcode){
				case OP_EMULEINFO:{
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					client->ProcessMuleInfoPacket(packet,size);

					// start secure identification, if
					//  - we have received eD2K and eMule info (old eMule)
					if (client->GetInfoPacketsReceived() == IP_BOTH)
						client->InfoPacketsReceived();

					client->SendMuleInfoPacket(true);
					break;
				}
				case OP_EMULEINFOANSWER:{
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					client->ProcessMuleInfoPacket(packet,size);

					// start secure identification, if
					//  - we have received eD2K and eMule info (old eMule)
					if (client->GetInfoPacketsReceived() == IP_BOTH)
						client->InfoPacketsReceived();
					break;
				}
				case OP_SECIDENTSTATE:{
					client->ProcessSecIdentStatePacket((uchar*)packet,size);
					if (client->GetSecureIdentState() == IS_SIGNATURENEEDED)
						client->SendSignaturePacket();
					else if (client->GetSecureIdentState() == IS_KEYANDSIGNEEDED){
						client->SendPublicKeyPacket();
						client->SendSignaturePacket();
					}
					break;
				}
				case OP_PUBLICKEY:{
					if (client->IsBanned() ){
						break;						
					}
					client->ProcessPublicKeyPacket((uchar*)packet,size);
					break;
				}
  				case OP_SIGNATURE:{
					client->ProcessSignaturePacket((uchar*)packet,size);
					break;
				}
				case OP_COMPRESSEDPART:{
	//				theApp.downloadqueue->AddDownDataOverheadOther(24);
					if (client->reqfile && !client->reqfile->IsStopped() && 
						(client->reqfile->GetStatus()==PS_READY || client->reqfile->GetStatus()==PS_EMPTY)){
						client->ProcessBlockPacket(packet,size,true);
						if (client->reqfile->IsStopped() || client->reqfile->GetStatus()==PS_PAUSED || client->reqfile->GetStatus()==PS_ERROR){
							Packet* packet = new Packet(OP_CANCELTRANSFER,0);
							theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
							client->socket->SendPacket(packet,true,true);
							client->SetDownloadState(client->reqfile->IsStopped() ? DS_NONE : DS_ONQUEUE);
						}
					}
					else{
						Packet* packet = new Packet(OP_CANCELTRANSFER,0);
						theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
						client->socket->SendPacket(packet,true,true);
						client->SetDownloadState((client->reqfile==NULL || client->reqfile->IsStopped()) ? DS_NONE : DS_ONQUEUE);
					}
					break;
				}
				case OP_QUEUERANKING:{
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					if (size != 12)
						throw GetResString(IDS_ERR_BADSIZE);
					uint16 newrank;
					memcpy(&newrank,packet+0,2);
					client->SetRemoteQueueFull(false);
					client->SetRemoteQueueRank(newrank);
					break;
				}
 				case OP_REQUESTSOURCES:{
					theApp.downloadqueue->AddDownDataOverheadSourceExchange(size);
					if (client->GetSourceExchangeVersion() > 1){
						if(size != 16)
							throw GetResString(IDS_ERR_BADSIZE);
		
						//first check shared file list, then download list
						CKnownFile* file = theApp.sharedfiles->GetFileByID((uchar*)packet);
						if(!file)
							file = theApp.downloadqueue->GetFileByID((uchar*)packet);
		
						if(file) {
							DWORD dwTimePassed = ::GetTickCount() - client->GetLastSrcReqTime() + CONNECTION_LATENCY;
							bool bNeverAskedBefore = client->GetLastSrcReqTime() == 0;
		
							if( 
								//if not complete and file is rare, allow once every 10 minutes
								( file->IsPartFile() &&
								((CPartFile*)file)->GetSourceCount() <= RARE_FILE * 2 &&
								(bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASK)
								) ||
								//OR if file is not rare or if file is complete, allow every 90 minutes
								( (bNeverAskedBefore || dwTimePassed > SOURCECLIENTREASK * MINCOMMONPENALTY) )
							) {
		
								client->SetLastSrcReqTime();
								Packet* tosend = file->CreateSrcInfoPacket(client);
								if(tosend){
									if ( theApp.glob_prefs->GetDebugSourceExchange() )
										AddDebugLogLine( false, GetResString(IDS_RECEIVEDSRCEXREQ), client->GetUserName(), file->GetFileName() );
									theApp.uploadqueue->AddUpDataOverheadSourceExchange(tosend->size);
									SendPacket(tosend, true, true);
								}
							}
						}
					}
					break;
				}
 				case OP_ANSWERSOURCES:{
					theApp.downloadqueue->AddDownDataOverheadSourceExchange(size);
					CSafeMemFile data((BYTE*)packet,size);
					uchar hash[16];
					data.Read(hash,16);
					CKnownFile* file = theApp.downloadqueue->GetFileByID((uchar*)packet);
					if(file){
						if (file->IsPartFile()){
							//set the client's answer time
							client->SetLastSrcAnswerTime();
							//and set the file's last answer time
							((CPartFile*)file)->SetLastAnsweredTime();
	
							((CPartFile*)file)->AddClientSources(&data, client->GetSourceExchangeVersion());
						}
					}
					break;
				}
				case OP_FILEDESC:{
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					client->ProcessMuleCommentPacket(packet,size);
					break;
				}
				case OP_REQUESTPREVIEW:{
					// IP banned, no answer for this request
					if (client->IsBanned() ){
						break;						
					}

					if (theApp.glob_prefs->CanSeeShares() == 0 					// everybody
						|| (theApp.glob_prefs->CanSeeShares() == 1 && client->IsFriend() )// friend
						&& theApp.glob_prefs->IsPreviewEnabled() )	
					{
						client->ProcessPreviewReq(packet,size);	
						AddDebugLogLine(true,"Client '%s' (%s) requested Preview - accepted",client->GetUserName(),client->GetFullIP());
					}
					else{
						// we don't send any answer here, because the client should know that he was not allowed to ask
						AddDebugLogLine(true,"Client '%s' (%s) requested Preview - denied",client->GetUserName(),client->GetFullIP());
					}
					break;
				}
				case OP_PREVIEWANSWER:{
					client->ProcessPreviewAnswer(packet, size);
					break;
				}
				default:
					theApp.downloadqueue->AddDownDataOverheadOther(size);
					PacketToDebugLogLine("eMule", packet, size, opcode);
					break;
			}
		}
		catch(CFileException* error){
			OUTPUT_DEBUG_TRACE();
			error->Delete();
			throw GetResString(IDS_ERR_INVALIDPACKAGE);
		}
		catch(CMemoryException* error){
			OUTPUT_DEBUG_TRACE();
			error->Delete();
			throw CString(_T("Memory exception"));
		}
	}
	catch(CString error){
		OUTPUT_DEBUG_TRACE();
		AddDebugLogLine(false,GetResString(IDS_ERR_BADCLIENTACTION),error.GetBuffer());
		if (client)
			client->SetDownloadState(DS_ERROR);
		//MORPH START - Added by SiRoB, ZZ Upload system 200308181923
		Disconnect("ProcessExtPacket error. " + error);
		//MORPH END   - Added by SiRoB, ZZ Upload system 200308181923
		return false;
	}
	return true;
}

void CClientReqSocket::PacketToDebugLogLine(const char* protocol, const char* packet, uint32 size, UINT opcode) const {
	CString buffer; 
	buffer.Format(_T("unknown %s opcode: 0x%02x, size=%u"), protocol, opcode, size);
	buffer += ", data=[";
	uint32 i = 0;
	for(; i < size && i < 50; i++){
		char temp[3];
		sprintf(temp,"%02x",(uint8)packet[i]);
		buffer += temp;
		buffer += " ";
	}
	buffer += (const char*)((i == size) ? "]" : "..]");
	AddDebugLogLine(false, buffer); 
}

void CClientReqSocket::OnInit(){
	//uint8 tv = 1;
	//SetSockOpt(SO_DONTLINGER,&tv,sizeof(BOOL));
}

void CClientReqSocket::OnSend(int nErrorCode){
	ResetTimeOutTimer();
	CEMSocket::OnSend(nErrorCode);
}

void CClientReqSocket::OnError(int nErrorCode){

	if (client)
		AddDebugLogLine(false,GetResString(IDS_ERR_BADCLIENT2),client->GetUserName(),client->GetFullIP(),nErrorCode);
	else
		AddDebugLogLine(false,GetResString(IDS_ERR_BADCLIENTACTION),GetErrorMessage(nErrorCode));
	//MORPH START - Added by SiRoB, ZZ Upload system 200308181923
	CString reason;
	reason.Format("OnError. error code: %i", nErrorCode);
	Disconnect(reason);
	//MORPH END   - Added by SiRoB, ZZ Upload system 200308181923
}

void CClientReqSocket::PacketReceivedCppEx(Packet* packet){
#ifndef _DEBUG
	try{
#endif
		switch (packet->prot){
			case OP_EDONKEYPROT:
				ProcessPacket(packet->pBuffer,packet->size,packet->opcode);
				break;
			case OP_PACKEDPROT:
				if (!packet->UnPackPacket()){
					SOCKADDR_IN sockAddr;
					memset(&sockAddr, 0, sizeof(sockAddr));
					int nSockAddrLen = sizeof(sockAddr);
					GetPeerName((SOCKADDR*)&sockAddr,&nSockAddrLen);
					AddDebugLogLine(false,_T("Failed to decompress client TCP packet; IP=%s  protocol=0x%02x  opcode=0x%02x  size=%u"), inet_ntoa(sockAddr.sin_addr), packet->prot, packet->opcode, packet->size);
					break;
				}
			case OP_EMULEPROT:
				ProcessExtPacket(packet->pBuffer,packet->size,packet->opcode);
				break;
		    default:{
			    SOCKADDR_IN sockAddr;
			    memset(&sockAddr, 0, sizeof(sockAddr));
			    int nSockAddrLen = sizeof(sockAddr);
			    GetPeerName((SOCKADDR*)&sockAddr,&nSockAddrLen);
			    AddDebugLogLine(false,_T("Received unknown client TCP packet; IP=%s  protocol=0x%02x  opcode=0x%02x  size=%u"), inet_ntoa(sockAddr.sin_addr), packet->prot, packet->opcode, packet->size);
				if (client)
					client->SetDownloadState(DS_ERROR);
				//MORPH START - Added by SiRoB, ZZ Upload system 200308181923
				Disconnect("Unknown TCP packet");
				//MORPH END   - Added by SiRoB, ZZ Upload system 200308181923
		    }
		}
#ifndef _DEBUG
	}
	catch(...){
		OUTPUT_DEBUG_TRACE();
		SOCKADDR_IN sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));
		int nSockAddrLen = sizeof(sockAddr);
		GetPeerName((SOCKADDR*)&sockAddr,&nSockAddrLen);
		AddDebugLogLine(false,_T("Unknown exception in CClientReqSocket::PacketReceived; IP=%s  protocol=0x%02x  opcode=0x%02x  size=%u"), inet_ntoa(sockAddr.sin_addr), packet?packet->prot:0, packet?packet->opcode:0, packet?packet->size:0);

		// TODO: This exception handler should definitively be *here*. Though we may get some very
		// strange socket deletion crashs if we disconnect a client's TCP socket on catching an exception
		// here. See also comments in 'CAsyncSocketExHelperWindow::WindowProc'
		//if (client)
		//	client->SetDownloadState(DS_ERROR);
		//Disconnect();
		throw; // pass the exception to the SEH
	}
#endif
}

int FilterSE(DWORD dwExCode, LPEXCEPTION_POINTERS pExPtrs)
{
	if (pExPtrs){
		const EXCEPTION_RECORD* er = pExPtrs->ExceptionRecord;
		CemuleApp::AddDebugLogLine(false,_T("Structured exception %08x in CClientReqSocket::PacketReceived at %08x"), er->ExceptionCode, er->ExceptionAddress);
	}
	else
		CemuleApp::AddDebugLogLine(false,_T("Structured exception %08x in CClientReqSocket::PacketReceived"), dwExCode);
	
	// this would continue the program "as usual"
	//return EXCEPTION_EXECUTE_HANDLER; 

	// this searches the next exception handler -> catch(...) in 'CAsyncSocketExHelperWindow::WindowProc'
	// as long as I do not know where any why we are crashing, I prefere to have it handled that way which worked fine in 28a/b
	return EXCEPTION_CONTINUE_SEARCH;
}

void CClientReqSocket::PacketReceived(Packet* packet){
	// this function is only here to get a chance of determining the crash address via SEH
	__try{
		PacketReceivedCppEx(packet);
	}
	__except(FilterSE(GetExceptionCode(), GetExceptionInformation())){
	}
}

void CClientReqSocket::OnReceive(int nErrorCode){
	ResetTimeOutTimer();
	CEMSocket::OnReceive(nErrorCode);
}

bool CClientReqSocket::Create(){
	theApp.listensocket->AddConnection();
	BOOL result = CAsyncSocketEx::Create(0,SOCK_STREAM,FD_WRITE|FD_READ|FD_CLOSE); // deadlake PROXYSUPPORT - changed to AsyncSocketEx
	OnInit();
	return result;
}

//MORPH START - Changed by SiRoB, Due to ZZ Upload System
bool CClientReqSocket::SendPacket(Packet* packet, bool delpacket, bool controlpacket,uint32 actualPayloadSize){
	ResetTimeOutTimer();
	return CEMSocket::SendPacket(packet,delpacket,controlpacket,actualPayloadSize);
}
//MORPH END   - Changed by SiRoB, Due to ZZ Upload System

//MORPH START - Added by SiRoB, ZZ Upload system 20030824-2238
SocketSentBytes CClientReqSocket::Send(uint32 maxNumberOfBytesToSend, bool onlyAllowedToSendControlPacket) {
    SocketSentBytes returnStatus = CEMSocket::Send(maxNumberOfBytesToSend, onlyAllowedToSendControlPacket);

    if(returnStatus.success && (returnStatus.sentBytesControlPackets > 0 || returnStatus.sentBytesStandardPackets > 0)) {
        ResetTimeOutTimer();
    }

    return returnStatus;
}
//MORPH END   - Added by SiRoB, ZZ Upload system 20030824-2238

// CListenSocket
// CListenSocket member functions
CListenSocket::CListenSocket(CPreferences* in_prefs){
	app_prefs = in_prefs;
	opensockets = 0;
	maxconnectionreached = 0;
	m_OpenSocketsInterval = 0;
	m_nPeningConnections = 0;
	per5average = 0;
}

CListenSocket::~CListenSocket(){
	Close();
	KillAllSockets();
}

bool CListenSocket::StartListening(){
	bListening = true;
	return (this->Create(app_prefs->GetPort(),SOCK_STREAM,FD_ACCEPT) && this->Listen());
}

void CListenSocket::ReStartListening(){
	bListening = true;
	if (m_nPeningConnections){
		m_nPeningConnections--;
		OnAccept(0);
	}
}

void CListenSocket::StopListening(){
	bListening = false;
	maxconnectionreached++;
}

void CListenSocket::OnAccept(int nErrorCode){
	if (!nErrorCode){
		m_nPeningConnections++;
		if (m_nPeningConnections < 1){
			ASSERT ( false );
			m_nPeningConnections = 1;
		}
		if (TooManySockets(true) && !theApp.serverconnect->IsConnecting()){
			StopListening();
			return;
		}
		else if ( bListening == false )
			ReStartListening(); //If the client is still at maxconnections, this will allow it to go above it.. But if you don't, you will get a lowID on all servers.
	
		while (m_nPeningConnections){
			m_nPeningConnections--;
			CClientReqSocket* newclient = new CClientReqSocket(app_prefs);
			if (!Accept(*newclient))
				newclient->Safe_Delete();
			else{
				newclient->AsyncSelect(FD_WRITE|FD_READ|FD_CLOSE);
				newclient->OnInit();
			}
			AddConnection();
		}
//		if (TooManySockets(true) && !theApp.serverconnect->IsConnecting())
//			StopListening();
	}
}

void CListenSocket::Process(){
	POSITION pos2;
	m_OpenSocketsInterval = 0;
	opensockets = 0;
	if (per5average)
		per5average /= 2;
	for(POSITION pos1 = socket_list.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
		socket_list.GetNext(pos1);
		CClientReqSocket* cur_sock = socket_list.GetAt(pos2);
		opensockets++;

	   if (cur_sock->deletethis){
		   if (cur_sock->m_SocketData.hSocket != INVALID_SOCKET){ // deadlake PROXYSUPPORT - changed to AsyncSocketEx
				cur_sock->Close();
		   }
		   else{
			   cur_sock->Delete_Timed();;
		   }
	   }
	   else
			socket_list.GetAt( pos2 )->CheckTimeOut();
   }
   if ( (GetOpenSockets()+5 < app_prefs->GetMaxConnections() || theApp.serverconnect->IsConnecting()) && !bListening)
	   ReStartListening();

/*/MORPH START - Added by Yun.SF3, Auto DynUp changing
   if (theApp.glob_prefs->IsAutoDynUpSwitching())
   {
	   if (GetOpenSockets() >= theApp.glob_prefs->MaxConnectionsSwitchBorder() + 50 && !theApp.glob_prefs->IsSUCEnabled())
		   SwitchSUC(true);
	   else if (GetOpenSockets() < theApp.glob_prefs->MaxConnectionsSwitchBorder() - 50 && !theApp.glob_prefs->IsDynUpEnabled())
		   SwitchSUC(false);
   }
//MORPH END - Added by Yun.SF3, Auto DynUp changing
*/
}

void CListenSocket::RecalculateStats(){
	memset(m_ConnectionStates,0,6);
	POSITION pos1,pos2;
	for(pos1 = socket_list.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
		socket_list.GetNext(pos1);
		CClientReqSocket* cur_sock = socket_list.GetAt(pos2);
		switch (cur_sock->GetConState()){
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
	POSITION pos2,pos1;
	for(pos1 = socket_list.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
       socket_list.GetNext(pos1);
	   if ( socket_list.GetAt(pos2) == todel )
			socket_list.RemoveAt(pos2);
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
	per5average++;
	m_OpenSocketsInterval++;
	opensockets++;
//MORPH START - Added by Yun.SF3, Auto DynUp changing
	if (theApp.glob_prefs->IsAutoDynUpSwitching())
		if (per5average >= theApp.glob_prefs->MaxConnectionsSwitchBorder() && !theApp.glob_prefs->IsSUCEnabled())
			SwitchSUC(true);
		else if (per5average <= 5 && !theApp.glob_prefs->IsDynUpEnabled())
			SwitchSUC(false);
//MORPH END - Added by Yun.SF3, Auto DynUp changing
}

bool CListenSocket::TooManySockets(bool bIgnoreInterval){
	if (GetOpenSockets() > app_prefs->GetMaxConnections() || (m_OpenSocketsInterval > (theApp.glob_prefs->GetMaxConperFive()*theApp.emuledlg->statisticswnd.GetMaxConperFiveModifier()) && !bIgnoreInterval) ){
		return true;
	}
	else
		return false;
}

bool CListenSocket::IsValidSocket(CClientReqSocket* totest){
	return socket_list.Find(totest);
}

void CListenSocket::Debug_ClientDeleted(CUpDownClient* deleted){
	POSITION pos1, pos2;
	for (pos1 = socket_list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		socket_list.GetNext(pos1);
		CClientReqSocket* cur_sock = socket_list.GetAt(pos2);
		if (!AfxIsValidAddress(cur_sock, sizeof(CClientReqSocket))) {
			AfxDebugBreak(); 
		}
		if (cur_sock->client == deleted){
			AfxDebugBreak();
		}
	}
}

//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
void CClientReqSocket::SmartUploadControl()
{
  // Smart Upload Control v2 (SUC) Version 17.3.2003 by [lovelace]
  // MaxVUR: maximum variable upload rate
  uint32 VUR=theApp.uploadqueue->GetMaxVUR();
  uint32 checkrespond=(::GetTickCount() - client->GetAskTime());
  
  uint32 VURstep=(((app_prefs->GetMaxUpload()*1024-theApp.uploadqueue->GetDatarate())>10000)
         ? (app_prefs->GetMaxUpload()*1024-theApp.uploadqueue->GetDatarate())/2 
         : UPLOAD_CHECK_CLIENT_DR/2);

  if (checkrespond<theApp.uploadqueue->GetAvgRespondTime(1))
    theApp.uploadqueue->SetAvgRespondTime(0,(uint32)(0.95*theApp.uploadqueue->GetAvgRespondTime(0)));
  else
    theApp.uploadqueue->SetAvgRespondTime(0,(uint32)((19*theApp.uploadqueue->GetAvgRespondTime(0)+1000)*0.05));
  if (checkrespond<theApp.glob_prefs->GetSUCPitch())
    theApp.uploadqueue->SetAvgRespondTime(1,(uint32)((499*theApp.uploadqueue->GetAvgRespondTime(1)+checkrespond)*0.002));

  if (theApp.uploadqueue->GetAvgRespondTime(0)>theApp.glob_prefs->GetSUCHigh())
    //lower
	theApp.uploadqueue->SetMaxVUR((uint32)(theApp.uploadqueue->GetDatarate()-UPLOAD_CHECK_CLIENT_DR/2));
  else if (theApp.uploadqueue->GetAvgRespondTime(0)<theApp.glob_prefs->GetSUCLow())
    {
    //higher
    if (VUR<(theApp.uploadqueue->GetDatarate()+VURstep))
	  theApp.uploadqueue->SetMaxVUR(theApp.uploadqueue->GetDatarate()+VURstep);
	}
  else
  { // slightly drift higher
    if (VUR<theApp.uploadqueue->GetDatarate()+theApp.glob_prefs->GetSUCDrift() && theApp.glob_prefs->GetSUCDrift())
	  theApp.uploadqueue->SetMaxVUR(theApp.uploadqueue->GetDatarate()+theApp.glob_prefs->GetSUCDrift());
	}
  if (theApp.glob_prefs->IsSUCLog ()) {
	theApp.emuledlg->AddDebugLogLine(false,"Smart Upload Control: (time:%ims, clip:%ims, ratio:%i) VUR:%iB/s",
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
		theApp.glob_prefs->SetDynUpEnabled(false);
		theApp.emuledlg->AddDebugLogLine(false,GetResString(IDS_SWITCHSUC));
		theApp.glob_prefs->SetSUCEnabled(true);
	}
	else{
		theApp.glob_prefs->SetSUCEnabled(false);
		theApp.emuledlg->AddDebugLogLine(false,GetResString(IDS_SWITCHDYNUP));
		theApp.glob_prefs->SetDynUpEnabled(true);

	}
}
//MORPH END - Added by Yun.SF3, Auto DynUp changing
