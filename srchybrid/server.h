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

class CTag;
class CFileDataIO;

struct IPRange_Struct2; //EastShare - added by AndCycle, IP to Country

#pragma pack(1)
struct ServerMet_Struct {
	uint32	ip;
	uint16	port;
	uint32	tagcount;
};
#pragma pack()

#define SRV_PR_LOW			2
#define SRV_PR_NORMAL		0
#define SRV_PR_HIGH			1


// Server TCP flags
#define	SRV_TCPFLG_COMPRESSION	0x00000001

// Server UDP flags
#define	SRV_UDPFLG_EXT_GETSOURCES	0x00000001
#define	SRV_UDPFLG_EXT_GETFILES		0x00000002

class CServer{
public:
	CServer(const ServerMet_Struct* in_data);
	CServer(uint16 in_port, LPCSTR i_addr);
	CServer(const CServer* pOld);
	~CServer();

	void	AddTag(CTag* in_tag)					{taglist->AddTail(in_tag);}
	LPCSTR	GetListName() const						{return listname;}
	LPCSTR	GetFullIP() const						{return ipfull;}
	LPCSTR	GetAddress() const;
	//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
	/*
	uint16	GetPort() const							{return port;}
	*/
	uint16	GetPort() const							{return realport ? realport : port;}
	uint16	GetConnPort() const						{return port;}
	void    SetPort(uint32 val)						{ realport = val;}
	//Morph End - added by AndCycle, aux Ports, by lugdunummaster
	bool	AddTagFromFile(CFileDataIO* servermet);
	void	SetListName(LPCSTR newname);
	void	SetDescription(LPCSTR newdescription);
	uint32	GetIP() const							{return ip;}
	uint32	GetFiles() const						{return files;}
	uint32	GetUsers() const						{return users;}
	LPCSTR	GetDescription() const					{return description;}
	uint32	GetPing() const							{return ping;}
	uint32	GetPreferences() const					{return preferences;}
	uint32	GetMaxUsers() const						{return maxusers;}
	void	SetMaxUsers(uint32 in_maxusers) 		{maxusers = in_maxusers;}
	void	SetUserCount(uint32 in_users)			{users = in_users;}
	void	SetFileCount(uint32 in_files)			{files = in_files;}
	void	ResetFailedCount()						{failedcount = 0;} 
	void	AddFailedCount()						{failedcount++;} 
	uint32	GetFailedCount() const					{return failedcount;}
	void	SetID(uint32 newip);
	LPCSTR	GetDynIP() const						{return dynip;}
	bool	HasDynIP() const						{return dynip;}
	void	SetDynIP(LPCSTR newdynip);
	uint32	GetLastPingedTime() const				{return lastpingedtime;}
	void	SetLastPingedTime(uint32 in_lastpingedtime)	{lastpingedtime = in_lastpingedtime;}
	uint32	GetLastPinged() const					{return lastpinged;}
	void	SetLastPinged(uint32 in_lastpinged)		{lastpinged = in_lastpinged;}
	uint8	GetLastDescPingedCount() const			{return lastdescpingedcout;}
	void	SetLastDescPingedCount(bool reset);
	void	SetPing(uint32 in_ping)					{ping = in_ping;}
	void	SetPreference(uint32 in_preferences)	{preferences = in_preferences;}
	void	SetIsStaticMember(bool in)				{staticservermember=in;}
	bool	IsStaticMember() const					{return staticservermember;}
	uint32	GetChallenge() const					{return challenge;}
	void	SetChallenge(uint32 in_challenge)		{challenge = in_challenge;}
	uint32	GetDescReqChallenge() const				{return m_uDescReqChallenge;}
	void	SetDescReqChallenge(uint32 uDescReqChallenge) {m_uDescReqChallenge = uDescReqChallenge;}
	uint32	GetSoftFiles() const					{return softfiles;}
	void	SetSoftFiles(uint32 in_softfiles)		{softfiles = in_softfiles;}
	uint32	GetHardFiles() const					{return hardfiles;}
	void	SetHardFiles(uint32 in_hardfiles)		{hardfiles = in_hardfiles;}
	const CString& GetVersion() const				{return m_strVersion;}
	void	SetVersion(LPCTSTR pszVersion)			{m_strVersion = pszVersion;}
	void	SetTCPFlags(uint32 uFlags)				{m_uTCPFlags = uFlags;}
	uint32	GetTCPFlags() const						{return m_uTCPFlags;}
	void	SetUDPFlags(uint32 uFlags)				{m_uUDPFlags = uFlags;}
	uint32	GetUDPFlags() const						{return m_uUDPFlags;}

private:
	uint32		challenge;
	uint32		m_uDescReqChallenge;
	uint32		lastpinged; //This is to get the ping delay.
	uint32		lastpingedtime; //This is to decided when we retry the ping.
	uint32		lastdescpingedcout;
	uint32		files;
	uint32		users;
	uint32		maxusers;
	uint32		softfiles;
	uint32		hardfiles;
	uint32		preferences;
	uint32		ping;
	LPCSTR		description;
	LPCSTR		listname;
	LPCSTR		dynip;
	uint32		tagcount;
	CHAR		ipfull[3+1+3+1+3+1+3+1]; // 16
	uint32		ip;
	uint16		port;
	uint16		realport;//Morph - added by AndCycle, aux Ports, by lugdunummaster
	uint8		staticservermember;
	uint32		failedcount; 
	CTypedPtrList<CPtrList, CTag*>*	taglist;
	CString		m_strVersion;
	uint32		m_uTCPFlags;
	uint32		m_uUDPFlags;

//EastShare Start - added by AndCycle, IP to Country
public:
	CString	GetCountryName() const;
	int		GetCountryFlagIndex() const;
	void	ResetIP2Country();

private:
	struct	IPRange_Struct2* m_structServerCountry;
 //EastShare End - added by AndCycle, IP to Country
};