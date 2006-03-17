//this file is part of eMule
//Copyright (C)2004 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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
#include <wininet.h>
#include "emule.h"
#include "UrlClient.h"
#include "PartFile.h"
#include "SafeFile.h"
#include "Statistics.h"
#include "Packets.h"
#include "ListenSocket.h"
#include "Preferences.h"
#include "OtherFunctions.h"
#include "SharedFileList.h"
#include "WebCacheSocket.h"
#include "UploadBandwidthThrottler.h"
#include "UploadQueue.h"
#include "Log.h"
// yonatan http start //////////////////////////////////////////////////////////////////////////
#include "ClientList.h"
#include "WebCache.h"
#include "WebCacheProxyClient.h"
#include "WebCachedBlockList.h"
#include "ClientUDPSocket.h"
#include "WebCacheOHCBManager.h"
// yonatan http end ////////////////////////////////////////////////////////////////////////////
//#include "ws2tcpip.h" // Superlexx - Proxy AutoDetect
#include "DownloadQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

#define HTTP_STATUS_INV_RANGE	416

UINT GetWebCacheSocketUploadTimeout()
{
	if (thePrefs.GetWebCacheExtraTimeout())
		return DOWNLOADTIMEOUT + SEC2MS(100 + WC_SOCKETEXTRATIMEOUT);	// must be lower than Upload timeout?
	else
		return DOWNLOADTIMEOUT + SEC2MS(100 + 30);
}

UINT GetWebCacheSocketDownloadTimeout()
{
	if (thePrefs.GetWebCacheExtraTimeout())
		return DOWNLOADTIMEOUT + SEC2MS(100 + WC_SOCKETEXTRATIMEOUT);	// must be lower than Upload timeout?
	else
		return DOWNLOADTIMEOUT + SEC2MS(100);	// must be lower than Upload timeout?
}


///////////////////////////////////////////////////////////////////////////////
// CWebCacheSocket

IMPLEMENT_DYNCREATE(CWebCacheSocket, CHttpClientReqSocket)

CWebCacheSocket::CWebCacheSocket(CUpDownClient* pClient)
{
	ASSERT( client == NULL );
	client = NULL;
	m_client = pClient;
// WebCache ////////////////////////////////////////////////////////////////////////////////////
	m_bReceivedHttpClose = false; // 'Connection: close' detector
}

CWebCacheSocket::~CWebCacheSocket()
{
	DetachFromClient();
}

void CWebCacheSocket::DetachFromClient()
{
	if (GetClient())
	{
		// faile safe, should never be needed
		if (GetClient()->m_pWCDownSocket == this){
			ASSERT(0);
			GetClient()->m_pWCDownSocket = NULL;
			GetClient()->SetWebCacheDownState( WCDS_NONE );
		}
		if (GetClient()->m_pWCUpSocket == this){
			ASSERT(0);
			GetClient()->m_pWCUpSocket = NULL;
			theApp.uploadBandwidthThrottler->RemoveFromStandardList(this); //MORPH - Added by SiRoB, WebCache Fix
			GetClient()->SetWebCacheUpState( WCUS_NONE );
		}
	}
}

void CWebCacheSocket::Safe_Delete()
{
	DetachFromClient();
	CClientReqSocket::Safe_Delete();
	m_client = NULL;
	ASSERT( GetClient() == NULL );
	ASSERT( client == NULL );
}

void CWebCacheSocket::OnSend(int nErrorCode)
{
//	Debug("%08x %hs\n", this, __FUNCTION__);
	// PC-TODO: We have to keep the ed2k connection of a client as long active as we are using
	// the associated WebCache connection -> Update the timeout of the ed2k socket.
	if (nErrorCode == 0 && GetClient() && GetClient()->socket)
		GetClient()->socket->ResetTimeOutTimer();
	CHttpClientReqSocket::OnSend(nErrorCode);
}

void CWebCacheSocket::OnReceive(int nErrorCode)
{
//	Debug("%08x %hs\n", this, __FUNCTION__);
	// PC-TODO: We have to keep the ed2k connection of a client as long active as we are using
	// the associated WebCache connection -> Update the timeout of the ed2k socket.
	if (nErrorCode == 0 && GetClient() && GetClient()->socket)
		GetClient()->socket->ResetTimeOutTimer();
	CHttpClientReqSocket::OnReceive(nErrorCode);
}

void CWebCacheSocket::OnError(int nErrorCode)
{
	Debug(_T("%08x %hs\n"), this, __FUNCTION__);
	CHttpClientReqSocket::OnError(nErrorCode);
}

bool CWebCacheSocket::ProcessHttpResponse()
{
	ASSERT(0);
	return false;
}

bool CWebCacheSocket::ProcessHttpResponseBody(const BYTE* /*pucData*/, UINT /*size*/)
{
	ASSERT(0);
	return false;
}

bool CWebCacheSocket::ProcessHttpRequest()
{
	ASSERT(0);
	return false;
}

// yonatan http start //////////////////////////////////////////////////////////////////////////
void CWebCacheSocket::ResetTimeOutTimer()
{
	timeout_timer = ::GetTickCount();
	if( GetClient() ) {
		if( GetClient()->socket ) {
			ASSERT( !GetClient()->socket->IsKindOf( RUNTIME_CLASS( CWebCacheSocket ) ) );
			GetClient()->socket->ResetTimeOutTimer();
		}
	}
}
// yonatan http end ////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// CWebCacheDownSocket

IMPLEMENT_DYNCREATE(CWebCacheDownSocket, CWebCacheSocket)

CWebCacheDownSocket::CWebCacheDownSocket(CUpDownClient* pClient)
	: CWebCacheSocket(pClient)
{
	ProxyConnectionCount++; // yonatan http
	m_bProxyConnCountFlag = false; //jp correct downsocket count
	blocksloaded = 0; //count blocksloaded for each proxy
	if (thePrefs.GetLogWebCacheEvents())
	AddDebugLogLine( false, _T("new CWebCacheDownSocket: ProxyConnectionCount=%u"), ProxyConnectionCount );
	Debug(_T("%08x %hs\n"), this, __FUNCTION__);
}

CWebCacheDownSocket::~CWebCacheDownSocket()
{
	if (m_bProxyConnCountFlag == false)
	{
		m_bProxyConnCountFlag = true;
		ProxyConnectionCount--; // yonatan http
		if (thePrefs.GetLogWebCacheEvents())
		AddDebugLogLine( false, _T("deleted CWebCacheDownSocket: ProxyConnectionCount=%u"), ProxyConnectionCount );
	}
	ASSERT( ProxyConnectionCount != -1 ); // yonatan http
	Debug(_T("%08x %hs\n"), this, __FUNCTION__);
	DetachFromClient();
}

// decreases ProxyConnectionCount and calls CWebCacheSocket::Safe_Delete()
void CWebCacheDownSocket::Safe_Delete()
{
	if (m_bProxyConnCountFlag == false)
	{
		m_bProxyConnCountFlag = true;
		ProxyConnectionCount--;
		if (thePrefs.GetLogWebCacheEvents())
		AddDebugLogLine( false, _T("deleted CWebCacheDownSocket: ProxyConnectionCount=%u"), ProxyConnectionCount );
	}
	ASSERT( ProxyConnectionCount != -1 );
	Debug(_T("%08x %hs\n"), this, __FUNCTION__);
	CWebCacheSocket::Safe_Delete();
}

void CWebCacheDownSocket::OnConnect(int nErrorCode)
{
ASSERT( GetClient() );
if (0 != nErrorCode)
{
	if( GetClient() ) // just in case
		if (GetClient()->IsProxy())
		{
			if (thePrefs.GetLogWebCacheEvents())
				AddDebugLogLine(false, _T("Connection to proxy failed. Trying next block"));
			if (SINGLEProxyClient->ProxyClientIsBusy())
				SINGLEProxyClient->DeleteBlock();
			WebCachedBlockList.TryToDL();
		}
		//MORPH STATR - Added by SiRoB, WebCache New Resolvename
		else {
			SOCKADDR_IN sockAddr = {0};
			int nSockAddrLen = sizeof(sockAddr);
			GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
			m_dwIPWC = sockAddr.sin_addr.S_un.S_addr;
		}
		//MORPH END   - Added by SiRoB, WebCache New Resolvename
}

CHttpClientReqSocket::OnConnect(nErrorCode);
}


