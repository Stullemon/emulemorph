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

#include "StdAfx.h"
#include "uploadqueue.h"
#include "packets.h"
#include "emule.h"
#include "SearchDlg.h"
#include "knownfile.h"
#include "listensocket.h"
#include "ini2.h"
#include "math.h"
#include "Exceptions.h"
#include "Scheduler.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


static uint32 counter, sec,statsave;
// -khaos--+++> Added iupdateconnstats...
static uint32 igraph, istats, iupdateconnstats;
// <-----khaos-

//TODO rewrite the whole networkcode, use overlapped sockets

CUploadQueue::CUploadQueue(CPreferences* in_prefs){
	app_prefs = in_prefs;
	VERIFY( (h_timer = SetTimer(0,0,100,UploadTimer)) );
	if (!h_timer)
		AddDebugLogLine(true,_T("Failed to create 'upload queue' timer - %s"),GetErrorMessage(GetLastError()));
	estadatarate = 2000;
	datarate = 0;
	//dataratems = 0;
	datarateave = 0;
	counter=0;
	successfullupcount = 0;
	failedupcount = 0;
	totaluploadtime = 0;
	//m_nUpDataRateMSOverhead = 0; //MORPH - Removed by SiRoB, ZZ UPload System 20030818-1923
	m_nUpDatarateOverhead = 0;
	m_nUpDataOverheadSourceExchange = 0;
	m_nUpDataOverheadFileRequest = 0;
	m_nUpDataOverheadOther = 0;
	m_nUpDataOverheadServer = 0;
	m_nUpDataOverheadSourceExchangePackets = 0;
	m_nUpDataOverheadFileRequestPackets = 0;
	m_nUpDataOverheadOtherPackets = 0;
	m_nUpDataOverheadServerPackets = 0;
	m_nLastStartUpload = 0;
	statsave=0;
	// -khaos--+++>
	iupdateconnstats=0;
	// <-----khaos-
	//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	AvgRespondTime[0]=500;
	AvgRespondTime[1]=theApp.glob_prefs->GetSUCPitch()/2;//Changed by Yun.SF3 (this is too much divided, original is 3)
	MaxVUR=512*(app_prefs->GetMaxUpload()+app_prefs->GetMinUpload()); //When we start with SUC take the middle range for upload
	//MORPH END - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	// By BadWolf - Accurate Speed Measurement
	sumavgUDRO = 0;
	//sendperclient = 0;
	//uLastAcceptNewClient = ::GetTickCount();
	//sumavgdata = 0;
	//m_delay = ::GetTickCount();
	//m_delaytmp = 0;
	// END By BadWolf - Accurate Speed Measurement

//MORPH START - Added by SiRoB, ZZ Upload System 20030723-0133
	m_MaxActiveClients = 0;
	m_MaxActiveClientsShortTime = 0;

	m_lastCalculatedDataRateTick = 0;
	m_dwLastCheckedForHighPrioClient = 0;
	m_dwLastResortedUploadSlots = 0;

	m_dwLastCalculatedAverageCombinedFilePrioAndCredit = 0;
	m_fAverageCombinedFilePrioAndCredit = 0;

	m_avarage_dr_sum = 0;
	m_dwLastSlotAddTick = 0;
	friendDatarate = 0;
	totalCompletedBytes = 0;
	m_FirstRanOutOfSlotsTick = 0;
//MORPH END - Added by SiRoB, ZZ Upload System 20030723-0133
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
    CUpDownClient* newclient = FindBestClientInQueue(true, client);

    if(newclient != NULL && // Only remove the client if there's someone to replace it
		(
			(client->IsFriend() && client->GetFriendSlot()) == false &&	// if it is not in a class that gives it a right
			client->GetPowerShared() == false &&						// to have a check performed to see if it can stay, we remove at once
			client->MoreUpThanDown() == false ||							//EastShare - added by AndCycle, PayBackFirst
			//client->m_BlockRequests_queue.IsEmpty() ||         // or if it doesn't want any more blocks
			(
				(
					(newclient->MoreUpThanDown() == true && client->MoreUpThanDown() == false || //EastShare - added by AndCycle, PayBackFirst
					newclient->MoreUpThanDown() == client->MoreUpThanDown() && //EastShare - added by AndCycle, PayBackFirst
						(newclient->MoreUpThanDown() == true && client->MoreUpThanDown() == true || //EastShare - added by AndCycle, PayBackFirst
						newclient->GetPowerShared() == true && client->GetPowerShared() == false || // new client wants powershare file, but old client don't
						newclient->GetPowerShared() == true && client->GetPowerShared() == true && newclient->GetFilePrioAsNumber() >= client->GetFilePrioAsNumber() // both want powersharedfile, and newer wants higher/same prio file
						)//EastShare - added by AndCycle, PayBackFirst
					) &&
					(client->IsFriend() && client->GetFriendSlot()) == false
				) || // old client don't have friend slot
				(newclient->IsFriend() && newclient->GetFriendSlot()) == true // new friend has friend slot, this means it is of highest prio, and will always get this slot
			)
		)
	){

        // Remove client from ul list to make room for higher/same prio client
        theApp.emuledlg->AddDebugLogLine(false, GetResString(IDS_ULSUCCESSFUL), client->GetUserName(), CastItoXBytes(client->GetQueueSessionPayloadUp()), CastItoXBytes(SESSIONAMOUNT*max(1, client->GetQueueSessionPayloadUp()/SESSIONAMOUNT)), (sint32)client->GetQueueSessionPayloadUp()-SESSIONAMOUNT*(max(1, client->GetQueueSessionPayloadUp()/SESSIONAMOUNT)));

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

//Morph Start - added by AndCycle, Equal Chance For Each File
	bool	rightGetQueueFile = false;
	bool	bothGetQueueFile = true;
	CKnownFile* rightReqFile = NULL;
	CKnownFile* leftReqFile = NULL;

	if(rightClient && leftClient){
		rightReqFile = theApp.sharedfiles->GetFileByID((uchar*)rightClient->GetUploadFileID());
		leftReqFile = theApp.sharedfiles->GetFileByID((uchar*)leftClient->GetUploadFileID());
	}
	if(rightReqFile && leftReqFile){

		switch(theApp.glob_prefs->GetEqualChanceForEachFileMode()){

			case ECFEF_ACCEPTED:{
				if(theApp.glob_prefs->IsECFEFallTime()){
					rightGetQueueFile = 
						rightReqFile->statistic.GetAllTimeAccepts() < leftReqFile->statistic.GetAllTimeAccepts();
					bothGetQueueFile =
						rightReqFile->statistic.GetAllTimeAccepts() == leftReqFile->statistic.GetAllTimeAccepts();

				}
				else{
					rightGetQueueFile = 
						rightReqFile->statistic.GetAccepts() < leftReqFile->statistic.GetAccepts();
					bothGetQueueFile =
						rightReqFile->statistic.GetAccepts() == leftReqFile->statistic.GetAccepts();
				}
			}break;

			case ECFEF_ACCEPTED_COMPLETE:{
				if(theApp.glob_prefs->IsECFEFallTime()){
					rightGetQueueFile =
						(float)rightReqFile->statistic.GetAllTimeAccepts()/rightReqFile->GetPartCount() <	
						(float)leftReqFile->statistic.GetAllTimeAccepts()/leftReqFile->GetPartCount() ;
					bothGetQueueFile =
						(float)rightReqFile->statistic.GetAllTimeAccepts()/rightReqFile->GetPartCount() ==	
						(float)leftReqFile->statistic.GetAllTimeAccepts()/leftReqFile->GetPartCount() ;
				}
				else{
					rightGetQueueFile =
						(float)rightReqFile->statistic.GetAccepts()/rightReqFile->GetPartCount() <	
						(float)leftReqFile->statistic.GetAccepts()/leftReqFile->GetPartCount() ;
					bothGetQueueFile =
						(float)rightReqFile->statistic.GetAccepts()/rightReqFile->GetPartCount() ==	
						(float)leftReqFile->statistic.GetAccepts()/leftReqFile->GetPartCount() ;
				}
			}break;

			case ECFEF_TRANSFERRED:{
				if(theApp.glob_prefs->IsECFEFallTime()){
					rightGetQueueFile =
						rightReqFile->statistic.GetAllTimeTransferred() < leftReqFile->statistic.GetTransferred();
					bothGetQueueFile =
						rightReqFile->statistic.GetAllTimeTransferred() == leftReqFile->statistic.GetTransferred();
				}
				else{
					rightGetQueueFile =
						rightReqFile->statistic.GetTransferred() < leftReqFile->statistic.GetTransferred();
					bothGetQueueFile =
						rightReqFile->statistic.GetTransferred() == leftReqFile->statistic.GetTransferred();
				}
			}break;

			case ECFEF_TRANSFERRED_COMPLETE:{
				if(theApp.glob_prefs->IsECFEFallTime()){
					rightGetQueueFile =
						(float)rightReqFile->statistic.GetAllTimeTransferred()/rightReqFile->GetFileSize() < 
						(float)leftReqFile->statistic.GetAllTimeTransferred()/leftReqFile->GetFileSize();
					bothGetQueueFile =
						(float)rightReqFile->statistic.GetAllTimeTransferred()/rightReqFile->GetFileSize() == 
						(float)leftReqFile->statistic.GetAllTimeTransferred()/leftReqFile->GetFileSize();
				}
				else{
					rightGetQueueFile =
						(float)rightReqFile->statistic.GetTransferred()/rightReqFile->GetFileSize() < 
						(float)leftReqFile->statistic.GetTransferred()/leftReqFile->GetFileSize();
					bothGetQueueFile =
						(float)rightReqFile->statistic.GetTransferred()/rightReqFile->GetFileSize() == 
						(float)leftReqFile->statistic.GetTransferred()/leftReqFile->GetFileSize();
				}
			}break;
		}
	}
	//Morph End - added by AndCycle, Equal Chance For Each File

	if(
		(leftClient != NULL &&
			(
				(rightClient->IsFriend() && rightClient->GetFriendSlot()) == true && (leftClient->IsFriend() && leftClient->GetFriendSlot()) == false || // rightClient has friend slot, but leftClient has not, so rightClient is better
				(leftClient->IsFriend() && leftClient->GetFriendSlot()) == (rightClient->IsFriend() && rightClient->GetFriendSlot()) && // both or none have friend slot, let file prio and score decide
				(
					rightClient->MoreUpThanDown() == true && leftClient->MoreUpThanDown() == false || //EastShare - added by AndCycle, PayBackFirst
					(leftClient->MoreUpThanDown() == rightClient->MoreUpThanDown()) && //EastShare - added by AndCycle, PayBackFirst
					(//EastShare - added by AndCycle, PayBackFirst
						leftClient->GetPowerShared() == false && rightClient->GetPowerShared() == true || // rightClient wants powershare file, but leftClient not, so rightClient is better
						leftClient->GetPowerShared() == true && rightClient->GetPowerShared() == true && // they both want powershare file
						(
							leftClient->GetFilePrioAsNumber() < rightClient->GetFilePrioAsNumber() || // and rightClient wants higher prio file, so rightClient is better
							leftClient->GetFilePrioAsNumber() ==  rightClient->GetFilePrioAsNumber() && 
							(
								rightGetQueueFile == true ||	//Morph - added by AndCycle, Equal Chance For Each File
								bothGetQueueFile == true &&		//Morph - added by AndCycle, Equal Chance For Each File
								(
									rightScore > leftScore  // same prio file, but rightClient has better score, so rightClient is better
								)
							)
						) ||  
						leftClient->GetPowerShared() == false && rightClient->GetPowerShared() == false && //neither want powershare file
						(
							rightGetQueueFile == true ||	//Morph - added by AndCycle, Equal Chance For Each File
							bothGetQueueFile == true &&		//Morph - added by AndCycle, Equal Chance For Each File
							(
								rightScore > leftScore  // but rightClient has better score, so rightClient is better
							)
						)
					)//EastShare - added by AndCycle, PayBackFirst
				)
			) ||
			leftClient == NULL // there's no old client to compare with, so rightClient is better (than null)
		) &&
		(!rightClient->IsBanned()) && // don't allow banned client to be best
		IsDownloading(rightClient) == false // don't allow downloading clients to be best
		//EastShare Start- added by AndCycle, dont allow identificaion failed client get upload?
		&& 
		!(
			(
				rightClient->credits->GetCurrentIdentState(rightClient->GetIP()) == IS_IDFAILED || 
				rightClient->credits->GetCurrentIdentState(rightClient->GetIP()) == IS_IDBADGUY || 
				rightClient->credits->GetCurrentIdentState(rightClient->GetIP()) == IS_IDNEEDED
			) && theApp.clientcredits->CryptoAvailable()
		)
		//EastShare End- added by AndCycle, dont allow identificaion failed client get upload?
	) {
		return true;
	} else {
		return false;
	}

}

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
	for (pos1 = waitinglist.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		waitinglist.GetNext(pos1);
		CUpDownClient* cur_client =	waitinglist.GetAt(pos2);
		// clear dead clients
		ASSERT ( cur_client->GetLastUpRequest() );
		if ((::GetTickCount() - cur_client->GetLastUpRequest() > MAX_PURGEQUEUETIME) || !theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID()) ){
			RemoveFromWaitingQueue(pos2,true);	
			if (!cur_client->socket)
			cur_client->Disconnected("Socket it NULL. 1");
		} else {
			// finished clearing
			uint32 cur_score = cur_client->GetScore(false);

			if (RightClientIsBetter(newclient, bestscore, cur_client, cur_score)) {
				// cur_client is more worthy than current best client that is ready to go (connected).
				if(!cur_client->HasLowID() || (cur_client->socket && cur_client->socket->IsConnected())) {
					// this client is a HighID or a lowID client that is ready to go (connected)
					// and it is more worthy
					bestscore = cur_score;
					toadd = pos2;
					newclient = waitinglist.GetAt(toadd);
				} else if(allowLowIdAddNextConnectToBeSet && !cur_client->m_bAddNextConnect) {
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
		
	if (lowclient != NULL && bestlowscore > bestscore && allowLowIdAddNextConnectToBeSet) {
		newclient = waitinglist.GetAt(toaddlow);

        // is newclient in same or better class as lowIdClientMustBeInSameOrBetterClassAsThisClient?
		if(lowIdClientMustBeInSameOrBetterClassAsThisClient == NULL ||
			(lowIdClientMustBeInSameOrBetterClassAsThisClient->IsFriend() && lowIdClientMustBeInSameOrBetterClassAsThisClient->GetFriendSlot()) == true &&
			(newclient->IsFriend() && newclient->GetFriendSlot()) == false || // lowIdClientMustBeInSameOrBetterClassAsThisClient has friend slot, but newclient not. lowIdClientMustBeInSameOrBetterClassAsThisClient is better
			(lowIdClientMustBeInSameOrBetterClassAsThisClient->IsFriend() && lowIdClientMustBeInSameOrBetterClassAsThisClient->GetFriendSlot()) == (newclient->IsFriend() && newclient->GetFriendSlot()) && // both, or neither has friend slots, let powershared and file prio decide
			(
		    	lowIdClientMustBeInSameOrBetterClassAsThisClient->MoreUpThanDown() == true && newclient->MoreUpThanDown() == false || //EastShare - added by AndCycle, PayBackFirst
				(lowIdClientMustBeInSameOrBetterClassAsThisClient->MoreUpThanDown() == newclient->MoreUpThanDown()) && //EastShare - added by AndCycle, PayBackFirst
				(//EastShare - added by AndCycle, PayBackFirst
					lowIdClientMustBeInSameOrBetterClassAsThisClient->GetPowerShared() == true && newclient->GetPowerShared() == false ||
					lowIdClientMustBeInSameOrBetterClassAsThisClient->GetPowerShared() == true && newclient->GetPowerShared() == true && // Both want powershared
					lowIdClientMustBeInSameOrBetterClassAsThisClient->GetFilePrioAsNumber() >= newclient->GetFilePrioAsNumber() || // and lowIdClientMustBeInSameOrBetterClassAsThisClient wants same or higher prio, it's ok
					lowIdClientMustBeInSameOrBetterClassAsThisClient->GetPowerShared() == false && newclient->GetPowerShared() == false // neither wants powershared file, it's ok
				)//EastShare - added by AndCycle, PayBackFirst
           )
		){
			newclient->m_bAddNextConnect = true;
		}
	}

    if (!toadd) {
		return NULL;
    } else {
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

		if(
			(uploadingClient->IsFriend() && uploadingClient->GetFriendSlot()) == true && (newclient->IsFriend() && newclient->GetFriendSlot()) == false ||
			(uploadingClient->IsFriend() && uploadingClient->GetFriendSlot()) == (newclient->IsFriend() && newclient->GetFriendSlot()) &&
			(
				uploadingClient->MoreUpThanDown() == true && newclient->MoreUpThanDown() == false || //EastShare - added by AndCycle, PayBackFirst
				(uploadingClient->MoreUpThanDown() == newclient->MoreUpThanDown()) && //EastShare - added by AndCycle, PayBackFirst
				(//EastShare - added by AndCycle, PayBackFirst
					uploadingClient->GetPowerShared() == true && newclient->GetPowerShared() == false ||
					uploadingClient->GetPowerShared() == true && newclient->GetPowerShared() == true && uploadingClient->GetFilePrioAsNumber() > newclient->GetFilePrioAsNumber() ||
					(
						uploadingClient->GetPowerShared() == true && newclient->GetPowerShared() == true && uploadingClient->GetFilePrioAsNumber() == newclient->GetFilePrioAsNumber() ||
						uploadingClient->GetPowerShared() == false && newclient->GetPowerShared() == false
					)//EastShare - added by AndCycle, PayBackFirst
				) &&
				(
					!newclient->HasLowID() || !newclient->m_bAddNextConnect ||
					newclient->HasLowID() && newclient->m_bAddNextConnect && newclientScore <= uploadingClient->GetScore(false)
					// Compare scores is more right than comparing waittime.
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
        theApp.uploadBandwidthThrottler->AddToStandardList(posCounter, newclient->socket);
    } else {
        // Add it last
        theApp.uploadBandwidthThrottler->AddToStandardList(uploadinglist.GetCount(), newclient->socket);
		uploadinglist.AddTail(newclient);
        newclient->SetSlotNumber(uploadinglist.GetCount());
    }
}


bool CUploadQueue::AddUpNextClient(CUpDownClient* directadd, bool highPrioCheck) {
	//POSITION toadd = 0;
	//uint32	bestscore = 0;
	CUpDownClient* newclient = NULL;
	// select next client or use given client
	if (!directadd){
        newclient = FindBestClientInQueue((highPrioCheck == false));

		if(newclient != NULL) {
            if(highPrioCheck == true) {
		        POSITION lastpos = uploadinglist.GetTailPosition();

                CUpDownClient* lastClient = NULL;
                if(lastpos != NULL) {
                    lastClient = uploadinglist.GetAt(lastpos);
                }
                if(lastClient != NULL) {

                    if ((newclient->IsFriend() && newclient->GetFriendSlot()) == true && (lastClient->IsFriend() && lastClient->GetFriendSlot()) == false ||
                    	(
							(newclient->IsFriend() && newclient->GetFriendSlot()) == (lastClient->IsFriend() && lastClient->GetFriendSlot()) &&
                        	(
								newclient->MoreUpThanDown() == true && lastClient->MoreUpThanDown() == false ||//EastShare - added by AndCycle, PayBackFirst
								(newclient->MoreUpThanDown() == lastClient->MoreUpThanDown()) &&//EastShare - added by AndCycle, PayBackFirst
					 			(//EastShare - added by AndCycle, PayBackFirst
									(newclient->GetPowerShared() == true && lastClient->GetPowerShared() == false ||
                	  				newclient->GetPowerShared() == true && lastClient->GetPowerShared() == true && newclient->GetFilePrioAsNumber() > lastClient->GetFilePrioAsNumber()
								))//EastShare - added by AndCycle, PayBackFirst
							)
						)
						) {

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
            }

		    //RemoveFromWaitingQueue(toadd, true);
            RemoveFromWaitingQueue(newclient, true);
			theApp.emuledlg->transferwnd.ShowQueueCount(waitinglist.GetCount());
		}
	} else
		newclient = directadd;

    if(newclient == NULL) {
        return false;
	}

	if (IsDownloading(newclient)) {
		return false;
	}

	bool connectSuccess = true;
	// tell the client that we are now ready to upload
	if (!newclient->socket || !newclient->socket->IsConnected()){
		newclient->SetUploadState(US_CONNECTING);
		connectSuccess = newclient->TryToConnect(true);
	} else {
		Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
		newclient->socket->SendPacket(packet,true);
		newclient->SetUploadState(US_UPLOADING);
	}

    if(connectSuccess) {
	newclient->SetUpStartTime();
	newclient->ResetSessionUp();
	// khaos::kmod+ Show Compression by Tarod
	newclient->ResetCompressionGain();
	// khaos::kmod-

	InsertInUploadingList(newclient);
	
	if(newclient->GetQueueSessionUp() > 0) {
		// this client has already gotten a successfullupcount++ when it was early removed
		// negate that successfullupcount++ so we can give it a new one when this session ends
		// this prevents a client that gets put back first on queue, being counted twice in the
		// stats.
		successfullupcount--;
	}

	// statistic
	CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)newclient->GetUploadFileID());
	if (reqfile)
		reqfile->statistic.AddAccepted();
	//	}
		
	theApp.emuledlg->transferwnd.uploadlistctrl.AddClient(newclient);
        
        m_dwLastSlotAddTick = ::GetTickCount();

        return true;
    } else {
        return false;
    }
}

/**
 * Maintenance method for the uploading slots. It adds and removes clients to/from the
 * uploading list. It also makes sure that all the uploading slots' Sockets always have
 * enough packets in their queues, etc.
 *
 * This method is called aproximately once every 100 milliseconds.
 */
void CUploadQueue::Process() {
	theApp.sharedfiles->Publish();
	DWORD curTick = ::GetTickCount();

	ReSortUploadSlots();

	uint32 tempMaxActiveClients = 0;
	uint32 tempMaxActiveClientsShortTime = 0;
	POSITION activeClientsTickPos = activeClients_tick_list.GetHeadPosition();
	POSITION activeClientsListPos = activeClients_list.GetHeadPosition();
    while(activeClientsListPos != NULL) {
        DWORD activeClientsTickSnapshot = activeClients_tick_list.GetAt(activeClientsTickPos);
        uint32 activeClientsSnapshot = activeClients_list.GetAt(activeClientsListPos);

        if(activeClientsSnapshot > tempMaxActiveClients) {
            tempMaxActiveClients = activeClientsSnapshot;
		}

        if(activeClientsSnapshot > tempMaxActiveClientsShortTime && curTick - activeClientsTickSnapshot < 10 * 1000) {
            tempMaxActiveClientsShortTime = activeClientsSnapshot;
		}

        activeClients_tick_list.GetNext(activeClientsTickPos);
        activeClients_list.GetNext(activeClientsListPos);
	}
    m_MaxActiveClients = tempMaxActiveClients;
    m_MaxActiveClientsShortTime = tempMaxActiveClientsShortTime;

    uint32 wantedNumberOfTrickles = GetWantedNumberOfTrickleUploads();

    // How many slots should be open? Trickle slots included (at least 2 trickles, 30% of total, and the slots are expected to use UPLOAD_CLIENT_REALISTIC_AVERAGE_DATARATE in average, whichever number is largest)
    sint32 wantedNumberOfTotalUploads = max((uint32)(estadatarate/UPLOAD_CLIENT_DATARATE), m_MaxActiveClientsShortTime + MINNUMBEROFTRICKLEUPLOADS);
    wantedNumberOfTotalUploads = max(wantedNumberOfTotalUploads, m_MaxActiveClientsShortTime*1.3);

    // PENDING: Each 3 seconds
    if(curTick - m_dwLastCheckedForHighPrioClient >= 3*1000) {
        bool added = AddUpNextClient(NULL, true);
        if(added == false) {
            // set timer so we can wait a while
            m_dwLastCheckedForHighPrioClient = curTick;
		}else{
            // there might be another highprio client
            // don't set timer, so that we check next call as well
        }
    }

    POSITION ulpos = uploadinglist.GetHeadPosition();
    // The loop that feeds the upload slots with data.
    while (ulpos != NULL) {
        // Get the client. Note! Also updates ulpos as a side effect.
		CUpDownClient* cur_client = uploadinglist.GetNext(ulpos);
		cur_client->SendBlockData();
	}

	POSITION lastpos = uploadinglist.GetTailPosition();

    CUpDownClient* lastClient = NULL;
    if(lastpos != NULL) {
        lastClient = uploadinglist.GetAt(lastpos);
	}

    // Save number of active clients for statistics
    uint32 highestNumberOfFullyActivatedSlotsSinceLastCall = theApp.uploadBandwidthThrottler->GetHighestNumberOfFullyActivatedSlotsSinceLastCallAndReset();
	activeClients_list.AddTail(highestNumberOfFullyActivatedSlotsSinceLastCall);
    activeClients_tick_list.AddTail(curTick);

    if(uploadinglist.GetCount() > MIN_UP_CLIENTS_ALLOWED &&
       ((uint32)uploadinglist.GetCount() > m_MaxActiveClients+wantedNumberOfTrickles ||
        (uint32)uploadinglist.GetCount() > m_MaxActiveClientsShortTime+wantedNumberOfTrickles && AcceptNewClient(uploadinglist.GetCount()) == false)) {
        // we need to close a trickle slot and put it back first on the queue
        if(lastClient != NULL && lastClient->GetUpStartTimeDelay() > 3*1000) {

            // There's to many open uploads (propably due to the user changing
            // the upload limit to a lower value). Remove the last opened upload and put
            // it back on the waitinglist. When it is put back, it get
            // to keep its waiting time. This means it is likely to soon be
            // choosen for upload again.

            m_FirstRanOutOfSlotsTick = 0;

            //theApp.emuledlg->AddDebugLogLine(false, "%s: Ended upload since there are too many upload slots opened.", lastClient->GetUserName());
            // Remove from upload list.

            RemoveFromUploadQueue(lastClient, GetResString(IDS_REMULMANYSLOTS), true, true);

			//Morph Start - added by AndCycle, for zz prio system there are some situation need to take care with
			//require for equal chance for each file, accepted base
			CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)lastClient->GetUploadFileID());
			if (reqfile){
				reqfile->statistic.DelAccepted();
			}
			//Morph End - added by AndCycle, for zz prio system there are some situation need to take care with

		    // add to queue again.
            // the client is allowed to keep its waiting position in the queue, since it was pre-empted
            AddClientToQueue(lastClient,true, true);
        }
    } else if(theApp.lastCommonRouteFinder->AcceptNewClient() &&
              (
               highestNumberOfFullyActivatedSlotsSinceLastCall + wantedNumberOfTrickles > (uint32)uploadinglist.GetCount()
              )
             ) {
        // we have given all slots bandwidth this round, and couldn't have given them more.
        if(m_FirstRanOutOfSlotsTick == 0) {
            m_FirstRanOutOfSlotsTick = curTick;
        }

        // open an extra slot so that we always have enough trickle slots
        if(m_FirstRanOutOfSlotsTick != 0 && curTick-m_FirstRanOutOfSlotsTick > 500 &&
           AcceptNewClient(uploadinglist.GetCount()+1) && waitinglist.GetCount() > 0 /*&&
           (curTick - m_dwLastSlotAddTick > MINWAITBEFOREOPENANOTHERSLOTMS)*/
          ) {
            // There's not enough open uploads. Open another one.
            AddUpNextClient();

            m_FirstRanOutOfSlotsTick = 0;
		}
    } else {
        m_FirstRanOutOfSlotsTick = 0;
	}

    // Save used bandwidth for speed calculations
    uint64 sentBytes = theApp.uploadBandwidthThrottler->GetNumberOfSentBytesSinceLastCallAndReset();
	avarage_dr_list.AddTail(sentBytes);
    m_avarage_dr_sum += sentBytes;

    uint64 sentBytesExcludingOverhead = theApp.uploadBandwidthThrottler->GetNumberOfSentBytesExcludingOverheadSinceLastCallAndReset();
	m_AvarageUDRO_list.AddTail(sentBytesExcludingOverhead);
    sumavgUDRO += sentBytesExcludingOverhead;

    avarage_friend_dr_list.AddTail(theApp.stat_sessionSentBytesToFriend);

    // Save time beetween each speed snapshot
    avarage_tick_list.AddTail(curTick);

    // don't save more than 30 secs of data
    while(avarage_tick_list.GetCount() > 3 && ::GetTickCount()-avarage_tick_list.GetHead() > 30*1000) {
   	    m_avarage_dr_sum -= avarage_dr_list.RemoveHead();
        sumavgUDRO -= m_AvarageUDRO_list.RemoveHead();
        avarage_friend_dr_list.RemoveHead();
        avarage_tick_list.RemoveHead();
	}

    // Don't save more than three minutes of data about number of fully active clients
    while(curTick-activeClients_tick_list.GetHead() > 3*60*1000) {
        activeClients_tick_list.RemoveHead();
	    activeClients_list.RemoveHead();
	}
}

bool CUploadQueue::AcceptNewClient(uint32 numberOfUploads){
	// check if we can allow a new client to start downloading form us
	if (numberOfUploads < MIN_UP_CLIENTS_ALLOWED)
		return true;
	else if (numberOfUploads >= MAX_UP_CLIENTS_ALLOWED)
		return false;

	//now the final check
    	//now the final check
    if (numberOfUploads < (GetDatarate()/UPLOAD_CLIENT_DATARATE)+3 ||
        GetDatarate() < 2400*3 && numberOfUploads < GetDatarate()/UPLOAD_CLIENT_DATARATE)
			return true;
//	}
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

/*void CUploadQueue::UpdateBanCount(){
	int count=0;
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)){
		CUpDownClient* cur_client= waitinglist.GetAt(pos);
		if(cur_client->IsBanned())
			count++;
	}
	SetBanCount(count);
}*/

/**
 * Add a client to the waiting queue for uploads.
 *
 * @param client address of the client that should be added to the waiting queue
 *
 * @param bIgnoreTimelimit don't check timelimit to possibly ban the client.
 *
 * @param addInFirstPlace the client should be added first in queue, not last
 */
void CUploadQueue::AddClientToQueue(CUpDownClient* client, bool bIgnoreTimelimit, bool addInFirstPlace){
	//MORPH START - Removed by SiRoB, Anti-leecher feature
	////MORPH START - Added by IceCream, Anti-leecher feature
	//if (theApp.glob_prefs->GetEnableAntiLeecher())
	//	if (client->TestLeecher()) 
	//		client->BanLeecher();
	////MORPH END   - Added by IceCream, Anti-leecher feature
	//MORPH END   - Removed by SiRoB, Anti-leecher feature
	if(addInFirstPlace == false) {
		if (theApp.serverconnect->IsConnected() && theApp.serverconnect->IsLowID() //This may need to be changed with the Kad now being used.
			&& !theApp.serverconnect->IsLocalServer(client->GetServerIP(),client->GetServerPort())
			&& client->GetDownloadState() == DS_NONE && !client->IsFriend()
			&& GetWaitingUserCount() > 50)
			return;
		client->AddAskedCount();
		client->SetLastUpRequest();
		if (!bIgnoreTimelimit){
			client->AddRequestCount(client->GetUploadFileID());
		}
		if ( client->IsBanned() )
			return;
    }
	

	uint16 cSameIP = 0;
	// check for double
	POSITION pos1, pos2;
	for (pos1 = waitinglist.GetHeadPosition();( pos2 = pos1 ) != NULL;){
		waitinglist.GetNext(pos1);
		CUpDownClient* cur_client = waitinglist.GetAt(pos2);
		if (cur_client == client){	//already on queue
// VQB LowID Slot Patch, enhanced in ZZUL
			if (addInFirstPlace == false && client->HasLowID()&&
				client->m_bAddNextConnect && AcceptNewClient(uploadinglist.GetCount())) {

				RemoveFromWaitingQueue(client, true);
				AddUpNextClient(client);
				client->m_bAddNextConnect = false;
				//theApp.emuledlg->AddDebugLogLine(true,"Added Low ID User On Reconnect: " + (CString)client->GetUserName()); // VQB:  perhaps only add to debug log?
				return;
			} //else if(client->HasLowID())
			//theApp.emuledlg->AddDebugLogLine(true, "Skipped LowID User on Reconnect: " + (CString)client->GetUserName());
// VQB end
			client->SendRankingInfo();
			theApp.emuledlg->transferwnd.queuelistctrl.RefreshClient(client);
			return;			
		}
		else if ( client->Compare(cur_client) ) {
			theApp.clientlist->AddTrackClient(client); // in any case keep track of this client

			// another client with same ip:port or hash
			// this happens only in rare cases, because same userhash / ip:ports are assigned to the right client on connecting in most cases
			if (cur_client->credits != NULL && cur_client->credits->GetCurrentIdentState(cur_client->GetIP()) == IS_IDENTIFIED){
				//cur_client has a valid secure hash, don't remove him
				AddDebugLogLine(false,CString(GetResString(IDS_SAMEUSERHASH)),client->GetUserName(),cur_client->GetUserName(),client->GetUserName() );
					return;
				}
			if (client->credits != NULL && client->credits->GetCurrentIdentState(client->GetIP()) == IS_IDENTIFIED){
				//client has a valid secure hash, add him remove other one
				AddDebugLogLine(false,CString(GetResString(IDS_SAMEUSERHASH)),client->GetUserName(),cur_client->GetUserName(),cur_client->GetUserName() );
				RemoveFromWaitingQueue(pos2,true);	
				if (!cur_client->socket){
					if(cur_client->Disconnected("Socket is NULL. 3")){
						delete cur_client;
					}
				}
			}
			else{
				// remove both since we dont know who the bad on is
				AddDebugLogLine(false,CString(GetResString(IDS_SAMEUSERHASH)),client->GetUserName(),cur_client->GetUserName(),"Both" );
				RemoveFromWaitingQueue(pos2,true);	
				if (!cur_client->socket){
					if(cur_client->Disconnected("Socket is NULL. 2")){
						delete cur_client;
					}
				}
			return;
			}
		}
		else if (client->GetIP() == cur_client->GetIP()){
			// same IP, different port, different userhash
			cSameIP++;
		}
	}
	if (cSameIP >= 3){
		// do not accept more than 3 clients from the same IP
		DEBUG_ONLY(AddDebugLogLine(false,"%s's (%s) request to enter the queue was rejected, because of too many clients with the same IP",client->GetUserName(), client->GetFullIP() ));
		return;
	}
	else if (theApp.clientlist->GetClientsFromIP(client->GetIP()) >= 3){
		DEBUG_ONLY(AddDebugLogLine(false,"%s's (%s) request to enter the queue was rejected, because of too many clients with the same IP (found in TrackedClientsList)",client->GetUserName(), client->GetFullIP() ));
		return;
	}
	// done

	// Add clients server to list.
	if (theApp.glob_prefs->AddServersFromClient() && client->GetServerIP() && client->GetServerPort()){
		in_addr host;
		host.S_un.S_addr = client->GetServerIP();
		CServer* srv = new CServer(client->GetServerPort(), inet_ntoa(host));
		srv->SetListName(srv->GetAddress());

		if (!theApp.emuledlg->serverwnd.serverlistctrl.AddServer(srv, true))
			delete srv;
	}

    if(addInFirstPlace == false) {
		// statistic values
		CKnownFile* reqfile = theApp.sharedfiles->GetFileByID((uchar*)client->GetUploadFileID());
		if (reqfile)
			reqfile->statistic.AddRequest();
// <<---- start of change ---->

		// better ways to cap the list
		uint32 softQueueLimit;
		uint32 hardQueueLimit;

		// these proportions could be tweaked. take 20 precent of queue size to buffer new client
		softQueueLimit = theApp.glob_prefs->GetQueueSize() - theApp.glob_prefs->GetQueueSize()/5;
		hardQueueLimit = theApp.glob_prefs->GetQueueSize();

		if ((uint32)waitinglist.GetCount() > hardQueueLimit){
			return;
		}
		else if((uint32)waitinglist.GetCount() > softQueueLimit){// soft queue limit is reached

			if (client->IsFriend() && client->GetFriendSlot() == false && // client is not a friend with friend slot
				client->MoreUpThanDown() == false && // client don't need Pay Back First //Morph - Added by AndCycle, Pay Back First
				client->GetPowerShared() == false && // client don't want powershared file //Morph - Added by AndCycle
					(
						client->GetCombinedFilePrioAndCredit() < GetAverageCombinedFilePrioAndCredit() && theApp.glob_prefs->GetEqualChanceForEachFileMode() == ECFEF_DISABLE ||
						client->GetCombinedFilePrioAndCredit() > GetAverageCombinedFilePrioAndCredit() && theApp.glob_prefs->GetEqualChanceForEachFileMode() != ECFEF_DISABLE//Morph - added by AndCycle, Equal Chance For Each File
					)// and client has lower credits/wants lower prio file than average client in queue
				) {
				// then block client from getting on queue
				return;
			}
		}
// <<---- end of change ---->

		if (client->IsDownloading()){
			// he's already downloading and wants probably only another file
			Packet* packet = new Packet(OP_ACCEPTUPLOADREQ,0);
			theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
			client->socket->SendPacket(packet,true);
			return;
		}

        client->ResetQueueSessionUp();
	}
	waitinglist.AddTail(client);
	client->SetUploadState(US_ONUPLOADQUEUE);

    // Add client to waiting list. If addInFirstPlace is set, client should not have its waiting time resetted
    theApp.emuledlg->transferwnd.queuelistctrl.AddClient(client, (addInFirstPlace == false));
	theApp.emuledlg->transferwnd.ShowQueueCount(waitinglist.GetCount());
    client->SendRankingInfo();
}

float CUploadQueue::GetAverageCombinedFilePrioAndCredit() {
    DWORD curTick = ::GetTickCount();

    if (curTick - m_dwLastCalculatedAverageCombinedFilePrioAndCredit > 5*1000) {
        m_dwLastCalculatedAverageCombinedFilePrioAndCredit = curTick;

        POSITION pos1, pos2;

        // TODO: is there a risk of overflow? I don't think so...
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
bool CUploadQueue::RemoveFromUploadQueue(CUpDownClient* client, CString reason, bool updatewindow, bool earlyabort){
	theApp.clientlist->AddTrackClient(client); // Keep track of this client
    uint32 posCounter = 0;
	for (POSITION pos = uploadinglist.GetHeadPosition();pos != 0;uploadinglist.GetNext(pos)){
		if (client == uploadinglist.GetAt(pos)){
			if (updatewindow)
				theApp.emuledlg->transferwnd.uploadlistctrl.RemoveClient(uploadinglist.GetAt(pos));
            
            if(!reason || reason.Compare("") == 0) {
                CString tempReason = GetResString(IDS_REMULNOREASON);
                reason = tempReason;
            }
            AddDebugLogLine(true,GetResString(IDS_REMULREASON), client->GetUserName(), reason);
			uploadinglist.RemoveAt(pos);
            theApp.uploadBandwidthThrottler->RemoveFromStandardList(client->socket);
			if(client->GetQueueSessionUp()){
				successfullupcount++;

                if(client->GetSessionUp()) {
					//wistily
					uint32 tempUpStartTimeDelay=client->GetUpStartTimeDelay();
					client->Add2UpTotalTime(tempUpStartTimeDelay);
					client->m_nAvUpDatarate= client->GetTransferedUp()/(client->GetUpTotalTime()/1000);
					/*totaluploadtime += client->GetUpStartTimeDelay()/1000;*/
					totaluploadtime += tempUpStartTimeDelay/1000;
					//MORPH START - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
					if (theApp.clientcredits->IsSaveUploadQueueWaitTime())
						client->Credits()->ClearUploadQueueWaitTime();	// Moonlight: SUQWT
					//MORPH END   - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
					//wistily stop
                    totalCompletedBytes += client->GetSessionUp();
                }
			    //} else if(client->HasBlocks() || client->GetUploadState() != US_UPLOADING) {
            } else if(earlyabort == false){
				failedupcount++;
				//MORPH START - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
				if (theApp.clientcredits->IsSaveUploadQueueWaitTime())
					client->Credits()->SaveUploadQueueWaitTime();	// Moonlight: SUQWT//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
				//MORPH END   - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
			}
            CKnownFile* requestedFile = theApp.sharedfiles->GetFileByID(client->GetUploadFileID());

            if(requestedFile != NULL) {
                //MORPH START - Changed by SiRoB, HotFix Due to Complete Source Feature
				//tempreqfile->NewAvailPartsInfo();
				if(!requestedFile->IsPartFile())
					requestedFile->NewAvailPartsInfo();
				else
					((CPartFile*)requestedFile)->NewSrcPartsInfo();
				//MORPH END   - Changed by SiRoB, HotFix Due to Complete Source Feature
            }

			client->SetUploadState(US_NONE);
			client->ClearUploadBlockRequests(/*!earlyabort*/);
			return true;
		}
        posCounter++;
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
			theApp.emuledlg->transferwnd.ShowQueueCount(waitinglist.GetCount());
		return true;
	}
	else
		return false;
}

// Moonlight: SUQWT: Save queue wait time and clear wait start times before removing from queue.//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
void CUploadQueue::RemoveFromWaitingQueue(POSITION pos, bool updatewindow){	
	CUpDownClient* todelete = waitinglist.GetAt(pos);
	waitinglist.RemoveAt(pos);
	if (updatewindow)
		theApp.emuledlg->transferwnd.queuelistctrl.RemoveClient(todelete);
	//MORPH START - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	if (theApp.clientcredits->IsSaveUploadQueueWaitTime()){
		todelete->Credits()->SaveUploadQueueWaitTime();	// Moonlight: SUQWT
		todelete->Credits()->ClearWaitStartTime();		// Moonlight: SUQWT
	}
	//MORPH END   - Added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	todelete->SetUploadState(US_NONE);
}

/*
bool CUploadQueue::CheckForTimeOver(CUpDownClient* client){
	if( client->GetUpStartTimeDelay() > 3600000 ){ // Try to keep the clients from downloading for ever.
		AddDebugLogLine(false, "%s: Upload session ended due to excessive time.", client->GetUserName());
		return true;
	}


	if (!theApp.glob_prefs->TransferFullChunks()){

		// Cache current client score
		const uint32 score = client->GetScore(true, true);

		// Check if another client has a bigger score
		for(POSITION pos = waitinglist.GetHeadPosition(); pos != 0; )
			if(score < waitinglist.GetNext(pos)->GetScore(true, false)){
				//theApp.emuledlg->AddDebugLogLine(false, "%s: Upload session ended due to score.", client->GetUserName());
				return true;
			}
	}
	else{
	// more than one chunk allowed if we use the score system

	// For some reason, some clients can continue to download after a chunk size.
	// Are they redownloading the same chunk over and over????
	if( client->GetSessionUp() > 10485760 ){
		AddDebugLogLine(false, "%s: Upload session ended due to excessive transfered amount.", client->GetUserName());
		return true;
	}

	}
	return false;
}
*/

void CUploadQueue::DeleteAll(){
	waitinglist.RemoveAll();
	uploadinglist.RemoveAll();
}

uint16 CUploadQueue::GetWaitingPosition(CUpDownClient* client){
	if (!IsOnUploadQueue(client))
		return 0;
	
	//MORPH START - Moved & Changed by SiRoB, This is a better place
	//MORPH START - Added by IceCream, Anti-leecher feature
	if (client->IsLeecher())
		return GetRandRange(25,75); // nRank = 75;
	//MORPH END   - Added by IceCream, Anti-leecher feature
	//MORPH END   - Moved & Changed by SiRoB, This is a better place 
	
	uint16 rank = 1;
	uint32 myscore = client->GetScore(false);
	for (POSITION pos = waitinglist.GetHeadPosition();pos != 0;waitinglist.GetNext(pos)){
		//MORPH START - Added by SiRoB, ZZ Upload System
		//if (waitinglist.GetAt(pos)->GetScore(false) > myscore)
		CUpDownClient* compareClient = waitinglist.GetAt(pos);
		if (RightClientIsBetter(client, myscore, compareClient, compareClient->GetScore(false)))
		//MORPH END - Added by SiRoB, ZZ Upload System
			rank++;
	}
	return rank;
}
//MORPH - Removed by SiRoB, ZZ UPload System 20030818-1923
/*void CUploadQueue::CompUpDatarateOverhead(){
	// Patch by BadWolf - Accurate datarate Calculation
	TransferredData newitem = {m_nUpDataRateMSOverhead,::GetTickCount()};
	this->m_AvarageUDRO_list.AddTail(newitem);
	sumavgUDRO += m_nUpDataRateMSOverhead;
	while ((float)(m_AvarageUDRO_list.GetTail().timestamp - m_AvarageUDRO_list.GetHead().timestamp)  > MAXAVERAGETIME)
		sumavgUDRO -= m_AvarageUDRO_list.RemoveHead().datalen;
	m_nUpDataRateMSOverhead = 0;
	if(m_AvarageUDRO_list.GetCount() > 10){
		DWORD dwDuration = m_AvarageUDRO_list.GetTail().timestamp - m_AvarageUDRO_list.GetHead().timestamp;
		if (dwDuration)
			m_nUpDatarateOverhead = 1000 * sumavgUDRO / dwDuration;
	}
	else
		m_nUpDatarateOverhead = 0;
	return;
	// END Patch by BadWolf	
}*/

VOID CALLBACK CUploadQueue::UploadTimer(HWND hwnd, UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	try
	{
		// Barry - Don't do anything if the app is shutting down - can cause unhandled exceptions
		if (!theApp.emuledlg->IsRunning())
			return;
		//MORPH START - Added by SiRoB, ZZ UPload System 20030818-1923
        // Elandal:ThreadSafeLogging -->
        // other threads may have queued up log lines. This prints them.
        theApp.emuledlg->HandleDebugLogQueue();
        // Elandal: ThreadSafeLogging <--

        // Send allowed data rate to UploadBandWidthThrottler in a thread safe way
        //MOPRH START - Modified by SiRoB
		theApp.lastCommonRouteFinder->SetPrefs(theApp.glob_prefs->IsDynUpEnabled(), theApp.uploadqueue->GetDatarate(), theApp.glob_prefs->GetMinUpload()*1024, (theApp.glob_prefs->IsSUCDoesWork())?theApp.uploadqueue->GetMaxVUR():theApp.glob_prefs->GetMaxUpload()*1024, (theApp.glob_prefs->GetDynUpPingTolerance() > 100)?((theApp.glob_prefs->GetDynUpPingTolerance()-100)/100.0f):0, theApp.glob_prefs->GetDynUpGoingUpDivider(), theApp.glob_prefs->GetDynUpGoingDownDivider(), theApp.glob_prefs->GetDynUpNumberOfPings(), 5);
		//MOPRH END   - Modified by SiRoB
		//MORPH END    - Added by SiRoB, ZZ UPload System 20030818-1923

		theApp.uploadqueue->Process();
		theApp.downloadqueue->Process();
		//theApp.uploadqueue->CompUpDatarateOverhead(); //MORPH - Removed by SiRoB, ZZ UPload System 20030818-1923
		theApp.downloadqueue->CompDownDatarateOverhead();
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

			if( theApp.serverconnect->IsConnecting() && !theApp.serverconnect->IsSingleConnect() )
				theApp.serverconnect->TryAnotherConnectionrequest();

			theApp.emuledlg->statisticswnd.UpdateConnectionsStatus();
			if (theApp.glob_prefs->WatchClipboard4ED2KLinks())
				theApp.emuledlg->searchwnd->SearchClipBoard();		

			if (theApp.serverconnect->IsConnecting())
				theApp.serverconnect->CheckForTimeout();

			// -khaos--+++> Update connection stats...
			iupdateconnstats++;
			// 2 seconds
			if (iupdateconnstats>=2) {
				iupdateconnstats=0;
				//theApp.emuledlg->statisticswnd.UpdateConnectionStats((float)theApp.uploadqueue->GetDatarate()/1024, (float)theApp.downloadqueue->GetDatarate()/1024);
				theApp.emuledlg->statisticswnd.UpdateConnectionStats((float)theApp.uploadqueue->GetDatarate()/1024, (float)theApp.downloadqueue->GetDatarate()/1024);
			}
			// <-----khaos-

			// display graphs
			if (theApp.glob_prefs->GetTrafficOMeterInterval()>0) {
				igraph++;

				if (igraph >= (uint32)(theApp.glob_prefs->GetTrafficOMeterInterval()) ) {
					igraph=0;
					//theApp.emuledlg->statisticswnd.SetCurrentRate((float)(theApp.uploadqueue->Getavgupload()/theApp.uploadqueue->Getavg())/1024,(float)(theApp.uploadqueue->Getavgdownload()/theApp.uploadqueue->Getavg())/1024);
					theApp.emuledlg->statisticswnd.SetCurrentRate((float)(theApp.uploadqueue->GetDatarate())/1024,(float)(theApp.downloadqueue->GetDatarate())/1024, (float)(theApp.uploadqueue->GetDatarate()-theApp.uploadqueue->GetToNetworkDatarate())/1024, (float)(theApp.uploadqueue->GetUpDatarateOverhead())/1024);
					//theApp.uploadqueue->Zeroavg();
				}
			}
			if (theApp.emuledlg->activewnd == &theApp.emuledlg->statisticswnd && theApp.emuledlg->IsWindowVisible() )  {
				// display stats
				if (theApp.glob_prefs->GetStatsInterval()>0) {
					istats++;

					if (istats >= (uint32)(theApp.glob_prefs->GetStatsInterval()) ) {
						istats=0;
						theApp.emuledlg->statisticswnd.ShowStatistics();
					}
				}
			}
			//save rates every second
			theApp.emuledlg->statisticswnd.RecordRate();
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
					if (!AfxCheckMemory()) AfxDebugBreak();
				#endif

				sec = 0;
				theApp.listensocket->Process();
				theApp.OnlineSig(); // Added By Bouc7 
				theApp.emuledlg->ShowTransferRate();
				
				// update cat-titles with downloadinfos only when needed
				if (theApp.glob_prefs->ShowCatTabInfos() && 
					theApp.emuledlg->activewnd==&theApp.emuledlg->transferwnd && 
					theApp.emuledlg->IsWindowVisible()) 
						theApp.emuledlg->transferwnd.UpdateCatTabTitles();
				
				if (theApp.glob_prefs->IsSchedulerEnabled()) theApp.scheduler->Check();
			}

			statsave++;
			// -khaos--+++>
			// 60 seconds
			if (statsave>=60) {
				// Time to save our cumulative statistics.
				statsave=0;
				
				// CPreferences::SaveStats() in Preferences.cpp
				// This function does NOT update the tree!
				theApp.glob_prefs->SaveStats();
			// <-----khaos-

				theApp.serverconnect->KeepConnectionAlive();
			}
		}
	}
	CATCH_DFLT_EXCEPTIONS("CUploadQueue::UploadTimer")
}

CUpDownClient* CUploadQueue::GetNextClient(CUpDownClient* lastclient){
	if (waitinglist.IsEmpty())
		return 0;
	if (!lastclient)
		return waitinglist.GetHead();
	POSITION pos = waitinglist.Find(lastclient);
	if (!pos){
		TRACE("Error: CServerList::GetNextClient");
		return waitinglist.GetHead();
	}
	waitinglist.GetNext(pos);
	if (!pos)
		return NULL;
	else
		return waitinglist.GetAt(pos);
}

void CUploadQueue::FindSourcesForFileById(CTypedPtrList<CPtrList, CUpDownClient*>* srclist, const uchar* filehash) {
	POSITION pos;
	
	pos = uploadinglist.GetHeadPosition();
	while(pos) {
		CUpDownClient *potential = uploadinglist.GetNext(pos);
		if(md4cmp(potential->GetUploadFileID(), filehash) == 0)
			srclist->AddTail(potential);
	}

	pos = waitinglist.GetHeadPosition();
	while(pos) {
		CUpDownClient *potential = waitinglist.GetNext(pos);
		if(md4cmp(potential->GetUploadFileID(), filehash) == 0)
			srclist->AddTail(potential);
	}
}

//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
void CUploadQueue::UpdateDatarates() {
    // Calculate average datarate
    if(::GetTickCount()-m_lastCalculatedDataRateTick > 500) {
        m_lastCalculatedDataRateTick = ::GetTickCount();

        if(avarage_dr_list.GetSize() >= 2 && m_AvarageUDRO_list.GetSize() >= 2 && (avarage_tick_list.GetTail() > avarage_tick_list.GetHead())) {
	        datarate = ((m_avarage_dr_sum-avarage_dr_list.GetHead())*1000) / (avarage_tick_list.GetTail()-avarage_tick_list.GetHead());
            m_nUpDatarateOverhead = ((sumavgUDRO-m_AvarageUDRO_list.GetHead())*1000) / (avarage_tick_list.GetTail()-avarage_tick_list.GetHead());
            friendDatarate = ((avarage_friend_dr_list.GetTail()-avarage_friend_dr_list.GetHead())*1000) / (avarage_tick_list.GetTail()-avarage_tick_list.GetHead());
        } else {
            datarate = 0;
            m_nUpDatarateOverhead = 0;
            friendDatarate = 0;
        }
    }
}

uint32 CUploadQueue::GetDatarate() {
    UpdateDatarates();
    return datarate;
}

uint32 CUploadQueue::GetUpDatarateOverhead() {
    UpdateDatarates();
    return m_nUpDatarateOverhead;
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

    if(minNumber < 2 && theApp.glob_prefs->GetMaxUpload() >= 4) {
        minNumber = 2;
    } else if(minNumber < 1 && theApp.glob_prefs->GetMaxUpload() >= 2) {
        minNumber = 1;
    }

    return max(((uint32)uploadinglist.GetCount())*0.3, minNumber);
}

/**
 * Resort the upload slots, so they are kept sorted even if file priorities
 * are changed by the user, friend slot is turned on/off, etc
 */
void CUploadQueue::ReSortUploadSlots(bool force) {
    DWORD curtick = ::GetTickCount();
    if(force ||  curtick - m_dwLastResortedUploadSlots >= 3*1000) {
        m_dwLastResortedUploadSlots = curtick;

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
    }
}
//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
