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
#include "emule.h"
#include "OtherFunctions.h"
#include <zlib/zlib.h>
#include "SearchDlg.h"
#include "SearchParams.h"
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
#include "Exceptions.h"
#include "Opcodes.h"
#include "StringConversion.h"
#include "Log.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define HTTPInit _T("Server: eMule\r\nConnection: close\r\nContent-Type: text/html\r\n")
#define HTTPInitGZ _T("Server: eMule\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Encoding: gzip\r\n")

#define WEB_SERVER_TEMPLATES_VERSION	5

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
	CString sPrevLocale(_tsetlocale(LC_TIME,NULL));
	_tsetlocale(LC_TIME, _T("English"));
	CTime t = GetCurrentTime();
	m_Params.sLastModified = t.FormatGmt(_T("%a, %d %b %Y %H:%M:%S GMT"));
	m_Params.sETag = MD5Sum(m_Params.sLastModified).GetHash();
	_tsetlocale(LC_TIME, sPrevLocale);

	CString sFile;
	if (thePrefs.GetTemplate()==_T("") || thePrefs.GetTemplate().MakeLower()==_T("emule.tmpl"))
		sFile= thePrefs.GetAppDir() + CString(_T("eMule.tmpl"));
	else sFile=thePrefs.GetTemplate();

	CStdioFile file;
	if(file.Open(sFile, CFile::modeRead|CFile::typeText|CFile::shareDenyWrite))
	{
		CString sAll;
		for(;;)
		{
			CString sLine;
			if(!file.ReadString(sLine))
				break;

			sAll += sLine + _T("\n");
		}
		file.Close();

		CString sVersion = _LoadTemplate(sAll,_T("TMPL_VERSION"));
		long lVersion = _tstol(sVersion);
		if(lVersion < WEB_SERVER_TEMPLATES_VERSION)
		{
			if(m_bServerWorking)
				LogError(LOG_STATUSBAR,GetResString(IDS_WS_ERR_LOADTEMPLATE),sFile);
		}
		else
		{
			m_Templates.sHeader = _LoadTemplate(sAll,_T("TMPL_HEADER_KAD"));
			m_Templates.sHeaderMetaRefresh = _LoadTemplate(sAll,_T("TMPL_HEADER_META_REFRESH"));
			m_Templates.sHeaderStylesheet = _LoadTemplate(sAll,_T("TMPL_HEADER_STYLESHEET"));
			m_Templates.sFooter = _LoadTemplate(sAll,_T("TMPL_FOOTER"));
			m_Templates.sServerList = _LoadTemplate(sAll,_T("TMPL_SERVER_LIST"));
			m_Templates.sServerLine = _LoadTemplate(sAll,_T("TMPL_SERVER_LINE"));
			m_Templates.sTransferImages = _LoadTemplate(sAll,_T("TMPL_TRANSFER_IMAGES"));
			m_Templates.sTransferList = _LoadTemplate(sAll,_T("TMPL_TRANSFER_LIST"));
			m_Templates.sTransferDownHeader = _LoadTemplate(sAll,_T("TMPL_TRANSFER_DOWN_HEADER"));
			m_Templates.sTransferDownFooter = _LoadTemplate(sAll,_T("TMPL_TRANSFER_DOWN_FOOTER"));
			m_Templates.sTransferDownLine = _LoadTemplate(sAll,_T("TMPL_TRANSFER_DOWN_LINE"));
			m_Templates.sTransferDownLineGood = _LoadTemplate(sAll,_T("TMPL_TRANSFER_DOWN_LINE_GOOD"));
			m_Templates.sTransferUpHeader = _LoadTemplate(sAll,_T("TMPL_TRANSFER_UP_HEADER"));
			m_Templates.sTransferUpFooter = _LoadTemplate(sAll,_T("TMPL_TRANSFER_UP_FOOTER"));
			m_Templates.sTransferUpLine = _LoadTemplate(sAll,_T("TMPL_TRANSFER_UP_LINE"));
			m_Templates.sTransferUpQueueShow = _LoadTemplate(sAll,_T("TMPL_TRANSFER_UP_QUEUE_SHOW"));
			m_Templates.sTransferUpQueueHide = _LoadTemplate(sAll,_T("TMPL_TRANSFER_UP_QUEUE_HIDE"));
			m_Templates.sTransferUpQueueLine = _LoadTemplate(sAll,_T("TMPL_TRANSFER_UP_QUEUE_LINE"));
			m_Templates.sTransferBadLink = _LoadTemplate(sAll,_T("TMPL_TRANSFER_BAD_LINK"));
			m_Templates.sDownloadLink = _LoadTemplate(sAll,_T("TMPL_DOWNLOAD_LINK"));
			m_Templates.sSharedList = _LoadTemplate(sAll,_T("TMPL_SHARED_LIST"));
			m_Templates.sSharedLine = _LoadTemplate(sAll,_T("TMPL_SHARED_LINE"));
			m_Templates.sSharedLineChanged = _LoadTemplate(sAll,_T("TMPL_SHARED_LINE_CHANGED"));
			m_Templates.sGraphs = _LoadTemplate(sAll,_T("TMPL_GRAPHS"));
			m_Templates.sLog = _LoadTemplate(sAll,_T("TMPL_LOG"));
			m_Templates.sServerInfo = _LoadTemplate(sAll,_T("TMPL_SERVERINFO"));
			m_Templates.sDebugLog = _LoadTemplate(sAll,_T("TMPL_DEBUGLOG"));
			m_Templates.sStats = _LoadTemplate(sAll,_T("TMPL_STATS"));
			m_Templates.sPreferences = _LoadTemplate(sAll,_T("TMPL_PREFERENCES_KAD"));
			m_Templates.sLogin = _LoadTemplate(sAll,_T("TMPL_LOGIN"));
			//MORPH START - Added by SiRoB/Commander, Login Failed from eMule+
			m_Templates.sFailedLogin = _LoadTemplate(sAll,_T("TMPL_FAILEDLOGIN"));
			//MORPH END   - Added by SiRoB/Commander, Login Failed from eMule+
			m_Templates.sConnectedServer = _LoadTemplate(sAll,_T("TMPL_CONNECTED_SERVER"));
			m_Templates.sAddServerBox = _LoadTemplate(sAll,_T("TMPL_ADDSERVERBOX"));
			m_Templates.sWebSearch = _LoadTemplate(sAll,_T("TMPL_WEBSEARCH"));
			m_Templates.sSearch = _LoadTemplate(sAll,_T("TMPL_SEARCH_KAD"));
			m_Templates.iProgressbarWidth=_tstoi(_LoadTemplate(sAll,_T("PROGRESSBARWIDTH")));
			m_Templates.sSearchHeader = _LoadTemplate(sAll,_T("TMPL_SEARCH_RESULT_HEADER"));			
			m_Templates.sSearchResultLine = _LoadTemplate(sAll,_T("TMPL_SEARCH_RESULT_LINE"));			
			m_Templates.sProgressbarImgs = _LoadTemplate(sAll,_T("PROGRESSBARIMGS"));
			m_Templates.sProgressbarImgsPercent = _LoadTemplate(sAll,_T("PROGRESSBARPERCENTIMG"));
			m_Templates.sClearCompleted = _LoadTemplate(sAll,_T("TMPL_TRANSFER_DOWN_CLEARBUTTON"));
			m_Templates.sCatArrow= _LoadTemplate(sAll,_T("TMPL_CATARROW"));
			m_Templates.sBootstrapLine= _LoadTemplate(sAll,_T("TMPL_BOOTSTRAPLINE"));
			m_Templates.sKad= _LoadTemplate(sAll,_T("TMPL_KADDLG"));

			m_Templates.sProgressbarImgsPercent.Replace(_T("[PROGRESSGIFNAME]"),_T("%s"));
			m_Templates.sProgressbarImgsPercent.Replace(_T("[PROGRESSGIFINTERNAL]"),_T("%i"));
			m_Templates.sProgressbarImgs.Replace(_T("[PROGRESSGIFNAME]"),_T("%s"));
			m_Templates.sProgressbarImgs.Replace(_T("[PROGRESSGIFINTERNAL]"),_T("%i"));			
		}
	}
	else
        if(m_bServerWorking)
			LogError(LOG_STATUSBAR,GetResString(IDS_WEB_ERR_CANTLOAD), sFile);

}

CWebServer::~CWebServer(void)
{
	if (m_bServerWorking) StopSockets();
}

