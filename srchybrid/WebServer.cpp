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
#include "OtherFunctions.h"
#include <zlib/zlib.h>
#include "SearchDlg.h"
#include "WebServer.h"
#include "ED2KLink.h"
#include "MD5Sum.h"
#include "SearchList.h"
#include <locale.h>
#include "HTRichEditCtrl.h"
#include "KademliaWnd.h"
#include "KadContactListCtrl.h"
#include "KadSearchListCtrl.h"
#include "UploadQueue.h"
#include "DownloadQueue.h"
#include "WebSocket.h"
#include "ServerList.h"
#include "SharedFileList.h"
#include "emuledlg.h"
#include "ServerWnd.h"
#include "Sockets.h"
#include "Server.h"
#include "TransferWnd.h"
#include "PartFile.h"
#include "UpDownClient.h"
#include "StatisticsDlg.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "Kademlia/Net/KademliaUDPListener.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define HTTPInit "Server: eMule\r\nConnection: close\r\nContent-Type: text/html\r\n"
#define HTTPInitGZ "Server: eMule\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Encoding: gzip\r\n"

#define WEB_SERVER_TEMPLATES_VERSION	3

CWebServer::CWebServer(void)
{
	m_Params.bShowUploadQueue = false;

	m_Params.DownloadSort = DOWN_SORT_NAME;
	m_Params.bDownloadSortReverse = false;
	m_Params.ServerSort = SERVER_SORT_NAME;
	m_Params.bServerSortReverse = false;
	m_Params.SharedSort = SHARED_SORT_NAME;
	m_Params.bSharedSortReverse = false;

	m_Params.sLastModified=_T("");
	m_Params.sETag=_T("");
	m_iSearchSortby=3;
	m_bSearchAsc=0;

	m_bServerWorking = false;

}

void CWebServer::ReloadTemplates()
{
	CString sPrevLocale(setlocale(LC_TIME,NULL));
	setlocale(LC_TIME, _T("English"));
	CTime t = GetCurrentTime();
	m_Params.sLastModified = t.FormatGmt("%a, %d %b %Y %H:%M:%S GMT");
	m_Params.sETag = MD5Sum(m_Params.sLastModified).GetHash();
	setlocale(LC_TIME, sPrevLocale);

	CString sFile;
	if (thePrefs.GetTemplate()=="" || thePrefs.GetTemplate().MakeLower()=="emule.tmpl")
		sFile= thePrefs.GetAppDir() + CString("eMule.tmpl");
	else sFile=thePrefs.GetTemplate();

	CStdioFile file;
	if(file.Open(sFile, CFile::modeRead|CFile::typeText))
	{
		CString sAll;
		for(;;)
		{
			CString sLine;
			if(!file.ReadString(sLine))
				break;

			sAll += sLine + "\n";
		}
		file.Close();

		CString sVersion = _LoadTemplate(sAll,"TMPL_VERSION");
		long lVersion = atol(sVersion);
		if(lVersion < WEB_SERVER_TEMPLATES_VERSION)
		{
			if(m_bServerWorking)
				AddLogLine(true,GetResString(IDS_WS_ERR_LOADTEMPLATE),sFile);
		}
		else
		{
			m_Templates.sHeader = _LoadTemplate(sAll,"TMPL_HEADER_KAD");
			m_Templates.sHeaderMetaRefresh = _LoadTemplate(sAll,"TMPL_HEADER_META_REFRESH");
			m_Templates.sHeaderStylesheet = _LoadTemplate(sAll,"TMPL_HEADER_STYLESHEET");
			m_Templates.sFooter = _LoadTemplate(sAll,"TMPL_FOOTER");
			m_Templates.sServerList = _LoadTemplate(sAll,"TMPL_SERVER_LIST");
			m_Templates.sServerLine = _LoadTemplate(sAll,"TMPL_SERVER_LINE");
			m_Templates.sTransferImages = _LoadTemplate(sAll,"TMPL_TRANSFER_IMAGES");
			m_Templates.sTransferList = _LoadTemplate(sAll,"TMPL_TRANSFER_LIST");
			m_Templates.sTransferDownHeader = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_HEADER");
			m_Templates.sTransferDownFooter = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_FOOTER");
			m_Templates.sTransferDownLine = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_LINE");
			m_Templates.sTransferDownLineGood = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_LINE_GOOD");
			m_Templates.sTransferUpHeader = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_HEADER");
			m_Templates.sTransferUpFooter = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_FOOTER");
			m_Templates.sTransferUpLine = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_LINE");
			m_Templates.sTransferUpQueueShow = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_QUEUE_SHOW");
			m_Templates.sTransferUpQueueHide = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_QUEUE_HIDE");
			m_Templates.sTransferUpQueueLine = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_QUEUE_LINE");
			m_Templates.sTransferBadLink = _LoadTemplate(sAll,"TMPL_TRANSFER_BAD_LINK");
			m_Templates.sDownloadLink = _LoadTemplate(sAll,"TMPL_DOWNLOAD_LINK");
			m_Templates.sSharedList = _LoadTemplate(sAll,"TMPL_SHARED_LIST");
			m_Templates.sSharedLine = _LoadTemplate(sAll,"TMPL_SHARED_LINE");
			m_Templates.sSharedLineChanged = _LoadTemplate(sAll,"TMPL_SHARED_LINE_CHANGED");
			m_Templates.sGraphs = _LoadTemplate(sAll,"TMPL_GRAPHS");
			m_Templates.sLog = _LoadTemplate(sAll,"TMPL_LOG");
			m_Templates.sServerInfo = _LoadTemplate(sAll,"TMPL_SERVERINFO");
			m_Templates.sDebugLog = _LoadTemplate(sAll,"TMPL_DEBUGLOG");
			m_Templates.sStats = _LoadTemplate(sAll,"TMPL_STATS");
			m_Templates.sPreferences = _LoadTemplate(sAll,"TMPL_PREFERENCES_KAD");
			m_Templates.sLogin = _LoadTemplate(sAll,"TMPL_LOGIN");
			m_Templates.sConnectedServer = _LoadTemplate(sAll,"TMPL_CONNECTED_SERVER");
			m_Templates.sAddServerBox = _LoadTemplate(sAll,"TMPL_ADDSERVERBOX");
			m_Templates.sWebSearch = _LoadTemplate(sAll,"TMPL_WEBSEARCH");
			m_Templates.sSearch = _LoadTemplate(sAll,"TMPL_SEARCH_KAD");
			m_Templates.iProgressbarWidth=atoi(_LoadTemplate(sAll,"PROGRESSBARWIDTH"));
			m_Templates.sSearchHeader = _LoadTemplate(sAll,"TMPL_SEARCH_RESULT_HEADER");			
			m_Templates.sSearchResultLine = _LoadTemplate(sAll,"TMPL_SEARCH_RESULT_LINE");			
			m_Templates.sProgressbarImgs = _LoadTemplate(sAll,"PROGRESSBARIMGS");
			m_Templates.sProgressbarImgsPercent = _LoadTemplate(sAll,"PROGRESSBARPERCENTIMG");
			m_Templates.sClearCompleted = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_CLEARBUTTON");
			m_Templates.sCatArrow= _LoadTemplate(sAll,"TMPL_CATARROW");
			m_Templates.sBootstrapLine= _LoadTemplate(sAll,"TMPL_BOOTSTRAPLINE");
			m_Templates.sKad= _LoadTemplate(sAll,"TMPL_KADDLG");

			m_Templates.sProgressbarImgsPercent.Replace("[PROGRESSGIFNAME]","%s");
			m_Templates.sProgressbarImgsPercent.Replace("[PROGRESSGIFINTERNAL]","%i");
			m_Templates.sProgressbarImgs.Replace("[PROGRESSGIFNAME]","%s");
			m_Templates.sProgressbarImgs.Replace("[PROGRESSGIFINTERNAL]","%i");			
		}
	}
	else
        if(m_bServerWorking)
			AddLogLine(true,GetResString(IDS_WEB_ERR_CANTLOAD), sFile);

}

CWebServer::~CWebServer(void)
{
	if (m_bServerWorking) StopSockets();
}

CString CWebServer::_LoadTemplate(CString sAll, CString sTemplateName)
{
	CString sRet = "";
	int nStart = sAll.Find("<--" + sTemplateName + "-->");
	int nEnd = sAll.Find("<--" + sTemplateName + "_END-->");
	if(nStart != -1 && nEnd != -1 && nStart<nEnd)
	{
		nStart += sTemplateName.GetLength() + 7;
		sRet = sAll.Mid(nStart, nEnd - nStart - 1);
	} else{
		if (sTemplateName=="TMPL_VERSION")
			AddLogLine(true,GetResString(IDS_WS_ERR_LOADTEMPLATE),sTemplateName);
		if (thePrefs.GetVerbose() && nStart==-1)
			AddDebugLogLine(false,GetResString(IDS_WEB_ERR_CANTLOAD),sTemplateName);
	}

	return sRet;
}

void CWebServer::RestartServer() {	//Cax2 - restarts the server with the new port settings
	StopSockets();
	if (m_bServerWorking)	
		StartSockets(this);
}

void CWebServer::StartServer(void)
{
	if(m_bServerWorking != thePrefs.GetWSIsEnabled())
		m_bServerWorking = thePrefs.GetWSIsEnabled();
	else
		return;

	if (m_bServerWorking) {
		ReloadTemplates();
		StartSockets(this);
	} else StopSockets();

	if(thePrefs.GetWSIsEnabled())
		AddLogLine(false,"%s: %s",_GetPlainResString(IDS_PW_WS), _GetPlainResString(IDS_ENABLED));
	else
		AddLogLine(false,"%s: %s",_GetPlainResString(IDS_PW_WS), _GetPlainResString(IDS_DISABLED));


}

void CWebServer::_RemoveServer(CString sIP, int nPort)
{
	CServer* server=theApp.serverlist->GetServerByAddress(sIP.GetBuffer() ,nPort);
	if (server!=NULL) theApp.emuledlg->SendMessage(WEB_REMOVE_SERVER, (WPARAM)server, NULL);
}

void CWebServer::_SetSharedFilePriority(CString hash, uint8 priority)
{	
	CKnownFile* cur_file;
	uchar fileid[16];
	DecodeBase16(hash.GetBuffer(),hash.GetLength(),fileid);

	cur_file=theApp.sharedfiles->GetFileByID(fileid);
	
	if (cur_file==0) return;

	if(priority >= 0 && priority < 5)
	{
		cur_file->SetAutoUpPriority(false);
		cur_file->SetUpPriority(priority);
	}
	else if(priority == 5)// && cur_file->IsPartFile())
	{
		cur_file->SetAutoUpPriority(true);
		cur_file->UpdateAutoUpPriority(); 
	}
}

void CWebServer::AddStatsLine(UpDown line)
{
	m_Params.PointsForWeb.Add(line);
	if(m_Params.PointsForWeb.GetCount() > WEB_GRAPH_WIDTH)
		m_Params.PointsForWeb.RemoveAt(0);
}

CString CWebServer::_SpecialChars(CString str) 
{
	str.Replace("&","&amp;");
	str.Replace("<","&lt;");
	str.Replace(">","&gt;");
	str.Replace("\"","&quot;");
	return str;
}

void CWebServer::_ConnectToServer(CString sIP, int nPort)
{
	CServer* server=NULL;
	if (!sIP.IsEmpty()) server=theApp.serverlist->GetServerByAddress(sIP.GetBuffer(),nPort);
	theApp.emuledlg->SendMessage(WEB_CONNECT_TO_SERVER, (WPARAM)server, NULL);
}

void CWebServer::ProcessFileReq(ThreadData Data) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if (pThis == NULL) return;

	CString filename=Data.sURL;
	CString contenttype;

	if (filename.Right(4).MakeLower()==".gif") contenttype="Content-Type: image/gif\r\n";
		else if (filename.Right(4).MakeLower()==".jpg"  || filename.Right(5).MakeLower()==".jpeg") contenttype="Content-Type: image/jpg\r\n";
		else if (filename.Right(4).MakeLower()==".bmp") contenttype="Content-Type: image/bmp\r\n";
		else if (filename.Right(4).MakeLower()==".png") contenttype="Content-Type: image/png\r\n";
		//DonQ - additional filetypes
		else if (filename.Right(4).MakeLower()==".ico") contenttype="Content-Type: image/x-icon\r\n";
		else if (filename.Right(4).MakeLower()==".css") contenttype="Content-Type: text/css\r\n";
		else if (filename.Right(3).MakeLower()==".js") contenttype="Content-Type: text/javascript\r\n";
		
	contenttype += "Last-Modified: " + pThis->m_Params.sLastModified + "\r\n" + "ETag: " + pThis->m_Params.sETag + "\r\n";
	
	filename.Replace('/','\\');

	if (filename.GetAt(0)=='\\') filename.Delete(0);
	filename=thePrefs.GetWebServerDir()+filename;
	CFile file;
	if(file.Open(filename, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary))
	{
		char* buffer=new char[file.GetLength()];
		int size=file.Read(buffer,file.GetLength());
		file.Close();
		Data.pSocket->SendContent(contenttype, buffer, size);
		delete[] buffer;
	}
}

