//this file is part of eMule
//Copyright (C)2002 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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
#include "resource.h"
#include "ED2KLink.h"
#include "OtherFunctions.h"
#include "SafeFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


namespace {
	struct autoFree {
		autoFree(TCHAR* p) : m_p(p) {}
		~autoFree() { free(m_p); }
	private:
		TCHAR * m_p;
	};
	inline unsigned int FromHexDigit(TCHAR digit) {
		switch (digit) {
		case _T('0'): return 0;
		case _T('1'): return 1;
		case _T('2'): return 2;
		case _T('3'): return 3;
		case _T('4'): return 4;
		case _T('5'): return 5;
		case _T('6'): return 6;
		case _T('7'): return 7;
		case _T('8'): return 8;
		case _T('9'): return 9;
		case _T('A'): return 10;
		case _T('B'): return 11;
		case _T('C'): return 12;
		case _T('D'): return 13;
		case _T('E'): return 14;
		case _T('F'): return 15;
		case _T('a'): return 10;
		case _T('b'): return 11;
		case _T('c'): return 12;
		case _T('d'): return 13;
		case _T('e'): return 14;
		case _T('f'): return 15;
		default: throw GetResString(IDS_ERR_ILLFORMEDHASH);
		}
	}
}

CED2KLink::~CED2KLink()
{
}

///////////////////////////////////////////// 
// CED2KServerListLink implementation 
///////////////////////////////////////////// 
CED2KServerListLink::CED2KServerListLink(const TCHAR* address)
{
	m_address = address;
}

CED2KServerListLink::~CED2KServerListLink()
{
} 

void
CED2KServerListLink::GetLink(CString& lnk)
{
	lnk = _T("ed2k://|serverlist|");
	lnk += m_address;
	lnk += _T("|/");
}

CED2KServerListLink*
CED2KServerListLink::GetServerListLink()
{
	return this;
}

CED2KServerLink*
CED2KServerListLink::GetServerLink()
{
	return 0;
}

CED2KFileLink*
CED2KServerListLink::GetFileLink()
{
	return 0;
}

CED2KLink::LinkType
CED2KServerListLink::GetKind() const
{
	return kServerList;
}

/////////////////////////////////////////////
// CED2KServerLink implementation
/////////////////////////////////////////////
CED2KServerLink::CED2KServerLink(const TCHAR* ip,const TCHAR* port)
{
	m_ip = inet_addr(ip);
	unsigned long ul = _tcstoul(port,0,10);
	if ( ul > 0xFFFF )
		throw GetResString(IDS_ERR_BADPORT);
	m_port = static_cast<uint16>(ul);
	m_defaultName = "Server ";
	m_defaultName += ip;
	m_defaultName += ":";
	m_defaultName += port;
}

CED2KServerLink::~CED2KServerLink()
{
}


void 
CED2KServerLink::GetLink(CString& lnk)
{
	in_addr adr;
	char buffer[32];
	lnk = _T("ed2k://|server|");
	adr.S_un.S_addr = m_ip;
	lnk += inet_ntoa(adr);
	lnk += _T("|");
	sprintf(buffer,"%d",static_cast<int>(m_port));
	lnk += buffer;
	lnk += _T("|/");
}

CED2KServerListLink*
CED2KServerLink::GetServerListLink() 
{ 
	return 0; 
}

CED2KServerLink* 
CED2KServerLink::GetServerLink() 
{ 
	return this; 
}

CED2KFileLink* 
CED2KServerLink::GetFileLink() 
{ 
	return 0; 
}

CED2KLink::LinkType 
CED2KServerLink::GetKind() const
{
	return kServer;
}


