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
#include "IPFilterDlg.h"
#include "IPFilter.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "MenuCmds.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////////////////
// COLUMN_INIT -- List View Columns

enum EIPFilterCols
{
	IPFILTER_COL_START = 0,
	IPFILTER_COL_END,
	IPFILTER_COL_LEVEL,
	IPFILTER_COL_HITS,
	IPFILTER_COL_DESC
};

static LCX_COLUMN_INIT _aColumns[] =
{
	{ IPFILTER_COL_START,	_T("Start"),		IDS_IP_START,	LVCFMT_LEFT,	-1, 0, ASCENDING,  NONE, _T("255.255.255.255") },
	{ IPFILTER_COL_END,		_T("End"),			IDS_IP_END,		LVCFMT_LEFT,	-1, 1, ASCENDING,  NONE, _T("255.255.255.255")},
	{ IPFILTER_COL_LEVEL,	_T("Level"),		IDS_IP_LEVEL,	LVCFMT_RIGHT,	-1, 2, ASCENDING,  NONE, _T("999") },
	{ IPFILTER_COL_HITS,	_T("Hits"),			IDS_IP_HITS,	LVCFMT_RIGHT,	-1, 3, DESCENDING, NONE, _T("99999") },
	{ IPFILTER_COL_DESC,	_T("Description"),	IDS_IP_DESC,	LVCFMT_LEFT,	-1, 4, ASCENDING,  NONE, _T("long long long long long long long long file name") },
};

#define	PREF_INI_SECTION	_T("IPFilterDlg")

int CIPFilterDlg::sm_iSortColumn = IPFILTER_COL_HITS;

IMPLEMENT_DYNAMIC(CIPFilterDlg, CDialog)

BEGIN_MESSAGE_MAP(CIPFilterDlg, CResizableDialog)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_IPFILTER, OnLvnColumnClickIPFilter)
	ON_NOTIFY(LVN_KEYDOWN, IDC_IPFILTER, OnLvnKeyDownIPFilter)
	ON_BN_CLICKED(IDC_COPY, OnBnClickedCopy)
	ON_BN_CLICKED(IDC_REMOVE, OnBnClickedDelete)
	ON_BN_CLICKED(IDC_APPEND, OnBnClickedAppend)
	ON_COMMAND(MP_COPYSELECTED, OnCopyIPFilter)
	ON_COMMAND(MP_REMOVE, OnDeleteIPFilter)
	ON_COMMAND(MP_SELECTALL, OnSelectAllIPFilter)
	ON_COMMAND(MP_FIND, OnFind)
	ON_BN_CLICKED(IDC_SAVE, OnBnClickedSave)
	ON_NOTIFY(LVN_DELETEALLITEMS, IDC_IPFILTER, OnLvnDeleteAllItemsIPfilter)
END_MESSAGE_MAP()

CIPFilterDlg::CIPFilterDlg(CWnd* pParent /*=NULL*/)
	: CResizableDialog(CIPFilterDlg::IDD, pParent)
{
	m_pMenuIPFilter = NULL;
	m_ulFilteredIPs = 0;
	m_ipfilter.m_pParent = this;
	m_ipfilter.SetRegistryKey(PREF_INI_SECTION);
	m_ipfilter.SetRegistryPrefix(_T("IPfilters_"));
}

CIPFilterDlg::~CIPFilterDlg()
{
	delete m_pMenuIPFilter;
	sm_iSortColumn = m_ipfilter.GetSortColumn();
}

void CIPFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPFILTER, m_ipfilter);
}

extern "C" int CALLBACK CIPFilterDlg::CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	ASSERT( lParamSort >= 0 && lParamSort < ARRSIZE(_aColumns) );