void CWebServer::ProcessURL(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return;

	//SetThreadLocale(thePrefs.GetLanguageID());

	//////////////////////////////////////////////////////////////////////////
	// Here we are in real trouble! We are accessing the entire emule main thread
	// data without any syncronization!! Either we use the message pump for emuledlg
	// or use some hundreds of critical sections... For now, an exception handler
	// shoul avoid the worse things.
	//////////////////////////////////////////////////////////////////////////
	CoInitialize(NULL);

	try{
		bool isUseGzip = thePrefs.GetWebUseGzip();
		CString Out = "";
		CString OutE = "";	// List Entry Templates
		CString OutE2 = "";
		CString OutS = "";	// ServerStatus Templates
		TCHAR *gzipOut = NULL;
		long gzipLen=0;
		
		CString HTTPProcessData = "";
		CString HTTPTemp = "";
		//char	HTTPTempC[100] = "";
		srand ( time(NULL) );

		long lSession = 0;
		if(_ParseURL(Data.sURL, "ses") != "")
			lSession = atol(_ParseURL(Data.sURL, "ses"));

		if (_ParseURL(Data.sURL, "w") == "password")
		{
			CString test=MD5Sum(_ParseURL(Data.sURL, "p")).GetHash();
			bool login=false;
			CString ip=inet_ntoa( Data.inadr );

			if(MD5Sum(_ParseURL(Data.sURL, "p")).GetHash() == thePrefs.GetWSPass())
			{
				Session ses;
				ses.admin=true;
				ses.startTime = CTime::GetCurrentTime();
				ses.lSession = lSession = rand() * 10000L + rand();
				pThis->m_Params.Sessions.Add(ses);
				theApp.emuledlg->serverwnd->UpdateMyInfo();
				AddLogLine(true,GetResString(IDS_WEB_ADMINLOGIN)+" (%s)",ip);
				login=true;
			}
			else if(thePrefs.GetWSIsLowUserEnabled() && thePrefs.GetWSLowPass()!="" && MD5Sum(_ParseURL(Data.sURL, "p")).GetHash() == thePrefs.GetWSLowPass())
			{
				Session ses;
				ses.admin=false;
				ses.startTime = CTime::GetCurrentTime();
				ses.lSession = lSession = rand() * 10000L + rand();
				pThis->m_Params.Sessions.Add(ses);
				theApp.emuledlg->serverwnd->UpdateMyInfo();
				AddLogLine(true,GetResString(IDS_WEB_GUESTLOGIN)+" (%s)",ip);
				login=true;
			} else {
				AddLogLine(true,GetResString(IDS_WEB_BADLOGINATTEMPT)+" (%s)",ip);

				BadLogin newban={inet_addr(ip), ::GetTickCount()};	// save failed attempt (ip,time)
				pThis->m_Params.badlogins.Add(newban);
				login=false;
			}
			isUseGzip = false; // [Julien]
			if (login) {
				uint32 ipn=inet_addr( ip) ;
				for(int i = 0; i < pThis->m_Params.badlogins.GetSize();)
				{
					if (ipn == pThis->m_Params.badlogins[i].datalen) {
						pThis->m_Params.badlogins.RemoveAt(i);
					}
					else
						i++;
				}
			}
		}

		CString sSession; sSession.Format("%ld", lSession);

		if (_ParseURL(Data.sURL, "w") == "logout") 
			_RemoveSession(Data, lSession);

		if(_IsLoggedIn(Data, lSession))
		{
			Out += _GetHeader(Data, lSession);
			CString sPage = _ParseURL(Data.sURL, "w");
			if (sPage == "server") 
			{
				Out += _GetServerList(Data);
			}
			else
			if (sPage == "download") 
			{
				Out += _GetDownloadLink(Data);
			}
			if (sPage == "kad") 
			{
				Out += _GetKadPage(Data);
			}
			else
			if (sPage == "shared") 
			{ 
				Out += _GetSharedFilesList(Data);
			}
			else
			if (sPage == "transfer") 
			{
				Out += _GetTransferList(Data);
			}
			else
			if (sPage == "websearch") 
			{
				Out += _GetWebSearch(Data);
			}
			else
			if (sPage == "search") 
			{
				Out += _GetSearch(Data);
			}
			else
			if (sPage == "graphs")
			{
				Out += _GetGraphs(Data);
			}
			else
			if (sPage == "log") 
			{
				Out += _GetLog(Data);
			}
			if (sPage == "sinfo") 
			{
				Out += _GetServerInfo(Data);
			}
			if (sPage == "debuglog") 
			{
				Out += _GetDebugLog(Data);
			}
			if (sPage == "stats") 
			{
				Out += _GetStats(Data);
			}
			if (sPage == "options") 
			{
				Out += _GetPreferences(Data);
			}
			Out += _GetFooter(Data);

			if (sPage == "")
				isUseGzip = false;

			if(isUseGzip)
			{
				bool bOk = false;
				try
				{
					uLongf destLen = Out.GetLength() + 1024;
					gzipOut = new TCHAR[destLen];
					if(_GzipCompress((Bytef*)gzipOut, &destLen, (const Bytef*)(TCHAR*)Out.GetBuffer(0), Out.GetLength(), Z_DEFAULT_COMPRESSION) == Z_OK)
					{
						bOk = true;
						gzipLen = destLen;
					}
				}
				catch(...){
					ASSERT(0);
				}
				if(!bOk)
				{
					isUseGzip = false;
					if(gzipOut != NULL)
					{
						delete[] gzipOut;
						gzipOut = NULL;
					}
				}
			}
		}
		else 
		{
			isUseGzip = false;

			uint32 ip= inet_addr(inet_ntoa( Data.inadr ));
			uint32 faults=0;

			// check for bans
			for(int i = 0; i < pThis->m_Params.badlogins.GetSize();i++)
				if ( pThis->m_Params.badlogins[i].datalen==ip ) faults++;

			if (faults>4) {
				Out += _GetPlainResString(IDS_ACCESSDENIED);
				
				// set 15 mins ban by using the badlist
				BadLogin preventive={ip, ::GetTickCount() + (15*60*1000) };
				for (int i=0;i<=5;i++)
					pThis->m_Params.badlogins.Add(preventive);

			}
			else
				Out += _GetLoginScreen(Data);
		}

		// send answer ...
		if(!isUseGzip)
		{
			Data.pSocket->SendContent(HTTPInit, Out, Out.GetLength());
		}
		else
		{
			Data.pSocket->SendContent(HTTPInitGZ, gzipOut, gzipLen);
		}
		if(gzipOut != NULL)
			delete[] gzipOut;
	}
	catch(...){
		TRACE("*** Unknown exception in CWebServer::ProcessURL\n");
		ASSERT(0);
	}

	CoUninitialize();
}

CString CWebServer::_ParseURLArray(CString URL, CString fieldname) {
	CString res,temp;

	while (URL.GetLength()>0) {
		int pos=URL.MakeLower().Find(fieldname.MakeLower() +"=");
		if (pos>-1) {
			temp=_ParseURL(URL,fieldname);
			if (temp=="") break;
			res.Append(temp+"|");
			URL.Delete(pos,10);
		} else break;
	}
	return res;
}

CString CWebServer::_ParseURL(CString URL, CString fieldname)
{

	CString value = "";
	CString Parameter = "";
	char fromReplace[4] = "";	// decode URL
	char toReplace[2] = "";		// decode URL
	int i = 0;
	int findPos = -1;
	int findLength = 0;

	if (URL.Find("?") > -1) {
		Parameter = URL.Mid(URL.Find("?")+1, URL.GetLength()-URL.Find("?")-1);

		// search the fieldname beginning / middle and strip the rest...
		if (Parameter.Find(fieldname + "=") == 0) {
			findPos = 0;
			findLength = fieldname.GetLength() + 1;
		}
		if (Parameter.Find("&" + fieldname + "=") > -1) {
			findPos = Parameter.Find("&" + fieldname + "=");
			findLength = fieldname.GetLength() + 2;
		}
		if (findPos > -1) {
			Parameter = Parameter.Mid(findPos + findLength, Parameter.GetLength());
			if (Parameter.Find("&") > -1) {
				Parameter = Parameter.Mid(0, Parameter.Find("&"));
			}
	
			value = Parameter;

			// decode value ...
			value.Replace("+", " ");
			for (i = 0 ; i <= 255 ; i++) {
				sprintf(fromReplace, "%%%02x", i);
				toReplace[0] = (char)i;
				toReplace[1] = NULL;
				value.Replace(fromReplace, toReplace);
				sprintf(fromReplace, "%%%02X", i);
				value.Replace(fromReplace, toReplace);
			}
		}
	}

	return value;
}

CString CWebServer::_GetHeader(ThreadData Data, long lSession)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession; sSession.Format("%ld", lSession);

	CString Out = pThis->m_Templates.sHeader;

	Out.Replace("[CharSet]", _GetWebCharSet());

	if(thePrefs.GetWebPageRefresh() != 0)
	{
		CString sPage = _ParseURL(Data.sURL, "w");
		if ((sPage == "transfer") ||
			(sPage == "server") ||
			(sPage == "graphs") ||
			(sPage == "log") ||
			(sPage == "sinfo") ||
			(sPage == "debuglog") ||
			(sPage == "kad") ||
			(sPage == "stats"))
		{
			CString sT = pThis->m_Templates.sHeaderMetaRefresh;
			CString sRefresh; sRefresh.Format("%d", thePrefs.GetWebPageRefresh());
			sT.Replace("[RefreshVal]", sRefresh);

			CString catadd="";
			if (sPage == "transfer")
				catadd="&cat="+_ParseURL(Data.sURL, "cat");
			sT.Replace("[wCommand]", _ParseURL(Data.sURL, "w")+catadd);

			Out.Replace("[HeaderMeta]", sT);
		}
	}
	Out.Replace("[Session]", sSession);
	Out.Replace("[HeaderMeta]", ""); // In case there are no meta
	Out.Replace("[eMuleAppName]", "eMule");
	Out.Replace("[version]", theApp.m_strCurVersionLong);
	Out.Replace("[StyleSheet]", pThis->m_Templates.sHeaderStylesheet);
	Out.Replace("[WebControl]", _GetPlainResString(IDS_WEB_CONTROL));
	Out.Replace("[Transfer]", _GetPlainResString(IDS_CD_TRANS));
	Out.Replace("[Server]", _GetPlainResString(IDS_SV_SERVERLIST));
	Out.Replace("[Shared]", _GetPlainResString(IDS_SHAREDFILES));
	Out.Replace("[Download]", _GetPlainResString(IDS_SW_LINK));
	Out.Replace("[Graphs]", _GetPlainResString(IDS_GRAPHS));
	Out.Replace("[Log]", _GetPlainResString(IDS_SV_LOG));
	Out.Replace("[ServerInfo]", _GetPlainResString(IDS_SV_SERVERINFO));
	Out.Replace("[DebugLog]", _GetPlainResString(IDS_SV_DEBUGLOG));
	Out.Replace("[Stats]", _GetPlainResString(IDS_SF_STATISTICS));
	Out.Replace("[Options]", _GetPlainResString(IDS_EM_PREFS));
	Out.Replace("[Logout]", _GetPlainResString(IDS_WEB_LOGOUT));
	Out.Replace("[Search]", _GetPlainResString(IDS_SW_SEARCHBOX));
	Out.Replace("[Kad]", _GetPlainResString(IDS_KADEMLIA));

	char HTTPTempC[100] = "";
	CString sConnected = "";
	if (theApp.IsConnected()) 
	{
		if(!theApp.IsFirewalled())
			sConnected = _GetPlainResString(IDS_CONNECTED);
		else
			sConnected = _GetPlainResString(IDS_CONNECTED) + " (" + _GetPlainResString(IDS_IDLOW) + ")";

		if( theApp.serverconnect->IsConnected()){
			CServer* cur_server = theApp.serverconnect->GetCurrentServer();
			sConnected += ": " + CString(cur_server->GetListName());
	
			sprintf(HTTPTempC, "%i ", cur_server->GetUsers());
			sConnected += " [" + CString(HTTPTempC) + _GetPlainResString(IDS_LUSERS) + "]";
		}
	} 
	else if (theApp.serverconnect->IsConnecting())
	{
		sConnected = _GetPlainResString(IDS_CONNECTING);
	}
	else
	{
		sConnected = _GetPlainResString(IDS_DISCONNECTED);
		if (IsSessionAdmin(Data,sSession)) sConnected+=" (<small><a href=\"?ses=" + sSession + "&w=server&c=connect\">"+_GetPlainResString(IDS_CONNECTTOANYSERVER)+"</a></small>)";
	}
	Out.Replace("[Connected]", "<b>"+_GetPlainResString(IDS_PW_CONNECTION)+":</b> "+sConnected);
    sprintf(HTTPTempC, _GetPlainResString(IDS_UPDOWNSMALL),(float)theApp.uploadqueue->GetDatarate()/1024,(float)theApp.downloadqueue->GetDatarate()/1024);
	CString buffer;
	// EC 25-12-2003
	CString MaxUpload;
	CString MaxDownload;
	MaxUpload.Format("%i",thePrefs.GetMaxUpload());
	MaxDownload.Format("%i",thePrefs.GetMaxDownload());
	if (MaxUpload == "65535")  MaxUpload = GetResString(IDS_PW_UNLIMITED);
	if (MaxDownload == "65535") MaxDownload = GetResString(IDS_PW_UNLIMITED);
	buffer.Format("%s/%s", MaxUpload, MaxDownload);
	// EC Ends
	Out.Replace("[Speed]", "<b>"+_GetPlainResString(IDS_DL_SPEED)+":</b> "+CString(HTTPTempC) + "<small> (" + _GetPlainResString(IDS_PW_CON_LIMITFRM) + ": " + buffer + ")</small>");

	buffer=GetResString(IDS_KADEMLIA)+": ";

	if (!thePrefs.GetNetworkKademlia()) {
		buffer.Append(GetResString(IDS_DISABLED));
	}
	else if ( !Kademlia::CKademlia::isRunning() ) {

		buffer.Append(GetResString(IDS_DISCONNECTED));

		if (IsSessionAdmin(Data,sSession)) 
			buffer+=" (<small><a href=\"?ses=" + sSession + "&w=kad&c=connect\">"+_GetPlainResString(IDS_MAIN_BTN_CONNECT)+"</a></small>)";
	}
	else if ( Kademlia::CKademlia::isRunning() && !Kademlia::CKademlia::isConnected() ) {

		buffer.Append(GetResString(IDS_CONNECTING));

		if (IsSessionAdmin(Data,sSession)) 
			buffer+=" (<small><a href=\"?ses=" + sSession + "&w=kad&c=disconnect\">"+_GetPlainResString(IDS_MAIN_BTN_DISCONNECT)+"</a></small>)";
	}
	else if (Kademlia::CKademlia::isFirewalled()) {
		buffer.Append(GetResString(IDS_FIREWALLED));
		if (IsSessionAdmin(Data,sSession)) 
			buffer+=" (<small><a href=\"?ses=" + sSession + "&w=kad&c=disconnect\">"+_GetPlainResString(IDS_IRC_DISCONNECT)+"</a></small>)";
	}
	else if (Kademlia::CKademlia::isConnected()) {
		buffer.Append(GetResString(IDS_CONNECTED));
		if (IsSessionAdmin(Data,sSession)) 
			buffer+=" (<small><a href=\"?ses=" + sSession + "&w=kad&c=disconnect\">"+_GetPlainResString(IDS_IRC_DISCONNECT)+"</a></small>)";
	}

	Out.Replace("[KademliaInfo]",buffer);

	return Out;
}

CString CWebServer::_GetFooter(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	return pThis->m_Templates.sFooter;
}

