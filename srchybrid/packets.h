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

class Packet
{
public:
	Packet(uint8 protocol = OP_EDONKEYPROT);
	Packet(char* header); // only used for receiving packets
	//khaos Packet(char* pPacketPart,uint32 nSize,bool bLast); // only used for splitted packets!
	Packet(CMemFile* datafile,uint8 protocol = OP_EDONKEYPROT);
	// -khaos--+++> For use in upload statistics... Optional var, shouldn't affect anything.
	Packet(int8 in_opcode,int32 in_size,uint8 protocol = OP_EDONKEYPROT,bool bFromPF = true);
	Packet(char* pPacketPart,uint32 nSize,bool bLast,bool bFromPF = true); // only used for splitted packets!
	// <-----khaos-
	~Packet();
	char*	GetHeader();
	char*	GetUDPHeader();
	char*	GetPacket();
	char*	DetachPacket();
	uint32	GetRealPacketSize()		{return size+6;}
	bool	IsSplitted()			{return m_bSplitted;}
	bool	IsLastSplitted()		{return m_bLastSplitted;}
	void	PackPacket();
	bool	UnPackPacket(UINT uMaxDecompressedSize = 50000);
	char*	pBuffer;
	uint32	size;
	uint8	opcode;
	uint8	prot;
	// -khaos--+++> Returns either -1, 0 or 1.  -1 is unset, 0 is from complete file, 1 is from part file
	bool	IsFromPF()				{return m_bFromPF;}
	// <-----khaos-
private:
	bool	m_bSplitted;
	bool	m_bLastSplitted;
	char	head[6];
	char*	completebuffer;
	char*	tempbuffer;
	bool	m_bPacked;
	// -khaos--+++>
	int		m_bFromPF;
	// <-----khaos-
};

struct STag{
	STag();
	STag(const STag& in);
	~STag();

	int8	type;
	LPSTR	tagname;
	union{
	  LPSTR	stringvalue;
	  int32	intvalue;
	  float floatvalue;
	};
	int8	specialtag;
};

class CTag {
public:
	CTag(LPCSTR name, uint32 intvalue);
	CTag(int8 special, uint32 intvalue);
	CTag(LPCSTR name, LPCSTR strvalue);
	CTag(int8 special, LPCSTR strvalue);
	CTag(const STag& in_tag);
	CTag(CFile* in_data);
	~CTag();
	
	CTag* CloneTag() { return new CTag(tag); }
	
	bool WriteTagToFile(CFile* file); //used for CMemfiles
	
	STag tag;
	CString GetFullInfo() const;
};

void ConvertED2KTag(CTag*& pTag);
