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
#include <io.h>
#include "emule.h"
#include "SharedFileList.h"
#include "KnownFileList.h"
#include "Packets.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "kademlia/kademlia/search.h"
#include "kademlia/kademlia/prefs.h"
#include "DownloadQueue.h"
#include "Statistics.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "KnownFile.h"
#include "Sockets.h"
#include "SafeFile.h"
#include "Server.h"
#include "UpDownClient.h"
#include "PartFile.h"
#include "emuledlg.h"
#include "SharedFilesWnd.h"
#include "StringConversion.h"
#include "ClientList.h"
#include "Log.h"

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
	CPublishKeyword(const CStringW& rstrKeyword)
	{
		m_strKeyword = rstrKeyword;
		ASSERT( rstrKeyword.GetLength() >= 3 );
		KadGetKeywordHash(rstrKeyword, &m_nKadID);
		SetNextPublishTime(0);
		SetPublishedCount(0);
	}

	const Kademlia::CUInt128& GetKadID() const { return m_nKadID; }
	const CStringW& GetKeyword() const { return m_strKeyword; }
	int GetRefCount() const { return m_aFiles.GetSize(); }
	const CSimpleKnownFileArray& GetReferences() const { return m_aFiles; }

	UINT GetNextPublishTime() const { return m_tNextPublishTime; }
	void SetNextPublishTime(UINT tNextPublishTime) { m_tNextPublishTime = tNextPublishTime; }

	UINT GetPublishedCount() const { return m_uPublishedCount; }
	void SetPublishedCount(UINT uPublishedCount) { m_uPublishedCount = uPublishedCount; }
	void IncPublishedCount() { m_uPublishedCount++; }

	BOOL AddRef(CKnownFile* pFile)
	{
		if (m_aFiles.Find(pFile) != -1)
		{
			ASSERT(0);
			return FALSE;
		}
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
	CStringW m_strKeyword;
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

	CPublishKeyword* FindKeyword(const CStringW& rstrKeyword, POSITION* ppos = NULL) const;
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

CPublishKeyword* CPublishKeywordList::FindKeyword(const CStringW& rstrKeyword, POSITION* ppos) const
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
		const CStringW& strKeyword = *it;
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
		const CStringW& strKeyword = *it;
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
		TRACE(_T("%3u: %-10ls  ref=%u  %s\n"), i, pPubKw->GetKeyword(), pPubKw->GetRefCount(), CastSecondsToHM(pPubKw->GetNextPublishTime()));
		i++;
	}
}
#endif


///////////////////////////////////////////////////////////////////////////////
// CSharedFileList

