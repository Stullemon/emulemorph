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
#include <math.h>
#include <sys/stat.h>
#include <io.h>
#include <winioctl.h>
#ifdef _DEBUG
#include "DebugHelpers.h"
#endif
#include "emule.h"
#include "PartFile.h"
#include "UpDownClient.h"
#include "ED2KLink.h"
#include "Preview.h"
#include "ArchiveRecovery.h"
#include "SearchList.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "kademlia/kademlia/search.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/utils/MiscUtils.h"
#include "kademlia/kademlia/prefs.h"
#include "DownloadQueue.h"
#include "IPFilter.h"
#include "MMServer.h"
#include "UploadQueue.h"
#include "OtherFunctions.h"
#include "Packets.h"
#include "Preferences.h"
#include "SafeFile.h"
#include "SharedFileList.h"
#include "ListenSocket.h"
#include "Sockets.h"
#include "Server.h"
#include "KnownFileList.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#include "TransferWnd.h"
#include "TaskbarNotifier.h"
#endif
#include "ServerList.h" //Morph - added by AndCycle, itsonlyme: cacheUDPsearchResults

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//Morph Start - added by AndCycle, itsonlyme: cacheUDPsearchResults
#define MAX_PREF_SERVERS	10	// itsonlyme: cacheUDPsearchResults
//Morph End - added by AndCycle, itsonlyme: cacheUDPsearchResults

// Barry - use this constant for both places
#define PROGRESS_HEIGHT 3

CBarShader CPartFile::s_LoadBar(PROGRESS_HEIGHT); // Barry - was 5
CBarShader CPartFile::s_ChunkBar(16); 

IMPLEMENT_DYNAMIC(CPartFile, CKnownFile)

CPartFile::CPartFile()
{
	Init();
}

CPartFile::CPartFile(CSearchFile* searchresult)
{
	Init();
	md4cpy(m_abyFileHash, searchresult->GetFileHash());
	for (int i = 0; i < searchresult->taglist.GetCount();i++){
		const CTag* pTag = searchresult->taglist[i];
		switch (pTag->tag.specialtag){
			case FT_FILENAME:{
				if (pTag->IsStr())
					SetFileName(pTag->tag.stringvalue);
				break;
			}
			case FT_FILESIZE:{
				if (pTag->IsInt())
					SetFileSize(pTag->tag.intvalue);
				break;
			}
			default:{
				bool bTagAdded = false;
				if (pTag->tag.specialtag != 0 && pTag->tag.tagname == NULL && (pTag->IsStr() || pTag->IsInt()))
				{
					static const struct
					{
						uint8	nName;
						uint8	nType;
					} _aMetaTags[] = 
					{
						{ FT_MEDIA_ARTIST,  2 },
						{ FT_MEDIA_ALBUM,   2 },
						{ FT_MEDIA_TITLE,   2 },
						{ FT_MEDIA_LENGTH,  3 },
						{ FT_MEDIA_BITRATE, 3 },
						{ FT_MEDIA_CODEC,   2 },
						{ FT_FILETYPE,		2 },
						{ FT_FILEFORMAT,	2 }
					};
					for (int t = 0; t < ARRSIZE(_aMetaTags); t++)
					{
						if (pTag->tag.type == _aMetaTags[t].nType && pTag->tag.specialtag == _aMetaTags[t].nName)
						{
							// skip string tags with empty string values
							if (pTag->IsStr() && (pTag->tag.stringvalue == NULL || pTag->tag.stringvalue[0] == '\0'))
								break;

							// skip integer tags with '0' values
							if (pTag->IsInt() && pTag->tag.intvalue == 0)
								break;

							TRACE("CPartFile::CPartFile(CSearchFile*): added tag %s\n", pTag->GetFullInfo());
							CTag* newtag = new CTag(pTag->tag);
							taglist.Add(newtag);
							bTagAdded = true;
							break;
						}
					}
				}

				if (!bTagAdded)
					TRACE("CPartFile::CPartFile(CSearchFile*): ignored tag %s\n", pTag->GetFullInfo());
			}
		}
	}
	CreatePartFile();
}

CPartFile::CPartFile(CString edonkeylink)
{
	CED2KLink* pLink = 0;
	try {
		pLink = CED2KLink::CreateLinkFromUrl(edonkeylink);
		_ASSERT( pLink != 0 );
		CED2KFileLink* pFileLink = pLink->GetFileLink();
		if (pFileLink==0) 
			throw GetResString(IDS_ERR_NOTAFILELINK);
		InitializeFromLink(pFileLink);
	} catch (CString error) {
		char buffer[200];
		sprintf(buffer,GetResString(IDS_ERR_INVALIDLINK),error.GetBuffer());
		AddLogLine(true, GetResString(IDS_ERR_LINKERROR), buffer);
		SetStatus(PS_ERROR);
	}
	delete pLink;
}

void CPartFile::InitializeFromLink(CED2KFileLink* fileLink)
{
	Init();
	try{
		SetFileName(fileLink->GetName());
		SetFileSize(fileLink->GetSize());
		md4cpy(m_abyFileHash, fileLink->GetHashKey());
		if (!theApp.downloadqueue->IsFileExisting(m_abyFileHash))
			CreatePartFile();
		else
			SetStatus(PS_ERROR);
	}
	catch(CString error){
		char buffer[200];
		sprintf(buffer, GetResString(IDS_ERR_INVALIDLINK), error.GetBuffer());
		AddLogLine(true, GetResString(IDS_ERR_LINKERROR), buffer);
		SetStatus(PS_ERROR);
	}
}

CPartFile::CPartFile(CED2KFileLink* fileLink)
{
	InitializeFromLink(fileLink);
}

void CPartFile::Init(){
	newdate = true;
	lastsearchtime = 0;
	lastsearchtimeKad = 0;
	lastpurgetime = ::GetTickCount();
	paused = false;
	stopped= false;
	status = PS_EMPTY;
	insufficient = false;
	m_bCompletionError = false;
	transfered = 0;
	m_iLastPausePurge = time(NULL);
	m_AllocateThread=NULL;

	if(thePrefs.GetNewAutoDown()){
		m_iDownPriority = PR_HIGH;
		m_bAutoDownPriority = true;
	}
	else{
		m_iDownPriority = PR_NORMAL;
		m_bAutoDownPriority = false;
	}
	srcarevisible = false;
	memset(m_anStates,0,sizeof(m_anStates));
	datarate = 0;
	hashsetneeded = true;
	count = 0;
	percentcompleted = 0;
	completedsize=0;
	m_bPreviewing = false;
	lastseencomplete = NULL;
	availablePartsCount=0;
	m_ClientSrcAnswered = 0;
	m_LastNoNeededCheck = 0;
	m_iRate = 0;
	m_strComment = "";
	m_nTotalBufferData = 0;
	m_nLastBufferFlushTime = 0;
	m_bRecoveringArchive = false;
	m_iGainDueToCompression = 0;
	m_iLostDueToCorruption = 0;
	m_iTotalPacketsSavedDueToICH = 0;
	hasRating	= false;
	hasComment	= false;
	hasBadRating= false;
	m_category=0;
	m_lastRefreshedDLDisplay = 0;
	m_is_A4AF_auto=false;
	m_bLocalSrcReqQueued = false;
	memset(src_stats,0,sizeof(src_stats));
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 0;
	m_nCompleteSourcesCountLo = 0;
	m_nCompleteSourcesCountHi = 0;
	m_dwFileAttributes = 0;
	m_bDeleteAfterAlloc=false;
	m_tActivated = 0;
	m_nDlActiveTime = 0;
	m_tLastModified = 0;
	m_tCreated = 0;

	m_PartsHashing = 0;		// SLUGFILLER: SafeHash
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	m_nVirtualCompleteSourcesCount = 0;
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - HotFix by SiRoB, khaos 14.6 missing
	m_iSesCompressionBytes = 0;
	m_iSesCorruptionBytes = 0;
	//MORPH END - HotFix by SiRoB, khaos 14.6 missing
	// khaos::categorymod+
	m_catResumeOrder=0;
	// khaos::categorymod-
	// khaos::accuratetimerem+
	m_nSecondsActive = 0;
	m_nInitialBytes = 0;
	m_dwActivatedTick = 0;
	// khaos::accuratetimerem-
	// khaos::kmod+
	m_bForceAllA4AF = false;
	m_bForceA4AFOff = false;
	// khaos::kmod-
	//MORPH START - Added by SiRoB, ZZ Upload System 20030723-0133
	m_NumberOfClientsWithPartStatus = 0;
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030723-0133
	//MORPH START - Added by SiRoB,  SharedStatusBar CPU Optimisation
	InChangedSharedStatusBar = false;
	//MORPH END   - Added by SiRoB,  SharedStatusBar CPU Optimisation
}


CPartFile::~CPartFile()
{
	// Barry - Ensure all buffered data is written
	try{
		if (m_AllocateThread != NULL){
			HANDLE hThread = m_AllocateThread->m_hThread;
			// 2 minutes to let the thread finish
			if (WaitForSingleObject(hThread, 120000) == WAIT_TIMEOUT)
				TerminateThread(hThread, 100);
		}

		if (m_hpartfile.m_hFile != INVALID_HANDLE_VALUE)
			FlushBuffer(true);
	}
	catch(CFileException* e){
		e->Delete();
	}
	
	if (m_hpartfile.m_hFile != INVALID_HANDLE_VALUE){
		// commit file and directory entry
		m_hpartfile.Close();
		// Update met file (with current directory entry)
		SavePartFile();
	}

	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;)
		delete gaplist.GetNext(pos);

	pos = m_BufferedData_list.GetHeadPosition();
	while (pos){
		PartFileBufferedData *item = m_BufferedData_list.GetNext(pos);
		delete[] item->data;
		delete item;
	}
}

#ifdef _DEBUG
void CPartFile::AssertValid() const
{
	CKnownFile::AssertValid();

	(void)lastsearchtime;
	(void)lastsearchtimeKad;
	srclist.AssertValid();
	A4AFsrclist.AssertValid();
	(void)lastseencomplete;
	m_hpartfile.AssertValid();
	m_FileCompleteMutex.AssertValid();
	(void)src_stats;
	CHECK_BOOL(m_bPreviewing);
	CHECK_BOOL(m_bRecoveringArchive);
	CHECK_BOOL(m_bLocalSrcReqQueued);
	CHECK_BOOL(srcarevisible);
	CHECK_BOOL(hashsetneeded);
	(void)m_iLastPausePurge;
	(void)count;
	(void)m_anStates;
	(void)completedsize;
	(void)m_iLostDueToCorruption;
	(void)m_iGainDueToCompression;
	(void)m_iTotalPacketsSavedDueToICH; 
	(void)datarate;
	(void)m_fullname;
	(void)m_partmetfilename;
	(void)transfered;
	CHECK_BOOL(paused);
	CHECK_BOOL(stopped);
	CHECK_BOOL(insufficient);
	CHECK_BOOL(m_bCompletionError);
	ASSERT( m_iDownPriority == PR_LOW || m_iDownPriority == PR_NORMAL || m_iDownPriority == PR_HIGH );
	CHECK_BOOL(m_bAutoDownPriority);
	ASSERT( status == PS_READY || status == PS_EMPTY || status == PS_WAITINGFORHASH || status == PS_ERROR || status == PS_COMPLETING || status == PS_COMPLETE );
	CHECK_BOOL(newdate);
	(void)lastpurgetime;
	(void)m_LastNoNeededCheck;
	gaplist.AssertValid();
	requestedblocks_list.AssertValid();
	m_SrcpartFrequency.AssertValid();
	ASSERT( percentcompleted >= 0.0F && percentcompleted <= 100.0F );
	corrupted_list.AssertValid();
	(void)availablePartsCount;
	(void)m_ClientSrcAnswered;
	(void)s_LoadBar;
	(void)s_ChunkBar;
	CHECK_BOOL(hasRating);
	CHECK_BOOL(hasBadRating);
	CHECK_BOOL(hasComment);
	(void)m_lastRefreshedDLDisplay;
	m_downloadingSourceList.AssertValid();
	m_BufferedData_list.AssertValid();
	(void)m_nTotalBufferData;
	(void)m_nLastBufferFlushTime;
	(void)m_category;
	CHECK_BOOL(m_is_A4AF_auto);
	(void)m_dwFileAttributes;
}

void CPartFile::Dump(CDumpContext& dc) const
{
	CKnownFile::Dump(dc);
}
#endif

void CPartFile::CreatePartFile()
{
	// use lowest free partfilenumber for free file (InterCeptor)
	int i = 0; 
	CString filename; 
	do{ 
		i++; 
		filename.Format("%s\\%03i.part", thePrefs.GetTempDir(), i); 
	}
	while(PathFileExists(filename));
	m_partmetfilename.Format("%03i.part.met",i); 
	SetPath(thePrefs.GetTempDir());
	m_fullname.Format("%s\\%s", thePrefs.GetTempDir(), m_partmetfilename);

	CTag* partnametag = new CTag(FT_PARTFILENAME,RemoveFileExtension(m_partmetfilename));
	taglist.Add(partnametag);
	
	Gap_Struct* gap = new Gap_Struct;
	gap->start = 0;
	gap->end = m_nFileSize-1;
	gaplist.AddTail(gap);

	CString partfull(RemoveFileExtension(m_fullname));
	SetFilePath(partfull);
	if (!m_hpartfile.Open(partfull,CFile::modeCreate|CFile::modeReadWrite|CFile::shareDenyWrite|CFile::osSequentialScan)){
		AddLogLine(false,GetResString(IDS_ERR_CREATEPARTFILE));
		SetStatus(PS_ERROR);
	}
	else{
		struct _stat fileinfo;
		if (_stat(partfull, &fileinfo) == 0){
			m_tLastModified = fileinfo.st_mtime;
			m_tCreated = fileinfo.st_ctime;
		}
		else
			AddDebugLogLine(false, _T("Failed to get file date for \"%s\" - %hs"), partfull, strerror(errno));
	}
	m_dwFileAttributes = GetFileAttributes(partfull);
	if (m_dwFileAttributes == INVALID_FILE_ATTRIBUTES)
		m_dwFileAttributes = 0;

	// SLUGFILLER: SafeHash - setting at the hotspot
	if (GetED2KPartCount() > 1)
		hashsetneeded = true;
	else {
		hashsetneeded = false;
		uchar* cur_hash = new uchar[16];
		md4cpy(cur_hash, m_abyFileHash);
		hashlist.Add(cur_hash);
	}

	// the important part
	m_PartsShareable.SetSize(GetPartCount());
	for (uint32 i = 0; i < GetPartCount();i++)
		m_PartsShareable[i] = false;
	// SLUGFILLER: SafeHash

	m_SrcpartFrequency.SetSize(GetPartCount());
	for (uint32 i = 0; i < GetPartCount();i++)
		m_SrcpartFrequency[i] = 0;
	paused = false;

	if (thePrefs.AutoFilenameCleanup())
		SetFileName(CleanupFilename(GetFileName()));

	SavePartFile();
	SetActive(theApp.IsConnected());
}

uint8 CPartFile::LoadPartFile(LPCTSTR in_directory,LPCTSTR in_filename, bool getsizeonly)
{
	bool isnewstyle;
	uint8 version,partmettype=PMT_UNKNOWN;

	CMap<uint16, uint16, Gap_Struct*, Gap_Struct*> gap_map; // Slugfiller
	transfered = 0;
	m_partmetfilename = in_filename;
	SetPath(in_directory);
	m_fullname.Format("%s\\%s", GetPath(), m_partmetfilename);
	
	// readfile data form part.met file
	CSafeBufferedFile metFile;
	CFileException fexpMet;
	if (!metFile.Open(m_fullname, CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary, &fexpMet)){
		CString strError;
		strError.Format(GetResString(IDS_ERR_OPENMET), m_partmetfilename, _T(""));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexpMet.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("%s"), strError);
		return false;
	}
	setvbuf(metFile.m_pStream, NULL, _IOFBF, 16384);

	try{
		version = metFile.ReadUInt8();
		if (version != PARTFILE_VERSION && version!= PARTFILE_SPLITTEDVERSION ){
			metFile.Close();
			AddLogLine(false, GetResString(IDS_ERR_BADMETVERSION), m_partmetfilename, GetFileName());
			return false;
		}
		
		isnewstyle=(version== PARTFILE_SPLITTEDVERSION);
		partmettype= isnewstyle?PMT_SPLITTED:PMT_DEFAULTOLD;
		if (!isnewstyle) {
			uint8 test[4];
			metFile.Seek(24, CFile::begin);
			metFile.Read(&test[0],1);
			metFile.Read(&test[1],1);
			metFile.Read(&test[2],1);
			metFile.Read(&test[3],1);

			metFile.Seek(1, CFile::begin);

			if (test[0]==0 && test[1]==0 && test[2]==2 && test[3]==1) {
				isnewstyle=true;	// edonkeys so called "old part style"
				partmettype=PMT_NEWOLD;
			}
		}

		if (isnewstyle) {
			uint32 temp;
			metFile.Read(&temp,4);

			if (temp==0) {	// 0.48 partmets - different again
				LoadHashsetFromFile(&metFile, false);
			} else {
				uchar gethash[16];
				metFile.Seek(2, CFile::begin);
				LoadDateFromFile(&metFile);
				metFile.Read(&gethash, 16);
				md4cpy(m_abyFileHash, gethash);
			}

		} else {
			LoadDateFromFile(&metFile);
			LoadHashsetFromFile(&metFile, false);
		}

		UINT tagcount = metFile.ReadUInt32();
		for (UINT j = 0; j < tagcount; j++){
			CTag* newtag = new CTag(&metFile);
			if (!getsizeonly || (getsizeonly && (newtag->tag.specialtag==FT_FILESIZE || newtag->tag.specialtag==FT_FILENAME))){
				switch(newtag->tag.specialtag){
					case FT_FILENAME:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 2) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						if (newtag->tag.stringvalue == NULL) {
							AddLogLine(true, GetResString(IDS_ERR_METCORRUPT), m_partmetfilename, GetFileName());
							delete newtag;
							return false;
						}
						SetFileName(newtag->tag.stringvalue);
						delete newtag;
						break;
					}
					case FT_LASTSEENCOMPLETE:{
						if (newtag->tag.type == 3)
							lastseencomplete = newtag->tag.intvalue;
						delete newtag;
						break;
					}
					case FT_FILESIZE:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						SetFileSize(newtag->tag.intvalue);
						delete newtag;
						break;
					}
					case FT_TRANSFERED:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						transfered = newtag->tag.intvalue;
						delete newtag;
						break;
					}
					case FT_FILETYPE:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 2) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						SetFileType(newtag->tag.stringvalue);
						delete newtag;
						break;
					}
					case FT_CATEGORY:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						m_category = newtag->tag.intvalue;
						delete newtag;
						break;
					}
					case FT_DLPRIORITY:{
						if (!isnewstyle){
							// SLUGFILLER: SafeHash - tag-type verification
							if (newtag->tag.type != 3) {
								taglist.Add(newtag);
								break;
							}
							// SLUGFILLER: SafeHash
							m_iDownPriority = newtag->tag.intvalue;
							if( m_iDownPriority == PR_AUTO ){
								m_iDownPriority = PR_HIGH;
								SetAutoDownPriority(true);
							}
							else{
								if (m_iDownPriority != PR_LOW && m_iDownPriority != PR_NORMAL && m_iDownPriority != PR_HIGH)
									m_iDownPriority = PR_NORMAL;
								SetAutoDownPriority(false);
							}
						}
						delete newtag;
						break;
					}
					case FT_STATUS:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						paused = newtag->tag.intvalue;
						stopped=paused;
						delete newtag;
						break;
					}
					case FT_ULPRIORITY:{
						if (!isnewstyle){
							// SLUGFILLER: SafeHash - tag-type verification
							if (newtag->tag.type != 3) {
								taglist.Add(newtag);
								break;
							}
							// SLUGFILLER: SafeHash
							int iUpPriority = newtag->tag.intvalue;
							if( iUpPriority == PR_AUTO ){
								SetUpPriority(PR_HIGH, false);
								SetAutoUpPriority(true);
							}
							else{
								if (iUpPriority != PR_VERYLOW && iUpPriority != PR_LOW && iUpPriority != PR_NORMAL && iUpPriority != PR_HIGH && iUpPriority != PR_VERYHIGH)
									iUpPriority = PR_NORMAL;
								SetUpPriority(iUpPriority, false);
								SetAutoUpPriority(false);
							}
						}
						delete newtag;
						break;
					}
					case FT_KADLASTPUBLISHSRC:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						SetLastPublishTimeKadSrc(newtag->tag.intvalue);
						delete newtag;
						break;
					}
					// xMule_MOD: showSharePermissions - load permissions
					case FT_PERMISSIONS: {
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						SetPermissions(newtag->tag.intvalue);
						delete newtag;
						break;
					}
					// xMule_MOD: showSharePermissions
					// khaos::categorymod+
					case FT_CATRESUMEORDER:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						m_catResumeOrder = newtag->tag.intvalue;
						delete newtag;
						break;
					}
					// khaos::categorymod-
					// khaos::compcorruptfix+
					case FT_COMPRESSIONBYTES:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						m_iGainDueToCompression = newtag->tag.intvalue;
						delete newtag;
						break;
					}
					case FT_CORRUPTIONBYTES:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						m_iLostDueToCorruption = newtag->tag.intvalue;
						delete newtag;
						break;
					}
					// khaos::compcorruptfix-
					// khaos::kmod+
					case FT_A4AFON:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						m_bForceAllA4AF = (newtag->tag.intvalue == 1) ? true : false;
						delete newtag;
						break;
					}
					case FT_A4AFOFF:{
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						m_bForceA4AFOff = (newtag->tag.intvalue == 1) ? true : false;
						delete newtag;
						break;
					}
					// khaos::kmod-
					// old tags: as long as they are not needed, take the chance to purge them
					case FT_KADLASTPUBLISHKEY:
						// SLUGFILLER: SafeHash - tag-type verification
						if (newtag->tag.type != 3) {
							taglist.Add(newtag);
							break;
						}
						// SLUGFILLER: SafeHash
						delete newtag;
						break;
					case FT_DL_ACTIVE_TIME:
						if (newtag->tag.type == 3)
							m_nDlActiveTime = newtag->tag.intvalue;
						delete newtag;
						break;
				    default:{
					    // Start Changes by Slugfiller for better exception handling
						if ((!newtag->tag.specialtag) &&
							 (newtag->tag.tagname[0] == FT_GAPSTART ||
							newtag->tag.tagname[0] == FT_GAPEND)){
							Gap_Struct* gap;
							uint16 gapkey = atoi(&newtag->tag.tagname[1]);
							if (!gap_map.Lookup(gapkey, gap))
							{
								gap = new Gap_Struct;
								gap_map.SetAt(gapkey, gap);
								gap->start = (uint32)-1;
								gap->end = (uint32)-1;
							}
							if (newtag->tag.tagname[0] == FT_GAPSTART)
								gap->start = newtag->tag.intvalue;
							if (newtag->tag.tagname[0] == FT_GAPEND)
								gap->end = newtag->tag.intvalue-1;
							delete newtag;
					    	// End Changes by Slugfiller for better exception handling
					    //MORPH START - Added by SiRoB, Avoid misusing of powersharing
						} else	if((!newtag->tag.specialtag) && strcmp(newtag->tag.tagname, FT_POWERSHARE) == 0) {
							SetPowerShared((newtag->tag.intvalue<=3)?newtag->tag.intvalue:-1);
							delete newtag;
						//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
						//MORPH START - Added by SiRoB, POWERSHARE Limit
						} else	if((!newtag->tag.specialtag) && strcmp(newtag->tag.tagname, FT_POWERSHARE_LIMIT) == 0) {
							SetPowerShareLimit((newtag->tag.intvalue<=200)?newtag->tag.intvalue:-1);
							delete newtag;
						//MORPH END   - Added by SiRoB, POWERSHARE Limit
						//MORPH START - Added by SiRoB, HIDEOS per file
						} else if((!newtag->tag.specialtag) && strcmp(newtag->tag.tagname, FT_HIDEOS) == 0) {
							SetHideOS((newtag->tag.intvalue<=10)?newtag->tag.intvalue:-1);
							delete newtag;
						} else if((!newtag->tag.specialtag) && strcmp(newtag->tag.tagname, FT_SELECTIVE_CHUNK) == 0) {
							SetSelectiveChunk(newtag->tag.intvalue<=1?newtag->tag.intvalue:-1);
							delete newtag;
						//MORPH END   - Added by SiRoB, HIDEOS per file
						//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
						} else	if((!newtag->tag.specialtag) && strcmp(newtag->tag.tagname, FT_SHAREONLYTHENEED) == 0) {
							SetShareOnlyTheNeed(newtag->tag.intvalue<=1?newtag->tag.intvalue:-1);
							delete newtag;
						//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
						}
						else
							taglist.Add(newtag);
					}
				}
			}
			else
				delete newtag;
		}

		// load the hashsets from the hybridstylepartmet
		if (isnewstyle && !getsizeonly && (metFile.GetPosition()<metFile.GetLength()) ) {
			uint8 temp;
			metFile.Read(&temp,1);
			
			uint16 parts=GetPartCount();	// assuming we will get all hashsets
			
			for (uint16 i = 0; i < parts && (metFile.GetPosition()+16<metFile.GetLength()); i++){
				uchar* cur_hash = new uchar[16];
				metFile.Read(cur_hash, 16);
				hashlist.Add(cur_hash);
			}

			uchar* checkhash= new uchar[16];
			if (!hashlist.IsEmpty()){
				uchar* buffer = new uchar[hashlist.GetCount()*16];
				for (int i = 0; i < hashlist.GetCount(); i++)
					md4cpy(buffer+(i*16), hashlist[i]);
				CreateHashFromString(buffer, hashlist.GetCount()*16, checkhash);
				delete[] buffer;
			}
			bool flag=false;
			if (!md4cmp(m_abyFileHash, checkhash))
				flag=true;
			else{
				for (int i = 0; i < hashlist.GetSize(); i++)
					delete[] hashlist[i];
				hashlist.RemoveAll();
				flag=false;
			}
			delete[] checkhash;
		}

		metFile.Close();
	}
	catch(CFileException* error){
		if (error->m_cause == CFileException::endOfFile)
			AddLogLine(true, GetResString(IDS_ERR_METCORRUPT), m_partmetfilename, GetFileName());
		else{
			char buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,MAX_CFEXP_ERRORMSG);
			AddLogLine(true, GetResString(IDS_ERR_FILEERROR), m_partmetfilename, GetFileName(), buffer);
		}
		error->Delete();
		return false;
	}
	catch(...){
		AddLogLine(true, GetResString(IDS_ERR_METCORRUPT), m_partmetfilename, GetFileName());
		ASSERT(0);
		return false;
	}

	if (getsizeonly) {
		return partmettype;
	}

	// Now to flush the map into the list (Slugfiller)
	for (POSITION pos = gap_map.GetStartPosition(); pos != NULL; ){
		Gap_Struct* gap;
		uint16 gapkey;
		gap_map.GetNextAssoc(pos, gapkey, gap);
		// SLUGFILLER: SafeHash - revised code, and extra safety
		if (gap->start != -1 && gap->end != -1 && gap->start <= gap->end && gap->start < m_nFileSize){
			if (gap->end >= m_nFileSize)
				gap->end = m_nFileSize-1; // Clipping
			AddGap(gap->start, gap->end); // All tags accounted for, use safe adding
		}
		delete gap;
		// SLUGFILLER: SafeHash
	}

	//check if this is a backup
	if(stricmp(strrchr(m_fullname, '.'), PARTMET_TMP_EXT) == 0)
		m_fullname = RemoveFileExtension(m_fullname);

	// open permanent handle
	CString searchpath(RemoveFileExtension(m_fullname));
	CFileException fexpPart;
	if (!m_hpartfile.Open(searchpath, CFile::modeReadWrite|CFile::shareDenyWrite|CFile::osSequentialScan, &fexpPart)){
		CString strError;
		strError.Format(GetResString(IDS_ERR_FILEOPEN), searchpath, GetFileName());
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexpPart.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("%s"), strError);
		return false;
	}

	// read part file creation time
	struct _stat fileinfo;
	if (_stat(searchpath, &fileinfo) == 0){
		m_tLastModified = fileinfo.st_mtime;
		m_tCreated = fileinfo.st_ctime;
	}
	else
		AddDebugLogLine(false, _T("Failed to get file date for \"%s\" - %hs"), searchpath, strerror(errno));

	try{
		SetFilePath(searchpath);
		m_dwFileAttributes = GetFileAttributes(GetFilePath());
		if (m_dwFileAttributes == INVALID_FILE_ATTRIBUTES)
			m_dwFileAttributes = 0;

		// SLUGFILLER: SafeHash - final safety, make sure any missing part of the file is gap
		if (m_hpartfile.GetLength() < m_nFileSize)
			AddGap(m_hpartfile.GetLength(), m_nFileSize-1);
		// Goes both ways - Partfile should never be too large
		if (m_hpartfile.GetLength() > m_nFileSize){
			TRACE("Partfile \"%s\" is too large! Truncating %I64u bytes.\n", GetFileName(), m_hpartfile.GetLength() - m_nFileSize);
			m_hpartfile.SetLength(m_nFileSize);
		}
		// SLUGFILLER: SafeHash

		// SLUGFILLER: SafeHash - ignore loaded hash for 1-chunk files
		if (GetED2KPartCount() <= 1) {
			for (int i = 0; i < hashlist.GetSize(); i++)
				delete[] hashlist[i];
			hashlist.RemoveAll();
			uchar* cur_hash = new uchar[16];
			md4cpy(cur_hash, m_abyFileHash);
			hashlist.Add(cur_hash);
		}

		// the important part
		m_PartsShareable.SetSize(GetPartCount());
		for (uint32 i = 0; i < GetPartCount();i++)
			m_PartsShareable[i] = false;
		// SLUGFILLER: SafeHash

		m_SrcpartFrequency.SetSize(GetPartCount());
		for (uint32 i = 0; i != GetPartCount();i++)
			m_SrcpartFrequency[i] = 0;
		SetStatus(PS_EMPTY);
		// check hashcount, filesatus etc
		if (GetHashCount() != GetED2KPartCount()){	// SLUGFILLER: SafeHash - use GetED2KPartCount
			ASSERT( hashlist.GetSize() == 0 );
			// SLUGFILLER: SafeHash - hashset load failed, delete the corrupt data
			for (int i = 0; i < hashlist.GetSize(); i++)
				delete[] hashlist[i];
			hashlist.RemoveAll();
			// SLUGFILLER: SafeHash
			hashsetneeded = true;
			return true;
		}
		else {
			hashsetneeded = false;
			for (int i = 0; i < hashlist.GetSize(); i++){
				if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
					SetStatus(PS_READY);
					break;
				}
			}
		}

		if (gaplist.IsEmpty()){	// is this file complete already?
			CompleteFile(false);
			return true;
		}

		if (!isnewstyle) // not for importing
		{
			// check date of .part file - if its wrong, rehash file
			CFileStatus filestatus;
			m_hpartfile.GetStatus(filestatus); // this; "...returns m_attribute without high-order flags" indicates a known MFC bug, wonder how many unknown there are... :)
			uint32 fdate = filestatus.m_mtime.GetTime();
			if (fdate == -1){
				if (thePrefs.GetVerbose())
				AddDebugLogLine(false, "Failed to convert file date of %s (%s)", filestatus.m_szFullName, GetFileName());
			}
			else
				AdjustNTFSDaylightFileTime(fdate, filestatus.m_szFullName);
			if (m_tUtcLastModified != fdate){
				CString strFileInfo;
				strFileInfo.Format(_T("%s (%s)"), GetFilePath(), GetFileName());
				AddLogLine(false, GetResString(IDS_ERR_REHASH), strFileInfo);
				// rehash
				// SLUGFILLER: SafeHash
				SetStatus(PS_EMPTY);	// no need to wait for hashes with the new system
				CPartHashThread* parthashthread = (CPartHashThread*) AfxBeginThread(RUNTIME_CLASS(CPartHashThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
				m_PartsHashing += parthashthread->SetFirstHash(this);	// Only hashes completed parts, why hash gaps?
				parthashthread->ResumeThread();
				// SLUGFILLER: SafeHash
			}
			// SLUGFILLER: SafeHash - update completed, even though unchecked
			else {
				for (int i = 0; i < GetPartCount(); i++)
					if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1))
						m_PartsShareable[i] = true;
			}
			// SLUGFILLER: SafeHash
		}
	}
	catch(CFileException* error){
		CString strError;
		strError.Format(_T("Failed to initialize part file \"%s\" (%s)"), m_hpartfile.GetFilePath(), GetFileName());
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (error->GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("%s"), strError);
		error->Delete();
		return false;
	}

	UpdateCompletedInfos();
	// khaos::compcorruptfix+
	if ( completedsize > transfered )
		m_iGainDueToCompression = completedsize - transfered;
	else if ( completedsize != transfered )
		m_iLostDueToCorruption = transfered - completedsize;
	// khaos::compcorruptfix-
	// khaos::accuratetimerem+
	m_nInitialBytes = completedsize;
	// khaos::accuratetimerem-
	return true;
}

