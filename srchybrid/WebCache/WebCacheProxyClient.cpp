#include "StdAfx.h"
#include "WebCacheProxyClient.h"
#include "WebCacheSocket.h"
#include "eMule.h"
#include "Packets.h"
#include "Statistics.h"
#include "ClientList.h"
#include "eMuleDlg.h"
#include "TransferWnd.h"
#include "WebCachedBlockList.h"

CWebCacheProxyClient* SINGLEProxyClient = 0;

CWebCacheProxyClient::CWebCacheProxyClient(CWebCachedBlock* iBlock) : CUpDownClient()
{
	m_bProxy = true;
	Crypt.isProxy = true;
	block = iBlock;
	m_clientSoft = SO_WEBCACHE;
}
void CWebCacheProxyClient::UpdateClient(CWebCachedBlock* iBlock) // don't delete and recreate SingleProxyClient
{
	//remove client from all lists
	theApp.clientlist->RemoveClient(this);
	if( reqfile ) {
		theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource( this, reqfile );
		POSITION pos = SINGLEProxyClient->reqfile->srclist.Find(this);
		if( pos )
			reqfile->srclist.RemoveAt(pos);
	}
	if( block )
		delete block;

	block = iBlock;	//update block

	if( m_pWCDownSocket )
	{
		if( !m_pWCDownSocket->m_bReceivedHttpClose //Not Connection: close header received
			&& thePrefs.PersistentConnectionsForProxyDownloads //persistant connections allowed
			&& m_pWCDownSocket->IsConnected() //Socket is connected
			&& !(thePrefs.GetWebCacheBlockLimit() != 0 && m_pWCDownSocket->blocksloaded >= thePrefs.GetWebCacheBlockLimit())) //Not Blocklimit set and limit reached
		{ 
			SOCKADDR_IN sockAddr = {0};
			int nSockAddrLen = sizeof(sockAddr);
			SINGLEProxyClient->m_pWCDownSocket->GetPeerName((SOCKADDR*)&sockAddr, &nSockAddrLen);
			if( sockAddr.sin_addr.S_un.S_addr != block->GetProxyIp() )
			{ // wrong ip - delete Socket
				if (thePrefs.GetLogWebCacheEvents())
					AddDebugLogLine(false, _T("RECONNECTING ProxyClient!!"));
				m_pWCDownSocket->Safe_Delete();
				m_pWCDownSocket = 0;
			}
#ifdef _DEBUG
			else
			{
				if (thePrefs.GetLogWebCacheEvents())
					AddDebugLogLine(false, _T("PERSISTANT CONNECTION!!"));
			}
#endif //_DEBUG
		}
		else
		{ // socket not connected or Connection: close header received or no persistant connections allowed or Block limit reached - delete Socket
			if (thePrefs.GetLogWebCacheEvents())
				AddDebugLogLine(false, _T("RECONNECTING ProxyClient!!"));
			m_pWCDownSocket->Safe_Delete();
			m_pWCDownSocket = 0;
		}
	}
}

CWebCacheProxyClient::~CWebCacheProxyClient(void)
{
	ASSERT(!theApp.emuledlg->IsRunning());
	if (thePrefs.GetLogWebCacheEvents())
		AddDebugLogLine(false, _T("ProxyClient deleted"));
	theApp.clientlist->RemoveClient(this);
	if( reqfile ) {
		theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource( this, reqfile );
		POSITION pos = SINGLEProxyClient->reqfile->srclist.Find(this);
		if( pos )
			reqfile->srclist.RemoveAt(pos);
	}
	if( block )
		delete block;
	SINGLEProxyClient = 0;
}

// this is an almost-copy of CUpDownClient::SendWebCacheBlockRequests
bool CWebCacheProxyClient::SendWebCacheBlockRequests()
{
	//check if we have reached the limit
	if (thePrefs.ses_WEBCACHEREQUESTS>100 && thePrefs.ses_successfull_WCDOWNLOADS == 0) //disable webcache for this session if more than 100 blocks were tried withouth success
	{
		thePrefs.WebCacheDisabledThisSession = true;
		AfxMessageBox(_T("Your proxy-server does not seem to be caching data. There was no successful Webcache-Requests out of more than 100 that were sent. Please review your proxy-configuration. WebCache downloads have been disabled until emule is restarted!"));
		return false;
	}

	ASSERT( block );
	ASSERT(block->IsValid()); //there was a problem with this after socket timeout. Should be taken care of now, but better check anyways

	USES_CONVERSION;
	ASSERT( GetDownloadState() == DS_DOWNLOADING );

	m_dwLastBlockReceived = ::GetTickCount();
	if (reqfile == NULL)
		throw CString(_T("Failed to send block requests - No 'reqfile' attached"));

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
		sockAddr.sin_port = htons( (block->m_uProxyIp == 0) ? 80 : thePrefs.webcachePort ); // Superlexx - TPS
		sockAddr.sin_addr.S_un.S_addr = (block->m_uProxyIp == 0) ? block->m_uHostIp : block->m_uProxyIp; // Superlexx - TPS
		m_pWCDownSocket->WaitForOnConnect();
		m_pWCDownSocket->Connect((SOCKADDR*)&sockAddr, sizeof sockAddr);
	}

	m_uReqStart = block->m_uStart;
	m_uReqEnd = block->m_uEnd;
	m_nUrlStartPos = m_uReqStart;

