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
#include "CommentDialogLst.h"
#include "PartFile.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
#include "emuledlg.h"
#include "ChatWnd.h"
#include "MenuCmds.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CCommentDialogLst, CResizablePage) 

BEGIN_MESSAGE_MAP(CCommentDialogLst, CResizablePage) 
   ON_BN_CLICKED(IDCOK, OnBnClickedApply) 
   ON_BN_CLICKED(IDCREF, OnBnClickedRefresh) 
   ON_NOTIFY(NM_DBLCLK, IDC_LST, OnNMDblclkLst)
   ON_WM_CONTEXTMENU()
END_MESSAGE_MAP() 

CCommentDialogLst::CCommentDialogLst() 
   : CResizablePage(CCommentDialogLst::IDD, IDS_CMT_READALL) 
{ 
	m_strCaption = GetResString(IDS_CMT_READALL);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
	m_file = NULL; 
} 

CCommentDialogLst::~CCommentDialogLst() 
{ 
} 

void CCommentDialogLst::DoDataExchange(CDataExchange* pDX) 
{ 
	CResizablePage::DoDataExchange(pDX); 
	DDX_Control(pDX, IDC_LST, pmyListCtrl);
} 

void CCommentDialogLst::OnBnClickedApply() 
{ 
	CResizablePage::OnOK(); 
} 

void CCommentDialogLst::OnBnClickedRefresh() 
{ 
	CompleteList(); 
} 

BOOL CCommentDialogLst::OnInitDialog()
{ 
	CResizablePage::OnInitDialog(); 
	InitWindowStyles(this);

	AddAnchor(IDC_LST,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDCREF,BOTTOM_RIGHT);
	AddAnchor(IDC_CMSTATUS,BOTTOM_LEFT);

	pmyListCtrl.InsertColumn(0, GetResString(IDS_QL_USERNAME), LVCFMT_LEFT, 130, -1); 
	pmyListCtrl.InsertColumn(1, GetResString(IDS_DL_FILENAME), LVCFMT_LEFT, 130, -1); 
	pmyListCtrl.InsertColumn(2, GetResString(IDS_QL_RATING), LVCFMT_LEFT, 80, 1); 
	pmyListCtrl.InsertColumn(3, GetResString(IDS_COMMENT), LVCFMT_LEFT, 340, 1); 
	pmyListCtrl.InsertColumn(4, GetResString(IDS_CLIENTSOFTWARE), LVCFMT_LEFT, 130, 1); //Commander - Added: ClientSoftware Column
        pmyListCtrl.InsertColumn(5, GetResString(IDS_COUNTRY), LVCFMT_LEFT, 130, 1); //Commander - Added: ClientCountry Column

	pmyListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT); //NoamSon: CopyComments

	Localize(); 
	CompleteList(); 

	return TRUE; 
} 

void CCommentDialogLst::Localize(void)
{ 
	GetDlgItem(IDCREF)->SetWindowText(GetResString(IDS_CMT_REFRESH)); 
} 

void CCommentDialogLst::CompleteList ()
{ 
	pmyListCtrl.DeleteAllItems();

	int count=0; 
	for (POSITION pos = m_file->srclist.GetHeadPosition(); pos != NULL; )
	{ 
		CUpDownClient* cur_src = m_file->srclist.GetNext(pos);
		if (cur_src->GetFileRate()>0 || !cur_src->GetFileComment().IsEmpty())
		{
			pmyListCtrl.InsertItem(LVIF_TEXT|LVIF_PARAM,count,cur_src->GetUserName(),0,0,1,(LPARAM)cur_src);
			pmyListCtrl.SetItemText(count, 1, cur_src->GetClientFilename()); 
			pmyListCtrl.SetItemText(count, 2, GetRateString(cur_src->GetFileRate())); 
			pmyListCtrl.SetItemText(count, 3, cur_src->GetFileComment());
			pmyListCtrl.SetItemText(count, 4, cur_src->GetClientSoftVer()); //Commander - Added: ClientSoftware Column
                        pmyListCtrl.SetItemText(count, 5, cur_src->GetCountryName()); //Commander - Added: ClientCountry Column
			count++;
		} 
	} 

	CString info;
	if (count==0)
		info=_T("(") + GetResString(IDS_CMT_NONE) + _T(")");
	GetDlgItem(IDC_CMSTATUS)->SetWindowText(info);
	m_file->UpdateFileRatingCommentAvail();
}

void CCommentDialogLst::OnNMDblclkLst(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (pmyListCtrl.GetSelectedCount()==0)
		return;

	CUpDownClient* client = (CUpDownClient*)pmyListCtrl.GetItemData(pmyListCtrl.GetSelectionMark());
	if (client)
		theApp.emuledlg->chatwnd->StartSession(client);
	theApp.emuledlg->SetActiveDialog(theApp.emuledlg->chatwnd);
	*pResult = 0;
}

// NoamSon: CopyComments+
void CCommentDialogLst::OnContextMenu(CWnd* pWnd, CPoint point){
	UINT flag = MF_STRING;
	if (pmyListCtrl.GetSelectionMark() == (-1))
		flag = MF_GRAYED;
	
	CTitleMenu popupMenu;
	popupMenu.CreatePopupMenu();
	popupMenu.AppendMenu(MF_STRING | flag,MP_MESSAGE, GetResString(IDS_CMT_COPYCLIPBOARD));
	GetPopupMenuPos(pmyListCtrl, point);
	popupMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( popupMenu.DestroyMenu() );
}
// NoamSon: CopyComments-

// NoamSon: CopyComments+
BOOL CCommentDialogLst::OnCommand(WPARAM wParam,LPARAM lParam ){
	int iSel = pmyListCtrl.GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1){
		switch (wParam){
			case MP_MESSAGE:
			POSITION pos = pmyListCtrl.GetFirstSelectedItemPosition(); 
			int itemPosition = pmyListCtrl.GetNextSelectedItem(pos); 
			CString cmt = pmyListCtrl.GetItemText(itemPosition,3);
			theApp.CopyTextToClipboard( cmt );
		
			return true;
		}
	}
	return CResizablePage::OnCommand(wParam, lParam);
}
// NoamSon: CopyComments-
