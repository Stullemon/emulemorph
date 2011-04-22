/*CAsyncProxySocketLayer by Tim Kosse (Tim.Kosse@gmx.de)
                 Version 1.6 (2003-03-26)
--------------------------------------------------------

Introduction:
-------------

This class is layer class for CAsyncSocketEx. With this class you
can connect through SOCKS4/5 and HTTP 1.1 proxies. This class works
as semi-transparent layer between CAsyncSocketEx and the actual socket.
This class is used in FileZilla, a powerful open-source FTP client.
It can be found under http://sourceforge.net/projects/filezilla
For more information about SOCKS4/5 goto
http://www.socks.nec.com/socksprot.html
For more information about HTTP 1.1 goto http://www.rfc-editor.org
and search for RFC2616

How to use?
-----------

You don't have to change much in you already existing code to use
CAsyncProxySocketLayer.

To use it, create an instance of CAsyncProxySocketLayer, call SetProxy
and attach it to a CAsyncSocketEx instance.

You have to process OnLayerCallback in you CAsyncSocketEx instance as it will
receive all layer nofications.

The following notifications are sent:

//Error codes
PROXYERROR_NOERROR			0
PROXYERROR_NOCONN			1 //Can't connect to proxy server, use GetLastError for more information
PROXYERROR_REQUESTFAILED	2 //Request failed, can't send data
PROXYERROR_AUTHREQUIRED		3 //Authentication required
PROXYERROR_AUTHTYPEUNKNOWN	4 //Authtype unknown or not supported
PROXYERROR_AUTHFAILED		5 //Authentication failed
PROXYERROR_AUTHNOLOGON		6
PROXYERROR_CANTRESOLVEHOST	7

//Status messages
PROXYSTATUS_LISTENSOCKETCREATED 8	//Called when a listen socket was created successfully. Unlike the normal listen function,
									//a socksified socket has to connect to the proxy to negotiate the details with the server
									//on which the listen socket will be created
									//The two parameters will contain the ip and port of the listen socket on the server.

If you want to use CAsyncProxySocketLayer to create a listen socket, you
have to use this overloaded function:
BOOL PrepareListen(unsigned long serverIp);
serverIP is the IP of the server you are already connected
through the SOCKS proxy. You can't use listen sockets over a
SOCKS proxy without a primary connection. Listen sockets are only
supported by SOCKS proxies, this won't work with HTTP proxies.
When the listen socket is created successfully, the PROXYSTATUS_LISTENSOCKETCREATED
notification is sent. The parameters  will tell you the ip and the port of the listen socket.
After it you have to handle the OnAccept message and accept the
connection.
Be carful when calling Accept: rConnected socket will NOT be filled! Instead use the instance which created the
listen socket, it will handle the data connection.
If you want to accept more than one connection, you have to create a listing socket for each of them!

Description of important functions and their parameters:
--------------------------------------------------------

void SetProxy(int nProxyType);
void SetProxy(int nProxyType, const char * ProxyHost, int nProxyPort);
void SetProxy(int nProxyType, const char *, int nProxyPort, const char * ProxyUser, const char * ProxyPass);

Call one of this functions to set the proxy type.
Parametes:
- nProxyType specifies the Proxy Type.
- ProxyHost and nProxyPort specify the address of the proxy
- ProxyUser and ProxyPass are only available for SOCKS5 proxies.

supported proxy types:
PROXYTYPE_NOPROXY
PROXYTYPE_SOCKS4
PROXYTYPE_SOCKS4A
PROXYTYPE_SOCKS5
PROXYTYPE_HTTP11

There are also some other functions:

GetProxyPeerName
Like GetPeerName of CAsyncSocket, but returns the address of the
server connected through the proxy.	If using proxies, GetPeerName
only returns the address of the proxy.

int GetProxyType();
Returns the used proxy

const int GetLastProxyError() const;
Returns the last proxy error

License
-------

Feel free to use this class, as long as you don't claim that you wrote it
and this copyright notice stays intact in the source files.
If you use this class in commercial applications, please send a short message
to tim.kosse@gmx.de


Version history
---------------

- 1.6 got rid of MFC
- 1.5 released CAsyncSocketExLayer version
- 1.4 added Unicode support
- 1.3 added basic HTTP1.1 authentication
      fixed memory leak in SOCKS5 code
	  OnSocksOperationFailed will be called after Socket has been closed
      fixed some minor bugs
- 1.2 renamed into CAsyncProxySocketLayer
      added HTTP1.1 proxy support
- 1.1 fixes all known bugs, mostly with SOCKS5 authentication
- 1.0 initial release
*/
#pragma once
#include "AsyncSocketExLayer.h"

