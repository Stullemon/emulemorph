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
#include "UpDownClient.h"
#include "opcodes.h"
#include "packets.h"
#include "emule.h"
#include "uploadqueue.h"
#include "otherstructs.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//	members of CUpDownClient
//	which are mainly used for uploading functions 

CBarShader CUpDownClient::s_UpStatusBar(16);

void CUpDownClient::DrawUpStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat){ 
	COLORREF crBoth; 
	COLORREF crNeither; 
	COLORREF crClientOnly; 
	COLORREF crSending;
	COLORREF crNextSending;

	if(bFlat) { 
		crBoth = RGB(0, 0, 0);
		crNeither = RGB(224, 224, 224);
		crClientOnly = RGB(0, 220, 255);
		crSending = RGB(0,150,0);
		crNextSending = RGB(255,208,0);
	} else { 
		crBoth = RGB(104, 104, 104);
		crNeither = RGB(224, 224, 224);
		crClientOnly = RGB(0, 220, 255);
		crSending = RGB(0, 150, 0);
		crNextSending = RGB(255,208,0);
	} 
	//MORPH START - Added by SiRoB, ZZ Upload System 20030724-0336
	uint16 partCount = 0;

	if(m_nUpPartCount > 0) {
		partCount = m_nUpPartCount;
	} else if(GetUploadFileID() != NULL) {
        CKnownFile* knownFile = theApp.sharedfiles->GetFileByID(GetUploadFileID());
        if(knownFile != NULL) {
            partCount = knownFile->GetPartCount();
        } else {
            partCount = 0;
        }
	}

	if(partCount > 0) {
		//uint32 filesize = PARTSIZE*(partCount);// wrong: the last part is not 9.28 MB
		//wistily correct filesize calculation START
		CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
		if(!currequpfile)  return;
		uint32 filesize = currequpfile->GetFileSize();
		//wistily end

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
		if (!m_DoneBlocks_list.IsEmpty()){
			block = m_DoneBlocks_list.GetTail();
			if(block){
				uint32 start = block->StartOffset/PARTSIZE;
				s_UpStatusBar.FillRange(start*PARTSIZE, (start+1)*PARTSIZE, crNextSending);
			}
		}
		if (!m_DoneBlocks_list.IsEmpty()){
			for(POSITION pos=m_DoneBlocks_list.GetHeadPosition();pos!=0;m_DoneBlocks_list.GetNext(pos)){
				Requested_Block_Struct* block = m_DoneBlocks_list.GetAt(pos);
				s_UpStatusBar.FillRange(block->StartOffset, block->EndOffset, crSending);
			}
		}
		s_UpStatusBar.Draw(dc, rect->left, rect->top, bFlat); 
	}
	//MORPH END - Added by SiRoB, ZZ Upload System 20030723-0133
} 

void CUpDownClient::SetUploadState(EUploadState news){
	m_nUploadState = news;
	theApp.emuledlg->transferwnd.clientlistctrl.RefreshClient(this);
}

//MORPH START - Added by Yun.SF3, ZZ Upload System
/**
 * Gets the queue score multiplier for this client, taking into consideration client's credits
 * and the requested file's priority.
 */
float CUpDownClient::GetCombinedFilePrioAndCredit() {
	ASSERT(credits != NULL);
	//MORPH START - Added by Yun.SF3, Only give credits to secured clients
	if (credits->GetCurrentIdentState(GetIP()) == IS_IDENTIFIED || theApp.glob_prefs->GetEnableAntiCreditHack())
		return (uint32)(10.0f*credits->GetScoreRatio(GetIP())*float(GetFilePrioAsNumber()));
	else 
		return float(GetFilePrioAsNumber());
	//MORPH END - Added by Yun.SF3, Only give credits to secured clients
	}


/**
 * Gets the file multiplier for the file this client has requested.
 */
