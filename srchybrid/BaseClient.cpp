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
#include "updownclient.h"
#include "emule.h"
#include "uploadqueue.h"
#include "Clientlist.h"
#include "SearchList.h"
#include "PreviewDlg.h"
#include "CxImage/xImage.h"
#include "version.h"
#include "FunnyNick.h" //MORPH - Added by IceCream, xrmb FunnyNick
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//	members of CUpDownClient
//	which are used by down and uploading functions 

CUpDownClient::CUpDownClient(CClientReqSocket* sender){
	socket = sender;
	reqfile = 0;
	Init();

	//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
	theApp.clientlist->AddClientType(GetClientSoft(), GetClientVerString());
	//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
}

CUpDownClient::CUpDownClient(CPartFile* in_reqfile, uint16 in_port, uint32 in_userid,uint32 in_serverip, uint16 in_serverport, bool ed2kID){
	//Converting to the HybridID system.. The ED2K system didn't take into account of IP address ending in 0..
	//All IP addresses ending in 0 were assumed to be a lowID because of the calculations.
	socket = 0;
	Init();
	m_nUserPort = in_port;
	//If this is a ED2K source, check if it's a lowID.. If not, convert it to a HyrbidID.
	//TODO: Find out how Servers deal with these IP addresses ending with .0 and see what is the lowest we can put here to catch some of them.
	if(ed2kID && !IsLowIDED2K(in_userid))
		m_nUserIDHybrid = ntohl(in_userid);
	else
		m_nUserIDHybrid = in_userid;
	sourcesslot=m_nUserIDHybrid%SOURCESSLOTS;
	//If create the FullIP address depending on source type.
	if (!HasLowID() && ed2kID){
		sprintf(m_szFullUserIP,"%i.%i.%i.%i",(uint8)in_userid,(uint8)(in_userid>>8),(uint8)(in_userid>>16),(uint8)(in_userid>>24));
	}
	else if(!HasLowID()){
		in_userid = ntohl(in_userid);
		sprintf(m_szFullUserIP,"%i.%i.%i.%i",(uint8)in_userid,(uint8)(in_userid>>8),(uint8)(in_userid>>16),(uint8)(in_userid>>24));
	}
	m_dwServerIP = in_serverip;
	m_nServerPort = in_serverport;
	reqfile = in_reqfile;
	ReGetClientSoft();
}

void CUpDownClient::Init(){
	MEMSET(m_szFullUserIP,0,21);
	credits = 0;
	sumavgDDR = 0; // By BadWolf - Accurate Speed Measurement
	sumavgUDR = 0; // by BadWolf - Accurate Speed Measurement
	m_bAddNextConnect = false;  // VQB Fix for LowID slots only on connection
	m_nAvDownDatarate = 0;
	m_nAvUpDatarate = 0;
	m_nChatstate = MS_NONE;
	m_nKadIPCheckState = KS_NONE;
	m_cShowDR = 0;
	m_nUDPPort = 0;
	m_nKadPort = 0;
	m_nMaxSendAllowed = 0;
	m_nTransferedUp = 0;
	m_cSendblock = 0;
	m_cAsked = 0;
	m_cDownAsked = 0;
	dataratems = 0;
	m_nUpDatarate = 0;
	m_pszUsername = 0;
	m_dwUserIP = 0;
	m_nUserIDHybrid = 0;
	m_nServerPort = 0;
	m_bLeecher = false; //MORPH - Added by IceCream, Antileecher feature
	old_m_pszUsername = 0; //MORPH - Added by IceCream, Antileecher feature
    m_iFileListRequested = 0;
	m_dwLastUpRequest = 0;
	m_bEmuleProtocol = false;
	usedcompressiondown = false;
	m_bUsedComprUp = false;
	m_bCompleteSource = false;
	m_bFriendSlot = false;
	m_bCommentDirty = false;
	m_bReaskPending = false;
	m_bUDPPending = false;
	m_byEmuleVersion = 0;
	m_nUserPort = 0;
	m_nPartCount = 0;
	m_nUpPartCount = 0;
	m_abyPartStatus = 0;
	//MORPH START - Added by SiRoB, HotFix related to khaos::kmod+ 
	m_PartStatus_list.RemoveAll();
	//MORPH END   - Added by SiRoB, HotFix related to khaos::kmod+
	m_abyUpPartStatus = 0;
	m_dwLastAskedTime = 0;
	m_nDownloadState = DS_NONE;
	m_dwUploadTime = 0;
	m_nTransferedDown = 0;
	m_nDownDatarate = 0;
	m_nDownDataRateMS = 0;
	m_nUploadState = US_NONE;
	m_dwLastBlockReceived = 0;
	m_byEmuleVersion = 0;
	m_byDataCompVer = 0;
	m_byUDPVer = 0;
	m_bySourceExchangeVer = 0;
	m_byAcceptCommentVer = 0;
	m_byExtendedRequestsVer = 0;
	m_nRemoteQueueRank = 0;
	m_dwLastSourceRequest = 0;
	m_dwLastSourceAnswer = 0;
	m_dwLastAskedForSources = 0;
	m_byCompatibleClient = 0;
	m_bySourceFrom = 0;
	m_bIsHybrid = false;
	m_bIsML=false;
	//MOPRH START - Added by SiRoB, Is Morph Client?
	m_bIsMorph = false;
	//MOPRH END   - Added by SiRoB, Is Morph Client?
	m_Friend = NULL;
	m_iRate=0;
	m_bMsgFiltered=false;
	m_strComment="";
	m_nCurSessionUp = 0;
	m_nCurQueueSessionUp = 0; //MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	m_nSumForAvgDownDataRate = 0;
	m_nSumForAvgUpDataRate = 0;
	m_clientSoft=SO_UNKNOWN;
	m_bRemoteQueueFull = false;
	md4clr(m_achUserHash);
	if (socket){
		SOCKADDR_IN sockAddr;
		MEMSET(&sockAddr, 0, sizeof(sockAddr));
		uint32 nSockAddrLen = sizeof(sockAddr);
		socket->GetPeerName((SOCKADDR*)&sockAddr,(int*)&nSockAddrLen);
		m_dwUserIP = sockAddr.sin_addr.S_un.S_addr;
		strcpy(m_szFullUserIP,inet_ntoa(sockAddr.sin_addr));
	}
	sourcesslot=0;
	m_fHashsetRequesting = 0;
	m_fSharedDirectories = 0;
	md4clr(requpfileid);
	m_nClientVersion = 0;
	m_nClientMajVersion = 0;
	m_nClientMinVersion = 0;
	m_nClientUpVersion = 0;
	m_lastRefreshedDLDisplay = 0;
	ResetCompressionGain();
	m_dwDownStartTime = 0;
	m_nLastBlockOffset = 0;

	m_SecureIdentState = IS_UNAVAILABLE;
	m_dwLastSignatureIP = 0;
	m_bySupportSecIdent = 0;
	m_byInfopacketsReceived = IP_NONE;
	m_bIsSpammer = false;
	m_cMessagesReceived = 0;
	m_cMessagesSend = 0;
	m_lastPartAsked = 0xffff;
	m_nUpCompleteSourcesCount= 0;
	m_bSupportsPreview = false;
	m_bPreviewReqPending = false;
	m_bPreviewAnsPending = false;

	m_last_l2hac_exec = 0;				//<<--enkeyDEV(th1) -L2HAC-
	m_L2HAC_time = 0;					//<<--enkeyDEV(th1) -L2HAC-
	m_l2hac_enabled = false;			//<<--enkeyDEV(th1) -L2HAC- lowid side

	// khaos::kmod+
	m_iLastSwapAttempt = 0;
	m_iLastActualSwap = 0;
	m_iLastForceA4AFAttempt = 0;
	// khaos::kmod-
	//MORPH START - Added by SiRoB, ZZ Upload System
	m_random_update_wait = (uint32)(rand()/(RAND_MAX/1000));
	//m_bHasPriorPart = false;
	//m_iPriorPartNumber = 0;
	m_currentPartNumberIsKnown = false;
	m_dwLastCheckedForEvictTick = 0;
    	m_addedPayloadQueueSession = 0;
	//MORPH END   - Added by SiRoB, ZZ Upload System
	//MORPH STRAT - Added by SiRoB, Better Download rate calcul
	m_AvarageDDRlastRemovedHeadTimestamp = 0;
	//MORPH END   - Added by SiRoB, Better Download rate calcul

	m_nDownTotalTime = 0;//wistily Total download time for this client for this emule session
	m_nUpTotalTime = 0;//wistily Total upload time for this client for this emule session
}

