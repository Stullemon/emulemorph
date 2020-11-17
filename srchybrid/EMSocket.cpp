//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "emsocket.h"
#include "AsyncProxySocketLayer.h"
#include "Packets.h"
#include "OtherFunctions.h"
#include "UploadBandwidthThrottler.h"
#include "Preferences.h"
#include "emuleDlg.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace {
	inline void EMTrace(char* fmt, ...) {
#ifdef EMSOCKET_DEBUG
		va_list argptr;
		char bufferline[512];
		va_start(argptr, fmt);
		_vsnprintf(bufferline, 512, fmt, argptr);
		va_end(argptr);
		//(Ornis+)
		char osDate[30],osTime[30]; 
		char temp[1024]; 
		_strtime( osTime );
		_strdate( osDate );
		int len = _snprintf(temp,1021,"%s %s: %s",osDate,osTime,bufferline);
		temp[len++] = 0x0d;
		temp[len++] = 0x0a;
		temp[len+1] = 0;
		HANDLE hFile = CreateFile("c:\\EMSocket.log",           // open MYFILE.TXT 
                GENERIC_WRITE,              // open for reading 
                FILE_SHARE_READ,           // share for reading 
                NULL,                      // no security 
                OPEN_ALWAYS,               // existing file only 
                FILE_ATTRIBUTE_NORMAL,     // normal file 
                NULL);                     // no attr. template 
  
		if (hFile != INVALID_HANDLE_VALUE) 
		{ 
			DWORD nbBytesWritten = 0;
			SetFilePointer(hFile, 0, NULL, FILE_END);
			BOOL b = WriteFile(
				hFile,                    // handle to file
				temp,                // data buffer
				len,     // number of bytes to write
				&nbBytesWritten,  // number of bytes written
				NULL        // overlapped buffer
			);
			CloseHandle(hFile);
		}
#else 
		//va_list argptr;
		//va_start(argptr, fmt);
		//va_end(argptr);
		UNREFERENCED_PARAMETER(fmt);
#endif //EMSOCKET_DEBUG
	}
}

IMPLEMENT_DYNAMIC(CEMSocket, CEncryptedStreamSocket)

CEMSocket::CEMSocket(void){
	byConnected = ES_NOTCONNECTED;
	m_uTimeOut = CONNECTION_TIMEOUT; // default timeout for ed2k sockets

	// Download (pseudo) rate control	
	downloadLimit = 0;
	downloadLimitEnable = false;
	pendingOnReceive = false;

	// Download partial header
	pendingHeaderSize = 0;

	// Download partial packet
	pendingPacket = NULL;
	pendingPacketSize = 0;

	// Upload control
#if !defined DONT_USE_SOCKET_BUFFERING
	sendblenWithoutControlPacket = 0;
	currentBufferSize = 0;
#endif
	sendbuffer = NULL;
	sendblen = 0;
	sent = 0;
	//m_bLinkedPackets = false;

	// deadlake PROXYSUPPORT
	m_pProxyLayer = NULL;
	m_bProxyConnectFailed = false;

    //m_startSendTick = 0;
    //m_lastSendLatency = 0;
    //m_latency_sum = 0;
    //m_wasBlocked = false;

#if !defined DONT_USE_SOCKET_BUFFERING
	m_currentPacket_is_controlpacketOverlapped = false;
	m_currentPackageIsFromPartFileOverlapped = false;
#else
    m_currentPacket_is_controlpacket = false;
	m_currentPackageIsFromPartFile = false;
#endif

    m_numberOfSentBytesCompleteFile = 0;
    m_numberOfSentBytesPartFile = 0;
    m_numberOfSentBytesControlPacket = 0;

    //MORPH - ZZUL
	/*
    lastCalledSend = ::GetTickCount();
    lastSent = ::GetTickCount()-1000;
	*/
	lastCalledSend = ::GetTickCount()-SEC2MS(1);
	lastFinishedStandard = 0;

	m_bAccelerateUpload = false;

#if !defined DONT_USE_SOCKET_BUFFERING
	m_actualPayloadSizeOverlapped = 0;
#else
    m_actualPayloadSize = 0;
    m_actualPayloadSizeSentForThisPacket = 0;
#endif
    m_actualPayloadSizeSent = 0;

    //MORPH - Changed by SiRoB, Show BusyTime
	/*
    m_bBusy = false;
    m_hasSent = false;
	*/
	DWORD curTick = GetTickCount();
	m_dwNotBusy = 0;
	m_dwNotBusyDelta = curTick-m_dwNotBusy;
	m_dwBusy = curTick;
	m_dwBusyDelta = 1;
	m_hasSent = false;

	m_bConnectionIsReadyForSend = false; //MORPH - ZZUL

#if defined DONT_USE_SOCKET_BUFFERING
	m_bUsesBigSendBuffers = false;
#else
	m_uCurrentSendBufferSize = 0;
	m_uCurrentRecvBufferSize = 0;
#endif
	m_pPendingSendOperation = NULL;
	// Overlapped sockets are under investigations for buggyness (heap corruption), only use for testing
	m_bOverlappedSending = thePrefs.GetUseOverlappedSockets() && theApp.IsWinSock2Available();
}

CEMSocket::~CEMSocket(){
	EMTrace("CEMSocket::~CEMSocket() on %d",(SOCKET)this);

    // need to be locked here to know that the other methods
    // won't be in the middle of things
    sendLocker.Lock();
	byConnected = ES_DISCONNECTED;
    sendLocker.Unlock();
	CleanUpOverlappedSendOperation(true);

    // now that we know no other method will keep adding to the queue
    // we can remove ourself from the queue
    theApp.uploadBandwidthThrottler->RemoveFromAllQueues(this);

    ClearQueues();
	RemoveAllLayers(); // deadlake PROXYSUPPORT
	AsyncSelect(0);
}

// deadlake PROXYSUPPORT
// By Maverick: Connection initialisition is done by class itself
BOOL CEMSocket::Connect(LPCSTR lpszHostAddress, UINT nHostPort)
{
	InitProxySupport();
	return CEncryptedStreamSocket::Connect(lpszHostAddress, nHostPort);
}
// end deadlake

// deadlake PROXYSUPPORT
// By Maverick: Connection initialisition is done by class itself
//BOOL CEMSocket::Connect(LPCTSTR lpszHostAddress, UINT nHostPort)
BOOL CEMSocket::Connect(SOCKADDR* pSockAddr, int iSockAddrLen)
{
	InitProxySupport();
	return CEncryptedStreamSocket::Connect(pSockAddr, iSockAddrLen);
}
// end deadlake

