/*
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

#include "Indexed.h"
#include "../../Types.h"
#include <list>
#include "SearchManager.h"
#include "../routing/Maps.h"
#include "../utils/UInt128.h"
#include "Entry.h"

struct Source{
	Kademlia::CUInt128 sourceID;
	CTypedPtrList<CPtrList, Kademlia::CEntry*> entryList;
};

struct KeywordHash{
	Kademlia::CUInt128 keyID;
	CTypedPtrList<CPtrList, Source*> sourceList;
};

struct SSearchTerm
{
	SSearchTerm();
	~SSearchTerm();
	
	enum {
		AND,
		OR,
		NAND,
		String,
		MetaTag,
		Min,
		Max,
	} type;
	
	Kademlia::CTag* tag;
	CString* str;

	SSearchTerm* left;
	SSearchTerm* right;
};

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CIndexed
{

public:
	CIndexed();
	~CIndexed();
	bool IndexedAdd( Kademlia::CUInt128 keyWordID, Kademlia::CUInt128 sourceID, Kademlia::CEntry* entry);
	uint32 GetIndexedCount() {return keywordHashList.GetCount();}
	void SendValidResult( Kademlia::CUInt128 keyWordID, const SSearchTerm* pSearchTerms, const sockaddr_in *senderAddress, bool source );
	uint32 m_totalIndexSource;
	uint32 m_totalIndexKeyword;
private:
	time_t m_lastClean;
	CTypedPtrList<CPtrList, KeywordHash*> keywordHashList;
	static CString m_filename;
	void readFile(void);
	void writeFile(void);
	void clean(void);
};

} // End namespace