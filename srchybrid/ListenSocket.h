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

// handling incoming connections (up or downloadrequests)

#pragma once
#include "EMSocket.h"

class CUpDownClient;
class CPacket;
class CTimerWnd;
class CPreferences;

class CClientReqSocket : public CEMSocket{
	friend class CListenSocket;
public:
	CClientReqSocket(CPreferences* in_prefs, CUpDownClient* in_client = 0);	
	void	Disconnect();

	void	ResetTimeOutTimer();
	bool	CheckTimeOut();
	void	Safe_Delete();
	
	bool	Create();
	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	virtual SocketSentBytes Send(uint32 maxNumberOfBytesToSend, bool onlyAllowedToSendControlPacket = false);
	//MORPH END - Added by SiRoB, ZZ Upload System 20030818-1923

	CUpDownClient*	client;
protected:
	virtual void Close()	{CAsyncSocketEx::Close();} // deadlake PROXYSUPPORT - changed to AsyncSocketEx
	virtual	void OnInit();
	void		 OnClose(int nErrorCode);
	void		 OnSend(int nErrorCode);
	void		 OnReceive(int nErrorCode);
	void		 OnError(int nErrorCode);
	void		 PacketReceived(Packet* packet);
	void		 PacketReceivedCppEx(Packet* packet);
private:
	void	Delete_Timed();
	~CClientReqSocket();

	bool	ProcessPacket(char* packet, uint32 size,UINT opcode);
	bool	ProcessExtPacket(char* packet, uint32 size,UINT opcode);
	void    PacketToDebugLogLine(const char* protocol, const char* packet, uint32 size, UINT opcode) const;
	//MORPH START - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	void  SmartUploadControl();
	//MORPH END - Added by SiRoB, Smart Upload Control v2 (SUC) [lovelace]
	CPreferences* app_prefs;
	uint32	timeout_timer;
	bool	deletethis;
	uint32	deltimer;

};


// CListenSocket command target
class CListenSocket : public CAsyncSocketEx
{
public:
	CListenSocket(CPreferences* in_prefs);
	~CListenSocket();
	bool	StartListening();
	void	StopListening();
	virtual void OnAccept(int nErrorCode);
	void	Process();
	void	RemoveSocket(CClientReqSocket* todel);
	void	AddSocket(CClientReqSocket* toadd);
	uint16	GetOpenSockets()		{return socket_list.GetCount();}
	void	KillAllSockets();
	bool	TooManySockets(bool bIgnoreInterval = false);
	uint32	GetMaxConnectionReached()	{return maxconnectionreached;}
	bool    IsValidSocket(CClientReqSocket* totest);
	void	AddConnection();
	void	RecalculateStats();
	void	ReStartListening();
	void	Debug_ClientDeleted(CUpDownClient* deleted);

	void	UpdateConnectionsStatus();
	float	GetMaxConperFiveModifier();
	uint32	GetPeakConnections()		{ return peakconnections; }
	uint32	GetTotalConnectionChecks()	{ return totalconnectionchecks; }
	float	GetAverageConnections()		{ return averageconnections; }
	uint32	GetActiveConnections()		{ return activeconnections; }

private:
	bool bListening;
	CPreferences* app_prefs;
	CTypedPtrList<CPtrList, CClientReqSocket*> socket_list;
	uint16	opensockets;
	uint16	m_OpenSocketsInterval;
	uint32	maxconnectionreached;
	uint16	m_ConnectionStates[3];
	uint16	m_nPeningConnections;
	uint32	peakconnections;
	uint32	totalconnectionchecks;
	float	averageconnections;
	uint32	activeconnections;
	//MORPH START - Added by Yun.SF3, Auto DynUp changing
	void	SwitchSUC(bool bSetSUCOn = false);
	uint16	per5average;
	//MORPH END - Added by Yun.SF3, Auto DynUp changing
};

