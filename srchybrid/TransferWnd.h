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
#include "SplitterControl.h"
#include "BtnST.h"
#include "TabCtrl.hpp"
#include "UploadListCtrl.h"
#include "DownloadListCtrl.h"
#include "QueueListCtrl.h"
#include "ClientListCtrl.h"
#include "progressctrlx.h" //Commander - Added: ClientQueueProgressBar
#include "DownloadClientsCtrl.h" //SLAHAM: ADDED DownloadClientsCtrl

class CUploadListCtrl;
class CDownloadListCtrl;
class CQueueListCtrl;
class CClientListCtrl;

class CTransferWnd : public CResizableDialog
{
	DECLARE_DYNAMIC(CTransferWnd)

public:
	CTransferWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CTransferWnd();

	void	ShowQueueCount(uint32 number);
	void	UpdateListCount(uint8 listindex, int iCount = -1);
	void	Localize();
	void UpdateCatTabTitles(bool force=true);
	void	UpdateDownloadClientsCount(int count); //SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
	bool 	isUploadQueueVisible() { return (m_uWnd2 == 0); } //SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
	bool	isDownloadListVisible() { return ((showlist == IDC_DOWNLOADLIST) || (showlist == IDC_DOWNLOADLIST+IDC_UPLOADLIST)); } //SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
	void	VerifyCatTabSize(bool _forceverify=false);
	void SwitchUploadList();

	// khaos::categorymod+
	int		GetActiveCategory()			{ return m_dlTab.GetCurSel(); }
	// khaos::categorymod-

	// Dialog Data
	enum { IDD = IDD_TRANSFER };
	CUploadListCtrl		uploadlistctrl;
	CDownloadListCtrl	downloadlistctrl;
	CQueueListCtrl		queuelistctrl;
	CClientListCtrl		clientlistctrl;
	CDownloadClientsCtrl	downloadclientsctrl; //SLAHAM: ADDED DownloadClientsCtrl
	CToolTipCtrl		m_tooltipCats;

	bool bQl;
	bool bKl;

	//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
	void OnBnClickedChangeView();
	void OnBnClickedDownUploads();
	void OnBnClickedDownloads();
	void OnBnClickedUploads();
	void OnBnClickedQueue();
	void OnBnClickedClient();
	void OnBnClickedTransfers();
	uint16 showlist;	
	//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons <=

protected:
	CSplitterControl m_wndSplitter;
	uint8 m_uWnd2;
	bool downloadlistactive;
	CButtonST m_uplBtn;
	TabControl m_dlTab;
	int	rightclickindex;
	int m_nDragIndex;
	int m_nDropIndex;
	int m_nLastCatTT;
	bool m_bIsDragging;
	CImageList* m_pDragImage;
	HICON icon_download;
	POINT m_pLastMousePoint;
	CProgressCtrlX queueBar; //Commander - Added: ClientQueueProgressBar
	CFont bold;//Commander - Added: ClientQueueProgressBar
	//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons =>
	CButtonST	m_btnChangeView;
	CButtonST	m_btnDownUploads;
	CButtonST	m_btnDownloads;
	CButtonST	m_btnUploads;
	CButtonST	m_btnQueue;
	CButtonST	m_btnTransfers;
	CButtonST	m_btnClient;
	void ShowList(uint16 list);
	//SLAHAM: ADDED [TPT] - TBH Transfer Window Buttons <=
	//SLAHAM: ADDED Switch Lists Icons =>
	CButtonST	m_btnULChangeView;
	CButtonST	m_btnULUploads;
	CButtonST	m_btnULQueue;
	CButtonST	m_btnULTransfers;
	CButtonST	m_btnULClients;	
	//SLAHAM: ADDED Switch Lists Icons <=

	void ShowWnd2(uint8 uList);
	void SetWnd2(uint8 uWnd2);
	void DoResize(int delta);
	void UpdateSplitterRange();
	void SetInitLayout();
	void DoSplitResize(int delta);
	void SetAllIcons();
	void SetWnd2Icon();
	void UpdateTabToolTips() {UpdateTabToolTips(-1);}
	void UpdateTabToolTips(int tab);
	CString GetTabStatistic(uint8 tab);
	int GetTabUnderMouse(CPoint* point);
	int GetItemUnderMouse(CListCtrl* ctrl);
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
