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

class CPartFile; //Morph - added by AndCycle, itsonlyme: cacheUDPsearchResults

class CServer;

class CServerList: public CLoggable
{
	friend class CServerListCtrl;
public:
	CServerList();
	~CServerList();

	bool		Init();
	bool		AddServer(CServer* in_server );
	void		RemoveServer(const CServer* out_server);
	void		RemoveAllServers(void);

	bool		AddServermetToList(const CString& rstrFile, bool bMerge = true);
	void		AddServersFromTextFile(const CString& rstrFilename);
	bool		SaveServermetToFile();

	void		ServerStats();
	void		ResetServerPos()	{serverpos = 0;}
	void		ResetSearchServerPos()	{searchserverpos = 0;}
	CServer*	GetNextServer();
	CServer*	GetNextSearchServer();
	CServer*	GetNextStatServer();
	CServer*	GetServerAt(uint32 pos) const { return list.GetAt(list.FindIndex(pos)); }
	uint32		GetServerCount() const { return list.GetCount(); }
	CServer*	GetNextServer(const CServer* lastserver) const; // slow
	//Morph Start - added by AndCycle, itsonlyme: cacheUDPsearchResults
	CServer*	GetNextServer(const CServer* lastserver, CPartFile *file) const;	// itsonlyme: cacheUDPsearchResults
	//Morph End - added by AndCycle, itsonlyme: cacheUDPsearchResults
	CServer*	GetServerByAddress(LPCTSTR address, uint16 port) const;
	CServer*	GetServerByIP(uint32 nIP) const;
	CServer*	GetServerByIP(uint32 nIP, uint16 nPort) const;
	bool		IsGoodServerIP(const CServer* in_server) const;
	void		GetStatus(uint32& total, uint32& failed, 
						  uint32& user, uint32& file, uint32& lowiduser, 
						  uint32& totaluser, uint32& totalfile, 
						  float& occ) const;
	void		GetUserFileStatus(uint32& user, uint32& file) const;
	
	void		Sort();
	//EastShare Start - PreferShareAll by AndCycle
	void		PushBackNoShare();	// SLUGFILLER: preferShareAll
	//EastShare End - PreferShareAll by AndCycle
	void		MoveServerDown(const CServer* aServer);
	uint32		GetServerPostion() const { return serverpos;}
	void		SetServerPosition(uint32 newPosition);
	uint32		GetDeletedServerCount() const { return delservercount; }
	void		Process();
	void		AutoUpdate();
	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	bool		GiveServersForTraceRoute();
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)

private:
	uint32		serverpos;
	uint32		searchserverpos;
	uint32		statserverpos;
	uint8		version;
	uint32		servercount;
	CTypedPtrList<CPtrList, CServer*>	list;
	uint32		delservercount;
	uint32		m_nLastSaved;

//EastShare Start - added by AndCycle, IP to Country
public:
	void ResetIP2Country();
//EastShare End - added by AndCycle, IP to Country
};