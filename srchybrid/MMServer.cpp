//this file is part of eMule
//Copyright (C)2003 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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
#include "mmserver.h"
#include "emule.h"
#include "opcodes.h"
#include "md5sum.h"
#include "searchdlg.h"
#include "packets.h"
#include "searchlist.h"
#include "Exceptions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#ifndef EWX_FORCEIFHUNG
#define EWX_FORCEIFHUNG     0x00000010
#endif


CMMServer::CMMServer(void)
{
	m_SendSearchList.SetSize(0);
	h_timer = NULL;
	m_cPWFailed = 0;
	m_dwBlocked = 0;
	m_pSocket = NULL;
	m_pPendingCommandSocket = NULL;

}

CMMServer::~CMMServer(void)
{
	DeleteSearchFiles();
	if (m_pSocket)
		delete m_pSocket;
	if (h_timer != NULL){
		KillTimer(0,h_timer);
	}
}

void CMMServer::Init(){
	if (theApp.glob_prefs->IsMMServerEnabled() && !m_pSocket){
		m_pSocket = new CListenMMSocket(this);
		if (!m_pSocket->Create()){
			StopServer();
			AddLogLine(false, GetResString(IDS_MMFAILED) );
		}
		else{
			AddLogLine(false, GetResString(IDS_MMSTARTED), theApp.glob_prefs->GetMMPort(), MM_STRVERSION );
		}
	}
}

void CMMServer::StopServer(){
	if (m_pSocket){
		delete m_pSocket;
		m_pSocket = NULL;
	}
}

void CMMServer::DeleteSearchFiles(){
	for (int i = 0; i!= m_SendSearchList.GetSize(); i++){
		delete m_SendSearchList[i];
	};
	m_SendSearchList.SetSize(0);
}

bool CMMServer::PreProcessPacket(char* pPacket, uint32 nSize, CMMSocket* sender){
	if (nSize >= 3){
		uint16 nSessionID;
		MEMCOPY(&nSessionID,pPacket+1,sizeof(nSessionID));
		if ( (m_nSessionID && nSessionID == m_nSessionID) || pPacket[0] == MMP_HELLO){
			return true;
		}
		else{
			CMMPacket* packet = new CMMPacket(MMP_INVALIDID);
			sender->SendPacket(packet);
			m_nSessionID = 0;
			return false;
		}
	}
	
	CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
	sender->SendPacket(packet);
	return false;
}

void CMMServer::ProcessHelloPacket(CMMData* data, CMMSocket* sender){
	CMMPacket* packet = new CMMPacket(MMP_HELLOANS);
	if (data->ReadByte() != MM_VERSION){
		packet->WriteByte(MMT_WRONGVERSION);
		sender->SendPacket(packet);
		return;
	}
	else{
		if(m_dwBlocked && m_dwBlocked > ::GetTickCount()){
			packet->WriteByte(MMT_WRONGPASSWORD);
			sender->SendPacket(packet);
			return;
		}
		CString plainPW = data->ReadString();
		CString testValue =MD5Sum(plainPW).GetHash();
		if (testValue != theApp.glob_prefs->GetMMPass() || plainPW.GetLength() == 0 ){
			m_dwBlocked = 0;
			packet->WriteByte(MMT_WRONGPASSWORD);
			sender->SendPacket(packet);
			m_cPWFailed++;
			if (m_cPWFailed == 3){
				m_cPWFailed = 0;
				m_dwBlocked = ::GetTickCount() + MMS_BLOCKTIME;
			}
			return;
		}
		else{
			// everything ok, new sessionid
			packet->WriteByte(MMT_OK);
			m_nSessionID = rand();
			packet->WriteShort(m_nSessionID);
			packet->WriteString(theApp.glob_prefs->GetUserNick());
			ProcessStatusRequest(sender,packet);
			//sender->SendPacket(packet);
		}

	}

}

