//this file is part of eMule
//Copyright (C)2003 Merkur ( merkur-@users.sourceforge.net / http://www.emule-project.net )
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

// PreviewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "PreviewDlg.h"
#include "CxImage/xImage.h"


// PreviewDlg dialog

IMPLEMENT_DYNAMIC(PreviewDlg, CDialog)
PreviewDlg::PreviewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(PreviewDlg::IDD, pParent)
{
}

PreviewDlg::~PreviewDlg()
{
}

void PreviewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PV_IMAGE, m_ImageStatic);
}


BEGIN_MESSAGE_MAP(PreviewDlg, CDialog)
	ON_BN_CLICKED(IDC_PV_EXIT, OnBnClickedPvExit)
	ON_BN_CLICKED(IDC_PV_NEXT, OnBnClickedPvNext)
	ON_BN_CLICKED(IDC_PV_PRIOR, OnBnClickedPvPrior)
END_MESSAGE_MAP()


BOOL PreviewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	InitWindowStyles(this);
	if (m_pFile == NULL){
		ASSERT ( false );
		return FALSE;
	}
	CString title =GetResString(IDS_DL_PREVIEW);
	title.Remove('&');
	SetWindowText( title + CString(_T(": ")) + m_pFile->GetFileName());
	m_nCurrentImage = 0;
	ShowImage(0);
	CImageList m_ImageList;
	m_ImageList.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,10);
	m_ImageList.SetBkColor(CLR_NONE);
	m_ImageList.Add(theApp.LoadIcon(IDI_CANCEL));
	m_ImageList.Add(theApp.LoadIcon(IDI_FORWARD));
	m_ImageList.Add(theApp.LoadIcon(IDI_BACK));
	((CButton*)GetDlgItem(IDC_PV_EXIT))->SetIcon(m_ImageList.ExtractIcon(0));
	((CButton*)GetDlgItem(IDC_PV_NEXT))->SetIcon(m_ImageList.ExtractIcon(1));
	((CButton*)GetDlgItem(IDC_PV_PRIOR))->SetIcon(m_ImageList.ExtractIcon(2));
	return TRUE;
}

void PreviewDlg::ShowImage(sint16 nNumber){
	uint16 nImageCount = m_pFile->GetPreviews().GetSize();
	if (nImageCount == 0)
		return;
	else if (nImageCount <= nNumber)
		nNumber = 0;
	else if (nNumber < 0)
		nNumber = nImageCount-1;

	m_nCurrentImage = nNumber;
	HBITMAP m_bitmap = m_pFile->GetPreviews()[nNumber]->MakeBitmap(m_ImageStatic.GetDC()->m_hDC);
	m_ImageStatic.SetBitmap(m_bitmap);
}

void PreviewDlg::Show(){
	Create(IDD_PREVIEWDIALOG, NULL);
}

// PreviewDlg message handlers

void PreviewDlg::OnBnClickedPvExit()
{
	OnClose();
}

void PreviewDlg::OnBnClickedPvNext()
{
	ShowImage(m_nCurrentImage+1);
}

void PreviewDlg::OnBnClickedPvPrior()
{
	ShowImage(m_nCurrentImage-1);
}

void PreviewDlg::OnClose(){
	CDialog::OnClose();
	delete this;
}
