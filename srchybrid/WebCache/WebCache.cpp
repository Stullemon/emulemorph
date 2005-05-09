#include "stdafx.h"
#include <share.h>
#include "WebCache.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "WebCachedBlock.h" // yonatan http
#include "eMule.h"
#include "WebCachedBlockList.h"
#include <windns.h>
#pragma warning(disable:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#include <crypto51/osrng.h>
#pragma warning(default:4516) // access-declarations are deprecated; member using-declarations provide a better alternative
#include "opcodes.h"
#include "kademlia/kademlia/Kademlia.h"
#include "RichEditCtrlX.h"
#include <atlenc.h>
#include "log.h"
#include "WebCacheSocket.h"//JP proxy configuration test
#include "Packets.h"//JP proxy configuration test
#include "Statistics.h" //JP proxy configuration test

#include <atlrx.h>
#import <msxml3.dll> // Superlexx - XML-based proxy auto-detector

#define MAX_PROXY_CONN 5

CUpDownClient* ProxyTestClient; //JP proxy configuration test

uint32 ProxyConnectionCount = 0;

bool AllowProxyConnection()
{
	return ProxyConnectionCount < MAX_PROXY_CONN;
}

uint32 ResolveWebCacheName() // returns 0 on error
{
	static uint32 wcip = 0;
	static bool popupFlag = false; // true if error message was already displayed once

	if( wcip == 0) {
		hostent* remoteHost;
		remoteHost = gethostbyname(CT2CA(thePrefs.webcacheName));
		if( !remoteHost ) {
			if( !popupFlag) {
				popupFlag = true; //moved up so it doesn't pop up several times if the popup isn't closed
				CString msg;
				msg.Format( _T( "WebCache Error - Failed to resolve HTTP proxy address:\n" ) \
							_T( "%s\n" ) \
							_T( "Please review your webcache settings" ), thePrefs.webcacheName );
				//MORPH START - Changed by SiRoB, Avoid crash in some case
				//if we want to popup a message we need to use windows sendmessage
				/*
				AfxMessageBox(msg);
				*/
				theApp.QueueLogLine(true,msg);
				//MORPH END   - Changed by SiRoB, Avoid crash in some case
			}
			return 0; // can't resolve..
		}
		wcip = *(reinterpret_cast<uint32*>(remoteHost->h_addr_list[0]));
	}
	return wcip;
}

// webmule start -- from Superlexx code
CSimpleMap<int, CString> webcachelist;

CString	WebCacheIndex2Name(int index){
    CString s = webcachelist.Lookup(index);
	return s ? s : _T("");
}

int	AddWebCache(CString webcachename){
	if (webcachelist.Add(webcachelist.GetSize(), webcachename))
		return webcachelist.GetSize()-1;
    else
        return -1;
}

int	GetWebCacheIndex(CString webcachename){
    return webcachelist.FindVal(webcachename);
}
// webmule end

// Superlexx - Proxy AutoDetect - start ////////////////////////////////////////////////////////