CString CWebServer::_GetServerList(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString sAddServerBox = "";

	CString sCmd = _ParseURL(Data.sURL, "c");
	if (sCmd == "connect" && IsSessionAdmin(Data,sSession) )
	{
		_ConnectToServer(_ParseURL(Data.sURL, "ip"),atoi(_ParseURL(Data.sURL, "port")));
	}	
	else if (sCmd == "disconnect" && IsSessionAdmin(Data,sSession)) 
	{
		theApp.emuledlg->SendMessage(WEB_DISCONNECT_SERVER, NULL);
	}	
	else if (sCmd == "remove" && IsSessionAdmin(Data,sSession)) 
	{
		CString sIP = _ParseURL(Data.sURL, "ip");
		int nPort = atoi(_ParseURL(Data.sURL, "port"));
		if(!sIP.IsEmpty())
			_RemoveServer(sIP,nPort);
	}	
	else if (sCmd == "options") 
	{
		sAddServerBox = _GetAddServerBox(Data);
	}
	CString sSort = _ParseURL(Data.sURL, "sort");
	if (sSort != "") 
	{
		if(sSort == "name")
			pThis->m_Params.ServerSort = SERVER_SORT_NAME;
		else
		if(sSort == "description")
			pThis->m_Params.ServerSort = SERVER_SORT_DESCRIPTION;
		else
		if(sSort == "ip")
			pThis->m_Params.ServerSort = SERVER_SORT_IP;
		else
		if(sSort == "users")
			pThis->m_Params.ServerSort = SERVER_SORT_USERS;
		else
		if(sSort == "files")
			pThis->m_Params.ServerSort = SERVER_SORT_FILES;

		if(_ParseURL(Data.sURL, "sortreverse") == "")
			pThis->m_Params.bServerSortReverse = false;
	}
	if (_ParseURL(Data.sURL, "sortreverse") != "") 
	{
		pThis->m_Params.bServerSortReverse = (_ParseURL(Data.sURL, "sortreverse") == "true");
	}
	CString sServerSortRev;
	if(pThis->m_Params.bServerSortReverse)
		sServerSortRev = "false";
	else
		sServerSortRev = "true";

	CString Out = pThis->m_Templates.sServerList;
	Out.Replace("[ConnectedServerData]", _GetConnectedServer(Data));
    Out.Replace("[AddServerBox]", sAddServerBox);
	Out.Replace("[Session]", sSession);
	if(pThis->m_Params.ServerSort == SERVER_SORT_NAME)
		Out.Replace("[SortName]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortName]", "");
	if(pThis->m_Params.ServerSort == SERVER_SORT_DESCRIPTION)
		Out.Replace("[SortDescription]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortDescription]", "");
	if(pThis->m_Params.ServerSort == SERVER_SORT_IP)
		Out.Replace("[SortIP]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortIP]", "");
	if(pThis->m_Params.ServerSort == SERVER_SORT_USERS)
		Out.Replace("[SortUsers]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortUsers]", "");
	if(pThis->m_Params.ServerSort == SERVER_SORT_FILES)
		Out.Replace("[SortFiles]", "&sortreverse=" + sServerSortRev);
	else
		Out.Replace("[SortFiles]", "");

	CString sortimg;
	if (pThis->m_Params.bServerSortReverse) sortimg="<img src=\"l_up.gif\">";
		else sortimg="<img src=\"l_down.gif\">";

	Out.Replace("[ServerList]", _GetPlainResString(IDS_SV_SERVERLIST));
	Out.Replace("[Servername]", _GetPlainResString(IDS_SL_SERVERNAME)+CString((pThis->m_Params.ServerSort==SERVER_SORT_NAME)?sortimg:""));
	Out.Replace("[Description]", _GetPlainResString(IDS_DESCRIPTION)+CString((pThis->m_Params.ServerSort==SERVER_SORT_DESCRIPTION)?sortimg:""));
	Out.Replace("[Address]", _GetPlainResString(IDS_IP)+CString((pThis->m_Params.ServerSort==SERVER_SORT_IP)?sortimg:""));
	Out.Replace("[Connect]", _GetPlainResString(IDS_IRC_CONNECT));
	Out.Replace("[Users]", _GetPlainResString(IDS_LUSERS)+CString((pThis->m_Params.ServerSort==SERVER_SORT_USERS)?sortimg:""));
	Out.Replace("[Files]", _GetPlainResString(IDS_LFILES)+CString((pThis->m_Params.ServerSort==SERVER_SORT_FILES)?sortimg:""));
	Out.Replace("[Actions]", _GetPlainResString(IDS_WEB_ACTIONS));

	CString OutE = pThis->m_Templates.sServerLine;
	OutE.Replace("[Connect]", _GetPlainResString(IDS_IRC_CONNECT));
	OutE.Replace("[RemoveServer]", _GetPlainResString(IDS_REMOVETHIS));
	OutE.Replace("[ConfirmRemove]", _GetPlainResString(IDS_WEB_CONFIRM_REMOVE_SERVER));

	CArray<ServerEntry, ServerEntry> ServerArray;

	// Populating array
	for (uint32 sc=0;sc<theApp.serverlist->GetServerCount();sc++)
	{
		CServer* cur_file = theApp.serverlist->GetServerAt(sc);
		ServerEntry Entry;
		Entry.sServerName = _SpecialChars(cur_file->GetListName());
		Entry.sServerDescription = _SpecialChars(cur_file->GetDescription());
		Entry.nServerPort = cur_file->GetPort();
		Entry.sServerIP = cur_file->GetAddress();
		Entry.nServerUsers = cur_file->GetUsers();
		Entry.nServerMaxUsers = cur_file->GetMaxUsers();
		Entry.nServerFiles = cur_file->GetFiles();

		ServerArray.Add(Entry);
	}
	// Sorting (simple bubble sort, we don't have tons of data here)
	bool bSorted = true;
	for(int nMax = 0;bSorted && nMax < ServerArray.GetCount()*2; nMax++)
	{
		bSorted = false;
		for(int i = 0; i < ServerArray.GetCount() - 1; i++)
		{
			bool bSwap = false;
			switch(pThis->m_Params.ServerSort)
			{
			case SERVER_SORT_NAME:
				bSwap = ServerArray[i].sServerName.CompareNoCase(ServerArray[i+1].sServerName) > 0;
				break;
			case SERVER_SORT_DESCRIPTION:
				bSwap = ServerArray[i].sServerDescription.CompareNoCase(ServerArray[i+1].sServerDescription) < 0;
				break;
			case SERVER_SORT_IP:
				bSwap = ServerArray[i].sServerIP.CompareNoCase(ServerArray[i+1].sServerIP) > 0;
				break;
			case SERVER_SORT_USERS:
				bSwap = ServerArray[i].nServerUsers < ServerArray[i+1].nServerUsers;
				break;
			case SERVER_SORT_FILES:
				bSwap = ServerArray[i].nServerFiles < ServerArray[i+1].nServerFiles;
				break;
			}
			if(pThis->m_Params.bServerSortReverse)
			{
				bSwap = !bSwap;
			}
			if(bSwap)
			{
				bSorted = true;
				ServerEntry TmpEntry = ServerArray[i];
				ServerArray[i] = ServerArray[i+1];
				ServerArray[i+1] = TmpEntry;
			}
		}
	}
	// Displaying
	CString sList = "";
	for(int i = 0; i < ServerArray.GetCount(); i++)
	{
		CString HTTPProcessData = OutE;	// Copy Entry Line to Temp
		HTTPProcessData.Replace("[1]", ServerArray[i].sServerName);
		HTTPProcessData.Replace("[2]", ServerArray[i].sServerDescription);
		CString sPort; sPort.Format(":%d", ServerArray[i].nServerPort);
		HTTPProcessData.Replace("[3]", ServerArray[i].sServerIP + sPort);

		CString sT;
		if(ServerArray[i].nServerUsers > 0)
		{
			if(ServerArray[i].nServerMaxUsers > 0)
				sT.Format("%d (%d)", ServerArray[i].nServerUsers, ServerArray[i].nServerMaxUsers);
			else
				sT.Format("%d", ServerArray[i].nServerUsers);
		}
		HTTPProcessData.Replace("[4]", sT);
		sT = "";
		if(ServerArray[i].nServerFiles > 0)
			sT.Format("%d", ServerArray[i].nServerFiles);
		HTTPProcessData.Replace("[5]", sT);

		CString sServerPort; sServerPort.Format("%d", ServerArray[i].nServerPort);

		HTTPProcessData.Replace("[6]", IsSessionAdmin(Data,sSession)? CString("?ses=" + sSession + "&w=server&c=connect&ip=" + ServerArray[i].sServerIP+"&port="+sServerPort):GetPermissionDenied());
		HTTPProcessData.Replace("[LinkRemove]", IsSessionAdmin(Data,sSession)?CString("?ses=" + sSession + "&w=server&c=remove&ip=" + ServerArray[i].sServerIP+"&port="+sServerPort):GetPermissionDenied());

		sList += HTTPProcessData;
	}
	Out.Replace("[ServersList]", sList);

	return Out;
}

