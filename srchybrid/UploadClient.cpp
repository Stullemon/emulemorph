//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "UrlClient.h"
#include "Opcodes.h"
#include "Packets.h"
#include "UploadQueue.h"
#include "Statistics.h"
#include "ClientList.h"
#include "ClientUDPSocket.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "PartFile.h"
#include "ClientCredits.h"
#include "ListenSocket.h"
#include "PeerCacheSocket.h"
#include "Sockets.h"
#include "OtherFunctions.h"
#include "SafeFile.h"
#include "DownloadQueue.h"
#include "emuledlg.h"
#include "TransferWnd.h"
#include "Log.h"
#include "WebCache\WebCacheSocket.h" // yonatan http // MORPH - Added by Commander, WebCache 1.2e

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
	COLORREF crNeither;
	COLORREF crNextSending;
	COLORREF crBoth;
	COLORREF crSending;
	COLORREF crBuffer;
	//MORPH START - Added by SiRoB, See chunk that we hide
	COLORREF crHiddenPartBySOTN;
	COLORREF crHiddenPartByHideOS;
	//MORPH END   - Added by SiRoB, See chunk that we hide

    if(GetSlotNumber() <= theApp.uploadqueue->GetActiveUploadsCount() ||
       (GetUploadState() != US_UPLOADING && GetUploadState() != US_CONNECTING) ) {
        crNeither = RGB(224, 224, 224);
	    crNextSending = RGB(255,208,0);
	    crBoth = bFlat ? RGB(0, 0, 0) : RGB(104, 104, 104);
	    crSending = RGB(0, 150, 0);
		crBuffer = RGB(255, 100, 100);
		//MORPH START - Added by SiRoB, See chunk that we hide
		crHiddenPartBySOTN = RGB(192, 96, 255);
		crHiddenPartByHideOS = RGB(96, 192, 255);
		//MORPH END   - Added by SiRoB, See chunk that we hide

    } else {
        // grayed out
        crNeither = RGB(248, 248, 248);
	    crNextSending = RGB(255,244,191);
	    crBoth = bFlat ? RGB(191, 191, 191) : RGB(191, 191, 191);
	    crSending = RGB(191, 229, 191);
		crBuffer = RGB(255, 216, 216);
		//MORPH START - Added by SiRoB, See chunk that we hide
		crHiddenPartBySOTN = RGB(224, 128, 255);
		crHiddenPartByHideOS = RGB(128, 224, 255);
		//MORPH END   - Added by SiRoB, See chunk that we hide
    }

	// wistily: UpStatusFix
	CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
	uint32 filesize;
	if (currequpfile)
		filesize=currequpfile->GetFileSize();
	else
		filesize=PARTSIZE*m_nUpPartCount;
	// wistily: UpStatusFix

    if(filesize > 0) {
		s_UpStatusBar.SetFileSize(filesize); 
		s_UpStatusBar.SetHeight(rect->bottom - rect->top); 
		s_UpStatusBar.SetWidth(rect->right - rect->left); 
		s_UpStatusBar.Fill(crNeither); 
		if (!onlygreyrect && m_abyUpPartStatus && currequpfile) { 
			uint32 i;
			for (i = 0;i < m_nUpPartCount;i++)
				if(m_abyUpPartStatus[i])
					s_UpStatusBar.FillRange(PARTSIZE*(i),PARTSIZE*(i+1),crBoth);
			//MORPH START - Added by SiRoB, See chunk that we hide
				else if (m_abyUpPartStatusHidden)
					if (m_abyUpPartStatusHidden[i])
						s_UpStatusBar.FillRange(PARTSIZE*(i),PARTSIZE*(i+1),m_bUpPartStatusHiddenBySOTN?crHiddenPartBySOTN:crHiddenPartByHideOS);
			//MORPH END   - Added by SiRoB, See chunk that we hide
			for (i;i < currequpfile->GetED2KPartCount();i++)
			//MORPH START - Added by SiRoB, See chunk that we hide
				 if (m_abyUpPartStatusHidden)
					if (m_abyUpPartStatusHidden[i])
						s_UpStatusBar.FillRange(PARTSIZE*(i),PARTSIZE*(i+1),m_bUpPartStatusHiddenBySOTN?crHiddenPartBySOTN:crHiddenPartByHideOS);
			//MORPH END   - Added by SiRoB, See chunk that we hide
			
		}
	    const Requested_Block_Struct* block;
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
			for(POSITION pos=m_DoneBlocks_list.GetHeadPosition();pos!=0;){
				block = m_DoneBlocks_list.GetNext(pos);
				s_UpStatusBar.FillRange(block->StartOffset, block->EndOffset + 1, crSending);
			}

            // Also show what data is buffered (with color crBuffer)
            uint32 total = 0;
    
		    for(POSITION pos=m_DoneBlocks_list.GetTailPosition();pos!=0; ){
			    Requested_Block_Struct* block = m_DoneBlocks_list.GetPrev(pos);
    
                if(total + (block->EndOffset-block->StartOffset) <= GetQueueSessionPayloadUp()) {
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
                else{
                    // entire block is still in buffer
                    total += block->EndOffset-block->StartOffset;
    			    s_UpStatusBar.FillRange(block->StartOffset, block->EndOffset, crBuffer);
                }
		    }
	    }
   	    s_UpStatusBar.Draw(dc, rect->left, rect->top, bFlat);
	}
} 

