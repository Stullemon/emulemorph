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
#include "stdafx.h"
#include "emule.h"
#include "SearchDlg.h"
#include "SearchResultsWnd.h"
#include "SearchParamsWnd.h"
#include "SearchParams.h"
#include "Packets.h"
#include "OtherFunctions.h"
#include "SearchList.h"
#include "Sockets.h"
#include "ServerList.h"
#include "Server.h"
#include "SafeFile.h"
#include "DownloadQueue.h"
#include "Statistics.h"
#include "emuledlg.h"
#include "opcodes.h"
#include "ED2KLink.h"
#include "Kademlia/Kademlia/Kademlia.h"
#include "kademlia/kademlia/SearchManager.h"
#include "kademlia/kademlia/search.h"
#include "SearchExpr.h"
#define USE_FLEX
#include "Parser.hpp"
#include "Scanner.h"
#include "HelpIDs.h"
#include "Exceptions.h"
#include "StringConversion.h"
#include "UserMsgs.h"
#include "TransferWnd.h" //MORPH - Added by SiRoB, Selective Category

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


extern int yyparse();
extern int yyerror(const char* errstr);
extern int yyerror(LPCTSTR errstr);
extern LPCTSTR _aszInvKadKeywordChars;

enum ESearchTimerID
{
	TimerServerTimeout = 1,
	TimerGlobalSearch
};

enum ESearchResultImage
{
	sriServerActive,
	sriGlobalActive,
	sriKadActice,
	sriClient,
	sriServer,
	sriGlobal,
	sriKad
};

static const LPCTSTR _apszSearchExprKeywords[] = { _T("AND"), _T("OR"), _T("NOT"), NULL };
static const TCHAR _szSearchExprSeperators[] = _T(" ()<>=");

// CSearchResultsWnd dialog

IMPLEMENT_DYNCREATE(CSearchResultsWnd, CResizableFormView)

BEGIN_MESSAGE_MAP(CSearchResultsWnd, CResizableFormView)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_SDOWNLOAD, OnBnClickedSdownload)
	ON_BN_CLICKED(IDC_CLEARALL, OnBnClickedClearall)
	ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, OnTcnSelchangeTab1)
	ON_MESSAGE(UM_CLOSETAB, OnCloseTab)
	ON_MESSAGE(UM_DBLCLICKTAB, OnDblClickTab)
	ON_WM_DESTROY()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_WM_CREATE()
	ON_WM_HELPINFO()
	ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
	ON_BN_CLICKED(IDC_OPEN_PARAMS_WND, OnBnClickedOpenParamsWnd)
	ON_WM_SYSCOMMAND()
	ON_NOTIFY(NM_CLICK, IDC_CATTAB2, OnNMClickCattab2) //MORPH - Added by SiRoB, Selection category support
END_MESSAGE_MAP()

CSearchResultsWnd::CSearchResultsWnd(CWnd* pParent /*=NULL*/)
	: CResizableFormView(CSearchResultsWnd::IDD)
{
	m_nSearchID = 0x80000000;
	global_search_timer = 0;
	searchpacket = NULL;
	canceld = false;
	servercount = 0;
	globsearch = false;
	icon_search = NULL;
	m_uTimerLocalServer = 0;
	m_iSentMoreReq = 0;
	searchselect.m_bCloseable = true;
}

CSearchResultsWnd::~CSearchResultsWnd()
{
	if (globsearch)
		delete searchpacket;
	if (icon_search)
		VERIFY( DestroyIcon(icon_search) );
	if (m_uTimerLocalServer)
		VERIFY( KillTimer(m_uTimerLocalServer) );
}

void CSearchResultsWnd::OnInitialUpdate()
{
	CResizableFormView::OnInitialUpdate();
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
	AddAnchor(IDC_CLEARALL, TOP_RIGHT);
	AddAnchor(IDC_OPEN_PARAMS_WND, TOP_RIGHT);
	AddAnchor(searchselect.m_hWnd,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_STATIC_DLTOof,BOTTOM_LEFT);
	AddAnchor(IDC_CATTAB2,BOTTOM_LEFT,BOTTOM_RIGHT);

	ShowSearchSelector(false);

	if (theApp.m_fontSymbol.m_hObject){
		GetDlgItem(IDC_STATIC_DLTOof)->SetFont(&theApp.m_fontSymbol);
		GetDlgItem(IDC_STATIC_DLTOof)->SetWindowText(_T("8")); // show a right-arrow
	}
}

void CSearchResultsWnd::DoDataExchange(CDataExchange* pDX)
{
	CResizableFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SEARCHLIST, searchlistctrl);
	DDX_Control(pDX, IDC_PROGRESS1, searchprogress);
	DDX_Control(pDX, IDC_TAB1, searchselect);
	DDX_Control(pDX, IDC_CATTAB2, m_cattabs);
}

void CSearchResultsWnd::StartSearch(SSearchParams* pParams)
{
	switch (pParams->eType)
	{
		case SearchTypeEd2kServer:
		case SearchTypeEd2kGlobal:
		case SearchTypeKademlia:
			StartNewSearch(pParams);
			break;

		case SearchTypeFileDonkey:
			ShellOpenFile(CreateWebQuery(pParams));
			delete pParams;
			return;

		default:
			ASSERT(0);
			delete pParams;
	}
}