void CEMSocket::InitProxySupport()
{
	m_bProxyConnectFailed = false;

	// ProxyInitialisation
	const ProxySettings& settings = thePrefs.GetProxySettings();
	if (settings.UseProxy && settings.type != PROXYTYPE_NOPROXY)
	{
		m_bOverlappedSending = false;
		Close();

		m_pProxyLayer = new CAsyncProxySocketLayer;
		switch (settings.type)
		{
			case PROXYTYPE_SOCKS4:
			case PROXYTYPE_SOCKS4A:
				m_pProxyLayer->SetProxy(settings.type, settings.name, settings.port);
				break;
			case PROXYTYPE_SOCKS5:
			case PROXYTYPE_HTTP10:
			case PROXYTYPE_HTTP11:
				if (settings.EnablePassword)
					m_pProxyLayer->SetProxy(settings.type, settings.name, settings.port, settings.user, settings.password);
				else
					m_pProxyLayer->SetProxy(settings.type, settings.name, settings.port);
				break;
			default:
				ASSERT(0);
		}
		AddLayer(m_pProxyLayer);

		// Connection Initialisation
		Create(0, SOCK_STREAM, FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE, thePrefs.GetBindAddrA());
		AsyncSelect(FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
	}
}

void CEMSocket::ClearQueues(){
	EMTrace("CEMSocket::ClearQueues on %d",(SOCKET)this);

    sendLocker.Lock();
	for(POSITION pos = controlpacket_queue.GetHeadPosition(); pos != NULL; )
		delete controlpacket_queue.GetNext(pos);
	controlpacket_queue.RemoveAll();

	for(POSITION pos = standartpacket_queue.GetHeadPosition(); pos != NULL; )
		delete standartpacket_queue.GetNext(pos).packet;
	standartpacket_queue.RemoveAll();
#if !defined DONT_USE_SOCKET_BUFFERING
	sendblenWithoutControlPacket = 0;
	for (POSITION pos = m_currentPacket_in_buffer_list.GetHeadPosition(); pos != NULL;) {
		delete m_currentPacket_in_buffer_list.GetNext(pos);
	}
	m_currentPacket_in_buffer_list.RemoveAll();
#endif
    sendLocker.Unlock();

	// Download (pseudo) rate control	
	downloadLimit = 0;
	downloadLimitEnable = false;
	pendingOnReceive = false;

	// Download partial header
	pendingHeaderSize = 0;

	// Download partial packet
	delete pendingPacket;
	pendingPacket = NULL;
	pendingPacketSize = 0;

	// Upload control
	delete[] sendbuffer;
	sendbuffer = NULL;

	sendblen = 0;
#if !defined DONT_USE_SOCKET_BUFFERING
	sendblenWithoutControlPacket = 0;
#endif
	sent = 0;
}

void CEMSocket::OnClose(int nErrorCode){
    // need to be locked here to know that the other methods
    // won't be in the middle of things
    sendLocker.Lock();
	byConnected = ES_DISCONNECTED;
    sendLocker.Unlock();

    // now that we know no other method will keep adding to the queue
    // we can remove ourself from the queue
    theApp.uploadBandwidthThrottler->RemoveFromAllQueues(this);

	CEncryptedStreamSocket::OnClose(nErrorCode); // deadlake changed socket to PROXYSUPPORT ( AsyncSocketEx )
	RemoveAllLayers(); // deadlake PROXYSUPPORT
	ClearQueues();
}

BOOL CEMSocket::AsyncSelect(long lEvent){
#ifdef EMSOCKET_DEBUG
	if (lEvent&FD_READ)
		EMTrace("  FD_READ");
	if (lEvent&FD_CLOSE)
		EMTrace("  FD_CLOSE");
	if (lEvent&FD_WRITE)
		EMTrace("  FD_WRITE");
#endif
	// deadlake changed to AsyncSocketEx PROXYSUPPORT
	if (m_SocketData.hSocket != INVALID_SOCKET)
		return CEncryptedStreamSocket::AsyncSelect(lEvent);
	return true;
}

void CEMSocket::OnReceive(int nErrorCode){
//MORPH START take download ack overhead into account
	OnReceive(nErrorCode,true);
}
void CEMSocket::OnReceive(int nErrorCode, bool bAddACK){
//MORPH END take download ack overhead into account
	// the 2 meg size was taken from another place
	static char GlobalReadBuffer[10*1024*1024];

	// Check for an error code
	if(nErrorCode != 0){
		OnError(nErrorCode);
		return;
	}
	
	// Check current connection state
	if(byConnected == ES_DISCONNECTED){
		return;
	}
	else if(byConnected == ES_NOTCONNECTED) { //MORPH - ZZUL	
		byConnected = ES_CONNECTED; // ES_DISCONNECTED, ES_NOTCONNECTED, ES_CONNECTED
	}

    // CPU load improvement
    if(downloadLimitEnable == true && downloadLimit == 0){
        EMTrace("CEMSocket::OnReceive blocked by limit");
        pendingOnReceive = true;

        //Receive(GlobalReadBuffer + pendingHeaderSize, 0);
        return;
    }

	// Remark: an overflow can not occur here
	uint32 readMax = sizeof(GlobalReadBuffer) - pendingHeaderSize; 
	if(downloadLimitEnable == true && readMax > downloadLimit) {
		readMax = downloadLimit;
	}

	// We attempt to read up to 2 megs at a time (minus whatever is in our internal read buffer)
	uint32 ret = Receive(GlobalReadBuffer + pendingHeaderSize, readMax);
	if(ret == SOCKET_ERROR || byConnected == ES_DISCONNECTED){
		return;
	}
#if !defined DONT_USE_SOCKET_BUFFERING
	uint32 recvbufferlimit = 2*ret;
	if (recvbufferlimit > (10*1024*1024)) {
		recvbufferlimit = (10*1024*1024);
	} else if (recvbufferlimit < 8192) {
		recvbufferlimit = 8192;
	}

	if (recvbufferlimit > m_uCurrentRecvBufferSize) {
		SetSockOpt(SO_RCVBUF, &recvbufferlimit, sizeof(recvbufferlimit), SOL_SOCKET);
	}
	int ilen = sizeof(int);
	GetSockOpt(SO_RCVBUF, &recvbufferlimit, &ilen, SOL_SOCKET);
	m_uCurrentRecvBufferSize = recvbufferlimit;
#endif

	// Bandwidth control
	if(downloadLimitEnable == true){
		// Update limit
		downloadLimit -= GetRealReceivedBytes();
	}
	//MORPH START take download ack overhead into account
	if(bAddACK)
		theApp.uploadBandwidthThrottler->SetDownDataOverheadOtherPackets((GetRealReceivedBytes()/1460+ (GetRealReceivedBytes()%1460)?1:0)*40); // 40 bytes tcp overhead per frame.
	//MORPH END take download ack overhead into account

	// CPU load improvement
	// Detect if the socket's buffer is empty (or the size did match...)
	pendingOnReceive = m_bFullReceive;

	if (ret == 0)
		return;

	// Copy back the partial header into the global read buffer for processing
	if(pendingHeaderSize > 0) {
  		memcpy(GlobalReadBuffer, pendingHeader, pendingHeaderSize);
		ret += pendingHeaderSize;
		pendingHeaderSize = 0;
	}

	if (IsRawDataMode())
	{
		DataReceived((BYTE*)GlobalReadBuffer, ret);
		return;
	}

	char *rptr = GlobalReadBuffer; // floating index initialized with begin of buffer
	const char *rend = GlobalReadBuffer + ret; // end of buffer

	// Loop, processing packets until we run out of them
	while ((rend - rptr >= PACKET_HEADER_SIZE) || ((pendingPacket != NULL) && (rend - rptr > 0)))
	{
		// Two possibilities here: 
		//
		// 1. There is no pending incoming packet
		// 2. There is already a partial pending incoming packet
		//
		// It's important to remember that emule exchange two kinds of packet
		// - The control packet
		// - The data packet for the transport of the block
		// 
		// The biggest part of the traffic is done with the data packets. 
		// The default size of one block is 10240 bytes (or less if compressed), but the
		// maximal size for one packet on the network is 1300 bytes. It's the reason
		// why most of the Blocks are splitted before to be sent. 
		//
		// Conclusion: When the download limit is disabled, this method can be at least 
		// called 8 times (10240/1300) by the lower layer before a splitted packet is 
		// rebuild and transferred to the above layer for processing.
		//
		// The purpose of this algorithm is to limit the amount of data exchanged between buffers

		if(pendingPacket == NULL){
			pendingPacket = new Packet(rptr); // Create new packet container. 
			rptr += 6;                        // Only the header is initialized so far

			// Bugfix We still need to check for a valid protocol
			// Remark: the default eMule v0.26b had removed this test......
			switch (pendingPacket->prot){
				case OP_EDONKEYPROT:
				case OP_PACKEDPROT:
				case OP_EMULEPROT:
					break;
				default:
					EMTrace("CEMSocket::OnReceive ERROR Wrong header");
					delete pendingPacket;
					pendingPacket = NULL;
					OnError(ERR_WRONGHEADER);
					return;
			}

			// Security: Check for buffer overflow (2MB)
			if(pendingPacket->size > sizeof(GlobalReadBuffer)) {
				delete pendingPacket;
				pendingPacket = NULL;
				OnError(ERR_TOOBIG);
				return;
			}

			// Init data buffer
			pendingPacket->pBuffer = new char[pendingPacket->size + 1];
			pendingPacketSize = 0;
		}

		// Bytes ready to be copied into packet's internal buffer
		ASSERT(rptr <= rend);
		uint32 toCopy = ((pendingPacket->size - pendingPacketSize) < (uint32)(rend - rptr)) ? 
			             (pendingPacket->size - pendingPacketSize) : (uint32)(rend - rptr);

		// Copy Bytes from Global buffer to packet's internal buffer
		memcpy(&pendingPacket->pBuffer[pendingPacketSize], rptr, toCopy);
		pendingPacketSize += toCopy;
		rptr += toCopy;
		
		// Check if packet is complet
		ASSERT(pendingPacket->size >= pendingPacketSize);
		if(pendingPacket->size == pendingPacketSize){
#ifdef EMSOCKET_DEBUG
			EMTrace("CEMSocket::PacketReceived on %d, opcode=%X, realSize=%d", 
				    (SOCKET)this, pendingPacket->opcode, pendingPacket->GetRealPacketSize());
#endif

			// Process packet
			bool bPacketResult = PacketReceived(pendingPacket);
			delete pendingPacket;	
			pendingPacket = NULL;
			pendingPacketSize = 0;

			if (!bPacketResult)
				return;
		}
	}

	// Finally, if there is any data left over, save it for next time
	ASSERT(rptr <= rend);
	ASSERT(rend - rptr < PACKET_HEADER_SIZE);
	if(rptr != rend) {
		// Keep the partial head
		pendingHeaderSize = rend - rptr;
		memcpy(pendingHeader, rptr, pendingHeaderSize);
	}	
}

void CEMSocket::SetDownloadLimit(uint32 limit){	
	downloadLimit = limit;
	downloadLimitEnable = true;	
	
	// CPU load improvement
	if(limit > 0 && pendingOnReceive == true){
		//MORPH START take download ack overhead into account
		/*
		OnReceive(0);
		*/
		OnReceive(0,false);
		//MORPH END take download ack overhead into account
	}
}

void CEMSocket::DisableDownloadLimit(){
	downloadLimitEnable = false;

	// CPU load improvement
	if(pendingOnReceive == true){
		//MORPH START take download ack overhead into account
		/*
		OnReceive(0);
		*/
		OnReceive(0,false);
		//MORPH END take download ack overhead into account
	}
}

/**
 * Queues up the packet to be sent. Another thread will actually send the packet.
 *
 * If the packet is not a control packet, and if the socket decides that its queue is
 * full and forceAdd is false, then the socket is allowed to refuse to add the packet
 * to its queue. It will then return false and it is up to the calling thread to try
 * to call SendPacket for that packet again at a later time.
 *
 * @param packet address to the packet that should be added to the queue
 *
 * @param delpacket if true, the responsibility for deleting the packet after it has been sent
 *                  has been transferred to this object. If false, don't delete the packet after it
 *                  has been sent.
 *
 * @param controlpacket the packet is a controlpacket
 *
 * @param forceAdd this packet must be added to the queue, even if it is full. If this flag is true
 *                 then the method can not refuse to add the packet, and therefore not return false.
 *
 * @return true if the packet was added to the queue, false otherwise
 */
void CEMSocket::SendPacket(Packet* packet, bool delpacket, bool controlpacket, uint32 actualPayloadSize, bool bForceImmediateSend){
	//EMTrace("CEMSocket::OnSenPacked1 linked: %i, controlcount %i, standartcount %i, isbusy: %i",m_bLinkedPackets, controlpacket_queue.GetCount(), standartpacket_queue.GetCount(), IsBusy());

    BOOL sendSignalNoLongerBusy = false;

    if (byConnected == ES_DISCONNECTED)
	{
        if(delpacket) {
			delete packet;
        }
		return;
    } else {
        if (!delpacket){
            //ASSERT ( !packet->IsSplitted() );
            //Packet* copy = new Packet(packet->opcode,packet->size);
			Packet* copy = new Packet(packet);//bugfix by Xanatos [cyrex2001]
		    //memcpy(copy->pBuffer,packet->pBuffer,packet->size);
		    packet = copy;
	    }

        //if(m_startSendTick > 0) {
        //    m_lastSendLatency = ::GetTickCount() - m_startSendTick;
        //}
		sendLocker.Lock();
        if (controlpacket) {
	        controlpacket_queue.AddTail(packet);

            if(m_bConnectionIsReadyForSend) { //MORPH - ZZUL
			// queue up for controlpacket
			theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
			}  //MORPH - ZZUL
	    } else {
            //MORPH START - ZZUL
            /*
            bool first = !((sendbuffer && !m_currentPacket_is_controlpacket) || !standartpacket_queue.IsEmpty());
            */
			sendSignalNoLongerBusy = standartpacket_queue.IsEmpty();
            //MORPH END   - ZZUL
            StandardPacketQueueEntry queueEntry = { actualPayloadSize, packet };
		    standartpacket_queue.AddTail(queueEntry);

            //MORPH START - ZZUL
            /*
            // reset timeout for the first time
            if (first) {
                lastFinishedStandard = ::GetTickCount();
                m_bAccelerateUpload = true;	// Always accelerate first packet in a block
            }
            */
            //MORPH END   - ZZUL
	    }
    }

    sendLocker.Unlock();
	if (bForceImmediateSend){
		//MORPH START - ZZUL
		/*
		// when this assert fires in debug you will chrash becuase the messagepump might process dsiconnect event.
		ASSERT( controlpacket_queue.GetSize() == 1 );
		Send(1024, 0, true);
		*/
		m_bConnectionIsReadyForSend = true;
		Send(1300, 0, true);
		//MORPH START - ZZUL
	}

	if(sendSignalNoLongerBusy) {
        theApp.uploadBandwidthThrottler->SignalNoLongerBusy();
    }
}

//MORPH START - Added by SiRoB, Send Packet Array to prevent uploadbandwiththrottler lock
#if !defined DONT_USE_SEND_ARRAY_PACKET
void CEMSocket::SendPacket(Packet* packet[], uint32 npacket, bool delpacket, bool controlpacket, uint32 actualPayloadSize, bool bForceImmediateSend){
	//EMTrace("CEMSocket::OnSenPacked1 linked: %i, controlcount %i, standartcount %i, isbusy: %i",m_bLinkedPackets, controlpacket_queue.GetCount(), standartpacket_queue.GetCount(), IsBusy());

    BOOL sendSignalNoLongerBusy = false;

    if (byConnected == ES_DISCONNECTED) {
        for (uint32 i = 0; i < npacket; i++) {
			if(delpacket) {
				delete packet[i];
			}
        }
		return;
    } else {
        if (!delpacket){
            for (uint32 i = 0; i < npacket; i++) {
				//ASSERT ( !packet[i]->IsSplitted() );
				//Packet* copy = new Packet(packet[i]->opcode,packet[i]->size);
				Packet* copy = new Packet(packet[i]);//bugfix by Xanatos [cyrex2001]
				//memcpy(copy[i]->pBuffer,packet[i]->pBuffer,packet[i]->size);
				packet[i] = copy;
			}
	    }

        sendLocker.Lock();

        //if(m_startSendTick > 0) {
        //    m_lastSendLatency = ::GetTickCount() - m_startSendTick;
        //}
        if (controlpacket) {
	        for (uint32 i = 0; i < npacket; i++) {
				controlpacket_queue.AddTail(packet[i]);
			}
			if(m_bConnectionIsReadyForSend) {
			// queue up for controlpacket
			theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
			}
	    } else {
			sendSignalNoLongerBusy = standartpacket_queue.IsEmpty();

            uint32 payloadSize = actualPayloadSize/npacket;
			while (payloadSize <= actualPayloadSize) {
				actualPayloadSize -= payloadSize;
				if(actualPayloadSize < payloadSize) {
					payloadSize += actualPayloadSize;
				}
				StandardPacketQueueEntry queueEntry = { payloadSize, *packet++ };
				standartpacket_queue.AddTail(queueEntry);
			}
		}
    }

    sendLocker.Unlock();
	if (bForceImmediateSend){
	//	ASSERT( controlpacket_queue.GetSize() == 1 );   // when this assert fires in debug you will chrash becuase the messagepump might process dsiconnect event.
		m_bConnectionIsReadyForSend = true;
		Send(1300, 0, true);
	}

	if(sendSignalNoLongerBusy) {
        theApp.uploadBandwidthThrottler->SignalNoLongerBusy();
    }
}
#endif
//MORPH END   - Added by SiRoB, Send Packet Array to prevent uploadbandwiththrottler lock

uint64 CEMSocket::GetSentBytesCompleteFileSinceLastCallAndReset() {
    statsLocker.Lock();

    uint64 sentBytes = m_numberOfSentBytesCompleteFile;
    m_numberOfSentBytesCompleteFile = 0;

    statsLocker.Unlock();

    return sentBytes;
}

uint64 CEMSocket::GetSentBytesPartFileSinceLastCallAndReset() {
    statsLocker.Lock();

    uint64 sentBytes = m_numberOfSentBytesPartFile;
    m_numberOfSentBytesPartFile = 0;

    statsLocker.Unlock();

    return sentBytes;
}

uint64 CEMSocket::GetSentBytesControlPacketSinceLastCallAndReset() {
    statsLocker.Lock();

    uint64 sentBytes = m_numberOfSentBytesControlPacket;
    m_numberOfSentBytesControlPacket = 0;

    statsLocker.Unlock();

    return sentBytes;
}

uint64 CEMSocket::GetSentPayloadSinceLastCall(bool bReset) {
	if (!bReset)
		return m_actualPayloadSizeSent;
    statsLocker.Lock();

    uint64 sentBytes = m_actualPayloadSizeSent;
    m_actualPayloadSizeSent = 0;

    statsLocker.Unlock();

    return sentBytes;
}

void CEMSocket::OnSend(int nErrorCode){
    //onSendWillBeCalledOuter = false;

    if (nErrorCode){
		OnError(nErrorCode);
		return;
	}

	//EMTrace("CEMSocket::OnSend linked: %i, controlcount %i, standartcount %i, isbusy: %i",m_bLinkedPackets, controlpacket_queue.GetCount(), standartpacket_queue.GetCount(), IsBusy());
	CEncryptedStreamSocket::OnSend(0);

	//MORPH - Changed by SiRoB, Show BusyTime
	/*
    m_bBusy = false;
	*/


    // stopped sending here.
    //StoppedSendSoUpdateStats();

    if (byConnected == ES_DISCONNECTED) {
		return;
    } else if(byConnected == ES_NOTCONNECTED) {
		byConnected = ES_CONNECTED;

	}
	m_bConnectionIsReadyForSend = true;

	//MORPH START - Changed by SiRoB, Show BusyTime
	busyLocker.Lock();
	DWORD curTick = ::GetTickCount();
	if (m_dwBusy) {
		m_dwBusyDelta = curTick - m_dwBusy;
		m_dwNotBusy = curTick;
		m_dwBusy = 0;
	}
	busyLocker.Unlock();
	//MORPH END   - Changed by SiRoB, Show BusyTime

	//!onlyAllowedToSendControlPacket means we still got the socket in Standardlist
#if !defined DONT_USE_SOCKET_BUFFERING
	bool gotControlPacket = !m_bOverlappedSending && sendblenWithoutControlPacket != sendblen - sent /*&& m_currentPacket_is_controlpacket*/ ||
		m_bOverlappedSending && m_currentPacket_is_controlpacketOverlapped;
	if(gotControlPacket || !controlpacket_queue.IsEmpty()) {
#else
    /*
    if(m_currentPacket_is_controlpacket) {
    */
	if(sendbuffer != NULL && m_currentPacket_is_controlpacket || !controlpacket_queue.IsEmpty()) {
#endif
        // queue up for control packet
        theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
    }

#ifndef USE_MORPH_READ_THREAD
#if !defined DONT_USE_SOCKET_BUFFERING
	if (!m_bOverlappedSending && (!standartpacket_queue.IsEmpty() || sendblen/*sendbuffer*/ != NULL))
#else
	if (!m_bOverlappedSending && (!standartpacket_queue.IsEmpty() || sendbuffer != NULL))
#endif
		theApp.uploadBandwidthThrottler->SocketAvailable();
#endif

#if !defined DONT_USE_SOCKET_BUFFERING
	if(standartpacket_queue.GetCount() > 0 || gotControlPacket)
#else
	if (standartpacket_queue.GetCount() > 0 || m_currentPacket_is_controlpacket)
#endif
	        theApp.uploadBandwidthThrottler->SignalNoLongerBusy();
}

//void CEMSocket::StoppedSendSoUpdateStats() {
//    if(m_startSendTick > 0) {
//        m_lastSendLatency = ::GetTickCount()-m_startSendTick;
//        
//        if(m_lastSendLatency > 0) {
//            if(m_wasBlocked == true) {
//                SocketTransferStats newLatencyStat = { m_lastSendLatency, ::GetTickCount() };
//                m_Average_sendlatency_list.AddTail(newLatencyStat);
//                m_latency_sum += m_lastSendLatency;
//            }
//
//            m_startSendTick = 0;
//            m_wasBlocked = false;
//
//            CleanSendLatencyList();
//        }
//    }
//}
//
//void CEMSocket::CleanSendLatencyList() {
//    while(m_Average_sendlatency_list.GetCount() > 0 && ::GetTickCount() - m_Average_sendlatency_list.GetHead().timestamp > 3*1000) {
//        SocketTransferStats removedLatencyStat = m_Average_sendlatency_list.RemoveHead();
//        m_latency_sum -= removedLatencyStat.latency;
//    }
//}


SocketSentBytes CEMSocket::Send(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket)
{
    if (byConnected == ES_DISCONNECTED)
	{
        SocketSentBytes returnVal = { false, 0, 0 };
        return returnVal;
    }
	if (m_bOverlappedSending)
		return SendOv(maxNumberOfBytesToSend, minFragSize, onlyAllowedToSendControlPacket);
	else
		return SendStd(maxNumberOfBytesToSend, minFragSize, onlyAllowedToSendControlPacket);
}

/**
 * Try to put queued up data on the socket.
 *
 * Control packets have higher priority, and will be sent first, if possible.
 * Standard packets can be split up in several package containers. In that case
 * all the parts of a split package must be sent in a row, without any control packet
 * in between.
 *
 * @param maxNumberOfBytesToSend This is the maximum number of bytes that is allowed to be put on the socket
 *                               this call. The actual number of sent bytes will be returned from the method.
 *
 * @param onlyAllowedToSendControlPacket This call we only try to put control packets on the sockets.
 *                                       If there's a standard packet "in the way", and we think that this socket
 *                                       is no longer an upload slot, then it is ok to send the standard packet to
 *                                       get it out of the way. But it is not allowed to pick a new standard packet
 *                                       from the queue during this call. Several split packets are counted as one
 *                                       standard packet though, so it is ok to finish them all off if necessary.
 *
 * @return the actual number of bytes that were put on the socket.
 */
SocketSentBytes CEMSocket::SendStd(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket) {
	//EMTrace("CEMSocket::Send linked: %i, controlcount %i, standartcount %i, isbusy: %i",m_bLinkedPackets, controlpacket_queue.GetCount(), standartpacket_queue.GetCount(), IsBusy());
	//MORPH - ZZUL
	/*
	sendLocker.Lock();
	*/
	//MORPH - ZZUL
    if (maxNumberOfBytesToSend == 0 && ::GetTickCount() - lastCalledSend < SEC2MS(1)) {
    	SocketSentBytes returnVal = { true, 0, 0 };
        return returnVal;
    }

    bool anErrorHasOccured = false;
    uint32 errorThatOccured = 0; //MORPH - ZZUL
    uint32 sentStandardPacketBytesThisCall = 0;
    uint32 sentControlPacketBytesThisCall = 0;

    //MORPH - ZZUL
    /*
    if(byConnected == ES_CONNECTED && IsEncryptionLayerReady()) {
    */
    if(m_bConnectionIsReadyForSend && IsEncryptionLayerReady()) {
    //MORPH - ZZUL
		if(minFragSize < 1) {
            minFragSize = 1;
        }

		if (maxNumberOfBytesToSend == 0) { //MORPH - ZZUL
#if !defined DONT_USE_SOCKET_BUFFERING
			ASSERT (sendblenWithoutControlPacket <= sendblen-sent);
			maxNumberOfBytesToSend = GetNeededBytes(sendblen, sendblenWithoutControlPacket, sendblenWithoutControlPacket != sendblen-sent, lastCalledSend);
		}
#else
            maxNumberOfBytesToSend = GetNeededBytes(sendbuffer, sendblen, sent, m_currentPacket_is_controlpacket, lastCalledSend);
		}
#endif
        maxNumberOfBytesToSend = GetNextFragSize(maxNumberOfBytesToSend, minFragSize);
		//MORPH START - ZZUL
		if (maxNumberOfBytesToSend >= 40)
			maxNumberOfBytesToSend -= 40;
		maxNumberOfBytesToSend -= maxNumberOfBytesToSend/1500 * 40;
		//MORPH END   - ZZUL

	//MORPH START - ZZUL
	/*
        bool bWasLongTimeSinceSend = (::GetTickCount() - lastSent) > 1000;

        lastCalledSend = ::GetTickCount();
	*/
        bool bWasLongTimeSinceSend = (::GetTickCount() - lastCalledSend) > 1000;
	//MORPH END   - ZZUL

		sendLocker.Lock(); // Moved down
#if !defined DONT_USE_SOCKET_BUFFERING
		ASSERT (sendblenWithoutControlPacket <= sendblen-sent);
		//We don't need to loop as we are buffering everything in one send to avoid not optimized tcp packetsize
		if ( sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall < maxNumberOfBytesToSend && anErrorHasOccured == false && // don't send more than allowed. Also, there should have been no error in earlier loop
              (sendblen/*sendbuffer*/ != NULL || !controlpacket_queue.IsEmpty() || !standartpacket_queue.IsEmpty()) && // there must exist something to send
               (onlyAllowedToSendControlPacket == false || // this means we are allowed to send both types of packets, so proceed
                sendblen/*sendbuffer*/ != NULL &&  sendblenWithoutControlPacket != sendblen - sent/*m_currentPacket_is_controlpacket == true*/ || // We are in the progress of sending a control packet. We are always allowed to send those
                sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall > 0 && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) % minFragSize != 0 ||
                sendblen/*sendbuffer*/ == NULL && !controlpacket_queue.IsEmpty() || // There's a control packet in queue, and we are not currently sending anything, so we will handle the control packet next
                sendblen/*sendbuffer*/ != NULL && sendblenWithoutControlPacket == sendblen - sent/*m_currentPacket_is_controlpacket == false*/ && bWasLongTimeSinceSend && (!controlpacket_queue.IsEmpty() || sendblen-sent != sendblenWithoutControlPacket) && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) < minFragSize // We have waited to long to clean the current packet (which may be a standard packet that is in the way). Proceed no matter what the value of onlyAllowedToSendControlPacket.
                )
              ) {
#else
        while(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall < maxNumberOfBytesToSend && anErrorHasOccured == false && // don't send more than allowed. Also, there should have been no error in earlier loop
              (sendbuffer != NULL || !controlpacket_queue.IsEmpty() || !standartpacket_queue.IsEmpty()) && // there must exist something to send
               (onlyAllowedToSendControlPacket == false || // this means we are allowed to send both types of packets, so proceed
                sendbuffer != NULL && m_currentPacket_is_controlpacket == true || // We are in the progress of sending a control packet. We are always allowed to send those
                sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall > 0 && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) % minFragSize != 0 || // Once we've started, continue to send until an even minFragsize to minimize packet overhead
                sendbuffer == NULL && !controlpacket_queue.IsEmpty() || // There's a control packet in queue, and we are not currently sending anything, so we will handle the control packet next
                sendbuffer != NULL && m_currentPacket_is_controlpacket == false && bWasLongTimeSinceSend && !controlpacket_queue.IsEmpty() && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) < minFragSize // We have waited to long to clean the current packet (which may be a standard packet that is in the way). Proceed no matter what the value of onlyAllowedToSendControlPacket.
               )
             )
		{
#endif

            // If we are currently not in the progress of sending a packet, we will need to find the next one to send
#if !defined DONT_USE_SOCKET_BUFFERING
			ASSERT(sendblen>=sent);
			while ((!controlpacket_queue.IsEmpty() || !standartpacket_queue.IsEmpty()) && sendblen-sent+sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall < maxNumberOfBytesToSend) {
				bool bcontrolpacket;
				uint32 ipacketpayloadsize = 0;
#else
			if(sendbuffer == NULL) {
#endif
				Packet* curPacket = NULL;
				if(!controlpacket_queue.IsEmpty()) {
                    // There's a control packet to send
#if !defined DONT_USE_SOCKET_BUFFERING
					bcontrolpacket = true;
#else
					m_currentPacket_is_controlpacket = true;
#endif

					curPacket = controlpacket_queue.RemoveHead();
                } else if(!standartpacket_queue.IsEmpty()) {
                    // There's a standard packet to send
#if !defined DONT_USE_SOCKET_BUFFERING
					bcontrolpacket = false;
#else
					m_currentPacket_is_controlpacket = false;
#endif
					StandardPacketQueueEntry queueEntry = standartpacket_queue.RemoveHead();
                    curPacket = queueEntry.packet;
#if !defined DONT_USE_SOCKET_BUFFERING
                    ipacketpayloadsize = queueEntry.actualPayloadSize;
#else
                    m_actualPayloadSize = queueEntry.actualPayloadSize;
                    m_actualPayloadSizeSentForThisPacket = 0;
                    // remember this for statistics purposes.
                    m_currentPackageIsFromPartFile = curPacket->IsFromPF();
#endif

                } else {
                    // Just to be safe. Shouldn't happen?
                    sendLocker.Unlock();

                    // if we reach this point, then there's something wrong with the while condition above!
                    ASSERT(0);
                    theApp.QueueDebugLogLine(true,_T("EMSocket: Couldn't get a new packet! There's an error in the first while condition in EMSocket::Send()"));

					//MORPH - Changed by SiRoB, Take into account IP+TCP Header
                	/*
                	SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
                	*/
                	SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall , sentControlPacketBytesThisCall + (((sentControlPacketBytesThisCall+sentStandardPacketBytesThisCall)/1460)+(((sentControlPacketBytesThisCall+sentStandardPacketBytesThisCall)%1460)?1:0)) * 40};
                	return returnVal;
                }

	            // We found a package to send. Get the data to send from the
	            // package container and dispose of the container.
#if !defined DONT_USE_SOCKET_BUFFERING
				uint32 packetsize = curPacket->GetRealPacketSize();
				uint32 sendbufferlimit = packetsize;
				if (sendbufferlimit > 10*1024*1024)
					sendbufferlimit = 10*1024*1024;
				else if (sendbufferlimit < 8192)
					sendbufferlimit = 8192;
				if (m_uCurrentSendBufferSize != sendbufferlimit) {
					SetSockOpt(SO_SNDBUF, &sendbufferlimit, sizeof(sendbufferlimit), SOL_SOCKET);
				}
				int ilen = sizeof(int);
				GetSockOpt(SO_SNDBUF, &sendbufferlimit, &ilen, SOL_SOCKET);
				m_uCurrentSendBufferSize = sendbufferlimit;
				if (sendbuffer) {
					if (sent > (currentBufferSize>>1) || currentBufferSize < sendblen+packetsize){
						ASSERT(sendblen>=sent);
						sendblen-=sent;
						if (currentBufferSize < sendblen+packetsize) {
							currentBufferSize = max(sendblen+packetsize, 2*m_uCurrentSendBufferSize);
							char* newsendbuffer = new char[currentBufferSize];
							memcpy(newsendbuffer, sendbuffer+sent, sendblen);
							delete[] sendbuffer;
							sendbuffer = newsendbuffer;
						} else {
							memmove(sendbuffer, sendbuffer+sent, sendblen);
						}
						sent = 0;
					}
					char* packetcore = curPacket->DetachPacket();
					// encrypting which cannot be done transparent by base class
					CryptPrepareSendData((uchar*)packetcore, packetsize);
					memcpy(sendbuffer+sendblen, packetcore, packetsize);
					delete[] packetcore;
					sendblen+=packetsize;
				} else {
					ASSERT (sent == 0);
					sendbuffer = curPacket->DetachPacket();
					sendblen = packetsize;
					// encrypting which cannot be done transparent by base class
					CryptPrepareSendData((uchar*)sendbuffer, packetsize);
					currentBufferSize = packetsize;
				}
				if (bcontrolpacket == false)
					sendblenWithoutControlPacket+=packetsize;
				BufferedPacket* newitem = new BufferedPacket;
				newitem->remainpacketsize = packetsize;
				newitem->isforpartfile = curPacket->IsFromPF();
				newitem->iscontrolpacket = bcontrolpacket;
				newitem->packetpayloadsize = ipacketpayloadsize;
				m_currentPacket_in_buffer_list.AddTail(newitem);
				delete curPacket;
#else
				sendblen = curPacket->GetRealPacketSize();
				sendbuffer = curPacket->DetachPacket();
				sent = 0;
				delete curPacket;

				// encrypting which cannot be done transparent by base class
				CryptPrepareSendData((uchar*)sendbuffer, sendblen);
#endif
			}

//            sendLocker.Unlock();

            // At this point we've got a packet to send in sendbuffer. Try to send it. Loop until entire packet
            // is sent, or until we reach maximum bytes to send for this call, or until we get an error.
            // NOTE! If send would block (returns WSAEWOULDBLOCK), we will return from this method INSIDE this loop.
            while (sent < sendblen &&
                   sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall < maxNumberOfBytesToSend &&
                   (
                    onlyAllowedToSendControlPacket == false || // this means we are allowed to send both types of packets, so proceed
#if !defined DONT_USE_SOCKET_BUFFERING
                    sendblenWithoutControlPacket != sendblen - sent/*m_currentPacket_is_controlpacket*/ ||
#else
                    m_currentPacket_is_controlpacket ||
#endif
                    bWasLongTimeSinceSend && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) < minFragSize ||
                    (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) % minFragSize != 0
                   ) &&
                   anErrorHasOccured == false)
			{
		        uint32 tosend = sendblen-sent;
#if !defined DONT_USE_SOCKET_BUFFERING
                if(!onlyAllowedToSendControlPacket || sendblenWithoutControlPacket != sendblen - sent/*m_currentPacket_is_controlpacket*/) {
#else
                if(!onlyAllowedToSendControlPacket || m_currentPacket_is_controlpacket) {
#endif
					if (maxNumberOfBytesToSend >= sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall && tosend > maxNumberOfBytesToSend-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall))
                        tosend = maxNumberOfBytesToSend-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall);
                } else if(bWasLongTimeSinceSend && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) < minFragSize) {
    		        if (minFragSize >= sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall && tosend > minFragSize-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall))
                        tosend = minFragSize-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall);
                } else {
                    uint32 nextFragMaxBytesToSent = GetNextFragSize(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall, minFragSize);
    		        if (nextFragMaxBytesToSent >= sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall && tosend > nextFragMaxBytesToSent-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall))
                        tosend = nextFragMaxBytesToSent-(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall);
                }
		        ASSERT (tosend != 0 && tosend <= sendblen-sent);
        		
                //DWORD tempStartSendTick = ::GetTickCount();

                //MORPH START - ZZUL
                /*
                lastSent = ::GetTickCount();
                */
                //MORPH END   - ZZUL

