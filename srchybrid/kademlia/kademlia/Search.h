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

#include "Search.h"

#include "../../Types.h"

#include "SearchManager.h"
#include "../routing/Maps.h"
#include "../utils/UInt128.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CSearch
{
	friend class CSearchManager;

public:
	uint32 getSearchID() {return m_searchID;}
	uint32 getSearchTypes() {return m_type;}
	void setSearchTypes( uint32 val ) {m_type = val;}
	uint32 getCount() {return m_count;}
	uint32 getKeywordCount() {return m_keywordcount;}
	void setKeywordCount(uint32 val) {m_keywordcount = val;}
	CUInt128	m_keywordPublish; //Need to make this private...
	CString getFileName(void) {return m_fileName;}
	CUInt128 getTarget(void) {return m_target;}

	enum
	{
		NODE,
		NODECOMPLETE,
		FILE,
		KEYWORD,
		STOREFILE,
		STOREKEYWORD
	};

private:
	CSearch();
	~CSearch();

	void go(void);
	void processResponse(const CUInt128 &target, uint32 fromIP, uint16 fromPort, ContactList *results);
	void processResult(const CUInt128 &target, uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagList *info);
	void processResultFile(const CUInt128 &target, uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagList *info);
	void processResultKeyword(const CUInt128 &target, uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagList *info);
	void jumpStart(void);
	void sendFindValue(const CUInt128 &target, const CUInt128 &check, uint32 ip, uint16 port);

	time_t		m_created;
	uint32		m_type;
	uint32		m_count; //Used for gui reasons.. May not be needed later..
	uint32		m_keywordcount; //Used for gui reasons.. May not be needed later..

	SEARCH_ID_CALLBACK		m_callbackID;
	SEARCH_KEYWORD_CALLBACK	m_callbackKeyword;

	uint32		m_searchID;
	CUInt128	m_target;
	byte		*m_searchTerms;
	uint32		m_lenSearchTerms;
	WordList	m_words;
	CString		m_fileName;

	ContactMap	m_possible;
	ContactMap	m_tried;
	ContactMap	m_responded;
	ContactMap	m_best;
	ContactList	m_delete;
};

} // End namespace