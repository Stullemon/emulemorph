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
#include "DownloadQueue.h"
#include "UpDownClient.h"
#include "PartFile.h"
#include "ed2kLink.h"
#include "SearchList.h"
#include "ClientList.h"
#include "UploadQueue.h"
#include "SharedFileList.h"
#include "OtherFunctions.h"
#include "SafeFile.h"
#include "Sockets.h"
#include "ServerList.h"
#include "Server.h"
#include "Packets.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#include "TransferWnd.h"
#include "TaskbarNotifier.h"
#include "MenuCmds.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CDownloadQueue::CDownloadQueue(CPreferences* in_prefs,CSharedFileList* in_sharedfilelist){
	app_prefs = in_prefs;
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
	m_nDownDataRateMSOverhead = 0;
	m_nDownDatarateOverhead = 0;
	m_nDownDataOverheadSourceExchange = 0;
	m_nDownDataOverheadFileRequest = 0;
	m_nDownDataOverheadOther = 0;
	m_nDownDataOverheadServer = 0;
	m_nDownDataOverheadKad = 0;
	m_nDownDataOverheadSourceExchangePackets = 0;
	m_nDownDataOverheadFileRequestPackets = 0;
	m_nDownDataOverheadOtherPackets = 0;
	m_nDownDataOverheadServerPackets = 0;
	m_nDownDataOverheadKadPackets = 0;
	sumavgDDRO = 0;
	//m_lastRefreshedDLDisplay = 0;
	m_dwNextTCPSrcReq = 0;
	m_cRequestsSentToServer = 0;
	// khaos::categorymod+
	m_iLastLinkQueuedTick = 0;
	m_bBusyPurgingLinks = false;
	m_ED2KLinkQueue.RemoveAll();
	// khaos::categorymod-
	// khaos::kmod+ A4AF
	forcea4af_file = NULL;
	// khaos::kmod-
}

//void CDownloadQueue::UpdateDisplayedInfo(bool force) {
//    DWORD curTick = ::GetTickCount();
//
//    if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+(uint32)(rand()/(RAND_MAX/1000))) {
//        theApp.emuledlg->transferwnd->downloadlistctrl.UpdateItem(this);
//        m_lastRefreshedDLDisplay = curTick;
//    }
//}

void CDownloadQueue::AddPartFilesToShare(){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		if (cur_file->GetStatus(true) == PS_READY)
			sharedfilelist->SafeAddKFile(cur_file,true);
	}
}

// SLUGFILLER: mergeKnown
void CDownloadQueue::SavePartFilesToKnown(CFile* file){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		CKnownFile* cur_file = (CKnownFile*)filelist.GetAt(pos);	// Write as known files
			cur_file->WriteToFile(file);
		}
}

uint32 CDownloadQueue::GetPartFilesCount(){
	return filelist.GetCount();
}
// SLUGFILLER: mergeKnown

void CDownloadQueue::CompDownDatarateOverhead(){
	// Patch By BadWolf - Accurate datarate Calculation
	TransferredData newitem = {m_nDownDataRateMSOverhead,::GetTickCount()};
	m_AvarageDDRO_list.AddTail(newitem);
	sumavgDDRO += m_nDownDataRateMSOverhead;

	while ((float)(m_AvarageDDRO_list.GetTail().timestamp - m_AvarageDDRO_list.GetHead().timestamp) > MAXAVERAGETIME) 
		sumavgDDRO -= m_AvarageDDRO_list.RemoveHead().datalen;

	m_nDownDataRateMSOverhead = 0;

	if(m_AvarageDDRO_list.GetCount() > 10){
		DWORD dwDuration = m_AvarageDDRO_list.GetTail().timestamp - m_AvarageDDRO_list.GetHead().timestamp;
		if (dwDuration)
			m_nDownDatarateOverhead = 1000 * sumavgDDRO / dwDuration;
	}
	else
		m_nDownDatarateOverhead = 0;

	// END Patch By BadWolf
}

