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

#include "StdAfx.h"
#include <zlib/zlib.h>
#include "updownclient.h"
#include "partfile.h"
#include "opcodes.h"
#include "packets.h"
#include "emule.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//	members of CUpDownClient
//	which are mainly used for downloading functions 
CBarShader CUpDownClient::s_StatusBar(16);
void CUpDownClient::DrawStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat){ 
	COLORREF crBoth; 
	COLORREF crNeither; 
	COLORREF crClientOnly; 
	COLORREF crPending;
	COLORREF crNextPending;
	//MORPH - Added by IceCream--- xrmb:seeTheNeed ---
	COLORREF crMeOnly; 
	//--- :xrmb ---

	if(bFlat) { 
		crBoth = RGB(0, 0, 0);
		crNeither = RGB(224, 224, 224);
		crClientOnly = RGB(0, 100, 255);
		crPending = RGB(0,150,0);
		crNextPending = RGB(255,208,0);
		//MORPH - Added by IceCream--- xrmb:seeTheNeed ---
		crMeOnly = RGB(112,112,112);
		//--- :xrmb ---
	} else { 
		crBoth = RGB(104, 104, 104);
		crNeither = RGB(240, 240, 240);
		crClientOnly = RGB(0, 100, 255);
		crPending = RGB(0, 150, 0);
		crNextPending = RGB(255,208,0);
		//MORPH - Added by IceCream--- xrmb:seeTheNeed ---
		crMeOnly = RGB(172,172,172);
		//--- :xrmb ---
	} 

	ASSERT(reqfile);
	s_StatusBar.SetFileSize(reqfile->GetFileSize()); 
	s_StatusBar.SetHeight(rect->bottom - rect->top); 
	s_StatusBar.SetWidth(rect->right - rect->left); 
	s_StatusBar.Fill(crNeither); 

	uint32 uEnd; 

	// Barry - was only showing one part from client, even when reserved bits from 2 parts
	CString gettingParts;
	ShowDownloadingParts(&gettingParts);

	if (!onlygreyrect && reqfile && m_abyPartStatus) { 
		for (uint32 i = 0;i < m_nPartCount;i++){
			if (m_abyPartStatus[i]){ 
				
				//unneeded check: end value is already checked in Draw(...) function //wistily
				/*
				if (PARTSIZE*(i+1) > reqfile->GetFileSize())
				uEnd = reqfile->GetFileSize();
				else
				uEnd = PARTSIZE*(i+1);
				*/
				uEnd = PARTSIZE*(i+1);
				

				if (reqfile->IsComplete(PARTSIZE*i,PARTSIZE*(i+1)-1)) 
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crBoth);
				else if (m_nDownloadState == DS_DOWNLOADING && m_nLastBlockOffset < uEnd &&
				         m_nLastBlockOffset >= PARTSIZE*i)
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crPending);
				else if (gettingParts.GetAt((uint16)i) == 'Y') //Sony: cast to (uint16) to fix VC7.1 2GB+ file error
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crNextPending);
				else
					s_StatusBar.FillRange(PARTSIZE*i, uEnd, crClientOnly);
			} 
			//MORPH - Added by IceCream --- xrmb:seeTheNeed ---
			else if (reqfile->IsComplete(PARTSIZE*i,PARTSIZE*(i+1)-1)){ 
				if (PARTSIZE*(i+1) > reqfile->GetFileSize()) 
					uEnd = reqfile->GetFileSize(); 
				else 
					uEnd = PARTSIZE*(i+1); 

				s_StatusBar.FillRange(PARTSIZE*i, uEnd, crMeOnly);
			}
			//--- :xrmb ---
		} 
	} 
	s_StatusBar.Draw(dc, rect->left, rect->top, bFlat); 
} 


bool CUpDownClient::Compare(CUpDownClient* tocomp, bool bIgnoreUserhash){
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

void CUpDownClient::AskForDownload(){
	if (theApp.listensocket->TooManySockets() && !(socket && socket->IsConnected()) ){
		if (GetDownloadState() != DS_TOOMANYCONNS)
			SetDownloadState(DS_TOOMANYCONNS);
		return;
	}

	m_bUDPPending = false;
	m_dwLastAskedTime = ::GetTickCount();
	SetDownloadState(DS_CONNECTING);
	TryToConnect();

}

bool CUpDownClient::IsSourceRequestAllowed() {
	DWORD dwTickCount = ::GetTickCount() + CONNECTION_LATENCY;
	unsigned int nTimePassedClient = dwTickCount - GetLastSrcAnswerTime();
	unsigned int nTimePassedFile   = dwTickCount - reqfile->GetLastAnsweredTime();
	bool bNeverAskedBefore = GetLastAskedForSources() == 0;

	UINT uSources = reqfile->GetSourceCount();
	return (
	         //if client has the correct extended protocol
	         ExtProtocolAvailable() && m_bySourceExchangeVer > 1 &&
	         //AND if we need more sources
	         theApp.glob_prefs->GetMaxSourcePerFileSoft() > uSources &&
	         //AND if...
	         (
	           //source is not complete and file is rare, allow once every 10 minutes
	           ( !m_bCompleteSource &&
	             ( uSources - reqfile->GetValidSourcesCount() <= RARE_FILE / 4 ||
			       uSources <= RARE_FILE * 2
			     ) &&
	             (bNeverAskedBefore || nTimePassedClient > SOURCECLIENTREASK)
	           ) ||
	           // otherwise, allow every 90 minutes, but only if we haven't
	           //   asked someone else in last 10 minutes
			   ( (bNeverAskedBefore || nTimePassedClient > (unsigned)(SOURCECLIENTREASK * reqfile->GetCommonFilePenalty())) &&
		         (nTimePassedFile > SOURCECLIENTREASK)
	           )
	         )
	       );
}

void CUpDownClient::SendFileRequest(){
	ASSERT(reqfile != NULL);
	if(!reqfile)
		return;
	AddAskedCountDown();

	CSafeMemFile dataFileReq(16+16);
	dataFileReq.Write(reqfile->GetFileHash(),16);
	if( GetExtendedRequestsVersion() > 0 ){
		reqfile->WritePartStatus(&dataFileReq);
	}
	if( GetExtendedRequestsVersion() > 1 ){
		reqfile->WriteCompleteSourcesCount(&dataFileReq);		// #zegzav:completesrc (add)
	}
	Packet* packet = new Packet(&dataFileReq);
	packet->opcode=OP_FILEREQUEST;
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet, true);

	// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
	// if the remote client answers the OP_FILEREQUEST with OP_FILEREQANSWER the file is shared by the remote client. if we
	// know that the file is shared, we know also that the file is complete and don't need to request the file status.
	if (reqfile->GetPartCount() > 1){
	CSafeMemFile dataSetReqFileID(16);
	dataSetReqFileID.Write(reqfile->GetFileHash(),16);
	packet = new Packet(&dataSetReqFileID);
	packet->opcode = OP_SETREQFILEID;
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet, true);
	}

	if( IsEmuleClient() ){
		SetRemoteQueueFull( true );
		SetRemoteQueueRank(0);
	}	
	if(IsSourceRequestAllowed()) {
		reqfile->SetLastAnsweredTimeTimeout();
		Packet* packet = new Packet(OP_REQUESTSOURCES,16,OP_EMULEPROT);
		md4cpy(packet->pBuffer,reqfile->GetFileHash());
		theApp.uploadqueue->AddUpDataOverheadSourceExchange(packet->size);
		socket->SendPacket(packet,true,true);
		SetLastAskedForSources();
		if ( theApp.glob_prefs->GetDebugSourceExchange() )
			AddDebugLogLine( false, "Send:Source Request User(%s) File(%s)", GetUserName(), reqfile->GetFileName() );
	}
}