void CSearchResultsWnd::OnTimer(UINT nIDEvent)
{
	CResizableFormView::OnTimer(nIDEvent);

	if (m_uTimerLocalServer != 0 && nIDEvent == m_uTimerLocalServer)
	{
		if (thePrefs.GetDebugServerSearchesLevel() > 0)
			Debug(_T("Timeout waiting on search results of local server\n"));
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
    
		    if (toask && servercount < theApp.serverlist->GetServerCount()-1){
			    servercount++;
				if (toask->GetUDPFlags() & SRV_UDPFLG_EXT_GETFILES)
					searchpacket->opcode = OP_GLOBSEARCHREQ2;
				else
					searchpacket->opcode = OP_GLOBSEARCHREQ;
				if (thePrefs.GetDebugServerUDPLevel() > 0)
					Debug(_T(">>> Sending %s  to server %s:%u (%u of %u)\n"), (searchpacket->opcode == OP_GLOBSEARCHREQ2) ? _T("OP__GlobSearchReq2") : _T("OP__GlobSearchReq"), toask->GetAddress(), toask->GetPort(), servercount, theApp.serverlist->GetServerCount());
				theStats.AddUpDataOverheadServer(searchpacket->size);
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

void CSearchResultsWnd::SetSearchResultsIcon(UINT uSearchID, int iImage)
{
    int iTabItems = searchselect.GetItemCount();
    for (int i = 0; i < iTabItems; i++)
	{
        TCITEM tci;
        tci.mask = TCIF_PARAM;
		if (searchselect.GetItem(i, &tci) && tci.lParam != NULL && ((const SSearchParams*)tci.lParam)->dwSearchID == uSearchID)
		{
			tci.mask = TCIF_IMAGE;
			tci.iImage = iImage;
			searchselect.SetItem(i, &tci);
			break;
		}
    }
}

void CSearchResultsWnd::SetActiveSearchResultsIcon(UINT uSearchID)
{
	SSearchParams* pParams = GetSearchResultsParams(uSearchID);
	if (pParams)
	{
		int iImage;
		if (pParams->eType == SearchTypeKademlia)
			iImage = sriKadActice;
		else if (pParams->eType == SearchTypeEd2kGlobal)
			iImage = sriGlobalActive;
		else
			iImage = sriServerActive;
		SetSearchResultsIcon(uSearchID, iImage);
	}
}

void CSearchResultsWnd::SetInactiveSearchResultsIcon(UINT uSearchID)
{
	SSearchParams* pParams = GetSearchResultsParams(uSearchID);
	if (pParams)
	{
		int iImage;
		if (pParams->eType == SearchTypeKademlia)
			iImage = sriKad;
		else if (pParams->eType == SearchTypeEd2kGlobal)
			iImage = sriGlobal;
		else
			iImage = sriServer;
		SetSearchResultsIcon(uSearchID, iImage);
	}
}

SSearchParams* CSearchResultsWnd::GetSearchResultsParams(UINT uSearchID) const
{
    int iTabItems = searchselect.GetItemCount();
    for (int i = 0; i < iTabItems; i++)
	{
        TCITEM tci;
        tci.mask = TCIF_PARAM;
		if (searchselect.GetItem(i, &tci) && tci.lParam != NULL && ((const SSearchParams*)tci.lParam)->dwSearchID == uSearchID)
			return (SSearchParams*)tci.lParam;
    }
	return NULL;
}

void CSearchResultsWnd::CancelSearch()
{
	SetInactiveSearchResultsIcon(m_nSearchID);

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
	m_pwndParams->m_ctlCancel.EnableWindow(false);
	if (pWndFocus && pWndFocus->m_hWnd == m_pwndParams->m_ctlCancel.m_hWnd)
		m_pwndParams->m_ctlName.SetFocus();
	m_pwndParams->m_ctlStart.EnableWindow(true);
}

void CSearchResultsWnd::CancelKadSearch(UINT uSearchID)
{
	SetInactiveSearchResultsIcon(uSearchID);
}

void CSearchResultsWnd::LocalSearchEnd(uint16 count, bool bMoreResultsAvailable)
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
			SetInactiveSearchResultsIcon(m_nSearchID);
			m_pwndParams->m_ctlStart.EnableWindow(TRUE);
			CWnd* pWndFocus = GetFocus();
			m_pwndParams->m_ctlCancel.EnableWindow(FALSE);
			if (pWndFocus && pWndFocus->m_hWnd == m_pwndParams->m_ctlCancel.m_hWnd)
				m_pwndParams->m_ctlName.SetFocus();
		}
		else{
			VERIFY( (global_search_timer = SetTimer(TimerGlobalSearch, 750, 0)) != NULL );
		}
	}
	m_pwndParams->m_ctlMore.EnableWindow(bMoreResultsAvailable && m_iSentMoreReq < MAX_MORE_SEARCH_REQ);
}

void CSearchResultsWnd::AddUDPResult(uint16 count)
{
	if (!canceld && count > MAX_RESULTS)
		CancelSearch();
}

void CSearchResultsWnd::OnBnClickedSdownload()
{
	//start download(s)
	DownloadSelected();
}

void CSearchResultsWnd::OnNMDblclkSearchlist(NMHDR *pNMHDR, LRESULT *pResult)
{
	OnBnClickedSdownload();
	*pResult = 0;
}

CString	CSearchResultsWnd::CreateWebQuery(SSearchParams* pParams)
{
	CString query;
	switch (pParams->eType)
	{
	case SearchTypeFileDonkey:
		query = _T("http://www.filedonkey.com/search.html?");
		query += _T("pattern=") + ToQueryString(pParams->strExpression);
		if (pParams->strFileType == ED2KFTSTR_AUDIO)
			query += _T("&media=Audio");
		else if (pParams->strFileType == ED2KFTSTR_VIDEO)
			query += _T("&media=Video");
		else if (pParams->strFileType == ED2KFTSTR_PROGRAM)
			query += _T("&media=Pro");
		query += _T("&requestby=emule");

		if (pParams->ulMinSize > 0)
			query.AppendFormat(_T("&min_size=%u"),pParams->ulMinSize);
		
		if (pParams->ulMaxSize > 0)
			query.AppendFormat(_T("&max_size=%u"),pParams->ulMaxSize);

		break;
	default:
		return _T("");
	}
	return query;
}

void CSearchResultsWnd::DownloadSelected()
{
	DownloadSelected(thePrefs.AddNewFilesPaused());
}

