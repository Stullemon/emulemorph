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

#include "stdafx.h"
#include "Kademlia.h"
#include "Prefs.h"
#include "Error.h"
#include "SearchManager.h"
#include "Indexed.h"
#include "../net/KademliaUDPListener.h"
#include "../routing/RoutingZone.h"
#include "../utils/MiscUtils.h"
#include "../../sharedfilelist.h"
#include "../routing/contact.h"
#include "../routing/timer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

CKademlia *CKademlia::instance = NULL;

KADEMLIA_LOG_CALLBACK			CKademlia::m_logCallback = NULL;
KADEMLIA_DEBUG_CALLBACK			CKademlia::m_debugCallback = NULL;
KADEMLIA_ERROR_CALLBACK			CKademlia::m_errorCallback = NULL;
KADEMLIA_SEARCHADD_CALLBACK		CKademlia::m_searchaddCallback = NULL;
KADEMLIA_SEARCHREM_CALLBACK		CKademlia::m_searchremCallback = NULL;
KADEMLIA_SEARCHREF_CALLBACK		CKademlia::m_searchrefCallback = NULL;
KADEMLIA_CONTACTADD_CALLBACK	CKademlia::m_contactaddCallback = NULL;
KADEMLIA_CONTACTREM_CALLBACK	CKademlia::m_contactremCallback = NULL;
KADEMLIA_CONTACTREF_CALLBACK	CKademlia::m_contactrefCallback = NULL;
KADEMLIA_REQUESTTCP_CALLBACK	CKademlia::m_requesttcpCallback = NULL;
KADEMLIA_UPDATESTATUS_CALLBACK	CKademlia::m_updatestatusCallback = NULL;
KADEMLIA_OVERHEADSEND_CALLBACK	CKademlia::m_overheadsendCallback = NULL;
KADEMLIA_OVERHEADRECV_CALLBACK	CKademlia::m_overheadrecvCallback = NULL;

void CKademlia::start(void)
{
	if (instance != NULL)
	{
		return;
	}
	start(new CPrefs());
//	Kademlia::CKademlia::getRoutingZone()->selfTest();

}

void CKademlia::start(byte *clientID, uint16 tcpPort, uint16 udpPort)
{
	if (instance != NULL)
	{
		return;
	}
	start(new CPrefs(clientID, tcpPort, udpPort));
}

void CKademlia::start(CPrefs *prefs)
{
	try
	{
		debugMsg("Starting Kademlia");
		srand((UINT)time(NULL));
		instance = new CKademlia();	
		instance->m_prefs = prefs;
		instance->m_udpListener = NULL;
		instance->m_routingZone = NULL;
		instance->m_indexed = new CIndexed();
		instance->m_routingZone = new CRoutingZone();
		instance->m_udpListener = new CKademliaUDPListener();
		int result = instance->m_udpListener->start(instance->m_prefs->getUDPPort());
		if (result != ERR_SUCCESS)
		{
			reportError(ERR_TCP_LISTENER_START_FAILURE, "Could not listen on udp port %ld", instance->m_prefs->getUDPPort());
		}

		CTimer::start();
	}
	catch (CException *e)
	{
		char err[512];
		e->GetErrorMessage(err, 512);
		reportError(ERR_UNKNOWN, err);
		e->Delete();
	}
}

void CKademlia::stop(bool bAppShutdown)
{
	debugMsg("Stopping Kademlia");

	CTimer::stop(bAppShutdown);
	CSearchManager::stopAllSearches();
	try
	{
		instance->m_udpListener->stop(bAppShutdown);
		delete instance->m_udpListener;
		instance->m_udpListener = NULL;
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::stop(1)");
		ASSERT(0);
	}

	try
	{
		delete instance->m_routingZone;
		instance->m_routingZone = NULL;
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::stop(2)");
		ASSERT(0);
	}

	try
	{
		delete instance->m_prefs;
		instance->m_prefs = NULL;
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::stop(3)");
		ASSERT(0);
	}

	try
	{
		delete instance->m_indexed;
		instance->m_indexed = NULL;
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::stop(4)");
		ASSERT(0);
	}

	try
	{
		delete instance;
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::stop(5)");
		ASSERT(0);
	}

	instance = NULL;
}

