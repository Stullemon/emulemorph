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
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


static uint32 counter, sec,statsave;
static UINT _uSaveStatistics = 0;
// -khaos--+++> Added iupdateconnstats...
static uint32 igraph, istats, iupdateconnstats;
// <-----khaos-

//TODO rewrite the whole networkcode, use overlapped sockets.. sure....

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
	//Removed By SiRoB, Not used due to zz Upload System
	/*
	m_dwRemovedClientByScore = ::GetTickCount();
	*/
    m_iHighestNumberOfFullyActivatedSlotsSinceLastCall = 0;
	//MORPH START - Added by SiRoB, Upload Splitting Class
	memset(m_iHighestNumberOfFullyActivatedSlotsSinceLastCallClass,0,sizeof(m_iHighestNumberOfFullyActivatedSlotsSinceLastCallClass));
	memset(m_aiSlotCounter,0,sizeof(m_aiSlotCounter));
	memset(m_abOnClientOverHideClientDatarate,0,sizeof(m_abOnClientOverHideClientDatarate));
	//MORPH END  - Added by SiRoB, Upload Splitting Class
	m_MaxActiveClients = 0;
	m_MaxActiveClientsShortTime = 0;

	//MORPH - Removed By SiRoB, not needed call UpdateDatarate only once in the process
	/*
	m_lastCalculatedDataRateTick = 0;
	*/
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

    //MORPH START - Changed by SiRoB, Upload Splitting Class
	/*
	CUpDownClient* newclient = FindBestClientInQueue(true, client);
	*/
	CUpDownClient* newclient = FindBestClientInQueue(true, client, onlyCheckForRemove);
	//MORPH END   - Changed by SiRoB, Upload Splitting Class
	
    if(newclient != NULL && // Only remove the client if there's someone to replace it
		RightClientIsSuperior(client, newclient) >= 0
      ){

        // Remove client from ul list to make room for higher/same prio client
        //AddDebugLogLine(false, GetResString(IDS_ULSUCCESSFUL), client->GetUserName(), CastItoXBytes(client->GetQueueSessionPayloadUp()), CastItoXBytes(client->GetCurrentSessionLimit()), (sint32)client->GetQueueSessionPayloadUp()-client->GetCurrentSessionLimit());

        //client->SetWaitStartTime();
	    ScheduleRemovalFromUploadQueue(client, _T("Successful completion of upload."), GetResString(IDS_UPLOAD_COMPLETED));
		//theApp.uploadqueue->AddClientToQueue(client,true);

        return true;
    } else if(onlyCheckForRemove == false) {
    	MoveDownInUploadQueue(client);

        return false;
    } else {
        return false;
    }
}

void CUploadQueue::MoveDownInUploadQueue(CUpDownClient* client) {
    // first find the client in the uploadinglist
    POSITION foundPos = uploadinglist.Find(client);
	if(foundPos != NULL) {
        //MORPH START - Added by SiRoB, Renumber slot -Fix-
		POSITION renumberPosition = uploadinglist.GetTailPosition();
		while(renumberPosition != foundPos) {
			CUpDownClient* renumberClient = uploadinglist.GetAt(renumberPosition);
			renumberClient->SetSlotNumber(renumberClient->GetSlotNumber()-1);
			uploadinglist.GetPrev(renumberPosition);
		}
		//MORPH END   - Added by SiRoB, Renumber slot -Fix-
			
		// Remove the found Client
		uploadinglist.RemoveAt(foundPos);
            
		theApp.uploadBandwidthThrottler->RemoveFromStandardList(client->socket,true);
		//MORPH START - Added by SiRoB, due to zz upload system PeerCache
		theApp.uploadBandwidthThrottler->RemoveFromStandardList((CClientReqSocket*)client->m_pPCUpSocket,true);
		//MORPH END   - Added by SiRoB, due to zz upload system PeerCache
    	//MORPH START - Added by SiRoB, due to zz upload system WebCache
		theApp.uploadBandwidthThrottler->RemoveFromStandardList((CClientReqSocket*)client->m_pWCUpSocket,true);
		//MORPH END   - Added by SiRoB, due to zz upload system WebCache
    	   
        // then add it last in it's class
        InsertInUploadingList(client);
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
bool CUploadQueue::RightClientIsBetter(CUpDownClient* leftClient, uint32 leftScore, CUpDownClient* rightClient, uint32 rightScore, bool checkforaddinuploadinglist) { //MORPH - Changed by SiRoB, Upload Splitting Class
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
				(iSuperior = RightClientIsSuperior(leftClient, rightClient)) > 0 || //MORPH - Changed by SiRoB, Upload Splitting Class
				iSuperior == 0 &&
				(//Morph - added by AndCycle, Equal Chance For Each File
					leftClient->GetEqualChanceValue() > rightClient->GetEqualChanceValue() ||	//rightClient want a file have less chance been uploaded
					leftClient->GetEqualChanceValue() == rightClient->GetEqualChanceValue() &&
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
			) ||
			leftClient == NULL  // there's no old client to compare with, so rightClient is better (than null)
		) &&
		//MORPH START - Changed by SiRoB, Code Optimization
		/*
		(!rightClient->IsBanned()) && // don't allow banned client to be best
		*/
		(rightClient->GetUploadState() != US_BANNED) && // don't allow banned client to be best
		//MORPH END   - Changed by SiRoB, Code Optimization
		IsDownloading(rightClient) == false // don't allow downloading clients to be best
		//MORPH START - Added by SiRoB, Upload Splitting Class
		&&
		(!checkforaddinuploadinglist ||
		 m_abAddClientOfThisClass[LAST_CLASS] && !(rightClient->IsFriend() && rightClient->GetFriendSlot()) && !rightClient->IsPBForPS() ||
		 m_abAddClientOfThisClass[1] && rightClient->IsPBForPS() ||
		 m_abAddClientOfThisClass[0] && rightClient->IsFriend() && rightClient->GetFriendSlot()
		)
		//MORPH END  - Added by SiRoB, Upload Splitting Class
	) {
		return true;
	} else {
		return false;
	}

}

