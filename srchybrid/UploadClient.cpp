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
#include "Opcodes.h"
#include "Packets.h"
#include "UploadQueue.h"
#include "ClientList.h"
#include "ClientUDPSocket.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "PartFile.h"
#include "ClientCredits.h"
#include "ListenSocket.h"
#include "Sockets.h"
#include "OtherFunctions.h"
#include "SafeFile.h"
#include "downloadqueue.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#include "TransferWnd.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//	members of CUpDownClient
//	which are mainly used for uploading functions 

CBarShader CUpDownClient::s_UpStatusBar(16);

void CUpDownClient::DrawUpStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat) const
{
	COLORREF crBoth; 
	COLORREF crNeither; 
	COLORREF crClientOnly; 
	COLORREF crSending;
	COLORREF crNextSending;
	COLORREF crBuffer;

	if(bFlat) { 
		crBoth = RGB(0, 0, 0);
	} else { 
		crBoth = RGB(104, 104, 104);
	} 

	crNeither = RGB(224, 224, 224);
	crClientOnly = RGB(0, 220, 255);
	crSending = RGB(0, 150, 0);
	crNextSending = RGB(255,208,0);
	crBuffer = RGB(255, 100, 100);

	// wistily: UpStatusFix
	CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
	uint32 filesize;
	if (currequpfile)
		filesize=currequpfile->GetFileSize();
	else
		filesize=PARTSIZE*(m_nUpPartCount);
	// wistily: UpStatusFix

	s_UpStatusBar.SetFileSize(filesize); 
	s_UpStatusBar.SetHeight(rect->bottom - rect->top); 
	s_UpStatusBar.SetWidth(rect->right - rect->left); 
	s_UpStatusBar.Fill(crNeither); 
	if (!onlygreyrect && m_abyUpPartStatus) { 
		for (uint32 i = 0;i < m_nUpPartCount;i++)
			if(m_abyUpPartStatus[i])
				s_UpStatusBar.FillRange(PARTSIZE*(i),PARTSIZE*(i+1),crBoth);
	}
	Requested_Block_Struct* block;
	if (!m_BlockRequests_queue.IsEmpty()){
		block = m_BlockRequests_queue.GetHead();
		if(block){
			uint32 start = block->StartOffset/PARTSIZE;
			s_UpStatusBar.FillRange(start*PARTSIZE, (start+1)*PARTSIZE, crNextSending);
		}
	}
	// PENDING: this is currently commented so that a yellow block is only shown for the next
	//          requested package. I would like for this to be commented the next test release.
	//          When we have confirmed that sockets no longer just stop requesting blocks,
	//          the code can be restored. /zz
	//if (!m_DoneBlocks_list.IsEmpty()){
	//	block = m_DoneBlocks_list.GetTail();
	//	if(block){
	//		uint32 start = block->StartOffset/PARTSIZE;
	//		s_UpStatusBar.FillRange(start*PARTSIZE, (start+1)*PARTSIZE, crNextSending);
	//	}
	//}
	if (!m_DoneBlocks_list.IsEmpty()){
		//for(POSITION pos=m_DoneBlocks_list.GetHeadPosition();pos!=0;m_DoneBlocks_list.GetNext(pos)){
		//	Requested_Block_Struct* block = m_DoneBlocks_list.GetAt(pos);
		//	s_UpStatusBar.FillRange(block->StartOffset, block->EndOffset, crSending);
		//}

		// Also show what data is buffered (with color crBuffer) this is mostly a temporary feedback for debugging purposes. Could be removed for final.
		uint32 total = 0;
		for(POSITION pos = m_DoneBlocks_list.GetHeadPosition(); pos!=0; ){
			const Requested_Block_Struct* block = m_DoneBlocks_list.GetNext(pos);

			if(total + (block->EndOffset-block->StartOffset) < GetQueueSessionPayloadUp()) {
				// block is sent
				s_UpStatusBar.FillRange(block->StartOffset, block->EndOffset, crSending);
				total += block->EndOffset-block->StartOffset;
			}
			else if (total < GetQueueSessionPayloadUp()){
				// block partly sent, partly in buffer
				total += block->EndOffset-block->StartOffset;
				uint32 rest = total - GetQueueSessionPayloadUp();
				uint32 newEnd = block->EndOffset-rest;

    			s_UpStatusBar.FillRange(block->StartOffset, newEnd, crSending);
    			s_UpStatusBar.FillRange(newEnd, block->EndOffset, crBuffer);
			}
			else {
				// entire block is still in buffer
				total += block->EndOffset-block->StartOffset;
    			s_UpStatusBar.FillRange(block->StartOffset, block->EndOffset, crBuffer);
			}
		}
	}
   	s_UpStatusBar.Draw(dc, rect->left, rect->top, bFlat); 
} 

