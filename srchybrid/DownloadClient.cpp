//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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
#include "stdafx.h"
#include "emule.h"
#include <zlib/zlib.h>
#include "UpDownClient.h"
#include "PartFile.h"
#include "OtherFunctions.h"
#include "ListenSocket.h"
#include "PeerCacheSocket.h"
//#include "opcodes.h" // ZZ:DownloadManager
#include "Preferences.h"
#include "SafeFile.h"
#include "Packets.h"
#include "Statistics.h"
#include "ClientCredits.h"
#include "DownloadQueue.h"
#include "ClientUDPSocket.h"
#include "emuledlg.h"
#include "TransferWnd.h"
#include "PeerCacheFinder.h"
#include "Exceptions.h"
#include "clientlist.h"
#include "IPFilter.h" //MORPH - Added by SiRoB

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

extern UINT GetPeerCacheSocketDownloadTimeout();

//	members of CUpDownClient
//	which are mainly used for downloading functions 
CBarShader CUpDownClient::s_StatusBar(16);
//MORPH - Changed by SiRoB, See A4AF PartStatus
/*
void CUpDownClient::DrawStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool  bFlat){ 
*/
void CUpDownClient::DrawStatusBar(CDC* dc, LPCRECT rect, CPartFile* file, bool  bFlat)
{ 
	COLORREF crNeither; 
	if(bFlat) {
		crNeither = RGB(224, 224, 224);
	} else {
		crNeither = RGB(240, 240, 240);
	}

	
	//MORPH START - Changed by SiRoB, See A4AF PartStatus
	/*
	ASSERT(reqfile);
	s_StatusBar.SetFileSize(reqfile->GetFileSize()); 
	*/
	ASSERT(file);
	s_StatusBar.SetFileSize(file->GetFileSize()); 
	//MORPH END   - Changed by SiRoB, See A4AF PartStatus
	s_StatusBar.SetHeight(rect->bottom - rect->top); 
	s_StatusBar.SetWidth(rect->right - rect->left); 
	s_StatusBar.Fill(crNeither); 

	//MORPH START - Changed by SiRoB, See A4AF PartStatus
	uint8* thisStatus;
	if(m_PartStatus_list.Lookup(file,thisStatus) && thisStatus){
		COLORREF crBoth; 
		COLORREF crClientOnly; 
		COLORREF crPending;
		COLORREF crNextPending;
		COLORREF crMeOnly; //MORPH - Added by IceCream--- xrmb:seeTheNeed ---
		COLORREF crA4AF; 
		if(bFlat) { 
			crBoth = RGB(0, 0, 0);
			crClientOnly = RGB(0, 100, 255);
			crPending = RGB(0,150,0);
			crNextPending = RGB(255,208,0);
			crMeOnly = RGB(112,112,112); //MORPH - Added by IceCream--- xrmb:seeTheNeed ---
			crA4AF = RGB(192, 100, 255);
		} else { 
			crBoth = RGB(104, 104, 104);
			crClientOnly = RGB(0, 100, 255);
			crPending = RGB(0, 150, 0);
			crNextPending = RGB(255,208,0);
			crMeOnly = RGB(172,172,172); //MORPH - Added by IceCream--- xrmb:seeTheNeed ---
			crA4AF = RGB(192, 100, 255);
		} 

		char* pcNextPendingBlks = NULL;
		if (m_nDownloadState == DS_DOWNLOADING && file == reqfile){
			pcNextPendingBlks = new char[m_nPartCount];
			memset(pcNextPendingBlks, 'N', m_nPartCount); // do not use '_strnset' for uninitialized memory!
			for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != 0; ){
				UINT uPart = m_PendingBlocks_list.GetNext(pos)->block->StartOffset / PARTSIZE;
				if (uPart < m_nPartCount)
					pcNextPendingBlks[uPart] = 'Y';
			}
		}

		for (uint32 i = 0;i < file->GetPartCount();i++){
			uint32 uEnd;
			if (thisStatus[i]){
				if (PARTSIZE*(i+1) > file->GetFileSize())
					uEnd = file->GetFileSize();
				else
					uEnd = PARTSIZE*(i+1);
				if (file->IsComplete(PARTSIZE*i,PARTSIZE*(i+1)-1))
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crBoth);
				else if (file == reqfile)
				{
					if (m_nDownloadState == DS_DOWNLOADING && m_nLastBlockOffset >= PARTSIZE*i && m_nLastBlockOffset < uEnd)
						s_StatusBar.FillRange(PARTSIZE*i, uEnd, crPending);
					else if (pcNextPendingBlks != NULL && pcNextPendingBlks[i] == 'Y')
						s_StatusBar.FillRange(PARTSIZE*i, uEnd, crNextPending);
					else
						s_StatusBar.FillRange(PARTSIZE*i, uEnd, crClientOnly);
				}
				else
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crA4AF);
			} 
			//MORPH - Added by IceCream --- xrmb:seeTheNeed ---
			else if (file->IsComplete(PARTSIZE*i,PARTSIZE*(i+1)-1)){ 
				if (PARTSIZE*(i+1) > file->GetFileSize()) 
					uEnd = file->GetFileSize(); 
				else 
					uEnd = PARTSIZE*(i+1); 
				s_StatusBar.FillRange(PARTSIZE*i, uEnd, crMeOnly);
			}
			//--- :xrmb ---
		}
		delete[] pcNextPendingBlks;
	} 
	//MORPH END   - Changed by SiRoB, See A4AF PartStatus
	s_StatusBar.Draw(dc, rect->left, rect->top, bFlat); 
} 

bool CUpDownClient::Compare(const CUpDownClient* tocomp, bool bIgnoreUserhash) const
{
	if(!bIgnoreUserhash && HasValidHash() && tocomp->HasValidHash())
	    return !md4cmp(this->GetUserHash(), tocomp->GetUserHash());
	if (HasLowID()){
        if (GetUserIDHybrid()!=0
			&& GetUserIDHybrid() == tocomp->GetUserIDHybrid()
			&& GetServerIP()!=0
			&& GetServerIP() == tocomp->GetServerIP()
			&& GetServerPort()!=0
			&& GetServerPort() == tocomp->GetServerPort())
            return true;
		if (GetIP()!=0
			&& GetIP() == tocomp->GetIP()
			&& GetUserPort()!=0
			&& GetUserPort() == tocomp->GetUserPort())
          return true;
        if (GetIP()!=0 
			&& GetIP() == tocomp->GetIP() 
			&& GetKadPort()!=0 
			&& GetKadPort() == tocomp->GetKadPort())
            return true;
        return false;
    }
    if (GetUserPort()!=0){
        if (GetUserIDHybrid() == tocomp->GetUserIDHybrid()
			&& GetUserPort() == tocomp->GetUserPort())
            return true;
        if (GetIP()!=0
			&& GetIP() == tocomp->GetIP()
			&& GetUserPort() == tocomp->GetUserPort())
            return true;
    }
	if(GetKadPort()!=0){
		if (GetUserIDHybrid() == tocomp->GetUserIDHybrid()
			&& GetKadPort() == tocomp->GetKadPort())
			return true;
		if( GetIP()!=0
			&& GetIP() == tocomp->GetIP()
			&& GetKadPort() == tocomp->GetKadPort())
			return true;
	}
	return false;
}

// Return bool is not if you asked or not..
// false = Client was deleted!
// true = client was not deleted!
bool CUpDownClient::AskForDownload()
{
	if (theApp.listensocket->TooManySockets() && !(socket && socket->IsConnected()) )
	{
		if (GetDownloadState() != DS_TOOMANYCONNS)
			SetDownloadState(DS_TOOMANYCONNS);
		return true;
	}
	if (m_bUDPPending)
	{
		m_nFailedUDPPackets++;
		theApp.downloadqueue->AddFailedUDPFileReasks();
	}
	m_bUDPPending = false;
    SwapToAnotherFile(_T("A4AF check before tcp file reask. CUpDownClient::AskForDownload()"), true, false, false, NULL, true, true); // ZZ:DownloadManager
    SetLastAskedTime(); // ZZ:DownloadManager
	SetDownloadState(DS_CONNECTING);
	return TryToConnect();
}

bool CUpDownClient::IsSourceRequestAllowed() const
{
    return IsSourceRequestAllowed(reqfile); // ZZ:DownloadManager
} // ZZ:DownloadManager

bool CUpDownClient::IsSourceRequestAllowed(CPartFile* partfile, bool sourceExchangeCheck) const // ZZ:DownloadManager
{ // ZZ:DownloadManager
	DWORD dwTickCount = ::GetTickCount() + CONNECTION_LATENCY;
	unsigned int nTimePassedClient = dwTickCount - GetLastSrcAnswerTime();
	unsigned int nTimePassedFile   = dwTickCount - partfile->GetLastAnsweredTime(); // ZZ:DownloadManager
	bool bNeverAskedBefore = GetLastAskedForSources() == 0;

// ZZ:DownloadManager -->
	UINT uSources = partfile->GetSourceCount();
    UINT uValidSources = partfile->GetValidSourcesCount();

    if(partfile != reqfile) {
        uSources++;
        uValidSources++;
    }

    UINT uReqValidSources = reqfile->GetValidSourcesCount();
// <-- ZZ:DownloadManager

	return (
	         //if client has the correct extended protocol
	         ExtProtocolAvailable() && GetSourceExchangeVersion() > 1 &&
	         //AND if we need more sources
	         thePrefs.GetMaxSourcePerFileSoft() > uSources &&
	         //AND if...
	         (
	           //source is not complete and file is very rare
	           (    !m_bCompleteSource
				 && (bNeverAskedBefore || nTimePassedClient > SOURCECLIENTREASKS)
			     && (uSources <= RARE_FILE/5)
				 && (!sourceExchangeCheck || partfile == reqfile || uValidSources < uReqValidSources && uReqValidSources > 3) // ZZ:DownloadManager
	           ) ||
	           //source is not complete and file is rare
	           ( !m_bCompleteSource
				 && (bNeverAskedBefore || nTimePassedClient > SOURCECLIENTREASKS)
			     && (uSources <= RARE_FILE || (!sourceExchangeCheck || partfile == reqfile) && uSources <= RARE_FILE / 2 + uValidSources) // ZZ:DownloadManager
				 && (nTimePassedFile > SOURCECLIENTREASKF)
				 && (!sourceExchangeCheck || partfile == reqfile || uValidSources < SOURCECLIENTREASKS/SOURCECLIENTREASKF && uValidSources < uReqValidSources) // ZZ:DownloadManager
	           ) ||
	           // OR if file is not rare
			   ( (bNeverAskedBefore || nTimePassedClient > (unsigned)(SOURCECLIENTREASKS * MINCOMMONPENALTY)) 
				 && (nTimePassedFile > (unsigned)(SOURCECLIENTREASKF * MINCOMMONPENALTY))
				 && (!sourceExchangeCheck || partfile == reqfile || uValidSources < SOURCECLIENTREASKS/SOURCECLIENTREASKF && uValidSources < uReqValidSources) // ZZ:DownloadManager
	           )
	         )
	       );
}