/////////////////////////////////////////////
// CED2KFileLink implementation
/////////////////////////////////////////////
CED2KFileLink::CED2KFileLink(const TCHAR* name,const TCHAR* size, const TCHAR* hash, const TCHAR* sources)
: m_name(name)
, m_size(size)
{
	if ( _tcslen(hash) != 32 )
		throw GetResString(IDS_ERR_ILLFORMEDHASH);

	if (_atoi64(size)>=4294967295)
			throw GetResString(IDS_ERR_TOOLARGEFILE);
	if (_atoi64(size)<=0)
			throw GetResString(IDS_ERR_NOTAFILELINK);
	

	for ( int idx = 0 ; idx < 16 ; ++idx) {
		m_hash[idx] = FromHexDigit(*hash++)*16;
		m_hash[idx] += FromHexDigit(*hash++);
	}
	SourcesList=NULL;

	if (sources){

		TCHAR* pNewString = _tcsdup(sources);
		autoFree liberator(pNewString);
		TCHAR* pCh = pNewString;
		TCHAR* pEnd;
		TCHAR* pIP;
		TCHAR* pPort;

		bool bAllowSources;
		TCHAR date[3];
		COleDateTime expirationDate;
		int nYear,nMonth,nDay;

		uint16 nCount = 0;
		uint32 dwID;
		uint16 nPort;
		uint32 dwServerIP = 0; 
		uint16 nServerPort = 0;
		unsigned long ul;

		int nInvalid = 0;

		pCh = _tcsstr( pCh, _T("sources") );
		if( pCh != NULL ) {
			pCh = pCh + 7; // point to char after "sources"
			pEnd = pCh;
			while( *pEnd ) pEnd++; // make pEnd point to the terminating NULL
			bAllowSources=true;
			// if there's an expiration date...
			if( *pCh == _T('@') && (pEnd-pCh) > 7 )
			{
				pCh++; // after '@'
				date[2] = 0; // terminate the two character string
				date[0] = *(pCh++); date[1] = *(pCh++);
				nYear = _tcstol( date, 0, 10 ) + 2000;
				date[0] = *(pCh++); date[1] = *(pCh++);
				nMonth = _tcstol( date, 0, 10 );
				date[0] = *(pCh++); date[1] = *(pCh++);
				nDay = _tcstol( date, 0, 10 );
				bAllowSources = ( expirationDate.SetDate(nYear,nMonth,nDay) == 0 );
				if (bAllowSources) bAllowSources=(COleDateTime::GetCurrentTime() < expirationDate);
			}

			// increment pCh to point to the first "ip:port" and check for sources
			if ( bAllowSources && ++pCh < pEnd ) {
				SourcesList = new CSafeMemFile(256);
				SourcesList->WriteUInt16(nCount); // init to 0, we'll fix this at the end.
				// for each "ip:port" source string until the end
				// limit to prevent overflow (uint16 due to CPartFile::AddClientSources)
				while( *pCh != 0 && nCount < MAXSHORT ) {
					pIP = pCh;
					// find the end of this ip:port string & start of next ip:port string.
					if( (pCh = _tcschr(pCh, _T(','))) != NULL ) {
						*pCh = 0; // terminate current "ip:port"
						pCh++; // point to next "ip:port"
					}
					else
						pCh = pEnd;

					// if port is not present for this ip, go to the next ip.
					if( (pPort = _tcschr(pIP, _T(':'))) == NULL )
					{	nInvalid++;	continue;	}

					*pPort = 0;	// terminate ip string
					pPort++;	// point pPort to port string.

					dwID = inet_addr( pIP );
					ul = _tcstoul( pPort, 0, 10 );
					nPort = static_cast<uint16>(ul);

					// skip bad ips / ports
					// SLUGFILLER: hostnameSources - progressive filtering
					if (ul > 0xFFFF || ul == 0 )	// port
					{	nInvalid++;	continue;	}
					if( dwID == INADDR_NONE) {	// hostname?
						if (strlen(pIP) > 512)
						{	nInvalid++;	continue;	}
						SUnresolvedHostname* hostname = new SUnresolvedHostname;
						hostname->nPort = nPort;
						hostname->strHostname = pIP;
						m_HostnameSourcesList.AddTail(hostname);
						continue;
					}
					if( IsLowIDED2K(dwID) )	// ip
					{	nInvalid++;	continue;	}
					// SLUGFILLER: hostnameSources

					SourcesList->WriteUInt32(dwID);
					SourcesList->WriteUInt16(nPort);
					SourcesList->WriteUInt32(dwServerIP);
					SourcesList->WriteUInt16(nServerPort);
					nCount++;
				}
				SourcesList->SeekToBegin();
				SourcesList->WriteUInt16(nCount);
				SourcesList->SeekToBegin();
				if (nCount==0) {
					delete SourcesList;
					SourcesList=NULL;
				}
			}
		}
	}
}


