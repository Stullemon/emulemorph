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
#include "otherfunctions.h"
#include "packets.h"
#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


Packet::Packet(uint8 protocol){
	m_bSplitted = false;
	m_bLastSplitted = false;
	size = 0;
	pBuffer = 0;
	completebuffer = 0;
	tempbuffer = 0;
	prot = protocol;
	m_bPacked = false;
}

Packet::Packet(char* header){
	m_bSplitted = false;
	m_bPacked = false;
	m_bLastSplitted = false;
	tempbuffer = 0;
	pBuffer = 0;
	completebuffer = 0;
	Header_Struct* head = (Header_Struct*) header;
	size = head->packetlength-1;
	opcode = head->command;
	prot = head->eDonkeyID;
}

//MOPRH START - Removed by SiRoB, ZZ Upload System 20030807-1911
//// -khaos--+++> Slightly modified for our stats uses...
//Packet::Packet(char* pPacketPart, uint32 nSize ,bool bLast, bool bFromPF){// only used for splitted packets!
//	m_bFromPF = bFromPF;
//	m_bSplitted = true;
//	m_bPacked = false;
//	m_bLastSplitted = bLast;
//	tempbuffer = 0;
//	pBuffer = 0;
//	completebuffer = pPacketPart;
//	size = nSize-6;
//}
//MOPRH END   - Removed by SiRoB, ZZ Upload System 20030807-1911

// -khaos--+++> Slightly modified for our stats uses...
//				If m_bFromPF = true then packet was formed from a partfile
//				If m_bFromPF = false then this packet was formed from a complete shared file.
Packet::Packet(int8 in_opcode,uint32 in_size,uint8 protocol,bool bFromPF){
	m_bFromPF = bFromPF;
	// <-----khaos-
	m_bSplitted = false;
	m_bPacked = false;
	m_bLastSplitted = false;
	tempbuffer = 0;
	if (in_size){
		completebuffer = new char[in_size+10];
		pBuffer = completebuffer+6;
		memset(completebuffer,0,in_size+10);
	}
	else{
		pBuffer = 0;
		completebuffer = 0;
	}
	opcode = in_opcode;
	size = in_size;
	prot = protocol;
}

Packet::Packet(CMemFile* datafile,uint8 protocol){
	m_bSplitted = false;
	m_bPacked = false;
	m_bLastSplitted = false;
	size = datafile->GetLength();
	completebuffer = new char[datafile->GetLength()+10];
	pBuffer = completebuffer+6;
	BYTE* tmp = datafile->Detach();;
	MEMCOPY(pBuffer,tmp,size);
	free(tmp);
	tempbuffer = 0;
	prot = protocol;
}

Packet::~Packet(){
	if (completebuffer)
		delete[] completebuffer;
	else if (pBuffer)
		delete [] pBuffer;
	if (tempbuffer)
		delete[] tempbuffer;
}

char* Packet::GetPacket(){
	if (completebuffer){
		if (!m_bSplitted)
			MEMCOPY(completebuffer,GetHeader(),6);
		return completebuffer;
	}
	else{
		if (tempbuffer){
			delete[] tempbuffer;
			tempbuffer = NULL; // 'new' may throw an exception
		}
		tempbuffer = new char[size+10];
		MEMCOPY(tempbuffer,GetHeader(),6);
		MEMCOPY(tempbuffer+6,pBuffer,size);

		return tempbuffer;
	}
}

char* Packet::DetachPacket(){
	if (completebuffer){
		if (!m_bSplitted)
			MEMCOPY(completebuffer,GetHeader(),6);
		char* result = completebuffer;
		completebuffer = 0;
		pBuffer = 0;
		return result;
	}
	else{
		if (tempbuffer){
			delete[] tempbuffer;
			tempbuffer = NULL; // 'new' may throw an exception
		}
		tempbuffer = new char[size+10];
		MEMCOPY(tempbuffer,GetHeader(),6);
		MEMCOPY(tempbuffer+6,pBuffer,size);
		char* result = tempbuffer;
		tempbuffer = 0;
		return result;
	}
}

char* Packet::GetHeader(){
	ASSERT ( !m_bSplitted );
	Header_Struct* header = (Header_Struct*) head;
	header->command = opcode;
	header->eDonkeyID =  prot;
	header->packetlength = size+1;
	return head;
}

