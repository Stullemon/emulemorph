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
#include "stdafx.h"
#include "emule.h"
#include "SearchDlg.h"
#include "Packets.h"
#include "OtherFunctions.h"
#include "SearchList.h"
#include "Sockets.h"
#include "ServerList.h"
#include "Server.h"
#include "SafeFile.h"
#include "DownloadQueue.h"
#include "UploadQueue.h"
#include "emuledlg.h"
#include "opcodes.h"
#include "ED2KLink.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/kademlia/search.h"
#include "CustomAutoComplete.h"
#include "SearchExpr.h"
#define USE_FLEX
#include "Parser.hpp"
#include "Scanner.h"
#include "Jigle/JigleSearch.h"
#include "TransferWnd.h" //MORPH - Added by SiRoB

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define	SEARCH_STRINGS_PROFILE	_T("AC_SearchStrings.dat")

typedef enum EOptsRows
{
	orCompleteSources,
	orCodec,
	orBitrate,
	orLength,
	orTitle,
	orAlbum,
	orArtist
};

extern int yyparse();
extern int yyerror(const char* errstr);
extern LPCTSTR _aszInvKadKeywordChars;

enum ESearchTimerID
{
	TimerServerTimeout = 1,
	TimerGlobalSearch
};

static const LPCTSTR _apszSearchExprKeywords[] = { _T("AND"), _T("OR"), _T("NOT"), NULL };
static const TCHAR _szSearchExprSeperators[] = _T(" ()<>=");

// CSearchDlg dialog

IMPLEMENT_DYNAMIC(CSearchDlg, CDialog)
CSearchDlg::CSearchDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CSearchDlg::IDD, pParent)
{
	m_nSearchID = 0x80000000;
	m_pacSearchString = NULL;
	m_pJigleThread = NULL;
	global_search_timer = 0;
	searchpacket = NULL;
	canceld = false;
	servercount = 0;
	globsearch = false;
	icon_search = NULL;
	m_uTimerLocalServer = 0;
	m_uLangID = MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT);
	m_iSentMoreReq = 0;
	searchselect.m_bCloseable = true;
}

CSearchDlg::~CSearchDlg()
{
	if (globsearch)
		delete searchpacket;
	if (icon_search)
		VERIFY( DestroyIcon(icon_search) );
	if (m_pacSearchString){
		m_pacSearchString->Unbind();
		m_pacSearchString->Release();
	}
	if (m_uTimerLocalServer)
		VERIFY( KillTimer(m_uTimerLocalServer) );
}

BOOL CSearchDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);
	theApp.searchlist->SetOutputWnd(&searchlistctrl);
	searchlistctrl.Init(theApp.searchlist);
	SetAllIcons();
	Localize();
	searchprogress.SetStep(1);
	global_search_timer = 0;
	globsearch = false;

	AddAnchor(IDC_SDOWNLOAD,BOTTOM_LEFT);
	AddAnchor(IDC_SEARCHLIST,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS1,BOTTOM_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_STARTS, TOP_RIGHT);
	AddAnchor(IDC_MORE, TOP_RIGHT);
	AddAnchor(IDC_CANCELS, TOP_RIGHT);
	AddAnchor(IDC_CLEARALL, TOP_RIGHT);
	AddAnchor(IDC_SEARCH_RESET, TOP_LEFT);
	AddAnchor(searchselect.m_hWnd,TOP_LEFT,TOP_RIGHT);
	// khaos::categorymod+ obsolete //AddAnchor(IDC_STATIC_DLTOof,BOTTOM_LEFT);
	// khaos::categorymod+ obsolete //AddAnchor(IDC_CATTAB2,BOTTOM_LEFT);
	AddAnchor(IDC_STATIC_FILTER,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SEARCH_OPTS,TOP_LEFT,TOP_RIGHT);

	ShowSearchSelector(false);

	m_ctlName.SendMessage(EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELONG(3, 3));
	m_ctlName.SetDisableSelectOnFocus(false);
	m_ctlName.SetSyntaxColoring(_apszSearchExprKeywords, _szSearchExprSeperators);

	if (thePrefs.GetUseAutocompletion()){
		m_pacSearchString = new CCustomAutoComplete();
		m_pacSearchString->AddRef();
		if (m_pacSearchString->Bind(m_ctlName, ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST))
			m_pacSearchString->LoadList(CString(thePrefs.GetConfigDir()) +  _T("\\") SEARCH_STRINGS_PROFILE);
		if (theApp.emuledlg->m_fontMarlett.m_hObject){
			GetDlgItem(IDC_DD)->SetFont(&theApp.emuledlg->m_fontMarlett);
			GetDlgItem(IDC_DD)->SetWindowText(_T("6")); // show a down-arrow
		}
	}
	else
		GetDlgItem(IDC_DD)->ShowWindow(SW_HIDE);
	//MORPH - Removed by SiRoB, IDC_CATTAB2 not used
	/*
	if (theApp.emuledlg->m_fontMarlett.m_hObject){
		GetDlgItem(IDC_STATIC_DLTOof)->SetFont(&theApp.emuledlg->m_fontMarlett);
		GetDlgItem(IDC_STATIC_DLTOof)->SetWindowText(_T("8")); // show a right-arrow
	}
	*/
	ASSERT( (GetDlgItem(IDC_EDITSEARCHMIN)->GetStyle() & ES_NUMBER) == 0 );
	ASSERT( (GetDlgItem(IDC_EDITSEARCHMAX)->GetStyle() & ES_NUMBER) == 0 );
	m_ctlName.LimitText(512); // max. length of search expression
	((CEdit*)GetDlgItem(IDC_EDITSEARCHEXTENSION))->LimitText(8); // max. length of file (type) extension

	if (methodBox.SetCurSel(thePrefs.GetSearchMethod()) == CB_ERR)
		methodBox.SetCurSel(SearchTypeServer);

	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy, theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	m_ctlOpts.SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (m_ctlOpts.GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	m_ctlOpts.SetExtendedStyle(LVS_EX_GRIDLINES);
	m_ctlOpts.InsertColumn(0, ""); // Parameter
	m_ctlOpts.InsertColumn(1, ""); // Value
	m_ctlOpts.InsertItem(orCompleteSources, GetResString(IDS_COMPLSOURCES));
	m_ctlOpts.InsertItem(orCodec, GetResString(IDS_CODEC));
	m_ctlOpts.InsertItem(orBitrate, GetResString(IDS_MINBITRATE));
	m_ctlOpts.InsertItem(orLength, GetResString(IDS_MINLENGTH));
	m_ctlOpts.InsertItem(orTitle, GetResString(IDS_TITLE));
	m_ctlOpts.InsertItem(orAlbum, GetResString(IDS_ALBUM));
	m_ctlOpts.InsertItem(orArtist, GetResString(IDS_ARTIST));
	m_ctlOpts.SetColumnWidth(0, LVSCW_AUTOSIZE);
	m_ctlOpts.SetColumnWidth(1, 120);

	UpdateControls();

	return true;
}

void CSearchDlg::DoDataExchange(CDataExchange* pDX){
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SEARCHLIST, searchlistctrl);
	DDX_Control(pDX, IDC_PROGRESS1, searchprogress);
	DDX_Control(pDX, IDC_COMBO1, methodBox);
	DDX_Control(pDX, IDC_TypeSearch, Stypebox);
	DDX_Control(pDX, IDC_TAB1, searchselect);
	DDX_Control(pDX, IDC_SEARCH_FRM, m_ctrlSearchFrm);
	// khaos::categorymod+ obsolete //DDX_Control(pDX, IDC_CATTAB2, m_cattabs);
	DDX_Control(pDX, IDC_MORE, m_ctlMore);
	DDX_Control(pDX, IDC_SEARCHNAME, m_ctlName);
	DDX_Control(pDX, IDC_SEARCH_OPTS, m_ctlOpts);
}


BEGIN_MESSAGE_MAP(CSearchDlg, CResizableDialog)
	ON_BN_CLICKED(IDC_STARTS, OnBnClickedStarts)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CANCELS, OnBnClickedCancels)
	ON_BN_CLICKED(IDC_SDOWNLOAD, OnBnClickedSdownload)
	ON_BN_CLICKED(IDC_CLEARALL, OnBnClickedClearall)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnTcnSelchangeTab1)
	ON_EN_CHANGE(IDC_SEARCHNAME, OnEnChangeSearchname)
	ON_CBN_SELCHANGE(IDC_TypeSearch, OnEnChangeSearchname)
	ON_MESSAGE(WM_CLOSETAB, OnCloseTab)
	ON_EN_CHANGE(IDC_SEARCHAVAIL , OnEnChangeSearchname)
	ON_EN_CHANGE(IDC_SEARCHEXTENTION, OnEnChangeSearchname)
	ON_EN_CHANGE(IDC_SEARCHMINSIZE, OnEnChangeSearchname)
	ON_EN_CHANGE(IDC_SEARCHMAXSIZE, OnEnChangeSearchname)
	ON_BN_CLICKED(IDC_SEARCH_RESET, OnBnClickedSearchReset)
	ON_BN_CLICKED(IDC_DD, OnDDClicked)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelchangeCombo1)
	ON_CBN_SELENDOK(IDC_COMBO1, OnCbnSelendokCombo1)
	ON_BN_CLICKED(IDC_MORE, OnBnClickedMore)
	ON_WM_SYSCOLORCHANGE()
	ON_WM_SIZE()
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CSearchDlg message handlers

void CSearchDlg::OnBnClickedStarts()
{
	searchprogress.SetPos(0);

	if (m_ctlOpts.GetEditCtrl()->GetSafeHwnd())
		m_ctlOpts.CommitEditCtrl();

	// start "normal" server-search
	if (m_ctlName.GetWindowTextLength())
	{
		if (m_pacSearchString && m_pacSearchString->IsBound()){
			CString strSearch;
			m_ctlName.GetWindowText(strSearch);
			if (!strSearch.IsEmpty())
				m_pacSearchString->AddItem(strSearch, 0);
		}
		switch (methodBox.GetCurSel())
		{
		case SearchTypeServer:
		case SearchTypeGlobal:
		case SearchTypeJigleSOAP:
			StartNewSearch();
			break;

		case SearchTypeKademlia:
			StartNewSearchKad();
			break;
		case SearchTypeFileDonkey:
		case SearchTypeJigle:
			ShellOpenFile(CreateWebQuery());
			return;
		}
	}
}