bool CPartFile::SavePartFile()
{
	switch (status){
		case PS_WAITINGFORHASH:
		case PS_HASHING:
			return false;
	}

	// search part file
	CFileFind ff;
	CString searchpath(RemoveFileExtension(m_fullname));
	bool end = !ff.FindFile(searchpath,0);
	if (!end)
		ff.FindNextFile();
	if (end || ff.IsDirectory()){
		AddLogLine(false, GetResString(IDS_ERR_SAVEMET) + _T(" - %s"), m_partmetfilename, GetFileName(), GetResString(IDS_ERR_PART_FNF));
		return false;
	}

	if (!m_PartsHashing){	// SLUGFILLER: SafeHash - don't update the file date unless all parts are hashed
		//get filedate
		CTime lwtime;
		if (!ff.GetLastWriteTime(lwtime)){
			if (thePrefs.GetVerbose())
				AddDebugLogLine(false, "Failed to get file date of %s (%s) - %s", m_partmetfilename, GetFileName(), GetErrorMessage(GetLastError()));
		}
		m_tLastModified = lwtime.GetTime();
		m_tUtcLastModified = m_tLastModified;
		if (m_tUtcLastModified == -1){
			if (thePrefs.GetVerbose())
				AddDebugLogLine(false, "Failed to convert file date of %s (%s)", m_partmetfilename, GetFileName());
		}
		else
			AdjustNTFSDaylightFileTime(m_tUtcLastModified, ff.GetFilePath());
	}	// SLUGFILLER: SafeHash
	ff.Close();

	CString strTmpFile(m_fullname);
	strTmpFile += PARTMET_TMP_EXT;

	// save file data to part.met file
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(strTmpFile, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary, &fexp)){
		CString strError;
		strError.Format(GetResString(IDS_ERR_SAVEMET), m_partmetfilename, GetFileName());
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("%s"), strError);
		return false;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

	try{
		//version
		file.WriteUInt8(PARTFILE_VERSION);
		//date
		file.WriteUInt32(m_tUtcLastModified);
		//hash
		file.WriteHash16(m_abyFileHash);
		UINT parts = hashlist.GetCount();
		file.WriteUInt16(parts);
		for (UINT x = 0; x < parts; x++)
			file.WriteHash16(hashlist[x]);
		//tags
		uint32 tagcount = 10/*Official*/ +5/*Khaos*/ +(gaplist.GetCount()*2);
		// Float meta tags are currently not written. All older eMule versions < 0.28b have 
		// a bug in the meta tag reading+writing code. To achive maximum backward 
		// compatibility for met files with older eMule versions we just don't write float 
		// tags. This is OK, because we (eMule) do not use float tags. The only float tags 
		// we may have to handle is the '# Sent' tag from the Hybrid, which is pretty 
		// useless but may be received from us via the servers.
		// 
		// The code for writing the float tags SHOULD BE ENABLED in SOME MONTHS (after most 
		// people are using the newer eMule versions which do not write broken float tags).	
		for (int j = 0; j < taglist.GetCount(); j++){
			if (taglist[j]->tag.type == 2 || taglist[j]->tag.type == 3)
				tagcount++;
		}
		
		if (GetPermissions()>=0) tagcount ++;		//MORPH - Added by SiRoB, Show Permissions
		if (GetHideOS()>=0) tagcount ++;			//MORPH - Added by SiRoB, HIDEOS
		if (GetSelectiveChunk()>=0) tagcount ++;	//MORPH - Added by SiRoB, HIDEOS
		if (GetShareOnlyTheNeed()>=0) tagcount ++;	//MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
		if (GetPowerSharedMode()>=0) tagcount ++;	//MORPH - Added by SiRoB, Avoid misusing of powersharing
		if (GetPowerShareLimit()>=0) tagcount ++;	//MORPH - Added by SiRoB, POWERSHARE Limit

		file.WriteUInt32(tagcount);

		CTag nametag(FT_FILENAME, GetFileName());
		nametag.WriteTagToFile(&file);

		CTag sizetag(FT_FILESIZE, m_nFileSize);
		sizetag.WriteTagToFile(&file);

		CTag transtag(FT_TRANSFERED, transfered);
		transtag.WriteTagToFile(&file);

		CTag statustag(FT_STATUS, (paused)? 1: 0);
		statustag.WriteTagToFile(&file);

		CTag prioritytag(FT_DLPRIORITY, IsAutoDownPriority() ? PR_AUTO : m_iDownPriority);
		prioritytag.WriteTagToFile(&file);

		CTag lsctag(FT_LASTSEENCOMPLETE,lastseencomplete.GetTime());
		lsctag.WriteTagToFile(&file);

		CTag ulprioritytag(FT_ULPRIORITY, IsAutoUpPriority() ? PR_AUTO : GetUpPriority());
		ulprioritytag.WriteTagToFile(&file);

		//MORPH - Changed by SiRoB
		//CTag categorytag(FT_CATEGORY, m_category);
		CTag categorytag(FT_CATEGORY, (m_category > thePrefs.GetCatCount() - 1)?0:m_category);
		categorytag.WriteTagToFile(&file);

		CTag kadLastPubSrc(FT_KADLASTPUBLISHSRC, GetLastPublishTimeKadSrc());
		kadLastPubSrc.WriteTagToFile(&file);

		CTag tagDlActiveTime(FT_DL_ACTIVE_TIME, GetDlActiveTime());
		tagDlActiveTime.WriteTagToFile(&file);

		//MORPH START - Added by SiRoB, Show Permissions
		// xMule_MOD: showSharePermissions - save permissions
		if (GetPermissions()>=0){
			CTag permtag(FT_PERMISSIONS, GetPermissions());
			permtag.WriteTagToFile(&file);
		}
		//MORPH START - Added by SiRoB, Show Permissions
		//MORPH START - Added by SiRoB, HIDEOS
		if (GetHideOS()>=0){
			CTag hideostag(FT_HIDEOS, GetHideOS());
			hideostag.WriteTagToFile(&file);
		}
		if (GetSelectiveChunk()>=0){
			CTag selectivechunktag(FT_SELECTIVE_CHUNK, GetSelectiveChunk());
			selectivechunktag.WriteTagToFile(&file);
		}
		//MORPH END   - Added by SiRoB, HIDEOS

		//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
		if (GetShareOnlyTheNeed()>=0){
			CTag shareonlytheneedtag(FT_SHAREONLYTHENEED, GetShareOnlyTheNeed());
			shareonlytheneedtag.WriteTagToFile(&file);
		}
		//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

		//MORPH START - Added by SiRoB, Avoid misusing of powersharing
		if (GetPowerSharedMode()>=0){
			CTag powersharetag(FT_POWERSHARE, GetPowerSharedMode());
			powersharetag.WriteTagToFile(&file);
		}
		//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
		//MORPH START - Added by SiRoB, POWERSHARE Limit
		if (GetPowerShareLimit()>=0){
			CTag powersharelimittag(FT_POWERSHARE_LIMIT, GetPowerShareLimit());
			powersharelimittag.WriteTagToFile(&file);
		}
		//MORPH END   - Added by SiRoB, POWERSHARE Limit
		// khaos::categorymod+
		CTag catresumetag(FT_CATRESUMEORDER, m_catResumeOrder );
		catresumetag.WriteTagToFile(&file);

		// khaos::compcorruptfix+
		CTag savedtag(FT_COMPRESSIONBYTES, (uint32)m_iGainDueToCompression + m_iSesCompressionBytes);
		savedtag.WriteTagToFile(&file);

		CTag losttag(FT_CORRUPTIONBYTES, (uint32)m_iLostDueToCorruption + m_iSesCorruptionBytes);
		losttag.WriteTagToFile(&file);
		// khaos::compcorruptfix-

		// khaos::kmod+ A4AF flags
		CTag forceon(FT_A4AFON, (uint32)(m_bForceAllA4AF?1:0));
		forceon.WriteTagToFile(&file);

		CTag forceoff(FT_A4AFOFF, (uint32)(m_bForceA4AFOff?1:0));
		forceoff.WriteTagToFile(&file);
		// khaos::kmod-

		for (int j = 0; j < taglist.GetCount(); j++){
			if (taglist[j]->tag.type == 2 || taglist[j]->tag.type == 3)
				taglist[j]->WriteTagToFile(&file);
		}

		//gaps
		char namebuffer[10];
		char* number = &namebuffer[1];
		uint16 i_pos = 0;
		for (POSITION pos = gaplist.GetHeadPosition();pos != 0;){
			Gap_Struct* gap = gaplist.GetNext(pos);
			itoa(i_pos,number,10);
			namebuffer[0] = FT_GAPSTART;
			CTag gapstarttag(namebuffer,gap->start);
			gapstarttag.WriteTagToFile(&file);
			// gap start = first missing byte but gap ends = first non-missing byte in edonkey
			// but I think its easier to user the real limits
			namebuffer[0] = FT_GAPEND;
			CTag gapendtag(namebuffer,gap->end+1);
			gapendtag.WriteTagToFile(&file);
			i_pos++;
		}

		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			file.Flush(); // flush file stream buffers to disk buffers
			if (_commit(_fileno(file.m_pStream)) != 0) // commit disk buffers to disk
				AfxThrowFileException(CFileException::hardIO, GetLastError(), file.GetFileName());
		}
		file.Close();
	}
	catch(CFileException* error){
		CString strError;
		strError.Format(GetResString(IDS_ERR_SAVEMET), m_partmetfilename, GetFileName());
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (error->GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("%s"), strError);
		error->Delete();

		// remove the partially written or otherwise damaged temporary file
		file.Abort(); // need to close the file before removing it. call 'Abort' instead of 'Close', just to avoid an ASSERT.
		(void)_tremove(strTmpFile);
		return false;
	}

	// after successfully writing the temporary part.met file...
	if (_tremove(m_fullname) != 0 && errno != ENOENT){
		if (thePrefs.GetVerbose())
		AddDebugLogLine(false, _T("Failed to remove \"%s\" - %s"), m_fullname, strerror(errno));
	}

	if (_trename(strTmpFile, m_fullname) != 0){
		int iErrno = errno;
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, _T("Failed to move temporary part.met file \"%s\" to \"%s\" - %s"), strTmpFile, m_fullname, strerror(iErrno));

		CString strError;
		strError.Format(GetResString(IDS_ERR_SAVEMET), m_partmetfilename, GetFileName());
		strError += _T(" - ");
		strError += strerror(iErrno);
		AddLogLine(false, _T("%s"), strError);
		return false;
	}

	// create a backup of the successfully written part.met file
	CString BAKName(m_fullname);
	BAKName.Append(PARTMET_BAK_EXT);
	if (!::CopyFile(m_fullname, BAKName, FALSE)){
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, "Failed to create backup of %s (%s) - %s", m_fullname, GetFileName(), GetErrorMessage(GetLastError()));
	}

	return true;
}

void CPartFile::PartFileHashFinished(CKnownFile* result){
	newdate = true;
	bool errorfound = false;
	// SLUGFILLER: SafeHash - one check for all
	for (uint32 i = 0; i < (uint32)hashlist.GetSize(); i++){
		if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
			if (!(result->GetPartHash(i) && !md4cmp(result->GetPartHash(i),this->GetPartHash(i)))){
				AddLogLine(false, GetResString(IDS_ERR_FOUNDCORRUPTION), i+1, GetFileName());
				AddGap(i*PARTSIZE,((((i+1)*PARTSIZE)-1) >= m_nFileSize) ? m_nFileSize-1 : ((i+1)*PARTSIZE)-1);
				errorfound = true;
			}
		}
	}
	// SLUGFILLER: SafeHash
	delete result;
	if (!errorfound){
		if (status == PS_COMPLETING){
			if (thePrefs.GetVerbose())
				AddDebugLogLine(true,"Completing file-hashing for \"%s\"", GetFileName());
			CompleteFile(true);
			return;
		}
		else
			AddLogLine(false, GetResString(IDS_HASHINGDONE), GetFileName());
	}
	else{
		SetStatus(PS_READY);
		if (thePrefs.GetVerbose())
			AddDebugLogLine(true,"File-hashing failed for \"%s\"", GetFileName());
		SavePartFile();
		return;
	}
	if (thePrefs.GetVerbose())
		AddDebugLogLine(true,"Completing file-hashing of \"%s\"", GetFileName());
	SetStatus(PS_READY);
	SavePartFile();
	theApp.sharedfiles->SafeAddKFile(this);
}

void CPartFile::AddGap(uint32 start, uint32 end){
	POSITION pos1, pos2;
	for (pos1 = gaplist.GetHeadPosition();(pos2 = pos1) != NULL;){
		Gap_Struct* cur_gap = gaplist.GetNext(pos1);
		if (cur_gap->start >= start && cur_gap->end <= end){ // this gap is inside the new gap - delete
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		}
		else if (cur_gap->start >= start && cur_gap->start <= end){// a part of this gap is in the new gap - extend limit and delete
			end = cur_gap->end;
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		}
		else if (cur_gap->end <= end && cur_gap->end >= start){// a part of this gap is in the new gap - extend limit and delete
			start = cur_gap->start;
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		}
		else if (start >= cur_gap->start && end <= cur_gap->end){// new gap is already inside this gap - return
			return;
		}
	}
	Gap_Struct* new_gap = new Gap_Struct;
	new_gap->start = start;
	new_gap->end = end;
	gaplist.AddTail(new_gap);
	//theApp.emuledlg->transferwnd->downloadlistctrl.UpdateItem(this);
	UpdateDisplayedInfo();
	newdate = true;
}

bool CPartFile::IsComplete(uint32 start, uint32 end) const
{
	if (end >= m_nFileSize)
		end = m_nFileSize-1;
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;){
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		if ((cur_gap->start >= start && cur_gap->end <= end)||(cur_gap->start >= start 
			&& cur_gap->start <= end)||(cur_gap->end <= end && cur_gap->end >= start)
			||(start >= cur_gap->start && end <= cur_gap->end))
		{
			return false;	
		}
	}
	return true;
}

bool CPartFile::IsPureGap(uint32 start, uint32 end) const
{
	if (end >= m_nFileSize)
		end = m_nFileSize-1;
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;){
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		if (start >= cur_gap->start  && end <= cur_gap->end ){
			return true;
		}
	}
	return false;
}

bool CPartFile::IsAlreadyRequested(uint32 start, uint32 end) const
{
	for (POSITION pos =  requestedblocks_list.GetHeadPosition();pos != 0; ){
		Requested_Block_Struct* cur_block =  requestedblocks_list.GetNext(pos);
		if ((start <= cur_block->EndOffset) && (end >= cur_block->StartOffset))
			return true;
	}
	return false;
}