void CUpDownClient::SetUploadState(EUploadState eNewState)
{
	if (eNewState != m_nUploadState)
	{
		if (m_nUploadState == US_UPLOADING)
		{
			m_nUpDatarate = 0;
			m_nSumForAvgUpDataRate = 0;
			m_AvarageUDR_list.RemoveAll();
		}
		// don't add any final cleanups for US_NONE here	
		m_nUploadState = eNewState;
		theApp.emuledlg->transferwnd->clientlistctrl.RefreshClient(this);
	}
}

/**
 * Gets the queue score multiplier for this client, taking into consideration client's credits
 * and the requested file's priority.
*/
double CUpDownClient::GetCombinedFilePrioAndCredit() {
	if (credits == 0){
		ASSERT ( IsKindOf(RUNTIME_CLASS(CUrlClient)) );
		return 0;
	}

	//Morph Start - added by AndCycle, Equal Chance For Each File
	if (thePrefs.IsEqualChanceEnable())
	{
		CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
		if(currequpfile)
			return currequpfile->statistic.GetEqualChanceValue();
	}
	//Morph End - added by AndCycle, Equal Chance For Each File

    return (uint32)(10.0f*credits->GetScoreRatio(GetIP())*float(GetFilePrioAsNumber()));
}

/**
* Gets the file multiplier for the file this client has requested.
*/
int CUpDownClient::GetFilePrioAsNumber() const {
	CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
	if(!currequpfile)
		return 0;

	// TODO coded by tecxx & herbert, one yet unsolved problem here:
	// sometimes a client asks for 2 files and there is no way to decide, which file the 
	// client finally gets. so it could happen that he is queued first because of a 
	// high prio file, but then asks for something completely different.
	int filepriority = 10; // standard
	//Morph Start - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	if (theApp.clientcredits->IsSaveUploadQueueWaitTime()){
		switch(currequpfile->GetUpPriority()){
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
		} 
	}
	else{
	//Morph End - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
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
	} 	//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	return filepriority;
}

/**
 * Gets the current waiting score for this client, taking into consideration waiting
 * time, priority of requested file, and the client's credits.
 */
uint32 CUpDownClient::GetScore(bool sysvalue, bool isdownloading, bool onlybasevalue) const
{
	if (!m_pszUsername)
		return 0;

	if (IsProxy())  // JP Proxies don't have credits
		return 0;	// JP Proxies don't have credits

	if (credits == 0){
		ASSERT ( IsKindOf(RUNTIME_CLASS(CUrlClient)) );
		return 0;
	}
	CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
	if(!currequpfile)
		return 0;
	
	// bad clients (see note in function)
	if (credits->GetCurrentIdentState(GetIP()) == IS_IDBADGUY)
		return 0;
	// friend slot
	if (IsFriend() && GetFriendSlot() && !HasLowID())
		return 0x0FFFFFFF;

	if (IsBanned() || m_bGPLEvildoer)
		return 0;

	if (sysvalue && HasLowID() && !(socket && socket->IsConnected())){
		return 0;
	}

	int filepriority = GetFilePrioAsNumber();
	
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
//Morph Start- modifed by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
		// Moonlight: SUQWT - I'm exploiting negative overflows to adjust wait start times. Overflows should not be an issue as long
		// as queue turnover rate is faster than 49 days.
		// ASSERT ( m_dwUploadTime-GetWaitStartTime() >= 0 ); //oct 28, 02: changed this from "> 0" to ">= 0"//original commented out
//Morph End- modifed by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)		
		fBaseValue += (float)(::GetTickCount() - m_dwUploadTime > 900000)? 900000:1800000;
		fBaseValue /= 1000;
	}
	if(thePrefs.UseCreditSystem())
	{
		float modif = credits->GetScoreRatio(GetIP());
		fBaseValue *= modif;
	}
        // MORPH START - Added by Commander, WebCache 1.2e
        // Superlexx - TPS - reward clients using port 80
	if(thePrefs.IsWebCacheDownloadEnabled() // only if we have webcache downloading on
		&& SupportsWebCache()				// and if the remote client supports webcache
		&& thePrefs.WebCacheIsTransparent()	// our proxy is transparent
		&& GetUserPort() == 80				// remote client uses port 80
		&& !HasLowID())						// remote client has HighID
		fBaseValue *= (float)1.2;

//	JP Webcache release START
// boost clients if webcache upload will likely result in 3 or more proxy-downloads
	if (SupportsWebCache() 
		&& GetWebCacheName() != _T("") 
		&& thePrefs.IsWebcacheReleaseAllowed()
		&& currequpfile->ReleaseViaWebCache)
	{
		uint32 WebCacheClientCounter = currequpfile->GetNumberOfClientsRequestingThisFileUsingThisWebcache(GetWebCacheName(), 10);
		if (WebCacheClientCounter >= 3)
		{
			fBaseValue *= WebCacheClientCounter;
			fBaseValue += 5000;
		}
	}
	//	JP Webcache release END
    // MORPH END - Added by Commander, WebCache 1.2e

	if (!onlybasevalue)
		fBaseValue *= (float(filepriority)/10.0f);

	if( (IsEmuleClient() || this->GetClientSoft() < 10) && m_byEmuleVersion <= 0x19 )
		fBaseValue *= 0.5f;
	return (uint32)fBaseValue;
}

