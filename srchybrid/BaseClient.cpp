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
#ifdef _DEBUG
#include "DebugHelpers.h"
#endif
#include "emule.h"
#include "UpDownClient.h"
#include "FriendList.h"
#include "Clientlist.h"
#include "OtherFunctions.h"
#include "PartFile.h"
#include "ListenSocket.h"
#include "PeerCacheSocket.h"
#include "Friend.h"
#include <zlib/zlib.h>
#include "Packets.h"
#include "Opcodes.h"
#include "SafeFile.h"
#include "Preferences.h"
#include "Server.h"
#include "ClientCredits.h"
#include "IPFilter.h"
#include "Statistics.h"
#include "Sockets.h"
#include "DownloadQueue.h"
#include "UploadQueue.h"
#include "SearchList.h"
#include "SharedFileList.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "Kademlia/Kademlia/Search.h"
#include "Kademlia/Kademlia/SearchManager.h"
#include "Kademlia/Utils/UInt128.h"
#include "Kademlia/Net/KademliaUDPListener.h"
#include "Kademlia/Kademlia/Prefs.h"
#include "emuledlg.h"
#include "ServerWnd.h"
#include "TransferWnd.h"
#include "ChatWnd.h"
#include "CxImage/xImage.h"
#include "PreviewDlg.h"
#include "Exceptions.h"
#include "Peercachefinder.h"
#include "ClientUDPSocket.h"
#include "shahashset.h"
#include "Log.h"
#include "WebCache/WebCacheSocket.h" // MORPH - Added by Commander, WebCache 1.2e

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


IMPLEMENT_DYNAMIC(CClientException, CException)
IMPLEMENT_DYNAMIC(CUpDownClient, CObject)

CUpDownClient::CUpDownClient(CClientReqSocket* sender)
{
	socket = sender;
	reqfile = NULL;
	Init();
}

CUpDownClient::CUpDownClient(CPartFile* in_reqfile, uint16 in_port, uint32 in_userid,uint32 in_serverip, uint16 in_serverport, bool ed2kID)
{
	//Converting to the HybridID system.. The ED2K system didn't take into account of IP address ending in 0..
	//All IP addresses ending in 0 were assumed to be a lowID because of the calculations.
	socket = NULL;
	reqfile = in_reqfile;
	Init();
	m_nUserPort = in_port;
	//If this is a ED2K source, check if it's a lowID.. If not, convert it to a HyrbidID.
	//Else, it's already in hybrid form.
	if(ed2kID && !IsLowID(in_userid))
		m_nUserIDHybrid = ntohl(in_userid);
	else
		m_nUserIDHybrid = in_userid;

	//If highID and ED2K source, incoming ID and IP are equal..
	//If highID and Kad source, incoming IP needs ntohl for the IP
	if (!HasLowID() && ed2kID)
		m_nConnectIP = in_userid;
	else if(!HasLowID())
		m_nConnectIP = ntohl(in_userid);
	m_dwServerIP = in_serverip;
	m_nServerPort = in_serverport;
}

void CUpDownClient::Init()
{
	//SLAHAM: ADDED Known Since/Last Asked =>
	uiDLAskingCounter = 0;
	dwThisClientIsKnownSince = ::GetTickCount();
	//SLAHAM: ADDED Known Since/Last Asked <=

	//SLAHAM: ADDED Show Downloading Time =>
	uiStartDLCount = 0;
	dwStartDLTime = 0;
	dwSessionDLTime = 0;
	dwTotalDLTime = 0;
	//SLAHAM: ADDED Show Downloading Time <=

	credits = 0;
	m_nSumForAvgUpDataRate = 0;
	//MORPH START - Changed by SiRoB, ZZUL_20040904	
	/*
	m_bAddNextConnect = false;
	*/
	m_dwWouldHaveGottenUploadSlotIfNotLowIdTick = 0;  // VQB Fix for LowID slots only on connection
	//MORPH END   - Changed by SiRoB, ZZUL_20040904	
	m_nChatstate = MS_NONE;
	m_nKadState = KS_NONE;
	m_cShowDR = 0;
	m_nUDPPort = 0;
	m_nKadPort = 0;
	m_nTransferredUp = 0;
	m_cAsked = 0;
	m_cDownAsked = 0;
	m_nUpDatarate = 0;
	m_pszUsername = 0;
	m_pszFunnyNick = 0; //Added by SiRoB, Dynamic FunnyNick
	m_nUserIDHybrid = 0;
	m_dwServerIP = 0;
	m_nServerPort = 0;
	m_bLeecher = false; //MORPH - Added by IceCream, Antileecher feature
	old_m_pszUsername = 0; //MORPH - Added by IceCream, Antileecher feature
    m_iFileListRequested = 0;
	m_dwLastUpRequest = 0;
	m_bEmuleProtocol = false;
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
	m_abyUpPartStatus = 0;
	m_abyUpPartStatusHidden = 0; //MORPH - Added by SiRoB, See chunk that we hide
	m_nDownloadState = DS_NONE;
	m_dwUploadTime = 0;
	m_nTransferredDown = 0;
	m_nDownDatarate = 0;
	m_nDownDataRateMS = 0;
	m_nUploadState = US_NONE;
	m_dwLastBlockReceived = 0;
	m_byDataCompVer = 0;
	m_byUDPVer = 0;
	m_bySourceExchangeVer = 0;
	m_byAcceptCommentVer = 0;
	m_byExtendedRequestsVer = 0;
	m_nRemoteQueueRank = 0;
	m_iDifferenceQueueRank = 0;	//Morph - added by AndCycle, DiffQR
	m_dwLastSourceRequest = 0;
	m_dwLastSourceAnswer = 0;
	m_dwLastAskedForSources = 0;
	m_byCompatibleClient = 0;
	m_nSourceFrom = SF_SERVER;
	m_bIsHybrid = false;
	m_bIsML=false;
	//MOPRH START - Added by SiRoB, Is Morph Client?
	m_bIsMorph = false;
	//MOPRH END   - Added by SiRoB, Is Morph Client?
	m_Friend = NULL;
	m_uFileRating=0;
	(void)m_strFileComment;
	m_fMessageFiltered = 0;
	m_fIsSpammer = 0;
	m_cMessagesReceived = 0;
	m_cMessagesSent = 0;
	m_nCurSessionUp = 0;
	m_nCurSessionDown = 0;
    m_nCurQueueSessionUp = 0;
	m_nSumForAvgDownDataRate = 0;
	m_clientSoft=SO_UNKNOWN;
	m_bRemoteQueueFull = false;
	md4clr(m_achUserHash);
	SetBuddyID(NULL);
	m_nBuddyIP = 0;
	m_nBuddyPort = 0;
	if (socket){
		SOCKADDR_IN sockAddr = {0};
		int nSockAddrLen = sizeof(sockAddr);
		socket->GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
		SetIP(sockAddr.sin_addr.S_un.S_addr);
	}
	else{
		SetIP(0);
	}
	//EastShare Start - added by AndCycle, IP to Country
	m_structUserCountry = theApp.ip2country->GetCountryFromIP(GetIP());
	//EastShare End - added by AndCycle, IP to Country
	m_fHashsetRequesting = 0;
	m_dwRequestedHashset = 0;	// SLUGFILLER: SafeHash
	m_fSharedDirectories = 0;
	m_fSentCancelTransfer = 0;
	m_nClientVersion = 0;
	m_lastRefreshedDLDisplay = 0;
	m_dwDownStartTime = 0;
	m_nLastBlockOffset = 0;
	m_bUnicodeSupport = false;
	m_SecureIdentState = IS_UNAVAILABLE;
	m_dwLastSignatureIP = 0;
	m_bySupportSecIdent = 0;
	m_byInfopacketsReceived = IP_NONE;
	m_lastPartAsked = 0xffff;
	m_nUpCompleteSourcesCount= 0;
	m_fSupportsPreview = 0;
	m_fPreviewReqPending = 0;
	m_fPreviewAnsPending = 0;
	m_bTransferredDownMini = false;
	m_addedPayloadQueueSession = 0;
	m_nCurQueueSessionPayloadUp = 0; // PENDING: Is this necessary? ResetSessionUp()...
	m_lastRefreshedULDisplay = ::GetTickCount();
	m_bGPLEvildoer = false;
	m_bHelloAnswerPending = false;
	m_fNoViewSharedFiles = 0;
	m_bMultiPacket = 0;
	md4clr(requpfileid);
	m_nTotalUDPPackets = 0;
	m_nFailedUDPPackets = 0;
	m_nUrlStartPos = (UINT)-1;
	m_iHttpSendState = 0;
	m_fPeerCache = 0;
	m_uPeerCacheDownloadPushId = 0;
	m_uPeerCacheUploadPushId = 0;
	m_pPCDownSocket = NULL;
	m_pPCUpSocket = NULL;
	m_uPeerCacheRemoteIP = 0;
	m_ePeerCacheDownState = PCDS_NONE;
	m_ePeerCacheUpState = PCUS_NONE;
	m_bPeerCacheDownHit = false;
	m_bPeerCacheUpHit = false;
	m_fNeedOurPublicIP = 0;
    m_random_update_wait = (uint32)(rand()/(RAND_MAX/1000));
	//MORPH START - Added by SiRoB, ZZUL_20040904
    m_dwLastCheckedForEvictTick = 0;
    m_addedPayloadQueueSession = 0;
	//MORPH END   - Added by SiRoB, ZZUL_20040904	
    m_bSourceExchangeSwapped = false; // ZZ:DownloadManager
	m_dwLastTriedToConnect = ::GetTickCount()-20*60*1000; // ZZ:DownloadManager
	m_fQueueRankPending = 0;
	m_fUnaskQueueRankRecv = 0;
	m_fFailedFileIdReqs = 0;

	m_slotNumber = 0;
    lastSwapForSourceExchangeTick = 0;
	m_pReqFileAICHHash = NULL;
	m_fSupportsAICH = 0;
	m_fAICHRequested = 0;
	m_byKadVersion = 0;
	SetLastBuddyPingPongTime();
	m_fSentOutOfPartReqs = 0;
	
	//MORPH START - Added By AndCycle, ZZUL_20050212-0200
	m_bScheduledForRemoval = false;
	m_bScheduledForRemovalWillKeepWaitingTimeIntact = false;
	//MORPH END   - Added By AndCycle, ZZUL_20050212-0200

	//MORPH START - Added by SiRoB, ET_MOD_VERSION 0x55
	m_strModVersion.Empty();
	//MORPH END   - Added by SiRoB, ET_MOD_VERSION 0x55
	m_uNotOfficial = 0;  //MOPRH - Added by SiRoB,  Control Mod Tag
	m_nDownTotalTime = 0;//wistily Total download time for this client for this emule session
	m_nUpTotalTime = 0;//wistily Total upload time for this client for this emule session
	//Morph Start - added by AndCycle, ICS
	// enkeyDev: ICS
	m_abyIncPartStatus = 0;
	m_incompletepartVer = 0;
	// <--- enkeyDev: ICS
	//Morph End - added by AndCycle, ICS

    // MORPH START - Added by Commander, WebCache 1.2e
    m_bProxy = false;
	m_bIsAcceptingOurOhcbs = true;
	m_bIsTrustedOHCBSender = true;
	m_bIsAllowedToSendOHCBs = true;
	m_uWebCacheFlags = 0;
	m_pWCDownSocket = NULL;
	m_pWCUpSocket = NULL;
	m_WA_webCacheIndex = -1;
	m_bWebCacheSupport = false;
	m_uWebCacheDownloadId = 0;
	m_uWebCacheUploadId = 0;
	m_eWebCacheDownState = WCDS_NONE;
	m_eWebCacheUpState = WCUS_NONE;
	//blocksLoaded = 0; //JP blocks are counted in the WCDownSocket code now
	b_webcacheInfoNeeded = false;
	//JP trusted OHCB-Senders Start
	WebCachedBlockRequests = 0;
	SuccessfulWebCachedBlockDownloads = 0;
	//JP trusted OHCB-Senders END
	// Superlexx - encryption
	Crypt.useNewKey = true;
	Crypt.isProxy = false;
	GenerateKey(Crypt.remoteMasterKey);	// generate a key - will be done right before sending
	for (int i=0; i<WC_KEYLENGTH; i++) Crypt.localMasterKey[i] = 0; // fill with zeroes so we can say if the key is valid
    lastMultiOHCBPacketSent = 0; // Superlexx - Multi-OHCB
	m_bWebCacheSupportsMultiOHCBs = false;
	// yonatan http end ////////////////////////////////////////////////////////////////////////////
	// MORPH END - Added by Commander, WebCache 1.2e
}

CUpDownClient::~CUpDownClient(){
	if (IsAICHReqPending()){
		m_fAICHRequested = FALSE;
		CAICHHashSet::ClientAICHRequestFailed(this);
	}

	theApp.clientlist->RemoveClient(this, _T("Destructing client object"));
	if (m_Friend){
		m_Friend->SetLinkedClient(NULL);
	}
	if (socket){
		socket->client = 0;
		socket->Safe_Delete();
	}

	//MORPH START - Added by SiRoB, Keep A4AF infos
	POSITION			pos = m_PartStatus_list.GetStartPosition();
	CPartFile*			curFile;
	uint8*				curPS;
	while (pos)
	{
		m_PartStatus_list.GetNextAssoc(pos, curFile, curPS);
		if (curPS != m_abyPartStatus)
			delete[] curPS;
	}
	//MORPH START - Added by AndCycle, ICS, Keep A4AF infos
	pos = m_IncPartStatus_list.GetStartPosition();
	while (pos)
	{
		m_IncPartStatus_list.GetNextAssoc(pos, curFile, curPS);
		if (curPS != m_abyIncPartStatus)
			delete[] curPS;
	}
	//MORPH END - Added by AndCycle, ICS, Keep A4AF infos
	m_nUpCompleteSourcesCount_list.RemoveAll();
	//MORPH END   - Added by SiRoB, Keep A4AF infos

	if (m_pPCDownSocket){
		m_pPCDownSocket->client = NULL;
		m_pPCDownSocket->Safe_Delete();
	}
	if (m_pPCUpSocket){
		m_pPCUpSocket->client = NULL;
		m_pPCUpSocket->Safe_Delete();
	}
	// MORPH START - Added by Commander, WebCache 1.2e
	if (m_pWCDownSocket){
		m_pWCDownSocket->client = NULL;
		m_pWCDownSocket->Safe_Delete();
	}
	if (m_pWCUpSocket){
		m_pWCUpSocket->client = NULL;
		m_pWCUpSocket->Safe_Delete();
	}
	// MORPH END - Added by Commander, WebCache 1.2e
	if (m_pszUsername)
		free(m_pszUsername);
	//FunnyNick
	if (m_pszFunnyNick) {
		delete[] m_pszFunnyNick;
		m_pszFunnyNick = NULL;
	}
	if (m_abyPartStatus){
		delete[] m_abyPartStatus;
		m_abyPartStatus = NULL;
	}
	if (m_abyUpPartStatus){
		delete[] m_abyUpPartStatus;
		m_abyUpPartStatus = NULL;
	}
	//Morph Start - added by AndCycle, ICS
	// enkeyDev: ICS
	if (m_abyIncPartStatus)
		delete[] m_abyIncPartStatus;
	// <--- enkeyDev: ICS
	//Morph End - added by AndCycle, ICS
	//MORPH START - Added by SiRoB, See chunk that we hide
	if (m_abyUpPartStatusHidden){
		delete[] m_abyUpPartStatusHidden;
		m_abyUpPartStatusHidden = NULL;
	}
	//MORPH END   - Added by SiRoB, See chunk that we hide
	ClearUploadBlockRequests();

	for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition();pos != 0;)
		delete m_DownloadBlocks_list.GetNext(pos);

	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition();pos != 0;)
		delete m_RequestedFiles_list.GetNext(pos);

	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0;){
		Pending_Block_Struct *pending = m_PendingBlocks_list.GetNext(pos);
		delete pending->block;
		// Not always allocated
		if (pending->zStream){
			inflateEnd(pending->zStream);
			delete pending->zStream;
		}
		delete pending;
	}

	for (POSITION pos = m_WaitingPackets_list.GetHeadPosition();pos != 0;)
		delete m_WaitingPackets_list.GetNext(pos);
	
	DEBUG_ONLY (theApp.listensocket->Debug_ClientDeleted(this));
	SetUploadFileID(NULL);

	m_fileReaskTimes.RemoveAll(); // ZZ:DownloadManager (one resk timestamp for each file)

	if (m_pReqFileAICHHash != NULL)
		delete m_pReqFileAICHHash;
}