CUpDownClient::~CUpDownClient(){
	//MORPH START - Added by IceCream, Maella -Support for tag ET_MOD_VERSION 
	theApp.clientlist->RemoveClientType(GetClientSoft(), GetClientVerString());
	//MORPH END  - Added by IceCream, Maella -Support for tag ET_MOD_VERSION 
	theApp.clientlist->RemoveClient(this, "Destructing client object");
	if (m_Friend){
		//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System
		m_Friend->SetLinkedClient(NULL);
		//theApp.friendlist->RefreshFriend(m_Friend);
		//m_Friend = NULL;
		//MORPH END - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System
	}
	if (socket){
		socket->client = 0;
		socket->Safe_Delete();
	}
	if (m_pszUsername)
		delete[] m_pszUsername;
	// khaos::kmod+ Free the memory from any stored statuses.
	POSITION			pos = m_PartStatus_list.GetStartPosition();
	CPartFile*			curFile;
	uint8*				curPS;
	while (pos)
	{
		m_PartStatus_list.GetNextAssoc(pos, curFile, curPS);
		delete[] curPS;
		if (curPS == m_abyPartStatus)
			m_abyPartStatus = NULL;
	}
	m_PartStatus_list.RemoveAll();
	// khaos::kmod-
	if (m_abyPartStatus)
		delete[] m_abyPartStatus;
	
	if (m_abyUpPartStatus)
		delete[] m_abyUpPartStatus;
	ClearUploadBlockRequests();

	for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition();pos != 0;m_DownloadBlocks_list.GetNext(pos))
		delete m_DownloadBlocks_list.GetAt(pos);
	m_DownloadBlocks_list.RemoveAll();
	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition();pos != 0;m_RequestedFiles_list.GetNext(pos))
		delete m_RequestedFiles_list.GetAt(pos);
	m_RequestedFiles_list.RemoveAll();
	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0;m_PendingBlocks_list.GetNext(pos)){
		Pending_Block_Struct *pending = m_PendingBlocks_list.GetAt(pos);
		delete pending->block;
		// Not always allocated
		if (pending->zStream){
			inflateEnd(pending->zStream);
			delete pending->zStream;
		}
		delete pending;
	}
	for (POSITION pos =m_WaitingPackets_list.GetHeadPosition();pos != 0;m_WaitingPackets_list.GetNext(pos))
		delete m_WaitingPackets_list.GetAt(pos);

	if (m_iRate>0 || m_strComment.GetLength()>0) {
		m_iRate=0; m_strComment="";
		reqfile->UpdateFileRatingCommentAvail();
	}

	m_PendingBlocks_list.RemoveAll();
	m_AvarageUDR_list.RemoveAll();
	m_AvarageDDR_list.RemoveAll();
	DEBUG_ONLY (theApp.listensocket->Debug_ClientDeleted(this));
	this->SetUploadFileID(NULL);
}

//MORPH START - Added by IceCream, Anti-leecher feature
bool CUpDownClient::TestLeecher(){
	if ((old_m_pszUsername == m_pszUsername) && (old_m_clientVerString == m_clientVerString))
		return IsLeecher();

	old_m_pszUsername = m_pszUsername;
	old_m_clientVerString = m_clientVerString;

	if (StrStrI(m_pszUsername,"$GAM3R$")||
	StrStrI(m_pszUsername,"G@m3r")||
	StrStrI(m_pszUsername,"$WAREZ$")||
	StrStrI(m_clientModString,"Freeza")||
	StrStrI(m_clientModString,"d-unit")||
	//StrStrI(m_clientModString,"NOS")|| //removed for the moment
	StrStrI(m_clientModString,"imperator")||
	StrStrI(m_pszUsername,"RAMMSTEIN")||//
	StrStrI(m_clientModString,"SpeedLoad")||
	//StrStrI(m_pszUsername,"toXic")|| //removed for the moment
	StrStrI(m_clientModString,"gt mod")||
	StrStrI(m_clientModString,"egomule")||
	StrStrI(m_pszUsername,"Leecha")||
	//StrStrI(m_clientModString,"aldo")|| //removed for the moment
	StrStrI(m_pszUsername,"energyfaker")||
	//StrStrI(m_pszUsername,"eDevil")|| //removed for the moment
	StrStrI(m_pszUsername,"darkmule")||
	StrStrI(m_clientModString,"darkmule")||
	StrStrI(m_clientModString,"LegoLas")||
	StrStrI(m_pszUsername,"phArAo")||
	StrStrI(m_pszUsername,"dodgethis")||
	StrStrI(m_clientModString,"dodgethis")|| //Updated
	StrStrI(m_pszUsername,"Reverse")||
	StrStrI(m_clientModString,"DM-")|| //hotfix
	StrStrI(m_pszUsername,"eVortex")||
	StrStrI(m_pszUsername,"|eVorte|X|")||
	StrStrI(m_clientModString,"|X|")||
	StrStrI(m_clientModString,"eVortex")||
	StrStrI(m_pszUsername,"Chief")||
	//StrStrI(m_pszUsername,"Mison")|| //Temporaly desactivated, ban only on mod tag
	StrStrI(m_clientModString,"Mison")||
	StrStrI(m_clientModString,"father")||
	StrStrI(m_clientModString,"Dragon")||
	StrStrI(m_clientModString,"booster")|| //Temporaly added, must check the tag
	StrStrI(m_clientModString,"$motty")||
	StrStrI(m_pszUsername,"$motty")||
	StrStrI(m_pszUsername,"emule-speed")||
	StrStrI(m_pszUsername,"celinesexy")||
	StrStrI(m_pszUsername,"Gate-eMule")||
	StrStrI(m_clientModString,"Thunder")||
	StrStrI(m_clientModString,"BuzzFuzz")||
	StrStrI(m_pszUsername,"BuzzFuzz")||
	StrStrI(m_clientModString,"Speed-Unit")|| 
	StrStrI(m_pszUsername,"Speed-Unit")|| 
	StrStrI(m_clientModString,"Killians")||
	StrStrI(m_pszUsername,"Killians")||
	StrStrI(m_pszUsername,"pubsman")||
	// EastShare START - Added by TAHO, Pretender
	StrStrI(m_pszUsername,"emule-element")||
	StrStrI(m_clientModString,"Element")|| 
	StrStrI(m_clientModString,"§¯Å]")|| 
	(StrStrI(m_clientModString,"EastShare") && StrStrI(m_clientVerString,"0.29"))||
	// EastShare END - Added by TAHO, Pretender
	(StrStrI(m_pszUsername,"emule") && StrStrI(m_pszUsername,"booster"))||
	(StrStrI(m_clientModString,"LSD.7c") && !StrStrI(m_clientVerString,"27"))||
	(StrStrI(m_clientModString,"Morph") && StrStrI(m_clientModString,"Max"))||
	((m_clientModString.IsEmpty() == false) && (StrStrI(m_clientVerString,"edonkey")))||
	((GetVersion()>589) && (GetSourceExchangeVersion()>0) && (GetClientSoft()==51))) //LSD, edonkey user with eMule property
	{
		return true;
	}
	return false;
}
//MORPH END   - Added by IceCream, Anti-leecher feature

void CUpDownClient::ClearHelloProperties()
{
	m_nUDPPort = 0;
	m_byUDPVer = 0;
	m_byDataCompVer = 0;
	m_byEmuleVersion = 0;
	m_bySourceExchangeVer = 0;
	m_byAcceptCommentVer = 0;
	m_byExtendedRequestsVer = 0;
	m_byCompatibleClient = 0;
	m_nKadPort = 0;
	m_bySupportSecIdent = 0;
	m_bSupportsPreview = false;
	m_nClientVersion = 0;
	m_nClientMajVersion = 0;
	m_nClientMinVersion = 0;
	m_nClientUpVersion = 0;
	m_fSharedDirectories = 0;

	//MORPH START - Added by SiRoB, ET_MOD_VERSION 0x55
	m_clientModString = "";
	//MORPH END   - Added by SiRoB, ET_MOD_VERSION 0x55
	//MOPRH START - Added by SiRoB, Is Morph Client?
	m_bIsMorph = false;
	//MOPRH END   - Added by SiRoB, Is Morph Client?
}

bool CUpDownClient::ProcessHelloPacket(char* pachPacket, uint32 nSize){
	CSafeMemFile data((BYTE*)pachPacket,nSize);
	uint8 hashsize;
	data.Read(&hashsize,1);
	// reset all client properties; a client may not send a particular emule tag any longer
	ClearHelloProperties();
	return ProcessHelloTypePacket(&data);
}

bool CUpDownClient::ProcessHelloAnswer(char* pachPacket, uint32 nSize){
	CSafeMemFile data((BYTE*)pachPacket,nSize);
	return ProcessHelloTypePacket(&data);
}