bool CPartFile::GetNextEmptyBlockInPart(uint16 partNumber, Requested_Block_Struct *result) const
{
	Gap_Struct *firstGap;
	Gap_Struct *currentGap;
	uint32 end;
	uint32 blockLimit;

	// Find start of this part
	uint32 partStart = (PARTSIZE * partNumber);
	uint32 start = partStart;

	// What is the end limit of this block, i.e. can't go outside part (or filesize)
	uint32 partEnd = (PARTSIZE * (partNumber + 1)) - 1;
	if (partEnd >= GetFileSize())
		partEnd = GetFileSize() - 1;

	// Loop until find a suitable gap and return true, or no more gaps and return false
	for (;;)
	{
		firstGap = NULL;

		// Find the first gap from the start position
		for (POSITION pos = gaplist.GetHeadPosition(); pos != 0; )
		{
			currentGap = gaplist.GetNext(pos);
			// Want gaps that overlap start<->partEnd
			if ((currentGap->start <= partEnd) && (currentGap->end >= start))
			{
				// Is this the first gap?
				if ((firstGap == NULL) || (currentGap->start < firstGap->start))
					firstGap = currentGap;
			}
		}

		// If no gaps after start, exit
		if (firstGap == NULL)
			return false;

		// Update start position if gap starts after current pos
		if (start < firstGap->start)
			start = firstGap->start;

		// If this is not within part, exit
		if (start > partEnd)
			return false;

		// Find end, keeping within the max block size and the part limit
		end = firstGap->end;
		blockLimit = partStart + (EMBLOCKSIZE * (((start - partStart) / EMBLOCKSIZE) + 1)) - 1;
		if (end > blockLimit)
			end = blockLimit;
		if (end > partEnd)
			end = partEnd;
	
		// If this gap has not already been requested, we have found a valid entry
		if (!IsAlreadyRequested(start, end))
		{
			// Was this block to be returned
			if (result != NULL)
			{
				result->StartOffset = start;
				result->EndOffset = end;
				md4cpy(result->FileID, GetFileHash());
				result->transferred = 0;
			}
			return true;
		}
		else
		{
			// Reposition to end of that gap
			start = end + 1;
		}

		// If tried all gaps then break out of the loop
		if (end == partEnd)
			break;
	}

	// No suitable gap found
	return false;
}

void CPartFile::FillGap(uint32 start, uint32 end){
	POSITION pos1, pos2;
	for (pos1 = gaplist.GetHeadPosition();(pos2 = pos1) != NULL;){
		Gap_Struct* cur_gap = gaplist.GetNext(pos1);
		if (cur_gap->start >= start && cur_gap->end <= end){ // our part fills this gap completly
			gaplist.RemoveAt(pos2);
			delete cur_gap;
			continue;
		}
		else if (cur_gap->start >= start && cur_gap->start <= end){// a part of this gap is in the part - set limit
			cur_gap->start = end+1;
		}
		else if (cur_gap->end <= end && cur_gap->end >= start){// a part of this gap is in the part - set limit
			cur_gap->end = start-1;
		}
		else if (start >= cur_gap->start && end <= cur_gap->end){
			uint32 buffer = cur_gap->end;
			cur_gap->end = start-1;
			cur_gap = new Gap_Struct;
			cur_gap->start = end+1;
			cur_gap->end = buffer;
			gaplist.InsertAfter(pos1,cur_gap);
			break; // [Lord KiRon]
		}
	}

	UpdateCompletedInfos();
	//theApp.emuledlg->transferwnd->downloadlistctrl.UpdateItem(this);
	UpdateDisplayedInfo();
	newdate = true;
	// Barry - The met file is now updated in FlushBuffer()
	//SavePartFile();
}

void CPartFile::UpdateCompletedInfos()
{
   	uint32 allgaps = 0; 

	for (POSITION pos = gaplist.GetHeadPosition();pos !=  0;){ 
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		allgaps += cur_gap->end - cur_gap->start;
	}

	if (gaplist.GetCount() || requestedblocks_list.GetCount()){ 
		percentcompleted = (1.0f-(float)(allgaps+1)/m_nFileSize) * 100; 
		completedsize = m_nFileSize - allgaps - 1; 
	} 
	else{
		percentcompleted = 100;
		completedsize = m_nFileSize;
	}
}

//MORPH START - Modified by SiRoB, Reduce ShareStatusBar CPU consumption
void CPartFile::DrawShareStatusBar(CDC* dc, LPCRECT rect, bool onlygreyrect, bool bFlat) /*const*/
{ 
	if( !IsPartFile() )
	{
		CKnownFile::DrawShareStatusBar( dc, rect, onlygreyrect, bFlat );
		return;
	}
	int iWidth=rect->right - rect->left;
	if (iWidth <= 0)	return;
	int iHeight=rect->bottom - rect->top;
	if (m_bitmapSharedStatusBar == (HBITMAP)NULL)
		VERIFY(m_bitmapSharedStatusBar.CreateBitmap(1, 1, 1, 8, NULL)); 
	CDC cdcStatus;
	HGDIOBJ hOldBitmap;
	cdcStatus.CreateCompatibleDC(dc);
	if(!InChangedSharedStatusBar || lastSize!=iWidth || lastonlygreyrect!=onlygreyrect || lastbFlat!=bFlat){
		InChangedSharedStatusBar = true;
		lastSize=iWidth;
		lastonlygreyrect=onlygreyrect;
		lastbFlat=bFlat;
		m_bitmapSharedStatusBar.DeleteObject(); 
		m_bitmapSharedStatusBar.CreateCompatibleBitmap(dc,  iWidth, iHeight); 
		m_bitmapSharedStatusBar.SetBitmapDimension(iWidth,  iHeight); 
		hOldBitmap = cdcStatus.SelectObject(m_bitmapSharedStatusBar);
		
		COLORREF crProgress;
		COLORREF crHave;
		COLORREF crPending;
		COLORREF crMissing = RGB(255, 0, 0);

		if(bFlat) { 
			crProgress = RGB(0, 150, 0);
			crHave = RGB(0, 0, 0);
			crPending = RGB(255,208,0);
		} else { 
			crProgress = RGB(0, 224, 0);
			crHave = RGB(104, 104, 104);
			crPending = RGB(255, 208, 0);
		} 

		s_ChunkBar.SetFileSize(this->GetFileSize()); 
		s_ChunkBar.SetHeight(iHeight); 
		s_ChunkBar.SetWidth(iWidth); 
		s_ChunkBar.Fill(crMissing); 
		COLORREF color;
		if (!onlygreyrect && !m_SrcpartFrequency.IsEmpty()) { 
			for (int i = 0;i < GetPartCount();i++)
				if(m_SrcpartFrequency[i] > 0 ){
					color = RGB(0, (210-(22*(m_SrcpartFrequency[i]-1)) <  0)? 0:210-(22*(m_SrcpartFrequency[i]-1)), 255);
					s_ChunkBar.FillRange(PARTSIZE*(i),PARTSIZE*(i+1),color);
				}
		}
   		s_ChunkBar.Draw(&cdcStatus, 0, 0, bFlat);
	}
	else
		hOldBitmap = cdcStatus.SelectObject(m_bitmapSharedStatusBar);
	dc->BitBlt(rect->left,rect->top,iWidth,iHeight,&cdcStatus,0,0,SRCCOPY);
	cdcStatus.SelectObject(hOldBitmap);
} 

void CPartFile::DrawStatusBar(CDC* dc, LPCRECT rect, bool bFlat) /*const*/
{
	COLORREF crProgress;
	COLORREF crHave;
	COLORREF crPending;
	COLORREF crMissing;		// SLUGFILLER: grayPause - moved down
	//--- xrmb:confirmedDownload ---
	COLORREF crUnconfirmed = RGB(255, 210, 0);
	//--- :xrmb ---
	COLORREF crDot = RGB(255, 255, 255);	//MORPH - Added by IceCream, SLUGFILLER: chunkDots
	bool notgray = GetStatus() == PS_EMPTY || GetStatus() == PS_READY; // SLUGFILLER: grayPause - only test once

	// SLUGFILLER: grayPause - Colors by status
	if(bFlat)
		crProgress = RGB(0, 150, 0);
	else
		crProgress = RGB(0, 224, 0);
	if(notgray) {
		crMissing = RGB(255, 0, 0);
		if(bFlat) {
			crHave = RGB(0, 0, 0);
			crPending = RGB(255,208,0);
		} else {
			crHave = RGB(104, 104, 104);
			crPending = RGB(255, 208, 0);
		}
	} else {
		crMissing = RGB(191, 64, 64);
		if(bFlat) {
			crHave = RGB(64, 64, 64);
			crPending = RGB(191,168,64);
		} else {
			crHave = RGB(116, 116, 116);
			crPending = RGB(191, 168, 64);
		}
	}
	// SLUGFILLER: grayPause

	s_ChunkBar.SetHeight(rect->bottom - rect->top);
	s_ChunkBar.SetWidth(rect->right - rect->left);
	s_ChunkBar.SetFileSize(m_nFileSize);
	s_ChunkBar.Fill(crHave);

	if(status == PS_COMPLETE || status == PS_COMPLETING) {
		s_ChunkBar.FillRange(0, m_nFileSize, crProgress);
		s_ChunkBar.Draw(dc, rect->left, rect->top, bFlat);
		percentcompleted = 100;
		completedsize = m_nFileSize;
		return;
	}

	// red gaps
	uint32 allgaps = 0;
	for (POSITION pos = gaplist.GetHeadPosition();pos !=  0;){
		const Gap_Struct* cur_gap = gaplist.GetNext(pos);
		allgaps += cur_gap->end - cur_gap->start;
		bool gapdone = false;
		uint32 gapstart = cur_gap->start;
		uint32 gapend = cur_gap->end;
		for (uint32 i = 0; i < GetPartCount(); i++){
			if (gapstart >= i*PARTSIZE && gapstart <=  (i+1)*PARTSIZE){ // is in this part?
				if (gapend <= (i+1)*PARTSIZE)
					gapdone = true;
				else
					gapend = (i+1)*PARTSIZE; // and next part

				// paint
				COLORREF color;
				if (m_SrcpartFrequency.GetCount() >= (INT_PTR)i && m_SrcpartFrequency[(uint16)i])  // frequency?
				// SLUGFILLER: grayPause
				{
					if(notgray)
						color = RGB(0,
									(210-(22*(m_SrcpartFrequency[(uint16)i]-1)) <  0)? 0:210-(22*(m_SrcpartFrequency[(uint16)i]-1))
									,255);
					else
						color = RGB(64,
									(169-(11*(m_SrcpartFrequency[(uint16)i]-1)) <  64)? 64:169-(11*(m_SrcpartFrequency[(uint16)i]-1))
									,191);
				}
				// SLUGFILLER: grayPause
				else
					color = crMissing;

				s_ChunkBar.FillRange(gapstart, gapend + 1,  color);

				if (gapdone) // finished?
					break;
				else{
					gapstart = gapend;
					gapend = cur_gap->end;
				}
			}
		}
	}

	// yellow pending parts
	for (POSITION pos = requestedblocks_list.GetHeadPosition();pos !=  0;){
		const Requested_Block_Struct* block =  requestedblocks_list.GetNext(pos);
		s_ChunkBar.FillRange(block->StartOffset + block->transferred, block->EndOffset,  crPending);
	}

	s_ChunkBar.Draw(dc, rect->left, rect->top, bFlat);

	// green progress
	//MORPH START - Added by IceCream --- xrmb:confirmedDownload ---
	RECT gaprect;
	gaprect.top = rect->top;
	gaprect.bottom = gaprect.top + PROGRESS_HEIGHT; // Barry - was 4
	gaprect.left = rect->left;

	float	percentconfirmed;
	uint32  confirmedsize;
	if((gaplist.GetCount() || requestedblocks_list.GetCount()))
	{
		// all this here should be done in the Process, not in the drawing!!! its a waste of cpu time, or maybe not :)
		uint16	completedParts=0;
		confirmedsize=0;
		for(uint32 i=0; i<GetPartCount(); i++)
		{
			uint32	end=(i+1)*PARTSIZE-1;
			bool	lastChunk=false;
			//--- last part? ---
			if(end>m_nFileSize)
			{
				end=m_nFileSize;
				lastChunk=true;
			}

			if(IsComplete(i*PARTSIZE, end))
			{
				completedParts++;

				if(lastChunk==false)
					confirmedsize+=PARTSIZE;
				else
					confirmedsize+=m_nFileSize % PARTSIZE;
			}
		}
		
		completedsize = m_nFileSize - allgaps - 1;

		percentcompleted = (float)completedsize/m_nFileSize*100;
		percentconfirmed = (float)confirmedsize/m_nFileSize*100;
	}
	else 
	{
		percentcompleted = 100;
		percentconfirmed = 100;
		completedsize = m_nFileSize;
		confirmedsize = m_nFileSize;
	}

	uint32	w=rect->right-rect->left+1;
	uint32	wc=(uint32)(percentconfirmed/100*w+0.5f);
	uint32	wp=(uint32)(percentcompleted/100*w+0.5f);

	if(!bFlat) {
		// SLUGFILLER: chunkDots
		s_LoadBar.SetWidth(1);
		s_LoadBar.SetFileSize(1);
		s_LoadBar.Fill(crDot);
		for(uint32 i=completedsize+PARTSIZE-(completedsize % PARTSIZE); i<m_nFileSize; i+=PARTSIZE)
			s_LoadBar.Draw(dc, gaprect.left+(uint32)((float)i*w/m_nFileSize), gaprect.top, false);
		// SLUGFILLER: chunkDots
		s_LoadBar.SetWidth(wp);
		s_LoadBar.SetFileSize(completedsize);
		s_LoadBar.Fill(crUnconfirmed);
		s_LoadBar.FillRange(0, confirmedsize, crProgress);
		s_LoadBar.Draw(dc, gaprect.left, gaprect.top, false);
	} else {
		gaprect.right = rect->left+wc;
		dc->FillRect(&gaprect, &CBrush(crProgress));
		gaprect.left = gaprect.right;
		gaprect.right = rect->left+wp;
		dc->FillRect(&gaprect, &CBrush(crUnconfirmed));
		//draw gray progress only if flat
		gaprect.left = gaprect.right;
		gaprect.right = rect->right;
		dc->FillRect(&gaprect, &CBrush(RGB(224,224,224)));
		// SLUGFILLER: chunkDots
		for(uint32 i=completedsize+PARTSIZE-(completedsize % PARTSIZE); i<m_nFileSize; i+=PARTSIZE){
			gaprect.left = gaprect.right = rect->left+(uint32)((float)i*w/m_nFileSize);
			gaprect.right++;
			dc->FillRect(&gaprect, &CBrush(RGB(128,128,128)));
		}
		// SLUGFILLER: chunkDots
	}
	//MORPH END   - Added by IceCream--- :xrmb ---
}

// SLUGFILLER: hideOS
void CPartFile::WritePartStatus(CSafeMemFile* file, CUpDownClient* client) /*const*/
{
	// SLUGFILLER: hideOS
	CArray<uint32, uint32> partspread;
	UINT parts;
	uint8 hideOS = GetHideOS()>=0?GetHideOS():thePrefs.GetHideOvershares();
	if (hideOS && client) {
		parts = CalcPartSpread(partspread, client);
	} else {	// simpler to set as 0 than to create another loop...
		parts = GetED2KPartCount();
		partspread.SetSize(parts);
		for (uint16 i = 0; i < parts; i++)
			partspread[i] = 0;
		hideOS = 1;
	}
	// SLUGFILLER: hideOS
	file->WriteUInt16(parts);
	UINT done = 0;
	while (done != parts){
		uint8 towrite = 0;
		for (UINT i = 0; i < 8; i++){
			if (partspread[done] < hideOS)	// SLUGFILLER: hideOS
				if (IsPartShareable(done))	// SLUGFILLER: SafeHash
					towrite |= (1<<i);
			done++;
			if (done == parts)
				break;
		}
		file->WriteUInt8(towrite);
	}
}

void CPartFile::WriteCompleteSourcesCount(CSafeMemFile* file) const
{
	file->WriteUInt16(m_nCompleteSourcesCount);
}

// -khaos--+++> These values are now cached.
/*
int CPartFile::GetValidSourcesCount() const
{
	int counter=0;
	for (POSITION pos = srclist.GetHeadPosition(); pos != NULL;){
		EDownloadState nDLState = srclist.GetNext(pos)->GetDownloadState();
		if (nDLState==DS_ONQUEUE || nDLState==DS_DOWNLOADING || nDLState==DS_CONNECTED || nDLState==DS_REMOTEQUEUEFULL)
			++counter;
	}
	return counter;
}

uint16 CPartFile::GetNotCurrentSourcesCount() const
{
	UINT counter = 0;
	for (POSITION pos = srclist.GetHeadPosition(); pos != NULL;){
		EDownloadState nDLState = srclist.GetNext(pos)->GetDownloadState();
		if (nDLState!=DS_ONQUEUE && nDLState!=DS_DOWNLOADING)
			counter++;
	}
	return counter;
}
*/
// <-----khaos-

//MORPH START - Added by IceCream, SLUGFILLER: checkDiskspace
uint32 CPartFile::GetNeededSpace() const
{
	if (m_hpartfile.GetLength() > GetFileSize())
		return 0;	// Shouldn't happen, but just in case
	return GetFileSize()-m_hpartfile.GetLength();
}
//MORPH END   - Added by IceCream, SLUGFILLER: checkDiskspace

uint8 CPartFile::GetStatus(bool ignorepause) const
{
	// SLUGFILLER: checkDiskspace
	if ((!paused && !insufficient) || status == PS_ERROR || status == PS_COMPLETING || status == PS_COMPLETE || ignorepause)
		return status;
	else if (paused)
		return PS_PAUSED;
	else
		return PS_INSUFFICIENT;
	// SLUGFILLER: checkDiskspace
}
void CPartFile::AddDownloadingSource(CUpDownClient* client){
	POSITION pos = m_downloadingSourceList.Find(client); // to be sure
	if(pos == NULL){
		m_downloadingSourceList.AddTail(client);
	}
}

void CPartFile::RemoveDownloadingSource(CUpDownClient* client){
	POSITION pos = m_downloadingSourceList.Find(client); // to be sure
	if(pos != NULL){
		m_downloadingSourceList.RemoveAt(pos);
	}
}

