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
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "Packets.h"
#include "kademlia/kademlia/search.h"
#include "kademlia/routing/Timer.h"
#include "kademlia/kademlia/prefs.h"
#define NOMD4MACROS
#include "kademlia/utils/md4.h"
#include "DownloadQueue.h"
#include "UploadQueue.h"
#include "KademliaMain.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "KnownFile.h"
#include "Sockets.h"
#include "SafeFile.h"
#include "Server.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#include "SharedFilesWnd.h"
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


typedef CSimpleArray<CKnownFile*> CSimpleKnownFileArray;


///////////////////////////////////////////////////////////////////////////////
// CPublishKeyword

class CPublishKeyword
{
public:
	CPublishKeyword(const CString& rstrKeyword)
	{
		m_strKeyword = rstrKeyword;
		ASSERT( rstrKeyword.GetLength() >= 3 );
		Kademlia::CMD4::hash((byte*)(LPCSTR)rstrKeyword, rstrKeyword.GetLength(), &m_nKadID);
		SetNextPublishTime(0);
		SetPublishedCount(0);
	}

	const Kademlia::CUInt128& GetKadID() const { return m_nKadID; }
	const CString& GetKeyword() const { return m_strKeyword; }
	int GetRefCount() const { return m_aFiles.GetSize(); }
	const CSimpleKnownFileArray& GetReferences() const { return m_aFiles; }

	UINT GetNextPublishTime() const { return m_tNextPublishTime; }
	void SetNextPublishTime(UINT tNextPublishTime) { m_tNextPublishTime = tNextPublishTime; }

	UINT GetPublishedCount() const { return m_uPublishedCount; }
	void SetPublishedCount(UINT uPublishedCount) { m_uPublishedCount = uPublishedCount; }
	void IncPublishedCount() { m_uPublishedCount++; }

	BOOL AddRef(CKnownFile* pFile)
	{
		ASSERT( m_aFiles.Find(pFile) == -1 );
		return m_aFiles.Add(pFile);
	}

	int RemoveRef(CKnownFile* pFile)
	{
		m_aFiles.Remove(pFile);
		return m_aFiles.GetSize();
	}

	void RemoveAllReferences()
	{
		m_aFiles.RemoveAll();
	}

	void RotateReferences(int iRotateSize)
	{
		if (m_aFiles.GetSize() > iRotateSize)
		{
			CKnownFile** ppRotated = (CKnownFile**)malloc(m_aFiles.m_nAllocSize * sizeof(*m_aFiles.GetData()));
			if (ppRotated != NULL)
			{
				memcpy(ppRotated, m_aFiles.GetData() + iRotateSize, (m_aFiles.GetSize() - iRotateSize) * sizeof(*m_aFiles.GetData()));
				memcpy(ppRotated + m_aFiles.GetSize() - iRotateSize, m_aFiles.GetData(), iRotateSize * sizeof(*m_aFiles.GetData()));
				free(m_aFiles.GetData());
				m_aFiles.m_aT = ppRotated;
			}
		}
	}

protected:
	CString m_strKeyword;
	Kademlia::CUInt128 m_nKadID;
	UINT m_tNextPublishTime;
	UINT m_uPublishedCount;
	CSimpleKnownFileArray m_aFiles;
};


///////////////////////////////////////////////////////////////////////////////
// CPublishKeywordList

class CPublishKeywordList
{
public:
	CPublishKeywordList();
	~CPublishKeywordList();

	void AddKeywords(CKnownFile* pFile);
	void RemoveKeywords(CKnownFile* pFile);
	void RemoveAllKeywords();

	void RemoveAllKeywordReferences();
	void PurgeUnreferencedKeywords();

	int GetCount() const { return m_lstKeywords.GetCount(); }

	CPublishKeyword* GetNextKeyword();
	void ResetNextKeyword();

	UINT GetNextPublishTime() const { return m_tNextPublishKeywordTime; }
	void SetNextPublishTime(UINT tNextPublishKeywordTime) { m_tNextPublishKeywordTime = tNextPublishKeywordTime; }

#ifdef _DEBUG
	void Dump();
#endif

protected:
	// can't use a CMap - too many disadvantages in processing the 'list'
	//CTypedPtrMap<CMapStringToPtr, CString, CPublishKeyword*> m_lstKeywords;
	CTypedPtrList<CPtrList, CPublishKeyword*> m_lstKeywords;
	POSITION m_posNextKeyword;
	UINT m_tNextPublishKeywordTime;

	CPublishKeyword* FindKeyword(const CString& rstrKeyword, POSITION* ppos = NULL) const;
};

CPublishKeywordList::CPublishKeywordList()
{
	ResetNextKeyword();
	SetNextPublishTime(0);
}

CPublishKeywordList::~CPublishKeywordList()
{
	RemoveAllKeywords();
}

CPublishKeyword* CPublishKeywordList::GetNextKeyword()
{
	if (m_posNextKeyword == NULL)
	{
		m_posNextKeyword = m_lstKeywords.GetHeadPosition();
		if (m_posNextKeyword == NULL)
			return NULL;
	}
	return m_lstKeywords.GetNext(m_posNextKeyword);
}

void CPublishKeywordList::ResetNextKeyword()
{
	m_posNextKeyword = m_lstKeywords.GetHeadPosition();
}

