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
#include "DirectDownloadDlg.h"
#include "OtherFunctions.h"
#include "emuleDlg.h"
#include "DownloadQueue.h"
#include "ED2KLink.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


#define	PREF_INI_SECTION	_T("DirectDownloadDlg")

IMPLEMENT_DYNAMIC(CDirectDownloadDlg, CDialog)

BEGIN_MESSAGE_MAP(CDirectDownloadDlg, CResizableDialog)
	ON_EN_KILLFOCUS(IDC_ELINK, OnEnKillfocusElink)
	ON_EN_UPDATE(IDC_ELINK, OnEnUpdateElink)
END_MESSAGE_MAP()

CDirectDownloadDlg::CDirectDownloadDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CDirectDownloadDlg::IDD, pParent)
{
}

CDirectDownloadDlg::~CDirectDownloadDlg()
{
}

void CDirectDownloadDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DDOWN_FRM, m_ctrlDirectDlFrm);
}

void CDirectDownloadDlg::UpdateControls()
{
	GetDlgItem(IDOK)->EnableWindow(GetDlgItem(IDC_ELINK)->GetWindowTextLength() > 0);
}

void CDirectDownloadDlg::OnEnUpdateElink()
{
	UpdateControls();
}

void CDirectDownloadDlg::OnEnKillfocusElink()
{
	CString strLinks;
	GetDlgItem(IDC_ELINK)->GetWindowText(strLinks);
	if (strLinks.IsEmpty() || strLinks.Find(_T('\n')) == -1)
		return;
	strLinks.Replace(_T("\n"), _T("\r\n"));
	strLinks.Replace(_T("\r\r"), _T("\r"));
	GetDlgItem(IDC_ELINK)->SetWindowText(strLinks);
}

void CDirectDownloadDlg::OnOK()
{
	CString strLinks;
	GetDlgItem(IDC_ELINK)->GetWindowText(strLinks);

	int curPos = 0;
	CString strTok = strLinks.Tokenize(_T("\t\n\r"), curPos);
	while (!strTok.IsEmpty())
	{
		if (strTok.Right(1) != _T("/"))
			strTok += _T("/");
		try
		{
			CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(strTok.Trim());
			if (pLink)
			{
				if (pLink->GetKind() == CED2KLink::kFile)
				{
					
					//MORPH START - Changed by SiRoB, Selection category support khaos::categorymod+
					/*
					theApp.downloadqueue->AddFileLinkToDownload(pLink->GetFileLink(), 0);
					/*/
					CED2KFileLink* pFileLink = (CED2KFileLink*)CED2KLink::CreateLinkFromUrl(strTok.Trim());
					theApp.downloadqueue->AddFileLinkToDownload(pFileLink, -1, true);
					/**/
					//MORPH END   - Changed by SiRoB, Selection category support khaos::categorymod-
				}
				else
				{
					delete pLink;
					throw CString(_T("bad link"));
				}
				delete pLink;
			}
		}
		catch(CString error)
		{
			TCHAR szBuffer[200];
			_snprintf(szBuffer, ARRSIZE(szBuffer), GetResString(IDS_ERR_INVALIDLINK), error);
			CString strError;
			strError.Format(GetResString(IDS_ERR_LINKERROR), szBuffer);
			AfxMessageBox(strError);
			return;
		}
		strTok = strLinks.Tokenize(_T("\t\n\r"), curPos);
	}

	CResizableDialog::OnOK();
}

BOOL CDirectDownloadDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);

	AddAnchor(IDC_DDOWN_FRM, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_ELINK, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	EnableSaveRestore(PREF_INI_SECTION);

	SetWindowText(GetResString(IDS_SW_DIRECTDOWNLOAD));
	m_ctrlDirectDlFrm.Init("Download");
	m_ctrlDirectDlFrm.SetWindowText(GetResString(IDS_SW_DIRECTDOWNLOAD));
	m_ctrlDirectDlFrm.SetText(GetResString(IDS_SW_DIRECTDOWNLOAD));
    GetDlgItem(IDC_FSTATIC2)->SetWindowText(GetResString(IDS_SW_LINK));

	UpdateControls();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}
