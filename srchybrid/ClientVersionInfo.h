//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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


#pragma once
#include "emule.h"
#include "updownclient.h"
// class to convert peercache Client versions to comparable values.

#define CVI_IGNORED	-1
// == means same client type, same or ignored version (for example eMule/0.4.* == eMule/0.4.2 )
// != means different client or different defined version (for example eMule/0.4.2 != SomeClient/0.4.2 )
// > mean _same client type_ and higher version, which therefor cannot be completely undefined ( for example eMule/1.* > eMule/0.4.2 )
// >= same as > but here the version can be undefined ( for example eMule/* >= eMule/0.4.2 )
class CClientVersionInfo{
public:

	CClientVersionInfo(CString strPCEncodedVersion)
	{
		m_nVerMajor = (UINT)CVI_IGNORED;
		m_nVerMinor = (UINT)CVI_IGNORED;
		m_nVerUpdate = (UINT)CVI_IGNORED;
		m_nVerBuild = (UINT)CVI_IGNORED;
		m_ClientTypeMajor = SO_UNKNOWN; 
		m_ClientTypeMinor = SO_UNKNOWN;

		int posSeperator = strPCEncodedVersion.Find('/',1);
		if (posSeperator == (-1) || strPCEncodedVersion.GetLength() - posSeperator < 2){
			theApp.QueueDebugLogLine( false, _T("PeerCache Error: Bad Version info in PeerCache Descriptor found: %s"), strPCEncodedVersion);
			return;
		}
		CString strClientType = strPCEncodedVersion.Left(posSeperator).Trim();
		CString strVersionNumber = strPCEncodedVersion.Mid(posSeperator+1).Trim();

		if (strClientType.CompareNoCase(_T("eMule")) == 0)
			m_ClientTypeMajor = SO_EMULE;
		else if (strClientType.CompareNoCase(_T("eDonkey")) == 0)
			m_ClientTypeMajor = SO_EDONKEYHYBRID;
		// can add more types here
		else{
			theApp.QueueDebugLogLine(false, _T("PeerCache Warning: Unknown Clienttype in descriptor file found"));
			m_ClientTypeMajor = SO_UNKNOWN;
		}

		int curPos2= 0;
		CString strNumber = strVersionNumber.Tokenize(_T("."),curPos2);
		if (strNumber.IsEmpty())
			return;
		else if (strNumber == _T("*"))
			m_nVerMajor = (UINT)-1;
		else
			m_nVerMajor = _tstoi(strNumber);
		strNumber = strVersionNumber.Tokenize(_T("."),curPos2);
		if (strNumber.IsEmpty())
			return;
		else if (strNumber == _T("*"))
			m_nVerMinor = (UINT)-1;
		else
			m_nVerMinor = _tstoi(strNumber);
		strNumber = strVersionNumber.Tokenize(_T("."),curPos2);
		if (strNumber.IsEmpty())
			return;
		else if (strNumber == _T("*"))
			m_nVerUpdate = (UINT)-1;
		else
			m_nVerUpdate = _tstoi(strNumber);
		strNumber = strVersionNumber.Tokenize(_T("."),curPos2);
		if (strNumber.IsEmpty())
			return;
		else if (strNumber == _T("*"))
			m_nVerBuild = (UINT)-1;
		else
			m_nVerBuild = _tstoi(strNumber);
	}
	
	CClientVersionInfo(uint32 dwTagVersionInfo, UINT nClientMajor)
	{
		UINT nClientMajVersion = (dwTagVersionInfo >> 17) & 0x7f;
		UINT nClientMinVersion = (dwTagVersionInfo>> 10) & 0x7f;
		UINT nClientUpVersion  = (dwTagVersionInfo >>  7) & 0x07;
		CClientVersionInfo(nClientMajVersion, nClientMinVersion, nClientUpVersion, (UINT)CVI_IGNORED, nClientMajor, SO_UNKNOWN);
	}

	CClientVersionInfo(uint32 nVerMajor, uint32 nVerMinor, uint32 nVerUpdate, uint32 nVerBuild, uint32 ClientTypeMajor, uint32 ClientTypeMinor = SO_UNKNOWN)
	{
		m_nVerMajor = nVerMajor;
		m_nVerMinor = nVerMinor;
		m_nVerUpdate = nVerUpdate;
		m_nVerBuild = nVerBuild;
		m_ClientTypeMajor = ClientTypeMajor;
		m_ClientTypeMinor = ClientTypeMinor;
	}

