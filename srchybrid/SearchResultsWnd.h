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
#include "ResizableLib\ResizableFormView.h"
#include "SearchListCtrl.h"
#include "ClosableTabCtrl.h"
#include "IconStatic.h"
#include "EditX.h"
#include "ComboBoxEx2.h"
#include "ListCtrlEditable.h"

class CCustomAutoComplete;
class Packet;
class CSafeMemFile;
class CSearchParamsWnd;
struct SSearchParams;


///////////////////////////////////////////////////////////////////////////////
// CSearchResultsWnd dialog

class CSearchResultsWnd : public CResizableFormView, public CLoggable
{
	DECLARE_DYNCREATE(CSearchResultsWnd)

public:
	CSearchResultsWnd(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSearchResultsWnd();
	
	enum { IDD = IDD_SEARCH };

	CSearchListCtrl searchlistctrl;
	CClosableTabCtrl searchselect;
	CSearchParamsWnd* m_pwndParams;

	void	Localize();

	void	StartSearch(SSearchParams* pParams);
	bool	SearchMore();
	void	CancelSearch();

	bool	DoNewEd2kSearch(SSearchParams* pParams);
	bool	DoNewKadSearch(SSearchParams* pParams);

	void	DownloadSelected();
	void	DownloadSelected(bool paused);

	bool	CanDeleteSearch(uint32 nSearchID) const;
	bool	CanDeleteAllSearches() const;
	void	DeleteSearch(uint32 nSearchID);
	void	DeleteAllSearchs();

	void	LocalSearchEnd(uint16 count, bool bMoreResultsAvailable);
	void	AddUDPResult(uint16 count);

	bool	CreateNewTab(SSearchParams* pParams);
	void	ShowSearchSelector(bool visible);
	//MORPH - Changed by SiRoB, Selection Category Support
	/*
	uint8	GetSelectedCat()							{return m_cattabs.GetCurSel();}
	*/
	int	GetSelectedCat()							{return m_cattabs.GetCurSel();}
	void	UpdateCatTabs();
	void	SaveSettings();

	virtual void OnInitialUpdate();

protected:
	Packet*		searchpacket;
	UINT_PTR	global_search_timer;
	UINT		m_uTimerLocalServer;
	CProgressCtrl searchprogress;
	bool		canceld;
	uint16		servercount;
	bool		globsearch;
	uint32		m_nSearchID;
	CImageList	m_imlSearchResults;
	CTabCtrl	m_cattabs;
	HICON		icon_search;
	int			m_iSentMoreReq;

	CString ToQueryString(CString str);
	bool StartNewSearch(SSearchParams* pParams);
	CString	CreateWebQuery(SSearchParams* pParams);
	void ShowResults(const SSearchParams* pParams);
	void SetAllIcons();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	afx_msg void OnNMDblclkSearchlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDestroy();
	afx_msg void OnSysColorChange();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnBnClickedSdownload();
	afx_msg void OnBnClickedClearall();
	afx_msg LRESULT OnCloseTab(WPARAM wparam, LPARAM lparam);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedOpenParamsWnd();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnNMClickCattab2(NMHDR *pNMHDR, LRESULT *pResult); //MORPH - Added by SiRoB, Selection category support
};
