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

#include "StdAfx.h"
#include "clientlist.h"
#include "emule.h"
#include "otherfunctions.h"
#include "Kademlia/routing/contact.h"
#include "Kademlia/routing/timer.h"
#include "Kademlia/Kademlia/kademlia.h"
#include "Kademlia/net/kademliaudplistener.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CClientList::CClientList(){
	m_dwLastBannCleanUp = 0;
	m_dwLastTrackedCleanUp = 0;
	m_bannedList.InitHashTable(331);
	m_trackedClientsList.InitHashTable(2011);
}

CClientList::~CClientList(){
	POSITION pos = m_trackedClientsList.GetStartPosition();
	uint32 nKey;
	CDeletedClient* pResult;
	while (pos != NULL){
		m_trackedClientsList.GetNextAssoc( pos, nKey, pResult );
		m_trackedClientsList.RemoveKey(nKey);
		delete pResult;
	}
}

// xrmb : statsclientstatus
// -khaos--+++> Rewritten to accomodate some new statistics, and just for cleanup's sake.
//				I've added three new stats: Number of cDonkey clients, # errored clients, # banned clients.
//				We also now support LMule
void CClientList::GetStatistics(uint32 &totalclient, int stats[], CMap<uint16, uint16, uint32, uint32> *clientVersionEDonkey, CMap<uint16, uint16, uint32, uint32> *clientVersionEDonkeyHybrid, CMap<uint16, uint16, uint32, uint32> *clientVersionEMule, CMap<uint16, uint16, uint32, uint32> *clientVersionLMule){
	totalclient = list.GetCount();
	if(clientVersionEDonkeyHybrid)	clientVersionEDonkeyHybrid->RemoveAll();
	if(clientVersionEDonkey)		clientVersionEDonkey->RemoveAll();
	if(clientVersionEMule)			clientVersionEMule->RemoveAll();
	if(clientVersionLMule)			clientVersionLMule->RemoveAll();
	POSITION pos1, pos2;

	for (int i=0;i<15;i++) stats[i]=0;

	stats[7]=m_bannedList.GetCount();

	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		if (cur_client->HasLowID()) ++stats[14];
		
		switch (cur_client->GetClientSoft()) {

			case SO_EDONKEY :				
				if(clientVersionEDonkey)
				{
					++stats[1];
					(*clientVersionEDonkey)[cur_client->GetVersion()]++;
				}
				break;

			case SO_EDONKEYHYBRID : 
				if(clientVersionEDonkeyHybrid)
				{
					++stats[4];
					(*clientVersionEDonkeyHybrid)[cur_client->GetVersion()]++;
				}
				break;
			case SO_EMULE   :
			case SO_OLDEMULE:
				//MORPH START - Moved by SiRoB, Due to Maella -Support for tag ET_MOD_VERSION 0x55 II- in client software tree
				++stats[2];
				//MORPH END   - Moved by SiRoB, Due to Maella -Support for tag ET_MOD_VERSION 0x55 II- in client software tree
				if(clientVersionEMule)
				{
					//++stats[2];
					uint8 version = cur_client->GetMuleVersion();
					if (version == 0xFF || version == 0x66 || version==0x69 || version==0x90 || version==0x33 || version==0x60 ) continue;
					uint16 versions = (cur_client->GetUpVersion()*100) + (cur_client->GetMinVersion()*100*10) + (cur_client->GetMajVersion()*100*10*100);
					if ((*clientVersionEMule)[versions] < 1) (*clientVersionEMule)[versions] = 0;
					(*clientVersionEMule)[versions]++;
				}
				break;

			case SO_XMULE:
				if(clientVersionLMule)
				{
					++stats[10];
					uint8 version = cur_client->GetMuleVersion();
					if (version == 0x66 || version==0x69 || version==0x90 || version==0x33) continue;
					uint16 versions = (cur_client->GetUpVersion()*100) + (cur_client->GetMinVersion()*100*10) + (cur_client->GetMajVersion()*100*10*100);
					if ((*clientVersionLMule)[versions] < 1) (*clientVersionLMule)[versions] = 0;
					(*clientVersionLMule)[versions]++;
				}

				break;

			case SO_UNKNOWN :	++stats[0];		break;
			case SO_CDONKEY :	++stats[5];		break;
			case SO_MLDONKEY:	++stats[3];		break;
			case SO_SHAREAZA:   ++stats[11];    break;
		}

		if (cur_client->Credits() != NULL){
			switch(cur_client->Credits()->GetCurrentIdentState(cur_client->GetIP())){
				case IS_IDENTIFIED:
					stats[12]++;
					break;
				case IS_IDFAILED:
				case IS_IDNEEDED:
				case IS_IDBADGUY:
					stats[13]++;
			}
		}

		if (cur_client->GetDownloadState()==DS_ERROR || cur_client->GetUploadState()==US_ERROR )
				++stats[6]; // Error

		switch (cur_client->GetUserPort()) {
			case 4662:
				++stats[8]; // Default Port
				break;
			default:
				++stats[9]; // Other Port
		}
	}
}
// <-----khaos-


