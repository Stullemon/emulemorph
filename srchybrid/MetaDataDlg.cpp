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
#include "kademlia/kademlia/tag.h"
#include "MetaDataDlg.h"
#include "OtherFunctions.h"
#include "Preferences.h"
#include "MenuCmds.h"
#include "Packets.h"
#include "KnownFile.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////////////
// COLUMN_INIT -- List View Columns

enum EMetaDataCols
{
	META_DATA_COL_NAME = 0,
	META_DATA_COL_TYPE,
	META_DATA_COL_VALUE
};

static LCX_COLUMN_INIT _aColumns[] =
{
	{ META_DATA_COL_NAME,	_T("Name"),		IDS_SW_NAME,	LVCFMT_LEFT,	-1, 0, ASCENDING,  NONE, _T("Temporary file MMMMM") },
	{ META_DATA_COL_TYPE,	_T("Type"),		IDS_TYPE,		LVCFMT_LEFT,	-1, 1, ASCENDING,  NONE, _T("Integer") },
	{ META_DATA_COL_VALUE,	_T("Value"),	IDS_VALUE,		LVCFMT_LEFT,	-1, 2, ASCENDING,  NONE, _T("long long long long long long long long file name.avi") }
};

#define	PREF_INI_SECTION	_T("MetaDataDlg")

// CMetaDataDlg dialog

IMPLEMENT_DYNAMIC(CMetaDataDlg, CResizablePage)

BEGIN_MESSAGE_MAP(CMetaDataDlg, CResizablePage)
	ON_NOTIFY(LVN_KEYDOWN, IDC_TAGS, OnLvnKeydownTags)
	ON_COMMAND(MP_COPYSELECTED, OnCopyTags)
	ON_COMMAND(MP_SELECTALL, OnSelectAllTags)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CMetaDataDlg::CMetaDataDlg()
	: CResizablePage(CMetaDataDlg::IDD, 0)
{
	m_file = NULL;
	m_taglist = NULL;
	m_strCaption = GetResString(IDS_META_DATA);
	m_psp.pszTitle = m_strCaption;
	m_psp.dwFlags |= PSP_USETITLE;
	m_pMenuTags = NULL;
	m_tags.m_pParent = this;
	m_tags.SetRegistryKey(PREF_INI_SECTION);
	m_tags.SetRegistryPrefix(_T("MetaDataTags_"));
}

CMetaDataDlg::~CMetaDataDlg()
{
	delete m_pMenuTags;
}

void CMetaDataDlg::SetTagList(Kademlia::TagList* taglist)
{
	m_taglist = taglist;
}

void CMetaDataDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizablePage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAGS, m_tags);
}