//Morph Start - added by AndCycle, Equal Chance For Each File
double CUpDownClient::GetEqualChanceValue() const
{
	CKnownFile* currentReqFile = theApp.sharedfiles->GetFileByID((uchar*)GetUploadFileID());

	if(currentReqFile != NULL){
		return currentReqFile->statistic.GetEqualChanceValue();
	}
	return 0;
}
//Morph End - added by AndCycle, Equal Chance For Each File

//Morph Start - added by AndCycle, Pay Back First
//Comment : becarefull when changing this function don't forget to change IsPBForPS() 
bool CUpDownClient::IsMoreUpThanDown() const{
	if(!IsSecure()) return false;
	CKnownFile* currentReqFile = theApp.sharedfiles->GetFileByID((uchar*)GetUploadFileID());
	if (currentReqFile == NULL) return false;
	return currentReqFile->IsPartFile()==false && thePrefs.IsPayBackFirst() && credits->GetPayBackFirstStatus();
}
//Morph End - added by AndCycle, Pay Back First

//Morph Start - added by AndCycle, separate secure check
bool CUpDownClient::IsSecure() const
{
	return credits && theApp.clientcredits->CryptoAvailable() && credits->GetCurrentIdentState(GetIP()) == IS_IDENTIFIED;
}
//Morph End - added by AndCycle, separate secure check

//MORPH START - Added by SiRoB, Code Optimization PBForPS()
bool CUpDownClient::IsPBForPS() const
{
	//replacement for return (IsMoreUpThanDown() || GetPowerShared());
	//<--Commun to both call
	if(!IsSecure()) return false;
	CKnownFile* currentReqFile = theApp.sharedfiles->GetFileByID(GetUploadFileID());
	if (currentReqFile == NULL)
		return false;
	//-->Commun to both call
	if (currentReqFile->GetPowerShared())
		return true;
	if(currentReqFile->IsPartFile()==false && thePrefs.IsPayBackFirst() && credits->GetPayBackFirstStatus())
		return true;
	return false;
}
//MORPH END   - Added by SiRoB, Code Optimization PBForPS()

//MORPH START - Added by Yun.SF3, ZZ Upload System
/**
* Checks if the file this client has requested has release priority.
*
* @return true if the requested file has release priority
*/
bool CUpDownClient::GetPowerShared() const {
//Comment : becarefull when changing this function don't forget to change IsPBForPS() 
	if(!IsSecure()) return false;
	CKnownFile* currentReqFile = theApp.sharedfiles->GetFileByID(GetUploadFileID());
	if (currentReqFile != NULL && currentReqFile->GetPowerShared())
		return true;
	return false;
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
       m_addedPayloadQueueSession > GetQueueSessionPayloadUp() && m_addedPayloadQueueSession-GetQueueSessionPayloadUp() > 50*1024) { // the buffered data is large enough allready
		return;
	}

	CFile file;
	byte* filedata = 0;
	CString fullname;
	bool bFromPF = true; // Statistic to breakdown uploaded data by complete file vs. partfile.
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
				fullname.Format(_T("%s\\%s"),srcfile->GetPath(),srcfile->GetFileName());
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
				bFromPF = false; // This is not a part file...
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

			// MORPH START - Added by Commander, WebCache 1.2e
			if (IsUploadingToWebCache()) // Superlexx - encryption: encrypt here
			{
				Crypt.RefreshLocalKey();
				Crypt.encryptor.SetKey(Crypt.localKey, WC_KEYLENGTH);
				Crypt.encryptor.DiscardBytes(16); // we must throw away 16 bytes of the key stream since they were already used once, 16 is the file hash length
				Crypt.encryptor.ProcessString(filedata, togo);
			}
			// MORPH END - Added by Commander, WebCache 1.2e
			// check extention to decide whether to compress or not
			CString ext = srcfile->GetFileName();
			ext.MakeLower();
			int pos = ext.ReverseFind(_T('.'));
			if (pos>-1)
				ext = ext.Mid(pos);
			bool compFlag = (ext!=_T(".zip") && ext!=_T(".rar") && ext!=_T(".ace") && ext!=_T(".ogm") && ext!=_T(".tar"));//no need to try compressing tar compressed files... [Yun.SF3]
			if (ext==_T(".avi") && thePrefs.GetDontCompressAvi())
				compFlag=false;

			// MORPH START - Modified by Commander, WebCache 1.2e
			/*
			if (!IsUploadingToPeerCache() && m_byDataCompVer == 1 && compFlag)
			*/
			if (!IsUploadingToPeerCache() && !IsUploadingToWebCache() && m_byDataCompVer == 1 && compFlag) // yonatan http
			// MORPH END - Modified by Commander, WebCache 1.2e
				CreatePackedPackets(filedata,togo,currentblock,bFromPF);
			else
				CreateStandartPackets(filedata,togo,currentblock,bFromPF);
			// <-----khaos-
			
			// file statistic
			//MORPH START - Changed by IceCream SLUGFILLER: Spreadbars
			/*
			srcfile->statistic.AddTransferred(togo);
			*/
			srcfile->statistic.AddTransferred(currentblock->StartOffset, togo);
			//MORPH END - Changed by IceCream SLUGFILLER: Spreadbars
			m_addedPayloadQueueSession += togo;

			m_DoneBlocks_list.AddHead(m_BlockRequests_queue.RemoveHead());
			delete[] filedata;
			filedata = 0;
		}
	}
	catch(CString error)
	{
		if (thePrefs.GetVerbose())
			DebugLogWarning(GetResString(IDS_ERR_CLIENTERRORED), GetUserName(), error);
		theApp.uploadqueue->RemoveFromUploadQueue(this, _T("Client error: ") + error);
		delete[] filedata;
		return;
	}
	catch(CFileException* e)
	{
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		e->GetErrorMessage(szError, ARRSIZE(szError));
		if (thePrefs.GetVerbose())
			DebugLogWarning(_T("Failed to create upload package for %s - %s"), GetUserName(), szError);
		theApp.uploadqueue->RemoveFromUploadQueue(this, ((CString)_T("Failed to create upload package.")) + szError);
		delete[] filedata;
		e->Delete();
		return;
	}
}

