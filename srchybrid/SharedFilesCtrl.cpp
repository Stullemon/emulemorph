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
#include "emuledlg.h"
#include "SharedFilesCtrl.h"
#include "OtherFunctions.h"
#include "CommentDialog.h"
#include "FileInfoDialog.h"
#include "MetaDataDlg.h"
#include "ResizableLib/ResizableSheet.h"
#include "KnownFile.h"
#include "MapKey.h"
#include "SharedFileList.h"
#include "MemDC.h"
#include "PartFile.h"
#include "MenuCmds.h"
#include "IrcWnd.h"
#include "SharedFilesWnd.h"
#include "Opcodes.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



//////////////////////////////////////////////////////////////////////////////
// CSharedFileDetailsSheet

class CSharedFileDetailsSheet : public CResizableSheet
{
	DECLARE_DYNAMIC(CSharedFileDetailsSheet)

public:
	CSharedFileDetailsSheet(CKnownFile* file);
	virtual ~CSharedFileDetailsSheet();

protected:
	CKnownFile* m_file;
	CFileInfoDialog m_wndMediaInfo;
	CMetaDataDlg m_wndMetaData;

	static int sm_iLastActivePage;

	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
	afx_msg void OnDestroy();
};

int CSharedFileDetailsSheet::sm_iLastActivePage;

IMPLEMENT_DYNAMIC(CSharedFileDetailsSheet, CResizableSheet)

BEGIN_MESSAGE_MAP(CSharedFileDetailsSheet, CResizableSheet)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

CSharedFileDetailsSheet::CSharedFileDetailsSheet(CKnownFile* file)
{
	m_file = file;
	m_psh.dwFlags &= ~PSH_HASHELP;
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	
	m_wndMediaInfo.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;

	m_wndMediaInfo.SetMyfile(file);
	if (theApp.glob_prefs->IsExtControlsEnabled())
		m_wndMetaData.SetFile(file);

	AddPage(&m_wndMediaInfo);
	if (theApp.glob_prefs->IsExtControlsEnabled())
		AddPage(&m_wndMetaData);
}

CSharedFileDetailsSheet::~CSharedFileDetailsSheet()
{
}

void CSharedFileDetailsSheet::OnDestroy()
{
	sm_iLastActivePage = GetActiveIndex();
	CResizableSheet::OnDestroy();
}

BOOL CSharedFileDetailsSheet::OnInitDialog()
{		
	EnableStackedTabs(FALSE);
	BOOL bResult = CResizableSheet::OnInitDialog();
	InitWindowStyles(this);
	EnableSaveRestore(_T("SharedFileDetailsSheet")); // call this after(!) OnInitDialog
	SetWindowText(GetResString(IDS_DETAILS) + _T(": ") + m_file->GetFileName());
	if (sm_iLastActivePage < GetPageCount())
		SetActivePage(sm_iLastActivePage);
	return bResult;
}


//////////////////////////////////////////////////////////////////////////////
// CSharedFilesCtrl

IMPLEMENT_DYNAMIC(CSharedFilesCtrl, CMuleListCtrl)
CSharedFilesCtrl::CSharedFilesCtrl() {
   sflist = 0;                // i_a 
   memset(&sortstat, 0, sizeof(sortstat));  // i_a 
}

CSharedFilesCtrl::~CSharedFilesCtrl(){
}

void CSharedFilesCtrl::Init(){
	CImageList ilDummyImageList; //dummy list for getting the proper height of listview entries
	ilDummyImageList.Create(1, theApp.GetSmallSytemIconSize().cy,theApp.m_iDfltImageListColorFlags|ILC_MASK, 1, 1); 
	SetImageList(&ilDummyImageList, LVSIL_SMALL);
	ASSERT( (GetStyle() & LVS_SHAREIMAGELISTS) == 0 );
	ilDummyImageList.Detach();

	SetExtendedStyle(LVS_EX_FULLROWSELECT);
	ModifyStyle(LVS_SINGLESEL,0);
	InsertColumn(0, GetResString(IDS_DL_FILENAME) ,LVCFMT_LEFT,250,0);
	InsertColumn(1,GetResString(IDS_DL_SIZE),LVCFMT_LEFT,100,1);
	InsertColumn(2,GetResString(IDS_TYPE),LVCFMT_LEFT,50,2);
	InsertColumn(3,GetResString(IDS_PRIORITY),LVCFMT_LEFT,70,3);
	InsertColumn(4,GetResString(IDS_PERMISSION),LVCFMT_LEFT,100,4);
	InsertColumn(5,GetResString(IDS_FILEID),LVCFMT_LEFT,220,5);
	InsertColumn(6,GetResString(IDS_SF_REQUESTS),LVCFMT_LEFT,100,6);
	InsertColumn(7,GetResString(IDS_SF_ACCEPTS),LVCFMT_LEFT,100,7);
	InsertColumn(8,GetResString(IDS_SF_TRANSFERRED),LVCFMT_LEFT,120,8);
	InsertColumn(9,GetResString(IDS_UPSTATUS),LVCFMT_LEFT,100,9);
	InsertColumn(10,GetResString(IDS_FOLDER),LVCFMT_LEFT,200,10);
	InsertColumn(11,GetResString(IDS_COMPLSOURCES),LVCFMT_LEFT,100,11);
	InsertColumn(12,GetResString(IDS_SHAREDTITLE),LVCFMT_LEFT,200,12);
	//MORPH START - Added by SiRoB, ZZ Upload System
	InsertColumn(13,GetResString(IDS_POWERSHARE_COLUMN_LABEL),LVCFMT_LEFT,70,13);
	//MORPH END - Added by SiRoB, ZZ Upload System
	//MORPH START - Added by IceCream, SLUGFILLER: showComments
	//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars
	InsertColumn(14,GetResString(IDS_SF_UPLOADED_PARTS),LVCFMT_LEFT,170,14); // SF
	InsertColumn(15,GetResString(IDS_SF_TURN_PART),LVCFMT_LEFT,100,15); // SF
	InsertColumn(16,GetResString(IDS_SF_TURN_SIMPLE),LVCFMT_LEFT,100,16); // VQB
	InsertColumn(17,GetResString(IDS_SF_FULLUPLOAD),LVCFMT_LEFT,100,17); // SF
	//MORPH END   - Added & Modified by IceCream, SLUGFILLER: Spreadbars
	m_ImageList.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,2);
	m_ImageList.SetBkColor(CLR_NONE);
	m_ImageList.Add(theApp.LoadIcon("RATING_NO"));  // 0
	m_ImageList.Add(theApp.LoadIcon("RATING_FAKE"));  // 1
	m_ImageList.Add(theApp.LoadIcon("RATING_POOR"));  // 2
	m_ImageList.Add(theApp.LoadIcon("RATING_GOOD"));  // 3
	m_ImageList.Add(theApp.LoadIcon("RATING_FAIR"));  // 4
	m_ImageList.Add(theApp.LoadIcon("RATING_EXCELLENT"));  // 5
	//MORPH END   - Added & Moddified by IceCream, SLUGFILLER: showComments
	CreateMenues();

	LoadSettings(CPreferences::tableShared);

	// Barry - Use preferred sort order from preferences
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableShared);
	bool sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableShared);
	SetSortArrow(sortItem, sortAscending);
	// SLUGFILLER: multiSort - load multiple params
	for (int i = theApp.glob_prefs->GetColumnSortCount(CPreferences::tableShared); i > 0; ) {
		i--;
		sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableShared, i);
		sortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableShared, i);
		SortItems(SortProc, sortItem + (sortAscending ? 0:20));
	}
	// SLUGFILLER: multiSort
}

