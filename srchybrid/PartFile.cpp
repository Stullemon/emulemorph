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
#include "partfile.h"
#include "emule.h"
#include "updownclient.h"
#include <math.h>
#include "ED2KLink.h"
#include "Preview.h"
#include "ArchiveRecovery.h"
#include <sys/stat.h>
#include <io.h>
#include "SearchList.h"
#include <winioctl.h>
#include "kademlia/kademlia/search.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/routing/Timer.h"
#include "kademlia/utils/MiscUtils.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// Barry - use this constant for both places
#define PROGRESS_HEIGHT 3

CBarShader CPartFile::s_LoadBar(PROGRESS_HEIGHT); // Barry - was 5
CBarShader CPartFile::s_ChunkBar(16); 

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
				if (pTag->tag.type == 2)
					SetFileName(pTag->tag.stringvalue);
				break;
			}
			case FT_FILESIZE:{
				if (pTag->tag.type == 3)
					SetFileSize(pTag->tag.intvalue);
				break;
			}
			default:{
				bool bTagAdded = false;
				if (pTag->tag.specialtag != 0 && pTag->tag.tagname == NULL && (pTag->tag.type == 2 || pTag->tag.type == 3))
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
							if (pTag->tag.type == 2 && (pTag->tag.stringvalue == NULL || pTag->tag.stringvalue[0] == '\0'))
								break;

							// skip integer tags with '0' values
							if (pTag->tag.type == 3 && pTag->tag.intvalue == 0)
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
		OUTPUT_DEBUG_TRACE();
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
		OUTPUT_DEBUG_TRACE();
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
	transfered = 0;

	if(theApp.glob_prefs->GetNewAutoDown()){
		m_iDownPriority = PR_HIGH;
		m_bAutoDownPriority = true;
	}
	else{
		m_iDownPriority = PR_NORMAL;
		m_bAutoDownPriority = false;
	}
	srcarevisible = false;
	// -khaos--+++> Initialize our stat vars to 0
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
	m_bPercentUpdated = false;
	m_bRecoveringArchive = false;
	m_iGainDueToCompression = 0;
	m_iLostDueToCorruption = 0;
	m_iTotalPacketsSavedDueToICH = 0;
	hasRating	= false;
	hasComment	= false;
	hasBadRating= false;
	m_lastdatetimecheck=0;
	m_category=0;
	updatemystatus=true;
	m_lastRefreshedDLDisplay = 0;
	//MORPH - Removed by SiRoB, Due to Khaos A4AF
	//m_is_A4AF_auto=false;
	m_bLocalSrcReqQueued = false;	
	memset(src_stats,0,sizeof(src_stats));
	m_nCompleteSourcesTime = time(NULL);
	m_nCompleteSourcesCount = 0;
	m_nCompleteSourcesCountLo = 0;
	m_nCompleteSourcesCountHi = 0;
	//MORPH START - HotFix by SiRoB, khaos 14.6 missing
	m_iSesCompressionBytes = 0;
	m_iSesCorruptionBytes = 0;
	//MORPH END - HotFix by SiRoB, khaos 14.6 missing
	m_iSrcA4AF = 0; //MORPH - Added by SiRoB, A4AF counter
	// khaos::categorymod+
	m_catResumeOrder=0;
	m_catFileGroup=0;
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
	m_random_update_wait = (uint32)(rand()/(RAND_MAX/1000));
	m_NumberOfClientsWithPartStatus = 0;
	//MORPH END   - Added by SiRoB, ZZ Upload System 20030723-0133
	//MORPH START - Added by SiRoB,  SharedStatusBar CPU Optimisation
	InChangedSharedStatusBar = false;
	m_pbitmapOldSharedStatusBar = NULL;
	//MORPH END   - Added by SiRoB,  SharedStatusBar CPU Optimisation
}


CPartFile::~CPartFile(){
	// Barry - Ensure all buffered data is written
	try{
		FlushBuffer();
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

	m_SrcpartFrequency.RemoveAll();
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;gaplist.GetNext(pos))
		delete gaplist.GetAt(pos);
	//MORPH START - Added by SiRoB, Reduce SharedStatusBar CPU consumption
	if(m_pbitmapOldSharedStatusBar != NULL) m_dcSharedStatusBar.SelectObject(m_pbitmapOldSharedStatusBar);
	//MORPH END - Added by SiRoB, Reduce SharedStatusBar CPU consumption
}

void CPartFile::CreatePartFile(){
	// use lowest free partfilenumber for free file (InterCeptor)
	int i = 0; 
	CString filename; 
	do 
	{ 
		i++; 
		filename.Format("%s\\%03i.part", theApp.glob_prefs->GetTempDir(), i); 
	}while(PathFileExists(filename)); 
	m_partmetfilename.Format("%03i.part.met",i); 

	SetPath(theApp.glob_prefs->GetIncomingDir());

	m_fullname.Format("%s\\%s",theApp.glob_prefs->GetTempDir(),m_partmetfilename);
	CTag* partnametag = new CTag(FT_PARTFILENAME,RemoveFileExtension(m_partmetfilename));
	taglist.Add(partnametag);
	
	Gap_Struct* gap = new Gap_Struct;
	gap->start = 0;
	gap->end = m_nFileSize-1;
	gaplist.AddTail(gap);

	dateC=time(NULL);
	CString partfull(RemoveFileExtension(m_fullname));
	SetFilePath(partfull);
	if (!m_hpartfile.Open(partfull,CFile::modeCreate|CFile::modeReadWrite|CFile::shareDenyWrite|CFile::osSequentialScan)){
		AddLogLine(false,GetResString(IDS_ERR_CREATEPARTFILE));
		SetStatus(PS_ERROR);
	}

	
	if (GetED2KPartHashCount() == 0)
		hashsetneeded = false;

	m_SrcpartFrequency.SetSize(GetPartCount());
	for (uint32 i = 0; i < GetPartCount();i++)
		m_SrcpartFrequency.Add(0);
	paused = false;

	if (theApp.glob_prefs->AutoFilenameCleanup())
		SetFileName(CleanupFilename(GetFileName()));

	SavePartFile();
}

