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
#include "ServerList.h"
#include "SafeFile.h"
#include "Exceptions.h"
#include "OtherFunctions.h"
#include "IPFilter.h"
#include "LastCommonRouteFinder.h"
#include "UploadQueue.h"
#include "DownloadQueue.h"
#include "Preferences.h"
#include "Opcodes.h"
#include "Server.h"
#include "Sockets.h"
#include "Packets.h"
#ifndef _CONSOLE
#include "emuledlg.h"
#include "HttpDownloadDlg.h"
#include "ServerWnd.h"
#endif
#include "Fakecheck.h" //MORPH - Added by SiRoB
#include "SharedFileList.h" //MORPH - Added by SiRoB
#include "PartFile.h" //Morph - added by AndCycle, itsonlyme: cacheUDPsearchResults

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define	SERVER_MET_FILENAME	_T("server.met")

CServerList::CServerList()
{
	servercount = 0;
	version = 0;
	serverpos = 0;
	searchserverpos = 0;
	statserverpos = 0;
	delservercount = 0;
	m_nLastSaved = ::GetTickCount();
}

CServerList::~CServerList()
{
	SaveServermetToFile();
	for (POSITION pos = list.GetHeadPosition(); pos != NULL; )
		delete list.GetNext(pos);
}

void CServerList::AutoUpdate()
{
	if (thePrefs.adresses_list.IsEmpty()){
		AfxMessageBox(GetResString(IDS_ERR_EMPTYADRESSESDAT),MB_ICONASTERISK);
		return;
	}
	bool bDownloaded=false;
	CString servermetdownload;
	CString servermetbackup;
	CString servermet;
	CString strURLToDownload; 
	servermetdownload.Format(_T("%sserver_met.download"), thePrefs.GetConfigDir());
	servermetbackup.Format(_T("%sserver_met.old"), thePrefs.GetConfigDir());
	servermet.Format(_T("%s") SERVER_MET_FILENAME, thePrefs.GetConfigDir());
	_tremove(servermetbackup);
	_tremove(servermetdownload);
	_trename(servermet,servermetbackup);
	
	POSITION Pos = thePrefs.adresses_list.GetHeadPosition(); 
	while (!bDownloaded && Pos != NULL){
		CHttpDownloadDlg dlgDownload;
		strURLToDownload = thePrefs.adresses_list.GetNext(Pos); 
		dlgDownload.m_sURLToDownload = strURLToDownload.GetBuffer();
		dlgDownload.m_sFileToDownloadInto = servermetdownload;
		if (dlgDownload.DoModal() == IDOK){
			bDownloaded=true;
		}
		else{
			AddLogLine(true,GetResString(IDS_ERR_FAILEDDOWNLOADMET), strURLToDownload.GetBuffer());
		}
	}
	if (bDownloaded){
		_trename(servermet, servermetdownload);
		_trename(servermetbackup, servermet);
	}
	else{
		_tremove(servermet);
		_trename(servermetbackup, servermet);
	}
}