void CUpDownClient::SendStartupLoadReq(){
	if (socket==NULL || reqfile==NULL){
		ASSERT(0);
		return;
	}
	SetDownloadState(DS_ONQUEUE);
	CSafeMemFile dataStartupLoadReq(16);
	dataStartupLoadReq.Write(reqfile->GetFileHash(),16);
	Packet* packet = new Packet(&dataStartupLoadReq);
	packet->opcode = OP_STARTUPLOADREQ;
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet, true, true);
}

void CUpDownClient::ProcessFileInfo(char* packet,uint32 size){
	CSafeMemFile data((BYTE*)packet,size);
	uchar cfilehash[16];
	data.Read(cfilehash,16);
	uint16 namelength;
	data.Read(&namelength,2);
	data.Read(m_strClientFilename.GetBuffer(namelength),namelength);
	m_strClientFilename.ReleaseBuffer(namelength);

	if (reqfile==NULL)
		throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileInfo; reqfile==NULL)");
	if (md4cmp(cfilehash,reqfile->GetFileHash()))
		throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileInfo; reqfile!=cfilehash)");

	// 26-Jul-2003: removed requesting the file status for files <= PARTSIZE for better compatibility with ed2k protocol (eDonkeyHybrid).
	// if the remote client answers the OP_FILEREQUEST with OP_FILEREQANSWER the file is shared by the remote client. if we
	// know that the file is shared, we know also that the file is complete and don't need to request the file status.
	if (reqfile->GetPartCount() == 1){
		if (m_abyPartStatus){
			delete[] m_abyPartStatus;
			m_abyPartStatus = NULL;
		}
		m_nPartCount = reqfile->GetPartCount();
		m_abyPartStatus = new uint8[m_nPartCount];
		//MORPH START - Added by SiRoB, Hot Fix for m_PartStatus_list
		m_PartStatus_list[reqfile] = m_abyPartStatus;
		//MORPH END   - Added by SiRoB, Hot Fix for m_PartStatus_list
		memset(m_abyPartStatus,1,m_nPartCount);
		m_bCompleteSource = true;

		UpdateDisplayedInfo();
		reqfile->UpdateAvailablePartsCount();

		// even if the file is <= PARTSIZE, we _may_ need the hashset for that file (if the file size == PARTSIZE)
		if (reqfile->hashsetneeded){
			Packet* packet = new Packet(OP_HASHSETREQUEST,16);
			md4cpy(packet->pBuffer,reqfile->GetFileHash());
			theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
			socket->SendPacket(packet,true,true);
			SetDownloadState(DS_REQHASHSET);
			m_fHashsetRequesting = 1;
			reqfile->hashsetneeded = false;
		}
		else{
			SendStartupLoadReq();
		}
		reqfile->NewSrcPartsInfo();
	}
}

void CUpDownClient::ProcessFileStatus(char* packet,uint32 size){
	CSafeMemFile data((BYTE*)packet,size);
	uchar cfilehash[16];
	data.Read(cfilehash,16);
	if ( (!reqfile) || md4cmp(cfilehash,reqfile->GetFileHash())){
		if (reqfile==NULL)
			throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileStatus; reqfile==NULL)");
		throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessFileStatus; reqfile!=cfilehash)");
	}
	uint16 nED2KPartCount;
	data.Read(&nED2KPartCount,2);
	if (m_abyPartStatus){
		delete[] m_abyPartStatus;
		m_abyPartStatus = NULL;
		// khaos::kmod+ 
		m_PartStatus_list[reqfile] = NULL;
		// khaos::kmod-
	}
	bool bPartsNeeded = false;
	if (!nED2KPartCount){
		m_nPartCount = reqfile->GetPartCount();
		m_abyPartStatus = new uint8[m_nPartCount];
		memset(m_abyPartStatus,1,m_nPartCount);
		bPartsNeeded = true;
		m_bCompleteSource = true;
	}
	else{
		if (reqfile->GetED2KPartCount() != nED2KPartCount){
			m_nPartCount = 0;
			throw GetResString(IDS_ERR_WRONGPARTNUMBER);
		}
		m_nPartCount = reqfile->GetPartCount();

		m_bCompleteSource = false;
		m_abyPartStatus = new uint8[m_nPartCount];
		uint16 done = 0;
		while (done != m_nPartCount){
			uint8 toread;
			data.Read(&toread,1);
			for (sint32 i = 0;i != 8;i++){
				m_abyPartStatus[done] = ((toread>>i)&1)? 1:0; 	
				if (m_abyPartStatus[done] && !reqfile->IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1))
					bPartsNeeded = true;
				done++;
				if (done == m_nPartCount)
					break;
			}
		}
	}

	//theApp.emuledlg->transferwnd.downloadlistctrl.UpdateItem(this);
	UpdateDisplayedInfo();
	reqfile->UpdateAvailablePartsCount();
    
	// khaos::kmod+ Save part statuses
	m_PartStatus_list[reqfile] = m_abyPartStatus;
	// khaos::kmod-

	if (!bPartsNeeded)
		SetDownloadState(DS_NONEEDEDPARTS);
	else if (reqfile->hashsetneeded){
		Packet* packet = new Packet(OP_HASHSETREQUEST,16);
		md4cpy(packet->pBuffer,reqfile->GetFileHash());
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
		socket->SendPacket(packet, true, true);
		SetDownloadState(DS_REQHASHSET);
		m_fHashsetRequesting = 1;
		reqfile->hashsetneeded = false;
	}
	else{
		SendStartupLoadReq();
	}
	reqfile->NewSrcPartsInfo();
}

