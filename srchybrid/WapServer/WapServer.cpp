//emulEspaña - Copied from WebServer.cpp and then modified by MoNKi [MoNKi: -Wap Server-]
#include "StdAfx.h"
#include "./CxImage/xImage.h"
#include "emule.h"
#include "OtherFunctions.h"
#include "SearchDlg.h"
#include "SearchParams.h"
#include "WapServer.h"
#include "ED2KLink.h"
#include "MD5Sum.h"
#include <stdlib.h>
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
#include "Exceptions.h"
// emulEspaña: added by Announ [Maella: -Allow Bandwidth Settings in <1KB Incremements-]
#include "opcodes.h"
// End emulEspaña

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define WMLInit "Server: eMule\r\nConnection: close\r\nContent-Type: text/vnd.wap.wml\r\n"

#define WAP_SERVER_TEMPLATES_VERSION	2

CWapServer::CWapServer(void)
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

CWapServer::~CWapServer(void)
{
	if (m_bServerWorking) StopWapSockets();
}

void CWapServer::RestartServer() {	//Cax2 - restarts the server with the new port settings
	StopWapSockets();
	if (m_bServerWorking)	
		StartWapSockets(this);
}

CString CWapServer::_LoadTemplate(CString sAll, CString sTemplateName)
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
		if (nStart==-1) AddDebugLogLine(false,GetResString(IDS_WEB_ERR_CANTLOAD),sTemplateName);
	}

	return sRet;
}

void CWapServer::ReloadTemplates()
{
	CString sPrevLocale(setlocale(LC_TIME,NULL));
	setlocale(LC_TIME, _T("English"));
	CTime t = GetCurrentTime();
	m_Params.sLastModified = t.FormatGmt("%a, %d %b %Y %H:%M:%S GMT");
	m_Params.sETag = MD5Sum(m_Params.sLastModified).GetHash();
	setlocale(LC_TIME, sPrevLocale);

	CString sFile;
	if (thePrefs.GetWapTemplate()=="" || thePrefs.GetWapTemplate().MakeLower()=="emule_wap.tmpl")
		sFile= thePrefs.GetAppDir() + CString("eMule_Wap.tmpl");
	else sFile=thePrefs.GetWapTemplate();

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
		if(lVersion < WAP_SERVER_TEMPLATES_VERSION)
		{
			if(m_bServerWorking)
				AddLogLine(true,GetResString(IDS_WS_ERR_LOADTEMPLATE),sFile);
		}
		else
		{
			m_Templates.sScriptFile = _LoadTemplate(sAll,"TMPL_WMLSCRIPT");
			m_Templates.sMain = _LoadTemplate(sAll,"TMPL_MAIN");
			m_Templates.sTransferDownLineLite = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_LINE_LITE");
			m_Templates.sTransferDownLineLiteGood = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_LINE_LITE_GOOD");
			m_Templates.sTransferDownFileDetails = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_FILE_DETAILS");
			m_Templates.sServerList = _LoadTemplate(sAll,"TMPL_SERVER_LIST");
			m_Templates.sServerMain = _LoadTemplate(sAll,"TMPL_SERVER_MAIN");
			m_Templates.sServerLine = _LoadTemplate(sAll,"TMPL_SERVER_LINE");
			m_Templates.sTransferDownList = _LoadTemplate(sAll,"TMPL_TRANSFER_LIST");
			m_Templates.sTransferUpList = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_LIST");
			m_Templates.sTransferUpLine = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_LINE");
			m_Templates.sTransferUpQueueList = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_QUEUE_LIST");
			m_Templates.sTransferUpQueueLine = _LoadTemplate(sAll,"TMPL_TRANSFER_UP_QUEUE_LINE");
			m_Templates.sDownloadLink = _LoadTemplate(sAll,"TMPL_DOWNLOAD_LINK");
			m_Templates.sSharedList = _LoadTemplate(sAll,"TMPL_SHARED_LIST");
			m_Templates.sSharedLine = _LoadTemplate(sAll,"TMPL_SHARED_LINE");
			m_Templates.sSharedLineChanged = _LoadTemplate(sAll,"TMPL_SHARED_LINE_CHANGED");
			m_Templates.sGraphs = _LoadTemplate(sAll,"TMPL_GRAPHS");
			m_Templates.sLog = _LoadTemplate(sAll,"TMPL_LOG");
			m_Templates.sServerInfo = _LoadTemplate(sAll,"TMPL_SERVERINFO");
			m_Templates.sDebugLog = _LoadTemplate(sAll,"TMPL_DEBUGLOG");
			m_Templates.sStats = _LoadTemplate(sAll,"TMPL_STATS");
			m_Templates.sPreferences = _LoadTemplate(sAll,"TMPL_PREFERENCES");
			m_Templates.sLogin = _LoadTemplate(sAll,"TMPL_LOGIN");
			m_Templates.sConnectedServer = _LoadTemplate(sAll,"TMPL_CONNECTED_SERVER");
			m_Templates.sServerOptions = _LoadTemplate(sAll,"TMPL_SERVER_OPTIONS");
			m_Templates.sSearchMain = _LoadTemplate(sAll,"TMPL_SEARCH_MAIN");
			m_Templates.sSearchList = _LoadTemplate(sAll,"TMPL_SEARCH_LIST");
			m_Templates.sSearchResultLine = _LoadTemplate(sAll,"TMPL_SEARCH_RESULT_LINE");			
			m_Templates.sClearCompleted = _LoadTemplate(sAll,"TMPL_TRANSFER_DOWN_CLEARBUTTON");
			m_Templates.sBootstrapLine= _LoadTemplate(sAll,"TMPL_BOOTSTRAPLINE");
			m_Templates.sKad= _LoadTemplate(sAll,"TMPL_KADDLG");
			
			CString tmpColor;
			tmpColor.Format("0x00%s",_LoadTemplate(sAll,"TMPL_DOWNLOAD_GRAPHCOLOR"));
			m_Templates.cDownColor = _tcstol(tmpColor,NULL,16);
			tmpColor.Format("0x00%s",_LoadTemplate(sAll,"TMPL_UPLOAD_GRAPHCOLOR"));
			m_Templates.cUpColor = _tcstol(tmpColor,NULL,16);
			tmpColor.Format("0x00%s",_LoadTemplate(sAll,"TMPL_CONNECTIONS_GRAPHCOLOR"));
			m_Templates.cConnectionsColor = _tcstol(tmpColor,NULL,16);
		}
	}
	else
        if(m_bServerWorking)
			AddLogLine(true,GetResString(IDS_WEB_ERR_CANTLOAD), sFile);

}

void CWapServer::StartServer(void)
{
	if(m_bServerWorking != thePrefs.GetWapServerEnabled())
		m_bServerWorking = thePrefs.GetWapServerEnabled();
	else
		return;

	if (m_bServerWorking) {
		ReloadTemplates();
		StartWapSockets(this);
	} else StopWapSockets();

	if(thePrefs.GetWapServerEnabled())
		AddLogLine(false,"%s: %s",_GetPlainResString(IDS_PW_WAP), _GetPlainResString(IDS_ENABLED));
	else
		AddLogLine(false,"%s: %s",_GetPlainResString(IDS_PW_WAP), _GetPlainResString(IDS_DISABLED));
}

void CWapServer::_RemoveServer(CString sIP, int nPort)
{
	CServer* server=theApp.serverlist->GetServerByAddress(sIP.GetBuffer() ,nPort);
	if (server!=NULL) theApp.emuledlg->SendMessage(WEB_REMOVE_SERVER, (WPARAM)server, NULL);
}

void CWapServer::_SetSharedFilePriority(CString hash, uint8 priority)
{	
	CKnownFile* cur_file;
	uchar fileid[16];
	if (hash.GetLength()!=32 || !DecodeBase16(hash.GetBuffer(),hash.GetLength(),fileid,ARRSIZE(fileid)))
		return;

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

void CWapServer::AddStatsLine(UpDown line)
{
	m_Params.PointsForWeb.Add(line);
	if(m_Params.PointsForWeb.GetCount() > thePrefs.GetWapGraphWidth())
		m_Params.PointsForWeb.RemoveAt(0);
}

CString CWapServer::_SpecialChars(CString str) 
{
	str.Replace("&","&amp;");
	str.Replace("<","&lt;");
	str.Replace(">","&gt;");
	str.Replace("\"","&quot;");
	str.Replace("'","&#39;");
	str.Replace("$","$$");

	return str;
}

void CWapServer::_ConnectToServer(CString sIP, int nPort)
{
	CServer* server=NULL;
	if (!sIP.IsEmpty()) server=theApp.serverlist->GetServerByAddress(sIP.GetBuffer(),nPort);
	theApp.emuledlg->SendMessage(WEB_CONNECT_TO_SERVER, (WPARAM)server, NULL);
}

void CWapServer::ProcessFileReq(WapThreadData Data) {
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if (pThis == NULL) return;

	CString filename=Data.sURL;
	CString contenttype;

	filename.Replace('/','\\');

	if (filename.GetAt(0)=='\\') filename.Delete(0);

	filename=thePrefs.GetWapServerDir()+filename;

	if(filename.Right(11).CompareNoCase("script.wmls")==0){
		SendScriptFile(Data);
		return;
	}

	if(filename.Right(5).MakeLower()==".wbmp" ||
	   filename.Right(4).MakeLower()==".png" ||
	   filename.Right(4).MakeLower()==".gif"){
		SendImageFile(Data,filename);
		return;
	}

	if (filename.Right(5).MakeLower()==".wbmp") contenttype="Content-Type: image/vnd.wap.wbmp\r\n";
	  else if (filename.Right(5).MakeLower()==".wmls") contenttype="Content-Type: text/vnd.wap.wmlscript\r\n";

	contenttype += "Last-Modified: " + pThis->m_Params.sLastModified + "\r\n" + "ETag: " + pThis->m_Params.sETag + "\r\n";

	if(!SendFile(Data,filename,contenttype))
		Data.pSocket->SendError404(true);
}

void CWapServer::ProcessURL(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
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
		CString Out = "";
		CString OutE = "";	// List Entry Templates
		CString OutE2 = "";

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

			if(MD5Sum(_ParseURL(Data.sURL, "p")).GetHash() == thePrefs.GetWapPass())
			{
				Session ses;
				ses.admin=true;
				ses.startTime = CTime::GetCurrentTime();
				ses.lSession = lSession = rand() * 10000L + rand();
				pThis->m_Params.Sessions.Add(ses);
				theApp.emuledlg->serverwnd->UpdateMyInfo();
				AddLogLine(true,GetResString(IDS_WAP_ADMINLOGIN)+" (%s)",ip);
				login=true;
			}
			else if(thePrefs.GetWapIsLowUserEnabled() && thePrefs.GetWapLowPass()!="" && MD5Sum(_ParseURL(Data.sURL, "p")).GetHash() == thePrefs.GetWapLowPass())
			{
				Session ses;
				ses.admin=false;
				ses.startTime = CTime::GetCurrentTime();
				ses.lSession = lSession = rand() * 10000L + rand();
				pThis->m_Params.Sessions.Add(ses);
				theApp.emuledlg->serverwnd->UpdateMyInfo();
				AddLogLine(true,GetResString(IDS_WAP_GUESTLOGIN)+" (%s)",ip);
				login=true;
			} else {
				AddLogLine(true,GetResString(IDS_WAP_BADLOGINATTEMPT)+" (%s)",ip);

				BadLogin newban={inet_addr(ip), ::GetTickCount()};	// save failed attempt (ip,time)
				pThis->m_Params.badlogins.Add(newban);
				login=false;
			}

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
			// Check if graph image request
			CString filename = _ParseURL(Data.sURL, "graphimage");
			if (filename.CompareNoCase("download")==0){
				SendGraphFile(Data, 1);
				return;
			}else if (filename.CompareNoCase("upload")==0){
				SendGraphFile(Data, 2);
				return;
			}else if (filename.CompareNoCase("connections")==0){
				SendGraphFile(Data, 3);
				return;
			}

			// Check if progressbar image request
			if(_ParseURL(Data.sURL, "progressbar")!=""){
				CString strFileHash = _ParseURL(Data.sURL, "progressbar");
				SendProgressBar(Data, strFileHash);
				return;
			}

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
			else
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
			if (sPage == "transferdown") 
			{
				Out += _GetTransferDownList(Data);
			}
			else
			if (sPage == "transferup") 
			{
				Out += _GetTransferUpList(Data);
			}
			else
			if (sPage == "transferqueue") 
			{
				Out += _GetTransferQueueList(Data);
			}
			else
			/*if (sPage == "websearch") 
			{
				Out += _GetWebSearch(Data);
			}
			else*/
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
			}else
			if (sPage == "sinfo") 
			{
				Out += _GetServerInfo(Data);
			}else
			if (sPage == "debuglog") 
			{
				Out += _GetDebugLog(Data);
			}else
			if (sPage == "stats") 
			{
				Out += _GetStats(Data);
			}else
			if (sPage == "options") 
			{
				Out += _GetPreferences(Data);
			}else
				Out += _GetMain(Data, lSession);
		}
		else 
		{
			uint32 ip= inet_addr(inet_ntoa( Data.inadr ));
			uint32 faults=0;

			// check for bans
			for(int i = 0; i < pThis->m_Params.badlogins.GetSize();i++)
				if ( pThis->m_Params.badlogins[i].datalen==ip ) faults++;

			if (faults>4) {
				Out +="<?xml version=\"1.0\" encoding=\""+ _GetWebCharSet() +"\"?>";
				Out +="<!DOCTYPE wml PUBLIC \"-//WAPFORUM//DTD WML 1.1//EN\" \"http://www.wapforum.org/DTD/wml_1.1.xml\">";
				Out +="<wml><card><p>";
				Out += _GetPlainResString(IDS_ACCESSDENIED);
				Out +="</p></card></wml>";
				
				// set 15 mins ban by using the badlist
				BadLogin preventive={ip, ::GetTickCount() + (15*60*1000) };
				for (int i=0;i<=5;i++)
					pThis->m_Params.badlogins.Add(preventive);

			}
			else
				Out += _GetLoginScreen(Data);
		}
		
		Out.Replace("[CharSet]", _GetWebCharSet());
		// send answer ...
		Data.pSocket->SendContent(WMLInit, Out, Out.GetLength());
	}
	catch(...){
		TRACE("*** Unknown exception in CWapServer::ProcessURL\n");
		ASSERT(0);
	}

	CoUninitialize();
}

CString CWapServer::_ParseURLArray(CString URL, CString fieldname) {
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

CString CWapServer::_ParseURL(CString URL, CString fieldname)
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

CString CWapServer::_GetMain(WapThreadData Data, long lSession)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession; sSession.Format("%ld", lSession);

	CString	Out = pThis->m_Templates.sMain;

/*	if(thePrefs.GetWebPageRefresh() != 0)
	{
		CString sPage = _ParseURL(Data.sURL, "w");
		if ((sPage == "transfer") ||
			(sPage == "server") ||
			(sPage == "graphs") ||
			(sPage == "log") ||
			(sPage == "sinfo") ||
			(sPage == "debuglog") ||
			(sPage == "stats"))
		{

			CString sT = pThis->m_Templates.sHeaderMetaRefresh;

			CString sRefresh; sRefresh.Format("%d", thePrefs.GetWebPageRefresh());
			sT.Replace("[RefreshVal]", sRefresh);

			CString catadd="";
			if (sPage == "transfer")
				catadd="&amp;cat="+_ParseURL(Data.sURL, "cat");
			sT.Replace("[wCommand]", _ParseURL(Data.sURL, "w")+catadd);

			Out.Replace("[HeaderMeta]", sT);
		}
	}
	Out.Replace("[HeaderMeta]", ""); // In case there are no meta	
	*/
	Out.Replace("[Session]", sSession);
	Out.Replace("[eMuleAppName]", "eMule");

	// modified by Announ [itsonlyme: -modname-]
	/*
	Out.Replace("[version]", _SpecialChars(theApp.m_strCurVersionLong));
	*/
	Out.Replace("[version]", _SpecialChars(theApp.m_strCurVersionLong)+CString(" ")+theApp.m_strModLongVersion);
	// End -modname-
	
	Out.Replace("[WapControl]", _GetPlainResString(IDS_WAP_CONTROL));
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
	Out.Replace("[TransferDown]", _GetPlainResString(IDS_TW_DOWNLOADS));
	Out.Replace("[TransferUp]", _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace("[TransferQueue]", _GetPlainResString(IDS_ONQUEUE));
	Out.Replace("[BasicData]", _GetPlainResString(IDS_WAP_BASICDATA));
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
	Out.Replace("[SearchResults]", _GetPlainResString(IDS_SW_RESULT));
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
			sConnected += ": " + _SpecialChars(cur_server->GetListName());
	
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
		
		if (IsSessionAdmin(Data,sSession)){
			sConnected+=" (<a href=\"./?ses=" + sSession + "&amp;w=server&amp;c=connect\">" + _GetPlainResString(IDS_CONNECTTOANYSERVER)+"</a>)";
		}
	}
	Out.Replace("[Connected]", "<b>"+_GetPlainResString(IDS_PW_CONNECTION)+":</b> "+sConnected);
    sprintf(HTTPTempC, _GetPlainResString(IDS_UPDOWNSMALL),(float)theApp.uploadqueue->GetDatarate()/1024,(float)theApp.downloadqueue->GetDatarate()/1024);
	CString sLimits;
	// EC 25-12-2003
	CString MaxUpload;
	CString MaxDownload;

	// emulEspaña: modified by MoNKi [Maella: -Allow Bandwidth Settings in <1KB Incremements-]
	/*
	MaxUpload.Format("%i",thePrefs.GetMaxUpload());
	MaxDownload.Format("%i",thePrefs.GetMaxDownload());
	if (MaxUpload == "65535")  MaxUpload = GetResString(IDS_PW_UNLIMITED);
	if (MaxDownload == "65535") MaxDownload = GetResString(IDS_PW_UNLIMITED);
	*/
	if ( CPreferences::GetMaxUpload() < UNLIMITED )
		MaxUpload.Format("%.1f", CPreferences::GetMaxUpload());
	else
		MaxUpload = GetResString(IDS_PW_UNLIMITED);
	if ( CPreferences::GetMaxDownload() < UNLIMITED )
		MaxDownload.Format("%.1f", CPreferences::GetMaxDownload());
	else
		MaxDownload = GetResString(IDS_PW_UNLIMITED);
	// End emulEspaña

	sLimits.Format("%s/%s", MaxUpload, MaxDownload);
	// EC Ends

	Out.Replace("[Speed]", "<b>"+_GetPlainResString(IDS_DL_SPEED)+":</b> "+_SpecialChars(CString(HTTPTempC)) + "(" + _GetPlainResString(IDS_PW_CON_LIMITFRM) + ": " + _SpecialChars(sLimits) + ")");

	CString buffer;
	buffer="<b>"+GetResString(IDS_KADEMLIA)+":</b> ";

	if (!thePrefs.GetNetworkKademlia()) {
		buffer.Append(_GetPlainResString(IDS_DISABLED));
	}
	else if ( !Kademlia::CKademlia::isRunning() ) {

		buffer.Append(_GetPlainResString(IDS_DISCONNECTED));

		if (IsSessionAdmin(Data,sSession)) 
			buffer+=" (<a href=\"?ses=" + sSession + "&amp;w=kad&amp;c=connect\">"+_GetPlainResString(IDS_MAIN_BTN_CONNECT)+"</a>)";
	}
	else if ( Kademlia::CKademlia::isRunning() && !Kademlia::CKademlia::isConnected() ) {

		buffer.Append(_GetPlainResString(IDS_CONNECTING));

		if (IsSessionAdmin(Data,sSession)) 
			buffer+=" (<a href=\"?ses=" + sSession + "&amp;w=kad&amp;c=disconnect\">"+_GetPlainResString(IDS_MAIN_BTN_DISCONNECT)+"</a>)";
	}
	else if (Kademlia::CKademlia::isFirewalled()) {
		buffer.Append(_GetPlainResString(IDS_FIREWALLED));
		if (IsSessionAdmin(Data,sSession)) 
			buffer+=" (<a href=\"?ses=" + sSession + "&amp;w=kad&amp;c=disconnect\">"+_GetPlainResString(IDS_IRC_DISCONNECT)+"</a>)";
	}
	else if (Kademlia::CKademlia::isConnected()) {
		buffer.Append(_GetPlainResString(IDS_CONNECTED));
		if (IsSessionAdmin(Data,sSession)) 
			buffer+=" (<a href=\"?ses=" + sSession + "&amp;w=kad&amp;c=disconnect\">"+_GetPlainResString(IDS_IRC_DISCONNECT)+"</a>)";
	}

	Out.Replace("[KademliaInfo]",buffer);

	return Out;
}