uint32 CPartFile::Process(uint32 reducedownload, uint8 m_icounter/*in percent*/, uint32 friendReduceddownload) //MORPH - Added by Yun.SF3, ZZ Upload System
{
	if (thePrefs.m_iDbgHeap >= 2)
		ASSERT_VALID(this);

	uint16	nOldTransSourceCount  = GetSrcStatisticsValue(DS_DOWNLOADING) ; 
	DWORD dwCurTick = ::GetTickCount();
	CUpDownClient* cur_src;

	// If buffer size exceeds limit, or if not written within time limit, flush data
	if ((m_nTotalBufferData > thePrefs.GetFileBufferSize()) || (dwCurTick > (m_nLastBufferFlushTime + BUFFER_TIME_LIMIT)))
	{
		// Avoid flushing while copying preview file
		if (!m_bPreviewing)
			FlushBuffer();
	}

	datarate = 0;

	// calculate datarate, set limit etc.
	if(m_icounter < 10)
	{
		uint32 cur_datarate;
		for(POSITION pos = m_downloadingSourceList.GetHeadPosition();pos!=0;)
		{
			cur_src = m_downloadingSourceList.GetNext(pos);
			if (thePrefs.m_iDbgHeap >= 2)
				ASSERT_VALID( cur_src );
			if(cur_src && cur_src->GetDownloadState() == DS_DOWNLOADING)
			{
				ASSERT( cur_src->socket );
				if (cur_src->socket)
				{
					cur_datarate = cur_src->CalculateDownloadRate();
					datarate+=cur_datarate;
					//MORPH - Added by Yun.SF3, ZZ Upload System
					uint32 curClientReducedDownload = reducedownload;
					if(cur_src->IsFriend() && cur_src->GetFriendSlot())
						curClientReducedDownload = friendReduceddownload;
					if(curClientReducedDownload)
					{
						uint32 limit = curClientReducedDownload*cur_datarate/1000;
						if(limit<1000 && curClientReducedDownload == 200)
						//MORPH - Added by Yun.SF3, ZZ Upload System
							limit +=1000;
						else if(limit<1)
							limit = 1;
						cur_src->socket->SetDownloadLimit(limit);
					}
				}
			}
		}
	}
	else
	{
		// -khaos--+++> Moved this here, otherwise we were setting our permanent variables to 0 every tenth of a second...
		memset(m_anStates,0,sizeof(m_anStates));
		memset(src_stats,0,sizeof(src_stats));
		uint16 nCountForState;

		//MORPH START - Added by SiRoB, Spread Reask For Better SUC functioning and more
		uint16 AvailableSrcCount = this->GetAvailableSrcCount()+1;
		uint16 srcPosReask = 0;
		//MORPH END   - Added by SiRoB, Spread Reask For Better SUC functioning and more
		
		for (POSITION pos = srclist.GetHeadPosition(); pos != NULL;)
		{
			cur_src = srclist.GetNext(pos);
			if (thePrefs.m_iDbgHeap >= 2)
				ASSERT_VALID( cur_src );

			// BEGIN -rewritten- refreshing statistics (no need for temp vars since it is not multithreaded)
			nCountForState = cur_src->GetDownloadState();
			//special case which is not yet set as downloadstate
			if (nCountForState == DS_ONQUEUE)
			{
				if( cur_src->IsRemoteQueueFull() )
					nCountForState = DS_REMOTEQUEUEFULL;
			}
			if (cur_src->IsBanned())
				nCountForState = DS_BANNED;
			if (cur_src->GetSourceFrom() >= SF_SERVER && cur_src->GetSourceFrom() <= SF_PASSIVE)
				++src_stats[cur_src->GetSourceFrom()];

			ASSERT( nCountForState < sizeof(m_anStates)/sizeof(m_anStates[0]) );
			m_anStates[nCountForState]++;
			
			// SLUGFILLER: SafeHash
			if (hashsetneeded) {
				switch (cur_src->GetDownloadState()){
				case DS_NONEEDEDPARTS:
				case DS_ONQUEUE:
					if (!cur_src->GetRequestedHashset())
						cur_src->RequestHashset();
					break;
				}
			}
			// SLUGFILLER: SafeHash

			DWORD dwLastCheck = dwCurTick - cur_src->GetLastAskedTime();

			switch (cur_src->GetDownloadState())
			{
				case DS_DOWNLOADING:{
					ASSERT( cur_src->socket );
					if (cur_src->socket)
					{
						uint32 cur_datarate = cur_src->CalculateDownloadRate();
						datarate += cur_datarate;
						//MORPH - Added by Yun.SF3, ZZ Upload System
						uint32 curClientReducedDownload = reducedownload;
						if(cur_src->IsFriend() && cur_src->GetFriendSlot())
							curClientReducedDownload = friendReduceddownload;
						if (curClientReducedDownload && cur_src->GetDownloadState() == DS_DOWNLOADING)
						{
							uint32 limit = curClientReducedDownload*cur_datarate/1000; //(uint32)(((float)reducedownload/100)*cur_datarate)/10;
							if (limit < 1000 && curClientReducedDownload == 200)
						//MORPH - Added by Yun.SF3, ZZ Upload System
								limit += 1000;
							else if (limit < 1)
								limit = 1;
							cur_src->socket->SetDownloadLimit(limit);
						}
						else
							cur_src->socket->DisableDownloadLimit();
					}
					break;
				}
				// Do nothing with this client..
				case DS_BANNED:
				case DS_ERROR:
					break;	
				// Check if something has changed with our or their ID state..
				case DS_LOWTOLOWIP:
				{
					//Make sure this source is still a LowID Client..
					if( cur_src->HasLowID() )
					{
						//Make sure we still cannot callback to this Client..
						if( !theApp.DoCallback( cur_src ) )
						{
							//If we are almost maxed on sources, slowly remove these client to see if we can find a better source.
							if( ((dwCurTick - lastpurgetime) > SEC2MS(30)) && (this->GetSourceCount() >= (thePrefs.GetMaxSourcePerFile()*.8 )) )
							{
						theApp.downloadqueue->RemoveSource( cur_src );
						lastpurgetime = dwCurTick;
							}
						break;
					}
					}
					// This should no longer be a LOWTOLOWIP..
					cur_src->SetDownloadState(DS_ONQUEUE);
						break;
				}
				case DS_NONEEDEDPARTS:
				{
					// we try to purge noneeded source, even without reaching the limit
					if( (dwCurTick - lastpurgetime) > SEC2MS(40) ){
						if( !cur_src->SwapToAnotherFile( false , false, false , NULL ) ){
							//however we only delete them if reaching the limit
							if (GetSourceCount() >= (thePrefs.GetMaxSourcePerFile()*.8 )){
								theApp.downloadqueue->RemoveSource( cur_src );
								lastpurgetime = dwCurTick;
								break; //Johnny-B - nothing more to do here (good eye!)
							}			
						}
						else
						{
							cur_src->DontSwapTo(this);
							lastpurgetime = dwCurTick;
							break;
						}
					}
					// doubled reasktime for no needed parts - save connections and traffic
					if (!((!cur_src->GetLastAskedTime()) || (dwLastCheck > FILEREASKTIME*2)))
						break; 
					// Recheck this client to see if still NNP.. Set to DS_NONE so that we force a TCP reask next time..
					cur_src->SetDownloadState(DS_NONE);
					break; 
				}
				case DS_ONQUEUE:
				{
					if( cur_src->IsRemoteQueueFull() )
					{
						if( ((dwCurTick - lastpurgetime) > MIN2MS(1)) && (GetSourceCount() >= (thePrefs.GetMaxSourcePerFile()*.8 )) )
						{
							theApp.downloadqueue->RemoveSource( cur_src );
							lastpurgetime = dwCurTick;
							break; //Johnny-B - nothing more to do here (good eye!)
						}
					}
					//MORPH START - Added by SiRoB, Due to Khaos A4AF
					// -khaos--+++>
					else
					{
					// <-----khaos-
						// khaos::kmod+ Smart A4AF Source Balancing (Now on a by-category basis)
						// Brute Force A4AF Transferring
						// Spread Reask
						if (!cur_src->GetLastForceA4AFTick() || (dwCurTick - cur_src->GetLastForceA4AFTick()) > (uint32)GetRandRange(150000,180000)/* 2m30s to 3m */)
						{
							if(thePrefs.UseSmartA4AFSwapping())
								if (cur_src->SwapToForcedA4AF())
									break; // This source was transferred, nothing more to do here.
						}
						if (!cur_src->GetLastBalanceTick() || (dwCurTick - cur_src->GetLastBalanceTick()) > (uint32)GetRandRange(150000,180000)/* 2m30s to 3m */)
						{
							uint8 iA4AFMode = thePrefs.AdvancedA4AFMode();
							if (iA4AFMode && thePrefs.GetCategory(GetCategory())->iAdvA4AFMode)
								iA4AFMode = thePrefs.GetCategory(GetCategory())->iAdvA4AFMode;
							if (iA4AFMode == 1 && cur_src->BalanceA4AFSources()) 
								break; // This source was transferred, nothing more to do here.
							if (iA4AFMode == 2 && cur_src->StackA4AFSources())
								break; // This source was transferred, nothing more to do here.
						}
					}
					//MORPH END - Added by SiRoB, Due to Khaos A4AF
					//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
					if ((cur_src->GetLastAskedTime()) && (dwLastCheck < FILEREASKTIME*2) && notSeenCompleteSource())
						break;//shadow#(onlydownloadcompletefiles)
					//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
					//Give up to 1 min for UDP to respond.. If we are within on min on TCP, do not try..
					if (theApp.IsConnected() && ((cur_src->GetLastAskedTime()) && (dwLastCheck > FILEREASKTIME-MIN2MS(2) && dwLastCheck < FILEREASKTIME-SEC2MS(1))))
						cur_src->UDPReaskForDownload();
				}
				case DS_CONNECTING:
				case DS_TOOMANYCONNS:
				case DS_NONE:
				case DS_WAITCALLBACK:
				{
					//MORPH START - Changed by SiRoB, Changed Spread Reask For Better SUC result and more
					/*
					if (theApp.IsConnected() && ((!cur_src->GetLastAskedTime()) || (dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME))
					*/
					if (theApp.IsConnected() && ((!cur_src->GetLastAskedTime()) || ((dwCurTick > (cur_src->GetLastAskedTime()+FILEREASKTIME)) &&  (dwCurTick % FILEREASKTIME > (UINT32)((FILEREASKTIME / AvailableSrcCount) * (srcPosReask++ % AvailableSrcCount))))))
					//MORPH END   - Changed by SiRoB, Spread Reask For Better SUC functioning and more
					{
						if(!cur_src->AskForDownload()) // NOTE: This may *delete* the client!!
							break; //I left this break here just as a reminder just in case re rearange things..
					}
					break;
				}
				// SLUGFILLER: SafeHash
				case DS_REQHASHSET:
					if (!cur_src->GetRequestedHashset() || dwCurTick - cur_src->GetRequestedHashset() > CONNECTION_TIMEOUT)	// timeout for hashset request
						cur_src->SendStartupLoadReq();	// this will set the client's status to on-queue
					break;
				// SLUGFILLER: SafeHash			
			}
		}

		//MORPH START - Changed by SiRoB, Advanced A4AF derivated from Khaos
		//if (IsA4AFAuto() && ((!m_LastNoNeededCheck) || (dwCurTick - m_LastNoNeededCheck > 900000)) )
		if (!thePrefs.AdvancedA4AFMode() && !thePrefs.UseSmartA4AFSwapping() && IsA4AFAuto() && ((!m_LastNoNeededCheck) || (dwCurTick - m_LastNoNeededCheck > 900000)) )
		//MORPH END   - Changed by SiRoB, Advanced A4AF derivated from Khaos
		{
			m_LastNoNeededCheck = dwCurTick;
			POSITION pos1, pos2;
			for (pos1 = A4AFsrclist.GetHeadPosition();(pos2=pos1)!=NULL;)
			{
				A4AFsrclist.GetNext(pos1);
				CUpDownClient *cur_source = A4AFsrclist.GetAt(pos2);
				if( cur_source->GetDownloadState() != DS_DOWNLOADING
					&& cur_source->reqfile 
					&& ( (!cur_source->reqfile->IsA4AFAuto()) || cur_source->GetDownloadState() == DS_NONEEDEDPARTS)
					&& !cur_source->IsSwapSuspended(this) )
				{
					CPartFile* oldfile = cur_source->reqfile;
					if (cur_source->SwapToAnotherFile(false, false, false, this)){
						cur_source->DontSwapTo(oldfile);
					}
				}
			}
		}
		if( thePrefs.GetMaxSourcePerFileUDP() > GetSourceCount()){
			if (theApp.downloadqueue->DoKademliaFileRequest() && (Kademlia::CKademlia::getTotalFile() < KADEMLIATOTALFILE) && (!lastsearchtimeKad || (dwCurTick - lastsearchtimeKad) > KADEMLIAREASKTIME) &&  Kademlia::CKademlia::isConnected() && theApp.IsConnected() && !stopped){ //Once we can handle lowID users in Kad, we remove the second IsConnected
				//Kademlia
				theApp.downloadqueue->SetLastKademliaFileRequest();
				if (Kademlia::CKademlia::isRunning() && !GetKadFileSearchID())
				{
					Kademlia::CUInt128 kadFileID;
					kadFileID.setValue(GetFileHash());
					Kademlia::CSearch* pSearch = Kademlia::CSearchManager::prepareFindFile(Kademlia::CSearch::FILE, true, kadFileID);
					if (pSearch)
					{
						lastsearchtimeKad = dwCurTick;
						SetKadFileSearchID(pSearch->getSearchID());
					}
						else
						SetKadFileSearchID(0);
				}
			}
		}
		else{
			if(GetKadFileSearchID())
			{
				Kademlia::CSearchManager::stopSearch(GetKadFileSearchID(), true);
			}
		}


		// check if we want new sources from server
		if ( !m_bLocalSrcReqQueued && ((!lastsearchtime) || (dwCurTick - lastsearchtime) > SERVERREASKTIME) && theApp.serverconnect->IsConnected()
			&& thePrefs.GetMaxSourcePerFileSoft() > GetSourceCount() && !stopped )
		{
			m_bLocalSrcReqQueued = true;
			theApp.downloadqueue->SendLocalSrcRequest(this);
		}

		count++;
		if (count == 3){
			count = 0;
			// khaos::kmod+ Save/Load Sources
			if (thePrefs.UseSaveLoadSources())
				m_sourcesaver.Process(this); //<<-- enkeyDEV(Ottavio84) -New SLS-
			// khaos::kmod-
			UpdateAutoDownPriority();
			UpdateDisplayedInfo();
			UpdateCompletedInfos();
		}
	}

	if ( GetSrcStatisticsValue(DS_DOWNLOADING) != nOldTransSourceCount ){
		// khaos::categorymod+
		//if (theApp.emuledlg->transferwnd->downloadlistctrl.curTab == 0)
		theApp.emuledlg->transferwnd->downloadlistctrl.ChangeCategory(theApp.emuledlg->transferwnd->GetActiveCategory());
		//else
		//	UpdateDisplayedInfo(true);
		// khaos::categorymod-
		if (thePrefs.ShowCatTabInfos())
			theApp.emuledlg->transferwnd->UpdateCatTabTitles();
	}

	return datarate;
	
}

bool CPartFile::CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, UINT* pdebug_lowiddropped, bool Ed2kID)
{
	// MOD Note: Do not change this part - Merkur
	uint32 hybridID = 0;
	if( Ed2kID )
		hybridID = ntohl(userid);
	else
	{
		hybridID = userid;
		userid = ntohl(userid);
	}

	// MOD Note: Do not change this part - Merkur
	if (theApp.serverconnect->IsConnected())
	{
		if(theApp.serverconnect->IsLowID())
		{
			if(theApp.serverconnect->GetClientID() == userid && theApp.serverconnect->GetCurrentServer()->GetIP() == serverip && theApp.serverconnect->GetCurrentServer()->GetPort() == serverport )
				return false;
			if(theApp.serverconnect->GetLocalIP() == userid)
				return false;
		}
		else
		{
			if(theApp.serverconnect->GetClientID() == userid && thePrefs.GetPort() == port)
				return false;
		}
	}
	if (Kademlia::CKademlia::isConnected())
	{
		if(Kademlia::CKademlia::isFirewalled())
		{
			//This will change with LowID support is added..
			if(Kademlia::CKademlia::getPrefs()->getIPAddress() == hybridID && thePrefs.GetPort() == port)
				return false;
		}
		else
		{
			if(Kademlia::CKademlia::getIPAddress() == hybridID && thePrefs.GetPort() == port)
				return false;
		}
	}

		//This allows *.*.*.0 clients to not be removed..
	if ( ((Ed2kID && IsLowID(userid)) || (!Ed2kID && IsLowID(hybridID))) && theApp.IsFirewalled())
	{
		if (pdebug_lowiddropped)
			(*pdebug_lowiddropped)++;
		return false;
	}
	// MOD Note - end
	return true;
}

void CPartFile::AddSources(CSafeMemFile* sources, uint32 serverip, uint16 serverport)
{
	UINT count = sources->ReadUInt8();

	if (stopped)
	{
		// since we may received multiple search source UDP results we have to "consume" all data of that packet
		sources->Seek(count*(4+2), SEEK_SET);
		return;
	}

	UINT debug_lowiddropped = 0;
	UINT debug_possiblesources = 0;
	for (UINT i = 0; i < count; i++)
	{
		uint32 userid = sources->ReadUInt32();
		uint16 port = sources->ReadUInt16();

		// check the HighID(IP) - "Filter LAN IPs" and "IPfilter" the received sources IP addresses
		if (!IsLowID(userid))
		{
			if (!IsGoodIP(userid))
			{ 
				// check for 0-IP, localhost and optionally for LAN addresses
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received from server - bad IP"), inet_ntoa(*(in_addr*)&userid));
				continue;
			}
			if (theApp.ipfilter->IsFiltered(userid))
			{
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received from server - IP filter (%s)"), inet_ntoa(*(in_addr*)&userid), theApp.ipfilter->GetLastHit());
				theApp.stat_filteredclients++; //MORPH - Added by SiRoB, To comptabilise ipfiltered
				continue;
			}
		}

		// additionally check for LowID and own IP
		if (!CanAddSource(userid, port, serverip, serverport, &debug_lowiddropped))
		{
			if (thePrefs.GetLogFilteredIPs())
				AddDebugLogLine(false, _T("Ignored source (IP=%s) received from server"), inet_ntoa(*(in_addr*)&userid));
			continue;
		}

		if( thePrefs.GetMaxSourcePerFile() > this->GetSourceCount() )
		{
			debug_possiblesources++;
			CUpDownClient* newsource = new CUpDownClient(this,port,userid,serverip,serverport,true);
			theApp.downloadqueue->CheckAndAddSource(this,newsource);
		}
		else
		{
			// since we may received multiple search source UDP results we have to "consume" all data of that packet
			sources->Seek((count-i)*(4+2), SEEK_SET);
			if(GetKadFileSearchID())
				Kademlia::CSearchManager::stopSearch(GetKadFileSearchID(), false);
			break;
		}
	}
	if ( thePrefs.GetDebugSourceExchange() )
		AddDebugLogLine(false,"RCV: %i sources from server, %i low id dropped, %i possible sources File(%s)",count,debug_lowiddropped,debug_possiblesources, GetFileName());
}

//Morph Start - added by AndCycle, itsonlyme: cacheUDPsearchResults
// itsonlyme: cacheUDPsearchResults
CServer *CPartFile::GetNextAvailServer()
{
	if (m_preferredServers.IsEmpty()) 
		return NULL;

	POSITION pos = m_preferredServers.GetHeadPosition();
	SServer aServer = m_preferredServers.GetValueAt(pos);
	m_preferredServers.RemoveAt(pos);


	CServer *nextServer = theApp.serverlist->GetServerByIP(aServer.m_nIP, aServer.m_nPort);
	if (!nextServer) 
		return NULL;

	CString tracemsg;
	tracemsg.Format("GetNextAvailServer returned %s:%i server with %i sources for %s", nextServer->GetAddress(), nextServer->GetPort(), aServer.m_uAvail, this->m_strFileName);
	TRACE(tracemsg);
	AddDebugLogLine(false, tracemsg);

	return nextServer;
}

void CPartFile::AddAvailServer(SServer server)
{
	m_preferredServers.Insert(server.m_uAvail, server);
	while (m_preferredServers.GetCount() > MAX_PREF_SERVERS)
		m_preferredServers.RemoveAt(m_preferredServers.GetHeadPosition());
}
// itsonlyme: cacheUDPsearchResults
//Morph End - added by AndCycle, itsonlyme: cacheUDPsearchResults

// SLUGFILLER: heapsortCompletesrc
static void HeapSort(CArray<uint16,uint16> &count, uint32 first, uint32 last){
	uint32 r;
	for ( r = first; !(r & 0x80000000) && (r<<1) < last; ){
		uint32 r2 = (r<<1)+1;
		if (r2 != last)
			if (count[r2] < count[r2+1])
				r2++;
		if (count[r] < count[r2]){
			uint16 t = count[r2];
			count[r2] = count[r];
			count[r] = t;
			r = r2;
		}
		else
			break;
	}
}
// SLUGFILLER: heapsortCompletesrc

