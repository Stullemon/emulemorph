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

#pragma once
#include "types.h"
#include "listensocket.h"
#include "partfile.h"
#include "clientcredits.h"
#include "safefile.h"
#include "BarShader.h"
#include "loggable.h"

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

struct PartFileStamp {
	CPartFile*	file;
	DWORD		timestamp;
};
class CClientReqSocket;
class CFriend;


class CUpDownClient: public CLoggable
{
	friend class CUploadQueue;

public:
	//base
	CUpDownClient(CClientReqSocket* sender = 0);
	CUpDownClient(CPartFile* in_reqfile, uint16 in_port, uint32 in_userid, uint32 in_serverup, uint16 in_serverport, bool ed2kID = false);
	~CUpDownClient();
	bool			Disconnected(CString reason = NULL,bool m_FromSocket = false); //MORPH - Changed by SiRoB, ZZ Upload system
	bool			TryToConnect(bool bIgnoreMaxCon = false);
	// khaos::concrash-
	void			ConnectionEstablished();
	uint32			GetUserIDHybrid()			{return m_nUserIDHybrid;}
	void			SetUserIDHybrid(uint32 val)	{m_nUserIDHybrid = val;}
	char*			GetUserName()				{return m_pszUsername;}
	uint32			GetIP()						{return m_dwUserIP;}
	bool			HasLowID()					{return IsLowIDHybrid(m_nUserIDHybrid);}
	char*			GetFullIP()					{return m_szFullUserIP;}
	uint16			GetUserPort()				{return m_nUserPort;}
	void			SetUserPort( uint16 val )	{m_nUserPort = val;}
	uint32			GetTransferedUp()			{return m_nTransferedUp;}
	uint32			GetTransferedDown()			{return m_nTransferedDown;}
	uint32			GetServerIP()				{return m_dwServerIP;}
	void			SetServerIP(uint32 nIP)		{m_dwServerIP = nIP;}
	uint16			GetServerPort()				{return m_nServerPort;}
	void			SetServerPort(uint16 nPort)	{m_nServerPort = nPort;}
	uchar*			GetUserHash()				{return (uchar*)m_achUserHash;}
	void			SetUserHash(uchar* m_achTempUserHash);
	bool			HasValidHash()				{return ((int*)m_achUserHash[0]) != 0 || ((int*)m_achUserHash[1]) != 0 ||
												        ((int*)m_achUserHash[2]) != 0 || ((int*)m_achUserHash[3]) != 0; }
	int				GetHashType();
	uint32			GetVersion()				{return m_nClientVersion;}
	uint32			GetMajVersion()				{return m_nClientMajVersion;}
	uint32			GetMinVersion()				{return m_nClientMinVersion;}
	uint32			GetUpVersion()				{return m_nClientUpVersion;}
	uint8			GetMuleVersion()			{return m_byEmuleVersion;}
	bool			ExtProtocolAvailable()		{return m_bEmuleProtocol;}
	bool			IsEmuleClient()				{return m_byEmuleVersion;}
	CClientCredits* Credits()					{return credits;}
	bool			IsBanned();
	bool			IsLeecher()					{return m_bLeecher;} //MORPH - Added by IceCream, Anti-leecher feature
	const CString&	GetClientFilename() const	{return m_strClientFilename;}
	bool			SupportsUDP()				{return m_byUDPVer != 0 && m_nUDPPort != 0;}
	uint16			GetUDPPort()				{return m_nUDPPort;}
	void			SetUDPPort(uint16 nPort)	{ m_nUDPPort = nPort; }
	uint16			GetKadPort()				{return m_nKadPort;}
	void			SetKadPort(uint16 nPort)	{ m_nKadPort = nPort; }
	uint8			GetUDPVersion()				{return m_byUDPVer;}
	uint8			GetExtendedRequestsVersion(){return m_byExtendedRequestsVer;}
	uint32			GetL2HACTime()				{return m_L2HAC_time ? (m_L2HAC_time - L2HAC_CALLBACK_PRECEDE) : 0;} //<<-- enkeyDEV(th1) -L2HAC-

	bool			IsFriend()					{return m_Friend != NULL;}
	float			GetCompression()	{return (float)compressiongain/notcompressed*100.0f;} // Add rod show compression
	void			ResetCompressionGain() {compressiongain = 0; notcompressed=1;} // Add show compression

