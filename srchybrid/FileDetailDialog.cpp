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
#include "OtherFunctions.h"
#include "FileInfoDialog.h"
#include "FileDetailDialog.h"
#include "Ini2.h"
#include "Preferences.h"
#include "UpDownClient.h"
#include "TitleMenu.h"
#include "MenuCmds.h"
#include "PartFile.h"
#include "DownloadQueue.h" //MORPH - Added by SiRoB

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

CFileDetailDialog::CFileDetailDialog(const CSimpleArray<CPartFile*>* paFiles, bool bInvokeCommentsPage)
{
	m_file = (*paFiles)[0];
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

		EED2KFileType eType = GetED2KFileTypeID(m_file->GetFileName());
		if (eType == ED2KFT_AUDIO || eType == ED2KFT_VIDEO){
			m_wndVideo.m_psp.dwFlags &= ~PSP_HASHELP;
			m_wndVideo.SetMyfile(m_file);
			AddPage(&m_wndVideo);
		}

		if (theApp.glob_prefs->IsExtControlsEnabled()){
			m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;
			m_wndMetaData.SetFile(m_file);
			AddPage(&m_wndMetaData);
		}
	}

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
	AddAnchor(IDC_PFSTATUS, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_PARTCOUNT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_HASHSET, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMPLSIZE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_DATARATE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SOURCECOUNT, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_FILECREATED, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LASTSEENCOMPL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LASTRECEIVED, TOP_LEFT, TOP_RIGHT);

	Localize();

	// properties which won't change during the dialog is open - no need to fresh them and mess with the focus
	if (m_paFiles->GetSize() == 1)
	{
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

		str.Format(_T("%u;  Available: %u (%.1f%%)"), (*m_paFiles)[0]->GetPartCount(), (*m_paFiles)[0]->GetAvailablePartCount(), (float)(((*m_paFiles)[0]->GetAvailablePartCount()*100)/(*m_paFiles)[0]->GetPartCount()));
		SetDlgItemText(IDC_PARTCOUNT, str);

		// date created
		if ((*m_paFiles)[0]->GetCrFileDate() != NULL){
			time_t fromtime = ((*m_paFiles)[0]->GetStatus() != PS_COMPLETE) ? time(NULL) : (*m_paFiles)[0]->GetFileDate();
			str.Format(_T("%s   ") + GetResString(IDS_TIMEBEFORE),
						(*m_paFiles)[0]->GetCrCFileDate().Format(theApp.glob_prefs->GetDateTimeFormat()),
						CastSecondsToLngHM(fromtime - (*m_paFiles)[0]->GetCrFileDate()));
		}
		else
			str = GetResString(IDS_UNKNOWN);
		SetDlgItemText(IDC_FILECREATED, str);

		// last seen complete
		struct tm* ptimLastSeenComplete = (*m_paFiles)[0]->lastseencomplete.GetLocalTm();
		if ((*m_paFiles)[0]->lastseencomplete == NULL || ptimLastSeenComplete == NULL)
			str.Format(GetResString(IDS_UNKNOWN));
		else{
			str.Format(_T("%s   ") + GetResString(IDS_TIMEBEFORE),
						(*m_paFiles)[0]->lastseencomplete.Format(theApp.glob_prefs->GetDateTimeFormat()),
						CastSecondsToLngHM(time(NULL) - safe_mktime(ptimLastSeenComplete)));
		}
		SetDlgItemText(IDC_LASTSEENCOMPL, str);

		// last receive
		if ((*m_paFiles)[0]->GetFileDate() != NULL && (*m_paFiles)[0]->GetRealFileSize() > 0){
			str.Format(_T("%s   ") + GetResString(IDS_TIMEBEFORE),
						(*m_paFiles)[0]->GetCFileDate().Format(theApp.glob_prefs->GetDateTimeFormat()),
						CastSecondsToLngHM(time(NULL) - (*m_paFiles)[0]->GetFileDate()));
		}
		else
			str = GetResString(IDS_UNKNOWN);
		SetDlgItemText(IDC_LASTRECEIVED, str);
	}
	else
	{
		SetDlgItemText(IDC_PFSTATUS, sm_pszNotAvail);
		SetDlgItemText(IDC_PARTCOUNT, sm_pszNotAvail);

		SetDlgItemText(IDC_FILECREATED, sm_pszNotAvail);
		SetDlgItemText(IDC_LASTSEENCOMPL, sm_pszNotAvail);
		SetDlgItemText(IDC_LASTRECEIVED, sm_pszNotAvail);
	}

	uint64 uFileSize = 0;
	uint64 uRealFileSize = 0;
	uint64 uTransfered = 0;
	uint64 uCorrupted = 0;
	uint64 uRecovered = 0;
	uint64 uCompression = 0;
	uint64 uCompleted = 0;
	int iHashsetAvailable = 0;
	UINT uDataRate = 0;
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
		iHashsetAvailable += ((*m_paFiles)[i]->GetHashCount() == (*m_paFiles)[i]->GetED2KPartCount()) ? 1 : 0; //MORPH - Changed by SiRoB, Safe Hash

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

	str.Format(_T("%s (%.2f%%)"), CastItoXBytes(uCompleted), uFileSize!=0 ? (uCompleted * 100.0 / uFileSize) : 0.0);
	SetDlgItemText(IDC_COMPLSIZE, str);

	str.Format(_T("%.2f %s"), uDataRate/1024.0, GetResString(IDS_KBYTESEC));
	SetDlgItemText(IDC_DATARATE, str);

	str.Format(_T("%s  (%s %s);  %s %s"), CastItoXBytes(uFileSize), GetFormatedUInt64(uFileSize), GetResString(IDS_BYTES), GetResString(IDS_ONDISK), CastItoXBytes(uRealFileSize));
	SetDlgItemText(IDC_FSIZE, str);

	SetDlgItemText(IDC_TRANSFERED, CastItoXBytes(uTransfered));
	SetDlgItemText(IDC_CORRUPTED, CastItoXBytes(uCorrupted));
	SetDlgItemInt(IDC_RECOVERED, uRecovered, FALSE);
	SetDlgItemText(IDC_COMPRESSION, CastItoXBytes(uCompression));

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
	GetDlgItem(IDC_FD_X9)->SetWindowText(GetResString(IDS_FD_PARTS));
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

	
	GetDlgItem(IDC_HSAV)->SetWindowText(GetResString(IDS_HSAV)+_T(":"));
	GetDlgItem(IDC_FD_CORR)->SetWindowText(GetResString(IDS_FD_CORR)+_T(":"));
	GetDlgItem(IDC_FD_RECOV)->SetWindowText(GetResString(IDS_FD_RECOV)+_T(":"));
	GetDlgItem(IDC_FD_COMPR)->SetWindowText(GetResString(IDS_FD_COMPR)+_T(":"));
	
	

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
	memset(m_aiColWidths, 0, sizeof m_aiColWidths);

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
	VERIFY( (m_timer = SetTimer(301, 5000, 0)) != NULL );

	return true;
}

