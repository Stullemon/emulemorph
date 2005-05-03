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
#include "Statistics.h"
#include "Preferences.h"
#include "Opcodes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

#ifdef _DEBUG
extern _CRT_ALLOC_HOOK g_pfnPrevCrtAllocHook;
#endif

//MORPH START - Removed by SiRoB, Changed by SiRoB, Better datarate mesurement for low and high speed
/*
#define MAXAVERAGETIME			SEC2MS(40) //millisecs
*/
//MORPH END   - Changed by SiRoB, Changed by SiRoB, Better datarate mesurement for low and high speed

///////////////////////////////////////////////////////////////////////////////
// CStatistics

CStatistics theStats;

float	CStatistics::maxDown;
float	CStatistics::maxDownavg;
float	CStatistics::cumDownavg;
float	CStatistics::maxcumDownavg;
float	CStatistics::maxcumDown;
float	CStatistics::cumUpavg;
float	CStatistics::maxcumUpavg;
float	CStatistics::maxcumUp;
float	CStatistics::maxUp;
float	CStatistics::maxUpavg;
float	CStatistics::rateDown;
float	CStatistics::rateUp;
uint32	CStatistics::timeTransfers;
uint32	CStatistics::timeDownloads;
uint32	CStatistics::timeUploads;
uint32	CStatistics::start_timeTransfers;
uint32	CStatistics::start_timeDownloads;
uint32	CStatistics::start_timeUploads;
uint32	CStatistics::time_thisTransfer;
uint32	CStatistics::time_thisDownload;
uint32	CStatistics::time_thisUpload;
uint32	CStatistics::timeServerDuration;
uint32	CStatistics::time_thisServerDuration;
uint32	CStatistics::m_nDownDatarateOverhead;
uint32	CStatistics::m_nDownDataRateMSOverhead;
uint64	CStatistics::m_nDownDataOverheadSourceExchange;
uint64	CStatistics::m_nDownDataOverheadSourceExchangePackets;
uint64	CStatistics::m_nDownDataOverheadFileRequest;
uint64	CStatistics::m_nDownDataOverheadFileRequestPackets;
uint64	CStatistics::m_nDownDataOverheadServer;
uint64	CStatistics::m_nDownDataOverheadServerPackets;
uint64	CStatistics::m_nDownDataOverheadKad;
uint64	CStatistics::m_nDownDataOverheadKadPackets;
uint64	CStatistics::m_nDownDataOverheadOther;
uint64	CStatistics::m_nDownDataOverheadOtherPackets;
uint32	CStatistics::m_nUpDatarateOverhead;
uint32	CStatistics::m_nUpDataRateMSOverhead;
uint64	CStatistics::m_nUpDataOverheadSourceExchange;
uint64	CStatistics::m_nUpDataOverheadSourceExchangePackets;
uint64	CStatistics::m_nUpDataOverheadFileRequest;
uint64	CStatistics::m_nUpDataOverheadFileRequestPackets;
uint64	CStatistics::m_nUpDataOverheadServer;
uint64	CStatistics::m_nUpDataOverheadServerPackets;
uint64	CStatistics::m_nUpDataOverheadKad;
uint64	CStatistics::m_nUpDataOverheadKadPackets;
uint64	CStatistics::m_nUpDataOverheadOther;
uint64	CStatistics::m_nUpDataOverheadOtherPackets;
uint32	CStatistics::m_sumavgDDRO;
uint32	CStatistics::m_sumavgUDRO;
//MORPH START - Added by SiRoB, Changed by SiRoB, Better datarate mesurement for low and high speed
DWORD	CStatistics::m_AvarageDDROPreviousAddedTimestamp;
DWORD	CStatistics::m_AvarageUDROPreviousAddedTimestamp;
//MORPH END   - Added by SiRoB, Changed by SiRoB, Better datarate mesurement for low and high speed

uint64	CStatistics::sessionReceivedBytes;
uint64	CStatistics::sessionSentBytes;
uint64	CStatistics::sessionSentBytesToFriend;
uint16	CStatistics::reconnects;
DWORD	CStatistics::transferStarttime;
DWORD	CStatistics::serverConnectTime;
uint32	CStatistics::filteredclients;
DWORD	CStatistics::starttime;
uint32	CStatistics::leecherclients; //Added by SiRoB

