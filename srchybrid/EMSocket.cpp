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
#include "emsocket.h"
#include "AsyncProxySocketLayer.h"
#include "Packets.h"
#include "OtherFunctions.h"
#include "UploadBandwidthThrottler.h"
#include "Preferences.h"
#include "emuleDlg.h"
#include "Log.h"
#include "WebCache/WebCacheSocket.h" // yonatan http // MORPH - Added by Commander, WebCache 1.2e

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
#endif //EMSOCKET_DEBUG
	}
}

IMPLEMENT_DYNAMIC(CEMSocket, CAsyncSocketEx)

CEMSocket::CEMSocket(void){
	byConnected = ES_NOTCONNECTED;
	m_uTimeOut = CONNECTION_TIMEOUT; // default timeout for ed2k sockets

	// Download (pseudo) rate control	
	downloadLimit = 0;
	downloadLimitEnable = false;
	pendingOnReceive = false;

	// Download partial header
	// memset(pendingHeader, 0, sizeof(pendingHeader));
	pendingHeaderSize = 0;

	// Download partial packet
	pendingPacket = NULL;
	pendingPacketSize = 0;

	// Upload control
	sendbuffer = NULL;
	sendblen = 0;
	sent = 0;
	//m_bLinkedPackets = false;

	// deadlake PROXYSUPPORT
	m_pProxyLayer = NULL;
	m_ProxyConnectFailed = false;

    //m_startSendTick = 0;
    //m_lastSendLatency = 0;
    //m_latency_sum = 0;
    //m_wasBlocked = false;

    m_currentPacket_is_controlpacket = false;
	m_currentPackageIsFromPartFile = false;

    m_numberOfSentBytesCompleteFile = 0;
    m_numberOfSentBytesPartFile = 0;
    m_numberOfSentBytesControlPacket = 0;

    lastCalledSend = ::GetTickCount();
    lastSent = ::GetTickCount()-1000;

	m_bAccelerateUpload = false;

    m_actualPayloadSize = 0;
    m_actualPayloadSizeSent = 0;

    //MORPH - Changed by SiRoB, Show BusyTime
	/*
    m_bBusy = false;
	*/
	m_dwBusy = 0;
	m_dwBusyDelta = 1;
	m_dwNotBusy = GetTickCount();
	m_dwNotBusyDelta = 0;

    m_hasSent = false;

    //int val = 0;
    //SetSockOpt(SO_SNDBUF, &val, sizeof(int));
}

CEMSocket::~CEMSocket(){
	EMTrace("CEMSocket::~CEMSocket() on %d",(SOCKET)this);

    // need to be locked here to know that the other methods
    // won't be in the middle of things
    sendLocker.Lock();
	byConnected = ES_DISCONNECTED;
    sendLocker.Unlock();

    // now that we know no other method will keep adding to the queue
    // we can remove ourself from the queue
    theApp.uploadBandwidthThrottler->RemoveFromAllQueues(this);

    ClearQueues();
	RemoveAllLayers(); // deadlake PROXYSUPPORT
	AsyncSelect(0);
}

// deadlake PROXYSUPPORT
// By Maverick: Connection initialisition is done by class itself
BOOL CEMSocket::Connect(LPCTSTR lpszHostAddress, UINT nHostPort)
{
	InitProxySupport();
	return CAsyncSocketEx::Connect(lpszHostAddress, nHostPort);
}
// end deadlake

// deadlake PROXYSUPPORT
// By Maverick: Connection initialisition is done by class itself
//BOOL CEMSocket::Connect(LPCTSTR lpszHostAddress, UINT nHostPort)
BOOL CEMSocket::Connect(SOCKADDR* pSockAddr, int iSockAddrLen)
{
	InitProxySupport();
	return CAsyncSocketEx::Connect(pSockAddr, iSockAddrLen);
}
// end deadlake