void CSearchResultsWnd::DownloadSelected(bool bPaused)
{
	CWaitCursor curWait;
	POSITION pos = searchlistctrl.GetFirstSelectedItemPosition(); 
	
	// khaos::categorymod+ Category selection stuff...
	if (!pos) return; // No point in asking for a category if there are no selected files to download.

	int useCat = m_cattabs.GetCurSel();
	bool	bCreatedNewCat = false;
	if (useCat==-1 && thePrefs.SelectCatForNewDL())
	{
		CSelCategoryDlg* getCatDlg = new CSelCategoryDlg((CWnd*)theApp.emuledlg);
		getCatDlg->DoModal();

		// Returns 0 on 'Cancel', otherwise it returns the selected category
		// or the index of a newly created category.  Users can opt to add the
		// links into a new category.
		useCat = getCatDlg->GetInput();
		bCreatedNewCat = getCatDlg->CreatedNewCat();
		bool	bCanceled = getCatDlg->WasCancelled(); //MORPH - Added by SiRoB, WasCanceled
		delete getCatDlg;
		if (bCanceled)
			return;
	}
	// khaos::categorymod-

	while (pos != NULL) 
	{ 
		int iIndex = searchlistctrl.GetNextSelectedItem(pos); 
		if (iIndex >= 0)
		{
			// get selected listview item (may be a child item from an expanded search result)
			CSearchFile* cur_file = (CSearchFile*)searchlistctrl.GetItemData(iIndex);

			if (cur_file->IsComplete() == 0 && cur_file->GetSourceCount() >= 50)
			{
				CString strMsg;
				strMsg.Format(GetResString(IDS_ASKDLINCOMPLETE), cur_file->GetFileName());
				int iAnswer = AfxMessageBox(strMsg, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
				if (iAnswer != IDYES)
					continue;
			}

			// khaos::categorymod+ m_cattabs is obsolete.
			if (!thePrefs.SelectCatForNewDL() && thePrefs.UseAutoCat() && useCat==-1)
			{
				useCat = theApp.downloadqueue->GetAutoCat(CString(cur_file->GetFileName()), (ULONG)cur_file->GetFileSize());
				if (!useCat && thePrefs.UseActiveCatForLinks())
					useCat = theApp.emuledlg->transferwnd->GetActiveCategory();
			}
			
			if (thePrefs.SmallFileDLPush() && cur_file->GetFileSize() < 154624)
				theApp.downloadqueue->AddSearchToDownload(cur_file, bPaused, useCat, 0);
			else if (thePrefs.AutoSetResumeOrder())
				theApp.downloadqueue->AddSearchToDownload(cur_file, bPaused, useCat, theApp.downloadqueue->GetMaxCatResumeOrder(useCat)+1);
			else
			// khaos::categorymod-
			// use filename of selected listview item
			theApp.downloadqueue->AddSearchToDownload(cur_file, bPaused, useCat, theApp.downloadqueue->GetMaxCatResumeOrder(useCat));

			// get parent
			if (cur_file->GetListParent()!=NULL)
				cur_file=cur_file->GetListParent();

			// update parent and all childs
			searchlistctrl.UpdateSources(cur_file);
		}
	}
	
	// This bit of code will resume the number of files that the user specifies in preferences (Off by default)
	if (thePrefs.StartDLInEmptyCats() > 0 && bCreatedNewCat && bPaused)
		for (int i = 0; i < thePrefs.StartDLInEmptyCats(); i++)
			if (!theApp.downloadqueue->StartNextFile(useCat)) break;
}

void CSearchResultsWnd::OnSysColorChange()
{
	CResizableFormView::OnSysColorChange();
	SetAllIcons();
	searchlistctrl.CreateMenues();
}

void CSearchResultsWnd::SetAllIcons()
{
	if (icon_search)
		VERIFY( DestroyIcon(icon_search) );
	icon_search = theApp.LoadIcon(_T("SearchResults"), 16, 16);
	((CStatic*)GetDlgItem(IDC_SEARCHLST_ICO))->SetIcon(icon_search);

	CImageList iml;
	iml.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,1);
	iml.SetBkColor(CLR_NONE);
	iml.Add(CTempIconLoader(_T("SearchMethod_ServerActive"), 16, 16));
	iml.Add(CTempIconLoader(_T("SearchMethod_GlobalActive"), 16, 16));
	iml.Add(CTempIconLoader(_T("SearchMethod_KademliaActive"), 16, 16));
	iml.Add(CTempIconLoader(_T("StatsClients"), 16, 16));
	iml.Add(CTempIconLoader(_T("SearchMethod_SERVER"), 16, 16));
	iml.Add(CTempIconLoader(_T("SearchMethod_GLOBAL"), 16, 16));
	iml.Add(CTempIconLoader(_T("SearchMethod_KADEMLIA"), 16, 16));
	searchselect.SetImageList(&iml);
	m_imlSearchResults.DeleteImageList();
	m_imlSearchResults.Attach(iml.Detach());
	searchselect.SetPadding(CSize(10, 3));
}

void CSearchResultsWnd::Localize()
{
	searchlistctrl.Localize();
	UpdateCatTabs();

    GetDlgItem(IDC_CLEARALL)->SetWindowText(GetResString(IDS_REMOVEALLSEARCH));
    GetDlgItem(IDC_RESULTS_LBL)->SetWindowText(GetResString(IDS_SW_RESULT));
    GetDlgItem(IDC_SDOWNLOAD)->SetWindowText(GetResString(IDS_SW_DOWNLOAD));
	GetDlgItem(IDC_OPEN_PARAMS_WND)->SetWindowText(GetResString(IDS_SEARCHPARAMS)+_T("..."));
}

void CSearchResultsWnd::OnBnClickedClearall()
{
	CancelSearch();
	DeleteAllSearchs();

	CWnd* pWndFocus = GetFocus();
	m_pwndParams->m_ctlMore.EnableWindow(FALSE);
	if (pWndFocus && pWndFocus->m_hWnd == m_pwndParams->m_ctlMore.m_hWnd)
		m_pwndParams->m_ctlStart.SetFocus();
}

static CSearchExpr _SearchExpr;
CStringArray _astrParserErrors;

#ifdef _DEBUG
static char _chLastChar = 0;
static CStringA _strSearchTree;

