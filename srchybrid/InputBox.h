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

class InputBox : public CDialog
{
	DECLARE_DYNAMIC(InputBox)

public:
	InputBox(CWnd* pParent = NULL);   // standard constructor
	virtual ~InputBox();
	virtual BOOL OnInitDialog();
	afx_msg void OnOK();
	// khaos::categorymod+
	afx_msg void OnCancel();
	// khaos::categorymod-
// Dialog Data
	enum { IDD = IDD_INPUTBOX };
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void OnCleanFilename();
	DECLARE_MESSAGE_MAP()
public:
	// khaos::categorymod+
	void	SetNumber(bool isNum = false) { isNumber = isNum; }
	int	GetInputInt();
	// khaos::categorymod-
	void	SetLabels(CString title, CString label, CString defaultStr);
	CString	GetInput();
	bool	WasCancelled() { return m_cancel;}
	void	SetEditFilenameMode(bool isfilenamemode=true) {m_bFilenameMode=isfilenamemode;}
private:
	CString m_label;
	CString m_title;
	CString m_default;
	CString m_return;
	bool	m_cancel;
	bool	m_bFilenameMode;
	// khaos::categorymod+
	bool	isNumber;
	// khaos::categorymod-
};