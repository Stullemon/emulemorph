//This file is part of the eMule WebCache mod
//http://ispcachingforemule.de.vu
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
#pragma once

#include "HttpClientReqSocket.h"

UINT GetWebCacheSocketUploadTimeout();
UINT GetWebCacheSocketDownloadTimeout();


///////////////////////////////////////////////////////////////////////////////
// CWebCacheSocket

class CWebCacheSocket : public CHttpClientReqSocket
{
	DECLARE_DYNCREATE(CWebCacheSocket)

public:
	virtual CUpDownClient* GetClient() const { return m_client; }
	virtual void SetClient(CUpDownClient* new_client) { m_client = new_client; }
	virtual void Safe_Delete();
// WebCache ////////////////////////////////////////////////////////////////////////////////////
	bool m_bReceivedHttpClose; // 'Connection: close' detector
	virtual void ResetTimeOutTimer();
// WebCache End ////////////////////////////////////////////////////////////////////////////////

protected:
	CWebCacheSocket(CUpDownClient* pClient = NULL);
	virtual ~CWebCacheSocket();
	virtual void DetachFromClient();

	CUpDownClient*	m_client;

	virtual void OnSend(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnError(int nErrorCode);

	virtual bool ProcessHttpResponse();
	virtual bool ProcessHttpResponseBody(const BYTE* pucData, UINT size);
	virtual bool ProcessHttpRequest();
};


///////////////////////////////////////////////////////////////////////////////
// CWebCacheDownSocket

class CWebCacheDownSocket : public CWebCacheSocket
{
	DECLARE_DYNCREATE(CWebCacheDownSocket)

public:
	CWebCacheDownSocket(CUpDownClient* pClient = NULL);
	uint32 blocksloaded;
	virtual void Safe_Delete();

protected:
	bool m_bProxyConnCountFlag;
	virtual ~CWebCacheDownSocket();
	virtual void DetachFromClient();

	virtual void OnConnect(int nErrorCode);
	virtual void OnClose(int nErrorCode);

	virtual bool ProcessHttpResponse();
	virtual bool ProcessHttpResponseBody(const BYTE* pucData, UINT size);
	virtual bool ProcessHttpRequest();
};


///////////////////////////////////////////////////////////////////////////////
// CWebCacheUpSocket

class CWebCacheUpSocket : public CWebCacheSocket
{
	DECLARE_DYNCREATE(CWebCacheUpSocket)

public:
	CWebCacheUpSocket(CUpDownClient* pClient = NULL);
// yonatan http start //////////////////////////////////////////////////////////////////////////
	bool ProcessFirstHttpGet( const char* header, UINT uSize );
// yonatan http end ////////////////////////////////////////////////////////////////////////////

protected:
	virtual ~CWebCacheUpSocket();
	virtual void DetachFromClient();

	virtual void OnSend(int nErrorCode);
	virtual void OnClose(int nErrorCode);

	virtual bool ProcessHttpResponse();
	virtual bool ProcessHttpResponseBody(const BYTE* pucData, UINT size);
	virtual bool ProcessHttpRequest();
};
