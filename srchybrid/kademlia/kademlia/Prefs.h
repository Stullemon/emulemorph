/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/

#pragma once

#include "../../stdafx.h"
#include "../../Types.h"
#include "../utils/UInt128.h"

struct Status{
	uint32	m_ip;
	uint16	m_udpport;
	uint16	m_tcpport;
	bool	m_connected;
	bool	m_firewalled;
	uint8	m_totalFile;
	uint8	m_totalStore;
	uint32	m_kademliaUsers;
};

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CPrefs
{
public:
	
	/** Use this to read preferences from the default file. */
	CPrefs();

	/** Use this to read preferences from the specified file. */
	CPrefs(LPCSTR filename);

	/** Use this to use given preferences without using file. */
	CPrefs(const CUInt128 &clientID, const uint16 tcpPort, const uint16 udpPort);

	~CPrefs(void);

	void	getClientID(CUInt128 *id)		{id->setValue(m_clientID);}
	void	getClientID(CString *id)		{m_clientID.toHexString(id);}
	void	setClientID(const CUInt128 &id)	{m_clientID = id;}
	CUInt128 getClientID(void)					{return m_clientID;}

	void	getClientHash(CUInt128 *id)		{id->setValue(m_clientHash);}
	void	getClientHash(CString *id)		{m_clientHash.toHexString(id);}
	void	setClientHash(const CUInt128 &id)	{m_clientHash = id;}

	//	void	getNickName(CString *name)		{name->SetString(m_nickname);}
//	void	setNickName(const CString *name){m_nickname.SetString(*name);}

	uint32	getVersion(void)				{return m_version;}
	void	setVersion(const uint32 val)	{m_version = val;}

	uint32	getIPAddress(void)				{return m_ip;}
	void	setIPAddress(const uint32 val)	{m_ip = val;incRecheckIP();}

	uint16	getTCPPort(void)				{return m_tcpPort;}
	void	setTCPPort(const uint16 val)	{m_tcpPort = val;}

	uint16	getUDPPort(void)				{return m_udpPort;}
	void	setUDPPort(const uint16 val)	{m_udpPort = val;}

	bool	getRecheckIP(void)				{return (m_recheckip<4);}
	void	setRecheckIP()					{m_recheckip = 0;}
	void	incRecheckIP()					{m_recheckip++;}

	bool	getLastContact(void)			{return ((time(NULL) - m_lastContact) < 120);}
	void	setLastContact(void)			{m_lastContact = time(NULL);}

	bool	getFirewalled(void)				{return (m_firewalled<1);}
	void	setFirewalled()					{m_firewalled = 0;}
	void	incFirewalled()					{m_firewalled++;}

	uint8	getTotalFile(void)				{return m_totalFile;}
	void	setTotalFile(const uint8 val)	{m_totalFile = val;}

	uint8	getTotalStore(void)				{return m_totalStore;}
	void	setTotalStore(const uint8 val)	{m_totalStore = val;}

	uint32	getKademliaUsers(void)			{return m_kademliaUsers;}
	void	setKademliaUsers(const uint32 val){m_kademliaUsers = val;}

	Status*	getStatus(bool closing = false);
private:
	CString	m_filename;

	CUInt128	m_clientID;
	CUInt128	m_clientHash;
	uint32		m_ip;
	uint32		m_tcpPort;
//	CString		m_nickname;
	uint32		m_version;
//	uint32		m_adminDoorPort;
//	CString		m_adminName;
//	CString		m_adminPass;
	uint32		m_udpPort;
//	uint32		m_doorPort;
//	CString		m_incoming;
//	CString		m_temp;
//	uint32		m_saveCor;
//	uint32		m_verifyCancel;
//	uint32		m_pmAllow;
//	uint32		m_frAllow;
//	uint32		m_pType;
//	float		m_userMaxUpF;
//	float		m_userMaxDownF;
//	float		m_lineDown;
//	float		m_maxUpSpeed;
//	float		m_maxDownSpeed;
//	uint32		m_maxCon;
//	uint32		m_screenLines;
	uint32		m_recheckip;
	time_t		m_lastContact;
	uint32		m_firewalled;
	uint8		m_totalFile;
	uint8		m_totalStore;
	uint32		m_kademliaUsers;

	void init(LPCSTR filename);
	void reset(void);
	void setDefaults(void);
	void readFile(void);
	void writeFile(void);
};

} // End namespace