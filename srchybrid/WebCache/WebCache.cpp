#include "stdafx.h"
#include <share.h>
#include "WebCache.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "WebCachedBlock.h" // yonatan http
#include "eMule.h"
#include "WebCachedBlockList.h"
#include <windns.h>
#include <crypto51/modes.h>
#include <crypto51/aes.h>
#include <crypto51/osrng.h>
#include "opcodes.h"
#include "kademlia/kademlia/Kademlia.h"
#include "RichEditCtrlX.h"
#include <atlenc.h>
#include "log.h"
#include "WebCacheSocket.h"//JP proxy configuration test
#include "Packets.h"//JP proxy configuration test
#include "Statistics.h" //JP proxy configuration test

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
void detectWebcacheOnStart()
{
	WCInfo_Struct* detectedWebcache = new WCInfo_Struct();

	bool reaskedDNS;	// tells if a DNS backward lookup has been performed during detection

	try
	{
		reaskedDNS=DetectWebCache(detectedWebcache);
	}
	catch(CString strError)
	{
		delete detectedWebcache;
		if (thePrefs.GetLogWebCacheEvents())
		AddDebugLogLine( false,_T("Error during webcachedetection on first start: ") + strError); // jp log
		if	((strError != _T("parsing webcache database failed")) &&
			(strError != _T("Not starting a detection:\nlast failed DNS reverse lookup attempt is too near,\nIP has not changed")))
		// AfxMessageBox(strError ,MB_OK | MB_ICONINFORMATION,0); //jp no messagebox for detect on startup
		return;
	}
	catch (...)
	{
		delete detectedWebcache;
		// AfxMessageBox(_T("Autodetection failed") ,MB_OK | MB_ICONINFORMATION,0); //jp no messagebox for detect on startup
		return;
	}
	
	if (AfxMessageBox((_T("Webcache detected, do you want to activate the webcache feature? \n\nYour ISP is:\t\t") + detectedWebcache->isp + _T("\n") +
		_T("Your proxy name is:\t") + detectedWebcache->webcache + _T("\n") +
		_T("The proxy port is:\t\t") + detectedWebcache->port + _T("\n\n") +
		_T("The block limit is:\t\t") + detectedWebcache->blockLimit + _T("\n") +
		_T("extra timeout needed:\t") + detectedWebcache->extraTimeout + _T("\n") +
		_T("caches local traffic:\t\t") + detectedWebcache->cachesLocal + _T("\n") +
		_T("Use persistent connections:\t") + detectedWebcache->persistentconns + _T("\n\n") +
		_T("reverse DNS lookup performed:\t") + (reaskedDNS?_T("yes"):_T("no")) +
		_T("\n\n") +
		_T("if you select NO your current settings will not be changed. You change them later in preferences-webcachesettings")
		),MB_YESNO | MB_ICONINFORMATION,0) == IDNO)
	{
		delete detectedWebcache;
		return;
	}

	else 
	{
	thePrefs.webcacheEnabled = true;
	thePrefs.webcacheName=detectedWebcache->webcache;
	thePrefs.webcachePort=_tstoi(detectedWebcache->port);
	thePrefs.SetWebCacheBlockLimit(_tstoi(detectedWebcache->blockLimit));
	thePrefs.SetWebCacheExtraTimeout(detectedWebcache->extraTimeout == _T("yes") ? true : false);
	thePrefs.SetWebCacheCachesLocalTraffic(detectedWebcache->cachesLocal == _T("yes") ? true : false);
	thePrefs.PersistentConnectionsForProxyDownloads = (detectedWebcache->persistentconns == _T("yes") ? true : false);	
	delete detectedWebcache;
	AfxMessageBox(GetResString(IDS_SETTINGCHANGED_RESTART));
	}
}
// jp detect webcache on startup END

bool DetectWebCache(WCInfo_Struct* detectedWebcache)	// find the webcache based on the local data base
{
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
		throw CString(_T("Not starting a detection:\nlast failed DNS backward lookup attempt is too near,\nIP has not changed"));
	}
	if (shostName==_T(""))
	{
		throw CString(_T("Reverse DNS lookup failed"));
	}

// see if we can detect the webcache

// open the webcaches.csv

	FILE* readFile = _tfsopen(thePrefs.GetConfigDir() + _T("webcaches.csv"), _T("r"), _SH_DENYWR);
	if (readFile == NULL)
		throw CString(_T("webcaches.csv not found"));

