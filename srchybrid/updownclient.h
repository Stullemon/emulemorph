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
#pragma once
#include "BarShader.h"
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country

// MORPH START - Added by Commander, WebCache 1.2e
#include "Preferences.h"
#include "WebCache/WebCache.h"
#include "WebCache/WebCacheCryptography.h"
//#include <crypto51/arc4.h>

// USING_NAMESPACE(CryptoPP)

enum EWebCacheDownState{
	WCDS_NONE = 0,
	WCDS_WAIT_CLIENT_REPLY,
	WCDS_WAIT_CACHE_REPLY,
	WCDS_DOWNLOADING
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
	uint32                  totalUnzipped; // Barry - This holds the total unzipped bytes for all packets so far
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
	KS_CONNECTED_BUDDY,
	KS_NONE_LOWID,
	KS_WAITCALLBACK_LOWID,
	KS_QUEUE_LOWID
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

enum ESourceFrom{
	SF_SERVER			= 0,
	SF_KADEMLIA			= 1,
	SF_SOURCE_EXCHANGE	= 2,
	SF_PASSIVE			= 3,
	SF_LINK				= 4,
	SF_SLS				= 5 //MORPH - Added by SiRoB, Save Load Sources (SLS)
};

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
	//base
	CUpDownClient(CClientReqSocket* sender = 0);
	CUpDownClient(CPartFile* in_reqfile, uint16 in_port, uint32 in_userid, uint32 in_serverup, uint16 in_serverport, bool ed2kID = false);
	virtual ~CUpDownClient();


	void StartDownload();
	virtual void CheckDownloadTimeout();
	virtual void SendCancelTransfer(Packet* packet = NULL);
	virtual bool	IsEd2kClient() const { return true; }
	virtual bool	Disconnected(LPCTSTR pszReason, bool bFromSocket = false);
	virtual bool	TryToConnect(bool bIgnoreMaxCon = false, CRuntimeClass* pClassSocket = NULL);
	virtual bool	Connect();
	virtual void	ConnectionEstablished();
	virtual void	OnSocketConnected(int nErrorCode);
	bool			CheckHandshakeFinished(UINT protocol, UINT opcode) const;
	void			CheckFailedFileIdReqs(const uchar* aucFileHash);
	uint32			GetUserIDHybrid() const							{ return m_nUserIDHybrid; }
	void			SetUserIDHybrid(uint32 val)						{ m_nUserIDHybrid = val; }
	LPCTSTR			GetUserName() const								{ return m_pszUsername; }
	void			SetUserName(LPCTSTR pszNewName);
	uint32			GetIP() const									{ return m_dwUserIP; }
	void			SetIP( uint32 val ) //Only use this when you know the real IP or when your clearing it.
					{
						m_dwUserIP = val;
						m_nConnectIP = val;
					}
	bool			HasLowID() const;
	uint32			GetConnectIP() const				{return m_nConnectIP;}
	uint16			GetUserPort() const								{ return m_nUserPort; }
	void			SetUserPort(uint16 val)							{ m_nUserPort = val; }
	uint32			GetTransferedUp() const							{ return m_nTransferedUp; }
	uint32			GetTransferedDown() const						{ return m_nTransferedDown; }
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
	void			ReGetClientSoft();
	uint32			GetVersion() const								{ return m_nClientVersion; }
	uint8			GetMuleVersion() const							{ return m_byEmuleVersion; }
	bool			ExtProtocolAvailable() const					{ return m_bEmuleProtocol; }
	bool			SupportMultiPacket() const						{ return m_bMultiPacket; }
	bool			SupportPeerCache() const { return m_fPeerCache; }
	bool			IsEmuleClient() const							{ return m_byEmuleVersion; }
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
	void			ProcessSharedFileList(char* pachPacket, uint32 nSize, LPCTSTR pszDirectory = NULL);
	void			ClearHelloProperties();
	bool			ProcessHelloAnswer(char* pachPacket, uint32 nSize);
	bool			ProcessHelloPacket(char* pachPacket, uint32 nSize);
	void			SendHelloAnswer();
	virtual bool	SendHelloPacket();
	void			SendMuleInfoPacket(bool bAnswer);
	void			ProcessMuleInfoPacket(char* pachPacket, uint32 nSize);
	void			ProcessMuleCommentPacket(char* pachPacket, uint32 nSize);
	//<<< eWombat [SNAFU_V3]
	void			ProcessUnknownHelloTag(CTag *tag);
	void			ProcessUnknownInfoTag(CTag *tag);
	//>>> eWombat [SNAFU_V3]
	void			ProcessEmuleQueueRank(char* packet, UINT size);
	void			ProcessEdonkeyQueueRank(char* packet, UINT size);
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

