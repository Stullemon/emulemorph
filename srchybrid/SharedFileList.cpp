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
#include "sharedfilelist.h"
#include "knownfilelist.h"
#include "packets.h"
#include "kademlia/kademlia/search.h"
#include "kademlia/routing/Timer.h"
#include <time.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CSharedFileList::CSharedFileList(CPreferences* in_prefs,CServerConnect* in_server,CKnownFileList* in_filelist){
	app_prefs = in_prefs;
	server = in_server;
	filelist = in_filelist;
	output = 0;
	m_Files_map.InitHashTable(1031);
	FindSharedFiles();
	m_lastPublishED2K = 0;
	m_lastPublishED2KFlag = true;
	m_currFile = 0;
	m_lastPublishKad = 0;
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
}

void CSharedFileList::FindSharedFiles(){

	if (!m_Files_map.IsEmpty()){
		CSingleLock sLock1(&list_mut,true); // list thread safe
		m_Files_map.RemoveAll();
		sLock1.Unlock();
		ASSERT( theApp.downloadqueue );
		if (theApp.downloadqueue)
			theApp.downloadqueue->AddPartFilesToShare(); // read partfiles
	}

	// khaos::kmod+ Fix: Shared files loaded multiple times.
	// TODO: Use a different list or array or something other which does not force use finally produce lowercased directory names
	CStringList l_sAdded;
	CString stemp;
	CString tempDir;
	CString ltempDir;
	
	stemp=app_prefs->GetIncomingDir();
	stemp.MakeLower();
	l_sAdded.AddHead( stemp );
	AddFilesFromDirectory(l_sAdded.GetHead());

	if (theApp.glob_prefs->GetCatCount()>1) {
		for (int ix=1;ix<theApp.glob_prefs->GetCatCount();ix++)
		{
			tempDir=CString( theApp.glob_prefs->GetCatPath(ix) );
			ltempDir=tempDir;ltempDir.MakeLower();

			if( l_sAdded.Find( ltempDir ) ==NULL ) {
				l_sAdded.AddHead( ltempDir );
				AddFilesFromDirectory(tempDir);
			}
		}
	}

	for (POSITION pos = app_prefs->shareddir_list.GetHeadPosition();pos != 0;app_prefs->shareddir_list.GetNext(pos))
	{
		tempDir = app_prefs->shareddir_list.GetAt(pos);
		ltempDir= tempDir;ltempDir.MakeLower();

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
	HashNextFile();
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
		uint32 fdate = mktime(lwtime.GetLocalTm());
		if (fdate == -1)
			AddDebugLogLine(false, "Failed to convert file date of %s", ff.GetFilePath());

		CKnownFile* toadd = filelist->FindKnownFile(ff.GetFileName().GetBuffer(),fdate,(uint32)ff.GetLength());
		if (toadd){
			toadd->SetPath(rstrDirectory);
			toadd->SetFilePath(ff.GetFilePath());
			/*CSingleLock sLock(&list_mut,true);
			m_Files_map.SetAt(CCKey(toadd->GetFileHash()),toadd);
			sLock.Unlock();
			toadd->UpdateClientUploadList();		// #zegzav:updcliuplst*/
			SafeAddKFile(toadd, true);	// SLUGFILLER: mergeKnown - no unmanagad adds
		}
		else{
			//not in knownfilelist - start adding thread to hash file if the hashing of this file isnt already waiting
			// SLUGFILLER: SafeHash - don't double hash, MY way
			if (!IsHashing(rstrDirectory, ff.GetFileName()) && !theApp.glob_prefs->IsTempFile(rstrDirectory, ff.GetFileName())){
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
	CSingleLock sLock(&list_mut,true);
	
	// SLUGFILLER: mergeKnown - check for duplicates
	CKnownFile* other = GetFileByID(toadd->GetFileHash());
	if (other && other != toadd){
		if (other->IsPartFile()){
			if (!toadd->IsPartFile()){	// fail-safe, two part files shouldn't have the same hash
				other->statistic.Merge(&toadd->statistic);
				if (!bOnlyAdd && output)
					output->UpdateFile(other);
				filelist->RemoveFile(toadd);
				delete toadd;
				sLock.Unlock();
				return;
			}
		}
		else {
			toadd->statistic.Merge(&other->statistic);
			RemoveFile(other);
			filelist->RemoveFile(other);
			delete other;
		}
	}
	filelist->FilterDuplicateKnownFiles(toadd);
	// SLUGFILLER: mergeKnown
	m_Files_map.SetAt(CCKey(toadd->GetFileHash()),toadd);
	sLock.Unlock();
	toadd->UpdateClientUploadList();		// #zegzav:updcliuplst
	if (bOnlyAdd)
		return;
	if (output)
		output->ShowFile(toadd);
	m_lastPublishED2KFlag = true;
}

// removes first occurrence of 'toremove' in 'list'
void CSharedFileList::RemoveFile(CKnownFile* toremove){
	if (output) output->RemoveFile(toremove);	// SLUGFILLER: mergeKnown - prevent crash in case of no output
	m_Files_map.RemoveKey(CCKey(toremove->GetFileHash()));
}

void CSharedFileList::Reload(){
	this->FindSharedFiles();
	if (output)
		output->ShowFileList(this);
}

void CSharedFileList::SetOutputCtrl(CSharedFilesCtrl* in_ctrl){
	output = in_ctrl;
	output->ShowFileList(this);
	HashNextFile();		// SLUGFILLER: SafeHash - if hashing not yet started, start it now
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
		CreateOfferedFilePacket(sortedList.GetNext(pos),&files);
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
		if (theApp.glob_prefs->GetDebugServerTCP())
			Debug(">>> Sending OP__OfferFiles(compressed); uncompr size=%u  compr size=%u  files=%u\n", uUncomprSize, packet->size, limit);
	}
	else{
		if (theApp.glob_prefs->GetDebugServerTCP())
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

void CSharedFileList::CreateOfferedFilePacket(CKnownFile* cur_file,CMemFile* files, bool bForServer, bool bSendED2KTags){
	cur_file->SetPublishedED2K(true);
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
		nClientID = ntohl(theApp.GetID());
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

	uint32 uTagCount = tags.GetSize();
	files->Write(&uTagCount,4);
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
	theApp.emuledlg->sharedfileswnd.sharedfilesctrl.ShowFilesCount();
	if (!currentlyhashing_list.IsEmpty())	// one hash at a time
		return;
	// SLUGFILLER: SafeHash
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

	// locking that hashing thread is needed because we may create a couple of those threads at startup when rehashing
	// potentially corrupted downloading part files. if all those hash threads would run concurrently, the io-system would be
	// under very heavy load and slowly progressing
	CSingleLock sLock1(&(theApp.hashing_mut)); // only one filehash at a time
	sLock1.Lock();
	
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

	sLock1.Unlock();
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

void CSharedFileList::Publish(){
	if( theApp.kademlia->getStatus()->m_totalStore > KADEMLIATOTALSTORE || !GetCount())
		return;
	if( (!m_lastPublishKad || (::GetTickCount() - m_lastPublishKad) > KADEMLIAPUBLISHTIME) && Kademlia::CTimer::getThreadID() && theApp.kademlia->isConnected() && theApp.IsConnected()){ //Once we can handle lowID users in Kad, we need to publish firewalled sources.
		if(m_currFile > GetCount())
			m_currFile = 0;
		CSingleLock sLock(&list_mut,true);
		CKnownFile* pCurKnownFile = GetFileByIndex(m_currFile);
		if(pCurKnownFile){
			Kademlia::CUInt128 testID;
			int test = pCurKnownFile->Publish(&testID);
			if (test == (-2)){
				return;
			}
			if (test == (-1)){
				m_currFile++;
				return;
			}
			else{
				if( test == 0){
					m_lastPublishKad = ::GetTickCount();
					Kademlia::CUInt128 kadFileID;
					kadFileID.setValue(pCurKnownFile->GetFileHash());
					Kademlia::CSearch* pSearch = Kademlia::CSearchManager::prepareFindFile(NULL, kadFileID);
					if (pSearch){
						pSearch->setSearchTypes(Kademlia::CSearch::STOREFILE);
						if (!PostThreadMessage(Kademlia::CTimer::getThreadID(), WM_KADEMLIA_STARTSEARCH, 0, (LPARAM)pSearch))
							Kademlia::CSearchManager::deleteSearch(pSearch);
					}
					return;
				}
				else{
					m_lastPublishKad = ::GetTickCount();
					Kademlia::CUInt128 kadFileID;
					kadFileID.setValue(pCurKnownFile->GetFileHash());
					Kademlia::CSearch* pSearch = Kademlia::CSearchManager::prepareFindFile(NULL, testID);
					if (pSearch){
						pSearch->setSearchTypes(Kademlia::CSearch::STOREKEYWORD);
						pSearch->m_keywordPublish = kadFileID;
						pSearch->setKeywordCount(test);
						if (!PostThreadMessage(Kademlia::CTimer::getThreadID(), WM_KADEMLIA_STARTSEARCH, 0, (LPARAM)pSearch))
							Kademlia::CSearchManager::deleteSearch(pSearch);
					}
					return;
				}
			}
		}
		else{
			m_currFile++;
		}
	}
}
