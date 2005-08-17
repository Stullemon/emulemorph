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

#include "MapKey.h"
#include "../../Types.h"
#include <list>
#include "SearchManager.h"
#include "../routing/Maps.h"
#include "../utils/UInt128.h"
#include "Entry.h"

typedef CTypedPtrList<CPtrList, Kademlia::CEntry*> CKadEntryPtrList;

struct Source
{
	Kademlia::CUInt128 sourceID;
	CKadEntryPtrList entryList;
};

typedef CMap<CCKey,const CCKey&,Source*,Source*> CSourceKeyMap;

struct KeyHash
{
	Kademlia::CUInt128 keyID;
	CSourceKeyMap m_Source_map;
};

typedef CTypedPtrList<CPtrList, Source*> CKadSourcePtrList;

struct SrcHash
{
	Kademlia::CUInt128 keyID;
	CKadSourcePtrList m_Source_map;
};

struct Load
{
	Kademlia::CUInt128 keyID;
	uint32 time;
};

struct SSearchTerm
{
	SSearchTerm();
	~SSearchTerm();
	
	enum ESearchTermType {
		AND,
		OR,
		NAND,
		String,
		MetaTag,
		OpGreaterEqual,
		OpLessEqual,
		OpGreater,
		OpLess,
		OpEqual,
		OpNotEqual
	} type;
	
	Kademlia::CKadTag* tag;
	CStringWArray* astr;

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

	bool AddKeyword(const CUInt128& keyWordID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8& load);
	bool AddSources(const CUInt128& keyWordID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8& load);
	bool AddNotes(const CUInt128& keyID, const CUInt128& sourceID, Kademlia::CEntry* entry, uint8& load);
	bool AddLoad(const CUInt128& keyID, uint32 time);
	uint32 GetIndexedCount() {return m_Keyword_map.GetCount();}
	uint32 GetFileKeyCount() {return m_Keyword_map.GetCount();}
	void SendValidKeywordResult(const CUInt128& keyID, const SSearchTerm* pSearchTerms, uint32 ip, uint16 port);
	void SendValidSourceResult(const CUInt128& keyID, uint32 ip, uint16 port);
	void SendValidNoteResult(const CUInt128& keyID, const CUInt128& CheckID, uint32 ip, uint16 port);
	bool SendStoreRequest(const CUInt128& keyID);
	uint32 m_totalIndexSource;
	uint32 m_totalIndexKeyword;
	uint32 m_totalIndexNotes;
	uint32 m_totalIndexLoad;

private:
	time_t m_lastClean;
	CMap<CCKey,const CCKey&,KeyHash*,KeyHash*> m_Keyword_map;
	CMap<CCKey,const CCKey&,SrcHash*,SrcHash*> m_Sources_map;
	CMap<CCKey,const CCKey&,SrcHash*,SrcHash*> m_Notes_map;
	CMap<CCKey,const CCKey&,Load*,Load*> m_Load_map;
	static CString m_sfilename;
	static CString m_kfilename;
	static CString m_loadfilename;
	void readFile(void);
	void clean(void);
};

} // End namespace