bool CServerList::Init()
{
	// auto update the list by using an url
	if (thePrefs.AutoServerlist())
		AutoUpdate();
	// Load Metfile
	CString strPath;
	strPath.Format(_T("%s") SERVER_MET_FILENAME, thePrefs.GetConfigDir());
	bool bRes = AddServermetToList(strPath, false);
	if (thePrefs.AutoServerlist()){
		strPath.Format(_T("%sserver_met.download"), thePrefs.GetConfigDir());
		bool bRes2 = AddServermetToList(strPath);
		if( !bRes && bRes2 )
			bRes = true;
	}
	// insert static servers from textfile
	strPath.Format(_T("%sstaticservers.dat"), thePrefs.GetConfigDir());
	AddServersFromTextFile(strPath);
	//MORPH START added by Yun.SF3: Ipfilter.dat update
	if (thePrefs.IsAutoUPdateIPFilterEnabled())
	theApp.ipfilter->UpdateIPFilterURL();
	//MORPH END added by Yun.SF3: Ipfilter.dat update

	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	if (thePrefs.IsUpdateFakeStartupEnabled())
	theApp.FakeCheck->DownloadFakeList();
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating

	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	theApp.serverlist->GiveServersForTraceRoute();
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
	return bRes;
}
bool CServerList::AddServermetToList(const CString& strFile, bool merge) 
{
	if (!merge)
	{
		theApp.emuledlg->serverwnd->serverlistctrl.DeleteAllItems();
		RemoveAllServers();
	}
	CSafeBufferedFile servermet;
	CFileException fexp;
	if (!servermet.Open(strFile,CFile::modeRead|CFile::osSequentialScan|CFile::typeBinary, &fexp)){
		if(!merge){
			CString strError(GetResString(IDS_ERR_LOADSERVERMET));
			char szError[MAX_CFEXP_ERRORMSG];
			if (fexp.GetErrorMessage(szError,MAX_CFEXP_ERRORMSG)){
				strError += _T(" - ");
				strError += szError;
			}
			AddLogLine(false, _T("%s"), strError);
		}
		return false;
	}
	setvbuf(servermet.m_pStream, NULL, _IOFBF, 16384);
	try{
		version = servermet.ReadUInt8();
		if (version != 0xE0 && version != MET_HEADER){
			servermet.Close();
			AddLogLine(false,GetResString(IDS_ERR_BADSERVERMETVERSION),version);
			return false;
		}
		theApp.emuledlg->serverwnd->serverlistctrl.Hide();
		theApp.emuledlg->serverwnd->serverlistctrl.SetRedraw(FALSE);
		UINT fservercount = servermet.ReadUInt32();
		
		ServerMet_Struct sbuffer;
		UINT iAddCount = 0;
		for (UINT j = 0; j < fservercount; j++)
		{
			// get server
			servermet.Read(&sbuffer,sizeof(ServerMet_Struct));
			CServer* newserver = new CServer(&sbuffer);
			//add tags
			for (UINT i = 0; i < sbuffer.tagcount; i++)
				newserver->AddTagFromFile(&servermet);
			// set listname for server
			if (!newserver->GetListName()){
				CString listname;
				listname.Format(_T("Server %s"), newserver->GetAddress());
				newserver->SetListName(listname);
			}
			if (!theApp.emuledlg->serverwnd->serverlistctrl.AddServer(newserver,true) ){
				CServer* update = theApp.serverlist->GetServerByAddress(newserver->GetAddress(), newserver->GetPort());
				if(update){
					update->SetListName( newserver->GetListName());
					if( newserver->GetDescription() )
						update->SetDescription( newserver->GetDescription());
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(update);
				}
				delete newserver;
			}
			else
				iAddCount++;
		}

		if (!merge)
			AddLogLine(true,GetResString(IDS_SERVERSFOUND),fservercount);
		else
			AddLogLine(true,GetResString(IDS_SERVERSADDED), iAddCount, fservercount-iAddCount);
		servermet.Close();
	}
	catch(CFileException* error){
		if (thePrefs.GetVerbose())
		{
			if (error->m_cause == CFileException::endOfFile){
				AddDebugLogLine(true,GetResString(IDS_ERR_BADSERVERLIST));
			}
			else{
				char buffer[MAX_CFEXP_ERRORMSG];
				error->GetErrorMessage(buffer,MAX_CFEXP_ERRORMSG);
				AddDebugLogLine(true,GetResString(IDS_ERR_FILEERROR_SERVERMET),buffer);
			}
		}
		error->Delete();
	}
	theApp.emuledlg->serverwnd->serverlistctrl.SetRedraw(TRUE);
	theApp.emuledlg->serverwnd->serverlistctrl.Visable();
	return true;
}

bool CServerList::AddServer(CServer* in_server)
{
	if (!IsGoodServerIP(in_server)){ // check for 0-IP, localhost and optionally for LAN addresses
		if (thePrefs.GetLogFilteredIPs())
			AddDebugLogLine(false, _T("Ignored server (IP=%s)"), ipstr(in_server->GetIP()));
		return false;
	}

	if (thePrefs.FilterServerByIP()){
		if (in_server->HasDynIP())
			return false;
		if (theApp.ipfilter->IsFiltered(in_server->GetIP())){
			if (thePrefs.GetLogFilteredIPs())
				AddDebugLogLine(false, _T("Ignored server (IP=%s) - IP filter (%s)"), ipstr(in_server->GetIP()), theApp.ipfilter->GetLastHit());
			return false;
		}
	}

	CServer* test_server = GetServerByAddress(in_server->GetAddress(), in_server->GetPort());
	if (test_server){
		test_server->ResetFailedCount();
		theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer( test_server );		
		return false;
	}
	list.AddTail(in_server);

	return true;
}

