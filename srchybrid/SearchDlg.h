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
#include "ResizableLib\ResizableDialog.h"
#include "SearchListCtrl.h"
#include "ClosableTabCtrl.h"
#include "IconStatic.h"
#include "EditX.h"
#include "ComboBoxEx2.h"
#include "kademlia/utils/UInt128.h"

class CCustomAutoComplete;
class CJigleSOAPThread;
class Packet;
class CSafeMemFile;


///////////////////////////////////////////////////////////////////////////////
// ESearchType

enum ESearchType
{
	//NOTE: The numbers are *equal* to the entries in the comboxbox -> TODO: use item data
	SearchTypeServer = 0,
	SearchTypeGlobal,
	SearchTypeKademlia,
	SearchTypeJigleSOAP,
	SearchTypeJigle,
	SearchTypeFileDonkey
};


///////////////////////////////////////////////////////////////////////////////
// SSearchParams

struct SSearchParams
{
	SSearchParams()
	{
		dwSearchID = (DWORD)-1;
		eType = SearchTypeServer;
		ulMinSize = 0;
		ulMaxSize = 0;
		iAvailability = -1;
	}
	DWORD dwSearchID;
	CString strExpression;
	ESearchType eType;
	CString strFileType;
	CString strMinSize;
	ULONG ulMinSize;
	CString strMaxSize;
	ULONG ulMaxSize;
	int iAvailability;
	CString strExtension;
	bool bMatchKeywords;
};


///////////////////////////////////////////////////////////////////////////////
// CSearchDlg dialog
class CSearchDlg : public CResizableDialog
{
	DECLARE_DYNAMIC(CSearchDlg)

public:
	CSearchDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSearchDlg();
	
	afx_msg void OnBnClickedStarts();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnBnClickedCancels();
	afx_msg void OnBnClickedSdownload();
	afx_msg void OnBnClickedClearall();
	afx_msg void OnBnClickedSearchReset();
	afx_msg void OnEnKillfocusElink();
	afx_msg void OnSearchKeyDown();

	enum { IDD = IDD_SEARCH };

	void	Localize();
	void	DownloadSelected();
	void	DownloadSelected(bool paused);
	void	LocalSearchEnd(uint16 count, bool bMoreResultsAvailable);
	void	AddUDPResult(uint16 count);
	void	DeleteSearch(uint32 nSearchID);
	void	DeleteAllSearchs();
	bool	CreateNewTab(SSearchParams* pParams);
	void	UpdateCatTabs();
	void	SearchClipBoard();
	// khaos::categorymod+ Changed Param: uint8 cat
	void	AddEd2kLinksToDownload(CString strlink, int theCat = -1);
	// Removed overloaded function
	// khaos::categorymod-
	CSearchListCtrl searchlistctrl;
	CClosableTabCtrl searchselect;
	void	IgnoreClipBoardLinks(CString strlink)		{m_lastclpbrd = strlink; }
	bool	DoNewSearch(SSearchParams* pParams);
	bool	DoNewKadSearch(SSearchParams* pParams);
	void	ShowSearchSelector(bool visible);
	BOOL	SaveSearchStrings();
	//MORPH - Removed by SiRoB, Khaos Category
	/*uint8	GetSelectedCat()	{ return m_cattabs.GetCurSel();}*/
	LRESULT ProcessJigleSearchResponse(WPARAM wParam, LPARAM lParam);

protected:
	void StartNewSearch(bool bKademlia = false);
	void StartNewSearchKad();
	LRESULT OnCloseTab(WPARAM wparam, LPARAM lparam);
	CString	CreateWebQuery();
	void UpdateControls();
	void ShowResults(const SSearchParams* pParams);
	void SetAllIcons();

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnNMDblclkSearchlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnEnChangeSearchname();
	afx_msg void OnDestroy();
	afx_msg void OnCbnSelchangeCombo1();
	afx_msg void OnCbnSelendokCombo1();
	afx_msg void OnBnClickedMore();
	afx_msg void OnSysColorChange();

	DECLARE_MESSAGE_MAP()

private:
	Packet*		searchpacket;		
	UINT_PTR	global_search_timer;
	UINT		m_uTimerLocalServer;
	CProgressCtrl searchprogress;
	bool		canceld;
	uint16		servercount;
	bool		globsearch;
	uint32		m_nSearchID;
	CEditX		m_ctlName;
	CComboBoxEx2 methodBox;
	CComboBox	Stypebox;
	CImageList	m_ImageList;
	CTabCtrl	m_cattabs;
	CString		m_lastclpbrd;
	bool		m_guardCBPrompt;
	CCustomAutoComplete* m_pacSearchString;
	CString	ToQueryString(CString str);

	HICON icon_search;
	CIconStatic m_ctrlSearchFrm;
	CIconStatic m_ctrlDirectDlFrm;
	CImageList m_imlSearchMethods;
	LCID		m_uLangID;
	CButton		m_ctlMore;
	int			m_iSentMoreReq;

	CJigleSOAPThread* m_pJigleThread;
	bool DoNewJigleSearch(SSearchParams* pParams);
};

bool GetSearchPacket(CSafeMemFile* data,
					 const CString& strSearchString, const CString& strLocalizedType,
					 ULONG ulMinSize, ULONG ulMaxSize, int iAvailability, 
					 const CString& strExtension, bool bAllowEmptySearchString);