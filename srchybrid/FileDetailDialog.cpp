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

// FileDetailDialog.cpp : implementation file
//

#include "stdafx.h"
#include "emule.h"
#include "OtherFunctions.h"
#include "FileInfoDialog.h"
#include "FileDetailDialog.h"
#include "Ini2.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialog

int CFileDetailDialog::sm_iLastActivePage;

IMPLEMENT_DYNAMIC(CFileDetailDialog, CResizableSheet)

BEGIN_MESSAGE_MAP(CFileDetailDialog, CResizableSheet)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CFileDetailDialog::CFileDetailDialog(CPartFile* file, bool bInvokeCommentsPage)
{
	m_file = file;
	m_psh.dwFlags &= ~PSH_HASHELP;
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	
	m_wndInfo.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndName.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndComments.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndVideo.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;

	m_wndInfo.SetMyfile(file);
	m_wndName.SetMyfile(file);
	m_wndVideo.SetMyfile(file);
	m_wndComments.SetMyfile(file);
	if (theApp.glob_prefs->IsExtControlsEnabled())
		m_wndMetaData.SetFile(file);

	AddPage(&m_wndInfo);
	AddPage(&m_wndName);
	AddPage(&m_wndComments);
	EED2KFileType eType = GetED2KFileTypeID(file->GetFileName());
	if (eType == ED2KFT_AUDIO || eType == ED2KFT_VIDEO)
		AddPage(&m_wndVideo);

	if (theApp.glob_prefs->IsExtControlsEnabled())
		AddPage(&m_wndMetaData);

	m_bInvokeCommentsPage =  bInvokeCommentsPage;
}

CFileDetailDialog::~CFileDetailDialog()
{
}

void CFileDetailDialog::OnDestroy()
{
	if (!m_bInvokeCommentsPage)
		sm_iLastActivePage = GetActiveIndex();
	CResizableSheet::OnDestroy();
}

BOOL CFileDetailDialog::OnInitDialog()
{		
	EnableStackedTabs(FALSE);
	BOOL bResult = CResizableSheet::OnInitDialog();
	InitWindowStyles(this);
	EnableSaveRestore(_T("FileDetailDialog")); // call this after(!) OnInitDialog
	SetWindowText(GetResString(IDS_FD_TITLE));

	if (m_bInvokeCommentsPage)
		SetActivePage(2);
	else if (sm_iLastActivePage < GetPageCount())
		SetActivePage(sm_iLastActivePage);

	return bResult;
}


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialogInfo dialog

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

	AddAnchor(IDC_FILECREATED, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LASTSEENCOMPL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LASTRECEIVED, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FNAME, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_METFILE, TOP_LEFT, TOP_RIGHT);

	Localize();
	RefreshData();
	VERIFY( (m_timer = SetTimer(301, 5000, 0)) );

	return true;
}

BOOL CFileDetailDialogInfo::OnSetActive()
{
	if (!CResizablePage::OnSetActive())
		return FALSE;
	GetDlgItem(IDC_FNAME)->SetWindowText(m_file->GetFileName());
	return TRUE;
}