	void			RequestSharedFileList();
	void			ProcessSharedFileList(char* pachPacket, uint32 nSize, LPCTSTR pszDirectory = NULL);
	// CString			GetUploadFileInfo(); Moved to WebServer.h
	
	void			SetUserName(char* pszNewName);
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
//	EClientSoftware	GetClientSoft()				{return m_clientSoft;}
//	void			ReGetClientSoft();
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
	void			ClearHelloProperties();
	bool			ProcessHelloAnswer(char* pachPacket, uint32 nSize);
	bool			ProcessHelloPacket(char* pachPacket, uint32 nSize);
	void			SendHelloAnswer();
	void			SendHelloPacket();
	void			SendMuleInfoPacket(bool bAnswer);
	void			ProcessMuleInfoPacket(char* pachPacket, uint32 nSize);
	void			ProcessMuleCommentPacket(char* pachPacket, uint32 nSize);
	bool			Compare(CUpDownClient* tocomp, bool bIgnoreUserhash = false);
	void			SetLastSrcReqTime()			{m_dwLastSourceRequest = ::GetTickCount();}
	void			SetLastSrcAnswerTime()		{m_dwLastSourceAnswer = ::GetTickCount();}
	void			SetLastAskedForSources()	{m_dwLastAskedForSources = ::GetTickCount();}
	uint32			GetLastSrcReqTime()			{return m_dwLastSourceRequest;}
	uint32			GetLastSrcAnswerTime()		{return m_dwLastSourceAnswer;}
	uint32			GetLastAskedForSources()	{return m_dwLastAskedForSources;}
	bool			GetFriendSlot();
	void			SetFriendSlot(bool bNV)		{m_bFriendSlot = bNV;}
	void			SetCommentDirty(bool bDirty = true) {m_bCommentDirty = bDirty;}
	uint8			GetSourceExchangeVersion()	{return m_bySourceExchangeVer;}
	
	uint32			GetAskTime()	{return AskTime;} //MORPH - Added by SiRoB - Smart Upload Control v2 (SUC) [lovelace]
	
	void			SendPublicKeyPacket();
	void			SendSignaturePacket();
	void			ProcessPublicKeyPacket(uchar* pachPacket, uint32 nSize);
	void			ProcessSignaturePacket(uchar* pachPacket, uint32 nSize);
	uint8			GetSecureIdentState()		{return m_SecureIdentState;}
	void			SendSecIdentStatePacket();
	void			ProcessSecIdentStatePacket(uchar* pachPacket, uint32 nSize);
	uint8			GetInfoPacketsReceived() { return m_byInfopacketsReceived; }
	void			InfoPacketsReceived();
	void			ResetFileStatusInfo();

	// preview
	void			SendPreviewRequest(CAbstractFile* pForFile);
	void			SendPreviewAnswer(CKnownFile* pForFile, CxImage** imgFrames, uint8 nCount);
	void			ProcessPreviewReq(char* pachPacket, uint32 nSize);
	void			ProcessPreviewAnswer(char* pachPacket, uint32 nSize);
	bool			SupportsPreview()		{return m_bSupportsPreview;}
	bool			SafeSendPacket(Packet* packet);

