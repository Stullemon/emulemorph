//this file is part of eMule
//Copyright (C)2002-2004 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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

#include "StdAfx.h"
#include "corruptionblackbox.h"
#include "knownfile.h"
#include "updownclient.h"
#include "log.h"
#include "emule.h"
#include "clientlist.h"
#include "opcodes.h"

#define	 CBB_BANTHRESHOLD	32 //% max corrupted data	

CCBBRecord::CCBBRecord(uint32 nStartPos, uint32 nEndPos, uint32 dwIP, EBBRStatus BBRStatus){
	if (nStartPos > nEndPos){
		ASSERT( false );
		return;
	}
	m_nStartPos = nStartPos;
	m_nEndPos = nEndPos;
	m_dwIP = dwIP;
	m_BBRStatus = BBRStatus;
}

CCBBRecord& CCBBRecord::operator=(const CCBBRecord& cv)
{
	m_nStartPos = cv.m_nStartPos;
	m_nEndPos = cv.m_nEndPos;
	m_dwIP = cv.m_dwIP;
	m_BBRStatus = cv.m_BBRStatus;
	return *this; 
}

bool CCBBRecord::Merge(uint32 nStartPos, uint32 nEndPos, uint32 dwIP, EBBRStatus BBRStatus){

	if (m_dwIP == dwIP && m_BBRStatus == BBRStatus && (nStartPos == m_nEndPos + 1 || nEndPos + 1 == m_nStartPos)){
		if (nStartPos == m_nEndPos + 1)
			m_nEndPos = nEndPos;
		else if (nEndPos + 1 == m_nStartPos)
			m_nStartPos = nStartPos;
		else
			ASSERT( false );

		return true;
	}
	else
		return false;
}

bool CCBBRecord::CanMerge(uint32 nStartPos, uint32 nEndPos, uint32 dwIP, EBBRStatus BBRStatus){

	if (m_dwIP == dwIP && m_BBRStatus == BBRStatus && (nStartPos == m_nEndPos + 1 || nEndPos + 1 == m_nStartPos)){
		return true;
	}
	else
		return false;
}

void CCorruptionBlackBox::Init(uint32 nFileSize) {
	m_aaRecords.SetSize(((uint64)nFileSize + (PARTSIZE - 1)) / PARTSIZE);
}

void CCorruptionBlackBox::Free() {
	m_aaRecords.RemoveAll();
	m_aaRecords.FreeExtra();
}