//MORPH START - Added by IceCream, Anti-leecher feature
LPCTSTR CUpDownClient::TestLeecher(){
	if (old_m_strClientSoftware != m_strClientSoftware)
	{
		if (StrStrI(m_strModVersion,_T("Freeza"))||
			StrStrI(m_strModVersion,_T("d-unit"))||
			//StrStrI(m_strModVersion,_T("NOS"))|| //removed for the moment
			StrStrI(m_strModVersion,_T("imperator"))||
			StrStrI(m_strModVersion,_T("SpeedLoad"))||
			StrStrI(m_strModVersion,_T("gt mod"))||
			StrStrI(m_strModVersion,_T("egomule"))||
			//StrStrI(m_strModVersion,"aldo")|| //removed for the moment
			StrStrI(m_strModVersion,_T("darkmule"))||
			StrStrI(m_strModVersion,_T("LegoLas"))||
			StrStrI(m_strModVersion,_T("dodgethis"))|| //Updated
			StrStrI(m_strModVersion,_T("DM-"))|| //hotfix
			StrStrI(m_strModVersion,_T("|X|"))||
			StrStrI(m_strModVersion,_T("eVorte"))||
			StrStrI(m_strModVersion,_T("Mison"))||
			StrStrI(m_strModVersion,_T("father"))||
			StrStrI(m_strModVersion,_T("Dragon"))||
			StrStrI(m_strModVersion,_T("booster"))|| //Temporaly added, must check the tag
			StrStrI(m_strModVersion,_T("$motty"))||
			StrStrI(m_strModVersion,_T("Thunder"))||
			StrStrI(m_strModVersion,_T("BuzzFuzz"))||
			StrStrI(m_strModVersion,_T("Speed-Unit"))|| 
			StrStrI(m_strModVersion,_T("Killians"))||
			StrStrI(m_strModVersion,_T("Element"))|| 
			StrStrI(m_strModVersion,_T("���]"))||
            StrStrI(m_strModVersion,_T("00de"))|| //Commander - Added: LeecherMod
			StrStrI(m_strModVersion,_T("Rappi"))||//Commander - Added: LeecherMod
			StrStrI(m_strModVersion,_T("EastShare")) && StrStrI(m_strClientSoftware,_T("0.29"))||
			// EastShare END - Added by TAHO, Pretender
			StrStrI(m_strModVersion,_T("LSD.7c")) && !StrStrI(m_strClientSoftware,_T("27"))||
			StrStrI(m_strModVersion,_T("Morph")) && (StrStrI(m_strModVersion,_T("Max")) || StrStrI(m_strModVersion,_T("+")))||
			StrStrI(m_strModVersion,_T("eChanblard v7.0")) ||
			StrStrI(m_strModVersion,_T("ACAT")) ||
			StrStrI(m_strModVersion,_T("!FREEANGEL!")) ||
			StrStrI(m_strModVersion,_T("          ")) ||
			m_strModVersion.IsEmpty() == false && StrStrI(m_strClientSoftware,_T("edonkey"))||
			((GetVersion()>589) && (GetSourceExchangeVersion()>0) && (GetClientSoft()==51)) //LSD, edonkey user with eMule property
			)
		{
			old_m_strClientSoftware = m_strClientSoftware;
			return _T("Bad MODSTRING");
		}
	}
	/*if (old_m_pszUsername != m_pszUsername)
	{
		if (StrStrI(m_pszUsername,_T("$GAM3R$"))||
			StrStrI(m_pszUsername,_T("G@m3r"))||
			StrStrI(m_pszUsername,_T("$WAREZ$"))||
			StrStrI(m_pszUsername,_T("RAMMSTEIN"))||//	
			//StrStrI(m_pszUsername,_T("toXic"))|| //removed for the moment
			StrStrI(m_pszUsername,_T("Leecha"))||
			//StrStrI(m_pszUsername,_T("eDevil"))|| //removed for the moment
			StrStrI(m_pszUsername,_T("darkmule"))||
			StrStrI(m_pszUsername,_T("phArAo"))||
			StrStrI(m_pszUsername,_T("dodgethis"))||
			StrStrI(m_pszUsername,_T("Reverse"))||
			StrStrI(m_pszUsername,_T("eVortex"))||
			StrStrI(m_pszUsername,_T("|eVorte|X|"))||
			StrStrI(m_pszUsername,_T("Chief"))||
			//StrStrI(m_pszUsername,"Mison"))|| //Temporaly desactivated, ban only on mod tag
			StrStrI(m_pszUsername,_T("$motty"))||
			StrStrI(m_pszUsername,_T("emule-speed"))||
			StrStrI(m_pszUsername,_T("celinesexy"))||
			StrStrI(m_pszUsername,_T("Gate-eMule"))||
			StrStrI(m_pszUsername,_T("energyfaker"))||
			StrStrI(m_pszUsername,_T("BuzzFuzz"))||
			StrStrI(m_pszUsername,_T("Speed-Unit"))|| 
			StrStrI(m_pszUsername,_T("Killians"))||
			StrStrI(m_pszUsername,_T("pubsman"))||
			StrStrI(m_pszUsername,_T("emule-element"))||
			StrStrI(m_pszUsername,_T("00de.de"))|| //Commander - Added: LeecherMod
			StrStrI(m_pszUsername,_T("emule")) && StrStrI(m_pszUsername,_T("booster"))
			)
		{
			old_m_pszUsername = m_pszUsername;
			return _T("Bad USERNAME");
		}
	}
	*/
	if (!m_strNotOfficial.IsEmpty() && m_strModVersion.IsEmpty() && (m_clientSoft == SO_EMULE) && (m_nClientVersion <= MAKE_CLIENT_VERSION(CemuleApp::m_nVersionMjr, CemuleApp::m_nVersionMin, CemuleApp::m_nVersionUpd))){
		return _T("Ghost MOD");
	}else if (StrStrI(m_strModVersion,theApp.m_strModVersion) && (m_uNotOfficial != 0x4394 &&  m_uNotOfficial != 0x11094 || m_nClientVersion < MAKE_CLIENT_VERSION(CemuleApp::m_nVersionMjr, CemuleApp::m_nVersionMin, CemuleApp::m_nVersionUpd))){
		return _T("Fake MODSTRING");
	}else if (m_nClientVersion > MAKE_CLIENT_VERSION(0, 30, 0) && m_byEmuleVersion > 0 && m_byEmuleVersion != 0x99 && m_clientSoft == SO_EMULE){
		return _T("Fake emuleVersion");
	}else	if (IsLeecher())
		return _T("Allready Known");
	return NULL;
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
	m_fSupportsPreview = 0;
	m_nClientVersion = 0;
	m_fSharedDirectories = 0;
	m_bMultiPacket = 0;
	m_fPeerCache = 0;
	m_uPeerCacheDownloadPushId = 0;
	m_uPeerCacheUploadPushId = 0;
	m_byKadVersion = 0;
}

bool CUpDownClient::ProcessHelloPacket(const uchar* pachPacket, uint32 nSize)
{
	CSafeMemFile data(pachPacket, nSize);
	data.ReadUInt8(); // read size of userhash
	// reset all client properties; a client may not send a particular emule tag any longer
	ClearHelloProperties();
	return ProcessHelloTypePacket(&data);
}

bool CUpDownClient::ProcessHelloAnswer(const uchar* pachPacket, uint32 nSize)
{
	CSafeMemFile data(pachPacket, nSize);
	bool bIsMule = ProcessHelloTypePacket(&data);
	m_bHelloAnswerPending = false;
	return bIsMule;
}

bool CUpDownClient::ProcessHelloTypePacket(CSafeMemFile* data)
{
	bool bDbgInfo = thePrefs.GetUseDebugDevice();
	m_strHelloInfo.Empty();
	// clear hello properties which can be changed _only_ on receiving OP_Hello/OP_HelloAnswer
	m_bIsHybrid = false;
	m_bIsML = false;
	m_fNoViewSharedFiles = 0;
	m_bUnicodeSupport = false;
	m_incompletepartVer = 0; // enkeyDev: ICS //Morph - added by AndCycle, ICS
	m_uNotOfficial = 0; //MOPRH - Added by SiRoB, Control Mod Tag
	//MORPH START - Added by SiRoB, ET_MOD_VERSION 0x55
	m_strModVersion.Empty();
	//MORPH END   - Added by SiRoB, ET_MOD_VERSION 0x55
	//MOPRH START - Added by SiRoB, Is Morph Client?
	m_bIsMorph = false;
	//MOPRH END   - Added by SiRoB, Is Morph Client?
	
	data->ReadHash16(m_achUserHash);
	if (bDbgInfo)
		m_strHelloInfo.AppendFormat(_T("Hash=%s (%s)"), md4str(m_achUserHash), DbgGetHashTypeString(m_achUserHash));
	m_nUserIDHybrid = data->ReadUInt32();
	if (bDbgInfo)
		m_strHelloInfo.AppendFormat(_T("  UserID=%u (%s)"), m_nUserIDHybrid, ipstr(m_nUserIDHybrid));
	uint16 nUserPort = data->ReadUInt16(); // hmm clientport is sent twice - why?
	if (bDbgInfo)
		m_strHelloInfo.AppendFormat(_T("  Port=%u"), nUserPort);
	
	DWORD dwEmuleTags = 0;
	uint32 tagcount = data->ReadUInt32();
	if (bDbgInfo)
		m_strHelloInfo.AppendFormat(_T("  Tags=%u"), tagcount);
	//MOPRH START - Added by SiRoB, Control Mod Tag
	m_strNotOfficial.Empty();
	CString strBanReason = NULL;
 	//MOPRH END   - Added by SiRoB, Control Mod Tag
	for (uint32 i = 0;i < tagcount; i++){
		CTag temptag(data, true);
		m_uNotOfficial <<= 1; //MOPRH - Added by SiRoB, Control Mod Tag
		m_uNotOfficial ^= temptag.GetNameID(); //MOPRH - Added by SiRoB,  Control Mod Tag
		switch (temptag.GetNameID()){
			case CT_NAME:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsStr()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: CT_NAME"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				if (m_pszUsername){
					free(m_pszUsername);
					m_pszUsername = NULL; // needed, in case 'nstrdup' fires an exception!!
				}
				m_pszUsername = _tcsdup(temptag.GetStr());
				if (bDbgInfo){
					if (m_pszUsername){//filter username for bad chars
						TCHAR* psz = m_pszUsername;
						while (*psz != _T('\0')) {
							if (*psz == _T('\n') || *psz == _T('\r'))
								*psz = _T(' ');
							psz++;
						}
					}
					m_strHelloInfo.AppendFormat(_T("\n  Name='%s'"), m_pszUsername);
				}
				break;
			case CT_VERSION:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: CT_VERSION"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  Version=%u"), temptag.GetInt());
				m_nClientVersion = temptag.GetInt();
				break;
			case CT_PORT:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: CT_PORT"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  Port=%u"), temptag.GetInt());
				nUserPort = temptag.GetInt();
				break;
			case CT_MOD_VERSION:
				m_strNotOfficial.AppendFormat(_T(",MID=%s"),temptag.GetFullInfo()); //MOPRH - Added by SiRoB, Control Mod Tag
				if (temptag.IsStr())
				{
					m_strModVersion = temptag.GetStr();
					//MOPRH START - Added by SiRoB, Is Morph Client?
					m_bIsMorph = StrStrI(m_strModVersion,_T("Morph"))!=0;
					//MOPRH END   - Added by SiRoB, Is Morph Client?
				}
				else if (temptag.IsInt())
					m_strModVersion.Format(_T("ModID=%u"), temptag.GetInt());
				else
					m_strModVersion = _T("ModID=<Unknwon>");
				if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ModID=%s"), m_strModVersion);
				CheckForGPLEvilDoer();
				break;
			// MORPH START - Added by Commander, WebCache 1.2e
			case WC_TAG_VOODOO:
				m_strNotOfficial.AppendFormat(_T(",WCV=%s"),temptag.GetFullInfo());
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("Suspect Hello-Tag: %s"),apszSnafuTag[3]);
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				m_bWebCacheSupport = temptag.IsInt() && temptag.GetInt() == 'ARC4';
				break;
			case WC_TAG_FLAGS:
				m_strNotOfficial.AppendFormat(_T(",WCF=%s"),temptag.GetFullInfo()); //MOPRH - Added by SiRoB, Control Mod Tag
				if (m_bWebCacheSupport && temptag.IsInt())
				{
					m_uWebCacheFlags = temptag.GetInt();
					b_webcacheInfoNeeded = m_uWebCacheFlags & WC_FLAGS_INFO_NEEDED;
					m_bWebCacheSupportsMultiOHCBs = (m_uWebCacheFlags & WC_FLAGS_MULTI_OHCBS)!=0;
				}
				break;
			// Superlexx webcache - moved to the multipacket
			// MORPH END - Added by Commander, WebCache 1.2e
			case CT_EMULE_UDPPORTS:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: CT_EMULE_UDPPORTS"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				// 16 KAD Port
				// 16 UDP Port
				m_nKadPort = (uint16)(temptag.GetInt() >> 16);
				m_nUDPPort = (uint16)temptag.GetInt();
				if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  KadPort=%u  UDPPort=%u"), m_nKadPort, m_nUDPPort);
				dwEmuleTags |= 1;
				break;
			case CT_EMULE_BUDDYUDP:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: CT_EMULE_BUDDYUDP"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				// 16 --Reserved for future use--
				// 16 BUDDY Port
				m_nBuddyPort = (uint16)temptag.GetInt();
				if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  BuddyPort=%u"), m_nBuddyPort);
				break;
			case CT_EMULE_BUDDYIP:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: CT_EMULE_BUDDYIP"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				// 32 BUDDY IP
				m_nBuddyIP = temptag.GetInt();
				if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  BuddyIP=%u"), m_nBuddyIP);
				break;
			case CT_EMULE_MISCOPTIONS1:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: CT_EMULE_MISCOPTIONS1"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				//  3 AICH Version (0 = not supported)
				//  1 Unicode
				//  4 UDP version
				//  4 Data compression version
				//  4 Secure Ident
				//  4 Source Exchange
				//  4 Ext. Requests
				//  4 Comments
				//	1 PeerChache supported
				//	1 No 'View Shared Files' supported
				//	1 MultiPacket
				//  1 Preview
				m_fSupportsAICH			= (temptag.GetInt() >> 29) & 0x07;
				m_bUnicodeSupport		= (temptag.GetInt() >> 28) & 0x01;
				m_byUDPVer				= (temptag.GetInt() >> 24) & 0x0f;
				m_byDataCompVer			= (temptag.GetInt() >> 20) & 0x0f;
				m_bySupportSecIdent		= (temptag.GetInt() >> 16) & 0x0f;
				m_bySourceExchangeVer	= (temptag.GetInt() >> 12) & 0x0f;
				m_byExtendedRequestsVer	= (temptag.GetInt() >>  8) & 0x0f;
				m_byAcceptCommentVer	= (temptag.GetInt() >>  4) & 0x0f;
				m_fPeerCache			= (temptag.GetInt() >>  3) & 0x01;
				m_fNoViewSharedFiles	= (temptag.GetInt() >>  2) & 0x01;
				m_bMultiPacket			= (temptag.GetInt() >>  1) & 0x01;
				m_fSupportsPreview		= (temptag.GetInt() >>  0) & 0x01;
				dwEmuleTags |= 2;
				if (bDbgInfo){
					m_strHelloInfo.AppendFormat(_T("\n  PeerCache=%u  UDPVer=%u  DataComp=%u  SecIdent=%u  SrcExchg=%u")
												_T("  ExtReq=%u  Commnt=%u  Preview=%u  NoViewFiles=%u  Unicode=%u"), 
												m_fPeerCache, m_byUDPVer, m_byDataCompVer, m_bySupportSecIdent, m_bySourceExchangeVer, 
												m_byExtendedRequestsVer, m_byAcceptCommentVer, m_fSupportsPreview, m_fNoViewSharedFiles, m_bUnicodeSupport);
				}
				break;
			case CT_EMULE_MISCOPTIONS2:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: CT_EMULE_MISCOPTIONS2"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				//	28 Reserved
				//   4 Kad Version
				m_byKadVersion			= (temptag.GetInt() >>  0) & 0x0f;
				dwEmuleTags |= 8;
				if (bDbgInfo){
					m_strHelloInfo.AppendFormat(_T("\n  KadVersion=%u"  ), m_byKadVersion );
				}
				break;
			case CT_EMULE_VERSION:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: CT_EMULE_VERSION"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				//  8 Compatible Client ID
				//  7 Mjr Version (Doesn't really matter..)
				//  7 Min Version (Only need 0-99)
				//  3 Upd Version (Only need 0-5)
				//  7 Bld Version (Only need 0-99) -- currently not used
				m_byCompatibleClient = (temptag.GetInt() >> 24);
				m_nClientVersion = temptag.GetInt() & 0x00ffffff;
				m_byEmuleVersion = 0x99;
				m_fSharedDirectories = 1;
				dwEmuleTags |= 4;
				if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ClientVer=%u.%u.%u.%u  Comptbl=%u"), (m_nClientVersion >> 17) & 0x7f, (m_nClientVersion >> 10) & 0x7f, (m_nClientVersion >> 7) & 0x07, m_nClientVersion & 0x7f, m_byCompatibleClient);
				break;
			//Morph Start - added by AndCycle, ICS
			// enkeyDEV: ICS
			case ET_INCOMPLETEPARTS:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				m_strNotOfficial.AppendFormat(_T(",ICS=%s"),temptag.GetFullInfo());
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: ET_INCOMPLETEPARTS"));
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				m_incompletepartVer = temptag.GetInt();
				break;
			// <--- enkeyDEV: ICS
			//Morph End - added by AndCycle, ICS
			default:
				//<<< [SNAFU_V3] Check unknown tags !
				if (!((temptag.GetNameID() & 0xF0)==0xF0) || strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
					ProcessUnknownHelloTag(&temptag, strBanReason);
				//>>> [SNAFU_V3] Save unknown tags !
				m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo()); //MOPRH - Added by SiRoB, Control Mod Tag
				if (bDbgInfo)
					m_strHelloInfo.AppendFormat(_T("\n  ***UnkTag=%s"), temptag.GetFullInfo());
		}
	}
	m_nUserPort = nUserPort;
	m_dwServerIP = data->ReadUInt32();
	m_nServerPort = data->ReadUInt16();
	if (bDbgInfo)
		m_strHelloInfo.AppendFormat(_T("\n  Server=%s:%u"), ipstr(m_dwServerIP), m_nServerPort);
	// Hybrid now has an extra uint32.. What is it for?
	// Also, many clients seem to send an extra 6? These are not eDonkeys or Hybrids..
	if ( data->GetLength() - data->GetPosition() == sizeof(uint32) ){
		uint32 test = data->ReadUInt32();
		if (test=='KDLM') 
		{
			m_bIsML=true;
			if (bDbgInfo)
				m_strHelloInfo += _T("\n  ***AddData: \"MLDK\"");
		}
		else{
			m_bIsHybrid = true;
			m_fSharedDirectories = 1;
			if (bDbgInfo)
				m_strHelloInfo.AppendFormat(_T("\n  ***AddData: uint32=%u (0x%08x)"), test, test);
		}
		}
	else if (data->GetPosition() < data->GetLength()){
		UINT uAddHelloDataSize = (UINT)(data->GetLength() - data->GetPosition());
		//MOPRH - Added by SiRoB, Control Mod Tag
		m_strNotOfficial.AppendFormat(_T(",ExtraByte=%u"),uAddHelloDataSize);
		if(strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
			strBanReason=_T("ExtraBytes");
		//MOPRH - Added by SiRoB, Control Mod Tag
		if (bDbgInfo) {
			if (uAddHelloDataSize == sizeof(uint32)){
				DWORD dwAddHelloInt32 = data->ReadUInt32();
				m_strHelloInfo.AppendFormat(_T("\n  ***AddData: uint32=%u (0x%08x)"), dwAddHelloInt32, dwAddHelloInt32);
			}
			else if (uAddHelloDataSize == sizeof(uint32)+sizeof(uint16)){
				DWORD dwAddHelloInt32 = data->ReadUInt32();
				WORD w = data->ReadUInt16();
				m_strHelloInfo.AppendFormat(_T("\n  ***AddData: uint32=%u (0x%08x),  uint16=%u (0x%04x)"), dwAddHelloInt32, dwAddHelloInt32, w, w);
			}
			else
				m_strHelloInfo.AppendFormat(_T("\n  ***AddData: %u bytes"), uAddHelloDataSize);
		}
	}

	SOCKADDR_IN sockAddr = {0};
	int nSockAddrLen = sizeof(sockAddr);
	socket->GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
	SetIP(sockAddr.sin_addr.S_un.S_addr);
	//EastShare Start - added by AndCycle, IP to Country
	m_structUserCountry = theApp.ip2country->GetCountryFromIP(GetIP());
	//EastShare End - added by AndCycle, IP to Country

	if (thePrefs.AddServersFromClient() && m_dwServerIP && m_nServerPort){
		CServer* addsrv = new CServer(m_nServerPort, ipstr(m_dwServerIP));
		addsrv->SetListName(addsrv->GetAddress());
		if (!theApp.emuledlg->serverwnd->serverlistctrl.AddServer(addsrv, true))
			delete addsrv;
	}

	//(a)If this is a highID user, store the ID in the Hybrid format.
	//(b)Some older clients will not send a ID, these client are HighID users that are not connected to a server.
	//(c)Kad users with a *.*.*.0 IPs will look like a lowID user they are actually a highID user.. They can be detected easily
	//because they will send a ID that is the same as their IP..
	if(!HasLowID() || m_nUserIDHybrid == 0 || m_nUserIDHybrid == m_dwUserIP ) 
		m_nUserIDHybrid = ntohl(m_dwUserIP);

	CClientCredits* pFoundCredits = theApp.clientcredits->GetCredit(m_achUserHash);
	if (credits == NULL){
		credits = pFoundCredits;
		if (!theApp.clientlist->ComparePriorUserhash(m_dwUserIP, m_nUserPort, pFoundCredits)){
			if (thePrefs.GetLogBannedClients())
				AddDebugLogLine(false, _T("Clients: %s (%s), Banreason: Userhash changed (Found in TrackedClientsList)"), GetUserName(), ipstr(GetConnectIP()));
			Ban();
		}
	}
	else if (credits != pFoundCredits){
		// userhash change ok, however two hours "waittime" before it can be used
		credits = pFoundCredits;
		if (thePrefs.GetLogBannedClients())
			AddDebugLogLine(false, _T("Clients: %s (%s), Banreason: Userhash changed"), GetUserName(), ipstr(GetConnectIP()));
		Ban();
	}

	if ((m_Friend = theApp.friendlist->SearchFriend(m_achUserHash, m_dwUserIP, m_nUserPort)) != NULL){
		// Link the friend to that client
		m_Friend->SetLinkedClient(this);
	}
	else{
		// avoid that an unwanted client instance keeps a friend slot
		SetFriendSlot(false);
	}

	// check for known major gpl breaker
	CString strBuffer = m_pszUsername;
	strBuffer.MakeUpper();
	strBuffer.Remove(_T(' '));
	if (strBuffer.Find(_T("EMULE-CLIENT")) != -1 || strBuffer.Find(_T("POWERMULE")) != -1 ){
		m_bGPLEvildoer = true;  
	}

	ReGetClientSoft();
	
	m_byInfopacketsReceived |= IP_EDONKEYPROTPACK;
	// check if at least CT_EMULEVERSION was received, all other tags are optional
	bool bIsMule = (dwEmuleTags & 0x04) == 0x04;
	if (bIsMule){
		m_bEmuleProtocol = true;
		m_byInfopacketsReceived |= IP_EMULEPROTPACK;
		//MORPH START - Added by SiRoB, Anti-leecher feature
		if(thePrefs.GetEnableAntiCreditHack() && strBanReason.IsEmpty())
			if (theApp.GetID()!=m_nUserIDHybrid && memcmp(m_achUserHash, thePrefs.GetUserHash(), 16)==0)
				strBanReason = _T("Anti Credit Hack");
		if(thePrefs.GetEnableAntiLeecher()){
			if (strBanReason.IsEmpty())
				strBanReason = TestLeecher(); 
			if(!strBanReason.IsEmpty())
				BanLeecher(strBanReason);
		}
		//MORPH END   - Added by SiRoB, Anti-leecher feature
	}
	//MORPH START - Added by SiRoB, Anti-leecher feature
	else if(!strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
		BanLeecher(strBanReason);
	//MORPH END   - Added by SiRoB, Anti-leecher feature
	
	//MORPH START - Moved by SiRoB, xrmb Funnynick START
	UpdateFunnyNick();
	//MORPH END   - Moved by IceCream, xrmb Funnynick END

	if (thePrefs.GetVerbose() && GetServerIP() == INADDR_NONE)
		AddDebugLogLine(false, _T("Received invalid server IP %s from %s"), ipstr(GetServerIP()), DbgGetClientInfo());

	if( GetKadPort() )
	{
		Kademlia::CKademlia::bootstrap(ntohl(GetIP()), GetKadPort());
	}

	return bIsMule;
}

// returns 'false', if client instance was deleted!
bool CUpDownClient::SendHelloPacket(){
	if (socket == NULL){
		ASSERT(0);
		return true;
	}

	CSafeMemFile data(128);
	data.WriteUInt8(16); // size of userhash
	SendHelloTypePacket(&data);
	Packet* packet = new Packet(&data);
	packet->opcode = OP_HELLO;
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__Hello", this);
	theStats.AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true);
	AskTime=::GetTickCount(); //MORPH - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]

	m_bHelloAnswerPending = true;
	return true;
}

