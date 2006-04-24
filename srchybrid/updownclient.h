//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "BarShader.h"
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country

// MORPH START - Added by Commander, WebCache 1.2e
#include "Preferences.h"
#include "WebCache/WebCache.h"
#include "WebCache/WebCacheCryptography.h"
#include "WebCache/WebCacheMFRList.h"

// USING_NAMESPACE(CryptoPP)

enum EWebCacheDownState{
	WCDS_NONE = 0,
	WCDS_WAIT_CLIENT_REPLY,
	WCDS_WAIT_CACHE_REPLY,
	WCDS_DOWNLOADINGVIA,
	WCDS_DOWNLOADINGFROM
};

enum EWebCacheUpState{
	WCUS_NONE = 0,
	WCUS_UPLOADING
};


class CWebCacheDownSocket;
class CWebCacheUpSocket;
// MORPH END - Added by Commander, WebCache 1.2e

#include "Opcodes.h"
class CTag; //Added by SiRoB, SNAFU
class CClientReqSocket;
class CPeerCacheDownSocket;
class CPeerCacheUpSocket;
class CFriend;
class CPartFile;
class CClientCredits;
class CAbstractFile;
class CKnownFile;
class Packet;
class CxImage;
struct Requested_Block_Struct;
class CSafeMemFile;
class CEMSocket;
class CAICHHash;
enum EUtf8Str;

struct Pending_Block_Struct{
	Pending_Block_Struct()
	{
		block = NULL;
		zStream = NULL;
		totalUnzipped = 0;
		fZStreamError = 0;
		fRecovered = 0;
		fQueued = 0;
	}
	Requested_Block_Struct*	block;
	struct z_stream_s*      zStream;       // Barry - Used to unzip packets
	UINT					totalUnzipped; // Barry - This holds the total unzipped bytes for all packets so far
	UINT					fZStreamError : 1,
							fRecovered    : 1,
							fQueued		  : 3;
};

#pragma pack(1)
struct Requested_File_Struct{
	uchar	  fileid[16];
	uint32	  lastasked;
	uint8	  badrequests;
};
#pragma pack()

enum EUploadState{
	US_UPLOADING,
	US_ONUPLOADQUEUE,
	US_WAITCALLBACK,
	US_CONNECTING,
	US_PENDING,
	US_LOWTOLOWIP,
	US_BANNED,
	US_ERROR,
	US_NONE
};

enum EDownloadState{
	DS_DOWNLOADING,
	DS_ONQUEUE,
	DS_CONNECTED,
	DS_CONNECTING,
	DS_WAITCALLBACK,
	DS_WAITCALLBACKKAD,
	DS_REQHASHSET,
	DS_NONEEDEDPARTS,
	DS_TOOMANYCONNS,
	DS_TOOMANYCONNSKAD,
	DS_LOWTOLOWIP,
	DS_BANNED,
	DS_ERROR,
	DS_NONE,
	DS_REMOTEQUEUEFULL  // not used yet, except in statistics
};

enum EPeerCacheDownState{
	PCDS_NONE = 0,
	PCDS_WAIT_CLIENT_REPLY,
	PCDS_WAIT_CACHE_REPLY,
	PCDS_DOWNLOADING
};

enum EPeerCacheUpState{
	PCUS_NONE = 0,
	PCUS_WAIT_CACHE_REPLY,
	PCUS_UPLOADING
};

enum EChatState{
	MS_NONE,
	MS_CHATTING,
	MS_CONNECTING,
	MS_UNABLETOCONNECT
};

enum EKadState{
	KS_NONE,
	KS_QUEUED_FWCHECK,
	KS_CONNECTING_FWCHECK,
	KS_CONNECTED_FWCHECK,
	KS_QUEUED_BUDDY,
	KS_INCOMING_BUDDY,
	KS_CONNECTING_BUDDY,
	KS_CONNECTED_BUDDY
};

enum EClientSoftware{
	SO_EMULE			= 0,	// default
	SO_CDONKEY			= 1,	// ET_COMPATIBLECLIENT
	SO_XMULE			= 2,	// ET_COMPATIBLECLIENT
	SO_AMULE			= 3,	// ET_COMPATIBLECLIENT
	SO_SHAREAZA			= 4,	// ET_COMPATIBLECLIENT
	SO_MLDONKEY			= 10,	// ET_COMPATIBLECLIENT
	SO_LPHANT			= 20,	// ET_COMPATIBLECLIENT
	// other client types which are not identified with ET_COMPATIBLECLIENT
	SO_EDONKEYHYBRID	= 50,
	SO_EDONKEY,
	SO_OLDEMULE,
	SO_URL,
	SO_WEBCACHE, // Superlexx - webcache - statistics // MORPH - Added by Commander, WebCache 1.2e
	SO_UNKNOWN
};

enum ESecureIdentState{
	IS_UNAVAILABLE		= 0,
	IS_ALLREQUESTSSEND  = 0,
	IS_SIGNATURENEEDED	= 1,
	IS_KEYANDSIGNEEDED	= 2,
};

enum EInfoPacketState{
	IP_NONE				= 0,
	IP_EDONKEYPROTPACK  = 1,
	IP_EMULEPROTPACK	= 2,
	IP_BOTH				= 3,
};

enum EHelloPacketState{
	HP_NONE				= 0,
	HP_HELLO			= 1,
	HP_HELLOANSWER		= 2,
	HP_BOTH				= 3,
};

enum ESourceFrom{
	SF_SERVER			= 0,
	SF_KADEMLIA			= 1,
	SF_SOURCE_EXCHANGE	= 2,
	SF_PASSIVE			= 3,
	SF_LINK				= 4,
	SF_SLS				= 5 //MORPH - Added by SiRoB, Save Load Sources (SLS)
};
//MORPH START - Added by SiRoB, See chunk that we hide
enum EChunkStatus{
	SC_AVAILABLE		= 1,
	SC_HIDDENBYSOTN		= 2,
	SC_HIDDENBYHIDEOS	= 4,
	SC_PARTIAL			= 8 //MORPH - Added By SiRoB, ICS merged into partstatus
};
//MORPH END   - Added by SiRoB, See chunk that we hide

#ifdef _DEBUG
	// use the 'Enums' only for debug builds, each enum costs 4 bytes (3 unused)
#define _EClientSoftware	EClientSoftware
#define _EChatState			EChatState
#define _EKadState			EKadState
#define _ESecureIdentState	ESecureIdentState
#define _EUploadState		EUploadState
#define _EDownloadState		EDownloadState
#define _ESourceFrom		ESourceFrom
#else
#define _EClientSoftware	uint8
#define _EChatState			uint8
#define _EKadState			uint8
#define _ESecureIdentState	uint8
#define _EUploadState		uint8
#define _EDownloadState		uint8
#define _ESourceFrom		uint8
#endif

struct PartFileStamp {
	CPartFile*	file;
	DWORD		timestamp;
};

#define	MAKE_CLIENT_VERSION(mjr, min, upd) \
	((UINT)(mjr)*100U*10U*100U + (UINT)(min)*100U*10U + (UINT)(upd)*100U)

//#pragma pack(2)
class CUpDownClient : public CObject
{
	DECLARE_DYNAMIC(CUpDownClient)

	friend class CUploadQueue;
public:
    void PrintUploadStatus();

	//base
	CUpDownClient(CClientReqSocket* sender = 0);
	CUpDownClient(CPartFile* in_reqfile, uint16 in_port, uint32 in_userid, uint32 in_serverup, uint16 in_serverport, bool ed2kID = false);
	virtual ~CUpDownClient();

