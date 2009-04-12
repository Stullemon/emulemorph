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

class InputBox : public CDialog
{
	DECLARE_DYNAMIC(InputBox)
public:
	InputBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~InputBox();

// Dialog Data
	enum { IDD = IDD_INPUTBOX };

	void	SetLabels(CString title, CString label, CString defaultStr);
	const CString& GetInput() const { return m_return; }
	bool	WasCancelled() const { return m_cancel;}
	void	SetEditFilenameMode(bool isfilenamemode = true) { m_bFilenameMode = isfilenamemode; }
	// khaos::categorymod+
	void	SetNumber(bool isNum = false) { isNumber = isNum; }
	const int	GetInputInt() const { return _tstoi(m_return); };
	// khaos::categorymod-
protected:
	CString m_label;
	CString m_title;
	CString m_default;
	CString m_return;
	bool	m_cancel;
	bool	m_bFilenameMode;
	HICON	m_icMain;
	// khaos::categorymod+
	bool	isNumber;
	// khaos::categorymod-
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	afx_msg void OnOK();
	afx_msg void OnCleanFilename();
	// khaos::categorymod+
	afx_msg void OnCancel();
	// khaos::categorymod-
	DECLARE_MESSAGE_MAP()
};