CString ReverseDnsLookupForWebCache(DWORD dwIP)	// taken from 0.43b PeerCache code
{
	CString strHostName;

	HMODULE hLib = LoadLibrary(_T("dnsapi.dll"));
	if (hLib)
	{
		DNS_STATUS (WINAPI *pfnDnsQueryConfig)(DNS_CONFIG_TYPE Config, DWORD Flag, PWSTR pwsAdapterName, PVOID pReserved, PVOID pBuffer, PDWORD pBufferLength);
		DNS_STATUS (WINAPI *pfnDnsQuery)(PCTSTR pszName, WORD wType, DWORD Options, PIP4_ARRAY aipServers, PDNS_RECORD* ppQueryResults, PVOID* pReserved);
		VOID (WINAPI *pfnDnsRecordListFree)(PDNS_RECORD pRecordList, DNS_FREE_TYPE FreeType);

		(FARPROC&)pfnDnsQueryConfig = GetProcAddress(hLib, "DnsQueryConfig");
		(FARPROC&)pfnDnsQuery = GetProcAddress(hLib, _TWINAPI("DnsQuery_"));
		(FARPROC&)pfnDnsRecordListFree = GetProcAddress(hLib, "DnsRecordListFree");

		if (pfnDnsQueryConfig && pfnDnsQuery && pfnDnsRecordListFree)
		{
			// WinXP: We explicitly need to pass the DNS servers to be used to DnsQuery, 
			// otherwise that function will never query a DNS server for the *local* host name.
			IP4_ARRAY* pDnsServers = NULL;
			BYTE aucBuff[16384];
			memset(aucBuff, 0, sizeof aucBuff);
			DWORD dwSize = sizeof aucBuff;
			DNS_STATUS nDnsState = (*pfnDnsQueryConfig)(DnsConfigDnsServerList, FALSE, NULL, NULL, aucBuff, &dwSize);
			if (nDnsState == 0)
			{
				if (dwSize >= 4)
				{
					DWORD dwDnsServers = ((DWORD*)aucBuff)[0];
					if (dwDnsServers >= 1 && dwSize == sizeof(DWORD) + dwDnsServers*sizeof(DWORD))
					{
						UINT uArrSize = sizeof(IP4_ARRAY) + sizeof(IP4_ADDRESS)*dwDnsServers;
						pDnsServers = (IP4_ARRAY*)new BYTE[uArrSize];
						memset(pDnsServers, 0, uArrSize);
						pDnsServers->AddrCount = dwDnsServers;
						for (UINT s = 0; s < dwDnsServers; s++)
							pDnsServers->AddrArray[s] = ((DWORD*)aucBuff)[1+s];
					}
				}
			}
			else{
				if (thePrefs.GetVerbose())
					theApp.QueueDebugLogLine(false, _T("ReverseDNS: Failed to get list of DNS servers - %s"), GetErrorMessage(nDnsState, 1));
			}

			CString strDnsQuery;
			strDnsQuery.Format(_T("%u.%u.%u.%u.IN-ADDR.ARPA"), (dwIP >> 24) & 0xFF, (dwIP >> 16) & 0xFF, (dwIP >> 8) & 0xFF, (dwIP >> 0) & 0xFF);

			// This is a *blocking* call!
			PDNS_RECORD pDnsRecords = NULL;
			nDnsState = (*pfnDnsQuery)(strDnsQuery, DNS_TYPE_PTR, DNS_QUERY_BYPASS_CACHE, pDnsServers, &pDnsRecords, NULL);
			if (nDnsState == 0)
			{
				if (pDnsRecords)
					strHostName = pDnsRecords->Data.PTR.pNameHost;
				if (pDnsRecords)
					(*pfnDnsRecordListFree)(pDnsRecords, DnsFreeRecordListDeep);
			}
			else{
				if (thePrefs.GetVerbose())
					theApp.QueueDebugLogLine(false, _T("ReverseDNS: Failed to resolve address \"%s\" - %s"), strDnsQuery, GetErrorMessage(nDnsState, 1));
			}

			delete[] (BYTE*)pDnsServers;
		}
		FreeLibrary(hLib);
	}

	return strHostName;
}

//jp detect webcache on startup
void AutodetectWebcache()
{
	WCInfo_Struct* detectedWebcache = new WCInfo_Struct();
	bool reaskedDNS;	// tells if a DNS reverse lookup has been performed during detection

	try
	{
		reaskedDNS=DetectWebCache(detectedWebcache, 2); // force using online DB
	}

	catch(CString strError)
	{
		delete detectedWebcache;
		if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false,_T("Error during automatic webcachedetection: ") + strError); // jp log
		return;
	}
	catch (...)
	{
		delete detectedWebcache;
		return;
	}
	
	if (detectedWebcache->active == _T("0"))
	{
		AddDebugLogLine(false, _T("Webcache Autodetection has detected that your ISP-proxy does not cache data. Webcache disabled."));
		thePrefs.webcacheEnabled = false;
		thePrefs.WebCacheDisabledThisSession = true;
		delete detectedWebcache;
		return;
	}
	else 
	{
		bool restart = false;
		if (thePrefs.webcacheName != detectedWebcache->webcache || thePrefs.webcachePort != _tstoi(detectedWebcache->port))
		{
			restart = true;
			thePrefs.WebCacheDisabledThisSession = true;
		}
	thePrefs.webcacheName=detectedWebcache->webcache;
	thePrefs.webcachePort=_tstoi(detectedWebcache->port);
	thePrefs.SetWebCacheBlockLimit(_tstoi(detectedWebcache->blockLimit));
		thePrefs.SetWebCacheExtraTimeout(detectedWebcache->extraTimeout == _T("1") ? true : false);
		thePrefs.SetWebCacheCachesLocalTraffic(detectedWebcache->cachesLocal == _T("1") ? true : false);
		thePrefs.PersistentConnectionsForProxyDownloads = (detectedWebcache->persistentconns == _T("1") ? true : false);	
		thePrefs.webcacheTrustLevel = _tstoi(detectedWebcache->trustLevel);
		if (thePrefs.webcacheEnabled && restart) //WC-ToDo need a modal dialogue here
			AddDebugLogLine( false, _T("Webcache autodetection detected a change in the Webcache-configuration, webcache has been deactivated until eMule is restarted.\n You can deactivate automatic webcache configuration in the Advanced Webcachesettings."));
		else if (!thePrefs.webcacheEnabled && restart)
		{
			CString comment = detectedWebcache->comment;
			for (int i=1; i*45 < comment.GetLength(); i++) // some quick-n-dirty beautifying  
				comment = comment.Left(i*45) + _T(" \n\t\t\t") + comment.Right(comment.GetLength() - i*45);
			
			CString message =	_T("Your ISP is:\t\t") + detectedWebcache->isp + _T(", ") + detectedWebcache->country + _T(", ") + detectedWebcache->location + _T("\n") +
								_T("Your proxy name is:\t") + detectedWebcache->webcache + _T("\n") +
								_T("The proxy port is:\t\t") + detectedWebcache->port + _T("\n") +
								(comment != _T("") ? _T("comment: \t\t") + comment : _T("")) + _T("\n") +
								_T("You can activate WebCache in the Webcachesettings");
			AddDebugLogLine( false, message);
		}
		else
			AddDebugLogLine( false, _T("Webcache Autodetection detected no changes!"));
	delete detectedWebcache;
	return;
	}
}
// jp detect webcache on startup END

