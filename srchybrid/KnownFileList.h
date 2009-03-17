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
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once
#include "MapKey.h"
#include "SHAHashset.h"

class CKnownFile;
typedef CMap<CCKey,const CCKey&,CKnownFile*,CKnownFile*> CKnownFilesMap;
typedef CMap<CSKey,const CSKey&,int,int> CancelledFilesMap;
typedef CMap<CAICHHash, const CAICHHash&, const CKnownFile*, const CKnownFile*> KnonwFilesByAICHMap;

class CKnownFileList 
{
	friend class CSharedFilesWnd;
	friend class CStatisticFile;
public:
	CKnownFileList();
	~CKnownFileList();

	bool	SafeAddKFile(CKnownFile* toadd);
	bool	Init();
	void	Save();
	void	Clear();
	void	Process();

	CKnownFile* FindKnownFile(LPCTSTR filename, uint32 date, uint64 size) const;
	CKnownFile* FindKnownFileByID(const uchar* hash) const;
	CKnownFile* FindKnownFileByPath(const CString& sFilePath) const;
	void	MergePartFileStats(CKnownFile* original);	// SLUGFILLER: mergeKnown - retrieve part file stats from known file
	bool	IsKnownFile(const CKnownFile* file) const;
	bool	IsFilePtrInList(const CKnownFile* file) const;

	void	AddCancelledFileID(const uchar* hash);
	bool	IsCancelledFileByID(const uchar* hash) const;

	const CKnownFilesMap& GetKnownFiles() const { return m_Files_map; }
	void	CopyKnownFileMap(CMap<CCKey,const CCKey&,CKnownFile*,CKnownFile*> &Files_Map);

	bool	ShouldPurgeAICHHashset(const CAICHHash& rAICHHash) const;
	void	AICHHashChanged(const CAICHHash* pOldAICHHash, const CAICHHash& rNewAICHHash, CKnownFile* pFile);

	//MORPH START - Added, Downloaded History [Monki/Xman]
#ifndef NO_HISTORY
	CKnownFilesMap* GetDownloadedFiles();
	bool RemoveKnownFile(CKnownFile *toRemove);
	void ClearHistory();

	bool	bReloadHistory; //Fafner: possible exception in history - 070626
#endif
	//MORPH END   - Added, Downloaded History [Monki/Xman]

private:
	bool	LoadKnownFiles();
	bool	LoadCancelledFiles();

	uint16 	requested;
	uint16 	accepted;
	uint64 	transferred;
	uint32 	m_nLastSaved;
	CKnownFilesMap		m_Files_map;
	CancelledFilesMap	m_mapCancelledFiles;
	// for faster access, map of files indexed by AICH-hash, not garantueed to be complete at this point (!)
	// (files which got AICH hashed later will not be added yet, because we don't need them, make sure to change this if needed)
	KnonwFilesByAICHMap m_mapKnownFilesByAICH;
	uint32	m_dwCancelledFilesSeed;
};
