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

class CPreferences;
class CServer;

class CServerList: public CLoggable
{
	friend class CServerListCtrl;
public:
	CServerList(CPreferences* in_prefs);
	~CServerList(void);
	bool		Init();
	bool		AddServer(CServer* in_server );
	void		RemoveServer(CServer* out_server);
	void		RemoveAllServers(void);
	bool		AddServermetToList(CString strFile, bool merge = true);
	void		AddServersFromTextFile(CString strFilename);
	bool		SaveServermetToFile(); //<<--9/22/02
	void		ServerStats();
	void		ResetServerPos()	{serverpos = 0;}
	void		ResetSearchServerPos()	{searchserverpos = 0;}
	CServer*	GetNextServer();
	CServer*	GetNextSearchServer();
	CServer*	GetNextStatServer();
	CServer*	GetServerAt(uint32 pos)	{return list.GetAt(list.FindIndex(pos));}
	uint32		GetServerCount()	{return list.GetCount();}
	CServer*	GetNextServer(CServer* lastserver); // slow
	CServer*	GetServerByAddress(LPCTSTR address, uint16 port);
	CServer*	GetServerByIP(uint32 nIP);
	CServer*	GetServerByIP(uint32 nIP, uint16 nPort);
	bool		IsGoodServerIP( CServer* in_server ); //<<--
	void		GetStatus( uint32 &total, uint32 &failed, uint32 &user, uint32 &file, uint32 &tuser, uint32 &tfile, float &occ);
	void		GetUserFileStatus( uint32 &user, uint32 &file);
//	bool		BroadCastPacket(Packet* packet); //send Packet to all server in the list
//	void		CancelUDPBroadcast();
//	void static CALLBACK UDPTimer(HWND hwnd, UINT uMsg,UINT_PTR idEvent,DWORD dwTime);
	void		Sort();
	//EastShare Start - PreferShareAll by AndCycle
	void		PushBackNoShare();	// SLUGFILLER: preferShareAll
	//EastShare End - PreferShareAll by AndCycle
	void		MoveServerDown(CServer* aServer);
	uint32		GetServerPostion()	{return serverpos;}
	void		SetServerPosition(uint32 newPosition) { if (newPosition<(uint32)list.GetCount() ) serverpos=newPosition; else serverpos=0;}
	uint32		GetDeletedServerCount()		{return delservercount;}
	void		Process();
	void		AutoUpdate();
	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	bool		GiveServersForTraceRoute();
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)

private:
//	void		SendNextPacket();
//	uint32		udp_timer;
//	POSITION	broadcastpos;
//	Packet*		broadcastpacket;
	uint32		serverpos;
	uint32		searchserverpos;
	uint32		statserverpos;
	int8		version;
	uint32		servercount;
	CTypedPtrList<CPtrList, CServer*>	list;
	CPreferences*	app_prefs;
	uint32		delservercount;
	uint32		m_nLastSaved;
};