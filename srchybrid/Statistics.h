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
#pragma once
#include <list>

// CStatistics
#define AVG_SESSION 0
#define AVG_TOTAL 2
#define AVG_TIME 1

class CStatistics
{
public:
	CStatistics();   // standard constructor
	~CStatistics();

	void RecordRate();
	float	GetAvgDownloadRate(int averageType);
	float	GetAvgUploadRate(int averageType);

	// -khaos--+++> (2-11-03)
	uint32	GetTransferTime()			{ return timeTransfers + time_thisTransfer; }
	uint32	GetUploadTime()				{ return timeUploads + time_thisUpload; }
	uint32	GetDownloadTime()			{ return timeDownloads + time_thisDownload; }
	uint32	GetServerDuration()			{ return timeServerDuration + time_thisServerDuration; }
	void	Add2TotalServerDuration()	{ timeServerDuration += time_thisServerDuration;
										  time_thisServerDuration = 0; }
	void	UpdateConnectionStats(float uploadrate, float downloadrate);
public:
	//	Cumulative Stats
	float	maxDown;
	float	maxDownavg;
	float	cumDownavg;
	float	maxcumDownavg;
	float	maxcumDown;
	float	cumUpavg;
	float	maxcumUpavg;
	float	maxcumUp;
	float	maxUp;
	float	maxUpavg;
	float	rateDown;
	float	rateUp;
	uint32	timeTransfers;
	uint32	timeDownloads;
	uint32	timeUploads;
	uint32	start_timeTransfers;
	uint32	start_timeDownloads;
	uint32	start_timeUploads;
	uint32	time_thisTransfer;
	uint32	time_thisDownload;
	uint32	time_thisUpload;
	uint32	timeServerDuration;
	uint32	time_thisServerDuration;

private:
	typedef struct TransferredData {
		uint32	datalen;
		DWORD	timestamp;
	};
	std::list<TransferredData> uprateHistory; // By BadWolf
	std::list<TransferredData> downrateHistory; // By BadWolf
	std::list<TransferredData> uprateHistoryFriends; //MORPH - Added by SiRoB, ZZ Upload System
};