void CPartFile::UpdatePartsInfo()
{
	if( !IsPartFile() )
	{
		CKnownFile::UpdatePartsInfo();
		return;
	}

	// Cache part count
	uint16 partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 

	// Reset part counters
	if(m_SrcpartFrequency.GetSize() < partcount)
		m_SrcpartFrequency.SetSize(partcount);
	for(int i = 0; i < partcount; i++)
		m_SrcpartFrequency[i] = 0;
	
	CArray<uint16,uint16> count;
	if (flag)
		count.SetSize(0, srclist.GetSize());
	for (POSITION pos = srclist.GetHeadPosition(); pos != 0; )
	{
		CUpDownClient* cur_src = srclist.GetNext(pos);
		if( cur_src->GetPartStatus() )
		{		
			for (int i = 0; i < partcount; i++)
			{
				if (cur_src->IsPartAvailable(i))
					m_SrcpartFrequency[i] += 1;
			}
			if ( flag )
			{
				count.Add(cur_src->GetUpCompleteSourcesCount());
			}
		}
	}

	if (flag)
	{
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;
	
		for (uint16 i = 0; i < partcount; i++)
		{
			if( !i )
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
			else if( m_nCompleteSourcesCount > m_SrcpartFrequency[i])
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
		}
	
		count.Add(m_nCompleteSourcesCount);
	
		int n = count.GetSize();
		if (n > 0)
		{
			// SLUGFILLER: heapsortCompletesrc
			int r;
			for (r = n/2; r--; )
				HeapSort(count, r, n-1);
			for (r = n; --r; ){
				uint16 t = count[r];
				count[r] = count[0];
				count[0] = t;
				HeapSort(count, 0, r-1);
			}
			// SLUGFILLER: heapsortCompletesrc

			// calculate range
			int i = n >> 1;			// (n / 2)
			int j = (n * 3) >> 2;	// (n * 3) / 4
			int k = (n * 7) >> 3;	// (n * 7) / 8

			//When still a part file, adjust your guesses by 20% to what you see..

			//Not many sources, so just use what you see..
			if (n < 5)
			{
//				m_nCompleteSourcesCount;
				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCountHi= m_nCompleteSourcesCount;
			}
			//For low guess and normal guess count
			//	If we see more sources then the guessed low and normal, use what we see.
			//	If we see less sources then the guessed low, adjust network accounts for 80%, we account for 20% with what we see and make sure we are still above the normal.
			//For high guess
			//  Adjust 80% network and 20% what we see.
			else if (n < 20)
			{
				if ( count.GetAt(i) < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountLo = m_nCompleteSourcesCount;
				else
					m_nCompleteSourcesCountLo = (uint16)((float)(count.GetAt(i)*.8)+(float)(m_nCompleteSourcesCount*.2));
				m_nCompleteSourcesCount= m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi= (uint16)((float)(count.GetAt(j)*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
			}
			else
			//Many sources..
			//For low guess
			//	Use what we see.
			//For normal guess
			//	Adjust network accounts for 80%, we account for 20% with what we see and make sure we are still above the low.
			//For high guess
			//  Adjust network accounts for 80%, we account for 20% with what we see and make sure we are still above the normal.
			{
				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCount= (uint16)((float)(count.GetAt(j)*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCount < m_nCompleteSourcesCountLo )
					m_nCompleteSourcesCount = m_nCompleteSourcesCountLo;
				m_nCompleteSourcesCountHi= (uint16)((float)(count.GetAt(k)*.8)+(float)(m_nCompleteSourcesCount*.2));
				if( m_nCompleteSourcesCountHi < m_nCompleteSourcesCount )
					m_nCompleteSourcesCountHi = m_nCompleteSourcesCount;
			}
		}
		m_nCompleteSourcesTime = time(NULL) + (60);
	}
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	m_nVirtualCompleteSourcesCount = (uint16)-1;
	for (uint16 i = 0; i < partcount; i++){
		if(m_nVirtualCompleteSourcesCount > m_SrcpartFrequency[i])
			m_nVirtualCompleteSourcesCount = m_SrcpartFrequency[i];
	}

	UpdatePowerShareLimit(m_nCompleteSourcesCountHi<200, m_nCompleteSourcesCountHi==1 || (m_nCompleteSourcesCountHi==0 && m_nVirtualCompleteSourcesCount>0) || m_nVirtualCompleteSourcesCount==1,m_nCompleteSourcesCountHi>((GetPowerShareLimit()>=0)?GetPowerShareLimit():thePrefs.GetPowerShareLimit()));
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	UpdateDisplayedInfo(true);
	//MORPH END   - Added by Yun.SF3, ZZ Upload System
	//MORPH START - Added by SiRoB,  SharedStatusBar CPU Optimisation
	InChangedSharedStatusBar = false;
	//MORPH END   - Added by SiRoB,  SharedStatusBar CPU Optimisation
}

bool  CPartFile::RemoveBlockFromList(uint32 start,uint32 end)
{
	bool bResult = false;
	for (POSITION pos = requestedblocks_list.GetHeadPosition(); pos != NULL; ){
		POSITION posLast = pos;
		Requested_Block_Struct* block = requestedblocks_list.GetNext(pos);
		if (block->StartOffset <= start && block->EndOffset >= end){
			requestedblocks_list.RemoveAt(posLast);
			bResult = true;
		}
	}
	return bResult;
}

bool CPartFile::IsInRequestedBlockList(const Requested_Block_Struct* block) const
{
	return requestedblocks_list.Find(const_cast<Requested_Block_Struct*>(block)) != NULL;
}

void CPartFile::RemoveAllRequestedBlocks(void)
{
	requestedblocks_list.RemoveAll();
}

void CPartFile::CompleteFile(bool bIsHashingDone)
{
	theApp.downloadqueue->RemoveLocalServerRequest(this);
	if(GetKadFileSearchID())
		Kademlia::CSearchManager::stopSearch(GetKadFileSearchID(), false);

	if( this->srcarevisible )
		theApp.emuledlg->transferwnd->downloadlistctrl.HideSources(this);
	
	if (!bIsHashingDone){
		//m_hpartfile.Flush(); // flush the OS buffer before completing...
		SetStatus(PS_COMPLETING);
		datarate = 0;
		CAddFileThread* addfilethread = (CAddFileThread*) AfxBeginThread(RUNTIME_CLASS(CAddFileThread), THREAD_PRIORITY_BELOW_NORMAL,0, CREATE_SUSPENDED);
		addfilethread->SetValues(0,thePrefs.GetTempDir(),RemoveFileExtension(m_partmetfilename),this);
		addfilethread->ResumeThread();	
		return;
	}
	else{
		StopFile();
		SetStatus(PS_COMPLETING);
		m_is_A4AF_auto=false;
		CWinThread *pThread = AfxBeginThread(CompleteThreadProc, this, THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED); // Lord KiRon - using threads for file completion
		if (pThread)
			pThread->ResumeThread();
		else
			throw GetResString(IDS_ERR_FILECOMPLETIONTHREAD);
	}
	theApp.emuledlg->transferwnd->downloadlistctrl.ShowFilesCount();
	if (thePrefs.ShowCatTabInfos() ) theApp.emuledlg->transferwnd->UpdateCatTabTitles();
	UpdateDisplayedInfo(true);
}

UINT CPartFile::CompleteThreadProc(LPVOID pvParams) 
{ 
	DbgSetThreadName("PartFileComplete");
	CPartFile* pFile = (CPartFile*)pvParams;
	if (!pFile)
		return (UINT)-1; 
   	pFile->PerformFileComplete(); 
   	return 0; 
}

void UncompressFile(LPCTSTR pszFilePath)
{
	// check, if it's a comressed file
	DWORD dwAttr = GetFileAttributes(pszFilePath);
	if (dwAttr == INVALID_FILE_ATTRIBUTES || (dwAttr & FILE_ATTRIBUTE_COMPRESSED) == 0)
		return;

	CString strDir = pszFilePath;
	PathRemoveFileSpec(strDir.GetBuffer());
	strDir.ReleaseBuffer();

	// If the directory of the file has the 'Compress' attribute, do not uncomress the file
	dwAttr = GetFileAttributes(strDir);
	if (dwAttr == INVALID_FILE_ATTRIBUTES || (dwAttr & FILE_ATTRIBUTE_COMPRESSED) != 0)
		return;

	HANDLE hFile = CreateFile(pszFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE){
		if (thePrefs.GetVerbose())
			theApp.QueueDebugLogLine(true, _T("Failed to open file \"%s\" for decompressing - %s"), pszFilePath, GetErrorMessage(GetLastError(), 1));
		return;
	}
	
	USHORT usInData = COMPRESSION_FORMAT_NONE;
	DWORD dwReturned = 0;
	if (!DeviceIoControl(hFile, FSCTL_SET_COMPRESSION, &usInData, sizeof usInData, NULL, 0, &dwReturned, NULL)){
		if (thePrefs.GetVerbose())
			theApp.QueueDebugLogLine(true, _T("Failed to decompress file \"%s\" - %s"), pszFilePath, GetErrorMessage(GetLastError(), 1));
	}
	CloseHandle(hFile);
}

// Lord KiRon - using threads for file completion
// NOTE: This function is executed within a seperate thread, do *NOT* use an lists/queues of the main thread without
// synchronization. Even the access to couple of members of the CPartFile (e.g. filename) would need to be properly
// synchronization to achive full multi threading compliance.
BOOL CPartFile::PerformFileComplete() 
{
	// If that function is invoked from within the file completion thread, it's ok if we wait (and block) the thread.
	CSingleLock sLock(&m_FileCompleteMutex, TRUE);

	CString strPartfilename(RemoveFileExtension(m_fullname));
	char* newfilename = nstrdup(GetFileName());
	strcpy(newfilename, (LPCTSTR)StripInvalidFilenameChars(newfilename));

	CString strNewname;
	CString indir;

	if (PathFileExists( thePrefs.GetCategory(GetCategory())->incomingpath)) {
		indir=thePrefs.GetCategory(GetCategory())->incomingpath;
		strNewname.Format("%s\\%s",indir,newfilename);
	}
	else{
		indir=thePrefs.GetIncomingDir();
		strNewname.Format("%s\\%s",indir,newfilename);
	}

	// close permanent handle
	try{
		if (m_hpartfile.m_hFile != INVALID_HANDLE_VALUE)
			m_hpartfile.Close();
	}
	catch(CFileException* error){
		char buffer[MAX_CFEXP_ERRORMSG];
		error->GetErrorMessage(buffer,MAX_CFEXP_ERRORMSG);
		theApp.QueueLogLine(true, GetResString(IDS_ERR_FILEERROR), m_partmetfilename, GetFileName(), buffer);
		error->Delete();
		//return false;
	}

	bool renamed = false;
	if(PathFileExists(strNewname))
	{
		renamed = true;
		int namecount = 0;

		size_t length = strlen(newfilename);
		ASSERT(length != 0); //name should never be 0

		//the file extension
		char *ext = strrchr(newfilename, '.');
		if(ext == NULL)
			ext = newfilename + length;

		char *last = ext;  //new end is the file name before extension
		last[0] = 0;  //truncate file name

		//search for matching ()s and check if it contains a number
		if((ext != newfilename) && (strrchr(newfilename, ')') + 1 == last)) {
			char *first = strrchr(newfilename, '(');
			if(first != NULL) {
				first++;
				bool found = true;
				for(char *step = first; step < last - 1; step++)
					if(*step < '0' || *step > '9') {
						found = false;
						break;
					}
				if(found) {
					namecount = atoi(first);
					last = first - 1;
					last[0] = 0;  //truncate again
				}
			}
		}

		CString strTestName;
		do {
			namecount++;
			strTestName.Format("%s\\%s(%d).%s", indir, newfilename, namecount, min(ext + 1, newfilename + length));
		}
		while (PathFileExists(strTestName));
		strNewname = strTestName;
	}
	delete[] newfilename;

	if (rename(strPartfilename,strNewname)){
		if (this){
			if (errno == ENOENT && strNewname.GetLength() >= MAX_PATH)
				theApp.QueueLogLine(true,GetResString(IDS_ERR_COMPLETIONFAILED) + _T(" - \"%s\": Path too long"),GetFileName(), strNewname);
			else
				theApp.QueueLogLine(true,GetResString(IDS_ERR_COMPLETIONFAILED) + _T(" - \"%s\": ") + CString(strerror(errno)),GetFileName(), strNewname);
		}
		paused = true;
		stopped = true;
		SetStatus(PS_ERROR);
		m_bCompletionError = true;
		if (theApp.emuledlg && theApp.emuledlg->IsRunning())
			VERIFY( PostMessage(theApp.emuledlg->m_hWnd, TM_FILECOMPLETED, FILE_COMPLETION_THREAD_FAILED, (LPARAM)this) );
		return FALSE;
	}

	UncompressFile(strNewname);

	// to have the accurate date stored in known.met we have to update the 'date' of a just completed file.
	// if we don't update the file date here (after commiting the file and before adding the record to known.met), 
	// that file will be rehashed at next startup and there would also be a duplicate entry (hash+size) in known.met
	// because of different file date!
	ASSERT( m_hpartfile.m_hFile == INVALID_HANDLE_VALUE ); // the file must be closed/commited!
	struct _stat st;
	if (_stat(strNewname, &st) == 0)
	{
		m_tLastModified = st.st_mtime;
		m_tUtcLastModified = m_tLastModified;
		AdjustNTFSDaylightFileTime(m_tUtcLastModified, strNewname);
	}

	// remove part.met file
	if (remove(m_fullname))
		theApp.QueueLogLine(true,GetResString(IDS_ERR_DELETEFAILED) + _T(" - ") + CString(strerror(errno)),m_fullname);
	// khaos::kmod+ Save/Load Sources
	else
		m_sourcesaver.DeleteFile(this); //<<-- enkeyDEV(Ottavio84) -New SLS-
	// khaos::kmod-

	// remove backup files
	CString BAKName(m_fullname);
	BAKName.Append(PARTMET_BAK_EXT);
	if (_taccess(BAKName, 0) == 0 && !::DeleteFile(BAKName))
		theApp.QueueLogLine(true,GetResString(IDS_ERR_DELETE) + _T(" - ") + GetErrorMessage(GetLastError()), BAKName);

	BAKName = m_fullname;
	BAKName.Append(PARTMET_TMP_EXT);
	if (_taccess(BAKName, 0) == 0 && !::DeleteFile(BAKName))
		theApp.QueueLogLine(true,GetResString(IDS_ERR_DELETE) + _T(" - ") + GetErrorMessage(GetLastError()), BAKName);

	// initialize 'this' part file for being a 'complete' file, this is to be done *before* releasing the file mutex.
	m_fullname = strNewname;
	SetPath(indir);
	SetFilePath(m_fullname);
	SetStatus(PS_COMPLETE);
	paused = false;
	// paused = false; // khaos::kmod+ No longer necessary.
// explicitly unlock the file before posting something to the main thread.
	sLock.Unlock();

	if (theApp.emuledlg && theApp.emuledlg->IsRunning())
		VERIFY( PostMessage(theApp.emuledlg->m_hWnd, TM_FILECOMPLETED, FILE_COMPLETION_THREAD_SUCCESS | (renamed ? FILE_COMPLETION_THREAD_RENAMED : 0), (LPARAM)this) );
	return TRUE;
}

// 'End' of file completion, to avoid multi threading synchronization problems, this is to be invoked from within the
// main thread!
void CPartFile::PerformFileCompleteEnd(DWORD dwResult)
{
	bool isShared = theApp.sharedfiles && theApp.sharedfiles->GetFileByID(GetFileHash()) == this;	// SLUGFILLER: mergeKnown
	if (dwResult & FILE_COMPLETION_THREAD_SUCCESS)
	{
		// give visual response
		AddLogLine(true,GetResString(IDS_DOWNLOADDONE),GetFileName());
		theApp.emuledlg->ShowNotifier(GetResString(IDS_TBN_DOWNLOADDONE) + _T('\n') + GetFileName(), TBN_DLOAD);
		if (dwResult & FILE_COMPLETION_THREAD_RENAMED)
		{
			CString strFilePath(GetFullName());
			PathStripPath(strFilePath.GetBuffer());
			strFilePath.ReleaseBuffer();
			AddLogLine(true, GetResString(IDS_DOWNLOADRENAMED), strFilePath);
		}
	
		if (isShared)	// SLUGFILLER: mergeKnown
			theApp.knownfiles->SafeAddKFile(this);
		theApp.downloadqueue->RemoveFile(this);
		// mobilemule
		theApp.mmserver->AddFinishedFile(this);
		// end mobilemule

		if (thePrefs.GetRemoveFinishedDownloads())
			theApp.emuledlg->transferwnd->downloadlistctrl.RemoveFile(this);
		else
			UpdateDisplayedInfo(true);

		theApp.emuledlg->transferwnd->downloadlistctrl.ShowFilesCount();

		// -khaos--+++> Extended Statistics Added 2-10-03
		thePrefs.Add2DownCompletedFiles();		// Increments cumDownCompletedFiles in prefs struct
		thePrefs.Add2DownSessionCompletedFiles(); // Increments sesDownCompletedFiles in prefs struct
		thePrefs.SaveCompletedDownloadsStat();	// Saves cumDownCompletedFiles to INI
		// <-----khaos- End Statistics Modifications

		// 05-Jän-2004 [bc]: ed2k and Kad are already full of totally wrong and/or not properly attached meta data. Take
		// the chance to clean any available meta data tags and provide only tags which were determined by us.
		UpdateMetaDataTags();
		//This was to update the sharedfile bar as this will now use different data.. But I think there may be a sync issue here.
		//UpdatePartsInfo();

		// republish that file to the ed2k-server to update the 'FT_COMPLETE_SOURCES' counter on the server.
		if (isShared)	// SLUGFILLER: mergeKnown
			theApp.sharedfiles->RepublishFile(this);
	}

	if (thePrefs.StartNextFile())
		theApp.downloadqueue->StartNextFile(GetCategory());

	// SLUGFILLER: mergeKnown
	if (!isShared)
		delete this;
	// SLUGFILLER: mergeKnown
}

void  CPartFile::RemoveAllSources(bool bTryToSwap){
	POSITION pos1,pos2;
	for( pos1 = srclist.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
		srclist.GetNext(pos1);
		if (bTryToSwap){
			if (!srclist.GetAt(pos2)->SwapToAnotherFile(true, true, true, NULL) )
				theApp.downloadqueue->RemoveSource(srclist.GetAt(pos2), false);
		}
		else
			theApp.downloadqueue->RemoveSource(srclist.GetAt(pos2), false);
	}
	UpdatePartsInfo();
	UpdateAvailablePartsCount();

	//[enkeyDEV(Ottavio84) -A4AF-]
	// remove all links A4AF in sources to this file
	if(!A4AFsrclist.IsEmpty())
	{
		POSITION pos1, pos2;
		for(pos1 = A4AFsrclist.GetHeadPosition();(pos2=pos1)!=NULL;)
		{
			A4AFsrclist.GetNext(pos1);
			
			POSITION pos3 = A4AFsrclist.GetAt(pos2)->m_OtherRequests_list.Find(this); 
			if(pos3)
			{ 
				A4AFsrclist.GetAt(pos2)->m_OtherRequests_list.RemoveAt(pos3);
				theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource(this->A4AFsrclist.GetAt(pos2),this);
			}
			else{
				pos3 = A4AFsrclist.GetAt(pos2)->m_OtherNoNeeded_list.Find(this); 
				if(pos3)
				{ 
					A4AFsrclist.GetAt(pos2)->m_OtherNoNeeded_list.RemoveAt(pos3);
					theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource(A4AFsrclist.GetAt(pos2),this);
				}
			}
		}
		A4AFsrclist.RemoveAll();
	}
	
	UpdateFileRatingCommentAvail();
}

void CPartFile::DeleteFile(){
	ASSERT ( !m_bPreviewing );

	// Barry - Need to tell any connected clients to stop sending the file
	StopFile(true);

	// feel free to implement a runtime handling mechanism!
	if (m_AllocateThread!=NULL) {
		theApp.AddLogLine(true,GetResString(IDS_DELETEAFTERALLOC),GetFileName());
		m_bDeleteAfterAlloc=true;
		return;
	}

	theApp.sharedfiles->RemoveFile(this);
	theApp.downloadqueue->RemoveFile(this);
	theApp.emuledlg->transferwnd->downloadlistctrl.RemoveFile(this);

	if (m_hpartfile.m_hFile != INVALID_HANDLE_VALUE)
		m_hpartfile.Close();

	if (remove(m_fullname))
		AddLogLine(true,GetResString(IDS_ERR_DELETE) + _T(" - ") + CString(strerror(errno)),m_fullname);	
	// khaos::kmod+ Save/Load Sources
	else
		m_sourcesaver.DeleteFile(this); //<<-- enkeyDEV(Ottavio84) -New SLS-
	// khaos::kmod-
	CString partfilename(RemoveFileExtension(m_fullname));
	if (remove(partfilename))
		AddLogLine(true,GetResString(IDS_ERR_DELETE) + _T(" - ") + CString(strerror(errno)),partfilename);

	CString BAKName(m_fullname);
	BAKName.Append(PARTMET_BAK_EXT);
	if (_taccess(BAKName, 0) == 0 && !::DeleteFile(BAKName))
		AddLogLine(true,GetResString(IDS_ERR_DELETE) + _T(" - ") + GetErrorMessage(GetLastError()), BAKName);

	BAKName = m_fullname;
	BAKName.Append(PARTMET_TMP_EXT);
	if (_taccess(BAKName, 0) == 0 && !::DeleteFile(BAKName))
		AddLogLine(true,GetResString(IDS_ERR_DELETE) + _T(" - ") + GetErrorMessage(GetLastError()), BAKName);

	delete this;
}

// SLUGFILLER: SafeHash remove - removed HashSinglePart completely.
/*
bool CPartFile::HashSinglePart(uint16 partnumber)
{
	if ((GetHashCount() <= partnumber) && (GetPartCount() > 1)){
		AddLogLine(true,GetResString(IDS_ERR_HASHERRORWARNING),GetFileName());
		hashsetneeded = true;
		return true;
	}
	else if (!GetPartHash(partnumber) && GetPartCount() != 1){
		AddLogLine(true,GetResString(IDS_ERR_INCOMPLETEHASH),GetFileName());
		hashsetneeded = true;
		return true;		
	}
	else{
		uchar hashresult[16];
		m_hpartfile.Seek((LONGLONG)PARTSIZE*partnumber,0);
		uint32 length = PARTSIZE;
		if ((ULONGLONG)PARTSIZE*(partnumber+1) > m_hpartfile.GetLength()){
			length = (m_hpartfile.GetLength() - ((ULONGLONG)PARTSIZE*partnumber));
			ASSERT( length <= PARTSIZE );
		}
		CreateHashFromFile(&m_hpartfile,length,hashresult);

		if (GetPartCount()>1 || GetFileSize()==PARTSIZE){
			if (md4cmp(hashresult,GetPartHash(partnumber)))
				return false;
			else
				return true;
		}
		else{
			if (md4cmp(hashresult,m_abyFileHash))
				return false;
			else
				return true;
		}
	}
}
*/

bool CPartFile::IsCorruptedPart(uint16 partnumber) const
{
	return (corrupted_list.Find(partnumber) != NULL);
}

// Barry - Also want to preview zip/rar files
bool CPartFile::IsArchive(bool onlyPreviewable) const
{
	if (onlyPreviewable){
		CString extension = CString(GetFileName()).Right(4);
		return ((extension.CompareNoCase(".zip") == 0) || (extension.CompareNoCase(".rar") == 0));
	}

	return (ED2KFT_ARCHIVE == GetED2KFileTypeID(GetFileName()));
}

//MORPH START - Added by IceCream, music preview and defeat 0-filler
bool CPartFile::IsMovie() const
{
	return (ED2KFT_VIDEO == GetED2KFileTypeID(GetFileName()) );
}

bool CPartFile::IsMusic() const
{
	return (ED2KFT_AUDIO == GetED2KFileTypeID(GetFileName()) );
}

bool CPartFile::IsCDImage() const
{
	return (ED2KFT_CDIMAGE == GetED2KFileTypeID(GetFileName()) );
}

bool CPartFile::IsDocument() const
{
	return (ED2KFT_DOCUMENT == GetED2KFileTypeID(GetFileName()) );
}
//MORPH END   - Added by IceCream, music preview and defeat 0-filler

void CPartFile::SetDownPriority(uint8 np)
{
	if (np != PR_LOW && np != PR_NORMAL && np != PR_HIGH){
		ASSERT(0);
		np = PR_NORMAL;
	}

	m_iDownPriority = np;
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspaceTimed(); // SLUGFILLER: checkDiskspace
	UpdateDisplayedInfo(true);
	SavePartFile();
}

bool CPartFile::CanOpenFile() const
{
	return (GetStatus()==PS_COMPLETE);
}

void CPartFile::OpenFile() const
{
	ShellOpenFile(GetFullName(), NULL);
}

bool CPartFile::CanStopFile() const
{
	bool bFileDone = (GetStatus()==PS_COMPLETE || GetStatus()==PS_COMPLETING);
	return (!IsStopped() && GetStatus()!=PS_ERROR && !bFileDone);
}

void CPartFile::StopFile(bool bCancel)
{
	// Barry - Need to tell any connected clients to stop sending the file
	PauseFile();
	lastsearchtimeKad = 0;
	RemoveAllSources(true);
	paused = true;
	stopped = true;
	insufficient = false;
	datarate = 0;
	memset(m_anStates,0,sizeof(m_anStates));
	//EastShare Start - Added by AndCycle, Only download complete files v2.1 (shadow)
	if ((status!=PS_COMPLETE)&&(status!=PS_COMPLETING)) lastseencomplete = NULL;//shadow#(onlydownloadcompletefiles)
	//EastShare End - Added by AndCycle, Only download complete files v2.1 (shadow)
	if (!bCancel)
		FlushBuffer(true);
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace();	// SLUGFILLER: checkDiskspace
	UpdateDisplayedInfo(true);
}

void CPartFile::StopPausedFile()
{
	//Once an hour, remove any sources for files which are no longer active downloads
	UINT uState = GetStatus();
	if( (uState==PS_PAUSED || uState==PS_INSUFFICIENT || uState==PS_ERROR) && !stopped && time(NULL) - m_iLastPausePurge > (60*60) )
	{
		StopFile();
	}
	else
	{
		if (m_bDeleteAfterAlloc && m_AllocateThread==NULL)
		{
			DeleteFile();
			return;
		}
	}
}

bool CPartFile::CanPauseFile() const
{
	bool bFileDone = (GetStatus()==PS_COMPLETE || GetStatus()==PS_COMPLETING);
	return (GetStatus()!=PS_PAUSED && GetStatus()!=PS_ERROR && !bFileDone);
}

void CPartFile::PauseFile(bool bInsufficient)
{
	// if file is already in 'insufficient' state, don't set it again to insufficient. this may happen if a disk full
	// condition is thrown before the automatically and periodically check free diskspace was done.
	if (bInsufficient && insufficient)
		return;

	// if file is already in 'paused' or 'insufficient' state, do not refresh the purge time
	if (!paused && !insufficient)
		m_iLastPausePurge = time(NULL);
	theApp.downloadqueue->RemoveLocalServerRequest(this);

	if(GetKadFileSearchID())
	{
		Kademlia::CSearchManager::stopSearch(GetKadFileSearchID(), true);
		lastsearchtimeKad = 0; //If we were in the middle of searching, reset timer so they can resume searching.
	}

	SetActive(false);

	if (status==PS_COMPLETE || status==PS_COMPLETING)
		return;

	Packet* packet = new Packet(OP_CANCELTRANSFER,0);
	for( POSITION pos = srclist.GetHeadPosition(); pos != NULL; )
	{
		CUpDownClient* cur_src = srclist.GetNext(pos);
		if (cur_src->GetDownloadState() == DS_DOWNLOADING)
		{
			if (!cur_src->GetSentCancelTransfer())
			{
				theApp.uploadqueue->AddUpDataOverheadFileRequest(packet->size);
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					DebugSend("OP__CancelTransfer", cur_src);
				cur_src->socket->SendPacket(packet,false,true);
				cur_src->SetSentCancelTransfer(1);
			}
			cur_src->SetDownloadState(DS_ONQUEUE);
		}
	}
	delete packet;

	if (bInsufficient)
	{
		AddLogLine(false, _T("Insufficient diskspace - pausing download of \"%s\""), GetFileName());
		insufficient = true;
	}
	else
	{
		paused = true;
		insufficient = false;
	}
	SetStatus(status); // to update item
	datarate = 0;
	m_anStates[DS_DOWNLOADING] = 0; // -khaos--+++> Renamed var.
	if (!bInsufficient)
	{
		theApp.downloadqueue->SortByPriority();
		theApp.downloadqueue->CheckDiskspace(); // SLUGFILLER: checkDiskspace
		SavePartFile();
	}
	UpdateDisplayedInfo(true);
}

bool CPartFile::CanResumeFile() const
{
	return (GetStatus()==PS_PAUSED || GetStatus()==PS_INSUFFICIENT || (GetStatus()==PS_ERROR && GetCompletionError()));
}

void CPartFile::ResumeFile()
{
	if (status==PS_COMPLETE || status==PS_COMPLETING)
		return;
	if (status==PS_ERROR && m_bCompletionError){
		ASSERT( gaplist.IsEmpty() );
		if (gaplist.IsEmpty()){
			// rehashing the file could probably be avoided, but better be in the safe side..
			m_bCompletionError = false;
			CompleteFile(false);
		}
		return;
	}
	paused = false;
	stopped = false;
	SetActive(theApp.IsConnected());
	lastsearchtime = 0;
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace(); // SLUGFILLER: checkDiskspace
	SavePartFile();
	UpdateDisplayedInfo(true);
}

// SLUGFILLER: checkDiskspace
void CPartFile::ResumeFileInsufficient()
{
	if (status==PS_COMPLETE || status==PS_COMPLETING)
		return;
	if (!insufficient)
		return;
	AddLogLine(false, _T("Resuming download of \"%s\""), GetFileName());
	insufficient = false;
	SetActive(theApp.IsConnected());
	lastsearchtime = 0;
	UpdateDisplayedInfo(true);
}
// SLUGFILLER: checkDiskspace

CString CPartFile::getPartfileStatus() const
{
	switch(GetStatus()){
		case PS_HASHING:
		case PS_WAITINGFORHASH:
			return GetResString(IDS_HASHING);
		case PS_COMPLETING:
			return GetResString(IDS_COMPLETING);
		case PS_COMPLETE:
			return GetResString(IDS_COMPLETE);
		case PS_PAUSED:
			if (stopped)
				return GetResString(IDS_STOPPED);
			return GetResString(IDS_PAUSED);
		// SLUGFILLER: checkDiskspace
		case PS_INSUFFICIENT:
			return GetResString(IDS_INSUFFICIENT);
		// SLUGFILLER: checkDiskspace
		case PS_ERROR:
			if (m_bCompletionError)
				return GetResString(IDS_INSUFFICIENT);
			return GetResString(IDS_ERRORLIKE);
	}
	if(GetSrcStatisticsValue(DS_DOWNLOADING) > 0)
		return GetResString(IDS_DOWNLOADING);
	else
		return GetResString(IDS_WAITING);
} 

int CPartFile::getPartfileStatusRang() const
{
	int tempstatus=0;
	if (GetSrcStatisticsValue(DS_DOWNLOADING) == 0)
		tempstatus=1;
	switch (GetStatus()) {
		case PS_HASHING: 
		case PS_WAITINGFORHASH:
			tempstatus=3;
			break; 
		case PS_COMPLETING:
			tempstatus=4;
			break; 
		case PS_COMPLETE:
			tempstatus=5;
			break; 
		case PS_PAUSED:
		case PS_INSUFFICIENT:	// SLUGFILLER: checkDiskspace
			tempstatus=2;
			break; 
		case PS_ERROR:
			tempstatus=6;
			break;
	}

	return tempstatus;
} 

sint32 CPartFile::getTimeRemaining() const
{ 
	if (GetDatarate() == 0)
		return -1;
	return (GetFileSize() - GetCompletedSize()) / GetDatarate();
}

// khaos::accuratetimerem+
sint32 CPartFile::GetTimeRemainingAvg()
{
	uint32 nCompletedSince = GetCompletedSize() - m_nInitialBytes;
	uint32 nSecondsActive = m_nSecondsActive;

	if (m_dwActivatedTick > 0)
		nSecondsActive += (uint32) ((GetTickCount() - m_dwActivatedTick) / 1000);
	
	if (nCompletedSince > 0 && nSecondsActive > 0)
	{
		//MORPH - Modified by SiRoB, HotFix to avoid overflow.
		//double nDataRateAvg = (double)(nCompletedSince / nSecondsActive);
		double nDataRateAvg = (double)nCompletedSince / (double)nSecondsActive;
		return ( (sint32)((GetFileSize() - GetCompletedSize()) / nDataRateAvg) );
	}
	return -1;
}
// khaos::accuratetimerem-

void CPartFile::PreviewFile(){
	if (IsArchive(true)){
		if ((!m_bRecoveringArchive) && (!m_bPreviewing))
			CArchiveRecovery::recover(this, true);
		return;
	}

	if (!CanPreviewFile()){
		ASSERT( false );
		return;
	}

	if (thePrefs.IsMoviePreviewBackup()){
		m_bPreviewing = true;
		CPreviewThread* pThread = (CPreviewThread*) AfxBeginThread(RUNTIME_CLASS(CPreviewThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
		pThread->SetValues(this,thePrefs.GetVideoPlayer());
		pThread->ResumeThread();
	}
	else{
		CString strLine = GetFullName();
		
		// strip available ".met" extension to get the part file name.
		if (strLine.GetLength()>4 && strLine.Right(4)==_T(".met"))
			strLine.Delete(strLine.GetLength()-4,4);

		// if the path contains spaces, quote the entire path
		if (strLine.Find(_T(' ')) != -1)
			strLine = _T('\"') + strLine + _T('\"');

		if (!thePrefs.GetVideoPlayer().IsEmpty())
		{
			// get directory of video player application
			CString path = thePrefs.GetVideoPlayer();
			int pos = path.ReverseFind(_T('\\'));
			if (pos == -1)
				path.Empty();
			else
				path = path.Left(pos + 1);

			if (thePrefs.GetPreviewSmallBlocks())
				FlushBuffer(true);

			ShellExecute(NULL, _T("open"), thePrefs.GetVideoPlayer(), strLine, path, SW_SHOWNORMAL);
		}
		else
			ShellExecute(NULL, _T("open"), strLine, NULL, NULL, SW_SHOWNORMAL);
	}
}

bool CPartFile::CanPreviewFile() const
{
	// Barry - Allow preview of archives of any length > 1k
	if (IsArchive(true))
	{
		if (GetStatus() != PS_COMPLETE &&  GetStatus() != PS_COMPLETING && GetFileSize()>1024 && GetCompletedSize()>1024 && (!m_bRecoveringArchive) && ((GetFreeDiskSpaceX(thePrefs.GetTempDir()) + 100000000) > (2*GetFileSize())))
			return true; 
		else 
			return false;
	}
	//MORPH START - Added by SiRoB, preview music file
	if (IsMusic())
		if (GetStatus() != PS_COMPLETE &&  GetStatus() != PS_COMPLETING && GetFileSize()>1024 && GetCompletedSize()>1024 && ((GetFreeDiskSpaceX(thePrefs.GetTempDir()) + 100000000) > (2*GetFileSize())))
			return true;
	//MORPH END   - Added by SiRoB, preview music file
	
	if (thePrefs.IsMoviePreviewBackup())
	{
		//MORPH - Changed by SiRoB, Authorize preview of files with 2 chunk available
		return !( (GetStatus() != PS_READY && GetStatus() != PS_PAUSED) 
			|| m_bPreviewing || GetPartCount() < 2 || !IsMovie() || (GetFreeDiskSpaceX(thePrefs.GetTempDir()) + 100000000) < GetFileSize()
			|| ( !IsPartShareable(0) || !IsPartShareable(GetPartCount()-1) ) );		// SLUGFILLER: SafeHash - only play hashed parts
	}
	else
	{
		TCHAR szVideoPlayerFileName[_MAX_FNAME];
		_tsplitpath(thePrefs.GetVideoPlayer(), NULL, NULL, szVideoPlayerFileName, NULL);

		// enable the preview command if the according option is specified 'PreviewSmallBlocks' 
		// or if VideoLAN client is specified
		if (thePrefs.GetPreviewSmallBlocks() || !_tcsicmp(szVideoPlayerFileName, _T("vlc")))
		{
			if (m_bPreviewing)
				return false;

			uint8 uState = GetStatus();
			if (!(uState == PS_READY || uState == PS_EMPTY || uState == PS_PAUSED || uState == PS_INSUFFICIENT))
				return false;

			// default: check the ED2K file format to be of type audio, video or CD image. 
			// but because this could disable the preview command for some file types which eMule does not know,
			// this test can be avoided by specifying 'PreviewSmallBlocks=2'
			if (thePrefs.GetPreviewSmallBlocks() <= 1)
			{
				// check the file extension
				EED2KFileType eFileType = GetED2KFileTypeID(GetFileName());
				if (!(eFileType == ED2KFT_VIDEO || eFileType == ED2KFT_AUDIO || eFileType == ED2KFT_CDIMAGE))
				{
					// check the ED2K file type
					LPCSTR pszED2KFileType = GetStrTagValue(FT_FILETYPE);
					if (pszED2KFileType == NULL || !(!stricmp(pszED2KFileType, "Audio") || !stricmp(pszED2KFileType, "Video")))
						return false;
				}
			}

		    // If it's an MPEG file, VLC is even capable of showing parts of the file if the beginning of the file is missing!
			bool bMPEG = false;
			LPCTSTR pszExt = _tcsrchr(GetFileName(), _T('.'));
			if (pszExt != NULL){
				CString strExt(pszExt);
				strExt.MakeLower();
				bMPEG = (strExt==_T(".mpg") || strExt==_T(".mpeg") || strExt==_T(".mpe") || strExt==_T(".mp3") || strExt==_T(".mp2") || strExt==_T(".mpa"));
			}

			if (bMPEG){
			    // TODO: search a block which is at least 16K (Audio) or 256K (Video)
				if (GetCompletedSize() < 16*1024)
					return false;
			}
			else{
			    // For AVI files it depends on the used codec..
				if (!IsComplete(0, 256*1024))
					return false;
			}
	
			return true;
		}
		else{
			return !((GetStatus() != PS_READY && GetStatus() != PS_PAUSED) 
					|| m_bPreviewing || GetPartCount() < 2 || !IsMovie() || !IsPartShareable(0));	// SLUGFILLER: SafeHash - only play hashed parts
		}
	}
}

void CPartFile::UpdateAvailablePartsCount(){
	UINT availablecounter = 0;
	UINT iPartCount = GetPartCount();
	for (UINT ixPart = 0; ixPart < iPartCount; ixPart++){
		for(POSITION pos = srclist.GetHeadPosition(); pos; ){
			if (srclist.GetNext(pos)->IsPartAvailable(ixPart)){
				availablecounter++; 
				break;
			}
		}
	}
	if (iPartCount == availablecounter && availablePartsCount < iPartCount)
		lastseencomplete = CTime::GetCurrentTime();
	availablePartsCount = availablecounter;
}

Packet* CPartFile::CreateSrcInfoPacket(CUpDownClient* forClient) const
{
	//We need to find where the dangling pointers are before uncommenting this..
	if(!IsPartFile())
		return CKnownFile::CreateSrcInfoPacket(forClient);

	if (forClient->reqfile != this)
		return NULL;

	if ( !(GetStatus() == PS_READY || GetStatus() == PS_EMPTY))
		return NULL;

	if (srclist.IsEmpty())
		return NULL;

	CSafeMemFile data(1024);
	uint16 nCount = 0;

	data.WriteHash16(m_abyFileHash);
	data.WriteUInt16(nCount);
	bool bNeeded;
	for (POSITION pos = srclist.GetHeadPosition();pos != 0;){
		bNeeded = false;
		CUpDownClient* cur_src = srclist.GetNext(pos);
		if (cur_src->HasLowID() || !cur_src->IsValidSource())
			continue;
		uint8* srcstatus = cur_src->GetPartStatus();
		if (srcstatus){
			if (cur_src->GetPartCount() == GetPartCount()){
				uint8* reqstatus = forClient->GetPartStatus();
				if (reqstatus){
					// only send sources which have needed parts for this client
					for (int x = 0; x < GetPartCount(); x++){
						if (srcstatus[x] && !reqstatus[x]){
							bNeeded = true;
							break;
						}
					}
				}
				else{
					// We know this client is valid. But don't know the part count status.. So, currently we just send them.
					for (int x = 0; x < GetPartCount(); x++){
						if (srcstatus[x]){
							bNeeded = true;
							break;
						}
					}
				}
			}
			else{
				// should never happen
				if (thePrefs.GetVerbose())
					DEBUG_ONLY(AddDebugLogLine(false,"*** %s - found source (%s) with wrong partcount (%u) attached to partfile \"%s\" (partcount=%u)", __FUNCTION__, cur_src->DbgGetClientInfo(), cur_src->GetPartCount(), GetFileName(), GetPartCount()));
			}
		}
		if( bNeeded ){
			nCount++;
			uint32 dwID;
			if(forClient->GetSourceExchangeVersion() > 2)
				dwID = cur_src->GetUserIDHybrid();
			else
				dwID = ntohl(cur_src->GetUserIDHybrid());
			data.WriteUInt32(dwID);
			data.WriteUInt16(cur_src->GetUserPort());
			data.WriteUInt32(cur_src->GetServerIP());
			data.WriteUInt16(cur_src->GetServerPort());
			if (forClient->GetSourceExchangeVersion() > 1)
				data.WriteHash16(cur_src->GetUserHash());
			if (nCount > 500)
				break;
		}
	}
	if (!nCount)
		return 0;
	data.Seek(16, 0);
	data.WriteUInt16(nCount);

	Packet* result = new Packet(&data, OP_EMULEPROT);
	result->opcode = OP_ANSWERSOURCES;
	// 16+2+501*(4+2+4+2+16) = 14046 bytes max.
	if ( result->size > 354 )
		result->PackPacket();
	if ( thePrefs.GetDebugSourceExchange() )
		AddDebugLogLine( false, "Send:Source User(%s) File(%s) Count(%i)", forClient->GetUserName(), GetFileName(), nCount );
	return result;
}

void CPartFile::AddClientSources(CSafeMemFile* sources, uint8 sourceexchangeversion)
{
	if (stopped)
		return;

	UINT nCount = sources->ReadUInt16();

	// Check if the data size matches the 'nCount' for v1 or v2 and eventually correct the source
	// exchange version while reading the packet data. Otherwise we could experience a higher
	// chance in dealing with wrong source data, userhashs and finally duplicate sources.
	UINT uDataSize = sources->GetLength() - sources->GetPosition();
	//Checks if version 1 packet is correct size
	if (nCount*(4+2+4+2) == uDataSize)
	{
		if( sourceexchangeversion != 1 )
			return;
	}
	//Checks if version 2&3 packet is correct size
	else if (nCount*(4+2+4+2+16) == uDataSize)
	{
		if( sourceexchangeversion == 1 )
			return;
	}
	else
	{
		// If v4 inserts additional data (like v2), the above code will correctly filter those packets.
		// If v4 appends additional data after <count>(<Sources>)[count], we are in trouble with the 
		// above code. Though a client which does not understand v4+ should never receive such a packet.
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, "Received invalid source exchange packet (v%u) of data size %u for %s", sourceexchangeversion, uDataSize, GetFileName());
		return;
	}

	if ( thePrefs.GetDebugSourceExchange() )
		AddDebugLogLine( false, "RCV:Sources File(%s) Count(%i)", GetFileName(), nCount );

	for (UINT i = 0; i < nCount; i++)
	{
		uint32 dwID = sources->ReadUInt32();
		uint16 nPort = sources->ReadUInt16();
		uint32 dwServerIP = sources->ReadUInt32();
		uint16 nServerPort = sources->ReadUInt16();

		uchar achUserHash[16];
		if (sourceexchangeversion > 1)
			sources->ReadHash16(achUserHash);

		//Clients send ID's the the Hyrbid format so highID clients with *.*.*.0 won't be falsely switched to a lowID..
		if (sourceexchangeversion == 3)
		{
			uint32 dwIDED2K = ntohl(dwID);

			// check the HighID(IP) - "Filter LAN IPs" and "IPfilter" the received sources IP addresses
			if (!IsLowID(dwID))
			{
				if (!IsGoodIP(dwIDED2K))
				{
					// check for 0-IP, localhost and optionally for LAN addresses
					if (thePrefs.GetLogFilteredIPs())
						AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange - bad IP"), inet_ntoa(*(in_addr*)&dwIDED2K));
					continue;
				}
				if (theApp.ipfilter->IsFiltered(dwIDED2K))
				{
					if (thePrefs.GetLogFilteredIPs())
						AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange - IP filter (%s)"), inet_ntoa(*(in_addr*)&dwIDED2K), theApp.ipfilter->GetLastHit());
					continue;
				}
			}
	
			// additionally check for LowID and own IP
			if (!CanAddSource(dwID, nPort, dwServerIP, nServerPort, NULL, false))
			{
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange"), inet_ntoa(*(in_addr*)&dwIDED2K));
					continue;
			}
		}
		else
		{
			// check the HighID(IP) - "Filter LAN IPs" and "IPfilter" the received sources IP addresses
			if (!IsLowID(dwID))
			{
				if (!IsGoodIP(dwID))
				{ 
					// check for 0-IP, localhost and optionally for LAN addresses
					if (thePrefs.GetLogFilteredIPs())
						AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange - bad IP"), inet_ntoa(*(in_addr*)&dwID));
					continue;
				}
				if (theApp.ipfilter->IsFiltered(dwID))
				{
					if (thePrefs.GetLogFilteredIPs())
						AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange - IP filter (%s)"), inet_ntoa(*(in_addr*)&dwID), theApp.ipfilter->GetLastHit());
					continue;
				}
			}
	
			// additionally check for LowID and own IP
			if (!CanAddSource(dwID, nPort, dwServerIP, nServerPort))
			{
				if (thePrefs.GetLogFilteredIPs())
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange"), inet_ntoa(*(in_addr*)&dwID));
				continue;
			}
		}

		if (thePrefs.GetMaxSourcePerFile() > GetSourceCount())
		{
			CUpDownClient* newsource;
			if( sourceexchangeversion == 3 )
				newsource = new CUpDownClient(this,nPort,dwID,dwServerIP,nServerPort,false);
			else
				newsource = new CUpDownClient(this,nPort,dwID,dwServerIP,nServerPort,true);
			if (sourceexchangeversion > 1)
				newsource->SetUserHash(achUserHash);
			newsource->SetSourceFrom(SF_SOURCE_EXCHANGE);
			theApp.downloadqueue->CheckAndAddSource(this,newsource);
		} 
		else
			break;
	}
}

// making this function return a higher when more sources have the extended
// protocol will force you to ask a larger variety of people for sources
/*int CPartFile::GetCommonFilePenalty() const
{
	//TODO: implement, but never return less than MINCOMMONPENALTY!
	return MINCOMMONPENALTY;
}
*/
/* Barry - Replaces BlockReceived() 

		Originally this only wrote to disk when a full 180k block 
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
uint32 CPartFile::WriteToBuffer(uint32 transize, BYTE *data, uint32 start, uint32 end, Requested_Block_Struct *block)
{
	ASSERT( (int)transize > 0 );

	// Increment transfered bytes counter for this file
	transfered += transize;

	// This is needed a few times
	uint32 lenData = end - start + 1;
	ASSERT( (int)lenData > 0 );

	// -khaos--+++>
	if( lenData > transize ) {
		m_iSesCompressionBytes += lenData-transize;
		thePrefs.Add2SavedFromCompression(lenData-transize);
	}
	// <-----khaos-

	// Occasionally packets are duplicated, no point writing it twice
	if (IsComplete(start, end))
	{
		if (thePrefs.GetVerbose())
			AddDebugLogLine(false, "File \"%s\" has already been written from %lu to %lu, %u bytes, part %u-%u", GetFileName(), start, end, lenData, start/PARTSIZE, end/PARTSIZE);
		return 0;
	}

	// SLUGFILLER: SafeHash
	CSingleLock sLock(&ICH_mut,true);	// Wait for ICH result
	PharseICHResult();	// Check result to prevent post-complete writing
	lenData = 0;	// this one is an effective counter

	// only write to gaps
	for (POSITION pos1 = gaplist.GetHeadPosition();pos1 != NULL;){
		Gap_Struct* cur_gap = gaplist.GetNext(pos1);

		if (start > cur_gap->end || end < cur_gap->start)
			continue;

		// Create a new buffered queue entry
		PartFileBufferedData *item = new PartFileBufferedData;
		item->start = (start > cur_gap->start)?start:cur_gap->start;
		item->end = (end < cur_gap->end)?end:cur_gap->end;
		item->block = block;

		uint32 lenDataClipped = item->end - item->start + 1;

		// Create copy of data as new buffer
		BYTE *buffer = new BYTE[lenDataClipped];
		memcpy(buffer, data+(item->start-start), lenDataClipped);
		item->data = buffer;
	// SLUGFILLER: SafeHash

		// Add to the queue in the correct position (most likely the end)
		PartFileBufferedData *queueItem;
		bool added = false;
		POSITION pos = m_BufferedData_list.GetTailPosition();
		while (pos != NULL)
		{	
		POSITION posLast = pos;
			queueItem = m_BufferedData_list.GetPrev(pos);
			if (item->end > queueItem->end)
			{
				added = true;
			m_BufferedData_list.InsertAfter(posLast, item);
				break;
			}
		}
		if (!added)
			m_BufferedData_list.AddHead(item);

	// SLUGFILLER: SafeHash
		lenData += lenDataClipped;	// calculate actual added data
	}
	// SLUGFILLER: SafeHash

	// Increment buffer size marker
	m_nTotalBufferData += lenData;

	// Mark this small section of the file as filled
	FillGap(start, end);	// SLUGFILLER: SafeHash - clean coding, removed "item->"

	// Update the flushed mark on the requested block 
	// The loop here is unfortunate but necessary to detect deleted blocks.
	POSITION pos = requestedblocks_list.GetHeadPosition();	// SLUGFILLER: SafeHash
	while (pos != NULL)
	{	
		// SLUGFILLER: SafeHash - clean coding, removed "item->"
		if (requestedblocks_list.GetNext(pos) == block)
			block->transferred += lenData;
		// SLUGFILLER: SafeHash
	}

	if (gaplist.IsEmpty())
		FlushBuffer(true);

	// Return the length of data written to the buffer
	return lenData;
}

void CPartFile::FlushBuffer(bool forcewait)
{
	bool bIncreasedFile=false;

	m_nLastBufferFlushTime = GetTickCount();
	if (m_BufferedData_list.IsEmpty())
		return;

	if (m_AllocateThread!=NULL) {
		// diskspace is being allocated right now.
		// so dont write and keep the data in the buffer for later.
		return;
	}else if (m_iAllocinfo>0) {
		bIncreasedFile=true;
		m_iAllocinfo=0;
	}

	//if (thePrefs.GetVerbose())
//	AddDebugLogLine(false, "Flushing file %s - buffer size = %ld bytes (%ld queued items) transfered = %ld [time = %ld]\n", GetFileName(), m_nTotalBufferData, m_BufferedData_list.GetCount(), transfered, m_nLastBufferFlushTime);

	uint32 partCount = GetPartCount();
	bool *changedPart = new bool[partCount];
	// Remember which parts need to be checked at the end of the flush
	for (int partNumber=0; (uint32)partNumber<partCount; partNumber++)
		changedPart[partNumber] = false;

	try
	{
		bool bCheckDiskspace = thePrefs.IsCheckDiskspaceEnabled() && thePrefs.GetMinFreeDiskSpace() > 0;
		ULONGLONG uFreeDiskSpace = bCheckDiskspace ? GetFreeDiskSpaceX(thePrefs.GetTempDir()) : 0;

		// Check free diskspace for compressed/sparse files before possibly increasing the file size
		if (bCheckDiskspace && !IsNormalFile())
		{
			// Compressed/sparse files; regardless whether the file is increased in size, 
			// check the amount of data which will be written
			// would need to use disk cluster sizes for more accuracy
			if (m_nTotalBufferData + thePrefs.GetMinFreeDiskSpace() >= uFreeDiskSpace)
				AfxThrowFileException(CFileException::diskFull, 0, m_hpartfile.GetFileName());
		}

		// SLUGFILLER: SafeHash
		CSingleLock sLock(&ICH_mut,true);	// ICH locks the file - otherwise it may be written to while being checked
		PharseICHResult();	// Check result from ICH
		// SLUGFILLER: SafeHash

		// Ensure file is big enough to write data to (the last item will be the furthest from the start)
		PartFileBufferedData *item = m_BufferedData_list.GetTail();
		if (m_hpartfile.GetLength() <= item->end)
		{
			// Check free diskspace for normal files before increasing the file size
			if (bCheckDiskspace && IsNormalFile())
			{
				// Normal files; check if increasing the file would reduce the amount of min. free space beyond the limit
				// would need to use disk cluster sizes for more accuracy
				ULONGLONG uIncrease = item->end + 1 - m_hpartfile.GetLength();
				if (uIncrease + thePrefs.GetMinFreeDiskSpace() >= uFreeDiskSpace)
					AfxThrowFileException(CFileException::diskFull, 0, m_hpartfile.GetFileName());
			}

			if (!IsNormalFile() || item->end-m_hpartfile.GetLength() < 2097152) 
				forcewait=true;	// <2MB -> alloc it at once

			// Allocate filesize
			if (!forcewait) {
				m_AllocateThread= AfxBeginThread(AllocateSpaceThread, this, THREAD_PRIORITY_LOWEST, 0, CREATE_SUSPENDED);
				if (m_AllocateThread == NULL)
				{
					TRACE(_T("Failed to create alloc thread! -> allocate blocking\n"));
					forcewait=true;
				} else {
					m_iAllocinfo=item->end+1;
					m_AllocateThread->ResumeThread();
					delete[] changedPart;
					return;
				}
			}
			
			if (forcewait) {
				bIncreasedFile=true;
				// If this is a NTFS compressed file and the current block is the 1st one to be written and there is not 
				// enough free disk space to hold the entire *uncompressed* file, windows throws a 'diskFull'!?
				if (IsNormalFile())
					m_hpartfile.SetLength(item->end + 1); // allocate disk space (may throw 'diskFull')
			}
		}

		// Loop through queue
		for (int i = m_BufferedData_list.GetCount(); i>0; i--)
		{
			// Get top item
			item = m_BufferedData_list.GetHead();

			// This is needed a few times
			uint32 lenData = item->end - item->start + 1;

			// SLUGFILLER: SafeHash - could be more than one part
			for (uint32 curpart = item->start/PARTSIZE; curpart <= item->end/PARTSIZE; curpart++)
				changedPart[curpart] = true;
			// SLUGFILLER: SafeHash

			// Go to the correct position in file and write block of data
			m_hpartfile.Seek(item->start, CFile::begin);
			m_hpartfile.Write(item->data, lenData);

			// Remove item from queue
			m_BufferedData_list.RemoveHead();

			// Decrease buffer size
			m_nTotalBufferData -= lenData;

			// Release memory used by this item
			delete [] item->data;
			delete item;
		}

		// Partfile should never be too large
 		if (m_hpartfile.GetLength() > m_nFileSize){
			// it's "last chance" correction. the real bugfix has to be applied 'somewhere' else
			TRACE("Partfile \"%s\" is too large! Truncating %I64u bytes.\n", GetFileName(), m_hpartfile.GetLength() - m_nFileSize);
			m_hpartfile.SetLength(m_nFileSize);
		}

		// Flush to disk
		m_hpartfile.Flush();

		// SLUGFILLER: SafeHash
		// Only if hashlist is available
		if (hashlist.GetCount() == GetED2KPartCount()){
			// Check each part of the file
			for (int partNumber = partCount-1; partNumber >= 0; partNumber--)
			{
				if (!changedPart[partNumber])
					continue;

				if (!GetPartHash(partNumber)) {
					AddLogLine(true,GetResString(IDS_ERR_INCOMPLETEHASH),GetFileName());
					hashsetneeded = true;
					ASSERT(FALSE);	// If this fails, something was seriously wrong with the hashset loading or the check above
				}

				// Is this 9MB part complete
				if (IsComplete(PARTSIZE * partNumber, (PARTSIZE * (partNumber + 1)) - 1))
				{
					// Is part corrupt
					// Let's check in another thread
					m_PartsHashing++;
					CPartHashThread* parthashthread = (CPartHashThread*) AfxBeginThread(RUNTIME_CLASS(CPartHashThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
					parthashthread->SetSinglePartHash(this, partNumber, false);
					parthashthread->ResumeThread();
				}
				else if (IsCorruptedPart(partNumber) && thePrefs.IsICHEnabled())
				{
					// Try to recover with minimal loss
					// But in another thread
					CPartHashThread* parthashthread = (CPartHashThread*) AfxBeginThread(RUNTIME_CLASS(CPartHashThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
					parthashthread->SetSinglePartHash(this, partNumber, true);	// Special case, doesn't increment hashing parts, since part isn't really complete
					parthashthread->ResumeThread();
				}
			}
		}
		else if (!hashsetneeded) {	// FIXME: Can we get here without thinking we have a hashset? Can this ever trigger?
			ASSERT(GetED2KPartCount() > 1);	// Files with only 1 chunk should have a forced hashset
			AddLogLine(true,GetResString(IDS_ERR_HASHERRORWARNING),GetFileName());
			hashsetneeded = true;
		}
		// SLUGFILLER: SafeHash

		// Update met file
		SavePartFile();

		if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
		{
			// Is this file finished?

			// SLUGFILLER: SafeHash remove - Don't perform file completion here

			// Check free diskspace
			//
			// Checking the free disk space again after the file was written could most likely be avoided, but because
			// we do not use real physical disk allocation units for the free disk computations, it should be more safe
			// and accurate to check the free disk space again, after file was written and buffers were flushed to disk.
			//
			// If useing a normal file, we could avoid the check disk space if the file was not increased.
			// If useing a compressed or sparse file, we always have to check the space 
			// regardless whether the file was increased in size or not.
			if (bCheckDiskspace && ((IsNormalFile() && bIncreasedFile) || !IsNormalFile()))
			{
				switch(GetStatus())
				{
				case PS_PAUSED:
				case PS_ERROR:
				case PS_COMPLETING:
				case PS_COMPLETE:
					break;
				default:
					if (GetFreeDiskSpaceX(thePrefs.GetTempDir()) < thePrefs.GetMinFreeDiskSpace())
					{
						if (IsNormalFile())
						{
							// Normal files: pause the file only if it would still grow
							uint32 nSpaceToGrow = GetNeededSpace();
							if (nSpaceToGrow)
								PauseFile(true/*bInsufficient*/);
						}
						else
						{
							// Compressed/sparse files: always pause the file
							PauseFile(true/*bInsufficient*/);
						}
					}
				}
			}
		}
	}
	catch (CFileException* error)
	{
		FlushBuffersExceptionHandler(error);	
	}
	catch(...)
	{
		FlushBuffersExceptionHandler();
	}
	delete[] changedPart;
}

