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
#include "loggable.h"

struct SIPFilter
{
   uint32	start;
   uint32	end;
   UINT		level;
   CString	desc;
   UINT		hits;
};

#define	DFLT_IPFILTER_FILENAME	_T("ipfilter.dat")

// 'CArray' would give us more cach hits, but would also be slow in array element creation 
// (because of the implicit ctor in 'SIPFilter'
//typedef CArray<SIPFilter, SIPFilter> CIPFilterArray; 
typedef CTypedPtrArray<CPtrArray, SIPFilter*> CIPFilterArray;

class CIPFilter: public CLoggable
{
public:
	CIPFilter();
	~CIPFilter();

	CString GetDefaultFilePath() const;

	void AddIPRange(uint32 IPfrom, uint32 IPto, UINT level, const CString& desc);
	void AddIP(uint32 IP, UINT level, const CString& desc); //MORPH - Added by SiRoB
	void RemoveAllIPFilters();
	bool RemoveIPFilter(const SIPFilter* pFilter);

	int AddFromFile(LPCTSTR pszFilePath, bool bShowResponse = true);
	int LoadFromDefaultFile(bool bShowResponse = true);
	void SaveToDefaultFile();

	bool IsFiltered(uint32 IP) /*const*/;
	bool IsFiltered(uint32 IP, UINT level) /*const*/;
	LPCTSTR GetLastHit() const;

	const CIPFilterArray& GetIPFilter() const;
	void    UpdateIPFilterURL();//MORPH START added by Yun.SF3: Ipfilter.dat update

private:
	const SIPFilter* m_pLastHit;
	CIPFilterArray m_iplist;

	bool ParseFilterLine1(const CString& sbuffer, uint32& ip1, uint32& ip2, UINT& level, CString& desc) const;
	bool ParseFilterLine2(const CString& sbuffer, uint32& ip1, uint32& ip2, UINT& level, CString& desc) const;
};
