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
#include <map>

#define COLLAPSE_ONLY	0
#define EXPAND_ONLY		1
#define EXPAND_COLLAPSE	2

// Foward declaration
class CPartFile;
class CUpDownClient;

enum ItemType {FILE_TYPE = 1, AVAILABLE_SOURCE = 2, UNAVAILABLE_SOURCE = 3};
struct CtrlItem_Struct{
   ItemType         type;
   CPartFile*       owner;
   void*            value; // could be both CPartFile or CUpDownClient
   CtrlItem_Struct* parent;
   DWORD            dwUpdated;
   CBitmap          status;
   ~CtrlItem_Struct() { status.DeleteObject(); }
};

// CDownloadListCtrl
class CDownloadListCtrl : public CMuleListCtrl
{
	DECLARE_DYNAMIC(CDownloadListCtrl)

public:
	CDownloadListCtrl();
	virtual ~CDownloadListCtrl();

	uint8	curTab;
	void	UpdateItem(void* toupdate);
	void	Init();
	void	AddFile(CPartFile* toadd);
	void	AddSource(CPartFile* owner,CUpDownClient* source,bool notavailable);
	void	RemoveSource(CUpDownClient* source,CPartFile* owner);
	bool	RemoveFile(const CPartFile* toremove);
	void	ClearCompleted(bool ignorecats=false);
	void	ClearCompleted(const CPartFile* pFile);
	void	SetStyle();
	void	CreateMenues();
	void	Localize();
	void	HideSources(CPartFile* toCollapse,bool isShift = false,bool isCtrl = false,bool isAlt = false);
	void	ShowFilesCount();
	void	ChangeCategory(int newsel);
	CString getTextList();
	void	ShowSelectedFileDetails();
	void	HideFile(CPartFile* tohide);
	void	ShowFile(CPartFile* tohide);
	void	ExpandCollapseItem(int item,uint8 expand,bool collapsesource=false);
	void	GetDisplayedFiles(CArray<CPartFile*,CPartFile*> *list);
	void	MoveCompletedfilesCat(uint8 from, uint8 to);
	int		GetCompleteDownloads(int cat,int &total);
protected:
	CImageList  m_ImageList;
	// Mighty Knife: Community visualization
	CImageList m_overlayimages;
	// [end] Mighty Knife

	CMenu		m_PrioMenu;
	CTitleMenu	m_FileMenu;
	CMenu		m_A4AFMenu;
	
	//MORPH - Removed by SiRoB, Remain time and size Columns have been splited
	/*
	bool		m_bRemainSort;
	*/

	CMenu		m_PermMenu;	// xMule_MOD: showSharePermissions
	CMenu		m_A4AFMenuFlag; //MORPH - Added by SiRoB, Advanced A4AF Flag derivated from Khaos

	typedef std::pair<void*, CtrlItem_Struct*> ListItemsPair;
	typedef std::multimap<void*, CtrlItem_Struct*> ListItems;
    ListItems	m_ListItems;

	void ShowFileDialog(CPartFile* pFile = NULL, bool bOpenCommentsPage = false);
	void SetAllIcons();
	void DrawFileItem(CDC *dc, int nColumn, LPRECT lpRect, CtrlItem_Struct *lpCtrlItem);
	void DrawSourceItem(CDC *dc, int nColumn, LPRECT lpRect, CtrlItem_Struct *lpCtrlItem);
    static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
    static int Compare(const CPartFile* file1, const CPartFile* file2, LPARAM lParamSort);
    static int Compare(const CUpDownClient* client1, const CUpDownClient* client2, LPARAM lParamSort, int sortMod);

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSysColorChange();
	afx_msg void OnItemActivate(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg	void OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnListModified(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkDownloadlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};