CStatistics::CStatistics()
{
	maxDown =				0;
	maxDownavg =			0;
	maxcumDown =			0;
	cumUpavg =				0;
	maxcumDownavg =			0;
	cumDownavg =			0;
	maxcumUpavg =			0;
	maxcumUp =				0;
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

	sessionReceivedBytes=0;
	sessionSentBytes=0;
    sessionSentBytesToFriend=0;
	reconnects=0;
	transferStarttime=0;
	serverConnectTime=0;
	filteredclients=0;
	leecherclients=0; //MORPH - Added by SiRoB
	starttime=0;
	

	m_nDownDataRateMSOverhead = 0;
	m_nDownDatarateOverhead = 0;
	m_nDownDataOverheadSourceExchange = 0;
	m_nDownDataOverheadSourceExchangePackets = 0;
	m_nDownDataOverheadFileRequest = 0;
	m_nDownDataOverheadFileRequestPackets = 0;
	m_nDownDataOverheadServer = 0;
	m_nDownDataOverheadServerPackets = 0;
	m_nDownDataOverheadKad = 0;
	m_nDownDataOverheadKadPackets = 0;
	m_nDownDataOverheadOther = 0;
	m_nDownDataOverheadOtherPackets = 0;
	m_sumavgDDRO = 0;

	m_nUpDataRateMSOverhead = 0;
	m_nUpDatarateOverhead = 0;
	m_nUpDataOverheadSourceExchange = 0;
	m_nUpDataOverheadSourceExchangePackets = 0;
	m_nUpDataOverheadFileRequest = 0;
	m_nUpDataOverheadFileRequestPackets = 0;
	m_nUpDataOverheadServer = 0;
	m_nUpDataOverheadServerPackets = 0;
	m_nUpDataOverheadKad = 0;
	m_nUpDataOverheadKadPackets = 0;
	m_nUpDataOverheadOther = 0;
	m_nUpDataOverheadOtherPackets = 0;
	m_sumavgUDRO = 0;
	m_AvarageUDROPreviousAddedTimestamp = GetTickCount(); //MORPH - Added by SiRoB, Better Upload rate calcul
	m_AvarageDDROPreviousAddedTimestamp = GetTickCount(); //MORPH - Added by SiRoB, Better Upload rate calcul
}

void CStatistics::Init()
{
	maxcumDown =			thePrefs.GetConnMaxDownRate();
	cumUpavg =				thePrefs.GetConnAvgUpRate();
	maxcumDownavg =			thePrefs.GetConnMaxAvgDownRate();
	cumDownavg =			thePrefs.GetConnAvgDownRate();
	maxcumUpavg =			thePrefs.GetConnMaxAvgUpRate();
	maxcumUp =				thePrefs.GetConnMaxUpRate();
}

