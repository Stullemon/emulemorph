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

struct Fakes_Struct{
   uchar			Hash[16];
   uint32			Lenght;
   CString			RealTitle;
};

#define	DFLT_FAKECHECK_FILENAME	_T("fake.dat")

typedef CTypedPtrArray<CPtrArray, Fakes_Struct*> CFakecheckArray;

class CFakecheck: public CLoggable
{
public:
	CFakecheck();
	~CFakecheck();
	void	AddFake(uchar* Hash,uint32& Lenght,CString& Realtitle);
	void	RemoveAllFakes();
	int		LoadFromFile();
	CString GetLastHit() const;
	bool	IsFake(uchar* Hash2test, uint32 lenght);
	void	DownloadFakeList();
	CString GetDefaultFilePath() const;
private:
	const Fakes_Struct* m_pLastHit;
	CFakecheckArray m_fakelist;
};