bool CUpDownClient::ProcessHelloTypePacket(CSafeMemFile* data){
	DWORD dwEmuleTags = 0;
	m_bIsHybrid = false;
	m_bIsML = false;
	data->Read(&m_achUserHash,16);
	data->Read(&m_nUserIDHybrid,4);
	uint16 nUserPort = 0;
	data->Read(&nUserPort,2); // hmm clientport is sent twice - why?

	uint32	tagcount;
	data->Read(&tagcount,4);
	for (uint32 i = 0;i < tagcount; i++){
		CTag temptag(data);
		switch(temptag.tag.specialtag){
			case CT_NAME:
				if (m_pszUsername){
					delete[] m_pszUsername;
					m_pszUsername = NULL; // needed, in case 'nstrdup' fires an exception!!
				}
				if( temptag.tag.stringvalue )
					m_pszUsername = nstrdup(temptag.tag.stringvalue);
				//MORPH START - Added by IceCream, xrmb Funnynick START
				if (!m_pszUsername)
					m_pszUsername=funnyNick.gimmeFunnyNick(m_achUserHash);
				else if((strncmp(m_pszUsername, "http://emule",12)==0)
					||(strncmp(m_pszUsername, "http://www.emule",16)==0)
					||(strncmp(m_pszUsername, "eMule v",7)==0)
					||(strncmp(m_pszUsername, "eMule Plus",10)==0)
					||(strncmp(m_pszUsername, "eMule OX",8)==0)
					||(strncmp(m_pszUsername, "eMule Plus",10)==0)
					||(strncmp(m_pszUsername, "eMule0",6)==0)
					||(strcmp(m_pszUsername, "")==0)) {
						delete m_pszUsername;
						m_pszUsername=funnyNick.gimmeFunnyNick(m_achUserHash);
				}
				//MORPH END   - Added by IceCream, xrmb Funnynick END
				break;
			case CT_VERSION:
				m_nClientVersion = temptag.tag.intvalue;
				break;
			case CT_PORT:
				nUserPort = temptag.tag.intvalue;
				break;
			case CT_EMULE_UDPPORTS:
				// 16 KAD Port
				// 16 UDP Port
				m_nKadPort = (uint16)(temptag.tag.intvalue >> 16);
				m_nUDPPort = (uint16)temptag.tag.intvalue;
				dwEmuleTags |= 1;
				break;
			case CT_EMULE_MISCOPTIONS1:
				//  4 Reserved for future use
				//  4 UDP version
				//  4 Data compression version
				//  4 Secure Ident
				//  4 Source Exchange
				//  4 Ext. Requests
				//  4 Comments
				//  4 Preview
				m_byUDPVer				= (temptag.tag.intvalue >> 4*6) & 0x0f;
				m_byDataCompVer			= (temptag.tag.intvalue >> 4*5) & 0x0f;
				m_bySupportSecIdent		= (temptag.tag.intvalue >> 4*4) & 0x0f;
				m_bySourceExchangeVer	= (temptag.tag.intvalue >> 4*3) & 0x0f;
				m_byExtendedRequestsVer	= (temptag.tag.intvalue >> 4*2) & 0x0f;
				m_byAcceptCommentVer	= (temptag.tag.intvalue >> 4*1) & 0x0f;
				m_bSupportsPreview		= (temptag.tag.intvalue >> 4*0) & 0x0f;
				dwEmuleTags |= 2;
				break;
			case CT_EMULE_VERSION:
				//  8 Compatible Client ID
				//  7 Mjr Version (Doesn't really matter..)
				//  7 Min Version (Only need 0-99)
				//  3 Upd Version (Only need 0-5)
				//  7 Bld Version (Only need 0-99)
				m_byCompatibleClient = (temptag.tag.intvalue >> 24);
				m_nClientVersion = temptag.tag.intvalue & 0x00ffffff;
				m_byEmuleVersion = 0x99;
				m_fSharedDirectories = 1;
				dwEmuleTags |= 4;
				break;
			//MORPH START - Added by SiRoB, ET_MOD_VERSION 0x55
			case ET_MOD_VERSION: 
				if( temptag.tag.stringvalue ){
					m_clientModString = temptag.tag.stringvalue;
					//MOPRH START - Added by SiRoB, Is Morph Client?
					m_bIsMorph = StrStrI(m_clientModString,"Morph");
					//MOPRH END   - Added by SiRoB, Is Morph Client?
				}
				break;
			//MORPH END   - Added by SiRoB, ET_MOD_VERSION 0x55
		}
	}
	m_nUserPort = nUserPort;
	data->Read(&m_dwServerIP,4);
	data->Read(&m_nServerPort,2);
	// Hybrid now has an extra uint32.. What is it for?
	// Also, many clients seem to send an extra 6? These are not eDonkeys or Hybrids..
	if ( data->GetLength() - data->GetPosition() == 4 ){
		uint32 test;
		data->Read(&test,4);
		if (test=='KDLM') 
			m_bIsML=true;
		else{
			m_bIsHybrid = true;
			m_fSharedDirectories = 1;
		}
	}

	// tecxx 1609 2002 - add client's servet to serverlist (Moved to uploadqueue.cpp)

	SOCKADDR_IN sockAddr;
	MEMSET(&sockAddr, 0, sizeof(sockAddr));
	uint32 nSockAddrLen = sizeof(sockAddr);
	socket->GetPeerName((SOCKADDR*)&sockAddr,(int*)&nSockAddrLen);
	

	m_dwUserIP = sockAddr.sin_addr.S_un.S_addr;
	strcpy(m_szFullUserIP,inet_ntoa(sockAddr.sin_addr));

	if (theApp.glob_prefs->AddServersFromClient() && m_dwServerIP && m_nServerPort){
		in_addr addhost;
		addhost.S_un.S_addr = m_dwServerIP;
		CServer* addsrv = new CServer(m_nServerPort, inet_ntoa(addhost));
		addsrv->SetListName(addsrv->GetAddress());

		if (!theApp.emuledlg->serverwnd.serverlistctrl.AddServer(addsrv, true))
			delete addsrv;
	}
	//Because most sources are from ED2K, I removed the m_nUserIDHybrid != m_dwUserIP check since this will trigger 90% of the time anyway.
	if(!HasLowID() || m_nUserIDHybrid == 0) 
		m_nUserIDHybrid = ntohl(m_dwUserIP);
	uchar key[16];
	md4cpy(key,m_achUserHash);
	CClientCredits* pFoundCredits = theApp.clientcredits->GetCredit(key);
	if (credits == NULL){
		credits = pFoundCredits;
		if (!theApp.clientlist->ComparePriorUserhash(m_dwUserIP, m_nUserPort, pFoundCredits)){
			AddDebugLogLine(false, GetResString(IDS_BANHASHCHANGEDT), GetUserName(), GetFullIP()); 
			Ban();
		}	
	}
	else if (credits != pFoundCredits){
		// userhash change ok, however two hours "waittime" before it can be used
		credits = pFoundCredits;
		AddDebugLogLine(false, GetResString(IDS_BANHASHCHANGED), GetUserName(),GetFullIP()); 
		Ban();
	}

	if ((m_Friend = theApp.friendlist->SearchFriend(key, m_dwUserIP, m_nUserPort)) != NULL){
		// Link the friend to that client
		//MORPH START - Added by Yun.SF3, ZZ Upload System
		m_Friend->SetLinkedClient(this);
		//MORPH END - Added by Yun.SF3, ZZ Upload System
	}
	else{
		// avoid that an unwanted client instance keeps a friend slot
		SetFriendSlot(false);
	}
	ReGetClientSoft();
	m_byInfopacketsReceived |= IP_EDONKEYPROTPACK;
	// check if at least CT_EMULEVERSION was received, all other tags are optional
	bool bIsMule = (dwEmuleTags & 0x04) == 0x04;
	if (bIsMule){
		m_bEmuleProtocol = true;
		m_byInfopacketsReceived |= IP_EMULEPROTPACK;
		//MORPH START - Added by SiRoB, Anti-leecher feature
		bool bLeecher = false;
		if(theApp.glob_prefs->GetEnableAntiCreditHack())
			if (theApp.GetID()!=m_nUserIDHybrid && memcmp(m_achUserHash, theApp.glob_prefs->GetUserHash(), 16)==0)
				bLeecher = true;
		if(theApp.glob_prefs->GetEnableAntiLeecher())
			if(TestLeecher())
				bLeecher = true;
		if(bLeecher)
			BanLeecher(!IsBanned());
		else
			m_bLeecher = false;
		//MORPH END   - Added by SiRoB, Anti-leecher feature
	}
	return bIsMule;
}

void CUpDownClient::SendHelloPacket(){
	if (socket == NULL){
		ASSERT(0);
		return;
	}

	// if IP is filtered, dont greet him but disconnect...
	SOCKADDR_IN sockAddr;
	MEMSET(&sockAddr, 0, sizeof(sockAddr));
	uint32 nSockAddrLen = sizeof(sockAddr);
	socket->GetPeerName((SOCKADDR*)&sockAddr,(int*)&nSockAddrLen);
	if ( theApp.ipfilter->IsFiltered(sockAddr.sin_addr.S_un.S_addr)) {
		AddDebugLogLine(true,GetResString(IDS_IPFILTERED),GetFullIP(),theApp.ipfilter->GetLastHit());
		if(Disconnected(GetResString(IDS_IPISFILTERED)+ " 1")){
			delete this;
		}
		theApp.stat_filteredclients++;
		return;
	}
	
	CSafeMemFile data(128);
	uint8 hashsize = 16;
	data.Write(&hashsize,1);
	SendHelloTypePacket(&data);
	Packet* packet = new Packet(&data);
	packet->opcode = OP_HELLO;
	theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true);
	AskTime=::GetTickCount(); //MORPH - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
}