CPublishKeyword* CPublishKeywordList::FindKeyword(const CString& rstrKeyword, POSITION* ppos) const
{
	POSITION pos = m_lstKeywords.GetHeadPosition();
	while (pos)
	{
		POSITION posLast = pos;
		CPublishKeyword* pPubKw = m_lstKeywords.GetNext(pos);
		if (pPubKw->GetKeyword() == rstrKeyword)
		{
			if (ppos)
				*ppos = posLast;
			return pPubKw;
		}
	}
	return NULL;
}

void CPublishKeywordList::AddKeywords(CKnownFile* pFile)
{
	const Kademlia::WordList& wordlist = pFile->GetKadKeywords();
	ASSERT( wordlist.size() > 0 );
	Kademlia::WordList::const_iterator it;
	for (it = wordlist.begin(); it != wordlist.end(); it++)
	{
		const CString& strKeyword = *it;
		CPublishKeyword* pPubKw = FindKeyword(strKeyword);
		if (pPubKw == NULL)
		{
			pPubKw = new CPublishKeyword(strKeyword);
			m_lstKeywords.AddTail(pPubKw);
			SetNextPublishTime(0);
		}
		pPubKw->AddRef(pFile);
	}
}

void CPublishKeywordList::RemoveKeywords(CKnownFile* pFile)
{
	const Kademlia::WordList& wordlist = pFile->GetKadKeywords();
	//ASSERT( wordlist.size() > 0 );
	Kademlia::WordList::const_iterator it;
	for (it = wordlist.begin(); it != wordlist.end(); it++)
	{
		const CString& strKeyword = *it;
		POSITION pos;
		CPublishKeyword* pPubKw = FindKeyword(strKeyword, &pos);
		if (pPubKw != NULL)
		{
			if (pPubKw->RemoveRef(pFile) == 0)
			{
				if (pos == m_posNextKeyword)
					(void)m_lstKeywords.GetNext(m_posNextKeyword);
				m_lstKeywords.RemoveAt(pos);
				delete pPubKw;
				SetNextPublishTime(0);
			}
		}
	}
}

void CPublishKeywordList::RemoveAllKeywords()
{
	POSITION pos = m_lstKeywords.GetHeadPosition();
	while (pos)
		delete m_lstKeywords.GetNext(pos);
	m_lstKeywords.RemoveAll();
	ResetNextKeyword();
	SetNextPublishTime(0);
}

void CPublishKeywordList::RemoveAllKeywordReferences()
{
	POSITION pos = m_lstKeywords.GetHeadPosition();
	while (pos)
		m_lstKeywords.GetNext(pos)->RemoveAllReferences();
}

void CPublishKeywordList::PurgeUnreferencedKeywords()
{
	POSITION pos = m_lstKeywords.GetHeadPosition();
	while (pos)
	{
		POSITION posLast = pos;
		CPublishKeyword* pPubKw = m_lstKeywords.GetNext(pos);
		if (pPubKw->GetRefCount() == 0)
		{
			if (posLast == m_posNextKeyword)
				(void)m_lstKeywords.GetNext(m_posNextKeyword);
			m_lstKeywords.RemoveAt(posLast);
			delete pPubKw;
			SetNextPublishTime(0);
		}
	}
}

#ifdef _DEBUG
void CPublishKeywordList::Dump()
{
	int i = 0;
	POSITION pos = m_lstKeywords.GetHeadPosition();
	while (pos)
	{
		CPublishKeyword* pPubKw = m_lstKeywords.GetNext(pos);
		TRACE("%3u: %-10s  ref=%u  %s\n", i, pPubKw->GetKeyword(), pPubKw->GetRefCount(), CastSecondsToHM(pPubKw->GetNextPublishTime()));
		i++;
	}
}
#endif


///////////////////////////////////////////////////////////////////////////////
// CSharedFileList

CSharedFileList::CSharedFileList(CPreferences* in_prefs,CServerConnect* in_server){
	app_prefs = in_prefs;
	server = in_server;
	output = 0;
	m_Files_map.InitHashTable(1031);
	m_keywords = new CPublishKeywordList;
	m_lastPublishED2K = 0;
	m_lastPublishED2KFlag = true;
	m_currFileSrc = 0;
	m_lastPublishKadSrc = 0;
	m_currFileKey = 0;
	m_lastProcessPublishKadKeywordList = 0;
//	FindSharedFiles(); //Removed by SiRoB - SAfe hash
}

CSharedFileList::~CSharedFileList(){
	while (!waitingforhash_list.IsEmpty()){
		UnknownFile_Struct* nextfile = waitingforhash_list.RemoveHead();
		delete nextfile;
	}
	// SLUGFILLER: SafeHash
	while (!currentlyhashing_list.IsEmpty()){
		UnknownFile_Struct* nextfile = currentlyhashing_list.RemoveHead();
		delete nextfile;
	}
	// SLUGFILLER: SafeHash
	delete m_keywords;
}