	void StartDownload();
	virtual void CheckDownloadTimeout();
	virtual void SendCancelTransfer(Packet* packet = NULL);
	virtual bool	IsEd2kClient() const { return true; }
	virtual bool	Disconnected(LPCTSTR pszReason, bool bFromSocket = false);
	virtual bool	TryToConnect(bool bIgnoreMaxCon = false, CRuntimeClass* pClassSocket = NULL, bool* filtered = NULL);
	virtual bool	Connect();
	virtual void	ConnectionEstablished();
	virtual void	OnSocketConnected(int nErrorCode);
	bool			CheckHandshakeFinished(UINT protocol = 0, UINT opcode = 0) const;
	void			CheckFailedFileIdReqs(const uchar* aucFileHash);
	uint32			GetUserIDHybrid() const							{ return m_nUserIDHybrid; }
	void			SetUserIDHybrid(uint32 val)						{ m_nUserIDHybrid = val; }
	/*FunnyNick*/LPCTSTR	GetUserName() const								{ return (thePrefs.DisplayFunnyNick() && m_pszFunnyNick)?m_pszFunnyNick:m_pszUsername; }
	/*FunnyNick*/void	UpdateFunnyNick();
	void			SetUserName(LPCTSTR pszNewName);
	uint32			GetIP() const									{ return m_dwUserIP; }
	void			SetIP( uint32 val ) //Only use this when you know the real IP or when your clearing it.
					{
						m_dwUserIP = val;
						m_nConnectIP = val;
					}
	__inline bool	HasLowID() const								{ return (m_nUserIDHybrid < 16777216); }
	uint32			GetConnectIP() const				{return m_nConnectIP;}
	uint16			GetUserPort() const								{ return m_nUserPort; }
	void			SetUserPort(uint16 val)							{ m_nUserPort = val; }
	UINT			GetTransferredUp() const						{ return m_nTransferredUp; }
	UINT			GetTransferredDown() const						{ return m_nTransferredDown; }
	uint32			GetServerIP() const								{ return m_dwServerIP; }
	void			SetServerIP(uint32 nIP)							{ m_dwServerIP = nIP; }
	uint16			GetServerPort() const							{ return m_nServerPort; }
	void			SetServerPort(uint16 nPort)						{ m_nServerPort = nPort; }
	const uchar*	GetUserHash() const								{ return (uchar*)m_achUserHash; }
	void			SetUserHash(const uchar* pUserHash);
	bool			HasValidHash() const
					{
						return ((const int*)m_achUserHash[0]) != 0 || ((const int*)m_achUserHash[1]) != 0 || ((const int*)m_achUserHash[2]) != 0 || ((const int*)m_achUserHash[3]) != 0; 
					}
	int				GetHashType() const;
	const uchar*	GetBuddyID() const								{ return (uchar*)m_achBuddyID; }
	void			SetBuddyID(const uchar* m_achTempBuddyID);
	bool			HasValidBuddyID() const							{ return m_bBuddyIDValid; }
	void			SetBuddyIP( uint32 val )						{ m_nBuddyIP = val; }
	uint32			GetBuddyIP() const								{ return m_nBuddyIP; }
	void			SetBuddyPort( uint16 val )						{ m_nBuddyPort = val; }
	uint16			GetBuddyPort() const							{ return m_nBuddyPort; }
	EClientSoftware	GetClientSoft() const							{ return (EClientSoftware)m_clientSoft; }
	const CString&	GetClientSoftVer() const						{ return m_strClientSoftware; }
	const CString&	GetClientModVer() const							{ return m_strModVersion; }
	const CString&	GetClientModTag() const							{ return m_strNotOfficial; }
	void			InitClientSoftwareVersion();
	UINT			GetVersion() const								{ return m_nClientVersion; }
	uint8			GetMuleVersion() const							{ return m_byEmuleVersion; }
	bool			ExtProtocolAvailable() const					{ return m_bEmuleProtocol; }
	bool			SupportMultiPacket() const						{ return m_bMultiPacket; }
	bool			SupportExtMultiPacket() const					{ return m_fExtMultiPacket; }
	bool			SupportPeerCache() const { return m_fPeerCache; }
	bool			SupportsLargeFiles() const						{ return m_fSupportsLargeFiles; }
	bool			IsEmuleClient() const							{ return m_byEmuleVersion!=0; }
	uint8			GetSourceExchangeVersion() const				{ return m_bySourceExchangeVer; }
	CClientCredits* Credits() const									{ return credits; }
	bool			IsBanned() const;
	const CString&	GetClientFilename() const						{ return m_strClientFilename; }
	void			SetClientFilename(const CString& fileName)		{ m_strClientFilename = fileName; }
	uint16			GetUDPPort() const								{ return m_nUDPPort; }
	void			SetUDPPort(uint16 nPort)						{ m_nUDPPort = nPort; }
	uint8			GetUDPVersion() const							{ return m_byUDPVer; }
	bool			SupportsUDP() const								{ return GetUDPVersion() != 0 && m_nUDPPort != 0; }
	uint16			GetKadPort() const								{ return m_nKadPort; }
	void			SetKadPort(uint16 nPort)						{ m_nKadPort = nPort; }
	uint8			GetExtendedRequestsVersion() const				{ return m_byExtendedRequestsVer; }
	void			RequestSharedFileList();
	void			ProcessSharedFileList(const uchar* pachPacket, UINT nSize, LPCTSTR pszDirectory = NULL);

	void			ClearHelloProperties();
	bool			ProcessHelloAnswer(const uchar* pachPacket, UINT nSize);
	bool			ProcessHelloPacket(const uchar* pachPacket, UINT nSize);
	void			SendHelloAnswer();
	virtual bool	SendHelloPacket();
	void			SendMuleInfoPacket(bool bAnswer);
	void			ProcessMuleInfoPacket(const uchar* pachPacket, UINT nSize);
	void			ProcessMuleCommentPacket(const uchar* pachPacket, UINT nSize);
	//<<< eWombat [SNAFU_V3]
	//MORPH - Changed by SiRoB
	void			ProcessUnknownHelloTag(CTag *tag, CString &pszReason);
	void			ProcessUnknownInfoTag(CTag *tag, CString &pszReason);
	//>>> eWombat [SNAFU_V3]
	void			ProcessEmuleQueueRank(const uchar* packet, UINT size);
	void			ProcessEdonkeyQueueRank(const uchar* packet, UINT size);
	void			CheckQueueRankFlood();
	bool			Compare(const CUpDownClient* tocomp, bool bIgnoreUserhash = false) const;
	void			ResetFileStatusInfo();
	uint32			GetLastSrcReqTime() const						{ return m_dwLastSourceRequest; }
	void			SetLastSrcReqTime()								{ m_dwLastSourceRequest = ::GetTickCount(); }
	uint32			GetLastSrcAnswerTime() const					{ return m_dwLastSourceAnswer; }
	void			SetLastSrcAnswerTime()							{ m_dwLastSourceAnswer = ::GetTickCount(); }
	uint32			GetLastAskedForSources() const					{ return m_dwLastAskedForSources; }
	void			SetLastAskedForSources()						{ m_dwLastAskedForSources = ::GetTickCount(); }
	bool			GetFriendSlot() const;
	//MORPH START - Changed by SiRoB, Friend Slot addon
	/*
	void			SetFriendSlot(bool bNV)							{ m_bFriendSlot = bNV; }
	*/
	void			SetFriendSlot(bool bNV);
	//MORPH END   - Changed by SiRoB, Friend Slot addon
	bool			IsFriend() const								{ return m_Friend != NULL; }
	void			SetCommentDirty(bool bDirty = true)				{ m_bCommentDirty = bDirty; }
	bool			GetSentCancelTransfer() const					{ return m_fSentCancelTransfer; }
	void			SetSentCancelTransfer(bool bVal)				{ m_fSentCancelTransfer = bVal; }
	void			ProcessPublicIPAnswer(const BYTE* pbyData, UINT uSize);
	void			SendPublicIPRequest();
	uint8			GetKadVersion()									{ return m_byKadVersion; }
	bool			SendBuddyPingPong()								{ return m_dwLastBuddyPingPongTime < ::GetTickCount(); }
	bool			AllowIncomeingBuddyPingPong()					{ return m_dwLastBuddyPingPongTime < (::GetTickCount()-(3*60*1000)); }
	void			SetLastBuddyPingPongTime()						{ m_dwLastBuddyPingPongTime = (::GetTickCount()+(10*60*1000)); }

	//MORPH START - Added by IceCream, Anti-leecher feature
	bool			IsLeecher()	const				{return m_bLeecher;}
	//MORPH END   - Added by IceCream, Anti-leecher feature
	float			GetCompression() const	{return (float)compressiongain/notcompressed*100.0f;} // Add rod show compression
	void			ResetCompressionGain() {compressiongain = 0; notcompressed=1;} // Add show compression
	uint32			GetAskTime()	{return AskTime;} //MORPH - Added by SiRoB - Smart Upload Control v2 (SUC) [lovelace]

