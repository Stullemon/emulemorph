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

#include "stdafx.h"
#include "Prefs.h"
#include "../io/ByteIO.h"
#include "../io/FileIO.h"
#include "Tag.h"
#include "../utils/UInt128.h"
#include "../utils/MiscUtils.h"
#include "../kademlia/SearchManager.h"
#include "../../opcodes.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CPrefs::CPrefs()
{
	CString filename = CMiscUtils::getAppDir();
	filename.Append(_T(CONFIGFOLDER));
	filename.Append("preferencesKad.dat");
	init(filename.GetBuffer(0));
}

CPrefs::CPrefs(LPCSTR filename)
{
	init(filename);
}

CPrefs::CPrefs(const CUInt128 &clientID, const uint16 tcpPort, const uint16 udpPort)
{
	reset();
	
	m_clientID = clientID;
	m_tcpPort = tcpPort;
	m_udpPort = udpPort;

	setDefaults();
}

CPrefs::~CPrefs(void)
{
	if (m_filename.GetLength() > 0)
		writeFile();
}

void CPrefs::init(LPCSTR filename)
{
	m_filename = filename;
	reset();
	readFile();
	setDefaults();
}

void CPrefs::setDefaults(void)
{
	// Set default values for anything not found
//	if (m_nickname.GetLength() == 0)
//		m_nickname = "http://emule-project.net";
	if (m_clientID == NULL)
		m_clientID.setValueRandom();
	if (m_tcpPort == 0)
		m_tcpPort = 4662;
	if (m_udpPort == 0)
		m_udpPort = 4673;
//	if (m_incoming.GetLength() == 0)
//	{
//		m_incoming = CMiscUtils::getAppDir();
//		m_incoming += "incoming";
//	}
//	if (m_temp.GetLength() == 0)
//	{
//		m_temp = CMiscUtils::getAppDir();
//		m_temp += "temp";
//	}
	if (m_version == 0)
		m_version = 0x3C;
//	if (m_maxCon == 0)
//		m_maxCon = 50;
//	if (m_screenLines == 0)
//		m_screenLines = 24;
	m_lastContact = 0;
	m_recheckip = 0;
	m_firewalled = 0;
	m_totalFile = 0;
	m_totalStore = 0;
}

void CPrefs::reset(void)
{
	m_clientHash	= NULL;
	m_clientID		= NULL;
	m_ip			= 0;
	m_tcpPort		= 0;
//	m_nickname		= "";
	m_version		= 0;
//	m_adminDoorPort	= 0;
//	m_adminName		= "";
//	m_adminPass		= "";
	m_tcpPort		= 0;
	m_udpPort		= 0;
//	m_doorPort		= 0;
//	m_incoming		= "";
//	m_temp			= "";
//	m_saveCor		= 0;
//	m_verifyCancel	= 0;
//	m_pmAllow		= 0;
//	m_frAllow		= 0;
//	m_pType			= 0;
//	m_userMaxUpF	= 0.0f;
//	m_userMaxDownF	= 0.0f;
//	m_lineDown		= 0.0f;
//	m_maxUpSpeed	= 0.0f;
//	m_maxDownSpeed	= 0.0f;
//	m_maxCon		= 0;
//	m_screenLines	= 0;
	m_recheckip		= 0;
	m_firewalled	= 0;
	m_kademliaUsers	= 0;
}