#if !defined DONT_USE_SOCKET_BUFFERING
				if (tosend >= m_uCurrentSendBufferSize) //Don't send more than socket buffer
					tosend = m_uCurrentSendBufferSize-1;
#endif
		        busyLocker.Lock(); //MORPH - Changed by SiRoB, Show BusyTime
		        uint32 result = CEncryptedStreamSocket::Send(sendbuffer+sent,tosend); // deadlake PROXYSUPPORT - changed to AsyncSocketEx
		        if (result == (uint32)SOCKET_ERROR){
			        uint32 error = GetLastError();
			        if (error == WSAEWOULDBLOCK){
                        //MORPH START - Changed by SiRoB, Show BusyTime
                        /*
                        m_bBusy = true;
                        */
                        DWORD curTick = GetTickCount();
                        if (m_dwBusy == 0) {
						    m_dwNotBusyDelta = curTick-m_dwNotBusy;
							m_dwBusy = curTick;
                        }
                        m_dwNotBusy = 0;
                        busyLocker.Unlock();
                        //MORPH END   - Changed by SiRoB, Show BusyTime

                        //m_wasBlocked = true;
                        sendLocker.Unlock();

                        //MORPH - Changed by SiRoB, Take into account IP+TCP Header
                        /*
                        SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
                        */
                        SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall , sentControlPacketBytesThisCall + (((sentControlPacketBytesThisCall+sentStandardPacketBytesThisCall)/1460)+(((sentControlPacketBytesThisCall+sentStandardPacketBytesThisCall)%1460)?1:0)) * 40};
                        return returnVal; // Send() blocked, onsend will be called when ready to send again
			        } else{
                        busyLocker.Unlock();  //MORPH - Changed by SiRoB, Show BusyTime
                        // Send() gave an error
                        anErrorHasOccured = true;
                        errorThatOccured = error; //MORPH - ZZUL
                        //DEBUG_ONLY( AddDebugLogLine(true,"EMSocket: An error has occured: %i", error) );
                    }
                } else {
                    // we managed to send some bytes. Perform bookkeeping.
                    ASSERT (tosend==result); //MORPH
                    //MORPH START - Changed by SiRoB, Show BusyTime
                    /*
                    m_bBusy = false;
                    */
                    DWORD curTick = ::GetTickCount();
                    if (m_dwBusy) {
                    	m_dwBusyDelta = curTick-m_dwBusy;
                    	m_dwNotBusy = curTick;
                    	m_dwBusy = 0;
                    }
                    //MORPH END   - Changed by SiRoB, Show BusyTime
                    m_hasSent = true;
                    busyLocker.Unlock();  //MORPH - Changed by SiRoB, Show BusyTime

                    lastCalledSend = ::GetTickCount(); //MORPH - ZZUL
                    sent += result;
#if !defined DONT_USE_SOCKET_BUFFERING
					uint32 sumofpacketsizesent = 0;
					uint32 sumofnocontrolpacketsizesent = 0;
					uint32 sumofpacketpartfilesizesent = 0;
					while (result > sumofpacketsizesent && result-sumofpacketsizesent >= m_currentPacket_in_buffer_list.GetHead()->remainpacketsize) {
						BufferedPacket* pPacket = m_currentPacket_in_buffer_list.RemoveHead();
						if (pPacket->iscontrolpacket == false) {
							sumofnocontrolpacketsizesent += pPacket->remainpacketsize;
							if (pPacket->isforpartfile)
								sumofpacketpartfilesizesent += pPacket->remainpacketsize;  
						
							if(0 < pPacket->packetpayloadsize) {
								statsLocker.Lock();
								m_actualPayloadSizeSent += pPacket->packetpayloadsize;
								statsLocker.Unlock();
							} else {
								ASSERT(0);
							}
							lastFinishedStandard = ::GetTickCount(); // reset timeout
							m_bAccelerateUpload = false; // Safe until told otherwise
							sendblenWithoutControlPacket -= pPacket->remainpacketsize;
						}
						sumofpacketsizesent += pPacket->remainpacketsize;
						delete pPacket;
					}
					if (result > sumofpacketsizesent) {
						BufferedPacket* pPacket = m_currentPacket_in_buffer_list.GetHead();
						uint32 partialpacketsizesent = result-sumofpacketsizesent;
						if (pPacket->iscontrolpacket == false) {
							sumofnocontrolpacketsizesent += partialpacketsizesent;
							if (pPacket->isforpartfile)
								sumofpacketpartfilesizesent += partialpacketsizesent;
							uint32 partialpayloadSentWithThisCall = (uint32)(((double)partialpacketsizesent/(double)(pPacket->remainpacketsize))*pPacket->packetpayloadsize);
							if(partialpayloadSentWithThisCall <= pPacket->packetpayloadsize) {
								statsLocker.Lock();
								m_actualPayloadSizeSent += partialpayloadSentWithThisCall;
								statsLocker.Unlock();
							} else {
								ASSERT(0);
							}
							pPacket->packetpayloadsize -= partialpayloadSentWithThisCall;
							sendblenWithoutControlPacket -= partialpacketsizesent;
						}
						pPacket->remainpacketsize -= partialpacketsizesent;
					}
#else
					if(!m_currentPacket_is_controlpacket) {
						uint32 payloadSentWithThisCall = (uint32)(((double)result/(double)sendblen)*m_actualPayloadSize);
						if(m_actualPayloadSizeSentForThisPacket + payloadSentWithThisCall <= m_actualPayloadSize) {
							m_actualPayloadSizeSentForThisPacket += payloadSentWithThisCall;

							statsLocker.Lock();
							m_actualPayloadSizeSent += payloadSentWithThisCall;
							statsLocker.Unlock();
						}

						ASSERT(m_actualPayloadSizeSentForThisPacket <= m_actualPayloadSize);
					}
#endif
                    // Log send bytes in correct class
#if !defined DONT_USE_SOCKET_BUFFERING
					if(sumofnocontrolpacketsizesent/*m_currentPacket_is_controlpacket == false*/) {
						sentStandardPacketBytesThisCall += sumofnocontrolpacketsizesent/*result*/;
						if(sumofpacketpartfilesizesent) {
#else
                    if(m_currentPacket_is_controlpacket == false) {
                        sentStandardPacketBytesThisCall += result;

                        if(m_currentPackageIsFromPartFile == true) {
#endif
                            statsLocker.Lock(); //MORPH - ZZUL
#if !defined DONT_USE_SOCKET_BUFFERING
                            m_numberOfSentBytesPartFile += sumofpacketpartfilesizesent/*result*/;
                            m_numberOfSentBytesCompleteFile += sumofnocontrolpacketsizesent-sumofpacketpartfilesizesent/*result*/;
#else
                            m_numberOfSentBytesPartFile += result;
#endif
                            statsLocker.Unlock(); //MORPH - ZZUL
                        } else {
                            statsLocker.Lock(); //MORPH - ZZUL
#if !defined DONT_USE_SOCKET_BUFFERING
                            m_numberOfSentBytesCompleteFile += sumofnocontrolpacketsizesent/*result*/;
#else
                            m_numberOfSentBytesCompleteFile += result;
#endif
                            statsLocker.Unlock();
                        }
#if !defined DONT_USE_SOCKET_BUFFERING
                    }
					sentControlPacketBytesThisCall += result-sumofnocontrolpacketsizesent;
					statsLocker.Lock();
					m_numberOfSentBytesControlPacket += result-sumofnocontrolpacketsizesent;
					statsLocker.Unlock();
#else
                    } else {
                        sentControlPacketBytesThisCall += result;
                        statsLocker.Lock(); //MORPH - ZZUL
                        m_numberOfSentBytesControlPacket += result;
                        statsLocker.Unlock(); //MORPH - ZZUL
                    }
#endif
                }
	        }

            //MORPH START - Changed by SiRoB, just to be sur i got some strange thing here
            /*
            if (sent == sendblen){
            */
            if (sent && sent == sendblen){
            //MORPH END   - Changed by SiRoB, just to be sur i got some strange thing here
                // we are done sending the current package. Delete it and set
                // sendbuffer to NULL so a new packet can be fetched.
		        delete[] sendbuffer;
		        sendbuffer = NULL;
			    sendblen = 0;

#if defined DONT_USE_SOCKET_BUFFERING
                if(!m_currentPacket_is_controlpacket) {
                    //MORPH START - ZZUL
                    /*
                    m_actualPayloadSizeSent += m_actualPayloadSize;
                    */
                    if(m_actualPayloadSizeSentForThisPacket < m_actualPayloadSize) {
                    	UINT rest = (m_actualPayloadSize-m_actualPayloadSizeSentForThisPacket);
                    	statsLocker.Lock();
                    	m_actualPayloadSizeSent += rest;
                    	statsLocker.Unlock();

                    	m_actualPayloadSizeSentForThisPacket += rest;
                    }

                    ASSERT(m_actualPayloadSizeSentForThisPacket == m_actualPayloadSize);
                    //MORPH END   - ZZUL

                    m_actualPayloadSize = 0;
                    m_actualPayloadSizeSentForThisPacket = 0; //MORPH - ZZUL

                    lastFinishedStandard = ::GetTickCount(); // reset timeout
                    m_bAccelerateUpload = false; // Safe until told otherwise
                }
#endif

                sent = 0;
            }

			// lock before checking the loop condition
