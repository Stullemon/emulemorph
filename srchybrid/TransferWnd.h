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
#include "..\ResizableLib\ResizableDialog.h"
#include "SplitterControl.h"
#include "BtnST.h"
#include "TabCtrl.hpp"
#include "UploadListCtrl.h"
#include "DownloadListCtrl.h"
#include "QueueListCtrl.h"
#include "ClientListCtrl.h"
#include "downloadclientsctrl.h"
#include "DropDownButton.h"
#include "progressctrlx.h" //Commander - Added: ClientQueueProgressBar

class CTransferWnd : public CResizableDialog
{
	DECLARE_DYNAMIC(CTransferWnd)

public:
	CTransferWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTransferWnd();

	enum EWnd1Icon {
		w1iSplitWindow = 0,
		w1iDownloadFiles,
		w1iUploading,
		w1iDownloading,
		w1iOnQueue,
		w1iClientsKnown
	};

	enum EWnd2 {
		wnd2Downloading = 0,
		wnd2Uploading = 1,
		wnd2OnQueue = 2,
		wnd2Clients = 3
	};

	void ShowQueueCount(uint32 number);
	void UpdateListCount(EWnd2 listindex, int iCount = -1);
	void UpdateFilesCount(int iCount);
	void Localize();
	void UpdateCatTabTitles(bool force=true);
	void VerifyCatTabSize(bool _forceverify=false);
	void SwitchUploadList();
	void ResetTransToolbar(bool bShowToolbar, bool bResetLists = true);

	// khaos::categorymod+
	int		GetActiveCategory()			{ return m_dlTab.GetCurSel(); }
	// khaos::categorymod-

	// Dialog Data
	enum { IDD = IDD_TRANSFER };
	CUploadListCtrl		uploadlistctrl;
	CDownloadListCtrl	downloadlistctrl;
	CQueueListCtrl		queuelistctrl;
	CClientListCtrl		clientlistctrl;
	CDownloadClientsCtrl	downloadclientsctrl;
	CToolTipCtrl		m_tooltipCats;

protected:
	CSplitterControl m_wndSplitter;
	EWnd2		m_uWnd2;
	bool downloadlistactive;
	CDropDownButton m_btnWnd1;
	CDropDownButton	m_btnWnd2;
	TabControl	m_dlTab;
	int			rightclickindex;
	int			m_nDragIndex;
	int			m_nDropIndex;
	int			m_nLastCatTT;
	int			m_isetcatmenu;
	bool		m_bIsDragging;
	CImageList* m_pDragImage;
	POINT		m_pLastMousePoint;
	uint32		m_dwShowListIDC;
	CProgressCtrlX queueBar; //Commander - Added: ClientQueueProgressBar
	CProgressCtrlX queueBar2; //Commander - Added: ClientQueueProgressBar
	CFont bold;//Commander - Added: ClientQueueProgressBar


	void	ShowWnd2(EWnd2 uList);
	void	SetWnd2(EWnd2 uWnd2);
	void	DoResize(int delta);
	void	UpdateSplitterRange();
	void	DoSplitResize(int delta);
	void	SetAllIcons();
	void	SetWnd1Icons();
	void	SetWnd2Icon();
	void	UpdateTabToolTips() {UpdateTabToolTips(-1);}
	void	UpdateTabToolTips(int tab);
	CString	GetTabStatistic(uint8 tab);
	int		GetTabUnderMouse(CPoint* point);
	int		GetItemUnderMouse(CListCtrl* ctrl);
	//MOPRH - Removed by SiRoB, Due to Khaos Cat
	/*
	CString GetCatTitle(int catid);
	*/
	//MOPRH - Moved by SiRoB, Due to Khaos Cat moved in public area
	/*
	int AddCategory(CString newtitle,CString newincoming,CString newcomment,CString newautocat,bool addTab=true);
	*/
	void EditCatTabLabel(int index,CString newlabel);
	void EditCatTabLabel(int index);
	void	ShowList(uint32 dwListIDC);
	void	ChangeDlIcon(EWnd1Icon iIcon);
	void	OnBnClickedDownUploads(bool bReDraw = false);

	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()
	afx_msg void OnHoverUploadList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnHoverDownloadList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTcnSelchangeDltab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnNMRclickDltab(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTabMovement(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnLvnKeydownDownloadlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnSysColorChange();
	afx_msg void OnDblclickDltab();
	afx_msg void OnBnClickedQueueRefreshButton();
	afx_msg void OnBnClickedChangeView();
	afx_msg void OnWnd1BtnDropDown(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnWnd2BtnDropDown(NMHDR *pNMHDR, LRESULT *pResult);

	// khaos::categorymod+
	void		CreateCategoryMenus();
	CTitleMenu	m_mnuCategory;
	CTitleMenu	m_mnuCatPriority;
	CTitleMenu	m_mnuCatViewFilter;
	CTitleMenu	m_mnuCatA4AF;
	// khaos::categorymod-
public:
	int AddCategory(CString newtitle,CString newincoming,CString newcomment,CString newautocat,bool addTab=true);	
};