void CPrefs::readFile(void)
{
	try
	{
		CFileIO file;
		if (file.Open(m_filename.GetBuffer(0), CFile::modeRead))
		{

			m_ip			= file.readUInt32();
			m_tcpPort		= file.readUInt16();

			if (m_clientID == NULL)
				file.readUInt128(&m_clientID);

			// Do this twice
//			for (int i=0; i<2; i++)
//			{
				TagList *tags = file.readTagList();
				CTag *tag;
				TagList::const_iterator it;
				for (it = tags->begin(); it != tags->end(); it++)
				{
					tag = *it;

					if (!tag->m_name.Compare(TAG_VERSION))
						m_version		= tag->GetInt();
//					else if (!tag->m_name.Compare(TAG_NAME))
//						m_nickname		= ((CTagStr *)tag)->m_value;
//					else if (!tag->m_name.Compare("adminDoorPort"))
//						m_adminDoorPort	= ((CTagUInt32 *)tag)->m_value;
//					else if (!tag->m_name.Compare("adminName"))
//						m_adminName		= ((CTagStr *)tag)->m_value;
//					else if (!tag->m_name.Compare("adminPass"))
//						m_adminPass		= ((CTagStr *)tag)->m_value;
//					else if (!tag->m_name.Compare("annexPort"))
//						m_udpPort		= ((CTagUInt32 *)tag)->m_value;
//					else if (!tag->m_name.Compare("doorPort"))
//						m_doorPort		= ((CTagUInt32 *)tag)->m_value;
//					else if (!tag->m_name.Compare("incoming"))
//						m_incoming		= ((CTagStr *)tag)->m_value;
//					else if (!tag->m_name.Compare("temp"))
//						m_temp			= ((CTagStr *)tag)->m_value;
//					else if (!tag->m_name.Compare("saveCor"))
//						m_saveCor		= ((CTagUInt32 *)tag)->m_value;
//					else if (!tag->m_name.Compare("verifyCancel"))
//						m_verifyCancel	= ((CTagUInt32 *)tag)->m_value;
//					else if (!tag->m_name.Compare("pmAllow"))
//						m_pmAllow		= ((CTagUInt32 *)tag)->m_value;
//					else if (!tag->m_name.Compare("frAllow"))
//						m_frAllow		= ((CTagUInt32 *)tag)->m_value;
//					else if (!tag->m_name.Compare("pType"))
//						m_pType			= ((CTagUInt32 *)tag)->m_value;
//					else if (!tag->m_name.Compare("userMaxUpF"))
//						m_userMaxUpF	= ((CTagFloat *)tag)->m_value;
//					else if (!tag->m_name.Compare("userMaxDownF"))
//						m_userMaxDownF	= ((CTagFloat *)tag)->m_value;
//					else if (!tag->m_name.Compare("lineDown"))
//						m_lineDown		= ((CTagFloat *)tag)->m_value;
//					else if (!tag->m_name.Compare("maxUpSpeed"))
//						m_maxUpSpeed	= ((CTagFloat *)tag)->m_value;
//					else if (!tag->m_name.Compare("maxDownSpeed"))
//						m_maxDownSpeed	= ((CTagFloat *)tag)->m_value;
//					else if (!tag->m_name.Compare("maxCon"))
//						m_maxCon		= ((CTagUInt32 *)tag)->m_value;
//					else if (!tag->m_name.Compare("screenLines"))
//						m_screenLines	= ((CTagUInt32 *)tag)->m_value;

					delete tag;
				}
				delete tags;
//			}
			file.Close();
		}
	} catch (...) {}
}

void CPrefs::writeFile(void)
{
	try
	{
		CFileIO file;
		if (file.Open(m_filename.GetBuffer(0), CFile::modeWrite | CFile::modeCreate))
		{
			file.writeUInt32(m_ip);
			file.writeUInt16(m_tcpPort);
			file.writeUInt128(m_clientID);
			//file.writeUInt32LE(1);
			file.writeByte(1);
//			file.writeTag("pType",			m_pType);
			file.writeTag(TAG_VERSION, m_version);
//			file.writeUInt32LE(19);
//			file.writeTag(TAG_NAME, m_nickname);
//			file.writeTag("adminDoorPort",	m_adminDoorPort);
//			file.writeTag("adminName",		m_adminName);
//			file.writeTag("adminPass",		m_adminPass);
//			file.writeTag("annexPort",		m_udpPort);
//			file.writeTag("doorPort",		m_doorPort);
//			file.writeTag("incoming",		m_incoming);
//			file.writeTag("temp",			m_temp);
//			file.writeTag("saveCor",		m_saveCor);
//			file.writeTag("verifyCancel",	m_verifyCancel);
//			file.writeTag("pmAllow",		m_pmAllow);
//			file.writeTag("frAllow",		m_frAllow);
//			file.writeTag("userMaxUpF",		m_userMaxUpF);
//			file.writeTag("userMaxDownF",	m_userMaxDownF);
//			file.writeTag("lineDown",		m_lineDown);
//			file.writeTag("maxUpSpeed",		m_maxUpSpeed);
//			file.writeTag("maxDownSpeed",	m_maxDownSpeed);
//			file.writeTag("maxCon",			m_maxCon);
//			file.writeTag("screenLines",	m_screenLines);
			file.Close();
		}
	} catch (...) {}
}

Status* CPrefs::getStatus(bool closing){
	CSearchManager::updateStats();
	Status* status = new Status;
	if(closing)
	{
		status->m_connected = false;
		status->m_firewalled = 0;
	}
	else
	{
		status->m_connected = getLastContact();
		status->m_firewalled = getFirewalled();
	}
	status->m_ip = getIPAddress();
	status->m_udpport = getUDPPort();
	status->m_tcpport = getTCPPort();
	status->m_totalFile = getTotalFile();
	status->m_totalStore = getTotalStore();
	status->m_kademliaUsers = getKademliaUsers();
	return status;
}