//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "UploadBandwidthThrottler.h" // ZZ:UploadBandWithThrottler (UDP)

class CServerConnect;
struct SServerUDPPacket;
class CUDPSocket;
class Packet;
class CServer;


///////////////////////////////////////////////////////////////////////////////
// CUDPSocketWnd

class CUDPSocketWnd : public CWnd
{
// Construction
public:
	CUDPSocketWnd();
	CUDPSocket* m_pOwner;

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg LRESULT OnDNSLookupDone(WPARAM wParam,LPARAM lParam);
};


///////////////////////////////////////////////////////////////////////////////
// CUDPSocket

class CUDPSocket : public CAsyncSocket, public ThrottledControlSocket // ZZ:UploadBandWithThrottler (UDP)
{
	friend class CServerConnect;

public:
	CUDPSocket();
	virtual ~CUDPSocket();

	bool	Create();
    SocketSentBytes SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize); // ZZ:UploadBandWithThrottler (UDP)
	void	SendPacket(Packet* packet,CServer* host);
	void	DnsLookupDone(WPARAM wp, LPARAM lp);

protected:
	void	AsyncResolveDNS(LPCSTR lpszHostAddress, UINT nHostPort);
	HANDLE	m_DnsTaskHandle; // dns lookup handle
	
	virtual void OnSend(int nErrorCode);
	virtual void OnReceive(int nErrorCode);

private:
	HWND m_hWndResolveMessage;	// where to send WM_DNSRESOLVED
	SOCKADDR_IN m_SaveAddr;
	CUDPSocketWnd m_udpwnd;

	void 	SendBuffer();
	bool	ProcessPacket(const BYTE* packet, UINT size, UINT opcode, uint32 nIP, uint16 nUDPPort);
	void	ProcessPacketError(UINT size, UINT opcode, uint32 nIP, uint16 nTCPPort, LPCTSTR pszError);

	uint8*	m_sendbuffer;
	uint32	m_sendblen;
	CServer* m_cur_server;
	char	m_DnsHostBuffer[MAXGETHOSTSTRUCT];	// dns lookup structure

	bool	m_bWouldBlock;
	CTypedPtrList<CPtrList, SServerUDPPacket*> controlpacket_queue;

	bool	IsBusy() const { return m_bWouldBlock; }
	int		SendTo(uint8* lpBuf,int nBufLen,uint32 dwIP, uint16 nPort);

    CCriticalSection sendLocker; // ZZ:UploadBandWithThrottler (UDP)
};