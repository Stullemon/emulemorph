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
#include <math.h>
#include "emule.h"
#include "UploadBandwidthThrottler.h"
#include "EMSocket.h"
#include "opcodes.h"
#include "LastCommonRouteFinder.h"
#include "OtherFunctions.h"
#include "emuledlg.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
/**
 * The constructor starts the thread.
 */
UploadBandwidthThrottler::UploadBandwidthThrottler(void) {
    m_SentBytesSinceLastCall = 0;
	m_SentBytesSinceLastCallOverhead = 0;
    //MORPH - Changed by SiRoB, Upload Splitting Class
	/*
	m_highestNumberOfFullyActivatedSlots = 0;
	*/
	memzero(m_highestNumberOfFullyActivatedSlots,sizeof(m_highestNumberOfFullyActivatedSlots));
	memzero(slotCounterClass,sizeof(slotCounterClass));
		
	threadEndedEvent = new CEvent(0, 1);
    pauseEvent = new CEvent(TRUE, TRUE);

    doRun = true;
    AfxBeginThread(RunProc, (LPVOID)this);
}

/**
 * The destructor stops the thread. If the thread has already stoppped, destructor does nothing.
 */
UploadBandwidthThrottler::~UploadBandwidthThrottler(void) {
    EndThread();
    delete threadEndedEvent;
	delete pauseEvent;
}

void UploadBandwidthThrottler::SetAllowedDataRate(uint32 newValue) {
    sendLocker.Lock();

    m_allowedDataRate = newValue;

	sendLocker.Unlock();
}

/**
 * Find out how many bytes that has been put on the sockets since the last call to this
 * method. Includes overhead of control packets.
 *
 * @return the number of bytes that has been put on the sockets since the last call
 */
uint64 UploadBandwidthThrottler::GetNumberOfSentBytesSinceLastCallAndReset() {
    sendLocker.Lock();
    
    uint64 numberOfSentBytesSinceLastCall = m_SentBytesSinceLastCall;
    m_SentBytesSinceLastCall = 0;

    sendLocker.Unlock();

    return numberOfSentBytesSinceLastCall;
}

/**
 * Find out how many bytes that has been put on the sockets since the last call to this
 * method. Excludes overhead of control packets.
 *
 * @return the number of bytes that has been put on the sockets since the last call
 */
uint64 UploadBandwidthThrottler::GetNumberOfSentBytesOverheadSinceLastCallAndReset() {
    sendLocker.Lock();
    
	uint64 numberOfSentBytesSinceLastCall = m_SentBytesSinceLastCallOverhead;
	m_SentBytesSinceLastCallOverhead = 0;

    sendLocker.Unlock();

    return numberOfSentBytesSinceLastCall;
}

/**
 * Find out the highest number of slots that has been fed data in the normal standard loop
 * of the thread since the last call of this method. This means all slots that haven't
 * been in the trickle state during the entire time since the last call.
 *
 * @return the highest number of fully activated slots during any loop since last call
 */
uint32 UploadBandwidthThrottler::GetHighestNumberOfFullyActivatedSlotsSinceLastCallAndReset(uint32 * HighestNumberOfFullyActivatedSlotsSinceLastCallClass) {
    sendLocker.Lock();
    
    //if(m_highestNumberOfFullyActivatedSlots > (uint32)m_StandardOrder_list.GetSize()) {
    //    theApp.QueueDebugLogLine(true, _T("UploadBandwidthThrottler: Throttler wants new slot when get-method called. m_highestNumberOfFullyActivatedSlots: %i m_StandardOrder_list.GetSize(): %i tick: %i"), m_highestNumberOfFullyActivatedSlots, m_StandardOrder_list.GetSize(), ::GetTickCount());
    //}

    uint32 highestNumberOfFullyActivatedSlots = m_highestNumberOfFullyActivatedSlots[LAST_CLASS];
	*HighestNumberOfFullyActivatedSlotsSinceLastCallClass = m_highestNumberOfFullyActivatedSlots[0];
	*++HighestNumberOfFullyActivatedSlotsSinceLastCallClass = m_highestNumberOfFullyActivatedSlots[1];
	*++HighestNumberOfFullyActivatedSlotsSinceLastCallClass = m_highestNumberOfFullyActivatedSlots[LAST_CLASS];
	memzero(m_highestNumberOfFullyActivatedSlots,sizeof(m_highestNumberOfFullyActivatedSlots));
	sendLocker.Unlock();

    return highestNumberOfFullyActivatedSlots;
}