void CSharedFilesCtrl::Localize() {
	CHeaderCtrl* pHeaderCtrl = GetHeaderCtrl();
	HDITEM hdi;
	hdi.mask = HDI_TEXT;
	CString strRes;

	strRes = GetResString(IDS_DL_FILENAME);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(0, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_DL_SIZE);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(1, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_TYPE);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(2, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_PRIORITY);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(3, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_PERMISSION);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(4, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_FILEID);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(5, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_SF_REQUESTS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(6, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_SF_ACCEPTS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(7, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_SF_TRANSFERRED);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(8, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_UPSTATUS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(9, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_FOLDER);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(10, &hdi);
	strRes.ReleaseBuffer();

	// #zegzav:completesrc
	strRes = GetResString(IDS_COMPLSOURCES);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(11, &hdi);
	strRes.ReleaseBuffer();
	// #zegzav:completesrc END

	//MORPH START - Added  by SiRoB, ZZ Upload System
	strRes = GetResString(IDS_POWERSHARE_COLUMN_LABEL);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(13, &hdi);
	strRes.ReleaseBuffer();
	//MORPH END - Added  by SiRoB, ZZ Upload System

	//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars
	strRes = GetResString(IDS_SF_UPLOADED_PARTS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(14, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_SF_TURN_PART);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(15, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_SF_TURN_SIMPLE);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(16, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_SF_FULLUPLOAD);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(17, &hdi);
	strRes.ReleaseBuffer();
	//MORPH END - Added by IceCream SLUGFILLER: Spreadbars

	CreateMenues();
}

void CSharedFilesCtrl::ShowFileList(CSharedFileList* in_sflist){
	DeleteAllItems();
	sflist = in_sflist;
	CCKey bufKey;
	CKnownFile* cur_file;
	for (POSITION pos = sflist->m_Files_map.GetStartPosition();pos != 0;){
		sflist->m_Files_map.GetNextAssoc(pos,bufKey,cur_file);
		ShowFile(cur_file);
	}
	ShowFilesCount();
}

#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)

void CSharedFilesCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct){
	if( !theApp.emuledlg->IsRunning() )
		return;
	if( !lpDrawItemStruct->itemData)
		return;
	CDC* odc = CDC::FromHandle(lpDrawItemStruct->hDC);
	BOOL bCtrlFocused = ((GetFocus() == this ) || (GetStyle() & LVS_SHOWSELALWAYS));
	if( odc && (lpDrawItemStruct->itemAction | ODA_SELECT) && (lpDrawItemStruct->itemState & ODS_SELECTED )){
		if(bCtrlFocused)
			odc->SetBkColor(m_crHighlight);
		else
			odc->SetBkColor(m_crNoHighlight);
	}
	else
		odc->SetBkColor(m_crWindow);

	CKnownFile* file = (CKnownFile*)lpDrawItemStruct->itemData;
	CMemDC dc(CDC::FromHandle(lpDrawItemStruct->hDC),&CRect(lpDrawItemStruct->rcItem));
	CFont* pOldFont = dc.SelectObject(GetFont());
	RECT cur_rec;
	memcpy(&cur_rec,&lpDrawItemStruct->rcItem,sizeof(RECT));
	COLORREF crOldTextColor = dc.SetTextColor(m_crWindowText);

	int iOldBkMode;
	if (m_crWindowTextBk == CLR_NONE){
		DefWindowProc(WM_ERASEBKGND, (WPARAM)(HDC)dc, 0);
		iOldBkMode = dc.SetBkMode(TRANSPARENT);
	}
	else
		iOldBkMode = OPAQUE;

	CString buffer;

	CHeaderCtrl *pHeaderCtrl = GetHeaderCtrl();
	int iCount = pHeaderCtrl->GetItemCount();
	const int iMarginX = 4;
	cur_rec.right = cur_rec.left - iMarginX*2;
	cur_rec.left += iMarginX;

	int iIconDrawWidth = theApp.GetSmallSytemIconSize().cx + 3;
	for(int iCurrent = 0; iCurrent < iCount; iCurrent++){
		int iColumn = pHeaderCtrl->OrderToIndex(iCurrent);
		if( !IsColumnHidden(iColumn) ){
			int next_left = cur_rec.left + GetColumnWidth(iColumn);	//MORPH - Added by IceCream, SLUGFILLER: showComments - some modular coding
			UINT uDTFlags = DLC_DT_TEXT;
			cur_rec.right += GetColumnWidth(iColumn);
			//MORPH START - Added by SiRoB, Don't draw hidden colums
			CRect Rect;
			this->GetClientRect(Rect);
			Rect.IntersectRect(Rect, &cur_rec);
			if (!Rect.IsRectEmpty()){
			//MORPH END   - Added by SiRoB, Don't draw hidden colums
					//MORPH - Moved by SiRoB, due to the draw system change on hidden rect
					// xMule_MOD: showSharePermissions, modified by itsonlyme
					// display not finished files in navy, blocked files in red and friend-only files in orange
					if (file->GetPermissions() == PERM_NOONE)
						dc->SetTextColor((COLORREF)RGB(240,0,0));
					else if (file->GetPermissions() == PERM_FRIENDS)
						dc->SetTextColor((COLORREF)RGB(208,128,0));
					else if (file->IsPartFile())
						dc->SetTextColor((COLORREF)RGB(0,0,192));
					// xMule_MOD: showSharePermissions
					switch(iColumn){
					case 0:{
						int iImage = theApp.GetFileTypeSystemImageIdx(file->GetFileName());
						if (theApp.GetSystemImageList() != NULL)
						::ImageList_Draw(theApp.GetSystemImageList(), iImage, dc->GetSafeHdc(), cur_rec.left, cur_rec.top, ILD_NORMAL|ILD_TRANSPARENT);
						cur_rec.left += iIconDrawWidth;
						buffer = file->GetFileName();
						//MORPH START - Added by IceCream, SLUGFILLER: showComments
						if ( theApp.glob_prefs->ShowRatingIndicator() && ( !file->GetFileComment().IsEmpty() || file->GetFileRate() )){ //Modified by IceCream, eMule plus rating icon
							POINT point= {cur_rec.left-4,cur_rec.top+2};
							int the_rating=0;
							if (file->GetFileRate())
								the_rating = file->GetFileRate();
							m_ImageList.Draw(dc,
								file->GetFileRate(), //Modified by IceCream, eMule plus rating icon
								point,
								ILD_NORMAL);
						}
						cur_rec.left+=9; //Modified by IceCream, eMule plus rating icon
						//MORPH END   - Added by IceCream, SLUGFILLER: showComments
						break;
					}
					case 1:
						buffer = CastItoXBytes(file->GetFileSize());
						uDTFlags |= DT_RIGHT;
						break;
					case 2:
						buffer = file->GetFileType();
						break;
					case 3:{
						switch (file->GetUpPriority()) {
							case PR_VERYLOW :
								buffer = GetResString(IDS_PRIOVERYLOW);
								break;
							case PR_LOW :
								if( file->IsAutoUpPriority() )
									buffer = GetResString(IDS_PRIOAUTOLOW);
								else
									buffer = GetResString(IDS_PRIOLOW);
								break;
							case PR_NORMAL :
								if( file->IsAutoUpPriority() )
									buffer = GetResString(IDS_PRIOAUTONORMAL);
								else
									buffer = GetResString(IDS_PRIONORMAL);
								break;
							case PR_HIGH :
								if( file->IsAutoUpPriority() )
									buffer = GetResString(IDS_PRIOAUTOHIGH);
								else
									buffer = GetResString(IDS_PRIOHIGH);
								break;
							case PR_VERYHIGH :
								buffer = GetResString(IDS_PRIORELEASE);
								break;
							default:
								buffer.Empty();
						}
						//MORPH START - Added by SiRoB, Powershare State in prio colums
						if(file->GetPowerShared()) {
							CString tempString = GetResString(IDS_POWERSHARE_PREFIX);
							tempString.Append(" ");
							tempString.Append(buffer);
							buffer.Empty();
							buffer = tempString;
						}
						//MORPH END   - Added by SiRoB, Powershare State in prio colums
						break;
					}
					case 4:
						// xMule_MOD: showSharePermissions
						switch (file->GetPermissions())
						{
							case PERM_NOONE: 
								buffer = GetResString(IDS_HIDDEN); 
								break;
							case PERM_FRIENDS: 
								buffer = GetResString(IDS_FSTATUS_FRIENDSONLY); 
								break;
							default: 
								buffer = GetResString(IDS_FSTATUS_PUBLIC);
								break;
						}
						// xMule_MOD: showSharePermissions
					break;
					case 5:
						buffer = md4str(file->GetFileHash());
						break;
					case 6:
						buffer.Format("%u (%u)",file->statistic.GetRequests(),file->statistic.GetAllTimeRequests());
						break;
					case 7:
						buffer.Format("%u (%u)",file->statistic.GetAccepts(),file->statistic.GetAllTimeAccepts());
						break;
					case 8:
						buffer.Format("%s (%s)",CastItoXBytes(file->statistic.GetTransferred()), CastItoXBytes(file->statistic.GetAllTimeTransferred()));
						break;
					case 9:
						if( file->GetPartCount()){
							cur_rec.bottom--;
							cur_rec.top++;
							if(!file->IsPartFile())
								file->DrawShareStatusBar(dc,&cur_rec,false,theApp.glob_prefs->UseFlatBar());
							else
								((CPartFile*)file)->DrawShareStatusBar(dc,&cur_rec,false,theApp.glob_prefs->UseFlatBar());
							cur_rec.bottom++;
							cur_rec.top--;
						}
						break;
					case 10:
						buffer = file->GetPath();
						PathRemoveBackslash(buffer.GetBuffer());
						buffer.ReleaseBuffer();
						break;
					case 11:
					{
						if (file->m_nCompleteSourcesCountLo == 0)
						{
							if (file->m_nCompleteSourcesCountHi == 0)
							{
								buffer= "?";
							}
							else
							{
								buffer.Format("< %u", file->m_nCompleteSourcesCountHi);
							}
						}
						else if (file->m_nCompleteSourcesCountLo == file->m_nCompleteSourcesCountHi)
						{
							buffer.Format("%u", file->m_nCompleteSourcesCountLo);
						}
						else
						{
							buffer.Format("%u - %u", file->m_nCompleteSourcesCountLo, file->m_nCompleteSourcesCountHi);
						}
						//MORPH START - Added by SiRoB, Avoid misusing of powersharing
						CString buffer2;
						buffer2.Format(" (%u)",file->m_nVirtualCompleteSourcesCount);
						buffer.Append(buffer2);
						//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
						break;
					}
					case 12:{
						CString ed2k;
						if(file->GetPublishedED2K())
							buffer = GetResString(IDS_YES);
						else
							buffer = GetResString(IDS_NO);
						buffer += "|";
						if( (uint32)time(NULL)-file->GetLastPublishTimeKadSrc() < KADEMLIAREPUBLISHTIME )
							buffer += GetResString(IDS_YES);
						else
							buffer += GetResString(IDS_NO);
						break;
					}
					//MORPH START - Added by SiRoB, ZZ Upload System
					case 13:{
						//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
						int powersharemode = file->GetPowerSharedMode();
						bool powershared = file->GetPowerShared();
						buffer = "[" + GetResString((powershared)?IDS_POWERSHARE_ON_LABEL:IDS_POWERSHARE_OFF_LABEL) + "] ";
						if(powersharemode == 2)
							buffer.Append(GetResString(IDS_POWERSHARE_AUTO_LABEL));
						else if (powersharemode == 1)
							buffer.Append(GetResString(IDS_POWERSHARE_ACTIVATED_LABEL));
						else
							buffer.Append(GetResString(IDS_POWERSHARE_DISABLED_LABEL));
						buffer.Append(" (");
						if (file->GetPowerShareAuto())
							buffer.Append(GetResString(IDS_POWERSHARE_ADVISED_LABEL));
						else if (file->GetPowerShareAuthorized())
							buffer.Append(GetResString(IDS_POWERSHARE_AUTHORIZED_LABEL));
						else
							buffer.Append(GetResString(IDS_POWERSHARE_DENIED_LABEL));
						buffer.Append(")");
						//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
						break;
					}
					//MORPH END   - Added by SiRoB, ZZ Upload System
					// SLUGFILLER: Spreadbars
					case 14:
						{
							cur_rec.bottom--;
							cur_rec.top++;
							file->statistic.DrawSpreadBar(dc,&cur_rec,theApp.glob_prefs->UseFlatBar());
							cur_rec.bottom++;
							cur_rec.top--;
							cur_rec.left = next_left;
						}
						continue;
					case 15:
						buffer.Format("%.2f",file->statistic.GetSpreadSortValue());
						break;
					case 16:
						if (file->GetFileSize())
							buffer.Format("%.2f",((float)file->statistic.GetAllTimeTransferred())/((float)file->GetFileSize()));
						else
							buffer.Format("%.2f",0.0f);
						break;
					case 17:
						buffer.Format("%.2f",file->statistic.GetFullSpreadCount());
						break;
					// SLUGFILLER: Spreadbars
				}
				if( iColumn != 9 )
					dc->DrawText(buffer, buffer.GetLength(),&cur_rec,uDTFlags);
			//MORPH START - Added by SiRoB, Don't draw hidden coloms
			}
			//MORPH END   - Added by SiRoB, Don't draw hidden coloms
			cur_rec.left = next_left;	//MORPH - Added by IceCream, SLUGFILLER: showComments - some modular coding
		}
	}
	ShowFilesCount();
	if ((lpDrawItemStruct->itemAction | ODA_SELECT) && (lpDrawItemStruct->itemState & ODS_SELECTED))
	{
		RECT outline_rec;
		memcpy(&outline_rec,&lpDrawItemStruct->rcItem,sizeof(RECT));

		outline_rec.top--;
		outline_rec.bottom++;
		dc->FrameRect(&outline_rec, &CBrush(m_crWindow));
		outline_rec.top++;
		outline_rec.bottom--;
		outline_rec.left++;
		outline_rec.right--;

		if (lpDrawItemStruct->itemID > 0 && GetItemState(lpDrawItemStruct->itemID - 1, LVIS_SELECTED))
			outline_rec.top--;

		if (lpDrawItemStruct->itemID + 1 < (UINT)GetItemCount() && GetItemState(lpDrawItemStruct->itemID + 1, LVIS_SELECTED))
			outline_rec.bottom++;

		if(bCtrlFocused)
			dc->FrameRect(&outline_rec, &CBrush(m_crFocusLine));
		else
			dc->FrameRect(&outline_rec, &CBrush(m_crNoFocusLine));
	}
	
	if (m_crWindowTextBk == CLR_NONE)
		dc.SetBkMode(iOldBkMode);
	dc.SelectObject(pOldFont);
	dc.SetTextColor(crOldTextColor);
}

void CSharedFilesCtrl::ShowFile(CKnownFile* file){
	uint32 itemnr = GetItemCount();
	itemnr = InsertItem(LVIF_TEXT|LVIF_PARAM,itemnr,LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)file);
	UpdateFile(file);
}

void CSharedFilesCtrl::RemoveFile(CKnownFile *toRemove) {
	LVFINDINFO info;
	info.flags = LVFI_PARAM;
	info.lParam = (LPARAM)toRemove;
	int nItem = FindItem(&info);
	if(nItem != -1)
		DeleteItem(nItem);
	ShowFilesCount();
}

BEGIN_MESSAGE_MAP(CSharedFilesCtrl, CMuleListCtrl)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnGetDispInfo)
END_MESSAGE_MAP()



// CSharedFilesCtrl message handlers
void CSharedFilesCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	CKnownFile* file = NULL;
	UINT uFlags = MF_GRAYED;
	if (iSel != -1){
		file = (CKnownFile*)GetItemData(iSel);
		uFlags = MF_ENABLED;
	}

	m_PrioMenu.EnableMenuItem(MP_PRIOVERYLOW, uFlags);
	m_PrioMenu.EnableMenuItem(MP_PRIOLOW, uFlags);
	m_PrioMenu.EnableMenuItem(MP_PRIONORMAL, uFlags);
	m_PrioMenu.EnableMenuItem(MP_PRIOHIGH, uFlags);
	m_PrioMenu.EnableMenuItem(MP_PRIOVERYHIGH, uFlags);
	m_PrioMenu.EnableMenuItem(MP_PRIOAUTO, uFlags);

	m_PermMenu.EnableMenuItem(MP_PERMNONE, uFlags);
	m_PermMenu.EnableMenuItem(MP_PERMFRIENDS, uFlags);
	m_PermMenu.EnableMenuItem(MP_PERMALL, uFlags);

	//MORPH START- Added by SiRoB, Avoid misusing of powershare
	m_PowershareMenu.EnableMenuItem(MP_POWERSHARE_OFF, uFlags);
	m_PowershareMenu.EnableMenuItem(MP_POWERSHARE_ON, uFlags);
	m_PowershareMenu.EnableMenuItem(MP_POWERSHARE_AUTO, uFlags);
	if (GetSelectedCount()==1){
		int powersharemode = file->GetPowerSharedMode();
		m_PowershareMenu.CheckMenuItem(MP_POWERSHARE_OFF, (powersharemode == 0) ? MF_CHECKED : MF_UNCHECKED);
		m_PowershareMenu.CheckMenuItem(MP_POWERSHARE_ON, (powersharemode == 1) ? MF_CHECKED : MF_UNCHECKED);
		m_PowershareMenu.CheckMenuItem(MP_POWERSHARE_AUTO, (powersharemode == 2) ? MF_CHECKED : MF_UNCHECKED);
		// xMule_MOD: showSharePermissions
		m_PermMenu.CheckMenuItem(MP_PERMALL, ((file->GetPermissions() == PERM_ALL) ? MF_CHECKED : MF_UNCHECKED));
		m_PermMenu.CheckMenuItem(MP_PERMFRIENDS, ((file->GetPermissions() == PERM_FRIENDS) ? MF_CHECKED : MF_UNCHECKED));
		m_PermMenu.CheckMenuItem(MP_PERMNONE, ((file->GetPermissions() == PERM_NOONE) ? MF_CHECKED : MF_UNCHECKED));
		// xMule_MOD: showSharePermissions
	}else{
		m_PowershareMenu.CheckMenuItem(MP_POWERSHARE_OFF, MF_UNCHECKED);
		m_PowershareMenu.CheckMenuItem(MP_POWERSHARE_ON, MF_UNCHECKED);
		m_PowershareMenu.CheckMenuItem(MP_POWERSHARE_AUTO, MF_UNCHECKED);
		// xMule_MOD: showSharePermissions
		m_PermMenu.CheckMenuItem(MP_PERMALL, MF_UNCHECKED);
		m_PermMenu.CheckMenuItem(MP_PERMFRIENDS,  MF_UNCHECKED);
		m_PermMenu.CheckMenuItem(MP_PERMNONE,  MF_UNCHECKED);
		// xMule_MOD: showSharePermissions
	}
	//MORPH END  - Added by SiRoB, Avoid misusing of powershare

	m_SharedFilesMenu.EnableMenuItem(MP_OPEN, (file!=NULL && !file->IsPartFile())?MF_ENABLED:MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(MP_CMT, uFlags); 
	m_SharedFilesMenu.EnableMenuItem(MP_DETAIL, uFlags);

	m_SharedFilesMenu.EnableMenuItem(MP_GETED2KLINK, uFlags);
	m_SharedFilesMenu.EnableMenuItem(MP_GETHTMLED2KLINK, uFlags);
	m_SharedFilesMenu.EnableMenuItem(MP_GETSOURCEED2KLINK, (file && theApp.IsConnected() && !theApp.IsFirewalled()) ? MF_ENABLED : MF_GRAYED);

	// itsonlyme: hostnameSource
	if (file && theApp.IsConnected() && !theApp.IsFirewalled() &&
		!CString(theApp.glob_prefs->GetYourHostname()).IsEmpty() &&
		CString(theApp.glob_prefs->GetYourHostname()).Find(".") != -1)
		m_SharedFilesMenu.EnableMenuItem(MP_GETHOSTNAMESOURCEED2KLINK, MF_ENABLED);
	else
		m_SharedFilesMenu.EnableMenuItem(MP_GETHOSTNAMESOURCEED2KLINK, MF_GRAYED);
	// itsonlyme: hostnameSource

	m_SharedFilesMenu.EnableMenuItem(Irc_SetSendLink, file && theApp.emuledlg->ircwnd->IsConnected() ? MF_ENABLED : MF_GRAYED);

	int counter;
	CMenu Web;
	Web.CreateMenu();
	UpdateURLMenu(Web,counter);
	UINT flag2 = ((counter == 0) || !file) ? MF_GRAYED : MF_STRING;
	m_SharedFilesMenu.AppendMenu(flag2 | MF_POPUP, (UINT_PTR)Web.m_hMenu, GetResString(IDS_WEBSERVICES));
	
	GetPopupMenuPos(*this, point);
	m_SharedFilesMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON,point.x,point.y,this);

	m_SharedFilesMenu.RemoveMenu(m_SharedFilesMenu.GetMenuItemCount()-1,MF_BYPOSITION);
	VERIFY( Web.DestroyMenu() );
}

