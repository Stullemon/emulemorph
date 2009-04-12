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
#pragma once
#include "ResizableLib/ResizableDialog.h"
#include "IconStatic.h"
#include "ButtonsTabCtrl.h"


class CDirectDownloadDlg : public CResizableDialog
{
	DECLARE_DYNAMIC(CDirectDownloadDlg)

public:
	CDirectDownloadDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDirectDownloadDlg();

// Dialog Data
	enum { IDD = IDD_DIRECT_DOWNLOAD };

protected:
	HICON m_icnWnd;
	CIconStatic m_ctrlDirectDlFrm;
	CButtonsTabCtrl	m_cattabs;

	void UpdateControls();
	void UpdateCatTabs();

	virtual BOOL OnInitDialog();
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnEnKillfocusElink();
	afx_msg void OnEnUpdateElink();
	//MORPH START - Added by SiRoB, Selection category support
	afx_msg void OnNMClickCats(NMHDR *pNMHDR, LRESULT *pResult);
 	//MORPH END   - Added by SiRoB, Selection category support
};