void CClientList::AddClient(CUpDownClient* toadd,bool bSkipDupTest){
	if ( !bSkipDupTest){
		if(list.Find(toadd))
			return;
	}
	theApp.emuledlg->transferwnd.clientlistctrl.AddClient(toadd);
	list.AddTail(toadd);
}
//MORPH START - Added by SiRoB, ZZ Upload system (USS)
bool CClientList::GiveClientsForTraceRoute() {
    // this is a host that lastCommonRouteFinder can use to traceroute
    return theApp.lastCommonRouteFinder->AddHostsToCheck(list);
}
//MORPH END   - Added by SiRoB, ZZ Upload system (USS)

//MORPH START - Changed by SiRoB, ZZ Upload system
void CClientList::RemoveClient(CUpDownClient* toremove, CString reason){
	POSITION pos = list.Find(toremove);
	if (pos){
		if(!reason || reason.Compare("") == 0) {
			reason = "No reason given.";
		}
		//just to be sure...
		theApp.uploadqueue->RemoveFromUploadQueue(toremove, "Client removed from CClientList::RemoveClient(). Reason: " + reason);
		theApp.uploadqueue->RemoveFromWaitingQueue(toremove);
		theApp.downloadqueue->RemoveSource(toremove);
		theApp.emuledlg->transferwnd.clientlistctrl.RemoveClient(toremove);
		list.RemoveAt(pos);
	}
	RemoveTCP(toremove);
}
//MORPH END   - Changed by SiRoB, ZZ Upload system

void CClientList::DeleteAll(){
	theApp.uploadqueue->DeleteAll();
	theApp.downloadqueue->DeleteAll();
	POSITION pos1, pos2;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		list.RemoveAt(pos2);
		delete cur_client; // recursiv: this will call RemoveClient
	}
	for (pos1 = RequestTCPList.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		RequestTCPList.GetNext(pos1);
		CUpDownClient* cur_client =	RequestTCPList.GetAt(pos2);
		RequestTCPList.RemoveAt(pos2);
		delete cur_client; // recursiv: this will call RemoveClient
	}
}

bool CClientList::AttachToAlreadyKnown(CUpDownClient** client, CClientReqSocket* sender){
	POSITION pos1, pos2;
	CUpDownClient* tocheck = (*client);
	CUpDownClient* found_client = NULL;
	CUpDownClient* found_client2 = NULL;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){	//
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		if (tocheck->Compare(cur_client,false)){ //matching userhash
			found_client2 = cur_client;
		}
		if (tocheck->Compare(cur_client,true)){	 //matching IP
			found_client = cur_client;
			break;
		}
	}
	if (found_client == NULL)
		found_client = found_client2;

	if (found_client != NULL){
		if (tocheck == found_client){
			//we found the same client instance (client may have sent more than one OP_HELLO). do not delete that client!
			return true;
		}
			if (sender){
			if (found_client->socket){
				if (found_client->socket->IsConnected() 
					&& (found_client->GetIP() != tocheck->GetIP() || found_client->GetUserPort() != tocheck->GetUserPort() ) )
					{
					// if found_client is connected and has the IS_IDENTIFIED, it's safe to say that the other one is a bad guy
					if (found_client->Credits() && found_client->Credits()->GetCurrentIdentState(found_client->GetIP()) == IS_IDENTIFIED){
						AddDebugLogLine(false, GetResString(IDS_BANHASHINVALID), tocheck->GetUserName(),tocheck->GetFullIP()); 
						tocheck->Ban();
						return false;
					}
	
					//IDS_CLIENTCOL Warning: Found matching client, to a currently connected client: %s (%s) and %s (%s)
					AddDebugLogLine(true,GetResString(IDS_CLIENTCOL),tocheck->GetUserName(),tocheck->GetFullIP(),found_client->GetUserName(),found_client->GetFullIP());
					return false;
				}
				found_client->socket->client = 0;
				found_client->socket->Safe_Delete();
				}
			found_client->socket = sender;
				tocheck->socket = 0;
			}
			*client = 0;
			delete tocheck;
			// TODO: I think we should reset some client properties here (like m_byEmuleVersion, 
			// m_byDataCompVer, m_bySourceExchangeVer,...). If we attach to a client instance 
			// which has already set some eMule specific properties but is no longer responding
			// to an OP_EMULEINFO message we may deal with that client in a wrong way.
		*client = found_client;
			return true;
		}
	return false;
}