// ZZ:UploadSpeedSense -->
bool CServerList::GiveServersForTraceRoute()
{
    return theApp.lastCommonRouteFinder->AddHostsToCheck(list);
}
// ZZ:UploadSpeedSense <--

void CServerList::ServerStats()
{
	// Update the server list even if we are connected to Kademlia only. The idea is for both networks to keep 
	// each other up to date.. Kad network can get you back into the ED2K network.. And the ED2K network can get 
	// you back into the Kad network..
	if (theApp.IsConnected() && theApp.serverconnect->IsUDPSocketAvailable() && list.GetCount() > 0)
	{
		CServer* ping_server = GetNextStatServer();
		if( !ping_server )
			return;

		uint32 tNow = (uint32)time(NULL);
		const CServer* test = ping_server;
        while(ping_server->GetLastPingedTime() != 0 && (tNow - ping_server->GetLastPingedTime()) < UDPSERVSTATREASKTIME){ 
			ping_server = GetNextStatServer();
			if( ping_server == test )
				return;
		}
		if (ping_server->GetFailedCount() >= thePrefs.GetDeadserverRetries() && thePrefs.DeadServer()){
			theApp.emuledlg->serverwnd->serverlistctrl.RemoveServer(ping_server);
			return;
		}
		Packet* packet = new Packet( OP_GLOBSERVSTATREQ, 4 );
		srand(tNow);
		uint32 time = 0x55AA0000 + (uint16)rand();
		ping_server->SetChallenge(time);
		PokeUInt32(packet->pBuffer, time);
		ping_server->SetLastPinged( ::GetTickCount() );
		ping_server->SetLastPingedTime(tNow);
		ping_server->AddFailedCount();
		theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer( ping_server );
		if (thePrefs.GetDebugServerUDPLevel() > 0)
			Debug(">>> Sending OP__GlobServStatReq to %s:%u\n", ping_server->GetAddress(), ping_server->GetPort());
		theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
		theApp.serverconnect->SendUDPPacket( packet, ping_server, true );
		
		ping_server->SetLastDescPingedCount(false);
		if(ping_server->GetLastDescPingedCount() < 2){
			// eserver 16.45+ supports a new OP_SERVER_DESC_RES answer, if the OP_SERVER_DESC_REQ contains a uint32
			// challenge, the server returns additional info with OP_SERVER_DESC_RES. To properly distinguish the
			// old and new OP_SERVER_DESC_RES answer, the challenge has to be selected carefully. The first 2 bytes 
			// of the challenge (in network byte order) MUST NOT be a valid string-len-int16!
			packet = new Packet(OP_SERVER_DESC_REQ,4);
			uint32 uDescReqChallenge = ((uint32)rand() << 16) + INV_SERV_DESC_LEN; // 0xF0FF = an 'invalid' string length.
			ping_server->SetDescReqChallenge(uDescReqChallenge);
			PokeUInt32(packet->pBuffer, uDescReqChallenge);
			theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
			if (thePrefs.GetDebugServerUDPLevel() > 0)
				Debug(">>> Sending OP__ServDescReq     to %s:%u, challenge %08x\n", ping_server->GetAddress(), ping_server->GetPort(), uDescReqChallenge);
			theApp.serverconnect->SendUDPPacket( packet, ping_server, true );
		}
		else{
			ping_server->SetLastDescPingedCount(true);
		}
	}
}

bool CServerList::IsGoodServerIP(const CServer* in_server) const
{
	if (in_server->HasDynIP())
		return true;
	return IsGoodIPPort(in_server->GetIP(), in_server->GetPort());
}

