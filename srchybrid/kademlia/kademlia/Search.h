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
#include "../routing/Maps.h"

class CKnownFile;
class CSafeMemFile;

namespace Kademlia
{
	void deleteTagListEntries(TagList* plistTag);
	class CByteIO;
	class CSearch
	{
			friend class CSearchManager;
		public:
			 // netfinity: Made some inline for performance reasons
			uint32 GetSearchID() const {return m_uSearchID;}
			uint32 GetSearchTypes() const {return m_uType;}
			void SetSearchTypes( uint32 uVal ) {m_uType = uVal;}
			void SetTargetID( CUInt128 uVal ) {m_uTarget = uVal;}
			uint32 GetAnswers() const;
			uint32 GetKadPacketSent() const {return m_uKadPacketSent;}
			uint32 GetRequestAnswer() const {return m_uTotalRequestAnswers;}
			void StorePacket(CContact* pContact); // netf
			const CString& GetFileName() const {return m_sFileName;}
			void SetFileName(const CString& sFileName) {m_sFileName = sFileName;}
			CUInt128 GetTarget() const {return m_uTarget;}
			void AddFileID(const CUInt128& uID);
			void PreparePacketForTags( CByteIO* pbyPacket, CKnownFile* pFile );
			bool Stoping() const {return m_bStoping;}
			uint32 GetNodeLoad() const;
			uint32 GetNodeLoadResonse() const {return m_uTotalLoadResponses;}
			uint32 GetNodeLoadTotal() const {return m_uTotalLoad;}
			void UpdateNodeLoad( uint8 uLoad );
			void SetSearchTermData( uint32 uSearchTermDataSize, LPBYTE pucSearchTermsData );
			enum
			{
			    NODE,
			    NODECOMPLETE,
			    FILE,
			    KEYWORD,
			    NOTES,
			    STOREFILE,
			    STOREKEYWORD,
			    STORENOTES,
			    FINDBUDDY,
			    FINDSOURCE
		};

			CSearch();
			~CSearch();

		private:
			void Go();
			void ProcessResponse(uint32 uFromIP, uint16 uFromPort, ContactList *plistResults);
			void ProcessResult(const CUInt128 &uAnswer, TagList *listInfo);
			void ProcessResultFile(const CUInt128 &uAnswer, TagList *listInfo);
			void ProcessResultKeyword(const CUInt128 &uAnswer, TagList *listInfo);
			void ProcessResultNotes(const CUInt128 &uAnswer, TagList *listInfo);
			void JumpStart();
			void SendFindValue(CContact* pContact);
			void PrepareToStop();

			CUInt128 GetSearchDistance(); // netfinity: Safe KAD - Calculate the search coverage to find the 3 "alive" closest nodes

			bool m_bStoping;
			time_t m_tCreated;
			uint32 m_uType;
			uint32 m_uAnswers;
			uint32 m_uTotalRequestAnswers;
			uint32 m_uKadPacketSent; //Used for gui reasons.. May not be needed later..
			uint32 m_uTotalLoad;
			uint32 m_uTotalLoadResponses;
			uint32 m_uLastResponse;
			uint32 m_uSearchID;
			uint32 m_uSearchTermsDataSize;
			LPBYTE m_pucSearchTermsData;
			CUInt128 m_uTarget;
			WordList m_listWords;
			CString m_sFileName;
			UIntList m_listFileIDs;
			ContactMap m_mapPossible;
			ContactMap m_mapTried;
			ContactMap m_mapRetried; // netfinity: Safe KAD - Sometimes it's good to try twice
			ContactMap m_mapResponded;
			ContactMap m_mapUseful; // netfinity: Safe KAD - Track the nodes who gave us some contacts (not filtered)
			//ContactMap m_mapBest; // netfinity: Safe KAD - Obsoleted
			ContactList m_listDelete;
			ContactMap m_mapInUse;
	};
}
void KadGetKeywordHash(const CStringW& rstrKeywordW, Kademlia::CUInt128* puKadID);
void KadGetKeywordHash(const CStringA& rstrKeywordA, Kademlia::CUInt128* puKadID);
CStringA KadGetKeywordBytes(const CStringW& rstrKeywordW);
