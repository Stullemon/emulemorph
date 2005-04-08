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
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
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
	memset(m_highestNumberOfFullyActivatedSlots,0,sizeof(m_highestNumberOfFullyActivatedSlots));
	memset(slotCounterClass,0,sizeof(slotCounterClass));
		
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
	memset(m_highestNumberOfFullyActivatedSlots,0,sizeof(m_highestNumberOfFullyActivatedSlots));
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
		Socket_stat* cur_socket_stat = NULL;
		if (!m_stat_list.Lookup(socket,cur_socket_stat)){
			cur_socket_stat = new Socket_stat;
			m_stat_list.SetAt(socket,cur_socket_stat);
			cur_socket_stat->realBytesToSpend = 0;
		}
		cur_socket_stat->classID = classID;
		if (classID==SCHED_CLASS)
			classID=LAST_CLASS;
		++slotCounterClass[classID];
	    uint32 sumofclientinclass = 0;
		for (uint32 i = 0; i < NB_SPLITTING_CLASS;i++)
			m_highestNumberOfFullyActivatedSlots[i] = 0;
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
			Socket_stat* stat = NULL;
			m_stat_list.Lookup(socket,stat);
			classID = stat->classID;
			if (!resort) {
				delete stat;
				m_stat_list.RemoveKey(socket);
			}
			
			if (classID==SCHED_CLASS)
				classID=LAST_CLASS;
			--slotCounterClass[classID];
			foundSocket = true;
        } else {
            slotCounter++;
        }
    }

	//MORPH START - Added by SiRoB, Upload Splitting Class
	if(resort == false && foundSocket) {
        uint32 sumofclientinclass = 0;
		for (uint32 i = 0; i < NB_SPLITTING_CLASS;i++)
			m_highestNumberOfFullyActivatedSlots[i] = 0;
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
    uint32 rememberedSlotCounter = 0;

	//MORPH START - Added by SiRoB, Upload Splitting Class
	sint64 realBytesToSpendClass[NB_SPLITTING_CLASS];
	memset(realBytesToSpendClass,0,sizeof(realBytesToSpendClass));
	uint32 allowedDataRateClass[NB_SPLITTING_CLASS];
	memset(allowedDataRateClass,0,sizeof(allowedDataRateClass));
	uint32 rememberedSlotCounterClass[NB_SPLITTING_CLASS];
	memset(rememberedSlotCounterClass,0,sizeof(rememberedSlotCounterClass));
	uint32 ClientDataRate[NB_SPLITTING_CLASS];
	memset(ClientDataRate,0,sizeof(ClientDataRate));
	DWORD sumtimeSinceLastLoop = 0;
	//MORPH END   - Added by SiRoB, Upload Splitting Class

    while(doRun) {
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
		memset(bytesToSpendClass,0,sizeof(bytesToSpendClass));
		DWORD temptimeSinceLastLoop = timeSinceLastLoop;
		for (uint32 classID = 0; classID < NB_SPLITTING_CLASS; classID++)
		{
			allowedDataRate = allowedDataRateClass[classID];
			realBytesToSpend = realBytesToSpendClass[classID];
		//MORPH END   - Added by SiRoB, Upload Splitting Class
			if(allowedDataRate != 0 && allowedDataRate != _UI32_MAX) {
        	    // prevent overflow
        	    if(timeSinceLastLoop == 0) {
        	        // no time has passed, so don't add any bytes. Shouldn't happen.
        	        bytesToSpend = realBytesToSpend/1000;
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
		sumtimeSinceLastLoop+=timeSinceLastLoop;
		//MORPH END   - Added by SiRoB, Upload Splitting Class
		
		if(bytesToSpendClass[LAST_CLASS] >= 1) {
			uint64 spentBytesClass[NB_SPLITTING_CLASS];
			memset(spentBytesClass,0,sizeof(spentBytesClass));
			uint64 spentOverheadClass[NB_SPLITTING_CLASS];
			memset(spentOverheadClass,0,sizeof(spentOverheadClass));

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

			for(uint32 slotCounter = 0; slotCounter < (uint32)m_StandardOrder_list.GetSize(); slotCounter++) {
                ThrottledFileSocket* socket = m_StandardOrder_list.GetAt(slotCounter);
				Socket_stat* stat = NULL;
				m_stat_list.Lookup(socket,stat);
				uint32 classID = stat->classID;
				if (sumtimeSinceLastLoop>0){
					if (ClientDataRate[classID] == 0)
						stat->realBytesToSpend = _I64_MAX;
					else if (_I64_MAX/sumtimeSinceLastLoop > ClientDataRate[classID] && _I64_MAX-ClientDataRate[classID]*sumtimeSinceLastLoop > stat->realBytesToSpend)
						stat->realBytesToSpend += ClientDataRate[classID]*sumtimeSinceLastLoop;
					else
						stat->realBytesToSpend = _I64_MAX;
					if (stat->classID==SCHED_CLASS && stat->realBytesToSpend>0)
						stat->realBytesToSpend = 1000;
				}
			}
			sumtimeSinceLastLoop = 0;

			// Send any queued up control packets first
			while(bytesToSpendClass[LAST_CLASS] > 0 && spentBytesClass[LAST_CLASS] <  (uint64)bytesToSpendClass[LAST_CLASS] && (!m_ControlQueueFirst_list.IsEmpty() || !m_ControlQueue_list.IsEmpty())) {
                ThrottledControlSocket* socket = NULL;
    			
				if(!m_ControlQueueFirst_list.IsEmpty()) {
                    socket = m_ControlQueueFirst_list.RemoveHead();
                } else if(!m_ControlQueue_list.IsEmpty()) {
                    socket = m_ControlQueue_list.RemoveHead();
                }

       			if(socket != NULL) {
				    SocketSentBytes socketSentBytes = socket->SendControlData(bytesToSpendClass[LAST_CLASS]-spentBytesClass[LAST_CLASS], minFragSize);
                    uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
         	       	spentBytesClass[LAST_CLASS] += lastSpentBytes;
					spentOverheadClass[LAST_CLASS] += socketSentBytes.sentBytesControlPackets;
         	 	  }
       		}

			// Check if any sockets haven't gotten data for a long time. Then trickle them a package.
     		for(uint32 slotCounter = 0; slotCounter < (uint32)m_StandardOrder_list.GetSize(); slotCounter++) {
                ThrottledFileSocket* socket = m_StandardOrder_list.GetAt(slotCounter);
				Socket_stat* stat = NULL;
				m_stat_list.Lookup(socket,stat);
				uint32 classID = stat->classID;
				if(socket != NULL) {
				    if(thisLoopTick-socket->GetLastCalledSend() >= SEC2MS(1)) {
      	              // trickle
      	              uint32 neededBytes = socket->GetNeededBytes(minFragSize<1000);

      	              if(neededBytes > 0) {
						    SocketSentBytes socketSentBytes = socket->SendFileAndControlData(neededBytes, minFragSize);
                            uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
                    	    stat->realBytesToSpend -= lastSpentBytes*1000;
							spentBytesClass[classID] += lastSpentBytes;
							spentOverheadClass[classID] += socketSentBytes.sentBytesControlPackets;
							if(m_highestNumberOfFullyActivatedSlots[classID] > slotCounter+1)
								m_highestNumberOfFullyActivatedSlots[classID] = slotCounter+1;
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

			uint32 lastpos = 0;
            for(uint32 classID=0;classID<NB_SPLITTING_CLASS;classID++)
			{
				for(uint32 maxCounter = lastpos; maxCounter < lastpos+slotCounterClass[classID] && bytesToSpendClass[LAST_CLASS] > 0 && spentBytesClass[LAST_CLASS] < (uint64)bytesToSpendClass[LAST_CLASS]; maxCounter++) {
					ThrottledFileSocket* socket = m_StandardOrder_list.GetAt(maxCounter);
					if(socket != NULL) {
						if(bytesToSpendClass[classID] <= 0 || spentBytesClass[classID] >= (uint64)bytesToSpendClass[classID])
							break;
						Socket_stat* stat = NULL;
						m_stat_list.Lookup(socket,stat);
						if(stat->realBytesToSpend > 999 && stat->classID < SCHED_CLASS)
						{
							uint32 bytesToSpendTemp = min(bytesToSpendClass[LAST_CLASS] - spentBytesClass[LAST_CLASS],bytesToSpendClass[classID] - spentBytesClass[classID]);
							bytesToSpendTemp = min(stat->realBytesToSpend/1000,bytesToSpendTemp);
							bytesToSpendTemp = max(doubleSendSize,bytesToSpendTemp);
							SocketSentBytes socketSentBytes = socket->SendFileAndControlData(bytesToSpendTemp, doubleSendSize);
							uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
							stat->realBytesToSpend -= lastSpentBytes*1000;
							spentBytesClass[classID] += lastSpentBytes;
							spentOverheadClass[classID] += socketSentBytes.sentBytesControlPackets;
							//MORPH START - Added by SiRoB, Upload Splitting Class
							if(maxCounter+1 > m_highestNumberOfFullyActivatedSlots[classID] && (lastSpentBytes >= doubleSendSize)) // || lastSpentBytes > 0 && spentBytes == bytesToSpend /*|| slotCounter+1 == (uint32)m_StandardOrder_list.GetSize())*/))
								m_highestNumberOfFullyActivatedSlots[classID] = maxCounter+1;
							if (classID < LAST_CLASS){
								spentBytesClass[LAST_CLASS] += lastSpentBytes;
								spentOverheadClass[LAST_CLASS] += spentOverheadClass[classID];
								if (m_highestNumberOfFullyActivatedSlots[classID]>m_highestNumberOfFullyActivatedSlots[LAST_CLASS])
									m_highestNumberOfFullyActivatedSlots[LAST_CLASS] = m_highestNumberOfFullyActivatedSlots[classID];
							}
						}
						//MORPH END   - Added by SiRoB, Upload Splitting Class
					} else {
						theApp.QueueDebugLogLine(false,_T("There was a NULL socket in the UploadBandwidthThrottler Standard list (equal-for-all)! Prevented usage. Index: %i Size: %i Class: %i"), rememberedSlotCounterClass[classID], m_StandardOrder_list.GetSize(), classID);
					}
				}
				lastpos += slotCounterClass[classID];
			}
			// Any bandwidth that hasn't been used yet are used first to last.
			// According allready ByteToSpend in class
			lastpos = 0;
            for(uint32 classID = 0; classID < NB_SPLITTING_CLASS; classID++)
			{	
				for(uint32 slotCounter = lastpos; slotCounter < lastpos+slotCounterClass[classID] && bytesToSpendClass[LAST_CLASS] > 0 && spentBytesClass[LAST_CLASS] < (uint64)bytesToSpendClass[LAST_CLASS]; slotCounter++) {
					ThrottledFileSocket* socket = m_StandardOrder_list.GetAt(slotCounter);
					if(bytesToSpendClass[classID] <= 0 || spentBytesClass[classID] >= (uint64)bytesToSpendClass[classID])
						break;
					if(socket != NULL) {
						Socket_stat* stat = NULL;
						m_stat_list.Lookup(socket,stat);
						if (stat->realBytesToSpend > 999){
							uint32 bytesToSpendTemp = min(bytesToSpendClass[LAST_CLASS] - spentBytesClass[LAST_CLASS],bytesToSpendClass[classID] - spentBytesClass[classID]);
							bytesToSpendTemp = min(stat->realBytesToSpend/1000,bytesToSpendTemp);
							bytesToSpendTemp = max(doubleSendSize,bytesToSpendTemp);
							SocketSentBytes socketSentBytes = socket->SendFileAndControlData(bytesToSpendTemp, doubleSendSize);
							uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
							stat->realBytesToSpend -= lastSpentBytes*1000;
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
						}
					} else {
						theApp.QueueDebugLogLine(false,_T("There was a NULL socket in the UploadBandwidthThrottler Standard list (fully activated)! Prevented usage. Index: %i Size: %i"), slotCounter, m_StandardOrder_list.GetSize());
					}
				}
				lastpos += slotCounterClass[classID];
				realBytesToSpendClass[classID] -= spentBytesClass[classID]*1000;
			}
			
			//MORPH START - Changed by SiRoB, Upload Splitting Class
			uint32 sumofclientinclass = 0;
			for(uint32 classID = 0; classID < NB_SPLITTING_CLASS; classID++)
			{
				if (allowedDataRateClass[classID] == 0)
					allowedDataRate = allowedDataRateClass[LAST_CLASS];
				else
					allowedDataRate = allowedDataRateClass[classID];
				realBytesToSpend = realBytesToSpendClass[classID];

				for(uint32 maxCounter = sumofclientinclass; maxCounter < sumofclientinclass+slotCounterClass[classID]; maxCounter++) {
					ThrottledFileSocket* socket = m_StandardOrder_list.GetAt(maxCounter);
					Socket_stat* stat = NULL;
					m_stat_list.Lookup(socket,stat);
					if(stat->realBytesToSpend < -((sint64)ClientDataRate[classID])*1000)
						stat->realBytesToSpend = -((sint64)ClientDataRate[classID])*1000;
					else if(stat->realBytesToSpend > ClientDataRate[classID])
							stat->realBytesToSpend = ClientDataRate[classID];
				}
				sumofclientinclass += slotCounterClass[classID];
				if(realBytesToSpend < -((sint64)allowedDataRate)*1000) {
					sint64 newRealBytesToSpend = -((sint64)allowedDataRate)*1000;
					realBytesToSpend = newRealBytesToSpend;
					if (m_highestNumberOfFullyActivatedSlots[classID] > sumofclientinclass)
						--m_highestNumberOfFullyActivatedSlots[classID];
				} else {
					uint64 bandwidthSavedTolerance = ClientDataRate[classID]*1000;
					if(realBytesToSpend > 0 && (uint64)realBytesToSpend > 999+bandwidthSavedTolerance) {
						sint64 newRealBytesToSpend = 999+bandwidthSavedTolerance;
						realBytesToSpend = newRealBytesToSpend;
						if (m_highestNumberOfFullyActivatedSlots[classID] < sumofclientinclass+1)
							++m_highestNumberOfFullyActivatedSlots[classID];
					}else if (m_highestNumberOfFullyActivatedSlots[classID] > sumofclientinclass)
							--m_highestNumberOfFullyActivatedSlots[classID];
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
    POSITION			pos = m_stat_list.GetStartPosition();
	ThrottledFileSocket* socket;
	Socket_stat* stat;
	while (pos)
	{
		m_stat_list.GetNextAssoc(pos, socket, stat);
		delete stat;
	}
	m_stat_list.RemoveAll();
	m_StandardOrder_list.RemoveAll();
    sendLocker.Unlock();

    return 0;
}
