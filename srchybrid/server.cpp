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
#include "emule.h"
#include "Server.h"
#include "Opcodes.h"
#include "OtherFunctions.h"
#include "Packets.h"
#include "IP2Country.h" //EastShare - added by AndCycle, IP to Country

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


CServer::CServer(const ServerMet_Struct* in_data)
{
	taglist = new CTypedPtrList<CPtrList, CTag*>;
	port = in_data->port;
	tagcount = 0;
	ip = in_data->ip;
	in_addr host;
	host.S_un.S_addr = ip;
	strcpy(ipfull,inet_ntoa(host));
	files = 0;
	users = 0;
	preferences = 0;
	ping = 0;
	description = NULL;
	listname = NULL;
	dynip = 0;
	failedcount = 0; 
	lastpinged = 0;
	lastpingedtime = 0;
	staticservermember=0;
	maxusers=0;
	softfiles = 0;
	hardfiles = 0;
	lastdescpingedcout = 0;
	m_uTCPFlags = 0;
	m_uUDPFlags = 0;

	m_structServerCountry = theApp.ip2country->GetCountryFromIP(ip); //EastShare - added by AndCycle, IP to Country
}

CServer::CServer(uint16 in_port, LPCSTR i_addr)
{
	port = in_port;

	taglist = new CTypedPtrList<CPtrList, CTag*>;
	tagcount = 0;

	if (inet_addr(i_addr) == INADDR_NONE && strcmp(i_addr, "255.255.255.255") != 0){
		dynip = nstrdup(i_addr);
		ip = 0;
	}
	else{
		ip = inet_addr(i_addr);
		dynip = NULL;
	}
	in_addr host;
	host.S_un.S_addr = ip;
	strcpy(ipfull,inet_ntoa(host));
	files = 0;
	users = 0;
	preferences = 0;
	ping = 0;
	description = NULL;
	listname = NULL;
	failedcount = 0; 
	lastpinged = 0;
	lastpingedtime = 0;
	staticservermember=0;
	maxusers=0;
	softfiles = 0;
	hardfiles = 0;
	lastdescpingedcout = 0;
	m_uTCPFlags = 0;
	m_uUDPFlags = 0;

	m_structServerCountry = theApp.ip2country->GetCountryFromIP(ip); //EastShare - added by AndCycle, IP to Country
}

// copy constructor
CServer::CServer(const CServer* pOld)
{
	taglist = new CTypedPtrList<CPtrList, CTag*>;
	for (POSITION pos = pOld->taglist->GetHeadPosition(); pos != NULL; ){
		CTag* pOldTag = pOld->taglist->GetNext(pos);
		taglist->AddTail(pOldTag->CloneTag());
	}
	port = pOld->port;
	ip = pOld->ip; 
	staticservermember=pOld->IsStaticMember();
	tagcount = pOld->tagcount;
	strcpy(ipfull,pOld->ipfull);
	files = pOld->files;
	users = pOld->users;
	preferences = pOld->preferences;
	ping = pOld->ping;
	failedcount = pOld->failedcount; 
	lastpinged = pOld->lastpinged;
	lastpingedtime = pOld->lastpingedtime;
	maxusers = pOld->maxusers;
	softfiles = pOld->softfiles;
	hardfiles = pOld->hardfiles;
	lastdescpingedcout = pOld->lastdescpingedcout;
	if (pOld->description)
		description = nstrdup(pOld->description);
	else
		description = NULL;
	if (pOld->listname)
		listname = nstrdup(pOld->listname);
	else
		listname = NULL;
	if (pOld->dynip)
		dynip = nstrdup(pOld->dynip);
	else
		dynip = NULL;
	m_strVersion = pOld->m_strVersion;
	m_uTCPFlags = pOld->m_uTCPFlags;
	m_uUDPFlags = pOld->m_uUDPFlags;

	m_structServerCountry = theApp.ip2country->GetCountryFromIP(ip); //EastShare - added by AndCycle, IP to Country
}

CServer::~CServer()
{
	delete[] description;
	delete[] listname;
	delete[] dynip;
	for(POSITION pos = taglist->GetHeadPosition(); pos != NULL; )
		delete taglist->GetNext(pos);
	delete taglist;
}