//Morph Start - added by AndCycle, separate special prio compare
int CUploadQueue::RightClientIsSuperior(CUpDownClient* leftClient, CUpDownClient* rightClient)
{
	//MORPH - Removed by SiRoB, After checking the code seems to be not needed
	/*
	if(leftClient == NULL){
		return 1;
	}
	if(rightClient == NULL){
		return -1;
	}
	*/

	//MORPH START - Changed by SiRoB, Code Optimization
	/*
	if((leftClient->IsFriend() && leftClient->GetFriendSlot()) == false && (rightClient->IsFriend() && rightClient->GetFriendSlot()) == true){
		return 1;
	}
	if((leftClient->IsFriend() && leftClient->GetFriendSlot()) == true && (rightClient->IsFriend() && rightClient->GetFriendSlot()) == false){
		return -1;
	}
	if(leftClient->IsPBForPS() == false && rightClient->IsPBForPS() == true){
		return 1;
	}
	else if(leftClient->IsPBForPS() == true && rightClient->IsPBForPS() == false){
		return -1;
	}

	//Morph - added by AndCyle, selective PS internal Prio
	if(thePrefs.IsPSinternalPrioEnable() && leftClient->IsPBForPS() == true && rightClient->IsPBForPS() == true){
		if(leftClient->GetFilePrioAsNumber() < rightClient->GetFilePrioAsNumber()){
			return 1;
		}
		if(leftClient->GetFilePrioAsNumber() > rightClient->GetFilePrioAsNumber()){
			return -1;
		}
	}
	return 0;
	*/
	int retvalue = 0;
	if(leftClient->IsFriend() && leftClient->GetFriendSlot()) --retvalue;
	if(rightClient->IsFriend() && rightClient->GetFriendSlot()) ++retvalue;
	if(retvalue==0)
	{
		if (leftClient->IsPBForPS()) --retvalue;
		if (rightClient->IsPBForPS()){
			++retvalue;
			//Morph - added by AndCyle, selective PS internal Prio
			if(retvalue == 0 && thePrefs.IsPSinternalPrioEnable())
				retvalue = rightClient->GetFilePrioAsNumber() - leftClient->GetFilePrioAsNumber();
			//Morph - added by AndCyle, selective PS internal Prio
		}
	}
	return retvalue;
	//MORPH END   - Changed by SiRoB, Code Optimization
}
//Morph End - added by AndCycle, separate special prio compare

/**
* Find the highest ranking client in the waiting queue, and return it.
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
CUpDownClient* CUploadQueue::FindBestClientInQueue(bool allowLowIdAddNextConnectToBeSet, CUpDownClient* lowIdClientMustBeInSameOrBetterClassAsThisClient, bool checkforaddinuploadinglist) //MORPH - Changed by SiRoB, Upload Splitting Class
{
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
		//While we are going through this list.. Lets check if a client appears to have left the network..
		ASSERT ( cur_client->GetLastUpRequest() );
		if ((::GetTickCount() - cur_client->GetLastUpRequest() > MAX_PURGEQUEUETIME) || !theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID()))
		{
			//This client has either not been seen in a long time, or we no longer share the file he wanted anymore..
			cur_client->ClearWaitStartTime();
			RemoveFromWaitingQueue(pos2,true);
			continue;
		}
		else
		{
		    // finished clearing
			uint32 cur_score = cur_client->GetScore(false);

			if (RightClientIsBetter(newclient, bestscore, cur_client, cur_score, checkforaddinuploadinglist)) //MORPH - Changed by SiRoB, Upload Splitting Class
			{
                // cur_client is more worthy than current best client that is ready to go (connected).
				if(!cur_client->HasLowID() || (cur_client->socket && cur_client->socket->IsConnected())) {
                    // this client is a HighID or a lowID client that is ready to go (connected)
                    // and it is more worthy
					bestscore = cur_score;
					toadd = pos2;
                    newclient = waitinglist.GetAt(toadd);
                }
				else if(allowLowIdAddNextConnectToBeSet && !cur_client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick)
				{
                    // this client is a lowID client that is not ready to go (not connected)
    
                    // now that we know this client is not ready to go, compare it to the best not ready client
                    // the best not ready client may be better than the best ready client, so we need to check
                    // against that client
					if (RightClientIsBetter(lowclient, bestlowscore, cur_client, cur_score, checkforaddinuploadinglist)) //MORPH - Changed by SiRoB, Upload Splitting Class
					{
                        // it is more worthy, keep it
						bestlowscore = cur_score;
						toaddlow = pos2;
                        lowclient = waitinglist.GetAt(toaddlow);
					}
				}
			}
		}
	}
		
	if (bestlowscore > bestscore && lowclient && allowLowIdAddNextConnectToBeSet)
	{
		if(lowIdClientMustBeInSameOrBetterClassAsThisClient == NULL ||
			lowIdClientMustBeInSameOrBetterClassAsThisClient->IsScheduledForRemoval() == true ||
			newclient != NULL && RightClientIsSuperior(lowIdClientMustBeInSameOrBetterClassAsThisClient, newclient) >= 0
		){
			DWORD connectTick = ::GetTickCount();
              if(connectTick == 0) connectTick = 1;
		      lowclient->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick = connectTick;
		}
	}

	if (!toadd)
		return NULL;
	else
		return waitinglist.GetAt(toadd);
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

	//uint32 newclientScore = newclient->GetScore(false);

	bool foundposition = false;
	POSITION pos = uploadinglist.GetTailPosition();
	while(pos != NULL && foundposition == false) {
		CUpDownClient* uploadingClient = uploadinglist.GetAt(pos);

		if(uploadingClient->IsScheduledForRemoval() == false && newclient->IsScheduledForRemoval() == true ||
    	   uploadingClient->IsScheduledForRemoval() && uploadingClient->GetScheduledUploadShouldKeepWaitingTime() && newclient->IsScheduledForRemoval() && newclient->GetScheduledUploadShouldKeepWaitingTime() == false ||
		   uploadingClient->IsScheduledForRemoval() == newclient->IsScheduledForRemoval() &&
		   (!uploadingClient->IsScheduledForRemoval() && !newclient->IsScheduledForRemoval() || uploadingClient->GetScheduledUploadShouldKeepWaitingTime() == newclient->GetScheduledUploadShouldKeepWaitingTime()) &&
		   RightClientIsSuperior(newclient, uploadingClient) >= 0)
		{
			foundposition = true;
		} else {
			insertPosition = pos;
			uploadinglist.GetPrev(pos);
			posCounter--;
		}
	}
	
	//MORPH START - Added by SiRoB, Upload Splitting Class
	uint32 classID = LAST_CLASS;
	if (newclient->IsScheduledForRemoval())
		classID = SCHED_CLASS;
	else if (newclient->IsFriend() && newclient->GetFriendSlot())
		classID = 0;
	else if (newclient->IsPBForPS())
		classID = 1;
	newclient->SetClassID(classID);
	//MORPH END   - Added by SiRoB, Upload Splitting Class

	if(insertPosition != NULL) {
		POSITION renumberPosition = insertPosition;
		uint32 renumberSlotNumber = posCounter+1; //MORPH - Changed by SiRoB, Fix
	    
		while(renumberPosition != NULL) {
			CUpDownClient* renumberClient = uploadinglist.GetAt(renumberPosition);

			renumberClient->SetSlotNumber(++renumberSlotNumber);
			renumberClient->UpdateDisplayedInfo(true);
			uploadinglist.GetNext(renumberPosition);
		}

		// add it at found pos
		newclient->SetSlotNumber(posCounter+1);
		uploadinglist.InsertBefore(insertPosition, newclient);
		//MORPH START - Changed by SiRoB, Upload Splitting Class
		/*
		theApp.uploadBandwidthThrottler->AddToStandardList(posCounter, newclient->GetFileUploadSocket());
		*/
		theApp.uploadBandwidthThrottler->AddToStandardList(posCounter, newclient->GetFileUploadSocket(),classID);
		//MORPH END   - Changed by SiRoB, Upload Splitting Class
	}else{
		// Add it last
		//MORPH START - Changed by SiRoB, Upload Splitting Class
		/*
		theApp.uploadBandwidthThrottler->AddToStandardList(uploadinglist.GetCount(), newclient->GetFileUploadSocket());
		*/
		theApp.uploadBandwidthThrottler->AddToStandardList(uploadinglist.GetCount(), newclient->GetFileUploadSocket(),classID);
		//MORPH END   - Changed by SiRoB, Upload Splitting Class
		uploadinglist.AddTail(newclient);
		newclient->SetSlotNumber(uploadinglist.GetCount());
	}
}

