//this file is part of eMule
//Copyright (C)2002 Merkur ( devs@emule-project.net / http://www.emule-project.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#pragma once
#include <list>

/*
					   CPartFile
					 /
		  CKnownFile
		/
CAbstractFile - CCollectionFile
		\
		  CSearchFile
*/

namespace Kademlia
{
	class CUInt128;
	class CEntry;
	typedef std::list<CStringW> WordList;
};

class CTag;

typedef CTypedPtrList<CPtrList, Kademlia::CEntry*> CKadEntryPtrList;

class CAbstractFile: public CObject
{
	DECLARE_DYNAMIC(CAbstractFile)

public:
	CAbstractFile();
	CAbstractFile(const CAbstractFile* pAbstractFile);
	virtual ~CAbstractFile();

	const CString& GetFileName() const { return m_strFileName; }
	virtual void SetFileName(LPCTSTR pszFileName, bool bReplaceInvalidFileSystemChars = false, bool bAutoSetFileType = true); // 'bReplaceInvalidFileSystemChars' is set to 'false' for backward compatibility!

	// returns the ED2K file type (an ASCII string)
	const CString& GetFileType() const { return m_strFileType; }
	virtual void SetFileType(LPCTSTR pszFileType);

	// returns the file type which is used to be shown in the GUI
	CString GetFileTypeDisplayStr() const;

	const uchar* GetFileHash() const { return m_abyFileHash; }
	void SetFileHash(const uchar* pucFileHash);
	bool HasNullHash() const;

	uint32 GetFileSize() const { return m_nFileSize; }
	virtual void SetFileSize(uint32 nFileSize) { m_nFileSize = nFileSize; }

	uint32 GetIntTagValue(uint8 tagname) const;
	uint32 GetIntTagValue(LPCSTR tagname) const;
	bool GetIntTagValue(uint8 tagname, uint32& ruValue) const;
	void SetIntTagValue(uint8 tagname, uint32 uValue);
	const CString& GetStrTagValue(uint8 tagname) const;
	const CString& GetStrTagValue(LPCSTR tagname) const;
	void SetStrTagValue(uint8 tagname, LPCTSTR);
	CTag* GetTag(uint8 tagname, uint8 tagtype) const;
	CTag* GetTag(LPCSTR tagname, uint8 tagtype) const;
	CTag* GetTag(uint8 tagname) const;
	CTag* GetTag(LPCSTR tagname) const;
	const CArray<CTag*, CTag*>& GetTags() const { return taglist; }
	void AddTagUnique(CTag* pTag);
	void ClearTags();
	void CopyTags(const CArray<CTag*, CTag*>& tags);
	virtual bool IsPartFile() const { return false; }

	bool	HasComment() const { return m_bHasComment; }
	void	SetHasComment(bool in) { m_bHasComment = in; }
	uint32	UserRating() const { return m_uUserRating; }
	bool	HasRating()	const { return m_uUserRating > 0; }
	bool	HasBadRating()	const { return ( HasRating() && (m_uUserRating < 2)); }
	void	SetUserRating(uint32 in) { m_uUserRating = in; }
	const CString& GetFileComment() /*const*/;
	uint8	GetFileRating() /*const*/;
	void	LoadComment();
	virtual void	UpdateFileRatingCommentAvail() = 0;

	bool AddNote(Kademlia::CEntry* pEntry);
	const CKadEntryPtrList& getNotes() const { return m_kadNotes; }

#ifdef _DEBUG
	// Diagnostic Support
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CString m_strFileName;
	uchar	m_abyFileHash[16];
	uint32	m_nFileSize;
	CString m_strComment;
	uint8	m_uRating;
	bool	m_bCommentLoaded;
	uint8	m_uUserRating;
	bool	m_bHasComment;
	CString m_strFileType;
	CArray<CTag*, CTag*> taglist;
	CKadEntryPtrList m_kadNotes;
};
