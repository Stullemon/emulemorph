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
#ifndef __UP_DOWN_CLIENT_H__
#define	__UP_DOWN_CLIENT_H__
#pragma once
#include "loggable.h"
#include "BarShader.h"

class CClientReqSocket;
class CFriend;
class CPartFile;
class CClientCredits;
class CAbstractFile;
class CKnownFile;
class Packet;
class CxImage;
struct Requested_Block_Struct;
class CSafeMemFile;

struct Pending_Block_Struct{
	Requested_Block_Struct*	block;
	struct z_stream_s*      zStream;       // Barry - Used to unzip packets
	uint32                  totalUnzipped; // Barry - This holds the total unzipped bytes for all packets so far
	UINT					fZStreamError : 1,
							fRecovered    : 1;
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
	DS_REQHASHSET,
	DS_NONEEDEDPARTS,
	DS_TOOMANYCONNS,
	DS_LOWTOLOWIP,
	DS_BANNED,
	DS_ERROR,
	DS_NONE,
	DS_REMOTEQUEUEFULL  // not used yet, except in statistics
};

enum EChatState{
	MS_NONE,
	MS_CHATTING,
	MS_CONNECTING,
	MS_UNABLETOCONNECT
};

enum EKadIPCheckState{
	KS_NONE,
	KS_QUEUED,
	KS_CONNECTING,
	KS_CONNECTED
};

enum EClientSoftware{
	SO_EMULE			= 0,
	SO_CDONKEY			= 1,
	SO_XMULE			= 2,
	SO_SHAREAZA			= 4,
	SO_EDONKEYHYBRID	= 50,
	SO_EDONKEY			= 51,
	SO_MLDONKEY			= 52,
	SO_OLDEMULE			= 53,
	SO_UNKNOWN			= 54
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
	SF_SLS				= 4 //MORPH - Added by SiRoB, Save Load Sources (SLS)
};

struct PartFileStamp {
	CPartFile*	file;
	DWORD		timestamp;
};

//#pragma pack(2)
class CUpDownClient: public CLoggable
#ifdef _DEBUG
					,public CObject