void CWebCacheDownSocket::DetachFromClient()
{
	if (GetClient())
	{
		if (GetClient()->m_pWCDownSocket == this)
			GetClient()->m_pWCDownSocket = NULL;
	}
}

void CWebCacheDownSocket::OnClose(int nErrorCode)
{
	Debug(_T("%08x %hs\n"), this, __FUNCTION__);

	DisableDownloadLimit(); // receive pending data
	CUpDownClient* pCurClient = GetClient();
	if (pCurClient && pCurClient->m_pWCDownSocket != this)
		pCurClient = NULL;

	CWebCacheSocket::OnClose(nErrorCode);

	if (pCurClient)
	{
		ASSERT( pCurClient->m_pWCDownSocket == NULL );

		// this callback is only invoked if that closed socket was(!) currently attached to the client
		pCurClient->OnWebCacheDownSocketClosed(nErrorCode);
	}
}

bool CWebCacheDownSocket::ProcessHttpResponse()
{
	if (GetClient() == NULL)
		throw CString(__FUNCTION__ " - No client attached to HTTP socket");

	if (!GetClient()->ProcessWebCacheDownHttpResponse(m_astrHttpHeaders))
		return false;

	return true;
}

bool CWebCacheDownSocket::ProcessHttpResponseBody(const BYTE* pucData, UINT uSize)
{
	if (GetClient() == NULL)
		throw CString(__FUNCTION__ " - No client attached to HTTP socket");

	GetClient()->ProcessWebCacheDownHttpResponseBody(pucData, uSize);

	return true;
}

bool CWebCacheDownSocket::ProcessHttpRequest()
{
	throw CString(_T("Unexpected HTTP request received"));
}


///////////////////////////////////////////////////////////////////////////////
// CWebCacheUpSocket

IMPLEMENT_DYNCREATE(CWebCacheUpSocket, CWebCacheSocket)

CWebCacheUpSocket::CWebCacheUpSocket(CUpDownClient* pClient)
	: CWebCacheSocket(pClient)
{
	Debug(_T("%08x %hs\n"), this, __FUNCTION__);
}

CWebCacheUpSocket::~CWebCacheUpSocket()
{
	Debug(_T("%08x %hs\n"), this, __FUNCTION__);
	theApp.uploadBandwidthThrottler->RemoveFromAllQueues(this); // Superlexx - from 0.44a PC code
	DetachFromClient();
}

void CWebCacheUpSocket::DetachFromClient()
{
	if (GetClient())
	{
		if (GetClient()->m_pWCUpSocket == this) {
			GetClient()->m_pWCUpSocket = NULL;
			theApp.uploadBandwidthThrottler->RemoveFromStandardList(this); // Superlexx - from 0.44a PC code
			GetClient()->SetWebCacheUpState( WCUS_NONE );
		}
	}
}

void CWebCacheUpSocket::OnSend(int nErrorCode)
{
	Debug(_T("%08x %hs\n"), this, __FUNCTION__);
	CWebCacheSocket::OnSend(nErrorCode);
}

void CWebCacheUpSocket::OnClose(int nErrorCode)
{
	Debug(_T("%08x %hs\n"), this, __FUNCTION__);
	CWebCacheSocket::OnClose(nErrorCode);
	if (GetClient())
	{
		if (GetClient()->m_pWCUpSocket == this)
		{
			DetachFromClient();

			// this callback is only invoked if that closed socket was(!) currently attached to the client
			//GetClient()->OnWebCacheUpSocketClosed(nErrorCode);
		}
	}
}

bool CWebCacheUpSocket::ProcessHttpResponse()
{
	if (GetClient() == NULL)
		throw CString(__FUNCTION__ " - No client attached to HTTP socket");

	if (!GetClient()->ProcessWebCacheUpHttpResponse(m_astrHttpHeaders))
		return false;

	return true;
}

bool CWebCacheUpSocket::ProcessHttpResponseBody(const BYTE* /*pucData*/, UINT /*uSize*/)
{
	throw CString(_T("Unexpected HTTP body in response received"));
}

bool CWebCacheUpSocket::ProcessHttpRequest()
{
	if (GetClient() == NULL)
		throw CString(__FUNCTION__ " - No client attached to HTTP socket");

	// DFA
	// yonatan http - extract client from header
	uint32 id = 0;
	for (int i = 0; i < m_astrHttpHeaders.GetCount(); i++)
	{
		const CStringA& rstrHdr = m_astrHttpHeaders.GetAt(i);
		if (_strnicmp(rstrHdr, "Pragma: IDs=", 12) == 0)
		{
			try
			{
				int posStart = 0;
				rstrHdr.Tokenize("=", posStart);
				int posEnd = posStart;
				rstrHdr.Tokenize("|", posEnd);
				id = atoi(rstrHdr.Mid(posStart, posEnd-posStart));
			}
			catch(...)
			{
				DebugHttpHeaders(m_astrHttpHeaders);
				TRACE(_T("*** Unexpected HTTP %hs\n"), rstrHdr);
				return false;
			}
		}
	}

	CUpDownClient* m_client_new = theApp.uploadqueue->FindClientByWebCacheUploadId( id );

	if( !m_client_new ) {
		DebugHttpHeaders(m_astrHttpHeaders);
		TRACE(_T("*** Http GET from unknown client, webcache-id: %u"), id );
		return false;
	}
	if( !m_client_new->SupportsWebCache() ) {
		DebugHttpHeaders(m_astrHttpHeaders);
		TRACE(_T("*** Http GET from non-webcache client: %s"), client->DbgGetClientInfo() );
		return false;
	}
// DFA
/*	if( GetClient()->m_pWCUpSocket ) {
		ASSERT(GetClient()->m_pWCUpSocket != this);
		if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine(false, _T("*** Http GET on standard listensocket from client with established http socket: %s"), client->DbgGetClientInfo() );
		GetClient()->m_pWCUpSocket->Safe_Delete();
		GetClient()->m_pWCUpSocket = 0;
	}*/

	if (m_client != m_client_new) // HTTP request of another client arrived here
	{
		TRACE(_T("*** DFA: switching socket from ") + CString(m_client->GetUserName()) + _T(" to ") + CString(m_client_new->GetUserName()) + _T("\n"));
		// Xchange the sockets
		//CWebCacheUpSocket* tmp = m_client->m_pWCUpSocket;
		m_client->m_pWCUpSocket = m_client_new->m_pWCUpSocket;
		if (m_client->m_pWCUpSocket)
			m_client->m_pWCUpSocket->SetClient(m_client);
		//m_client_new->m_pWCUpSocket = this;
	}
	m_client = m_client_new; // set the (maybe new) owner in the class variable
	GetClient()->m_pWCUpSocket = this;

	UINT uHttpRes = GetClient()->ProcessWebCacheUpHttpRequest(m_astrHttpHeaders);

	if (uHttpRes != HTTP_STATUS_OK){
		CStringA strResponse;
		strResponse.AppendFormat("HTTP/1.1 %u\r\n", uHttpRes);
		strResponse.AppendFormat("Content-Length: 0\r\n");
		strResponse.AppendFormat("\r\n");

		if (thePrefs.GetDebugClientTCPLevel() > 0)
			Debug(_T("Sending WebCache HTTP response:\n%hs"), strResponse);
		CRawPacket* pHttpPacket = new CRawPacket(strResponse);
		theStats.AddUpDataOverheadFileRequest(pHttpPacket->size);
		SendPacket(pHttpPacket);
		SetHttpState(HttpStateUnknown);

		// PC-TODO: Problem, the packet which was queued for sending will not be sent, if we immediatly
		// close that socket. Currently I just let it timeout.
		//return false;
		SetTimeOut(SEC2MS(30));
		return true;
	}
	GetClient()->SetHttpSendState(0);

	SetHttpState(HttpStateRecvExpected);
	GetClient()->SetUploadState(US_UPLOADING);
	
	return true;
}