void CDownloadQueue::Init(){
	// find all part files, read & hash them if needed and store into a list
	CFileFind ff;
	int count = 0;

	CString searchPath(app_prefs->GetTempDir());
	searchPath += "\\*.part.met";

	//check all part.met files
	bool end = !ff.FindFile(searchPath, 0);
	while (!end){
		end = !ff.FindNextFile();
		if (ff.IsDirectory())
			continue;
		CPartFile* toadd = new CPartFile();
		if (toadd->LoadPartFile(app_prefs->GetTempDir(),ff.GetFileName().GetBuffer())){
			count++;
			filelist.AddTail(toadd);			// to downloadqueue
			// SLUGFILLER: SafeHash remove - part files are shared later
			theApp.emuledlg->transferwnd->downloadlistctrl.AddFile(toadd);// show in downloadwindow
		}
		else
			delete toadd;
	}
	ff.Close();
	UpdatePNRFile(); //<<-- enkeyDEV(ColdShine) -PartfileNameRecovery-

	//try recovering any part.met files
	searchPath += ".backup";
	end = !ff.FindFile(searchPath, 0);
	while (!end){
		end = !ff.FindNextFile();
		if (ff.IsDirectory())
			continue;
		CPartFile* toadd = new CPartFile();
		if (toadd->LoadPartFile(app_prefs->GetTempDir(),ff.GetFileName().GetBuffer())){
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
	UpdatePNRFile(); //<<-- enkeyDEV(ColdShine) -PartfileNameRecovery-

	if(count == 0) {
		AddLogLine(false,GetResString(IDS_NOPARTSFOUND));
	} else {
		AddLogLine(false,GetResString(IDS_FOUNDPARTS),count);
		SortByPriority();
		CheckDiskspace();	// SLUGFILLER: checkDiskspace
	}
	VERIFY( m_srcwnd.CreateEx(0, AfxRegisterWndClass(0),_T("Hostname Resolve Wnd"),WS_OVERLAPPED, 0, 0, 0, 0, NULL, NULL));		// SLUGFILLER: hostnameSources
}

CDownloadQueue::~CDownloadQueue(){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;)
		delete filelist.GetNext(pos);
	m_srcwnd.DestroyWindow(); // just to avoid a MFC warning
}

// [InterCeptor]
void CDownloadQueue::SavePartFiles(bool del /*= false*/) {
	for (POSITION pos =filelist.GetHeadPosition();pos != 0;){	
		CPartFile* cur_file = filelist.GetNext(pos);
		cur_file->m_hpartfile.Flush();
		cur_file->SavePartFile();
		if (del)
		    delete cur_file;
	}
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
		paused = (uint8)theApp.glob_prefs->AddNewFilesPaused();
	AddDownload(newfile, (paused==1));

	// If the search result is from OP_GLOBSEARCHRES there may also be a source
	if (toadd->GetClientID() && toadd->GetClientPort()){
		CSafeMemFile sources(1+4+2);
		try{
		    uint8 uSources = 1;
		    sources.Write(&uSources, 1);
		    uint32 uIP = toadd->GetClientID();
		    sources.Write(&uIP, 4);
		    uint16 uPort = toadd->GetClientPort();
		    sources.Write(&uPort, 2);
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
		    uint8 uSources = 1;
		    sources.Write(&uSources, 1);
			sources.Write(&aClients[i].m_nIP, 4);
			sources.Write(&aClients[i].m_nPort, 2);
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
		paused = (uint8)theApp.glob_prefs->AddNewFilesPaused();
	AddDownload(newfile, (paused==1));
}

// New parameter: int Category = -1 By Default
// This entire function was rewritten.
bool CDownloadQueue::StartNextFile(int cat){
	
	CPartFile*  pfile = NULL;
	

	// Standard operation of StartNextFile is supported by passing -1 for Category...
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		if (cur_file->GetStatus() == PS_PAUSED) {
			if (!pfile)
				pfile = cur_file;
			else {
				if (pfile->GetCategory()!=cat && cur_file->GetCategory()==cat  && cat!=-1) {
					if (theApp.glob_prefs->GetResumeSameCat()){
						pfile = cur_file;
						if (pfile->GetDownPriority() == PR_HIGH && pfile->GetCatResumeOrder() == 0) break; continue;
					}
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
	if (pfile) {
		pfile->ResumeFile();
		return true;
	}

	return false;
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
void CDownloadQueue::AddFileLinkToDownload(CED2KFileLink* pLink, bool AllocatedLink, bool SkipQueue)
{
	if (theApp.glob_prefs->SelectCatForNewDL() && !SkipQueue)
	{
		m_ED2KLinkQueue.AddTail(pLink);
		m_iLastLinkQueuedTick = GetTickCount();
		return;
	}

	int useCat = pLink->GetCat();

	if (theApp.glob_prefs->UseAutoCat() && useCat == -1)
		useCat = theApp.downloadqueue->GetAutoCat(CString(pLink->GetName()), (ULONG)pLink->GetSize());
	else if (theApp.glob_prefs->UseActiveCatForLinks() && useCat == -1)
		useCat = theApp.emuledlg->transferwnd->GetActiveCategory();
	else if (useCat == -1)
		useCat = 0;

	// Just in case...
	if (m_ED2KLinkQueue.GetCount() && !theApp.glob_prefs->SelectCatForNewDL()) PurgeED2KLinkQueue();
	m_iLastLinkQueuedTick = 0;
	// khaos::categorymod-

	CPartFile* pNewFile = new CPartFile(pLink);

	if (pNewFile->GetStatus() == PS_ERROR)
	{
		delete pNewFile;
		pNewFile=NULL;
	}
	else
	{
		// khaos::categorymod+ Pass useCat instead of cat and autoset resume order.
		pNewFile->SetCategory(useCat);
		//MORPH START - Added by IceCream, Increase the size of the Push Small Files feature from 50KB to 150KB
		//if (theApp.glob_prefs->SmallFileDLPush() && pNewFile->GetFileSize() < 51200)
		if (theApp.glob_prefs->SmallFileDLPush() && pNewFile->GetFileSize() < 154624)
			pNewFile->SetCatResumeOrder(0);
		//MORPH END   - Added by IceCream, Increase the size of the Push Small Files feature from 50KB to 150KB
		//MORPH START - Changed by SiRoB, Fix to correct incremental resume order
		//else if (theApp.glob_prefs->AutoSetResumeOrder()) pNewFile->SetCatResumeOrder(GetMaxCatResumeOrder(useCat));
		else if (theApp.glob_prefs->AutoSetResumeOrder()) pNewFile->SetCatResumeOrder(GetMaxCatResumeOrder(useCat)+1);
		//MORPH END   - Changed by SiRoB, Fix to correct incremental resume order
		// khaos::categorymod-
		AddDownload(pNewFile,theApp.glob_prefs->AddNewFilesPaused());
	}

	if(pLink->HasValidSources())
	{
		if (pNewFile)
			pNewFile->AddClientSources(pLink->SourcesList,1);
		else
		{
			CPartFile* pPartFile = GetFileByID((uchar*)pLink->GetHashKey());
			if (pPartFile) pPartFile->AddClientSources(pLink->SourcesList,1);
		}
	}
	// SLUGFILLER: hostnameSources
	if(pLink->HasHostnameSources()) {
		for (POSITION pos = pLink->m_HostnameSourcesList.GetHeadPosition(); pos != NULL; pLink->m_HostnameSourcesList.GetNext(pos))
			m_srcwnd.AddToResolve((uchar*)pLink->GetHashKey(), pLink->m_HostnameSourcesList.GetAt(pos)->strHostname, pLink->m_HostnameSourcesList.GetAt(pos)->nPort);
	}
	// SLUGFILLER: hostnameSources

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

	if (theApp.glob_prefs->SelectCatForNewDL())
	{
		CSelCategoryDlg* getCatDlg = new CSelCategoryDlg((CWnd*)theApp.emuledlg);
		getCatDlg->DoModal();
		
		// Returns 0 on 'Cancel', otherwise it returns the selected category
		// or the index of a newly created category.  Users can opt to add the
		// links into a new category.
		useCat = getCatDlg->GetInput();
		bCreatedNewCat = getCatDlg->CreatedNewCat();
		delete getCatDlg;
	}
	else if (theApp.glob_prefs->UseActiveCatForLinks())
		useCat = theApp.emuledlg->transferwnd->GetActiveCategory();
	else
		useCat = 0;

	int useOrder = GetMaxCatResumeOrder(useCat);

	for (POSITION pos = m_ED2KLinkQueue.GetHeadPosition(); pos != 0; m_ED2KLinkQueue.GetNext(pos))
	{
		CED2KFileLink*	pLink = m_ED2KLinkQueue.GetAt(pos);
		CPartFile*		pNewFile =	new CPartFile(pLink);
		
		if (pNewFile->GetStatus() == PS_ERROR)
		{
			delete pNewFile;
			pNewFile = NULL;
		}
		else
		{
			if (!theApp.glob_prefs->SelectCatForNewDL() && theApp.glob_prefs->UseAutoCat())
			{
				useCat = GetAutoCat(CString(pNewFile->GetFileName()), (ULONG)pNewFile->GetFileSize());
				if (!useCat && theApp.glob_prefs->UseActiveCatForLinks())
					useCat = theApp.emuledlg->transferwnd->GetActiveCategory();
			}
			pNewFile->SetCategory(useCat);
			//MORPH START - Added by IceCream, Increase the size of the Push Small Files feature from 50KB to 150KB
			//if (theApp.glob_prefs->SmallFileDLPush() && pNewFile->GetFileSize() < 51200)
			if (theApp.glob_prefs->SmallFileDLPush() && pNewFile->GetFileSize() < 154624)
				pNewFile->SetCatResumeOrder(0);
			//MORPH END   - Added by IceCream, Increase the size of the Push Small Files feature from 50KB to 150KB
			else if (theApp.glob_prefs->AutoSetResumeOrder()) {
				useOrder++;
				pNewFile->SetCatResumeOrder(useOrder);
			}
			AddDownload(pNewFile,theApp.glob_prefs->AddNewFilesPaused());
			addedFiles++;
		}

		if(pLink->HasValidSources())
		{
			if (pNewFile)
				pNewFile->AddClientSources(pLink->SourcesList,1);
			else
			{
				CPartFile* pPartFile = GetFileByID((uchar*)pLink->GetHashKey());
				if (pPartFile) pPartFile->AddClientSources(pLink->SourcesList, 1);
			}
		}
		// SLUGFILLER: hostnameSources
		if(pLink->HasHostnameSources()) {
			for (POSITION pos = pLink->m_HostnameSourcesList.GetHeadPosition(); pos != NULL; pLink->m_HostnameSourcesList.GetNext(pos))
				m_srcwnd.AddToResolve((uchar*)pLink->GetHashKey(), pLink->m_HostnameSourcesList.GetAt(pos)->strHostname, pLink->m_HostnameSourcesList.GetAt(pos)->nPort);
		}
		// SLUGFILLER: hostnameSources
		// We're done with this link.
		delete pLink;
		pLink = NULL;
	}
	
	m_ED2KLinkQueue.RemoveAll();

	// This bit of code will resume the number of files that the user specifies in preferences (Off by default)
	if (theApp.glob_prefs->StartDLInEmptyCats() > 0 && bCreatedNewCat && theApp.glob_prefs->AddNewFilesPaused())
		for (int i = 0; i < theApp.glob_prefs->StartDLInEmptyCats(); i++)
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

	if (theApp.glob_prefs->GetCatCount() <= 1)
		return 0;

	for (int ix = 1; ix < theApp.glob_prefs->GetCatCount(); ix++)
	{ 
		Category_Struct* curCat = theApp.glob_prefs->GetCategory(ix);
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
	CString sFilterMask = theApp.glob_prefs->GetCategory(nCat)->viewfilters.sAdvancedFilterMask;
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
				theApp.emuledlg->AddDebugLogLine(false, "Category '%s' has invalid Category Mask String.", theApp.glob_prefs->GetCategory(nCat)->title);
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
		newfile->StopFile();
	// khaos::accuratetimerem+
	else
		newfile->SetActivatedTick();
	// khaos::accuratetimerem-
	//MORPH - Removed by SiRoB, Due to Khaos Categorie
	//SetAutoCat(newfile);// HoaX_69 / Slugfiller: AutoCat
	filelist.AddTail(newfile);
	UpdatePNRFile(); //<<-- enkeyDEV(ColdShine) -PartfileNameRecovery-
	SortByPriority();
	CheckDiskspace();	// SLUGFILLER: checkDiskspace
	theApp.emuledlg->transferwnd->downloadlistctrl.AddFile(newfile);
	AddLogLine(true,GetResString(IDS_NEWDOWNLOAD),newfile->GetFileName());
	CString msgTemp;
	msgTemp.Format(GetResString(IDS_NEWDOWNLOAD)+"\n",newfile->GetFileName());
	theApp.emuledlg->ShowNotifier(msgTemp, TBN_DLOADADDED);
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
    uint32 maxDownload = app_prefs->GetMaxDownload();

    if(theApp.uploadqueue->GetUploadQueueLength() <= 0) {
        // copied from prefs load
        if( app_prefs->GetMaxUpload() != 0 ){
            if(app_prefs->GetMaxUpload() < 4 && (app_prefs->GetMaxUpload()*3 < app_prefs->GetMaxDownload())) {
				maxDownload = app_prefs->GetMaxUpload()*3;
            }
    
            if(app_prefs->GetMaxUpload() < 10 && (app_prefs->GetMaxUpload()*4 < app_prefs->GetMaxDownload())) {
				maxDownload = app_prefs->GetMaxUpload()*4;
            }
		}
    }

	if (maxDownload != UNLIMITED && datarate > 1500){
		downspeed = (maxDownload*1024*100)/(datarate+1); //(uint16)((float)((float)(app_prefs->GetMaxDownload()*1024)/(datarate+1)) * 100);
		if (downspeed < 50)
			downspeed = 50;
		else if (downspeed > 200)
			downspeed = 200;
	}
	// ZZ:UploadSpeedSense <--

    uint32 friendDownspeed = downspeed;

    if(theApp.uploadqueue->GetUploadQueueLength() > 0 && app_prefs->IsZZRatioDoesWork()) {//MORPH - Changed by SiRoB, :( Added By Yun.SF3, Option for Ratio Systems
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
                //theApp.emuledlg->AddLogLine(true, "Limiting downspeed");
		   }
		}

        // has this client downloaded more than it has uploaded this session? (friends included)
        // then limit its download speed from all friends
        // limit will be removed as soon as upload has catched up to download
        if(theApp.stat_sessionReceivedBytes/3 > (theApp.stat_sessionSentBytes) &&
           datarate > 1500) {

            float secondsNeededToEvenOut = (theApp.stat_sessionReceivedBytes/3-theApp.stat_sessionSentBytes)/(theApp.uploadqueue->GetDatarate()+1);
			uint32 tempDownspeed = max(min(3*100/max(secondsNeededToEvenOut, 1), 200), 30);

			if(friendDownspeed == 0 || tempDownspeed < friendDownspeed) {
                friendDownspeed = tempDownspeed;
            }
        }
	}

	uint32 datarateX=0;
	udcounter++;

	//filelist is already sorted by prio, therefore I removed all the extra loops..
	for (POSITION pos =filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		CPartFile* cur_file =  filelist.GetAt(pos);
		if ((cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY)/* && cur_file->GetDownPriority() == PR_HIGH*/){
			datarateX += cur_file->Process(downspeed, udcounter, friendDownspeed);
		}
		else{
			//This will make sure we don't keep old sources to paused and stoped files..
			cur_file->StopPausedFile();
		}
	}

/*	for (POSITION pos =filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		CPartFile* cur_file =  filelist.GetAt(pos);
		if ((cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY) && cur_file->GetDownPriority() == PR_NORMAL){
			datarateX += cur_file->Process(downspeed,udcounter);
		}
	}
	for (POSITION pos =filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		CPartFile* cur_file =  filelist.GetAt(pos);
		if ((cur_file->GetStatus() == PS_READY || cur_file->GetStatus() == PS_EMPTY) && (cur_file->GetDownPriority() == PR_LOW)){
			datarateX += cur_file->Process(downspeed,udcounter);
		}
	}
*/
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
	else if (m_ED2KLinkQueue.GetCount() && !theApp.glob_prefs->SelectCatForNewDL()) // This should not happen.
	{
		PurgeED2KLinkQueue();
		theApp.emuledlg->AddDebugLogLine(false, "ERROR: Links in ED2K Link Queue while SelectCatForNewDL was disabled!");
	}
	// khaos::categorymod-

	CheckDiskspaceTimed();
}
 //MORPH - Added by Yun.SF3, ZZ Upload System

CPartFile*	CDownloadQueue::GetFileByIndex(int index){
	POSITION pos = filelist.FindIndex(index);
	if (pos)
		return filelist.GetAt(pos);
	else
		return NULL;
}

CPartFile*	CDownloadQueue::GetFileByID(const uchar* filehash){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		if (!md4cmp(filehash,cur_file->GetFileHash()))
			return cur_file;
	}
	return 0;
}

CPartFile*	CDownloadQueue::GetFileByKadFileSearchID(uint32 id){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		if (id == cur_file->GetKadFileSearchID())
			return cur_file;
	}
	return 0;
}

bool CDownloadQueue::IsPartFile(void* totest){
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		if (totest == filelist.GetNext(pos))
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

void CDownloadQueue::CheckAndAddSource(CPartFile* sender,CUpDownClient* source){
	if (sender->IsStopped()){
		delete source;
		return;
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
					return;
				}
				// set request for this source
				if (cur_client->AddRequestForAnotherFile(sender)){
					theApp.emuledlg->transferwnd->downloadlistctrl.AddSource(sender,cur_client,true);
					delete source;
					return;
				}
				else{
					delete source;
					return;
				}
			}
		}
	}
	//our new source is real new but maybe it is already uploading to us?
	//if yes the known client will be attached to the var "source"
	//and the old sourceclient will be deleted
	if (theApp.clientlist->AttachToAlreadyKnown(&source,0)){
#ifdef _DEBUG
		if (source->reqfile){
			// if a client sent us wrong sources (sources for some other file for which we asked but which we are also
			// downloading) we may get a little in trouble here when "moving" this source to some other partfile without
			// further checks and updates.
			if (md4cmp(source->reqfile->GetFileHash(), sender->GetFileHash()) != 0)
				AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddSource -- added potential wrong source (%u)(diff. filehash) to file \"%s\""), source->GetUserIDHybrid(), sender->GetFileName());
			if (source->reqfile->GetPartCount() != 0 && source->reqfile->GetPartCount() != sender->GetPartCount())
				AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddSource -- added potential wrong source (%u)(diff. partcount) to file \"%s\""), source->GetUserIDHybrid(), sender->GetFileName());
		}
