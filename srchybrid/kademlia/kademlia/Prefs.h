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
#include "../utils/UInt128.h"
#include "Loggable.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CPrefs: public CLoggable
{
public:
	
	CPrefs(void);

	~CPrefs(void);

	void	getKadID(CUInt128 *id)		{id->setValue(m_clientID);}
	void	getKadID(CString *id)		{m_clientID.toHexString(id);}
	void	setKadID(const CUInt128 &id)	{m_clientID = id;}
	CUInt128 getKadID(void)					{return m_clientID;}

	void	getClientHash(CUInt128 *id)	{id->setValue(m_clientHash);}
	void	getClientHash(CString *id)	{m_clientHash.toHexString(id);}
	void	setClientHash(const CUInt128 &id)	{m_clientHash = id;}
	CUInt128 getClientHash(void)		{return m_clientHash;}

	uint32	getIPAddress(void)				{return m_ip;}
	void	setIPAddress(uint32 val);

	bool	getRecheckIP(void)				{return (m_recheckip<4);}
	void	setRecheckIP(void)					{m_recheckip = 0;setFirewalled();}
	void	incRecheckIP(void)					{m_recheckip++;}

	bool	getLastContact(void);
	void	setLastContact(void)			{m_lastContact = time(NULL);}
	bool	hasLostConnection(void);

	bool	getFirewalled(void);
	void	setFirewalled(void);
	void	incFirewalled(void);

	uint8	getTotalFile(void)				{return m_totalFile;}
	void	setTotalFile(uint8 val)			{m_totalFile = val;}

	uint8	getTotalStoreSrc(void)			{return m_totalStoreSrc;}
	void	setTotalStoreSrc(uint8 val)		{m_totalStoreSrc = val;}

	uint8	getTotalStoreKey(void)			{return m_totalStoreKey;}
	void	setTotalStoreKey(uint8 val)		{m_totalStoreKey = val;}

	uint8	getTotalSource(void)			{return m_totalSource;}
	void	setTotalSource(uint8 val)		{m_totalSource = val;}

	uint32	getKademliaUsers(void)			{return m_kademliaUsers;}
	void	setKademliaUsers(uint32 val)	{m_kademliaUsers = val;}

	uint32	getKademliaFiles(void)			{return m_kademliaFiles;}
	void	setKademliaFiles(void);

	bool	getPublish(void)				{return m_Publish;}
	void	setPublish(bool val)			{m_Publish = val;}

	bool	getFindBuddy(void);
	void	setFindBuddy( bool val = true)	{m_findBuddy = val;}

private:
	CString	m_filename;

	time_t		m_lastContact;
	CUInt128	m_clientID;
	CUInt128	m_clientHash;
	uint32		m_ip;
	uint32		m_ipLast;
	uint32		m_recheckip;
	uint32		m_firewalled;
	uint32		m_kademliaUsers;
	uint32		m_kademliaFiles;
	uint8		m_totalFile;
	uint8		m_totalStoreSrc;
	uint8		m_totalStoreKey;
	uint8		m_totalSource;
	bool		m_Publish;
	bool		m_findBuddy;
	bool		m_lastFirewallState;

	void init(LPCTSTR filename);
	void reset(void);
	void setDefaults(void);
	void readFile(void);
	void writeFile(void);
};

} // End namespace