void CUpDownClient::SendMuleInfoPacket(bool bAnswer){
	if (socket == NULL){
		ASSERT(0);
		return;
	}

	CSafeMemFile data(128);
	uint8 version = theApp.m_uCurVersionShort;
	data.Write(&version,1);
	uint8 protversion = EMULE_PROTOCOL;
	data.Write(&protversion,1);
	//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
	//uint32 tagcount = 8; //+1;
	//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
	uint32 tagcount = 9; //+1;//<<--enkeyDEV(th1) -L2HAC- tag
	data.Write(&tagcount,4);
	CTag tag(ET_COMPRESSION,1);
	tag.WriteTagToFile(&data);
	CTag tag2(ET_UDPVER,3);
	tag2.WriteTagToFile(&data);
	CTag tag3(ET_UDPPORT,theApp.glob_prefs->GetUDPPort());
	tag3.WriteTagToFile(&data);
	CTag tag4(ET_SOURCEEXCHANGE,3);
	tag4.WriteTagToFile(&data);
	CTag tag5(ET_COMMENTS,1);
	tag5.WriteTagToFile(&data);
	CTag tag6(ET_EXTENDEDREQUEST,2);
	tag6.WriteTagToFile(&data);

	uint32 dwTagValue = (theApp.clientcredits->CryptoAvailable() ? 3 : 0);
	if (theApp.glob_prefs->IsPreviewEnabled())
		dwTagValue |= 128;
	CTag tag7(ET_FEATURES, dwTagValue);
	tag7.WriteTagToFile(&data);
	//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
	// Maella -Support for tag ET_MOD_VERSION 0x55-
	//MORPH START - Added by IceCream, Anti-leecher feature
	if (StrStrI(m_clientModString,"Mison")||StrStrI(m_clientModString,"eVort")||StrStrI(m_clientModString,"booster")||IsLeecher())
	{
		CTag tag8(ET_MOD_VERSION, m_clientModString);
		tag8.WriteTagToFile(&data);
	}
	else
	{
		CTag tag8(ET_MOD_VERSION, MOD_VERSION);
		tag8.WriteTagToFile(&data);
	}
	//MORPH END   - Added by IceCream, Anti-leecher feature
	// Maella end
	//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-

	CTag tag9(ET_L2HAC,FILEREASKTIME);	//<<--enkeyDEV(th1) -L2HAC-
	tag9.WriteTagToFile(&data);			//<<--enkeyDEV(th1) -L2HAC-
	Packet* packet = new Packet(&data,OP_EMULEPROT);
	if (!bAnswer)
		packet->opcode = OP_EMULEINFO;
	else
		packet->opcode = OP_EMULEINFOANSWER;
	theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true,true);
}

void CUpDownClient::ProcessMuleInfoPacket(char* pachPacket, uint32 nSize){
	CSafeMemFile data((BYTE*)pachPacket,nSize);
	m_byCompatibleClient = 0;
	//The version number part of this packet will soon be useless since it is only able to go to v.99.
	//Why the version is a uint8 and why it was not done as a tag like the eDonkey hello packet is not known..
	//Therefore, sooner or later, we are going to have to switch over to using the eDonkey hello packet to set the version.
	//No sense making a third value sent for versions..
	data.Read(&m_byEmuleVersion,1);
	if( m_byEmuleVersion == 0x2B )
		m_byEmuleVersion = 0x22;
	uint8 protversion;
	data.Read(&protversion,1);

	//implicitly supported options by older clients
	if (protversion == EMULE_PROTOCOL) {
		//in the future do not use version to guess about new features

		if(m_byEmuleVersion < 0x25 && m_byEmuleVersion > 0x22)
			m_byUDPVer = 1;

		if(m_byEmuleVersion < 0x25 && m_byEmuleVersion > 0x21)
			m_bySourceExchangeVer = 1;

		if(m_byEmuleVersion == 0x24)
			m_byAcceptCommentVer = 1;

		// Shared directories are requested from eMule 0.28+ because eMule 0.27 has a bug in 
		// the OP_ASKSHAREDFILESDIR handler, which does not return the shared files for a 
		// directory which has a trailing backslash.
		if(m_byEmuleVersion >= 0x28 && !m_bIsML) // MLdonkey currently does not support shared directories
			m_fSharedDirectories = 1;

	} else {
		return;
	}
	m_bEmuleProtocol = true;
	m_L2HAC_time = 0;			//<<--enkeyDEV(th1) -L2HAC-

	uint32 tagcount;
	data.Read(&tagcount,4);
	for (uint32 i = 0;i < tagcount; i++){
		CTag temptag(&data);
		switch(temptag.tag.specialtag){
			case ET_COMPRESSION:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: data compression version
				m_byDataCompVer = temptag.tag.intvalue;
				break;
			case ET_UDPPORT:
				// Bits 31-16: 0 - reserved
				// Bits 15- 0: UDP port
				m_nUDPPort = temptag.tag.intvalue;
				break;
			case ET_UDPVER:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: UDP protocol version
				m_byUDPVer = temptag.tag.intvalue;
				break;
			case ET_SOURCEEXCHANGE:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: source exchange protocol version
				m_bySourceExchangeVer = temptag.tag.intvalue;
				break;
			case ET_COMMENTS:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: comments version
				m_byAcceptCommentVer = temptag.tag.intvalue;
				break;
			case ET_EXTENDEDREQUEST:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: extended requests version
				m_byExtendedRequestsVer = temptag.tag.intvalue;
				break;
			case ET_COMPATIBLECLIENT:
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: compatible client ID
				m_byCompatibleClient = temptag.tag.intvalue;
				break;
			case ET_FEATURES:
				// Bits 31- 1: 0 - reserved
				// Bit      0: secure identification
				m_bySupportSecIdent = temptag.tag.intvalue & 3;
				m_bSupportsPreview = (temptag.tag.intvalue & 128) > 0;
				break;
			// START enkeyDEV(th1) -L2HAC-
			case ET_L2HAC:
				m_L2HAC_time = temptag.tag.intvalue;
				break;
			// END enkeyDEV(th1) -L2HAC-
 			//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
			case ET_MOD_VERSION: 
				m_clientModString = temptag.tag.stringvalue;
				//MOPRH START - Added by SiRoB, Is Morph Client?
				m_bIsMorph = StrStrI(m_clientModString,"Morph");
				//MOPRH END   - Added by SiRoB, Is Morph Client?
				break;
			//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
		}
	}

	// START enkeyDEV(th1) -L2HAC- Enable every emule anyway and disable bad clients
	if (!m_L2HAC_time) m_L2HAC_time = L2HAC_DEFAULT_EMULE;
	if (m_L2HAC_time < L2HAC_MIN_TIME || m_L2HAC_time > L2HAC_MAX_TIME) m_L2HAC_time = 0;
	// END enkeyDEV(th1) -L2HAC-

	if( m_byDataCompVer == 0 ){
		m_bySourceExchangeVer = 0;
		m_byExtendedRequestsVer = 0;
		m_byAcceptCommentVer = 0;
		m_nUDPPort = 0;
		m_L2HAC_time = 0;			//<<-- enkeyDEV(th1) -L2HAC-
	}
	ReGetClientSoft();
	m_byInfopacketsReceived |= IP_EMULEPROTPACK;

	//MORPH START - Added by SiRoB, Anti-leecher feature
	if(theApp.glob_prefs->GetEnableAntiLeecher())
		if(TestLeecher())
			BanLeecher(!IsBanned());
	//MORPH END   - Added by SiRoB, Anti-leecher feature
}

void CUpDownClient::SendHelloAnswer(){
	if (socket == NULL){
		ASSERT(0);
		return;
	}

	CSafeMemFile data(128);
	SendHelloTypePacket(&data);
	Packet* packet = new Packet(&data);
	packet->opcode = OP_HELLOANSWER;
	theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true);
}

