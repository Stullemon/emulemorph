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

#include "stdafx.h"
#include "CollectionListCtrl.h"
#include "OtherFunctions.h"
#include "AbstractFile.h"
#include "FileDetailDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

enum ECols
{
	colName = 0,
	colSize,
	colHash
};

IMPLEMENT_DYNAMIC(CCollectionListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CCollectionListCtrl, CMuleListCtrl)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_RCLICK, OnNMRclick)
END_MESSAGE_MAP()

CCollectionListCtrl::CCollectionListCtrl()
	: CListCtrlItemWalk(this)
{
}

CCollectionListCtrl::~CCollectionListCtrl()
{
}

void CCollectionListCtrl::Init(CString strNameAdd)
{
	SetName(_T("CollectionListCtrl") + strNameAdd);
	ModifyStyle(LVS_SINGLESEL,0);
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	InsertColumn(colName,GetResString(IDS_DL_FILENAME),LVCFMT_LEFT,250);
	InsertColumn(colSize,GetResString(IDS_DL_SIZE),LVCFMT_LEFT,100);
	InsertColumn(colHash,GetResString(IDS_FILEHASH),LVCFMT_LEFT,250);
	LoadSettings();

	SetSortArrow();
	SortItems(SortProc, MAKELONG(GetSortItem(), (GetSortAscending() ? 0:1)));
}

int CCollectionListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CAbstractFile* item1 = (CAbstractFile*)lParam1;
	CAbstractFile* item2 = (CAbstractFile*)lParam2; 
	if((item1 == NULL) || (item2 == NULL))
		return 0;

	int iResult;
	switch(LOWORD(lParamSort))
	{
		case colName:
			iResult = CompareLocaleStringNoCase(item1->GetFileName(),item2->GetFileName());
			break;
		case colSize:
			iResult = item1->GetFileSize()==item2->GetFileSize()?0:(item1->GetFileSize()>item2->GetFileSize()?1:-1);
			break;
		case colHash:
			iResult = memcmp(item1->GetFileHash(), item2->GetFileHash(), 16);
			break;
		default:
			return 0;
	}
	if (HIWORD(lParamSort))
		iResult = -iResult;
	return iResult;
}

void CCollectionListCtrl::OnColumnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Determine ascending based on whether already sorted on this column
	int iSortItem = GetSortItem();
	bool bOldSortAscending = GetSortAscending();
	bool bSortAscending = (iSortItem != pNMListView->iSubItem) ? true : !bOldSortAscending;

	// Item is column clicked
	iSortItem = pNMListView->iSubItem;

	// Sort table
	UpdateSortHistory(MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));
	SetSortArrow(iSortItem, bSortAscending);
	SortItems(SortProc, MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));

	*pResult = 0;
}

void CCollectionListCtrl::AddFileToList(CAbstractFile* pAbstractFile)
{
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)pAbstractFile;
	int iItem = FindItem(&find);
	if (iItem != -1)
	{
		ASSERT(0);
		return;
	}

	iItem = InsertItem(LVIF_TEXT|LVIF_PARAM,GetItemCount(),NULL,0,0,0,(LPARAM)pAbstractFile);
	if (iItem != -1)
	{
		SetItemText(iItem,colName,pAbstractFile->GetFileName());
		SetItemText(iItem,colSize,CastItoXBytes(pAbstractFile->GetFileSize()));
		SetItemText(iItem,colHash,::md4str(pAbstractFile->GetFileHash()));
	}
}

void CCollectionListCtrl::RemoveFileFromList(CAbstractFile* pAbstractFile)
{
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)pAbstractFile;
	int iItem = FindItem(&find);
	if (iItem != -1)
		DeleteItem(iItem);
	else
		ASSERT(0);
}

void CCollectionListCtrl::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	CTypedPtrList<CPtrList, CAbstractFile*> abstractFileList;
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos != NULL)
	{
		int index = GetNextSelectedItem(pos);
		if (index >= 0)
			abstractFileList.AddTail((CAbstractFile*)GetItemData(index));
	}

	if(abstractFileList.GetCount() > 0)
	{
		CFileDetailDialog dialog(abstractFileList, 0, this);
		dialog.DoModal();
	}
	*pResult = 0;
}
