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
#include "UploadQueue.h"
#include "Packets.h"
#include "KnownFile.h"
#include "ListenSocket.h"
#include "Exceptions.h"
#include "Scheduler.h"
#include "PerfLog.h"
#include "UploadBandwidthThrottler.h"
#include "ClientList.h"
#include "LastCommonRouteFinder.h"
#include "DownloadQueue.h"
#include "FriendList.h"
#include "Statistics.h"
#include "MMServer.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "Sockets.h"
#include "ClientCredits.h"
#include "Server.h"
#include "ServerList.h"
#include "WebServer.h"
#include "emuledlg.h"
#include "ServerWnd.h"
#include "TransferWnd.h"
#include "SearchDlg.h"
#include "StatisticsDlg.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "Kademlia/Kademlia/Prefs.h"
#include "PartFile.h" //MORPH - Added by SiRoB

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static uint32 counter, sec,statsave;
static UINT _uSaveStatistics = 0;
// -khaos--+++> Added iupdateconnstats...
static uint32 igraph, istats, iupdateconnstats;
// <-----khaos-

//TODO rewrite the whole networkcode, use overlapped sockets

CUploadQueue::CUploadQueue()
{
	VERIFY( (h_timer = SetTimer(0,0,100,UploadTimer)) != NULL );
	if (thePrefs.GetVerbose() && !h_timer)
		AddDebugLogLine(true,_T("Failed to create 'upload queue' timer - %s"),GetErrorMessage(GetLastError()));
	datarate = 0;
	counter=0;
	successfullupcount = 0;
	failedupcount = 0;
	totaluploadtime = 0;
	m_nLastStartUpload = 0;
	statsave=0;
	// -khaos--+++>
	iupdateconnstats=0;
	// <-----khaos-

    m_iHighestNumberOfFullyActivatedSlotsSinceLastCall = 0;
	m_MaxActiveClients = 0;
	m_MaxActiveClientsShortTime = 0;

	m_lastCalculatedDataRateTick = 0;
	m_avarage_dr_sum = 0;
	friendDatarate = 0;

	m_dwLastResortedUploadSlots = 0;
	//MORPH START - Added by SiRoB, ZZUL_20040904
	m_dwLastCheckedForHighPrioClient = 0;

    m_dwLastCalculatedAverageCombinedFilePrioAndCredit = 0;
    m_fAverageCombinedFilePrioAndCredit = 0;
	//MORPH END   - Added by SiRoB, ZZUL_20040904
	//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	AvgRespondTime[0]=500;
	AvgRespondTime[1]=thePrefs.GetSUCPitch()/2;//Changed by Yun.SF3 (this is too much divided, original is 3)
	MaxVUR=512*(thePrefs.GetMaxUpload()+thePrefs.GetMinUpload()); //When we start with SUC take the middle range for upload
	//MORPH END - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
}

/**
 * Remove the client from upload socket if there's another client with same/higher
 * class that wants to get an upload socket. If there's not another matching client
 * move this client down in the upload list so that it is after all other of it's class.
 *
 * @param client address of the client that should be removed or moved down
 *
 * @return true if the client was removed. false if it is still in upload list
 */
bool CUploadQueue::RemoveOrMoveDown(CUpDownClient* client, bool onlyCheckForRemove) {
    if(onlyCheckForRemove == false) {
        CheckForHighPrioClient();
    }

    CUpDownClient* newclient = FindBestClientInQueue(true, client);

    if(newclient != NULL && // Only remove the client if there's someone to replace it
		RightClientIsSuperior(client, newclient) >= 0
      ){

        // Remove client from ul list to make room for higher/same prio client
        AddDebugLogLine(false, GetResString(IDS_ULSUCCESSFUL), client->GetUserName(), CastItoXBytes(client->GetQueueSessionPayloadUp()), CastItoXBytes(client->GetCurrentSessionLimit()), (sint32)client->GetQueueSessionPayloadUp()-client->GetCurrentSessionLimit());

        client->SetWaitStartTime();
	    theApp.uploadqueue->RemoveFromUploadQueue(client, GetResString(IDS_REMULSUCCESS));
	    theApp.uploadqueue->AddClientToQueue(client,true);

        return true;
    } else if(onlyCheckForRemove == false) {
        // Move down
        // first find the client in the uploadinglist
        uint32 posCounter = 0;
        POSITION foundPos = NULL;
        POSITION pos = uploadinglist.GetHeadPosition();
	    while(pos != NULL && foundPos == NULL) {
		    if (uploadinglist.GetAt(pos) == client){
                foundPos = pos;
            } else {
                uploadinglist.GetNext(pos);
                posCounter++;
            }
	    }

        if(foundPos != NULL) {
            // Remove the found Client
		    uploadinglist.RemoveAt(foundPos);
            theApp.uploadBandwidthThrottler->RemoveFromStandardList(client->socket);

            // then add it last in it's class
            InsertInUploadingList(client);
        }

        return false;
    } else {
        return false;
    }
}
/**
* Compares two clients, considering requested file and score (waitingtime, credits, requested file prio), and decides if the right
* client is better than the left clien. If so, it returns true.
*
* Clients are ranked in the following classes:
*    1: Friends (friends are internally ranked by which file they want; if it is powershared; upload priority of file)
*    x: Clients that need to be PayBackFirst
*    2: Clients that wants powershared files of prio release
*    3: Clients that wants powershared files of prio high
*    4: Clients that wants powershared files of prio normal
*    5: Clients that wants powershared files of prio low
*    6: Clients that wants powershared files of prio lowest
*    7: Other clients
*
* Clients are then ranked inside their classes by their credits and waiting time (== score).
*
* Another description of the above ranking:
*
* First sortorder is if the client is a friend with a friend slot. Any client that is a friend with a friend slot,
* is ranked higher than any client that does not have a friend slot.
*
* Second sortorder is if the requested file if a powershared file. All clients wanting powershared files are ranked higher
* than any client wanting a not powershared filed.
*
* If the file is powershared, then second sortorder is file priority. For instance. Any client wanting a powershared file with
* upload priority high, is ranked higher than any client wanting a powershared file with upload file priority normal.
*
* If both clients wants powershared files, and of the same upload priority, then the score is used to decide which client is better.
* The score, as usual, weighs in the client's waiting time, credits, and requested file's upload priority.
*
* If both clients wants files that are not powershared, then scores are used to compare the clients, as in official eMule.
*
* @param leftClient a pointer to the left client
*
* @param leftScore the precalculated score for leftClient, which is calculated with leftClient->GetSCore()
*
* @param rightClient a pointer to the right client
*
* @param rightScore the precalculated score for rightClient, which is calculated with rightClient->GetSCore()
*
* @return true if right client is better, false if clients are equal. False if left client is better.
*/
bool CUploadQueue::RightClientIsBetter(CUpDownClient* leftClient, uint32 leftScore, CUpDownClient* rightClient, uint32 rightScore) {
    if(!rightClient) {
        return false;
    }

    bool leftLowIdMissed = false;
    bool rightLowIdMissed = false;
    
    if(leftClient) {
        leftLowIdMissed = leftClient->HasLowID() && leftClient->socket && leftClient->socket->IsConnected() && leftClient->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick;
        rightLowIdMissed = rightClient->HasLowID() && rightClient->socket && rightClient->socket->IsConnected() && rightClient->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick;
    }

	int iSuperior;
	if(
		(leftClient != NULL &&
			(
				(iSuperior = RightClientIsSuperior(leftClient, rightClient)) > 0 ||
				iSuperior == 0 &&
				(//Morph - added by AndCycle, try to finish faster for the one have finished more than others, for keep full chunk transfer
					leftClient->GetQueueSessionPayloadUp() < rightClient->GetQueueSessionPayloadUp() ||
					leftClient->GetQueueSessionPayloadUp() == rightClient->GetQueueSessionPayloadUp() &&
					(//Morph - added by AndCycle, Equal Chance For Each File
						leftClient->GetEqualChanceValue() > rightClient->GetEqualChanceValue() ||	//rightClient want a file have less chance been uploaded
						leftClient->GetEqualChanceValue() == rightClient->GetEqualChanceValue() &&
						(
							leftClient->GetFilePrioAsNumber() ==  rightClient->GetFilePrioAsNumber() || // same prio file
							leftClient->GetPowerShared() == false && rightClient->GetPowerShared() == false //neither want powershare file
				        ) && // they are equal in powersharing
				        (
							!leftLowIdMissed && rightLowIdMissed || // rightClient is lowId and has missed a slot and is currently connected
				
							leftLowIdMissed && rightLowIdMissed && // both have missed a slot and both are currently connected
							leftClient->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick > rightClient->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick || // but right client missed earlier
				
							(
								!leftLowIdMissed && !rightLowIdMissed || // none have both missed and is currently connected
				
								leftLowIdMissed && rightLowIdMissed && // both have missed a slot
								leftClient->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick == rightClient->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick // and at same time (should hardly ever happen)
				            ) &&
							rightScore > leftScore // but rightClient has better score, so rightClient is better
						)
					)
				)
			) ||
			leftClient == NULL // there's no old client to compare with, so rightClient is better (than null)
		) &&
		(!rightClient->IsBanned()) && // don't allow banned client to be best
		IsDownloading(rightClient) == false // don't allow downloading clients to be best
	) {
		return true;
	} else {
		return false;
	}

}

