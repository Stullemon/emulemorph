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
#include "MuleListCtrl.h"
#include "TitleMenu.h"
#include <map>
#include "ListCtrlItemWalk.h"

#define COLLAPSE_ONLY	0
#define EXPAND_ONLY		1
#define EXPAND_COLLAPSE	2

// Foward declaration
class CPartFile;
class CUpDownClient;
class CDownloadListCtrl;
class CToolTipCtrlX;


///////////////////////////////////////////////////////////////////////////////
// CtrlItem_Struct

enum ItemType {FILE_TYPE = 1, AVAILABLE_SOURCE = 2, UNAVAILABLE_SOURCE = 3};

class CtrlItem_Struct : public CObject
{
	DECLARE_DYNAMIC(CtrlItem_Struct)

public:
	~CtrlItem_Struct() { status.DeleteObject(); }

	ItemType         type;
	CPartFile*       owner;
	void*            value; // could be both CPartFile or CUpDownClient
	CtrlItem_Struct* parent;
	DWORD            dwUpdated;
	CBitmap          status;
	DWORD            dwUpdatedchunk; //MORPH
	CBitmap          statuschunk;	//MORPH - Downloading Chunk Detail Display
};


///////////////////////////////////////////////////////////////////////////////
// CDownloadListListCtrlItemWalk

class CDownloadListListCtrlItemWalk : public CListCtrlItemWalk
{
public:
	CDownloadListListCtrlItemWalk(CDownloadListCtrl* pListCtrl);

	virtual CObject* GetNextSelectableItem();
	virtual CObject* GetPrevSelectableItem();

	void SetItemType(ItemType eItemType) { m_eItemType = eItemType; }

protected:
	CDownloadListCtrl* m_pDownloadListCtrl;
	ItemType m_eItemType;
};


///////////////////////////////////////////////////////////////////////////////
// CDownloadListCtrl

class CDownloadListCtrl : public CMuleListCtrl, public CDownloadListListCtrlItemWalk
{
	DECLARE_DYNAMIC(CDownloadListCtrl)
	friend class CDownloadListListCtrlItemWalk;
	friend class CDownloadClientsCtrl; //SLAHAM: ADDED DownloadClientsCtrl

public:
	CDownloadListCtrl();
	virtual ~CDownloadListCtrl();

	UINT	curTab;

	void	UpdateItem(void* toupdate);
	void	Init();
	void	AddFile(CPartFile* toadd);
	void	AddSource(CPartFile* owner, CUpDownClient* source, bool notavailable);
	void	RemoveSource(CUpDownClient* source, CPartFile* owner);
	bool	RemoveFile(const CPartFile* toremove);
	void	ClearCompleted(int incat=-2);
	void	ClearCompleted(const CPartFile* pFile);
	void	SetStyle();
	void	CreateMenues();
	void	Localize();
	void	ShowFilesCount();
	void	ChangeCategory(int newsel);
	CString getTextList();
	void	ShowSelectedFileDetails();
	void	HideFile(CPartFile* tohide);
	void	ShowFile(CPartFile* tohide);
	void	ExpandCollapseItem(int iItem, int iAction, bool bCollapseSource = false);
	void	HideSources(CPartFile* toCollapse);
	void	GetDisplayedFiles(CArray<CPartFile*, CPartFile*>* list);
	void	MoveCompletedfilesCat(uint8 from, uint8 to);
	int		GetCompleteDownloads(int cat,int &total);
	void	UpdateCurrentCategoryView();
	void	UpdateCurrentCategoryView(CPartFile* thisfile);
	CImageList *CreateDragImage(int iItem, LPPOINT lpPoint);
	void	FillCatsMenu(CMenu& rCatsMenu, int iFilesInCats = (-1));
	CTitleMenu* GetPrioMenu();
	float	GetFinishedSize();
	bool	ReportAvailableCommands(CList<int>& liAvailableCommands);

protected:
	CImageList  m_ImageList;
	// Mighty Knife: Community visualization
	CImageList m_overlayimages;
	// [end] Mighty Knife

	CTitleMenu	m_PrioMenu;
	CTitleMenu	m_FileMenu;
	CTitleMenu	m_PreviewMenu;
	//MORPH - Added icons
	/*
	CMenu		m_SourcesMenu;
	*/
	CTitleMenu	m_SourcesMenu;
	//MORPH - ADDED icons
	//MORPH - Removed by SiRoB, Remain time and size Columns have been splited
	/*
	bool		m_bRemainSort;
	*/

	CTitleMenu		m_PermMenu;	// xMule_MOD: showSharePermissions
	CTitleMenu		m_A4AFMenuFlag; //MORPH - Added by SiRoB, Advanced A4AF Flag derivated from Khaos

	typedef std::pair<void*, CtrlItem_Struct*> ListItemsPair;
	typedef std::multimap<void*, CtrlItem_Struct*> ListItems;
    ListItems	m_ListItems;
	CFont		m_fontBold; // may contain a locally created bold font
	CFont*		m_pFontBold;// points to the bold font which is to be used (may be the locally created or the default bold font)
	CFont		m_fontSmaller;//MORPH - Draw Display Chunk Detail
	CFont		m_fontBoldSmaller;//MORPH - Added by SiRoB, Draw Client Percentage
	CToolTipCtrlX* m_tooltip;
	uint32		m_dwLastAvailableCommandsCheck;
	bool		m_availableCommandsDirty;

	void ShowFileDialog(UINT uInvokePage);
	void ShowClientDialog(CUpDownClient* pClient);
	void SetAllIcons();
	void DrawFileItem(CDC *dc, int nColumn, LPCRECT lpRect, UINT uDrawTextAlignment, CtrlItem_Struct *pCtrlItem);
	void DrawSourceItem(CDC *dc, int nColumn, LPCRECT lpRect, UINT uDrawTextAlignment, CtrlItem_Struct *pCtrlItem);
	int GetFilesCountInCurCat();
	void GetFileItemDisplayText(CPartFile *lpPartFile, int iSubItem, LPTSTR pszText, int cchTextMax);
	void GetSourceItemDisplayText(const CtrlItem_Struct *pCtrlItem, int iSubItem, LPTSTR pszText, int cchTextMax);

	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
    static int Compare(const CPartFile* file1, const CPartFile* file2, LPARAM lParamSort);
	//MORPH START - Keep A4AF Infos
	/*
    static int Compare(const CUpDownClient* client1, const CUpDownClient* client2, LPARAM lParamSort);
	*/
	static int Compare(const CUpDownClient* client1, const CUpDownClient* client2, LPARAM lParamSort, const CPartFile* file);
	//MORPH END   - A4AF Infos

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnListModified(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnColumnClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemActivate(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNmDblClk(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSysColorChange();
};
