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
#pragma once

class CUpDownClient;
typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

class CUploadQueue
{
public:
	CUploadQueue();
	~CUploadQueue();

	void	Process();
	void	AddClientToQueue(CUpDownClient* client,bool bIgnoreTimelimit = false, bool addInFirstPlace = false);
	bool	RemoveFromUploadQueue(CUpDownClient* client, LPCTSTR pszReason = NULL, bool updatewindow = true, bool earlyabort = false);
	bool	RemoveFromWaitingQueue(CUpDownClient* client,bool updatewindow = true);
	bool	IsOnUploadQueue(CUpDownClient* client)	const {return (waitinglist.Find(client) != 0);}
	//MORPH START - Changed by SiRoB, ResortUploadSlot fix
	/*
	bool	IsDownloading(CUpDownClient* client)	const {return (uploadinglist.Find(client) != 0;}
	*/
	bool	IsDownloading(CUpDownClient* client)	const {return (uploadinglist.Find(client) != 0 || tempUploadinglist.Find(client)!=0);}
	//MORPH END   - Changed by SiRoB, ResortUploadSlot fix

    void    UpdateDatarates();
	uint32	GetDatarate();
	uint32  GetToNetworkDatarate();

	//MORPH START - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	uint32	GetAvgRespondTime(uint8 index)	{return AvgRespondTime[index]>10 ? AvgRespondTime[index] : 1500;}
	void	SetAvgRespondTime(uint8 index,uint32 in_AvgRespondTime)	{AvgRespondTime[index]=in_AvgRespondTime;}
	//uint32	GetMaxVUR()	{return MaxVUR;}//[lovelace]
	uint32	GetMaxVUR();
	//void	SetMaxVUR(uint32 in_MaxVUR, uint32 min, uint32 max){MaxVUR=((in_MaxVUR>max)?max:((in_MaxVUR<min)?min:in_MaxVUR));}//[lovelace]
	void	SetMaxVUR(uint32 in_MaxVUR){MaxVUR=in_MaxVUR;}
	//MORPH END   - Added & Modified by SiRoB, Smart Upload Control v2 (SUC) [lovelace]

	int		GetWaitingUserCount()					{return waitinglist.GetCount();}
	int		GetUploadQueueLength()					{return uploadinglist.GetCount();}
	uint32	GetActiveUploadsCount()					{return m_MaxActiveClientsShortTime;}

	POSITION GetFirstFromUploadList()				{return uploadinglist.GetHeadPosition();}
	CUpDownClient* GetNextFromUploadList(POSITION &curpos)	{return uploadinglist.GetNext(curpos);}
	CUpDownClient* GetQueueClientAt(POSITION &curpos)	{return uploadinglist.GetAt(curpos);}

	POSITION GetFirstFromWaitingList()				{return waitinglist.GetHeadPosition();}
	CUpDownClient* GetNextFromWaitingList(POSITION &curpos)	{return waitinglist.GetNext(curpos);}
	CUpDownClient* GetWaitClientAt(POSITION &curpos)	{return waitinglist.GetAt(curpos);}

	CUpDownClient*	GetWaitingClientByIP_UDP(uint32 dwIP, uint16 nUDPPort);
	CUpDownClient*	GetWaitingClientByIP(uint32 dwIP);
	CUpDownClient*	GetNextClient(const CUpDownClient* update);


	//MORPH START - Added by SiRoB, WebCache 1.2f
	CUpDownClient*	FindClientByWebCacheUploadId(const uint32 id); // Superlexx - webcache
	//MORPH END   - Added by SiRoB, WebCache 1.2f
	
	void	DeleteAll();
	uint16	GetWaitingPosition(CUpDownClient* client);





	uint32	GetSuccessfullUpCount()					{return successfullupcount;}
	uint32	GetFailedUpCount()						{return failedupcount;}
	uint32	GetAverageUpTime();
//	void	FindSourcesForFileById(CUpDownClientPtrList* srclist, const uchar* filehash);

	bool    RemoveOrMoveDown(CUpDownClient* client, bool onlyCheckForRemove = false);
	CUpDownClient* FindBestClientInQueue(bool allowLowIdAddNextConnectToBeSet = false, CUpDownClient* lowIdClientMustBeInSameOrBetterClassAsThisClient = NULL);
	bool	RightClientIsBetter(CUpDownClient* leftClient, uint32 leftScore, CUpDownClient* rightClient, uint32 rightScore);
	void	ReSortUploadSlots(bool force = false);

	//Morph - added by AndCycle, separate special prio compare
	int	RightClientIsSuperior(CUpDownClient* leftClient, CUpDownClient* rightClient);

protected:
	void	RemoveFromWaitingQueue(POSITION pos, bool updatewindow);
//	POSITION	GetWaitingClient(CUpDownClient* client);
//	POSITION	GetWaitingClientByID(CUpDownClient* client);
//	POSITION	GetDownloadingClient(CUpDownClient* client);
	bool		AcceptNewClient();
	bool		AcceptNewClient(uint32 curUploadSlots);
	bool		ForceNewClient(bool allowEmptyWaitingQueue = false);

	bool		AddUpNextClient(CUpDownClient* directadd = 0, bool highPrioCheck = false);
	
	static VOID CALLBACK UploadTimer(HWND hWnd, UINT nMsg, UINT nId, DWORD dwTime);

private:
	void	UpdateMaxClientScore();
	uint32	GetMaxClientScore()						{return m_imaxscore;}
	void    UpdateActiveClientsInfo(DWORD curTick);

    void InsertInUploadingList(CUpDownClient* newclient);
    double GetAverageCombinedFilePrioAndCredit();
	void CheckForHighPrioClient();

	CUpDownClientPtrList	waitinglist;
	CUpDownClientPtrList	uploadinglist;
	CUpDownClientPtrList	tempUploadinglist;
			
	// By BadWolf - Accurate Speed Measurement
	typedef struct TransferredData {
		uint32	datalen;
		DWORD	timestamp;
	};
	CList<uint64,uint64> avarage_dr_list;
    CList<uint64,uint64> avarage_friend_dr_list;
	CList<DWORD,DWORD> avarage_tick_list;
	CList<int,int> activeClients_list;
	CList<DWORD,DWORD> activeClients_tick_list;
	uint32	datarate;   //datarate sent to network (including friends)
    uint32  friendDatarate; // datarate of sent to friends (included in above total)
	// By BadWolf - Accurate Speed Measurement

	UINT_PTR h_timer;
	uint32	successfullupcount;
	uint32	failedupcount;
	uint32	totaluploadtime;
	uint32	m_nLastStartUpload;
	bool	lastupslotHighID; // VQB lowID alternation
	uint32	m_dwRemovedClientByScore;

	uint32	m_imaxscore;

    DWORD   m_dwLastCalculatedAverageCombinedFilePrioAndCredit;
    float   m_fAverageCombinedFilePrioAndCredit;
    uint32  m_iHighestNumberOfFullyActivatedSlotsSinceLastCall;
    uint32  m_MaxActiveClients;
    uint32  m_MaxActiveClientsShortTime;
	uint32	m_iHighestUploadDatarateReachedByOneClient;	//MORPH - Added by SiRoB, Upload Splitting Class

    DWORD   m_lastCalculatedDataRateTick;
    uint64  m_avarage_dr_sum;

    DWORD   m_dwLastResortedUploadSlots;

	DWORD   m_dwLastCheckedForHighPrioClient;

	//MORPH START - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	uint32	AvgRespondTime[2];
	uint32	MaxVUR;
	//MORPH END - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
};