void CUpDownClient::SendFileRequest()
{
    // normally asktime has already been reset here, but then SwapToAnotherFile will return without much work, so check to make sure
    SwapToAnotherFile(_T("A4AF check before tcp file reask. CUpDownClient::SendFileRequest()"), true, false, false, NULL, true, true); // ZZ:DownloadManager

	ASSERT(reqfile != NULL);
	if(!reqfile)
		return;
	AddAskedCountDown();

	CSafeMemFile dataFileReq(16+16);
	dataFileReq.WriteHash16(reqfile->GetFileHash());

	if( SupportMultiPacket() )
	{
		dataFileReq.WriteUInt8(OP_REQUESTFILENAME);
		//Extended information
		if( GetExtendedRequestsVersion() > 0 )
			reqfile->WritePartStatus(&dataFileReq);
		if( GetExtendedRequestsVersion() > 1 )
			reqfile->WriteCompleteSourcesCount(&dataFileReq);
		if (reqfile->GetPartCount() > 1)
			dataFileReq.WriteUInt8(OP_SETREQFILEID);
		if( IsEmuleClient() )
		{
			SetRemoteQueueFull( true );
			SetRemoteQueueRank(0);
		}	
		if(IsSourceRequestAllowed())
		{
			dataFileReq.WriteUInt8(OP_REQUESTSOURCES);
			reqfile->SetLastAnsweredTimeTimeout();
			SetLastAskedForSources();
			if (thePrefs.GetDebugSourceExchange())
				AddDebugLogLine(false, _T("Send:Source Request User(%s) File(%s)"), GetUserName(), reqfile->GetFileName() );
		}
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__MultiPacket", this, (char*)reqfile->GetFileHash());
		Packet* packet = new Packet(&dataFileReq, OP_EMULEPROT);
		packet->opcode = OP_MULTIPACKET;
		theStats.AddUpDataOverheadFileRequest(packet->size);
		socket->SendPacket(packet, true);
	}
	else
	{
		//This is extended information
		if( GetExtendedRequestsVersion() > 0 ){
			reqfile->WritePartStatus(&dataFileReq);
		}
		if( GetExtendedRequestsVersion() > 1 ){
			reqfile->WriteCompleteSourcesCount(&dataFileReq);
		}
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__FileRequest", this, (char*)reqfile->GetFileHash());
		Packet* packet = new Packet(&dataFileReq);
		packet->opcode=OP_REQUESTFILENAME;
		theStats.AddUpDataOverheadFileRequest(packet->size);
		socket->SendPacket(packet, true);

		// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
		// if the remote client answers the OP_REQUESTFILENAME with OP_REQFILENAMEANSWER the file is shared by the remote client. if we
		// know that the file is shared, we know also that the file is complete and don't need to request the file status.
		if (reqfile->GetPartCount() > 1)
		{
			if (thePrefs.GetDebugClientTCPLevel() > 0)
				DebugSend("OP__SetReqFileID", this, (char*)reqfile->GetFileHash());
			CSafeMemFile dataSetReqFileID(16);
			dataSetReqFileID.WriteHash16(reqfile->GetFileHash());
			packet = new Packet(&dataSetReqFileID);
			packet->opcode = OP_SETREQFILEID;
		    theStats.AddUpDataOverheadFileRequest(packet->size);
			socket->SendPacket(packet, true);
		}

		if( IsEmuleClient() )
		{
			SetRemoteQueueFull( true );
			SetRemoteQueueRank(0);
		}	
		if(IsSourceRequestAllowed()) 
		{
		    if (thePrefs.GetDebugClientTCPLevel() > 0){
			    DebugSend("OP__RequestSources", this, (char*)reqfile->GetFileHash());
			    if (GetLastAskedForSources() == 0)
				    Debug(_T("  first source request\n"));
			    else
				    Debug(_T("  last source request was before %s\n"), CastSecondsToHM((GetTickCount() - GetLastAskedForSources())/1000));
		    }
			reqfile->SetLastAnsweredTimeTimeout();
			Packet* packet = new Packet(OP_REQUESTSOURCES,16,OP_EMULEPROT);
			md4cpy(packet->pBuffer,reqfile->GetFileHash());
			theStats.AddUpDataOverheadSourceExchange(packet->size);
			socket->SendPacket(packet,true,true);
			SetLastAskedForSources();
			if (thePrefs.GetDebugSourceExchange())
				AddDebugLogLine(false, _T("Send:Source Request User(%s) File(%s)"), GetUserName(), reqfile->GetFileName() );
		}
	}
    SetLastAskedTime(); // ZZ:DownloadManager
}
void CUpDownClient::SendStartupLoadReq()
{
	if (socket==NULL || reqfile==NULL)
	{
		ASSERT(0);
		return;
	}
	SetDownloadState(DS_ONQUEUE);
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__StartupLoadReq", this);
	CSafeMemFile dataStartupLoadReq(16);
	dataStartupLoadReq.WriteHash16(reqfile->GetFileHash());
	Packet* packet = new Packet(&dataStartupLoadReq);
	packet->opcode = OP_STARTUPLOADREQ;
	theStats.AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet, true, true);
	m_fQueueRankPending = 1;
	m_fUnaskQueueRankRecv = 0;
}

void CUpDownClient::ProcessFileInfo(CSafeMemFile* data, CPartFile* file)
{
	if (file==NULL)
		throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileInfo; file==NULL)");
	if (reqfile==NULL)
		throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileInfo; reqfile==NULL)");
	if (file != reqfile)
		throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileInfo; reqfile!=file)");
	m_strClientFilename = data->ReadString();
	// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
	// if the remote client answers the OP_REQUESTFILENAME with OP_REQFILENAMEANSWER the file is shared by the remote client. if we
	// know that the file is shared, we know also that the file is complete and don't need to request the file status.
	if (reqfile->GetPartCount() == 1)
	{
		//MORPH START - Changed by SiRoB,  See A4AF PartStatus
		/*
		if (m_abyPartStatus)
		{
			delete[] m_abyPartStatus;
			m_abyPartStatus = NULL;
		}
		*/
		uint8* thisStatus;
		if(m_PartStatus_list.Lookup(reqfile, thisStatus))
		{
			delete[] thisStatus;
			m_PartStatus_list.RemoveKey(reqfile);
		}
		m_abyPartStatus = NULL;
		//MORPH   END - Changed by SiRoB, See A4AF PartStatus
		m_nPartCount = reqfile->GetPartCount();
		m_abyPartStatus = new uint8[m_nPartCount];
		//MORPH START - Added by SiRoB, See A4AF PartStatus
		m_PartStatus_list.SetAt(reqfile,m_abyPartStatus);
		//MORPH END   - Added by SiRoB, See A4AF PartStatus
		memset(m_abyPartStatus,1,m_nPartCount);
		m_bCompleteSource = true;

		if (thePrefs.GetDebugClientTCPLevel() > 0)
		{
		    int iNeeded = 0;
		    for (int i = 0; i < m_nPartCount; i++)
			    if (!reqfile->IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1))
				    iNeeded++;
			char* psz = new char[m_nPartCount + 1];
			for (int i = 0; i < m_nPartCount; i++)
				psz[i] = m_abyPartStatus[i] ? '#' : '.';
			psz[i] = '\0';
			Debug(_T("  Parts=%u  %hs  Needed=%u\n"), m_nPartCount, psz, iNeeded);
			delete[] psz;
		}
		UpdateDisplayedInfo();
		reqfile->UpdateAvailablePartsCount();
		// even if the file is <= PARTSIZE, we _may_ need the hashset for that file (if the file size == PARTSIZE)
		if (reqfile->hashsetneeded)
		{
			RequestHashset();	// SLUGFILLER: SafeHash
		}
		else
		{
			SendStartupLoadReq();
		}
		reqfile->UpdatePartsInfo();
	}
}

void CUpDownClient::ProcessFileStatus(bool bUdpPacket, CSafeMemFile* data, CPartFile* file)
{
	if ( !reqfile || file != reqfile )
	{
		if (reqfile==NULL)
			throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileStatus; reqfile==NULL)");
		throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileStatus; reqfile!=file)");
	}
	uint16 nED2KPartCount = data->ReadUInt16();

	//MORPH START - Added by SiRoB, See A4AF PartStatus
	/*
	if (m_abyPartStatus)
	{
		delete[] m_abyPartStatus;
		m_abyPartStatus = NULL;
	}
	*/
	uint8* thisStatus;
	if(m_PartStatus_list.Lookup(reqfile, thisStatus))
	{
		delete[] thisStatus;
		m_PartStatus_list.RemoveKey(reqfile);
	}
	m_abyPartStatus = NULL;
	//MORPH   END - Added by SiRoB, See A4AF PartStatus

	bool bPartsNeeded = false;
	int iNeeded = 0;
	if (!nED2KPartCount)
	{
		m_nPartCount = reqfile->GetPartCount();
		m_abyPartStatus = new uint8[m_nPartCount];
		//MORPH START - Added by SiRoB, See A4AF PartStatus
		m_PartStatus_list.SetAt(reqfile, m_abyPartStatus);
		//MORPH   END - Added by SiRoB, See A4AF PartStatus
		memset(m_abyPartStatus,1,m_nPartCount);
		bPartsNeeded = true;
		m_bCompleteSource = true;
		if (bUdpPacket ? (thePrefs.GetDebugClientUDPLevel() > 0) : (thePrefs.GetDebugClientTCPLevel() > 0))
		{
			for (int i = 0; i < m_nPartCount; i++)
			{
				if (!reqfile->IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1))
					iNeeded++;
	}
		}
	}
	else
	{
		if (reqfile->GetED2KPartCount() != nED2KPartCount)
		{
			CString strError;
			strError.Format(_T("ProcessFileStatus - wrong part number recv=%u  expected=%u  %s"), nED2KPartCount, reqfile->GetED2KPartCount(), DbgGetFileInfo(reqfile->GetFileHash()));
			m_nPartCount = 0;
			throw strError;
		}
		m_nPartCount = reqfile->GetPartCount();

		m_bCompleteSource = false;
		m_abyPartStatus = new uint8[m_nPartCount];
		//MORPH START - Added by SiRoB, See A4AF PartStatus
		m_PartStatus_list.SetAt(reqfile, m_abyPartStatus);
		//MORPH   END - Added by SiRoB, See A4AF PartStatus
		uint16 done = 0;
		while (done != m_nPartCount)
		{
			uint8 toread = data->ReadUInt8();
			for (sint32 i = 0;i != 8;i++)
			{
				m_abyPartStatus[done] = ((toread>>i)&1)? 1:0; 	
				if (m_abyPartStatus[done])
				{
					if (!reqfile->IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1)){
						bPartsNeeded = true;
						iNeeded++;
					}
				}
				done++;
				if (done == m_nPartCount)
					break;
			}
		}
	}

	if (bUdpPacket ? (thePrefs.GetDebugClientUDPLevel() > 0) : (thePrefs.GetDebugClientTCPLevel() > 0))
	{
		char* psz = new char[m_nPartCount + 1];
		for (int i = 0; i < m_nPartCount; i++)
			psz[i] = m_abyPartStatus[i] ? '#' : '.';
		psz[i] = '\0';
		Debug(_T("  Parts=%u  %hs  Needed=%u\n"), m_nPartCount, psz, iNeeded);
		delete[] psz;
	}
	
	
	UpdateDisplayedInfo();
	reqfile->UpdateAvailablePartsCount();
    
	// NOTE: This function is invoked from TCP and UDP socket!
	if (!bUdpPacket)
	{
		// SLUGFILLER: SafeHash - request hashset first, check needed parts later
		if (reqfile->hashsetneeded)
		{
			RequestHashset();	// SLUGFILLER: SafeHash
		}
		else if (!bPartsNeeded)
			SetDownloadState(DS_NONEEDEDPARTS);
		// SLUGFILLER: SafeHash
		//If we are using the eMule filerequest packets, this is taken care of in the Multipacket!
		else
		{
			SendStartupLoadReq();
		}
	}
	else
	{
		if (!bPartsNeeded)
			SetDownloadState(DS_NONEEDEDPARTS);
		else
			SetDownloadState(DS_ONQUEUE);
	}
	reqfile->UpdatePartsInfo();
}

bool CUpDownClient::AddRequestForAnotherFile(CPartFile* file){
	for (POSITION pos = m_OtherNoNeeded_list.GetHeadPosition();pos != 0;){
		if (m_OtherNoNeeded_list.GetNext(pos) == file)
			return false;
	}
	for (POSITION pos = m_OtherRequests_list.GetHeadPosition();pos != 0;){
		if (m_OtherRequests_list.GetNext(pos) == file)
			return false;
	}
	m_OtherRequests_list.AddTail(file);
	file->A4AFsrclist.AddTail(this); // [enkeyDEV(Ottavio84) -A4AF-]

	return true;
}

