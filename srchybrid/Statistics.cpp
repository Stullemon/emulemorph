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
#include "Statistics.h"
#include "UploadQueue.h"
#include "Preferences.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
extern _CRT_ALLOC_HOOK g_pfnPrevCrtAllocHook;
#endif


CStatistics::CStatistics()
{
	maxDown=0;
	maxDownavg=0;
	maxcumDown =			thePrefs.GetConnMaxDownRate();
	cumUpavg =				thePrefs.GetConnAvgUpRate();
	maxcumDownavg =			thePrefs.GetConnMaxAvgDownRate();
	cumDownavg =			thePrefs.GetConnAvgDownRate();
	maxcumUpavg =			thePrefs.GetConnMaxAvgUpRate();
	maxcumUp =				thePrefs.GetConnMaxUpRate();
	maxUp =					0;
	maxUpavg =				0;
	rateDown =				0;
	rateUp =				0;
	timeTransfers =			0;
	timeDownloads =			0;
	timeUploads =			0;
	start_timeTransfers =	0;
	start_timeDownloads =	0;
	start_timeUploads =		0;
	time_thisTransfer =		0;
	time_thisDownload =		0;
	time_thisUpload =		0;
	timeServerDuration =	0;
	time_thisServerDuration=0;
}

CStatistics::~CStatistics()
{
}


// -khaos--+++> This function is going to basically calculate and save a bunch of averages.
//				I made a seperate funtion so that it would always run instead of having
//				the averages not be calculated if the graphs are disabled (Which is bad!).
void CStatistics::UpdateConnectionStats(float uploadrate, float downloadrate){
	rateUp = uploadrate;
	rateDown = downloadrate;

	if (maxUp<uploadrate) maxUp=uploadrate;
	if (maxcumUp<maxUp) {
		maxcumUp=maxUp;
		thePrefs.Add2ConnMaxUpRate(maxcumUp);
	}

	if (maxDown<downloadrate) maxDown=downloadrate; // MOVED from SetCurrentRate!
	if (maxcumDown<maxDown) {
		maxcumDown=maxDown;
		thePrefs.Add2ConnMaxDownRate(maxcumDown);
	}

	cumDownavg = GetAvgDownloadRate(AVG_TOTAL);
	if (maxcumDownavg<cumDownavg) {
		maxcumDownavg=cumDownavg;
		thePrefs.Add2ConnMaxAvgDownRate(maxcumDownavg);
	}

	cumUpavg = GetAvgUploadRate(AVG_TOTAL);
	if (maxcumUpavg<cumUpavg) {
		maxcumUpavg=cumUpavg;
		thePrefs.Add2ConnMaxAvgUpRate(maxcumUpavg);
	}
	

	// Transfer Times (Increment Session)
	if (uploadrate > 0 || downloadrate > 0) {
		if (start_timeTransfers == 0) start_timeTransfers = GetTickCount();
		else time_thisTransfer = (GetTickCount() - start_timeTransfers) / 1000;

		if (uploadrate > 0) {
			if (start_timeUploads == 0) start_timeUploads = GetTickCount();
			else time_thisUpload = (GetTickCount() - start_timeUploads) / 1000;
		}

		if (downloadrate > 0) {
			if (start_timeDownloads == 0) start_timeDownloads = GetTickCount();
			else time_thisDownload = (GetTickCount() - start_timeDownloads) / 1000;
		}
	}

	if (uploadrate == 0 && downloadrate == 0 && (time_thisTransfer > 0 || start_timeTransfers > 0)) {
		timeTransfers += time_thisTransfer;
		time_thisTransfer = 0;
		start_timeTransfers = 0;
	}

	if (uploadrate == 0 && (time_thisUpload > 0 || start_timeUploads > 0)) {
		timeUploads += time_thisUpload;
		time_thisUpload = 0;
		start_timeUploads = 0;
	}

	if (downloadrate == 0 && (time_thisDownload > 0 || start_timeDownloads > 0)) {
		timeDownloads += time_thisDownload;
		time_thisDownload = 0;
		start_timeDownloads = 0;
	}

	// Server Durations
	if (theApp.stat_serverConnectTime==0) theApp.statistics->time_thisServerDuration = 0;
	else time_thisServerDuration = ( GetTickCount() - theApp.stat_serverConnectTime ) / 1000;
}
// <-----khaos-