void CUpDownClient::SendMuleInfoPacket(bool bAnswer){
	if (socket == NULL){
		ASSERT(0);
		return;
	}

	CSafeMemFile data(128);
	data.WriteUInt8(theApp.m_uCurVersionShort);
	data.WriteUInt8(EMULE_PROTOCOL);
	//MORPH START - Added by SiRoB, Don't send MOD_VERSION to client that don't support it to reduce overhead
	bool bSendModVersion = (m_strModVersion.GetLength() || m_pszUsername==NULL) && !IsLeecher();
	if (bSendModVersion)
		data.WriteUInt32(7/*7 OFFICIAL*/+1/*ET_MOD_VERSION*/+1/*enkeyDev: ICS*/); // nr. of tags
	else
	//MORPH END   - Added by SiRoB, Don't send MOD_VERSION to client that don't support it to reduce overhead
		data.WriteUInt32(7); // nr. of tags
	CTag tag(ET_COMPRESSION,1);
	tag.WriteTagToFile(&data);
	CTag tag2(ET_UDPVER,4);
	tag2.WriteTagToFile(&data);
	CTag tag3(ET_UDPPORT,thePrefs.GetUDPPort());
	tag3.WriteTagToFile(&data);
	CTag tag4(ET_SOURCEEXCHANGE,3);
	tag4.WriteTagToFile(&data);
	CTag tag5(ET_COMMENTS,1);
	tag5.WriteTagToFile(&data);
	CTag tag6(ET_EXTENDEDREQUEST,2);
	tag6.WriteTagToFile(&data);

	uint32 dwTagValue = (theApp.clientcredits->CryptoAvailable() ? 3 : 0);
	if (thePrefs.CanSeeShares() != vsfaNobody) // set 'Preview supported' only if 'View Shared Files' allowed
		dwTagValue |= 128;
	CTag tag7(ET_FEATURES, dwTagValue);
	tag7.WriteTagToFile(&data);
	if (bSendModVersion){ //MORPH - Added by SiRoB, Don't send MOD_VERSION to client that don't support it to reduce overhead
		CTag tag8(ET_MOD_VERSION, theApp.m_strModVersion);
		tag8.WriteTagToFile(&data);
		//Morph Start - added by AndCycle, ICS
		// enkeyDev: ICS
		CTag tag9(ET_INCOMPLETEPARTS,1);
		tag9.WriteTagToFile(&data);
		// <--- enkeyDev: ICS
		//Morph End - added by AndCycle, ICS
	}  //MORPH - Added by SiRoB, Don't send MOD_VERSION to client that don't support it to reduce overhead
	Packet* packet = new Packet(&data,OP_EMULEPROT);
	if (!bAnswer)
		packet->opcode = OP_EMULEINFO;
	else
		packet->opcode = OP_EMULEINFOANSWER;
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend(!bAnswer ? "OP__EmuleInfo" : "OP__EmuleInfoAnswer", this);
	theStats.AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true,true);
}

void CUpDownClient::ProcessMuleInfoPacket(const uchar* pachPacket, uint32 nSize)
{
	bool bDbgInfo = thePrefs.GetUseDebugDevice();
	m_strMuleInfo.Empty();
	CSafeMemFile data(pachPacket, nSize);
	m_byCompatibleClient = 0;
	m_byEmuleVersion = data.ReadUInt8();
	if (bDbgInfo)
		m_strMuleInfo.AppendFormat(_T("EmuleVer=0x%x"), (UINT)m_byEmuleVersion);
	if( m_byEmuleVersion == 0x2B)
		m_byEmuleVersion = 0x22;
	uint8 protversion = data.ReadUInt8();
	if (bDbgInfo)
		m_strMuleInfo.AppendFormat(_T("  ProtVer=%u"), (UINT)protversion);

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
	m_incompletepartVer = 0;	// enkeyDEV: ICS //Morph - added by AndCycle, ICS

	uint32 tagcount = data.ReadUInt32();
	if (bDbgInfo)
		m_strMuleInfo.AppendFormat(_T("  Tags=%u"), (UINT)tagcount);
	CString strBanReason=NULL; //MORPH - Added by SiRoB, Control mod Tag
	for (uint32 i = 0;i < tagcount; i++){
		CTag temptag(&data, false);
		m_uNotOfficial <<= 1; //MOPRH - Added by SiRoB, Control Mod Tag
		m_uNotOfficial ^= temptag.GetNameID(); //MOPRH - Added by SiRoB,  Control Mod Tag
		switch (temptag.GetNameID()){
			case ET_COMPRESSION:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: ET_COMPRESSION"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: data compression version
				m_byDataCompVer = temptag.GetInt();
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  Compr=%u"), (UINT)temptag.GetInt());
				break;
			case ET_UDPPORT:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: ET_UDPPORT"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				// Bits 31-16: 0 - reserved
				// Bits 15- 0: UDP port
				m_nUDPPort = temptag.GetInt();
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  UDPPort=%u"), (UINT)temptag.GetInt());
				break;
			case ET_UDPVER:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: ET_UDPVER"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: UDP protocol version
				m_byUDPVer = temptag.GetInt();
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  UDPVer=%u"), (UINT)temptag.GetInt());
				break;
			case ET_SOURCEEXCHANGE:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: ET_SOURCEEXCHANGE"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: source exchange protocol version
				m_bySourceExchangeVer = temptag.GetInt();
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  SrcExch=%u"), (UINT)temptag.GetInt());
				break;
			case ET_COMMENTS:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: ET_COMMENTS"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: comments version
				m_byAcceptCommentVer = temptag.GetInt();
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  Commnts=%u"), (UINT)temptag.GetInt());
				break;
			case ET_EXTENDEDREQUEST:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: ET_EXTENDEDREQUEST"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: extended requests version
				m_byExtendedRequestsVer = temptag.GetInt();
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ExtReq=%u"), (UINT)temptag.GetInt());
				break;
			case ET_COMPATIBLECLIENT:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: ET_COMPATIBLECLIENT"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				// Bits 31- 8: 0 - reserved
				// Bits  7- 0: compatible client ID
				m_byCompatibleClient = temptag.GetInt();
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  Comptbl=%u"), (UINT)temptag.GetInt());
				break;
			case ET_FEATURES:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: ET_FEATURES"));
					m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo());
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				// Bits 31- 8: 0 - reserved
				// Bit	    7: Preview
				// Bit   6- 0: secure identification
				m_bySupportSecIdent = (temptag.GetInt()     ) & 3;
				m_fSupportsPreview  = (temptag.GetInt() >> 7) & 1;
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  SecIdent=%u  Preview=%u"), m_bySupportSecIdent, m_fSupportsPreview);
				break;
 			case ET_MOD_VERSION: 
				m_strNotOfficial.AppendFormat(_T(",mid=%s"),temptag.GetFullInfo()); //MOPRH - Added by SiRoB, Control Mod Tag
				if (temptag.IsStr())
				{
					m_strModVersion = temptag.GetStr();
					//MOPRH START - Added by SiRoB, Is Morph Client?
					m_bIsMorph = StrStrI(m_strModVersion,_T("Morph"))!=0;
					//MOPRH END   - Added by SiRoB, Is Morph Client?
				}
				else if (temptag.IsInt())
					m_strModVersion.Format(_T("ModID=%u"), temptag.GetInt());
				else
					m_strModVersion = _T("ModID=<Unknwon>");
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ModID=%s"), m_strModVersion);
				CheckForGPLEvilDoer();
				break;
			//Morph Start - added by AndCycle, ICS
			// enkeyDEV: ICS
			case ET_INCOMPLETEPARTS:
				//MOPRH START - Added by SiRoB,  Control Mod Tag
				m_strNotOfficial.AppendFormat(_T(",ics=%s"),temptag.GetFullInfo());
				if (!temptag.IsInt()){
					if (strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
						strBanReason.Format(_T("BadType eMuleInfo-Tag: ET_INCOMPLETEPARTS"));
					break;
				}
				//MOPRH END - Added by SiRoB,  Control Mod Tag
				m_incompletepartVer = temptag.GetInt();
				break;
			// <--- enkeyDEV: ICS
			//Morph End - added by AndCycle, ICS
			default:
				//<<< [SNAFU_V3] Check unknown tags !
				if (!((temptag.GetNameID() & 0xF0)==0xF0) || strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
					ProcessUnknownInfoTag(&temptag, strBanReason);
				//>>> [SNAFU_V3] Check unknown tags !
				m_strNotOfficial.AppendFormat(_T(",%s"),temptag.GetFullInfo()); //MOPRH - Added by SiRoB, Control Mod Tag
				if (bDbgInfo)
					m_strMuleInfo.AppendFormat(_T("\n  ***UnkTag=%s"), temptag.GetFullInfo());
		}
	}
	if( m_byDataCompVer == 0 ){
		m_bySourceExchangeVer = 0;
		m_byExtendedRequestsVer = 0;
		m_byAcceptCommentVer = 0;
		m_nUDPPort = 0;
		m_incompletepartVer = 0;	// enkeyDEV: ICS //Morph - added by AndCycle, ICS
	}
	if (/*bDbgInfo &&*/ data.GetPosition() < data.GetLength()){
		if (bDbgInfo)
		m_strMuleInfo.AppendFormat(_T("\n  ***AddData: %u bytes"), data.GetLength() - data.GetPosition());
		//MOPRH - Added by SiRoB, Control Mod Tag
		m_strNotOfficial.AppendFormat(_T(",extrabyte=%u"),data.GetPosition() < data.GetLength());
		if(strBanReason.IsEmpty() && thePrefs.GetEnableAntiLeecher())
			strBanReason=_T("extrabytes");
		//MOPRH - Added by SiRoB, Control Mod Tag
	}

	ReGetClientSoft();
	m_byInfopacketsReceived |= IP_EMULEPROTPACK;

	if (thePrefs.GetVerbose() && GetServerIP() == INADDR_NONE)
		AddDebugLogLine(false, _T("Received invalid server IP %s from %s"), ipstr(GetServerIP()), DbgGetClientInfo());

	//MORPH START - Added by SiRoB, Anti-leecher feature
	if(thePrefs.GetEnableAntiLeecher())
	{
		if (strBanReason.IsEmpty())
			strBanReason = TestLeecher();
		if (!strBanReason.IsEmpty())
			BanLeecher(strBanReason);
	}
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
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__HelloAnswer", this);
	theStats.AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true);
}