CString CWapServer::_GetServerList(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString sCmd = _ParseURL(Data.sURL, "c");

	if (sCmd == "options"){
		CString sAddServerBox = _GetServerOptions(Data);
		sAddServerBox.Replace("[Session]", sSession);
		return sAddServerBox;
	}
	else{
		if (sCmd == "connect" && IsSessionAdmin(Data,sSession) )
		{
			_ConnectToServer(_ParseURL(Data.sURL, "ip"),atoi(_ParseURL(Data.sURL, "port")));
		}	
		else if (sCmd == "disconnect" && IsSessionAdmin(Data,sSession)) 
		{
			theApp.emuledlg->SendMessage(WEB_DISCONNECT_SERVER, NULL);
		}	
		
		if (_ParseURL(Data.sURL, "showlist")=="true"){
			if (sCmd == "remove" && IsSessionAdmin(Data,sSession)) 
			{
				CString sIP = _ParseURL(Data.sURL, "ip");
				int nPort = atoi(_ParseURL(Data.sURL, "port"));
				if(!sIP.IsEmpty())
					_RemoveServer(sIP,nPort);
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
			//Out.Replace("[AddServerBox]", sAddServerBox);
			Out.Replace("[Session]", sSession);
			int sortpos=0;
			if(pThis->m_Params.ServerSort == SERVER_SORT_NAME){
				Out.Replace("[SortName]", "./?ses=[Session]&amp;w=server&amp;sort=name&amp;showlist=true&amp;sortreverse=" + sServerSortRev);
				sortpos=1;

				CString strSort=_GetPlainResString(IDS_SL_SERVERNAME);
				if (pThis->m_Params.bServerSortReverse)
					strSort+=" (-)";
				else
					strSort+=" (+)";
				Out.Replace("[Servername]", strSort);
			}else
				Out.Replace("[SortName]", "./?ses=[Session]&amp;w=server&amp;sort=name&amp;showlist=true");
			if(pThis->m_Params.ServerSort == SERVER_SORT_DESCRIPTION){
				Out.Replace("[SortDescription]", "./?ses=[Session]&amp;w=server&amp;sort=description&amp;showlist=true&amp;sortreverse=" + sServerSortRev);
				sortpos=2;

				CString strSort=_GetPlainResString(IDS_DESCRIPTION);
				if (pThis->m_Params.bServerSortReverse)
					strSort+=" (-)";
				else
					strSort+=" (+)";
				Out.Replace("[Description]", strSort);
			}else
				Out.Replace("[SortDescription]", "./?ses=[Session]&amp;w=server&amp;sort=description&amp;showlist=true");
			if(pThis->m_Params.ServerSort == SERVER_SORT_IP){
				Out.Replace("[SortIP]", "./?ses=[Session]&amp;w=server&amp;sort=ip&amp;showlist=true&amp;sortreverse=" + sServerSortRev);
				sortpos=3;

				CString strSort=_GetPlainResString(IDS_IP);
				if (pThis->m_Params.bServerSortReverse)
					strSort+=" (-)";
				else
					strSort+=" (+)";
				Out.Replace("[Address]", strSort);
			}else
				Out.Replace("[SortIP]", "./?ses=[Session]&amp;w=server&amp;sort=ip&amp;showlist=true");
			if(pThis->m_Params.ServerSort == SERVER_SORT_USERS){
				Out.Replace("[SortUsers]", "./?ses=[Session]&amp;w=server&amp;sort=users&amp;showlist=true&amp;sortreverse=" + sServerSortRev);
				sortpos=4;

				CString strSort=_GetPlainResString(IDS_LUSERS);
				if (pThis->m_Params.bServerSortReverse)
					strSort+=" (-)";
				else
					strSort+=" (+)";
				Out.Replace("[Users]", strSort);
			}else
				Out.Replace("[SortUsers]", "./?ses=[Session]&amp;w=server&amp;sort=users&amp;showlist=true");
			if(pThis->m_Params.ServerSort == SERVER_SORT_FILES){
				Out.Replace("[SortFiles]", "./?ses=[Session]&amp;w=server&amp;sort=files&amp;showlist=true&amp;sortreverse=" + sServerSortRev);
				sortpos=5;

				CString strSort=_GetPlainResString(IDS_LFILES);
				if (pThis->m_Params.bServerSortReverse)
					strSort+=" (-)";
				else
					strSort+=" (+)";
				Out.Replace("[Files]", strSort);
			}else
				Out.Replace("[SortFiles]", "./?ses=[Session]&amp;w=server&amp;sort=files&amp;showlist=true");

			//[SortByVal:x1,x2,x3,...]
			int sortbypos=Out.Find("[SortByVal:",0);
			if(sortbypos!=-1){
				int sortbypos2=Out.Find("]",sortbypos+1);
				if(sortbypos2!=-1){
					CString strSortByArray=Out.Mid(sortbypos+11,sortbypos2-sortbypos-11);
					CString resToken;
					int curPos = 0,curPos2=0;
					bool posfound=false;
					resToken= strSortByArray.Tokenize(",",curPos);
					while (resToken != "" && !posfound)
					{
						curPos2++;
						if(sortpos==atoi(resToken)){
							CString strTemp;
							strTemp.Format("%i",curPos2);
							Out.Replace(Out.Mid(sortbypos,sortbypos2-sortbypos+1),strTemp);
							posfound=true;
						}
						resToken= strSortByArray.Tokenize(",",curPos);
					};
					if(!posfound)
						Out.Replace(Out.Mid(sortbypos,sortbypos2-sortbypos+1),"1");
				}
			}

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
			int startpos=0,endpos;

			if(_ParseURL(Data.sURL, "startpos")!="")
				startpos=atoi(_ParseURL(Data.sURL, "startpos"));

			endpos = startpos + thePrefs.GetWapMaxItemsInPages();

			if (endpos>ServerArray.GetCount())
				endpos=ServerArray.GetCount();

			if (startpos>ServerArray.GetCount()){
				startpos=endpos-thePrefs.GetWapMaxItemsInPages();
				if (startpos<0) startpos=0;
			}

			for(int i = startpos; i < endpos; i++)
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

				HTTPProcessData.Replace("[6]", IsSessionAdmin(Data,sSession)? CString("./?ses=" + sSession + "&amp;w=server&amp;c=connect&amp;ip=" + ServerArray[i].sServerIP+"&amp;port="+sServerPort):GetPermissionDenied());
				HTTPProcessData.Replace("[LinkRemove]", IsSessionAdmin(Data,sSession)?CString("./?ses=" + sSession + "&amp;w=server&amp;c=remove&amp;ip=" + ServerArray[i].sServerIP+"&amp;port="+sServerPort+"&amp;showlist=true"):GetPermissionDenied());

				sList += HTTPProcessData;
			}
			
			//Navigation Line
			CString strNavLine,strNavUrl;
			int pos,pos2;
			strNavUrl = Data.sURL;
			if((pos=strNavUrl.Find("startpos=",0))>1){
				if (strNavUrl.Mid(pos-1,1)=="&") pos--;
				pos2=strNavUrl.Find("&",pos+1);
				if (pos2==-1) pos2 = strNavUrl.GetLength();
				strNavUrl = strNavUrl.Left(pos) + strNavUrl.Right(strNavUrl.GetLength()-pos2);
			}	
			strNavUrl=_SpecialChars(strNavUrl);

			if(startpos>0){
				startpos-=thePrefs.GetWapMaxItemsInPages();
				if (startpos<0) startpos = 0;
				strNavLine.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_PREV) + "</a>&nbsp;", startpos);
			}
			if(endpos<ServerArray.GetCount()){
				CString strTemp;
				strTemp.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_NEXT) + "</a> ", endpos);
				strNavLine += strTemp;
			}
			Out.Replace("[NavigationLine]", strNavLine);
			Out.Replace("[ServersList]", sList);

			Out.Replace("[ServerList]", _GetPlainResString(IDS_SV_SERVERLIST));
			Out.Replace("[Servername]", _GetPlainResString(IDS_SL_SERVERNAME));
			Out.Replace("[Description]", _GetPlainResString(IDS_DESCRIPTION));
			Out.Replace("[Address]", _GetPlainResString(IDS_IP));
			Out.Replace("[Connect]", _GetPlainResString(IDS_IRC_CONNECT));
			Out.Replace("[Users]", _GetPlainResString(IDS_LUSERS));
			Out.Replace("[Files]", _GetPlainResString(IDS_LFILES));
			Out.Replace("[Actions]", _GetPlainResString(IDS_WEB_ACTIONS));
			Out.Replace("[Session]", sSession);
			Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
			Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
			Out.Replace("[SortBy]", _GetPlainResString(IDS_WAP_SORTBY));

			return Out;
		} else {
			CString Out = pThis->m_Templates.sServerMain;
			Out.Replace("[ServerList]", _GetPlainResString(IDS_SV_SERVERLIST));
			Out.Replace("[ConnectedServerData]", _GetConnectedServer(Data));
			Out.Replace("[Session]", sSession);
			Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
			Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
			Out.Replace("[Refresh]", _GetPlainResString(IDS_WAP_REFRESH));

			return Out;
		}
	}
}

