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
#include "OtherFunctions.h"
#include "FileInfoDialog.h"
#include "FileDetailDialog.h"
#include "Preferences.h"
#include "UpDownClient.h"
#include "TitleMenu.h"
#include "MenuCmds.h"
#include "PartFile.h"
#include "StringConversion.h"
#include "shahashset.h"
#include "HighColorTab.hpp"
#include "UserMsgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#endif


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialog

LPCTSTR CFileDetailDialog::m_pPshStartPage;

IMPLEMENT_DYNAMIC(CFileDetailDialog, CListViewWalkerPropertySheet)

BEGIN_MESSAGE_MAP(CFileDetailDialog, CListViewWalkerPropertySheet)
	ON_WM_DESTROY()
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
END_MESSAGE_MAP()

CFileDetailDialog::CFileDetailDialog(const CSimpleArray<CPartFile*>* paFiles, UINT uPshInvokePage, CListCtrlItemWalk* pListCtrl)
	: CListViewWalkerPropertySheet(pListCtrl)
{
	m_uPshInvokePage = uPshInvokePage;
	for (int i = 0; i < paFiles->GetSize(); i++)
		m_aItems.Add((*paFiles)[i]);
	m_psh.dwFlags &= ~PSH_HASHELP;
	
	m_wndInfo.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndInfo.m_psp.dwFlags |= PSP_USEICONID;
	m_wndInfo.m_psp.pszIcon = _T("FILEINFO");
	m_wndInfo.SetFiles(&m_aItems);
	AddPage(&m_wndInfo);

	if (paFiles->GetSize() == 1)
	{
		m_wndName.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndName.m_psp.dwFlags |= PSP_USEICONID;
		m_wndName.m_psp.pszIcon = _T("FILERENAME");
		m_wndName.SetFiles(&m_aItems);
		AddPage(&m_wndName);

		m_wndComments.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndComments.m_psp.dwFlags |= PSP_USEICONID;
		m_wndComments.m_psp.pszIcon = _T("FILECOMMENTS");
		m_wndComments.SetFiles(&m_aItems);
		AddPage(&m_wndComments);
	}

	m_wndMediaInfo.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndMediaInfo.m_psp.dwFlags |= PSP_USEICONID;
	m_wndMediaInfo.m_psp.pszIcon = _T("MEDIAINFO");
	m_wndMediaInfo.SetFiles(&m_aItems);
	AddPage(&m_wndMediaInfo);

	if (paFiles->GetSize() == 1)
	{
		if (thePrefs.IsExtControlsEnabled()){
			m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;
			m_wndMetaData.m_psp.dwFlags |= PSP_USEICONID;
			m_wndMetaData.m_psp.pszIcon = _T("METADATA");
			m_wndMetaData.SetFiles(&m_aItems);
			AddPage(&m_wndMetaData);
		}
	}

	m_wndFileLink.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndFileLink.m_psp.dwFlags |= PSP_USEICONID;
	m_wndFileLink.m_psp.pszIcon = _T("ED2KLINK");
	m_wndFileLink.SetFiles(&m_aItems);
	AddPage(&m_wndFileLink);

	LPCTSTR pPshStartPage = m_pPshStartPage;
	if (m_uPshInvokePage != 0)
		pPshStartPage = MAKEINTRESOURCE(m_uPshInvokePage);
	for (int i = 0; i < m_pages.GetSize(); i++)
	{
		CPropertyPage* pPage = GetPage(i);
		if (pPage->m_psp.pszTemplate == pPshStartPage)
		{
			m_psh.nStartPage = i;
			break;
		}
	}
}

CFileDetailDialog::~CFileDetailDialog()
{
}

void CFileDetailDialog::OnDestroy()
{
	if (m_uPshInvokePage == 0)
		m_pPshStartPage = GetPage(GetActiveIndex())->m_psp.pszTemplate;
	CListViewWalkerPropertySheet::OnDestroy();
}

BOOL CFileDetailDialog::OnInitDialog()
{		
	EnableStackedTabs(FALSE);
	BOOL bResult = CListViewWalkerPropertySheet::OnInitDialog();
	HighColorTab::UpdateImageList(*this);
	InitWindowStyles(this);
	EnableSaveRestore(_T("FileDetailDialog")); // call this after(!) OnInitDialog
	UpdateTitle();
	return bResult;
	}

