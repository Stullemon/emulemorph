//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "TransferDlg.h"
#include "Log.h"
#include "Collection.h"
#ifndef USE_MORPH_READ_THREAD
#include "UploadDiskIOThread.h"
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
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
	COLORREF crHiddenPartBySOTNandHideOS;
	//MORPH END   - Added by SiRoB, See chunk that we hide
	COLORREF crProgress;
    if(GetSlotNumber() <= theApp.uploadqueue->GetActiveUploadsCount(m_classID) || //MORPH - Upload Splitting Class
       (GetUploadState() != US_UPLOADING && GetUploadState() != US_CONNECTING) ) {
        crNeither = RGB(224, 224, 224);
	    crNextSending = RGB(255,208,0);
	    crBoth = bFlat ? RGB(0, 0, 0) : RGB(104, 104, 104);
	    crSending = RGB(0, 150, 0);
		crBuffer = RGB(255, 100, 100);
		//MORPH START - Added by SiRoB, See chunk that we hide
		crHiddenPartBySOTN = RGB(192, 96, 255);
		crHiddenPartByHideOS = RGB(96, 192, 255);
		crHiddenPartBySOTNandHideOS = RGB(96, 96, 255);
		//MORPH END   - Added by SiRoB, See chunk that we hide
		crProgress = RGB(0, 224, 0);
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
		crHiddenPartBySOTNandHideOS = RGB(128, 128, 255);
		//MORPH END   - Added by SiRoB, See chunk that we hide
		crProgress = RGB(191, 255, 191);
    }

	// wistily: UpStatusFix
	//MORPH - Changed by SiRoB, Optimization requpfile
	/*
	CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
	*/
	CKnownFile* currequpfile = CheckAndGetReqUpFile();
	//MORPH - Changed by SiRoB, Optimization requpfile
	EMFileSize filesize;
	if (currequpfile)
		filesize = currequpfile->GetFileSize();
	else
		filesize = (uint64)(PARTSIZE * (uint64)m_nUpPartCount);
	// wistily: UpStatusFix

    if(filesize > (uint64)0) {
		s_UpStatusBar.SetFileSize(filesize); 
		s_UpStatusBar.SetHeight(rect->bottom - rect->top); 
		s_UpStatusBar.SetWidth(rect->right - rect->left); 
		s_UpStatusBar.Fill(crNeither); 
		//Fafner: start: client percentage - 080429
		// Moved up to be thread safe for check if(m_abyUpPartStatus[i]&SC_XFER)
		UploadingToClient_Struct* pUpClientStruct = theApp.uploadqueue->GetUploadingClientStructByClient(this);
		CSingleLock* lockBlockLists = NULL;
		if (pUpClientStruct != NULL)
		{
			lockBlockLists = new CSingleLock (&pUpClientStruct->m_csBlockListsLock, TRUE);
			ASSERT(lockBlockLists->IsLocked());
		}
		//Fafner: start: client percentage - 080429

		//MORPH START - Changed by SiRoB, See chunk that we hide
		/*
		if (!onlygreyrect && m_abyUpPartStatus) {
			for (UINT i = 0; i < m_nUpPartCount; i++)
				if (m_abyUpPartStatus[i])
					s_UpStatusBar.FillRange(PARTSIZE*(uint64)(i), PARTSIZE*(uint64)(i+1), crBoth);
		*/
		if (!onlygreyrect && m_abyUpPartStatus && currequpfile) {
			UINT i;
			for (i = 0;i < currequpfile->GetPartCount();i++) {
				if (m_abyUpPartStatus[i] == 0)
					continue;
				COLORREF crChunk;
				if (m_abyUpPartStatus[i]&SC_AVAILABLE)
					crChunk = crBoth;
				else if (m_abyUpPartStatus[i]&SC_HIDDENBYSOTN && m_abyUpPartStatus[i]&SC_HIDDENBYHIDEOS)
					crChunk = crHiddenPartBySOTNandHideOS;
				else if (m_abyUpPartStatus[i]&SC_HIDDENBYSOTN)
					crChunk = crHiddenPartBySOTN;
				else if (m_abyUpPartStatus[i]&SC_HIDDENBYHIDEOS)
					crChunk = crHiddenPartByHideOS;
				else if (m_abyUpPartStatus[i]&SC_XFER) //Fafner: mark transferred parts - 080325
					crChunk = crProgress;
				else 
					crChunk = crBoth;
				s_UpStatusBar.FillRange(PARTSIZE*(uint64)(i), PARTSIZE*(uint64)(i+1), crChunk);
			}
		//MORPH END   - Changed by SiRoB, See chunk that we hide
		}

		//Fafner: start: client percentage - 080429
		/*
		UploadingToClient_Struct* pUpClientStruct = theApp.uploadqueue->GetUploadingClientStructByClient(this);
		ASSERT(pUpClientStruct != NULL);
		*/
		//Fafner: start: client percentage - 080429
		if (pUpClientStruct != NULL)
		{
			//Fafner: start: client percentage - 080429
			/*
			CSingleLock lockBlockLists(&pUpClientStruct->m_csBlockListsLock, TRUE);
			ASSERT(lockBlockLists.IsLocked());
			*/
			//Fafner: start: client percentage - 080429
			const Requested_Block_Struct* block;
			if (!pUpClientStruct->m_BlockRequests_queue.IsEmpty()) {
				block = pUpClientStruct->m_BlockRequests_queue.GetHead();
				if (block) {
					uint32 start = (uint32)(block->StartOffset / PARTSIZE);
					s_UpStatusBar.FillRange((uint64)start * PARTSIZE, (uint64)(start + 1) * PARTSIZE, crNextSending);
				}
			}
			if (!pUpClientStruct->m_DoneBlocks_list.IsEmpty()) {
				block = pUpClientStruct->m_DoneBlocks_list.GetHead();
				if (block) {
					uint32 start = (uint32)(block->StartOffset / PARTSIZE);
					s_UpStatusBar.FillRange((uint64)start * PARTSIZE, (uint64)(start + 1) * PARTSIZE, crNextSending);
				}
			}
			if (!pUpClientStruct->m_DoneBlocks_list.IsEmpty()) {
				for (POSITION pos = pUpClientStruct->m_DoneBlocks_list.GetHeadPosition(); pos != 0;) {
					block = pUpClientStruct->m_DoneBlocks_list.GetNext(pos);
					s_UpStatusBar.FillRange(block->StartOffset, block->EndOffset + 1, crProgress);
				}

				// Also show what data is buffered (with color crBuffer)
				uint64 total = 0;

				for (POSITION pos = pUpClientStruct->m_DoneBlocks_list.GetTailPosition(); pos != 0; ) {
					Requested_Block_Struct* block = pUpClientStruct->m_DoneBlocks_list.GetPrev(pos);

					/*MORPH - FIX for zz code*/if (total + (block->EndOffset - block->StartOffset) <= GetSessionPayloadUp()) {
						// block is sent
						s_UpStatusBar.FillRange((uint64)block->StartOffset, (uint64)block->EndOffset, crProgress);
						total += block->EndOffset - block->StartOffset;
					}
					/*MORPH - FIX for zz code*/else if (total < GetSessionPayloadUp()) {
						// block partly sent, partly in buffer
						total += block->EndOffset - block->StartOffset;
						/*MORPH - FIX for zz code*/uint64 rest = total - GetSessionPayloadUp();
						uint64 newEnd = (block->EndOffset - rest);

						s_UpStatusBar.FillRange((uint64)block->StartOffset, (uint64)newEnd, crSending);
						s_UpStatusBar.FillRange((uint64)newEnd, (uint64)block->EndOffset, crBuffer);
					}
					else {
						// entire block is still in buffer
						total += block->EndOffset - block->StartOffset;
						s_UpStatusBar.FillRange((uint64)block->StartOffset, (uint64)block->EndOffset, crBuffer);
					}
				}
			}
			//Fafner: start: client percentage - 080429
			/*
			lockBlockLists.Unlock();
			*/
			if (lockBlockLists)
			{
				lockBlockLists->Unlock();
				delete lockBlockLists;
				lockBlockLists = NULL;
			}
			//Fafner: start: client percentage - 080429
		}
   	    s_UpStatusBar.Draw(dc, rect->left, rect->top, bFlat);
	}
} 