void CUpDownClient::ClearDownloadBlockRequests()
{
	for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition();pos != 0;){
		Requested_Block_Struct* cur_block = m_DownloadBlocks_list.GetNext(pos);
		if (reqfile){
			reqfile->RemoveBlockFromList(cur_block->StartOffset,cur_block->EndOffset);
		}
		delete cur_block;
	}
	m_DownloadBlocks_list.RemoveAll();

	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0;){
		Pending_Block_Struct *pending = m_PendingBlocks_list.GetNext(pos);
		if (reqfile){
			reqfile->RemoveBlockFromList(pending->block->StartOffset, pending->block->EndOffset);
		}

		delete pending->block;
		// Not always allocated
		if (pending->zStream){
			inflateEnd(pending->zStream);
			delete pending->zStream;
		}
		delete pending;
	}
	m_PendingBlocks_list.RemoveAll();
}

void CUpDownClient::SetDownloadState(EDownloadState nNewState){
	if (m_nDownloadState != nNewState){
		if (reqfile){
			if(nNewState == DS_DOWNLOADING){
				reqfile->AddDownloadingSource(this);
			}
			else if(m_nDownloadState == DS_DOWNLOADING){
				reqfile->RemoveDownloadingSource(this);
			}
		}

	
		// SLUGFILLER: SafeHash
		if (reqfile && m_nDownloadState == DS_REQHASHSET && nNewState != DS_REQHASHSET)
			reqfile->hashsetneeded = false;
		if (nNewState == DS_REQHASHSET)
			m_dwRequestedHashset = GetTickCount();
		// SLUGFILLER: SafeHash

		if (m_nDownloadState == DS_DOWNLOADING ){

			// -khaos--+++> Extended Statistics (Successful/Failed Download Sessions)
			if ( m_bTransferredDownMini && nNewState != DS_ERROR )
				thePrefs.Add2DownSuccessfulSessions(); // Increment our counters for successful sessions (Cumulative AND Session)
			else
				thePrefs.Add2DownFailedSessions(); // Increment our counters failed sessions (Cumulative AND Session)
			//wistily start
			uint32 tempDownTimeDifference= GetDownTimeDifference();
			Add2DownTotalTime(tempDownTimeDifference);
			if (m_nDownTotalTime > 999) //Added by SiRoB, to avoid div by zero
				m_nAvDownDatarate = m_nTransferedDown/(m_nDownTotalTime/1000);
			thePrefs.Add2DownSAvgTime(tempDownTimeDifference/1000);
			/*thePrefs.Add2DownSAvgTime(GetDownTimeDifference()/1000);*/
			//wistily stop

			// <-----khaos-

			m_nDownloadState = nNewState;
			
			ClearDownloadBlockRequests();
				
			m_nDownDatarate = 0;
			if (nNewState == DS_NONE){
				
				//MORPH START - Removed by SiRoB, See A4AF PartStatus
				/*
				if (m_abyPartStatus)
					delete[] m_abyPartStatus;
				*/
				//MORPH   END - Removed by SiRoB, See A4AF PartStatus
				m_abyPartStatus = NULL;
				m_nPartCount = 0;
			}
			if (socket && nNewState != DS_ERROR)
				socket->DisableDownloadLimit();
		}
		m_nDownloadState = nNewState;
		if( GetDownloadState() == DS_DOWNLOADING ){
			if ( IsEmuleClient() )
				SetRemoteQueueFull(false);
			SetRemoteQueueRank(0);
			SetAskedCountDown(0);
		}
		UpdateDisplayedInfo(true);
	}
}

void CUpDownClient::ProcessHashSet(char* packet,uint32 size){
	if (!m_fHashsetRequesting)
		throw CString(_T("unwanted hashset"));
	if ( (!reqfile) || md4cmp(packet,reqfile->GetFileHash())){
		CheckFailedFileIdReqs((uchar*)packet);
		throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessHashSet)");	
	}
	CSafeMemFile data((BYTE*)packet,size);
	if (reqfile->LoadHashsetFromFile(&data,true)){
		m_fHashsetRequesting = 0;
		m_dwRequestedHashset = 0;	// SLUGFILLER: SafeHash
		reqfile->PerformFirstHash();		// SLUGFILLER: SafeHash - Rehash
	}
	else{
		reqfile->hashsetneeded = true;
		throw GetResString(IDS_ERR_BADHASHSET);
	}
	SendStartupLoadReq();
}

void CUpDownClient::CreateBlockRequests(int iMaxBlocks)
{
	ASSERT( iMaxBlocks >= 1 /*&& iMaxBlocks <= 3*/ );
	if (m_DownloadBlocks_list.IsEmpty())
	{
		uint16 count = iMaxBlocks - m_PendingBlocks_list.GetCount();
		Requested_Block_Struct** toadd = new Requested_Block_Struct*[count];
		if (reqfile->GetNextRequestedBlock(this,toadd,&count)){
			for (int i = 0; i < count; i++)
				m_DownloadBlocks_list.AddTail(toadd[i]);			
		}
		delete[] toadd;
	}

	while (m_PendingBlocks_list.GetCount() < iMaxBlocks && !m_DownloadBlocks_list.IsEmpty())
	{
		Pending_Block_Struct* pblock = new Pending_Block_Struct;
		pblock->block = m_DownloadBlocks_list.RemoveHead();
		pblock->zStream = NULL;
		pblock->totalUnzipped = 0;
		pblock->fZStreamError = 0;
		pblock->fRecovered = 0;
		m_PendingBlocks_list.AddTail(pblock);
	}
}

void CUpDownClient::SendBlockRequests(){
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__RequestParts", this, reqfile!=NULL ? (char*)reqfile->GetFileHash() : NULL);
	m_dwLastBlockReceived = ::GetTickCount();
	if (!reqfile)
		return;
	CreateBlockRequests(3);
	if (m_PendingBlocks_list.IsEmpty()){
		SendCancelTransfer();
		SetDownloadState(DS_ONQUEUE);	// SLUGFILLER: noNeededRequeue
		return;
	}
	const int iPacketSize = 16+(3*4)+(3*4); // 40
	Packet* packet = new Packet(OP_REQUESTPARTS,iPacketSize);
	CSafeMemFile data((BYTE*)packet->pBuffer,iPacketSize);
	data.WriteHash16(reqfile->GetFileHash());
	POSITION pos = m_PendingBlocks_list.GetHeadPosition();
	for (uint32 i = 0; i != 3; i++){
		if (pos){
			Pending_Block_Struct* pending = m_PendingBlocks_list.GetNext(pos);
			ASSERT( pending->block->StartOffset <= pending->block->EndOffset );
			//ASSERT( pending->zStream == NULL );
			//ASSERT( pending->totalUnzipped == 0 );
			pending->fZStreamError = 0;
			pending->fRecovered = 0;
			data.WriteUInt32(pending->block->StartOffset);
		}
		else
			data.WriteUInt32(0);
	}
	pos = m_PendingBlocks_list.GetHeadPosition();
	for (uint32 i = 0; i != 3; i++){
		if (pos){
			Requested_Block_Struct* block = m_PendingBlocks_list.GetNext(pos)->block;
			uint32 endpos = block->EndOffset+1;
			data.WriteUInt32(endpos);
			if (thePrefs.GetDebugClientTCPLevel() > 0){
				CString strInfo;
				strInfo.Format(_T("  Block request: Start%u=%u  End%u=%u  Size=%u  Part=%u-%u"), i, block->StartOffset, i, endpos, endpos - block->StartOffset, block->StartOffset/PARTSIZE, (endpos-1)/PARTSIZE);
				strInfo.AppendFormat(_T(",  Complete=%s"), reqfile->IsComplete(block->StartOffset, block->EndOffset) ? _T("Yes(NOTE:)") : _T("No"));
				strInfo.AppendFormat(_T(",  PureGap=%s"), reqfile->IsPureGap(block->StartOffset, block->EndOffset) ? _T("Yes") : _T("No(NOTE:)"));
				strInfo.AppendFormat(_T(",  AlreadyRequested=%s"), reqfile->IsAlreadyRequested(block->StartOffset, block->EndOffset) ? _T("Yes") : _T("No(NOTE:)"));
				strInfo += _T('\n');
				Debug(strInfo);
			}
		}
		else
		{
			data.WriteUInt32(0);
			if (thePrefs.GetDebugClientTCPLevel() > 0)
				Debug(_T("  Block request: Start%u=%u  End%u=%u  Size=%u\n"), i, 0, i, 0, 0);
		}
	}
	theStats.AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet,true,true);
}