CString CWebServer::_LoadTemplate(CString sAll, CString sTemplateName)
{
	CString sRet = _T("");
	int nStart = sAll.Find(_T("<--") + sTemplateName + _T("-->"));
	int nEnd = sAll.Find(_T("<--") + sTemplateName + _T("_END-->"));
	if(nStart != -1 && nEnd != -1 && nStart<nEnd)
	{
		nStart += sTemplateName.GetLength() + 7;
		sRet = sAll.Mid(nStart, nEnd - nStart - 1);
	} else{
		if (sTemplateName==_T("TMPL_VERSION"))
			LogError(LOG_STATUSBAR,GetResString(IDS_WS_ERR_LOADTEMPLATE),sTemplateName);
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
		AddLogLine(false,_T("%s: %s"),_GetPlainResString(IDS_PW_WS), _GetPlainResString(IDS_ENABLED));
	else
		AddLogLine(false,_T("%s: %s"),_GetPlainResString(IDS_PW_WS), _GetPlainResString(IDS_DISABLED));


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
		//MORPH START - Added by SiRoB, force savepart to update auto up flag since i removed the update in UpdateAutoUpPriority optimization
		if(cur_file->IsPartFile() && !cur_file->IsAutoUpPriority()){
			cur_file->SetAutoUpPriority(true);		
			((CPartFile*)cur_file)->SavePartFile();
		}else
		//MORPH END   - Added by SiRoB, force savepart to update auto up flag since i removed the update in UpdateAutoUpPriority optimization
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

__inline void PlainString(CString& rstr, bool noquot)
{
	rstr.Replace(_T("&"), _T("&amp;"));
	rstr.Replace(_T("<"), _T("&lt;"));
	rstr.Replace(_T(">"), _T("&gt;"));
	rstr.Replace(_T("\""), _T("&quot;"));
	if(noquot)
	{
        rstr.Replace(_T("'"), _T("\\'"));
		rstr.Replace(_T("\n"), _T("\\n"));
	}
}

CString CWebServer::_SpecialChars(CString str) 
{
	PlainString(str,false);
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

	if (filename.Right(4).MakeLower()==_T(".gif")) contenttype=_T("Content-Type: image/gif\r\n");
		else if (filename.Right(4).MakeLower()==_T(".jpg")  || filename.Right(5).MakeLower()==_T(".jpeg")) contenttype=_T("Content-Type: image/jpg\r\n");
		else if (filename.Right(4).MakeLower()==_T(".bmp")) contenttype=_T("Content-Type: image/bmp\r\n");
		else if (filename.Right(4).MakeLower()==_T(".png")) contenttype=_T("Content-Type: image/png\r\n");
		//DonQ - additional filetypes
		else if (filename.Right(4).MakeLower()==_T(".ico")) contenttype=_T("Content-Type: image/x-icon\r\n");
		else if (filename.Right(4).MakeLower()==_T(".css")) contenttype=_T("Content-Type: text/css\r\n");
		else if (filename.Right(3).MakeLower()==_T(".js")) contenttype=_T("Content-Type: text/javascript\r\n");
		
	contenttype += _T("Last-Modified: ") + pThis->m_Params.sLastModified + _T("\r\n") + _T("ETag: ") + pThis->m_Params.sETag + _T("\r\n");
	
	filename.Replace(_T('/'),_T('\\'));

	if (filename.GetAt(0)==_T('\\')) filename.Delete(0);
	filename=thePrefs.GetWebServerDir()+filename;
	CFile file;
	if(file.Open(filename, CFile::modeRead|CFile::shareDenyWrite|CFile::typeBinary))
	{
		USES_CONVERSION;
		char* buffer=new char[file.GetLength()];
		int size=file.Read(buffer,file.GetLength());
		file.Close();
		Data.pSocket->SendContent(T2CA(contenttype), buffer, size);
		delete[] buffer;
	}
}

void CWebServer::ProcessURL(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return;

	//////////////////////////////////////////////////////////////////////////
	// Here we are in real trouble! We are accessing the entire emule main thread
	// data without any syncronization!! Either we use the message pump for emuledlg
	// or use some hundreds of critical sections... For now, an exception handler
	// shoul avoid the worse things.
	//////////////////////////////////////////////////////////////////////////
	CoInitialize(NULL);

	try
	{
		USES_CONVERSION;
		bool isUseGzip = thePrefs.GetWebUseGzip();
		CString Out = _T("");
		CString OutE = _T("");	// List Entry Templates
		CString OutE2 = _T("");
		CString OutS = _T("");	// ServerStatus Templates
		TCHAR *gzipOut = NULL;
		long gzipLen=0;
		
		bool login=false;
        bool justAddLink = false;
		
		CString HTTPProcessData = _T("");
		CString HTTPTemp = _T("");
		//char	HTTPTempC[100] = _T("");
		srand ( time(NULL) );

		long lSession = 0;
		if(_ParseURL(Data.sURL, _T("ses")) != _T(""))
			lSession = _tstol(_ParseURL(Data.sURL, _T("ses")));

		if (_ParseURL(Data.sURL, _T("w")) == _T("password"))
		{
			CString test=MD5Sum(_ParseURL(Data.sURL, _T("p"))).GetHash();
			CString ip=ipstr(Data.inadr);

            if (_ParseURL(Data.sURL, _T("c")) != _T("")) {
                // just sent password to add link remotely. Don't start a session.
                justAddLink = true;
            }

			if(MD5Sum(_ParseURL(Data.sURL, _T("p"))).GetHash() == thePrefs.GetWSPass())
			{
	            if (!justAddLink) 
	            {
                    // user wants to login
				    Session ses;
				    ses.admin=true;
				    ses.startTime = CTime::GetCurrentTime();
				    ses.lSession = lSession = GetRandomUInt32();
				    pThis->m_Params.Sessions.Add(ses);
                }
				theApp.emuledlg->serverwnd->UpdateMyInfo();
				AddLogLine(true,GetResString(IDS_WEB_ADMINLOGIN)+_T(" (%s)"),ip);
				login=true;
			}
			else if(thePrefs.GetWSIsLowUserEnabled() && thePrefs.GetWSLowPass()!=_T("") && MD5Sum(_ParseURL(Data.sURL, _T("p"))).GetHash() == thePrefs.GetWSLowPass())
			{
				Session ses;
				ses.admin=false;
				ses.startTime = CTime::GetCurrentTime();
				ses.lSession = lSession = GetRandomUInt32();
				pThis->m_Params.Sessions.Add(ses);
				theApp.emuledlg->serverwnd->UpdateMyInfo();
				AddLogLine(true,GetResString(IDS_WEB_GUESTLOGIN)+_T(" (%s)"),ip);
				login=true;
			} else {
				LogWarning(LOG_STATUSBAR,GetResString(IDS_WEB_BADLOGINATTEMPT)+_T(" (%s)"),ip);

				BadLogin newban={inet_addr(T2CA(ip)), ::GetTickCount()};	// save failed attempt (ip,time)
				pThis->m_Params.badlogins.Add(newban);
				login=false;
			}
			isUseGzip = false; // [Julien]
			if (login) {
				uint32 ipn=inet_addr(T2CA(ip)) ;
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

		CString sSession; sSession.Format(_T("%u"), lSession);

		if (_ParseURL(Data.sURL, _T("w")) == _T("logout")) 
			_RemoveSession(Data, lSession);

		if(_IsLoggedIn(Data, lSession))
		{
			Out += _GetHeader(Data, lSession);
			CString sPage = _ParseURL(Data.sURL, _T("w"));
			if (sPage == _T("server")) 
			{
				Out += _GetServerList(Data);
			}
			else
			if (sPage == _T("download")) 
			{
				Out += _GetDownloadLink(Data);
			}
			if (sPage == _T("kad")) 
			{
				Out += _GetKadPage(Data);
			}
			else
			if (sPage == _T("shared")) 
			{ 
				Out += _GetSharedFilesList(Data);
			}
			else
			if (sPage == _T("transfer")) 
			{
				Out += _GetTransferList(Data);
			}
			else
			if (sPage == _T("websearch")) 
			{
				Out += _GetWebSearch(Data);
			}
			else
			if (sPage == _T("search")) 
			{
				Out += _GetSearch(Data);
			}
			else
			if (sPage == _T("graphs"))
			{
				Out += _GetGraphs(Data);
			}
			else
			if (sPage == _T("log")) 
			{
				Out += _GetLog(Data);
			}
			if (sPage == _T("sinfo")) 
			{
				Out += _GetServerInfo(Data);
			}
			if (sPage == _T("debuglog")) 
			{
				Out += _GetDebugLog(Data);
			}
			if (sPage == _T("stats")) 
			{
				Out += _GetStats(Data);
			}
			if (sPage == _T("options")) 
			{
				Out += _GetPreferences(Data);
			}
			Out += _GetFooter(Data);

			if (sPage == _T(""))
				isUseGzip = false;

			if(isUseGzip)
			{
				bool bOk = false;
				try
				{
					const CStringA* pstrOutA;
#ifdef _UNICODE
					CStringA strA(wc2utf8(Out));
					pstrOutA = &strA;
#else
					pstrOutA = &Out;
#endif
					uLongf destLen = pstrOutA->GetLength() + 1024;
					gzipOut = new TCHAR[destLen];
					if(_GzipCompress((Bytef*)gzipOut, &destLen, (const Bytef*)(LPCSTR)*pstrOutA, pstrOutA->GetLength(), Z_DEFAULT_COMPRESSION) == Z_OK)
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
        else if(justAddLink && login)
        {
            Out += _GetRemoteLinkAddedOk(Data);
        }
		else 
		{
			isUseGzip = false;

			uint32 ip= inet_addr(T2CA(ipstr(Data.inadr)));
			uint32 faults=0;

			// check for bans
			for(int i = 0; i < pThis->m_Params.badlogins.GetSize();i++)
				if ( pThis->m_Params.badlogins[i].datalen==ip ) faults++;

			if (faults>4) {
				//MORPH START - Changed by SiRoB/Commander, FAILEDLOGIN
				/*
				Out += _GetPlainResString(IDS_ACCESSDENIED);
				*/
				Out += _GetFailedLoginScreen(Data);
				//MORPH END   - Changed by SiRoB/Commander, FAILEDLOGIN
				
				
				// set 15 mins ban by using the badlist
				BadLogin preventive={ip, ::GetTickCount() + (15*60*1000) };
				for (int i=0;i<=5;i++)
					pThis->m_Params.badlogins.Add(preventive);

			}
            else if(justAddLink)
                Out += _GetRemoteLinkAddedFailed(Data);
			else
				Out += _GetLoginScreen(Data);
		}

		// send answer ...
		if(!isUseGzip)
		{
			Data.pSocket->SendContent(T2CA(HTTPInit), Out);
		}
		else
		{
			Data.pSocket->SendContent(T2CA(HTTPInitGZ), gzipOut, gzipLen);
		}
		if(gzipOut != NULL)
			delete[] gzipOut;
	}
	catch(...){
		AddDebugLogLine( DLP_VERYHIGH, false, _T("*** Unknown exception in CWebServer::ProcessURL\n") );
		ASSERT(0);
	}

	CoUninitialize();
}

CString CWebServer::_ParseURLArray(CString URL, CString fieldname) {
	CString res,temp;

	while (URL.GetLength()>0) {
		int pos=URL.MakeLower().Find(fieldname.MakeLower() +_T("="));
		if (pos>-1) {
			temp=_ParseURL(URL,fieldname);
			if (temp==_T("")) break;
			res.Append(temp+_T("|"));
			URL.Delete(pos,10);
		} else break;
	}
	return res;
}

CString CWebServer::_ParseURL(CString URL, CString fieldname)
{
	CString value = _T("");
	CString Parameter = _T("");
	TCHAR fromReplace[4] = _T("");	// decode URL
	TCHAR toReplace[2] = _T("");		// decode URL
	int i = 0;
	int findPos = -1;
	int findLength = 0;

	if (URL.Find(_T("?")) > -1) {
		Parameter = URL.Mid(URL.Find(_T("?"))+1, URL.GetLength()-URL.Find(_T("?"))-1);

		// search the fieldname beginning / middle and strip the rest...
		if (Parameter.Find(fieldname + _T("=")) == 0) {
			findPos = 0;
			findLength = fieldname.GetLength() + 1;
		}
		if (Parameter.Find(_T("&") + fieldname + _T("=")) > -1) {
			findPos = Parameter.Find(_T("&") + fieldname + _T("="));
			findLength = fieldname.GetLength() + 2;
		}
		if (findPos > -1) {
			Parameter = Parameter.Mid(findPos + findLength, Parameter.GetLength());
			if (Parameter.Find(_T("&")) > -1) {
				Parameter = Parameter.Mid(0, Parameter.Find(_T("&")));
			}
	
			value = Parameter;

			// decode value ...
			value.Replace(_T("+"), _T(" "));
			value=URLDecode(value);
		}
	}

#ifdef _UNICODE
	CStringA strValueA;
	LPSTR pszA = strValueA.GetBuffer(value.GetLength());
	for (int i = 0; i < value.GetLength(); i++)
		*pszA++ = (CHAR)value[i];
	strValueA.ReleaseBuffer(value.GetLength());
	value = strValueA;
#endif

	return value;
}

CString CWebServer::_GetHeader(ThreadData Data, long lSession)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString buffer;
	CString sSession; sSession.Format(_T("%u"), lSession);

	CString Out = pThis->m_Templates.sHeader;

	Out.Replace(_T("[CharSet]"), _GetWebCharSet());

	if(thePrefs.GetWebPageRefresh() != 0)
	{
		CString sPage = _ParseURL(Data.sURL, _T("w"));
		if ((sPage == _T("transfer")) ||
			(sPage == _T("server")) ||
			(sPage == _T("graphs")) ||
			(sPage == _T("log")) ||
			(sPage == _T("sinfo")) ||
			(sPage == _T("debuglog")) ||
			(sPage == _T("kad")) ||
			(sPage == _T("stats")))
		{
			CString sT = pThis->m_Templates.sHeaderMetaRefresh;
			CString sRefresh; sRefresh.Format(_T("%d"), thePrefs.GetWebPageRefresh());
			sT.Replace(_T("[RefreshVal]"), sRefresh);

			CString catadd=_T("");
			if (sPage == _T("transfer"))
				catadd=_T("&cat=")+_ParseURL(Data.sURL, _T("cat"));
			sT.Replace(_T("[wCommand]"), _ParseURL(Data.sURL, _T("w"))+catadd);

			Out.Replace(_T("[HeaderMeta]"), sT);
		}
	}
	Out.Replace(_T("[Session]"), sSession);
	Out.Replace(_T("[HeaderMeta]"), _T("")); // In case there are no meta
	Out.Replace(_T("[eMuleAppName]"), _T("eMule"));
	//MORPH START - Changed by SiRoB, [itsonlyme: -modname-]
	/*
	Out.Replace(_T("[version]"), theApp.m_strCurVersionLong);
	*/
	Out.Replace(_T("[version]"), theApp.m_strCurVersionLong + _T(" [") + theApp.m_strModLongVersion + _T("]"));
	//MORPH END   - Changed by SiRoB, [itsonlyme: -modname-]
	Out.Replace(_T("[StyleSheet]"), pThis->m_Templates.sHeaderStylesheet);
	Out.Replace(_T("[WebControl]"), _GetPlainResString(IDS_WEB_CONTROL));
	Out.Replace(_T("[Transfer]"), _GetPlainResString(IDS_CD_TRANS));
	Out.Replace(_T("[Server]"), _GetPlainResString(IDS_SV_SERVERLIST));
	Out.Replace(_T("[Shared]"), _GetPlainResString(IDS_SHAREDFILES));
	Out.Replace(_T("[Download]"), _GetPlainResString(IDS_SW_LINK));
	Out.Replace(_T("[Graphs]"), _GetPlainResString(IDS_GRAPHS));
	Out.Replace(_T("[Log]"), _GetPlainResString(IDS_SV_LOG));
	Out.Replace(_T("[ServerInfo]"), _GetPlainResString(IDS_SV_SERVERINFO));
	Out.Replace(_T("[DebugLog]"), _GetPlainResString(IDS_SV_DEBUGLOG));
	Out.Replace(_T("[Stats]"), _GetPlainResString(IDS_SF_STATISTICS));
	Out.Replace(_T("[Options]"), _GetPlainResString(IDS_EM_PREFS));
	Out.Replace(_T("[Logout]"), _GetPlainResString(IDS_WEB_LOGOUT));
	Out.Replace(_T("[Search]"), _GetPlainResString(IDS_SW_SEARCHBOX));
	Out.Replace(_T("[Kad]"), _GetPlainResString(IDS_KADEMLIA));

	TCHAR HTTPTempC[100] = _T("");
	CString sConnected = _T("");
	if (theApp.IsConnected()) 
	{
		if(!theApp.IsFirewalled())
			sConnected = _GetPlainResString(IDS_CONNECTED);
		else
			sConnected = _GetPlainResString(IDS_CONNECTED) + _T(" (") + _GetPlainResString(IDS_IDLOW) + _T(")");

		if( theApp.serverconnect->IsConnected()){
			CServer* cur_server = theApp.serverconnect->GetCurrentServer();
			sConnected += _T(": ") + cur_server->GetListName();
	
			_stprintf(HTTPTempC, _T("%i "), cur_server->GetUsers());
			sConnected += _T(" [") + CString(HTTPTempC) + _GetPlainResString(IDS_LUSERS) + _T("]");
		}
	} 
	else if (theApp.serverconnect->IsConnecting())
	{
		sConnected = _GetPlainResString(IDS_CONNECTING);
	}
	else
	{
		sConnected = _GetPlainResString(IDS_DISCONNECTED);
		if (IsSessionAdmin(Data,sSession)) sConnected+=_T(" (<small><a href=\"?ses=") + sSession + _T("&w=server&c=connect\">")+_GetPlainResString(IDS_CONNECTTOANYSERVER)+_T("</a></small>)");
	}
	Out.Replace(_T("[Connected]"), _T("<b>")+_GetPlainResString(IDS_PW_CONNECTION)+_T(":</b> ")+sConnected);
    _stprintf(HTTPTempC, _GetPlainResString(IDS_UPDOWNSMALL),(float)theApp.uploadqueue->GetDatarate()/1024,(float)theApp.downloadqueue->GetDatarate()/1024);

	// EC 25-12-2003
	CString MaxUpload;
	CString MaxDownload;
	MaxUpload.Format(_T("%i"),thePrefs.GetMaxUpload());
	MaxDownload.Format(_T("%i"),thePrefs.GetMaxDownload());
	if (MaxUpload == _T("65535"))  MaxUpload = GetResString(IDS_PW_UNLIMITED);
	if (MaxDownload == _T("65535")) MaxDownload = GetResString(IDS_PW_UNLIMITED);
	buffer.Format(_T("%s/%s"), MaxUpload, MaxDownload);
	// EC Ends
	Out.Replace(_T("[Speed]"), _T("<b>")+_GetPlainResString(IDS_DL_SPEED)+_T(":</b> ")+CString(HTTPTempC) + _T("<small> (") + _GetPlainResString(IDS_PW_CON_LIMITFRM) + _T(": ") + buffer + _T(")</small>"));

	buffer=GetResString(IDS_KADEMLIA)+_T(": ");

	if (!thePrefs.GetNetworkKademlia()) {
		buffer.Append(GetResString(IDS_DISABLED));
	}
	else if ( !Kademlia::CKademlia::isRunning() ) {

		buffer.Append(GetResString(IDS_DISCONNECTED));

		if (IsSessionAdmin(Data,sSession)) 
			buffer+=_T(" (<small><a href=\"?ses=") + sSession + _T("&w=kad&c=connect\">")+_GetPlainResString(IDS_MAIN_BTN_CONNECT)+_T("</a></small>)");
	}
	else if ( Kademlia::CKademlia::isRunning() && !Kademlia::CKademlia::isConnected() ) {

		buffer.Append(GetResString(IDS_CONNECTING));

		if (IsSessionAdmin(Data,sSession)) 
			buffer+=_T(" (<small><a href=\"?ses=") + sSession + _T("&w=kad&c=disconnect\">")+_GetPlainResString(IDS_MAIN_BTN_DISCONNECT)+_T("</a></small>)");
	}
	else if (Kademlia::CKademlia::isFirewalled()) {
		buffer.Append(GetResString(IDS_FIREWALLED));
		if (IsSessionAdmin(Data,sSession)) 
			buffer+=_T(" (<small><a href=\"?ses=") + sSession + _T("&w=kad&c=disconnect\">")+_GetPlainResString(IDS_IRC_DISCONNECT)+_T("</a></small>)");
	}
	else if (Kademlia::CKademlia::isConnected()) {
		buffer.Append(GetResString(IDS_CONNECTED));
		if (IsSessionAdmin(Data,sSession)) 
			buffer+=_T(" (<small><a href=\"?ses=") + sSession + _T("&w=kad&c=disconnect\">")+_GetPlainResString(IDS_IRC_DISCONNECT)+_T("</a></small>)");
	}

	Out.Replace(_T("[KademliaInfo]"),buffer);

	return Out;
}

CString CWebServer::_GetFooter(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	return pThis->m_Templates.sFooter;
}

CString CWebServer::_GetServerList(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));

	CString sAddServerBox = _T("");

	CString sCmd = _ParseURL(Data.sURL, _T("c"));
	if (sCmd == _T("connect") && IsSessionAdmin(Data,sSession) )
	{
		_ConnectToServer(_ParseURL(Data.sURL, _T("ip")),_tstoi(_ParseURL(Data.sURL, _T("port"))));
	}	
	else if (sCmd == _T("disconnect") && IsSessionAdmin(Data,sSession)) 
	{
		theApp.emuledlg->SendMessage(WEB_DISCONNECT_SERVER, NULL);
	}	
	else if (sCmd == _T("remove") && IsSessionAdmin(Data,sSession)) 
	{
		CString sIP = _ParseURL(Data.sURL, _T("ip"));
		int nPort = _tstoi(_ParseURL(Data.sURL, _T("port")));
		if(!sIP.IsEmpty())
			_RemoveServer(sIP,nPort);
	}	
	else if (sCmd == _T("options")) 
	{
		sAddServerBox = _GetAddServerBox(Data);
	}
	CString sSort = _ParseURL(Data.sURL, _T("sort"));
	if (sSort != _T("")) 
	{
		if(sSort == _T("name"))
			pThis->m_Params.ServerSort = SERVER_SORT_NAME;
		else
		if(sSort == _T("description"))
			pThis->m_Params.ServerSort = SERVER_SORT_DESCRIPTION;
		else
		if(sSort == _T("ip"))
			pThis->m_Params.ServerSort = SERVER_SORT_IP;
		else
		if(sSort == _T("users"))
			pThis->m_Params.ServerSort = SERVER_SORT_USERS;
		else
		if(sSort == _T("files"))
			pThis->m_Params.ServerSort = SERVER_SORT_FILES;

		if(_ParseURL(Data.sURL, _T("sortreverse")) == _T(""))
			pThis->m_Params.bServerSortReverse = false;
	}
	if (_ParseURL(Data.sURL, _T("sortreverse")) != _T("")) 
	{
		pThis->m_Params.bServerSortReverse = (_ParseURL(Data.sURL, _T("sortreverse")) == _T("true"));
	}
	CString sServerSortRev;
	if(pThis->m_Params.bServerSortReverse)
		sServerSortRev = _T("false");
	else
		sServerSortRev = _T("true");

	CString Out = pThis->m_Templates.sServerList;
	Out.Replace(_T("[ConnectedServerData]"), _GetConnectedServer(Data));
    Out.Replace(_T("[AddServerBox]"), sAddServerBox);
	Out.Replace(_T("[Session]"), sSession);
	if(pThis->m_Params.ServerSort == SERVER_SORT_NAME)
		Out.Replace(_T("[SortName]"), _T("&sortreverse=") + sServerSortRev);
	else
		Out.Replace(_T("[SortName]"), _T(""));
	if(pThis->m_Params.ServerSort == SERVER_SORT_DESCRIPTION)
		Out.Replace(_T("[SortDescription]"), _T("&sortreverse=") + sServerSortRev);
	else
		Out.Replace(_T("[SortDescription]"), _T(""));
	if(pThis->m_Params.ServerSort == SERVER_SORT_IP)
		Out.Replace(_T("[SortIP]"), _T("&sortreverse=") + sServerSortRev);
	else
		Out.Replace(_T("[SortIP]"), _T(""));
	if(pThis->m_Params.ServerSort == SERVER_SORT_USERS)
		Out.Replace(_T("[SortUsers]"), _T("&sortreverse=") + sServerSortRev);
	else
		Out.Replace(_T("[SortUsers]"), _T(""));
	if(pThis->m_Params.ServerSort == SERVER_SORT_FILES)
		Out.Replace(_T("[SortFiles]"), _T("&sortreverse=") + sServerSortRev);
	else
		Out.Replace(_T("[SortFiles]"), _T(""));

	CString sortimg;
	if (pThis->m_Params.bServerSortReverse) sortimg=_T("<img src=\"l_up.gif\">");
		else sortimg=_T("<img src=\"l_down.gif\">");

	Out.Replace(_T("[ServerList]"), _GetPlainResString(IDS_SV_SERVERLIST));
	Out.Replace(_T("[Servername]"), _GetPlainResString(IDS_SL_SERVERNAME)+CString((pThis->m_Params.ServerSort==SERVER_SORT_NAME)?sortimg:_T("")));
	Out.Replace(_T("[Description]"), _GetPlainResString(IDS_DESCRIPTION)+CString((pThis->m_Params.ServerSort==SERVER_SORT_DESCRIPTION)?sortimg:_T("")));
	Out.Replace(_T("[Address]"), _GetPlainResString(IDS_IP)+CString((pThis->m_Params.ServerSort==SERVER_SORT_IP)?sortimg:_T("")));
	Out.Replace(_T("[Connect]"), _GetPlainResString(IDS_IRC_CONNECT));
	Out.Replace(_T("[Users]"), _GetPlainResString(IDS_LUSERS)+CString((pThis->m_Params.ServerSort==SERVER_SORT_USERS)?sortimg:_T("")));
	Out.Replace(_T("[Files]"), _GetPlainResString(IDS_LFILES)+CString((pThis->m_Params.ServerSort==SERVER_SORT_FILES)?sortimg:_T("")));
	Out.Replace(_T("[Actions]"), _GetPlainResString(IDS_WEB_ACTIONS));

	CString OutE = pThis->m_Templates.sServerLine;
	OutE.Replace(_T("[Connect]"), _GetPlainResString(IDS_IRC_CONNECT));
	OutE.Replace(_T("[RemoveServer]"), _GetPlainResString(IDS_REMOVETHIS));
	OutE.Replace(_T("[ConfirmRemove]"), _GetPlainResString(IDS_WEB_CONFIRM_REMOVE_SERVER));

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
	CString sList = _T("");
	for(int i = 0; i < ServerArray.GetCount(); i++)
	{
		CString HTTPProcessData = OutE;	// Copy Entry Line to Temp
		HTTPProcessData.Replace(_T("[1]"), ServerArray[i].sServerName);
		HTTPProcessData.Replace(_T("[2]"), ServerArray[i].sServerDescription);
		CString sPort; sPort.Format(_T(":%d"), ServerArray[i].nServerPort);
		HTTPProcessData.Replace(_T("[3]"), ServerArray[i].sServerIP + sPort);

		CString sT;
		if(ServerArray[i].nServerUsers > 0)
		{
			if(ServerArray[i].nServerMaxUsers > 0)
				sT.Format(_T("%d (%d)"), ServerArray[i].nServerUsers, ServerArray[i].nServerMaxUsers);
			else
				sT.Format(_T("%d"), ServerArray[i].nServerUsers);
		}
		HTTPProcessData.Replace(_T("[4]"), sT);
		sT = _T("");
		if(ServerArray[i].nServerFiles > 0)
			sT.Format(_T("%d"), ServerArray[i].nServerFiles);
		HTTPProcessData.Replace(_T("[5]"), sT);

		CString sServerPort; sServerPort.Format(_T("%d"), ServerArray[i].nServerPort);

		HTTPProcessData.Replace(_T("[6]"), IsSessionAdmin(Data,sSession)? CString(_T("?ses=") + sSession + _T("&w=server&c=connect&ip=") + ServerArray[i].sServerIP+_T("&port=")+sServerPort):GetPermissionDenied());
		HTTPProcessData.Replace(_T("[LinkRemove]"), IsSessionAdmin(Data,sSession)?CString(_T("?ses=") + sSession + _T("&w=server&c=remove&ip=") + ServerArray[i].sServerIP+_T("&port=")+sServerPort):GetPermissionDenied());

		sList += HTTPProcessData;
	}
	Out.Replace(_T("[ServersList]"), sList);

	return Out;
}

CString CWebServer::_GetTransferList(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));
	int cat=_tstoi(_ParseURL(Data.sURL,_T("cat")));

	bool clcompl=(_ParseURL(Data.sURL,_T("ClCompl"))==_T("yes") );
	CString sCat; (cat!=0)?sCat.Format(_T("&cat=%i"),cat):sCat=_T("");

	CString Out = _T("");

	if (clcompl && IsSessionAdmin(Data,sSession)) 
		theApp.emuledlg->transferwnd->downloadlistctrl.ClearCompleted(true);

	if (_ParseURL(Data.sURL, _T("c")) != _T("") && IsSessionAdmin(Data,sSession)) 
	{
		CString HTTPTemp = _ParseURL(Data.sURL, _T("c"));
		theApp.AddEd2kLinksToDownload(HTTPTemp,cat);
	}

	if (_ParseURL(Data.sURL, _T("op")) != _T("") &&
		_ParseURL(Data.sURL, _T("file")) != _T(""))
	{
		uchar FileHash[16];
		_GetFileHash(_ParseURL(Data.sURL, _T("file")), FileHash);

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

		if(_ParseURL(Data.sURL, _T("op")) == _T("pause") && IsSessionAdmin(Data,sSession))
		{
			if(found_file)
				found_file->PauseFile();
		}
		else if(_ParseURL(Data.sURL, _T("op")) == _T("resume") && IsSessionAdmin(Data,sSession))
		{
			if(found_file)
				found_file->ResumeFile();
		}
		else if(_ParseURL(Data.sURL, _T("op")) == _T("cancel") && IsSessionAdmin(Data,sSession))
		{
			if(found_file)
				found_file->DeleteFile();
		}
		else if(_ParseURL(Data.sURL, _T("op")) == _T("prioup") && IsSessionAdmin(Data,sSession))
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
		else if(_ParseURL(Data.sURL, _T("op")) == _T("priodown") && IsSessionAdmin(Data,sSession))
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
	if (_ParseURL(Data.sURL, _T("sort")) != _T("")) 
	{
		if(_ParseURL(Data.sURL, _T("sort")) == _T("name"))
			pThis->m_Params.DownloadSort = DOWN_SORT_NAME;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("size"))
			pThis->m_Params.DownloadSort = DOWN_SORT_SIZE;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("transferred"))
			pThis->m_Params.DownloadSort = DOWN_SORT_TRANSFERRED;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("speed"))
			pThis->m_Params.DownloadSort = DOWN_SORT_SPEED;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("progress"))
			pThis->m_Params.DownloadSort = DOWN_SORT_PROGRESS;

		if(_ParseURL(Data.sURL, _T("sortreverse")) == _T(""))
			pThis->m_Params.bDownloadSortReverse = false;
	}
	if (_ParseURL(Data.sURL, _T("sortreverse")) != _T("")) 
	{
		pThis->m_Params.bDownloadSortReverse = (_ParseURL(Data.sURL, _T("sortreverse")) == _T("true"));
	}
	if(_ParseURL(Data.sURL, _T("showuploadqueue")) == _T("true"))
	{
		pThis->m_Params.bShowUploadQueue = true;
	}
	if(_ParseURL(Data.sURL, _T("showuploadqueue")) == _T("false"))
	{
		pThis->m_Params.bShowUploadQueue = false;
	}
	CString sDownloadSortRev;
	if(pThis->m_Params.bDownloadSortReverse)
		sDownloadSortRev = _T("false");
	else
		sDownloadSortRev = _T("true");

	Out += pThis->m_Templates.sTransferImages;
	Out += pThis->m_Templates.sTransferList;
	Out.Replace(_T("[DownloadHeader]"), pThis->m_Templates.sTransferDownHeader);
	Out.Replace(_T("[DownloadFooter]"), pThis->m_Templates.sTransferDownFooter);
	Out.Replace(_T("[UploadHeader]"), pThis->m_Templates.sTransferUpHeader);
	Out.Replace(_T("[UploadFooter]"), pThis->m_Templates.sTransferUpFooter);
	Out.Replace(_T("[Session]"), sSession);

	InsertCatBox(Out,cat,_T(""),true,true);

	if(pThis->m_Params.DownloadSort == DOWN_SORT_NAME)
		Out.Replace(_T("[SortName]"), _T("&sortreverse=") + sDownloadSortRev);
	else
		Out.Replace(_T("[SortName]"), _T(""));
	if(pThis->m_Params.DownloadSort == DOWN_SORT_SIZE)
		Out.Replace(_T("[SortSize]"), _T("&sortreverse=") + sDownloadSortRev);
	else
		Out.Replace(_T("[SortSize]"), _T(""));
	if(pThis->m_Params.DownloadSort == DOWN_SORT_TRANSFERRED)
		Out.Replace(_T("[SortTransferred]"), _T("&sortreverse=") + sDownloadSortRev);
	else
		Out.Replace(_T("[SortTransferred]"), _T(""));
	if(pThis->m_Params.DownloadSort == DOWN_SORT_SPEED)
		Out.Replace(_T("[SortSpeed]"), _T("&sortreverse=") + sDownloadSortRev);
	else
		Out.Replace(_T("[SortSpeed]"), _T(""));
	if(pThis->m_Params.DownloadSort == DOWN_SORT_PROGRESS)
		Out.Replace(_T("[SortProgress]"), _T("&sortreverse=") + sDownloadSortRev);
	else
		Out.Replace(_T("[SortProgress]"), _T(""));

	CString sortimg;
	if (pThis->m_Params.bDownloadSortReverse) sortimg=_T("<img src=\"l_up.gif\">");
		else sortimg=_T("<img src=\"l_down.gif\">");

	Out.Replace(_T("[Filename]"), _GetPlainResString(IDS_DL_FILENAME)+CString((pThis->m_Params.DownloadSort == DOWN_SORT_NAME)?sortimg:_T("")));
	Out.Replace(_T("[Size]"), _GetPlainResString(IDS_DL_SIZE)+CString((pThis->m_Params.DownloadSort == DOWN_SORT_SIZE)?sortimg:_T("")));
	Out.Replace(_T("[Transferred]"), _GetPlainResString(IDS_COMPLETE)+CString((pThis->m_Params.DownloadSort == DOWN_SORT_TRANSFERRED)?sortimg:_T("")));
	Out.Replace(_T("[Progress]"), _GetPlainResString(IDS_DL_PROGRESS)+CString((pThis->m_Params.DownloadSort == DOWN_SORT_PROGRESS)?sortimg:_T("")));
	Out.Replace(_T("[Speed]"), _GetPlainResString(IDS_DL_SPEED)+CString((pThis->m_Params.DownloadSort == DOWN_SORT_SPEED)?sortimg:_T("")));
	Out.Replace(_T("[Sources]"), _GetPlainResString(IDS_DL_SOURCES));
	Out.Replace(_T("[Actions]"), _GetPlainResString(IDS_WEB_ACTIONS));
	Out.Replace(_T("[User]"), _GetPlainResString(IDS_QL_USERNAME));
	Out.Replace(_T("[TotalDown]"), _GetPlainResString(IDS_INFLST_USER_TOTALDOWNLOAD));
	Out.Replace(_T("[TotalUp]"), _GetPlainResString(IDS_INFLST_USER_TOTALUPLOAD));
	Out.Replace(_T("[Prio]"), _GetPlainResString(IDS_PRIORITY));
	Out.Replace(_T("[CatSel]"),sCat);
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
	CString sDownList = _T("");
	CString HTTPTemp;
	CString label=_GetPlainResString(IDS_SW_LINK);

	for(int i = 0; i < FilesArray.GetCount(); i++)
	{
		CString JSfileinfo=FilesArray[i].sFileInfo;
		JSfileinfo.Replace(_T("\n"),_T("\\n"));
		JSfileinfo.Replace(_T("\'"),_T("\\'"));			// [Thx2 Zune]
		
		CString sActions = _T("<acronym title=\"") + FilesArray[i].sFileStatus + _T("\"><a href=\"javascript:alert(\'")+ JSfileinfo + _T("')\"><img src=\"l_info.gif\" alt=\"") + FilesArray[i].sFileStatus + _T("\"></a></acronym> ");

		sActions += _T("<acronym title=\"")+label+_T("\"><a href=\"")+ FilesArray[i].sED2kLink +_T("\"><img src=\"l_ed2klink.gif\" alt=\"")+label+_T("\"></a></acronym>");

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
					CString sResume=_T("");
					sResume.Format(_T("<acronym title=\"[Resume]\"><a href=\"[Link]\"><img src=\"l_resume.gif\" alt=\"[Resume]\"></a></acronym> "));
					sResume.Replace(_T("[Link]"), CString(_T("?ses=") + sSession + _T("&w=transfer&op=resume&file=") + FilesArray[i].sFileHash +sCat )) ;
					sActions += sResume;
				}
			}
				break; 
			default: // waiting or downloading
			{
				if (IsSessionAdmin(Data,sSession)) {
					CString sPause;
					sPause.Format(_T("<acronym title=\"[Pause]\"><a href=\"[Link]\"><img src=\"l_pause.gif\" alt=\"[Pause]\"></a></acronym> "));
					sPause.Replace(_T("[Link]"), CString(_T("?ses=") + sSession + _T("&w=transfer&op=pause&file=") + FilesArray[i].sFileHash+sCat ));
					sActions += sPause;
				}
			}
			break;
		}
		if(bCanBeDeleted)
		{
			if (IsSessionAdmin(Data,sSession)) {
				CString sCancel;
				sCancel.Format(_T("<acronym title=\"[Cancel]\"><a href=\"[Link]\" onclick=\"return confirm(\'[ConfirmCancel]\')\"><img src=\"l_cancel.gif\" alt=\"[Cancel]\"></a></acronym> "));
				sCancel.Replace(_T("[Link]"), CString(_T("?ses=") + sSession + _T("&w=transfer&op=cancel&file=") + FilesArray[i].sFileHash+sCat ) );
				sActions += sCancel;
			}
		}

		if (IsSessionAdmin(Data,sSession)) {
			sActions.Replace(_T("[Resume]"), _GetPlainResString(IDS_DL_RESUME));
			sActions.Replace(_T("[Pause]"), _GetPlainResString(IDS_DL_PAUSE));
			sActions.Replace(_T("[Cancel]"), _GetPlainResString(IDS_MAIN_BTN_CANCEL));
			sActions.Replace(_T("[ConfirmCancel]"), _GetPlainResStringNoQuote(IDS_Q_CANCELDL2));
			
			if (FilesArray[i].nFileStatus!=PS_COMPLETE && FilesArray[i].nFileStatus!=PS_COMPLETING) {
				sActions.Append(_T("<acronym title=\"[PriorityUp]\"><a href=\"?ses=[Session]&amp;w=transfer&op=prioup&file=") + FilesArray[i].sFileHash+sCat+_T("\"><img src=\"l_up.gif\" alt=\"[PriorityUp]\"></a></acronym>"));
				sActions.Append(_T("&nbsp;<acronym title=\"[PriorityDown]\"><a href=\"?ses=[Session]&amp;w=transfer&op=priodown&file=") + FilesArray[i].sFileHash+sCat +_T("\"><img src=\"l_down.gif\" alt=\"[PriorityDown]\"></a></acronym>"));
			}
		}

		CString HTTPProcessData;
		// if downloading, draw in other color
		if(FilesArray[i].lFileSpeed > 0)
			HTTPProcessData = OutE2;
		else
			HTTPProcessData = OutE;

		if(FilesArray[i].sFileName.GetLength() > SHORT_FILENAME_LENGTH)
			HTTPProcessData.Replace(_T("[ShortFileName]"), FilesArray[i].sFileName.Left(SHORT_FILENAME_LENGTH) + _T("..."));
		else
			HTTPProcessData.Replace(_T("[ShortFileName]"), FilesArray[i].sFileName);

		HTTPProcessData.Replace(_T("[FileInfo]"), FilesArray[i].sFileInfo);

		fTotalSize += FilesArray[i].lFileSize;

		HTTPProcessData.Replace(_T("[2]"), CastItoXBytes(FilesArray[i].lFileSize, false, false));

		if(FilesArray[i].lFileTransferred > 0)
		{
			fTotalTransferred += FilesArray[i].lFileTransferred;

			HTTPProcessData.Replace(_T("[3]"), CastItoXBytes(FilesArray[i].lFileTransferred, false, false));
		}
		else
			HTTPProcessData.Replace(_T("[3]"), _T("-"));

		HTTPProcessData.Replace(_T("[DownloadBar]"), _GetDownloadGraph(Data,FilesArray[i].sFileHash));

		if(FilesArray[i].lFileSpeed > 0.0f)
		{
			fTotalSpeed += FilesArray[i].lFileSpeed;

			HTTPTemp.Format(_T("%s"), CastItoXBytes(FilesArray[i].lFileSpeed, false, true));
			HTTPProcessData.Replace(_T("[4]"), HTTPTemp);
		}
		else
			HTTPProcessData.Replace(_T("[4]"), _T("-"));
		if(FilesArray[i].lSourceCount > 0)
		{
			HTTPTemp.Format(_T("%i&nbsp;/&nbsp;%8i&nbsp;(%i)"),
				FilesArray[i].lSourceCount-FilesArray[i].lNotCurrentSourceCount,
				FilesArray[i].lSourceCount,
				FilesArray[i].lTransferringSourceCount);

		HTTPProcessData.Replace(_T("[5]"), HTTPTemp);
		}
		else
			HTTPProcessData.Replace(_T("[5]"), _T("-"));

		switch(FilesArray[i].nFilePrio) {
			case 0: HTTPTemp=GetResString(IDS_PRIOLOW);break;
			case 10: HTTPTemp=GetResString(IDS_PRIOAUTOLOW);break;

			case 1: HTTPTemp=GetResString(IDS_PRIONORMAL);break;
			case 11: HTTPTemp=GetResString(IDS_PRIOAUTONORMAL);break;

			case 2: HTTPTemp=GetResString(IDS_PRIOHIGH);break;
			case 12: HTTPTemp=GetResString(IDS_PRIOAUTOHIGH);break;
		}

		HTTPProcessData.Replace(_T("[PrioVal]"), HTTPTemp);
		HTTPProcessData.Replace(_T("[6]"), sActions);

		sDownList += HTTPProcessData;
	}

	Out.Replace(_T("[DownloadFilesList]"), sDownList);
    Out.Replace(_T("[PriorityUp]"), _GetPlainResString(IDS_PRIORITY_UP));
    Out.Replace(_T("[PriorityDown]"), _GetPlainResString(IDS_PRIORITY_DOWN));
	// Elandal: cast from float to integral type always drops fractions.
	// avoids implicit cast warning
	Out.Replace(_T("[TotalDownSize]"), CastItoXBytes(fTotalSize, false, false));
	Out.Replace(_T("[TotalDownTransferred]"), CastItoXBytes(fTotalTransferred, false, false));

	Out.Replace(_T("[ClearCompletedButton]"),(completedAv && IsSessionAdmin(Data,sSession))?pThis->m_Templates.sClearCompleted :_T(""));

	HTTPTemp.Format(_T("%s"), CastItoXBytes(fTotalSpeed, false, true));
	Out.Replace(_T("[TotalDownSpeed]"), HTTPTemp);
	OutE = pThis->m_Templates.sTransferUpLine;
	
	HTTPTemp.Format(_T("%i"),pThis->m_Templates.iProgressbarWidth);
	Out.Replace(_T("[PROGRESSBARWIDTHVAL]"),HTTPTemp);

	fTotalSize = 0;
	fTotalTransferred = 0;
	fTotalSpeed = 0;

	CString sUpList = _T("");
	
	for (POSITION pos = theApp.uploadqueue->GetFirstFromUploadList();
		pos != 0;theApp.uploadqueue->GetNextFromUploadList(pos))
	{
		CUpDownClient* cur_client = theApp.uploadqueue->GetQueueClientAt(pos);
		CString HTTPProcessData = OutE;
		HTTPProcessData.Replace(_T("[1]"), _SpecialChars(cur_client->GetUserName()));
		HTTPProcessData.Replace(_T("[FileInfo]"), _SpecialChars(GetUploadFileInfo(cur_client)));

		CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetUploadFileID());
		if (file)
			HTTPProcessData.Replace(_T("[2]"), _SpecialChars(CString(file->GetFileName())));
		else
			HTTPProcessData.Replace(_T("[2]"), _GetPlainResString(IDS_REQ_UNKNOWNFILE));

		fTotalSize += cur_client->GetTransferedDown();
		fTotalTransferred += cur_client->GetTransferedUp();
		CString HTTPTemp;
		HTTPTemp.Format(_T("%s / %s"), CastItoXBytes(cur_client->GetTransferedDown(), false, false),CastItoXBytes(cur_client->GetTransferedUp(), false, false));
		HTTPProcessData.Replace(_T("[3]"), HTTPTemp);

		fTotalSpeed += cur_client->GetDatarate();
		HTTPTemp.Format(_T("%s"), CastItoXBytes(cur_client->GetDatarate(), false, true));
		HTTPProcessData.Replace(_T("[4]"), HTTPTemp);

		sUpList += HTTPProcessData;
	}
	Out.Replace(_T("[UploadFilesList]"), sUpList);
	// Elandal: cast from float to integral type always drops fractions.
	// avoids implicit cast warning
	HTTPTemp.Format(_T("%s / %s"), CastItoXBytes(fTotalSize, false, false), CastItoXBytes(fTotalTransferred, false, false));
	Out.Replace(_T("[TotalUpTransferred]"), HTTPTemp);
	HTTPTemp.Format(_T("%s"), CastItoXBytes(fTotalSpeed, false, true));
	Out.Replace(_T("[TotalUpSpeed]"), HTTPTemp);

	if(pThis->m_Params.bShowUploadQueue) 
	{
		Out.Replace(_T("[UploadQueue]"), pThis->m_Templates.sTransferUpQueueShow);
		Out.Replace(_T("[UploadQueueList]"), _GetPlainResString(IDS_ONQUEUE));
		Out.Replace(_T("[UserNameTitle]"), _GetPlainResString(IDS_QL_USERNAME));
		Out.Replace(_T("[FileNameTitle]"), _GetPlainResString(IDS_DL_FILENAME));
		Out.Replace(_T("[ScoreTitle]"), _GetPlainResString(IDS_SCORE));
		Out.Replace(_T("[BannedTitle]"), _GetPlainResString(IDS_BANNED));

		OutE = pThis->m_Templates.sTransferUpQueueLine;
		// Replace [xx]
		CString sQueue = _T("");

		for (POSITION pos = theApp.uploadqueue->GetFirstFromWaitingList(); pos != 0;theApp.uploadqueue->GetNextFromWaitingList(pos)){
			CUpDownClient* cur_client = theApp.uploadqueue->GetWaitClientAt(pos);
			CString HTTPProcessData;
            TCHAR HTTPTempC[100] = _T("");
			HTTPProcessData = OutE;
			HTTPProcessData.Replace(_T("[UserName]"), _SpecialChars(cur_client->GetUserName()));
			if (!cur_client->GetRequestFile()) continue;
			CKnownFile* file = theApp.sharedfiles->GetFileByID(cur_client->GetRequestFile()->GetFileHash());
			if (file)
				HTTPProcessData.Replace(_T("[FileName]"), _SpecialChars(cur_client->GetRequestFile()->GetFileName()));
			else
				HTTPProcessData.Replace(_T("[FileName]"), _T("?"));
			_stprintf(HTTPTempC, _T("%i") , cur_client->GetScore(false));
			CString HTTPTemp = HTTPTempC;
			HTTPProcessData.Replace(_T("[Score]"), HTTPTemp);
			if (cur_client->IsBanned())
				HTTPProcessData.Replace(_T("[Banned]"), _GetPlainResString(IDS_YES));
			else
				HTTPProcessData.Replace(_T("[Banned]"), _GetPlainResString(IDS_NO));
			sQueue += HTTPProcessData;
		}
		Out.Replace(_T("[QueueList]"), sQueue);
	}
	else {
		Out.Replace(_T("[UploadQueue]"), pThis->m_Templates.sTransferUpQueueHide);
	}
	Out.Replace(_T("[ShowQueue]"), _GetPlainResString(IDS_WEB_SHOW_UPLOAD_QUEUE));
	Out.Replace(_T("[HideQueue]"), _GetPlainResString(IDS_WEB_HIDE_UPLOAD_QUEUE));
	Out.Replace(_T("[Session]"), sSession);
	Out.Replace(_T("[CLEARCOMPLETED]"),_GetPlainResString(IDS_DL_CLEAR));

	CString buffer;
	buffer.Format(_T("%s (%i)"), _GetPlainResString(IDS_TW_DOWNLOADS),FilesArray.GetCount());
	Out.Replace(_T("[DownloadList]"),buffer);
	buffer.Format(_T("%s (%i)"),_GetPlainResString(IDS_PW_CON_UPLBL),theApp.uploadqueue->GetUploadQueueLength());
	Out.Replace(_T("[UploadList]"), buffer );
	Out.Replace(_T("[CatSel]"),sCat);

	return Out;
}

