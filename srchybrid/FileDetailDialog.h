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
#pragma once
#include "ResizableLib/ResizablePage.h"
#include "ResizableLib/ResizableSheet.h"
#include "FileInfoDialog.h"
#include "CommentDialogLst.h"
#include "MetaDataDlg.h"
#include "MuleListCtrl.h"
//class CMuleListCtrl;

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

	void SetMyfile(const CSimpleArray<CPartFile*>* paFiles) { m_paFiles = paFiles; }

	// Dialog Data
	enum { IDD = IDD_FILEDETAILS_INFO };

protected:
	CString m_strCaption;
	const CSimpleArray<CPartFile*>* m_paFiles;
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
};


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialogName dialog

class CFileDetailDialogName : public CResizablePage
{
	DECLARE_DYNAMIC(CFileDetailDialogName)

public:
	CFileDetailDialogName();   // standard constructor
	virtual ~CFileDetailDialogName();

	void SetMyfile(CPartFile* file)		{m_file=file;}

	// Dialog Data
	enum { IDD = IDD_FILEDETAILS_NAME };

protected:
	CString m_strCaption;
	CPartFile* m_file;
	bool m_bAppliedSystemImageList;
	CMuleListCtrl pmyListCtrl;

	uint32 m_timer;
	int m_aiColWidths[2];
	uint8	m_sortindex;
	bool	m_sortorder;

	void Localize();
	void RefreshData();
	void FillSourcenameList();
	void Copy();

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	static int CALLBACK CompareListNameItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedButtonStrip();
	afx_msg void TakeOver();
	afx_msg void OnRename();
	afx_msg void OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMDblclkList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnDestroy();
};


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialog

class CFileDetailDialog : public CResizableSheet
{
	DECLARE_DYNAMIC(CFileDetailDialog)

public:
	CFileDetailDialog(const CSimpleArray<CPartFile*>* paFiles, bool bInvokeCommentsPage = false);
	virtual ~CFileDetailDialog();

protected:
	bool m_bInvokeCommentsPage;
	CPartFile*	m_file;
	CSimpleArray<const CKnownFile*> m_aKnownFiles;
	CFileDetailDialogInfo m_wndInfo;
	CFileDetailDialogName m_wndName;
	CFileInfoDialog m_wndVideo;
	CCommentDialogLst m_wndComments;
	CMetaDataDlg m_wndMetaData;

	static LPCTSTR m_pPshStartPage;

	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
};