int CUpDownClient::GetFilePrioAsNumber() {
//MORPH START - Added by Yun.SF3, ZZ Upload System
	// TODO coded by tecxx & herbert, one yet unsolved problem here:
	// sometimes a client asks for 2 files and there is no way to decide, which file the 
	// client finally gets. so it could happen that he is queued first because of a 
	// high prio file, but then asks for something completely different.
	int filepriority = 10; // standard
	if(GetUploadFileID() != NULL){ 
		if(theApp.sharedfiles->GetFileByID(GetUploadFileID()) != NULL){ 
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
//MORPH - Added by Yun.SF3, ZZ Upload System
	return filepriority;
}

/**
 * Gets the current waiting score for this client, taking into consideration waiting
 * time, priority of requested file, and the client's credits.
 */
uint32 CUpDownClient::GetScore(bool sysvalue, bool isdownloading, bool onlybasevalue){
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

	int filepriority = GetFilePrioAsNumber();
	//MORPH - Added by Yun.SF3, ZZ Upload System

	// calculate score, based on waitingtime and other factors
	float fBaseValue;
	if (onlybasevalue)
		fBaseValue = 100;
	else if (!isdownloading)
		fBaseValue = (float)(::GetTickCount()-GetWaitStartTime())/1000;
	else{
		// we dont want one client to download forever
		// the first 15 min downloadtime counts as 15 min waitingtime and you get a 15 min bonus while you are in the first 15 min :)
		// (to avoid 20 sec downloads) after this the score won't raise anymore 
		fBaseValue = (float)(m_dwUploadTime-GetWaitStartTime());
		ASSERT ( m_dwUploadTime-GetWaitStartTime() >= 0 ); //oct 28, 02: changed this from "> 0" to ">= 0"
		fBaseValue += (float)(::GetTickCount() - m_dwUploadTime > 900000)? 900000:1800000;
		fBaseValue /= 1000;
	}
	if (((theApp.glob_prefs->UseCreditSystem()) && (credits->GetCurrentIdentState(GetIP()) == IS_IDENTIFIED)) || ((!IsEmuleClient()) && (GetSourceExchangeVersion()== 0)) || theApp.glob_prefs->GetEnableAntiCreditHack()) //MORPH - Added by IceCream, count credit also for edonkey users
	{
		float modif = credits->GetScoreRatio(GetIP());
		fBaseValue *= modif;
		if( !m_bySupportSecIdent && modif == 1 )
			fBaseValue *= 0.85f;
	}
	if (!onlybasevalue)
		fBaseValue *= (float(filepriority)/10.0f);
	//MORPH START - Added by Yun.SF3, boost the less uploaded files
	if (!theApp.sharedfiles->GetFileByID(GetUploadFileID())->IsAutoUpPriority() && theApp.glob_prefs->IsBoostLess())
	{
		if (theApp.sharedfiles->GetFileByID(GetUploadFileID())->statistic.GetAccepts())
			fBaseValue /= theApp.sharedfiles->GetFileByID(GetUploadFileID())->statistic.GetAccepts();
		else 
			fBaseValue *= 2;
	}
	if ((IsFriend()) && (theApp.glob_prefs->IsBoostFriends()) && (theApp.glob_prefs->UseCreditSystem()) && ((credits->GetCurrentIdentState(GetIP()) == IS_IDENTIFIED) || theApp.glob_prefs->GetEnableAntiCreditHack() || ((!IsEmuleClient()) && (GetSourceExchangeVersion()==0)))) //MORPH - Added by IceCream, only boost for secured friend
		fBaseValue *=1.5f;
	//MORPH END - Added by Yun.SF3, boost the less uploaded files
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

//EastShare Start - added by AndCycle, Pay Back First
bool CUpDownClient::MoreUpThanDown(){

	if(!theApp.glob_prefs->IsPayBackFirst()){
		return false;

	}else if((credits->GetCurrentIdentState(GetIP()) == IS_IDFAILED || 
		credits->GetCurrentIdentState(GetIP()) == IS_IDBADGUY || 
		credits->GetCurrentIdentState(GetIP()) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable()){
		return false;

	}else if(credits->GetDownloadedTotal() < 1048576){
		return false;

//	}else if(GetUploadState() == US_UPLOADING){

	}else if(GetQueueSessionPayloadUp() > 0){//keep PayBackFirst client for full chunk transfer

		if(GetQueueSessionPayloadUp() > SESSIONAMOUNT){//kick PayBackFirst client after full chunk transfer
			return false;
		}else{
			return chkPayBackFirstTag();
		}
	}else{
		setPayBackFirstTag(credits->GetDownloadedTotal() > credits->GetUploadedTotal());
	}

	return chkPayBackFirstTag();
}
//EastShare End - added by AndCycle, Pay Back First

//MORPH START - Added by Yun.SF3, ZZ Upload System
/**
 * Checks if the file this client has requested has release priority.
 *
 * @return true if the requested file has release priority
 */
bool CUpDownClient::GetPowerShared() {
	if ((credits->GetCurrentIdentState(GetIP()) == IS_IDFAILED || 
		credits->GetCurrentIdentState(GetIP()) == IS_IDBADGUY || 
		credits->GetCurrentIdentState(GetIP()) == IS_IDNEEDED) && theApp.clientcredits->CryptoAvailable()){
		return false;
	}
	if(GetUploadFileID() != NULL &&
       theApp.sharedfiles->GetFileByID(GetUploadFileID()) != NULL) {
		return theApp.sharedfiles->GetFileByID(GetUploadFileID())->GetPowerShared();
	} else {
		return false;
	}
}
//MORPH END - Added by Yun.SF3, ZZ Upload System

// Checks if it is next requested block from another chunk of the actual file or from another file 
// 
// [Returns] 
//   true : Next requested block is from another different chunk or file than last downloaded block 
//   false: Next requested block is from same chunk that last downloaded block 
bool CUpDownClient::IsDifferentPartBlock(bool startNextChunk)
{ 
	Requested_Block_Struct* lastBlock;
	Requested_Block_Struct* currBlock;
	uint32 lastDone = 0;
	uint32 currRequested = 0;
	
	bool different = false;
	
	try {
		if(!m_BlockRequests_queue.IsEmpty()) {
			currBlock = (Requested_Block_Struct*)m_BlockRequests_queue.GetHead(); 
			currRequested = currBlock->StartOffset / PARTSIZE; 

			if(m_currentPartNumberIsKnown == false || startNextChunk && currRequested != m_currentPartNumber) {
				m_currentPartNumberIsKnown = true;
				m_currentPartNumber = currRequested;
			}
		}

		// Check if we have good lists and proceed to check for different chunks
		if (!m_BlockRequests_queue.IsEmpty() && !m_DoneBlocks_list.IsEmpty()) {
			// Calculate corresponding parts to blocks
			lastBlock = (Requested_Block_Struct*)m_DoneBlocks_list.GetTail();
			lastDone = lastBlock->StartOffset / PARTSIZE;
             
			// Test is we are asking same file and same part
			if ( lastDone != currRequested)
			{ 
				different = true;
				//theApp.emuledlg->AddDebugLogLine(false, "%s: Upload session ended due to new chunk.", this->GetUserName()); //MORPH - Added by Yun.SF3, ZZ Upload System

			}
			if (md4cmp(lastBlock->FileID, currBlock->FileID) != 0)
			{ 
				different = true;
				//AddDebugLogLine(false, "%s: Upload session ended due to different file.", this->GetUserName()); //MORPH - Added by Yun.SF3, ZZ Upload System

			}
		} 
   	}
   	catch(...)
   	{ 
			AddDebugLogLine(false, "%s: Upload session ended due to error.", this->GetUserName());
      		different = true; 
   	} 
//	theApp.emuledlg->AddDebugLogLine(false, "Debug: User %s, last_done_part (%u) %s (%u) next_requested_part, sent %u Kbs.", GetUserName(), last_done_part, different_part? "!=": "==", next_requested_part, this->GetTransferedUp() / 1024); 

	return different; 
}

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

void CUpDownClient::CreateNextBlockPackage(bool startNextChunk){
//MORPH START - Added by SiRoB, ZZ Upload System
	bool useChunkLimit = false;
	// time critical
	// check if we should kick this client
	// Note: Full chunk transfers really should be enforced, not an option

    if(m_BlockRequests_queue.IsEmpty() ||
       m_addedPayloadQueueSession > GetQueueSessionPayloadUp() && m_addedPayloadQueueSession-GetQueueSessionPayloadUp() > 512*1024) {
        // There are no new blocks requested, but we will happily transfer more
        // if the client just requests something
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
		bool firstLoop = true;

		// repeat as long as next requested block is in the same chunk as previous blocks
		// and as long as there is a next requested block
        while (!m_BlockRequests_queue.IsEmpty() &&
               (m_addedPayloadQueueSession < GetQueueSessionPayloadUp() || m_addedPayloadQueueSession-GetQueueSessionPayloadUp() < 1*1024*1024)) {
			firstLoop = false;
			Requested_Block_Struct* currentblock = m_BlockRequests_queue.RemoveHead();
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

            // Everyone are limited to a single chunk (the client may be allowed to stay connected pass
            // a single chunk, but that is decided at another place. From there parameter startNextChunk may
            // may be passed as true, to allow jump into next chunk).

            if(currentblock->EndOffset > srcfile->GetFileSize()) {
                currentblock->EndOffset = srcfile->GetFileSize();
            }

            uint32 newEndOffset;
            
            // prevent overflow
            if(_UI32_MAX-currentblock->StartOffset > 1*1024*1024) {
                newEndOffset = currentblock->StartOffset + 1*1024*1024;
            } else {
                newEndOffset = _UI32_MAX;
            }

            if(newEndOffset > srcfile->GetFileSize()) {
                newEndOffset = srcfile->GetFileSize();
            }

            if(currentblock->StartOffset < currentblock->EndOffset && newEndOffset > currentblock->EndOffset) {
                newEndOffset = currentblock->EndOffset;
            }

            // Check that StartOffset and EndOffset is in the same chunk
            if(currentblock->EndOffset > newEndOffset || currentblock->EndOffset < currentblock->StartOffset) {
                // The EndOffset goes into the next chunk. Split this request in two parts.
                // Set the current block's EndOffSet to the end of the chunk that StartOffset is in.
                // The part of the request that is after the chunk end, is inserted into a new block
                // and that block is put back on the block request queue. It will be worked off the
                // next call, unless the client is kicked due to reached chunk end.

                // PENDING: This limiting need to be checked carefully, to see that it is really possible
                //          to get a full chunk! Can the limiting below be confirmed?

                // create the "rest block"
                Requested_Block_Struct* tempblock = new Requested_Block_Struct;
                if(newEndOffset < srcfile->GetFileSize()) {
                    // it was not a wraparound request
                    tempblock->StartOffset = newEndOffset;
                } else {
                    // it was a wraparound request
                    tempblock->StartOffset = 0;
                }
                tempblock->EndOffset = currentblock->EndOffset;
                md4cpy(&tempblock->FileID,&currentblock->FileID);

                m_BlockRequests_queue.AddHead(tempblock);

                // shorten the current block
                currentblock->EndOffset = newEndOffset;
            }

            // This can no longer be a wrapped around request, since it has been limited to
                // a single chunk.
				togo = currentblock->EndOffset - currentblock->StartOffset;

		if (srcfile->IsPartFile() && !((CPartFile*)srcfile)->IsComplete(currentblock->StartOffset,currentblock->EndOffset-1))
					throw GetResString(IDS_ERR_INCOMPLETEBLOCK);

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
			if (lockFile.m_pObject)
				lockFile.m_pObject->Unlock(); // Unlock the (part) file as soon as we are done with accessing it.

			SetUploadFileID(currentblock->FileID);

			// check extention to decide whether to compress or not
			CString ext=srcfile->GetFileName();ext.MakeLower();
			int pos=ext.ReverseFind('.');
			if (pos>-1) ext=ext.Mid(pos);
			bool compFlag=(ext!=".zip" && ext!=".rar" && ext!=".ace" && ext!=".ogm" );
			if (ext==".avi" && theApp.glob_prefs->GetDontCompressAvi()) compFlag=false;

			// -khaos--+++> We're going to add bFromPF as a parameter to the calls to create packets...
			if (m_byDataCompVer == 1 && compFlag )
				CreatePackedPackets(filedata,togo,currentblock,bFromPF);
			else
				CreateStandartPackets(filedata,togo,currentblock,bFromPF);
			// <-----khaos-
			
			// file statistic
			//MORPH START - Added by IceCream SLUGFILLER: Spreadbars
			//srcfile->statistic.AddTransferred(togo);
			srcfile->statistic.AddTransferred(currentblock->StartOffset, togo);
			//MORPH END - Added by IceCream SLUGFILLER: Spreadbars
			m_addedPayloadQueueSession += togo;
			m_DoneBlocks_list.AddTail(currentblock);
			delete[] filedata;
			filedata = 0;
		}
	}
	catch(CString error){
		OUTPUT_DEBUG_TRACE();
		AddDebugLogLine(false,GetResString(IDS_ERR_CLIENTERRORED),GetUserName(),error.GetBuffer());
		theApp.uploadqueue->RemoveFromUploadQueue(this, "Client error: " + error);
		SetUploadFileID(NULL);
		if (filedata)
			delete[] filedata;
		return;
	}
	catch(CFileException* e){
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		e->GetErrorMessage(szError, ARRSIZE(szError));
		AddDebugLogLine(false,_T("Failed to create upload package for %s - %s"),GetUserName(),szError);
		theApp.uploadqueue->RemoveFromUploadQueue(this, ((CString)"Failed to create upload package.") + szError);
		SetUploadFileID(NULL);
		if (filedata)
			delete[] filedata;
		e->Delete();
		return;
	}
//	AddDebugLogLine(false,"Debug: Packet done. Size: %i",blockpack->GetLength());
	return;
}


void CUpDownClient::ProcessUpFileStatus(char* packet,uint32 size){
	if (m_abyUpPartStatus) {
		delete[] m_abyUpPartStatus;
		m_abyUpPartStatus = NULL;	// added by jicxicmic
	}
	m_nUpPartCount = 0;
	m_nUpCompleteSourcesCount= 0;
	if( size == 16 )
		return;
	CSafeMemFile data((BYTE*)packet,size);
	uchar cfilehash[16];
	data.Read(cfilehash,16);
	CKnownFile* tempreqfile = theApp.sharedfiles->GetFileByID(cfilehash);
	uint16 nED2KUpPartCount;
	data.Read(&nED2KUpPartCount,2);
	if (!nED2KUpPartCount){
		m_nUpPartCount = tempreqfile->GetPartCount();
		m_abyUpPartStatus = new uint8[m_nUpPartCount];
		memset(m_abyUpPartStatus,0,m_nUpPartCount);
	}

	else{
		if (tempreqfile->GetED2KPartCount() != nED2KUpPartCount){
			m_nUpPartCount = 0;
			return;
		}
		m_nUpPartCount = tempreqfile->GetPartCount();
		m_abyUpPartStatus = new uint8[m_nUpPartCount];
		uint16 done = 0;
		while (done != m_nUpPartCount){
			uint8 toread;
			data.Read(&toread,1);
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
		if (/*(GetExtendedRequestsVersion() > 1) && */(data.GetLength() - data.GetPosition() > 1))
		{
			uint16 nCount;
			data.Read(&nCount,2);
			SetUpCompleteSourcesCount(nCount);
		}
	}
	//MORPH START - Changed by SiRoB, HotFix Due to Complete Source Feature
	//tempreqfile->NewAvailPartsInfo();
	if(!tempreqfile->IsPartFile())
		tempreqfile->NewAvailPartsInfo();
	else
		((CPartFile*)tempreqfile)->NewSrcPartsInfo();
	//MORPH END   - Changed by SiRoB, HotFix Due to Complete Source Feature
	
	theApp.emuledlg->transferwnd.queuelistctrl.RefreshClient(this);
}


// -khaos--+++> Added new parameter: bool bFromPF
//MORPH START - Modified by SiRoB, ZZ Upload system 20030818-1923
uint64 CUpDownClient::CreateStandartPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF){
    uint64 totalBytes = 0;
//MORPH END   - Modified by SiRoB, ZZ Upload system 20030818-1923
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
		memcpy(&packet->pBuffer[16],&statpos,4);
		uint32 endpos = (currentblock->EndOffset - togo);
		memcpy(&packet->pBuffer[20],&endpos,4);
		memfile.Read(&packet->pBuffer[24],nPacketSize);
		//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
		//m_BlockSend_queue.AddTail(packet);
		totalBytes += packet->GetRealPacketSize();

		// put packet directly on socket
		socket->SendPacket(packet,true,false, nPacketSize);
		//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	}
	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	return totalBytes;
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
}

// -khaos--+++> Added new parameter: bool bFromPF
//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
uint64 CUpDownClient::CreatePackedPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF){
    uint64 totalBytes = 0;
//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	// <-----khaos-
	BYTE* output = new BYTE[togo+300];
	uLongf newsize = togo+300;
	uint16 result = compress2(output,&newsize,data,togo,9);
	if (result != Z_OK || togo <= newsize){
		delete[] output;
		//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
		// -khaos--+++>  Our new boolean...
		totalBytes = CreateStandartPackets(data,togo,currentblock,bFromPF);
		// <-----khaos-
		return totalBytes;
		//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
	}
	m_bUsedComprUp = true;
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
		memcpy(&packet->pBuffer[16],&statpos,4);
		memcpy(&packet->pBuffer[20],&newsize,4);
		memfile.Read(&packet->pBuffer[24],nPacketSize);
		//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
		//m_BlockSend_queue.AddTail(packet);
		totalBytes += packet->GetRealPacketSize();

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
	//MORPH START - Added by SiRoB, ZZ Upload System 20030818-1923
	return totalBytes;
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030818-1923
}

void CUpDownClient::SetUploadFileID(uchar* tempreqfileid){
	CKnownFile* newreqfile = NULL;
	if( tempreqfileid )
		newreqfile = theApp.sharedfiles->GetFileByID(tempreqfileid);
	CKnownFile* oldreqfile = theApp.sharedfiles->GetFileByID(requpfileid);
	if(newreqfile == oldreqfile)
		return;
	if(newreqfile){
		newreqfile->AddQueuedCount();
		newreqfile->AddUploadingClient(this);
		md4cpy(requpfileid,tempreqfileid);
	}
	else{
		md4clr(requpfileid);
	}
	if(oldreqfile){
		oldreqfile->SubQueuedCount();
		oldreqfile->RemoveUploadingClient(this);
	}
}


void CUpDownClient::AddReqBlock(Requested_Block_Struct* reqblock){

	for (POSITION pos = m_DoneBlocks_list.GetHeadPosition();pos != 0;m_DoneBlocks_list.GetNext(pos)){
		if (reqblock->StartOffset == m_DoneBlocks_list.GetAt(pos)->StartOffset && reqblock->EndOffset == m_DoneBlocks_list.GetAt(pos)->EndOffset){
			delete reqblock;
			return;
		}
	}
	for (POSITION pos = m_BlockRequests_queue.GetHeadPosition();pos != 0;m_BlockRequests_queue.GetNext(pos)){
		if (reqblock->StartOffset == m_BlockRequests_queue.GetAt(pos)->StartOffset && reqblock->EndOffset == m_BlockRequests_queue.GetAt(pos)->EndOffset){
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
	    theApp.glob_prefs->Add2SessionTransferData(GetClientSoft(), GetUserPort(), false, true, sentBytesCompleteFile, (IsFriend()&& GetFriendSlot()));
	    theApp.glob_prefs->Add2SessionTransferData(GetClientSoft(), GetUserPort(), true, true, sentBytesPartFile, (IsFriend()&& GetFriendSlot()));
	    m_nTransferedUp += sentBytesCompleteFile + sentBytesPartFile;
//Give more credits to rare files uploaders [Yun.SF3]
		CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);//check this if download completion problems occurs [Yun.SF3]
		if (currequpfile->m_nVirtualCompleteSourcesCountMin && theApp.glob_prefs->IsBoostLess())
        credits->AddUploaded((sentBytesCompleteFile + sentBytesPartFile)/currequpfile->m_nVirtualCompleteSourcesCountMin, GetIP());
		else
        credits->AddUploaded(sentBytesCompleteFile + sentBytesPartFile, GetIP());
//Give more credits to rare files uploaders [Yun.SF3]

        sentBytesPayload = socket->GetSentPayloadSinceLastCallAndReset();
        m_nCurQueueSessionPayloadUp += sentBytesPayload;

        if(GetUploadState() == US_UPLOADING) {
            bool useChunkLimit = false; // PENDING: Get from prefs, or enforce?

            bool wasRemoved = false;
            if(useChunkLimit == false && GetQueueSessionPayloadUp() > SESSIONAMOUNT && curTick-m_dwLastCheckedForEvictTick >= 5) {
                m_dwLastCheckedForEvictTick = curTick;
                wasRemoved = theApp.uploadqueue->RemoveOrMoveDown(this, true);
            }

            if(wasRemoved == false && GetQueueSessionPayloadUp()/SESSIONAMOUNT > m_curSessionAmountNumber) {
                // Should we end this upload?

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
        theApp.emuledlg->transferwnd.uploadlistctrl.RefreshClient(this);
        theApp.emuledlg->transferwnd.clientlistctrl.RefreshClient(this);
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
	data.Write(file->GetFileHash(),16);
	uint16 parts = file->GetHashCount();
	data.Write(&parts,2);
	for (int i = 0; i < parts; i++)
		data.Write(file->GetPartHash(i),16);
	Packet* packet = new Packet(&data);
	packet->opcode = OP_HASHSETANSWER;
	theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet,true,true);
}

void CUpDownClient::ClearUploadBlockRequests(){
	FlushSendBlocks();
	for (POSITION pos = m_BlockRequests_queue.GetHeadPosition();pos != 0;m_BlockRequests_queue.GetNext(pos))
		delete m_BlockRequests_queue.GetAt(pos);
	m_BlockRequests_queue.RemoveAll();
	
	for (POSITION pos = m_DoneBlocks_list.GetHeadPosition();pos != 0;m_DoneBlocks_list.GetNext(pos))
		delete m_DoneBlocks_list.GetAt(pos);
	m_DoneBlocks_list.RemoveAll();
}

//MORPH START - Added by Yun.SF3, ZZ Upload System
void CUpDownClient::ClearUploadDoneBlocks() {
	for (POSITION pos = m_DoneBlocks_list.GetHeadPosition();pos != 0;m_DoneBlocks_list.GetNext(pos))
		delete m_DoneBlocks_list.GetAt(pos);
	m_DoneBlocks_list.RemoveAll();
}
//MORPH END   - Added by Yun.SF3, ZZ Upload System

void CUpDownClient::SendRankingInfo(){
	if (!ExtProtocolAvailable())
		return;
	uint16 nRank = theApp.uploadqueue->GetWaitingPosition(this);
	if (!nRank)
		return;
	Packet* packet = new Packet(OP_QUEUERANKING,12,OP_EMULEPROT);
	memset(packet->pBuffer,0,12);
	memcpy(packet->pBuffer+0,&nRank,2);
	// START enkeyDev(th1) -EDT-
	if (GetDownloadTimeVersion()) {
		uint32 avg_value;
		uint32 err_value;
		EstimateDownloadTime(avg_value, err_value);
		memcpy(packet->pBuffer+4,&avg_value,4);
		memcpy(packet->pBuffer+8,&err_value,4);
	}
	// END enkeyDev(th1) -EDT-
	theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true,true);
}

void CUpDownClient::SendCommentInfo(CKnownFile *file) {
	if (!m_bCommentDirty || file == NULL || !ExtProtocolAvailable() || m_byAcceptCommentVer < 1)
		return;
	m_bCommentDirty = false;

	int8 rating=file->GetFileRate();
	CString desc=file->GetFileComment();
	if(file->GetFileRate() == 0 && desc.IsEmpty())
		return;

	CSafeMemFile data(256);
	data.Write(&rating,sizeof(rating));
	int length=desc.GetLength();
	if (length>128)
		length=128;
	data.Write(&length,sizeof(length));
	if (length>0)
		data.Write(desc.GetBuffer(),length);
	Packet *packet = new Packet(&data,OP_EMULEPROT);
	packet->opcode = OP_FILEDESC;
	theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
	socket->SendPacket(packet,true);
}

void  CUpDownClient::AddRequestCount(uchar* fileid){
	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition();pos != 0;m_RequestedFiles_list.GetNext(pos)){
		Requested_File_Struct* cur_struct = m_RequestedFiles_list.GetAt(pos);
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
	memset(new_struct,0,sizeof(Requested_File_Struct));
	md4cpy(new_struct->fileid,fileid);
	new_struct->lastasked = ::GetTickCount();
	m_RequestedFiles_list.AddHead(new_struct);
}

void  CUpDownClient::UnBan(){
	theApp.clientlist->AddTrackClient(this);
	theApp.clientlist->RemoveBannedClient( GetIP() );
	SetUploadState(US_NONE);
	ClearWaitStartTime();
	theApp.emuledlg->transferwnd.ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition();pos != 0;m_RequestedFiles_list.GetNext(pos)){
		Requested_File_Struct* cur_struct = m_RequestedFiles_list.GetAt(pos);
		cur_struct->badrequests = 0;
		cur_struct->lastasked = 0;	
	}
//	theApp.emuledlg->transferwnd.queuelistctrl.RefreshClient(this, true, true);
}

void CUpDownClient::Ban(){
	theApp.clientlist->AddTrackClient(this);
	if ( !IsBanned() ){
		AddDebugLogLine(false,GetResString(IDS_CLIENTBLOCKED),GetUserName());
	}
#ifdef _DEBUG
	else
		AddDebugLogLine(false,"Ban refreshed for %s (%s) ", GetUserName(), GetFullIP() );
#endif
	theApp.clientlist->AddBannedClient( GetIP() );
	//theApp.uploadqueue->UpdateBanCount();
	theApp.emuledlg->transferwnd.ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	theApp.emuledlg->transferwnd.queuelistctrl.RefreshClient(this);

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
	theApp.emuledlg->transferwnd.ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	theApp.emuledlg->transferwnd.queuelistctrl.RefreshClient(this);
	if (log_message)
		theApp.emuledlg->AddDebugLogLine(false,GetResString(IDS_ANTILEECHERLOG),GetUserName());
}
//MORPH END   - Added by IceCream, Anti-leecher feature


void CUpDownClient::UDPFileReasked(){
	AddAskedCount();
	SetLastUpRequest();
	SetLastL2HACExecution(); //<<-- enkeyDEV(th1) -L2HAC-
	uint16 nRank = theApp.uploadqueue->GetWaitingPosition(this);
//	Packet* response = new Packet(OP_REASKACK,2,OP_EMULEPROT); // enkeyDev(th1) original, commented
//	memcpy(response->pBuffer,&nRank,2); // enkeyDev(th1) original, commented
	// START enkeyDev(th1) -EDT-
	Packet* response;
	if (GetDownloadTimeVersion()) {
		response = new Packet(OP_REASKACK,12,OP_EMULEPROT);
		memset(response->pBuffer,0,12);
		memcpy(response->pBuffer,&nRank,2);
		uint32 avg_value;
		uint32 err_value;
		EstimateDownloadTime(avg_value, err_value);
		memcpy(response->pBuffer+4,&avg_value,4);
		memcpy(response->pBuffer+8,&err_value,4);
	}
	else
	{
		response = new Packet(OP_REASKACK,2,OP_EMULEPROT);
	memcpy(response->pBuffer,&nRank,2);
	}
	// END enkeyDev(th1) -EDT-
	theApp.uploadqueue->AddUpDataOverheadFileRequest(response->size);
	theApp.clientudp->SendPacket(response,GetIP(),GetUDPPort());
}

uint32 CUpDownClient::GetWaitStartTime(){
	if (credits == NULL){
		ASSERT ( false );
		return 0;
	}
	uint32 dwResult = credits->GetSecureWaitStartTime(GetIP());
	if (dwResult > m_dwUploadTime && IsDownloading()){
		//this happens only if two clients with invalid securehash are in the queue - if at all
		dwResult = m_dwUploadTime-1;

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

bool CUpDownClient::GetFriendSlot(){
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
// START enkeyDev(th1) -EDT-
void CUpDownClient::EstimateDownloadTime(uint32 &avg_time, uint32 &err_time)
{
	avg_time = 0;
	err_time = EDT_UNDEFINED;
	if (!IsDownloading())
		theApp.m_edt.EstimateTime(this, avg_time, err_time);
}
// END enkeyDev(th1) -EDT-