// attaches socket to client and calls ProcessHttpPacket
bool CWebCacheUpSocket::ProcessFirstHttpGet( const char* header, UINT uSize )
{
	// yonatan http - extract client from header
	LPBYTE pBody = NULL;
	int iSizeBody = 0;
	ClearHttpHeaders();
	ProcessHttpHeaderPacket(header, uSize, pBody, iSizeBody);
	uint32 id = 0; //WebCache Fix
	for (int i = 0; i < m_astrHttpHeaders.GetCount(); i++)
	{
		const CStringA& rstrHdr = m_astrHttpHeaders.GetAt(i);

		//JP proxy configuration test START
		if (_strnicmp(rstrHdr, "GET /encryptedData/WebCachePing.htm HTTP/1.1", 30) == 0)
		{
			if (thePrefs.expectingWebCachePing)
			{
				thePrefs.expectingWebCachePing = false;
				AfxMessageBox(_T("Proxy configuration Test Successfull"));
				AddLogLine(false, _T("Proxy configuration Test Successfull"));
			}
			else 
				if (thePrefs.GetLogWebCacheEvents())
					AddDebugLogLine(false, _T("WebCachePing received, but no test in progress"));
		return true; //everything that needs to be done with this packet has been done
		}
		//JP proxy configuration test END

		if (_strnicmp(rstrHdr, "Pragma: IDs=", 12) == 0)
		{
			//if (sscanf((LPCSTR)rstrHdr+22, _T("%u"), &id) != 1){
			try
			{
				int posStart = 0;
				rstrHdr.Tokenize("=", posStart);
				int posEnd = posStart;
				rstrHdr.Tokenize("|", posEnd);
				id = atoi(rstrHdr.Mid(posStart, posEnd-posStart));
			}
			catch(...)
			{
				DebugHttpHeaders(m_astrHttpHeaders);
				TRACE(_T("*** Unexpected HTTP %hs\n"), rstrHdr);
				return false;
			}
		}
	}

	m_client = theApp.uploadqueue->FindClientByWebCacheUploadId( id );

	if( !GetClient() ) {
		DebugHttpHeaders(m_astrHttpHeaders);
		TRACE(_T("*** Http GET from unknown client, webcache-id: %u"), id );
		return false;
	}
	if( !GetClient()->SupportsWebCache() ) {
		DebugHttpHeaders(m_astrHttpHeaders);
		TRACE(_T("*** Http GET from non-webcache client: %s"), m_client->DbgGetClientInfo() );
		return false;
	}

	if( GetClient()->m_pWCUpSocket ) {
		ASSERT(GetClient()->m_pWCUpSocket != this);
		if (thePrefs.GetLogWebCacheEvents())
		AddDebugLogLine(false, _T("*** Http GET on standard listensocket from client with established http socket: %s"), m_client->DbgGetClientInfo() );
		GetClient()->m_pWCUpSocket->Safe_Delete();
		GetClient()->m_pWCUpSocket = 0;
	}

	GetClient()->m_pWCUpSocket = this;
	SetTimeOut(GetWebCacheSocketUploadTimeout());  //MORPH - Added by SiRoB, WebCache Fix
	
	ClearHttpHeaders();
	SetHttpState( HttpStateRecvExpected );
	//theApp.uploadBandwidthThrottler->AddToStandardList(0, this); // Superlexx - from 0.44a PC code guess
	return( ProcessHttpPacket( (BYTE*)header, uSize ) );
}

///////////////////////////////////////////////////////////////////////////////
// WebCache client

bool CUpDownClient::ProcessWebCacheDownHttpResponse(const CStringAArray& astrHeaders)
{
// error might arrive after client-socket has timed out -	ASSERT( GetDownloadState() == DS_DOWNLOADING );
//	ASSERT( m_eWebCacheDownState == WCDS_WAIT_CACHE_REPLY || m_eWebCacheDownState == WCDS_WAIT_CLIENT_REPLY );

	if( m_eWebCacheDownState != WCDS_WAIT_CACHE_REPLY && m_eWebCacheDownState != WCDS_WAIT_CLIENT_REPLY )
		throw CString(_T("Failed to process HTTP response - Invalid client webcache download state"));

	if (reqfile == NULL)
		throw CString(_T("Failed to process HTTP response - No 'reqfile' attached"));
	if (GetDownloadState() != DS_DOWNLOADING && !IsProxy())
		throw CString(_T("Failed to process HTTP response - Invalid client download state"));
	if (astrHeaders.GetCount() == 0)
		throw CString(_T("Unexpected HTTP response - No headers available"));

///JP check for connection: close header START///
	for (int i = 1; i < astrHeaders.GetCount(); i++)
	{
		const CStringA& rstrHdr = astrHeaders.GetAt(i);
		if ( rstrHdr.Left( 11 ).CompareNoCase( "Connection:" ) == 0 )
		{
			int pos = 11;

			while( pos > 0 && pos < rstrHdr.GetLength() )
			{
				CStringA token = rstrHdr.Tokenize( ",", pos );
				token.Trim();
				if( token.CompareNoCase( "close" ) == 0 ) 
				{
					ASSERT( m_pWCDownSocket ); // yonatan tmp
					AddDebugLogLine( false, _T("Received a \"Connection: close\" header on a WCDownSocket") ); // yonatan tmp
					if( m_pWCDownSocket ) // just in case
						m_pWCDownSocket->m_bReceivedHttpClose = true;
					break;
				}
			}
		}
		if ( rstrHdr.Left( 17 ).CompareNoCase( "Proxy-Connection:" ) == 0 )
		{
			int pos = 17;

			while( pos > 0 && pos < rstrHdr.GetLength() )
			{
				CStringA token = rstrHdr.Tokenize( ",", pos );
				token.Trim();
				if( token.CompareNoCase( "close" ) == 0 ) 
				{
					ASSERT( m_pWCDownSocket ); // yonatan tmp
					AddDebugLogLine( false, _T("Received a \"Proxy-Connection: close\" header on a WCDownSocket") ); // yonatan tmp
					if( m_pWCDownSocket ) // just in case
						m_pWCDownSocket->m_bReceivedHttpClose = true;
					break;
				}
			}
		}
	}
//JP check for Connection: close header END///

	const CStringA& rstrHdr = astrHeaders.GetAt(0);
	UINT uHttpMajVer, uHttpMinVer, uHttpStatusCode;
	if (sscanf(rstrHdr, "HTTP/%u.%u %u", &uHttpMajVer, &uHttpMinVer, &uHttpStatusCode) != 3){
		CString strError;
		strError.Format(_T("Unexpected HTTP response: \"%hs\""), rstrHdr);
		throw strError;
	}
	if (uHttpMajVer != 1 || (uHttpMinVer != 0 && uHttpMinVer != 1)){
		CString strError;
		strError.Format(_T("Unexpected HTTP version: \"%hs\""), rstrHdr);
		throw strError;
	}
	bool bExpectData = uHttpStatusCode == HTTP_STATUS_OK; // || uHttpStatusCode == HTTP_STATUS_PARTIAL_CONTENT;
	if (!bExpectData){
		CString strError;
		strError.Format(_T("Unexpected HTTP status code \"%u\""), uHttpStatusCode);
		throw strError;
	}

	uint64 uContentLength = 0;
	for (int i = 1; i < astrHeaders.GetCount(); i++)
	{
		const CStringA& rstrHdr = astrHeaders.GetAt(i);
		if (bExpectData && rstrHdr.Left( 15 ).CompareNoCase( "Content-Length:" ) == 0)
		{
			uContentLength = _atoi64((LPCSTR)rstrHdr + 15);
			if (uContentLength > m_uReqEnd - m_uReqStart + 1){
				CString strError;
				strError.Format(_T("Unexpected HTTP header field \"%hs\""), rstrHdr);
				throw strError;
			}
		}
		else if ( rstrHdr.Left( 7 ).CompareNoCase( "Server:" ) == 0 )
		{
			if (IsProxy())
				m_strClientSoftware = rstrHdr.Mid(7).Trim();
		}
	}

	if (uContentLength != m_uReqEnd - m_uReqStart + 1){
		if (thePrefs.GetDebugClientTCPLevel() <= 0)
			DebugHttpHeaders(astrHeaders);
		CString strError;
		strError.Format(_T("Unexpected HTTP response - Content-Length mismatch"));
		throw strError;
	}

//	SetDownloadState(DS_DOWNLOADING);

	//PC-TODO: Where does this flag need to be cleared again? 
	// When client is allowed to send more block requests?
	// Also, we have to support both type of downloads within in the same connection.

	// WC-TODO: Find out when, if and where this is changed. Should this line be here?
	if (IsProxy())
		SetWebCacheDownState(WCDS_DOWNLOADINGFROM);
	else
		SetWebCacheDownState(WCDS_DOWNLOADINGVIA);

	return true;
}