void CServerList::RemoveServer(const CServer* out_server)
{
	for(POSITION pos = list.GetHeadPosition(); pos != NULL; )
	{
		POSITION pos2 = pos;
		const CServer* test_server = list.GetNext(pos);
		if (test_server == out_server){
			if (theApp.downloadqueue->cur_udpserver == out_server)
				theApp.downloadqueue->cur_udpserver = NULL;
			list.RemoveAt(pos2);
			delservercount++;
			delete test_server;
			return;
		}
	}
}

void CServerList::RemoveAllServers()
{
	for (POSITION pos = list.GetHeadPosition(); pos != NULL; ){
		delete list.GetNext(pos);
		delservercount++;
	}
	list.RemoveAll();
}

void CServerList::GetStatus(uint32& total, uint32& failed, uint32& user, uint32& file, 
							uint32& tuser, uint32& tfile, float& occ) const
{
	total = list.GetCount();
	failed = 0;
	user = 0;
	file = 0;
	tuser=0;
	tfile = 0;
	occ=0;
	uint32 maxusers=0;
	uint32 tuserk = 0;

	for (POSITION pos = list.GetHeadPosition(); pos != 0; ){
		const CServer* curr = list.GetNext(pos);
		if( curr->GetFailedCount() ){
			failed++;
		}
		else{
			user += curr->GetUsers();
			file += curr->GetFiles();
		}
		tuser += curr->GetUsers();
		tfile += curr->GetFiles();
		
		if (curr->GetMaxUsers()) {
			tuserk += curr->GetUsers(); // total users on servers with known maximum
			maxusers+=curr->GetMaxUsers();
		}
	}
	
	if (maxusers > 0)
		occ = (float)(tuserk*100) / maxusers;
}

void CServerList::GetUserFileStatus(uint32& user, uint32& file) const
{
	user = 0;
	file = 0;
	for (POSITION pos = list.GetHeadPosition(); pos != 0; ){
		const CServer* curr = list.GetNext(pos);
		if( !curr->GetFailedCount() ){
			user += curr->GetUsers();
			file += curr->GetFiles();
		}
	}
}

void CServerList::MoveServerDown(const CServer* aServer)
{
   POSITION pos1, pos2;
   uint16 i = 0;
   for( pos1 = list.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
	   list.GetNext(pos1);
	   CServer* cur_server = list.GetAt(pos2);
	   if (cur_server==aServer){
		   list.AddTail(cur_server);
		   list.RemoveAt(pos2);
		   return;
	   }
	   i++;
	   if (i == list.GetCount())
		   break;
   }
}

void CServerList::SetServerPosition(uint32 newPosition)
{
	if (newPosition < (uint32)list.GetCount())
		serverpos = newPosition;
	else
		serverpos = 0;
}

void CServerList::Sort()
{
   POSITION pos1, pos2;
   uint16 i = 0;
   for( pos1 = list.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
	   list.GetNext(pos1);
	   CServer* cur_server = list.GetAt(pos2);
	   if (cur_server->GetPreferences()== SRV_PR_HIGH){
		   list.AddHead(cur_server);
		   list.RemoveAt(pos2);
	   }
	   else if (cur_server->GetPreferences() == SRV_PR_LOW){
		   list.AddTail(cur_server);
		   list.RemoveAt(pos2);
	   }
	   i++;
	   if (i == list.GetCount())
		   break;
   }
}

//EastShare Start - PreferShareAll by AndCycle
// SLUGFILLER: preferShareAll
void CServerList::PushBackNoShare(){
	uint32 files = theApp.sharedfiles->GetCount()+theApp.sharedfiles->GetHashingCount();	// SLUGFILLER: SafeHash - use estimate
	POSITION pos1, pos2;
	uint16 i = 0;
	for( pos1 = list.GetHeadPosition(); ( pos2 = pos1 ) != NULL; ){
		list.GetNext(pos1);
		CServer* cur_server = list.GetAt(pos2);
		if ((cur_server->GetSoftFiles() && cur_server->GetSoftFiles() < files) ||
			(cur_server->GetHardFiles() && cur_server->GetHardFiles() < files)){
			list.AddTail(cur_server);
			list.RemoveAt(pos2);
		}
		i++;
		if (i == list.GetCount())
			break;
	}
}
// SLUGFILLER: preferShareAll
//EastShare End - PreferShareAll by AndCycle