void CUpDownClient::ProcessExtendedInfo(CSafeMemFile* data, CKnownFile* tempreqfile)
{
	if (m_abyUpPartStatus) 
	{
		delete[] m_abyUpPartStatus;
		m_abyUpPartStatus = NULL;	// added by jicxicmic
	}
	//MORPH START - Added by SiRoB, See chunk that we hide
	if (m_abyUpPartStatusHidden)
	{
		delete[] m_abyUpPartStatusHidden;
		m_abyUpPartStatusHidden = NULL;
	}
	//MORPH END   - Added by SiRoB, See chunk that we hide
	m_nUpPartCount = 0;
	m_nUpCompleteSourcesCount= 0;
	if( GetExtendedRequestsVersion() == 0 )
		return;
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

void CUpDownClient::CreateStandartPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF){
	uint32 nPacketSize;
	CMemFile memfile((BYTE*)data,togo);
	if (togo > 10240) 
		nPacketSize = togo/(uint32)(togo/10240);
	else
		nPacketSize = togo;
	while (togo){
		if (togo < nPacketSize*2)
			nPacketSize = togo;
		ASSERT( nPacketSize );
		togo -= nPacketSize;

		uint32 statpos = (currentblock->EndOffset - togo) - nPacketSize;
		uint32 endpos = (currentblock->EndOffset - togo);
		if (IsUploadingToPeerCache())
		{
			if (m_pPCUpSocket == NULL){
				ASSERT(0);
				CString strError;
				strError.Format(_T("Failed to upload to PeerCache - missing socket; %s"), DbgGetClientInfo());
				throw strError;
			}
			USES_CONVERSION;
			CSafeMemFile dataHttp(10240);
			if (m_iHttpSendState == 0)
			{
				CKnownFile* srcfile = theApp.sharedfiles->GetFileByID(GetUploadFileID());
				CStringA str;
				str.AppendFormat("HTTP/1.0 206\r\n");
				str.AppendFormat("Content-Range: bytes %u-%u/%u\r\n", currentblock->StartOffset, currentblock->EndOffset - 1, srcfile->GetFileSize());
				str.AppendFormat("Content-Type: application/octet-stream\r\n");
				str.AppendFormat("Content-Length: %u\r\n", currentblock->EndOffset - currentblock->StartOffset);
				str.AppendFormat("Server: eMule/%s\r\n", T2CA(theApp.m_strCurVersionLong));
				str.AppendFormat("\r\n");
				dataHttp.Write((LPCSTR)str, str.GetLength());
				theStats.AddUpDataOverheadFileRequest(dataHttp.GetLength());

				m_iHttpSendState = 1;
				if (thePrefs.GetDebugClientTCPLevel() > 0){
					DebugSend("PeerCache-HTTP", this, (char*)GetUploadFileID());
					Debug(_T("  %hs\n"), str);
				}
			}
			dataHttp.Write(data, nPacketSize);
			data += nPacketSize;

			if (thePrefs.GetDebugClientTCPLevel() > 1){
				DebugSend("PeerCache-HTTP data", this, (char*)GetUploadFileID());
				Debug(_T("  Start=%u  End=%u  Size=%u\n"), statpos, endpos, nPacketSize);
			}

			UINT uRawPacketSize = dataHttp.GetLength();
			LPBYTE pRawPacketData = dataHttp.Detach();
			CRawPacket* packet = new CRawPacket((char*)pRawPacketData, uRawPacketSize, bFromPF);
			m_pPCUpSocket->SendPacket(packet, true, false, nPacketSize);
			free(pRawPacketData);
		}
		// MORPH START - Added by Commander, WebCache 1.2e
		else if (IsUploadingToWebCache())
		{
			if (m_pWCUpSocket == NULL){
				ASSERT(0);
				CString strError;
				strError.Format(_T("Failed to upload to WebCache - missing socket; %s"), DbgGetClientInfo());
				throw strError;
			}
			USES_CONVERSION;
			CSafeMemFile dataHttp(10240);
			if (m_iHttpSendState == 0) // yonatan - not sure it's wise to use this (also used by PC).
			{
				CKnownFile* srcfile = theApp.sharedfiles->GetFileByID(GetUploadFileID());
				CStringA str;
				str.AppendFormat("HTTP/1.1 200\r\n");
				str.AppendFormat("Content-Length: %u\r\n", currentblock->EndOffset - currentblock->StartOffset);
				str.AppendFormat("Cache-Control: public\r\n");
//				str.AppendFormat("Cache-Control: max-age=864000\r\n"); // overrides expires header in HTTP/1.1
				str.AppendFormat("Cache-Control: no-transform\r\n");
				str.AppendFormat("Expires: Mon, 03 Sep 2007 01:23:45 GMT\r\n" ); // rolled-back to 1.1b code (possible bug w/soothsayers' proxy)
				str.AppendFormat("Connection: keep-alive\r\nProxy-Connection: keep-alive\r\n");
				str.AppendFormat("Server: eMule/%s %s\r\n", T2CA(theApp.m_strCurVersionLong), T2CA(MOD_VERSION));
				str.AppendFormat("\r\n");
				dataHttp.Write((LPCSTR)str, str.GetLength());
				theStats.AddUpDataOverheadFileRequest(dataHttp.GetLength());

				m_iHttpSendState = 1;
				if (thePrefs.GetDebugClientTCPLevel() > 0){
					DebugSend("WebCache-HTTP", this, (char*)GetUploadFileID());
					Debug(_T("  %hs\n"), str);
				}
			}
			dataHttp.Write(data, nPacketSize);
			data += nPacketSize;

			if (thePrefs.GetDebugClientTCPLevel() > 1){
				DebugSend("WebCache-HTTP data", this, (char*)GetUploadFileID());
				Debug(_T("  Start=%u  End=%u  Size=%u\n"), statpos, endpos, nPacketSize);
			}

			UINT uRawPacketSize = dataHttp.GetLength();
			LPBYTE pRawPacketData = dataHttp.Detach();
			CRawPacket* packet = new CRawPacket((char*)pRawPacketData, uRawPacketSize, bFromPF);
			m_pWCUpSocket->SendPacket(packet, true, false, nPacketSize);
			free(pRawPacketData);
		}
		// MORPH END - Added by Commander, WebCache 1.2e
		else
		{
			Packet* packet = new Packet(OP_SENDINGPART,nPacketSize+24, OP_EDONKEYPROT, bFromPF);
			md4cpy(&packet->pBuffer[0],GetUploadFileID());
			PokeUInt32(&packet->pBuffer[16], statpos);
			PokeUInt32(&packet->pBuffer[20], endpos);
		
			memfile.Read(&packet->pBuffer[24],nPacketSize);

			if (thePrefs.GetDebugClientTCPLevel() > 0){
				DebugSend("OP__SendingPart", this, (char*)GetUploadFileID());
				Debug(_T("  Start=%u  End=%u  Size=%u\n"), statpos, endpos, nPacketSize);
			}
			// put packet directly on socket
			theStats.AddUpDataOverheadFileRequest(24);
			socket->SendPacket(packet,true,false, nPacketSize);
		}
	}
}