bool CUpDownClient::ProcessWebCacheDownHttpResponseBody(const BYTE* pucData, UINT uSize)
{
	ProcessHttpBlockPacket(pucData, uSize);
	return true;
}

UINT CUpDownClient::ProcessWebCacheUpHttpRequest(const CStringAArray& astrHeaders)
{
// yonatan	ASSERT( m_eWebCacheUpState == WCUS_WAIT_CACHE_REPLY );

	if (astrHeaders.GetCount() == 0)
		return HTTP_STATUS_BAD_REQUEST;

	const CStringA& rstrHdr = astrHeaders.GetAt(0);
	char szUrl[1024];
	UINT uHttpMajVer, uHttpMinVer;
	if (sscanf(rstrHdr, "GET %1023s HTTP/%u.%u", szUrl, &uHttpMajVer, &uHttpMinVer) != 3){
		DebugHttpHeaders(astrHeaders);
		return HTTP_STATUS_BAD_REQUEST;
	}
	if (uHttpMajVer != 1 || (uHttpMinVer != 0 && uHttpMinVer != 1)){
		DebugHttpHeaders(astrHeaders);
		return HTTP_STATUS_BAD_REQUEST;
	}

    char b64_marc4_szFileHash[23]; // hm, why not 24? zero byte at the end
	uint64 ui64RangeStart = 0;
	uint64 ui64RangeEnd = 0;
// webcache url
	if (sscanf(szUrl, "/encryptedData/%I64u-%I64u/%22s", &ui64RangeStart, &ui64RangeEnd, b64_marc4_szFileHash ) != 3){ // Superlexx - encryption : shorter file hash due to base64 coding, new URL format
		DebugHttpHeaders(astrHeaders);
		return HTTP_STATUS_BAD_REQUEST;
	}

////// Superlexx - encryption - start ////////////////////////////////////////////////////////////////
	int numberOfHeaders = astrHeaders.GetSize();
	bool slaveKeyFound = false;
	CStringA buffer;

	for (int i=0; i<numberOfHeaders; i++)
	{
		buffer = astrHeaders.GetAt(i);
		if (buffer.Left(8) == "Pragma: ")		// pragma found
		{
			int slaveKeyPragmaPos = 0;
			buffer.Tokenize("|", slaveKeyPragmaPos);	// find the slaveKey-pragma
			CStringA b64_localSlaveKey = buffer.Mid(slaveKeyPragmaPos, WC_B64_KEYLENGTH);
			if (!WC_b64_Decode(b64_localSlaveKey, Crypt.localSlaveKey, WC_KEYLENGTH))	// base64 -> byte*
			{
				DebugHttpHeaders(astrHeaders);
				TRACE(_T("*** Bad slaveKey received in %s\n"), buffer);
				return HTTP_STATUS_BAD_REQUEST;
			}
			slaveKeyFound = true;
		}
//		else if ( buffer == "Connection: close" )
		else if ( buffer.Left( 11 ).CompareNoCase( "Connection:" ) == 0 )
		{
			int pos = 11;

			while( pos > 0 && pos < buffer.GetLength() ) {
				CStringA token = buffer.Tokenize( ",", pos );
				token.Trim();
				if( token.CompareNoCase( "close" ) == 0 ) {
					ASSERT( m_pWCUpSocket ); // yonatan tmp
					AddDebugLogLine( false, _T("Received a \"Connection: close\" header on a WCUpSocket") ); // yonatan tmp
					if( m_pWCUpSocket ) // just in case
						m_pWCUpSocket->m_bReceivedHttpClose = true; // WC-TODO: Safe_Delete socket after block xfer
					break;
				}
			}
		}
		else if ( buffer.Left( 17 ).CompareNoCase( "Proxy-Connection:" ) == 0 )
		{
			int pos = 17;

			while( pos > 0 && pos < buffer.GetLength() ) {
				CStringA token = buffer.Tokenize( ",", pos );
				token.Trim();
				if( token.CompareNoCase( "close" ) == 0 ) {
					ASSERT( m_pWCUpSocket ); // yonatan tmp
					AddDebugLogLine( false, _T("Received a \"Proxy-Connection: close\" header on a WCUpSocket") ); // yonatan tmp
					if( m_pWCUpSocket ) // just in case
						m_pWCUpSocket->m_bReceivedHttpClose = true; // WC-TODO: Safe_Delete socket after block xfer
					break;
				}
			}
		}
	}

	if (!slaveKeyFound)
	{
		DebugHttpHeaders(astrHeaders);
		TRACE(_T("*** No slaveKey received in %s\n"), szUrl);
		return HTTP_STATUS_BAD_REQUEST;
	}
	
	byte marc4_szFileHash[16];
	if (!WC_b64_Decode(b64_marc4_szFileHash, marc4_szFileHash, 16))
	{
		DebugHttpHeaders(astrHeaders);
		TRACE(_T("*** fileHash decoding failed in %s\n"), szUrl);
		return HTTP_STATUS_BAD_REQUEST;
	}

	Crypt.RefreshLocalKey();
	Crypt.encryptor.SetKey(Crypt.localKey, WC_KEYLENGTH);

	Crypt.encryptor.ProcessString(marc4_szFileHash, 16);	// use encryptor as decryptor ;)
	uchar aucUploadFileID[16];

	md4cpy(aucUploadFileID, marc4_szFileHash);
////// Superlexx - encryption - end //////////////////////////////////////////////////////////////////	

	/*uchar aucUploadFileID[16];
	if (!strmd4(szFileHash, aucUploadFileID)){
		DebugHttpHeaders(astrHeaders);
		return HTTP_STATUS_BAD_REQUEST;
	}*/

	if (md4cmp(aucUploadFileID, requpfileid) != 0) // client sending an HTTP request for wrong file
	{
		DebugHttpHeaders(astrHeaders);
		return HTTP_STATUS_FORBIDDEN;
	}

	//MORPH START -  Changed by SiRoB, Optimization requpfile
	/*
	CKnownFile* pUploadFile = theApp.sharedfiles->GetFileByID(aucUploadFileID);
	*/
	CKnownFile* pUploadFile = CheckAndGetReqUpFile();
	//MORPH END   -  Changed by SiRoB, Optimization requpfile
	if (pUploadFile == NULL){
		DebugHttpHeaders(astrHeaders);
		return HTTP_STATUS_NOT_FOUND;
	}
	//MORPH START - Changed by SiRoB, WebCache Fix
	/*
	if (ui64RangeEnd <= ui64RangeStart){ // && ui64RangeEnd-ui64RangeStart <= MAX_WEBCACHE_BLOCK_SIZE ???
	*/
	if (ui64RangeEnd > pUploadFile->GetFileSize() - (uint64)1)
		ui64RangeEnd = pUploadFile->GetFileSize() - (uint64)1;
	if (ui64RangeEnd < ui64RangeStart){
	//MORPH END - Changed by SiRoB, WebCache Fix
		DebugHttpHeaders(astrHeaders);
		TRACE(_T("*** Bad range in URL %s\n"), szUrl);
		return HTTP_STATUS_INV_RANGE;
	}

	//PC-TODO: Where does this flag need to be cleared again? 
	// When client is removed from uploading list?
	// When client is allowed to send more block requests?
	
	// everything is setup for uploading with WebCache.

	SetWebCacheUpState(WCUS_UPLOADING);

	Requested_Block_Struct* reqblock = new Requested_Block_Struct;
	reqblock->StartOffset = ui64RangeStart;
	reqblock->EndOffset = ui64RangeEnd + 1;
	md4cpy(reqblock->FileID, aucUploadFileID);
	reqblock->transferred = 0;
	AddReqBlock(reqblock);

	return HTTP_STATUS_OK;
}