void CCorruptionBlackBox::TransferredData(uint32 nStartPos, uint32 nEndPos, const CUpDownClient* pSender){
	if (nEndPos - nStartPos >= PARTSIZE){
		ASSERT( false );
		return;
	}
	if (nStartPos > nEndPos){
		ASSERT( false );
		return;
	}
	if(!pSender) return; //MORPH - Added by SiRoB, Import Parts [SR13]
	uint32 dwSenderIP = pSender->GetIP();
	// we store records seperated for each part, so we don't have to search all entries everytime
	
	// convert pos to relative block pos
	uint16 nPart = nStartPos / PARTSIZE;
	uint32 nRelStartPos = nStartPos - nPart*PARTSIZE;
	uint32 nRelEndPos = nEndPos - nPart*PARTSIZE;
	if (nRelEndPos >= PARTSIZE){
		// data crosses the partborder, split it
		nRelEndPos = PARTSIZE-1;
		uint32 nTmpStartPos = nPart*PARTSIZE + nRelEndPos + 1;
		ASSERT( nTmpStartPos % PARTSIZE == 0); // remove later
		TransferredData(nTmpStartPos, nEndPos, pSender);
	}
	if (nPart >= m_aaRecords.GetCount()){
		//ASSERT( false );
		m_aaRecords.SetSize(nPart+1);
	}
	int posMerge = -1;
	uint32 ndbgRewritten = 0;
	for (int i= 0; i < m_aaRecords[nPart].GetCount(); i++){
		if (m_aaRecords[nPart][i].CanMerge(nRelStartPos, nRelEndPos, dwSenderIP, BBR_NONE)){
			posMerge = i;
		}
		// check if there is already an pending entry and overwrite it
		else if (m_aaRecords[nPart][i].m_BBRStatus == BBR_NONE){
			if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos){
				// old one is included in new one -> delete
				ndbgRewritten += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart].RemoveAt(i);
				i--;
			}
			else if (m_aaRecords[nPart][i].m_nStartPos < nRelStartPos && m_aaRecords[nPart][i].m_nEndPos > nRelEndPos){
			    // old one includes new one
				// check if the old one and new one have the same ip
				if (dwSenderIP != m_aaRecords[nPart][i].m_dwIP){
					// different IP, means we have to split it 2 times
					uint32 nTmpEndPos1 = m_aaRecords[nPart][i].m_nEndPos;
					uint32 nTmpStartPos1 = nRelEndPos + 1;
					uint32 nTmpStartPos2 = m_aaRecords[nPart][i].m_nStartPos;
					uint32 nTmpEndPos2 = nRelStartPos - 1;
					m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
					m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
					uint32 dwOldIP = m_aaRecords[nPart][i].m_dwIP;
					m_aaRecords[nPart][i].m_dwIP = dwSenderIP;
					m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos1,nTmpEndPos1, dwOldIP));
					m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos2,nTmpEndPos2, dwOldIP));
					// and are done then
				}
				AddDebugLogLine(DLP_DEFAULT, false, _T("CorruptionBlackBox: Debug: %i bytes were rewritten and records replaced with new stats (1)"), (nRelEndPos - nRelStartPos)+1);
				return;
			}
			else if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nStartPos <= nRelEndPos){
				// old one laps over new one on the right site
				ASSERT( nRelEndPos - m_aaRecords[nPart][i].m_nStartPos > 0 );
				ndbgRewritten += nRelEndPos - m_aaRecords[nPart][i].m_nStartPos;
				m_aaRecords[nPart][i].m_nStartPos = nRelEndPos + 1;
			}
			else if (m_aaRecords[nPart][i].m_nEndPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos){
				// old one laps over new one on the left site
				ASSERT( m_aaRecords[nPart][i].m_nEndPos - nRelStartPos > 0 );
				ndbgRewritten += m_aaRecords[nPart][i].m_nEndPos - nRelStartPos;
				m_aaRecords[nPart][i].m_nEndPos = nRelStartPos - 1;
			}
		}
	}
	if (posMerge != (-1) ){
		VERIFY( m_aaRecords[nPart][posMerge].Merge(nRelStartPos, nRelEndPos, dwSenderIP, BBR_NONE) );
	}
	else
		m_aaRecords[nPart].Add(CCBBRecord(nRelStartPos, nRelEndPos, dwSenderIP, BBR_NONE));
	
	if (ndbgRewritten > 0){
		AddDebugLogLine(DLP_DEFAULT, false, _T("CorruptionBlackBox: Debug: %i bytes were rewritten and records replaced with new stats (2)"), ndbgRewritten);
	}
}

void CCorruptionBlackBox::VerifiedData(uint32 nStartPos, uint32 nEndPos){
	if (nEndPos - nStartPos >= PARTSIZE){
		ASSERT( false );
		return;
	}
	// convert pos to relative block pos
	uint16 nPart = nStartPos / PARTSIZE;
	uint32 nRelStartPos = nStartPos - nPart*PARTSIZE;
	uint32 nRelEndPos = nEndPos - nPart*PARTSIZE;
	if (nRelEndPos >= PARTSIZE){
		ASSERT( false );
		return;
	}
	if (nPart >= m_aaRecords.GetCount()){
		//ASSERT( false );
		m_aaRecords.SetSize(nPart+1);
	}
	uint32 nDbgVerifiedBytes = 0;
	uint32 nDbgOldEntries = m_aaRecords[nPart].GetCount();
#ifdef _DEBUG
	CMap<int, int, int, int> mapDebug;
#endif
	for (int i= 0; i < m_aaRecords[nPart].GetCount(); i++){
		if (m_aaRecords[nPart][i].m_BBRStatus == BBR_NONE || m_aaRecords[nPart][i].m_BBRStatus == BBR_VERIFIED){
			if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos){
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
				DEBUG_ONLY(mapDebug.SetAt(m_aaRecords[nPart][i].m_dwIP, 1));
			}
			else if (m_aaRecords[nPart][i].m_nStartPos < nRelStartPos && m_aaRecords[nPart][i].m_nEndPos > nRelEndPos){
			    // need to split it 2*
				uint32 nTmpEndPos1 = m_aaRecords[nPart][i].m_nEndPos;
				uint32 nTmpStartPos1 = nRelEndPos + 1;
				uint32 nTmpStartPos2 = m_aaRecords[nPart][i].m_nStartPos;
				uint32 nTmpEndPos2 = nRelStartPos - 1;
				m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
				m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
				m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos1, nTmpEndPos1, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos2, nTmpEndPos2, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
				DEBUG_ONLY(mapDebug.SetAt(m_aaRecords[nPart][i].m_dwIP, 1));
			}
			else if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nStartPos <= nRelEndPos){
				// need to split it
				uint32 nTmpEndPos = m_aaRecords[nPart][i].m_nEndPos;
				uint32 nTmpStartPos = nRelEndPos + 1;
				m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
				m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
				DEBUG_ONLY(mapDebug.SetAt(m_aaRecords[nPart][i].m_dwIP, 1));
			}
			else if (m_aaRecords[nPart][i].m_nEndPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos){
				// need to split it
				uint32 nTmpStartPos = m_aaRecords[nPart][i].m_nStartPos;
				uint32 nTmpEndPos = nRelStartPos - 1;
				m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
				m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_VERIFIED;
				DEBUG_ONLY(mapDebug.SetAt(m_aaRecords[nPart][i].m_dwIP, 1));
			}
		}
	}