CString CWebServer::_GetDownloadLink(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));

	if (!IsSessionAdmin(Data,sSession)) {
		CString ad=_T("<br><br><div align=\"center\" class=\"message\">[Message]</div>");
		ad.Replace(_T("[Message]"),_GetPlainResString(IDS_ACCESSDENIED));
		return ad;
	}

	CString Out = pThis->m_Templates.sDownloadLink;

	Out.Replace(_T("[Download]"), _GetPlainResString(IDS_SW_DOWNLOAD));
	Out.Replace(_T("[Ed2klink]"), _GetPlainResString(IDS_SW_LINK));
	Out.Replace(_T("[Start]"), _GetPlainResString(IDS_SW_START));
	Out.Replace(_T("[Session]"), sSession);

	if (thePrefs.GetCatCount()>1)
		InsertCatBox(Out,0, pThis->m_Templates.sCatArrow );
	else Out.Replace(_T("[CATBOX]"),_T(""));

	return Out;
}

CString CWebServer::_GetSharedFilesList(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
        CString sSession = _ParseURL(Data.sURL, _T("ses"));
	if(pThis == NULL)
		return _T("");
	if (_ParseURL(Data.sURL, _T("sort")) != _T("")) 
	{
		if(_ParseURL(Data.sURL, _T("sort")) == _T("name"))
			pThis->m_Params.SharedSort = SHARED_SORT_NAME;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("size"))
			pThis->m_Params.SharedSort = SHARED_SORT_SIZE;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("transferred"))
			pThis->m_Params.SharedSort = SHARED_SORT_TRANSFERRED;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("alltimetransferred"))
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_TRANSFERRED;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("requests"))
			pThis->m_Params.SharedSort = SHARED_SORT_REQUESTS;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("alltimerequests"))
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_REQUESTS;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("accepts"))
			pThis->m_Params.SharedSort = SHARED_SORT_ACCEPTS;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("alltimeaccepts"))
			pThis->m_Params.SharedSort = SHARED_SORT_ALL_TIME_ACCEPTS;
		else
		if(_ParseURL(Data.sURL, _T("sort")) == _T("priority"))
			pThis->m_Params.SharedSort = SHARED_SORT_PRIORITY;

		if(_ParseURL(Data.sURL, _T("sortreverse")) == _T(""))
			pThis->m_Params.bSharedSortReverse = false;
	}
	if (_ParseURL(Data.sURL, _T("sortreverse")) != _T("")) 
		pThis->m_Params.bSharedSortReverse = (_ParseURL(Data.sURL, _T("sortreverse")) == _T("true"));

	if(_ParseURL(Data.sURL, _T("hash")) != _T("") && _ParseURL(Data.sURL, _T("setpriority")) != _T("") && IsSessionAdmin(Data,sSession)) 
		_SetSharedFilePriority(_ParseURL(Data.sURL, _T("hash")),_tstoi(_ParseURL(Data.sURL, _T("setpriority"))));

	if(_ParseURL(Data.sURL, _T("reload")) == _T("true"))
	{
		theApp.emuledlg->SendMessage(WEB_SHARED_FILES_RELOAD);
	}

	CString sSharedSortRev;
	if(pThis->m_Params.bSharedSortReverse)
		sSharedSortRev = _T("false");
	else
		sSharedSortRev = _T("true");

	CString sortimg;
	if (pThis->m_Params.bSharedSortReverse) sortimg=_T("<img src=\"l_up.gif\">");
		else sortimg=_T("<img src=\"l_down.gif\">");


    //Name sorting link
	CString Out = pThis->m_Templates.sSharedList;
	if(pThis->m_Params.SharedSort == SHARED_SORT_NAME)
		Out.Replace(_T("[SortName]"), _T("sort=name&sortreverse=") + sSharedSortRev);
	else
		Out.Replace(_T("[SortName]"), _T("sort=name"));
	//Size sorting Link
    if(pThis->m_Params.SharedSort == SHARED_SORT_SIZE)
		Out.Replace(_T("[SortSize]"), _T("sort=size&sortreverse=") + sSharedSortRev);
	else
		Out.Replace(_T("[SortSize]"), _T("sort=size"));
	//Priority sorting Link
    if(pThis->m_Params.SharedSort == SHARED_SORT_PRIORITY)
		Out.Replace(_T("[SortPriority]"), _T("sort=priority&sortreverse=") + sSharedSortRev);
	else
		Out.Replace(_T("[SortPriority]"), _T("sort=priority"));
    //Transferred sorting link
	if(pThis->m_Params.SharedSort == SHARED_SORT_TRANSFERRED)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace(_T("[SortTransferred]"), _T("sort=alltimetransferred&sortreverse=") + sSharedSortRev);
		else
			Out.Replace(_T("[SortTransferred]"), _T("sort=transferred&sortreverse=") + sSharedSortRev);
	}
	else
	if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_TRANSFERRED)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace(_T("[SortTransferred]"), _T("sort=transferred&sortreverse=") + sSharedSortRev);
		else
			Out.Replace(_T("[SortTransferred]"), _T("sort=alltimetransferred&sortreverse=") + sSharedSortRev);
	}
	else
        Out.Replace(_T("[SortTransferred]"), _T("&sort=transferred&sortreverse=false"));
    //Request sorting link
	if(pThis->m_Params.SharedSort == SHARED_SORT_REQUESTS)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace(_T("[SortRequests]"), _T("sort=alltimerequests&sortreverse=") + sSharedSortRev);
		else
			Out.Replace(_T("[SortRequests]"), _T("sort=requests&sortreverse=") + sSharedSortRev);
	}
	else
	if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_REQUESTS)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace(_T("[SortRequests]"), _T("sort=requests&sortreverse=") + sSharedSortRev);
		else
			Out.Replace(_T("[SortRequests]"), _T("sort=alltimerequests&sortreverse=") + sSharedSortRev);
	}
	else
        Out.Replace(_T("[SortRequests]"), _T("&sort=requests&sortreverse=false"));
    //Accepts sorting link
	if(pThis->m_Params.SharedSort == SHARED_SORT_ACCEPTS)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace(_T("[SortAccepts]"), _T("sort=alltimeaccepts&sortreverse=") + sSharedSortRev);
		else
			Out.Replace(_T("[SortAccepts]"), _T("sort=accepts&sortreverse=") + sSharedSortRev);
	}
	else
	if(pThis->m_Params.SharedSort == SHARED_SORT_ALL_TIME_ACCEPTS)
	{
		if(pThis->m_Params.bSharedSortReverse)
            Out.Replace(_T("[SortAccepts]"), _T("sort=accepts&sortreverse=") + sSharedSortRev);
		else
			Out.Replace(_T("[SortAccepts]"), _T("sort=alltimeaccepts&sortreverse=") + sSharedSortRev);
	}
	else
        Out.Replace(_T("[SortAccepts]"), _T("&sort=accepts&sortreverse=false"));

	if(_ParseURL(Data.sURL, _T("reload")) == _T("true"))
	{
		//CString resultlog = _SpecialChars(theApp.emuledlg->logtext);	//Pick-up last line of the log
		//resultlog = resultlog.TrimRight(_T('\n'));
		//resultlog = resultlog.Mid(resultlog.ReverseFind(_T('\n')));
		CString resultlog = _SpecialChars(theApp.emuledlg->GetLastLogEntry());
		Out.Replace(_T("[Message]"),resultlog);
	}
	else
        Out.Replace(_T("[Message]"),_T(""));


	Out.Replace(_T("[Filename]"), _GetPlainResString(IDS_DL_FILENAME)+CString((pThis->m_Params.SharedSort == SHARED_SORT_NAME)?sortimg:_T("")));
	Out.Replace(_T("[Priority]"),  _GetPlainResString(IDS_PRIORITY)+CString((pThis->m_Params.SharedSort == SHARED_SORT_PRIORITY)?sortimg:_T("")));
	Out.Replace(_T("[FileTransferred]"),  _GetPlainResString(IDS_SF_TRANSFERRED)+CString((pThis->m_Params.SharedSort == SHARED_SORT_TRANSFERRED)?sortimg:_T("")));
	Out.Replace(_T("[FileRequests]"),  _GetPlainResString(IDS_SF_REQUESTS)+CString((pThis->m_Params.SharedSort == SHARED_SORT_REQUESTS)?sortimg:_T("")));
	Out.Replace(_T("[FileAccepts]"),  _GetPlainResString(IDS_SF_ACCEPTS)+CString((pThis->m_Params.SharedSort == SHARED_SORT_ACCEPTS)?sortimg:_T("")));
	Out.Replace(_T("[Size]"), _GetPlainResString(IDS_DL_SIZE)+CString((pThis->m_Params.SharedSort == SHARED_SORT_SIZE)?sortimg:_T("")));
	Out.Replace(_T("[Ed2klink]"), _GetPlainResString(IDS_SW_LINK));
	Out.Replace(_T("[Reload]"), _GetPlainResString(IDS_SF_RELOAD));
	Out.Replace(_T("[Session]"), sSession);

	CString OutE = pThis->m_Templates.sSharedLine; 
	OutE.Replace(_T("[Ed2klink]"), _GetPlainResString(IDS_SW_LINK));
    OutE.Replace(_T("[PriorityUp]"), _GetPlainResString(IDS_PRIORITY_UP));
    OutE.Replace(_T("[PriorityDown]"), _GetPlainResString(IDS_PRIORITY_DOWN));

	CString OutE2 = pThis->m_Templates.sSharedLineChanged; 
	OutE.Replace(_T("[Ed2klink]"), _GetPlainResString(IDS_SW_LINK));
    OutE.Replace(_T("[PriorityUp]"), _GetPlainResString(IDS_PRIORITY_UP));
    OutE.Replace(_T("[PriorityUp]"), _GetPlainResString(IDS_PRIORITY_DOWN));

	CArray<SharedFiles, SharedFiles> SharedArray;
	// Populating array
	for (int ix=0;ix<theApp.sharedfiles->GetCount();ix++)
	{
		CCKey bufKey;
		CKnownFile* cur_file;
		cur_file=theApp.sharedfiles->GetFileByIndex(ix);// m_Files_map.GetNextAssoc(pos,bufKey,cur_file);

		SharedFiles dFile;
		dFile.sFileName = cur_file->GetFileName();
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
	CString sSharedList = _T("");
	for(int i = 0; i < SharedArray.GetCount(); i++)
	{
		TCHAR HTTPTempC[100] = _T("");
		CString HTTPProcessData;
		if (SharedArray[i].sFileHash == _ParseURL(Data.sURL,_T("hash")) )
            HTTPProcessData = OutE2;
		else
            HTTPProcessData = OutE;

		HTTPProcessData.Replace(_T("[FileName]"), _SpecialChars(SharedArray[i].sFileName));
		if(SharedArray[i].sFileName.GetLength() > SHORT_FILENAME_LENGTH)
            HTTPProcessData.Replace(_T("[ShortFileName]"), _SpecialChars(SharedArray[i].sFileName.Left(SHORT_FILENAME_LENGTH)) + _T("..."));
		else
			HTTPProcessData.Replace(_T("[ShortFileName]"), _SpecialChars(SharedArray[i].sFileName));

		_stprintf(HTTPTempC, _T("%s"),CastItoXBytes(SharedArray[i].lFileSize, false, false));
		HTTPProcessData.Replace(_T("[FileSize]"), CString(HTTPTempC));
		HTTPProcessData.Replace(_T("[FileLink]"), SharedArray[i].sED2kLink);

		_stprintf(HTTPTempC, _T("%s"),CastItoXBytes(SharedArray[i].nFileTransferred, false, false));
		HTTPProcessData.Replace(_T("[FileTransferred]"), CString(HTTPTempC));

		_stprintf(HTTPTempC, _T("%s"),CastItoXBytes(SharedArray[i].nFileAllTimeTransferred, false, false));
		HTTPProcessData.Replace(_T("[FileAllTimeTransferred]"), CString(HTTPTempC));

		_stprintf(HTTPTempC, _T("%i"), SharedArray[i].nFileRequests);
		HTTPProcessData.Replace(_T("[FileRequests]"), CString(HTTPTempC));

		_stprintf(HTTPTempC, _T("%i"), SharedArray[i].nFileAllTimeRequests);
		HTTPProcessData.Replace(_T("[FileAllTimeRequests]"), CString(HTTPTempC));

		_stprintf(HTTPTempC, _T("%i"), SharedArray[i].nFileAccepts);
		HTTPProcessData.Replace(_T("[FileAccepts]"), CString(HTTPTempC));

		_stprintf(HTTPTempC, _T("%i"), SharedArray[i].nFileAllTimeAccepts);
		HTTPProcessData.Replace(_T("[FileAllTimeAccepts]"), CString(HTTPTempC));

		HTTPProcessData.Replace(_T("[Priority]"), SharedArray[i].sFilePriority);

		HTTPProcessData.Replace(_T("[FileHash]"), SharedArray[i].sFileHash);

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
        _stprintf(HTTPTempC, _T("%i"), upperpriority);
		HTTPProcessData.Replace(_T("[PriorityUpLink]"), _T("hash=") + SharedArray[i].sFileHash +_T("&setpriority=") + CString(HTTPTempC));
        _stprintf(HTTPTempC, _T("%i"), lesserpriority);
		HTTPProcessData.Replace(_T("[PriorityDownLink]"), _T("hash=") + SharedArray[i].sFileHash +_T("&setpriority=") + CString(HTTPTempC)); 

		sSharedList += HTTPProcessData;
	}
	Out.Replace(_T("[SharedFilesList]"), sSharedList);
	Out.Replace(_T("[Session]"), sSession);

	return Out;
}