//			sendLocker.Lock();
        }
        sendLocker.Unlock(); //MORPH - ZZUL 
    }

    //MORPH START - ZZUL
    /*
    if(onlyAllowedToSendControlPacket && (!controlpacket_queue.IsEmpty() || sendbuffer != NULL && m_currentPacket_is_controlpacket)) {
        // enter control packet send queue
        // we might enter control packet queue several times for the same package,
        // but that costs very little overhead. Less overhead than trying to make sure
        // that we only enter the queue once.
        theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
    }

    //CleanSendLatencyList();

    sendLocker.Unlock();
    */
	//!onlyAllowedToSendControlPacket means we still got the socket in Standardlist
	if (onlyAllowedToSendControlPacket) {
		sendLocker.Lock();
#if !defined DONT_USE_SOCKET_BUFFERING
		if(m_bConnectionIsReadyForSend && byConnected != ES_DISCONNECTED && (sendblenWithoutControlPacket != sendblen - sent /*m_currentPacket_is_controlpacket == true*/ || !controlpacket_queue.IsEmpty())) {
#else
		if(m_bConnectionIsReadyForSend && byConnected != ES_DISCONNECTED && (sendbuffer != NULL && m_currentPacket_is_controlpacket || !controlpacket_queue.IsEmpty())) {
#endif
			theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
		}
    	sendLocker.Unlock();
	}
    //MORPH END   - ZZUL

	//MORPH - Changed by SiRoB, Take into account IP+TCP Header
    /*
    SocketSentBytes returnVal = { !anErrorHasOccured, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
    */
#if !defined DONT_USE_SOCKET_BUFFERING
	// Revisited by Stulle: Returning success true has been introduced along with the socket buffering.
	// Since that code changes how much data is pumped into the socket I am not entirely sure, if this
	// has been changed to ensure we reset the timeout timer in CClientReqSocket. However, ZZ's code is
	// not forcing the success to be true so I added the alternative here, too. Furthermore, I have
	// revised the calling send functions in CClientReqSocket to at least log any error.
	SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall + (((sentControlPacketBytesThisCall+sentStandardPacketBytesThisCall)/1460)+(((sentControlPacketBytesThisCall+sentStandardPacketBytesThisCall)%1460)?1:0)) * 40, errorThatOccured};
