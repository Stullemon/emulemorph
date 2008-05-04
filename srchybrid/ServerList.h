//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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

class CServer;

class CServerList
{
	friend class CServerListCtrl;
public:
	CServerList();
	~CServerList();

	bool		Init();
	void		Process();
	void		Sort();
	void		GetUserSortedServers();
	void		MoveServerDown(const CServer* pServer);
	void		AutoUpdate();
	bool		AddServerMetToList(const CString& rstrFile, bool bMerge);
	void		AddServersFromTextFile(const CString& rstrFilename);
	bool		SaveServermetToFile();
	bool		SaveStaticServers();

	bool		AddServer(const CServer* pServer, bool bAddTail = true);
	void		RemoveServer(const CServer* pServer);
	void		RemoveAllServers();
	void		RemoveDuplicatesByAddress(const CServer* pExceptThis);
	void		RemoveDuplicatesByIP(const CServer* pExceptThis);

	UINT		GetServerCount() const { return list.GetCount(); }
	CServer*	GetServerAt(UINT pos) const { return list.GetAt(list.FindIndex(pos)); }
	CServer*	GetSuccServer(const CServer* lastserver) const;
	CServer*	GetNextServer(bool bOnlyObfuscated);
	CServer*	GetServerByAddress(LPCTSTR address, uint16 port) const;
	CServer*	GetServerByIP(uint32 nIP) const;
	CServer*	GetServerByIPTCP(uint32 nIP, uint16 nTCPPort) const;
	CServer*	GetServerByIPUDP(uint32 nIP, uint16 nUDPPort, bool bObfuscationPorts = true) const;
	int			GetPositionOfServer(const CServer* pServer) const;

	void		SetServerPosition(UINT newPosition);
	UINT		GetServerPostion() const { return serverpos; }

	void		ResetSearchServerPos() { searchserverpos = 0; }
	CServer*	GetNextSearchServer();

	void		ServerStats();
	CServer*	GetNextStatServer();

	bool		IsGoodServerIP(const CServer* pServer) const;
	void		GetStatus(uint32& total, uint32& failed, uint32& user, uint32& file, uint32& lowiduser, 
						  uint32& totaluser, uint32& totalfile, float& occ) const;
	void		GetAvgFile(uint32& average) const;
	void		GetUserFileStatus(uint32& user, uint32& file) const;
	UINT		GetDeletedServerCount() const { return delservercount; }

    bool        GiveServersForTraceRoute();
	//EastShare Start - PreferShareAll by AndCycle
	void		PushBackNoShare();	// SLUGFILLER: preferShareAll
	//EastShare End - PreferShareAll by AndCycle

	void		CheckForExpiredUDPKeys();
#ifdef _DEBUG
	void		Dump();
#endif

private:
	UINT		serverpos;
	UINT		searchserverpos;
	UINT		statserverpos;
	uint8		version;
	UINT		servercount;
	CTypedPtrList<CPtrList, CServer*> list;
	UINT		delservercount;
	DWORD		m_nLastSaved;
//EastShare Start - added by AndCycle, IP to Country
public:
	void ResetIP2Country();
//EastShare End - added by AndCycle, IP to Country
};