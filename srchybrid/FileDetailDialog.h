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
#include "ResizableLib/ResizablePage.h"
#include "ResizableLib/ResizableSheet.h"
#include "FileInfoDialog.h"
#include "CommentDialogLst.h"
#include "MetaDataDlg.h"
#include "MuleListCtrl.h"
#include "ED2kLinkDlg.h"
#include "ListViewWalkerPropertySheet.h"

struct FCtrlItem_Struct{
   CString	filename;
   uint16	count;
};

///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialogInfo dialog

class CFileDetailDialogInfo : public CResizablePage
{
	DECLARE_DYNAMIC(CFileDetailDialogInfo)

public:
	CFileDetailDialogInfo();   // standard constructor
	virtual ~CFileDetailDialogInfo();

	void SetFiles(const CSimpleArray<CObject*>* paFiles) { m_paFiles = paFiles; m_bDataChanged = true; }

	// Dialog Data
	enum { IDD = IDD_FILEDETAILS_INFO };

protected:
	CString m_strCaption;
	const CSimpleArray<CObject*>* m_paFiles;
	bool m_bDataChanged;
	uint32 m_timer;
	static LPCTSTR sm_pszNotAvail;

	void Localize();
	void RefreshData();

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnSetActive();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
};


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialogName dialog

class CFileDetailDialogName : public CResizablePage
{
	DECLARE_DYNAMIC(CFileDetailDialogName)

public:
	CFileDetailDialogName();   // standard constructor
	virtual ~CFileDetailDialogName();

	void SetFiles(const CSimpleArray<CObject*>* paFiles) { m_paFiles = paFiles; m_bDataChanged = true; }

	// Dialog Data
	enum { IDD = IDD_FILEDETAILS_NAME };

protected:
	CString m_strCaption;
	const CSimpleArray<CObject*>* m_paFiles;
	bool m_bDataChanged;
	bool m_bAppliedSystemImageList;
	CMuleListCtrl m_listFileNames;
	bool m_bSelf;

	uint32 m_timer;
	int m_aiColWidths[2];
	uint8	m_sortindex;
	bool	m_sortorder;

	void Localize();
	void RefreshData();
	void FillSourcenameList();
	void Copy();
	bool CanRenameFile() const;
	void RenameFile();

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnSetActive();
	virtual BOOL OnApply();
	
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	static int CALLBACK CompareListNameItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedButtonStrip();
	afx_msg void TakeOver();
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
	afx_msg void OnEnChangeFilename();
};


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialog

class CFileDetailDialog : public CListViewWalkerPropertySheet
{
	DECLARE_DYNAMIC(CFileDetailDialog)

public:
	CFileDetailDialog(const CSimpleArray<CPartFile*>* paFiles, UINT uInvokePage = 0, CListCtrlItemWalk* pListCtrl = NULL);
	virtual ~CFileDetailDialog();

protected:
	CFileDetailDialogInfo	m_wndInfo;
	CFileDetailDialogName	m_wndName;
	CFileInfoDialog			m_wndMediaInfo;
	CCommentDialogLst		m_wndComments;
	CMetaDataDlg			m_wndMetaData;
	CED2kLinkDlg			m_wndFileLink;

	UINT m_uPshInvokePage;
	static LPCTSTR m_pPshStartPage;

	void UpdateTitle();

	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
};