/* Barry - Originally this only wrote to disk when a full 180k block 
           had been received from a client, and only asked for data in 
		   180k blocks.

		   This meant that on average 90k was lost for every connection
		   to a client data source. That is a lot of wasted data.

		   To reduce the lost data, packets are now written to a buffer
		   and flushed to disk regularly regardless of size downloaded.
		   This includes compressed packets.

		   Data is also requested only where gaps are, not in 180k blocks.
		   The requests will still not exceed 180k, but may be smaller to
		   fill a gap.
*/
void CUpDownClient::ProcessBlockPacket(char *packet, uint32 size, bool packed)
{
	uint32 nDbgStartPos = *((uint32*)(packet+16));
	if (thePrefs.GetDebugClientTCPLevel() > 1){
		if (packed)
			Debug(_T("  Start=%u  BlockSize=%u  Size=%u  %s\n"), nDbgStartPos, *((uint32*)(packet + 16+4)), size-24, DbgGetFileInfo((uchar*)packet));
		else
			Debug(_T("  Start=%u  End=%u  Size=%u  %s\n"), nDbgStartPos, *((uint32*)(packet + 16+4)), *((uint32*)(packet + 16+4)) - nDbgStartPos, DbgGetFileInfo((uchar*)packet));
	}

	// Ignore if no data required
	if (!(GetDownloadState() == DS_DOWNLOADING || GetDownloadState() == DS_NONEEDEDPARTS)){
		TRACE("%s - Invalid download state\n", __FUNCTION__);
		return;
	}

	const int HEADER_SIZE = 24;

	// Update stats
	m_dwLastBlockReceived = ::GetTickCount();

	// Read data from packet
	CSafeMemFile data((BYTE*)packet, size);
	uchar fileID[16];
	data.ReadHash16(fileID);

	// Check that this data is for the correct file
	if ( (!reqfile) || md4cmp(packet, reqfile->GetFileHash()))
	{
		throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessBlockPacket)");
	}

	// Find the start & end positions, and size of this chunk of data
	uint32 nStartPos;
	uint32 nEndPos;
	uint32 nBlockSize = 0;
		nStartPos = data.ReadUInt32();
	if (packed)
	{
			nBlockSize = data.ReadUInt32();
		nEndPos = nStartPos + (size - HEADER_SIZE);
	}
	else
			nEndPos = data.ReadUInt32();

	// Check that packet size matches the declared data size + header size (24)
	    if (nEndPos == nStartPos || size != ((nEndPos - nStartPos) + HEADER_SIZE))
		throw GetResString(IDS_ERR_BADDATABLOCK) + _T(" (ProcessBlockPacket)");

	// -khaos--+++>
	// Extended statistics information based on which client and remote port sent this data.
	// The new function adds the bytes to the grand total as well as the given client/port.
	// bFromPF is not relevant to downloaded data.  It is purely an uploads statistic.

	//MORPH START - Added by Yun.SF3, ZZ Upload System
	/*
	thePrefs.Add2SessionTransferData(GetClientSoft(), GetUserPort(), false, false, size - HEADER_SIZE);
	*/
	thePrefs.Add2SessionTransferData(GetClientSoft(), GetUserPort(), false, false, size - HEADER_SIZE, false);
	//MORPH END - Added by Yun.SF3, ZZ Upload System
	// <-----khaos-

	m_nDownDataRateMS += size - HEADER_SIZE;
	if (credits)
		credits->AddDownloaded(size - HEADER_SIZE, GetIP());

	// Move end back one, should be inclusive
	nEndPos--;

	// Loop through to find the reserved block that this is within
	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != NULL; )
	{
		POSITION posLast = pos;
		Pending_Block_Struct *cur_block = m_PendingBlocks_list.GetNext(pos);
		if ((cur_block->block->StartOffset <= nStartPos) && (cur_block->block->EndOffset >= nStartPos))
		{
			// Found reserved block

				if (cur_block->fZStreamError){
					if (thePrefs.GetVerbose())
					AddDebugLogLine(false, _T("Ignoring %u bytes of block starting at %u because of errornous zstream state for file \"%s\" - %s"), size - HEADER_SIZE, nStartPos, reqfile->GetFileName(), DbgGetClientInfo());
			    reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
			    return;
		    }
        
			// Remember this start pos, used to draw part downloading in list
			m_nLastBlockOffset = nStartPos;  

			// Occasionally packets are duplicated, no point writing it twice
			// This will be 0 in these cases, or the length written otherwise
			uint32 lenWritten = 0;

			// Handle differently depending on whether packed or not
			if (!packed)
			{
				// Write to disk (will be buffered in part file class)
				lenWritten = reqfile->WriteToBuffer(size - HEADER_SIZE, 
													(BYTE *) (packet + HEADER_SIZE),
													nStartPos,
													nEndPos,
													cur_block->block );
			}
			else // Packed
			{
					ASSERT( (int)size > 0 );
				// Create space to store unzipped data, the size is only an initial guess, will be resized in unzip() if not big enough
				uint32 lenUnzipped = (size * 2); 
				// Don't get too big
				if (lenUnzipped > (EMBLOCKSIZE + 300))
					lenUnzipped = (EMBLOCKSIZE + 300);
				BYTE *unzipped = new BYTE[lenUnzipped];

				// Try to unzip the packet
				int result = unzip(cur_block, (BYTE *)(packet + HEADER_SIZE), (size - HEADER_SIZE), &unzipped, &lenUnzipped);
				// no block can be uncompressed to >2GB, 'lenUnzipped' is obviously errornous.
				if (result == Z_OK && (int)lenUnzipped >= 0)
				{
					//MORPH START - Added by IceCream, Defeat 0-filled Part Senders from Maella
					if((thePrefs.GetEnableZeroFilledTest() == true) && (reqfile->IsCDImage() == false) && (reqfile->IsArchive() == false) && (reqfile->IsDocument() == false)){
						// Check the compression factor
						//EastShare START - Modified by TAHO, modified 0-filled Part Error
						//if(lenUnzipped > 25*nBlockSize){
						if(lenUnzipped > 25*nBlockSize 
							&& (reqfile->GetFileSize()/EMBLOCKSIZE) > (cur_block->block->StartOffset/EMBLOCKSIZE + 3)){

							// Format User hash
							CString userHash;
							for(int  i=0; i<16; i++){
								TCHAR buffer[33];
								_stprintf(buffer, _T("%02X"), GetUserHash()[i]);
								userHash += buffer;
							}

							// Log
							AddLogLine(true,  _T("Received suspicious block: file '%s', part %i, block %i, blocksize %i, comp. blocksize %i, comp. factor %0.1f)"), reqfile->GetFileName(), cur_block->block->StartOffset/PARTSIZE, cur_block->block->StartOffset/EMBLOCKSIZE, lenUnzipped, nBlockSize, lenUnzipped/nBlockSize);
							AddLogLine(false, _T("Username '%s' (IP %s:%i), hash %s"), m_pszUsername, ipstr(GetIP()), GetUserPort(), userHash);

							// Ban => serious error (Attack?)
							if(lenUnzipped > 4*nBlockSize && reqfile->IsArchive() == true){
								theApp.ipfilter->AddIP(GetIP(), 1, _T("Temporary"));
								SetDownloadState(DS_ERROR);
							}

							// Do Not Save
							lenWritten = 0;
							lenUnzipped = 0; // skip writting
							reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
							AddLogLine(false, _T("Block dropped"));
						}
					}
					//MORPH END   - Added by IceCream, Defeat 0-filled Part Senders from Maella

					if (lenUnzipped > 0) // Write any unzipped data to disk
					{
							ASSERT( (int)lenUnzipped > 0 );

						// Use the current start and end positions for the uncompressed data
						nStartPos = cur_block->block->StartOffset + cur_block->totalUnzipped - lenUnzipped;
						nEndPos = cur_block->block->StartOffset + cur_block->totalUnzipped - 1;

					    if (nStartPos > cur_block->block->EndOffset || nEndPos > cur_block->block->EndOffset){
							if (thePrefs.GetVerbose())
								AddDebugLogLine(false, GetResString(IDS_ERR_CORRUPTCOMPRPKG),reqfile->GetFileName(),666);
							reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
							// There is no chance to recover from this error
						}
						else{
							// Write uncompressed data to file
							lenWritten = reqfile->WriteToBuffer(size - HEADER_SIZE,
								unzipped,
								nStartPos,
								nEndPos,
								cur_block->block );
						}
					}
				}
				else
				{
						if (thePrefs.GetVerbose())
						{
							CString strZipError;
							if (cur_block->zStream && cur_block->zStream->msg)
								strZipError.Format(_T(" - %hs"), cur_block->zStream->msg);
							if (result == Z_OK && (int)lenUnzipped < 0){
								ASSERT(0);
								strZipError.AppendFormat(_T("; Z_OK,lenUnzipped=%d"), lenUnzipped);
							}
							AddDebugLogLine(false, GetResString(IDS_ERR_CORRUPTCOMPRPKG) + strZipError, reqfile->GetFileName(), result);
						}
						reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);

					// If we had an zstream error, there is no chance that we could recover from it nor that we
					// could use the current zstream (which is in error state) any longer.
				    if (cur_block->zStream){
					    inflateEnd(cur_block->zStream);
					    delete cur_block->zStream;
					    cur_block->zStream = NULL;
				    }
    
					// Although we can't further use the current zstream, there is no need to disconnect the sending 
					// client because the next zstream (a series of 10K-blocks which build a 180K-block) could be
					// valid again. Just ignore all further blocks for the current zstream.
					cur_block->fZStreamError = 1;
					cur_block->totalUnzipped = 0;
					//MORPH START - Added by IceCream, Defeat 0-filled Part Senders from Maella
					if(thePrefs.GetEnableZeroFilledTest() == true) {
						CString userHash;
						for(int  i=0; i<16; i++){
							TCHAR buffer[33];
							_stprintf(buffer, _T("%02X"), GetUserHash()[i]);
							userHash += buffer;
						}
						// Ban => serious error (Attack?)
						AddLogLine(false, _T(GetResString(IDS_CORRUPTDATASENT)), m_pszUsername, ipstr(GetConnectIP()), GetUserPort(), userHash, GetClientSoftVer()); //MORPH - Modified by IceCream
						theApp.ipfilter->AddIP(GetIP(), 1, _T("Temporary"));
						SetDownloadState(DS_ERROR);
					}
					//MORPH END   - Added by IceCream, Defeat 0-filled Part Senders from Maella
				}
				delete [] unzipped;
			}

			// These checks only need to be done if any data was written
			if (lenWritten > 0)
			{
				m_nTransferedDown += lenWritten;
				
				SetTransferredDownMini(); // Sets boolean m_bTransferredDownMini to true // -khaos--+++> For determining whether the current download session was a success or not.

				// If finished reserved block
				if (nEndPos == cur_block->block->EndOffset)
				{
					reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
					delete cur_block->block;
					// Not always allocated
					if (cur_block->zStream){
						inflateEnd(cur_block->zStream);
						delete cur_block->zStream;
					}
					delete cur_block;
						m_PendingBlocks_list.RemoveAt(posLast);

					// Request next block
						if (thePrefs.GetDebugClientTCPLevel() > 0)
							DebugSend("More block requests", this);
					SendBlockRequests();	
				}
			}

			// Stop looping and exit method
			return;
			}
		}

	TRACE("%s - Dropping packet\n", __FUNCTION__);
}

int CUpDownClient::unzip(Pending_Block_Struct *block, BYTE *zipped, uint32 lenZipped, BYTE **unzipped, uint32 *lenUnzipped, int iRecursion)
{
#define TRACE_UNZIP	/*TRACE*/

	TRACE_UNZIP("unzip: Zipd=%6u Unzd=%6u Rcrs=%d", lenZipped, *lenUnzipped, iRecursion);
  	int err = Z_DATA_ERROR;
  	try
	{
	    // Save some typing
	    z_stream *zS = block->zStream;
    
	    // Is this the first time this block has been unzipped
	    if (zS == NULL)
	    {
		    // Create stream
		    block->zStream = new z_stream;
		    zS = block->zStream;
    
		    // Initialise stream values
		    zS->zalloc = (alloc_func)0;
		    zS->zfree = (free_func)0;
		    zS->opaque = (voidpf)0;
    
		    // Set output data streams, do this here to avoid overwriting on recursive calls
		    zS->next_out = (*unzipped);
		    zS->avail_out = (*lenUnzipped);
    
		    // Initialise the z_stream
		    err = inflateInit(zS);
			if (err != Z_OK){
				TRACE_UNZIP("; Error: new stream failed: %d\n", err);
			    return err;
			}

			ASSERT( block->totalUnzipped == 0 );
		}

	    // Use whatever input is provided
	    zS->next_in  = zipped;
	    zS->avail_in = lenZipped;
    
	    // Only set the output if not being called recursively
	    if (iRecursion == 0)
	    {
		    zS->next_out = (*unzipped);
		    zS->avail_out = (*lenUnzipped);
	    }
    
	    // Try to unzip the data
		TRACE_UNZIP("; inflate(ain=%6u tin=%6u aout=%6u tout=%6u)", zS->avail_in, zS->total_in, zS->avail_out, zS->total_out);
	    err = inflate(zS, Z_SYNC_FLUSH);
    
	    // Is zip finished reading all currently available input and writing all generated output
	    if (err == Z_STREAM_END)
	    {
		    // Finish up
		    err = inflateEnd(zS);
			if (err != Z_OK){
				TRACE_UNZIP("; Error: end stream failed: %d\n", err);
			    return err;
			}
			TRACE_UNZIP("; Z_STREAM_END\n");

		    // Got a good result, set the size to the amount unzipped in this call (including all recursive calls)
		    (*lenUnzipped) = (zS->total_out - block->totalUnzipped);
		    block->totalUnzipped = zS->total_out;
	    }
	    else if ((err == Z_OK) && (zS->avail_out == 0) && (zS->avail_in != 0))
	    {
		    // Output array was not big enough, call recursively until there is enough space
			TRACE_UNZIP("; output array not big enough (ain=%u)\n", zS->avail_in);
    
		    // What size should we try next
		    uint32 newLength = (*lenUnzipped) *= 2;
		    if (newLength == 0)
			    newLength = lenZipped * 2;
    
		    // Copy any data that was successfully unzipped to new array
		    BYTE *temp = new BYTE[newLength];
			ASSERT( zS->total_out - block->totalUnzipped <= newLength );
		    memcpy(temp, (*unzipped), (zS->total_out - block->totalUnzipped));
		    delete [] (*unzipped);
		    (*unzipped) = temp;
		    (*lenUnzipped) = newLength;
    
		    // Position stream output to correct place in new array
		    zS->next_out = (*unzipped) + (zS->total_out - block->totalUnzipped);
		    zS->avail_out = (*lenUnzipped) - (zS->total_out - block->totalUnzipped);
    
		    // Try again
		    err = unzip(block, zS->next_in, zS->avail_in, unzipped, lenUnzipped, iRecursion + 1);
	    }
	    else if ((err == Z_OK) && (zS->avail_in == 0))
	    {
			TRACE_UNZIP("; all input processed\n");
		    // All available input has been processed, everything ok.
		    // Set the size to the amount unzipped in this call (including all recursive calls)
		    (*lenUnzipped) = (zS->total_out - block->totalUnzipped);
		    block->totalUnzipped = zS->total_out;
	    }
	    else
	    {
		    // Should not get here unless input data is corrupt
			if (thePrefs.GetVerbose())
			{
				CString strZipError;
				if (zS->msg)
					strZipError.Format(_T(" %d: '%hs'"), err, zS->msg);
				else if (err != Z_OK)
					strZipError.Format(_T(" %d"), err);
				TRACE_UNZIP("; Error: %s\n", strZipError);
				AddDebugLogLine(false, _T("Unexpected zip error%s in file \"%s\""), strZipError, reqfile ? reqfile->GetFileName() : NULL);
			}
	    }
    
	    if (err != Z_OK)
		    (*lenUnzipped) = 0;
  	}
  	catch (...){
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Unknown exception in %hs: file \"%s\""), __FUNCTION__, reqfile ? reqfile->GetFileName() : NULL);
		err = Z_DATA_ERROR;
		ASSERT(0);
	}

  	return err;
}