#else
	SocketSentBytes returnVal = { !anErrorHasOccured, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall + (((sentControlPacketBytesThisCall + sentStandardPacketBytesThisCall) / 1460) + (((sentControlPacketBytesThisCall + sentStandardPacketBytesThisCall) % 1460) ? 1 : 0)) * 40, errorThatOccured };
#endif
		return returnVal;
}

/**
 * Try to put queued up data on the socket with Overlapped methods.
 *
 * Control packets have higher priority, and will be sent first, if possible.
 *
 * @param maxNumberOfBytesToSend This is the maximum number of bytes that is allowed to be put on the socket
 *                               this call. The actual number of sent bytes will be returned from the method.
 *
 * @param onlyAllowedToSendControlPacket This call we only try to put control packets on the sockets.
 *
 * @return the actual number of bytes that were put on the socket.
 */
SocketSentBytes CEMSocket::SendOv(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket) {
	//EMTrace("CEMSocket::Send linked: %i, controlcount %i, standartcount %i, isbusy: %i",m_bLinkedPackets, controlpacket_queue.GetCount(), standartpacket_queue.GetCount(), IsBusy());
    ASSERT( m_pProxyLayer == NULL );
	//MORPH START - ZZUL
	/*
	sendLocker.Lock();
	*/
	if (maxNumberOfBytesToSend == 0 && ::GetTickCount() - lastCalledSend < SEC2MS(1)) {
		SocketSentBytes returnVal = { true, 0, 0 };
		return returnVal;
	}
	//MORPH END   - ZZUL
	bool anErrorHasOccured = false;
	uint32 errorThatOccured = 0; //MORPH - ZZUL
	uint32 sentStandardPacketBytesThisCall = 0;
    uint32 sentControlPacketBytesThisCall = 0;
    
	//MORPH START - ZZUL
	/*
	if(byConnected == ES_CONNECTED && IsEncryptionLayerReady() && !IsBusyExtensiveCheck() && maxNumberOfBytesToSend > 0) {
	*/
	if(m_bConnectionIsReadyForSend && IsEncryptionLayerReady() && !IsBusyExtensiveCheck() && maxNumberOfBytesToSend > 0) {
	//MORPH END   - ZZUL
        if(minFragSize < 1) {
            minFragSize = 1;
        }

		if (maxNumberOfBytesToSend == 0) { //MORPH - ZZUL
#if !defined DONT_USE_SOCKET_BUFFERING
			maxNumberOfBytesToSend = GetNeededBytes(sendblen, 0, m_currentPacket_is_controlpacketOverlapped, lastCalledSend);
		}
#else
			maxNumberOfBytesToSend = GetNeededBytes(sendbuffer, sendblen, sent, m_currentPacket_is_controlpacket, lastCalledSend);
	}
#endif
		maxNumberOfBytesToSend = GetNextFragSize(maxNumberOfBytesToSend, minFragSize);
		//MORPH START - ZZUL
		/*
		lastCalledSend = ::GetTickCount();
		*/
		if (maxNumberOfBytesToSend >= 40)
			maxNumberOfBytesToSend -= 40;
		maxNumberOfBytesToSend -= maxNumberOfBytesToSend / 1500 * 40;
		sendLocker.Lock(); // Moved down
		//MORPH END   - ZZUL
		ASSERT( m_pPendingSendOperation == NULL && m_aBufferSend.IsEmpty());
		sint32 nBytesLeft = maxNumberOfBytesToSend;
		if (sendbuffer != NULL || !controlpacket_queue.IsEmpty() || !standartpacket_queue.IsEmpty())
		{
			// WSASend takes multiple buffers which is quite nice for our case, as we have to call send
			// only once regardless how many packets we want to ship without memorymoving. But before
			// we can do this, collect all buffers we want to send in this call

			// first send the existing sendbuffer (already started packet)
			if (sendbuffer != NULL)
			{
				WSABUF pCurBuf;;
				pCurBuf.len = min(sendblen - sent, (uint32)nBytesLeft);
				pCurBuf.buf = new CHAR[pCurBuf.len];
				memcpy(pCurBuf.buf, sendbuffer + sent, pCurBuf.len);
				sent += pCurBuf.len;
				m_aBufferSend.Add(pCurBuf);
				nBytesLeft -= pCurBuf.len;
				if (sent == sendblen) //finished the buffer
				{
					delete[] sendbuffer;
					sendbuffer = NULL;
					sendblen = 0;
				}
				sentStandardPacketBytesThisCall += pCurBuf.len; // Sendbuffer is always a standard packet in this method
				lastFinishedStandard = ::GetTickCount();
				m_bAccelerateUpload = false;
				statsLocker.Lock(); //MORPH - ZZUL
#if !defined DONT_USE_SOCKET_BUFFERING
				m_actualPayloadSizeSent += m_actualPayloadSizeOverlapped;
				m_actualPayloadSizeOverlapped = 0;
				if (m_currentPackageIsFromPartFileOverlapped)
#else
				m_actualPayloadSizeSent += m_actualPayloadSize;
				m_actualPayloadSize = 0;
				if (m_currentPackageIsFromPartFile)
#endif
					m_numberOfSentBytesPartFile += pCurBuf.len;
				else
					m_numberOfSentBytesCompleteFile += pCurBuf.len;
				statsLocker.Unlock(); //MORPH - ZZUL
			}

			// next send all control packets if there are any and we have bytes left
			while (!controlpacket_queue.IsEmpty() && nBytesLeft > 0)
			{
				// send controlpackets always completely, ignoring the limit by a few bytes if we must
				WSABUF pCurBuf;
				Packet* curPacket = controlpacket_queue.RemoveHead();
				pCurBuf.len = curPacket->GetRealPacketSize();
				pCurBuf.buf = curPacket->DetachPacket();
				delete curPacket;
				// encrypting which cannot be done transparent by base class
				CryptPrepareSendData((uchar*)pCurBuf.buf, pCurBuf.len);
				m_aBufferSend.Add(pCurBuf);
				nBytesLeft -= pCurBuf.len;
				sentControlPacketBytesThisCall += pCurBuf.len;
			}

			// and now finally the standard packets if there are any and we have bytes left and we are allowed to
			while (!standartpacket_queue.IsEmpty() && nBytesLeft > 0 && !onlyAllowedToSendControlPacket)
			{
				StandardPacketQueueEntry queueEntry = standartpacket_queue.RemoveHead();
				WSABUF pCurBuf;
				Packet* curPacket = queueEntry.packet;
#if !defined DONT_USE_SOCKET_BUFFERING
				uint32 sendbufferlimit = curPacket->GetRealPacketSize();
				m_currentPackageIsFromPartFileOverlapped = curPacket->IsFromPF();
#else
				m_currentPackageIsFromPartFile = curPacket->IsFromPF();
#endif
				
				// can we send it right away or only a part of it?
				if (queueEntry.packet->GetRealPacketSize() <= (uint32)nBytesLeft)
				{
					// yay
					pCurBuf.len = curPacket->GetRealPacketSize();
					pCurBuf.buf = curPacket->DetachPacket();
					CryptPrepareSendData((uchar*)pCurBuf.buf, pCurBuf.len);// encrypting which cannot be done transparent by base class
					m_actualPayloadSizeSent += queueEntry.actualPayloadSize;
					lastFinishedStandard = ::GetTickCount();
					m_bAccelerateUpload = false;
				}
				else
				{	// aww, well first stuff everything into the sendbuffer and then send what we can of it
					ASSERT( sendbuffer == NULL );
#if !defined DONT_USE_SOCKET_BUFFERING
					m_actualPayloadSizeOverlapped = queueEntry.actualPayloadSize;
#else
					m_actualPayloadSize = queueEntry.actualPayloadSize;
#endif
					sendblen = curPacket->GetRealPacketSize();
					sendbuffer = curPacket->DetachPacket();
					sent = 0;
					CryptPrepareSendData((uchar*)sendbuffer, sendblen); // encrypting which cannot be done transparent by base class
					pCurBuf.len = min(sendblen - sent, (uint32)nBytesLeft);
					pCurBuf.buf = new CHAR[pCurBuf.len];
					memcpy(pCurBuf.buf, sendbuffer, pCurBuf.len);
					sent += pCurBuf.len;
					ASSERT (sent < sendblen);
#if !defined DONT_USE_SOCKET_BUFFERING
					m_currentPacket_is_controlpacketOverlapped = false;
#else
					m_currentPacket_is_controlpacket = false;
#endif
				}
				delete curPacket;
				m_aBufferSend.Add(pCurBuf);
				nBytesLeft -= pCurBuf.len;
				sentStandardPacketBytesThisCall += pCurBuf.len;
				statsLocker.Lock(); //MORPH - ZZUL
#if !defined DONT_USE_SOCKET_BUFFERING
				if (m_currentPackageIsFromPartFileOverlapped)
#else
				if(m_currentPackageIsFromPartFile)
#endif
					m_numberOfSentBytesPartFile += pCurBuf.len;
				else
					m_numberOfSentBytesCompleteFile += pCurBuf.len;
				statsLocker.Unlock(); //MORPH - ZZUL
#if !defined DONT_USE_SOCKET_BUFFERING
				if (sendbufferlimit > 10 * 1024 * 1024)
					sendbufferlimit = 10 * 1024 * 1024;
				else if (sendbufferlimit < 8192)
					sendbufferlimit = 8192;
				if (m_uCurrentSendBufferSize < sendbufferlimit) {
					// Note: Since we use overlapped sockets here the buffer will hold a multiple
					// of the buffer, regardless. Thus we just enlarge it to the max package size
					// we encounter and leave it there.
					theApp.QueueDebugLogLineEx(ERROR, _T("SendOv() increasing buffer curBuf %u newBuf %u"), m_uCurrentSendBufferSize, sendbufferlimit);
					if (m_uCurrentSendBufferSize != 0)
					{
						SetSockOpt(SO_SNDBUF, &sendbufferlimit, sizeof(sendbufferlimit), SOL_SOCKET);
					}
					int ilen = sizeof(int);
					GetSockOpt(SO_SNDBUF, &sendbufferlimit, &ilen, SOL_SOCKET);
					m_uCurrentSendBufferSize = sendbufferlimit;
				}
#endif
			}
			// allright, prepare to send our collected buffers
			m_pPendingSendOperation = new WSAOVERLAPPED;
			ZeroMemory(m_pPendingSendOperation, sizeof(WSAOVERLAPPED));
			m_pPendingSendOperation->hEvent = theApp.uploadBandwidthThrottler->GetSocketAvailableEvent();
			DWORD dwBytesSent = 0;
			if (CEncryptedStreamSocket::SendOv(m_aBufferSend, dwBytesSent, m_pPendingSendOperation) == 0)
			{
				ASSERT( dwBytesSent > 0 );
				//MORPH START - Changed by SiRoB, Show BusyTime
				busyLocker.Lock();
				DWORD curTick = ::GetTickCount();
				if (m_dwBusy) {
					m_dwBusyDelta = curTick - m_dwBusy;
					m_dwNotBusy = curTick;
					m_dwBusy = 0;
				}
				busyLocker.Unlock();
				//MORPH END   - Changed by SiRoB, Show BusyTime
				CleanUpOverlappedSendOperation(false);
			}
			else
			{
				int nError = WSAGetLastError();
				if (nError != WSA_IO_PENDING)
				{
					anErrorHasOccured = true;
					// Note: This will cause the error to be logged twice but it also gives us more information
					errorThatOccured = uint32(nError); //MORPH - ZZUL
					theApp.QueueDebugLogLineEx(ERROR, _T("WSASend() Error: %u, %s"), nError, GetErrorMessage(nError));
				}
				//MORPH START - Changed by SiRoB, Show BusyTime
				else
				{
					DWORD curTick = GetTickCount();
					if (m_dwBusy == 0) {
						m_dwNotBusyDelta = curTick - m_dwNotBusy;
						m_dwBusy = curTick;
					}
					m_dwNotBusy = 0;
					busyLocker.Unlock();
				}
				//MORPH END   - Changed by SiRoB, Show BusyTime

			}
			lastCalledSend = ::GetTickCount(); //MORPH - ZZUL
		}
    }

	//MORPH START - ZZUL
	/*
	if(onlyAllowedToSendControlPacket && !controlpacket_queue.IsEmpty())
	{
	*/
	//!onlyAllowedToSendControlPacket means we still got the socket in Standardlist
	if (onlyAllowedToSendControlPacket && m_bConnectionIsReadyForSend && byConnected != ES_DISCONNECTED && !controlpacket_queue.IsEmpty()) {
	//MORPH END   - ZZUL
        // enter control packet send queue
        // we might enter control packet queue several times for the same package,
        // but that costs very little overhead. Less overhead than trying to make sure
        // that we only enter the queue once.
        theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
    }

    sendLocker.Unlock();
	//MORPH - Changed by SiRoB, Take into account IP+TCP Header
	/*
	SocketSentBytes returnVal = { !anErrorHasOccured, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
	*/
	SocketSentBytes returnVal = { !anErrorHasOccured, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall + (((sentControlPacketBytesThisCall + sentStandardPacketBytesThisCall) / 1460) + (((sentControlPacketBytesThisCall + sentStandardPacketBytesThisCall) % 1460) ? 1 : 0)) * 40, errorThatOccured };
    return returnVal;
}

