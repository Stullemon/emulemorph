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
#include "serverlist.h"
#include "emule.h"
#include "HttpDownloadDlg.h"
#include "safefile.h"
#include <io.h>
#include "Exceptions.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CServerList::CServerList(CPreferences* in_prefs){
	servercount = 0;
	version = 0;
	serverpos = 0;
	searchserverpos = 0;
	statserverpos = 0;
	app_prefs = in_prefs;
	//udp_timer = 0;
	delservercount = 0;
	m_nLastSaved = ::GetTickCount();
}

void CServerList::AutoUpdate(){
	if (app_prefs->adresses_list.IsEmpty()){
		AfxMessageBox(GetResString(IDS_ERR_EMPTYADRESSESDAT),MB_ICONASTERISK);
		return;
	}
	bool bDownloaded=false;
	CString servermetdownload;
	CString servermetbackup;
	CString servermet;
	CString strURLToDownload; 
	servermetdownload.Format("%sserver_met.download",app_prefs->GetConfigDir());
	servermetbackup.Format("%sserver_met.old",app_prefs->GetConfigDir());
	servermet.Format("%sserver.met",app_prefs->GetConfigDir());
	remove(servermetbackup);
	remove(servermetdownload);
	rename(servermet,servermetbackup);
	
	POSITION Pos = app_prefs->adresses_list.GetHeadPosition(); 
	while ((!bDownloaded) && (Pos != NULL)){
		CHttpDownloadDlg dlgDownload;
		strURLToDownload = app_prefs->adresses_list.GetNext(Pos); 
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
		rename(servermet, servermetdownload);
		rename(servermetbackup, servermet);
	}
	else{
		remove(servermet);
		rename(servermetbackup,servermet);
	}
}

bool CServerList::Init(){
	// auto update the list by using an url
	if (app_prefs->AutoServerlist())
		AutoUpdate();
	// Load Metfile
	CString strPath;
	strPath.Format( "%sserver.met", app_prefs->GetConfigDir());
	bool bRes = AddServermetToList(strPath, false);
	if( theApp.glob_prefs->AutoServerlist() ){
		strPath.Format( "%sserver_met.download", app_prefs->GetConfigDir());
		bool bRes2 = AddServermetToList(strPath);
		if( !bRes && bRes2 )
			bRes = true;
	}
	// insert static servers from textfile
	strPath.Format( "%sstaticservers.dat", app_prefs->GetConfigDir());
	AddServersFromTextFile(strPath);
	//MORPH START added by Yun.SF3: Ipfilter.dat update
	if (app_prefs->IsAutoUPdateIPFilterEnabled())
	theApp.ipfilter->UpdateIPFilterURL();
	//MORPH END added by Yun.SF3: Ipfilter.dat update

	//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
	if (app_prefs->IsUpdateFakeStartupEnabled())
	theApp.FakeCheck->DownloadFakeList();
	//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating

	//MORPH START - Added by SiRoB, ZZ Upload system (USS)
	theApp.serverlist->GiveServersForTraceRoute();
	//MORPH END   - Added by SiRoB, ZZ Upload system (USS)
	return bRes;
}