void CUpDownClient::SetUploadState(EUploadState news){
	// don't add any final cleanups for US_NONE here	
	//MORPH START - Added by SiRoB, Keep PowerShare State when client have been added in uploadqueue
	if (news!=US_UPLOADING) m_bPowerShared = GetPowerShared();
	//MORPH START - Added by SiRoB, Keep PowerShare State when client have been added in uploadqueue
	m_nUploadState = news;
	theApp.emuledlg->transferwnd->clientlistctrl.RefreshClient(this);
}
/**
* Gets the current waiting score for this client, taking into consideration waiting
* time, priority of requested file, and the client's credits.
*/
uint32 CUpDownClient::GetScore(bool sysvalue, bool isdownloading, bool onlybasevalue) const
{

	DWORD curTick = ::GetTickCount();

	//TODO: complete this (friends, uploadspeed, emuleuser etc etc)
	if (!m_pszUsername)
		return 0;

	if (credits == 0){
		ASSERT ( false );
		return 0;
	}

	CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
	if(!currequpfile)
		return 0;
	
	// bad clients (see note in function)
	if (credits->GetCurrentIdentState(GetIP()) == IS_IDBADGUY)
		return 0;

	//MORPH START - Added by IceCream, Anti-leecher feature
	if (IsLeecher())
		return 0;
	//MORPH END   - Added by IceCream, Anti-leecher feature

	// friend slot
	if (IsFriend() && GetFriendSlot() && !HasLowID())
		return 0x0FFFFFFF;

	if (IsBanned())
		return 0;

	if (sysvalue && HasLowID() && !(socket && socket->IsConnected())){
		return 0;
	}

	//MORPH - Changed by SiRoB, Upload ZZ System
	/*
	// TODO coded by tecxx & herbert, one yet unsolved problem here:
	// sometimes a client asks for 2 files and there is no way to decide, which file the 
	// client finally gets. so it could happen that he is queued first because of a 
	// high prio file, but then asks for something completely different.
	int filepriority = 10; // standard
	switch(currequpfile->GetUpPriority()){
		case PR_VERYHIGH:
			filepriority = 18;
			break;
		case PR_HIGH: 
			filepriority = 9; 
			break; 
		case PR_LOW: 
			filepriority = 6; 
			break; 
		case PR_VERYLOW:
			filepriority = 2;
			break;
		case PR_NORMAL: 
			default: 
			filepriority = 7; 
		break; 
	}
	*/
	int filepriority = GetFilePrioAsNumber();
	
	// calculate score, based on waitingtime and other factors
	float fBaseValue;
	if (onlybasevalue)
		fBaseValue = 100;
	else if (!isdownloading)
		fBaseValue = (float)(curTick-GetWaitStartTime())/1000;
	else{
		// we dont want one client to download forever
		// the first 15 min downloadtime counts as 15 min waitingtime and you get a 15 min bonus while you are in the first 15 min :)
		// (to avoid 20 sec downloads) after this the score won't raise anymore 
		fBaseValue = (float)(m_dwUploadTime-GetWaitStartTime());
//Morph Start- modifed by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
		// Moonlight: SUQWT - I'm exploiting negative overflows to adjust wait start times. Overflows should not be an issue as long
		// as queue turnover rate is faster than 49 days.
		// ASSERT ( m_dwUploadTime-GetWaitStartTime() >= 0 ); //oct 28, 02: changed this from "> 0" to ">= 0"//original commented out
//Morph End- modifed by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)		
		fBaseValue += (float)(curTick - m_dwUploadTime > 900000)? 900000:1800000;
		fBaseValue /= 1000;
	}
	if(thePrefs.UseCreditSystem())
	{
		float modif = credits->GetScoreRatio(GetIP());
		fBaseValue *= modif;
		if(!m_bySupportSecIdent){
			switch(thePrefs.GetCreditSystem()){
				case CS_OFFICIAL:
				//for those unsecure client have no credit, official gives lower Score
				case CS_PAWCIO:
					if(modif == 1)
						fBaseValue *= 0.95f;
					break;
				case CS_LOVELACE:
					//I think lovelace give enough punishment
				case CS_EASTSHARE:
					//this also punish those no credit client, so no need for more punishment
				default:
					break;
			}
		}
	}
	if (!onlybasevalue)
		fBaseValue *= (float(filepriority)/10.0f);
	//MORPH START - Added by Yun.SF3, boost friend
	if ((IsFriend()) && (thePrefs.IsBoostFriends()) && (thePrefs.UseCreditSystem()) && ((credits->GetCurrentIdentState(GetIP()) == IS_IDENTIFIED) || thePrefs.GetEnableAntiCreditHack() || ((!IsEmuleClient()) && (GetSourceExchangeVersion()==0)))) //MORPH - Added by IceCream, only boost for secured friend
		fBaseValue *=1.5f;
	//MORPH END - Added by Yun.SF3, boost friend

	if (!isdownloading && !onlybasevalue){
		if (sysvalue && HasLowID() && !(socket && socket->IsConnected()) ){
			if (!theApp.serverconnect->IsConnected() || theApp.serverconnect->IsLowID() || theApp.listensocket->TooManySockets()) //This may have to change when I add firewall support to Kad
				return 0;
		}
	}
	if( (IsEmuleClient() || this->GetClientSoft() < 10) && m_byEmuleVersion <= 0x19 )
		fBaseValue *= 0.5f;

	return (uint32)fBaseValue;
}

//MORPH START - Added by Yun.SF3, ZZ Upload System
/**
* Gets the queue score multiplier for this client, taking into consideration client's credits
* and the requested file's priority.
*/
double CUpDownClient::GetCombinedFilePrioAndCredit()
{
	ASSERT(credits != NULL);

	CKnownFile* clientReqFile = theApp.sharedfiles->GetFileByID((uchar*)GetUploadFileID());

	if(clientReqFile){
	
		//Morph Start - added by AndCycle, Equal Chance For Each File
		if(thePrefs.GetEqualChanceForEachFileMode() != ECFEF_DISABLE){
			return clientReqFile->GetEqualChanceValue();
		}
		//Morph End - added by AndCycle, Equal Chance For Each File

		return	(uint32)(10.0f*credits->GetScoreRatio(GetIP())*float(GetFilePrioAsNumber()));//original
	}
	return 0;

}