CString CWebServer::_GetGraphs(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString Out = pThis->m_Templates.sGraphs;

	CString sGraphDownload = _T(""), sGraphUpload = _T(""), sGraphCons = _T("");
	CString sTmp = _T("");

	for(int i = 0; i < WEB_GRAPH_WIDTH; i++)
	{
		if(i < pThis->m_Params.PointsForWeb.GetCount())
		{
			if(i != 0) {
				sGraphDownload += _T(",");
				sGraphUpload += _T(",");
				sGraphCons += _T(",");
			}
			
			// download
			sTmp.Format(_T("%d") , (uint32) (pThis->m_Params.PointsForWeb[i].download*1024));
			sGraphDownload += sTmp;
			// upload
			sTmp.Format(_T("%d") , (uint32) (pThis->m_Params.PointsForWeb[i].upload*1024));
			sGraphUpload += sTmp;
			// connections
			sTmp.Format(_T("%d") , (uint32) (pThis->m_Params.PointsForWeb[i].connections));
			sGraphCons += sTmp;
		}
	}
	Out.Replace(_T("[GraphDownload]"), sGraphDownload);
	Out.Replace(_T("[GraphUpload]"), sGraphUpload);
	Out.Replace(_T("[GraphConnections]"), sGraphCons);

	Out.Replace(_T("[TxtDownload]"), _GetPlainResString(IDS_DOWNLOAD));
	Out.Replace(_T("[TxtUpload]"), _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace(_T("[TxtTime]"), _GetPlainResString(IDS_TIME));
	Out.Replace(_T("[TxtConnections]"), _GetPlainResString(IDS_SP_ACTCON));
	Out.Replace(_T("[KByteSec]"), _GetPlainResString(IDS_KBYTESEC));
	Out.Replace(_T("[TxtTime]"), _GetPlainResString(IDS_TIME));

	CString sScale;
	sScale.Format(_T("%s"), CastSecondsToHM(thePrefs.GetTrafficOMeterInterval() * WEB_GRAPH_WIDTH) );

	CString s1, s2,s3;
	s1.Format(_T("%d"), thePrefs.GetMaxGraphDownloadRate() + 4);
	s2.Format(_T("%d"), thePrefs.GetMaxGraphUploadRate() + 4);
	s3.Format(_T("%d"), thePrefs.GetMaxConnections()+20);

	Out.Replace(_T("[ScaleTime]"), sScale);
	Out.Replace(_T("[MaxDownload]"), s1);
	Out.Replace(_T("[MaxUpload]"), s2);
	Out.Replace(_T("[MaxConnections]"), s3);

	return Out;
}

CString CWebServer::_GetAddServerBox(ThreadData Data)
{

	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));

	if (!IsSessionAdmin(Data,sSession)) return _T("");

	CString Out = pThis->m_Templates.sAddServerBox;
	if(_ParseURL(Data.sURL, _T("addserver")) == _T("true"))
	{
		CServer* nsrv = new CServer(_tstoi(_ParseURL(Data.sURL, _T("serverport"))), _ParseURL(Data.sURL, _T("serveraddr")).GetBuffer() );
		nsrv->SetListName(_ParseURL(Data.sURL, _T("servername")));
		if (!theApp.emuledlg->serverwnd->serverlistctrl.AddServer(nsrv,true))
			delete nsrv;
		CString resultlog = _SpecialChars(theApp.emuledlg->GetLastLogEntry());
		Out.Replace(_T("[Message]"),resultlog);
	}
	else
		if(_ParseURL(Data.sURL, _T("updateservermetfromurl")) == _T("true"))
		{
				theApp.emuledlg->serverwnd->UpdateServerMetFromURL(_ParseURL(Data.sURL, _T("servermeturl")));
				//CString resultlog = _SpecialChars(theApp.emuledlg->logtext);
				//resultlog = resultlog.TrimRight(_T('\n'));
				//resultlog = resultlog.Mid(resultlog.ReverseFind(_T('\n')));
				CString resultlog = _SpecialChars(theApp.emuledlg->GetLastLogEntry());
				Out.Replace(_T("[Message]"),resultlog);
		}
		else
		Out.Replace(_T("[Message]"), _T(""));
    Out.Replace(_T("[AddServer]"), _GetPlainResString(IDS_SV_NEWSERVER));
	Out.Replace(_T("[IP]"), _GetPlainResString(IDS_SV_ADDRESS));
	Out.Replace(_T("[Port]"), _GetPlainResString(IDS_SV_PORT));
	Out.Replace(_T("[Name]"), _GetPlainResString(IDS_SW_NAME));
	Out.Replace(_T("[Add]"), _GetPlainResString(IDS_SV_ADD));
	Out.Replace(_T("[UpdateServerMetFromURL]"), _GetPlainResString(IDS_SV_MET));
	Out.Replace(_T("[URL]"), _GetPlainResString(IDS_SV_URL));
	Out.Replace(_T("[Apply]"), _GetPlainResString(IDS_PW_APPLY));

	return Out;
}
CString	CWebServer::_GetWebSearch(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));
    
	CString Out = pThis->m_Templates.sWebSearch;
	if(_ParseURL(Data.sURL, _T("tosearch")) != _T(""))
	{
		CString query;
		CString tosearch = _ParseURL(Data.sURL, _T("tosearch"));
		query = _T("http://www.filedonkey.com/fdsearch/index.php?media=");
		query += _ParseURL(Data.sURL, _T("media"));
		tosearch = URLEncode(tosearch);
		tosearch.Replace(_T("%20"),_T("+"));
		query += _T("&pattern=");
		query += _ParseURL(Data.sURL, _T("tosearch"));
		query += _T("&action=search&name=FD-Search&op=modload&file=index&requestby=emule");
		Out += _T("\n<script language=\"javascript\">");
		Out += _T("\n searchwindow=window.open('")+ query + _T("','searchwindow');");
		Out += _T("\n</script>");
	}
	Out.Replace(_T("[Session]"), sSession);
	Out.Replace(_T("[Name]"), _GetPlainResString(IDS_SW_NAME));
	Out.Replace(_T("[Type]"), _GetPlainResString(IDS_TYPE));

	Out.Replace(_T("[FileTypeAny]"), _GetPlainResString(IDS_SEARCH_ANY));
	Out.Replace(_T("[FileTypeAud]"), _GetPlainResString(IDS_SEARCH_AUDIO));
	Out.Replace(_T("[FileTypeVid]"), _GetPlainResString(IDS_SEARCH_VIDEO));
	Out.Replace(_T("[FileTypePro]"), _GetPlainResString(IDS_SEARCH_PRG));

	Out.Replace(_T("[Search]"), _GetPlainResString(IDS_SW_SEARCHBOX));
	Out.Replace(_T("[WebSearch]"), _GetPlainResString(IDS_SW_WEBBASED));
	
	return Out;
}

