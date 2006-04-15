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
		HANDLE hFile = CreateFile(_T("c:\\EMSocket.log"),           // open MYFILE.TXT 
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

IMPLEMENT_DYNAMIC(CEMSocket, CAsyncSocketEx)

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

    m_currentPacket_is_controlpacket = false;
	m_currentPackageIsFromPartFile = false;

    m_numberOfSentBytesCompleteFile = 0;
    m_numberOfSentBytesPartFile = 0;
    m_numberOfSentBytesControlPacket = 0;

    lastCalledSend = ::GetTickCount();

	m_bAccelerateUpload = false;

    m_actualPayloadSize = 0;
    m_actualPayloadSizeSentForThisPacket = 0;
    m_actualPayloadSizeSent = 0;

    //MORPH - Changed by SiRoB, Show BusyTime
	/*
    m_bBusy = false;
	*/
	DWORD curTick = GetTickCount();
	m_dwNotBusy = 0;
	m_dwNotBusyDelta = curTick-m_dwNotBusy;
	m_dwBusy = curTick;
	m_dwBusyDelta = 1;
	m_hasSent = false;

    int val = 256*1024;
    HRESULT sndBufResult = SetSockOpt(SO_SNDBUF, &val, sizeof(int));
    if(sndBufResult == SOCKET_ERROR) {
        CString pstrReason = GetErrorMessage(WSAGetLastError(), 1);
        theApp.QueueDebugLogLine(false,_T("CEMSocket::CEMSocket(): Couldn't set SO_SNDBUF: %s"), pstrReason);
    }

    HRESULT rcvBufResult = SetSockOpt(SO_RCVBUF, &val, sizeof(int));
    if(rcvBufResult == SOCKET_ERROR) {
        CString pstrReason = GetErrorMessage(WSAGetLastError(), 1);
        theApp.QueueDebugLogLine(false,_T("CEMSocket::CEMSocket(): Couldn't set SO_RCVBUF: %s"), pstrReason);
    }
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
BOOL CEMSocket::Connect(LPCSTR lpszHostAddress, UINT nHostPort)
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
	m_bProxyConnectFailed = false;

	// ProxyInitialisation
	const ProxySettings& settings = thePrefs.GetProxySettings();
	if (settings.UseProxy && settings.type != PROXYTYPE_NOPROXY)
	{
		Close();

		m_pProxyLayer=new CAsyncProxySocketLayer;
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
	sent = 0;
}

void CEMSocket::OnConnect(int nErrorCode) {
    CAsyncSocketEx::OnConnect(nErrorCode);
	if (nErrorCode == 0)
	{
        sendLocker.Lock();
		if(byConnected == ES_NOTCONNECTED) {
			byConnected = ES_CONNECTED;
			if(!controlpacket_queue.IsEmpty())
				theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, true);
		}

		//MORPH - Added by SiRoB, Show BusyTime
		/*
		m_bBusy = false;
		*/
		DWORD curTick = GetTickCount();
		if (m_dwBusy) {
			m_dwBusyDelta = curTick-m_dwBusy;
			m_dwNotBusy = curTick;
		}
		m_dwBusy = 0;
		//MORPH - Added by SiRoB, Show BusyTime
		sendLocker.Unlock();
	}
    //theApp.QueueDebugLogLine(false,_T("CEMSocket::OnConnect(%i)"), nErrorCode);
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
    sendLocker.Lock();
	if(byConnected == ES_DISCONNECTED){
        sendLocker.Unlock();
		return;
	}
	else if(byConnected == ES_NOTCONNECTED) {	
		byConnected = ES_CONNECTED; // ES_DISCONNECTED, ES_NOTCONNECTED, ES_CONNECTED

	}
    sendLocker.Unlock();

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

			if (byConnected == ES_CONNECTED) {
				// queue up for controlpacket
				theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
			}
	    } else {
            //bool first = !((sendbuffer && !m_currentPacket_is_controlpacket) || !standartpacket_queue.IsEmpty());
            StandardPacketQueueEntry queueEntry = { actualPayloadSize, packet };
		    standartpacket_queue.AddTail(queueEntry);

            // reset timeout for the first time
            //if (first) {
            //    lastFinishedStandard = ::GetTickCount();
            //    m_bAccelerateUpload = true;	// Always accelerate first packet in a block
            //}
	    }
    }

    sendLocker.Unlock();
}

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