void CEMSocket::InitProxySupport()
{
	// ProxyInitialisation
	const ProxySettings& settings = thePrefs.GetProxy();
	m_ProxyConnectFailed = false;
	if (settings.UseProxy && settings.type != PROXYTYPE_NOPROXY)
	{
		Close();

		m_pProxyLayer=new CAsyncProxySocketLayer;
		switch (settings.type)
		{
			case PROXYTYPE_SOCKS4:
				m_pProxyLayer->SetProxy(PROXYTYPE_SOCKS4,settings.name,settings.port);
				break;
			case PROXYTYPE_SOCKS4A:
				m_pProxyLayer->SetProxy(PROXYTYPE_SOCKS4A,settings.name,settings.port);
				break;
			case PROXYTYPE_SOCKS5:
				if (settings.EnablePassword)
					m_pProxyLayer->SetProxy(PROXYTYPE_SOCKS5,settings.name, settings.port, settings.user, settings.password);
				else
					m_pProxyLayer->SetProxy(PROXYTYPE_SOCKS5,settings.name,settings.port);
				break;
			case PROXYTYPE_HTTP11:
				if (settings.EnablePassword)
					m_pProxyLayer->SetProxy(PROXYTYPE_HTTP11,settings.name,settings.port, settings.user, settings.password);
				else
					m_pProxyLayer->SetProxy(PROXYTYPE_HTTP11,settings.name,settings.port);
				break;
			default: ASSERT(FALSE);
		}
		AddLayer(m_pProxyLayer);

		// Connection Initialisation
		Create();
		AsyncSelect(FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE);
	}
}

void CEMSocket::ClearQueues(){
	EMTrace("CEMSocket::ClearQueues on %d",(SOCKET)this);
	for(POSITION pos = controlpacket_queue.GetHeadPosition(); pos != NULL; )
		delete controlpacket_queue.GetNext(pos);
	controlpacket_queue.RemoveAll();

	for(POSITION pos = standartpacket_queue.GetHeadPosition(); pos != NULL; )
		delete standartpacket_queue.GetNext(pos).packet;
	standartpacket_queue.RemoveAll();

	// Download (pseudo) rate control	
	downloadLimit = 0;
	downloadLimitEnable = false;
	pendingOnReceive = false;

	// Download partial header
	// memset(pendingHeader, 0, sizeof(pendingHeader));
	pendingHeaderSize = 0;

	// Download partial packet
	if(pendingPacket != NULL){
		delete pendingPacket;
		pendingPacket = NULL;
		pendingPacketSize = 0;
	}

	// Upload control
	if(sendbuffer != NULL){
		delete[] sendbuffer;
		sendbuffer = NULL;
	}
	sendblen = 0;
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

	CAsyncSocketEx::OnClose(nErrorCode); // deadlake changed socket to PROXYSUPPORT ( AsyncSocketEx )
	RemoveAllLayers(); // deadlake PROXYSUPPORT
	ClearQueues();
};

BOOL CEMSocket::AsyncSelect(long lEvent){
	if (lEvent&FD_READ)
		EMTrace("  FD_READ");
	if (lEvent&FD_CLOSE)
		EMTrace("  FD_CLOSE");
	if (lEvent&FD_WRITE)
		EMTrace("  FD_WRITE");
	// deadlake changed to AsyncSocketEx PROXYSUPPORT
	if (m_SocketData.hSocket != INVALID_SOCKET)
		return CAsyncSocketEx::AsyncSelect(lEvent);
	return true;
}