/**
 * Add a socket to the list of sockets that have upload slots. The main thread will
 * continously call send on these sockets, to give them chance to work off their queues.
 * The sockets are called in the order they exist in the list, so the top socket (index 0)
 * will be given a chance first to use bandwidth, and then the next socket (index 1) etc.
 *
 * It is possible to add a socket several times to the list without removing it inbetween,
 * but that should be avoided.
 *
 * @param index insert the socket at this place in the list. An index that is higher than the
 *              current number of sockets in the list will mean that the socket should be inserted
 *              last in the list.
 *
 * @param socket the address to the socket that should be added to the list. If the address is NULL,
 *               this method will do nothing.
 */
void UploadBandwidthThrottler::AddToStandardList(uint32 index, ThrottledFileSocket* socket, uint32 classID) {
    if(socket != NULL) {
        sendLocker.Lock();

		RemoveFromStandardListNoLock(socket);
		if(index > (uint32)m_StandardOrder_list.GetSize()) {
			index = m_StandardOrder_list.GetSize();
		}
		m_StandardOrder_list.InsertAt(index, socket);
		//MORPH START - Added by SiRoB & AndCycle, Upload Splitting Class
		Socket_stat* cur_socket_stat = new Socket_stat;
		cur_socket_stat->classID = classID;
		m_StandardOrder_list_stat.InsertAt(index, cur_socket_stat);
		++slotCounterClass[classID];
		//MORPH END - Added by SiRoB & AndCycle, Upload Splitting Class

        sendLocker.Unlock();
//	} else {
//		if (thePrefs.GetVerbose())
//		DebugLogError(LOG_STATUSBAR,_T("Tried to add NULL socket to UploadBandwidthThrottler Standard list! Prevented."));
    }
}

/**
 * Remove a socket from the list of sockets that have upload slots.
 *
 * If the socket has mistakenly been added several times to the list, this method
 * will return all of the entries for the socket.
 *
 * @param socket the address of the socket that should be removed from the list. If this socket
 *               does not exist in the list, this method will do nothing.
 */
bool UploadBandwidthThrottler::RemoveFromStandardList(ThrottledFileSocket* socket, bool resort) {
	bool returnValue;
    sendLocker.Lock();

	//MORPH START - Added by SiRoB, Upload Splitting Class
	/*
	returnValue = RemoveFromStandardListNoLock(socket);
	*/
	returnValue = RemoveFromStandardListNoLock(socket, resort);
	//MORPH END   - Added by SiRoB, Upload Splitting Class
	
    sendLocker.Unlock();

    return returnValue;
}

/**
 * Remove a socket from the list of sockets that have upload slots. NOT THREADSAFE!
 * This is an internal method that doesn't take the necessary lock before it removes
 * the socket. This method should only be called when the current thread already owns
 * the sendLocker lock!
 *
 * @param socket address of the socket that should be removed from the list. If this socket
 *               does not exist in the list, this method will do nothing.
 */
bool UploadBandwidthThrottler::RemoveFromStandardListNoLock(ThrottledFileSocket* socket, bool resort) {
	// Find the slot
    int slotCounter = 0;
    bool foundSocket = false;
	uint32 classID = NULL;
    while(slotCounter < m_StandardOrder_list.GetSize() && foundSocket == false) {
        if(m_StandardOrder_list.GetAt(slotCounter) == socket) {
			// Remove the slot
            m_StandardOrder_list.RemoveAt(slotCounter);
			// Remove the slot
			Socket_stat* stat = m_StandardOrder_list_stat.GetAt(slotCounter);
			classID = stat->classID;
			delete stat;
			m_StandardOrder_list_stat.RemoveAt(slotCounter);
			--slotCounterClass[classID];
			foundSocket = true;
        } else {
            slotCounter++;
        }
    }

	//MORPH START - Added by SiRoB, Upload Splitting Class
	if(resort == false && foundSocket) {
        for (uint32 i = classID; i < NB_SPLITTING_CLASS;i++)
			if (m_highestNumberOfFullyActivatedSlots[i] > 0) --m_highestNumberOfFullyActivatedSlots[i];
    }
	//MORPH END  - Added by SiRoB, Upload Splitting Class

	return foundSocket;
}