void CUpDownClient::SendHelloTypePacket(CMemFile* data)
{
	data->Write(theApp.glob_prefs->GetUserHash(),16);

	uint32 clientid = theApp.GetID();
	data->Write(&clientid,4);
	uint16 nPort = theApp.glob_prefs->GetPort();
	data->Write(&nPort,2);

	uint32 tagcount = 6/*5*/;//MORPH - Changed by SiRoB, MOD_VERSION tag
	data->Write(&tagcount,4);
	// eD2K Name
	//MORPH START - Added by IceCream, Anti-leecher feature
	if (StrStrI(m_pszUsername,"G@m3r")||StrStrI(m_pszUsername,"$WAREZ$")||StrStrI(m_pszUsername,"chief"))
	{
		CTag tagName(CT_NAME, m_pszUsername);
		tagName.WriteTagToFile(data);
	}
	else
	{
		CTag tagName(CT_NAME,theApp.glob_prefs->GetUserNick());
		tagName.WriteTagToFile(data);
	}
	//MORPH END   - Added by IceCream, Anti-leecher feature

	// eD2K Version
	CTag tagVersion(CT_VERSION,EDONKEYVERSION);
	tagVersion.WriteTagToFile(data);

	// eMule UDP Ports
	uint32 kadUDPPort = 0;
	if(theApp.kademlia->isConnected())
	{
		kadUDPPort = theApp.kademlia->getUdpPort();
	}
	CTag tagUdpPorts(CT_EMULE_UDPPORTS, 
				(kadUDPPort									<< 16) |
				((uint32)theApp.glob_prefs->GetUDPPort()         ) ); 
	tagUdpPorts.WriteTagToFile(data);

	// eMule Misc. Options #1
	const UINT uUdpVer				= 3;
	const UINT uDataCompVer			= 1;
	const UINT uSupportSecIdent		= 3;
	const UINT uSourceExchangeVer	= 3;
	const UINT uExtendedRequestsVer	= 2;
	const UINT uAcceptCommentVer	= 1;
	const UINT uSupportsPreview		= 1;
	CTag tagMisOptions(CT_EMULE_MISCOPTIONS1, 
				(uUdpVer				<< 4*6) |
				(uDataCompVer			<< 4*5) |
				(uSupportSecIdent		<< 4*4) |
				(uSourceExchangeVer		<< 4*3) |
				(uExtendedRequestsVer	<< 4*2) |
				(uAcceptCommentVer		<< 4*1) |
				(uSupportsPreview		<< 4*0) );
	tagMisOptions.WriteTagToFile(data);

	// eMule Version
	CTag tagMuleVersion(CT_EMULE_VERSION, 
				//(uCompatibleClientID	<< 24) |
				(VERSION_MJR			<< 17) |
				(VERSION_MIN			<< 10) |
				(VERSION_UPDATE			<<  7) //|
//				(RESERVED			     ) 
				);
	tagMuleVersion.WriteTagToFile(data);

	//MORPH - Added by SiRoB, ET_MOD_VERSION 0x55
	//MORPH START - Added by SiRoB, Anti-leecher feature
	if (StrStrI(m_clientModString,"Mison")||StrStrI(m_clientModString,"eVort")||StrStrI(m_clientModString,"booster")||IsLeecher())
	{
		CTag tagMODVersion(ET_MOD_VERSION, m_clientModString);
		tagMODVersion.WriteTagToFile(data);
	}
	else
	{
		CTag tagMODVersion(ET_MOD_VERSION, MOD_VERSION);
		tagMODVersion.WriteTagToFile(data);
	}
	//MORPH END   - Added by SiRoB, Anti-leecher feature
	//MORPH - Added by SiRoB, ET_MOD_VERSION 0x55

	uint32 dwIP;
	if (theApp.serverconnect->IsConnected()){
		dwIP = theApp.serverconnect->GetCurrentServer()->GetIP();
		nPort = theApp.serverconnect->GetCurrentServer()->GetPort();
	}
	else{
		nPort = 0;
		dwIP = 0;
	}
	data->Write(&dwIP,4);
	data->Write(&nPort,2);
}

void CUpDownClient::ProcessMuleCommentPacket(char* pachPacket, uint32 nSize){
	if( reqfile ){
		if( reqfile->IsPartFile()){
			int length;
			if (nSize>(sizeof(m_iRate)+sizeof(length)-1)){
				CSafeMemFile data((BYTE*)pachPacket,nSize);
				data.Read(&m_iRate,sizeof(m_iRate));
				data.Read(&length,sizeof(length));
				reqfile->SetHasRating(true);
				AddDebugLogLine(false,GetResString(IDS_RATINGRECV),m_strClientFilename,m_iRate);
				if ( length > data.GetLength() - data.GetPosition() ){
					length = data.GetLength() - data.GetPosition();
				}
				if (length>50) length=50;
				if (length>0){
					data.Read(m_strComment.GetBuffer(length),length);
					m_strComment.ReleaseBuffer(length);
					AddDebugLogLine(false,GetResString(IDS_DESCRIPTIONRECV), m_strClientFilename, m_strComment);
					reqfile->SetHasComment(true);
					
					// test if comment is filtered
					if (theApp.glob_prefs->GetCommentFilter().GetLength()>0) {
						CString resToken;
						CString strlink=theApp.glob_prefs->GetCommentFilter();
						strlink.MakeLower();
						int curPos=0;
						resToken= strlink.Tokenize("|",curPos);
						while (resToken != "") {
							if (m_strComment.MakeLower().Find(resToken)>-1) {
								m_strComment="";
								m_iRate=0;
								reqfile->SetHasRating(false);
								reqfile->SetHasComment(false);
								break;
							}
							resToken= strlink.Tokenize("|",curPos);
						}
		
					}
				}
			}
			if (reqfile->HasRating() || reqfile->HasComment()) theApp.emuledlg->transferwnd.downloadlistctrl.UpdateItem(reqfile);
		}
	}
}

//MORPH START - Changed by SiRoB, ZZ UPload system
bool CUpDownClient::Disconnected(CString reason, bool m_FromSocket){
//MORPH END   - Changed by SiRoB, ZZ UPload system
	//If this is a KAD client object, just delete it!
	if(this->GetKadIPCheckState() != KS_NONE){
		return true;
	}	
	// There is at least one case where this ASSERT will give a false warning. When a new client instance is created
	// on receiving OP_HELLO and the packet is parsed, it may throw an exception which let us delete/Disconnect that
	// client which is not yet in the client-list.
	ASSERT(theApp.clientlist->IsValidClient(this));

	if (GetUploadState() == US_UPLOADING)
		//MORPH START - Changed by SiRoB, ZZ UPload system 20030818-1923
		theApp.uploadqueue->RemoveFromUploadQueue(this, reason);
		//MORPH END   - Changed by SiRoB, ZZ UPload system 20030818-1923
	if (m_BlockSend_queue.GetCount() > 0) {
		// Although this should not happen, it happens sometimes. The problem we may run into here is as follows:
		//
		// 1.) If we do not clear the block send requests for that client, we will send those blocks next time the client
		// gets an upload slot. But because we are starting to send any available block send requests right _before_ the
		// remote client had a chance to prepare to deal with them, the first sent blocks will get dropped by the client.
		// Worst thing here is, because the blocks are zipped and can therefore only be uncompressed when the first block
		// was received, all of those sent blocks will create a lot of uncompress errors at the remote client.
		//
		// 2.) The remote client may have already received those blocks from some other client when it gets the next
		// upload slot.
		AddDebugLogLine(false, "Disconnected client %u. Block send queue=%u.", GetUserIDHybrid(), m_BlockSend_queue.GetCount());
		ClearUploadBlockRequests();
	}
	if (GetDownloadState() == DS_DOWNLOADING){
		//MORPH START - Added by SiRoB, ZZ UPload system 20030818-1923
        	if(!reason || reason.Compare("") == 0) {
            		CString temp = GetResString(IDS_REMULNOREASON);
            		reason = temp;
        	}
        	AddDebugLogLine(false,GetResString(IDS_DWLSESSIONENDED), GetUserName(), reason);
		//MORPH END - Added by SiRoB, ZZ UPload system 20030818-1923
		
		SetDownloadState(DS_ONQUEUE);
	}
	else if(GetDownloadState() == DS_CONNECTED){
		// client didn't responsed to our request for some reasons (remotely banned?)
		// or it just doesn't has this file, so try to swap first
		if (!SwapToAnotherFile(true, true, true, NULL)){
			theApp.downloadqueue->RemoveSource(this);
			//DEBUG_ONLY(theApp.emuledlg->AddDebugLogLine(false, "Removed %s from downloadqueue - didn't responsed to filerequests",GetUserName()));
		}
	}
		

	// The remote client does not have to answer with OP_HASHSETANSWER *immediatly* 
	// after we've sent OP_HASHSETREQUEST. It may occure that a (buggy) remote client 
	// is sending use another OP_FILESTATUS which would let us change to DL-state to DS_ONQUEUE.
	if (((GetDownloadState() == DS_REQHASHSET) || m_fHashsetRequesting) && (reqfile))
        reqfile->hashsetneeded = true;

	ASSERT(theApp.clientlist->IsValidClient(this));

	//check if this client is needed in any way, if not delete it
	bool bDelete = true;
	switch(m_nUploadState){
		case US_ONUPLOADQUEUE:
			bDelete = false;
//		default:
//			this->SetUploadFileID(NULL);
	};
	switch(m_nDownloadState){
		case DS_ONQUEUE:
		case DS_TOOMANYCONNS:
		case DS_NONEEDEDPARTS:
		case DS_LOWTOLOWIP:
			bDelete = false;
	};

	switch(m_nUploadState){
		case US_CONNECTING:
		case US_WAITCALLBACK:
		case US_ERROR:
			bDelete = true;
	};
	switch(m_nDownloadState){
		case DS_CONNECTING:
		case DS_WAITCALLBACK:
		case DS_ERROR:
			bDelete = true;
	};
	

	if (GetChatState() != MS_NONE){
		bDelete = false;
		theApp.emuledlg->chatwnd.chatselector.ConnectingResult(this,false);
	}
	if (!m_FromSocket && socket){
		ASSERT (theApp.listensocket->IsValidSocket(socket));
		socket->Safe_Delete();
	}
	socket = 0;
	if (m_iFileListRequested){
		AddDebugLogLine(false,GetResString(IDS_SHAREDFILES_FAILED),GetUserName());
		m_iFileListRequested = 0;
	}
 	if (m_Friend)
		theApp.friendlist->RefreshFriend(m_Friend);
	theApp.emuledlg->transferwnd.clientlistctrl.RefreshClient(this);
	if (bDelete){
		return true;
	}
	else{
		m_fHashsetRequesting = 0;
		return false;
	}
}

