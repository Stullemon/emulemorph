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
#include <io.h>
#include "emule.h"
#include "DownloadQueue.h"
#include "UpDownClient.h"
#include "PartFile.h"
#include "ed2kLink.h"
#include "SearchList.h"
#include "ClientList.h"
#include "Statistics.h"
#include "SharedFileList.h"
#include "OtherFunctions.h"
#include "SafeFile.h"
#include "Sockets.h"
#include "ServerList.h"
#include "Server.h"
#include "Packets.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "kademlia/utils/uint128.h"
#include "ipfilter.h"
#include "emuledlg.h"
#include "TransferWnd.h"
#include "TaskbarNotifier.h"
#include "MenuCmds.h"
#include "UploadQueue.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CDownloadQueue::CDownloadQueue(CSharedFileList* in_sharedfilelist)
{
	sharedfilelist = in_sharedfilelist;
	filesrdy = 0;
	datarate = 0;
	cur_udpserver = 0;
	lastfile = 0;
	lastcheckdiskspacetime = 0;	// SLUGFILLER: checkDiskspace
	lastudpsearchtime = 0;
	lastudpstattime = 0;
	SetLastKademliaFileRequest();
	udcounter = 0;
	m_iSearchedServers = 0;
	m_datarateMS=0;
	m_nUDPFileReasks = 0;
	m_nFailedUDPFileReasks = 0;
	m_dwNextTCPSrcReq = 0;
	m_cRequestsSentToServer = 0;
    m_dwLastA4AFtime = 0; // ZZ:DownloadManager
	// khaos::categorymod+
	m_iLastLinkQueuedTick = 0;
	m_bBusyPurgingLinks = false;
	m_ED2KLinkQueue.RemoveAll();
	// khaos::categorymod-
	// khaos::kmod+ A4AF
	forcea4af_file = NULL;
	// khaos::kmod-
}

void CDownloadQueue::AddPartFilesToShare()
{
	for (POSITION pos = filelist.GetHeadPosition(); pos != 0; )
	{
		CPartFile* cur_file = filelist.GetNext(pos);
		if (cur_file->GetStatus(true) == PS_READY)
			sharedfilelist->SafeAddKFile(cur_file,true);
	}
}

// SLUGFILLER: mergeKnown
void CDownloadQueue::SavePartFilesToKnown(CFileDataIO* file){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		CKnownFile* cur_file = (CKnownFile*)filelist.GetAt(pos);	// Write as known files
			cur_file->WriteToFile(file);
		}
}

uint32 CDownloadQueue::GetPartFilesCount(){
	return filelist.GetCount();
}
// SLUGFILLER: mergeKnown

void CDownloadQueue::Init(){
	// find all part files, read & hash them if needed and store into a list
	CFileFind ff;
	int count = 0;

	CString searchPath(thePrefs.GetTempDir());
	searchPath += "\\*.part.met";

	//check all part.met files
	bool end = !ff.FindFile(searchPath, 0);
	while (!end){
		end = !ff.FindNextFile();
		if (ff.IsDirectory())
			continue;
		CPartFile* toadd = new CPartFile();
		if (toadd->LoadPartFile(thePrefs.GetTempDir(),ff.GetFileName().GetBuffer())){
			count++;
			filelist.AddTail(toadd);			// to downloadqueue
			// SLUGFILLER: SafeHash remove - part files are shared later
			theApp.emuledlg->transferwnd->downloadlistctrl.AddFile(toadd);// show in downloadwindow
		}
		else
			delete toadd;
	}
	ff.Close();

	//try recovering any part.met files
	searchPath += ".backup";
	end = !ff.FindFile(searchPath, 0);
	while (!end){
		end = !ff.FindNextFile();
		if (ff.IsDirectory())
			continue;
		CPartFile* toadd = new CPartFile();
		if (toadd->LoadPartFile(thePrefs.GetTempDir(),ff.GetFileName().GetBuffer())){
			toadd->SavePartFile(); // resave backup
			count++;
			filelist.AddTail(toadd);			// to downloadqueue
			// SLUGFILLER: SafeHash remove - part files are shared later
			theApp.emuledlg->transferwnd->downloadlistctrl.AddFile(toadd);// show in downloadwindow

			AddLogLine(false, GetResString(IDS_RECOVERED_PARTMET), toadd->GetFileName());
		}
		else {
			delete toadd;
		}
	}
	ff.Close();

	if(count == 0) {
		AddLogLine(false,GetResString(IDS_NOPARTSFOUND));
	} else {
		AddLogLine(false,GetResString(IDS_FOUNDPARTS),count);
		SortByPriority();
		CheckDiskspace();	// SLUGFILLER: checkDiskspace
	}
	VERIFY( m_srcwnd.CreateEx(0, AfxRegisterWndClass(0),_T("Hostname Resolve Wnd"),WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL));

	ExportPartMetFilesOverview();
}

CDownloadQueue::~CDownloadQueue(){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;)
		delete filelist.GetNext(pos);
	m_srcwnd.DestroyWindow(); // just to avoid a MFC warning
}

// khaos::categorymod+
// New Param: uint16 useOrder (Def: 0)
void CDownloadQueue::AddSearchToDownload(CSearchFile* toadd,uint8 paused,uint8 cat, uint16 useOrder){
	if (IsFileExisting(toadd->GetFileHash()))
		return;
	CPartFile* newfile = new CPartFile(toadd);
	if (newfile->GetStatus() == PS_ERROR){
		delete newfile;
		return;
	}
	newfile->SetCategory(cat);
	newfile->SetCatResumeOrder(useOrder);
	if (paused == 2)
		paused = (uint8)thePrefs.AddNewFilesPaused();
	AddDownload(newfile, (paused==1));

	// If the search result is from OP_GLOBSEARCHRES there may also be a source
	if (toadd->GetClientID() && toadd->GetClientPort()){
		CSafeMemFile sources(1+4+2);
		try{
			sources.WriteUInt8(1);
			sources.WriteUInt32(toadd->GetClientID());
			sources.WriteUInt16(toadd->GetClientPort());
		    sources.SeekToBegin();
		    newfile->AddSources(&sources, toadd->GetClientServerIP(), toadd->GetClientServerPort());
		}
		catch(CFileException* error){
			ASSERT(0);
			error->Delete();
		}
	}

	// Add more sources which were found via global UDP search
	const CSimpleArray<CSearchFile::SClient>& aClients = toadd->GetClients();
	for (int i = 0; i < aClients.GetSize(); i++){
		CSafeMemFile sources(1+4+2);
		try{
			sources.WriteUInt8(1);
			sources.WriteUInt32(aClients[i].m_nIP);
			sources.WriteUInt16(aClients[i].m_nPort);
		    sources.SeekToBegin();
			newfile->AddSources(&sources,aClients[i].m_nServerIP, aClients[i].m_nServerPort);
	    }
		catch(CFileException* error){
			ASSERT(0);
			error->Delete();
			break;
		}
	}
	//Morph Start - added by AndCycle, itsonlyme: cacheUDPsearchResults
	// itsonlyme: cacheUDPsearchResults
	const CSimpleArray<CSearchFile::SServer>& aServers = toadd->GetServers();
	for (int i = 0; i < aServers.GetSize(); i++){
		CPartFile::SServer tmpServer(aServers[i].m_nIP, aServers[i].m_nPort);
		tmpServer.m_uAvail = aServers[i].m_uAvail;
		newfile->AddAvailServer(tmpServer);
		AddDebugLogLine(false, "Caching server with %i sources for %s", aServers[i].m_uAvail, newfile->GetFileName());
	}
	// itsonlyme: cacheUDPsearchResults
	//Morph End - added by AndCycle, itsonlyme: cacheUDPsearchResults
}

// New Param: uint16 useOrder (Def: 0)
void CDownloadQueue::AddSearchToDownload(CString link,uint8 paused, uint8 cat, uint16 useOrder){
	CPartFile* newfile = new CPartFile(link);
	if (newfile->GetStatus() == PS_ERROR){
		delete newfile;
		return;
	}
	newfile->SetCategory(cat);
	newfile->SetCatResumeOrder(useOrder); // Added
	if (paused == 2)
		paused = (uint8)thePrefs.AddNewFilesPaused();
	AddDownload(newfile, (paused==1));
}

// New parameter: int Category = -1 By Default
// This entire function was rewritten.
bool CDownloadQueue::StartNextFile(int cat , bool force){
	
	CPartFile*  pfile = NULL;
	

	// Standard operation of StartNextFile is supported by passing -1 for Category...
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		if (cur_file->GetStatus() == PS_PAUSED)
		{
			if (!force && thePrefs.GetCategory(cur_file->GetCategory())->bResumeFileOnlyInSameCat && cur_file->GetCategory()!=cat && cat!=-1) //MORPH START - Added by SiRoB, Resume file only in the same category
				continue;
			if (!pfile) {
				pfile = cur_file;
			}
			else {
				if (pfile->GetCategory()==cat && thePrefs.GetResumeSameCat() && cur_file->GetCategory()!=cat  && cat!=-1)
					continue;
				else if (pfile->GetCategory()!=cat && thePrefs.GetResumeSameCat() && cur_file->GetCategory()==cat  && cat!=-1) {
					pfile = cur_file;
					if (pfile->GetDownPriority() == PR_HIGH && pfile->GetCatResumeOrder() == 0) break; continue;
				}
				else if (cur_file->GetCatResumeOrder() < pfile->GetCatResumeOrder()) {
					pfile = cur_file;
					if (pfile->GetDownPriority() == PR_HIGH && pfile->GetCatResumeOrder() == 0) break; continue;
				}
				else if (cur_file->GetCatResumeOrder() == pfile->GetCatResumeOrder() && (cur_file->GetDownPriority() > pfile->GetDownPriority())){
					pfile = cur_file; 
					if (pfile->GetDownPriority() == PR_HIGH && pfile->GetCatResumeOrder() == 0) break; continue;
				}
			}
		}
	}
	if (pfile) pfile->ResumeFile();
	return pfile;
}

// This function is used for the category commands Stop Last and Pause Last.
// This is a new function.
void CDownloadQueue::StopPauseLastFile(int Mode, int Category) {
	CPartFile*  pfile = NULL;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)) {
		CPartFile* cur_file = filelist.GetAt(pos);
		if ((cur_file->GetStatus() < 4) && (cur_file->GetCategory() == Category || Category == -1)) {
			if (!pfile)
				pfile = cur_file;
			else {
				if (cur_file->GetCatResumeOrder() > pfile->GetCatResumeOrder())
					pfile = cur_file;
				else if (cur_file->GetCatResumeOrder() == pfile->GetCatResumeOrder() && (cur_file->GetDownPriority() < pfile->GetDownPriority()))
					pfile = cur_file;
			}
		}
	}
	if (pfile) {
		Mode == MP_STOP ? pfile->StopFile() : pfile->PauseFile();
	}
}