LRESULT CFileDetailDialog::OnDataChanged(WPARAM, LPARAM)
		{
	UpdateTitle();
	return 1;
	}

void CFileDetailDialog::UpdateTitle()
{
	if (m_aItems.GetSize() == 1)
		SetWindowText(GetResString(IDS_DETAILS) + _T(": ") + STATIC_DOWNCAST(CKnownFile, m_aItems[0])->GetFileName());
	else
		SetWindowText(GetResString(IDS_DETAILS));
}


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialogInfo dialog

LPCTSTR CFileDetailDialogInfo::sm_pszNotAvail = _T("-");

IMPLEMENT_DYNAMIC(CFileDetailDialogInfo, CResizablePage)

BEGIN_MESSAGE_MAP(CFileDetailDialogInfo, CResizablePage)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
END_MESSAGE_MAP()

CFileDetailDialogInfo::CFileDetailDialogInfo()
	: CResizablePage(CFileDetailDialogInfo::IDD, 0)
{
	m_paFiles = NULL;
	m_bDataChanged = false;
	m_strCaption = GetResString(IDS_FILEINFORMATION);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
	m_timer = 0;
}

CFileDetailDialogInfo::~CFileDetailDialogInfo()
{
}

void CFileDetailDialogInfo::OnTimer(UINT nIDEvent)
{
	RefreshData();
}

void CFileDetailDialogInfo::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
}

BOOL CFileDetailDialogInfo::OnInitDialog()
{
	CResizablePage::OnInitDialog();
	InitWindowStyles(this);
	AddAnchor(IDC_FD_X0, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FD_X6, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FD_X8, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_FNAME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_METFILE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FHASH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FSIZE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FD_AICHHASH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_PARTCOUNT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HASHSET, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMPLSIZE, TOP_LEFT, TOP_RIGHT);
//MORPH START - Changed by SiRoB, WebCache 1.2f
/*
	AddAnchor(IDC_DATARATE, TOP_LEFT, TOP_RIGHT);
*/
	AddAnchor(IDC_WCReq, TOP_LEFT, TOP_RIGHT); //JP
	AddAnchor(IDC_WCDOWNL, TOP_LEFT, TOP_RIGHT); //JP
//MORPH END   - Changed by SiRoB, WebCache 1.2f
	AddAnchor(IDC_SOURCECOUNT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_RECOVERED, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_FILECREATED, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_DL_ACTIVE_TIME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LASTSEENCOMPL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LASTRECEIVED, TOP_LEFT, TOP_RIGHT);

	Localize();

	// no need to explicitly call 'RefreshData' here, 'OnSetActive' will be called right after 'OnInitDialog'
	;

	// start time for calling 'RefreshData'
	VERIFY( (m_timer = SetTimer(301, 5000, 0)) != NULL );

	return TRUE;
}

BOOL CFileDetailDialogInfo::OnSetActive()
{
	if (!CResizablePage::OnSetActive())
		return FALSE;
	if (m_bDataChanged)
	{
		RefreshData();
		m_bDataChanged = false;
	}
	return TRUE;
}

LRESULT CFileDetailDialogInfo::OnDataChanged(WPARAM, LPARAM)
{
	m_bDataChanged = true;
	return 1;
}