/**
* Notifies the send thread that it should try to call controlpacket send
* for the supplied socket. It is allowed to call this method several times
* for the same socket, without having controlpacket send called for the socket
* first. The doublette entries are never filtered, since it is incurs less cpu
* overhead to simply call Send() in the socket for each double. Send() will
* already have done its work when the second Send() is called, and will just
* return with little cpu overhead.
*
* @param socket address to the socket that requests to have controlpacket send
*               to be called on it
*/
void UploadBandwidthThrottler::QueueForSendingControlPacket(ThrottledControlSocket* socket, bool hasSent) {
	// Get critical section
    tempQueueLocker.Lock();

    if(doRun) {
        if(hasSent) {
            m_TempControlQueueFirst_list.AddTail(socket);
        } else {
        	m_TempControlQueue_list.AddTail(socket);
    	}
    }

	// End critical section
    tempQueueLocker.Unlock();
}

/**
 * Remove the socket from all lists and queues. This will make it safe to
 * erase/delete the socket. It will also cause the main thread to stop calling
 * send() for the socket.
 *
 * @param socket address to the socket that should be removed
 */
void UploadBandwidthThrottler::RemoveFromAllQueues(ThrottledControlSocket* socket, bool lock) {
    if(lock) {
		// Get critical section
   		sendLocker.Lock();
    }

    if(doRun) {
        // Remove this socket from control packet queue
        {
            POSITION pos1, pos2;
	        for (pos1 = m_ControlQueue_list.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
		        m_ControlQueue_list.GetNext(pos1);
		        ThrottledControlSocket* socketinQueue = m_ControlQueue_list.GetAt(pos2);

                if(socketinQueue == socket) {
                    m_ControlQueue_list.RemoveAt(pos2);
                }
            }
        }
        
        {
            POSITION pos1, pos2;
	        for (pos1 = m_ControlQueueFirst_list.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
		        m_ControlQueueFirst_list.GetNext(pos1);
		        ThrottledControlSocket* socketinQueue = m_ControlQueueFirst_list.GetAt(pos2);

                if(socketinQueue == socket) {
                    m_ControlQueueFirst_list.RemoveAt(pos2);
                }
            }
        }

        tempQueueLocker.Lock();
        {
            POSITION pos1, pos2;
	        for (pos1 = m_TempControlQueue_list.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
		        m_TempControlQueue_list.GetNext(pos1);
		        ThrottledControlSocket* socketinQueue = m_TempControlQueue_list.GetAt(pos2);

                if(socketinQueue == socket) {
                    m_TempControlQueue_list.RemoveAt(pos2);
                }
            }
        }

        {
            POSITION pos1, pos2;
	        for (pos1 = m_TempControlQueueFirst_list.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
		        m_TempControlQueueFirst_list.GetNext(pos1);
		        ThrottledControlSocket* socketinQueue = m_TempControlQueueFirst_list.GetAt(pos2);

                if(socketinQueue == socket) {
                    m_TempControlQueueFirst_list.RemoveAt(pos2);
                }
            }
        }
        tempQueueLocker.Unlock();
    }

    if(lock) {
		// End critical section
        sendLocker.Unlock();
    }
}

void UploadBandwidthThrottler::RemoveFromAllQueues(ThrottledFileSocket* socket) {
	// Get critical section
    sendLocker.Lock();

    if(doRun) {
        RemoveFromAllQueues(socket, false);

		// And remove it from upload slots
        RemoveFromStandardListNoLock(socket);
    }

	// End critical section
    sendLocker.Unlock();
}

/**
 * Make the thread exit. This method will not return until the thread has stopped
 * looping. This guarantees that the thread will not access the CEMSockets after this
 * call has exited.
 */
void UploadBandwidthThrottler::EndThread() {
    sendLocker.Lock();

	// signal the thread to stop looping and exit.
    doRun = false;

    sendLocker.Unlock();

    Pause(false);

	// wait for the thread to signal that it has stopped looping.
    threadEndedEvent->Lock();
}

void UploadBandwidthThrottler::Pause(bool paused) {
    if(paused) {
        pauseEvent->ResetEvent();
    } else {
        pauseEvent->SetEvent();
    }
}

