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
#include "updownclient.h" //MORPH - Added by SiRoB

class CClientReqSocket;
//class CUpDownClient;

namespace Kademlia{
	class CContact;
};
typedef CTypedPtrList<CPtrList, CUpDownClient*> CUpDownClientPtrList;

//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
#include <map>
//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-

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
	void	GetStatistics(uint32 &totalclient, int stats[], CMap<uint16, uint16, uint32, uint32> *clientVersionEDonkey=NULL, CMap<uint16, uint16, uint32, uint32> *clientVersionEDonkeyHybrid=NULL, CMap<uint16, uint16, uint32, uint32> *clientVersionEMule=NULL, CMap<uint16, uint16, uint32, uint32> *clientVersionLMule=NULL); // xrmb : statsclientstatus
	void	DeleteAll();
	bool	AttachToAlreadyKnown(CUpDownClient** client, CClientReqSocket* sender);
	CUpDownClient* FindClientByIP(uint32 clientip, UINT port);
	CUpDownClient* FindClientByUserHash(const uchar* clienthash);
	CUpDownClient* FindClientByIP(uint32 clientip);
	CUpDownClient* FindClientByIP_UDP(uint32 clientip, UINT nUDPport);
	CUpDownClient* FindClientByServerID(uint32 uServerIP, uint32 uUserID);
	CUpDownClient* FindClientByID_KadPort(uint32 clientID,uint16 kadPort);
	void	GetClientListByFileID(CUpDownClientPtrList *clientlist, const uchar *fileid);	// #zegzav:updcliuplst
	
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

//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
public:
	void AddClientType(EClientSoftware clientSoft, const CString& description);
	void RemoveClientType(EClientSoftware clientSoft, const CString& description);

	// Used for construction of statistic tree
	typedef std::map<CString, uint32> ClientMap; // The CMap class does not sort correctly the string
	uint32 GetTotalclient() const {return list.GetCount();}
	const ClientMap& GeteMuleMap() const {return m_eMuleMap;}
	const ClientMap& GetlMuleMap() const {return m_lMuleMap;}
	const ClientMap& GeteDonkeyMap() const {return m_eDonkeyMap;}
	const ClientMap& GeteDonkeyHybridMap() const {return m_eDonkeyHybridMap;}
	const ClientMap& GetcDonkeyMap() const {return m_cDonkeyMap;}
	const ClientMap& GetoldMlDonkeyMap() const {return m_oldMlDonkeyMap;}
	const ClientMap& GetShareazaMap() const {return m_shareazaMap;}
	const ClientMap& GetUnknownMap() const {return m_unknownMap;}

private:
	ClientMap m_eMuleMap;
	ClientMap m_lMuleMap;
	ClientMap m_eDonkeyMap;
	ClientMap m_eDonkeyHybridMap;
	ClientMap m_cDonkeyMap;
	ClientMap m_oldMlDonkeyMap;
	ClientMap m_shareazaMap;
	ClientMap m_unknownMap;
//MORPH - Added by Yun.SF3, Maella -Support for tag ET_MOD_VERSION 0x55 II-
};