//MORPH START - Display current uploading chunk
void CUpDownClient::DrawUpStatusBarChunk(CDC* dc, RECT* rect, bool /*onlygreyrect*/, bool  bFlat) const
{
	COLORREF crNeither;
	COLORREF crNextSending;
	COLORREF crBoth;
	COLORREF crSending;
	COLORREF crBuffer;
	COLORREF crProgress;
	COLORREF crDot;
    if(GetSlotNumber() <= theApp.uploadqueue->GetActiveUploadsCount(m_classID) ||
       (GetUploadState() != US_UPLOADING && GetUploadState() != US_CONNECTING) ) {
        crNeither = RGB(224, 224, 224);
	    crNextSending = RGB(255,208,0);
	    crBoth = bFlat ? RGB(0, 0, 0) : RGB(104, 104, 104);
	    crSending = RGB(0, 150, 0);
		crBuffer = RGB(255, 100, 100);
		crDot = RGB(255, 255, 255);
		crProgress = RGB(0, 224, 0);
    } else {
        // grayed out
        crNeither = RGB(248, 248, 248);
	    crNextSending = RGB(255,244,191);
	    crBoth = bFlat ? RGB(191, 191, 191) : RGB(191, 191, 191);
	    crSending = RGB(191, 229, 191);
		crBuffer = RGB(255, 216, 216);
		crDot = RGB(255, 255, 255);
		crProgress = RGB(191, 255, 191);
    }

	// wistily: UpStatusFix
	//MORPH - Changed by SiRoB, Optimization requpfile
	/*
	CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
	*/
	CKnownFile* currequpfile = CheckAndGetReqUpFile();
	EMFileSize filesize;
	uint64 chunksize = PARTSIZE;
	if (currequpfile)
		filesize=currequpfile->GetFileSize();
	else
		filesize = (uint64)(PARTSIZE * (uint64)m_nUpPartCount);
	// wistily: UpStatusFix

	if(filesize <= (uint64)0)
		return;

	UploadingToClient_Struct* pUpClientStruct = theApp.uploadqueue->GetUploadingClientStructByClient(this);
	ASSERT(pUpClientStruct != NULL);
	if (pUpClientStruct != NULL)
	{
		CSingleLock lockBlockLists(&pUpClientStruct->m_csBlockListsLock, TRUE);
		ASSERT(lockBlockLists.IsLocked());
		if (!pUpClientStruct->m_BlockRequests_queue.IsEmpty() || !pUpClientStruct->m_DoneBlocks_list.IsEmpty()) {
			uint32 cur_chunk = (uint32)-1;
			uint64 start = (uint64)-1;
			uint64 end = (uint64)-1;
			const Requested_Block_Struct* block;
			if (!pUpClientStruct->m_DoneBlocks_list.IsEmpty()) {
				block = pUpClientStruct->m_DoneBlocks_list.GetHead();
				if (cur_chunk == (uint32)-1) {
					cur_chunk = (uint32)(block->StartOffset / PARTSIZE);
					start = end = cur_chunk * PARTSIZE;
					end += PARTSIZE - 1;
					if (end > filesize)
					{
						end = filesize;
						chunksize = end - start;
						if (chunksize <= 0) chunksize = PARTSIZE;
					}
					s_UpStatusBar.SetFileSize(chunksize);
					s_UpStatusBar.SetHeight(rect->bottom - rect->top);
					s_UpStatusBar.SetWidth(rect->right - rect->left);
					/*
					if (end > filesize) {
						end = filesize;
						s_UpStatusBar.Reset();
						s_UpStatusBar.FillRange(0, end%PARTSIZE, crNeither);
					} else
					*/
					s_UpStatusBar.Fill(crNeither);
				}
			}
			if (!pUpClientStruct->m_BlockRequests_queue.IsEmpty()) {
				for (POSITION pos = pUpClientStruct->m_BlockRequests_queue.GetHeadPosition(); pos != 0;) {
					block = pUpClientStruct->m_BlockRequests_queue.GetNext(pos);
					if (cur_chunk == (uint32)-1) {
						cur_chunk = (uint32)(block->StartOffset / PARTSIZE);
						start = end = cur_chunk * PARTSIZE;
						end += PARTSIZE - 1;
						if (end > filesize)
						{
							end = filesize;
							chunksize = end - start;
							if (chunksize <= 0) chunksize = PARTSIZE;
						}
						s_UpStatusBar.SetFileSize(chunksize);
						s_UpStatusBar.SetHeight(rect->bottom - rect->top);
						s_UpStatusBar.SetWidth(rect->right - rect->left);
						/*
						if (end > filesize) {
							end = filesize;
							s_UpStatusBar.Reset();
							s_UpStatusBar.FillRange(0, end%PARTSIZE, crNeither);
						} else
						*/
						s_UpStatusBar.Fill(crNeither);
					}
					if (block->StartOffset <= end && block->EndOffset >= start) {
						s_UpStatusBar.FillRange((block->StartOffset > start) ? block->StartOffset % PARTSIZE : (uint64)0, ((block->EndOffset < end) ? block->EndOffset + 1 : end) % PARTSIZE, crNextSending);
					}
				}
			}

			if (!pUpClientStruct->m_DoneBlocks_list.IsEmpty() && cur_chunk != (uint32)-1) {
				// Also show what data is buffered (with color crBuffer)
				uint64 total = 0;

				for (POSITION pos = pUpClientStruct->m_DoneBlocks_list.GetTailPosition(); pos != 0; ) {
					block = pUpClientStruct->m_DoneBlocks_list.GetPrev(pos);
					if (block->StartOffset <= end && block->EndOffset >= start) {
						if (total + (block->EndOffset - block->StartOffset) <= GetSessionPayloadUp()) {
							// block is sent
							s_UpStatusBar.FillRange((block->StartOffset > start) ? block->StartOffset % PARTSIZE : (uint64)0, ((block->EndOffset < end) ? block->EndOffset + 1 : end) % PARTSIZE, crProgress);
							total += block->EndOffset - block->StartOffset;
						}
						else if (total < GetSessionPayloadUp()) {
							// block partly sent, partly in buffer
							total += block->EndOffset - block->StartOffset;
							uint64 rest = total - GetSessionPayloadUp();
							uint64 newEnd = (block->EndOffset - rest);
							if (newEnd >= start) {
								if (newEnd <= end) {
									uint64 uNewEnd = newEnd % PARTSIZE;
									s_UpStatusBar.FillRange(block->StartOffset % PARTSIZE, uNewEnd, crSending);
									if (block->EndOffset <= end)
										s_UpStatusBar.FillRange(uNewEnd, block->EndOffset % PARTSIZE, crBuffer);
									else
										s_UpStatusBar.FillRange(uNewEnd, end % PARTSIZE, crBuffer);
								}
								else
									s_UpStatusBar.FillRange(block->StartOffset % PARTSIZE, end % PARTSIZE, crSending);
							}
							else if (block->EndOffset <= end)
								s_UpStatusBar.FillRange((uint64)0, block->EndOffset % PARTSIZE, crBuffer);
						}
						else {
							// entire block is still in buffer
							total += block->EndOffset - block->StartOffset;
							s_UpStatusBar.FillRange((block->StartOffset > start) ? block->StartOffset % PARTSIZE : (uint64)0, ((block->EndOffset < end) ? block->EndOffset : end) % PARTSIZE, crBuffer);
						}
					}
					else
						total += block->EndOffset - block->StartOffset;
				}
			}
			s_UpStatusBar.Draw(dc, rect->left, rect->top, bFlat);

			if (thePrefs.m_bEnableChunkDots) {
				s_UpStatusBar.SetHeight(3);
				s_UpStatusBar.SetWidth(1);
				s_UpStatusBar.SetFileSize((uint64)1);
				s_UpStatusBar.Fill(crDot);
				uint32	w = rect->right - rect->left + 1;
				if (!pUpClientStruct->m_BlockRequests_queue.IsEmpty()) {
					for (POSITION pos = pUpClientStruct->m_BlockRequests_queue.GetHeadPosition(); pos != 0;) {
						block = pUpClientStruct->m_BlockRequests_queue.GetNext(pos);
						if (block->StartOffset <= end && block->EndOffset >= start) {
							if (block->StartOffset >= start) {
								if (block->EndOffset <= end) {
									s_UpStatusBar.Draw(dc, rect->left + (int)((double)(block->StartOffset % PARTSIZE) * w / (uint64)chunksize), rect->top, bFlat);
									s_UpStatusBar.Draw(dc, rect->left + (int)((double)(block->EndOffset % PARTSIZE) * w / (uint64)chunksize), rect->top, bFlat);
								}
								else
									s_UpStatusBar.Draw(dc, rect->left + (int)((double)(block->StartOffset % PARTSIZE) * w / (uint64)chunksize), rect->top, bFlat);
							}
							else if (block->EndOffset <= end)
								s_UpStatusBar.Draw(dc, rect->left + (int)((double)(block->EndOffset % PARTSIZE) * w / (uint64)chunksize), rect->top, bFlat);
						}
					}
				}
				if (!pUpClientStruct->m_DoneBlocks_list.IsEmpty()) {
					for (POSITION pos = pUpClientStruct->m_DoneBlocks_list.GetHeadPosition(); pos != 0;) {
						block = pUpClientStruct->m_DoneBlocks_list.GetNext(pos);
						if (block->StartOffset <= end && block->EndOffset >= start) {
							if (block->StartOffset >= start) {
								if (block->EndOffset <= end) {
									s_UpStatusBar.Draw(dc, rect->left + (int)((double)(block->StartOffset % PARTSIZE) * w / (uint64)chunksize), rect->top, bFlat);
									s_UpStatusBar.Draw(dc, rect->left + (int)((double)(block->EndOffset % PARTSIZE) * w / (uint64)chunksize), rect->top, bFlat);
								}
								else
									s_UpStatusBar.Draw(dc, rect->left + (int)((double)(block->StartOffset % PARTSIZE) * w / (uint64)chunksize), rect->top, bFlat);
							}
							else if (block->EndOffset <= end)
								s_UpStatusBar.Draw(dc, rect->left + (int)((double)(block->EndOffset % PARTSIZE) * w / (uint64)chunksize), rect->top, bFlat);
						}
					}
				}
			}
		}
		lockBlockLists.Unlock();
	}
}

