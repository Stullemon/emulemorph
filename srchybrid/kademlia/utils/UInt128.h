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

////////////////////////////////////////
namespace Kademlia {
////////////////////////////////////////

class CUInt128 
{
public:

	CUInt128();
	CUInt128(bool fill);
	CUInt128(ULONG value);
	CUInt128(const byte *valueBE);
	/**
	 * Generates a new number, copying the most significant 'numBits' bits from 'value'.
	 * The remaining bits are randomly generated.
	 */
	CUInt128(const CUInt128 &value, UINT numBits = 128);

	const byte* getData() const { return (byte*)m_data; }
	byte* getDataPtr() const { return (byte*)m_data; }

	/** Bit at level 0 being most significant. */
	UINT getBitNumber(UINT bit) const;
	int compareTo(const CUInt128 &other) const;
	int compareTo(ULONG value) const;

	void toHexString(CString *str) const;
	void toBinaryString(CString *str, bool trim = false) const;
	void toByteArray(byte *b) const;

	ULONG get32BitChunk(int val) const {return m_data[val];}

	CUInt128& setValue(const CUInt128 &value);
	CUInt128& setValue(ULONG value);
	CUInt128& setValueBE(const byte *valueBE);

	CUInt128& setValueRandom(void);
	CUInt128& setValueGUID(void);

	CUInt128& setBitNumber(UINT bit, UINT value);
	CUInt128& shiftLeft(UINT bits);

	CUInt128& add(const CUInt128 &value);
	CUInt128& add(ULONG value);
	CUInt128& subtract(const CUInt128 &value);
	CUInt128& subtract(ULONG value);

	CUInt128& xor(const CUInt128 &value);
	CUInt128& xorBE(const byte *valueBE);

	void operator+  (const CUInt128 &value) {add(value);}
	void operator-  (const CUInt128 &value) {subtract(value);}
	void operator=  (const CUInt128 &value) {setValue(value);}
	bool operator<  (const CUInt128 &value) const {return (compareTo(value) <  0);}
	bool operator>  (const CUInt128 &value) const {return (compareTo(value) >  0);}
	bool operator<= (const CUInt128 &value) const {return (compareTo(value) <= 0);}
	bool operator>= (const CUInt128 &value) const {return (compareTo(value) >= 0);}
	bool operator== (const CUInt128 &value) const {return (compareTo(value) == 0);}
	bool operator!= (const CUInt128 &value) const {return (compareTo(value) != 0);}

	void operator+  (ULONG value) {add(value);}
	void operator-  (ULONG value) {subtract(value);}
	void operator=  (ULONG value) {setValue(value);}
	bool operator<  (ULONG value) const {return (compareTo(value) <  0);}
	bool operator>  (ULONG value) const {return (compareTo(value) >  0);}
	bool operator<= (ULONG value) const {return (compareTo(value) <= 0);}
	bool operator>= (ULONG value) const {return (compareTo(value) >= 0);}
	bool operator== (ULONG value) const {return (compareTo(value) == 0);}
	bool operator!= (ULONG value) const {return (compareTo(value) != 0);}

private:

	ULONG m_data[4];
};

} // End namespace