#endif
{
	friend class CUploadQueue;
public:
	//base
	CUpDownClient(CClientReqSocket* sender = 0);
	CUpDownClient(CPartFile* in_reqfile, uint16 in_port, uint32 in_userid, uint32 in_serverup, uint16 in_serverport, bool ed2kID = false);
	~CUpDownClient();

	bool			Disconnected(bool bFromSocket = false);
	bool			TryToConnect(bool bIgnoreMaxCon = false);
	void			ConnectionEstablished();
	bool			CheckHandshakeFinished(UINT protocol, UINT opcode) const;

	uint32			GetUserIDHybrid() const		{return m_nUserIDHybrid;}
	void			SetUserIDHybrid(uint32 val)	{m_nUserIDHybrid = val;}

	LPCSTR			GetUserName() const			{return m_pszUsername;}
	void			SetUserName(LPCSTR pszNewName);

	uint32			GetIP() const				{return m_dwUserIP;}
	bool			HasLowID() const;
	LPCSTR			GetFullIP() const			{return m_szFullUserIP;}

	uint16			GetUserPort() const			{return m_nUserPort;}
	void			SetUserPort( uint16 val )	{m_nUserPort = val;}

	uint32			GetTransferedUp() const		{return m_nTransferedUp;}
	uint32			GetTransferedDown() const	{return m_nTransferedDown;}

	uint32			GetServerIP() const			{return m_dwServerIP;}
	void			SetServerIP(uint32 nIP)		{m_dwServerIP = nIP;}

	uint16			GetServerPort() const		{return m_nServerPort;}
	void			SetServerPort(uint16 nPort)	{m_nServerPort = nPort;}
	
	const uchar*	GetUserHash() const			{return (uchar*)m_achUserHash;}
	void			SetUserHash(const uchar* m_achTempUserHash);
	bool			HasValidHash() const		{return ((const int*)m_achUserHash[0]) != 0 || ((const int*)m_achUserHash[1]) != 0 || ((const int*)m_achUserHash[2]) != 0 || ((const int*)m_achUserHash[3]) != 0; }
	int				GetHashType() const;

	EClientSoftware	GetClientSoft() const		{return m_clientSoft;}
	const CString&	GetClientSoftVer() const	{return m_strClientSoftware;}
	const CString&	GetClientModVer() const		{return m_strModVersion;}
	void			ReGetClientSoft();
	uint32			GetVersion() const			{return m_nClientVersion;}
	uint8			GetMuleVersion() const		{return m_byEmuleVersion;}
	bool			ExtProtocolAvailable() const{return m_bEmuleProtocol;}
	bool			IsEmuleClient() const		{return m_byEmuleVersion;}
	uint8			GetSourceExchangeVersion() const {return m_bySourceExchangeVer;}
	CClientCredits* Credits() const				{return credits;}
	bool			IsBanned() const;
	const CString&	GetClientFilename() const	{return m_strClientFilename;}

	uint16			GetUDPPort() const			{return m_nUDPPort;}
	void			SetUDPPort(uint16 nPort)	{ m_nUDPPort = nPort; }
	uint8			GetUDPVersion() const		{return m_byUDPVer;}
	bool			SupportsUDP() const			{return GetUDPVersion() != 0 && m_nUDPPort != 0;}

	uint16			GetKadPort() const			{return m_nKadPort;}
	void			SetKadPort(uint16 nPort)	{ m_nKadPort = nPort; }

	uint8			GetExtendedRequestsVersion() const {return m_byExtendedRequestsVer;}

	void			RequestSharedFileList();
	void			ProcessSharedFileList(char* pachPacket, uint32 nSize, LPCTSTR pszDirectory = NULL);

	void			ClearHelloProperties();
	bool			ProcessHelloAnswer(char* pachPacket, uint32 nSize);
	bool			ProcessHelloPacket(char* pachPacket, uint32 nSize);
	void			SendHelloAnswer();
	bool			SendHelloPacket();
	void			SendMuleInfoPacket(bool bAnswer);
	void			ProcessMuleInfoPacket(char* pachPacket, uint32 nSize);
	void			ProcessMuleCommentPacket(char* pachPacket, uint32 nSize);
	bool			Compare(const CUpDownClient* tocomp, bool bIgnoreUserhash = false) const;
	void			ResetFileStatusInfo();

	uint32			GetLastSrcReqTime() const	{return m_dwLastSourceRequest;}
	void			SetLastSrcReqTime()			{m_dwLastSourceRequest = ::GetTickCount();}

	uint32			GetLastSrcAnswerTime() const{return m_dwLastSourceAnswer;}
	void			SetLastSrcAnswerTime()		{m_dwLastSourceAnswer = ::GetTickCount();}

	uint32			GetLastAskedForSources() const {return m_dwLastAskedForSources;}
	void			SetLastAskedForSources()	{m_dwLastAskedForSources = ::GetTickCount();}

	bool			GetFriendSlot() const;
	void			SetFriendSlot(bool bNV)		{m_bFriendSlot = bNV;}
	bool			IsFriend() const			{return m_Friend != NULL;}

	void			SetCommentDirty(bool bDirty = true) {m_bCommentDirty = bDirty;}

	bool			GetSentCancelTransfer() const {return m_fSentCancelTransfer;}
	void			SetSentCancelTransfer(bool bVal) {m_fSentCancelTransfer = bVal;}
	
	//MORPH START - Added by IceCream, Anti-leecher feature
	bool			IsLeecher()	const				{return m_bLeecher;}
	//MORPH END   - Added by IceCream, Anti-leecher feature
	uint32			GetL2HACTime(); //<<-- enkeyDEV(th1) -L2HAC-
	float			GetCompression() const	{return (float)compressiongain/notcompressed*100.0f;} // Add rod show compression
	void			ResetCompressionGain() {compressiongain = 0; notcompressed=1;} // Add show compression
	uint32			GetAskTime()	{return AskTime;} //MORPH - Added by SiRoB - Smart Upload Control v2 (SUC) [lovelace]

	// secure ident
	void			SendPublicKeyPacket();
	void			SendSignaturePacket();
	void			ProcessPublicKeyPacket(uchar* pachPacket, uint32 nSize);
	void			ProcessSignaturePacket(uchar* pachPacket, uint32 nSize);
	uint8			GetSecureIdentState() const	{return m_SecureIdentState;}
	void			SendSecIdentStatePacket();
	void			ProcessSecIdentStatePacket(uchar* pachPacket, uint32 nSize);
	uint8			GetInfoPacketsReceived() const {return m_byInfopacketsReceived;}
	void			InfoPacketsReceived();

	// preview
	void			SendPreviewRequest(const CAbstractFile* pForFile);
	void			SendPreviewAnswer(const CKnownFile* pForFile, CxImage** imgFrames, uint8 nCount);
	void			ProcessPreviewReq(char* pachPacket, uint32 nSize);
	void			ProcessPreviewAnswer(char* pachPacket, uint32 nSize);
	bool			SupportsPreview() const		{return m_bSupportsPreview;}
	bool			SafeSendPacket(Packet* packet);

	//upload
	EUploadState	GetUploadState() const		{return m_nUploadState;}
	void			SetUploadState(EUploadState news);

	uint32			GetWaitStartTime() const;
	void 			SetWaitStartTime();
	void 			ClearWaitStartTime();

	uint32			GetWaitTime() const			{return m_dwUploadTime - GetWaitStartTime();}

	bool			IsDownloading() const		{return (m_nUploadState == US_UPLOADING);}
	bool			HasBlocks() const			{return !m_BlockRequests_queue.IsEmpty();}
	//MORPH START - Changed by SiRoB, ZZ Upload System
	sint32			GetDatarate() const			{return m_nUpDatarate;}	
	//MORPH END - Changed by SiRoB, ZZ Upload System
	uint32			GetScore(bool sysvalue, bool isdownloading = false, bool onlybasevalue = false) const;

	void			AddReqBlock(Requested_Block_Struct* reqblock);
	void			CreateNextBlockPackage();

	uint32			GetUpStartTimeDelay() const	{return ::GetTickCount() - m_dwUploadTime;}
	void 			SetUpStartTime() {m_dwUploadTime = ::GetTickCount();}

	void			SendHashsetPacket(char* forfileid);
	
	const uchar*	GetUploadFileID() const		{return requpfileid;}
	void			SetUploadFileID(const uchar* tempreqfileid);

	uint32			SendBlockData();

	void			ClearUploadBlockRequests();
	void			SendRankingInfo();
	void			SendCommentInfo(/*const*/ CKnownFile *file);
	void			AddRequestCount(const uchar* fileid);

	void			UnBan();
	void			Ban();
	void			BanLeecher(int log_message = true); //MORPH - Added by IceCream, Anti-leecher feature

	uint32			GetAskedCount() const		{return m_cAsked;}
	void			AddAskedCount()				{m_cAsked++;}
	void			SetAskedCount( uint32 m_cInAsked)				{m_cAsked = m_cInAsked;}
	void			FlushSendBlocks();			// call this when you stop upload, or the socket might be not able to send

	uint32			GetLastUpRequest() const	{return m_dwLastUpRequest;}
	void			SetLastUpRequest()			{m_dwLastUpRequest = ::GetTickCount();}
	
	uint32			GetSessionUp() const			{return m_nTransferedUp - m_nCurSessionUp;}
	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	void			ResetSessionUp()		{m_nCurSessionUp = m_nTransferedUp; m_addedPayloadQueueSession = GetQueueSessionPayloadUp(); } 
	uint32			GetQueueSessionUp()	const		{return m_nTransferedUp - m_nCurQueueSessionUp;}
	void			ResetQueueSessionUp()		{m_nCurQueueSessionUp = m_nTransferedUp; m_nCurQueueSessionPayloadUp = 0; m_curSessionAmountNumber = 0;} 
	uint32			GetQueueSessionPayloadUp() const			{return m_nCurQueueSessionPayloadUp;}
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923

	void			ProcessUpFileStatus(char* packet,uint32 size);
	uint16			GetUpPartCount() const			{return m_nUpPartCount;}

	void			DrawUpStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat) const;
	uint32          GetPayloadInBuffer() const {return m_addedPayloadQueueSession-GetQueueSessionPayloadUp(); }

	//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-0133
	bool			GetPowerShared() const;
	void			ClearUploadDoneBlocks();
	double			GetCombinedFilePrioAndCredit();
	//MORPH END - Added by SiRoB, ZZ Upload System 20030723-0133
	// START enkeyDEV(th1) -L2HAC-
	void			SetLastL2HACExecution(uint32 m_set_to = 0)		{m_last_l2hac_exec = m_set_to ? m_set_to : ::GetTickCount();}
	uint32			GetLastL2HACExecution()		{return m_last_l2hac_exec;}
	// END enkeyDEV(th1) -L2HAC-

	//download
	uint32			GetAskedCountDown() const	{return m_cDownAsked;}
	void			AddAskedCountDown()				{m_cDownAsked++;}
	void			SetAskedCountDown( uint32 m_cInDownAsked)				{m_cDownAsked = m_cInDownAsked;}
	EDownloadState	GetDownloadState() const			{return m_nDownloadState;}
	void			SetDownloadState(EDownloadState nNewState);
	uint32			GetLastAskedTime()	const		{return m_dwLastAskedTime;}
	bool			IsPartAvailable(uint16 iPart) const	{return	( (iPart >= m_nPartCount) || (!m_abyPartStatus) )? 0:m_abyPartStatus[iPart];} 	
	bool			IsUpPartAvailable(uint16 iPart) const	{return	( (iPart >= m_nUpPartCount) || (!m_abyUpPartStatus) )? 0:m_abyUpPartStatus[iPart];} 	
	uint8*			GetPartStatus()	const			{return m_abyPartStatus;}
	uint16			GetPartCount() const		{return m_nPartCount;}
	uint32			GetDownloadDatarate() const	{return m_nDownDatarate;}

	uint16			GetRemoteQueueRank() const	{return m_nRemoteQueueRank;}
	void			SetRemoteQueueRank(uint16 nr);
	bool			IsRemoteQueueFull() const	{return m_bRemoteQueueFull;}
	void			SetRemoteQueueFull( bool flag )	{m_bRemoteQueueFull = flag;}
	int				GetDiffQR()					{return m_iDifferenceQueueRank;}	//Morph - added by AndCycle, DiffQR

	//MORPH START - Added by SiRoB, Advanced A4AF derivated from Khaos
	//void			DrawStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat);
	void			DrawStatusBar(CDC* dc, RECT* rect, CPartFile* File, bool  bFlat);
	//MORPH END   - Added by SiRoB, Advanced A4AF derivated from Khaos
	void			EnableL2HAC()				{m_l2hac_enabled = true;} //<<-- enkeyDEV(th1) -L2HAC- lowid side
	void			DisableL2HAC()				{m_l2hac_enabled = false;} //<<-- enkeyDEV(th1) -L2HAC- lowid side
	bool			IsL2HACEnabled()			{return m_l2hac_enabled;} //<<-- enkeyDEV(th1) -L2HAC- lowid side
	void			AskForDownload();
	void			SendFileRequest();
	void			SendStartupLoadReq();
	void			ProcessFileInfo(char* packet,uint32 size);
	void			ProcessFileStatus(char* packet,uint32 size);
	void			ProcessHashSet(char* packet,uint32 size);
	bool			AddRequestForAnotherFile(CPartFile* file);
	void			SendBlockRequests();
	void			ProcessBlockPacket(char* packet, uint32 size, bool packed = false);
	void			ClearDownloadBlockRequests();
	uint32			CalculateDownloadRate();
	uint16			GetAvailablePartCount() const;

	bool			SwapToAnotherFile(bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile = NULL);
	void			DontSwapTo(/*const*/ CPartFile* file);
	bool			IsSwapSuspended(const CPartFile* file) /*const*/;
	//MORPH - Changed by SiRoB, khaos::kmod+ Smart A4AF Handling
	bool			DoSwap(CPartFile* SwapTo, bool anotherfile = false, int iDebugMode = 0);
	bool			BalanceA4AFSources(bool byPriorityOnly = false);
	bool			StackA4AFSources();
	bool			SwapToForcedA4AF();
	uint32			GetLastBalanceTick()		{return m_iLastSwapAttempt;}
	uint32			GetLastForceA4AFTick()	{ return m_iLastForceA4AFAttempt; }
	//MORPH - Changed by SiRoB, khaos::kmod-

	void			UDPFileReasked();
	void			UDPReaskACK(uint16 nNewQR);
	void			UDPReaskFNF();
	void			UDPReaskForDownload();

	bool			IsSourceRequestAllowed() const;
	bool			IsValidSource() const;

	ESourceFrom		GetSourceFrom() const		{return m_nSourceFrom;}
	void			SetSourceFrom(ESourceFrom val) {m_nSourceFrom = val;}

	// -khaos--+++> Download Sessions Stuff
	//MORPH START - Changed by SiRoB, Better Download rate calcul
	//void			SetDownStartTime()			{m_dwDownStartTime = ::GetTickCount();}
	void			SetDownStartTime()			{m_dwDownStartTime = ::GetTickCount(); m_AvarageDDRlastRemovedHeadTimestamp = 0;}
	//MORPH END   - Changed by SiRoB, Better Download rate calcul
	uint32			GetDownTimeDifference()		{uint32 myTime = m_dwDownStartTime; m_dwDownStartTime = 0; return ::GetTickCount() - myTime;}

	bool			GetTransferredDownMini() const {return m_bTransferredDownMini;}
	void			SetTransferredDownMini()	{m_bTransferredDownMini=true;}
	void			InitTransferredDownMini()	{m_bTransferredDownMini=false;}
	//				A4AF Stats Stuff:
	//				In CPartFile::Process, I am going to keep a tally of how many clients
	//				in that PF's source list are A4AF for other files.  This tally is worthless
	//				to the PartFile that it belongs to, but when we add all of these counts up for
	//				each PartFile, we will get an accurate count of how many A4AF requests there are
	//				total.  This is for the Found Sources section of the tree.  This is a better, faster
	//				option than looping through the lists for unavailable sources.
	uint16			GetA4AFCount() const		{return m_OtherRequests_list.GetCount();}
	// <-----khaos-

	uint16			GetUpCompleteSourcesCount() const {return m_nUpCompleteSourcesCount;}
	void			SetUpCompleteSourcesCount(uint16 n)	{m_nUpCompleteSourcesCount= n;}

	//chat
	EChatState		GetChatState() const		{return m_nChatstate;}
	void			SetChatState(EChatState nNewS)	{m_nChatstate = nNewS;}

	//KadIPCheck
	EKadIPCheckState GetKadIPCheckState() const	{return m_nKadIPCheckState;}
	void			SetKadIPCHeckState(EKadIPCheckState nNewS)	{m_nKadIPCheckState = nNewS;}

	//File Comment 
    const CString&	GetFileComment() const		{return m_strComment;} 
    void			SetFileComment(LPCSTR desc)	{m_strComment = desc;}

    uint8			GetFileRate() const			{return m_iRate;}
    void			SetFileRate(int8 iNewRate)	{m_iRate=iNewRate;}

	// Barry - Process zip file as it arrives, don't need to wait until end of block
	int				unzip(Pending_Block_Struct *block, BYTE *zipped, uint32 lenZipped, BYTE **unzipped, uint32 *lenUnzipped, int iRecursion = 0);
	// Barry - Sets string to show parts downloading, eg NNNYNNNNYYNYN
	void			ShowDownloadingParts(CString* partsYN) const;
	void			UpdateDisplayedInfo(bool force = false);

	int             GetFileListRequested() const{return m_iFileListRequested;}
    void            SetFileListRequested(int iFileListRequested) { m_iFileListRequested = iFileListRequested; }

	LPCTSTR			DbgGetDownloadState() const;
	LPCTSTR			DbgGetUploadState() const;
	CString			DbgGetClientInfo(bool bFormatIP = false) const;
	CString			DbgGetFullClientSoftVer() const;