void CFileDetailDialogInfo::RefreshData()
{
	CString str;

	if (m_paFiles->GetSize() == 1)
	{
		const CPartFile* file = STATIC_DOWNCAST(CPartFile, (*m_paFiles)[0]);

		// if file is completed, we output the 'file path' and not the 'part.met file path'
		if (file->GetStatus(true) == PS_COMPLETE)
			GetDlgItem(IDC_FD_X2)->SetWindowText(GetResString(IDS_DL_FILENAME));

		SetDlgItemText(IDC_FNAME, file->GetFileName());
		SetDlgItemText(IDC_METFILE, file->GetFullName());
		SetDlgItemText(IDC_FHASH, md4str(file->GetFileHash()));

		if (file->GetTransferringSrcCount() > 0)
			str.Format(GetResString(IDS_PARTINFOS2), file->GetTransferringSrcCount());
		else
			str = file->getPartfileStatus();
		SetDlgItemText(IDC_PFSTATUS, str);

		str.Format(_T("%u;  %s: %u (%.1f%%)"), file->GetPartCount(), GetResString(IDS_AVAILABLE) , file->GetAvailablePartCount(), (float)((file->GetAvailablePartCount()*100)/file->GetPartCount()));
		SetDlgItemText(IDC_PARTCOUNT, str);

		// date created
		if (file->GetCrFileDate() != 0){
			str.Format(_T("%s   ") + GetResString(IDS_TIMEBEFORE),
						file->GetCrCFileDate().Format(thePrefs.GetDateTimeFormat()),
						CastSecondsToLngHM(time(NULL) - file->GetCrFileDate()));
		}
		else
			str = GetResString(IDS_UNKNOWN);
		SetDlgItemText(IDC_FILECREATED, str);

		// active download time
		uint32 nDlActiveTime = file->GetDlActiveTime();
		if (nDlActiveTime)
			str = CastSecondsToLngHM(nDlActiveTime);
		else
			str = GetResString(IDS_UNKNOWN);
		SetDlgItemText(IDC_DL_ACTIVE_TIME, str);

		// last seen complete
		struct tm* ptimLastSeenComplete = file->lastseencomplete.GetLocalTm();
		if (file->lastseencomplete == NULL || ptimLastSeenComplete == NULL)
			str.Format(GetResString(IDS_NEVER));
		else{
			str.Format(_T("%s   ") + GetResString(IDS_TIMEBEFORE),
						file->lastseencomplete.Format(thePrefs.GetDateTimeFormat()),
						CastSecondsToLngHM(time(NULL) - safe_mktime(ptimLastSeenComplete)));
		}
		SetDlgItemText(IDC_LASTSEENCOMPL, str);

		// last receive
		if (file->GetFileDate() != 0 && file->GetRealFileSize() > 0)
		{
			// 'Last Modified' sometimes is up to 2 seconds greater than the current time ???
			// If it's related to the FAT32 seconds time resolution the max. failure should still be only 1 sec.
			// Happens at least on FAT32 with very high download speed.
			uint32 tLastModified = file->GetFileDate();
			uint32 tNow = time(NULL);
			uint32 tAgo;
			if (tNow >= tLastModified)
				tAgo = tNow - tLastModified;
			else{
				TRACE("tNow = %s\n", CTime(tNow).Format("%X"));
				TRACE("tLMd = %s, +%u\n", CTime(tLastModified).Format("%X"), tLastModified - tNow);
				TRACE("\n");
				tAgo = 0;
			}
			str.Format(_T("%s   ") + GetResString(IDS_TIMEBEFORE),
						file->GetCFileDate().Format(thePrefs.GetDateTimeFormat()),
						CastSecondsToLngHM(tAgo));
		}
		else
			str = GetResString(IDS_NEVER);
		SetDlgItemText(IDC_LASTRECEIVED, str);

		// AICH Hash
		switch(file->GetAICHHashset()->GetStatus()){
			case AICH_TRUSTED:
			case AICH_VERIFIED:
			case AICH_HASHSETCOMPLETE:
				if (file->GetAICHHashset()->HasValidMasterHash()){
					SetDlgItemText(IDC_FD_AICHHASH, file->GetAICHHashset()->GetMasterHash().GetString());
					break;
				}
			default:
				SetDlgItemText(IDC_FD_AICHHASH, GetResString(IDS_UNKNOWN));
		}
	}
	else
	{
		SetDlgItemText(IDC_FNAME, sm_pszNotAvail);
		SetDlgItemText(IDC_METFILE, sm_pszNotAvail);
		SetDlgItemText(IDC_FHASH, sm_pszNotAvail);

		SetDlgItemText(IDC_PFSTATUS, sm_pszNotAvail);
		SetDlgItemText(IDC_PARTCOUNT, sm_pszNotAvail);

		SetDlgItemText(IDC_FILECREATED, sm_pszNotAvail);
		SetDlgItemText(IDC_DL_ACTIVE_TIME, sm_pszNotAvail);
		SetDlgItemText(IDC_LASTSEENCOMPL, sm_pszNotAvail);
		SetDlgItemText(IDC_LASTRECEIVED, sm_pszNotAvail);
		SetDlgItemText(IDC_FD_AICHHASH, sm_pszNotAvail);
	}

	uint64 uFileSize = 0;
	uint64 uRealFileSize = 0;
	uint64 uTransferred = 0;
	uint64 uCorrupted = 0;
	uint64 uRecovered = 0;
	uint64 uCompression = 0;
	uint64 uCompleted = 0;
	int iHashsetAvailable = 0;
	uint32 uDataRate = 0;
	UINT uSources = 0;
	UINT uValidSources = 0;
	UINT uNNPSources = 0;
	UINT uA4AFSources = 0;
	//MORPH START - Changed by SiRoB, WebCache 1.2f
	uint32	uWebcacherequests = 0; //JP webcache
	uint32	uSuccessfulWebcacherequests = 0;//jp webcache
	uint32  uWebcachedownloaded = 0;//jp webcache
	//MORPH END   - Changed by SiRoB, WebCache 1.2f

	for (int i = 0; i < m_paFiles->GetSize(); i++)
	{
		const CPartFile* file = STATIC_DOWNCAST(CPartFile, (*m_paFiles)[i]);

		uFileSize += file->GetFileSize();
		uRealFileSize += file->GetRealFileSize();
		uTransferred += file->GetTransferred();
		uCorrupted += file->GetCorruptionLoss();
		uRecovered += file->GetRecoveredPartsByICH();
		uCompression += file->GetCompressionGain();
		uDataRate += file->GetDatarate();
		uCompleted += file->GetCompletedSize();
		iHashsetAvailable += (file->GetHashCount() == file->GetED2KPartCount()) ? 1 : 0;	// SLUGFILLER: SafeHash - use GetED2KPartCount

		//MORPH START - Changed by SiRoB, WebCache 1.2f
		uWebcacherequests += file->Webcacherequests;//jp webcache
		uSuccessfulWebcacherequests += file->SuccessfulWebcacherequests;//jp webcache
		uWebcachedownloaded += file->WebCacheDownDataThisFile;//jp webcache
		//MORPH END   - Changed by SiRoB, WebCache 1.2f

		if (file->IsPartFile())
		{
			uSources += file->GetSourceCount();
			uValidSources += file->GetValidSourcesCount();
			uNNPSources += file->GetSrcStatisticsValue(DS_NONEEDEDPARTS);
			uA4AFSources += file->GetSrcA4AFCount();
		}
	}

	if (iHashsetAvailable == 0)
		SetDlgItemText(IDC_HASHSET, GetResString(IDS_NO));
	else if (iHashsetAvailable == m_paFiles->GetSize())
		SetDlgItemText(IDC_HASHSET, GetResString(IDS_YES));
	else
		SetDlgItemText(IDC_HASHSET, _T(""));

	str.Format(_T("%s (%.1f%%)"), CastItoXBytes(uCompleted, false, false), uFileSize!=0 ? (uCompleted * 100.0 / uFileSize) : 0.0);
	SetDlgItemText(IDC_COMPLSIZE, str);

	str.Format(_T("%s"), CastItoXBytes(uDataRate, false, true));
	SetDlgItemText(IDC_DATARATE, str);

	str.Format(_T("%s  (%s %s);  %s %s"), CastItoXBytes(uFileSize, false, false), GetFormatedUInt64(uFileSize), GetResString(IDS_BYTES), GetResString(IDS_ONDISK), CastItoXBytes(uRealFileSize, false, false));
	SetDlgItemText(IDC_FSIZE, str);

	SetDlgItemText(IDC_TRANSFERRED, CastItoXBytes(uTransferred, false, false));

	str.Format(_T("%s (%.1f%%)"), CastItoXBytes(uCorrupted, false, false), uTransferred!=0 ? (uCorrupted * 100.0 / uTransferred) : 0.0);
	SetDlgItemText(IDC_CORRUPTED, str);

	str.Format(_T("%i "),uRecovered);
	str.Append(GetResString(IDS_FD_PARTS));
	SetDlgItemText(IDC_RECOVERED, str);

	str.Format(_T("%s (%.1f%%)"), CastItoXBytes(uCompression, false, false), uTransferred!=0 ? (uCompression * 100.0 / uTransferred) : 0.0);
	SetDlgItemText(IDC_COMPRESSION, str);

	str.Format(GetResString(IDS_SOURCESINFO), uSources, uValidSources, uNNPSources, uA4AFSources);
	SetDlgItemText(IDC_SOURCECOUNT, str);

	//MORPH START - Changed by SiRoB, WebCache 1.2f
	double percentSessions = 0;
	if (uWebcacherequests != 0)
		percentSessions = (double) 100 * uSuccessfulWebcacherequests / uWebcacherequests;
	str.Format( _T("%u/%u (%1.1f%%)"), uSuccessfulWebcacherequests, uWebcacherequests, percentSessions );
	SetDlgItemText(IDC_WCReq, str);
	SetDlgItemText(IDC_WCDOWNL, CastItoXBytes(uWebcachedownloaded, false, false));
	//MORPH END   - Changed by SiRoB, WebCache 1.2f
}

