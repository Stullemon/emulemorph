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
#include "FileDetailDialog.h"
#include "PartFile.h"
#include "AbstractFile.h"
#include "SearchFile.h"
#include "HighColorTab.hpp"
#include "UserMsgs.h"
#include "SharedFilesCtrl.h"
#include "DownloadListCtrl.h"
#include "SearchListCtrl.h"
#include "CollectionListCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialog

LPCTSTR CFileDetailDialog::m_pPshStartPage;

IMPLEMENT_DYNAMIC(CFileDetailDialog, CListViewWalkerPropertySheet)

BEGIN_MESSAGE_MAP(CFileDetailDialog, CListViewWalkerPropertySheet)
	ON_WM_DESTROY()
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
END_MESSAGE_MAP()

CFileDetailDialog::CFileDetailDialog(CTypedPtrList<CPtrList, CKnownFile*>& paFiles, UINT uPshInvokePage, CListCtrlItemWalk* pListCtrl)
	: CListViewWalkerPropertySheet(pListCtrl)
{
	m_uPshInvokePage = uPshInvokePage;
	POSITION pos = paFiles.GetHeadPosition();
	while (pos)
		m_aItems.Add(paFiles.GetNext(pos));
	m_psh.dwFlags &= ~PSH_HASHELP;
	AddPages();
}

CFileDetailDialog::CFileDetailDialog(CTypedPtrList<CPtrList, CAbstractFile*>& paFiles, UINT uPshInvokePage, CListCtrlItemWalk* pListCtrl)
	: CListViewWalkerPropertySheet(pListCtrl)
{
	m_uPshInvokePage = uPshInvokePage;
	POSITION pos = paFiles.GetHeadPosition();
	while (pos)
		m_aItems.Add(paFiles.GetNext(pos));
	m_psh.dwFlags &= ~PSH_HASHELP;
	AddPages();
}

CFileDetailDialog::CFileDetailDialog(CTypedPtrList<CPtrList, CSearchFile*>& paFiles, UINT uPshInvokePage, CListCtrlItemWalk* pListCtrl)
	: CListViewWalkerPropertySheet(pListCtrl)
{
	m_uPshInvokePage = uPshInvokePage;
	POSITION pos = paFiles.GetHeadPosition();
	while (pos)
		m_aItems.Add(paFiles.GetNext(pos));
	m_psh.dwFlags &= ~PSH_HASHELP;
	AddPages();
}

CFileDetailDialog::CFileDetailDialog(const CSimpleArray<CPartFile*>* paFiles, UINT uPshInvokePage, CListCtrlItemWalk* pListCtrl)
	: CListViewWalkerPropertySheet(pListCtrl)
{
	m_uPshInvokePage = uPshInvokePage;
	for (int i = 0; i < paFiles->GetSize(); i++)
		m_aItems.Add((*paFiles)[i]);
	m_psh.dwFlags &= ~PSH_HASHELP;
	AddPages();
}

CFileDetailDialog::CFileDetailDialog(const CSearchFile* file, UINT uPshInvokePage, CListCtrlItemWalk* pListCtrl)
	: CListViewWalkerPropertySheet(pListCtrl)
{
	m_uPshInvokePage = uPshInvokePage;
	m_aItems.Add(const_cast<CSearchFile*>(file));
	m_psh.dwFlags &= ~PSH_HASHELP;
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	AddPages();
}

CFileDetailDialog::~CFileDetailDialog()
{
}