/**
* Gets the file multiplier for the file this client has requested.
*/
int CUpDownClient::GetFilePrioAsNumber() const
{
//MORPH START - Added by Yun.SF3, ZZ Upload System
	// TODO coded by tecxx & herbert, one yet unsolved problem here:
	// sometimes a client asks for 2 files and there is no way to decide, which file the 
	// client finally gets. so it could happen that he is queued first because of a 
	// high prio file, but then asks for something completely different.
	int filepriority = 10; // standard
	if(GetUploadFileID() != NULL){ 
		if(theApp.sharedfiles->GetFileByID(GetUploadFileID()) != NULL){ 
			if (theApp.clientcredits->IsSaveUploadQueueWaitTime()){
				switch(theApp.sharedfiles->GetFileByID(GetUploadFileID())->GetUpPriority()){ 
				//Morph Start - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
				// --> Moonlight: SUQWT - Changed the priority distribution for a wider spread.
					case PR_VERYHIGH:
						filepriority = 27;  // 18, 50% boost    <-- SUQWT - original values commented.
						break;
					case PR_HIGH: 
						filepriority = 12;  // 9, 33% boost
						break; 
					case PR_LOW: 
						filepriority = 5;   // 6, 17% reduction
						break; 
					case PR_VERYLOW:
						filepriority = 2;   // 2, no change
						break;
					case PR_NORMAL: 
						default: 
						filepriority = 8;   // 7, 14% boost
					break; 
				// <-- Moonlight: SUQWT
				//Morph End - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
				} 
			}
			else{
				switch(theApp.sharedfiles->GetFileByID(GetUploadFileID())->GetUpPriority()){ 
					case PR_VERYHIGH:
						filepriority = 18;
						break;
					case PR_HIGH: 
						filepriority = 9; 
						break; 
					case PR_LOW: 
						filepriority = 6; 
						break; 
					case PR_VERYLOW:
						filepriority = 2;
						break;
					case PR_NORMAL: 
						default: 
						filepriority = 7; 
					break; 
				} 
			}
		} 
	} 
//MORPH - Added by Yun.SF3, ZZ Upload System
	return filepriority;
}

//Morph Start - added by AndCycle, Equal Chance For Each File
double CUpDownClient::GetEqualChanceValue() const
{
	CKnownFile* currentReqFile = theApp.sharedfiles->GetFileByID((uchar*)GetUploadFileID());

	if(currentReqFile != NULL){
		return currentReqFile->GetEqualChanceValue();
	}
	return 0;
}
//Morph End - added by AndCycle, Equal Chance For Each File

//Morph Start - added by AndCycle, Pay Back First
bool CUpDownClient::IsMoreUpThanDown() const{
	return thePrefs.IsPayBackFirst() ? credits->GetPayBackFirstStatus() : false ;
}
//Morph End - added by AndCycle, Pay Back First

//MORPH START - Added by Yun.SF3, ZZ Upload System
/**
* Checks if the file this client has requested has release priority.
*
* @return true if the requested file has release priority
*/
bool CUpDownClient::GetPowerShared() const {
	//MORPH START - Changed by SiRoB, Keep PowerShare State when client have been added in uploadqueue
	if (credits)
		if (credits->GetCurrentIdentState(GetIP()) != IS_IDENTIFIED)
			return false;

	bool bPowerShared;
	if (GetUploadFileID() != NULL && theApp.sharedfiles->GetFileByID(GetUploadFileID()) != NULL) {
		bPowerShared = theApp.sharedfiles->GetFileByID(GetUploadFileID())->GetPowerShared();
	} else {
		bPowerShared = false;
	}
	return (m_bPowerShared && GetUploadState()==US_UPLOADING) || bPowerShared;
	//MORPH END   - Changed by SiRoB, Keep PowerShare State when client have been added in uploadqueue
}
//MORPH END - Added by Yun.SF3, ZZ Upload System

class CSyncHelper
{
public:
	CSyncHelper()
	{
		m_pObject = NULL;
	}
	~CSyncHelper()
	{
		if (m_pObject)
			m_pObject->Unlock();
	}
	CSyncObject* m_pObject;
};