	// secure ident
	void			SendPublicKeyPacket();
	void			SendSignaturePacket();
	void			ProcessPublicKeyPacket(const uchar* pachPacket, UINT nSize);
	void			ProcessSignaturePacket(const uchar* pachPacket, UINT nSize);
	uint8			GetSecureIdentState() const						{ return (uint8)m_SecureIdentState; }
	void			SendSecIdentStatePacket();
	void			ProcessSecIdentStatePacket(const uchar* pachPacket, UINT nSize);
	uint8			GetInfoPacketsReceived() const					{ return m_byInfopacketsReceived; }
	void			InfoPacketsReceived();
	// preview
	void			SendPreviewRequest(const CAbstractFile* pForFile);
	void			SendPreviewAnswer(const CKnownFile* pForFile, CxImage** imgFrames, uint8 nCount);
	void			ProcessPreviewReq(const uchar* pachPacket, UINT nSize);
	void			ProcessPreviewAnswer(const uchar* pachPacket, UINT nSize);
	bool			GetPreviewSupport() const						{ return m_fSupportsPreview && GetViewSharedFilesSupport(); }
	bool			GetViewSharedFilesSupport() const				{ return m_fNoViewSharedFiles==0; }
	bool			SafeSendPacket(Packet* packet);
	void			CheckForGPLEvilDoer();
	//upload
	EUploadState	GetUploadState() const							{ return (EUploadState)m_nUploadState; }
	void			SetUploadState(EUploadState news);
	//EastShare START - Modified by TAHO, modified SUQWT
	/*
	uint32			GetWaitStartTime() const;
	*/
	sint64			GetWaitStartTime() const;
	//EastShare END - Modified by TAHO, modified SUQWT
	void 			SetWaitStartTime();
	void 			ClearWaitStartTime();
	uint32			GetWaitTime() const								{ return (uint32)(m_dwUploadTime - GetWaitStartTime()); }
	bool			IsDownloading() const							{ return (m_nUploadState == US_UPLOADING); }
	bool			HasBlocks() const								{ return !m_BlockRequests_queue.IsEmpty(); }
	UINT			GetDatarate() const								{ return m_nUpDatarate; }	
	UINT			GetScore(bool sysvalue, bool isdownloading = false, bool onlybasevalue = false) const;
	void			AddReqBlock(Requested_Block_Struct* reqblock);
	void			CreateNextBlockPackage();
	uint32			GetUpStartTimeDelay() const						{ return ::GetTickCount() - m_dwUploadTime; }
	void 			SetUpStartTime()								{ m_dwUploadTime = ::GetTickCount(); }
	void			SendHashsetPacket(const uchar* fileid);
	const uchar*	GetUploadFileID() const							{ return requpfileid; }
	void			SetUploadFileID(CKnownFile* newreqfile);
	//MORPH START - Added by SiRoB, Optimization requpfile
	CKnownFile*		CheckAndGetReqUpFile() const;
	//MORPH END   - Added by SiRoB, Optimization requpfile
	UINT			SendBlockData();
	void			ClearUploadBlockRequests();
	void			SendRankingInfo();
	void			SendCommentInfo(/*const*/ CKnownFile *file);
	void			AddRequestCount(const uchar* fileid);
	void			UnBan();
	void			Ban(LPCTSTR pszReason = NULL);
	void			BanLeecher(LPCTSTR pszReason = NULL); //MORPH - Added by IceCream, Anti-leecher feature
	UINT			GetAskedCount() const							{ return m_cAsked; }
	void			AddAskedCount()									{ m_cAsked++; }
	void			SetAskedCount(UINT m_cInAsked)					{ m_cAsked = m_cInAsked; }
	void			FlushSendBlocks(); // call this when you stop upload, or the socket might be not able to send
	uint32			GetLastUpRequest() const						{ return m_dwLastUpRequest; }
	void			SetLastUpRequest()								{ m_dwLastUpRequest = ::GetTickCount(); }
	void			SetCollectionUploadSlot(bool bValue);
	bool			HasCollectionUploadSlot() const					{ return m_bCollectionUploadSlot; }

	UINT			GetSessionUp() const							{ return m_nTransferredUp - m_nCurSessionUp; }
	void			ResetSessionUp(){
						m_nCurSessionUp = m_nTransferredUp;
						m_addedPayloadQueueSession = GetQueueSessionPayloadUp(); 
						//m_nCurQueueSessionPayloadUp = 0;
					}

	uint32			GetQueueSessionUp() const
                    {
                        return m_nTransferredUp - m_nCurQueueSessionUp;
                    }

	void			ResetQueueSessionUp()
                    {
                        m_nCurQueueSessionUp = m_nTransferredUp;
						m_nCurQueueSessionPayloadUp = 0;
                        m_curSessionAmountNumber = 0;
					} 

	UINT			GetSessionDown() const							{ return m_nTransferredDown - m_nCurSessionDown; }
	void			ResetSessionDown() {
						m_nCurSessionDown = m_nTransferredDown;
					}
	UINT			GetQueueSessionPayloadUp() const				{ return m_nCurQueueSessionPayloadUp; }
    UINT			GetPayloadInBuffer() const						{ return m_addedPayloadQueueSession - GetQueueSessionPayloadUp(); }

    uint64          GetCurrentSessionLimit() const
                    {
                        return (uint64)SESSIONMAXTRANS*(m_curSessionAmountNumber+1)+1*1024;
                    }

