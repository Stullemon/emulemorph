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
#include "CommentDialog.h"
#include "KnownFile.h"
#include "OtherFunctions.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CommentDialog dialog 

IMPLEMENT_DYNAMIC(CCommentDialog, CDialog) 
CCommentDialog::CCommentDialog(CKnownFile* file) 
   : CDialog(CCommentDialog::IDD, 0) 
{ 
   m_file = file; 
} 

CCommentDialog::~CCommentDialog() 
{ 
} 

void CCommentDialog::DoDataExchange(CDataExchange* pDX) 
{ 
   CDialog::DoDataExchange(pDX); 
   DDX_Control(pDX, IDC_RATELIST, ratebox);//for rate 
} 

BEGIN_MESSAGE_MAP(CCommentDialog, CDialog) 
   ON_BN_CLICKED(IDCOK, OnBnClickedApply) 
   ON_BN_CLICKED(IDCCANCEL, OnBnClickedCancel) 
END_MESSAGE_MAP() 



void CCommentDialog::OnBnClickedApply() 
{ 
   CString SValue; 
   GetDlgItem(IDC_CMT_TEXT)->GetWindowText(SValue); 
   m_file->SetFileComment(SValue); 
   m_file->SetFileRate((uint8)ratebox.GetCurSel());//for Rate//
   CDialog::OnOK(); 
} 

void CCommentDialog::OnBnClickedCancel() 
{ 
   CDialog::OnCancel(); 
} 

BOOL CCommentDialog::OnInitDialog(){ 
	CDialog::OnInitDialog(); 
	InitWindowStyles(this);
	Localize(); 

	GetDlgItem(IDC_CMT_TEXT)->SetWindowText(m_file->GetFileComment()); 
	((CEdit*)GetDlgItem(IDC_CMT_TEXT))->SetLimitText(50);

	return TRUE; 
} 

void CCommentDialog::Localize(void){ 
   GetDlgItem(IDCOK)->SetWindowText(GetResString(IDS_PW_APPLY)); 
   GetDlgItem(IDCCANCEL)->SetWindowText(GetResString(IDS_CANCEL)); 

   GetDlgItem(IDC_CMT_LQUEST)->SetWindowText(GetResString(IDS_CMT_QUEST)); 
   GetDlgItem(IDC_CMT_LAIDE)->SetWindowText(GetResString(IDS_CMT_AIDE)); 

   //for rate// 
   GetDlgItem(IDC_RATEQUEST)->SetWindowText(GetResString(IDS_CMT_RATEQUEST)); 
   GetDlgItem(IDC_RATEHELP)->SetWindowText(GetResString(IDS_CMT_RATEHELP)); 

   while (ratebox.GetCount()>0) ratebox.DeleteString(0); 
   
   ratebox.AddString(GetResString(IDS_CMT_NOTRATED)); 
   ratebox.AddString(GetResString(IDS_CMT_FAKE)); 
   ratebox.AddString(GetResString(IDS_CMT_POOR)); 
   ratebox.AddString(GetResString(IDS_CMT_GOOD)); 
   ratebox.AddString(GetResString(IDS_CMT_FAIR)); 
   ratebox.AddString(GetResString(IDS_CMT_EXCELLENT)); 
   if (ratebox.SetCurSel(m_file->GetFileRate())==CB_ERR)ratebox.SetCurSel(0) ; 
   //--end rate--//

   CString strTitle; 
   strTitle.Format(GetResString(IDS_CMT_TITLE),m_file->GetFileName());

   SetWindowText(strTitle); 
    
} 