//Morph Start - added by AndCycle, separate special prio compare
int CUploadQueue::RightClientIsSuperior(CUpDownClient* leftClient, CUpDownClient* rightClient)
{
	if(leftClient == NULL){
		return 1;
	}
	if(rightClient == NULL){
		return -1;
	}
	if((leftClient->IsFriend() && leftClient->GetFriendSlot()) == false && (rightClient->IsFriend() && rightClient->GetFriendSlot()) == true){
		return 1;
	}
	else if((leftClient->IsFriend() && leftClient->GetFriendSlot()) == true && (rightClient->IsFriend() && rightClient->GetFriendSlot()) == false){
		return -1;
	}
	if(leftClient->IsPBForPS() == false && rightClient->IsPBForPS() == true){
		return 1;
	}
	if(leftClient->IsPBForPS() == true && rightClient->IsPBForPS() == false){
		return -1;
	}
	else
		return 0;
}
//Morph End - added by AndCycle, separate special prio compare

/**
* Find the highest ranking client in the waiting queue, and return it.
* Clients are ranked in the following classes:
* Clients are ranked in the following classes:
*    1: Friends (friends are internally ranked by which file they want; if it is powershared; upload priority of file)
*    x: Clients that need to be PayBackFirst
*    2: Clients that wants powershared files of prio release
*    3: Clients that wants powershared files of prio high
*    4: Clients that wants powershared files of prio normal
*    5: Clients that wants powershared files of prio low
*    6: Clients that wants powershared files of prio lowest
*    7: Other clients
*
* Clients are then ranked inside their classes by their credits and waiting time (== score).
*
* Low id client are ranked as lowest possible, unless they are currently connected.
* A low id client that is not connected, but would have been ranked highest if it
* had been connected, gets a flag set. This flag means that the client should be
* allowed to get an upload slot immediately once it connects.
*
* @return address of the highest ranking client.
*/
CUpDownClient* CUploadQueue::FindBestClientInQueue(bool allowLowIdAddNextConnectToBeSet, CUpDownClient* lowIdClientMustBeInSameOrBetterClassAsThisClient) {
	POSITION toadd = 0;
	POSITION toaddlow = 0;
	uint32	bestscore = 0;
	uint32  bestlowscore = 0;

	CUpDownClient* newclient = NULL;
	CUpDownClient* lowclient = NULL;

	POSITION pos1, pos2;
	for (pos1 = waitinglist.GetHeadPosition();( pos2 = pos1 ) != NULL;)
	{
		waitinglist.GetNext(pos1);
		CUpDownClient* cur_client =	waitinglist.GetAt(pos2);
		// clear dead clients
		ASSERT ( cur_client->GetLastUpRequest() );
		if ((::GetTickCount() - cur_client->GetLastUpRequest() > MAX_PURGEQUEUETIME) || !theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID()) )
		{
			RemoveFromWaitingQueue(pos2,true);
			continue;
		}
		else
		{
			// finished clearing
			uint32 cur_score = cur_client->GetScore(false);

			if (RightClientIsBetter(newclient, bestscore, cur_client, cur_score))
			{
                // cur_client is more worthy than current best client that is ready to go (connected).
				if(!cur_client->HasLowID() || (cur_client->socket && cur_client->socket->IsConnected())) {
                    // this client is a HighID or a lowID client that is ready to go (connected)
                    // and it is more worthy
					bestscore = cur_score;
					toadd = pos2;
                    newclient = waitinglist.GetAt(toadd);
                } else if(allowLowIdAddNextConnectToBeSet && !cur_client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick) {
                    // this client is a lowID client that is not ready to go (not connected)
    
                    // now that we know this client is not ready to go, compare it to the best not ready client
                    // the best not ready client may be better than the best ready client, so we need to check
                    // against that client
					if (RightClientIsBetter(lowclient, bestlowscore, cur_client, cur_score)){
						// it is more worthy, keep it
						bestlowscore = cur_score;
						toaddlow = pos2;
                        lowclient = waitinglist.GetAt(toaddlow);
					}
				}
			} else {
					// cur_client is more worthy. Save it.
			}
		}
	}
		
	if (bestlowscore > bestscore && lowclient && allowLowIdAddNextConnectToBeSet)
	{
		newclient = waitinglist.GetAt(toaddlow);

		// is newclient in same or better class as lowIdClientMustBeInSameOrBetterClassAsThisClient?
		if(lowIdClientMustBeInSameOrBetterClassAsThisClient == NULL ||
			RightClientIsSuperior(lowIdClientMustBeInSameOrBetterClassAsThisClient, newclient) >= 0
		){
			DWORD connectTick = ::GetTickCount();
              if(connectTick == 0) connectTick = 1;
		      lowclient->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick = connectTick;
		}
	}

	if (!toadd)
	{
		return NULL;
	}
	else
	{
		return waitinglist.GetAt(toadd);
	}
}

/**
 * Insert the client at the correct place in the uploading list.
 * The client should be inserted after all of its class, but before any
 * client of a lower ranking class.
 *
 * Clients are ranked in the following classes:
 *    1: Friends (friends are internally ranked by which file they want; if it is powershared; upload priority of file)
*    x: Clients that need to be PayBackFirst
 *    2: Clients that wants powershared files of prio release
 *    3: Clients that wants powershared files of prio high
 *    4: Clients that wants powershared files of prio normal
 *    5: Clients that wants powershared files of prio low
 *    6: Clients that wants powershared files of prio lowest
 *    7: Other clients
 *
 * Since low ID clients are only put in an upload slot when they call us, it means they will
 * have to wait about 10-30 minutes longer to be put in an upload slot than a high id client.
 * In that time, the low ID client could possibly have gone from being a trickle slot, into
 * being a fully activated slot. At the time when the low ID client would have been put into an
 * upload slot, if it had been a high id slot, a boolean flag is set to true (AddNextConnect = true).
 *
 * A client that has AddNextConnect set when it calls back, will immiediately be given an upload slot.
 * When it is added to the upload list with this method, it will also get the time when it entered the
 * queue taken into consideration. It will be added so that it is before all clients (within in its class) 
 * that entered queue later than it. This way it will be able to possibly skip being a trickle slot,
 * since it has already been forced to wait extra time to be put in a upload slot. This makes the
 * low ID clients have almost exactly the same bandwidth from us (proportionally to the number of low ID
 * clients compared to the number of high ID clients) as high ID clients. This is a definitely a further
 * improvement of VQB's excellent low ID handling.
 *
 * @param newclient address of the client that should be inserted in the uploading list
 */