	bool			ProcessExtendedInfo(CSafeMemFile* packet, CKnownFile* tempreqfile);
	uint16			GetUpPartCount() const							{ return m_nUpPartCount; }
	void			DrawUpStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat) const;
	bool			IsUpPartAvailable(UINT iPart) const {
						//MORPH - Changed by SiRoB, See chunk that we hide
						/*
						return (iPart>=m_nUpPartCount || !m_abyUpPartStatus) ? false : m_abyUpPartStatus[iPart]!=0;
						*/
						return (iPart>=m_nUpPartCount || !m_abyUpPartStatus) ? false : m_abyUpPartStatus[iPart]&SC_AVAILABLE;

					}
	uint8*			GetUpPartStatus() const							{ return m_abyUpPartStatus; }
    /*
	float           GetCombinedFilePrioAndCredit();
	*/
	double			GetCombinedFilePrioAndCredit();
	
	//MORPH START - Added By AndCycle, ZZUL_20050212-0200
	void			ScheduleRemovalFromUploadQueue(LPCTSTR pszDebugReason, CString strDisplayReason, bool keepWaitingTimeIntact = false) {
                        if(!m_bScheduledForRemoval) {
						    m_bScheduledForRemoval = true;
						    m_pszScheduledForRemovalDebugReason = pszDebugReason;
						    m_strScheduledForRemovalDisplayReason = strDisplayReason;
						    m_bScheduledForRemovalWillKeepWaitingTimeIntact = keepWaitingTimeIntact;
						    m_bScheduledForRemovalAtTick = ::GetTickCount();
                        }
					}

	void			UnscheduleForRemoval() {
						m_bScheduledForRemoval = false;

                        if(GetQueueSessionPayloadUp() > GetCurrentSessionLimit()) {
                            m_curSessionAmountNumber++;
                        }
					}

	bool			IsScheduledForRemoval() const { return m_bScheduledForRemoval; }

	bool			GetScheduledUploadShouldKeepWaitingTime() { return m_bScheduledForRemovalWillKeepWaitingTimeIntact; }

	LPCTSTR			GetScheduledRemovalDebugReason() const { return m_pszScheduledForRemovalDebugReason; }
    CString         GetScheduledRemovalDisplayReason() const { return m_strScheduledForRemovalDisplayReason; }

	bool			GetScheduledRemovalLimboComplete() { return m_bScheduledForRemoval && ::GetTickCount()-m_bScheduledForRemovalAtTick > SEC2MS(10); }
	DWORD			GetScheduledForRemovalAtTick() {return m_bScheduledForRemovalAtTick;} //MORPH Added by SiRoB
	//MORPH END   - Added By AndCycle, ZZUL_20050212-0200

	//download
	UINT			GetAskedCountDown() const						{ return m_cDownAsked; }
	void			AddAskedCountDown()								{ m_cDownAsked++; }
	void			SetAskedCountDown(UINT cInDownAsked)			{ m_cDownAsked = cInDownAsked; }
	EDownloadState	GetDownloadState() const						{ return (EDownloadState)m_nDownloadState; }
	void			SetDownloadState(EDownloadState nNewState, LPCTSTR pszReason = _T("Unspecified"));
	uint32			GetLastAskedTime(const CPartFile* partFile = NULL) const;
    void            SetLastAskedTime()								{ m_fileReaskTimes.SetAt(reqfile, ::GetTickCount()); }
	bool			IsPartAvailable(UINT iPart) const {
						//MORPH - Changed by SiRoB, ICS merged into partstatus
						/*
						return (iPart>=m_nPartCount || !m_abyPartStatus) ? false : (m_abyPartStatus[iPart]!=0);
						*/
						return (iPart>=m_nPartCount || !m_abyPartStatus) ? false : (m_abyPartStatus[iPart]&SC_AVAILABLE);
						//MORPH - Changed by SiRoB, ICS merged into partstatus
					}
	// Mighty Knife: Community visualization
	bool			IsCommunity() const;
	// [end] Mighty Knife

	uint8*			GetPartStatus() const							{ return m_abyPartStatus; }
	//MORPH START - Added by SiRoB, Keep A4AF infos
	uint8*			GetPartStatus(const CPartFile* partfile) const		{ uint8* thisAbyPartStatus; return m_PartStatus_list.Lookup(partfile,thisAbyPartStatus)?thisAbyPartStatus:0; }
	//MORPH END   - Added by SiRoB, Keep A4AF infos
	uint16			GetPartCount() const							{ return m_nPartCount; }
	UINT			GetDownloadDatarate() const						{ return m_nDownDatarate; }
	UINT			GetRemoteQueueRank() const						{ return m_nRemoteQueueRank; }
	void			SetRemoteQueueRank(UINT nr, bool bUpdateDisplay = false);
	bool			IsRemoteQueueFull() const						{ return m_bRemoteQueueFull; }
	void			SetRemoteQueueFull(bool flag)					{ m_bRemoteQueueFull = flag; }
	//Morph START - added by AndCycle, DiffQR
	int				GetDiffQR()	const				{return m_iDifferenceQueueRank;}
	//Morph END   - added by AndCycle, DiffQR
	//MORPH START - Added by SiRoB, Advanced A4AF derivated from Khaos
	/*
	void			DrawStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool  bFlat) const;
	*/
	void			DrawStatusBar(CDC* dc, LPCRECT rect, const CPartFile* File, bool  bFlat) const;
	//MORPH END   - Added by SiRoB, Advanced A4AF derivated from Khaos
	bool			AskForDownload();
	virtual void	SendFileRequest();
	void			SendStartupLoadReq();
	void			ProcessFileInfo(CSafeMemFile* data, CPartFile* file);
	void			ProcessFileStatus(bool bUdpPacket, CSafeMemFile* data, CPartFile* file);
	void			ProcessHashSet(const uchar* data, UINT size);
	void			ProcessAcceptUpload();
	bool			AddRequestForAnotherFile(CPartFile* file);
	void			CreateBlockRequests(int iMaxBlocks);
	virtual void	SendBlockRequests(bool ed2k = false); //MORPH - Changed by SiRoB, WebCache Retry by ed2k
	virtual bool	SendHttpBlockRequests();
	virtual void	ProcessBlockPacket(const uchar* packet, UINT size, bool packed, bool bI64Offsets);
	virtual void	ProcessHttpBlockPacket(const BYTE* pucData, UINT uSize);
	void			ClearDownloadBlockRequests();
	void			SendOutOfPartReqsAndAddToWaitingQueue();
	UINT			CalculateDownloadRate();
	uint16			GetAvailablePartCount() const;
	bool			SwapToAnotherFile(LPCTSTR pszReason, bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile = NULL, bool allowSame = true, bool isAboutToAsk = false, bool debug = false); // ZZ:DownloadManager
	void			DontSwapTo(/*const*/ CPartFile* file);
	bool			IsSwapSuspended(const CPartFile* file, const bool allowShortReaskTime = false, const bool fileIsNNP = false) /*const*/; // ZZ:DownloadManager
    uint32          GetTimeUntilReask() const;
    uint32          GetTimeUntilReask(const CPartFile* file) const;
    uint32			GetTimeUntilReask(const CPartFile* file, const bool allowShortReaskTime, const bool useGivenNNP = false, const bool givenNNP = false) const;
	void			UDPReaskACK(uint16 nNewQR);
	void			UDPReaskFNF();
	void			UDPReaskForDownload();
	bool			IsSourceRequestAllowed() const;
    bool            IsSourceRequestAllowed(CPartFile* partfile, bool sourceExchangeCheck = false) const; // ZZ:DownloadManager

	bool			IsValidSource() const;
	ESourceFrom		GetSourceFrom() const							{ return (ESourceFrom)m_nSourceFrom; }
	void			SetSourceFrom(ESourceFrom val)					{ m_nSourceFrom = (_ESourceFrom)val; }

	void			SetDownStartTime()								{ m_dwDownStartTime = ::GetTickCount(); }
	uint32			GetDownTimeDifference(boolean clear = true)	{
						uint32 myTime = m_dwDownStartTime; 
						if(clear) m_dwDownStartTime = 0;
						return ::GetTickCount() - myTime;
					}
	bool			GetTransferredDownMini() const					{ return m_bTransferredDownMini; }
	void			SetTransferredDownMini()						{ m_bTransferredDownMini = true; }
	void			InitTransferredDownMini()						{ m_bTransferredDownMini = false; }
	UINT			GetA4AFCount() const							{ return m_OtherRequests_list.GetCount(); }

	uint16			GetUpCompleteSourcesCount() const				{ return m_nUpCompleteSourcesCount; }
	//MORPH START - Added by SiRoB, Keep A4AF infos
	uint16			GetUpCompleteSourcesCount(const CPartFile* partfile) const				{ uint16 CompletSourceCount; return m_nUpCompleteSourcesCount_list.Lookup(partfile,CompletSourceCount)?CompletSourceCount:0; }
	//MORPH END   - Added by SiRoB, Keep A4AF infos
	
	//MORPH START - Changed by SiRoB, Keep A4AF infos
	/*
	void			SetUpCompleteSourcesCount(uint16 n)				{ m_nUpCompleteSourcesCount = n; }
	*/
	void			SetUpCompleteSourcesCount(uint16 n)				{ m_nUpCompleteSourcesCount = n; m_nUpCompleteSourcesCount_list.SetAt(reqfile,n); }
	//MORPH END   - Added by SiRoB, Keep A4AF infos

	//chat
	EChatState		GetChatState() const							{ return (EChatState)m_nChatstate; }
	void			SetChatState(EChatState nNewS)					{ m_nChatstate = (_EChatState)nNewS; }

	//KadIPCheck
	EKadState		GetKadState() const								{ return (EKadState)m_nKadState; }
	void			SetKadState(EKadState nNewS)					{ m_nKadState = (_EKadState)nNewS; }

	//File Comment
	bool			HasFileComment() const						{ return !m_strFileComment.IsEmpty(); }
    const CString&	GetFileComment() const						{ return m_strFileComment; } 
    void			SetFileComment(LPCTSTR pszComment)			{ m_strFileComment = pszComment; }

	bool			HasFileRating() const						{ return m_uFileRating > 0; }
    uint8			GetFileRating() const						{ return m_uFileRating; }
    void			SetFileRating(uint8 uRating)				{ m_uFileRating = uRating; }

	// Barry - Process zip file as it arrives, don't need to wait until end of block
	int				unzip(Pending_Block_Struct *block, const BYTE *zipped, UINT lenZipped, BYTE **unzipped, UINT *lenUnzipped, int iRecursion = 0);
	void			UpdateDisplayedInfo(bool force = false);
	int             GetFileListRequested() const					{ return m_iFileListRequested; }
    void            SetFileListRequested(int iFileListRequested)	{ m_iFileListRequested = iFileListRequested; }

	// message filtering
	uint8			GetMessagesReceived() const						{ return m_cMessagesReceived; }
	void			SetMessagesReceived(uint8 nCount)				{ m_cMessagesReceived = nCount; }
	void			IncMessagesReceived()							{ m_cMessagesReceived++; }
	uint8			GetMessagesSent() const							{ return m_cMessagesSent; }
	void			SetMessagesSent(uint8 nCount)					{ m_cMessagesSent = nCount; }
	void			IncMessagesSent()								{ m_cMessagesSent++; }
	bool			IsSpammer() const								{ return m_fIsSpammer; }
	void			SetSpammer(bool bVal);
	bool			GetMessageFiltered() const						{ return m_fMessageFiltered; }
	void			SetMessageFiltered(bool bVal);
	
	virtual void	SetRequestFile(CPartFile* pReqFile);
	CPartFile*		GetRequestFile() const						{return reqfile;}

	// AICH Stuff
	void			SetReqFileAICHHash(CAICHHash* val);
	CAICHHash*		GetReqFileAICHHash() const					{return m_pReqFileAICHHash;}
	bool			IsSupportingAICH() const					{return m_fSupportsAICH & 0x01;}
	void			SendAICHRequest(CPartFile* pForFile, uint16 nPart);
	bool			IsAICHReqPending() const					{return m_fAICHRequested; }
	void			ProcessAICHAnswer(const uchar* packet, UINT size);
	void			ProcessAICHRequest(const uchar* packet, UINT size);
	void			ProcessAICHFileHash(CSafeMemFile* data, CPartFile* file);

	EUtf8Str		GetUnicodeSupport() const;

	CString			GetDownloadStateDisplayString() const;
	CString			GetUploadStateDisplayString() const;

	LPCTSTR			DbgGetDownloadState() const;
	LPCTSTR			DbgGetUploadState() const;
	LPCTSTR			DbgGetKadState() const;
	CString			DbgGetClientInfo(bool bFormatIP = false) const;
	CString			DbgGetFullClientSoftVer() const;
	const CString&	DbgGetHelloInfo() const { return m_strHelloInfo; }
	const CString&	DbgGetMuleInfo() const { return m_strMuleInfo; }

