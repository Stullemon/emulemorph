//this file is part of eMule
// added by quekky
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


/////////////////////////////////////////////////////////////////////////////
// CRichEditCtrlX window

class CRichEditCtrlX : public CRichEditCtrl
{
public:
	CRichEditCtrlX() {}

	CRichEditCtrlX& operator<<(LPCTSTR psz)
	{
		ReplaceSel(psz);
		return *this;
	}

	CRichEditCtrlX& operator<<(char* psz)
	{
		ReplaceSel(psz);
		return *this;
	}

	CRichEditCtrlX& operator<<(UINT uVal)
	{
		CString strVal;
		strVal.Format(_T("%u"), uVal);
		ReplaceSel(strVal);
		return *this;
	}

	CRichEditCtrlX& operator<<(int iVal)
	{
		CString strVal;
		strVal.Format(_T("%d"), iVal);
		ReplaceSel(strVal);
		return *this;
	}

	CRichEditCtrlX& operator<<(double fVal)
	{
		CString strVal;
		strVal.Format(_T("%.3f"), fVal);
		ReplaceSel(strVal);
		return *this;
	}

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg UINT OnGetDlgCode();
	afx_msg BOOL OnEnLink(NMHDR *pNMHDR, LRESULT *pResult);
};


/////////////////////////////////////////////////////////////////////////////
// CFileInfoDialog dialog

class CFileInfoDialog : public CResizablePage
{
	DECLARE_DYNAMIC(CFileInfoDialog)

public:
	CFileInfoDialog();   // standard constructor
	virtual ~CFileInfoDialog();

	void SetMyfile(CKnownFile* file) {m_file=file;}

// Dialog Data
	enum { IDD = IDD_FILEINFO };

protected:
	CString m_strCaption;
	CKnownFile* m_file;
	long m_lAudioBitrate;
	BOOL m_bAudioRoundBitrate;
	CRichEditCtrlX m_fi;
	CHARFORMAT m_cfDef;
	CHARFORMAT m_cfBold;
	CHARFORMAT m_cfRed;

	void RefreshData();
	void Localize();
	void AddFileInfo(LPCTSTR pszFmt, ...);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedRoundbit();
};