void CPartFile::FlushBuffersExceptionHandler(CFileException* error)
{
	if (thePrefs.IsCheckDiskspaceEnabled() && error->m_cause == CFileException::diskFull)
	{
		AddLogLine(true, GetResString(IDS_ERR_OUTOFSPACE), GetFileName());
		if (theApp.emuledlg->IsRunning() && thePrefs.GetNotifierPopOnImportantError()){
			CString msg;
			msg.Format(GetResString(IDS_ERR_OUTOFSPACE), GetFileName());
			theApp.emuledlg->ShowNotifier(msg, TBN_IMPORTANTEVENT);
		}

			// 'CFileException::diskFull' is also used for 'not enough min. free space'
			if (theApp.emuledlg->IsRunning())
			{
				if (thePrefs.IsCheckDiskspaceEnabled() && thePrefs.GetMinFreeDiskSpace()==0)
					theApp.downloadqueue->CheckDiskspace(true);
				else
					PauseFile(true/*bInsufficient*/);
			}
		}
	else
	{
		if (thePrefs.IsErrorBeepEnabled())
			Beep(800,200);

		if (error->m_cause == CFileException::diskFull) 
		{
			AddLogLine(true, GetResString(IDS_ERR_OUTOFSPACE), GetFileName());
			// may be called during shutdown!
			if (theApp.emuledlg->IsRunning() && thePrefs.GetNotifierPopOnImportantError()){
				CString msg;
				msg.Format(GetResString(IDS_ERR_OUTOFSPACE), GetFileName());
				theApp.emuledlg->ShowNotifier(msg, TBN_IMPORTANTEVENT);
			}
		}
		else
		{
			char buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,MAX_CFEXP_ERRORMSG);
			AddLogLine(true, GetResString(IDS_ERR_WRITEERROR), GetFileName(), buffer);
			SetStatus(PS_ERROR);
		}
		paused = true;
		m_iLastPausePurge = time(NULL);
		theApp.downloadqueue->RemoveLocalServerRequest(this);
		datarate = 0;
		m_anStates[DS_DOWNLOADING] = 0;
	}
	
	if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
		UpdateDisplayedInfo();

	error->Delete();
}