void CUpDownClient::CreatePackedPackets(byte* data,uint32 togo, Requested_Block_Struct* currentblock, bool bFromPF){
	BYTE* output = new BYTE[togo+300];
	uLongf newsize = togo+300;
	uint16 result = compress2(output,&newsize,data,togo,9);
	if (result != Z_OK || togo <= newsize){
		delete[] output;
		CreateStandartPackets(data,togo,currentblock,bFromPF);
		return;
	}

	// khaos::kmod+ Show Compression by Tarod
	compressiongain += (togo-newsize);
	notcompressed += togo;
	// khaos::kmod-
	CMemFile memfile(output,newsize);
	uint32 oldSize = togo;
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
		ASSERT( nPacketSize );
		togo -= nPacketSize;
		Packet* packet = new Packet(OP_COMPRESSEDPART,nPacketSize+24,OP_EMULEPROT,bFromPF);
		md4cpy(&packet->pBuffer[0],GetUploadFileID());
		uint32 statpos = currentblock->StartOffset;
		PokeUInt32(&packet->pBuffer[16], statpos);
		PokeUInt32(&packet->pBuffer[20], newsize);
		memfile.Read(&packet->pBuffer[24],nPacketSize);

		if (thePrefs.GetDebugClientTCPLevel() > 0){
			DebugSend("OP__CompressedPart", this, (char*)GetUploadFileID());
			Debug(_T("  Start=%u  BlockSize=%u  Size=%u\n"), statpos, newsize, nPacketSize);
		}
        // approximate payload size
		uint32 payloadSize = nPacketSize*oldSize/newsize;

		if(togo == 0 && totalPayloadSize+payloadSize < oldSize) {
			payloadSize = oldSize-totalPayloadSize;
		}
		totalPayloadSize += payloadSize;

        // put packet directly on socket
		theStats.AddUpDataOverheadFileRequest(24);
		socket->SendPacket(packet,true,false, payloadSize);
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

	// clear old status
	if (m_abyUpPartStatus) 
	{
		delete[] m_abyUpPartStatus;
		m_abyUpPartStatus = NULL;
	}
	m_nUpPartCount = 0;
	m_nUpCompleteSourcesCount= 0;

	if(newreqfile){
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
    if(GetUploadState() != US_UPLOADING) {
        if(thePrefs.GetLogUlDlEvents())
            AddDebugLogLine(DLP_LOW, false, _T("UploadClient: Client tried to add req block when not in upload slot! Prevented req blocks from being added. %s"), DbgGetClientInfo());
		delete reqblock;
        return;
    }

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
	DWORD curTick = ::GetTickCount();

	uint64 sentBytesCompleteFile = 0;
	uint64 sentBytesPartFile = 0;
	uint64 sentBytesPayload = 0;

    if(GetFileUploadSocket() && (m_ePeerCacheUpState != PCUS_WAIT_CACHE_REPLY)) {
		CEMSocket* s = GetFileUploadSocket();

        if(m_pPCUpSocket && IsUploadingToPeerCache()) {
            // Check if filedata has been sent via the normal socket since last call.
            uint64 sentBytesCompleteFileNormalSocket = socket->GetSentBytesCompleteFileSinceLastCallAndReset();
            uint64 sentBytesPartFileNormalSocket = socket->GetSentBytesPartFileSinceLastCallAndReset();

			if(thePrefs.GetVerbose() && (sentBytesCompleteFileNormalSocket + sentBytesPartFileNormalSocket > 0)) {
                AddDebugLogLine(false, _T("Sent file data via normal socket when in PC mode. Bytes: %I64i."), sentBytesCompleteFileNormalSocket + sentBytesPartFileNormalSocket);
			}
        }

        // MORPH START - Added by Commander, WebCache 1.2e
		// Superlexx - 0.44a port attempt
		if(m_pWCUpSocket && IsUploadingToWebCache()) {
            // Check if filedata has been sent via the normal socket since last call.
            uint64 sentBytesCompleteFileNormalSocket = socket->GetSentBytesCompleteFileSinceLastCallAndReset();
            uint64 sentBytesPartFileNormalSocket = socket->GetSentBytesPartFileSinceLastCallAndReset();

			if(thePrefs.GetVerbose() && (sentBytesCompleteFileNormalSocket + sentBytesPartFileNormalSocket > 0)) {
                AddDebugLogLine(false, _T("Sent file data via normal socket when in WC mode. Bytes: %I64i."), sentBytesCompleteFileNormalSocket + sentBytesPartFileNormalSocket);
			}
        }
    	// MORPH END - Added by Commander, WebCache 1.2e

	    // Extended statistics information based on which client software and which port we sent this data to...
	    // This also updates the grand total for sent bytes, etc.  And where this data came from.
        sentBytesCompleteFile = s->GetSentBytesCompleteFileSinceLastCallAndReset();
		sentBytesPartFile = s->GetSentBytesPartFileSinceLastCallAndReset();
		thePrefs.Add2SessionTransferData(GetClientSoft(), GetUserPort(), false, true, sentBytesCompleteFile, (IsFriend()&& GetFriendSlot()));
		thePrefs.Add2SessionTransferData(GetClientSoft(), GetUserPort(), true, true, sentBytesPartFile, (IsFriend()&& GetFriendSlot()));

		m_nTransferedUp += sentBytesCompleteFile + sentBytesPartFile;
		credits->AddUploaded(sentBytesCompleteFile + sentBytesPartFile, GetIP()); 

		sentBytesPayload = s->GetSentPayloadSinceLastCallAndReset();
		m_nCurQueueSessionPayloadUp += sentBytesPayload;

        if(GetUploadState() == US_UPLOADING) {
            bool wasRemoved = false;
            if(GetQueueSessionPayloadUp() > SESSIONMAXTRANS+20*1024 && curTick-m_dwLastCheckedForEvictTick >= 5*1000) {
                m_dwLastCheckedForEvictTick = curTick;
                wasRemoved = theApp.uploadqueue->RemoveOrMoveDown(this, true);
            }
			//The main reason for this is that if we put the client back on queue and it goes
            if(wasRemoved == false && GetQueueSessionPayloadUp() > GetCurrentSessionLimit()) {
                // Should we end this upload?
				//EastShare Start - added by AndCycle, Pay Back First
				//check again does client satisfy the conditions
				credits->InitPayBackFirstStatus();
				//EastShare End - added by AndCycle, Pay Back First

                // first clear the average speed, to show ?? as speed in upload slot display
                m_AvarageUDR_list.RemoveAll();
                m_nSumForAvgUpDataRate = 0;

	            // Give clients in queue a chance to kick this client out.
                // It will be kicked out only if queue contains a client
                // of same/higher class as this client, and that new
                // client must either be a high ID client, or a low ID
                // client that is currently connected.
                wasRemoved = theApp.uploadqueue->RemoveOrMoveDown(this);
			//the downloader didn't send any reqeust blocks. Then the connection times out..
                if(!wasRemoved) {
                    // It wasn't removed, so it is allowed to pass into the next amount.
                    m_curSessionAmountNumber++;
                }
            }
			//I did some tests with eDonkey also and it seems to work well with them also..
            if(wasRemoved) {
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					DebugSend("OP__OutOfPartReqs", this);
				Packet* pCancelTransferPacket = new Packet(OP_OUTOFPARTREQS, 0);
				theStats.AddUpDataOverheadFileRequest(pCancelTransferPacket->size);
				socket->SendPacket(pCancelTransferPacket,true,true);
            	theApp.uploadqueue->AddClientToQueue(this,true);
        	} 
			else {
    	        // read blocks from file and put on socket
    	        CreateNextBlockPackage();
    	    }
    	}
    }

	//MORPH START - Modified by SiRoB, Better Upload rate calcul
	if(sentBytesCompleteFile + sentBytesPartFile > 0) {
		// Store how much data we've transfered this round,
		// to be able to calculate average speed later
		// keep sum of all values in list up to date
		TransferredData newitem = {sentBytesCompleteFile + sentBytesPartFile, curTick};
		m_AvarageUDR_list.AddTail(newitem);
		m_nSumForAvgUpDataRate += sentBytesCompleteFile + sentBytesPartFile;
	}
			
	while (m_AvarageUDR_list.GetCount() > 0)
		if ((curTick - m_AvarageUDR_list.GetHead().timestamp) > 10000) {
			m_nSumForAvgUpDataRate -=  m_AvarageUDR_list.RemoveHead().datalen;
		}else
			break;
    if(m_AvarageUDR_list.GetCount() > 0) {
		if(m_AvarageUDR_list.GetCount() == 1)
			m_nUpDatarate = ((ULONGLONG)m_nSumForAvgUpDataRate*1000) / 10000;
		else {
			DWORD dwDuration = m_AvarageUDR_list.GetTail().timestamp - m_AvarageUDR_list.GetHead().timestamp;
			if ((m_AvarageUDR_list.GetCount() - 1)*(curTick - m_AvarageUDR_list.GetTail().timestamp) >= dwDuration)
				dwDuration = curTick - m_AvarageUDR_list.GetHead().timestamp - dwDuration / (m_AvarageUDR_list.GetCount() - 1);
			if (dwDuration < 5000) dwDuration = 5000;
			m_nUpDatarate = ((ULONGLONG)(m_nSumForAvgUpDataRate - m_AvarageUDR_list.GetHead().datalen)*1000) / dwDuration;
		}
	} else {
   	    m_nUpDatarate = 0;
	}
	//MORPH END   - Modified by SiRoB, Better Upload rate calcul
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
	if (socket)      //socket may be NULL...
		socket->TruncateQueues();
}

void CUpDownClient::SendHashsetPacket(char* forfileid){
	CKnownFile* file = theApp.sharedfiles->GetFileByID((uchar*)forfileid);
	if (!file){
		CheckFailedFileIdReqs((uchar*)forfileid);
		throw GetResString(IDS_ERR_REQ_FNF) + _T(" (SendHashsetPacket)");
	}

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
	theStats.AddUpDataOverheadFileRequest(packet->size);
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
	theStats.AddUpDataOverheadFileRequest(packet->size);
	socket->SendPacket(packet,true,true);
}

void CUpDownClient::SendCommentInfo(/*const*/ CKnownFile *file)
{
	if (!m_bCommentDirty || file == NULL || !ExtProtocolAvailable() || m_byAcceptCommentVer < 1)
		return;
	m_bCommentDirty = false;

	uint8 rating = file->GetFileRating();
	const CString& desc = file->GetFileComment();
	if (file->GetFileRating() == 0 && desc.IsEmpty())
		return;

	CSafeMemFile data(256);
	data.WriteUInt8(rating);
	data.WriteLongString(desc, GetUnicodeSupport());
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__FileDesc", this, (char*)file->GetFileHash());
	Packet *packet = new Packet(&data,OP_EMULEPROT);
	packet->opcode = OP_FILEDESC;
	theStats.AddUpDataOverheadFileRequest(packet->size);
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

void  CUpDownClient::UnBan()
{
	theApp.clientlist->AddTrackClient(this);
	theApp.clientlist->RemoveBannedClient( GetIP() );
	SetUploadState(US_NONE);
	ClearWaitStartTime();
	theApp.emuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition();pos != 0;)
	{
		Requested_File_Struct* cur_struct = m_RequestedFiles_list.GetNext(pos);
		cur_struct->badrequests = 0;
		cur_struct->lastasked = 0;	
	}
}

void CUpDownClient::Ban(LPCTSTR pszReason)
{
	theApp.clientlist->AddTrackClient(this);
	// EastShare START - Modified by TAHO, modified SUQWT
	//if(theApp.clientcredits->IsSaveUploadQueueWaitTime()) ClearWaitStartTime();	// Moonlight: SUQWT//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	if(theApp.clientcredits->IsSaveUploadQueueWaitTime()) {
		ClearWaitStartTime();
		if (credits != NULL){
			credits->ClearUploadQueueWaitTime();
		}
	}
	// EastShare END - Modified by TAHO, modified SUQWT
	if ( !IsBanned() ){
		if (thePrefs.GetLogBannedClients())
			AddDebugLogLine(false,_T("Banned: %s; %s"), pszReason==NULL ? _T("Aggressive behaviour") : pszReason, DbgGetClientInfo());
	}
#ifdef _DEBUG
	else{
		if (thePrefs.GetLogBannedClients())
			AddDebugLogLine(false,_T("Banned: (refreshed): %s; %s"), pszReason==NULL ? _T("Aggressive behaviour") : pszReason, DbgGetClientInfo());
	}
#endif
	theApp.clientlist->AddBannedClient( GetIP() );
	SetUploadState(US_BANNED);
	theApp.emuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	theApp.emuledlg->transferwnd->queuelistctrl.RefreshClient(this);
	if (socket != NULL && socket->IsConnected())
		socket->ShutDown(SD_RECEIVE); // let the socket timeout, since we dont want to risk to delete the client right now. This isnt acutally perfect, could be changed later
}