void CUpDownClient::CreateNextBlockPackage(){
	// See if we can do an early return. There may be no new blocks to load from disk and add to buffer, or buffer may be large enough allready.
	if(m_BlockRequests_queue.IsEmpty() || // There are no new blocks requested
	m_addedPayloadQueueSession > GetQueueSessionPayloadUp() && m_addedPayloadQueueSession-GetQueueSessionPayloadUp() > 50*1024) { // the buffered data is large enough allready (at least 0.2 MBytes there)
		return;
	}

	CFile file;
	byte* filedata = 0;
	CString fullname;
	// -khaos--+++> Statistic to breakdown uploaded data by complete file vs. partfile.
	bool bFromPF = true;
	// <-----khaos-
	CSyncHelper lockFile;
	try{
		// Buffer new data if current buffer is less than 1 MBytes
		while (!m_BlockRequests_queue.IsEmpty() &&
			(m_addedPayloadQueueSession <= GetQueueSessionPayloadUp() || m_addedPayloadQueueSession-GetQueueSessionPayloadUp() < 100*1024)) {

			Requested_Block_Struct* currentblock = m_BlockRequests_queue.GetHead();
			CKnownFile* srcfile = theApp.sharedfiles->GetFileByID(currentblock->FileID);
			if (!srcfile)
				throw GetResString(IDS_ERR_REQ_FNF);

			if (srcfile->IsPartFile() && ((CPartFile*)srcfile)->GetStatus() != PS_COMPLETE){
				// Do not access a part file, if it is currently moved into the incoming directory.
				// Because the moving of part file into the incoming directory may take a noticable 
				// amount of time, we can not wait for 'm_FileCompleteMutex' and block the main thread.
				if (!((CPartFile*)srcfile)->m_FileCompleteMutex.Lock(0)){ // just do a quick test of the mutex's state and return if it's locked.
					return;
				}
				lockFile.m_pObject = &((CPartFile*)srcfile)->m_FileCompleteMutex;
				// If it's a part file which we are uploading the file remains locked until we've read the
				// current block. This way the file completion thread can not (try to) "move" the file into
				// the incoming directory.

				fullname = RemoveFileExtension(((CPartFile*)srcfile)->GetFullName());
			}
			else{
				fullname.Format("%s\\%s",srcfile->GetPath(),srcfile->GetFileName());
			}
		
			uint32 togo;
			if (currentblock->StartOffset > currentblock->EndOffset){
				togo = currentblock->EndOffset + (srcfile->GetFileSize() - currentblock->StartOffset);
			}
			else{
				togo = currentblock->EndOffset - currentblock->StartOffset;
				if (srcfile->IsPartFile() && !((CPartFile*)srcfile)->IsComplete(currentblock->StartOffset,currentblock->EndOffset-1))
					throw GetResString(IDS_ERR_INCOMPLETEBLOCK);
			}

			if( togo > EMBLOCKSIZE*3 )
				throw GetResString(IDS_ERR_LARGEREQBLOCK);
			
			if (!srcfile->IsPartFile()){
				// -khaos--+++> This is not a part file...
				bFromPF = false;
				// <-----khaos-
				if (!file.Open(fullname,CFile::modeRead|CFile::osSequentialScan|CFile::shareDenyNone))
					throw GetResString(IDS_ERR_OPEN);
				file.Seek(currentblock->StartOffset,0);
				
				filedata = new byte[togo+500];
				if (uint32 done = file.Read(filedata,togo) != togo){
					file.SeekToBegin();
					file.Read(filedata + done,togo-done);
				}
				file.Close();
			}
			else{
				CPartFile* partfile = (CPartFile*)srcfile;

				partfile->m_hpartfile.Seek(currentblock->StartOffset,0);
				
				filedata = new byte[togo+500];
				if (uint32 done = partfile->m_hpartfile.Read(filedata,togo) != togo){
					partfile->m_hpartfile.SeekToBegin();
					partfile->m_hpartfile.Read(filedata + done,togo-done);
				}
			}
			if (lockFile.m_pObject){
				lockFile.m_pObject->Unlock(); // Unlock the (part) file as soon as we are done with accessing it.
				lockFile.m_pObject = NULL;
			}

			SetUploadFileID(srcfile);

			// check extention to decide whether to compress or not
			CString ext=srcfile->GetFileName();ext.MakeLower();
			int pos=ext.ReverseFind('.');
			if (pos>-1) ext=ext.Mid(pos);
			bool compFlag=(ext!=".zip" && ext!=".rar" && ext!=".ace" && ext!=".ogm" && ext!=".tar");//no need to try compressing tar compressed files... [Yun.SF3]
			if (ext==".avi" && thePrefs.GetDontCompressAvi()) compFlag=false;

			// -khaos--+++> We're going to add bFromPF as a parameter to the calls to create packets...
			if (m_byDataCompVer == 1 && compFlag )
				CreatePackedPackets(filedata,togo,currentblock,bFromPF);
			else
				CreateStandartPackets(filedata,togo,currentblock,bFromPF);
			// <-----khaos-
			
			// file statistic
			//MORPH START - Added by IceCream SLUGFILLER: Spreadbars
			/*
			srcfile->statistic.AddTransferred(togo);
			*/
			srcfile->statistic.AddTransferred(currentblock->StartOffset, togo);
			//MORPH END - Added by IceCream SLUGFILLER: Spreadbars
			m_addedPayloadQueueSession += togo;

			m_DoneBlocks_list.AddHead(m_BlockRequests_queue.RemoveHead());
			delete[] filedata;
			filedata = 0;
		}
	}
	catch(CString error)
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false,GetResString(IDS_ERR_CLIENTERRORED),GetUserName(),error.GetBuffer());
		theApp.uploadqueue->RemoveFromUploadQueue(this, "Client error: " + error);
		if (filedata)
			delete[] filedata;
		return;
	}
	catch(CFileException* e)
	{
		if (thePrefs.GetVerbose())
		{
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			e->GetErrorMessage(szError, ARRSIZE(szError));
			AddDebugLogLine(false,_T("Failed to create upload package for %s - %s"),GetUserName(),szError);
		}
		theApp.uploadqueue->RemoveFromUploadQueue(this, ((CString)"Failed to create upload package."));
		if (filedata)
			delete[] filedata;
		e->Delete();
		return;
	}
	//if (thePrefs.GetVerbose())
	//	AddDebugLogLine(false,"Debug: Packet done. Size: %i",blockpack->GetLength());
	//return true;
}