bool CUpDownClient::AddRequestForAnotherFile(CPartFile* file){
	for (POSITION pos = m_OtherNoNeeded_list.GetHeadPosition();pos != 0;m_OtherNoNeeded_list.GetNext(pos)){
		if (m_OtherNoNeeded_list.GetAt(pos) == file)
			return false;
	}
	for (POSITION pos = m_OtherRequests_list.GetHeadPosition();pos != 0;m_OtherRequests_list.GetNext(pos)){
		if (m_OtherRequests_list.GetAt(pos) == file)
			return false;
	}
	m_OtherRequests_list.AddTail(file);
	//MORPH START - Added by SiRoB, A4AF counter
	file->IncreaseSourceCountA4AF(); // SiRoB A4AF
	//MORPH END   - Added by SiRoB, A4AF counter
	return true;
}

void CUpDownClient::SetDownloadState(EDownloadState nNewState){
	if (m_nDownloadState != nNewState){
		if(nNewState == DS_DOWNLOADING){
			reqfile->AddDownloadingSource(this);
		}
		else if(m_nDownloadState == DS_DOWNLOADING){
			reqfile->RemoveDownloadingSource(this);
		}

		if (m_nDownloadState == DS_DOWNLOADING ){

			// -khaos--+++> Extended Statistics (Successful/Failed Download Sessions)
			if ( m_bTransferredDownMini && nNewState != DS_ERROR )
				theApp.glob_prefs->Add2DownSuccessfulSessions(); // Increment our counters for successful sessions (Cumulative AND Session)
			else
				theApp.glob_prefs->Add2DownFailedSessions(); // Increment our counters failed sessions (Cumulative AND Session)
			//wistily start
			uint32 tempDownTimeDifference= GetDownTimeDifference();
			Add2DownTotalTime(tempDownTimeDifference);
			if (m_nDownTotalTime>1000) //Added by SiRoB, to avoid div by zero
				m_nAvDownDatarate = m_nTransferedDown/(m_nDownTotalTime/1000);
			theApp.glob_prefs->Add2DownSAvgTime(tempDownTimeDifference/1000);
			/*theApp.glob_prefs->Add2DownSAvgTime(GetDownTimeDifference()/1000);*/
			//wistily stop

			// <-----khaos-

			m_nDownloadState = nNewState;
			for (POSITION pos = m_DownloadBlocks_list.GetHeadPosition();pos != 0;m_DownloadBlocks_list.GetNext(pos)){
				Requested_Block_Struct* cur_block = m_DownloadBlocks_list.GetAt(pos);
				reqfile->RemoveBlockFromList(cur_block->StartOffset,cur_block->EndOffset);
				delete m_DownloadBlocks_list.GetAt(pos);
			}
			m_DownloadBlocks_list.RemoveAll();

			for (POSITION pos = m_PendingBlocks_list.GetHeadPosition();pos != 0;m_PendingBlocks_list.GetNext(pos))
			{
				Pending_Block_Struct *pending = m_PendingBlocks_list.GetAt(pos);
				if (reqfile)
					reqfile->RemoveBlockFromList(pending->block->StartOffset, pending->block->EndOffset);
				
				delete pending->block;
				// Not always allocated
				if (pending->zStream){
					inflateEnd(pending->zStream);
					delete pending->zStream;
				}
				delete pending;
			}
			m_PendingBlocks_list.RemoveAll();
			m_nDownDatarate = 0;
			if (nNewState == DS_NONE){
				//khaos::kmod+ m_PartStatus_list
				/*if (m_abyPartStatus)
					delete[] m_abyPartStatus;
				m_abyPartStatus = NULL;
				m_nPartCount = 0;*/
				// khaos::kmod- m_PartStatus_list
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
	if ( (!reqfile) || md4cmp(packet,reqfile->GetFileHash()))
		throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessHashSet)");	
	CSafeMemFile data((BYTE*)packet,size);
	if (reqfile->LoadHashsetFromFile(&data,true)){
		m_fHashsetRequesting = 0;
	}
	else{
		reqfile->hashsetneeded = true;
		throw GetResString(IDS_ERR_BADHASHSET);
	}
	SendStartupLoadReq();
}

void CUpDownClient::SendBlockRequests(){
	m_dwLastBlockReceived = ::GetTickCount();
	if (!reqfile)
		return;
	if (m_DownloadBlocks_list.IsEmpty())
	{
		// Barry - instead of getting 3, just get how many is needed
		uint16 count = 3 - m_PendingBlocks_list.GetCount();
		Requested_Block_Struct** toadd = new Requested_Block_Struct*[count];
		if (reqfile->GetNextRequestedBlock(this,toadd,&count)){
			for (int i = 0; i < count; i++)
				m_DownloadBlocks_list.AddTail(toadd[i]);			
		}
		delete[] toadd;
	}

	// Barry - Why are unfinished blocks requested again, not just new ones?

	while (m_PendingBlocks_list.GetCount() < 3 && !m_DownloadBlocks_list.IsEmpty()){
		Pending_Block_Struct* pblock = new Pending_Block_Struct;
		pblock->block = m_DownloadBlocks_list.RemoveHead();
		pblock->zStream = NULL;
		pblock->totalUnzipped = 0;
		m_PendingBlocks_list.AddTail(pblock);
	}
	if (m_PendingBlocks_list.IsEmpty()){
		Packet* packet = new Packet(OP_CANCELTRANSFER,0);
		theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
		socket->SendPacket(packet,true,true);
		SetDownloadState(DS_NONEEDEDPARTS);
		return;
	}
	const int iPacketSize = 16+(3*4)+(3*4); // 40
	Packet* packet = new Packet(OP_REQUESTPARTS,iPacketSize);
	CSafeMemFile data((BYTE*)packet->pBuffer,iPacketSize);
	data.Write(reqfile->GetFileHash(),16);
	POSITION pos = m_PendingBlocks_list.GetHeadPosition();
	uint32 null = 0;
	Requested_Block_Struct* block;
	for (uint32 i = 0; i != 3; i++){
		if (pos){
			block = m_PendingBlocks_list.GetAt(pos)->block;
			m_PendingBlocks_list.GetNext(pos);
			data.Write(&block->StartOffset,4);
		}
		else
			data.Write(&null,4);
	}
	pos = m_PendingBlocks_list.GetHeadPosition();
	for (uint32 i = 0; i != 3; i++){
		if (pos){
			block = m_PendingBlocks_list.GetAt(pos)->block;
			m_PendingBlocks_list.GetNext(pos);
			uint32 endpos = block->EndOffset+1;
			data.Write(&endpos,4);
		}
		else
			data.Write(&null,4);
	}
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
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
  try {
	// Ignore if no data required
	if (!(GetDownloadState() == DS_DOWNLOADING || GetDownloadState() == DS_NONEEDEDPARTS))
		return;

	const int HEADER_SIZE = 24;

	// Update stats
	m_dwLastBlockReceived = ::GetTickCount();


	// Read data from packet
	CSafeMemFile data((BYTE*)packet, size);
	uchar fileID[16];
	data.Read(fileID, 16);

	// Check that this data is for the correct file
	if ( (!reqfile) || md4cmp(packet, reqfile->GetFileHash()))
	{
		throw GetResString(IDS_ERR_WRONGFILEID) + _T(" (ProcessBlockPacket)");
	}

	// Find the start & end positions, and size of this chunk of data
	uint32 nStartPos;
	uint32 nEndPos;
	uint32 nBlockSize = 0;
	data.Read(&nStartPos, 4);
	if (packed)
	{
		data.Read(&nBlockSize, 4);
		nEndPos = nStartPos + (size - HEADER_SIZE);
		usedcompressiondown = true;
	}
	else
		data.Read(&nEndPos,4);

	// Check that packet size matches the declared data size + header size (24)
	if ( size != ((nEndPos - nStartPos) + HEADER_SIZE))
		throw GetResString(IDS_ERR_BADDATABLOCK) + _T(" (ProcessBlockPacket)");

	// -khaos--+++>
	// Extended statistics information based on which client and remote port sent this data.
	// The new function adds the bytes to the grand total as well as the given client/port.
	// bFromPF is not relevant to downloaded data.  It is purely an uploads statistic.

	//MORPH START - Added by Yun.SF3, ZZ Upload System
	theApp.glob_prefs->Add2SessionTransferData(GetClientSoft(), GetUserPort(), false, false, size - HEADER_SIZE, false);
	//MORPH END - Added by Yun.SF3, ZZ Upload System
	// <-----khaos-

	m_nDownDataRateMS += size - HEADER_SIZE;
	credits->AddDownloaded((size - HEADER_SIZE)/reqfile->m_nCompleteSourcesCount, GetIP());//Give more credits to rare files uploaders [Yun.SF3]
	// Move end back one, should be inclusive
	nEndPos--;

	// Loop through to find the reserved block that this is within
	Pending_Block_Struct *cur_block;
	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != NULL; m_PendingBlocks_list.GetNext(pos))
	{
		cur_block = m_PendingBlocks_list.GetAt(pos);
		if ((cur_block->block->StartOffset <= nStartPos) && (cur_block->block->EndOffset >= nStartPos))
		{
			// Found reserved block

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
				// Create space to store unzipped data, the size is only an initial guess, will be resized in unzip() if not big enough
				uint32 lenUnzipped = (size * 2); 
				// Don't get too big
				if (lenUnzipped > (EMBLOCKSIZE + 300))
					lenUnzipped = (EMBLOCKSIZE + 300);
				BYTE *unzipped = new BYTE[lenUnzipped];

				// Try to unzip the packet
				int result = unzip(cur_block, (BYTE *)(packet + HEADER_SIZE), (size - HEADER_SIZE), &unzipped, &lenUnzipped);
				if (result == Z_OK)
				{
					//MORPH START - Added by IceCream, Defeat 0-filled Part Senders from Maella
					if((theApp.glob_prefs->GetEnableZeroFilledTest() == true) && (reqfile->IsCDImage() == false) && (reqfile->IsArchive() == false) && (reqfile->IsDocument() == false)){
						// Check the compression factor
						if(lenUnzipped > 25*nBlockSize){

							// Format User hash
							CString userHash;
							for(int  i=0; i<16; i++){
								TCHAR buffer[33];
								_stprintf(buffer, _T("%02X"), GetUserHash()[i]);
								userHash += buffer;
							}

							// Log
							theApp.emuledlg->AddLogLine(true,  _T("Received suspicious block: file '%s', part %i, block %i, blocksize %i, comp. blocksize %i, comp. factor %0.1f)"), reqfile->GetFileName(), cur_block->block->StartOffset/PARTSIZE, cur_block->block->StartOffset/EMBLOCKSIZE, lenUnzipped, nBlockSize, lenUnzipped/nBlockSize);
							theApp.emuledlg->AddLogLine(false, _T("Username '%s' (IP %s:%i), hash %s"), m_pszUsername, GetFullIP(), GetUserPort(), userHash);

							// Ban => serious error (Attack?)
							if(lenUnzipped > 4*nBlockSize && reqfile->IsArchive() == true){
								theApp.ipfilter->AddBannedIPRange(GetIP(), GetIP(), 1, _T("Temporary"));
								SetDownloadState(DS_ERROR);
							}

							// Do Not Save
							lenWritten = 0;
							lenUnzipped = 0; // skip writting
							reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
							theApp.emuledlg->AddLogLine(false, _T("Block dropped"));
						}
					}
					//MORPH END   - Added by IceCream, Defeat 0-filled Part Senders from Maella

					// Write any unzipped data to disk
					if (lenUnzipped > 0)
					{
						// Use the current start and end positions for the uncompressed data
						nStartPos = cur_block->block->StartOffset + cur_block->totalUnzipped - lenUnzipped;
						nEndPos = cur_block->block->StartOffset + cur_block->totalUnzipped - 1;

						if( nEndPos > cur_block->block->EndOffset){
							AddDebugLogLine(false, GetResString(IDS_ERR_CORRUPTCOMPRPKG),reqfile->GetFileName(),666);
							reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
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
					AddDebugLogLine(false, GetResString(IDS_ERR_CORRUPTCOMPRPKG), reqfile->GetFileName(), result);
					reqfile->RemoveBlockFromList(cur_block->block->StartOffset, cur_block->block->EndOffset);
					//MORPH START - Added by IceCream, Defeat 0-filled Part Senders from Maella
					if(theApp.glob_prefs->GetEnableZeroFilledTest() == true) {
						CString userHash;
						for(int  i=0; i<16; i++){
							TCHAR buffer[33];
							_stprintf(buffer, _T("%02X"), GetUserHash()[i]);
							userHash += buffer;
						}
						// Ban => serious error (Attack?)
						theApp.emuledlg->AddLogLine(false, _T(GetResString(IDS_CORRUPTDATASENT)), m_pszUsername, GetFullIP(), GetUserPort(), userHash, GetClientVerString()); //MORPH - Modified by IceCream
						theApp.ipfilter->AddBannedIPRange(GetIP(), GetIP(), 1, _T("Temporary"));
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
					m_PendingBlocks_list.RemoveAt(pos);

					// Request next block
					SendBlockRequests();	
				}
			}

			// Stop looping and exit method
			return;
		}
	}
  } catch (...) {}
}

int CUpDownClient::unzip(Pending_Block_Struct *block, BYTE *zipped, uint32 lenZipped, BYTE **unzipped, uint32 *lenUnzipped, bool recursive)
{
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
		    if (err != Z_OK)
			    return err;
	    }
    
	    // Use whatever input is provided
	    zS->next_in  = zipped;
	    zS->avail_in = lenZipped;
    
	    // Only set the output if not being called recursively
	    if (!recursive)
	    {
		    zS->next_out = (*unzipped);
		    zS->avail_out = (*lenUnzipped);
	    }
    
	    // Try to unzip the data
	    err = inflate(zS, Z_SYNC_FLUSH);
    
	    // Is zip finished reading all currently available input and writing all generated output
	    if (err == Z_STREAM_END)
	    {
		    // Finish up
		    err = inflateEnd(zS);
		    if (err != Z_OK)
			    return err;
    
		    // Got a good result, set the size to the amount unzipped in this call (including all recursive calls)
		    (*lenUnzipped) = (zS->total_out - block->totalUnzipped);
		    block->totalUnzipped = zS->total_out;
	    }
	    else if ((err == Z_OK) && (zS->avail_out == 0))
	    {
		    // Output array was not big enough, call recursively until there is enough space
    
		    // What size should we try next
		    uint32 newLength = (*lenUnzipped) *= 2;
		    if (newLength == 0)
			    newLength = lenZipped * 2;
    
		    // Copy any data that was successfully unzipped to new array
		    BYTE *temp = new BYTE[newLength];
		    memcpy(temp, (*unzipped), (zS->total_out - block->totalUnzipped));
			delete [] (*unzipped);
		    (*unzipped) = temp;
		    (*lenUnzipped) = newLength;
    
		    // Position stream output to correct place in new array
		    zS->next_out = (*unzipped) + (zS->total_out - block->totalUnzipped);
		    zS->avail_out = (*lenUnzipped) - (zS->total_out - block->totalUnzipped);
    
		    // Try again
		    err = unzip(block, zS->next_in, zS->avail_in, unzipped, lenUnzipped, true);
	    }
	    else if ((err == Z_OK) && (zS->avail_in == 0))
	    {
		    // All available input has been processed, everything ok.
		    // Set the size to the amount unzipped in this call (including all recursive calls)
		    (*lenUnzipped) = (zS->total_out - block->totalUnzipped);
		    block->totalUnzipped = zS->total_out;
	    }
	    else
	    {
		    // Should not get here unless input data is corrupt
			AddDebugLogLine(false,"Unexpected zip error in file \"%s\"", reqfile ? reqfile->GetFileName() : NULL);
	    }
    
	    if (err != Z_OK)
		    (*lenUnzipped) = 0;
  	}
  	catch (...){
		err = Z_DATA_ERROR;
	}
  	return err;
}

uint32 CUpDownClient::CalculateDownloadRate(){

	// Patch By BadWolf - Accurate datarate Calculation
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	//MORPH START - Modified by SiRoB, Better Download rate calcul
	uint32 cur_tick = ::GetTickCount();
    if(m_nDownDataRateMS > 0) {
		TransferredData newitem = {m_nDownDataRateMS,cur_tick};
		m_AvarageDDR_list.AddTail(newitem);
		m_nSumForAvgDownDataRate += m_nDownDataRateMS;
		m_nDownDataRateMS = 0;
    }
	
	while (m_AvarageDDR_list.GetCount() > 0)
		if((cur_tick - m_AvarageDDR_list.GetHead().timestamp)> 20*1000){
			m_AvarageDDRlastRemovedHeadTimestamp = m_AvarageDDR_list.GetHead().timestamp;
			m_nSumForAvgDownDataRate -= m_AvarageDDR_list.RemoveHead().datalen;
		}else
			break;
	
	if (m_AvarageDDR_list.GetCount() > 0){
		DWORD dwDuration = m_AvarageDDR_list.GetTail().timestamp - ((m_AvarageDDRlastRemovedHeadTimestamp)?m_AvarageDDRlastRemovedHeadTimestamp:m_dwDownStartTime);
		m_nDownDatarate = (1000.0 * m_nSumForAvgDownDataRate) / ((dwDuration>1000)?dwDuration:1000);
	}else
		m_nDownDatarate = 0;
	
	UpdateDisplayedInfo();

	//MORPH END   - Modified by SiRoB, Better Download rate calcul
	//MORPH END   - Added by Yun.SF3, ZZ Upload System
    // no need for a special timeout here. Socket timeout will take care of it.
    // and we want to keep downloads going as long as possible.
    //if ((::GetTickCount() - m_dwLastBlockReceived) > DOWNLOADTIMEOUT){
    //    Packet* packet = new Packet(OP_CANCELTRANSFER,0);
    //    theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
    //    socket->SendPacket(packet,true,true);
    //    SetDownloadState(DS_ONQUEUE);
    //}
		
	return m_nDownDatarate;
}

uint16 CUpDownClient::GetAvailablePartCount(){
	uint16 result = 0;
	for (int i = 0;i < m_nPartCount;i++){
		if (IsPartAvailable(i))
			result++;
	}
	return result;
}

void CUpDownClient::SetRemoteQueueRank(uint16 nr){
	m_nRemoteQueueRank = nr;
	UpdateDisplayedInfo();
}

void CUpDownClient::UDPReaskACK(uint16 nNewQR){
	m_bUDPPending = false;
	SetRemoteQueueRank(nNewQR);
	m_dwLastAskedTime = ::GetTickCount();
}

void CUpDownClient::UDPReaskFNF(){
	m_bUDPPending = false;
	if (GetDownloadState()!=DS_DOWNLOADING){ // avoid premature deletion of 'this' client
		AddDebugLogLine(false,CString("UDP ANSWER FNF : %s"),GetUserName());
		theApp.downloadqueue->RemoveSource(this);
		if (!socket)
			//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
			Disconnected("UDP failed. Socket is NULL.");
			//MORPH START - Added by SiRoB, ZZ Upload system 20030818-1923
	}
	else
		AddDebugLogLine(false,CString("UDP ANSWER FNF : %s - did not remove client because of current download state"),GetUserName());
}

void CUpDownClient::UDPReaskForDownload(){
	ASSERT ( reqfile );
	if(!reqfile || m_bUDPPending)
		return;

	//the line "m_bUDPPending = true;" use to be here
	// deadlake PROXYSUPPORT
	const ProxySettings& proxy = theApp.glob_prefs->GetProxy();
	if(m_nUDPPort != 0 && theApp.glob_prefs->GetUDPPort() != 0 &&
	   !theApp.IsFirewalled() && !HasLowID() && !(socket && socket->IsConnected())&& (!proxy.UseProxy)) { // deadlake PROXYSUPPORT

		//don't use udp to ask for sources
		if(IsSourceRequestAllowed())
			return;
		m_bUDPPending = true;
		uint16 packetsize= 16;
		if (GetUDPVersion() >= 3)
			packetsize+= 2;
		Packet* response = new Packet(OP_REASKFILEPING,packetsize,OP_EMULEPROT);	// #zegzav:completesrc (modify)
		md4cpy(response->pBuffer,reqfile->GetFileHash());
		if (GetUDPVersion() >= 3)
		{
			uint16 completecount= reqfile->m_nCompleteSourcesCount;
			memcpy(response->pBuffer+16, &completecount, 2);
		}
		// #zegzav:completesrc_udp (add) - END
//MORPH END - Yun.SF3, Complete source feature v0.07a zegzav
		theApp.uploadqueue->AddUpDataOverheadFileRequest(response->size);
		theApp.clientudp->SendPacket(response,GetIP(),GetUDPPort());
	}
}

// Barry - Sets string to show parts downloading, eg NNNYNNNNYYNYN
void CUpDownClient::ShowDownloadingParts(CString *partsYN)
{
	Requested_Block_Struct *cur_block;
	int x;

	// Initialise to all N's
	char *n = new char[m_nPartCount+1];
	_strnset(n, 'N', m_nPartCount);
	n[m_nPartCount] = 0;
	partsYN->SetString(n, m_nPartCount);
	delete [] n;

	for (POSITION pos = m_PendingBlocks_list.GetHeadPosition(); pos != 0; m_PendingBlocks_list.GetNext(pos))
	{
		cur_block = m_PendingBlocks_list.GetAt(pos)->block;
		x = (cur_block->StartOffset / PARTSIZE);
		partsYN->SetAt(x, 'Y');
	}
}

void CUpDownClient::UpdateDisplayedInfo(boolean force) {
	DWORD curTick = ::GetTickCount();

//MORPH - Added by SiRoB, ZZ Upload System
	if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+m_random_update_wait) {
		theApp.emuledlg->transferwnd.downloadlistctrl.UpdateItem(this);
		theApp.emuledlg->transferwnd.clientlistctrl.RefreshClient(this);
		m_lastRefreshedDLDisplay = curTick;
	}
}