// Superlexx - encryption - start ////////////////////////////////////////////////////////////////
	if (IsProxy())
		md4cpy(Crypt.remoteKey, block->remoteKey);
	Crypt.RefreshRemoteKey();
	Crypt.decryptor.SetKey(Crypt.remoteKey, WC_KEYLENGTH);

	
	const uchar* fileHash = block->m_FileID;
	byte marc4_fileHash[16];
	md4cpy(marc4_fileHash, fileHash);
		
	Crypt.decryptor.ProcessString(marc4_fileHash, 16);	// here we use decryptor as encryptor ;)
	CStringA b64_marc4_filehash;
	WC_b64_Encode(marc4_fileHash, 16, b64_marc4_filehash);
// Superlexx - encryption - end //////////////////////////////////////////////////////////////////

	CStringA strWCRequest;
	strWCRequest.AppendFormat("GET http://%s:%u/encryptedData/%u-%u/%s.htm HTTP/1.1\r\n",
		ipstrA( block->m_uHostIp ), // clients' IP
		block->m_uHostPort, // clients' port
		m_uReqStart,		// StartOffset
		m_uReqEnd,			// EndOffset
		b64_marc4_filehash );	// Superlexx - encryption - request using the encrypted file hash

	strWCRequest.AppendFormat("Host: %s:%u\r\n", ipstrA( block->m_uHostIp ), block->m_uHostPort ); // clients' IP and port
	strWCRequest.AppendFormat("Cache-Control: only-if-cached\r\n" );
	if (thePrefs.GetWebCacheBlockLimit() != 0 && thePrefs.GetWebCacheBlockLimit() - m_pWCDownSocket->blocksloaded <= 1)
		strWCRequest.AppendFormat("Connection: close\r\nProxy-Connection: close\r\n" );
	else
// yonatan - removed 'Connection: keep-alive' - RFC 2068		strWCRequest.AppendFormat("Connection: keep-alive\r\nProxy-Connection: keep-alive\r\n" );
		strWCRequest.AppendFormat("Proxy-Connection: keep-alive\r\n" );
	//MORPH START - Changed by SiRoB, ModID
	/*
	strWCRequest.AppendFormat("User-Agent: eMule/%s %s\r\n", T2CA(theApp.m_strCurVersionLong), T2CA(MOD_VERSION));
	*/
	strWCRequest.AppendFormat("User-Agent: eMule/%s %s\r\n", T2CA(theApp.m_strCurVersionLong), T2CA(theApp.m_strModVersion));
	//MORPH END   - Changed by SiRoB, ModID

	strWCRequest.AppendFormat("\r\n");

	if (thePrefs.GetDebugClientTCPLevel() > 0){
		DebugSend("WebCache-GET (cached block)", this, (char*)reqfile->GetFileHash());
		Debug(_T("  %hs\n"), strWCRequest);
	}

	CRawPacket* pHttpPacket = new CRawPacket(strWCRequest);
	theStats.AddUpDataOverheadFileRequest(pHttpPacket->size);
	m_pWCDownSocket->SendPacket(pHttpPacket);
	m_pWCDownSocket->SetHttpState(HttpStateRecvExpected);
	SetWebCacheDownState(WCDS_WAIT_CLIENT_REPLY);
	m_PendingBlocks_list.AddTail(block->CreatePendingBlock());
	thePrefs.ses_WEBCACHEREQUESTS ++; // jp webcache statistics
	return true;
}

// always returns false (which means to the rest of emule that client was deleted)
bool CWebCacheProxyClient::TryToConnect(bool bIgnoreMaxCon, CRuntimeClass* pClassSocket)
{
//	ASSERT( !block );
	if( !ProxyClientIsBusy() )
		WebCachedBlockList.TryToDL();

	return SINGLEProxyClient; // yonatan tmp
}

void CWebCacheProxyClient::OnWebCachedBlockDownloaded( const Requested_Block_Struct* reqblock )
{
	ASSERT( block );
	block->OnSuccessfulDownload();
	delete block;
	block = 0;
	if (thePrefs.GetLogWebCacheEvents())
		AddDebugLogLine( false, _T("Cached block downloaded from proxy!!!") ); // yonatan log
	thePrefs.ses_successfull_WCDOWNLOADS ++; // jp webcache statistics
}

bool CWebCacheProxyClient::ProxyClientIsBusy()
{
	if (!m_pWCDownSocket)
	{
		if (block)
		{
			delete block;
			block = 0;
			if (thePrefs.GetLogWebCacheEvents())
				AddDebugLogLine(false, _T("WebCachedBlock without Webcachesocket deleted"));
		}
	}
	return block;
}

void CWebCacheProxyClient::DeleteBlock()
{
	if (block)
	{
		delete block;
		block = 0;
	}
}