void CMMServer::ProcessStatusRequest(CMMSocket* sender, CMMPacket* packet){
	if (packet == NULL)
		packet = new CMMPacket(MMP_STATUSANSWER);
	else
		packet->WriteByte(MMP_STATUSANSWER);

	packet->WriteShort((uint16)theApp.uploadqueue->GetDatarate()/100);
	packet->WriteShort((uint16)((theApp.glob_prefs->GetMaxGraphUploadRate()*1024)/100));
	packet->WriteShort((uint16)theApp.downloadqueue->GetDatarate()/100);
	packet->WriteShort((uint16)((theApp.glob_prefs->GetMaxGraphDownloadRate()*1024)/100));
	packet->WriteByte((uint8)theApp.downloadqueue->GetDownloadingFileCount());
	packet->WriteByte((uint8)theApp.downloadqueue->GetPausedFileCount());
	packet->WriteInt(theApp.stat_sessionReceivedBytes/1048576);
	packet->WriteShort((uint16)((theApp.emuledlg->statisticswnd.GetAvgDownloadRate(0)*1024)/100));
	if (theApp.serverconnect->IsConnected()){
		if(theApp.serverconnect->IsLowID())
			packet->WriteByte(1);
		else
			packet->WriteByte(2);
		if (theApp.serverconnect->GetCurrentServer() != NULL)
			packet->WriteInt(theApp.serverconnect->GetCurrentServer()->GetUsers());
	}
	else{
		packet->WriteByte(0);
		packet->WriteInt(0);
	}

	sender->SendPacket(packet);
}

void CMMServer::ProcessFileListRequest(CMMSocket* sender, CMMPacket* packet){
	
	if (packet == NULL)
		packet = new CMMPacket(MMP_FILELISTANS);
	else
		packet->WriteByte(MMP_FILELISTANS);
	
	int nCount = theApp.glob_prefs->GetCatCount();
	packet->WriteByte(nCount);
	for (int i = 0; i != nCount; i++){
		packet->WriteString(theApp.glob_prefs->GetCategory(i)->title);
	}

	nCount = (theApp.downloadqueue->GetFileCount() > 50)? 50 : theApp.downloadqueue->GetFileCount();
	m_SentFileList.SetSize(nCount);
	packet->WriteByte(nCount);
	for (int i = 0; i != nCount; i++){
		// while this is not the fastest method the trace this list, it's not timecritical here
		CPartFile* cur_file = theApp.downloadqueue->GetFileByIndex(i);
		if (cur_file == NULL){
			delete packet;
			packet = new CMMPacket(MMP_GENERALERROR);
			sender->SendPacket(packet);
			ASSERT ( false );
			return;
		}
		m_SentFileList[i] = cur_file;
		if (cur_file->GetStatus(false) == PS_PAUSED)
			packet->WriteByte(MMT_PAUSED);
		else{
			if (cur_file->GetTransferingSrcCount() > 0)
				packet->WriteByte(MMT_DOWNLOADING);
			else
				packet->WriteByte(MMT_WAITING);
		}
		packet->WriteString(cur_file->GetFileName());
		packet->WriteByte(cur_file->GetCategory());
	}
	sender->SendPacket(packet);
}

void CMMServer::ProcessFinishedListRequest(CMMSocket* sender){
	CMMPacket* packet = new CMMPacket(MMP_FINISHEDANS);
	int nCount = theApp.glob_prefs->GetCatCount();
	packet->WriteByte(nCount);
	for (int i = 0; i != nCount; i++){
		packet->WriteString(theApp.glob_prefs->GetCategory(i)->title);
	}

	nCount = (m_SentFinishedList.GetCount() > 30)? 30 : m_SentFinishedList.GetCount();
	packet->WriteByte(nCount);
	for (int i = 0; i != nCount; i++){
		CKnownFile* cur_file = m_SentFinishedList[i];
		packet->WriteByte(0xFF);
		packet->WriteString(cur_file->GetFileName());
		if (cur_file->IsPartFile())
			packet->WriteByte(((CPartFile*)cur_file)->GetCategory());
		else
			packet->WriteByte(0);
	}
	sender->SendPacket(packet);
}

