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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
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
#include "Opcodes.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


// CommentDialog dialog

IMPLEMENT_DYNAMIC(CCommentDialog, CDialog)

BEGIN_MESSAGE_MAP(CCommentDialog, CDialog)
	ON_BN_CLICKED(IDCOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDC_RESET, OnBnClickedReset)
END_MESSAGE_MAP()

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
	DDX_Control(pDX, IDC_RATELIST, m_ratebox);
}

void CCommentDialog::OnBnClickedOk()
{
	CString SValue;
	GetDlgItem(IDC_CMT_TEXT)->GetWindowText(SValue);
	m_file->SetFileComment(SValue);
	m_file->SetFileRate((uint8)m_ratebox.GetCurSel());
	CDialog::OnOK();
}

void CCommentDialog::OnBnClickedCancel()
{
	CDialog::OnCancel();
}

void CCommentDialog::OnBnClickedReset()
{
	SetDlgItemText(IDC_CMT_TEXT, _T(""));
	m_ratebox.SetCurSel(0);
}

BOOL CCommentDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	InitWindowStyles(this);
	Localize();

	GetDlgItem(IDC_CMT_TEXT)->SetWindowText(m_file->GetFileComment());
	((CEdit*)GetDlgItem(IDC_CMT_TEXT))->SetLimitText(MAXFILECOMMENTLEN);
	return TRUE;
}

void CCommentDialog::Localize(void)
{
	GetDlgItem(IDCCANCEL)->SetWindowText(GetResString(IDS_CANCEL));
	GetDlgItem(IDC_RESET)->SetWindowText(GetResString(IDS_PW_RESET));

	GetDlgItem(IDC_CMT_LQUEST)->SetWindowText(GetResString(IDS_CMT_QUEST));
	GetDlgItem(IDC_CMT_LAIDE)->SetWindowText(GetResString(IDS_CMT_AIDE));

	GetDlgItem(IDC_RATEQUEST)->SetWindowText(GetResString(IDS_CMT_RATEQUEST));
	GetDlgItem(IDC_RATEHELP)->SetWindowText(GetResString(IDS_CMT_RATEHELP));

	while (m_ratebox.GetCount()>0)
		m_ratebox.DeleteString(0);
	m_ratebox.AddString(GetResString(IDS_CMT_NOTRATED));
	m_ratebox.AddString(GetResString(IDS_CMT_FAKE));
	m_ratebox.AddString(GetResString(IDS_CMT_POOR));
	m_ratebox.AddString(GetResString(IDS_CMT_GOOD));
	m_ratebox.AddString(GetResString(IDS_CMT_FAIR));
	m_ratebox.AddString(GetResString(IDS_CMT_EXCELLENT));
	if (m_ratebox.SetCurSel(m_file->GetFileRate()) == CB_ERR)
		m_ratebox.SetCurSel(0);

	CString strTitle;
	strTitle.Format(GetResString(IDS_CMT_TITLE),m_file->GetFileName());
	SetWindowText(strTitle);
}