//MORPH START - Added By AndCycle, ZZUL_20050212-0200
CUpDownClient* CUploadQueue::FindLastUnScheduledForRemovalClientInUploadList() {
	POSITION pos = uploadinglist.GetTailPosition();
	while(pos != NULL){
        // Get the client. Note! Also updates pos as a side effect.
		CUpDownClient* cur_client = uploadinglist.GetPrev(pos);

		if(!cur_client->IsScheduledForRemoval()) {
			return cur_client;
		}
	}

    return NULL;
}

CUpDownClient* CUploadQueue::FindBestScheduledForRemovalClientInUploadListThatCanBeReinstated() {
    POSITION pos = uploadinglist.GetHeadPosition();
	while(pos != NULL){
        // Get the client. Note! Also updates pos as a side effect.
		CUpDownClient* cur_client = uploadinglist.GetNext(pos);

        if(cur_client->IsScheduledForRemoval() && cur_client->GetScheduledUploadShouldKeepWaitingTime()) {
            return cur_client;
		}
	}

    return NULL;
}

uint32 CUploadQueue::GetEffectiveUploadListCount() {
    uint32 count = 0;

	POSITION pos = uploadinglist.GetTailPosition();
	while(pos != NULL){
        // Get the client. Note! Also updates pos as a side effect.
		CUpDownClient* cur_client = uploadinglist.GetPrev(pos);

		if(!cur_client->IsScheduledForRemoval()) {
			pos = NULL;
        } else {
            count++;
        }
	}

    return uploadinglist.GetCount()-count;
}
//MORPH END   - Added By AndCycle, ZZUL_20050212-0200