void CSharedFileList::FindSharedFiles(){

	// SLUGFILLER: SafeHash remove - only called after the download queue is created
		CSingleLock sLock1(&list_mut,true); // list thread safe
		m_Files_map.RemoveAll();
		//m_keywords->RemoveAllKeywords();
		sLock1.Unlock();
		ASSERT( theApp.downloadqueue );
		if (theApp.downloadqueue)
			theApp.downloadqueue->AddPartFilesToShare(); // read partfiles
	// SLUGFILLER: SafeHash remove - only called after the download queue is created

	// khaos::kmod+ Fix: Shared files loaded multiple times.
	CStringList l_sAdded;
	CString stemp;
	CString tempDir;
	CString ltempDir;
	
	stemp=app_prefs->GetIncomingDir();
	stemp.MakeLower();
	if (stemp.Right(1)!="\\") stemp+="\\";

	l_sAdded.AddHead( stemp );
	AddFilesFromDirectory(l_sAdded.GetHead());

	if (theApp.glob_prefs->GetCatCount()>1) {
		for (int ix=1;ix<theApp.glob_prefs->GetCatCount();ix++)
		{
			tempDir=CString( theApp.glob_prefs->GetCatPath(ix) );
			ltempDir=tempDir;ltempDir.MakeLower();
			if (ltempDir.Right(1)!="\\") ltempDir+="\\";

			if( l_sAdded.Find( ltempDir ) ==NULL ) {
				l_sAdded.AddHead( ltempDir );
				AddFilesFromDirectory(tempDir);
			}
		}
	}

	for (POSITION pos = app_prefs->shareddir_list.GetHeadPosition();pos != 0;app_prefs->shareddir_list.GetNext(pos))
	{
		tempDir = app_prefs->shareddir_list.GetAt(pos);
		ltempDir= tempDir;
		ltempDir.MakeLower();
		if (ltempDir.Right(1)!="\\") ltempDir+="\\";

		if( l_sAdded.Find( ltempDir ) ==NULL ) {
			l_sAdded.AddHead( ltempDir );
			AddFilesFromDirectory(tempDir);
		}
	}
	// khaos::kmod-
	if (waitingforhash_list.IsEmpty())
		AddLogLine(false,GetResString(IDS_SHAREDFOUND), m_Files_map.GetCount());
	else
		AddLogLine(false,GetResString(IDS_SHAREDFOUNDHASHING), m_Files_map.GetCount(), waitingforhash_list.GetCount());
	
	// Mighty Knife: Report hashing files
	if (!waitingforhash_list.IsEmpty()) {
		if (theApp.glob_prefs->GetReportHashingFiles ()) {
			POSITION p = waitingforhash_list.GetHeadPosition ();
			while (p != NULL) {
				UnknownFile_Struct* f = waitingforhash_list.GetAt (p);
				CString hashfilename;
				hashfilename.Format ("%s%s",f->strDirectory, f->strName);
				theApp.emuledlg->AddLogLine(false, "New file: '%s'", (const char*) hashfilename);
				waitingforhash_list.GetNext (p);
			}
		}
		HashNextFile();   // Why should we call this procedure if there's no file to hash ?
						  // so i moved the call into this if clause. This also removes
					      // an unnecessary message "All files hashed", which is added to
						  // the log there.
	}
	// HashNextFile();
	// [end] Mighty Knife

}

void CSharedFileList::AddFilesFromDirectory(const CString& rstrDirectory){
	CFileFind ff;
	
	CString searchpath;
	searchpath.Format("%s\\*",rstrDirectory);
	bool end = !ff.FindFile(searchpath,0);
	if (end)
		return;

	while (!end){
		end = !ff.FindNextFile();
		if (ff.IsDirectory() || ff.IsDots() || ff.IsSystem() || ff.IsTemporary() || ff.GetLength()==0 || ff.GetLength()>=4294967295 )
			continue;

		// ignore real(!) LNK files
		TCHAR szExt[_MAX_EXT];
		_tsplitpath(ff.GetFileName(), NULL, NULL, NULL, szExt);
		if (_tcsicmp(szExt, _T(".lnk")) == 0){
			SHFILEINFO info;
			if (SHGetFileInfo(ff.GetFilePath(), 0, &info, sizeof(info), SHGFI_ATTRIBUTES) && (info.dwAttributes & SFGAO_LINK)){
				CComPtr<IShellLink> pShellLink;
				if (SUCCEEDED(pShellLink.CoCreateInstance(CLSID_ShellLink))){
					CComQIPtr<IPersistFile> pPersistFile = pShellLink;
					if (pPersistFile){
						USES_CONVERSION;
						if (SUCCEEDED(pPersistFile->Load(T2COLE(ff.GetFilePath()), STGM_READ))){
							TCHAR szResolvedPath[MAX_PATH];
							if (pShellLink->GetPath(szResolvedPath, ARRSIZE(szResolvedPath), NULL, 0) == NOERROR){
								TRACE("Did not share file %s\n",ff.GetFilePath());
								continue;
							}
						}
					}
				}
			}
		}

		CTime lwtime;
		if (!ff.GetLastWriteTime(lwtime))
			AddDebugLogLine(false, "Failed to get file date of %s - %s", ff.GetFilePath(), GetErrorMessage(GetLastError()));
		uint32 fdate = safe_mktime(lwtime.GetLocalTm());
		if (fdate == -1)
			AddDebugLogLine(false, "Failed to convert file date of %s", ff.GetFilePath());

		// Mighty Knife: try to correct the daylight saving bug.
		// Very special. Never activate this in a release version !!!
//		#ifdef MIGHTY_SUMMERTIME

		// HINT TO THE MAIN DEVELOPERS FOR CLEANUP
		// ---------------------------------------
		// Please change the data type of CKnownFile::date from "uint32" to "time_t"
		// (and with that also the return value of "CKnownFile::GetFileDate").
		// Otherwise such if-clauses like the above "if (date == -1)" would make no sense !
		// Since the data types are almost identical (apart from the sign) this should
		// make neighter a big deal, nor a any difference.

		if (theApp.glob_prefs->GetDaylightSavingPatch ()) {
			if (fdate != -1) {
				time_t fdate2 = fdate;
				CorrectLocalFileTime (ff.GetFilePath(),fdate2);
				fdate = fdate2;
			}
		}
//		#endif
		// [end] Mighty Knife

		CKnownFile* toadd = theApp.knownfiles->FindKnownFile(ff.GetFileName().GetBuffer(),fdate,(uint32)ff.GetLength());
		if (toadd){
			toadd->SetPath(rstrDirectory);
			toadd->SetFilePath(ff.GetFilePath());
			//MORPH - Removed by SiRoB: Safe Hash
			SafeAddKFile(toadd, true);	// SLUGFILLER: mergeKnown - no unmanagad adds
		}
		else{
			//not in knownfilelist - start adding thread to hash file if the hashing of this file isnt already waiting
			// SLUGFILLER: SafeHash - don't double hash, MY way
			if (!IsHashing(rstrDirectory, ff.GetFileName()) && !theApp.downloadqueue->IsTempFile(rstrDirectory, ff.GetFileName()) && !theApp.glob_prefs->IsConfigFile(rstrDirectory, ff.GetFileName())){
			UnknownFile_Struct* tohash = new UnknownFile_Struct;
				tohash->strDirectory = rstrDirectory;
				tohash->strName = ff.GetFileName();
				waitingforhash_list.AddTail(tohash);
				}
			else
				TRACE("Did not share file %s\n",ff.GetFilePath());
			// SLUGFILLER: SafeHash
			}
	}
	ff.Close();
}

