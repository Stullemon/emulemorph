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
#include "MiscUtils.h"
#include "../kademlia/Kademlia.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CString CMiscUtils::appDirectory;

void CMiscUtils::ipAddressToString(uint32 ip, CString *string)
{
	string->Format(_T("%ld.%ld.%ld.%ld"), 
					((ip >> 24) & 0xFF), 
					((ip >> 16) & 0xFF), 
					((ip >>  8) & 0xFF), 
					((ip      ) & 0xFF) );
}

LPCTSTR CMiscUtils::getAppDir(void)
{
	if (appDirectory.GetLength() == 0)
	{
		TCHAR *buffer = new TCHAR[MAX_PATH];
		GetModuleFileName(0, buffer, MAX_PATH);
		LPTSTR end = _tcsrchr(buffer, _T('\\')) + 1;
		*end = _T('\0');
		appDirectory = buffer;
		delete [] buffer;
	}
	return appDirectory;
}

void CMiscUtils::debugHexDump(const byte *data, uint32 lenData)
{
#ifdef DEBUG
	try
	{
		uint16 lenLine = 16;
		uint32 pos = 0;
		byte c = 0;

		while (pos < lenData)
		{
			CStringA line;
			CStringA single;
			line.Format("%08X ", pos);
			lenLine = min((lenData - pos), 16);
			for (int i=0; i<lenLine; i++)
			{
				single.Format(" %02X", data[pos+i]);
				line += single;
				if (i == 7)
					line += " ";
			}
			line += CString(' ', 60 - line.GetLength());
			for (int i=0; i<lenLine; i++)
			{
				c = data[pos + i];
				single.Format("%c", (((c > 31) && (c < 127)) ? c : '.'));
				line += single;
			}
			//JOHNTODO Is this method Unicode friendly?
			AddDebugLogLine(false, _T("%s"), line);
			pos += lenLine;
		}
	} 
	catch (...)
	{
		AddDebugLogLine(false, _T("Exception in CMiscUtils::debugHexDump\n"));
	}
#endif
}

