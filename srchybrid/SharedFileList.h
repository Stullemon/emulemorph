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
#include "opcodes.h"
#include "emule.h"
#include "types.h"
#include "preferences.h"
#include "KnownFile.h"
#include "knownfilelist.h"
#include "sharedfilesctrl.h"
#include "sockets.h"
#include "partfile.h"
#include "mapkey.h"
#include "loggable.h"

struct UnknownFile_Struct{
	CString strName;
	CString strDirectory;
};

class CKnownFileList;

class CSharedFileList: public CLoggable
{
	friend class CSharedFilesCtrl;
	friend class CClientReqSocket;
public:
	CSharedFileList(CPreferences* in_prefs,CServerConnect* in_server, CKnownFileList* in_filelist);
	~CSharedFileList();
	void	SendListToServer();
	void	Reload();
	void	SafeAddKFile(CKnownFile* toadd, bool bOnlyAdd = false);
	void	SetOutputCtrl(CSharedFilesCtrl* in_ctrl);
	void	RemoveFile(CKnownFile* toremove);
	CMutex	list_mut;
	CKnownFile*	GetFileByID(const uchar* filehash);
	CKnownFile*	GetFileByIndex(int index);
	CKnownFileList*		filelist;
	void	CreateOfferedFilePacket(CKnownFile* cur_file, CMemFile* files, bool bForServer = true, bool bSendED2KTags = true);
	// -khaos--+++> New parameter, pbytesLargest
	uint64	GetDatasize(uint64 &pbytesLargest);
	// <-----khaos-
	uint16	GetCount()	{return m_Files_map.GetCount(); }
	uint16	GetHashingCount()	{return waitingforhash_list.GetCount()+currentlyhashing_list.GetCount(); }	// SLUGFILLER: SafeHash
	void	UpdateFile(CKnownFile* toupdate);
	void	AddFilesFromDirectory(const CString& rstrDirectory);
	void	HashFailed(UnknownFile_Struct* hashed);		// SLUGFILLER: SafeHash
	void	ClearED2KPublishInfo();
	void	Process();
	void	Publish();

private:
	void	FindSharedFiles();
	void	HashNextFile();
	// SLUGFILLER: SafeHash
	bool	IsHashing(const CString& rstrDirectory, const CString& rstrName);
	void	RemoveFromHashing(CKnownFile* hashed);
	// SLUGFILLER: SafeHash

	CMap<CCKey,const CCKey&,CKnownFile*,CKnownFile*> m_Files_map;
	CTypedPtrList<CPtrList, UnknownFile_Struct*> waitingforhash_list;
	CTypedPtrList<CPtrList, UnknownFile_Struct*> currentlyhashing_list;	// SLUGFILLER: SafeHash
	CPreferences*		app_prefs;
	CServerConnect*		server;
	CSharedFilesCtrl*	output;
	uint32 m_lastPublishED2K;
	uint32 m_lastPublishED2KFlag;
	int m_currFileSrc;
	int m_currFileKey;
	uint32 m_lastPublishKadSrc;
	uint32 m_lastPublishKadKey;
};

//class CPartFile;
class CAddFileThread : public CWinThread
{
		DECLARE_DYNCREATE(CAddFileThread)
protected:
	CAddFileThread();
public:
	virtual	BOOL	InitInstance() {return true;}
	virtual int		Run();
	void	SetValues(CSharedFileList* pOwner, LPCTSTR directory, LPCTSTR filename, CPartFile* in_partfile_Owner = 0);
private:
	CSharedFileList* m_pOwner;
	CString			 strDirectory;
	CString			 strFilename;
	CPartFile*		 partfile_Owner;
};