void CUpDownClient::SendHelloTypePacket(CSafeMemFile* data)
{
	data->WriteHash16(thePrefs.GetUserHash());
	uint32 clientid;
	clientid = theApp.GetID();

	data->WriteUInt32(clientid);
	data->WriteUInt16(thePrefs.GetPort());

	uint32 tagcount = 6;

	if( theApp.clientlist->GetBuddy() && theApp.IsFirewalled() )
		tagcount += 2;

	//MORPH START - Added by SiRoB, Don't send MOD_VERSION to client that don't support it to reduce overhead
	bool bSendModVersion = (m_strModVersion.GetLength() || m_pszUsername==NULL) && !IsLeecher();
	if (bSendModVersion) tagcount+=(1/*MOD_VERSION*/+1/*enkeyDev: ICS*/);
	//MORPH END   - Added by SiRoB, Don't send MOD_VERSION to client that don't support it to reduce overhead
	if (bSendModVersion || m_clientSoft == SO_LPHANT) tagcount+=(1/*WC_VOODOO*/+1/*WC_FLAGS*/); // MORPH - Modified by Commander, WebCache 1.2e

	data->WriteUInt32(tagcount);

	// eD2K Name

	// TODO implement multi language website which informs users of the effects of bad mods
	CTag tagName(CT_NAME, (!m_bGPLEvildoer) ? thePrefs.GetUserNick() : _T("Please use a GPL-conform version of eMule") );
	tagName.WriteTagToFile(data, utf8strRaw);

	// eD2K Version
	CTag tagVersion(CT_VERSION,EDONKEYVERSION);
	tagVersion.WriteTagToFile(data);

	// eMule UDP Ports
	uint32 kadUDPPort = 0;
	if(Kademlia::CKademlia::isConnected())
	{
		kadUDPPort = thePrefs.GetUDPPort();
	}
	CTag tagUdpPorts(CT_EMULE_UDPPORTS, 
				((uint32)kadUDPPort			   << 16) |
				((uint32)thePrefs.GetUDPPort() <<  0)
				); 
	tagUdpPorts.WriteTagToFile(data);

	if( theApp.clientlist->GetBuddy() && theApp.IsFirewalled() )
	{
		CTag tagBuddyIP(CT_EMULE_BUDDYIP, theApp.clientlist->GetBuddy()->GetBuddyIP() ); 
		tagBuddyIP.WriteTagToFile(data);
	
		CTag tagBuddyPort(CT_EMULE_BUDDYUDP, 
//					( RESERVED												)
					((uint32)theApp.clientlist->GetBuddy()->GetBuddyPort()  ) 
					);
		tagBuddyPort.WriteTagToFile(data);
	}

	// eMule Misc. Options #1
	const UINT uUdpVer				= 4;
	const UINT uDataCompVer			= 1;
	const UINT uSupportSecIdent		= theApp.clientcredits->CryptoAvailable() ? 3 : 0;
	const UINT uSourceExchangeVer	= 3;
	const UINT uExtendedRequestsVer	= 2;
	const UINT uAcceptCommentVer	= 1;
	const UINT uNoViewSharedFiles	= (thePrefs.CanSeeShares() == vsfaNobody) ? 1 : 0; // for backward compatibility this has to be a 'negative' flag
	const UINT uMultiPacket			= 1;
	const UINT uSupportPreview		= (thePrefs.CanSeeShares() != vsfaNobody) ? 1 : 0; // set 'Preview supported' only if 'View Shared Files' allowed
	const UINT uPeerCache			= 1;
	const UINT uUnicodeSupport		= 1;
	const UINT nAICHVer				= 1;
	CTag tagMisOptions1(CT_EMULE_MISCOPTIONS1, 
				(nAICHVer				<< 29) |
				(uUnicodeSupport		<< 28) |
				(uUdpVer				<< 24) |
				(uDataCompVer			<< 20) |
				(uSupportSecIdent		<< 16) |
				(uSourceExchangeVer		<< 12) |
				(uExtendedRequestsVer	<<  8) |
				(uAcceptCommentVer		<<  4) |
				(uPeerCache				<<  3) |
				(uNoViewSharedFiles		<<  2) |
				(uMultiPacket			<<  1) |
				(uSupportPreview		<<  0)
				);
	tagMisOptions1.WriteTagToFile(data);

	// eMule Misc. Options #2
	const UINT uKadVersion			= 1;
	CTag tagMisOptions2(CT_EMULE_MISCOPTIONS2, 
//				(RESERVED				     ) 
				(uKadVersion			<<  0) 
				);
	tagMisOptions2.WriteTagToFile(data);

	// eMule Version
	CTag tagMuleVersion(CT_EMULE_VERSION, 
				//(uCompatibleClientID	<< 24) |
				(CemuleApp::m_nVersionMjr	<< 17) |
				(CemuleApp::m_nVersionMin	<< 10) |
				(CemuleApp::m_nVersionUpd	<<  7) 
//				(RESERVED			     ) 
				);
	tagMuleVersion.WriteTagToFile(data);

	if (bSendModVersion) { //MORPH - Added by SiRoB, Don't send MOD_VERSION to client that don't support it to reduce overhead
		//MORPH - Added by SiRoB, ET_MOD_VERSION 0x55
		CTag tagMODVersion(ET_MOD_VERSION, theApp.m_strModVersion);
		tagMODVersion.WriteTagToFile(data);
		//MORPH - Added by SiRoB, ET_MOD_VERSION 0x55

		//Morph Start - added by AndCycle, ICS
		// enkeyDev: ICS
		CTag tagIncompleteParts(ET_INCOMPLETEPARTS,1);
		tagIncompleteParts.WriteTagToFile(data);
		// <--- enkeyDev: ICS
		//Morph End - added by AndCycle, ICS
	} //MORPH - Added by SiRoB, Don't send MOD_VERSION to client that don't support it to reduce overhead
	// MORPH START - Added by SiRoB, WebCache 1.2f
	if (bSendModVersion || m_clientSoft == SO_LPHANT)
	{
		CTag tagWebCacheVoodoo( WC_TAG_VOODOO, (uint32)'ARC4' );
		tagWebCacheVoodoo.WriteTagToFile(data);
		uint32 flags = WC_FLAGS_UDP | WC_FLAGS_NO_OHCBS | WC_FLAGS_MULTI_OHCBS;
		bool localMasterKeyNeeded = true;
		for(int i=0; localMasterKeyNeeded && i < WC_KEYLENGTH; i++)
		localMasterKeyNeeded = (Crypt.localMasterKey[i]==0);
		if (b_webcacheInfoNeeded || m_WA_webCacheIndex == -1 || localMasterKeyNeeded)
			flags |= WC_FLAGS_INFO_NEEDED;
		CTag tagWebCacheFlags( WC_TAG_FLAGS, flags);
		tagWebCacheFlags.WriteTagToFile(data);
	}
	// MORPH END   - Added by SiRoB, WebCache 1.2f
	uint32 dwIP;
	uint16 nPort;
	if (theApp.serverconnect->IsConnected()){
		dwIP = theApp.serverconnect->GetCurrentServer()->GetIP();
		nPort = theApp.serverconnect->GetCurrentServer()->GetPort();
#ifdef _DEBUG
		if (dwIP == theApp.serverconnect->GetLocalIP()){
			dwIP = 0;
			nPort = 0;
		}
#endif
	}
	else{
		nPort = 0;
		dwIP = 0;
	}
	data->WriteUInt32(dwIP);
	data->WriteUInt16(nPort);
//	data->WriteUInt32(dwIP); //The Hybrid added some bits here, what ARE THEY FOR?
}

void CUpDownClient::ProcessMuleCommentPacket(const uchar* pachPacket, uint32 nSize)
{
	if (reqfile && reqfile->IsPartFile())
	{
		CSafeMemFile data(pachPacket, nSize);
		uint8 uRating = data.ReadUInt8();
		if (thePrefs.GetLogRatingDescReceived() && uRating > 0)
			AddDebugLogLine(false, GetResString(IDS_RATINGRECV), m_strClientFilename, uRating);
		CString strComment;
		UINT uLength = data.ReadUInt32();
		if (uLength > 0)
		{
			// we have to increase the raw max. allowed file comment len because of possible UTF8 encoding.
			if (uLength > MAXFILECOMMENTLEN*3)
				uLength = MAXFILECOMMENTLEN*3;
			strComment = data.ReadString(GetUnicodeSupport()!=utf8strNone, uLength);
			if (thePrefs.GetLogRatingDescReceived() && !strComment.IsEmpty())
				AddDebugLogLine(false, GetResString(IDS_DESCRIPTIONRECV), m_strClientFilename, strComment);

			// test if comment is filtered
			if (!thePrefs.GetCommentFilter().IsEmpty())
			{
				CString strCommentLower(strComment);
				strCommentLower.MakeLower();

				int iPos = 0;
				CString strFilter(thePrefs.GetCommentFilter().Tokenize(_T("|"), iPos));
				while (!strFilter.IsEmpty())
				{
					// comment filters are already in lowercase, compare with temp. lowercased received comment
					if (strCommentLower.Find(strFilter) >= 0)
					{
						strComment.Empty();
						uRating = 0;
						break;
					}
					strFilter = thePrefs.GetCommentFilter().Tokenize(_T("|"), iPos);
				}
			}
		}
		if (!strComment.IsEmpty() || uRating > 0)
		{
			m_strFileComment = strComment;
			m_uFileRating = uRating;
			reqfile->UpdateFileRatingCommentAvail();
		}
	}
}

bool CUpDownClient::Disconnected(LPCTSTR pszReason, bool bFromSocket)
{
	ASSERT(theApp.clientlist->IsValidClient(this));

	//If this is a KAD client object, just delete it!
	SetKadState(KS_NONE);
	
	if (GetUploadState() == US_UPLOADING || GetUploadState() == US_CONNECTING || GetUploadState() == US_BANNED) //MORPH - Changed by SiRoB
	{
		if (thePrefs.GetLogUlDlEvents() && GetUploadState()==US_UPLOADING && m_fSentOutOfPartReqs==0 && !theApp.uploadqueue->IsOnUploadQueue(this))
			DebugLog(_T("Disconnected client removed from upload queue and waiting list: %s"), DbgGetClientInfo());
		theApp.uploadqueue->RemoveFromUploadQueue(this, pszReason);
	}

	// 28-Jun-2004 [bc]: re-applied this patch which was in 0.30b-0.30e. it does not seem to solve the bug but
	// it does not hurt either...
	if (m_BlockRequests_queue.GetCount() > 0 || m_DoneBlocks_list.GetCount()){
		// Although this should not happen, it seems(?) to happens sometimes. The problem we may run into here is as follows:
		//
		// 1.) If we do not clear the block send requests for that client, we will send those blocks next time the client
		// gets an upload slot. But because we are starting to send any available block send requests right _before_ the
		// remote client had a chance to prepare to deal with them, the first sent blocks will get dropped by the client.
		// Worst thing here is, because the blocks are zipped and can therefore only be uncompressed when the first block
		// was received, all of those sent blocks will create a lot of uncompress errors at the remote client.
		//
		// 2.) The remote client may have already received those blocks from some other client when it gets the next
		// upload slot.
        DebugLogWarning(_T("Disconnected client with non empty block send queue; %s reqs: %s doneblocks: %s"), DbgGetClientInfo(), m_BlockRequests_queue.GetCount() > 0 ? _T("true") : _T("false"), m_DoneBlocks_list.GetCount() ? _T("true") : _T("false"));
		ClearUploadBlockRequests();
	}

	if (GetDownloadState() == DS_DOWNLOADING){
		if (m_ePeerCacheDownState == PCDS_WAIT_CACHE_REPLY || m_ePeerCacheDownState == PCDS_DOWNLOADING)
			theApp.m_pPeerCache->DownloadAttemptFailed();
		SetDownloadState(DS_ONQUEUE, CString(_T("Disconnected: ")) + pszReason);
	}
	else{
		// ensure that all possible block requests are removed from the partfile
		ClearDownloadBlockRequests();
		if(GetDownloadState() == DS_CONNECTED){
		    //MORPH START - Added by SiRoB, Don't kill source if it's the only one complet source, it's a friend or a proxy
			if(reqfile && m_bCompleteSource && reqfile->m_nCompleteSourcesCountLo == 1  || IsFriend() || IsProxy())
				SetDownloadState(DS_ONQUEUE);
			else {
			//MORPH END   - Added by SiRoB, Don't kill source if it's the only one complet source or it's a friend
			    theApp.clientlist->m_globDeadSourceList.AddDeadSource(this);
				theApp.downloadqueue->RemoveSource(this);
		    }
	    }
	}

	// we had still an AICH request pending, handle it
	if (IsAICHReqPending()){
		m_fAICHRequested = FALSE;
		CAICHHashSet::ClientAICHRequestFailed(this);
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
			break;
	}
	switch(m_nDownloadState){
		case DS_ONQUEUE:
		case DS_TOOMANYCONNS:
		case DS_NONEEDEDPARTS:
		case DS_LOWTOLOWIP:
			bDelete = false;
	}

	switch(m_nUploadState){
		case US_CONNECTING:
			if (thePrefs.GetLogUlDlEvents())
	            AddDebugLogLine(DLP_VERYLOW, true,_T("---- %s: Removing connecting client from upload list. Reason: %s ----"), DbgGetClientInfo(), pszReason);
		case US_WAITCALLBACK:
			//MORPH START - Added by SiRoB, Don't kill client if we are the only one complet source or it's a friend or it's a proxy.
			if(reqfile && !reqfile->IsPartFile() && reqfile->m_nCompleteSourcesCountLo == 1  || IsFriend())
			{
				SetUploadState(US_NONE);
				bDelete = false;
				break;
			}
			//MORPH END   - Added by SiRoB, Don't kill client if we are the only one complet source or it's a friend or it's a proxy.
		case US_ERROR:
			theApp.clientlist->m_globDeadSourceList.AddDeadSource(this);
			bDelete = true;
	}
	switch(m_nDownloadState){
		case DS_CONNECTING:
		case DS_WAITCALLBACK:
			//MORPH START - Added by SiRoB, Don't kill source if it's the only one complet source, it's a friend or a proxy
			if(m_bCompleteSource && reqfile->m_nCompleteSourcesCountLo == 1 || IsFriend() || IsProxy() || !IsEd2kClient())
			{
				bDelete = true;
				break;
			}
			//MORPH END   - Added by SiRoB, Don't kill source if it's a proxy
		case DS_ERROR:
			theApp.clientlist->m_globDeadSourceList.AddDeadSource(this);
			bDelete = true;
	}

	if (GetChatState() != MS_NONE){
		bDelete = false;
		theApp.emuledlg->chatwnd->chatselector.ConnectingResult(this,false);
	}
	
	if (!bFromSocket && socket){
		ASSERT (theApp.listensocket->IsValidSocket(socket));
		socket->Safe_Delete();
	}
	socket = 0;

    if (m_iFileListRequested){
		LogWarning(LOG_STATUSBAR, GetResString(IDS_SHAREDFILES_FAILED), GetUserName());
        m_iFileListRequested = 0;
	}

	if (m_Friend)
		theApp.friendlist->RefreshFriend(m_Friend);

	theApp.emuledlg->transferwnd->clientlistctrl.RefreshClient(this);

	if (bDelete)
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			Debug(_T("--- Deleted client            %s; Reason=%s\n"), DbgGetClientInfo(true), pszReason);
		return true;
	}
	else
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			Debug(_T("--- Disconnected client       %s; Reason=%s\n"), DbgGetClientInfo(true), pszReason);
		m_fHashsetRequesting = 0;
		m_dwRequestedHashset = 0;	// SLUGFILLER: SafeHash
		SetSentCancelTransfer(0);
		m_bHelloAnswerPending = false;
		m_fQueueRankPending = 0;
		m_fFailedFileIdReqs = 0;
		m_fUnaskQueueRankRecv = 0;
		m_uPeerCacheDownloadPushId = 0;
		m_uPeerCacheUploadPushId = 0;
		m_uPeerCacheRemoteIP = 0;
		SetPeerCacheDownState(PCDS_NONE);
		SetPeerCacheUpState(PCUS_NONE);
		if (m_pPCDownSocket){
			m_pPCDownSocket->client = NULL;
			m_pPCDownSocket->Safe_Delete();
		}
		if (m_pPCUpSocket){
			m_pPCUpSocket->client = NULL;
			m_pPCUpSocket->Safe_Delete();
		}
		// MORPH START - Added by Commander, WebCache 1.2e
		SetWebCacheDownState(WCDS_NONE);
		SetWebCacheUpState(WCUS_NONE);
		if (m_pWCDownSocket){
			m_pWCDownSocket->client = NULL;
			m_pWCDownSocket->Safe_Delete();
		}
		if (m_pWCUpSocket){
			m_pWCUpSocket->client = NULL;
			m_pWCUpSocket->Safe_Delete();
		}
		// MORPH END - Added by Commander, WebCache 1.2e
		m_fSentOutOfPartReqs = 0;
		return false;
	}
}