void CSearchDlg::OnTimer(UINT nIDEvent)
{
	CResizableDialog::OnTimer(nIDEvent);

	if (m_uTimerLocalServer != 0 && nIDEvent == m_uTimerLocalServer)
	{
		if (thePrefs.GetDebugServerSearchesLevel() > 0)
			Debug("Timeout waiting on search results of local server\n");
		// the local server did not answer within the timeout
		VERIFY( KillTimer(m_uTimerLocalServer) );
		m_uTimerLocalServer = 0;

		// start the global search
		if (globsearch)
		{
			if (global_search_timer == 0)
				VERIFY( (global_search_timer = SetTimer(TimerGlobalSearch, 750, 0)) != NULL );
		}
		else
			CancelSearch();
	}
	else if (nIDEvent == global_search_timer)
	{
	    if (theApp.serverconnect->IsConnected()){
		    CServer* toask = theApp.serverlist->GetNextSearchServer();
		    if (toask == theApp.serverlist->GetServerByAddress(theApp.serverconnect->GetCurrentServer()->GetAddress(),theApp.serverconnect->GetCurrentServer()->GetPort()))
			    toask = theApp.serverlist->GetNextSearchServer();
    
		    if (toask && theApp.serverlist->GetServerCount()-1 != servercount){
			    servercount++;
				if (toask->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)
					searchpacket->opcode = OP_GLOBSEARCHREQ2;
				else
					searchpacket->opcode = OP_GLOBSEARCHREQ;
				if (thePrefs.GetDebugServerUDPLevel() > 0)
					Debug(">>> Sending %s  to server %s:%u (%u of %u)\n", (searchpacket->opcode == OP_GLOBSEARCHREQ2) ? "OP__GlobSearchReq2" : "OP__GlobSearchReq", toask->GetAddress(), toask->GetPort(), servercount, theApp.serverlist->GetServerCount());
				theApp.uploadqueue->AddUpDataOverheadServer(searchpacket->size);
			    theApp.serverconnect->SendUDPPacket(searchpacket,toask,false);
			    searchprogress.StepIt();
		    }
		    else
				CancelSearch();
	    }
	    else
			CancelSearch();
    }
	else
		ASSERT( 0 );
}

void CSearchDlg::OnBnClickedCancels()
{
	CancelSearch();
}

void CSearchDlg::CancelSearch()
{
	canceld = true;

	// delete any global search timer
	if (globsearch){
		delete searchpacket;
		searchpacket = NULL;
	}
	globsearch = false;
	if (global_search_timer){
		VERIFY( KillTimer(global_search_timer) );
		global_search_timer = 0;
		searchprogress.SetPos(0);
	}

	// delete local server timeout timer
	if (m_uTimerLocalServer){
		VERIFY( KillTimer(m_uTimerLocalServer) );
		m_uTimerLocalServer = 0;
	}

	// set focus
	CWnd* pWndFocus = GetFocus();
	GetDlgItem(IDC_CANCELS)->EnableWindow(false);
	if (pWndFocus == GetDlgItem(IDC_CANCELS))
		m_ctlName.SetFocus();
	GetDlgItem(IDC_STARTS)->EnableWindow(true);
}

void CSearchDlg::LocalSearchEnd(uint16 count, bool bMoreResultsAvailable)
{
	// local server has answered, kill the timeout timer
	if (m_uTimerLocalServer){
		VERIFY( KillTimer(m_uTimerLocalServer) );
		m_uTimerLocalServer = 0;
	}

	if (!canceld && count > MAX_RESULTS)
		CancelSearch();
	if (!canceld){
		if (!globsearch){
			GetDlgItem(IDC_STARTS)->EnableWindow(true);
			CWnd* pWndFocus = GetFocus();
			GetDlgItem(IDC_CANCELS)->EnableWindow(false);
			if (pWndFocus == GetDlgItem(IDC_CANCELS))
				m_ctlName.SetFocus();
		}
		else{
			VERIFY( (global_search_timer = SetTimer(TimerGlobalSearch, 750, 0)) != NULL );
		}
	}
	m_ctlMore.EnableWindow(bMoreResultsAvailable && m_iSentMoreReq < MAX_MORE_SEARCH_REQ);
}

void CSearchDlg::AddUDPResult(uint16 count){
	if (!canceld && count > MAX_RESULTS)
		CancelSearch();
}

void CSearchDlg::OnBnClickedSdownload(){
	//start download(s)
	DownloadSelected();
}

BOOL CSearchDlg::PreTranslateMessage(MSG* pMsg) 
{
	if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE))
	   	return FALSE;

	if( m_pacSearchString && m_pacSearchString->IsBound() && ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_DELETE) && (pMsg->hwnd == m_ctlName.m_hWnd) && (GetAsyncKeyState(VK_MENU)<0 || GetAsyncKeyState(VK_CONTROL)<0)) )
		m_pacSearchString->Clear();

   	if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_RETURN))
	{
	   	if (pMsg->hwnd == GetDlgItem(IDC_SEARCHLIST)->m_hWnd)
			OnBnClickedSdownload();
	   	else if (pMsg->hwnd == m_ctlName.m_hWnd || 
				 pMsg->hwnd == GetDlgItem(IDC_TypeSearch)->m_hWnd || 
				 pMsg->hwnd == GetDlgItem(IDC_EDITSEARCHMIN)->m_hWnd || 
				 pMsg->hwnd == GetDlgItem(IDC_EDITSEARCHMAX)->m_hWnd || 
				 pMsg->hwnd == GetDlgItem(IDC_EDITSEARCHAVAIBILITY)->m_hWnd || 
				 pMsg->hwnd == GetDlgItem(IDC_COMBO1)->m_hWnd || 
				 pMsg->hwnd == ((CComboBoxEx2*)GetDlgItem(IDC_COMBO1))->GetComboBoxCtrl()->GetSafeHwnd() ||
				 pMsg->hwnd == GetDlgItem(IDC_EDITSEARCHEXTENSION)->m_hWnd ||
				 pMsg->hwnd == m_ctlOpts.m_hWnd ||
				 pMsg->hwnd == m_ctlOpts.GetEditCtrl()->GetSafeHwnd()
				){
			if (m_pacSearchString && m_pacSearchString->IsBound() && pMsg->hwnd == m_ctlName.m_hWnd){
				CString strText;
				m_ctlName.GetWindowText(strText);
				if (!strText.IsEmpty()){
					m_ctlName.SetWindowText(_T("")); // this seems to be the only chance to let the dropdown list to disapear
					m_ctlName.SetWindowText(strText);
					m_ctlName.SetSel(strText.GetLength(), strText.GetLength());
				}
			}
			OnBnClickedStarts();
   		}
	}

	return CResizableDialog::PreTranslateMessage(pMsg);
}

void CSearchDlg::OnNMDblclkSearchlist(NMHDR *pNMHDR, LRESULT *pResult){
	OnBnClickedSdownload();
	*pResult = 0;
}

ULONG GetSearchSize(const CString& strExpr)
{
	ULONG ulNum;
	TCHAR szUnit[40];
	int iArgs = _stscanf(strExpr, _T("%u%s"), &ulNum, szUnit);
	if (iArgs <= 0)
		return 0;
	if (iArgs == 2){
		CString strUnits(szUnit);
		strUnits.Trim();
		if (!strUnits.IsEmpty()){
			if (strUnits.CompareNoCase(_T("b")) == 0 || strUnits.CompareNoCase(_T("byte")) == 0 || strUnits.CompareNoCase(_T("bytes")) == 0)
				return ulNum * 1U; // Bytes
			else if (strUnits.CompareNoCase(_T("k")) == 0 || strUnits.CompareNoCase(_T("kb")) == 0 || strUnits.CompareNoCase(_T("kbyte")) == 0 || strUnits.CompareNoCase(_T("kbytes")) == 0)
				return ulNum * 1024U; // KBytes
			else if (strUnits.CompareNoCase(_T("m")) == 0 || strUnits.CompareNoCase(_T("mb")) == 0 || strUnits.CompareNoCase(_T("mbyte")) == 0 || strUnits.CompareNoCase(_T("mbytes")) == 0)
				return ulNum * 1024U*1024; // MBytes
			else if (strUnits.CompareNoCase(_T("g")) == 0 || strUnits.CompareNoCase(_T("gb")) == 0 || strUnits.CompareNoCase(_T("gbyte")) == 0 || strUnits.CompareNoCase(_T("gbytes")) == 0)
				return ulNum * 1024U*1024U*1024U; // GBytes
			else{
				AfxMessageBox(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + GetResString(IDS_SEARCH_INVALIDMINMAX));
				return (ULONG)-1;
			}
		}
	}

	return ulNum * 1024U*1024U; // Default = MBytes
}

