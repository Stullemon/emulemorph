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
#include <zlib/zlib.h>
#include "Packets.h"
#include "OtherFunctions.h"
#include "SafeFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#pragma pack(1)
struct Header_Struct{
	uint8	eDonkeyID;
	uint32	packetlength;
	uint8	command;
};
#pragma pack()

#pragma pack(1)
struct UDP_Header_Struct{
	uint8	eDonkeyID;
	uint8	command;
};
#pragma pack()


Packet::Packet(uint8 protocol){
	m_bSplitted = false;
	m_bLastSplitted = false;
	m_bFromPF = false;
	size = 0;
	pBuffer = 0;
	completebuffer = 0;
	tempbuffer = 0;
	opcode = 0x00;
	prot = protocol;
	m_bPacked = false;
}

Packet::Packet(char* header){
	m_bSplitted = false;
	m_bPacked = false;
	m_bLastSplitted = false;
	m_bFromPF = false;
	tempbuffer = 0;
	pBuffer = 0;
	completebuffer = 0;
	Header_Struct* head = (Header_Struct*) header;
	size = head->packetlength-1;
	opcode = head->command;
	prot = head->eDonkeyID;
}

Packet::Packet(char* pPacketPart, uint32 nSize, bool bLast, bool bFromPartFile){// only used for splitted packets!
	m_bFromPF = bFromPartFile;
	m_bSplitted = true;
	m_bPacked = false;
	m_bLastSplitted = bLast;
	tempbuffer = 0;
	pBuffer = 0;
	completebuffer = pPacketPart;
	size = nSize-6;
	opcode = 0x00;
	prot = 0x00;
}

