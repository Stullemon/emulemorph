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
// FriendListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "KademliaWnd.h"
#include "KadSearchListCtrl.h"
#include "KadContactListCtrl.h"
#include "Ini2.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CKadSearchListCtrl

enum ECols
{
	colNum = 0,
	colKey,
	colType,
	colName
};

IMPLEMENT_DYNAMIC(CKadSearchListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CKadSearchListCtrl, CMuleListCtrl)
	ON_NOTIFY_REFLECT (NM_RCLICK, OnNMRclick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
END_MESSAGE_MAP()

CKadSearchListCtrl::CKadSearchListCtrl()
{
	SetGeneralPurposeFind(true);
	m_strLVName = "KadSearchListCtrl";
}

CKadSearchListCtrl::~CKadSearchListCtrl()
{
}

void CKadSearchListCtrl::Init()
{
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	InsertColumn(colNum, GetResString(IDS_NUMBER) ,LVCFMT_LEFT,50);
	InsertColumn(colKey, GetResString(IDS_KEY) ,LVCFMT_LEFT,50);
	InsertColumn(colType, GetResString(IDS_TYPE) ,LVCFMT_LEFT,100);
	InsertColumn(colName, GetResString(IDS_SW_NAME) ,LVCFMT_LEFT,100);
	Localize();

	CString strIniFile;
	strIniFile.Format(_T("%spreferences.ini"), theApp.glob_prefs->GetConfigDir());
	CIni ini(strIniFile, "eMule");
	LoadSettings(&ini, m_strLVName);
	int iSortItem = ini.GetInt(m_strLVName + "SortItem");
	bool bSortAscending = ini.GetInt(m_strLVName + "SortAscending");
	SetSortArrow(iSortItem, bSortAscending);
	SortItems(SortProc, MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));
}

void CKadSearchListCtrl::SaveAllSettings(CIni* ini)
{
	SaveSettings(ini, m_strLVName);
	ini->WriteInt(m_strLVName + "SortItem", GetSortItem());
	ini->WriteInt(m_strLVName + "SortAscending", GetSortAscending());
}

void CKadSearchListCtrl::Localize()
{
	CImageList iml;
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.SetBkColor(CLR_NONE);
	iml.Add(CTempIconLoader("KadFileSearch"));
	iml.Add(CTempIconLoader("KadWordSearch"));
	iml.Add(CTempIconLoader("KadNodeSearch"));
	iml.Add(CTempIconLoader("KadStoreFile"));
	iml.Add(CTempIconLoader("KadStoreWord"));
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	HIMAGELIST himl = ApplyImageList(iml.Detach());
	if (himl)
		ImageList_Destroy(himl);
}

void CKadSearchListCtrl::SearchAdd(Kademlia::CSearch* search)
{
	try
	{
		ASSERT( search != NULL );
		sint32 itemnr = GetItemCount();
		InsertItem(LVIF_TEXT|LVIF_PARAM,itemnr,0,0,0,0,(LPARAM)search);
		SearchRef(search);
		CString id;
		id.Format("%s (%i)",GetResString(IDS_KADSEARCHLAB), itemnr+1);
		theApp.emuledlg->kademliawnd->GetDlgItem(IDC_KADSEARCHLAB)->SetWindowText(id);
	}
	catch(...){ASSERT(0);}
}

void CKadSearchListCtrl::SearchRem(Kademlia::CSearch* search)
{
	try
	{
		ASSERT( search != NULL );
		CPartFile* temp = theApp.downloadqueue->GetFileByKadFileSearchID(search->getSearchID());
		if(temp){
			temp->SetKadFileSearchID(0);
		}
		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)search;
		sint32 result = FindItem(&find);
		if (result != (-1)){
			DeleteItem(result);
		}
		CString id;
		id.Format("%s (%i)", GetResString(IDS_KADSEARCHLAB), GetItemCount());
		theApp.emuledlg->kademliawnd->GetDlgItem(IDC_KADSEARCHLAB)->SetWindowText(id);
	}
	catch(...){ASSERT(0);}
}

void CKadSearchListCtrl::SearchRef(Kademlia::CSearch* search)
{
	try
	{
		ASSERT( search != NULL );
		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)search;
		sint32 result = FindItem(&find);
		if (result != (-1)){
			CString id;
			id.Format( "%i", search->getSearchID());
			SetItemText(result,colNum,id);
			switch(search->getSearchTypes()){
				case Kademlia::CSearch::FILE:
					id.Format(GetResString(IDS_KAD_SEARCHSRC), search->getCount());
					SetItem(result,0,LVIF_IMAGE,0,0,0,0,0,0);
					break;
				case Kademlia::CSearch::KEYWORD:
					id.Format(GetResString(IDS_KAD_SEARCHKW), search->getCount());
					SetItem(result,0,LVIF_IMAGE,0,1,0,0,0,0);
					break;
				case Kademlia::CSearch::NODE:
				case Kademlia::CSearch::NODECOMPLETE:
					id.Format(GetResString(IDS_KAD_NODE), search->getCount());
					SetItem(result,0,LVIF_IMAGE,0,2,0,0,0,0);
					break;
				case Kademlia::CSearch::STOREFILE:
					id.Format(GetResString(IDS_KAD_STOREFILE), search->getCount(), search->getKeywordCount());
					SetItem(result,0,LVIF_IMAGE,0,3,0,0,0,0);
					break;
				case Kademlia::CSearch::STOREKEYWORD:
					id.Format(GetResString(IDS_KAD_STOREKW), search->getCount(), search->getKeywordCount());
					SetItem(result,0,LVIF_IMAGE,0,4,0,0,0,0);
					break;
				default:
					id.Format(GetResString(IDS_KAD_UNKNOWN), search->getCount());
			}
			SetItemText(result,colType,id);
			SetItemText(result,colName,search->getFileName());
			if(search->getTarget() != NULL)
			{
				search->getTarget().toHexString(&id);
				SetItemText(result,colKey,id);
			}
		}
	}
	catch(...){ASSERT(0);}
}

void CKadSearchListCtrl::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult){	
	*pResult = 0;
}

BOOL CKadSearchListCtrl::OnCommand(WPARAM wParam,LPARAM lParam ){
	return true;
}

void CKadSearchListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult) {
	*pResult = 0;
}

void CKadSearchListCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Determine ascending based on whether already sorted on this column
	int iSortItem = GetSortItem();
	bool bOldSortAscending = GetSortAscending();
	bool bSortAscending = (iSortItem != pNMListView->iSubItem) ? true : !bOldSortAscending;

	// Item is column clicked
	iSortItem = pNMListView->iSubItem;

	// Sort table
	SetSortArrow(iSortItem, bSortAscending);
	SortItems(SortProc, MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));

	*pResult = 0;
}

int CKadSearchListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){ 
	Kademlia::CSearch* item1 = (Kademlia::CSearch*)lParam1; 
	Kademlia::CSearch* item2 = (Kademlia::CSearch*)lParam2; 
	if((item1 == NULL) || (item2 == NULL))
		return 0;

	int iResult;
	switch(LOWORD(lParamSort))
	{
		case colNum:
			iResult = item1->getSearchID() - item2->getSearchID();
			break;
		case colType:
			iResult = item1->getSearchTypes() - item2->getSearchTypes();
			break;
		case colName:
			iResult = item1->getFileName().CompareNoCase(item2->getFileName());
		default:
			return 0;
	}
	if (HIWORD(lParamSort))
		iResult = -iResult;
	return iResult;
}