// This function returns the highest resume order in a given category.
// It can be used to automatically assign a resume order to a partfile
// when it is added...  Or maybe it will have other uses in the future.
uint16 CDownloadQueue::GetMaxCatResumeOrder(uint8 iCategory /* = 0*/)
{
	uint16		max   = 0;
	
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos))
	{
		CPartFile* cur_file = filelist.GetAt(pos);
		if (cur_file->GetCategory() == iCategory && cur_file->GetCatResumeOrder() > max)
			max = cur_file->GetCatResumeOrder();
	}

	return max;
}

// This function has been modified in order
// to accomodate the category selection.
// NEW PARAM:  bool AllocatedLink = false by default
void CDownloadQueue::AddFileLinkToDownload(CED2KFileLink* pLink, int theCat, bool AllocatedLink)
{
	if (thePrefs.SelectCatForNewDL() && theCat==-1)
	{
		m_ED2KLinkQueue.AddTail(pLink);
		m_iLastLinkQueuedTick = GetTickCount();
		return;
	}

	int useCat = theCat;

	if (useCat == -1){
		if (thePrefs.UseAutoCat())
			useCat = theApp.downloadqueue->GetAutoCat(CString(pLink->GetName()), (ULONG)pLink->GetSize());
		else if (thePrefs.UseActiveCatForLinks())
			useCat = theApp.emuledlg->transferwnd->GetActiveCategory();
		else
			useCat = 0;
	}
	// Just in case...
	if (m_ED2KLinkQueue.GetCount() && !thePrefs.SelectCatForNewDL()) PurgeED2KLinkQueue();
	m_iLastLinkQueuedTick = 0;
	// khaos::categorymod-

	CPartFile* newfile = new CPartFile(pLink);

	if (newfile->GetStatus() == PS_ERROR){
		delete newfile;
		newfile=NULL;
	}
	else {
		// khaos::categorymod+ Pass useCat instead of cat and autoset resume order.
		newfile->SetCategory(useCat);
		if (thePrefs.SmallFileDLPush() && newfile->GetFileSize() < 154624)
			newfile->SetCatResumeOrder(0);
		else if (thePrefs.AutoSetResumeOrder())
			newfile->SetCatResumeOrder(GetMaxCatResumeOrder(useCat)+1);
		// khaos::categorymod-
		AddDownload(newfile,thePrefs.AddNewFilesPaused());
	}

	CPartFile* partfile = newfile;
	if (partfile == NULL)
		partfile = GetFileByID(pLink->GetHashKey());
	if (partfile)
	{
		if (pLink->HasValidSources())
			partfile->AddClientSources(pLink->SourcesList,1);
	}

	if(pLink->HasHostnameSources())
	{
		POSITION pos = pLink->m_HostnameSourcesList.GetHeadPosition();
		while (pos != NULL)
		{
			const SUnresolvedHostname* pUnresHost = pLink->m_HostnameSourcesList.GetNext(pos);
			m_srcwnd.AddToResolve(pLink->GetHashKey(), pUnresHost->strHostname, pUnresHost->nPort, pUnresHost->strURL);
		}
	}

	// khaos::categorymod+ Deallocate memory, because if we've gotten here,
	// this link wasn't added to the queue and therefore there's no reason to
	// not delete it.
	if (AllocatedLink) {
		delete pLink;
		pLink = NULL;
	}
	// khaos::categorymod-
}

// khaos::categorymod+ New function, used to add all of the
// ED2K links on the link queue to the downloads.  This is
// called when no new links have been added to the download
// list for half a second, or when there are queued links
// and the user has disabled the SelectCatForLinks feature.
bool CDownloadQueue::PurgeED2KLinkQueue()
{
	if (m_ED2KLinkQueue.IsEmpty()) return false;
	
	m_bBusyPurgingLinks = true;

	uint8	useCat;
	int		addedFiles = 0;
	bool	bCreatedNewCat = false;
	bool	bCanceled = false; //MORPH - Added by SiRoB, WasCanceled
	if (thePrefs.SelectCatForNewDL())
	{
		CSelCategoryDlg* getCatDlg = new CSelCategoryDlg((CWnd*)theApp.emuledlg);
		getCatDlg->DoModal();
		
		// Returns 0 on 'Cancel', otherwise it returns the selected category
		// or the index of a newly created category.  Users can opt to add the
		// links into a new category.
		useCat = getCatDlg->GetInput();
		bCreatedNewCat = getCatDlg->CreatedNewCat();
		bCanceled = getCatDlg->WasCancelled(); //MORPH - Added by SiRoB, WasCanceled
		delete getCatDlg;
	}
	else if (thePrefs.UseActiveCatForLinks())
		useCat = theApp.emuledlg->transferwnd->GetActiveCategory();
	else
		useCat = 0;

	int useOrder = GetMaxCatResumeOrder(useCat);

	for (POSITION pos = m_ED2KLinkQueue.GetHeadPosition(); pos != 0; m_ED2KLinkQueue.GetNext(pos))
	{
		CED2KFileLink*	pLink = m_ED2KLinkQueue.GetAt(pos);
		//MORPH START - Added by SiRoB, WasCanceled
		if (bCanceled) {
			delete pLink;
			pLink = NULL;
			continue;
		}
		//MORPH END   - Added by SiRoB, WasCanceled
		CPartFile*		newfile =	new CPartFile(pLink);
		
		if (newfile->GetStatus() == PS_ERROR) 
		{
			delete newfile;
			newfile = NULL;
		}
		else
		{
			if (!thePrefs.SelectCatForNewDL() && thePrefs.UseAutoCat())
			{
				useCat = GetAutoCat(CString(newfile->GetFileName()), (ULONG)newfile->GetFileSize());
				if (!useCat && thePrefs.UseActiveCatForLinks())
					useCat = theApp.emuledlg->transferwnd->GetActiveCategory();
			}
			newfile->SetCategory(useCat);
			if (thePrefs.SmallFileDLPush() && newfile->GetFileSize() < 154624)
				newfile->SetCatResumeOrder(0);
			else if (thePrefs.AutoSetResumeOrder()) {
				useOrder++;
				newfile->SetCatResumeOrder(useOrder);
			}
			AddDownload(newfile,thePrefs.AddNewFilesPaused());
			addedFiles++;
		}

		CPartFile* partfile = newfile;
		if (partfile == NULL)
			partfile = GetFileByID(pLink->GetHashKey());
		if (partfile)
		{
			if (pLink->HasValidSources())
				partfile->AddClientSources(pLink->SourcesList,1);
		}

		if(pLink->HasHostnameSources())
		{
			POSITION pos = pLink->m_HostnameSourcesList.GetHeadPosition();
			while (pos != NULL)
			{
				const SUnresolvedHostname* pUnresHost = pLink->m_HostnameSourcesList.GetNext(pos);
				m_srcwnd.AddToResolve(pLink->GetHashKey(), pUnresHost->strHostname, pUnresHost->nPort, pUnresHost->strURL);
			}
		}
		// We're done with this link.
		delete pLink;
		pLink = NULL;
	}
	
	m_ED2KLinkQueue.RemoveAll();

	// This bit of code will resume the number of files that the user specifies in preferences (Off by default)
	if (thePrefs.StartDLInEmptyCats() > 0 && bCreatedNewCat && thePrefs.AddNewFilesPaused())
		for (int i = 0; i < thePrefs.StartDLInEmptyCats(); i++)
			if (!StartNextFile(useCat)) break;

	m_bBusyPurgingLinks = false;
	m_iLastLinkQueuedTick = 0;
	return true;
}

// Returns statistics about a category's files' states.
void CDownloadQueue::GetCategoryFileCounts(uint8 iCategory, int cntFiles[])
{
	// cntFiles Array Indices:
	// 0 = Total
	// 1 = Transferring (Transferring Sources > 0)
	// 2 = Active (READY and EMPTY)
	// 3 = Unactive (PAUSED)
	// 4 = Complete (COMPLETE and COMPLETING)
	// 5 = Error (ERROR)
	// 6 = Other (Everything else...)
	for (int i = 0; i < 7; i++) cntFiles[i] = 0;

	for (POSITION pos = filelist.GetHeadPosition(); pos != NULL; filelist.GetNext(pos))
	{
		CPartFile* cur_file = filelist.GetAt(pos);
		if (cur_file->GetCategory() != iCategory) continue;

		cntFiles[0]++;
		if (cur_file->GetTransferingSrcCount() > 0) cntFiles[1]++;

		switch (cur_file->GetStatus(false))
		{
			case	PS_READY:
			case	PS_EMPTY:
				cntFiles[2]++;
				break;

			case	PS_PAUSED:
			//case	PS_INSUFFICIENT:
				cntFiles[3]++;
				break;

			case	PS_COMPLETING:
			case	PS_COMPLETE:
				cntFiles[4]++;
				break;

			case	PS_ERROR:
				cntFiles[5]++;
				break;
			
			case	PS_WAITINGFORHASH:
			case	PS_HASHING:
			case	PS_UNKNOWN:
				cntFiles[6]++;
				break;
		}
	}
}

// Returns the number of active files in a category.
uint16 CDownloadQueue::GetCatActiveFileCount(uint8 iCategory)
{
	uint16 iCount = 0;

	for (POSITION pos = filelist.GetHeadPosition(); pos != NULL; filelist.GetNext(pos))
	{
		CPartFile* cur_file = filelist.GetAt(pos);
		if (cur_file->GetCategory() != iCategory) continue;

		switch (cur_file->GetStatus(false))
		{
			case	PS_READY:
			case	PS_EMPTY:
			case	PS_COMPLETING:
				iCount++;
				break;
			default:
				break;
		}
	}

	return iCount;
}

// Returns the number of files in a category.
uint16 CDownloadQueue::GetCategoryFileCount(uint8 iCategory)
{
	uint16 iCount = 0;

	for (POSITION pos = filelist.GetHeadPosition(); pos != NULL; filelist.GetNext(pos))
	{
		CPartFile* cur_file = filelist.GetAt(pos);
		if (cur_file->GetCategory() == iCategory) iCount++;
	}

	return iCount;
}

