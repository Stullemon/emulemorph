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
#include "serverlistctrl.h"
#include "afxwin.h"
#include "ResizableLib\ResizableDialog.h"
#include "HyperTextCtrl.h"
#include "iconstatic.h"
#include "afxcmn.h"
#include "LogEditCtrl.h"
#include "CustomAutoComplete.h"

class CHTRichEditCtrl;

// CServerWnd dialog

class CServerWnd : public CResizableDialog
{
	DECLARE_DYNAMIC(CServerWnd)

public:
	CServerWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CServerWnd();
	void Localize();
	bool UpdateServerMetFromURL(CString strURL);
	void ToggleDebugWindow();
	void UpdateMyInfo();
	void UpdateLogTabSelection();
	BOOL SaveServerMetStrings();
	void ShowServerInfo();

// Dialog Data
	enum { IDD = IDD_SERVER };
	CServerListCtrl serverlistctrl;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()
public:
	CHTRichEditCtrl* servermsgbox;
	CLogEditCtrl logbox;
	CLogEditCtrl debuglog;
	afx_msg void OnBnClickedAddserver();
	afx_msg void OnBnClickedUpdateservermetfromurl();
	afx_msg void OnBnClickedResetLog();
	afx_msg void OnBnConnect();
	afx_msg void OnTcnSelchangeTab3(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnEnLinkServerBox(NMHDR *pNMHDR, LRESULT *pResult);
	CTabCtrl StatusSelector;
	void UpdateControlsState();
private:
	CIconStatic m_ctrlNewServerFrm;
	CIconStatic m_ctrlUpdateServerFrm;
	CIconStatic m_ctrlMyInfo;
	CImageList imagelist;
	HICON icon_srvlist;
	bool	debug;
	CMuleListCtrl* MyInfoList;
	CCustomAutoComplete* m_pacServerMetURL;
	CString m_strClickNewVersion;
	LCID m_uLangID;
};