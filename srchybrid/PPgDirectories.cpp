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
#include "emuledlg.h"
#include "SharedFilesWnd.h"
#include "PPgDirectories.h"
#include "otherfunctions.h"
#include "InputBox.h"
#include "SharedFileList.h"
#include "Preferences.h"
#include "HelpIDs.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNAMIC(CPPgDirectories, CPropertyPage)

BEGIN_MESSAGE_MAP(CPPgDirectories, CPropertyPage)
	ON_BN_CLICKED(IDC_SELTEMPDIR, OnBnClickedSeltempdir)
	ON_BN_CLICKED(IDC_SELINCDIR,OnBnClickedSelincdir)
	ON_EN_CHANGE(IDC_INCFILES,	OnSettingsChange)
	ON_EN_CHANGE(IDC_TEMPFILES, OnSettingsChange)
	ON_BN_CLICKED(IDC_UNCADD,	OnBnClickedAddUNC)
	ON_BN_CLICKED(IDC_UNCREM,	OnBnClickedRemUNC)
	ON_WM_HELPINFO()
END_MESSAGE_MAP()

CPPgDirectories::CPPgDirectories()
	: CPropertyPage(CPPgDirectories::IDD)
{
	
}

CPPgDirectories::~CPPgDirectories()
{
}

void CPPgDirectories::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SHARESELECTOR, m_ShareSelector);
	DDX_Control(pDX, IDC_UNCLIST, m_ctlUncPaths);
}

BOOL CPPgDirectories::OnInitDialog()
{
	CWaitCursor curWait; // initialization of that dialog may take a while..
	CPropertyPage::OnInitDialog();
	InitWindowStyles(this);
	m_ShareSelector.Init();	

	((CEdit*)GetDlgItem(IDC_INCFILES))->SetLimitText(509);
	((CEdit*)GetDlgItem(IDC_TEMPFILES))->SetLimitText(509);
	m_ctlUncPaths.InsertColumn(0, GetResString(IDS_UNCFOLDERS), LVCFMT_LEFT, 280, -1); 
	m_ctlUncPaths.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	LoadSettings();
	Localize();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPPgDirectories::LoadSettings(void)
{
	GetDlgItem(IDC_INCFILES)->SetWindowText(thePrefs.incomingdir);
	GetDlgItem(IDC_TEMPFILES)->SetWindowText(thePrefs.tempdir);
	m_ShareSelector.SetSharedDirectories(&thePrefs.shareddir_list);
	FillUncList();
}

void CPPgDirectories::OnBnClickedSelincdir()
{
	TCHAR buffer[MAX_PATH] = {0};
	GetDlgItemText(IDC_INCFILES, buffer, ARRSIZE(buffer));
	if(SelectDir(GetSafeHwnd(),buffer,GetResString(IDS_SELECT_INCOMINGDIR)))
		GetDlgItem(IDC_INCFILES)->SetWindowText(buffer);
}

void CPPgDirectories::OnBnClickedSeltempdir()
{
	TCHAR buffer[MAX_PATH] = {0};
	GetDlgItemText(IDC_TEMPFILES, buffer, ARRSIZE(buffer));
	if(SelectDir(GetSafeHwnd(),buffer,GetResString(IDS_SELECT_TEMPDIR)))
		GetDlgItem(IDC_TEMPFILES)->SetWindowText(buffer);
}

BOOL CPPgDirectories::OnApply()
{
	CString testdirchanged=thePrefs.GetTempDir();

	CString strIncomingDir;
	GetDlgItemText(IDC_INCFILES, strIncomingDir);
	// SLUGFILLER: SafeHash remove - removed installation dir unsharing
	
	CString strTempDir;
	GetDlgItemText(IDC_TEMPFILES, strTempDir);
	// SLUGFILLER: SafeHash remove - removed installation dir unsharing

	// SLUGFILLER: SafeHash remove - removed installation dir unsharing

	_sntprintf(thePrefs.incomingdir, ARRSIZE(thePrefs.incomingdir), _T("%s"), strIncomingDir);
	MakeFoldername(thePrefs.incomingdir);
	sprintf(thePrefs.GetCategory(0)->incomingpath,"%s",thePrefs.incomingdir);

	_sntprintf(thePrefs.tempdir, ARRSIZE(thePrefs.tempdir), _T("%s"), strTempDir);
	MakeFoldername(thePrefs.tempdir);

	thePrefs.shareddir_list.RemoveAll();

	m_ShareSelector.GetSharedDirectories(&thePrefs.shareddir_list);
	for (int i = 0; i < m_ctlUncPaths.GetItemCount(); i++)
		thePrefs.shareddir_list.AddTail(m_ctlUncPaths.GetItemText(i, 0));

	// SLUGFILLER: SafeHash remove - removed installation dir unsharing

	theApp.emuledlg->sharedfileswnd->Reload();

	if (testdirchanged.CompareNoCase(thePrefs.GetTempDir())!=0)
		AfxMessageBox(GetResString(IDS_SETTINGCHANGED_RESTART));
	
	SetModified(0);
	return CPropertyPage::OnApply();
}

BOOL CPPgDirectories::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam == USRMSG_ITEMSTATECHANGED)
		SetModified();	
	else if (wParam == ID_HELP)
	{
		OnHelp();
		return TRUE;
	}
	return CPropertyPage::OnCommand(wParam, lParam);
}