class CAsyncProxySocketLayer : public CAsyncSocketExLayer
{
// Attribute
public:

// Operationen
public:
	CAsyncProxySocketLayer();
	virtual ~CAsyncProxySocketLayer();

// �berschreibungen
public:
	virtual void Close();
	virtual BOOL Connect(LPCSTR lpHostAddress, UINT nHostPort);
	virtual BOOL Connect(const SOCKADDR* lpSockAddr, int nSockAddrLen);
	virtual BOOL Listen(int nConnectionBacklog);

	void SetProxy(int nProxyType); //Only PROXYTYPE_NOPROXY
	void SetProxy(int nProxyType, const CStringA& strProxyHost, int ProxyPort); //May not be PROXYTYPE_NOPROXY
	void SetProxy(int nProxyType, const CStringA& strProxyHost, int ProxyPort, const CStringA& strProxyUser, const CStringA& strProxyPass); //Only SOCKS5 and HTTP1.1 proxies
	//Sets the proxy details.
	//nProxyType - Type of the proxy. May be PROXYTYPE_NONE, PROXYTYPE_SOCKS4, PROXYTYPE_SOCKS5 or PROXYTYPE_HTTP11
	//ProxyHost - The address of the proxy. Can be either IP or URL
	//ProxyPort - The port of the proxy
	//ProxyUser - the username for SOCKS5 proxies
	//ProxyPass - the password for SOCKS5 proxies

	//Prepare listen
	BOOL PrepareListen(ULONG ip);

	//Returns the type of the proxy
	int GetProxyType() const;

#ifdef _AFX
	virtual BOOL GetPeerName(CString& rPeerAddress, UINT& rPeerPort);
#endif
	virtual BOOL GetPeerName(SOCKADDR* lpSockAddr, int* lpSockAddrLen);

// Implementierung
protected:
	virtual BOOL Accept(CAsyncSocketEx& rConnectedSocket, SOCKADDR* lpSockAddr = NULL, int* lpSockAddrLen = NULL);
	virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);
	virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);

	//Notification event handlers
	virtual void OnAccept(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);

private:
	void Reset();
	void ClearBuffer();		//Clears the receive buffer

	char *m_pRecvBuffer;	//The receive buffer
	int m_nRecvBufferLen;	//Length of the RecvBuffer
	int m_nRecvBufferPos;	//Position within the receive buffer

	char *m_pStrBuffer;		//Recvbuffer needed by HTTP1.1 proxy
	int m_iStrBuffSize;

	int m_nProxyOpState;	//State of an operation
	int m_nProxyOpID;		//Currently active operation (0 if none)

	u_short m_nProxyPeerPort;//Port of the server you are connected to, retrieve via GetPeerName
	u_long m_nProxyPeerIP;	//IP of the server you are connected to, retrieve via GetPeerName
	typedef struct
	{
		int			nProxyType;
		CStringA	strProxyHost;
		int			nProxyPort;
		CStringA	strProxyUser;
		CStringA	strProxyPass;
		BOOL		bUseLogon;
	} t_proxydata;			//This structure will be used to hold the proxy details
	t_proxydata m_ProxyData;//Structure to hold the data set by SetProxy
	LPSTR m_pProxyPeerHost;//The host connected to
};

//Errorcodes
#define PROXYERROR_NOERROR			0
#define PROXYERROR_NOCONN			1 //Can't connect to proxy server, use GetLastError for more information
#define PROXYERROR_REQUESTFAILED	2 //Request failed, can't send data
#define PROXYERROR_AUTHREQUIRED		3 //Authentication required
#define PROXYERROR_AUTHTYPEUNKNOWN	4 //Authtype unknown or not supported
#define PROXYERROR_AUTHFAILED		5 //Authentication failed
#define PROXYERROR_AUTHNOLOGON		6
#define PROXYERROR_CANTRESOLVEHOST	7

//Status messages
#define PROXYSTATUS_LISTENSOCKETCREATED 8	//Called when a listen socket was created successfully. Unlike the normal listen function,
											//a socksified socket has to connect to the proxy to negotiate the details with the server
											//on which the listen socket will be created
											//The two parameters will contain the ip and port of the listen socket on the server.

CString GetProxyError(UINT nError);

struct t_ListenSocketCreatedStruct
{
	unsigned long ip;
	UINT nPort;
};

//Proxytypes
#define PROXYTYPE_NOPROXY	0
#define PROXYTYPE_SOCKS4	1
#define PROXYTYPE_SOCKS4A	2
#define PROXYTYPE_SOCKS5	3
#define PROXYTYPE_HTTP10	4
#define PROXYTYPE_HTTP11	5

#define PROXYOP_CONNECT	1
#define PROXYOP_BIND	2
