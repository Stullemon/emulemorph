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


// SearchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "SearchDlg.h"
#include "packets.h"
#include "server.h"
#include "opcodes.h"
#include "otherfunctions.h"
#include "ED2KLink.h"
#include "SearchList.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/routing/Timer.h"
#include "kademlia/kademlia/search.h"
#include "CustomAutoComplete.h"
#include "SearchExpr.h"
#define USE_FLEX
#include "Parser.hpp"
#include "Scanner.h"
#include "Jigle/JigleSearch.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define	SEARCH_STRINGS_PROFILE	_T("AC_SearchStrings.dat")

extern int yyparse();
extern int yyerror(const char* errstr);

enum ESearchTimerID
{
	TimerServerTimeout = 1,
	TimerGlobalSearch
};

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
	m_guardCBPrompt = false;
	icon_search = NULL;
	m_uTimerLocalServer = 0;
	m_uLangID = MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT);
	m_iSentMoreReq = 0;
}

CSearchDlg::~CSearchDlg(){
	if (globsearch) delete searchpacket;
	DestroyIcon(icon_search);
	if (m_pacSearchString){
		m_pacSearchString->Unbind();
		m_pacSearchString->Release();
	}
	if (m_uTimerLocalServer)
		VERIFY( KillTimer(m_uTimerLocalServer) );
}

BOOL CSearchDlg::OnInitDialog(){
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);
	theApp.searchlist->SetOutputWnd(&searchlistctrl);
	searchlistctrl.Init(theApp.searchlist);
	m_imlSearchMethods.Create(13,13,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,10);
	m_imlSearchMethods.SetBkColor(CLR_NONE);
	m_imlSearchMethods.Add(CTempIconLoader(IDI_SCH_SERVER, 13, 13));
	m_imlSearchMethods.Add(CTempIconLoader(IDI_SCH_GLOBAL, 13, 13));
	m_imlSearchMethods.Add(CTempIconLoader(IDI_SCH_JIGLE, 13, 13));
	m_imlSearchMethods.Add(CTempIconLoader(IDI_SCH_FILEDONKEY, 13, 13));
	m_imlSearchMethods.Add(CTempIconLoader(IDI_SCH_KADEMLIA, 13, 13));
	Localize();
	searchprogress.SetStep(1);
	global_search_timer = 0;
	globsearch = false;
	m_guardCBPrompt=false;

	m_ctrlSearchFrm.Init(IDI_NORMALSEARCH);
	m_ctrlDirectDlFrm.Init(IDI_DIRECTDOWNLOAD);

	icon_search=(HICON)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_SEARCHRESULTS), IMAGE_ICON, 16, 16, 0);
	((CStatic*)GetDlgItem(IDC_SEARCHLST_ICO))->SetIcon(icon_search);
	
	m_ImageList.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,10);
	m_ImageList.SetBkColor(CLR_NONE);
	m_ImageList.Add(theApp.LoadIcon(IDI_BN_SEARCH));
	searchselect.SetImageList(&m_ImageList);

	AddAnchor(IDC_SDOWNLOAD,BOTTOM_LEFT);
	AddAnchor(IDC_SEARCHLIST,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS1,BOTTOM_LEFT,BOTTOM_RIGHT);

	AddAnchor(IDC_DDOWN_FRM, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_ELINK, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STARTS, TOP_RIGHT);
	AddAnchor(IDC_MORE, TOP_RIGHT);
	AddAnchor(IDC_CANCELS, TOP_RIGHT);
	AddAnchor(IDC_CLEARALL, TOP_RIGHT);
	AddAnchor(IDC_SEARCH_RESET, TOP_LEFT);
	AddAnchor(searchselect.m_hWnd,TOP_LEFT,TOP_RIGHT);
	// khaos::categorymod+ obsolete //AddAnchor(IDC_STATIC_DLTOof,BOTTOM_LEFT);
	// khaos::categorymod+ obsolete //AddAnchor(IDC_CATTAB2,BOTTOM_LEFT);
	ShowSearchSelector(false);
	m_lastclpbrd="";

	if (theApp.glob_prefs->GetUseAutocompletion()){
		m_pacSearchString = new CCustomAutoComplete();
		m_pacSearchString->AddRef();
		if (m_pacSearchString->Bind(::GetDlgItem(m_hWnd, IDC_SEARCHNAME), ACO_UPDOWNKEYDROPSLIST | ACO_AUTOSUGGEST))
			m_pacSearchString->LoadList(CString(theApp.glob_prefs->GetConfigDir()) +  _T("\\") SEARCH_STRINGS_PROFILE);
	}

	ASSERT( (GetDlgItem(IDC_EDITSEARCHMIN)->GetStyle() & ES_NUMBER) == 0 );
	ASSERT( (GetDlgItem(IDC_EDITSEARCHMAX)->GetStyle() & ES_NUMBER) == 0 );
	
	((CEdit*)GetDlgItem(IDC_SEARCHNAME))->LimitText(512); // max. length of search expression
	((CEdit*)GetDlgItem(IDC_EDITSEARCHEXTENSION))->LimitText(8); // max. length of file (type) extension

	if (methodBox.SetCurSel(theApp.glob_prefs->GetSearchMethod()) == CB_ERR)
		methodBox.SetCurSel(SearchTypeServer);

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
	DDX_Control(pDX, IDC_DDOWN_FRM, m_ctrlDirectDlFrm);
	// khaos::categorymod+ obsolete //DDX_Control(pDX, IDC_CATTAB2, m_cattabs);
	DDX_Control(pDX, IDC_MORE, m_ctlMore);
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
	ON_EN_KILLFOCUS(IDC_ELINK, OnEnKillfocusElink)
	ON_WM_DESTROY()
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelchangeCombo1)
	ON_CBN_SELENDOK(IDC_COMBO1, OnCbnSelendokCombo1)
	ON_BN_CLICKED(IDC_MORE, OnBnClickedMore)