CString	CSearchDlg::CreateWebQuery()
{
	CString query,tosearch;
	unsigned long num;
	switch (methodBox.GetCurSel())
	{
	case SearchTypeFileDonkey:
		query = "http://www.filedonkey.com/search.html?";
		m_ctlName.GetWindowText(tosearch);
		query += "pattern="+ToQueryString(tosearch);
		GetDlgItem(IDC_TypeSearch)->GetWindowText(tosearch);
		if (GetResString(IDS_SEARCH_AUDIO)==tosearch)
			tosearch="Audio";
		else if (GetResString(IDS_SEARCH_VIDEO)==tosearch)
			tosearch="Video";
		else if (GetResString(IDS_SEARCH_PRG)==tosearch)
			tosearch="Pro";
		else
			tosearch="All";
		query +="&media=" + tosearch + "&requestby=emule";

		GetDlgItem(IDC_EDITSEARCHMIN)->GetWindowText(tosearch);
		if ((num=GetSearchSize(tosearch)) == (ULONG)-1)
			return "";
		if (num>0)
			query.AppendFormat("&min_size=%u",num);
		
		GetDlgItem(IDC_EDITSEARCHMAX)->GetWindowText(tosearch);
		if ((num=GetSearchSize(tosearch)) == (ULONG)-1)
			return "";
		if (num>0)
			query.AppendFormat("&max_size=%u",num);

		break;
	case SearchTypeJigle:
		// www.jigle.com
		//
		// Type				ID
		// --------------------------
		//   Any			0
		//   Audio			1
		//   Video			2
		//   Image			3
		//   Program		4
		//   Document		5
		//   Collection		6

		query = "http://www.jigle.com/search?";
		m_ctlName.GetWindowText(tosearch);
		query += "p="+ToQueryString(tosearch) + "&ma=";
		GetDlgItem(IDC_EDITSEARCHAVAIBILITY)->GetWindowText(tosearch);
		if (atol(tosearch)>1000) tosearch="1000";
		else if (atoi(tosearch)>500) tosearch="500";
		else if (atoi(tosearch)>200) tosearch="200";
		else if (atoi(tosearch)>100) tosearch="100";
		else if (atoi(tosearch)>50) tosearch="50";
		else if (atoi(tosearch)>20) tosearch="20";
		else if (atoi(tosearch)>10) tosearch="10";
		else tosearch="1";
		query += tosearch +"&v=0&d=1&a=0&l=10";		//Cax2 might want to change to l=25 for more results per page... 
		GetDlgItem(IDC_TypeSearch)->GetWindowText(tosearch);
		if (GetResString(IDS_SEARCH_AUDIO)==tosearch)
			tosearch="1";
		else if (GetResString(IDS_SEARCH_VIDEO)==tosearch)
			tosearch="2";
		else if (GetResString(IDS_SEARCH_PICS)==tosearch)
			tosearch="3";
		else if (GetResString(IDS_SEARCH_PRG)==tosearch)
			tosearch="4";
		else
			tosearch="0";
		query +="&t=" +tosearch;
		GetDlgItem(IDC_EDITSEARCHEXTENSION)->GetWindowText(tosearch);
		query +="&x=" + ToQueryString(tosearch);
		GetDlgItem(IDC_EDITSEARCHMIN)->GetWindowText(tosearch);
		if ((num=GetSearchSize(tosearch)) == (ULONG)-1)
			return "";
		tosearch.Format("%u",((num>0)?num:1));
		query +="&sl=" +tosearch;
		GetDlgItem(IDC_EDITSEARCHMAX)->GetWindowText(tosearch);
		if ((num=GetSearchSize(tosearch)) == (ULONG)-1)
			return "";
		tosearch.Format(((num>0)?"%u":""),num);
		query +="&su=" + tosearch;
		if (IsDlgButtonChecked(IDC_MATCH_KEYWORDS))
			query += _T("&kw=1");
		break;
	default:
		return "";
	}
	return query;
}

void CSearchDlg::DownloadSelected()
{
	DownloadSelected(thePrefs.AddNewFilesPaused());
}

void CSearchDlg::DownloadSelected(bool paused)
{
	CWaitCursor curWait;
	POSITION pos = searchlistctrl.GetFirstSelectedItemPosition(); 
	
	// khaos::categorymod+ Category selection stuff...
	if (!pos) return; // No point in asking for a category if there are no selected files to download.

	int useCat;

	if(thePrefs.SelectCatForNewDL())
	{
		CSelCategoryDlg getCatDlg;
		getCatDlg.DoModal();

		// Returns 0 on 'Cancel', otherwise it returns the selected category
		// or the index of a newly created category.  Users can opt to add the
		// links into a new category.
		useCat = getCatDlg.GetInput();
	}
	else if(thePrefs.UseActiveCatForLinks())
		useCat = theApp.emuledlg->transferwnd->GetActiveCategory();
	else
		useCat = 0;

	int useOrder = theApp.downloadqueue->GetMaxCatResumeOrder(useCat);
	// khaos::categorymod-

	while (pos != NULL) 
	{ 
		int index = searchlistctrl.GetNextSelectedItem(pos); 
		if (index > -1)
		{
			CSearchFile* cur_file = (CSearchFile*)searchlistctrl.GetItemData(index);
			// khaos::categorymod+ m_cattabs is obsolete.
			if (!thePrefs.SelectCatForNewDL() && thePrefs.UseAutoCat())
			{
				useCat = theApp.downloadqueue->GetAutoCat(CString(cur_file->GetFileName()), (ULONG)cur_file->GetFileSize());
				if (!useCat && thePrefs.UseActiveCatForLinks())
					useCat = theApp.emuledlg->transferwnd->GetActiveCategory();
			}

			if (thePrefs.SmallFileDLPush() && cur_file->GetFileSize() < 154624)
				theApp.downloadqueue->AddSearchToDownload(cur_file, paused, useCat, 0);
			else
			{
				useOrder++;
				theApp.downloadqueue->AddSearchToDownload(cur_file, paused, useCat, useOrder);
			}
			// khaos::categorymod-
			if (cur_file->GetListParent()!=NULL)
				cur_file=cur_file->GetListParent();
			searchlistctrl.UpdateSources(cur_file);
		}
	}
}

void CSearchDlg::OnSysColorChange()
{
	CResizableDialog::OnSysColorChange();
	SetAllIcons();
}