void CPartFile::FlushBuffersExceptionHandler()
	{
		ASSERT(0);
		AddLogLine(true, GetResString(IDS_ERR_WRITEERROR), GetFileName(), GetResString(IDS_UNKNOWN));
		SetStatus(PS_ERROR);
		paused = true;
		m_iLastPausePurge = time(NULL);
		theApp.downloadqueue->RemoveLocalServerRequest(this);
		datarate = 0;
		m_anStates[DS_DOWNLOADING] = 0;
		if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
			UpdateDisplayedInfo();
	}

UINT AFX_CDECL CPartFile::AllocateSpaceThread(LPVOID lpParam)
{
	DbgSetThreadName("Partfile-Allocate Space");

	CPartFile* myfile=(CPartFile*)lpParam;
	theApp.QueueDebugLogLine(false,"ALLOC:Start (%s) (%s)",myfile->GetFileName(), CastItoXBytes(myfile->m_iAllocinfo) );

	try{
		// If this is a NTFS compressed file and the current block is the 1st one to be written and there is not 
		// enough free disk space to hold the entire *uncompressed* file, windows throws a 'diskFull'!?
		myfile->m_hpartfile.SetLength(myfile->m_iAllocinfo); // allocate disk space (may throw 'diskFull')

		// force the alloc, by temporary writing a non zero to the fileend
		byte x=255;
		myfile->m_hpartfile.Seek(-1,CFile::end);
		myfile->m_hpartfile.Write(&x,1);
		myfile->m_hpartfile.Flush();
		x=0;
		myfile->m_hpartfile.Seek(-1,CFile::end);
		myfile->m_hpartfile.Write(&x,0);
		myfile->m_hpartfile.Flush();
	}
	catch (CFileException* error)
	{
		VERIFY( PostMessage(theApp.emuledlg->m_hWnd,TM_FILEALLOCEXC,(WPARAM)myfile,(LPARAM)error) );
		myfile->m_AllocateThread=NULL;

		return 1;
	}
	catch(...)
	{

		VERIFY( PostMessage(theApp.emuledlg->m_hWnd,TM_FILEALLOCEXC,(WPARAM)myfile,0) );
		myfile->m_AllocateThread=NULL;
		return 2;
	}

	myfile->m_AllocateThread=NULL;
	theApp.QueueDebugLogLine(false,"ALLOC:End (%s)",myfile->GetFileName());
	return 0;
}

// Barry - This will invert the gap list, up to caller to delete gaps when done
// 'Gaps' returned are really the filled areas, and guaranteed to be in order
void CPartFile::GetFilledList(CTypedPtrList<CPtrList, Gap_Struct*> *filled) const
{
	Gap_Struct *gap;
	Gap_Struct *best=NULL;
	POSITION pos;
	uint32 start = 0;
	uint32 bestEnd = 0;

	// Loop until done
	bool finished = false;
	while (!finished)
	{
		finished = true;
		// Find first gap after current start pos
		bestEnd = m_nFileSize;
		pos = gaplist.GetHeadPosition();
		while (pos != NULL)
		{
			gap = gaplist.GetNext(pos);
			if ((gap->start > start) && (gap->end < bestEnd))
			{
				best = gap;
				bestEnd = best->end;
				finished = false;
			}
		}

		// TODO: here we have a problem - it occured that eMule crashed because of "best==NULL" while
		// recovering an archive which was currently in "completing" state...
		if (best==NULL){
			ASSERT(0);
			return;
		}

		if (!finished)
		{
			// Invert this gap
			gap = new Gap_Struct;
			gap->start = start;
			gap->end = best->start - 1;
			start = best->end + 1;
			filled->AddTail(gap);
		}
		else if (best->end < m_nFileSize)
		{
			gap = new Gap_Struct;
			gap->start = best->end + 1;
			gap->end = m_nFileSize;
			filled->AddTail(gap);
		}
	}
}

void CPartFile::UpdateFileRatingCommentAvail()
{
	if (!this)
		return;

	bool prev=(hasComment || hasRating);
	bool prevbad=hasBadRating;

	hasComment=false;
	int badratings=0;
	int ratings=0;

	for (POSITION pos = srclist.GetHeadPosition(); pos != NULL;)
	{
		CUpDownClient* cur_src = srclist.GetNext(pos);
		if (cur_src->GetFileComment().GetLength() > 0)
			hasComment = true;
		if (cur_src->GetFileRate() > 0)
			ratings++;
		if (cur_src->GetFileRate() == 1)
			badratings++;
	}
	hasBadRating = (badratings > (ratings/3));
	hasRating = (ratings > 0);
	if ((prev != (hasComment || hasRating)) || (prevbad != hasBadRating))
		UpdateDisplayedInfo(true);
}

void CPartFile::UpdateDisplayedInfo(boolean force)
{
	if (theApp.emuledlg->IsRunning()){
		DWORD curTick = ::GetTickCount();

		if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+(uint32)(rand()/(RAND_MAX/1000))) {
			theApp.emuledlg->transferwnd->downloadlistctrl.UpdateItem(this);
			m_lastRefreshedDLDisplay = curTick;
		}
	}
}

// khaos::kmod+ Make these settings for auto-priority customizable in preferences.
//				For now, made them dynamic; because ADP should be relative given
//				what it is currently used for.
void CPartFile::UpdateAutoDownPriority()
{
	// Small optimization: Do not bother with all this if file is paused or stopped.
	if( !IsAutoDownPriority() || GetStatus()==PS_PAUSED || IsStopped() )
		return;
	uint16 nHighestSC = theApp.downloadqueue->GetHighestAvailableSourceCount();
	if ( GetAvailableSrcCount() > ((int)(nHighestSC * .40)) ) {
		if( GetDownPriority() != PR_LOW )
			SetDownPriority( PR_LOW );
		return;
	}
	if ( GetAvailableSrcCount() > ((int)(nHighestSC * .20)) ) {
		if( GetDownPriority() != PR_NORMAL )
			SetDownPriority( PR_NORMAL );
		return;
	}
	if( GetDownPriority() != PR_HIGH )
		SetDownPriority( PR_HIGH );
}
// khaos::kmod-

uint8 CPartFile::GetCategory() const
{
	//MORPH - Changed by SiRoB
	/*
	if (m_category > thePrefs.GetCatCount() - 1)
		m_category = 0;
	*/
	return m_category > thePrefs.GetCatCount() - 1?0:m_category;
}


// Ornis: Creating progressive presentation of the partfilestatuses - for webdisplay
CString CPartFile::GetProgressString(uint16 size) const
{
	char crProgress = '0';//green
	char crHave = '1';	// black
	char crPending='2';	// yellow
	char crMissing='3';  // red
	
	char crWaiting[6];
	crWaiting[0]='4'; // blue few source
	crWaiting[1]='5';
	crWaiting[2]='6';
	crWaiting[3]='7';
	crWaiting[4]='8';
	crWaiting[5]='9'; // full sources

	CString my_ChunkBar="";
	for (uint16 i=0;i<=size+1;i++) my_ChunkBar.AppendChar(crHave);	// one more for safety

	float unit= (float)size/(float)m_nFileSize;
	uint32 allgaps = 0;

	if(GetStatus() == PS_COMPLETE || GetStatus() == PS_COMPLETING) {
		CharFillRange(&my_ChunkBar,0,(uint32)(m_nFileSize*unit), crProgress);
	} else

	// red gaps
	for (POSITION pos = gaplist.GetHeadPosition();pos !=  0;){
		Gap_Struct* cur_gap = gaplist.GetNext(pos);
		allgaps += cur_gap->end - cur_gap->start;
		bool gapdone = false;
		uint32 gapstart = cur_gap->start;
		uint32 gapend = cur_gap->end;
		for (uint32 i = 0; i < GetPartCount(); i++){
			if (gapstart >= i*PARTSIZE && gapstart <=  (i+1)*PARTSIZE){ // is in this part?
				if (gapend <= (i+1)*PARTSIZE)
					gapdone = true;
				else{
					gapend = (i+1)*PARTSIZE; // and next part
				}
				// paint
				uint8 color;
				if (m_SrcpartFrequency.GetCount() >= (INT_PTR)i && m_SrcpartFrequency[(uint16)i])  // frequency?
					//color = crWaiting;
					color = m_SrcpartFrequency[(uint16)i] <  10 ? crWaiting[m_SrcpartFrequency[(uint16)i]/2]:crWaiting[5];
				else
					color = crMissing;

				CharFillRange(&my_ChunkBar,(uint32)(gapstart*unit), (uint32)(gapend*unit + 1),  color);

				if (gapdone) // finished?
					break;
				else{
					gapstart = gapend;
					gapend = cur_gap->end;
				}
			}
		}
	}

	// yellow pending parts
	for (POSITION pos = requestedblocks_list.GetHeadPosition();pos !=  0;){
		Requested_Block_Struct* block =  requestedblocks_list.GetNext(pos);
		CharFillRange(&my_ChunkBar, (uint32)((block->StartOffset + block->transferred)*unit), (uint32)(block->EndOffset*unit),  crPending);
	}

	return my_ChunkBar;
}

void CPartFile::CharFillRange(CString* buffer,uint32 start, uint32 end, char color) const
{
	for (uint32 i=start;i<=end;i++)
		buffer->SetAt(i,color);
}

void CPartFile::SetCategory(uint8 cat, bool setprio)
{
	m_category=cat;
	
	// set new prio
	if (setprio && IsPartFile()){
		switch (thePrefs.GetCategory(GetCategory())->prio) {
		case 0:
			break;
		case 1:
			SetAutoDownPriority(false); 
			SetDownPriority(PR_LOW);
			break;
		case 2:
			SetAutoDownPriority(false);
			SetDownPriority(PR_NORMAL);
			break;
		case 3:
			SetAutoDownPriority(false);
			SetDownPriority(PR_HIGH);
			break;
		case 4:
			SetAutoDownPriority(true);
			SetDownPriority(PR_HIGH);
			break;
		}
		SavePartFile();
	}
}

void CPartFile::SetStatus(uint8 in)
{
	ASSERT( in != PS_PAUSED && in != PS_INSUFFICIENT );

	status=in;
	if (theApp.emuledlg->IsRunning()){
		// khaos::accuratetimerem+
		switch (status)
		{
			case PS_READY:
			case PS_EMPTY:
				if (m_dwActivatedTick == 0)
					m_dwActivatedTick = GetTickCount();
				break;
			default:
				if (m_dwActivatedTick != 0)
				{
					m_nSecondsActive += (uint32) ((GetTickCount() - m_dwActivatedTick) / 1000);
					m_dwActivatedTick = 0;
				}
				break;
		}
		// khaos::accuratetimerem-

		//MORPH START - Changed by SiRoB, Khaos Categorie
		/*
		if (theApp.emuledlg->transferwnd->downloadlistctrl.curTab==0)
			theApp.emuledlg->transferwnd->downloadlistctrl.ChangeCategory(0);
		//else
			UpdateDisplayedInfo(true);
		*/
		theApp.emuledlg->transferwnd->downloadlistctrl.ChangeCategory(theApp.emuledlg->transferwnd->GetActiveCategory());
		//MORPH END - Changed by SiRoB, Khaos Categorie
		
		if (thePrefs.ShowCatTabInfos())
			theApp.emuledlg->transferwnd->UpdateCatTabTitles();
	}
}

uint64 CPartFile::GetRealFileSize() const
{
	return ::GetDiskFileSize(GetFilePath());
}

uint8* CPartFile::MMCreatePartStatus(){
	// create partstatus + info in mobilemule protocol specs
	// result needs to be deleted[] | slow, but not timecritical
	uint8* result = new uint8[GetPartCount()+1];
	for (int i = 0; i != GetPartCount(); i++){
		result[i] = 0;
		if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
			result[i] = 1;
			continue;
		}
		else{
			if (IsComplete(i*PARTSIZE + (0*(PARTSIZE/3)), ((i*PARTSIZE)+(1*(PARTSIZE/3)))-1))
				result[i] += 2;
			if (IsComplete(i*PARTSIZE+ (1*(PARTSIZE/3)), ((i*PARTSIZE)+(2*(PARTSIZE/3)))-1))
				result[i] += 4;
			if (IsComplete(i*PARTSIZE+ (2*(PARTSIZE/3)), ((i*PARTSIZE)+(3*(PARTSIZE/3)))-1))
				result[i] += 8;
			uint8 freq = (uint8)m_SrcpartFrequency[i];
			if (freq > 44)
				freq = 44;
			freq = ceilf((float)freq/3);
			freq <<= 4;
			result[i] += freq;
		}

	}
	return result;
};

uint16 CPartFile::GetSrcStatisticsValue(EDownloadState nDLState) const
{
	ASSERT( nDLState < ARRSIZE(m_anStates) );
	return m_anStates[nDLState];
}

uint16 CPartFile::GetTransferingSrcCount() const
{
	return GetSrcStatisticsValue(DS_DOWNLOADING);
}

// [Maella -Enhanced Chunk Selection- (based on jicxicmic)]

#pragma pack(1)
struct Chunk {
	uint16 part;			// Index of the chunk
	union {
		uint16 frequency;	// Availability of the chunk
		uint16 rank;		// Download priority factor (highest = 0, lowest = 0xffff)
	};
};
#pragma pack()

bool CPartFile::GetNextRequestedBlock(CUpDownClient* sender, 
									Requested_Block_Struct** newblocks, 
									  uint16* count) /*const*/
{
	// The purpose of this function is to return a list of blocks (~180KB) to
	// download. To avoid a prematurely stop of the downloading, all blocks that 
	// are requested from the same source must be located within the same 
	// chunk (=> part ~9MB).
	//  
	// The selection of the chunk to download is one of the CRITICAL parts of the 
	// edonkey network. The selection algorithm must insure the best spreading
	// of files.
	//  
	// The selection is based on 4 criteria:
	//  1.  Frequency of the chunk (availability), very rare chunks must be downloaded 
	//      as quickly as possible to become a new available source.
	//  2.  Parts used for preview (first + last chunk), preview or check a 
	//      file (e.g. movie, mp3)
	//  3.  Request state (downloading in process), try to ask each source for another 
	//      chunk. Spread the requests between all sources.
	//  4.  Completion (shortest-to-complete), partially retrieved chunks should be 
	//      completed before starting to download other one.
	//  
	// The frequency criterion defines three zones: very rare (<10%), rare (<50%)
	// and common (>30%). Inside each zone, the criteria have a specific ‘weight? used 
	// to calculate the priority of chunks. The chunk(s) with the highest 
	// priority (highest=0, lowest=0xffff) is/are selected first.
	//  
	//          very rare   (preview)       rare                      common
	//    0% <---- +0 pt ----> 10% <----- +10000 pt -----> 50% <---- +20000 pt ----> 100%
	// 1.  <------- frequency: +25*frequency pt ----------->
	// 2.  <- preview: +1 pt --><-------------- preview: set to 10000 pt ------------->
	// 3.                       <------ request: download in progress +20000 pt ------>
	// 4a. <- completion: 0% +100, 25% +75 .. 100% +0 pt --><-- !req => completion --->
	// 4b.                                                  <--- req => !completion -->
	//  
	// Unrolled, the priority scale is:
	//  
	// 0..xxxx       unrequested and requested very rare chunks
	// 10000..1xxxx  unrequested rare chunks + unrequested preview chunks
	// 20000..2xxxx  unrequested common chunks (priority to the most complete)
	// 30000..3xxxx  requested rare chunks + requested preview chunks
	// 40000..4xxxx  requested common chunks (priority to the least complete)
	//
	// This algorithm usually selects first the rarest chunk(s). However, partially
	// complete chunk(s) that is/are close to completion may overtake the priority 
	// (priority inversion).
	// For the common chuncks, the algorithm tries to spread the dowload between
	// the sources
	//

	// Check input parameters
	if(count == 0)
		return false;
	if(sender->GetPartStatus() == NULL)
		return false;

	// Define and create the list of the chunks to download
	const uint16 partCount = GetPartCount();
	CList<Chunk> chunksList(partCount);

	// Main loop
	uint16 newBlockCount = 0;
	while(newBlockCount != *count){
		// Create a request block stucture if a chunk has been previously selected
		if(sender->m_lastPartAsked != 0xffff){
			Requested_Block_Struct* pBlock = new Requested_Block_Struct;
			if(GetNextEmptyBlockInPart(sender->m_lastPartAsked, pBlock) == true){
				// Keep a track of all pending requested blocks
				requestedblocks_list.AddTail(pBlock);
				// Update list of blocks to return
				newblocks[newBlockCount++] = pBlock;
				// Skip end of loop (=> CPU load)
				continue;
			} 
			else {
				// All blocks for this chunk have been already requested
				delete pBlock;
				// => Try to select another chunk
				sender->m_lastPartAsked = 0xffff;
			}
		}

		// Check if a new chunk must be selected (e.g. download starting, previous chunk complete)
		if(sender->m_lastPartAsked == 0xffff){

			// Quantify all chunks (create list of chunks to download) 
			// This is done only one time and only if it is necessary (=> CPU load)
			if(chunksList.IsEmpty() == TRUE){
				// Indentify the locally missing part(s) that this source has
				for(uint16 i=0; i < partCount; i++){
					if(sender->IsPartAvailable(i) == true && GetNextEmptyBlockInPart(i, NULL) == true){
						// Create a new entry for this chunk and add it to the list
						Chunk newEntry;
						newEntry.part = i;
						newEntry.frequency = m_SrcpartFrequency[i];
						chunksList.AddTail(newEntry);
					}
				}

				// Check if any bloks(s) could be downloaded
				if(chunksList.IsEmpty() == TRUE){
					break; // Exit main loop while()
				}

				// Define the bounds of the three zones (very rare, rare)
				// more depending on available sources
				uint8 modif=10;
				if (GetSourceCount()>800) modif=2; 	else if (GetSourceCount()>200) modif=5;//Morph - corrected by AndCycle
				uint16 limit= modif*GetSourceCount()/ 100;
				if (limit==0) limit=1;

				const uint16 veryRareBound = limit;
				const uint16 rareBound = 2*limit;

    			// Cache Preview state (Criterion 2)
    			const bool isPreviewEnable = thePrefs.GetPreviewPrio() && 
           									(IsArchive() || IsMovie() || IsMusic()); //MORPH - Added by IceCream, preview also music file

				// Collect and calculate criteria for all chunks
				for(POSITION pos = chunksList.GetHeadPosition(); pos != NULL; ){
					Chunk& cur_chunk = chunksList.GetNext(pos);

					// Offsets of chunk
					const uint32 uStart = cur_chunk.part * PARTSIZE;
					const uint32 uEnd  = ((GetFileSize() - 1) < (uStart + PARTSIZE - 1)) ? 
										(GetFileSize() - 1) : (uStart + PARTSIZE - 1);

					// Criterion 2. Parts used for preview
					// Remark: - We need to download the first part and the last part(s).
					//        - When the last part is very small, it's necessary to 
					//          download the two last parts.
					bool critPreview = false;
					if(isPreviewEnable == true){
						if(cur_chunk.part == 0){
							critPreview = true; // First chunk
						}
						else if(cur_chunk.part == partCount-1){
							critPreview = true; // Last chunk 
						}
						else if(cur_chunk.part == partCount-2){
							// Last chunk - 1 (only if last chunk is too small)
							const uint32 sizeOfLastChunk = GetFileSize() - uEnd;
							if(sizeOfLastChunk < PARTSIZE/3){
								critPreview = true; // Last chunk - 1
							}
						}
					}

					// Criterion 3. Request state (downloading in process from other source(s))
					const bool critRequested = cur_chunk.frequency > veryRareBound &&  // => CPU load
											IsAlreadyRequested(uStart, uEnd);

					// Criterion 4. Completion
					uint32 partSize = PARTSIZE;
					for(POSITION pos = gaplist.GetHeadPosition(); pos != NULL; ) {
						const Gap_Struct* cur_gap = gaplist.GetNext(pos);
						// Check if Gap is into the limit
						if(cur_gap->start < uStart) {
							if(cur_gap->end > uStart && cur_gap->end < uEnd) {
								partSize -= cur_gap->end - uStart + 1;
							}
						else if(cur_gap->end >= uEnd) {
								partSize = 0;
								break; // exit loop for()
							}
						}
						else if(cur_gap->start <= uEnd) {
							if(cur_gap->end < uEnd) {
								partSize -= cur_gap->end - cur_gap->start + 1;
							}
							else {
								partSize -= uEnd - cur_gap->start + 1;
							}
						}
					}
					const uint16 critCompletion = (uint16)(partSize/(PARTSIZE/100)); // in [%]

					// Calculate priority with all criteria
					if(cur_chunk.frequency <= veryRareBound){
						// 0..xxxx unrequested + requested very rare chunks
						cur_chunk.rank = (25 * cur_chunk.frequency) +      // Criterion 1
										((critPreview == true) ? 0 : 1) + // Criterion 2
										(100 - critCompletion);           // Criterion 4
					}
					else if(critPreview == true){
						// 10000..10100  unrequested preview chunks
						// 30000..30100  requested preview chunks
						cur_chunk.rank = ((critRequested == false) ? 10000 : 30000) + // Criterion 3
										(100 - critCompletion);                      // Criterion 4
					}
					else if(cur_chunk.frequency <= rareBound){
						// 10101..1xxxx  unrequested rare chunks
						// 30101..3xxxx  requested rare chunks
						cur_chunk.rank = (25 * cur_chunk.frequency) +                 // Criterion 1 
										((critRequested == false) ? 10101 : 30101) + // Criterion 3
										(100 - critCompletion);                      // Criterion 4
					}
					else { // common chunk
						if(critRequested == false){ // Criterion 3
							// 20000..2xxxx  unrequested common chunks
							cur_chunk.rank = 20000 +                // Criterion 3
											(100 - critCompletion); // Criterion 4
						}
						else{
							// 40000..4xxxx  requested common chunks
							// Remark: The weight of the completion criterion is inversed
							//         to spead the requests over the completing chunks. 
							//         Without this, the chunk closest to completion will  
							//         received every new sources.
							cur_chunk.rank = 40000 +                // Criterion 3
											(critCompletion);       // Criterion 4				
						}
					}
				}
			}

			// Select the next chunk to download
			if(chunksList.IsEmpty() == FALSE){
				// Find and count the chunck(s) with the highest priority
				uint16 count = 0; // Number of found chunks with same priority
				uint16 rank = 0xffff; // Highest priority found
				for(POSITION pos = chunksList.GetHeadPosition(); pos != NULL; ){
					const Chunk& cur_chunk = chunksList.GetNext(pos);
					if(cur_chunk.rank < rank){
						count = 1;
						rank = cur_chunk.rank;
					}
					else if(cur_chunk.rank == rank){
						count++;
					}
				}

				// Use a random access to avoid that everybody tries to download the 
				// same chunks at the same time (=> spread the selected chunk among clients)
				uint16 randomness = 1 + (uint16)((((uint32)rand()*(count-1))+(RAND_MAX/2))/RAND_MAX);
				for(POSITION pos = chunksList.GetHeadPosition(); ; ){
					POSITION cur_pos = pos;
					const Chunk& cur_chunk = chunksList.GetNext(pos);
					if(cur_chunk.rank == rank){
						randomness--; 
						if(randomness == 0){
							// Selection process is over 
							sender->m_lastPartAsked = cur_chunk.part;
							// Remark: this list might be reused up to ?count?times
							chunksList.RemoveAt(cur_pos);
							break; // exit loop for()
						}  
					}
				}
			}
			else {
				// There is no remaining chunk to download
				break; // Exit main loop while()
			}
		}
	}
	// Return the number of the blocks 
	*count = newBlockCount;
	
	// Return
	return (newBlockCount > 0);
}
// Maella end