CString CWebServer::_GetLog(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));

	CString Out = pThis->m_Templates.sLog;

	if (_ParseURL(Data.sURL, _T("clear")) == _T("yes") && IsSessionAdmin(Data,sSession))
	{
		theApp.emuledlg->ResetLog();
	}
	Out.Replace(_T("[Clear]"), _GetPlainResString(IDS_PW_RESET));
	Out.Replace(_T("[Log]"), _SpecialChars(theApp.emuledlg->GetAllLogEntries())+_T("<br><a name=\"end\"></a>"));
	Out.Replace(_T("[Session]"), sSession);

	return Out;
}

CString CWebServer::_GetServerInfo(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));

	CString Out = pThis->m_Templates.sServerInfo;

	if (_ParseURL(Data.sURL, _T("clear")) == _T("yes"))
	{
		theApp.emuledlg->serverwnd->servermsgbox->SetWindowText(_T(""));
	}
	Out.Replace(_T("[Clear]"), _GetPlainResString(IDS_PW_RESET));
	Out.Replace(_T("[ServerInfo]"), _SpecialChars(theApp.emuledlg->serverwnd->servermsgbox->GetText()));
	Out.Replace(_T("[Session]"), sSession);

	return Out;
}

CString CWebServer::_GetDebugLog(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));

	CString Out = pThis->m_Templates.sDebugLog;

	if (_ParseURL(Data.sURL, _T("clear")) == _T("yes") && IsSessionAdmin(Data,sSession))
	{
		theApp.emuledlg->ResetDebugLog();
	}
	Out.Replace(_T("[Clear]"), _GetPlainResString(IDS_PW_RESET));
	Out.Replace(_T("[DebugLog]"), _SpecialChars(theApp.emuledlg->GetAllDebugLogEntries())+_T("<br><a name=\"end\"></a>"));
	Out.Replace(_T("[Session]"), sSession);

	return Out;

}