bool CServerList::AddServermetToList(CString strFile, bool merge){
	if (!merge)
	{
		theApp.emuledlg->serverwnd.serverlistctrl.DeleteAllItems();
		this->RemoveAllServers();
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
		servermet.Read(&version,1);
		if (version != 0xE0 && version != MET_HEADER){
			servermet.Close();
			AddLogLine(false,GetResString(IDS_ERR_BADSERVERMETVERSION),version);
			return false;
		}
		theApp.emuledlg->serverwnd.serverlistctrl.Hide();
		theApp.emuledlg->serverwnd.serverlistctrl.SetRedraw(FALSE);
		uint32 fservercount;
		servermet.Read(&fservercount,4);
		
		ServerMet_Struct sbuffer;
		uint32 iAddCount = 0;
		for (uint32 j = 0;j != fservercount;j++){
			// get server
			servermet.Read(&sbuffer,sizeof(ServerMet_Struct));
			CServer* newserver = new CServer(&sbuffer);
			//add tags
			for (uint32 i=0;i != sbuffer.tagcount;i++)
				newserver->AddTagFromFile(&servermet);
			// set listname for server
			if (!newserver->GetListName()){
				char* listname = new char[strlen(newserver->GetAddress())+8];
				sprintf(listname,"Server %s",newserver->GetAddress());
				newserver->SetListName(listname);
				delete[] listname;
			}
			if (!theApp.emuledlg->serverwnd.serverlistctrl.AddServer(newserver,true) ){
				CServer* update = theApp.serverlist->GetServerByAddress(newserver->GetAddress(), newserver->GetPort());
				if(update){
					update->SetListName( newserver->GetListName());
					if( newserver->GetDescription() )
						update->SetDescription( newserver->GetDescription());
					theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer(update);
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
		OUTPUT_DEBUG_TRACE();
		if (error->m_cause == CFileException::endOfFile)
			AddDebugLogLine(true,GetResString(IDS_ERR_BADSERVERLIST));
		else{
			char buffer[MAX_CFEXP_ERRORMSG];
			error->GetErrorMessage(buffer,MAX_CFEXP_ERRORMSG);
			AddDebugLogLine(true,GetResString(IDS_ERR_FILEERROR_SERVERMET),buffer);
		}
		error->Delete();	//memleak fix
	}
	theApp.emuledlg->serverwnd.serverlistctrl.SetRedraw(TRUE);
	theApp.emuledlg->serverwnd.serverlistctrl.Visable();
	return true;
}

bool CServerList::AddServer(CServer* in_server){

	if (!IsGoodServerIP(in_server)){ // check for 0-IP, localhost and optionally for LAN addresses
		uint32 nIP = in_server->GetIP();
		AddDebugLogLine(false, _T("Ignored server with IP=%s"), inet_ntoa(*(in_addr*)&nIP));
		return false;
	}

	if (theApp.glob_prefs->FilterServerByIP()){
		if (in_server->HasDynIP())
			return false;
		uint32 nIP = in_server->GetIP();
		if (theApp.ipfilter->IsFiltered(nIP)){
			AddDebugLogLine(false, _T("IPfiltered server with IP=%s (%s)"), inet_ntoa(*(in_addr*)&nIP), theApp.ipfilter->GetLastHit());
			return false;
		}
	}

	CServer* test_server = GetServerByAddress(in_server->GetAddress(), in_server->GetPort());
	if (test_server){
		test_server->ResetFailedCount();
		theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer( test_server );		
		return false;
	}
	list.AddTail(in_server);

	return true;
}

//MORPH START - Added by SiRoB, ZZ Upload system (USS)
bool CServerList::GiveServersForTraceRoute() {
    return theApp.lastCommonRouteFinder->AddHostsToCheck(list);
}
//MORPH END   - Added by SiRoB, ZZ Upload system (USS)

void CServerList::ServerStats(){
	uint32 temp;
	temp = (uint32)time(NULL);

	if( theApp.IsConnected() && theApp.serverconnect->IsUDPSocketAvailable() && list.GetCount() > 0 ) {
		CServer* ping_server = GetNextStatServer();
		CServer* test = ping_server;
		if( !ping_server )
			return;
        while(ping_server->GetLastPingedTime() != 0 && ((uint32)time(NULL) - ping_server->GetLastPingedTime()) < UDPSERVSTATREASKTIME){ 
			ping_server = this->GetNextStatServer();
			if( ping_server == test )
				return;
		}
		if( ping_server->GetFailedCount() >= theApp.glob_prefs->GetDeadserverRetries() && theApp.glob_prefs->DeadServer() ){
			theApp.emuledlg->serverwnd.serverlistctrl.RemoveServer(ping_server);
			return;
		}
		Packet* packet = new Packet( OP_GLOBSERVSTATREQ, 4 );
		srand((unsigned)time(NULL));
		uint32 time = 0x55AA0000 + (uint16)rand();
		ping_server->SetChallenge(time);
		MEMCOPY( packet->pBuffer, &time, 4 );
		ping_server->SetLastPinged( ::GetTickCount() );
		ping_server->SetLastPingedTime( temp );
		ping_server->AddFailedCount();
		theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer( ping_server );
		if (theApp.glob_prefs->GetDebugServerUDP())
			Debug(">>> Sending OP__GlobServStatReq to %s:%u\n", ping_server->GetAddress(), ping_server->GetPort());
		theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
		theApp.serverconnect->SendUDPPacket( packet, ping_server, true );
		ping_server->SetLastDescPingedCount(false);
		if(ping_server->GetLastDescPingedCount() < 2){
			if (theApp.glob_prefs->GetDebugServerUDP())
				Debug(">>> Sending OP__ServDescReq     to %s:%u\n", ping_server->GetAddress(), ping_server->GetPort());
			packet = new Packet( OP_SERVER_DESC_REQ,0);
			theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
			theApp.serverconnect->SendUDPPacket( packet, ping_server, true );
		}
		else{
			ping_server->SetLastDescPingedCount(true);
		}
	}
}

bool CServerList::IsGoodServerIP(CServer* in_server){ 
	if (in_server->HasDynIP())
		return true;
	return IsGoodIPPort(in_server->GetIP(), in_server->GetPort());
}

void CServerList::RemoveServer(CServer* out_server){
	for(POSITION pos = list.GetHeadPosition(); pos != NULL; )
	{
		POSITION pos2 = pos;
		CServer* test_server = list.GetNext(pos);
		if (test_server == out_server){
			if (theApp.downloadqueue->cur_udpserver == out_server)
				theApp.downloadqueue->cur_udpserver = 0;
			list.RemoveAt(pos2);
			delservercount++;
			delete test_server;
			return;
		}
	}
}

void CServerList::RemoveAllServers(){
	for(POSITION pos = list.GetHeadPosition(); pos != NULL; pos = list.GetHeadPosition()) {
		delete list.GetAt(pos);
		list.RemoveAt(pos);
		delservercount++;
	}
}

void CServerList::GetStatus(uint32 &total, uint32 &failed, uint32 &user, uint32 &file, uint32 &tuser, uint32 &tfile,float &occ){
	total = list.GetCount();
	failed = 0;
	user = 0;
	file = 0;
	tuser=0;
	tfile = 0;
	occ=0;
	uint32 maxusers=0;
	uint32 tuserk = 0;

	CServer* curr;
	for (POSITION pos = list.GetHeadPosition(); pos !=0;list.GetNext(pos)){
		curr = (CServer*)list.GetAt(pos);
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
	if (maxusers>0) occ=(float)(tuserk*100)/maxusers;
}

void CServerList::GetUserFileStatus(uint32 &user, uint32 &file){
	user = 0;
	file = 0;
	CServer* curr;
	for (POSITION pos = list.GetHeadPosition(); pos !=0;list.GetNext(pos)){
		curr = (CServer*)list.GetAt(pos);
		if( !curr->GetFailedCount() ){
			user += curr->GetUsers();
			file += curr->GetFiles();
		}
	}
}

CServerList::~CServerList(){
	SaveServermetToFile();
	for(POSITION pos = list.GetHeadPosition(); pos != NULL; pos = list.GetHeadPosition()) {
		delete list.GetAt(pos);
		list.RemoveAt(pos);
	}
//	if (udp_timer)
//		KillTimer(0,udp_timer);
}

void CServerList::MoveServerDown(CServer* aServer){
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

void CServerList::Sort(){
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
CServer* CServerList::GetNextServer(){
	CServer* nextserver = 0;
	uint32 i = 0;
	if (serverpos>=((uint32)list.GetCount()) ) return 0;
	while (!nextserver && i < (uint32)list.GetCount()){
		POSITION posIndex = list.FindIndex(serverpos);
		if (posIndex == NULL) {	// check if search position is still valid (could be corrupted by server delete operation)
			posIndex = list.GetHeadPosition();
			serverpos= 0;       //<<--9/27/02 zg
		}

		nextserver = list.GetAt(posIndex);
		serverpos++;
		i++;
		// TODO: Add more option to filter bad server like min ping, min users etc
		//if (nextserver->preferences = ?)
		//	nextserver = 0;
		//if (serverpos == list.GetCount()) return 0;//			serverpos = 0;
	}
	return nextserver;
}

CServer* CServerList::GetNextSearchServer(){
	CServer* nextserver = 0;
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

CServer* CServerList::GetNextStatServer(){
	CServer* nextserver = 0;
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

//bool CServerList::BroadCastPacket(Packet* packet){ // unused atm . but might be useful later
//	if (udp_timer)
//		return false;
//	VERIFY( (udp_timer = SetTimer(0,0,UDPSEARCHSPEED,UDPTimer)) );
//	broadcastpos = list.GetHeadPosition();
//	broadcastpacket = packet;
//	return true;
//}

//void CServerList::SendNextPacket(){
//	if (theApp.listensocket->TooManySockets()){
//		KillTimer(0,udp_timer);
//		udp_timer = 0;
//		delete broadcastpacket;
//		return;
//	}
//
//	if (broadcastpos != 0){
//		CServer* cur_server = list.GetAt(broadcastpos);
//		if (cur_server != theApp.serverconnect->GetCurrentServer())
//			theApp.serverconnect->SendUDPPacket(broadcastpacket,cur_server,false);
//		list.GetNext(broadcastpos);
//	}
//	else{
//		KillTimer(0,udp_timer);
//		udp_timer = 0;
//		delete broadcastpacket;
//	}
//}

//void CServerList::CancelUDPBroadcast(){
//	if (udp_timer){
//		KillTimer(0,udp_timer);
//		udp_timer = 0;
//		delete broadcastpacket;
//	}
//}

//void CALLBACK CServerList::UDPTimer(HWND hwnd, UINT uMsg,UINT_PTR idEvent,DWORD dwTime)
//{
//	// NOTE: Always handle all type of MFC exceptions in TimerProcs - otherwise we'll get mem leaks
//	try{
//		theApp.serverlist->SendNextPacket();
//	}
//	CATCH_DFLT_EXCEPTIONS("CServerList::UDPTimer")
//}

CServer* CServerList::GetNextServer(CServer* lastserver){
	if (list.IsEmpty())
		return 0;
	if (!lastserver) return list.GetHead();

	POSITION pos = list.Find(lastserver);
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

CServer* CServerList::GetServerByAddress(LPCTSTR address, uint16 port){
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
        if (( port == s->GetPort() || port==0) && !strcmp(s->GetAddress(),address)) 
			return s; 
	}
	return NULL;
}

CServer* CServerList::GetServerByIP(uint32 nIP){
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
		if (s->GetIP() == nIP)
			return s; 
	}
	return NULL;
}

CServer* CServerList::GetServerByIP(uint32 nIP, uint16 nPort){
	for (POSITION pos = list.GetHeadPosition();pos != 0;){
        CServer* s = list.GetNext(pos);
		if (s->GetIP() == nIP && s->GetPort() == nPort)
			return s; 
	}
	return NULL;
}

bool CServerList::SaveServermetToFile(){
	DEBUG_ONLY(AddDebugLogLine(false, "Saved Serverlist"));
	m_nLastSaved = ::GetTickCount(); 
	CString newservermet(app_prefs->GetConfigDir());
	newservermet += _T("server.met.new");
	CSafeBufferedFile servermet;
	CFileException fexp;
	if (!servermet.Open(newservermet, CFile::modeWrite|CFile::modeCreate|CFile::typeBinary, &fexp)){
		CString strError(GetResString(IDS_ERR_SAVESERVERMET));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (fexp.GetErrorMessage(szError, ELEMENT_COUNT(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(true, _T("%s"), strError);
		return false;
	}
	setvbuf(servermet.m_pStream, NULL, _IOFBF, 16384);
	
	try{
		version = 0xE0;
		servermet.Write(&version, 1);
		uint32 fservercount = list.GetCount();
		servermet.Write(&fservercount,4);
		ServerMet_Struct sbuffer;
		CServer* nextserver;

		for (uint32 j = 0;j != fservercount;j++){
			nextserver = this->GetServerAt(j);
			sbuffer.ip = nextserver->GetIP();
			sbuffer.port = nextserver->GetPort();
			uint16 tagcount = 11;
			if (nextserver->GetListName() && nextserver->GetListName()[0] != '\0')
				tagcount++;
			if (nextserver->GetDynIP() && nextserver->GetDynIP()[0] != '\0')
				tagcount++;
			if (nextserver->GetDescription() && nextserver->GetDescription()[0] != '\0')
				tagcount++;
			sbuffer.tagcount = tagcount;
			servermet.Write(&sbuffer, sizeof(ServerMet_Struct));
			
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
			CTag version(ST_VERSION, (LPCSTR)nextserver->GetVersion() );
			version.WriteTagToFile(&servermet);
			CTag tagUDPFlags(ST_UDPFLAGS, nextserver->GetUDPFlags() );
			tagUDPFlags.WriteTagToFile(&servermet);
		}
		if (theApp.glob_prefs->GetCommitFiles() >= 2 || (theApp.glob_prefs->GetCommitFiles() >= 1 && !theApp.emuledlg->IsRunning())){
			servermet.Flush(); // flush file stream buffers to disk buffers
			if (_commit(_fileno(servermet.m_pStream)) != 0) // commit disk buffers to disk
				AfxThrowFileException(CFileException::hardIO, GetLastError(), servermet.GetFileName());
		}
		servermet.Close();

		CString curservermet(app_prefs->GetConfigDir());
		CString oldservermet(app_prefs->GetConfigDir());
		curservermet += "server.met";
		oldservermet += "server_met.old";
		
		if (_taccess(oldservermet, 0) == 0)
			CFile::Remove(oldservermet);
		if (_taccess(curservermet, 0) == 0)
			CFile::Rename(curservermet,oldservermet);
		CFile::Rename(newservermet,curservermet);
	}
	catch(CFileException* error) {
		OUTPUT_DEBUG_TRACE();
		CString strError(GetResString(IDS_ERR_SAVESERVERMET2));
		TCHAR szError[MAX_CFEXP_ERRORMSG];
		if (error->GetErrorMessage(szError, ELEMENT_COUNT(szError))){
			strError += _T(" - ");
			strError += szError;
		}
		AddLogLine(false, _T("%s"), strError);
		error->Delete();
		return false;
	}
	return true;
}

void CServerList::AddServersFromTextFile(CString strFilename) {
	CString strLine;
	CStdioFile f;
	if (!f.Open(strFilename, CFile::modeRead | CFile::typeText))
		return;
	while(f.ReadString(strLine)) {
		// format is host:port,priority,Name
		if (strLine.GetLength() < 5)
			continue;
		if (strLine.GetAt(0) == '#' || strLine.GetAt(0) == '/')
			continue;

		// fetch host
		int pos = strLine.Find(':');
		if (pos == -1){
			pos = strLine.Find(','); 
			if (pos == -1) 
			continue;
		}
		CString strHost = strLine.Left(pos);
		strLine = strLine.Mid(pos+1);
		// fetch  port
		pos = strLine.Find(',');
		if (pos == -1)
			continue;
		CString strPort = strLine.Left(pos);
		strLine = strLine.Mid(pos+1);

		// Barry - fetch priority
		pos = strLine.Find(',');
		int priority = SRV_PR_HIGH;
		if (pos == 1)
		{
			CString strPriority = strLine.Left(pos);
			try
			{
				priority = atoi(strPriority.GetBuffer(0));
				if ((priority < 0) || (priority > 2))
					priority = SRV_PR_HIGH;
			} catch (...) {}
			strLine = strLine.Mid(pos+1);
		}
	
		// fetch name
		CString strName = strLine;
		strName.Replace("\r", "");
		strName.Replace("\n", "");

		// create server object and add it to the list
		CServer* nsrv = new CServer(atoi(strPort), strHost.GetBuffer());
		nsrv->SetListName(strName.GetBuffer());
		nsrv->SetIsStaticMember(true);
		// Barry - Was always high
		nsrv->SetPreference(priority); 
		if (!theApp.emuledlg->serverwnd.serverlistctrl.AddServer(nsrv, true))
		{
			delete nsrv;
			CServer* srvexisting = GetServerByAddress(strHost.GetBuffer(), atoi(strPort));
			if (srvexisting) {
				srvexisting->SetListName(strName.GetBuffer());
				srvexisting->SetIsStaticMember(true);
				// Barry - Was always high
				srvexisting->SetPreference(priority); 
				if (theApp.emuledlg->serverwnd) theApp.emuledlg->serverwnd.serverlistctrl.RefreshServer(srvexisting);
			}

		}
	}

	f.Close();
}

void CServerList::Process()
{
	if (::GetTickCount() - m_nLastSaved > MIN2MS(17))
		this->SaveServermetToFile();
}
