/*
Copyright (C)2003 Barry Dunne (http://www.emule-project.net)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Note To Mods //
/*
Please do not change anything here and release it..
There is going to be a new forum created just for the Kademlia side of the client..
If you feel there is an error or a way to improve something, please
post it in the forum first and let us look at it.. If it is a real improvement,
it will be added to the offical client.. Changing something without knowing
what all it does can cause great harm to the network if released in mass form..
Any mod that changes anything within the Kademlia side will not be allowed to advertise
there client on the eMule forum..
*/
#pragma once

class CSharedFileList;
struct Status;

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CPrefs;
class CRoutingZone;
class CKademliaUDPListener;
class CKademliaError;
class CSearch;
class CContact;
class CIndexed;
class CEntry;

#define KADEMLIA_VERSION 0.1

typedef void (CALLBACK *KADEMLIA_LOG_CALLBACK)				(LPCSTR lpszMsg);
typedef void (CALLBACK *KADEMLIA_DEBUG_CALLBACK)			(LPCSTR lpszMsg);
typedef void (CALLBACK *KADEMLIA_ERROR_CALLBACK)			(CKademliaError *lpError);
typedef void (CALLBACK *KADEMLIA_SEARCHADD_CALLBACK)		(CSearch *lpSearch);
typedef void (CALLBACK *KADEMLIA_SEARCHREM_CALLBACK)		(CSearch *lpSearch);
typedef void (CALLBACK *KADEMLIA_SEARCHREF_CALLBACK)		(CSearch *lpSearch);
typedef void (CALLBACK *KADEMLIA_CONTACTADD_CALLBACK)		(CContact *lpSearch);
typedef void (CALLBACK *KADEMLIA_CONTACTREM_CALLBACK)		(CContact *lpSearch);
typedef void (CALLBACK *KADEMLIA_CONTACTREF_CALLBACK)		(CContact *lpSearch);
typedef void (CALLBACK *KADEMLIA_INDEXEDADD_CALLBACK)		(CEntry *lpSearch);
typedef void (CALLBACK *KADEMLIA_INDEXEDREM_CALLBACK)		(CEntry *lpSearch);
typedef void (CALLBACK *KADEMLIA_INDEXEDREF_CALLBACK)		(CEntry *lpSearch);
typedef void (CALLBACK *KADEMLIA_REQUESTTCP_CALLBACK)		(CContact *lpContact);
typedef void (CALLBACK *KADEMLIA_UPDATESTATUS_CALLBACK)		(Status *lpContact);
typedef void (CALLBACK *KADEMLIA_OVERHEADSEND_CALLBACK)		(uint32 size);
typedef void (CALLBACK *KADEMLIA_OVERHEADRECV_CALLBACK)		(uint32 size);
/* Eg:
	void CALLBACK myErrHandler(Kademlia::CKademliaError *lpError)
	{
		std::cerr << "Error #" << lpError->m_ErrorCode << std::endl;
	}
	CKademlia::setErrorCallback(myErrHandler);
*/

class CKademlia
{
public:

	static void start(void);
	static void start(CPrefs *prefs);
	static void start(byte *clientID, uint16 tcpPort, uint16 udpPort);
	static void stop(bool bAppShutdown);
 
	static void setLogCallback			(KADEMLIA_LOG_CALLBACK   callback)			{m_logCallback = callback;}
	static void setDebugCallback		(KADEMLIA_DEBUG_CALLBACK callback)			{m_debugCallback = callback;}
	static void setErrorCallback		(KADEMLIA_ERROR_CALLBACK callback)			{m_errorCallback = callback;}
	static void setSearchAddCallback	(KADEMLIA_SEARCHADD_CALLBACK callback)		{m_searchaddCallback = callback;}
	static void setSearchRemCallback	(KADEMLIA_SEARCHREM_CALLBACK callback)		{m_searchremCallback = callback;}
	static void setSearchRefCallback	(KADEMLIA_SEARCHREF_CALLBACK callback)		{m_searchrefCallback = callback;}
	static void setContactAddCallback	(KADEMLIA_CONTACTADD_CALLBACK callback)		{m_contactaddCallback = callback;}
	static void setContactRemCallback	(KADEMLIA_CONTACTREM_CALLBACK callback)		{m_contactremCallback = callback;}
	static void setContactRefCallback	(KADEMLIA_CONTACTREF_CALLBACK callback)		{m_contactrefCallback = callback;}
	static void setRequestTCPCallback	(KADEMLIA_REQUESTTCP_CALLBACK callback)		{m_requesttcpCallback = callback;}
	static void setUpdateStatusCallback	(KADEMLIA_UPDATESTATUS_CALLBACK callback)	{m_updatestatusCallback = callback;}
	static void setOverheadSendCallback	(KADEMLIA_OVERHEADSEND_CALLBACK callback)	{m_overheadsendCallback = callback;}
	static void setOverheadRecvCallback	(KADEMLIA_OVERHEADRECV_CALLBACK callback)	{m_overheadrecvCallback = callback;}

	static void logMsg				(LPCSTR lpMsg, ...);
	static void logLine				(LPCSTR lpMsg);
	static void debugMsg			(LPCSTR lpMsg, ...);
	static void debugLine			(LPCSTR lpLine);
	static void reportError			(const int errorCode, LPCSTR errorDescription, ...);
	static void reportSearchAdd		(CSearch* search);
	static void reportSearchRem		(CSearch* search);
	static void reportSearchRef		(CSearch* search);
	static void reportContactAdd	(CContact* contact);
	static void reportContactRem	(CContact* contact);
	static void reportContactRef	(CContact* contact);
	static void reportRequestTcp	(CContact* contact);
	static void reportUpdateStatus	(::Status* status);
	static void reportOverheadSend	(uint32 size);
	static void reportOverheadRecv	(uint32 size);

	static CPrefs				*getPrefs(void);
	static CRoutingZone			*getRoutingZone(void);
	static CKademliaUDPListener	*getUDPListener(void);
	static CIndexed				*getIndexed(void);
	static ::CSharedFileList	*getSharedFileList(void);
	static void					setSharedFileList(::CSharedFileList *in);

	static void bootstrap(LPCSTR ip, uint16 port);

private:
	CKademlia() {}

	static CKademlia *instance;

	static KADEMLIA_LOG_CALLBACK			m_logCallback;
	static KADEMLIA_DEBUG_CALLBACK			m_debugCallback;
	static KADEMLIA_ERROR_CALLBACK			m_errorCallback;
	static KADEMLIA_SEARCHADD_CALLBACK		m_searchaddCallback;
	static KADEMLIA_SEARCHREM_CALLBACK		m_searchremCallback;
	static KADEMLIA_SEARCHREF_CALLBACK		m_searchrefCallback;
	static KADEMLIA_CONTACTADD_CALLBACK		m_contactaddCallback;
	static KADEMLIA_CONTACTREM_CALLBACK		m_contactremCallback;
	static KADEMLIA_CONTACTREF_CALLBACK		m_contactrefCallback;
	static KADEMLIA_REQUESTTCP_CALLBACK		m_requesttcpCallback;
	static KADEMLIA_UPDATESTATUS_CALLBACK	m_updatestatusCallback;
	static KADEMLIA_OVERHEADSEND_CALLBACK	m_overheadsendCallback;
	static KADEMLIA_OVERHEADRECV_CALLBACK	m_overheadrecvCallback;

	CPrefs					*m_prefs;
	CRoutingZone			*m_routingZone;
	CKademliaUDPListener	*m_udpListener;
	CIndexed				*m_indexed;
	::CSharedFileList		*m_sharedFileList;
};

} // End namespace