//MORPH START - Added by IceCream, Anti-leecher feature
void CUpDownClient::BanLeecher(LPCTSTR pszReason){
	theApp.clientlist->AddTrackClient(this);
	// EastShare START - Modified by TAHO, modified SUQWT
	//if(theApp.clientcredits->IsSaveUploadQueueWaitTime()) ClearWaitStartTime();	// Moonlight: SUQWT//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	if(theApp.clientcredits->IsSaveUploadQueueWaitTime()) {
		ClearWaitStartTime();
		if (credits != NULL){
			credits->ClearUploadQueueWaitTime();
		}
	}
	// EastShare END - Modified by TAHO, modified SUQWT
	if (!m_bLeecher){
		theStats.leecherclients++;
		m_bLeecher = true;
		AddDebugLogLine(false,GetResString(IDS_ANTILEECHERLOG) + _T(" (%s)"),DbgGetClientInfo(),pszReason==NULL ? _T("No Reason") : pszReason);
	}
	theApp.clientlist->AddBannedClient( GetIP() );
	SetUploadState(US_BANNED);
	theApp.emuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	theApp.emuledlg->transferwnd->queuelistctrl.RefreshClient(this);
	if (socket != NULL && socket->IsConnected())
		socket->ShutDown(SD_RECEIVE); // let the socket timeout, since we dont want to risk to delete the client right now. This isnt acutally perfect, could be changed later
}
//MORPH END   - Added by IceCream, Anti-leecher feature