void CSearchDlg::SetAllIcons()
{
	m_ctrlSearchFrm.Init("SearchParams");

	if (icon_search)
		VERIFY( DestroyIcon(icon_search) );
	icon_search = theApp.LoadIcon("SearchResults", 16, 16);
	((CStatic*)GetDlgItem(IDC_SEARCHLST_ICO))->SetIcon(icon_search);

	CImageList iml;
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.SetBkColor(CLR_NONE);
	iml.Add(CTempIconLoader("SearchMethod_SERVER", 16, 16));
	iml.Add(CTempIconLoader("SearchMethod_GLOBAL", 16, 16));
	iml.Add(CTempIconLoader("SearchMethod_JIGLE", 16, 16));
	iml.Add(CTempIconLoader("SearchMethod_KADEMLIA", 16, 16));
	iml.Add(CTempIconLoader("StatsClients", 16, 16));
	searchselect.SetImageList(&iml);
	m_imlSearchResults.DeleteImageList();
	m_imlSearchResults.Attach(iml.Detach());
	searchselect.SetPadding(CSize(10, 3));

	iml.Create(13,13,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.SetBkColor(CLR_NONE);
	iml.Add(CTempIconLoader("SearchMethod_SERVER", 13, 13));
	iml.Add(CTempIconLoader("SearchMethod_GLOBAL", 13, 13));
	iml.Add(CTempIconLoader("SearchMethod_JIGLE", 13, 13));
	iml.Add(CTempIconLoader("SearchMethod_FILEDONKEY", 13, 13));
	iml.Add(CTempIconLoader("SearchMethod_KADEMLIA", 13, 13));
	methodBox.SetImageList(&iml);
	m_imlSearchMethods.DeleteImageList();
	m_imlSearchMethods.Attach(iml.Detach());
}

void CSearchDlg::Localize()
{
	searchlistctrl.Localize();
	//MORPH Removed by SiRoB, Due to Khaos Category
	/*
	UpdateCatTabs();
	*/

	if (thePrefs.GetLanguageID() != m_uLangID){
		m_uLangID = thePrefs.GetLanguageID();
		m_ctrlSearchFrm.SetWindowText(GetResString(IDS_SW_SEARCHBOX));
		m_ctrlSearchFrm.SetText(GetResString(IDS_SW_SEARCHBOX));

		GetDlgItem(IDC_MSTATIC3)->SetWindowText(GetResString(IDS_SW_NAME));
		GetDlgItem(IDC_MSTATIC7)->SetWindowText(GetResString(IDS_SW_TYPE));
		GetDlgItem(IDC_STATIC_FILTER)->SetWindowText(GetResString(IDS_FILTER));
		GetDlgItem(IDC_STARTS)->SetWindowText(GetResString(IDS_SW_START));
		GetDlgItem(IDC_CANCELS)->SetWindowText(GetResString(IDS_CANCEL));
		GetDlgItem(IDC_CLEARALL)->SetWindowText(GetResString(IDS_REMOVEALLSEARCH));
		GetDlgItem(IDC_RESULTS_LBL)->SetWindowText(GetResString(IDS_SW_RESULT));
		GetDlgItem(IDC_SDOWNLOAD)->SetWindowText(GetResString(IDS_SW_DOWNLOAD));
		GetDlgItem(IDC_MSTATIC7)->SetWindowText(GetResString(IDS_TYPE));
		GetDlgItem(IDC_SEARCH_RESET)->SetWindowText(GetResString(IDS_PW_RESET));	
		GetDlgItem(IDC_SEARCHMINSIZE)->SetWindowText(GetResString(IDS_SEARCHMINSIZE)+":");
		GetDlgItem(IDC_SEARCHMAXSIZE)->SetWindowText(GetResString(IDS_SEARCHMAXSIZE)+":");
		GetDlgItem(IDC_SEARCHEXTENTION)->SetWindowText(GetResString(IDS_SEARCHEXTENTION)+":");
		GetDlgItem(IDC_SEARCHAVAIL)->SetWindowText(GetResString(IDS_SEARCHAVAIL)+":");
		GetDlgItem(IDC_METH)->SetWindowText(GetResString(IDS_METHOD));
		GetDlgItem(IDC_SD_MB1)->SetWindowText(GetResString(IDS_MBYTES));
		GetDlgItem(IDC_SD_MB2)->SetWindowText(GetResString(IDS_MBYTES));
	  	SetDlgItemText(IDC_MATCH_KEYWORDS, GetResString(IDS_MATCH_KEYWORDS));
		SetDlgItemText(IDC_MORE,GetResString(IDS_MORE));

		m_ctlOpts.SetItemText(orCompleteSources, 0, GetResString(IDS_COMPLSOURCES));
		m_ctlOpts.SetItemText(orCodec, 0, GetResString(IDS_CODEC));
		m_ctlOpts.SetItemText(orBitrate, 0, GetResString(IDS_MINBITRATE));
		m_ctlOpts.SetItemText(orLength, 0, GetResString(IDS_MINLENGTH));
		m_ctlOpts.SetItemText(orTitle, 0, GetResString(IDS_TITLE));
		m_ctlOpts.SetItemText(orAlbum, 0, GetResString(IDS_ALBUM));
		m_ctlOpts.SetItemText(orArtist, 0, GetResString(IDS_ARTIST));

	}

	int iMethod = methodBox.GetCurSel();
	methodBox.ResetContent();
	VERIFY( methodBox.AddItem(GetResString(IDS_SERVER), 0) == SearchTypeServer );
	VERIFY( methodBox.AddItem(GetResString(IDS_GLOBALSEARCH), 1) == SearchTypeGlobal );
	VERIFY( methodBox.AddItem(_T("Kad"), 4) == SearchTypeKademlia );
	VERIFY( methodBox.AddItem(_T("Jigle"), 2) == SearchTypeJigleSOAP );
	VERIFY( methodBox.AddItem(_T("Jigle (Web)"), 2) == SearchTypeJigle );
	VERIFY( methodBox.AddItem(_T("FileDonkey (Web)"), 3) == SearchTypeFileDonkey );
	UpdateHorzExtent(methodBox, 13); // adjust dropped width to ensure all strings are fully visible
	methodBox.SetCurSel(iMethod != CB_ERR ? iMethod : SearchTypeServer);

	while (Stypebox.GetCount()>0) Stypebox.DeleteString(0);
	Stypebox.AddString(GetResString(IDS_SEARCH_ANY));
	Stypebox.AddString(GetResString(IDS_SEARCH_ARC));
	Stypebox.AddString(GetResString(IDS_SEARCH_AUDIO));
	Stypebox.AddString(GetResString(IDS_SEARCH_CDIMG));
	Stypebox.AddString(GetResString(IDS_SEARCH_PICS));
	Stypebox.AddString(GetResString(IDS_SEARCH_PRG));
	Stypebox.AddString(GetResString(IDS_SEARCH_VIDEO));
	Stypebox.SetCurSel(Stypebox.FindString(-1,GetResString(IDS_SEARCH_ANY)));
}

void CSearchDlg::OnBnClickedClearall()
{
	CancelSearch();
	DeleteAllSearchs();

	CWnd* pWndFocus = GetFocus();
	m_ctlMore.EnableWindow(FALSE);
	if (pWndFocus && pWndFocus->m_hWnd == m_ctlMore.m_hWnd)
		GetDlgItem(IDC_STARTS)->SetFocus();
}

static CSearchExpr _SearchExpr;
CStringArray _astrParserErrors;

#ifdef _DEBUG
static char _chLastChar = 0;
static CString _strSearchTree;

bool DumpSearchTree(int& iExpr, const CSearchExpr& rSearchExpr)
{
	if (iExpr >= rSearchExpr.m_aExpr.GetCount())
		return false;
	CString strTok = rSearchExpr.m_aExpr[iExpr++];
	if (strTok == SEARCHOPTOK_AND || strTok == SEARCHOPTOK_OR || strTok == SEARCHOPTOK_NOT){
		if (_chLastChar != '(' && _chLastChar != '\0')
			_strSearchTree.AppendFormat(" ");
		_strSearchTree.AppendFormat("(%s ", strTok.Mid(1));
		_chLastChar = '(';
		DumpSearchTree(iExpr, rSearchExpr);
		DumpSearchTree(iExpr, rSearchExpr);
		_strSearchTree.AppendFormat(")");
		_chLastChar = ')';
	}
	else{
		if (_chLastChar != '(' && _chLastChar != '\0')
			_strSearchTree.AppendFormat(" ");
		_strSearchTree.AppendFormat("\"%s\"", strTok);
		_chLastChar = '\1';
	}
	return true;
}

bool DumpSearchTree(const CSearchExpr& rSearchExpr)
{
	_chLastChar = '\0';
	int iExpr = 0;
	return DumpSearchTree(iExpr, rSearchExpr);
}
#endif//!_DEBUG

void ParsedSearchExpression(const CSearchExpr* pexpr)
{
	int iOpAnd = 0;
	int iOpOr = 0;
	int iOpNot = 0;
	CString strDbg;
	for (int i = 0; i < pexpr->m_aExpr.GetCount(); i++){
		CString str(pexpr->m_aExpr[i]);
		if (str == SEARCHOPTOK_AND){
			iOpAnd++;
			strDbg.AppendFormat("%s ", str.Mid(1));
		}
		else if (str == SEARCHOPTOK_OR){
			iOpOr++;
			strDbg.AppendFormat("%s ", str.Mid(1));
		}
		else if (str == SEARCHOPTOK_NOT){
			iOpNot++;
			strDbg.AppendFormat("%s ", str.Mid(1));
		}
		else{
			strDbg.AppendFormat("\"%s\" ", str);
		}
	}
	if (thePrefs.GetDebugServerSearchesLevel() > 0)
		Debug("Search Expr: %s\n", strDbg);

	// this limit (+ the additonal operators which will be added later) has to match the limit in 'CreateSearchExpressionTree'
	//	+1 Type (Audio, Video)
	//	+1 MinSize
	//	+1 MaxSize
	//	+1 Avail
	//	+1 Extension
	//	+1 Complete sources
	//	+1 Codec
	//	+1 Bitrate
	//	+1 Length
	//	+1 Title
	//	+1 Album
	//	+1 Artist
	// ---------------
	//  12
	if (iOpAnd + iOpOr + iOpNot > 10)
		yyerror(GetResString(IDS_SEARCH_TOOCOMPLEX));

	_SearchExpr.m_aExpr.RemoveAll();
	// optimize search expression, if no OR nor NOT specified
	if (iOpAnd > 0 && iOpOr == 0 && iOpNot == 0){
		CString strAndTerms;
		for (int i = 0; i < pexpr->m_aExpr.GetCount(); i++){
			if (pexpr->m_aExpr[i] != SEARCHOPTOK_AND){
				if (!strAndTerms.IsEmpty())
					strAndTerms += _T(' ');
				strAndTerms += pexpr->m_aExpr[i];
			}
		}
		ASSERT( _SearchExpr.m_aExpr.GetCount() == 0);
		_SearchExpr.m_aExpr.Add(strAndTerms);
	}
	else
		_SearchExpr.m_aExpr.Append(pexpr->m_aExpr);

#ifdef _DEBUG
	if (thePrefs.GetDebugServerSearchesLevel() > 0){
		_strSearchTree.Empty();
		DumpSearchTree(_SearchExpr);
		Debug("Search Tree: %s\n", _strSearchTree);
	}
#endif
}

CString DbgGetMetaTagName(UINT uMetaTagID)
{
	extern CString GetMetaTagName(UINT uTagID);
	return GetMetaTagName(uMetaTagID);
}

CString DbgGetMetaTagName(LPCSTR pszMetaTagID)
{
	extern CString GetMetaTagName(UINT uTagID);

	if (strlen(pszMetaTagID) == 1)
		return GetMetaTagName(((BYTE*)pszMetaTagID)[0]);
	CString strName;
	strName.Format(_T("\"%s\""), pszMetaTagID);
	return strName;
}

CString DbgGetOperatorName(bool bEd2k, UINT uOperator)
{
	static const LPCTSTR _aszEd2kOps[] = 
	{
		_T("="),
		_T(">"),
		_T("<"),
		_T(">="),
		_T("<="),
		_T("<>"),
	};
	static const LPCTSTR _aszKadOps[] = 
	{
		_T("="),
		_T(">="),
		_T("<="),
		_T(">"),
		_T("<"),
		_T("<>"),
	};

	if (bEd2k)
	{
		if (uOperator >= ARRSIZE(_aszEd2kOps)){
			ASSERT(0);
			return _T("*UnkOp*");
		}
		return _aszEd2kOps[uOperator];
	}
	else
	{
		if (uOperator >= ARRSIZE(_aszKadOps)){
			ASSERT(0);
			return _T("*UnkOp*");
		}
		return _aszKadOps[uOperator];
	}
}

void WriteBooleanAND(CString& rstrDbg, CSafeMemFile& data)
{
	data.WriteUInt8(0);				// boolean operator parameter type
	data.WriteUInt8(0x00);			// "AND"
	rstrDbg.AppendFormat("AND ");
}

void WriteBooleanOR(CString& rstrDbg, CSafeMemFile& data)
{
	data.WriteUInt8(0);				// boolean operator parameter type
	data.WriteUInt8(0x01);			// "OR"
	rstrDbg.AppendFormat("OR ");
}

void WriteBooleanNOT(CString& rstrDbg, CSafeMemFile& data)
{
	data.WriteUInt8(0);				// boolean operator parameter type
	data.WriteUInt8(0x02);			// "NOT"
	rstrDbg.AppendFormat("NOT ");
}

void WriteMetaDataSearchParam(CString& rstrDbg, CSafeMemFile& data, const CString& rstrValue)
{
	data.WriteUInt8(1);				// string parameter type
	data.WriteString(rstrValue);	// string value
	rstrDbg.AppendFormat("\"%s\" ", rstrValue);
}

void WriteMetaDataSearchParam(CString& rstrDbg, CSafeMemFile& data, UINT uMetaTagID, const CString& rstrValue)
{
	data.WriteUInt8(2);				// string parameter type
	data.WriteString(rstrValue);	// string value
	data.WriteUInt16(sizeof uint8);	// meta tag ID length
	data.WriteUInt8(uMetaTagID);	// meta tag ID name
	rstrDbg.AppendFormat("%s=\"%s\" ", DbgGetMetaTagName(uMetaTagID), rstrValue);
}

void WriteMetaDataSearchParam(CString& rstrDbg, CSafeMemFile& data, LPCSTR pszMetaTagID, const CString& rstrValue)
{
	data.WriteUInt8(2);				// string parameter type
	data.WriteString(rstrValue);	// string value
	data.WriteString(pszMetaTagID);	// meta tag ID
	rstrDbg.AppendFormat("%s=\"%s\" ", DbgGetMetaTagName(pszMetaTagID), rstrValue);
}

void WriteMetaDataSearchParam(CString& rstrDbg, CSafeMemFile& data, UINT uMetaTagID, UINT uOperator, UINT uValue, bool bEd2k)
{
	data.WriteUInt8(3);				// numeric parameter type
	data.WriteUInt32(uValue);		// numeric value
	data.WriteUInt8(uOperator);		// comparison operator
	data.WriteUInt16(sizeof uint8);	// meta tag ID length
	data.WriteUInt8(uMetaTagID);	// meta tag ID name
	rstrDbg.AppendFormat("%s%s%u ", DbgGetMetaTagName(uMetaTagID), DbgGetOperatorName(bEd2k, uOperator), uValue);
}

void WriteMetaDataSearchParam(CString& rstrDbg, CSafeMemFile& data, LPCSTR pszMetaTagID, UINT uOperator, UINT uValue, bool bEd2k)
{
	data.WriteUInt8(3);				// numeric parameter type
	data.WriteUInt32(uValue);		// numeric value
	data.WriteUInt8(uOperator);		// comparison operator
	data.WriteString(pszMetaTagID);	// meta tag ID
	rstrDbg.AppendFormat("%s%s%u ", DbgGetMetaTagName(pszMetaTagID), DbgGetOperatorName(bEd2k, uOperator), uValue);
}

void WriteOldMinMetaDataSearchParam(CString& rstrDbg, CSafeMemFile& data, UINT uMetaTagID, UINT uValue, bool bEd2k)
{
	UINT uOperator;
	if (bEd2k){
		uOperator = ED2K_SEARCH_OP_GREATER;
		uValue -= 1;
	}
	else
		uOperator = KAD_SEARCH_OP_GREATER_EQUAL;
	WriteMetaDataSearchParam(rstrDbg, data, uMetaTagID, uOperator, uValue, bEd2k);
}

void WriteOldMinMetaDataSearchParam(CString& rstrDbg, CSafeMemFile& data, LPCSTR pszMetaTagID, UINT uValue, bool bEd2k)
{
	UINT uOperator;
	if (bEd2k){
		uOperator = ED2K_SEARCH_OP_GREATER;
		uValue -= 1;
	}
	else
		uOperator = KAD_SEARCH_OP_GREATER_EQUAL;
	WriteMetaDataSearchParam(rstrDbg, data, pszMetaTagID, uOperator, uValue, bEd2k);
}

void WriteOldMaxMetaDataSearchParam(CString& rstrDbg, CSafeMemFile& data, LPCSTR pszMetaTagID, UINT uValue, bool bEd2k)
{
	UINT uOperator;
	if (bEd2k){
		uOperator = ED2K_SEARCH_OP_LESS;
		uValue += 1;
	}
	else
		uOperator = KAD_SEARCH_OP_LESS_EQUAL;
	WriteMetaDataSearchParam(rstrDbg, data, pszMetaTagID, uOperator, uValue, bEd2k);
}

void WriteOldMaxMetaDataSearchParam(CString& rstrDbg, CSafeMemFile& data, UINT uMetaTagID, UINT uValue, bool bEd2k)
{
	UINT uOperator;
	if (bEd2k){
		uOperator = ED2K_SEARCH_OP_LESS;
		uValue += 1;
	}
	else
		uOperator = KAD_SEARCH_OP_LESS_EQUAL;
	WriteMetaDataSearchParam(rstrDbg, data, uMetaTagID, uOperator, uValue, bEd2k);
}

bool GetSearchPacket(CSafeMemFile* pData,
					 const CString& strSearchString, const CString& strLocalizedType,
					 ULONG ulMinSize, ULONG ulMaxSize, UINT uAvailability, const CString& strExtension, 
					 UINT uComplete, ULONG ulMinBitrate, ULONG ulMinLength, const CString& strCodec,
					 const CString& strTitle, const CString& strAlbum, const CString& strArtist,
					 bool bAllowEmptySearchString, bool bEd2k)
{
	CSafeMemFile& data = *pData;

	//TODO: use 'GetE2DKFileTypeSearchTerm'
	CString strType;
	if (GetResString(IDS_SEARCH_AUDIO) == strLocalizedType)
		strType = _T("Audio");
	else if (GetResString(IDS_SEARCH_VIDEO) == strLocalizedType)
		strType = _T("Video");
	else if (GetResString(IDS_SEARCH_PRG) == strLocalizedType)
		strType = _T("Pro");
	else if (GetResString(IDS_SEARCH_PICS) == strLocalizedType)
		strType = _T("Image");
	else if (GetResString(IDS_SEARCH_ARC) == strLocalizedType){
		// eDonkeyHybrid 0.48 uses type "Pro" for archives files
		// www.filedonkey.com uses type "Pro" for archives files
		//strType = _T("Pro"); //TODO: Use a proper type - filtering the search results locally is evil!
	}
	else if (GetResString(IDS_SEARCH_CDIMG) == strLocalizedType){
		// eDonkeyHybrid 0.48 uses *no* type for iso/nrg/cue/img files
		// www.filedonkey.com uses type "Pro" for CD-image files
		//strType = _T("Pro"); //TODO: Use a proper type - filtering the search results locally is evil!
	}
	else{
		//TODO: Support "Doc" types
		ASSERT( GetResString(IDS_SEARCH_ANY) == strLocalizedType );
	}

	if (!bAllowEmptySearchString && strSearchString.IsEmpty() && strExtension.IsEmpty())
		return false;

	_astrParserErrors.RemoveAll();
	_SearchExpr.m_aExpr.RemoveAll();
	if (!strSearchString.IsEmpty()){
	    LexInit(strSearchString);
	    int iParseResult = yyparse();
	    LexFree();
	    if (_astrParserErrors.GetSize() > 0){
		    _SearchExpr.m_aExpr.RemoveAll();
		    AfxMessageBox(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + _astrParserErrors[_astrParserErrors.GetSize() - 1], MB_ICONWARNING);
		    return false;
	    }
	    else if (iParseResult != 0){
		    _SearchExpr.m_aExpr.RemoveAll();
		    AfxMessageBox(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + GetResString(IDS_SEARCH_GENERALERROR), MB_ICONWARNING);
		    return false;
	    }
	}

	// get total nr. of search terms
	int iTotalTerms = 0;
	if (!strExtension.IsEmpty())
		iTotalTerms++;
	if (uAvailability > 0)
		iTotalTerms++;
	if (ulMaxSize > 0)
		iTotalTerms++;
	if (ulMinSize > 0)
		iTotalTerms++;
	if (!strType.IsEmpty())
		iTotalTerms++;
	if (uComplete > 0)
		iTotalTerms++;
	if (ulMinBitrate > 0)
		iTotalTerms++;
	if (ulMinLength > 0)
		iTotalTerms++;
	if (!strCodec.IsEmpty())
		iTotalTerms++;
	if (!strTitle.IsEmpty())
		iTotalTerms++;
	if (!strAlbum.IsEmpty())
		iTotalTerms++;
	if (!strArtist.IsEmpty())
		iTotalTerms++;
	iTotalTerms += _SearchExpr.m_aExpr.GetCount();

	// create ed2k search expression
	CString strDbg;
	int iParameterCount = 0;
	if (_SearchExpr.m_aExpr.GetCount() <= 1)
	{
		// If we don't have a NOT or OR operator we use a series of AND terms which can be processed by
		// the servers with less load (request lugdunummaster)
		//
		// input:      "a" AND min=1 AND max=2
		// instead of: AND AND "a" min=1 max=2
		// we use:     AND "a" AND min=1 max=2

		if (_SearchExpr.m_aExpr.GetCount() > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteMetaDataSearchParam(strDbg, data, _SearchExpr.m_aExpr[0]);
		}

		if (!strType.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteMetaDataSearchParam(strDbg, data, FT_FILETYPE, strType);
		}
		
		if (ulMinSize > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteOldMinMetaDataSearchParam(strDbg, data, FT_FILESIZE, ulMinSize, bEd2k);
		}

		if (ulMaxSize > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteOldMaxMetaDataSearchParam(strDbg, data, FT_FILESIZE, ulMaxSize, bEd2k);
		}
		
		if (uAvailability > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteOldMinMetaDataSearchParam(strDbg, data, FT_SOURCES, uAvailability, bEd2k);
		}

		if (!strExtension.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteMetaDataSearchParam(strDbg, data, FT_FILEFORMAT, strExtension);
		}

		if (uComplete > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteOldMinMetaDataSearchParam(strDbg, data, FT_COMPLETE_SOURCES, uComplete, bEd2k);
		}

		if (ulMinBitrate > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteOldMinMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_BITRATE : TAG_MEDIA_BITRATE, ulMinBitrate, bEd2k);
		}

		if (ulMinLength > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteOldMinMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_LENGTH : TAG_MEDIA_LENGTH, ulMinLength, bEd2k);
		}

		if (!strCodec.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_CODEC : TAG_MEDIA_CODEC, strCodec);
		}

		if (!strTitle.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_TITLE : TAG_MEDIA_TITLE, strTitle);
		}

		if (!strAlbum.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_ALBUM : TAG_MEDIA_ALBUM, strAlbum);
		}

		if (!strArtist.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
			WriteMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_ARTIST : TAG_MEDIA_ARTIST, strArtist);
		}

		ASSERT( iParameterCount == iTotalTerms );
	}
	else
	{
		if (!strExtension.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}

		if (uAvailability > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}
	  
		if (ulMaxSize > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}
        
		if (ulMinSize > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}
        
		if (!strType.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}
        
		if (uComplete > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}

		if (ulMinBitrate > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}

		if (ulMinLength > 0){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}

		if (!strCodec.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}

		if (!strTitle.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}

		if (!strAlbum.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}

		if (!strArtist.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				WriteBooleanAND(strDbg, data);
		}

		ASSERT( iParameterCount + _SearchExpr.m_aExpr.GetCount() == iTotalTerms );

		for (int j = 0; j < _SearchExpr.m_aExpr.GetCount(); j++){
			CString str(_SearchExpr.m_aExpr[j]);
			if (str == SEARCHOPTOK_AND)
				WriteBooleanAND(strDbg, data);
			else if (str == SEARCHOPTOK_OR)
				WriteBooleanOR(strDbg, data);
			else if (str == SEARCHOPTOK_NOT)
				WriteBooleanNOT(strDbg, data);
			else
				WriteMetaDataSearchParam(strDbg, data, str);
		}

		if (!strType.IsEmpty())
			WriteMetaDataSearchParam(strDbg, data, FT_FILETYPE, strType);

		if (ulMinSize > 0)
			WriteOldMinMetaDataSearchParam(strDbg, data, FT_FILESIZE, ulMinSize, bEd2k);

		if (ulMaxSize > 0)
			WriteOldMaxMetaDataSearchParam(strDbg, data, FT_FILESIZE, ulMaxSize, bEd2k);

		if (uAvailability > 0)
			WriteOldMinMetaDataSearchParam(strDbg, data, FT_SOURCES, uAvailability, bEd2k);

		if (!strExtension.IsEmpty())
			WriteMetaDataSearchParam(strDbg, data, FT_FILEFORMAT, strExtension);

		if (uComplete > 0)
			WriteOldMinMetaDataSearchParam(strDbg, data, FT_COMPLETE_SOURCES, uComplete, bEd2k);

		if (ulMinBitrate > 0)
			WriteOldMinMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_BITRATE : TAG_MEDIA_BITRATE, ulMinBitrate, bEd2k);

		if (ulMinLength > 0)
			WriteOldMinMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_LENGTH : TAG_MEDIA_LENGTH, ulMinLength, bEd2k);

		if (!strCodec.IsEmpty())
			WriteMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_CODEC : TAG_MEDIA_CODEC, strCodec);

		if (!strTitle.IsEmpty())
			WriteMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_TITLE : TAG_MEDIA_TITLE, strTitle);

		if (!strAlbum.IsEmpty())
			WriteMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_ALBUM : TAG_MEDIA_ALBUM, strAlbum);

		if (!strArtist.IsEmpty())
			WriteMetaDataSearchParam(strDbg, data, bEd2k ? FT_ED2K_MEDIA_ARTIST : TAG_MEDIA_ARTIST, strArtist);
	}

	if (thePrefs.GetDebugServerSearchesLevel() > 0)
		Debug("Search Data: %s\n", strDbg);
	_SearchExpr.m_aExpr.RemoveAll();
	return true;
}