void CSharedFileList::SafeAddKFile(CKnownFile* toadd, bool bOnlyAdd){
	RemoveFromHashing(toadd);	// SLUGFILLER: SafeHash - hashed ok, remove from list, in case it was on the list
	AddFile(toadd);
	toadd->UpdateClientUploadList();		// #zegzav:updcliuplst
	if (bOnlyAdd)
		return;
	if (output)
		output->ShowFile(toadd);
	m_lastPublishED2KFlag = true;
}

void CSharedFileList::AddFile(CKnownFile* pFile)
{
	CSingleLock sLock(&list_mut, true);
	// SLUGFILLER: mergeKnown - check for duplicates
	CKnownFile* other = GetFileByID(pFile->GetFileHash());
	if (other && other != pFile){
		if (other->IsPartFile()){
			if (!pFile->IsPartFile()){	// fail-safe, two part files shouldn't have the same hash
				other->statistic.Merge(&pFile->statistic);
				if (output)
					output->UpdateFile(other);
				theApp.knownfiles->RemoveFile(pFile);
				delete pFile;
				sLock.Unlock();
				return;
			}
		}
		else {
			pFile->statistic.Merge(&other->statistic);
			RemoveFile(other);
			theApp.knownfiles->RemoveFile(other);
			delete other;
		}
	}
	theApp.knownfiles->FilterDuplicateKnownFiles(pFile);
	// SLUGFILLER: mergeKnown
	CCKey key(pFile->GetFileHash());
	CKnownFile* pFileInMap;
	if (m_Files_map.Lookup(key, pFileInMap)){
		TRACE("%s: File already in shared file list: %s \"%s\" \"%s\"\n", __FUNCTION__, md4str(pFileInMap->GetFileHash()), pFileInMap->GetFileName(), pFileInMap->GetFilePath());
		TRACE("%s: File to add:                      %s \"%s\" \"%s\"\n", __FUNCTION__, md4str(pFile->GetFileHash()), pFile->GetFileName(), pFile->GetFilePath());
	}
	m_Files_map.SetAt(key, pFile);
	m_keywords->AddKeywords(pFile);
	sLock.Unlock();
}

void CSharedFileList::RemoveFile(CKnownFile* pFile)
{
	if (output)	// SLUGFILLER: mergeKnown - prevent crash in case of no output
		output->RemoveFile(pFile);
	m_Files_map.RemoveKey(CCKey(pFile->GetFileHash()));
	m_keywords->RemoveKeywords(pFile);
	pFile->statistic.SetLastUsed(time(NULL)); //EastShare - Added by TAHO, .met file control
}

void CSharedFileList::Reload(){
	// SLUGFILLER: SafeHash - don't allow to be called until after the control is loaded
	if (!output)
		return;
	m_keywords->RemoveAllKeywordReferences();	
	FindSharedFiles();
	m_keywords->PurgeUnreferencedKeywords();
	output->ShowFileList(this);
	// SLUGFILLER: SafeHash
}

void CSharedFileList::SetOutputCtrl(CSharedFilesCtrl* in_ctrl)
{
	output = in_ctrl;
	output->ShowFileList(this);
	Reload();		// SLUGFILLER: SafeHash - load shared files after everything
}

uint8 GetRealPrio(uint8 in)
{
	switch(in) {
		case 4 : return 0;
		case 0 : return 1;
		case 1 : return 2;
		case 2 : return 3;
		case 3 : return 4;
	}
	return 0;
}

