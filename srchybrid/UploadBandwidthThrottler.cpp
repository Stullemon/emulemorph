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
#include "stdafx.h"
#include "emule.h"
#include "UploadBandwidthThrottler.h"
#include "EMSocket.h"
#include "opcodes.h"
#include "UploadQueue.h"
#include "LastCommonRouteFinder.h"
#include "OtherFunctions.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#endif

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
	m_SentBytesSinceLastCallExcludingOverhead = 0;
	m_highestNumberOfFullyActivatedSlots = 0;
	
	threadEndedEvent = new CEvent(0, 1);
	doRun = true;
	AfxBeginThread(RunProc, (LPVOID)this);
}

/**
* The destructor stops the thread. If the thread has already stoppped, destructor does nothing.
*/
UploadBandwidthThrottler::~UploadBandwidthThrottler(void) {
	EndThread();
	
	delete threadEndedEvent;
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
uint64 UploadBandwidthThrottler::GetNumberOfSentBytesExcludingOverheadSinceLastCallAndReset() {
	sendLocker.Lock();
	
	uint64 numberOfSentBytesSinceLastCall = m_SentBytesSinceLastCallExcludingOverhead;
	m_SentBytesSinceLastCallExcludingOverhead = 0;

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
uint32 UploadBandwidthThrottler::GetHighestNumberOfFullyActivatedSlotsSinceLastCallAndReset() {
	sendLocker.Lock();
	
	uint64 highestNumberOfFullyActivatedSlots = m_highestNumberOfFullyActivatedSlots;
    m_highestNumberOfFullyActivatedSlots = 1;

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
void UploadBandwidthThrottler::AddToStandardList(uint32 index, CEMSocket* socket) {
	if(socket != NULL) {
		sendLocker.Lock();

		RemoveFromStandardListNoLock(socket);

		if(index > (uint32)m_StandardOrder_list.GetSize()) {
			index = m_StandardOrder_list.GetSize();
		}
		m_StandardOrder_list.InsertAt(index, socket);

		sendLocker.Unlock();
	} else {
		//theApp.emuledlg->AddDebugLogLine(true,"Tried to add NULL socket to UploadBandwidthThrottler Standard list! Prevented.");
		//	theApp.emuledlg->AddDebugLogLine(true,"Tried to add NULL socket to UploadBandwidthThrottler Standard list! Prevented.");
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
void UploadBandwidthThrottler::RemoveFromStandardList(CEMSocket* socket) {
	sendLocker.Lock();

	RemoveFromStandardListNoLock(socket);

	sendLocker.Unlock();
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
void UploadBandwidthThrottler::RemoveFromStandardListNoLock(CEMSocket* socket) {
    // Find the slot
	int slotCounter = 0;
	bool foundSocket = false;
	while(slotCounter < m_StandardOrder_list.GetSize() && foundSocket == false) {
		if(m_StandardOrder_list.GetAt(slotCounter) == socket) {
            // Remove the slot
			m_StandardOrder_list.RemoveAt(slotCounter);
			foundSocket = true;
		} else {
			slotCounter++;
		}
	}
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
void UploadBandwidthThrottler::QueueForSendingControlPacket(CEMSocket* socket) {
    // Get critical section
	tempQueueLocker.Lock();


	if(doRun) {
		m_TempControlQueue_list.AddTail(socket);
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
void UploadBandwidthThrottler::RemoveFromAllQueues(CEMSocket* socket) {
    // Get critical section
	sendLocker.Lock();

	if(doRun) {
        // Remove this socket from control packet queue
		{
			POSITION pos1, pos2;
			for (pos1 = m_ControlQueue_list.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
				m_ControlQueue_list.GetNext(pos1);
				CEMSocket* socketinQueue = m_ControlQueue_list.GetAt(pos2);

				if(socketinQueue == socket) {
					m_ControlQueue_list.RemoveAt(pos2);
				}
			}
		}
	    
		tempQueueLocker.Lock();
		{
			POSITION pos1, pos2;
			for (pos1 = m_TempControlQueue_list.GetHeadPosition();( pos2 = pos1 ) != NULL;) {
				m_TempControlQueue_list.GetNext(pos1);
				CEMSocket* socketinQueue = m_TempControlQueue_list.GetAt(pos2);

				if(socketinQueue == socket) {
					m_TempControlQueue_list.RemoveAt(pos2);
				}
			}
		}
		tempQueueLocker.Unlock();

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

    // wait for the thread to signal that it has stopped looping.
	threadEndedEvent->Lock();
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
    //uint32 lastMaxAllowedDataRate = 1;

	while(doRun) {
	DWORD timeSinceLastLoop = ::GetTickCount() - lastLoopTick;

	#define TIME_BETWEEN_UPLOAD_LOOPS 1
	if(timeSinceLastLoop < TIME_BETWEEN_UPLOAD_LOOPS) {
		Sleep(TIME_BETWEEN_UPLOAD_LOOPS-timeSinceLastLoop);
	}

	sendLocker.Lock();

        // Get current speed from UploadSpeedSense
	allowedDataRate = theApp.lastCommonRouteFinder->GetUpload();
	    
        uint32 minFragSize = allowedDataRate / 50;
        if(minFragSize < 512) {
            minFragSize = 512;
        } else if(minFragSize > 1440) {
            minFragSize = 1440;
        }

		const DWORD thisLoopTick = ::GetTickCount();
		timeSinceLastLoop = thisLoopTick - lastLoopTick;
		if(timeSinceLastLoop > 1*1000) {
			theApp.emuledlg->QueueDebugLogLine(false,"UploadBandwidthThrottler: Time since last loop too long (%i).", timeSinceLastLoop);

			timeSinceLastLoop = 1*1000;
			lastLoopTick = thisLoopTick - timeSinceLastLoop;
		}

        // Calculate how many bytes we can spend
		if(allowedDataRate != 0) {
            realBytesToSpend += allowedDataRate*(thisLoopTick-lastLoopTick);
		} else {
            realBytesToSpend = _I64_MAX;
		}
        sint64 bytesToSpend = realBytesToSpend/1000;

		lastLoopTick = thisLoopTick;

		uint64 spentBytes = 0;
		uint64 spentOverhead = 0;

		tempQueueLocker.Lock();

        // are there any sockets in m_TempControlQueue_list? Move them to normal m_ControlQueue_list;
		while(!m_TempControlQueue_list.IsEmpty()) {
			CEMSocket* moveSocket = m_TempControlQueue_list.RemoveHead();
			m_ControlQueue_list.AddTail(moveSocket);
		}

		tempQueueLocker.Unlock();
	    
        uint32 lastSpentBytes = minFragSize;
		// Send any queued up control packets first
        // Send any queued up control packets first
        //while(bytesToSpend > 0 && (spentBytes+minFragSize <= (uint64)bytesToSpend || lastSpentBytes > 0 && spentBytes <= (uint64)bytesToSpend) && !m_ControlQueue_list.IsEmpty()) {
        while(bytesToSpend > 0 && spentBytes/*+minFragSize*/ < /*=*/ (uint64)bytesToSpend && !m_ControlQueue_list.IsEmpty()) {
			CEMSocket* socket = m_ControlQueue_list.RemoveHead();

			if(socket != NULL) {
                SocketSentBytes socketSentBytes = socket->Send(bytesToSpend-spentBytes, minFragSize, true);
                lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
                spentBytes += lastSpentBytes;
				spentOverhead += socketSentBytes.sentBytesControlPackets;
			}
		}

        // Check if any sockets haven't gotten data for a long time. Then trickle them a package.
	for(uint32 slotCounter = 0; slotCounter < (uint32)m_StandardOrder_list.GetSize(); slotCounter++) {
		CEMSocket* socket = m_StandardOrder_list.GetAt(slotCounter);

		if(socket != NULL) {
                if((thisLoopTick-socket->GetLastCalledSend())*1000 > SEC2MS(1)) {
                    // trickle
                    uint32 neededBytes = socket->GetNeededBytes();
				// trickle
                    if(neededBytes > 0) {
                        SocketSentBytes socketSentBytes = socket->Send(neededBytes, minFragSize);
                        lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;
                        spentBytes += lastSpentBytes;
				spentOverhead += socketSentBytes.sentBytesControlPackets;

                        if(lastSpentBytes > 0 && slotCounter+1 < m_highestNumberOfFullyActivatedSlots) {
                            m_highestNumberOfFullyActivatedSlots = slotCounter+1;
			}
                    }
                }
		} else {
                theApp.emuledlg->QueueDebugLogLine(false,"There was a NULL socket in the UploadBandwidthThrottler Standard list (trickle)! Prevented usage. Index: %i Size: %i", slotCounter, m_StandardOrder_list.GetSize());
		}
	}

        lastSpentBytes = minFragSize;

        // Any bandwidth that hasn't been used yet are used for the fully activated upload slots.
        if(m_ControlQueue_list.IsEmpty()) {
            for(uint32 slotCounter = 0; slotCounter < (uint32)m_StandardOrder_list.GetSize() && bytesToSpend > 0 && (/*spentBytes+minFragSize <= (uint64)bytesToSpend || lastSpentBytes > 0 &&*/ spentBytes < (uint64)bytesToSpend); slotCounter++) {
		CEMSocket* socket = m_StandardOrder_list.GetAt(slotCounter);

		if(socket != NULL) {
                    SocketSentBytes socketSentBytes = socket->Send(bytesToSpend-spentBytes, minFragSize);
				lastSpentBytes = socketSentBytes.sentBytesControlPackets + socketSentBytes.sentBytesStandardPackets;

				spentBytes += lastSpentBytes;
				spentOverhead += socketSentBytes.sentBytesControlPackets;

                    if(slotCounter+1 > m_highestNumberOfFullyActivatedSlots && (lastSpentBytes >= minFragSize)) { // || lastSpentBytes > 0 && spentBytes == bytesToSpend /*|| slotCounter+1 == (uint32)m_StandardOrder_list.GetSize())*/)) {
				m_highestNumberOfFullyActivatedSlots = slotCounter+1;
			}
		} else {
			theApp.emuledlg->QueueDebugLogLine(false,"There was a NULL socket in the UploadBandwidthThrottler Standard list (fully activated)! Prevented usage. Index: %i Size: %i", slotCounter, m_StandardOrder_list.GetSize());
		}
	}
        }

        realBytesToSpend -= spentBytes*1000;

        //if(spentBytes > 0) {
        //    theApp.emuledlg->QueueDebugLogLine(false,"UploadBandwidthThrottler::RunInternal(): Sent: %I64i ControlQueueSize: %i", spentBytes, m_ControlQueue_list.GetSize());
        //}

        if(realBytesToSpend < -(((sint64)m_StandardOrder_list.GetSize()+1)*minFragSize)*1000) {
            sint64 newRealBytesToSpend = -(((sint64)m_StandardOrder_list.GetSize()+1)*minFragSize)*1000;

            //theApp.emuledlg->QueueDebugLogLine(false,"UploadBandwidthThrottler::RunInternal(): Overcharged bytesToSpend. Limiting negative value. Old value: %I64i New value: %I64i", realBytesToSpend, newRealBytesToSpend);

            realBytesToSpend = newRealBytesToSpend;
        } else if(realBytesToSpend > 999) {
            sint64 newRealBytesToSpend = 999;

            //theApp.emuledlg->QueueDebugLogLine(false,"UploadBandwidthThrottler::RunInternal(): Too high saved bytesToSpend. Limiting value. Old value: %I64i New value: %I64i", realBytesToSpend, newRealBytesToSpend);
            realBytesToSpend = newRealBytesToSpend;

            m_highestNumberOfFullyActivatedSlots = m_StandardOrder_list.GetSize()+1;
		}

		m_SentBytesSinceLastCall += spentBytes;
		m_SentBytesSinceLastCallExcludingOverhead += spentOverhead;

		sendLocker.Unlock();
	}

	threadEndedEvent->SetEvent();

	tempQueueLocker.Lock();
	m_TempControlQueue_list.RemoveAll();
	tempQueueLocker.Unlock();

	sendLocker.Lock();

	m_ControlQueue_list.RemoveAll();
	m_StandardOrder_list.RemoveAll();
	sendLocker.Unlock();

	return 0;
}

