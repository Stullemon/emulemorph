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

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialog

LPCTSTR CFileDetailDialog::m_pPshStartPage;

IMPLEMENT_DYNAMIC(CFileDetailDialog, CResizableSheet)

BEGIN_MESSAGE_MAP(CFileDetailDialog, CResizableSheet)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CFileDetailDialog::CFileDetailDialog(const CSimpleArray<CPartFile*>* paFiles, EInvokePage eInvokePage)
{
	m_file = (*paFiles)[0];
	for (int i = 0; i < paFiles->GetSize(); i++)
		m_aKnownFiles.Add((*paFiles)[i]);
	m_psh.dwFlags &= ~PSH_HASHELP;
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	
	m_wndInfo.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndInfo.SetMyfile(paFiles);
	AddPage(&m_wndInfo);

	if (paFiles->GetSize() == 1)
	{
		m_wndName.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndName.SetMyfile(m_file);
		AddPage(&m_wndName);

		m_wndComments.m_psp.dwFlags &= ~PSP_HASHELP;
		m_wndComments.SetMyfile(m_file);
		AddPage(&m_wndComments);
	}

	m_wndVideo.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndVideo.SetMyfile(&m_aKnownFiles);
	AddPage(&m_wndVideo);

	if (paFiles->GetSize() == 1)
	{
		if (thePrefs.IsExtControlsEnabled()){
			m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;
			m_wndMetaData.SetFile(m_file);
			AddPage(&m_wndMetaData);
		}
	}

	m_wndFileLink.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndFileLink.SetMyfile(&m_aKnownFiles);
	AddPage(&m_wndFileLink);


	m_eInvokePage = eInvokePage;
}

CFileDetailDialog::~CFileDetailDialog()
{
}

void CFileDetailDialog::OnDestroy()
{
	if (m_eInvokePage == INP_NONE)
		m_pPshStartPage = GetPage(GetActiveIndex())->m_psp.pszTemplate;
	CResizableSheet::OnDestroy();
}

BOOL CFileDetailDialog::OnInitDialog()
{		
	EnableStackedTabs(FALSE);
	BOOL bResult = CResizableSheet::OnInitDialog();
	InitWindowStyles(this);
	EnableSaveRestore(_T("FileDetailDialog")); // call this after(!) OnInitDialog
	SetWindowText(GetResString(IDS_FD_TITLE));

	LPCTSTR pPshStartPage = m_pPshStartPage;
	switch(m_eInvokePage){
		case INP_COMMENTPAGE:
			pPshStartPage = MAKEINTRESOURCE(IDD_COMMENTLST);
			break;
		case INP_LINKPAGE:
			pPshStartPage = MAKEINTRESOURCE(IDD_ED2KLINK);
			break;
		case INP_NONE:
			break;
		default:
			ASSERT ( false );
	}

	for (int i = 0; i < m_pages.GetSize(); i++)
	{
		CPropertyPage* pPage = GetPage(i);
		if (pPage->m_psp.pszTemplate == pPshStartPage)
		{
			SetActivePage(i);
			break;
		}
	}

	return bResult;
}


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialogInfo dialog

LPCTSTR CFileDetailDialogInfo::sm_pszNotAvail = _T("-");

IMPLEMENT_DYNAMIC(CFileDetailDialogInfo, CResizablePage)

BEGIN_MESSAGE_MAP(CFileDetailDialogInfo, CResizablePage)
	ON_WM_TIMER()
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CFileDetailDialogInfo::CFileDetailDialogInfo()
	: CResizablePage(CFileDetailDialogInfo::IDD, 0)
{
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
	AddAnchor(IDC_DATARATE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SOURCECOUNT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_RECOVERED, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_FILECREATED, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_DL_ACTIVE_TIME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LASTSEENCOMPL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LASTRECEIVED, TOP_LEFT, TOP_RIGHT);

	Localize();

	// properties which won't change during the dialog is open - no need to fresh them and mess with the focus
	if (m_paFiles->GetSize() == 1)
	{
		// if file is completed, we output the 'file path' and not the 'part.met file path'
		if ((*m_paFiles)[0]->GetStatus(true) == PS_COMPLETE)
			GetDlgItem(IDC_FD_X2)->SetWindowText(GetResString(IDS_DL_FILENAME));

		SetDlgItemText(IDC_FNAME, (*m_paFiles)[0]->GetFileName());
		SetDlgItemText(IDC_METFILE, (*m_paFiles)[0]->GetFullName());
		SetDlgItemText(IDC_FHASH, md4str((*m_paFiles)[0]->GetFileHash()));
	}
	else
	{
		SetDlgItemText(IDC_FNAME, sm_pszNotAvail);
		SetDlgItemText(IDC_METFILE, sm_pszNotAvail);
		SetDlgItemText(IDC_FHASH, sm_pszNotAvail);
	}

	RefreshData();
	VERIFY( (m_timer = SetTimer(301, 5000, 0)) != NULL );

	return true;
}