void CFileDetailDialogName::RefreshData()
{
	GetDlgItem(IDC_RENAME)->EnableWindow((m_file->GetStatus() == PS_COMPLETE || m_file->GetStatus() == PS_COMPLETING) ? false:true);//add by CML 

	CString bufferS;
	GetDlgItem(IDC_FILENAME)->GetWindowText(bufferS);
	if (bufferS.GetLength()<3)
		GetDlgItem(IDC_FILENAME)->SetWindowText(m_file->GetFileName());

	FillSourcenameList();
}

void CFileDetailDialogName::OnDestroy()
{

	for (int i=0;i<pmyListCtrl->GetItemCount();++i) {

		FCtrlItem_Struct* item= (FCtrlItem_Struct*)pmyListCtrl->GetItemData(i);
		delete item;
	}

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
	if (theApp.glob_prefs->GetLanguageID() != MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT)){
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

	if (pmyListCtrl->GetHeaderCtrl()->GetItemCount() < 2)
	{
		pmyListCtrl->DeleteColumn(0); 
		pmyListCtrl->InsertColumn(0, GetResString(IDS_DL_FILENAME), LVCFMT_LEFT, m_aiColWidths[0], -1); 
		pmyListCtrl->InsertColumn(1, GetResString(IDS_DL_SOURCES), LVCFMT_LEFT, m_aiColWidths[1], 1); 
	}

	CString strText; 

	// reset
	for (int i=0;i<pmyListCtrl->GetItemCount();i++){
		FCtrlItem_Struct* item= (FCtrlItem_Struct*)pmyListCtrl->GetItemData(i);
		item->count=0;
	}

	// update
	for (POSITION pos = m_file->srclist.GetHeadPosition(); pos != NULL; )
	{ 
		CUpDownClient* cur_src = m_file->srclist.GetNext(pos); 
		if (cur_src->reqfile!=m_file || cur_src->GetClientFilename().GetLength()==0)
			continue;

		info.psz = cur_src->GetClientFilename(); 
		if ((itempos=pmyListCtrl->FindItem(&info, -1)) == -1)
		{ 
			FCtrlItem_Struct* newitem= new FCtrlItem_Struct();
			newitem->count=1;
			newitem->filename=cur_src->GetClientFilename();
			
			pmyListCtrl->InsertItem(LVIF_TEXT|LVIF_PARAM,0,cur_src->GetClientFilename(),0,0,0,(LPARAM)newitem);
			pmyListCtrl->SetItemText(0, 1, "1"); 
		}
		else
		{
			FCtrlItem_Struct* item= (FCtrlItem_Struct*)pmyListCtrl->GetItemData(itempos);
			item->count+=1;
			strText.Format("%i",item->count);
			pmyListCtrl->SetItemText(itempos, 1,strText ); 
		} 
	} 

	// remove 0'er
	for (int i=0;i<pmyListCtrl->GetItemCount();i++)
	{
		FCtrlItem_Struct* item= (FCtrlItem_Struct*)pmyListCtrl->GetItemData(i);
		if (item && item->count==0)
		{
			delete item;
			pmyListCtrl->DeleteItem(i);
			i=0;
		}
	}

	pmyListCtrl->SortItems(&CompareListNameItems, m_sortindex + ( (m_sortorder) ? 0:10) );
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

	if (m_sortindex!=pNMLV->iSubItem) m_sortorder=1; else m_sortorder=!(bool)m_sortorder;
	m_sortindex=pNMLV->iSubItem;

	pmyListCtrl->SortItems(&CompareListNameItems, m_sortindex + ( (m_sortorder) ? 0:10) );

	*pResult = 0;
}

int CALLBACK CFileDetailDialogName::CompareListNameItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{ 
	FCtrlItem_Struct* item1=(FCtrlItem_Struct*) lParam1;
	FCtrlItem_Struct* item2=(FCtrlItem_Struct*) lParam2;
	switch (lParamSort){
		case 0: return ( stricmp(item1->filename,item2->filename)); break;
		case 10: return ( stricmp(item2->filename,item1->filename)); break;
		case 1: return (item1->count - item2->count); break;
		case 11: return (item2->count - item1->count); break;

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
	m_file->UpdateDisplayedInfo();
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