	//MORPH START - Added by IceCream, Anti-leecher feature
	bool			IsLeecher()	const				{return m_bLeecher;}
	//MORPH END   - Added by IceCream, Anti-leecher feature
	float			GetCompression() const	{return (float)compressiongain/notcompressed*100.0f;} // Add rod show compression
	void			ResetCompressionGain() {compressiongain = 0; notcompressed=1;} // Add show compression
	uint32			GetAskTime()	{return AskTime;} //MORPH - Added by SiRoB - Smart Upload Control v2 (SUC) [lovelace]

	// secure ident
	void			SendPublicKeyPacket();
	void			SendSignaturePacket();
	void			ProcessPublicKeyPacket(uchar* pachPacket, uint32 nSize);
	void			ProcessSignaturePacket(uchar* pachPacket, uint32 nSize);
	uint8			GetSecureIdentState() const						{ return m_SecureIdentState; }
	void			SendSecIdentStatePacket();
	void			ProcessSecIdentStatePacket(uchar* pachPacket, uint32 nSize);
	uint8			GetInfoPacketsReceived() const					{ return m_byInfopacketsReceived; }
	void			InfoPacketsReceived();
	// preview
	void			SendPreviewRequest(const CAbstractFile* pForFile);
	void			SendPreviewAnswer(const CKnownFile* pForFile, CxImage** imgFrames, uint8 nCount);
	void			ProcessPreviewReq(char* pachPacket, uint32 nSize);
	void			ProcessPreviewAnswer(char* pachPacket, uint32 nSize);
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
	uint32			GetWaitTime() const								{ return m_dwUploadTime - GetWaitStartTime(); }
	bool			IsDownloading() const							{ return (m_nUploadState == US_UPLOADING); }
	bool			HasBlocks() const								{ return !m_BlockRequests_queue.IsEmpty(); }
	uint32			GetDatarate() const								{ return m_nUpDatarate; }	
	uint32			GetScore(bool sysvalue, bool isdownloading = false, bool onlybasevalue = false) const;
	void			AddReqBlock(Requested_Block_Struct* reqblock);
	void			CreateNextBlockPackage();
	uint32			GetUpStartTimeDelay() const						{ return ::GetTickCount() - m_dwUploadTime; }
	void 			SetUpStartTime()								{ m_dwUploadTime = ::GetTickCount(); }
	void			SendHashsetPacket(char* forfileid);
	const uchar*	GetUploadFileID() const							{ return requpfileid; }
	void			SetUploadFileID(CKnownFile* newreqfile);
	//MORPH - Changed by SiRoB,  -Fix-
	/*
	uint32			SendBlockData();
	*/
	bool			SendBlockData();
	//MORPH - Changed by SiRoB, -Fix-
	void			ClearUploadBlockRequests();
	void			SendRankingInfo();
	void			SendCommentInfo(/*const*/ CKnownFile *file);
	void			AddRequestCount(const uchar* fileid);
	void			UnBan();
	void			Ban(LPCTSTR pszReason = NULL);
	void			BanLeecher(LPCTSTR pszReason = NULL); //MORPH - Added by IceCream, Anti-leecher feature

	uint32			GetAskedCount() const							{ return m_cAsked; }
	void			AddAskedCount()									{ m_cAsked++; }
	void			SetAskedCount(uint32 m_cInAsked)				{ m_cAsked = m_cInAsked; }
	void			FlushSendBlocks(); // call this when you stop upload, or the socket might be not able to send
	uint32			GetLastUpRequest() const						{ return m_dwLastUpRequest; }
	void			SetLastUpRequest()								{ m_dwLastUpRequest = ::GetTickCount(); }
	uint32			GetSessionUp() const							{ return m_nTransferedUp - m_nCurSessionUp; }
	void			ResetSessionUp(){
						m_nCurSessionUp = m_nTransferedUp;
						m_addedPayloadQueueSession = GetQueueSessionPayloadUp(); 
						//m_nCurQueueSessionPayloadUp = 0;
					}
	uint32			GetQueueSessionUp() const
                    {
                        return m_nTransferedUp - m_nCurQueueSessionUp;
                    }

