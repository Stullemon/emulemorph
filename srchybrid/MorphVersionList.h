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

class CUpDownClient;

class CMorphVersionList
{
public:
	CMorphVersionList(void);
	~CMorphVersionList(void);
	
	void SaveList();
	void LoadList();

	uint8 GetMorphSubVer(uint8 in)	{ return m_uMorphSubVer[in]; }
	void SetMorphSubVer(uint8 iMjrVer, uint8 iSubVer)	{ m_uMorphSubVer[iMjrVer] = iSubVer; }
	bool GetListValid()				{ return m_bListValid; }
	void SetListValid(bool in)		{ m_bListValid = in; }
	uint8 GetListLength()			{ return m_uListLength; }

protected:
	uint8 m_uMorphSubVer[20];
	uint8 m_uListLength;
	bool m_bListValid;

	class CSettingsData
	{
	public:
		CSettingsData(DWORD dwValue)
		{
			dwData = dwValue;
		}
		DWORD dwData;
	};
	typedef CTypedPtrList<CPtrList, CSettingsData*> SettingsList;
};

extern CMorphVersionList theMorphVerList;