BOOL CSharedFilesCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	UINT selectedCount = this->GetSelectedCount(); 
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);

	if (selectedCount>0){
		// itsonlyme: selFix
		CArray<CKnownFile*> arraySelFiles;
		POSITION pos = GetFirstSelectedItemPosition();
		while (pos != NULL)
		{
			int iSel = GetNextSelectedItem(pos);
			arraySelFiles.Add((CKnownFile*)GetItemData(iSel));
		}
		CKnownFile* file = arraySelFiles[0];
		// itsonlyme: selFix
		if (wParam>=MP_WEBURL && wParam<=MP_WEBURL+256) {
			RunURL(file, theApp.webservices.GetAt(wParam-MP_WEBURL) );
		}

		switch (wParam){
			case Irc_SetSendLink:
			{
				theApp.emuledlg->ircwnd->SetSendFileString(CreateED2kLink(file));
				break;
			}
			case MP_GETED2KLINK:{
				if(selectedCount > 1)
				{
					CString str;
					for (int i = 0; i < arraySelFiles.GetSize(); i++)	// itsonlyme: selFix
					{
						file = arraySelFiles[i];	// itsonlyme: selFix
						str.Append(CreateED2kLink(file) + "\n"); 
					}

					theApp.CopyTextToClipboard(str);
					//AfxMessageBox(GetResString(IDS_COPIED2CB) + str);
					break; 
				}
				theApp.CopyTextToClipboard(CreateED2kLink(file));
				break;
			}
			case MP_GETHTMLED2KLINK:
				if(selectedCount > 1)
				{
					CString str;
					for (int i = 0; i < arraySelFiles.GetSize(); i++)	// itsonlyme: selFix
					{
						file = arraySelFiles[i];	// itsonlyme: selFix
						str += CreateHTMLED2kLink(file) + "\n"; 
					}
					theApp.CopyTextToClipboard(str);
					//AfxMessageBox(GetResString(IDS_COPIED2CB) + str);
					break; 
				} 
				theApp.CopyTextToClipboard(CreateHTMLED2kLink(file));
				break;
			case MP_GETSOURCEED2KLINK:
			{
				if(selectedCount > 1)
				{
					CString str;
					for (int i = 0; i < arraySelFiles.GetSize(); i++)	// itsonlyme: selFix
					{
						file = arraySelFiles[i];	// itsonlyme: selFix
						str += theApp.CreateED2kSourceLink(file) + "\n"; 
					}
					theApp.CopyTextToClipboard(str);
					//AfxMessageBox(GetResString(IDS_COPIED2CB) + str);
					break; 
				} 
				CString strLink = theApp.CreateED2kSourceLink(file);
				if (!strLink.IsEmpty())
					theApp.CopyTextToClipboard(strLink);
				break;
			}
			// EastShare Start added by linekin, TBH delete shared file
			case MP_DELFILE:
				{
					for (int i = 0; i < arraySelFiles.GetSize(); i++)	// itsonlyme: selFix
					{
						file = arraySelFiles[i];	// itsonlyme: selFix
						DeleteFileFromHD(file);
					}
					break;
				}
			// EastShare End
			// itsonlyme: hostnameSource
			case MP_GETHOSTNAMESOURCEED2KLINK:
			{
				if(selectedCount > 1)
				{
					CString str;
					for (int i = 0; i < arraySelFiles.GetSize(); i++)	// itsonlyme: selFix
					{
						file = arraySelFiles[i];	// itsonlyme: selFix
						str += theApp.CreateED2kHostnameSourceLink(file) + "\n"; 
					}
					theApp.CopyTextToClipboard(str);
					//AfxMessageBox(GetResString(IDS_COPIED2CB) + str);
					break; 
				} 
				CString strLink = theApp.CreateED2kHostnameSourceLink(file);
				if (!strLink.IsEmpty())
					theApp.CopyTextToClipboard(strLink);
				break;
			}
			// itsonlyme: hostnameSource
			//MORPH START - Added by milobac, FakeCheck, FakeReport, Auto-updating
			case MP_FAKEREPORT:
				{
					if(selectedCount > 1)
					{
						//CString str;
						for (int i = 0; i < arraySelFiles.GetSize(); i++)	// itsonlyme: selFix
						{
							file = arraySelFiles[i];	// itsonlyme: selFix
							//str.Append(theApp.CreateED2kLink(file) + "\n"); 
						}

						//theApp.CopyTextToClipboard(str);
						//AfxMessageBox(GetResString(IDS_COPIED2CB) + str);
						break; 
					}
					//theApp.CopyTextToClipboard(theApp.CreateED2kLink(file));
					ShellExecute(NULL, NULL, "http://edonkeyfakes.ath.cx/report/index.php?link2="+CreateED2kLink(file), NULL, theApp.glob_prefs->GetAppDir(), SW_SHOWDEFAULT);
					break;
				}
			//MORPH END - Added by milobac, FakeCheck, FakeReport, Auto-updating
			case MP_OPEN:
			{
				if (!file->IsPartFile())	//EastShare -  [fix] small bug, eMule allows to open unfinished files (NoamSon) by AndCycle
				OpenFile(file);
				break; 
			}
			//MORPH START - Added by SiRoB, About Popup Open File Folder entry
			case MP_OPENFILEFOLDER:
			{
				OpenFileFolder(file);
				break;
			}
			//MORPH END - Added by SiRoB, About Popup Open File Folder entry
			//For Comments 
			case MP_CMT: 
            { 
				ShowComments(iSel);
                break; 
            } 
            //*END Comments*//
			case MPG_ALTENTER:
			case MP_DETAIL:
				if (file){
					CSharedFileDetailsSheet sheet(file);
					sheet.DoModal();
				}
				break;

			case MP_PRIOVERYLOW:
			case MP_PRIOLOW:
			case MP_PRIONORMAL:
			case MP_PRIOHIGH:
			case MP_PRIOVERYHIGH:
			case MP_PRIOAUTO:
				{
					for (int i = 0; i < arraySelFiles.GetSize(); i++)	// itsonlyme: selFix
					{
						file = arraySelFiles[i];
						switch (wParam) {
							case MP_PRIOVERYLOW:
								{	file->SetAutoUpPriority(false);file->SetUpPriority(PR_VERYLOW);SetItemText(iSel,3,GetResString(IDS_PRIOVERYLOW ));break;	}
							case MP_PRIOLOW:
								{	file->SetAutoUpPriority(false);file->SetUpPriority(PR_LOW);SetItemText(iSel,3,GetResString(IDS_PRIOLOW ));break;	}
							case MP_PRIONORMAL:
								{	file->SetAutoUpPriority(false);file->SetUpPriority(PR_NORMAL);SetItemText(iSel,3,GetResString(IDS_PRIONORMAL ));break;	}
							case MP_PRIOHIGH:
								{	file->SetAutoUpPriority(false);file->SetUpPriority(PR_HIGH);SetItemText(iSel,3,GetResString(IDS_PRIOHIGH ));break;	}
							case MP_PRIOVERYHIGH:
								{	file->SetAutoUpPriority(false);file->SetUpPriority(PR_VERYHIGH);SetItemText(iSel,3,GetResString(IDS_PRIORELEASE ));break;	}//Hunter
							case MP_PRIOAUTO:
								{	file->SetAutoUpPriority(true);file->UpdateAutoUpPriority();UpdateFile(file); break;	}//Hunter
						}
					}
					break;
				}
			//MORPH START - Added by SiRoB, ZZ Upload System
			case MP_POWERSHARE_ON:
			case MP_POWERSHARE_OFF:
			//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
			case MP_POWERSHARE_AUTO:
			{
				for (int i = 0; i < arraySelFiles.GetSize(); i++)	// itsonlyme: selFix
				{
					file = arraySelFiles[i];	// itsonlyme: selFix
					switch (wParam) {
						case MP_POWERSHARE_ON:
							{	file->SetPowerShared(1);UpdateFile(file);break;	}
						case MP_POWERSHARE_OFF:
							{	file->SetPowerShared(0);UpdateFile(file);break;	}
						case MP_POWERSHARE_AUTO:
							{	file->SetPowerShared(2);UpdateFile(file);break;	}
					}
				}
				break;
			}
			//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
			//MORPH END   - Added by SiRoB, ZZ Upload System
			// xMule_MOD: showSharePermissions
			// with itsonlyme's sorting fix
			case MP_PERMNONE:
			case MP_PERMFRIENDS:
			case MP_PERMALL: {
				for (int i = 0; i < arraySelFiles.GetSize(); i++)	// itsonlyme: selFix
				{
					file = arraySelFiles[i];	// itsonlyme: selFix
					switch (wParam)
					{
						case MP_PERMNONE:
							file->SetPermissions(PERM_NOONE);
							UpdateFile(file);
							break;
						case MP_PERMFRIENDS:
							file->SetPermissions(PERM_FRIENDS);
							UpdateFile(file);
							break;
						default : // case MP_PERMALL:
							file->SetPermissions(PERM_ALL);
							UpdateFile(file);
							break;
					}
				}
				Invalidate();
				break;
			}
			// xMule_MOD: showSharePermissions


		}
	}
	return true;
}


void CSharedFilesCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult){
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = theApp.glob_prefs->GetColumnSortItem(CPreferences::tableShared);
	bool m_oldSortAscending = theApp.glob_prefs->GetColumnSortAscending(CPreferences::tableShared);
	bool sortAscending = (sortItem != pNMListView->iSubItem) ? true : !m_oldSortAscending;

	// Item is column clicked
	sortItem = pNMListView->iSubItem;

	// Save new preferences
	theApp.glob_prefs->SetColumnSortItem(CPreferences::tableShared, sortItem);
	theApp.glob_prefs->SetColumnSortAscending(CPreferences::tableShared, sortAscending);

	// Ornis 4-way-sorting
	int adder=0;
	if (pNMListView->iSubItem>5 && pNMListView->iSubItem<9) {
		if (!sortAscending) sortstat[pNMListView->iSubItem-6]=!sortstat[pNMListView->iSubItem-6];
		adder=sortstat[pNMListView->iSubItem-6] ? 0:100;
	}

	// Sort table
	if (adder==0)	
		SetSortArrow(sortItem, sortAscending); 
	else
		SetSortArrow(sortItem, sortAscending ? arrowDoubleUp : arrowDoubleDown);
	SortItems(SortProc, sortItem + adder + (sortAscending ? 0:20));

	*pResult = 0;
}

int CSharedFilesCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort){
	CKnownFile* item1 = (CKnownFile*)lParam1;
	CKnownFile* item2 = (CKnownFile*)lParam2;	
	switch(lParamSort){
		case 0: //filename asc
			return _tcsicmp(item1->GetFileName(),item2->GetFileName());
		case 20: //filename desc
			return _tcsicmp(item2->GetFileName(),item1->GetFileName());

		case 1: //filesize asc
			return item1->GetFileSize()==item2->GetFileSize()?0:(item1->GetFileSize()>item2->GetFileSize()?1:-1);

		case 21: //filesize desc
			return item1->GetFileSize()==item2->GetFileSize()?0:(item2->GetFileSize()>item1->GetFileSize()?1:-1);


		case 2: //filetype asc
			return item1->GetFileType().CompareNoCase(item2->GetFileType());
		case 22: //filetype desc
			return item2->GetFileType().CompareNoCase(item1->GetFileType());

		case 3: //prio asc
			//MORPH START - Changed by SiRoB, Powerstate in prio colums
			if (item1->GetPowerShared() == false && item2->GetPowerShared() == true)
				return -1;			
			else if (item1->GetPowerShared() == true && item2->GetPowerShared() == false)
				return 1;
			else			
				if(item1->GetUpPriority() == PR_VERYLOW && item2->GetUpPriority() != PR_VERYLOW)
					return -1;
				else if (item1->GetUpPriority() != PR_VERYLOW && item2->GetUpPriority() == PR_VERYLOW)
					return 1;
				else
					return item1->GetUpPriority()-item2->GetUpPriority();
			//MORPH END   - Changed by SiRoB, Powerstate in prio colums
		case 23: //prio desc
			//MORPH START - Changed by SiRoB, Powerstate in prio colums
			if (item2->GetPowerShared() == false && item1->GetPowerShared() == true)
				return -1;			
			else if (item2->GetPowerShared() == true && item1->GetPowerShared() == false)
				return 1;
			else		
				if(item2->GetUpPriority() == PR_VERYLOW && item1->GetUpPriority() != PR_VERYLOW )
					return -1;
				else if (item2->GetUpPriority() != PR_VERYLOW && item1->GetUpPriority() == PR_VERYLOW)
					return 1;
				else
					return item2->GetUpPriority()-item1->GetUpPriority();
			//MORPH END  - Changed by SiRoB, Powerstate in prio colums
		case 4: //permission asc
			return item2->GetPermissions()-item1->GetPermissions();
		case 24: //permission desc
			return item1->GetPermissions()-item2->GetPermissions();

		case 5: //fileID asc
			return memcmp(item1->GetFileHash(), item2->GetFileHash(), 16);
		case 25: //fileID desc
			return memcmp(item2->GetFileHash(), item1->GetFileHash(), 16);

		case 6: //requests asc
			return item1->statistic.GetRequests() - item2->statistic.GetRequests();
		case 26: //requests desc
			return item2->statistic.GetRequests() - item1->statistic.GetRequests();
		case 7: //acc requests asc
			return item1->statistic.GetAccepts() - item2->statistic.GetAccepts();
		case 27: //acc requests desc
			return item2->statistic.GetAccepts() - item1->statistic.GetAccepts();
		case 8: //all transferred asc
			return item1->statistic.GetTransferred()==item2->statistic.GetTransferred()?0:(item1->statistic.GetTransferred()>item2->statistic.GetTransferred()?1:-1);
		case 28: //all transferred desc
			return item1->statistic.GetTransferred()==item2->statistic.GetTransferred()?0:(item2->statistic.GetTransferred()>item1->statistic.GetTransferred()?1:-1);

		case 10: //folder asc
			return _tcsicmp(item1->GetPath(),item2->GetPath());
		case 30: //folder desc
			return _tcsicmp(item2->GetPath(),item1->GetPath());

		// #zegzav:completesrc
		case 11: //complete sources asc
			return CompareUnsigned(item1->m_nCompleteSourcesCount, item2->m_nCompleteSourcesCount);
		case 31: //complete sources desc
			return CompareUnsigned(item2->m_nCompleteSourcesCount, item1->m_nCompleteSourcesCount);
		// #zegzav:completesrc END

		//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
		//MORPH START - Added by SiRoB, ZZ Upload System
		case 13:
			if (item1->GetPowerShared() == false && item2->GetPowerShared() == true)
				return -1;
			else if (item1->GetPowerShared() == true && item2->GetPowerShared() == false)
				return 1;
			else
				if (item1->GetPowerSharedMode() == 0 && item2->GetPowerSharedMode() != 0)
					return -1;
				else if (item1->GetPowerSharedMode() == 1 && item2->GetPowerSharedMode() != 1)
					return 1;
				else if (item1->GetPowerSharedMode() == 2 && item2->GetPowerSharedMode() != 2)
					return 1-item2->GetPowerSharedMode();
				else
					if (item1->GetPowerShareAuthorized() == false && item2->GetPowerShareAuthorized() == true)
						return -1;
					else if (item1->GetPowerShareAuthorized() == true && item2->GetPowerShareAuthorized() == false)
						return 1;
					else
						if (item1->GetPowerShareAuto() == false && item2->GetPowerShareAuto() == true)
							return -1;
						else if (item1->GetPowerShareAuto() == true && item2->GetPowerShareAuto() == false)
							return 1;
						else
							return 0;
		case 33:
			if (item2->GetPowerShared() == false && item1->GetPowerShared() == true)
				return -1;
			else if (item2->GetPowerShared() == true && item1->GetPowerShared() == false)
				return 1;
			else
				if (item2->GetPowerSharedMode() == 0 && item1->GetPowerSharedMode() != 0)
					return -1;
				else if (item2->GetPowerSharedMode() == 1 && item1->GetPowerSharedMode() != 1)
					return 1;
				else if (item2->GetPowerSharedMode() == 2 && item1->GetPowerSharedMode() != 2)
					return 1-item1->GetPowerSharedMode();
				else
					if (item2->GetPowerShareAuthorized() == false && item1->GetPowerShareAuthorized() == true)
						return -1;
					else if (item2->GetPowerShareAuthorized() == true && item1->GetPowerShareAuthorized() == false)
						return 1;
					else
						if (item2->GetPowerShareAuto() == false && item1->GetPowerShareAuto() == true)
							return -1;
						else if (item2->GetPowerShareAuto() == true && item1->GetPowerShareAuto() == false)
							return 1;
						else
							return 0;
		//MORPH END - Added by SiRoB, ZZ Upload System
		//MORPH END - Changed by SiRoB, Avoid misusing of powersharing
		//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars
		case 14: //spread asc
		case 15:
			return 10000*(item1->statistic.GetSpreadSortValue()-item2->statistic.GetSpreadSortValue());
		case 34: //spread desc
		case 35:
			return 10000*(item2->statistic.GetSpreadSortValue()-item1->statistic.GetSpreadSortValue());

		case 16: // VQB:  Simple UL asc
		case 36: //VQB:  Simple UL desc
			{
				float x1 = ((float)item1->statistic.GetAllTimeTransferred())/((float)item1->GetFileSize());
				float x2 = ((float)item2->statistic.GetAllTimeTransferred())/((float)item2->GetFileSize());
				if (lParamSort == 13) return 10000*(x1-x2); else return 10000*(x2-x1);
			}
		case 17: // SF:  Full Upload Count asc
			return 10000*(item1->statistic.GetFullSpreadCount()-item2->statistic.GetFullSpreadCount());
		case 37: // SF:  Full Upload Count desc
			return 10000*(item2->statistic.GetFullSpreadCount()-item1->statistic.GetFullSpreadCount());
		//MORPH END   - Added by IceCream, SLUGFILLER: Spreadbars
		case 106: //all requests asc
			return item1->statistic.GetAllTimeRequests() - item2->statistic.GetAllTimeRequests();
		case 126: //all requests desc
			return item2->statistic.GetAllTimeRequests() - item1->statistic.GetAllTimeRequests();
		case 107: //all acc requests asc
			return item1->statistic.GetAllTimeAccepts() - item2->statistic.GetAllTimeAccepts();
		case 127: //all acc requests desc
			return item2->statistic.GetAllTimeAccepts() - item1->statistic.GetAllTimeAccepts();
		case 108: //all transferred asc
			return item1->statistic.GetAllTimeTransferred()==item2->statistic.GetAllTimeTransferred()?0:(item1->statistic.GetAllTimeTransferred()>item2->statistic.GetAllTimeTransferred()?1:-1);
		case 128: //all transferred desc
			return item1->statistic.GetAllTimeTransferred()==item2->statistic.GetAllTimeTransferred()?0:(item2->statistic.GetAllTimeTransferred()>item1->statistic.GetAllTimeTransferred()?1:-1);

		default: 
			return 0;
	}
}