void CUpDownClient::ProcessExtendedInfo(CSafeMemFile* data, CKnownFile* tempreqfile)
{
	if (m_abyUpPartStatus) 
	{
		delete[] m_abyUpPartStatus;
		m_abyUpPartStatus = NULL;	// added by jicxicmic
	}
	m_nUpPartCount = 0;
	m_nUpCompleteSourcesCount= 0;
	if( GetExtendedRequestsVersion() == 0 )
		return;
	if (data->GetLength() == 16){
		return;
		// to all developers: in the next version the client will be disconnected when causing this error!
		//please fix your protocol implementation (shareaza, xmule, etc)!
	}
	uint16 nED2KUpPartCount = data->ReadUInt16();
	if (!nED2KUpPartCount)
	{
		m_nUpPartCount = tempreqfile->GetPartCount();
		m_abyUpPartStatus = new uint8[m_nUpPartCount];
		memset(m_abyUpPartStatus,0,m_nUpPartCount);
	}
	else
	{
		if (tempreqfile->GetED2KPartCount() != nED2KUpPartCount)
		{
			//We already checked if we are talking about the same file.. So if we get here, something really strange happened!
			m_nUpPartCount = 0;
			return;
		}
		m_nUpPartCount = tempreqfile->GetPartCount();
		m_abyUpPartStatus = new uint8[m_nUpPartCount];
		uint16 done = 0;
		while (done != m_nUpPartCount)
		{
			uint8 toread = data->ReadUInt8();
			for (sint32 i = 0;i != 8;i++){
				m_abyUpPartStatus[done] = ((toread>>i)&1)? 1:0;
//				We may want to use this for another feature..
//				if (m_abyUpPartStatus[done] && !tempreqfile->IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1))
//					bPartsNeeded = true;
				done++;
				if (done == m_nUpPartCount)
					break;
			}
		}
		if (GetExtendedRequestsVersion() > 1)
		{
			uint16 nCompleteCountLast = GetUpCompleteSourcesCount();
			uint16 nCompleteCountNew = data->ReadUInt16();
			SetUpCompleteSourcesCount(nCompleteCountNew);
			if (nCompleteCountLast != nCompleteCountNew)
		{
				tempreqfile->UpdatePartsInfo();
			}
		}
	}
	theApp.emuledlg->transferwnd->queuelistctrl.RefreshClient(this);
}

