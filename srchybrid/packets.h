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
#pragma once
#include "opcodes.h"

class CFileDataIO;

class Packet
{
public:
	Packet(uint8 protocol = OP_EDONKEYPROT);
	Packet(char* header); // only used for receiving packets
	Packet(CMemFile* datafile, uint8 protocol = OP_EDONKEYPROT, uint8 ucOpcode = 0x00);
	Packet(const CStringA& str, uint8 ucProtocol, uint8 ucOpcode);
	Packet(uint8 in_opcode, uint32 in_size, uint8 protocol = OP_EDONKEYPROT, bool bFromPartFile = true);
	Packet(char* pPacketPart,uint32 nSize,bool bLast,bool bFromPartFile = true); // only used for splitted packets!
	virtual ~Packet();

	virtual char* GetHeader();
	virtual char* GetUDPHeader();
	virtual char* GetPacket();
	virtual char* DetachPacket();
	virtual uint32 GetRealPacketSize() const	{return size+6;}
//	bool	IsSplitted()			{return m_bSplitted;}
//	bool	IsLastSplitted()		{return m_bLastSplitted;}
	bool	IsFromPF() const					{return m_bFromPF;}
	void	PackPacket();
	bool	UnPackPacket(UINT uMaxDecompressedSize = 50000);
	char*	pBuffer;
	uint32	size;
	uint8	opcode;
	uint8	prot;

protected:
	bool	m_bSplitted;
	bool	m_bLastSplitted;
	bool	m_bPacked;
	bool	m_bFromPF;
	char*	completebuffer;
	char*	tempbuffer;
	char	head[6];
};

class CRawPacket : public Packet
{
public:
	CRawPacket(const CStringA& rstr);
	CRawPacket(const char* pcData, UINT uSize, bool bFromPartFile = false);
	virtual ~CRawPacket();

	virtual char*	GetHeader();
	virtual char*	GetUDPHeader();
	virtual char*	GetPacket()					{return pBuffer; }
	virtual void	AttachPacket(char* pcData, UINT uSize, bool bFromPartFile = false);
	virtual char*	DetachPacket();
	virtual uint32	GetRealPacketSize() const	{return size;}
};

struct STag{
	STag();
	STag(const STag& in);
	~STag();

	uint8	type;
	uint8	specialtag;
	LPSTR	tagname;
	union{
	  LPSTR	stringvalue;
	  uint32 intvalue;
	  float floatvalue;
	  BYTE* pData;
	};
};

class CTag {
public:
	CTag(LPCSTR name, uint32 intvalue);
	CTag(uint8 special, uint32 intvalue);
	CTag(LPCSTR name, LPCSTR strvalue);
	CTag(uint8 special, LPCSTR strvalue);
	CTag(uint8 uName, const BYTE* pucHash);
	CTag(const STag& in_tag);
	CTag(CFileDataIO* in_data);
	~CTag();
	
	bool IsStr() const { return tag.type == TAGTYPE_STRING; }
	bool IsInt() const { return tag.type == TAGTYPE_UINT32; }
	bool IsFloat() const { return tag.type == TAGTYPE_FLOAT32; }
	bool IsHash() const { return tag.type == TAGTYPE_HASH; }
	
	UINT GetInt() const { return tag.intvalue; }
	CString GetStr() const;
	const BYTE* GetHash() const { return tag.pData; }
	
	CTag* CloneTag() { return new CTag(tag); }
	
	bool WriteTagToFile(CFileDataIO* file) const;		// old eD2K tags
	bool WriteNewEd2kTag(CFileDataIO* file) const;	// new eD2K tags
	
	STag tag;
	CString GetFullInfo() const;
};

__inline int CmpED2KTagName(LPCSTR pszTagName1, LPCSTR pszTagName2){
	// string compare is independant from any codepage and/or LC_CTYPE setting.
	return __ascii_stricmp(pszTagName1, pszTagName2);
}
void ConvertED2KTag(CTag*& pTag);