	void			ResetQueueSessionUp()
                    {
                        m_nCurQueueSessionUp = m_nTransferedUp;
						m_nCurQueueSessionPayloadUp = 0;
                        m_curSessionAmountNumber = 0;
					} 
	uint32			GetQueueSessionPayloadUp() { return m_nCurQueueSessionPayloadUp; }


	uint32			GetQueueSessionPayloadUp() const { return m_nCurQueueSessionPayloadUp; }
    uint64          GetCurrentSessionLimit() const
                    {
                        return (uint64)SESSIONMAXTRANS*(m_curSessionAmountNumber+1)+20*1024;
                    }

	void			ProcessExtendedInfo(CSafeMemFile* packet, CKnownFile* tempreqfile);
	uint16			GetUpPartCount() const							{ return m_nUpPartCount; }
	void			DrawUpStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat) const;
    uint32          GetPayloadInBuffer() const						{ return m_addedPayloadQueueSession - GetQueueSessionPayloadUp(); }
	bool			IsUpPartAvailable(uint16 iPart) const {
						return (iPart>=m_nUpPartCount || !m_abyUpPartStatus) ? 0 : m_abyUpPartStatus[iPart];
					}
	uint8*			GetUpPartStatus() const							{ return m_abyUpPartStatus; }
    /*
	float           GetCombinedFilePrioAndCredit();
	*/
	double			GetCombinedFilePrioAndCredit();

	//download
	uint32			GetAskedCountDown() const						{ return m_cDownAsked; }
	void			AddAskedCountDown()								{ m_cDownAsked++; }
	void			SetAskedCountDown(uint32 m_cInDownAsked)		{ m_cDownAsked = m_cInDownAsked; }
	EDownloadState	GetDownloadState() const						{ return (EDownloadState)m_nDownloadState; }
	void			SetDownloadState(EDownloadState nNewState);

	uint32			GetLastAskedTime(const CPartFile* partFile = NULL) const;
    void            SetLastAskedTime()								{ m_fileReaskTimes.SetAt(reqfile, ::GetTickCount()); }
	bool			IsPartAvailable(uint16 iPart) const {
						return (iPart>=m_nPartCount || !m_abyPartStatus) ? 0 : m_abyPartStatus[iPart];
					}
	// Mighty Knife: Community visualization
	bool			IsCommunity() const;
	// [end] Mighty Knife

	uint8*			GetPartStatus() const							{ return m_abyPartStatus; }
	uint16			GetPartCount() const							{ return m_nPartCount; }
	uint32			GetDownloadDatarate() const						{ return m_nDownDatarate; }
	uint16			GetRemoteQueueRank() const						{ return m_nRemoteQueueRank; }
	void			SetRemoteQueueRank(uint16 nr);
	bool			IsRemoteQueueFull() const						{ return m_bRemoteQueueFull; }
	void			SetRemoteQueueFull(bool flag)					{ m_bRemoteQueueFull = flag; }
	//Morph START - added by AndCycle, DiffQR
	int				GetDiffQR()					{return m_iDifferenceQueueRank;}
	//Morph END   - added by AndCycle, DiffQR
	//MORPH START - Added by SiRoB, Advanced A4AF derivated from Khaos
	/*
	void			DrawStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool  bFlat) const;
	*/
	void			DrawStatusBar(CDC* dc, LPCRECT rect, CPartFile* File, bool  bFlat) const;
	//MORPH END   - Added by SiRoB, Advanced A4AF derivated from Khaos
	bool			AskForDownload();
	virtual void	SendFileRequest();
	void			SendStartupLoadReq();
	void			ProcessFileInfo(CSafeMemFile* data, CPartFile* file);
	void			ProcessFileStatus(bool bUdpPacket, CSafeMemFile* data, CPartFile* file);
	void			ProcessHashSet(char* data, uint32 size);
	void			ProcessAcceptUpload();
	bool			AddRequestForAnotherFile(CPartFile* file);
	void			CreateBlockRequests(int iMaxBlocks);
	virtual void	SendBlockRequests();
	virtual bool	SendHttpBlockRequests();
	virtual void	ProcessBlockPacket(char* packet, uint32 size, bool packed = false);
	virtual void	ProcessHttpBlockPacket(const BYTE* pucData, UINT uSize);
	void			ClearDownloadBlockRequests();
	uint32			CalculateDownloadRate();
	UINT			GetAvailablePartCount() const;
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
	void			SetSourceFrom(ESourceFrom val) {m_nSourceFrom = val;}
	// -khaos--+++> Download Sessions Stuff
	void			SetDownStartTime()								{ m_dwDownStartTime = ::GetTickCount(); }
	uint32			GetDownTimeDifference() {
						uint32 myTime = m_dwDownStartTime; 
						m_dwDownStartTime = 0;
						return ::GetTickCount() - myTime;
					}
	bool			GetTransferredDownMini() const					{ return m_bTransferredDownMini; }
	void			SetTransferredDownMini()						{ m_bTransferredDownMini = true; }
	void			InitTransferredDownMini()						{ m_bTransferredDownMini = false; }
	uint16			GetA4AFCount() const							{ return m_OtherRequests_list.GetCount(); }

	uint16			GetUpCompleteSourcesCount() const				{ return m_nUpCompleteSourcesCount; }
	void			SetUpCompleteSourcesCount(uint16 n)				{ m_nUpCompleteSourcesCount = n; }

	//chat
	EChatState		GetChatState() const							{ return (EChatState)m_nChatstate; }
	void			SetChatState(EChatState nNewS)					{ m_nChatstate = nNewS; }
	//chat
	//KadIPCheck
	EKadState		GetKadState() const								{ return (EKadState)m_nKadState; }
	void			SetKadState(EKadState nNewS)					{ m_nKadState = nNewS; }

	//File Comment
	bool			HasFileComment() const						{ return !m_strFileComment.IsEmpty(); }
    const CString&	GetFileComment() const						{ return m_strFileComment; } 
    void			SetFileComment(LPCTSTR pszComment)			{ m_strFileComment = pszComment; }

	bool			HasFileRating() const						{ return m_uFileRating > 0; }
    uint8			GetFileRating() const						{ return m_uFileRating; }
    void			SetFileRating(uint8 uRating)				{ m_uFileRating = uRating; }

	// Barry - Process zip file as it arrives, don't need to wait until end of block
	int				unzip(Pending_Block_Struct *block, BYTE *zipped, uint32 lenZipped, BYTE **unzipped, uint32 *lenUnzipped, int iRecursion = 0);
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
	void			SetSpammer(bool bVal)							{ m_fIsSpammer = bVal ? 1 : 0; }
	bool			GetMessageFiltered() const						{ return m_fMessageFiltered; }
	void			SetMessageFiltered(bool bVal)					{ m_fMessageFiltered = bVal ? 1 : 0; }
	
	virtual void	SetRequestFile(CPartFile* pReqFile);
	CPartFile*		GetRequestFile() const						{return reqfile;}

	// AICH Stuff
	void			SetReqFileAICHHash(CAICHHash* val);
	CAICHHash*		GetReqFileAICHHash() const					{return m_pReqFileAICHHash;}
	bool			IsSupportingAICH() const					{return m_fSupportsAICH & 0x01;}
	void			SendAICHRequest(CPartFile* pForFile, uint16 nPart);
	bool			IsAICHReqPending() const					{return m_fAICHRequested; }
	void			ProcessAICHAnswer(char* packet, UINT size);
	void			ProcessAICHRequest(char* packet, UINT size);
	void			ProcessAICHFileHash(CSafeMemFile* data, CPartFile* file);

	EUtf8Str		GetUnicodeSupport() const;

	CString			GetDownloadStateDisplayString() const;
	CString			GetUploadStateDisplayString() const;

	LPCTSTR			DbgGetDownloadState() const;
	LPCTSTR			DbgGetUploadState() const;
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
	//MORPH START - Added by SiRoB, See chunk that we hide
	uint8*			m_abyUpPartStatusHidden;
	bool			m_bUpPartStatusHiddenBySOTN;
 	//MORPH END   - Added by SiRoB, See chunk that we hide
	CTypedPtrList<CPtrList, CPartFile*> m_OtherRequests_list;
	CTypedPtrList<CPtrList, CPartFile*> m_OtherNoNeeded_list;
	uint16			m_lastPartAsked;
	//MORPH START - Changed by SiRoB, ZZ LowID handling
	/*
	bool			m_bAddNextConnect;  // VQB Fix for LowID slots only on connection
	*/
	DWORD			m_dwWouldHaveGottenUploadSlotIfNotLowIdTick;  // VQB Fix for LowID slots only on connection
	//MORPH END   - Changed by SiRoB, ZZ LowID handling

	void SetSlotNumber(uint32 newValue) { m_slotNumber = newValue; }
	uint32 GetSlotNumber() const { return m_slotNumber; }
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
	bool SupportsOhcbSuppression() const {return (m_uWebCacheFlags & WC_FLAGS_NO_OHCBS);}
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
	uint16 GetNumberOfClientsBehindOurWebCacheHavingSameFileAndNeedingThisBlock(Pending_Block_Struct* pending); // Superlexx - COtN - it's getting better all the time...
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
	bool ProcessPeerCacheQuery(const char* packet, UINT size);
	bool ProcessPeerCacheAnswer(const char* packet, UINT size);
	bool ProcessPeerCacheAcknowledge(const char* packet, UINT size);
	void OnPeerCacheDownSocketClosed(int nErrorCode);
	bool OnPeerCacheDownSocketTimeout();

	bool ProcessPeerCacheDownHttpResponse(const CStringAArray& astrHeaders);
	bool ProcessPeerCacheDownHttpResponseBody(const BYTE* pucData, UINT uSize);
	bool ProcessPeerCacheUpHttpResponse(const CStringAArray& astrHeaders);
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
	//EastShare End - Added by AndCycle, PayBackFirst
	//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-0133
	bool			GetPowerShared() const;
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
	void	ProcessFileIncStatus(CSafeMemFile* data,uint32 size, bool readHash = false);
	uint32	GetIncompletePartVersion()	{return m_incompletepartVer;}
	bool	IsIncPartAvailable(uint16 iPart)	{return	( (iPart >= m_nPartCount) || (!m_abyIncPartStatus) )? 0:m_abyIncPartStatus[iPart];}
	bool	IsPendingListEmpty()      {return m_PendingBlocks_list.IsEmpty();}
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
	bool			m_bWebCacheSupport;
	// Superlexx - webcache
	int		m_WA_webCacheIndex;	// index of the webcache name
	uint16	m_WA_HTTPPort;		// remote webserver port
	uint32	m_uWebCacheDownloadId;	// we must attach this ID when sending HTTP download request to the remote client.
	uint32	m_uWebCacheUploadId;	// incoming HTTP requests are identified as WC-requests,
									// if the header contains this ID and there is a known client with same ID in downloading state.
									// used for client authorization, should be substituted by a HttpIdList? later
									// for efficiency reasons.
	// Superlexx end
    // MORPH END - Added by Commander, WebCache 1.2e
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
	void	CreateStandartPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF = true);
	void	CreatePackedPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF = true);
	
	uint32	m_nConnectIP;		// holds the supposed IP or (after we had a connection) the real IP
	uint32	m_dwUserIP;			// holds 0 (real IP not yet available) or the real IP (after we had a connection)
	uint32	m_dwServerIP;
	uint32	m_nUserIDHybrid;
	uint16	m_nUserPort;
	uint16	m_nServerPort;
	uint32	m_nClientVersion;