void CUploadQueue::InsertInUploadingList(CUpDownClient* newclient) {
	POSITION insertPosition = NULL;
	uint32 posCounter = uploadinglist.GetCount();

	uint32 newclientScore = newclient->GetScore(false);

	bool foundposition = false;
	POSITION pos = uploadinglist.GetTailPosition();
	while(pos != NULL && foundposition == false) {
		CUpDownClient* uploadingClient = uploadinglist.GetAt(pos);

		int iSuperior;//Morph - added by AndCycle, separate special prio compare
		if(
			(iSuperior = RightClientIsSuperior(newclient, uploadingClient)) > 0 ||
			iSuperior == 0 &&
			(
             //uploadingClient->GetDatarate() > newclient->GetDatarate() ||
             //uploadingClient->GetDatarate() == newclient->GetDatarate() &&
             (!newclient->HasLowID() || !newclient->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick ||
             newclient->HasLowID() && newclient->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick && (::GetTickCount()-newclient->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick) <= uploadingClient->GetUpStartTimeDelay()
             )
            )
		) {
			foundposition = true;
		} else {
			insertPosition = pos;
			uploadinglist.GetPrev(pos);
			posCounter--;
		}
	}

	if(insertPosition != NULL) {
		POSITION renumberPosition = insertPosition;
		uint32 renumberSlotNumber = posCounter;
	    
		uploadinglist.GetNext(renumberPosition);
		while(renumberPosition != NULL) {
			renumberSlotNumber++;

			CUpDownClient* renumberClient = uploadinglist.GetAt(renumberPosition);

			renumberClient->SetSlotNumber(renumberSlotNumber+1);

			uploadinglist.GetNext(renumberPosition);
		}

		// add it at found pos
		newclient->SetSlotNumber(posCounter+1);
		uploadinglist.InsertBefore(insertPosition, newclient);
		if (newclient->GetFriendSlot())
			theApp.uploadBandwidthThrottler->AddToStandardList(uploadinglist.GetCount(), newclient->GetFileUploadSocket(),0);
		else
			theApp.uploadBandwidthThrottler->AddToStandardList(posCounter, newclient->socket,2);
	}	else{
		// Add it last
		if (newclient->GetFriendSlot())
			theApp.uploadBandwidthThrottler->AddToStandardList(uploadinglist.GetCount(), newclient->GetFileUploadSocket(),0);
		else
			theApp.uploadBandwidthThrottler->AddToStandardList(uploadinglist.GetCount(), newclient->GetFileUploadSocket(),2);
		uploadinglist.AddTail(newclient);
		newclient->SetSlotNumber(uploadinglist.GetCount());
	}
}

bool CUploadQueue::AddUpNextClient(CUpDownClient* directadd, bool highPrioCheck) {
	CUpDownClient* newclient = NULL;
	// select next client or use given client
	if (!directadd)
	{
        newclient = FindBestClientInQueue(highPrioCheck == false);

		if(newclient) {
            if(highPrioCheck == true) {
                if((newclient->IsFriend() && newclient->GetFriendSlot()) || newclient->GetPowerShared()) {
		            POSITION lastpos = uploadinglist.GetTailPosition();

                    CUpDownClient* lastClient = NULL;
                    if(lastpos != NULL) {
                        lastClient = uploadinglist.GetAt(lastpos);
                    }
					if(lastClient != NULL) {

						if (RightClientIsSuperior(lastClient, newclient) > 0) {

							//theApp.emuledlg->AddDebugLogLine(false, "%s: Ended upload to make room for higher prio client.", lastClient->GetUserName());
							// Remove last client from ul list to make room for higher prio client
							theApp.uploadqueue->RemoveFromUploadQueue(lastClient, GetResString(IDS_REMULHIGHERPRIO), true, true);

							// add to queue again.
							// the client is allowed to keep its waiting position in the queue, since it was pre-empted
                            AddClientToQueue(lastClient,true, true);
                        } else {
                            return false;
                        }
                    }
                } else {
                    return false;
                }
            }

			RemoveFromWaitingQueue(newclient, true);
			theApp.emuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
		}
	}
	else
	{
		newclient = directadd;
		/*if (!IsDownloading(newclient)){
            if (newclient->HasLowID())
                AddLogLine(true,"DirectAdd:  LowID: %s", newclient->GetUserName());
			else
                AddLogLine(true,"DirectAdd:  HighID: %s", newclient->GetUserName());
		}*/
	}

	if(newclient == NULL) {
		return false;
	}

	/*
	if (!thePrefs.TransferFullChunks())
		UpdateMaxClientScore(); // refresh score caching, now that the highest score is removed
	*/
	if (IsDownloading(newclient))
	{
		return false;
	}
	// tell the client that we are now ready to upload
	if (!newclient->socket || !newclient->socket->IsConnected())
	{
		newclient->SetUploadState(US_CONNECTING);
		if (!newclient->TryToConnect(true))
			return false;
	}
	else
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__AcceptUploadReq", newclient);
		Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
		theStats.AddUpDataOverheadFileRequest(packet->size);
		newclient->socket->SendPacket(packet,true);
		newclient->SetUploadState(US_UPLOADING);
	}

	newclient->SetUpStartTime();
	newclient->ResetSessionUp();
	// khaos::kmod+ Show Compression by Tarod
	newclient->ResetCompressionGain();
	// khaos::kmod-

	InsertInUploadingList(newclient);
	
    m_nLastStartUpload = ::GetTickCount();

    if(newclient->GetQueueSessionUp() > 0) {
        // This client has already gotten a successfullupcount++ when it was early removed.
        // Negate that successfullupcount++ so we can give it a new one when this session ends
        // this prevents a client that gets put back first on queue, being counted twice in the
        // stats.
        successfullupcount--;
        theStats.DecTotalCompletedBytes(newclient->GetQueueSessionUp());
    }

	// statistic
	CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)newclient->GetUploadFileID());
	if (reqfile){
		reqfile->statistic.AddAccepted();
	}
	
	theApp.emuledlg->transferwnd->uploadlistctrl.AddClient(newclient);

	return true;
}

void CUploadQueue::UpdateActiveClientsInfo(DWORD curTick) {
    // Save number of active clients for statistics
    uint32 tempHighest = theApp.uploadBandwidthThrottler->GetHighestNumberOfFullyActivatedSlotsSinceLastCallAndReset();

    if(thePrefs.GetLogUlDlEvents() && theApp.uploadBandwidthThrottler->GetStandardListSize() > (uint32)uploadinglist.GetSize()) {
        // debug info, will remove this when I'm done.
        //AddDebugLogLine(false, _T("UploadQueue: Error! Throttler has more slots than UploadQueue! Throttler: %i UploadQueue: %i Tick: %i"), theApp.uploadBandwidthThrottler->GetStandardListSize(), uploadinglist.GetSize(), ::GetTickCount());

		if(tempHighest > (uint32)uploadinglist.GetSize()) {
        	tempHighest = uploadinglist.GetSize();
		}
    }

    m_iHighestNumberOfFullyActivatedSlotsSinceLastCall = tempHighest;

    // save 15 minutes of data about number of fully active clients
    uint32 tempMaxRemoved = 0;
    while(!activeClients_tick_list.IsEmpty() && !activeClients_list.IsEmpty() && curTick-activeClients_tick_list.GetHead() > 20*1000) {
            activeClients_tick_list.RemoveHead();
	        uint32 removed = activeClients_list.RemoveHead();

            if(removed > tempMaxRemoved) {
                tempMaxRemoved = removed;
            }
        }

	activeClients_list.AddTail(m_iHighestNumberOfFullyActivatedSlotsSinceLastCall);
    activeClients_tick_list.AddTail(curTick);

    if(activeClients_tick_list.GetSize() > 1) {
        uint32 tempMaxActiveClients = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
        uint32 tempMaxActiveClientsShortTime = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
        POSITION activeClientsTickPos = activeClients_tick_list.GetTailPosition();
        POSITION activeClientsListPos = activeClients_list.GetTailPosition();
        while(activeClientsListPos != NULL && (tempMaxRemoved > tempMaxActiveClients && tempMaxRemoved >= m_MaxActiveClients || curTick - activeClients_tick_list.GetAt(activeClientsTickPos) < 10 * 1000)) {
			DWORD activeClientsTickSnapshot = activeClients_tick_list.GetAt(activeClientsTickPos);
			uint32 activeClientsSnapshot = activeClients_list.GetAt(activeClientsListPos);

			if(activeClientsSnapshot > tempMaxActiveClients) {
				tempMaxActiveClients = activeClientsSnapshot;
			}

			if(activeClientsSnapshot > tempMaxActiveClientsShortTime && curTick - activeClientsTickSnapshot < 10 * 1000) {
				tempMaxActiveClientsShortTime = activeClientsSnapshot;
			}

            activeClients_tick_list.GetPrev(activeClientsTickPos);
            activeClients_list.GetPrev(activeClientsListPos);
		}

        if(tempMaxActiveClients > m_MaxActiveClients) {
			m_MaxActiveClients = tempMaxActiveClients;
        }

		m_MaxActiveClientsShortTime = tempMaxActiveClientsShortTime;
    } else {
        m_MaxActiveClients = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
        m_MaxActiveClientsShortTime = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
    }
}