// Returns the source count of the file with the highest available source count.
// nCat is optional and allows you to specify a certain category.
uint16 CDownloadQueue::GetHighestAvailableSourceCount(int nCat)
{
	uint16 nCount = 0;

	for (POSITION pos = filelist.GetHeadPosition(); pos != NULL; filelist.GetNext(pos))
	{
		CPartFile* curFile = filelist.GetAt(pos);
		if (nCount < curFile->GetAvailableSrcCount() && (nCat == -1 || curFile->GetCategory() == nCat))
			nCount = curFile->GetAvailableSrcCount();
	}

	return nCount;
}

// khaos::categorymod+ GetAutoCat returns a category index of a category
// that passes the filters.
// Idea by HoaX_69.
uint8 CDownloadQueue::GetAutoCat(CString sFullName, ULONG nFileSize)
{
	if (sFullName.IsEmpty())
		return 0;

	if (thePrefs.GetCatCount() <= 1)
		return 0;

	for (int ix = 1; ix < thePrefs.GetCatCount(); ix++)
	{ 
		Category_Struct* curCat = thePrefs.GetCategory(ix);
		if (!curCat->selectioncriteria.bAdvancedFilterMask && !curCat->selectioncriteria.bFileSize)
			continue;
		if (curCat->selectioncriteria.bAdvancedFilterMask && !ApplyFilterMask(sFullName, ix))
			continue;
		if (curCat->selectioncriteria.bFileSize && (nFileSize < curCat->viewfilters.nFSizeMin || (curCat->viewfilters.nFSizeMax == 0 || nFileSize > curCat->viewfilters.nFSizeMax)))
			continue;
		return ix;
	}

	return 0;
}

// Checks a part-file's "pretty filename" against a filter mask and returns
// true if it passes.  See read-me for details.
bool CDownloadQueue::ApplyFilterMask(CString sFullName, uint8 nCat)
{
	CString sFilterMask = thePrefs.GetCategory(nCat)->viewfilters.sAdvancedFilterMask;
	sFilterMask.Trim();

	if (sFilterMask == "")
		return false;

		sFullName.MakeLower();
	sFilterMask.MakeLower();

	if (sFilterMask.Left(1) == "<")
		{
			bool bPassedGlobal[3];
			bPassedGlobal[0] = false;
			bPassedGlobal[1] = true;
			bPassedGlobal[2] = false;

			for (int i = 0; i < 3; i++)
			{
				int iStart = 0;
				switch (i)
				{
			case 0: iStart = sFilterMask.Find("<all("); break;
			case 1: iStart = sFilterMask.Find("<any("); break;
			case 2: iStart = sFilterMask.Find("<none("); break;
				}

				if (iStart == -1)
				{
					bPassedGlobal[i] = true; // We need to do this since not all criteria are needed in order to match the category.
					continue; // Skip this criteria block.
				}

				i !=2 ? (iStart += 5) : (iStart += 6);

			int iEnd = sFilterMask.Find(")>", iStart);
			int iLT = sFilterMask.Find("<", iStart);
			int iGT = sFilterMask.Find(">", iStart);

				if (iEnd == -1 || (iLT != -1 && iLT < iEnd) || iGT < iEnd)
				{
				AddDebugLogLine(false, "Category '%s' has invalid Category Mask String.", thePrefs.GetCategory(nCat)->title);
					break; // Move on to next category.
				}
				if (iStart == iEnd)
				{
					bPassedGlobal[i] = true; // Just because this criteria block is empty doesn't mean the mask should fail.
					continue; // Skip this criteria block.
				}

			CString sSegment = sFilterMask.Mid(iStart, iEnd - iStart);

				int curPosBlock = 0;
				CString cmpSubBlock = sSegment.Tokenize(":", curPosBlock);

				while (cmpSubBlock != "")
				{
					bool bPassed = (i == 1) ? false : true;

					int curPosToken = 0;
					CString cmpSubStr = cmpSubBlock.Tokenize("|", curPosToken);

				while (cmpSubStr != "")
				{
					int cmpResult;

					if (cmpSubStr.Find("*") != -1 || cmpSubStr.Find("?") != -1)
						cmpResult = (wildcmp(cmpSubStr.GetBuffer(), sFullName.GetBuffer()) == 0) ? -1 : 1;
					else
						cmpResult = sFullName.Find(cmpSubStr);

					switch (i)
					{
							case 0:	if (cmpResult == -1) bPassed = false; break;
							case 1:	if (cmpResult != -1) bPassed = true; break;
							case 2:	if (cmpResult != -1) bPassed = false; break;
						}
						cmpSubStr = cmpSubBlock.Tokenize("|", curPosToken);
						}
					switch (i)
						{
						case 0:
						case 2: if (bPassed) bPassedGlobal[i] = true; break;
						case 1: if (!bPassed) bPassedGlobal[i] = false; break;
						}
					cmpSubBlock = sSegment.Tokenize(":", curPosBlock);
					}
				}
			for (int i = 0; i < 3; i++)
			if (!bPassedGlobal[i]) return false;
		return true;
		}
		else
		{
			int curPos = 0;
		CString cmpSubStr = sFilterMask.Tokenize("|", curPos);

			while (cmpSubStr != "")
			{
				int cmpResult;

				if (cmpSubStr.Find("*") != -1 || cmpSubStr.Find("?") != -1)
					cmpResult = (wildcmp(cmpSubStr.GetBuffer(), sFullName.GetBuffer()) == 0) ? -1 : 1;
				else
					cmpResult = sFullName.Find(cmpSubStr);

				if(cmpResult != -1)
				return true;
			cmpSubStr = sFilterMask.Tokenize("|", curPos);
			}
		}
	return false;
}
// khaos::categorymod-

void CDownloadQueue::AddDownload(CPartFile* newfile,bool paused) {
	// Barry - Add in paused mode if required
	if (paused)
		newfile->PauseFile();
	// khaos::accuratetimerem+
	else
		newfile->SetActivatedTick();
	// khaos::accuratetimerem-
	//MORPH - Removed by SiRoB, Due to Khaos Categorie
	/*
	SetAutoCat(newfile);// HoaX_69 / Slugfiller: AutoCat
	*/
	filelist.AddTail(newfile);
	SortByPriority();
	CheckDiskspace();	// SLUGFILLER: checkDiskspace
	theApp.emuledlg->transferwnd->downloadlistctrl.AddFile(newfile);
	AddLogLine(true,GetResString(IDS_NEWDOWNLOAD),newfile->GetFileName());
	CString msgTemp;
	msgTemp.Format(GetResString(IDS_NEWDOWNLOAD) + _T("\n"), newfile->GetFileName());
	theApp.emuledlg->ShowNotifier(msgTemp, TBN_DLOADADDED);
	ExportPartMetFilesOverview();
}

bool CDownloadQueue::IsFileExisting(const uchar* fileid, bool bLogWarnings)
{
	const CKnownFile* file = sharedfilelist->GetFileByID(fileid);
	if (file){
		if (bLogWarnings){
			if (file->IsPartFile())
				AddLogLine(true, GetResString(IDS_ERR_ALREADY_DOWNLOADING), file->GetFileName() );
			else
				AddLogLine(true, GetResString(IDS_ERR_ALREADY_DOWNLOADED), file->GetFileName() );
		}
		return true;
	}
	else if ((file = GetFileByID(fileid)) != NULL){
		if (bLogWarnings)
			AddLogLine(true, GetResString(IDS_ERR_ALREADY_DOWNLOADING), file->GetFileName() );
		return true;
	}
	return false;
}