#ifdef _DEBUG
	uint32 nClients = mapDebug.GetCount();
#else
	uint32 nClients = 0;
#endif
	AddDebugLogLine(DLP_DEFAULT, false, _T("Found and marked %u recorded bytes of %u as verified in the CorruptionBlackBox records, %u(%u) records found, %u different clients"), nDbgVerifiedBytes, (nEndPos-nStartPos)+1, m_aaRecords[nPart].GetCount(), nDbgOldEntries, nClients);
}



void CCorruptionBlackBox::CorruptedData(uint32 nStartPos, uint32 nEndPos){
	if (nEndPos - nStartPos >= EMBLOCKSIZE){
		ASSERT( false );
		return;
	}
	// convert pos to relative block pos
	uint16 nPart = nStartPos / PARTSIZE;
	uint32 nRelStartPos = nStartPos - nPart*PARTSIZE;
	uint32 nRelEndPos = nEndPos - nPart*PARTSIZE;
	if (nRelEndPos >= PARTSIZE){
		ASSERT( false );
		return;
	}
	if (nPart >= m_aaRecords.GetCount()){
		//ASSERT( false );
		m_aaRecords.SetSize(nPart+1);
	}
	uint32 nDbgVerifiedBytes = 0;
	CArray<uint32, uint32> aGuiltyClients;
	for (int i= 0; i < m_aaRecords[nPart].GetCount(); i++){
		if (m_aaRecords[nPart][i].m_BBRStatus == BBR_NONE){
			if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos){
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
				aGuiltyClients.Add(m_aaRecords[nPart][i].m_dwIP);
			}
			else if (m_aaRecords[nPart][i].m_nStartPos < nRelStartPos && m_aaRecords[nPart][i].m_nEndPos > nRelEndPos){
			    // need to split it 2*
				uint32 nTmpEndPos1 = m_aaRecords[nPart][i].m_nEndPos;
				uint32 nTmpStartPos1 = nRelEndPos + 1;
				uint32 nTmpStartPos2 = m_aaRecords[nPart][i].m_nStartPos;
				uint32 nTmpEndPos2 = nRelStartPos - 1;
				m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
				m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
				m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos1, nTmpEndPos1, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos2, nTmpEndPos2, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
				aGuiltyClients.Add(m_aaRecords[nPart][i].m_dwIP);
			}
			else if (m_aaRecords[nPart][i].m_nStartPos >= nRelStartPos && m_aaRecords[nPart][i].m_nStartPos <= nRelEndPos){
				// need to split it
				uint32 nTmpEndPos = m_aaRecords[nPart][i].m_nEndPos;
				uint32 nTmpStartPos = nRelEndPos + 1;
				m_aaRecords[nPart][i].m_nEndPos = nRelEndPos;
				m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
				aGuiltyClients.Add(m_aaRecords[nPart][i].m_dwIP);
			}
			else if (m_aaRecords[nPart][i].m_nEndPos >= nRelStartPos && m_aaRecords[nPart][i].m_nEndPos <= nRelEndPos){
				// need to split it
				uint32 nTmpStartPos = m_aaRecords[nPart][i].m_nStartPos;
				uint32 nTmpEndPos = nRelStartPos - 1;
				m_aaRecords[nPart][i].m_nStartPos = nRelStartPos;
				m_aaRecords[nPart].Add(CCBBRecord(nTmpStartPos, nTmpEndPos, m_aaRecords[nPart][i].m_dwIP, m_aaRecords[nPart][i].m_BBRStatus));
				nDbgVerifiedBytes +=  (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
				m_aaRecords[nPart][i].m_BBRStatus = BBR_CORRUPTED;
				aGuiltyClients.Add(m_aaRecords[nPart][i].m_dwIP);
			}
		}
	}
	// check if any IPs are already banned, so we can skip the test for those
	for(int k = 0; k < aGuiltyClients.GetCount();){
		// remove doubles
		for(int y = k+1; y < aGuiltyClients.GetCount();){
			if (aGuiltyClients[k] == aGuiltyClients[y])
				aGuiltyClients.RemoveAt(y);
			else
				y++;
		}
		if (theApp.clientlist->IsBannedClient(aGuiltyClients[k])){
			AddDebugLogLine(DLP_DEFAULT, false, _T("CorruptionBlackBox: Suspicous IP (%s) is already banned, skipping recheck"), ipstr(aGuiltyClients[k]));
			aGuiltyClients.RemoveAt(k);
		}
		else
			k++;
	}
	AddDebugLogLine(DLP_HIGH, false, _T("Found and marked %u recorded bytes of %u as corrupted in the CorruptionBlackBox records, %u clients involved"), nDbgVerifiedBytes, (nEndPos-nStartPos)+1, aGuiltyClients.GetCount());
	if (aGuiltyClients.GetCount() > 0){
		// parse all recorded data for this file to produce a statistic for the involved clients
		
		// first init arrays for the statistic
		CArray<uint32, uint32> aDataCorrupt;
		CArray<uint32, uint32> aDataVerified;
		aDataCorrupt.SetSize(aGuiltyClients.GetCount());
		aDataVerified.SetSize(aGuiltyClients.GetCount());
		for (int j = 0; j < aGuiltyClients.GetCount(); j++)
			aDataCorrupt[j] = aDataVerified[j] = 0;

		// now the parsing
		for (int nPart = 0; nPart < m_aaRecords.GetCount(); nPart++){
			for (int i = 0; i < m_aaRecords[nPart].GetCount(); i++){
				for(int k = 0; k < aGuiltyClients.GetCount(); k++){
					if (m_aaRecords[nPart][i].m_dwIP == aGuiltyClients[k]){
						if (m_aaRecords[nPart][i].m_BBRStatus == BBR_CORRUPTED){
							// corrupted data records are always counted as at least blocksize or bigger
							aDataCorrupt[k] += max((m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1, EMBLOCKSIZE);
						}
						else if(m_aaRecords[nPart][i].m_BBRStatus == BBR_VERIFIED){
							aDataVerified[k] += (m_aaRecords[nPart][i].m_nEndPos-m_aaRecords[nPart][i].m_nStartPos)+1;
						}
					}
				}
			}
		}
		for(int k = 0; k < aGuiltyClients.GetCount(); k++){
			// calculate the percentage of corrupted data for each client and ban
			// him if the limit is reached
			int nCorruptPercentage;
			if ((aDataVerified[k] + aDataCorrupt[k]) > 0)
				nCorruptPercentage = ((uint64)aDataCorrupt[k]*100)/(aDataVerified[k] + aDataCorrupt[k]);
			else {
				AddDebugLogLine(DLP_HIGH, false, _T("CorruptionBlackBox: Programm Error: No records for guilty client found!"));
				ASSERT( false );
				nCorruptPercentage = 0;
			}
			if ( nCorruptPercentage > CBB_BANTHRESHOLD){

				CUpDownClient* pEvilClient = theApp.clientlist->FindClientByIP(aGuiltyClients[k]);
				if (pEvilClient != NULL){
					AddDebugLogLine(DLP_HIGH, false, _T("CorruptionBlackBox: Banning: Found client which send %s of %s corrupted data, %s"), CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), pEvilClient->DbgGetClientInfo());
					theApp.clientlist->AddTrackClient(pEvilClient);
					pEvilClient->Ban(_T("Identified as sender of corrupt data"));
				}
				else{
					AddDebugLogLine(DLP_HIGH, false, _T("CorruptionBlackBox: Banning: Found client which send %s of %s corrupted data, %s"), CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), ipstr(aGuiltyClients[k]));
					theApp.clientlist->AddBannedClient(aGuiltyClients[k]);
				}
			}
			else{
				CUpDownClient* pSuspectClient = theApp.clientlist->FindClientByIP(aGuiltyClients[k]);
				if (pSuspectClient != NULL){
					AddDebugLogLine(DLP_DEFAULT, false, _T("CorruptionBlackBox: Reporting: Found client which probably send %s of %s corrupted data, but it is within the acceptable limit, %s"), CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), pSuspectClient->DbgGetClientInfo());
					theApp.clientlist->AddTrackClient(pSuspectClient);
				}
				else
					AddDebugLogLine(DLP_DEFAULT, false, _T("CorruptionBlackBox: Reporting: Found client which probably send %s of %s corrupted data, but it is within the acceptable limit, %s"), CastItoXBytes(aDataCorrupt[k]), CastItoXBytes((aDataVerified[k] + aDataCorrupt[k])), ipstr(aGuiltyClients[k]));
			}
		}
	}
}