bool DumpSearchTree(int& iExpr, const CSearchExpr& rSearchExpr)
{
	if (iExpr >= rSearchExpr.m_aExpr.GetCount())
		return false;
	CStringA strTok = rSearchExpr.m_aExpr[iExpr++];
	if (strTok == SEARCHOPTOK_AND || strTok == SEARCHOPTOK_OR || strTok == SEARCHOPTOK_NOT)
	{
		if (_chLastChar != '(' && _chLastChar != '\0')
			_strSearchTree.AppendFormat(" ");
		_strSearchTree.AppendFormat("(%s ", strTok.Mid(1));
		_chLastChar = '(';
		DumpSearchTree(iExpr, rSearchExpr);
		DumpSearchTree(iExpr, rSearchExpr);
		_strSearchTree.AppendFormat(")");
		_chLastChar = ')';
	}
	else
	{
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

static CStringA _strCurKadKeywordA;

void ParsedSearchExpression(const CSearchExpr* pexpr)
{
	int iOpAnd = 0;
	int iOpOr = 0;
	int iOpNot = 0;
	CStringA strDbg;
	for (int i = 0; i < pexpr->m_aExpr.GetCount(); i++)
	{
		CStringA str(pexpr->m_aExpr[i]);
		if (str == SEARCHOPTOK_AND)
		{
			iOpAnd++;
			strDbg.AppendFormat("%s ", str.Mid(1));
		}
		else if (str == SEARCHOPTOK_OR)
		{
			iOpOr++;
			strDbg.AppendFormat("%s ", str.Mid(1));
		}
		else if (str == SEARCHOPTOK_NOT)
		{
			iOpNot++;
			strDbg.AppendFormat("%s ", str.Mid(1));
		}
		else
		{
			strDbg.AppendFormat("\"%s\" ", str);
		}
	}
	if (thePrefs.GetDebugServerSearchesLevel() > 0)
		Debug(_T("Search Expr: %hs\n"), strDbg);

	// this limit (+ the additional operators which will be added later) has to match the limit in 'CreateSearchExpressionTree'
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
	if (iOpAnd > 0 && iOpOr == 0 && iOpNot == 0)
	{
		CStringA strAndTerms;
		for (int i = 0; i < pexpr->m_aExpr.GetCount(); i++)
		{
			if (pexpr->m_aExpr[i] != SEARCHOPTOK_AND)
			{
				// Minor optimization: Because we added the Kad keyword to the boolean search expression,
				// we remove it here (and only here) again because we know that the entire search expression
				// does only contain (implicit) ANDed strings.
				if (pexpr->m_aExpr[i] != _strCurKadKeywordA)
				{
					if (!strAndTerms.IsEmpty())
						strAndTerms += ' ';
					strAndTerms += pexpr->m_aExpr[i];
				}
			}
		}
		ASSERT( _SearchExpr.m_aExpr.GetCount() == 0);
		_SearchExpr.m_aExpr.Add(strAndTerms);
	}
	else
	{
		if (pexpr->m_aExpr.GetCount() != 1 || pexpr->m_aExpr[0] != _strCurKadKeywordA)
			_SearchExpr.m_aExpr.Append(pexpr->m_aExpr);
	}

#ifdef _DEBUG
	if (thePrefs.GetDebugServerSearchesLevel() > 0){
		_strSearchTree.Empty();
		DumpSearchTree(_SearchExpr);
		Debug(_T("Search Tree: %hs\n"), _strSearchTree);
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

class CSearchExprTarget
{
public:
	CSearchExprTarget(CSafeMemFile* pData, EUtf8Str eStrEncode)
	{
		m_data = pData;
		m_eStrEncode = eStrEncode;
	}

	const CString& GetDebugString() const
	{
		return m_strDbg;
	}

	void WriteBooleanAND()
{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x00);			// "AND"
		m_strDbg.AppendFormat(_T("AND "));
}

	void WriteBooleanOR()
{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x01);			// "OR"
		m_strDbg.AppendFormat(_T("OR "));
}

	void WriteBooleanNOT()
{
		m_data->WriteUInt8(0);				// boolean operator parameter type
		m_data->WriteUInt8(0x02);			// "NOT"
		m_strDbg.AppendFormat(_T("NOT "));
}

	void WriteMetaDataSearchParam(const CString& rstrValue)
{
		m_data->WriteUInt8(1);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_strDbg.AppendFormat(_T("\"%s\" "), rstrValue);
}

	void WriteMetaDataSearchParam(UINT uMetaTagID, const CString& rstrValue)
{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_data->WriteUInt16(sizeof uint8);	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);	// meta tag ID name
		m_strDbg.AppendFormat(_T("%s=\"%s\" "), DbgGetMetaTagName(uMetaTagID), rstrValue);
}

	void WriteMetaDataSearchParamA(UINT uMetaTagID, const CStringA& rstrValueA)
{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValueA);	// string value
		m_data->WriteUInt16(sizeof uint8);	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);	// meta tag ID name
		m_strDbg.AppendFormat(_T("%s=\"%hs\" "), DbgGetMetaTagName(uMetaTagID), rstrValueA);
}

	void WriteMetaDataSearchParam(LPCSTR pszMetaTagID, const CString& rstrValue)
{
		m_data->WriteUInt8(2);				// string parameter type
		m_data->WriteString(rstrValue, m_eStrEncode); // string value
		m_data->WriteString(pszMetaTagID);	// meta tag ID
		m_strDbg.AppendFormat(_T("%s=\"%s\" "), DbgGetMetaTagName(pszMetaTagID), rstrValue);
}

	void WriteMetaDataSearchParam(UINT uMetaTagID, UINT uOperator, UINT uValue, bool bEd2k)
{
		m_data->WriteUInt8(3);				// numeric parameter type
		m_data->WriteUInt32(uValue);		// numeric value
		m_data->WriteUInt8(uOperator);		// comparison operator
		m_data->WriteUInt16(sizeof uint8);	// meta tag ID length
		m_data->WriteUInt8(uMetaTagID);	// meta tag ID name
		m_strDbg.AppendFormat(_T("%s%s%u "), DbgGetMetaTagName(uMetaTagID), DbgGetOperatorName(bEd2k, uOperator), uValue);
}

	void WriteMetaDataSearchParam(LPCSTR pszMetaTagID, UINT uOperator, UINT uValue, bool bEd2k)
{
		m_data->WriteUInt8(3);				// numeric parameter type
		m_data->WriteUInt32(uValue);		// numeric value
		m_data->WriteUInt8(uOperator);		// comparison operator
		m_data->WriteString(pszMetaTagID);	// meta tag ID
		m_strDbg.AppendFormat(_T("%s%s%u "), DbgGetMetaTagName(pszMetaTagID), DbgGetOperatorName(bEd2k, uOperator), uValue);
}

	void WriteOldMinMetaDataSearchParam(UINT uMetaTagID, UINT uValue, bool bEd2k)
{
	UINT uOperator;
	if (bEd2k){
		uOperator = ED2K_SEARCH_OP_GREATER;
		uValue -= 1;
	}
	else
		uOperator = KAD_SEARCH_OP_GREATER_EQUAL;
		WriteMetaDataSearchParam(uMetaTagID, uOperator, uValue, bEd2k);
}

	void WriteOldMinMetaDataSearchParam(LPCSTR pszMetaTagID, UINT uValue, bool bEd2k)
{
	UINT uOperator;
	if (bEd2k){
		uOperator = ED2K_SEARCH_OP_GREATER;
		uValue -= 1;
	}
	else
		uOperator = KAD_SEARCH_OP_GREATER_EQUAL;
		WriteMetaDataSearchParam(pszMetaTagID, uOperator, uValue, bEd2k);
}

	void WriteOldMaxMetaDataSearchParam(LPCSTR pszMetaTagID, UINT uValue, bool bEd2k)
{
	UINT uOperator;
	if (bEd2k){
		uOperator = ED2K_SEARCH_OP_LESS;
		uValue += 1;
	}
	else
		uOperator = KAD_SEARCH_OP_LESS_EQUAL;
		WriteMetaDataSearchParam(pszMetaTagID, uOperator, uValue, bEd2k);
}

	void WriteOldMaxMetaDataSearchParam(UINT uMetaTagID, UINT uValue, bool bEd2k)
{
	UINT uOperator;
	if (bEd2k){
		uOperator = ED2K_SEARCH_OP_LESS;
		uValue += 1;
	}
	else
		uOperator = KAD_SEARCH_OP_LESS_EQUAL;
		WriteMetaDataSearchParam(uMetaTagID, uOperator, uValue, bEd2k);
}