// Superlexx - XML-based proxy autodetector - start ////////////////////////////////////////////
bool DetectWebCache(WCInfo_Struct* detectedWebcache, uint8 attempt)
{
	using namespace MSXML2;
	if (!theApp.GetPublicIP() && !Kademlia::CKademlia::getIPAddress())
		throw CString(_T("No public IP, please connect to an ed2k-server or Kad.\nYour TCP port must be reachable (highID) if you are connected to a server,\nbut it's not necessary when connected to Kad"));

// do reverse lookup
	CString shostName;
	bool reaskedDNS = false;
	uint32 PublicIP = theApp.GetPublicIP() ? theApp.GetPublicIP() : ntohl(Kademlia::CKademlia::getIPAddress());
	bool lastLookupFailed = thePrefs.GetLastResolvedName()==_T("");
	bool IPchangedSinceLastLookup = (PublicIP != thePrefs.GetWebCacheLastGlobalIP());
	bool enoughTimePassedSinceLastLookup = (GetTickCount() - thePrefs.GetWebCacheLastSearch() > (uint32)WCREASKMS);

	if (enoughTimePassedSinceLastLookup || IPchangedSinceLastLookup)
	{
		shostName = ReverseDnsLookupForWebCache(PublicIP);
		reaskedDNS = true;
		thePrefs.SetWebCacheLastSearch(GetTickCount());
		thePrefs.SetWebCacheLastGlobalIP(PublicIP);
		thePrefs.SetLastResolvedName(shostName);
	}
	else if (!lastLookupFailed)
		shostName = thePrefs.GetLastResolvedName();	// we had success last time, maybe our proxy is in the database now
	else
	{
		throw CString(_T("Not starting a detection:\nlast failed DNS reverse lookup attempt is too near,\nIP has not changed"));
	}
	if (shostName==_T(""))
	{
		throw CString(_T("Reverse DNS lookup failed"));
	}

// see if we can detect the webcache

	bool webcacheFound = false;
	MSXML2::IXMLDOMDocumentPtr pXMLDom;
	HRESULT hr;

	CoInitialize(NULL);

	hr = pXMLDom.CreateInstance(__uuidof(DOMDocument30));
	if (FAILED(hr)) 
		throw printf("Failed to instantiate DOMDocument30 class.");

	pXMLDom->async = VARIANT_FALSE;
	switch (attempt)
	{
	case 1:	// first attempt, load data from a local file
		if (pXMLDom->load(thePrefs.GetConfigDir() + "webcaches.xml") != VARIANT_TRUE)
		{
			DetectWebCache(detectedWebcache, 2); // make a second attempt, load data from the website
			return reaskedDNS;
		}
		break;
	case 2:
	{
			if ( pXMLDom->load("http://webcache-emule.sourceforge.net/webcacheURL.php") != VARIANT_TRUE)
				throw("Your ISP was not found in the local database; loading the URL of xml data from the SF website failed.");
			MSXML2::IXMLDOMNodePtr configURLNode = pXMLDom->selectSingleNode("/webcacheemule/configURL");
			int result = pXMLDom->load((_bstr_t)(configURLNode->text + _T("?hostName=") + shostName));
			configURLNode.Release();
			if (result != VARIANT_TRUE)
				throw("Failed loading xml data from the webcache-emule website.");			
		}
		break;
	default:
		throw (_T("invalid argument"));
	}
	
	MSXML2::IXMLDOMNodeListPtr proxies, proxyParms;
		
	proxies = pXMLDom->selectNodes("/webcacheemule/proxies");
	int imax = proxies->length;

	CString query, pattern, nodeName;
	CAtlRegExp<> reHostName;

// maybe the string casting is a bit too optimistic, but it works

	for (int i = 0; i < imax; i++)
		{
		// Query a node-set.
		query.Format(_T("/webcacheemule/proxies[%i]/*"), i);
		proxyParms = pXMLDom->selectNodes((_bstr_t)query);

		pattern = (CString)(char*)proxyParms->item[0]->text;
		// transform the wildcards to patterns
		pattern.Replace(_T("."), _T("\\."));
		pattern.Replace(_T("?"), _T("\\a?"));
		pattern.Replace(_T("*"), _T("\\a+"));

		REParseError status = reHostName.Parse(pattern);
		if (REPARSE_ERROR_OK != status)
			throw (_T("bad host name"));
			
		CAtlREMatchContext<> mcHostName;

		if (reHostName.Match(shostName, &mcHostName))
		{
			webcacheFound = true;
			// set default values
			detectedWebcache->location = _T("");
			detectedWebcache->active = _T("1");
			detectedWebcache->persistentconns = _T("0");
			detectedWebcache->cachesLocal = _T("1");
			detectedWebcache->extraTimeout = _T("0");
			detectedWebcache->blockLimit = _T("0");
			detectedWebcache->trustLevel = _T("30");


			for (int ii = 0; ii<proxyParms->length; ii++)
			{
				nodeName = (CString)(char*)proxyParms->item[ii]->nodeName;
				if (nodeName == "proxyName")
					detectedWebcache->webcache = (CString)(char*)proxyParms->item[ii]->text;
				else if (nodeName == "proxyPort")
					detectedWebcache->port = (CString)(char*)proxyParms->item[ii]->text;
				else if (nodeName == "ISPName")
					detectedWebcache->isp = (CString)(char*)proxyParms->item[ii]->text;
				else if (nodeName == "country")
					detectedWebcache->country = (CString)(char*)proxyParms->item[ii]->text;
				else if (nodeName == "location")
					detectedWebcache->location = (CString)(char*)proxyParms->item[ii]->text;
				else if (nodeName == "blockLimit")
					detectedWebcache->blockLimit = (CString)(char*)proxyParms->item[ii]->text;
				else if (nodeName == "extraTimeout")
					detectedWebcache->extraTimeout = (CString)(char*)proxyParms->item[ii]->text;
				else if (nodeName == "cachesLocal")
					detectedWebcache->cachesLocal = (CString)(char*)proxyParms->item[ii]->text;
				else if (nodeName == "persistentConn")
					detectedWebcache->persistentconns = (CString)(char*)proxyParms->item[ii]->text;
				else if (nodeName == "comment")
					detectedWebcache->comment = (CString)(char*)proxyParms->item[ii]->text;
				else if (nodeName == "active")
					detectedWebcache->active = (CString)(char*)proxyParms->item[ii]->text;
				else if (nodeName == "trustLevel")
					detectedWebcache->trustLevel = (CString)(char*)proxyParms->item[ii]->text;

			}
		}
	}

	pXMLDom.Release();
	proxies.Release();
	proxyParms.Release();
	CoUninitialize();

	if (!webcacheFound)
		switch (attempt)
	{
		case 1:
			DetectWebCache(detectedWebcache, 2);
			break;
		case 2:
	{
		CString message;
		message.Format(_T("Sorry, your ISP is not in the database,\nyour ISP identifier is %s\n"), shostName);
		message += _T("To enable autodetection, please find out and submit your ISPs proxy.");
		throw message;
	}
		default:
			throw (_T("invalid argument"));
	}

	return reaskedDNS;
}
// Superlexx - XML-based proxy autodetector - end //////////////////////////////////////////////