void CStatistics::RecordRate() {
	
	if (theApp.stat_transferStarttime==0) return;

	// every second By BadWolf - Accurate datarate Calculation
	uint32 stick =  ::GetTickCount();
	TransferredData newitemUP = {theApp.stat_sessionSentBytes, stick};
	TransferredData newitemDN = {theApp.stat_sessionReceivedBytes, stick};
	//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	TransferredData newitemFriends = {theApp.stat_sessionSentBytesToFriend, stick};
	//MORPH END   - Added by SiRoB, ZZ Upload system 20030818-1923

	downrateHistory.push_front(newitemDN);
	uprateHistory.push_front(newitemUP);
	//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	uprateHistoryFriends.push_front(newitemFriends);
	//MORPH END   - Added by SiRoB, ZZ Upload system 20030818-1923

	// limit to maxmins
	int iAverageSeconds = thePrefs.GetStatsAverageMinutes()*60;
	while ((float)(downrateHistory.front().timestamp - downrateHistory.back().timestamp) / 1000.0 > iAverageSeconds)
		downrateHistory.pop_back();
	while ((float)(uprateHistory.front().timestamp - uprateHistory.back().timestamp) / 1000.0 > iAverageSeconds)
		uprateHistory.pop_back();
	//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	while ((float)(uprateHistoryFriends.front().timestamp - uprateHistoryFriends.back().timestamp) / 1000.0 > iAverageSeconds)
		uprateHistoryFriends.pop_back();
	//MORPH END   - Added by SiRoB, ZZ Upload system 20030818-1923
	//theApp.emuledlg->ShowTransferRate(false);
}

// Changed these two functions (khaos)...
float CStatistics::GetAvgDownloadRate(int averageType) {
	DWORD running;
	switch(averageType) {
		case AVG_SESSION:
			if (theApp.stat_transferStarttime == 0) return 0;            
			running=(GetTickCount()-theApp.stat_transferStarttime)/1000;
			if (running<5) return 0;
			return (float) (theApp.stat_sessionReceivedBytes/1024) / running;

		case AVG_TOTAL:
			if (theApp.stat_transferStarttime == 0) return thePrefs.GetConnAvgDownRate();
			running=(GetTickCount()-theApp.stat_transferStarttime)/1000;
			if (running<5) return thePrefs.GetConnAvgDownRate();
			return (float) ((( (float) (theApp.stat_sessionReceivedBytes/1024) / running ) + thePrefs.GetConnAvgDownRate() ) / 2 );

		default:
			// By BadWolf - Accurate datarate Calculation
			if (downrateHistory.size()==0) return 0;
			float deltat = (float)(downrateHistory.front().timestamp - downrateHistory.back().timestamp) / 1000.0;
			if (deltat > 0.0) 
				return (float)((float)(downrateHistory.front().datalen-downrateHistory.back().datalen) / deltat)/1024;
			else
				return 0;
			// END By BadWolf - Accurate datarate Calculation
	}
}

float CStatistics::GetAvgUploadRate(int averageType) {
	DWORD running;
	switch(averageType) {
		case AVG_SESSION:
			if (theApp.stat_transferStarttime == 0) return 0;            
			running=(GetTickCount()-theApp.stat_transferStarttime)/1000;
			if (running<5) return 0;
			return (float) (theApp.stat_sessionSentBytes/1024) / running;

		case AVG_TOTAL:
			if (theApp.stat_transferStarttime == 0) return thePrefs.GetConnAvgUpRate();
			running=(GetTickCount()-theApp.stat_transferStarttime)/1000;
			if (running<5) return thePrefs.GetConnAvgUpRate();
			return (float) ((( (float) (theApp.stat_sessionSentBytes/1024) / running ) + thePrefs.GetConnAvgUpRate() ) / 2 );

		default:
			// By BadWolf - Accurate datarate Calculation
			if (uprateHistory.size()==0) return 0;
			float deltat = (float)(uprateHistory.front().timestamp - uprateHistory.back().timestamp) / 1000.0;
			if (deltat > 0.0) 
				return (float)((float)(uprateHistory.front().datalen-uprateHistory.back().datalen) / deltat)/1024;
			else
				return 0;
			// END By BadWolf - Accurate datarate Calculation
	}
}
// <-----khaos-