protected:
	CSafeMemFile* m_data;
	CString m_strDbg;
	EUtf8Str m_eStrEncode;
};


bool GetSearchPacket(CSafeMemFile* pData, SSearchParams* pParams)
{
	CStringA strFileType;
	if (pParams->strFileType == ED2KFTSTR_ARCHIVE){
		// eDonkeyHybrid 0.48 uses type "Pro" for archives files
		// www.filedonkey.com uses type "Pro" for archives files
		strFileType = ED2KFTSTR_PROGRAM;
	}
	else if (pParams->strFileType == ED2KFTSTR_CDIMAGE){
		// eDonkeyHybrid 0.48 uses *no* type for iso/nrg/cue/img files
		// www.filedonkey.com uses type "Pro" for CD-image files
		strFileType = ED2KFTSTR_PROGRAM;
	}
	else{
		//TODO: Support "Doc" types
		strFileType = pParams->strFileType;
	}

	_strCurKadKeywordA.Empty();
	ASSERT( !pParams->strExpression.IsEmpty() );
	if (pParams->eType == SearchTypeKademlia)
	{
		ASSERT( !pParams->strKeyword.IsEmpty() );
		_strCurKadKeywordA = StrToUtf8(pParams->strKeyword);
	}
	if (pParams->strBooleanExpr.IsEmpty())
		pParams->strBooleanExpr = pParams->strExpression;
	if (pParams->strBooleanExpr.IsEmpty())
		return false;

	//TRACE(_T("Raw search expr:\n"));
	//TRACE(_T("%s"), pParams->strBooleanExpr);
	//TRACE(_T("  %s\n"), DbgGetHexDump((uchar*)(LPCTSTR)pParams->strBooleanExpr, pParams->strBooleanExpr.GetLength()*sizeof(TCHAR)));
	_astrParserErrors.RemoveAll();
	_SearchExpr.m_aExpr.RemoveAll();
	if (!pParams->strBooleanExpr.IsEmpty())
	{
		// check this here again, we could have been called from Webinterface or MM
		if (!pParams->bUnicode)
		{
			CStringA strACP(pParams->strBooleanExpr);
			if (!IsValidEd2kStringA(strACP))
				throw new CMsgBoxException(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + GetResString(IDS_SEARCH_INVALIDCHAR), MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
		}

	    LexInit(pParams->strBooleanExpr);
	    int iParseResult = yyparse();
	    LexFree();
	    if (_astrParserErrors.GetSize() > 0)
		{
		    _SearchExpr.m_aExpr.RemoveAll();
		    throw new CMsgBoxException(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + _astrParserErrors[_astrParserErrors.GetSize() - 1], MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
	    }
	    else if (iParseResult != 0)
		{
		    _SearchExpr.m_aExpr.RemoveAll();
		    throw new CMsgBoxException(GetResString(IDS_SEARCH_EXPRERROR) + _T("\n\n") + GetResString(IDS_SEARCH_GENERALERROR), MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
	    }
	}
	//TRACE(_T("Parsed search expr:\n"));
	//for (int i = 0; i < _SearchExpr.m_aExpr.GetCount(); i++){
	//	TRACE(_T("%s"), _SearchExpr.m_aExpr[i]);
	//	TRACE(_T("  %s\n"), DbgGetHexDump((uchar*)(LPCTSTR)_SearchExpr.m_aExpr[i], _SearchExpr.m_aExpr[i].GetLength()*sizeof(TCHAR)));
	//}

	// get total nr. of search terms
	int iTotalTerms = 0;
	if (!pParams->strExtension.IsEmpty())
		iTotalTerms++;
	if (pParams->uAvailability > 0)
		iTotalTerms++;
	if (pParams->ulMaxSize > 0)
		iTotalTerms++;
	if (pParams->ulMinSize > 0)
		iTotalTerms++;
	if (!strFileType.IsEmpty())
		iTotalTerms++;
	if (pParams->uComplete > 0)
		iTotalTerms++;
	if (pParams->ulMinBitrate > 0)
		iTotalTerms++;
	if (pParams->ulMinLength > 0)
		iTotalTerms++;
	if (!pParams->strCodec.IsEmpty())
		iTotalTerms++;
	if (!pParams->strTitle.IsEmpty())
		iTotalTerms++;
	if (!pParams->strAlbum.IsEmpty())
		iTotalTerms++;
	if (!pParams->strArtist.IsEmpty())
		iTotalTerms++;
	iTotalTerms += _SearchExpr.m_aExpr.GetCount();

	// create ed2k search expression
	bool bEd2k = (pParams->eType == SearchTypeEd2kServer || pParams->eType == SearchTypeEd2kGlobal);
	CSearchExprTarget target(pData, pParams->bUnicode ? utf8strRaw : utf8strNone);
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
				target.WriteBooleanAND();
			target.WriteMetaDataSearchParam(OptUtf8ToStr(_SearchExpr.m_aExpr[0]));
		}

		if (!strFileType.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteMetaDataSearchParamA(FT_FILETYPE, strFileType);
		}
		
		if (pParams->ulMinSize > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteOldMinMetaDataSearchParam(FT_FILESIZE, pParams->ulMinSize, bEd2k);
		}

		if (pParams->ulMaxSize > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteOldMaxMetaDataSearchParam(FT_FILESIZE, pParams->ulMaxSize, bEd2k);
		}
		
		if (pParams->uAvailability > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteOldMinMetaDataSearchParam(FT_SOURCES, pParams->uAvailability, bEd2k);
		}

		if (!pParams->strExtension.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, pParams->strExtension);
		}

		if (pParams->uComplete > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteOldMinMetaDataSearchParam(FT_COMPLETE_SOURCES, pParams->uComplete, bEd2k);
		}

		if (pParams->ulMinBitrate > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteOldMinMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_BITRATE : TAG_MEDIA_BITRATE, pParams->ulMinBitrate, bEd2k);
		}

		if (pParams->ulMinLength > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteOldMinMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_LENGTH : TAG_MEDIA_LENGTH, pParams->ulMinLength, bEd2k);
		}

		if (!pParams->strCodec.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_CODEC : TAG_MEDIA_CODEC, pParams->strCodec);
		}

		if (!pParams->strTitle.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_TITLE : TAG_MEDIA_TITLE, pParams->strTitle);
		}

		if (!pParams->strAlbum.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_ALBUM : TAG_MEDIA_ALBUM, pParams->strAlbum);
		}

		if (!pParams->strArtist.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
			target.WriteMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_ARTIST : TAG_MEDIA_ARTIST, pParams->strArtist);
		}

		ASSERT( iParameterCount == iTotalTerms );
	}
	else
	{
		if (!pParams->strExtension.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}

		if (pParams->uAvailability > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}
	  
		if (pParams->ulMaxSize > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}
        
		if (pParams->ulMinSize > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}
        
		if (!strFileType.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}
        
		if (pParams->uComplete > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}

		if (pParams->ulMinBitrate > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}

		if (pParams->ulMinLength > 0){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}

		if (!pParams->strCodec.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}

		if (!pParams->strTitle.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}

		if (!pParams->strAlbum.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}

		if (!pParams->strArtist.IsEmpty()){
			if (++iParameterCount < iTotalTerms)
				target.WriteBooleanAND();
		}

		ASSERT( iParameterCount + _SearchExpr.m_aExpr.GetCount() == iTotalTerms );

		for (int j = 0; j < _SearchExpr.m_aExpr.GetCount(); j++)
		{
			CStringA strA(_SearchExpr.m_aExpr[j]);
			if (strA == SEARCHOPTOK_AND)
				target.WriteBooleanAND();
			else if (strA == SEARCHOPTOK_OR)
				target.WriteBooleanOR();
			else if (strA == SEARCHOPTOK_NOT)
				target.WriteBooleanNOT();
			else
				target.WriteMetaDataSearchParam(OptUtf8ToStr(strA));
		}

		if (!strFileType.IsEmpty())
			target.WriteMetaDataSearchParamA(FT_FILETYPE, strFileType);

		if (pParams->ulMinSize > 0)
			target.WriteOldMinMetaDataSearchParam(FT_FILESIZE, pParams->ulMinSize, bEd2k);

		if (pParams->ulMaxSize > 0)
			target.WriteOldMaxMetaDataSearchParam(FT_FILESIZE, pParams->ulMaxSize, bEd2k);

		if (pParams->uAvailability > 0)
			target.WriteOldMinMetaDataSearchParam(FT_SOURCES, pParams->uAvailability, bEd2k);

		if (!pParams->strExtension.IsEmpty())
			target.WriteMetaDataSearchParam(FT_FILEFORMAT, pParams->strExtension);

		if (pParams->uComplete > 0)
			target.WriteOldMinMetaDataSearchParam(FT_COMPLETE_SOURCES, pParams->uComplete, bEd2k);

		if (pParams->ulMinBitrate > 0)
			target.WriteOldMinMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_BITRATE : TAG_MEDIA_BITRATE, pParams->ulMinBitrate, bEd2k);

		if (pParams->ulMinLength > 0)
			target.WriteOldMinMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_LENGTH : TAG_MEDIA_LENGTH, pParams->ulMinLength, bEd2k);

		if (!pParams->strCodec.IsEmpty())
			target.WriteMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_CODEC : TAG_MEDIA_CODEC, pParams->strCodec);

		if (!pParams->strTitle.IsEmpty())
			target.WriteMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_TITLE : TAG_MEDIA_TITLE, pParams->strTitle);

		if (!pParams->strAlbum.IsEmpty())
			target.WriteMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_ALBUM : TAG_MEDIA_ALBUM, pParams->strAlbum);

		if (!pParams->strArtist.IsEmpty())
			target.WriteMetaDataSearchParam(bEd2k ? FT_ED2K_MEDIA_ARTIST : TAG_MEDIA_ARTIST, pParams->strArtist);
	}

	if (thePrefs.GetDebugServerSearchesLevel() > 0)
		Debug(_T("Search Data: %s\n"), target.GetDebugString());
	_SearchExpr.m_aExpr.RemoveAll();
	return true;
}