// ZZ:DownloadManager -->
    const bool      IsInNoNeededList(const CPartFile* fileToCheck) const;
    const bool      SwapToRightFile(CPartFile* SwapTo, CPartFile* cur_file, bool ignoreSuspensions, bool SwapToIsNNPFile, bool isNNPFile, bool& wasSkippedDueToSourceExchange, bool doAgressiveSwapping = false, bool debug = false);
    const DWORD     getLastTriedToConnectTime() { return m_dwLastTriedToConnect; }
// <-- ZZ:DownloadManager

#ifdef _DEBUG
	// Diagnostic Support
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	CClientReqSocket* socket;
	CClientCredits*	credits;
	CFriend*		m_Friend;
	//MORPH START - Added by SiRoB, Show compression
	uint32			compressiongain;
	uint32			notcompressed; // Add show compression
	//MORPH END   - Added by SiRoB, Show compression
	uint8*			m_abyUpPartStatus;
	CTypedPtrList<CPtrList, CPartFile*> m_OtherRequests_list;
	CTypedPtrList<CPtrList, CPartFile*> m_OtherNoNeeded_list;
	uint16			m_lastPartAsked;
	//MORPH START - Changed by SiRoB, ZZ LowID handling
	/*
	bool			m_bAddNextConnect;
	*/
	DWORD			m_dwWouldHaveGottenUploadSlotIfNotLowIdTick;  // VQB Fix for LowID slots only on connection
	//MORPH END   - Changed by SiRoB, ZZ LowID handling
	uint32			m_nSelectedChunk;	// SLUGFILLER: hideOS

	void SetSlotNumber(UINT newValue) { m_slotNumber = newValue; }
	UINT GetSlotNumber() const { return m_slotNumber; }
	//MORPH START - Added by SiRoB, UPload Splitting Class
	uint32 GetClassID() const {return m_classID;}
	void	SetClassID(uint32 newvalue) { m_classID = newvalue;}
	//MORPH END   - Added by SiRoB, UPload Splitting Class
	//MORPH - Changed by SiRoB, WebCache Fix
	/*
	CEMSocket* GetFileUploadSocket(bool log = false);
	*/
	CClientReqSocket* GetFileUploadSocket(bool log = false);
	//MORPH END - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-01333

// MORPH START - Added by Commander, WebCache 1.2f
///////////////////////////////////////////////////////////////////////////
// WebCache client
private:
	bool m_bIsTrustedOHCBSender;
	bool m_bIsAllowedToSendOHCBs;
	uint32 m_uWebCacheFlags;
	EWebCacheDownState m_eWebCacheDownState;
	EWebCacheUpState m_eWebCacheUpState;
	bool b_webcacheInfoNeeded;
protected:
	bool m_bProxy;
public:
	bool m_bIsAcceptingOurOhcbs; // default - true, set to false on OP_DONT_SEND_OHCBS
	CWebCacheDownSocket* m_pWCDownSocket;
	CWebCacheUpSocket* m_pWCUpSocket;

	bool SupportsWebCacheUDP() const {return (m_uWebCacheFlags & WC_FLAGS_UDP) && SupportsUDP();}
	bool SupportsOhcbSuppression() const {return (m_uWebCacheFlags & WC_FLAGS_NO_OHCBS)!=0;}
	bool SupportsWebCacheProtocol() const {return SupportsOhcbSuppression();} // this is the first version that supports that
	bool IsProxy() const {return m_bProxy;}
	bool IsUploadingToWebCache() const;
	bool IsDownloadingFromWebCache() const;
	bool ProcessWebCacheDownHttpResponse(const CStringAArray& astrHeaders);
	bool ProcessWebCacheDownHttpResponseBody(const BYTE* pucData, UINT uSize);
	bool ProcessWebCacheUpHttpResponse(const CStringAArray& astrHeaders);
	UINT ProcessWebCacheUpHttpRequest(const CStringAArray& astrHeaders);
	void OnWebCacheDownSocketClosed(int nErrorCode);
	void OnWebCacheDownSocketTimeout();
	void SetWebCacheDownState(EWebCacheDownState eState);
	EWebCacheDownState CUpDownClient::GetWebCacheDownState() const {return m_eWebCacheDownState;}
	void SetWebCacheUpState(EWebCacheUpState eState);
	EWebCacheUpState CUpDownClient::GetWebCacheUpState() const {return m_eWebCacheUpState;}
	virtual bool SendWebCacheBlockRequests();
	void PublishWebCachedBlock( const Requested_Block_Struct* block );
	bool IsWebCacheUpSocketConnected() const;
	bool IsWebCacheDownSocketConnected() const;
//	uint16 blocksLoaded;	// Superlexx - block transfer limiter //JP blocks are counted in the socket code now
	uint16 GetNumberOfClientsBehindOurWebCacheAskingForSameFile();	// what a name ;)
	UINT GetNumberOfClientsBehindOurWebCacheHavingSameFileAndNeedingThisBlock(Pending_Block_Struct* pending); // Superlexx - COtN - it's getting better all the time...
	bool WebCacheInfoNeeded() {return b_webcacheInfoNeeded;}
	void SetWebCacheInfoNeeded(bool value) {b_webcacheInfoNeeded = value;}
//JP trusted-OHCB-senders START
	uint32 WebCachedBlockRequests;
	uint32 SuccessfulWebCachedBlockDownloads;
	bool IsTrustedOHCBSender() const {return m_bIsTrustedOHCBSender;}
	void AddWebCachedBlockToStats( bool IsGood );
//JP trusted-OHCB-senders END
//JP stop sendig OHCBs START
	void SendStopOHCBSending();
	void SendResumeOHCBSendingTCP();
	void SendResumeOHCBSendingUDP();
//JP stop sendig OHCBs END
	CWebCacheCryptography Crypt; // Superlexx - encryption
	uint32 lastMultiOHCBPacketSent;
	void SendOHCBsNow();
	bool a(uint32 a) {return a <= WC_MAX_OHCBS_IN_UDP_PACKET;}
	bool b(uint32 a, uint32 b) { return (a * 100.0 / (b + 1) < 0,2)!=0; }
	bool c(uint32 a, uint32 b) { return a > 4 && (b * 100) / a < 20; }