char* Packet::GetUDPHeader(){
	ASSERT ( !m_bSplitted );
	memset(head,0,6);
	UDP_Header_Struct* header = (UDP_Header_Struct*) head;
	header->command = opcode;
	header->eDonkeyID =  prot;
	return head;
}

void Packet::PackPacket(){
	ASSERT (!m_bSplitted);
	uLongf newsize = size+300;
	BYTE* output = new BYTE[newsize];
	uint16 result = compress2(output,&newsize,(BYTE*)pBuffer,size,Z_BEST_COMPRESSION);
	if (result != Z_OK || size <= newsize){
		delete[] output;
		return;
	}
	prot = OP_PACKEDPROT;
	MEMCOPY(pBuffer,output,newsize);
	//MORPH START - Added by IceCream, Hotfix by MKThunderStorm about the source exchange compression
	size = newsize;
	//MORPH END   - Added by IceCream, Hotfix by MKThunderStorm about the source exchange compression
	delete[] output;
	m_bPacked = true;
}

bool Packet::UnPackPacket(UINT uMaxDecompressedSize){
	ASSERT ( prot == OP_PACKEDPROT); 
	uint32 nNewSize = size*10+300;
	if (nNewSize > uMaxDecompressedSize){
		//ASSERT(0);
		nNewSize = uMaxDecompressedSize;
	}
	BYTE* unpack = new BYTE[nNewSize];
	uLongf unpackedsize = nNewSize;
	uint16 result = uncompress(unpack,&unpackedsize,(BYTE*)pBuffer,size);
	if (result == Z_OK){
		ASSERT ( completebuffer == NULL );
		ASSERT ( pBuffer != NULL );
		size = unpackedsize;
		delete[] pBuffer;
		pBuffer = (char*)unpack;
		prot =  OP_EMULEPROT;
		return true;				
	}
	delete[] unpack;
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// STag

STag::STag()
{
	type = 0;
	tagname = NULL;
	stringvalue = NULL;
	intvalue = 0;
	specialtag = 0;
}

STag::STag(const STag& in)
{
	type = in.type;
	tagname = in.tagname!=NULL ? nstrdup(in.tagname) : NULL;
	if (in.type == 2)
		stringvalue = in.stringvalue!=NULL ? nstrdup(in.stringvalue) : NULL;
	else if (in.type == 3)
		intvalue = in.intvalue;
	else if (in.type == 4)
		floatvalue = in.floatvalue;
	else{
		ASSERT(0);
		intvalue = 0;
	}

	specialtag = in.specialtag;
}

STag::~STag()
{
	if (tagname)
		delete[] tagname;
	if (type == 2 && stringvalue)
		delete[] stringvalue;
}


///////////////////////////////////////////////////////////////////////////////
// CTag

CTag::CTag(LPCSTR name,uint32 intvalue){
	tag.tagname = nstrdup(name);
	tag.type = 3;
	tag.intvalue = intvalue;
}

CTag::CTag(int8 special, uint32 intvalue){
	tag.type = 3;
	tag.intvalue = intvalue;
	tag.specialtag = special;
}

CTag::CTag(LPCSTR name,LPCSTR strvalue){
	tag.tagname = nstrdup(name);
	tag.type = 2;
	tag.stringvalue = nstrdup(strvalue);
}

CTag::CTag(int8 special, LPCSTR strvalue){
	tag.type = 2;
	tag.stringvalue = nstrdup(strvalue);
	tag.specialtag = special;
}

CTag::CTag(const STag& in_tag)
	: tag(in_tag)
{
}

CTag::CTag(CFile* in_data)
{
	in_data->Read(&tag.type,1);
	uint16 length;
	in_data->Read(&length,2);
	if (length == 1)
		in_data->Read(&tag.specialtag,1);
	else {
		tag.tagname = new char[length+1];
		in_data->Read(tag.tagname,length);
		tag.tagname[length] = 0;
	}

	// NOTE: It's very important that we read the *entire* packet data, even if we do
	// not use each tag. Otherwise we will get troubles when the packets are returned in 
	// a list - like the search results from a server.
	if (tag.type == 2){ // STRING
		in_data->Read(&length,2);
		tag.stringvalue = new char[length+1];
		in_data->Read(tag.stringvalue,length);
		tag.stringvalue[length] = 0;
	}
	else if (tag.type == 3){ // DWORD
		in_data->Read(&tag.intvalue,4);
	}
	else if (tag.type == 4){ // FLOAT (used by Hybrid 0.48)
		in_data->Read(&tag.floatvalue,4);
	}
	else if (tag.type == 1){ // HASH (never seen)
		TRACE("CTag::CTag(CFile*); Reading *unverified* HASH tag\n");
		in_data->Seek(16, CFile::current);
	}
	else if (tag.type == 5){ // BOOL (never seen; propably 1 bit)
		// NOTE: This is preventive code, it was never tested
		TRACE("CTag::CTag(CFile*); Reading *unverified* BOOL tag\n");
		in_data->Seek(1, CFile::current);
	}
	else if (tag.type == 6){ // BOOL Array (never seen; propably <numbits> <bits>)
		// NOTE: This is preventive code, it was never tested
		TRACE("CTag::CTag(CFile*); Reading *unverified* BOOL Array tag\n");
		uint16 len;
		in_data->Read(&len,2);
		in_data->Seek((len+7)/8, CFile::current);
	}
	else if (tag.type == 7){ // BLOB (never seen; propably <len> <byte>)
		// NOTE: This is preventive code, it was never tested
		TRACE("CTag::CTag(CFile*); Reading *unverified* BLOB tag\n");
		uint16 len;
		in_data->Read(&len,2);
		in_data->Seek(len, CFile::current);
	}
	else{
		if (length == 1)
			TRACE("CTag::CTag(CFile*); Unknown tag: type=0x%02X  specialtag=%u\n", tag.type, tag.specialtag);
		else
			TRACE("CTag::CTag(CFile*); Unknown tag: type=0x%02X  name=\"%s\"\n", tag.type, tag.tagname);
	}
}

CTag::~CTag(){
}

bool CTag::WriteTagToFile(CFile* file)
{
	// don't write tags of unknown types, we wouldn't be able to read them in again 
	// and the met file would be corrupted
	if (tag.type==2 || tag.type==3 || tag.type==4){
		file->Write(&tag.type,1);
		
		if (tag.tagname){
			uint16 taglen= (uint16)strlen(tag.tagname);
			file->Write(&taglen,2);
			file->Write(tag.tagname,taglen);
		}
		else{
			uint16 taglen = 1;
			file->Write(&taglen,2);
			file->Write(&tag.specialtag,taglen);
		}

		if (tag.type == 2){
			uint16 len = (uint16)strlen(tag.stringvalue);
			file->Write(&len,2);
			file->Write(tag.stringvalue,len);
		}
		else if (tag.type == 3)
			file->Write(&tag.intvalue,4);
		else if (tag.type == 4)
			file->Write(&tag.floatvalue,4);
		//TODO: Support more tag types
		else{
			TRACE("CTag::WriteTagToFile(CFile*); Unknown tag: type=0x%02X\n", tag.type);
			ASSERT(0);
			return false;
		}
		return true;
	}
	else{
		TRACE("CTag::WriteTagToFile(CFile*); Ignored tag with unknown type=0x%02X\n", tag.type);
		ASSERT(0);
		return false;
	}
}

CString CTag::GetFullInfo() const
{
	CString strTag;
	if (tag.tagname){
		strTag = _T('\"');
		strTag += tag.tagname;
		strTag += _T('\"');
	}
	else{
		strTag.Format(_T("0x%02X"), tag.specialtag);
	}
	strTag += _T("=");
	if (tag.type == 2){
		strTag += _T("\"");
		strTag += tag.stringvalue;
		strTag += _T("\"");
	}
	else if (tag.type == 3){
		TCHAR szBuff[16];
		_itot(tag.intvalue, szBuff, 10);
		strTag += szBuff;
	}
	else if (tag.type == 4){
		TCHAR szBuff[16];
		_sntprintf(szBuff, ELEMENT_COUNT(szBuff), _T("%f"), tag.floatvalue);
		strTag += szBuff;
	}
	else{
		CString strBuff;
		strBuff.Format(_T("Type=%u"), tag.type);
		strTag += strBuff;
	}
	return strTag;
}
