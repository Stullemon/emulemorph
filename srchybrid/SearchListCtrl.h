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
#include "MuleListCtrl.h"

// CSearchListCtrl
class CSearchList;
class CSearchFile;

//enum ItemType {ITEMTYPE_PARENT = 1, ITEMTYPE_CHILD= 2};
struct SearchCtrlItem_Struct{
   //ItemType         type;
   CSearchFile*		value;
   CSearchFile*     owner;
   uchar			filehash[16];
   uint16			childcount;
//   ~SearchCtrlItem_Struct() { status.DeleteObject(); }
};

class CSearchListCtrl : public CMuleListCtrl
{
	DECLARE_DYNAMIC(CSearchListCtrl)

public:
	CSearchListCtrl();
	virtual ~CSearchListCtrl();
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);

	void	Init(CSearchList* in_searchlist);
	void	CreateMenues();
	void	UpdateSources(CSearchFile* toupdate);
	void	AddResult(CSearchFile* toshow);
	void	RemoveResult( CSearchFile* toremove);
	void	Localize();
	void	ShowResults(uint32 nResultsID);
	void	NoTabs()	{ m_nResultsID = 0; }
	uint32	m_nResultsID;

protected:
	int m_iColumns;
	bool m_bSetImageList;
	void	DrawSourceParent(CDC *dc, int nColumn, LPRECT lpRect, CSearchFile* src);
	void	DrawSourceChild(CDC *dc, int nColumn, LPRECT lpRect, CSearchFile* src);

	static int Compare(CSearchFile* item1, CSearchFile* item2, LPARAM lParamSort);
	static int CompareChild(CSearchFile* file1, CSearchFile* file2, LPARAM lParamSort);

	static int CALLBACK SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg	void OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnLvnDeleteallitems(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDblClick(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()
private:
	CTitleMenu	 m_SearchFileMenu;
	CSearchList* searchlist;

	void	ExpandCollapseItem(int item);
	void	HideSources(CSearchFile* toCollapse);
	void	SetStyle();
};