void CFileDetailDialogInfo::OnDestroy()
{
	if (m_timer){
		KillTimer(m_timer);
		m_timer = 0;
	}
}

void CFileDetailDialogInfo::Localize()
{
	GetDlgItem(IDC_FD_X0)->SetWindowText(GetResString(IDS_FD_GENERAL));
	GetDlgItem(IDC_FD_X1)->SetWindowText(GetResString(IDS_SW_NAME)+_T(':'));
	GetDlgItem(IDC_FD_X2)->SetWindowText(GetResString(IDS_FD_MET));
	GetDlgItem(IDC_FD_X3)->SetWindowText(GetResString(IDS_FD_HASH));
	GetDlgItem(IDC_FD_X4)->SetWindowText(GetResString(IDS_DL_SIZE)+_T(':'));
	GetDlgItem(IDC_FD_X9)->SetWindowText(GetResString(IDS_FD_PARTS)+_T(':'));
	GetDlgItem(IDC_FD_X5)->SetWindowText(GetResString(IDS_STATUS)+_T(':'));
	GetDlgItem(IDC_FD_X6)->SetWindowText(GetResString(IDS_FD_TRANSFER));
	GetDlgItem(IDC_FD_X7)->SetWindowText(GetResString(IDS_DL_SOURCES)+_T(':'));
	GetDlgItem(IDC_FD_X14)->SetWindowText(GetResString(IDS_FD_TRANS));
	GetDlgItem(IDC_FD_X12)->SetWindowText(GetResString(IDS_FD_COMPSIZE));
	GetDlgItem(IDC_FD_X13)->SetWindowText(GetResString(IDS_FD_DATARATE));
	GetDlgItem(IDC_FD_X15)->SetWindowText(GetResString(IDS_LASTSEENCOMPL));
	GetDlgItem(IDC_FD_LASTCHANGE)->SetWindowText(GetResString(IDS_FD_LASTCHANGE));
	GetDlgItem(IDC_FD_X8)->SetWindowText(GetResString(IDS_FD_TIMEDATE));
	GetDlgItem(IDC_FD_X16)->SetWindowText(GetResString(IDS_FD_DOWNLOADSTARTED));
	GetDlgItem(IDC_DL_ACTIVE_TIME_LBL)->SetWindowText(GetResString(IDS_DL_ACTIVE_TIME)+_T(':'));
	GetDlgItem(IDC_HSAV)->SetWindowText(GetResString(IDS_HSAV)+_T(':'));
	GetDlgItem(IDC_FD_CORR)->SetWindowText(GetResString(IDS_FD_CORR)+_T(':'));
	GetDlgItem(IDC_FD_RECOV)->SetWindowText(GetResString(IDS_FD_RECOV)+_T(':'));
	GetDlgItem(IDC_FD_COMPR)->SetWindowText(GetResString(IDS_FD_COMPR)+_T(':'));
	GetDlgItem(IDC_FD_XAICH)->SetWindowText(GetResString(IDS_IACHHASH)+_T(':'));
    GetDlgItem(IDC_WCDOWNL)->SetWindowText(GetResString(IDS_WCDOWNL));
	GetDlgItem(IDC_WC_REQ_SUCC)->SetWindowText(GetResString(IDS_WC_REQ_SUCC));
    GetDlgItem(IDC_WC_DOWNLOADED)->SetWindowText(GetResString(IDS_WC_DOWNLOADED));
}


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialogName dialog