CSharedFileList::CSharedFileList(CServerConnect* in_server)
{
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
	FindSharedFiles();
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

void CSharedFileList::FindSharedFiles()
{
	if (!m_Files_map.IsEmpty())
	{
		// Mighty Knife: CRC32-Tag - Public method to lock the filelist 
		// Reason: KnownFile-Objects are deleted only in the following RemoveAll-Command !
		// They must not be deleted when the CRC32-Thread writes the CRC into the object !
		CSingleLock sLockCRC32 (&FileListLockMutex,true);
		// [end] Mighty Knife

		POSITION pos = m_Files_map.GetStartPosition();
		while (pos)
		{
			POSITION posLast = pos;
			CCKey key;
			CKnownFile* cur_file;
			m_Files_map.GetNextAssoc(pos, key, cur_file);
			if (cur_file->IsKindOf(RUNTIME_CLASS(CPartFile)) 
				&& !theApp.downloadqueue->IsPartFile(cur_file) 
					&& !theApp.knownfiles->IsFilePtrInList(cur_file)
					&& _taccess(cur_file->GetFilePath(), 0) == 0)
				continue;
			m_UnsharedFiles_map.SetAt(CSKey(cur_file->GetFileHash()), true);
			m_Files_map.RemoveKey(key);
		}

		// Mighty Knife: CRC32-Tag - Public method to lock the filelist 
		sLockCRC32.Unlock ();
		// [end] Mighty Knife
	
		ASSERT( theApp.downloadqueue );
		if (theApp.downloadqueue)
			theApp.downloadqueue->AddPartFilesToShare(); // read partfiles
	}

	// khaos::kmod+ Fix: Shared files loaded multiple times.
	CStringList l_sAdded;
	CString stemp;
	CString tempDir;
	CString ltempDir;
	
	stemp=thePrefs.GetIncomingDir();
	stemp.MakeLower();
	if (stemp.Right(1)!=_T("\\"))
		stemp+=_T("\\");

	l_sAdded.AddHead( stemp );
	AddFilesFromDirectory(l_sAdded.GetHead());

	if (thePrefs.GetCatCount()>1) {
		for (int ix=1;ix<thePrefs.GetCatCount();ix++)
		{
			tempDir=CString( thePrefs.GetCatPath(ix) );
			ltempDir=tempDir;ltempDir.MakeLower();
			if (ltempDir.Right(1)!=_T("\\"))
				ltempDir+=_T("\\");

			if( l_sAdded.Find( ltempDir ) ==NULL ) {
				l_sAdded.AddHead( ltempDir );
				AddFilesFromDirectory(tempDir);
			}
		}
	}

	for (POSITION pos = thePrefs.shareddir_list.GetHeadPosition();pos != 0;)
	{
		tempDir = thePrefs.shareddir_list.GetNext(pos);
		ltempDir= tempDir;
		ltempDir.MakeLower();
		if (ltempDir.Right(1)!=_T("\\"))
			ltempDir+=_T("\\");

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
		if (thePrefs.GetReportHashingFiles ()) {
			POSITION p = waitingforhash_list.GetHeadPosition ();
			while (p != NULL) {
				UnknownFile_Struct* f = waitingforhash_list.GetAt (p);
				CString hashfilename;
				hashfilename.Format (_T("%s\\%s"),f->strDirectory, f->strName);
				if (hashfilename.Find (_T("\\\\")) >= 0) hashfilename.Format (_T("%s%s"),f->strDirectory, f->strName);
				Log(GetResString(IDS_HASHING_NEWFILE), hashfilename);
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

void CSharedFileList::AddFilesFromDirectory(const CString& rstrDirectory)
{
	CFileFind ff;
	
	CString searchpath;
	searchpath.Format(_T("%s\\*"),rstrDirectory);
	bool end = !ff.FindFile(searchpath,0);
	if (end)
		return;

	while (!end)
	{
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
								TRACE(_T("%hs: Did not share file \"%s\" - not supported file type\n"), __FUNCTION__, ff.GetFilePath());
								continue;
							}
						}
					}
				}
			}
		}

		// ignore real(!) thumbs.db files -- seems that lot of ppl have 'thumbs.db' files without the 'System' file attribute
		if (ff.GetFileName().CompareNoCase(_T("thumbs.db")) == 0)
		{
			// if that's a valid 'Storage' file, we declare it as a "thumbs.db" file.
			USES_CONVERSION;
			CComPtr<IStorage> pStorage;
			if (StgOpenStorage(T2CW(ff.GetFilePath()), NULL, STGM_READ | STGM_SHARE_DENY_WRITE, NULL, 0, &pStorage) == S_OK)
			{
				CComPtr<IEnumSTATSTG> pEnumSTATSTG;
				if (SUCCEEDED(pStorage->EnumElements(0, NULL, 0, &pEnumSTATSTG)))
				{
					STATSTG statstg = {0};
					if (pEnumSTATSTG->Next(1, &statstg, 0) == S_OK)
					{
						CoTaskMemFree(statstg.pwcsName);
						statstg.pwcsName = NULL;
						TRACE(_T("%hs: Did not share file \"%s\" - not supported file type\n"), __FUNCTION__, ff.GetFilePath());
						continue;
					}
				}
			}
		}

		CTime lwtime;
		if (!ff.GetLastWriteTime(lwtime)){
			if (thePrefs.GetVerbose())
				AddDebugLogLine(false, _T("Failed to get file date of %s - %s"), ff.GetFilePath(), GetErrorMessage(GetLastError()));
		}
		uint32 fdate = lwtime.GetTime();
		if (fdate == -1){
			if (thePrefs.GetVerbose())
				AddDebugLogLine(false, _T("Failed to convert file date of %s"), ff.GetFilePath());
		}
		else
			AdjustNTFSDaylightFileTime(fdate, ff.GetFilePath());

		CKnownFile* toadd = theApp.knownfiles->FindKnownFile(ff.GetFileName(), fdate, (uint32)ff.GetLength());
		if (toadd)
		{
			CCKey key(toadd->GetFileHash());
			CKnownFile* pFileInMap;
			if (m_Files_map.Lookup(key, pFileInMap))
			{
				TRACE(_T("%hs: File already in shared file list: %s \"%s\"\n"), __FUNCTION__, md4str(pFileInMap->GetFileHash()), pFileInMap->GetFilePath());
				TRACE(_T("%hs: File to add:                      %s \"%s\"\n"), __FUNCTION__, md4str(toadd->GetFileHash()), ff.GetFilePath());
				if (!pFileInMap->IsKindOf(RUNTIME_CLASS(CPartFile)) || theApp.downloadqueue->IsPartFile(pFileInMap))
					LogWarning(_T("Duplicate shared files: \"%s\" and \"%s\""), pFileInMap->GetFilePath(), ff.GetFilePath());
			}
			else
			{
				toadd->SetPath(rstrDirectory);
				toadd->SetFilePath(ff.GetFilePath());
				AddFile(toadd);
			}
		}
		else
		{
			//not in knownfilelist - start adding thread to hash file if the hashing of this file isnt already waiting
			// SLUGFILLER: SafeHash - don't double hash, MY way
			if (!IsHashing(rstrDirectory, ff.GetFileName()) && !thePrefs.IsTempFile(rstrDirectory, ff.GetFileName())){
			UnknownFile_Struct* tohash = new UnknownFile_Struct;
				tohash->strDirectory = rstrDirectory;
				tohash->strName = ff.GetFileName();
				waitingforhash_list.AddTail(tohash);
				}
			else
				TRACE(_T("%hs: Did not share file \"%s\" - already hashing or temp. file\n"), __FUNCTION__, ff.GetFilePath());
			// SLUGFILLER: SafeHash
			}
	}
	ff.Close();
}