// -khaos--+++> Added new parameter: bool bFromPF
void CUpDownClient::CreateStandartPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF){
	// <-----khaos-
	uint32 nPacketSize;
	CMemFile memfile((BYTE*)data,togo);
	if (togo > 10240) 
		nPacketSize = togo/(uint32)(togo/10240);
	else
		nPacketSize = togo;
	while (togo){
		if (togo < nPacketSize*2)
			nPacketSize = togo;
		togo -= nPacketSize;
		// -khaos--+++> Create the packet with the new boolean.
		Packet* packet = new Packet(OP_SENDINGPART,nPacketSize+24, OP_EDONKEYPROT, bFromPF);
		// <-----khaos-
		md4cpy(&packet->pBuffer[0],GetUploadFileID());
		uint32 statpos = (currentblock->EndOffset - togo) - nPacketSize;
		PokeUInt32(&packet->pBuffer[16], statpos);
		
		uint32 endpos = (currentblock->EndOffset - togo);
		PokeUInt32(&packet->pBuffer[20], endpos);
		
		memfile.Read(&packet->pBuffer[24],nPacketSize);

		if (thePrefs.GetDebugClientTCPLevel() > 0){
			DebugSend("OP__SendingPart", this, (char*)GetUploadFileID());
			Debug("  Start=%u  End=%u  Size=%u\n", statpos, endpos, nPacketSize);
		}
		// put packet directly on socket
		socket->SendPacket(packet,true,false, nPacketSize);
		//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	}
}

// -khaos--+++> Added new parameter: bool bFromPF
void CUpDownClient::CreatePackedPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF){
	// <-----khaos-
	BYTE* output = new BYTE[togo+300];
	uLongf newsize = togo+300;
	uint16 result = compress2(output,&newsize,data,togo,9);
	if (result != Z_OK || togo <= newsize){
		delete[] output;
		//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
		// -khaos--+++>  Our new boolean...
		CreateStandartPackets(data,togo,currentblock,bFromPF);
		// <-----khaos-
		return;
	}

	// khaos::kmod+ Show Compression by Tarod
	compressiongain += (togo-newsize);
	notcompressed += togo;
	// khaos::kmod-
	CMemFile memfile(output,newsize);
	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	uint32 oldSize = togo;
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	togo = newsize;
	uint32 nPacketSize;
	if (togo > 10240) 
		nPacketSize = togo/(uint32)(togo/10240);
	else
		nPacketSize = togo;
	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	uint32 totalPayloadSize = 0;
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	while (togo){
		if (togo < nPacketSize*2)
			nPacketSize = togo;
		togo -= nPacketSize;
		// -khaos--+++> Create the packet with the new boolean.
		Packet* packet = new Packet(OP_COMPRESSEDPART,nPacketSize+24,OP_EMULEPROT,bFromPF);
		// <-----khaos-
		md4cpy(&packet->pBuffer[0],GetUploadFileID());
		uint32 statpos = currentblock->StartOffset;
		PokeUInt32(&packet->pBuffer[16], statpos);
		PokeUInt32(&packet->pBuffer[20], newsize);
		memfile.Read(&packet->pBuffer[24],nPacketSize);

		if (thePrefs.GetDebugClientTCPLevel() > 0){
			DebugSend("OP__CompressedPart", this, (char*)GetUploadFileID());
			Debug("  Start=%u  BlockSize=%u  Size=%u\n", statpos, newsize, nPacketSize);
		}
		// approximate payload size
		uint32 payloadSize = nPacketSize*oldSize/newsize;

		if(togo == 0 && totalPayloadSize+payloadSize < oldSize) {
			payloadSize = oldSize-totalPayloadSize;
		}
		totalPayloadSize += payloadSize;

		// put packet directly on socket
		socket->SendPacket(packet,true,false, payloadSize);
		//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	}
	delete[] output;
}

void CUpDownClient::SetUploadFileID(CKnownFile* newreqfile)
{
	CKnownFile* oldreqfile;
	//We use the knownfilelist because we may have unshared the file..
	//But we always check the download list first because that person may have decided to redownload that file.
	//Which will replace the object in the knownfilelist if completed.
	if ((oldreqfile = theApp.downloadqueue->GetFileByID(requpfileid)) == NULL )
		oldreqfile = theApp.knownfiles->FindKnownFileByID(requpfileid);

	if(newreqfile == oldreqfile)
		return;
	if(newreqfile){
		//MORPH START - Changed by SiRoB, Keep PowerShare State when client have been added in uploadqueue
		m_bPowerShared = newreqfile->GetPowerShared();
		//MORPH END   - Changed by SiRoB, Keep PowerShare State when client have been added in uploadqueue
		newreqfile->AddUploadingClient(this);
		md4cpy(requpfileid, newreqfile->GetFileHash());
	}
	else
		md4clr(requpfileid);

	if (oldreqfile)
		oldreqfile->RemoveUploadingClient(this);
}

void CUpDownClient::AddReqBlock(Requested_Block_Struct* reqblock)
{
	for (POSITION pos = m_DoneBlocks_list.GetHeadPosition(); pos != 0; ){
		const Requested_Block_Struct* cur_reqblock = m_DoneBlocks_list.GetNext(pos);
		if (reqblock->StartOffset == cur_reqblock->StartOffset && reqblock->EndOffset == cur_reqblock->EndOffset){
			delete reqblock;
			return;
		}
	}
	for (POSITION pos = m_BlockRequests_queue.GetHeadPosition(); pos != 0; ){
		const Requested_Block_Struct* cur_reqblock = m_BlockRequests_queue.GetNext(pos);
		if (reqblock->StartOffset == cur_reqblock->StartOffset && reqblock->EndOffset == cur_reqblock->EndOffset){
			delete reqblock;
			return;
		}
	}
	m_BlockRequests_queue.AddTail(reqblock);

}

uint32 CUpDownClient::SendBlockData(){
//MORPH START - Added by Yun.SF3, ZZ Upload System
	DWORD curTick = ::GetTickCount();

	uint64 sentBytesCompleteFile = 0;
	uint64 sentBytesPartFile = 0;
	uint64 sentBytesPayload = 0;

	if(socket) {
		// Perform book keeping

		// first get how many bytes was send
		sentBytesCompleteFile = socket->GetSentBytesCompleteFileSinceLastCallAndReset();
		sentBytesPartFile = socket->GetSentBytesPartFileSinceLastCallAndReset();

		// store this information in proper places
		// -khaos--+++>
		// Extended statistics information based on which client software and which port we sent this data to...
		// This also updates the grand total for sent bytes, etc.  And where this data came from.  Yeesh.
		thePrefs.Add2SessionTransferData(GetClientSoft(), GetUserPort(), false, true, sentBytesCompleteFile, (IsFriend()&& GetFriendSlot()));
		thePrefs.Add2SessionTransferData(GetClientSoft(), GetUserPort(), true, true, sentBytesPartFile, (IsFriend()&& GetFriendSlot()));
		m_nTransferedUp += sentBytesCompleteFile + sentBytesPartFile;
		credits->AddUploaded(sentBytesCompleteFile + sentBytesPartFile, GetIP()); 

		sentBytesPayload = socket->GetSentPayloadSinceLastCallAndReset();
		m_nCurQueueSessionPayloadUp += sentBytesPayload;

		if(GetUploadState() == US_UPLOADING) {
			bool useChunkLimit = false; // PENDING: Get from prefs, or enforce?

			bool wasRemoved = false;
			if(useChunkLimit == false && GetQueueSessionPayloadUp() > SESSIONAMOUNT && curTick-m_dwLastCheckedForEvictTick >= 5*1000) {
				m_dwLastCheckedForEvictTick = curTick;
				wasRemoved = theApp.uploadqueue->RemoveOrMoveDown(this, true);
				//MORPH START - Changed by SiRoB, Keep PowerShare State when client have been added in uploadqueue
				//reset
				m_bPowerShared = false;
				//MORPH START - Changed by SiRoB, Keep PowerShare State when client have been added in uploadqueue
			}

			if(wasRemoved == false && GetQueueSessionPayloadUp()/SESSIONAMOUNT > m_curSessionAmountNumber) {
				// Should we end this upload?

				//EastShare Start - added by AndCycle, Pay Back First
				//check again does client satisfy the conditions
				credits->InitPayBackFirstStatus();
				//EastShare End - added by AndCycle, Pay Back First

				// first clear the average speed, to show ?? as speed in upload slot display
				m_AvarageUDR_list.RemoveAll();
				sumavgUDR = 0;
	
				// Give clients in queue a chance to kick this client out.
				// It will be kicked out only if queue contains a client
				// of same/higher class as this client, and that new
				// client must either be a high ID client, or a low ID
				// client that is currently connected.
				wasRemoved = theApp.uploadqueue->RemoveOrMoveDown(this);

				if(!wasRemoved) {
					// It wasn't removed, so it is allowed to pass into the next amount.
					m_curSessionAmountNumber++;
				}
			}

			if(wasRemoved == false) {
				// read blocks from file and put on socket
				CreateNextBlockPackage();
			}
		}
	}

	if(sentBytesCompleteFile + sentBytesPartFile > 0 ||
		m_AvarageUDR_list.GetCount() == 0 || (::GetTickCount() - m_AvarageUDR_list.GetTail().timestamp) > 1*1000) {
		// Store how much data we've transfered this round,
		// to be able to calculate average speed later
		// keep sum of all values in list up to date
		TransferredData newitem = {sentBytesCompleteFile + sentBytesPartFile, curTick};
		m_AvarageUDR_list.AddTail(newitem);
		sumavgUDR += sentBytesCompleteFile + sentBytesPartFile;
	}
			
	// remove to old values in list
	while (m_AvarageUDR_list.GetCount() > 0 && (::GetTickCount() - m_AvarageUDR_list.GetHead().timestamp) > 10*1000) {
		// keep sum of all values in list up to date
		sumavgUDR -= m_AvarageUDR_list.RemoveHead().datalen;
	}

	// Calculate average speed for this slot
	if(m_AvarageUDR_list.GetCount() > 0 && (::GetTickCount() - m_AvarageUDR_list.GetHead().timestamp) > 0 && GetUpStartTimeDelay() > 2*1000) {
		m_nUpDatarate = (sumavgUDR*1000) / (::GetTickCount()-m_AvarageUDR_list.GetHead().timestamp);
	} else {
		// not enough values to calculate trustworthy speed. Use -1 to tell this
		m_nUpDatarate = -1;
	}

	// Check if it's time to update the display.
	if (curTick-m_lastRefreshedULDisplay > MINWAIT_BEFORE_ULDISPLAY_WINDOWUPDATE+(uint32)(rand()*800/RAND_MAX)) {
		// Update display
		theApp.emuledlg->transferwnd->uploadlistctrl.RefreshClient(this);
		theApp.emuledlg->transferwnd->clientlistctrl.RefreshClient(this);
		m_lastRefreshedULDisplay = curTick;
	}
	
	return sentBytesCompleteFile + sentBytesPartFile;
}

/**
* See description for CEMSocket::TruncateQueues().
*/
void CUpDownClient::FlushSendBlocks(){ // call this when you stop upload, or the socket might be not able to send
	//MORPH START - Added by Yun.SF3, ZZ Upload System    
	if (socket)	//socket may be NULL...
		socket->TruncateQueues();
	//MORPH END - Added by Yun.SF3, ZZ Upload System
}

void CUpDownClient::SendHashsetPacket(char* forfileid){
	CKnownFile* file = theApp.sharedfiles->GetFileByID((uchar*)forfileid);
	if (!file)
		throw GetResString(IDS_ERR_REQ_FNF) + _T(" (SendHashsetPacket)");

	CSafeMemFile data(1024);
	data.WriteHash16(file->GetFileHash());
	UINT parts = file->GetHashCount();
	data.WriteUInt16(parts);
	for (UINT i = 0; i < parts; i++)
		data.WriteHash16(file->GetPartHash(i));
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__HashSetAnswer", this, forfileid);
	Packet* packet = new Packet(&data);
	packet->opcode = OP_HASHSETANSWER;
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet,true,true);
}