//--group to aligned int32
	uint8	m_byEmuleVersion;
	uint8	m_byDataCompVer;
	bool	m_bEmuleProtocol;
	bool	m_bIsHybrid;
//--group to aligned int32
	TCHAR*	m_pszUsername;
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
	bool	m_bHelloAnswerPending;
	uint8	m_byInfopacketsReceived;	// have we received the edonkeyprot and emuleprot packet already (see InfoPacketsReceived() )
	uint8	m_bySupportSecIdent;
//--group to aligned int32
	uint32	m_dwLastSignatureIP;
	CString m_strClientSoftware;
	CString	old_m_strClientSoftware; //MORPH - Added by IceCream, Antileecher feature
	CString         m_strModVersion;
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

	bool	m_bUnicodeSupport;
	bool	m_bBuddyIDValid;
	uint16	m_nBuddyPort;
	//--group to aligned int32
	uint32	m_nBuddyIP;
	uchar	m_achBuddyID[16];
	CString m_strHelloInfo;
	CString m_strMuleInfo;

	// states
#ifdef _DEBUG
	// use the 'Enums' only for debug builds, each enum costs 4 bytes (3 unused)
	EClientSoftware		m_clientSoft;
	EChatState	m_nChatstate;
	EKadState			m_nKadState;
	ESecureIdentState	m_SecureIdentState;
	EUploadState		m_nUploadState;
	EDownloadState		m_nDownloadState;
	ESourceFrom			m_nSourceFrom;