END_MESSAGE_MAP()


// CSearchDlg message handlers

void CSearchDlg::OnBnClickedStarts()
{
	searchprogress.SetPos(0);

	// ed2k-links
	if (GetDlgItem(IDC_ELINK)->GetWindowTextLength()){
		CString strlink;
		GetDlgItem(IDC_ELINK)->GetWindowText(strlink);
		GetDlgItem(IDC_ELINK)->SetWindowText("");

		AddEd2kLinksToDownload(strlink);
		return;
	}

	// start "normal" server-search
	if (GetDlgItem(IDC_SEARCHNAME)->GetWindowTextLength())
	{
		if (m_pacSearchString && m_pacSearchString->IsBound()){
			CString strSearch;
			if (GetDlgItemText(IDC_SEARCHNAME, strSearch))
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
		default:
			return;
		}
	}
}

void CSearchDlg::OnTimer(UINT nIDEvent)
{
	CResizableDialog::OnTimer(nIDEvent);

	if (m_uTimerLocalServer != 0 && nIDEvent == m_uTimerLocalServer)
	{
		if (theApp.glob_prefs->GetDebugServerSearches())
			Debug("Timeout waiting on search results of local server\n");
		// the local server did not answer within the timeout
		VERIFY( KillTimer(m_uTimerLocalServer) );
		m_uTimerLocalServer = 0;

		// start the global search
		if (globsearch)
		{
			if (global_search_timer == 0)
				VERIFY( (global_search_timer = SetTimer(TimerGlobalSearch, 750, 0)) );
		}
		else
			OnBnClickedCancels();
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
				if (theApp.glob_prefs->GetDebugServerUDP())
					Debug(">>> Sending %s  to server %s:%u (%u of %u)\n", (searchpacket->opcode == OP_GLOBSEARCHREQ2) ? "OP__GlobSearchReq2" : "OP__GlobSearchReq", toask->GetAddress(), toask->GetPort(), servercount, theApp.serverlist->GetServerCount());
			theApp.serverconnect->SendUDPPacket(searchpacket,toask,false);
			searchprogress.StepIt();
		}
		else
			OnBnClickedCancels();
	}
	else
		OnBnClickedCancels();
   }
	else
		ASSERT( 0 );
}

void CSearchDlg::OnBnClickedCancels()
{
	canceld = true;
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
		GetDlgItem(IDC_SEARCHNAME)->SetFocus();
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
		OnBnClickedCancels();
	if (!canceld){	
		if (!globsearch){
			GetDlgItem(IDC_STARTS)->EnableWindow(true);
			CWnd* pWndFocus = GetFocus();
			GetDlgItem(IDC_CANCELS)->EnableWindow(false);
			if (pWndFocus == GetDlgItem(IDC_CANCELS))
				GetDlgItem(IDC_SEARCHNAME)->SetFocus();
		}
		else{
			VERIFY( (global_search_timer = SetTimer(TimerGlobalSearch, 750, 0)) );
		}
	}
	m_ctlMore.EnableWindow(bMoreResultsAvailable && m_iSentMoreReq < MAX_MORE_SEARCH_REQ);
}
void CSearchDlg::AddUDPResult(uint16 count){
	if (!canceld && count > MAX_RESULTS)
		OnBnClickedCancels();
}

void CSearchDlg::OnBnClickedSdownload(){
	//start download(s)
	DownloadSelected();
}