bool CUpDownClient::ProcessWebCacheUpHttpResponse(const CStringAArray& astrHeaders)
{
// yonatan	ASSERT( m_eWebCacheUpState == WCUS_WAIT_CACHE_REPLY );

	if (astrHeaders.GetCount() == 0)
		throw CString(_T("Unexpected HTTP response - No headers available"));

	const CStringA& rstrHdr = astrHeaders.GetAt(0);
	UINT uHttpMajVer, uHttpMinVer, uHttpStatusCode;
	if (sscanf(rstrHdr, "HTTP/%u.%u %u", &uHttpMajVer, &uHttpMinVer, &uHttpStatusCode) != 3){
		CString strError;
		strError.Format(_T("Unexpected HTTP response: \"%hs\""), rstrHdr);
		throw strError;
	}
	if (uHttpMajVer != 1 || (uHttpMinVer != 0 && uHttpMinVer != 1)){
		CString strError;
		strError.Format(_T("Unexpected HTTP version: \"%hs\""), rstrHdr);
		throw strError;
	}

	CString strError;
	strError.Format(_T("Unexpected HTTP status code \"%u\""), uHttpStatusCode);
	throw strError;
}

bool CUpDownClient::SendWebCacheBlockRequests()
{
	//JP test if proxy is working
	if (thePrefs.ses_PROXYREQUESTS>100 && thePrefs.ses_successfullPROXYREQUESTS == 0) //disable webcache for this session if more than 100 blocks were tried withouth success
	{
		if(thePrefs.WebCacheDisabledThisSession == false) {
			thePrefs.WebCacheDisabledThisSession = true;
			AfxMessageBox(_T("Your Proxy server is not responding to your requests. Please check your webcachesettings."));
		}
		return false;
	}

	Crypt.useNewKey = true;	// moved here from SendBlockRequests()
	ASSERT( !m_PendingBlocks_list.IsEmpty() );

	USES_CONVERSION;
	ASSERT( GetDownloadState() == DS_DOWNLOADING );

	m_dwLastBlockReceived = ::GetTickCount();
	if (reqfile == NULL)
		throw CString(_T("Failed to send block requests - No 'reqfile' attached"));

	//JP START delete the socket if a Connection: close was received or blocklimit reached
	if( m_pWCDownSocket)
		if (m_pWCDownSocket->m_bReceivedHttpClose 
			||(thePrefs.GetWebCacheBlockLimit() != 0 && m_pWCDownSocket->blocksloaded >= thePrefs.GetWebCacheBlockLimit()))
		{
			m_pWCDownSocket->Safe_Delete();
		}
	//JP END

	if( m_pWCDownSocket == NULL ) {
		m_pWCDownSocket = new CWebCacheDownSocket(this);
		m_pWCDownSocket->SetTimeOut(GetWebCacheSocketDownloadTimeout());
		if (!m_pWCDownSocket->Create()){
			m_pWCDownSocket->Safe_Delete();
			m_pWCDownSocket = 0;
			return false;
		}
	}

	if( !m_pWCDownSocket->IsConnected() ) {
		SOCKADDR_IN sockAddr = {0};
		sockAddr.sin_family = AF_INET;
		sockAddr.sin_port = htons( thePrefs.WebCacheIsTransparent() ? 80 : thePrefs.webcachePort ); // Superlexx - TPS
		sockAddr.sin_addr.S_un.S_addr = thePrefs.WebCacheIsTransparent() ? GetIP() : ResolveWebCacheName(); // Superlexx - TPS
		if (sockAddr.sin_addr.S_un.S_addr == 0) //webcache name could not be resolved
			return false;
		m_pWCDownSocket->WaitForOnConnect();
		m_pWCDownSocket->Connect((SOCKADDR*)&sockAddr, sizeof sockAddr);
	}

	POSITION pos = m_PendingBlocks_list.GetHeadPosition();
	Pending_Block_Struct* pending = m_PendingBlocks_list.GetNext(pos);
	ASSERT( pending->block->StartOffset <= pending->block->EndOffset );
	pending->fZStreamError = 0;
	pending->fRecovered = 0;

	m_uReqStart = pending->block->StartOffset;
	m_uReqEnd = pending->block->EndOffset;
	m_nUrlStartPos = m_uReqStart;

// Superlexx - encryption - start ////////////////////////////////////////////////////////////
	CStringA b64_remoteSlaveKey;
	GenerateKey(Crypt.remoteSlaveKey);
	WC_b64_Encode(Crypt.remoteSlaveKey, WC_KEYLENGTH, b64_remoteSlaveKey);

	Crypt.RefreshRemoteKey();
	Crypt.decryptor.SetKey(Crypt.remoteKey, WC_KEYLENGTH);
	
	const uchar* fileHash = reqfile->GetFileHash();
	byte marc4_fileHash[16];
	md4cpy(marc4_fileHash, fileHash);
		
	Crypt.decryptor.ProcessString(marc4_fileHash, 16);	// here we use decryptor as encryptor ;)
	CStringA b64_marc4_filehash;
	WC_b64_Encode(marc4_fileHash, 16, b64_marc4_filehash);
// Superlexx - encryption - end //////////////////////////////////////////////////////////////

	CStringA strWCRequest;
	if (thePrefs.WebCacheIsTransparent())	// Superlexx - TPS
		strWCRequest.AppendFormat("GET /encryptedData/%I64u-%I64u/%s.htm HTTP/1.1\r\n",
			m_uReqStart,	// StartOffset
			m_uReqEnd,		// EndOffset
			b64_marc4_filehash );	// Superlexx - encryption - encrypted filehash
	else
	strWCRequest.AppendFormat("GET http://%s:%u/encryptedData/%I64u-%I64u/%s.htm HTTP/1.1\r\n",
		ipstrA( GetIP() ), // clients' IP
		GetUserPort(),	// clients' port
		m_uReqStart,	// StartOffset
		m_uReqEnd,		// EndOffset
		b64_marc4_filehash );	// Superlexx - encryption - encrypted filehash
		
	strWCRequest.AppendFormat("Host: %s:%u\r\n", ipstrA( GetIP() ), GetUserPort() ); // clients' IP and port
	strWCRequest.AppendFormat("Cache-Control: max-age=0\r\n" ); // do NOT DL this from the proxy! (timeout issue)
	if (thePrefs.GetWebCacheBlockLimit() != 0 && thePrefs.GetWebCacheBlockLimit() - m_pWCDownSocket->blocksloaded <= 1)
		strWCRequest.AppendFormat("Connection: close\r\nProxy-Connection: close\r\n" );
	else
// yonatan - removed 'Connection: keep-alive' - RFC 2068		strWCRequest.AppendFormat("Connection: keep-alive\r\nProxy-Connection: keep-alive\r\n" );
		strWCRequest.AppendFormat("Proxy-Connection: keep-alive\r\n" );
	strWCRequest.AppendFormat("Pragma: IDs=%u|", m_uWebCacheDownloadId);
	strWCRequest.AppendFormat("%s\r\n", b64_remoteSlaveKey);	// Superlexx - encryption : the remote slave key
	//MORPH START - Changed by SiRoB, ModID
	/*
	strWCRequest.AppendFormat("User-Agent: eMule/%s %s\r\n", T2CA(theApp.m_strCurVersionLong), T2CA(MOD_VERSION));
	*/
	strWCRequest.AppendFormat("User-Agent: eMule/%s %s\r\n", T2CA(theApp.m_strCurVersionLong), T2CA(theApp.m_strModVersion));
	//MORPH END   - Changed by SiRoB, ModID
	strWCRequest.AppendFormat("\r\n");

	if (thePrefs.GetDebugClientTCPLevel() > 0){
		DebugSend("WebCache-GET", this, reqfile->GetFileHash());
		Debug(_T("  %hs\n"), strWCRequest);
	}

	TRACE("HTTP GET sent:\r\n" + strWCRequest);

	CRawPacket* pHttpPacket = new CRawPacket(strWCRequest);
	theStats.AddUpDataOverheadFileRequest(pHttpPacket->size);
	m_pWCDownSocket->SendPacket(pHttpPacket);
	m_pWCDownSocket->SetHttpState(HttpStateRecvExpected);
	SetWebCacheDownState(WCDS_WAIT_CLIENT_REPLY);
	thePrefs.ses_PROXYREQUESTS++;
	return true;
}