bool CUploadQueue::AddUpNextClient(LPCTSTR pszReason, CUpDownClient* directadd, bool highPrioCheck) {
	CUpDownClient* newclient = NULL;
	// select next client or use given client
	if (!directadd)
	{
        if(!highPrioCheck) {
            newclient = FindBestScheduledForRemovalClientInUploadListThatCanBeReinstated();
        }

        if(!newclient) {
            //MORPH START - Changed by SiRoB, Upload Splitting Class
			/*
			newclient = FindBestClientInQueue(highPrioCheck == false);
			*/
			newclient = FindBestClientInQueue(highPrioCheck == false,0,true);
			//MORPH END   - Changed by SiRoB, Upload Splitting Class
        }

		if(newclient) {
            if(highPrioCheck == true) {
                if(m_abAddClientOfThisClass[0] && newclient->IsFriend() && newclient->GetFriendSlot() || m_abAddClientOfThisClass[1] && newclient->GetPowerShared()) { //MORPH - Changed by SiRoB, Upload Splitting Class
		            POSITION lastpos = uploadinglist.GetTailPosition();

                    CUpDownClient* lastClient = FindLastUnScheduledForRemovalClientInUploadList();

					if(lastClient != NULL) {
						if (newclient->IsScheduledForRemoval() == false && lastClient->IsScheduledForRemoval() == true ||
  							newclient->IsScheduledForRemoval() && newclient->GetScheduledUploadShouldKeepWaitingTime() && lastClient->IsScheduledForRemoval() && !lastClient->GetScheduledUploadShouldKeepWaitingTime() ||
						    newclient->IsScheduledForRemoval() == lastClient->IsScheduledForRemoval() &&
                            (!newclient->IsScheduledForRemoval() && !lastClient->IsScheduledForRemoval() || newclient->GetScheduledUploadShouldKeepWaitingTime() == lastClient->GetScheduledUploadShouldKeepWaitingTime()) &&
							RightClientIsSuperior(lastClient, newclient) > 0) {

							//AddDebugLogLine(false, "%s: Ended upload to make room for higher prio client.", lastClient->GetUserName());
							// Remove last client from ul list to make room for higher prio client
		                    ScheduleRemovalFromUploadQueue(lastClient, GetResString(IDS_REMULHIGHERPRIO), GetResString(IDS_UPLOAD_PREEMPTED), true);
                        } else {
                            return false;
                        }
                    }
                } else {
                    return false;
                }
            }

            if(!IsDownloading(newclient) && !newclient->IsScheduledForRemoval()) {
            RemoveFromWaitingQueue(newclient, true);
			theApp.emuledlg->transferwnd->ShowQueueCount(waitinglist.GetCount());
            //} else {
            //    newclient->UnscheduleForRemoval();
            //    MoveDownInUploadQueue(newclient);
            }
		}
	}
	else
		newclient = directadd;

	if(newclient == NULL)
		return false;

	//Removed by SiRoB, Not used due to zz Upload system
	/*
	if (!thePrefs.TransferFullChunks())
		UpdateMaxClientScore(); // refresh score caching, now that the highest score is removed
	*/
	if (IsDownloading(newclient))
	{
        if(newclient->IsScheduledForRemoval()) {
            newclient->UnscheduleForRemoval();
            m_nLastStartUpload = ::GetTickCount();
    
            MoveDownInUploadQueue(newclient);

            if(pszReason && thePrefs.GetLogUlDlEvents())
                AddDebugLogLine(false, _T("Unscheduling client from being removed from upload list: %s Client: %s"), pszReason, newclient->DbgGetClientInfo());
            return true;
        }

		return false;
	}

    if(pszReason && thePrefs.GetLogUlDlEvents())
        AddDebugLogLine(false, _T("Adding client to upload list: %s Client: %s"), pszReason, newclient->DbgGetClientInfo());

	// tell the client that we are now ready to upload
	if (!newclient->socket || !newclient->socket->IsConnected())
	{
		newclient->SetUploadState(US_CONNECTING);
		if (!newclient->TryToConnect(true))
			return false;
		if (!newclient->socket) // Pawcio: BC
		{
			LogWarning(false,_T("---- Trying to add new client in queue with NULL SOCKET: %s"),newclient->DbgGetClientInfo());
			newclient->SetUploadState(US_NONE);
			return false;
		}
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
	
	//MORPH START - Changed by SiRoB, Upload Splitting Class
	uint32 newclientClassID = newclient->GetClassID();
	if(thePrefs.GetLogUlDlEvents()){
		CString buffer;
		buffer.Format(_T("USC: Added Slot in class %i -"),newclientClassID);
		for (uint32 classID = 0; classID < NB_SPLITTING_CLASS; classID++)
			buffer.AppendFormat(_T("[C%i %i/%i %i]-"),classID,m_aiSlotCounter[classID],m_iHighestNumberOfFullyActivatedSlotsSinceLastCallClass[classID],m_abOnClientOverHideClientDatarate[classID]);
		buffer.AppendFormat(_T(" Client: %s"),newclient->DbgGetClientInfo());
		DebugLog(LOG_USC,buffer);
	}
	m_abOnClientOverHideClientDatarate[newclientClassID] = False;
	for (uint32 classID = newclientClassID; classID < NB_SPLITTING_CLASS; classID++){
		++m_aiSlotCounter[classID];
		m_abAddClientOfThisClass[classID] = m_abOnClientOverHideClientDatarate[classID] || //one client in class reached max upload limit
											m_iHighestNumberOfFullyActivatedSlotsSinceLastCallClass[classID]>m_aiSlotCounter[classID]; //Upload Throttler want new slot
	}
	//MORPH END   - Changed by SiRoB, Upload Splitting Class
	
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
	if (reqfile)
		reqfile->statistic.AddAccepted();
	
	theApp.emuledlg->transferwnd->uploadlistctrl.AddClient(newclient);

	return true;
}

void CUploadQueue::UpdateActiveClientsInfo(DWORD curTick) {
    // Save number of active clients for statistics
    //MORPH START - Changed by SiRoB, Upload Splitting Class
	uint32 tempHighest = theApp.uploadBandwidthThrottler->GetHighestNumberOfFullyActivatedSlotsSinceLastCallAndReset(m_iHighestNumberOfFullyActivatedSlotsSinceLastCallClass);
	//MORPH START - Changed by SiRoB, Upload Splitting Class
	
	if(thePrefs.GetLogUlDlEvents() && theApp.uploadBandwidthThrottler->GetStandardListSize() > (uint32)uploadinglist.GetSize()) {
        // debug info, will remove this when I'm done.
        //DebugLogError(_T("UploadQueue: Error! Throttler has more slots than UploadQueue! Throttler: %i UploadQueue: %i Tick: %i"), theApp.uploadBandwidthThrottler->GetStandardListSize(), uploadinglist.GetSize(), ::GetTickCount());

		if(tempHighest > (uint32)uploadinglist.GetSize()+1) {
        	tempHighest = uploadinglist.GetSize()+1;
		}
    }

    m_iHighestNumberOfFullyActivatedSlotsSinceLastCall = tempHighest;
	
	// save 15 minutes of data about number of fully active clients
    uint32 tempMaxRemoved = -1;
    while(!activeClients_tick_list.IsEmpty() && !activeClients_list.IsEmpty() && curTick-activeClients_tick_list.GetHead() > 2*60*1000) {
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

        if(tempMaxRemoved > m_MaxActiveClients) {
			m_MaxActiveClients = tempMaxActiveClients;
        }

		m_MaxActiveClientsShortTime = tempMaxActiveClientsShortTime;
    } else {
        m_MaxActiveClients = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
        m_MaxActiveClientsShortTime = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
    }
}

/**
 * Maintenance method for the uploading slots. It adds and removes clients to/from the
 * uploading list. It also makes sure that all the uploading slots' Sockets always have
 * enough packets in their queues, etc.
 *
 * This method is called approximately once every 100 milliseconds.
 */
void CUploadQueue::Process() {
    DWORD curTick = ::GetTickCount();

	//MORPH - Added By SiRoB, not needed call UpdateDatarate only once in the process
	UpdateDatarates();
	//MORPH - Added By SiRoB, not needed call UpdateDatarate only once in the process

	UpdateActiveClientsInfo(curTick);

	//MORPH START - Added by SiRoB, Upload Splitting Class
	memset(m_aiSlotCounter,0,sizeof(m_aiSlotCounter));
	POSITION pos2 = uploadinglist.GetHeadPosition();
	while(pos2 != NULL){
		// Get the client. Note! Also updates pos as a side effect.
		CUpDownClient* cur_client = uploadinglist.GetNext(pos2);
		uint32 classID = cur_client->GetClassID();
		uint32 maxdatarate = thePrefs.GetMaxClientDataRate();
		switch (classID){
			case 0: maxdatarate = thePrefs.GetMaxClientDataRateFriend();break;
			case 1: maxdatarate = thePrefs.GetMaxClientDataRatePowerShare();break;
			default: maxdatarate = thePrefs.GetMaxClientDataRate();break;
		}
		if (maxdatarate > 0 && m_nLastStartUpload + 10000 < curTick && cur_client->GetDatarate()*10 >= 11*maxdatarate)
			m_abOnClientOverHideClientDatarate[classID] = true;
		for (uint32 i = classID; i < NB_SPLITTING_CLASS; i++)
			++m_aiSlotCounter[i];
	}
	uint32 curUploadSlots = (uint32)GetEffectiveUploadListCount();
	for (uint32 classID = 0; classID < NB_SPLITTING_CLASS; classID++)
		m_abAddClientOfThisClass[classID] = m_abOnClientOverHideClientDatarate[classID] || //one client in class reached max upload limit
											m_iHighestNumberOfFullyActivatedSlotsSinceLastCallClass[classID]>m_aiSlotCounter[classID] || //Upload Throttler want new slot
											m_iHighestNumberOfFullyActivatedSlotsSinceLastCallClass[classID]>curUploadSlots; //Scheduled slot
											
	//MORPH END   - Added by SiRoB, Upload Splitting Class
	
	CheckForHighPrioClient();
	
	//MORPH START - Changed vy SiRoB, cache (uint32)uploadinglist.GetCount()
	//Morph Start - changed by AndCycle, Dont Remove Spare Trickle Slot
	/*
	if(::GetTickCount()-m_nLastStartUpload > SEC2MS(20) && GetEffectiveUploadListCount() > 0 && GetEffectiveUploadListCount() > m_MaxActiveClientsShortTime+GetWantedNumberOfTrickleUploads() && AcceptNewClient(GetEffectiveUploadListCount()-1) == false) {
	*/
	if(thePrefs.DoRemoveSpareTrickleSlot() && ::GetTickCount()-m_nLastStartUpload > SEC2MS(20) && GetEffectiveUploadListCount() > 0 && GetEffectiveUploadListCount() > m_MaxActiveClientsShortTime+GetWantedNumberOfTrickleUploads() && AcceptNewClient(GetEffectiveUploadListCount()-1) == false) {
	//Morph End - changed by AndCycle, Dont Remove Spare Trickle Slot
	//MORPH END   - Changed by SiRoB, 
        // we need to close a trickle slot and put it back first on the queue

        POSITION lastpos = uploadinglist.GetTailPosition();

        CUpDownClient* lastClient = NULL;
        if(lastpos != NULL) {
            lastClient = uploadinglist.GetAt(lastpos);
        }

        if(lastClient != NULL && !lastClient->IsScheduledForRemoval() /*lastClient->GetUpStartTimeDelay() > 3*1000*/) {
            // There's too many open uploads (propably due to the user changing
            // the upload limit to a lower value). Remove the last opened upload and put
            // it back on the waitinglist. When it is put back, it get
            // to keep its waiting time. This means it is likely to soon be
            // choosen for upload again.

            // Remove from upload list.
            ScheduleRemovalFromUploadQueue(lastClient, GetResString(IDS_REMULMANYSLOTS), GetResString(IDS_UPLOAD_TOO_MANY_SLOTS), true /*, true*/);
		    // add to queue again.
            // the client is allowed to keep its waiting position in the queue, since it was pre-empted
            //lastClient->SendOutOfPartReqsAndAddToWaitingQueue(true);

            m_nLastStartUpload = ::GetTickCount();
        }
    } else if (ForceNewClient()){
        // There's not enough open uploads. Open another one.
        AddUpNextClient(_T("Not enough open upload slots for current ul speed"));
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
			if(!cur_client->IsScheduledForRemoval() || ::GetTickCount()-m_nLastStartUpload <= SEC2MS(11) || !cur_client->GetScheduledRemovalLimboComplete() || pos != NULL || cur_client->GetSlotNumber() <= GetActiveUploadsCount() || ForceNewClient(true)) {
				cur_client->SendBlockData();
			} else {
				bool keepWaitingTime = cur_client->GetScheduledUploadShouldKeepWaitingTime();
				RemoveFromUploadQueue(cur_client, (CString)_T("Scheduled for removal: ") + cur_client->GetScheduledRemovalDebugReason(), true, keepWaitingTime);
				AddClientToQueue(cur_client,keepWaitingTime,keepWaitingTime);
                m_nLastStartUpload = ::GetTickCount()-SEC2MS(9);
			}
		}
	}
		
	//MORPH START - Changed by SiRoB, Better datarate mesurement for low and high speed
	uint64 sentBytes = theApp.uploadBandwidthThrottler->GetNumberOfSentBytesSinceLastCallAndReset();
	if (sentBytes>0) {
    	if (avarage_tick_list.GetCount() > 0)
			avarage_tick_listPreviousAddedTimestamp = avarage_tick_list.GetTail();
		else
			avarage_tick_listPreviousAddedTimestamp = curTick;
		// Save used bandwidth for speed calculations
		avarage_dr_list.AddTail(sentBytes);
		m_avarage_dr_sum += sentBytes;
		uint64 sentBytesOverhead = theApp.uploadBandwidthThrottler->GetNumberOfSentBytesOverheadSinceLastCallAndReset();

		avarage_friend_dr_list.AddTail(theStats.sessionSentBytesToFriend);
    	// Save time beetween each speed snapshot
		avarage_tick_list.AddTail(curTick);
	}

	// don't save more than 5 secs of data
	while(avarage_tick_list.GetCount() > 1 && (curTick - avarage_tick_list.GetHead()) > MAXAVERAGETIMEUPLOAD){
		m_avarage_dr_sum -= avarage_dr_list.RemoveHead();
		avarage_friend_dr_list.RemoveHead();
		avarage_tick_list.RemoveHead();
	}
	//MORPH END  - Changed by SiRoB, Better datarate mesurement for low and high speed
};

