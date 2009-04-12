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
#include "ResizableLib/ResizablePage.h"

class CKnownFile;

///////////////////////////////////////////////////////////////////////////////
// CED2kLinkDlg

class CED2kLinkDlg : public CResizablePage
{ 
	DECLARE_DYNAMIC(CED2kLinkDlg) 

public: 
	CED2kLinkDlg(); 
	virtual ~CED2kLinkDlg(); 

	void SetFiles(const CSimpleArray<CObject*>* paFiles) { m_paFiles = paFiles; m_bDataChanged = true; }

// Dialog Data 
	enum { IDD = IDD_ED2KLINK }; 

protected: 
	CEdit m_ctrlLinkEdit;
	CString m_strCaption;
	const CSimpleArray<CObject*>* m_paFiles;
	bool m_bDataChanged;

	void Localize(); 
	void UpdateLink();

	virtual BOOL OnInitDialog(); 
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support 
	virtual BOOL OnSetActive();
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP() 
	afx_msg void OnBnClickedClipboard();
	afx_msg void OnSettingsChange();
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);

	//emulEspa�a. Added by MoNKi [MoNKi: -HTTP Sources in eLinks-]
	afx_msg void OnBtnAddHttp();
	afx_msg void OnBtnRemoveHttp();
	afx_msg void OnBtnClearHttp();
	//End emulEspa�a
};
