#pragma once

#include "AsyncSocketEx.h"
#include "AsyncProxySocketLayer.h"

class CIrcMain;

class CIrcSocket : public CAsyncSocketEx
{
public:
	CIrcSocket(CIrcMain* pIrcMain);
	virtual ~CIrcSocket();

	BOOL Create(UINT nSocketPort = 0, int nSocketType = SOCK_STREAM,
				long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT |	FD_CONNECT | FD_CLOSE,
				LPCTSTR lpszSocketAddress = NULL );
	void Connect();
	int SendString(CString message);

	virtual void OnConnect(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnClose(int nErrorCode);

	CAsyncProxySocketLayer* m_pProxyLayer;
	virtual void RemoveAllLayers();

protected:
	virtual int	OnLayerCallback(const CAsyncSocketExLayer* pLayer, int nType, int nParam1, int nParam2);

private:
	CIrcMain*	m_pIrcMain;
};