bool CUploadQueue::AcceptNewClient(){
	uint32 curUploadSlots = (uint32)GetEffectiveUploadListCount();
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

	if (curUploadSlots >= 4 &&
        (
         /*curUploadSlots >= (datarate/UPLOAD_CHECK_CLIENT_DR) ||*/ //MORPH - Removed by SiRoB, 
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

bool CUploadQueue::ForceNewClient(bool simulateScheduledClosingOfSlot) {
	//MORPH START - Changed by SiRoB, Upload Splitting Class
	/*
	if (::GetTickCount() - m_nLastStartUpload < SEC2MS(1) && datarate < 102400 )
    */
	if (::GetTickCount() - m_nLastStartUpload < SEC2MS(3))
	//MORPH END   - Changed by SiRoB, Upload Splitting Class
		return false;
	
	uint32 curUploadSlots = (uint32)GetEffectiveUploadListCount();
    uint32 curUploadSlotsReal = (uint32)uploadinglist.GetCount();

    if(simulateScheduledClosingOfSlot) {
        if(curUploadSlotsReal < 1) {
            return true;
        } else {
            curUploadSlotsReal--;
        }
    }
	else //MORPH - Added by SiRoB, -Fix-
    if (curUploadSlots < MIN_UP_CLIENTS_ALLOWED)
		return true;

    if(!AcceptNewClient(curUploadSlots) || !theApp.lastCommonRouteFinder->AcceptNewClient()) { // UploadSpeedSense can veto a new slot if USS enabled
		return false;
    }

    uint32 activeSlots = m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;

    if(simulateScheduledClosingOfSlot) {
        activeSlots = m_MaxActiveClientsShortTime;
    }

    if(curUploadSlotsReal < m_iHighestNumberOfFullyActivatedSlotsSinceLastCall /*+1*/ ||
		curUploadSlots/*+1*/ < m_iHighestNumberOfFullyActivatedSlotsSinceLastCall/*+1*/ && ::GetTickCount() - m_nLastStartUpload > SEC2MS(10)) {
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
	    //This is to keep users from abusing the limits we put on lowID callbacks.
	    //1)Check if we are connected to any network and that we are a lowID.
	    //(Although this check shouldn't matter as they wouldn't have found us..
	    // But, maybe I'm missing something, so it's best to check as a precaution.)
	    //2)Check if the user is connected to Kad. We do allow all Kad Callbacks.
	    //3)Check if the user is in our download list or a friend..
	    //We give these users a special pass as they are helping us..
	    //4)Are we connected to a server? If we are, is the user on the same server?
	    //TCP lowID callbacks are also allowed..
	    //5)If the queue is very short, allow anyone in as we want to make sure
	    //our upload is always used.
	    if (theApp.IsConnected() 
		    && theApp.IsFirewalled()
		    && !client->GetKadPort()
		    && client->GetDownloadState() == DS_NONE 
		    && !client->IsFriend()
		    && theApp.serverconnect
			&& !theApp.serverconnect->IsLocalServer(client->GetServerIP(),client->GetServerPort())
			&& GetWaitingUserCount() > 50)
			return;
		client->AddAskedCount();
		client->SetLastUpRequest();
		if (!bIgnoreTimelimit)
			client->AddRequestCount(client->GetUploadFileID());
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
                client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick && AcceptNewClient() &&
				(m_abAddClientOfThisClass[LAST_CLASS] && !(client->IsFriend() && client->GetFriendSlot()) && !client->IsPBForPS() ||
				 m_abAddClientOfThisClass[1] && client->IsPBForPS() ||
				 m_abAddClientOfThisClass[0] && client->IsFriend() && client->GetFriendSlot())) //MORPH - Added by SiRoB, Upload Splitting Class
			{
                CUpDownClient* bestQueuedClient = FindBestClientInQueue(false);
                if(bestQueuedClient == client) {
					RemoveFromWaitingQueue(client, true);
				    AddUpNextClient(_T("Adding ****lowid when reconneting."), client);
				    return;
                //} else {
                //client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick = 0;
				}
			}
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
						delete cur_client;
				}
			}
			else
			{
				// remove both since we do not know who the bad one is
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false,CString(GetResString(IDS_SAMEUSERHASH)),client->GetUserName(),cur_client->GetUserName(),_T("Both") );
				// EastShare - Added by TAHO, modified SUQWT
				waitinglist.GetAt(pos2)->ClearWaitStartTime(); 
				// EastShare - Added by TAHO, modified SUQWT
				RemoveFromWaitingQueue(pos2,true);
				if (!cur_client->socket)
				{
					if(cur_client->Disconnected(_T("AddClientToQueue - same userhash 2")))
						delete cur_client;
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
					!thePrefs.IsEqualChanceEnable() && client->GetCombinedFilePrioAndCredit() < GetAverageCombinedFilePrioAndCredit() ||
					thePrefs.IsEqualChanceEnable() && client->GetCombinedFilePrioAndCredit() > GetAverageCombinedFilePrioAndCredit()//Morph - added by AndCycle, Equal Chance For Each File
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
		// EastShare - Added by TAHO, modified SUQWT
		// Mighty Knife: Check for credits!=NULL
		if (client->Credits() != NULL)
			client->Credits()->SetSecWaitStartTime();
		// [end] Mighty Knife
		// EastShare - Added by TAHO, modified SUQWT
	}

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

		//Morph - partial changed by AndCycle, Equal Chance For Each File - the equal chance have a risk of overflow ... so I use double
		double sum = 0;
		for (POSITION pos = waitinglist.GetHeadPosition(); pos != NULL;/**/){
		    CUpDownClient* cur_client =	waitinglist.GetNext(pos);
			sum += cur_client->GetCombinedFilePrioAndCredit();
		}
		m_fAverageCombinedFilePrioAndCredit = sum/waitinglist.GetSize();
	}

	return m_fAverageCombinedFilePrioAndCredit;
}
// Moonlight: SUQWT: Reset wait time on session success, save it on failure.//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)