void CKademlia::logMsg(LPCSTR lpMsg, ...)
{
	try
	{
		if ( instance == NULL || m_logCallback == NULL )
		{
			return;
		}
		CString msg;
		va_list args;
		va_start(args, lpMsg);
		msg.FormatV(lpMsg, args);
		va_end(args);
		(*m_logCallback)(msg.GetBuffer(0));
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::logMsg");
		return;
	}
}

void CKademlia::logLine(LPCSTR lpLine)
{
	try
	{
		if ( instance == NULL || m_logCallback == NULL )
		{
			return;
		}
		(*m_logCallback)(lpLine);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::logLine");
		return;
	}
}

void CKademlia::debugMsg(LPCSTR lpMsg, ...)
{
	try
	{
		if ( instance == NULL || m_debugCallback == NULL )
		{
			return;
		}
		CString msg;
		va_list args;
		va_start(args, lpMsg);
		msg.FormatV(lpMsg, args);
		va_end(args);
		(*m_debugCallback)(msg.GetBuffer(0));
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::debugMsg");
		return;
	}
}

void CKademlia::debugLine(LPCSTR lpLine)
{
	try
	{
		if ( instance == NULL || m_debugCallback == NULL )
		{
			return;
		}
		(*m_debugCallback)(lpLine);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::debugLine");
		return;
	}
}
void CKademlia::reportError(const int errorCode, LPCSTR errorDescription, ...)
{
	try
	{
		if ( instance == NULL || m_errorCallback == NULL )
		{
			return;
		}
		CString msg;
		va_list args;
		va_start(args, errorDescription);
		msg.FormatV(errorDescription, args);
		va_end(args);
		CKademliaError error(errorCode, msg.GetBuffer(0));
		(*m_errorCallback)(&error);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::reportError");
		return;
	}
}

void CKademlia::reportSearchAdd(CSearch* search)
{
	try
	{
		if ( instance == NULL || m_searchaddCallback == NULL  )
		{
			return;
		}
		(*m_searchaddCallback)(search);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::reportSearchAdd");
		return;
	}
}

void CKademlia::reportSearchRem(CSearch* search)
{
	try
	{
		if ( instance == NULL || m_searchremCallback == NULL )
		{
			return;
		}
		(*m_searchremCallback)(search);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::reportSearchRem");
		return;
	}
}

void CKademlia::reportSearchRef(CSearch* search)
{
	try
	{
		if ( instance == NULL || m_searchrefCallback == NULL )
		{
			return;
		}
		(*m_searchrefCallback)(search);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::reportSearchRef");
		return;
	}
}

void CKademlia::reportContactAdd(CContact* contact)
{
	try
	{
		if ( instance == NULL || m_contactaddCallback == NULL )
		{
			return;
		}
		(*m_contactaddCallback)(contact);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::reportContactAdd");
		return;
	}
}

void CKademlia::reportContactRem(CContact* contact)
{
	try
	{
		if ( instance == NULL || m_contactremCallback == NULL )
		{
			return;
		}
		(*m_contactremCallback)(contact);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::reportContactRem");
		return;
	}
}

void CKademlia::reportContactRef(CContact* contact)
{
	try
	{
		if ( instance == NULL || m_contactrefCallback == NULL )
		{
			return;
		}
		(*m_contactrefCallback)(contact);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::reportContactRef");
		return;
	}
}