bool CUpDownClient::IsUploadingToWebCache() const
{
	// this function should not check any socket ptrs, as the according sockets may already be closed/deleted
	return m_eWebCacheUpState == WCUS_UPLOADING;
}

bool CUpDownClient::IsDownloadingFromWebCache() const
{
	// this function should not check any socket ptrs, as the according sockets may already be closed/deleted
	return m_eWebCacheDownState != WCDS_NONE;
}

void CUpDownClient::OnWebCacheDownSocketClosed(int nErrorCode)
{
	if (nErrorCode)
		return;

	// restart WC download if cache just closed the connection without obvious reason
	if (GetDownloadState() == DS_DOWNLOADING)
	{
		if (thePrefs.GetLogWebCacheEvents())
		AddDebugLogLine(DLP_HIGH, false, _T("WebCache: Socket closed unexpedtedly, trying to reestablish connection"));
		TRACE(_T("+++ Restarting WebCache download - socket closed\n"));
		ASSERT( m_pWCDownSocket == NULL );
		SetWebCacheDownState(WCDS_NONE);
		if (IsProxy()) //JP fix neverending loop if the current block is not a PureGap any more
		{
			if (SINGLEProxyClient->ProxyClientIsBusy())
				SINGLEProxyClient->DeleteBlock();
			WebCachedBlockList.TryToDL();
		}
		else			
		SendBlockRequests(); //MORPH START - Changed by SiRoB, WebCache Fix
	}
	return;
}

void CUpDownClient::OnWebCacheDownSocketTimeout()
{
	// restart WC download if cache just stalls
	if (GetDownloadState() == DS_DOWNLOADING)
	{
		if (thePrefs.GetLogWebCacheEvents())
		AddDebugLogLine(DLP_HIGH, false, _T("WebCache Error: Socket TimeOut, trying to reestablish connection"));
		TRACE(_T("+++ Restarting WebCache download - socket timeout\n"));
		if (m_pWCDownSocket)
		{
			m_pWCDownSocket->Safe_Delete();
			ASSERT( m_pWCDownSocket == NULL );
		}
		SetWebCacheDownState(WCDS_NONE);
		if (IsProxy()) //JP fix neverending loop if the current block is not a PureGap any more
		{
			if (SINGLEProxyClient->ProxyClientIsBusy())
				SINGLEProxyClient->DeleteBlock();
			WebCachedBlockList.TryToDL();
		}
		else			
		 SendBlockRequests(); //MORPH - Changed by SiRoB, WebCache Fix
	}
	return;
}

void CUpDownClient::SetWebCacheDownState(EWebCacheDownState eState)
{
	if (m_eWebCacheDownState != eState)
	{
		m_eWebCacheDownState = eState;
		UpdateDisplayedInfo();
		if( eState == WCDS_WAIT_CLIENT_REPLY ) {
			ASSERT( m_pWCDownSocket );
			m_pWCDownSocket->DisableDownloadLimit();
		}
	}
}

void CUpDownClient::SetWebCacheUpState(EWebCacheUpState eState)
{
	if (m_eWebCacheUpState != eState)
	{
		m_eWebCacheUpState = eState;

		theApp.uploadqueue->ReSortUploadSlots(true); // Superlexx - from 0.44a PC code

		UpdateDisplayedInfo();
	}
}