void CMMServer::ProcessFileCommand(CMMData* data, CMMSocket* sender){
	uint8 byCommand = data->ReadByte();
	uint8 byFileIndex = data->ReadByte();
	if (byFileIndex >= m_SentFileList.GetSize()
		|| !theApp.downloadqueue->IsPartFile(m_SentFileList[byFileIndex]))
	{
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;		
	}
	CPartFile* selFile = m_SentFileList[byFileIndex];
	switch (byCommand){
		case MMT_PAUSE:
			selFile->PauseFile();
			break;
		case MMT_RESUME:
			selFile->ResumeFile();
			break;
		case MMT_CANCEL:{
			switch(selFile->GetStatus()) { 
				case PS_WAITINGFORHASH: 
				case PS_HASHING: 
				case PS_COMPLETING: 
				case PS_COMPLETE:  
					break;
				case PS_PAUSED:
					selFile->DeleteFile(); 
					break;
				default:
					if (theApp.glob_prefs->StartNextFile()) 
						theApp.downloadqueue->StartNextFile();
					selFile->DeleteFile(); 
			}
			break;
		}
		default:
			CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
			sender->SendPacket(packet);
			return;
	}
	CMMPacket* packet = new CMMPacket(MMP_FILECOMMANDANS);
	ProcessFileListRequest(sender,packet); 

}

void  CMMServer::ProcessDetailRequest(CMMData* data, CMMSocket* sender){
	uint8 byFileIndex = data->ReadByte();
	if (byFileIndex >= m_SentFileList.GetSize()
		|| !theApp.downloadqueue->IsPartFile(m_SentFileList[byFileIndex]))
	{
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;		
	}
	CPartFile* selFile = m_SentFileList[byFileIndex];
	CMMPacket* packet = new CMMPacket(MMP_FILEDETAILANS);
	uint32 test = selFile->GetFileSize();
	packet->WriteInt(selFile->GetFileSize());
	packet->WriteInt(selFile->GetTransfered());
	packet->WriteInt(selFile->GetCompletedSize());
	packet->WriteShort(selFile->GetDatarate()/100);
	packet->WriteShort(selFile->GetSourceCount());
	packet->WriteShort(selFile->GetTransferingSrcCount());
	if (selFile->IsAutoDownPriority()){
		packet->WriteByte(4);
	}
	else{
		packet->WriteByte(selFile->GetDownPriority());
	}
	uint8* parts = selFile->MMCreatePartStatus();
	packet->WriteByte(selFile->GetPartCount());
	for (int i = 0; i != selFile->GetPartCount(); i++){
		packet->WriteByte(parts[i]);
	}
	delete[] parts;
	sender->SendPacket(packet);
}

void  CMMServer::ProcessCommandRequest(CMMData* data, CMMSocket* sender){
	uint8 byCommand = data->ReadByte();
	bool bSuccess = false;
	bool bQueueCommand = false;
	switch(byCommand){
		case MMT_SDEMULE:
		case MMT_SDPC:
			h_timer = SetTimer(0,0,5000,CommandTimer);
			if (h_timer)
				bSuccess = true;
			bQueueCommand = true;
			break;
		case MMT_SERVERCONNECT:
			theApp.serverconnect->ConnectToAnyServer();
			bSuccess = true;
			break;
	}
	if (bSuccess){
		CMMPacket* packet = new CMMPacket(MMP_COMMANDANS);
		sender->SendPacket(packet);
		if (bQueueCommand)
			m_byPendingCommand = byCommand;
	}
	else{
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;
	}
}