CString CWebServer::_GetStats(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));

	// refresh statistics
	theApp.emuledlg->statisticswnd->ShowStatistics(true);

	CString Out = pThis->m_Templates.sStats;
	//MORPH START - Changed by SiRoB/Commander, Tree Stat
	/*
	Out.Replace(_T("[STATSDATA]"), theApp.emuledlg->statisticswnd->stattree.GetHTML(false));
	*/
	if (!Out.Replace(_T("[STATSDATA]"), theApp.emuledlg->statisticswnd->stattree.GetHTML(false)))
		Out.Replace(_T("[Stats]"), theApp.emuledlg->statisticswnd->stattree.GetHTMLForExport());
	//MORPH END   - Changed by SiRoB/Commander, Tree Stat

	return Out;

}

CString CWebServer::_GetPreferences(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));

	CString Out = pThis->m_Templates.sPreferences;
	Out.Replace(_T("[Session]"), sSession);

	if ((_ParseURL(Data.sURL, _T("saveprefs")) == _T("true")) && IsSessionAdmin(Data,sSession) ) {
		if(_ParseURL(Data.sURL, _T("gzip")) == _T("true") || _ParseURL(Data.sURL, _T("gzip")).MakeLower() == _T("on"))
		{
			thePrefs.SetWebUseGzip(true);
		}
		if(_ParseURL(Data.sURL, _T("gzip")) == _T("false") || _ParseURL(Data.sURL, _T("gzip")) == _T(""))
		{
			thePrefs.SetWebUseGzip(false);
		}
		if(_ParseURL(Data.sURL, _T("showuploadqueue")) == _T("true") || _ParseURL(Data.sURL, _T("showuploadqueue")).MakeLower() == _T("on") )
		{
			pThis->m_Params.bShowUploadQueue = true;
		}
		if(_ParseURL(Data.sURL, _T("showuploadqueue")) == _T("false") || _ParseURL(Data.sURL, _T("showuploadqueue")) == _T(""))
		{
			pThis->m_Params.bShowUploadQueue = false;
		}
		if(_ParseURL(Data.sURL, _T("refresh")) != _T(""))
		{
			thePrefs.SetWebPageRefresh(_tstoi(_ParseURL(Data.sURL, _T("refresh"))));
		}
		if(_ParseURL(Data.sURL, _T("maxdown")) != _T(""))
		{
			thePrefs.SetMaxDownload(_tstoi(_ParseURL(Data.sURL, _T("maxdown"))));
		}
		if(_ParseURL(Data.sURL, _T("maxup")) != _T(""))
		{
			thePrefs.SetMaxUpload(_tstoi(_ParseURL(Data.sURL, _T("maxup"))));
		}
		uint16 lastmaxgu=thePrefs.GetMaxGraphUploadRate();
		uint16 lastmaxgd=thePrefs.GetMaxGraphDownloadRate();

		if(_ParseURL(Data.sURL, _T("maxcapdown")) != _T(""))
		{
			thePrefs.SetMaxGraphDownloadRate(_tstoi(_ParseURL(Data.sURL, _T("maxcapdown"))));
		}
		if(_ParseURL(Data.sURL, _T("maxcapup")) != _T(""))
		{
			thePrefs.SetMaxGraphUploadRate(_tstoi(_ParseURL(Data.sURL, _T("maxcapup"))));
		}

		if(lastmaxgu != thePrefs.GetMaxGraphUploadRate()) 
			theApp.emuledlg->statisticswnd->SetARange(false,thePrefs.GetMaxGraphUploadRate());
		if(lastmaxgd!=thePrefs.GetMaxGraphDownloadRate())
			theApp.emuledlg->statisticswnd->SetARange(true,thePrefs.GetMaxGraphDownloadRate());


		if(_ParseURL(Data.sURL, _T("maxsources")) != _T(""))
		{
			thePrefs.SetMaxSourcesPerFile(_tstoi(_ParseURL(Data.sURL, _T("maxsources"))));
		}
		if(_ParseURL(Data.sURL, _T("maxconnections")) != _T(""))
		{
			thePrefs.SetMaxConnections(_tstoi(_ParseURL(Data.sURL, _T("maxconnections"))));
		}
		if(_ParseURL(Data.sURL, _T("maxconnectionsperfive")) != _T(""))
		{
			thePrefs.SetMaxConsPerFive(_tstoi(_ParseURL(Data.sURL, _T("maxconnectionsperfive"))));
		}
		thePrefs.SetTransferFullChunks((_ParseURL(Data.sURL, _T("fullchunks")).MakeLower() == _T("on")));
		thePrefs.SetPreviewPrio((_ParseURL(Data.sURL, _T("firstandlast")).MakeLower() == _T("on")));

		thePrefs.SetNetworkED2K((_ParseURL(Data.sURL, _T("neted2k")).MakeLower() == _T("on")));
		thePrefs.SetNetworkKademlia((_ParseURL(Data.sURL, _T("netkad")).MakeLower() == _T("on")));
	}
	
	// Fill form
	if(thePrefs.GetWebUseGzip())
	{
		Out.Replace(_T("[UseGzipVal]"), _T("checked"));
	}
	else
	{
		Out.Replace(_T("[UseGzipVal]"), _T(""));
	}
    if(pThis->m_Params.bShowUploadQueue)
	{
		Out.Replace(_T("[ShowUploadQueueVal]"), _T("checked"));
	}
	else
	{
		Out.Replace(_T("[ShowUploadQueueVal]"), _T(""));
	}
	if(thePrefs.GetPreviewPrio())
	{
		Out.Replace(_T("[FirstAndLastVal]"), _T("checked"));
	}
	else
	{
		Out.Replace(_T("[FirstAndLastVal]"), _T(""));
	}
	if(thePrefs.TransferFullChunks())
	{
		Out.Replace(_T("[FullChunksVal]"), _T("checked"));
	}
	else
	{
		Out.Replace(_T("[FullChunksVal]"), _T(""));
	}
	CString sRefresh;
	
	sRefresh.Format(_T("%d"), thePrefs.GetWebPageRefresh());
	Out.Replace(_T("[RefreshVal]"), sRefresh);

	sRefresh.Format(_T("%d"), thePrefs.GetMaxSourcePerFile());
	Out.Replace(_T("[MaxSourcesVal]"), sRefresh);

	sRefresh.Format(_T("%d"), thePrefs.GetMaxConnections());
	Out.Replace(_T("[MaxConnectionsVal]"), sRefresh);

	sRefresh.Format(_T("%d"), thePrefs.GetMaxConperFive());
	Out.Replace(_T("[MaxConnectionsPer5Val]"), sRefresh);

	Out.Replace(_T("[ED2KVAL]"), (thePrefs.GetNetworkED2K())?_T("checked"):_T("") );
	Out.Replace(_T("[KADVAL]"), (thePrefs.GetNetworkKademlia())?_T("checked"):_T("") );

	Out.Replace(_T("[KBS]"), _GetPlainResString(IDS_KBYTESEC)+_T(":"));
	Out.Replace(_T("[FileSettings]"), CString(_GetPlainResString(IDS_WEB_FILESETTINGS)+_T(":")));
	Out.Replace(_T("[LimitForm]"), _GetPlainResString(IDS_WEB_CONLIMITS)+_T(":"));
	Out.Replace(_T("[MaxSources]"), _GetPlainResString(IDS_PW_MAXSOURCES)+_T(":"));
	Out.Replace(_T("[MaxConnections]"), _GetPlainResString(IDS_PW_MAXC)+_T(":"));
	Out.Replace(_T("[MaxConnectionsPer5]"), _GetPlainResString(IDS_MAXCON5SECLABEL)+_T(":"));
	Out.Replace(_T("[UseGzipForm]"), _GetPlainResString(IDS_WEB_GZIP_COMPRESSION));
	Out.Replace(_T("[UseGzipComment]"), _GetPlainResString(IDS_WEB_GZIP_COMMENT));
	Out.Replace(_T("[ShowUploadQueueForm]"), _GetPlainResString(IDS_WEB_SHOW_UPLOAD_QUEUE));
	Out.Replace(_T("[ShowUploadQueueComment]"), _GetPlainResString(IDS_WEB_UPLOAD_QUEUE_COMMENT));
	Out.Replace(_T("[ShowQueue]"), _GetPlainResString(IDS_WEB_SHOW_UPLOAD_QUEUE));
	Out.Replace(_T("[HideQueue]"), _GetPlainResString(IDS_WEB_HIDE_UPLOAD_QUEUE));
	Out.Replace(_T("[RefreshTimeForm]"), _GetPlainResString(IDS_WEB_REFRESH_TIME));
	Out.Replace(_T("[RefreshTimeComment]"), _GetPlainResString(IDS_WEB_REFRESH_COMMENT));
	Out.Replace(_T("[SpeedForm]"), _GetPlainResString(IDS_SPEED_LIMITS));
	Out.Replace(_T("[MaxDown]"), _GetPlainResString(IDS_DOWNLOAD));
	Out.Replace(_T("[MaxUp]"), _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace(_T("[SpeedCapForm]"), _GetPlainResString(IDS_CAPACITY_LIMITS));
	Out.Replace(_T("[MaxCapDown]"), _GetPlainResString(IDS_DOWNLOAD));
	Out.Replace(_T("[MaxCapUp]"), _GetPlainResString(IDS_PW_CON_UPLBL));
	Out.Replace(_T("[TryFullChunks]"), _GetPlainResString(IDS_FULLCHUNKTRANS));
	Out.Replace(_T("[FirstAndLast]"), _GetPlainResString(IDS_DOWNLOADMOVIECHUNKS));
	Out.Replace(_T("[WebControl]"), _GetPlainResString(IDS_WEB_CONTROL));
	Out.Replace(_T("[eMuleAppName]"), _T("eMule"));
	Out.Replace(_T("[Apply]"), _GetPlainResString(IDS_PW_APPLY));

	Out.Replace(_T("[NETWORKS]"), _GetPlainResString(IDS_NETWORK));

	Out.Replace(_T("[BOOTSTRAP]"), _GetPlainResString(IDS_BOOTSTRAP));
	Out.Replace(_T("[BS_IP]"), _GetPlainResString(IDS_IP));
	Out.Replace(_T("[BS_PORT]"), _GetPlainResString(IDS_PORT));

	Out.Replace(_T("[KADEMLIA]"), GetResString(IDS_KADEMLIA) );

	CString sT;
	int n = (int)thePrefs.GetMaxDownload();
	if(n < 0 || n == 65535) n = 0;
	sT.Format(_T("%d"), n);
	Out.Replace(_T("[MaxDownVal]"), sT);
	n = (int)thePrefs.GetMaxUpload();
	if(n < 0 || n == 65535) n = 0;
	sT.Format(_T("%d"), n);
	Out.Replace(_T("[MaxUpVal]"), sT);
	sT.Format(_T("%d"), thePrefs.GetMaxGraphDownloadRate());
	Out.Replace(_T("[MaxCapDownVal]"), sT);
	sT.Format(_T("%d"), thePrefs.GetMaxGraphUploadRate());
	Out.Replace(_T("[MaxCapUpVal]"), sT);

	return Out;
}