IMPLEMENT_DYNAMIC(CFileDetailDialogName, CResizablePage)

BEGIN_MESSAGE_MAP(CFileDetailDialogName, CResizablePage)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTONSTRIP, OnBnClickedButtonStrip)
	ON_BN_CLICKED(IDC_TAKEOVER, TakeOver)	
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LISTCTRLFILENAMES, OnLvnColumnclick)
	ON_NOTIFY(NM_DBLCLK, IDC_LISTCTRLFILENAMES, OnNMDblclkList)
	ON_NOTIFY(NM_RCLICK, IDC_LISTCTRLFILENAMES, OnNMRclickList)
	ON_MESSAGE(UM_DATA_CHANGED, OnDataChanged)
	ON_EN_CHANGE(IDC_FILENAME, OnEnChangeFilename)
END_MESSAGE_MAP()

CFileDetailDialogName::CFileDetailDialogName()
	: CResizablePage(CFileDetailDialogName::IDD, 0)
{
	m_paFiles = NULL;
	m_bDataChanged = false;
	m_strCaption = GetResString(IDS_DL_FILENAME);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
	m_timer = 0;
	memset(m_aiColWidths, 0, sizeof m_aiColWidths);
	m_bAppliedSystemImageList = false;
	m_sortorder=0;
	m_sortindex=1;
	m_bSelf = false;
}