// khaos::kmod+ Smart A4AF Swapping

// This function has been modified to take file priority
// and source count into consideration when swapping a source
// to another file.
bool CUpDownClient::SwapToAnotherFile(bool bIgnoreNoNeeded, bool forceSmart){
	bool addReqFile = true;

	if (GetDownloadState() == DS_DOWNLOADING)
		return false;
	if (GetDownloadState() == DS_NONEEDEDPARTS)
	{
		POSITION myPos = m_OtherRequests_list.Find(reqfile);
		//MORPH START - Added by SiRoB, A4AF counter
		//if (myPos) m_OtherRequests_list.RemoveAt(myPos);
		if (myPos){
			m_OtherRequests_list.RemoveAt(myPos);
			reqfile->DecreaseSourceCountA4AF();
			theApp.emuledlg->transferwnd.downloadlistctrl.RemoveSource(this,reqfile);
		}
		//MORPH END - Added by SiRoB, A4AF counter
		addReqFile = false;
		}
	CPartFile* pSwap = NULL;
	if (!m_OtherRequests_list.IsEmpty())
	{
		for (POSITION pos = m_OtherRequests_list.GetHeadPosition();pos != 0;m_OtherRequests_list.GetNext(pos))
		{
			CPartFile* cur_file = m_OtherRequests_list.GetAt(pos);
			if (cur_file == reqfile)
			{
				addReqFile = false;
				continue;
	}
			if (pSwap && pSwap->ForceAllA4AF() && theApp.glob_prefs->UseSmartA4AFSwapping())
				continue;

			if (cur_file != reqfile &&
				theApp.downloadqueue->IsPartFile(cur_file) &&
				!cur_file->IsStopped() &&
				(cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY) &&
				(theApp.glob_prefs->GetMaxSourcePerFileSoft() > cur_file->GetSourceCount() || !theApp.glob_prefs->RespectMaxSources()))
			{
				if (!theApp.glob_prefs->UseSmartA4AFSwapping() && !forceSmart) {
					pSwap = cur_file;
					break;
				}
				if (theApp.glob_prefs->UseSmartA4AFSwapping() && cur_file->ForceAllA4AF())
				{
					pSwap = cur_file;
					continue;
			}
				else if (!pSwap)
					pSwap = cur_file;
				else if (pSwap->GetDownPriority() > cur_file->GetDownPriority())
					pSwap = cur_file;
				else if (pSwap->GetDownPriority() == cur_file->GetDownPriority() && pSwap->GetAvailableSrcCount() > cur_file->GetAvailableSrcCount())
					pSwap = cur_file;
			}
		}
		//MORPH START - Added by SiRoB, A4AF counter
		//if (pSwap) m_OtherRequests_list.RemoveAt(m_OtherRequests_list.Find(pSwap));
		if (pSwap){
			m_OtherRequests_list.RemoveAt(m_OtherRequests_list.Find(pSwap));
			pSwap->DecreaseSourceCountA4AF();
			theApp.emuledlg->transferwnd.downloadlistctrl.RemoveSource(this,pSwap);
		}
		//MORPH END   - Added by SiRoB, A4AF counter
	}
	if (!pSwap && bIgnoreNoNeeded)
	{
		for (POSITION pos = m_OtherNoNeeded_list.GetHeadPosition();pos != 0;m_OtherNoNeeded_list.GetNext(pos))
		{
			CPartFile* cur_file = m_OtherNoNeeded_list.GetAt(pos);

			if (cur_file != reqfile &&
				theApp.downloadqueue->IsPartFile(cur_file)
				&& !cur_file->IsStopped() &&
				(cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY) &&
				(theApp.glob_prefs->GetMaxSourcePerFileSoft() > cur_file->GetSourceCount() || !theApp.glob_prefs->RespectMaxSources()))
			{
				if (!theApp.glob_prefs->UseSmartA4AFSwapping() && !forceSmart) {
					pSwap = cur_file;
					break;
				}
				if (!pSwap)
					pSwap = cur_file;
				else if (pSwap->GetDownPriority() < cur_file->GetDownPriority())
					pSwap = cur_file;
				else if (pSwap->GetDownPriority() == cur_file->GetDownPriority() && pSwap->GetAvailableSrcCount() > cur_file->GetAvailableSrcCount())
					pSwap = cur_file;
			}
		}
		if (pSwap) m_OtherNoNeeded_list.RemoveAt(m_OtherNoNeeded_list.Find(pSwap));
	}
	if (pSwap)
	{
		if (GetDownloadState() == DS_NONEEDEDPARTS && !m_OtherNoNeeded_list.Find(reqfile))
			m_OtherNoNeeded_list.AddHead(reqfile);

		SwapThisSource(pSwap, addReqFile, 3);

		return true;
	}
	else
		return false;

}