float CUpDownClient::GetUpChunkProgressPercent() const
{
	//MORPH - Changed by SiRoB, Optimization requpfile
	/*
	CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
	*/
	CKnownFile* currequpfile = CheckAndGetReqUpFile();
	EMFileSize filesize;
	uint64 chunksize = PARTSIZE;
	if (currequpfile)
		filesize=currequpfile->GetFileSize();
	else
		filesize = (uint64)(PARTSIZE * (uint64)m_nUpPartCount);

	if(filesize <= (uint64)0)
		return 0.0f;

	UploadingToClient_Struct* pUpClientStruct = theApp.uploadqueue->GetUploadingClientStructByClient(this);
	ASSERT(pUpClientStruct != NULL);
	if (pUpClientStruct != NULL)
	{
		CSingleLock lockBlockLists(&pUpClientStruct->m_csBlockListsLock, TRUE);
		ASSERT(lockBlockLists.IsLocked());
		if (!pUpClientStruct->m_DoneBlocks_list.IsEmpty()) {
			uint32 cur_chunk = (uint32)-1;
			uint64 start = (uint64)-1;
			uint64 end = (uint64)-1;
			const Requested_Block_Struct* block;
			block = pUpClientStruct->m_DoneBlocks_list.GetHead();
			cur_chunk = (uint32)(block->StartOffset / PARTSIZE);
			start = end = cur_chunk * PARTSIZE;
			end += PARTSIZE - 1;
			if (end > filesize)
			{
				chunksize = end - start;
				if (chunksize <= 0) chunksize = PARTSIZE;
			}
			lockBlockLists.Unlock();
			return (float)(((double)(block->EndOffset % PARTSIZE) / (double)chunksize) * 100.0f);
		}
		lockBlockLists.Unlock();
	}
	return 0.0f;
}
//MORPH END   - Display current uploading chunk

void CUpDownClient::DrawUpStatusBarChunkText(CDC* dc, RECT* cur_rec) const //Fafner: part number - 080317
{
	if (!thePrefs.GetUseClientPercentage())
		return;
	CString Sbuffer;
	CRect rcDraw = cur_rec;
	rcDraw.top--;rcDraw.bottom--;
	COLORREF oldclr = dc->SetTextColor(RGB(0,0,0));
	int iOMode = dc->SetBkMode(TRANSPARENT);
	UploadingToClient_Struct* pUpClientStruct = theApp.uploadqueue->GetUploadingClientStructByClient(this);
	ASSERT(pUpClientStruct != NULL);
	if (pUpClientStruct != NULL)
	{
		CSingleLock lockBlockLists(&pUpClientStruct->m_csBlockListsLock, TRUE);
		ASSERT(lockBlockLists.IsLocked());
		if (!pUpClientStruct->m_DoneBlocks_list.IsEmpty())
			Sbuffer.Format(_T("%u"), (UINT)(pUpClientStruct->m_DoneBlocks_list.GetHead()->StartOffset / PARTSIZE));
		else if (!pUpClientStruct->m_BlockRequests_queue.IsEmpty())
			Sbuffer.Format(_T("%u"), (UINT)(pUpClientStruct->m_BlockRequests_queue.GetHead()->StartOffset / PARTSIZE));
		else
			Sbuffer.Format(_T("?"));
		lockBlockLists.Unlock();
	}

	//MORPH START - Show percentage finished
	Sbuffer.AppendFormat(_T(" @ %.1f%%"),GetUpChunkProgressPercent());
	//MORPH END   - Show percentage finished
	
	#define	DrawChunkText	dc->DrawText(Sbuffer, Sbuffer.GetLength(), &rcDraw, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)
	DrawChunkText;
	rcDraw.left+=1;rcDraw.right+=1;
	DrawChunkText;
	rcDraw.left+=1;rcDraw.right+=1;
	DrawChunkText;
	
	rcDraw.top+=1;rcDraw.bottom+=1;
	DrawChunkText;
	rcDraw.top+=1;rcDraw.bottom+=1;
	DrawChunkText;
	
	rcDraw.left-=1;rcDraw.right-=1;
	DrawChunkText;
	rcDraw.left-=1;rcDraw.right-=1;
	DrawChunkText;
	
	rcDraw.top-=1;rcDraw.bottom-=1;
	DrawChunkText;
	
	rcDraw.left++;rcDraw.right++;
	dc->SetTextColor(RGB(255,255,255));
	DrawChunkText;
	dc->SetBkMode(iOMode);
	dc->SetTextColor(oldclr);
}

void CUpDownClient::DrawCompletedPercent(CDC* dc, RECT* cur_rec) const //Fafner: client percentage - 080325
{
	if (!thePrefs.GetUseClientPercentage())
		return;
	float percent = GetCompletedPercent();
	if (percent <= 0.001f) // increased from 5 % to 0.1 %
		return;
	CString Sbuffer;
	CRect rcDraw = cur_rec;
	rcDraw.top--;rcDraw.bottom--;
	COLORREF oldclr = dc->SetTextColor(RGB(0,0,0));
	int iOMode = dc->SetBkMode(TRANSPARENT);
	Sbuffer.Format(_T("%.1f%%"), percent);
	
	#define	DrawPercentText	dc->DrawText(Sbuffer, Sbuffer.GetLength(), &rcDraw, DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)
	DrawPercentText;
	rcDraw.left+=1;rcDraw.right+=1;
	DrawPercentText;
	rcDraw.left+=1;rcDraw.right+=1;
	DrawPercentText;
	
	rcDraw.top+=1;rcDraw.bottom+=1;
	DrawPercentText;
	rcDraw.top+=1;rcDraw.bottom+=1;
	DrawPercentText;
	
	rcDraw.left-=1;rcDraw.right-=1;
	DrawPercentText;
	rcDraw.left-=1;rcDraw.right-=1;
	DrawPercentText;
	
	rcDraw.top-=1;rcDraw.bottom-=1;
	DrawPercentText;
	
	rcDraw.left++;rcDraw.right++;
	dc->SetTextColor(RGB(255,255,255));
	DrawPercentText;
	dc->SetBkMode(iOMode);
	dc->SetTextColor(oldclr);
}

float CUpDownClient::GetCompletedPercent() const //Fafner: client percentage - 080325
{
	//MORPH START - Changed by Stulle, try to avoid crash
	//TODO: This crashfix should really not look like it does. How in the world could this happen?
	try
	{
		CKnownFile* currequpfile = CheckAndGetReqUpFile();
		if(!currequpfile) // no NULL-pointer
			return 0.0f;
		const uint16 uPartCount = currequpfile->GetPartCount();
		if(uPartCount > 0) // no division by zero
			return (float)(m_uiCompletedParts + m_uiCurrentChunks) / (float)uPartCount * 100.0f;
		else
			return 0.0f;
	}
	catch(...)
	{	//Note: although we check for currequpfile != NULL above, there may be cases where GetPartCount() crashes anyway. Further investigation ist needed.
		//Note: DebugLogError may be removed later
		DebugLogError(_T("CUpDownClient::GetCompletedPercent: exception - client %s"), DbgGetClientInfo());
		return 0.0f;
	}
	//MORPH END   - Changed by Stulle, try to avoid crash
}

void CUpDownClient::SetUploadState(EUploadState eNewState)
{
	if (eNewState != m_nUploadState)
	{
		//MORPH START - ReadBlockFromFileThread
#ifdef USE_MORPH_READ_THREAD
		if (m_readblockthread) {
			m_readblockthread->StopReadBlock();
			m_readblockthread = NULL;
		}
#endif
		//MORPH END   - ReadBlockFromFileThread
		if (m_nUploadState == US_UPLOADING)
		{
			// Reset upload data rate computation
			m_nUpDatarate = 0;
			m_nSumForAvgUpDataRate = 0;
			m_AvarageUDR_list.RemoveAll();
		}
		if (eNewState == US_UPLOADING) {
			m_fSentOutOfPartReqs = 0;
			m_AvarageUDRLastRemovedTimestamp = GetTickCount(); //MORPH - Added by SiRoB, Better Upload rate calcul
		}
		// don't add any final cleanups for US_NONE here
		m_nUploadState = (_EUploadState)eNewState;
		theApp.emuledlg->transferwnd->GetClientList()->RefreshClient(this);
	}
}

/**
 * Gets the queue score multiplier for this client, taking into consideration client's credits
 * and the requested file's priority.
 */