CFileDetailDialogName::~CFileDetailDialogName()
{
}

void CFileDetailDialogName::OnTimer(UINT nIDEvent)
{
	RefreshData();
}

void CFileDetailDialogName::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LISTCTRLFILENAMES, m_listFileNames);
}

BOOL CFileDetailDialogName::OnInitDialog()
{
	CResizablePage::OnInitDialog();
	InitWindowStyles(this);

	AddAnchor(IDC_FD_SN, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LISTCTRLFILENAMES, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TAKEOVER, BOTTOM_LEFT);
	AddAnchor(IDC_BUTTONSTRIP, BOTTOM_RIGHT);
	AddAnchor(IDC_FILENAME, BOTTOM_LEFT, BOTTOM_RIGHT);

	m_listFileNames.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP);
	m_listFileNames.InsertColumn(0, GetResString(IDS_DL_FILENAME), LVCFMT_LEFT, 380); 
	m_listFileNames.InsertColumn(1, GetResString(IDS_DL_SOURCES), LVCFMT_LEFT, 80); 
	ASSERT( (m_listFileNames.GetStyle() & LVS_SHAREIMAGELISTS) != 0 );
	m_listFileNames.LoadSettings(CPreferences::tableFilenames);

	m_sortindex = thePrefs.GetColumnSortItem(CPreferences::tableFilenames);
	m_sortorder = thePrefs.GetColumnSortAscending(CPreferences::tableFilenames);

	m_listFileNames.SetSortArrow(m_sortindex, m_sortorder);
	m_listFileNames.SortItems(&CompareListNameItems, m_sortindex + ( (m_sortorder) ? 0:10) );

	Localize();

	// no need to explicitly call 'RefreshData' here, 'OnSetActive' will be called right after 'OnInitDialog'
	;

	// start time for calling 'RefreshData'
	VERIFY( (m_timer = SetTimer(301, 5000, 0)) != NULL );

	return TRUE;
}

BOOL CFileDetailDialogName::OnSetActive()
{
	if (!CResizablePage::OnSetActive())
		return FALSE;
	if (m_bDataChanged)
	{
		m_bSelf = true;
		GetDlgItem(IDC_FILENAME)->SetWindowText(STATIC_DOWNCAST(CPartFile, (*m_paFiles)[0])->GetFileName());
		m_bSelf = false;
		RefreshData();
		m_bDataChanged = false;
	}
	return TRUE;
}

LRESULT CFileDetailDialogName::OnDataChanged(WPARAM, LPARAM)
{
	m_bDataChanged = true;
	return 1;
}