// try to find a matching record

	bool webcacheFound = false;
	char buffer[1024];
	int	lenBuf = 1024;
	int pos = 0;
	CString sbuffer, identifier, webcache, port, isp, blockLimit, extraTimeout, cachesLocal, persistentconns;
	int lineNumber = 0;
	while (!feof(readFile) && !webcacheFound)
	{
		if (fgets(buffer,lenBuf,readFile)==0) break;
		lineNumber++;
		sbuffer = buffer;
		
		// remove comments
		pos = sbuffer.Find(_T("//"));
		if (pos>=0)
			sbuffer.Truncate(pos);

		pos = 0;
		sbuffer.Trim(_T("\r\n"));

		CString toTrim = _T("\t ");

		if (!sbuffer.IsEmpty())
		{
			identifier = sbuffer.Tokenize(_T("\t"),pos);	if (pos <= 0) goto ParseError;		identifier.Trim(toTrim);
			webcache = sbuffer.Tokenize(_T(":"),pos);		if (pos <= 0) goto ParseError;		webcache.Trim(toTrim);
			port = sbuffer.Tokenize(_T("\t"),pos);			if (pos <= 0) goto ParseError;		port.Trim(toTrim);
			isp = sbuffer.Tokenize(_T("\t"),pos);			if (pos <= 0) goto ParseError;		isp.Trim(toTrim);
			blockLimit = sbuffer.Tokenize(_T("\t"),pos);	if (pos <= 0) goto ParseError;		blockLimit.Trim(toTrim);
			extraTimeout = sbuffer.Tokenize(_T("\t"),pos);	if (pos <= 0) goto ParseError;		extraTimeout.Trim(toTrim);
			cachesLocal = sbuffer.Tokenize(_T("\t"),pos);	if (pos <= 0) goto ParseError;		cachesLocal.Trim(toTrim);
			persistentconns = sbuffer.Tokenize(_T("\t"),pos);									persistentconns.Trim(toTrim);
			
			if( !(persistentconns == _T("yes") || persistentconns == _T("no"))
				|| !(cachesLocal == _T("yes") || cachesLocal == _T("no"))
				|| !(extraTimeout == _T("yes") || extraTimeout == _T("no")) )
				goto ParseError;

			if (shostName.Right(identifier.GetLength()) == identifier)
			{
				detectedWebcache->webcache = webcache;
				detectedWebcache->port = port;
				detectedWebcache->isp = isp;
				detectedWebcache->blockLimit = blockLimit;
				detectedWebcache->extraTimeout = extraTimeout;
				detectedWebcache->cachesLocal = cachesLocal;
				detectedWebcache->persistentconns = persistentconns;
				webcacheFound = true;
			}
		}
	}

// close the webcaches.csv
	fclose(readFile);

	if (!webcacheFound)
	{
		CString message;
		message.Format(_T("Sorry, your ISP is not in the database,\nyour ISP identifier is %s\n"), shostName);
		message += _T("To enable autodetection, please find out and submit your ISPs proxy.");
		throw message;
	}
	return reaskedDNS;

ParseError:
	CString strParseError = _T("parsing webcache database failed");
	strParseError.AppendFormat( _T(" - line %d:\n%s"), lineNumber, sbuffer );
	fclose( readFile );
	throw strParseError;
	return reaskedDNS;
}

// Superlexx - Proxy AutoDetect - end //////////////////////////////////////////////////////////

// Superlexx - en/decryption - start ///////////////////////////////////////////////////////////
bool AESEncrypt(byte* data, uint32 size, byte* key)
{
	using namespace CryptoPP;
	byte iv[AES::BLOCKSIZE];
	for (int i=0; i<AES::BLOCKSIZE; i++)
		iv[i] = 0;

	CFB_Mode<AES >::Encryption cfbEncryption(key, WC_KEYLENGTH, iv);

	byte enc_data[EMBLOCKSIZE]; // used for temporary storing of the ciphertext

	//data[0] = 13;
	// encrypt
	cfbEncryption.ProcessData(enc_data, data, size);

	for (uint32 i=0; i<size; i++) // data := enc_data // better use memcpy here?
		data[i] = enc_data[i];

	return true;
}

bool AESDecrypt(byte* data, uint32 size, byte* key)
{
	using namespace CryptoPP;
	byte iv[AES::BLOCKSIZE];
	for (int i=0; i<AES::BLOCKSIZE; i++)
		iv[i] = 0;
	byte dec_data[EMBLOCKSIZE];

	CFB_Mode<AES >::Decryption cfbDecryption(key, WC_KEYLENGTH, iv);
	cfbDecryption.ProcessData(dec_data, data, size);

	for (uint32 i=0; i<size; i++) // data := dec_data // better use memcpy here?
		data[i] = dec_data[i];

	return true;
}

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