#else
	uint8 m_clientSoft;
	uint8 m_nChatstate;
	uint8 m_nKadState;
	uint8 m_SecureIdentState;
	uint8 m_nUploadState;
	uint8 m_nDownloadState;
	uint8 m_nSourceFrom;
#endif

	CTypedPtrList<CPtrList, Packet*> m_WaitingPackets_list;
	CList<PartFileStamp, PartFileStamp> m_DontSwap_list;

	uint32  AskTime; //MORPH - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	bool	m_bIsMorph; //MORPH - Added by SiRoB, Is Morph client?
	bool		m_bLeecher; //MORPH - Added by IceCream, anti-leecher feature

	////////////////////////////////////////////////////////////////////////
	// Upload
	//
    int	GetFilePrioAsNumber() const;

	uint32		m_nTransferedUp;
	uint32		m_dwUploadTime;
	uint32		m_nAvUpDatarate; //Wistily
	uint32		m_nUpTotalTime;//wistily total lenght of this client's uploads during this session in ms
	uint32		m_cAsked;
	uint32		m_dwLastUpRequest;
	uint32		m_nCurSessionUp;
    uint32      m_nCurQueueSessionUp;
	uint32      m_nCurQueueSessionPayloadUp;
	uint32      m_addedPayloadQueueSession;
    uint32      m_curSessionAmountNumber;
	uint16		m_nUpPartCount;
	uint16		m_nUpCompleteSourcesCount;
	uchar		requpfileid[16];
    uint32      m_slotNumber;

    DWORD       m_dwLastCheckedForEvictTick;

	typedef struct TransferredData {
		uint32	datalen;
		DWORD	timestamp;
	};
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_BlockRequests_queue;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_DoneBlocks_list;
	CTypedPtrList<CPtrList, Requested_File_Struct*>	 m_RequestedFiles_list;


	// Download
	//
	CPartFile*	reqfile;
	CAICHHash*  m_pReqFileAICHHash; 
	uint32		m_cDownAsked;
	uint8*		m_abyPartStatus;
	CString		m_strClientFilename;
	uint32		m_nTransferedDown;
	uint32		m_dwDownStartTime;
	uint32      m_nLastBlockOffset;
	uint32		m_dwLastBlockReceived;
	uint32		m_nTotalUDPPackets;
	uint32		m_nFailedUDPPackets;
	//--group to aligned int32
	uint16		m_cShowDR;
	uint16		m_nRemoteQueueRank;
	//MORPH START - Added by AndCycle, DiffQR
	int			m_iDifferenceQueueRank;
	//MORPH END  - Added by AndCycle, DiffQR
	//--group to aligned int32
	uint16		m_nPartCount;
	bool		m_bRemoteQueueFull;
	bool		m_bCompleteSource;
	//--group to aligned int32
	bool		m_bReaskPending;
	bool		m_bUDPPending;
	bool		m_bTransferredDownMini;


	CStringA	m_strUrlPath;
	UINT		m_uReqStart;
	UINT		m_uReqEnd;
	UINT		m_nUrlStartPos;


	//////////////////////////////////////////////////////////
	// Upload data rate computation
	//
	uint32		m_nUpDatarate;
	uint32		m_nSumForAvgUpDataRate;
	CList<TransferredData, TransferredData> m_AvarageUDR_list;

	//////////////////////////////////////////////////////////
	// Download data rate computation
	//
	uint32		m_nDownDatarate;
	uint32		m_nAvDownDatarate; //Wistily
	uint32		m_nDownTotalTime;// wistily total lenght of this client's downloads during this session in ms
	uint32		m_nDownDataRateMS;
	uint32		m_nSumForAvgDownDataRate;
	CList<TransferredData,TransferredData> m_AvarageDDR_list;
	
	//////////////////////////////////////////////////////////
	// GUI helpers
	
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
		 m_fAICHRequested     : 1; 

	CTypedPtrList<CPtrList, Pending_Block_Struct*>	 m_PendingBlocks_list;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_DownloadBlocks_list;

    bool    m_bSourceExchangeSwapped; // ZZ:DownloadManager
    DWORD   lastSwapForSourceExchangeTick; // ZZ:DownloadManaager
    bool    DoSwap(CPartFile* SwapTo, bool bRemoveCompletely, LPCTSTR reason); // ZZ:DownloadManager
    CMap<CPartFile*, CPartFile*, DWORD, DWORD> m_fileReaskTimes; // ZZ:DownloadManager (one resk timestamp for each file)
    DWORD   m_dwLastTriedToConnect; // ZZ:DownloadManager (one resk timestamp for each file)
    bool    RecentlySwappedForSourceExchange() { return ::GetTickCount()-lastSwapForSourceExchangeTick < 30*1000; } // ZZ:DownloadManager
    void    SetSwapForSourceExchangeTick() { lastSwapForSourceExchangeTick = ::GetTickCount(); } // ZZ:DownloadManager

	//MORPH START - Added by SiRoB, m_PartStatus_list
	CMap<CPartFile*, CPartFile*, uint8*, uint8*>	 m_PartStatus_list;
	CMap<CPartFile*, CPartFile*, uint8*, uint8*>	 m_IncPartStatus_list; //MORPH - Added by AndCycle, ICS, See A4AF PartStatus
	//MORPH END   - Added by SiRoB, m_PartStatus_list

