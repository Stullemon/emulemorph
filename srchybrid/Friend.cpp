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
#include "Friend.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
#include "Packets.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


const char CFriend::sm_abyNullHash[16] = {0};

CFriend::CFriend(void)
{
	m_dwLastSeen = 0;
	m_dwLastUsedIP = 0;
	m_nLastUsedPort = 0;
	m_dwLastChatted = 0;
	(void)m_strName;
	m_LinkedClient = 0;
	md4cpy(m_abyUserhash, sm_abyNullHash);
	m_dwHasHash = 0;

	//MORPH START - Added by Yun.SF3, ZZ Upload System
	m_friendSlot = false;
	//MORPH END - Added by Yun.SF3, ZZ Upload System
}

//Added this to work with the IRC.. Probably a better way to do it.. But wanted this in the release..
CFriend::CFriend(const uchar* abyUserhash, uint32 dwLastSeen, uint32 dwLastUsedIP, uint32 nLastUsedPort, 
				 uint32 dwLastChatted, LPCTSTR pszName, uint32 dwHasHash){
	m_dwLastSeen = dwLastSeen;
	m_dwLastUsedIP = dwLastUsedIP;
	m_nLastUsedPort = nLastUsedPort;
	m_dwLastChatted = dwLastChatted;
	if( dwHasHash && abyUserhash){
		md4cpy(m_abyUserhash,abyUserhash);
		m_dwHasHash = md4cmp(m_abyUserhash, sm_abyNullHash) ? 1 : 0;
	}
	else{
		md4cpy(m_abyUserhash, sm_abyNullHash);
		m_dwHasHash = 0;
	}
	m_strName = pszName;
	m_LinkedClient = 0;
	//MORPH START - Added by Yun.SF3, ZZ Upload System
	m_friendSlot = false;
	//MORPH END - Added by Yun.SF3, ZZ Upload System
}

CFriend::CFriend(CUpDownClient* client){
	ASSERT ( client );
	//MORPH START - Modified by SiRoB, ZZ Upload System
	//m_dwLastSeen = time(NULL);
	//m_dwLastUsedIP = client->GetIP();
	//m_nLastUsedPort = client->GetUserPort();
	//m_dwLastChatted = 0;
	//m_strName = client->GetUserName();
	//md4cpy(m_abyUserhash,client->GetUserHash());
	//m_dwHasHash = md4cmp(m_abyUserhash, sm_abyNullHash) ? 1 : 0;
	//m_LinkedClient = client;
	m_dwLastChatted = 0;
	m_LinkedClient = NULL;
	m_friendSlot = false;
	SetLinkedClient(client);
	//MORPH END - Modified by SiRoB, ZZ Upload System
}

CFriend::~CFriend(void)
{
	//MORPH START - Added by SiRoB, ZZ Upload System
	if(m_LinkedClient != NULL) {
 	m_LinkedClient->SetFriendSlot(false);
	m_LinkedClient->m_Friend = NULL;
	m_LinkedClient = NULL;
	}
	//MORPH END - Added by SiRoB, ZZ Upload System
}

void CFriend::LoadFromFile(CFile* file){
	file->Read(m_abyUserhash,16);
	m_dwHasHash = md4cmp(m_abyUserhash, sm_abyNullHash) ? 1 : 0;
	file->Read(&m_dwLastUsedIP,4);
	file->Read(&m_nLastUsedPort,2);
	file->Read(&m_dwLastSeen,4);
	file->Read(&m_dwLastChatted,4);
	uint32 tagcount;
	file->Read(&tagcount,4);
	for (uint32 j = 0; j != tagcount;j++){
		CTag* newtag = new CTag(file);
		switch(newtag->tag.specialtag){
			case FF_NAME:{
				m_strName = newtag->tag.stringvalue;
				break;
			}
			//MORPH START - Added by Yun.SF3, ZZ Upload System
			case FF_FRIENDSLOT: {
				m_friendSlot = (newtag->tag.intvalue == 1) ? true : false;
				break;
			}
			//MORPH END - Added by Yun.SF3, ZZ Upload System
		}	
		delete newtag;
	}
}

void CFriend::WriteToFile(CFile* file){
	if (!m_dwHasHash)
		md4cpy(m_abyUserhash, sm_abyNullHash);
	file->Write(m_abyUserhash,16);
	file->Write(&m_dwLastUsedIP,4);
	file->Write(&m_nLastUsedPort,2);
	file->Write(&m_dwLastSeen,4);
	file->Write(&m_dwLastChatted,4);

	uint32 tagcount = 0;
	if (!m_strName.IsEmpty())
		tagcount++;
//MORPH - Added by Yun.SF3, ZZ Upload System
if(m_LinkedClient !=NULL && m_LinkedClient->GetFriendSlot()  || m_LinkedClient == NULL && m_friendSlot == true) {
		tagcount++;
    }
	//MORPH - Added by Yun.SF3, ZZ Upload System
	file->Write(&tagcount,4);
	if (!m_strName.IsEmpty()){
		CTag nametag(FF_NAME,m_strName.GetBuffer());
		nametag.WriteTagToFile(file);
	}
//MORPH - Added by Yun.SF3, ZZ Upload System
if(m_LinkedClient != NULL && m_LinkedClient->GetFriendSlot() || m_LinkedClient == NULL && m_friendSlot == true) {
		CTag friendslottag(FF_FRIENDSLOT,1);
		friendslottag.WriteTagToFile(file);
    }
}

bool CFriend::HasUserhash() {
    for(int counter = 0; counter < 16; counter++) {
        if(m_abyUserhash[counter] != 0) {
            return true;
}
}

    return false;
}

void CFriend::SetFriendSlot(bool newValue) {
    if(m_LinkedClient != NULL) {
        m_LinkedClient->SetFriendSlot(newValue);
    }

    m_friendSlot = newValue;
}

bool CFriend::GetFriendSlot() {
    if(m_LinkedClient != NULL) {
        return m_LinkedClient->GetFriendSlot();
    } else {
        return m_friendSlot;
    }
}
void CFriend::SetLinkedClient(CUpDownClient* linkedClient) {
    if(linkedClient != NULL) {
        if(m_LinkedClient == NULL) {
            linkedClient->SetFriendSlot(m_friendSlot);
        } else {
            linkedClient->SetFriendSlot(m_LinkedClient->GetFriendSlot());
        }

	    m_dwLastSeen = time(NULL);
	    m_dwLastUsedIP = linkedClient->GetIP();
	    m_nLastUsedPort = linkedClient->GetUserPort();
	    m_strName = linkedClient->GetUserName();
	    md4cpy(m_abyUserhash,linkedClient->GetUserHash());
	    m_dwHasHash = md4cmp(m_abyUserhash, sm_abyNullHash) ? 1 : 0;

        linkedClient->m_Friend = this;
    } else if(m_LinkedClient != NULL) {
        m_friendSlot = m_LinkedClient->GetFriendSlot();
    }

	if(linkedClient != m_LinkedClient) {
		if(m_LinkedClient != NULL) {
			// the old client is no longer friend, since it is no longer the linked client
			m_LinkedClient->SetFriendSlot(false);
			m_LinkedClient->m_Friend = NULL;
		}
		m_LinkedClient = linkedClient;
	}
};
//MORPH END - Modified by SiRoB, Added by Yun.SF3, ZZ Upload System