void CFileDetailDialogInfo::RefreshData()
{
	CString bufferS;

	GetDlgItem(IDC_FNAME)->SetWindowText(MakeStringEscaped(m_file->GetFileName()));
	GetDlgItem(IDC_METFILE)->SetWindowText(m_file->GetFullName());

	GetDlgItem(IDC_FHASH)->SetWindowText(EncodeBase16(m_file->GetFileHash(), 16));

	bufferS.Format("%s  (%s %s)",CastItoXBytes(m_file->GetFileSize()),GetFormatedUInt(m_file->GetFileSize()),GetResString(IDS_BYTES));
	GetDlgItem(IDC_FSIZE)->SetWindowText(bufferS);

	if (m_file->GetTransferingSrcCount()>0) bufferS.Format(GetResString(IDS_PARTINFOS2),m_file->GetTransferingSrcCount());
		else bufferS=m_file->getPartfileStatus();
	GetDlgItem(IDC_PFSTATUS)->SetWindowText(bufferS);
	
	bufferS.Format("%i",m_file->GetHashCount());
	GetDlgItem(IDC_PARTCOUNT)->SetWindowText(bufferS);

	GetDlgItem(IDC_TRANSFERED)->SetWindowText(CastItoXBytes(m_file->GetTransfered()));

	//Khaos corumptionByte++
	bufferS.Format(GetResString(IDS_FD_STATS),CastItoXBytes(m_file->GetLostDueToCorruption()+m_file->GetSesCorruptionBytes()), CastItoXBytes(m_file->GetGainDueToCompression()+m_file->GetSesCompressionBytes()), m_file->TotalPacketsSavedDueToICH() );
	GetDlgItem(IDC_FD_STATS)->SetWindowText(bufferS);
	//Khaos corumptionByte--
	GetDlgItem(IDC_COMPLSIZE)->SetWindowText(CastItoXBytes(m_file->GetCompletedSize()));

	bufferS.Format("%.2f ",m_file->GetPercentCompleted());
	GetDlgItem(IDC_PROCCOMPL)->SetWindowText(bufferS+GetResString(IDS_PROCDONE));

	bufferS.Format("%.2f %s",(float)m_file->GetDatarate()/1024,GetResString(IDS_KBYTESEC));
	GetDlgItem(IDC_DATARATE)->SetWindowText(bufferS);

	if (m_file->IsPartFile() ) bufferS.Format(GetResString(IDS_SOURCESINFO),m_file->GetSourceCount(),m_file->GetValidSourcesCount(),m_file->GetSrcStatisticsValue(DS_NONEEDEDPARTS),m_file->GetSrcA4AFCount());
		else bufferS="";
	GetDlgItem(IDC_SOURCECOUNT)->SetWindowText(bufferS);

	bufferS.Format(GetResString(IDS_AVAIL),m_file->GetPartCount(),m_file->GetAvailablePartCount(),(float) ((m_file->GetAvailablePartCount()*100)/ m_file->GetPartCount()));
	GetDlgItem(IDC_PARTAVAILABLE)->SetWindowText(bufferS);

	if (m_file->lastseencomplete==NULL) bufferS.Format(GetResString(IDS_UNKNOWN).MakeLower()); else
		bufferS.Format( "%s   "+GetResString(IDS_TIMEBEFORE),m_file->lastseencomplete.Format( theApp.glob_prefs->GetDateTimeFormat()),
		CastSecondsToLngHM( time(NULL)- mktime(m_file->lastseencomplete.GetLocalTm()))
		);
	GetDlgItem(IDC_LASTSEENCOMPL)->SetWindowText(bufferS);

	if (m_file->GetFileDate()!=NULL&& m_file->GetRealFileSize()>0) 
		bufferS.Format( "%s   "+GetResString(IDS_TIMEBEFORE),m_file->GetCFileDate().Format( theApp.glob_prefs->GetDateTimeFormat()),
			CastSecondsToLngHM(time(NULL)- m_file->GetFileDate()));
		else bufferS=GetResString(IDS_UNKNOWN);
	GetDlgItem(IDC_LASTRECEIVED)->SetWindowText(bufferS);

	if (m_file->IsPartFile()) bufferS.Format("(%s %s)",GetResString(IDS_ONDISK),CastItoXBytes(m_file->GetRealFileSize()));
		else bufferS="";
	GetDlgItem(IDC_FSIZE2)->SetWindowText(bufferS );

	if (m_file->GetCrFileDate()!=NULL) {
		uint32 fromtime=(m_file->GetStatus()!=PS_COMPLETE)?time(NULL):m_file->GetFileDate();
		bufferS.Format( "%s   "+GetResString(IDS_TIMEBEFORE),m_file->GetCrCFileDate().Format( theApp.glob_prefs->GetDateTimeFormat()),
			CastSecondsToLngHM(fromtime - m_file->GetCrFileDate()));
	}
	else bufferS=GetResString(IDS_UNKNOWN);
	GetDlgItem(IDC_FILECREATED)->SetWindowText(bufferS);
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
	GetDlgItem(IDC_FD_X1)->SetWindowText(GetResString(IDS_FD_NAME));
	GetDlgItem(IDC_FD_X2)->SetWindowText(GetResString(IDS_FD_MET));
	GetDlgItem(IDC_FD_X3)->SetWindowText(GetResString(IDS_FD_HASH));
	GetDlgItem(IDC_FD_X4)->SetWindowText(GetResString(IDS_FD_SIZE));
	GetDlgItem(IDC_FD_X5)->SetWindowText(GetResString(IDS_FD_STATUS));
	GetDlgItem(IDC_FD_X6)->SetWindowText(GetResString(IDS_FD_TRANSFER));
	GetDlgItem(IDC_FD_X7)->SetWindowText(GetResString(IDS_DL_SOURCES)+":");
	GetDlgItem(IDC_FD_X9)->SetWindowText(GetResString(IDS_FD_HASHSETS));
	GetDlgItem(IDC_FD_X14)->SetWindowText(GetResString(IDS_FD_TRANS));
	GetDlgItem(IDC_FD_X12)->SetWindowText(GetResString(IDS_FD_COMPSIZE));
	GetDlgItem(IDC_FD_X13)->SetWindowText(GetResString(IDS_FD_DATARATE));
	GetDlgItem(IDC_FD_X15)->SetWindowText(GetResString(IDS_LASTSEENCOMPL));
	GetDlgItem(IDC_FD_LASTCHANGE)->SetWindowText(GetResString(IDS_FD_LASTCHANGE));
	GetDlgItem(IDC_FD_X8)->SetWindowText(GetResString(IDS_FD_TIMEDATE));
	GetDlgItem(IDC_FD_X16)->SetWindowText(GetResString(IDS_FD_DOWNLOADSTARTED));
}