// Moonlight: SUQWT - Compare linear time instead of time indexes to avoid overflow-induced false positives.//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
//EastShare START - Modified by TAHO, modified SUQWT
//uint32 CUpDownClient::GetWaitStartTime() const
sint64 CUpDownClient::GetWaitStartTime() const
//EastShare END - Modified by TAHO, modified SUQWT
{
	if (credits == NULL){
		ASSERT ( false );
		return 0;
	}

	//EastShare START - Modified by TAHO, modified SUQWT
	//uint32 dwResult = credits->GetSecureWaitStartTime(GetIP());
	sint64 dwResult = credits->GetSecureWaitStartTime(GetIP());
	uint32 now = ::GetTickCount();
	if ( dwResult > now) { 
		dwResult = now - 1;
	}
//MORPH START - Changed by SiRoB, Moonlight's Save Upload Queue Wait Time (MSUQWT)
//	uint32 dwTicks = ::GetTickCount();
//	if ((!theApp.clientcredits->IsSaveUploadQueueWaitTime() && (dwResult > m_dwUploadTime) ||
//		theApp.clientcredits->IsSaveUploadQueueWaitTime() && ((int)(m_dwUploadTime - dwResult) < 0))
//		&& IsDownloading()){
	if (IsDownloading() && (dwResult > m_dwUploadTime)) {
//MORPH END - Changed by SiRoB, Moonlight's Save Upload Queue Wait Time (MSUQWT)
//EastShare END - Modified by TAHO, modified SUQWT
	//this happens only if two clients with invalid securehash are in the queue - if at all
			dwResult = m_dwUploadTime-1;

		if (thePrefs.GetVerbose())
			DEBUG_ONLY(AddDebugLogLine(false,_T("Warning: CUpDownClient::GetWaitStartTime() waittime Collision (%s)"),GetUserName()));
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

CClientReqSocket* CUpDownClient::GetFileUploadSocket(bool log) {
    if(m_pPCUpSocket && (IsUploadingToPeerCache() || m_ePeerCacheUpState == PCUS_WAIT_CACHE_REPLY)) {
        if(thePrefs.GetVerbose() && log)
            AddDebugLogLine(false, _T("%s got peercache socket."), DbgGetClientInfo());

        return m_pPCUpSocket;
    }
// MORPH START - Modified by Commander, WebCache 1.2e
    else if(m_pWCUpSocket && IsUploadingToWebCache()) {
        if(thePrefs.GetVerbose() && log)
            AddDebugLogLine(false, _T("%s got webcache socket."), DbgGetClientInfo());

        return m_pWCUpSocket;
}
	// <-- WebCache
	else {
        if(thePrefs.GetVerbose() && log)
            AddDebugLogLine(false, _T("%s got normal socket."), DbgGetClientInfo());
        return socket;
    }
}
// MORPH END - Modified by Commander, WebCache 1.2e
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
		CString tag = ctemp.Tokenize (_T("|"),p).Trim ();
		if (tag != "") isCom = ntemp.Find (tag) >= 0;
	} while ((!isCom) && (p >= 0));
	return isCom;
}
// [end] Mighty Knife
//MORPH START - Added by SIRoB, GetAverage Upload to client Wistily idea
uint32 CUpDownClient::GetAvUpDatarate() const
{
	uint32 tempUpCurrentTotalTime = GetUpTotalTime();
	if (GetUploadState() == US_UPLOADING)
		tempUpCurrentTotalTime += GetTickCount() - m_dwUploadTime;
	if (tempUpCurrentTotalTime & ~1023)
		return	GetTransferedUp()/(tempUpCurrentTotalTime/1000);
	else
		return 0;
}
//MORPH END  - Added by SIRoB, GetAverage Upload to client
//MORPH START - Added by SiRoB, ShareOnlyTheNeed hide Uploaded and uploading part
void CUpDownClient::GetUploadingAndUploadedPart(uint8* m_abyUpPartUploadingAndUploaded, uint16 partcount)
{
	memset(m_abyUpPartUploadingAndUploaded,0,partcount);
	const Requested_Block_Struct* block;
	if (!m_BlockRequests_queue.IsEmpty()){
		block = m_BlockRequests_queue.GetHead();
		if(block){
			uint32 start = block->StartOffset/PARTSIZE;
			m_abyUpPartUploadingAndUploaded[start] = 1;
		}
	}
	if (!m_DoneBlocks_list.IsEmpty()){
		for(POSITION pos=m_DoneBlocks_list.GetHeadPosition();pos!=0;){
			block = m_DoneBlocks_list.GetNext(pos);
			uint32 start = block->StartOffset/PARTSIZE;
			m_abyUpPartUploadingAndUploaded[start] = 1;
		}
	}
}
//MORPH END   - Added by SiRoB, ShareOnlyTheNeed hide Uploaded and uploading part