#endif
		source->reqfile = sender;
	}
	else{
		theApp.clientlist->AddClient(source,true);
	}
	
	if (source->GetFileRate()>0 || source->GetFileComment().GetLength()>0) sender->UpdateFileRatingCommentAvail();

#ifdef _DEBUG
	if (source->GetPartCount()!=0 && source->GetPartCount()!=sender->GetPartCount()){
		DEBUG_ONLY(AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddSource -- New added source (%u, %s) had still value in partcount"), source->GetUserIDHybrid(), sender->GetFileName()));
	}
#endif

	sender->srclist.AddTail(source);
	theApp.emuledlg->transferwnd->downloadlistctrl.AddSource(sender,source,false);
	//UpdateDisplayedInfo();
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
			AddDebugLogLine(false, _T("Ignored already known source with IP=%s"), inet_ntoa(*(in_addr*)&nClientIP));
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
			return false;
		}
	}
#ifdef _DEBUG
	if (source->reqfile){
		// if a client sent us wrong sources (sources for some other file for which we asked but which we are also
		// downloading) we may get a little in trouble here when "moving" this source to some other partfile without
		// further checks and updates.
		if (md4cmp(source->reqfile->GetFileHash(), sender->GetFileHash()) != 0)
			AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddKnownSource -- added potential wrong source (%u)(diff. filehash) to file \"%s\""), source->GetUserIDHybrid(), sender->GetFileName());
		if (source->reqfile->GetPartCount() != 0 && source->reqfile->GetPartCount() != sender->GetPartCount())
			AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddKnownSource -- added potential wrong source (%u)(diff. partcount) to file \"%s\""), source->GetUserIDHybrid(), sender->GetFileName());
	}