void CSharedFilesCtrl::UpdateFile(CKnownFile* toupdate){
	if( !theApp.emuledlg->IsRunning())
		return;
	LVFINDINFO find;
	find.flags = LVFI_PARAM;
	find.lParam = (LPARAM)toupdate;
	sint16 result = FindItem(&find);
	if (result != -1) {
		Update(result) ;   // Added by Tarod to real time refresh - DonGato - 11/11/2002
		theApp.emuledlg->sharedfileswnd->Check4StatUpdate(toupdate);
	}
}

void CSharedFilesCtrl::ShowFilesCount() {
	CString counter;
	// SLUGFILLER: SafeHash - hashing counter
	if (theApp.sharedfiles->GetHashingCount())
		counter.Format(_T(" (%i/%i)"), theApp.sharedfiles->GetCount(), theApp.sharedfiles->GetCount()+theApp.sharedfiles->GetHashingCount());
	else
		counter.Format(_T(" (%i)"), theApp.sharedfiles->GetCount());
	// SLUGFILLER: SafeHash
	theApp.emuledlg->sharedfileswnd->GetDlgItem(IDC_TRAFFIC_TEXT)->SetWindowText(GetResString(IDS_SF_FILES)+counter  );
}

void CSharedFilesCtrl::OpenFile(CKnownFile* file){
	char* buffer = new char[MAX_PATH];
	_snprintf(buffer,MAX_PATH,"%s\\%s",file->GetPath(),file->GetFileName());
	AddLogLine( false, "%s\\%s",file->GetPath(),file->GetFileName());
	ShellOpenFile(buffer, NULL);
	delete[] buffer;
}

void CSharedFilesCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1) {
		CKnownFile* file = (CKnownFile*)GetItemData(iSel);
		if (file){
			//MORPH Changed by SiRoB - Double click unfinished files in SharedFile window display FileDetail
			//if (GetKeyState(VK_MENU) & 0x8000){
			if (GetKeyState(VK_MENU) & 0x8000 || file->IsPartFile()){
				CSharedFileDetailsSheet sheet(file);
				sheet.DoModal();
			}
			else
				OpenFile(file);
		}
	}
	*pResult = 0;
}

void CSharedFilesCtrl::CreateMenues() {
	//MORPH START - Added by SiRoB, ZZ Upload System
	if (m_PowershareMenu) VERIFY( m_PowershareMenu.DestroyMenu() );
	//MORPH END - Added by SiRoB, ZZ Upload System
	
	if (m_PrioMenu) VERIFY( m_PrioMenu.DestroyMenu() );
	if (m_PermMenu) VERIFY( m_PermMenu.DestroyMenu() );
	if (m_SharedFilesMenu) VERIFY( m_SharedFilesMenu.DestroyMenu() );

	//MORPH START - Added by SiRoB, ZZ Upload System
	// add powershare switcher
	m_PowershareMenu.CreateMenu();
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_ON,GetResString(IDS_POWERSHARE_ON));
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_OFF,GetResString(IDS_POWERSHARE_OFF));
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_AUTO,GetResString(IDS_POWERSHARE_AUTO));
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH END - Added by SiRoB, ZZ Upload System

	// add priority switcher
	m_PrioMenu.CreateMenu();
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOVERYLOW,GetResString(IDS_PRIOVERYLOW));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOLOW,GetResString(IDS_PRIOLOW));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIONORMAL,GetResString(IDS_PRIONORMAL));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOHIGH, GetResString(IDS_PRIOHIGH));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOVERYHIGH, GetResString(IDS_PRIORELEASE));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOAUTO, GetResString(IDS_PRIOAUTO));//UAP

	// add permission switcher
	m_PermMenu.CreateMenu();
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMNONE,	GetResString(IDS_HIDDEN));	// xMule_MOD: showSharePermissions
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMFRIENDS,	GetResString(IDS_FSTATUS_FRIENDSONLY));
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMALL,		GetResString(IDS_FSTATUS_PUBLIC));

	m_SharedFilesMenu.CreatePopupMenu();
	m_SharedFilesMenu.AddMenuTitle(GetResString(IDS_SHAREDFILES));
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PermMenu.m_hMenu, GetResString(IDS_PERMISSION));	// xMule_MOD: showSharePermissions - done
	// todo enable when it works
	//m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PermMenu.m_hMenu, GetResString(IDS_PERMISSION));
	//MORPH START - Added by SiRoB, ZZ Upload System
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PowershareMenu.m_hMenu, GetResString(IDS_POWERSHARE));
	//MORPH END - Added by SiRoB, ZZ Upload System
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PrioMenu.m_hMenu, GetResString(IDS_PRIORITY) );
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR);
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_OPEN, GetResString(IDS_OPENFILE));
	//MORPH START - Added by SiRoB, About Open File Folder entry
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_OPENFILEFOLDER, GetResString(IDS_OPENFILEFOLDER)); //MORPH - Added by SiRoB, About Popup Open File Folder entry
	//MORPH END - Added by SiRoB, About Open File Folder entry
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_DELFILE, GetResString(IDS_DELETEFILE)); // EastShare added by linekin, TBH delete shared file
	//***Comments 11/27/03**// 
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_CMT, GetResString(IDS_CMT_ADD)); 
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_DETAIL, GetResString(IDS_SHOWDETAILS));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_FAKEREPORT,GetResString(IDS_FAKEREPORT)); //MORPH - Added by milobac, FakeCheck, FakeReport, Auto-updating
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
	//****end  comments***//

	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_GETED2KLINK, GetResString(IDS_DL_LINK1));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_GETHTMLED2KLINK, GetResString(IDS_DL_LINK2));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_GETSOURCEED2KLINK, GetResString(IDS_CREATESOURCELINK));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_GETHOSTNAMESOURCEED2KLINK, GetResString(IDS_CREATEHOSTNAMESRCLINK));	// itsonlyme: hostnameSource
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
	
	//This menu option is is for testing..
	m_SharedFilesMenu.AppendMenu(MF_STRING,Irc_SetSendLink,GetResString(IDS_IRC_ADDLINKTOIRC));
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
}