void CFileDetailDialogName::RefreshData()
{
	bool bEnableRename = CanRenameFile();
	GetDlgItem(IDC_FILENAME)->EnableWindow(bEnableRename);
	GetDlgItem(IDC_BUTTONSTRIP)->EnableWindow(bEnableRename);
	GetDlgItem(IDC_TAKEOVER)->EnableWindow(bEnableRename);

	FillSourcenameList();
}

void CFileDetailDialogName::OnDestroy()
{
	m_listFileNames.SaveSettings(CPreferences::tableFilenames);

	for (int i=0;i<m_listFileNames.GetItemCount();++i) {
		FCtrlItem_Struct* item= (FCtrlItem_Struct*)m_listFileNames.GetItemData(i);
		delete item;
	}

	if (m_timer){
		KillTimer(m_timer);
		m_timer = 0;
	}
}

void CFileDetailDialogName::Localize()
{
	if (thePrefs.GetLanguageID() != MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT)){
	    GetDlgItem(IDC_TAKEOVER)->SetWindowText(GetResString(IDS_TAKEOVER));
	    GetDlgItem(IDC_BUTTONSTRIP)->SetWindowText(GetResString(IDS_CLEANUP));
	    GetDlgItem(IDC_FD_SN)->SetWindowText(GetResString(IDS_SOURCENAMES));
    }
}

void CFileDetailDialogName::FillSourcenameList()
{
	LVFINDINFO info; 
	info.flags = LVFI_STRING; 
	int itempos; 

	CString strText; 

	// reset
	for (int i=0;i<m_listFileNames.GetItemCount();i++){
		FCtrlItem_Struct* item= (FCtrlItem_Struct*)m_listFileNames.GetItemData(i);
		item->count=0;
	}

	// update
	const CPartFile* file = STATIC_DOWNCAST(CPartFile, (*m_paFiles)[0]);
	for (POSITION pos = file->srclist.GetHeadPosition(); pos != NULL; )
	{ 
		CUpDownClient* cur_src = file->srclist.GetNext(pos); 
		if (cur_src->GetRequestFile() != file || cur_src->GetClientFilename().GetLength()==0)
			continue;

		info.psz = cur_src->GetClientFilename(); 
		if ((itempos=m_listFileNames.FindItem(&info, -1)) == -1)
		{ 
			FCtrlItem_Struct* newitem= new FCtrlItem_Struct();
			newitem->count=1;
			newitem->filename=cur_src->GetClientFilename();

			int iSystemIconIdx = theApp.GetFileTypeSystemImageIdx(cur_src->GetClientFilename());
			if (theApp.GetSystemImageList() && !m_bAppliedSystemImageList)
			{
				m_listFileNames.ApplyImageList(theApp.GetSystemImageList());
				ASSERT( (m_listFileNames.GetStyle() & LVS_SHAREIMAGELISTS) != 0 );
				m_bAppliedSystemImageList = true;
			}

			int ix=m_listFileNames.InsertItem(LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE, m_listFileNames.GetItemCount() ,cur_src->GetClientFilename(),0,0,iSystemIconIdx,(LPARAM)newitem);
			m_listFileNames.SetItemText(ix, 1, _T("1")); 
		}
		else
		{
			FCtrlItem_Struct* item= (FCtrlItem_Struct*)m_listFileNames.GetItemData(itempos);
			item->count+=1;
			strText.Format(_T("%i"),item->count);
			m_listFileNames.SetItemText(itempos, 1,strText ); 
		} 
	} 

	// remove 0'er
	for (int i=0;i<m_listFileNames.GetItemCount();i++)
	{
		FCtrlItem_Struct* item= (FCtrlItem_Struct*)m_listFileNames.GetItemData(i);
		if (item && item->count==0)
		{
			delete item;
			m_listFileNames.DeleteItem(i);
			i=0;
		}
	}

	m_listFileNames.SortItems(&CompareListNameItems, m_sortindex + ( (m_sortorder) ? 0:10) );
}

void CFileDetailDialogName::TakeOver()
{
	int iSel = m_listFileNames.GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1)
		GetDlgItem(IDC_FILENAME)->SetWindowText(m_listFileNames.GetItemText(iSel, 0));
}

