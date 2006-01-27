//this file is part of eMule
//Copyright (C)2002-2006 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "MuleListCtrl.h"
#include "TitleMenu.h"
#include "ListCtrlItemWalk.h"

// Mighty Knife: CRC32-Tag
#include "FileProcessing.h"
// [end] Mighty Knife

class CSharedFileList;
class CKnownFile;
class CDirectoryItem;

class CSharedFilesCtrl : public CMuleListCtrl, public CListCtrlItemWalk
{
	friend class CSharedDirsTreeCtrl;
	DECLARE_DYNAMIC(CSharedFilesCtrl)

public:
	CSharedFilesCtrl();
	virtual ~CSharedFilesCtrl();

	void	Init();
	void	CreateMenues();
	void	ReloadFileList();
	void	AddFile(const CKnownFile* file);
	void	RemoveFile(const CKnownFile* file);
	void	UpdateFile(const CKnownFile* file);
	void	Localize();
	void	ShowFilesCount();
	void	ShowComments(CKnownFile* file);
	void	SetAICHHashing(uint32 nVal)				{ nAICHHashing = nVal; } 
	void	SetDirectoryFilter(CDirectoryItem* pNewFilter, bool bRefresh = true);

protected:
	CTitleMenu	m_SharedFilesMenu;
	CTitleMenu		m_CollectionsMenu;
	CMenu		m_PrioMenu;
	CMenu		m_PermMenu; //MORPH START - Added by SiRoB, Keep Permission flag
	CMenu       m_PowershareMenu; //MORPH - Added by SiRoB, ZZ Upload System
	CMenu		m_SpreadbarMenu; //MORPH	- Added by AndCycle, SLUGFILLER: Spreadbars - per file
	CMenu		m_HideOSMenu; //MORPH - Added by SiRoB, HIDEOS
	CMenu		m_SelectiveChunkMenu; //MORPH - Added by SiRoB, HIDEOS
	CMenu		m_ShareOnlyTheNeedMenu; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
	CMenu		m_PowerShareLimitMenu; //MORPH - Added by SiRoB, POWERSHARE Limit
	CMenu		m_CRC32Menu; //MORPH - Added by SiRoB, CRC32-Tag
	bool		sortstat[4];
	CImageList	m_ImageList;
	CDirectoryItem*	m_pDirectoryFilter;
	volatile uint32 nAICHHashing;

	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	void		OpenFile(const CKnownFile* file);
	void ShowFileDialog(CTypedPtrList<CPtrList, CKnownFile*>& aFiles, UINT uPshInvokePage = 0);
	void SetAllIcons();
	int FindFile(const CKnownFile* pFile);

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSysColorChange();
	afx_msg	void OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	// Mighty Knife: CRC32-Tag
	CFileProcessingThread m_FileProcessingThread;
	afx_msg LRESULT OnCRC32RenameFile (WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCRC32UpdateFile (WPARAM wParam, LPARAM lParam);
	// [end] Mighty Knife
};