void CSharedFilesCtrl::ShowComments(int index) {
	// MORPH START - Added by IceCream, Hotfix by k-man to avoid middle mouse button crash
	if (index == -1) // k-man - middle mouse button crash (index = -1)
		return;
	// MORPH END   - Added by IceCream, Hotfix by k-man to avoid middle mouse button crash
	CKnownFile* file = (CKnownFile*)GetItemData(index);
    CCommentDialog dialog(file); 
	//MORPH START - Added by IceCream, SLUGFILLER: batchComment
	if (dialog.DoModal() == IDOK) {
		POSITION pos = this->GetFirstSelectedItemPosition();
		while( pos != NULL )
		{
			int iSel=this->GetNextSelectedItem(pos);
			CKnownFile* otherfile = (CKnownFile*)this->GetItemData(iSel);
			if (otherfile == file)
				continue;
			otherfile->SetFileComment(file->GetFileComment());
			otherfile->SetFileRate(file->GetFileRate());
		}
	}
	//MORPH END   - Added by IceCream, SLUGFILLER: batchComment
}

void CSharedFilesCtrl::OnGetDispInfo(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	if (theApp.emuledlg->IsRunning()){
		// Although we have an owner drawn listview control we store the text for the primary item in the listview, to be
		// capable of quick searching those items via the keyboard. Because our listview items may change their contents,
		// we do this via a text callback function. The listview control will send us the LVN_DISPINFO notification if
		// it needs to know the contents of the primary item.
		//
		// But, the listview control sends this notification all the time, even if we do not search for an item. At least
		// this notification is only sent for the visible items and not for all items in the list. Though, because this
		// function is invoked *very* often, no *NOT* put any time consuming code here in.

		if (pDispInfo->item.mask & LVIF_TEXT){
			const CKnownFile* pFile = reinterpret_cast<CKnownFile*>(pDispInfo->item.lParam);
			if (pFile != NULL){
				switch (pDispInfo->item.iSubItem){
					case 0:
						if (pDispInfo->item.cchTextMax > 0){
							_tcsncpy(pDispInfo->item.pszText, pFile->GetFileName(), pDispInfo->item.cchTextMax);
							pDispInfo->item.pszText[pDispInfo->item.cchTextMax-1] = _T('\0');
						}
						break;
					default:
						// shouldn't happen
						pDispInfo->item.pszText[0] = _T('\0');
						break;
				}
			}
		}
	}
	*pResult = 0;
}

//MORPH START - Added by SiRoB, About Popup Open File Folder entry
void CSharedFilesCtrl::OpenFileFolder(CKnownFile* file){
	char* buffer = new char[MAX_PATH];
	_snprintf(buffer,MAX_PATH,"%s",file->GetPath());
	ShellOpenFile(buffer, NULL);
	delete[] buffer;
}
//MORPH END - Added by SiRoB, About Popup Open File Folder entry
// EastShare Start added by linekin, TBH delete shared file
void CSharedFilesCtrl::DeleteFileFromHD(CKnownFile *file)
{
	if (((CPartFile*)file)->IsPartFile())
		DeleteFileFromHDPart((CPartFile*)file);
	else DeleteFileFromHDByKnown(file);
}
void CSharedFilesCtrl::DeleteFileFromHDByKnown(CKnownFile* file)
{
	char buffer[200];
	char* buffer2 = new char[MAX_PATH];
	_snprintf(buffer2,MAX_PATH,"%s\\%s",file->GetPath(),file->GetFileName());
	sprintf(buffer,"Are you sure you want to delete \"%s\"?",file->GetFileName());
	if (MessageBox(buffer,"Really?",MB_ICONQUESTION|MB_YESNO) == IDYES)
	{
		if (!DeleteFile(buffer2))
		{
			sprintf(buffer,"Couldn't delete \"%s\".",file->GetFileName());
			MessageBox(buffer,"Error",MB_ICONERROR|MB_OK);
		}
		else
		{
			theApp.emuledlg->AddLogLine(true,"\"%s\" successfully deleted.",file->GetFileName());
			theApp.sharedfiles->Reload();
		}
	}
	delete[] buffer2;
}
void CSharedFilesCtrl::DeleteFileFromHDPart(CPartFile* file)
{
	ASSERT ( !file->m_bPreviewing );
	char buffer[200];
	sprintf(buffer,"Are you sure you want to delete \"%s\"?",file->GetFileName());
	if (MessageBox(buffer,"Really?",MB_ICONQUESTION|MB_YESNO) == IDYES)
	{
		file->DeleteFile();
		theApp.emuledlg->AddLogLine(true,"\"%s\" successfully deleted.",file->GetFileName());
		theApp.sharedfiles->Reload();
	}
}
// EastShare End

