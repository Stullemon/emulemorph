#pragma once
#include "kademlia/kademlia/prefs.h"
#include "Kademlia/routing/timer.h"
#include "loggable.h"

class CKademliaMain : public CLoggable
{
public:
	CKademliaMain(void);
	~CKademliaMain(void);

	void	setStatus(Status* val);
	Status*	getStatus(void)			{return status;}
	bool	isConnected(void)		{return status->m_connected;}
	bool	isFirewalled(void)		{return status->m_firewalled;}
	void	Connect();
	void	DisConnect();
	DWORD	GetThreadID();
	void Bootstrap(CString ip,uint16 port);
	void Bootstrap(uint32 ip,uint16 port);

	uint32	getIP(void)				{return status->m_ip;}
	uint16	getUdpPort(void)		{return status->m_udpport;}
	uint16	getTcpPort(void)		{return status->m_tcpport;}
private:
	//Most likely delete these and only keep the Status Object..
	Status* status;
};