CString CWapServer::_GetTransferDownList(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");
	int cat=atoi(_ParseURL(Data.sURL,"cat"));

	bool clcompl=(_ParseURL(Data.sURL,"ClCompl")=="yes" );
	CString sCat; (cat!=0)?sCat.Format("&amp;cat=%i",cat):sCat="";

	CString Out = "";

	if (clcompl && IsSessionAdmin(Data,sSession)) 
		theApp.emuledlg->transferwnd->downloadlistctrl.ClearCompleted(true);

	if (_ParseURL(Data.sURL, "c") != "" && IsSessionAdmin(Data,sSession)) 
	{
		CString HTTPTemp = _ParseURL(Data.sURL, "c");
		theApp.AddEd2kLinksToDownload(HTTPTemp,(uint8)cat);
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
	CString sDownloadSortRev;
	if(pThis->m_Params.bDownloadSortReverse)
		sDownloadSortRev = "false";
	else
		sDownloadSortRev = "true";

	CPartFile* found_file = NULL;

	if(_ParseURL(Data.sURL, "showfile")!=""){
		uchar FileHash[16];
		_GetFileHash(_ParseURL(Data.sURL, "showfile"), FileHash);

		CArray<CPartFile*,CPartFile*> partlist;
		theApp.emuledlg->transferwnd->downloadlistctrl.GetDisplayedFiles(&partlist);
		
		for (int fx=0;fx<partlist.GetCount();fx++)
		{
			CPartFile* cur_file =  partlist.GetAt(fx);

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

		if(found_file){
			Out += pThis->m_Templates.sTransferDownFileDetails;

			CString JSfileinfo=_SpecialChars(found_file->GetInfoSummary(found_file));
			JSfileinfo.Replace("\n","\\n");
			CString sActions = found_file->getPartfileStatus() + "<br/>";
			CString sED2kLink;
			CString strFileSize;
			CString strCleanFileName;
			CString strFileHash;
			CString strFileName;
			CString HTTPTemp;

			strFileName = _SpecialChars(found_file->GetFileName());
			strFileHash = EncodeBase16(found_file->GetFileHash(), 16);
			strFileSize.Format("%ul",found_file->GetFileSize());
			
			strCleanFileName = RemoveWMLScriptInvalidChars(found_file->GetFileName());
			for(int pos=0; pos<strCleanFileName.GetLength(); pos++){
				unsigned char c;
				c = strCleanFileName.GetAt(pos);
				if(!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))){
					c = '.';
				}
				strCleanFileName.SetAt(pos, c);
			}

			sED2kLink.Format("<anchor>[Ed2klink]<go href=\"./script.wmls#showed2k('"+ strCleanFileName  + "','" + strFileSize + "','" + strFileHash + "')\"/></anchor><br/>");
			sED2kLink.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
			sActions += sED2kLink;

			bool bCanBeDeleted = true;
			switch(found_file->GetStatus())
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
						sResume.Format("<a href=\"[Link]\">[Resume]</a><br />");
						sResume.Replace("[Link]", CString("./?ses=" + sSession + "&amp;w=transferdown&amp;op=resume&amp;file=" + strFileHash + sCat + "&amp;showfile=" + strFileHash )) ;
						sActions += sResume;
					}
				}
					break; 
				default: // waiting or downloading
				{
					if (IsSessionAdmin(Data,sSession)) {
						CString sPause;
						sPause.Format("<a href=\"[Link]\">[Pause]</a><br />");
						sPause.Replace("[Link]", CString("./?ses=" + sSession + "&amp;w=transferdown&amp;op=pause&amp;file=" + strFileHash + sCat + "&amp;showfile=" + strFileHash));
						sActions += sPause;
					}
				}
				break;
			}
			if(bCanBeDeleted)
			{
				if (IsSessionAdmin(Data,sSession)) {
					CString sCancel;
					sCancel.Format("<a href=\"./script.wmls#confirmcancel('[Link]')\">[Cancel]</a><br />");
					sCancel.Replace("[Link]", CString("./?ses=" + sSession + "&amp;w=transferdown&amp;op=cancel&amp;file=" + strFileHash + sCat ) );
					sActions += sCancel;
				}
			}

			if (IsSessionAdmin(Data,sSession)) {
				sActions.Replace("[Resume]", _GetPlainResString(IDS_DL_RESUME));
				sActions.Replace("[Pause]", _GetPlainResString(IDS_DL_PAUSE));
				sActions.Replace("[Cancel]", _GetPlainResString(IDS_MAIN_BTN_CANCEL));
				sActions.Replace("[ConfirmCancel]", _GetPlainResStringNoQuote(IDS_Q_CANCELDL2));
				
				if (found_file->GetStatus()!=PS_COMPLETE && found_file->GetStatus()!=PS_COMPLETING) {
					sActions.Append("<a href=\"./?ses=[Session]&amp;w=transferdown&amp;op=prioup&amp;file=" + strFileHash + sCat + "&amp;showfile=" + strFileHash + "\">[PriorityUp]</a><br/>");
					sActions.Append("<a href=\"./?ses=[Session]&amp;w=transferdown&amp;op=priodown&amp;file=" + strFileHash + sCat + "&amp;showfile=" + strFileHash + "\">[PriorityDown]</a><br/>");
				}
			}

			Out.Replace("[FileName]", strFileName);

			if(strFileName.GetLength() > SHORT_FILENAME_LENGTH){
				Out.Replace("[ShortFileName]", strFileName.Left(SHORT_FILENAME_LENGTH) + "...");
			}else{
				Out.Replace("[ShortFileName]", strFileName);
			}
			Out.Replace("[FileInfo]", _SpecialChars(found_file->GetInfoSummary(found_file)));

			Out.Replace("[2]", CastItoXBytes(found_file->GetFileSize()));

			if(found_file->GetCompletedSize() > 0)
			{
				Out.Replace("[3]", CastItoXBytes(found_file->GetCompletedSize()));
			}
			else{
				Out.Replace("[3]", "-");
			}

			Out.Replace("[DownloadBar]", _GetDownloadGraph(Data,strFileHash));

			if(found_file->GetDatarate() > 0.0f)
			{
				HTTPTemp.Format("%8.2f %s", found_file->GetDatarate()/1024.0 ,_GetPlainResString(IDS_KBYTESEC) );
				Out.Replace("[4]", HTTPTemp);
			}
			else{
				Out.Replace("[4]", "-");
			}
			if(found_file->GetSourceCount() > 0)
			{
				HTTPTemp.Format("%i&nbsp;/&nbsp;%8i&nbsp;(%i)",
					found_file->GetSourceCount()-found_file->GetNotCurrentSourcesCount(),
					found_file->GetSourceCount(),
					found_file->GetTransferingSrcCount());

				Out.Replace("[5]", HTTPTemp);
			}
			else{
				Out.Replace("[5]", "-");
			}

			switch(found_file->GetDownPriority()) {
				case 0: HTTPTemp=GetResString(IDS_PRIOLOW);break;
				case 10: HTTPTemp=GetResString(IDS_PRIOAUTOLOW);break;

				case 1: HTTPTemp=GetResString(IDS_PRIONORMAL);break;
				case 11: HTTPTemp=GetResString(IDS_PRIOAUTONORMAL);break;

				case 2: HTTPTemp=GetResString(IDS_PRIOHIGH);break;
				case 12: HTTPTemp=GetResString(IDS_PRIOAUTOHIGH);break;
			}

			Out.Replace("[PrioVal]", HTTPTemp);
			Out.Replace("[6]", sActions);

			HTTPTemp.Format("%.1f%%",found_file->GetPercentCompleted());
			Out.Replace("[FileProgress]", HTTPTemp);

			Out.Replace("[FileHash]",strFileHash);
		}
	}

	if (!found_file){
		Out += pThis->m_Templates.sTransferDownList;

		InsertCatBox(Out,cat,_GetPlainResString(IDS_SELECTCAT)+":&nbsp;<br/>",true,true);
		Out.Replace("[URL]", _SpecialChars("." + Data.sURL));

		int sortpos=0;
		if(pThis->m_Params.DownloadSort == DOWN_SORT_NAME){
			Out.Replace("[SortName]", "./?ses=[Session]&amp;w=transferdown&amp;sort=name&amp;sortreverse=" + sDownloadSortRev);
			sortpos=1;
			CString strSort=_GetPlainResString(IDS_DL_FILENAME);
			if (pThis->m_Params.bDownloadSortReverse)
				strSort+=" (-)";
			else
				strSort+=" (+)";
			Out.Replace("[Filename]", strSort);
		}else
			Out.Replace("[SortName]", "./?ses=[Session]&amp;w=transferdown&amp;sort=name");
		if(pThis->m_Params.DownloadSort == DOWN_SORT_SIZE){
			Out.Replace("[SortSize]", "./?ses=[Session]&amp;w=transferdown&amp;sort=size&amp;sortreverse=" + sDownloadSortRev);
			sortpos=2;
			CString strSort=_GetPlainResString(IDS_DL_SIZE);
			if (pThis->m_Params.bDownloadSortReverse)
				strSort+=" (-)";
			else
				strSort+=" (+)";
			Out.Replace("[Size]", strSort);
		}else
			Out.Replace("[SortSize]", "./?ses=[Session]&amp;w=transferdown&amp;sort=size");
		if(pThis->m_Params.DownloadSort == DOWN_SORT_TRANSFERRED){
			Out.Replace("[SortTransferred]", "./?ses=[Session]&amp;w=transferdown&amp;sort=transferred&amp;sortreverse=" + sDownloadSortRev);
			sortpos=3;
			CString strSort=_GetPlainResString(IDS_COMPLETE);
			if (pThis->m_Params.bDownloadSortReverse)
				strSort+=" (-)";
			else
				strSort+=" (+)";
			Out.Replace("[Transferred]", strSort);
		}else
			Out.Replace("[SortTransferred]", "./?ses=[Session]&amp;w=transferdown&amp;sort=transferred");
		if(pThis->m_Params.DownloadSort == DOWN_SORT_SPEED){
			Out.Replace("[SortSpeed]", "./?ses=[Session]&amp;w=transferdown&amp;sort=speed&amp;sortreverse=" + sDownloadSortRev);
			sortpos=4;
			CString strSort=_GetPlainResString(IDS_DL_SPEED);
			if (pThis->m_Params.bDownloadSortReverse)
				strSort+=" (-)";
			else
				strSort+=" (+)";
			Out.Replace("[Speed]", strSort);
		}else
			Out.Replace("[SortSpeed]", "./?ses=[Session]&amp;w=transferdown&amp;sort=speed");
		if(pThis->m_Params.DownloadSort == DOWN_SORT_PROGRESS){
			Out.Replace("[SortProgress]", "./?ses=[Session]&amp;w=transferdown&amp;sort=progress&amp;sortreverse=" + sDownloadSortRev);
			sortpos=5;
			CString strSort=_GetPlainResString(IDS_DL_PROGRESS);
			if (pThis->m_Params.bDownloadSortReverse)
				strSort+=" (-)";
			else
				strSort+=" (+)";
			Out.Replace("[Progress]", strSort);
		}else
			Out.Replace("[SortProgress]", "./?ses=[Session]&amp;w=transferdown&amp;sort=progress");

		//[SortByVal:x1,x2,x3,...]
		int sortbypos=Out.Find("[SortByVal:",0);
		if(sortbypos!=-1){
			int sortbypos2=Out.Find("]",sortbypos+1);
			if(sortbypos2!=-1){
				CString strSortByArray=Out.Mid(sortbypos+11,sortbypos2-sortbypos-11);
				CString resToken;
				int curPos = 0,curPos2=0;
				bool posfound=false;
				resToken= strSortByArray.Tokenize(",",curPos);
				while (resToken != "" && !posfound)
				{
					curPos2++;
					if(sortpos==atoi(resToken)){
						CString strTemp;
						strTemp.Format("%i",curPos2);
						Out.Replace(Out.Mid(sortbypos,sortbypos2-sortbypos+1),strTemp);
						posfound=true;
					}
					resToken= strSortByArray.Tokenize(",",curPos);
				};
				if(!posfound)
					Out.Replace(Out.Mid(sortbypos,sortbypos2-sortbypos+1),"1");
			}
		}

		CString OutE = pThis->m_Templates.sTransferDownLineLite;
		CString OutE2 = pThis->m_Templates.sTransferDownLineLiteGood;

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
				if (theApp.serverconnect->IsConnected() && !theApp.serverconnect->IsLowID())
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
		int startpos=0,endpos;

		if(_ParseURL(Data.sURL, "startpos")!="")
			startpos=atoi(_ParseURL(Data.sURL, "startpos"));

		endpos = startpos + thePrefs.GetWapMaxItemsInPages();

		if (endpos>FilesArray.GetCount())
			endpos=FilesArray.GetCount();

		if (startpos>FilesArray.GetCount()){
			startpos=endpos-thePrefs.GetWapMaxItemsInPages();
			if (startpos<0) startpos=0;
		}

		for(int i = startpos; i < endpos; i++)
		{
			CString JSfileinfo=FilesArray[i].sFileInfo;
			JSfileinfo.Replace("\n","\\n");
			CString sActions = FilesArray[i].sFileStatus + "<br /> ";
			CString sED2kLink;
			CString strFileSize;
			CString strCleanFileName;

			strFileSize.Format("%ul",FilesArray[i].lFileSize);
			strCleanFileName = _SpecialChars(FilesArray[i].sFileName);
			strCleanFileName.Replace(" ",".");
			sED2kLink.Format("<anchor>[Ed2klink]<go href=\"./script.wmls#showed2k('"+ strCleanFileName  + "','" + strFileSize + "','" + FilesArray[i].sFileHash + "')\"/></anchor><br />");
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
						sResume.Format("<a href=\"[Link]\">[Resume]</a><br />");
						sResume.Replace("[Link]", CString("./?ses=" + sSession + "&amp;w=transferdown&amp;op=resume&amp;file=" + FilesArray[i].sFileHash +sCat )) ;
						sActions += sResume;
					}
				}
					break; 
				default: // waiting or downloading
				{
					if (IsSessionAdmin(Data,sSession)) {
						CString sPause;
						sPause.Format("<a href=\"[Link]\">[Pause]</a><br />");
						sPause.Replace("[Link]", CString("./?ses=" + sSession + "&amp;w=transferdown&amp;op=pause&amp;file=" + FilesArray[i].sFileHash+sCat ));
						sActions += sPause;
					}
				}
				break;
			}
			if(bCanBeDeleted)
			{
				if (IsSessionAdmin(Data,sSession)) {
					CString sCancel;
					sCancel.Format("<a href=\"./script.wmls#confirmcancel('[Link]')\">[Cancel]</a><br />");
					sCancel.Replace("[Link]", CString("./?ses=" + sSession + "&amp;w=transferdown&amp;op=cancel&amp;file=" + FilesArray[i].sFileHash+sCat ) );
					sActions += sCancel;
				}
			}

			if (IsSessionAdmin(Data,sSession)) {
				sActions.Replace("[Resume]", _GetPlainResString(IDS_DL_RESUME));
				sActions.Replace("[Pause]", _GetPlainResString(IDS_DL_PAUSE));
				sActions.Replace("[Cancel]", _GetPlainResString(IDS_MAIN_BTN_CANCEL));
				sActions.Replace("[ConfirmCancel]", _GetPlainResStringNoQuote(IDS_Q_CANCELDL2));
				
				if (FilesArray[i].nFileStatus!=PS_COMPLETE && FilesArray[i].nFileStatus!=PS_COMPLETING) {
					sActions.Append("<a href=\"./?ses=[Session]&amp;w=transferdown&amp;op=prioup&amp;file=" + FilesArray[i].sFileHash+sCat+"\">[PriorityUp]</a><br />");
					sActions.Append("<a href=\"./?ses=[Session]&amp;w=transferdown&amp;op=priodown&amp;file=" + FilesArray[i].sFileHash+sCat +"\">[PriorityDown]</a><br />");
				}
			}

			CString HTTPProcessData;
			// if downloading, draw in other color
			if(FilesArray[i].lFileSpeed > 0)
				HTTPProcessData = OutE2;
			else
				HTTPProcessData = OutE;

			CString sFileNumber; sFileNumber.Format("%d",i+1);
			HTTPProcessData.Replace("[FileNumber]", sFileNumber);

			HTTPProcessData.Replace("[FileName]", FilesArray[i].sFileName);

			if(FilesArray[i].sFileName.GetLength() > SHORT_FILENAME_LENGTH){
				HTTPProcessData.Replace("[ShortFileName]", FilesArray[i].sFileName.Left(SHORT_FILENAME_LENGTH) + "...");
			}else{
				HTTPProcessData.Replace("[ShortFileName]", FilesArray[i].sFileName);
			}
			HTTPProcessData.Replace("[FileInfo]", FilesArray[i].sFileInfo);

			fTotalSize += FilesArray[i].lFileSize;

			HTTPProcessData.Replace("[2]", CastItoXBytes(FilesArray[i].lFileSize));

			if(FilesArray[i].lFileTransferred > 0)
			{
				fTotalTransferred += FilesArray[i].lFileTransferred;

				HTTPProcessData.Replace("[3]", CastItoXBytes(FilesArray[i].lFileTransferred));
			}
			else{
				HTTPProcessData.Replace("[3]", "-");
			}

			HTTPProcessData.Replace("[DownloadBar]", _GetDownloadGraph(Data,FilesArray[i].sFileHash));

			if(FilesArray[i].lFileSpeed > 0.0f)
			{
				fTotalSpeed += FilesArray[i].lFileSpeed;

				HTTPTemp.Format("%8.2f %s", FilesArray[i].lFileSpeed/1024.0 ,_GetPlainResString(IDS_KBYTESEC) );
				HTTPProcessData.Replace("[4]", HTTPTemp);
			}
			else{
				HTTPProcessData.Replace("[4]", "-");
			}
			if(FilesArray[i].lSourceCount > 0)
			{
				HTTPTemp.Format("%i&nbsp;/&nbsp;%8i&nbsp;(%i)",
					FilesArray[i].lSourceCount-FilesArray[i].lNotCurrentSourceCount,
					FilesArray[i].lSourceCount,
					FilesArray[i].lTransferringSourceCount);

				HTTPProcessData.Replace("[5]", HTTPTemp);
			}
			else{
				HTTPProcessData.Replace("[5]", "-");
			}

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

			HTTPTemp.Format("%.1f%%",FilesArray[i].fCompleted);
			HTTPProcessData.Replace("[FileProgress]", HTTPTemp);

			HTTPProcessData.Replace("[FileHash]",FilesArray[i].sFileHash);

			sDownList += HTTPProcessData;
		}

		//Navigation Line
		CString strNavLine,strNavUrl;
		int pos,pos2;
		strNavUrl = Data.sURL;
		if((pos=strNavUrl.Find("startpos=",0))>1){
			if (strNavUrl.Mid(pos-1,1)=="&") pos--;
			pos2=strNavUrl.Find("&",pos+1);
			if (pos2==-1) pos2 = strNavUrl.GetLength();
			strNavUrl = strNavUrl.Left(pos) + strNavUrl.Right(strNavUrl.GetLength()-pos2);
		}	
		strNavUrl=_SpecialChars(strNavUrl);

		if(startpos>0){
			startpos-=thePrefs.GetWapMaxItemsInPages();
			if (startpos<0) startpos = 0;
			strNavLine.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_PREV) + "</a>&nbsp;", startpos);
		}
		if(endpos<FilesArray.GetCount()){
			CString strTemp;
			strTemp.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_NEXT) + "</a> ", endpos);
			strNavLine += strTemp;
		}
		Out.Replace("[NavigationLine]", strNavLine);

		Out.Replace("[DownloadFilesListLite]", sDownList);

		// Elandal: cast from float to integral type always drops fractions.
		// avoids implicit cast warning
		Out.Replace("[TotalDownSize]", CastItoXBytes((uint64)fTotalSize));
		Out.Replace("[TotalDownTransferred]", CastItoXBytes((uint64)fTotalTransferred));

		Out.Replace("[ClearCompletedButton]",(completedAv && IsSessionAdmin(Data,sSession))?pThis->m_Templates.sClearCompleted :"");

		HTTPTemp.Format("%8.2f %s", fTotalSpeed/1024.0,_GetPlainResString(IDS_KBYTESEC));
		Out.Replace("[TotalDownSpeed]", HTTPTemp);
		
		Out.Replace("[CLEARCOMPLETED]",_GetPlainResString(IDS_DL_CLEAR));

		CString buffer;
		buffer.Format("%s (%i)", _GetPlainResString(IDS_TW_DOWNLOADS),FilesArray.GetCount());
		Out.Replace("[DownloadList]",buffer);
		Out.Replace("[CatSel]",sCat);
		
	}

	// modified by Announ [itsonlyme: -modname-]
	/*
	Out.Replace("[version]", _SpecialChars(theApp.m_strCurVersionLong));
	*/
	Out.Replace("[version]", _SpecialChars(theApp.m_strCurVersionLong)+CString(" ")+theApp.m_strModLongVersion);
	// End -modname-
	Out.Replace("[eMuleAppName]", "eMule");
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
	Out.Replace("[PriorityUp]", _GetPlainResString(IDS_PRIORITY_UP));
	Out.Replace("[PriorityDown]", _GetPlainResString(IDS_PRIORITY_DOWN));

	Out.Replace("[Filename]", _GetPlainResString(IDS_DL_FILENAME));
	Out.Replace("[Size]", _GetPlainResString(IDS_DL_SIZE));
	Out.Replace("[Transferred]", _GetPlainResString(IDS_COMPLETE));
	Out.Replace("[Progress]", _GetPlainResString(IDS_DL_PROGRESS));
	Out.Replace("[Speed]", _GetPlainResString(IDS_DL_SPEED));
	Out.Replace("[Sources]", _GetPlainResString(IDS_DL_SOURCES));
	Out.Replace("[Actions]", _GetPlainResString(IDS_WEB_ACTIONS));
	Out.Replace("[User]", _GetPlainResString(IDS_QL_USERNAME));
	Out.Replace("[TotalDown]", _GetPlainResString(IDS_INFLST_USER_TOTALDOWNLOAD));
	Out.Replace("[Prio]", _GetPlainResString(IDS_PRIORITY));
	Out.Replace("[CatSel]",sCat);
	Out.Replace("[Session]", sSession);
	Out.Replace("[Section]", _GetPlainResString(IDS_TW_DOWNLOADS));
	Out.Replace("[SortBy]", _GetPlainResString(IDS_WAP_SORTBY));

	return Out;
}