void CKademlia::reportRequestTcp(CContact* contact)
{
	try
	{
		if ( instance == NULL || m_requesttcpCallback == NULL )
		{
			delete contact;
			contact = NULL;
			return;
		}
		(*m_requesttcpCallback)(contact);
		//Contact was originally deleted in the callback method. This may have caused
		//Messaging issues.. So, we now make sure and delete it in the Kad thread.
		delete contact;
		contact = NULL;
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::reportRequestTcp");
		if(contact)
		{
			delete contact;
			contact = NULL;
		}
		return;
	}
}

void CKademlia::reportUpdateStatus(::Status* status)
{
	try
	{
		if ( instance == NULL || m_updatestatusCallback == NULL )
		{
			delete status;
			return;
		}
		(*m_updatestatusCallback)(status);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::reportUpdateStatus");
		return;
	}
}

void CKademlia::reportOverheadSend(uint32 size)
{
	try
	{
		if ( instance == NULL || m_overheadsendCallback == 0 )
		{
			return;
		}
		(m_overheadsendCallback)(size);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::reportOverheadSend");
		return;
	}
}

void CKademlia::reportOverheadRecv(uint32 size)
{
	try
	{
		if ( instance == NULL || m_overheadrecvCallback == 0 )
		{
			return;
		}
		(m_overheadrecvCallback)(size);
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::reportOverheadRecv");
		return;
	}
}

CPrefs *CKademlia::getPrefs(void)
{
	try
	{
		if (instance == NULL || instance->m_prefs == NULL)
		{
			return NULL;
		}
	}
	catch(...)
	{ 
		CKademlia::debugLine("Exception in CKademlia::getPrefs");
		return NULL; 
	}
	return instance->m_prefs;
}

CKademliaUDPListener *CKademlia::getUDPListener(void)
{
	try
	{
		if (instance == NULL || instance->m_udpListener == NULL)
		{
			return NULL;
		}
	}
	catch(...)
	{ 
		CKademlia::debugLine("Exception in CKademlia::getUDPListener");
		return NULL; 
	}
	return instance->m_udpListener;
}

UINT CKademlia::getUDPListenerThreadID()
{
	UINT uThreadID = 0;
	try
	{
		if (instance != NULL && instance->m_udpListener != NULL)
			uThreadID = instance->m_udpListener->getThreadID();
	}
	catch(...)
	{ 
		CKademlia::debugLine("Exception in " __FUNCTION__);
		return NULL; 
	}
	return uThreadID;
}

CRoutingZone *CKademlia::getRoutingZone(void)
{
	try
	{
		if (instance == NULL || instance->m_routingZone == NULL)
		{
			return NULL;
		}
	}
	catch(...)
	{ 
		CKademlia::debugLine("Exception in CKademlia::reportRoutingZone");
		return NULL; 
	}
	return instance->m_routingZone;
}

::CSharedFileList *CKademlia::getSharedFileList(void)
{
	try
	{
		if (instance == NULL || instance->m_sharedFileList == NULL)
		{
			return NULL;
		}
	}
	catch(...)
	{ 
		CKademlia::debugLine("Exception in CKademlia::getSharedFileList");
		return NULL; 
	}
	return instance->m_sharedFileList;
}

CIndexed *CKademlia::getIndexed(void)
{
	try
	{
		if ( instance == NULL || instance->m_indexed == NULL)
		{
			return NULL;
		}
	}
	catch(...)
	{ 
		CKademlia::debugLine("Exception in CKademlia::getIndexed");
		return NULL; 
	}
	return instance->m_indexed;
}

void CKademlia::setSharedFileList(::CSharedFileList *in)
{
	try
	{
		if (instance != NULL)
		{
			instance->m_sharedFileList = in;
		}
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::setSgaredFileList");
	}
}

void CKademlia::bootstrap(LPCTSTR ip, uint16 port)
{
	try
	{
		if (instance != NULL)
		{
			instance->m_udpListener->bootstrap(ip, port);
		}
	}
	catch(...)
	{
		CKademlia::debugLine("Exception in CKademlia::bootstrap");
	}
}