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
#include "KadContactListCtrl.h"
#include "Ini2.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CONContactListCtrl

enum ECols
{
	colID = 0,
	colType,
	colContact,
	colDistance
};

IMPLEMENT_DYNAMIC(CKadContactListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CKadContactListCtrl, CMuleListCtrl)
	ON_NOTIFY_REFLECT (NM_RCLICK, OnNMRclick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CKadContactListCtrl::CKadContactListCtrl()
{
	SetGeneralPurposeFind(true);
	m_strLVName = "ONContactListCtrl";
}

CKadContactListCtrl::~CKadContactListCtrl()
{
}

void CKadContactListCtrl::Init()
{
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);

	InsertColumn(colID,"ID",LVCFMT_LEFT,100);
	InsertColumn(colType,GetResString(IDS_TYPE) ,LVCFMT_LEFT,50);
	InsertColumn(colContact, GetResString(IDS_KADCONTACTLAB) ,LVCFMT_LEFT,50);
	InsertColumn(colDistance,GetResString(IDS_KADDISTANCE),LVCFMT_LEFT,50);
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

void CKadContactListCtrl::SaveAllSettings(CIni* ini)
{
	SaveSettings(ini, m_strLVName);
	ini->WriteInt(m_strLVName + "SortItem", GetSortItem());
	ini->WriteInt(m_strLVName + "SortAscending", GetSortAscending());
}

void CKadContactListCtrl::Localize()
{
	CImageList iml;
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.SetBkColor(CLR_NONE);
	iml.Add(CTempIconLoader("Contact0"));
	iml.Add(CTempIconLoader("Contact2"));
	iml.Add(CTempIconLoader("Contact4"));
	//right now we only have 3 types... But this may change in the future..
	iml.Add(CTempIconLoader("Contact3"));
	iml.Add(CTempIconLoader("Contact4"));
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	HIMAGELIST himl = ApplyImageList(iml.Detach());
	if (himl)
		ImageList_Destroy(himl);
}

void CKadContactListCtrl::ContactAdd(Kademlia::CContact* contact)
{
	try
	{
		ASSERT( contact != NULL );
		uint32 itemnr = GetItemCount();
		InsertItem(LVIF_TEXT|LVIF_PARAM,itemnr,0,0,0,0,(LPARAM)contact);
		ContactRef(contact);
		CString id;
		id.Format("%s (%i)", GetResString(IDS_KADCONTACTLAB) , itemnr+1);
		theApp.emuledlg->kademliawnd->GetDlgItem(IDC_KADCONTACTLAB)->SetWindowText(id);
	}
	catch(...){ASSERT(0);}
}

void CKadContactListCtrl::ContactRem(Kademlia::CContact* contact)
{
	try
	{
		ASSERT( contact != NULL );
		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)contact;
		sint32 result = FindItem(&find);
		if (result != (-1)){
			DeleteItem(result);
		}
		CString id;
		id.Format("%s (%i)", GetResString(IDS_KADCONTACTLAB) , GetItemCount());
		theApp.emuledlg->kademliawnd->GetDlgItem(IDC_KADCONTACTLAB)->SetWindowText(id);
	}
	catch(...){ASSERT(0);}
}

void CKadContactListCtrl::ContactRef(Kademlia::CContact* contact)
{
	try
	{
		ASSERT( contact != NULL );
		LVFINDINFO find;
		find.flags = LVFI_PARAM;
		find.lParam = (LPARAM)contact;
		sint32 result = FindItem(&find);
		if (result != (-1)){
			SetItem(result,0,LVIF_IMAGE,0,contact->getType()>4?4:contact->getType(),0,0,0,0);

			CString id;
			contact->getClientID(&id);
			SetItemText(result,colID,id);
			
			id.Format("%i",contact->getType());
			SetItemText(result,colType,id);
			
			if(contact->madeContact())
				id = GetResString(IDS_YES);
			else
				id = GetResString(IDS_NO);
			SetItemText(result,colContact,id);

			contact->getDistance(&id);
			SetItemText(result,colDistance,id);
		}
	}
	catch(...){ASSERT(0);}
}

void CKadContactListCtrl::OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
}

BOOL CKadContactListCtrl::OnCommand(WPARAM wParam,LPARAM lParam)
{
	return TRUE;
}

void CKadContactListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
}

void CKadContactListCtrl::OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult)
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

int CKadContactListCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	Kademlia::CContact* item1 = (Kademlia::CContact*)lParam1;
	Kademlia::CContact* item2 = (Kademlia::CContact*)lParam2; 
	if((item1 == NULL) || (item2 == NULL))
		return 0;

	int iResult;
	switch(LOWORD(lParamSort))
	{
		case colID:
		{
			Kademlia::CUInt128 i1;
			Kademlia::CUInt128 i2;
			item1->getClientID(&i1);
			item2->getClientID(&i2);
			iResult = i1.compareTo(i2);
			break;
		}
		case colType:
			iResult = item1->getType() - item2->getType();
			break;
		case colContact:
			iResult = item1->madeContact() - item2->madeContact();
			break;
		case colDistance:
			{
				Kademlia::CUInt128 distance1, distance2;
				item1->getDistance(&distance1);
				item2->getDistance(&distance2);
				iResult = distance1.compareTo(distance2);
				break;
			}
		default:
			return 0;
	}
	if (HIWORD(lParamSort))
		iResult = -iResult;
	return iResult;
}