	CClientReqSocket*	socket;
	CClientCredits*		credits;
	CFriend*			m_Friend;
	//upload
	uint32	compressiongain; /// Add show compression
	uint32  notcompressed; // Add show compression
	EUploadState	GetUploadState()			{return m_nUploadState;}
	void			SetUploadState(EUploadState news);
	uint32			GetWaitStartTime();
	uint32			GetWaitTime()				{return m_dwUploadTime-GetWaitStartTime();}
	bool			IsDownloading()				{return (m_nUploadState == US_UPLOADING);}
	bool			HasBlocks()					{return !(m_BlockSend_queue.IsEmpty() && m_BlockRequests_queue.IsEmpty());}
	//MORPH START - Added by SiRoB, ZZ Upload System 20030723-0133
	sint32			GetDatarate()				{return m_nUpDatarate;}	
	//MORPH END - Added by SiRoB, ZZ Upload System 20030723-0133
	uint32			GetScore(bool sysvalue, bool isdownloading = false, bool onlybasevalue = false);
	void			AddReqBlock(Requested_Block_Struct* reqblock);
	bool			m_bAddNextConnect;  // VQB Fix for LowID slots only on connection
	//MORPH START - Added by SiRoB, ZZ Upload System
	void			CreateNextBlockPackage(bool startNextChunk = false);
	//MORPH END - Added by SiRoB, ZZ Upload System
	void 			SetUpStartTime() {m_dwUploadTime = ::GetTickCount();}
	uint32			GetUpStartTimeDelay()		{return ::GetTickCount() - m_dwUploadTime;}
	void 			SetWaitStartTime();
	void 			ClearWaitStartTime();
	void			SendHashsetPacket(char* forfileid);
	void			SetUploadFileID(uchar* tempreqfileid);
	uchar*			GetUploadFileID()	{return requpfileid;}
	//MORPH START - Added by SiRoB, ZZ Upload System
	uint32			SendBlockData();
	//MORPH END - Added by SiRoB, ZZ Upload System
	void			ClearUploadBlockRequests();
	void			SendRankingInfo();
	void			SendCommentInfo(CKnownFile *file);
	void			AddRequestCount(uchar* fileid);
	//MORPH START - Added by SiRoB, ZZ Upload System 20030723-0133
	bool			IsDifferentPartBlock(bool startNextChunk = false);
	//MORPH END - Added by SiRoB, ZZ Upload System 20030723-0133
	void			UnBan();
	void			Ban();
	void			BanLeecher(int log_message = true); //MORPH - Added by IceCream, Anti-leecher feature
	uint32			GetAskedCount()				{return m_cAsked;}
	void			AddAskedCount()				{m_cAsked++;}
	void			SetAskedCount( uint32 m_cInAsked)				{m_cAsked = m_cInAsked;}
	void			FlushSendBlocks();			// call this when you stop upload, or the socket might be not able to send
	void			SetLastUpRequest()			{m_dwLastUpRequest = ::GetTickCount();}
	uint32			GetLastUpRequest()			{return m_dwLastUpRequest;}
	// START enkeyDEV(th1) -L2HAC-
	void			SetLastL2HACExecution(uint32 m_set_to = 0)		{m_last_l2hac_exec = m_set_to ? m_set_to : ::GetTickCount();}
	uint32			GetLastL2HACExecution()		{return m_last_l2hac_exec;}
	// END enkeyDEV(th1) -L2HAC-
	void			UDPFileReasked();
	uint32			GetSessionUp()			{return m_nTransferedUp - m_nCurSessionUp;}
	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	void			ResetSessionUp()		{m_nCurSessionUp = m_nTransferedUp; m_addedPayloadQueueSession = GetQueueSessionPayloadUp(); } 
	uint32			GetQueueSessionUp()			{return m_nTransferedUp - m_nCurQueueSessionUp;}
	void			ResetQueueSessionUp()		{m_nCurQueueSessionUp = m_nTransferedUp; m_nCurQueueSessionPayloadUp = 0; m_curSessionAmountNumber = 0;} 
	uint32			GetQueueSessionPayloadUp()			{return m_nCurQueueSessionPayloadUp;}
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	void			ProcessUpFileStatus(char* packet,uint32 size);
	uint16			GetUpPartCount()			{return m_nUpPartCount;}
	void			DrawUpStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat);

	//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-0133
	bool            GetPowerShared();
	void            ClearUploadDoneBlocks();
	float           GetCombinedFilePrioAndCredit();
	//MORPH END - Added by SiRoB, ZZ Upload System 20030723-0133

	//download
	uint32			GetAskedCountDown()				{return m_cDownAsked;} //<<--
	void			AddAskedCountDown()				{m_cDownAsked++;}
	void			SetAskedCountDown( uint32 m_cInDownAsked)				{m_cDownAsked = m_cInDownAsked;}
	EDownloadState	GetDownloadState()			{return m_nDownloadState;}
	void			SetDownloadState(EDownloadState nNewState);
	uint32			GetLastAskedTime()			{return m_dwLastAskedTime;}
	bool			IsPartAvailable(uint16 iPart)	{return	( (iPart >= m_nPartCount) || (!m_abyPartStatus) )? 0:m_abyPartStatus[iPart];} 	
	bool			IsUpPartAvailable(uint16 iPart)	{return	( (iPart >= m_nUpPartCount) || (!m_abyUpPartStatus) )? 0:m_abyUpPartStatus[iPart];} 	
	uint8*			GetPartStatus()				{return m_abyPartStatus;}
	//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-01333
	uint8*			GetUpPartStatus()				{return m_abyUpPartStatus;}
	//MORPH END   - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-01333
	uint16			GetPartCount() const		{return m_nPartCount;}
	uint32			GetDownloadDatarate()		{return m_nDownDatarate;}
	uint16			GetRemoteQueueRank()		{return m_nRemoteQueueRank;}
	void			SetRemoteQueueFull( bool flag )	{m_bRemoteQueueFull = flag;}
	bool			IsRemoteQueueFull()			{return m_bRemoteQueueFull;}
	void			SetRemoteQueueRank(uint16 nr);
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
	uint32			CalculateDownloadRate();
	uint16			GetAvailablePartCount();
	bool			SwapToAnotherFile(bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile = NULL);
	void			DontSwapTo(CPartFile* file);
	bool			IsSwapSuspended(CPartFile* file);
	bool			DoSwap(CPartFile* SwapTo, bool anotherfile=false, int iDebugMode = 0);
	// khaos::kmod+ Smart A4AF Handling
	//bool			SwapToAnotherFile(bool bIgnoreNoNeeded = false, bool forceSmart = false);
	bool			BalanceA4AFSources(bool byPriorityOnly = false);
	bool			StackA4AFSources();
	bool			SwapToForcedA4AF();
	//void			SwapThisSource(CPartFile* pNewFile, bool bAddReqFile, int iDebugMode = 0);
	uint32			GetLastBalanceTick()		{return m_iLastSwapAttempt;}
	uint32			GetLastForceA4AFTick()	{ return m_iLastForceA4AFAttempt; }
	// khaos::kmod-
	void			UDPReaskACK(uint16 nNewQR);
	void			UDPReaskFNF();
	void			UDPReaskForDownload();
	bool			IsSourceRequestAllowed();
	// -khaos--+++> Download Sessions Stuff
	uint8			GetSourceFrom()				{return m_bySourceFrom;}
	void			SetSourceFrom(uint8 val)	{m_bySourceFrom=val;}
	//MORPH START - Changed by SiRoB, Better Download rate calcul
	//void			SetDownStartTime()			{m_dwDownStartTime = ::GetTickCount();}
	void			SetDownStartTime()			{m_dwDownStartTime = ::GetTickCount(); m_AvarageDDRlastRemovedHeadTimestamp = 0;}
	//MORPH END   - Changed by SiRoB, Better Download rate calcul
	uint32			GetDownTimeDifference()		{uint32 myTime = m_dwDownStartTime; m_dwDownStartTime = 0; return ::GetTickCount() - myTime;}
	bool			GetTransferredDownMini()	{return m_bTransferredDownMini;}
	void			SetTransferredDownMini()	{m_bTransferredDownMini=true;}
	void			InitTransferredDownMini()	{m_bTransferredDownMini=false;}
	//				A4AF Stats Stuff:
	//				In CPartFile::Process, I am going to keep a tally of how many clients
	//				in that PF's source list are A4AF for other files.  This tally is worthless
	//				to the PartFile that it belongs to, but when we add all of these counts up for
	//				each PartFile, we will get an accurate count of how many A4AF requests there are
	//				total.  This is for the Found Sources section of the tree.  This is a better, faster
	//				option than looping through the lists for unavailable sources.
	uint16			GetA4AFCount()				{return m_OtherRequests_list.GetCount();}
	// <-----khaos-

	uint16			GetUpCompleteSourcesCount()	{return m_nUpCompleteSourcesCount;}
	void			SetUpCompleteSourcesCount(uint16 n)	{m_nUpCompleteSourcesCount= n;}

	CPartFile*		reqfile;
	int				sourcesslot;

	//chat
	EChatState		GetChatState()				{return m_nChatstate;}
	void			SetChatState(EChatState nNewS)	{m_nChatstate = nNewS;}
	bool			m_bIsSpammer;
	uint8			m_cMessagesReceived;	// count of chatmessages he sent to me
	uint8			m_cMessagesSend;		// count of chatmessages I sent to him

	//KadIPCheck
	EKadIPCheckState GetKadIPCheckState()		{return m_nKadIPCheckState;}
	void			SetKadIPCHeckState(EKadIPCheckState nNewS)	{m_nKadIPCheckState = nNewS;}

	//File Comment 
    CString			GetFileComment()			{return m_strComment;} 
    void			SetFileComment(char *desc)	{m_strComment.Format("%s",desc);}
    uint8			GetFileRate()				{return m_iRate;}
    void			SetFileRate(int8 iNewRate)	{m_iRate=iNewRate;}

	// Barry - Process zip file as it arrives, don't need to wait until end of block
	int unzip(Pending_Block_Struct *block, BYTE *zipped, uint32 lenZipped, BYTE **unzipped, uint32 *lenUnzipped, bool recursive = false);
	// Barry - Sets string to show parts downloading, eg NNNYNNNNYYNYN
	void ShowDownloadingParts(CString *partsYN);
	void UpdateDisplayedInfo(boolean force=false);
    int             GetFileListRequested() { return m_iFileListRequested; }
    void            SetFileListRequested(int iFileListRequested) { m_iFileListRequested = iFileListRequested; }
	bool			m_bMsgFiltered;

	//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-01333
	void SetSlotNumber(uint32 newValue) { m_slotNumber = newValue; }
	uint32 GetSlotNumber() { return m_slotNumber; }
	//MORPH END - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-01333

	//MORPH START - Added by SiRoB, Is Morph Client
	bool IsMorph() { return m_bIsMorph;}
	//MORPH END   - Added by SiRoB, Is Morph Client

	bool	MoreUpThanDown();	//EastShare - Added by AndCycle, PayBackFirst
	void	setPayBackFirstTag(bool tag) {m_bPayBackFirstTag=tag;}	//EastShare - Added by AndCycle, PayBackFirst
	bool	chkPayBackFirstTag() {return m_bPayBackFirstTag;} //EastShare - Added by AndCycle, PayBackFirst

	//wistily start
	void  Add2DownTotalTime(uint32 length){m_nDownTotalTime += length;}//wistily
	void  Add2UpTotalTime(uint32 length){m_nUpTotalTime += length;}//wistily
	uint32  GetDownTotalTime()  {return m_nDownTotalTime;}//wistily
	uint32  GetAvDownDatarate()  {return m_nAvDownDatarate;}//wistily
	uint32  GetUpTotalTime()  {return m_nUpTotalTime;}//wistily
	uint32  GetAvUpDatarate()  {return m_nAvUpDatarate;}//wistily
	//wistily stop