CString CWebServer::_GetTransferList(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");
	int cat=atoi(_ParseURL(Data.sURL,"cat"));

	bool clcompl=(_ParseURL(Data.sURL,"ClCompl")=="yes" );
	CString sCat; (cat!=0)?sCat.Format("&cat=%i",cat):sCat="";

	CString Out = "";

	if (clcompl && IsSessionAdmin(Data,sSession)) 
		theApp.emuledlg->transferwnd->downloadlistctrl.ClearCompleted(true);

	if (_ParseURL(Data.sURL, "c") != "" && IsSessionAdmin(Data,sSession)) 
	{
		CString HTTPTemp = _ParseURL(Data.sURL, "c");
		theApp.emuledlg->searchwnd->AddEd2kLinksToDownload(HTTPTemp,cat);
	}

	if (_ParseURL(Data.sURL, "op") != "" &&
		_ParseURL(Data.sURL, "file") != "")
	{
		uchar FileHash[16];
		_GetFileHash(_ParseURL(Data.sURL, "file"), FileHash);

		CPartFile* found_file = NULL;
		for (int fx=0;fx<theApp.downloadqueue->GetFileCount();fx++)
		{
			CPartFile* cur_file =  theApp.downloadqueue->GetFileByIndex(fx);

			bool bGood = true;
			for(int i = 0; i < 16; i++)
			{
				if(cur_file->GetFileHash()[i] != FileHash[i])
				{
					bGood = false;
					break;
				}
			}
			if(bGood)
			{
				found_file = cur_file;
				break;
			}
		}

		if(_ParseURL(Data.sURL, "op") == "pause" && IsSessionAdmin(Data,sSession))
		{
			if(found_file)
				found_file->PauseFile();
		}
		else if(_ParseURL(Data.sURL, "op") == "resume" && IsSessionAdmin(Data,sSession))
		{
			if(found_file)
				found_file->ResumeFile();
		}
		else if(_ParseURL(Data.sURL, "op") == "cancel" && IsSessionAdmin(Data,sSession))
		{
			if(found_file)
				found_file->DeleteFile();
		}
		else if(_ParseURL(Data.sURL, "op") == "prioup" && IsSessionAdmin(Data,sSession))
		{
			if(found_file) {
			  if (!found_file->IsAutoDownPriority())
				switch (found_file->GetDownPriority()) {
					case PR_LOW: found_file->SetAutoDownPriority(false); found_file->SetDownPriority(PR_NORMAL);break;
					case PR_NORMAL: found_file->SetAutoDownPriority(false); found_file->SetDownPriority(PR_HIGH);break;
					case PR_HIGH: found_file->SetAutoDownPriority(true); found_file->SetDownPriority(PR_HIGH);break;
				}
			  else {found_file->SetAutoDownPriority(false); found_file->SetDownPriority(PR_LOW);}
			}
		}
		else if(_ParseURL(Data.sURL, "op") == "priodown" && IsSessionAdmin(Data,sSession))
		{
			if(found_file) {
			  if (!found_file->IsAutoDownPriority())
				switch (found_file->GetDownPriority()) {
					case PR_LOW: found_file->SetAutoDownPriority(true); found_file->SetDownPriority(PR_HIGH);break;
					case PR_NORMAL: found_file->SetAutoDownPriority(false); found_file->SetDownPriority(PR_LOW);break;
					case PR_HIGH: found_file->SetAutoDownPriority(false); found_file->SetDownPriority(PR_NORMAL);break;
				}
			  else {found_file->SetAutoDownPriority(false); found_file->SetDownPriority(PR_HIGH);}
			}
		}
	}
	if (_ParseURL(Data.sURL, "sort") != "") 
	{
		if(_ParseURL(Data.sURL, "sort") == "name")
			pThis->m_Params.DownloadSort = DOWN_SORT_NAME;
		else
		if(_ParseURL(Data.sURL, "sort") == "size")
			pThis->m_Params.DownloadSort = DOWN_SORT_SIZE;
		else
		if(_ParseURL(Data.sURL, "sort") == "transferred")
			pThis->m_Params.DownloadSort = DOWN_SORT_TRANSFERRED;
		else
		if(_ParseURL(Data.sURL, "sort") == "speed")
			pThis->m_Params.DownloadSort = DOWN_SORT_SPEED;
		else
		if(_ParseURL(Data.sURL, "sort") == "progress")
			pThis->m_Params.DownloadSort = DOWN_SORT_PROGRESS;

		if(_ParseURL(Data.sURL, "sortreverse") == "")
			pThis->m_Params.bDownloadSortReverse = false;
	}
	if (_ParseURL(Data.sURL, "sortreverse") != "") 
	{
		pThis->m_Params.bDownloadSortReverse = (_ParseURL(Data.sURL, "sortreverse") == "true");
	}
	if(_ParseURL(Data.sURL, "showuploadqueue") == "true")
	{
		pThis->m_Params.bShowUploadQueue = true;
	}
	if(_ParseURL(Data.sURL, "showuploadqueue") == "false")
	{
		pThis->m_Params.bShowUploadQueue = false;
	}
	CString sDownloadSortRev;
	if(pThis->m_Params.bDownloadSortReverse)
		sDownloadSortRev = "false";
	else
		sDownloadSortRev = "true";

	Out += pThis->m_Templates.sTransferImages;
	Out += pThis->m_Templates.sTransferList;
	Out.Replace("[DownloadHeader]", pThis->m_Templates.sTransferDownHeader);
	Out.Replace("[DownloadFooter]", pThis->m_Templates.sTransferDownFooter);
	Out.Replace("[UploadHeader]", pThis->m_Templates.sTransferUpHeader);
	Out.Replace("[UploadFooter]", pThis->m_Templates.sTransferUpFooter);
	Out.Replace("[Session]", sSession);

	InsertCatBox(Out,cat,"",true,true);

	if(pThis->m_Params.DownloadSort == DOWN_SORT_NAME)
		Out.Replace("[SortName]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortName]", "");
	if(pThis->m_Params.DownloadSort == DOWN_SORT_SIZE)
		Out.Replace("[SortSize]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortSize]", "");
	if(pThis->m_Params.DownloadSort == DOWN_SORT_TRANSFERRED)
		Out.Replace("[SortTransferred]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortTransferred]", "");
	if(pThis->m_Params.DownloadSort == DOWN_SORT_SPEED)
		Out.Replace("[SortSpeed]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortSpeed]", "");
	if(pThis->m_Params.DownloadSort == DOWN_SORT_PROGRESS)
		Out.Replace("[SortProgress]", "&sortreverse=" + sDownloadSortRev);
	else
		Out.Replace("[SortProgress]", "");

	CString sortimg;
	if (pThis->m_Params.bDownloadSortReverse) sortimg="<img src=\"l_up.gif\">";
		else sortimg="<img src=\"l_down.gif\">";

	Out.Replace("[Filename]", _GetPlainResString(IDS_DL_FILENAME)+CString((pThis->m_Params.DownloadSort == DOWN_SORT_NAME)?sortimg:""));
	Out.Replace("[Size]", _GetPlainResString(IDS_DL_SIZE)+CString((pThis->m_Params.DownloadSort == DOWN_SORT_SIZE)?sortimg:""));
	Out.Replace("[Transferred]", _GetPlainResString(IDS_COMPLETE)+CString((pThis->m_Params.DownloadSort == DOWN_SORT_TRANSFERRED)?sortimg:""));
	Out.Replace("[Progress]", _GetPlainResString(IDS_DL_PROGRESS)+CString((pThis->m_Params.DownloadSort == DOWN_SORT_PROGRESS)?sortimg:""));
	Out.Replace("[Speed]", _GetPlainResString(IDS_DL_SPEED)+CString((pThis->m_Params.DownloadSort == DOWN_SORT_SPEED)?sortimg:""));
	Out.Replace("[Sources]", _GetPlainResString(IDS_DL_SOURCES));
	Out.Replace("[Actions]", _GetPlainResString(IDS_WEB_ACTIONS));
	Out.Replace("[User]", _GetPlainResString(IDS_QL_USERNAME));
	Out.Replace("[TotalDown]", _GetPlainResString(IDS_INFLST_USER_TOTALDOWNLOAD));
	Out.Replace("[TotalUp]", _GetPlainResString(IDS_INFLST_USER_TOTALUPLOAD));
	Out.Replace("[Prio]", _GetPlainResString(IDS_PRIORITY));
	Out.Replace("[CatSel]",sCat);
	CString OutE = pThis->m_Templates.sTransferDownLine;
	CString OutE2 = pThis->m_Templates.sTransferDownLineGood;

	float fTotalSize = 0, fTotalTransferred = 0, fTotalSpeed = 0;
	CArray<DownloadFiles, DownloadFiles> FilesArray;

	CArray<CPartFile*,CPartFile*> partlist;
	theApp.emuledlg->transferwnd->downloadlistctrl.GetDisplayedFiles(&partlist);

	// Populating array
	bool completedAv=false;
	for (int i=0;i<partlist.GetCount();i++) {
		
		CPartFile* cur_file=partlist.GetAt(i);
		if (cur_file) {

			DownloadFiles dFile;
			dFile.sFileName = _SpecialChars(cur_file->GetFileName());
			dFile.lFileSize = cur_file->GetFileSize();
			dFile.lFileTransferred = cur_file->GetCompletedSize();
			dFile.fCompleted = cur_file->GetPercentCompleted();
			dFile.lFileSpeed = cur_file->GetDatarate();
			dFile.nFileStatus = cur_file->GetStatus();
			dFile.sFileStatus = cur_file->getPartfileStatus();
			dFile.nFilePrio = cur_file->GetDownPriority();
			if (cur_file->IsAutoDownPriority()) dFile.nFilePrio+=10;
			dFile.sFileHash = EncodeBase16(cur_file->GetFileHash(), 16);
			dFile.lSourceCount = cur_file->GetSourceCount();
			dFile.lNotCurrentSourceCount = cur_file->GetNotCurrentSourcesCount();
			dFile.lTransferringSourceCount = cur_file->GetTransferingSrcCount();
			if (theApp.IsConnected() && !theApp.IsFirewalled())
				dFile.sED2kLink = theApp.CreateED2kSourceLink(cur_file);
			else
				dFile.sED2kLink = CreateED2kLink(cur_file);
			dFile.sFileInfo = _SpecialChars(cur_file->GetInfoSummary(cur_file));

			if (cat>0 && cur_file->GetCategory()!=cat) continue;
			if (cat<0) {
				switch (cat) {
					case -1 : if (cur_file->GetCategory()!=0) continue; break;
					case -2 : if (!cur_file->IsPartFile()) continue; break;
					case -3 : if (cur_file->IsPartFile()) continue; break;
					case -4 : if (!((cur_file->GetStatus()==PS_READY|| cur_file->GetStatus()==PS_EMPTY) && cur_file->GetTransferingSrcCount()==0)) continue; break;
					case -5 : if (!((cur_file->GetStatus()==PS_READY|| cur_file->GetStatus()==PS_EMPTY) && cur_file->GetTransferingSrcCount()>0)) continue; break;
					case -6 : if (cur_file->GetStatus()!=PS_ERROR) continue; break;
					case -7 : if (cur_file->GetStatus()!=PS_PAUSED) continue; break;
					case -8 : if (!cur_file->IsStopped()) continue; break;
					case -9 : if (!cur_file->IsMovie()) continue; break;
					case -10 : if (ED2KFT_AUDIO != GetED2KFileTypeID(cur_file->GetFileName())) continue; break;
					case -11 : if (!cur_file->IsArchive()) continue; break;
					case -12 : if (ED2KFT_CDIMAGE != GetED2KFileTypeID(cur_file->GetFileName())) continue; break;
				}
			}
			FilesArray.Add(dFile);
			if (!cur_file->IsPartFile()) completedAv=true;
		}
	}
	// Sorting (simple bubble sort, we don't have tons of data here)
	bool bSorted = true;
	for(int nMax = 0;bSorted && nMax < FilesArray.GetCount()*2; nMax++)
	{
		bSorted = false;
		for(int i = 0; i < FilesArray.GetCount() - 1; i++)
		{
			bool bSwap = false;
			switch(pThis->m_Params.DownloadSort)
			{
				case DOWN_SORT_NAME:
					bSwap = FilesArray[i].sFileName.CompareNoCase(FilesArray[i+1].sFileName) > 0;
					break;
				case DOWN_SORT_SIZE:
					bSwap = FilesArray[i].lFileSize < FilesArray[i+1].lFileSize;
					break;
				case DOWN_SORT_TRANSFERRED:
					bSwap = FilesArray[i].lFileTransferred < FilesArray[i+1].lFileTransferred;
					break;
				case DOWN_SORT_SPEED:
					bSwap = FilesArray[i].lFileSpeed < FilesArray[i+1].lFileSpeed;
					break;
				case DOWN_SORT_PROGRESS:
						bSwap = FilesArray[i].fCompleted  < FilesArray[i+1].fCompleted ;
						break;
			}
			if(pThis->m_Params.bDownloadSortReverse)
			{
				bSwap = !bSwap;
			}
			if(bSwap)
			{
				bSorted = true;
				DownloadFiles TmpFile = FilesArray[i];
				FilesArray[i] = FilesArray[i+1];
				FilesArray[i+1] = TmpFile;
			}
		}
	}

	// Displaying
	CString sDownList = "";
	CString HTTPTemp;

	for(int i = 0; i < FilesArray.GetCount(); i++)
	{
		CString JSfileinfo=FilesArray[i].sFileInfo;
		JSfileinfo.Replace("\n","\\n");
		CString sActions = "<acronym title=\"" + FilesArray[i].sFileStatus + "\"><a href=\"javascript:alert(\'"+ JSfileinfo+"')\"><img src=\"l_info.gif\" alt=\"" + FilesArray[i].sFileStatus + "\"></a></acronym> ";

		CString sED2kLink;
		sED2kLink.Format("<acronym title=\"[Ed2klink]\"><a href=\""+ FilesArray[i].sED2kLink +"\"><img src=\"l_ed2klink.gif\" alt=\"[Ed2klink]\"></a></acronym> ");
		sED2kLink.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
		sActions += sED2kLink;

		bool bCanBeDeleted = true;
		switch(FilesArray[i].nFileStatus)
		{
			case PS_COMPLETING:
			case PS_COMPLETE:
				bCanBeDeleted = false;
				break;
			case PS_HASHING: 
			case PS_WAITINGFORHASH:
			case PS_ERROR:
				break;
			case PS_PAUSED:
			{
				if (IsSessionAdmin(Data,sSession)) {
					CString sResume="";
					sResume.Format("<acronym title=\"[Resume]\"><a href=\"[Link]\"><img src=\"l_resume.gif\" alt=\"[Resume]\"></a></acronym> ");
					sResume.Replace("[Link]", CString("?ses=" + sSession + "&w=transfer&op=resume&file=" + FilesArray[i].sFileHash +sCat )) ;
					sActions += sResume;
				}
			}
				break; 
			default: // waiting or downloading
			{
				if (IsSessionAdmin(Data,sSession)) {
					CString sPause;
					sPause.Format("<acronym title=\"[Pause]\"><a href=\"[Link]\"><img src=\"l_pause.gif\" alt=\"[Pause]\"></a></acronym> ");
					sPause.Replace("[Link]", CString("?ses=" + sSession + "&w=transfer&op=pause&file=" + FilesArray[i].sFileHash+sCat ));
					sActions += sPause;
				}
			}
			break;
		}
		if(bCanBeDeleted)
		{
			if (IsSessionAdmin(Data,sSession)) {
				CString sCancel;
				sCancel.Format("<acronym title=\"[Cancel]\"><a href=\"[Link]\" onclick=\"return confirm(\'[ConfirmCancel]\')\"><img src=\"l_cancel.gif\" alt=\"[Cancel]\"></a></acronym> ");
				sCancel.Replace("[Link]", CString("?ses=" + sSession + "&w=transfer&op=cancel&file=" + FilesArray[i].sFileHash+sCat ) );
				sActions += sCancel;
			}
		}

		if (IsSessionAdmin(Data,sSession)) {
			sActions.Replace("[Resume]", _GetPlainResString(IDS_DL_RESUME));
			sActions.Replace("[Pause]", _GetPlainResString(IDS_DL_PAUSE));
			sActions.Replace("[Cancel]", _GetPlainResString(IDS_MAIN_BTN_CANCEL));
			sActions.Replace("[ConfirmCancel]", _GetPlainResStringNoQuote(IDS_Q_CANCELDL2));
			
			if (FilesArray[i].nFileStatus!=PS_COMPLETE && FilesArray[i].nFileStatus!=PS_COMPLETING) {
				sActions.Append("<acronym title=\"[PriorityUp]\"><a href=\"?ses=[Session]&amp;w=transfer&op=prioup&file=" + FilesArray[i].sFileHash+sCat+"\"><img src=\"l_up.gif\" alt=\"[PriorityUp]\"></a></acronym>");
				sActions.Append("&nbsp;<acronym title=\"[PriorityDown]\"><a href=\"?ses=[Session]&amp;w=transfer&op=priodown&file=" + FilesArray[i].sFileHash+sCat +"\"><img src=\"l_down.gif\" alt=\"[PriorityDown]\"></a></acronym>");
			}
		}

		CString HTTPProcessData;
		// if downloading, draw in other color
		if(FilesArray[i].lFileSpeed > 0)
			HTTPProcessData = OutE2;
		else
			HTTPProcessData = OutE;

		if(FilesArray[i].sFileName.GetLength() > SHORT_FILENAME_LENGTH)
			HTTPProcessData.Replace("[ShortFileName]", FilesArray[i].sFileName.Left(SHORT_FILENAME_LENGTH) + "...");
		else
			HTTPProcessData.Replace("[ShortFileName]", FilesArray[i].sFileName);

		HTTPProcessData.Replace("[FileInfo]", FilesArray[i].sFileInfo);

		fTotalSize += FilesArray[i].lFileSize;

		HTTPProcessData.Replace("[2]", CastItoXBytes(FilesArray[i].lFileSize));

		if(FilesArray[i].lFileTransferred > 0)
		{
			fTotalTransferred += FilesArray[i].lFileTransferred;

			HTTPProcessData.Replace("[3]", CastItoXBytes(FilesArray[i].lFileTransferred));
		}
		else
			HTTPProcessData.Replace("[3]", "-");

		HTTPProcessData.Replace("[DownloadBar]", _GetDownloadGraph(Data,FilesArray[i].sFileHash));

		if(FilesArray[i].lFileSpeed > 0.0f)
		{
			fTotalSpeed += FilesArray[i].lFileSpeed;

			HTTPTemp.Format("%8.2f %s", FilesArray[i].lFileSpeed/1024.0 ,_GetPlainResString(IDS_KBYTESEC) );
			HTTPProcessData.Replace("[4]", HTTPTemp);
		}
		else
			HTTPProcessData.Replace("[4]", "-");
		if(FilesArray[i].lSourceCount > 0)
		{
			HTTPTemp.Format("%i&nbsp;/&nbsp;%8i&nbsp;(%i)",
				FilesArray[i].lSourceCount-FilesArray[i].lNotCurrentSourceCount,
				FilesArray[i].lSourceCount,
				FilesArray[i].lTransferringSourceCount);

		HTTPProcessData.Replace("[5]", HTTPTemp);
		}
		else
			HTTPProcessData.Replace("[5]", "-");

		switch(FilesArray[i].nFilePrio) {
			case 0: HTTPTemp=GetResString(IDS_PRIOLOW);break;
			case 10: HTTPTemp=GetResString(IDS_PRIOAUTOLOW);break;

			case 1: HTTPTemp=GetResString(IDS_PRIONORMAL);break;
			case 11: HTTPTemp=GetResString(IDS_PRIOAUTONORMAL);break;

			case 2: HTTPTemp=GetResString(IDS_PRIOHIGH);break;
			case 12: HTTPTemp=GetResString(IDS_PRIOAUTOHIGH);break;
		}

		HTTPProcessData.Replace("[PrioVal]", HTTPTemp);
		HTTPProcessData.Replace("[6]", sActions);

		sDownList += HTTPProcessData;
	}

	Out.Replace("[DownloadFilesList]", sDownList);
    Out.Replace("[PriorityUp]", _GetPlainResString(IDS_PRIORITY_UP));
    Out.Replace("[PriorityDown]", _GetPlainResString(IDS_PRIORITY_DOWN));
	// Elandal: cast from float to integral type always drops fractions.
	// avoids implicit cast warning
	Out.Replace("[TotalDownSize]", CastItoXBytes((uint64)fTotalSize));
	Out.Replace("[TotalDownTransferred]", CastItoXBytes((uint64)fTotalTransferred));

	Out.Replace("[ClearCompletedButton]",(completedAv && IsSessionAdmin(Data,sSession))?pThis->m_Templates.sClearCompleted :"");

	HTTPTemp.Format("%8.2f %s", fTotalSpeed/1024.0,_GetPlainResString(IDS_KBYTESEC));
	Out.Replace("[TotalDownSpeed]", HTTPTemp);
	OutE = pThis->m_Templates.sTransferUpLine;
	
	HTTPTemp.Format("%i",pThis->m_Templates.iProgressbarWidth);
	Out.Replace("[PROGRESSBARWIDTHVAL]",HTTPTemp);

	fTotalSize = 0;
	fTotalTransferred = 0;
	fTotalSpeed = 0;

	CString sUpList = "";
	
	for (POSITION pos = theApp.uploadqueue->GetFirstFromUploadList();
		pos != 0;theApp.uploadqueue->GetNextFromUploadList(pos))
	{
		CUpDownClient* cur_client = theApp.uploadqueue->GetQueueClientAt(pos);
		CString HTTPProcessData = OutE;
		HTTPProcessData.Replace("[1]", _SpecialChars(cur_client->GetUserName()));
		HTTPProcessData.Replace("[FileInfo]", _SpecialChars(GetUploadFileInfo(cur_client)));

		CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID());
		if (file)
			HTTPProcessData.Replace("[2]", _SpecialChars(CString(file->GetFileName())));
		else
			HTTPProcessData.Replace("[2]", _GetPlainResString(IDS_REQ_UNKNOWNFILE));

		fTotalSize += cur_client->GetTransferedDown();
		fTotalTransferred += cur_client->GetTransferedUp();
		CString HTTPTemp;
		HTTPTemp.Format("%s / %s", CastItoXBytes(cur_client->GetTransferedDown()),CastItoXBytes(cur_client->GetTransferedUp()));
		HTTPProcessData.Replace("[3]", HTTPTemp);

		fTotalSpeed += cur_client->GetDatarate();
		HTTPTemp.Format("%8.2f " + _GetPlainResString(IDS_KBYTESEC), cur_client->GetDatarate()/1024.0);
		HTTPProcessData.Replace("[4]", HTTPTemp);

		sUpList += HTTPProcessData;
	}
	Out.Replace("[UploadFilesList]", sUpList);
	// Elandal: cast from float to integral type always drops fractions.
	// avoids implicit cast warning
	HTTPTemp.Format("%s / %s", CastItoXBytes((uint64)fTotalSize), CastItoXBytes((uint64)fTotalTransferred));
	Out.Replace("[TotalUpTransferred]", HTTPTemp);
	HTTPTemp.Format("%8.2f " + _GetPlainResString(IDS_KBYTESEC), fTotalSpeed/1024.0);
	Out.Replace("[TotalUpSpeed]", HTTPTemp);

	if(pThis->m_Params.bShowUploadQueue) 
	{
		Out.Replace("[UploadQueue]", pThis->m_Templates.sTransferUpQueueShow);
		Out.Replace("[UploadQueueList]", _GetPlainResString(IDS_ONQUEUE));
		Out.Replace("[UserNameTitle]", _GetPlainResString(IDS_QL_USERNAME));
		Out.Replace("[FileNameTitle]", _GetPlainResString(IDS_DL_FILENAME));
		Out.Replace("[ScoreTitle]", _GetPlainResString(IDS_SCORE));
		Out.Replace("[BannedTitle]", _GetPlainResString(IDS_BANNED));

		OutE = pThis->m_Templates.sTransferUpQueueLine;
		// Replace [xx]
		CString sQueue = "";

		for (POSITION pos = theApp.uploadqueue->GetFirstFromWaitingList(); pos != 0;theApp.uploadqueue->GetNextFromWaitingList(pos)){
			CUpDownClient* cur_client = theApp.uploadqueue->GetWaitClientAt(pos);
			CString HTTPProcessData;
            char HTTPTempC[100] = "";
			HTTPProcessData = OutE;
			HTTPProcessData.Replace("[UserName]", _SpecialChars(cur_client->GetUserName()));
			if (!cur_client->reqfile) continue;
			CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->reqfile->GetFileHash());
			if (file)
				HTTPProcessData.Replace("[FileName]", _SpecialChars(cur_client->reqfile->GetFileName()));
			else
				HTTPProcessData.Replace("[FileName]", "?");
			sprintf(HTTPTempC, "%i" , cur_client->GetScore(false));
			CString HTTPTemp = HTTPTempC;
			HTTPProcessData.Replace("[Score]", HTTPTemp);
			if (cur_client->IsBanned())
				HTTPProcessData.Replace("[Banned]", _GetPlainResString(IDS_YES));
			else
				HTTPProcessData.Replace("[Banned]", _GetPlainResString(IDS_NO));
			sQueue += HTTPProcessData;
		}
		Out.Replace("[QueueList]", sQueue);
	}
	else {
		Out.Replace("[UploadQueue]", pThis->m_Templates.sTransferUpQueueHide);
	}
	Out.Replace("[ShowQueue]", _GetPlainResString(IDS_WEB_SHOW_UPLOAD_QUEUE));
	Out.Replace("[HideQueue]", _GetPlainResString(IDS_WEB_HIDE_UPLOAD_QUEUE));
	Out.Replace("[Session]", sSession);
	Out.Replace("[CLEARCOMPLETED]",_GetPlainResString(IDS_DL_CLEAR));

	CString buffer;
	buffer.Format("%s (%i)", _GetPlainResString(IDS_TW_DOWNLOADS),FilesArray.GetCount());
	Out.Replace("[DownloadList]",buffer);
	buffer.Format("%s (%i)",_GetPlainResString(IDS_PW_CON_UPLBL),theApp.uploadqueue->GetUploadQueueLength());
	Out.Replace("[UploadList]", buffer );
	Out.Replace("[CatSel]",sCat);

	return Out;
}