void CUpDownClient::PublishWebCachedBlock( const Requested_Block_Struct* block )
{
	//MORPH START - Changed by SiRoB, New ResolveWebcachename
	/*
	POSITION OHCBpos = WC_OHCBManager.AddWCBlock(	thePrefs.WebCacheIsTransparent() ? 0 : ResolveWebCacheName(),
	*/
	POSITION OHCBpos = WC_OHCBManager.AddWCBlock(	thePrefs.WebCacheIsTransparent() ? 0 : m_pWCDownSocket->GetIP(),
													GetIP(),
													GetUserPort(),
													reqfile->GetFileHash(),
													block->StartOffset,
													block->EndOffset,
													Crypt.remoteKey,
													reqfile->IsLargeFile());

	POSITION pos = reqfile->srclist.GetHeadPosition();
	const uchar* filehash;

	UINT part = (UINT)(block->StartOffset / PARTSIZE);
	uint32 nrOfSentOHCBs = 0;
	filehash = reqfile->GetFileHash();

	while( pos )
	{
		CUpDownClient* cur_client = reqfile->srclist.GetNext( pos );
		if( !cur_client->IsProxy()
			&& cur_client != this // 'this' is the client we have downloaded the block from
			&& cur_client->m_bIsAcceptingOurOhcbs
			&& !cur_client->IsPartAvailable( part )	// using standard function for pre-1.9a clients
			&& !cur_client->SupportsMultiOHCBs()	// pre-1.9a client
			&& cur_client->IsBehindOurWebCache() )	// inefficient
		{
			CSafeMemFile data;
			// <Proxy IP 4><IP 4><PORT 2><filehash 16><offset 4><key 16>
			if (thePrefs.WebCacheIsTransparent())
				data.WriteUInt32( 0 ); // Superlexx - TPS
			else
				//MORPH START - Changed by SiRoB, New ResolveWebCachename
				/*
				data.WriteUInt32( ResolveWebCacheName() ); // Proxy IP
				*/
				data.WriteUInt32( m_pWCDownSocket->GetIP() ); // Proxy IP
				//MORPH END   - Changed by SiRoB, New ResolveWebCachename

			data.WriteUInt32( GetIP() ); // Source client IP
			data.WriteUInt16( GetUserPort() ); // Source client port
			data.WriteHash16( block->FileID/*reqfile->GetFileHash()*/ ); // filehash
			if (reqfile->IsLargeFile()) {
				data.WriteUInt64( block->StartOffset ); // start offset
				data.WriteUInt64( block->EndOffset ); // end offset
			} else {
				data.WriteUInt32( (uint32)block->StartOffset ); // start offset
				data.WriteUInt32( (uint32)block->EndOffset ); // end offset
			}
			// Superlexx - encryption : remoteKey
			data.Write( Crypt.remoteKey, WC_KEYLENGTH );
			// Superlexx end

			if( cur_client->SupportsWebCacheUDP()
				&& !cur_client->HasLowID()
				&& GetUDPPort() != 0	// thx to SiRoB
				&& !(cur_client->socket && socket->IsConnected()))
			{	// send UDP
				data.WriteUInt32( cur_client->m_uWebCacheDownloadId );
				if (thePrefs.GetLogWebCacheEvents())
					AddDebugLogLine( false, _T("WCBlock sent to client - UDP"));
				Packet* packet = new Packet(&data);
				if (cur_client->SupportsWebCacheProtocol())
					packet->prot = OP_WEBCACHEPROT; //if the client supports webcacheprot use that (keep backwards compatiblity)
				else
					packet->prot = OP_EMULEPROT; //UDP-Packets use eMule-protocol WC-TODO: remove this eventually
				packet->opcode = OP_HTTP_CACHED_BLOCK;
				if (thePrefs.GetDebugClientUDPLevel() > 0)
					DebugSend("OP__Http_Cached_Block (UDP)", cur_client );
				theApp.clientudp->SendPacket(packet, cur_client->GetIP(), cur_client->GetUDPPort());
				WC_OHCBManager.AddRecipient(OHCBpos, cur_client);
				nrOfSentOHCBs++;
			}
			else
			{	// send TCP
				Packet* packet = new Packet(&data);
				if (cur_client->SupportsWebCacheProtocol())
					packet->prot = OP_WEBCACHEPROT;
				else
					packet->prot = OP_EDONKEYPROT; //TCP-Packets use edonkey-protocol WC-TODO: remove this eventually
				packet->opcode = OP_HTTP_CACHED_BLOCK;
				theStats.AddUpDataOverheadOther(packet->size);
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					DebugSend("OP__Http_Cached_Block (TCP)", cur_client );
				if( cur_client->socket && socket->IsConnected() )
				{
					if (thePrefs.GetLogWebCacheEvents())
						AddDebugLogLine( false, _T("WCBlock sent to client - TCP") );
					cur_client->socket->SendPacket( packet );
					nrOfSentOHCBs++;
				}
				else
				{
					if (thePrefs.GetLogWebCacheEvents())
						AddDebugLogLine( false, _T("WCBlock added to list - TCP") );
					cur_client->m_WaitingPackets_list.AddTail(packet);
				}
				WC_OHCBManager.AddRecipient(OHCBpos, cur_client);
			}
		}
	}

	if (nrOfSentOHCBs < WC_NR_OF_XPRESS_OHCBS)
	{
		uint32 now = GetTickCount();
		CUpDownClientPtrList* XpressOHCBRecipients = theApp.clientlist->XpressOHCBRecipients(WC_NR_OF_XPRESS_OHCBS - nrOfSentOHCBs, block);
		uint32 nrOfOHCBsInThePacket = 0;
		while (XpressOHCBRecipients->GetCount() > 0)
		{
			CUpDownClient* cur_client = XpressOHCBRecipients->GetHead();
			Packet* packet = WC_OHCBManager.GetWCBlocksForClient(cur_client, nrOfOHCBsInThePacket, OHCBpos);
			theStats.AddUpDataOverheadOther(packet->size);

			if( cur_client->SupportsWebCacheUDP()
				&& !cur_client->HasLowID()
				&& GetUDPPort() != 0
				&& !(cur_client->socket && socket->IsConnected())
				&& nrOfOHCBsInThePacket <= WC_MAX_OHCBS_IN_UDP_PACKET
				&& theApp.downloadqueue->GetFailedUDPFileReasks() * 100.0 / (theApp.downloadqueue->GetUDPFileReasks() + 1) < 0,2 // less than 20% of UDP reasks failed globally
				&& cur_client->m_nTotalUDPPackets > 4 && (float)(cur_client->m_nFailedUDPPackets/cur_client->m_nTotalUDPPackets) < 0,2) // less than 20% of UDP reasks failed for this client
			{	// send UDP
				if (thePrefs.GetLogWebCacheEvents())
					AddDebugLogLine( false, _T("Multi-WCBlock (%d OHCBs, UDP) sent to client %s"), nrOfOHCBsInThePacket, cur_client->DbgGetClientInfo());
				if (thePrefs.GetDebugClientUDPLevel() > 0)
					DebugSend("OP__Multi_Http_Cached_Block (UDP)", cur_client );
				lastMultiOHCBPacketSent = now;
				theApp.clientudp->SendPacket(packet, cur_client->GetIP(), cur_client->GetUDPPort());
			}
			else
			{
				if (thePrefs.GetDebugClientTCPLevel() > 0)
					DebugSend("OP__Multi_Http_Cached_Block (TCP)", cur_client );
				if( cur_client->socket && socket->IsConnected() )
				{
					if (thePrefs.GetLogWebCacheEvents())
						AddDebugLogLine( false, _T("Multi-WCBlock (%d OHCBs, TCP) sent to client %s"), nrOfOHCBsInThePacket, cur_client->DbgGetClientInfo());
					lastMultiOHCBPacketSent = now;
					cur_client->socket->SendPacket( packet );
				}
				else
				{
					if (thePrefs.GetLogWebCacheEvents())
						AddDebugLogLine( false, _T("Multi-WCBlock (%d OHCBs, TCP SafeSendPacket) sent to client %s"), nrOfOHCBsInThePacket, cur_client->DbgGetClientInfo());
					lastMultiOHCBPacketSent = now;
					cur_client->SafeSendPacket(packet);
				}
			}
			XpressOHCBRecipients->RemoveHead();
		}
		delete XpressOHCBRecipients;
	}
}

bool CUpDownClient::IsWebCacheUpSocketConnected() const
{
	return( m_pWCUpSocket ? m_pWCUpSocket->IsConnected() : false );
}

bool CUpDownClient::IsWebCacheDownSocketConnected() const
{
	return( m_pWCDownSocket ? m_pWCDownSocket->IsConnected() : false );
}

uint16 CUpDownClient::GetNumberOfClientsBehindOurWebCacheAskingForSameFile()
{
	POSITION pos = reqfile->srclist.GetHeadPosition();

	uint16 toReturn = 0;

	while( pos ) {
		CUpDownClient* cur_client = reqfile->srclist.GetNext( pos );
		if( !cur_client->IsProxy()
			&& cur_client != this // 'this' is the client we want to download data from
			&& cur_client->IsBehindOurWebCache())
			toReturn++;
	}
	return toReturn;
}

//uint16 CUpDownClient::GetNumberOfClientsBehindOurWebCacheHavingSameFileAndNeedingThisBlock(Pending_Block_Struct* pending, uint32 maxClients) // Superlexx - COtN
//{
//	uint16 toReturn = 0;
//	uint16 part = pending->block->StartOffset / PARTSIZE;
////	POSITION pos = reqfile->srclist.GetHeadPosition();
//	POSITION pos = 
//	while( pos )
//	{
//		CUpDownClient* cur_client = reqfile->srclist.GetNext( pos );
//		if( cur_client->m_bIsAcceptingOurOhcbs
//			&& !cur_client->IsProxy()
//			&& cur_client != this // 'this' is the client we want to download data from
//			&& cur_client->IsBehindOurWebCache()
//			&& !cur_client->IsPartAvailable(part))
//			toReturn++;
//	}
//	return toReturn;
//}
//JP trusted OHCB-senders START

// yonatan - moved code from IsTrustedOhcbSender to AddWebCachedBlockToStats,
// might delete the client who sent the OHCB (if IsGood==false SafeSendPacket is called)

void CUpDownClient::AddWebCachedBlockToStats( bool IsGood )
{
	WebCachedBlockRequests++;
	if (IsGood)
		SuccessfulWebCachedBlockDownloads++;

	// check if we still trust the senders' ohcbs 
	if (WebCachedBlockRequests<100 //we need to try 100 blocks before the statistic is meaningfull 
		|| (SuccessfulWebCachedBlockDownloads*100/WebCachedBlockRequests >= thePrefs.webcacheTrustLevel)) //if we have tried those blocks we need to be more successfull than the trustlevel 
		return; 
	// client has sent too many bad ohcbs 
	if (thePrefs.GetLogWebCacheEvents()) 
		AddDebugLogLine(false, _T("We don't trust OHCBs from client %s"), DbgGetClientInfo());      
	m_bIsTrustedOHCBSender = false; 
	SendStopOHCBSending(); // Support for this is checked in SendStopOHCBSending 
}