/**
 * Maintenance method for the uploading slots. It adds and removes clients to the
 * uploading list. It also makes sure that all the uploading slots' Sockets always have
 * enough packets in their queues, etc.
 *
 * This method is called aproximately once every 100 milliseconds.
 */
void CUploadQueue::Process() {

	theApp.sharedfiles->Publish();

    DWORD curTick = ::GetTickCount();

	UpdateActiveClientsInfo(curTick);

    CheckForHighPrioClient();
	//Morph Start - changed by AndCycle, Dont Remove Spare Trickle Slot
	/*
	if(uploadinglist.GetSize() > 0 && (uint32)uploadinglist.GetCount() > m_MaxActiveClientsShortTime+GetWantedNumberOfTrickleUploads() && AcceptNewClient(uploadinglist.GetSize()-1) == false) {
	*/
	if(thePrefs.DoRemoveSpareTrickleSlot() && uploadinglist.GetSize() > 0 && (uint32)uploadinglist.GetCount() > m_MaxActiveClientsShortTime+GetWantedNumberOfTrickleUploads() && AcceptNewClient(uploadinglist.GetSize()-1) == false) {
	//Morph End - changed by AndCycle, Dont Remove Spare Trickle Slot
        // we need to close a trickle slot and put it back first on the queue

        POSITION lastpos = uploadinglist.GetTailPosition();

        CUpDownClient* lastClient = NULL;
        if(lastpos != NULL) {
            lastClient = uploadinglist.GetAt(lastpos);
        }

        if(lastClient != NULL && lastClient->GetUpStartTimeDelay() > 3*1000) {
            // There's too many open uploads (propably due to the user changing
            // the upload limit to a lower value). Remove the last opened upload and put
            // it back on the waitinglist. When it is put back, it get
            // to keep its waiting time. This means it is likely to soon be
            // choosen for upload again.

            // Remove from upload list.
            RemoveFromUploadQueue(lastClient, _T("Too many upload slots opened."), true, true);

		    // add to queue again.
            // the client is allowed to keep its waiting position in the queue, since it was pre-empted
            AddClientToQueue(lastClient,true, true);
        }
    } else if (ForceNewClient()){
        // There's not enough open uploads. Open another one.
		AddUpNextClient();
	}

    // The loop that feeds the upload slots with data.
	POSITION pos = uploadinglist.GetHeadPosition();
	while(pos != NULL){
        // Get the client. Note! Also updates pos as a side effect.
		CUpDownClient* cur_client = uploadinglist.GetNext(pos);
		if (thePrefs.m_iDbgHeap >= 2)
			ASSERT_VALID(cur_client);
		//It seems chatting or friend slots can get stuck at times in upload.. This needs looked into..
		if (!cur_client->socket)
		{
			RemoveFromUploadQueue(cur_client, _T("Uploading to client without socket? (CUploadQueue::Process)"));
			if(cur_client->Disconnected(_T("CUploadQueue::Process"))){
				delete cur_client;
			}
		} else {
			cur_client->SendBlockData();
		}
	}

	//MORPH START - Changed by SiRoB, Better datarate mesurement for low and high speed
	uint64 sentBytes = theApp.uploadBandwidthThrottler->GetNumberOfSentBytesSinceLastCallAndReset();
	if (sentBytes>0) {
    	// Save used bandwidth for speed calculations
		avarage_dr_list.AddTail(sentBytes);
		m_avarage_dr_sum += sentBytes;
		uint64 sentBytesOverhead = theApp.uploadBandwidthThrottler->GetNumberOfSentBytesOverheadSinceLastCallAndReset();

		avarage_friend_dr_list.AddTail(theStats.sessionSentBytesToFriend);
    	// Save time beetween each speed snapshot
		avarage_tick_list.AddTail(curTick);
	}

	// don't save more than 30 secs of data
	while(avarage_tick_list.GetCount() > 0)
		if ((curTick - avarage_tick_list.GetHead()) > 30000) {
			m_avarage_dr_sum -= avarage_dr_list.RemoveHead();
			avarage_friend_dr_list.RemoveHead();
			avarage_tick_list.RemoveHead();
		}else
			break;
	//MORPH END  - Changed by SiRoB, Better datarate mesurement for low and high speed
};

bool CUploadQueue::AcceptNewClient(){
	uint32 curUploadSlots = (uint32)uploadinglist.GetCount();
    return AcceptNewClient(curUploadSlots);
}

bool CUploadQueue::AcceptNewClient(uint32 curUploadSlots){
// check if we can allow a new client to start downloading from us

	if (curUploadSlots < MIN_UP_CLIENTS_ALLOWED)
		return true;

    uint32 wantedNumberOfTrickles = GetWantedNumberOfTrickleUploads();
    if(curUploadSlots > m_MaxActiveClients+wantedNumberOfTrickles) {
        return false;
    }

	uint16 MaxSpeed;

    if (thePrefs.IsDynUpEnabled())
        MaxSpeed = theApp.lastCommonRouteFinder->GetUpload()/1024;        
    else
		MaxSpeed = thePrefs.GetMaxUpload();
			// add to queue again.
	if (curUploadSlots >= 4 &&
        (
         curUploadSlots >= (datarate/UPLOAD_CHECK_CLIENT_DR) ||
         curUploadSlots >= ((uint32)MaxSpeed)*1024/UPLOAD_CLIENT_DATARATE ||
         (
          thePrefs.GetMaxUpload() == UNLIMITED &&
          !thePrefs.IsDynUpEnabled() &&
          thePrefs.GetMaxGraphUploadRate() > 0 &&
          curUploadSlots >= ((uint32)thePrefs.GetMaxGraphUploadRate())*1024/UPLOAD_CLIENT_DATARATE
         )
        )
    ) // max number of clients to allow for all circumstances
	    return false;

	return true;
}

bool CUploadQueue::ForceNewClient(bool allowEmptyWaitingQueue) {
    if(!allowEmptyWaitingQueue && waitinglist.GetSize() <= 0)
        return false;

	if (::GetTickCount() - m_nLastStartUpload < 1000 && datarate < 102400 )
		return false;

	uint32 curUploadSlots = (uint32)uploadinglist.GetCount();

	if (curUploadSlots < MIN_UP_CLIENTS_ALLOWED)
		return true;

    if(!AcceptNewClient(curUploadSlots) || !theApp.lastCommonRouteFinder->AcceptNewClient()) { // UploadSpeedSense can veto a new slot if USS enabled
		return false;
    }

	uint16 MaxSpeed;

    if (thePrefs.IsDynUpEnabled())
        MaxSpeed = theApp.lastCommonRouteFinder->GetUpload()/1024;        
    else
		MaxSpeed = thePrefs.GetMaxUpload();

	uint32 upPerClient = UPLOAD_CLIENT_DATARATE;

    // if throttler doesn't require another slot, go with a slightly more restrictive method
	if( MaxSpeed > 20 || MaxSpeed == UNLIMITED)
		upPerClient += datarate/43;

	if( upPerClient > 7680 )
		upPerClient = 7680;

	//now the final check

	if ( MaxSpeed == UNLIMITED )
	{
		if (curUploadSlots < (datarate/upPerClient))
		return true;
	}
	else{
		uint16 nMaxSlots;
		if (MaxSpeed > 12)
			nMaxSlots = (uint16)(((float)(MaxSpeed*1024)) / upPerClient);
		else if (MaxSpeed > 7)
			nMaxSlots = MIN_UP_CLIENTS_ALLOWED + 2;
		else if (MaxSpeed > 3)
			nMaxSlots = MIN_UP_CLIENTS_ALLOWED + 1;
		else
			nMaxSlots = MIN_UP_CLIENTS_ALLOWED;

	//now the final check
		if ( curUploadSlots < nMaxSlots )
		{
			return true;
		}
	}

    uint32 wantedNumberOfTrickles = GetWantedNumberOfTrickleUploads();
    if(m_iHighestNumberOfFullyActivatedSlotsSinceLastCall + wantedNumberOfTrickles > (uint32)uploadinglist.GetSize()) {
        // uploadThrottler requests another slot. If throttler says it needs another slot, we will allow more slots
        // than what we require ourself. Never allow more slots than to give each slot high enough average transfer speed, though (checked above).
        if(thePrefs.GetLogUlDlEvents() && waitinglist.GetSize() > 0)
            AddDebugLogLine(false, _T("UploadQueue: Added new slot since throttler needs it. m_iHighestNumberOfFullyActivatedSlotsSinceLastCall: %i uploadinglist.GetSize(): %i tick: %i"), m_iHighestNumberOfFullyActivatedSlotsSinceLastCall, uploadinglist.GetSize(), ::GetTickCount());
			return true;
    }

    //nope
	return false;
}