bool CSearchDlg::DoNewSearch(SSearchParams* pParams)
{
	if (!theApp.serverconnect->IsConnected())
		return false;

	CSafeMemFile data(100);
	if (!GetSearchPacket(&data, pParams->strExpression, pParams->strFileType, 
						 pParams->ulMinSize, pParams->ulMaxSize, pParams->uAvailability, pParams->strExtension, 
						 pParams->uComplete, pParams->ulMinBitrate, pParams->ulMinLength, pParams->strCodec,
						 "", "", "",
						 false, true) 
		|| data.GetLength() == 0)
		return false;

	CString strResultType = pParams->strFileType;
	if (strResultType == GetResString(IDS_SEARCH_PRG))
		strResultType = GetResString(IDS_SEARCH_ANY);
	m_nSearchID++;
	pParams->dwSearchID = m_nSearchID;
	theApp.searchlist->NewSearch(&searchlistctrl, strResultType, m_nSearchID);
	canceld = false;

	if (m_uTimerLocalServer){
		VERIFY( KillTimer(m_uTimerLocalServer) );
		m_uTimerLocalServer = 0;
	}

	// once we've send a new search request, any previously received 'More' gets invalid.
	CWnd* pWndFocus = GetFocus();
	m_ctlMore.EnableWindow(FALSE);
	if (pWndFocus && pWndFocus->m_hWnd == m_ctlMore.m_hWnd)
		GetDlgItem(IDC_CANCELS)->SetFocus();
	m_iSentMoreReq = 0;

	Packet* packet = new Packet(&data);
	packet->opcode = OP_SEARCHREQUEST;
	if (thePrefs.GetDebugServerTCPLevel() > 0)
		Debug(">>> Sending OP__SearchRequest\n");
	theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
	theApp.serverconnect->SendPacket(packet,false);

	if (pParams->eType == SearchTypeGlobal && theApp.serverconnect->IsUDPSocketAvailable())
	{
		// set timeout timer for local server
		m_uTimerLocalServer = SetTimer(TimerServerTimeout, 50000, NULL);

		if( thePrefs.Score() ){
			theApp.serverlist->ResetSearchServerPos();
		}

		if (globsearch){
			delete searchpacket;
			searchpacket = NULL;
		}
		searchpacket = packet;
		searchpacket->opcode = OP_GLOBSEARCHREQ; // will be changed later when actually sending the packet!!
		servercount = 0;
		searchprogress.SetRange32(0,theApp.serverlist->GetServerCount()-1);
		globsearch = true;
	}
	else{
		globsearch = false;
		delete packet;
	}
	CreateNewTab(pParams);
	return true;
}
	