BOOL CFileDetailDialogInfo::OnSetActive()
{
	if (!CResizablePage::OnSetActive())
		return FALSE;
	if (m_paFiles->GetSize() == 1)
		SetDlgItemText(IDC_FNAME, (*m_paFiles)[0]->GetFileName());
	return TRUE;
}

void CFileDetailDialogInfo::RefreshData()
{
	CString str;

	if (m_paFiles->GetSize() == 1)
	{
		if ((*m_paFiles)[0]->GetTransferingSrcCount() > 0)
			str.Format(GetResString(IDS_PARTINFOS2), (*m_paFiles)[0]->GetTransferingSrcCount());
		else
			str = (*m_paFiles)[0]->getPartfileStatus();
		SetDlgItemText(IDC_PFSTATUS, str);

		str.Format(_T("%u;  %s: %u (%.1f%%)"), (*m_paFiles)[0]->GetPartCount(), GetResString(IDS_AVAILABLE) , (*m_paFiles)[0]->GetAvailablePartCount(), (float)(((*m_paFiles)[0]->GetAvailablePartCount()*100)/(*m_paFiles)[0]->GetPartCount()));
		SetDlgItemText(IDC_PARTCOUNT, str);

		// date created
		if ((*m_paFiles)[0]->GetCrFileDate() != 0){
			str.Format(_T("%s   ") + GetResString(IDS_TIMEBEFORE),
						(*m_paFiles)[0]->GetCrCFileDate().Format(thePrefs.GetDateTimeFormat()),
						CastSecondsToLngHM(time(NULL) - (*m_paFiles)[0]->GetCrFileDate()));
		}
		else
			str = GetResString(IDS_UNKNOWN);
		SetDlgItemText(IDC_FILECREATED, str);

		// active download time
		uint32 nDlActiveTime = (*m_paFiles)[0]->GetDlActiveTime();
		if (nDlActiveTime)
			str = CastSecondsToLngHM(nDlActiveTime);
		else
			str = GetResString(IDS_UNKNOWN);
		SetDlgItemText(IDC_DL_ACTIVE_TIME, str);

		// last seen complete
		struct tm* ptimLastSeenComplete = (*m_paFiles)[0]->lastseencomplete.GetLocalTm();
		if ((*m_paFiles)[0]->lastseencomplete == NULL || ptimLastSeenComplete == NULL)
			str.Format(GetResString(IDS_UNKNOWN));
		else{
			str.Format(_T("%s   ") + GetResString(IDS_TIMEBEFORE),
						(*m_paFiles)[0]->lastseencomplete.Format(thePrefs.GetDateTimeFormat()),
						CastSecondsToLngHM(time(NULL) - safe_mktime(ptimLastSeenComplete)));
		}
		SetDlgItemText(IDC_LASTSEENCOMPL, str);

		// last receive
		if ((*m_paFiles)[0]->GetFileDate() != 0 && (*m_paFiles)[0]->GetRealFileSize() > 0)
		{
			// 'Last Modified' sometimes is up to 2 seconds greater than the current time ???
			// If it's related to the FAT32 seconds time resolution the max. failure should still be only 1 sec.
			// Happens at least on FAT32 with very high download speed.
			uint32 tLastModified = (*m_paFiles)[0]->GetFileDate();
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
						(*m_paFiles)[0]->GetCFileDate().Format(thePrefs.GetDateTimeFormat()),
						CastSecondsToLngHM(tAgo));
		}
		else
			str = GetResString(IDS_UNKNOWN);
		SetDlgItemText(IDC_LASTRECEIVED, str);

		// AICH Hash
		switch((*m_paFiles)[0]->GetAICHHashset()->GetStatus()){
			case AICH_TRUSTED:
			case AICH_VERIFIED:
			case AICH_HASHSETCOMPLETE:
				if ((*m_paFiles)[0]->GetAICHHashset()->HasValidMasterHash()){
					SetDlgItemText(IDC_FD_AICHHASH, (*m_paFiles)[0]->GetAICHHashset()->GetMasterHash().GetString());
					break;
				}
			default:
				SetDlgItemText(IDC_FD_AICHHASH, GetResString(IDS_UNKNOWN));
		}

	}
	else
	{
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
	uint64 uTransfered = 0;
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
	for (int i = 0; i < m_paFiles->GetSize(); i++)
	{
		uFileSize += (*m_paFiles)[i]->GetFileSize();
		uRealFileSize += (*m_paFiles)[i]->GetRealFileSize();
		uTransfered += (*m_paFiles)[i]->GetTransfered();
		uCorrupted += (*m_paFiles)[i]->GetLostDueToCorruption();
		uRecovered += (*m_paFiles)[i]->TotalPacketsSavedDueToICH();
		uCompression += (*m_paFiles)[i]->GetGainDueToCompression();
		uDataRate += (*m_paFiles)[i]->GetDatarate();
		uCompleted += (*m_paFiles)[i]->GetCompletedSize();
		iHashsetAvailable += ((*m_paFiles)[i]->GetHashCount() == (*m_paFiles)[i]->GetED2KPartHashCount()) ? 1 : 0;

		if ((*m_paFiles)[i]->IsPartFile())
		{
			uSources += (*m_paFiles)[i]->GetSourceCount();
			uValidSources += (*m_paFiles)[i]->GetValidSourcesCount();
			uNNPSources += (*m_paFiles)[i]->GetSrcStatisticsValue(DS_NONEEDEDPARTS);
			uA4AFSources += (*m_paFiles)[i]->GetSrcA4AFCount();
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

	SetDlgItemText(IDC_TRANSFERED, CastItoXBytes(uTransfered, false, false));

	str.Format(_T("%s (%.1f%%)"), CastItoXBytes(uCorrupted, false, false), uTransfered!=0 ? (uCorrupted * 100.0 / uTransfered) : 0.0);
	SetDlgItemText(IDC_CORRUPTED, str);

	str.Format(_T("%i "),uRecovered);
	str.Append(GetResString(IDS_FD_PARTS));
	SetDlgItemText(IDC_RECOVERED, str);

	str.Format(_T("%s (%.1f%%)"), CastItoXBytes(uCompression, false, false), uTransfered!=0 ? (uCompression * 100.0 / uTransfered) : 0.0);
	SetDlgItemText(IDC_COMPRESSION, str);

	str.Format(GetResString(IDS_SOURCESINFO), uSources, uValidSources, uNNPSources, uA4AFSources);
	SetDlgItemText(IDC_SOURCECOUNT, str);
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
	GetDlgItem(IDC_FD_X1)->SetWindowText(GetResString(IDS_SW_NAME));
	GetDlgItem(IDC_FD_X2)->SetWindowText(GetResString(IDS_FD_MET));
	GetDlgItem(IDC_FD_X3)->SetWindowText(GetResString(IDS_FD_HASH));
	GetDlgItem(IDC_FD_X4)->SetWindowText(GetResString(IDS_DL_SIZE));
	GetDlgItem(IDC_FD_X9)->SetWindowText(GetResString(IDS_FD_PARTS)+_T(":") );
	GetDlgItem(IDC_FD_X5)->SetWindowText(GetResString(IDS_FD_STATUS));
	GetDlgItem(IDC_FD_X6)->SetWindowText(GetResString(IDS_FD_TRANSFER));
	GetDlgItem(IDC_FD_X7)->SetWindowText(GetResString(IDS_DL_SOURCES)+_T(":"));
	GetDlgItem(IDC_FD_X14)->SetWindowText(GetResString(IDS_FD_TRANS));
	GetDlgItem(IDC_FD_X12)->SetWindowText(GetResString(IDS_FD_COMPSIZE));
	GetDlgItem(IDC_FD_X13)->SetWindowText(GetResString(IDS_FD_DATARATE));
	GetDlgItem(IDC_FD_X15)->SetWindowText(GetResString(IDS_LASTSEENCOMPL));
	GetDlgItem(IDC_FD_LASTCHANGE)->SetWindowText(GetResString(IDS_FD_LASTCHANGE));
	GetDlgItem(IDC_FD_X8)->SetWindowText(GetResString(IDS_FD_TIMEDATE));
	GetDlgItem(IDC_FD_X16)->SetWindowText(GetResString(IDS_FD_DOWNLOADSTARTED));
	GetDlgItem(IDC_DL_ACTIVE_TIME_LBL)->SetWindowText(GetResString(IDS_DL_ACTIVE_TIME)+_T(':'));
	GetDlgItem(IDC_HSAV)->SetWindowText(GetResString(IDS_HSAV)+_T(":"));
	GetDlgItem(IDC_FD_CORR)->SetWindowText(GetResString(IDS_FD_CORR)+_T(":"));
	GetDlgItem(IDC_FD_RECOV)->SetWindowText(GetResString(IDS_FD_RECOV)+_T(":"));
	GetDlgItem(IDC_FD_COMPR)->SetWindowText(GetResString(IDS_FD_COMPR)+_T(":"));
	GetDlgItem(IDC_FD_XAICH)->SetWindowText(GetResString(IDS_IACHHASH)+_T(":"));
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
	ON_BN_CLICKED(IDC_RENAME, OnRename) // Added by Tarod [Juanjo]
END_MESSAGE_MAP()

CFileDetailDialogName::CFileDetailDialogName()
	: CResizablePage(CFileDetailDialogName::IDD, 0)
{
	m_strCaption = GetResString(IDS_DL_FILENAME);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
	m_timer = 0;
	memset(m_aiColWidths, 0, sizeof m_aiColWidths);
	m_bAppliedSystemImageList = false;
	m_sortorder=0;
	m_sortindex=1;
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
	DDX_Control(pDX, IDC_LISTCTRLFILENAMES, pmyListCtrl);
}

BOOL CFileDetailDialogName::OnInitDialog()
{
	CResizablePage::OnInitDialog();
	InitWindowStyles(this);

	AddAnchor(IDC_FD_SN, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LISTCTRLFILENAMES, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TAKEOVER, BOTTOM_LEFT);
	AddAnchor(IDC_BUTTONSTRIP, BOTTOM_RIGHT);
	AddAnchor(IDC_RENAME, BOTTOM_RIGHT);
	AddAnchor(IDC_FILENAME, BOTTOM_LEFT, BOTTOM_RIGHT);

	pmyListCtrl.InsertColumn(0, GetResString(IDS_DL_FILENAME), LVCFMT_LEFT, 380); 
	pmyListCtrl.InsertColumn(1, GetResString(IDS_DL_SOURCES), LVCFMT_LEFT, 80); 
	ASSERT( (pmyListCtrl.GetStyle() & LVS_SHAREIMAGELISTS) != 0 );
	pmyListCtrl.LoadSettings(CPreferences::tableFilenames);

	m_sortindex = thePrefs.GetColumnSortItem(CPreferences::tableFilenames);
	m_sortorder = thePrefs.GetColumnSortAscending(CPreferences::tableFilenames);

	pmyListCtrl.SetSortArrow(m_sortindex, m_sortorder);
	pmyListCtrl.SortItems(&CompareListNameItems, m_sortindex + ( (m_sortorder) ? 0:10) );

	Localize();
	RefreshData();
	VERIFY( (m_timer = SetTimer(301, 5000, 0)) != NULL );

	return true;
}

void CFileDetailDialogName::RefreshData()
{
	bool bEnableRename = CanRenameFile();
	GetDlgItem(IDC_RENAME)->EnableWindow(bEnableRename);
	GetDlgItem(IDC_FILENAME)->EnableWindow(bEnableRename);
	GetDlgItem(IDC_BUTTONSTRIP)->EnableWindow(bEnableRename);
	GetDlgItem(IDC_TAKEOVER)->EnableWindow(bEnableRename);

	CString bufferS;
	GetDlgItem(IDC_FILENAME)->GetWindowText(bufferS);
	if (bufferS.GetLength()<3)
		GetDlgItem(IDC_FILENAME)->SetWindowText(m_file->GetFileName());

	FillSourcenameList();
}

void CFileDetailDialogName::OnDestroy()
{
	pmyListCtrl.SaveSettings(CPreferences::tableFilenames);

	for (int i=0;i<pmyListCtrl.GetItemCount();++i) {
		FCtrlItem_Struct* item= (FCtrlItem_Struct*)pmyListCtrl.GetItemData(i);
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
	    GetDlgItem(IDC_RENAME)->SetWindowText(GetResString(IDS_RENAME));
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
	for (int i=0;i<pmyListCtrl.GetItemCount();i++){
		FCtrlItem_Struct* item= (FCtrlItem_Struct*)pmyListCtrl.GetItemData(i);
		item->count=0;
	}

	// update
	for (POSITION pos = m_file->srclist.GetHeadPosition(); pos != NULL; )
	{ 
		CUpDownClient* cur_src = m_file->srclist.GetNext(pos); 
		if (cur_src->GetRequestFile() != m_file || cur_src->GetClientFilename().GetLength()==0)
			continue;

		info.psz = cur_src->GetClientFilename(); 
		if ((itempos=pmyListCtrl.FindItem(&info, -1)) == -1)
		{ 
			FCtrlItem_Struct* newitem= new FCtrlItem_Struct();
			newitem->count=1;
			newitem->filename=cur_src->GetClientFilename();

			int iSystemIconIdx = theApp.GetFileTypeSystemImageIdx(cur_src->GetClientFilename());
			if (theApp.GetSystemImageList() && !m_bAppliedSystemImageList)
			{
				pmyListCtrl.ApplyImageList(theApp.GetSystemImageList());
				ASSERT( (pmyListCtrl.GetStyle() & LVS_SHAREIMAGELISTS) != 0 );
				m_bAppliedSystemImageList = true;
			}

			int ix=pmyListCtrl.InsertItem(LVIF_TEXT|LVIF_PARAM|LVIF_IMAGE, pmyListCtrl.GetItemCount() ,cur_src->GetClientFilename(),0,0,iSystemIconIdx,(LPARAM)newitem);
			pmyListCtrl.SetItemText(ix, 1, _T("1")); 
		}
		else
		{
			FCtrlItem_Struct* item= (FCtrlItem_Struct*)pmyListCtrl.GetItemData(itempos);
			item->count+=1;
			strText.Format(_T("%i"),item->count);
			pmyListCtrl.SetItemText(itempos, 1,strText ); 
		} 
	} 

	// remove 0'er
	for (int i=0;i<pmyListCtrl.GetItemCount();i++)
	{
		FCtrlItem_Struct* item= (FCtrlItem_Struct*)pmyListCtrl.GetItemData(i);
		if (item && item->count==0)
		{
			delete item;
			pmyListCtrl.DeleteItem(i);
			i=0;
		}
	}

	pmyListCtrl.SortItems(&CompareListNameItems, m_sortindex + ( (m_sortorder) ? 0:10) );
}

void CFileDetailDialogName::TakeOver()
{
	if (pmyListCtrl.GetSelectedCount() > 0) {
		POSITION pos = pmyListCtrl.GetFirstSelectedItemPosition();
		if (pos){
			int itemPosition = pmyListCtrl.GetNextSelectedItem(pos); 
			GetDlgItem(IDC_FILENAME)->SetWindowText(pmyListCtrl.GetItemText(itemPosition,0));
		}
	} 
}

void CFileDetailDialogName::Copy()
{
	POSITION pos = pmyListCtrl.GetFirstSelectedItemPosition();
	if (pos){
		int iItem = pmyListCtrl.GetNextSelectedItem(pos);
		theApp.CopyTextToClipboard(pmyListCtrl.GetItemText(iItem, 0));
	} 
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

	if (m_sortindex!=pNMLV->iSubItem) m_sortorder=1; else m_sortorder=!(bool)m_sortorder;
	m_sortindex=pNMLV->iSubItem;

	pmyListCtrl.SetSortArrow(m_sortindex, m_sortorder);
	pmyListCtrl.SortItems(&CompareListNameItems, m_sortindex + ( (m_sortorder) ? 0:10) );

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
	if (pmyListCtrl.GetSelectionMark() == (-1))
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
	if (pmyListCtrl.GetSelectionMark() != (-1))
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

void CFileDetailDialogName::OnRename()
{
	if (CanRenameFile())
	{
		CString strNewFileName;
		GetDlgItem(IDC_FILENAME)->GetWindowText(strNewFileName);
		if (strNewFileName.IsEmpty() || !IsValidEd2kString(strNewFileName))
			return;
		m_file->SetFileName(strNewFileName, true);
		m_file->UpdateDisplayedInfo();
		m_file->SavePartFile();
	}
}

BOOL CFileDetailDialogName::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && pMsg->hwnd == GetDlgItem(IDC_FILENAME)->m_hWnd)
	{
		OnRename();
		return TRUE;
	}
	return CResizablePage::PreTranslateMessage(pMsg);
}

bool CFileDetailDialogName::CanRenameFile() const
{
	return (m_file->GetStatus() != PS_COMPLETE && m_file->GetStatus() != PS_COMPLETING);
}