// MORPH END - Added by Commander, WebCache 1.2f

	///////////////////////////////////////////////////////////////////////////
	// PeerCache client
	// 
	bool IsDownloadingFromPeerCache() const;
	bool IsUploadingToPeerCache() const;
	void SetPeerCacheDownState(EPeerCacheDownState eState);
	void SetPeerCacheUpState(EPeerCacheUpState eState);

	int  GetHttpSendState() const									{ return m_iHttpSendState; }
	void SetHttpSendState(int iState)								{ m_iHttpSendState = iState; }

	bool SendPeerCacheFileRequest();
	bool ProcessPeerCacheQuery(const uchar* packet, UINT size);
	bool ProcessPeerCacheAnswer(const uchar* packet, UINT size);
	bool ProcessPeerCacheAcknowledge(const uchar* packet, UINT size);
	void OnPeerCacheDownSocketClosed(int nErrorCode);
	bool OnPeerCacheDownSocketTimeout();

	bool ProcessPeerCacheDownHttpResponse(const CStringAArray& astrHeaders);
	bool ProcessPeerCacheDownHttpResponseBody(const BYTE* pucData, UINT uSize);
	void ProcessPeerCacheUpHttpResponse(const CStringAArray& astrHeaders);
	UINT ProcessPeerCacheUpHttpRequest(const CStringAArray& astrHeaders);

	virtual bool ProcessHttpDownResponse(const CStringAArray& astrHeaders);
	virtual bool ProcessHttpDownResponseBody(const BYTE* pucData, UINT uSize);

	CPeerCacheDownSocket* m_pPCDownSocket;
	CPeerCacheUpSocket* m_pPCUpSocket;

	LPCTSTR		TestLeecher(); //MORPH - Added by IceCream, anti-leecher feature
	//MORPH START - Added by SiRoB, Is Morph Client
	bool IsMorph() const { return m_bIsMorph;}
	//MORPH END   - Added by SiRoB, Is Morph Client

	//EastShare Start - Added by AndCycle, PayBackFirst
	bool	IsMoreUpThanDown() const;
	bool	IsMoreUpThanDown(const CKnownFile* file) const; //MORPH - Added by SiRoB, Code Optimization
	//EastShare End - Added by AndCycle, PayBackFirst
	//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-0133
	bool			GetPowerShared() const;
	bool			GetPowerShared(const CKnownFile* file) const;  //MORPH - Added by SiRoB, Code Optimization
	//MORPH END - Added by SiRoB, ZZ Upload System 20030723-0133
	//MORPH START - Added by SiRoB, Show client Requested file
	void			ShowRequestedFiles();
	//MORPH END   - Added by SiRoB, Show client Requested file
	
	//Morph Start - added by AndCycle, take PayBackFirst have same class with PowerShare
	//this is used to replace all "GetPowerShared()" with "IsPBForPS()" in UploadQueue.cpp
	bool	IsPBForPS() const;
	//Morph End - added by AndCycle, take PayBackFirst have same class with PowerShare

	//Morph - added by AndCycle, separate secure check
	bool	IsSecure() const;

	//Morph - added by AndCycle, Equal Chance For Each File
	double	GetEqualChanceValue() const;

	//Morph Start - added by AndCycle, ICS
	// enkeyDEV: ICS
	void	ProcessFileIncStatus(CSafeMemFile* data,uint32 size, CPartFile* pFile);
	uint32	GetIncompletePartVersion()	{return m_incompletepartVer;}
	// <--- enkeyDEV: ICS
	//Morph End - added by AndCycle, ICS

	//MORPH START - Added by SiRoB, ShareOnlyTheNeed hide Uploaded and uploading part
	void GetUploadingAndUploadedPart(uint8* abyUpPartUploadingAndUploaded, uint16 partcount);
	//MORPH END   - Added by SiRoB, ShareOnlyTheNeed hide Uploaded and uploading part
	//wistily start
	void  Add2DownTotalTime(uint32 length){m_nDownTotalTime += length;}
	void  Add2UpTotalTime(uint32 length){m_nUpTotalTime += length;}
	uint32  GetDownTotalTime() const  {return m_nDownTotalTime;}
	uint32  GetAvDownDatarate() const;
	uint32  GetUpTotalTime() const  {return m_nUpTotalTime;}
	uint32  GetAvUpDatarate()const;
	//wistily stop
        // MORPH START - Added by Commander, WebCache 1.2e
	// Squid defaults..
	bool			UsesCachedTCPPort() { return ( (GetUserPort()==80)
													|| (GetUserPort()==21)
													|| (GetUserPort()==443)
													|| (GetUserPort()==563)
													|| (GetUserPort()==70)
													|| (GetUserPort()==210)
													|| ((GetUserPort()>=1025) && (GetUserPort()<=65535)));}
	bool			SupportsWebCache() const { return m_bWebCacheSupport; }
	bool			SupportsMultiOHCBs() const {return m_bWebCacheSupportsMultiOHCBs;}
	bool			IsBehindOurWebCache() const
					{
						// WC-TODO: make this more efficient
						return( thePrefs.webcacheName == GetWebCacheName() );
					}
// jp webcache
	CString			GetWebCacheName() const 
					{ 
						if (SupportsWebCache())
							return WebCacheIndex2Name(m_WA_webCacheIndex);
						else
							return _T("");
					}
	bool			m_bWebcacheFailedTry; //MORPH - Added by SiRoB, New ResolveWebCachename
	bool			m_bWebCacheSupport;
	bool			m_bWebCacheSupportsMultiOHCBs;
	// Superlexx - webcache
	int		m_WA_webCacheIndex;	// index of the webcache name
	uint16	m_WA_HTTPPort;		// remote webserver port
	uint32	m_uWebCacheDownloadId;	// we must attach this ID when sending HTTP download request to the remote client.
	uint32	m_uWebCacheUploadId;	// incoming HTTP requests are identified as WC-requests,
									// if the header contains this ID and there is a known client with same ID in downloading state.
									// used for client authorization, should be substituted by a HttpIdList? later
									// for efficiency reasons.
// Superlexx - MFR
	CWebCacheMFRList	requestedFiles; // the files this client requested from us
	Packet*	CreateMFRPacket();		// builds a separate MFR-packet
	bool	AttachMultiOHCBsRequest(CSafeMemFile &data); // Superlexx - attaches a multiple files request
	bool	IsPartAvailable(UINT iPart, const byte* fileHash) {return requestedFiles.IsPartAvailable(iPart, fileHash);}
    // MORPH END - Added by Commander, WebCache 1.2e
	void	SetPartCount(uint16 parts) { m_nUpPartCount = parts;}
	//MORPH - Added by SiRoB, ReadBlockFromFileThread
	void	SetReadBlockFromFileBuffer(byte* pdata) {filedata = pdata;};
	//MORPH - Added by SiRoB, ReadBlockFromFileThread
protected:
	int m_iHttpSendState;
	uint32 m_uPeerCacheDownloadPushId;
	uint32 m_uPeerCacheUploadPushId;
	uint32 m_uPeerCacheRemoteIP;
	bool m_bPeerCacheDownHit;
	bool m_bPeerCacheUpHit;
	EPeerCacheDownState m_ePeerCacheDownState;
	EPeerCacheUpState m_ePeerCacheUpState;

protected:
	// base
	void	Init();
	bool	ProcessHelloTypePacket(CSafeMemFile* data);
	void	SendHelloTypePacket(CSafeMemFile* data);
	void	CreateStandartPackets(byte* data, UINT togo, Requested_Block_Struct* currentblock, bool bFromPF = true);
	void	CreatePackedPackets(byte* data, UINT togo, Requested_Block_Struct* currentblock, bool bFromPF = true);
	
	uint32	m_nConnectIP;		// holds the supposed IP or (after we had a connection) the real IP
	uint32	m_dwUserIP;			// holds 0 (real IP not yet available) or the real IP (after we had a connection)
	uint32	m_dwServerIP;
	uint32	m_nUserIDHybrid;
	uint16	m_nUserPort;
	uint16	m_nServerPort;
	UINT	m_nClientVersion;
//--group to aligned int32
	uint8	m_byEmuleVersion;
	uint8	m_byDataCompVer;
	bool	m_bEmuleProtocol;
	bool	m_bIsHybrid;
//--group to aligned int32
	TCHAR*	m_pszUsername;
	TCHAR*	m_pszFunnyNick; //MORPH - Added by SiRoB, FunnyNick
	TCHAR*	old_m_pszUsername; //MORPH - Added by IceCream, Antileecher feature
	uchar	m_achUserHash[16];
	uint16	m_nUDPPort;
	uint16	m_nKadPort;
//--group to aligned int32
	uint8	m_byUDPVer;
	uint8	m_bySourceExchangeVer;
	uint8	m_byAcceptCommentVer;
	uint8	m_byExtendedRequestsVer;
//--group to aligned int32
	uint8	m_byCompatibleClient;
	bool	m_bFriendSlot;
	bool	m_bCommentDirty;
	bool	m_bIsML;
//--group to aligned int32
	bool	m_bGPLEvildoer;
	uint8	m_byHelloPacketState; //MORPH - Changed by SiRoB, Fix Connection Collision
	uint8	m_byInfopacketsReceived;	// have we received the edonkeyprot and emuleprot packet already (see InfoPacketsReceived() )
	uint8	m_bySupportSecIdent;