/*void CUpDownClient::AddWebCachedBlockToStats( bool IsGood )
{
	WebCachedBlockRequests++;
	if (IsGood)
		SuccessfulWebCachedBlockDownloads++;
	// check if we still trust the senders' ohcbs
if (WebCachedBlockRequests<10) //if less than 10 requests made
		return;
	if (WebCachedBlockRequests>=10 //if between 10 and
	&& WebCachedBlockRequests<50 //50 requests made and
		&& SuccessfulWebCachedBlockDownloads*100/WebCachedBlockRequests>=10)// more than 10% successfull
		return;
	if (WebCachedBlockRequests>=50 //if between 50 and
	&& WebCachedBlockRequests<100 //100 requests made and
		&& SuccessfulWebCachedBlockDownloads*100/WebCachedBlockRequests>=20)// more than 20% successfull
		return;
	if (WebCachedBlockRequests>=100 //if more than 100 requests made and
		&& SuccessfulWebCachedBlockDownloads*100/WebCachedBlockRequests>=30)// more than 30% successfull
		return;
	// client has sent too many bad ohcbs
		if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine(false, _T("We don't trust OHCBs from client %s"), DbgGetClientInfo());	
		m_bIsTrustedOHCBSender = false;
	SendStopOHCBSending(); // Support for this is checked in SendStopOHCBSending
}*/
//JP trusted OHCB-senders END

void CUpDownClient::SendStopOHCBSending() //make a new TCP-connection here because it's important
{
if( SupportsOhcbSuppression() ) 
{
			Packet* packet = new Packet(OP_DONT_SEND_OHCBS,0,OP_WEBCACHEPROT);
	if( SafeSendPacket( packet ) ) 
	{ // if client was not deleted
				theStats.AddUpDataOverheadOther(packet->size);
		AddDebugLogLine( false, _T("OP_DONT_SEND_OHCBS sent")); // yonatan tmp
				DebugSend("OP__Dont_Send_Ohcbs", this );
		m_bIsAllowedToSendOHCBs = false;
			}
}
}

//JP TEST THIS!!! (WE ARE NOT USING IT YET)
void CUpDownClient::SendResumeOHCBSendingTCP() //only add the packet to the queue because it's not so important ??
{
if( SupportsOhcbSuppression() && m_bIsTrustedOHCBSender) 
{
	Packet* packet = new Packet(OP_RESUME_SEND_OHCBS,0,OP_WEBCACHEPROT);
	theStats.AddUpDataOverheadOther(packet->size);
	if (thePrefs.GetDebugClientTCPLevel() > 0)
		DebugSend("OP_RESUME_SEND_OHCBS (TCP)", this );
	if( socket && socket->IsConnected() ) {
		if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false, _T("OP_RESUME_SEND_OHCBS sent to client - TCP") );
		socket->SendPacket( packet );
	} else {
		if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false, _T("OP_RESUME_SEND_OHCBS added to list - TCP") );
		m_WaitingPackets_list.AddTail(packet);
		}
	m_bIsAllowedToSendOHCBs = true;
}
}
//JP Add THIS!!! (WE ARE NOT USING IT YET)
void CUpDownClient::SendResumeOHCBSendingUDP()
{
	if( SupportsOhcbSuppression() && m_bIsTrustedOHCBSender && SupportsWebCacheUDP() && !HasLowID()) 
	{
		//JP WC-TODO: add sending the UDP packet here
		m_bIsAllowedToSendOHCBs = true;
	}
}

// Superlexx - MFR
// note that the requesting a file in the usual way doesn't mean requesting OHCBs for it anymore
// the chunk states might be useful for chunk selection, not implemented and not planned ATM

Packet* CUpDownClient::CreateMFRPacket()
{
	// TODO: WC: remove the commented block after testing
	/*
	CSafeMemFile* data = new CSafeMemFile();
	if (!AttachMultiOHCBsRequest(*data))
	{
		delete data;
		return NULL;	// we don't want anything from this client
	}
	Packet* toSend = new Packet(data, OP_WEBCACHEPROT, OP_MULTI_FILE_REQ);
	delete data;
	*/
	CSafeMemFile data;
	if (!AttachMultiOHCBsRequest(data))
		return NULL;	// we don't want anything from this client
	Packet* toSend = new Packet(&data, OP_WEBCACHEPROT, OP_MULTI_FILE_REQ);
	uint32 unpackedSize = toSend->size;
	toSend->PackPacket();
	if (toSend->size > unpackedSize)
		toSend->UnPackPacket(unpackedSize);
	return toSend;
}

bool CUpDownClient::AttachMultiOHCBsRequest(CSafeMemFile &data)
{
	if (!SupportsWebCache()
		|| !IsBehindOurWebCache()
		|| !SupportsMultiOHCBs())
		return false;
	ASSERT(reqfile);
	//MORPH - Changed By SiRoB, WebCache Fix
	uint8 fileCount = m_OtherRequests_list.GetCount()+m_OtherNoNeeded_list.GetCount();
	if (!fileCount && data.GetPosition() == 0)
		return false;
	data.WriteUInt8(fileCount); // number of requested files will be written here later
	
	for (POSITION pos = m_OtherRequests_list.GetHeadPosition(); pos; m_OtherRequests_list.GetNext(pos))
	{
		data.WriteHash16(m_OtherRequests_list.GetAt(pos)->GetFileHash());
		m_OtherRequests_list.GetAt(pos)->WritePartStatus(&data);
	}
	
	for (POSITION pos = m_OtherNoNeeded_list.GetHeadPosition(); pos; m_OtherNoNeeded_list.GetNext(pos))
	{
		data.WriteHash16(m_OtherNoNeeded_list.GetAt(pos)->GetFileHash());
		m_OtherNoNeeded_list.GetAt(pos)->WritePartStatus(&data);
	}
	return true;
}

void CUpDownClient::SendOHCBsNow()
{
	uint32 nrOfOHCBsInThePacket = 0;
	uint32 now = GetTickCount();
	Packet* packet = WC_OHCBManager.GetWCBlocksForClient(this, nrOfOHCBsInThePacket, NULL);
	if (!packet) // nothing to send
		return;
	theStats.AddUpDataOverheadOther(packet->size);
	if( !HasLowID()
		&& GetUDPPort() != 0
		&& !(socket && socket->IsConnected())
//		&& nrOfOHCBsInThePacket <= WC_MAX_OHCBS_IN_UDP_PACKET
		&& a(nrOfOHCBsInThePacket)
//		&& (theApp.downloadqueue->GetFailedUDPFileReasks() * 100.0 / (theApp.downloadqueue->GetUDPFileReasks() + 1) < 0,2 ) // less than 20% of UDP reasks failed globally
		&& b(theApp.downloadqueue->GetFailedUDPFileReasks(), theApp.downloadqueue->GetUDPFileReasks()) // less than 20% of UDP reasks failed globally
//		&& (m_nTotalUDPPackets > 4 && (m_nFailedUDPPackets * 100) / m_nTotalUDPPackets < 20)) // less than 20% of UDP reasks failed for this client
		&& c(m_nTotalUDPPackets, m_nFailedUDPPackets)) // less than 20% of UDP reasks failed for this client
	{	// send UDP
		if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false, _T("multi-OHCB-packet sent to client - UDP"));
		if (thePrefs.GetDebugClientUDPLevel() > 0)
			DebugSend("OP__Multi_Http_Cached_Block (UDP)", this );
		lastMultiOHCBPacketSent = now;
		theApp.clientudp->SendPacket(packet, GetIP(), GetUDPPort());
	}
	else if (!HasLowID() // don't try to send data to disconnected lowIDs
			|| (socket && socket->IsConnected()))
	{	// send TCP
		if (thePrefs.GetDebugClientTCPLevel() > 0)
			DebugSend("OP__Multi_Http_Cached_Block (TCP)", this);
		if( socket && socket->IsConnected() )
		{
			if (thePrefs.GetLogWebCacheEvents())
				AddDebugLogLine( false, _T("Multi-WCBlock sent to client - TCP") );
			lastMultiOHCBPacketSent = now;
			socket->SendPacket( packet );
		}
		else
		{
		if (thePrefs.GetLogWebCacheEvents())
				AddDebugLogLine( false, _T("Multi-WCBlock sent to client - TCP SafeSendPacket") );
			lastMultiOHCBPacketSent = now;
			SafeSendPacket(packet); // it's ugly, I know...
		}
}
}