CString CWebServer::_GetDownloadLink(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	if (!IsSessionAdmin(Data,sSession)) {
		CString ad="<br><br><div align=\"center\" class=\"message\">[Message]</div>";
		ad.Replace("[Message]",_GetPlainResString(IDS_ACCESSDENIED));
		return ad;
	}

	CString Out = pThis->m_Templates.sDownloadLink;

	Out.Replace("[Download]", _GetPlainResString(IDS_SW_DOWNLOAD));
	Out.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
	Out.Replace("[Start]", _GetPlainResString(IDS_SW_START));
	Out.Replace("[Session]", sSession);

	if (thePrefs.GetCatCount()>1)
		InsertCatBox(Out,0, pThis->m_Templates.sCatArrow );
	else Out.Replace("[CATBOX]","");

	return Out;
}

CString CWebServer::_GetSharedFilesList(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
        CString sSession = _ParseURL(Data.sURL, "ses");
	if(pThis == NULL)
		return "";
	if (_ParseURL(Data.sURL, "sort") != "") 
	{
		if(_ParseURL(Data.sURL, "sort") == "name")
			pThis->m_Params.SharedSort = SHARED_SORT_NAME;
		else
		if(_ParseURL(Data.sURL, "sort") == "size")
			pThis->m_Params.SharedSort = SHARED_SORT_SIZE;
		else
		if(_ParseURL(Data.sURL, "sort") == "transferred")
			pThis->m_Params.SharedSort = SHARED_SORT_TRANSFERRED;
		else
		if(_ParseURL(Data.sURL, "sort") == "alltimetransferred")
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_TRANSFERRED;
		else
		if(_ParseURL(Data.sURL, "sort") == "requests")
			pThis->m_Params.SharedSort = SHARED_SORT_REQUESTS;
		else
		if(_ParseURL(Data.sURL, "sort") == "alltimerequests")
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_REQUESTS;
		else
		if(_ParseURL(Data.sURL, "sort") == "accepts")
			pThis->m_Params.SharedSort = SHARED_SORT_ACCEPTS;
		else
		if(_ParseURL(Data.sURL, "sort") == "alltimeaccepts")
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_ACCEPTS;
		else
		if(_ParseURL(Data.sURL, "sort") == "priority")
			pThis->m_Params.SharedSort = SHARED_SORT_PRIORITY;

		if(_ParseURL(Data.sURL, "sortreverse") == "")
			pThis->m_Params.bSharedSortReverse = false;
	}
	if (_ParseURL(Data.sURL, "sortreverse") != "") 
		pThis->m_Params.bSharedSortReverse = (_ParseURL(Data.sURL, "sortreverse") == "true");

	if(_ParseURL(Data.sURL, "hash") != "" && _ParseURL(Data.sURL, "setpriority") != "" && IsSessionAdmin(Data,sSession)) 
		_SetSharedFilePriority(_ParseURL(Data.sURL, "hash"),atoi(_ParseURL(Data.sURL, "setpriority")));

	if(_ParseURL(Data.sURL, "reload") == "true")
	{
		theApp.emuledlg->SendMessage(WEB_SHARED_FILES_RELOAD);
	}

	CString sSharedSortRev;
	if(pThis->m_Params.bSharedSortReverse)
		sSharedSortRev = "false";
	else
		sSharedSortRev = "true";

	CString sortimg;
	if (pThis->m_Params.bSharedSortReverse) sortimg="<img src=\"l_up.gif\">";
		else sortimg="<img src=\"l_down.gif\">";


    //Name sorting link
	CString Out = pThis->m_Templates.sSharedList;
	if(pThis->m_Params.SharedSort == SHARED_SORT_NAME)
		Out.Replace("[SortName]", "sort=name&sortreverse=" + sSharedSortRev);
	else
		Out.Replace("[SortName]", "sort=name");
	//Size sorting Link
    if(pThis->m_Params.SharedSort == SHARED_SORT_SIZE)
		Out.Replace("[SortSize]", "sort=size&sortreverse=" + sSharedSortRev);
	else
		Out.Replace("[SortSize]", "sort=size");
	//Priority sorting Link
    if(pThis->m_Params.SharedSort == SHARED_SORT_PRIORITY)
		Out.Replace("[SortPriority]", "sort=priority&sortreverse=" + sSharedSortRev);
	else
		Out.Replace("[SortPriority]", "sort=priority");
    //Transferred sorting link
	if(pThis->m_Params.SharedSort == SHARED_SORT_TRANSFERRED)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortTransferred]", "sort=alltimetransferred&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortTransferred]", "sort=transferred&sortreverse=" + sSharedSortRev);
	}
	else
	if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_TRANSFERRED)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortTransferred]", "sort=transferred&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortTransferred]", "sort=alltimetransferred&sortreverse=" + sSharedSortRev);
	}
	else
        Out.Replace("[SortTransferred]", "&sort=transferred&sortreverse=false");
    //Request sorting link
	if(pThis->m_Params.SharedSort == SHARED_SORT_REQUESTS)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortRequests]", "sort=alltimerequests&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortRequests]", "sort=requests&sortreverse=" + sSharedSortRev);
	}
	else
	if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_REQUESTS)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortRequests]", "sort=requests&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortRequests]", "sort=alltimerequests&sortreverse=" + sSharedSortRev);
	}
	else
        Out.Replace("[SortRequests]", "&sort=requests&sortreverse=false");
    //Accepts sorting link
	if(pThis->m_Params.SharedSort == SHARED_SORT_ACCEPTS)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortAccepts]", "sort=alltimeaccepts&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortAccepts]", "sort=accepts&sortreverse=" + sSharedSortRev);
	}
	else
	if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_ACCEPTS)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortAccepts]", "sort=accepts&sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortAccepts]", "sort=alltimeaccepts&sortreverse=" + sSharedSortRev);
	}
	else
        Out.Replace("[SortAccepts]", "&sort=accepts&sortreverse=false");

	if(_ParseURL(Data.sURL, "reload") == "true")
	{
		//CString resultlog = _SpecialChars(theApp.emuledlg->logtext);	//Pick-up last line of the log
		//resultlog = resultlog.TrimRight('\n');
		//resultlog = resultlog.Mid(resultlog.ReverseFind('\n'));
		CString resultlog = _SpecialChars(theApp.emuledlg->GetLastLogEntry());
		Out.Replace("[Message]",resultlog);
	}
	else
        Out.Replace("[Message]","");


	Out.Replace("[Filename]", _GetPlainResString(IDS_DL_FILENAME)+CString((pThis->m_Params.SharedSort == SHARED_SORT_NAME)?sortimg:""));
	Out.Replace("[Priority]",  _GetPlainResString(IDS_PRIORITY)+CString((pThis->m_Params.SharedSort == SHARED_SORT_PRIORITY)?sortimg:""));
	Out.Replace("[FileTransferred]",  _GetPlainResString(IDS_SF_TRANSFERRED)+CString((pThis->m_Params.SharedSort == SHARED_SORT_TRANSFERRED)?sortimg:""));
	Out.Replace("[FileRequests]",  _GetPlainResString(IDS_SF_REQUESTS)+CString((pThis->m_Params.SharedSort == SHARED_SORT_REQUESTS)?sortimg:""));
	Out.Replace("[FileAccepts]",  _GetPlainResString(IDS_SF_ACCEPTS)+CString((pThis->m_Params.SharedSort == SHARED_SORT_ACCEPTS)?sortimg:""));
	Out.Replace("[Size]", _GetPlainResString(IDS_DL_SIZE)+CString((pThis->m_Params.SharedSort == SHARED_SORT_SIZE)?sortimg:""));
	Out.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
	Out.Replace("[Reload]", _GetPlainResString(IDS_SF_RELOAD));
	Out.Replace("[Session]", sSession);

	CString OutE = pThis->m_Templates.sSharedLine; 
	OutE.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
    OutE.Replace("[PriorityUp]", _GetPlainResString(IDS_PRIORITY_UP));
    OutE.Replace("[PriorityDown]", _GetPlainResString(IDS_PRIORITY_DOWN));

	CString OutE2 = pThis->m_Templates.sSharedLineChanged; 
	OutE.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
    OutE.Replace("[PriorityUp]", _GetPlainResString(IDS_PRIORITY_UP));
    OutE.Replace("[PriorityUp]", _GetPlainResString(IDS_PRIORITY_DOWN));

	CArray<SharedFiles, SharedFiles> SharedArray;
	// Populating array
	for (int ix=0;ix<theApp.sharedfiles->GetCount();ix++)
	{
		CCKey bufKey;
		CKnownFile* cur_file;
		cur_file=theApp.sharedfiles->GetFileByIndex(ix);// m_Files_map.GetNextAssoc(pos,bufKey,cur_file);

		SharedFiles dFile;
		dFile.sFileName = _SpecialChars(cur_file->GetFileName());
		dFile.lFileSize = cur_file->GetFileSize();
		if (theApp.IsConnected() && !theApp.IsFirewalled()) 
			dFile.sED2kLink = theApp.CreateED2kSourceLink(cur_file);
		else
            dFile.sED2kLink = CreateED2kLink(cur_file);

		dFile.nFileTransferred = cur_file->statistic.GetTransferred();
		dFile.nFileAllTimeTransferred = cur_file->statistic.GetAllTimeTransferred();
		dFile.nFileRequests = cur_file->statistic.GetRequests();
		dFile.nFileAllTimeRequests = cur_file->statistic.GetAllTimeRequests();
		dFile.nFileAccepts = cur_file->statistic.GetAccepts();
		dFile.nFileAllTimeAccepts = cur_file->statistic.GetAllTimeAccepts();

		dFile.sFileHash = EncodeBase16(cur_file->GetFileHash(), 16);
		if (cur_file->IsAutoUpPriority())
        {
            if (cur_file->GetUpPriority() == PR_LOW)
                dFile.sFilePriority = _GetPlainResString(IDS_PRIOAUTOLOW);
            else if (cur_file->GetUpPriority() == PR_NORMAL)
				dFile.sFilePriority = _GetPlainResString(IDS_PRIOAUTONORMAL);
			else if (cur_file->GetUpPriority() == PR_HIGH)
				dFile.sFilePriority = _GetPlainResString(IDS_PRIOAUTOHIGH);
			else if (cur_file->GetUpPriority() == PR_VERYHIGH)
				dFile.sFilePriority = _GetPlainResString(IDS_PRIOAUTORELEASE);
		}
		else
		{
			if (cur_file->GetUpPriority() == PR_VERYLOW)
				dFile.sFilePriority = _GetPlainResString(IDS_PRIOVERYLOW);
			else if (cur_file->GetUpPriority() == PR_LOW)
				dFile.sFilePriority = _GetPlainResString(IDS_PRIOLOW);
			else if (cur_file->GetUpPriority() == PR_NORMAL)
				dFile.sFilePriority = _GetPlainResString(IDS_PRIONORMAL);
			else if (cur_file->GetUpPriority() == PR_HIGH)
				dFile.sFilePriority = _GetPlainResString(IDS_PRIOHIGH);
			else if (cur_file->GetUpPriority() == PR_VERYHIGH)
				dFile.sFilePriority = _GetPlainResString(IDS_PRIORELEASE);
		}
		dFile.nFilePriority = cur_file->GetUpPriority();
		dFile.bFileAutoPriority = cur_file->IsAutoUpPriority();
		SharedArray.Add(dFile);
	}

	// Sorting (simple bubble sort, we don't have tons of data here)
	bool bSorted = true;
	
	for(int nMax = 0;bSorted && nMax < SharedArray.GetCount()*2; nMax++)
	{
		bSorted = false;
		for(int i = 0; i < SharedArray.GetCount() - 1; i++)
		{
			bool bSwap = false;
			switch(pThis->m_Params.SharedSort)
			{
			case SHARED_SORT_NAME:
				bSwap = SharedArray[i].sFileName.CompareNoCase(SharedArray[i+1].sFileName) > 0;
				break;
			case SHARED_SORT_SIZE:
				bSwap = SharedArray[i].lFileSize < SharedArray[i+1].lFileSize;
				break;
			case SHARED_SORT_TRANSFERRED:
				bSwap = SharedArray[i].nFileTransferred < SharedArray[i+1].nFileTransferred;
				break;
			case SHARED_SORT_ALL_TIME_TRANSFERRED:
				bSwap = SharedArray[i].nFileAllTimeTransferred < SharedArray[i+1].nFileAllTimeTransferred;
				break;
			case SHARED_SORT_REQUESTS:
				bSwap = SharedArray[i].nFileRequests < SharedArray[i+1].nFileRequests;
				break;
			case SHARED_SORT_ALL_TIME_REQUESTS:
				bSwap = SharedArray[i].nFileAllTimeRequests < SharedArray[i+1].nFileAllTimeRequests;
				break;
			case SHARED_SORT_ACCEPTS:
				bSwap = SharedArray[i].nFileAccepts < SharedArray[i+1].nFileAccepts;
				break;
			case SHARED_SORT_ALL_TIME_ACCEPTS:
				bSwap = SharedArray[i].nFileAllTimeAccepts < SharedArray[i+1].nFileAllTimeAccepts;
				break;
			case SHARED_SORT_PRIORITY:
				//Very low priority is define equal to 4 ! Must adapte sorting code
				if(SharedArray[i].nFilePriority == 4)
				{
					if(SharedArray[i+1].nFilePriority == 4)
						bSwap = false;
					else
						bSwap = true;
				}        
				else
					if(SharedArray[i+1].nFilePriority == 4)
					{
						if(SharedArray[i].nFilePriority == 4)
							bSwap = true;
						else
							bSwap = false;
					}
					else
						bSwap = SharedArray[i].nFilePriority < SharedArray[i+1].nFilePriority;
				break;
			}
			if(pThis->m_Params.bSharedSortReverse)
			{
				bSwap = !bSwap;
			}
			if(bSwap)
			{
				bSorted = true;
				SharedFiles TmpFile = SharedArray[i];
				SharedArray[i] = SharedArray[i+1];
				SharedArray[i+1] = TmpFile;
			}
		}
	}
	// Displaying
	CString sSharedList = "";
	for(int i = 0; i < SharedArray.GetCount(); i++)
	{
		char HTTPTempC[100] = "";
		CString HTTPProcessData;
		if (SharedArray[i].sFileHash == _ParseURL(Data.sURL,"hash") )
            HTTPProcessData = OutE2;
		else
            HTTPProcessData = OutE;

		HTTPProcessData.Replace("[FileName]", _SpecialChars(SharedArray[i].sFileName));
		if(SharedArray[i].sFileName.GetLength() > SHORT_FILENAME_LENGTH)
            HTTPProcessData.Replace("[ShortFileName]", _SpecialChars(SharedArray[i].sFileName.Left(SHORT_FILENAME_LENGTH)) + "...");
		else
			HTTPProcessData.Replace("[ShortFileName]", _SpecialChars(SharedArray[i].sFileName));

		sprintf(HTTPTempC, "%s",CastItoXBytes(SharedArray[i].lFileSize));
		HTTPProcessData.Replace("[FileSize]", CString(HTTPTempC));
		HTTPProcessData.Replace("[FileLink]", SharedArray[i].sED2kLink);

		sprintf(HTTPTempC, "%s",CastItoXBytes(SharedArray[i].nFileTransferred));
		HTTPProcessData.Replace("[FileTransferred]", CString(HTTPTempC));

		sprintf(HTTPTempC, "%s",CastItoXBytes(SharedArray[i].nFileAllTimeTransferred));
		HTTPProcessData.Replace("[FileAllTimeTransferred]", CString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i].nFileRequests);
		HTTPProcessData.Replace("[FileRequests]", CString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i].nFileAllTimeRequests);
		HTTPProcessData.Replace("[FileAllTimeRequests]", CString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i].nFileAccepts);
		HTTPProcessData.Replace("[FileAccepts]", CString(HTTPTempC));

		sprintf(HTTPTempC, "%i", SharedArray[i].nFileAllTimeAccepts);
		HTTPProcessData.Replace("[FileAllTimeAccepts]", CString(HTTPTempC));

		HTTPProcessData.Replace("[Priority]", SharedArray[i].sFilePriority);

		HTTPProcessData.Replace("[FileHash]", SharedArray[i].sFileHash);

		uint8 upperpriority=0, lesserpriority=0;
		if(SharedArray[i].nFilePriority == 4)
		{
			upperpriority = 0;	lesserpriority = 4;
		}
		else
		if(SharedArray[i].nFilePriority == 0)
		{
			upperpriority = 1;	lesserpriority = 4;
		}
		else
		if(SharedArray[i].nFilePriority == 1)
		{
			upperpriority = 2;	lesserpriority = 0;
		}
		else
		if(SharedArray[i].nFilePriority == 2)
		{
			upperpriority = 3;	lesserpriority = 1;
		}
		else
		if(SharedArray[i].nFilePriority == 3)
		{
			upperpriority = 5;	lesserpriority = 2;
		}
		else
		if(SharedArray[i].nFilePriority == 5)
		{
			upperpriority = 5;	lesserpriority = 3;
		}
		if(SharedArray[i].bFileAutoPriority)
		{
			upperpriority = 5;	lesserpriority = 3;
		}
        sprintf(HTTPTempC, "%i", upperpriority);
		HTTPProcessData.Replace("[PriorityUpLink]", "hash=" + SharedArray[i].sFileHash +"&setpriority=" + CString(HTTPTempC));
        sprintf(HTTPTempC, "%i", lesserpriority);
		HTTPProcessData.Replace("[PriorityDownLink]", "hash=" + SharedArray[i].sFileHash +"&setpriority=" + CString(HTTPTempC)); 

		sSharedList += HTTPProcessData;
	}
	Out.Replace("[SharedFilesList]", sSharedList);
	Out.Replace("[Session]", sSession);

	return Out;
}