//EastShare Start - added by AndCycle, IP to Country
public:
	CString			GetCountryName(bool longName = false) const;
	int				GetCountryFlagIndex() const;
	void			ResetIP2Country();
private:
	struct	IPRange_Struct2* m_structUserCountry; //EastShare - added by AndCycle, IP to Country
//EastShare End - added by AndCycle, IP to Country

//Morph Start - added by AndCycle, ICS
	// enkeyDEV: ICS
	uint32	m_incompletepartVer;
	uint8*	m_abyIncPartStatus;
	// <--- enkeyDEV: ICS
//Morph End - added by AndCycle, ICS

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
	_T("[Rumata (rus)(Plus v1f)]")							//14
	};

//<<< new tags from eMule 0.04x
#define CT_UNKNOWNx0			0x00 // Hybrid Horde protocol
#define CT_UNKNOWNx12			0x12 // http://www.haspepapa-welt.de (DodgeBoards)
#define CT_UNKNOWNx13			0x13 // http://www.haspepapa-welt.de (DodgeBoards)
#define CT_UNKNOWNx14			0x14 // http://www.haspepapa-welt.de (DodgeBoards)
#define CT_UNKNOWNx15			0x15 // http://www.haspepapa-welt.de (DodgeBoards) & DarkMule |eVorte|X|
#define CT_UNKNOWNx16			0x16 // http://www.haspepapa-welt.de (DodgeBoards)
#define CT_UNKNOWNx17			0x17 // http://www.haspepapa-welt.de (DodgeBoards)
#define CT_UNKNOWNx22			0x22 // DarkMule |eVorte|X|
#define CT_UNKNOWNx63			0x63 // ?
#define CT_UNKNOWNx64			0x64 // ?
#define CT_UNKNOWNx69			0x69 // eMuleReactor
#define CT_UNKNOWNx79			0x79 // Bionic
#define CT_UNKNOWNx88			0x88 // DarkMule v6 |eVorte|X|
#define CT_UNKNOWNx8c			0x8c // eMule v0.27c [LSD7c] 
#define CT_UNKNOWNx8d			0x8d // unknown Leecher - (client version:60)
#define CT_UNKNOWNx99			0x99 // eMule v0.26d [RAMMSTEIN 8b]
#define CT_UNKNOWNxbb			0xbb // emule.de (client version:60)
#define CT_UNKNOWNxc4			0xc4 //MD5 Community from new bionic - hello
#define CT_FRIENDSHARING		0x66 //eWombat  [SNAFU]
#define CT_DARK					0x54 //eWombat [SNAFU]
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