CString CWebServer::_GetLoginScreen(ThreadData Data)
{

	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));

	CString Out = _T("");

	Out += pThis->m_Templates.sLogin;

	Out.Replace(_T("[CharSet]"), _GetWebCharSet());
	Out.Replace(_T("[eMulePlus]"), _T("eMule"));
	Out.Replace(_T("[eMuleAppName]"), _T("eMule"));
	//MORPH START - Changed by SiRoB, [itsonlyme: -modname-]
	/*
	Out.Replace(_T("[version]"), theApp.m_strCurVersionLong);
	*/
	Out.Replace(_T("[version]"), theApp.m_strCurVersionLong + _T(" [") + theApp.m_strModLongVersion + _T("]"));
	//MORPH END   - Changed by SiRoB, [itsonlyme: -modname-]
	Out.Replace(_T("[Login]"), _GetPlainResString(IDS_WEB_LOGIN));
	Out.Replace(_T("[EnterPassword]"), _GetPlainResString(IDS_WEB_ENTER_PASSWORD));
	Out.Replace(_T("[LoginNow]"), _GetPlainResString(IDS_WEB_LOGIN_NOW));
	Out.Replace(_T("[WebControl]"), _GetPlainResString(IDS_WEB_CONTROL));

	return Out;
}

//MORPH START - Added by SiRoB/Commander, FAILEDLOGIN
CString CWebServer::_GetFailedLoginScreen(ThreadData Data)
{

	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));

	CString Out = _T("");

	Out += pThis->m_Templates.sFailedLogin;

	Out.Replace(_T("[CharSet]"), _GetWebCharSet());
	Out.Replace(_T("[eMulePlus]"), _T("eMule"));
	Out.Replace(_T("[eMuleAppName]"), _T("eMule"));
	//MORPH START - Changed by SiRoB, [itsonlyme: -modname-]
	/*
	Out.Replace(_T("[version]"), theApp.m_strCurVersionLong);
	*/
	Out.Replace(_T("[version]"), theApp.m_strCurVersionLong + _T(" [") + theApp.m_strModLongVersion + _T("]"));
	//MORPH END   - Changed by SiRoB, [itsonlyme: -modname-]
	Out.Replace(_T("[Login]"), _GetPlainResString(IDS_WEB_LOGIN));
	Out.Replace(_T("[BanMessage]"), _T("You have been banned for 15 min due to failed login attempts!"));
	Out.Replace(_T("[LoginNow]"), _GetPlainResString(IDS_WEB_LOGIN_NOW));
	Out.Replace(_T("[WebControl]"), _GetPlainResString(IDS_WEB_CONTROL));

	return Out;
}
//MORPH END   - Added by SiRoB/Commander, FAILEDLOGIN

CString CWebServer::_GetRemoteLinkAddedOk(ThreadData Data)
{

	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString Out = _T("");

    int cat=_tstoi(_ParseURL(Data.sURL,_T("cat")));
	CString HTTPTemp = _ParseURL(Data.sURL, _T("c"));
	theApp.AddEd2kLinksToDownload(HTTPTemp,cat);

    Out += _T("<status result=\"OK\">");
    Out += _T("<description>") + GetResString(IDS_WEB_REMOTE_LINK_ADDED) + _T("</description>");
    Out += _T("<filename>") + HTTPTemp + _T("</filename>");
    Out += _T("</status>");

	return Out;
}

CString CWebServer::_GetRemoteLinkAddedFailed(ThreadData Data)
{

	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString Out = _T("");

    Out += _T("<status result=\"FAILED\" reason=\"WRONG_PASSWORD\">");
    Out += _T("<description>") + GetResString(IDS_WEB_REMOTE_LINK_NOT_ADDED) + _T("</description>");
    Out += _T("</status>");

	return Out;
}

CString CWebServer::_GetConnectedServer(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));

	CString HTTPTemp = _T("");
	TCHAR	HTTPTempC[100] = _T("");
	CString OutS = pThis->m_Templates.sConnectedServer;
	OutS.Replace(_T("[ConnectedServer]"), _GetPlainResString(IDS_PW_SERVER));
	OutS.Replace(_T("[Servername]"), _GetPlainResString(IDS_SL_SERVERNAME));
	OutS.Replace(_T("[Status]"), _GetPlainResString(IDS_STATUS));
	OutS.Replace(_T("[Usercount]"), _GetPlainResString(IDS_LUSERS));
	OutS.Replace(_T("[Action]"), _GetPlainResString(IDS_CONNECTING));
	OutS.Replace(_T("[URL_Disconnect]"), IsSessionAdmin(Data,sSession)?CString(_T("?ses=") + sSession + _T("&w=server&c=disconnect")):GetPermissionDenied());
	OutS.Replace(_T("[URL_Connect]"), IsSessionAdmin(Data,sSession)?CString(_T("?ses=") + sSession + _T("&w=server&c=connect")):GetPermissionDenied());
	OutS.Replace(_T("[Disconnect]"), _GetPlainResString(IDS_IRC_DISCONNECT));
	OutS.Replace(_T("[Connect]"), _GetPlainResString(IDS_CONNECTTOANYSERVER));
	OutS.Replace(_T("[URL_ServerOptions]"), IsSessionAdmin(Data,sSession)?CString(_T("?ses=") + sSession + _T("&w=server&c=options")):GetPermissionDenied());
	OutS.Replace(_T("[ServerOptions]"), CString(_GetPlainResString(IDS_SERVER)+_GetPlainResString(IDS_EM_PREFS)));

	if (theApp.IsConnected()) {
		if(!theApp.IsFirewalled())
			OutS.Replace(_T("[1]"), _GetPlainResString(IDS_CONNECTED));
		else
			OutS.Replace(_T("[1]"), _GetPlainResString(IDS_CONNECTED) + _T(" (") + _GetPlainResString(IDS_IDLOW) + _T(")"));

		if(theApp.serverconnect->IsConnected()){
			CServer* cur_server = theApp.serverconnect->GetCurrentServer();
			OutS.Replace(_T("[2]"), cur_server->GetListName());
	
			_stprintf(HTTPTempC, _T("%10i"), cur_server->GetUsers());
			HTTPTemp = HTTPTempC;												
			OutS.Replace(_T("[3]"), HTTPTemp);
		}

	} else if (theApp.serverconnect->IsConnecting()) {
		OutS.Replace(_T("[1]"), _GetPlainResString(IDS_CONNECTING));
		OutS.Replace(_T("[2]"), _T(""));
		OutS.Replace(_T("[3]"), _T(""));
	} else {
		OutS.Replace(_T("[1]"), _GetPlainResString(IDS_DISCONNECTED));
		OutS.Replace(_T("[2]"), _T(""));
		OutS.Replace(_T("[3]"), _T(""));
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
		return _T("");

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
		return _T("");

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

	long sessionID=_tstol(SsessionID);
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
	return _T("javascript:alert(\'")+_GetPlainResString(IDS_ACCESSDENIED)+_T("\')");
}

bool CWebServer::_GetFileHash(CString sHash, uchar *FileHash)
{
	char hex_byte[3];
	int byte;
	hex_byte[2] = _T('\0');
	for (int i = 0; i < 16; i++) 
	{
		hex_byte[0] = sHash.GetAt(i*2);
		hex_byte[1] = sHash.GetAt((i*2 + 1));
		sscanf(hex_byte, "%02x", &byte);
		FileHash[i] = (uchar)byte;
	}
	return true;
}

CString	CWebServer::_GetPlainResString(RESSTRIDTYPE nID, bool noquot)
{
	CString sRet = _GetResString(nID);
	sRet.Remove('&');

	PlainString(sRet, noquot);
	return sRet;
}

CString	CWebServer::_GetWebCharSet()
{
#ifdef _UNICODE
	return _T("utf-8");
#else
	CString strHtmlCharset = thePrefs.GetHtmlCharset();
	if (!strHtmlCharset.IsEmpty())
		return strHtmlCharset;
	return _T("ISO-8859-1"); // Western European (ISO)
#endif
}

// Ornis: creating the progressbar. colored if ressources are given/available
CString CWebServer::_GetDownloadGraph(ThreadData Data,CString filehash)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CPartFile* cur_file;

	// cool style
	CString progresscolor[12];
	progresscolor[0]=_T("transparent.gif");
	progresscolor[1]=_T("black.gif");
	progresscolor[2]=_T("yellow.gif");
	progresscolor[3]=_T("red.gif");

	progresscolor[4]=_T("blue1.gif");
	progresscolor[5]=_T("blue2.gif");
	progresscolor[6]=_T("blue3.gif");
	progresscolor[7]=_T("blue4.gif");
	progresscolor[8]=_T("blue5.gif");
	progresscolor[9]=_T("blue6.gif");

	progresscolor[10]=_T("green.gif");
	progresscolor[11]=_T("greenpercent.gif");

	uchar fileid[16];
	if (filehash.GetLength()!=32 || !DecodeBase16(filehash.GetBuffer(),filehash.GetLength(),fileid,ARRSIZE(fileid)))
		return _T("");

	CString Out = _T("");
	CString temp;

	cur_file=theApp.downloadqueue->GetFileByID(fileid);
	if (cur_file==0 || !cur_file->IsPartFile()) {
		temp.Format(pThis->m_Templates.sProgressbarImgsPercent+_T("<br>"),progresscolor[11],pThis->m_Templates.iProgressbarWidth);
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
			if (lastcolor!= _tstoi(s_ChunkBar.Mid(i,1))  ) {
				if (i>lastindex) {
					temp.Format(pThis->m_Templates.sProgressbarImgs ,progresscolor[lastcolor],i-lastindex);

					Out+=temp;
				}
				lastcolor=_tstoi(s_ChunkBar.Mid(i,1));
				lastindex=i;
			}
		}
		temp.Format(pThis->m_Templates.sProgressbarImgs,progresscolor[lastcolor],pThis->m_Templates.iProgressbarWidth-lastindex);
		Out+=temp;

		int compl=(int)((pThis->m_Templates.iProgressbarWidth/100.0)*cur_file->GetPercentCompleted());
		(compl>0)?temp.Format(pThis->m_Templates.sProgressbarImgsPercent+_T("<br>"),progresscolor[11],compl) :temp.Format(pThis->m_Templates.sProgressbarImgsPercent+_T("<br>"),progresscolor[0],5);
		Out=temp+Out;
	}
	return Out;
}