CString CPartFile::GetInfoSummary(CPartFile* partfile) const
{
	CString strHash=EncodeBase16(partfile->GetFileHash(),16);

	CString Sbuffer,info,lsc, compl,buffer,lastdwl;

	if (partfile->IsPartFile()) {
		lsc.Format("%s", CastItoXBytes(partfile->GetCompletedSize()));
		compl.Format( "%s", CastItoXBytes(partfile->GetFileSize()));
		buffer.Format( "%s/%s",lsc,compl);
		compl.Format( "%s: %s (%.2f%%)\n",GetResString(IDS_DL_TRANSFCOMPL),buffer,partfile->GetPercentCompleted());
	} else
		compl="\n";

	if (partfile->lastseencomplete==NULL) lsc.Format("%s",GetResString(IDS_UNKNOWN).MakeLower() ); else
		lsc.Format( "%s", partfile->lastseencomplete.Format( thePrefs.GetDateTimeFormat()));

	float availability = 0;
	if(partfile->GetPartCount() != 0) {
		// Elandal: type to float before division
		// avoids implicit cast warning
		availability = partfile->GetAvailablePartCount() * 100.0 / partfile->GetPartCount();
	}

	CString avail="";
	if (partfile->IsPartFile()) avail.Format(GetResString(IDS_AVAIL),partfile->GetPartCount(),partfile->GetAvailablePartCount(),availability);

	if (partfile->GetCFileDate()!=NULL) lastdwl.Format( "%s",partfile->GetCFileDate().Format( thePrefs.GetDateTimeFormat()));
		else lastdwl=GetResString(IDS_UNKNOWN);
	
	CString sourcesinfo="";
	if (partfile->IsPartFile() ) sourcesinfo.Format( GetResString(IDS_DL_SOURCES)+": "+ GetResString(IDS_SOURCESINFO),partfile->GetSourceCount(),partfile->GetValidSourcesCount(),partfile->GetSrcStatisticsValue(DS_NONEEDEDPARTS),partfile->GetSrcA4AFCount());
		
	// always show space on disk
	CString sod="  ("+GetResString(IDS_ONDISK) +CastItoXBytes(partfile->GetRealFileSize())+")";

	CString status;
	if (partfile->GetTransferingSrcCount()>0)
		status.Format(GetResString(IDS_PARTINFOS2)+"\n",partfile->GetTransferingSrcCount());
	else 
		status.Format("%s\n",partfile->getPartfileStatus());

	//TODO: don't show the part.met filename for completed files..
	info.Format(GetResString(IDS_DL_FILENAME)+": %s\n"
		+GetResString(IDS_FD_HASH) +" %s\n"
		+GetResString(IDS_FD_SIZE) +" %s  %s\n"
		+GetResString(IDS_PARTINFOS)+" %s\n\n"
		+GetResString(IDS_STATUS)+": "+status
		+"%s"
		+sourcesinfo
		+(sourcesinfo.IsEmpty()?"":"\n\n")
		+"%s"
		+GetResString(IDS_LASTSEENCOMPL)+" "+lsc+"\n"
		+GetResString(IDS_FD_LASTCHANGE)+" "+lastdwl,
		partfile->GetFileName(),
		strHash,
		CastItoXBytes(partfile->GetFileSize()),	sod,
		partfile->GetPartMetFileName(),
		compl,
		avail
	);

	return info;
}

bool CPartFile::GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth,void* pSender)
{
	if (!IsPartFile()){
		return CKnownFile::GrabImage(GetPath() + CString("\\") + GetFileName(),nFramesToGrab, dStartTime, bReduceColor, nMaxWidth, pSender);
	}
	else{
		if ( ((GetStatus() != PS_READY && GetStatus() != PS_PAUSED) || m_bPreviewing || GetPartCount() < 2 || !IsComplete(0,PARTSIZE-1))  )
			return false;
		CString strFileName = RemoveFileExtension(GetFullName());
		if (m_FileCompleteMutex.Lock(100)){
			m_bPreviewing = true; 
			try{
				if (m_hpartfile.m_hFile != INVALID_HANDLE_VALUE){
					m_hpartfile.Close();
				}
			}
			catch(CFileException* exception){
				exception->Delete();
				m_FileCompleteMutex.Unlock();
				m_bPreviewing = false; 
				return false;
			}
		}
		else
			return false;

		return CKnownFile::GrabImage(strFileName,nFramesToGrab, dStartTime, bReduceColor, nMaxWidth, pSender);
	}
}

void CPartFile::GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender)
{
	// unlock and reopen the file
	if (IsPartFile()){
		CString strFileName = RemoveFileExtension(GetFullName());
		if (!m_hpartfile.Open(strFileName, CFile::modeReadWrite|CFile::shareDenyWrite|CFile::osSequentialScan)){
			// uhuh, that's really bad
			AddLogLine(true,IDS_FAILEDREOPEN,RemoveFileExtension(GetPartMetFileName()),GetFileName());
			SetStatus(PS_ERROR);
			StopFile();
		}
		m_bPreviewing = false;
		m_FileCompleteMutex.Unlock();
		// continue processing
	}
	CKnownFile::GrabbingFinished(imgResults, nFramesGrabbed, pSender);
}

void CPartFile::GetSizeToTransferAndNeededSpace(uint32& rui32SizeToTransfer, uint32& rui32NeededSpace) const
{
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;)
	{
		const Gap_Struct* cur_gap = gaplist.GetNext(pos);
		uint32 uGapSize = cur_gap->end - cur_gap->start;
		rui32SizeToTransfer += uGapSize;
		if(cur_gap->end == GetFileSize()-1)
			rui32NeededSpace = uGapSize;
	}
}

void CPartFile::SetLastAnsweredTimeTimeout()
{
	m_ClientSrcAnswered = 2 * CONNECTION_LATENCY + ::GetTickCount() - SOURCECLIENTREASKS;
}

/*Checks, if a given item should be shown in a given category
AllcatTypes:
	0	all
	1	all not assigned
	2	not completed
	3	completed
	4	waiting
	5	transfering
	6	errorous
	7	paused
	8	stopped
	10	Video
	11	Audio
	12	Archive
	13	CDImage
*/
// Rewritten.
bool CPartFile::CheckShowItemInGivenCat(int inCategory)
{

	Category_Struct* curCat = thePrefs.GetCategory(inCategory);

	if (curCat->viewfilters.bSuspendFilters && (GetCategory() == inCategory || curCat->viewfilters.nFromCats == 0))
		return true;

	if (curCat->viewfilters.nFromCats == 2 && GetCategory() != inCategory)
		return false;

	if (!curCat->viewfilters.bVideo && IsMovie())
		return false;
	if (!curCat->viewfilters.bAudio && ED2KFT_AUDIO == GetED2KFileTypeID(GetFileName()))
		return false;
	if (!curCat->viewfilters.bArchives && IsArchive())
		return false;
	if (!curCat->viewfilters.bImages && ED2KFT_CDIMAGE == GetED2KFileTypeID(GetFileName()))
		return false;
	if (!curCat->viewfilters.bWaiting && GetStatus()!=PS_PAUSED && !IsStopped() && ((GetStatus()==PS_READY|| GetStatus()==PS_EMPTY) && GetTransferingSrcCount()==0))
		return false;
	if (!curCat->viewfilters.bTransferring && ((GetStatus()==PS_READY|| GetStatus()==PS_EMPTY) && GetTransferingSrcCount()>0))
		return false;
	if (!curCat->viewfilters.bComplete && GetStatus() == PS_COMPLETE)
		return false;
	if (!curCat->viewfilters.bCompleting && GetStatus() == PS_COMPLETING)
		return false;
	if (!curCat->viewfilters.bHashing && GetStatus() == PS_HASHING)
		return false;
	if (!curCat->viewfilters.bPaused && GetStatus()==PS_PAUSED && !IsStopped())
		return false;
	if (!curCat->viewfilters.bStopped && IsStopped() && IsPartFile())
		return false;
	if (!curCat->viewfilters.bErrorUnknown && (GetStatus() == PS_ERROR || GetStatus() == PS_UNKNOWN))
		return false;
	if (GetFileSize() < curCat->viewfilters.nFSizeMin || (curCat->viewfilters.nFSizeMax != 0 && GetFileSize() > curCat->viewfilters.nFSizeMax))
		return false;
	uint32 nTemp = GetFileSize() - GetCompletedSize();
	if (nTemp < curCat->viewfilters.nRSizeMin || (curCat->viewfilters.nRSizeMax != 0 && nTemp > curCat->viewfilters.nRSizeMax))
		return false;
	if (curCat->viewfilters.nTimeRemainingMin > 0 || curCat->viewfilters.nTimeRemainingMax > 0)
	{
		sint32 nTemp2 = getTimeRemaining();
		if (nTemp2 < (sint32)curCat->viewfilters.nTimeRemainingMin || (curCat->viewfilters.nTimeRemainingMax != 0 && nTemp2 > (sint32)curCat->viewfilters.nTimeRemainingMax))
			return false;
	}
	nTemp = GetSourceCount();
	if (nTemp < curCat->viewfilters.nSourceCountMin || (curCat->viewfilters.nSourceCountMax != 0 && nTemp > curCat->viewfilters.nSourceCountMax))
		return false;
	nTemp = GetAvailableSrcCount();
	if (nTemp < curCat->viewfilters.nAvailSourceCountMin || (curCat->viewfilters.nAvailSourceCountMax != 0 && nTemp > curCat->viewfilters.nAvailSourceCountMax))
		return false;
	if (!curCat->viewfilters.sAdvancedFilterMask.IsEmpty() && !theApp.downloadqueue->ApplyFilterMask(GetFileName(), inCategory))
		return false;

	return true;
}
// khaos::categorymod-

void CPartFile::SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars)
{
	CKnownFile* pFile = theApp.sharedfiles->GetFileByID(GetFileHash());
	if (pFile && pFile == this)
	{
		theApp.sharedfiles->RemoveKeywords(this);
		CKnownFile::SetFileName(pszFileName, bReplaceInvalidFileSystemChars);
		theApp.sharedfiles->AddKeywords(this);
	}
	else
		CKnownFile::SetFileName(pszFileName, bReplaceInvalidFileSystemChars);
}

void CPartFile::SetActive(bool bActive)
{
	time_t tNow = time(NULL);
	if (bActive)
	{
		if (theApp.IsConnected())
		{
			if (m_tActivated == 0)
				m_tActivated = tNow;
		}
	}
	else
	{
		if (m_tActivated != 0)
		{
			m_nDlActiveTime += tNow - m_tActivated;
			m_tActivated = 0;
		}
	}
}

uint32 CPartFile::GetDlActiveTime() const
{
	uint32 nDlActiveTime = m_nDlActiveTime;
	if (m_tActivated != 0)
		nDlActiveTime += time(NULL) - m_tActivated;
	return nDlActiveTime;
}

// SLUGFILLER: SafeHash
void CPartFile::PerformFirstHash()
{
	CPartHashThread* parthashthread = (CPartHashThread*) AfxBeginThread(RUNTIME_CLASS(CPartHashThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
	m_PartsHashing += parthashthread->SetFirstHash(this);	// Only hashes completed parts, why hash gaps?
	parthashthread->ResumeThread();
}

bool CPartFile::IsPartShareable(uint16 partnumber) const
{
	if (partnumber < GetPartCount())
		return m_PartsShareable[partnumber];
	else
		return false;
}

bool CPartFile::IsRangeShareable(uint32 start, uint32 end) const
{
	uint16 first = start/PARTSIZE;
	uint16 last = end/PARTSIZE+1;
	if (last > GetPartCount() || first >= last)
		return false;
	for (uint16 i = first; i < last; i++)
		if (!m_PartsShareable[i])
			return false;
	return true;
}

void CPartFile::PartHashFinished(uint16 partnumber, bool corrupt)
{
	if (partnumber >= GetPartCount())
		return;
	m_PartsHashing--;
	if (corrupt){
		uint32	partRange = (partnumber < GetPartCount()-1)?PARTSIZE:(m_nFileSize % PARTSIZE);
		AddLogLine(true, GetResString(IDS_ERR_PARTCORRUPT), partnumber, GetFileName());
		if (partRange > 0) {
			partRange--;
			AddGap((ULONGLONG)PARTSIZE*partnumber, (ULONGLONG)PARTSIZE*partnumber + partRange);
		}

		// add part to corrupted list, if not already there
		if (!IsCorruptedPart(partnumber))
			corrupted_list.AddTail(partnumber);
		// Reduce transfered amount by corrupt amount
		m_iLostDueToCorruption += (partRange + 1);
		thePrefs.Add2LostFromCorruption(partRange + 1);

		// Update met file - gaps data changed
		SavePartFile();
	} else {
		AddDebugLogLine(false, "Finished part %u of \"%s\"", partnumber, GetFileName());

		// Successfully completed part, make it available for sharing
		POSITION posCorrupted = corrupted_list.Find(partnumber);
		if (posCorrupted)
			corrupted_list.RemoveAt(posCorrupted);

		// Successfully completed part, make it available for sharing
		m_PartsShareable[partnumber] = true;
		if (status == PS_EMPTY)
		{
			SetStatus( PS_READY);
			if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
				theApp.sharedfiles->SafeAddKFile(this);
		}
		if (!m_PartsHashing){
			// Update met file - file fully hashed
			SavePartFile();
		}

		if (theApp.emuledlg->IsRunning()){ // may be called during shutdown!
			// Is this file finished?
			if (!m_PartsHashing && gaplist.IsEmpty())
				CompleteFile(false);	// Everything was just confirmed as hashed, is it really necessary to verify hashes again?
		}
	}
}

void CPartFile::PharseICHResult()
{
	if (m_ICHPartsComplete.IsEmpty())
		return;

	while (!m_ICHPartsComplete.IsEmpty()) {
		uint16 partnumber = m_ICHPartsComplete.RemoveHead();
		uint32 partRange = (partnumber < GetPartCount()-1)?PARTSIZE:(m_nFileSize % PARTSIZE);

		m_iTotalPacketsSavedDueToICH++;
		thePrefs.Add2SessionPartsSavedByICH(1);

		if (partRange > 0) {
			partRange--;
			FillGap(PARTSIZE*partnumber, PARTSIZE*partnumber + partRange);
			RemoveBlockFromList(PARTSIZE*partnumber, PARTSIZE*partnumber + partRange);
		}

		// remove from corrupted list
		POSITION posCorrupted = corrupted_list.Find(partnumber);
		if (posCorrupted)
			corrupted_list.RemoveAt(posCorrupted);

		AddLogLine(true, GetResString(IDS_ICHWORKED), partnumber, GetFileName());

		// Successfully completed part, make it available for sharing
		m_PartsShareable[partnumber] = true;
		if (status == PS_EMPTY)
		{
			SetStatus( PS_READY);
			if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
				theApp.sharedfiles->SafeAddKFile(this);
		}
	}

	// Update met file - gaps data changed
	SavePartFile();

	if (theApp.emuledlg->IsRunning()){ // may be called during shutdown!
		// Is this file finished?
		if (!m_PartsHashing && gaplist.IsEmpty())
			CompleteFile(false);	// Everything was just confirmed as hashed, is it really nececery to verify hashes again?
	}
}

IMPLEMENT_DYNCREATE(CPartHashThread, CWinThread)

uint16 CPartHashThread::SetFirstHash(CPartFile* pOwner)
{
	m_pOwner = pOwner;
	m_ICHused = false;
	directory = thePrefs.GetTempDir();
	filename = RemoveFileExtension(pOwner->GetPartMetFileName());
	for (int i = 0; i < pOwner->GetPartCount(); i++)
		if (pOwner->IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
			uchar* cur_hash = new uchar[16];
			md4cpy(cur_hash, pOwner->GetPartHash(i));

			m_PartsToHash.Add(i);
			m_DesiredHashes.Add(cur_hash);
		}
	return m_PartsToHash.GetSize();
}

void CPartHashThread::SetSinglePartHash(CPartFile* pOwner, uint16 part, bool ICHused)
{
	m_pOwner = pOwner;
	m_ICHused = ICHused;
	directory = thePrefs.GetTempDir();
	filename = RemoveFileExtension(pOwner->GetPartMetFileName());

	if (part >= pOwner->GetPartCount()) {	// Out of bounds, no point in even trying
		PostMessage(theApp.emuledlg->m_hWnd,TM_PARTHASHEDCORRUPT,part,(LPARAM)m_pOwner);
		return;
	}

	uchar* cur_hash = new uchar[16];
	md4cpy(cur_hash, pOwner->GetPartHash(part));

	m_PartsToHash.Add(part);
	m_DesiredHashes.Add(cur_hash);
}

int CPartHashThread::Run()
{
	CFile file;
	CSingleLock sLock(&(m_pOwner->ICH_mut)); // ICH locks the file
	if (m_ICHused)
		sLock.Lock();
	if (file.Open(directory+"\\"+filename,CFile::modeRead|CFile::osSequentialScan|CFile::shareDenyNone)){
		for (int i = 0; i < m_PartsToHash.GetSize(); i++){
			uint16 partnumber = m_PartsToHash[i];
			uchar hashresult[16];
			file.Seek((LONGLONG)PARTSIZE*partnumber,0);
			uint32 length = PARTSIZE;
			if ((ULONGLONG)PARTSIZE*(partnumber+1) > file.GetLength()){
				length = (file.GetLength()- ((ULONGLONG)PARTSIZE*partnumber));
				ASSERT( length <= PARTSIZE );
			}
			m_pOwner->CreateHashFromFile(&file,length,hashresult);
			if (!theApp.emuledlg->IsRunning())	// in case of shutdown while still hashing
				break;

			if (md4cmp(hashresult,m_DesiredHashes[i])){
				if (!m_ICHused)		// ICH only sends successes
					PostMessage(theApp.emuledlg->m_hWnd,TM_PARTHASHEDCORRUPT,partnumber,(LPARAM)m_pOwner);
			} else {
				if (m_ICHused)
					m_pOwner->m_ICHPartsComplete.AddTail(partnumber);	// Time critical, don't use message callback
				else
					PostMessage(theApp.emuledlg->m_hWnd,TM_PARTHASHEDOK,partnumber,(LPARAM)m_pOwner);
			}
		}
		file.Close();
	}
	for (int i = 0; i < m_DesiredHashes.GetSize(); i++)
		delete m_DesiredHashes[i];
	if (m_ICHused)
		sLock.Unlock();
	return 0;
}
// SLUGFILLER: SafeHash

//MORPH START - Added by IceCream, eMule Plus rating icons
int CPartFile::GetRating(){  
	if (!hasRating) return 0;
    int num,tot,fRate;  
    num=tot=0;  
    for(POSITION pos = srclist.GetHeadPosition();pos!=0;){
		fRate =((CUpDownClient*) srclist.GetNext(pos))->GetFileRate();  
		if (fRate > 0)  
		{
			num++;
			//Cax2 - bugfix: for some ?%#ing reason  fair=4 & good=3, breaking the progression from fake(1) to excellent(5)
			//the reason is for compatibility with official rating sort order
			if (fRate==3 || fRate==4) fRate=(fRate==3)?4:3;
			tot+=fRate;  
		}
	}
	if (num>0)
	{
		num=(float)tot/num+.5; //Cax2 - get the average of all the ratings
		//Cax2 - bugfix: for some ?%#ing reason good=3 & fair=4, breaking the progression from fake(1) to excellent(5)
		//the reason is for compatibility with official rating sort order
		if (num==3 || num==4) num=(num==3)?4:3;
	}
    return num; //Cax2 - if no ratings found, will return 0!
}
//MORPH END   - Added by IceCream, eMule Plus rating icons

//Morph - added by AndCycle, Only download complete files v2.1 (shadow)
bool	CPartFile::notSeenCompleteSource() const
{
	
	if(!thePrefs.OnlyDownloadCompleteFiles()){
		return false;
	}
	else if(lastseencomplete == NULL){
		return true;
	}
	else if(CTime::GetCurrentTime() - lastseencomplete > 1209600){//14 days, no complete source...
		return true;
	}
	return false;

}
//Morph - added by AndCycle, Only download complete files v2.1 (shadow)