bool CUpDownClient::TryToConnect(bool bIgnoreMaxCon){
	if (theApp.listensocket->TooManySockets() && !bIgnoreMaxCon && !(socket && socket->IsConnected())){
		//MORPH START - Changed by SiRoB, ZZ UPload system
		if(Disconnected("Failed to connect. Too many sockets.")){
		//MORPH END   - Changed by SiRoB, ZZ UPload system
			delete this;
			return false;
		}
		return true;
	}

	if (theApp.IsFirewalled() && HasLowID()){
		if (GetDownloadState() == DS_CONNECTING)
			SetDownloadState(DS_LOWTOLOWIP);
		else if (GetDownloadState() == DS_REQHASHSET){
			SetDownloadState(DS_ONQUEUE);
			reqfile->hashsetneeded = true;
		}
		if (GetUploadState() == US_CONNECTING){
			//MORPH START - Changed by SiRoB, ZZ UPload system
			if(Disconnected("Failed to connect. Can not connect from low ID to another client with low ID.")){
			//MORPH END   - Changed by SiRoB, ZZ UPload system
				delete this;
			}
		}
		//Never connect lowID to lowID
		return false;
	}

	if (!socket){
		socket = new CClientReqSocket(theApp.glob_prefs,this);
		if (!socket->Create()){
			socket->Safe_Delete();
			return false; //Fix
		}
	}
	else if (!socket->IsConnected()){
		socket->Safe_Delete();
		socket = new CClientReqSocket(theApp.glob_prefs,this);
		if (!socket->Create()){
			socket->Safe_Delete();
			return false; //Fix
		}
	}
	else{
		ConnectionEstablished();
		return true;
	}
	// MOD Note: Do not change this part - Merkur
	if (HasLowID()){
		if (GetDownloadState() == DS_CONNECTING)
			SetDownloadState(DS_WAITCALLBACK);
		if (GetUploadState() == US_CONNECTING){
			//MORPH START - Changed by SiRoB, ZZ UPload system
			if(Disconnected("Failed to connect. 1")){
			//MORPH END   - Changed by SiRoB, ZZ UPload system
				delete this;
				return false;
			}
			return true;
		}

		if (theApp.serverconnect->IsLocalServer(m_dwServerIP,m_nServerPort)){
			Packet* packet = new Packet(OP_CALLBACKREQUEST,4);
			MEMCOPY(packet->pBuffer,&m_nUserIDHybrid,4);
			if (theApp.glob_prefs->GetDebugServerTCP())
				Debug(">>> Sending OP__CallbackRequest\n");
			theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
			theApp.serverconnect->SendPacket(packet);
		}
		else{
			if (GetUploadState() == US_NONE && (!GetRemoteQueueRank() || m_bReaskPending) ){
				theApp.downloadqueue->RemoveSource(this);
				//MORPH START - Changed by SiRoB, ZZ UPload system 20030818-1923
				if(Disconnected("Failed to connect. 2")){
				//MORPH END   - Changed by SiRoB, ZZ UPload system 20030818-1923
					delete this;
					return false;
				}
				return true;
			}
			else{
				if (GetDownloadState() == DS_WAITCALLBACK){
					m_bReaskPending = true;
					SetDownloadState(DS_ONQUEUE);
				}
			}
		}
	}
	// MOD Note - end
	else{
		socket->Connect(GetFullIP(),GetUserPort());
		SendHelloPacket();
	}

	return true;
}

void CUpDownClient::ConnectionEstablished(){
	m_cFailed = 0;
	// ok we have a connection, lets see if we want anything from this client
	if (GetKadIPCheckState() == KS_CONNECTING ){
		this->SetKadIPCHeckState(KS_CONNECTED);
		return; //Return because this object is not within the general population of client objects
	}
	if (GetChatState() == MS_CONNECTING)
		theApp.emuledlg->chatwnd.chatselector.ConnectingResult(this,true);
	switch(GetDownloadState()){
		case DS_CONNECTING:
		case DS_WAITCALLBACK:
			m_bReaskPending = false;
			SetDownloadState(DS_CONNECTED);
			SendFileRequest();
	}
	if (m_bReaskPending){
		m_bReaskPending = false;
		if (GetDownloadState() != DS_NONE && GetDownloadState() != DS_DOWNLOADING){
			SetDownloadState(DS_CONNECTED);
			SendFileRequest();
		}
	}
	switch(GetUploadState()){
		case US_CONNECTING:
		case US_WAITCALLBACK:
			if (theApp.uploadqueue->IsDownloading(this)){
				SetUploadState(US_UPLOADING);
				Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
				theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
				socket->SendPacket(packet,true);
			}
	}
    if (m_iFileListRequested == 1){
        Packet* packet = new Packet(m_fSharedDirectories ? OP_ASKSHAREDDIRS : OP_ASKSHAREDFILES,0);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		socket->SendPacket(packet,true,true);
	}
	while (!m_WaitingPackets_list.IsEmpty()){
		socket->SendPacket(m_WaitingPackets_list.RemoveHead());
	}
}


void CUpDownClient::ReGetClientSoft(){
	// Maella -Support for tag ET_MOD_VERSION 0x55 II-
	theApp.clientlist->RemoveClientType(GetClientSoft(), GetClientVerString());
	// Maella end

	int iHashType = GetHashType();
	if(m_pszUsername == NULL){
		m_clientSoft = SO_UNKNOWN;
	}
	else if(iHashType == SO_OLDEMULE){
		m_clientSoft = SO_OLDEMULE;
	}
	else if(iHashType == SO_EMULE){
		if( m_byEmuleVersion != 0x99 )
		{
			m_nClientMajVersion = 0;
			m_nClientMinVersion = (m_byEmuleVersion >> 4)*10 + (m_byEmuleVersion & 0x0f);
			m_nClientUpVersion = 0;
			m_nClientVersion = m_nClientMinVersion*10*100;
		}
		else
		{
			m_nClientMajVersion   = (m_nClientVersion >> 17) & 0x7f;
			m_nClientMinVersion   = (m_nClientVersion >> 10) & 0x7f;
			m_nClientUpVersion    = (m_nClientVersion >>  7) & 0x07;
		}
		switch(m_byCompatibleClient){
			case SO_CDONKEY:
				m_clientSoft = SO_CDONKEY;
				break;
			case SO_XMULE:
				m_clientSoft = SO_XMULE;
				break;
			case SO_SHAREAZA:
				m_clientSoft = SO_SHAREAZA;
				break;
			default:
				if (m_bIsML)
					m_clientSoft = SO_MLDONKEY;
				else if (m_bIsHybrid)
					m_clientSoft = SO_EDONKEYHYBRID;
				else
					m_clientSoft = SO_EMULE;
		}
	//	return;
	}
	else if (m_bIsML || iHashType == SO_MLDONKEY) {
		m_clientSoft= SO_MLDONKEY;
	}
	else if( m_bIsHybrid ) {
		m_clientSoft=SO_EDONKEYHYBRID;
	}
	else {
		m_clientSoft=SO_EDONKEY;
}

	// Format name of client
	switch(GetClientSoft()){
		case SO_EDONKEY:
			m_clientVerString.Format(_T("eDonkey v%u"),GetVersion());
			break;
		case SO_EDONKEYHYBRID:
			m_clientVerString.Format(_T("eDonkeyHybrid v%u"),GetVersion());
			break;
		case SO_EMULE:
			if(GetMuleVersion() == 0x99)
				m_clientVerString.Format(_T("eMule v%u.%u%c"), GetMajVersion(), GetMinVersion(), _T('a') + GetUpVersion());
			else
				m_clientVerString.Format(_T("eMule v%u.%u"), GetMajVersion(), GetMinVersion());
			break;
		case SO_OLDEMULE:
			if(GetMuleVersion() == 0x99)
				m_clientVerString.Format(_T("old eMule v%u.%u%c"), GetMajVersion(), GetMinVersion(), _T('a') + GetUpVersion());
			else
				m_clientVerString.Format(_T("old eMule v%u.%u"), GetMajVersion(), GetMinVersion());
			break;
		case SO_CDONKEY:
			if(GetMuleVersion() == 0x99)
				m_clientVerString.Format(_T("cDonkey v%u.%u%c"), GetMajVersion(), GetMinVersion(), _T('a') + GetUpVersion());
			else
				m_clientVerString.Format(_T("cDonkey v%u.%u"), GetMajVersion(), GetMinVersion());
			break;
		case SO_XMULE:
			if(GetMuleVersion() == 0x99)
				m_clientVerString.Format(_T("lMule v%u.%u%c"), GetMajVersion(), GetMinVersion(), _T('a') + GetUpVersion());
			else
				m_clientVerString.Format(_T("lMule v%u.%u"), GetMajVersion(), GetMinVersion());
			break;
		case SO_SHAREAZA:
			if(GetMuleVersion() == 0x99)
				m_clientVerString.Format(_T("Shareaza v%u.%u%c"), GetMajVersion(), GetMinVersion(), _T('a') + GetUpVersion());
			else
				m_clientVerString.Format(_T("Shareaza v%u.%u"), GetMajVersion(), GetMinVersion());
			break;
		case SO_MLDONKEY:
			if(GetMuleVersion() == 0x99)
				m_clientVerString.Format(_T("MlDonkey v%u.%u%c"), GetMajVersion(), GetMinVersion(), _T('a') + GetUpVersion());
			else
				m_clientVerString.Format(_T("MlDonkey v%u.%u"), GetMajVersion(), GetMinVersion());
			break;
		default:
			m_clientVerString = GetResString(IDS_UNKNOWN);
	}
	if(m_clientModString.IsEmpty() == false){
		m_clientVerString += _T(" [");
		m_clientVerString += m_clientModString;
		m_clientVerString += _T("]");
	}	

	// Maella -Support for tag ET_MOD_VERSION 0x55 II-
	theApp.clientlist->AddClientType(GetClientSoft(), GetClientVerString());
	// Maella end
}
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-