// This next function is designed to balance the sources among
// files of the same or greater priority.  What we're going to do
// is:
//		A) See if this client is A4AF for any other files.
//		B) Make sure it isn't currently downloading.
//		C) Try to transfer it to another file that meets the criteria.
//			1. The new file MUST have higher priority OR...
//			2. Have the same priority and...
//			3. (new->sources < cur->sources) && ((cur->sources-new->sources)/new->sources > .1)
// Returns false if source is not transferred, true if it is transferred.
// This function works independent of the download manager.
bool CUpDownClient::BalanceA4AFSources(bool byPriorityOnly)
{
	m_iLastSwapAttempt = GetTickCount();

	// Not exactly a great idea to swap sources that are currently transferring to us, now is it?
	if (GetDownloadState() == DS_DOWNLOADING || m_OtherRequests_list.IsEmpty() || reqfile == theApp.downloadqueue->forcea4af_file || reqfile->ForceAllA4AF())
		return false;

	// Don't swap in first three minutes of run time or within three minutes of the last swap.
	if ((GetTickCount()-theApp.stat_starttime) <= 180000 || (GetTickCount()-m_iLastActualSwap) <= 180000)
		return false;

	CPartFile* pSwap = NULL;
	bool addReqFile = true;

	for (POSITION pos = m_OtherRequests_list.GetHeadPosition(); pos != NULL; m_OtherRequests_list.GetNext(pos))
	{
		CPartFile* cur_file = m_OtherRequests_list.GetAt(pos);
		if (cur_file == reqfile)
			addReqFile = false;
		if (pSwap && pSwap->ForceAllA4AF())
			continue;

		if (cur_file != reqfile &&
			!cur_file->ForceA4AFOff() &&
			theApp.downloadqueue->IsPartFile(cur_file) &&
			!cur_file->IsStopped() &&
			(cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY) &&
			(theApp.glob_prefs->GetMaxSourcePerFileSoft() > cur_file->GetSourceCount() || !theApp.glob_prefs->RespectMaxSources()))
		{
			if (cur_file->GetSrcStatisticsValue(DS_TOOMANYCONNS) > 5 || cur_file->GetSrcStatisticsValue(DS_TOOMANYCONNS) == cur_file->GetSourceCount())
				continue;
			if (cur_file->ForceAllA4AF()) {
				pSwap = cur_file;
				continue;
			}
			else if (!pSwap)
				pSwap = cur_file;
			else if (pSwap->GetDownPriority() < cur_file->GetDownPriority())
				pSwap = cur_file;
			else if (!byPriorityOnly && pSwap->GetDownPriority() == cur_file->GetDownPriority() && pSwap->GetAvailableSrcCount() > cur_file->GetAvailableSrcCount())
				pSwap = cur_file;
		}
	}

	if (pSwap) {
		// So we have a potential swap, but we still need to check to make sure that we
		// wouldn't be better served to just leave this source where it is. (reqfile)
		if (!pSwap->ForceAllA4AF())
		{
		if (pSwap->GetDownPriority() < reqfile->GetDownPriority())
			return false;
		else if (pSwap->GetDownPriority() == reqfile->GetDownPriority()) {
			if (byPriorityOnly)
				return false; // This option only uses the priority as a factor.
			if (pSwap->GetSourceCount() >= reqfile->GetSourceCount())
				return false;
			// If the difference in source counts is less than 10%, leave this source right where it is.
			// This is a simple way to avoid constant swapping, because the source counts will never be precisely
			// the same for each file.  It works because at this point reqfile always has more sources than pSwap.
			if ( ( (float)(pSwap->GetAvailableSrcCount() / reqfile->GetAvailableSrcCount()) ) > .9 )
				return false;
		}
		}

		SwapThisSource(pSwap, addReqFile, 0);

		return true;
	}
	
	return false;
}