//MORPH - Added by Yun.SF3, ZZ Upload System
void CDownloadQueue::Process(){
	
	ProcessLocalRequests(); // send src requests to local server

	uint32 downspeed = 0;

	// ZZ:UploadSpeedSense -->
    // Enforce a session ul:dl ratio of 1:3 no matter what upload speed.
    // The old ratio code is only used if the upload-queue is empty.
    // If the queue is empty, the client may not have gotten enough
    // requests, to be able to upload full configured speed. So we fallback
    // to old ratio-variant.
    // This new ratio check really needs to be here with the new more
    // powerful friends slot, to prevent the user from giving all his
    // bandwidth to just friends, and at the same time leaching from
    // all other clients.
    uint32 maxDownload = thePrefs.GetMaxDownload();

    if(theApp.uploadqueue->GetUploadQueueLength() <= 0) {
        // copied from prefs load
        if( thePrefs.GetMaxUpload() != 0 ){
            if(thePrefs.GetMaxUpload() < 4 && (thePrefs.GetMaxUpload()*3 < thePrefs.GetMaxDownload())) {
				maxDownload = thePrefs.GetMaxUpload()*3;
            }
    
            if(thePrefs.GetMaxUpload() < 10 && (thePrefs.GetMaxUpload()*4 < thePrefs.GetMaxDownload())) {
				maxDownload = thePrefs.GetMaxUpload()*4;
            }
		}
    }

	if ((maxDownload != UNLIMITED && datarate > 300) || thePrefs.IsZZRatioDoesWork()){
		downspeed = (maxDownload*1024*100)/(datarate+1); //(uint16)((float)((float)(thePrefs.GetMaxDownload()*1024)/(datarate+1)) * 100);
		if (downspeed < 50)
			downspeed = 50;
		else if (downspeed > 200)
			downspeed = 200;
	}
	// ZZ:UploadSpeedSense <--

    uint32 friendDownspeed = downspeed;
	bool tempIsZZRatioInWork = false; //MORPH - Added by SiRoB, ZZ Ratio in work

	if(theApp.uploadqueue->GetUploadQueueLength() > 0 && thePrefs.IsZZRatioDoesWork()) {
        // has this client downloaded more than it has uploaded this session? (friends excluded)
        // then limit its download speed from all clients but friends
        // limit will be removed as soon as upload has catched up to download
        if(theApp.stat_sessionReceivedBytes/3 > (theApp.stat_sessionSentBytes-theApp.stat_sessionSentBytesToFriend) &&
           datarate > 1500) {

            // calc allowed dl speed for rest of network (those clients that don't have
            // friend slots. Since you don't upload as much to them, you won't be able to download
            // as much from them. This will only be lower than friends speed if you are currently
            // uploading to a friend slot, otherwise they are the same.

            uint32 secondsNeededToEvenOut = (theApp.stat_sessionReceivedBytes/3-(theApp.stat_sessionSentBytes-theApp.stat_sessionSentBytesToFriend))/(theApp.uploadqueue->GetToNetworkDatarate()+1);
            uint32 tempDownspeed = max(min(3*100/max(secondsNeededToEvenOut, 1), 200), 30);

           if(downspeed == 0 || tempDownspeed < downspeed) {
                downspeed = tempDownspeed;
                //AddLogLine(true, "Limiting downspeed");
				tempIsZZRatioInWork = true; //MORPH - Added by SiRoB, ZZ Ratio in work
		   }
		}

        // has this client downloaded more than it has uploaded this session? (friends included)
        // then limit its download speed from all friends
        // limit will be removed as soon as upload has catched up to download
        if(theApp.stat_sessionReceivedBytes/3 > theApp.stat_sessionSentBytes &&
           datarate > 1500) {

            float secondsNeededToEvenOut = (theApp.stat_sessionReceivedBytes/3-theApp.stat_sessionSentBytes)/(theApp.uploadqueue->GetDatarate()+1);
			uint32 tempDownspeed = max(min(3*100/max(secondsNeededToEvenOut, 1), 200), 30);

			if(friendDownspeed == 0 || tempDownspeed < friendDownspeed) {
                friendDownspeed = tempDownspeed;
				tempIsZZRatioInWork = true; //MORPH - Added by SiRoB, ZZ Ratio in work
            }
        }
	}

	m_bIsZZRatioInWork = tempIsZZRatioInWork; //MORPH - Added by SiRoB, ZZ Ratio in work

	uint32 datarateX=0;
	udcounter++;

	//filelist is already sorted by prio, therefore I removed all the extra loops..
	for (POSITION pos =filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		if (cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY){
			datarateX += cur_file->Process(downspeed,udcounter,friendDownspeed);
		}
		else{
			//This will make sure we don't keep old sources to paused and stoped files..
			cur_file->StopPausedFile();
		}
	}

    datarate = datarateX;

	if (udcounter == 5){
		if (theApp.serverconnect->IsUDPSocketAvailable()){
			if((!lastudpstattime) || (::GetTickCount() - lastudpstattime) > UDPSERVERSTATTIME){
				lastudpstattime = ::GetTickCount();
				theApp.serverlist->ServerStats();
			}
		}
	}

	if (udcounter == 10){
		udcounter = 0;
		if (theApp.serverconnect->IsUDPSocketAvailable()){
			if ((!lastudpsearchtime) || (::GetTickCount() - lastudpsearchtime) > UDPSERVERREASKTIME)
				SendNextUDPPacket();
		}
	}

	// khaos::categorymod+ Purge ED2K Link Queue
	if (m_iLastLinkQueuedTick && !m_bBusyPurgingLinks && (GetTickCount() - m_iLastLinkQueuedTick) > 400)
		PurgeED2KLinkQueue();
	else if (m_ED2KLinkQueue.GetCount() && !thePrefs.SelectCatForNewDL()) // This should not happen.
	{
		PurgeED2KLinkQueue();
		AddDebugLogLine(false, "ERROR: Links in ED2K Link Queue while SelectCatForNewDL was disabled!");
	}
	// khaos::categorymod-

	CheckDiskspaceTimed();
}
 //MORPH - Added by Yun.SF3, ZZ Upload System

CPartFile* CDownloadQueue::GetFileByIndex(int index) const
{
	POSITION pos = filelist.FindIndex(index);
	if (pos)
		return filelist.GetAt(pos);
	return NULL;
}

CPartFile* CDownloadQueue::GetFileByID(const uchar* filehash) const
{
	for (POSITION pos = filelist.GetHeadPosition(); pos != 0; )
	{
		CPartFile* cur_file = filelist.GetNext(pos);
		if (!md4cmp(filehash,cur_file->GetFileHash()))
			return cur_file;
	}
	return NULL;
}

CPartFile* CDownloadQueue::GetFileByKadFileSearchID(uint32 id) const
{
	for (POSITION pos = filelist.GetHeadPosition(); pos != 0; )
	{
		CPartFile* cur_file = filelist.GetNext(pos);
		if (id == cur_file->GetKadFileSearchID())
			return cur_file;
	}
	return NULL;
}

bool CDownloadQueue::IsPartFile(const CKnownFile* file) const
{
	for (POSITION pos = filelist.GetHeadPosition(); pos != 0; )
	{
		if (file == filelist.GetNext(pos))
			return true;
	}
	return false;
}

// SLUGFILLER: SafeHash
bool CDownloadQueue::IsTempFile(const CString& rstrDirectory, const CString& rstrName) const
{
	// do not share a part file from the temp directory, if there is still a corresponding entry in
	// the download queue -- because that part file is not yet complete.
	CString othername = rstrName;
	int extpos = othername.ReverseFind(_T('.'));
	if (extpos == -1)
		return false;
	CString ext = othername.Mid(extpos);
	if (ext.CompareNoCase(_T(".met"))) {
		if (!ext.CompareNoCase(_T(".part")))
			othername += _T(".met");
		else if (!ext.CompareNoCase(PARTMET_BAK_EXT) || !ext.CompareNoCase(PARTMET_TMP_EXT))
			othername = othername.Left(extpos);
		else
			return false;
	}
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		if (!othername.CompareNoCase(cur_file->GetPartMetFileName()))
			return true;
	}

	return false;
}
// SLUGFILLER: SafeHash

bool CDownloadQueue::CheckAndAddSource(CPartFile* sender,CUpDownClient* source){
	if (sender->IsStopped()){
		delete source;
		return false;
	}

	if (source->HasValidHash())
	{
		if(!md4cmp(source->GetUserHash(), thePrefs.GetUserHash()))
		{
			if (thePrefs.GetVerbose())
				AddDebugLogLine(false, _T("Tried to add source with matching hash to your own."));
			delete source;
			return false;
		}
	}

	// "Filter LAN IPs" and/or "IPfilter" is not required here, because it was already done in parent functions

	// uses this only for temp. clients
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		for (POSITION pos2 = cur_file->srclist.GetHeadPosition();pos2 != 0; ){
			CUpDownClient* cur_client = cur_file->srclist.GetNext(pos2);
			if (cur_client->Compare(source, true) || cur_client->Compare(source, false)){
				if (cur_file == sender){ // this file has already this source
					delete source;
					return false;
				}
				// set request for this source
				if (cur_client->AddRequestForAnotherFile(sender)){
					theApp.emuledlg->transferwnd->downloadlistctrl.AddSource(sender,cur_client,true);
					delete source;
// ZZ:DownloadManager -->
                    if(cur_client->GetDownloadState() != DS_CONNECTED) {
                        cur_client->SwapToAnotherFile(_T("New A4AF source found. CDownloadQueue::CheckAndAddSource()"), false, false, false, NULL, true, false); // ZZ:DownloadManager
                    }
// <-- ZZ:DownloadManager
					return false;
				}
				else{
					delete source;
					return false;
				}
			}
		}
	}
	//our new source is real new but maybe it is already uploading to us?
	//if yes the known client will be attached to the var "source"
	//and the old sourceclient will be deleted
	if (theApp.clientlist->AttachToAlreadyKnown(&source,0)){
#ifdef _DEBUG
		if (thePrefs.GetVerbose() && source->reqfile){
			// if a client sent us wrong sources (sources for some other file for which we asked but which we are also
			// downloading) we may get a little in trouble here when "moving" this source to some other partfile without
			// further checks and updates.
			if (md4cmp(source->reqfile->GetFileHash(), sender->GetFileHash()) != 0)
				AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddSource -- added potential wrong source (%u)(diff. filehash) to file \"%s\""), source->GetUserIDHybrid(), sender->GetFileName());
			if (source->reqfile->GetPartCount() != 0 && source->reqfile->GetPartCount() != sender->GetPartCount())
				AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddSource -- added potential wrong source (%u)(diff. partcount) to file \"%s\""), source->GetUserIDHybrid(), sender->GetFileName());
		}
#endif
		source->SetRequestFile(sender);
	}
	else{
		// here we know that the client instance 'source' is a new created client instance (see callers) 
		// which is therefor not already in the clientlist, we can avoid the check for duplicate client list entries 
		// when adding this client
		theApp.clientlist->AddClient(source,true);
	}
	
	if (source->GetFileRate()>0 || !source->GetFileComment().IsEmpty())
		sender->UpdateFileRatingCommentAvail();

#ifdef _DEBUG
	if (thePrefs.GetVerbose() && source->GetPartCount()!=0 && source->GetPartCount()!=sender->GetPartCount()){
		DEBUG_ONLY(AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddSource -- New added source (%u, %s) had still value in partcount"), source->GetUserIDHybrid(), sender->GetFileName()));
	}
#endif

	sender->srclist.AddTail(source);
	theApp.emuledlg->transferwnd->downloadlistctrl.AddSource(sender,source,false);
	return true;
}

bool CDownloadQueue::CheckAndAddKnownSource(CPartFile* sender,CUpDownClient* source){
	if (sender->IsStopped())
		return false;

	// "Filter LAN IPs" -- this may be needed here in case we are connected to the internet and are also connected
	// to a LAN and some client from within the LAN connected to us. Though this situation may be supported in future
	// by adding that client to the source list and filtering that client's LAN IP when sending sources to
	// a client within the internet.
	//
	// "IPfilter" is not needed here, because that "known" client was already IPfiltered when receiving OP_HELLO.
	if (!source->HasLowID()){
		uint32 nClientIP = ntohl(source->GetUserIDHybrid());
		if (!IsGoodIP(nClientIP)){ // check for 0-IP, localhost and LAN addresses
			if (thePrefs.GetLogFilteredIPs())
				AddDebugLogLine(false, _T("Ignored already known source with IP=%s"), ipstr(nClientIP));
			return false;
		}
	}

	// use this for client which are already know (downloading for example)
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		if (cur_file->srclist.Find(source)){
			if (cur_file == sender)
				return false;
			if (source->AddRequestForAnotherFile(sender))
				theApp.emuledlg->transferwnd->downloadlistctrl.AddSource(sender,source,true);
// ZZ:DownloadManager -->
                if(source->GetDownloadState() != DS_CONNECTED) {
                    source->SwapToAnotherFile(_T("New A4AF source found. CDownloadQueue::CheckAndAddKnownSource()"), false, false, false, NULL, true, false); // ZZ:DownloadManager
                }
// <-- ZZ:DownloadManager
			return false;
		}
	}