CString CWebServer::_GetGraphs(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString Out = pThis->m_Templates.sGraphs;

	CString sGraphDownload = "", sGraphUpload = "", sGraphCons = "";
	CString sTmp = "";

	for(int i = 0; i < WEB_GRAPH_WIDTH; i++)
	{
		if(i < pThis->m_Params.PointsForWeb.GetCount())
		{
			if(i != 0) {
				sGraphDownload += ",";
				sGraphUpload += ",";
				sGraphCons += ",";
			}
			
			// download
			sTmp.Format("%d" , (uint32) (pThis->m_Params.PointsForWeb[i].download*1024));
			sGraphDownload += sTmp;
			// upload
			sTmp.Format("%d" , (uint32) (pThis->m_Params.PointsForWeb[i].upload*1024));
			sGraphUpload += sTmp;
			// connections
			sTmp.Format("%d" , (uint32) (pThis->m_Params.PointsForWeb[i].connections));
			sGraphCons += sTmp;
		}
	}
	Out.Replace("[GraphDownload]", sGraphDownload);
	Out.Replace("[GraphUpload]", sGraphUpload);
	Out.Replace("[GraphConnections]", sGraphCons);

	Out.Replace("[TxtDownload]", _GetPlainResString(IDS_DOWNLOAD));
	Out.Replace("[TxtUpload]", _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace("[TxtTime]", _GetPlainResString(IDS_TIME));
	Out.Replace("[TxtConnections]", _GetPlainResString(IDS_SP_ACTCON));
	Out.Replace("[KByteSec]", _GetPlainResString(IDS_KBYTESEC));
	Out.Replace("[TxtTime]", _GetPlainResString(IDS_TIME));

	CString sScale;
	sScale.Format("%s", CastSecondsToHM(thePrefs.GetTrafficOMeterInterval() * WEB_GRAPH_WIDTH) );

	CString s1, s2,s3;
	s1.Format("%d", thePrefs.GetMaxGraphDownloadRate() + 4);
	s2.Format("%d", thePrefs.GetMaxGraphUploadRate() + 4);
	s3.Format("%d", thePrefs.GetMaxConnections()+20);

	Out.Replace("[ScaleTime]", sScale);
	Out.Replace("[MaxDownload]", s1);
	Out.Replace("[MaxUpload]", s2);
	Out.Replace("[MaxConnections]", s3);

	return Out;
}

CString CWebServer::_GetAddServerBox(ThreadData Data)
{

	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	if (!IsSessionAdmin(Data,sSession)) return "";

	CString Out = pThis->m_Templates.sAddServerBox;
	if(_ParseURL(Data.sURL, "addserver") == "true")
	{
		CServer* nsrv = new CServer(atoi(_ParseURL(Data.sURL, "serverport")), _ParseURL(Data.sURL, "serveraddr").GetBuffer() );
		nsrv->SetListName(_ParseURL(Data.sURL, "servername"));
		if (!theApp.emuledlg->serverwnd->serverlistctrl.AddServer(nsrv,true))
			delete nsrv;
		CString resultlog = _SpecialChars(theApp.emuledlg->GetLastLogEntry());
		Out.Replace("[Message]",resultlog);
	}
	else
		if(_ParseURL(Data.sURL, "updateservermetfromurl") == "true")
		{
				theApp.emuledlg->serverwnd->UpdateServerMetFromURL(_ParseURL(Data.sURL, "servermeturl"));
				//CString resultlog = _SpecialChars(theApp.emuledlg->logtext);
				//resultlog = resultlog.TrimRight('\n');
				//resultlog = resultlog.Mid(resultlog.ReverseFind('\n'));
				CString resultlog = _SpecialChars(theApp.emuledlg->GetLastLogEntry());
				Out.Replace("[Message]",resultlog);
		}
		else
		Out.Replace("[Message]", "");
    Out.Replace("[AddServer]", _GetPlainResString(IDS_SV_NEWSERVER));
	Out.Replace("[IP]", _GetPlainResString(IDS_SV_ADDRESS));
	Out.Replace("[Port]", _GetPlainResString(IDS_SV_PORT));
	Out.Replace("[Name]", _GetPlainResString(IDS_SW_NAME));
	Out.Replace("[Add]", _GetPlainResString(IDS_SV_ADD));
	Out.Replace("[UpdateServerMetFromURL]", _GetPlainResString(IDS_SV_MET));
	Out.Replace("[URL]", _GetPlainResString(IDS_SV_URL));
	Out.Replace("[Apply]", _GetPlainResString(IDS_PW_APPLY));

	return Out;
}
CString	CWebServer::_GetWebSearch(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");
    
	CString Out = pThis->m_Templates.sWebSearch;
	if(_ParseURL(Data.sURL, "tosearch") != "")
	{
		CString query;
		CString tosearch = _ParseURL(Data.sURL, "tosearch");
		query = "http://www.filedonkey.com/fdsearch/index.php?media=";
		query += _ParseURL(Data.sURL, "media");
		tosearch = URLEncode(tosearch);
		tosearch.Replace("%20","+");
		query += "&pattern=";
		query += _ParseURL(Data.sURL, "tosearch");
		query += "&action=search&name=FD-Search&op=modload&file=index&requestby=emule";
		Out += "\n<script language=\"javascript\">";
		Out += "\n searchwindow=window.open('"+ query + "','searchwindow');";
		Out += "\n</script>";
	}
	Out.Replace("[Session]", sSession);
	Out.Replace("[Name]", _GetPlainResString(IDS_SW_NAME));
	Out.Replace("[Type]", _GetPlainResString(IDS_TYPE));
	Out.Replace("[Any]", _GetPlainResString(IDS_SEARCH_ANY));
	Out.Replace("[Audio]", _GetPlainResString(IDS_SEARCH_AUDIO));
	Out.Replace("[Video]", _GetPlainResString(IDS_SEARCH_VIDEO));
	Out.Replace("[Other]", "Other");
	Out.Replace("[Search]", _GetPlainResString(IDS_SW_SEARCHBOX));
	Out.Replace("[WebSearch]", _GetPlainResString(IDS_SW_WEBBASED));
	
	return Out;
}

CString CWebServer::_GetLog(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = pThis->m_Templates.sLog;

	if (_ParseURL(Data.sURL, "clear") == "yes" && IsSessionAdmin(Data,sSession))
	{
		theApp.emuledlg->ResetLog();
	}
	Out.Replace("[Clear]", _GetPlainResString(IDS_PW_RESET));
	Out.Replace("[Log]", _SpecialChars(theApp.emuledlg->GetAllLogEntries())+"<br><a name=\"end\"></a>" );
	Out.Replace("[Session]", sSession);

	return Out;
}

CString CWebServer::_GetServerInfo(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = pThis->m_Templates.sServerInfo;

	if (_ParseURL(Data.sURL, "clear") == "yes")
	{
		theApp.emuledlg->serverwnd->servermsgbox->SetWindowText("");
	}
	Out.Replace("[Clear]", _GetPlainResString(IDS_PW_RESET));
	Out.Replace("[ServerInfo]", _SpecialChars(theApp.emuledlg->serverwnd->servermsgbox->GetText()));
	Out.Replace("[Session]", sSession);

	return Out;
}

CString CWebServer::_GetDebugLog(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = pThis->m_Templates.sDebugLog;

	if (_ParseURL(Data.sURL, "clear") == "yes" && IsSessionAdmin(Data,sSession))
	{
		theApp.emuledlg->ResetDebugLog();
	}
	Out.Replace("[Clear]", _GetPlainResString(IDS_PW_RESET));
	Out.Replace("[DebugLog]", _SpecialChars(theApp.emuledlg->GetAllDebugLogEntries())+"<br><a name=\"end\"></a>" );
	Out.Replace("[Session]", sSession);

	return Out;

}

CString CWebServer::_GetStats(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	// refresh statistics
	theApp.emuledlg->statisticswnd->ShowStatistics(true);

	CString Out = pThis->m_Templates.sStats;
	Out.Replace("[STATSDATA]", theApp.emuledlg->statisticswnd->stattree.GetHTML(false));

	return Out;

}

CString CWebServer::_GetPreferences(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = pThis->m_Templates.sPreferences;
	Out.Replace("[Session]", sSession);

	if ((_ParseURL(Data.sURL, "saveprefs") == "true") && IsSessionAdmin(Data,sSession) ) {
		if(_ParseURL(Data.sURL, "gzip") == "true" || _ParseURL(Data.sURL, "gzip").MakeLower() == "on")
		{
			thePrefs.SetWebUseGzip(true);
		}
		if(_ParseURL(Data.sURL, "gzip") == "false" || _ParseURL(Data.sURL, "gzip") == "")
		{
			thePrefs.SetWebUseGzip(false);
		}
		if(_ParseURL(Data.sURL, "showuploadqueue") == "true" || _ParseURL(Data.sURL, "showuploadqueue").MakeLower() == "on" )
		{
			pThis->m_Params.bShowUploadQueue = true;
		}
		if(_ParseURL(Data.sURL, "showuploadqueue") == "false" || _ParseURL(Data.sURL, "showuploadqueue") == "")
		{
			pThis->m_Params.bShowUploadQueue = false;
		}
		if(_ParseURL(Data.sURL, "refresh") != "")
		{
			thePrefs.SetWebPageRefresh(atoi(_ParseURL(Data.sURL, "refresh")));
		}
		if(_ParseURL(Data.sURL, "maxdown") != "")
		{
			thePrefs.SetMaxDownload(atoi(_ParseURL(Data.sURL, "maxdown")));
		}
		if(_ParseURL(Data.sURL, "maxup") != "")
		{
			thePrefs.SetMaxUpload(atoi(_ParseURL(Data.sURL, "maxup")));
		}
		uint16 lastmaxgu=thePrefs.GetMaxGraphUploadRate();
		uint16 lastmaxgd=thePrefs.GetMaxGraphDownloadRate();

		if(_ParseURL(Data.sURL, "maxcapdown") != "")
		{
			thePrefs.SetMaxGraphDownloadRate(atoi(_ParseURL(Data.sURL, "maxcapdown")));
		}
		if(_ParseURL(Data.sURL, "maxcapup") != "")
		{
			thePrefs.SetMaxGraphUploadRate(atoi(_ParseURL(Data.sURL, "maxcapup")));
		}

		if(lastmaxgu != thePrefs.GetMaxGraphUploadRate()) 
			theApp.emuledlg->statisticswnd->SetARange(false,thePrefs.GetMaxGraphUploadRate());
		if(lastmaxgd!=thePrefs.GetMaxGraphDownloadRate())
			theApp.emuledlg->statisticswnd->SetARange(true,thePrefs.GetMaxGraphDownloadRate());


		if(_ParseURL(Data.sURL, "maxsources") != "")
		{
			thePrefs.SetMaxSourcesPerFile(atoi(_ParseURL(Data.sURL, "maxsources")));
		}
		if(_ParseURL(Data.sURL, "maxconnections") != "")
		{
			thePrefs.SetMaxConnections(atoi(_ParseURL(Data.sURL, "maxconnections")));
		}
		if(_ParseURL(Data.sURL, "maxconnectionsperfive") != "")
		{
			thePrefs.SetMaxConsPerFive(atoi(_ParseURL(Data.sURL, "maxconnectionsperfive")));
		}
		thePrefs.SetTransferFullChunks((_ParseURL(Data.sURL, "fullchunks").MakeLower() == "on"));
		thePrefs.SetPreviewPrio((_ParseURL(Data.sURL, "firstandlast").MakeLower() == "on"));

		thePrefs.SetNetworkED2K((_ParseURL(Data.sURL, "neted2k").MakeLower() == "on"));
		thePrefs.SetNetworkKademlia((_ParseURL(Data.sURL, "netkad").MakeLower() == "on"));
	}
	
	// Fill form
	if(thePrefs.GetWebUseGzip())
	{
		Out.Replace("[UseGzipVal]", "checked");
	}
	else
	{
		Out.Replace("[UseGzipVal]", "");
	}
    if(pThis->m_Params.bShowUploadQueue)
	{
		Out.Replace("[ShowUploadQueueVal]", "checked");
	}
	else
	{
		Out.Replace("[ShowUploadQueueVal]", "");
	}
	if(thePrefs.GetPreviewPrio())
	{
		Out.Replace("[FirstAndLastVal]", "checked");
	}
	else
	{
		Out.Replace("[FirstAndLastVal]", "");
	}
	if(thePrefs.TransferFullChunks())
	{
		Out.Replace("[FullChunksVal]", "checked");
	}
	else
	{
		Out.Replace("[FullChunksVal]", "");
	}
	CString sRefresh;
	
	sRefresh.Format("%d", thePrefs.GetWebPageRefresh());
	Out.Replace("[RefreshVal]", sRefresh);

	sRefresh.Format("%d", thePrefs.GetMaxSourcePerFile());
	Out.Replace("[MaxSourcesVal]", sRefresh);

	sRefresh.Format("%d", thePrefs.GetMaxConnections());
	Out.Replace("[MaxConnectionsVal]", sRefresh);

	sRefresh.Format("%d", thePrefs.GetMaxConperFive());
	Out.Replace("[MaxConnectionsPer5Val]", sRefresh);

	Out.Replace("[ED2KVAL]", (thePrefs.GetNetworkED2K())?"checked":"" );
	Out.Replace("[KADVAL]", (thePrefs.GetNetworkKademlia())?"checked":"" );

	Out.Replace("[KBS]", _GetPlainResString(IDS_KBYTESEC)+":");
	Out.Replace("[FileSettings]", CString(_GetPlainResString(IDS_WEB_FILESETTINGS)+":"));
	Out.Replace("[LimitForm]", _GetPlainResString(IDS_WEB_CONLIMITS)+":");
	Out.Replace("[MaxSources]", _GetPlainResString(IDS_PW_MAXSOURCES)+":");
	Out.Replace("[MaxConnections]", _GetPlainResString(IDS_PW_MAXC)+":");
	Out.Replace("[MaxConnectionsPer5]", _GetPlainResString(IDS_MAXCON5SECLABEL)+":");
	Out.Replace("[UseGzipForm]", _GetPlainResString(IDS_WEB_GZIP_COMPRESSION));
	Out.Replace("[UseGzipComment]", _GetPlainResString(IDS_WEB_GZIP_COMMENT));
	Out.Replace("[ShowUploadQueueForm]", _GetPlainResString(IDS_WEB_SHOW_UPLOAD_QUEUE));
	Out.Replace("[ShowUploadQueueComment]", _GetPlainResString(IDS_WEB_UPLOAD_QUEUE_COMMENT));
	Out.Replace("[ShowQueue]", _GetPlainResString(IDS_WEB_SHOW_UPLOAD_QUEUE));
	Out.Replace("[HideQueue]", _GetPlainResString(IDS_WEB_HIDE_UPLOAD_QUEUE));
	Out.Replace("[RefreshTimeForm]", _GetPlainResString(IDS_WEB_REFRESH_TIME));
	Out.Replace("[RefreshTimeComment]", _GetPlainResString(IDS_WEB_REFRESH_COMMENT));
	Out.Replace("[SpeedForm]", _GetPlainResString(IDS_SPEED_LIMITS));
	Out.Replace("[MaxDown]", _GetPlainResString(IDS_DOWNLOAD));
	Out.Replace("[MaxUp]", _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace("[SpeedCapForm]", _GetPlainResString(IDS_CAPACITY_LIMITS));
	Out.Replace("[MaxCapDown]", _GetPlainResString(IDS_DOWNLOAD));
	Out.Replace("[MaxCapUp]", _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace("[TryFullChunks]", _GetPlainResString(IDS_FULLCHUNKTRANS));
	Out.Replace("[FirstAndLast]", _GetPlainResString(IDS_DOWNLOADMOVIECHUNKS));
	Out.Replace("[WebControl]", _GetPlainResString(IDS_WEB_CONTROL));
	Out.Replace("[eMuleAppName]", "eMule");
	Out.Replace("[Apply]", _GetPlainResString(IDS_PW_APPLY));

	Out.Replace("[NETWORKS]", _GetPlainResString(IDS_NETWORK));

	//if ( !Kademlia::CKademlia::isRunning() || theApp.kademlia->isConnected()) {}

	Out.Replace("[BOOTSTRAP]", _GetPlainResString(IDS_BOOTSTRAP));
	Out.Replace("[BS_IP]", _GetPlainResString(IDS_IP));
	Out.Replace("[BS_PORT]", _GetPlainResString(IDS_PORT));

	Out.Replace("[KADEMLIA]", GetResString(IDS_KADEMLIA) );

	CString sT;
	int n = (int)thePrefs.GetMaxDownload();
	if(n < 0 || n == 65535) n = 0;
	sT.Format("%d", n);
	Out.Replace("[MaxDownVal]", sT);
	n = (int)thePrefs.GetMaxUpload();
	if(n < 0 || n == 65535) n = 0;
	sT.Format("%d", n);
	Out.Replace("[MaxUpVal]", sT);
	sT.Format("%d", thePrefs.GetMaxGraphDownloadRate());
	Out.Replace("[MaxCapDownVal]", sT);
	sT.Format("%d", thePrefs.GetMaxGraphUploadRate());
	Out.Replace("[MaxCapUpVal]", sT);

	return Out;
}

CString CWebServer::_GetLoginScreen(ThreadData Data)
{

	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = "";

	Out += pThis->m_Templates.sLogin;

	Out.Replace("[CharSet]", _GetWebCharSet());
	Out.Replace("[eMulePlus]", "eMule");
	Out.Replace("[eMuleAppName]", "eMule");
	Out.Replace("[version]", theApp.m_strCurVersionLong);
	Out.Replace("[Login]", _GetPlainResString(IDS_WEB_LOGIN));
	Out.Replace("[EnterPassword]", _GetPlainResString(IDS_WEB_ENTER_PASSWORD));
	Out.Replace("[LoginNow]", _GetPlainResString(IDS_WEB_LOGIN_NOW));
	Out.Replace("[WebControl]", _GetPlainResString(IDS_WEB_CONTROL));

	return Out;
}

CString CWebServer::_GetConnectedServer(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString HTTPTemp = "";
	char	HTTPTempC[100] = "";
	CString OutS = pThis->m_Templates.sConnectedServer;
	OutS.Replace("[ConnectedServer]", _GetPlainResString(IDS_PW_SERVER));
	OutS.Replace("[Servername]", _GetPlainResString(IDS_SL_SERVERNAME));
	OutS.Replace("[Status]", _GetPlainResString(IDS_STATUS));
	OutS.Replace("[Usercount]", _GetPlainResString(IDS_LUSERS));
	OutS.Replace("[Action]", _GetPlainResString(IDS_CONNECTING));
	OutS.Replace("[URL_Disconnect]", IsSessionAdmin(Data,sSession)?CString("?ses=" + sSession + "&w=server&c=disconnect"):GetPermissionDenied());
	OutS.Replace("[URL_Connect]", IsSessionAdmin(Data,sSession)?CString("?ses=" + sSession + "&w=server&c=connect"):GetPermissionDenied());
	OutS.Replace("[Disconnect]", _GetPlainResString(IDS_IRC_DISCONNECT));
	OutS.Replace("[Connect]", _GetPlainResString(IDS_CONNECTTOANYSERVER));
	OutS.Replace("[URL_ServerOptions]", IsSessionAdmin(Data,sSession)?CString("?ses=" + sSession + "&w=server&c=options"):GetPermissionDenied());
	OutS.Replace("[ServerOptions]", CString(_GetPlainResString(IDS_SERVER)+_GetPlainResString(IDS_EM_PREFS)));

	if (theApp.IsConnected()) {
		if(!theApp.IsFirewalled())
			OutS.Replace("[1]", _GetPlainResString(IDS_CONNECTED));
		else
			OutS.Replace("[1]", _GetPlainResString(IDS_CONNECTED) + " (" + _GetPlainResString(IDS_IDLOW) + ")");

		if(theApp.serverconnect->IsConnected()){
			CServer* cur_server = theApp.serverconnect->GetCurrentServer();
			OutS.Replace("[2]", CString(cur_server->GetListName()));
	
			sprintf(HTTPTempC, "%10i", cur_server->GetUsers());
			HTTPTemp = HTTPTempC;												
			OutS.Replace("[3]", HTTPTemp);
		}

	} else if (theApp.serverconnect->IsConnecting()) {
		OutS.Replace("[1]", _GetPlainResString(IDS_CONNECTING));
		OutS.Replace("[2]", "");
		OutS.Replace("[3]", "");
	} else {
		OutS.Replace("[1]", _GetPlainResString(IDS_DISCONNECTED));
		OutS.Replace("[2]", "");
		OutS.Replace("[3]", "");
	}
	return OutS;
}

// We have to add gz-header and some other stuff
// to standard zlib functions
// in order to use gzip in web pages
int CWebServer::_GzipCompress(BYTE* dest, ULONG* destLen, const BYTE* source, ULONG sourceLen, int level)
{ 
	const static int gz_magic[2] = {0x1f, 0x8b}; // gzip magic header
	int err;
	uLong crc;
	z_stream stream = {0};
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;
	crc = crc32(0L, Z_NULL, 0);
	// init Zlib stream
	// NOTE windowBits is passed < 0 to suppress zlib header
	err = deflateInit2(&stream, level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY);
	if (err != Z_OK)
		return err;

	sprintf((char*)dest , "%c%c%c%c%c%c%c%c%c%c", gz_magic[0], gz_magic[1],
		Z_DEFLATED, 0 /*flags*/, 0,0,0,0 /*time*/, 0 /*xflags*/, 255);
	// wire buffers
	stream.next_in = (Bytef*) source ;
	stream.avail_in = (uInt)sourceLen;
	stream.next_out = ((Bytef*) dest) + 10;
	stream.avail_out = *destLen - 18;
	// doit
	err = deflate(&stream, Z_FINISH);
	if (err != Z_STREAM_END)
	{
		deflateEnd(&stream);
		return err;
	}
	err = deflateEnd(&stream);
	crc = crc32(crc, (const Bytef *) source ,  sourceLen );
	//CRC
	*(((Bytef*) dest)+10+stream.total_out) = (Bytef)(crc & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+1) = (Bytef)((crc>>8) & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+2) = (Bytef)((crc>>16) & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+3) = (Bytef)((crc>>24) & 0xFF);
	// Length
	*(((Bytef*) dest)+10+stream.total_out+4) = (Bytef)( sourceLen  & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+5) = (Bytef)(( sourceLen >>8) & 0xFF);
	*(((Bytef*) dest)+10+stream.total_out+6) = (Bytef)(( sourceLen >>16) &	0xFF);
	*(((Bytef*) dest)+10+stream.total_out+7) = (Bytef)(( sourceLen >>24) &	0xFF);
	// return  destLength
	*destLen = 10 + stream.total_out + 8;
	return err;
}

bool CWebServer::_IsLoggedIn(ThreadData Data, long lSession)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	_RemoveTimeOuts(Data,lSession);

	// find our session
	// i should have used CMap there, but i like CArray more ;-)
	for(int i = 0; i < pThis->m_Params.Sessions.GetSize(); i++)
	{
		if(pThis->m_Params.Sessions[i].lSession == lSession && lSession != 0)
		{
			// if found, also reset expiration time
			pThis->m_Params.Sessions[i].startTime = CTime::GetCurrentTime();
			return true;
		}
	}

	return false;
}

void CWebServer::_RemoveTimeOuts(ThreadData Data, long lSession) {
	// remove expired sessions
	CWebServer *pThis = (CWebServer *)Data.pThis;
	pThis->UpdateSessionCount();
}

bool CWebServer::_RemoveSession(ThreadData Data, long lSession)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	// find our session
	for(int i = 0; i < pThis->m_Params.Sessions.GetSize(); i++)
	{
		if(pThis->m_Params.Sessions[i].lSession == lSession && lSession != 0)
		{
			pThis->m_Params.Sessions.RemoveAt(i);
			theApp.emuledlg->serverwnd->UpdateMyInfo();
			AddLogLine(true,GetResString(IDS_WEB_SESSIONEND));
			return true;
		}
	}
	return false;
}

Session CWebServer::GetSessionByID(ThreadData Data,long sessionID) {
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis != NULL) {
		for(int i = 0; i < pThis->m_Params.Sessions.GetSize(); i++)
		{
			if(pThis->m_Params.Sessions[i].lSession == sessionID && sessionID != 0)
				return pThis->m_Params.Sessions.GetAt(i);
		}
	}

	Session ses;
	ses.admin=false;
	ses.startTime = 0;

	return ses;
}