void  CMMServer::ProcessSearchRequest(CMMData* data, CMMSocket* sender){
	DeleteSearchFiles();
	CString strSearch = data->ReadString();
	uint8 byType = data->ReadByte();
	CString strLocalSearchType;
	switch(byType){
		case 0:
			strLocalSearchType = GetResString(IDS_SEARCH_ANY);
			break;
		case 1:
			strLocalSearchType = GetResString(IDS_SEARCH_ARC);
			break;
		case 2:
			strLocalSearchType = GetResString(IDS_SEARCH_AUDIO);
			break;
		case 3:
			strLocalSearchType = GetResString(IDS_SEARCH_CDIMG);
			break;
		case 4:
			strLocalSearchType = GetResString(IDS_SEARCH_PRG);
			break;
		case 5:
			strLocalSearchType = GetResString(IDS_SEARCH_VIDEO);
			break;
		default:
			ASSERT ( false );
			strLocalSearchType = GetResString(IDS_SEARCH_ANY);
	}

	bool bServerError = false;

	if (!theApp.serverconnect->IsConnected()){
		CMMPacket* packet = new CMMPacket(MMP_SEARCHANS);
		packet->WriteByte(MMT_NOTCONNECTED);
		sender->SendPacket(packet);
		return;
	}

	CSafeMemFile searchdata(100);
	if (!GetSearchPacket(searchdata, strSearch, strLocalSearchType, 0, 0, -1, "", false) || searchdata.GetLength() == 0){
		bServerError = true;
	}
	else{
		h_timer = SetTimer(0,0,20000,CommandTimer);
		if (!h_timer){
			bServerError = true;
		}
	}
	if (bServerError){
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;
	}

	m_byPendingCommand = MMT_SEARCH;
	m_pPendingCommandSocket = sender;

	theApp.searchlist->NewSearch(NULL, strLocalSearchType , MMS_SEARCHID, true);
	Packet* searchpacket = new Packet(&searchdata);
	searchpacket->opcode = OP_SEARCHREQUEST;
	theApp.uploadqueue->AddUpDataOverheadServer(searchpacket->size);
	theApp.serverconnect->SendPacket(searchpacket,true);
	char buffer[] = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Type: application/octet-stream\r\n";
	sender->Send(buffer,strlen(buffer));
}

void CMMServer::SearchFinished(bool bTimeOut){
#define MAXRESULTS	20
	if (h_timer != 0){
		KillTimer(0,h_timer);
		h_timer = 0;
	}
	if (m_pPendingCommandSocket == NULL)
		return;
	if (bTimeOut){
		CMMPacket* packet = new CMMPacket(MMP_SEARCHANS);
		packet->WriteByte(MMT_TIMEDOUT);
		packet->m_bSpecialHeader = true;
		m_pPendingCommandSocket->SendPacket(packet);
	}
	else if (theApp.searchlist->GetFoundFiles(MMS_SEARCHID) == 0){
		CMMPacket* packet = new CMMPacket(MMP_SEARCHANS);
		packet->WriteByte(MMT_NORESULTS);
		packet->m_bSpecialHeader = true;
		m_pPendingCommandSocket->SendPacket(packet);
	}
	else{
		uint16 results = theApp.searchlist->GetFoundFiles(MMS_SEARCHID);
		if (results > MAXRESULTS)
			results = MAXRESULTS;
		m_SendSearchList.SetSize(results);
		CMMPacket* packet = new CMMPacket(MMP_SEARCHANS);
		packet->m_bSpecialHeader = true;
		packet->WriteByte(MMT_OK);
		packet->WriteByte(results);
		for (int i = 0; i != results; i++){
			CSearchFile* cur_file = theApp.searchlist->DetachNextFile(MMS_SEARCHID);
			m_SendSearchList[i] = cur_file;
			packet->WriteString(cur_file->GetFileName());
			packet->WriteShort( cur_file->GetSourceCount() );
			packet->WriteInt(cur_file->GetFileSize());
		}
		m_pPendingCommandSocket->SendPacket(packet);
		theApp.searchlist->RemoveResults(MMS_SEARCHID);
	}
	m_pPendingCommandSocket = NULL;
}

void  CMMServer::ProcessDownloadRequest(CMMData* data, CMMSocket* sender){
	uint8 byFileIndex = data->ReadByte();
	if (byFileIndex >= m_SendSearchList.GetSize() )
	{
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;		
	}
	CSearchFile* todownload = m_SendSearchList[byFileIndex];
	theApp.downloadqueue->AddSearchToDownload(todownload,0);
	CMMPacket* packet = new CMMPacket(MMP_DOWNLOADANS);
	if (theApp.downloadqueue->GetFileByID(todownload->GetFileHash()) != NULL){
		packet->WriteByte(MMT_OK);
	}
	else{
		packet->WriteByte(MMT_FAILED);
	}
	sender->SendPacket(packet);
}