//Returned bool is not if the TryToConnect is successful or not..
//false means the client was deleted!
//true means the client was not deleted!
bool CUpDownClient::TryToConnect(bool bIgnoreMaxCon, CRuntimeClass* pClassSocket)
{
	if (theApp.listensocket->TooManySockets() && !bIgnoreMaxCon && !(socket && socket->IsConnected()))
	{
		if(Disconnected(_T("Too many connections")))
		{
			delete this;
			return false;
		}
		return true;
	}

	uint32 uClientIP = GetIP();
	if (uClientIP == 0 && !HasLowID())
		uClientIP = ntohl(m_nUserIDHybrid);
	if (uClientIP)
	{
		// although we filter all received IPs (server sources, source exchange) and all incomming connection attempts,
		// we do have to filter outgoing connection attempts here too, because we may have updated the ip filter list
		if (theApp.ipfilter->IsFiltered(uClientIP))
		{
			theStats.filteredclients++;
			if (thePrefs.GetLogFilteredIPs())
				AddDebugLogLine(true, GetResString(IDS_IPFILTERED), ipstr(uClientIP), theApp.ipfilter->GetLastHit());
			if (Disconnected(_T("IPFilter")))
			{
				delete this;
				return false;
			}
			return true;
		}

		// for safety: check again whether that IP is banned
		if (theApp.clientlist->IsBannedClient(uClientIP))
		{
			if (thePrefs.GetLogBannedClients())
				AddDebugLogLine(false, _T("Refused to connect to banned client %s"), DbgGetClientInfo());
			if (Disconnected(_T("Banned IP")))
			{
				delete this;
				return false;
			}
			return true;
		}
	}

	if( GetKadState() == KS_QUEUED_FWCHECK )
		SetKadState(KS_CONNECTING_FWCHECK);

	if ( HasLowID() )
	{
		if(!theApp.DoCallback(this))
		{
			//We cannot do a callback!
			if (GetDownloadState() == DS_CONNECTING)
				SetDownloadState(DS_LOWTOLOWIP);
			else if (GetDownloadState() == DS_REQHASHSET)
			{
				SetDownloadState(DS_ONQUEUE);
				reqfile->hashsetneeded = true;
			}
			if (GetUploadState() == US_CONNECTING)
			{
				if(Disconnected(_T("LowID->LowID and US_CONNECTING")))
				{
					delete this;
					return false;
				}
			}
			return true;
		}

		//We already know we are not firewalled here as the above condition already detected LowID->LowID and returned.
		//If ANYTHING changes with the "if(!theApp.DoCallback(this))" above that will let you fall through 
		//with the condition that the source is firewalled and we are firewalled, we must
		//recheck it before the this check..
		if( HasValidBuddyID() && !GetBuddyIP() && !GetBuddyPort() && !theApp.serverconnect->IsLocalServer(GetServerIP(), GetServerPort()))
		{
			//This is a Kad firewalled source that we want to do a special callback because it has no buddyIP or buddyPort.
			if( Kademlia::CKademlia::isConnected() )
			{
				//We are connect to Kad
				if( Kademlia::CKademlia::getPrefs()->getTotalSource() > 0 || Kademlia::CSearchManager::alreadySearchingFor(Kademlia::CUInt128(GetBuddyID())))
				{
					//There are too many source lookups already or we are already searching this key.
					SetDownloadState(DS_TOOMANYCONNSKAD);
					return true;
				}
			}
		}
	}

	if (!socket || !socket->IsConnected())
	{
		if (socket)
			socket->Safe_Delete();
		if (pClassSocket == NULL)
			pClassSocket = RUNTIME_CLASS(CClientReqSocket);
		socket = static_cast<CClientReqSocket*>(pClassSocket->CreateObject());
		socket->SetClient(this);
		if (!socket->Create())
		{
			socket->Safe_Delete();
			return true;
		}
	}
	else
	{
		ConnectionEstablished();
		return true;
	}
	// MOD Note: Do not change this part - Merkur
	if (HasLowID())
	{
		if (GetDownloadState() == DS_CONNECTING)
			SetDownloadState(DS_WAITCALLBACK);
		if (GetUploadState() == US_CONNECTING)
		{
			if(Disconnected(_T("LowID and US_CONNECTING")))
			{
				delete this;
				return false;
			}
			return true;
		}

		if (theApp.serverconnect->IsLocalServer(m_dwServerIP,m_nServerPort))
		{
			Packet* packet = new Packet(OP_CALLBACKREQUEST,4);
			PokeUInt32(packet->pBuffer, m_nUserIDHybrid);
			if (thePrefs.GetDebugServerTCPLevel() > 0 || thePrefs.GetDebugClientTCPLevel() > 0)
				DebugSend("OP__CallbackRequest", this);
			theStats.AddUpDataOverheadServer(packet->size);
			theApp.serverconnect->SendPacket(packet);
			SetDownloadState(DS_WAITCALLBACK);
		}
		else
		{
			if ( GetUploadState() == US_NONE && (!GetRemoteQueueRank() || m_bReaskPending) )
			{
				if( !HasValidBuddyID() )
				{
					theApp.downloadqueue->RemoveSource(this);
					if(Disconnected(_T("LowID and US_NONE and QR=0")))
					{
						delete this;
						return false;
					}
					return true;
				}
				
				if( !Kademlia::CKademlia::isConnected() )
				{
					//We are not connected to Kad and this is a Kad Firewalled source..
					theApp.downloadqueue->RemoveSource(this);
					{
						if(Disconnected(_T("Kad Firewalled source but not connected to Kad.")))
						{
							delete this;
							return false;
						}
						return true;
					}
				}
                if( GetDownloadState() == DS_WAITCALLBACK )
				{
					if( GetBuddyIP() && GetBuddyPort())
					{
						CSafeMemFile bio(34);
						Kademlia::CUInt128 buddy(GetBuddyID());
						bio.WriteUInt128(&buddy);
						Kademlia::CUInt128 file(reqfile->GetFileHash());
						bio.WriteUInt128(&file);
						bio.WriteUInt16(thePrefs.GetPort());
						if (thePrefs.GetDebugClientKadUDPLevel() > 0 || thePrefs.GetDebugClientUDPLevel() > 0)
							DebugSend("KadCallbackReq", this);
						Packet* packet = new Packet(&bio, OP_KADEMLIAHEADER);
						packet->opcode = KADEMLIA_CALLBACK_REQ;
						theStats.AddUpDataOverheadKad(packet->size);
						theApp.clientudp->SendPacket(packet, GetBuddyIP(), GetBuddyPort());
						SetDownloadState(DS_WAITCALLBACKKAD);
					}
					else
					{
						//Create search to find buddy.
						Kademlia::CSearch *findSource = new Kademlia::CSearch;
						findSource->setSearchTypes(Kademlia::CSearch::FINDSOURCE);
						Kademlia::CUInt128 ID(GetBuddyID());
						findSource->setTargetID(ID);
						findSource->addFileID(Kademlia::CUInt128(reqfile->GetFileHash()));
						if(Kademlia::CSearchManager::startSearch(findSource))
						{
							//Started lookup..
							SetDownloadState(DS_WAITCALLBACKKAD);
						}
						else
						{
							//This should never happen..
							ASSERT(0);
						}
					}
				}
			}
			else
			{
				if (GetDownloadState() == DS_WAITCALLBACK)
				{
					m_bReaskPending = true;
					SetDownloadState(DS_ONQUEUE);
				}
			}
		}
	}
	// MOD Note - end
	else
	{
		if (!Connect())
			return false; // client was deleted!
	}
	return true;
}

bool CUpDownClient::Connect()
{
	//Try to always tell the socket to WaitForOnConnect before you call Connect.
	socket->WaitForOnConnect();
	SOCKADDR_IN sockAddr = {0};
	sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons(GetUserPort());
	sockAddr.sin_addr.S_un.S_addr = GetConnectIP();
	socket->Connect((SOCKADDR*)&sockAddr, sizeof sockAddr);
	if (!SendHelloPacket())
		return false; // client was deleted!
	return true;
}

void CUpDownClient::ConnectionEstablished()
{
	// ok we have a connection, lets see if we want anything from this client

	// check if we should use this client to retrieve our public IP
	if (theApp.GetPublicIP() == 0 && theApp.serverconnect->IsConnected() && m_fPeerCache)
		SendPublicIPRequest();

	switch(GetKadState())
	{
		case KS_CONNECTING_FWCHECK:
            SetKadState(KS_CONNECTED_FWCHECK);
			break;
		case KS_CONNECTING_BUDDY:
		case KS_INCOMING_BUDDY:
			SetKadState(KS_CONNECTED_BUDDY);
			break;
	}

	if (GetChatState() == MS_CONNECTING || GetChatState() == MS_CHATTING)
		theApp.emuledlg->chatwnd->chatselector.ConnectingResult(this,true);

	switch(GetDownloadState())
	{
		case DS_CONNECTING:
		case DS_WAITCALLBACK:
		case DS_WAITCALLBACKKAD:
			m_bReaskPending = false;
			SetDownloadState(DS_CONNECTED);
			SendFileRequest();
			break;
	}

	if (m_bReaskPending)
	{
		m_bReaskPending = false;
		if (GetDownloadState() != DS_NONE && GetDownloadState() != DS_DOWNLOADING)
		{
			SetDownloadState(DS_CONNECTED);
			SendFileRequest();
		}
	}

	switch(GetUploadState())
	{
		case US_CONNECTING:
		case US_WAITCALLBACK:
			if (theApp.uploadqueue->IsDownloading(this))
			{
				SetUploadState(US_UPLOADING);
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					DebugSend("OP__AcceptUploadReq", this);
				Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
				theStats.AddUpDataOverheadFileRequest(packet->size);
				socket->SendPacket(packet,true);
			}
	}

	if (m_iFileListRequested == 1)
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend(m_fSharedDirectories ? "OP__AskSharedDirs" : "OP__AskSharedFiles", this);
        Packet* packet = new Packet(m_fSharedDirectories ? OP_ASKSHAREDDIRS : OP_ASKSHAREDFILES,0);
		theStats.AddUpDataOverheadOther(packet->size);
		socket->SendPacket(packet,true,true);
	}

	while (!m_WaitingPackets_list.IsEmpty())
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("Buffered Packet", this);
		socket->SendPacket(m_WaitingPackets_list.RemoveHead());
	}

}

