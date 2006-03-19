#include "stdafx.h"
#include "WebCachedBlock.h"
#include "WebCache.h"
#include "SafeFile.h"
#include "OtherFunctions.h"
#include "eMule.h"
#include "DownloadQueue.h"
#include "Opcodes.h"
#include "WebCacheProxyClient.h"
#include "ClientList.h"
#include "eMuleDlg.h"
#include "TransferWnd.h"
#include "WebCacheSocket.h"
#include "WebCachedBlockList.h"
#include "ThrottledChunkList.h" // jp Don't request chunks for which we are currently receiving proxy sources
#include "KnownFileList.h"
#include "Log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CWebCachedBlock::CWebCachedBlock( const BYTE* packet, uint32 size, CUpDownClient* client, bool XpressOHCB )
{
	m_uRequestCount = 0; // what is this for?
	m_bDownloaded = false;
	m_bRequested = false;

	md4cpy( m_UserHash, client->GetUserHash() );
	CSafeMemFile indata(packet, size );
	// <Proxy-ip 4><IP 4><PORT 2><filehash 16><m_uStart 4><m_uEnd 4><remoteKey WC_KEYLENGTH>
	m_uProxyIp = indata.ReadUInt32();
	m_uHostIp = indata.ReadUInt32();
	m_uHostPort = indata.ReadUInt16();
	indata.ReadHash16( m_FileID );

	const CPartFile* file = GetFile();

	if( !file )
	{
		const CKnownFile* knownFile = GetKnownFile();
		if( knownFile && thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false, _T("deleting CWebCachedBlock because %s is not a PartFile\n"), knownFile->GetFileName() );
		else if (!knownFile && thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false, _T("deleting CWebCachedBlock because we don't know a file with the hash: %s\n"), md4str( m_FileID ) );
	
		delete this;
		return;
	}
	if (file->IsLargeFile()) {
		if (client->SupportsLargeFiles()) {
			m_uStart = indata.ReadUInt64();
			m_uEnd = indata.ReadUInt64();
		} else {
			DebugLogWarning(_T("deleting CWebCacheBlock because Client without 64bit file support requested large file; %s, File=\"%s\""), client->DbgGetClientInfo(), file->GetFileName());
			delete this;
			return;
		}
	}
	else {
		m_uStart = indata.ReadUInt32();
		m_uEnd = indata.ReadUInt32();
	}
	m_uTime = GetTickCount(); //JP remove old chunks (currently only for Stopped-List)

	// Superlexx - encryption
	indata.Read( remoteKey, WC_KEYLENGTH );

	// yonatan log
	if (thePrefs.GetLogWebCacheEvents())
	AddDebugLogLine( false, _T("CWebCachedBlock: proxy-ip=%s, host-ip=%s, host-port=%u, filehash=%s, start=%I64u, end=%I64u, key=%s16"),
		ipstr(m_uProxyIp),
		ipstr(m_uHostIp),
		m_uHostPort,
		md4str( m_FileID ), //not sure if this is correct, but now I can read the hash even in unicode build
		m_uStart,
		m_uEnd ,
		md4str(remoteKey)); //not sure if this is correct, but now I can read the key even in unicode build

	//JP don't accept OHCBs for stopped files
	if( file->IsStopped() ) {
		if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false, _T("deleting CWebCachedBlock because %s is Stopped\n"), file->GetFileName() );
		delete this;
		return;
	}
		
	//JP accept OHCBs for paused files
	if( file->GetStatus()==PS_PAUSED ) {
		Debug( _T("CWebCachedBlock: %s is paused\n"), file->GetFileName() );
		StoppedWebCachedBlockList.CleanUp();
		if( !StoppedWebCachedBlockList.IsFull() && IsValid())
			StoppedWebCachedBlockList.AddTail(this);
		else
		delete this;
		return;
	}

	if( file->GetStatus()==PS_ERROR ) {
		if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false, _T("deleting CWebCachedBlock because %s - Status = PS_ERROR\n"), file->GetFileName() );
		delete this;
		return;
	}

	if( thePrefs.WebCacheIsTransparent() && m_uHostPort != 80 )
	{
		if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false, _T("deleting CWebCachedBlock because only port 80 is cached on a transparent proxies, received port: %i\n"), m_uHostPort );
		delete this;
		return;
	}

