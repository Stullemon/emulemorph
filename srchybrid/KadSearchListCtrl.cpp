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
#include "emule.h"
#include "KademliaWnd.h"
#include "KadSearchListCtrl.h"
#include "KadContactListCtrl.h"
#include "Ini2.h"
#include "OtherFunctions.h"
#include "emuledlg.h"
#include "DownloadQueue.h"
#include "PartFile.h"

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
	colName,
	colStop
};

IMPLEMENT_DYNAMIC(CKadSearchListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CKadSearchListCtrl, CMuleListCtrl)
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
END_MESSAGE_MAP()

CKadSearchListCtrl::CKadSearchListCtrl()
{
	SetGeneralPurposeFind(true);
	m_strLVName = _T("KadSearchListCtrl");
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
	InsertColumn(colStop, GetResString(IDS_STATUS),LVCFMT_LEFT,100);
	SetAllIcons();
	Localize();

	CString strIniFile;
	strIniFile.Format(_T("%spreferences.ini"), thePrefs.GetConfigDir());
	CIni ini(strIniFile, _T("eMule"));
	LoadSettings(&ini, m_strLVName);
	int iSortItem = ini.GetInt(m_strLVName + _T("SortItem"));
	bool bSortAscending = ini.GetInt(m_strLVName + _T("SortAscending"));
	SetSortArrow(iSortItem, bSortAscending);
	SortItems(SortProc, MAKELONG(iSortItem, (bSortAscending ? 0 : 0x0001)));
}

void CKadSearchListCtrl::UpdateKadSearchCount() {
	CString id;
	id.Format(_T("%s (%i)"),GetResString(IDS_KADSEARCHLAB), GetItemCount() );
	theApp.emuledlg->kademliawnd->GetDlgItem(IDC_KADSEARCHLAB)->SetWindowText(id);
}

void CKadSearchListCtrl::SaveAllSettings(CIni* ini)
{
	SaveSettings(ini, m_strLVName);
	ini->WriteInt(m_strLVName + _T("SortItem"), GetSortItem());
	ini->WriteInt(m_strLVName + _T("SortAscending"), GetSortAscending());
}

void CKadSearchListCtrl::OnSysColorChange()
{
	CMuleListCtrl::OnSysColorChange();
	SetAllIcons();
}

void CKadSearchListCtrl::SetAllIcons()
{
	CImageList iml;
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.SetBkColor(CLR_NONE);
	iml.Add(CTempIconLoader(_T("KadFileSearch")));
	iml.Add(CTempIconLoader(_T("KadWordSearch")));
	iml.Add(CTempIconLoader(_T("KadNodeSearch")));
	iml.Add(CTempIconLoader(_T("KadStoreFile")));
	iml.Add(CTempIconLoader(_T("KadStoreWord")));
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	HIMAGELIST himl = ApplyImageList(iml.Detach());
	if (himl)
		ImageList_Destroy(himl);
}

void CKadSearchListCtrl::Localize()
{
	// who let this empty?
	// masta notices those things
	// and ornis have to do the slavework :)

	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	for (int icol=0;icol<pHeaderCtrl->GetItemCount();icol++) {
		switch (icol) {
			case 0: strRes = GetResString(IDS_NUMBER); break;
			case 1: strRes = GetResString(IDS_KEY); break;
			case 2: strRes = GetResString(IDS_TYPE); break;
			case 3: strRes = GetResString(IDS_SW_NAME); break;
			case 4: strRes = GetResString(IDS_STATUS); break;
		}
	
		hdi.pszText = strRes.GetBuffer();
		pHeaderCtrl->SetItem(icol, &hdi);
		strRes.ReleaseBuffer();
	}

	int iItems = GetItemCount();
	for (int i = 0; i < iItems; i++)
		SearchRef((Kademlia::CSearch*)GetItemData(i));
}

void CKadSearchListCtrl::UpdateSearch(int iItem, const Kademlia::CSearch* search)
{
	CString id;
	id.Format(_T("%i"), search->getSearchID());
	SetItemText(iItem,colNum,id);

	switch(search->getSearchTypes()){
		case Kademlia::CSearch::FILE:
			id.Format(GetResString(IDS_KAD_SEARCHSRC), search->getCount(), search->getCountSent());
			SetItem(iItem,0,LVIF_IMAGE,0,0,0,0,0,0);
			break;
		case Kademlia::CSearch::KEYWORD:
			id.Format(GetResString(IDS_KAD_SEARCHKW), search->getCount(), search->getCountSent());
			SetItem(iItem,0,LVIF_IMAGE,0,1,0,0,0,0);
			break;
		case Kademlia::CSearch::NODE:
		case Kademlia::CSearch::NODECOMPLETE:
			id.Format(GetResString(IDS_KAD_NODE), search->getCount(), search->getCountSent());
			SetItem(iItem,0,LVIF_IMAGE,0,2,0,0,0,0);
			break;
		case Kademlia::CSearch::STOREFILE:
			id.Format(GetResString(IDS_KAD_STOREFILE), search->getCount(), search->getCountSent());
			SetItem(iItem,0,LVIF_IMAGE,0,3,0,0,0,0);
			break;
		case Kademlia::CSearch::STOREKEYWORD:
			id.Format(GetResString(IDS_KAD_STOREKW), search->getCount(), search->getCountSent());
			SetItem(iItem,0,LVIF_IMAGE,0,4,0,0,0,0);
			break;
		default:
			id.Format(GetResString(IDS_KAD_UNKNOWN), search->getCount(), search->getCountSent());
	}
	SetItemText(iItem,colType,id);

	SetItemText(iItem,colName,search->getFileName());

	if(search->getTarget() != NULL)
	{
		search->getTarget().toHexString(&id);
		SetItemText(iItem,colKey,id);
	}

	if(search->Stoping())
		SetItemText(iItem,colStop,_T("Stopping"));
	else
		SetItemText(iItem,colStop,_T("Active"));
}

void CKadSearchListCtrl::SearchAdd(const Kademlia::CSearch* search)
{
	try
	{
		ASSERT( search != NULL );
		int iItem = InsertItem(LVIF_TEXT|LVIF_PARAM,GetItemCount(),NULL,0,0,0,(LPARAM)search);
		if (iItem >= 0)
		{
			UpdateSearch(iItem, search);
			UpdateKadSearchCount();
		}
	}
	catch(...){ASSERT(0);}
}

void CKadSearchListCtrl::SearchRem(const Kademlia::CSearch* search)
{
	try
	{
		ASSERT( search != NULL );
		CPartFile* temp = theApp.downloadqueue->GetFileByKadFileSearchID(search->getSearchID());
		if(temp)
		{
			temp->SetKadFileSearchID(0);
		}

		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)search;
		int iItem = FindItem(&find);
		if (iItem != -1)
		{
			DeleteItem(iItem);
			UpdateKadSearchCount();
		}
	}
	catch(...)
	{
		ASSERT(0);
	}
}

void CKadSearchListCtrl::SearchRef(const Kademlia::CSearch* search)
{
	try
	{
		ASSERT( search != NULL );
		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)search;
		int iItem = FindItem(&find);
		if (iItem != -1)
			UpdateSearch(iItem, search);
	}
	catch(...){ASSERT(0);}
}

BOOL CKadSearchListCtrl::OnCommand(WPARAM wParam,LPARAM lParam ){
	return true;
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