void CUpDownClient::ReGetClientSoft()
{
	if (m_pszUsername == NULL){
		m_clientSoft = SO_UNKNOWN;
		return;
	}

	int iHashType = GetHashType();
	if (iHashType == SO_EMULE){
		LPCTSTR pszSoftware;
		switch(m_byCompatibleClient){
			case SO_CDONKEY:
				m_clientSoft = SO_CDONKEY;
				pszSoftware = _T("cDonkey");
				break;
			case SO_XMULE:
				m_clientSoft = SO_XMULE;
				pszSoftware = _T("xMule");
				break;
			case SO_AMULE:
				m_clientSoft = SO_AMULE;
				pszSoftware = _T("aMule");
				break;
			case SO_SHAREAZA:
				m_clientSoft = SO_SHAREAZA;
				pszSoftware = _T("Shareaza");
				break;
			case SO_LPHANT:
				m_clientSoft = SO_LPHANT;
				pszSoftware = _T("lphant");
				break;
			default:
				if (m_bIsML || m_byCompatibleClient == SO_MLDONKEY){
					m_clientSoft = SO_MLDONKEY;
					pszSoftware = _T("MLdonkey");
				}
				else if (m_bIsHybrid){
					m_clientSoft = SO_EDONKEYHYBRID;
					pszSoftware = _T("eDonkeyHybrid");
				}
				else if (m_byCompatibleClient != 0){
					m_clientSoft = SO_XMULE; // means: 'eMule Compatible'
					pszSoftware = _T("eMule Compat");
				}
				else{
					m_clientSoft = SO_EMULE;
					pszSoftware = _T("eMule");
				}
		}

		int iLen;
		TCHAR szSoftware[128];
		if (m_byEmuleVersion == 0){
			m_nClientVersion = MAKE_CLIENT_VERSION(0, 0, 0);
			iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s"), pszSoftware);
		}
		else if (m_byEmuleVersion != 0x99){
			UINT nClientMinVersion = (m_byEmuleVersion >> 4)*10 + (m_byEmuleVersion & 0x0f);
			m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
			iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s v0.%u"), pszSoftware, nClientMinVersion);
		}
		else{
			UINT nClientMajVersion = (m_nClientVersion >> 17) & 0x7f;
			UINT nClientMinVersion = (m_nClientVersion >> 10) & 0x7f;
			UINT nClientUpVersion  = (m_nClientVersion >>  7) & 0x07;
			m_nClientVersion = MAKE_CLIENT_VERSION(nClientMajVersion, nClientMinVersion, nClientUpVersion);
			if (m_clientSoft == SO_EMULE)
				iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s v%u.%u%c"), pszSoftware, nClientMajVersion, nClientMinVersion, _T('a') + nClientUpVersion);
			else if (m_clientSoft == SO_AMULE || nClientUpVersion != 0)
				iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s v%u.%u.%u"), pszSoftware, nClientMajVersion, nClientMinVersion, nClientUpVersion);
			else
				iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("%s v%u.%u"), pszSoftware, nClientMajVersion, nClientMinVersion);
		}
		if (iLen > 0){
			memcpy(m_strClientSoftware.GetBuffer(iLen), szSoftware, iLen*sizeof(TCHAR));
			m_strClientSoftware.ReleaseBuffer(iLen);
			//MORPH START - Added by SiRoB, MODSTRING
			if(!m_strModVersion.IsEmpty())
				m_strClientSoftware.Append(_T(" [") + m_strModVersion + _T("]"));
			//MORPH END   - Added by SiRoB, MODSTRING
		}
		return;
	}

	if (m_bIsHybrid){
		m_clientSoft = SO_EDONKEYHYBRID;
		// seen:
		// 105010	0.50.10
		// 10501	0.50.1
		// 10103	1.1.3
		// 10102	1.1.2
		// 10100	1.1
		// 1051		0.51.0
		// 1002		1.0.2
		// 1000		1.0
		// 501		0.50.1

		UINT nClientMajVersion;
		UINT nClientMinVersion;
		UINT nClientUpVersion;
		if (m_nClientVersion > 100000){
			UINT uMaj = m_nClientVersion/100000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = (m_nClientVersion - uMaj*100000) / 100;
			nClientUpVersion = m_nClientVersion % 100;
		}
		else if (m_nClientVersion >= 10100 && m_nClientVersion <= 10109){
			nClientMajVersion = 1;
			nClientMinVersion = 1;
			nClientUpVersion = m_nClientVersion % 10;
		}
		else if (m_nClientVersion > 10000){
			UINT uMaj = m_nClientVersion/10000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = (m_nClientVersion - uMaj*10000) / 10;
			nClientUpVersion = m_nClientVersion % 10;
		}
		else if (m_nClientVersion >= 1000 && m_nClientVersion < 1020){
			UINT uMaj = m_nClientVersion/1000;
			nClientMajVersion = uMaj;
			nClientMinVersion = (m_nClientVersion - uMaj*1000) / 10;
			nClientUpVersion = m_nClientVersion % 10;
		}
		else if (m_nClientVersion > 1000){
			UINT uMaj = m_nClientVersion/1000;
			nClientMajVersion = uMaj - 1;
			nClientMinVersion = m_nClientVersion - uMaj*1000;
			nClientUpVersion = 0;
		}
		else if (m_nClientVersion > 100){
			UINT uMin = m_nClientVersion/10;
			nClientMajVersion = 0;
			nClientMinVersion = uMin;
			nClientUpVersion = m_nClientVersion - uMin*10;
		}
		else{
			nClientMajVersion = 0;
			nClientMinVersion = m_nClientVersion;
			nClientUpVersion = 0;
		}
		m_nClientVersion = MAKE_CLIENT_VERSION(nClientMajVersion, nClientMinVersion, nClientUpVersion);

		int iLen;
		TCHAR szSoftware[128];
		if (nClientUpVersion)
			iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("eDonkeyHybrid v%u.%u.%u"), nClientMajVersion, nClientMinVersion, nClientUpVersion);
		else
			iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("eDonkeyHybrid v%u.%u"), nClientMajVersion, nClientMinVersion);
		if (iLen > 0){
			memcpy(m_strClientSoftware.GetBuffer(iLen), szSoftware, iLen*sizeof(TCHAR));
			m_strClientSoftware.ReleaseBuffer(iLen);
			//MORPH START - Added by SiRoB, MODSTRING
			if(!m_strModVersion.IsEmpty())
				m_strClientSoftware.Append(_T(" [") + m_strModVersion + _T("]"));
			//MORPH END   - Added by SiRoB, MODSTRING
		}
		return;
	}

	if (m_bIsML || iHashType == SO_MLDONKEY){
		m_clientSoft = SO_MLDONKEY;
		UINT nClientMinVersion = m_nClientVersion;
		m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
		TCHAR szSoftware[128];
		int iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("MLdonkey v0.%u"), nClientMinVersion);
		if (iLen > 0){
			memcpy(m_strClientSoftware.GetBuffer(iLen), szSoftware, iLen*sizeof(TCHAR));
			m_strClientSoftware.ReleaseBuffer(iLen);
			//MORPH START - Added by SiRoB, MODSTRING
			if(!m_strModVersion.IsEmpty())
				m_strClientSoftware.Append(_T(" [") + m_strModVersion + _T("]"));
			//MORPH END   - Added by SiRoB, MODSTRING
		}
		return;
	}

	if (iHashType == SO_OLDEMULE){
		m_clientSoft = SO_OLDEMULE;
		UINT nClientMinVersion = m_nClientVersion;
		m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
		TCHAR szSoftware[128];
		int iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("Old eMule v0.%u"), nClientMinVersion);
		if (iLen > 0){
			memcpy(m_strClientSoftware.GetBuffer(iLen), szSoftware, iLen*sizeof(TCHAR));
			m_strClientSoftware.ReleaseBuffer(iLen);
			//MORPH START - Added by SiRoB, MODSTRING
			if(!m_strModVersion.IsEmpty())
				m_strClientSoftware.Append(_T(" [") + m_strModVersion + _T("]"));
			//MORPH END   - Added by SiRoB, MODSTRING
		}
		return;
	}

	m_clientSoft = SO_EDONKEY;
	UINT nClientMinVersion = m_nClientVersion;
	m_nClientVersion = MAKE_CLIENT_VERSION(0, nClientMinVersion, 0);
	TCHAR szSoftware[128];
	int iLen = _sntprintf(szSoftware, ARRSIZE(szSoftware), _T("eDonkey v0.%u"), nClientMinVersion);
	if (iLen > 0){
		memcpy(m_strClientSoftware.GetBuffer(iLen), szSoftware, iLen*sizeof(TCHAR));
		m_strClientSoftware.ReleaseBuffer(iLen);
		//MORPH START - Added by SiRoB, MODSTRING
		if(!m_strModVersion.IsEmpty())
			m_strClientSoftware.Append(_T(" [") + m_strModVersion + _T("]"));
		//MORPH END   - Added by SiRoB, MODSTRING
	}
}

int CUpDownClient::GetHashType() const
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

void CUpDownClient::SetUserName(LPCTSTR pszNewName)
{
	if (m_pszUsername){
		free(m_pszUsername);
		m_pszUsername = NULL;
	}
	if( pszNewName )
		m_pszUsername = _tcsdup(pszNewName);
	//MORPH START - Added by SiRoB, Anti-leecher feature
	if(thePrefs.GetEnableAntiLeecher())
	{
		LPCTSTR pszLeecherReason = TestLeecher();
		if (pszLeecherReason != NULL)
			BanLeecher(pszLeecherReason);
	}
	//MORPH END   - Added by SiRoB, Anti-leecher feature
	//MORPH START - Added by IceCream, xrmb Funnynick START
	UpdateFunnyNick();
	//MORPH END   - Added by IceCream, xrmb Funnynick END
}

void CUpDownClient::RequestSharedFileList()
{
	if (m_iFileListRequested == 0){
		AddLogLine(true,GetResString(IDS_SHAREDFILES_REQUEST),GetUserName());
    	m_iFileListRequested = 1;
		TryToConnect(true);
	}
	else{
		LogWarning(LOG_STATUSBAR, _T("Requesting shared files from user %s (%u) is already in progress"), GetUserName(), GetUserIDHybrid());
	}
}

void CUpDownClient::ProcessSharedFileList(const uchar* pachPacket, uint32 nSize, LPCTSTR pszDirectory)
{
	if (m_iFileListRequested > 0)
	{
        m_iFileListRequested--;
		theApp.searchlist->ProcessSearchAnswer(pachPacket,nSize,this,NULL,pszDirectory);
	}
}

void CUpDownClient::SetUserHash(const uchar* pucUserHash)
{
	if( pucUserHash == NULL ){
		md4clr(m_achUserHash);
		return;
	}
	md4cpy(m_achUserHash, pucUserHash);
}

void CUpDownClient::SetBuddyID(const uchar* pucBuddyID)
{
	if( pucBuddyID == NULL ){
		md4clr(m_achBuddyID);
		m_bBuddyIDValid = false;
		return;
	}
	m_bBuddyIDValid = true;
	md4cpy(m_achBuddyID, pucBuddyID);
}

void CUpDownClient::SendPublicKeyPacket()
{
	// send our public key to the client who requested it
	if (socket == NULL || credits == NULL || m_SecureIdentState != IS_KEYANDSIGNEEDED){
		ASSERT ( false );
		return;
	}
	if (!theApp.clientcredits->CryptoAvailable())
		return;

    Packet* packet = new Packet(OP_PUBLICKEY,theApp.clientcredits->GetPubKeyLen() + 1,OP_EMULEPROT);
	theStats.AddUpDataOverheadOther(packet->size);
	memcpy(packet->pBuffer+1,theApp.clientcredits->GetPublicKey(), theApp.clientcredits->GetPubKeyLen());
	packet->pBuffer[0] = theApp.clientcredits->GetPubKeyLen();
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__PublicKey", this);
	socket->SendPacket(packet,true,true);
	m_SecureIdentState = IS_SIGNATURENEEDED;
}

void CUpDownClient::SendSignaturePacket()
{
	// signate the public key of this client and send it
	if (socket == NULL || credits == NULL || m_SecureIdentState == 0){
		ASSERT ( false );
		return;
	}

	if (!theApp.clientcredits->CryptoAvailable())
		return;
	if (credits->GetSecIDKeyLen() == 0)
		return; // We don't have his public key yet, will be back here later
	// do we have a challenge value received (actually we should if we are in this function)
	if (credits->m_dwCryptRndChallengeFrom == 0){
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("Want to send signature but challenge value is invalid ('%s')"), GetUserName());
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
	theStats.AddUpDataOverheadOther(packet->size);
	memcpy(packet->pBuffer+1,achBuffer, siglen);
	packet->pBuffer[0] = siglen;
	if (bUseV2)
		packet->pBuffer[1+siglen] = byChaIPKind;
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__Signature", this);
	socket->SendPacket(packet,true,true);
	m_SecureIdentState = IS_ALLREQUESTSSEND;
}

void CUpDownClient::ProcessPublicKeyPacket(const uchar* pachPacket, uint32 nSize)
{
	theApp.clientlist->AddTrackClient(this);

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
		else if(m_SecureIdentState == IS_KEYANDSIGNEEDED)
		{
			// something is wrong
			if (thePrefs.GetLogSecureIdent())
				AddDebugLogLine(false, _T("Invalid State error: IS_KEYANDSIGNEEDED in ProcessPublicKeyPacket"));
		}
	}
	else
	{
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("Failed to use new received public key"));
	}
}

void CUpDownClient::ProcessSignaturePacket(const uchar* pachPacket, uint32 nSize)
{
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
	if (m_dwLastSignatureIP == GetIP())
	{
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("received multiple signatures from one client"));
		return;
	}
	
	// also make sure this client has a public key
	if (credits->GetSecIDKeyLen() == 0)
	{
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("received signature for client without public key"));
		return;
	}
	
	// and one more check: did we ask for a signature and sent a challange packet?
	if (credits->m_dwCryptRndChallengeFor == 0)
	{
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("received signature for client with invalid challenge value ('%s')"), GetUserName());
		return;
	}

	if (theApp.clientcredits->VerifyIdent(credits, pachPacket+1, pachPacket[0], GetIP(), byChaIPKind ) ){
		// result is saved in function abouve
		//if (thePrefs.GetLogSecureIdent())
		//	AddDebugLogLine(false, _T("'%s' has passed the secure identification, V2 State: %i"), GetUserName(), byChaIPKind);
	}
	else
	{
		if (thePrefs.GetLogSecureIdent())
			AddDebugLogLine(false, _T("'%s' has failed the secure identification, V2 State: %i"), GetUserName(), byChaIPKind);
	}
	m_dwLastSignatureIP = GetIP(); 
}

void CUpDownClient::SendSecIdentStatePacket()
{
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
			//if (thePrefs.GetLogSecureIdent())
			//	AddDebugLogLine(false, _T("Not sending SecIdentState Packet, because State is Zero"));
			return;
		}
		// crypt: send random data to sign
		uint32 dwRandom = rand()+1;
		credits->m_dwCryptRndChallengeFor = dwRandom;
		Packet* packet = new Packet(OP_SECIDENTSTATE,5,OP_EMULEPROT);
		theStats.AddUpDataOverheadOther(packet->size);
		packet->pBuffer[0] = nValue;
		PokeUInt32(packet->pBuffer+1, dwRandom);
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__SecIdentState", this);
		socket->SendPacket(packet,true,true);
	}
	else
		ASSERT ( false );
}

void CUpDownClient::ProcessSecIdentStatePacket(const uchar* pachPacket, uint32 nSize)
{
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
	credits->m_dwCryptRndChallengeFrom = PeekUInt32(pachPacket+1);
}

void CUpDownClient::InfoPacketsReceived()
{
	// indicates that both Information Packets has been received
	// needed for actions, which process data from both packets
	ASSERT ( m_byInfopacketsReceived == IP_BOTH );
	m_byInfopacketsReceived = IP_NONE;
	
	if (m_bySupportSecIdent){
		SendSecIdentStatePacket();
	}
}

void CUpDownClient::ResetFileStatusInfo()
{
	//MORPH START - Changed by SiRoB, Keep A4AF infos
	/*
	if (m_abyPartStatus){
		delete[] m_abyPartStatus;
		m_abyPartStatus = NULL;
	}
	*/
	m_abyPartStatus = NULL;
	//MORPH END   - Changed by SiRoB, Keep A4AF infos
	//Morph Start - added by AndCycle, ICS
	// enkeyDev: ICS
	//MORPH START - Added by AndCycle, ICS, Keep A4AF infos
	/*
	if (m_abyIncPartStatus){
		delete[] m_abyIncPartStatus;
		m_abyIncPartStatus = NULL;
	}
	*/
	//MORPH END - Added by AndCycle, ICS, Keep A4AF infos
	m_abyIncPartStatus = NULL;
	// <--- enkeyDev: ICS
	//Morph End - added by AndCycle, ICS
	m_iDifferenceQueueRank = 0;	//Morph - added by AndCycle, DiffQR
	m_nRemoteQueueRank = 0;
	m_nPartCount = 0;
	m_strClientFilename.Empty();
	m_bCompleteSource = false;
	m_uFileRating = 0;
	m_strFileComment.Empty();
	if (m_pReqFileAICHHash != NULL){
		delete m_pReqFileAICHHash;
		m_pReqFileAICHHash = NULL;
	}

	//MORPH START - Added by SiRoB, HotFix Due Complete Source Feature
	m_nUpCompleteSourcesCount = 0;
	//MORPH END   - Added by SiRoB, HotFix Due Complete Source Feature
}

bool CUpDownClient::IsBanned() const
{
	//MORPH START - Added by SiRoB, Code Optimization
	if (m_nUploadState == US_BANNED)
		return true;
	//MORPH END   - Added by SiRoB, Code Optimization
	return theApp.clientlist->IsBannedClient(GetIP());
}

void CUpDownClient::SendPreviewRequest(const CAbstractFile* pForFile)
{
	if (m_fPreviewReqPending == 0){
		m_fPreviewReqPending = 1;
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__RequestPreview", this, pForFile->GetFileHash());
		Packet* packet = new Packet(OP_REQUESTPREVIEW,16,OP_EMULEPROT);
		md4cpy(packet->pBuffer,pForFile->GetFileHash());
		theStats.AddUpDataOverheadOther(packet->size);
		SafeSendPacket(packet);
	}
	else{
		LogWarning(LOG_STATUSBAR, GetResString(IDS_ERR_PREVIEWALREADY));
	}
}

void CUpDownClient::SendPreviewAnswer(const CKnownFile* pForFile, CxImage** imgFrames, uint8 nCount)
{
	m_fPreviewAnsPending = 0;
	CSafeMemFile data(1024);
	if (pForFile){
		data.WriteHash16(pForFile->GetFileHash());
	}
	else{
		static const uchar _aucZeroHash[16] = {0};
		data.WriteHash16(_aucZeroHash);
	}
	data.WriteUInt8(nCount);
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
		data.WriteUInt32(nResultSize);
		data.Write(abyResultBuffer, nResultSize);
		free(abyResultBuffer);
	}
	Packet* packet = new Packet(&data, OP_EMULEPROT);
	packet->opcode = OP_PREVIEWANSWER;
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__PreviewAnswer", this, (uchar*)packet->pBuffer);
	theStats.AddUpDataOverheadOther(packet->size);
	SafeSendPacket(packet);
}

void CUpDownClient::ProcessPreviewReq(const uchar* pachPacket, uint32 nSize)
{
	if (nSize < 16)
		throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
	
	if (m_fPreviewAnsPending || thePrefs.CanSeeShares()==vsfaNobody || (thePrefs.CanSeeShares()==vsfaFriends && !IsFriend()))
		return;
	
	m_fPreviewAnsPending = 1;
	CKnownFile* previewFile = theApp.sharedfiles->GetFileByID(pachPacket);
	if (previewFile == NULL){
		SendPreviewAnswer(NULL, NULL, 0);
	}
	else{
		previewFile->GrabImage(4,0,true,450,this);
	}
}

void CUpDownClient::ProcessPreviewAnswer(const uchar* pachPacket, uint32 nSize)
{
	if (m_fPreviewReqPending == 0)
		return;
	m_fPreviewReqPending = 0;
	CSafeMemFile data(pachPacket, nSize);
	uchar Hash[16];
	data.ReadHash16(Hash);
	uint8 nCount = data.ReadUInt8();
	if (nCount == 0){
		LogError(LOG_STATUSBAR, GetResString(IDS_ERR_PREVIEWFAILED), GetUserName());
		return;
	}
	CSearchFile* sfile = theApp.searchlist->GetSearchFileByHash(Hash);
	if (sfile == NULL){
		//already deleted
		return;
	}

	BYTE* pBuffer = NULL;
	try{
	for (int i = 0; i != nCount; i++){
			uint32 nImgSize = data.ReadUInt32();
			if (nImgSize > nSize)
				throw CString(_T("CUpDownClient::ProcessPreviewAnswer - Provided image size exceeds limit"));
			pBuffer = new BYTE[nImgSize];
			data.Read(pBuffer, nImgSize);
			CxImage* image = new CxImage(pBuffer, nImgSize, CXIMAGE_FORMAT_PNG);
		delete[] pBuffer;
			pBuffer = NULL;
		if (image->IsValid()){
			sfile->AddPreviewImg(image);
		}
	}
	}
	catch(...){
		if (pBuffer != NULL)
			delete[] pBuffer;
		throw;
	}
	(new PreviewDlg())->SetFile(sfile);
}