//--group to aligned int32
	uint32	m_dwLastSignatureIP;
	CString m_strClientSoftware;
	CString	old_m_strClientSoftware; //MORPH - Added by IceCream, Antileecher feature
	CString m_strModVersion;
	uint32	m_dwLastSourceRequest;
	uint32	m_dwLastSourceAnswer;
	uint32	m_dwLastAskedForSources;
    int     m_iFileListRequested;
	CString	m_strFileComment;
	//--group to aligned int32
	uint8	m_uFileRating;
	uint8	m_cMessagesReceived;		// count of chatmessages he sent to me
	uint8	m_cMessagesSent;			// count of chatmessages I sent to him
	bool	m_bMultiPacket;
	//--group to aligned int32
	bool	m_bUnicodeSupport;
	bool	m_bBuddyIDValid;
	uint16	m_nBuddyPort;
	//--group to aligned int32
	uint32	m_nBuddyIP;
	uint32	m_dwLastBuddyPingPongTime;
	uchar	m_achBuddyID[16];
	CString m_strHelloInfo;
	CString m_strMuleInfo;
	uint8	m_byKadVersion;

	// States
	_EClientSoftware	m_clientSoft;
	_EChatState			m_nChatstate;
	_EKadState			m_nKadState;
	_ESecureIdentState	m_SecureIdentState;
	_EUploadState		m_nUploadState;
	_EDownloadState		m_nDownloadState;
	_ESourceFrom		m_nSourceFrom;

	CTypedPtrList<CPtrList, Packet*> m_WaitingPackets_list;
	CList<PartFileStamp> m_DontSwap_list;

	uint32  AskTime; //MORPH - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	bool	m_bIsMorph; //MORPH - Added by SiRoB, Is Morph client?
	bool	m_bLeecher; //MORPH - Added by IceCream, anti-leecher feature
	CString m_strNotOfficial; //MORPH - Added by SiRoB, Control Mod Tag
	uint32  m_uNotOfficial; //MORPH - Added by SiRoB, Control Mod Tag
	////////////////////////////////////////////////////////////////////////
	// Upload
	//
	int	GetFilePrioAsNumber() const;

	UINT		m_nTransferredUp;
	uint32		m_dwUploadTime;
	uint32		m_nAvUpDatarate; //Wistily
	uint32		m_nUpTotalTime;//wistily total lenght of this client's uploads during this session in ms
	UINT		m_cAsked;
	uint32		m_dwLastUpRequest;
	UINT		m_nCurSessionUp;
	UINT		m_nCurSessionDown;
    uint32      m_nCurQueueSessionUp;
    UINT		m_nCurQueueSessionPayloadUp;
    UINT		m_addedPayloadQueueSession;
    uint32      m_curSessionAmountNumber;
	uint16		m_nUpPartCount;
	uint16		m_nUpCompleteSourcesCount;
	uchar		requpfileid[16];
	//MORPH START - Added by SiRoB, Optimization requpfile
	CKnownFile* requpfile;
	DWORD		requpfileid_lasttimeupdated;
	//MORPH END   - Added by SiRoB, Optimization requpfile
    UINT		m_slotNumber;
	bool		m_bCollectionUploadSlot;
	uint32		m_classID; //MORPH - Added by SiRoB, UPload Splitting Class

	/*ZZ*/DWORD       m_dwLastCheckedForEvictTick;

	typedef struct TransferredData {
		UINT	datalen;
		DWORD	timestamp;
	};
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_BlockRequests_queue;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_DoneBlocks_list;
	CTypedPtrList<CPtrList, Requested_File_Struct*>	 m_RequestedFiles_list;

	//MORPH START - Added by SiRoB, ReadBlockFromFileThread
	byte* filedata;
	//MORPH END   - Added by SiRoB, ReadBlockFromFileThread
	//////////////////////////////////////////////////////////
	// Download
	//
	CPartFile*	reqfile;
	CAICHHash*  m_pReqFileAICHHash; 
	UINT		m_cDownAsked;
	uint8*		m_abyPartStatus;
	CString		m_strClientFilename;
	UINT		m_nTransferredDown;
	uint32		m_dwDownStartTime;
	uint64		m_nLastBlockOffset;
	uint32		m_dwLastBlockReceived;
	UINT		m_nTotalUDPPackets;
	UINT		m_nFailedUDPPackets;
	UINT		m_nRemoteQueueRank;
	//MORPH START - Added by AndCycle, DiffQR
	int			m_iDifferenceQueueRank;
	//MORPH END  - Added by AndCycle, DiffQR
	//--group to aligned int32
	bool		m_bRemoteQueueFull;
	bool		m_bCompleteSource;
	uint16		m_nPartCount;
	//--group to aligned int32
	uint16		m_cShowDR;
	bool		m_bReaskPending;
	bool		m_bUDPPending;
	bool		m_bTransferredDownMini;

	// Download from URL
	CStringA	m_strUrlPath;
	uint64		m_uReqStart;
	uint64		m_uReqEnd;
	uint64		m_nUrlStartPos;


	//////////////////////////////////////////////////////////
	// Upload data rate computation
	//
	UINT		m_nUpDatarate;
	UINT		m_nSumForAvgUpDataRate;
	CList<TransferredData> m_AvarageUDR_list;
	DWORD		m_AvarageUDRLastRemovedTimestamp;	//MORPH - Added by SiRoB, Better datarate mesurement for low and high speed
	//////////////////////////////////////////////////////////
	// Download data rate computation
	//
	UINT		m_nDownDatarate;
	uint32		m_nAvDownDatarate; //Wistily
	uint32		m_nDownTotalTime;// wistily total lenght of this client's downloads during this session in ms
	UINT		m_nDownDataRateMS;
	UINT		m_nSumForAvgDownDataRate;
	CList<TransferredData> m_AvarageDDR_list;
	uint32		m_AvarageDDR_ListLastRemovedTimestamp; //MORPH - Added by SiRoB, Better datarate mesurement for low and high speed

	//////////////////////////////////////////////////////////
	// GUI helpers
	//
	static CBarShader s_StatusBar;
	static CBarShader s_UpStatusBar;

	DWORD		m_lastRefreshedDLDisplay;
    DWORD		m_lastRefreshedULDisplay;
    uint32      m_random_update_wait;

	// using bitfield for less important flags, to save some bytes
	UINT m_fHashsetRequesting : 1, // we have sent a hashset request to this client in the current connection
		 m_fSharedDirectories : 1, // client supports OP_ASKSHAREDIRS opcodes
		 m_fSentCancelTransfer: 1, // we have sent an OP_CANCELTRANSFER in the current connection
		 m_fNoViewSharedFiles : 1, // client has disabled the 'View Shared Files' feature, if this flag is not set, we just know that we don't know for sure if it is enabled
		 m_fSupportsPreview   : 1,
		 m_fPreviewReqPending : 1,
		 m_fPreviewAnsPending : 1,
		 m_fIsSpammer		  : 1,
		 m_fMessageFiltered   : 1,
		 m_fPeerCache		  : 1,
		 m_fQueueRankPending  : 1,
		 m_fUnaskQueueRankRecv: 2,
		 m_fFailedFileIdReqs  : 4, // nr. of failed file-id related requests per connection
		 m_fNeedOurPublicIP	  : 1, // we requested our IP from this client
		 m_fSupportsAICH	  : 3,
		 m_fAICHRequested     : 1,
		 m_fSentOutOfPartReqs : 1,
		 m_fSupportsLargeFiles: 1,
		 m_fExtMultiPacket	  : 1,
		 m_fFailedDownload	  : 1; //MORPH - Added by SiRoB, Fix Connection Collision

	CTypedPtrList<CPtrList, Pending_Block_Struct*>	 m_PendingBlocks_list;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_DownloadBlocks_list;

    bool    m_bSourceExchangeSwapped; // ZZ:DownloadManager
    DWORD   lastSwapForSourceExchangeTick; // ZZ:DownloadManaager
    bool    DoSwap(CPartFile* SwapTo, bool bRemoveCompletely, LPCTSTR reason); // ZZ:DownloadManager
    CMap<CPartFile*, CPartFile*, DWORD, DWORD> m_fileReaskTimes; // ZZ:DownloadManager (one resk timestamp for each file)
    DWORD   m_dwLastTriedToConnect; // ZZ:DownloadManager (one resk timestamp for each file)
    bool    RecentlySwappedForSourceExchange() { return ::GetTickCount()-lastSwapForSourceExchangeTick < 30*1000; } // ZZ:DownloadManager
    void    SetSwapForSourceExchangeTick() { lastSwapForSourceExchangeTick = ::GetTickCount(); } // ZZ:DownloadManager

	//MORPH START - Added by SiRoB, Keep A4AF infos
	CMap<const CPartFile*, const CPartFile*, uint8*, uint8*>	 m_PartStatus_list;
	CMap<const CPartFile*, const CPartFile*, uint16, uint16>	 m_nUpCompleteSourcesCount_list;
	//MORPH END   - Added by SiRoB, Keep A4AF infos

	//MORPH START - Added By AndCycle, ZZUL_20050212-0200
	LPCTSTR m_pszScheduledForRemovalDebugReason;
    CString m_strScheduledForRemovalDisplayReason;
	bool	m_bScheduledForRemoval;
	bool	m_bScheduledForRemovalWillKeepWaitingTimeIntact;
	DWORD	m_bScheduledForRemovalAtTick;
	//MORPH END   - Added By AndCycle, ZZUL_20050212-0200