void CEMSocket::OnReceive(int nErrorCode){
	// the 2 meg size was taken from another place
	static char GlobalReadBuffer[2000000];

	// Check for an error code
	if(nErrorCode != 0){
		OnError(nErrorCode);
		return;
	}
	
	// Check current connection state
	if(byConnected == ES_DISCONNECTED){
		return;
	}
	else {	
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
	if(ret == SOCKET_ERROR || ret == 0){
		return;
	}

	// Bandwidth control
	if(downloadLimitEnable == true){
		// Update limit
		downloadLimit -= ret;
	}

	// CPU load improvement
	// Detect if the socket's buffer is empty (or the size did match...)
	pendingOnReceive = (ret == readMax) ? true : false;

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

	// MORPH START - Added by Commander, WebCache 1.2e
	if( *(uint32*)GlobalReadBuffer == ' TEG' ) {
		CWebCacheUpSocket* WCSocket;
		if( !IsKindOf( RUNTIME_CLASS( CWebCacheUpSocket ) ) ) { // yonatan http - WC-TODO: make sure this is a new, incoming connection?
			// Turn this into a CWebCacheUpSocket and attach to client.
			SOCKET s = Detach(); // Detach socket from this (CClientReqSocket)
			WCSocket = new CWebCacheUpSocket(); // Create a new WebCache socket
			WCSocket->Attach( s, FD_WRITE|FD_READ|FD_CLOSE );
			if( WCSocket->ProcessFirstHttpGet( GlobalReadBuffer, ret ) ) {
				delete this;
				return;
			}
		} else {
			static_cast<CWebCacheUpSocket*>(this)->ProcessHttpPacket( (BYTE*)GlobalReadBuffer, ret );
		}
	}
	// MORPH END - Added by Commander, WebCache 1.2e

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
// MORPH START - Added by SiRoB, WebCache 1.2f
// WebCache ////////////////////////////////////////////////////////////////////////////////////
				case OP_WEBCACHEPACKEDPROT:
				case OP_WEBCACHEPROT: // yonatan - webcache protocol packets
// MORPH END   - Added by SiRoB, WebCache 1.2f
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
// WebCache ////////////////////////////////////////////////////////////////////////////////////
	// JP added netfinity download throttler
	// MOD BEGIN netfinity: Accumulate download limits
	downloadLimit += limit; 
	downloadLimitEnable = true;	
	if(downloadLimit > 20 * limit && downloadLimit > 4500) // Allow a maximum of 2.0 sec to accumulate or 3 * MTU
		downloadLimit = max(20 * limit, 4500); 
	// MOD END netfinity

/*(original code)
//	downloadLimit = limit;
//	downloadLimitEnable = true;	
*/
	
	
	// CPU load improvement
	if(limit > 0 && pendingOnReceive == true){
		OnReceive(0);
	}
}

void CEMSocket::DisableDownloadLimit(){
	downloadLimitEnable = false;

	// CPU load improvement
	if(pendingOnReceive == true){
		OnReceive(0);
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
void CEMSocket::SendPacket(Packet* packet, bool delpacket, bool controlpacket, uint32 actualPayloadSize){
	//EMTrace("CEMSocket::OnSenPacked1 linked: %i, controlcount %i, standartcount %i, isbusy: %i",m_bLinkedPackets, controlpacket_queue.GetCount(), standartpacket_queue.GetCount(), IsBusy());

    sendLocker.Lock();

    if (byConnected == ES_DISCONNECTED) {
        sendLocker.Unlock();
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

        if (controlpacket) {
	        controlpacket_queue.AddTail(packet);

            // queue up for controlpacket
            theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
	    } else {
            bool first = !((sendbuffer && !m_currentPacket_is_controlpacket) || !standartpacket_queue.IsEmpty());
            StandardPacketQueueEntry queueEntry = { actualPayloadSize, packet };
		    standartpacket_queue.AddTail(queueEntry);

            // reset timeout for the first time
            if (first) {
                lastFinishedStandard = ::GetTickCount();
                m_bAccelerateUpload = true;	// Always accelerate first packet in a block
            }
	    }
    }

    sendLocker.Unlock();
}

uint64 CEMSocket::GetSentBytesCompleteFileSinceLastCallAndReset() {
    sendLocker.Lock();

    uint64 sentBytes = m_numberOfSentBytesCompleteFile;
    m_numberOfSentBytesCompleteFile = 0;

    sendLocker.Unlock();

    return sentBytes;
}

uint64 CEMSocket::GetSentBytesPartFileSinceLastCallAndReset() {
    sendLocker.Lock();

    uint64 sentBytes = m_numberOfSentBytesPartFile;
    m_numberOfSentBytesPartFile = 0;

    sendLocker.Unlock();

    return sentBytes;
}

uint64 CEMSocket::GetSentBytesControlPacketSinceLastCallAndReset() {
    sendLocker.Lock();

    uint64 sentBytes = m_numberOfSentBytesControlPacket;
    m_numberOfSentBytesControlPacket = 0;

    sendLocker.Unlock();

    return sentBytes;
}

uint64 CEMSocket::GetSentPayloadSinceLastCallAndReset() {
    sendLocker.Lock();

    uint64 sentBytes = m_actualPayloadSizeSent;
    m_actualPayloadSizeSent = 0;

    sendLocker.Unlock();

    return sentBytes;
}

void CEMSocket::OnSend(int nErrorCode){
    //onSendWillBeCalledOuter = false;

    if (nErrorCode){
		OnError(nErrorCode);
		return;
	}

	//EMTrace("CEMSocket::OnSend linked: %i, controlcount %i, standartcount %i, isbusy: %i",m_bLinkedPackets, controlpacket_queue.GetCount(), standartpacket_queue.GetCount(), IsBusy());

    sendLocker.Lock();

    //MORPH - Changed by SiRoB, Show BusyTime
	/*
    m_bBusy = false;
	*/
	DWORD curTick = GetTickCount();
	if (m_dwBusy) {
		m_dwBusyDelta = curTick-m_dwBusy;
		m_dwNotBusy = curTick;
	}
	m_dwBusy = 0;
	
    // stopped sending here.
    //StoppedSendSoUpdateStats();

    if (byConnected == ES_DISCONNECTED) {
        sendLocker.Unlock();
		return;
    } else
		byConnected = ES_CONNECTED;

	if(m_currentPacket_is_controlpacket) {
		// queue up for control packet
        theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
    }

    sendLocker.Unlock();
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
SocketSentBytes CEMSocket::Send(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket) {
	//EMTrace("CEMSocket::Send linked: %i, controlcount %i, standartcount %i, isbusy: %i",m_bLinkedPackets, controlpacket_queue.GetCount(), standartpacket_queue.GetCount(), IsBusy());
    sendLocker.Lock();

	if (byConnected == ES_DISCONNECTED) {
        sendLocker.Unlock();
        SocketSentBytes returnVal = { false, 0, 0 };
        return returnVal;
    //MORPH - Changed by SiRoB, Show BusyTime
	//} else if (m_bBusy && onlyAllowedToSendControlPacket /*&& ::GetTickCount() - lastSent < 50*/) {
	} else if (m_dwBusy && onlyAllowedToSendControlPacket /*&& ::GetTickCount() - lastSent < 50*/) {
	    sendLocker.Unlock();
        SocketSentBytes returnVal = { true, 0, 0 };
        return returnVal;
    }

    if(minFragSize < 1) {
        minFragSize = 1;
    }

    maxNumberOfBytesToSend = GetNextFragSize(maxNumberOfBytesToSend, minFragSize);

    bool bWasLongTimeSinceSend = (::GetTickCount() - lastSent) > 1000;

    //lastCalledSend = ::GetTickCount();

    bool anErrorHasOccured = false;
    uint32 sentStandardPacketBytesThisCall = 0;
    uint32 sentControlPacketBytesThisCall = 0;

    while(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall < maxNumberOfBytesToSend && anErrorHasOccured == false && // don't send more than allowed. Also, there should have been no error in earlier loop
          (!controlpacket_queue.IsEmpty() || !standartpacket_queue.IsEmpty() || sendbuffer != NULL) && // there must exist something to send
          (onlyAllowedToSendControlPacket == false || // this means we are allowed to send both types of packets, so proceed
           sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall > 0 && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) % minFragSize != 0 ||
           sendbuffer == NULL && !controlpacket_queue.IsEmpty() || // There's a control packet in queue, and we are not currently sending anything, so we will handle the control packet next
           sendbuffer != NULL && m_currentPacket_is_controlpacket == true || // We are in the progress of sending a control packet. We are always allowed to send those
           sendbuffer != NULL && m_currentPacket_is_controlpacket == false && bWasLongTimeSinceSend && !controlpacket_queue.IsEmpty() && standartpacket_queue.IsEmpty() && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) < minFragSize // We have waited to long to clean the current packet (which may be a standard packet that is in the way). Proceed no matter what the value of onlyAllowedToSendControlPacket.
          )
         ) {

        // If we are currently not in the progress of sending a packet, we will need to find the next one to send
        if(sendbuffer == NULL) {
            Packet* curPacket = NULL;
            if(!controlpacket_queue.IsEmpty()) {
                // There's a control packet to send
                m_currentPacket_is_controlpacket = true;
                curPacket = controlpacket_queue.RemoveHead();
            } else if(!standartpacket_queue.IsEmpty() /*&& onlyAllowedToSendControlPacket == false*/) {
                // There's a standard packet to send
                m_currentPacket_is_controlpacket = false;
                StandardPacketQueueEntry queueEntry = standartpacket_queue.RemoveHead();
                curPacket = queueEntry.packet;
                m_actualPayloadSize = queueEntry.actualPayloadSize;

                // remember this for statistics purposes.
                m_currentPackageIsFromPartFile = curPacket->IsFromPF();
            } else {
                // Just to be safe. Shouldn't happen?
                sendLocker.Unlock();

                // if we reach this point, then there's something wrong with the while condition above!
                ASSERT(0);
                theApp.QueueDebugLogLine(true,_T("EMSocket: Couldn't get a new packet! There's an error in the first while condition in EMSocket::Send()"));

                SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
                return returnVal;
            }

            // We found a package to send. Get the data to send from the
            // package container and dispose of the container.
            sendblen = curPacket->GetRealPacketSize();
            sendbuffer = curPacket->DetachPacket();
            sent = 0;
            delete curPacket;
        }

        // At this point we've got a packet to send in sendbuffer. Try to send it. Loop until entire packet
        // is sent, or until we reach maximum bytes to send for this call, or until we get an error.
        // NOTE! If send would block (returns WSAEWOULDBLOCK), we will return from this method INSIDE this loop.
        while (sent < sendblen &&
               sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall < maxNumberOfBytesToSend &&
               (
                onlyAllowedToSendControlPacket == false || // this means we are allowed to send both types of packets, so proceed
                m_currentPacket_is_controlpacket ||
                bWasLongTimeSinceSend && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) < minFragSize ||
                (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) % minFragSize != 0
               ) &&
               anErrorHasOccured == false) {
		    uint32 tosend = sendblen-sent;
            if(!onlyAllowedToSendControlPacket || m_currentPacket_is_controlpacket) {
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

            lastSent = ::GetTickCount();

		    uint32 result = CAsyncSocketEx::Send(sendbuffer+sent,tosend); // deadlake PROXYSUPPORT - changed to AsyncSocketEx
		    if (result == (uint32)SOCKET_ERROR){
			    uint32 error = GetLastError();
			    if (error == WSAEWOULDBLOCK){
                    //MORPH - Changed by SiRoB, Show BusyTime
					/*
                    m_bBusy = true;
					*/
					DWORD curTick = GetTickCount();
					if (m_dwBusy == 0) {
						m_dwNotBusyDelta = curTick-m_dwNotBusy;
						m_dwBusy = curTick;
					}
					m_dwNotBusy = 0;
					
					
                    //m_wasBlocked = true;
                    sendLocker.Unlock();

                    SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
                    return returnVal; // Send() blocked, onsend will be called when ready to send again
			    } else{
                    // Send() gave an error
                    anErrorHasOccured = true;
                    //DEBUG_ONLY( AddDebugLogLine(true,"EMSocket: An error has occured: %i", error) );
                }
            } else {
                // we managed to send some bytes. Perform bookkeeping.
                //MORPH - Changed by SiRoB, Show BusyTime
				/*
                m_bBusy = false;
				*/
				DWORD curTick = GetTickCount();
				if (m_dwBusy) {
					m_dwBusyDelta = curTick-m_dwBusy;
					m_dwNotBusy = curTick;
				}
				m_dwBusy = 0;

				m_hasSent = true;
                lastCalledSend = ::GetTickCount();

                sent += result;

                // Log send bytes in correct class
                if(m_currentPacket_is_controlpacket == false) {
                    sentStandardPacketBytesThisCall += result;

                    if(m_currentPackageIsFromPartFile == true) {
                        m_numberOfSentBytesPartFile += result;
                    } else {
                        m_numberOfSentBytesCompleteFile += result;
                    }
                } else {
                    sentControlPacketBytesThisCall += result;
                    m_numberOfSentBytesControlPacket += result;
                }
            }
	    }

        if (sent == sendblen){
            // we are done sending the current package. Delete it and set
            // sendbuffer to NULL so a new packet can be fetched.
		    delete[] sendbuffer;
		    sendbuffer = NULL;
			sendblen = 0;

            if(!m_currentPacket_is_controlpacket) {
                m_actualPayloadSizeSent += m_actualPayloadSize;
                m_actualPayloadSize = 0;

                lastFinishedStandard = ::GetTickCount(); // reset timeout
                m_bAccelerateUpload = false; // Safe until told otherwise
            }

            sent = 0;
        }
    }


    if(onlyAllowedToSendControlPacket && (!controlpacket_queue.IsEmpty() || sendbuffer != NULL && m_currentPacket_is_controlpacket)) {
        // enter control packet send queue
        // we might enter control packet queue several times for the same package,
        // but that costs very little overhead. Less overhead than trying to make sure
        // that we only enter the queue once.
        theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
    }

    //CleanSendLatencyList();

    sendLocker.Unlock();

    SocketSentBytes returnVal = { !anErrorHasOccured, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
    return returnVal;
}

uint32 CEMSocket::GetNextFragSize(uint32 current, uint32 minFragSize) {
    uint32 ret;
	if(current % minFragSize == 0) {
        ret = current;
    } else {
        ret = minFragSize*(current/minFragSize+1);
    }
	//MORPH START - Added by SiRoB, Anti WSAEWOULDBLOCK ensure that socket buffer is larger than app one
	if (current >= 65535)
		ret = 65535-1;
		return ret;
	//MORPH END   - Added by SiRoB, Anti WSAEWOULDBLOCK ensure that socket buffer is larger than app one
}

/**
 * Decides the (minimum) amount the socket needs to send to prevent timeout.
 * 
 * @author SlugFiller
 */
//MORPH - Changed by SiRoB, Scale to lowspeed
/*
uint32 CEMSocket::GetNeededBytes() {
*/
uint32 CEMSocket::GetNeededBytes(bool lowspeed) {
	sendLocker.Lock();
	if (byConnected == ES_DISCONNECTED) {
		sendLocker.Unlock();
		return 0;
	}

    if (!((sendbuffer && !m_currentPacket_is_controlpacket) || !standartpacket_queue.IsEmpty())) {
    	// No standard packet to send. Even if data needs to be sent to prevent timout, there's nothing to send.
        sendLocker.Unlock();
		return 0;
	}

	if (((sendbuffer && !m_currentPacket_is_controlpacket)) && !controlpacket_queue.IsEmpty())
		m_bAccelerateUpload = true;	// We might be trying to send a block request, accelerate packet

	uint32 sendgap = ::GetTickCount() - lastCalledSend;

	uint64 timetotal = m_bAccelerateUpload?45000:90000;
	//MORPH START - Added by SiRoB, 1kB/s and 0.1kB/s for lowspeed
	if (!lowspeed) timetotal = m_bAccelerateUpload?4500:9000;
	//MORPH END   - Added by SiRoB, 1kB/s and 0.1kB/s for lowspeed
	uint64 timeleft = ::GetTickCount() - lastFinishedStandard;
	uint64 sizeleft, sizetotal;
	if (sendbuffer && !m_currentPacket_is_controlpacket) {
		sizeleft = sendblen-sent;
		sizetotal = sendblen;
	}
	else {
		sizeleft = sizetotal = standartpacket_queue.GetHead().packet->GetRealPacketSize();
	}
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
	int recvRetCode = CAsyncSocketEx::Receive(lpBuf,nBufLen,nFlags); // deadlake PROXYSUPPORT - changed to AsyncSocketEx
	switch (recvRetCode) {
	case 0:
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


// deadlake PROXYSUPPORT ( RESETS LAYER CHAIN BY MAVERICK )
void CEMSocket::RemoveAllLayers()
{
	CAsyncSocketEx::RemoveAllLayers();
	
	// ProxyLayer Destruction
	if (m_pProxyLayer) 
	{
		delete m_pProxyLayer;
		m_pProxyLayer = NULL;
	}
}

int CEMSocket::OnLayerCallback(const CAsyncSocketExLayer *pLayer, int nType, int nParam1, int nParam2)
{
	ASSERT(pLayer);
	if (nType==LAYERCALLBACK_STATECHANGE)
	{
		CString logline;
		if (pLayer==m_pProxyLayer)
		{
			//logline.Format(_T("ProxyLayer changed state from %d to %d"), nParam2, nParam1);
			//AddLogLine(false,logline);
		}else
			//logline.Format(_T("Layer @ %d changed state from %d to %d"), pLayer, nParam2, nParam1);
			//AddLogLine(false,logline);
		return 1;
	}
	else if (nType==LAYERCALLBACK_LAYERSPECIFIC)
	{
		if (pLayer==m_pProxyLayer)
		{
			switch (nParam1)
			{
				// changed by deadlake -> errormessages could be ignored -> there's not a problem with the connection - 
				// only the proxyserver handles the connections to low ( small bandwidth? )
				case PROXYERROR_NOCONN:{
					//TODO: This error message(s) should be outputed only during startup - otherwise we'll see a lot of
					//them in the log window which would be of no use.
					if (thePrefs.GetShowProxyErrors()){
						CString strError(_T("Can't connect to proxy"));
						CString strErrInf;
						if (nParam2 && GetErrorMessage(nParam2, strErrInf))
							strError += _T(" - ") + strErrInf;
						LogWarning(false, _T("%s"), strError);
					}
					break;
				}
				case PROXYERROR_REQUESTFAILED:{
					//TODO: This error message(s) should be outputed only during startup - otherwise we'll see a lot of
					//them in the log window which would be of no use.
					if (thePrefs.GetShowProxyErrors()){
						CString strError(_T("Proxy request failed"));
						if (nParam2){
							strError += _T(" - ");
							strError += (LPCSTR)nParam2;
						}
						LogWarning(false, _T("%s"), strError);
					}
					break;
				}
				case PROXYERROR_AUTHTYPEUNKNOWN:
					LogWarning(false,_T("Required authtype reported by proxy server is unknown or unsupported"));
					break;
				case PROXYERROR_AUTHFAILED:
					LogWarning(false,_T("Authentification failed"));
					break;
				case PROXYERROR_AUTHNOLOGON:
					LogWarning(false,_T("Proxy requires authentification"));
					break;
				case PROXYERROR_CANTRESOLVEHOST:
					LogWarning(false,_T("Can't resolve host of proxy"));
					break;
				default:{
					LogWarning(false,_T("Proxy error - %s"), GetProxyError(nParam1));
				}
			}
		}
	}
	return 1;
}
// end deadlake

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
	CAsyncSocketEx::AssertValid();

	const_cast<CEMSocket*>(this)->sendLocker.Lock();

	ASSERT( byConnected==ES_DISCONNECTED || byConnected==ES_NOTCONNECTED || byConnected==ES_CONNECTED );
	CHECK_BOOL(m_ProxyConnectFailed);
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
	CHECK_BOOL(m_currentPacket_is_controlpacket);
    //(void)sendLocker;
    (void)m_numberOfSentBytesCompleteFile;
    (void)m_numberOfSentBytesPartFile;
    (void)m_numberOfSentBytesControlPacket;
    CHECK_BOOL(m_currentPackageIsFromPartFile);
    (void)lastCalledSend;
    (void)m_actualPayloadSize;
    (void)m_actualPayloadSizeSent;

	const_cast<CEMSocket*>(this)->sendLocker.Unlock();
}
#endif

#ifdef _DEBUG
void CEMSocket::Dump(CDumpContext& dc) const
{
	CAsyncSocketEx::Dump(dc);
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