void CSearchDlg::OnBnClickedMore()
{
	CWnd* pWndFocus = GetFocus();
	m_ctlMore.EnableWindow(FALSE);
	if (pWndFocus && pWndFocus->m_hWnd == m_ctlMore.m_hWnd)
		GetDlgItem(IDC_STARTS)->SetFocus();

	if (!theApp.serverconnect->IsConnected())
		return;

	pWndFocus = GetFocus();
	GetDlgItem(IDC_STARTS)->EnableWindow(false);
	if (pWndFocus == GetDlgItem(IDC_STARTS))
		m_ctlName.SetFocus();
	GetDlgItem(IDC_CANCELS)->EnableWindow(true);

	Packet* packet = new Packet();
	packet->opcode = OP_QUERY_MORE_RESULT;
	if (thePrefs.GetDebugServerTCPLevel() > 0)
		Debug(">>> Sending OP__QueryMoreResults\n");
	theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
	theApp.serverconnect->SendPacket(packet);
	m_iSentMoreReq++;
}

void CSearchDlg::StartNewSearch(bool bKademlia)
{
	ESearchType eSearchType = (ESearchType)methodBox.GetCurSel();
	if (bKademlia ? !Kademlia::CKademlia::isConnected() : ((eSearchType == SearchTypeServer || eSearchType == SearchTypeGlobal) && !theApp.serverconnect->IsConnected()))
	{
		if (!bKademlia)
		AfxMessageBox(GetResString(IDS_ERR_NOTCONNECTED),MB_ICONASTERISK);
		else
			AfxMessageBox(GetResString(IDS_ERR_NOTCONNECTEDKAD), MB_ICONASTERISK);
	}
	else{
		CString strSearchString;
		m_ctlName.GetWindowText(strSearchString);
		strSearchString.Trim();

		CString strSearchType;
		GetDlgItem(IDC_TypeSearch)->GetWindowText(strSearchType);
		//strSearchType.Trim();

		CString strMinSize;
		GetDlgItem(IDC_EDITSEARCHMIN)->GetWindowText(strMinSize);
		ULONG ulMinSize = GetSearchSize(strMinSize);
		if (ulMinSize == (ULONG)-1)
			return;

		CString strMaxSize;
		GetDlgItem(IDC_EDITSEARCHMAX)->GetWindowText(strMaxSize);
		ULONG ulMaxSize = GetSearchSize(strMaxSize);
		if (ulMaxSize == (ULONG)-1)
			return;
		
		if (ulMaxSize < ulMinSize){
			ulMaxSize = 0; // TODO: Create a message box for that
			SetDlgItemText(IDC_EDITSEARCHMAX, _T(""));
		}

		CString strExtension;
		GetDlgItem(IDC_EDITSEARCHEXTENSION)->GetWindowText(strExtension);
		strExtension.Trim();
		if (!strExtension.IsEmpty() && strExtension[0] == _T('.')){
			strExtension = strExtension.Mid(1);
			SetDlgItemText(IDC_EDITSEARCHEXTENSION, strExtension);
		}

		CString strAvailability;
		GetDlgItem(IDC_EDITSEARCHAVAIBILITY)->GetWindowText(strAvailability);
		UINT uAvailability = 0;
		_stscanf(strAvailability, _T("%u"), &uAvailability);

		UINT uComplete = 0;
		if ((m_ctlOpts.GetItemData(orCompleteSources) & 1) == 0){
			CString strComplete = m_ctlOpts.GetItemText(orCompleteSources, 1);
			_stscanf(strComplete, _T("%u"), &uComplete);
		}
	
		CString strCodec;
		if ((m_ctlOpts.GetItemData(orCodec) & 1) == 0)
			strCodec = m_ctlOpts.GetItemText(orCodec, 1);
		strCodec.Trim();

		ULONG ulMinBitrate = 0;
		if ((m_ctlOpts.GetItemData(orBitrate) & 1) == 0){
			CString strMinBitrate = m_ctlOpts.GetItemText(orBitrate, 1);
			_stscanf(strMinBitrate, _T("%u"), &ulMinBitrate);
		}

		ULONG ulMinLength = 0;
		if ((m_ctlOpts.GetItemData(orLength) & 1) == 0){
			CString strMinLength = m_ctlOpts.GetItemText(orLength, 1);
			if (!strMinLength.IsEmpty())
			{
				UINT hour = 0, min = 0, sec = 0;
				if (sscanf(strMinLength, "%u : %u : %u", &hour, &min, &sec) == 3)
					ulMinLength = hour * 3600 + min * 60 + sec;
				else if (sscanf(strMinLength, "%u : %u", &min, &sec) == 2)
					ulMinLength = min * 60 + sec;
				else if (sscanf(strMinLength, "%u", &sec) == 1)
					ulMinLength = sec;
			}
		}

		SSearchParams* pParams = new SSearchParams;
		pParams->strExpression = strSearchString;
		pParams->eType = eSearchType;
		pParams->strFileType = strSearchType;
		pParams->strMinSize = strMinSize;
		pParams->ulMinSize = ulMinSize;
		pParams->strMaxSize = strMaxSize;
		pParams->ulMaxSize = ulMaxSize;
		pParams->uAvailability = uAvailability;
		pParams->strExtension = strExtension;
		pParams->bMatchKeywords = IsDlgButtonChecked(IDC_MATCH_KEYWORDS);
		pParams->uComplete = uComplete;
		pParams->strCodec = strCodec;
		pParams->ulMinBitrate = ulMinBitrate;
		pParams->ulMinLength = ulMinLength;
		if ((m_ctlOpts.GetItemData(orTitle) & 1) == 0){
			pParams->strTitle = m_ctlOpts.GetItemText(orTitle, 1);
			pParams->strTitle.Trim();
		}
		if ((m_ctlOpts.GetItemData(orAlbum) & 1) == 0){
			pParams->strAlbum = m_ctlOpts.GetItemText(orAlbum, 1);
			pParams->strAlbum.Trim();
		}
		if ((m_ctlOpts.GetItemData(orArtist) & 1) == 0){
			pParams->strArtist = m_ctlOpts.GetItemText(orArtist, 1);
			pParams->strArtist.Trim();
		}

		if (!bKademlia)
		{
			bool bSearchStarted = false;
		    if (eSearchType == SearchTypeServer || eSearchType == SearchTypeGlobal)
		    {
				bSearchStarted = DoNewSearch(pParams);
		        if (!bSearchStarted)
				    delete pParams;
		    }
		    else if (eSearchType == SearchTypeJigleSOAP)
		    {
				bSearchStarted = DoNewJigleSearch(pParams);
			    if (!bSearchStarted)
				    delete pParams;
		    }
		    else{
			    ASSERT(0);
			    delete pParams;
		    }

			if (bSearchStarted)
			{
				CWnd* pWndFocus = GetFocus();
				GetDlgItem(IDC_STARTS)->EnableWindow(false);
				if (pWndFocus == GetDlgItem(IDC_STARTS))
					m_ctlName.SetFocus();
				GetDlgItem(IDC_CANCELS)->EnableWindow(true);
			}
		}
		else
		{
			if (!DoNewKadSearch(pParams))
				delete pParams;
		}
	}
}