int CUpDownClient::GetHashType()
{
	if (m_achUserHash[5] == 13 && m_achUserHash[14] == 110)
		return SO_OLDEMULE;
	else if (m_achUserHash[5] == 14 && m_achUserHash[14] == 111)
		return SO_EMULE;
 	else if (m_achUserHash[5] == 'M' && m_achUserHash[14] == 'L')
		return SO_MLDONKEY;
	else
		return SO_UNKNOWN;
}

void CUpDownClient::SetUserName(char* pszNewName){
	if (m_pszUsername){
		delete[] m_pszUsername;
		m_pszUsername = NULL;// needed, in case 'nstrdup' fires an exception!!
	}
	if( pszNewName )
		m_pszUsername = nstrdup(pszNewName);
}

void CUpDownClient::RequestSharedFileList(){
	if (m_iFileListRequested == 0){
		AddDebugLogLine(true,GetResString(IDS_SHAREDFILES_REQUEST),GetUserName());
    	m_iFileListRequested = 1;
		TryToConnect(true);
	}
	else
		AddDebugLogLine(true,_T("Requesting shared files from user %s (%u) is already in progress"),GetUserName(),GetUserIDHybrid());
}

void CUpDownClient::ProcessSharedFileList(char* pachPacket, uint32 nSize, LPCTSTR pszDirectory){
    if (m_iFileListRequested > 0){
        m_iFileListRequested--;
		theApp.searchlist->ProcessSearchanswer(pachPacket,nSize,this,NULL,pszDirectory);
	}
}

void CUpDownClient::SetUserHash(uchar* m_achTempUserHash){
	if( m_achTempUserHash == NULL ){
		md4clr(m_achUserHash);
		return;
	}
	md4cpy(m_achUserHash,m_achTempUserHash);
}

void CUpDownClient::SendPublicKeyPacket(){
	///* delete this line later*/DEBUG_ONLY(theApp.emuledlg->AddDebugLogLine(false, "sending public key to '%s'", GetUserName()));
	// send our public key to the client who requested it
	if (socket == NULL || credits == NULL || m_SecureIdentState != IS_KEYANDSIGNEEDED){
		ASSERT ( false );
		return;
	}
	if (!theApp.clientcredits->CryptoAvailable())
		return;

    Packet* packet = new Packet(OP_PUBLICKEY,theApp.clientcredits->GetPubKeyLen() + 1,OP_EMULEPROT);
	theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
	MEMCOPY(packet->pBuffer+1,theApp.clientcredits->GetPublicKey(), theApp.clientcredits->GetPubKeyLen());
	packet->pBuffer[0] = theApp.clientcredits->GetPubKeyLen();
	socket->SendPacket(packet,true,true);
	m_SecureIdentState = IS_SIGNATURENEEDED;
}

void CUpDownClient::SendSignaturePacket(){
	// signate the public key of this client and send it
	if (socket == NULL || credits == NULL || m_SecureIdentState == 0){
		ASSERT ( false );
		return;
	}

	if (!theApp.clientcredits->CryptoAvailable())
		return;
	if (credits->GetSecIDKeyLen() == 0)
		return; // We don't have his public key yet, will be back here later
		///* delete this line later*/ DEBUG_ONLY(theApp.emuledlg->AddDebugLogLine(false, "sending signature key to '%s'", GetUserName()));
	// do we have a challenge value recieved (actually we should if we are in this function)
	if (credits->m_dwCryptRndChallengeFrom == 0){
		AddDebugLogLine(false, "Want to send signature but challenge value is invalid ('%s')", GetUserName());
		return;
	}
	// v2
	// we will use v1 as default, except if only v2 is supported
	bool bUseV2;
	if ( (m_bySupportSecIdent&1) == 1 )
		bUseV2 = false;
	else
		bUseV2 = true;

	uint8 byChaIPKind = 0;
	uint32 ChallengeIP = 0;
	if (bUseV2){
		if (theApp.serverconnect->GetClientID() == 0 || theApp.serverconnect->IsLowID()){
			// we cannot do not know for sure our public ip, so use the remote clients one
			ChallengeIP = GetIP();
			byChaIPKind = CRYPT_CIP_REMOTECLIENT;
		}
		else{
			ChallengeIP = theApp.serverconnect->GetClientID();
			byChaIPKind  = CRYPT_CIP_LOCALCLIENT;
		}
	}
	//end v2
	uchar achBuffer[250];
	uint8 siglen = theApp.clientcredits->CreateSignature(credits, achBuffer,  250, ChallengeIP, byChaIPKind );
	if (siglen == 0){
		ASSERT ( false );
		return;
	}
	Packet* packet = new Packet(OP_SIGNATURE,siglen + 1+ ( (bUseV2)? 1:0 ),OP_EMULEPROT);
	theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
	MEMCOPY(packet->pBuffer+1,achBuffer, siglen);
	packet->pBuffer[0] = siglen;
	if (bUseV2)
		packet->pBuffer[1+siglen] = byChaIPKind;
	socket->SendPacket(packet,true,true);
	m_SecureIdentState = IS_ALLREQUESTSSEND;
}

void CUpDownClient::ProcessPublicKeyPacket(uchar* pachPacket, uint32 nSize){
	theApp.clientlist->AddTrackClient(this);

	///* delete this line later*/ DEBUG_ONLY(theApp.emuledlg->AddDebugLogLine(false, "recieving public key from '%s'", GetUserName()));
	if (socket == NULL || credits == NULL || pachPacket[0] != nSize-1
		|| nSize == 0 || nSize > 250){
		ASSERT ( false );
		return;
	}
	if (!theApp.clientcredits->CryptoAvailable())
		return;
	// the function will handle everything (mulitple key etc)
	if (credits->SetSecureIdent(pachPacket+1, pachPacket[0])){
		// if this client wants a signature, now we can send him one
		if (m_SecureIdentState == IS_SIGNATURENEEDED){
			SendSignaturePacket();
		}
		else if(m_SecureIdentState == IS_KEYANDSIGNEEDED){
			// something is wrong
			if (theApp.glob_prefs->GetDebugSecuredConnection()) //MORPH - Added by SiRoB, Debug Log Option for Secured Connection
				AddDebugLogLine(false, "Invalid State error: IS_KEYANDSIGNEEDED in ProcessPublicKeyPacket");
		}
	}
	else{
		if (theApp.glob_prefs->GetDebugSecuredConnection()) //MORPH - Added by SiRoB, Debug Log Option for Secured Connection
			AddDebugLogLine(false, "Failed to use new received public key");
	}
}

