//this file is part of eMule
//Copyright (C)2002-2005 Merkur ( devs@emule-project.net / http://www.emule-project.net )
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
#include "SharedDirsTreeCtrl.h"
#include "SplitterControl.h"
#include "EditDelayed.h"
#include "ListViewWalkerPropertySheet.h"
#include "HistoryListCtrl.h" //MORPH - Added, Downloaded History [Monki/Xman]

/////////////////////////////////////////////////////////////////////////////////////////////
// CSharedFileDetailsModelessSheet
class CFileDetailDlgStatistics;
class CED2kLinkDlg;
class CArchivePreviewDlg;
class CFileInfoDialog;
class CMetaDataDlg;
class CSharedFileDetailsModelessSheet : public CListViewPropertySheet
{
	DECLARE_DYNAMIC(CSharedFileDetailsModelessSheet)

public:
	CSharedFileDetailsModelessSheet();
	virtual ~CSharedFileDetailsModelessSheet();
	void SetFiles(CTypedPtrList<CPtrList, CShareableFile*>& aFiles);

protected:
	CFileDetailDlgStatistics*	m_wndStatistics;
	CED2kLinkDlg*				m_wndFileLink;
	CArchivePreviewDlg*			m_wndArchiveInfo;
	CFileInfoDialog*			m_wndMediaInfo;
	CMetaDataDlg*				m_wndMetaData;

	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnDataChanged(WPARAM, LPARAM);
};

class CSharedFilesWnd : public CResizableDialog
{
	DECLARE_DYNAMIC(CSharedFilesWnd)

public:
	CSharedFilesWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSharedFilesWnd();

	void Localize();
	void SetToolTipsDelay(DWORD dwDelay);
	void Reload(bool bForceTreeReload = false);
	uint32	GetFilterColumn() const				{ return m_nFilterColumn; }
	void OnVolumesChanged()						{ m_ctlSharedDirTree.OnVolumesChanged(); }
	void OnSingleFileShareStatusChanged()		{ m_ctlSharedDirTree.FileSystemTreeUpdateBoldState(NULL); }
	//MORPH START - Changed, Downloaded History [Monki/Xman]
#ifdef NO_HISTORY
	void ShowSelectedFilesDetails(bool bForce = false);
#else
	void ShowSelectedFilesDetails(bool bForce = false, bool bHistory = false);
#endif
	//MORPH END   - Changed, Downloaded History [Monki/Xman]
	void ShowDetailsPanel(bool bShow);

// Dialog Data
	enum { IDD = IDD_FILES };

	CSharedFilesCtrl sharedfilesctrl;
	CStringArray m_astrFilter;
	CSharedDirsTreeCtrl m_ctlSharedDirTree;

private:

	CFont bold;
	HICON icon_files;
	CSplitterControl m_wndSplitter;
	CEditDelayed	m_ctlFilter;
	CHeaderCtrl		m_ctlSharedListHeader;
	uint32			m_nFilterColumn;
	bool			m_bDetailsVisible;
	CSharedFileDetailsModelessSheet	m_dlgDetails;

protected:
	void SetAllIcons();
	void DoResize(int delta);

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg LRESULT OnChangeFilter(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedReloadSharedFiles();
	afx_msg void OnLvnItemActivateSharedFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNmClickSharedFiles(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnStnDblClickFilesIco();
	afx_msg void OnSysColorChange();
	afx_msg void OnTvnSelChangedSharedDirsTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnBnClickedSfHideshowdetails();
	afx_msg void OnLvnItemchangedSflist(NMHDR *pNMHDR, LRESULT *pResult);
	//MORPH START - Added, Downloaded History [Monki/Xman]
#ifndef NO_HISTORY
public:
	CHistoryListCtrl historylistctrl;
protected:
	afx_msg void OnLvnItemActivateHistorylist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMClickHistorylist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedHlist(NMHDR *pNMHDR, LRESULT *pResult);
#endif
	//MORPH END   - Added, Downloaded History [Monki/Xman]
};