///////////////////////////////////////////////////////////////////////////////
// CFileDetailDialogName dialog

#define	FILE_DETAIL_PREF_INI_SECTION	_T("FileDetailNameDlg")
#define	FILE_DETAIL_PREF_INI_COLWIDTH	_T("Col%uWidth")

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
	MEMSET(m_aiColWidths, 0, sizeof m_aiColWidths);
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
}

BOOL CFileDetailDialogName::OnInitDialog()
{
	CString strIniFile;
	strIniFile.Format(_T("%spreferences.ini"), theApp.glob_prefs->GetConfigDir());
	CIni ini(strIniFile, FILE_DETAIL_PREF_INI_SECTION);
	m_aiColWidths[0] = ini.GetInt("Col0Width", 340);
	m_aiColWidths[1] = ini.GetInt("Col1Width", 60);

	CResizablePage::OnInitDialog();
	InitWindowStyles(this);
	pmyListCtrl = (CListCtrl*)GetDlgItem(IDC_LISTCTRLFILENAMES);
	pmyListCtrl->SetExtendedStyle(LVS_EX_FULLROWSELECT);

	AddAnchor(IDC_FD_SN, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LISTCTRLFILENAMES, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TAKEOVER, BOTTOM_LEFT);
	AddAnchor(IDC_BUTTONSTRIP, BOTTOM_RIGHT);
	AddAnchor(IDC_RENAME, BOTTOM_RIGHT);
	AddAnchor(IDC_FILENAME, BOTTOM_LEFT, BOTTOM_RIGHT);

	Localize();
	RefreshData();
	VERIFY( (m_timer = SetTimer(301, 5000, 0)) );

	return true;
}

void CFileDetailDialogName::RefreshData()
{
	GetDlgItem(IDC_RENAME)->EnableWindow((m_file->GetStatus() == PS_COMPLETE || m_file->GetStatus() == PS_COMPLETING) ? false:true);//add by CML 

	CString bufferS;
	GetDlgItem(IDC_FILENAME)->GetWindowText(bufferS);
	if (bufferS.GetLength()<3)
		GetDlgItem(IDC_FILENAME)->SetWindowText(m_file->GetFileName());

	int sel=pmyListCtrl->GetSelectionMark();
	FillSourcenameList();
	pmyListCtrl->SetSelectionMark(sel);
}

void CFileDetailDialogName::OnDestroy()
{
	CString strIniFile;
	strIniFile.Format(_T("%spreferences.ini"), theApp.glob_prefs->GetConfigDir());
	CIni ini(strIniFile, FILE_DETAIL_PREF_INI_SECTION);
	ini.WriteInt("Col0Width", pmyListCtrl->GetColumnWidth(0));
	ini.WriteInt("Col1Width", pmyListCtrl->GetColumnWidth(1));

	if (m_timer){
		KillTimer(m_timer);
		m_timer = 0;
	}
}

void CFileDetailDialogName::Localize()
{
	GetDlgItem(IDC_TAKEOVER)->SetWindowText(GetResString(IDS_TAKEOVER));
	GetDlgItem(IDC_BUTTONSTRIP)->SetWindowText(GetResString(IDS_CLEANUP));
	GetDlgItem(IDC_RENAME)->SetWindowText(GetResString(IDS_RENAME));
	GetDlgItem(IDC_FD_SN)->SetWindowText(GetResString(IDS_SOURCENAMES));
}