CString CWapServer::_GetTransferUpList(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = "", OutE = "";

	Out += pThis->m_Templates.sTransferUpList;

	// modified by Announ [itsonlyme: -modname-]
	/*
	Out.Replace("[version]", _SpecialChars(theApp.m_strCurVersionLong));
	*/
	Out.Replace("[version]", _SpecialChars(theApp.m_strCurVersionLong)+CString(" ")+theApp.m_strModLongVersion);
	// End -modname-
	Out.Replace("[eMuleAppName]", "eMule");
	Out.Replace("[Section]", _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
	
	OutE = pThis->m_Templates.sTransferUpLine;

	float fTotalSize = 0, fTotalTransferred = 0, fTotalSpeed = 0;

	CString sUpList = "";
	int startpos=0,endpos;

	if(_ParseURL(Data.sURL, "startpos")!="")
		startpos=atoi(_ParseURL(Data.sURL, "startpos"));

	endpos = startpos + thePrefs.GetWapMaxItemsInPages();

	if (endpos>theApp.uploadqueue->GetUploadQueueLength())
		endpos=theApp.uploadqueue->GetUploadQueueLength();

	if (startpos>theApp.uploadqueue->GetUploadQueueLength()){
		startpos=endpos-thePrefs.GetWapMaxItemsInPages();
		if (startpos<0) startpos=0;
	}

	int fCount = 0;
	for (POSITION pos = theApp.uploadqueue->GetFirstFromUploadList();
		pos != 0 && fCount < endpos;theApp.uploadqueue->GetNextFromUploadList(pos))
	{
		if(fCount >= startpos){
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
		fCount++;
	}

	//Navigation Line
	CString strNavLine,strNavUrl;
	int position,position2;
	strNavUrl = Data.sURL;
	if((position=strNavUrl.Find("startpos=",0))>1){
		if (strNavUrl.Mid(position-1,1)=="&") position--;
		position2=strNavUrl.Find("&",position+1);
		if (position2==-1) position2 = strNavUrl.GetLength();
		strNavUrl = strNavUrl.Left(position) + strNavUrl.Right(strNavUrl.GetLength()-position2);
	}	

	strNavUrl=_SpecialChars(strNavUrl);

	if(startpos>0){
		startpos-=thePrefs.GetWapMaxItemsInPages();
		if (startpos<0) startpos = 0;
		strNavLine.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_PREV) + "</a>&nbsp;", startpos);
	}
	if(endpos<theApp.uploadqueue->GetUploadQueueLength()){
		CString strTemp;
		strTemp.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_NEXT) + "</a> ", endpos);
		strNavLine += strTemp;
	}
	Out.Replace("[NavigationLine]", strNavLine);

	Out.Replace("[UploadFilesList]", sUpList);
	// Elandal: cast from float to integral type always drops fractions.
	// avoids implicit cast warning
	CString HTTPTemp;
	HTTPTemp.Format("%s / %s", CastItoXBytes((uint64)fTotalSize), CastItoXBytes((uint64)fTotalTransferred));
	Out.Replace("[TotalUpTransferred]", HTTPTemp);
	HTTPTemp.Format("%8.2f " + _GetPlainResString(IDS_KBYTESEC), fTotalSpeed/1024.0);
	Out.Replace("[TotalUpSpeed]", HTTPTemp);

	Out.Replace("[Session]", sSession);

	CString buffer;
	buffer.Format("%s (%i)",_GetPlainResString(IDS_PW_CON_UPLBL),theApp.uploadqueue->GetUploadQueueLength());
	Out.Replace("[UploadList]", buffer );
	
	Out.Replace("[Filename]", _GetPlainResString(IDS_DL_FILENAME));
	Out.Replace("[Transferred]", _GetPlainResString(IDS_COMPLETE));
	Out.Replace("[Speed]", _GetPlainResString(IDS_DL_SPEED));
	Out.Replace("[User]", _GetPlainResString(IDS_QL_USERNAME));
	Out.Replace("[TotalUp]", _GetPlainResString(IDS_INFLST_USER_TOTALUPLOAD));

	return Out;
}

CString CWapServer::_GetTransferQueueList(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = "", OutE = "";

	Out=pThis->m_Templates.sTransferUpQueueList;

	// modified by Announ [itsonlyme: -modname-]
	/*
	Out.Replace("[version]", _SpecialChars(theApp.m_strCurVersionLong));
	*/
	Out.Replace("[version]", _SpecialChars(theApp.m_strCurVersionLong)+CString(" ")+theApp.m_strModLongVersion);
	// End -modname-
	Out.Replace("[eMuleAppName]", "eMule");
	Out.Replace("[Section]", _GetPlainResString(IDS_ONQUEUE));
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));

	CString buffer;
	buffer.Format("%s (%i)",_GetPlainResString(IDS_ONQUEUE),theApp.uploadqueue->GetWaitingUserCount());
	Out.Replace("[UploadQueueList]", buffer );

	OutE = pThis->m_Templates.sTransferUpQueueLine;
	// Replace [xx]
	CString sQueue = "";

	float fTotalSize = 0, fTotalTransferred = 0, fTotalSpeed = 0;

	int startpos=0,endpos;

	if(_ParseURL(Data.sURL, "startpos")!="")
		startpos=atoi(_ParseURL(Data.sURL, "startpos"));

	endpos = startpos + thePrefs.GetWapMaxItemsInPages();

	if (endpos>theApp.uploadqueue->GetWaitingUserCount())
		endpos=theApp.uploadqueue->GetWaitingUserCount();

	if (startpos>theApp.uploadqueue->GetWaitingUserCount()){
		startpos=endpos-thePrefs.GetWapMaxItemsInPages();
		if (startpos<0) startpos=0;
	}

	int fCount = 0;
	for (POSITION pos = theApp.uploadqueue->GetFirstFromWaitingList(); pos != 0 && fCount<endpos ;theApp.uploadqueue->GetNextFromWaitingList(pos)){
		if(fCount>=startpos){
			CUpDownClient* cur_client = theApp.uploadqueue->GetWaitClientAt(pos);
			CString HTTPProcessData;
			char HTTPTempC[100] = "";
			HTTPProcessData = OutE;
			HTTPProcessData.Replace("[UserName]", _SpecialChars(cur_client->GetUserName()));
			CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID());
			if (file)
				HTTPProcessData.Replace("[FileName]", _SpecialChars(file->GetFileName()));
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
		fCount++;
	}
	//Navigation Line
	CString strNavLine,strNavUrl;
	int position,position2;
	strNavUrl = Data.sURL;
	if((position=strNavUrl.Find("startpos=",0))>1){
		if (strNavUrl.Mid(position-1,1)=="&") position--;
		position2=strNavUrl.Find("&",position+1);
		if (position2==-1) position2 = strNavUrl.GetLength();
		strNavUrl = strNavUrl.Left(position) + strNavUrl.Right(strNavUrl.GetLength()-position2);
	}	
	strNavUrl=_SpecialChars(strNavUrl);

	if(startpos>0){
		startpos-=thePrefs.GetWapMaxItemsInPages();
		if (startpos<0) startpos = 0;
		strNavLine.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_PREV) + "</a>&nbsp;", startpos);
	}
	if(endpos<theApp.uploadqueue->GetWaitingUserCount()){
		CString strTemp;
		strTemp.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_NEXT) + "</a> ", endpos);
		strNavLine += strTemp;
	}
	Out.Replace("[NavigationLine]", strNavLine);

	Out.Replace("[QueueList]", sQueue);
	Out.Replace("[Session]", sSession);
	Out.Replace("[UserNameTitle]", _GetPlainResString(IDS_QL_USERNAME));
	Out.Replace("[FileNameTitle]", _GetPlainResString(IDS_DL_FILENAME));
	Out.Replace("[ScoreTitle]", _GetPlainResString(IDS_SCORE));
	Out.Replace("[BannedTitle]", _GetPlainResString(IDS_BANNED));

	return Out;
}

CString CWapServer::_GetDownloadLink(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	if (!IsSessionAdmin(Data,sSession)) {
		CString ad="<br/><br/>[Message]";
		ad.Replace("[Message]",_GetPlainResString(IDS_ACCESSDENIED));
		return ad;
	}

	CString Out = pThis->m_Templates.sDownloadLink;

	Out.Replace("[Download]", _GetPlainResString(IDS_SW_DOWNLOAD));
	Out.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
	Out.Replace("[Start]", _GetPlainResString(IDS_SW_START));
	Out.Replace("[Session]", sSession);
	Out.Replace("[Section]", _GetPlainResString(IDS_SW_LINK));
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));

	if (thePrefs.GetCatCount()>1){
		InsertCatBox(Out,0,_GetPlainResString(IDS_TOCAT)+":&nbsp;<br/>");
		Out.Replace("[URL]", _SpecialChars("." + Data.sURL));
	}else Out.Replace("[CATBOX]","");

	return Out;
}

CString CWapServer::_GetSharedFilesList(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
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
    
	CString Out = pThis->m_Templates.sSharedList;
	int sortpos=0;
	//Name sorting link	
	if(pThis->m_Params.SharedSort == SHARED_SORT_NAME){
		Out.Replace("[SortName]", "./?ses=[Session]&amp;w=shared&amp;sort=name&amp;sortreverse=" + sSharedSortRev);
		sortpos=1;
		CString strSort=_GetPlainResString(IDS_DL_FILENAME);
		if (pThis->m_Params.bSharedSortReverse)
			strSort+=" (-)";
		else
			strSort+=" (+)";
		Out.Replace("[FilenameTitle]", strSort);
	}else
		Out.Replace("[SortName]", "./?ses=[Session]&amp;w=shared&amp;sort=name");
	//Size sorting Link
	if(pThis->m_Params.SharedSort == SHARED_SORT_SIZE){
		Out.Replace("[SortSize]", "./?ses=[Session]&amp;w=shared&amp;sort=size&amp;sortreverse=" + sSharedSortRev);
		sortpos=2;
		CString strSort=_GetPlainResString(IDS_DL_SIZE);
		if (pThis->m_Params.bSharedSortReverse)
			strSort+=" (-)";
		else
			strSort+=" (+)";
		Out.Replace("[SizeTitle]", strSort);
	}else
		Out.Replace("[SortSize]", "./?ses=[Session]&amp;w=shared&amp;sort=size");
	//Priority sorting Link
	if(pThis->m_Params.SharedSort == SHARED_SORT_PRIORITY){
		Out.Replace("[SortPriority]", "./?ses=[Session]&amp;w=shared&amp;sort=priority&amp;sortreverse=" + sSharedSortRev);
		sortpos=3;
		CString strSort=_GetPlainResString(IDS_PRIORITY);
		if (pThis->m_Params.bSharedSortReverse)
			strSort+=" (-)";
		else
			strSort+=" (+)";
		Out.Replace("[PriorityTitle]", strSort);
	}else
		Out.Replace("[SortPriority]", "./?ses=[Session]&amp;w=shared&amp;sort=priority");
    //Transferred sorting link
	if(pThis->m_Params.SharedSort == SHARED_SORT_TRANSFERRED)
	{
		sortpos=4;
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortTransferred]", "./?ses=[Session]&amp;w=shared&amp;sort=alltimetransferred&amp;sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortTransferred]", "./?ses=[Session]&amp;w=shared&amp;sort=transferred&amp;sortreverse=" + sSharedSortRev);

		CString strSort=_GetPlainResString(IDS_SF_TRANSFERRED);
		if (pThis->m_Params.bSharedSortReverse)
			strSort+=" (-)";
		else
			strSort+=" (+)";
		Out.Replace("[FileTransferredTitle]", strSort);
	}
	else
	if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_TRANSFERRED)
	{
		sortpos=4;
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortTransferred]", "./?ses=[Session]&amp;w=shared&amp;sort=transferred&amp;sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortTransferred]", "./?ses=[Session]&amp;w=shared&amp;sort=alltimetransferred&amp;sortreverse=" + sSharedSortRev);

		CString strSort=_GetPlainResString(IDS_SF_TRANSFERRED);
		if (pThis->m_Params.bSharedSortReverse)
			strSort+=" (-)";
		else
			strSort+=" (+)";
		Out.Replace("[FileTransferredTitle]", strSort);
	}
	else
        Out.Replace("[SortTransferred]", "./?ses=[Session]&amp;w=shared&amp;sort=transferred&amp;sortreverse=false");
    //Request sorting link
	if(pThis->m_Params.SharedSort == SHARED_SORT_REQUESTS)
	{
		sortpos=5;
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortRequests]", "./?ses=[Session]&amp;w=shared&amp;sort=alltimerequests&amp;sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortRequests]", "./?ses=[Session]&amp;w=shared&amp;sort=requests&amp;sortreverse=" + sSharedSortRev);

		CString strSort=_GetPlainResString(IDS_SF_REQUESTS);
		if (pThis->m_Params.bSharedSortReverse)
			strSort+=" (-)";
		else
			strSort+=" (+)";
		Out.Replace("[FileRequestsTitle]", strSort);
	}
	else
	if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_REQUESTS)
	{
		sortpos=5;
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortRequests]", "./?ses=[Session]&amp;w=shared&amp;sort=requests&amp;sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortRequests]", "./?ses=[Session]&amp;w=shared&amp;sort=alltimerequests&amp;sortreverse=" + sSharedSortRev);

		CString strSort=_GetPlainResString(IDS_SF_REQUESTS);
		if (pThis->m_Params.bSharedSortReverse)
			strSort+=" (-)";
		else
			strSort+=" (+)";
		Out.Replace("[FileRequestsTitle]", strSort);
	}
	else
        Out.Replace("[SortRequests]", "./?ses=[Session]&amp;w=shared&amp;sort=requests&amp;sortreverse=false");
    //Accepts sorting link
	if(pThis->m_Params.SharedSort == SHARED_SORT_ACCEPTS)
	{
		sortpos=6;
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortAccepts]", "./?ses=[Session]&amp;w=shared&amp;sort=alltimeaccepts&amp;sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortAccepts]", "./?ses=[Session]&amp;w=shared&amp;sort=accepts&amp;sortreverse=" + sSharedSortRev);

		CString strSort=_GetPlainResString(IDS_SF_ACCEPTS);
		if (pThis->m_Params.bSharedSortReverse)
			strSort+=" (-)";
		else
			strSort+=" (+)";
		Out.Replace("[FileAcceptsTitle]", strSort);
	}
	else
	if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_ACCEPTS)
	{
		sortpos=6;
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace("[SortAccepts]", "./?ses=[Session]&amp;w=shared&amp;sort=accepts&amp;sortreverse=" + sSharedSortRev);
		else
			Out.Replace("[SortAccepts]", "./?ses=[Session]&amp;w=shared&amp;sort=alltimeaccepts&amp;sortreverse=" + sSharedSortRev);

		CString strSort=_GetPlainResString(IDS_SF_ACCEPTS);
		if (pThis->m_Params.bSharedSortReverse)
			strSort+=" (-)";
		else
			strSort+=" (+)";
		Out.Replace("[FileAcceptsTitle]", strSort);
	}
	else
        Out.Replace("[SortAccepts]", "./?ses=[Session]&amp;w=shared&amp;sort=accepts&amp;sortreverse=false");

	//[SortByVal:x1,x2,x3,...]
	int sortbypos=Out.Find("[SortByVal:",0);
	if(sortbypos!=-1){
		int sortbypos2=Out.Find("]",sortbypos+1);
		if(sortbypos2!=-1){
			CString strSortByArray=Out.Mid(sortbypos+11,sortbypos2-sortbypos-11);
			CString resToken;
			int curPos = 0,curPos2=0;
			bool posfound=false;
			resToken= strSortByArray.Tokenize(",",curPos);
			while (resToken != "" && !posfound)
			{
				curPos2++;
				if(sortpos==atoi(resToken)){
					CString strTemp;
					strTemp.Format("%i",curPos2);
					Out.Replace(Out.Mid(sortbypos,sortbypos2-sortbypos+1),strTemp);
					posfound=true;
				}
				resToken= strSortByArray.Tokenize(",",curPos);
			};
			if(!posfound)
				Out.Replace(Out.Mid(sortbypos,sortbypos2-sortbypos+1),"1");
		}
	}

	if(_ParseURL(Data.sURL, "reload") == "true")
	{
		//CString resultlog = _SpecialChars(theApp.emuledlg->logtext);	//Pick-up last line of the log
		//resultlog = resultlog.TrimRight('\n');
		//resultlog = resultlog.Mid(resultlog.ReverseFind('\n'));
		CString resultlog = _SpecialChars(theApp.emuledlg->GetLastLogEntry());
		Out.Replace("[Message]",resultlog+"<br/><br/>");
	}
	else
        Out.Replace("[Message]","");

	Out.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
	Out.Replace("[Reload]", _GetPlainResString(IDS_SF_RELOAD));
	Out.Replace("[Session]", sSession);

	CString OutE = pThis->m_Templates.sSharedLine; 

	OutE.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
    OutE.Replace("[PriorityUp]", _GetPlainResString(IDS_PRIORITY_UP));
    OutE.Replace("[PriorityDown]", _GetPlainResString(IDS_PRIORITY_DOWN));

	CString OutE2 = pThis->m_Templates.sSharedLineChanged; 

	OutE2.Replace("[Ed2klink]", _GetPlainResString(IDS_SW_LINK));
    OutE2.Replace("[PriorityUp]", _GetPlainResString(IDS_PRIORITY_UP));
    OutE2.Replace("[PriorityDown]", _GetPlainResString(IDS_PRIORITY_DOWN));

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
	int startpos=0,endpos;

	if(_ParseURL(Data.sURL, "startpos")!="")
		startpos=atoi(_ParseURL(Data.sURL, "startpos"));

	endpos = startpos + thePrefs.GetWapMaxItemsInPages();

	if (endpos>SharedArray.GetCount())
		endpos=SharedArray.GetCount();

	if (startpos>SharedArray.GetCount()){
		startpos=endpos-thePrefs.GetWapMaxItemsInPages();
		if (startpos<0) startpos=0;
	}

	for(int i = startpos; i < endpos; i++)
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
		HTTPProcessData.Replace("[PriorityUpLink]", "hash=" + SharedArray[i].sFileHash +"&amp;setpriority=" + CString(HTTPTempC));
        sprintf(HTTPTempC, "%i", lesserpriority);
		HTTPProcessData.Replace("[PriorityDownLink]", "hash=" + SharedArray[i].sFileHash +"&amp;setpriority=" + CString(HTTPTempC));

		sSharedList += HTTPProcessData;
	}
	Out.Replace("[SharedFilesList]", sSharedList);
	Out.Replace("[Session]", sSession);

	//Navigation Line
	CString strNavLine,strNavUrl;
	int position,position2;
	strNavUrl = Data.sURL;
	if((position=strNavUrl.Find("startpos=",0))>1){
		if (strNavUrl.Mid(position-1,1)=="&") position--;
		position2=strNavUrl.Find("&",position+1);
		if (position2==-1) position2 = strNavUrl.GetLength();
		strNavUrl = strNavUrl.Left(position) + strNavUrl.Right(strNavUrl.GetLength()-position2);
	}	
	strNavUrl=_SpecialChars(strNavUrl);

	if(startpos>0){
		startpos-=thePrefs.GetWapMaxItemsInPages();
		if (startpos<0) startpos = 0;
		strNavLine.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_PREV) + "</a>&nbsp;", startpos);
	}
	if(endpos<SharedArray.GetCount()){
		CString strTemp;
		strTemp.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_NEXT) + "</a> ", endpos);
		strNavLine += strTemp;
	}
	Out.Replace("[NavigationLine]", strNavLine);

	Out.Replace("[FileHashTitle]", _GetPlainResString(IDS_FD_HASH));
	Out.Replace("[FilenameTitle]", _GetPlainResString(IDS_DL_FILENAME));
	Out.Replace("[PriorityTitle]",  _GetPlainResString(IDS_PRIORITY));
    Out.Replace("[FileTransferredTitle]",  _GetPlainResString(IDS_SF_TRANSFERRED));
    Out.Replace("[FileRequestsTitle]",  _GetPlainResString(IDS_SF_REQUESTS));
    Out.Replace("[FileAcceptsTitle]",  _GetPlainResString(IDS_SF_ACCEPTS));
	Out.Replace("[SizeTitle]", _GetPlainResString(IDS_DL_SIZE));
	Out.Replace("[Section]", _GetPlainResString(IDS_SHAREDFILES));
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
	Out.Replace("[SortBy]", _GetPlainResString(IDS_WAP_SORTBY));

	return Out;
}

