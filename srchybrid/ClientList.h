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

class CClientReqSocket;
class CUpDownClient;
namespace Kademlia{
	class CContact;
};
typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

#define BAN_CLEANUP_TIME	1200000 // 20 min


//------------CDeletedClient Class----------------------
// this class / list is a bit overkill, but currently needed to avoid any exploit possibtility
// it will keep track of certain clients attributes for 2 hours, while the CUpDownClient object might be deleted already
// currently: IP, Port, UserHash
struct PORTANDHASH{
	uint16 nPort;
	void* pHash;
};

class CDeletedClient{
public:
	CDeletedClient(CUpDownClient* pClient);
	CArray<PORTANDHASH,PORTANDHASH> m_ItemsList;
	uint32							m_dwInserted;
};

// ----------------------CClientList Class---------------
class CClientList: public CLoggable
{
	friend class CClientListCtrl;

public:
	CClientList();
	~CClientList();
	void	AddClient(CUpDownClient* toadd,bool bSkipDupTest = false);
	void	RemoveClient(CUpDownClient* toremove);
	void	GetStatistics(uint32& totalclient, int stats[],
						  CMap<uint32, uint32, uint32, uint32>& clientVersionEDonkey,
						  CMap<uint32, uint32, uint32, uint32>& clientVersionEDonkeyHybrid,
						  CMap<uint32, uint32, uint32, uint32>& clientVersionEMule,
						  CMap<uint32, uint32, uint32, uint32>& clientVersionAMule);
	//MORPH START - Slugfiller: modid
	void	GetModStatistics(CRBMap<uint16, CRBMap<CString, uint32>* > *clientMods);
	void	ReleaseModStatistics(CRBMap<uint16, CRBMap<CString, uint32>* > *clientMods);
	//MORPH END   - Slugfiller: modid
	void	DeleteAll();
	bool	AttachToAlreadyKnown(CUpDownClient** client, CClientReqSocket* sender);
	CUpDownClient* FindClientByIP(uint32 clientip, UINT port) const;
	CUpDownClient* FindClientByUserHash(const uchar* clienthash) const;
	CUpDownClient* FindClientByIP(uint32 clientip) const;
	CUpDownClient* FindClientByIP_UDP(uint32 clientip, UINT nUDPport) const;
	CUpDownClient* FindClientByServerID(uint32 uServerIP, uint32 uUserID) const;
	CUpDownClient* FindClientByUserID_KadPort(uint32 clientID,uint16 kadPort) const;
	CUpDownClient* FindClientByIP_KadPort(uint32 ip, uint16 port) const;
	CUpDownClient* GetRandomKadClient() const;
//	void	GetClientListByFileID(CUpDownClientPtrList *clientlist, const uchar *fileid);	// #zegzav:updcliuplst

	void	AddBannedClient(uint32 dwIP);
	bool	IsBannedClient(uint32 dwIP);
	void	RemoveBannedClient(uint32 dwIP);
	uint16	GetBannedCount()			{return m_bannedList.GetCount(); }

	void	AddTrackClient(CUpDownClient* toadd);
	bool	ComparePriorUserhash(uint32 dwIP, uint16 nPort, void* pNewHash);
	uint16	GetClientsFromIP(uint32 dwIP);

	void	Process();
	void	RequestTCP(Kademlia::CContact* contact);
	void	RemoveTCP(CUpDownClient* torem);

	bool	IsValidClient(CUpDownClient* tocheck);
	void	Debug_SocketDeleted(CClientReqSocket* deleted);

	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	bool GiveClientsForTraceRoute();
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
private:
	CUpDownClientPtrList list;
	CMap<uint32, uint32, uint32, uint32> m_bannedList;
	CMap<uint32, uint32, CDeletedClient*, CDeletedClient*> m_trackedClientsList;
	uint32	m_dwLastBannCleanUp;
	uint32	m_dwLastTrackedCleanUp;
	CUpDownClientPtrList RequestTCPList;
	CCriticalSection m_RequestTCPLock;

//EastShare Start - added by AndCycle, IP to Country
public:
	void ResetIP2Country();
//EastShare End - added by AndCycle, IP to Country
};