/*	if( !file->IsPureGap( m_uStart, m_uEnd ) ) {
		Debug( _T( "CWebCachedBlock: Not a pure gap (block already downloaded), file: %s\n"), file->GetFileName() );
		delete this;
		return;
	}
*/
	if( WebCachedBlockList.IsFull() ) {
		if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false, _T("deleting CWebCachedBlock because WebCachedBlockList is Full!!!") );
		delete this;
		return;
	}

	if( IsValid() ) {
		//JP moved here so only valid chunks get added
	// jp Don't request chunks for which we are currently receiving proxy sources START Here because we also need to add blocks that are NotPureGaps
		ThrottledChunk cur_ThrottledChunk;
		md4cpy(cur_ThrottledChunk.FileID, this->m_FileID);
		cur_ThrottledChunk.ChunkNr=(uint16)(this->m_uStart/PARTSIZE);
		cur_ThrottledChunk.timestamp = GetTickCount();
		ThrottledChunkList.AddToList(cur_ThrottledChunk); // compare this chunk to the chunks in the list and add it if it's not found
	// jp Don't request chunks for which we are currently receiving proxy sources END
		GetFile()->AddGap(m_uStart, m_uEnd);
		if( !DownloadIfPossible() ) {
			XpressOHCB ? WebCachedBlockList.AddHead( this ) : WebCachedBlockList.AddTail( this );
			if (thePrefs.GetLogWebCacheEvents())
			AddDebugLogLine( false, _T("WebCachedBlock added to queue") );
		}
	} else {
		delete this;
	}
}


CWebCachedBlock::~CWebCachedBlock()
{
if( theApp.clientlist ) 
{
		CUpDownClient* client = theApp.clientlist->FindClientByUserHash( m_UserHash );
		CPartFile* file = GetFile();

		if(m_bRequested)
		{
			if( client )
				client->AddWebCachedBlockToStats( m_bDownloaded );
			if (file)
			file->AddWebCachedBlockToStats( m_bDownloaded, m_uEnd-m_uStart  );
	}
	}

	if (thePrefs.GetLogWebCacheEvents())
	AddDebugLogLine( false, _T("CWebCachedBlock::~CWebCachedBlock(): blocks on queue: %u"), WebCachedBlockList.GetCount());
	ASSERT( !WebCachedBlockList.Find( this ) );
}
								 
CPartFile* CWebCachedBlock::GetFile() const
{
	return( theApp.downloadqueue->GetFileByID( m_FileID ) );
}

CKnownFile* CWebCachedBlock::GetKnownFile() const
{
	return( theApp.knownfiles->FindKnownFileByID( m_FileID ) );
}

uint32 CWebCachedBlock::GetProxyIp() const
{
	return( m_uProxyIp );
}

void CWebCachedBlock::OnSuccessfulDownload()
{
	m_bDownloaded = true;
}

void CWebCachedBlock::OnWebCacheBlockRequestSent()
{
	m_bRequested = true;
}

bool CWebCachedBlock::IsValid() const
{
	CPartFile* file = GetFile();
	CUpDownClient* client = theApp.clientlist->FindClientByUserHash( m_UserHash );

	if( !client )
	{
		if (thePrefs.GetLogWebCacheEvents())
		AddDebugLogLine( false, _T("Dropping webcached block received from a unknown client") );
	}
	else if ( !client->IsTrustedOHCBSender() )
	{
		if (thePrefs.GetLogWebCacheEvents())
		AddDebugLogLine( false, _T("Dropping webcached block received from an untrusted client: %s"), client->DbgGetClientInfo() );
	}

	return( file
		&& m_uStart <= m_uEnd
		&& m_uEnd <= file->GetFileSize()
		&& client
		&& client->IsTrustedOHCBSender()
		&& file->IsPureGap( m_uStart, m_uEnd ) );
}