void CPPgDirectories::Localize(void)
{
	if(m_hWnd)
	{
		SetWindowText(GetResString(IDS_PW_DIR));

		GetDlgItem(IDC_INCOMING_FRM)->SetWindowText(GetResString(IDS_PW_INCOMING));
		GetDlgItem(IDC_TEMP_FRM)->SetWindowText(GetResString(IDS_PW_TEMP));
		GetDlgItem(IDC_SELINCDIR)->SetWindowText(GetResString(IDS_PW_BROWSE));
		GetDlgItem(IDC_SELTEMPDIR)->SetWindowText(GetResString(IDS_PW_BROWSE));
		GetDlgItem(IDC_SHARED_FRM)->SetWindowText(GetResString(IDS_PW_SHARED));
	}
}

void CPPgDirectories::FillUncList(void)
{
	m_ctlUncPaths.DeleteAllItems();

	for (POSITION pos = thePrefs.shareddir_list.GetHeadPosition(); pos != 0; )
	{
		CString folder = thePrefs.shareddir_list.GetNext(pos);
		if (PathIsUNC(folder))
			m_ctlUncPaths.InsertItem(0, folder);
	}
}

void CPPgDirectories::OnBnClickedAddUNC()
{
	InputBox inputbox;
	inputbox.SetLabels(GetResString(IDS_UNCFOLDERS), GetResString(IDS_UNCFOLDERS), _T("\\\\Server\\Share"));
	if (inputbox.DoModal() != IDOK)
		return;
	CString unc=inputbox.GetInput();

	// basic unc-check 
	if (!PathIsUNC(unc)){
		AfxMessageBox(GetResString(IDS_ERR_BADUNC), MB_ICONERROR);
		return;
	}

	if (unc.Right(1) == _T("\\"))
		unc.Delete(unc.GetLength()-1, 1);

	for (POSITION pos = thePrefs.shareddir_list.GetHeadPosition();pos != 0;){
		if (unc.CompareNoCase(thePrefs.shareddir_list.GetNext(pos))==0)
			return;
	}
	for (int posi = 0; posi < m_ctlUncPaths.GetItemCount(); posi++){
		if (unc.CompareNoCase(m_ctlUncPaths.GetItemText(posi, 0)) == 0)
			return;
	}

	m_ctlUncPaths.InsertItem(m_ctlUncPaths.GetItemCount(), unc);
	SetModified();
}

void CPPgDirectories::OnBnClickedRemUNC()
{
	int index = m_ctlUncPaths.GetSelectionMark();
	if (index == -1 || m_ctlUncPaths.GetSelectedCount() == 0)
		return;
	m_ctlUncPaths.DeleteItem(index);
	SetModified();
}

void CPPgDirectories::OnHelp()
{
	theApp.ShowHelp(eMule_FAQ_Preferences_Directories);
}

BOOL CPPgDirectories::OnHelpInfo(HELPINFO* pHelpInfo)
{
	OnHelp();
	return TRUE;
}