//Morph Start - added by AndCycle, Equal Chance For Each File
/*
float CUpDownClient::GetCombinedFilePrioAndCredit() {
*/
double CUpDownClient::GetCombinedFilePrioAndCredit() {
//Morph END - added by AndCycle, Equal Chance For Each File
	if (credits == 0){
		ASSERT ( IsKindOf(RUNTIME_CLASS(CUrlClient)) );
		return 0.0F;
	}

	//Morph Start - added by AndCycle, Equal Chance For Each File
	if (thePrefs.IsEqualChanceEnable())
	{
		//MORPH START - Changed by SiRoB, Optimization requpfile
		/*
		CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
		*/
		CKnownFile* currequpfile = CheckAndGetReqUpFile();
		if(currequpfile)
			return currequpfile->statistic.GetEqualChanceValue();
	}
	//Morph End - added by AndCycle, Equal Chance For Each File

    //Morph Start - added by AndCycle, Equal Chance For Each File
    /*
    return 10.0f * credits->GetScoreRatio(GetIP()) * (float)GetFilePrioAsNumber();
    */
	return 10.0 * double(credits->GetScoreRatio(GetIP())) * double(GetFilePrioAsNumber());
	//Morph END - added by AndCycle, Equal Chance For Each File
}

/**
* Gets the file multiplier for the file this client has requested.
*/
int CUpDownClient::GetFilePrioAsNumber() const {
	//MORPH - Changed by SiRoB, Optimization requpfile
	/*
	CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
	*/
	CKnownFile* currequpfile = CheckAndGetReqUpFile();
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

	if (credits == 0){
		ASSERT ( IsKindOf(RUNTIME_CLASS(CUrlClient)) );
		return 0;
	}
	//MORPH - Changed by SiRoB, Optimization requpfile
	/*
	CKnownFile* currequpfile = theApp.sharedfiles->GetFileByID(requpfileid);
	*/
	CKnownFile* currequpfile = CheckAndGetReqUpFile();
	if(!currequpfile)
		return 0;
	
	// bad clients (see note in function)
	if (credits->GetCurrentIdentState(GetIP()) == IS_IDBADGUY)
		return 0;
	// friend slot
	if (IsFriend() && GetFriendSlot() && !HasLowID())
		return 0x0FFFFFFF;

	//MORPH - Changed by SiRoB, Code Optimization
	/*
	if (IsBanned() || m_bGPLEvildoer)
	*/
	if (m_nUploadState==US_BANNED || m_bGPLEvildoer)
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
		//ASSERT ( m_dwUploadTime-GetWaitStartTime() >= 0 ); //oct 28, 02: changed this from "> 0" to ">= 0" -> // 02-Okt-2006 []: ">=0" is always true!
		fBaseValue += (float)(::GetTickCount() - m_dwUploadTime > 900000)? 900000:1800000;
		fBaseValue /= 1000;
	}
	if(thePrefs.UseCreditSystem())
	{
		float modif = credits->GetScoreRatio(GetIP());
		fBaseValue *= modif;
	}
	if (!onlybasevalue)
		fBaseValue *= (float(filepriority)/10.0f);

	if( (IsEmuleClient() || this->GetClientSoft() < 10) && m_byEmuleVersion <= 0x19 )
		fBaseValue *= 0.5f;
	return (uint32)fBaseValue;
}

//Morph Start - added by AndCycle, Equal Chance For Each File
double CUpDownClient::GetEqualChanceValue() const
{
	//MORPH - Changed by SiRoB, Optimization requpfile
	/*
	CKnownFile* currentReqFile = theApp.sharedfiles->GetFileByID((uchar*)GetUploadFileID());
	*/
	CKnownFile* currentReqFile = CheckAndGetReqUpFile();
	if(currentReqFile != NULL){
		return currentReqFile->statistic.GetEqualChanceValue();
	}
	return 0;
}
//Morph End - added by AndCycle, Equal Chance For Each File

//Morph Start - added by AndCycle, Pay Back First
//Comment : becarefull when changing this function don't forget to change IsPBForPS() 
bool CUpDownClient::IsMoreUpThanDown() const{
	//MORPH - Changed by SiRoB, Optimization requpfile
	/*
	CKnownFile* currentReqFile = theApp.sharedfiles->GetFileByID((uchar*)GetUploadFileID());
	*/
	CKnownFile* currentReqFile = CheckAndGetReqUpFile();
	return currentReqFile && currentReqFile->IsPartFile()==false && credits->GetPayBackFirstStatus() && thePrefs.IsPayBackFirst() && IsSecure();
}
//Morph End - added by AndCycle, Pay Back First
//MORPH START - Added by SiRoB, Code Optimization
bool CUpDownClient::IsMoreUpThanDown(const CKnownFile* file) const{
	return !file->IsPartFile() && credits->GetPayBackFirstStatus() && thePrefs.IsPayBackFirst() && IsSecure();
}
//MORPH END   - Added by SiRoB, Code Optimization

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
	//MORPH - Changed by SiRoB, Optimization requpfile
	/*
	CKnownFile* currentReqFile = theApp.sharedfiles->GetFileByID(GetUploadFileID());
	*/
	CKnownFile* currentReqFile = CheckAndGetReqUpFile();
	//MORPH - Changed by SiRoB, Optimization requpfile
	if (currentReqFile == NULL)
		return false;
	//-->Commun to both call
	if (
		currentReqFile->GetPowerShared() || 
		!currentReqFile->IsPartFile() && credits->GetPayBackFirstStatus() && thePrefs.IsPayBackFirst() && IsSecure() ||
		(!currentReqFile->IsPartFile() && currentReqFile->statistic.GetFairPlay()) // EastShare - FairPlay by AndCycle
		)
		return true;
	return false;
}
//MORPH END   - Added by SiRoB, Code Optimization PBForPS()

bool CUpDownClient::ProcessExtendedInfo(CSafeMemFile* data, CKnownFile* tempreqfile)
{
	if (m_abyUpPartStatus) { //Fafner: missing? - 080325
		delete[] m_abyUpPartStatus;
		m_abyUpPartStatus = NULL;
	}
	m_nUpPartCount = 0;
	m_nUpCompleteSourcesCount= 0;
	m_uiCompletedParts = 0; //Fafner: client percentage - 080325
	m_uiLastChunk = (UINT)-1; //Fafner: client percentage - 080325
	m_uiCurrentChunks = 0; //Fafner: client percentage - 080325
	if (GetExtendedRequestsVersion() == 0)
		return true;

	uint16 nED2KUpPartCount = data->ReadUInt16();
	if (!nED2KUpPartCount)
	{
		m_nUpPartCount = tempreqfile->GetPartCount();
		m_abyUpPartStatus = new uint8[m_nUpPartCount];
		memset(m_abyUpPartStatus, 0, m_nUpPartCount);
	}
	else
	{
		if (tempreqfile->GetED2KPartCount() != nED2KUpPartCount)
		{
			//We already checked if we are talking about the same file.. So if we get here, something really strange happened!
			m_nUpPartCount = 0;
			return false;
		}
		m_nUpPartCount = tempreqfile->GetPartCount();
		m_abyUpPartStatus = new uint8[m_nUpPartCount];
		uint16 done = 0;
		while (done != m_nUpPartCount)
		{
			uint8 toread = data->ReadUInt8();
			for (UINT i = 0; i != 8; i++)
			{
				m_abyUpPartStatus[done] = ((toread >> i) & 1) ? 1 : 0;
//				We may want to use this for another feature..
//				if (m_abyUpPartStatus[done] && !tempreqfile->IsComplete((uint64)done*PARTSIZE,((uint64)(done+1)*PARTSIZE)-1))
//					bPartsNeeded = true;
				done++;
				if (done == m_nUpPartCount)
					break;
			}
		}
	}
	if (GetExtendedRequestsVersion() > 1)
	{
		uint16 nCompleteCountLast = GetUpCompleteSourcesCount();
		uint16 nCompleteCountNew = data->ReadUInt16();
		SetUpCompleteSourcesCount(nCompleteCountNew);
		if (nCompleteCountLast != nCompleteCountNew)
			tempreqfile->UpdatePartsInfo();
	}
	if (m_abyUpPartStatus) { //Fafner: client percentage - 080325
		UINT result = 0;
		for (UINT i = 0; i < tempreqfile->GetPartCount(); i++) {
			if (m_abyUpPartStatus[i] & SC_AVAILABLE)
				result++;
		}
		m_uiCompletedParts = result;
	}
	theApp.emuledlg->transferwnd->GetQueueList()->RefreshClient(this);
	return true;
}