// sends a packet, if needed it will establish a connection before
// options used: ignore max connections, control packet, delete packet
// !if the functions returns false that client object was deleted because the connection try failed and the object wasn't needed anymore.
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

bool CUpDownClient::HasLowID() const
{
	return IsLowID(m_nUserIDHybrid);
}

#ifdef _DEBUG
void CUpDownClient::AssertValid() const
{
	CObject::AssertValid();

	//SLAHAM: ADDED =>
	(void)uiStartDLCount;
	(void)dwStartDLTime;
	(void)dwSessionDLTime;
	(void)dwTotalDLTime;
	(void)uiDLAskingCounter;
	(void)dwThisClientIsKnownSince;
	//SLAHAM: ADDED <=

	CHECK_OBJ(socket);
	CHECK_PTR(credits);
	CHECK_PTR(m_Friend);
	CHECK_OBJ(reqfile);
	(void)compressiongain;
	(void)notcompressed;
	(void)m_abyUpPartStatus;
	m_OtherRequests_list.AssertValid();
	m_OtherNoNeeded_list.AssertValid();
	(void)m_lastPartAsked;
	(void)m_cMessagesReceived;
	(void)m_cMessagesSent;
	(void)m_dwUserIP;
	(void)m_dwServerIP;
	(void)m_nUserIDHybrid;
	(void)m_nUserPort;
	(void)m_nServerPort;
	(void)m_nClientVersion;
	(void)m_nUpDatarate;
	(void)m_byEmuleVersion;
	(void)m_byDataCompVer;
	CHECK_BOOL(m_bEmuleProtocol);
	CHECK_BOOL(m_bIsHybrid);
	(void)m_pszUsername;
	(void)m_achUserHash;
	(void)m_achBuddyID;
	(void)m_nBuddyIP;
	(void)m_nBuddyPort;
	(void)m_nUDPPort;
	(void)m_nKadPort;
	(void)m_byUDPVer;
	(void)m_bySourceExchangeVer;
	(void)m_byAcceptCommentVer;
	(void)m_byExtendedRequestsVer;
	CHECK_BOOL(m_bFriendSlot);
	CHECK_BOOL(m_bCommentDirty);
	CHECK_BOOL(m_bIsML);
	//ASSERT( m_clientSoft >= SO_EMULE && m_clientSoft <= SO_SHAREAZA || m_clientSoft == SO_MLDONKEY || m_clientSoft >= SO_EDONKEYHYBRID && m_clientSoft <= SO_UNKNOWN );
	(void)m_strClientSoftware;
	(void)m_dwLastSourceRequest;
	(void)m_dwLastSourceAnswer;
	(void)m_dwLastAskedForSources;
    (void)m_iFileListRequested;
	(void)m_byCompatibleClient;
	m_WaitingPackets_list.AssertValid();
	m_DontSwap_list.AssertValid();
	(void)m_lastRefreshedDLDisplay;
	ASSERT( m_SecureIdentState >= IS_UNAVAILABLE && m_SecureIdentState <= IS_KEYANDSIGNEEDED );
	(void)m_dwLastSignatureIP;
	ASSERT( (m_byInfopacketsReceived & ~IP_BOTH) == 0 );
	(void)m_bySupportSecIdent;
	(void)m_nTransferredUp;
	ASSERT( m_nUploadState >= US_UPLOADING && m_nUploadState <= US_NONE );
	(void)m_dwUploadTime;
	(void)m_cAsked;
	(void)m_dwLastUpRequest;
	(void)m_nCurSessionUp;
    (void)m_nCurQueueSessionPayloadUp;
    (void)m_addedPayloadQueueSession;
	(void)m_nUpPartCount;
	(void)m_nUpCompleteSourcesCount;
	(void)s_UpStatusBar;
	(void)requpfileid;
    (void)m_lastRefreshedULDisplay;
	m_AvarageUDR_list.AssertValid();
	m_BlockRequests_queue.AssertValid();
	m_DoneBlocks_list.AssertValid();
	m_RequestedFiles_list.AssertValid();
	ASSERT( m_nDownloadState >= DS_DOWNLOADING && m_nDownloadState <= DS_NONE );
	(void)m_cDownAsked;
	(void)m_abyPartStatus;
	(void)m_strClientFilename;
	(void)m_nTransferredDown;
	(void)m_dwDownStartTime;
	(void)m_nLastBlockOffset;
	(void)m_nDownDatarate;
	(void)m_nDownDataRateMS;
	(void)m_nSumForAvgDownDataRate;
	(void)m_cShowDR;
	(void)m_nRemoteQueueRank;
	(void)m_dwLastBlockReceived;
	(void)m_nPartCount;
	ASSERT( m_nSourceFrom >= SF_SERVER && m_nSourceFrom <= SF_SLS );
	CHECK_BOOL(m_bRemoteQueueFull);
	CHECK_BOOL(m_bCompleteSource);
	CHECK_BOOL(m_bReaskPending);
	CHECK_BOOL(m_bUDPPending);
	CHECK_BOOL(m_bTransferredDownMini);
	CHECK_BOOL(m_bUnicodeSupport);
	ASSERT( m_nKadState >= KS_NONE && m_nKadState <= KS_CONNECTED_BUDDY );
	m_AvarageDDR_list.AssertValid();
	(void)m_nSumForAvgUpDataRate;
	m_PendingBlocks_list.AssertValid();
	m_DownloadBlocks_list.AssertValid();
	(void)s_StatusBar;
	ASSERT( m_nChatstate >= MS_NONE && m_nChatstate <= MS_UNABLETOCONNECT );
	(void)m_strFileComment;
	(void)m_uFileRating;
#undef CHECK_PTR
#undef CHECK_BOOL
}
#endif

#ifdef _DEBUG
void CUpDownClient::Dump(CDumpContext& dc) const
{
	CObject::Dump(dc);
}
#endif

LPCTSTR CUpDownClient::DbgGetDownloadState() const
{
	const static LPCTSTR apszState[] =
	{
		_T("Downloading"),
		_T("OnQueue"),
		_T("Connected"),
		_T("Connecting"),
		_T("WaitCallback"),
		_T("WaitCallbackKad"),
		_T("ReqHashSet"),
		_T("NoNeededParts"),
		_T("TooManyConns"),
		_T("TooManyConnsKad"),
		_T("LowToLowIp"),
		_T("Banned"),
		_T("Error"),
		_T("None"),
		_T("RemoteQueueFull")
	};
	if (GetDownloadState() >= ARRSIZE(apszState))
		return _T("*Unknown*");
	return apszState[GetDownloadState()];
}

LPCTSTR CUpDownClient::DbgGetUploadState() const
{
	const static LPCTSTR apszState[] =
	{
		_T("Uploading"),
		_T("OnUploadQueue"),
		_T("WaitCallback"),
		_T("Connecting"),
		_T("Pending"),
		_T("LowToLowIp"),
		_T("Banned"),
		_T("Error"),
		_T("None")
	};
	if (GetUploadState() >= ARRSIZE(apszState))
		return _T("*Unknown*");
	return apszState[GetUploadState()];
}

LPCTSTR CUpDownClient::DbgGetKadState() const
{
	const static LPCTSTR apszState[] =
	{
		_T("None"),
		_T("FwCheckQueued"),
		_T("FwCheckConnecting"),
		_T("FwCheckConnected"),
		_T("BuddyQueued"),
		_T("BuddyIncoming"),
		_T("BuddyConnecting"),
		_T("BuddyConnected")
	};
	if (GetKadState() >= ARRSIZE(apszState))
		return _T("*Unknown*");
	return apszState[GetKadState()];
}

CString CUpDownClient::DbgGetFullClientSoftVer() const
{
	//MORPH START - Changed by SiRoB, [itsonlyme: -modname-]
	/*if (GetClientModVer().IsEmpty())
		return GetClientSoftVer();
	return GetClientSoftVer() + _T(" [") + GetClientModVer() + _T(']');
	*/
	return GetClientSoftVer();
	//MORPH END   - Changed by SiRoB, [itsonlyme: -modname-]
}

CString CUpDownClient::DbgGetClientInfo(bool bFormatIP) const
{
	CString str;
	if (this != NULL)
	{
		try{
			if (HasLowID())
			{
				if (GetConnectIP())
				{
					str.Format(_T("%u@%s (%s) '%s' (%s,%s/%s/%s)"),
						GetUserIDHybrid(), ipstr(GetServerIP()),
						ipstr(GetConnectIP()),
						GetUserName(),
						DbgGetFullClientSoftVer(),
						DbgGetDownloadState(), DbgGetUploadState(), DbgGetKadState());
				}
				else
				{
					str.Format(_T("%u@%s '%s' (%s,%s/%s/%s)"),
						GetUserIDHybrid(), ipstr(GetServerIP()),
						GetUserName(),
						DbgGetFullClientSoftVer(),
						DbgGetDownloadState(), DbgGetUploadState(), DbgGetKadState());
				}
			}
			else
			{
				str.Format(bFormatIP ? _T("%-15s '%s' (%s,%s/%s/%s)") : _T("%s '%s' (%s,%s/%s/%s)"),
					ipstr(GetConnectIP()),
					GetUserName(),
					DbgGetFullClientSoftVer(),
					DbgGetDownloadState(), DbgGetUploadState(), DbgGetKadState());
			}
		}
		catch(...){
			str.Format(_T("%08x - Invalid client instance"), this);
		}
	}
	return str;
}

bool CUpDownClient::CheckHandshakeFinished(UINT protocol, UINT opcode) const
{
	if (m_bHelloAnswerPending)
	{
		// 24-Nov-2004 [bc]: The reason for this is that 2 clients are connecting to each other at the same..
		//if (thePrefs.GetVerbose())
		//	AddDebugLogLine(DLP_VERYLOW, false, _T("Handshake not finished - while processing packet: %s; %s"), DbgGetClientTCPOpcode(protocol, opcode), DbgGetClientInfo());
		return false;
	}

	return true;
}

void CUpDownClient::CheckForGPLEvilDoer()
{
	if (!m_strModVersion.IsEmpty()){
		LPCTSTR pszModVersion = (LPCTSTR)m_strModVersion;

		// skip leading spaces
		while (*pszModVersion == _T(' '))
			pszModVersion++;

		// check for known major gpl breaker
		if (_tcsnicmp(pszModVersion, _T("LH"), 2)==0 ||
			_tcsnicmp(pszModVersion, _T("LIO"), 3)==0 ||
			_tcsnicmp(pszModVersion, _T("PLUS PLUS"), 9)==0 ||
			_tcsnicmp(pszModVersion, _T("WAREZFAW.COM 2.0"),16)==0)
			m_bGPLEvildoer = true;
	}
}

void CUpDownClient::OnSocketConnected(int nErrorCode)
{
}

CString CUpDownClient::GetDownloadStateDisplayString() const
{
	CString strState;
	switch (GetDownloadState())
	{
		case DS_CONNECTING:
			strState = GetResString(IDS_CONNECTING);
			break;
		case DS_CONNECTED:
			strState = GetResString(IDS_ASKING);
			break;
		case DS_WAITCALLBACK:
			strState = GetResString(IDS_CONNVIASERVER);
			break;
		case DS_ONQUEUE:
			if (IsRemoteQueueFull())
				strState = GetResString(IDS_QUEUEFULL);
			else
			// EastShare START - Modified by TAHO, moved and moddified from Priority column
			//strState = GetResString(IDS_ONQUEUE);
			{
				if ( GetRemoteQueueRank()){
					//Morph - modified by AndCycle, DiffQR
					strState.Format(_T("QR: %u (%i)"), GetRemoteQueueRank(), GetDiffQR());
					//Morph - modified by AndCycle, DiffQR
				}
				else{
				strState = GetResString(IDS_ONQUEUE);
				}
			}
			// EastShare END - Modified by TAHO, moved and moddified from Priority column
			break;
		case DS_DOWNLOADING:
			strState = GetResString(IDS_TRANSFERRING);
			break;
		case DS_REQHASHSET:
			strState = GetResString(IDS_RECHASHSET);
			break;
		case DS_NONEEDEDPARTS:
			strState = GetResString(IDS_NONEEDEDPARTS);
			break;
		case DS_LOWTOLOWIP:
			strState = GetResString(IDS_NOCONNECTLOW2LOW);
			break;
		case DS_TOOMANYCONNS:
			strState = GetResString(IDS_TOOMANYCONNS);
			break;
		case DS_ERROR:
			strState = GetResString(IDS_ERROR);
			break;
		case DS_WAITCALLBACKKAD:
			strState = GetResString(IDS_KAD_WAITCBK);
			break;
		case DS_TOOMANYCONNSKAD:
			strState = GetResString(IDS_KAD_TOOMANDYKADLKPS);
			break;
	}
/*
	if (thePrefs.GetPeerCacheShow())
	{
*/		switch (m_ePeerCacheDownState)
		{
		case PCDS_WAIT_CLIENT_REPLY:
			strState = _T("Peer")+GetResString(IDS_PCDS_CLIENTWAIT);
			break;
		case PCDS_WAIT_CACHE_REPLY:
			strState = _T("Peer")+GetResString(IDS_PCDS_CACHEWAIT);
			break;
		case PCDS_DOWNLOADING:
			strState = _T("Peer")+GetResString(IDS_CACHE);
			break;
		}
		if (m_ePeerCacheDownState != PCDS_NONE && m_bPeerCacheDownHit)
			strState = _T("Peer Hit");
/*
	}
*/
	// MORPH START - Added by Commander, WebCache 1.2e
	switch (m_eWebCacheDownState)
	{
	case WCDS_WAIT_CLIENT_REPLY:
		strState = _T("ProxyWait");
		break;
	case WCDS_WAIT_CACHE_REPLY:
		strState = _T("WC-Bug:CacheWait"); // not needed...
		break;
	case WCDS_DOWNLOADINGVIA:
		strState = _T("Via Proxy");
		break;
	case WCDS_DOWNLOADINGFROM:
		strState = _T("From Proxy");
		break;
	}
	// MORPH END - Added by Commander, WebCache 1.2e
	return strState;
}

CString CUpDownClient::GetUploadStateDisplayString() const
{
	CString strState;
	switch (GetUploadState()){
		case US_ONUPLOADQUEUE:
			strState = GetResString(IDS_ONQUEUE);
			break;
		case US_PENDING:
			strState = GetResString(IDS_CL_PENDING);
			break;
		case US_LOWTOLOWIP:
			strState = GetResString(IDS_CL_LOW2LOW);
			break;
		case US_BANNED:
			strState = GetResString(IDS_BANNED);
			break;
		case US_ERROR:
			strState = GetResString(IDS_ERROR);
			break;
		case US_CONNECTING:
			strState = GetResString(IDS_CONNECTING);
			break;
		case US_WAITCALLBACK:
			strState = GetResString(IDS_CONNVIASERVER);
			break;
		case US_UPLOADING:
            if(IsScheduledForRemoval()) {
				strState = GetScheduledRemovalDisplayReason();
            } else if(GetSlotNumber() <= theApp.uploadqueue->GetActiveUploadsCount()) {
				strState = GetResString(IDS_TRANSFERRING);
            } else {
                strState = GetResString(IDS_TRICKLING);
            }
            //CString strStateTemp = strState;
            //strState.Format(_T("%i: %s"), GetSlotNumber(), strStateTemp);
			break;
	}
/*
	if (thePrefs.GetPeerCacheShow())
	{
*/		switch (m_ePeerCacheUpState)
		{
		case PCUS_WAIT_CACHE_REPLY:
			strState = _T("PeerCacheWait");
			break;
		case PCUS_UPLOADING:
			strState = _T("PeerCache");
			break;
		}
		if (m_ePeerCacheUpState != PCUS_NONE && m_bPeerCacheUpHit)
			strState += _T(" Hit");
/*
	}
*/
	// MORPH START - Added by Commander, WebCache 1.2e
	if( m_eWebCacheUpState == WCUS_UPLOADING )
		strState = _T("Via Proxy");
	// MORPH START - Added by Commander, WebCache 1.2e
	return strState;
}

void CUpDownClient::SendPublicIPRequest(){
	if (socket && socket->IsConnected()){
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__PublicIPReq", this);
		Packet* packet = new Packet(OP_PUBLICIP_REQ,0,OP_EMULEPROT);
		theStats.AddUpDataOverheadOther(packet->size);
		socket->SendPacket(packet,true);
		m_fNeedOurPublicIP = 1;
	}
}

