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
#include "MuleListCtrl.h"
#include "TitleMenu.h"

// CSharedFilesCtrl
class CSharedFileList;
class CKnownFile;
class CPartFile; //MORPH - Added by SiRoB, delete shared file

class CSharedFilesCtrl : public CMuleListCtrl
{
	DECLARE_DYNAMIC(CSharedFilesCtrl)

public:
	CSharedFilesCtrl();
	virtual ~CSharedFilesCtrl();
	void	Init();
	void	CreateMenues();
	void	ShowFileList(CSharedFileList* in_sflist);
	void	ShowFile(CKnownFile* file);
	void	RemoveFile(CKnownFile* toremove);
	void	UpdateFile(CKnownFile* file);
	void	Localize();
	void	ShowFilesCount();
	void	ShowComments(int index);
protected:
	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()
	afx_msg	void OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult);
private:
	CImageList  m_ImageList;	//MORPH - Added by IceCream, SLUGFILLER: showComments
	CTitleMenu	m_SharedFilesMenu;
	//MORPH START - Added by SiRoB, ZZ Upload System
	CMenu       m_PowershareMenu;
	//MORPH END - Added by SiRoB, ZZ Upload System
	CMenu		m_PrioMenu;
	CMenu		m_PermMenu;
	CSharedFileList* sflist;
	bool		sortstat[3];
	void		OpenFile(CKnownFile* file);
	//MORPH START - Added by SiRoB, About Open File Folder entry
	void		OpenFileFolder(CKnownFile* file);
	//MORPH END - Added by SiRoB, About Open File Folder entry
	// EastShare Start added by linekin, TBH delete shared file
	void		DeleteFileFromHDPart(CPartFile* file);
	void		DeleteFileFromHDByKnown(CKnownFile* file);
	void		DeleteFileFromHD(CKnownFile* file);
	// EastShare End
};