void CSharedFileList::SendListToServer(){
	if (m_Files_map.IsEmpty() || !server->IsConnected())
	{
		return;
	}
	CSafeMemFile files(1024);
	CCKey bufKey;
	CKnownFile* cur_file,cur_file2;
	POSITION pos,pos2;
	CTypedPtrList<CPtrList, CKnownFile*> sortedList;
	bool added=false;
	for(pos=m_Files_map.GetStartPosition(); pos!=0;)
	{
		m_Files_map.GetNextAssoc(pos, bufKey, cur_file);
		added=false;
		//insertsort into sortedList
		if(!cur_file->GetPublishedED2K())
		{
			for (pos2 = sortedList.GetHeadPosition();pos2 != 0 && !added;sortedList.GetNext(pos2))
			{
				if (GetRealPrio(sortedList.GetAt(pos2)->GetUpPriority()) <= GetRealPrio(cur_file->GetUpPriority()) )
				{
					sortedList.InsertBefore(pos2,cur_file);
					added=true;
				}
			}
			if (!added)
			{
				sortedList.AddTail(cur_file);
			}
		}
	}

	CServer* pCurServer = server->GetCurrentServer();
	// add to packet
	uint32 limit = pCurServer ? pCurServer->GetSoftFiles() : 0;
	if( limit == 0 || limit > 200 )
	{
		limit = 200;
	}
	if( (uint32)sortedList.GetCount() < limit )
	{
		limit = sortedList.GetCount();
		if (limit == 0)
		{
			m_lastPublishED2KFlag = false;
			return;
		}
	}
	files.Write(&limit,4);
	uint32 count=0;
	for (pos = sortedList.GetHeadPosition();pos != 0 && count<limit; )
	{
		count++;
		CKnownFile* file = sortedList.GetNext(pos);
		CreateOfferedFilePacket(file, &files);
		file->SetPublishedED2K(true);
	}
	sortedList.RemoveAll();
	Packet* packet = new Packet(&files);
	packet->opcode = OP_OFFERFILES;
	// compress packet
	//   - this kind of data is highly compressable (N * (1 MD4 and at least 3 string meta data tags and 1 integer meta data tag))
	//   - the min. amount of data needed for one published file is ~100 bytes
	//   - this function is called once when connecting to a server and when a file becomes shareable - so, it's called rarely.
	//   - if the compressed size is still >= the original size, we send the uncompressed packet
	// therefor we always try to compress the packet
	if (pCurServer && pCurServer->GetTCPFlags() & SRV_TCPFLG_COMPRESSION){
		UINT uUncomprSize = packet->size;
		packet->PackPacket();
		if (theApp.glob_prefs->GetDebugServerTCPLevel() > 0)
			Debug(">>> Sending OP__OfferFiles(compressed); uncompr size=%u  compr size=%u  files=%u\n", uUncomprSize, packet->size, limit);
	}
	else{
		if (theApp.glob_prefs->GetDebugServerTCPLevel() > 0)
			Debug(">>> Sending OP__OfferFiles; size=%u  files=%u\n", packet->size, limit);
	}
	theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
	AddDebugLogLine(false,"Server, Sendlist: Packet size:%u", packet->size);
	server->SendPacket(packet,true);
}

CKnownFile* CSharedFileList::GetFileByIndex(int index){
	int count=0;
	CKnownFile* cur_file;
	CCKey bufKey;

	for (POSITION pos = m_Files_map.GetStartPosition();pos != 0;){
		m_Files_map.GetNextAssoc(pos,bufKey,cur_file);
		if (index==count)
			return cur_file;
		count++;
	}
	return 0;
}

void CSharedFileList::ClearED2KPublishInfo(){
	CKnownFile* cur_file;
	CCKey bufKey;
	for (POSITION pos = m_Files_map.GetStartPosition();pos != 0;){
		m_Files_map.GetNextAssoc(pos,bufKey,cur_file);
		cur_file->SetPublishedED2K(false);
	}
}