#ifdef _DEBUG
	if (thePrefs.GetVerbose() && source->reqfile){
		// if a client sent us wrong sources (sources for some other file for which we asked but which we are also
		// downloading) we may get a little in trouble here when "moving" this source to some other partfile without
		// further checks and updates.
		if (md4cmp(source->reqfile->GetFileHash(), sender->GetFileHash()) != 0)
			AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddKnownSource -- added potential wrong source (%u)(diff. filehash) to file \"%s\""), source->GetUserIDHybrid(), sender->GetFileName());
		if (source->reqfile->GetPartCount() != 0 && source->reqfile->GetPartCount() != sender->GetPartCount())
			AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddKnownSource -- added potential wrong source (%u)(diff. partcount) to file \"%s\""), source->GetUserIDHybrid(), sender->GetFileName());
	}
#endif
	source->SetRequestFile(sender);
	if (source->GetFileRate()>0 || !source->GetFileComment().IsEmpty())
		sender->UpdateFileRatingCommentAvail();
	sender->srclist.AddTail(source);
	source->SetSourceFrom(SF_PASSIVE);
#ifdef _DEBUG
	if (thePrefs.GetVerbose() && source->GetPartCount()!=0 && source->GetPartCount()!=sender->GetPartCount()){
		DEBUG_ONLY(AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddKnownSource -- New added source (%u, %s) had still value in partcount"), source->GetUserIDHybrid(), sender->GetFileName()));
	}
#endif

	theApp.emuledlg->transferwnd->downloadlistctrl.AddSource(sender,source,false);
	//UpdateDisplayedInfo();
	return true;
}

bool CDownloadQueue::RemoveSource(CUpDownClient* toremove, bool bDoStatsUpdate){
	bool removed = false;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		for (POSITION pos2 = cur_file->srclist.GetHeadPosition();pos2 != 0; cur_file->srclist.GetNext(pos2)){
			if (toremove == cur_file->srclist.GetAt(pos2)){
				cur_file->srclist.RemoveAt(pos2);
				
				removed = true;
				if ( bDoStatsUpdate ){
					cur_file->RemoveDownloadingSource(toremove);
					cur_file->UpdatePartsInfo();
				}
				break;
			}
		}
		if ( bDoStatsUpdate )
			cur_file->UpdateAvailablePartsCount();
	}
	
	// remove this source on all files in the downloadqueue who link this source
	// pretty slow but no way arround, maybe using a Map is better, but that's slower on other parts
	POSITION pos3, pos4;
	for(pos3 = toremove->m_OtherRequests_list.GetHeadPosition();(pos4=pos3)!=NULL;)
	{
		toremove->m_OtherRequests_list.GetNext(pos3);				
		POSITION pos5 = toremove->m_OtherRequests_list.GetAt(pos4)->A4AFsrclist.Find(toremove); 
		if(pos5)
		{ 
			toremove->m_OtherRequests_list.GetAt(pos4)->A4AFsrclist.RemoveAt(pos5);
			theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource(toremove,toremove->m_OtherRequests_list.GetAt(pos4));
			toremove->m_OtherRequests_list.RemoveAt(pos4);
		}
	}
	for(pos3 = toremove->m_OtherNoNeeded_list.GetHeadPosition();(pos4=pos3)!=NULL;)
	{
		toremove->m_OtherNoNeeded_list.GetNext(pos3);				
		POSITION pos5 = toremove->m_OtherNoNeeded_list.GetAt(pos4)->A4AFsrclist.Find(toremove); 
		if(pos5)
		{ 
			toremove->m_OtherNoNeeded_list.GetAt(pos4)->A4AFsrclist.RemoveAt(pos5);
			theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource(toremove,toremove->m_OtherNoNeeded_list.GetAt(pos4));
			toremove->m_OtherNoNeeded_list.RemoveAt(pos4);
		}
	}

	if (toremove->GetFileRate()>0 || !toremove->GetFileComment().IsEmpty())
		toremove->reqfile->UpdateFileRatingCommentAvail();

	toremove->SetDownloadState(DS_NONE);
	theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource(toremove,0);
	toremove->ResetFileStatusInfo();
	toremove->SetRequestFile(NULL);
	return removed;
}

void CDownloadQueue::RemoveFile(CPartFile* toremove)
{
	RemoveLocalServerRequest(toremove);

	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		if (toremove == filelist.GetAt(pos)){
			filelist.RemoveAt(pos);
			break;
		}
	}
	SortByPriority();
	CheckDiskspace();	// SLUGFILLER: checkDiskspace
	ExportPartMetFilesOverview();
}

void CDownloadQueue::DeleteAll(){
	POSITION pos;
	for (pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		cur_file->srclist.RemoveAll();
		// Barry - Should also remove all requested blocks
		// Don't worry about deleting the blocks, that gets handled 
		// when CUpDownClient is deleted in CClientList::DeleteAll()
		cur_file->RemoveAllRequestedBlocks();
	}
}

// Max. file IDs per UDP packet
// ----------------------------
// 576 - 30 bytes of header (28 for UDP, 2 for "E3 9A" edonkey proto) = 546 bytes
// 546 / 16 = 34
#define MAX_FILES_PER_UDP_PACKET	31	// 2+16*31 = 498 ... is still less than 512 bytes!!

#define MAX_REQUESTS_PER_SERVER		35

int CDownloadQueue::GetMaxFilesPerUDPServerPacket() const
{
	int iMaxFilesPerPacket;
	if (cur_udpserver && cur_udpserver->GetUDPFlags() & SRV_UDPFLG_EXT_GETSOURCES)
	{
		// get max. file ids per packet
		if (m_cRequestsSentToServer < MAX_REQUESTS_PER_SERVER)
			iMaxFilesPerPacket = min(MAX_FILES_PER_UDP_PACKET, MAX_REQUESTS_PER_SERVER - m_cRequestsSentToServer);
		else{
			ASSERT(0);
			iMaxFilesPerPacket = 0;
		}
	}
	else
		iMaxFilesPerPacket = 1;

	return iMaxFilesPerPacket;
}

bool CDownloadQueue::SendGlobGetSourcesUDPPacket(CSafeMemFile* data)
{
	bool bSentPacket = false;

	if (   cur_udpserver
		&& (theApp.serverconnect->GetCurrentServer() == NULL || 
			cur_udpserver != theApp.serverlist->GetServerByAddress(theApp.serverconnect->GetCurrentServer()->GetAddress(),theApp.serverconnect->GetCurrentServer()->GetPort())))
	{
		ASSERT( data->GetLength() > 0 && data->GetLength() % 16 == 0 );
		int iFileIDs = data->GetLength() / 16;
		if (thePrefs.GetDebugServerUDPLevel() > 0)
			Debug(_T(">>> Sending OP__GlobGetSources to server(#%02x) %-15s (%3u of %3u); FileIDs=%u\n"), cur_udpserver->GetUDPFlags(), cur_udpserver->GetAddress(), m_iSearchedServers + 1, theApp.serverlist->GetServerCount(), iFileIDs);
		Packet packet(data);
		packet.opcode = OP_GLOBGETSOURCES;
		theStats.AddUpDataOverheadServer(packet.size);
		theApp.serverconnect->SendUDPPacket(&packet,cur_udpserver,false);
		
		m_cRequestsSentToServer += iFileIDs;
		bSentPacket = true;
	}

	return bSentPacket;
}

bool CDownloadQueue::SendNextUDPPacket()
{
	if (   filelist.IsEmpty() 
        || !theApp.serverconnect->IsUDPSocketAvailable() 
        || !theApp.serverconnect->IsConnected())
		return false;
	if (!cur_udpserver){
		if ((cur_udpserver = theApp.serverlist->GetNextServer(cur_udpserver)) == NULL){
			TRACE("ERROR:SendNextUDPPacket() no server found\n");
			StopUDPRequests();
		};
		m_cRequestsSentToServer = 0;
	}

	// get max. file ids per packet for current server
	int iMaxFilesPerPacket = GetMaxFilesPerUDPServerPacket();

	// loop until the packet is filled or a packet was sent
	bool bSentPacket = false;
	CSafeMemFile dataGlobGetSources(16);
	int iFiles = 0;
	while (iFiles < iMaxFilesPerPacket && !bSentPacket)
	{
		// get next file to search sources for
		CPartFile* nextfile = NULL;
		while (!bSentPacket && !(nextfile && (nextfile->GetStatus() == PS_READY || nextfile->GetStatus() == PS_EMPTY)))
		{
			if (lastfile == NULL) // we just started the global source searching or have switched the server
			{
				// get first file to search sources for
				nextfile = filelist.GetHead();
				lastfile = nextfile;
			}
			else
			{
				POSITION pos = filelist.Find(lastfile);
				if (pos == 0) // the last file is no longer in the DL-list (may have been finished or canceld)
				{
					// get first file to search sources for
					nextfile = filelist.GetHead();
					lastfile = nextfile;
				}
				else
				{
					filelist.GetNext(pos);
					if (pos == 0) // finished asking the current server for all files
					{
						// if there are pending requests for the current server, send them
						if (dataGlobGetSources.GetLength() > 0)
						{
							if (SendGlobGetSourcesUDPPacket(&dataGlobGetSources))
								bSentPacket = true;
							dataGlobGetSources.SetLength(0);
						}

						// get next server to ask
						//Morph Start - added by AndCycle, itsonlyme: cacheUDPsearchResults
						/*
						cur_udpserver = theApp.serverlist->GetNextServer(cur_udpserver);
						*/
						cur_udpserver = theApp.serverlist->GetNextServer(cur_udpserver, lastfile);	// itsonlyme: cacheUDPsearchResults
						//Morph End - added by AndCycle, itsonlyme: cacheUDPsearchResults
						m_cRequestsSentToServer = 0;
						if (cur_udpserver == NULL)
						{
							// finished asking all servers for all files
							if (thePrefs.GetDebugServerUDPLevel() > 0 && thePrefs.GetDebugServerSourcesLevel() > 0)
								Debug(_T("Finished UDP search processing for all servers (%u)\n"), theApp.serverlist->GetServerCount());

							lastudpsearchtime = ::GetTickCount();
							lastfile = NULL;
							m_iSearchedServers = 0;
							return false; // finished (processed all file & all servers)
						}
						m_iSearchedServers++;

						// if we already sent a packet, switch to the next file at next function call
						if (bSentPacket){
							lastfile = NULL;
							break;
						}

						// get max. file ids per packet for current server
						iMaxFilesPerPacket = GetMaxFilesPerUDPServerPacket();

						// have selected a new server; get first file to search sources for
						nextfile = filelist.GetHead();
						lastfile = nextfile;
					}
					else
					{
						nextfile = filelist.GetAt(pos);
						lastfile = nextfile;
					}
				}
			}
		}

		if (!bSentPacket && nextfile && nextfile->GetSourceCount() < thePrefs.GetMaxSourcePerFileUDP())
		{
			dataGlobGetSources.WriteHash16(nextfile->GetFileHash());
			iFiles++;
			if (thePrefs.GetDebugServerUDPLevel() > 0 && thePrefs.GetDebugServerSourcesLevel() > 0)
				Debug(_T(">>> Queued  OP__GlobGetSources to server(#%02x) %-15s (%3u of %3u); Buff  %u=%s\n"), cur_udpserver->GetUDPFlags(), cur_udpserver->GetAddress(), m_iSearchedServers + 1, theApp.serverlist->GetServerCount(), iFiles, DbgGetFileInfo(nextfile->GetFileHash()));
		}
	}

	ASSERT( dataGlobGetSources.GetLength() == 0 || !bSentPacket );

	if (!bSentPacket && dataGlobGetSources.GetLength() > 0)
		SendGlobGetSourcesUDPPacket(&dataGlobGetSources);

	// send max 35 UDP request to one server per interval
	// if we have more than 35 files, we rotate the list and use it as queue
	if (m_cRequestsSentToServer >= MAX_REQUESTS_PER_SERVER)
	{
		if (thePrefs.GetDebugServerUDPLevel() > 0 && thePrefs.GetDebugServerSourcesLevel() > 0)
			Debug(_T("Rotating file list\n"));

		// move the last 35 files to the head
		if (filelist.GetCount() >= MAX_REQUESTS_PER_SERVER){
			for (int i = 0; i != MAX_REQUESTS_PER_SERVER; i++){
				filelist.AddHead( filelist.RemoveTail() );
			}
		}

		// and next server
		cur_udpserver = theApp.serverlist->GetNextServer(cur_udpserver);
		m_cRequestsSentToServer = 0;
		if (cur_udpserver == NULL){
			lastudpsearchtime = ::GetTickCount();
			lastfile = NULL;
			return false; // finished (processed all file & all servers)
		}
		m_iSearchedServers++;
		lastfile = NULL;
	}

	return true;
}

