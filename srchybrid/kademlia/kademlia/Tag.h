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
#include <list>
#include "opcodes.h"
#include "OtherFunctions.h"

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CUInt128;

class CTagNameString : protected CStringA
{
public:
	CTagNameString()
	{
	}

	CTagNameString(LPCSTR psz)
		: CStringA(psz)
	{
	}

	CTagNameString(LPCSTR psz, int len)
		: CStringA(psz, len)
	{
	}

	// A tag name may include character values >= 0xD0 and therefor also >= 0xF0. to prevent those
	// characters be interpreted as multi byte character sequences we have to sensure that a binary
	// string compare is performed.
	int Compare(LPCSTR psz) const throw()
	{
		ATLASSERT( AtlIsValidString(psz) );
		// Do a binary string compare. (independant from any codepage and/or LC_CTYPE setting.)
		return strcmp(GetString(), psz);
	}

	int CompareNoCase(LPCSTR psz) const throw()
	{
		ATLASSERT( AtlIsValidString(psz) );

		// Version #1
		// Do a case-insensitive ASCII string compare.
		// NOTE: The current locale category LC_CTYPE *MUST* be set to "C"!
		//return stricmp(GetString(), psz);

		// Version #2 - independant from any codepage and/or LC_CTYPE setting.
		return __ascii_stricmp(GetString(), psz);
	}

	CTagNameString& operator=(LPCSTR pszSrc)
	{
		CStringA::operator=(pszSrc);
		return *this;
	}

	operator PCXSTR() const throw()
	{
		return CStringA::operator PCXSTR();
	}

	XCHAR operator[]( int iChar ) const throw()
	{
		return CStringA::operator [](iChar);
	}

	PXSTR GetBuffer()
	{
		return CStringA::GetBuffer();
	}

	PXSTR GetBuffer(int nMinBufferLength)
	{
		return CStringA::GetBuffer(nMinBufferLength);
	}

	int GetLength() const throw()
	{
		return CStringA::GetLength();
	}
};


//class CTagValueString : protected CStringW
//{
//public:
//	CTagValueString(){}
//};

#define CTagValueString CStringW


class CTag
{
public:
	byte	m_type;
	CTagNameString m_name;

	CTag(byte type, LPCSTR name)
		: m_name(name)
	{
		m_type = type;
	}
	virtual ~CTag() {}

	bool IsStr()  const { return m_type == TAGTYPE_STRING; }
	bool IsNum()  const { return m_type == TAGTYPE_UINT32 || m_type == TAGTYPE_UINT16 || m_type == TAGTYPE_UINT8 || m_type == TAGTYPE_BOOL || m_type == TAGTYPE_FLOAT32 || m_type == 0xFE; }
	bool IsInt()  const { return m_type == TAGTYPE_UINT32 || m_type == TAGTYPE_UINT16 || m_type == TAGTYPE_UINT8 || m_type == 0xFE; }
	bool IsFloat()const { return m_type == TAGTYPE_FLOAT32; }
	bool IsBsob() const { return m_type == TAGTYPE_BSOB; }
	bool IsHash() const { return m_type == TAGTYPE_HASH; }

	virtual CTagValueString GetStr() const { ASSERT(0); return L""; }
	virtual uint32 GetInt() const { ASSERT(0); return 0; }
	virtual float GetFloat() const { ASSERT(0); return 0.0F; }
	virtual const BYTE* GetBsob() const { ASSERT(0); return NULL; }
	virtual uint8 GetBsobSize() const { ASSERT(0); return 0; }
	virtual bool GetBool() const { ASSERT(0); return false; }
	virtual const BYTE* GetHash() const { ASSERT(0); return NULL; }

protected:
	CTag() {}
};


class CTagUnk : public CTag
{
public:
	CTagUnk(byte type, LPCSTR name)
		: CTag(type, name)
	{ }
};


class CTagStr : public CTag
{
public:
	CTagStr(LPCSTR name, LPCWSTR value, int len)
		: CTag(TAGTYPE_STRING, name)
		, m_value(value, len)
	{ }

#ifndef _UNICODE
	CTagStr(LPCSTR name, const CString& rstr)
		: CTag(TAGTYPE_STRING, name)
		, m_value(rstr)
	{ }
#endif

	CTagStr(LPCSTR name, const CStringW& rstr)
		: CTag(TAGTYPE_STRING, name)
		, m_value(rstr)
	{ }

	virtual CTagValueString GetStr() const { return m_value; }

protected:
	CTagValueString m_value;
};


class CTagUInt : public CTag
{
public:
	CTagUInt(LPCSTR name, uint32 value)
		: CTag(0xFE, name)
		, m_value(value)
	{ }

	virtual uint32 GetInt() const { return m_value; }

protected:
	uint32 m_value;
};


class CTagUInt32 : public CTag
{
public:
	CTagUInt32(LPCSTR name, uint32 value)
		: CTag(TAGTYPE_UINT32, name)
		, m_value(value)
	{ }

	virtual uint32 GetInt() const { return m_value; }

protected:
	uint32 m_value;
};


class CTagFloat : public CTag
{
public:
	CTagFloat(LPCSTR name, float value)
		: CTag(TAGTYPE_FLOAT32, name)
		, m_value(value)
	{ }

	virtual float GetFloat() const { return m_value; }

protected:
	float m_value;
};


class CTagBool : public CTag
{
public:
	CTagBool(LPCSTR name, bool value)
		: CTag(TAGTYPE_BOOL, name)
		, m_value(value)
	{ }

	virtual bool GetBool() const { return m_value; }

protected:
	bool m_value;
};


class CTagUInt16 : public CTag
{
public:
	CTagUInt16(LPCSTR name, uint16 value)
		: CTag(TAGTYPE_UINT16, name)
		, m_value(value)
	{ }

	virtual uint32 GetInt() const { return m_value; }

protected:
	uint16 m_value;
};


class CTagUInt8 : public CTag
{
public:
	CTagUInt8(LPCSTR name, uint8 value)
		: CTag(TAGTYPE_UINT8, name)
		, m_value(value)
	{ }

	virtual uint32 GetInt() const { return m_value; }

protected:
	uint8 m_value;
};


class CTagBsob : public CTag
{
public:
	CTagBsob(LPCSTR name, const BYTE* value, uint8 nSize)
		: CTag(TAGTYPE_BSOB, name)
	{
		m_value = new BYTE[nSize];
		memcpy(m_value, value, nSize);
		m_size = nSize;
	}

	~CTagBsob()
	{
		delete[] m_value;
	}

	virtual const BYTE* GetBsob() const { return m_value; }
	virtual uint8 GetBsobSize() const { return m_size; }

protected:
	BYTE* m_value;
	uint8 m_size;
};


class CTagHash : public CTag
{
public:
	CTagHash(LPCSTR name, const BYTE* value) 
		: CTag(TAGTYPE_HASH, name)
	{ 
		m_value = new BYTE[16];
		md4cpy(m_value, value);
	}

	~CTagHash()
	{
		delete[] m_value;
	}

	virtual const BYTE* GetHash() const { return m_value; }

protected:
	BYTE* m_value;
};

typedef std::list<CTag*> TagList;


} // End namespace


void KadTagStrMakeLower(CTagValueString& rstr);