BOOL CMetaDataDlg::OnInitDialog()
{
	CResizablePage::OnInitDialog();
	InitWindowStyles(this);

	AddAnchor(IDC_TAGS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_TOTAL_TAGS, BOTTOM_LEFT, BOTTOM_RIGHT);

	m_tags.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_tags.ReadColumnStats(ARRSIZE(_aColumns), _aColumns);
	m_tags.CreateColumns(ARRSIZE(_aColumns), _aColumns);

	InitTags();

	m_pMenuTags = new CMenu();
	if (m_pMenuTags->CreatePopupMenu())
	{
		m_pMenuTags->AppendMenu(MF_ENABLED | MF_STRING, MP_COPYSELECTED, GetResString(IDS_COPY));
		m_pMenuTags->AppendMenu(MF_SEPARATOR);
		m_pMenuTags->AppendMenu(MF_ENABLED | MF_STRING, MP_SELECTALL, GetResString(IDS_SELECTALL));
	}
	m_tags.m_pMenu = m_pMenuTags;
	m_tags.m_pParent = this;

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

static const struct {
	UINT uID;
	LPCTSTR pszName;
} _aTagNames[] =
{
	{ FT_FILENAME,			_T("Name") },
	{ FT_FILESIZE,			_T("Size") },
	{ FT_FILETYPE,			_T("Type") },
	{ FT_FILEFORMAT,		_T("Format") },
	{ 0x05,					_T("Collection") },
	{ 0x06,					_T("Part path") },
	{ 0x07,					_T("Part hash") },
	{ FT_TRANSFERED,		_T("Transfered") },
	{ FT_GAPSTART,			_T("Gap start") },
	{ FT_GAPEND,			_T("Gap end") },
	{ 0x0B,					_T("Description") },
	{ 0x0C,					_T("Ping") },
	{ 0x0D,					_T("Fail") },
	{ 0x0E,					_T("Preference") },
	{ 0x0F,					_T("Port") },
	{ 0x10,					_T("IP") },
	{ 0x11,					_T("Version") },
	{ FT_PARTFILENAME,		_T("Temporary file") },
	{ 0x13,					_T("Priority") },
	{ FT_STATUS,			_T("Status") },
	{ FT_SOURCES,			_T("Availability") },
	//{ 0x16,				_T("QTime") },
	{ 0x16,					_T("Permission") },	// though not (never) used, this will be found in almost all known.met files
	{ 0x17,					_T("Parts") },
	{ FT_MEDIA_ARTIST,		_T("Artist") },
	{ FT_MEDIA_ALBUM,		_T("Album") },
	{ FT_MEDIA_TITLE,		_T("Title") },
	{ FT_MEDIA_LENGTH,		_T("Length") },
	{ FT_MEDIA_BITRATE,		_T("Bitrate") },
	{ FT_MEDIA_CODEC,		_T("Codec") },
	{ 0xFA,					_T("Server port") },
	{ 0xFB,					_T("Server IP") },
	{ 0xFC,					_T("Source UDP port") },
	{ 0xFD,					_T("Source TCP port") },
	{ 0xFE,					_T("Source IP") },
	{ 0xFF,					_T("Source type") }
};

CString GetName(const CTag* pTag)
{
	CString strName;
	if (pTag->tag.specialtag)
	{
		for (int i = 0; i < ARRSIZE(_aTagNames); i++){
			if (pTag->tag.specialtag == _aTagNames[i].uID){
				strName = _aTagNames[i].pszName;
				break;
			}
		}
		if (strName.IsEmpty())
			strName.Format(_T("Tag 0x%02X"), pTag->tag.specialtag);
	}
	else
		strName = pTag->tag.tagname;
	return strName;
}

CString GetName(const Kademlia::CTag* pTag)
{
	CString strName;
	if (pTag->m_name.GetLength() == 1)
	{
		for (int i = 0; i < ARRSIZE(_aTagNames); i++){
			if ((BYTE)pTag->m_name[0] == _aTagNames[i].uID){
				strName = _aTagNames[i].pszName;
				break;
			}
		}
		if (strName.IsEmpty())
			strName.Format(_T("Tag 0x%02X"), (BYTE)pTag->m_name[0]);
	}
	else
		strName = pTag->m_name;
	return strName;
}

CString GetValue(const CTag* pTag)
{
	CString strValue;
	if (pTag->tag.type == 2)
		strValue = pTag->tag.stringvalue;
	else if (pTag->tag.type == 3)
	{
		if (pTag->tag.specialtag == 0x10 || pTag->tag.specialtag >= 0xFA)
			strValue.Format(_T("%u"), pTag->tag.intvalue);
		else if (pTag->tag.specialtag == FT_MEDIA_LENGTH)
			SecToTimeLength(pTag->tag.intvalue, strValue);
		else
			strValue = GetFormatedUInt(pTag->tag.intvalue);
	}
	else if (pTag->tag.type == 4)
		strValue.Format(_T("%f"), pTag->tag.floatvalue);
	else
		strValue.Format(_T("<Unknown value of type 0x%02X>"), pTag->tag.type);
	return strValue;
}

CString GetValue(const Kademlia::CTag* pTag)
{
	CString strValue;
	if (pTag->IsStr())
		strValue = pTag->GetStr();
	else if (pTag->IsInt())
	{
		if ((BYTE)pTag->m_name[0] == 0x10 || (BYTE)pTag->m_name[0] > 0xFA)
			strValue.Format(_T("%u"), pTag->GetInt());
		else if (pTag->m_name == TAG_MEDIA_LENGTH)
			SecToTimeLength(pTag->GetInt(), strValue);
		else
			strValue = GetFormatedUInt(pTag->GetInt());
	}
	else if (pTag->m_type == 4)
		strValue.Format(_T("%f"), pTag->GetFloat());
	else if (pTag->m_type == 5)
		strValue.Format(_T("%u"), pTag->GetInt());
	else
		strValue.Format(_T("<Unknown value of type 0x%02X>"), pTag->m_type);
	return strValue;
}

CString GetType(UINT uType)
{
	CString strValue;
	if (uType == 2)
		strValue = _T("String");
	else if (uType == 3)
		strValue = _T("Int32");
	else if (uType == 4)
		strValue = _T("Float");
	else if (uType == 5)
		strValue = _T("Bool");
	else if (uType == 8)
		strValue = _T("Int16");
	else if (uType == 9)
		strValue = _T("Int8");
	else
		strValue.Format(_T("<Unknown type 0x%02X>"), uType);
	return strValue;
}

void CMetaDataDlg::InitTags()
{
	CWaitCursor curWait;
	m_tags.DeleteAllItems();
	m_tags.SetRedraw(FALSE);

	if (m_file != NULL)
	{
		const CArray<CTag*,CTag*>& aTags = m_file->GetTags();
		int iTags = aTags.GetCount();
		for (int i = 0; i < iTags; i++)
		{
			const CTag* pTag = aTags.GetAt(i);
			CString strBuff;
			LVITEM lvi;
			lvi.mask = LVIF_TEXT;
			lvi.iItem = INT_MAX;
			lvi.iSubItem = META_DATA_COL_NAME;
			strBuff = GetName(pTag);
			lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
			int iItem = m_tags.InsertItem(&lvi);
			if (iItem >= 0)
			{
				lvi.mask = LVIF_TEXT;
				lvi.iItem = iItem;

				strBuff = GetType(pTag->tag.type);
				lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
				lvi.iSubItem = META_DATA_COL_TYPE;
				m_tags.SetItem(&lvi);

				strBuff = GetValue(pTag);
				lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
				lvi.iSubItem = META_DATA_COL_VALUE;
				m_tags.SetItem(&lvi);
			}
		}
	}
	else if (m_taglist != NULL)
	{
		const Kademlia::CTag* pTag;
		Kademlia::TagList::const_iterator it;
		for (it = m_taglist->begin(); it != m_taglist->end(); it++)
		{
			pTag = *it;
			CString strBuff;
			LVITEM lvi;
			lvi.mask = LVIF_TEXT;
			lvi.iItem = INT_MAX;
			lvi.iSubItem = META_DATA_COL_NAME;
			strBuff = GetName(pTag);
			lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
			int iItem = m_tags.InsertItem(&lvi);
			if (iItem >= 0)
			{
				lvi.mask = LVIF_TEXT;
				lvi.iItem = iItem;

				strBuff = GetType(pTag->m_type);
				lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
				lvi.iSubItem = META_DATA_COL_TYPE;
				m_tags.SetItem(&lvi);

				strBuff = GetValue(pTag);
				lvi.pszText = const_cast<LPTSTR>((LPCTSTR)strBuff);
				lvi.iSubItem = META_DATA_COL_VALUE;
				m_tags.SetItem(&lvi);
			}
		}
	}
	CString strTmp;
	strTmp.Format(_T("Total tags: %u"), m_tags.GetItemCount());
	SetDlgItemText(IDC_TOTAL_TAGS, strTmp);
	m_tags.SetRedraw();
}

void CMetaDataDlg::OnCopyTags()
{
	CWaitCursor curWait;
	int iSelected = 0;
	CString strData;
	POSITION pos = m_tags.GetFirstSelectedItemPosition();
	while (pos)
	{
		int iItem = m_tags.GetNextSelectedItem(pos);
		CString strValue = m_tags.GetItemText(iItem, META_DATA_COL_VALUE);

		if (!strValue.IsEmpty())
		{
			if (!strData.IsEmpty())
				strData += _T("\r\n");
			strData += strValue;
			iSelected++;
		}
	}

	if (!strData.IsEmpty()){
		if (iSelected > 1)
			strData += _T("\r\n");
		theApp.CopyTextToClipboard(strData);
	}
}

void CMetaDataDlg::OnSelectAllTags()
{
	m_tags.SelectAllItems();
}

void CMetaDataDlg::OnLvnKeydownTags(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDown = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);

	if (pLVKeyDown->wVKey == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
		OnCopyTags();
	*pResult = 0;
}

void CMetaDataDlg::OnDestroy()
{
	m_tags.WriteColumnStats(ARRSIZE(_aColumns), _aColumns);
	CResizablePage::OnDestroy();
}
