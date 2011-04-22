//this file is part of eMule
//Copyright (C)2002-2008 Merkur ( strEmail.Format("%s@%s", "devteam", "emule-project.net") / http://www.emule-project.net )
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
#include "resource.h"
#include "eMule.h"
#include "InputBox.h"
#include "OtherFunctions.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(InputBox, CDialog)

BEGIN_MESSAGE_MAP(InputBox, CDialog)
	ON_BN_CLICKED(IDC_CLEANFILENAME, OnCleanFilename)
END_MESSAGE_MAP()

InputBox::InputBox(CWnd* pParent /*=NULL*/)
	: CDialog(InputBox::IDD, pParent)
{
	m_cancel = true;
	m_bFilenameMode = false;
	m_icMain = NULL;
	// khaos::categorymod+
	isNumber=false;
	// khaos::categorymod-
}

InputBox::~InputBox()
{
	if (m_icMain)
		VERIFY( DestroyIcon(m_icMain) );
}

void InputBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

void InputBox::OnOK()
{
	m_cancel = false;
	//MORPH START - Added by SiRoB, categorymod
	if (isNumber)
		GetDlgItemText(IDC_TEXTNUM,m_return);
	else
	//MORPH END   - Added by SiRoB, categorymod
		GetDlgItemText(IDC_TEXT, m_return);
	m_return.Trim();
	CDialog::OnOK();
}


// khaos::categorymod+
void InputBox::OnCancel()
{
	if (isNumber) m_return = "-1";
	else m_return = "0";
	
	CDialog::OnCancel();
}
// khaos::categorymod-

void InputBox::SetLabels(CString title, CString label, CString defaultStr)
{
	m_label = label;
	m_title = title;
	m_default = defaultStr;
}

BOOL InputBox::OnInitDialog()
{
	CDialog::OnInitDialog();
	InitWindowStyles(this);
	SetIcon( m_icMain = theApp.LoadIcon(_T("RENAME")),FALSE);

	GetDlgItem(IDC_IBLABEL)->SetWindowText(m_label);
	// khaos::categorymod+
	if (!isNumber)
		GetDlgItem(IDC_TEXT)->SetWindowText(m_default);
	else {
		GetDlgItem(IDC_TEXTNUM)->SetWindowText(m_default);
		GetDlgItem(IDC_TEXT)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_TEXTNUM)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_TEXTNUM)->SetFocus();
	}
	// khaos::categorymod-
	SetWindowText(m_title);

	SetDlgItemText(IDOK, GetResString(IDS_TREEOPTIONS_OK) );
	GetDlgItem(IDCANCEL)->SetWindowText(GetResString(IDS_CANCEL));
	SetDlgItemText(IDC_CLEANFILENAME,GetResString(IDS_CLEANUP));
	GetDlgItem(IDC_CLEANFILENAME)->ShowWindow(m_bFilenameMode ? SW_NORMAL : SW_HIDE);

	return TRUE;
}

void InputBox::OnCleanFilename()
{
	CString filename;
	GetDlgItem(IDC_TEXT)->GetWindowText(filename);
	GetDlgItem(IDC_TEXT)->SetWindowText(CleanupFilename(filename));
}