void CUpDownClient::ProcessPublicIPAnswer(const BYTE* pbyData, UINT uSize){
	if (uSize != 4)
		throw GetResString(IDS_ERR_WRONGPACKAGESIZE);
	uint32 dwIP = PeekUInt32(pbyData);
	if (m_fNeedOurPublicIP == 1){ // did we?
		m_fNeedOurPublicIP = 0;
		if (theApp.GetPublicIP() == 0 && !::IsLowID(dwIP) )
			theApp.SetPublicIP(dwIP);
	}	
}

void CUpDownClient::CheckFailedFileIdReqs(const uchar* aucFileHash)
{
	if ( aucFileHash != NULL && (theApp.sharedfiles->IsUnsharedFile(aucFileHash) || theApp.downloadqueue->GetFileByID(aucFileHash)) )
		return;
	//if (GetDownloadState() != DS_DOWNLOADING) // filereq floods are never allowed!
	{
		if (m_fFailedFileIdReqs < 6)// NOTE: Do not increase this nr. without increasing the bits for 'm_fFailedFileIdReqs'
			m_fFailedFileIdReqs++;
		if (m_fFailedFileIdReqs == 6)
		{
			if (theApp.clientlist->GetBadRequests(this) < 2)
				theApp.clientlist->TrackBadRequest(this, 1);
			if (theApp.clientlist->GetBadRequests(this) == 2){
				theApp.clientlist->TrackBadRequest(this, -2); // reset so the client will not be rebanned right after the ban is lifted
				Ban(_T("FileReq flood"));
			}
			throw CString(thePrefs.GetLogBannedClients() ? _T("FileReq flood") : _T(""));
		}
	}
}

EUtf8Str CUpDownClient::GetUnicodeSupport() const
{
	if (m_bUnicodeSupport)
		return utf8strRaw;
	return utf8strNone;
}
//MORPH START - Added by SiRoB, ZZUL_20040904
void CUpDownClient::SetFriendSlot(bool bNV)		
{
    bool oldValue = m_bFriendSlot;
    m_bFriendSlot = bNV;
    if(theApp.uploadqueue && oldValue != m_bFriendSlot)
        theApp.uploadqueue->ReSortUploadSlots(true);
}
//MORPH END   - Added by SiRoB, ZZUL_20040904
//MORPH START - Added by SiRoB, Show Requested Files
void CUpDownClient::ShowRequestedFiles()
{
	CString fileList;
	fileList += GetResString(IDS_LISTREQDL);
	fileList += "\n--------------------------\n" ; 
	if ( reqfile  && reqfile->IsPartFile())
	{
		fileList += reqfile->GetFileName(); 
		for(POSITION pos = m_OtherRequests_list.GetHeadPosition();pos!=0;m_OtherRequests_list.GetNext(pos))
		{
			fileList += "\n" ; 
			fileList += m_OtherRequests_list.GetAt(pos)->GetFileName(); 
		}
		for(POSITION pos = m_OtherNoNeeded_list.GetHeadPosition();pos!=0;m_OtherNoNeeded_list.GetNext(pos))
		{
			fileList += "\n" ;
			fileList += m_OtherNoNeeded_list.GetAt(pos)->GetFileName();
		}
	}
	else
		fileList += GetResString(IDS_LISTREQNODL);
	fileList += "\n\n\n";
	fileList += GetResString(IDS_LISTREQUL);
	fileList += "\n------------------------\n" ; 
	CKnownFile* uploadfile = theApp.sharedfiles->GetFileByID((uchar*)requpfileid);
	if(uploadfile)
		fileList += uploadfile->GetFileName();
	else
		fileList += GetResString(IDS_LISTREQNOUL);
	AfxMessageBox(fileList,MB_OK);
}
//MORPH END   - Added by SiRoB, Show Requested Files

//EastShare Start - added by AndCycle, IP to Country
// Superlexx - client's location
CString	CUpDownClient::GetCountryName(bool longName) const {
	return theApp.ip2country->GetCountryNameFromRef(m_structUserCountry,longName);
}

int CUpDownClient::GetCountryFlagIndex() const {
	return m_structUserCountry->FlagIndex;
}
//MORPH START - Changed by SiRoB, ProxyClient
void CUpDownClient::ResetIP2Country(uint32 m_dwIP){
	m_structUserCountry = theApp.ip2country->GetCountryFromIP((m_dwIP)?m_dwIP:m_dwUserIP);
}
//MORPH END - Changed by SiRoB, ProxyClient
//EastShare End - added by AndCycle, IP to Country
//<<< eWombat [SNAFU_V3]
void CUpDownClient::ProcessUnknownHelloTag(CTag *tag, CString &pszReason)
{
LPCTSTR strSnafuTag=NULL;
switch(tag->GetNameID())
	{
	case CT_UNKNOWNx12:
	case CT_UNKNOWNx13:
	case CT_UNKNOWNx14:
	case CT_UNKNOWNx16:
	case CT_UNKNOWNx17:
	case CT_UNKNOWNxE6:			strSnafuTag=apszSnafuTag[0];break;//buffer=_T("DodgeBoards");break;
	case CT_UNKNOWNx15:			strSnafuTag=apszSnafuTag[1];break;//buffer=_T("DodgeBoards & DarkMule |eVorte|X|");break;
	case CT_UNKNOWNx22:			strSnafuTag=apszSnafuTag[2];break;//buffer=_T("DarkMule v6 |eVorte|X|");break;
	case CT_UNKNOWNx5D:
	case CT_UNKNOWNx6B:
	case CT_UNKNOWNx6C:			strSnafuTag=apszSnafuTag[17];break;
	case CT_UNKNOWNx74:
	case CT_UNKNOWNx87:			strSnafuTag=apszSnafuTag[17];break;
	case CT_UNKNOWNxF0:
	case CT_UNKNOWNxF4:			strSnafuTag=apszSnafuTag[17];break;
	//case CT_UNKNOWNx69:			strSnafuTag=apszSnafuTag[3];break;//buffer=_T("eMuleReactor");break;
	case CT_UNKNOWNx79:			strSnafuTag=apszSnafuTag[4];break;//buffer=_T("Bionic");break;
	case CT_UNKNOWNx83:			strSnafuTag=apszSnafuTag[15];break;//buffer=_T("Fusspi");break;
	case CT_UNKNOWNx76:			
	case CT_UNKNOWNxCD:			strSnafuTag=apszSnafuTag[16];break;//buffer=_T("www.donkey2002.to");break;
	case CT_UNKNOWNx88:
		////If its a LSD its o.k
		if (m_strModVersion.IsEmpty() || _tcsnicmp(m_strModVersion, _T("LSD"),3)!=0)
			strSnafuTag=apszSnafuTag[5];//[LSD7c]
		break;
	case CT_UNKNOWNx8c:			strSnafuTag=apszSnafuTag[5];break;//buffer=_T("[LSD7c]");break; 
	case CT_UNKNOWNx8d:			strSnafuTag=apszSnafuTag[6];break;//buffer=_T("[0x8d] unknown Leecher - (client version:60)");break;
	case CT_UNKNOWNx99:			strSnafuTag=apszSnafuTag[7];break;//buffer=_T("[RAMMSTEIN]");break;		//STRIKE BACK
	case CT_UNKNOWNx98:
	case CT_UNKNOWNx9C:
	case CT_UNKNOWNxDA:			strSnafuTag=apszSnafuTag[3];break;//buffer=_T("eMuleReactor");break;
	case CT_UNKNOWNxc4:			strSnafuTag=apszSnafuTag[8];break;//buffer=_T("[MD5 Community]");break;	//USED BY NEW BIONIC => 0x12 Sender
	case CT_FRIENDSHARING:		//STRIKE BACK
		//if (theApp.glob_prefs->GetAntiFriendshare())
		//	{
			if (tag->IsInt() && tag->GetInt() == FRIENDSHARING_ID) //Mit dieser ID Definitiv
				{
					BanLeecher(_T("Friend Sharing detected"));
					return;				
				}
		//	}
		break;
	case CT_DARK:				//STRIKE BACK
	case CT_UNKNOWNx7A:
	case CT_UNKNOWNxCA:
			strSnafuTag=apszSnafuTag[9];break;//buffer=_T("new DarkMule");
		break;
	}
	if (strSnafuTag!=NULL)
	{
		pszReason.Format(_T("Suspect Hello-Tag: %s"),strSnafuTag);
	}
}
void CUpDownClient::ProcessUnknownInfoTag(CTag *tag, CString &pszReason)
{
LPCTSTR strSnafuTag=NULL;
switch(tag->GetNameID())
	{
	case ET_MOD_UNKNOWNx12:
	case ET_MOD_UNKNOWNx13:
	case ET_MOD_UNKNOWNx14:
	case ET_MOD_UNKNOWNx17:		strSnafuTag=apszSnafuTag[0];break;//("[DodgeBoards]")
	case ET_MOD_UNKNOWNx2F:		strSnafuTag=apszSnafuTag[10];break;//buffer=_T("[OMEGA v.07 Heiko]");break;
	case ET_MOD_UNKNOWNx36:
	case ET_MOD_UNKNOWNx5B:
	case ET_MOD_UNKNOWNxA6:		strSnafuTag=apszSnafuTag[11];break;//buffer=_T("eMule v0.26 Leecher");break;
	case ET_MOD_UNKNOWNx60:		strSnafuTag=apszSnafuTag[12];break;//buffer=_T("[Hunter]");break; //STRIKE BACK
	case ET_MOD_UNKNOWNx76:		strSnafuTag=apszSnafuTag[0];break;//buffer=_T("[DodgeBoards]");break;
	case ET_MOD_UNKNOWNx50:		
	case ET_MOD_UNKNOWNxB1:		
	case ET_MOD_UNKNOWNxB4:		
	case ET_MOD_UNKNOWNxC8:		
	case ET_MOD_UNKNOWNxC9:		strSnafuTag=apszSnafuTag[13];break;//buffer=_T("[Bionic 0.20 Beta]");break;
	case ET_MOD_UNKNOWNxDA:		strSnafuTag=apszSnafuTag[14];break;//buffer=_T("[Rumata (rus)(Plus v1f)]");break;
	}
	if (strSnafuTag!=NULL)
	{
		pszReason.Format(_T("Suspect eMuleInfo-Tag: %s"), strSnafuTag);
	}
}
//>>> eWombat [SNAFU_V3]

//MORPH - Added by SiRoB, most of the code from xrmb FunnyNick
void CUpDownClient::UpdateFunnyNick()
{
	if(m_pszUsername == NULL || 
		_tcsnicmp(m_pszUsername, _T("http://emule"),12) < 0 &&
		_tcsnicmp(m_pszUsername, _T("http://www.emule"),16) < 0 &&
		_tcsnicmp(m_pszUsername, _T("www.emule"),9) < 0 &&
		_tcsnicmp(m_pszUsername, _T("www.shareaza"),12) < 0 &&
		_tcsnicmp(m_pszUsername, _T("eMule v"),7) < 0 &&
		_tcsnicmp(m_pszUsername, _T("eMule Plus"),10) < 0 &&
		_tcsnicmp(m_pszUsername, _T("eMule OX"),8) < 0 &&
		_tcsnicmp(m_pszUsername, _T("eMule Plus"),10) < 0 &&
		_tcsnicmp(m_pszUsername, _T("eMule0"),6) < 0 &&
		_tcsicmp(m_pszUsername, _T("")) < 0)
		return;
	// preffix table
const static LPCTSTR apszPreFix[] =
	{
	_T("ATX-"),			//0
	_T("Gameboy "),
	_T("PS/2-"),
	_T("USB-"),
	_T("Angry "),
	_T("Atrocious "),
	_T("Attractive "),
	_T("Bad "),
	_T("Barbarious "),
	_T("Beautiful "),
	_T("Black "),		//10
	_T("Blond "),
	_T("Blue "),
	_T("Bright "),
	_T("Brown "),
	_T("Cool "),
	_T("Cruel "),
	_T("Cubic "),
	_T("Cute "),
	_T("Dance "),
	_T("Dark "),		//20
	_T("Dinky "),
	_T("Drunk "),
	_T("Dumb "),
	_T("E"),
	_T("Electro "),
	_T("Elite "),
	_T("Fast "),
	_T("Flying "),
	_T("Fourios "),
	_T("Frustraded "),	//30
	_T("Funny "),
	_T("Furious "),
	_T("Giant "),
	_T("Giga "),
	_T("Green "),
	_T("Handsome "),
	_T("Hard "),
	_T("Harsh "),
	_T("Hiphop "),
	_T("Holy "),		//40
	_T("Horny "),
	_T("Hot "),
	_T("House "),
	_T("I"),
	_T("Lame "),
	_T("Leaking "),
	_T("Lone "),
	_T("Lovely "),
	_T("Lucky "),
	_T("Micro "),		//50
	_T("Mighty "),
	_T("Mini "),
	_T("Nice "),
	_T("Orange "),
	_T("Pretty "),
	_T("Red "),
	_T("Sexy "),
	_T("Slow "),
	_T("Smooth "),
	_T("Stinky "),		//60
	_T("Strong "),
	_T("Super "),
	_T("Unholy "),
	_T("White "),
	_T("Wild "),
	_T("X"),
	_T("XBox "),
	_T("Yellow "),
	_T("Kentucky Fried "),
	_T("Mc"),			//70
	_T("Alien "),
	_T("Bavarian "),
	_T("Crazy "),
	_T("Death "),
	_T("Drunken "),
	_T("Fat "),
	_T("Hazardous "),
	_T("Holy "),
	_T("Infested "),
	_T("Insane "),		//80
	_T("Mutated "),
	_T("Nasty "),
	_T("Purple "),
	_T("Radioactive "),
	_T("Ugly "),
	_T("Green "),		//86
	};
#define NB_PREFIX 87 
#define MAX_PREFIXSIZE 15

// suffix table
const static LPCTSTR apszSuffix[] =
	{
	_T("16"),		//0
	_T("3"),
	_T("6"),
	_T("7"),
	_T("Abe"),
	_T("Bee"),
	_T("Bird"),
	_T("Boy"),
	_T("Cat"),
	_T("Cow"),
	_T("Crow"),		//10
	_T("DJ"),
	_T("Dad"),
	_T("Deer"),
	_T("Dog"),
	_T("Donkey"),
	_T("Duck"),
	_T("Eagle"),
	_T("Elephant"),
	_T("Fly"),
	_T("Fox"),		//20
	_T("Frog"),
	_T("Girl"),
	_T("Girlie"),
	_T("Guinea Pig"),
	_T("Hasi"),
	_T("Hawk"),
	_T("Jackal"),
	_T("Lizard"),
	_T("MC"),
	_T("Men"),		//30
	_T("Mom"),
	_T("Mouse"),
	_T("Mule"),
	_T("Pig"),
	_T("Rabbit"),
	_T("Rat"),
	_T("Rhino"),
	_T("Smurf"),
	_T("Snail"),
	_T("Snake"),	//40
	_T("Star"),
	_T("Tiger"),
	_T("Wolf"),
	_T("Butterfly"),
	_T("Elk"),
	_T("Godzilla"),
	_T("Horse"),
	_T("Penguin"),
	_T("Pony"), 
	_T("Reindeer"),	//50
	_T("Sheep"),
	_T("Sock Puppet"),
	_T("Worm"),
	_T("Bermuda")	//54
	};
#define NB_SUFFIX 56 
#define MAX_SUFFIXSIZE 11

	//--- if we get an id, we can generate the same random name for this user over and over... so much about randomness :) ---
	if(m_achUserHash)
	{
		uint32	x=0x7d726d62; // < xrmb :)
		uint8	a=m_achUserHash[5]  ^ m_achUserHash[7]  ^ m_achUserHash[15] ^ m_achUserHash[4];
		uint8	b=m_achUserHash[11] ^ m_achUserHash[9]  ^ m_achUserHash[12] ^ m_achUserHash[1];
		uint8	c=m_achUserHash[3]  ^ m_achUserHash[14] ^ m_achUserHash[6]  ^ m_achUserHash[13];
		uint8	d=m_achUserHash[2]  ^ m_achUserHash[0]  ^ m_achUserHash[10] ^ m_achUserHash[8];
		uint32	e=(a<<24) + (b<<16) + (c<<8) + d;
		srand(e^x);
	}

	if (m_pszFunnyNick) {
		delete[] m_pszFunnyNick;
		m_pszFunnyNick = NULL;
	}
	// pick random suffix and prefix
	m_pszFunnyNick = new TCHAR[13+MAX_PREFIXSIZE+MAX_SUFFIXSIZE];
	_tcscpy(m_pszFunnyNick, _T("[FunnyNick] "));
	_tcscat(m_pszFunnyNick, apszPreFix[rand()%NB_PREFIX]);
	_tcscat(m_pszFunnyNick, apszSuffix[rand()%NB_SUFFIX]);

	//--- make the rand random again ---
	if(m_achUserHash)
		srand((unsigned)time(NULL));
}
//MORPH END  - Added by SiRoB, most of the code from xrmb FunnyNick