uint32 CUpDownClient::CalculateDownloadRate(){
	// Patch By BadWolf - Accurate datarate Calculation
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	//MORPH START - Changed by SiRoB, Changed by SiRoB, Better datarate mesurement for low and high speed
	uint32 cur_tick = ::GetTickCount();
    if(m_nDownDataRateMS > 0) {
		TransferredData newitem = {m_nDownDataRateMS,cur_tick};
		m_AvarageDDR_list.AddTail(newitem);
		m_nSumForAvgDownDataRate += m_nDownDataRateMS;
		m_nDownDataRateMS = 0;
    }
	
	while (m_AvarageDDR_list.GetCount() > 0)
		if((cur_tick - m_AvarageDDR_list.GetHead().timestamp) > 30000)
			m_nSumForAvgDownDataRate -= m_AvarageDDR_list.RemoveHead().datalen;
		else
			break;
	
	if (m_AvarageDDR_list.GetCount() > 0){
		if (m_AvarageDDR_list.GetCount() == 1)
			m_nDownDatarate = (m_nSumForAvgDownDataRate*1000) / 30000;
		else{
			DWORD dwDuration = m_AvarageDDR_list.GetTail().timestamp - m_AvarageDDR_list.GetHead().timestamp;
			if ((m_AvarageDDR_list.GetCount() - 1)*(cur_tick - m_AvarageDDR_list.GetTail().timestamp) > dwDuration)
				dwDuration = cur_tick - m_AvarageDDR_list.GetHead().timestamp - dwDuration / (m_AvarageDDR_list.GetCount() - 1);
			if (dwDuration < 5000) dwDuration = 5000;
			m_nDownDatarate = ((m_nSumForAvgDownDataRate - m_AvarageDDR_list.GetHead().datalen)*1000) / dwDuration;
		}
	}else
		m_nDownDatarate = 0;
	
	UpdateDisplayedInfo();

	//MORPH END   - Changed by SiRoB, Better datarate mesurement for low and high speed
	
	return m_nDownDatarate;
}

void CUpDownClient::CheckDownloadTimeout()
{
	if (IsDownloadingFromPeerCache() && m_pPCDownSocket && m_pPCDownSocket->IsConnected())
	{
		ASSERT( DOWNLOADTIMEOUT < m_pPCDownSocket->GetTimeOut() );
		if (GetTickCount() - m_dwLastBlockReceived > DOWNLOADTIMEOUT)
		{
			OnPeerCacheDownSocketTimeout();
		}
	}
	else
	{
		if ((::GetTickCount() - m_dwLastBlockReceived) > DOWNLOADTIMEOUT)
		{
			ASSERT( socket != NULL );
			if (socket != NULL)
			{
				ASSERT( !socket->IsRawDataMode() );
				if (!socket->IsRawDataMode())
					SendCancelTransfer();
			}
			SetDownloadState(DS_ONQUEUE);
		}
	}
}

UINT CUpDownClient::GetAvailablePartCount() const
{
	UINT result = 0;
	for (int i = 0;i < m_nPartCount;i++){
		if (IsPartAvailable(i))
			result++;
	}
	return result;
}

void CUpDownClient::SetRemoteQueueRank(uint16 nr){

	//Morph - added by AndCycle, DiffQR
	if(nr == 0){
		m_iDifferenceQueueRank = 0;
	}
	else if(m_nRemoteQueueRank){
		m_iDifferenceQueueRank = (nr-m_nRemoteQueueRank);
	}
	//Morph - added by AndCycle, DiffQR

	m_nRemoteQueueRank = nr;
	UpdateDisplayedInfo();
}

void CUpDownClient::UDPReaskACK(uint16 nNewQR){
	m_bUDPPending = false;
	SetRemoteQueueRank(nNewQR);
    SetLastAskedTime(); // ZZ:DownloadManager
}

void CUpDownClient::UDPReaskFNF(){
	m_bUDPPending = false;
	if (GetDownloadState()!=DS_DOWNLOADING){ // avoid premature deletion of 'this' client
		if (thePrefs.GetVerbose())
			AddDebugLogLine(DLP_LOW, false, _T("UDP ANSWER FNF : %s - %s"),DbgGetClientInfo(), DbgGetFileInfo(reqfile ? reqfile->GetFileHash() : NULL));
		switch (GetDownloadState()) {
			case DS_ONQUEUE:
			case DS_NONEEDEDPARTS:
                DontSwapTo(reqfile); // ZZ:DownloadManager
                if (SwapToAnotherFile(_T("Source says it doesn't have the file. CUpDownClient::UDPReaskFNF()"), true, true, true, NULL, false, false))
					break;
				/*fall through*/
			default:
				theApp.downloadqueue->RemoveSource(this);
				if (!socket){
					if (Disconnected(_T("UDPReaskFNF socket=NULL")))
						delete this;
				}
		}
	}
	else
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false,_T("UDP ANSWER FNF : %s - did not remove client because of current download state"),GetUserName());
	}
}

void CUpDownClient::UDPReaskForDownload()
{
	ASSERT ( reqfile );
	if(!reqfile || m_bUDPPending)
		return;

	if( m_nTotalUDPPackets > 3 && ((float)(m_nFailedUDPPackets/m_nTotalUDPPackets) > .3))
		return;

	//the line "m_bUDPPending = true;" use to be here
	// deadlake PROXYSUPPORT
	const ProxySettings& proxy = thePrefs.GetProxy();
	if(m_nUDPPort != 0 && thePrefs.GetUDPPort() != 0 &&
	   !theApp.IsFirewalled() && !HasLowID() && !(socket && socket->IsConnected())&& (!proxy.UseProxy))
	{ 
		// deadlake PROXYSUPPORT
		//don't use udp to ask for sources
		if(IsSourceRequestAllowed())
			return;

// ZZ:DownloadManager -->
        if(SwapToAnotherFile(_T("A4AF check before OP__ReaskFilePing. CUpDownClient::UDPReaskForDownload()"), true, false, false, NULL, true, true)) {
            return; // we swapped, so need to go to tcp
        }
// ZZ:DownloadManager <--

		m_bUDPPending = true;
		CSafeMemFile data(128);
		data.WriteHash16(reqfile->GetFileHash());
		if (GetUDPVersion() > 3)
		{
			if (reqfile->IsPartFile())
				((CPartFile*)reqfile)->WritePartStatus(&data);
			else
				data.WriteUInt16(0);
		}
		if (GetUDPVersion() > 2)
			data.WriteUInt16(reqfile->m_nCompleteSourcesCount);
		if (thePrefs.GetDebugClientUDPLevel() > 0)
			DebugSend("OP__ReaskFilePing", this, (char*)reqfile->GetFileHash());
		Packet* response = new Packet(&data, OP_EMULEPROT);
		response->opcode = OP_REASKFILEPING;
		theStats.AddUpDataOverheadFileRequest(response->size);
		theApp.downloadqueue->AddUDPFileReasks();
		theApp.clientudp->SendPacket(response,GetIP(),GetUDPPort());
		m_nTotalUDPPackets++;
	}
}

// SLUGFILLER: SafeHash
void CUpDownClient::RequestHashset(){
	if (socket)
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__HashSetRequest", this, (char*)reqfile->GetFileHash());
			Packet* packet = new Packet(OP_HASHSETREQUEST,16);
			md4cpy(packet->pBuffer,reqfile->GetFileHash());
			theStats.AddUpDataOverheadFileRequest(packet->size);
			socket->SendPacket(packet, true, true);
			SetDownloadState(DS_REQHASHSET);
			m_fHashsetRequesting = 1;
			reqfile->hashsetneeded = false;
	}
	else
		ASSERT(0);
}
// SLUGFILLER: SafeHash

void CUpDownClient::UpdateDisplayedInfo(bool force)
{
#ifdef _DEBUG
	force = true;
#endif
	DWORD curTick = ::GetTickCount();

    if(force || (curTick-m_lastRefreshedDLDisplay) > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+(uint32)(rand()/(RAND_MAX/1000))) {
	    theApp.emuledlg->transferwnd->downloadlistctrl.UpdateItem(this);
		theApp.emuledlg->transferwnd->clientlistctrl.RefreshClient(this);
        m_lastRefreshedDLDisplay = curTick;
    }
}

// ZZ:DownloadManager -->
const bool CUpDownClient::IsInOtherRequestList(const CPartFile* fileToCheck) const {
    for (POSITION pos = m_OtherRequests_list.GetHeadPosition();pos != 0;m_OtherRequests_list.GetNext(pos)) {
        if(m_OtherRequests_list.GetAt(pos) == fileToCheck) {
            return true;
        }
    }

    return false;
}
// <-- ZZ:DownloadManager