bool CSearchResultsWnd::StartNewSearch(SSearchParams* pParams)
{
	ESearchType eSearchType = pParams->eType;

	if (eSearchType == SearchTypeEd2kServer || eSearchType == SearchTypeEd2kGlobal)
	{
		if (!theApp.serverconnect->IsConnected())
		{
			AfxMessageBox(GetResString(IDS_ERR_NOTCONNECTED), MB_ICONWARNING);
			delete pParams;
			return false;
		}

		try
		{
			if (!DoNewEd2kSearch(pParams))
			{
				delete pParams;
				return false;
			}
		}
		catch (CMsgBoxException* ex)
		{
			AfxMessageBox(ex->m_strMsg, ex->m_uType, ex->m_uHelpID);
			ex->Delete();
			delete pParams;
			return false;
		}

		CWnd* pWndFocus = GetFocus();
		m_pwndParams->m_ctlStart.EnableWindow(FALSE);
		if (pWndFocus && pWndFocus->m_hWnd  == m_pwndParams->m_ctlStart.m_hWnd)
			m_pwndParams->m_ctlName.SetFocus();
		m_pwndParams->m_ctlCancel.EnableWindow(TRUE);

		return true;
	}

	if (eSearchType == SearchTypeKademlia)
	{
		if (!Kademlia::CKademlia::isRunning() || !Kademlia::CKademlia::isConnected())
		{
			AfxMessageBox(GetResString(IDS_ERR_NOTCONNECTEDKAD), MB_ICONWARNING);
			delete pParams;
			return false;
		}

		try
		{
			if (!DoNewKadSearch(pParams))
			{
				delete pParams;
				return false;
			}
		}
		catch (CMsgBoxException* ex)
		{
			AfxMessageBox(ex->m_strMsg, ex->m_uType, ex->m_uHelpID);
			ex->Delete();
			delete pParams;
			return false;
		}

		return true;
	}

	ASSERT(0);
	delete pParams;
	return false;
}