// This function is going to basically calculate and save a bunch of averages.
//				I made a seperate funtion so that it would always run instead of having
//				the averages not be calculated if the graphs are disabled (Which is bad!).
void CStatistics::UpdateConnectionStats(float uploadrate, float downloadrate)
{
	rateUp = uploadrate;
	rateDown = downloadrate;

	if (maxUp < uploadrate)
		maxUp = uploadrate;
	if (maxcumUp < maxUp){
		maxcumUp = maxUp;
		thePrefs.SetConnMaxUpRate(maxcumUp);
	}

	if (maxDown < downloadrate)
		maxDown = downloadrate;
	if (maxcumDown < maxDown){
		maxcumDown = maxDown;
		thePrefs.SetConnMaxDownRate(maxcumDown);
	}

	cumDownavg = GetAvgDownloadRate(AVG_TOTAL);
	if (maxcumDownavg < cumDownavg){
		maxcumDownavg = cumDownavg;
		thePrefs.SetConnMaxAvgDownRate(maxcumDownavg);
	}

	cumUpavg = GetAvgUploadRate(AVG_TOTAL);
	if (maxcumUpavg < cumUpavg){
		maxcumUpavg = cumUpavg;
		thePrefs.SetConnMaxAvgUpRate(maxcumUpavg);
	}
	
	// Transfer Times (Increment Session)
	if (uploadrate > 0 || downloadrate > 0) {
		if (start_timeTransfers == 0)
			start_timeTransfers = GetTickCount();
		else
			time_thisTransfer = (GetTickCount() - start_timeTransfers) / 1000;

		if (uploadrate > 0) {
			if (start_timeUploads == 0)
				start_timeUploads = GetTickCount();
			else
				time_thisUpload = (GetTickCount() - start_timeUploads) / 1000;
		}

		if (downloadrate > 0) {
			if (start_timeDownloads == 0)
				start_timeDownloads = GetTickCount();
			else
				time_thisDownload = (GetTickCount() - start_timeDownloads) / 1000;
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
	if (theStats.serverConnectTime == 0) 
		time_thisServerDuration = 0;
	else
		time_thisServerDuration = (GetTickCount() - theStats.serverConnectTime) / 1000;
}

void CStatistics::RecordRate()
{
	if (theStats.transferStarttime == 0)
		return;

	// Accurate datarate Calculation
	uint32 stick = GetTickCount();
	TransferredData newitemUP = {theStats.sessionSentBytes, stick};
	TransferredData newitemDN = {theStats.sessionReceivedBytes, stick};
	//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	TransferredData newitemFriends = {theStats.sessionSentBytesToFriend, stick};
	//MORPH END   - Added by SiRoB, ZZ Upload system 20030818-1923

	downrateHistory.push_front(newitemDN);
	uprateHistory.push_front(newitemUP);

	//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	uprateHistoryFriends.push_front(newitemFriends);
	//MORPH END   - Added by SiRoB, ZZ Upload system 20030818-1923

	// limit to maxmins
	UINT uAverageMilliseconds = thePrefs.GetStatsAverageMinutes() * (UINT)60000;
	while (downrateHistory.front().timestamp - downrateHistory.back().timestamp > uAverageMilliseconds)
		downrateHistory.pop_back();
	while (uprateHistory.front().timestamp - uprateHistory.back().timestamp > uAverageMilliseconds)
		uprateHistory.pop_back();
	//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	while (uprateHistoryFriends.front().timestamp - uprateHistoryFriends.back().timestamp > uAverageMilliseconds)
		uprateHistoryFriends.pop_back();
	//MORPH END   - Added by SiRoB, ZZ Upload system 20030818-1923
}

// Changed these two functions (khaos)...
float CStatistics::GetAvgDownloadRate(int averageType)
{
	DWORD running;
	switch (averageType)
	{
		case AVG_SESSION:
			if (theStats.transferStarttime == 0)
				return 0.0F;
			running = (GetTickCount() - theStats.transferStarttime) / 1000;
			if (running < 5)
				return 0.0F;
			return (float)(theStats.sessionReceivedBytes / 1024) / running;

		case AVG_TOTAL:
			if (theStats.transferStarttime == 0)
				return thePrefs.GetConnAvgDownRate();
			running = (GetTickCount() - theStats.transferStarttime) / 1000;
			if (running < 5)
				return thePrefs.GetConnAvgDownRate();
			return (((float)(theStats.sessionReceivedBytes / 1024) / running) + thePrefs.GetConnAvgDownRate()) / 2.0F;

		default:
			if (downrateHistory.size() == 0)
				return 0.0F;
			float deltat = (downrateHistory.front().timestamp - downrateHistory.back().timestamp) / 1000.0F;
			if (deltat > 0.0F)
				return ((downrateHistory.front().datalen - downrateHistory.back().datalen) / deltat) / 1024.0F;
			return 0.0F;
	}
}

float CStatistics::GetAvgUploadRate(int averageType)
{
	DWORD running;
	switch (averageType)
	{
		case AVG_SESSION:
			if (theStats.transferStarttime == 0)
				return 0.0F;
			running = (GetTickCount() - theStats.transferStarttime) / 1000;
			if (running < 5)
				return 0.0F;
			return (float)(theStats.sessionSentBytes / 1024) / running;

		case AVG_TOTAL:
			if (theStats.transferStarttime == 0)
				return thePrefs.GetConnAvgUpRate();
			running = (GetTickCount() - theStats.transferStarttime) / 1000;
			if (running < 5)
				return thePrefs.GetConnAvgUpRate();
			return (((float)(theStats.sessionSentBytes / 1024) / running) + thePrefs.GetConnAvgUpRate()) / 2.0F;

		default:
			if (uprateHistory.size() == 0)
				return 0.0F;
			float deltat = (uprateHistory.front().timestamp - uprateHistory.back().timestamp) / 1000.0F;
			if (deltat > 0.0F)
				return ((uprateHistory.front().datalen - uprateHistory.back().datalen) / deltat) / 1024.0F;
			return 0.0F;
	}
}

void CStatistics::CompDownDatarateOverhead()
{
	//MORPH START - Changed by SiRoB, Better datarate mesurement for low and high speed
	DWORD curTick = GetTickCount();
	if (m_nDownDataRateMSOverhead > 0) {
		if (m_AvarageDDRO_list.GetCount() > 0)
			m_AvarageDDROPreviousAddedTimestamp = m_AvarageDDRO_list.GetTail().timestamp;
		TransferredData newitem = {m_nDownDataRateMSOverhead, curTick};
		m_AvarageDDRO_list.AddTail(newitem);
		m_sumavgDDRO += m_nDownDataRateMSOverhead;
		m_nDownDataRateMSOverhead = 0;
	}
	while (m_AvarageDDRO_list.GetCount() > 1 &&  (m_AvarageDDROPreviousAddedTimestamp - m_AvarageDDRO_list.GetHead().timestamp) > MAXAVERAGETIMEDOWNLOAD)
		m_sumavgDDRO -= m_AvarageDDRO_list.RemoveHead().datalen;

	if (m_AvarageDDRO_list.GetCount() > 1) {
		DWORD dwDuration = m_AvarageDDRO_list.GetTail().timestamp - m_AvarageDDRO_list.GetHead().timestamp;
		if (dwDuration < 880) dwDuration = 880;
		DWORD dwAvgTickDuration = dwDuration / (m_AvarageDDRO_list.GetCount() - 1);
		if ((curTick - m_AvarageDDRO_list.GetTail().timestamp) > dwAvgTickDuration)
			dwDuration += curTick - m_AvarageDDRO_list.GetTail().timestamp - dwAvgTickDuration;
		m_nDownDatarateOverhead = 1000U * (m_sumavgDDRO - m_AvarageDDRO_list.GetHead().datalen) / dwDuration;
	} else if (m_AvarageDDRO_list.GetCount() == 1) {
		DWORD dwDuration = m_AvarageDDRO_list.GetTail().timestamp - m_AvarageDDROPreviousAddedTimestamp;
		if (dwDuration < 880) dwDuration = 880;
		if ((curTick - m_AvarageDDRO_list.GetTail().timestamp) > dwDuration)
			dwDuration = curTick - m_AvarageDDRO_list.GetTail().timestamp;
		m_nDownDatarateOverhead = 1000 * m_sumavgDDRO / dwDuration;
	} else
		m_nDownDatarateOverhead = 0;
	//MORPH END  - Changed by SiRoB, Better datarate mesurement for low and high speed
}

void CStatistics::CompUpDatarateOverhead()
{
	//MORPH START - Changed by SiRoB, Better datarate mesurement for low and high speed
	DWORD curTick = GetTickCount();
	if (m_nUpDataRateMSOverhead > 0) {
		if (m_AvarageUDRO_list.GetCount() > 0)
			m_AvarageUDROPreviousAddedTimestamp = m_AvarageUDRO_list.GetTail().timestamp;
		TransferredData newitem = {m_nUpDataRateMSOverhead, curTick};
		m_AvarageUDRO_list.AddTail(newitem);
		m_sumavgUDRO += m_nUpDataRateMSOverhead;
		m_nUpDataRateMSOverhead = 0;
	}

	while (m_AvarageUDRO_list.GetCount() > 1 && (m_AvarageUDROPreviousAddedTimestamp - m_AvarageUDRO_list.GetHead().timestamp) > MAXAVERAGETIMEUPLOAD)
		m_sumavgUDRO -= m_AvarageUDRO_list.RemoveHead().datalen;

	if (m_AvarageUDRO_list.GetCount() > 1) {
		DWORD dwDuration = m_AvarageUDRO_list.GetTail().timestamp - m_AvarageUDRO_list.GetHead().timestamp;
		if (dwDuration < 880) dwDuration = 880;
		DWORD dwAvgTickDuration = dwDuration / (m_AvarageUDRO_list.GetCount() - 1);
		if ((curTick - m_AvarageUDRO_list.GetTail().timestamp) > dwAvgTickDuration)
			dwDuration += curTick - m_AvarageUDRO_list.GetTail().timestamp - dwAvgTickDuration;
		m_nUpDatarateOverhead = 1000U * (m_sumavgUDRO - m_AvarageUDRO_list.GetHead().datalen) / dwDuration;
	} else if (m_AvarageUDRO_list.GetCount() == 1) {
		DWORD dwDuration = m_AvarageUDRO_list.GetTail().timestamp - m_AvarageUDROPreviousAddedTimestamp;
		if (dwDuration < 880) dwDuration = 880;
		if ((curTick - m_AvarageUDRO_list.GetTail().timestamp) > dwDuration)
			dwDuration = curTick - m_AvarageUDRO_list.GetTail().timestamp;
		m_nUpDatarateOverhead = 1000U * m_sumavgUDRO / dwDuration;
	} else
		m_nUpDatarateOverhead = 0;
	//MORPH END  - Changed by SiRoB, Better datarate mesurement for low and high speed
}

void CStatistics::ResetDownDatarateOverhead()
{
	m_nDownDataRateMSOverhead = 0;
	m_AvarageDDRO_list.RemoveAll();
	m_sumavgDDRO = 0;
	m_nDownDatarateOverhead = 0;
}

void CStatistics::ResetUpDatarateOverhead()
{
	m_nUpDataRateMSOverhead = 0;
	m_sumavgUDRO = 0;
	m_AvarageUDRO_list.RemoveAll();
	m_nUpDatarateOverhead = 0;
}