Packet::Packet(uint8 in_opcode, uint32 in_size, uint8 protocol, bool bFromPartFile){
	m_bFromPF = bFromPartFile;
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

Packet::Packet(CMemFile* datafile, uint8 protocol, uint8 ucOpcode){
	m_bSplitted = false;
	m_bPacked = false;
	m_bLastSplitted = false;
	m_bFromPF = false;
	size = datafile->GetLength();
	completebuffer = new char[datafile->GetLength()+10];
	pBuffer = completebuffer+6;
	BYTE* tmp = datafile->Detach();
	memcpy(pBuffer,tmp,size);
	free(tmp);
	tempbuffer = 0;
	opcode = ucOpcode;
	prot = protocol;
}

Packet::Packet(const CStringA& str, uint8 ucProtocol, uint8 ucOpcode){
	m_bSplitted = false;
	m_bPacked = false;
	m_bLastSplitted = false;
	m_bFromPF = false;
	size = str.GetLength();
	completebuffer = new char[size+10];
	pBuffer = completebuffer+6;
	memcpy(pBuffer,(LPCSTR)str,size);
	tempbuffer = 0;
	opcode = ucOpcode;
	prot = ucProtocol;
}

Packet::~Packet(){
	if (completebuffer)
		delete[] completebuffer;
	else
		delete [] pBuffer;
	delete[] tempbuffer;
}

char* Packet::GetPacket(){
	if (completebuffer){
		if (!m_bSplitted)
			memcpy(completebuffer,GetHeader(),6);
		return completebuffer;
	}
	else{
		if (tempbuffer){
			delete[] tempbuffer;
			tempbuffer = NULL; // 'new' may throw an exception
		}
		tempbuffer = new char[size+10];
		memcpy(tempbuffer,GetHeader(),6);
		memcpy(tempbuffer+6,pBuffer,size);
		return tempbuffer;
	}
}

char* Packet::DetachPacket(){
	if (completebuffer){
		if (!m_bSplitted)
			memcpy(completebuffer,GetHeader(),6);
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
		memcpy(tempbuffer,GetHeader(),6);
		memcpy(tempbuffer+6,pBuffer,size);
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
	if( prot == OP_KADEMLIAHEADER )
		prot = OP_KADEMLIAPACKEDPROT;
	else
		prot = OP_PACKEDPROT;
	memcpy(pBuffer,output,newsize);
	size = newsize;
	delete[] output;
	m_bPacked = true;
}

bool Packet::UnPackPacket(UINT uMaxDecompressedSize){
	ASSERT ( prot == OP_PACKEDPROT || prot == OP_KADEMLIAPACKEDPROT); 
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
		if( prot == OP_KADEMLIAPACKEDPROT )
			prot = OP_KADEMLIAHEADER;
		else
			prot =  OP_EMULEPROT;
		return true;
	}
	delete[] unpack;
	return false;
}


///////////////////////////////////////////////////////////////////////////////
// CRawPacket

CRawPacket::CRawPacket(const CStringA& rstr)
{
	ASSERT( opcode == 0 );
	ASSERT( !m_bSplitted );
	ASSERT( !m_bLastSplitted );
	ASSERT( !m_bPacked );
	ASSERT( !m_bFromPF );
	ASSERT( completebuffer == NULL );
	ASSERT( tempbuffer == NULL );

	prot = 0x00;
	size = rstr.GetLength();
	pBuffer = new char[size];
	memcpy(pBuffer, (LPCSTR)rstr, size);
}

CRawPacket::CRawPacket(const char* pcData, UINT uSize, bool bFromPartFile)
{
	ASSERT( opcode == 0 );
	ASSERT( !m_bSplitted );
	ASSERT( !m_bLastSplitted );
	ASSERT( !m_bPacked );
	ASSERT( !m_bFromPF );
	ASSERT( completebuffer == NULL );
	ASSERT( tempbuffer == NULL );

	prot = 0x00;
	size = uSize;
	pBuffer = new char[size];
	memcpy(pBuffer, pcData, size);
	m_bFromPF = bFromPartFile;
}

CRawPacket::~CRawPacket()
{
	ASSERT( completebuffer == NULL );
}

char* CRawPacket::GetHeader()
{
	ASSERT(0);
	return NULL;
}

char* CRawPacket::GetUDPHeader()
{
	ASSERT(0);
	return NULL;
}

void CRawPacket::AttachPacket(char* pPacketData, UINT uPacketSize, bool bFromPartFile)
{
	ASSERT( pBuffer == NULL );
	pBuffer = pPacketData;
	size = uPacketSize;
	m_bFromPF = bFromPartFile;
}

char* CRawPacket::DetachPacket()
{
	char* pResult = pBuffer;
	pBuffer = NULL;
	return pResult;
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
	if (in.type == TAGTYPE_STRING)
		stringvalue = in.stringvalue!=NULL ? nstrdup(in.stringvalue) : NULL;
	else if (in.type == TAGTYPE_UINT32)
		intvalue = in.intvalue;
	else if (in.type == TAGTYPE_FLOAT32)
		floatvalue = in.floatvalue;
	else{
		ASSERT(0);
		intvalue = 0;
	}

	specialtag = in.specialtag;
}

STag::~STag()
{
	delete[] tagname;
	if (type == TAGTYPE_STRING)
		delete[] stringvalue;
	if (type == TAGTYPE_HASH)
		delete[] pData;
}


///////////////////////////////////////////////////////////////////////////////
// CTag

CTag::CTag(LPCSTR name, uint32 intvalue)
{
	tag.type = TAGTYPE_UINT32;
	tag.tagname = nstrdup(name);
	tag.intvalue = intvalue;
}

CTag::CTag(uint8 special, uint32 intvalue)
{
	tag.type = TAGTYPE_UINT32;
	tag.specialtag = special;
	tag.intvalue = intvalue;
}

CTag::CTag(LPCSTR name, LPCSTR strvalue)
{
	tag.type = TAGTYPE_STRING;
	tag.tagname = nstrdup(name);
	tag.stringvalue = nstrdup(strvalue);
}

CTag::CTag(uint8 special, LPCSTR strvalue)
{
	tag.type = TAGTYPE_STRING;
	tag.specialtag = special;
	tag.stringvalue = nstrdup(strvalue);
}

CTag::CTag(uint8 uName, const BYTE* pucHash)
{
	tag.type = TAGTYPE_HASH;
	tag.specialtag = uName;
	tag.pData = new BYTE[16];
	md4cpy(tag.pData, pucHash);
}

CTag::CTag(const STag& in_tag)
	: tag(in_tag)
{
}

CTag::CTag(CFileDataIO* data)
{
	tag.type = data->ReadUInt8();
	if (tag.type & 0x80)
	{
		tag.type &= 0x7F;
		tag.specialtag = data->ReadUInt8();
	}
	else
	{
		UINT length = data->ReadUInt16();
		if (length == 1)
			tag.specialtag = data->ReadUInt8();
		else
		{
			tag.tagname = new char[length+1];
			data->Read(tag.tagname,length);
			tag.tagname[length] = '\0';
		}
	}

	// NOTE: It's very important that we read the *entire* packet data, even if we do
	// not use each tag. Otherwise we will get troubles when the packets are returned in 
	// a list - like the search results from a server.
	if (tag.type == TAGTYPE_STRING)
	{
		UINT length = data->ReadUInt16();
		tag.stringvalue = new char[length+1];
		data->Read(tag.stringvalue, length);
		tag.stringvalue[length] = '\0';
	}
	else if (tag.type == TAGTYPE_UINT32)
	{
		tag.intvalue = data->ReadUInt32();
	}
	else if (tag.type == TAGTYPE_UINT16)
	{
		tag.intvalue = data->ReadUInt16();
		tag.type = TAGTYPE_UINT32;
	}
	else if (tag.type == TAGTYPE_UINT8)
	{
		tag.intvalue = data->ReadUInt8();
		tag.type = TAGTYPE_UINT32;
	}
	else if (tag.type == TAGTYPE_FLOAT32)
	{
		data->Read(&tag.floatvalue, 4);
	}
	else if (tag.type >= TAGTYPE_STR1 && tag.type <= TAGTYPE_STR16)
	{
		UINT length = tag.type - TAGTYPE_STR1 + 1;
		tag.stringvalue = new char[length+1];
		data->Read(tag.stringvalue, length);
		tag.stringvalue[length] = '\0';
		tag.type = TAGTYPE_STRING;
	}
	else if (tag.type == TAGTYPE_HASH)
	{
		tag.pData = new BYTE[16];
		data->Read(tag.pData, 16);
	}
	else if (tag.type == TAGTYPE_BOOL)
	{
		TRACE("***NOTE: %s; Reading BOOL tag\n", __FUNCTION__);
		data->Seek(1, CFile::current);
	}
	else if (tag.type == TAGTYPE_BOOLARRAY)
	{
		TRACE("***NOTE: %s; Reading BOOL Array tag\n", __FUNCTION__);
		uint16 len;
		data->Read(&len, 2);
		// 07-Apr-2004: eMule versions prior to 0.42e.29 used the formula "(len+7)/8"!
		data->Seek((len/8)+1, CFile::current);
	}
	else if (tag.type == TAGTYPE_BLOB) // (never seen; <len> <byte>)
	{
		TRACE("***NOTE: %s; Reading BLOB tag\n", __FUNCTION__);
		uint32 len;
		// 07-Apr-2004: eMule versions prior to 0.42e.29 handled the "len" as int16!
		data->Read(&len,4);
		data->Seek(len, CFile::current);
	}
	else
	{
		if (tag.specialtag != 0)
			TRACE("%s; Unknown tag: type=0x%02X  specialtag=%u\n", __FUNCTION__, tag.type, tag.specialtag);
		else
			TRACE("%s; Unknown tag: type=0x%02X  name=\"%s\"\n", __FUNCTION__, tag.type, tag.tagname);
	}
}

CTag::~CTag()
{
}

bool CTag::WriteNewEd2kTag(CFileDataIO* data) const
{
	ASSERT( tag.type != 0 );

	// Write tag type
	UINT uTagStrValLen = 0;
	uint8 uTagType;
	if (IsInt())
	{
		if (tag.intvalue <= 0xFF)
			uTagType = TAGTYPE_UINT8;
		else if (tag.intvalue <= 0xFFFF)
			uTagType = TAGTYPE_UINT16;
		else
			uTagType = TAGTYPE_UINT32;
	}
	else if (IsStr())
	{
		uTagStrValLen = strlen(tag.stringvalue);
		if (uTagStrValLen >= 1 && uTagStrValLen <= 16)
			uTagType = TAGTYPE_STR1 + uTagStrValLen - 1;
		else
			uTagType = TAGTYPE_STRING;
	}
	else
		uTagType = tag.type;

	// Write tag name
	if (tag.tagname)
	{
		data->WriteUInt8(uTagType);
		UINT uTagNameLen = strlen(tag.tagname);
		data->WriteUInt16(uTagNameLen);
		data->Write(tag.tagname, uTagNameLen);
	}
	else
	{
		ASSERT( tag.specialtag != 0 );
		data->WriteUInt8(uTagType | 0x80);
		data->WriteUInt8(tag.specialtag);
	}

	// Write tag data
	if (uTagType == TAGTYPE_STRING)
	{
		data->WriteUInt16(uTagStrValLen);
		data->Write(tag.stringvalue, uTagStrValLen);
	}
	else if (uTagType >= TAGTYPE_STR1 && uTagType <= TAGTYPE_STR16)
	{
		data->Write(tag.stringvalue, uTagStrValLen);
	}
	else if (uTagType == TAGTYPE_UINT32)
	{
		data->WriteUInt32(tag.intvalue);
	}
	else if (uTagType == TAGTYPE_UINT16)
	{
		data->WriteUInt16(tag.intvalue);
	}
	else if (uTagType == TAGTYPE_UINT8)
	{
		data->WriteUInt8(tag.intvalue);
	}
	else if (uTagType == TAGTYPE_FLOAT32)
	{
		data->Write(&tag.floatvalue, 4);
	}
	else if (uTagType == TAGTYPE_HASH)
	{
		data->WriteHash16(tag.pData);
	}
	else
	{
		TRACE("%s; Unknown tag: type=0x%02X\n", __FUNCTION__, uTagType);
		ASSERT(0);
		return false;
	}

	return true;
}

bool CTag::WriteTagToFile(CFileDataIO* file) const
{
	// don't write tags of unknown types, we wouldn't be able to read them in again 
	// and the met file would be corrupted
	if (tag.type == TAGTYPE_STRING || tag.type == TAGTYPE_UINT32 || tag.type == TAGTYPE_FLOAT32)
	{
		file->WriteUInt8(tag.type);
		
		if (tag.tagname)
		{
			UINT taglen = strlen(tag.tagname);
			file->WriteUInt16(taglen);
			file->Write(tag.tagname, taglen);
		}
		else
		{
			file->WriteUInt16(1);
			file->WriteUInt8(tag.specialtag);
		}

		if (tag.type == TAGTYPE_STRING)
		{
			UINT len = strlen(tag.stringvalue);
			file->WriteUInt16(len);
			file->Write(tag.stringvalue,len);
		}
		else if (tag.type == TAGTYPE_UINT32)
			file->WriteUInt32(tag.intvalue);
		else if (tag.type == TAGTYPE_FLOAT32)
			file->Write(&tag.floatvalue, 4);
		//TODO: Support more tag types
		else
		{
			TRACE("%s; Unknown tag: type=0x%02X\n", __FUNCTION__, tag.type);
			ASSERT(0);
			return false;
		}
		return true;
	}
	else
	{
		TRACE("%s; Ignored tag with unknown type=0x%02X\n", __FUNCTION__, tag.type);
		ASSERT(0);
		return false;
	}
}

CString CTag::GetFullInfo() const
{
	CString strTag;
	if (tag.tagname)
	{
		strTag = _T('\"');
		strTag += tag.tagname;
		strTag += _T('\"');
	}
	else
	{
		strTag.Format(_T("0x%02X"), tag.specialtag);
	}
	strTag += _T("=");
	if (tag.type == TAGTYPE_STRING)
	{
		strTag += _T("\"");
		strTag += tag.stringvalue;
		strTag += _T("\"");
	}
	else if (tag.type >= TAGTYPE_STR1 && tag.type <= TAGTYPE_STR16)
	{
		strTag.AppendFormat(_T("(Str%u)\"%s\""), tag.type - TAGTYPE_STR1 + 1, tag.stringvalue);
	}
	else if (tag.type == TAGTYPE_UINT32)
	{
		strTag.AppendFormat(_T("(Int32)%u"), tag.intvalue);
	}
	else if (tag.type == TAGTYPE_UINT16)
	{
		strTag.AppendFormat(_T("(Int16)%u"), tag.intvalue);
	}
	else if (tag.type == TAGTYPE_UINT8)
	{
		strTag.AppendFormat(_T("(Int8)%u"), tag.intvalue);
	}
	else if (tag.type == TAGTYPE_FLOAT32)
	{
		strTag.AppendFormat(_T("(Float32)%f"), tag.floatvalue);
	}
	else
	{
		strTag.AppendFormat(_T("Type=%u"), tag.type);
	}
	return strTag;
}

CString CTag::GetStr() const
{
	CString str;
	if (IsStr() && tag.stringvalue != NULL)
	{
			str = tag.stringvalue;
	}
	return str;
}
