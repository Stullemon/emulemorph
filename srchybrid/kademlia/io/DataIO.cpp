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
#include "DataIO.h"
#include "../kademlia/Kademlia.h"
#include "../kademlia/Tag.h"
#include "../utils/LittleEndian.h"
#include "../utils/UInt128.h"
#include "IOException.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


////////////////////////////////////////
using namespace Kademlia;
////////////////////////////////////////

// This may look confusing that the normal methods use le() and the LE methods don't.
// The reason is the variables are stored in memory in little endian format already.

byte CDataIO::readByte(void)
{
	byte retVal;
	readArray(&retVal, 1);
	return retVal;
}

uint8 CDataIO::readUInt8BE(void)
{
	return readUInt8();
}

uint16 CDataIO::readUInt16BE(void)
{
	return le(readUInt16());
}

uint32 CDataIO::readUInt32BE(void)
{
	return le(readUInt32());
}

void CDataIO::readUInt128BE(CUInt128 *value)
{
	byte b[16];
	readArray(b, 16);
	value->setValue(b);
}

uint8 CDataIO::readUInt8(void)
{
	uint8 retVal;
	readArray(&retVal, sizeof(uint8));
	return retVal;
}

uint16 CDataIO::readUInt16(void)
{
	uint16 retVal;
	readArray(&retVal, sizeof(uint16));
	return retVal;
}

uint32 CDataIO::readUInt32(void)
{
	uint32 retVal;
	readArray(&retVal, sizeof(uint32));
	return retVal;
}

void CDataIO::readUInt128(CUInt128 *value)
{
	readArray(value->getDataPtr(), sizeof(uint32)*4);
}

float CDataIO::readFloat(void)
{
	float retVal;
	readArray(&retVal, sizeof(float));
	return retVal;
}

CTag *CDataIO::readTag(void)
{
	CTag *retVal = NULL;
	char *name = NULL;
	char *value = NULL;
	try
	{
		byte type = readByte();
		uint16 lenName = readUInt16();
		name = new char[lenName+1];
		name[lenName] = 0;
		readArray(name, lenName);

		switch (type)
		{
			case 0x01:
				retVal = new CTagUnk(type, name);
				break;

			case 0x02:{
				UINT lenValue = readUInt16();
				value = new char[lenValue+1];
				value[lenValue] = 0;
				readArray(value, lenValue);
				retVal = new CTagStr(name, value);
				delete [] value;
				value = NULL;
				break;
			}

			case 0x03:
				retVal = new CTagUInt32(name, readUInt32());
				break;

			case 0x04:
				retVal = new CTagFloat(name, readFloat());
				break;

			case 0x08:
				retVal = new CTagUInt16(name, readUInt16());
				break;

			case 0x09:
				retVal = new CTagUInt8(name, readUInt8());
				break;

			default:
				CKademlia::logMsg("****************************");
				CKademlia::logMsg("Found Unknown TAG Type (0x%02X)", type);
				CKademlia::logMsg("****************************");
				retVal = new CTagUnk(type, name);
		}
		delete [] name;
		name = NULL;
	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CDataIO:readTag (IO Error(%i))", ioe->m_cause);
		delete [] name;
		delete [] value;
		throw ioe;
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CDataIO:readTag");
		delete [] name;
		delete [] value;
		throw;
	}
	return retVal;
}

TagList	*CDataIO::readTagList(void)
{
	TagList *retVal = new TagList;
	uint32 count = readByte();
	for (uint32 i=0; i<count; i++)
		retVal->push_back(readTag());
	return retVal;
}

void CDataIO::writeByte(byte val)
{
	writeArray(&val, 1);
}

void CDataIO::writeUInt8BE(uint8 val)
{
	writeUInt8(val);
}

void CDataIO::writeUInt16BE(uint16 val)
{
	writeUInt16(le(val));
}

void CDataIO::writeUInt32BE(uint32 val)
{
	writeUInt32(le(val));
}

void CDataIO::writeUInt128BE(const CUInt128 &val)
{
	byte b[16];
	val.toByteArray(b);
	writeArray(b, 16);
}

void CDataIO::writeUInt8(uint8 val)
{
	writeArray(&val, sizeof(uint8));
}

void CDataIO::writeUInt16(uint16 val)
{
	writeArray(&val, sizeof(uint16));
}

void CDataIO::writeUInt32(uint32 val)
{
	writeArray(&val, sizeof(uint32));
}

void CDataIO::writeUInt128(const CUInt128 &val)
{
	writeArray(val.getData(), sizeof(uint32)*4);
}

void CDataIO::writeFloat(float val)
{
	writeArray(&val, sizeof(float));
}

void CDataIO::writeTag(const CTag *tag)
{
	try
	{
		uint8 type;
		if (tag->m_type == 0xFE)
		{
			if (tag->GetInt() <= 0xFF)
				type = 0x09;
			else if (tag->GetInt() <= 0xFFFF)
				type = 0x08;
			else
				type = 0x03;
		}
		else
			type = tag->m_type;

		writeByte(type);

		CString name = tag->m_name;
		writeUInt16(name.GetLength());
		writeArray(name.GetBuffer(0), name.GetLength());

		switch (type)
		{
			case 0x01:
				break;
			case 0x02:
				writeUInt16(tag->GetStr().GetLength());
				writeArray(tag->GetStr(), tag->GetStr().GetLength());
				break;
			case 0x03:
				writeUInt32(tag->GetInt());
				break;
			case 0x04:
				writeFloat(tag->GetFloat());
				break;
			case 0x08:
				writeUInt16(tag->GetInt());
				break;
			case 0x09:
				writeUInt8(tag->GetInt());
				break;
		}
	} 
	catch (CIOException *ioe)
	{
		CKademlia::debugMsg("Exception in CDataIO:writeTag (IO Error(%i))", ioe->m_cause);
		throw ioe;
	}
	catch (...) 
	{
		CKademlia::debugLine("Exception in CDataIO:writeTag");
		throw;
	}
}

void CDataIO::writeTag(byte type, LPCSTR name)
{
	CTagUnk tag(type, name);
	writeTag(&tag);
}

void CDataIO::writeTag(LPCSTR name, LPCSTR value)
{
	CTagStr tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTag(LPCSTR name, uint32 value)
{
	CTagUInt32 tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTag(LPCSTR name, uint16 value)
{
	CTagUInt16 tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTag(LPCSTR name, uint8 value)
{
	CTagUInt8 tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTag(LPCSTR name, float value)
{
	CTagFloat tag(name, value);
	writeTag(&tag);
}

void CDataIO::writeTagList(const TagList &tagList)
{
	uint32 count = (uint32)tagList.size();
	ASSERT( count <= 0xFF );
	writeByte(count);
	TagList::const_iterator it;
	for (it = tagList.begin(); it != tagList.end(); it++)
		writeTag(*it);
}