//this file is part of eMule
//Copyright (C)2002-2004 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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


enum EBBRStatus{
	BBR_NONE = 0,
	BBR_VERIFIED,
	BBR_CORRUPTED
};


class CCBBRecord
{
public:
	CCBBRecord(uint32 nStartPos = 0, uint32 nEndPos = 0, uint32 dwIP = 0, EBBRStatus BBRStatus = BBR_NONE);
	CCBBRecord(const CCBBRecord& cv)			{ *this = cv; }
	CCBBRecord& operator=(const CCBBRecord& cv);

	bool	Merge(uint32 nStartPos, uint32 nEndPos, uint32 dwIP, EBBRStatus BBRStatus = BBR_NONE);
	bool	CanMerge(uint32 nStartPos, uint32 nEndPos, uint32 dwIP, EBBRStatus BBRStatus = BBR_NONE);

	uint32	m_nStartPos;
	uint32	m_nEndPos;
	uint32	m_dwIP;
	EBBRStatus 	m_BBRStatus;
};


typedef CArray<CCBBRecord, CCBBRecord&>	CRecordArray;

class CCorruptionBlackBox
{
public:
	CCorruptionBlackBox()			{}
	~CCorruptionBlackBox()			{}
	void	Init(uint32 nFileSize);
	void	Free();
	void	TransferredData(uint32 nStartPos, uint32 nEndPos, const CUpDownClient* pSender);
	void	VerifiedData(uint32 nStartPos, uint32 nEndPos);
	void	CorruptedData(uint32 nStartPos, uint32 nEndPos);


private:
	CArray<CRecordArray, CRecordArray&> m_aaRecords;
};