CString CWapServer::_GetGraphs(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString Out = pThis->m_Templates.sGraphs;
	
	CString sSession = _ParseURL(Data.sURL, "ses");
	Out.Replace("[Session]", sSession);
	
	Out.Replace("[TxtDownload]", _GetPlainResString(IDS_DOWNLOAD));
	Out.Replace("[TxtUpload]", _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace("[TxtTime]", _GetPlainResString(IDS_TIME));
	Out.Replace("[TxtConnections]", _GetPlainResString(IDS_SP_ACTCON));
	Out.Replace("[KByteSec]", _GetPlainResString(IDS_KBYTESEC));
	Out.Replace("[TxtTime]", _GetPlainResString(IDS_TIME));
	Out.Replace("[Section]", _GetPlainResString(IDS_GRAPHS));
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));

	CString sScale;
	sScale.Format("%s", CastSecondsToHM(thePrefs.GetTrafficOMeterInterval() * thePrefs.GetWapGraphWidth()) );

	// emulEspaña: modified by MoNKi [Maella: -Allow Bandwidth Settings in <1KB Incremements-]
	/*
	CString s1, s2,s3;
	s1.Format("%d", thePrefs.GetMaxGraphDownloadRate() + 4);
	s2.Format("%d", thePrefs.GetMaxGraphUploadRate() + 4);
	s3.Format("%d", thePrefs.GetMaxConnections()+20);

	Out.Replace("[ScaleTime]", sScale);
	Out.Replace("[MaxDownload]", s1);
	Out.Replace("[MaxUpload]", s2);
	Out.Replace("[MaxConnections]", s3);
	*/
	CString s1, s2, s3;
	s1.Format("%d", (int)thePrefs.GetMaxGraphDownloadRate() + 4);
	s2.Format("%d", (int)thePrefs.GetMaxGraphUploadRate() + 4);
	s3.Format("%d", thePrefs.GetMaxConnections()+20);
	
	Out.Replace("[ScaleTime]", sScale);
	Out.Replace("[MaxDownload]", s1);
	Out.Replace("[MaxUpload]", s2);
	Out.Replace("[MaxConnections]", s3);
	// End emulEspaña

	return Out;
}

CString CWapServer::_GetServerOptions(WapThreadData Data)
{

	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	if (!IsSessionAdmin(Data,sSession)) return "";

	CString Out = pThis->m_Templates.sServerOptions;

	if(_ParseURL(Data.sURL, "addserver") == "true")
	{
		CServer* nsrv = new CServer(atoi(_ParseURL(Data.sURL, "serverport")), _ParseURL(Data.sURL, "serveraddr").GetBuffer() );
		nsrv->SetListName(_ParseURL(Data.sURL, "servername"));
		theApp.emuledlg->serverwnd->serverlistctrl.AddServer(nsrv,true);
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
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
	Out.Replace("[Section]", CString(_GetPlainResString(IDS_SERVER)+ " " + _GetPlainResString(IDS_EM_PREFS)));
	return Out;
}

/*
CString	CWapServer::_GetWebSearch(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
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
		query += "&amp;pattern=";
		query += _ParseURL(Data.sURL, "tosearch");
		query += "&amp;action=search&amp;name=FD-Search&amp;op=modload&amp;file=index&amp;requestby=emule";
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
}*/

CString CWapServer::_GetLog(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = pThis->m_Templates.sLog;

	if (_ParseURL(Data.sURL, "clear") == "yes" && IsSessionAdmin(Data,sSession))
	{
		theApp.emuledlg->ResetLog();
	}
	Out.Replace("[Clear]", _GetPlainResString(IDS_PW_RESET));

	CString strServerInfo, strServerInfo2, strServerInfo3, strToken;
	CList<CString,CString> logRows;
	int pos=0;
	strServerInfo = _SpecialChars(theApp.emuledlg->GetAllLogEntries());
	strToken=strServerInfo.Tokenize("\r\n",pos);
	while (strToken != "")
	{
		logRows.AddTail(strToken);
		strToken = strServerInfo.Tokenize("\r\n",pos);
	};
	
	POSITION pos2 = logRows.GetTailPosition();
	while (pos2 != NULL)
	{
		strToken = logRows.GetPrev(pos2);
		strServerInfo3=strServerInfo2+"<br/>"+strToken;
		if((UINT)strServerInfo3.GetLength() > thePrefs.GetWapLogsSize()){
			if(strServerInfo2=="")
				strServerInfo2=strServerInfo3;
			break;
		}
		else{
			strServerInfo2="<br/>"+strToken+strServerInfo2;
		}
	}
	logRows.RemoveAll();
	Out.Replace("[Log]", strServerInfo2);
	Out.Replace("[Session]", sSession);
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
	Out.Replace("[Section]", _GetPlainResString(IDS_SV_LOG));

	return Out;
}

CString CWapServer::_GetServerInfo(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = pThis->m_Templates.sServerInfo;
	
	if (_ParseURL(Data.sURL, "clear") == "yes")
	{
		theApp.emuledlg->serverwnd->servermsgbox->SetWindowText("");
	}

	CString strServerInfo, strServerInfo2, strServerInfo3, strToken;
	CList<CString,CString> logRows;
	int pos=0;
	strServerInfo = _SpecialChars(theApp.emuledlg->serverwnd->servermsgbox->GetText());
	strToken=strServerInfo.Tokenize("\r\n",pos);
	while (strToken != "")
	{
		logRows.AddTail(strToken);
		strToken = strServerInfo.Tokenize("\r\n",pos);
	};
	
	POSITION pos2 = logRows.GetTailPosition();
	while (pos2 != NULL)
	{
		strToken = logRows.GetPrev(pos2);
		strServerInfo3=strServerInfo2+"<br/>"+strToken;
		if((UINT)strServerInfo3.GetLength() > thePrefs.GetWapLogsSize()){
			if(strServerInfo2=="")
				strServerInfo2=strServerInfo3;
			break;
		}
		else{
			strServerInfo2="<br/>"+strToken+strServerInfo2;
		}
	}
	logRows.RemoveAll();
	Out.Replace("[ServerInfo]", strServerInfo2);
	Out.Replace("[Clear]", _GetPlainResString(IDS_PW_RESET));
	Out.Replace("[Session]", sSession);
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
	Out.Replace("[Section]", _GetPlainResString(IDS_SV_SERVERINFO));

	return Out;
}

CString CWapServer::_GetDebugLog(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = pThis->m_Templates.sDebugLog;

	if (_ParseURL(Data.sURL, "clear") == "yes" && IsSessionAdmin(Data,sSession))
	{
		theApp.emuledlg->ResetDebugLog();
	}
	Out.Replace("[Clear]", _GetPlainResString(IDS_PW_RESET));

	CString strServerInfo, strServerInfo2, strServerInfo3, strToken;
	CList<CString,CString> logRows;
	int pos=0;
	strServerInfo = _SpecialChars(theApp.emuledlg->GetAllDebugLogEntries());
	strToken=strServerInfo.Tokenize("\r\n",pos);
	while (strToken != "")
	{
		logRows.AddTail(strToken);
		strToken = strServerInfo.Tokenize("\r\n",pos);
	};
	
	POSITION pos2 = logRows.GetTailPosition();
	while (pos2 != NULL)
	{
		strToken = logRows.GetPrev(pos2);
		strServerInfo3=strServerInfo2+"<br/>"+strToken;
		if((UINT)strServerInfo3.GetLength() > thePrefs.GetWapLogsSize()){
			if(strServerInfo2=="")
				strServerInfo2=strServerInfo3;
			break;
		}
		else{
			strServerInfo2="<br/>"+strToken+strServerInfo2;
		}
	}
	logRows.RemoveAll();
	Out.Replace("[DebugLog]", strServerInfo2);
	Out.Replace("[Session]", sSession);
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
	Out.Replace("[Section]", _GetPlainResString(IDS_SV_DEBUGLOG));

	return Out;

}

CString CWapServer::_GetStats(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	// refresh statistics
	theApp.emuledlg->statisticswnd->ShowStatistics(true);

	CString Out = pThis->m_Templates.sStats;

	if(_ParseURL(Data.sURL, "show")!=""){
		HTREEITEM item;
		item = (HTREEITEM)strtoul(_ParseURL(Data.sURL, "show"),NULL,16);
		if(theApp.emuledlg->statisticswnd->stattree.ItemExist(item) &&
			theApp.emuledlg->statisticswnd->stattree.ItemHasChildren(item))
		{
			CString strTemp;
			strTemp = "<b>" + theApp.emuledlg->statisticswnd->stattree.GetItemText(item) + "</b><br/>";
			strTemp += theApp.emuledlg->statisticswnd->stattree.GetWML(false, false, true,theApp.emuledlg->statisticswnd->stattree.GetChildItem(item),1,false);
			Out.Replace("[Stats]", strTemp);
		}
		else
		{
			Out.Replace("[Stats]", theApp.emuledlg->statisticswnd->stattree.GetWML(false, true));
		}
	}
	else
	{
		Out.Replace("[Stats]", theApp.emuledlg->statisticswnd->stattree.GetWML(false, true));
	}

	Out.Replace("[Session]",sSession);
	Out.Replace("[Section]", _GetPlainResString(IDS_SF_STATISTICS));
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));

	return Out;

}

CString CWapServer::_GetPreferences(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = pThis->m_Templates.sPreferences;

	Out.Replace("[Session]", sSession);

	if ((_ParseURL(Data.sURL, "saveprefs") == "true") && IsSessionAdmin(Data,sSession) ) {
		if(_ParseURL(Data.sURL, "logssize") != "")
		{
			UINT logssize = atoi(_ParseURL(Data.sURL, "logssize"));
			thePrefs.SetWapLogsSize(logssize);
		}
		if(_ParseURL(Data.sURL, "itemsperpage")!="")
		{
			thePrefs.SetWapMaxItemsInPages(atoi(_ParseURL(Data.sURL, "itemsperpage")));
		}
		if(_ParseURL(Data.sURL, "graphswidth")!="")
		{
			thePrefs.SetWapGraphWidth(atoi(_ParseURL(Data.sURL, "graphswidth")));
		}
		if(_ParseURL(Data.sURL, "graphsheight")!="")
		{
			thePrefs.SetWapGraphHeight(atoi(_ParseURL(Data.sURL, "graphsheight")));
		}
		if(_ParseURL(Data.sURL, "sendbwimages")!="")
		{
			thePrefs.SetWapAllwaysSendBWImages(_ParseURL(Data.sURL, "sendbwimages").MakeLower() == "on");
		}
		if(_ParseURL(Data.sURL, "sendimages")!="")
		{
			thePrefs.SetWapSendImages(_ParseURL(Data.sURL, "sendimages").MakeLower() == "on");
		}
		if(_ParseURL(Data.sURL, "sendgraphs")!="")
		{
			thePrefs.SetWapSendGraphs(_ParseURL(Data.sURL, "sendgraphs").MakeLower() == "on");
		}
		if(_ParseURL(Data.sURL, "sendprogressbars")!="")
		{
			thePrefs.SetWapSendProgressBars(_ParseURL(Data.sURL, "sendprogressbars").MakeLower() == "on");
		}

		if(_ParseURL(Data.sURL, "filledgraphs").MakeLower() == "on")
		{
			thePrefs.SetWapGraphsFilled(true);
		}
		if(_ParseURL(Data.sURL, "filledgraphs").MakeLower() == "off")
		{
			thePrefs.SetWapGraphsFilled(false);
		}
		if(_ParseURL(Data.sURL, "refresh") != "")
		{
			thePrefs.SetWebPageRefresh(atoi(_ParseURL(Data.sURL, "refresh")));
		}

		// emulEspaña: modified by MoNKi [Maella: -Allow Bandwidth Settings in <1KB Incremements-]
		//if(_ParseURL(Data.sURL, "maxdown") != "")
		//{
		//	thePrefs.SetMaxDownload(atoi(_ParseURL(Data.sURL, "maxdown")));
		//}
		//if(_ParseURL(Data.sURL, "maxup") != "")
		//{
		//	thePrefs.SetMaxUpload(atoi(_ParseURL(Data.sURL, "maxup")));
		//}
		//uint16 lastmaxgu=thePrefs.GetMaxGraphUploadRate();
		//uint16 lastmaxgd=thePrefs.GetMaxGraphDownloadRate();

		//if(_ParseURL(Data.sURL, "maxcapdown") != "")
		//{
		//	thePrefs.SetMaxGraphDownloadRate(atoi(_ParseURL(Data.sURL, "maxcapdown")));
		//}
		//if(_ParseURL(Data.sURL, "maxcapup") != "")
		//{
		//	thePrefs.SetMaxGraphUploadRate(atoi(_ParseURL(Data.sURL, "maxcapup")));
		//}

		//if(lastmaxgu != thePrefs.GetMaxGraphUploadRate()) 
		//	theApp.emuledlg->statisticswnd.SetARange(false,thePrefs.GetMaxGraphUploadRate());
		//if(lastmaxgd!=thePrefs.GetMaxGraphDownloadRate())
		//	theApp.emuledlg->statisticswnd.SetARange(true,thePrefs.GetMaxGraphDownloadRate());

		if(_ParseURL(Data.sURL, "maxdown") != "")
		{
			float maxdown = atof(_ParseURL(Data.sURL, "maxdown"));
			if(maxdown > 0.0f)
				thePrefs.SetMaxDownload(maxdown);
			else
				thePrefs.SetMaxDownload(UNLIMITED);
		}
		if(_ParseURL(Data.sURL, "maxup") != "")
		{
			float maxup = atof(_ParseURL(Data.sURL, "maxup"));
			if(maxup > 0.0f)
				thePrefs.SetMaxUpload(maxup);
			else
				thePrefs.SetMaxUpload(UNLIMITED);
		}
		if(_ParseURL(Data.sURL, "maxcapdown") != "")
		{
			thePrefs.SetMaxGraphDownloadRate(atof(_ParseURL(Data.sURL, "maxcapdown")));
		}
		if(_ParseURL(Data.sURL, "maxcapup") != "")
		{
			thePrefs.SetMaxGraphUploadRate(atof(_ParseURL(Data.sURL, "maxcapup")));
		}
		// End emulEspaña

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
		//thePrefs.SetPreviewPrio((_ParseURL(Data.sURL, "firstandlast").MakeLower() == "on"));	// emulEspaña: removed by Announ [Announ: -per file option for downloading preview parts-]

		thePrefs.SetNetworkED2K((_ParseURL(Data.sURL, "neted2k").MakeLower() == "on"));
		thePrefs.SetNetworkKademlia((_ParseURL(Data.sURL, "netkad").MakeLower() == "on"));
	}

	// Fill form
	Out.Replace("[WapGraphFilledVal]",thePrefs.GetWapGraphsFilled()?"1":"2");
	Out.Replace("[SendGraphsVal]",thePrefs.GetWapSendGraphs()?"1":"2");
	Out.Replace("[SendBWImagesVal]",thePrefs.GetWapAllwaysSendBWImages()?"1":"2");
	Out.Replace("[SendImagesVal]",thePrefs.GetWapSendImages()?"1":"2");
	Out.Replace("[SendProgressBarsVal]",thePrefs.GetWapSendProgressBars()?"1":"2");
	// emulEspaña: modified by Announ [Announ: -per file option for downloading preview parts-]
	/*
	Out.Replace("[FirstAndLastVal]",thePrefs.GetPreviewPrio()?"1":"2");
	*/
	// End emulEspaña

	Out.Replace("[FullChunksVal]",thePrefs.TransferFullChunks()?"1":"2");

	CString sRefresh;

	sRefresh.Format("%d", thePrefs.GetWapLogsSize());
	Out.Replace("[LogsSizeVal]", sRefresh);

	sRefresh.Format("%d", thePrefs.GetWapGraphHeight());
	Out.Replace("[GraphsHeightVal]", sRefresh);

	sRefresh.Format("%d", thePrefs.GetWapGraphWidth());
	Out.Replace("[GraphsWidthVal]", sRefresh);

	sRefresh.Format("%d", thePrefs.GetWapMaxItemsInPages());
	Out.Replace("[ItemsPerPageVal]", sRefresh);

	sRefresh.Format("%d", thePrefs.GetWebPageRefresh());
	Out.Replace("[RefreshVal]", sRefresh);

	sRefresh.Format("%d", thePrefs.GetMaxSourcePerFile());
	Out.Replace("[MaxSourcesVal]", sRefresh);

	sRefresh.Format("%d", thePrefs.GetMaxConnections());
	Out.Replace("[MaxConnectionsVal]", sRefresh);

	sRefresh.Format("%d", thePrefs.GetMaxConperFive());
	Out.Replace("[MaxConnectionsPer5Val]", sRefresh);

	Out.Replace("[ED2KVAL]", (thePrefs.GetNetworkED2K())?"1":"2" );
	Out.Replace("[KADVAL]", (thePrefs.GetNetworkKademlia())?"1":"2" );

	Out.Replace("[KBS]", _GetPlainResString(IDS_KBYTESEC)+":");
	Out.Replace("[FileSettings]", CString(_GetPlainResString(IDS_WEB_FILESETTINGS)+":"));
	Out.Replace("[LimitForm]", _GetPlainResString(IDS_WEB_CONLIMITS)+":");
	Out.Replace("[MaxSources]", _GetPlainResString(IDS_PW_MAXSOURCES)+":");
	Out.Replace("[MaxConnections]", _GetPlainResString(IDS_PW_MAXC)+":");
	Out.Replace("[MaxConnectionsPer5]", _GetPlainResString(IDS_MAXCON5SECLABEL)+":");
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
	Out.Replace("[WapControl]", _GetPlainResString(IDS_WAP_CONTROL));
	Out.Replace("[eMuleAppName]", "eMule");
	Out.Replace("[Apply]", _GetPlainResString(IDS_PW_APPLY));

	Out.Replace("[Yes]", _GetPlainResString(IDS_YES));
	Out.Replace("[No]", _GetPlainResString(IDS_NO));	
	Out.Replace("[On]", _GetPlainResString(IDS_WAP_ON));
	Out.Replace("[Off]", _GetPlainResString(IDS_WAP_OFF));
	Out.Replace("[Section]", _GetPlainResString(IDS_EM_PREFS));
	Out.Replace("[SendImages]", _GetPlainResString(IDS_WAP_SENDIMAGES));
	Out.Replace("[SendGraphs]", _GetPlainResString(IDS_WAP_SENDGRAPHS));
	Out.Replace("[SendProgressBars]", _GetPlainResString(IDS_WAP_SENDPROGRESSBARS));
	Out.Replace("[SendBWImages]", _GetPlainResString(IDS_WAP_SENDBWIMAGES));
	Out.Replace("[GraphsWidth]", _GetPlainResString(IDS_WAP_GRAPHSWIDTH));
	Out.Replace("[GraphsHeight]", _GetPlainResString(IDS_WAP_GRAPHSHEIGHT));
	Out.Replace("[FilledGraphs]", _GetPlainResString(IDS_WAP_FILLEDGRAPHS));
	Out.Replace("[LogsSize]", _GetPlainResString(IDS_WAP_LOGSSIZE));
	Out.Replace("[ItemsPerPage]", _GetPlainResString(IDS_WAP_ITEMSPERPAGE));
	Out.Replace("[Bytes]", _GetPlainResString(IDS_BYTES));
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
	Out.Replace("[NETWORKS]", _GetPlainResString(IDS_NETWORK));
	Out.Replace("[BOOTSTRAP]", _GetPlainResString(IDS_BOOTSTRAP));
	Out.Replace("[BS_IP]", _GetPlainResString(IDS_IP));
	Out.Replace("[BS_PORT]", _GetPlainResString(IDS_PORT));
	Out.Replace("[KADEMLIA]", GetResString(IDS_KADEMLIA) );

	// emulEspaña: modified by MoNKi [Maella: -Allow Bandwidth Settings in <1KB Incremements-]
	//CString sT;
	//int n = (int)thePrefs.GetMaxDownload();
	//if(n < 0 || n == 65535) n = 0;
	//sT.Format("%d", n);
	//Out.Replace("[MaxDownVal]", sT);
	//n = (int)thePrefs.GetMaxUpload();
	//if(n < 0 || n == 65535) n = 0;
	//sT.Format("%d", n);
	//Out.Replace("[MaxUpVal]", sT);
	//sT.Format("%d", thePrefs.GetMaxGraphDownloadRate());
	//Out.Replace("[MaxCapDownVal]", sT);
	//sT.Format("%d", thePrefs.GetMaxGraphUploadRate());
	//Out.Replace("[MaxCapUpVal]", sT);
	CString sT;
	float n = thePrefs.GetMaxDownload();
	if(n < 0.0f || n >= UNLIMITED) n = 0.0f;
	sT.Format("%0.1f", n);
	Out.Replace("[MaxDownVal]", sT);
	n = thePrefs.GetMaxUpload();
	if(n < 0.0f || n >= UNLIMITED) n = 0.0f;
	sT.Format("%.1f", n);
	Out.Replace("[MaxUpVal]", sT);
	sT.Format("%.1f", thePrefs.GetMaxGraphDownloadRate());
	Out.Replace("[MaxCapDownVal]", sT);
	sT.Format("%.1f", thePrefs.GetMaxGraphUploadRate());
	Out.Replace("[MaxCapUpVal]", sT);
	// End emulEspaña

	return Out;
}

