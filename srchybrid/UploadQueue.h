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
#include "Loggable.h"

class CUpDownClient;
typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

class CUploadQueue: public CLoggable
{
public:
	CUploadQueue();
	~CUploadQueue();

	void	Process();
//MORPH START - Changed by Yun.SF3, ZZ upload system
	void	AddClientToQueue(CUpDownClient* client,bool bIgnoreTimelimit = false, bool addInFirstPlace = false);
//MORPH END   - Changed by Yun.SF3, ZZ upload system
	bool	RemoveFromUploadQueue(CUpDownClient* client, LPCTSTR pszReason = NULL, bool updatewindow = true, bool earlyabort = false);
	bool	RemoveFromWaitingQueue(CUpDownClient* client,bool updatewindow = true);
	bool	IsOnUploadQueue(CUpDownClient* client)	const {return (waitinglist.Find(client) != 0);}
	bool	IsDownloading(CUpDownClient* client)	const {return (uploadinglist.Find(client) != 0);}

//MORPH START - Added by Yun.SF3, ZZ upload system
    void    UpdateDatarates();
	uint32	GetDatarate();
	uint32  GetToNetworkDatarate();
//MORPH END   - Added by Yun.SF3, ZZ upload system

	//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	uint32	GetAvgRespondTime(uint8 index)	{return AvgRespondTime[index]>10 ? AvgRespondTime[index] : 1500;}
	void	SetAvgRespondTime(uint8 index,uint32 in_AvgRespondTime)	{AvgRespondTime[index]=in_AvgRespondTime;}
	//uint32	GetMaxVUR()	{return MaxVUR;}//[lovelace]
	uint32	GetMaxVUR();
	//void	SetMaxVUR(uint32 in_MaxVUR, uint32 min, uint32 max){MaxVUR=((in_MaxVUR>max)?max:((in_MaxVUR<min)?min:in_MaxVUR));}//[lovelace]
	void	SetMaxVUR(uint32 in_MaxVUR){MaxVUR=in_MaxVUR;}
	//MORPH END   - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]

	bool	CheckForTimeOver(CUpDownClient* client);
	int		GetWaitingUserCount()					{return waitinglist.GetCount();}
	int		GetUploadQueueLength()					{return uploadinglist.GetCount();}
	//MORPH START - Added by SiRoB, ZZ Upload System 20040213-1623
	int		GetActiveUploadsCount()					{return m_MaxActiveClientsShortTime;}
	double  GetAverageActiveUploadsCount()          {return m_averageActiveClients;}
	//MORPH END - Added by SiRoB, ZZ Upload System 20040213-1623

	POSITION GetFirstFromUploadList()				{return uploadinglist.GetHeadPosition();}
	CUpDownClient* GetNextFromUploadList(POSITION &curpos)	{return uploadinglist.GetNext(curpos);}
	CUpDownClient* GetQueueClientAt(POSITION &curpos)	{return uploadinglist.GetAt(curpos);}

	POSITION GetFirstFromWaitingList()				{return waitinglist.GetHeadPosition();}
	CUpDownClient* GetNextFromWaitingList(POSITION &curpos)	{return waitinglist.GetNext(curpos);}
	CUpDownClient* GetWaitClientAt(POSITION &curpos)	{return waitinglist.GetAt(curpos);}

	CUpDownClient*	GetWaitingClientByIP_UDP(uint32 dwIP, uint16 nUDPPort);
	CUpDownClient*	GetWaitingClientByIP(uint32 dwIP);
	CUpDownClient*	GetNextClient(const CUpDownClient* update);

	//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
    	uint64 GetTotalCompletedBytes() { return totalCompletedBytes; }
	//MORPH END   - Added by SiRoB, ZZ Upload system 20030818-1923

	
	void	DeleteAll();
	uint16	GetWaitingPosition(CUpDownClient* client);