	CClientVersionInfo(){
		CClientVersionInfo((UINT)CVI_IGNORED, (UINT)CVI_IGNORED, (UINT)CVI_IGNORED, (UINT)CVI_IGNORED, SO_UNKNOWN, SO_UNKNOWN); 
	}

	CClientVersionInfo(const CClientVersionInfo& cv)		{*this = cv;}

	CClientVersionInfo& operator=(const CClientVersionInfo& cv)
	{
		m_nVerMajor = cv.m_nVerMajor;
		m_nVerMinor = cv.m_nVerMinor;
		m_nVerUpdate = cv.m_nVerUpdate;
		m_nVerBuild = cv.m_nVerBuild;
		m_ClientTypeMajor = cv.m_ClientTypeMajor;
		m_ClientTypeMinor = cv.m_ClientTypeMinor;
		return *this; 
	}

	friend bool operator==(const CClientVersionInfo& c1, const CClientVersionInfo& c2)
	{
		return ( (c1.m_nVerMajor == (-1) || c2.m_nVerMajor == (-1) || c1.m_nVerMajor == c2.m_nVerMajor)
			&& (c1.m_nVerMinor == (-1) || c2.m_nVerMinor == (-1) || c1.m_nVerMinor == c2.m_nVerMinor)
			&& (c1.m_nVerUpdate == (-1) || c2.m_nVerUpdate == (-1) || c1.m_nVerUpdate == c2.m_nVerUpdate)
			&& (c1.m_nVerBuild == (-1) || c2.m_nVerBuild == (-1) || c1.m_nVerBuild == c2.m_nVerBuild)
			&& (c1.m_ClientTypeMajor == (-1) || c2.m_ClientTypeMajor == (-1) || c1.m_ClientTypeMajor == c2.m_ClientTypeMajor)
			&& (c1.m_ClientTypeMinor == (-1) || c2.m_ClientTypeMinor == (-1) || c1.m_ClientTypeMinor == c2.m_ClientTypeMinor)
			);
	}

	friend bool operator !=(const CClientVersionInfo& c1, const CClientVersionInfo& c2)
	{
		return !(c1 == c2);
	}
	
	friend bool operator >(const CClientVersionInfo& c1, const CClientVersionInfo& c2)
	{
		if ( (c1.m_ClientTypeMajor == (-1) || c2.m_ClientTypeMajor == (-1) || c1.m_ClientTypeMajor != c2.m_ClientTypeMajor)
			|| (c1.m_ClientTypeMinor != c2.m_ClientTypeMinor))
			return false;
		if (c1.m_nVerMajor != (-1) && c2.m_nVerMajor != (-1) && c1.m_nVerMajor > c2.m_nVerMajor)
			return true;
		if (c1.m_nVerMinor != (-1) && c2.m_nVerMinor != (-1) && c1.m_nVerMinor > c2.m_nVerMinor)
			return true;
		if (c1.m_nVerUpdate != (-1) && c2.m_nVerUpdate != (-1) && c1.m_nVerUpdate > c2.m_nVerUpdate)
			return true;
		if (c1.m_nVerBuild != (-1) && c2.m_nVerBuild != (-1) && c1.m_nVerBuild > c2.m_nVerBuild)
			return true;
		return false;
	}

	friend bool operator <(const CClientVersionInfo& c1, const CClientVersionInfo& c2)
	{
		return c2 > c1;
	}

	friend bool operator <=(const CClientVersionInfo& c1, const CClientVersionInfo& c2)
	{
		return c2 > c1 || c1 == c2;
	}

	friend bool operator >=(const CClientVersionInfo& c1, const CClientVersionInfo& c2)
	{
		return c1 > c2 || c1 == c2;
	}
	
	UINT m_nVerMajor;
	UINT m_nVerMinor;
	UINT m_nVerUpdate;
	UINT m_nVerBuild;
	UINT m_ClientTypeMajor;
	UINT m_ClientTypeMinor; //unused atm
};