Pending_Block_Struct* CWebCachedBlock::CreatePendingBlock()
{
	Pending_Block_Struct* result = new Pending_Block_Struct;
	result->fRecovered = 0;
	result->fZStreamError = 0;
	result->totalUnzipped = 0;
	result->zStream = 0;
	result->block = new Requested_Block_Struct;
	result->block->StartOffset = m_uStart;
	result->block->EndOffset = m_uEnd;
	result->block->transferred = 0;
	md4cpy( result->block->FileID, m_FileID );
	return result;
}

bool CWebCachedBlock::DownloadIfPossible()
{
	if (thePrefs.GetLogWebCacheEvents())
	AddDebugLogLine( false, _T("CWebCachedBlock::DownloadIfPossible(): blocks on queue: %u"), WebCachedBlockList.GetCount());
	
	if( (SINGLEProxyClient && !SINGLEProxyClient->ProxyClientIsBusy() ) // proxy client exists and is not busy
		|| !SINGLEProxyClient ) // proxy client doesn't exist
	{
		UpdateProxyClient();
		return true;
		}
	else 
		return false;
}

void CWebCachedBlock::UpdateProxyClient()
{
	if (!SINGLEProxyClient)
	{
	if (thePrefs.GetLogWebCacheEvents())	
		AddDebugLogLine( false, _T("Creating new Proxy Client"));
		SINGLEProxyClient = new CWebCacheProxyClient(this);
	}
	else
	{
//		if (thePrefs.GetLogWebCacheEvents())	
//		AddDebugLogLine( false, _T("Proxy Client Updated")); //JP no need to spam the log
		SINGLEProxyClient->UpdateClient(this);
	}

	CStringA strWCRequest;
	CString username;
	if (m_uProxyIp == 0){
		username = "Transparent HTTP Proxy"; // Superlexx - TPS
		SINGLEProxyClient->ResetIP2Country(m_uHostIp); //MORPH Added by SiRoB, IPtoCountry ProxyClient
	}else{
		username.Format( _T("HTTP Proxy @ %s"), ipstr(m_uProxyIp) );
		SINGLEProxyClient->ResetIP2Country(m_uProxyIp); //MORPH Added by SiRoB, IPtoCountry ProxyClient
	}
	SINGLEProxyClient->SetUserName( username );
	SINGLEProxyClient->Crypt.SetRemoteKey( remoteKey ); // Superlexx - encryption
	SINGLEProxyClient->SetRequestFile( GetFile() );

	SINGLEProxyClient->SetDownloadState( DS_DOWNLOADING );
	SINGLEProxyClient->InitTransferredDownMini();
	SINGLEProxyClient->SetDownStartTime();

	theApp.clientlist->AddClient(SINGLEProxyClient);
	theApp.emuledlg->transferwnd->downloadlistctrl.AddSource(SINGLEProxyClient->reqfile,SINGLEProxyClient,false);
	SINGLEProxyClient->reqfile->srclist.AddTail(SINGLEProxyClient);

	OnWebCacheBlockRequestSent();
	if(!SINGLEProxyClient->SendWebCacheBlockRequests())
	//JP remove SINGLEProxyClient from all lists if Sending fails
	{
		theApp.clientlist->RemoveClient(SINGLEProxyClient);
		if( SINGLEProxyClient->reqfile ) {
			theApp.emuledlg->transferwnd->downloadlistctrl.RemoveSource( SINGLEProxyClient, SINGLEProxyClient->reqfile );
			theApp.emuledlg->transferwnd->downloadclientsctrl.RemoveClient(SINGLEProxyClient); // MORPH - Added by SiRoB, DownloadClientsCtrl ProxyClient
			POSITION pos = SINGLEProxyClient->reqfile->srclist.Find(SINGLEProxyClient);
			if( pos )
				SINGLEProxyClient->reqfile->srclist.RemoveAt(pos);
	}
	}
}