// This function is designed to give as many sources as possible
// to the file with the lowest resume order in the category.  It
// is yet another methodology for A4AF behavior, and designed to
// complete the next file in a series as quickly as possible.
// NOTE: This code requires my download manager to be installed!
bool CUpDownClient::StackA4AFSources()
{
	m_iLastSwapAttempt = GetTickCount();

	if (GetDownloadState() == DS_DOWNLOADING || m_OtherRequests_list.IsEmpty() || reqfile == theApp.downloadqueue->forcea4af_file || reqfile->ForceAllA4AF())
		return false;

	// Don't swap in first three minutes of run time or within three minutes of the last swap.
	if ((GetTickCount()-theApp.stat_starttime) <= 180000 || (GetTickCount()-m_iLastActualSwap) <= 180000)
		return false;

	uint8 iCategory = reqfile->GetCategory();

	CPartFile* pSwap = NULL;
	bool addReqFile = true;

	for (POSITION pos = m_OtherRequests_list.GetHeadPosition(); pos != NULL; m_OtherRequests_list.GetNext(pos)) {
		CPartFile* cur_file = m_OtherRequests_list.GetAt(pos);
		if (cur_file == reqfile)
			addReqFile = false;
		if (pSwap && pSwap->ForceAllA4AF())
			continue;

		if ( cur_file->GetCategory() == iCategory &&
			cur_file != reqfile &&
			!cur_file->ForceA4AFOff() &&
			theApp.downloadqueue->IsPartFile(cur_file) &&
			!cur_file->IsStopped() &&
			(cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY) &&
			(theApp.glob_prefs->GetMaxSourcePerFileSoft() > cur_file->GetSourceCount() || !theApp.glob_prefs->RespectMaxSources()))
		{
			if (cur_file->GetSrcStatisticsValue(DS_TOOMANYCONNS) > 5 || cur_file->GetSrcStatisticsValue(DS_TOOMANYCONNS) == cur_file->GetSourceCount())
				return false;
			if (cur_file->ForceAllA4AF()) {
				pSwap = cur_file;
				continue;
			}
			else if (!pSwap)
				pSwap = cur_file;
			else if (pSwap->GetCatResumeOrder() > cur_file->GetCatResumeOrder())
				pSwap = cur_file;
			else if (pSwap->GetCatResumeOrder() == cur_file->GetCatResumeOrder() && pSwap->GetAvailableSrcCount() > cur_file->GetAvailableSrcCount())
				pSwap = cur_file;
		}
	}

	if (!pSwap)
		return BalanceA4AFSources(true); // If we were unable to find a stackable download for this source, we can try a priority-only balance.
	else
	{
		if (!pSwap->ForceAllA4AF())
		{
		if (pSwap->GetCatResumeOrder() > reqfile->GetCatResumeOrder())
			return false;
		else if (pSwap->GetCatResumeOrder() == reqfile->GetCatResumeOrder())
		{
			if (pSwap->GetAvailableSrcCount() >= reqfile->GetAvailableSrcCount())
				return false;
			// If the difference in source counts is less than 10%, leave this source right where it is.
			if ( ( (float)(pSwap->GetAvailableSrcCount() / reqfile->GetAvailableSrcCount()) ) > .9 )
				return false;
		}
		}
		
		SwapThisSource(pSwap, addReqFile, 1);

		return true;
	}
	
	return false;
}