void CUpDownClient::SetUploadFileID(CKnownFile* newreqfile)
{
	//MORPH - Changed by SiRoB, Optimization requpfile
	/*
	CKnownFile* oldreqfile;
	//We use the knownfilelist because we may have unshared the file..
	//But we always check the download list first because that person may have decided to redownload that file.
	//Which will replace the object in the knownfilelist if completed.
	if ((oldreqfile = theApp.downloadqueue->GetFileByID(requpfileid)) == NULL)
	*/
	CKnownFile* oldreqfile = requpfile;
	if (!theApp.downloadqueue->IsPartFile(requpfile))
		oldreqfile = theApp.knownfiles->FindKnownFileByID(requpfileid);
	else
	{
		// In some _very_ rare cases it is possible that we have different files with the same hash in the downloadlist
		// as well as in the sharedlist (redownloading a unshared file, then resharing it before the first part has been downloaded)
		// to make sure that in no case a deleted client object is left on the list, we need to doublecheck
		// TODO: Fix the whole issue properly
		CKnownFile* pCheck = theApp.sharedfiles->GetFileByID(requpfileid);
		if (pCheck != NULL && pCheck != oldreqfile)
		{
			ASSERT( false );
			pCheck->RemoveUploadingClient(this);
		}
	}

	if (newreqfile == oldreqfile)
		return;

	// clear old status
	if (m_abyUpPartStatus) { //Fafner: missing? - 080325
		delete[] m_abyUpPartStatus;
		m_abyUpPartStatus = NULL;
	}
	m_nUpPartCount = 0;
	m_nUpCompleteSourcesCount= 0;
	m_nSelectedChunk = 0;	// SLUGFILLER: hideOS - TODO: Notify the file the chunk is free for all

	if (newreqfile)
	{
		newreqfile->AddUploadingClient(this);
		md4cpy(requpfileid, newreqfile->GetFileHash());
	}
	else
		md4clr(requpfileid);

	//MORPH START - Added by SiRoB, Optimization requpfile
	requpfile = newreqfile;
	requpfileid_lasttimeupdated = theApp.sharedfiles->GetLastTimeFileMapUpdated();
	//MORPH END   - Added by SiRoB, Optimization requpfile

	if (oldreqfile) 
		oldreqfile->RemoveUploadingClient(this);
}
static INT_PTR dbgLastQueueCount = 0;
void CUpDownClient::AddReqBlock(Requested_Block_Struct* reqblock, bool bSignalIOThread)
{
	// do _all_ sanitychecks on the requested block here, than put it on the blocklsit for the client
	// UploadDiskIPThread will handle those lateron

	if (reqblock != NULL)
	{
		if(GetUploadState() != US_UPLOADING) {
			if(thePrefs.GetLogUlDlEvents())
				AddDebugLogLine(DLP_LOW, false, _T("UploadClient: Client tried to add req block when not in upload slot! Prevented req blocks from being added. %s"), DbgGetClientInfo());
			delete reqblock;
			return;
		}

		if(HasCollectionUploadSlot()){
			CKnownFile* pDownloadingFile = theApp.sharedfiles->GetFileByID(reqblock->FileID);
			if(pDownloadingFile != NULL){
				if ( !(CCollection::HasCollectionExtention(pDownloadingFile->GetFileName()) && pDownloadingFile->GetFileSize() < (uint64)MAXPRIORITYCOLL_SIZE) ){
					AddDebugLogLine(DLP_HIGH, false, _T("UploadClient: Client tried to add req block for non collection while having a collection slot! Prevented req blocks from being added. %s"), DbgGetClientInfo());
					delete reqblock;
					return;
				}
			}
			else
				ASSERT( false );
		}

		CKnownFile* srcfile = theApp.sharedfiles->GetFileByID(reqblock->FileID);
		if (srcfile == NULL)
		{
			DebugLogWarning(GetResString(IDS_ERR_REQ_FNF));
			delete reqblock;
			return;
		}

		UploadingToClient_Struct* pUploadingClientStruct = theApp.uploadqueue->GetUploadingClientStructByClient(this);
		if (pUploadingClientStruct == NULL)
		{
			DebugLogError(_T("AddReqBlock: Uploading client not found in Uploadlist, %s, %s"), DbgGetClientInfo(), srcfile->GetFileName());
			delete reqblock;
			return;
		}

		if (pUploadingClientStruct->m_bIOError)
		{
			DebugLogWarning(_T("AddReqBlock: Uploading client has pending IO Error, %s, %s"), DbgGetClientInfo(), srcfile->GetFileName());
			delete reqblock;
			return;
		}

		//MORPH START - Changed by SiRoB, SLUGFILLER: SafeHash
		/*
		if (srcfile->IsPartFile() && !((CPartFile*)srcfile)->IsComplete(reqblock->StartOffset,reqblock->EndOffset-1, true))
		*/
		if (srcfile->IsPartFile() && !((CPartFile*)srcfile)->IsRangeShareable(reqblock->StartOffset, reqblock->EndOffset - 1))	// SLUGFILLER: SafeHash - final safety precaution
		//MORPH END  - Changed by SiRoB, SLUGFILLER: SafeHash
		{
			DebugLogWarning(_T("AddReqBlock: %s, %s"), GetResString(IDS_ERR_INCOMPLETEBLOCK), DbgGetClientInfo(), srcfile->GetFileName());
			delete reqblock;
			return;
		}

		if (reqblock->StartOffset >= reqblock->EndOffset || reqblock->EndOffset > srcfile->GetFileSize())
		{
			DebugLogError(_T("AddReqBlock: Invalid Blockrequests (negative or bytes to read, read after EOF), %s, %s"), DbgGetClientInfo(), srcfile->GetFileName());
			delete reqblock;
			return;		
		}

		if (reqblock->EndOffset - reqblock->StartOffset > EMBLOCKSIZE*3)
		{
			DebugLogWarning(_T("AddReqBlock: %s, %s"), GetResString(IDS_ERR_LARGEREQBLOCK), DbgGetClientInfo(), srcfile->GetFileName());
			delete reqblock;
			return;
		}

		//MORPH START - Added by SiRoB, Anti Anti HideOS & SOTN :p 
		if (m_abyUpPartStatus) {
			for (UINT i = (UINT)(reqblock->StartOffset / PARTSIZE); i < (UINT)((reqblock->EndOffset - 1) / PARTSIZE + 1); i++)
				//if (m_abyUpPartStatus[i]>SC_AVAILABLE)
				if (m_abyUpPartStatus[i] & SC_HIDDENBYSOTN || m_abyUpPartStatus[i] & SC_HIDDENBYHIDEOS) //Fafner: mark transferred parts (here: take care of) - 080325
				{
					DebugLogWarning(_T("%s: Part %u, %I64u - %I64u "), GetResString(IDS_ERR_HIDDENBLOCK), i, reqblock->EndOffset, reqblock->StartOffset);
					delete reqblock;
					return;
				}
		}
		else {
			DebugLogWarning(_T("%s: Part %u, %I64u - %I64u "), GetResString(IDS_ERR_HIDDENSOURCE), (UINT)(reqblock->StartOffset / PARTSIZE), reqblock->EndOffset, reqblock->StartOffset);
			delete reqblock;
			return;
		}
		//MORPH END   - Added by SiRoB, Anti Anti HideOS & SOTN :p 

		CSingleLock lockBlockLists(&pUploadingClientStruct->m_csBlockListsLock, TRUE);
		if (!lockBlockLists.IsLocked())
		{
			ASSERT( false );
			delete reqblock;
			return;
		}

		for (POSITION pos = pUploadingClientStruct->m_DoneBlocks_list.GetHeadPosition(); pos != 0; ){
			const Requested_Block_Struct* cur_reqblock = pUploadingClientStruct->m_DoneBlocks_list.GetNext(pos);
			if (reqblock->StartOffset == cur_reqblock->StartOffset && reqblock->EndOffset == cur_reqblock->EndOffset){
				delete reqblock;
				return;
			}
		}
		for (POSITION pos = pUploadingClientStruct->m_BlockRequests_queue.GetHeadPosition(); pos != 0; ){
			const Requested_Block_Struct* cur_reqblock = pUploadingClientStruct->m_BlockRequests_queue.GetNext(pos);
			if (reqblock->StartOffset == cur_reqblock->StartOffset && reqblock->EndOffset == cur_reqblock->EndOffset){
				delete reqblock;
				return;
			}
		}
		pUploadingClientStruct->m_BlockRequests_queue.AddTail(reqblock);
		dbgLastQueueCount = pUploadingClientStruct->m_BlockRequests_queue.GetCount();
		lockBlockLists.Unlock(); // not needed, just to make it visible
	}
	if (bSignalIOThread && theApp.m_pUploadDiskIOThread != NULL)
	{
		/*DebugLog(_T("BlockRequest Packet received, we have currently %u waiting requests and %s data in buffer (%u in ready packets, %s in pending IO Disk read) , socket busy: %s"), dbgLastQueueCount
			,CastItoXBytes(GetQueueSessionUploadAdded() - (GetQueueSessionPayloadUp() + socket->GetSentPayloadSinceLastCall(false)) , false, false, 2)
			, socket->DbgGetStdQueueCount(), CastItoXBytes((uint32)theApp.m_pUploadDiskIOThread->dbgDataReadPending, false, false, 2)
			,_T("?")); */ 
		theApp.m_pUploadDiskIOThread->NewBlockRequestsAvailable();
	}
	//MORPH START - Determine Remote Speed
	DWORD curTick = GetTickCount();
	if (curTick - m_dwUpDatarateAVG > SEC2MS(1)) {
		m_nUpDatarateAVG = 1000*(m_nTransferredUp-m_nTransferredUpDatarateAVG)/(curTick+1 - m_dwUpDatarateAVG);
		m_nTransferredUpDatarateAVG = m_nTransferredUp;
		m_dwUpDatarateAVG = curTick;
	}
	//MORPH END   - Determine Remote Speed
}

