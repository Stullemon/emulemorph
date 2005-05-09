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
#include "StringConversion.h"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// CommentDialog dialog

IMPLEMENT_DYNAMIC(CCommentDialog, CResizablePage)

BEGIN_MESSAGE_MAP(CCommentDialog, CResizablePage)
	ON_BN_CLICKED(IDC_RESET, OnBnClickedReset)
	ON_WM_DESTROY()
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
	ON_EN_CHANGE(IDC_CMT_TEXT, OnEnChangeCmtText)
	ON_CBN_SELENDOK(IDC_RATELIST, OnCbnSelendokRatelist)
	ON_CBN_SELCHANGE(IDC_RATELIST, OnCbnSelchangeRatelist)
END_MESSAGE_MAP()

CCommentDialog::CCommentDialog()
	: CResizablePage(CCommentDialog::IDD, 0)
{
	m_paFiles = NULL;
	m_bDataChanged = false;
	m_strCaption = GetResString(IDS_COMMENT);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
	m_bMergedComment = false;
	m_bSelf = false;
}

CCommentDialog::~CCommentDialog()
{
}

void CCommentDialog::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RATELIST, m_ratebox);
}

BOOL CCommentDialog::OnInitDialog()
{
	CResizablePage::OnInitDialog();
	InitWindowStyles(this);

	AddAnchor(IDC_CMT_LQUEST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CMT_LAIDE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_CMT_TEXT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_RATEQUEST, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_RATEHELP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_RESET, TOP_RIGHT);

	Localize();

	return TRUE;
}

BOOL CCommentDialog::OnSetActive()
{
	if (!CResizablePage::OnSetActive())
		return FALSE;
	if (m_bDataChanged)
	{
		int iRating = -1;
		m_bMergedComment = false;
		CString strComment;
		for (int i = 0; i < m_paFiles->GetSize(); i++)
		{
			CKnownFile* file = STATIC_DOWNCAST(CKnownFile, (*m_paFiles)[i]);
			if (i == 0)
			{
				strComment = file->GetFileComment();
				iRating = file->GetFileRating();
			}
			else
			{
				if (!m_bMergedComment && strComment.Compare(file->GetFileComment()) != 0)
				{
					strComment.Empty();
					m_bMergedComment = true;
				}
				if (iRating != -1 && iRating != file->GetFileRating())
					iRating = -1;
			}
		}

		m_bSelf = true;
		SetDlgItemText(IDC_CMT_TEXT, strComment);
		((CEdit*)GetDlgItem(IDC_CMT_TEXT))->SetLimitText(MAXFILECOMMENTLEN);
		//MORPH START - Changed by SiRoB, Proper rating order
		/*
		m_ratebox.SetCurSel(iRating);
		*/
		m_ratebox.SetCurSel((iRating==4)?3:(iRating==3)?4:iRating);
		//MORPH END   - Changed by SiRoB, Proper rating order
		m_bSelf = false;

		m_bDataChanged = false;
	}

	return TRUE;
}

LRESULT CCommentDialog::OnDataChanged(WPARAM, LPARAM)
{
	m_bDataChanged = true;
	return 1;
}

void CCommentDialog::OnBnClickedReset()
{
	SetDlgItemText(IDC_CMT_TEXT, _T(""));
	m_bMergedComment = false;
	m_ratebox.SetCurSel(0);
}

BOOL CCommentDialog::OnApply()
{
	if (!m_bDataChanged)
	{
	    CString strComment;
	    GetDlgItem(IDC_CMT_TEXT)->GetWindowText(strComment);
	    int iRating = m_ratebox.GetCurSel();
	    for (int i = 0; i < m_paFiles->GetSize(); i++)
	    {
		    CKnownFile* file = STATIC_DOWNCAST(CKnownFile, (*m_paFiles)[i]);
		    if (!strComment.IsEmpty() || !m_bMergedComment)
			    file->SetFileComment(strComment);
		    if (iRating != -1)
			    //MORPH START - Changed by SiRoB, Proper rating order
				/*
				file->SetFileRating(iRating);
				*/
				file->SetFileRating((iRating==3)?4:(iRating==4)?3:iRating);
				//MORPH END   - Changed by SiRoB, Proper rating order
	    }
	}
	return CResizablePage::OnApply();
}

void CCommentDialog::Localize(void)
{
	GetDlgItem(IDC_RESET)->SetWindowText(GetResString(IDS_PW_RESET));

	GetDlgItem(IDC_CMT_LQUEST)->SetWindowText(GetResString(IDS_CMT_QUEST));
	GetDlgItem(IDC_CMT_LAIDE)->SetWindowText(GetResString(IDS_CMT_AIDE));

	GetDlgItem(IDC_RATEQUEST)->SetWindowText(GetResString(IDS_CMT_RATEQUEST));
	GetDlgItem(IDC_RATEHELP)->SetWindowText(GetResString(IDS_CMT_RATEHELP));

	CImageList iml;
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.SetBkColor(CLR_NONE);
	iml.Add(CTempIconLoader(_T("Rating_NotRated"), 16, 16));
	iml.Add(CTempIconLoader(_T("Rating_Fake"), 16, 16));
	iml.Add(CTempIconLoader(_T("Rating_Poor"), 16, 16));
	iml.Add(CTempIconLoader(_T("Rating_Fair"), 16, 16));
	iml.Add(CTempIconLoader(_T("Rating_Good"), 16, 16));
	iml.Add(CTempIconLoader(_T("Rating_Excellent"), 16, 16));
	m_ratebox.SetImageList(&iml);
	m_imlRating.DeleteImageList();
	m_imlRating.Attach(iml.Detach());
	
	m_ratebox.ResetContent();
	m_ratebox.AddItem(GetResString(IDS_CMT_NOTRATED), 0);
	m_ratebox.AddItem(GetResString(IDS_CMT_FAKE), 1);
	m_ratebox.AddItem(GetResString(IDS_CMT_POOR), 2);
	m_ratebox.AddItem(GetResString(IDS_CMT_FAIR), 3);
	//MORPH START - Moved by FrankyFive, Proper rating order
	m_ratebox.AddItem(GetResString(IDS_CMT_GOOD), 4);
	//MORPH END   - Moved by FrankyFive, Proper rating order
	m_ratebox.AddItem(GetResString(IDS_CMT_EXCELLENT), 5);
	UpdateHorzExtent(m_ratebox, 16); // adjust dropped width to ensure all strings are fully visible
}

void CCommentDialog::OnDestroy()
{
	m_imlRating.DeleteImageList();
	CResizablePage::OnDestroy();
}

void CCommentDialog::OnEnChangeCmtText()
{
	if (!m_bSelf)
		SetModified();
}

void CCommentDialog::OnCbnSelendokRatelist()
{
	if (!m_bSelf)
		SetModified();
}

void CCommentDialog::OnCbnSelchangeRatelist()
{
	if (!m_bSelf)
		SetModified();
}
