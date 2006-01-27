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
#include "CommentListCtrl.h"
#include "OtherFunctions.h"
#include "MenuCmds.h"
#include "TitleMenu.h"
#include "emule.h"
#include "UpDownClient.h"
#include "kademlia/kademlia/Entry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif

enum ECols
{
	colUserName = 0,
	colFileName,
	colComment,
	colRating,
	colClientSoft,  //MORPH - Added by SiRoB, ClientSoftware Column
	colClientCountry //MORPH - Added by SiRoB, ClientCountry Column
};

IMPLEMENT_DYNAMIC(CCommentListCtrl, CMuleListCtrl)

BEGIN_MESSAGE_MAP(CCommentListCtrl, CMuleListCtrl)
   	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

CCommentListCtrl::CCommentListCtrl()
	: CListCtrlItemWalk(this)
{
}

CCommentListCtrl::~CCommentListCtrl()
{
}

void CCommentListCtrl::Localize(void)
{
}

void CCommentListCtrl::Init(void)
{
	SetName(_T("CommentListCtrl"));
	ModifyStyle(LVS_SINGLESEL,0);
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	InsertColumn(colUserName, GetResString(IDS_QL_USERNAME), LVCFMT_LEFT, 130, -1); 
	InsertColumn(colFileName, GetResString(IDS_DL_FILENAME), LVCFMT_LEFT, 130, -1); 
	InsertColumn(colComment, GetResString(IDS_QL_RATING), LVCFMT_LEFT, 80, 1); 
	InsertColumn(colRating, GetResString(IDS_COMMENT), LVCFMT_LEFT, 340, 1);
	InsertColumn(colClientSoft, GetResString(IDS_CD_CSOFT), LVCFMT_LEFT, 130, 1); //Commander - Added: ClientSoftware Column
	InsertColumn(colClientCountry, GetResString(IDS_COUNTRY), LVCFMT_LEFT, 130, 1); //Commander - Added: ClientCountry Column
			
	LoadSettings();
}

void CCommentListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	UINT flag = MF_STRING;
	if (GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED) == -1)
		flag = MF_GRAYED;

	CTitleMenu popupMenu;
	popupMenu.CreatePopupMenu();
	popupMenu.AppendMenu(MF_STRING | flag, MP_MESSAGE, GetResString(IDS_CMT_COPYCLIPBOARD));

	GetPopupMenuPos(*this, point);
	popupMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( popupMenu.DestroyMenu() );
}
BOOL CCommentListCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1)
	{
		switch (wParam)
		{
		case MP_MESSAGE:
			theApp.CopyTextToClipboard(GetItemText(iSel, 3));
			return TRUE;
		}
	}
	return CMuleListCtrl::OnCommand(wParam, lParam);
}

void CCommentListCtrl::AddItem(Kademlia::CEntry* entry)
{
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)entry;
	int iItem = FindItem(&find);
	if (iItem == -1)
	{
		//Do not try to access the entry object after inserting this item.
		//It is possible that the object is deleted.
		int index = InsertItem(LVIF_TEXT|LVIF_PARAM,0,(LPCTSTR)_T(""),0,0,1,(LPARAM)entry);
		SetItemText(index, 1, entry->m_fileName); 
		SetItemText(index, 2, GetRateString((UINT)entry->GetIntTagValue(TAG_FILERATING))); 
		SetItemText(index, 3, entry->GetStrTagValue(TAG_DESCRIPTION));
	}
}

void CCommentListCtrl::AddItem(CUpDownClient* client)
{
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)client;
	int iItem = FindItem(&find);
	if (iItem == -1)
	{
		//Do not try to access the client object after inserting this item.
		//It is possible that the object is deleted.
		int index = InsertItem(LVIF_TEXT|LVIF_PARAM,0,client->GetUserName(),0,0,1,(LPARAM)client);
		SetItemText(index, 1, client->GetClientFilename()); 
		SetItemText(index, 2, GetRateString(client->GetFileRating())); 
		SetItemText(index, 3, client->GetFileComment());
		SetItemText(index, 4, client->GetClientSoftVer()); //Commander - Added: ClientSoftware Column
		SetItemText(index, 5, client->GetCountryName()); //Commander - Added: ClientCountry Column
	}
}

//JOHNTODO: I want to add sorting here.. But need to first find a way to make
// sure a CUpDownClient object wasn't deleted while viewing the comment list..

// And TODO: save column setup (.SaveSettings() ) before listctrlobject is destroyed or when setup changes