uint32 CUpDownClient::UpdateUploadingStatisticsData()
{
    DWORD curTick = ::GetTickCount();

    uint64 sentBytesCompleteFile = 0;
    uint64 sentBytesPartFile = 0;
    uint64 sentBytesPayload = 0;

    if (GetFileUploadSocket() && (m_ePeerCacheUpState != PCUS_WAIT_CACHE_REPLY))
	{
		CEMSocket* s = GetFileUploadSocket();
		UINT uUpStatsPort;
        if (m_pPCUpSocket && IsUploadingToPeerCache())
		{
			uUpStatsPort = (UINT)-1;

            // Check if filedata has been sent via the normal socket since last call.
            uint64 sentBytesCompleteFileNormalSocket = socket->GetSentBytesCompleteFileSinceLastCallAndReset();
            uint64 sentBytesPartFileNormalSocket = socket->GetSentBytesPartFileSinceLastCallAndReset();

			if(thePrefs.GetVerbose() && (sentBytesCompleteFileNormalSocket + sentBytesPartFileNormalSocket > 0)) {
                AddDebugLogLine(false, _T("Sent file data via normal socket when in PC mode. Bytes: %I64i."), sentBytesCompleteFileNormalSocket + sentBytesPartFileNormalSocket);
			}
        }
		else
			uUpStatsPort = GetUserPort();

	    // Extended statistics information based on which client software and which port we sent this data to...
	    // This also updates the grand total for sent bytes, etc.  And where this data came from.
        sentBytesCompleteFile = s->GetSentBytesCompleteFileSinceLastCallAndReset();
        sentBytesPartFile = s->GetSentBytesPartFileSinceLastCallAndReset();
		thePrefs.Add2SessionTransferData(GetClientSoft(), uUpStatsPort, false, true, (UINT)sentBytesCompleteFile, (IsFriend() && GetFriendSlot()));
		thePrefs.Add2SessionTransferData(GetClientSoft(), uUpStatsPort, true, true, (UINT)sentBytesPartFile, (IsFriend() && GetFriendSlot()));

		m_nTransferredUp = (UINT)(m_nTransferredUp + sentBytesCompleteFile + sentBytesPartFile);
        credits->AddUploaded((UINT)(sentBytesCompleteFile + sentBytesPartFile), GetIP());

        sentBytesPayload = s->GetSentPayloadSinceLastCall(true);
        m_nCurQueueSessionPayloadUp = (UINT)(m_nCurQueueSessionPayloadUp + sentBytesPayload);

		// on some rare cases (namely switching uploadfilees while still data is in the sendqueue), we count some bytes for
		// the wrong file, but fixing it (and not counting data only based on what was put into the queue and not sent yet) isn't really worth it
		//MORPH - Changed by SiRoB, Optimization requpfile
		/*
		CKnownFile* pCurrentUploadFile = theApp.sharedfiles->GetFileByID(GetUploadFileID());
		*/
		CKnownFile* pCurrentUploadFile = CheckAndGetReqUpFile();
		if (pCurrentUploadFile != NULL)
			pCurrentUploadFile->statistic.AddTransferred(sentBytesPayload);
		else
			ASSERT( false );

		// increase the sockets buffer on fast uploads. Even though this check should rather be in the throttler thread,
		// its better to do it here because we can access the clients downloadrate which the trottler cant
#if defined DONT_USE_SOCKET_BUFFERING
		if (GetDatarate() > 100 * 1024) 
			s->UseBigSendBuffer();
#endif
    }

	//MORPH START - Modified by SiRoB, Better Upload rate calcul
	/*
    if(sentBytesCompleteFile + sentBytesPartFile > 0 ||
        m_AvarageUDR_list.GetCount() == 0 || (curTick - m_AvarageUDR_list.GetTail().timestamp) > 1*1000) {
        // Store how much data we've transferred this round,
        // to be able to calculate average speed later
        // keep sum of all values in list up to date
        TransferredData newitem = {(UINT)(sentBytesCompleteFile + sentBytesPartFile), curTick};
        m_AvarageUDR_list.AddTail(newitem);
        m_nSumForAvgUpDataRate = (UINT)(m_nSumForAvgUpDataRate + sentBytesCompleteFile + sentBytesPartFile);
    }

    // remove to old values in list
    while (m_AvarageUDR_list.GetCount() > 0 && (curTick - m_AvarageUDR_list.GetHead().timestamp) > 10*1000) {
        // keep sum of all values in list up to date
        m_nSumForAvgUpDataRate -= m_AvarageUDR_list.RemoveHead().datalen;
    }

    // Calculate average speed for this slot
    if(m_AvarageUDR_list.GetCount() > 0 && (curTick - m_AvarageUDR_list.GetHead().timestamp) > 0 && GetUpStartTimeDelay() > 2*1000) {
        m_nUpDatarate = (UINT)(((ULONGLONG)m_nSumForAvgUpDataRate*1000) / (curTick - m_AvarageUDR_list.GetHead().timestamp));
    } else {
        // not enough values to calculate trustworthy speed. Use -1 to tell this
        m_nUpDatarate = 0; //-1;
    }
	*/
	curTick = GetTickCount();
	if(sentBytesCompleteFile + sentBytesPartFile > 0) {
		// Store how much data we've Transferred this round,
		// to be able to calculate average speed later
		// keep sum of all values in list up to date
		TransferredData newitem = {(UINT)(sentBytesCompleteFile + sentBytesPartFile), curTick};
		m_AvarageUDR_list.AddTail(newitem);
		m_nSumForAvgUpDataRate = (UINT)(m_nSumForAvgUpDataRate + sentBytesCompleteFile + sentBytesPartFile);
	}
	
	while ((UINT)m_AvarageUDR_list.GetCount() > 1 && (curTick - m_AvarageUDR_list.GetHead().timestamp) > MAXAVERAGETIMEUPLOAD) {
		m_AvarageUDRLastRemovedTimestamp = m_AvarageUDR_list.GetHead().timestamp;
		m_nSumForAvgUpDataRate -= m_AvarageUDR_list.RemoveHead().datalen;
	}
	
    if(m_AvarageUDR_list.GetCount() > 1) {
		DWORD dwDuration = m_AvarageUDR_list.GetTail().timestamp - m_AvarageUDRLastRemovedTimestamp;
		if (dwDuration < 100) dwDuration = 100;
		DWORD dwAvgTickDuration = dwDuration / (m_AvarageUDR_list.GetCount()-1);
		if ((curTick - m_AvarageUDR_list.GetTail().timestamp) > dwAvgTickDuration)
			dwDuration += curTick - m_AvarageUDR_list.GetTail().timestamp - dwAvgTickDuration;
		m_nUpDatarate = (UINT)(1000U * (ULONGLONG)m_nSumForAvgUpDataRate / dwDuration);
	}else if(m_AvarageUDR_list.GetCount() == 1) {
		DWORD dwDuration = m_AvarageUDR_list.GetTail().timestamp - m_AvarageUDRLastRemovedTimestamp;
		if (dwDuration < 100) dwDuration = 100;
		if ((curTick - m_AvarageUDR_list.GetTail().timestamp) > dwDuration)
			dwDuration = curTick - m_AvarageUDR_list.GetTail().timestamp;
		m_nUpDatarate = (UINT)(1000U * (ULONGLONG)m_nSumForAvgUpDataRate / dwDuration);
	} else {
		m_nUpDatarate = 0;
	}
	//MORPH END   - Modified by SiRoB, Better Upload rate calcul

    // Check if it's time to update the display.
	//MORPH START - UpdateItemThread
	/*
    if (curTick-m_lastRefreshedULDisplay > MINWAIT_BEFORE_ULDISPLAY_WINDOWUPDATE+(uint32)(rand()*800/RAND_MAX)) {
        // Update display
        theApp.emuledlg->transferwnd->GetUploadList()->RefreshClient(this);
        theApp.emuledlg->transferwnd->GetClientList()->RefreshClient(this);
        m_lastRefreshedULDisplay = curTick;
    }
	*/
        theApp.emuledlg->transferwnd->GetUploadList()->RefreshClient(this);
        theApp.emuledlg->transferwnd->GetClientList()->RefreshClient(this);
	//MORPH END - UpdateItemThread

    return (UINT)(sentBytesCompleteFile + sentBytesPartFile);
}