bool CSearchResultsWnd::DoNewEd2kSearch(SSearchParams* pParams)
{
	if (!theApp.serverconnect->IsConnected())
		return false;

	CSafeMemFile data(100);
	if (!GetSearchPacket(&data, pParams) || data.GetLength() == 0)
		return false;

	CancelSearch();

	CStringA strResultType = pParams->strFileType;
	if (strResultType == ED2KFTSTR_PROGRAM)
		strResultType.Empty();
	m_nSearchID++;
	pParams->dwSearchID = m_nSearchID;
	theApp.searchlist->NewSearch(&searchlistctrl, strResultType, m_nSearchID);
	canceld = false;

	if (m_uTimerLocalServer){
		VERIFY( KillTimer(m_uTimerLocalServer) );
		m_uTimerLocalServer = 0;
	}

	// once we've sent a new search request, any previously received 'More' gets invalid.
	CWnd* pWndFocus = GetFocus();
	m_pwndParams->m_ctlMore.EnableWindow(FALSE);
	if (pWndFocus && pWndFocus->m_hWnd == m_pwndParams->m_ctlMore.m_hWnd)
		m_pwndParams->m_ctlCancel.SetFocus();
	m_iSentMoreReq = 0;

	Packet* packet = new Packet(&data);
	packet->opcode = OP_SEARCHREQUEST;
	if (thePrefs.GetDebugServerTCPLevel() > 0)
		Debug(_T(">>> Sending OP__SearchRequest\n"));
	theStats.AddUpDataOverheadServer(packet->size);
	theApp.serverconnect->SendPacket(packet,false);

	if (pParams->eType == SearchTypeEd2kGlobal && theApp.serverconnect->IsUDPSocketAvailable())
	{
		// set timeout timer for local server
		m_uTimerLocalServer = SetTimer(TimerServerTimeout, 50000, NULL);

		if (thePrefs.Score())
			theApp.serverlist->ResetSearchServerPos();

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
	
bool CSearchResultsWnd::SearchMore()
{
	if (!theApp.serverconnect->IsConnected())
		return false;

	SetActiveSearchResultsIcon(m_nSearchID);
	canceld = false;

	Packet* packet = new Packet();
	packet->opcode = OP_QUERY_MORE_RESULT;
	if (thePrefs.GetDebugServerTCPLevel() > 0)
		Debug(_T(">>> Sending OP__QueryMoreResults\n"));
	theStats.AddUpDataOverheadServer(packet->size);
	theApp.serverconnect->SendPacket(packet);
	m_iSentMoreReq++;
	return true;
}

bool CSearchResultsWnd::DoNewKadSearch(SSearchParams* pParams)
{
	if (!Kademlia::CKademlia::isConnected())
		return false;

	int iPos = 0;
	pParams->strKeyword = pParams->strExpression.Tokenize(_T(" "), iPos);
	pParams->strKeyword.Trim();
	if (pParams->strKeyword.IsEmpty() || pParams->strKeyword.FindOneOf(_aszInvKadKeywordChars) != -1){
		CString strError;
		strError.Format(GetResString(IDS_KAD_SEARCH_KEYWORD_INVALID), _aszInvKadKeywordChars);
		throw new CMsgBoxException(strError, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
	}

	CSafeMemFile data(100);
	if (!GetSearchPacket(&data, pParams)/* || (!pParams->strBooleanExpr.IsEmpty() && data.GetLength() == 0)*/)
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
		pSearch = Kademlia::CSearchManager::prepareFindKeywords(pParams->bUnicode, pParams->strKeyword, uSearchTermsSize, pSearchTermsData);
		delete pSearchTermsData;
		if (!pSearch){
			ASSERT(0);
			return false;
		}
	}
	catch (CString strException)
	{
		delete pSearchTermsData;
		throw new CMsgBoxException(strException, MB_ICONWARNING | MB_HELP, eMule_FAQ_Search - HID_BASE_PROMPT);
	}
	pParams->dwSearchID = pSearch->getSearchID();
	CStringA strResultType = pParams->strFileType;
	if (strResultType == ED2KFTSTR_PROGRAM)
		strResultType.Empty();
	theApp.searchlist->NewSearch(&searchlistctrl,strResultType,pParams->dwSearchID);
	CreateNewTab(pParams);
	return true;
}

bool CSearchResultsWnd::CreateNewTab(SSearchParams* pParams)
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
	TCITEM newitem;
	if (pParams->strExpression.IsEmpty())
		pParams->strExpression = _T("-");
	newitem.mask = TCIF_PARAM | TCIF_TEXT | TCIF_IMAGE;
	newitem.lParam = (LPARAM)pParams;
	CString label = pParams->strExpression + _T(" (0)");
	newitem.pszText = const_cast<LPTSTR>((LPCTSTR)label);
	newitem.cchTextMax = 0;
	if (pParams->bClientSharedFiles)
		newitem.iImage = sriClient;
	else if (pParams->eType == SearchTypeKademlia)
		newitem.iImage = sriKadActice;
	else if (pParams->eType == SearchTypeEd2kGlobal)
		newitem.iImage = sriGlobalActive;
	else{
		ASSERT( pParams->eType == SearchTypeEd2kServer );
		newitem.iImage = sriServerActive;
	}
	int itemnr = searchselect.InsertItem(INT_MAX, &newitem);
	if (!searchselect.IsWindowVisible())
		ShowSearchSelector(true);
	searchselect.SetCurSel(itemnr);
	searchlistctrl.ShowResults(pParams->dwSearchID);
	return true;
}

bool CSearchResultsWnd::CanDeleteSearch(uint32 nSearchID) const
{
	return (searchselect.GetItemCount() > 0);
}

void CSearchResultsWnd::DeleteSearch(uint32 nSearchID)
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
	if (nSearchID == m_nSearchID)
		m_pwndParams->m_ctlMore.EnableWindow(FALSE);
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
			if (searchselect.GetItem(iCurSel, &item) && item.lParam != NULL){
				searchselect.HighlightItem(iCurSel, FALSE);
				ShowResults((const SSearchParams*)item.lParam);
			}
		}
	}
	else{
		searchlistctrl.DeleteAllItems();
		ShowSearchSelector(false);
		searchlistctrl.NoTabs();
	}
}

bool CSearchResultsWnd::CanDeleteAllSearches() const
{
	return (searchselect.GetItemCount() > 0);
}

void CSearchResultsWnd::DeleteAllSearchs()
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
	m_pwndParams->m_ctlMore.EnableWindow(FALSE);
}