bool CUpDownClient::SwapToForcedA4AF()
{
	m_iLastForceA4AFAttempt = GetTickCount();

	// Don't swap in first three minutes of run time or within three minutes of the last swap.
	if ((GetTickCount()-theApp.stat_starttime) <= 180000 || (GetTickCount()-m_iLastActualSwap) <= 180000)
		return false;

	CPartFile* pForcedA4AF = theApp.downloadqueue->forcea4af_file;

	if (pForcedA4AF == reqfile || pForcedA4AF == NULL || GetDownloadState() == DS_DOWNLOADING || m_OtherRequests_list.IsEmpty())
		return false;

	bool addReqFile = true;
	bool swapToFA4AF = false;

	for (POSITION pos = m_OtherRequests_list.GetHeadPosition(); pos != NULL; m_OtherRequests_list.GetNext(pos))
	{
		CPartFile* cur_file = m_OtherRequests_list.GetAt(pos);
		if (cur_file == reqfile)
			addReqFile = false;
		else if (theApp.downloadqueue->IsPartFile(cur_file) &&
				!cur_file->IsStopped() &&
				(cur_file->GetStatus(false) == PS_READY || cur_file->GetStatus(false) == PS_EMPTY) &&
				(theApp.glob_prefs->GetMaxSourcePerFileSoft() > cur_file->GetSourceCount() || !theApp.glob_prefs->RespectMaxSources()))
		{		
			if (cur_file == pForcedA4AF)
				swapToFA4AF = true;
		}
	}

	if (swapToFA4AF)
	{
		SwapThisSource(pForcedA4AF, addReqFile, 2);
		return true;
	}
	return false;
}