CUploadQueue::~CUploadQueue(){
	if (h_timer)
		KillTimer(0,h_timer);
}

CUpDownClient* CUploadQueue::GetWaitingClientByIP_UDP(uint32 dwIP, uint16 nUDPPort){
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;){
		CUpDownClient* cur_client = waitinglist.GetNext(pos);
		if (dwIP == cur_client->GetIP() && nUDPPort == cur_client->GetUDPPort())
			return cur_client;
	}
	return 0;
}

CUpDownClient* CUploadQueue::GetWaitingClientByIP(uint32 dwIP){
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;){
		CUpDownClient* cur_client = waitinglist.GetNext(pos);
		if (dwIP == cur_client->GetIP())
			return cur_client;
	}
	return 0;
}

/**
* Add a client to the waiting queue for uploads.
*
* @param client address of the client that should be added to the waiting queue
*
* @param bIgnoreTimelimit don't check timelimit to possibly ban the client.
 *
 * @param addInFirstPlace the client should be added first in queue, not last
*/
void CUploadQueue::AddClientToQueue(CUpDownClient* client, bool bIgnoreTimelimit, bool addInFirstPlace)
{
    if(addInFirstPlace == false) {
		if (theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID() //This may need to be changed with the Kad now being used.
			&& !theApp.serverconnect->IsLocalServer(client->GetServerIP(),client->GetServerPort())
			&& client->GetDownloadState() == DS_NONE && !client->IsFriend()
			&& GetWaitingUserCount() > 50)
			return;
		client->AddAskedCount();
		client->SetLastUpRequest();
		if (!bIgnoreTimelimit)
		{
			client->AddRequestCount(client->GetUploadFileID());
		}
	    if (client->IsBanned())
		return;
    }
	uint16 cSameIP = 0;
	// check for double
	POSITION pos1, pos2;
	for (pos1 = waitinglist.GetHeadPosition();( pos2 = pos1 ) != NULL;)
	{
		waitinglist.GetNext(pos1);
		CUpDownClient* cur_client = waitinglist.GetAt(pos2);
		if (cur_client == client)
		{	
			//already on queue
            // VQB LowID Slot Patch, enhanced in ZZUL
            if (addInFirstPlace == false && client->HasLowID() &&
                client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick && AcceptNewClient())
			{
                    if(thePrefs.GetLogUlDlEvents())
                        AddDebugLogLine(true, _T("Adding ****lowid when reconneting. Client: %s"), client->DbgGetClientInfo());
					RemoveFromWaitingQueue(client, true);
					AddUpNextClient(client);
                //client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick = 0;
					return;
			}
			// VQB end
			client->SendRankingInfo();
			theApp.emuledlg->transferwnd->queuelistctrl.RefreshClient(client);
			return;			
		}
		else if ( client->Compare(cur_client) )
		{
			theApp.clientlist->AddTrackClient(client); // in any case keep track of this client

			// another client with same ip:port or hash
			// this happens only in rare cases, because same userhash / ip:ports are assigned to the right client on connecting in most cases
			if (cur_client->credits != NULL && cur_client->credits->GetCurrentIdentState(cur_client->GetIP()) == IS_IDENTIFIED)
			{
				//cur_client has a valid secure hash, don't remove him
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false,CString(GetResString(IDS_SAMEUSERHASH)),client->GetUserName(),cur_client->GetUserName(),client->GetUserName() );
				return;
			}
			if (client->credits != NULL && client->credits->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED)
			{
				//client has a valid secure hash, add him remove other one
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false,CString(GetResString(IDS_SAMEUSERHASH)),client->GetUserName(),cur_client->GetUserName(),cur_client->GetUserName() );
				// EastShare - Added by TAHO, modified SUQWT
				waitinglist.GetAt(pos2)->ClearWaitStartTime();
				// EastShare - Added by TAHO, modified SUQWT
				RemoveFromWaitingQueue(pos2,true);
				if (!cur_client->socket)
				{
					if(cur_client->Disconnected(_T("AddClientToQueue - same userhash 1")))
					{
						delete cur_client;
					}
				}
			}
			else
			{
				// remove both since we dont know who the bad on is
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false,CString(GetResString(IDS_SAMEUSERHASH)),client->GetUserName(),cur_client->GetUserName(),"Both" );
				// EastShare - Added by TAHO, modified SUQWT
				waitinglist.GetAt(pos2)->ClearWaitStartTime(); 
				// EastShare - Added by TAHO, modified SUQWT
				RemoveFromWaitingQueue(pos2,true);
				if (!cur_client->socket)
				{
					if(cur_client->Disconnected(_T("AddClientToQueue - same userhash 2")))
					{
						delete cur_client;
					}
				}
				return;
			}
		}
		else if (client->GetIP() == cur_client->GetIP())
		{
			// same IP, different port, different userhash
			cSameIP++;
		}
	}
	if (cSameIP >= 3)
	{
		// do not accept more than 3 clients from the same IP
		if (thePrefs.GetVerbose())
			DEBUG_ONLY( AddDebugLogLine(false,_T("%s's (%s) request to enter the queue was rejected, because of too many clients with the same IP"), client->GetUserName(), ipstr(client->GetConnectIP())) );
		return;
	}
	else if (theApp.clientlist->GetClientsFromIP(client->GetIP()) >= 3)
	{
		if (thePrefs.GetVerbose())
			DEBUG_ONLY( AddDebugLogLine(false,_T("%s's (%s) request to enter the queue was rejected, because of too many clients with the same IP (found in TrackedClientsList)"), client->GetUserName(), ipstr(client->GetConnectIP())) );
		return;
	}
	// done

	// Add clients server to list.
	if (thePrefs.AddServersFromClient() && client->GetServerIP() && client->GetServerPort())
	{
		CServer* srv = new CServer(client->GetServerPort(), ipstr(client->GetServerIP()));
		srv->SetListName(srv->GetAddress());

		if (!theApp.emuledlg->serverwnd->serverlistctrl.AddServer(srv, true))
			delete srv;
	}

    if(addInFirstPlace == false) {
	    // statistic values
	CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)client->GetUploadFileID());
	if (reqfile)
		reqfile->statistic.AddRequest();

	//Morph Start - added by AndCycle, SLUGFILLER: infiniteQueue
	if(!thePrefs.IsInfiniteQueueEnabled()){
		// cap the list
    	// the queue limit in prefs is only a soft limit. Hard limit is 25% higher, to let in powershare clients and other
    	// high ranking clients after soft limit has been reached
    	uint32 softQueueLimit = thePrefs.GetQueueSize();
    	uint32 hardQueueLimit = thePrefs.GetQueueSize() + max(thePrefs.GetQueueSize()/4, 200);

        // if soft queue limit has been reached, only let in high ranking clients
		if ((uint32)waitinglist.GetCount() >= hardQueueLimit ||
			(uint32)waitinglist.GetCount() >= softQueueLimit && // soft queue limit is reached
			(client->IsFriend() && client->GetFriendSlot()) == false && // client is not a friend with friend slot
    	     client->IsPBForPS() == false && // client don't want powershared file
			(
				client->GetCombinedFilePrioAndCredit() < GetAverageCombinedFilePrioAndCredit() && !thePrefs.IsEqualChanceEnable() ||
				client->GetCombinedFilePrioAndCredit() > GetAverageCombinedFilePrioAndCredit() && thePrefs.IsEqualChanceEnable()//Morph - added by AndCycle, Equal Chance For Each File
			)// and client has lower credits/wants lower prio file than average client in queue
			) {
				// then block client from getting on queue
				return;
			}
	}
	//Morph End - added by AndCycle, SLUGFILLER: infiniteQueue
	if (client->IsDownloading())
	{
		// he's already downloading and wants probably only another file
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__AcceptUploadReq", client);
		Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
		theStats.AddUpDataOverheadFileRequest(packet->size);
		client->socket->SendPacket(packet,true);
		return;
	}
    
        client->ResetQueueSessionUp();
	}

	// EastShare - Added by TAHO, modified SUQWT
	client->Credits()->SetSecWaitStartTime();
	// EastShare - Added by TAHO, modified SUQWT
	waitinglist.AddTail(client);
	client->SetUploadState(US_ONUPLOADQUEUE);

    // Add client to waiting list. If addInFirstPlace is set, client should not have its waiting time resetted
	theApp.emuledlg->transferwnd->queuelistctrl.AddClient(client, (addInFirstPlace == false));
	theApp.emuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
	client->SendRankingInfo();

}