CString CWapServer::_GetLoginScreen(WapThreadData Data)
{

	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");

	CString Out = "";

	Out += pThis->m_Templates.sLogin;

	Out.Replace("[eMulePlus]", "eMule");
	Out.Replace("[eMuleAppName]", "eMule");
	// modified by Announ [itsonlyme: -modname-]
	/*
	Out.Replace("[version]", _SpecialChars(theApp.m_strCurVersionLong));
	*/
	Out.Replace("[version]", _SpecialChars(theApp.m_strCurVersionLong)+CString(" ")+theApp.m_strModLongVersion);
	// End -modname-
	Out.Replace("[Login]", _GetPlainResString(IDS_WEB_LOGIN));
	Out.Replace("[EnterPassword]", _GetPlainResString(IDS_WEB_ENTER_PASSWORD));
	Out.Replace("[LoginNow]", _GetPlainResString(IDS_WEB_LOGIN_NOW));
	Out.Replace("[WapControl]", _GetPlainResString(IDS_WAP_CONTROL));
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));

	return Out;
}

CString CWapServer::_GetConnectedServer(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
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
	OutS.Replace("[URL_Disconnect]", IsSessionAdmin(Data,sSession)?CString("./?ses=" + sSession + "&amp;w=server&amp;c=disconnect"):GetPermissionDenied());
	OutS.Replace("[URL_Connect]", IsSessionAdmin(Data,sSession)?CString("./?ses=" + sSession + "&amp;w=server&amp;c=connect"):GetPermissionDenied());
	OutS.Replace("[Disconnect]", _GetPlainResString(IDS_IRC_DISCONNECT));
	OutS.Replace("[Connect]", _GetPlainResString(IDS_CONNECTTOANYSERVER));
	OutS.Replace("[URL_ServerOptions]", IsSessionAdmin(Data,sSession)?CString("./?ses=" + sSession + "&amp;w=server&amp;c=options"):GetPermissionDenied());
	OutS.Replace("[ServerOptions]", CString(_GetPlainResString(IDS_SERVER)+ " " + _GetPlainResString(IDS_EM_PREFS)));
	OutS.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	OutS.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));

	if (theApp.serverconnect->IsConnected()) {
		if(!theApp.serverconnect->IsLowID())
			OutS.Replace("[1]", _GetPlainResString(IDS_CONNECTED));
		else
			OutS.Replace("[1]", _GetPlainResString(IDS_CONNECTED) + " (" + _GetPlainResString(IDS_IDLOW) + ")");

		CServer* cur_server = theApp.serverconnect->GetCurrentServer();
		OutS.Replace("[2]", _SpecialChars(cur_server->GetListName()));

		sprintf(HTTPTempC, "%10i", cur_server->GetUsers());
		HTTPTemp = HTTPTempC;												
		OutS.Replace("[3]", HTTPTemp);

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

bool CWapServer::_IsLoggedIn(WapThreadData Data, long lSession)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
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

void CWapServer::_RemoveTimeOuts(WapThreadData Data, long lSession) {
	// remove expired sessions
	CWapServer *pThis = (CWapServer *)Data.pThis;
	pThis->UpdateSessionCount();
}

bool CWapServer::_RemoveSession(WapThreadData Data, long lSession)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	// find our session
	for(int i = 0; i < pThis->m_Params.Sessions.GetSize(); i++)
	{
		if(pThis->m_Params.Sessions[i].lSession == lSession && lSession != 0)
		{
			pThis->m_Params.Sessions.RemoveAt(i);
			theApp.emuledlg->serverwnd->UpdateMyInfo();
			AddLogLine(true,GetResString(IDS_WAP_SESSIONEND));
			return true;
		}
	}
	return false;
}

Session CWapServer::GetSessionByID(WapThreadData Data,long sessionID) {
	CWapServer *pThis = (CWapServer *)Data.pThis;
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

bool CWapServer::IsSessionAdmin(WapThreadData Data,CString SsessionID){

	long sessionID=_atoi64(SsessionID);
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis != NULL) {
		for(int i = 0; i < pThis->m_Params.Sessions.GetSize(); i++)
		{
			if(pThis->m_Params.Sessions[i].lSession == sessionID && sessionID != 0)
				return pThis->m_Params.Sessions[i].admin;
		}
	}
	return false;
}

CString CWapServer::GetPermissionDenied() {
	return "./script.wmls#accessdenied()";
}

bool CWapServer::_GetFileHash(CString sHash, uchar *FileHash)
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

void CWapServer::PlainString(CString& rstr, bool noquot)
{
	rstr.Replace("&", "");
	//rstr.Replace("&", "&amp;");
	rstr.Replace("<", "&lt;");
	rstr.Replace(">", "&gt;");
	rstr.Replace("\"", "&quot;");

	if(noquot)
	{
        rstr.Replace("'", "\\'");
		rstr.Replace("\n", "\\n");
	}
	else
	{
        rstr.Replace("'", "&apos;");
	}
}

CString	CWapServer::_GetPlainResString(RESSTRIDTYPE nID, bool noquot)
{
	CString sRet = _GetResString(nID);
	PlainString(sRet, noquot);
	return sRet;
}

// EC + kuchin
CString	CWapServer::_GetWebCharSet()
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

CString CWapServer::_GetDownloadGraph(WapThreadData Data,CString filehash)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	if(thePrefs.GetWapSendProgressBars()){
		CString sSession = _ParseURL(Data.sURL, "ses");
		CString Out;
		Out="<img src=\"./?ses=" + sSession + "&amp;progressbar=" + filehash + "\" alt=\"ProgressBar\"/>";
		return Out;
	}
	else return "";
}


CString	CWapServer::_GetSearch(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	CString sSession = _ParseURL(Data.sURL, "ses");
	CString Out = pThis->m_Templates.sSearchMain;
	CString OutList = pThis->m_Templates.sSearchList;

	if (_ParseURL(Data.sURL, "downloads") != "" && IsSessionAdmin(Data,sSession) ) {
		CString downloads=_ParseURLArray(Data.sURL,"downloads");
		uint8 cat=atoi(_ParseURL(Data.sURL, "cat"));

		CString resToken;
		int curPos=0;
		resToken= downloads.Tokenize("|",curPos);
		while (resToken != "")
		{
			uchar fileid[16];
			if (resToken.GetLength()==32 && DecodeBase16(resToken.GetBuffer(),resToken.GetLength(),fileid,ARRSIZE(fileid)))
			theApp.searchlist->AddFileToDownloadByHash(fileid,cat);
			resToken= downloads.Tokenize("|",curPos);
		}
	}

	if(_ParseURL(Data.sURL, "tosearch") != "" && IsSessionAdmin(Data,sSession) )
	{
		// perform search
		theApp.emuledlg->searchwnd->DeleteAllSearchs();
		SSearchParams* pParams = new SSearchParams;
		pParams->strExpression = _ParseURL(Data.sURL, "tosearch");
		pParams->strFileType = _ParseURL(Data.sURL, "type");
		pParams->ulMinSize = atol(_ParseURL(Data.sURL, "min"))*1048576;
		pParams->ulMaxSize = atol(_ParseURL(Data.sURL, "max"))*1048576;
		if (pParams->ulMaxSize < pParams->ulMinSize)
			pParams->ulMaxSize = 0;
		
		pParams->uAvailability = (_ParseURL(Data.sURL, "avail")=="")?0:atoi(_ParseURL(Data.sURL, "avail"));
		if (pParams->uAvailability > 1000000)
			pParams->uAvailability = 1000000;
			
		pParams->strExtension = _ParseURL(Data.sURL, "ext");
	
		CString method=(_ParseURL(Data.sURL, "method"));

		if (method == "kademlia")
			pParams->eType = SearchTypeKademlia;
		else if (method == "global")
			pParams->eType = SearchTypeEd2kGlobal;
		else
			pParams->eType = SearchTypeEd2kServer;

		CString strResponse = "<a href=\"./?ses=[Session]&amp;w=search&amp;showlist=true\">" + _GetPlainResString(IDS_SW_SEARCHINGINFO) + "</a>";
		try
		{
		if (pParams->eType != SearchTypeKademlia){
				if (!theApp.emuledlg->searchwnd->DoNewEd2kSearch(pParams)){
				delete pParams;
					strResponse = _GetPlainResString(IDS_ERR_NOTCONNECTED);
				}
		}
		else{
				if (!theApp.emuledlg->searchwnd->DoNewKadSearch(pParams)){
					delete pParams;
					strResponse = _GetPlainResString(IDS_ERR_NOTCONNECTEDKAD);
				}
			}
		}
		catch (CMsgBoxException* ex)
		{
			strResponse = ex->m_strMsg;
			PlainString(strResponse, false);
			ex->Delete();
				delete pParams;
		}
		OutList.Replace("[Message]",strResponse);
	}
	else if(_ParseURL(Data.sURL, "tosearch") != "" && !IsSessionAdmin(Data,sSession) ) {
		OutList.Replace("[Message]",_GetPlainResString(IDS_ACCESSDENIED));
	}
	else OutList.Replace("[Message]","");

	if(_ParseURL(Data.sURL, "showlist") == "true"){
		CString sSort = _ParseURL(Data.sURL, "sort");	if (sSort.GetLength()>0) pThis->m_iSearchSortby=atoi(sSort);
		sSort = _ParseURL(Data.sURL, "sortAsc");		if (sSort.GetLength()>0) pThis->m_bSearchAsc=atoi(sSort);

		int startpos = 0;
		bool more;

		if(_ParseURL(Data.sURL, "startpos")!="")
			startpos = atoi(_ParseURL(Data.sURL, "startpos"));

		int sortpos=0;
		if(pThis->m_iSearchSortby==0){
			sortpos=1;
			CString strSortBy;
			if(pThis->m_bSearchAsc==0)
				strSortBy=" (+)";
			else
				strSortBy=" (-)";
			OutList.Replace("[Filename]", _GetPlainResString(IDS_DL_FILENAME) + strSortBy);
		}
		else if(pThis->m_iSearchSortby==1){
			sortpos=2;
			CString strSortBy;
			if(pThis->m_bSearchAsc==0)
				strSortBy=" (+)";
			else
				strSortBy=" (-)";
			OutList.Replace("[Filesize]", _GetPlainResString(IDS_DL_SIZE) + strSortBy);
		}
		else if(pThis->m_iSearchSortby==2){
			sortpos=3;
			CString strSortBy;
			if(pThis->m_bSearchAsc==0)
				strSortBy=" (+)";
			else
				strSortBy=" (-)";
			OutList.Replace("[Filehash]", _GetPlainResString(IDS_FILEHASH) + strSortBy);
		}
		else if(pThis->m_iSearchSortby==3){
			sortpos=4;
			CString strSortBy;
			if(pThis->m_bSearchAsc==0)
				strSortBy=" (+)";
			else
				strSortBy=" (-)";
			OutList.Replace("[Sources]", _GetPlainResString(IDS_DL_SOURCES) + strSortBy);
		}
		//[SortByVal:x1,x2,x3,...]
		int sortbypos=OutList.Find("[SortByVal:",0);
		if(sortbypos!=-1){
			int sortbypos2=OutList.Find("]",sortbypos+1);
			if(sortbypos2!=-1){
				CString strSortByArray=OutList.Mid(sortbypos+11,sortbypos2-sortbypos-11);
				CString resToken;
				int curPos = 0,curPos2=0;
				bool posfound=false;
				resToken= strSortByArray.Tokenize(",",curPos);
				while (resToken != "" && !posfound)
				{
					curPos2++;
					if(sortpos==atoi(resToken)){
						CString strTemp;
						strTemp.Format("%i",curPos2);
						OutList.Replace(OutList.Mid(sortbypos,sortbypos2-sortbypos+1),strTemp);
						posfound=true;
					}
					resToken= strSortByArray.Tokenize(",",curPos);
				};
				if(!posfound)
					OutList.Replace(Out.Mid(sortbypos,sortbypos2-sortbypos+1),"1");
			}
		}

		CString val;
		val.Format("%i",(pThis->m_iSearchSortby!=0 || (pThis->m_iSearchSortby==0 && pThis->m_bSearchAsc==0 ))?1:0 );
		OutList.Replace("[SORTASCVALUE0]", val);
		val.Format("%i",(pThis->m_iSearchSortby!=1 || (pThis->m_iSearchSortby==1 && pThis->m_bSearchAsc==0 ))?1:0 );
		OutList.Replace("[SORTASCVALUE1]", val);
		val.Format("%i",(pThis->m_iSearchSortby!=2 || (pThis->m_iSearchSortby==2 && pThis->m_bSearchAsc==0 ))?1:0 );
		OutList.Replace("[SORTASCVALUE2]", val);
		val.Format("%i",(pThis->m_iSearchSortby!=3 || (pThis->m_iSearchSortby==3 && pThis->m_bSearchAsc==0 ))?1:0 );
		OutList.Replace("[SORTASCVALUE3]", val);

		CString result = theApp.searchlist->GetWapList(pThis->m_Templates.sSearchResultLine,pThis->m_iSearchSortby,pThis->m_bSearchAsc,startpos,thePrefs.GetWapMaxItemsInPages(),more);

		//Navigation Line
		CString strNavLine,strNavUrl;
		int pos,pos2;
		strNavUrl = Data.sURL;
		if((pos=strNavUrl.Find("startpos=",0))>1){
			if (strNavUrl.Mid(pos-1,1)=="&") pos--;
			pos2=strNavUrl.Find("&",pos+1);
			if (pos2==-1) pos2 = strNavUrl.GetLength();
			strNavUrl = strNavUrl.Left(pos) + strNavUrl.Right(strNavUrl.GetLength()-pos2);
		}	
		strNavUrl=_SpecialChars(strNavUrl);

		if(more){
			strNavLine.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_NEXT) + "</a> ", startpos+thePrefs.GetWapMaxItemsInPages());
		}
		if(startpos>0){
			CString strTemp;
			startpos-=thePrefs.GetWapMaxItemsInPages();
			if (startpos<0) startpos = 0;
			strTemp.Format("<a href=\"." + strNavUrl + "&amp;startpos=%d\">" + _GetPlainResString(IDS_WAP_PREV) + "</a>&nbsp;", startpos);
			strNavLine.Insert(0,strTemp);
		}
		OutList.Replace("[NavigationLine]", strNavLine);

		if (thePrefs.GetCatCount()>1) InsertCatBox(OutList,0,_GetPlainResString(IDS_TOCAT)+":&nbsp;<br/>"); else OutList.Replace("[CATBOX]","");
		OutList.Replace("[URL]", _SpecialChars("." + Data.sURL));
		
		OutList.Replace("[RESULTLIST]", result);
		OutList.Replace("[Result]", _GetPlainResString(IDS_SW_RESULT) );
		OutList.Replace("[Session]", sSession);
		OutList.Replace("[RefetchResults]", _GetPlainResString(IDS_SW_REFETCHRES));
		OutList.Replace("[Download]", _GetPlainResString(IDS_DOWNLOAD));
		OutList.Replace("[Filename]", _GetPlainResString(IDS_DL_FILENAME));
		OutList.Replace("[Filesize]", _GetPlainResString(IDS_DL_SIZE));
		OutList.Replace("[Filehash]", _GetPlainResString(IDS_FILEHASH));
		OutList.Replace("[Sources]", _GetPlainResString(IDS_DL_SOURCES));
		OutList.Replace("[Section]", _GetPlainResString(IDS_SW_SEARCHBOX));
		OutList.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
		OutList.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));
		OutList.Replace("[SortBy]", _GetPlainResString(IDS_WAP_SORTBY));
		return OutList;

	}
	else
	{ // No showlist
		Out.Replace("[Name]", _GetPlainResString(IDS_SW_NAME));
		Out.Replace("[Type]", _GetPlainResString(IDS_TYPE));
		Out.Replace("[Any]", _GetPlainResString(IDS_SEARCH_ANY));
		Out.Replace("[Audio]", _GetPlainResString(IDS_SEARCH_AUDIO));
		Out.Replace("[Image]", _GetPlainResString(IDS_SEARCH_PICS));

		Out.Replace("[Video]", _GetPlainResString(IDS_SEARCH_VIDEO));
		Out.Replace("[Other]", "Other");
		Out.Replace("[Search]", _GetPlainResString(IDS_SW_SEARCHBOX));
		Out.Replace("[Session]", sSession);

		Out.Replace("[SizeMin]", _GetPlainResString(IDS_SEARCHMINSIZE));
		Out.Replace("[SizeMax]", _GetPlainResString(IDS_SEARCHMAXSIZE));
		Out.Replace("[Availabl]", _GetPlainResString(IDS_SEARCHAVAIL));
		Out.Replace("[Extention]", _GetPlainResString(IDS_SEARCHEXTENTION));
		//Out.Replace("[Global]", _GetPlainResString(IDS_GLOBALSEARCH2));
		Out.Replace("[MB]", _GetPlainResString(IDS_MBYTES));
		Out.Replace("[Section]", _GetPlainResString(IDS_SW_SEARCHBOX));
		Out.Replace("[Yes]", _GetPlainResString(IDS_YES));
		Out.Replace("[No]", _GetPlainResString(IDS_NO));
		Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
		Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));

		Out.Replace("[METHOD]", _GetPlainResString(IDS_METHOD));
		Out.Replace("[USESSERVER]", _GetPlainResString(IDS_SERVER));
		Out.Replace("[Global]", _GetPlainResString(IDS_GLOBALSEARCH));
		Out.Replace("[KADEMLIA]", _GetPlainResString(IDS_KADEMLIA) );
		return Out;
	}
}