void CFileDetailDialogName::FillSourcenameList()
{
	LVFINDINFO info; 
	info.flags = LVFI_STRING; 
	int itempos; 
	int namecount; 

	if (pmyListCtrl->GetHeaderCtrl()->GetItemCount() < 2)
	{
		pmyListCtrl->DeleteColumn(0); 
		pmyListCtrl->InsertColumn(0, GetResString(IDS_DL_FILENAME), LVCFMT_LEFT, m_aiColWidths[0], -1); 
		pmyListCtrl->InsertColumn(1, GetResString(IDS_DL_SOURCES), LVCFMT_LEFT, m_aiColWidths[1], 1); 
	}

	CString strText; 

	// reset
	for (int i=0;i<pmyListCtrl->GetItemCount();i++){
		pmyListCtrl->SetItemText(i, 1, "0"); 
		pmyListCtrl->SetItemData(i, 0);
	}

	// update
	for (int sl=0;sl<SOURCESSLOTS;sl++)
	{
		for (POSITION pos = m_file->srclists[sl].GetHeadPosition(); pos != NULL; )
		{ 
			CUpDownClient* cur_src = m_file->srclists[sl].GetNext(pos); 
			if (cur_src->reqfile!=m_file || cur_src->GetClientFilename().GetLength()==0)
				continue;

			info.psz = cur_src->GetClientFilename(); 
			if ((itempos=pmyListCtrl->FindItem(&info, -1)) == -1)
			{ 
				pmyListCtrl->InsertItem(0, cur_src->GetClientFilename()); 
				pmyListCtrl->SetItemText(0, 1, "1"); 
				pmyListCtrl->SetItemData(0, 1); 
			}
			else
			{ 
				namecount = atoi(pmyListCtrl->GetItemText(itempos, 1))+1; 
				CString nameCountStr; 
				nameCountStr.Format("%i", namecount); 
				pmyListCtrl->SetItemText(itempos, 1, nameCountStr.GetString()); 
				pmyListCtrl->SetItemData(itempos, namecount); 
			} 
			pmyListCtrl->SortItems(CompareListNameItems, 11); 
			m_SortAscending[0] =true;
			m_SortAscending[1] =false;
		} 
	}

	// remove 0'er
	for (int i=0;i<pmyListCtrl->GetItemCount();i++)
	{
		if (pmyListCtrl->GetItemData(i)==0)
		{
			pmyListCtrl->DeleteItem(i);
			i=0;
		}
	}
}

void CFileDetailDialogName::TakeOver()
{
	int itemPosition; 

	if (pmyListCtrl->GetSelectedCount() > 0) {
		POSITION pos = pmyListCtrl->GetFirstSelectedItemPosition(); 
		itemPosition = pmyListCtrl->GetNextSelectedItem(pos); 
		GetDlgItem(IDC_FILENAME)->SetWindowText(pmyListCtrl->GetItemText(itemPosition,0));
	} 
}

void CFileDetailDialogName::OnBnClickedButtonStrip()
{
	CString filename;

	GetDlgItem(IDC_FILENAME)->GetWindowText(filename);
	GetDlgItem(IDC_FILENAME)->SetWindowText( CleanupFilename(filename) );
}

void CFileDetailDialogName::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	m_SortAscending[pNMLV->iSubItem] = !m_SortAscending[pNMLV->iSubItem];
	//SetSortArrow(pNMLV->iSubItem, m_SortAscending[pNMLV->iSubItem]);

	pmyListCtrl->SortItems(&CompareListNameItems,pNMLV->iSubItem+((m_SortAscending[pNMLV->iSubItem])? 0:10));

	*pResult = 0;
}

int CALLBACK CFileDetailDialogName::CompareListNameItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{ 
	switch (lParamSort){
		case 1: return (lParam1 - lParam2); break;
		case 11: return (lParam2 - lParam1); break;
		default: return 0;
	}
} 

void CFileDetailDialogName::OnNMDblclkList(NMHDR *pNMHDR, LRESULT *pResult)
{
	TakeOver();
	*pResult = 0;
}

void CFileDetailDialogName::OnNMRclickList(NMHDR *pNMHDR, LRESULT *pResult)
{
	UINT flag = MF_STRING;
	if (pmyListCtrl->GetSelectionMark() == (-1))
		flag = MF_GRAYED;

	POINT point;
	::GetCursorPos(&point);
	CTitleMenu popupMenu;
	popupMenu.CreatePopupMenu();
	popupMenu.AppendMenu(flag,MP_MESSAGE, GetResString(IDS_TAKEOVER));
	popupMenu.AppendMenu(MF_STRING,MP_RESTORE, GetResString(IDS_SV_UPDATE));
	popupMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON, point.x, point.y, this);
	VERIFY( popupMenu.DestroyMenu() );

	*pResult = 0;
}

BOOL CFileDetailDialogName::OnCommand(WPARAM wParam,LPARAM lParam )
{
	if (pmyListCtrl->GetSelectionMark() != (-1))
	{
		switch (wParam)
		{
		case MP_MESSAGE:
			TakeOver();
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
	CString NewFileName; 
	GetDlgItem(IDC_FILENAME)->GetWindowText(NewFileName);

	m_file->SetFileName(NewFileName, true); 
	m_file->SavePartFile(); 
	theApp.downloadqueue->UpdatePNRFile(m_file); //<<-- enkeyDEV(ColdShine) -PartfileNameRecovery-
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