bool CSharedFileList::SafeAddKFile(CKnownFile* toadd, bool bOnlyAdd)
{
	bool bAdded = false;
	RemoveFromHashing(toadd);	// SLUGFILLER: SafeHash - hashed ok, remove from list, in case it was on the list
	bAdded = AddFile(toadd);
	if (bOnlyAdd)
		return bAdded;
	if (output)
		output->ShowFile(toadd);
	m_lastPublishED2KFlag = true;
	return bAdded;
}

void CSharedFileList::RepublishFile(CKnownFile* pFile)
{
	CServer* pCurServer = server->GetCurrentServer();
	if (pCurServer && (pCurServer->GetTCPFlags() & SRV_TCPFLG_COMPRESSION))
	{
		m_lastPublishED2KFlag = true;
		pFile->SetPublishedED2K(false); // FIXME: this creates a wrong 'No' for the ed2k shared info in the listview until the file is shared again.
	}
}

bool CSharedFileList::AddFile(CKnownFile* pFile)
{
	ASSERT( pFile->GetHashCount() == pFile->GetED2KPartHashCount() );
	ASSERT( !pFile->IsKindOf(RUNTIME_CLASS(CPartFile)) || !STATIC_DOWNCAST(CPartFile, pFile)->hashsetneeded );

	CCKey key(pFile->GetFileHash());
	CKnownFile* pFileInMap;
	if (m_Files_map.Lookup(key, pFileInMap))
	{
		TRACE(_T("%hs: File already in shared file list: %s \"%s\" \"%s\"\n"), __FUNCTION__, md4str(pFileInMap->GetFileHash()), pFileInMap->GetFileName(), pFileInMap->GetFilePath());
		TRACE(_T("%hs: File to add:                      %s \"%s\" \"%s\"\n"), __FUNCTION__, md4str(pFile->GetFileHash()), pFile->GetFileName(), pFile->GetFilePath());
		if (!pFileInMap->IsKindOf(RUNTIME_CLASS(CPartFile)) || theApp.downloadqueue->IsPartFile(pFileInMap))
			LogWarning(_T("Duplicate shared files: \"%s\" and \"%s\""), pFileInMap->GetFilePath(), pFile->GetFilePath());
		return false;
	}
	// SLUGFILLER: mergeKnown
	pFile->SetLastSeen();	// okay, we see it
	// SLUGFILLER: mergeKnown
	m_UnsharedFiles_map.RemoveKey(CSKey(pFile->GetFileHash()));	
	m_Files_map.SetAt(key, pFile);
	m_keywords->AddKeywords(pFile);

	return true;
}

