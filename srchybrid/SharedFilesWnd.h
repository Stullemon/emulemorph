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
#include "ResizableLib\ResizableDialog.h"
#include "SharedFilesCtrl.h"
#include "ProgressCtrlX.h"
#include "IconStatic.h"

class CSharedFilesWnd : public CResizableDialog
{
	DECLARE_DYNAMIC(CSharedFilesWnd)

public:
	CSharedFilesWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSharedFilesWnd();

	void Localize();
	void ShowSelectedFilesSummary();
	void Reload();

// Dialog Data
	enum { IDD = IDD_FILES };

	CSharedFilesCtrl sharedfilesctrl;

private:
	CProgressCtrlX pop_bar;
	CProgressCtrlX pop_baraccept;
	CProgressCtrlX pop_bartrans;
	CFont bold;
	CIconStatic m_ctrlStatisticsFrm;
	HICON icon_files;

protected:
	void SetAllIcons();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedReloadsharedfiles();
	afx_msg void OnLvnItemActivateSflist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickSflist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSysColorChange();
	afx_msg void OnStnDblclickFilesIco();
};