void CFileDetailDialog::AddPages(void)
{
	if(m_pListCtrl->GetListCtrl()->IsKindOf(RUNTIME_CLASS(CSharedFilesCtrl)))
	{
		m_wndMediaInfo.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndMediaInfo.m_psp.dwFlags |= PSP_USEICONID;
		m_wndMediaInfo.m_psp.pszIcon = _T("MEDIAINFO");
		m_wndMediaInfo.SetFiles(&m_aItems);
		AddPage(&m_wndMediaInfo);

		m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndMetaData.m_psp.dwFlags |= PSP_USEICONID;
		m_wndMetaData.m_psp.pszIcon = _T("METADATA");
		if (m_aItems.GetSize() == 1 && thePrefs.IsExtControlsEnabled()) {
			m_wndMetaData.SetFiles(&m_aItems);
			AddPage(&m_wndMetaData);
		}

		m_wndFileLink.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndFileLink.m_psp.dwFlags |= PSP_USEICONID;
		m_wndFileLink.m_psp.pszIcon = _T("ED2KLINK");
		m_wndFileLink.SetFiles(&m_aItems);
		AddPage(&m_wndFileLink);

		m_wndFileComments.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndFileComments.m_psp.dwFlags |= PSP_USEICONID;
		m_wndFileComments.m_psp.pszIcon = _T("FileComments");
		m_wndFileComments.SetFiles(&m_aItems);
		AddPage(&m_wndFileComments);
	}

	else if(m_pListCtrl->GetListCtrl()->IsKindOf(RUNTIME_CLASS(CDownloadListCtrl)))
	{
		m_wndInfo.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndInfo.m_psp.dwFlags |= PSP_USEICONID;
		m_wndInfo.m_psp.pszIcon = _T("FILEINFO");
		m_wndInfo.SetFiles(&m_aItems);
		AddPage(&m_wndInfo);

		if (m_aItems.GetSize() == 1)
		{
			m_wndName.m_psp.dwFlags &= ~PSP_HASHELP;
			m_wndName.m_psp.dwFlags |= PSP_USEICONID;
			m_wndName.m_psp.pszIcon = _T("FILERENAME");
			m_wndName.SetFiles(&m_aItems);
			AddPage(&m_wndName);
		}

		m_wndComments.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndComments.m_psp.dwFlags |= PSP_USEICONID;
		m_wndComments.m_psp.pszIcon = _T("FILECOMMENTS");
		m_wndComments.SetFiles(&m_aItems);
		AddPage(&m_wndComments);

		m_wndMediaInfo.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndMediaInfo.m_psp.dwFlags |= PSP_USEICONID;
		m_wndMediaInfo.m_psp.pszIcon = _T("MEDIAINFO");
		m_wndMediaInfo.SetFiles(&m_aItems);
		AddPage(&m_wndMediaInfo);

		if (m_aItems.GetSize() == 1)
		{
			if (thePrefs.IsExtControlsEnabled()){
				m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;
				m_wndMetaData.m_psp.dwFlags |= PSP_USEICONID;
				m_wndMetaData.m_psp.pszIcon = _T("METADATA");
				m_wndMetaData.SetFiles(&m_aItems);
				AddPage(&m_wndMetaData);
			}
		}

		m_wndFileLink.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndFileLink.m_psp.dwFlags |= PSP_USEICONID;
		m_wndFileLink.m_psp.pszIcon = _T("ED2KLINK");
		m_wndFileLink.SetFiles(&m_aItems);
		AddPage(&m_wndFileLink);
	}

	else if(m_pListCtrl->GetListCtrl()->IsKindOf(RUNTIME_CLASS(CSearchListCtrl)))
	{
		m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndMetaData.m_psp.dwFlags |= PSP_USEICONID;
		m_wndMetaData.m_psp.pszIcon = _T("METADATA");
		if (m_aItems.GetSize() == 1 && thePrefs.IsExtControlsEnabled()) {
			m_wndMetaData.SetFiles(&m_aItems);
			AddPage(&m_wndMetaData);
		}

		m_wndComments.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndComments.m_psp.dwFlags |= PSP_USEICONID;
		m_wndComments.m_psp.pszIcon = _T("FileComments");
		m_wndComments.SetFiles(&m_aItems);
		AddPage(&m_wndComments);
	}

	else if(m_pListCtrl->GetListCtrl()->IsKindOf(RUNTIME_CLASS(CCollectionListCtrl)))
	{
		m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndMetaData.m_psp.dwFlags |= PSP_USEICONID;
		m_wndMetaData.m_psp.pszIcon = _T("METADATA");
		if (m_aItems.GetSize() == 1 && thePrefs.IsExtControlsEnabled()) {
			m_wndMetaData.SetFiles(&m_aItems);
			AddPage(&m_wndMetaData);
		}
	}

	LPCTSTR pPshStartPage = m_pPshStartPage;
	if (m_uPshInvokePage != 0)
		pPshStartPage = MAKEINTRESOURCE(m_uPshInvokePage);
	for (int i = 0; i < m_pages.GetSize(); i++)
	{
		CPropertyPage* pPage = GetPage(i);
		if (pPage->m_psp.pszTemplate == pPshStartPage)
		{
			m_psh.nStartPage = i;
			break;
		}
	}
}

void CFileDetailDialog::OnDestroy()
{
	if (m_uPshInvokePage == 0)
		m_pPshStartPage = GetPage(GetActiveIndex())->m_psp.pszTemplate;
	CListViewWalkerPropertySheet::OnDestroy();
}

BOOL CFileDetailDialog::OnInitDialog()
{		
	EnableStackedTabs(FALSE);
	BOOL bResult = CListViewWalkerPropertySheet::OnInitDialog();
	HighColorTab::UpdateImageList(*this);
	InitWindowStyles(this);
	EnableSaveRestore(_T("FileDetailDialog")); // call this after(!) OnInitDialog
	UpdateTitle();
	return bResult;
}

LRESULT CFileDetailDialog::OnDataChanged(WPARAM, LPARAM)
{
	UpdateTitle();
	return 1;
}

void CFileDetailDialog::UpdateTitle()
{
	if (m_aItems.GetSize() == 1)
		SetWindowText(GetResString(IDS_DETAILS) + _T(": ") + STATIC_DOWNCAST(CAbstractFile, m_aItems[0])->GetFileName());
	else
		SetWindowText(GetResString(IDS_DETAILS));
}

BOOL CFileDetailDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if(m_pListCtrl->GetListCtrl()->IsKindOf(RUNTIME_CLASS(CSharedFilesCtrl)))
	{
		if (wParam == ID_APPLY_NOW)
		{
			CSharedFilesCtrl* pSharedFilesCtrl = DYNAMIC_DOWNCAST(CSharedFilesCtrl, m_pListCtrl->GetListCtrl());
			if (pSharedFilesCtrl)
			{
				for (int i = 0; i < m_aItems.GetSize(); i++) {
					// so, and why does this not(!) work while the sheet is open ??
					pSharedFilesCtrl->UpdateFile(DYNAMIC_DOWNCAST(CKnownFile, m_aItems[i]));
				}
			}
		}
	}
	return CListViewWalkerPropertySheet::OnCommand(wParam, lParam);
}