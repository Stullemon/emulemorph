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
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "emule.h"
#include "AddSourceDlg.h"
#include "PartFile.h"
#include "OtherFunctions.h"
#include "UpDownClient.h"
#include "DownloadQueue.h"
#include <wininet.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// CAddSourceDlg dialog

IMPLEMENT_DYNAMIC(CAddSourceDlg, CDialog)

BEGIN_MESSAGE_MAP(CAddSourceDlg, CResizableDialog)
	ON_BN_CLICKED(IDC_RSRC, OnBnClickedRadio1)
	ON_BN_CLICKED(IDC_RURL, OnBnClickedRadio4)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedButton1)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

CAddSourceDlg::CAddSourceDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CAddSourceDlg::IDD, pParent)
	, m_nSourceType(0)
{
	m_pFile = NULL;
}

CAddSourceDlg::~CAddSourceDlg()
{
}

void CAddSourceDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RSRC, m_nSourceType);
}

void CAddSourceDlg::SetFile( CPartFile *pFile )
{
	if( pFile )
		m_pFile = pFile;
}

BOOL CAddSourceDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);

	AddAnchor(IDC_SOURCE_TYPE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_EDIT10, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDC_BUTTON1, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);

	// localize
	SetDlgItemText(IDC_BUTTON1,GetResString(IDS_ADD));
	SetDlgItemText(IDCANCEL,GetResString(IDS_CANCEL));
	SetDlgItemText(IDC_RSRC,GetResString(IDS_SOURCECLIENT));
	SetDlgItemText(IDC_SOURCE_TYPE,GetResString(IDS_META_SRCTYPE));
	SetDlgItemText(IDC_RURL,GetResString(IDS_SV_URL));
	SetDlgItemText(IDC_UIP,GetResString(IDS_USERSIP));
	SetDlgItemText(IDC_PORT,GetResString(IDS_PORT));	

	EnableSaveRestore(_T("AddSourceDlg"));

	OnBnClickedRadio1();
	if( m_pFile )
		SetWindowText(m_pFile->GetFileName());
	return TRUE;
}
void CAddSourceDlg::OnBnClickedRadio1()
{
	m_nSourceType = 0;
	GetDlgItem(IDC_EDIT2)->EnableWindow(true);
	GetDlgItem(IDC_EDIT3)->EnableWindow(true);
	GetDlgItem(IDC_EDIT10)->EnableWindow(false);
}

void CAddSourceDlg::OnBnClickedRadio4()
{
	m_nSourceType = 1;
	GetDlgItem(IDC_EDIT2)->EnableWindow(false);
	GetDlgItem(IDC_EDIT3)->EnableWindow(false);
	GetDlgItem(IDC_EDIT10)->EnableWindow(true);
}

void CAddSourceDlg::OnBnClickedButton1()
{
	if( !m_pFile )
		return;
	switch( m_nSourceType )
	{
		case 0:
		{
			BOOL bTranslated = FALSE;
			uint16 port = GetDlgItemInt(IDC_EDIT3, &bTranslated, FALSE);
			if( bTranslated == FALSE )
				return;
			uint32 ip;
			CString sip;
			GetDlgItem(IDC_EDIT2)->GetWindowText(sip);
			USES_CONVERSION;
			if ((ip = inet_addr(T2CA(sip))) == INADDR_NONE && _tcscmp(sip, _T("255.255.255.255")) != 0)
				ip = 0;
			if( ::IsGoodIPPort(ip, port) )
			{
				CUpDownClient* toadd = new CUpDownClient(m_pFile, port, ntohl(ip), 0, 0 );
				toadd->SetSourceFrom( SF_PASSIVE );
				theApp.downloadqueue->CheckAndAddSource(m_pFile, toadd);
			}
			break;
		}
		case 1:
		{
			CString strURL;
			GetDlgItem(IDC_EDIT10)->GetWindowText(strURL);
			if (!strURL.IsEmpty())
			{
				TCHAR szScheme[INTERNET_MAX_SCHEME_LENGTH];
				TCHAR szHostName[INTERNET_MAX_HOST_NAME_LENGTH];
				TCHAR szUrlPath[INTERNET_MAX_PATH_LENGTH];
				TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
				TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
				TCHAR szExtraInfo[INTERNET_MAX_URL_LENGTH];
				URL_COMPONENTS Url = {0};
				Url.dwStructSize = sizeof(Url);
				Url.lpszScheme = szScheme;
				Url.dwSchemeLength = ARRSIZE(szScheme);
				Url.lpszHostName = szHostName;
				Url.dwHostNameLength = ARRSIZE(szHostName);
				Url.lpszUserName = szUserName;
				Url.dwUserNameLength = ARRSIZE(szUserName);
				Url.lpszPassword = szPassword;
				Url.dwPasswordLength = ARRSIZE(szPassword);
				Url.lpszUrlPath = szUrlPath;
				Url.dwUrlPathLength = ARRSIZE(szUrlPath);
				Url.lpszExtraInfo = szExtraInfo;
				Url.dwExtraInfoLength = ARRSIZE(szExtraInfo);
				if (InternetCrackUrl(strURL, 0, 0, &Url) && Url.dwHostNameLength > 0)
				{
					SUnresolvedHostname* hostname = new SUnresolvedHostname;
					hostname->strURL = strURL;
					hostname->strHostname = szHostName;
					theApp.downloadqueue->AddToResolved(m_pFile, hostname);
					delete hostname;
				}
			}
			break;
		}
	}
}

void CAddSourceDlg::OnBnClickedOk()
{
	OnBnClickedButton1();
	OnOK();
}
