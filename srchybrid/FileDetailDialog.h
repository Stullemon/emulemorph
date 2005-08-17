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
#pragma once
#include "ListViewWalkerPropertySheet.h"
#include "FileInfoDialog.h"
#include "CommentDialogLst.h"
#include "CommentDialog.h"
#include "MetaDataDlg.h"
#include "ED2kLinkDlg.h"
#include "FileDetailDialogInfo.h"
#include "FileDetailDialogName.h"

class CSearchFile;

class CFileDetailDialog : public CListViewWalkerPropertySheet
{
	DECLARE_DYNAMIC(CFileDetailDialog)

public:
	CFileDetailDialog(CTypedPtrList<CPtrList, CKnownFile*>& paFiles, UINT uPshInvokePage = 0, CListCtrlItemWalk* pListCtrl = NULL);
	CFileDetailDialog(const CSimpleArray<CPartFile*>* paFiles, UINT uInvokePage = 0, CListCtrlItemWalk* pListCtrl = NULL);
	CFileDetailDialog(const CSearchFile* file, UINT uPshInvokePage = 0, CListCtrlItemWalk* pListCtrl = NULL);
	CFileDetailDialog(CTypedPtrList<CPtrList, CAbstractFile*>& paFiles, UINT uPshInvokePage = 0, CListCtrlItemWalk* pListCtrl = NULL);
	CFileDetailDialog(CTypedPtrList<CPtrList, CSearchFile*>& paFiles, UINT uPshInvokePage = 0, CListCtrlItemWalk* pListCtrl = NULL);
	virtual ~CFileDetailDialog();

protected:
	CFileDetailDialogInfo	m_wndInfo;
	CFileDetailDialogName	m_wndName;
	CFileInfoDialog			m_wndMediaInfo;
	CCommentDialogLst		m_wndComments;
	CMetaDataDlg			m_wndMetaData;
	CED2kLinkDlg			m_wndFileLink;
	CCommentDialog			m_wndFileComments;

	UINT m_uPshInvokePage;
	static LPCTSTR m_pPshStartPage;

	void UpdateTitle();

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
	void AddPages(void);
};