double CUploadQueue::GetAverageCombinedFilePrioAndCredit() {
	DWORD curTick = ::GetTickCount();

	if (curTick - m_dwLastCalculatedAverageCombinedFilePrioAndCredit > 5*1000) {
		m_dwLastCalculatedAverageCombinedFilePrioAndCredit = curTick;

		POSITION pos1, pos2;

		// TODO: is there a risk of overflow? I don't think so...
		//Morph - partial changed by AndCycle, Equal Chance For Each File - the equal chance have a risk of overflow ... so I use double
		double sum = 0;

		for (pos1 = waitinglist.GetHeadPosition();( pos2 = pos1 ) != NULL;){
			waitinglist.GetNext(pos1);
			CUpDownClient* cur_client =	waitinglist.GetAt(pos2);

			sum += cur_client->GetCombinedFilePrioAndCredit();
		}

		m_fAverageCombinedFilePrioAndCredit = sum/waitinglist.GetSize();
	}

	return m_fAverageCombinedFilePrioAndCredit;
}
// Moonlight: SUQWT: Reset wait time on session success, save it on failure.//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
bool CUploadQueue::RemoveFromUploadQueue(CUpDownClient* client, LPCTSTR pszReason, bool updatewindow, bool earlyabort){
    bool result = false;
    uint32 slotCounter = 1;
	for (POSITION pos = uploadinglist.GetHeadPosition();pos != 0;){
        POSITION curPos = pos;
        CUpDownClient* curClient = uploadinglist.GetNext(pos);
		if (client == curClient){
			if (updatewindow)
				theApp.emuledlg->transferwnd->uploadlistctrl.RemoveClient(client);
	
			if (thePrefs.GetLogUlDlEvents())
				AddDebugLogLine(DLP_VERYLOW, true,_T("---- %s: Removing client from upload list. Reason: %s ----"), client->DbgGetClientInfo(), pszReason==NULL ? _T("") : pszReason);
            client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick = 0;
            uploadinglist.RemoveAt(curPos);

            bool removed = theApp.uploadBandwidthThrottler->RemoveFromStandardList(client->socket);
            bool pcRemoved = theApp.uploadBandwidthThrottler->RemoveFromStandardList((CClientReqSocket*)client->m_pPCUpSocket);

            //if(thePrefs.GetLogUlDlEvents() && !(removed || pcRemoved)) {
            //    AddDebugLogLine(false, _T("UploadQueue: Didn't find socket to delete. Adress: 0x%x"), client->socket);
            //}
			//EastShare Start - added by AndCycle, Pay Back First
			//client normal leave the upload queue, check does client still satisfy requirement
			if(earlyabort == false){
				client->credits->InitPayBackFirstStatus();
			}
			//EastShare End - added by AndCycle, Pay Back First

			if(client->GetQueueSessionUp() > 0){
				++successfullupcount;
				theStats.IncTotalCompletedBytes(client->GetQueueSessionUp());
				if(client->GetSessionUp() > 0) {
					//wistily
					uint32 tempUpStartTimeDelay=client->GetUpStartTimeDelay();
					client->Add2UpTotalTime(tempUpStartTimeDelay);
					totaluploadtime += tempUpStartTimeDelay/1000;
					/*
					totaluploadtime += client->GetUpStartTimeDelay()/1000;
					*/
					//wistily stop
				}
			} else if(earlyabort == false)
				++failedupcount;


			CKnownFile* requestedFile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());

		    if(requestedFile != NULL) {
		        requestedFile->UpdatePartsInfo();
		    }

			theApp.clientlist->AddTrackClient(client); // Keep track of this client
			client->SetUploadState(US_NONE);
			client->ClearUploadBlockRequests();
	
            m_iHighestNumberOfFullyActivatedSlotsSinceLastCall = 0;

			//MORPH START - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
			// EastShare START - Marked by TAHO, modified SUQWT
			if(earlyabort == true)
			{
				//client->Credits()->SaveUploadQueueWaitTime();
			}
			else if(client->GetQueueSessionUp() < SESSIONMAXTRANS)
			{
				int keeppct = (100 - (100 * client->GetQueueSessionUp()/SESSIONMAXTRANS)) - 10;// At least 10% time credit 'penalty'
				if (keeppct < 0)    keeppct = 0;
				client->Credits()->SaveUploadQueueWaitTime(keeppct);
				client->Credits()->SetSecWaitStartTime(); // EastShare - Added by TAHO, modified SUQWT
			}
			else
			{
				client->Credits()->ClearUploadQueueWaitTime();	// Moonlight: SUQWT
				client->Credits()->ClearWaitStartTime(); // EastShare - Added by TAHO, modified SUQWT
			}
			// EastShare END - Marked by TAHO, modified SUQWT
			//MORPH END   - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
			result = true;
        } else {
            curClient->SetSlotNumber(slotCounter);
            slotCounter++;
        }
	}
	return result;
}

uint32 CUploadQueue::GetAverageUpTime(){
	if( successfullupcount ){
		return totaluploadtime/successfullupcount;
	}
	return 0;
}

bool CUploadQueue::RemoveFromWaitingQueue(CUpDownClient* client, bool updatewindow){
	POSITION pos = waitinglist.Find(client);
	if (pos){
		RemoveFromWaitingQueue(pos,updatewindow);
		if (updatewindow)
			theApp.emuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
		return true;
	}
	else
		return false;
}

void CUploadQueue::RemoveFromWaitingQueue(POSITION pos, bool updatewindow){	
	CUpDownClient* todelete = waitinglist.GetAt(pos);
	waitinglist.RemoveAt(pos);
	if (updatewindow)
		theApp.emuledlg->transferwnd->queuelistctrl.RemoveClient(todelete);
	//MORPH START - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	if (theApp.clientcredits->IsSaveUploadQueueWaitTime()){
		todelete->Credits()->SaveUploadQueueWaitTime();	// Moonlight: SUQWT
		// EastShare START - Marked by TAHO, modified SUQWT
		//todelete->Credits()->ClearWaitStartTime();		// Moonlight: SUQWT 
		// EastShare END - Marked by TAHO, modified SUQWT
	}
	//MORPH END   - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	todelete->SetUploadState(US_NONE);
}