// ZZ:DownloadManager -->
const bool CUpDownClient::SwapToRightFile(CPartFile* SwapTo, CPartFile* cur_file, bool ignoreSuspensions, bool SwapToIsNNPFile, bool curFileisNNPFile, bool& wasSkippedDueToSourceExchange, bool doAgressiveSwapping, bool debug) {
    bool printDebug = debug && thePrefs.GetLogA4AF();

    if(printDebug) {
        AddDebugLogLine(DLP_LOW, false, _T("oooo Debug: SwapToRightFile. Start compare SwapTo: %s and cur_file %s"), SwapTo?SwapTo->GetFileName():_T("null"), cur_file->GetFileName());
        AddDebugLogLine(DLP_LOW, false, _T("oooo Debug: doAgressiveSwapping: %s"), doAgressiveSwapping?_T("true"):_T("false"));
    }

    if (!SwapTo) {
        return true;
    }
	//MORPH START - Added by SiRoB, ForcedA4AF
	if (thePrefs.UseSmartA4AFSwapping())
	{
		if (cur_file == theApp.downloadqueue->forcea4af_file)
		{
			if(printDebug)
                AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: Don't swap because cur_file have the GetAllA4AF checked."));
			return true;
		} else if (cur_file->ForceAllA4AF())
		{
			if(printDebug)
                AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: Don't swap because cur_file have the ForcedA4AFON checked."));
			return true;
		}
	}

	//MORPH END   - Added by SiRoB, ForcedA4AF
						
    if(!curFileisNNPFile && cur_file->GetSourceCount() < thePrefs.GetMaxSourcePerFile() ||
        curFileisNNPFile && cur_file->GetSourceCount() < thePrefs.GetMaxSourcePerFile()*.8) {

            if(printDebug)
                AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: cur_file does probably not have too many sources."));

            if(SwapTo->GetSourceCount() > thePrefs.GetMaxSourcePerFile() ||
               SwapTo->GetSourceCount() >= thePrefs.GetMaxSourcePerFile()*.8 &&
               SwapTo == reqfile &&
               (
                GetDownloadState() == DS_LOWTOLOWIP ||
                GetDownloadState() == DS_REMOTEQUEUEFULL
               )
              ) {
                if(printDebug)
                    AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: SwapTo is about to be deleted due to too many sources on that file, so we can steal it."));
                return true;
            }

            if(SwapToIsNNPFile && !curFileisNNPFile) {
                if(printDebug)
                    AddDebugLogLine(DLP_LOW, false, _T("oooo Debug: SwapTo is NNP and cur_file is not."));
                return true;
            }
			//MORPH START - Added by SiRoB, Avanced A4AF
			uint8 cur_file_iA4AFMode = thePrefs.AdvancedA4AFMode();
			if (cur_file_iA4AFMode && thePrefs.GetCategory(cur_file->GetCategory())->iAdvA4AFMode)
				cur_file_iA4AFMode = thePrefs.GetCategory(cur_file->GetCategory())->iAdvA4AFMode;
			uint8 SwapTo_iA4AFMode = thePrefs.AdvancedA4AFMode();
			if (SwapTo_iA4AFMode && thePrefs.GetCategory(SwapTo->GetCategory())->iAdvA4AFMode)
				SwapTo_iA4AFMode = thePrefs.GetCategory(SwapTo->GetCategory())->iAdvA4AFMode;
			
			//MORPH END   - Added by SiRoB, Avanced A4AF
            if(SwapToIsNNPFile == curFileisNNPFile) {
                if(printDebug)
                    AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: SwapToIsNNPFile == curFileisNNPFile"));

                if(ignoreSuspensions  || !IsSwapSuspended(cur_file, doAgressiveSwapping, curFileisNNPFile)) {
                    if(printDebug)
                        AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: No suspend block."));

                    if(
						//MORPH START - Added by SiRoB, ForcedA4AF
						(
							thePrefs.UseSmartA4AFSwapping() && SwapTo != theApp.downloadqueue->forcea4af_file
							||
							!thePrefs.UseSmartA4AFSwapping()
						)
						&&
						(
							!(thePrefs.UseSmartA4AFSwapping() && cur_file->ForceA4AFOff()) &&
						//MORPH END   - Added by SiRoB, ForcedA4AF
							!SwapTo->IsA4AFAuto() &&
							(
								cur_file->IsA4AFAuto() ||
								//MORPH START - Added by SiRoB, Stacking A4AF
								(
									SwapTo_iA4AFMode == 2 &&
									cur_file->GetCatResumeOrder() < SwapTo->GetCatResumeOrder() ||
									(
										cur_file->GetCatResumeOrder() == SwapTo->GetCatResumeOrder() &&
										SwapTo_iA4AFMode == 2 &&
										SwapTo_iA4AFMode == cur_file_iA4AFMode
										||
										SwapTo_iA4AFMode != 2 &&
										SwapTo_iA4AFMode == cur_file_iA4AFMode
									) &&
								//MORPH END   - Added by SiRoB, Stacking A4AF		
									(
										thePrefs.GetCategory(cur_file->GetCategory())->prio > thePrefs.GetCategory(SwapTo->GetCategory())->prio ||
										thePrefs.GetCategory(cur_file->GetCategory())->prio == thePrefs.GetCategory(SwapTo->GetCategory())->prio &&
										(
											cur_file->GetDownPriority() > SwapTo->GetDownPriority() ||
											cur_file->GetDownPriority() == SwapTo->GetDownPriority() &&
											(
												cur_file->GetCategory() == SwapTo->GetCategory() && cur_file->GetCategory() != 0 &&
												(thePrefs.GetCategory(cur_file->GetCategory())->downloadInAlphabeticalOrder && thePrefs.IsExtControlsEnabled()) && 
												cur_file->GetFileName() && SwapTo->GetFileName() &&
												cur_file->GetFileName().CompareNoCase(SwapTo->GetFileName()) < 0
												//MORPH START - Added by SiRoB, Balancing A4AF
												||
												SwapTo_iA4AFMode != 0 &&
												cur_file->GetAvailableSrcCount() < SwapTo->GetAvailableSrcCount()
												//MORPH END   - Added by SiRoB, Balancing A4AF
											)
										)
									)
								) //MORPH END   - Added by SiRoB, Stacking A4AF
							)
						) //MORPH - Added by SiRoB, ForcedA4AF
					) {
                        if(printDebug)
                            AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: Higher prio."));

                        if(IsSourceRequestAllowed(cur_file) && (cur_file->AllowSwapForSourceExchange() || cur_file == reqfile) ||
                           !(IsSourceRequestAllowed(SwapTo) && (SwapTo->AllowSwapForSourceExchange() || SwapTo == reqfile)) ||
                           (GetDownloadState()==DS_ONQUEUE && GetRemoteQueueRank() <= 50)) {
                            if(printDebug)
                                AddDebugLogLine(DLP_LOW, false, _T("oooo Debug: Source Request check ok."));
                            return true;
                        } else {
                            if(printDebug)
                                AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: Source Request check failed."));
                            wasSkippedDueToSourceExchange = true;
                        }
                    }

				    if(IsSourceRequestAllowed(cur_file, true) && (cur_file->AllowSwapForSourceExchange() || cur_file == reqfile) &&
                       !(IsSourceRequestAllowed(SwapTo, true) && (SwapTo->AllowSwapForSourceExchange() || SwapTo == reqfile)) &&
                       (GetDownloadState()!=DS_ONQUEUE || GetDownloadState()==DS_ONQUEUE && GetRemoteQueueRank() > 50)) {
                        wasSkippedDueToSourceExchange = true;

                        if(printDebug)
                            AddDebugLogLine(DLP_LOW, false, _T("oooo Debug: Source Exchange."));
                        return true;
                    }
                } else if(printDebug) {
                    AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: Suspend block."));
                }
            } else if(printDebug) {
                AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: SwapTo is NNP and cur_file is not."));
            }
    } else if(printDebug) {
        AddDebugLogLine(DLP_VERYLOW, false, _T("oooo Debug: cur_file probably has too many sources."));
    }

    if(printDebug)
        AddDebugLogLine(DLP_LOW, false, _T("oooo Debug: Return false"));

    return false;
}
// <-- ZZ:DownloadManager

// ZZ:DownloadManager -->
bool CUpDownClient::SwapToAnotherFile(LPCTSTR reason, bool bIgnoreNoNeeded, bool ignoreSuspensions, bool bRemoveCompletely, CPartFile* toFile, bool allowSame, bool isAboutToAsk, bool debug){
    bool printDebug = debug && thePrefs.GetLogA4AF();

    //if(GetDownloadState() == DS_NONEEDEDPARTS) {
    //    printDebug = true;
    //}

    if(printDebug)
        AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Switching source \"%s\" Status %i; Remove = %s; bIgnoreNoNeeded = %s; allowSame = %s; Reason = \"%s\""), this->GetUserName(), this->GetDownloadState(), (bRemoveCompletely ? _T("Yes") : _T("No")), (bIgnoreNoNeeded ? _T("Yes") : _T("No")), (allowSame ? _T("Yes") : _T("No")), reason);

    if(!bRemoveCompletely && allowSame && thePrefs.GetA4AFSaveCpu()) {
        // Only swap if we can't keep the old source
        if(printDebug)
            AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: return false since prefs setting to save cpu is enabled."));
        return false;
    }

    if(!bRemoveCompletely && allowSame && m_OtherRequests_list.IsEmpty() && (!bIgnoreNoNeeded || m_OtherNoNeeded_list.IsEmpty())) {
        // no file to swap too, and it's ok to keep it
        if(printDebug)
            AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: return false due to no file to swap too, and it's ok to keep it."));
        return false;
    }

    if (!bRemoveCompletely &&
        (GetDownloadState() != DS_ONQUEUE &&
         GetDownloadState() != DS_NONEEDEDPARTS &&
         GetDownloadState() != DS_TOOMANYCONNS &&
         GetDownloadState() != DS_REMOTEQUEUEFULL &&
         GetDownloadState() != DS_CONNECTED
        )) {
        if(printDebug)
            AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: return false due to wrong state."));
		return false;
    }

    bool doAgressiveSwapping = (bRemoveCompletely || !allowSame || isAboutToAsk);
    if(printDebug)
        AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: doAgressiveSwapping: %s"), doAgressiveSwapping?_T("true"):_T("false"));

	CPartFile* SwapTo = NULL;
	CPartFile* cur_file = NULL;
	//int cur_prio= -1; //ZZ:DownloadManager
	POSITION finalpos = NULL;
	CTypedPtrList<CPtrList, CPartFile*>* usedList;

    if(allowSame && !bRemoveCompletely) {
        SwapTo = reqfile;
        if(printDebug)
            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: allowSame: File %s SourceReq: %s"), reqfile->GetFileName(), IsSourceRequestAllowed(reqfile)?_T("true"):_T("false"));
    }

    bool SwapToIsNNP = (SwapTo != NULL && SwapTo == reqfile && GetDownloadState() == DS_NONEEDEDPARTS);

    CPartFile* skippedDueToSourceExchange = NULL;
    bool skippedIsNNP = false;

	if (!m_OtherRequests_list.IsEmpty()){
        if(printDebug)
            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: m_OtherRequests_list"));

		usedList = &m_OtherRequests_list;
		for (POSITION pos = m_OtherRequests_list.GetHeadPosition();pos != 0;m_OtherRequests_list.GetNext(pos)){
			cur_file = m_OtherRequests_list.GetAt(pos);

            if(printDebug)
                AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Checking file: %s SoureReq: %s"), cur_file->GetFileName(), IsSourceRequestAllowed(cur_file)?_T("true"):_T("false"));

            if(!bRemoveCompletely && !ignoreSuspensions && allowSame && IsSwapSuspended(cur_file, doAgressiveSwapping, false)) {
                if(printDebug)
                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: continue due to IsSwapSuspended(file) == true"));
                continue;
            }

			if (cur_file != reqfile && theApp.downloadqueue->IsPartFile(cur_file) && !cur_file->IsStopped() 
				&& (!cur_file->notSeenCompleteSource()) //EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
				&& (cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY))	
			{
                if(printDebug)
                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: It's a partfile, not stopped, etc."));

				if (toFile != NULL){
					if (cur_file == toFile){
                        if(printDebug)
                            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Found toFile."));

                        SwapTo = cur_file;
                        SwapToIsNNP = false;
						finalpos = pos;
						break;
					}
				} else {
                    bool wasSkippedDueToSourceExchange = false;
                    if(SwapToRightFile(SwapTo, cur_file, ignoreSuspensions, SwapToIsNNP, false, wasSkippedDueToSourceExchange, doAgressiveSwapping, debug)) {
                        if(printDebug)
                            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Swapping to file %s"), cur_file->GetFileName());

                        if(SwapTo && wasSkippedDueToSourceExchange) {
                            if(debug && thePrefs.GetLogA4AF()) AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Swapped due to source exchange possibility"));
                            bool discardSkipped = false;
                            if(SwapToRightFile(skippedDueToSourceExchange, SwapTo, ignoreSuspensions, skippedIsNNP, SwapToIsNNP, discardSkipped, doAgressiveSwapping, debug)) {
                                skippedDueToSourceExchange = SwapTo;
                                skippedIsNNP = skippedIsNNP?true:(SwapTo == reqfile && GetDownloadState() == DS_NONEEDEDPARTS);
                                if(printDebug)
                                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Skipped file was better than last skipped file."));
                            }
                        }
				
                        SwapTo = cur_file;
                        SwapToIsNNP = false;
					    finalpos=pos;
                    } else {
                        if(printDebug)
                            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Keeping file %s"), SwapTo->GetFileName());
                        if(wasSkippedDueToSourceExchange) {
                            if(printDebug)
                                AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Kept the file due to source exchange possibility"));
                            bool discardSkipped = false;
                            if(SwapToRightFile(skippedDueToSourceExchange, cur_file, ignoreSuspensions, skippedIsNNP, false, discardSkipped, doAgressiveSwapping, debug)) {
                                skippedDueToSourceExchange = cur_file;
                                skippedIsNNP = false;
                                if(printDebug)
                                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Skipped file was better than last skipped file."));
                            }
                        }
                    }
                }
			}
		}
	}

    if ((!SwapTo || SwapTo == reqfile && GetDownloadState() == DS_NONEEDEDPARTS) && bIgnoreNoNeeded){
        if(printDebug)
            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: m_OtherNoNeeded_list"));

		usedList = &m_OtherNoNeeded_list;
		for (POSITION pos = m_OtherNoNeeded_list.GetHeadPosition();pos != 0;m_OtherNoNeeded_list.GetNext(pos)){
			cur_file = m_OtherNoNeeded_list.GetAt(pos);

            if(printDebug)
                AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Checking file: %s "), cur_file->GetFileName());

            if(!bRemoveCompletely && !ignoreSuspensions && allowSame && IsSwapSuspended(cur_file, doAgressiveSwapping, true)) {
                if(printDebug)
                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: continue due to !IsSwapSuspended(file) == true"));
                continue;
            }

			if (cur_file != reqfile && theApp.downloadqueue->IsPartFile(cur_file) && !cur_file->IsStopped() 
				&& (cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY) )	
			{
                if(printDebug)
                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: It's a partfile, not stopped, etc."));

				if (toFile != NULL){
					if (cur_file == toFile){
                        if(printDebug)
                            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Found toFile."));
    
						SwapTo = cur_file;
						finalpos = pos;
						break;
					}
				} else {
                    bool wasSkippedDueToSourceExchange = false;
                    if(SwapToRightFile(SwapTo, cur_file, ignoreSuspensions, SwapToIsNNP, true, wasSkippedDueToSourceExchange, doAgressiveSwapping, debug)) {
                        if(printDebug)
                            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Swapping to file %s"), cur_file->GetFileName());

                        if(SwapTo && wasSkippedDueToSourceExchange) {
                            if(printDebug)
                                AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Swapped due to source exchange possibility"));
                            bool discardSkipped = false;
                            if(SwapToRightFile(skippedDueToSourceExchange, SwapTo, ignoreSuspensions, skippedIsNNP, SwapToIsNNP, discardSkipped, doAgressiveSwapping, debug)) {
                                skippedDueToSourceExchange = SwapTo;
                                skippedIsNNP = skippedIsNNP?true:(SwapTo == reqfile && GetDownloadState() == DS_NONEEDEDPARTS);
                                if(printDebug)
                                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Skipped file was better than last skipped file."));
                            }
                        }

                        SwapTo = cur_file;
                        SwapToIsNNP = true;
					    finalpos=pos;
                    } else {
                        if(printDebug)
                            AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Keeping file %s"), SwapTo->GetFileName());
                        if(wasSkippedDueToSourceExchange) {
                            if(debug && thePrefs.GetVerbose()) AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Kept the file due to source exchange possibility"));
                            bool discardSkipped = false;
                            if(SwapToRightFile(skippedDueToSourceExchange, cur_file, ignoreSuspensions, skippedIsNNP, true, discardSkipped, doAgressiveSwapping, debug)) {
                                skippedDueToSourceExchange = cur_file;
                                skippedIsNNP = true;
                                if(printDebug)
                                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Skipped file was better than last skipped file."));
                            }
                        }
                    }
				}
			}
		}
	}

	if (SwapTo){
        if(printDebug) {
            if(SwapTo != reqfile) {
                AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Found file to swap to %s"), SwapTo->GetFileName());
            } else {
                AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Will keep current file. %s"), SwapTo->GetFileName());
            }
        }

		CString strInfo(reason);
        if(skippedDueToSourceExchange) {
            bool wasSkippedDueToSourceExchange = false;
            bool skippedIsBetter = SwapToRightFile(SwapTo, skippedDueToSourceExchange, ignoreSuspensions, SwapToIsNNP, skippedIsNNP, wasSkippedDueToSourceExchange, doAgressiveSwapping, debug);
            if(skippedIsBetter || wasSkippedDueToSourceExchange) {
                SwapTo->SetSwapForSourceExchangeTick();
                strInfo = _T("******SourceExchange-Swap****** ") + strInfo;
                if(printDebug) {
                    AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Due to sourceExchange."));
                } else if(thePrefs.GetLogA4AF() && reqfile == SwapTo) {
                    AddDebugLogLine(DLP_LOW, false, _T("ooo Didn't swap source due to source exchange possibility. '%s' Status %i; Remove = %s '%s' Reason: %s"), this->GetUserName(), this->GetDownloadState(), (bRemoveCompletely ? _T("Yes") : _T("No") ), (this->reqfile)?this->reqfile->GetFileName():_T("null"), strInfo);
                }
            } else if(printDebug) {
				AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Normal. SwapTo better than skippedDueToSourceExchange."));
            }
        } else if(printDebug) {
			AddDebugLogLine(DLP_VERYLOW, false, _T("ooo Debug: Normal. skippedDueToSourceExchange == NULL"));
        }

		if (SwapTo != reqfile && DoSwap(SwapTo,bRemoveCompletely, strInfo)){
            if(debug && thePrefs.GetLogA4AF()) AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Swap successful."));
			usedList->RemoveAt(finalpos);
			return true;
        } else if(printDebug) {
            AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Swap didn't happen."));
		}
	}

    if(printDebug)
        AddDebugLogLine(DLP_LOW, false, _T("ooo Debug: Done \"%s\""), this->GetUserName());

	return false;
}