void CUpDownClient::ClearUploadBlockRequests()
{
	FlushSendBlocks();

	for (POSITION pos = m_BlockRequests_queue.GetHeadPosition();pos != 0;)
		delete m_BlockRequests_queue.GetNext(pos);
	m_BlockRequests_queue.RemoveAll();
	
	for (POSITION pos = m_DoneBlocks_list.GetHeadPosition();pos != 0;)
		delete m_DoneBlocks_list.GetNext(pos);
	m_DoneBlocks_list.RemoveAll();
}

void CUpDownClient::SendRankingInfo(){
	if (!ExtProtocolAvailable())
		return;
	uint16 nRank = theApp.uploadqueue->GetWaitingPosition(this);
	if (!nRank)
		return;
	Packet* packet = new Packet(OP_QUEUERANKING,12,OP_EMULEPROT);
	PokeUInt16(packet->pBuffer+0, nRank);
	memset(packet->pBuffer+2, 0, 10);
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__QueueRank", this);
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet,true,true);
}

void CUpDownClient::SendCommentInfo(/*const*/ CKnownFile *file)
{
	if (!m_bCommentDirty || file == NULL || !ExtProtocolAvailable() || m_byAcceptCommentVer < 1)
		return;
	m_bCommentDirty = false;

	uint8 rating = file->GetFileRate();
	CString desc=file->GetFileComment();
	if(file->GetFileRate() == 0 && desc.IsEmpty())
		return;

	CSafeMemFile data(256);
	data.WriteUInt8(rating);
	int length=desc.GetLength();
	if (length>128)
		length=128;
	data.WriteUInt32(length);
	if (length>0)
		data.Write(desc.GetBuffer(),length);
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__FileDesc", this, (char*)file->GetFileHash());
	Packet *packet = new Packet(&data,OP_EMULEPROT);
	packet->opcode = OP_FILEDESC;
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet,true);
}

void  CUpDownClient::AddRequestCount(const uchar* fileid)
{
	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition(); pos != 0; ){
		Requested_File_Struct* cur_struct = m_RequestedFiles_list.GetNext(pos);
		if (!md4cmp(cur_struct->fileid,fileid)){
			if (::GetTickCount() - cur_struct->lastasked < MIN_REQUESTTIME && !GetFriendSlot()){ 
				if (GetDownloadState() != DS_DOWNLOADING)
					cur_struct->badrequests++;
				if (cur_struct->badrequests == BADCLIENTBAN){
					Ban();
				}
			}
			else{
				if (cur_struct->badrequests)
					cur_struct->badrequests--;
			}
			cur_struct->lastasked = ::GetTickCount();
			return;
		}
	}
	Requested_File_Struct* new_struct = new Requested_File_Struct;
	md4cpy(new_struct->fileid,fileid);
	new_struct->lastasked = ::GetTickCount();
	new_struct->badrequests = 0;
	m_RequestedFiles_list.AddHead(new_struct);
}