void CUpDownClient::SwapThisSource(CPartFile* pNewFile, bool bAddReqFile, int iDebugMode)
{
	m_iLastActualSwap = GetTickCount();

	if (theApp.glob_prefs->ShowA4AFDebugOutput()) theApp.emuledlg->AddDebugLogLine(false, "%s: Just swapped '%s' from '%s' to '%s'. (%s)", iDebugMode==2?"Smart A4AF Swapping":"Advanced A4AF Handling", GetUserName(), reqfile->GetFileName(), pNewFile->GetFileName(), iDebugMode==0?"Balancing":iDebugMode==1?"Stacking":iDebugMode==2?"Forced":"N/A");
	
	//MORPH START  - Added by SiRoB, A4AF counter
	//if (bAddReqFile) m_OtherRequests_list.AddHead(reqfile);
	if (bAddReqFile) {
		m_OtherRequests_list.AddHead(reqfile);
		reqfile->IncreaseSourceCountA4AF();
		theApp.emuledlg->transferwnd.downloadlistctrl.AddSource(reqfile,this,true);
	}else
		theApp.emuledlg->transferwnd.downloadlistctrl.RemoveSource(this,reqfile);
	
	POSITION pos = m_OtherRequests_list.Find(pNewFile);
	if (pos)
		//m_OtherRequests_list.RemoveAt(pos);
		{m_OtherRequests_list.RemoveAt(pos);pNewFile->DecreaseSourceCountA4AF();}
	//MORPH END  - Added by SiRoB, A4AF counter
	pos = m_OtherNoNeeded_list.Find(pNewFile);
	if (pos)
		m_OtherNoNeeded_list.RemoveAt(pos);

	theApp.downloadqueue->RemoveSource(this, false, false);

	m_iRate=0;
	m_strComment="";
    uint8* thisStatus;
	if (m_PartStatus_list.Lookup(pNewFile, thisStatus))
	{
		m_abyPartStatus = thisStatus;
		m_nPartCount = pNewFile->GetPartCount();
	}
	else
	{
		m_nPartCount = 0;
		m_abyPartStatus = NULL;
	}
	
	pNewFile->srclists[sourcesslot].AddTail(this);
	theApp.emuledlg->transferwnd.downloadlistctrl.AddSource(pNewFile, this, false);
	theApp.downloadqueue->UpdateDisplayedInfo();
	pNewFile->UpdateAvailablePartsCount();

	reqfile = pNewFile;
	
	// If we have no info for this source then we should reask for it sometime
	// in the next two minutes...
	if (m_abyPartStatus == 0)
	{
		DWORD dwReAskTime = GetTickCount() - GetRandRange(FILEREASKTIME-120000,FILEREASKTIME);
		if (dwReAskTime > m_dwLastAskedTime)
			m_dwLastAskedTime = dwReAskTime;
	}
}
// khaos::kmod-