#define COMPARE_NUM( a, b ) ((a) < (b))			\
							  ? -1				\
							  : ( ((b) < (a))	\
									? 1			\
									: 0			\
								)

	int iResult;

	if (lParam1==0 && lParam2==0)
		iResult = 0;
	else if (lParam1==0)
		iResult = 1;
	else if (lParam2==0)
		iResult = -1;
	else{
		if (lParamSort == IPFILTER_COL_START)
		{
			iResult = COMPARE_NUM(((SIPFilter*)lParam1)->start, ((SIPFilter*)lParam2)->start);
		}
		else if (lParamSort == IPFILTER_COL_END)
		{
			iResult = COMPARE_NUM(((SIPFilter*)lParam1)->end, ((SIPFilter*)lParam2)->end);
		}
		else if (lParamSort == IPFILTER_COL_LEVEL)
		{
			iResult = COMPARE_NUM(((SIPFilter*)lParam1)->level, ((SIPFilter*)lParam2)->level);
		}
		else if (lParamSort == IPFILTER_COL_HITS)
		{
			iResult = COMPARE_NUM(((SIPFilter*)lParam1)->hits, ((SIPFilter*)lParam2)->hits);
		}
		else if (lParamSort == IPFILTER_COL_DESC)
		{
			iResult = strcmp(((SIPFilter*)lParam1)->desc, ((SIPFilter*)lParam2)->desc);
		}
		else
		{
			ASSERT(0);
			iResult = 0;
		}
	}
#undef COMPARE_NUM

	if (_aColumns[lParamSort].eSortOrder == DESCENDING)
		return -iResult;
	else
		return iResult;
}

void CIPFilterDlg::UpdateItems()
{
	// Update (sort, if needed) the listview items
	if (m_ipfilter.GetSortColumn() != -1)
		m_ipfilter.SortItems(CIPFilterDlg::CompareItems, m_ipfilter.GetSortColumn());
}

void CIPFilterDlg::OnLvnColumnClickIPFilter(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	m_ipfilter.UpdateSortOrder(pNMLV, ARRSIZE(_aColumns), _aColumns);
	UpdateItems();
	*pResult = 0;
}