bool CWebServer::IsSessionAdmin(ThreadData Data,CString SsessionID){

	long sessionID=_atoi64(SsessionID);
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis != NULL) {
		for(int i = 0; i < pThis->m_Params.Sessions.GetSize(); i++)
		{
			if(pThis->m_Params.Sessions[i].lSession == sessionID && sessionID != 0)
				return pThis->m_Params.Sessions[i].admin;
		}
	}
	return false;
}

CString CWebServer::GetPermissionDenied() {
	return "javascript:alert(\'"+_GetPlainResString(IDS_ACCESSDENIED)+"\')";
}

bool CWebServer::_GetFileHash(CString sHash, uchar *FileHash)
{
	char hex_byte[3];
	int byte;
	hex_byte[2] = '\0';
	for (int i = 0; i < 16; i++) 
	{
		hex_byte[0] = sHash.GetAt(i*2);
		hex_byte[1] = sHash.GetAt((i*2 + 1));
		sscanf(hex_byte, "%02x", &byte);
		FileHash[i] = (uchar)byte;
	}
	return true;
}

__inline void PlainString(CString& rstr, bool noquot)
{
	rstr.Replace("&", "");
	if(noquot)
	{
        rstr.Replace("'", "\\'");
		rstr.Replace("\n", "\\n");
	}
}

CString	CWebServer::_GetPlainResString(RESSTRIDTYPE nID, bool noquot)
{
	CString sRet = _GetResString(nID);
	PlainString(sRet, noquot);
	return sRet;
}

// EC + kuchin
CString	CWebServer::_GetWebCharSet()
{
	switch (thePrefs.GetLanguageID())
	{
		case MAKELANGID(LANG_POLISH,SUBLANG_DEFAULT):				return "windows-1250";
		case MAKELANGID(LANG_RUSSIAN,SUBLANG_DEFAULT):				return "windows-1251";
		case MAKELANGID(LANG_GREEK,SUBLANG_DEFAULT):				return "ISO-8859-7";
		case MAKELANGID(LANG_HEBREW,SUBLANG_DEFAULT):				return "windows-1255";
		case MAKELANGID(LANG_KOREAN,SUBLANG_DEFAULT):				return "EUC-KR";
		case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_SIMPLIFIED):	return "GB2312";
		case MAKELANGID(LANG_CHINESE,SUBLANG_CHINESE_TRADITIONAL):	return "Big5";
		case MAKELANGID(LANG_LITHUANIAN,SUBLANG_DEFAULT):			return "windows-1257";
		case MAKELANGID(LANG_TURKISH,SUBLANG_DEFAULT):				return "windows-1254";
	}

	// Western (Latin) includes Catalan, Danish, Dutch, English, Faeroese, Finnish, French,
	// German, Galician, Irish, Icelandic, Italian, Norwegian, Portuguese, Spanish and Swedish
	return "ISO-8859-1";
}