void CUploadQueue::UpdateMaxClientScore()
{
	m_imaxscore=0;
	for(POSITION pos = waitinglist.GetHeadPosition(); pos != 0; ) {
		uint32 score = waitinglist.GetNext(pos)->GetScore(true, false);
		if(score > m_imaxscore )
			m_imaxscore=score;
	}
}

void CUploadQueue::DeleteAll(){
	waitinglist.RemoveAll();
	uploadinglist.RemoveAll();
    // PENDING: Remove from UploadBandwidthThrottler as well!
}

uint16 CUploadQueue::GetWaitingPosition(CUpDownClient* client)
{
	if (!IsOnUploadQueue(client))
		return 0;
	UINT rank = 1;
	uint32 myscore = client->GetScore(false);
	for (POSITION pos = waitinglist.GetHeadPosition(); pos != 0; ){
		//MORPH START - Added by SiRoB, ZZ Upload System
		/*
		if (waitinglist.GetNext(pos)->GetScore(false) > myscore)
		*/
		CUpDownClient* compareClient = waitinglist.GetNext(pos);
		if (RightClientIsBetter(client, myscore, compareClient, compareClient->GetScore(false)))
		//MORPH END - Added by SiRoB, ZZ Upload System
			rank++;
	}
	return rank;
}

VOID CALLBACK CUploadQueue::UploadTimer(HWND hwnd, UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	try
	{
		// Barry - Don't do anything if the app is shutting down - can cause unhandled exceptions
		if (!theApp.emuledlg->IsRunning())
			return;

        // Elandal:ThreadSafeLogging -->
        // other threads may have queued up log lines. This prints them.
		theApp.HandleDebugLogQueue();
        theApp.HandleLogQueue();
        // Elandal: ThreadSafeLogging <--

		// Send allowed data rate to UploadBandWidthThrottler in a thread safe way
		//MOPRH START - Modified by SiRoB
		/*
		theApp.uploadBandwidthThrottler->SetAllowedDataRate(thePrefs.GetMaxUpload()*1024);
		// ZZ:UploadSpeedSense -->
		theApp.lastCommonRouteFinder->SetPrefs(thePrefs.IsDynUpEnabled(), theApp.uploadqueue->GetDatarate(), thePrefs.GetMinUpload()*1024, (thePrefs.GetMaxUpload() != 0)?thePrefs.GetMaxUpload()*1024:thePrefs.GetMaxGraphUploadRate()*1024, thePrefs.IsDynUpUseMillisecondPingTolerance(), (thePrefs.GetDynUpPingTolerance() > 100)?((thePrefs.GetDynUpPingTolerance()-100)/100.0f):0, thePrefs.GetDynUpPingToleranceMilliseconds(), thePrefs.GetDynUpGoingUpDivider(), thePrefs.GetDynUpGoingDownDivider(), thePrefs.GetDynUpNumberOfPings(), 20); // PENDING: Hard coded min pLowestPingAllowed
		*/
		theApp.lastCommonRouteFinder->SetPrefs(thePrefs.IsDynUpEnabled(), theApp.uploadqueue->GetDatarate(), thePrefs.GetMinUpload()*1024, (thePrefs.IsSUCDoesWork())?theApp.uploadqueue->GetMaxVUR():(thePrefs.GetMaxUpload() != 0)?thePrefs.GetMaxUpload()*1024:thePrefs.GetMaxGraphUploadRate()*1024, thePrefs.IsDynUpUseMillisecondPingTolerance(), (thePrefs.GetDynUpPingTolerance() > 100)?((thePrefs.GetDynUpPingTolerance()-100)/100.0f):0, thePrefs.GetDynUpPingToleranceMilliseconds(), thePrefs.GetDynUpGoingUpDivider(), thePrefs.GetDynUpGoingDownDivider(), thePrefs.GetDynUpNumberOfPings(), 20, thePrefs.IsUSSLog(),thePrefs.GetMaxFriendByteToSend());
		//MOPRH END   - Modified by SiRoB

		theApp.uploadqueue->Process();
		theApp.downloadqueue->Process();
		if (thePrefs.ShowOverhead()){
			theStats.CompUpDatarateOverhead();
			theStats.CompDownDatarateOverhead();
		}
		counter++;

		// one second
		if (counter >= 10){
			counter=0;

			// try to use different time intervals here to not create any disk-IO bottle necks by saving all files at once
			theApp.clientcredits->Process();	// 13 minutes
			theApp.serverlist->Process();		// 17 minutes
			theApp.knownfiles->Process();		// 11 minutes
			theApp.friendlist->Process();		// 19 minutes
			theApp.clientlist->Process();
			theApp.sharedfiles->Process();
			if( Kademlia::CKademlia::isRunning() )
			{
				ASSERT( Kademlia::CKademlia::getPrefs() != NULL);
				Kademlia::CKademlia::process();
				if(Kademlia::CKademlia::getPrefs()->hasLostConnection())
				{
					Kademlia::CKademlia::stop();
					theApp.emuledlg->ShowConnectionState();
				}
			}
			if( theApp.serverconnect->IsConnecting() && !theApp.serverconnect->IsSingleConnect() )
				theApp.serverconnect->TryAnotherConnectionrequest();

			theApp.listensocket->UpdateConnectionsStatus();
			if (thePrefs.WatchClipboard4ED2KLinks())
				theApp.SearchClipboard();		

			if (theApp.serverconnect->IsConnecting())
				theApp.serverconnect->CheckForTimeout();

			// -khaos--+++> Update connection stats...
			iupdateconnstats++;
			// 2 seconds
			if (iupdateconnstats>=2) {
				iupdateconnstats=0;
				theStats.UpdateConnectionStats((float)theApp.uploadqueue->GetDatarate()/1024, (float)theApp.downloadqueue->GetDatarate()/1024);
			}
			// <-----khaos-

			// display graphs
			if (thePrefs.GetTrafficOMeterInterval()>0) {
				igraph++;

				if (igraph >= (uint32)(thePrefs.GetTrafficOMeterInterval()) ) {
					igraph=0;
					//theApp.emuledlg->statisticswnd->SetCurrentRate((float)(theApp.uploadqueue->GetDatarate())/1024,(float)(theApp.downloadqueue->GetDatarate())/1024);
					theApp.emuledlg->statisticswnd->SetCurrentRate((float)(theApp.uploadqueue->GetDatarate())/1024,(float)(theApp.downloadqueue->GetDatarate())/1024);
					//theApp.uploadqueue->Zeroavg();
				}
			}
			if (theApp.emuledlg->activewnd == theApp.emuledlg->statisticswnd && theApp.emuledlg->IsWindowVisible() )  {
				// display stats
				if (thePrefs.GetStatsInterval()>0) {
					istats++;

					if (istats >= (uint32)(thePrefs.GetStatsInterval()) ) {
						istats=0;
						theApp.emuledlg->statisticswnd->ShowStatistics();
					}
				}
			}
			//save rates every second
			theStats.RecordRate();
			// mobilemule sockets
			theApp.mmserver->Process();

			//MORPH START - Added by SiRoB, ZZ Upload system (USS)
			theApp.emuledlg->ShowPing();

			bool gotEnoughHosts = theApp.clientlist->GiveClientsForTraceRoute();
			if(gotEnoughHosts == false) {
					theApp.serverlist->GiveServersForTraceRoute();
			}
			//MORPH END   - Added by SiRoB, ZZ Upload system (USS)

			sec++;
			// 5 seconds
			if (sec>=5) {

#ifdef _DEBUG
				if (thePrefs.m_iDbgHeap > 0 && !AfxCheckMemory())
					AfxDebugBreak();
#endif

				sec = 0;
				theApp.listensocket->Process();
				theApp.OnlineSig(); // Added By Bouc7 

                //Commander - Removed: Blinking Tray Icon On Message Recieve [emulEspaña] - Start
				// Update every second
				/*
				theApp.emuledlg->ShowTransferRate();
				*/
				//Commander - Removed: Blinking Tray Icon On Message Recieve [emulEspaña] - End
				
				if (!thePrefs.TransferFullChunks())
					theApp.uploadqueue->UpdateMaxClientScore();

				// update cat-titles with downloadinfos only when needed
				if (thePrefs.ShowCatTabInfos() && 
					theApp.emuledlg->activewnd == theApp.emuledlg->transferwnd && 
					theApp.emuledlg->IsWindowVisible()) 
						theApp.emuledlg->transferwnd->UpdateCatTabTitles(false);
				
				if (thePrefs.IsSchedulerEnabled())
					theApp.scheduler->Check();

                theApp.emuledlg->transferwnd->UpdateListCount(1, -1);
			}

			//Commander - Moved: Blinking Tray Icon On Message Recieve [emulEspaña] - Start
			// Update every second
			theApp.emuledlg->ShowTransferRate();
			//Commander - Moved: Blinking Tray Icon On Message Recieve [emulEspaña] - End

			statsave++;
			// 60 seconds
			if (statsave>=60) {
				statsave=0;

				if (thePrefs.GetWSIsEnabled())
					theApp.webserver->UpdateSessionCount();

				theApp.serverconnect->KeepConnectionAlive();
			}

			_uSaveStatistics++;
			if (_uSaveStatistics >= thePrefs.GetStatsSaveInterval())
			{
				_uSaveStatistics = 0;
				thePrefs.SaveStats();
			}
		}

		// need more accuracy here. don't rely on the 'sec' and 'statsave' helpers.
		thePerfLog.LogSamples();
	}
	CATCH_DFLT_EXCEPTIONS(_T("CUploadQueue::UploadTimer"))
}