bool CPartFile::LoadPartFile(LPCTSTR in_directory,LPCTSTR in_filename)
{
	CMap<uint16, uint16, Gap_Struct*, Gap_Struct*> gap_map; // Slugfiller
	transfered = 0;
	m_partmetfilename = in_filename;
	SetPath(in_directory);
	m_fullname.Format("%s\\%s", GetPath(), m_partmetfilename);

	// read file creation time
	struct _stat fileinfo;
	if (_stat(m_fullname, &fileinfo) == 0)
		dateC = fileinfo.st_ctime;

	// readfile data form part.met file
	CSafeBufferedFile metFile;
	CFileException fexpMet;
	if (!metFile.Open(m_fullname, CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary, &fexpMet)){
		CString strError;
		strError.Format(GetResString(IDS_ERR_OPENMET), m_partmetfilename, _T(""));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexpMet.GetErrorMessage(szError, ELEMENT_COUNT(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("%s"), strError);
		return false;
	}
	setvbuf(metFile.m_pStream, NULL, _IOFBF, 16384);

	try{
		uint8 version;
		metFile.Read(&version,1);
		if (version != PARTFILE_VERSION){
			metFile.Close();
			AddLogLine(false, GetResString(IDS_ERR_BADMETVERSION), m_partmetfilename, GetFileName());
			return false;
		}
		LoadDateFromFile(&metFile);
		LoadHashsetFromFile(&metFile, false);

		uint32 tagcount;
		metFile.Read(&tagcount, 4);

		bool bInitialBytesSet = false; // khaos::accuratetimerem+ One Line

		for (uint32 j = 0; j != tagcount; j++){
			CTag* newtag = new CTag(&metFile);
			switch(newtag->tag.specialtag){
				case FT_FILENAME:{
					if(newtag->tag.stringvalue == NULL) {
						AddLogLine(true, GetResString(IDS_ERR_METCORRUPT), m_partmetfilename, GetFileName());
						delete newtag;
						return false;
					}
					SetFileName(newtag->tag.stringvalue);
					delete newtag;
					break;
				}
				case FT_LASTSEENCOMPLETE: {
					lastseencomplete = newtag->tag.intvalue;
					delete newtag;
					break;
				}
				case FT_FILESIZE:{
					SetFileSize(newtag->tag.intvalue);
					delete newtag;
					break;
				}
				case FT_TRANSFERED:{
					transfered = newtag->tag.intvalue;
					delete newtag;
					break;
				}
				case FT_CATEGORY:{
					m_category = newtag->tag.intvalue;
					delete newtag;
					break;
				}
				case FT_DLPRIORITY:{
					m_iDownPriority = newtag->tag.intvalue;
					delete newtag;
					if( m_iDownPriority == PR_AUTO ){
						m_iDownPriority = PR_HIGH;
						SetAutoDownPriority(true);
					}
					else
						SetAutoDownPriority(false);
					break;
				}
				case FT_STATUS:{
					paused = newtag->tag.intvalue;
					stopped=paused;
					delete newtag;
					break;
				}
				case FT_ULPRIORITY:{
					SetUpPriority(newtag->tag.intvalue, false);
					delete newtag;
					if( GetUpPriority() == PR_AUTO ){
						SetUpPriority(PR_HIGH, false);
						SetAutoUpPriority(true);
					}
					else
						SetAutoUpPriority(false);
					break;
				}
				case FT_ONLASTPUBLISH:{
					SetLastPublishTimeKad(newtag->tag.intvalue);
					delete newtag;
					break;
				}
				// khaos::categorymod+
				case FT_CATRESUMEORDER:{
					m_catResumeOrder = newtag->tag.intvalue;
					delete newtag;
					break;
				}
				case FT_CATFILEGROUP:{
					m_catFileGroup = newtag->tag.intvalue;
					delete newtag;
					break;
				}
				// khaos::categorymod-
				// khaos::compcorruptfix+
				case FT_COMPRESSIONBYTES:
					{
						m_iGainDueToCompression = newtag->tag.intvalue;
						delete newtag;
						break;
					}
				case FT_CORRUPTIONBYTES:
					{
						m_iLostDueToCorruption = newtag->tag.intvalue;
						delete newtag;
						break;
					}
				// khaos::compcorruptfix-
				// khaos::kmod+
				case FT_A4AFON:
					{
						m_bForceAllA4AF = (newtag->tag.intvalue == 1) ? true : false;
						delete newtag;
						break;
					}
				case FT_A4AFOFF:
					{
						m_bForceA4AFOff = (newtag->tag.intvalue == 1) ? true : false;
						delete newtag;
						break;
					}
				// khaos::kmod-
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
							gap->start = -1;
							gap->end = -1;
						}
						if (newtag->tag.tagname[0] == FT_GAPSTART)
							gap->start = newtag->tag.intvalue;
						if (newtag->tag.tagname[0] == FT_GAPEND)
							gap->end = newtag->tag.intvalue-1;
						delete newtag;
					// End Changes by Slugfiller for better exception handling
					//MORPH START - Added by SiRoB, ZZ Upload System
					} else if((!newtag->tag.specialtag) && strcmp(newtag->tag.tagname, FT_POWERSHARE) == 0) {
						//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
						//SetPowerShared(newtag->tag.intvalue == 1);
						SetPowerShared((newtag->tag.intvalue<3)?newtag->tag.intvalue:2);
						//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
						delete newtag;
					}
					//MORPH END - Added by SiRoB, ZZ Upload System
					else
						taglist.Add(newtag);
				}
			}
		}
		metFile.Close();
	}
	catch(CFileException* error){
		OUTPUT_DEBUG_TRACE();
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
		OUTPUT_DEBUG_TRACE();
		AddLogLine(true, GetResString(IDS_ERR_METCORRUPT), m_partmetfilename, GetFileName());
		return false;
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
		if (fexpPart.GetErrorMessage(szError, ELEMENT_COUNT(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("%s"), strError);
		return false;
	}
	SetFilePath(searchpath);
	
	// SLUGFILLER: SafeHash - final safety, make sure any missing part of the file is gap
	if (m_hpartfile.GetLength() < m_nFileSize)
		AddGap(m_hpartfile.GetLength(), m_nFileSize-1);
	// Goes both ways - Partfile should never be too large
	if (m_hpartfile.GetLength() > m_nFileSize){
		TRACE("Partfile \"%s\" is too large! Truncating %I64u bytes.\n", GetFileName(), m_hpartfile.GetLength() - m_nFileSize);
		m_hpartfile.SetLength(m_nFileSize);
	}
	// SLUGFILLER: SafeHash

	theApp.sharedfiles->filelist->FilterDuplicateKnownFiles(this);	// SLUGFILLER: mergeKnown - load statistics

	m_SrcpartFrequency.SetSize(GetPartCount());
	for (uint32 i = 0; i != GetPartCount();i++)
		m_SrcpartFrequency.Add(0);
	SetStatus(PS_EMPTY);
	// check hashcount, filesatus etc
	if (GetHashCount() != GetED2KPartHashCount()){
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

	// check date of .part file - if its wrong, rehash file
	CFileStatus filestatus;
	m_hpartfile.GetStatus(filestatus); // this; "...returns m_attribute without high-order flags" indicates a known MFC bug, wonder how many unknown there are... :)
	time_t fdate = mktime(filestatus.m_mtime.GetLocalTm());
	if (fdate == -1)
		AddDebugLogLine(false, "Failed to convert file date of %s (%s)", filestatus.m_szFullName, GetFileName());
	if (date != fdate){
		AddLogLine(false, GetResString(IDS_ERR_REHASH), m_fullname, GetFileName());
		// rehash
		SetStatus(PS_WAITINGFORHASH);
		CAddFileThread* addfilethread = (CAddFileThread*) AfxBeginThread(RUNTIME_CLASS(CAddFileThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
		addfilethread->SetValues(0, GetPath(), m_hpartfile.GetFileName().GetBuffer(), this);
		addfilethread->ResumeThread();
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

bool CPartFile::SavePartFile(){
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
		AddLogLine(false, GetResString(IDS_ERR_SAVEMET), GetResString(IDS_ERR_PART_FNF), m_partmetfilename, GetFileName());
		return false;
	}

	//get filedate
	CTime lwtime;
	if (!ff.GetLastWriteTime(lwtime))
		AddDebugLogLine(false, "Failed to get file date of %s (%s) - %s", m_partmetfilename, GetFileName(), GetErrorMessage(GetLastError()));
	date = mktime(lwtime.GetLocalTm());
	if (date == -1)
		AddDebugLogLine(false, "Failed to convert file date of %s (%s)", m_partmetfilename, GetFileName());
	ff.Close();
	uint32 lsc = mktime(lastseencomplete.GetLocalTm());

	CString strTmpFile(m_fullname);
	strTmpFile += PARTMET_TMP_EXT;

	// save file data to part.met file
	CSafeBufferedFile file;
	CFileException fexp;
	if (!file.Open(strTmpFile, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary, &fexp)){
		CString strError;
		strError.Format(GetResString(IDS_ERR_SAVEMET), m_partmetfilename, GetFileName());
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ELEMENT_COUNT(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("%s"), strError);
		return false;
	}
	setvbuf(file.m_pStream, NULL, _IOFBF, 16384);

	try{
		//version
		uint8 version = PARTFILE_VERSION;
		file.Write(&version,1);
		//date
		file.Write(&date,4);
		//hash
		file.Write(&m_abyFileHash,16);
		uint16 parts = hashlist.GetCount();
		file.Write(&parts,2);
		for (int x = 0; x != parts; x++)
			file.Write(hashlist[x],16);
		//tags
		// khaos::categorymod+
		// -khaos--+++>
		//uint32 tagcount = 9+(gaplist.GetCount()*2);
		//MORPH START - Modified by SiRoB, ZZ Upload System
		uint32 tagcount = 9/*Official*/+5/*Khaos*/+1/*ZZ*/+(gaplist.GetCount()*2);
		//MORPH START - Modified by SiRoB, ZZ Upload System
		// <-----khaos-
		// khaos::categorymod-
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
		file.Write(&tagcount,4);

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

		CTag lsctag(FT_LASTSEENCOMPLETE,lsc);
		lsctag.WriteTagToFile(&file);

		CTag ulprioritytag(FT_ULPRIORITY, IsAutoUpPriority() ? PR_AUTO : GetUpPriority());
		ulprioritytag.WriteTagToFile(&file);

		CTag categorytag(FT_CATEGORY, m_category);
		categorytag.WriteTagToFile(&file);

		CTag onLastPub(FT_ONLASTPUBLISH, GetLastPublishTimeKad());
		onLastPub.WriteTagToFile(&file);

		//MORPH START - Added by SiRoB, ZZ Upload System
		//MORPH START - Added by SiRoB, Avoid misusing of powersharing
		CTag powersharetag(FT_POWERSHARE, GetPowerSharedMode());
		//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
		powersharetag.WriteTagToFile(&file);
		//MORPH END - Added by SiRoB, ZZ Upload System

		// khaos::categorymod+
		CTag catresumetag(FT_CATRESUMEORDER, m_catResumeOrder );
		catresumetag.WriteTagToFile(&file);

		//CTag catfilegroup(FT_CATFILEGROUP, m_catFileGroup);
		//catfilegroup.WriteTagToFile(&file);
		// khaos::categorymod-

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
		for (POSITION pos = gaplist.GetHeadPosition();pos != 0;gaplist.GetNext(pos)){
			itoa(i_pos,number,10);
			namebuffer[0] = FT_GAPSTART;
			CTag gapstarttag(namebuffer,gaplist.GetAt(pos)->start);
			gapstarttag.WriteTagToFile(&file);
			// gap start = first missing byte but gap ends = first non-missing byte in edonkey
			// but I think its easier to user the real limits
			namebuffer[0] = FT_GAPEND;
			CTag gapendtag(namebuffer,(gaplist.GetAt(pos)->end)+1);
			gapendtag.WriteTagToFile(&file);
			i_pos++;
		}

		if (theApp.glob_prefs->GetCommitFiles() >= 2 || (theApp.glob_prefs->GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			file.Flush(); // flush file stream buffers to disk buffers
			if (_commit(_fileno(file.m_pStream)) != 0) // commit disk buffers to disk
				AfxThrowFileException(CFileException::hardIO, GetLastError(), file.GetFileName());
		}
		file.Close();
	}
	catch(CFileException* error){
		OUTPUT_DEBUG_TRACE();
		CString strError;
		strError.Format(GetResString(IDS_ERR_SAVEMET), m_partmetfilename, GetFileName());
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ELEMENT_COUNT(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("%s"), strError);
		error->Delete();

		file.Close();
		(void)_tremove(strTmpFile); // remove the partially written or otherwise damaged temporary file
		return false;
	}

	// after successfully writing the temporary part.met file...
	if (_tremove(m_fullname) != 0 && errno != ENOENT)
		AddDebugLogLine(false, _T("Failed to remove \"%s\" - %s"), m_fullname, strerror(errno));

	if (_trename(strTmpFile, m_fullname) != 0){
		int iErrno = errno;
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
	if (!::CopyFile(m_fullname, BAKName, FALSE))
		AddDebugLogLine(false, "Failed to create backup of %s (%s) - %s", m_fullname, GetFileName(), GetErrorMessage(GetLastError()));

	return true;
}

void CPartFile::PartFileHashFinished(CKnownFile* result){
	newdate = true;
	bool errorfound = false;
	if (GetED2KPartHashCount() == 0){
		if (IsComplete(0, m_nFileSize-1)){
			if (md4cmp(result->GetFileHash(), GetFileHash())){
				AddLogLine(false, GetResString(IDS_ERR_FOUNDCORRUPTION), 1, GetFileName());
				AddGap(0, m_nFileSize-1);
				errorfound = true;
			}
		}
	}
	else{
		for (uint32 i = 0; i < (uint32)hashlist.GetSize(); i++){
			if (IsComplete(i*PARTSIZE,((i+1)*PARTSIZE)-1)){
				if (!(result->GetPartHash(i) && !md4cmp(result->GetPartHash(i),this->GetPartHash(i)))){
					AddLogLine(false, GetResString(IDS_ERR_FOUNDCORRUPTION), i+1, GetFileName());
					AddGap(i*PARTSIZE,((((i+1)*PARTSIZE)-1) >= m_nFileSize) ? m_nFileSize-1 : ((i+1)*PARTSIZE)-1);
					errorfound = true;
				}
			}
		}
	}
	delete result;
	if (!errorfound){
		if (status == PS_COMPLETING){
			CompleteFile(true);
			return;
		}
		else
			AddLogLine(false, GetResString(IDS_HASHINGDONE), GetFileName());
	}
	else{
		SetStatus(PS_READY);
		SavePartFile();
		return;
	}
	SetStatus( PS_READY);
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
	//theApp.emuledlg->transferwnd.downloadlistctrl.UpdateItem(this);
	UpdateDisplayedInfo();
	newdate = true;
}

bool CPartFile::IsComplete(uint32 start, uint32 end){
	if (end >= m_nFileSize)
		end = m_nFileSize-1;
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;gaplist.GetNext(pos)){
		Gap_Struct* cur_gap = gaplist.GetAt(pos);
		if ((cur_gap->start >= start && cur_gap->end <= end)||(cur_gap->start >= start 
			&& cur_gap->start <= end)||(cur_gap->end <= end && cur_gap->end >= start)
			||(start >= cur_gap->start && end <= cur_gap->end))
		{
			return false;	
		}
	}
	return true;
}

bool CPartFile::IsPureGap(uint32 start, uint32 end){
	if (end >= m_nFileSize)
		end = m_nFileSize-1;
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;gaplist.GetNext(pos)){
		Gap_Struct* cur_gap = gaplist.GetAt(pos);
		if (start >= cur_gap->start  && end <= cur_gap->end ){
			return true;
		}
	}
	return false;
}

bool CPartFile::IsAlreadyRequested(uint32 start, uint32 end){
	for (POSITION pos =  requestedblocks_list.GetHeadPosition();pos != 0; requestedblocks_list.GetNext(pos)){
		Requested_Block_Struct* cur_block =  requestedblocks_list.GetAt(pos);
		if ((start <= cur_block->EndOffset) && (end >= cur_block->StartOffset))
			return true;
	}
	return false;
}

bool CPartFile::GetNextEmptyBlockInPart(uint16 partNumber, Requested_Block_Struct *result)
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
	while (true)
	{
		firstGap = NULL;

		// Find the first gap from the start position
		for (POSITION pos = gaplist.GetHeadPosition(); pos != 0; gaplist.GetNext(pos))
		{
			currentGap = gaplist.GetAt(pos);
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
	//theApp.emuledlg->transferwnd.downloadlistctrl.UpdateItem(this);
	UpdateDisplayedInfo();
	newdate = true;
	// Barry - The met file is now updated in FlushBuffer()
	//SavePartFile();
}

void CPartFile::UpdateCompletedInfos()
{
   	uint32 allgaps = 0; 

	for (POSITION pos = gaplist.GetHeadPosition(); pos != 0; gaplist.GetNext(pos)){ 
		Gap_Struct* cur_gap = gaplist.GetAt(pos); 
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
void CPartFile::DrawShareStatusBar(CDC* dc, RECT* rect, bool onlygreyrect, bool  bFlat){ 
	int wSize=rect->right - rect->left;
	int hSize=rect->bottom - rect->top;

	if (wSize>0){
		if(!InChangedSharedStatusBar || lastSize!=wSize || lastonlygreyrect!=onlygreyrect || lastbFlat!=bFlat){
			InChangedSharedStatusBar = true;
			lastSize=wSize;
			lastonlygreyrect=onlygreyrect;
			lastbFlat=bFlat;
			if(m_pbitmapOldSharedStatusBar && m_bitmapSharedStatusBar.GetSafeHandle() && m_dcSharedStatusBar.GetSafeHdc())
			{
				m_dcSharedStatusBar.SelectObject(m_pbitmapOldSharedStatusBar);
				m_bitmapSharedStatusBar.DeleteObject();
				m_bitmapSharedStatusBar.CreateCompatibleBitmap(dc, wSize, hSize);
				m_pbitmapOldSharedStatusBar = m_dcSharedStatusBar.SelectObject(&m_bitmapSharedStatusBar);
			}
			if(m_dcSharedStatusBar.GetSafeHdc() == NULL){
				m_dcSharedStatusBar.CreateCompatibleDC(dc);
				m_bitmapSharedStatusBar.DeleteObject();
				m_bitmapSharedStatusBar.CreateCompatibleBitmap(dc, wSize, hSize);
				m_pbitmapOldSharedStatusBar = m_dcSharedStatusBar.SelectObject(&m_bitmapSharedStatusBar);
			}
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
			s_ChunkBar.SetHeight(hSize); 
			s_ChunkBar.SetWidth(wSize); 
			s_ChunkBar.Fill(crMissing); 
			COLORREF color;
			if (!onlygreyrect && !m_SrcpartFrequency.IsEmpty()) { 
				for (int i = 0;i < GetPartCount();i++)
					if(m_SrcpartFrequency[i] > 0 ){
						color = RGB(0, (210-(22*(m_SrcpartFrequency[i]-1)) <  0)? 0:210-(22*(m_SrcpartFrequency[i]-1)), 255);
						s_ChunkBar.FillRange(PARTSIZE*(i),PARTSIZE*(i+1),color);
					}
			}
   			s_ChunkBar.Draw(&m_dcSharedStatusBar, 0, 0, bFlat);
		}
		if(m_dcSharedStatusBar.GetSafeHdc() != NULL)	dc->BitBlt(rect->left,rect->top,wSize,hSize,&m_dcSharedStatusBar,0,0,SRCCOPY);
	}
} 

void CPartFile::DrawStatusBar(CDC* dc, RECT* rect, bool bFlat){ 
	COLORREF crProgress;
	COLORREF crHave;
	COLORREF crPending;
	COLORREF crMissing;		// SLUGFILLER: grayPause - moved down
	//--- xrmb:confirmedDownload ---
	COLORREF crUnconfirmed = RGB(255, 210, 0);
	//--- :xrmb ---
	COLORREF crDot = RGB(255, 255, 255);	//MORPH - Added by IceCream, SLUGFILLER: chunkDots
	bool notgray = GetStatus() == PS_EMPTY || GetStatus() == PS_READY; // SLUGFILLER: grayPause - only test once
	//MORPH START - Added by IceCream, SLUGFILLER: grayPause - Colors by status
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
//		crProgress = RGB(0, 224, 0);
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
	//MORPH END   - Added by IceCream, SLUGFILLER: grayPause

	s_ChunkBar.SetHeight(rect->bottom - rect->top);
	s_ChunkBar.SetWidth(rect->right - rect->left);
	s_ChunkBar.SetFileSize(m_nFileSize);
	s_ChunkBar.Fill(crHave);

	uint32 allgaps = 0;

	if(status == PS_COMPLETE || status == PS_COMPLETING) {
		s_ChunkBar.FillRange(0, m_nFileSize, crProgress);
		s_ChunkBar.Draw(dc, rect->left, rect->top, bFlat);
		percentcompleted = 100;
		completedsize = m_nFileSize;
		return;
	}

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
				COLORREF color;
				if (m_SrcpartFrequency.GetCount() >= (INT_PTR)i && m_SrcpartFrequency[(uint16)i])  // frequency?
				//MORPH START - Added by IceCream, SLUGFILLER: grayPause
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
				//MORPH END   - Added by IceCream SLUGFILLER: grayPause
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
		Requested_Block_Struct* block =  requestedblocks_list.GetNext(pos);
		s_ChunkBar.FillRange((block->StartOffset + block->transferred), block->EndOffset,  crPending);
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

void CPartFile::WritePartStatus(CFile* file, CUpDownClient* client){
	// SLUGFILLER: hideOS
	CArray<uint32, uint32> partspread;
	uint16 parts;
	uint8 hideOS = theApp.glob_prefs->GetHideOvershares();
	if (hideOS && client) {
		parts = CalcPartSpread(partspread, client);
	} else {	// simpler to set as 0 than to create another loop...
		parts = GetED2KPartCount();
		for (uint16 i = 0; i < parts; i++)
			partspread.Add(0);
		hideOS = 1;
	}
	file->Write(&parts,2);
	ASSERT(parts != 0);
	//wistily SOTN START
	if (GetPowerShareAuto()){ 
        uint16 done = 0;
		if(m_AvailPartFrequency.GetSize() == parts) //MORPH - Added by SiRoB, fixe when m_AvailPartFrequency is not as expected
			while (done != parts){
				uint8 towrite = 0;
				for (uint32 i = 0;i != 8;i++){
					if (m_AvailPartFrequency[done]==0)	
						if (IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1))
						towrite |= (1<<i);
					done++;
					if (done == parts)
						break;
				}
				file->Write(&towrite,1);
			}
	}
	//wistily SOTN END
	else {		
		// SLUGFILLER: hideOS
		uint16 done = 0;
		while (done != parts){
			uint8 towrite = 0;
			for (uint32 i = 0;i != 8;i++){
				if (partspread[done] < hideOS)	// SLUGFILLER: hideOS
					if (IsComplete(done*PARTSIZE,((done+1)*PARTSIZE)-1))
						towrite |= (1<<i);
				done++;
				if (done == parts)
					break;
			}
			file->Write(&towrite,1);
		}
	}
}

void CPartFile::WriteCompleteSourcesCount(CFile* file)
{
	uint16 completecount = m_nCompleteSourcesCount;
	file->Write(&completecount,2);
}

// -khaos--+++> These values are now cached.
/*int CPartFile::GetValidSourcesCount() {
	int counter=0;
	POSITION pos1,pos2;
	for (int sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty())
	for (pos1 = srclists[sl].GetHeadPosition();( pos2 = pos1 ) != NULL;){
		srclists[sl].GetNext(pos1);
		CUpDownClient* cur_src = srclists[sl].GetAt(pos2);
		if (cur_src->GetDownloadState()==DS_ONQUEUE || cur_src->GetDownloadState()==DS_DOWNLOADING ||
			cur_src->GetDownloadState()==DS_CONNECTED || cur_src->GetDownloadState()==DS_REMOTEQUEUEFULL ) ++counter;
	}
	return counter;
}

uint16 CPartFile::GetNotCurrentSourcesCount(){
	uint16 counter=0;

	POSITION pos1,pos2;
	for (int sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty())
	for (pos1 = srclists[sl].GetHeadPosition();( pos2 = pos1 ) != NULL;){
		srclists[sl].GetNext(pos1);
		CUpDownClient* cur_src = srclists[sl].GetAt(pos2);
		if (cur_src->GetDownloadState()!=DS_ONQUEUE && cur_src->GetDownloadState()!=DS_DOWNLOADING) counter++;
	}
	return counter;
}*/
// <-----khaos-

//MORPH START - Added by IceCream, SLUGFILLER: checkDiskspace
uint32 CPartFile::GetNeededSpace(){
	if (m_hpartfile.GetLength() > GetFileSize())
		return 0;	// Shouldn't happen, but just in case
	return GetFileSize()-m_hpartfile.GetLength();
}
//MORPH END   - Added by IceCream, SLUGFILLER: checkDiskspace

uint8 CPartFile::GetStatus(bool ignorepause){
	//MORPH START - Added by IceCream, SLUGFILLER: checkDiskspace
	if ((!paused && !insufficient) || status == PS_ERROR || status == PS_COMPLETING || status == PS_COMPLETE || ignorepause)
		return status;
	else if (paused)
		return PS_PAUSED;
	else
		return PS_INSUFFICIENT;
	//MORPH END   - Added by IceCream, SLUGFILLER: checkDiskspace
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
	uint16	nOldTransSourceCount  = GetSrcStatisticsValue(DS_DOWNLOADING); 
	DWORD dwCurTick = ::GetTickCount();
	CUpDownClient* cur_src;

	// If buffer size exceeds limit, or if not written within time limit, flush data
	if ((m_nTotalBufferData > theApp.glob_prefs->GetFileBufferSize()) || (dwCurTick > (m_nLastBufferFlushTime + BUFFER_TIME_LIMIT))){
		// Avoid flushing while copying preview file
		if (!m_bPreviewing)
			FlushBuffer();
	}

	datarate = 0;

	// calculate datarate, set limit etc.
	if(m_icounter < 10){
		uint32 cur_datarate;
		for(POSITION pos = m_downloadingSourceList.GetHeadPosition();pos!=0;){
			cur_src = m_downloadingSourceList.GetNext(pos);
			if(cur_src && cur_src->GetDownloadState() == DS_DOWNLOADING){
				ASSERT( cur_src->socket );
				if (cur_src->socket){
						cur_datarate = cur_src->CalculateDownloadRate();
						datarate+=cur_datarate;
						//MORPH START - Added by IceCream, beta-patch by stobo about the download limit may stop downloads
						/*if(reducedownload)
						{
							uint32 limit = reducedownload*cur_datarate/1000;
							if(limit<1000 && reducedownload == 200)
								limit +=1000;
							else if(limit<1)
								limit = 1;
							cur_src->socket->SetDownloadLimit(limit);
						}*/
						if(reducedownload){
							uint32 limit = reducedownload*cur_datarate/1000;
							if(limit<1000 && reducedownload == 200)
								limit +=1000;
							else if(limit<30)
								limit = 30;
							cur_src->socket->SetDownloadLimit(limit);
						}
						//MORPH END   - Added by IceCream, beta-patch by stobo about the download limit may stop downloads
				}
			}
		}
	}
	else{

		// -khaos--+++> Moved this here, otherwise we were setting our permanent variables to 0 every tenth of a second...
		memset(m_anStates,0,sizeof(m_anStates));
		memset(src_stats,0,sizeof(src_stats));
		uint16 nCountForState;

		//MORPH START - Added by SiRoB, Spread Reask For Better SUC functioning and more
		uint16 AvailableSrcCount = this->GetAvailableSrcCount()+1;
		uint16 srcPosReask = 0;
		//MORPH END   - Added by SiRoB, Spread Reask For Better SUC functioning and more
		
		// khaos::kmod+ Smart A4AF Swapping
		bool b_SwapNNP = false;
		if ((dwCurTick-m_LastNoNeededCheck) > 10000) {
			m_LastNoNeededCheck = dwCurTick;
			b_SwapNNP = true;
		}
		// khaos::kmod-
		POSITION pos1, pos2;
		for (uint32 sl = 0; sl < SOURCESSLOTS; sl++){
			if (!srclists[sl].IsEmpty()){
				for (pos1 = srclists[sl].GetHeadPosition();( pos2 = pos1 ) != NULL;){
					srclists[sl].GetNext(pos1);
					cur_src = srclists[sl].GetAt(pos2);
					
					// BEGIN -rewritten- refreshing statistics (no need for temp vars since it is not multithreaded)
					nCountForState = cur_src->GetDownloadState();
					//special case which is not yet set as downloadstate
					if (nCountForState == DS_ONQUEUE){
						if( cur_src->IsRemoteQueueFull() )
							nCountForState = DS_REMOTEQUEUEFULL;
					}
					if (cur_src->IsBanned()) nCountForState=DS_BANNED;
					if (cur_src->GetSourceFrom()>=0 && cur_src->GetSourceFrom()<=2)
						++src_stats[cur_src->GetSourceFrom()];

					ASSERT( nCountForState < sizeof(m_anStates)/sizeof(m_anStates[0]) );
					m_anStates[nCountForState]++;
					switch (cur_src->GetDownloadState()){
						case DS_DOWNLOADING:{
							//MORPH - Added by Yun.SF3, ZZ Upload System
							uint32 curClientReducedDownload = reducedownload;
							if(cur_src->IsFriend() && cur_src->GetFriendSlot()) {
								curClientReducedDownload = friendReduceddownload;
								}
							//MORPH - Added by Yun.SF3, ZZ Upload System

							ASSERT( cur_src->socket );
							if (cur_src->socket){
								uint32 cur_datarate = cur_src->CalculateDownloadRate();
								datarate += cur_datarate;
								//MORPH - Added by Yun.SF3, ZZ Upload System
								if (curClientReducedDownload && cur_src->GetDownloadState() == DS_DOWNLOADING){
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
						case DS_BANNED:
						case DS_ERROR:
							break;	
						case DS_LOWTOLOWIP:	// if we now have a high ip we can ask
							if( ((dwCurTick - lastpurgetime) > 30000) && (this->GetSourceCount() >= (theApp.glob_prefs->GetMaxSourcePerFile()*.8 )) ){
								theApp.downloadqueue->RemoveSource( cur_src );
								lastpurgetime = dwCurTick;
								break;
							}
							if (theApp.IsFirewalled())
								break;
						case DS_NONEEDEDPARTS:{
							//MORPH - Changed by SiRoB, Due to Khaos A4AF
							/*// we try to purge noneeded source, even without reaching the limit
							if( cur_src->GetDownloadState() == DS_NONEEDEDPARTS && (dwCurTick - lastpurgetime) > 40000 ){
								if( !cur_src->SwapToAnotherFile( false , false, false , NULL ) ){
									//however we only delete them if reaching the limit
									if (GetSourceCount() >= (theApp.glob_prefs->GetMaxSourcePerFile()*.8 )){
										theApp.downloadqueue->RemoveSource( cur_src );
										lastpurgetime = dwCurTick;
										break; //Johnny-B - nothing more to do here (good eye!)
									}			
								}
								else{
									cur_src->DontSwapTo(this);
									lastpurgetime = dwCurTick;
									break;
								}
							}*/
							if( b_SwapNNP ) {
								if( cur_src->GetDownloadState() == DS_NONEEDEDPARTS && (dwCurTick - lastpurgetime) > 40000 ){
									if( !cur_src->SwapToAnotherFile( false ) ) {
										if (GetSourceCount() >= (theApp.glob_prefs->GetMaxSourcePerFile()*.8) ) {
											lastpurgetime = dwCurTick;
											theApp.downloadqueue->RemoveSource( cur_src );
											break; // This source was deleted!
										}
									}
									else break; // This source was transferred!
								}
								
							}
							// doubled reasktime for no needed parts - save connections and traffic
							if (!((!cur_src->GetLastAskedTime()) || (dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME*2))
								break; 
						}
						case DS_ONQUEUE:{
							if( cur_src->IsRemoteQueueFull() ) {
								if( ((dwCurTick - lastpurgetime) > 60000) && (this->GetSourceCount() >= (theApp.glob_prefs->GetMaxSourcePerFile()*.8 )) ){
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
									if (cur_src->SwapToForcedA4AF())
										break; // This source was transferred, nothing more to do here.
								}
								if (!cur_src->GetLastBalanceTick() || (dwCurTick - cur_src->GetLastBalanceTick()) > (uint32)GetRandRange(150000,180000)/* 2m30s to 3m */)
								{
									uint8 iA4AFMode = theApp.glob_prefs->AdvancedA4AFMode();
									if (iA4AFMode && theApp.glob_prefs->GetCategory(GetCategory())->iAdvA4AFMode)
										iA4AFMode = theApp.glob_prefs->GetCategory(GetCategory())->iAdvA4AFMode;

									if (iA4AFMode == 1 && cur_src->BalanceA4AFSources()) 
										break; // This source was transferred, nothing more to do here.
									if (iA4AFMode == 2 && cur_src->StackA4AFSources())
										break; // This source was transferred, nothing more to do here.
								}
							}
							//MORPH END - Added by SiRoB, Due to Khaos A4AF
							if (theApp.IsConnected() && ((!cur_src->GetLastAskedTime()) || (dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME-20000))
								cur_src->UDPReaskForDownload();

						}
						case DS_CONNECTING:
						case DS_TOOMANYCONNS:
						case DS_NONE:
						case DS_WAITCALLBACK:
							//MORPH START - Changed by SiRoB, Changed Spread Reask For Better SUC result and more
							//if (theApp.serverconnect->IsConnected() && ((!cur_src->GetLastAskedTime()) || (dwCurTick - cur_src->GetLastAskedTime()) > FILEREASKTIME)){
							if (theApp.IsConnected() && ((!cur_src->GetLastAskedTime()) || ((dwCurTick > (cur_src->GetLastAskedTime()+FILEREASKTIME)) &&  (dwCurTick % FILEREASKTIME > (UINT32)((FILEREASKTIME / AvailableSrcCount) * (srcPosReask++ % AvailableSrcCount)))))){
							//MORPH END   - Changed by SiRoB, Spread Reask For Better SUC functioning and more
								//MORPH START - Changed by SiRoB, Avoid deletion of source when not needed
								/*cur_src->AskForDownload(); // NOTE: This may *delete* the client!!
								cur_src = NULL; // TODO: implement some 'result' from 'AskForDownload' to know whether the client was deleted.*/
								cur_src->AskForDownload();
								if (!theApp.clientlist->IsValidClient(cur_src)){cur_src = NULL;}
								//MORPH END - Changed by SiRoB, Avoid deletion of source when not needed
							}
							break;
					}
				}
			}
		}
		//MORPH - Removed by SiRoB, Due to Khaos A4AF
		/*if (IsA4AFAuto() && ((!m_LastNoNeededCheck) || (dwCurTick - m_LastNoNeededCheck > 900000)) )
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
		}*/
		if( theApp.glob_prefs->GetMaxSourcePerFileUDP() > GetSourceCount()){
			if (theApp.downloadqueue->DoKademliaFileRequest() && (theApp.kademlia->getStatus()->m_totalFile < KADEMLIATOTALFILE) && (!lastsearchtimeKad || (dwCurTick - lastsearchtimeKad) > KADEMLIAREASKTIME) &&  theApp.kademlia->isConnected() && theApp.IsConnected() && !stopped){ //Once we can handle lowID users in ON, we remove the second IsConnected
				//Kademlia
				theApp.downloadqueue->SetLastKademliaFileRequest();
				lastsearchtimeKad = dwCurTick;
				if (Kademlia::CTimer::getThreadID() && !GetKadFileSearchID()){
					Kademlia::CUInt128 kadFileID;
					kadFileID.setValue(GetFileHash());
					Kademlia::CSearch* pSearch = Kademlia::CSearchManager::prepareFindFile(KademliaResultFileCallback, kadFileID);
					pSearch->setSearchTypes(Kademlia::CSearch::FILE);
					if (pSearch){
						uint32 nSearchID = pSearch->getSearchID();
						if (!PostThreadMessage(Kademlia::CTimer::getThreadID(), WM_KADEMLIA_STARTSEARCH, 0, (LPARAM)pSearch))
							Kademlia::CSearchManager::deleteSearch(pSearch);
						else
							SetKadFileSearchID(nSearchID);
					}
				}
			}
		}
		else{
			if(GetKadFileSearchID()){
				if (Kademlia::CTimer::getThreadID())
					VERIFY( PostThreadMessage(Kademlia::CTimer::getThreadID(), WM_KADEMLIA_STOPSEARCH, 0, GetKadFileSearchID()));
				SetKadFileSearchID(0);
			}
		}


		// check if we want new sources from server
		//uint16 test = theApp.glob_prefs->GetMaxSourcePerFileSoft();
		if ( !m_bLocalSrcReqQueued && ((!lastsearchtime) || (dwCurTick - lastsearchtime) > SERVERREASKTIME) && theApp.serverconnect->IsConnected()
			&& theApp.glob_prefs->GetMaxSourcePerFileSoft() > GetSourceCount() && !stopped )
		{
			m_bLocalSrcReqQueued = true;
			theApp.downloadqueue->SendLocalSrcRequest(this);
		}

		count++;
		if (count == 3){
			count = 0;
		// khaos::kmod+ Save/Load Sources
		if (theApp.glob_prefs->UseSaveLoadSources())
			m_sourcesaver.Process(this); //<<-- enkeyDEV(Ottavio84) -New SLS-
		// khaos::kmod-
			UpdateAutoDownPriority();
			UpdateDisplayedInfo();
			if(m_bPercentUpdated == false)
				UpdateCompletedInfos();
			m_bPercentUpdated = false;
		}
	}

	if ( GetSrcStatisticsValue(DS_DOWNLOADING) != nOldTransSourceCount ){
		// khaos::categorymod+
		//if (theApp.emuledlg->transferwnd.downloadlistctrl.curTab == 0)
		theApp.emuledlg->transferwnd.downloadlistctrl.ChangeCategory(theApp.emuledlg->transferwnd.GetCategoryTab()); 
		//else
		//	UpdateDisplayedInfo(true);
		// khaos::categorymod-
		if (theApp.glob_prefs->ShowCatTabInfos())
			theApp.emuledlg->transferwnd.UpdateCatTabTitles();
	}

	return datarate;
}

bool CPartFile::CanAddSource(uint32 userid, uint16 port, uint32 serverip, uint16 serverport, uint8* pdebug_lowiddropped)
{
	// MOD Note: Do not change this part - Merkur
	// check first if we are this source
	if (theApp.serverconnect->IsLowID() && theApp.serverconnect->IsConnected()){
		if ((theApp.serverconnect->GetClientID() == userid) && inet_addr(theApp.serverconnect->GetCurrentServer()->GetFullIP()) == serverip)
			return false;
	}
	else if (theApp.serverconnect->GetClientID() == userid){
#ifdef _DEBUG
		// bluecow - please do not remove this in the debug version - i need this for testing.
		if (theApp.serverconnect->IsLowID() || theApp.glob_prefs->GetPort() == port)
#endif
		return false;
	}
	else if (IsLowIDED2K(userid) && !theApp.serverconnect->IsLocalServer(serverip,serverport)){
		if (pdebug_lowiddropped)
			(*pdebug_lowiddropped)++;
		return false;
	}
	// MOD Note - end
	
	return true;
}

void CPartFile::AddSources(CMemFile* sources,uint32 serverip, uint16 serverport)
{
	uint8 count;
	sources->Read(&count,1);

	if (stopped){
		// since we may received multiple search source UDP results we have to "consume" all data of that packet
		sources->Seek(count*(4+2), SEEK_SET);
		return;
	}

	uint8 debug_lowiddropped = 0;
	uint8 debug_possiblesources = 0;
	for (int i = 0;i != count;i++){
		uint32 userid;
		sources->Read(&userid,4);
		uint16 port;
		sources->Read(&port,2);

		// "Filter LAN IPs" and "IPfilter" the received sources IP addresses
		if (userid >= 16777216){
			if (!IsGoodIP(userid)){ // check for 0-IP, localhost and optionally for LAN addresses
				AddDebugLogLine(false, _T(GetResString(IDS_IPIGNOREDSRV)), inet_ntoa(*(in_addr*)&userid));
				continue;
			}
			if (theApp.ipfilter->IsFiltered(userid)){
				AddDebugLogLine(false, _T(GetResString(IDS_IPFILTEREDSRV)), inet_ntoa(*(in_addr*)&userid), theApp.ipfilter->GetLastHit());
				theApp.stat_filteredclients++; //MORPH - Added by SiRoB, To comptabilise ipfiltered
				continue;
			}
		}

		if (!CanAddSource(userid, port, serverip, serverport, &debug_lowiddropped))
			continue;

		if( theApp.glob_prefs->GetMaxSourcePerFile() > this->GetSourceCount() ){
			debug_possiblesources++;
			CUpDownClient* newsource = new CUpDownClient(this,port,userid,serverip,serverport,true);
			theApp.downloadqueue->CheckAndAddSource(this,newsource);
		}
		else{
			// since we may received multiple search source UDP results we have to "consume" all data of that packet
			sources->Seek((count-i)*(4+2), SEEK_SET);

			//Kademlia::CSearchManager::stopSearch(kadFileSearchID);
			if(GetKadFileSearchID()){
				if (Kademlia::CTimer::getThreadID())
					VERIFY( PostThreadMessage(Kademlia::CTimer::getThreadID(), WM_KADEMLIA_STOPSEARCH, 0, GetKadFileSearchID()) );
				SetKadFileSearchID(0);
			}
			break;
		}
	}
	if ( theApp.glob_prefs->GetDebugSourceExchange() )
		AddDebugLogLine(false,"RCV: %i sources from server, %i low id dropped, %i possible sources File(%s)",count,debug_lowiddropped,debug_possiblesources, GetFileName());
}

// SLUGFILLER: heapsortCompletesrc
static void HeapSort(CArray<uint16,uint16> &count, int32 first, int32 last){
	int32 r;
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

void CPartFile::NewSrcPartsInfo(){
	// Cache part count
	uint16 partcount = GetPartCount();
	bool flag = (time(NULL) - m_nCompleteSourcesTime > 0); 

	CArray<uint16,uint16> count;	// SLUGFILLER: heapsortCompletesrc
	count.SetSize(0, m_ClientUploadList.GetSize());

	// Increase size if necessary
	if(m_SrcpartFrequency.GetSize() < partcount)
	{
		m_SrcpartFrequency.SetSize(partcount);
	}
	// Reset part counters
	for(int i = 0; i < partcount; i++)
	{
		m_SrcpartFrequency[i] = 0;
	}
	CUpDownClient* cur_src;
	
	uint16 cur_count = 0;
	for(int sl=0; sl<SOURCESSLOTS; ++sl) 
	{
		if (!srclists[sl].IsEmpty()) 
		{
			for (POSITION pos = srclists[sl].GetHeadPosition(); pos != 0; )
			{
				cur_src = srclists[sl].GetNext(pos);
				for (int i = 0; i < partcount; i++)
				{
					if (cur_src->IsPartAvailable(i))
					{
						m_SrcpartFrequency[i] +=1;
					}
				}
				cur_count= cur_src->GetUpCompleteSourcesCount();
				if ( flag && cur_count )
				{
					count.Add(cur_count);
				}
			}
		}
	}

	if( flag )
	{
		m_nCompleteSourcesCount = m_nCompleteSourcesCountLo = m_nCompleteSourcesCountHi = 0;
	
		for (uint16 i = 0; i < partcount; i++)
		{
			if( !i )
			{
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
			}
			else if( m_nCompleteSourcesCount > m_SrcpartFrequency[i])
			{
				m_nCompleteSourcesCount = m_SrcpartFrequency[i];
			}
		}
	
		if (m_nCompleteSourcesCount)
		{
			count.Add(m_nCompleteSourcesCount);
		}
	
		count.FreeExtra();
	
		int32 n = count.GetSize();
		if (n > 0)
		{
			// SLUGFILLER: heapsortCompletesrc
			int32 r;
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
			int32 i= n >> 1;		// (n / 2)
			int32 j= (n * 3) >> 2;	// (n * 3) / 4
			int32 k= (n * 7) >> 3;	// (n * 7) / 8
			if (n < 5)
			{
				m_nCompleteSourcesCount= count.GetAt(i);
				m_nCompleteSourcesCountLo= 0;
				m_nCompleteSourcesCountHi= m_nCompleteSourcesCount;
			}
			else if (n < 10)
			{
				m_nCompleteSourcesCount= count.GetAt(i);
				m_nCompleteSourcesCountLo= count.GetAt(i - 1);
				m_nCompleteSourcesCountHi= count.GetAt(i + 1);
			}
			else if (n < 20)
			{
				m_nCompleteSourcesCount= count.GetAt(i);
				m_nCompleteSourcesCountLo= count.GetAt(i);
				m_nCompleteSourcesCountHi= count.GetAt(j);
			}
			else
			{
				m_nCompleteSourcesCount= count.GetAt(j);
				m_nCompleteSourcesCountLo= m_nCompleteSourcesCount;
				m_nCompleteSourcesCountHi= count.GetAt(k);
			}
		}
		m_nCompleteSourcesTime = time(NULL) + (60);
	}
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	UpdatePowerShareLimit(m_nCompleteSourcesCountHi<21, m_nCompleteSourcesCountHi==1);
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	UpdateDisplayedInfo();
	//MORPH END   - Added by Yun.SF3, ZZ Upload System
	//MORPH START - Added by SiRoB,  SharedStatusBar CPU Optimisation
	InChangedSharedStatusBar = false;
	//MORPH END   - Added by SiRoB,  SharedStatusBar CPU Optimisation
}

//MORPH START - Added by IceCream, Maella about enhanced Chunk Selection v1.06
/*
bool CPartFile::GetNextRequestedBlock(CUpDownClient* sender,Requested_Block_Struct** newblocks,uint16* count){


	uint16 requestedCount = *count;
	if (requestedCount == 0)
		return false;
	uint16 newblockcount = 0;
	uint8* partsav = sender->GetPartStatus();
	if (partsav == NULL)
		return false;
	*count = 0;
	uint16 randomness;
	CList<int,int> liGoodParts;
	CList<int,int> liPossibleParts;
	bool finished = false;
	while (!finished)
	{
		// Need to empty lists to avoid infinite loop when file is smaller than 180k
		// Otherwise it would keep looping to find 3 blocks, there is only one and it is requested
		liGoodParts.RemoveAll();
		liPossibleParts.RemoveAll();
		// Barry - Top priority should be to continue downloading from current blocks (if anything left to download)
		bool foundPriorityPart = false;
		CString gettingParts;
		sender->ShowDownloadingParts(&gettingParts);

		for (int i=(gettingParts.GetLength()-1); i>=0; i--)
		{
			if ((gettingParts.GetAt(i) == 'Y') && (GetNextEmptyBlockInPart(i, 0)))
			{
				liGoodParts.AddHead(i);
				foundPriorityPart = true;
			}
		}

		// detect general rare parts
		if (!foundPriorityPart) {
			uint8 modif=10;
			if (GetSourceCount()>200) modif=5; else if (GetSourceCount()>800) modif=1;
			uint16 limit= modif*GetSourceCount()/ 100;
			if (limit==0) limit=1;
			for (int i = 0;i < sender->GetPartCount();++i){
				if (sender->IsPartAvailable(i) && m_SrcpartFrequency[i]<=limit ) {
					liGoodParts.AddHead(0);
				foundPriorityPart = true;
					break;
				}				
			}
		}

		// Barry - Give priority to end parts of archives and movies
		if ((!foundPriorityPart) && (IsArchive() || IsMovie()) && theApp.glob_prefs->GetPreviewPrio())
		{
			uint32 partCount = GetPartCount();
			// First part
			if (sender->IsPartAvailable(0) && GetNextEmptyBlockInPart(0, 0))
			{
				liGoodParts.AddHead(0);
				foundPriorityPart = true;
			}
			else if ((partCount > 1))
			{
				// Last part
				if (sender->IsPartAvailable(partCount-1) && GetNextEmptyBlockInPart(partCount-1, 0))
				{
					liGoodParts.AddHead(partCount-1);
					foundPriorityPart = true;
				}
				// Barry - Better to get rarest than these, add to list, but do not exclude all others.
				// These get priority over other parts with same availability.
				else if (partCount > 2)
				{
					// Second part
					if (sender->IsPartAvailable(1) && GetNextEmptyBlockInPart(1, 0))
						liGoodParts.AddHead(1);
					// Penultimate part
					else if (sender->IsPartAvailable(partCount-2) && GetNextEmptyBlockInPart(partCount-2, 0))
						liGoodParts.AddHead(partCount-2);
				}
			}
		}
		if (!foundPriorityPart)
		{
			randomness = (uint16)ROUND(((float)rand()/RAND_MAX)*(GetPartCount()-1));
			for (uint16 i = 0;i < GetPartCount();i++){
				if (sender->IsPartAvailable(randomness))
				{                                        
					if (partsav[randomness] && !IsComplete(randomness*PARTSIZE,((randomness+1)*PARTSIZE)-1)){
						if (IsPureGap(randomness*PARTSIZE,((randomness+1)*PARTSIZE)-1))
						{
							if (GetNextEmptyBlockInPart(randomness,0))
								liPossibleParts.AddHead(randomness);
						}
						else if (GetNextEmptyBlockInPart(randomness,0))
							liGoodParts.AddTail(randomness); // Barry - Add after archive/movie entries
					}
				}
				randomness++;
				if (randomness == GetPartCount())
					randomness = 0;
			}
		}
		CList<int,int>* usedlist;
		if (!liGoodParts.IsEmpty())
			usedlist = &liGoodParts;
		else if (!liPossibleParts.IsEmpty())
			usedlist = &liPossibleParts;
		else{
			if (!newblockcount){
				return false;
			}
			else
				break;
		}
		uint16 nRarest = 0xFFFF;
		uint16 usedpart = usedlist->GetHead();
		for (POSITION pos = usedlist->GetHeadPosition();pos != 0;usedlist->GetNext(pos)){
			if (m_SrcpartFrequency.GetCount() >= usedlist->GetAt(pos)
				&& m_SrcpartFrequency[usedlist->GetAt(pos)] < nRarest){
					nRarest = m_SrcpartFrequency[usedlist->GetAt(pos)];
					usedpart = usedlist->GetAt(pos);
				}
		}
		while (true){
			Requested_Block_Struct* block = new Requested_Block_Struct;
			if (GetNextEmptyBlockInPart(usedpart,block)){
				requestedblocks_list.AddTail(block);
				newblocks[newblockcount] = block;
				newblockcount++;
				if (newblockcount == requestedCount){
					finished = true;
					break;
				}
			}
			else
			{
				delete block;
				break;
			}
		}
	} //wend
	*count = newblockcount;
	return true;
}
*/


void  CPartFile::RemoveBlockFromList(uint32 start,uint32 end){
	POSITION pos1,pos2;
	for (pos1 = requestedblocks_list.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
	   requestedblocks_list.GetNext(pos1);
		if (requestedblocks_list.GetAt(pos2)->StartOffset <= start && requestedblocks_list.GetAt(pos2)->EndOffset >= end)
			requestedblocks_list.RemoveAt(pos2);
	}
}

void CPartFile::RemoveAllRequestedBlocks(void)
{
	requestedblocks_list.RemoveAll();
}

void CPartFile::CompleteFile(bool bIsHashingDone){
	if(GetKadFileSearchID()){
		if (Kademlia::CTimer::getThreadID())
			VERIFY( PostThreadMessage(Kademlia::CTimer::getThreadID(), WM_KADEMLIA_STOPSEARCH, 0, GetKadFileSearchID()) );
		SetKadFileSearchID(0);
	}
	if( this->srcarevisible )
		theApp.emuledlg->transferwnd.downloadlistctrl.HideSources(this);
	
	if (!bIsHashingDone){
		//m_hpartfile.Flush(); // flush the OS buffer before completing...
		SetStatus(PS_COMPLETING);
		datarate = 0;
		CAddFileThread* addfilethread = (CAddFileThread*) AfxBeginThread(RUNTIME_CLASS(CAddFileThread), THREAD_PRIORITY_BELOW_NORMAL,0, CREATE_SUSPENDED);
		addfilethread->SetValues(0,theApp.glob_prefs->GetTempDir(),RemoveFileExtension(m_partmetfilename),this);
		addfilethread->ResumeThread();	
		return;
	}
	else{
		StopFile(false); // khaos::kmod- Don't change the vars to true.
		UpdateDisplayedInfo(true); // khaos::kmod+ Show that the sources have been removed.
		SetStatus( PS_COMPLETING);
		//MORPH - Removed by SiRoB, Due to Khaos A4AF
		/*m_is_A4AF_auto=false;*/
		CWinThread *pThread = AfxBeginThread(CompleteThreadProc, this, THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED); // Lord KiRon - using threads for file completion
		if (pThread)
			pThread->ResumeThread();
		else
			throw GetResString(IDS_ERR_FILECOMPLETIONTHREAD);
	}
	theApp.emuledlg->transferwnd.downloadlistctrl.ShowFilesCount();
	if (theApp.glob_prefs->ShowCatTabInfos() ) theApp.emuledlg->transferwnd.UpdateCatTabTitles();
	UpdateDisplayedInfo(true);
}

UINT CPartFile::CompleteThreadProc(LPVOID pvParams) 
{ 
	DbgSetThreadName("PartFileComplete");
	CPartFile* pFile = (CPartFile*)pvParams;
	if (!pFile)
		return -1; 
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
		theApp.emuledlg->AddDebugLogLine(true, _T("Failed to open file \"%s\" for decompressing - %s"), pszFilePath, GetErrorMessage(GetLastError(), 1));
		return;
	}
	
	USHORT usInData = COMPRESSION_FORMAT_NONE;
	DWORD dwReturned = 0;
	if (!DeviceIoControl(hFile, FSCTL_SET_COMPRESSION, &usInData, sizeof usInData, NULL, 0, &dwReturned, NULL))
		theApp.emuledlg->AddDebugLogLine(true, _T("Failed to decompress file \"%s\" - %s"), pszFilePath, GetErrorMessage(GetLastError(), 1));
	CloseHandle(hFile);
}

// Lord KiRon - using threads for file completion
BOOL CPartFile::PerformFileComplete() 
{
	// khaos::compcorruptfix+ No longer needed.
	//theApp.glob_prefs->Add2LostFromCorruption(m_iLostDueToCorruption);
	//theApp.glob_prefs->Add2SavedFromCompression(m_iGainDueToCompression);
	// khaos::compcorruptfix-

	// If that function is invoked from within the file completion thread, it's ok if we wait (and block) the thread.
	CSingleLock(&m_FileCompleteMutex,TRUE); // will be unlocked on exit
	CString strPartfilename(RemoveFileExtension(m_fullname));
	char* newfilename = nstrdup(GetFileName());
	strcpy(newfilename, (LPCTSTR)theApp.StripInvalidFilenameChars(newfilename));

	CString strNewname;
	CString indir;

	if (PathFileExists( theApp.glob_prefs->GetCategory(GetCategory())->incomingpath)) {
		indir=theApp.glob_prefs->GetCategory(GetCategory())->incomingpath;
		strNewname.Format("%s\\%s",indir,newfilename);
	} else{
		indir=theApp.glob_prefs->GetIncomingDir();
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
		AddLogLine(true, GetResString(IDS_ERR_FILEERROR), m_partmetfilename, GetFileName(), buffer);
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
			strTestName.Format("%s\\%s(%d).%s", indir,
			newfilename, namecount, min(ext + 1, newfilename + length));
		} while(PathFileExists(strTestName));
		strNewname = strTestName;
	}
	delete[] newfilename;

	if (rename(strPartfilename,strNewname)){
		if (this){
			if (errno == ENOENT && strNewname.GetLength() >= MAX_PATH)
				AddLogLine(true,GetResString(IDS_ERR_COMPLETIONFAILED) + _T(" - Path too long"),GetFileName());
			else
				AddLogLine(true,GetResString(IDS_ERR_COMPLETIONFAILED) + _T(" - ") + CString(strerror(errno)),GetFileName());
		}
		paused = true;
		SetStatus(PS_ERROR);
		// khaos::categorymod+ Support for standard resume.
		theApp.downloadqueue->StartNextFile(theApp.glob_prefs->GetResumeSameCat()?GetCategory():-1);
		// khaos::categorymod-
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
		date = st.st_mtime;

	if (remove(m_fullname))
		AddLogLine(true,GetResString(IDS_ERR_DELETEFAILED) + _T(" - ") + CString(strerror(errno)),m_fullname);
	// khaos::kmod+ Save/Load Sources
	else
        m_sourcesaver.DeleteFile(this); //<<-- enkeyDEV(Ottavio84) -New SLS-
	// khaos::kmod-

	CString BAKName(m_fullname);
	BAKName.Append(PARTMET_BAK_EXT);
	if (_taccess(BAKName, 0) == 0 && !::DeleteFile(BAKName))
		AddLogLine(true,GetResString(IDS_ERR_DELETE) + _T(" - ") + GetErrorMessage(GetLastError()), BAKName);

	BAKName = m_fullname;
	BAKName.Append(PARTMET_TMP_EXT);
	if (_taccess(BAKName, 0) == 0 && !::DeleteFile(BAKName))
		AddLogLine(true,GetResString(IDS_ERR_DELETE) + _T(" - ") + GetErrorMessage(GetLastError()), BAKName);

	m_fullname = strNewname;
	SetPath(indir);
	SetFilePath(m_fullname);
	SetStatus( PS_COMPLETE);
	// paused = false; // khaos::kmod+ No longer necessary.
	AddLogLine(true,GetResString(IDS_DOWNLOADDONE),GetFileName());
	theApp.emuledlg->ShowNotifier(GetResString(IDS_TBN_DOWNLOADDONE)+"\n"+GetFileName(), TBN_DLOAD);
	if (renamed)
		AddLogLine(true, GetResString(IDS_DOWNLOADRENAMED), strrchr(strNewname, '\\') + 1);
	theApp.knownfiles->SafeAddKFile(this);
	theApp.downloadqueue->RemoveFile(this);
	// mobilemule
	theApp.mmserver->AddFinishedFile(this);
	// end mobilemule
	UpdateDisplayedInfo(true);
	theApp.emuledlg->transferwnd.downloadlistctrl.ShowFilesCount();
	//SHAddToRecentDocs(SHARD_PATH, fullname); // This is a real nasty call that takes ~110 ms on my 1.4 GHz Athlon and isn't really needed afai see...[ozon]

	// Barry - Just in case
	//		transfered = m_nFileSize;
	// khaos::categorymod+
	theApp.downloadqueue->StartNextFile(theApp.glob_prefs->GetResumeSameCat()?GetCategory():-1);
	// khaos::categorymod-

	// -khaos--+++> Extended Statistics Added 2-10-03
	theApp.glob_prefs->Add2DownCompletedFiles();		// Increments cumDownCompletedFiles in prefs struct
	theApp.glob_prefs->Add2DownSessionCompletedFiles(); // Increments sesDownCompletedFiles in prefs struct
	theApp.glob_prefs->SaveCompletedDownloadsStat();	// Saves cumDownCompletedFiles to INI
	// <-----khaos- End Statistics Modifications

	GetMetaDataTags();

	return TRUE;
}

void  CPartFile::RemoveAllSources(bool bTryToSwap){
	POSITION pos1,pos2;
	for (int sl=0;sl<SOURCESSLOTS;sl++){
		if (!srclists[sl].IsEmpty()){
			for( pos1 = srclists[sl].GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
				srclists[sl].GetNext(pos1);
				if (bTryToSwap){
					//MORPH START - Changed by SiRoB, Due to Khaos A4AF
					if (!srclists[sl].GetAt(pos2)->SwapToAnotherFile(true))
						theApp.downloadqueue->RemoveSource(srclists[sl].GetAt(pos2));
					//MORPH END   - Changed by SiRoB, Due to Khaos A4AF
				}
				else
					theApp.downloadqueue->RemoveSource(srclists[sl].GetAt(pos2));
			}
		}
	}
	NewSrcPartsInfo(); 
	UpdateAvailablePartsCount();
	//MORPH START - Added by SiRoB, A4AF counter
	CleanA4AFSource(this);
	//MORPH END   - Added by SiRoB, A4AF counter
	UpdateFileRatingCommentAvail();
}

void CPartFile::DeleteFile(){
	ASSERT ( !m_bPreviewing );

	// Barry - Need to tell any connected clients to stop sending the file
	StopFile();

	theApp.sharedfiles->RemoveFile(this);
	theApp.downloadqueue->RemoveFile(this);
	theApp.emuledlg->transferwnd.downloadlistctrl.RemoveFile(this);

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

bool CPartFile::HashSinglePart(uint16 partnumber)
{
	if ((GetHashCount() <= partnumber) && (GetPartCount() > 1)){
		AddLogLine(true,GetResString(IDS_ERR_HASHERRORWARNING),GetFileName());
		this->hashsetneeded = true;
		return true;
	}
	else if(!GetPartHash(partnumber) && GetPartCount() != 1){
		AddLogLine(true,GetResString(IDS_ERR_INCOMPLETEHASH),GetFileName());
		this->hashsetneeded = true;
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

bool CPartFile::IsCorruptedPart(uint16 partnumber){
	return corrupted_list.Find(partnumber);
}

// Barry - Also want to preview zip/rar files
bool CPartFile::IsArchive(bool onlyPreviewable)
{
	CString extension = CString(GetFileName()).Right(4);
	
	if (onlyPreviewable) return ((extension.CompareNoCase(".zip") == 0) || (extension.CompareNoCase(".rar") == 0));
	
	return (ED2KFT_ARCHIVE == GetED2KFileTypeID(GetFileName()));
}

//MORPH START - Added by IceCream, music preview and defeat 0-filler
bool CPartFile::IsMovie(){
	return (ED2KFT_VIDEO == GetED2KFileTypeID(GetFileName()) );
}

bool CPartFile::IsMusic(){
	return (ED2KFT_AUDIO == GetED2KFileTypeID(GetFileName()) );
}

bool CPartFile::IsCDImage(){
	return (ED2KFT_CDIMAGE == GetED2KFileTypeID(GetFileName()) );
}

bool CPartFile::IsDocument(){
	return (ED2KFT_DOCUMENT == GetED2KFileTypeID(GetFileName()) );
}
//MORPH END   - Added by IceCream, music preview and defeat 0-filler

void CPartFile::SetDownPriority(uint8 np){
	m_iDownPriority = np;
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace(); // SLUGFILLER: checkDiskspace
	UpdateDisplayedInfo(true);
	SavePartFile();
}

// khaos::kmod+ Added param to set the paused and stopped vars...
void CPartFile::StopFile(bool setVars){
	// khaos::kmod-
	// Barry - Need to tell any connected clients to stop sending the file
	PauseFile();
	lastsearchtimeKad = 0;
	RemoveAllSources(true);
	// khaos::kmod+ setvars...  Fix for completed files not showing up when "stopped" is deseleted in view filters.
	paused = setVars;
	stopped = setVars;
	// khaos::kmod-
	datarate = 0;
	memset(m_anStates,0,sizeof(m_anStates));
	FlushBuffer();
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace();	// SLUGFILLER: checkDiskspace
	UpdateDisplayedInfo(true);
}

void CPartFile::PauseFile(){
	if(GetKadFileSearchID()){
		if (Kademlia::CTimer::getThreadID())
			VERIFY( PostThreadMessage(Kademlia::CTimer::getThreadID(), WM_KADEMLIA_STOPSEARCH, 0, GetKadFileSearchID()));
		SetKadFileSearchID(0);
		lastsearchtimeKad = 0; //If we were in the middle of searching, reset timer so they can resume searching.
	}

	if (status==PS_COMPLETE || status==PS_COMPLETING)
		return;
	Packet* packet = new Packet(OP_CANCELTRANSFER,0);
	for (int sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty())
		for( POSITION pos = srclists[sl].GetHeadPosition(); pos != NULL; ){
			CUpDownClient* cur_src = srclists[sl].GetNext(pos);
			if (cur_src->GetDownloadState() == DS_DOWNLOADING){
				theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
				cur_src->socket->SendPacket(packet,false,true);
				cur_src->SetDownloadState(DS_ONQUEUE);
			}
		}
	delete packet;
	paused = true;
	SetStatus(status); // to update item
	datarate = 0;
	m_anStates[DS_DOWNLOADING] = 0; // -khaos--+++> Renamed var.
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace(); // SLUGFILLER: checkDiskspace
	UpdateDisplayedInfo(true);
	SavePartFile();
}

void CPartFile::ResumeFile(){
	if (status==PS_COMPLETE || status==PS_COMPLETING)
		return;
	paused = false;
	stopped = false;
	lastsearchtime = 0;
	theApp.downloadqueue->SortByPriority();
	theApp.downloadqueue->CheckDiskspace(); // SLUGFILLER: checkDiskspace
	SavePartFile();
	//theApp.emuledlg->transferwnd.downloadlistctrl.UpdateItem(this);
	UpdateDisplayedInfo(true);
}

//MORPH START - Added by IceCream, SLUGFILLER: checkDiskspace
void CPartFile::PauseFileInsufficient(){
	if (status==PS_COMPLETE || status==PS_COMPLETING)
		return;
	Packet* packet = new Packet(OP_CANCELTRANSFER,0);
	for (int sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty())
		for( POSITION pos = srclists[sl].GetHeadPosition(); pos != NULL; ){
			CUpDownClient* cur_src = srclists[sl].GetNext(pos);
			if (cur_src->GetDownloadState() == DS_DOWNLOADING){
				theApp.uploadqueue->AddUpDataOverheadOther(packet->size);
				cur_src->socket->SendPacket(packet,false,true);
				cur_src->SetDownloadState(DS_ONQUEUE);
			}
		}
	delete packet;
	insufficient = true;
	SetStatus(status); // to update item
	datarate = 0;
	m_anStates[DS_DOWNLOADING] = 0; // -khaos--+++> Renamed var.
	UpdateDisplayedInfo(true);
}

void CPartFile::ResumeFileInsufficient(){
	if (status==PS_COMPLETE || status==PS_COMPLETING)
		return;
	if (!insufficient)
		return;
	insufficient = false;
	lastsearchtime = 0;
	//theApp.emuledlg->transferwnd.downloadlistctrl.UpdateItem(this);
	UpdateDisplayedInfo(true);
}
//MORPH END   - Added by IceCream, SLUGFILLER: checkDiskspace

CString CPartFile::getPartfileStatus(){ 

	switch(GetStatus()){
		case PS_HASHING:
		case PS_WAITINGFORHASH:
			return GetResString(IDS_HASHING);
		case PS_COMPLETING:
			return GetResString(IDS_COMPLETING);
		case PS_COMPLETE:
			return GetResString(IDS_COMPLETE);
		case PS_PAUSED:
			return GetResString(IDS_PAUSED);
		// SLUGFILLER: checkDiskspace
		case PS_INSUFFICIENT:
			return GetResString(IDS_INSUFFICIENT);
		// SLUGFILLER: checkDiskspace
		case PS_ERROR:
			return GetResString(IDS_ERRORLIKE);
	}
	if(GetSrcStatisticsValue(DS_DOWNLOADING) > 0)
		return GetResString(IDS_DOWNLOADING);
	else
		return GetResString(IDS_WAITING);
} 

//MORPH START - Added by Yun.SF3, ZZ Upload System
int CPartFile::getPartfileStatusRang(){ 
	
	int tempstatus=2;
	if (GetSrcStatisticsValue(DS_DOWNLOADING) == 0)
		tempstatus=3;
	switch (GetStatus()) {
		case PS_HASHING: 
		case PS_WAITINGFORHASH:
			tempstatus=5;
			break; 
		case PS_COMPLETING:
			tempstatus=1;
			break; 
		case PS_COMPLETE:
			tempstatus=0;
			break; 
		case PS_PAUSED:
		case PS_INSUFFICIENT:	//MORPH - Added by IceCream, SLUGFILLER: checkDiskspace
			tempstatus=4;
			break; 
		case PS_ERROR:
			tempstatus=6;
			break;
	}

	return tempstatus;
} 
//MORPH END - Added by Yun.SF3, ZZ Upload System

//MORPH START - Added by Yun.SF3, ZZ Upload System
sint32 CPartFile::getTimeRemaining(){ 
	if (GetDatarate()<=0) return -1;
	
	return( (GetFileSize()-GetCompletedSize())/ GetDatarate());
}
//MORPH END - Added by Yun.SF3, ZZ Upload System


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

	if (!PreviewAvailable()){
		ASSERT( false );
		return;
	}


	if (theApp.glob_prefs->IsMoviePreviewBackup()){
		m_bPreviewing = true;
		CPreviewThread* pThread = (CPreviewThread*) AfxBeginThread(RUNTIME_CLASS(CPreviewThread), THREAD_PRIORITY_NORMAL,0, CREATE_SUSPENDED);
		pThread->SetValues(this,theApp.glob_prefs->GetVideoPlayer());
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

		if (!theApp.glob_prefs->GetVideoPlayer().IsEmpty())
		{
			// get directory of video player application
		CString path = theApp.glob_prefs->GetVideoPlayer();
		int pos = path.ReverseFind(_T('\\'));
		if (pos == -1)
			path.Empty();
		else
			path = path.Left(pos + 1);

			if (theApp.glob_prefs->GetPreviewSmallBlocks())
				FlushBuffer();

			ShellExecute(NULL, _T("open"), theApp.glob_prefs->GetVideoPlayer(), strLine, path, SW_SHOWNORMAL);
		}
		else
			ShellExecute(NULL, _T("open"), strLine, NULL, NULL, SW_SHOWNORMAL);
	}
}

bool CPartFile::PreviewAvailable()
{
	uint64 space = GetFreeDiskSpaceX(theApp.glob_prefs->GetTempDir());
	// Barry - Allow preview of archives of any length > 1k
	if (IsArchive(true)) {
		if (GetStatus() != PS_COMPLETE && GetStatus() != PS_COMPLETING && GetFileSize()>1024 && GetCompletedSize()>1024 && (!m_bRecoveringArchive) && ((space + 100000000) > (2*GetFileSize())))
			return true;
		else
			return false;
	}
	//MORPH START - Aded by SiRoB, preview music file
	if (IsMusic())
		if (GetStatus() != PS_COMPLETE &&  GetStatus() != PS_COMPLETING && GetFileSize()>1024 && GetCompletedSize()>1024 && ((space + 100000000) > (2*GetFileSize())))
			return true;
	//MORPH START - Aded by SiRoB, preview music file
	
	if (theApp.glob_prefs->IsMoviePreviewBackup())
		//MORPH - Changed by SiRoB, Authorize preview of files with 2 chunk available
		return !( (GetStatus() != PS_READY && GetStatus() != PS_PAUSED) 
			|| m_bPreviewing || GetPartCount() < 2 || !IsMovie() || (space + 100000000) < GetFileSize()
			|| ( !IsComplete(0,PARTSIZE-1) || !IsComplete(PARTSIZE*(GetPartCount()-1),GetFileSize()-1)));
	else
	{
		TCHAR szVideoPlayerFileName[_MAX_FNAME];
		_tsplitpath(theApp.glob_prefs->GetVideoPlayer(), NULL, NULL, szVideoPlayerFileName, NULL);

		// enable the preview command if the according option is specified 'PreviewSmallBlocks' 
		// or if VideoLAN client is specified
		if (theApp.glob_prefs->GetPreviewSmallBlocks() || !_tcsicmp(szVideoPlayerFileName, _T("vlc"))){
		    if (m_bPreviewing)
			    return false;

		    uint8 uState = GetStatus();
		    if (!(uState == PS_READY || uState == PS_EMPTY || uState == PS_PAUSED))
			    return false;
		    
			// default: check the ED2K file format to be of type audio, video or CD image. 
			// but because this could disable the preview command for some file types which eMule does not know,
			// this test can be avoided by specifying 'PreviewSmallBlocks=2'
			if (theApp.glob_prefs->GetPreviewSmallBlocks() <= 1)
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
				    || m_bPreviewing || GetPartCount() < 2 || !IsMovie() || !IsComplete(0,PARTSIZE-1)); 
		}
	}
}

void CPartFile::UpdateAvailablePartsCount(){
	uint16 availablecounter = 0;
	bool breakflag = false;
	uint16 iPartCount = GetPartCount();
	for (uint32 ixPart = 0; ixPart < iPartCount; ixPart++){
		breakflag = false;
		for (uint32 sl = 0; sl < SOURCESSLOTS && !breakflag; sl++){
			if (!srclists[sl].IsEmpty()){
				for(POSITION pos = srclists[sl].GetHeadPosition(); pos && !breakflag; ){
					if (srclists[sl].GetNext(pos)->IsPartAvailable(ixPart)){ 
						availablecounter++; 
						breakflag = true;
					}
				}
			}
		}
	}
	if (iPartCount == availablecounter && availablePartsCount < iPartCount)
		lastseencomplete = CTime::GetCurrentTime();
	availablePartsCount = availablecounter;
}

Packet*	CPartFile::CreateSrcInfoPacket(CUpDownClient* forClient){

	if (forClient->reqfile != this)
		return NULL;

	int sl;
	bool empty=true;
	for (sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty()) {empty=false;break;}
	if (empty) return 0;

	CMemFile data;
	uint16 nCount = 0;

	data.Write(m_abyFileHash,16);
	data.Write(&nCount,2);
	bool bNeeded;
	for (sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty())
	for (POSITION pos = srclists[sl].GetHeadPosition();pos != 0;srclists[sl].GetNext(pos)){
		bNeeded = false;
		CUpDownClient* cur_src = srclists[sl].GetAt(pos);
		if (cur_src->HasLowID())
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
					// if we don't know the need parts for this client, return any source
					// currently a client sends it's file status only after it has at least one complete part,
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
				DEBUG_ONLY(AddDebugLogLine(false,"*** CPartFile::CreateSrcInfoPacket - found source (%u) with wrong partcount (%u) attached to partfile \"%s\" (partcount=%u)", cur_src->GetUserIDHybrid(), cur_src->GetPartCount(), GetFileName(), GetPartCount()));
			}
		}
		if( bNeeded ){
			nCount++;
			uint32 dwID;
			if(forClient->GetSourceExchangeVersion() > 2)
				dwID = cur_src->GetUserIDHybrid();
			else
				dwID = ntohl(cur_src->GetUserIDHybrid());
			uint16 nPort = cur_src->GetUserPort();
			uint32 dwServerIP = cur_src->GetServerIP();
			uint16 nServerPort = cur_src->GetServerPort();
			data.Write(&dwID, 4);
			data.Write(&nPort, 2);
			data.Write(&dwServerIP, 4);
			data.Write(&nServerPort, 2);
			if (forClient->GetSourceExchangeVersion() > 1)
				data.Write(cur_src->GetUserHash(),16);
			if (nCount > 50/*500*/) //MORPH - HotFix by SiRoB about too much source exchanged with compression fix
				break;
		}
	}
	if (!nCount)
		return 0;
	data.Seek(16, 0);
	data.Write(&nCount, 2);

	Packet* result = new Packet(&data, OP_EMULEPROT);
	result->opcode = OP_ANSWERSOURCES;
	// 16+2+501*(4+2+4+2+16) = 15048 bytes max.
	if ( result->size > 354 )
		result->PackPacket();
	if ( theApp.glob_prefs->GetDebugSourceExchange() )
		AddDebugLogLine( false, "Send:Source User(%s) File(%s) Count(%i)", forClient->GetUserName(), GetFileName(), nCount );
	return result;
}

void CPartFile::AddClientSources(CMemFile* sources, uint8 sourceexchangeversion){
	if (stopped) return;
	uint16 nCount;
	sources->Read(&nCount,2);

	// Check if the data size matches the 'nCount' for v1 or v2 and eventually correct the source
	// exchange version while reading the packet data. Otherwise we could experience a higher
	// chance in dealing with wrong source data, userhashs and finally duplicate sources.
	UINT uDataSize = sources->GetLength() - sources->GetPosition();
	//Checks if version 1 packet is correct size
	if ((UINT)nCount*(4+2+4+2) == uDataSize){
		if( sourceexchangeversion != 1 )
			return;
	}
	//Checks if version 2&3 packet is correct size
	else if ((UINT)nCount*(4+2+4+2+16) == uDataSize){
		if( sourceexchangeversion == 1 )
			return;
	}
	else{
		// If v4 inserts additional data (like v2), the above code will correctly filter those packets.
		// If v4 appends additional data after <count>(<Sources>)[count], we are in trouble with the 
		// above code. Though a client which does not understand v4+ should never receive such a packet.
		AddDebugLogLine(false, "Received invalid source exchange packet (v%u) of data size %u for %s", sourceexchangeversion, uDataSize, GetFileName());
		return;
	}

	if ( theApp.glob_prefs->GetDebugSourceExchange() )
		AddDebugLogLine( false, "RCV:Sources File(%s) Count(%i)", GetFileName(), nCount );

	for (int i = 0;i != nCount;i++){
		uint32 dwID;
		uint16 nPort;
		uint32 dwServerIP;
		uint16 nServerPort;
		uchar achUserHash[16];
		sources->Read(&dwID,4);
		sources->Read(&nPort,2);
		sources->Read(&dwServerIP,4);
		sources->Read(&nServerPort,2);
		if (sourceexchangeversion > 1)
			sources->Read(achUserHash,16);

		if( sourceexchangeversion == 3 ){
			uint32 dwIDtemp = ntohl(dwID);
			// "Filter LAN IPs" and "IPfilter" the received sources IP addresses
			if (!IsLowIDHybrid(dwID)){
				if (!IsGoodIP(dwIDtemp)){ // check for 0-IP, localhost and optionally for LAN addresses
					AddDebugLogLine(false, _T("Ignored source (IP=%s) received via source exchange"), inet_ntoa(*(in_addr*)&dwIDtemp));
					continue;
				}
				if (theApp.ipfilter->IsFiltered(dwIDtemp)){
					AddDebugLogLine(false, _T("IPfiltered source IP=%s (%s) received via source exchange"), inet_ntoa(*(in_addr*)&dwIDtemp), theApp.ipfilter->GetLastHit());
					continue;
				}
			}
			else{
				continue;
			}
	
			// check if we are this source
			if (theApp.serverconnect->IsLowID() && theApp.serverconnect->IsConnected()){
				if ((theApp.serverconnect->GetClientID() == dwIDtemp) && theApp.serverconnect->GetCurrentServer()->GetIP() == dwServerIP)
					continue;
				// although we are currently having a LowID, we could have had a HighID before, which
				// that client is sending us now! seems unlikely ... it happend!
				if (dwIDtemp == theApp.serverconnect->GetLocalIP())
					continue;
			}
			else if (theApp.serverconnect->GetClientID() == dwIDtemp){
				continue;
			}
		}
		else{
		// "Filter LAN IPs" and "IPfilter" the received sources IP addresses
			if (!IsLowIDED2K(dwID)){
			if (!IsGoodIP(dwID)){ // check for 0-IP, localhost and optionally for LAN addresses
				AddDebugLogLine(false, _T(GetResString(IDS_IPIGNOREDSE)), inet_ntoa(*(in_addr*)&dwID));
				continue;
			}
			if (theApp.ipfilter->IsFiltered(dwID)){
				AddDebugLogLine(false, _T(GetResString(IDS_IPFILTEREDSE)), inet_ntoa(*(in_addr*)&dwID), theApp.ipfilter->GetLastHit());
				theApp.stat_filteredclients++; //MORPH - Added by SiRoB, To comptabilise ipfiltered
				continue;
			}
		}
			else{
				continue;
			}

		// check if we are this source
			if (theApp.serverconnect->IsLowID() && theApp.serverconnect->IsConnected()){
			if ((theApp.serverconnect->GetClientID() == dwID) && theApp.serverconnect->GetCurrentServer()->GetIP() == dwServerIP)
				continue;
			// although we are currently having a LowID, we could have had a HighID before, which
			// that client is sending us now! seems unlikely ... it happend!
			if (dwID == theApp.serverconnect->GetLocalIP())
				continue;
		}
		else if (theApp.serverconnect->GetClientID() == dwID){
			continue;
		}
		}
		if( theApp.glob_prefs->GetMaxSourcePerFile() > this->GetSourceCount() ){
			CUpDownClient* newsource;
			if( sourceexchangeversion == 3 )
				newsource = new CUpDownClient(this,nPort,dwID,dwServerIP,nServerPort,false);
			else
				newsource = new CUpDownClient(this,nPort,dwID,dwServerIP,nServerPort,true);
			if (sourceexchangeversion > 1)
				newsource->SetUserHash(achUserHash);
			newsource->SetSourceFrom(2);
			theApp.downloadqueue->CheckAndAddSource(this,newsource);
		} 
		else
			break;
	}
}

// making this function return a higher when more sources have the extended
// protocol will force you to ask a larger variety of people for sources
int CPartFile::GetCommonFilePenalty() {
	//TODO: implement, but never return less than MINCOMMONPENALTY!
	return MINCOMMONPENALTY;
}

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
	// Increment transfered bytes counter for this file
	transfered += transize;

	// This is needed a few times
	uint32 lenData = end - start + 1;

	// -khaos--+++>
	if( lenData > transize ) {
		m_iSesCompressionBytes += lenData-transize;
		theApp.glob_prefs->Add2SavedFromCompression(lenData-transize);
	}
	// <-----khaos-

	// Occasionally packets are duplicated, no point writing it twice
	if (IsComplete(start, end))
	{
		AddDebugLogLine(false, "File \"%s\" has already been written from %lu to %lu", GetFileName(), start, end);
		return 0;
	}

	// Create copy of data as new buffer
	BYTE *buffer = new BYTE[lenData];
	memcpy(buffer, data, lenData);

		// Create a new buffered queue entry
		PartFileBufferedData *item = new PartFileBufferedData;
	item->data = buffer;
	item->start = start;
	item->end = end;
	item->block = block;

	// Add to the queue in the correct position (most likely the end)
	PartFileBufferedData *queueItem;
	bool added = false;
	POSITION pos = m_BufferedData_list.GetTailPosition();
	while (pos != NULL)
	{	
		queueItem = m_BufferedData_list.GetPrev(pos);
		if (item->end > queueItem->end)
		{
			added = true;
			m_BufferedData_list.InsertAfter(pos, item);
			break;
		}
	}
	if (!added)
		m_BufferedData_list.AddHead(item);

	// Increment buffer size marker
	m_nTotalBufferData += lenData;

	// Mark this small section of the file as filled
	FillGap(item->start, item->end);

	// Update the flushed mark on the requested block 
	// The loop here is unfortunate but necessary to detect deleted blocks.
	pos = requestedblocks_list.GetHeadPosition();
	while (pos != NULL)
	{	
		if (requestedblocks_list.GetNext(pos) == item->block)
			item->block->transferred += lenData;
	}

	if (gaplist.IsEmpty()) FlushBuffer();

	// Return the length of data written to the buffer
	return lenData;
}

void CPartFile::FlushBuffer(void)
{
	m_nLastBufferFlushTime = GetTickCount();
	if (m_BufferedData_list.IsEmpty())
		return;

	uint32 partCount = GetPartCount();
	bool *changedPart = new bool[partCount];

//	theApp.emuledlg->AddDebugLogLine(false, "Flushing file %s - buffer size = %ld bytes (%ld queued items) transfered = %ld [time = %ld]\n", GetFileName(), m_nTotalBufferData, m_BufferedData_list.GetCount(), transfered, m_nLastBufferFlushTime);

	try
	{
		// Remember which parts need to be checked at the end of the flush
		for (int partNumber=0; (uint32)partNumber<partCount; partNumber++)
		{
			changedPart[partNumber] = false;
		}

		// Ensure file is big enough to write data to (the last item will be the furthest from the start)
		PartFileBufferedData *item = m_BufferedData_list.GetTail();
		if (m_hpartfile.GetLength() <= item->end)
			m_hpartfile.SetLength(item->end + 1);

		// Loop through queue
		for (int i = m_BufferedData_list.GetCount(); i>0; i--)
		{
			// Get top item
			item = m_BufferedData_list.GetHead();

			// This is needed a few times
			uint32 lenData = item->end - item->start + 1;

			/*int curpart = item->start/PARTSIZE;
			changedPart[curpart] = true;*/
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

		// Check each part of the file
		uint32 partRange = (m_hpartfile.GetLength() % PARTSIZE) - 1;
		for (int partNumber = partCount-1; partNumber >= 0; partNumber--)
		{
			if (changedPart[partNumber] == false)
			{
				// Any parts other than last must be full size
				partRange = PARTSIZE - 1;
				continue;
			}

			// Is this 9MB part complete
			if ( IsComplete(PARTSIZE * partNumber, (PARTSIZE * (partNumber + 1)) - 1 ) )
			{
				// Is part corrupt
				if (!HashSinglePart(partNumber))
				{
					AddLogLine(true, GetResString(IDS_ERR_PARTCORRUPT), partNumber, GetFileName());
					AddGap(PARTSIZE*partNumber, (PARTSIZE*partNumber + partRange));
					corrupted_list.AddTail(partNumber);
					// Reduce transfered amount by corrupt amount
					this->m_iLostDueToCorruption += (partRange + 1);
					theApp.glob_prefs->Add2LostFromCorruption(partRange + 1);
				}
				else
				{
					if (!hashsetneeded)
						AddDebugLogLine(false, "Finished part %u of \"%s\"", partNumber, GetFileName());

					// Successfully completed part, make it available for sharing
					if (status == PS_EMPTY)
					{
						SetStatus(PS_READY);
						if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
							theApp.sharedfiles->SafeAddKFile(this);
					}
				}
			}
			else if (IsCorruptedPart(partNumber) && theApp.glob_prefs->IsICHEnabled())
			{
				// Try to recover with minimal loss
				if (HashSinglePart(partNumber))
				{
					++m_iTotalPacketsSavedDueToICH;
					theApp.glob_prefs->Add2SessionPartsSavedByICH(1);
					FillGap(PARTSIZE*partNumber,(PARTSIZE*partNumber+partRange));
					RemoveBlockFromList(PARTSIZE*partNumber,(PARTSIZE*partNumber + partRange));
					AddLogLine(true,GetResString(IDS_ICHWORKED),partNumber,GetFileName());
				}
			}
			// Any parts other than last must be full size
			partRange = PARTSIZE - 1;
		}

		// Update met file
		SavePartFile();

		// Is this file finished?
		if (theApp.emuledlg->IsRunning()){ // may be called during shutdown!
			if (gaplist.IsEmpty())
				CompleteFile(false);
	}
	}
	catch (CFileException* error)
	{
		OUTPUT_DEBUG_TRACE();

		// SLUGFILLER: checkDiskspace - Shouldn't happen, fail-safe
		if (theApp.glob_prefs->IsCheckDiskspaceEnabled() && error->m_cause == CFileException::diskFull) {
			theApp.downloadqueue->CheckDiskspace();
			AddLogLine(true, GetResString(IDS_ERR_OUTOFSPACE), this->GetFileName());
			if (theApp.emuledlg->IsRunning() && theApp.glob_prefs->GetNotifierPopOnImportantError()){
				CString msg;
				msg.Format(GetResString(IDS_ERR_OUTOFSPACE), this->GetFileName());
				theApp.emuledlg->ShowNotifier(msg, TBN_IMPORTANTEVENT, false);
			}
		// SLUGFILLER: checkDiskspace
		}
		else {
			if (theApp.glob_prefs->IsErrorBeepEnabled())
				Beep(800,200);

			if (error->m_cause == CFileException::diskFull) 
			{
				AddLogLine(true, GetResString(IDS_ERR_OUTOFSPACE), this->GetFileName());
				// may be called during shutdown!
				if (theApp.emuledlg->IsRunning() && theApp.glob_prefs->GetNotifierPopOnImportantError()){
					CString msg;
					msg.Format(GetResString(IDS_ERR_OUTOFSPACE), this->GetFileName());
					theApp.emuledlg->ShowNotifier(msg, TBN_IMPORTANTEVENT, false);
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
			datarate = 0;
			m_anStates[DS_DOWNLOADING] = 0;
		}
	
		if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
		UpdateDisplayedInfo();
		error->Delete();
	}
	catch(...)
	{
		AddLogLine(true, GetResString(IDS_ERR_WRITEERROR), GetFileName(), GetResString(IDS_UNKNOWN));
		SetStatus(PS_ERROR);
		paused = true;
		datarate = 0;
		m_anStates[DS_DOWNLOADING] = 0;
		if (theApp.emuledlg->IsRunning()) // may be called during shutdown!
			UpdateDisplayedInfo();
	}
	delete[] changedPart;

}
// Barry - This will invert the gap list, up to caller to delete gaps when done
// 'Gaps' returned are really the filled areas, and guaranteed to be in order
void CPartFile::GetFilledList(CTypedPtrList<CPtrList, Gap_Struct*> *filled)
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

void CPartFile::UpdateFileRatingCommentAvail() {
	if (!this) return;

	bool prev=(hasComment || hasRating);
	bool prevbad=hasBadRating;

	hasComment=false;
	int badratings=0;
	int ratings=0;

	POSITION pos1,pos2;
	for (int sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty())
	for (pos1 = srclists[sl].GetHeadPosition();( pos2 = pos1 ) != NULL;){
		srclists[sl].GetNext(pos1);
		CUpDownClient* cur_src = srclists[sl].GetAt(pos2);
		if (cur_src->GetFileComment().GetLength()>0) hasComment=true;
		if (cur_src->GetFileRate()>0) ratings++;
		if (cur_src->GetFileRate()==1) badratings++;
		//if (hasComment && hasRating) break;
	}
	hasBadRating=(badratings> (ratings/3));
	hasRating=(ratings>0);
	if ((prev!=(hasComment || hasRating) ) || (prevbad!=hasBadRating) ) UpdateDisplayedInfo(true);
}

uint16 CPartFile::GetSourceCount() {
	uint16 count=0;
	for (int i=0;i<SOURCESSLOTS;i++) count+=srclists[i].GetCount();
	return count;
}

void CPartFile::UpdateDisplayedInfo(boolean force) {
	if (theApp.emuledlg->IsRunning()){
		DWORD curTick = ::GetTickCount();
		//MORPH START - Added by SiRoB, ZZ Upload System 20030723-0133
		//if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+(uint32)(rand()/(RAND_MAX/1000))) {
		if(force || curTick-m_lastRefreshedDLDisplay > MINWAIT_BEFORE_DLDISPLAY_WINDOWUPDATE+m_random_update_wait) {
		//MORPH END - Added by SiRoB, ZZ Upload System 20030723-0133
			theApp.emuledlg->transferwnd.downloadlistctrl.UpdateItem(this);
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
	if( !IsAutoDownPriority() || IsPaused() || IsStopped() )
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

uint8 CPartFile::GetCategory() {
	if(m_category>theApp.glob_prefs->GetCatCount()-1) m_category=0;
	return m_category;
}


// Ornis: Creating progressive presentation of the partfilestatuses - for webdisplay
CString CPartFile::GetProgressString(uint16 size){
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
		for (POSITION pos = gaplist.GetHeadPosition();pos !=  0;gaplist.GetNext(pos)){
			Gap_Struct* cur_gap = gaplist.GetAt(pos);
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
					if (m_SrcpartFrequency.GetCount() >= (INT_PTR)i && m_SrcpartFrequency[i])  // frequency?
						//color = crWaiting;
						color = m_SrcpartFrequency[i] <  10 ? crWaiting[m_SrcpartFrequency[i]/2]:crWaiting[5];
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
		for (POSITION pos = requestedblocks_list.GetHeadPosition();pos !=  0;requestedblocks_list.GetNext(pos))
		{
			Requested_Block_Struct* block =  requestedblocks_list.GetAt(pos);
			CharFillRange(&my_ChunkBar, (uint32)((block->StartOffset + block->transferred)*unit), (uint32)(block->EndOffset*unit),  crPending);
		}

		return my_ChunkBar;
}

void CPartFile::CharFillRange(CString* buffer,uint32 start, uint32 end, char color) {
	for (uint32 i=start;i<=end;i++)
		buffer->SetAt(i,color);
}

void CPartFile::SetCategory(uint8 cat){
	m_category=cat;
	
	// set new prio
	if (IsPartFile()) switch (theApp.glob_prefs->GetCategory(GetCategory())->prio) {
		case 0 : break;
		case 1 : SetAutoDownPriority(false); SetDownPriority(PR_LOW);break;
		case 2 : SetAutoDownPriority(false); SetDownPriority(PR_NORMAL);break;
		case 3 : SetAutoDownPriority(false); SetDownPriority(PR_HIGH);break;
		case 4 : SetAutoDownPriority(true); SetDownPriority(PR_HIGH); break;
	}

	if (IsPartFile()) SavePartFile();
}

void CPartFile::SetStatus(uint8 in) {
	status = in;
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

		uint8 curCatIndex = theApp.emuledlg->transferwnd.downloadlistctrl.curTab;

		theApp.emuledlg->transferwnd.downloadlistctrl.ChangeCategory(curCatIndex);

		if (theApp.glob_prefs->ShowCatTabInfos())
			theApp.emuledlg->transferwnd.UpdateCatTabTitles();
	}
}

ULONGLONG GetDiskFileSize(LPCTSTR pszFilePath)
{
	static BOOL _bInitialized = FALSE;
	static DWORD (WINAPI *_pfnGetCompressedFileSize)(LPCSTR, LPDWORD) = NULL;

	if (!_bInitialized){
		_bInitialized = TRUE;
		(FARPROC&)_pfnGetCompressedFileSize = GetProcAddress(GetModuleHandle("kernel32.dll"), "GetCompressedFileSizeA");
	}

	// If the file is not compressed nor sparse, 'GetCompressedFileSize' returns the 'normal' file size.
	if (_pfnGetCompressedFileSize)
	{
		ULONGLONG ullCompFileSize;
		LPDWORD pdwCompFileSize = (LPDWORD)&ullCompFileSize;
		pdwCompFileSize[0] = (*_pfnGetCompressedFileSize)(pszFilePath, &pdwCompFileSize[1]);
		if (pdwCompFileSize[0] != INVALID_FILE_SIZE || GetLastError() == NO_ERROR)
			return ullCompFileSize;
	}

	// If 'GetCompressedFileSize' failed or is not available, use the default function
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(pszFilePath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
		return 0;
	FindClose(hFind);

	return (ULONGLONG)fd.nFileSizeHigh << 32 | (ULONGLONG)fd.nFileSizeLow;
}
uint64 CPartFile::GetRealFileSize()
{
//	if (IsPartFile()){
//		// If we had a file completion error the 'm_hpartfile.m_hFile' may be invalid and 'GetLength()'
//		// will throw an exception. Because this function is used for various functions, the CFileException
//		// is catched here.
//		if (m_hpartfile.m_hFile != INVALID_HANDLE_VALUE){
//			try{
//				return m_hpartfile.GetLength();
//			}
//			catch(CFileException* error){
//				char buffer[MAX_CFEXP_ERRORMSG];
//				error->GetErrorMessage(buffer,MAX_CFEXP_ERRORMSG);
//				TRACE("Exception in GetRealFileSize(\"%s\") - %s", RemoveFileExtension(m_fullname), buffer);
//				error->Delete();
//			}
//		}
//		struct _stati64 st;
//		if (_stati64(RemoveFileExtension(m_fullname), &st) == 0)
//			return st.st_size;
//	}
//	return GetFileSize();
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

uint16  CPartFile::GetSrcStatisticsValue(uint16 value){
	ASSERT ( value < sizeof(m_anStates) );
	return m_anStates[value];
}

uint16	CPartFile::GetTransferingSrcCount(){
	return GetSrcStatisticsValue(DS_DOWNLOADING);
}

// [Maella -Enhanced Chunk Selection- (based on jicxicmic)]
bool CPartFile::GetNextRequestedBlock(CUpDownClient* sender, 
                                      Requested_Block_Struct** newblocks, 
                                      uint16* count){

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
	// and common (>30%). Inside each zone, the criteria have a specific ‘weight’, used 
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
				if (GetSourceCount()>200) modif=5; else if (GetSourceCount()>800) modif=2;
				uint16 limit= modif*GetSourceCount()/ 100;
				if (limit==0) limit=1;

				const uint16 sourceCount = GetSourceCount();
				const uint16 veryRareBound = limit;
				const uint16 rareBound = 2*limit;

    // Cache Preview state (Criterion 2)
    const bool isPreviewEnable = theApp.glob_prefs->GetPreviewPrio() && 
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
							// Remark: this list might be reused up to ‘*count’ times
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
CString CPartFile::GetInfoSummary(CPartFile* partfile) {
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
		lsc.Format( "%s", partfile->lastseencomplete.Format( theApp.glob_prefs->GetDateTimeFormat()));

	float availability = 0;
	if(partfile->GetPartCount() != 0) {
		// Elandal: type to float before division
		// avoids implicit cast warning
		availability = partfile->GetAvailablePartCount() * 100.0 / partfile->GetPartCount();
	}

	CString avail="";
	if (partfile->IsPartFile()) avail.Format(GetResString(IDS_AVAIL),partfile->GetPartCount(),partfile->GetAvailablePartCount(),availability);

	if (partfile->GetCFileDate()!=NULL) lastdwl.Format( "%s",partfile->GetCFileDate().Format( theApp.glob_prefs->GetDateTimeFormat()));
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

void KademliaSearchFile(uint32 searchID, const Kademlia::CUInt128* pcontactID, uint8 type, uint32 ip, uint16 tcp, uint16 udp, uint32 serverip, uint16 serverport)
{
	ip = ntohl(ip);
	if (!IsGoodIP(ip))
	{
		CemuleApp::AddDebugLogLine(false, _T("Ignored source (IP=%s) received from Kademlia"), inet_ntoa(*(in_addr*)&ip));
		return;
	}
	if (theApp.ipfilter->IsFiltered(ip))
	{
		CemuleApp::AddDebugLogLine(false, _T("IPfiltered source IP=%s (%s) received from Kademlia"), inet_ntoa(*(in_addr*)&ip), theApp.ipfilter->GetLastHit());
		return;
	}
	ip = ntohl(ip);
	if( (ip == theApp.kademlia->getIP() || ip == theApp.GetID()) && tcp == theApp.glob_prefs->GetPort())
		return;
//	CString idStr;
//	CString ipStr;
//	pcontactID->toHexString(&idStr);
//	Kademlia::CMiscUtils::ipAddressToString(ip, &ipStr);
	CPartFile* temp = theApp.downloadqueue->GetFileByKadFileSearchID(searchID);
	if( temp ){
		if(!temp->IsStopped() && theApp.glob_prefs->GetMaxSourcePerFileUDP() > temp->GetSourceCount()){
			CUpDownClient* ctemp = new CUpDownClient(temp,tcp,ip,0,0);
			if(!tcp){
				//These are firewalled users and we don't support them yet.
				delete ctemp;
				return;
			}
			else{
				ctemp->SetSourceFrom(1);
				ctemp->SetServerIP(serverip);
				ctemp->SetServerPort(serverport);
				ctemp->SetKadPort(udp);
			}
			theApp.downloadqueue->CheckAndAddSource(temp, ctemp);
		}
	}
}

bool CPartFile::GrabImage(uint8 nFramesToGrab, double dStartTime, bool bReduceColor, uint16 nMaxWidth,void* pSender){
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

void CPartFile::GrabbingFinished(CxImage** imgResults, uint8 nFramesGrabbed, void* pSender){
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


void CPartFile::GetSizeToTransferAndNeededSpace(uint32& pui32SizeToTransfer, uint32& pui32NeededSpace) 
{
	for (POSITION pos = gaplist.GetHeadPosition();pos != 0;gaplist.GetNext(pos)){
		Gap_Struct* cur_gap = gaplist.GetAt(pos);
		pui32SizeToTransfer += cur_gap->end - cur_gap->start;
		if(cur_gap->end == GetFileSize()-1)
			pui32NeededSpace = cur_gap->end - cur_gap->start;
	}
}

//MORPH START - Added by SiRoB, A4AF counter
void CPartFile::CleanA4AFSource(CPartFile* toremove){
	POSITION pos1, pos2;
	CPartFile* cur_file;
	CUpDownClient* cur_src;
	for (POSITION filepos = theApp.downloadqueue->filelist.GetHeadPosition(); filepos != 0; theApp.downloadqueue->filelist.GetNext(filepos)){
		cur_file = theApp.downloadqueue->filelist.GetAt(filepos);
		for (uint32 sl = 0; sl < SOURCESSLOTS; sl++)
			if (!cur_file->srclists[sl].IsEmpty())
				for (pos1 = cur_file->srclists[sl].GetHeadPosition();( pos2 = pos1 ) != NULL;){
					cur_file->srclists[sl].GetNext(pos1);
					cur_src = cur_file->srclists[sl].GetAt(pos2);
					if (!cur_src->m_OtherRequests_list.IsEmpty()){
						if (cur_file!=toremove){
							POSITION myPos = cur_src->m_OtherRequests_list.Find(toremove);
							if (myPos){
								cur_src->m_OtherRequests_list.RemoveAt(myPos);
								toremove->DecreaseSourceCountA4AF();
								theApp.emuledlg->transferwnd.downloadlistctrl.RemoveSource(cur_src,toremove);
							}
						}else{
							POSITION myPos = cur_src->m_OtherRequests_list.Find(cur_file);
							if (myPos){
								cur_src->m_OtherRequests_list.RemoveAt(myPos);
								cur_file->DecreaseSourceCountA4AF();
								theApp.emuledlg->transferwnd.downloadlistctrl.RemoveSource(cur_src,cur_file);
							}
						}
					}
				}
	}
}
//MORPH END   - Added by SiRoB, A4AF counter

//MORPH START - Added by IceCream, eMule Plus rating icons
int CPartFile::GetRating(){  
    if (!hasRating) return 0;  
    int num,tot,fRate;  
    num=tot=0;  
    POSITION pos1,pos2;  
    for (int sl=0;sl<SOURCESSLOTS;sl++) if (!srclists[sl].IsEmpty())  
    for (pos1 = srclists[sl].GetHeadPosition();( pos2 = pos1 ) != NULL;){  
          srclists[sl].GetNext(pos1);  
          fRate =((CUpDownClient*) srclists[sl].GetAt(pos2))->GetFileRate();  
          if (fRate>0)  
          {  
              num++;
      //Cax2 - bugfix: for some £$%#ing reason  fair=4 & good=3, breaking the progression from fake(1) to excellent(5)
      if (fRate==3 || fRate==4) fRate=(fRate==3)?4:3;
              tot+=fRate;  
          }  
    }  
  if (num>0)
  {
  num=(float)tot/num+.5; //Cax2 - get the average of all the ratings
  //Cax2 - bugfix: for some £$%#ing reason good=3 & fair=4, breaking the progression from fake(1) to excellent(5)
  if (num==3 || num==4) num=(num==3)?4:3;
  }
    return num; //Cax2 - if no ratings found, will return 0!
}
//MORPH END   - Added by IceCream, eMule Plus rating icons