void CDownloadQueue::StopUDPRequests(){
	cur_udpserver = 0;
	lastudpsearchtime = ::GetTickCount();
	lastfile = 0;
}

// SLUGFILLER: checkDiskspace
bool CDownloadQueue::CompareParts(POSITION pos1, POSITION pos2){
	CPartFile* file1 = filelist.GetAt(pos1);
	CPartFile* file2 = filelist.GetAt(pos2);
	if (file1->GetDownPriority() == file2->GetDownPriority())
		return file1->GetPartMetFileName().CompareNoCase(file2->GetPartMetFileName())>=0;
	if (file1->GetDownPriority() < file2->GetDownPriority())
		return true;
	return false;
}

void CDownloadQueue::SwapParts(POSITION pos1, POSITION pos2){
	CPartFile* file1 = filelist.GetAt(pos1);
	CPartFile* file2 = filelist.GetAt(pos2);
	filelist.SetAt(pos1, file2);
	filelist.SetAt(pos2, file1);
}

void CDownloadQueue::HeapSort(uint16 first, uint16 last){
	uint16 r;
	POSITION pos1 = filelist.FindIndex(first);
	for ( r = first; !(r & 0x8000) && (r<<1) < last; ){
		uint16 r2 = (r<<1)+1;
		POSITION pos2 = filelist.FindIndex(r2);
		if (r2 != last){
			POSITION pos3 = pos2;
			filelist.GetNext(pos3);
			if (!CompareParts(pos2, pos3)){
				pos2 = pos3;
				r2++;
			}
		}
		if (!CompareParts(pos1, pos2)) {
			SwapParts(pos1, pos2);
			r = r2;
			pos1 = pos2;
		}
		else
			break;
	}
}

void CDownloadQueue::SortByPriority(){
	uint16 n = filelist.GetCount();
	if (!n)
		return;
	uint16 i;
	for ( i = n/2; i--; )
		HeapSort(i, n-1);
	for ( i = n; --i; ){
		SwapParts(filelist.FindIndex(0), filelist.FindIndex(i));
		HeapSort(0, i-1);
	}
}

void CDownloadQueue::CheckDiskspaceTimed()
{
	if ((!lastcheckdiskspacetime) || (::GetTickCount() - lastcheckdiskspacetime) > DISKSPACERECHECKTIME)
		CheckDiskspace();
}

void CDownloadQueue::CheckDiskspace(bool bNotEnoughSpaceLeft)
{
	lastcheckdiskspacetime = ::GetTickCount();

	// sorting the list could be done here, but I prefer to "see" that function call in the calling functions.
	//SortByPriority();

	// If disabled, resume any previously paused files
	if (!thePrefs.IsCheckDiskspaceEnabled())
	{
		if (!bNotEnoughSpaceLeft) // avoid worse case, if we already had 'disk full'
		{
			for( POSITION pos1 = filelist.GetHeadPosition(); pos1 != NULL; )
			{
				CPartFile* cur_file = filelist.GetNext(pos1);
				switch(cur_file->GetStatus())
				{
				case PS_PAUSED:
				case PS_ERROR:
				case PS_COMPLETING:
				case PS_COMPLETE:
					continue;
				}
				cur_file->ResumeFileInsufficient();
			}
		}
		return;
	}

	// 'bNotEnoughSpaceLeft' - avoid worse case, if we already had 'disk full'
	uint64 nTotalAvailableSpace = bNotEnoughSpaceLeft ? 0 : GetFreeDiskSpaceX(thePrefs.GetTempDir());
	if (thePrefs.GetMinFreeDiskSpace() == 0)
	{
		for( POSITION pos1 = filelist.GetHeadPosition(); pos1 != NULL; )
		{
			CPartFile* cur_file = filelist.GetNext(pos1);
			switch(cur_file->GetStatus())
			{
			case PS_PAUSED:
			case PS_ERROR:
			case PS_COMPLETING:
			case PS_COMPLETE:
				continue;
			}

			// Pause the file only if it would grow in size and would exceed the currently available free space
			uint32 nSpaceToGo = cur_file->GetNeededSpace();
			if (nSpaceToGo <= nTotalAvailableSpace)
			{
				nTotalAvailableSpace -= nSpaceToGo;
				cur_file->ResumeFileInsufficient();
			}
			else
				cur_file->PauseFile(true/*bInsufficient*/);
		}
	}
	else
	{
		for( POSITION pos1 = filelist.GetHeadPosition(); pos1 != NULL; )
		{
			CPartFile* cur_file = filelist.GetNext(pos1);
			switch(cur_file->GetStatus())
			{
			case PS_PAUSED:
			case PS_ERROR:
			case PS_COMPLETING:
			case PS_COMPLETE:
				continue;
			}

			if (nTotalAvailableSpace < thePrefs.GetMinFreeDiskSpace())
			{
				if (cur_file->IsNormalFile())
				{
					// Normal files: pause the file only if it would still grow
					uint32 nSpaceToGrow = cur_file->GetNeededSpace();
					if (nSpaceToGrow)
						cur_file->PauseFile(true/*bInsufficient*/);
				}
				else
				{
					// Compressed/sparse files: always pause the file
					cur_file->PauseFile(true/*bInsufficient*/);
				}
			}
			else
			{
				// doesn't work this way. resuming the file without checking if there is a chance to successfully
				// flush any available buffered file data will pause the file right after it was resumed and disturb
				// the StopPausedFile function.
				//cur_file->ResumeFileInsufficient();
			}
		}
	}
}
// SLUGFILLER: checkDiskspace

void CDownloadQueue::GetDownloadStats(SDownloadStats& results)
{
	memset(&results, 0, sizeof results);
	for (POSITION pos = filelist.GetHeadPosition(); pos != 0; )
	{
		const CPartFile* cur_file = filelist.GetNext(pos);

		results.a[0]  += cur_file->GetSourceCount();
		results.a[1]  += cur_file->GetTransferingSrcCount();
		results.a[2]  += cur_file->GetSrcStatisticsValue(DS_ONQUEUE);
		results.a[3]  += cur_file->GetSrcStatisticsValue(DS_REMOTEQUEUEFULL);
		results.a[4]  += cur_file->GetSrcStatisticsValue(DS_NONEEDEDPARTS);
		results.a[5]  += cur_file->GetSrcStatisticsValue(DS_CONNECTED);
		results.a[6]  += cur_file->GetSrcStatisticsValue(DS_REQHASHSET);
		results.a[7]  += cur_file->GetSrcStatisticsValue(DS_CONNECTING);
		results.a[8]  += cur_file->GetSrcStatisticsValue(DS_WAITCALLBACK);
		results.a[9]  += cur_file->GetSrcStatisticsValue(DS_TOOMANYCONNS);
		results.a[10] += cur_file->GetSrcStatisticsValue(DS_LOWTOLOWIP);
		results.a[11] += cur_file->GetSrcStatisticsValue(DS_NONE);
		results.a[12] += cur_file->GetSrcStatisticsValue(DS_ERROR);
		results.a[13] += cur_file->GetSrcStatisticsValue(DS_BANNED);
		results.a[14] += cur_file->src_stats[3];
		results.a[15] += cur_file->GetSrcA4AFCount();
		results.a[16] += cur_file->src_stats[0];
		results.a[17] += cur_file->src_stats[1];
		results.a[18] += cur_file->src_stats[2];
	}
}

CUpDownClient* CDownloadQueue::GetDownloadClientByIP(uint32 dwIP){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		for (POSITION pos2 = cur_file->srclist.GetHeadPosition();pos2 != 0; ){
			CUpDownClient* cur_client = cur_file->srclist.GetNext(pos2);
			if (dwIP == cur_client->GetIP()){
				return cur_client;
			}
		}
	}
	return NULL;
}

CUpDownClient* CDownloadQueue::GetDownloadClientByIP_UDP(uint32 dwIP, uint16 nUDPPort){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		for (POSITION pos2 = cur_file->srclist.GetHeadPosition();pos2 != 0;){
			CUpDownClient* cur_client = cur_file->srclist.GetNext(pos2);
			if (dwIP == cur_client->GetIP() && nUDPPort == cur_client->GetUDPPort()){
				return cur_client;
			}
		}
	}
	return NULL;
}

bool CDownloadQueue::IsInList(const CUpDownClient* client) const
{
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		for (POSITION pos2 = cur_file->srclist.GetHeadPosition();pos2 != 0;){
			if (cur_file->srclist.GetNext(pos2) == client)
				return true;
		}
	}
	return false;
}