CUpDownClient* CUploadQueue::GetNextClient(const CUpDownClient* lastclient){
	if (waitinglist.IsEmpty())
		return 0;
	if (!lastclient)
		return waitinglist.GetHead();
	POSITION pos = waitinglist.Find(const_cast<CUpDownClient*>(lastclient));
	if (!pos){
		TRACE("Error: CUploadQueue::GetNextClient");
		return waitinglist.GetHead();
	}
	waitinglist.GetNext(pos);
	if (!pos)
		return NULL;
	else
		return waitinglist.GetAt(pos);
}

/*void CUploadQueue::FindSourcesForFileById(CUpDownClientPtrList* srclist, const uchar* filehash) {
	POSITION pos;
	
	pos = uploadinglist.GetHeadPosition();
	while(pos) 
	{
		CUpDownClient *potential = uploadinglist.GetNext(pos);
		if(md4cmp(potential->GetUploadFileID(), filehash) == 0)
			srclist->AddTail(potential);
	}

	pos = waitinglist.GetHeadPosition();
	while(pos) 
	{
		CUpDownClient *potential = waitinglist.GetNext(pos);
		if(md4cmp(potential->GetUploadFileID(), filehash) == 0)
			srclist->AddTail(potential);
	}
}
*/

//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
void CUploadQueue::UpdateDatarates() {
	// Calculate average datarate
	if(::GetTickCount()-m_lastCalculatedDataRateTick > 500) {
		m_lastCalculatedDataRateTick = ::GetTickCount();

		//MORPH START - Changed by SiRoB, Better datarate mesurement for low and high speed
		/*
		if(avarage_dr_list.GetSize() >= 2 && (avarage_tick_list.GetTail() > avarage_tick_list.GetHead())) {
			datarate = ((m_avarage_dr_sum-avarage_dr_list.GetHead())*1000) / (avarage_tick_list.GetTail()-avarage_tick_list.GetHead());
			friendDatarate = ((avarage_friend_dr_list.GetTail()-avarage_friend_dr_list.GetHead())*1000) / (avarage_tick_list.GetTail()-avarage_tick_list.GetHead());

		}
		*/
		if (avarage_tick_list.GetCount() > 0){
			if (avarage_tick_list.GetCount() == 1){
				datarate = (m_avarage_dr_sum*1000) / 30000;
				friendDatarate = 0;
			}
			else {
				DWORD dwDuration = avarage_tick_list.GetTail() - avarage_tick_list.GetHead();
				if ((avarage_tick_list.GetCount() - 1) * (m_lastCalculatedDataRateTick - avarage_tick_list.GetTail()) > dwDuration)
					dwDuration = m_lastCalculatedDataRateTick - avarage_tick_list.GetHead() - dwDuration / (avarage_tick_list.GetCount()-1);
				if (dwDuration < 5000) dwDuration = 5000;
				datarate = ((m_avarage_dr_sum-avarage_dr_list.GetHead())*1000) / dwDuration;
				friendDatarate = ((avarage_friend_dr_list.GetTail()-avarage_friend_dr_list.GetHead())*1000) / dwDuration;
			}
		}else {
			datarate = 0;
			friendDatarate = 0;
		}
		//MORPH END   - Changed by SiRoB, Better datarate mesurement for low and high speed
	}
}

uint32 CUploadQueue::GetDatarate() {
	UpdateDatarates();
	return datarate;
}

uint32 CUploadQueue::GetToNetworkDatarate() {
	UpdateDatarates();
	if(datarate > friendDatarate) {
		return datarate - friendDatarate;
	} else {
		return 0;
	}
}

uint32 CUploadQueue::GetWantedNumberOfTrickleUploads() {
    uint32 minNumber = MINNUMBEROFTRICKLEUPLOADS;

    //if(minNumber < 2 && thePrefs.GetMaxUpload() >= 4) {
    //    minNumber = 2;
    //} else
    if(minNumber < 1 && GetDatarate() >= 2*1024 /*thePrefs.GetMaxUpload() >= 2*/) {
        minNumber = 1;
    }

    return max(((uint32)uploadinglist.GetCount())*0.2, minNumber);
}

/**
 * Resort the upload slots, so they are kept sorted even if file priorities
 * are changed by the user, friend slot is turned on/off, etc
 */
void CUploadQueue::ReSortUploadSlots(bool force) {
    DWORD curtick = ::GetTickCount();
    if(force ||  curtick - m_dwLastResortedUploadSlots >= 10*1000) {
    	m_dwLastResortedUploadSlots = curtick;

		theApp.uploadBandwidthThrottler->Pause(true);

    	CTypedPtrList<CPtrList, CUpDownClient*> tempUploadinglist;

    	// Remove all clients from uploading list and store in tempList
    	POSITION ulpos = uploadinglist.GetHeadPosition();
    	while (ulpos != NULL) {
    	    POSITION curpos = ulpos;
    	    uploadinglist.GetNext(ulpos);

    	    // Get and remove the client from upload list.
			CUpDownClient* cur_client = uploadinglist.GetAt(curpos);

    	    uploadinglist.RemoveAt(curpos);

    	    // Remove the found Client from UploadBandwidthThrottler
    	    theApp.uploadBandwidthThrottler->RemoveFromStandardList(cur_client->socket);
            theApp.uploadBandwidthThrottler->RemoveFromStandardList((CClientReqSocket*)cur_client->m_pPCUpSocket);

    	    tempUploadinglist.AddTail(cur_client);
    	}

		// Remove one at a time from temp list and reinsert in correct position in uploading list
    	POSITION tempPos = tempUploadinglist.GetHeadPosition();
    	while(tempPos != NULL) {
    	    POSITION curpos = tempPos;
    	    tempUploadinglist.GetNext(tempPos);

    	    // Get and remove the client from upload list.
			CUpDownClient* cur_client = tempUploadinglist.GetAt(curpos);

    	    tempUploadinglist.RemoveAt(curpos);

    	    // This will insert in correct place
    	    InsertInUploadingList(cur_client);
		}

		theApp.uploadBandwidthThrottler->Pause(false);
	}
}

void CUploadQueue::CheckForHighPrioClient() {
    // PENDING: Each 3 seconds
    DWORD curTick = ::GetTickCount();
    if(curTick - m_dwLastCheckedForHighPrioClient >= 3*1000) {
        m_dwLastCheckedForHighPrioClient = curTick;

        bool added = true;
        while(added) {
            added = AddUpNextClient(NULL, true);
        }
	}
}

//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
uint32	CUploadQueue::GetMaxVUR()
{
	return min(max(MaxVUR,(uint32)1024*thePrefs.GetMinUpload()),(uint32)1024*thePrefs.GetMaxUpload());
}
//MORPH END   - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]