void CSharedFileList::CreateOfferedFilePacket(CKnownFile* cur_file,CMemFile* files, bool bForServer, bool bSendED2KTags)
{
	// NOTE: This function is used for creating the offered file packet for Servers _and_ for Clients..
	files->Write(cur_file->GetFileHash(),16);

	// This function is used for offering files to the local server and for sending
	// shared files to some other client. In each case we send our IP+Port only, if
	// we have a HighID.
	uint32 nClientID;
	uint16 nClientPort;
	if (!theApp.IsConnected() || theApp.IsFirewalled()){
		nClientID = 0;
		nClientPort = 0;
	}
	else{
		nClientID = theApp.GetID();
		nClientPort = theApp.glob_prefs->GetPort();
	}
	files->Write(&nClientID,4);
	files->Write(&nClientPort,2);

	CSimpleArray<CTag*> tags;

	tags.Add(new CTag(FT_FILENAME,cur_file->GetFileName()));
	tags.Add(new CTag(FT_FILESIZE,cur_file->GetFileSize()));

	CString strED2KFileType(GetED2KFileTypeSearchTerm(GetED2KFileTypeID(cur_file->GetFileName())));
	if (!strED2KFileType.IsEmpty())
		tags.Add(new CTag(FT_FILETYPE,strED2KFileType));

	CString strExt;
	int iExt = cur_file->GetFileName().ReverseFind(_T('.'));
	if (iExt != -1){
		strExt = cur_file->GetFileName().Mid(iExt);
		if (!strExt.IsEmpty()){
			strExt = strExt.Mid(1);
			if (!strExt.IsEmpty()){
				strExt.MakeLower();
				tags.Add(new CTag(FT_FILEFORMAT,strExt)); // file extension without a "."
			}
		}
	}

	// only send verified meta data to servers/clients
	if (cur_file->GetMetaDataVer() > 0)
	{
	static const struct
	{
		bool	bSendToServer;
		uint8	nName;
		uint8	nED2KType;
		LPCSTR	pszED2KName;
	} _aMetaTags[] = 
	{
		// Artist, Album and Title are disabled because they should be already part of the filename
		// and would therefore be redundant information sent to the servers.. and the servers count the
		// amount of sent data!
		{ false, FT_MEDIA_ARTIST,	2, FT_ED2K_MEDIA_ARTIST },
		{ false, FT_MEDIA_ALBUM,	2, FT_ED2K_MEDIA_ALBUM },
		{ false, FT_MEDIA_TITLE,	2, FT_ED2K_MEDIA_TITLE },
		{ true,  FT_MEDIA_LENGTH,	2, FT_ED2K_MEDIA_LENGTH },
		{ true,  FT_MEDIA_BITRATE,	3, FT_ED2K_MEDIA_BITRATE },
		{ true,  FT_MEDIA_CODEC,	2, FT_ED2K_MEDIA_CODEC }
	};
	for (int i = 0; i < ARRSIZE(_aMetaTags); i++){
		if (bForServer && !_aMetaTags[i].bSendToServer)
			continue;
		CTag* pTag = cur_file->GetTag(_aMetaTags[i].nName);
		if (pTag != NULL)
		{
			// skip string tags with empty string values
			if (pTag->tag.type == 2 && (pTag->tag.stringvalue == NULL || pTag->tag.stringvalue[0] == '\0'))
				continue;
			
			// skip integer tags with '0' values
			if (pTag->tag.type == 3 && pTag->tag.intvalue == 0)
				continue;
			
			if (bSendED2KTags)
			{
				if (_aMetaTags[i].nED2KType == 2 && pTag->tag.type == 2)
					tags.Add(new CTag(_aMetaTags[i].pszED2KName, pTag->tag.stringvalue));
				else if (_aMetaTags[i].nED2KType == 3 && pTag->tag.type == 3)
					tags.Add(new CTag(_aMetaTags[i].pszED2KName, pTag->tag.intvalue));
				else if (_aMetaTags[i].nName == FT_MEDIA_LENGTH && pTag->tag.type == 3){
					ASSERT( _aMetaTags[i].nED2KType == 2 );
					CStringA strValue;
					SecToTimeLength(pTag->tag.intvalue, strValue);
					tags.Add(new CTag(_aMetaTags[i].pszED2KName, strValue));
				}
				else
					ASSERT(0);
			}
			else{
				tags.Add(new CTag(pTag->tag));
			}
		}
	}
	}

	uint32 uTagCount = tags.GetSize();
	files->Write(&uTagCount, 4);
	for (int i = 0; i < tags.GetSize(); i++){
		tags[i]->WriteTagToFile(files);
		delete tags[i];
	}
}

// -khaos--+++> New param:  pbytesLargest, pointer to uint64.
//				Various other changes to accomodate our new statistic...
//				Point of this is to find the largest file currently shared.
uint64 CSharedFileList::GetDatasize(uint64 &pbytesLargest) {
	pbytesLargest=0;
	// <-----khaos-
	uint64 fsize;
	fsize=0;

	CCKey bufKey;
	CKnownFile* cur_file;
	for (POSITION pos = m_Files_map.GetStartPosition();pos != 0;){
		m_Files_map.GetNextAssoc(pos,bufKey,cur_file);
		fsize+=cur_file->GetFileSize();
		// -khaos--+++> If this file is bigger than all the others...well duh.
		if (cur_file->GetFileSize() > pbytesLargest) pbytesLargest = cur_file->GetFileSize();
		// <-----khaos-
	}
	return fsize;
}

CKnownFile*	CSharedFileList::GetFileByID(const uchar* filehash){
	if (filehash){
		CKnownFile* result;
		CCKey tkey(filehash);
		if (m_Files_map.Lookup(tkey,result))
			return result;
	}
	return NULL;
}

void CSharedFileList::HashNextFile(){
	// SLUGFILLER: SafeHash
	if (!theApp.emuledlg || !::IsWindow(theApp.emuledlg->m_hWnd))	// wait for the dialog to open
		return;
	if (theApp.emuledlg && theApp.emuledlg->IsRunning())
		theApp.emuledlg->sharedfileswnd->sharedfilesctrl.ShowFilesCount();
	if (!currentlyhashing_list.IsEmpty())	// one hash at a time
		return;
	// SLUGFILLER: SafeHash

	// Mighty Knife: Report hashing files
	if (waitingforhash_list.IsEmpty()) {
		if (theApp.glob_prefs->GetReportHashingFiles ()) {
			theApp.emuledlg->AddLogLine(false, "Hashing of new files completed.");
		}
	}
	// [end] Mighty Knife

	if (waitingforhash_list.IsEmpty())
		return;
	UnknownFile_Struct* nextfile = waitingforhash_list.RemoveHead();
	currentlyhashing_list.AddTail(nextfile);	// SLUGFILLER: SafeHash - keep track
	CAddFileThread* addfilethread = (CAddFileThread*) AfxBeginThread(RUNTIME_CLASS(CAddFileThread), THREAD_PRIORITY_BELOW_NORMAL,0, CREATE_SUSPENDED);
	addfilethread->SetValues(this,nextfile->strDirectory,nextfile->strName);
	addfilethread->ResumeThread();
	// SLUGFILLER: SafeHash remove - nextfile deleting handled elsewhere
	//delete nextfile;
}

