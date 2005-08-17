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
#include "Entry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CEntry::CEntry()
{
	ip = 0;
	tcpport = 0;
	udpport = 0;
	(void)fileName;
	size = 0;
	lifetime = 0;
	source = false;
}

CEntry::~CEntry()
{
	TagList::const_iterator it;
	for (it = taglist.begin(); it != taglist.end(); it++)
		delete *it;
}

CEntry* CEntry::Copy()
{
	CEntry* pEntry = new CEntry();
	pEntry->fileName = fileName;
	pEntry->ip = ip;
	pEntry->keyID.setValue(keyID);
	pEntry->lifetime = lifetime;
	pEntry->size = size;
	pEntry->source = source;
	pEntry->sourceID.setValue(sourceID);
	pEntry->tcpport = tcpport;
	pEntry->udpport = udpport;
	TagList::const_iterator it;
	Kademlia::CKadTag* tag;
	for (it = taglist.begin(); it != taglist.end(); it++)
	{
		tag = *it;
		pEntry->taglist.push_back(tag->Copy());
	}
	return pEntry;
}

uint32 CEntry::GetIntTagValue(LPCSTR tagname) const
{
	TagList::const_iterator it;
	Kademlia::CKadTag* tag;
	for (it = taglist.begin(); it != taglist.end(); it++)
	{
		tag = *it;
		if (!tag->m_name.Compare(tagname)&& tag->IsInt())
			return tag->GetInt();
	}
	return 0;
}

CStringW CEntry::GetStrTagValue(LPCSTR tagname) const
{
	TagList::const_iterator it;
	Kademlia::CKadTag* tag;
	for (it = taglist.begin(); it != taglist.end(); it++)
	{
		tag = *it;
		if (!tag->m_name.Compare(tagname)&& tag->IsStr())
			return tag->GetStr();
	}
	return _T("");
}