//MORPH START - Added By AndCycle, ZZUL_20050212-0200
void CUploadQueue::ScheduleRemovalFromUploadQueue(CUpDownClient* client, LPCTSTR pszDebugReason, CString strDisplayReason, bool earlyabort) {
	if (thePrefs.GetLogUlDlEvents())
        AddDebugLogLine(DLP_VERYLOW, true,_T("Scheduling to remove client from upload list: %s Client: %s Transfered: %s SessionUp: %s QueueSessionUp: %s QueueSessionPayload: %s"), pszDebugReason==NULL ? _T("") : pszDebugReason, client->DbgGetClientInfo(), CastSecondsToHM( client->GetUpStartTimeDelay()/1000), CastItoXBytes(client->GetSessionUp(), false, false), CastItoXBytes(client->GetQueueSessionUp(), false, false), CastItoXBytes(client->GetQueueSessionPayloadUp(), false, false));

    client->ScheduleRemovalFromUploadQueue(pszDebugReason, strDisplayReason, earlyabort);
	MoveDownInUploadQueue(client);

    m_nLastStartUpload = ::GetTickCount();
}
//MORPH END   - Added By AndCycle, ZZUL_20050212-0200

bool CUploadQueue::RemoveFromUploadQueue(CUpDownClient* client, LPCTSTR pszReason, bool updatewindow, bool earlyabort){
    POSITION foundPos = uploadinglist.Find(client);
	if(foundPos != NULL) {
		POSITION renumberPosition = uploadinglist.GetTailPosition();
		while(renumberPosition != foundPos) {
			CUpDownClient* renumberClient = uploadinglist.GetAt(renumberPosition);
			renumberClient->SetSlotNumber(renumberClient->GetSlotNumber()-1);
			uploadinglist.GetPrev(renumberPosition);
		}
        if(client->socket) {
			if (thePrefs.GetDebugClientTCPLevel() > 0)
			    DebugSend("OP__OutOfPartReqs", client);
			Packet* pCancelTransferPacket = new Packet(OP_OUTOFPARTREQS, 0);
			theStats.AddUpDataOverheadFileRequest(pCancelTransferPacket->size);
			client->socket->SendPacket(pCancelTransferPacket,true,true);
        }
		if (updatewindow)
			theApp.emuledlg->transferwnd->uploadlistctrl.RemoveClient(client);
		if (thePrefs.GetLogUlDlEvents())
               AddDebugLogLine(DLP_VERYLOW, true,_T("Removing client from upload list: %s Client: %s Transferred: %s SessionUp: %s QueueSessionUp: %s QueueSessionPayload: %s"), pszReason==NULL ? _T("") : pszReason, client->DbgGetClientInfo(), CastSecondsToHM( client->GetUpStartTimeDelay()/1000), CastItoXBytes(client->GetSessionUp(), false, false), CastItoXBytes(client->GetQueueSessionUp(), false, false), CastItoXBytes(client->GetQueueSessionPayloadUp(), false, false));
       	client->m_dwWouldHaveGottenUploadSlotIfNotLowIdTick = 0;
		client->UnscheduleForRemoval();
		uploadinglist.RemoveAt(foundPos);

		/* Morph - been take care later
        if(!earlyabort)
               client->SetWaitStartTime();
		*/

		bool removed = theApp.uploadBandwidthThrottler->RemoveFromStandardList(client->socket);

		// Mighty Knife: more detailed logging
		/*
		if (thePrefs.GetLogUlDlEvents())
			AddDebugLogLine(DLP_VERYLOW, true,_T("---- Main socket %ssuccessully removed from upload list."),removed ? _T("") : _T("NOT "));
		*/
		// [end] Mighty Knife

		bool pcRemoved = theApp.uploadBandwidthThrottler->RemoveFromStandardList((CClientReqSocket*)client->m_pPCUpSocket);
		// Mighty Knife: more detailed logging
		/*
		if (thePrefs.GetLogUlDlEvents())
			AddDebugLogLine(DLP_VERYLOW, true,_T("---- PeerCache-socket %ssuccessully removed from upload list."),pcRemoved ? _T("") : _T("NOT "));
		*/
		// [end] Mighty Knife

		//MORPH START - Added by SiRoB, due to zz upload system WebCache
		bool wcRemoved = theApp.uploadBandwidthThrottler->RemoveFromStandardList((CClientReqSocket*)client->m_pWCUpSocket);
		//MORPH END   - Added by SiRoB, due to zz upload system WebCache

		// Mighty Knife: more detailed logging
		/*
		if (thePrefs.GetLogUlDlEvents())
			AddDebugLogLine(DLP_VERYLOW, true,_T("---- WebCache-socket %ssuccessully removed from upload list."),wcRemoved ? _T("") : _T("NOT "));
		*/
		// [end] Mighty Knife

		/*if(thePrefs.GetLogUlDlEvents() && !(removed || pcRemoved || wcRemoved)) {
        	DebugLogError(false, _T("UploadQueue: Didn't find socket to delete. socket: 0x%x, PCUpSocket: 0x%x, WCUpSocket: 0x%x"), client->socket,client->m_pPCUpSocket,client->m_pWCUpSocket);
        }*/
		//EastShare Start - added by AndCycle, Pay Back First
		//client normal leave the upload queue, check does client still satisfy requirement
		if(earlyabort == false){
			// Mighty Knife: Check for credits!=NULL
			if (client->Credits() != NULL)
				client->Credits()->InitPayBackFirstStatus();
			// [end] Mighty Knife
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

			//MORPH START - Moved by SiRoB, du to ShareOnlyTheNeed hide Uploaded and uploading part
			theApp.clientlist->AddTrackClient(client); // Keep track of this client
			client->SetUploadState(US_NONE);
			client->ClearUploadBlockRequests();
			//MORPH END   - Moved by SiRoB, du to ShareOnlyTheNeed hide Uploaded and uploading part

			CKnownFile* requestedFile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());
			if(requestedFile != NULL) {
			    //MORPH START - Added by SiRoB, UpdatePartsInfo -Fix-
				if(requestedFile->IsPartFile())
					((CPartFile*)requestedFile)->UpdatePartsInfo();
				else
				//MORPH END   - Added by SiRoB, UpdatePartsInfo -Fix-
					requestedFile->UpdatePartsInfo();
			}

		m_iHighestNumberOfFullyActivatedSlotsSinceLastCall = 0;
		//MORPH START - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
		// EastShare START - Marked by TAHO, modified SUQWT

		// Mighty Knife: Check for credits!=NULL
		if (client->Credits()!=NULL) {
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
		}
		// [end] Mighty Knife
		// EastShare END - Marked by TAHO, modified SUQWT
		//MORPH END   - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
		return true;
	}
	return false;
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
		//Removed by SiRoB, due to zz way m_dwWouldHaveGottenUploadSlotIfNotLowIdTick
		/*
		client->m_bAddNextConnect = false;
		*/
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

//Removed by SiRoB, not used due to zz way
/*
bool CUploadQueue::CheckForTimeOver(CUpDownClient* client){
	//If we have nobody in the queue, do NOT remove the current uploads..
	//This will save some bandwidth and some unneeded swapping from upload/queue/upload..
	if ( waitinglist.IsEmpty() || client->GetFriendSlot() )
		return false;
	if (!thePrefs.TransferFullChunks()){
	    if( client->GetUpStartTimeDelay() > SESSIONMAXTIME){ // Try to keep the clients from downloading for ever
		    if (thePrefs.GetLogUlDlEvents())
			    AddDebugLogLine(DLP_LOW, false, _T("%s: Upload session ended due to max time %s."), client->GetUserName(), CastSecondsToHM(SESSIONMAXTIME/1000));
		    return true;
	    }

		// Cache current client score
		const uint32 score = client->GetScore(true, true);

		// Check if another client has a bigger score
		if (score < GetMaxClientScore() && m_dwRemovedClientByScore < GetTickCount()) {
			if (thePrefs.GetLogUlDlEvents())
				AddDebugLogLine(DLP_VERYLOW, false, _T("%s: Upload session ended due to score."), client->GetUserName());
			//Set timer to prevent to many uploadslot getting kick do to score.
			//Upload slots are delayed by a min of 1 sec and the maxscore is reset every 5 sec.
			//So, I choose 6 secs to make sure the maxscore it updated before doing this again.
			m_dwRemovedClientByScore = GetTickCount()+SEC2MS(6);
			return true;
		}
	}
	else{
		// Allow the client to download a specified amount per session
		if( client->GetQueueSessionPayloadUp() > SESSIONMAXTRANS ){
			if (thePrefs.GetLogUlDlEvents())
				AddDebugLogLine(DLP_DEFAULT, false, _T("%s: Upload session ended due to max transferred amount. %s"), client->GetUserName(), CastItoXBytes(SESSIONMAXTRANS, false, false));
			return true;
		}
	}
	return false;
}
*/
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

		//MOPRH START - Modified by SiRoB
		/*
		// ZZ:UploadSpeedSense -->
		theApp.lastCommonRouteFinder->SetPrefs(thePrefs.IsDynUpEnabled(), theApp.uploadqueue->GetDatarate(), thePrefs.GetMinUpload()*1024, (thePrefs.GetMaxUpload() != 0)?thePrefs.GetMaxUpload()*1024:thePrefs.GetMaxGraphUploadRate()*1024, thePrefs.IsDynUpUseMillisecondPingTolerance(), (thePrefs.GetDynUpPingTolerance() > 100)?((thePrefs.GetDynUpPingTolerance()-100)/100.0f):0, thePrefs.GetDynUpPingToleranceMilliseconds(), thePrefs.GetDynUpGoingUpDivider(), thePrefs.GetDynUpGoingDownDivider(), thePrefs.GetDynUpNumberOfPings(), 20); // PENDING: Hard coded min pLowestPingAllowed
		*/
		theApp.lastCommonRouteFinder->SetPrefs(thePrefs.IsDynUpEnabled(), theApp.uploadqueue->GetDatarate(), thePrefs.GetMinUpload()*1024, (thePrefs.IsSUCDoesWork())?theApp.uploadqueue->GetMaxVUR():(thePrefs.GetMaxUpload() != 0)?thePrefs.GetMaxUpload()*1024:thePrefs.GetMaxGraphUploadRate()*1024, thePrefs.IsDynUpUseMillisecondPingTolerance(), (thePrefs.GetDynUpPingTolerance() > 100)?((thePrefs.GetDynUpPingTolerance()-100)/100.0f):0, thePrefs.GetDynUpPingToleranceMilliseconds(), thePrefs.GetDynUpGoingUpDivider(), thePrefs.GetDynUpGoingDownDivider(), thePrefs.GetDynUpNumberOfPings(), 5, thePrefs.IsUSSLog(), thePrefs.GetGlobalDataRateFriend(), thePrefs.GetMaxClientDataRateFriend(), thePrefs.GetGlobalDataRatePowerShare(), thePrefs.GetMaxClientDataRatePowerShare(), thePrefs.GetMaxClientDataRate());
		//MOPRH END   - Modified by SiRoB, Upload Splitting Class

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
					//theApp.emuledlg->statisticswnd->SetCurrentRate((float)(theApp.uploadqueue->Getavgupload()/theApp.uploadqueue->Getavg())/1024,(float)(theApp.uploadqueue->Getavgdownload()/theApp.uploadqueue->Getavg())/1024);
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
				/*
				if (!thePrefs.TransferFullChunks())
					theApp.uploadqueue->UpdateMaxClientScore();
				*/
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

//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
void CUploadQueue::UpdateDatarates() {
	// Calculate average datarate
	//MORPH - Removed By SiRoB, not needed call UpdateDatarate only once in the process
	/*
	if(::GetTickCount()-m_lastCalculatedDataRateTick > 500) {
	m_lastCalculatedDataRateTick = ::GetTickCount();
	*/
		

		//MORPH START - Changed by SiRoB, Better datarate mesurement for low and high speed
		/*
		if(avarage_dr_list.GetSize() >= 2 && (avarage_tick_list.GetTail() > avarage_tick_list.GetHead())) {
			datarate = ((m_avarage_dr_sum-avarage_dr_list.GetHead())*1000) / (avarage_tick_list.GetTail()-avarage_tick_list.GetHead());
			friendDatarate = ((avarage_friend_dr_list.GetTail()-avarage_friend_dr_list.GetHead())*1000) / (avarage_tick_list.GetTail()-avarage_tick_list.GetHead());

		}
		*/
		if (avarage_tick_list.GetCount() > 1){
			DWORD dwDuration = avarage_tick_list.GetTail() - avarage_tick_list.GetHead();
			DWORD curTick = ::GetTickCount();
			if ((curTick - avarage_tick_list.GetTail()) > (avarage_tick_list.GetTail() - avarage_tick_listPreviousAddedTimestamp))
				dwDuration += curTick - avarage_tick_list.GetTail() - (avarage_tick_list.GetTail() - avarage_tick_listPreviousAddedTimestamp);
			if (dwDuration < MAXAVERAGETIMEUPLOAD/2) dwDuration = MAXAVERAGETIMEUPLOAD/2;
			datarate = 1000 * (m_avarage_dr_sum-avarage_dr_list.GetHead()) / dwDuration;
			friendDatarate = 1000 * (avarage_friend_dr_list.GetTail()-avarage_friend_dr_list.GetHead()) / dwDuration;
		}else if (avarage_tick_list.GetCount() == 1){
			DWORD dwDuration = avarage_tick_list.GetTail() - avarage_tick_listPreviousAddedTimestamp;
			DWORD curTick = ::GetTickCount();
			if ((curTick - avarage_tick_list.GetTail()) > dwDuration)
				dwDuration = curTick - avarage_tick_list.GetTail();
			if (dwDuration < MAXAVERAGETIMEUPLOAD/2) dwDuration = MAXAVERAGETIMEUPLOAD/2;
			datarate = 1000 * m_avarage_dr_sum / dwDuration;
			friendDatarate = 0;
		}else {
			datarate = 0;
			friendDatarate = 0;
		}
		//MORPH END   - Changed by SiRoB, Better datarate mesurement for low and high speed
	//MORPH - Removed By SiRoB, not needed call UpdateDatarate only once in the process
	/*
	}
	*/
}

uint32 CUploadQueue::GetDatarate() {
	//MORPH - Removed By SiRoB, not needed call UpdateDatarate only once in the process
	/*
	UpdateDatarates();
	*/
	return datarate;
}

uint32 CUploadQueue::GetToNetworkDatarate() {
	//MORPH - Removed By SiRoB, not needed call UpdateDatarate only once in the process
	/*
	UpdateDatarates();
	*/
	if(datarate > friendDatarate) {
		return datarate - friendDatarate;
	} else {
		return 0;
	}
}

//MORPH START - Added By AndCycle, ZZUL_20050212-0200
uint32 CUploadQueue::GetWantedNumberOfTrickleUploads() {
    uint32 minNumber = MINNUMBEROFTRICKLEUPLOADS;

    //if(minNumber < 2 && thePrefs.GetMaxUpload() >= 4) {
    //    minNumber = 2;
    //} else
    if(minNumber < 1 && GetDatarate() >= 2*1024 /*thePrefs.GetMaxUpload() >= 2*/) {
        minNumber = 1;
    }

    return max(((uint32)GetEffectiveUploadListCount())*0.1, minNumber);
}
//MORPH END   - Added By AndCycle, ZZUL_20050212-0200

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
   			theApp.uploadBandwidthThrottler->RemoveFromStandardList(cur_client->socket,true);
			theApp.uploadBandwidthThrottler->RemoveFromStandardList((CClientReqSocket*)cur_client->m_pPCUpSocket,true);
			//MORPH START - Added by SiRoB, due to zz upload system WebCache
			theApp.uploadBandwidthThrottler->RemoveFromStandardList((CClientReqSocket*)cur_client->m_pWCUpSocket,true);
			//MORPH END   - Added by SiRoB, due to zz upload system WebCache
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
            added = AddUpNextClient(_T("High prio client (i.e. friend/powershare)."), NULL, true);
        }
	}
}
// MORPH START - Added by Commander, WebCache 1.2e
CUpDownClient*	CUploadQueue::FindClientByWebCacheUploadId(const uint32 id) // Superlexx - webcache - can be made more efficient
{
	for (POSITION pos = uploadinglist.GetHeadPosition(); pos != NULL;)
	{
		CUpDownClient* cur_client = uploadinglist.GetNext(pos);
		if ( cur_client->m_uWebCacheUploadId == id )
			return cur_client;
	}
	return 0;
}
// MORPH END - Added by Commander, WebCache 1.2e

//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
uint32	CUploadQueue::GetMaxVUR()
{
	return min(max(MaxVUR,(uint32)1024*thePrefs.GetMinUpload()),(uint32)1024*thePrefs.GetMaxUpload());
}
//MORPH END   - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]