BOOL CSearchDlg::PreTranslateMessage(MSG* pMsg) 
{
	if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_ESCAPE))
	   	return FALSE;

	if( m_pacSearchString && m_pacSearchString->IsBound() && ((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_DELETE) && (pMsg->hwnd == GetDlgItem(IDC_SEARCHNAME)->m_hWnd) && (GetAsyncKeyState(VK_MENU)<0 || GetAsyncKeyState(VK_CONTROL)<0)) )
		m_pacSearchString->Clear();

   	if((pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_RETURN)){
	   	if (pMsg->hwnd == GetDlgItem(IDC_SEARCHLIST)->m_hWnd)
			OnBnClickedSdownload();
	   	else if (pMsg->hwnd == GetDlgItem(IDC_SEARCHNAME)->m_hWnd || 
				 pMsg->hwnd == GetDlgItem(IDC_TypeSearch)->m_hWnd || 
				 pMsg->hwnd == GetDlgItem(IDC_EDITSEARCHMIN)->m_hWnd || 
				 pMsg->hwnd == GetDlgItem(IDC_EDITSEARCHMAX)->m_hWnd || 
				 pMsg->hwnd == GetDlgItem(IDC_EDITSEARCHAVAIBILITY)->m_hWnd || 
				 pMsg->hwnd == GetDlgItem(IDC_COMBO1)->m_hWnd || 
				 pMsg->hwnd == ((CComboBoxEx2*)GetDlgItem(IDC_COMBO1))->GetComboBoxCtrl()->GetSafeHwnd() ||
				 pMsg->hwnd == GetDlgItem(IDC_EDITSEARCHEXTENSION)->m_hWnd){
			if (m_pacSearchString && m_pacSearchString->IsBound() && pMsg->hwnd == GetDlgItem(IDC_SEARCHNAME)->m_hWnd){
				CString strText;
				GetDlgItem(IDC_SEARCHNAME)->GetWindowText(strText);
				if (!strText.IsEmpty()){
					GetDlgItem(IDC_SEARCHNAME)->SetWindowText(_T("")); // this seems to be the only chance to let the dropdown list to disapear
					GetDlgItem(IDC_SEARCHNAME)->SetWindowText(strText);
					((CEdit*)GetDlgItem(IDC_SEARCHNAME))->SetSel(strText.GetLength(), strText.GetLength());
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
				return -1;
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
		GetDlgItem(IDC_SEARCHNAME)->GetWindowText(tosearch);
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
		GetDlgItem(IDC_SEARCHNAME)->GetWindowText(tosearch);
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


void CSearchDlg::DownloadSelected() {
	int index = -1; 
	CSearchFile* cur_file=NULL;

	POSITION pos = searchlistctrl.GetFirstSelectedItemPosition();
	
	// khaos::categorymod+ Category selection stuff...
	if (!pos) return; // No point in asking for a category if there are no selected files to download.

	int useCat;

	if(theApp.glob_prefs->SelectCatForNewDL())
	{
		CSelCategoryDlg getCatDlg;
		getCatDlg.DoModal();

		// Returns 0 on 'Cancel', otherwise it returns the selected category
		// or the index of a newly created category.  Users can opt to add the
		// links into a new category.
		useCat = getCatDlg.GetInput();
	}
	else if(theApp.glob_prefs->UseActiveCatForLinks())
		useCat = theApp.emuledlg->transferwnd.GetActiveCategory();
	else
		useCat = 0;

	int useOrder = theApp.downloadqueue->GetMaxCatResumeOrder(useCat);
	// khaos::categorymod-

	while(pos != NULL) 
	{ 
		index = searchlistctrl.GetNextSelectedItem(pos);

		if (index > -1) {
			cur_file=(CSearchFile*)searchlistctrl.GetItemData(index);
			// khaos::categorymod+ m_cattabs is obsolete.
			if (!theApp.glob_prefs->SelectCatForNewDL() && theApp.glob_prefs->UseAutoCat())
			{
				useCat = theApp.downloadqueue->GetAutoCat(CString(cur_file->GetFileName()), (ULONG)cur_file->GetFileSize());
				if (!useCat && theApp.glob_prefs->UseActiveCatForLinks())
					useCat = theApp.emuledlg->transferwnd.GetActiveCategory();
			}

			if (theApp.glob_prefs->SmallFileDLPush() && cur_file->GetFileSize() < 154624)
				theApp.downloadqueue->AddSearchToDownload(cur_file, useCat, 0);
			else
			{
				useOrder++;
				theApp.downloadqueue->AddSearchToDownload(cur_file, useCat, useOrder);
			}
			// khaos::categorymod-
			if (cur_file->GetListParent()!=NULL) cur_file=cur_file->GetListParent();
			searchlistctrl.UpdateSources(cur_file);
		}
	} 
}


void CSearchDlg::Localize(){
	searchlistctrl.Localize();
	//MORPH Removed by SiRoB, Due to Khaos Category
	/*UpdateCatTabs();*/

	if (theApp.glob_prefs->GetLanguageID() != m_uLangID){
		m_uLangID = theApp.glob_prefs->GetLanguageID();
		m_ctrlSearchFrm.SetWindowText(GetResString(IDS_SW_SEARCHBOX));
		m_ctrlSearchFrm.SetText(GetResString(IDS_SW_SEARCHBOX));
		m_ctrlDirectDlFrm.SetWindowText(GetResString(IDS_SW_DIRECTDOWNLOAD));
		m_ctrlDirectDlFrm.SetText(GetResString(IDS_SW_DIRECTDOWNLOAD));

		GetDlgItem(IDC_MSTATIC3)->SetWindowText(GetResString(IDS_SW_NAME));
		GetDlgItem(IDC_MSTATIC7)->SetWindowText(GetResString(IDS_SW_TYPE));
		GetDlgItem(IDC_STATIC_FILTER)->SetWindowText(GetResString(IDS_FILTER));
		GetDlgItem(IIDC_FSTATIC2)->SetWindowText(GetResString(IDS_SW_LINK));
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
	}

	int iMethod = methodBox.GetCurSel();
	methodBox.ResetContent();
	methodBox.SetImageList(&m_imlSearchMethods);
	VERIFY( methodBox.AddItem(GetResString(IDS_SERVER), 0) == SearchTypeServer );
	VERIFY( methodBox.AddItem(GetResString(IDS_GLOBALSEARCH), 1) == SearchTypeGlobal );
	VERIFY( methodBox.AddItem(_T("Kademlia"), 4) == SearchTypeKademlia );
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
	OnBnClickedCancels();
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
	if (theApp.glob_prefs->GetDebugServerSearches())
		Debug("Search Expr: %s\n", strDbg);

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
	if (theApp.glob_prefs->GetDebugServerSearches()){
		_strSearchTree.Empty();
	DumpSearchTree(_SearchExpr);
		Debug("Search Tree: %s\n", _strSearchTree);
	}
#endif
}

bool GetSearchPacket(CSafeMemFile& data,
					 const CString& strSearchString, const CString& strLocalizedType,
					 ULONG ulMinSize, ULONG ulMaxSize, int iAvailability, 
					 const CString& strExtension, bool bAllowEmptySearchString)
		{
	static const uchar _aucSearchType[3]         = {       0x01, 0x00, FT_FILETYPE	 };
	static const uchar _aucSearchExtension[3]    = {       0x01, 0x00, FT_FILEFORMAT };
	static const uchar _aucSearchAvailability[4] = { 0x01, 0x01, 0x00, FT_SOURCES    };
	static const uchar _aucSearchMin[4]			 = { 0x01, 0x01, 0x00, FT_FILESIZE   };
	static const uchar _aucSearchMax[4]			 = { 0x02, 0x01, 0x00, FT_FILESIZE   };

	static const byte stringParameter  = 1;
	static const byte typeParameter    = 2;
	static const byte numericParameter = 3;

	static const uint16 andParameter = 0x0000;	// 0x00, 0x00
	static const uint16 orParameter  = 0x0100;	// 0x00, 0x01
	static const uint16 notParameter = 0x0200;	// 0x00, 0x02

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
	// Because the availability is used with the 'Min' operator, we decrement the entered value by one to get
	// more 'useable' results from the server (from a user's POV).
	if (iAvailability >= 0)
		iTotalTerms++;
	if (ulMaxSize > 0)
		iTotalTerms++;
	if (ulMinSize > 0)
		iTotalTerms++;
	if (!strType.IsEmpty())
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
			if (++iParameterCount < iTotalTerms){
				data.Write(&andParameter, 2);
				strDbg.AppendFormat("AND ");
			}
			data.Write(&stringParameter, 1);
			uint16 nSize = _SearchExpr.m_aExpr[0].GetLength();
			data.Write(&nSize, 2);
			data.Write((LPCSTR)_SearchExpr.m_aExpr[0], nSize);
			strDbg.AppendFormat("\"%s\" ", _SearchExpr.m_aExpr[0]);
		}

		if (!strType.IsEmpty()){
			if (++iParameterCount < iTotalTerms){
				data.Write(&andParameter, 2);
				strDbg.AppendFormat("AND ");
			}
			data.Write(&typeParameter, 1);
			uint16 nSize = strType.GetLength();
			data.Write(&nSize, 2);
			data.Write((LPCSTR)strType, nSize);
			data.Write(_aucSearchType, sizeof _aucSearchType);
			strDbg.AppendFormat("type=\"%s\" ", strType);
		}
	  
		if (ulMinSize > 0){
			if (++iParameterCount < iTotalTerms){
				data.Write(&andParameter, 2);
				strDbg.AppendFormat("AND ");
			}
			data.Write(&numericParameter, 1);
			data.Write(&ulMinSize, 4);
			data.Write(_aucSearchMin, sizeof _aucSearchMin);
			strDbg.AppendFormat("min=%u ", ulMinSize);
		}

		if (ulMaxSize > 0){
			if (++iParameterCount < iTotalTerms){
				data.Write(&andParameter, 2);
				strDbg.AppendFormat("AND ");
			}
			data.Write(&numericParameter, 1);
			data.Write(&ulMaxSize, 4);
			data.Write(_aucSearchMax, sizeof _aucSearchMax);
			strDbg.AppendFormat("max=%u ", ulMaxSize);
		}
		
		if (iAvailability >= 0){
			if (++iParameterCount < iTotalTerms){
				data.Write(&andParameter, 2);
				strDbg.AppendFormat("AND ");
			}
			data.Write(&numericParameter, 1);
			data.Write(&iAvailability, 4);
			data.Write(_aucSearchAvailability, sizeof _aucSearchAvailability);
			strDbg.AppendFormat("avail=%u ", iAvailability);
		}

		if (!strExtension.IsEmpty()){
			if (++iParameterCount < iTotalTerms){
		    	data.Write(&andParameter,2);
				strDbg.AppendFormat("AND ");
			}
			CString strData(strExtension);
			data.Write(&typeParameter, 1);
			uint16 nSize = strData.GetLength();
			data.Write(&nSize, 2);
			data.Write((LPCSTR)strData, nSize);
			data.Write(_aucSearchExtension, sizeof _aucSearchExtension);
			strDbg.AppendFormat("ext=\"%s\" ", strData);
		}
        
		ASSERT( iParameterCount == iTotalTerms );
	}
	else
		{
		if (!strExtension.IsEmpty()){
			if (++iParameterCount < iTotalTerms){
				data.Write(&andParameter, 2);
				strDbg.AppendFormat("AND ");
			}
		}

		if (iAvailability >= 0){
			if (++iParameterCount < iTotalTerms){
				data.Write(&andParameter,2);
				strDbg.AppendFormat("AND ");
			}
		}
	  
		if (ulMaxSize > 0){
			if (++iParameterCount < iTotalTerms){
				data.Write(&andParameter,2);
				strDbg.AppendFormat("AND ");
			}
		}
        
		if (ulMinSize > 0){
			if (++iParameterCount < iTotalTerms){
				data.Write(&andParameter,2);
				strDbg.AppendFormat("AND ");
			}
		}
        
		if (!strType.IsEmpty()){
			if (++iParameterCount < iTotalTerms){
			  data.Write(&andParameter,2);
				strDbg.AppendFormat("AND ");
			}
		}
        
		for (int j = 0; j < _SearchExpr.m_aExpr.GetCount(); j++){
			CString str(_SearchExpr.m_aExpr[j]);
			if (str == SEARCHOPTOK_AND){
		      data.Write(&andParameter,2);
				strDbg.AppendFormat("AND ");
			}
			else if (str == SEARCHOPTOK_OR){
				data.Write(&orParameter, 2);
				strDbg.AppendFormat("OR ");
			}
			else if (str == SEARCHOPTOK_NOT){
				data.Write(&notParameter, 2);
				strDbg.AppendFormat("NOT ");
			}
			else{
				data.Write(&stringParameter, 1);
				uint16 nSize = str.GetLength();
				data.Write(&nSize, 2);
				data.Write((LPCSTR)str, nSize);
				strDbg.AppendFormat("\"%s\" ", str);
			}
		}

		if (!strType.IsEmpty()){
			data.Write(&typeParameter, 1);
			uint16 nSize = strType.GetLength();
			data.Write(&nSize, 2);
			data.Write((LPCSTR)strType, nSize);
			data.Write(_aucSearchType, sizeof _aucSearchType);
			strDbg.AppendFormat("type=\"%s\" ", strType);
		}

		if (ulMinSize > 0){
			data.Write(&numericParameter, 1);
			data.Write(&ulMinSize, 4);
			data.Write(_aucSearchMin, sizeof _aucSearchMin);
			strDbg.AppendFormat("min=%u ", ulMinSize);
		}

		if (ulMaxSize > 0){
			data.Write(&numericParameter, 1);
			data.Write(&ulMaxSize, 4);
			data.Write(_aucSearchMax, sizeof _aucSearchMax);
			strDbg.AppendFormat("max=%u ", ulMaxSize);
		}

		if (iAvailability >= 0){
			data.Write(&numericParameter, 1);
			data.Write(&iAvailability, 4);
			data.Write(_aucSearchAvailability, sizeof _aucSearchAvailability);
			strDbg.AppendFormat("avail=%u ", iAvailability);
		}

		if (!strExtension.IsEmpty()){
			CString strData(strExtension);
			data.Write(&typeParameter, 1);
			uint16 nSize = strData.GetLength();
			data.Write(&nSize, 2);
			data.Write((LPCSTR)strData, nSize);
			data.Write(_aucSearchExtension, sizeof _aucSearchExtension);
			strDbg.AppendFormat("ext=\"%s\" ", strData);
		}
		}

	if (theApp.glob_prefs->GetDebugServerSearches())
		Debug("Search Data: %s\n", strDbg);
	_SearchExpr.m_aExpr.RemoveAll();
	return true;
		}

bool CSearchDlg::DoNewSearch(SSearchParams* pParams)
{
	if (!theApp.serverconnect->IsConnected())
		return false;

	CSafeMemFile data(100);
	if (!GetSearchPacket(data, pParams->strExpression, pParams->strFileType, 
						 pParams->ulMinSize, pParams->ulMaxSize, pParams->iAvailability, pParams->strExtension, false) 
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
	if (theApp.glob_prefs->GetDebugServerTCP())
		Debug(">>> Sending OP__SearchRequest\n");
		theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
		theApp.serverconnect->SendPacket(packet,false);

	if (pParams->eType == SearchTypeGlobal && theApp.serverconnect->IsUDPSocketAvailable())
	{
		// set timeout timer for local server
		m_uTimerLocalServer = SetTimer(TimerServerTimeout, 50000, NULL);

			if( theApp.glob_prefs->Score() ){
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
		GetDlgItem(IDC_SEARCHNAME)->SetFocus();
	GetDlgItem(IDC_CANCELS)->EnableWindow(true);
	
	Packet* packet = new Packet();
	packet->opcode = OP_QUERY_MORE_RESULT;
	if (theApp.glob_prefs->GetDebugServerTCP())
		Debug(">>> Sending OP__QueryMoreResults\n");
	theApp.uploadqueue->AddUpDataOverheadServer(packet->size);
	theApp.serverconnect->SendPacket(packet);
	m_iSentMoreReq++;
}

void CSearchDlg::StartNewSearch(bool bKademlia)
{
	ESearchType eSearchType = (ESearchType)methodBox.GetCurSel();
	if (bKademlia ? !theApp.kademlia->isConnected() : ((eSearchType == SearchTypeServer || eSearchType == SearchTypeGlobal) && !theApp.serverconnect->IsConnected()))
		AfxMessageBox(GetResString(IDS_ERR_NOTCONNECTED),MB_ICONASTERISK);
	else{
		CString strSearchString;
		GetDlgItem(IDC_SEARCHNAME)->GetWindowText(strSearchString);
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
		
		// 06-Mai-2003: always sending the max-size is no longer needed
		// there is no need to always send a default max size (confirmed by lugdunummaster)
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

		// Because the availability is used with the 'Min' operator, we decrement the entered value by one to get
		// more 'useable' results from the server (from a user's POV). Therefore its correct to send an
		// 'Availability' of 0, because it will return all files with an 'Availability' of >= 1.
		CString strAvailability;
		GetDlgItem(IDC_EDITSEARCHAVAIBILITY)->GetWindowText(strAvailability);
		int iAvailability;
		if (strAvailability.IsEmpty())
			iAvailability = -1; // No Availability specified
		else
			iAvailability = _tstoi(strAvailability) - 1;

		SSearchParams* pParams = new SSearchParams;
		pParams->strExpression = strSearchString;
		pParams->eType = eSearchType;
		pParams->strFileType = strSearchType;
		pParams->strMinSize = strMinSize;
		pParams->ulMinSize = ulMinSize;
		pParams->strMaxSize = strMaxSize;
		pParams->ulMaxSize = ulMaxSize;
		pParams->iAvailability = iAvailability;
		pParams->strExtension = strExtension;
		pParams->bMatchKeywords = IsDlgButtonChecked(IDC_MATCH_KEYWORDS);

		if (!bKademlia){
		CWnd* pWndFocus = GetFocus();
		GetDlgItem(IDC_STARTS)->EnableWindow(false);
		if (pWndFocus == GetDlgItem(IDC_STARTS))
			GetDlgItem(IDC_SEARCHNAME)->SetFocus();
		GetDlgItem(IDC_CANCELS)->EnableWindow(true);

		    if (eSearchType == SearchTypeServer || eSearchType == SearchTypeGlobal)
		    {
		        if (!DoNewSearch(pParams))
				    delete pParams;
		    }
		    else if (eSearchType == SearchTypeJigleSOAP)
		    {
			    if (!DoNewJigleSearch(pParams))
				    delete pParams;
		    }
		    else{
			    ASSERT(0);
			    delete pParams;
		    }
		}
		else{
			if (!DoNewKadSearch(pParams))
				delete pParams;
		}
	}
}

bool CSearchDlg::DoNewKadSearch(SSearchParams* pParams)
{
	if (!theApp.kademlia->isConnected())
		return false;

	if (Kademlia::CTimer::getThreadID() == 0)
		return false;

	int iPos = 0;
	CString strKeyWord = pParams->strExpression.Tokenize(_T(" "), iPos);
	if (strKeyWord.IsEmpty())
		return false;
	CString strSearchExpr = (iPos >= pParams->strExpression.GetLength()) ? _T("") : pParams->strExpression.Mid(iPos);

	CSafeMemFile data(100);
	if (!GetSearchPacket(data, strSearchExpr, pParams->strFileType, pParams->ulMinSize, pParams->ulMaxSize, pParams->iAvailability, pParams->strExtension, true) || (!strSearchExpr.IsEmpty() && data.GetLength() == 0))
		return false;

	LPBYTE pSearchTermsData = NULL;
	UINT uSearchTermsSize = data.GetLength();
	if (uSearchTermsSize){
		pSearchTermsData = new BYTE[uSearchTermsSize];
		data.SeekToBegin();
		data.Read(pSearchTermsData, uSearchTermsSize);
	}
	uint32 searchID = 0;
	Kademlia::CSearch* pSearch = Kademlia::CSearchManager::prepareFindKeywords(KademliaResultKeywordCallback, strKeyWord, uSearchTermsSize, pSearchTermsData);
	delete pSearchTermsData;
	if (pSearch){
		uint32 searchid = pSearch->getSearchID();
		pSearch->setSearchTypes(Kademlia::CSearch::KEYWORD);
		if (!PostThreadMessage(Kademlia::CTimer::getThreadID(), WM_KADEMLIA_STARTSEARCH, 0, (LPARAM)pSearch)){
			Kademlia::CSearchManager::deleteSearch(pSearch);
		}
		else
			searchID = searchid;
	}
	if (searchID == 0)
		return false;
	pParams->dwSearchID = searchID;
	CString strResultType = pParams->strFileType;
	if (strResultType == GetResString(IDS_SEARCH_PRG))
		strResultType = GetResString(IDS_SEARCH_ANY);
	theApp.searchlist->NewSearch(&searchlistctrl,strResultType,searchID);
	CreateNewTab(pParams);
	return true;
}

void CSearchDlg::StartNewSearchKad(){
	StartNewSearch(true);
}

bool CSearchDlg::CreateNewTab(SSearchParams* pParams){
    int iTabItems = searchselect.GetItemCount();
    for (int i = 0; i < iTabItems; i++){
        TCITEM tci;
        tci.mask = TCIF_PARAM;
		if (searchselect.GetItem(i, &tci) && tci.lParam != NULL && ((const SSearchParams*)tci.lParam)->dwSearchID == pParams->dwSearchID)
			return false;
    }
	// add new tab
	TCITEM newitem;
	if (pParams->strExpression.IsEmpty())
		pParams->strExpression = _T("-");
	newitem.mask = TCIF_PARAM|TCIF_TEXT|TCIF_IMAGE;
	newitem.lParam = (LPARAM)pParams;
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)pParams->strExpression);
	newitem.cchTextMax = 0;
	newitem.iImage = 0;
	int itemnr = searchselect.InsertItem(INT_MAX, &newitem);
	if (!searchselect.IsWindowVisible())
		ShowSearchSelector(true);
	searchselect.SetCurSel(itemnr);
	searchlistctrl.ShowResults(pParams->dwSearchID);
	return true;
}

void CSearchDlg::DeleteSearch(uint32 nSearchID)
{
	Kademlia::CSearchManager::stopSearch(nSearchID);
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
		OnBnClickedCancels();
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

void CSearchDlg::DeleteAllSearchs()
{
	for (int i = 0; i < searchselect.GetItemCount(); i++){
		TCITEM item;
		item.mask = TCIF_PARAM;
		item.lParam = -1;
		if (searchselect.GetItem(i, &item) && item.lParam != -1 && item.lParam != NULL){
			Kademlia::CSearchManager::stopSearch(((const SSearchParams*)item.lParam)->dwSearchID);
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
	SetDlgItemText(IDC_SEARCHNAME, pParams->strExpression);
	SetDlgItemText(IDC_EDITSEARCHMIN, pParams->strMinSize);
	SetDlgItemText(IDC_EDITSEARCHMAX, pParams->strMaxSize);
	SetDlgItemText(IDC_EDITSEARCHEXTENSION, pParams->strExtension);

	CString strBuff;
	if (pParams->iAvailability > 0)
		strBuff.Format(_T("%u"), pParams->iAvailability);
	else
		strBuff.Empty();
	SetDlgItemText(IDC_EDITSEARCHAVAIBILITY, strBuff);

	searchlistctrl.ShowResults(pParams->dwSearchID);
}

void CSearchDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	CWaitCursor curWait; // this may take a while
	TCITEM item;
	item.mask = TCIF_PARAM;
	int cur_sel = searchselect.GetCurSel();
	if (cur_sel == (-1))
		return;
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
			OnBnClickedCancels();
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
	GetDlgItem(IDC_EDITSEARCHAVAIBILITY)->SetWindowText("");	
	GetDlgItem(IDC_EDITSEARCHEXTENSION)->SetWindowText("");	
	GetDlgItem(IDC_EDITSEARCHMIN)->SetWindowText("");	
	GetDlgItem(IDC_EDITSEARCHMAX)->SetWindowText("");	
	GetDlgItem(IDC_SEARCHNAME)->SetWindowText("");
	Stypebox.SetCurSel(Stypebox.FindString(-1,GetResString(IDS_SEARCH_ANY)));
	methodBox.SetCurSel(0);

	OnEnChangeSearchname();
}

// khaos::categorymod+ These things are obsolete, replaced by the select cat dialog.
/*void CSearchDlg::UpdateCatTabs() {
	int oldsel=m_cattabs.GetCurSel();
	m_cattabs.DeleteAllItems();
	for (int ix=0;ix<theApp.glob_prefs->GetCatCount();ix++)
		m_cattabs.InsertItem(ix,(ix==0)?GetResString(IDS_ALL):theApp.glob_prefs->GetCategory(ix)->title);
	if (oldsel>=m_cattabs.GetItemCount() || oldsel==-1)
		oldsel=0;

	m_cattabs.SetCurSel(oldsel);
	int flag;
	flag=(m_cattabs.GetItemCount()>1) ? SW_SHOW:SW_HIDE;
	
	this->GetDlgItem(IDC_CATTAB2)->ShowWindow(flag);
	this->GetDlgItem(IDC_STATIC_DLTOof)->ShowWindow(flag);
}*/
// khaos::categorymod-

void CSearchDlg::SearchClipBoard() {
	if (m_guardCBPrompt) return;

	CString clpbrd=theApp.CopyTextFromClipboard();
	if (clpbrd=="") return;

	if (clpbrd.Compare(m_lastclpbrd)==0) return;

	if (clpbrd.Left(13).CompareNoCase("ed2k://|file|")==0) {
		m_guardCBPrompt=true;
		if (AfxMessageBox(GetResString(IDS_ED2KLINKFIX) + _T("\r\n\r\n") + GetResString(IDS_ADDDOWNLOADSFROMCB)+_T("\r\n") + clpbrd,MB_YESNO|MB_TOPMOST)==IDYES)
			// khaos::categorymod+ Modified so that it will display the Sel Cat dlg.
			AddEd2kLinksToDownload(clpbrd);
			// khaos::categorymod-
	}
	m_lastclpbrd=clpbrd;
	m_guardCBPrompt=false;

}

// khaos::categorymod+ Removed Param: uint8 cat
void CSearchDlg::AddEd2kLinksToDownload(CString strlink, int theCat){
// khaos:: categorymod-
	CString resToken;
	int curPos=0;

	resToken= strlink.Tokenize("\t\n\r",curPos);
	while (resToken != "")
	{
		
		if (resToken.Right(1)!="/") resToken+="/";
		try {
			CED2KLink* pLink = CED2KLink::CreateLinkFromUrl(resToken.Trim());
			_ASSERT( pLink !=0 );
			if ( pLink->GetKind() == CED2KLink::kFile) {
				// khaos::categorymod+ Modified to support sel cat
				// pFileLink IS NOT A LEAK, DO NOT DELETE.
				//MORPH START - HotFix by SiRoB, Khaos 14.6 Tempory Patch
				//CED2KFileLink* pFileLink = new CED2KFileLink(pLink);
				CED2KFileLink* pFileLink = (CED2KFileLink*)CED2KLink::CreateLinkFromUrl(resToken.Trim());
				//MORPH END - HotFix by SiRoB, Khaos 14.6 Tempory Patch
				pFileLink->SetCat(theCat);
				theApp.downloadqueue->AddFileLinkToDownload(pFileLink, true, theCat>=0?true:false);
				// khaos::categorymod-
			} else {
				delete pLink; // [i_a] memleak
				throw CString(_T("bad link"));
			}
			delete pLink;
		} catch(CString error){
			OUTPUT_DEBUG_TRACE();
			char buffer[200];
			sprintf(buffer,GetResString(IDS_ERR_INVALIDLINK),error.GetBuffer());
			AddLogLine(true, GetResString(IDS_ERR_LINKERROR), buffer);
		}
		resToken= strlink.Tokenize("\t\n\r",curPos);
	}
}

void CSearchDlg::OnEnKillfocusElink()
{
	CString content;
	GetDlgItem(IDC_ELINK)->GetWindowText(content);
	if (content.GetLength()==0 || content.Find('\n')==-1) return;
	content.Replace("\n","\r\n");
	content.Replace("\r\r","\r");
	GetDlgItem(IDC_ELINK)->SetWindowText(content);
}
CString	CSearchDlg::ToQueryString(CString str){
	CString sTmp = URLEncode(str);
	sTmp.Replace("%20","+");
	return sTmp;
}

void CSearchDlg::ShowSearchSelector(bool visible){
	WINDOWPLACEMENT wpSearchWinPos;
	WINDOWPLACEMENT wpSelectWinPos;
	searchselect.GetWindowPlacement(&wpSelectWinPos);
	searchlistctrl.GetWindowPlacement(&wpSearchWinPos);
	if (visible)
		wpSearchWinPos.rcNormalPosition.top = wpSelectWinPos.rcNormalPosition.bottom;
	else
		wpSearchWinPos.rcNormalPosition.top = wpSelectWinPos.rcNormalPosition.top;
	searchselect.ShowWindow( ((visible)? SW_SHOW : SW_HIDE) );
	RemoveAnchor(searchlistctrl);
	searchlistctrl.SetWindowPlacement(&wpSearchWinPos);
	AddAnchor(searchlistctrl,TOP_LEFT,BOTTOM_RIGHT);
	GetDlgItem(IDC_CLEARALL)->ShowWindow((visible) ? SW_SHOW:SW_HIDE);
}

BOOL CSearchDlg::SaveSearchStrings()
{
	if (m_pacSearchString == NULL)
		return FALSE;
	return m_pacSearchString->SaveList(CString(theApp.glob_prefs->GetConfigDir()) + _T("\\") SEARCH_STRINGS_PROFILE);
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
		theApp.glob_prefs->SetSearchMethod(iMethod);
	GetDlgItem(IDC_MATCH_KEYWORDS)->EnableWindow(iMethod == SearchTypeJigleSOAP || iMethod == SearchTypeJigle);
	GetDlgItem(IDC_MATCH_KEYWORDS)->ShowWindow((iMethod == SearchTypeJigleSOAP || iMethod == SearchTypeJigle) ? SW_SHOW : SW_HIDE);
}

void CSearchDlg::OnCbnSelchangeCombo1()
{
	UpdateControls();
}

void CSearchDlg::OnCbnSelendokCombo1()
{
	UpdateControls();
}

void KademliaSearchKeyword(uint32 searchID, const Kademlia::CUInt128* fileID, LPCSTR name, uint32 size, LPCSTR type, uint16 numProperties, va_list args)
{
	CString idStr;
	fileID->toHexString(&idStr);
	uchar fileid[16];
	DecodeBase16(idStr.GetBuffer(),idStr.GetLength(),fileid);
	
	CMemFile* temp = new CMemFile(250);
	temp->Write(&fileid, 16);
	
	uint32 clientip = 0;
	temp->Write(&clientip, 4);
	uint16 clientport = 0;
	temp->Write(&clientport, 2);
	
	// write tag list
	UINT uFilePosTagCount = temp->GetPosition();
	uint32 tagcount = 0;
	temp->Write(&tagcount, 4);

	// standard tags
	CTag tagName(FT_FILENAME, name);
	tagName.WriteTagToFile(temp);
	tagcount++;

	CTag tagSize(FT_FILESIZE, size);
	tagSize.WriteTagToFile(temp);
	tagcount++;

	if (type != NULL && type[0] != '\0'){
		CTag tagType(FT_FILETYPE, type);
		tagType.WriteTagToFile(temp);
		tagcount++;
	}

	// additional tags
	while (numProperties-- > 0){
		LPCSTR pszPropType = va_arg(args, LPCSTR);
		LPCSTR pszPropName = va_arg(args, LPCSTR);
		LPCSTR pszPropValue = va_arg(args, LPCSTR);
		if( (int)pszPropType == 2 )
		{
			if (pszPropValue != NULL && pszPropValue[0] != '\0'){
				if (strlen(pszPropName) == 1){
					CTag tagProp((uint8)*pszPropName, pszPropValue);
					tagProp.WriteTagToFile(temp);
				}
				else{
					CTag tagProp(pszPropName, pszPropValue);
					tagProp.WriteTagToFile(temp);
				}
				tagcount++;
			}
		}
		else if( (int)pszPropType == 3 )
		{
			CTag tagProp(pszPropName, (uint32)pszPropValue);
			tagProp.WriteTagToFile(temp);
			tagcount++;
		}
		else
		{
			if (pszPropValue != NULL && pszPropValue[0] != '\0'){
				CTag tagProp(pszPropName, pszPropValue);
				tagProp.WriteTagToFile(temp);
				tagcount++;
			}
		}
	}
	temp->Seek(uFilePosTagCount, SEEK_SET);
	temp->Write(&tagcount, 4);
	
	temp->SeekToBegin();
	CSearchFile* tempFile = new CSearchFile(temp, searchID, 0, 0, 0, true);
	theApp.searchlist->AddToList(tempFile);
	
	delete temp;
}