void CSearchResultsWnd::ShowResults(const SSearchParams* pParams)
{
	// restoring the params works and is nice during development/testing but pretty annoying in practice.
	// TODO: maybe it should be done explicitly via a context menu function or such.
	if (GetAsyncKeyState(VK_CONTROL) < 0)
		m_pwndParams->SetParameters(pParams);
	searchlistctrl.ShowResults(pParams->dwSearchID);
}

void CSearchResultsWnd::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	CWaitCursor curWait; // this may take a while
	int cur_sel = searchselect.GetCurSel();
	if (cur_sel == -1)
		return;
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (searchselect.GetItem(cur_sel, &item) && item.lParam != NULL)
	{
		searchselect.HighlightItem(cur_sel, FALSE);
		ShowResults((const SSearchParams*)item.lParam);
	}
	*pResult = 0;
}

LRESULT CSearchResultsWnd::OnCloseTab(WPARAM wParam, LPARAM lParam)
{
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (searchselect.GetItem((int)wParam, &item) && item.lParam != NULL)
	{
		int nSearchID = ((const SSearchParams*)item.lParam)->dwSearchID;
		if (!canceld && nSearchID == m_nSearchID)
			CancelSearch();
		DeleteSearch(nSearchID);
	}
	return TRUE;
}

LRESULT CSearchResultsWnd::OnDblClickTab(WPARAM wParam, LPARAM lParam)
{
	TCITEM item;
	item.mask = TCIF_PARAM;
	if (searchselect.GetItem((int)wParam, &item) && item.lParam != NULL)
	{
		m_pwndParams->SetParameters((const SSearchParams*)item.lParam);
	}
	return TRUE;
}

void CSearchResultsWnd::UpdateCatTabs() {
	int oldsel=m_cattabs.GetCurSel();
	m_cattabs.DeleteAllItems();
	for (int ix=0;ix<thePrefs.GetCatCount();ix++){
	//MORPH START - Changed by SiRoB, Selection category support
	/*
		CString label=(ix==0)?GetResString(IDS_ALL):thePrefs.GetCategory(ix)->title;
		label.Replace(_T("&"),_T("&&"));
		m_cattabs.InsertItem(ix,label);
	}
	if (oldsel>=m_cattabs.GetItemCount() || oldsel==-1)
		oldsel=0;
	*/
		CString label=thePrefs.GetCategory(ix)->title;
		label.Replace(_T("&"),_T("&&"));
		m_cattabs.InsertItem(ix,label);
	}
	if (oldsel>=m_cattabs.GetItemCount())
		oldsel=-1;
	//MORPH END   - Changed by SiRoB, Selection category support

	m_cattabs.SetCurSel(oldsel);
	int flag;
	flag=(m_cattabs.GetItemCount()>1) ? SW_SHOW:SW_HIDE;
	
	GetDlgItem(IDC_CATTAB2)->ShowWindow(flag);
	GetDlgItem(IDC_STATIC_DLTOof)->ShowWindow(flag);
}

CString	CSearchResultsWnd::ToQueryString(CString str){
	CString sTmp = URLEncode(str);
	sTmp.Replace(_T("%20"), _T("+"));
	return sTmp;
}

void CSearchResultsWnd::ShowSearchSelector(bool visible)
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
	AddAnchor(searchlistctrl, TOP_LEFT, BOTTOM_RIGHT);
	GetDlgItem(IDC_CLEARALL)->ShowWindow(visible ? SW_SHOW : SW_HIDE);
}

void CSearchResultsWnd::SaveSettings()
{
	searchlistctrl.SaveSettings(CPreferences::tableSearch);
}

void CSearchResultsWnd::OnDestroy()
{
	int iTabItems = searchselect.GetItemCount();
	for (int i = 0; i < iTabItems; i++){
		TCITEM tci;
		tci.mask = TCIF_PARAM;
		if (searchselect.GetItem(i, &tci) && tci.lParam != NULL){
			delete (SSearchParams*)tci.lParam;
		}
	}

	CResizableFormView::OnDestroy();
}

void CSearchResultsWnd::OnSize(UINT nType, int cx, int cy)
{
	CResizableFormView::OnSize(nType, cx, cy);
}

void CSearchResultsWnd::OnClose()
{
	// Do not pass the WM_CLOSE to the base class. Since we have a rich edit control *and* an attached auto complete
	// control, the WM_CLOSE will get generated by the rich edit control when user presses ESC while the auto complete
	// is open.
	//__super::OnClose();
}

BOOL CSearchResultsWnd::OnHelpInfo(HELPINFO* pHelpInfo)
{
	theApp.ShowHelp(eMule_FAQ_Search);
	return TRUE;
}

LRESULT CSearchResultsWnd::OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam)
{
	GetDlgItem(IDC_OPEN_PARAMS_WND)->ShowWindow( theApp.emuledlg->searchwnd->IsSearchParamsWndVisible() ? SW_HIDE : SW_SHOW );
	return 0;
}

void CSearchResultsWnd::OnBnClickedOpenParamsWnd()
{
	theApp.emuledlg->searchwnd->OpenParametersWnd();
}

void CSearchResultsWnd::OnSysCommand(UINT nID, LPARAM lParam)
{
	if (nID == SC_KEYMENU)
	{
		if (lParam == EMULE_HOTMENU_ACCEL)
			theApp.emuledlg->SendMessage(WM_COMMAND, IDC_HOTMENU);
		else
			theApp.emuledlg->SendMessage(WM_SYSCOMMAND, nID, lParam);
		return;
	}
	__super::OnSysCommand(nID, lParam);
}

//MORPH START - Added by SiRoB, Selection category support
void CSearchResultsWnd::OnNMClickCattab2(NMHDR *pNMHDR, LRESULT *pResult)
{
	POINT point;
	::GetCursorPos(&point);

	CPoint pt(point);
	TCHITTESTINFO hitinfo;
	CRect rect;
	m_cattabs.GetWindowRect(&rect);
	pt.Offset(0-rect.left,0-rect.top);
	hitinfo.pt = pt;

	// Find the destination tab...
	unsigned int nTab = m_cattabs.HitTest( &hitinfo );
	if( hitinfo.flags != TCHT_NOWHERE )
		if(nTab==m_cattabs.GetCurSel())
		{
			m_cattabs.DeselectAll(false);
		}
	*pResult = 0;
}
//MORPH END - Added by SiRoB, Selection category support
