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

#define AVBLYSHADECOUNT 13

class CSearchList;
class CSearchFile;

struct SearchCtrlItem_Struct{
   CSearchFile*		value;
   CSearchFile*     owner;
   uchar			filehash[16];
   uint16			childcount;
};

class CSortSelectionState{
public:
	uint32	m_nSortItem;
	bool	m_bSortAscending;
	uint32	m_nScrollPosition;
	CArray<int, int>	m_aSelectedItems;
};

class CSearchListCtrl : public CMuleListCtrl, public CListCtrlItemWalk
{
	DECLARE_DYNAMIC(CSearchListCtrl)

public:
	CSearchListCtrl();
	virtual ~CSearchListCtrl();

	void	Init(CSearchList* in_searchlist);
	void	CreateMenues();
	void	UpdateSources(const CSearchFile* toupdate);
	void	AddResult(const CSearchFile* toshow);
	void	RemoveResult(const CSearchFile* toremove);
	void	Localize();
	void	ShowResults(uint32 nResultsID);
	void	ClearResultViewState(uint32 nResultsID);
	void	NoTabs()	{ m_nResultsID = 0; }

protected:
	uint32		m_nResultsID;
	CTitleMenu	m_SearchFileMenu;
	CSearchList* searchlist;

	COLORREF	m_crSearchResultDownloading;
	COLORREF	m_crSearchResultDownloadStopped;
	COLORREF	m_crSearchResultKnown;
	COLORREF	m_crSearchResultShareing;
	COLORREF	m_crSearchResultCancelled;
	COLORREF	m_crShades[AVBLYSHADECOUNT];

	CMap<int,int, CSortSelectionState*, CSortSelectionState*> 	m_mapSortSelectionStates;

	COLORREF GetSearchItemColor(/*const*/ CSearchFile* src);
	CString GetCompleteSourcesDisplayString(const CSearchFile* pFile, UINT uSources, bool* pbComplete = NULL) const;
	void	ExpandCollapseItem(int iItem, int iAction);
	void	HideSources(CSearchFile* toCollapse);
	void	SetStyle();
	void	SetHighlightColors();

	void	DrawSourceParent(CDC *dc, int nColumn, LPRECT lpRect, /*const*/ CSearchFile* src);
	void	DrawSourceChild(CDC *dc, int nColumn, LPRECT lpRect, /*const*/ CSearchFile* src);

	static int Compare(const CSearchFile* item1, const CSearchFile* item2, LPARAM lParamSort);
	static int CompareChild(const CSearchFile* file1, const CSearchFile* file2, LPARAM lParamSort);
	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnSysColorChange();
	afx_msg	void OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnLvnDeleteallitems(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDblClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLvnKeydown(NMHDR *pNMHDR, LRESULT *pResult);
};