CED2KFileLink::~CED2KFileLink()
{
	if (SourcesList){
		delete SourcesList;
		SourcesList=NULL;
	}
	// SLUGFILLER: hostnameSources
	while (!m_HostnameSourcesList.IsEmpty())
		delete m_HostnameSourcesList.RemoveHead();
	// SLUGFILLER: hostnameSources
}

void 
CED2KFileLink::GetLink(CString& lnk)
{
	lnk = _T("ed2k://|file|");
	lnk += m_name;
	lnk += _T("|");
	lnk += m_size;
	lnk += _T("|");
	for (int idx=0; idx != 16 ; ++idx ) {
		unsigned int ui1 = m_hash[idx] / 16;
		unsigned int ui2 = m_hash[idx] % 16;
		lnk+= static_cast<TCHAR>( ui1 > 9 ? (_T('0')+ui1) : (_T('A')+(ui1-10)) );
		lnk+= static_cast<TCHAR>( ui2 > 9 ? (_T('0')+ui2) : (_T('A')+(ui2-10)) );
	}
	lnk += _T("|/");
}

CED2KServerListLink*
CED2KFileLink::GetServerListLink() 
{ 
	return 0; 
}

CED2KServerLink* 
CED2KFileLink::GetServerLink() 
{ 
	return 0; 
}
CED2KFileLink* 
CED2KFileLink::GetFileLink() 
{ 
	m_name=URLDecode(m_name);
	return this; 
}

CED2KLink::LinkType 
CED2KFileLink::GetKind() const
{
	return kFile;
}


//static 
CED2KLink* 
CED2KLink::CreateLinkFromUrl( const TCHAR * uri)
{
	// Parse pseudo-URI
	const TCHAR* pChArray[7];
	if (uri==0) 
		throw CString(_T("null ed2k link"));
	TCHAR* pNewString = _tcsdup(uri);
	autoFree liberator(pNewString);
	TCHAR* pCh = pNewString;
	const TCHAR* pStart = pCh;
	int idx = 0;
	for (idx=0;idx<7;idx++) pChArray[idx]=NULL;
	idx = 0;
	while( idx <7 && ((pCh = _tcschr(pCh,_T('|'))) !=0) ) {
		pChArray[idx++] = pStart;
		*pCh = 0;
		++ pCh;
		pStart = pCh;
	}
	if ( *pStart != _T('/') ) {
		throw GetResString(IDS_ERR_BADED2KLINK);
	}
	if (   idx < 3
		|| pChArray[0] == 0 
		|| pChArray[1] == 0 
		|| pChArray[2] == 0 
//		|| pChArray[3] == 0 // This was preventing ed2k serverlist links from working.. 
		|| _tcscmp( _T("ed2k://") , pChArray[0]  ) != 0 
	   ) {
			throw GetResString(IDS_ERR_BADED2KLINK);
	}
	if ( _tcscmp( _T("file") , pChArray[1]  ) == 0 && idx >=  5 && pChArray[4] != 0 ) {
		return new CED2KFileLink(pChArray[2],pChArray[3],pChArray[4],pChArray[6]);
	}
	else if ( _tcscmp( _T("serverlist") , pChArray[1] ) == 0 && idx == 3 ) {
		return new CED2KServerListLink(pChArray[2]);
	}
	else if ( _tcscmp( _T("server") , pChArray[1]  ) == 0 && idx == 4 ) {
		return new CED2KServerLink(pChArray[2],pChArray[3]);
	}
	else {
		throw GetResString(IDS_ERR_NOSLLINK);
	}
	return 0;
}
