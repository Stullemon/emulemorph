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
#include "../utils/UInt128.h"
#include "../utils/MiscUtils.h"
#include "../kademlia/SearchManager.h"
#include "../../opcodes.h"
#include "../Routing/RoutingZone.h"
#include "../kademlia/kademlia.h"
#include "preferences.h"
#include "emule.h"
#include "emuledlg.h"
#include "SafeFile.h"

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
	filename.Append("preferencesK.dat");
	init(filename.GetBuffer(0));
}

CPrefs::~CPrefs(void)
{
	if (m_filename.GetLength() > 0)
		writeFile();
}

void CPrefs::init(LPCSTR filename)
{
	m_clientID.setValueRandom();
	m_lastContact = 0;
	m_recheckip = 0;
	m_firewalled = 0;
	m_totalFile = 0;
	m_totalStoreSrc = 0;
	m_totalStoreKey = 0;
	m_Publish = false;
	m_clientHash.setValue((uchar*)thePrefs.GetUserHash());
	m_ip			= 0;
	m_recheckip		= 0;
	m_firewalled	= 0;
	m_kademliaUsers	= 0;
	m_filename = filename;
	readFile();
}

void CPrefs::readFile(void)
{
	try
	{
		CSafeBufferedFile file;
		CFileException fexp;
		if (file.Open(m_filename.GetBuffer(0),CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary, &fexp))
		{
			setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
			m_ip = file.ReadUInt32();
			file.ReadUInt16();
			file.ReadUInt128(&m_clientID);
			file.Close();
		}
	}
	//TODO: Make this catch an CFileException..
	catch (...) 
	{
		TRACE("Exception in CPrefs::readFile\n");
	}
}

void CPrefs::writeFile(void)
{
	try
	{
		CSafeBufferedFile file;
		CFileException fexp;
		if (file.Open(m_filename.GetBuffer(0), CFile::modeWrite|CFile::modeCreate|CFile::typeBinary, &fexp))
		{
			setvbuf(file.m_pStream, NULL, _IOFBF, 16384);
			file.WriteUInt32(m_ip);
			file.WriteUInt16(0); //This is no longer used.
			file.WriteUInt128(&m_clientID);
			file.WriteUInt8(0); //This is to tell older clients there are no tags..
			file.Close();
		}
	} 
	//TODO: Make this catch an CFileException 
	catch (...) 
	{
		TRACE("Exception in CPrefs::writeFile\n");
	}
}

bool CPrefs::getLastContact(void)
{
	return ((time(NULL) - m_lastContact) < KADEMLIADISCONNECTDELAY);
}

void CPrefs::setFirewalled(void)
{
	m_firewalled = 0;
	theApp.emuledlg->ShowConnectionState();
}

void CPrefs::incFirewalled(void)
{
	m_firewalled++;
	theApp.emuledlg->ShowConnectionState();
}