void CSharedFileList::FileHashingFinished(CKnownFile* file)
{
	// File hashing finished for a shared file (none partfile)
	//	- reading shared directories at startup and hashing files which were not found in known.met
	//	- reading shared directories during runtime (user hit Reload button, added a shared directory, ...)

	ASSERT( !IsFilePtrInList(file) );
	ASSERT( !theApp.knownfiles->IsFilePtrInList(file) );

	CKnownFile* found_file = GetFileByID(file->GetFileHash());
	if (found_file == NULL)
	{
		SafeAddKFile(file);
		theApp.knownfiles->SafeAddKFile(file);
	}
	else
	{
		TRACE(_T("%hs: File already in shared file list: %s \"%s\"\n"), __FUNCTION__, md4str(found_file->GetFileHash()), found_file->GetFilePath());
		TRACE(_T("%hs: File to add:                      %s \"%s\"\n"), __FUNCTION__, md4str(file->GetFileHash()), file->GetFilePath());
		LogWarning(_T("Duplicate shared files: \"%s\" and \"%s\""), found_file->GetFilePath(), file->GetFilePath());

		RemoveFromHashing(file);
		if (!IsFilePtrInList(file) && !theApp.knownfiles->IsFilePtrInList(file))
			delete file;
		else
			ASSERT(0);
	}
}

void CSharedFileList::RemoveFile(CKnownFile* pFile)
{
	output->RemoveFile(pFile);
	m_UnsharedFiles_map.SetAt(CSKey(pFile->GetFileHash()), true);
	m_Files_map.RemoveKey(CCKey(pFile->GetFileHash()));
	m_keywords->RemoveKeywords(pFile);
}

void CSharedFileList::Reload()
{
	m_keywords->RemoveAllKeywordReferences();	
	FindSharedFiles();
	m_keywords->PurgeUnreferencedKeywords();
	if (output)
		output->ShowFileList(this);
}

void CSharedFileList::SetOutputCtrl(CSharedFilesCtrl* in_ctrl)
{
	output = in_ctrl;
	output->ShowFileList(this);
	HashNextFile();		// SLUGFILLER: SafeHash - if hashing not yet started, start it now
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
	files.WriteUInt32(limit);
	uint32 count=0;
	for (pos = sortedList.GetHeadPosition();pos != 0 && count<limit; )
	{
		count++;
		CKnownFile* file = sortedList.GetNext(pos);
		CreateOfferedFilePacket(file, &files, pCurServer);
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
		if (thePrefs.GetDebugServerTCPLevel() > 0)
			Debug(_T(">>> Sending OP__OfferFiles(compressed); uncompr size=%u  compr size=%u  files=%u\n"), uUncomprSize, packet->size, limit);
	}
	else{
		if (thePrefs.GetDebugServerTCPLevel() > 0)
			Debug(_T(">>> Sending OP__OfferFiles; size=%u  files=%u\n"), packet->size, limit);
	}
	theStats.AddUpDataOverheadServer(packet->size);
	if (thePrefs.GetVerbose())
		AddDebugLogLine(false, _T("Server, Sendlist: Packet size:%u"), packet->size);
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
	m_lastPublishED2KFlag = true;
	for (POSITION pos = m_Files_map.GetStartPosition();pos != 0;){
		m_Files_map.GetNextAssoc(pos,bufKey,cur_file);
		cur_file->SetPublishedED2K(false);
	}
}