CString	CWebServer::_GetSearch(ThreadData Data)
{
	CWebServer *pThis = (CWebServer *)Data.pThis;
	if(pThis == NULL)
		return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));
	CString Out = pThis->m_Templates.sSearch;

	if (_ParseURL(Data.sURL, _T("downloads")) != _T("") && IsSessionAdmin(Data,sSession) ) {
		CString downloads=_ParseURLArray(Data.sURL,_T("downloads"));
		uint8 cat=_tstoi(_ParseURL(Data.sURL, _T("cat")));

		CString resToken;
		int curPos=0;
		resToken= downloads.Tokenize(_T("|"),curPos);
		while (resToken != _T(""))
		{
			uchar fileid[16];
			if (resToken.GetLength()==32 && DecodeBase16(resToken.GetBuffer(),resToken.GetLength(),fileid,ARRSIZE(fileid)))
				theApp.searchlist->AddFileToDownloadByHash(fileid,cat);
			resToken= downloads.Tokenize(_T("|"),curPos);
		}
	}

	if(_ParseURL(Data.sURL, _T("tosearch")) != _T("") && IsSessionAdmin(Data,sSession) )
	{
		// perform search
		theApp.emuledlg->searchwnd->DeleteAllSearchs();

		// get method
		CString method=(_ParseURL(Data.sURL, _T("method")));

		SSearchParams* pParams = new SSearchParams;
		pParams->strExpression = _ParseURL(Data.sURL, _T("tosearch"));
		pParams->strFileType = _ParseURL(Data.sURL, _T("type"));
		// for safety: this string is sent to servers and/or kad nodes, validate it!
		if (!pParams->strFileType.IsEmpty()
			&& pParams->strFileType != ED2KFTSTR_ARCHIVE
			&& pParams->strFileType != ED2KFTSTR_AUDIO
			&& pParams->strFileType != ED2KFTSTR_CDIMAGE
			&& pParams->strFileType != ED2KFTSTR_DOCUMENT
			&& pParams->strFileType != ED2KFTSTR_IMAGE
			&& pParams->strFileType != ED2KFTSTR_PROGRAM
			&& pParams->strFileType != ED2KFTSTR_VIDEO){
			ASSERT(0);
			pParams->strFileType.Empty();
		}
		pParams->ulMinSize = _tstol(_ParseURL(Data.sURL, _T("min")))*1048576;
		pParams->ulMaxSize = _tstol(_ParseURL(Data.sURL, _T("max")))*1048576;
		if (pParams->ulMaxSize < pParams->ulMinSize)
			pParams->ulMaxSize = 0;
		
		pParams->uAvailability = (_ParseURL(Data.sURL, _T("avail"))==_T(""))?0:_tstoi(_ParseURL(Data.sURL, _T("avail")));
		if (pParams->uAvailability > 1000000)
			pParams->uAvailability = 1000000;

		pParams->strExtension = _ParseURL(Data.sURL, _T("ext"));
		if (method == _T("kademlia"))
			pParams->eType = SearchTypeKademlia;
		else if (method == _T("global"))
			pParams->eType = SearchTypeEd2kGlobal;
		else
			pParams->eType = SearchTypeEd2kServer;

		CString strResponse = _GetPlainResString(IDS_SW_SEARCHINGINFO);
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
		Out.Replace(_T("[Message]"),strResponse);
	}
	else if(_ParseURL(Data.sURL, _T("tosearch")) != _T("") && !IsSessionAdmin(Data,sSession) ) {
		Out.Replace(_T("[Message]"),_GetPlainResString(IDS_ACCESSDENIED));
	}
	else Out.Replace(_T("[Message]"),_T(""));

	CString sSort = _ParseURL(Data.sURL, _T("sort"));	if (sSort.GetLength()>0) pThis->m_iSearchSortby=_tstoi(sSort);
	sSort = _ParseURL(Data.sURL, _T("sortAsc"));		if (sSort.GetLength()>0) pThis->m_bSearchAsc=_tstoi(sSort);

	CString result=pThis->m_Templates.sSearchHeader +
		theApp.searchlist->GetWebList(pThis->m_Templates.sSearchResultLine,pThis->m_iSearchSortby,pThis->m_bSearchAsc);

	if (thePrefs.GetCatCount()>1) InsertCatBox(Out,0,pThis->m_Templates.sCatArrow); else Out.Replace(_T("[CATBOX]"),_T(""));
	
	CString sortimg;
	if (pThis->m_bSearchAsc) sortimg=_T("<img src=\"l_up.gif\">");
		else sortimg=_T("<img src=\"l_down.gif\">");

	Out.Replace(_T("[SEARCHINFOMSG]"),_T(""));
	Out.Replace(_T("[RESULTLIST]"), result);
	Out.Replace(_T("[Result]"), GetResString(IDS_SW_RESULT) );
	Out.Replace(_T("[Session]"), sSession);
	Out.Replace(_T("[WebSearch]"), _GetPlainResString(IDS_SW_WEBBASED));
	Out.Replace(_T("[Name]"), _GetPlainResString(IDS_SW_NAME));
	Out.Replace(_T("[Type]"), _GetPlainResString(IDS_TYPE));

	// 'file type' display strings
	Out.Replace(_T("[FileTypeAny]"), _GetPlainResString(IDS_SEARCH_ANY));
	Out.Replace(_T("[FileTypeArc]"), _GetPlainResString(IDS_SEARCH_ARC));
	Out.Replace(_T("[FileTypeAud]"), _GetPlainResString(IDS_SEARCH_AUDIO));
	Out.Replace(_T("[FileTypeIso]"), _GetPlainResString(IDS_SEARCH_CDIMG));
	Out.Replace(_T("[FileTypeDoc]"), _GetPlainResString(IDS_SEARCH_DOC));
	Out.Replace(_T("[FileTypeImg]"), _GetPlainResString(IDS_SEARCH_PICS));
	Out.Replace(_T("[FileTypePro]"), _GetPlainResString(IDS_SEARCH_PRG));
	Out.Replace(_T("[FileTypeVid]"), _GetPlainResString(IDS_SEARCH_VIDEO));

	// 'file type' ID strings
	Out.Replace(_T("[FileTypeAnyId]"), _T(""));
	Out.Replace(_T("[FileTypeArcId]"), _T(ED2KFTSTR_ARCHIVE));
	Out.Replace(_T("[FileTypeAudId]"), _T(ED2KFTSTR_AUDIO));
	Out.Replace(_T("[FileTypeIsoId]"), _T(ED2KFTSTR_CDIMAGE));
	Out.Replace(_T("[FileTypeDocId]"), _T(ED2KFTSTR_DOCUMENT));
	Out.Replace(_T("[FileTypeImgId]"), _T(ED2KFTSTR_IMAGE));
	Out.Replace(_T("[FileTypeProId]"), _T(ED2KFTSTR_PROGRAM));
	Out.Replace(_T("[FileTypeVidId]"), _T(ED2KFTSTR_VIDEO));

	Out.Replace(_T("[Search]"), _GetPlainResString(IDS_SW_SEARCHBOX));
	Out.Replace(_T("[RefetchResults]"), _GetPlainResString(IDS_SW_REFETCHRES));
	Out.Replace(_T("[Download]"), _GetPlainResString(IDS_DOWNLOAD));

	Out.Replace(_T("[Filesize]"), _GetPlainResString(IDS_DL_SIZE)+CString((pThis->m_iSearchSortby==1)?sortimg:_T("")));
	Out.Replace(_T("[Sources]"), _GetPlainResString(IDS_DL_SOURCES)+CString((pThis->m_iSearchSortby==3)?sortimg:_T("")));
	Out.Replace(_T("[Filehash]"), _GetPlainResString(IDS_FILEHASH)+CString((pThis->m_iSearchSortby==2)?sortimg:_T("")));
	Out.Replace(_T("[Filename]"), _GetPlainResString(IDS_DL_FILENAME)+CString((pThis->m_iSearchSortby==0)?sortimg:_T("")));
	Out.Replace(_T("[WebSearch]"), _GetPlainResString(IDS_SW_WEBBASED));

	Out.Replace(_T("[SizeMin]"), _GetPlainResString(IDS_SEARCHMINSIZE));
	Out.Replace(_T("[SizeMax]"), _GetPlainResString(IDS_SEARCHMAXSIZE));
	Out.Replace(_T("[Availabl]"), _GetPlainResString(IDS_SEARCHAVAIL));
	Out.Replace(_T("[Extention]"), _GetPlainResString(IDS_SEARCHEXTENTION));
	Out.Replace(_T("[MB]"), _GetPlainResString(IDS_MBYTES));
	
	Out.Replace(_T("[METHOD]"), _GetPlainResString(IDS_METHOD));
	Out.Replace(_T("[USESSERVER]"), _GetPlainResString(IDS_SERVER));
	Out.Replace(_T("[USEKADEMLIA]"), _GetPlainResString(IDS_KADEMLIA));
	Out.Replace(_T("[Global]"), _GetPlainResString(IDS_GLOBALSEARCH));

	CString val;
	val.Format(_T("%i"),(pThis->m_iSearchSortby!=0 || (pThis->m_iSearchSortby==0 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace(_T("[SORTASCVALUE0]"), val);
	val.Format(_T("%i"),(pThis->m_iSearchSortby!=1 || (pThis->m_iSearchSortby==1 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace(_T("[SORTASCVALUE1]"), val);
	val.Format(_T("%i"),(pThis->m_iSearchSortby!=2 || (pThis->m_iSearchSortby==2 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace(_T("[SORTASCVALUE2]"), val);
	val.Format(_T("%i"),(pThis->m_iSearchSortby!=3 || (pThis->m_iSearchSortby==3 && pThis->m_bSearchAsc==0 ))?1:0 );
	Out.Replace(_T("[SORTASCVALUE3]"), val);

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
		if(ts.GetTotalSeconds() > thePrefs.GetWebTimeoutMins()*60 ) {
			m_Params.Sessions.RemoveAt(i);
			theApp.emuledlg->serverwnd->UpdateMyInfo();
			AddLogLine(true,GetResString(IDS_WEB_SESSIONEND));
		}
		else
			i++;
	}

	if (oldvalue!=m_Params.Sessions.GetSize()) theApp.emuledlg->serverwnd->UpdateMyInfo();
		
	return m_Params.Sessions.GetCount();
}
void CWebServer::InsertCatBox(CString &Out,int preselect,CString boxlabel,bool jump,bool extraCats) {
	CString tempBuf2,tempBuf3;
	if (jump) tempBuf2=_T("onchange=GotoCat(this.form.cat.options[this.form.cat.selectedIndex].value)>");
	else tempBuf2=_T(">");
	CString tempBuf=_T("<form><select name=\"cat\" size=\"1\"")+tempBuf2;

	for (int i=0;i< thePrefs.GetCatCount();i++) {
		tempBuf3= (i==preselect)? _T(" selected"):_T("");
		tempBuf2.Format(_T("<option%s value=\"%i\">%s</option>"),tempBuf3,i, (i==0)?_GetPlainResString(IDS_ALL):thePrefs.GetCategory(i)->title );
		tempBuf.Append(tempBuf2);
	}
	if (extraCats) {
		if (thePrefs.GetCatCount()>1){
			tempBuf2.Format(_T("<option>------------</option>"));
			tempBuf.Append(tempBuf2);
		}
	
		for (int i=(thePrefs.GetCatCount()>1)?1:2;i<=12;i++) {
			tempBuf3= ( (-i)==preselect)? _T(" selected"):_T("");
			tempBuf2.Format(_T("<option%s value=\"%i\">%s</option>"),tempBuf3,-i, GetSubCatLabel(-i) );
			tempBuf.Append(tempBuf2);
		}
	}
	tempBuf.Append(_T("</select></form>"));
	Out.Replace(_T("[CATBOX]"),boxlabel+tempBuf);
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
	return _T("?");
}

// Elandal: moved from CUpDownClient
// Webserber [kuchin]
CString CWebServer::GetUploadFileInfo(CUpDownClient* client)
{
	if(client == NULL) return _T("");
	CString sRet;

	// build info text and display it
	sRet.Format(GetResString(IDS_USERINFO), client->GetUserName(), client->GetUserIDHybrid());
	if (client->GetRequestFile())
	{
		sRet += GetResString(IDS_SF_REQUESTED) + CString(client->GetRequestFile()->GetFileName()) + _T("\n");
		CString stat;
		stat.Format(GetResString(IDS_FILESTATS_SESSION)+GetResString(IDS_FILESTATS_TOTAL),
			client->GetRequestFile()->statistic.GetAccepts(),
			client->GetRequestFile()->statistic.GetRequests(),
			CastItoXBytes(client->GetRequestFile()->statistic.GetTransferred(), false, false),
			client->GetRequestFile()->statistic.GetAllTimeAccepts(),
			client->GetRequestFile()->statistic.GetAllTimeRequests(),
			CastItoXBytes(client->GetRequestFile()->statistic.GetAllTimeTransferred(), false, false));
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
		return _T("");

	if (!thePrefs.GetNetworkKademlia()) return _T("");

	CString sSession = _ParseURL(Data.sURL, _T("ses"));
	CString Out = pThis->m_Templates.sKad;

	if (_ParseURL(Data.sURL, _T("bootstrap")) != _T("") && IsSessionAdmin(Data,sSession) ) {
		CString dest=_ParseURL(Data.sURL, _T("ipport"));
		int pos=dest.Find(_T(':'));
		if (pos!=-1) {
			uint16 port=_tstoi(dest.Right( dest.GetLength()-pos-1));
			CString ip=dest.Left(pos);
			Kademlia::CKademlia::bootstrap(ip,port);
		}
	}

	if (_ParseURL(Data.sURL, _T("c")) == _T("connect") && IsSessionAdmin(Data,sSession) ) {
		Kademlia::CKademlia::start();
	}

	if (_ParseURL(Data.sURL, _T("c")) == _T("disconnect") && IsSessionAdmin(Data,sSession) ) {
		Kademlia::CKademlia::stop();
	}

	// check the condition if bootstrap is possible
	if ( Kademlia::CKademlia::isRunning() &&  !Kademlia::CKademlia::isConnected()) {

		Out.Replace(_T("[BOOTSTRAPLINE]"), pThis->m_Templates.sBootstrapLine );

		// Bootstrap
		CString bsip=_ParseURL(Data.sURL, _T("bsip"));
		uint16 bsport=_tstoi(_ParseURL(Data.sURL, _T("bsport")));

		if (!bsip.IsEmpty() && bsport>0)
			Kademlia::CKademlia::bootstrap(bsip,bsport);
	} else Out.Replace(_T("[BOOTSTRAPLINE]"), _T("") );

	// Infos
	CString info;
	if (thePrefs.GetNetworkKademlia()) {
		CString buffer;
			
			buffer.Format(_T("%s: %i<br>"), GetResString(IDS_KADCONTACTLAB), theApp.emuledlg->kademliawnd->GetContactCount());
			info.Append(buffer);

			buffer.Format(_T("%s: %i<br>"), GetResString(IDS_KADSEARCHLAB), theApp.emuledlg->kademliawnd->searchList->GetItemCount());
			info.Append(buffer);

		
	} else info=_T("");
	Out.Replace(_T("[KADSTATSDATA]"),info);

	Out.Replace(_T("[BS_IP]"),GetResString(IDS_IP));
	Out.Replace(_T("[BS_PORT]"),GetResString(IDS_PORT));
	Out.Replace(_T("[BOOTSTRAP]"),GetResString(IDS_BOOTSTRAP));
	Out.Replace(_T("[KADSTAT]"),GetResString(IDS_STATSSETUPINFO));

	return Out;
}
