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
#include "IrcSocket.h"
#include "emule.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CIrcSocket::CIrcSocket(CIrcMain* pIrcMain) : CAsyncSocketEx()
{
	m_pIrcMain = pIrcMain;
	m_pProxyLayer = NULL;
}

CIrcSocket::~CIrcSocket()
{
	RemoveAllLayers();
}

BOOL CIrcSocket::Create(UINT nSocketPort, int nSocketType, long lEvent, LPCTSTR lpszSocketAddress)
{
	const ProxySettings& proxy = theApp.glob_prefs->GetProxy();
	if (proxy.UseProxy && proxy.type != PROXYTYPE_NOPROXY)
	{
		m_pProxyLayer = new CAsyncProxySocketLayer;
		switch (proxy.type)
		{
			case PROXYTYPE_SOCKS4:
				m_pProxyLayer->SetProxy(PROXYTYPE_SOCKS4, proxy.name, proxy.port);
				break;
			case PROXYTYPE_SOCKS4A:
				m_pProxyLayer->SetProxy(PROXYTYPE_SOCKS4A, proxy.name, proxy.port);
				break;
			case PROXYTYPE_SOCKS5:
				if (proxy.EnablePassword)
					m_pProxyLayer->SetProxy(PROXYTYPE_SOCKS5, proxy.name, proxy.port, proxy.user, proxy.password);
				else
					m_pProxyLayer->SetProxy(PROXYTYPE_SOCKS5, proxy.name, proxy.port);
				break;
			case PROXYTYPE_HTTP11:
				if (proxy.EnablePassword)
					m_pProxyLayer->SetProxy(PROXYTYPE_HTTP11, proxy.name, proxy.port, proxy.user, proxy.password);
				else
					m_pProxyLayer->SetProxy(PROXYTYPE_HTTP11, proxy.name, proxy.port);
				break;
			default:
				ASSERT(0);
		}
		AddLayer(m_pProxyLayer);
	}

	return CAsyncSocketEx::Create(nSocketPort, nSocketType, lEvent, lpszSocketAddress);
}

void CIrcSocket::Connect()
{
	CAsyncSocketEx::Connect(theApp.glob_prefs->GetIRCServer(), 6667);
}

void CIrcSocket::OnReceive(int nErrorCode)
{
	if (nErrorCode){
		AddDebugLogLine(false, _T("IRC socket: Failed to read - %s"), GetErrorMessage(nErrorCode, 1));
		return;
	}

	char buffer[256];
	int length = Receive(buffer, sizeof(buffer)-1);
	if (length < 0){
		AddDebugLogLine(false, _T("IRC socket: Failed to read - %s"), GetErrorMessage(GetLastError(), 1));
		return;
	}
	if (length > 0){
		buffer[length] = '\0';
		theApp.downloadqueue->AddDownDataOverheadOther(length);
		m_pIrcMain->PreParseMessage(buffer);
	}
}

void CIrcSocket::OnConnect(int nErrorCode)
{
	if (nErrorCode){
		AddLogLine(true, _T("IRC socket: Failed to connect - %s"), GetErrorMessage(nErrorCode, 1));
		return;
	}
	m_pIrcMain->SetConnectStatus(true);
	m_pIrcMain->SendLogin();
}

void CIrcSocket::OnClose(int nErrorCode)
{
	if (nErrorCode){
		AddDebugLogLine(false, _T("IRC socket: Failed to close - %s"), GetErrorMessage(nErrorCode, 1));
		return;
	}
	RemoveAllLayers();
	m_pIrcMain->SetConnectStatus(false);
	CAsyncSocketEx::Close();
}

int CIrcSocket::SendString(CString message){
	message += _T("\r\n");
	int size = message.GetLength();
	theApp.uploadqueue->AddUpDataOverheadOther(size);
	return Send(message, size);
}

void CIrcSocket::RemoveAllLayers()
{
	CAsyncSocketEx::RemoveAllLayers();
	
	if (m_pProxyLayer){
		delete m_pProxyLayer;
		m_pProxyLayer = NULL;
	}
}

int CIrcSocket::OnLayerCallback(const CAsyncSocketExLayer* pLayer, int nType, int nParam1, int nParam2)
{
	if (nType == LAYERCALLBACK_LAYERSPECIFIC)
	{
		ASSERT( pLayer );
		if (pLayer == m_pProxyLayer)
		{
			switch (nParam1)
			{
				case PROXYERROR_NOCONN:{
					CString strError(_T("IRC socket: Can't connect to proxy server"));
					CString strErrInf;
					if (nParam2 && GetErrorMessage(nParam2, strErrInf))
						strError += _T(" - ") + strErrInf;
					AddLogLine(true, _T("%s"), strError);
					break;
				}
				case PROXYERROR_REQUESTFAILED:{
					CString strError(_T("IRC socket: Proxy server request failed"));
					if (nParam2){
						strError += _T(" - ");
						strError += (LPCSTR)nParam2;
					}
					AddLogLine(true, _T("%s"), strError);
					break;
				}
				case PROXYERROR_AUTHTYPEUNKNOWN:
					AddLogLine(true, _T("IRC socket: Required authentification type reported by proxy server is unknown or unsupported"));
					break;
				case PROXYERROR_AUTHFAILED:
					AddLogLine(true, _T("IRC socket: Proxy server authentification failed"));
					break;
				case PROXYERROR_AUTHNOLOGON:
					AddLogLine(true, _T("IRC socket: Proxy server requires authentification"));
					break;
				case PROXYERROR_CANTRESOLVEHOST:
					AddLogLine(true, _T("IRC socket: Can't resolve host of proxy server"));
					break;
				default:{
					AddLogLine(true, _T("IRC socket: Proxy server error - %s"), GetProxyError(nParam1));
				}
			}
		}
	}
	return 1;
}