// khaos::categorymod+ We need to reset the resume order, too, so that these files don't
// screw up the order of 'All' category.  This function is modified.
void CDownloadQueue::ResetCatParts(int cat, uint8 useCat)
{
	int useOrder = GetMaxCatResumeOrder(useCat);
	CPartFile* cur_file;
	for (POSITION pos = filelist.GetHeadPosition(); pos != 0; ){
		cur_file = filelist.GetNext(pos);

		if (cur_file->GetCategory() == cat)
		{
			useOrder++;
			cur_file->SetCategory(useCat);
			cur_file->SetCatResumeOrder(useOrder);
		}
		else if (cur_file->GetCategory() > cat)
			cur_file->SetCategory(cur_file->GetCategory() - 1, false);
	}
}
// khaos::categorymod-

void CDownloadQueue::SetCatPrio(int cat, uint8 newprio)
{
	for (POSITION pos = filelist.GetHeadPosition(); pos != 0; ){
		CPartFile* cur_file = filelist.GetNext(pos);
		if (cat==0 || cur_file->GetCategory()==cat)
			if (newprio==PR_AUTO) {
				cur_file->SetAutoDownPriority(true);
				cur_file->SetDownPriority(PR_HIGH);
			}
			else {
				cur_file->SetAutoDownPriority(false);
				cur_file->SetDownPriority(newprio);
			}
	}
}

// ZZ:DownloadManager -->
void CDownloadQueue::RemoveAutoPrioInCat(int cat, uint8 newprio){
	CPartFile* cur_file;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		cur_file = filelist.GetAt(pos);
        if (cur_file->IsAutoDownPriority() && (cat==0 || cur_file->GetCategory()==cat)) {
			cur_file->SetAutoDownPriority(false);
			cur_file->SetDownPriority(newprio);
		}
	}
}
// <-- ZZ:DownloadManager

void CDownloadQueue::SetCatStatus(int cat, int newstatus)
{
	bool reset=false;

	POSITION pos= filelist.GetHeadPosition();
	while (pos != 0)
	{
		CPartFile* cur_file = filelist.GetAt(pos);
		if (!cur_file)
			continue;

		if (cat==-1 || 
			(cat==-2 && cur_file->GetCategory()==0) ||
			(cat==0 && cur_file->CheckShowItemInGivenCat(cat)) || 
			(cat>0 && cat==cur_file->GetCategory()))
		{
			 switch (newstatus) {
				case MP_CANCEL:
					cur_file->DeleteFile();
					reset = true;
					break;
				case MP_PAUSE:
					cur_file->PauseFile();
					break;
				case MP_STOP:
					cur_file->StopFile();
					break;
				case MP_RESUME: 
					if (cur_file->CanResumeFile()){
						if (cur_file->GetStatus() == PS_INSUFFICIENT)
							cur_file->ResumeFileInsufficient();
						else
						cur_file->ResumeFile();
					}
					break;
			}
		}
		filelist.GetNext(pos);
		if (reset)
		{
			reset = false;
			pos = filelist.GetHeadPosition();
		}
	}
}

void CDownloadQueue::MoveCat(uint8 from, uint8 to)
{
	if (from < to)
		--to;

	POSITION pos= filelist.GetHeadPosition();
	while (pos != 0)
	{
		CPartFile* cur_file = filelist.GetAt(pos);
		if (!cur_file)
			continue;

		uint8 mycat = cur_file->GetCategory();
		if ((mycat>=min(from,to) && mycat<=max(from,to)))
		{
			//if ((from<to && (mycat<from || mycat>to)) || (from>to && (mycat>from || mycat<to)) )  continue; //not affected

			if (mycat == from)
				cur_file->SetCategory(to);
			else {
				if (from < to)
					cur_file->SetCategory(mycat - 1);
				else
					cur_file->SetCategory(mycat + 1);
			}
		}
		filelist.GetNext(pos);
	}
}

UINT CDownloadQueue::GetDownloadingFileCount() const
{
	UINT result = 0;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		UINT uStatus = filelist.GetNext(pos)->GetStatus();
		if (uStatus == PS_READY || uStatus == PS_EMPTY)
			result++;
	}
	return result;
}

uint16 CDownloadQueue::GetPausedFileCount(){
	uint16 result = 0;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		if (cur_file->GetStatus() == PS_PAUSED)
			result++;
	}
	return result;
}

void CDownloadQueue::DisableAllA4AFAuto(void)
{
	for (POSITION pos = filelist.GetHeadPosition(); pos != NULL; )
		filelist.GetNext(pos)->SetA4AFAuto(false);
}

//MORPH START - Removed by SiRoB, Due to Khaos Categorie
/*
// HoaX_69: BEGIN AutoCat function
void CDownloadQueue::SetAutoCat(CPartFile* newfile){
	if(thePrefs.GetCatCount()>1){
		for (int ix=1;ix<thePrefs.GetCatCount();ix++){	
			int curPos = 0;
			CString catExt = thePrefs.GetCategory(ix)->autocat;
			catExt.MakeLower();

			// No need to compare agains an empty AutoCat array
			if( catExt.IsEmpty())
				continue;

			CString fullname = newfile->GetFileName();
			fullname.MakeLower();
			CString cmpExt = catExt.Tokenize(_T("|"), curPos);

			while (!cmpExt.IsEmpty()) {
				// HoaX_69: Allow wildcards in autocat string
				//  thanks to: bluecow, khaos and SlugFiller
				if(cmpExt.Find(_T("*")) != -1 || cmpExt.Find(_T("?")) != -1){
					// Use wildcards
					if(PathMatchSpec(fullname, cmpExt)){
						newfile->SetCategory(ix);
						return;
					}
				}else{
					if(fullname.Find(cmpExt) != -1){
						newfile->SetCategory(ix);
						return;
					}
				}
				cmpExt = catExt.Tokenize(_T("|"),curPos);
			}
		}
	}
}
// HoaX_69: END
*/
//MORPH END  - Removed by SiRoB, Due to Khaos Categorie

void CDownloadQueue::ResetLocalServerRequests()
{
	m_dwNextTCPSrcReq = 0;
	m_localServerReqQueue.RemoveAll();

	POSITION pos = filelist.GetHeadPosition();
	while (pos != NULL)
	{ 
		CPartFile* pFile = filelist.GetNext(pos);
		UINT uState = pFile->GetStatus();
		if (uState == PS_READY || uState == PS_EMPTY)
			pFile->ResumeFile();
		pFile->m_bLocalSrcReqQueued = false;
	}
}

void CDownloadQueue::RemoveLocalServerRequest(CPartFile* pFile)
{
	POSITION pos1, pos2;
	for( pos1 = m_localServerReqQueue.GetHeadPosition(); ( pos2 = pos1 ) != NULL; )
	{
		m_localServerReqQueue.GetNext(pos1);
		if (m_localServerReqQueue.GetAt(pos2) == pFile)
		{
			m_localServerReqQueue.RemoveAt(pos2);
			pFile->m_bLocalSrcReqQueued = false;
			// could 'break' here.. fail safe: go through entire list..
		}
	}
}

void CDownloadQueue::ProcessLocalRequests()
{
	if ( (!m_localServerReqQueue.IsEmpty()) && (m_dwNextTCPSrcReq < ::GetTickCount()) )
	{
		CSafeMemFile dataTcpFrame(22);
		const int iMaxFilesPerTcpFrame = 15;
		int iFiles = 0;
		while (!m_localServerReqQueue.IsEmpty() && iFiles < iMaxFilesPerTcpFrame)
		{
			// find the file with the longest waitingtime
			POSITION pos1, pos2;
			uint32 dwBestWaitTime = 0xFFFFFFFF;
			POSITION posNextRequest = NULL;
			CPartFile* cur_file;
			for( pos1 = m_localServerReqQueue.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
				m_localServerReqQueue.GetNext(pos1);
				cur_file = m_localServerReqQueue.GetAt(pos2);
				if (cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY)
				{
					uint8 nPriority = cur_file->GetDownPriority();
					if (nPriority > PR_HIGH){
						ASSERT(0);
						nPriority = PR_HIGH;
					}

					if (cur_file->lastsearchtime + (PR_HIGH-nPriority) < dwBestWaitTime ){
						dwBestWaitTime = cur_file->lastsearchtime + (PR_HIGH-nPriority);
						posNextRequest = pos2;
					}
				}
				else{
					m_localServerReqQueue.RemoveAt(pos2);
					cur_file->m_bLocalSrcReqQueued = false;
					if (thePrefs.GetDebugSourceExchange())
						AddDebugLogLine(false, _T("Local server source request for file \"%s\" not sent because of status '%s'"), cur_file->GetFileName(), cur_file->getPartfileStatus());
				}
			}
			
			if (posNextRequest != NULL)
			{
				cur_file = m_localServerReqQueue.GetAt(posNextRequest);
				cur_file->m_bLocalSrcReqQueued = false;
				cur_file->lastsearchtime = ::GetTickCount();
				m_localServerReqQueue.RemoveAt(posNextRequest);
				iFiles++;
				
				// create request packet
				Packet* packet = new Packet(OP_GETSOURCES,16);
				md4cpy(packet->pBuffer,cur_file->GetFileHash());
				if (thePrefs.GetDebugServerTCPLevel() > 0)
					Debug(_T(">>> Sending OP__GetSources(%2u/%2u); %s\n"), iFiles, iMaxFilesPerTcpFrame, DbgGetFileInfo(cur_file->GetFileHash()));
				dataTcpFrame.Write(packet->GetPacket(), packet->GetRealPacketSize());
				delete packet;

				if ( thePrefs.GetDebugSourceExchange() )
					AddDebugLogLine(false, _T("Send:Source Request Server File(%s)"), cur_file->GetFileName());
			}
		}

		int iSize = dataTcpFrame.GetLength();
		if (iSize > 0)
		{
			// create one 'packet' which contains all buffered OP_GETSOURCES eD2K packets to be sent with one TCP frame
			// server credits: 16*iMaxFilesPerTcpFrame+1 = 241
			Packet* packet = new Packet(new char[iSize], dataTcpFrame.GetLength(), true, false);
			dataTcpFrame.Seek(0, CFile::begin);
			dataTcpFrame.Read(packet->GetPacket(), iSize);
			theStats.AddUpDataOverheadServer(packet->size);
			theApp.serverconnect->SendPacket(packet, true);
		}

		// next TCP frame with up to 15 source requests is allowed to be sent in..
		m_dwNextTCPSrcReq = ::GetTickCount() + SEC2MS(iMaxFilesPerTcpFrame*(16+4));
	}
}