private:
	// base
	void	Init();
	bool	ProcessHelloTypePacket(CSafeMemFile* data);
	void	SendHelloTypePacket(CMemFile* data);
	uint32	m_dwUserIP;
	uint32	m_dwServerIP;
	uint32	m_nUserIDHybrid;
	uint16	m_nUserPort;
	uint16	m_nServerPort;
	uint32	m_nClientVersion;
	uint32	m_nClientMajVersion;
	uint32	m_nClientMinVersion;
	uint32	m_nClientUpVersion;
	uint32	m_nUpDatarate;
	uint32	dataratems;
	uint32	m_cSendblock;
	uint8	m_byEmuleVersion;
	uint8	m_byDataCompVer;
	bool	m_bEmuleProtocol;
	char*	m_pszUsername;
	char	m_szFullUserIP[21];
	char	m_achUserHash[16];
	uint16	m_nUDPPort;
	uint8	m_byUDPVer;
	uint16	m_nKadPort;
	uint8	m_bySourceExchangeVer;
	uint8	m_byAcceptCommentVer;
	uint8	m_byExtendedRequestsVer;
	uint8	m_cFailed;
//	EClientSoftware m_clientSoft; //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-

	uint32	m_dwLastSourceRequest;
	uint32	m_dwLastSourceAnswer;
	uint32	m_dwLastAskedForSources;
    int     m_iFileListRequested;
	bool	m_bFriendSlot;
	bool	m_bCommentDirty;
	bool	m_bIsHybrid;
	bool	m_bIsML;
	// preview
	bool	m_bSupportsPreview;
	bool	m_bPreviewReqPending;
	bool	m_bPreviewAnsPending;

	uint8	m_byCompatibleClient;
	CTypedPtrList<CPtrList, Packet*>				 m_WaitingPackets_list;
	CList<PartFileStamp, PartFileStamp>				 m_DontSwap_list;
	DWORD	m_lastRefreshedDLDisplay;
    
	uint32  AskTime; //MORPH - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	bool	m_bIsMorph; //MORPH - Added by SiRoB, Is Morph client?
	bool	m_bIsEastShare; // EastShare - Added by Pretender, Is EastShare Cilent?

	uint32	m_L2HAC_time;			//<<-- enkeyDEV(th1) -L2HAC-


	ESecureIdentState	m_SecureIdentState;
	uint8	m_byInfopacketsReceived;			// have we received the edonkeyprot and emuleprot packet already (see InfoPacketsReceived() )
	uint32	m_dwLastSignatureIP;
	uint8	m_bySupportSecIdent;


	//upload
	// -khaos--+++> Added parameters: bool bFromPF = true
	//MORPH START - Changed by SiRoB, ZZ UPload system 20030818-1923
	uint64 CreateStandartPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF = true);
	uint64 CreatePackedPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF = true);
	//MORPH END   - Changed by SiRoB, ZZ UPload system 20030818-1923
	// <-----khaos-
	//MORPH START - Added by SiRoB, ZZ Upload System
	int CUpDownClient::GetFilePrioAsNumber();
	//MORPH END - Added by SiRoB, ZZ Upload System
	bool		m_bLeecher; //MORPH - Added by IceCream, anti-leecher feature
	char*		old_m_pszUsername; //MORPH - Added by IceCream, Antileecher feature
	CString		old_m_clientVerString; //MORPH - Added by IceCream, Antileecher feature
	uint32		m_nTransferedUp;
	EUploadState m_nUploadState;
	uint32		m_dwWaitTime;
	uint32		m_dwUploadTime;
	uint32		m_nMaxSendAllowed;
	uint32		m_nAvUpDatarate;
	uint32		m_cAsked;
	uint32		m_dwLastUpRequest;
	uint32		m_last_l2hac_exec; //<<-- enkeyDEV(th1) -L2HAC-
	bool		m_bUsedComprUp;	//only used for interface output
	uint32		m_nCurSessionUp;
	uint32      m_nCurQueueSessionUp;
	uint32      m_nCurQueueSessionPayloadUp;
	uint32      m_addedPayloadQueueSession;
	uint32      m_curSessionAmountNumber;
	uint16		m_nUpPartCount;
	uint16		m_nUpCompleteSourcesCount;
	static		CBarShader s_UpStatusBar;
	uchar		requpfileid[16];
	//MORPH START - Added by SiRoB, ZZ Upload System 20030807-1911
	uint16      m_currentPartNumber;
	bool        m_currentPartNumberIsKnown;
	uint32      m_slotNumber;

	DWORD       m_dwLastCheckedForEvictTick;