// SLUGFILLER: SafeHash
bool CSharedFileList::IsHashing(const CString& rstrDirectory, const CString& rstrName){
	for (POSITION pos = waitingforhash_list.GetHeadPosition(); pos != 0; ){
		const UnknownFile_Struct* pFile = waitingforhash_list.GetNext(pos);
		if (!pFile->strName.CompareNoCase(rstrName) && !CompareDirectories(pFile->strDirectory, rstrDirectory))
			return true;
	}
	for (POSITION pos = currentlyhashing_list.GetHeadPosition(); pos != 0; ){
		const UnknownFile_Struct* pFile = currentlyhashing_list.GetNext(pos);
		if (!pFile->strName.CompareNoCase(rstrName) && !CompareDirectories(pFile->strDirectory, rstrDirectory))
			return true;
	}
	return false;
}

void CSharedFileList::RemoveFromHashing(CKnownFile* hashed){
	for (POSITION pos = currentlyhashing_list.GetHeadPosition(); pos != 0; ){
		POSITION posLast = pos;
		const UnknownFile_Struct* pFile = currentlyhashing_list.GetNext(pos);
		if (!pFile->strName.CompareNoCase(hashed->GetFileName()) && !CompareDirectories(pFile->strDirectory, hashed->GetPath())){
			currentlyhashing_list.RemoveAt(posLast);
			delete pFile;
			HashNextFile();			// start next hash if possible, but only if a previous hash finished
			return;
		}
	}
}

void CSharedFileList::HashFailed(UnknownFile_Struct* hashed){
	for (POSITION pos = currentlyhashing_list.GetHeadPosition(); pos != 0; ){
		POSITION posLast = pos;
		const UnknownFile_Struct* pFile = currentlyhashing_list.GetNext(pos);
		if (!pFile->strName.CompareNoCase(hashed->strName) && !CompareDirectories(pFile->strDirectory, hashed->strDirectory)){
			currentlyhashing_list.RemoveAt(posLast);
			delete pFile;
			HashNextFile();			// start next hash if possible, but only if a previous hash finished
			break;
		}
	}
	ASSERT(0);
	delete hashed;
}
// SLUGFILLER: SafeHash

IMPLEMENT_DYNCREATE(CAddFileThread, CWinThread)
CAddFileThread::CAddFileThread(){
	m_pOwner = 0;
}
void CAddFileThread::SetValues(CSharedFileList* pOwner, LPCTSTR in_directory, LPCTSTR in_filename, CPartFile* in_partfile_Owner){
	 m_pOwner = pOwner;
	 strDirectory = in_directory;
	 strFilename = in_filename;
	 partfile_Owner = in_partfile_Owner;
}

int CAddFileThread::Run(){
	DbgSetThreadName("Hashing %s", strFilename);
	if (!(m_pOwner || partfile_Owner) || strFilename.IsEmpty() || !theApp.emuledlg->IsRunning() )
		AfxEndThread(0,true);
	
	CoInitialize(NULL);

	// SLUGFILLER: SafeHash remove - locking code removed, unnececery
	
	CKnownFile* newrecord = new CKnownFile();
	if (newrecord->CreateFromFile(strDirectory,strFilename) && theApp.emuledlg && theApp.emuledlg->IsRunning()){	// SLUGFILLER: SafeHash - in case of shutdown while still hashing
		VERIFY( PostMessage(theApp.emuledlg->m_hWnd,TM_FINISHEDHASHING,(m_pOwner ? 0:(WPARAM)partfile_Owner),(LPARAM)newrecord) );
	}
	else{
		// SLUGFILLER: SafeHash - inform main program of hash failure
		if (m_pOwner && theApp.emuledlg && theApp.emuledlg->IsRunning()){
			UnknownFile_Struct* hashed = new UnknownFile_Struct;
			hashed->strDirectory = strDirectory;
			hashed->strName = strFilename;
			VERIFY( PostMessage(theApp.emuledlg->m_hWnd,TM_HASHFAILED,0,(LPARAM)hashed) );
		}
		// SLUGFILLER: SafeHash
		delete newrecord;
	}

	// SLUGFILLER: SafeHash remove - locking code removed, unnececery
	CoUninitialize();

	AfxEndThread(0,true);
	return 0;
}

void CSharedFileList::UpdateFile(CKnownFile* toupdate)
{
	output->UpdateFile(toupdate);
}

void CSharedFileList::Process()
{
	if( !m_lastPublishED2KFlag || ( ::GetTickCount() - m_lastPublishED2K < ED2KREPUBLISHTIME ) )
	{
		return;
	}
	this->SendListToServer();
	m_lastPublishED2K = ::GetTickCount();
}