void  CUpDownClient::UnBan(){
	theApp.clientlist->AddTrackClient(this);
	theApp.clientlist->RemoveBannedClient( GetIP() );
	SetUploadState(US_NONE);
	ClearWaitStartTime();
	theApp.emuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition();pos != 0;){
		Requested_File_Struct* cur_struct = m_RequestedFiles_list.GetNext(pos);
		cur_struct->badrequests = 0;
		cur_struct->lastasked = 0;	
	}
//	theApp.emuledlg->transferwnd->queuelistctrl.RefreshClient(this, true, true);
}

// Moonlight: SUQWT - Reset the wait time on ban, do not give time credit for banned clients queue time!//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
void CUpDownClient::Ban(){
	theApp.clientlist->AddTrackClient(this);
	if(theApp.clientcredits->IsSaveUploadQueueWaitTime()) ClearWaitStartTime();	// Moonlight: SUQWT//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	if ( !IsBanned() ){
		if (thePrefs.GetLogBannedClients())
			AddDebugLogLine(false,GetResString(IDS_CLIENTBLOCKED),GetUserName());
	}
#ifdef _DEBUG
	else{
		if (thePrefs.GetLogBannedClients())
			AddDebugLogLine(false,"Ban refreshed for %s (%s) ", GetUserName(), ipstr(GetConnectIP()));
	}
#endif
	theApp.clientlist->AddBannedClient( GetIP() );
	//theApp.uploadqueue->UpdateBanCount();
	SetUploadState(US_BANNED);
	theApp.emuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	theApp.emuledlg->transferwnd->queuelistctrl.RefreshClient(this);

}

//MORPH START - Added by IceCream, Anti-leecher feature
void CUpDownClient::BanLeecher(int log_message){
	theApp.clientlist->AddTrackClient(this);
	if (!m_bLeecher){
		theApp.stat_leecherclients++;
		m_bLeecher = true;
	}
	theApp.clientlist->AddBannedClient( GetIP() );
	SetUploadState(US_BANNED);
	theApp.emuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	theApp.emuledlg->transferwnd->queuelistctrl.RefreshClient(this);
	if (log_message)
		theApp.emuledlg->AddDebugLogLine(false,GetResString(IDS_ANTILEECHERLOG),GetUserName());
}
//MORPH END   - Added by IceCream, Anti-leecher feature

// Moonlight: SUQWT - Compare linear time instead of time indexes to avoid overflow-induced false positives.//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
uint32 CUpDownClient::GetWaitStartTime() const
{
	if (credits == NULL){
		ASSERT ( false );
		return 0;
	}
	uint32 dwResult = credits->GetSecureWaitStartTime(GetIP());
//MORPH START - Changed by SiRoB
/*
	//Morph Start - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	uint32 dwTicks = ::GetTickCount();
	// Moonlight: SUQWT - To avoid erroneous check due to overflow, convert time index to plain time.
	if ((dwTicks - dwResult) < (dwTicks - m_dwUploadTime) && IsDownloading()){	
	//if (dwResult > m_dwUploadTime && IsDownloading()){//original commented out
//Morph End - modified by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
*/
	uint32 dwTicks = ::GetTickCount();
	if ((!theApp.clientcredits->IsSaveUploadQueueWaitTime() && (dwResult > m_dwUploadTime) ||
		theApp.clientcredits->IsSaveUploadQueueWaitTime() && ((dwTicks - dwResult) < (dwTicks - m_dwUploadTime)))
		&& IsDownloading()){
//MORPH END - Changed by SiRoB
	//this happens only if two clients with invalid securehash are in the queue - if at all
			dwResult = m_dwUploadTime-1;

		if (thePrefs.GetVerbose())
			DEBUG_ONLY(AddDebugLogLine(false,"Warning: CUpDownClient::GetWaitStartTime() waittime Collision (%s)",GetUserName()));
	}
	return dwResult;
}

void CUpDownClient::SetWaitStartTime(){
	if (credits == NULL){
		return;
	}
	credits->SetSecWaitStartTime(GetIP());
}

void CUpDownClient::ClearWaitStartTime(){
	if (credits == NULL){
		return;
	}
	credits->ClearWaitStartTime();
}

bool CUpDownClient::GetFriendSlot() const
{
	if (credits && theApp.clientcredits->CryptoAvailable()){
		switch(credits->GetCurrentIdentState(GetIP())){
			case IS_IDFAILED:
			case IS_IDNEEDED:
			case IS_IDBADGUY:
				return false;
		}
	}
	return m_bFriendSlot;
}
/* Name:     IsCommunity
   Function: Test if client is community member
   Return:   true  - if one of the community tags occur in the name of the actual client
					 and community sharing is enabled
			 false - otherwise
   Remarks:  All strings will be treated case-insensitive. There can be more than one
			 community tag in the community tag string - all these strings must be separated
			 by "|". Spaces around community tags are trimmed.
   Author:   Mighty Knife
*/
bool CUpDownClient::IsCommunity() const {
	if (!thePrefs.IsCommunityEnabled()) return false;
	CString ntemp = m_pszUsername;
	ntemp.MakeLower ();
	CString ctemp = thePrefs.GetCommunityName(); 
	ctemp.MakeLower ();
	bool isCom = false;
// The different community tags are separated by "|", so we have to extract each
// before testing if it's contained in the username.
	int p=0;
	do {
		CString tag = ctemp.Tokenize ("|",p).Trim ();
		if (tag != "") isCom = ntemp.Find (tag) >= 0;
	} while ((!isCom) && (p >= 0));
	return isCom;
}
// [end] Mighty Knife