int CWapServer::UpdateSessionCount() {

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

void CWapServer::InsertCatBox(CString &Out,int preselect,CString boxlabel,bool jump,bool extraCats) {
	CString tempBuf2,tempBuf3;
	CString tempBuf;
	int pos=0;
	tempBuf3 = "<select name=\"cat\">";

	for (int i=0;i< thePrefs.GetCatCount();i++) {
		pos++;
		if(preselect==i)
			tempBuf3.Format("<select name=\"cat\" ivalue=\"%i\">",pos);
		if(jump)
			tempBuf2.Format("<option value=\"%i\" onpick=\"./script.wmls#GotoCat('%i','[URL]')\">%s</option>",i,i, (i==0)?_GetPlainResString(IDS_ALL):thePrefs.GetCategory(i)->title );
		else
			tempBuf2.Format("<option value=\"%i\">%s</option>",i, (i==0)?_GetPlainResString(IDS_ALL):thePrefs.GetCategory(i)->title );
		tempBuf.Append(tempBuf2);
	}
	if (extraCats) {
		if (thePrefs.GetCatCount()>1){
			pos++;
			tempBuf2.Format("<option>------------</option>");
			tempBuf.Append(tempBuf2);
		}
	
		for (int i=(thePrefs.GetCatCount()>1)?1:2;i<=12;i++) {
			pos++;
			if(preselect==-i)
				tempBuf3.Format("<select name=\"cat\" ivalue=\"%i\">",pos);
			if(jump)
				tempBuf2.Format("<option value=\"%i\" onpick=\"./script.wmls#GotoCat('%i','[URL]')\">%s</option>",-i,-i, GetSubCatLabel(-i) );
			else
				tempBuf2.Format("<option value=\"%i\">%s</option>",-i, GetSubCatLabel(-i) );
			tempBuf.Append(tempBuf2);
		}
	}
	tempBuf.Insert(0,tempBuf3);
	tempBuf.Append("</select>");
	//if(jump) tempBuf.Append("<a href=\"./script.wmls#GotoCat($(cat),'[URL]')\">Go</a>");
	Out.Replace("[CATBOX]",boxlabel+tempBuf);
}

CString CWapServer::GetSubCatLabel(int cat) {
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
CString CWapServer::GetUploadFileInfo(CUpDownClient* client)
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

	return "";
}

void CWapServer::DrawLineInCxImage(CxImage *image,int x1, int y1, int x2, int y2, COLORREF color){
	int dx, dy, incr_x, incr_y, const1, const2, x, y, p, k;

	if (x1==x2 && y1==y2)
		image->SetPixelColor(x1,y1,color);
	else {
		// Bresenham Algorithm
		dx = x2 - x1;
		if (dx < 0){
			dx = -dx;
			incr_x = -1;
		}
		else incr_x = 1;

		dy = y2 - y1;
		if (dy < 0){
			dy = -dy;
			incr_y = -1;
		}
		else incr_y = 1;

		x = x1;
		y = y1;
		image->SetPixelColor(x,y,color);

		if (dx > dy) {
			p = 2*dy - dx;
			const1 = 2*dy;
			const2 = 2*(dy - dx);
			for (k=1; k <= dx; k++){
				x = x + incr_x;
				if (p < 0)
					p = p + const1;
				else {
					y = y + incr_y;
					p = p + const2;
				}
					image->SetPixelColor(x,y,color);
			}
		} else {
			p = 2*dx - dy;
			const1 = 2*dx;
			const2 = 2*(dx - dy);
			for (k=1; k <= dy; k++){
				y = y+ incr_y;
				if (p < 0)
					p = p + const1;
				else {
					x = x + incr_x;
					p = p + const2;
				}
					image->SetPixelColor(x,y,color);
			}
		}
	}
}

void CWapServer::SendGraphFile(WapThreadData Data, int file_val){
	CxImage* cImage;
	int pos, width, height;
	long curval;
	float scalefactor;
	COLORREF color1,color2,color3,curcolor;
	bool png=false, gif=false;

	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return;

	if(!thePrefs.GetWapSendGraphs()){
		Data.pSocket->SendError404();
		return;
	}

	png=BrowserAccept(Data,"image/png");
	gif=BrowserAccept(Data,"image/gif");
	if(!(png || gif) && BrowserAccept(Data,"image/*")){
		png=true;
		gif=true;
	}

	width=thePrefs.GetWapGraphWidth();
	height=thePrefs.GetWapGraphHeight();
	switch(file_val){
		case 1:
			//(height-2) because the border are 2 pixels (1 top, 1 bottom)
			scalefactor=(float)(height-2)/(thePrefs.GetMaxGraphDownloadRate() + 4);
			break;
		case 2:
			scalefactor=(float)(height-2)/(thePrefs.GetMaxGraphUploadRate() + 4);
			break;
		case 3:
			scalefactor=(float)(height-2)/(thePrefs.GetMaxConnections()+20);
			break;
	}

	cImage = new CxImage(width, height, 24);
	if(!cImage){
		Data.pSocket->SendError404();
		return;
	}
	cImage->Clear(0xff);

	if((png || gif) && !thePrefs.GetWapAllwaysSendBWImages()){
		color1=pThis->m_Templates.cDownColor; 
		color2=pThis->m_Templates.cUpColor; 
		color3=pThis->m_Templates.cConnectionsColor; 

		RGBQUAD colors[16];
		colors[0]=cImage->RGBtoRGBQUAD(0x00000000);
		colors[1]=cImage->RGBtoRGBQUAD(color1);
		colors[2]=cImage->RGBtoRGBQUAD(color2);
		colors[3]=cImage->RGBtoRGBQUAD(color3);
		for(int i=4; i<16; i++)
			colors[i]=cImage->RGBtoRGBQUAD(0x00FFFFFF);

		cImage->DecreaseBpp(4,false,colors);
	} else {
		color1=color2=color3=0x00000000;
		cImage->DecreaseBpp(1,false);
	}

	pos = pThis->m_Params.PointsForWeb.GetCount()-1;

	if(thePrefs.GetWapGraphsFilled()){
		for(int x = width-2; x > 0 && pos >= 0; x--, pos--)
		{
			switch(file_val){
				case 1:
					curval=(pThis->m_Params.PointsForWeb[pos].download * scalefactor);
					curcolor=color1;
					break;
				case 2:
					curval=(pThis->m_Params.PointsForWeb[pos].upload * scalefactor);
					curcolor=color2;
					break;
				case 3:
					curval=(pThis->m_Params.PointsForWeb[pos].connections * scalefactor);
					curcolor=color3;
					break;
			}
			for(int y = 0; y <= curval ;y++)
				cImage->SetPixelColor(x,y,curcolor);
		}
	} else {
		int oldval=0;
		for(int x = width-1; x > 0 && pos >= 0; x--, pos--)
		{
			switch(file_val){
				case 1:
					curval=(pThis->m_Params.PointsForWeb[pos].download * scalefactor);
					curcolor=color1;
					break;
				case 2:
					curval=(pThis->m_Params.PointsForWeb[pos].upload * scalefactor);
					curcolor=color2;
					break;
				case 3:
					curval=(pThis->m_Params.PointsForWeb[pos].connections * scalefactor);
					curcolor=color3;
					break;
			}
			DrawLineInCxImage(cImage,x,curval,x+1,oldval,curcolor);
			oldval=curval;
		}
	}

	//Creates a border
	for(int x = 0; x < width; x++)
	{
		cImage->SetPixelColor(x,0,0);
		cImage->SetPixelColor(x,height-1,0);
	}
	for(int y = 0; y < height; y++)
	{
		cImage->SetPixelColor(0,y,0);
		cImage->SetPixelColor(width-1,y,0);
	}


	BYTE * buffer = NULL;
	long size = 0;

	CString contenttype;

	cImage->SetTransIndex(-1);
	SendSmallestCxImage(Data, cImage);

	delete cImage;
}

void CWapServer::SendScriptFile(WapThreadData Data){
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return;

	if(!BrowserAccept(Data,"text/vnd.wap.wmlscript")){
		CString Out;
		Out ="<?xml version=\"1.0\" encoding=\""+ _GetWebCharSet() +"\"?>";
		Out +="<!DOCTYPE wml PUBLIC \"-//WAPFORUM//DTD WML 1.1//EN\" \"http://www.wapforum.org/DTD/wml_1.1.xml\">";
		Out +="<wml><card><p>";
		Out +="Error: This browser do not support WMLScript";
		Out +="</p></card></wml>";
		Data.pSocket->SendContent(WMLInit, Out, Out.GetLength());
		return;
	}

	CString wmlsStr, question;
	
	wmlsStr = pThis->m_Templates.sScriptFile;
	wmlsStr.Replace("[Yes]",RemoveWMLScriptInvalidChars(_GetPlainResString(IDS_YES)));
	wmlsStr.Replace("[No]",RemoveWMLScriptInvalidChars(_GetPlainResString(IDS_NO)));
	wmlsStr.Replace("[ConfirmRemoveServer]", RemoveWMLScriptInvalidChars(_GetPlainResString(IDS_WEB_CONFIRM_REMOVE_SERVER)));

	if(!wmlsStr.IsEmpty()) wmlsStr+="\n\n";

	//Confirm cancel
	question=RemoveWMLScriptInvalidChars(_GetPlainResStringNoQuote(IDS_Q_CANCELDL2));
	question.Replace("\\n","");
	wmlsStr += "extern function confirmcancel(curl) {\n"
			"var a = Dialogs.confirm(\"" + 
			question + "\", "
			"\"" + RemoveWMLScriptInvalidChars(_GetPlainResString(IDS_YES)) + "\", " +
			"\"" + RemoveWMLScriptInvalidChars(_GetPlainResString(IDS_NO)) + "\");\n"
			"if(a) WMLBrowser.go(curl);\n"
			"}\n\n";

	//Access denied
	wmlsStr += "extern function accessdenied() {\n"
			"var a = Dialogs.alert(\"" +
			RemoveWMLScriptInvalidChars(_GetPlainResString(IDS_ACCESSDENIED)) + "\");\n"
			"}\n\n";

	//Shows an ed2klink
	wmlsStr += "extern function showed2k(fname,hash,size) {\n"
			"var ed2k = \"ed2k://|file|\" + fname + \"|\" + size + \"|\" + hash + \"|/\";\n"
			"var a = Dialogs.alert(ed2k);\n"
			"}";

	Data.pSocket->SendContent("Server: eMule\r\nConnection: close\r\nContent-Type: text/vnd.wap.wmlscript\r\n", wmlsStr, wmlsStr.GetLength());
}

bool CWapServer::BrowserAccept(WapThreadData Data, CString sAccept){
	int curPos= 0;
	
	CString header(Data.pSocket->m_pBuf,Data.pSocket->m_dwHttpHeaderLen);
	header.Replace("\r\n","\n");

	CString resToken;
	resToken=header.Tokenize("\n",curPos);
	while (resToken != "")
	{
		if(resToken.Left(7).MakeLower()=="accept:")
			if(CString(resToken).MakeLower().Find(sAccept.MakeLower())!=-1)
				return true;

		resToken = header.Tokenize("\n",curPos);
	};
	return false;		
}

void CWapServer::SendImageFile(WapThreadData Data, CString filename){
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if (pThis == NULL) return;

	if(!thePrefs.GetWapSendImages()){
		Data.pSocket->SendError404();
		return;
	}

	bool png=false, gif=false;
	bool sended = false;		

	png=BrowserAccept(Data,"image/png");
	gif=BrowserAccept(Data,"image/gif");
	if(!(png || gif) && BrowserAccept(Data,"image/*")){
		png=true;
		gif=true;
	}

	CString pngFile = filename.Left(filename.ReverseFind('.')) + ".png";
	CString gifFile = filename.Left(filename.ReverseFind('.')) + ".gif";
	CString wbmpFile = filename.Left(filename.ReverseFind('.')) + ".wbmp";

	bool pngExist,gifExist,wbmpExist;
	pngExist=FileExist(pngFile);
	gifExist=FileExist(gifFile);
	wbmpExist=FileExist(wbmpFile);

	if(thePrefs.GetWapAllwaysSendBWImages()){
		filename=wbmpFile;
	}

	CxImage *cImage;
	BYTE * buffer = NULL;
	long size = 0;
	CString moreContentType = "Last-Modified: " + pThis->m_Params.sLastModified + "\r\n" + "ETag: " + pThis->m_Params.sETag + "\r\n";

	if(!(png || gif) || thePrefs.GetWapAllwaysSendBWImages()){
		if(wbmpExist){
			cImage = new CxImage;
			if(cImage && cImage->Load(wbmpFile,CXIMAGE_FORMAT_WBMP)){
				BYTE * bufferPNG = NULL;
				BYTE * bufferGIF = NULL;
				long sizePNG = 0, sizeGIF = 0, sizeWBMP = 0;

				if(png) cImage->Encode(bufferPNG, sizePNG, CXIMAGE_FORMAT_PNG);
				
				if(gif){
					cImage->SetCodecOption(2); //LZW
					cImage->Encode(bufferGIF, sizeGIF, CXIMAGE_FORMAT_GIF);
				}

				sizeWBMP=FileSize(wbmpFile);

				if(bufferPNG && bufferGIF && (sizePNG<=sizeGIF) && (sizePNG<=sizeWBMP)){
					Data.pSocket->SendContent("Server: eMule\r\nCache-Control: public\r\nContent-Type: image/png\r\n" + moreContentType, bufferPNG, sizePNG);
				}
				else if(bufferPNG && bufferGIF && (sizeGIF<=sizePNG) && (sizeGIF<=sizeWBMP)){
					Data.pSocket->SendContent("Server: eMule\r\nCache-Control: public\r\nContent-Type: image/gif\r\n" + moreContentType, bufferGIF, sizeGIF);
				}
				else if(bufferPNG && (sizePNG<=sizeWBMP)){
					Data.pSocket->SendContent("Server: eMule\r\nCache-Control: public\r\nContent-Type: image/png\r\n" + moreContentType, bufferPNG, sizePNG);
				}
				else if(bufferGIF && (sizeGIF<=sizeWBMP)){
					Data.pSocket->SendContent("Server: eMule\r\nCache-Control: public\r\nContent-Type: image/gif\r\n" + moreContentType, bufferGIF, sizeGIF);
				}
				else {
					SendFile(Data,wbmpFile,"Server: eMule\r\nCache-Control: public\r\nContent-Type: image/vnd.wap.wbmp\r\n" + moreContentType);
				}

				delete[] bufferPNG;
				delete[] bufferGIF;
				delete cImage;
			}
		}
		else if(pngExist){
			cImage = new CxImage;
			if(cImage && cImage->Load(pngFile,CXIMAGE_FORMAT_PNG)){
				SendSmallestCxImage(Data,cImage);
				delete cImage;
			}
		}
		else if(gifExist){
			cImage = new CxImage;
			if(cImage && cImage->Load(gifFile,CXIMAGE_FORMAT_GIF)){
				SendSmallestCxImage(Data,cImage);
				delete cImage;
			}
		}
	}
	else {
		if(png && gif){
			if(pngExist && gifExist){
				if(FileSize(pngFile)<FileSize(gifFile))
					SendFile(Data,pngFile,"Server: eMule\r\nCache-Control: public\r\nContent-Type: image/png\r\n" + moreContentType);
				else
					SendFile(Data,gifFile,"Server: eMule\r\nCache-Control: public\r\nContent-Type: image/gif\r\n" + moreContentType);
			}
			else if(pngExist)
				SendFile(Data,pngFile,"Server: eMule\r\nCache-Control: public\r\nContent-Type: image/png\r\n" + moreContentType);
			else if(gifExist)
				SendFile(Data,gifFile,"Server: eMule\r\nCache-Control: public\r\nContent-Type: image/gif\r\n" + moreContentType);
			else if(wbmpExist)
				SendFile(Data,wbmpFile,"Server: eMule\r\nCache-Control: public\r\nContent-Type: image/vnd.wap.wbmp\r\n" + moreContentType);
			else
				Data.pSocket->SendError404();
		}
		else if(png || gif){
			if(png && pngExist)
				SendFile(Data,pngFile,"Server: eMule\r\nCache-Control: public\r\nContent-Type: image/png\r\n" + moreContentType);
			else if(gif && gifExist)
				SendFile(Data,gifFile,"Server: eMule\r\nCache-Control: public\r\nContent-Type: image/gif\r\n" + moreContentType);
			else if(wbmpExist)
				SendFile(Data,wbmpFile,"Server: eMule\r\nCache-Control: public\r\nContent-Type: image/vnd.wap.wbmp\r\n" + moreContentType);
			else{
				if(pngExist){
					cImage = new CxImage;
					if(cImage && cImage->Load(pngFile,CXIMAGE_FORMAT_PNG)){
						cImage->DecreaseBpp(1,true);
						SendSmallestCxImage(Data, cImage);
						delete cImage;
					}
				}
				if(gifExist){
					cImage = new CxImage;
					if(cImage && cImage->Load(gifFile,CXIMAGE_FORMAT_GIF)){
						cImage->DecreaseBpp(1,true);
						SendSmallestCxImage(Data, cImage);
						delete cImage;
					}
				}
				else{
					Data.pSocket->SendError404();
				}
			}
		}
		else{
			if(wbmpExist)
				SendFile(Data,wbmpFile,"Server: eMule\r\nCache-Control: public\r\nContent-Type: image/vnd.wap.wbmp\r\n" + moreContentType);
			else {
				if(pngExist){
					cImage = new CxImage;
					if(cImage && cImage->Load(pngFile,CXIMAGE_FORMAT_PNG)){
						cImage->DecreaseBpp(1,true);
						SendSmallestCxImage(Data, cImage);
						delete cImage;
					}
				}
				if(gifExist){
					cImage = new CxImage;
					if(cImage && cImage->Load(gifFile,CXIMAGE_FORMAT_GIF)){
						cImage->DecreaseBpp(1,true);
						SendSmallestCxImage(Data, cImage);
						delete cImage;
					}
				}
				else{
					Data.pSocket->SendError404();
				}
			}
		}
	}
}

void CWapServer::SendProgressBar(WapThreadData Data, CString filehash)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return;

	CPartFile* cur_file;

	uchar fileid[16];
	if(filehash.GetLength()!=32 || !DecodeBase16(filehash.GetBuffer(),filehash.GetLength(),fileid,ARRSIZE(fileid)))
		return;
	
	CxImage progressImage;
	bool png=false, gif=false;

	png=BrowserAccept(Data, "image/png");
	gif=BrowserAccept(Data, "image/gif");
	if(!(png || gif) && BrowserAccept(Data,"image/*")){
		png=true;
		gif=true;
	}

	// cool style
	RGBQUAD progresscolor[16];
	progresscolor[0]=progressImage.RGBtoRGBQUAD(0x0000c000);		//green
	progresscolor[1]=progressImage.RGBtoRGBQUAD(0x00000000);		//black
	progresscolor[2]=progressImage.RGBtoRGBQUAD(0x0000ffff);		//yellow
	progresscolor[3]=progressImage.RGBtoRGBQUAD(0x000000ff);		//red
	progresscolor[4]=progressImage.RGBtoRGBQUAD(0x00ffd200);		//blue1
	progresscolor[5]=progressImage.RGBtoRGBQUAD(0x00ffb100);		//blue2
	progresscolor[6]=progressImage.RGBtoRGBQUAD(0x00ff8500);		//blue3
	progresscolor[7]=progressImage.RGBtoRGBQUAD(0x00ff5900);		//blue4
	progresscolor[8]=progressImage.RGBtoRGBQUAD(0x00ff2d00);		//blue5
	progresscolor[9]=progressImage.RGBtoRGBQUAD(0x00ff0000);		//blue6
	for(int i=10; i<16; i++)
		progresscolor[i]=progressImage.RGBtoRGBQUAD(0x00FFFFFF);	//white

	CString s_ChunkBar;
	cur_file=theApp.downloadqueue->GetFileByID(fileid);
	if (cur_file==0 || !cur_file->IsPartFile()) {
		for(int i=0;i<thePrefs.GetWapGraphWidth()-2;i++)
			s_ChunkBar+="0";
	}
	else
	{
		s_ChunkBar=cur_file->GetProgressString(thePrefs.GetWapGraphWidth()-1);
	}

	if((png || gif) && !thePrefs.GetWapAllwaysSendBWImages()){
		//Single Color Bar (13px = 8px (progress) + 2px (percent) + 3px (border))
		progressImage.Create(thePrefs.GetWapGraphWidth(),13,4,0);
		progressImage.SetPalette(progresscolor,16);
	}else{
		//Double B/N Bar (22px = 8px (progress1) + 8px (progress2) + 2px (percent) + 2px (ext_border) + 2px (int_borders))
		progressImage.Create(thePrefs.GetWapGraphWidth(),22,24,0);
	}

	progressImage.Clear(0xFF);

	// and now make a graph out of the array - need to be in a progressive way
	int curcolor;
	for (uint16 i=1;i<progressImage.GetWidth()-1;i++) {
		curcolor = int(s_ChunkBar.GetAt(i-1)) - int('0');
		if(curcolor>9) MessageBox(NULL,"Ups","Ups",MB_OK);
		if((png || gif) && !thePrefs.GetWapAllwaysSendBWImages())
		{
			for(int y=0;y<10;y++)
		        progressImage.SetPixelIndex(i,y,curcolor);
		}
		else
		{
			if(curcolor==0 || curcolor==1){ //DOWNLOADED
				for(int y=10;y<18;y++)
			        progressImage.SetPixelColor(i,y,0);
			}

			if(curcolor!=3){ //!RED
				for(int y=0;y<9;y++)
			        progressImage.SetPixelColor(i,y,0);
			}
		}
	}

	int compl;
	if(cur_file)
		compl=(int)(((thePrefs.GetWapGraphWidth()-2)/100.0)*cur_file->GetPercentCompleted());
	else
		compl=thePrefs.GetWapGraphWidth()-2;

	if((png || gif) && !thePrefs.GetWapAllwaysSendBWImages())
	{
		for(int x=1; x <= compl; x++){
		    progressImage.SetPixelColor(x,progressImage.GetHeight()-2,progresscolor[0]);
			progressImage.SetPixelColor(x,progressImage.GetHeight()-3,progresscolor[0]);
		}
	}
	else
	{
		for(int x=1; x <= compl; x++){
		    progressImage.SetPixelColor(x,progressImage.GetHeight()-2,0);
			progressImage.SetPixelColor(x,progressImage.GetHeight()-3,0);
		}
	}

	//Creates a border
	for(unsigned int x = 0; x < progressImage.GetWidth(); x++)
	{
		progressImage.SetPixelColor(x,0,0);
		progressImage.SetPixelColor(x,progressImage.GetHeight()-1,0);
	}
	for(unsigned int y = 0; y < progressImage.GetHeight(); y++)
	{
		progressImage.SetPixelColor(0,y,0);
		progressImage.SetPixelColor(progressImage.GetWidth()-1,y,0);
	}
	
	//Draws internals separators.
	if((png || gif) && !thePrefs.GetWapAllwaysSendBWImages()){
		for(unsigned int x = 0; x < progressImage.GetWidth(); x++)
		{
			progressImage.SetPixelColor(x,progressImage.GetHeight()-4,0);
		}
	} else {
		for(unsigned int x = 0; x < progressImage.GetWidth(); x+=2)
		{
			progressImage.SetPixelColor(x,progressImage.GetHeight()-4,0);
			progressImage.SetPixelColor(x,progressImage.GetHeight()-13,0);
		}
	}

	BYTE * buffer = NULL;
	long size = 0;

	//Select the smallest file
	progressImage.SetTransIndex(-1);
	SendSmallestCxImage(Data, &progressImage);
}