void CUpDownClient::SendOutOfPartReqsAndAddToWaitingQueue()
{
	//OP_OUTOFPARTREQS will tell the downloading client to go back to OnQueue..
	//The main reason for this is that if we put the client back on queue and it goes
	//back to the upload before the socket times out... We get a situation where the
	//downloader thinks it already sent the requested blocks and the uploader thinks
	//the downloader didn't send any request blocks. Then the connection times out..
	//I did some tests with eDonkey also and it seems to work well with them also..
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__OutOfPartReqs", this);
	Packet* pPacket = new Packet(OP_OUTOFPARTREQS, 0);
	theStats.AddUpDataOverheadFileRequest(pPacket->size);
	SendPacket(pPacket, true);
	m_fSentOutOfPartReqs = 1;
    theApp.uploadqueue->AddClientToQueue(this, true);
}

/**
 * See description for CEMSocket::TruncateQueues().
 */
void CUpDownClient::FlushSendBlocks(){ // call this when you stop upload, or the socket might be not able to send
    if (socket)      //socket may be NULL...
        socket->TruncateQueues();
}

void CUpDownClient::SendHashsetPacket(const uchar* pData, uint32 nSize, bool bFileIdentifiers)
{
	Packet* packet;
	CSafeMemFile fileResponse(1024);
	if (bFileIdentifiers)
	{
		CSafeMemFile data(pData, nSize);
		CFileIdentifierSA fileIdent;
		if (!fileIdent.ReadIdentifier(&data))
			throw _T("Bad FileIdentifier (OP_HASHSETREQUEST2)");
		CKnownFile* file = theApp.sharedfiles->GetFileByIdentifier(fileIdent, false);
		if (file == NULL)
		{
			CheckFailedFileIdReqs(fileIdent.GetMD4Hash());
			throw GetResString(IDS_ERR_REQ_FNF) + _T(" (SendHashsetPacket2)");
		}
		uint8 byOptions = data.ReadUInt8();
		bool bMD4 = (byOptions & 0x01) > 0;
		bool bAICH = (byOptions & 0x02) > 0;
		if (!bMD4 && !bAICH)
		{
			DebugLogWarning(_T("Client sent HashSet request with none or unknown HashSet type requested (%u) - file: %s, client %s")
				, byOptions, file->GetFileName(), DbgGetClientInfo());
			return;
		}
		file->GetFileIdentifier().WriteIdentifier(&fileResponse);
		// even if we don't happen to have an AICH hashset yet for some reason we send a proper (possible empty) response
		file->GetFileIdentifier().WriteHashSetsToPacket(&fileResponse, bMD4, bAICH); 
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__HashSetAnswer", this, file->GetFileIdentifier().GetMD4Hash());
		packet = new Packet(&fileResponse, OP_EMULEPROT, OP_HASHSETANSWER2);
	}
	else
	{
		if (nSize != 16)
		{
			ASSERT( false );
			return;
		}
		CKnownFile* file = theApp.sharedfiles->GetFileByID(pData);
		if (!file){
			CheckFailedFileIdReqs(pData);
			throw GetResString(IDS_ERR_REQ_FNF) + _T(" (SendHashsetPacket)");
		}
		file->GetFileIdentifier().WriteMD4HashsetToFile(&fileResponse);
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__HashSetAnswer", this, pData);
		packet = new Packet(&fileResponse, OP_EDONKEYPROT, OP_HASHSETANSWER);
	}
	theStats.AddUpDataOverheadFileRequest(packet->size);
	SendPacket(packet, true);
}

void CUpDownClient::SendRankingInfo(){
	if (!ExtProtocolAvailable())
		return;
	UINT nRank = theApp.uploadqueue->GetWaitingPosition(this);
	if (!nRank)
		return;
	Packet* packet = new Packet(OP_QUEUERANKING,12,OP_EMULEPROT);
	PokeUInt16(packet->pBuffer+0, (uint16)nRank);
	memset(packet->pBuffer+2, 0, 10);
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__QueueRank", this);
	theStats.AddUpDataOverheadFileRequest(packet->size);
	SendPacket(packet, true);
}

void CUpDownClient::SendCommentInfo(/*const*/ CKnownFile *file)
{
	if (!m_bCommentDirty || file == NULL || !ExtProtocolAvailable() || m_byAcceptCommentVer < 1)
		return;
	m_bCommentDirty = false;

	UINT rating = file->GetFileRating();
	const CString& desc = file->GetFileComment();
	if (file->GetFileRating() == 0 && desc.IsEmpty())
		return;

	CSafeMemFile data(256);
	data.WriteUInt8((uint8)rating);
	data.WriteLongString(desc, GetUnicodeSupport());
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP__FileDesc", this, file->GetFileHash());
	Packet *packet = new Packet(&data,OP_EMULEPROT);
	packet->opcode = OP_FILEDESC;
	theStats.AddUpDataOverheadFileRequest(packet->size);
	SendPacket(packet, true);
}

void CUpDownClient::AddRequestCount(const uchar* fileid)
{
	for (POSITION pos = m_RequestedFiles_list.GetHeadPosition(); pos != 0; ){
		Requested_File_Struct* cur_struct = m_RequestedFiles_list.GetNext(pos);
		if (!md4cmp(cur_struct->fileid,fileid)){
			if (::GetTickCount() - cur_struct->lastasked < MIN_REQUESTTIME && !GetFriendSlot()){ 
				if (GetDownloadState() != DS_DOWNLOADING) {
			   		// morph some extra suprious verbose tracking, read http://forum.emule-project.net/index.php?showtopic=136682
					cur_struct->badrequests++;
					DebugLogError( _T("Client: %s (%s), Increased bad request to %d"), GetUserName(), ipstr(GetConnectIP()),cur_struct->badrequests);
				}
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
	theApp.clientlist->RemoveBannedClient(GetIP());
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
	SetChatState(MS_NONE);
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
	if (!IsBanned()){
		if (thePrefs.GetLogBannedClients())
			AddDebugLogLine(false,_T("Banned: %s; %s"), pszReason==NULL ? _T("Aggressive behaviour") : pszReason, DbgGetClientInfo());
	}
#ifdef _DEBUG
	else{
		if (thePrefs.GetLogBannedClients())
			AddDebugLogLine(false,_T("Banned: (refreshed): %s; %s"), pszReason==NULL ? _T("Aggressive behaviour") : pszReason, DbgGetClientInfo());
	}
#endif
	theApp.clientlist->AddBannedClient(GetIP());
	SetUploadState(US_BANNED);
	theApp.emuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	theApp.emuledlg->transferwnd->GetQueueList()->RefreshClient(this);
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
		//AddDebugLogLine(false,GetResString(IDS_ANTILEECHERLOG) + _T(" (%s)"),DbgGetClientInfo(),pszReason==NULL ? _T("No Reason") : pszReason);
		DebugLog(LOG_MORPH|LOG_WARNING,_T("[%s]-(%s) Client %s"),pszReason==NULL ? _T("No Reason") : pszReason ,m_strNotOfficial ,DbgGetClientInfo());
	}
	theApp.clientlist->AddBannedClient( GetIP() );
	SetUploadState(US_BANNED);
	theApp.emuledlg->transferwnd->ShowQueueCount(theApp.uploadqueue->GetWaitingUserCount());
	theApp.emuledlg->transferwnd->GetQueueList()->RefreshClient(this);
	if (socket != NULL && socket->IsConnected())
		socket->ShutDown(SD_RECEIVE); // let the socket timeout, since we dont want to risk to delete the client right now. This isnt acutally perfect, could be changed later
}
//MORPH END   - Added by IceCream, Anti-leecher feature

// Moonlight: SUQWT - Compare linear time instead of time indexes to avoid overflow-induced false positives.//Morph - added by AndCycle, Moonlight's Save Upload Queue Wait Time (MSUQWT)
//EastShare START - Modified by TAHO, modified SUQWT
/*
uint32 CUpDownClient::GetWaitStartTime() const
*/
sint64 CUpDownClient::GetWaitStartTime() const
//EastShare END - Modified by TAHO, modified SUQWT
{
	if (credits == NULL){
		ASSERT ( false );
		return 0;
	}
	//EastShare START - Modified by TAHO, modified SUQWT
	/*
	uint32 dwResult = credits->GetSecureWaitStartTime(GetIP());
	if (dwResult > m_dwUploadTime && IsDownloading()){
	*/
	sint64 dwResult = credits->GetSecureWaitStartTime(GetIP());
	uint32 now = ::GetTickCount();
	if ( dwResult > now) { 
		dwResult = now - 1;
	}
	//MORPH START - Changed by SiRoB, Moonlight's Save Upload Queue Wait Time (MSUQWT)
	/*
	uint32 dwTicks = ::GetTickCount();
	if ((!theApp.clientcredits->IsSaveUploadQueueWaitTime() && (dwResult > m_dwUploadTime) ||
		theApp.clientcredits->IsSaveUploadQueueWaitTime() && ((int)(m_dwUploadTime - dwResult) < 0))
		&& IsDownloading()){
	*/
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
			//MORPH - Changed by SiRoB, Code Optimization
			/*
			case IS_IDFAILED:
			case IS_IDNEEDED:
			case IS_IDBADGUY:
				return false;
			*/
			case IS_NOTAVAILABLE:
			case IS_IDENTIFIED:
				return m_bFriendSlot;
		}
		return false;
	}
	return m_bFriendSlot;
}