void CSharedFileList::CreateOfferedFilePacket(const CKnownFile* cur_file, CSafeMemFile* files, 
											  CServer* pServer, CUpDownClient* pClient)
{
	UINT uEmuleVer = (pClient && pClient->IsEmuleClient()) ? pClient->GetVersion() : 0;

	// NOTE: This function is used for creating the offered file packet for Servers _and_ for Clients..
	files->WriteHash16(cur_file->GetFileHash());

	// *) This function is used for offering files to the local server and for sending
	//    shared files to some other client. In each case we send our IP+Port only, if
	//    we have a HighID.
	// *) Newer eservers also support 2 special IP+port values which are used to hold basic file status info.
	uint32 nClientID = 0;
	uint16 nClientPort = 0;
	if (pServer)
	{
		// we use the 'TCP-compression' server feature flag as indicator for a 'newer' server.
		if (pServer->GetTCPFlags() & SRV_TCPFLG_COMPRESSION)
		{
			if (cur_file->IsPartFile())
			{
				// publishing an incomplete file
				nClientID = 0xFCFCFCFC;
				nClientPort = 0xFCFC;
			}
			else
			{
				// publishing a complete file
				nClientID = 0xFBFBFBFB;
				nClientPort = 0xFBFB;
			}
		}
		else
		{
			// check eD2K ID state
			if (theApp.serverconnect->IsConnected() && !theApp.serverconnect->IsLowID())
			{
				nClientID = theApp.GetID();
				nClientPort = thePrefs.GetPort();
			}
		}
	}
	else
	{
		if (theApp.IsConnected() && !theApp.IsFirewalled())
		{
			nClientID = theApp.GetID();
			nClientPort = thePrefs.GetPort();
		}
	}
	files->WriteUInt32(nClientID);
	files->WriteUInt16(nClientPort);
	//TRACE("Publishing file: Hash=%s  ClientIP=%s  ClientPort=%u\n", md4str(cur_file->GetFileHash()), ipstr(nClientID), nClientPort);

	CSimpleArray<CTag*> tags;

	tags.Add(new CTag(FT_FILENAME, cur_file->GetFileName()));
	tags.Add(new CTag(FT_FILESIZE,cur_file->GetFileSize()));

	// NOTE: Archives and CD-Images are published with file type "Pro"
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
				tags.Add(new CTag(FT_FILEFORMAT, strExt)); // file extension without a "."
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
			{ false, FT_MEDIA_ARTIST,	TAGTYPE_STRING, FT_ED2K_MEDIA_ARTIST },
			{ false, FT_MEDIA_ALBUM,	TAGTYPE_STRING, FT_ED2K_MEDIA_ALBUM },
			{ false, FT_MEDIA_TITLE,	TAGTYPE_STRING, FT_ED2K_MEDIA_TITLE },
			{ true,  FT_MEDIA_LENGTH,	TAGTYPE_STRING, FT_ED2K_MEDIA_LENGTH },
			{ true,  FT_MEDIA_BITRATE,	TAGTYPE_UINT32, FT_ED2K_MEDIA_BITRATE },
			{ true,  FT_MEDIA_CODEC,	TAGTYPE_STRING, FT_ED2K_MEDIA_CODEC }
		};
		for (int i = 0; i < ARRSIZE(_aMetaTags); i++)
		{
			if (pServer!=NULL && !_aMetaTags[i].bSendToServer)
				continue;
			CTag* pTag = cur_file->GetTag(_aMetaTags[i].nName);
			if (pTag != NULL)
			{
				// skip string tags with empty string values
				if (pTag->IsStr() && pTag->GetStr().IsEmpty())
					continue;
				
				// skip integer tags with '0' values
				if (pTag->IsInt() && pTag->GetInt() == 0)
					continue;
				
				if (_aMetaTags[i].nED2KType == TAGTYPE_STRING && pTag->IsStr())
				{
					if (pServer && (pServer->GetTCPFlags() & SRV_TCPFLG_NEWTAGS))
						tags.Add(new CTag(_aMetaTags[i].nName, pTag->GetStr()));
					else
						tags.Add(new CTag(_aMetaTags[i].pszED2KName, pTag->GetStr()));
				}
				else if (_aMetaTags[i].nED2KType == TAGTYPE_UINT32 && pTag->IsInt())
				{
					if (pServer && (pServer->GetTCPFlags() & SRV_TCPFLG_NEWTAGS))
						tags.Add(new CTag(_aMetaTags[i].nName, pTag->GetInt()));
					else
						tags.Add(new CTag(_aMetaTags[i].pszED2KName, pTag->GetInt()));
				}
				else if (_aMetaTags[i].nName == FT_MEDIA_LENGTH && pTag->IsInt())
				{
					ASSERT( _aMetaTags[i].nED2KType == TAGTYPE_STRING );
					// All 'eserver' versions and eMule versions >= 0.42.4 support the media length tag with type 'integer'
					if (   pServer!=NULL && (pServer->GetTCPFlags() & SRV_TCPFLG_COMPRESSION)
						|| uEmuleVer >= MAKE_CLIENT_VERSION(0,42,4))
					{
						if (pServer && (pServer->GetTCPFlags() & SRV_TCPFLG_NEWTAGS))
							tags.Add(new CTag(_aMetaTags[i].nName, pTag->GetInt()));
						else
							tags.Add(new CTag(_aMetaTags[i].pszED2KName, pTag->GetInt()));
					}
					else
					{
						CString strValue;
						SecToTimeLength(pTag->GetInt(), strValue);
						tags.Add(new CTag(_aMetaTags[i].pszED2KName, strValue));
					}
				}
				else
					ASSERT(0);
			}
		}
	}

	EUtf8Str eStrEncode;
