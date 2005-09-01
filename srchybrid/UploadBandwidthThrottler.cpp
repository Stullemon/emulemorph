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
//#include <math.h>
#include "emule.h"
#include "UploadBandwidthThrottler.h"
#include "EMSocket.h"
#include "opcodes.h"
#include "LastCommonRouteFinder.h"
#include "OtherFunctions.h"
//#include "emuledlg.h"
#include "uploadqueue.h"
#include "preferences.h"

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
	for (uint8 i = 0; i < NB_SPLITTING_CLASS; i++)
		m_highestNumberOfFullyActivatedSlots[i] = (UINT)0;
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
	for (uint8 i = 0; i < NB_SPLITTING_CLASS; i++)
		m_highestNumberOfFullyActivatedSlots[i] = (UINT)0;
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
			cur_socket_stat->realBytesToSpend = 1000;
		}
		cur_socket_stat->classID = classID;
		if (classID==SCHED_CLASS) {
			++slotCounterClass[classID];
			classID=LAST_CLASS;
		}
		++slotCounterClass[classID];
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
			if (classID==SCHED_CLASS) {
				--slotCounterClass[classID];
				classID=LAST_CLASS;
			}
			--slotCounterClass[classID];
			foundSocket = true;
        } else {
            slotCounter++;
        }
    }

	//MORPH START - Added by SiRoB, Upload Splitting Class
	if(resort == false && foundSocket) {
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

#define ADJUSTING_STEP	(1*1024)
    DWORD lastLoopTick = ::GetTickCount();
	//MORPH START - Added by SiRoB, Upload Splitting Class
	uint32 allowedDataRateClass[NB_SPLITTING_CLASS];
	uint32 ClientDataRate[NB_SPLITTING_CLASS];
	sint64 realBytesToSpendClass[NB_SPLITTING_CLASS];
	memset(realBytesToSpendClass,0,sizeof(realBytesToSpendClass));
	uint32 numberoffullconsumedslot[NB_SPLITTING_CLASS];
	//MORPH END   - Added by SiRoB, Upload Splitting Class
	uint32 nEstiminatedLimit = 0;
	sint32 nSlotsBusyLevel = 0;
	DWORD nUploadStartTime = 0;
	bool bUploadUnlimited;

    while(doRun) {
        pauseEvent->Lock();

		theApp.lastCommonRouteFinder->GetClassByteToSend(allowedDataRateClass,ClientDataRate);

		bUploadUnlimited = thePrefs.GetMaxUpload() == UNLIMITED;
		if (bUploadUnlimited && nUploadStartTime != 0 && ::GetTickCount()- nUploadStartTime > SEC2MS(60) ){ // upload is unlimited
			if (theApp.uploadqueue){
				if (nEstiminatedLimit == 0){ // no autolimit was set yet
					if (nSlotsBusyLevel >= 250){ // sockets indicated that the BW limit has been reached
						nEstiminatedLimit = theApp.uploadqueue->GetDatarate() - (3*ADJUSTING_STEP);
						allowedDataRateClass[LAST_CLASS] = min(nEstiminatedLimit, allowedDataRateClass[LAST_CLASS]);
						nSlotsBusyLevel = -200;
					}
				}
				else{
					if (nSlotsBusyLevel > 250){
						nEstiminatedLimit -= ADJUSTING_STEP;
						nSlotsBusyLevel = 0;

					    }
                        else if (nSlotsBusyLevel < (-250)){
						nEstiminatedLimit += ADJUSTING_STEP;
						nSlotsBusyLevel = 0;
					}
						allowedDataRateClass[LAST_CLASS] = min(nEstiminatedLimit, allowedDataRateClass[LAST_CLASS]);
				} 
			}
		}

		uint32 minFragSize = 1300;
        uint32 doubleSendSize = minFragSize*2; // send two packages at a time so they can share an ACK
        if(allowedDataRateClass[LAST_CLASS] < 6*1024) {
            minFragSize = 536;
            doubleSendSize = minFragSize; // don't send two packages at a time at very low speeds to give them a smoother load
        }

		DWORD timeSinceLastLoop = ::GetTickCount() - lastLoopTick;
        uint32 sleepTime = 1;
        if(timeSinceLastLoop < sleepTime)
            Sleep(sleepTime-timeSinceLastLoop);
		const DWORD thisLoopTick = ::GetTickCount();
        if (thisLoopTick - lastLoopTick > 1000) {
			lastLoopTick = thisLoopTick - 1000;
		}
		timeSinceLastLoop = thisLoopTick - lastLoopTick;
		lastLoopTick = thisLoopTick;
		
		sendLocker.Lock();
		if (timeSinceLastLoop > 0) {
			uint32 allowedDataRate = allowedDataRateClass[LAST_CLASS];
			if(_I64_MAX/timeSinceLastLoop > allowedDataRate && _I64_MAX-allowedDataRate*timeSinceLastLoop > realBytesToSpendClass[LAST_CLASS]) {
				realBytesToSpendClass[LAST_CLASS] += allowedDataRate*timeSinceLastLoop;
			} else {
				realBytesToSpendClass[LAST_CLASS] = _I64_MAX;
			}
		}

		if (timeSinceLastLoop > 0) {
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
		}

		sint64 BytesToSpend = realBytesToSpendClass[LAST_CLASS] / 1000;
		uint64 ControlspentBytes = 0;
		uint64 ControlspentOverhead = 0;

		// Send any queued up control packets first
		while(BytesToSpend > 0 && ControlspentBytes < (uint64)BytesToSpend && (!m_ControlQueueFirst_list.IsEmpty() || !m_ControlQueue_list.IsEmpty())) {
			ThrottledControlSocket* socket = NULL;
			
			if(!m_ControlQueueFirst_list.IsEmpty()) {
				socket = m_ControlQueueFirst_list.RemoveHead();
			} else if(!m_ControlQueue_list.IsEmpty()) {
				socket = m_ControlQueue_list.RemoveHead();
			}

			if (socket != NULL) {
				SocketSentBytes socketSentBytes = socket->SendControlData(BytesToSpend - ControlspentBytes, minFragSize);
				uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
				if (lastSpentBytes) {
					Socket_stat* stat = NULL;
					if(m_stat_list.Lookup((ThrottledFileSocket*)socket,stat)) {
						stat->realBytesToSpend -= lastSpentBytes*1000;
						uint32 classID = (stat->classID==SCHED_CLASS)?LAST_CLASS:stat->classID;
						if (classID < LAST_CLASS)
							realBytesToSpendClass[classID] -= lastSpentBytes*1000;
					}
					ControlspentBytes += lastSpentBytes;
					ControlspentOverhead += socketSentBytes.sentBytesControlPackets;
				}
        	}
       	}
					
		uint32 lastclientpos = 0;
		for (uint32 classID = 0; classID < NB_SPLITTING_CLASS; classID++) {
			if (slotCounterClass[classID]) {
				for(uint32 slotCounter = lastclientpos; slotCounter < lastclientpos + slotCounterClass[classID] && BytesToSpend > 0 && ControlspentBytes < (uint64)BytesToSpend; slotCounter++) {
					ThrottledFileSocket* socket = m_StandardOrder_list.GetAt(slotCounter);
					if(socket != NULL) {
						Socket_stat* stat = NULL;
						if (m_stat_list.Lookup(socket, stat)) {
							uint32 neededBytes = socket->GetNeededBytes(1000 > minFragSize);
							if (neededBytes > 0 && GetTickCount()-socket->GetLastCalledSend() >= 1000) {
								SocketSentBytes socketSentBytes = socket->SendFileAndControlData(neededBytes, minFragSize);
								uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
								if (lastSpentBytes) {
									uint64 realByteSpent = lastSpentBytes*1000;
									stat->realBytesToSpend -= realByteSpent;
									if(m_highestNumberOfFullyActivatedSlots[classID] > slotCounter+1)
										m_highestNumberOfFullyActivatedSlots[classID] = slotCounter+1;
									if (m_highestNumberOfFullyActivatedSlots[LAST_CLASS] < m_highestNumberOfFullyActivatedSlots[classID])
										m_highestNumberOfFullyActivatedSlots[LAST_CLASS] = m_highestNumberOfFullyActivatedSlots[classID];
									if (classID < LAST_CLASS)
										realBytesToSpendClass[classID] -= realByteSpent;
									ControlspentBytes += lastSpentBytes;
									ControlspentOverhead += socketSentBytes.sentBytesControlPackets;
								}
							}
						}
					}
				}
				lastclientpos += slotCounterClass[classID];
			}
		}
		realBytesToSpendClass[LAST_CLASS] -= 1000*ControlspentBytes;
		m_SentBytesSinceLastCall += ControlspentBytes;
		m_SentBytesSinceLastCallOverhead += ControlspentOverhead;

		lastclientpos = 0;
		for (uint32 classID = 0; classID < NB_SPLITTING_CLASS; classID++) {
			//Calculate allowed data to spend for a class (realBytesToSpendClass)
			BytesToSpend = realBytesToSpendClass[LAST_CLASS] / 1000;
			if(classID < LAST_CLASS) {
				if (allowedDataRateClass[classID] > 0) {
					if(timeSinceLastLoop > 0) {
						if(_I64_MAX/timeSinceLastLoop > allowedDataRateClass[classID] && _I64_MAX-allowedDataRateClass[classID]*timeSinceLastLoop > realBytesToSpendClass[classID]) {
							realBytesToSpendClass[classID] += allowedDataRateClass[classID]*timeSinceLastLoop;
						} else {
							realBytesToSpendClass[classID] = realBytesToSpendClass[LAST_CLASS];
						}
					}
					sint64 curClassByteToSpend = realBytesToSpendClass[classID] / 1000;
					if (BytesToSpend > curClassByteToSpend) {
						BytesToSpend = curClassByteToSpend;
					}
				} else {
					realBytesToSpendClass[classID] = realBytesToSpendClass[LAST_CLASS];
				}
			}
			uint64 spentBytes = 0;
			uint64 spentOverhead = 0;
			memset(numberoffullconsumedslot,0,sizeof(numberoffullconsumedslot));
			if(slotCounterClass[classID]) {
				for(uint32 slotCounter = lastclientpos; slotCounter < lastclientpos + slotCounterClass[classID]; slotCounter++) {
					ThrottledFileSocket* socket = m_StandardOrder_list.GetAt(slotCounter);
					if(socket != NULL) {
						Socket_stat* stat = NULL;
						if (m_stat_list.Lookup(socket,stat)) {
							//calculate client allowed data for a client (stat->realBytesToSpend)
							if (timeSinceLastLoop > 0) {
								if (ClientDataRate[classID] > 0) {
									if  (_I64_MAX/timeSinceLastLoop > ClientDataRate[classID] && _I64_MAX-ClientDataRate[classID]*timeSinceLastLoop > stat->realBytesToSpend)
										stat->realBytesToSpend += ClientDataRate[classID]*timeSinceLastLoop;
									else
										stat->realBytesToSpend = _I64_MAX;
								} else {
									stat->realBytesToSpend = _I64_MAX;
								}

							}
							//Try to send client allowed data for a client but not more than class allowed data
							if (stat->realBytesToSpend > 999 && stat->classID < SCHED_CLASS) {
								if (BytesToSpend > 0 && spentBytes < (uint64)BytesToSpend) {
									uint32 BytesToSpendTemp = min(stat->realBytesToSpend / 1000, BytesToSpend - (sint64)spentBytes);
									SocketSentBytes socketSentBytes = socket->SendFileAndControlData(BytesToSpendTemp, doubleSendSize);
									uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
									stat->realBytesToSpend -= lastSpentBytes*1000;
									if(slotCounter+1 > m_highestNumberOfFullyActivatedSlots[classID] && (lastSpentBytes >= doubleSendSize)) // || lastSpentBytes > 0 && spentBytes == bytesToSpend /*|| slotCounter+1 == (uint32)m_StandardOrder_list.GetSize())*/))
										m_highestNumberOfFullyActivatedSlots[classID] = slotCounter+1;
									if (m_highestNumberOfFullyActivatedSlots[classID] > m_highestNumberOfFullyActivatedSlots[LAST_CLASS])
										m_highestNumberOfFullyActivatedSlots[LAST_CLASS] = m_highestNumberOfFullyActivatedSlots[classID];
									spentBytes += lastSpentBytes;
									spentOverhead += socketSentBytes.sentBytesControlPackets;
								}
							}
						}
					}
				}
				// Any bandwidth that hasn't been used yet are used first to last.
				for(uint32 slotCounter = lastclientpos; slotCounter < lastclientpos + slotCounterClass[classID]; slotCounter++) {
					ThrottledFileSocket* socket = m_StandardOrder_list.GetAt(slotCounter);
					if(socket != NULL) {
						Socket_stat* stat = NULL;
						if (m_stat_list.Lookup(socket,stat)) {
							if (stat->realBytesToSpend > 999) {
								if (BytesToSpend > 0 && spentBytes < (uint64)BytesToSpend) {
									uint32 BytesToSpendTemp = min(stat->realBytesToSpend/1000, BytesToSpend - (sint64)spentBytes);
									SocketSentBytes socketSentBytes = socket->SendFileAndControlData(BytesToSpendTemp, doubleSendSize);
									uint32 lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
									stat->realBytesToSpend -= lastSpentBytes*1000;
									if(slotCounter+1 > m_highestNumberOfFullyActivatedSlots[classID]) // || lastSpentBytes > 0 && spentBytes == bytesToSpend /*|| slotCounter+1 == (uint32)m_StandardOrder_list.GetSize())*/))
										m_highestNumberOfFullyActivatedSlots[classID] = slotCounter+1;
									if (m_highestNumberOfFullyActivatedSlots[classID] > m_highestNumberOfFullyActivatedSlots[LAST_CLASS])
										m_highestNumberOfFullyActivatedSlots[LAST_CLASS] = m_highestNumberOfFullyActivatedSlots[classID];
									spentBytes += lastSpentBytes;
									spentOverhead += socketSentBytes.sentBytesControlPackets;
								}
								if  (stat->realBytesToSpend > max(999, ClientDataRate[classID])) {
									stat->realBytesToSpend = max(999, ClientDataRate[classID]);
								} else
									++numberoffullconsumedslot[classID];

							}
						}
					}
				}
				if (classID < LAST_CLASS)
					realBytesToSpendClass[classID] -= spentBytes*1000;

				lastclientpos += slotCounterClass[classID];
				realBytesToSpendClass[LAST_CLASS] -= spentBytes*1000;
				
				m_SentBytesSinceLastCall += spentBytes;
				m_SentBytesSinceLastCallOverhead += spentOverhead;
			}
			if (bUploadUnlimited){
				int cBusy = 0;
				for (int i = 0; i < m_StandardOrder_list.GetSize(); i++){
					if (m_StandardOrder_list[i] != NULL && m_StandardOrder_list[i]->IsBusy())
						cBusy++;
				}
				if (m_StandardOrder_list.GetSize() > 0){
					float fBusyPercent = ((float)cBusy/(float)m_StandardOrder_list.GetSize()) * 100;
					if (cBusy > 2 && fBusyPercent > 75.00f && nSlotsBusyLevel < 255){
						nSlotsBusyLevel++;
					}
					else if ( (cBusy <= 2 || fBusyPercent < 25.00f) && nSlotsBusyLevel > (-255)){
						nSlotsBusyLevel--;
					}
					if (m_StandardOrder_list.GetSize() >= 3 && nUploadStartTime == 0)
						nUploadStartTime = ::GetTickCount();
				}
			}
		}
		if (timeSinceLastLoop > 0) {
			lastclientpos = m_StandardOrder_list.GetSize();
			for (int classID = LAST_CLASS; classID >= 0; classID--) {
				if (m_highestNumberOfFullyActivatedSlots[classID] == lastclientpos && numberoffullconsumedslot > 0)
					++m_highestNumberOfFullyActivatedSlots[classID];
				if (realBytesToSpendClass[classID] > max(999, allowedDataRateClass[classID])) {
					realBytesToSpendClass[classID] = max(999, allowedDataRateClass[classID]);
					if (realBytesToSpendClass[classID] > realBytesToSpendClass[LAST_CLASS])
						realBytesToSpendClass[classID] = realBytesToSpendClass[LAST_CLASS];
					if (m_highestNumberOfFullyActivatedSlots[classID] < lastclientpos+1)
						++m_highestNumberOfFullyActivatedSlots[classID];
				}
				lastclientpos -= slotCounterClass[classID];
			}
		}
		sendLocker.Unlock();
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