// Superlexx - en/decryption - start ///////////////////////////////////////////////////////////
void GenerateKey (byte* key)
{
	using namespace CryptoPP;
	AutoSeededRandomPool rng;
	rng.GenerateBlock(key, WC_KEYLENGTH);
}

// Superlexx - en/decryption - end /////////////////////////////////////////////////////////////


// URL-compatible Base64 en/decoder - start ////////////////////////////////////////////////////

BOOL WC_b64_Encode( const byte* source, int len, CStringA& destination )
{
	int dest_len = ATL::Base64EncodeGetRequiredLength( len, ATL_BASE64_FLAG_NOCRLF | ATL_BASE64_FLAG_NOPAD );
	char* dest_buf = destination.GetBufferSetLength( dest_len );
	if( !ATL::Base64Encode( source, len, dest_buf, &dest_len, ATL_BASE64_FLAG_NOCRLF | ATL_BASE64_FLAG_NOPAD ) ) {
		destination.ReleaseBuffer(0);
		return false;
	}
	destination.ReleaseBuffer( dest_len );
	ASSERT( dest_len == 22 );
	destination.Replace( '/', '_' );
	return true;
}

BOOL WC_b64_Decode( const CStringA& source, byte* destination, int dest_len )
{
	CStringA tmp = source;
	tmp.Replace( '_', '/' );
	return ATL::Base64Decode( tmp, tmp.GetLength(), destination, &dest_len );
}