CServer* CServerList::GetNextServer()
{
	if (serverpos >= (uint32)list.GetCount())
		return 0;

	CServer* nextserver = NULL;
	uint32 i = 0;
	while (!nextserver && i < (uint32)list.GetCount()){
		POSITION posIndex = list.FindIndex(serverpos);
		if (posIndex == NULL) {	// check if search position is still valid (could be corrupted by server delete operation)
			posIndex = list.GetHeadPosition();
			serverpos = 0;
		}

		nextserver = list.GetAt(posIndex);
		serverpos++;
		i++;
		// TODO: Add more option to filter bad server like min ping, min users etc
	}
	return nextserver;
}

CServer* CServerList::GetNextSearchServer()
{
	CServer* nextserver = NULL;
	uint32 i = 0;
	while (!nextserver && i < (uint32)list.GetCount()){
		POSITION posIndex = list.FindIndex(searchserverpos);
		if (posIndex == NULL) {	// check if search position is still valid (could be corrupted by server delete operation)
			posIndex = list.GetHeadPosition();
			searchserverpos=0;
		}
		nextserver = list.GetAt(posIndex);
		searchserverpos++;
		i++;
		if (searchserverpos == list.GetCount())
			searchserverpos = 0;
	}
	return nextserver;
}

CServer* CServerList::GetNextStatServer()
{
	CServer* nextserver = NULL;
	uint32 i = 0;
	while (!nextserver && i < (uint32)list.GetCount()){
		POSITION posIndex = list.FindIndex(statserverpos);
		if (posIndex == NULL) {	// check if search position is still valid (could be corrupted by server delete operation)
			posIndex = list.GetHeadPosition();
			statserverpos=0;
		}

		nextserver = list.GetAt(posIndex);
		statserverpos++;
		i++;
		if (statserverpos == list.GetCount())
			statserverpos = 0;
	}
	return nextserver;
}

CServer* CServerList::GetNextServer(const CServer* lastserver) const
{
	if (list.IsEmpty())
		return 0;
	if (!lastserver)
		return list.GetHead();

	POSITION pos = list.Find(const_cast<CServer*>(lastserver));
	if (!pos){
		TRACE("Error: CServerList::GetNextServer");
		return list.GetHead();
	}
	list.GetNext(pos);
	if (!pos)
		return NULL;
	else
		return list.GetAt(pos);
}

//Morph Start - added by AndCycle, itsonlyme: cacheUDPsearchResults
// itsonlyme: cacheUDPsearchResults
CServer* CServerList::GetNextServer(const CServer *lastserver, CPartFile *file) const
{
	if (!file)
		return GetNextServer(lastserver);

	CServer *nextServer = file->GetNextAvailServer();
	if (!nextServer) 
		return GetNextServer(lastserver);
	else
		return nextServer;
}
// itsonlyme: cacheUDPsearchResults
//Morph End - added by AndCycle, itsonlyme: cacheUDPsearchResults

CServer* CServerList::GetServerByAddress(LPCTSTR address, uint16 port) const
{
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
        if (( port == s->GetPort() || port==0) && !strcmp(s->GetAddress(),address)) 
			return s; 
	}
	return NULL;
}

CServer* CServerList::GetServerByIP(uint32 nIP) const
{
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
		if (s->GetIP() == nIP)
			return s; 
	}
	return NULL;
}

CServer* CServerList::GetServerByIP(uint32 nIP, uint16 nPort) const
{
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
		if (s->GetIP() == nIP && s->GetPort() == nPort)
			return s; 
	}
	return NULL;
}