CUpDownClient* CClientList::FindClientByIP(uint32 clientip,uint16 port){
	POSITION pos1, pos2;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		if (cur_client->GetIP() == clientip && cur_client->GetUserPort() == port)
			return cur_client;
	}
	return 0;
}

CUpDownClient* CClientList::FindClientByUserHash(uchar* clienthash){
	POSITION pos1, pos2;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		if (!md4cmp(cur_client->GetUserHash() ,clienthash))
			return cur_client;
	}
	return 0;
}

void CClientList::AddBannedClient(uint32 dwIP){
	m_bannedList.SetAt(dwIP, ::GetTickCount());
}

bool CClientList::IsBannedClient(uint32 dwIP){
	uint32 dwBantime = 0;
	if (m_bannedList.Lookup(dwIP, dwBantime)){
		if (dwBantime + CLIENTBANTIME > ::GetTickCount() )
			return true;
		else
			RemoveBannedClient(dwIP);
		}
	return false; 
		}

void CClientList::RemoveBannedClient(uint32 dwIP){
	m_bannedList.RemoveKey(dwIP);
	}

void CClientList::AddTrackClient(CUpDownClient* toadd){
	CDeletedClient* pResult = 0;
	if (m_trackedClientsList.Lookup(toadd->GetIP(), pResult)){
		pResult->m_dwInserted = ::GetTickCount();
		for (int i = 0; i != pResult->m_ItemsList.GetCount(); i++){
			if (pResult->m_ItemsList[i].nPort == toadd->GetUserPort()){
				// already tracked, update
				pResult->m_ItemsList[i].pHash = toadd->Credits();
				return;
			}
		}
		PORTANDHASH porthash = { toadd->GetUserPort(), toadd->Credits()};
		pResult->m_ItemsList.Add(porthash);
	}
	else{
		m_trackedClientsList.SetAt(toadd->GetIP(), new CDeletedClient(toadd));
	}
}

// true = everything ok, hash didn't changed
// false = hash changed
bool CClientList::ComparePriorUserhash(uint32 dwIP, uint16 nPort, void* pNewHash){
	CDeletedClient* pResult = 0;
	if (m_trackedClientsList.Lookup(dwIP, pResult)){
		for (int i = 0; i != pResult->m_ItemsList.GetCount(); i++){
			if (pResult->m_ItemsList[i].nPort == nPort){
				if (pResult->m_ItemsList[i].pHash != pNewHash)
					return false;
				else
					break;
			}
		}
	}
	return true;
}

uint16 CClientList::GetClientsFromIP(uint32 dwIP){
	CDeletedClient* pResult = 0;
	if (m_trackedClientsList.Lookup(dwIP, pResult)){
		return pResult->m_ItemsList.GetCount();
	}
	return 0;
}