// URL-compatible Base64 en/decoder - end //////////////////////////////////////////////////////

//JP proxy configuration test START
bool PingviaProxy(CString WebCacheName, uint16 WebCachePort)
{
	USES_CONVERSION;

	//create testclient
	if (ProxyTestClient)
		delete ProxyTestClient;
	ProxyTestClient = new CUpDownClient();

	//Resolve webcache information
	uint32 destIp;		// transparent-proxy ? our ip : proxy ip
	uint16 destPort;	// transparent-proxy ? 80 : proxy port
	 	
 	if( thePrefs.WebCacheIsTransparent() ) { // transparent proxy
 		destIp = theApp.GetPublicIP();
 		destPort = 80;
 	} else { // regular proxy
 		hostent* remoteHost = gethostbyname(CT2CA(WebCacheName)); //Resolve webcache name
 		if( !remoteHost ) // if failed to resolve
 			return false;
 		destIp = *(reinterpret_cast<uint32*>(remoteHost->h_addr_list[0]));
 		destPort = WebCachePort;
 	}
	
	//create DownSocket
	ProxyTestClient->m_pWCDownSocket = new CWebCacheDownSocket(ProxyTestClient);
	ProxyTestClient->m_pWCDownSocket->SetTimeOut(GetWebCacheSocketDownloadTimeout());
	if (!ProxyTestClient->m_pWCDownSocket->Create()){
		ProxyTestClient->m_pWCDownSocket->Safe_Delete();
		ProxyTestClient->m_pWCDownSocket = 0;
		return false;
	}
		
	//connect to Proxy
		SOCKADDR_IN sockAddr = {0};
		sockAddr.sin_family = AF_INET;
	sockAddr.sin_port = htons( destPort );
	sockAddr.sin_addr.S_un.S_addr = destIp; 
		ProxyTestClient->m_pWCDownSocket->WaitForOnConnect(); // Superlexx - from 0.44a PC code
		ProxyTestClient->m_pWCDownSocket->Connect((SOCKADDR*)&sockAddr, sizeof sockAddr);

	//send ping get
	CStringA strWCPingRequest;
	strWCPingRequest.AppendFormat("GET http://%s:%u/encryptedData/WebCachePing.htm HTTP/1.1\r\n",
		ipstrA( theApp.GetPublicIP() ), // our own IP
		thePrefs.port);	// our port
	strWCPingRequest.AppendFormat("Host: %s:%u\r\n", ipstrA( theApp.GetPublicIP() ), thePrefs.port ); // our IP and port
	strWCPingRequest.AppendFormat("Cache-Control: max-age=0\r\n" ); // do NOT DL this from the proxy! (timeout issue)
	strWCPingRequest.AppendFormat("Connection: close\r\nProxy-Connection: close\r\n" ); //only needed for 1 transmission
	//MORPH START - Changed by SiRoB, ModID
	/*
	strWCPingRequest.AppendFormat("User-Agent: eMule/%s %s\r\n", T2CA(theApp.m_strCurVersionLong), T2CA(MOD_VERSION));
	*/
	strWCPingRequest.AppendFormat("User-Agent: eMule/%s %s\r\n", T2CA(theApp.m_strCurVersionLong), T2CA(theApp.m_strModVersion));
	//MORPH END   - Changed by SiRoB, ModID
	strWCPingRequest.AppendFormat("\r\n");
		
	CRawPacket* pHttpPacket = new CRawPacket(strWCPingRequest);
	theStats.AddUpDataOverheadFileRequest(pHttpPacket->size);
	ProxyTestClient->m_pWCDownSocket->SendPacket(pHttpPacket);
	return true;
}
//JP proxy configuration test END