#endif
	source->reqfile = sender;
	if (source->GetFileRate()>0 || source->GetFileComment().GetLength()>0)
		sender->UpdateFileRatingCommentAvail();
	sender->srclist.AddTail(source);
	source->SetSourceFrom(SF_PASSIVE);
#ifdef _DEBUG
	if (source->GetPartCount()!=0 && source->GetPartCount()!=sender->GetPartCount()){
		DEBUG_ONLY(AddDebugLogLine(false, _T("*** CDownloadQueue::CheckAndAddKnownSource -- New added source (%u, %s) had still value in partcount"), source->GetUserIDHybrid(), sender->GetFileName()));
	}
#endif

	theApp.emuledlg->transferwnd->downloadlistctrl.AddSource(sender,source,false);
	//UpdateDisplayedInfo();
	return true;
}

bool CDownloadQueue::RemoveSource(CUpDownClient* toremove, bool	updatewindow, bool bDoStatsUpdate){
	bool removed = false;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;){
		CPartFile* cur_file = filelist.GetNext(pos);
		for (POSITION pos2 = cur_file->srclist.GetHeadPosition();pos2 != 0; cur_file->srclist.GetNext(pos2)){
			if (toremove == cur_file->srclist.GetAt(pos2)){
				cur_file->srclist.RemoveAt(pos2);
				
				removed = true;
				if ( bDoStatsUpdate ){
					cur_file->RemoveDownloadingSource(toremove);
					cur_file->NewSrcPartsInfo();
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

	if (toremove->GetFileComment().GetLength()>0 || toremove->GetFileRate()>0 )
		toremove->reqfile->UpdateFileRatingCommentAvail();

	if (updatewindow){
		toremove->SetDownloadState(DS_NONE);
		theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource(toremove,0);
	}
	toremove->ResetFileStatusInfo();
	toremove->reqfile = 0;
	return removed;
}

void CDownloadQueue::RemoveFile(CPartFile* toremove)
{
	RemoveLocalServerRequest(toremove);

	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		if (toremove == filelist.GetAt(pos)){
			filelist.RemoveAt(pos);
			UpdatePNRFile(); //<<-- enkeyDEV(ColdShine) -PartfileNameRecovery-
			return;
		}
	}
	SortByPriority();
	CheckDiskspace();	// SLUGFILLER: checkDiskspace
}

void CDownloadQueue::DeleteAll(){
	POSITION pos;
	for (pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		CPartFile* cur_file = filelist.GetAt(pos);
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
		if (theApp.glob_prefs->GetDebugServerUDPLevel() > 0)
			Debug(">>> Sending OP__GlobGetSources to server(#%02x) %-15s (%3u of %3u); FileIDs=%u\n", cur_udpserver->GetUDPFlags(), cur_udpserver->GetAddress(), m_iSearchedServers + 1, theApp.serverlist->GetServerCount(), iFileIDs);
		Packet packet(data);
		packet.opcode = OP_GLOBGETSOURCES;
		theApp.uploadqueue->AddUpDataOverheadServer(packet.size);
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
						cur_udpserver = theApp.serverlist->GetNextServer(cur_udpserver, lastfile);	// itsonlyme: cacheUDPsearchResults
						/*//original
						cur_udpserver = theApp.serverlist->GetNextServer(cur_udpserver);
						*/
						//Morph End - added by AndCycle, itsonlyme: cacheUDPsearchResults
						m_cRequestsSentToServer = 0;
						if (cur_udpserver == NULL)
						{
							// finished asking all servers for all files
							if (theApp.glob_prefs->GetDebugServerUDPLevel() > 0 && theApp.glob_prefs->GetDebugServerSourcesLevel() > 0)
								Debug("Finished UDP search processing for all servers (%u)\n", theApp.serverlist->GetServerCount());

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

		if (!bSentPacket && nextfile && nextfile->GetSourceCount() < theApp.glob_prefs->GetMaxSourcePerFileUDP())
		{
			dataGlobGetSources.Write(nextfile->GetFileHash(), 16);
			iFiles++;
			if (theApp.glob_prefs->GetDebugServerUDPLevel() > 0 && theApp.glob_prefs->GetDebugServerSourcesLevel() > 0)
				Debug(">>> Sending OP__GlobGetSources to server(#%02x) %-15s (%3u of %3u); Buff  %u=%s\n", cur_udpserver->GetUDPFlags(), cur_udpserver->GetAddress(), m_iSearchedServers + 1, theApp.serverlist->GetServerCount(), iFiles, DbgGetFileInfo(nextfile->GetFileHash()));
		}
	}

	ASSERT( dataGlobGetSources.GetLength() == 0 || !bSentPacket );

	if (!bSentPacket && dataGlobGetSources.GetLength() > 0)
		SendGlobGetSourcesUDPPacket(&dataGlobGetSources);

	// send max 35 UDP request to one server per interval
	// if we have more than 35 files, we rotate the list and use it as queue
	if (m_cRequestsSentToServer >= MAX_REQUESTS_PER_SERVER)
	{
		if (theApp.glob_prefs->GetDebugServerUDPLevel() > 0 && theApp.glob_prefs->GetDebugServerSourcesLevel() > 0)
			Debug("Rotating file list\n");

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
	UpdatePNRFile(); //<<-- enkeyDEV(ColdShine) -PartfileNameRecovery-
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
	if (!theApp.glob_prefs->IsCheckDiskspaceEnabled())
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
	uint64 nTotalAvailableSpace = bNotEnoughSpaceLeft ? 0 : GetFreeDiskSpaceX(theApp.glob_prefs->GetTempDir());
	if (theApp.glob_prefs->GetMinFreeDiskSpace() == 0)
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

			if (nTotalAvailableSpace < theApp.glob_prefs->GetMinFreeDiskSpace())
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

// -khaos--+++> Rewritten GetDownloadStats
void CDownloadQueue::GetDownloadStats(int results[]) {
	
	for (int i=0;i<19;i++)
		results[i] = 0;

	for (POSITION pos =theApp.downloadqueue->filelist.GetHeadPosition();pos != 0;theApp.downloadqueue->filelist.GetNext(pos)){
		CPartFile* cur_file = filelist.GetAt(pos);

		results[0]+=cur_file->GetSourceCount();
		results[1]+=cur_file->GetTransferingSrcCount();
		// -khaos-/
		results[2]+=cur_file->GetSrcStatisticsValue(DS_ONQUEUE);
		results[3]+=cur_file->GetSrcStatisticsValue(DS_REMOTEQUEUEFULL);
		results[4]+=cur_file->GetSrcStatisticsValue(DS_NONEEDEDPARTS);
		results[5]+=cur_file->GetSrcStatisticsValue(DS_CONNECTED);
		results[6]+=cur_file->GetSrcStatisticsValue(DS_REQHASHSET);
		results[7]+=cur_file->GetSrcStatisticsValue(DS_CONNECTING);
		results[8]+=cur_file->GetSrcStatisticsValue(DS_WAITCALLBACK);
		results[9]+=cur_file->GetSrcStatisticsValue(DS_TOOMANYCONNS);
		results[10]+=cur_file->GetSrcStatisticsValue(DS_LOWTOLOWIP);
		results[11]+=cur_file->GetSrcStatisticsValue(DS_NONE);
		results[12]+=cur_file->GetSrcStatisticsValue(DS_ERROR);
		results[13]+=cur_file->GetSrcStatisticsValue(DS_BANNED);
		results[14]+=cur_file->TotalPacketsSavedDueToICH();
		results[15]+=cur_file->GetSrcA4AFCount();
		// /-khaos-

		results[16]+=cur_file->src_stats[0];
		results[17]+=cur_file->src_stats[1];
		results[18]+=cur_file->src_stats[2];

	}
} // GetDownloadStats
// <-----khaos-

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
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos))
	{
		CPartFile* cur_file = filelist.GetAt(pos);
		if (cur_file->GetCategory() == cat)
		{
			useOrder++;
			cur_file->SetCategory(useCat);
			cur_file->SetCatResumeOrder(useOrder);
		}
		else if (cur_file->GetCategory() > cat)
			cur_file->SetCategory(cur_file->GetCategory() - 1);
	}
}
// khaos::categorymod-

void CDownloadQueue::SetCatPrio(int cat, uint8 newprio){
	// itsonlyme: selFix
	CArray<CPartFile*> filesList;
	CPartFile* cur_file;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos))
		filesList.Add(filelist.GetAt(pos));
	for (int i = 0; i < filesList.GetSize(); i++){
		cur_file = filesList[i];
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
	// itsonlyme: selFix
}

void CDownloadQueue::SetCatStatus(int cat, int newstatus){
	bool reset=false;
	CPartFile* cur_file;

	POSITION pos= filelist.GetHeadPosition();
	while (pos != 0) {
		cur_file = filelist.GetAt(pos);
		if (!cur_file)
			continue;
		if ( cat==-1 || (cat==-2 && cur_file->GetCategory()==0) ||
			(cat==0 && cur_file->CheckShowItemInGivenCat(cat))
			 || ( cat>0 && cat==cur_file->GetCategory() ) )
		{
			 switch (newstatus) {
				case MP_CANCEL:cur_file->DeleteFile();reset=true;break;
				case MP_PAUSE:cur_file->PauseFile();break;
				case MP_STOP:cur_file->StopFile();break;
				case MP_RESUME: if (cur_file->GetStatus()==PS_PAUSED) cur_file->ResumeFile();break;
			}
		}
		filelist.GetNext(pos);
		if (reset) {reset=false;pos= filelist.GetHeadPosition();}
	}
}

void CDownloadQueue::MoveCat(uint8 from, uint8 to){
	if (from < to) --to;
	uint8 mycat;
	CPartFile* cur_file;

	POSITION pos= filelist.GetHeadPosition();
	while (pos != 0) {
		cur_file = filelist.GetAt(pos);
		if (!cur_file) continue;
		mycat=cur_file->GetCategory();
		if ((mycat>=min(from,to) && mycat<=max(from,to)) ) {
			//if ((from<to && (mycat<from || mycat>to)) || (from>to && (mycat>from || mycat<to)) )  continue; //not affected

			if (mycat==from) cur_file->SetCategory(to); 
			else {
				if (from<to) cur_file->SetCategory(mycat-1);
					else cur_file->SetCategory(mycat+1);
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
	CPartFile* cur_file;
	for (POSITION pos = filelist.GetHeadPosition(); pos != NULL; filelist.GetNext(pos)) {
		cur_file = (CPartFile*)filelist.GetAt(pos);
		if (cur_file != NULL) cur_file->SetA4AFAuto(false);
	}
}

//MORPH START - Removed by SiRoB, Due to Khaos Categorie
/*// HoaX_69: BEGIN AutoCat function
void CDownloadQueue::SetAutoCat(CPartFile* newfile){
	if(theApp.glob_prefs->GetCatCount()>1){
		for (int ix=1;ix<theApp.glob_prefs->GetCatCount();ix++){	
			int curPos = 0;
			CString catExt = theApp.glob_prefs->GetCategory(ix)->autocat;
			catExt.MakeLower();

			// No need to compare agains an empty AutoCat array
			if( catExt == "")
				continue;

			CString fullname = newfile->GetFileName();
			fullname.MakeLower();
			CString cmpExt = catExt.Tokenize("|", curPos);

			while (cmpExt != "") {
				// HoaX_69: Allow wildcards in autocat string
				//  thanks to: bluecow, khaos and SlugFiller
				if(cmpExt.Find(CString("*")) != -1 || cmpExt.Find(CString("?")) != -1){
					// Use wildcards
					char* file = fullname.GetBuffer();
					char* spec = cmpExt.GetBuffer();
					if(PathMatchSpec(file, spec)){
						newfile->SetCategory(ix);
						return;
					}
				}else{
					if(fullname.Find(cmpExt) != -1){
						newfile->SetCategory(ix);
						return;
					}
				}
				cmpExt = catExt.Tokenize("|",curPos);
			}
		}
	}
}
// HoaX_69: END*/
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
					AddDebugLogLine(false, "Local server source request for file \"%s\" not sent because of status '%s'", cur_file->GetFileName(), cur_file->getPartfileStatus());
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
				if (theApp.glob_prefs->GetDebugServerTCPLevel() > 0)
					Debug(">>> Sending OP__GetSources(%2u/%2u); %s\n", iFiles, iMaxFilesPerTcpFrame, DbgGetFileInfo(cur_file->GetFileHash()));
				dataTcpFrame.Write(packet->GetPacket(), packet->GetRealPacketSize());
				delete packet;

				if ( theApp.glob_prefs->GetDebugSourceExchange() )
					AddDebugLogLine( false, "Send:Source Request Server File(%s)", cur_file->GetFileName() );
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
			theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
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

void CDownloadQueue::GetDownloadStats(int results[],uint64& pui64TotFileSize,uint64& pui64TotBytesLeftToTransfer,uint64& pui64TotNeededSpace) 
{
	results[0]=0;
	results[1]=0;
	results[2]=0;
	for (POSITION pos = filelist.GetHeadPosition();pos != 0; filelist.GetNext(pos)){
		uint32 ui32SizeToTransfer=0;
		uint32 ui32NeededSpace=0;
		CPartFile* cur_file = filelist.GetAt(pos);
		//Morph - modified by AndCycle, Bug In Getdownloadstats,  Jan 29 2004, 03:47 PM
		//if (cur_file->getPartfileStatus() != GetResString(IDS_PAUSED)) { 
		if (cur_file->GetStatus() != PS_PAUSED) { 
			cur_file->GetSizeToTransferAndNeededSpace (ui32SizeToTransfer,ui32NeededSpace);
			pui64TotFileSize += cur_file->GetFileSize(); 
			pui64TotBytesLeftToTransfer += ui32SizeToTransfer;
			pui64TotNeededSpace += ui32NeededSpace;
			results[2]++; 
		} 
		results[0]+=cur_file->GetSourceCount();
		results[1]+=cur_file->GetTransferingSrcCount();
	}
}

///////////////////////////////////////////////////////////////////////////////
// CSourceHostnameResolveWnd

// SLUGFILLER: hostnameSources

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

void CSourceHostnameResolveWnd::AddToResolve(const uchar* fileid, LPCTSTR pszHostname, uint16 port)
{
	bool bResolving = !m_toresolve.IsEmpty();

	// double checking
	if (!theApp.downloadqueue->GetFileByID(fileid))
		return;

	Hostname_Entry* entry = new Hostname_Entry;
	md4cpy(entry->fileid, fileid);
	entry->strHostname = pszHostname;
	entry->port = port;
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
					CSafeMemFile sources(1+4+2);
					uint8 uSources = 1;
					uint16 nPort = resolved->port;
					sources.Write(&uSources, 1);
					sources.Write(&nIP, 4);
					sources.Write(&nPort, 2);
					sources.SeekToBegin();
					file->AddSources(&sources,0,0);
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

// START enkeyDEV(ColdShine) -PartfileNameRecovery-
void CDownloadQueue::UpdatePNRFile(CPartFile * ppfUpdate)
{
	// All PNR records are 1KB fixed-length:
	// |--262---|   |---262---|   |--262---|   |-236-|   |2-|
	// 1 <=260  1   1  <=260  1   1 <=260  1             1 1
	// <partname<   >ed2k_link>   "filename"             \r\n

	CFile file;
	if (!file.Open(theApp.glob_prefs->GetConfigDir() + _T("PNRecovery.dat"), CFile::modeCreate | CFile::modeNoTruncate | CFile::modeReadWrite | CFile::osSequentialScan))
	{
		AddLogLine(true, GetResString(IDS_ERROR_SAVEFILE) + _T(" PNRecovery.dat"));
		return;
	}
	char achRecord[1024];
	if (ppfUpdate)
	{
		// Only one record to update: need to find it (or append at EOF), then overwrite.
		BuildPNRRecord(ppfUpdate, achRecord, sizeof(achRecord));
		char achField1[262];
		for (unsigned cRecords = file.GetLength() >> 10; cRecords--;)
	{
			file.Read(achField1, sizeof(achField1));
			if (memicmp(achField1, achRecord, sizeof(achField1)) == 0)			// Have we found it?
			{
				file.Seek(-int(sizeof(achField1)), CFile::current);				// Seek back to beginning of record.
				break;
			}
			else
				file.Seek(1024 - sizeof(achField1), CFile::current);			// Skip this record.
		}
		file.Write(achRecord, sizeof(achRecord));								// We are at EOF or just over the record, write either case.
	}
	else
		// Need to write the entire list, so just write 'em in random order.
		for (POSITION pos = filelist.GetHeadPosition(); pos != 0; filelist.GetNext(pos))
		{
			BuildPNRRecord(filelist.GetAt(pos), achRecord, sizeof(achRecord));
			file.Write(achRecord, sizeof(achRecord));
	}
}

void CDownloadQueue::BuildPNRRecord(CPartFile * ppf, char * pszBuff, unsigned cchBuffMax)
{
	ASSERT(cchBuffMax == 1024);													// PNR record size.
	unsigned cch = wnsprintf(pszBuff, 262, "<%s<", ppf->GetFullName());			// Write full path to the partfile.
	FillMemory(pszBuff + cch, 262 - cch, ' ');									// Pad out "partfile" field to 262 chars.
	cch = wnsprintf(pszBuff + 262, 262, ">%s>", CreateED2kLink(ppf));	// Write eD2k link.
	FillMemory(pszBuff + 262 + cch, 262 - cch, ' ');							// Pad out "link" field to 262 chars.
	cch = wnsprintf(pszBuff + 262 + 262, 262, "\"%s\"", ppf->GetFileName());	// Write final filename.
	FillMemory(pszBuff + 262 + 262 + cch, 262 + 236 - cch, ' ');				// Pad out "filename" field to 262 chars, and fill 236 chars of padding.
	pszBuff[1022] = '\r';														// Add carriage return to make it readable with (e.g.) notepad.
	pszBuff[1023] = '\n';
}
// END enkeyDEV(ColdShine) -PartfileNameRecovery-
//MORPH START - Added by SiRoB, ZZ Ratio
bool CDownloadQueue::IsFilesPowershared()
{
	for (POSITION pos = filelist.GetHeadPosition();pos != 0;filelist.GetNext(pos)){
		CPartFile* cur_file =  filelist.GetAt(pos);
		if (cur_file->IsPartFile() && cur_file->GetPowerSharedMode()==1)
			return true;
	}
	return false;
}
//MORPH END   - Added by SiRoB, ZZ Ratio