void CSharedFileList::Publish()
{
	UINT tNow = time(NULL);

	if(Kademlia::CTimer::getThreadID() && theApp.kademlia->isConnected() && theApp.IsConnected() && GetCount())
	{ //Once we can handle lowID users in Kad, we need to publish firewalled sources.
		if( theApp.kademlia->getStatus()->m_totalStoreKey < KADEMLIATOTALSTOREKEY && theApp.kademlia->getStatus()->m_keywordPublish)
		{
			if( (!m_lastProcessPublishKadKeywordList || (::GetTickCount() - m_lastProcessPublishKadKeywordList) > KADEMLIAPUBLISHTIME) )
			{
				CSingleLock sLock(&list_mut,true);
				if (tNow >= m_keywords->GetNextPublishTime())
				{
					// faile safe; reset the next publish keyword, the "worse case" would be that we process the
					// keyword list each KADEMLIAPUBLISHTIME seconds
					m_keywords->SetNextPublishTime(0);

					// search the next keyword which has to be (re)-published
					UINT tMinNextPublishTime = (UINT)-1;
					int iCheckedKeywords = 0;
					CPublishKeyword* pPubKw = m_keywords->GetNextKeyword();
					while (pPubKw)
					{
						iCheckedKeywords++;
						UINT tNextKwPublishTime = pPubKw->GetNextPublishTime();

						ASSERT( pPubKw->GetRefCount() != 0 );

						if (tNextKwPublishTime == 0 || tNextKwPublishTime <= tNow)
						{
							DEBUG_ONLY( Debug("pkwlst: %-18s  Refs=%3u  Published=%2u  NextPublish=%s  Publishing\n", pPubKw->GetKeyword(), pPubKw->GetRefCount(), pPubKw->GetPublishedCount(), CastSecondsToHM(tNextKwPublishTime - tNow)) );

							Kademlia::CSearch* pSearch = Kademlia::CSearchManager::prepareFindFile(NULL, pPubKw->GetKadID());
							if (pSearch)
							{
								pSearch->setSearchTypes(Kademlia::CSearch::STOREKEYWORD);

								// add all file IDs which relate to the current keyword to be published
								const CSimpleKnownFileArray& aFiles = pPubKw->GetReferences();
								for (int f = 0; f < aFiles.GetSize(); f++)
								{
									ASSERT_VALID( aFiles[f] );
									Kademlia::CUInt128 kadFileID(aFiles[f]->GetFileHash());
									pSearch->addFileID(kadFileID);
									if( f > 150 )
									{
										pPubKw->RotateReferences(150);
										break;
									}
								}

								pSearch->PreparePacket();

								if (!PostThreadMessage(Kademlia::CTimer::getThreadID(), WM_KADEMLIA_STARTSEARCH, 0, (LPARAM)pSearch))
									Kademlia::CSearchManager::deleteSearch(pSearch);
								else
								{
									pPubKw->SetNextPublishTime(tNow + KADEMLIAREPUBLISHTIME);
									pPubKw->IncPublishedCount();
								}
							}
							break;
						}
						//DEBUG_ONLY( Debug("pkwlst: %-18s  Refs=%3u  Published=%2u  NextPublish=%s\n", pPubKw->GetKeyword(), pPubKw->GetRefCount(), pPubKw->GetPublishedCount(), CastSecondsToHM(tNextKwPublishTime - tNow)) );

						if (tNextKwPublishTime < tMinNextPublishTime)
							tMinNextPublishTime = tNextKwPublishTime;

						if (iCheckedKeywords >= m_keywords->GetCount()){
							DEBUG_ONLY( Debug("pkwlst: EOL\n") );
							// we processed the entire list of keywords to be published, set the next list processing
							// time to the min. time the next keyword has to be published.
							m_keywords->SetNextPublishTime(tMinNextPublishTime);
							break;
						}

						pPubKw = m_keywords->GetNextKeyword();
					}
				}
				else{
					//DEBUG_ONLY( Debug("Next processing of publish keyword list in %s\n", CastSecondsToHM(m_keywords->GetNextPublishTime() - tNow)) );
				}

				// even if we did not publish a keyword, reset the timer so that this list is processed
				// only every KADEMLIAPUBLISHTIME seconds.
				m_lastProcessPublishKadKeywordList = GetTickCount();
			}
		}
		
		if( theApp.kademlia->getStatus()->m_totalStoreSrc < KADEMLIATOTALSTORESRC)
		{
			if( (!m_lastPublishKadSrc || (::GetTickCount() - m_lastPublishKadSrc) > KADEMLIAPUBLISHTIME) )
			{
				if(m_currFileSrc > GetCount())
					m_currFileSrc = 0;
				CSingleLock sLock(&list_mut,true);
				CKnownFile* pCurKnownFile = GetFileByIndex(m_currFileSrc);
				if(pCurKnownFile)
				{
					Kademlia::CUInt128 testID;
					if (pCurKnownFile->PublishSrc(&testID))
					{
						Kademlia::CUInt128 kadFileID;
						kadFileID.setValue(pCurKnownFile->GetFileHash());
						Kademlia::CSearch* pSearch = Kademlia::CSearchManager::prepareFindFile(NULL, kadFileID);
						if (pSearch)
						{
							pSearch->setSearchTypes(Kademlia::CSearch::STOREFILE);
							if (!PostThreadMessage(Kademlia::CTimer::getThreadID(), WM_KADEMLIA_STARTSEARCH, 0, (LPARAM)pSearch))
								Kademlia::CSearchManager::deleteSearch(pSearch);
						}
					}	
				}
				m_currFileSrc++;

				// even if we did not publish a source, reset the timer so that this list is processed
				// only every KADEMLIAPUBLISHTIME seconds.
				m_lastPublishKadSrc = ::GetTickCount();
			}
		}
	}
}

void CSharedFileList::AddKeywords(CKnownFile* pFile)
{
	m_keywords->AddKeywords(pFile);
}

void CSharedFileList::RemoveKeywords(CKnownFile* pFile)
{
	m_keywords->RemoveKeywords(pFile);
}