#ifdef _UNICODE
	if (pServer != NULL && (pServer->GetTCPFlags() & SRV_TCPFLG_UNICODE)){
		// eserver doesn't properly support searching with ASCII-7 strings in BOM-UTF8 published strings
		//eStrEncode = utf8strOptBOM;
		eStrEncode = utf8strRaw;
	}
	else if (pClient && !pClient->GetUnicodeSupport())
		eStrEncode = utf8strNone;
	else
		eStrEncode = utf8strRaw;
#else
	eStrEncode = utf8strNone;
#endif

	files->WriteUInt32(tags.GetSize());
	for (int i = 0; i < tags.GetSize(); i++)
	{
		const CTag* pTag = tags[i];
		//TRACE("  %s\n", pTag->GetFullInfo());
		if (pServer && (pServer->GetTCPFlags() & SRV_TCPFLG_NEWTAGS) || (uEmuleVer >= MAKE_CLIENT_VERSION(0,42,7)))
			pTag->WriteNewEd2kTag(files, eStrEncode);
		else
			pTag->WriteTagToFile(files, eStrEncode);
		delete pTag;
	}
}

// -khaos--+++> New param:  pbytesLargest, pointer to uint64.
//				Various other changes to accomodate our new statistic...
//				Point of this is to find the largest file currently shared.
uint64 CSharedFileList::GetDatasize(uint64 &pbytesLargest) const
{
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
		if (cur_file->GetFileSize() > pbytesLargest)
			pbytesLargest = cur_file->GetFileSize();
		// <-----khaos-
	}
	return fsize;
}

CKnownFile* CSharedFileList::GetFileByID(const uchar* hash) const
{
	if (hash)
	{
		CKnownFile* found_file;
		CCKey key(hash);
		if (m_Files_map.Lookup(key, found_file))
			return found_file;
	}
	return NULL;
}