bool CUpDownClient::DoSwap(CPartFile* SwapTo, bool bRemoveCompletely, LPCTSTR reason) // ZZ:DownloadManager
{
    if (thePrefs.GetLogA4AF()) // ZZ:DownloadManager
        AddDebugLogLine(DLP_LOW, false, _T("ooo Swapped source '%s' Status %i; Remove = %s '%s'   -->   %s Reason: %s"), this->GetUserName(), this->GetDownloadState(), (bRemoveCompletely ? _T("Yes") : _T("No") ), (this->reqfile)?this->reqfile->GetFileName():_T("null"), SwapTo->GetFileName(), reason); // ZZ:DownloadManager

	// 17-Dez-2003 [bc]: This "reqfile->srclists[sourcesslot].Find(this)" was the only place where 
	// the usage of the "CPartFile::srclists[100]" is more effective than using one list. If this
	// function here is still (again) a performance problem there is a more effective way to handle
	// the 'Find' situation. Hint: usage of a node ptr which is stored in the CUpDownClient.
	POSITION pos = reqfile->srclist.Find(this);
	if(pos)
	{
    	reqfile->srclist.RemoveAt(pos);
    } else {
        AddDebugLogLine(DLP_HIGH, true, _T("o-o Unsync between parfile->srclist and client otherfiles list. Swapping client where client has file as reqfile, but file doesn't have client in srclist. '%s' Status %i; Remove = %s '%s'   -->   '%s'  SwapReason: %s"), this->GetUserName(), this->GetDownloadState(), (bRemoveCompletely ? _T("Yes") : _T("No") ), (this->reqfile)?this->reqfile->GetFileName():_T("null"), SwapTo->GetFileName(), reason); // ZZ:DownloadManager
    }

	// remove this client from the A4AF list of our new reqfile
	POSITION pos2 = SwapTo->A4AFsrclist.Find(this);
	if (pos2){
		SwapTo->A4AFsrclist.RemoveAt(pos2);
    } else {
        AddDebugLogLine(DLP_HIGH, true, _T("o-o Unsync between parfile->srclist and client otherfiles list. Swapping client where client has file in another list, but file doesn't have client in a4af srclist. '%s' Status %i; Remove = %s '%s'   -->   '%s'  SwapReason: %s"), this->GetUserName(), this->GetDownloadState(), (bRemoveCompletely ? _T("Yes") : _T("No") ), (this->reqfile)?this->reqfile->GetFileName():_T("null"), SwapTo->GetFileName(), reason); // ZZ:DownloadManager
    }
	theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource(this,SwapTo);

	reqfile->RemoveDownloadingSource(this);

	if(!bRemoveCompletely)
	{
		reqfile->A4AFsrclist.AddTail(this);
		if (GetDownloadState() == DS_NONEEDEDPARTS)
			m_OtherNoNeeded_list.AddTail(reqfile);
		else
			m_OtherRequests_list.AddTail(reqfile);

		theApp.emuledlg->transferwnd->downloadlistctrl.AddSource(reqfile,this,true);
	}

	SetDownloadState(DS_NONE);
	ResetFileStatusInfo();
	m_nRemoteQueueRank = 0;
	m_iDifferenceQueueRank = 0;	//Morph - added by AndCycle, DiffQR

	reqfile->UpdatePartsInfo();
	reqfile->UpdateAvailablePartsCount();
	SetRequestFile(SwapTo);

	SwapTo->srclist.AddTail(this);
	theApp.emuledlg->transferwnd->downloadlistctrl.AddSource(SwapTo,this,false);

	return true;
}

void CUpDownClient::DontSwapTo(/*const*/ CPartFile* file)
{
	DWORD dwNow = ::GetTickCount();

	for (POSITION pos = m_DontSwap_list.GetHeadPosition(); pos != 0; m_DontSwap_list.GetNext(pos))
		if(m_DontSwap_list.GetAt(pos).file == file) {
			m_DontSwap_list.GetAt(pos).timestamp = dwNow ;
			return;
		}
	PartFileStamp newfs = {file, dwNow };
	m_DontSwap_list.AddHead(newfs);
}

bool CUpDownClient::IsSwapSuspended(const CPartFile* file, const bool allowShortReaskTime, const bool fileIsNNP) // ZZ:DownloadManager
{
// ZZ:DownloadManager -->
    // Don't swap if we have reasked this client too recently
    if(GetTimeUntilReask(file, allowShortReaskTime) > 0)
        return true;
// <-- ZZ:DownloadManager

	if (m_DontSwap_list.GetCount()==0)
		return false;

	for (POSITION pos = m_DontSwap_list.GetHeadPosition(); pos != 0 && m_DontSwap_list.GetCount()>0; m_DontSwap_list.GetNext(pos)){
		if(m_DontSwap_list.GetAt(pos).file == file){
			if ( ::GetTickCount() - m_DontSwap_list.GetAt(pos).timestamp  >= PURGESOURCESWAPSTOP ) {
				m_DontSwap_list.RemoveAt(pos);
				return false;
			}
			else
				return true;
		}
		else if (m_DontSwap_list.GetAt(pos).file == NULL) // in which cases should this happen?
			m_DontSwap_list.RemoveAt(pos);
	}

	return false;
}

uint32 CUpDownClient::GetTimeUntilReask(const CPartFile* file, const bool allowShortReaskTime) const {
    DWORD lastAskedTimeTick = GetLastAskedTime(file);
    if(lastAskedTimeTick != 0) {
        DWORD tick = ::GetTickCount();

        DWORD reaskTime;
        if(allowShortReaskTime) {
            reaskTime = MIN_REQUESTTIME;
        } else if(m_OtherRequests_list.GetSize() == 0 &&
           (file == reqfile && GetDownloadState() == DS_NONEEDEDPARTS ||
           file != reqfile /*&& !IsInOtherRequestList(file)*/)) {
            reaskTime = FILEREASKTIME*2;
        } else {
            reaskTime = FILEREASKTIME;
        }

        if(tick-lastAskedTimeTick < reaskTime) {
            return reaskTime-(tick-lastAskedTimeTick);
        } else {
            return 0;
        }
    } else {
        return 0;
    }
}

uint32 CUpDownClient::GetTimeUntilReask(const CPartFile* file) const {
    return GetTimeUntilReask(file, false);
}

uint32 CUpDownClient::GetTimeUntilReask() const {
    return GetTimeUntilReask(reqfile);
}

bool CUpDownClient::IsValidSource() const
{
	bool valid = false;
	switch(GetDownloadState())
	{
		case DS_DOWNLOADING:
		case DS_ONQUEUE:
		case DS_CONNECTED:
		case DS_NONEEDEDPARTS:
		case DS_REMOTEQUEUEFULL:
		case DS_REQHASHSET:
			valid = IsEd2kClient();
	}
	return valid;
}


void CUpDownClient::StartDownload()
{
	SetDownloadState(DS_DOWNLOADING);
	InitTransferredDownMini();
	SetDownStartTime();
	m_lastPartAsked = 0xffff;
	SendBlockRequests();
}