bool CSearchDlg::DoNewKadSearch(SSearchParams* pParams)
{
	if (!Kademlia::CKademlia::isRunning())
		return false;

	if (!Kademlia::CKademlia::isConnected())
		return false;

	int iPos = 0;
	CString strKeyWord = pParams->strExpression.Tokenize(_T(" "), iPos);
	strKeyWord.Trim();
	if (strKeyWord.IsEmpty() || strKeyWord.FindOneOf(_aszInvKadKeywordChars) != -1){
		CString strError;
		strError.Format(GetResString(IDS_KAD_SEARCH_KEYWORD_INVALID), _aszInvKadKeywordChars);
		AfxMessageBox(strError);
		return false;
	}
	CString strSearchExpr = (iPos >= pParams->strExpression.GetLength()) ? _T("") : pParams->strExpression.Mid(iPos);

	CSafeMemFile data(100);
	if (!GetSearchPacket(&data, strSearchExpr, 
						 pParams->strFileType, pParams->ulMinSize, pParams->ulMaxSize, pParams->uAvailability, 
						 pParams->strExtension, 0/*pParams->uComplete*/, pParams->ulMinBitrate, pParams->ulMinLength,
						 pParams->strCodec, pParams->strTitle, pParams->strAlbum, pParams->strArtist,
						 true, false) 
		|| (!strSearchExpr.IsEmpty() && data.GetLength() == 0))
		return false;

	LPBYTE pSearchTermsData = NULL;
	UINT uSearchTermsSize = data.GetLength();
	if (uSearchTermsSize){
		pSearchTermsData = new BYTE[uSearchTermsSize];
		data.SeekToBegin();
		data.Read(pSearchTermsData, uSearchTermsSize);
	}

	Kademlia::CSearch* pSearch = NULL;
	try
	{
		pSearch = Kademlia::CSearchManager::prepareFindKeywords(Kademlia::CSearch::KEYWORD, true, strKeyWord, uSearchTermsSize, pSearchTermsData);
	delete pSearchTermsData;
		if (!pSearch){
			ASSERT(0);
			return false;
		}
	}
	catch (CString strException)
	{
		AfxMessageBox(strException);
		delete pSearchTermsData;
		return false;
	}
	pParams->dwSearchID = pSearch->getSearchID();
	CString strResultType = pParams->strFileType;
	if (strResultType == GetResString(IDS_SEARCH_PRG))
		strResultType = GetResString(IDS_SEARCH_ANY);
	theApp.searchlist->NewSearch(&searchlistctrl,strResultType,pParams->dwSearchID);
	CreateNewTab(pParams);
	return true;
}

void CSearchDlg::StartNewSearchKad(){
	StartNewSearch(true);
}

bool CSearchDlg::CreateNewTab(SSearchParams* pParams)
{
    int iTabItems = searchselect.GetItemCount();
    for (int i = 0; i < iTabItems; i++)
	{
        TCITEM tci;
        tci.mask = TCIF_PARAM;
		if (searchselect.GetItem(i, &tci) && tci.lParam != NULL && ((const SSearchParams*)tci.lParam)->dwSearchID == pParams->dwSearchID)
			return false;
    }
	// add new tab
	// add new tab
	TCITEM newitem;
	if (pParams->strExpression.IsEmpty())
		pParams->strExpression = _T("-");
	newitem.mask = TCIF_PARAM|TCIF_TEXT|TCIF_IMAGE;
	newitem.lParam = (LPARAM)pParams;
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)pParams->strExpression);
	newitem.cchTextMax = 0;
	if (pParams->bClientSharedFiles)
		newitem.iImage = 4;
	else if (pParams->eType == SearchTypeGlobal)
		newitem.iImage = 1;
	else if (pParams->eType == SearchTypeJigleSOAP)
		newitem.iImage = 2;
	else if (pParams->eType == SearchTypeKademlia)
		newitem.iImage = 3;
	else{
		ASSERT( pParams->eType == SearchTypeServer );
	newitem.iImage = 0;
	}
	int itemnr = searchselect.InsertItem(INT_MAX, &newitem);
	if (!searchselect.IsWindowVisible())
		ShowSearchSelector(true);
	searchselect.SetCurSel(itemnr);
	searchlistctrl.ShowResults(pParams->dwSearchID);
	return true;
}

bool CSearchDlg::CanDeleteSearch(uint32 nSearchID) const
{
	return (searchselect.GetItemCount() > 0);
}

void CSearchDlg::DeleteSearch(uint32 nSearchID)
{
	Kademlia::CSearchManager::stopSearch(nSearchID, false);
	TCITEM item;
	item.mask = TCIF_PARAM;
	item.lParam = -1;
	int i;
	for (i = 0; i != searchselect.GetItemCount();i++){
		if (searchselect.GetItem(i, &item) && item.lParam != -1 && item.lParam != NULL && ((const SSearchParams*)item.lParam)->dwSearchID == nSearchID)
			break;
	}
	if (item.lParam == -1 || item.lParam == NULL || ((const SSearchParams*)item.lParam)->dwSearchID != nSearchID)
		return;

	// delete search results
	if (!canceld && nSearchID == m_nSearchID)
		CancelSearch();
	theApp.searchlist->RemoveResults(nSearchID);

	// delete search tab
	int iCurSel = searchselect.GetCurSel();
	searchselect.DeleteItem(i);
	delete (SSearchParams*)item.lParam;

	int iTabItems = searchselect.GetItemCount();
	if (iTabItems > 0){
		// select next search tab
		if (iCurSel == CB_ERR)
			iCurSel = 0;
		else if (iCurSel >= iTabItems)
			iCurSel = iTabItems - 1;
		(void)searchselect.SetCurSel(iCurSel);	// returns CB_ERR if error or no prev. selection(!)
		iCurSel = searchselect.GetCurSel();		// get the real current selection
		if (iCurSel == CB_ERR)					// if still error
			iCurSel = searchselect.SetCurSel(0);
		if (iCurSel != CB_ERR){
			item.mask = TCIF_PARAM;
			item.lParam = NULL;
			if (searchselect.GetItem(iCurSel, &item) && item.lParam != NULL)
				ShowResults((const SSearchParams*)item.lParam);
		}
	}
	else{
		searchlistctrl.DeleteAllItems();
		ShowSearchSelector(false);
		searchlistctrl.NoTabs();
	}
}

bool CSearchDlg::CanDeleteAllSearches() const
{
	return (searchselect.GetItemCount() > 0);
}

void CSearchDlg::DeleteAllSearchs()
{
	for (int i = 0; i < searchselect.GetItemCount(); i++){
		TCITEM item;
		item.mask = TCIF_PARAM;
		item.lParam = -1;
		if (searchselect.GetItem(i, &item) && item.lParam != -1 && item.lParam != NULL){
			Kademlia::CSearchManager::stopSearch(((const SSearchParams*)item.lParam)->dwSearchID, false);
			delete (SSearchParams*)item.lParam;
		}
	}
	theApp.searchlist->Clear();
	searchlistctrl.DeleteAllItems();
	ShowSearchSelector(false);
	searchselect.DeleteAllItems();
}