// Ornis: creating the progressbar. colored if ressources are given/available
CString CWebServer::_GetDownloadGraph(ThreadData Data,CString filehash)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CPartFile* cur_file;

	// cool style
	CString progresscolor[12];
	progresscolor[0]="transparent.gif";
	progresscolor[1]="black.gif";
	progresscolor[2]="yellow.gif";
	progresscolor[3]="red.gif";

	progresscolor[4]="blue1.gif";
	progresscolor[5]="blue2.gif";
	progresscolor[6]="blue3.gif";
	progresscolor[7]="blue4.gif";
	progresscolor[8]="blue5.gif";
	progresscolor[9]="blue6.gif";

	progresscolor[10]="green.gif";
	progresscolor[11]="greenpercent.gif";

	uchar fileid[16];
	DecodeBase16(filehash.GetBuffer(),filehash.GetLength(),fileid);
	

	CString Out = "";
	CString temp;

	cur_file=theApp.downloadqueue->GetFileByID(fileid);
	if (cur_file==0 || !cur_file->IsPartFile()) {
		temp.Format(pThis->m_Templates.sProgressbarImgsPercent+"<br>",progresscolor[11],pThis->m_Templates.iProgressbarWidth);
		Out+=temp;
		temp.Format(pThis->m_Templates.sProgressbarImgs,progresscolor[10],pThis->m_Templates.iProgressbarWidth);
		Out+=temp;
	}
		else
	{
		CString s_ChunkBar=cur_file->GetProgressString(pThis->m_Templates.iProgressbarWidth);

		// and now make a graph out of the array - need to be in a progressive way
		uint8 lastcolor=1;
		uint16 lastindex=0;
		for (uint16 i=0;i<pThis->m_Templates.iProgressbarWidth;i++) {
			if (lastcolor!= atoi(s_ChunkBar.Mid(i,1))  ) {
				if (i>lastindex) {
					temp.Format(pThis->m_Templates.sProgressbarImgs ,progresscolor[lastcolor],i-lastindex);

					Out+=temp;
				}
				lastcolor=atoi(s_ChunkBar.Mid(i,1));
				lastindex=i;
			}
		}
		temp.Format(pThis->m_Templates.sProgressbarImgs,progresscolor[lastcolor],pThis->m_Templates.iProgressbarWidth-lastindex);
		Out+=temp;

		int compl=(int)((pThis->m_Templates.iProgressbarWidth/100.0)*cur_file->GetPercentCompleted());
		(compl>0)?temp.Format(pThis->m_Templates.sProgressbarImgsPercent+"<br>",progresscolor[11],compl) :temp.Format(pThis->m_Templates.sProgressbarImgsPercent+"<br>",progresscolor[0],5);
		Out=temp+Out;
	}
	return Out;
}

CString	CWebServer::_GetSearch(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");
	CString Out = pThis->m_Templates.sSearch;

	if (_ParseURL(Data.sURL, "downloads") != "" && IsSessionAdmin(Data,sSession) ) {
		CString downloads=_ParseURLArray(Data.sURL,"downloads");
		uint8 cat=atoi(_ParseURL(Data.sURL, "cat"));

		CString resToken;
		int curPos=0;
		resToken= downloads.Tokenize("|",curPos);
		while (resToken != "")
		{
			uchar fileid[16];
			DecodeBase16(resToken.GetBuffer(),resToken.GetLength(),fileid);
			theApp.searchlist->AddFileToDownloadByHash(fileid,cat);
			resToken= downloads.Tokenize("|",curPos);
		}
	}

	if(_ParseURL(Data.sURL, "tosearch") != "" && IsSessionAdmin(Data,sSession) )
	{
		// perform search
		theApp.emuledlg->searchwnd->DeleteAllSearchs();

		// get method
		CString method=(_ParseURL(Data.sURL, "method"));

		SSearchParams* pParams = new SSearchParams;
		pParams->strExpression = _ParseURL(Data.sURL, "tosearch");
		pParams->strFileType = _ParseURL(Data.sURL, "type");
		pParams->ulMinSize = atol(_ParseURL(Data.sURL, "min"))*1048576;
		pParams->ulMaxSize = atol(_ParseURL(Data.sURL, "max"))*1048576;
		pParams->iAvailability = (_ParseURL(Data.sURL, "avail")=="")?-1:atoi(_ParseURL(Data.sURL, "avail"));
		pParams->strExtension = _ParseURL(Data.sURL, "ext");
		if (method == "kademlia")
			pParams->eType = SearchTypeKademlia;
		else if (method == "global")
			pParams->eType = SearchTypeGlobal;
		else
			pParams->eType = SearchTypeServer;

		if (pParams->eType != SearchTypeKademlia){
		    if (!theApp.emuledlg->searchwnd->DoNewSearch(pParams))
				delete pParams;
		}
		else{
			if (!theApp.emuledlg->searchwnd->DoNewKadSearch(pParams))
				delete pParams;
		}
		Out.Replace("[Message]",_GetPlainResString(IDS_SW_SEARCHINGINFO));
	}
	else if(_ParseURL(Data.sURL, "tosearch") != "" && !IsSessionAdmin(Data,sSession) ) {
		Out.Replace("[Message]",_GetPlainResString(IDS_ACCESSDENIED));
	}
	else Out.Replace("[Message]","");

	CString sSort = _ParseURL(Data.sURL, "sort");	if (sSort.GetLength()>0) pThis->m_iSearchSortby=atoi(sSort);
	sSort = _ParseURL(Data.sURL, "sortAsc");		if (sSort.GetLength()>0) pThis->m_bSearchAsc=atoi(sSort);

	CString result=pThis->m_Templates.sSearchHeader +
		theApp.searchlist->GetWebList(pThis->m_Templates.sSearchResultLine,pThis->m_iSearchSortby,pThis->m_bSearchAsc);

	if (thePrefs.GetCatCount()>1) InsertCatBox(Out,0,pThis->m_Templates.sCatArrow); else Out.Replace("[CATBOX]","");
	
	CString sortimg;
	if (pThis->m_bSearchAsc) sortimg="<img src=\"l_up.gif\">";
		else sortimg="<img src=\"l_down.gif\">";

	Out.Replace("[SEARCHINFOMSG]","");
	Out.Replace("[RESULTLIST]", result);
	Out.Replace("[Result]", GetResString(IDS_SW_RESULT) );
	Out.Replace("[Session]", sSession);
	Out.Replace("[WebSearch]", _GetPlainResString(IDS_SW_WEBBASED));
	Out.Replace("[Name]", _GetPlainResString(IDS_SW_NAME));
	Out.Replace("[Type]", _GetPlainResString(IDS_TYPE));
	Out.Replace("[Any]", _GetPlainResString(IDS_SEARCH_ANY));
	Out.Replace("[Audio]", _GetPlainResString(IDS_SEARCH_AUDIO));
	Out.Replace("[Image]", _GetPlainResString(IDS_SEARCH_PICS));
	Out.Replace("[Video]", _GetPlainResString(IDS_SEARCH_VIDEO));
	Out.Replace("[Other]", "Other");
	Out.Replace("[Search]", _GetPlainResString(IDS_SW_SEARCHBOX));
	Out.Replace("[RefetchResults]", _GetPlainResString(IDS_SW_REFETCHRES));
	Out.Replace("[Download]", _GetPlainResString(IDS_DOWNLOAD));

	Out.Replace("[Filesize]", _GetPlainResString(IDS_DL_SIZE)+CString((pThis->m_iSearchSortby==1)?sortimg:""));
	Out.Replace("[Sources]", _GetPlainResString(IDS_DL_SOURCES)+CString((pThis->m_iSearchSortby==3)?sortimg:""));
	Out.Replace("[Filehash]", _GetPlainResString(IDS_FILEHASH)+CString((pThis->m_iSearchSortby==2)?sortimg:""));
	Out.Replace("[Filename]", _GetPlainResString(IDS_DL_FILENAME)+CString((pThis->m_iSearchSortby==0)?sortimg:""));
	Out.Replace("[WebSearch]", _GetPlainResString(IDS_SW_WEBBASED));

	Out.Replace("[SizeMin]", _GetPlainResString(IDS_SEARCHMINSIZE));
	Out.Replace("[SizeMax]", _GetPlainResString(IDS_SEARCHMAXSIZE));
	Out.Replace("[Availabl]", _GetPlainResString(IDS_SEARCHAVAIL));
	Out.Replace("[Extention]", _GetPlainResString(IDS_SEARCHEXTENTION));
	Out.Replace("[MB]", _GetPlainResString(IDS_MBYTES));
	
	Out.Replace("[METHOD]", _GetPlainResString(IDS_METHOD));
	Out.Replace("[USESSERVER]", _GetPlainResString(IDS_SERVER));
	Out.Replace("[USEKADEMLIA]", _GetPlainResString(IDS_KADEMLIA));
	Out.Replace("[Global]", _GetPlainResString(IDS_GLOBALSEARCH));

	CString val;
	val.Format("%i",(pThis->m_iSearchSortby!=0 || (pThis->m_iSearchSortby==0 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE0]", val);
	val.Format("%i",(pThis->m_iSearchSortby!=1 || (pThis->m_iSearchSortby==1 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE1]", val);
	val.Format("%i",(pThis->m_iSearchSortby!=2 || (pThis->m_iSearchSortby==2 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE2]", val);
	val.Format("%i",(pThis->m_iSearchSortby!=3 || (pThis->m_iSearchSortby==3 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace("[SORTASCVALUE3]", val);

	return Out;
}

int CWebServer::UpdateSessionCount() {

	// remove old bans
	for(int i = 0; i < m_Params.badlogins.GetSize();)
	{
		uint32 diff= ::GetTickCount() - m_Params.badlogins[i].timestamp ;
		if ( diff >1000*60*15 && (::GetTickCount() > m_Params.badlogins[i].timestamp ) ) {
			m_Params.badlogins.RemoveAt(i);
		}
			else i++;
	}

	// count & remove old session
	int oldvalue=m_Params.Sessions.GetSize();
	for(int i = 0; i < m_Params.Sessions.GetSize();)
	{
		CTimeSpan ts = CTime::GetCurrentTime() - m_Params.Sessions[i].startTime;
		if(ts.GetTotalSeconds() > SESSION_TIMEOUT_SECS) {
			m_Params.Sessions.RemoveAt(i);
		}
		else
			i++;
	}

	if (oldvalue!=m_Params.Sessions.GetSize()) theApp.emuledlg->serverwnd->UpdateMyInfo();
		
	return m_Params.Sessions.GetCount();
}
void CWebServer::InsertCatBox(CString &Out,int preselect,CString boxlabel,bool jump,bool extraCats) {
	CString tempBuf2,tempBuf3;
	if (jump) tempBuf2="onchange=GotoCat(this.form.cat.options[this.form.cat.selectedIndex].value)>";
	else tempBuf2=">";
	CString tempBuf="<form><select name=\"cat\" size=\"1\""+tempBuf2;

	for (int i=0;i< thePrefs.GetCatCount();i++) {
		tempBuf3= (i==preselect)? " selected":"";
		tempBuf2.Format("<option%s value=\"%i\">%s</option>",tempBuf3,i, (i==0)?_GetPlainResString(IDS_ALL):thePrefs.GetCategory(i)->title );
		tempBuf.Append(tempBuf2);
	}
	if (extraCats) {
		if (thePrefs.GetCatCount()>1){
			tempBuf2.Format("<option>------------</option>");
			tempBuf.Append(tempBuf2);
		}
	
		for (int i=(thePrefs.GetCatCount()>1)?1:2;i<=12;i++) {
			tempBuf3= ( (-i)==preselect)? " selected":"";
			tempBuf2.Format("<option%s value=\"%i\">%s</option>",tempBuf3,-i, GetSubCatLabel(-i) );
			tempBuf.Append(tempBuf2);
		}
	}
	tempBuf.Append("</select></form>");
	Out.Replace("[CATBOX]",boxlabel+tempBuf);
}

CString CWebServer::GetSubCatLabel(int cat) {
	switch (cat) {
		case -1: return _GetPlainResString(IDS_ALLOTHERS);
		case -2: return _GetPlainResString(IDS_STATUS_NOTCOMPLETED);
		case -3: return _GetPlainResString(IDS_DL_TRANSFCOMPL);
		case -4: return _GetPlainResString(IDS_WAITING);
		case -5: return _GetPlainResString(IDS_DOWNLOADING);
		case -6: return _GetPlainResString(IDS_ERRORLIKE);
		case -7: return _GetPlainResString(IDS_PAUSED);
		case -8: return _GetPlainResString(IDS_STOPPED);
		case -9: return _GetPlainResString(IDS_VIDEO);
		case -10: return _GetPlainResString(IDS_AUDIO);
		case -11: return _GetPlainResString(IDS_SEARCH_ARC);
		case -12: return _GetPlainResString(IDS_SEARCH_CDIMG);
	}
	return "?";
}

// Elandal: moved from CUpDownClient
// Webserber [kuchin]
CString CWebServer::GetUploadFileInfo(CUpDownClient* client)
{
	if(client == NULL) return "";
	CString sRet;

	// build info text and display it
	sRet.Format(GetResString(IDS_USERINFO), client->GetUserName(), client->GetUserIDHybrid());
	if (client->reqfile)
	{
		sRet += GetResString(IDS_SF_REQUESTED) + CString(client->reqfile->GetFileName()) + "\n";
		CString stat;
		stat.Format(GetResString(IDS_FILESTATS_SESSION)+GetResString(IDS_FILESTATS_TOTAL),
			client->reqfile->statistic.GetAccepts(),
			client->reqfile->statistic.GetRequests(),
			CastItoXBytes(client->reqfile->statistic.GetTransferred()),
			client->reqfile->statistic.GetAllTimeAccepts(),
			client->reqfile->statistic.GetAllTimeRequests(),
			CastItoXBytes(client->reqfile->statistic.GetAllTimeTransferred()) );
		sRet += stat;
	}
	else
	{
		sRet += GetResString(IDS_REQ_UNKNOWNFILE);
	}
	return sRet;
}


CString	CWebServer::_GetKadPage(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	if (!thePrefs.GetNetworkKademlia()) return "";

	CString sSession = _ParseURL(Data.sURL, "ses");
	CString Out = pThis->m_Templates.sKad;

	if (_ParseURL(Data.sURL, "bootstrap") != "" && IsSessionAdmin(Data,sSession) ) {
		CString dest=_ParseURL(Data.sURL, "ipport");
		int pos=dest.Find(':');
		if (pos!=-1) {
			uint16 port=atoi(dest.Right( dest.GetLength()-pos-1));
			CString ip=dest.Left(pos);
			Kademlia::CKademlia::bootstrap(ip,port);
		}
	}

	if (_ParseURL(Data.sURL, "c") == "connect" && IsSessionAdmin(Data,sSession) ) {
		Kademlia::CKademlia::start();
	}

	if (_ParseURL(Data.sURL, "c") == "disconnect" && IsSessionAdmin(Data,sSession) ) {
		Kademlia::CKademlia::stop();
	}

	// check the condition if bootstrap is possible
	if ( Kademlia::CKademlia::isRunning() &&  !Kademlia::CKademlia::isConnected()) {

		Out.Replace("[BOOTSTRAPLINE]", pThis->m_Templates.sBootstrapLine );

		// Bootstrap
		CString bsip=_ParseURL(Data.sURL, "bsip");
		uint16 bsport=atoi(_ParseURL(Data.sURL, "bsport"));

		if (!bsip.IsEmpty() && bsport>0)
			Kademlia::CKademlia::bootstrap(bsip,bsport);
	} else Out.Replace("[BOOTSTRAPLINE]", "" );

	// Infos
	CString info;
	if (thePrefs.GetNetworkKademlia()) {
		CString buffer;
			
			buffer.Format("%s: %i<br>", GetResString(IDS_KADCONTACTLAB), theApp.emuledlg->kademliawnd->contactList->GetItemCount());
			info.Append(buffer);

			buffer.Format("%s: %i<br>", GetResString(IDS_KADSEARCHLAB), theApp.emuledlg->kademliawnd->searchList->GetItemCount());
			info.Append(buffer);

		
	} else info="";
	Out.Replace("[KADSTATSDATA]",info);

	Out.Replace("[BS_IP]",GetResString(IDS_IP));
	Out.Replace("[BS_PORT]",GetResString(IDS_PORT));
	Out.Replace("[BOOTSTRAP]",GetResString(IDS_BOOTSTRAP));
	Out.Replace("[KADSTAT]",GetResString(IDS_STATSSETUPINFO));

	return Out;
}