void CClientList::Process(){
	const uint32 cur_tick = ::GetTickCount();
	if (m_dwLastBannCleanUp + BAN_CLEANUP_TIME < cur_tick){
		m_dwLastBannCleanUp = cur_tick;
		
		POSITION pos = m_bannedList.GetStartPosition();
		uint32 nKey;
		uint32 dwBantime;
		while (pos != NULL){
			m_bannedList.GetNextAssoc( pos, nKey, dwBantime );
			if (dwBantime + CLIENTBANTIME < cur_tick )
				RemoveBannedClient(nKey);
		}
	}

	
	if (m_dwLastTrackedCleanUp + TRACKED_CLEANUP_TIME < cur_tick ){
		m_dwLastTrackedCleanUp = cur_tick;
		AddDebugLogLine(false, "Cleaning up TrackedClientList, %i clients on List...", m_trackedClientsList.GetCount());
		POSITION pos = m_trackedClientsList.GetStartPosition();
		uint32 nKey;
		CDeletedClient* pResult;
		while (pos != NULL){
			m_trackedClientsList.GetNextAssoc( pos, nKey, pResult );
			if (pResult->m_dwInserted + KEEPTRACK_TIME < cur_tick ){
				m_trackedClientsList.RemoveKey(nKey);
				delete pResult;
			}
		}
		AddDebugLogLine(false, "...done, %i clients left on list", m_trackedClientsList.GetCount());
	}

	//We need to try to connect to the clients in RequestTCPList
	//If connected, remove them from the list and send a message back to ON so we can send a ACK.
	//If we don't connect, we need to remove the client..
	//The sockets timeout should delete this object.
	POSITION pos1, pos2;
	for (pos1 = RequestTCPList.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		RequestTCPList.GetNext(pos1);
		CUpDownClient* cur_client =	RequestTCPList.GetAt(pos2);
		switch(cur_client->GetKadIPCheckState()){
			case KS_QUEUED:
				cur_client->SetKadIPCHeckState(KS_CONNECTING);
				if(!cur_client->TryToConnect()){
					return;
				}
				break;
			case KS_CONNECTING:
				break;
			case KS_CONNECTED:
				//Set the Kademlia client a TCP connection ack! This most likely needs to be done in the Kademlia thread using a message to avoid issues.
				Kademlia::CKademlia::getUDPListener()->sendNullPacket(KADEMLIA_FIREWALLED_ACK, ntohl(cur_client->GetIP()), cur_client->GetKadPort());
				if(cur_client->Disconnected()){
					delete cur_client;
					return;
				}
				else{
					ASSERT(0);
				}
			default:
				ASSERT(0);
		}
	}
}


void CClientList::Debug_SocketDeleted(CClientReqSocket* deleted){
	POSITION pos1, pos2;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		if (!AfxIsValidAddress(cur_client, sizeof(CUpDownClient))) {
			AfxDebugBreak();
		}
		if (cur_client->socket == deleted){
			AfxDebugBreak();
		}
	}
}

bool CClientList::IsValidClient(CUpDownClient* tocheck){
	return list.Find(tocheck);
}

// #zegzav:updcliuplst
void CClientList::GetClientListByFileID(CTypedPtrList<CPtrList, CUpDownClient*> *clientlist, const uchar *fileid)
{
	clientlist->RemoveAll();
	for (POSITION pos = list.GetHeadPosition(); pos != 0; ) 
	{
		CUpDownClient *cur_src= list.GetNext(pos);
		if (md4cmp(cur_src->GetUploadFileID(), fileid) == 0)
			clientlist->AddTail(cur_src);
	}
}

void CClientList::RequestTCP(Kademlia::CContact* contact){
	//If eMule already knows this client, abort this.. It could cause conflicts since you could connect twice to the client..
	//Although the odds of this happening is very small, it could still happen. This will make this a very minimal occurence.
	//TODO: Maybe integrate these clients into the main clientlist to try to avoid this issue more.
	if(this->FindClientByIP(ntohl(contact->getIPAddress()), contact->getTCPPort())){
		delete contact;
		return;
	}
	//Add these to the RequestTCP list then process them.
	CUpDownClient* test = new CUpDownClient(0, contact->getTCPPort(), contact->getIPAddress(), 0, 0 );
	test->SetKadPort(contact->getUDPPort());
	test->SetKadIPCHeckState(KS_QUEUED);
	RequestTCPList.AddTail(test);
	delete contact;
}

void CClientList::RemoveTCP(CUpDownClient* torem){
	POSITION pos = RequestTCPList.Find(torem);
	if(pos){
		RequestTCPList.RemoveAt(pos);
	}
}

CUpDownClient* CClientList::FindClientByID_KadPort(uint32 clientID,uint16 kadPort){
	POSITION pos1, pos2;
	for (pos1 = list.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		list.GetNext(pos1);
		CUpDownClient* cur_client =	list.GetAt(pos2);
		if (cur_client->GetUserIDHybrid() == clientID && cur_client->GetKadPort() == kadPort)
			return cur_client;
	}
	return 0;
}