#ifdef _DEBUG
	// Diagnostic Support
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	CClientReqSocket* socket;
	CClientCredits*	credits;
	CFriend*		m_Friend;
	CPartFile*		reqfile;
	uint32			compressiongain; /// Add show compression
	uint32			notcompressed; // Add show compression
	uint8*			m_abyUpPartStatus;
	CTypedPtrList<CPtrList, CPartFile*> m_OtherRequests_list;
	CTypedPtrList<CPtrList, CPartFile*> m_OtherNoNeeded_list;
	uint16			m_lastPartAsked;
	bool			m_bAddNextConnect;  // VQB Fix for LowID slots only on connection
	bool			m_bIsSpammer;
	uint8			m_cMessagesReceived;	// count of chatmessages he sent to me
	uint8			m_cMessagesSend;		// count of chatmessages I sent to him
	bool			m_bMsgFiltered;

	//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-01333
	void SetSlotNumber(uint32 newValue) { m_slotNumber = newValue; }
	uint32 GetSlotNumber() const { return m_slotNumber; }
	//MORPH END - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-01333
	bool		TestLeecher(); //MORPH - Added by IceCream, anti-leecher feature
	//MORPH START - Added by SiRoB, Is Morph Client
	bool IsMorph() const { return m_bIsMorph;}
	//MORPH END   - Added by SiRoB, Is Morph Client

	//EastShare Start - Added by AndCycle, PayBackFirst
	bool	MoreUpThanDown() const;
	//EastShare End - Added by AndCycle, PayBackFirst

	//Morph - added by AndCycle, Equal Chance For Each File
	double	GetEqualChanceValue() const;

	//wistily start
	void  Add2DownTotalTime(uint32 length){m_nDownTotalTime += length;}//wistily
	void  Add2UpTotalTime(uint32 length){m_nUpTotalTime += length;}//wistily
	uint32  GetDownTotalTime() const  {return m_nDownTotalTime;}//wistily
	uint32  GetAvDownDatarate() const  {return m_nAvDownDatarate;}//wistily
	uint32  GetUpTotalTime() const  {return m_nUpTotalTime;}//wistily
	uint32  GetAvUpDatarate() const  {return m_nAvUpDatarate;}//wistily
	//wistily stop