CEMSocket* CUpDownClient::GetFileUploadSocket(bool bLog)
{
    if (m_pPCUpSocket && (IsUploadingToPeerCache() || m_ePeerCacheUpState == PCUS_WAIT_CACHE_REPLY))
	{
        if (bLog && thePrefs.GetVerbose())
            AddDebugLogLine(false, _T("%s got peercache socket."), DbgGetClientInfo());
        return m_pPCUpSocket;
    }
	else
	{
        if (bLog && thePrefs.GetVerbose())
            AddDebugLogLine(false, _T("%s got normal socket."), DbgGetClientInfo());
        return socket;
    }
}

void CUpDownClient::SetCollectionUploadSlot(bool bValue){
	ASSERT( !IsDownloading() || bValue == m_bCollectionUploadSlot );
	m_bCollectionUploadSlot = bValue;
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
	if (tempUpCurrentTotalTime > 999)
		return	GetTransferredUp()/(tempUpCurrentTotalTime/1000);
	else
		return 0;
}
//MORPH END  - Added by SIRoB, GetAverage Upload to client

//MORPH START - Added by SiRoB, ShareOnlyTheNeed hide Uploaded and uploading part
void CUpDownClient::GetUploadingAndUploadedPart(uint8* m_abyUpPartUploadingAndUploaded, uint32 partcount) const
{
	memset(m_abyUpPartUploadingAndUploaded,0,partcount);
	const Requested_Block_Struct* block;
	UploadingToClient_Struct* pUpClientStruct = theApp.uploadqueue->GetUploadingClientStructByClient(this);
	ASSERT(pUpClientStruct != NULL);
	if (pUpClientStruct != NULL)
	{
		CSingleLock lockBlockLists(&pUpClientStruct->m_csBlockListsLock, TRUE);
		ASSERT(lockBlockLists.IsLocked());
		if (!pUpClientStruct->m_BlockRequests_queue.IsEmpty()) {
			block = pUpClientStruct->m_BlockRequests_queue.GetHead();
			if (block) {
				uint32 start = (UINT)(block->StartOffset / PARTSIZE);
				m_abyUpPartUploadingAndUploaded[start] = 1;
			}
		}
		if (!pUpClientStruct->m_DoneBlocks_list.IsEmpty()) {
			for (POSITION pos = pUpClientStruct->m_DoneBlocks_list.GetHeadPosition(); pos != 0;) {
				block = pUpClientStruct->m_DoneBlocks_list.GetNext(pos);
				uint32 start = (UINT)(block->StartOffset / PARTSIZE);
				m_abyUpPartUploadingAndUploaded[start] = 1;
			}
		}
		lockBlockLists.Unlock();
	}
}
//MORPH END   - Added by SiRoB, ShareOnlyTheNeed hide Uploaded and uploading part
//MORPH START - Added by SiRoB, Optimization requpfile
CKnownFile* CUpDownClient::CheckAndGetReqUpFile() const {
	if (requpfileid_lasttimeupdated < theApp.sharedfiles->GetLastTimeFileMapUpdated()) {
		return theApp.sharedfiles->GetFileByID(requpfileid);
		//requpfileid_lasttimeupdated = theApp.sharedfiles->GetLastTimeFileMapUpdated();
	}
	return requpfile;
}
//MORPH END   - Added by SiRoB, Optimization requpfile

//MORPH START - Changed by SiRoB, ReadBlockFromFileThread
#ifdef USE_MORPH_READ_THREAD
IMPLEMENT_DYNCREATE(CReadBlockFromFileThread, CWinThread)

void CReadBlockFromFileThread::SetReadBlockFromFile(LPCTSTR filepath, uint64 startOffset, uint32 toread, CUpDownClient* client, CSyncObject* lockhandle) {
	fullname = filepath;
	StartOffset = startOffset;
	togo = toread;
	m_client = client;
	m_clientname = m_client->GetUserName(); //Fafner: avoid possible crash - 080421
	m_lockhandle = lockhandle;
	pauseEvent.SetEvent();
} 

void CReadBlockFromFileThread::StopReadBlock() {
	doRun = false;
	pauseEvent.SetEvent();
} 

int CReadBlockFromFileThread::Run() {
	DbgSetThreadName("CReadBlockFromFileThread");
	
	//InitThreadLocale();
	// SLUGFILLER: SafeHash
	CReadWriteLock lock(&theApp.m_threadlock);
	if (!lock.ReadLock(0))
		return 0;
	// SLUGFILLER: SafeHash

	CFile file;
	byte* filedata = NULL;
	doRun = true;
	pauseEvent.Lock();
	while(doRun) {
		CSyncHelper lockFile;
		try{
			if (m_lockhandle) {
				lockFile.m_pObject = m_lockhandle;
				if (m_lockhandle->Lock(1000) == 0) { //Fafner: Lock() == Lock(INFINITE): waits forever - 080421
					CString str;
					str.Format(_T("file is locked: %s"), fullname);
					throw str;
				}
			}
			
			if (!file.Open(fullname,CFile::modeRead|CFile::osSequentialScan|CFile::shareDenyNone))
				throw GetResString(IDS_ERR_OPEN);

			file.Seek(StartOffset,0);
				
			filedata = new byte[togo+500];
			if (uint32 done = file.Read(filedata,togo) != togo){
				file.SeekToBegin();
				file.Read(filedata + done,togo-done);
			}
			file.Close();
			
			if (lockFile.m_pObject){
				lockFile.m_pObject->Unlock(); // Unlock the (part) file as soon as we are done with accessing it.
				lockFile.m_pObject = NULL;
			}
			
			if (theApp.emuledlg && theApp.emuledlg->IsRunning())
				PostMessage(theApp.emuledlg->m_hWnd,TM_READBLOCKFROMFILEDONE, (WPARAM)filedata,(LPARAM)m_client);
			else {
				delete[] filedata;
				filedata = NULL;
			}
		}
		catch(CString error)
		{
			if (lockFile.m_pObject) { //Fafner: missing? - 080421
				lockFile.m_pObject->Unlock(); // Unlock the (part) file as soon as we are done with accessing it.
				lockFile.m_pObject = NULL;
			}
			if (thePrefs.GetVerbose())
				theApp.QueueDebugLogLine(false,GetResString(IDS_ERR_CLIENTERRORED), m_clientname, error); //Fafner: avoid possible crash - 080421
			if (theApp.emuledlg && theApp.emuledlg->IsRunning())
				PostMessage(theApp.emuledlg->m_hWnd,TM_READBLOCKFROMFILEDONE,(WPARAM)-1,(LPARAM)m_client);
			else if (filedata != (byte*)-1 && filedata != (byte*)-2 && filedata != NULL) {
				delete[] filedata;
				filedata = NULL;
			}
			return 1;
		}
		catch(CFileException* e)
		{
			TCHAR szError[MAX_CFEXP_ERRORMSG];
			e->GetErrorMessage(szError, ARRSIZE(szError));
			if (lockFile.m_pObject) { //Fafner: missing? - 080421
				lockFile.m_pObject->Unlock(); // Unlock the (part) file as soon as we are done with accessing it.
				lockFile.m_pObject = NULL;
			}
			if (thePrefs.GetVerbose())
				theApp.QueueDebugLogLine(false,_T("Failed to create upload package for %s - %s"), m_clientname, szError); //Fafner: avoid possible crash - 080421
			if (theApp.emuledlg && theApp.emuledlg->IsRunning())
				PostMessage(theApp.emuledlg->m_hWnd,TM_READBLOCKFROMFILEDONE,(WPARAM)-1,(LPARAM)m_client);
			else if (filedata != (byte*)-1 && filedata != (byte*)-2 && filedata != NULL) {
				delete[] filedata;
				filedata = NULL;
			}
			e->Delete();
			return 2;
		}
		catch(...)
		{
			if (lockFile.m_pObject) { //Fafner: missing? - 080421
				lockFile.m_pObject->Unlock(); // Unlock the (part) file as soon as we are done with accessing it.
				lockFile.m_pObject = NULL;
			}
			if (theApp.emuledlg && theApp.emuledlg->IsRunning())
				PostMessage(theApp.emuledlg->m_hWnd,TM_READBLOCKFROMFILEDONE,(WPARAM)-1,(LPARAM)m_client);
			else if (filedata != (byte*)-1 && filedata != (byte*)-2 && filedata != NULL) {
				delete[] filedata;
				filedata = NULL;
			}
			return 3;
		}
		
		pauseEvent.Lock();
	}
	return 0;
}
#endif
//MORPH END    - Changed by SiRoB, ReadBlockFromFileThread