//EastShare Start - added by AndCycle, IP to Country
public:
	CString			GetCountryName(bool longName = false) const;
	int				GetCountryFlagIndex() const;
	void			ResetIP2Country(uint32 dwIP = 0);

	//SLAHAM: ADDED Show Downloading Time =>
	uint16	uiStartDLCount;
	DWORD	dwStartDLTime;
	DWORD	dwSessionDLTime;
	DWORD	dwTotalDLTime;
	//SLAHAM: ADDED Show Downloading Time <=

	//SLAHAM: ADDED Known Since/Last Asked Counter =>
	uint16	uiDLAskingCounter; 
	DWORD	dwThisClientIsKnownSince;
	//SLAHAM: ADDED Known Since/Last Asked Counter <=
private:
	struct	IPRange_Struct2* m_structUserCountry; //EastShare - added by AndCycle, IP to Country
//EastShare End - added by AndCycle, IP to Country

//Morph Start - added by AndCycle, ICS
	// enkeyDEV: ICS
	uint32	m_incompletepartVer;
	// <--- enkeyDEV: ICS
//Morph End - added by AndCycle, ICS

public:
	bool	IsMorphLeecher(); // Morph Start - added by Stulle, Morph Leecher Detection
};
//#pragma pack()
//>>> eWombat [SNAFU_V3]
static LPCTSTR apszSnafuTag[] =
	{
	_T("[DodgeBoards]"),									//0
	_T("[DodgeBoards & DarkMule eVorteX]"),					//1
	_T("[DarkMule v6 eVorteX]"),							//2
	_T("[eMuleReactor]"),									//3
	_T("[Bionic]"),											//4
	_T("[LSD7c]"),											//5
	_T("[0x8d] unknown Leecher - (client version:60)"),		//6
	_T("[RAMMSTEIN]"),										//7
	_T("[MD5 Community]"),									//8
	_T("[new DarkMule]"),									//9
	_T("[OMEGA v.07 Heiko]"),								//10
	_T("[eMule v0.26 Leecher]"),							//11
	_T("[Hunter]"),											//12
	_T("[Bionic 0.20 Beta]"),								//13
	_T("[Rumata (rus)(Plus v1f)]"),							//14
	_T("[Fusspi]"),											//15
	_T("[donkey2002]"),										//16
	_T("[md4]")										        //17
	};

//<<< new tags from eMule 0.04x
#define CT_UNKNOWNx0			0x00 // Hybrid Horde protocol
#define CT_UNKNOWNx12			0x12 // http://www.haspepapa-welt.de (DodgeBoards)
#define CT_UNKNOWNx13			0x13 // http://www.haspepapa-welt.de (DodgeBoards)
#define CT_UNKNOWNx14			0x14 // http://www.haspepapa-welt.de (DodgeBoards)
#define CT_UNKNOWNx15			0x15 // http://www.haspepapa-welt.de (DodgeBoards) & DarkMule |eVorte|X|
#define CT_UNKNOWNx16			0x16 // http://www.haspepapa-welt.de (DodgeBoards)
#define CT_UNKNOWNx17			0x17 // http://www.haspepapa-welt.de (DodgeBoards)
#define CT_UNKNOWNxE6			0xE6 // http://www.haspepapa-welt.de
#define CT_UNKNOWNx22			0x22 // DarkMule |eVorte|X|
#define CT_UNKNOWNx5D			0x5D // md4 
#define CT_UNKNOWNx63			0x63 // ?
#define CT_UNKNOWNx64			0x64 // ?
#define CT_UNKNOWNx69			0x69 // eMuleReactor
#define CT_UNKNOWNx6B			0x6B // md4
#define CT_UNKNOWNx6C			0x6C // md4
#define CT_UNKNOWNx74			0x74 // md4
#define CT_UNKNOWNx76			0x76 // www.donkey2002.to
#define CT_UNKNOWNx79			0x79 // Bionic
#define CT_UNKNOWNx7A			0x7A // NewDarkMule
#define CT_UNKNOWNx83			0x83 // Fusspi
#define CT_UNKNOWNx87			0x87 // md4
#define CT_UNKNOWNx88			0x88 // DarkMule v6 |eVorte|X|
#define CT_UNKNOWNx8c			0x8c // eMule v0.27c [LSD7c] 
#define CT_UNKNOWNx8d			0x8d // unknown Leecher - (client version:60)
#define CT_UNKNOWNx97			0x97 // Emulereactor Community Mod
#define CT_UNKNOWNx98			0x98 // Emulereactor Community Mod
#define CT_UNKNOWNx99			0x99 // eMule v0.26d [RAMMSTEIN 8b]
#define CT_UNKNOWNx9C			0x9C // Emulereactor Community Mod
#define CT_UNKNOWNxbb			0xbb // emule.de (client version:60)
#define CT_UNKNOWNxc4			0xc4 //MD5 Community from new bionic - hello
#define CT_UNKNOWNxCA			0xCA // NewDarkMule
#define CT_UNKNOWNxCD			0xCD // www.donkey2002.to
#define CT_UNKNOWNxDA			0xDA // Emulereactor Community Mod
#define CT_UNKNOWNxF0			0xF0 // Emulereactor Community Mod
#define CT_UNKNOWNxF4			0xF4 // Emulereactor Community Mod

#define CT_FRIENDSHARING		0x66 //eWombat  [SNAFU]
#define CT_DARK					0x54 //eWombat [SNAFU]
#define FRIENDSHARING_ID 0x5F73F1A0 // Magic Key, DO NOT CHANGE!

// unknown eMule tags
#define ET_MOD_UNKNOWNx12		0x12 // http://www.haspepapa-welt.de
#define ET_MOD_UNKNOWNx13		0x13 // http://www.haspepapa-welt.de
#define ET_MOD_UNKNOWNx14		0x14 // http://www.haspepapa-welt.de
#define ET_MOD_UNKNOWNx17		0x17 // http://www.haspepapa-welt.de
#define ET_MOD_UNKNOWNx2F		0x2F // eMule v0.30 [OMEGA v.07 Heiko]
#define ET_MOD_UNKNOWNx30		0x30 // aMule 1.2.0
#define ET_MOD_UNKNOWNx36		0x36 // eMule v0.26
#define ET_MOD_UNKNOWNx3C		0x3C // enkeyDev.6 / LamerzChoice 9.9a
#define ET_MOD_UNKNOWNx41		0x41 // CrewMod (pre-release mod based on Plus) identification
#define ET_MOD_UNKNOWNx42		0x42 // CrewMod (pre-release mod based on Plus) key verification
#define ET_MOD_UNKNOWNx43		0x43 // CrewMod (pre-release mod based on Plus) version info
#define ET_MOD_UNKNOWNx50		0x50 // Bionic 0.20 Beta]
#define ET_MOD_UNKNOWNx59		0x59 // emule 0.40 / eMule v0.30 [LSD.12e]
#define ET_MOD_UNKNOWNx5B		0x5B // eMule v0.26
#define ET_MOD_UNKNOWNx60		0x60 // eMule v0.30a Hunter.6 + eMule v0.26
#define ET_MOD_UNKNOWNx64		0x64 // LSD.9dT / Athlazan(0.29c)Alpha.3
#define ET_MOD_UNKNOWNx76		0x76 // http://www.haspepapa-welt.de (DodgeBoards)
#define ET_MOD_UNKNOWNx84		0x84 // eChanblardv3.2
#define ET_MOD_UNKNOWNx85		0x85 // ? 
#define ET_MOD_UNKNOWNx86		0x86 // ? 
#define ET_MOD_UNKNOWNx93		0x93 // ?
#define ET_MOD_UNKNOWNxA6		0xA6 // eMule v0.26
#define ET_MOD_UNKNOWNxB1		0xB1 // Bionic 0.20 Beta]
#define ET_MOD_UNKNOWNxB4		0xB4 // Bionic 0.20 Beta]
#define ET_MOD_UNKNOWNxC8		0xC8 // Bionic 0.20 Beta]
#define ET_MOD_UNKNOWNxC9		0xC9 // Bionic 0.20 Beta]
#define ET_MOD_UNKNOWNxDA		0xDA // Rumata (rus)(Plus v1f) - leecher mod?
//>>> eWombat [SNAFU_V3]

//MORPH START - Added by SiRoB, ReadBlockFromFileThread
class CReadBlockFromFileThread : public CWinThread
{
	DECLARE_DYNCREATE(CReadBlockFromFileThread)
protected:
	CReadBlockFromFileThread()	{}
public:
	virtual	BOOL	InitInstance() {return true;}
	virtual int		Run();
	void			SetReadBlockFromFile(CKnownFile* pfile, uint64 startOffset, uint32 togo, CUpDownClient* client);
private:
	uint64			StartOffset;
	uint32			togo;
	CUpDownClient*	m_client;
	CKnownFile*		srcfile;
	CSyncObject*	lockFile;
};
//MORPH END   - Added by SiRoB, ReadBlockFromFileThread