void CDownloadQueue::SendLocalSrcRequest(CPartFile* sender){
	ASSERT ( !m_localServerReqQueue.Find(sender) );
	m_localServerReqQueue.AddTail(sender);
}

void CDownloadQueue::GetDownloadStats(int results[],
									  uint64& rui64TotFileSize,
									  uint64& rui64TotBytesLeftToTransfer,
									  uint64& rui64TotNeededSpace)
{
	results[0]=0;
	results[1]=0;
	results[2]=0;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0; )
	{
		const CPartFile* cur_file = filelist.GetNext(pos);
		UINT uState = cur_file->GetStatus();
		if (uState == PS_READY || uState == PS_EMPTY)
		{
			uint32 ui32SizeToTransfer=0;
			uint32 ui32NeededSpace=0;
			cur_file->GetSizeToTransferAndNeededSpace (ui32SizeToTransfer,ui32NeededSpace);
			rui64TotFileSize += cur_file->GetFileSize(); 
			rui64TotBytesLeftToTransfer += ui32SizeToTransfer;
			rui64TotNeededSpace += ui32NeededSpace;
			results[2]++; 
		} 
		results[0]+=cur_file->GetSourceCount();
		results[1]+=cur_file->GetTransferingSrcCount();
	}
}

///////////////////////////////////////////////////////////////////////////////
// CSourceHostnameResolveWnd

BEGIN_MESSAGE_MAP(CSourceHostnameResolveWnd, CWnd)
	ON_MESSAGE(WM_HOSTNAMERESOLVED, OnHostnameResolved)
END_MESSAGE_MAP()

CSourceHostnameResolveWnd::CSourceHostnameResolveWnd()
{
}

CSourceHostnameResolveWnd::~CSourceHostnameResolveWnd()
{
	while (!m_toresolve.IsEmpty())
		delete m_toresolve.RemoveHead();
}

void CSourceHostnameResolveWnd::AddToResolve(const uchar* fileid, LPCSTR pszHostname, uint16 port, LPCTSTR pszURL)
{
	bool bResolving = !m_toresolve.IsEmpty();

	// double checking
	if (!theApp.downloadqueue->GetFileByID(fileid))
		return;

	Hostname_Entry* entry = new Hostname_Entry;
	md4cpy(entry->fileid, fileid);
	entry->strHostname = pszHostname;
	entry->port = port;
	entry->strURL = pszURL;
	m_toresolve.AddTail(entry);

	if (bResolving)
		return;

	memset(m_aucHostnameBuffer, 0, sizeof(m_aucHostnameBuffer));
	if (WSAAsyncGetHostByName(m_hWnd, WM_HOSTNAMERESOLVED, entry->strHostname, m_aucHostnameBuffer, sizeof m_aucHostnameBuffer) != 0)
		return;
	m_toresolve.RemoveHead();
	delete entry;
}

LRESULT CSourceHostnameResolveWnd::OnHostnameResolved(WPARAM wParam,LPARAM lParam)
{
	Hostname_Entry* resolved = m_toresolve.RemoveHead();
	if (WSAGETASYNCERROR(lParam) == 0)
	{
		int iBufLen = WSAGETASYNCBUFLEN(lParam);
		if (iBufLen >= sizeof(HOSTENT))
		{
			LPHOSTENT pHost = (LPHOSTENT)m_aucHostnameBuffer;
			if (pHost->h_length == 4 && pHost->h_addr_list && pHost->h_addr_list[0])
			{
				uint32 nIP = ((LPIN_ADDR)(pHost->h_addr_list[0]))->s_addr;

				CPartFile* file = theApp.downloadqueue->GetFileByID(resolved->fileid);
				if (file)
				{
					if (resolved->strURL.IsEmpty())
					{
						CSafeMemFile sources(1+4+2);
						sources.WriteUInt8(1);
						sources.WriteUInt32(nIP);
						sources.WriteUInt16(resolved->port);
						sources.SeekToBegin();
						file->AddSources(&sources,0,0);
					}
					else
					{
						file->AddSource(resolved->strURL, nIP);
					}
				}
			}
		}
	}
	delete resolved;

	while (!m_toresolve.IsEmpty())
	{
		Hostname_Entry* entry = m_toresolve.GetHead();
		memset(m_aucHostnameBuffer, 0, sizeof(m_aucHostnameBuffer));
		if (WSAAsyncGetHostByName(m_hWnd, WM_HOSTNAMERESOLVED, entry->strHostname, m_aucHostnameBuffer, sizeof m_aucHostnameBuffer) != 0)
			return TRUE;
		m_toresolve.RemoveHead();
		delete entry;
	}
	return TRUE;
}
// SLUGFILLER: hostnameSources

bool CDownloadQueue::DoKademliaFileRequest()
{
	return ((::GetTickCount() - lastkademliafilerequest) > KADEMLIAASKTIME);
}

void CDownloadQueue::KademliaSearchFile(uint32 searchID, const Kademlia::CUInt128* pcontactID, uint8 type, uint32 ip, uint16 tcp, uint16 udp, uint32 serverip, uint16 serverport, uint32 clientid)
{
	//Safty measure to make sure we are looking for these sources
	CPartFile* temp = GetFileByKadFileSearchID(searchID);
	if( !temp )
		return;
	//Do we need more sources?
	if(!(!temp->IsStopped() && thePrefs.GetMaxSourcePerFile() > temp->GetSourceCount()))
		return;

	uint32 ED2Kip = ntohl(ip);
	if (theApp.ipfilter->IsFiltered(ED2Kip))
	{
		if (thePrefs.GetLogFilteredIPs())
			AddDebugLogLine(false, _T("IPfiltered source IP=%s (%s) received from Kademlia"), ipstr(ED2Kip), theApp.ipfilter->GetLastHit());
		return;
	}
	if( (ip == Kademlia::CKademlia::getIPAddress() || ED2Kip == theApp.serverconnect->GetClientID()) && tcp == thePrefs.GetPort())
		return;
	CUpDownClient* ctemp = NULL; 
	switch( type )
	{
		case 1:
		{
			//NonFirewalled users
			if(!tcp)
			{
				if (thePrefs.GetVerbose())
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received from Kademlia, no tcp port received"), ipstr(ip));
				return;
			}
			if (!IsGoodIP(ED2Kip))
			{
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received from Kademlia"), ipstr(ED2Kip));
				return;
			}
			ctemp = new CUpDownClient(temp,tcp,ip,0,0,false);
			ctemp->SetSourceFrom(SF_KADEMLIA);
			ctemp->SetServerIP(serverip);
			ctemp->SetServerPort(serverport);
			ctemp->SetKadPort(udp);
			break;
		}
/*		case 2:
		{
			//Don't use this type... Some clients will process it wrong..
			break;
		}
		case 3:
		{
			//This will be a firewaled client connected to Kad only.
			break;
		}
*/	}

	if (ctemp)
		CheckAndAddSource(temp, ctemp);
}

void CDownloadQueue::ExportPartMetFilesOverview() const
{
	CString strFileListPath = thePrefs.GetAppDir() + _T("downloads.txt");
	
	CString strTmpFileListPath = strFileListPath;
	PathRenameExtension(strTmpFileListPath.GetBuffer(MAX_PATH), _T(".tmp"));
	strTmpFileListPath.ReleaseBuffer();

	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(strTmpFileListPath, CFile::modeCreate | CFile::modeWrite | CFile::typeText | CFile::shareDenyWrite, &fexp))
	{
		CString strError;
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("Failed to create part.met file list%s"), strError);
		return;
	}

	try
	{
		file.printf(_T("Date:      %s\n"), CTime::GetCurrentTime().Format(_T("%c")));
		file.printf(_T("Directory: %s\n"), thePrefs.GetTempDir());
		file.printf(_T("\n"));
		file.printf(_T("Part file\teD2K link\n"));
		file.printf(_T("--------------------------------------------------------------------------------\n"));
		for (POSITION pos = filelist.GetHeadPosition(); pos != 0; )
		{
			const CPartFile* pPartFile = filelist.GetNext(pos);
			if (pPartFile->GetStatus(true) != PS_COMPLETE)
			{
				CString strPartFilePath(pPartFile->GetFilePath());
				TCHAR szNam[_MAX_FNAME];
				TCHAR szExt[_MAX_EXT];
				_tsplitpath(strPartFilePath, NULL, NULL, szNam, szExt);
				file.printf(_T("%s%s\t%s\n"), szNam, szExt, CreateED2kLink(pPartFile));
			}
		}

		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			file.Flush(); // flush file stream buffers to disk buffers
			if (_commit(_fileno(file.m_pStream)) != 0) // commit disk buffers to disk
				AfxThrowFileException(CFileException::hardIO, GetLastError(), file.GetFileName());
		}
		file.Close();

		CString strBakFileListPath = strFileListPath;
		PathRenameExtension(strBakFileListPath.GetBuffer(MAX_PATH), _T(".bak"));
		strBakFileListPath.ReleaseBuffer();

		if (_taccess(strBakFileListPath, 0) == 0)
			CFile::Remove(strBakFileListPath);
		if (_taccess(strFileListPath, 0) == 0)
			CFile::Rename(strFileListPath, strBakFileListPath);
		CFile::Rename(strTmpFileListPath, strFileListPath);
	}
	catch(CFileException* e)
	{
		CString strError;
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (e->GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("Failed to write part.met file list%s"), strError);
		e->Delete();
		file.Abort();
		(void)_tremove(file.GetFilePath());
	}
}
void CDownloadQueue::OnConnectionState(bool bConnected)
{
	for (POSITION pos = filelist.GetHeadPosition(); pos != 0; )
	{
		CPartFile* pPartFile = filelist.GetNext(pos);
		if (pPartFile->IsPartFile())
			pPartFile->SetActive(bConnected);
	}
}

//MORPH START - Added by SiRoB, ZZ Ratio
bool CDownloadQueue::IsFilesPowershared()
{
	if (filelist.IsEmpty()) {
		for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
			CPartFile* cur_file =  filelist.GetNext(pos);
			if (cur_file->IsPartFile() && ((cur_file->GetPowerSharedMode()>=0)?cur_file->GetPowerSharedMode():thePrefs.GetPowerShareMode())&1)
				return true;
		}
	}
	return false;
}
//MORPH END   - Added by SiRoB, ZZ Ratio