//MORPH END - Added by SiRoB, ZZ Upload System 20030807-1911
	bool		m_bPayBackFirstTag; //EastShare - added by AndCycle, Pay Back First
public:
	uint16		m_lastPartAsked;
	uint8*		m_abyUpPartStatus;
	uint32		m_nSumForAvgUpDataRate;
	CTypedPtrList<CPtrList, CPartFile*>				 m_OtherRequests_list;
	CTypedPtrList<CPtrList, CPartFile*>				 m_OtherNoNeeded_list;
	bool		TestLeecher(); //MORPH - Added by IceCream, anti-leecher feature
private:
	CList<TransferredData,TransferredData>			 m_AvarageUDR_list; // By BadWolf
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	DWORD		m_lastRefreshedULDisplay;
	CTypedPtrList<CPtrList, Packet*>				 m_BlockSend_queue;
	//MORPH END - Added by Yun.SF3, ZZ Upload System
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_BlockRequests_queue;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_DoneBlocks_list;
	CTypedPtrList<CPtrList, Requested_File_Struct*>	 m_RequestedFiles_list;
	bool		m_l2hac_enabled; //<<-- enkeyDEV(th1) -L2HAC- lowid side
	//download
    

	bool		m_bRemoteQueueFull;
	bool		usedcompressiondown; //only used for interface output
	EDownloadState m_nDownloadState;
	uint16		m_nPartCount;
	uint32		m_cDownAsked;
	uint8*		m_abyPartStatus;
	uint32		m_dwLastAskedTime;
	CString		m_strClientFilename;
	uint32		m_nTransferedDown;
	// -khaos--+++> Download Session Stats
	bool		m_bTransferredDownMini;
	uint32		m_dwDownStartTime;
	// <-----khaos-
	uint32      m_nLastBlockOffset;   // Patch for show parts that you download [Cax2]
	uint32		m_nDownDatarate;
	uint32		m_nDownDataRateMS;
	uint32		m_nAvDownDatarate;
	uint16		m_cShowDR;
	uint32		m_dwLastBlockReceived;
	uint16		m_nRemoteQueueRank;
	bool		m_bCompleteSource;
	bool		m_bReaskPending;
	bool		m_bUDPPending;
	uint32		m_nSumForAvgDownDataRate;
	uint8		m_bySourceFrom;
	// khaos::kmod+
	uint32		m_iLastSwapAttempt;
	uint32		m_iLastActualSwap;
	uint32		m_iLastForceA4AFAttempt;
	CString		m_sModIdent;
	CString		m_sClientVersion;
	CMap<CPartFile*, CPartFile*, uint8*, uint8*>	 m_PartStatus_list;
	// khaos::kmod-
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	uint32		m_random_update_wait;
	//MORPH END - Added by Yun.SF3, ZZ Upload System
	// By BadWolf - Accurate Speed Measurement (Ottavio84 idea)
	CList<TransferredData,TransferredData>			 m_AvarageDDR_list;
	//MORPH START - Added by SiRoB, Better Download Speed calcul
	uint32	m_AvarageDDRlastRemovedHeadTimestamp;
	//MORPH END   - Added by SiRoB, Better Download Speed calcul
	sint32	sumavgDDR;
	sint32	sumavgUDR;
	// END By BadWolf - Accurate Speed Measurement (Ottavio84 idea)

	CTypedPtrList<CPtrList, Pending_Block_Struct*>	 m_PendingBlocks_list;
	CTypedPtrList<CPtrList, Requested_Block_Struct*> m_DownloadBlocks_list;
	
	uint32 m_nDownTotalTime;// wistily total lenght of this client's downloads during this session in ms
	uint32 m_nUpTotalTime;//wistily total lenght of this client's uploads during this session in ms

	static CBarShader s_StatusBar;
	// chat
	EChatState	m_nChatstate;
	CString		m_strComment;
	int8		m_iRate; 

	// KadIPCheck
	EKadIPCheckState m_nKadIPCheckState;

	// using bitfield for less important flags, to save some bytes
	UINT m_fHashsetRequesting : 1, // we have sent a hashset request to this client
		 m_fSharedDirectories : 1; // client supports OP_ASKSHAREDIRS opcodes
 //MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
// Maella -Support for tag ET_MOD_VERSION 0x55-
private:
	EClientSoftware m_clientSoft;
	CString         m_clientModString;
	CString         m_clientVerString;

public:
	void            ReGetClientSoft();
	EClientSoftware GetClientSoft() const { return m_clientSoft; }
	const CString&  GetClientModString() const { return m_clientModString; }
	const CString&  GetClientVerString() const { return m_clientVerString; }
// Maella end
	//MORPH END - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	uint16 m_iPriorPartNumber;
	bool m_bHasPriorPart;
	//MORPH END - Added by Yun.SF3, ZZ Upload System
};

//MORPH START - Added by IceCream, xrmb's Hashthief protection
	//--- xrmb:hashthieves1 ---
	typedef CMap <uint64, uint64, uint32, uint32> t_offensecounter;
	extern t_offensecounter offensecounter;
	typedef CMap <uint64, uint64, uint64, uint64> t_hashbase;
	extern t_hashbase hashbase;
	//--- :xrmb ---
//MORPH END   - Added by IceCream, xrmb's Hashthief protection