uint64 CEMSocket::GetSentPayloadSinceLastCallAndReset() {
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

	sendLocker.Lock();
    if (byConnected == ES_DISCONNECTED) {
        sendLocker.Unlock();
		return;
	}
	
	bool signalNotBusy = m_dwBusy>0;
	
	//MORPH - Added by SiRoB, Show BusyTime
	/*
	m_bBusy = false;
	*/
	DWORD curTick = GetTickCount();
	if (m_dwBusy) {
		m_dwBusyDelta = curTick-m_dwBusy;
		m_dwNotBusy = curTick;
	}
	m_dwBusy = 0;
	//MORPH - Added by SiRoB, Show BusyTime
	
	sendLocker.Unlock();

	if(signalNotBusy) {
        theApp.uploadBandwidthThrottler->SignalNoLongerBusy();
    }
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

    if (maxNumberOfBytesToSend == 0 && ::GetTickCount() - lastCalledSend < SEC2MS(1)) {
        SocketSentBytes returnVal = { true, 0, 0 };
        return returnVal;
    }

	if (byConnected == ES_DISCONNECTED) {
		SocketSentBytes returnVal = { false, 0, 0 };
		return returnVal;
	}

    bool anErrorHasOccured = false;
    uint32 errorThatOccured = 0;
    uint32 sentStandardPacketBytesThisCall = 0;
    uint32 sentControlPacketBytesThisCall = 0;
	
    // This unlocked byConnected check may be OK since the sendLock is taken before the queues are accessed, and they are only removed from.
    //MORPH - Changed by SiRoB, Show BusyTime
	/*
	if(byConnected == ES_CONNECTED && (!m_dwBusy || !onlyAllowedToSendControlPacket)) {
	*/
	if(!m_dwBusy || !onlyAllowedToSendControlPacket) {
	    if(minFragSize < 1) {
            minFragSize = 1;
        }

        if(maxNumberOfBytesToSend == 0) {
            maxNumberOfBytesToSend = GetNeededBytes(sendbuffer, sendblen, sent, m_currentPacket_is_controlpacket, lastCalledSend);
        }

    maxNumberOfBytesToSend = GetNextFragSize(maxNumberOfBytesToSend, minFragSize);

        bool bWasLongTimeSinceSend = (::GetTickCount() - lastCalledSend) > 1000;

        sendLocker.Lock();

        while(sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall < maxNumberOfBytesToSend && anErrorHasOccured == false && // don't send more than allowed. Also, there should have been no error in earlier loop
              (sendbuffer != NULL || !controlpacket_queue.IsEmpty() || !standartpacket_queue.IsEmpty()) && // there must exist something to send
               (onlyAllowedToSendControlPacket == false || // this means we are allowed to send both types of packets, so proceed
                sendbuffer != NULL && m_currentPacket_is_controlpacket == true || // We are in the progress of sending a control packet. We are always allowed to send those
                sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall > 0 && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) % minFragSize != 0 ||
                sendbuffer == NULL && !controlpacket_queue.IsEmpty() || // There's a control packet in queue, and we are not currently sending anything, so we will handle the control packet next
                sendbuffer != NULL && m_currentPacket_is_controlpacket == false && bWasLongTimeSinceSend && !controlpacket_queue.IsEmpty() /*&& standartpacket_queue.IsEmpty()*/ && (sentStandardPacketBytesThisCall + sentControlPacketBytesThisCall) < minFragSize // We have waited to long to clean the current packet (which may be a standard packet that is in the way). Proceed no matter what the value of onlyAllowedToSendControlPacket.
               )
              ) {

            // If we are currently not in the progress of sending a packet, we will need to find the next one to send
            if(sendbuffer == NULL) {
                Packet* curPacket = NULL;
                if(!controlpacket_queue.IsEmpty()) {
                    // There's a control packet to send
                    m_currentPacket_is_controlpacket = true;
                    curPacket = controlpacket_queue.RemoveHead();
                } else if(!standartpacket_queue.IsEmpty()) {
                    // There's a standard packet to send
                    m_currentPacket_is_controlpacket = false;
                    StandardPacketQueueEntry queueEntry = standartpacket_queue.RemoveHead();
                    curPacket = queueEntry.packet;
                    m_actualPayloadSize = queueEntry.actualPayloadSize;
                    m_actualPayloadSizeSentForThisPacket = 0;

                    // remember this for statistics purposes.
                    m_currentPackageIsFromPartFile = curPacket->IsFromPF();
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
                	SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall  + ((sentStandardPacketBytesThisCall/minFragSize)+((sentStandardPacketBytesThisCall%minFragSize)?1:0)) * 40, sentControlPacketBytesThisCall + ((sentControlPacketBytesThisCall/minFragSize)+((sentControlPacketBytesThisCall%minFragSize)?1:0)) * 40};
                	return returnVal;
		}

	            	// We found a package to send. Get the data to send from the
	            	// package container and dispose of the container.
	            	sendblen = curPacket->GetRealPacketSize();
			sendbuffer = curPacket->DetachPacket();
	            	sent = 0;
	            	delete curPacket;
		}

            sendLocker.Unlock();

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

		    //MORPH START - Added by SiRoB, Anti WSAEWOULDBLOCK ensure that socket buffer is larger than app one
			ASSERT(tosend<256*1024);
			if (tosend >= 128*1024)
				tosend = 128*1024-1;
			//MORPH END   - Added by SiRoB, Anti WSAEWOULDBLOCK ensure that socket buffer is larger than app one
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
                    //sendLocker.Unlock();

                    //MORPH - Changed by SiRoB, Take into account IP+TCP Header
                    /*
                    SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
                    */
                    SocketSentBytes returnVal = { true, sentStandardPacketBytesThisCall  + ((sentStandardPacketBytesThisCall/minFragSize)+((sentStandardPacketBytesThisCall%minFragSize)?1:0)) * 40, sentControlPacketBytesThisCall + ((sentControlPacketBytesThisCall/minFragSize)+((sentControlPacketBytesThisCall%minFragSize)?1:0)) * 40};
                    return returnVal; // Send() blocked, onsend will be called when ready to send again
			    } else{
                    // Send() gave an error
                    anErrorHasOccured = true;
                        errorThatOccured = error;
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

                // Log send bytes in correct class
                if(m_currentPacket_is_controlpacket == false) {
                    sentStandardPacketBytesThisCall += result;

                    if(m_currentPackageIsFromPartFile == true) {
                            statsLocker.Lock();
                        m_numberOfSentBytesPartFile += result;
                            statsLocker.Unlock();
                    } else {
                            statsLocker.Lock();
                        m_numberOfSentBytesCompleteFile += result;
                            statsLocker.Unlock();
                    }
                } else {
                    sentControlPacketBytesThisCall += result;

                        statsLocker.Lock();
                    m_numberOfSentBytesControlPacket += result;
                        statsLocker.Unlock();
                }
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

            if(!m_currentPacket_is_controlpacket) {
                    if(m_actualPayloadSizeSentForThisPacket < m_actualPayloadSize) {
                        UINT rest = (m_actualPayloadSize-m_actualPayloadSizeSentForThisPacket);
                        statsLocker.Lock();
                        m_actualPayloadSizeSent += rest;
                        statsLocker.Unlock();

                        m_actualPayloadSizeSentForThisPacket += rest;
                    }

                    ASSERT(m_actualPayloadSizeSentForThisPacket == m_actualPayloadSize);

                m_actualPayloadSize = 0;
                    m_actualPayloadSizeSentForThisPacket = 0;

                lastFinishedStandard = ::GetTickCount(); // reset timeout
                m_bAccelerateUpload = false; // Safe until told otherwise
            }

            sent = 0;
        }

            // lock before checking the loop condition
            sendLocker.Lock();
    }

        sendLocker.Unlock();
    }

	//!onlyAllowedToSendControlPacket means we still got the socket in Standardlist
	if (onlyAllowedToSendControlPacket) {
		sendLocker.Lock();
        if(sendbuffer != NULL || !controlpacket_queue.IsEmpty()) {
            theApp.uploadBandwidthThrottler->QueueForSendingControlPacket(this, HasSent());
        }
        sendLocker.Unlock();
    }

    //MORPH - Changed by SiRoB, Take into account IP+TCP Header
    /*
    SocketSentBytes returnVal = { !anErrorHasOccured, sentStandardPacketBytesThisCall, sentControlPacketBytesThisCall };
    */
    SocketSentBytes returnVal = { !anErrorHasOccured, sentStandardPacketBytesThisCall  + ((sentStandardPacketBytesThisCall/minFragSize)+((sentStandardPacketBytesThisCall%minFragSize)?1:0)) * 40, sentControlPacketBytesThisCall + ((sentControlPacketBytesThisCall/minFragSize)+((sentControlPacketBytesThisCall%minFragSize)?1:0)) * 40};
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
uint32 CEMSocket::GetNeededBytes(const char* sendbuffer, const uint32 sendblen, const uint32 sent, const bool currentPacket_is_controlpacket, const DWORD lastCalledSend) {
	sendLocker.Lock();
	//if (byConnected == ES_DISCONNECTED) {
	//	sendLocker.Unlock();
	//	return 0;
	//}

    if (!((sendbuffer && !currentPacket_is_controlpacket) || !standartpacket_queue.IsEmpty())) {
    	// No standard packet to send. Even if data needs to be sent to prevent timout, there's nothing to send.
        sendLocker.Unlock();
		return 0;
	}

	if (((sendbuffer && !currentPacket_is_controlpacket)) && !controlpacket_queue.IsEmpty())
		m_bAccelerateUpload = true;	// We might be trying to send a block request, accelerate packet

	uint32 sendgap = ::GetTickCount() - lastCalledSend;

	uint64 timetotal = m_bAccelerateUpload?45000:90000;
	uint64 timeleft = ::GetTickCount() - lastFinishedStandard;
	uint64 sizeleft, sizetotal;
	if (sendbuffer && !currentPacket_is_controlpacket) {
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
	//EMTrace("CEMSocket::Receive on %d, maxSize=%d",(SOCKET)this,nBufLen);
	int recvRetCode = CAsyncSocketEx::Receive(lpBuf,nBufLen,nFlags); // deadlake PROXYSUPPORT - changed to AsyncSocketEx
	switch (recvRetCode) {
	case 0:
		//EMTrace("CEMSocket::##Received FIN on %d, maxSize=%d",(SOCKET)this,nBufLen);
		// FIN received on socket // Connection is being closed by peer
		//ASSERT (false);
		if (0 == AsyncSelect(FD_CLOSE|FD_WRITE) ) { // no more READ notifications ...
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
	CAsyncSocketEx::RemoveAllLayers();
	delete m_pProxyLayer;
	m_pProxyLayer = NULL;
}

int CEMSocket::OnLayerCallback(const CAsyncSocketExLayer *pLayer, int nType, int nCode, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	ASSERT(pLayer);
	if (nType==LAYERCALLBACK_STATECHANGE)
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
	else if (nType==LAYERCALLBACK_LAYERSPECIFIC)
	{
		if (pLayer==m_pProxyLayer)
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
	CAsyncSocketEx::AssertValid();

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
	CHECK_BOOL(m_currentPacket_is_controlpacket);
    //(void)sendLocker;
    (void)m_numberOfSentBytesCompleteFile;
    (void)m_numberOfSentBytesPartFile;
    (void)m_numberOfSentBytesControlPacket;
    CHECK_BOOL(m_currentPackageIsFromPartFile);
    (void)lastCalledSend;
    (void)m_actualPayloadSize;
    (void)m_actualPayloadSizeSentForThisPacket;
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