void CFileDetailDialogName::Copy()
{
	int iSel = m_listFileNames.GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1)
		theApp.CopyTextToClipboard(m_listFileNames.GetItemText(iSel, 0));
}

void CFileDetailDialogName::OnBnClickedButtonStrip()
{
	CString filename;
	GetDlgItem(IDC_FILENAME)->GetWindowText(filename);
	GetDlgItem(IDC_FILENAME)->SetWindowText(CleanupFilename(filename));
}

void CFileDetailDialogName::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	if (m_sortindex != pNMLV->iSubItem)
		m_sortorder = 1;
	else
		m_sortorder = !m_sortorder;
	m_sortindex=pNMLV->iSubItem;

	m_listFileNames.SetSortArrow(m_sortindex, m_sortorder);
	m_listFileNames.SortItems(&CompareListNameItems, m_sortindex + (m_sortorder ? 0 : 10));

	*pResult = 0;
}

int CALLBACK CFileDetailDialogName::CompareListNameItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	FCtrlItem_Struct* item1=(FCtrlItem_Struct*) lParam1;
	FCtrlItem_Struct* item2=(FCtrlItem_Struct*) lParam2;
	switch (lParamSort){
		case 0:
			return CompareLocaleStringNoCase(item1->filename, item2->filename);
		case 10:
			return CompareLocaleStringNoCase(item2->filename, item1->filename);
		case 1:
			return (item1->count - item2->count);
		case 11:
			return (item2->count - item1->count);
	}
	return 0;
} 

void CFileDetailDialogName::OnNMDblclkList(NMHDR *pNMHDR, LRESULT *pResult)
{
	TakeOver();
	*pResult = 0;
}

void CFileDetailDialogName::OnNMRclickList(NMHDR *pNMHDR, LRESULT *pResult)
{
	UINT flag = MF_STRING;
	if (m_listFileNames.GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED) == -1)
		flag = MF_GRAYED;

	POINT point;
	::GetCursorPos(&point);
	CTitleMenu popupMenu;
	popupMenu.CreatePopupMenu();
	popupMenu.AppendMenu(flag,MP_MESSAGE, GetResString(IDS_TAKEOVER));
	popupMenu.AppendMenu(flag,MP_COPYSELECTED, GetResString(IDS_COPY));
	popupMenu.AppendMenu(MF_STRING,MP_RESTORE, GetResString(IDS_SV_UPDATE));
	popupMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( popupMenu.DestroyMenu() );

	*pResult = 0;
}

BOOL CFileDetailDialogName::OnCommand(WPARAM wParam,LPARAM lParam )
{
	int iSel = m_listFileNames.GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1)
	{
		switch (wParam)
		{
		case MP_MESSAGE:
			TakeOver();
			return true;
		case MP_COPYSELECTED:
			Copy();
			return true;
		case MP_RESTORE:
			FillSourcenameList();
			return true;
		}
	}
	return CResizablePage::OnCommand(wParam, lParam);
}

void CFileDetailDialogName::RenameFile()
{
	if (CanRenameFile())
	{
		CString strNewFileName;
		GetDlgItem(IDC_FILENAME)->GetWindowText(strNewFileName);
		strNewFileName.Trim();
		if (strNewFileName.IsEmpty() || !IsValidEd2kString(strNewFileName))
			return;
		CPartFile* file = STATIC_DOWNCAST(CPartFile, (*m_paFiles)[0]);
		file->SetFileName(strNewFileName, true);
		file->UpdateDisplayedInfo();
		file->SavePartFile();
	}
}

bool CFileDetailDialogName::CanRenameFile() const
{
	const CPartFile* file = STATIC_DOWNCAST(CPartFile, (*m_paFiles)[0]);
	return (file->GetStatus() != PS_COMPLETE && file->GetStatus() != PS_COMPLETING);
}

void CFileDetailDialogName::OnEnChangeFilename()
{
	if (!m_bSelf)
		SetModified();
}

BOOL CFileDetailDialogName::OnApply()
{
	if (!m_bDataChanged)
		RenameFile();
	return CResizablePage::OnApply();
}