bool CServer::AddTagFromFile(CFile* servermet)
{
	if (servermet == NULL)
		return false;
	CTag* tag = new CTag(servermet);
	switch(tag->tag.specialtag){		
	case ST_SERVERNAME:
		if(tag->tag.stringvalue)
			listname = nstrdup(tag->tag.stringvalue);
		else
			listname = NULL;
		delete tag;
		break;
	case ST_DESCRIPTION:
		if( tag->tag.stringvalue )
			description = nstrdup(tag->tag.stringvalue);
		else
			description = NULL;
		delete tag;
		break;
	case ST_PREFERENCE:
		preferences =tag->tag.intvalue;
		delete tag;
		break;
	case ST_PING:
		ping = tag->tag.intvalue;
		delete tag;
		break;
	case ST_DYNIP:
		if ( tag->tag.stringvalue )
			dynip = nstrdup(tag->tag.stringvalue);
		else
			dynip = NULL;
		delete tag;
		break;
	case ST_FAIL:
		failedcount = tag->tag.intvalue;
		delete tag;
		break;
	case ST_LASTPING:
		lastpingedtime = tag->tag.intvalue;
		delete tag;
		break;
	case ST_MAXUSERS:
		maxusers = tag->tag.intvalue;
		delete tag;
		break;
	case ST_SOFTFILES:
		softfiles = tag->tag.intvalue;
		delete tag;
		break;
	case ST_HARDFILES:
		hardfiles = tag->tag.intvalue;
		delete tag;
		break;
	case ST_VERSION:
		if (tag->tag.type == 2)
			m_strVersion = tag->tag.stringvalue;
		delete tag;
		break;
	case ST_UDPFLAGS:
		if (tag->tag.type == 3)
			m_uUDPFlags = tag->tag.intvalue;
		delete tag;
		break;
	default:
		if (tag->tag.specialtag){
			tag->tag.tagname = nstrdup("Unknown");
			AddTag(tag);
		}
		else if (!strcmp(tag->tag.tagname,"files")){
			files = tag->tag.intvalue;
			delete tag;
		}
		else if (!strcmp(tag->tag.tagname,"users")){
			users = tag->tag.intvalue;
			delete tag;
		}
		else
			AddTag(tag);
	}
	return true;
}

void CServer::SetListName(LPCSTR newname)
{
	if (listname){
		delete[] listname;
		listname = NULL;// needed, in case 'nstrdup' fires an exception!!
	}
	if (newname)
		listname = nstrdup(newname);
}

void CServer::SetDescription(LPCSTR newname)
{
	if (description){
		delete[] description;
		description = NULL;// needed, in case 'nstrdup' fires an exception!!
	}
	if( newname )
		description = nstrdup(newname);
}

LPCSTR CServer::GetAddress() const
{
	if (dynip)
		return dynip;
	else
		return ipfull;
}

void CServer::SetID(uint32 newip)
{
	ip = newip;
	in_addr host;
	host.S_un.S_addr = ip;
	strcpy(ipfull,inet_ntoa(host));
	m_structServerCountry = theApp.ip2country->GetCountryFromIP(ip); //EastShare - added by AndCycle, IP to Country
}

void CServer::SetDynIP(LPCSTR newdynip)
{
	if (dynip){
		delete[] dynip;
		dynip = NULL;// needed, in case 'nstrdup' fires an exception!!
	}
	if( newdynip )
		dynip = nstrdup(newdynip);
}

void CServer::SetLastDescPingedCount(bool bReset)
{
	if (bReset)
		lastdescpingedcout = 0;
	else
		lastdescpingedcout++;
}

//EastShare Start - added by AndCycle, IP to Country
CString CServer::GetCountryName() const{
	CString tempStr;

	if(theApp.ip2country->IsIP2Country() == false) return "";

	switch(theApp.glob_prefs->GetIP2CountryNameMode()){
		case IP2CountryName_SHORT:
			tempStr.Format("<%s>",m_structServerCountry->ShortCountryName);
			return tempStr;
		case IP2CountryName_MID:
			tempStr.Format("<%s>",m_structServerCountry->MidCountryName);
			return tempStr;
		case IP2CountryName_LONG:
			tempStr.Format("<%s>",m_structServerCountry->LongCountryName);
			return tempStr;
	}
	return "";
}

int CServer::GetCountryFlagIndex() const{
	return m_structServerCountry->FlagIndex;
}

void CServer::ResetIP2Country(){
	m_structServerCountry = theApp.ip2country->GetCountryFromIP(ip);
}
//EastShare End - added by AndCycle, IP to Country