BOOL CIPFilterDlg::OnInitDialog()
{
	CResizableDialog::OnInitDialog();
	InitWindowStyles(this);

	AddAnchor(IDC_IPFILTER, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TOTAL_IPFILTER_LABEL, BOTTOM_LEFT);
	AddAnchor(IDC_TOTAL_IPFILTER, BOTTOM_LEFT);
	AddAnchor(IDC_TOTAL_IPS_LABEL, BOTTOM_LEFT);
	AddAnchor(IDC_TOTAL_IPS, BOTTOM_LEFT);
	AddAnchor(IDC_COPY, BOTTOM_RIGHT);
	AddAnchor(IDC_REMOVE, BOTTOM_RIGHT);
	AddAnchor(IDC_APPEND, BOTTOM_RIGHT);
	AddAnchor(IDC_SAVE, BOTTOM_RIGHT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	EnableSaveRestore(PREF_INI_SECTION);

	m_ipfilter.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_ipfilter.EnableHdrCtrlSortBitmaps();
	m_ipfilter.ReadColumnStats(ARRSIZE(_aColumns), _aColumns);
	m_ipfilter.CreateColumns(ARRSIZE(_aColumns), _aColumns);
  	m_ipfilter.InitColumnOrders(ARRSIZE(_aColumns), _aColumns);
	m_ipfilter.UpdateSortColumn(ARRSIZE(_aColumns), _aColumns);

	InitIPFilters();
	
	m_pMenuIPFilter = new CMenu();
	if (m_pMenuIPFilter->CreatePopupMenu())
	{
		m_pMenuIPFilter->AppendMenu(MF_ENABLED | MF_STRING, MP_COPYSELECTED, GetResString(IDS_COPY));
		m_pMenuIPFilter->AppendMenu(MF_ENABLED | MF_STRING, MP_REMOVE, GetResString(IDS_REMOVE));
		m_pMenuIPFilter->AppendMenu(MF_SEPARATOR);
		m_pMenuIPFilter->AppendMenu(MF_ENABLED | MF_STRING, MP_SELECTALL, GetResString(IDS_SELECTALL));
		m_pMenuIPFilter->AppendMenu(MF_SEPARATOR);
		m_pMenuIPFilter->AppendMenu(MF_ENABLED | MF_STRING, MP_FIND, GetResString(IDS_FIND));
	}
	m_ipfilter.m_pMenu = m_pMenuIPFilter;
	m_ipfilter.m_pParent = this;

	
	// localize
	SetWindowText(GetResString(IDS_IPFILTER));
	SetDlgItemText(IDC_STATICIPLABEL,GetResString(IDS_IP_RULES));
	SetDlgItemText(IDC_TOTAL_IPFILTER_LABEL,GetResString(IDS_TOTAL_IPFILTER_LABEL));
	SetDlgItemText(IDC_TOTAL_IPS_LABEL,GetResString(IDS_TOTAL_IPS_LABEL));
	SetDlgItemText(IDC_COPY,GetResString(IDS_COPY));
	SetDlgItemText(IDC_REMOVE,GetResString(IDS_DELETESELECTED));
	SetDlgItemText(IDC_APPEND,GetResString(IDS_APPEND));
	SetDlgItemText(IDC_SAVE,GetResString(IDS_SAVE));
	SetDlgItemText(IDOK,GetResString(IDS_FD_CLOSE));

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

void CIPFilterDlg::InitIPFilters()
{
	CWaitCursor curWait;
	m_ipfilter.DeleteAllItems();
	m_ipfilter.SetRedraw(FALSE);
	m_ulFilteredIPs = 0;

	const CIPFilterArray& ipfilter = theApp.ipfilter->GetIPFilter();
	int iFilters = ipfilter.GetCount();
	m_ipfilter.SetItemCount(iFilters);
	for (int i = 0; i < iFilters; i++)
	{
		const SIPFilter* pFilter = ipfilter[i];

		m_ulFilteredIPs += pFilter->end - pFilter->start + 1;

		TCHAR szBuff[256];
		LVITEM lvi;
		lvi.mask = LVIF_TEXT | LVIF_PARAM;
		lvi.iItem = INT_MAX;
		lvi.iSubItem = IPFILTER_COL_START;
		in_addr ip;
		ip.S_un.S_addr = htonl(pFilter->start);
		lvi.pszText = inet_ntoa(ip);
		lvi.lParam = (DWORD)pFilter;
		int iItem = m_ipfilter.InsertItem(&lvi);
		if (iItem >= 0)
		{
			lvi.mask = LVIF_TEXT;
			lvi.iItem = iItem;

			ip.S_un.S_addr = htonl(pFilter->end);
			lvi.pszText = inet_ntoa(ip);
			lvi.iSubItem = IPFILTER_COL_END;
			m_ipfilter.SetItem(&lvi);

			itoa(pFilter->level, szBuff, 10);
			lvi.pszText = szBuff;
			lvi.iItem = iItem;
			lvi.iSubItem = IPFILTER_COL_LEVEL;
			m_ipfilter.SetItem(&lvi);

			itoa(pFilter->hits, szBuff, 10);
			lvi.pszText = szBuff;
			lvi.iItem = iItem;
			lvi.iSubItem = IPFILTER_COL_HITS;
			m_ipfilter.SetItem(&lvi);

			lvi.pszText = const_cast<LPTSTR>((LPCTSTR)pFilter->desc);
			lvi.iItem = iItem;
			lvi.iSubItem = IPFILTER_COL_DESC;
			m_ipfilter.SetItem(&lvi);
		}
	}
	UpdateItems();
	m_ipfilter.SetRedraw();
	SetDlgItemText(IDC_TOTAL_IPFILTER, GetFormatedUInt(m_ipfilter.GetItemCount()));
	SetDlgItemText(IDC_TOTAL_IPS, GetFormatedUInt(m_ulFilteredIPs));
}

void CIPFilterDlg::OnBnClickedCopy()
{
	CWaitCursor curWait;
	int iSelected = 0;
	CString strData;
	POSITION pos = m_ipfilter.GetFirstSelectedItemPosition();
	while (pos)
	{
		int iItem = m_ipfilter.GetNextSelectedItem(pos);
		if (!strData.IsEmpty())
			strData += _T("\r\n");

		strData.AppendFormat(_T("%-15s - %-15s  Hits=%-5s  %s"), 
				m_ipfilter.GetItemText(iItem, IPFILTER_COL_START),
				m_ipfilter.GetItemText(iItem, IPFILTER_COL_END),
				m_ipfilter.GetItemText(iItem, IPFILTER_COL_HITS),
				m_ipfilter.GetItemText(iItem, IPFILTER_COL_DESC));
		iSelected++;
	}

	if (!strData.IsEmpty())
	{
		if (iSelected > 1)
			strData += _T("\r\n");
		theApp.CopyTextToClipboard(strData);
	}
}

void CIPFilterDlg::OnCopyIPFilter()
{
	OnBnClickedCopy();
}

void CIPFilterDlg::OnSelectAllIPFilter()
{
	m_ipfilter.SelectAllItems();
}

void CIPFilterDlg::OnBnClickedAppend()
{
	CString strFilePath;
	if (DialogBrowseFile(strFilePath, GetResString(IDS_IPFILTERFILES)))
	{
		CWaitCursor curWait;
		if (theApp.ipfilter->AddFromFile(strFilePath, true))
			InitIPFilters();
	}
}

void CIPFilterDlg::OnBnClickedDelete()
{
	if (m_ipfilter.GetSelectedCount() == 0)
		return;
	if (AfxMessageBox(_T(GetResString(IDS_DELETEIPFILTERS)), MB_YESNOCANCEL) != IDYES)
		return;

	CWaitCursor curWait;
	CUIntArray aItems;
	POSITION pos = m_ipfilter.GetFirstSelectedItemPosition();
	while (pos)
	{
		int iItem = m_ipfilter.GetNextSelectedItem(pos);
		const SIPFilter* pFilter = (SIPFilter*)m_ipfilter.GetItemData(iItem);
		if (pFilter)
		{
			ULONG ulIPRange = pFilter->end - pFilter->start + 1;
			if (theApp.ipfilter->RemoveIPFilter(pFilter))
			{
				aItems.Add(iItem);
				m_ulFilteredIPs -= ulIPRange;
			}
		}
	}

	m_ipfilter.SetRedraw(FALSE);
	for (int i = aItems.GetCount() - 1; i >= 0; i--)
		m_ipfilter.DeleteItem(aItems[i]);
	if (aItems.GetCount() > 0)
	{
		int iNextSelItem = aItems[0];
		if (iNextSelItem >= m_ipfilter.GetItemCount())
			iNextSelItem--;
		if (iNextSelItem >= 0)
		{
			m_ipfilter.SetItemState(iNextSelItem, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			m_ipfilter.SetSelectionMark(iNextSelItem);
		}
	}
	m_ipfilter.SetRedraw();
	SetDlgItemText(IDC_TOTAL_IPFILTER, GetFormatedUInt(m_ipfilter.GetItemCount()));
	SetDlgItemText(IDC_TOTAL_IPS, GetFormatedUInt(m_ulFilteredIPs));
}

void CIPFilterDlg::OnDeleteIPFilter()
{
	OnBnClickedDelete();
}

void CIPFilterDlg::OnLvnKeyDownIPFilter(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	if (pLVKeyDow->wVKey == VK_DELETE)
		OnDeleteIPFilter();
	else if (pLVKeyDow->wVKey == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
		OnCopyIPFilter();
	*pResult = 0;
}

void CIPFilterDlg::OnLvnDeleteAllItemsIPfilter(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = TRUE;
}

void CIPFilterDlg::OnDestroy()
{
	m_ipfilter.WriteColumnStats(ARRSIZE(_aColumns), _aColumns);
	CResizableDialog::OnDestroy();
}

void CIPFilterDlg::OnBnClickedSave()
{
	CWaitCursor curWait;
	try
	{
		theApp.ipfilter->SaveToDefaultFile();
	}
	catch(CString err)
	{
		AfxMessageBox(err);
	}
}

void CIPFilterDlg::OnFind()
{
	m_ipfilter.OnFindStart();
}