bool CSharedFileList::IsFilePtrInList(const CKnownFile* file) const
{
	if (file)
	{
		POSITION pos = m_Files_map.GetStartPosition();
		while (pos)
		{
			CCKey key;
			CKnownFile* cur_file;
			m_Files_map.GetNextAssoc(pos, key, cur_file);
			if (file == cur_file)
				return true;
		}
	}
	return false;
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
		if (thePrefs.GetReportHashingFiles ()) {
			AddLogLine(false, GetResString(IDS_HASHING_COMPLETED2));
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
	// SLUGFILLER: SafeHash - nextfile deleting handled elsewhere
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
	delete hashed;
}
// SLUGFILLER: SafeHash

IMPLEMENT_DYNCREATE(CAddFileThread, CWinThread)

CAddFileThread::CAddFileThread()
{
	m_pOwner = NULL;
	m_partfile = NULL;
}

void CAddFileThread::SetValues(CSharedFileList* pOwner, LPCTSTR directory, LPCTSTR filename, CPartFile* partfile)
{
	 m_pOwner = pOwner;
	 m_strDirectory = directory;
	 m_strFilename = filename;
	 m_partfile = partfile;
}

BOOL CAddFileThread::InitInstance()
{
	InitThreadLocale();
	return TRUE;
}

int CAddFileThread::Run()
{
	DbgSetThreadName("Hashing %s", m_strFilename);
	if ( !(m_pOwner || m_partfile) || m_strFilename.IsEmpty() || !theApp.emuledlg->IsRunning() )
		return 0;
	
	CoInitialize(NULL);

	// locking that hashing thread is needed because we may create a couple of those threads at startup when rehashing
	// potentially corrupted downloading part files. if all those hash threads would run concurrently, the io-system would be
	// under very heavy load and slowly progressing
	CSingleLock sLock1(&theApp.hashing_mut); // only one filehash at a time
	sLock1.Lock();

	CString strFilePath;
	_tmakepath(strFilePath.GetBuffer(MAX_PATH), NULL, m_strDirectory, m_strFilename, NULL);
	strFilePath.ReleaseBuffer();
	if (m_partfile)
		Log(GetResString(IDS_HASHINGFILE) + _T(" \"%s\" \"%s\""), m_partfile->GetFileName(), strFilePath);
	else
		Log(GetResString(IDS_HASHINGFILE) + _T(" \"%s\""), strFilePath);

	CKnownFile* newrecord = new CKnownFile();
	if (newrecord->CreateFromFile(m_strDirectory, m_strFilename, m_partfile) && theApp.emuledlg && theApp.emuledlg->IsRunning()) // SLUGFILLER: SafeHash - in case of shutdown while still hashing
	{
		if (m_partfile && m_partfile->GetFileOp() == PFOP_HASHING)
			m_partfile->SetFileOp(PFOP_NONE);
		VERIFY( PostMessage(theApp.emuledlg->m_hWnd, TM_FINISHEDHASHING, (m_pOwner ? 0: (WPARAM)m_partfile), (LPARAM)newrecord) );
	}
	else
	{
		if (theApp.emuledlg && theApp.emuledlg->IsRunning())
		{
			if (m_partfile && m_partfile->GetFileOp() == PFOP_HASHING)
				m_partfile->SetFileOp(PFOP_NONE);
		}

		// SLUGFILLER: SafeHash - inform main program of hash failure
		if (m_pOwner && theApp.emuledlg && theApp.emuledlg->IsRunning())
		{
			UnknownFile_Struct* hashed = new UnknownFile_Struct;
			hashed->strDirectory = m_strDirectory;
			hashed->strName = m_strFilename;
			VERIFY( PostMessage(theApp.emuledlg->m_hWnd,TM_HASHFAILED,0,(LPARAM)hashed) );
		}
		// SLUGFILLER: SafeHash
		delete newrecord;
	}

	sLock1.Unlock();
	CoUninitialize();

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
	SendListToServer();
	m_lastPublishED2K = ::GetTickCount();
}

void CSharedFileList::Publish()
{
	UINT tNow = time(NULL);

	bool isFirewalled = theApp.IsFirewalled();
	if( Kademlia::CKademlia::isConnected() && ( !isFirewalled || ( isFirewalled && theApp.clientlist->GetBuddyStatus() == 2)) && GetCount() && Kademlia::CKademlia::getPublish())
	{ 
		if( Kademlia::CKademlia::getTotalStoreKey() < KADEMLIATOTALSTOREKEY)
		{
			if( (!m_lastProcessPublishKadKeywordList || (::GetTickCount() - m_lastProcessPublishKadKeywordList) > KADEMLIAPUBLISHTIME) )
			{
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
							DEBUG_ONLY( Debug(_T("pkwlst: %-18ls  Refs=%3u  Published=%2u  NextPublish=%s  Publishing\n"), pPubKw->GetKeyword(), pPubKw->GetRefCount(), pPubKw->GetPublishedCount(), CastSecondsToHM(tNextKwPublishTime - tNow)) );

							Kademlia::CSearch* pSearch = Kademlia::CSearchManager::prepareFindFile(Kademlia::CSearch::STOREKEYWORD, false, pPubKw->GetKadID());
							if (pSearch)
							{
								// add all file IDs which relate to the current keyword to be published
								const CSimpleKnownFileArray& aFiles = pPubKw->GetReferences();
								uint32 count = 0;
								for (int f = 0; f < aFiles.GetSize(); f++)
								{
									ASSERT_VALID( aFiles[f] );
									//Only publish complete files as someone else should have the full file to publish these keywords.
									//As a side effect, this may help reduce people finding incomplete files in the network.
									if( !aFiles[f]->IsPartFile() )
									{
										count++;
									Kademlia::CUInt128 kadFileID(aFiles[f]->GetFileHash());
									pSearch->addFileID(kadFileID);
										if( count > 150 )
										{
											pPubKw->RotateReferences(f);
											break;
										}
									}
								}

								if( count )
								{
								pSearch->PreparePacket();
									pPubKw->SetNextPublishTime(tNow + (KADEMLIAREPUBLISHTIMEK));
									pPubKw->IncPublishedCount();
									Kademlia::CSearchManager::startSearch(pSearch);
								}
								else
								{
									pPubKw->SetNextPublishTime(tNow + (KADEMLIAREPUBLISHTIMEK/2));
									pPubKw->IncPublishedCount();
									delete pSearch;
								}
							}
							break;
						}
						//DEBUG_ONLY( Debug("pkwlst: %-18s  Refs=%3u  Published=%2u  NextPublish=%s\n", pPubKw->GetKeyword(), pPubKw->GetRefCount(), pPubKw->GetPublishedCount(), CastSecondsToHM(tNextKwPublishTime - tNow)) );

						if (tNextKwPublishTime < tMinNextPublishTime)
							tMinNextPublishTime = tNextKwPublishTime;

						if (iCheckedKeywords >= m_keywords->GetCount()){
							DEBUG_ONLY( Debug(_T("pkwlst: EOL\n")) );
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
		
		if( Kademlia::CKademlia::getTotalStoreSrc() < KADEMLIATOTALSTORESRC)
		{
			if( (!m_lastPublishKadSrc || (::GetTickCount() - m_lastPublishKadSrc) > KADEMLIAPUBLISHTIME) )
			{
				if(m_currFileSrc > GetCount())
					m_currFileSrc = 0;
				CKnownFile* pCurKnownFile = GetFileByIndex(m_currFileSrc);
				if(pCurKnownFile)
				{
					//Only publish source if two conditions.
					//1) We are not firewalled..
					//2) We are firewalled, but it's a complete source..
					//
					//HighID users will find incomplete sources passively..
					//If the overhead of lowID is not to high, maybe we can start publishing all lowID sources..
					if (pCurKnownFile->PublishSrc() && (!theApp.IsFirewalled() || (theApp.IsFirewalled() && !pCurKnownFile->IsPartFile())))
					{
						Kademlia::CUInt128 kadFileID;
						kadFileID.setValue(pCurKnownFile->GetFileHash());
						Kademlia::CSearchManager::prepareFindFile(Kademlia::CSearch::STOREFILE, true, kadFileID);
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

void CSharedFileList::DeletePartFileInstances() const
{
	// this is only allowed during shut down
	ASSERT( theApp.m_app_state == APP_STATE_SHUTINGDOWN );
	ASSERT( theApp.knownfiles );

	POSITION pos = m_Files_map.GetStartPosition();
	while (pos)
	{
		CCKey key;
		CKnownFile* cur_file;
		m_Files_map.GetNextAssoc(pos, key, cur_file);
		if (cur_file->IsKindOf(RUNTIME_CLASS(CPartFile)))
		{
			if (!theApp.downloadqueue->IsPartFile(cur_file) && !theApp.knownfiles->IsFilePtrInList(cur_file))
				delete cur_file; // this is only allowed during shut down
		}
	}
}

bool CSharedFileList::IsUnsharedFile(const uchar* auFileHash) const {
	bool bFound;
	if (auFileHash){
		CSKey key(auFileHash);
		if (m_UnsharedFiles_map.Lookup(key, bFound))
			return true;
	}
	return false;
}

//MORPH START - Added by SiRoB, POWERSHARE Limit
void CSharedFileList::UpdatePartsInfo()
{
	if (m_Files_map.IsEmpty())
		return;
	CCKey bufKey;
	CKnownFile* file;
	POSITION pos;
	for(pos=m_Files_map.GetStartPosition(); pos!=0;)
	{
		m_Files_map.GetNextAssoc(pos, bufKey, file);
		if (((file->GetPowerSharedMode()>=0)?file->GetPowerSharedMode():thePrefs.GetPowerShareMode()) == 3)
			file->UpdatePartsInfo();
	}
}
//MORPH END - Added by SiRoB, POWERSHARE Limit