void  CMMServer::ProcessPreviewRequest(CMMData* data, CMMSocket* sender){
	uint8 byFileType = data->ReadByte();
	uint8 byFileIndex = data->ReadByte();
	uint16 nDisplayWidth = data->ReadShort();
	uint8 nNumber = data->ReadByte();
	CKnownFile* knownfile;
	bool bError = false;

	if (byFileType == MMT_PARTFILFE){
		if (byFileIndex >= m_SentFileList.GetSize()
		|| !theApp.downloadqueue->IsPartFile(m_SentFileList[byFileIndex]))
		{
			bError = true;	
		}
		else
			knownfile = m_SentFileList[byFileIndex];
	}
	else if (byFileType == MMT_FINISHEDFILE){
		if (byFileIndex >= m_SentFinishedList.GetSize()
		|| !theApp.knownfiles->IsKnownFile(m_SentFinishedList[byFileIndex]))
		{
			bError = true;	
		}
		else
			knownfile = m_SentFinishedList[byFileIndex];
	}

	if (!bError){
		if (h_timer != 0)
			bError = true;
		else{
			h_timer = SetTimer(0,0,20000,CommandTimer);
			if (!h_timer){
				bError = true;
			}
			else{
				if (nDisplayWidth > 140)
					nDisplayWidth = 140;
				m_byPendingCommand = MMT_PREVIEW;
				m_pPendingCommandSocket = sender;
				if (!knownfile->GrabImage(1,(nNumber+1)*50.0,true,nDisplayWidth,this))
					PreviewFinished(NULL,0);
			}
		}
	}

	if (bError){
		CMMPacket* packet = new CMMPacket(MMP_GENERALERROR);
		sender->SendPacket(packet);
		ASSERT ( false );
		return;
	}
}

void CMMServer::PreviewFinished(CxImage** imgFrames, uint8 nCount){
	if (h_timer != 0){
		KillTimer(0,h_timer);
		h_timer = 0;
	}
	if (m_byPendingCommand != MMT_PREVIEW)
		return;
	m_byPendingCommand = 0;
	if (m_pPendingCommandSocket == NULL)
		return;

	CMMPacket* packet = new CMMPacket(MMP_PREVIEWANS);
	if (imgFrames != NULL && nCount != 0){
		packet->WriteByte(MMT_OK);
		CxImage* cur_frame = imgFrames[0];
		if (cur_frame == NULL){
			ASSERT ( false );
			return;
		}
		BYTE* abyResultBuffer = NULL;
		long nResultSize = 0;
		if (!cur_frame->Encode(abyResultBuffer, nResultSize, CXIMAGE_FORMAT_PNG)){
			ASSERT ( false );			
			return;
		}
		packet->WriteInt(nResultSize);
		packet->m_pBuffer->Write(abyResultBuffer, nResultSize);
		free(abyResultBuffer);
	}
	else{
		packet->WriteByte(MMT_FAILED);
	}

	m_pPendingCommandSocket->SendPacket(packet);
	m_pPendingCommandSocket = NULL;
}

void CMMServer::Process(){
	if (m_pSocket){ 
		m_pSocket->Process(); 
	} 
}

VOID CALLBACK CMMServer::CommandTimer(HWND hwnd, UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
{
	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
	try
	{
		KillTimer(0,theApp.mmserver->h_timer);
		theApp.mmserver->h_timer = 0;
		switch(theApp.mmserver->m_byPendingCommand){
			case MMT_SDPC:{
				HANDLE hToken; 
				TOKEN_PRIVILEGES tkp; 
				try{
					if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
						throw; 
					LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
					tkp.PrivilegeCount = 1;  // one privilege to set    
					tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
					AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
				}
				catch(...){
				}
				if (!ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCEIFHUNG, 0)) 
					break;
			}
			case MMT_SDEMULE:
				theApp.m_app_state	= APP_STATE_SHUTINGDOWN;
				SendMessage(theApp.emuledlg->m_hWnd,WM_CLOSE,0,0);
				break;
			case MMT_SEARCH:
				theApp.mmserver->SearchFinished(true);
				break;
			case MMT_PREVIEW:
				theApp.mmserver->PreviewFinished(NULL,0);
				break;
		}
	}
	CATCH_DFLT_EXCEPTIONS("CMMServer::CommandTimer")
}
