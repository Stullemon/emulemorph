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

#include "../../Types.h"
#include <list>

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CTag
{
public:
	byte	m_type;
	CString m_name;

	CTag(byte type, LPCSTR name) { m_type = type; m_name = name; }
	virtual ~CTag() {}

	bool IsStr()  const { return m_type == 2; }
	bool IsNum()  const { return m_type == 3 || m_type == 8 || m_type == 9 || m_type == 5 || m_type == 4 || m_type == 0xFE; }
	bool IsInt()  const { return m_type == 3 || m_type == 8 || m_type == 9 || m_type == 0xFE; }
	bool IsFloat()const { return m_type == 4; }

	virtual CString GetStr() const = 0;
	virtual uint32 GetInt() const = 0;
	virtual float GetFloat() const = 0;

protected:
	CTag() {}
};

class CTagUnk : public CTag
{
public:
	CTagUnk(byte type, LPCSTR name) { m_type = type; m_name = name; }

	virtual CString GetStr() const { return _T(""); }
	virtual uint32 GetInt() const { return 0; }
	virtual float GetFloat() const { return 0.0F; }
};

class CTagStr : public CTag
{
public:
	CTagStr(LPCSTR name, LPCSTR value) { m_type = 0x02; m_name = name; m_value = value; }

	virtual CString GetStr() const { return m_value; }
	virtual uint32 GetInt() const { return atoi(m_value); }
	virtual float GetFloat() const { return atof(m_value); }

protected:
	CString m_value;
};

class CTagUInt : public CTag
{
public:
	CTagUInt(LPCSTR name, uint32 value) { m_type = 0xFE; m_name = name; m_value = value; }

	virtual CString GetStr() const { CString str; ultoa(m_value, str.GetBuffer(10), 10); return str; }
	virtual uint32 GetInt() const { return m_value; }
	virtual float GetFloat() const { return m_value; }

protected:
	uint32 m_value;
};

class CTagUInt32 : public CTag
{
public:
	CTagUInt32(LPCSTR name, uint32 value) { m_type = 0x03; m_name = name; m_value = value; }

	virtual CString GetStr() const { CString str; ultoa(m_value, str.GetBuffer(10), 10); return str; }
	virtual uint32 GetInt() const { return m_value; }
	virtual float GetFloat() const { return m_value; }

protected:
	uint32 m_value;
};

class CTagFloat : public CTag
{
public:
	CTagFloat(LPCSTR name, float value) { m_type = 0x04; m_name = name; m_value = value; }

	virtual CString GetStr() const { CString str; ultoa(m_value, str.GetBuffer(10), 10); return str; }
	virtual uint32 GetInt() const { return m_value; }
	virtual float GetFloat() const { return m_value; }

protected:
	float m_value;
};

class CTagBool : public CTag
{
public:
	CTagBool(LPCSTR name, bool value) { m_type = 0x05; m_name = name; m_value = value; }

	virtual CString GetStr() const { CString str; ultoa(m_value, str.GetBuffer(10), 10); return str; }
	virtual uint32 GetInt() const { return m_value; }
	virtual float GetFloat() const { return m_value; }

protected:
	bool m_value;
};

class CTagUInt16 : public CTag
{
public:
	CTagUInt16(LPCSTR name, uint16 value) { m_type = 0x08; m_name = name; m_value = value; }

	virtual CString GetStr() const { CString str; ultoa(m_value, str.GetBuffer(10), 10); return str; }
	virtual uint32 GetInt() const { return m_value; }
	virtual float GetFloat() const { return m_value; }

protected:
	uint16 m_value;
};

class CTagUInt8 : public CTag
{
public:
	CTagUInt8(LPCSTR name, uint8 value) { m_type = 0x09; m_name = name; m_value = value; }

	virtual CString GetStr() const { CString str; ultoa(m_value, str.GetBuffer(10), 10); return str; }
	virtual uint32 GetInt() const { return m_value; }
	virtual float GetFloat() const { return m_value; }

protected:
	uint8 m_value;
};

typedef std::list<CTag*> TagList;

} // End namespace