void CUpDownClient::ProcessSignaturePacket(uchar* pachPacket, uint32 nSize){
	///* delete this line later*/ DEBUG_ONLY(theApp.emuledlg->AddDebugLogLine(false, "receiving signature from '%s'", GetUserName()));
	// here we spread the good guys from the bad ones ;)

	if (socket == NULL || credits == NULL || nSize == 0 || nSize > 250){
		ASSERT ( false );
		return;
	}

	uint8 byChaIPKind;
	if (pachPacket[0] == nSize-1)
		byChaIPKind = 0;
	else if (pachPacket[0] == nSize-2 && (m_bySupportSecIdent & 2) > 0) //v2
		byChaIPKind = pachPacket[nSize-1];
	else{
		ASSERT ( false );
		return;
	}
	if (!theApp.clientcredits->CryptoAvailable())
		return;
	
	// we accept only one signature per IP, to avoid floods which need a lot cpu time for cryptfunctions
	if (m_dwLastSignatureIP == GetIP()){
		if (theApp.glob_prefs->GetDebugSecuredConnection()) //MORPH - Added by SiRoB, Debug Log Option for Secured Connection
			AddDebugLogLine(false, "received multiple signatures from one client");
		return;
	}
	// also make sure this client has a public key
	if (credits->GetSecIDKeyLen() == 0){
		AddDebugLogLine(false, "received signature for client without public key");
		return;
	}
	// and one more check: did we ask for a signature and sent a challange packet?
	if (credits->m_dwCryptRndChallengeFor == 0){
		AddDebugLogLine(false, "received signature for client with invalid challenge value ('%s')", GetUserName());
		return;
	}

	if (theApp.clientcredits->VerifyIdent(credits, pachPacket+1, pachPacket[0], GetIP(), byChaIPKind ) ){
		// result is saved in function abouve
		if (theApp.glob_prefs->GetDebugSecuredConnection()) //MORPH - Added by SiRoB, Debug Log Option for Secured Connection
			AddDebugLogLine(false, "'%s' has passed the secure identification, V2 State: %i", GetUserName(), byChaIPKind);
	}
	else
		if (theApp.glob_prefs->GetDebugSecuredConnection()) //MORPH - Added by SiRoB, Debug Log Option for Secured Connection
			AddDebugLogLine(false, "'%s' has failed the secure identification, V2 State: %i", GetUserName(), byChaIPKind);
	m_dwLastSignatureIP = GetIP(); 
}

void CUpDownClient::SendSecIdentStatePacket(){
	// check if we need public key and signature
	uint8 nValue = 0;
	if (credits){
		if (theApp.clientcredits->CryptoAvailable()){
			if (credits->GetSecIDKeyLen() == 0)
				nValue = IS_KEYANDSIGNEEDED;
			else if (m_dwLastSignatureIP != GetIP())
				nValue = IS_SIGNATURENEEDED;
		}
		if (nValue == 0){
			//DEBUG_ONLY(theApp.emuledlg->AddDebugLogLine(false, "Not sending SecIdentState Packet, because State is Zero"));
			return;
		}
		// crypt: send random data to sign
		uint32 dwRandom = rand()+1;
		credits->m_dwCryptRndChallengeFor = dwRandom;
		//DEBUG_ONLY(theApp.emuledlg->AddDebugLogLine(false, "sending SecIdentState Packet, state: %i (to '%s')", nValue, GetUserName() ));
		Packet* packet = new Packet(OP_SECIDENTSTATE,5,OP_EMULEPROT);
		theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
		packet->pBuffer[0] = nValue;
		MEMCOPY(packet->pBuffer+1,&dwRandom, sizeof(dwRandom));
		socket->SendPacket(packet,true,true);
	}
	else
		ASSERT ( false );
}

void CUpDownClient::ProcessSecIdentStatePacket(uchar* pachPacket, uint32 nSize){
	if (nSize != 5)
		return;
	if (!credits){
		ASSERT ( false );
		return;
	}
	switch(pachPacket[0]){
			case 0:
				m_SecureIdentState = IS_UNAVAILABLE;
				break;
			case 1:
				m_SecureIdentState = IS_SIGNATURENEEDED;
				break;
			case 2:
				m_SecureIdentState = IS_KEYANDSIGNEEDED;
				break;
		}
	uint32 dwRandom;
	MEMCOPY(&dwRandom, pachPacket+1,4);
	credits->m_dwCryptRndChallengeFrom = dwRandom;
	//DEBUG_ONLY(theApp.emuledlg->AddDebugLogLine(false, "recieved SecIdentState Packet, state: %i", pachPacket[0]));
}

void CUpDownClient::InfoPacketsReceived(){
	// indicates that both Information Packets has been received
	// needed for actions, which process data from both packets
	ASSERT ( m_byInfopacketsReceived == IP_BOTH );
	m_byInfopacketsReceived = IP_NONE;
	
	if (m_bySupportSecIdent){
		SendSecIdentStatePacket();
	}
}
void CUpDownClient::ResetFileStatusInfo(){
	//MORPH START - Removed by SiRoB, khaos::+
	/*if (m_abyPartStatus){
		delete[] m_abyPartStatus;
		m_abyPartStatus = NULL;
	}
	m_nPartCount = 0;*/
	//MORPH END   - Removed by SiRoB, khaos::-
	m_strClientFilename = "";
	m_bCompleteSource = false;
	m_dwLastAskedTime = 0;
	m_iRate=0;
	m_strComment="";
	//MORPH START - Added by SiRoB, HotFix Due Complete Source Feature
	m_nUpCompleteSourcesCount = 0;
	//MORPH END   - Added by SiRoB, HotFix Due Complete Source Feature
}

bool CUpDownClient::IsBanned(){
	return ( (theApp.clientlist->IsBannedClient(GetIP()) ) && m_nDownloadState != DS_DOWNLOADING);
}

void CUpDownClient::SendPreviewRequest(CAbstractFile* pForFile){
	if (!m_bPreviewReqPending){
		m_bPreviewReqPending = true;
		Packet* packet = new Packet(OP_REQUESTPREVIEW,16,OP_EMULEPROT);
		MEMCOPY(packet->pBuffer,pForFile->GetFileHash(),16);
		SafeSendPacket(packet);
	}
	else{
		//to res table - later
		AddLogLine(true, GetResString(IDS_ERR_PREVIEWALREADY));
	}
}

void CUpDownClient::SendPreviewAnswer(CKnownFile* pForFile, CxImage** imgFrames, uint8 nCount){
	m_bPreviewAnsPending = false;	
	CSafeMemFile data(1024);
	if (pForFile){
		data.Write(pForFile->GetFileHash(),16);
	}
	else{
		char ZeroHash[16];
		md4clr(ZeroHash);
		data.Write(ZeroHash,16);
	}
	data.Write(&nCount,1);
	for (int i = 0; i != nCount; i++){
		if (imgFrames == NULL){
			ASSERT ( false );
			return;
		}
		CxImage* cur_frame = imgFrames[i];
		if (cur_frame == NULL){
			ASSERT ( false );
			return;
		}
		BYTE* abyResultBuffer = NULL;
		long nResultSize = 0;
		if (!cur_frame->Encode(abyResultBuffer, nResultSize, CXIMAGE_FORMAT_PNG)){
			ASSERT ( false );			
			return;
		}
		data.Write(&nResultSize,4);
		data.Write(abyResultBuffer, nResultSize);
		free(abyResultBuffer);
	}
	Packet* packet = new Packet(&data, OP_EMULEPROT);
	packet->opcode = OP_PREVIEWANSWER;
	SafeSendPacket(packet);
}

void CUpDownClient::ProcessPreviewReq(char* pachPacket, uint32 nSize){
	if (nSize < 16)
		throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
	
	if (m_bPreviewAnsPending || theApp.glob_prefs->ServePreview()==2 || (theApp.glob_prefs->ServePreview()==1 && !IsFriend() ) )
		return;
	
	m_bPreviewAnsPending = true;
	CKnownFile* previewFile = theApp.sharedfiles->GetFileByID((uchar*)pachPacket);
	if (previewFile == NULL){
		SendPreviewAnswer(NULL, NULL, 0);
	}
	else{
		previewFile->GrabImage(4,0,true,450,this);
	}
}

void CUpDownClient::ProcessPreviewAnswer(char* pachPacket, uint32 nSize){
	if (!m_bPreviewReqPending)
		return;
	m_bPreviewReqPending = false;
	CSafeMemFile data((BYTE*)pachPacket,nSize);
	uchar Hash[16];
	data.Read(Hash,16);
	uint8 nCount;
	data.Read(&nCount,1);
	if (nCount == 0){
		// to res table -later
		AddLogLine(true, GetResString(IDS_ERR_PREVIEWFAILED),GetUserName());
		return;
	}
	CSearchFile* sfile = theApp.searchlist->GetSearchFileByHash(Hash);
	if (sfile == NULL){
		//already deleted
		return;
	}
	for (int i = 0; i != nCount; i++){
		long nSize;
		data.Read(&nSize,4);
		BYTE* pBuffer = new BYTE[nSize];
		data.Read(pBuffer,nSize);
		CxImage* image = new CxImage(pBuffer, nSize,CXIMAGE_FORMAT_PNG);
		delete[] pBuffer;
		if (image->IsValid()){
			sfile->AddPreviewImg(image);
		}
	}
	(new PreviewDlg())->SetFile(sfile);
}

// sends a packet, if needed it will establish a connection before
// options used: ignore max connections, control packet, delete packet
// !if the functions returns false it is _possible_ that this clientobject was deleted, because the connectiontry fails 
bool CUpDownClient::SafeSendPacket(Packet* packet){
	if (socket && socket->IsConnected()){
		socket->SendPacket(packet);
		return true;
	}
	else{
		m_WaitingPackets_list.AddTail(packet);
		return TryToConnect(true);
	}
}