void CUpDownClient::SendCancelTransfer(Packet* packet)
{
	if (socket == NULL || !IsEd2kClient()){
		ASSERT(0);
		return;
	}
	
	if (!GetSentCancelTransfer())
	{
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__CancelTransfer", this);

		bool bDeletePacket;
		Packet* pCancelTransferPacket;
		if (packet)
		{
			pCancelTransferPacket = packet;
			bDeletePacket = false;
		}
		else
		{
			pCancelTransferPacket = new Packet(OP_CANCELTRANSFER, 0);
			bDeletePacket = true;
		}
		theStats.AddUpDataOverheadFileRequest(pCancelTransferPacket->size);
		socket->SendPacket(pCancelTransferPacket,bDeletePacket,true);
		SetSentCancelTransfer(1);
	}

	if (m_pPCDownSocket)
	{
		m_pPCDownSocket->Safe_Delete();
		m_pPCDownSocket = NULL;
		SetPeerCacheDownState(PCDS_NONE);
	}
}

void CUpDownClient::SetRequestFile(CPartFile* pReqFile)
{
	reqfile = pReqFile;
}

void CUpDownClient::ProcessAcceptUpload()
{
	m_fQueueRankPending = 1;
	//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
	/*
	if (reqfile && !reqfile->IsStopped() && (reqfile->GetStatus()==PS_READY || reqfile->GetStatus()==PS_EMPTY))
	*/
	if (reqfile && !reqfile->IsStopped() && (reqfile->GetStatus()==PS_READY || reqfile->GetStatus()==PS_EMPTY) && !reqfile->notSeenCompleteSource())
	//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
	{
		SetSentCancelTransfer(0);
		if (GetDownloadState() == DS_ONQUEUE)
		{
			// PC-TODO: If remote client does not answer the PeerCache query within a timeout, 
			// automatically fall back to ed2k download.
			if ( !SupportPeerCache() // client knows peercahce protocol
				||!thePrefs.IsPeerCacheDownloadEnabled() // user has enabled peercache downlaods
				|| !theApp.m_pPeerCache->IsCacheAvailable() // we have found our cache and its usable
				|| !theApp.m_pPeerCache->IsClientPCCompatible(m_nClientVersion, GetClientSoft()) // the client version is accepted by the cahce
				|| !SendPeerCacheFileRequest()) // request made
			{
				StartDownload();
			}
		}
	}
	else
	{
		SendCancelTransfer();
		SetDownloadState((reqfile==NULL || reqfile->IsStopped()) ? DS_NONE : DS_ONQUEUE);
	}
}

void CUpDownClient::ProcessEdonkeyQueueRank(char* packet, UINT size)
{
	CSafeMemFile data((BYTE*)packet, size);
	uint32 rank = data.ReadUInt32();
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		Debug(_T("  %u (prev. %d)\n"), rank, IsRemoteQueueFull() ? (UINT)-1 : (UINT)GetRemoteQueueRank());
	SetRemoteQueueRank(rank);
	CheckQueueRankFlood();
}

void CUpDownClient::ProcessEmuleQueueRank(char* packet, UINT size)
{
	if (size != 12)
		throw GetResString(IDS_ERR_BADSIZE);
	uint16 rank = PeekUInt16(packet);
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		Debug(_T("  %u (prev. %d)\n"), rank, IsRemoteQueueFull() ? (UINT)-1 : (UINT)GetRemoteQueueRank());
	SetRemoteQueueFull(false);
	SetRemoteQueueRank(rank);
	CheckQueueRankFlood();
}

void CUpDownClient::CheckQueueRankFlood()
{
	if (m_fQueueRankPending == 0)
	{
		if (GetDownloadState() != DS_DOWNLOADING)
		{
			if (m_fUnaskQueueRankRecv < 3) // NOTE: Do not increase this nr. without increasing the bits for 'm_fUnaskQueueRankRecv'
				m_fUnaskQueueRankRecv++;
			if (m_fUnaskQueueRankRecv == 3)
			{
				if (theApp.clientlist->GetBadRequests(this) < 2)
					theApp.clientlist->TrackBadRequest(this, 1);
				if (theApp.clientlist->GetBadRequests(this) == 2){
					theApp.clientlist->TrackBadRequest(this, -2); // reset so the client will not be rebanned right after the ban is lifted
					Ban(_T("QR flood"));
				}
				throw CString(thePrefs.GetLogBannedClients() ? _T("QR flood") : _T(""));
			}
		}
	}
	else
	{
		m_fQueueRankPending = 0;
		m_fUnaskQueueRankRecv = 0;
	}
}

//// This next function is designed to balance the sources among
//// files of the same or greater priority.  What we're going to do
//// is:
////		A) See if this client is A4AF for any other files.
////		B) Make sure it isn't currently downloading.
////		C) Try to transfer it to another file that meets the criteria.
////			1. The new file MUST have higher priority OR...
////			2. Have the same priority and...
////			3. (new->sources < cur->sources) && ((cur->sources-new->sources)/new->sources > .1)
//// Returns false if source is not transferred, true if it is transferred.
//// This function works independent of the download manager.
//bool CUpDownClient::BalanceA4AFSources(bool byPriorityOnly)
//{
//	CPartFile* pSwap = NULL;
//	POSITION finalpos = NULL;
//
//	for (POSITION pos = m_OtherRequests_list.GetHeadPosition(); pos != NULL; m_OtherRequests_list.GetNext(pos))
//	{
//		CPartFile* cur_file = m_OtherRequests_list.GetAt(pos);
//		if (pSwap && pSwap->ForceAllA4AF())
//			continue;
//
//		if (cur_file != reqfile &&
//			!cur_file->ForceA4AFOff() &&
//			theApp.downloadqueue->IsPartFile(cur_file) &&
//			!cur_file->IsStopped() &&
//			(cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY) &&
//			(thePrefs.GetMaxSourcePerFileSoft() > cur_file->GetSourceCount() || !thePrefs.RespectMaxSources()))
//		{
//			if (cur_file->GetSrcStatisticsValue(DS_TOOMANYCONNS) > 5 || cur_file->GetSrcStatisticsValue(DS_TOOMANYCONNS) == cur_file->GetSourceCount())
//				continue;
//			//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
//			if (cur_file->notSeenCompleteSource())
//				continue;
//			//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
//			if (cur_file->ForceAllA4AF()) {
//				pSwap = cur_file;
//				finalpos = pos;
//				continue;
//			}
//			else if (!pSwap){
//				pSwap = cur_file;
//				finalpos = pos;
//			}
//			else if (pSwap->GetDownPriority() < cur_file->GetDownPriority()){
//				pSwap = cur_file;
//				finalpos = pos;
//			}
//			else if (!byPriorityOnly && pSwap->GetDownPriority() == cur_file->GetDownPriority() && pSwap->GetAvailableSrcCount() > cur_file->GetAvailableSrcCount()){
//				pSwap = cur_file;
//				finalpos = pos;
//			}
//		}
//	}
//
//	if (pSwap) {
//		// So we have a potential swap, but we still need to check to make sure that we
//		// wouldn't be better served to just leave this source where it is. (reqfile)
//		if (!pSwap->ForceAllA4AF())
//		{
//			if (pSwap->GetDownPriority() < reqfile->GetDownPriority())
//				return false;
//			else if (pSwap->GetDownPriority() == reqfile->GetDownPriority()) {
//				if (byPriorityOnly)
//					return false; // This option only uses the priority as a factor.
//				if (pSwap->GetSourceCount() >= reqfile->GetSourceCount())
//					return false;
//				// If the difference in source counts is less than 10%, leave this source right where it is.
//				// This is a simple way to avoid constant swapping, because the source counts will never be precisely
//				// the same for each file.  It works because at this point reqfile always has more sources than pSwap.
//				if ( ( ((float)pSwap->GetAvailableSrcCount() / reqfile->GetAvailableSrcCount()) ) > .9 )
//					return false;
//			}
//		}
//
//		if(DoSwap(pSwap, false, 0)){
//			m_OtherRequests_list.RemoveAt(finalpos);
//			return true;
//		}
//	}
//	
//	return false;
//}
//
//// This function is designed to give as many sources as possible
//// to the file with the lowest resume order in the category.  It
//// is yet another methodology for A4AF behavior, and designed to
//// complete the next file in a series as quickly as possible.
//// NOTE: This code requires my download manager to be installed!
//bool CUpDownClient::StackA4AFSources()
//{
//	m_iLastSwapAttempt = GetTickCount();
//
//	if (GetDownloadState() == DS_DOWNLOADING || m_OtherRequests_list.IsEmpty() || reqfile == theApp.downloadqueue->forcea4af_file || reqfile->ForceAllA4AF())
//		return false;
//
//	// Don't swap in first three minutes of run time or within three minutes of the last swap.
//	if ((GetTickCount()-theApp.stat_starttime) <= 180000 || (GetTickCount()-m_iLastActualSwap) <= 180000)
//		return false;
//
//	uint8 iCategory = reqfile->GetCategory();
//
//	CPartFile* pSwap = NULL;
//	POSITION finalpos = NULL;
//
//	for (POSITION pos = m_OtherRequests_list.GetHeadPosition(); pos != NULL; m_OtherRequests_list.GetNext(pos)) {
//		CPartFile* cur_file = m_OtherRequests_list.GetAt(pos);
//		if (pSwap && pSwap->ForceAllA4AF())
//			continue;
//
//		if ( cur_file->GetCategory() == iCategory &&
//			cur_file != reqfile &&
//			!cur_file->ForceA4AFOff() &&
//			theApp.downloadqueue->IsPartFile(cur_file) &&
//			!cur_file->IsStopped() &&
//			(cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY) &&
//			(thePrefs.GetMaxSourcePerFileSoft() > cur_file->GetSourceCount() || !thePrefs.RespectMaxSources()))
//		{
//			if (cur_file->GetSrcStatisticsValue(DS_TOOMANYCONNS) > 5 || cur_file->GetSrcStatisticsValue(DS_TOOMANYCONNS) == cur_file->GetSourceCount())
//				return false;
//			//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
//			if (cur_file->notSeenCompleteSource())
//				continue;
//			//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
//			if (cur_file->ForceAllA4AF()) {
//				pSwap = cur_file;
//				finalpos = pos;
//				continue;
//			}
//			//Morph Start - added by AndCycle - the NNP shouldn't need to be checked, currently there is issue in maintain OtherRequest
//			else if (m_OtherNoNeeded_list.Find(cur_file)){
//				continue;
//			}
//			//Morph End - added by AndCycle
//			else if (!pSwap){
//                pSwap = cur_file;
//				finalpos = pos;
//			}
//			else if (pSwap->GetCatResumeOrder() > cur_file->GetCatResumeOrder()){
//				pSwap = cur_file;
//				finalpos = pos;
//			}
//			else if (pSwap->GetCatResumeOrder() == cur_file->GetCatResumeOrder() && pSwap->GetAvailableSrcCount() > cur_file->GetAvailableSrcCount()){
//				pSwap = cur_file;
//				finalpos = pos;
//			}
//		}
//	}
//
//	if (!pSwap)
//		return BalanceA4AFSources(true); // If we were unable to find a stackable download for this source, we can try a priority-only balance.
//	else
//	{
//		if (!pSwap->ForceAllA4AF())
//		{
//			if (pSwap->GetCatResumeOrder() > reqfile->GetCatResumeOrder())
//				return false;
//			else if (pSwap->GetCatResumeOrder() == reqfile->GetCatResumeOrder())
//			{
//				if (pSwap->GetAvailableSrcCount() >= reqfile->GetAvailableSrcCount())
//					return false;
//				// If the difference in source counts is less than 10%, leave this source right where it is.
//				if ( ( ((float)pSwap->GetAvailableSrcCount() / reqfile->GetAvailableSrcCount()) ) > .9 )
//					return false;
//			}
//		}
//		
//		if(DoSwap(pSwap, false, 1)){
//			m_OtherRequests_list.RemoveAt(finalpos);
//			return true;
//		}
//	}
//	
//	return false;
//}