bool CServerList::SaveServermetToFile()
{
	if (thePrefs.GetLogFileSaving())
		AddDebugLogLine(false, "Saving servers list file \"%s\"", SERVER_MET_FILENAME);
	m_nLastSaved = ::GetTickCount(); 
	CString newservermet(thePrefs.GetConfigDir());
	newservermet += SERVER_MET_FILENAME _T(".new");
	CSafeBufferedFile servermet;
	CFileException fexp;
	if (!servermet.Open(newservermet, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary, &fexp)){
		CString strError(GetResString(IDS_ERR_SAVESERVERMET));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(true, _T("%s"), strError);
		return false;
	}
	setvbuf(servermet.m_pStream, NULL, _IOFBF, 16384);
	
	try{
		servermet.WriteUInt8(0xE0);
		
		UINT fservercount = list.GetCount();
		servermet.WriteUInt32(fservercount);
		
		for (UINT j = 0; j < fservercount; j++)
		{
			ServerMet_Struct sbuffer;
			const CServer* nextserver = GetServerAt(j);

			sbuffer.ip = nextserver->GetIP();
			//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
			/*
			sbuffer.port = nextserver->GetPort();
			*/
			sbuffer.port = nextserver->GetConnPort();
			//Morph End - added by AndCycle, aux Ports, by lugdunummaster
			uint16 tagcount = 11;
			if (nextserver->GetListName() && nextserver->GetListName()[0] != '\0')
				tagcount++;
			if (nextserver->GetDynIP() && nextserver->GetDynIP()[0] != '\0')
				tagcount++;
			if (nextserver->GetDescription() && nextserver->GetDescription()[0] != '\0')
				tagcount++;
			//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
			if (nextserver->GetPort() != nextserver->GetConnPort())
				tagcount++;
			//Morph End- added by AndCycle, aux Ports, by lugdunummaster
			sbuffer.tagcount = tagcount;
			servermet.Write(&sbuffer, sizeof sbuffer);
			
			if( nextserver->GetListName() && nextserver->GetListName()[0] != '\0' ){
				CTag servername( ST_SERVERNAME, nextserver->GetListName() );
				servername.WriteTagToFile(&servermet);
			}
			if( nextserver->GetDynIP() && nextserver->GetDynIP()[0] != '\0' ){
				CTag serverdynip( ST_DYNIP, nextserver->GetDynIP() );
				serverdynip.WriteTagToFile(&servermet);
			}
			if( nextserver->GetDescription() && nextserver->GetDescription()[0] != '\0' ){
				CTag serverdesc( ST_DESCRIPTION, nextserver->GetDescription() );
				serverdesc.WriteTagToFile(&servermet);
			}
			CTag serverfail(ST_FAIL, nextserver->GetFailedCount() );
			serverfail.WriteTagToFile(&servermet);
			CTag serverpref( ST_PREFERENCE, nextserver->GetPreferences() );
			serverpref.WriteTagToFile(&servermet);
			CTag serveruser("users", nextserver->GetUsers() );
			serveruser.WriteTagToFile(&servermet);
			CTag serverfiles("files", nextserver->GetFiles() );
			serverfiles.WriteTagToFile(&servermet);
			//Morph Start - added by AndCycle, aux Ports, by lugdunummaster
			if (nextserver->GetPort() != nextserver->GetConnPort()) {
				char temp[64] ;
				sprintf(temp, "%d", nextserver->GetPort()) ;
				CTag auxportslist("auxportslist", temp);
				auxportslist.WriteTagToFile(&servermet);
			}
			//Morph End - added by AndCycle, aux Ports, by lugdunummaster
			CTag serverping(ST_PING, nextserver->GetPing() );
			serverping.WriteTagToFile(&servermet);
			CTag serverlastp(ST_LASTPING, nextserver->GetLastPingedTime() );
			serverlastp.WriteTagToFile(&servermet);
			CTag servermaxusers(ST_MAXUSERS, nextserver->GetMaxUsers() );
			servermaxusers.WriteTagToFile(&servermet);
			CTag softfiles(ST_SOFTFILES, nextserver->GetSoftFiles() );
			softfiles.WriteTagToFile(&servermet);
			CTag hardfiles(ST_HARDFILES, nextserver->GetHardFiles() );
			hardfiles.WriteTagToFile(&servermet);
			// as long as we don't receive an integer version tag from the local server (TCP) we store it as string
			CTag version(ST_VERSION, (LPCSTR)nextserver->GetVersion() );
			version.WriteTagToFile(&servermet);
			CTag tagUDPFlags(ST_UDPFLAGS, nextserver->GetUDPFlags() );
			tagUDPFlags.WriteTagToFile(&servermet);
		}

		if (thePrefs.GetCommitFiles() >= 2 || (thePrefs.GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			servermet.Flush(); // flush file stream buffers to disk buffers
			if (_commit(_fileno(servermet.m_pStream)) != 0) // commit disk buffers to disk
				AfxThrowFileException(CFileException::hardIO, GetLastError(), servermet.GetFileName());
		}
		servermet.Close();

		CString curservermet(thePrefs.GetConfigDir());
		CString oldservermet(thePrefs.GetConfigDir());
		curservermet += SERVER_MET_FILENAME;
		oldservermet += _T("server_met.old");
		
		if (_taccess(oldservermet, 0) == 0)
			CFile::Remove(oldservermet);
		if (_taccess(curservermet, 0) == 0)
			CFile::Rename(curservermet,oldservermet);
		CFile::Rename(newservermet,curservermet);
	}
	catch(CFileException* error) {
		CString strError(GetResString(IDS_ERR_SAVESERVERMET2));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (error->GetErrorMessage(szError, ARRSIZE(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("%s"), strError);
		error->Delete();
		return false;
	}
	return true;
}

void CServerList::AddServersFromTextFile(const CString& strFilename)
{
	CStdioFile f;
	if (!f.Open(strFilename, CFile::modeRead | CFile::typeText))
		return;

	CString strLine;
	while (f.ReadString(strLine))
	{
		// format is host:port,priority,Name
		if (strLine.GetLength() < 5)
			continue;
		if (strLine.GetAt(0) == _T('#') || strLine.GetAt(0) == _T('/'))
			continue;

		// fetch host
		int pos = strLine.Find(_T(':'));
		if (pos == -1){
			pos = strLine.Find(_T(','));
			if (pos == -1) 
			continue;
		}
		CString strHost = strLine.Left(pos);
		strLine = strLine.Mid(pos+1);
		// fetch  port
		pos = strLine.Find(_T(','));
		if (pos == -1)
			continue;
		CString strPort = strLine.Left(pos);
		strLine = strLine.Mid(pos+1);

		// Barry - fetch priority
		pos = strLine.Find(_T(','));
		int priority = SRV_PR_HIGH;
		if (pos == 1)
		{
			CString strPriority = strLine.Left(pos);
			try
			{
				priority = atoi(strPriority.GetBuffer(0));
				if (priority < 0 || priority > 2)
					priority = SRV_PR_HIGH;
			}
			catch (...){
				ASSERT(0);
			}
			strLine = strLine.Mid(pos+1);
		}
	
		// fetch name
		CString strName = strLine;
		strName.Replace(_T("\r"), _T(""));
		strName.Replace(_T("\n"), _T(""));

		// create server object and add it to the list
		CServer* nsrv = new CServer(atoi(strPort), strHost.GetBuffer());
		nsrv->SetListName(strName.GetBuffer());
		nsrv->SetIsStaticMember(true);
		nsrv->SetPreference(priority); 
		if (!theApp.emuledlg->serverwnd->serverlistctrl.AddServer(nsrv, true))
		{
			delete nsrv;
			CServer* srvexisting = GetServerByAddress(strHost.GetBuffer(), atoi(strPort));
			if (srvexisting) {
				srvexisting->SetListName(strName.GetBuffer());
				srvexisting->SetIsStaticMember(true);
				srvexisting->SetPreference(priority); 
				if (theApp.emuledlg->serverwnd)
					theApp.emuledlg->serverwnd->serverlistctrl.RefreshServer(srvexisting);
			}

		}
	}

	f.Close();
}

void CServerList::Process()
{
	if (::GetTickCount() - m_nLastSaved > MIN2MS(17))
		SaveServermetToFile();
}

//EastShare Start - added by AndCycle, IP to Country
void CServerList::ResetIP2Country(){

	CServer *cur_server;

	for(POSITION pos = list.GetHeadPosition(); pos != NULL; list.GetNext(pos)){
		cur_server = list.GetAt(pos);
		cur_server->ResetIP2Country();
	}
}
//EastShare End - added by AndCycle, IP to Country