/**
 * Start the thread. Called from the constructor in this class.
 *
 * @param pParam
 *
 * @return
 */
UINT AFX_CDECL UploadBandwidthThrottler::RunProc(LPVOID pParam) {
	DbgSetThreadName("UploadBandwidthThrottler");
	InitThreadLocale();
    UploadBandwidthThrottler* uploadBandwidthThrottler = (UploadBandwidthThrottler*)pParam;

    return uploadBandwidthThrottler->RunInternal();
}

/**
 * The thread method that handles calling send for the individual sockets.
 *
 * Control packets will always be tried to be sent first. If there is any bandwidth leftover
 * after that, send() for the upload slot sockets will be called in priority order until we have run
 * out of available bandwidth for this loop. Upload slots will not be allowed to go without having sent
 * called for more than a defined amount of time (i.e. two seconds).
 *
 * @return always returns 0.
 */
UINT UploadBandwidthThrottler::RunInternal() {
    DWORD lastLoopTick = ::GetTickCount();

	sint64 realBytesToSpend = 0;

	uint32 allowedDataRate = 0;
	uint32 lastMaxAllowedDataRate = 1;

    uint32 rememberedSlotCounter = 0;

	//MORPH START - Added by SiRoB, Upload Splitting Class
	sint64 realBytesToSpendClass[NB_SPLITTING_CLASS];
	memzero(realBytesToSpendClass,sizeof(realBytesToSpendClass));
	uint32 allowedDataRateClass[NB_SPLITTING_CLASS];
	memzero(allowedDataRateClass,sizeof(allowedDataRateClass));
	uint32 rememberedSlotCounterClass[NB_SPLITTING_CLASS];
	memzero(rememberedSlotCounterClass,sizeof(rememberedSlotCounterClass));
	uint32 ClientDataRate[NB_SPLITTING_CLASS];
	memzero(ClientDataRate,sizeof(ClientDataRate));
	DWORD lastTickNotReachedBandwidth[NB_SPLITTING_CLASS];
	memset(lastTickNotReachedBandwidth,::GetTickCount(),sizeof(lastTickNotReachedBandwidth));
	//MORPH END   - Added by SiRoB, Upload Splitting Class
    DWORD lastTickReachedBandwidth[NB_SPLITTING_CLASS];
	memset(lastTickReachedBandwidth,::GetTickCount(),sizeof(lastTickReachedBandwidth));

    while(doRun) {
/*Moved to Downloadqueue::Process() to avoid locking main thread
		//MORPH START - Added by SiRoB, WebCache 1.2f
		if (thePrefs.expectingWebCachePing && (::GetTickCount() - thePrefs.WebCachePingSendTime > SEC2MS(30)))
		{
			thePrefs.expectingWebCachePing = false;
			thePrefs.WebCacheDisabledThisSession = true; //Disable webcache downloads for the current proxy settings
			//JP we need a modeless dialogue here!!
//			AfxMessageBox(_T("Proxy configuration Test Failed please review your proxy-settings"));
			theApp.QueueLogLine(false, _T("Proxy configuration Test Failed please review your proxy-settings. Webcache downloads have been deactivated until emule is restarted."));
		}
		//MORPH - Added by SiRoB, WebCache 1.2f
*/
        pauseEvent->Lock();

        DWORD timeSinceLastLoop = ::GetTickCount() - lastLoopTick;

		// Get current speed from UploadSpeedSense
        //MORPH START - Changed by SiRoB, Upload Splitting Class
		/*
		allowedDataRate = theApp.lastCommonRouteFinder->GetUpload();
		*/
		theApp.lastCommonRouteFinder->GetClassByteToSend(allowedDataRateClass,ClientDataRate);
		//MORPH END   - Changed by SiRoB, Upload Splitting Class

		uint32 minFragSize = 1300;
        uint32 doubleSendSize = minFragSize*2; // send two packages at a time so they can share an ACK
        if(allowedDataRateClass[LAST_CLASS] < 6*1024) {
            minFragSize = 536;
            doubleSendSize = minFragSize; // don't send two packages at a time at very low speeds to give them a smoother load
        }

#define TIME_BETWEEN_UPLOAD_LOOPS 1
        uint32 sleepTime;
        if(allowedDataRateClass[LAST_CLASS] == 0 || allowedDataRateClass[LAST_CLASS] == _UI32_MAX || realBytesToSpendClass[LAST_CLASS] >= 1000) {
                // we could send at once, but sleep a while to not suck up all cpu
            sleepTime = TIME_BETWEEN_UPLOAD_LOOPS;
            } else {
                // sleep for just as long as we need to get back to having one byte to send
            sleepTime = max((uint32)ceil((double)(-realBytesToSpendClass[LAST_CLASS] + 1000)/allowedDataRateClass[LAST_CLASS]), TIME_BETWEEN_UPLOAD_LOOPS);

        }

        if(timeSinceLastLoop < sleepTime) {
            Sleep(sleepTime-timeSinceLastLoop);
        }
        
        const DWORD thisLoopTick = ::GetTickCount();
        timeSinceLastLoop = thisLoopTick - lastLoopTick;

		// Calculate how many bytes we can spend
		sint64 bytesToSpend = 0;

		//MORPH START - Added by SiRoB, Upload Splitting Class
		sint64 bytesToSpendClass[NB_SPLITTING_CLASS];
		memzero(bytesToSpendClass,sizeof(bytesToSpendClass));
		DWORD temptimeSinceLastLoop = timeSinceLastLoop;
		DWORD templastLoopTick = thisLoopTick; //MORPH - Moved by SiRoB, -Fix-
		for (uint32 classID = 0; classID < NB_SPLITTING_CLASS; classID++)
		{
			allowedDataRate = allowedDataRateClass[classID];
			bytesToSpend = bytesToSpendClass[classID];
			realBytesToSpend = realBytesToSpendClass[classID];
		//MORPH END   - Added by SiRoB, Upload Splitting Class
			if(allowedDataRate != 0 && allowedDataRate != _UI32_MAX) {
        	    // prevent overflow
        	    if(timeSinceLastLoop == 0) {
        	        // no time has passed, so don't add any bytes. Shouldn't happen.
        	        bytesToSpend = 0; //realBytesToSpend/1000;
        	    } else if(_I64_MAX/timeSinceLastLoop > allowedDataRate && _I64_MAX-allowedDataRate*timeSinceLastLoop > realBytesToSpend) {
        	        if (classID == LAST_CLASS) lastLoopTick = thisLoopTick; //MORPH - Added by SiRoB, lastLoopTick Fix
					if(timeSinceLastLoop > sleepTime + 2000) {
				        if (classID == LAST_CLASS)
							theApp.QueueDebugLogLine(false,_T("UploadBandwidthThrottler: Time since last loop too long. time: %ims wanted: %ims Max: %ims"), timeSinceLastLoop, sleepTime, sleepTime + 2000);
        
        	            timeSinceLastLoop = sleepTime + 2000;
						if (classID == LAST_CLASS)
							lastLoopTick = thisLoopTick - timeSinceLastLoop;
        	        }

        	        realBytesToSpend += allowedDataRate*timeSinceLastLoop;

        	        bytesToSpend = realBytesToSpend/1000;
        		} else {
					if (classID == LAST_CLASS) lastLoopTick = thisLoopTick; //MORPH - Added by SiRoB, lastLoopTick Fix
        		    realBytesToSpend = _I64_MAX;
        	        bytesToSpend = _I32_MAX;
        	    }
        	} else {
        	    if (classID == LAST_CLASS) lastLoopTick = thisLoopTick; //MORPH - Added by SiRoB, lastLoopTick Fix
				realBytesToSpend = _I64_MAX;
        	    bytesToSpend = _I32_MAX;
        	}
		//MORPH START - Added by SiRoB, Upload Splitting Class
			bytesToSpendClass[classID] = bytesToSpend;
			realBytesToSpendClass[classID] = realBytesToSpend;
			if (classID < LAST_CLASS)
				timeSinceLastLoop = temptimeSinceLastLoop;
		}
		//MORPH END   - Added by SiRoB, Upload Splitting Class

		if(bytesToSpendClass[LAST_CLASS] >= 1) {
			uint64 spentBytesClass[NB_SPLITTING_CLASS];
			memzero(spentBytesClass,sizeof(spentBytesClass));
			uint64 spentOverheadClass[NB_SPLITTING_CLASS];
			memzero(spentOverheadClass,sizeof(spentOverheadClass));

			sendLocker.Lock();

        	tempQueueLocker.Lock();
    
	        // are there any sockets in m_TempControlQueue_list? Move them to normal m_ControlQueue_list;
            while(!m_TempControlQueueFirst_list.IsEmpty()) {
                ThrottledControlSocket* moveSocket = m_TempControlQueueFirst_list.RemoveHead();
                m_ControlQueueFirst_list.AddTail(moveSocket);
            }
        	while(!m_TempControlQueue_list.IsEmpty()) {
                ThrottledControlSocket* moveSocket = m_TempControlQueue_list.RemoveHead();
        	    m_ControlQueue_list.AddTail(moveSocket);
        	}
    
       		tempQueueLocker.Unlock();
        
        	// Send any queued up control packets first
            while(bytesToSpendClass[LAST_CLASS] > 0 && spentBytesClass[LAST_CLASS] <  (uint64)bytesToSpendClass[LAST_CLASS] && (!m_ControlQueueFirst_list.IsEmpty() || !m_ControlQueue_list.IsEmpty())) {
                ThrottledControlSocket* socket = NULL;
    			
				if(!m_ControlQueueFirst_list.IsEmpty()) {
                    socket = m_ControlQueueFirst_list.RemoveHead();
                } else if(!m_ControlQueue_list.IsEmpty()) {
                    socket = m_ControlQueue_list.RemoveHead();
                }

       	     if(socket != NULL) {
				    SocketSentBytes socketSentBytes = socket->SendControlData(_I32_MAX/*bytesToSpendClass[LAST_CLASS]-spentBytesClass[LAST_CLASS]*/, minFragSize);
                    uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
         	       	spentBytesClass[LAST_CLASS] += lastSpentBytes;
					spentOverheadClass[LAST_CLASS] += socketSentBytes.sentBytesControlPackets;
         	 	  }
       		}
			// Check if any sockets haven't gotten data for a long time. Then trickle them a package.
     		for(uint32 slotCounter = 0; slotCounter < (uint32)m_StandardOrder_list.GetSize(); slotCounter++) {
                ThrottledFileSocket* socket = m_StandardOrder_list.GetAt(slotCounter);
				uint32 classID = m_StandardOrder_list_stat.GetAt(slotCounter)->classID;
				
     			if(socket != NULL) {
				    if(thisLoopTick-socket->GetLastCalledSend() > SEC2MS(1)) {
      	              // trickle
      	              uint32 neededBytes = socket->GetNeededBytes(minFragSize<1000);

      	              if(neededBytes > 0) {
						    SocketSentBytes socketSentBytes = socket->SendFileAndControlData(neededBytes, minFragSize);
                            uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
                    	    spentBytesClass[classID] += lastSpentBytes;
							spentOverheadClass[classID] += socketSentBytes.sentBytesControlPackets;
							if(lastSpentBytes > 0 && m_highestNumberOfFullyActivatedSlots[classID] > 0)
								m_highestNumberOfFullyActivatedSlots[classID] = min(slotCounter+1,m_highestNumberOfFullyActivatedSlots[classID]);
							if (classID < LAST_CLASS){
								spentBytesClass[LAST_CLASS] += lastSpentBytes;
								spentOverheadClass[LAST_CLASS] += spentOverheadClass[classID];
								if (m_highestNumberOfFullyActivatedSlots[LAST_CLASS]<m_highestNumberOfFullyActivatedSlots[classID])
									m_highestNumberOfFullyActivatedSlots[LAST_CLASS] = m_highestNumberOfFullyActivatedSlots[classID];
							}
						}
               		}
        	    } else {
				    theApp.QueueDebugLogLine(false,_T("There was a NULL socket in the UploadBandwidthThrottler Standard list (trickle)! Prevented usage. Index: %i Size: %i"), slotCounter, m_StandardOrder_list.GetSize());
           		}
        	}
			//MORPH START - Added by SiRoB, Upload Splitting Class

			uint32 lastpos = 0;
            for(uint32 classID=0;classID<NB_SPLITTING_CLASS;classID++)
			{
				if (slotCounterClass[classID]==0) continue;
				if (allowedDataRateClass[classID]==0)
					bytesToSpendClass[classID] = bytesToSpendClass[LAST_CLASS];

				bool isFocused = ClientDataRate[classID] == 0;
				for(uint32 maxCounter = lastpos; maxCounter < lastpos+slotCounterClass[classID] && bytesToSpendClass[LAST_CLASS] > 0 && spentBytesClass[LAST_CLASS] < (uint64)bytesToSpendClass[LAST_CLASS]; maxCounter++) {
					uint32 posSocket = maxCounter;
					if(isFocused == false)
					{
						if(rememberedSlotCounterClass[classID] >= slotCounterClass[classID]) {
	                		rememberedSlotCounterClass[classID] = 0;
						}
						posSocket = lastpos+rememberedSlotCounterClass[classID];
					}
					ThrottledFileSocket* socket = m_StandardOrder_list.GetAt(posSocket);
					
					if(socket != NULL) {
						uint64 bytesToSpendTemp = bytesToSpendClass[LAST_CLASS]-spentBytesClass[LAST_CLASS];
						if(allowedDataRateClass[classID] > 0 && classID < LAST_CLASS && slotCounterClass[classID]<(uint32)m_StandardOrder_list.GetSize()){
							if(bytesToSpendClass[classID] > 0 && spentBytesClass[classID] < (uint64)bytesToSpendClass[classID])
								bytesToSpendTemp = min(bytesToSpendClass[classID]-spentBytesClass[classID],bytesToSpendTemp);
							else
								break;
						}
						if (isFocused == false) bytesToSpendTemp = min(doubleSendSize,bytesToSpendTemp);
						SocketSentBytes socketSentBytes = socket->SendFileAndControlData(bytesToSpendTemp, doubleSendSize);
						uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
						spentBytesClass[classID] += lastSpentBytes;
						spentOverheadClass[classID] += socketSentBytes.sentBytesControlPackets;
						//MORPH START - Added by SiRoB, Upload Splitting Class
						if(posSocket+1 > m_highestNumberOfFullyActivatedSlots[classID] && (lastSpentBytes < bytesToSpendTemp || lastSpentBytes >= doubleSendSize)) // || lastSpentBytes > 0 && spentBytes == bytesToSpend /*|| slotCounter+1 == (uint32)m_StandardOrder_list.GetSize())*/))
							m_highestNumberOfFullyActivatedSlots[classID] = posSocket+1;
						if (classID < LAST_CLASS){
							spentBytesClass[LAST_CLASS] += lastSpentBytes;
							spentOverheadClass[LAST_CLASS] += spentOverheadClass[classID];
							if (m_highestNumberOfFullyActivatedSlots[classID]>m_highestNumberOfFullyActivatedSlots[LAST_CLASS])
								m_highestNumberOfFullyActivatedSlots[LAST_CLASS] = m_highestNumberOfFullyActivatedSlots[classID];
						}
						//MORPH END   - Added by SiRoB, Upload Splitting Class
					} else {
						theApp.QueueDebugLogLine(false,_T("There was a NULL socket in the UploadBandwidthThrottler Standard list (equal-for-all)! Prevented usage. Index: %i Size: %i Class: %s"), rememberedSlotCounterClass[classID], m_StandardOrder_list.GetSize(), classID);
					}
					//MORPH START - Changed by SiRoB, Upload Splitting Class
					/*
					rememberedSlotCounter++;
					*/
					if(isFocused == false) ++rememberedSlotCounterClass[classID];
					//MORPH END   - Changed by SiRoB, Upload Splitting Class
				}
				lastpos += slotCounterClass[classID];
			}
		    // Any bandwidth that hasn't been used yet are used first to last.
			// According allready ByteToSpend in class
			lastpos = 0;
            for(uint32 classID = 0; classID < NB_SPLITTING_CLASS; classID++)
			{	
				if ((allowedDataRateClass[classID]==0 || classID==LAST_CLASS) &&  ClientDataRate[classID]==0)
				{
					for(uint32 slotCounter = lastpos; slotCounter < lastpos+slotCounterClass[classID] && bytesToSpendClass[LAST_CLASS] > 0 && spentBytesClass[LAST_CLASS] < (uint64)bytesToSpendClass[LAST_CLASS]; slotCounter++) {
						ThrottledFileSocket* socket = m_StandardOrder_list.GetAt(slotCounter);
						if(socket != NULL) {
							uint32 bytesToSpendTemp = bytesToSpendClass[LAST_CLASS]-spentBytesClass[LAST_CLASS];
							SocketSentBytes socketSentBytes = socket->SendFileAndControlData(bytesToSpendTemp, doubleSendSize);
							uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
							spentBytesClass[classID] += lastSpentBytes;
							spentOverheadClass[classID] += socketSentBytes.sentBytesControlPackets;
							if(slotCounter+1 > m_highestNumberOfFullyActivatedSlots[classID] && (lastSpentBytes < bytesToSpendTemp || lastSpentBytes >= doubleSendSize)) // || lastSpentBytes > 0 && spentBytes == bytesToSpend /*|| slotCounter+1 == (uint32)m_StandardOrder_list.GetSize())*/))
								m_highestNumberOfFullyActivatedSlots[classID] = slotCounter+1;
							if (classID < LAST_CLASS){
								spentBytesClass[LAST_CLASS] += lastSpentBytes;
								spentOverheadClass[LAST_CLASS] += spentOverheadClass[classID];
								if(m_highestNumberOfFullyActivatedSlots[classID]>m_highestNumberOfFullyActivatedSlots[LAST_CLASS])
									m_highestNumberOfFullyActivatedSlots[LAST_CLASS] = m_highestNumberOfFullyActivatedSlots[classID];
							}
						} else {
							theApp.QueueDebugLogLine(false,_T("There was a NULL socket in the UploadBandwidthThrottler Standard list (fully activated)! Prevented usage. Index: %i Size: %i"), slotCounter, m_StandardOrder_list.GetSize());
						}
					}
				}
				lastpos += slotCounterClass[classID];
				realBytesToSpendClass[classID] -= spentBytesClass[classID]*1000;
				
			}

			//MORPH START - Changed by SiRoB, Upload Splitting Class
			uint32 sumofclientinclass = 0;
			for(uint32 classID = 0; classID < NB_SPLITTING_CLASS; classID++)
			{
				sumofclientinclass += slotCounterClass[classID];
				if (allowedDataRateClass[classID]==0 && sumofclientinclass==m_StandardOrder_list.GetSize())
					realBytesToSpend = realBytesToSpendClass[LAST_CLASS];
				else
					realBytesToSpend = realBytesToSpendClass[classID];
				uint64 bandwidthSavedTolerance = ((classID==LAST_CLASS)?m_StandardOrder_list.GetSize():slotCounterClass[classID])*minFragSize*1000;
				if(realBytesToSpend > 0 && (uint64)realBytesToSpend > 999+bandwidthSavedTolerance) {
					realBytesToSpend = 999+bandwidthSavedTolerance;
					if (m_highestNumberOfFullyActivatedSlots[classID] < ((classID==LAST_CLASS)?m_StandardOrder_list.GetSize():sumofclientinclass)+1)
						m_highestNumberOfFullyActivatedSlots[classID] = ((classID==LAST_CLASS)?m_StandardOrder_list.GetSize():sumofclientinclass)+1;
					//theApp.QueueDebugLogLine(false, _T("UploadBandwidthThrottler: Throttler requests new slot due to bw not reached. m_highestNumberOfFullyActivatedSlots: %i m_StandardOrder_list.GetSize(): %i tick: %i"), m_highestNumberOfFullyActivatedSlots, m_StandardOrder_list.GetSize(), thisLoopTick);
           		}
       			realBytesToSpendClass[classID] = realBytesToSpend;
			}
			//MORPH END   - Changed by SiRoB, Upload Splitting Class

       		m_SentBytesSinceLastCall += spentBytesClass[LAST_CLASS];
	    	m_SentBytesSinceLastCallOverhead += spentOverheadClass[LAST_CLASS];
       		sendLocker.Unlock();
		}
    }

    threadEndedEvent->SetEvent();

    tempQueueLocker.Lock();
    m_TempControlQueue_list.RemoveAll();
	m_TempControlQueueFirst_list.RemoveAll();
    tempQueueLocker.Unlock();

    sendLocker.Lock();

    m_ControlQueue_list.RemoveAll();
    for(uint32 slotCounter = 0; slotCounter < (uint32)m_StandardOrder_list.GetSize(); slotCounter++)
		delete m_StandardOrder_list_stat.GetAt(slotCounter);
	m_StandardOrder_list.RemoveAll();
    sendLocker.Unlock();

    return 0;
}