	uint32	GetSuccessfullUpCount()					{return successfullupcount;}
	uint32	GetFailedUpCount()						{return failedupcount;}
	uint32	GetAverageUpTime();
//	void	FindSourcesForFileById(CUpDownClientPtrList* srclist, const uchar* filehash);
	//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	bool    RemoveOrMoveDown(CUpDownClient* client, bool onlyCheckForRemove = false);
	CUpDownClient* FindBestClientInQueue(bool allowLowIdAddNextConnectToBeSet = false, CUpDownClient* lowIdClientMustBeInSameOrBetterClassAsThisClient = NULL);
	bool RightClientIsBetter(CUpDownClient* leftClient, uint32 leftScore, CUpDownClient* rightClient, uint32 rightScore);
	//MORPH END   - Added by SiRoB, ZZ Upload system 20030818-1923

	//Morph - added by AndCycle, separate special prio compare
	int	RightClientIsSuperior(CUpDownClient* leftClient, CUpDownClient* rightClient);

protected:
	void	RemoveFromWaitingQueue(POSITION pos, bool updatewindow);
//	POSITION	GetWaitingClient(CUpDownClient* client);
//	POSITION	GetWaitingClientByID(CUpDownClient* client);
//	POSITION	GetDownloadingClient(CUpDownClient* client);
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	bool		AcceptNewClient(uint32 numberOfUploads);
	bool		AddUpNextClient(CUpDownClient* directadd = 0, bool highPrioCheck = false);
	//MORPH END - Added by Yun.SF3, ZZ Upload System
	
	static VOID CALLBACK UploadTimer(HWND hWnd, UINT nMsg, UINT nId, DWORD dwTime);

private:
	//MORPH - Removed by SiRoB, ZZ Upload System
	/*
	void	UpdateMaxClientScore();
	uint32	GetMaxClientScore()						{return m_imaxscore;}
	*/

	CUpDownClientPtrList	waitinglist;
	CUpDownClientPtrList	uploadinglist;
	
    void InsertInUploadingList(CUpDownClient* newclient);
    void RemoveLowestFromWaitinglist();
    double GetAverageCombinedFilePrioAndCredit();
    uint32 GetWantedNumberOfTrickleUploads();

	//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-0133
	void	ReSortUploadSlots(bool force = false);

	//MORPH START - Changed by SiRoB, Better datarate mesurement for low and high speed
	typedef struct TransferredData {
		uint32	datalen;
		DWORD	timestamp;
	};

	CList<uint64,uint64> avarage_dr_list;
    CList<uint64,uint64> avarage_friend_dr_list;
	CList<DWORD,DWORD> avarage_tick_list;
	CList<int,int> activeClients_list;
	CList<DWORD,DWORD> activeClients_tick_list;

	uint32	datarate;   //datarate of sent to network (including friends)
	uint32  friendDatarate; // datarate of sent to friends (included in above total)
	//uint32	dataratems;
	//MORPH END - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20030723-0133

	UINT_PTR h_timer;
	uint32	successfullupcount;
	uint32	failedupcount;
	uint32	totaluploadtime;

	//MORPH START - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20040213-1623
	uint32  m_MaxActiveClients;
	uint32  m_MaxActiveClientsShortTime;
	double  m_averageActiveClients;
	//MORPH END - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System 20040213-1623

	uint32	m_nLastStartUpload;
	bool	lastupslotHighID; // VQB lowID alternation
	//MORPH - Removed by SiRoB, Not used in this mod, full chunk transfer is enforced	
	/*
	bool	m_bRemovedClientByScore;
	*/

	//MORPH START - Added by SiRoB, ZZ Upload System 20030824-2238
	DWORD   m_lastCalculatedDataRateTick;
	DWORD   m_dwLastCheckedForHighPrioClient;

	DWORD   m_dwLastCalculatedAverageCombinedFilePrioAndCredit;
	double	m_fAverageCombinedFilePrioAndCredit;

	uint64	m_avarage_dr_sum;
	DWORD   m_dwLastResortedUploadSlots;

	DWORD   m_dwLastSlotAddTick;
	uint64  totalCompletedBytes;
	DWORD   m_FirstRanOutOfSlotsTick;
	//MORPH END - Added by SiRoB, ZZ Upload System 20030824-2238

	//MORPH START - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	uint32	AvgRespondTime[2];
	uint32	MaxVUR;
	//MORPH END - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
};