bool CWapServer::FileExist(CString fileName){
	CFile file;
	CFileException e;
	if(file.Open(fileName, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary, &e))
	{
		file.Close();
		return true;
	}
	return false;
}

long CWapServer::FileSize(CString fileName){
	CFile file;
	CFileException e;
	long size=0;
	
	if(file.Open(fileName, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary, &e))
	{
		size=file.GetLength();
		file.Close();
		return size;
	}
	return 0;
}

bool CWapServer::SendFile(WapThreadData Data, CString fileName, CString contentType){
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return false;

	CFile file;
	if(file.Open(fileName, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary))
	{
		char* buffer=new char[file.GetLength()];
		int size=file.Read(buffer,file.GetLength());
		file.Close();
		Data.pSocket->SendContent(contentType, buffer, size);
		delete[] buffer;
		return true;
	}
	return false;
}

void CWapServer::SendSmallestCxImage(WapThreadData Data, CxImage *cImage){
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return;

	bool png=false, gif=false, bwImage;
	ENUM_WAP_IMAGE_FORMAT imageToSend = WITHOUT_IMAGE;

	png=BrowserAccept(Data, "image/png");
	gif=BrowserAccept(Data, "image/gif");
	if(!(png || gif) && BrowserAccept(Data,"image/*")){
		png=true;
		gif=true;
	}
	bwImage=(thePrefs.GetWapAllwaysSendBWImages() || !(png || gif));

	if(bwImage && (cImage->GetBpp()>1)){
		cImage->DecreaseBpp(1,true);
	}

	if(!bwImage && (cImage->GetBpp()>4)){
		cImage->DecreaseBpp(4,true);
	}
	
	long sizePNG = 0, sizeGIF = 0, sizeWBMP = 0;
	BYTE * bufferPNG = NULL;
	BYTE * bufferGIF = NULL;
	BYTE * bufferWBMP = NULL;
	
	if(png) cImage->Encode(bufferPNG, sizePNG, CXIMAGE_FORMAT_PNG);
	
	if(gif){
		cImage->SetCodecOption(2); //LZW
		cImage->Encode(bufferGIF, sizeGIF, CXIMAGE_FORMAT_GIF);
	}

	cImage->DecreaseBpp(1,true);
	cImage->Encode(bufferWBMP, sizeWBMP, CXIMAGE_FORMAT_WBMP);

	if(bwImage){
		if(bufferGIF && bufferPNG && bufferWBMP && (sizeGIF<=sizePNG) && (sizeGIF<=sizeWBMP)){
			imageToSend = GIF_IMAGE;
		}else if(bufferGIF && bufferPNG && bufferWBMP && (sizePNG<=sizeGIF) && (sizePNG<=sizeWBMP)){
			imageToSend = PNG_IMAGE;
		}else if(bufferGIF && bufferWBMP && (sizeGIF<=sizeWBMP)){
			imageToSend = GIF_IMAGE;
		}else if(bufferPNG && bufferWBMP && (sizePNG<=sizeWBMP)){
			imageToSend = PNG_IMAGE;
		}else if(bufferWBMP){
			imageToSend = WBMP_IMAGE;
		}else if(!bufferWBMP){ //???
			if(bufferPNG && bufferGIF){
				if (sizeGIF<=sizePNG){
					imageToSend = GIF_IMAGE;
				}else{
					imageToSend = PNG_IMAGE;
				}
			} else if(bufferPNG){
				imageToSend = PNG_IMAGE;
			} else if(bufferGIF){
				imageToSend = GIF_IMAGE;
			} else {
				imageToSend = WITHOUT_IMAGE;
			}
		}
	} else {
		//!bwImage
		if(bufferGIF && bufferPNG && (sizeGIF<=sizePNG)){
			imageToSend = GIF_IMAGE;
		}else if(bufferGIF && bufferPNG && (sizePNG<=sizeGIF)){
			imageToSend = PNG_IMAGE;
		}else if(bufferGIF){
			imageToSend = GIF_IMAGE;
		}else if(bufferPNG){
			imageToSend = PNG_IMAGE;
		}else if(bufferWBMP){
			imageToSend = WBMP_IMAGE;
		}else{
			imageToSend = WITHOUT_IMAGE;
		}
	}

	switch(imageToSend){
		case GIF_IMAGE:
			//AddDebugLogLine(false, "Wap Server: Send GIF CxImage");
			Data.pSocket->SendContent("Server: eMule\r\nConnection: close\r\nCache-Control: no-cache\r\nContent-Type: image/gif\r\n", bufferGIF, sizeGIF);
			break;
		case PNG_IMAGE:
			//AddDebugLogLine(false, "Wap Server: Send PNG CxImage");
			Data.pSocket->SendContent("Server: eMule\r\nConnection: close\r\nCache-Control: no-cache\r\nContent-Type: image/png\r\n", bufferPNG, sizePNG);
			break;
		case WBMP_IMAGE:
			//AddDebugLogLine(false, "Wap Server: Send WBMP CxImage");
			Data.pSocket->SendContent("Server: eMule\r\nConnection: close\r\nCache-Control: no-cache\r\nContent-Type: image/vnd.wap.wbmp\r\n", bufferWBMP, sizeWBMP);
			break;
		default:
			AddDebugLogLine(false, "Wap Server: Error sending CxImage -> No image send");
			Data.pSocket->SendError404();
			break;
	}

	delete[] bufferPNG;
	delete[] bufferGIF;
	delete[] bufferWBMP;
}

CString CWapServer::RemoveWMLScriptInvalidChars(CString input)
{
	//This function removes accents, and chars >= 128 (ASCII)
	//because some mobiles has problems with those chars in wmlscripts.
	CString tmpStr;
	tmpStr = input;
	for(int i=0; i<input.GetLength(); i++){ 
		char c;
		c = input.GetAt(i);
		switch (c){
			//Lower Case:
			case 'á':
			case 'ä':
			case 'à':
			case 'â':
				c = 'a';
				break;
			case 'é':
			case 'ë':
			case 'è':
			case 'ê':
				c = 'e';
				break;
			case 'í':
			case 'ï':
			case 'ì':
			case 'î':
				c = 'i';
				break;
			case 'ó':
			case 'ö':
			case 'ò':
			case 'ô':
				c = 'o';
				break;
			case 'ú':
			case 'ü':
			case 'ù':
			case 'û':
				c = 'u';
				break;
			case 'ý':
			case 'ÿ':
				c = 'y';
				break;
			case 'ñ':
				c = 'n';
				break;

			//Upper Case:
			case 'Á':
			case 'Ä':
			case 'À':
			case 'Â':
				c = 'A';
				break;
			case 'É':
			case 'Ë':
			case 'È':
			case 'Ê':
				c = 'E';
				break;
			case 'Í':
			case 'Ï':
			case 'Ì':
			case 'Î':
				c = 'I';
				break;
			case 'Ó':
			case 'Ö':
			case 'Ò':
			case 'Ô':
				c = 'O';
				break;
			case 'Ú':
			case 'Ü':
			case 'Ù':
			case 'Û':
				c = 'U';
				break;
			case 'Ý':
				c = 'Y';
				break;
			case 'Ñ':
				c = 'N';
				break;

			//Default Chars
			default:
				if((unsigned char)c >= 128)
					c = '_';
				break;
		}
		tmpStr.SetAt(i,c);
	}
	return tmpStr;
}

CString	CWapServer::_GetKadPage(WapThreadData Data)
{
	CWapServer *pThis = (CWapServer *)Data.pThis;
	if(pThis == NULL)
		return "";

	if (!thePrefs.GetNetworkKademlia()) return "";

	CString sSession = _ParseURL(Data.sURL, "ses");
	CString Out = pThis->m_Templates.sKad;
	Out.Replace("[Session]", sSession);

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
			
			buffer.Format("%s: %i<br/>", _GetPlainResString(IDS_KADCONTACTLAB), theApp.emuledlg->kademliawnd->contactList->GetItemCount());
			info.Append(buffer);

			buffer.Format("%s: %i<br/>", _GetPlainResString(IDS_KADSEARCHLAB), theApp.emuledlg->kademliawnd->searchList->GetItemCount());
			info.Append(buffer);

		
	} else info="";
	Out.Replace("[KADSTATSDATA]",info);

	Out.Replace("[BS_IP]",_GetPlainResString(IDS_IP));
	Out.Replace("[BS_PORT]",_GetPlainResString(IDS_PORT));
	Out.Replace("[BOOTSTRAP]",_GetPlainResString(IDS_BOOTSTRAP));
	Out.Replace("[KADSTAT]",_GetPlainResString(IDS_STATSSETUPINFO));
	Out.Replace("[Section]", _GetPlainResString(IDS_KADEMLIA));
	Out.Replace("[Home]", _GetPlainResString(IDS_WAP_HOME));
	Out.Replace("[Back]", _GetPlainResString(IDS_WAP_BACK));

	return Out;
}