uint32 CEMSocket::GetNextFragSize(uint32 current, uint32 minFragSize) {
    if(current % minFragSize == 0) {
        return current;
    } else {
        return minFragSize*(current/minFragSize+1);
    }
}

/**
 * Decides the (minimum) amount the socket needs to send to prevent timeout.
 * 
 * @author SlugFiller
 */
/*
uint32 CEMSocket::GetNeededBytes() {
*/
#if !defined DONT_USE_SOCKET_BUFFERING
uint32 CEMSocket::GetNeededBytes(const uint32 sendblen, const uint32 sendblenWithoutControlPacket, const bool currentPacket_is_controlpacket, const DWORD lastCalledSend) {
#else
uint32 CEMSocket::GetNeededBytes(const char* sendbuffer, const uint32 sendblen, const uint32 sent, const bool currentPacket_is_controlpacket, const DWORD lastCalledSend) {
#endif
	/*
	sendLocker.Lock();
	if (byConnected == ES_DISCONNECTED) {
		sendLocker.Unlock();
		return 0;
	}
	*/

#if !defined DONT_USE_SOCKET_BUFFERING
    if (!((sendblen/*sendbuffer*/ && !currentPacket_is_controlpacket) || !standartpacket_queue.IsEmpty() || sendblenWithoutControlPacket)) {
#else
    if (!((sendbuffer && !currentPacket_is_controlpacket) || !standartpacket_queue.IsEmpty())) {
#endif
    	// No standard packet to send. Even if data needs to be sent to prevent timout, there's nothing to send.
        sendLocker.Unlock();
		return 0;
	}

#if !defined DONT_USE_SOCKET_BUFFERING
	if (((sendblen/*sendbuffer*/ && !currentPacket_is_controlpacket)) && (!controlpacket_queue.IsEmpty()))
#else
	if (((sendbuffer && !currentPacket_is_controlpacket)) && !controlpacket_queue.IsEmpty())
#endif
		m_bAccelerateUpload = true;	// We might be trying to send a block request, accelerate packet

	uint32 sendgap = ::GetTickCount() - lastCalledSend;

	uint64 timetotal = m_bAccelerateUpload?15000:30000;   //45000:90000;
	uint64 timeleft = ::GetTickCount() - lastFinishedStandard;
	uint64 sizeleft, sizetotal;
#if !defined DONT_USE_SOCKET_BUFFERING
	if (sendblen/*sendbuffer*/ && !currentPacket_is_controlpacket) {
		if(m_currentPacket_in_buffer_list.GetCount()>0) // We don't use this list on overlapped sockets
			sizeleft = m_currentPacket_in_buffer_list.GetHead()->remainpacketsize;
		else
			sizeleft = sendblen - sent;
		sizetotal = sendblen;
	}
	else {
		if (sendblenWithoutControlPacket) {
			ASSERT(m_currentPacket_in_buffer_list.GetCount() > 0); // We don't use this list on overlapped sockets but then we should not get here
			POSITION pos = m_currentPacket_in_buffer_list.GetHeadPosition();
			while (m_currentPacket_in_buffer_list.GetAt(pos)->iscontrolpacket) {
				m_currentPacket_in_buffer_list.GetNext(pos);
			}
			sizeleft = m_currentPacket_in_buffer_list.GetAt(pos)->remainpacketsize;
			sizetotal = sendblen;
		} else {
			sizeleft = sizetotal = standartpacket_queue.GetHead().packet->GetRealPacketSize();
		}
	}
#else
	if (sendbuffer && !currentPacket_is_controlpacket) {
		sizeleft = sendblen-sent;
		sizetotal = sendblen;
	}
	else {
		sizeleft = sizetotal = standartpacket_queue.GetHead().packet->GetRealPacketSize();
	}
#endif
	sendLocker.Unlock();

	if (timeleft >= timetotal)
		return (UINT)sizeleft;
	timeleft = timetotal-timeleft;
	if (timeleft*sizetotal >= timetotal*sizeleft) {
		// don't use 'GetTimeOut' here in case the timeout value is high,
		if (sendgap > SEC2MS(20))
			return 1;	// Don't let the socket itself time out - Might happen when switching from spread(non-focus) slot to trickle slot
		return 0;
	}
	uint64 decval = timeleft*sizetotal/timetotal;
	if (!decval)
		return (UINT)sizeleft;
	if (decval < sizeleft)
		return (UINT)(sizeleft-decval+1);	// Round up
	else
		return 1;
}

// pach2:
// written this overriden Receive to handle transparently FIN notifications coming from calls to recv()
// This was maybe(??) the cause of a lot of socket error, notably after a brutal close from peer
// also added trace so that we can debug after the fact ...
int CEMSocket::Receive(void* lpBuf, int nBufLen, int nFlags)
{
//	EMTrace("CEMSocket::Receive on %d, maxSize=%d",(SOCKET)this,nBufLen);
	int recvRetCode = CEncryptedStreamSocket::Receive(lpBuf,nBufLen,nFlags); // deadlake PROXYSUPPORT - changed to AsyncSocketEx
	switch (recvRetCode) {
	case 0:
		if (GetRealReceivedBytes() > 0) // we received data but it was for the underlying encryption layer - all fine
			return 0;
		//EMTrace("CEMSocket::##Received FIN on %d, maxSize=%d",(SOCKET)this,nBufLen);
		// FIN received on socket // Connection is being closed by peer
		//ASSERT (false);
		if ( 0 == AsyncSelect(FD_CLOSE|FD_WRITE) ) { // no more READ notifications ...
			//int waserr = GetLastError(); // oups, AsyncSelect failed !!!
			ASSERT(false);
		}
		return 0;
	case SOCKET_ERROR:
		switch(GetLastError()) {
		case WSANOTINITIALISED:
			ASSERT(false);
			EMTrace("CEMSocket::OnReceive:A successful AfxSocketInit must occur before using this API.");
			break;
		case WSAENETDOWN:
			ASSERT(true);
			EMTrace("CEMSocket::OnReceive:The socket %d received a net down error",(SOCKET)this);
			break;
		case WSAENOTCONN: // The socket is not connected. 
			EMTrace("CEMSocket::OnReceive:The socket %d is not connected",(SOCKET)this);
			break;
		case WSAEINPROGRESS:   // A blocking Windows Sockets operation is in progress. 
			EMTrace("CEMSocket::OnReceive:The socket %d is blocked",(SOCKET)this);
			break;
		case WSAEWOULDBLOCK:   // The socket is marked as nonblocking and the Receive operation would block. 
			EMTrace("CEMSocket::OnReceive:The socket %d would block",(SOCKET)this);
			break;
		case WSAENOTSOCK:   // The descriptor is not a socket. 
			EMTrace("CEMSocket::OnReceive:The descriptor %d is not a socket (may have been closed or never created)",(SOCKET)this);
			break;
		case WSAEOPNOTSUPP:  // MSG_OOB was specified, but the socket is not of type SOCK_STREAM. 
			break;
		case WSAESHUTDOWN:   // The socket has been shut down; it is not possible to call Receive on a socket after ShutDown has been invoked with nHow set to 0 or 2. 
			EMTrace("CEMSocket::OnReceive:The socket %d has been shut down",(SOCKET)this);
			break;
		case WSAEMSGSIZE:   // The datagram was too large to fit into the specified buffer and was truncated. 
			EMTrace("CEMSocket::OnReceive:The datagram was too large to fit and was truncated (socket %d)",(SOCKET)this);
			break;
		case WSAEINVAL:   // The socket has not been bound with Bind. 
			EMTrace("CEMSocket::OnReceive:The socket %d has not been bound",(SOCKET)this);
			break;
		case WSAECONNABORTED:   // The virtual circuit was aborted due to timeout or other failure. 
			EMTrace("CEMSocket::OnReceive:The socket %d has not been bound",(SOCKET)this);
			break;
		case WSAECONNRESET:   // The virtual circuit was reset by the remote side. 
			EMTrace("CEMSocket::OnReceive:The socket %d has not been bound",(SOCKET)this);
			break;
		default:
			EMTrace("CEMSocket::OnReceive:Unexpected socket error %x on socket %d",GetLastError(),(SOCKET)this);
			break;
		}
		break;
	default:
//		EMTrace("CEMSocket::OnReceive on %d, receivedSize=%d",(SOCKET)this,recvRetCode);
		return recvRetCode;
	}
	return SOCKET_ERROR;
}

void CEMSocket::RemoveAllLayers()
{
	CEncryptedStreamSocket::RemoveAllLayers();
	delete m_pProxyLayer;
	m_pProxyLayer = NULL;
}

int CEMSocket::OnLayerCallback(const CAsyncSocketExLayer *pLayer, int nType, int nCode, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	ASSERT( pLayer );
	if (nType == LAYERCALLBACK_STATECHANGE)
	{
		/*CString logline;
		if (pLayer==m_pProxyLayer)
		{
			//logline.Format(_T("ProxyLayer changed state from %d to %d"), wParam, nCode);
			//AddLogLine(false,logline);
		}else
			//logline.Format(_T("Layer @ %d changed state from %d to %d"), pLayer, wParam, nCode);
			//AddLogLine(false,logline);*/
		return 1;
	}
	else if (nType == LAYERCALLBACK_LAYERSPECIFIC)
	{
		if (pLayer == m_pProxyLayer)
		{
			switch (nCode)
			{
				case PROXYERROR_NOCONN:
					// We failed to connect to the proxy.
					m_bProxyConnectFailed = true;
					/* fall through */
				case PROXYERROR_REQUESTFAILED:
					// We are connected to the proxy but it failed to connect to the peer.
					if (thePrefs.GetVerbose()) {
						m_strLastProxyError = GetProxyError(nCode);
						if (lParam && ((LPCSTR)lParam)[0] != '\0') {
							m_strLastProxyError += _T(" - ");
							m_strLastProxyError += (LPCSTR)lParam;
						}
						// Appending the Winsock error code is actually not needed because that error code
						// gets reported by to the original caller anyway and will get reported eventually
						// by calling 'GetFullErrorMessage',
						/*if (wParam) {
							CString strErrInf;
							if (GetErrorMessage(wParam, strErrInf, 1))
								m_strLastProxyError += _T(" - ") + strErrInf;
						}*/
					}
					break;
				default:
					m_strLastProxyError = GetProxyError(nCode);
					LogWarning(false, _T("Proxy-Error: %s"), m_strLastProxyError);
			}
		}
	}
	return 1;
}

/**
 * Removes all packets from the standard queue that don't have to be sent for the socket to be able to send a control packet.
 *
 * Before a socket can send a new packet, the current packet has to be finished. If the current packet is part of
 * a split packet, then all parts of that split packet must be sent before the socket can send a control packet.
 *
 * This method keeps in standard queue only those packets that must be sent (rest of split packet), and removes everything
 * after it. The method doesn't touch the control packet queue.
 */
void CEMSocket::TruncateQueues() {
    sendLocker.Lock();

    // Clear the standard queue totally
    // Please note! There may still be a standardpacket in the sendbuffer variable!
	for(POSITION pos = standartpacket_queue.GetHeadPosition(); pos != NULL; )
		delete standartpacket_queue.GetNext(pos).packet;
	standartpacket_queue.RemoveAll();

    sendLocker.Unlock();
}

#ifdef _DEBUG
void CEMSocket::AssertValid() const
{
	CEncryptedStreamSocket::AssertValid();

	const_cast<CEMSocket*>(this)->sendLocker.Lock();

	ASSERT( byConnected==ES_DISCONNECTED || byConnected==ES_NOTCONNECTED || byConnected==ES_CONNECTED );
	CHECK_BOOL(m_bProxyConnectFailed);
	CHECK_PTR(m_pProxyLayer);
	(void)downloadLimit;
	CHECK_BOOL(downloadLimitEnable);
	CHECK_BOOL(pendingOnReceive);
	//char pendingHeader[PACKET_HEADER_SIZE];
	pendingHeaderSize;
	CHECK_PTR(pendingPacket);
	(void)pendingPacketSize;
	CHECK_ARR(sendbuffer, sendblen);
	(void)sent;
	controlpacket_queue.AssertValid();
	standartpacket_queue.AssertValid();
#if !defined DONT_USE_SOCKET_BUFFERING
	m_currentPacket_in_buffer_list.AssertValid();
	CHECK_BOOL(m_currentPacket_is_controlpacketOverlapped);
#else
	CHECK_BOOL(m_currentPacket_is_controlpacket);
#endif
    //(void)sendLocker;
    (void)m_numberOfSentBytesCompleteFile;
    (void)m_numberOfSentBytesPartFile;
    (void)m_numberOfSentBytesControlPacket;
#if !defined DONT_USE_SOCKET_BUFFERING
    (void)sendblenWithoutControlPacket;
	CHECK_BOOL(m_currentPackageIsFromPartFileOverlapped);
#else
    CHECK_BOOL(m_currentPackageIsFromPartFile);
#endif
    (void)lastCalledSend;
#if !defined DONT_USE_SOCKET_BUFFERING
	(void)m_actualPayloadSizeOverlapped;
#else
    (void)m_actualPayloadSize;
    (void)m_actualPayloadSizeSentForThisPacket;
#endif
    (void)m_actualPayloadSizeSent;

	const_cast<CEMSocket*>(this)->sendLocker.Unlock();
}
#endif

#ifdef _DEBUG
void CEMSocket::Dump(CDumpContext& dc) const
{
	CEncryptedStreamSocket::Dump(dc);
}
#endif

void CEMSocket::DataReceived(const BYTE*, UINT)
{
	ASSERT(0);
}

UINT CEMSocket::GetTimeOut() const
{
	return m_uTimeOut;
}

void CEMSocket::SetTimeOut(UINT uTimeOut)
{
	m_uTimeOut = uTimeOut;
}

CString CEMSocket::GetFullErrorMessage(DWORD nErrorCode)
{
	CString strError;

	// Proxy error
	if (!GetLastProxyError().IsEmpty())
	{
		strError = GetLastProxyError();
		// If we had a proxy error and the socket error is WSAECONNABORTED, we just 'aborted'
		// the TCP connection ourself - no need to show that self-created error too.
		if (nErrorCode == WSAECONNABORTED)
			return strError;
	}

	// Winsock error
	if (nErrorCode)
	{
		if (!strError.IsEmpty())
			strError += _T(": ");
		strError += GetErrorMessage(nErrorCode, 1);
	}

	return strError;
}

#if defined DONT_USE_SOCKET_BUFFERING
// increases the send buffer to a bigger size
bool CEMSocket::UseBigSendBuffer()
{
#define BIGSIZE 128 * 1024
	if (m_bUsesBigSendBuffers)
		return true;
	m_bUsesBigSendBuffers = true;
    int val = BIGSIZE;
    int vallen = sizeof(int);
	int oldval = 0;
	GetSockOpt(SO_SNDBUF, &oldval, &vallen);
	if (val > oldval)
		SetSockOpt(SO_SNDBUF, &val, sizeof(int));
	val = 0;
	vallen = sizeof(int);
	GetSockOpt(SO_SNDBUF, &val, &vallen);
#if defined(_DEBUG) || defined(_BETA) || defined(_DEVBUILD)
	if (val == BIGSIZE)
		theApp.QueueDebugLogLine(false, _T("Increased Sendbuffer for uploading socket from %uKB to %uKB"), oldval/1024, val/1024);
	else
		theApp.QueueDebugLogLine(false, _T("Failed to increase Sendbuffer for uploading socket, stays at %uKB"), oldval/1024);
#endif
	return val == BIGSIZE;
}
#endif

bool CEMSocket::IsBusyExtensiveCheck()
{
	
	if (!m_bOverlappedSending)
		//MORPH START - Added by SiRoB, Show busyTime
		/*
		return m_bBusy;
		*/
		return m_dwBusy != 0;
		//MORPH END   - Added by SiRoB, Show busyTime

	CSingleLock lockSend(&sendLocker, TRUE);
	if (m_pPendingSendOperation == NULL)
		return false;
	else
	{
		DWORD dwTransferred = 0;
		DWORD dwFlags;
		if (WSAGetOverlappedResult(GetSocketHandle(), m_pPendingSendOperation, &dwTransferred, FALSE, &dwFlags) == TRUE)
		{
			CleanUpOverlappedSendOperation(false);
			OnSend(0);
			return false;
		}
		else
		{
			int nError = WSAGetLastError();
			if (nError == WSA_IO_INCOMPLETE)
				//MORPH START - Changed by SiRoB, Show BusyTime
			{
				busyLocker.Lock();
				DWORD curTick = GetTickCount();
				if (m_dwBusy == 0) {
					m_dwNotBusyDelta = curTick - m_dwNotBusy;
					m_dwBusy = curTick;
				}
				m_dwNotBusy = 0;
				busyLocker.Unlock();
				//MORPH END   - Changed by SiRoB, Show BusyTime
				return true;
			} //MORPH - Changed by SiRoB, Show BusyTime
			else
			{
				CleanUpOverlappedSendOperation(false);
				theApp.QueueDebugLogLineEx(LOG_ERROR, _T("WSAGetOverlappedResult return error: %s"), GetErrorMessage(nError));
				return false;
			}
		}
	}
}

// won't always deliver the proper result (sometimes reports busy even if it isn't anymore and thread related errors) but doesn't needs locks or function calls
bool CEMSocket::IsBusyQuickCheck() const
{
	if (!m_bOverlappedSending)
		//MORPH START - Added by SiRoB, Show busyTime
		/*
		return m_bBusy;
		*/
		return m_dwBusy != 0;
		//MORPH END   - Added by SiRoB, Show busyTime
	else 
		return m_pPendingSendOperation != NULL;
}

void CEMSocket::CleanUpOverlappedSendOperation(bool bCancelRequestFirst)
{
	CSingleLock lockSend(&sendLocker, TRUE);
	if (m_pPendingSendOperation != NULL)
	{
		if (bCancelRequestFirst)
			CancelIo((HANDLE)GetSocketHandle());
		delete m_pPendingSendOperation;
		m_pPendingSendOperation = NULL;
		for (int i = 0; i < m_aBufferSend.GetCount(); i++)
		{
			WSABUF pDel = m_aBufferSend[i];
			delete[] pDel.buf;
		}
		 m_aBufferSend.RemoveAll();
	}
}

#ifndef USE_MORPH_READ_THREAD
bool CEMSocket::HasQueues(bool bOnlyStandardPackets) const
{
	// not trustworthy threaded? but it's ok if we don't get the correct result now and then
	return sendbuffer || standartpacket_queue.GetCount() > 0 || (controlpacket_queue.GetCount() > 0 && !bOnlyStandardPackets);
} 

bool CEMSocket::IsEnoughFileDataQueued(uint32 nMinFilePayloadBytes) const
{
	// check we have at least nMinFilePayloadBytes Payload data in our standardqueue
	for (POSITION pos = standartpacket_queue.GetHeadPosition(); pos != NULL; standartpacket_queue.GetNext(pos))
	{
		if (standartpacket_queue.GetAt(pos).actualPayloadSize > nMinFilePayloadBytes)
			return true;
		else
			nMinFilePayloadBytes -= standartpacket_queue.GetAt(pos).actualPayloadSize;
	}
	return false;
}
#endif