//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
void CClientList::AddClientType(EClientSoftware clientSoft, const CString& description){
	// Update in real time the maps
	switch(clientSoft){
		case SO_EDONKEY:{
				ClientMap::iterator it = m_eDonkeyMap.find(description);
				if(it == m_eDonkeyMap.end())
					m_eDonkeyMap[description] = 1; // First inscription
				else
					it->second++;
			}
			break;
		case SO_EDONKEYHYBRID:{
				ClientMap::iterator it = m_eDonkeyHybridMap.find(description);
				if(it == m_eDonkeyHybridMap.end())
					m_eDonkeyHybridMap[description] = 1; // First inscription
				else
					it->second++;
			}
			break;
		case SO_EMULE:{
				ClientMap::iterator it = m_eMuleMap.find(description);
				if(it == m_eMuleMap.end())
					m_eMuleMap[description] = 1; // First inscription
				else
					it->second++;
			}
			break;
		case SO_CDONKEY:{
				ClientMap::iterator it = m_cDonkeyMap.find(description);
				if(it == m_cDonkeyMap.end())
					m_cDonkeyMap[description] = 1; // First inscription
				else
					it->second++;
			}
			break;
		case SO_XMULE:{
				ClientMap::iterator it = m_lMuleMap.find(description);
				if(it == m_lMuleMap.end())
					m_lMuleMap[description] = 1; // First inscription
				else
					it->second++;
			}
			break;
		case SO_SHAREAZA:{
				ClientMap::iterator it = m_shareazaMap.find(description);
				if(it == m_shareazaMap.end())
					m_shareazaMap[description] = 1; // First inscription
				else
					it->second++;
			}
			break;
		case SO_MLDONKEY:{
				ClientMap::iterator it = m_oldMlDonkeyMap.find(description);
				if(it == m_oldMlDonkeyMap.end())
					m_oldMlDonkeyMap[description] = 1; // First inscription
				else
					it->second++;
			}
			break;
		default:{ // Don't forget to add here new type of clients
				ClientMap::iterator it = m_unknownMap.find(description);
				if(it == m_unknownMap.end())
					m_unknownMap[description] = 1; // First inscription
				else
					it->second++;
			}
			break; 
	}
}

void CClientList::RemoveClientType(EClientSoftware clientSoft, const CString& description){
	// Update in real time the maps
	switch(clientSoft){
		case SO_EDONKEY:{
				ClientMap::iterator it = m_eDonkeyMap.find(description);
				if(it != m_eDonkeyMap.end())
					if(it->second > 1)
						it->second--;
					else
						m_eDonkeyMap.erase(it);
			}
			break;
		case SO_EDONKEYHYBRID:{
				ClientMap::iterator it = m_eDonkeyHybridMap.find(description);
				if(it != m_eDonkeyHybridMap.end())
					if(it->second > 1)
						it->second--;
					else
						m_eDonkeyHybridMap.erase(it);
			}
			break;
		case SO_EMULE:{
				ClientMap::iterator it = m_eMuleMap.find(description);
				if(it != m_eMuleMap.end())
					if(it->second > 1)
						it->second--;
					else
						m_eMuleMap.erase(it);
			}
			break;
		case SO_CDONKEY:{
				ClientMap::iterator it = m_cDonkeyMap.find(description);
				if(it != m_cDonkeyMap.end())
					if(it->second > 1)
						it->second--;
					else
						m_cDonkeyMap.erase(it);
			}
			break;
		case SO_XMULE:{
				ClientMap::iterator it = m_lMuleMap.find(description);
				if(it != m_lMuleMap.end())
					if(it->second > 1)
						it->second--;
					else
						m_lMuleMap.erase(it);
			}
			break;
		case SO_SHAREAZA:{
				ClientMap::iterator it = m_shareazaMap.find(description);
				if(it != m_shareazaMap.end())
					if(it->second > 1)
						it->second--;
					else
						m_shareazaMap.erase(it);
			}
			break;
		case SO_MLDONKEY:{
				ClientMap::iterator it = m_oldMlDonkeyMap.find(description);
				if(it != m_oldMlDonkeyMap.end())
					if(it->second > 1)
						it->second--;
					else
						m_oldMlDonkeyMap.erase(it);
			}
			break;
		default:{  // Don't forget to add here new type of clients
				ClientMap::iterator it = m_unknownMap.find(description);
				if(it != m_unknownMap.end())
					if(it->second > 1)
						it->second--;
					else
						m_unknownMap.erase(it);
			}
			break;
	}
}
//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-

