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

namespace Kademlia
{
	class CUInt128
	{
		public:
			// netfinity: Inlined default constructor
			CUInt128() : m_uData0(0), m_uData1(0), m_uData2(0), m_uData3(0) {}
			CUInt128(bool bFill);
			CUInt128(ULONG uValue);
			CUInt128(const byte *pbyValueBE);
			//Generates a new number, copying the most significant 'numBits' bits from 'value'.
			//The remaining bits are randomly generated.
			CUInt128(const CUInt128 &uValue, UINT uNumBits/* = 128*/);
			// netfinity: Copy constructor - This one is extra fast as it is completly inline!!!
			CUInt128(const CUInt128 &uValue) : m_uData0(uValue.m_uData0), m_uData1(uValue.m_uData1), m_uData2(uValue.m_uData2), m_uData3(uValue.m_uData3) {}

			const byte* GetData() const;
			byte* GetDataPtr() const;
			/** Bit at level 0 being most significant. */
			UINT GetBitNumber(UINT uBit) const;
			int CompareTo(const CUInt128 &uOther) const;
			int CompareTo(ULONG uValue) const;
			void ToHexString(CString *pstr) const;
			void ToBinaryString(CString *pstr, bool bTrim = false) const;
			void ToByteArray(byte *pby) const;
			ULONG Get32BitChunk(int iVal) const;
			// netfinity: Safe KAD - Check if a number appears to be constructed rather than randomly generated
			bool IsGoodRandom() const;
			CUInt128& SetValue(const CUInt128 &uValue);
			CUInt128& SetValue(ULONG uValue);
			CUInt128& SetValueBE(const byte *pbyValueBE);
			CUInt128& SetValueRandom();
			CUInt128& SetValueGUID();
			CUInt128& SetBitNumber(UINT uBit, UINT uValue);
			CUInt128& ShiftLeft(UINT uBits);
			CUInt128& Add(const CUInt128 &uValue);
			CUInt128& Add(ULONG uValue);
			CUInt128& Subtract(const CUInt128 &uValue);
			CUInt128& Subtract(ULONG uValue);
			CUInt128& Xor(const CUInt128 &uValue);
			CUInt128& XorBE(const byte *pbyValueBE);
			void operator+  (const CUInt128 &uValue);
			void operator-  (const CUInt128 &uValue);
			void operator=  (const CUInt128 &uValue);
			bool operator<  (const CUInt128 &uValue) const;
			bool operator>  (const CUInt128 &uValue) const;
			bool operator<= (const CUInt128 &uValue) const;
			bool operator>= (const CUInt128 &uValue) const;
			bool operator== (const CUInt128 &uValue) const;
			bool operator!= (const CUInt128 &uValue) const;
			void operator+  (ULONG uValue);
			void operator-  (ULONG uValue);
			void operator=  (ULONG uValue);
			bool operator<  (ULONG uValue) const;
			bool operator>  (ULONG uValue) const;
			bool operator<= (ULONG uValue) const;
			bool operator>= (ULONG uValue) const;
			bool operator== (ULONG uValue) const;
			bool operator!= (ULONG uValue) const;
		private:
			// netfinity: Constructors can't initialize arrays inline
			union
			{
			ULONG		m_uData[4];
				struct
				{
					ULONG	m_uData0;
					ULONG	m_uData1;
					ULONG	m_uData2;
					ULONG	m_uData3;
				};
			};
	};

	// netfinity: This code is better used inline, for speed

	inline
	CUInt128& CUInt128::SetValue(const CUInt128 &uValue)
	{
		m_uData[0] = uValue.m_uData[0];
		m_uData[1] = uValue.m_uData[1];
		m_uData[2] = uValue.m_uData[2];
		m_uData[3] = uValue.m_uData[3];
		return *this;
	}

	inline
	CUInt128& CUInt128::SetValue(ULONG uValue)
	{
		m_uData[0] = 0;
		m_uData[1] = 0;
		m_uData[2] = 0;
		m_uData[3] = uValue;
		return *this;
	}

	inline
	CUInt128& CUInt128::Xor(const CUInt128 &uValue)
	{
		m_uData[0] ^= uValue.m_uData[0];
		m_uData[1] ^= uValue.m_uData[1];
		m_uData[2] ^= uValue.m_uData[2];
		m_uData[3] ^= uValue.m_uData[3];
		return *this;
	}

	inline
	void CUInt128::operator=  (const CUInt128 &uValue)
	{
		SetValue(uValue);
	}

	inline
	bool CUInt128::operator<  (const CUInt128 &uValue) const
	{
		if (m_uData[0] == uValue.m_uData[0])
			if (m_uData[1] == uValue.m_uData[1])
				if (m_uData[2] == uValue.m_uData[2])
					return (m_uData[3] < uValue.m_uData[3]);
				else
					return (m_uData[2] < uValue.m_uData[2]);
			else
				return (m_uData[1] < uValue.m_uData[1]);
		else
			return (m_uData[0] < uValue.m_uData[0]);
	}

	inline
	bool CUInt128::operator>  (const CUInt128 &uValue) const
	{
		if (m_uData[0] == uValue.m_uData[0])
			if (m_uData[1] == uValue.m_uData[1])
				if (m_uData[2] == uValue.m_uData[2])
					return (m_uData[3] > uValue.m_uData[3]);
				else
					return (m_uData[2] > uValue.m_uData[2]);
			else
				return (m_uData[1] > uValue.m_uData[1]);
		else
			return (m_uData[0] > uValue.m_uData[0]);
	}

	inline
	bool CUInt128::operator<= (const CUInt128 &uValue) const
	{
		if (m_uData[0] == uValue.m_uData[0])
			if (m_uData[1] == uValue.m_uData[1])
				if (m_uData[2] == uValue.m_uData[2])
					return (m_uData[3] <= uValue.m_uData[3]);
				else
					return (m_uData[2] < uValue.m_uData[2]);
			else
				return (m_uData[1] < uValue.m_uData[1]);
		else
			return (m_uData[0] < uValue.m_uData[0]);
	}

	inline
	bool CUInt128::operator>= (const CUInt128 &uValue) const
	{
		if (m_uData[0] == uValue.m_uData[0])
			if (m_uData[1] == uValue.m_uData[1])
				if (m_uData[2] == uValue.m_uData[2])
					return (m_uData[3] >= uValue.m_uData[3]);
				else
					return (m_uData[2] > uValue.m_uData[2]);
			else
				return (m_uData[1] > uValue.m_uData[1]);
		else
			return (m_uData[0] > uValue.m_uData[0]);
}

	inline
	bool CUInt128::operator== (const CUInt128 &uValue) const
	{
		return (m_uData[0] == uValue.m_uData[0] && m_uData[1] == uValue.m_uData[1] && m_uData[2] == uValue.m_uData[2] && m_uData[3] == uValue.m_uData[3]);
	}

	inline
	bool CUInt128::operator!= (const CUInt128 &uValue) const
	{
		return (m_uData[0] != uValue.m_uData[0] || m_uData[1] != uValue.m_uData[1] || m_uData[2] != uValue.m_uData[2] || m_uData[3] != uValue.m_uData[3]);
	}
}