void CSearchDlg::ShowResults(const SSearchParams* pParams)
{
	if (!pParams->bClientSharedFiles)
	{
		m_ctlName.SetWindowText(pParams->strExpression);
		SetDlgItemText(IDC_EDITSEARCHMIN, pParams->strMinSize);
		SetDlgItemText(IDC_EDITSEARCHMAX, pParams->strMaxSize);
		SetDlgItemText(IDC_EDITSEARCHEXTENSION, pParams->strExtension);

		if (pParams->uAvailability > 0)
			SetDlgItemInt(IDC_EDITSEARCHAVAIBILITY, pParams->uAvailability, FALSE);

		CString strBuff;
		if (pParams->uComplete > 0)
			strBuff.Format(_T("%u"), pParams->uComplete);
		else
			strBuff.Empty();
		m_ctlOpts.SetItemText(orCompleteSources, 1, strBuff);

		m_ctlOpts.SetItemText(orCodec, 1, pParams->strCodec);

		if (pParams->ulMinBitrate > 0)
			strBuff.Format(_T("%u"), pParams->ulMinBitrate);
		else
			strBuff.Empty();
		m_ctlOpts.SetItemText(orBitrate, 1, strBuff);

		if (pParams->ulMinLength > 0)
			SecToTimeLength(pParams->ulMinLength, strBuff);
		else
			strBuff.Empty();
		m_ctlOpts.SetItemText(orLength, 1, strBuff);
		m_ctlOpts.SetItemText(orTitle, 1, pParams->strTitle);
		m_ctlOpts.SetItemText(orAlbum, 1, pParams->strAlbum);
		m_ctlOpts.SetItemText(orArtist, 1, pParams->strArtist);
	}
	searchlistctrl.ShowResults(pParams->dwSearchID);
}

void CSearchDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	CWaitCursor curWait; // this may take a while
	int cur_sel = searchselect.GetCurSel();
	if (cur_sel == -1)
		return;
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (searchselect.GetItem(cur_sel, &item) && item.lParam != NULL)
		ShowResults((const SSearchParams*)item.lParam);
	*pResult = 0;
}

LRESULT CSearchDlg::OnCloseTab(WPARAM wparam, LPARAM lparam)
{
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (searchselect.GetItem((int)wparam, &item) && item.lParam != NULL)
	{
		int nSearchID = ((const SSearchParams*)item.lParam)->dwSearchID;
		if (!canceld && nSearchID == m_nSearchID)
			CancelSearch();
		DeleteSearch(nSearchID);
	}
	return TRUE;
}

void CSearchDlg::OnEnChangeSearchname()
{
	GetDlgItem(IDC_STARTS)->EnableWindow(true);
}

void CSearchDlg::OnBnClickedSearchReset()
{
	GetDlgItem(IDC_EDITSEARCHAVAIBILITY)->SetWindowText(_T(""));
	GetDlgItem(IDC_EDITSEARCHEXTENSION)->SetWindowText(_T(""));
	GetDlgItem(IDC_EDITSEARCHMIN)->SetWindowText(_T(""));
	GetDlgItem(IDC_EDITSEARCHMAX)->SetWindowText(_T(""));
	m_ctlName.SetWindowText(_T(""));
	Stypebox.SetCurSel(Stypebox.FindString(-1,GetResString(IDS_SEARCH_ANY)));

	for (int i = 0; i < m_ctlOpts.GetItemCount(); i++)
		m_ctlOpts.SetItemText(i, 1, _T(""));

	OnEnChangeSearchname();
}

// khaos::categorymod+ These things are obsolete, replaced by the select cat dialog.
/*
void CSearchDlg::UpdateCatTabs() {
	int oldsel=m_cattabs.GetCurSel();
	m_cattabs.DeleteAllItems();
	for (int ix=0;ix<thePrefs.GetCatCount();ix++)
		m_cattabs.InsertItem(ix,(ix==0)?GetResString(IDS_ALL):thePrefs.GetCategory(ix)->title);
	if (oldsel>=m_cattabs.GetItemCount() || oldsel==-1)
		oldsel=0;

	m_cattabs.SetCurSel(oldsel);
	int flag;
	flag=(m_cattabs.GetItemCount()>1) ? SW_SHOW:SW_HIDE;
	
	GetDlgItem(IDC_CATTAB2)->ShowWindow(flag);
	GetDlgItem(IDC_STATIC_DLTOof)->ShowWindow(flag);
}
*/
// khaos::categorymod-

CString	CSearchDlg::ToQueryString(CString str){
	CString sTmp = URLEncode(str);
	sTmp.Replace("%20","+");
	return sTmp;
}

void CSearchDlg::ShowSearchSelector(bool visible)
{
	WINDOWPLACEMENT wpSearchWinPos;
	WINDOWPLACEMENT wpSelectWinPos;
	searchselect.GetWindowPlacement(&wpSelectWinPos);
	searchlistctrl.GetWindowPlacement(&wpSearchWinPos);
	if (visible)
		wpSearchWinPos.rcNormalPosition.top = wpSelectWinPos.rcNormalPosition.bottom;
	else
		wpSearchWinPos.rcNormalPosition.top = wpSelectWinPos.rcNormalPosition.top;
	searchselect.ShowWindow(visible ? SW_SHOW : SW_HIDE);
	RemoveAnchor(searchlistctrl);
	searchlistctrl.SetWindowPlacement(&wpSearchWinPos);
	AddAnchor(searchlistctrl,TOP_LEFT,BOTTOM_RIGHT);
	GetDlgItem(IDC_CLEARALL)->ShowWindow(visible ? SW_SHOW : SW_HIDE);
}

BOOL CSearchDlg::SaveSearchStrings()
{
	if (m_pacSearchString == NULL)
		return FALSE;
	return m_pacSearchString->SaveList(CString(thePrefs.GetConfigDir()) + _T("\\") SEARCH_STRINGS_PROFILE);
}

void CSearchDlg::SaveAllSettings()
{
	searchlistctrl.SaveSettings(CPreferences::tableSearch);
	SaveSearchStrings();
}

void CSearchDlg::OnDestroy()
{
	if (m_pJigleThread)
		m_pJigleThread->PostThreadMessage(WM_QUIT, 0, 0);

	int iTabItems = searchselect.GetItemCount();
	for (int i = 0; i < iTabItems; i++){
		TCITEM tci;
		tci.mask = TCIF_PARAM;
		if (searchselect.GetItem(i, &tci) && tci.lParam != NULL){
			delete (SSearchParams*)tci.lParam;
		}
	}

	CResizableDialog::OnDestroy();
	m_imlSearchMethods.DeleteImageList();
}

void CSearchDlg::UpdateControls()
{
	int iMethod = methodBox.GetCurSel();
	if (iMethod != CB_ERR)
		thePrefs.SetSearchMethod(iMethod);
	GetDlgItem(IDC_MATCH_KEYWORDS)->EnableWindow(iMethod == SearchTypeJigleSOAP || iMethod == SearchTypeJigle);
	GetDlgItem(IDC_MATCH_KEYWORDS)->ShowWindow((iMethod == SearchTypeJigleSOAP || iMethod == SearchTypeJigle) ? SW_SHOW : SW_HIDE);

	m_ctlOpts.SetItemData(orCompleteSources, (iMethod==SearchTypeKademlia || iMethod==SearchTypeJigleSOAP || iMethod==SearchTypeJigle || iMethod==SearchTypeFileDonkey) ? 1 : 0);
	m_ctlOpts.SetItemData(orCodec, (iMethod==SearchTypeJigleSOAP || iMethod==SearchTypeJigle || iMethod==SearchTypeFileDonkey) ? 1 : 0);
	m_ctlOpts.SetItemData(orBitrate, (iMethod==SearchTypeJigleSOAP || iMethod==SearchTypeJigle || iMethod==SearchTypeFileDonkey) ? 1 : 0);
	m_ctlOpts.SetItemData(orLength, (iMethod==SearchTypeJigleSOAP || iMethod==SearchTypeJigle || iMethod==SearchTypeFileDonkey) ? 1 : 0);
	m_ctlOpts.SetItemData(orTitle, (iMethod==SearchTypeServer || iMethod==SearchTypeGlobal || iMethod==SearchTypeJigleSOAP || iMethod==SearchTypeJigle || iMethod==SearchTypeFileDonkey) ? 1 : 0);
	m_ctlOpts.SetItemData(orAlbum, (iMethod==SearchTypeServer || iMethod==SearchTypeGlobal || iMethod==SearchTypeJigleSOAP || iMethod==SearchTypeJigle || iMethod==SearchTypeFileDonkey) ? 1 : 0);
	m_ctlOpts.SetItemData(orArtist, (iMethod==SearchTypeServer || iMethod==SearchTypeGlobal || iMethod==SearchTypeJigleSOAP || iMethod==SearchTypeJigle || iMethod==SearchTypeFileDonkey) ? 1 : 0);
}

void CSearchDlg::OnCbnSelchangeCombo1()
{
	UpdateControls();
}

void CSearchDlg::OnCbnSelendokCombo1()
{
	UpdateControls();
}

void CSearchDlg::OnDDClicked() {
	
	CWnd* box=GetDlgItem(IDC_SEARCHNAME);
	box->SetFocus();
	box->SetWindowText("");
	box->SendMessage(WM_KEYDOWN,VK_DOWN,0x00510001);
	
}

void CSearchDlg::OnSize(UINT nType, int cx, int cy)
{
	CResizableDialog::OnSize(nType, cx, cy);
	if (m_ctlOpts.m_hWnd)
	{
		CRect rcClnt;
		m_ctlOpts.GetClientRect(&rcClnt);
		int iCol1Width = rcClnt.Width() - m_ctlOpts.GetColumnWidth(0) /*- GetSystemMetrics(SM_CXVSCROLL)*/;
		m_ctlOpts.SetColumnWidth(1, iCol1Width);
	}
}

void CSearchDlg::ResetHistory()
{
	if (m_pacSearchString==NULL) 
		return;

	CWnd* box=GetDlgItem(IDC_SEARCHNAME);
	box->SetFocus();
	box->SendMessage(WM_KEYDOWN,VK_ESCAPE ,0x00510001);

	m_pacSearchString->Clear(); 
}

void CSearchDlg::OnClose()
{
	// Do not pass the WM_CLOSE to the base class. Since we have a rich edit control *and* an attached auto complete
	// control, the WM_CLOSE will get generated by the rich edit control when user presses ESC while the auto complete
	// is open.
	//__super::OnClose();
}