private:
	// base
	void	Init();
	bool	ProcessHelloTypePacket(CSafeMemFile* data);
	void	SendHelloTypePacket(CMemFile* data);
	// -khaos--+++> Added parameters: bool bFromPF = true
	//MORPH START - Changed by SiRoB, ZZ UPload system 20030818-1923
	/*
	void	CreateStandartPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF = true);
	void	CreatePackedPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF = true);
	*/
	uint64 CreateStandartPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF = true);
	uint64 CreatePackedPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF = true);
	//MORPH END   - Changed by SiRoB, ZZ UPload system 20030818-1923
	// <-----khaos-
	
	
	uint32	m_dwUserIP;
	uint32	m_dwServerIP;
	uint32	m_nUserIDHybrid;
	uint16	m_nUserPort;
	uint16	m_nServerPort;
	uint32	m_nClientVersion;
	uint32	m_nUpDatarate;
	uint32	dataratems;
	uint8	m_byEmuleVersion;
	uint8	m_byDataCompVer;
	bool	m_bEmuleProtocol;
	bool	m_bIsHybrid;
	char*	m_pszUsername;
	char*	old_m_pszUsername; //MORPH - Added by IceCream, Antileecher feature
	char	m_szFullUserIP[3+1+3+1+3+1+3+1]; // 16
	char	m_achUserHash[16];
	uint16	m_nUDPPort;
	uint16	m_nKadPort;
	uint8	m_byUDPVer;
	uint8	m_bySourceExchangeVer;
	uint8	m_byAcceptCommentVer;
	uint8	m_byExtendedRequestsVer;
	uint8	m_byCompatibleClient;
	bool	m_bFriendSlot;
	bool	m_bCommentDirty;
	bool	m_bIsML;
	bool	m_bGPLEvildoer;
	bool	m_bHelloAnswerPending;
	EClientSoftware m_clientSoft;
	CString m_strClientSoftware;
	CString	old_m_strClientSoftware; //MORPH - Added by IceCream, Antileecher feature
	CString         m_strModVersion;
	uint32	m_dwLastSourceRequest;
	uint32	m_dwLastSourceAnswer;
	uint32	m_dwLastAskedForSources;
    int     m_iFileListRequested;
	CString	m_strComment;
	EChatState	m_nChatstate;
	EKadIPCheckState m_nKadIPCheckState;
	// preview
	bool	m_bSupportsPreview;
	bool	m_bPreviewReqPending;
	bool	m_bPreviewAnsPending;
	int8	m_iRate;

	CTypedPtrList<CPtrList, Packet*> m_WaitingPackets_list;
	CList<PartFileStamp, PartFileStamp> m_DontSwap_list;
	DWORD	m_lastRefreshedDLDisplay;
        DWORD	m_lastRefreshedULDisplay;

	uint32  AskTime; //MORPH - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	bool	m_bIsMorph; //MORPH - Added by SiRoB, Is Morph client?
	bool	m_bIsEastShare; // EastShare - Added by Pretender, Is EastShare Cilent?

	uint32	m_L2HAC_time;			//<<-- enkeyDEV(th1) -L2HAC-

	ESecureIdentState m_SecureIdentState;
	uint32	m_dwLastSignatureIP;
	uint8	m_byInfopacketsReceived;			// have we received the edonkeyprot and emuleprot packet already (see InfoPacketsReceived() )
	uint8	m_bySupportSecIdent;

	//upload
	//MORPH START - Added by SiRoB, ZZ Upload System
	int CUpDownClient::GetFilePrioAsNumber() const;
	//MORPH END - Added by SiRoB, ZZ Upload System
	bool		m_bLeecher; //MORPH - Added by IceCream, anti-leecher feature
	uint32		m_nTransferedUp;
	EUploadState m_nUploadState;
	uint32		m_dwUploadTime;
	uint32		m_nAvUpDatarate; //Wistily
	uint32		m_cAsked;
	uint32		m_dwLastUpRequest;
	uint32		m_last_l2hac_exec; //<<-- enkeyDEV(th1) -L2HAC-
	uint32		m_nCurSessionUp;
	uint32      m_nCurQueueSessionUp;
	uint32      m_nCurQueueSessionPayloadUp;
	uint32      m_addedPayloadQueueSession;
	uint32      m_curSessionAmountNumber;
	uint16		m_nUpPartCount;
	uint16		m_nUpCompleteSourcesCount;
	static CBarShader s_UpStatusBar;
	uchar		requpfileid[16];

	typedef struct TransferredData {
		uint32	datalen;
		DWORD	timestamp;
	};
	CList<TransferredData,TransferredData>			 m_AvarageUDR_list; // By BadWolf
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_BlockRequests_queue;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_DoneBlocks_list;
	CTypedPtrList<CPtrList, Requested_File_Struct*>	 m_RequestedFiles_list;
	bool		m_l2hac_enabled; //<<-- enkeyDEV(th1) -L2HAC- lowid side
	//MORPH START - Added by SiRoB, ZZ Upload System 20030807-1911
	uint32      m_slotNumber;
	DWORD       m_dwLastCheckedForEvictTick;
	//MORPH END - Added by SiRoB, ZZ Upload System 20030807-1911
	bool		m_bPayBackFirstTag; //EastShare - added by AndCycle, Pay Back First
	bool		m_bFullChunkTransferTag;//Morph - added by AndCycle, keep full chunk transfer

	//download
	EDownloadState m_nDownloadState;
	uint32		m_cDownAsked;
	uint8*		m_abyPartStatus;
	uint32		m_dwLastAskedTime;
	CString		m_strClientFilename;
	uint32		m_nTransferedDown;
	// -khaos--+++> Download Session Stats
	uint32		m_dwDownStartTime;
	// <-----khaos-
	uint32      m_nLastBlockOffset;   // Patch for show parts that you download [Cax2]
	uint32		m_nDownDatarate;
	uint32		m_nDownDataRateMS;
	uint32		m_nAvDownDatarate; //Wistily
	uint32		m_nSumForAvgDownDataRate;
	uint16		m_cShowDR;
	uint16		m_nRemoteQueueRank;
	int			m_iDifferenceQueueRank;	//Morph - added by AndCycle, DiffQR
	uint32		m_dwLastBlockReceived;
	ESourceFrom	m_nSourceFrom;
	uint16		m_nPartCount;
	bool		m_bRemoteQueueFull;
	bool		m_bCompleteSource;
	bool		m_bReaskPending;
	bool		m_bUDPPending;
	bool		m_bTransferredDownMini;
	static CBarShader s_StatusBar;

	// using bitfield for less important flags, to save some bytes
	UINT m_fHashsetRequesting : 1, // we have sent a hashset request to this client in the current connection
		 m_fSharedDirectories : 1, // client supports OP_ASKSHAREDIRS opcodes
		 m_fSentCancelTransfer: 1; // we have sent an OP_CANCELTRANSFER in the current connection

	// By BadWolf - Accurate Speed Measurement (Ottavio84 idea)
	CList<TransferredData,TransferredData>			 m_AvarageDDR_list;
	//MORPH START - Added by SiRoB, Better Download Speed calcul
	uint32	m_AvarageDDRlastRemovedHeadTimestamp;
	//MORPH END   - Added by SiRoB, Better Download Speed calcul
	sint32	sumavgUDR;
	// END By BadWolf - Accurate Speed Measurement (Ottavio84 idea)

	CTypedPtrList<CPtrList, Pending_Block_Struct*>	 m_PendingBlocks_list;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_DownloadBlocks_list;
	// khaos::kmod+
	uint32		m_iLastSwapAttempt;
	uint32		m_iLastActualSwap;
	uint32		m_iLastForceA4AFAttempt;
	CMap<CPartFile*, CPartFile*, uint8*, uint8*>	 m_PartStatus_list;
	// khaos::kmod-
	uint32 m_nDownTotalTime;// wistily total lenght of this client's downloads during this session in ms
	uint32 m_nUpTotalTime;//wistily total lenght of this client's uploads during this session in ms
};
//#pragma pack()

#endif//__UP_DOWN_CLIENT_H__
