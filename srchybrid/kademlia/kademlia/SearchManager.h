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
#include <map>
#include <list>
#include "../utils/UInt128.h"
#include "../routing/Maps.h"


////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CSearch;
class CRoutingZone;
class CTimer;
class CTag;
typedef std::list<CTag*> TagList;

// If type is unknown it will be an empty string
// If there are any properties about the file to report, there will follow LPCSTR key/value pairs.
typedef void (CALLBACK *SEARCH_KEYWORD_CALLBACK)(uint32 searchID, CUInt128 fileID, LPCSTR name, uint32 size, LPCSTR type, uint16 numProperties, ...);
typedef void (CALLBACK *SEARCH_ID_CALLBACK)(uint32 searchID, CUInt128 contactID, uint8 type, uint32 ip, uint16 tcp, uint16 udp, uint32 serverip, uint16 port);

typedef std::list<CString> WordList;
typedef std::map<CUInt128, CSearch*> SearchMap;

#define SEARCH_IMAGE	"-image"
#define SEARCH_AUDIO	"-audio"
#define SEARCH_VIDEO	"-video"
#define SEARCH_DOC		"-doc"
#define SEARCH_PRO		"-pro"

class CSearchManager
{
	friend class CRoutingZone;
	friend class CTimer;

public:

	static void stopSearch(uint32 searchID);
	static void stopAllSearches(void);

	// Search for a particular file
	// Will return unique search id, returns zero if already searching for this file.
	static CSearch* prepareFindFile(SEARCH_ID_CALLBACK callback, const CUInt128 &id);

	// Pass a list of keywords and restrictions.
	// eg:  uint32 searchID = findFile(MyCallback, 2, "Britney", SEARCH_IMAGE);
	// Will return unique search id, returns zero if already searching for this keyword.
	static CSearch* prepareFindKeywords(SEARCH_KEYWORD_CALLBACK callback, uint32 numParams, LPCSTR keyword1, ...);
	static CSearch* prepareFindKeywords(SEARCH_KEYWORD_CALLBACK callback, LPCSTR keyword1, UINT uSearchTermsSize, LPBYTE pucSearchTermsData);

	static void startSearch(CSearch* pSearch);
	static void deleteSearch(CSearch* pSearch);

	static void processResponse(const CUInt128 &target, uint32 fromIP, uint16 fromPort, ContactList *results);
	static void processResult(const CUInt128 &target, uint32 fromIP, uint16 fromPort, const CUInt128 &answer, TagList *info);
	static void processPublishResult(const CUInt128 &target);

	static void getWords(LPCSTR str, WordList *words);

	static void updateStats(void);

	static bool isNodeSearch(const CUInt128 &target);
private:

	static void findNode(const CUInt128 &id);
	static void findNodeComplete(const CUInt128 &id);
	static bool alreadySearchingFor(const CUInt128 &target);

	static uint32 m_nextID;
	static SearchMap m_searches;
	static CCriticalSection m_critical;

	static void jumpStart(void);
};

} // End namespace
