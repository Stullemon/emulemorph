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
#include "InputBox.h"
#include "WebServices.h"
#include "TransferWnd.h"

// Mighty Knife: CRC32-Tag, Mass Rename
#include "AddCRC32TagDialog.h"
#include "MassRename.h"
// [end] Mighty Knife

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
	CSharedFileDetailsSheet(const CTypedPtrList<CPtrList, CKnownFile*>& aFiles);
	virtual ~CSharedFileDetailsSheet();

protected:
	CSimpleArray<const CKnownFile*> m_aKnownFiles;
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

CSharedFileDetailsSheet::CSharedFileDetailsSheet(const CTypedPtrList<CPtrList, CKnownFile*>& aFiles)
{
	POSITION pos = aFiles.GetHeadPosition();
	while (pos)
		m_aKnownFiles.Add(aFiles.GetNext(pos));
	m_psh.dwFlags &= ~PSH_HASHELP;
	m_psh.dwFlags |= PSH_NOAPPLYNOW;
	
	m_wndMediaInfo.m_psp.dwFlags &= ~PSP_HASHELP;
	m_wndMetaData.m_psp.dwFlags &= ~PSP_HASHELP;

	m_wndMediaInfo.SetMyfile(&m_aKnownFiles);
	if (m_aKnownFiles.GetSize() == 1 && thePrefs.IsExtControlsEnabled())
		m_wndMetaData.SetFile(m_aKnownFiles[0]);

	AddPage(&m_wndMediaInfo);
	if (m_aKnownFiles.GetSize() == 1 && thePrefs.IsExtControlsEnabled())
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
	if (m_aKnownFiles.GetSize() == 1)
		SetWindowText(GetResString(IDS_DETAILS) + _T(": ") + m_aKnownFiles[0]->GetFileName());
	else
		SetWindowText(GetResString(IDS_DETAILS));
	if (sm_iLastActivePage < GetPageCount())
		SetActivePage(sm_iLastActivePage);
	return bResult;
}


//////////////////////////////////////////////////////////////////////////////
// CSharedFilesCtrl

IMPLEMENT_DYNAMIC(CSharedFilesCtrl, CMuleListCtrl)
CSharedFilesCtrl::CSharedFilesCtrl()
{
	memset(&sortstat, 0, sizeof(sortstat));
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
	InsertColumn(4,GetResString(IDS_FILEID),LVCFMT_LEFT,220,4);
	InsertColumn(5,GetResString(IDS_SF_REQUESTS),LVCFMT_LEFT,100,5);
	InsertColumn(6,GetResString(IDS_SF_ACCEPTS),LVCFMT_LEFT,100,6);
	InsertColumn(7,GetResString(IDS_SF_TRANSFERRED),LVCFMT_LEFT,120,7);
	InsertColumn(8,GetResString(IDS_UPSTATUS),LVCFMT_LEFT,100,8);
	InsertColumn(9,GetResString(IDS_FOLDER),LVCFMT_LEFT,200,9);
	InsertColumn(10,GetResString(IDS_COMPLSOURCES),LVCFMT_LEFT,100,10);
	InsertColumn(11,GetResString(IDS_SHAREDTITLE),LVCFMT_LEFT,200,11);
	//MORPH START - Added by SiRoB, Keep Permission flag
	InsertColumn(12,GetResString(IDS_PERMISSION),LVCFMT_LEFT,100,12);
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
	//MORPH START - Added by SiRoB, HIDEOS
    InsertColumn(18,GetResString(IDS_HIDEOS),LVCFMT_LEFT,100,18);
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	InsertColumn(19,GetResString(IDS_SHAREONLYTHENEED),LVCFMT_LEFT,100,19);
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
	m_ImageList.Create(16,16,theApp.m_iDfltImageListColorFlags|ILC_MASK,0,2);
	m_ImageList.SetBkColor(CLR_NONE);
	m_ImageList.Add(theApp.LoadIcon("RATING_NO"));  // 0
	m_ImageList.Add(theApp.LoadIcon("RATING_FAKE"));  // 1
	m_ImageList.Add(theApp.LoadIcon("RATING_POOR"));  // 2
	m_ImageList.Add(theApp.LoadIcon("RATING_GOOD"));  // 3
	m_ImageList.Add(theApp.LoadIcon("RATING_FAIR"));  // 4
	m_ImageList.Add(theApp.LoadIcon("RATING_EXCELLENT"));  // 5
	//MORPH END   - Added & Moddified by IceCream, SLUGFILLER: showComments

	// Mighty Knife: CRC32-Tag
	InsertColumn(20,"calculated CRC32",LVCFMT_LEFT,120,20);
	InsertColumn(21,"CRC32-check ok",LVCFMT_LEFT,100,21);
	// [end] Mighty Knife

	CreateMenues();

	LoadSettings(CPreferences::tableShared);

	// Barry - Use preferred sort order from preferences
	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableShared);
	bool sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableShared);
	SetSortArrow(sortItem, sortAscending);
	// SLUGFILLER: multiSort - load multiple params
	for (int i = thePrefs.GetColumnSortCount(CPreferences::tableShared); i > 0; ) {
		i--;
		sortItem = thePrefs.GetColumnSortItem(CPreferences::tableShared, i);
		sortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableShared, i);
		// Mighty Knife: CRC32-Tag - Indexes shifted by 10
		SortItems(SortProc, sortItem + (sortAscending ? 0:30));
		// [end] Mighty Knife
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

	strRes = GetResString(IDS_FILEID);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(4, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_SF_REQUESTS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(5, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_SF_ACCEPTS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(6, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_SF_TRANSFERRED);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(7, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_UPSTATUS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(8, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_FOLDER);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(9, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_COMPLSOURCES);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(10, &hdi);
	strRes.ReleaseBuffer();

	strRes = GetResString(IDS_SHAREDTITLE);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(11, &hdi);
	strRes.ReleaseBuffer();

	//MORPH START - Added  by SiRoB, Keep Permission flag
	strRes = GetResString(IDS_PERMISSION);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(12, &hdi);
	strRes.ReleaseBuffer();
	//MORPH END   - Added  by SiRoB, Keep Permission flag

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
	//MORPH START - Added by SiRoB, HIDEOS
    strRes = GetResString(IDS_HIDEOS);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(18, &hdi);
	strRes.ReleaseBuffer();
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	strRes = GetResString(IDS_SHAREONLYTHENEED);
	hdi.pszText = strRes.GetBuffer();
	pHeaderCtrl->SetItem(19, &hdi);
	strRes.ReleaseBuffer();
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
	
	CreateMenues();
}

void CSharedFilesCtrl::ShowFileList(const CSharedFileList* pSharedFiles)
{
	DeleteAllItems();

	CCKey bufKey;
	CKnownFile* cur_file;
	for (POSITION pos = pSharedFiles->m_Files_map.GetStartPosition(); pos != 0; ){
		pSharedFiles->m_Files_map.GetNextAssoc(pos, bufKey, cur_file);
		ShowFile(cur_file);
	}
	ShowFilesCount();
}

#define DLC_DT_TEXT (DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX|DT_END_ELLIPSIS)

void CSharedFilesCtrl::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
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

	const CKnownFile* file = (CKnownFile*)lpDrawItemStruct->itemData;
	CMemDC dc(CDC::FromHandle(lpDrawItemStruct->hDC), &lpDrawItemStruct->rcItem);
	CFont* pOldFont = dc.SelectObject(GetFont());
	RECT cur_rec = lpDrawItemStruct->rcItem;
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
				uint8 Perm = file->GetPermissions()>=0?file->GetPermissions():thePrefs.GetPermissions();
				if (iColumn==0){
					if (file->IsPartFile())
						dc->SetTextColor((COLORREF)RGB(0,0,192));
				}else if (Perm == PERM_NOONE)
					dc->SetTextColor((COLORREF)RGB(0,175,0));
				else if (Perm == PERM_FRIENDS)
					dc->SetTextColor((COLORREF)RGB(208,128,0));
				// Mighty Knife: Community visible filelist
				else if (Perm == PERM_COMMUNITY)
					dc->SetTextColor((COLORREF)RGB(240,0,128));
				// [end] Mighty Knife
				else if (Perm == PERM_ALL)
					dc->SetTextColor((COLORREF)RGB(240,0,0));
				// xMule_MOD: showSharePermissions
				switch(iColumn){
					case 0:{
						int iImage = theApp.GetFileTypeSystemImageIdx(file->GetFileName());
						if (theApp.GetSystemImageList() != NULL)
						::ImageList_Draw(theApp.GetSystemImageList(), iImage, dc->GetSafeHdc(), cur_rec.left, cur_rec.top, ILD_NORMAL|ILD_TRANSPARENT);
						cur_rec.left += iIconDrawWidth;
						buffer = file->GetFileName();
						//MORPH START - Added by IceCream, SLUGFILLER: showComments
						CKnownFile* pfile =(CKnownFile*)lpDrawItemStruct->itemData;
						if ( thePrefs.ShowRatingIndicator() && ( !pfile->GetFileComment().IsEmpty() || pfile->GetFileRate() )){ //Modified by IceCream, eMule plus rating icon
							POINT point= {cur_rec.left-4,cur_rec.top+2};
							int the_rating=0;
							if (pfile->GetFileRate())
								the_rating = pfile->GetFileRate();
							m_ImageList.Draw(dc,
								pfile->GetFileRate(), //Modified by IceCream, eMule plus rating icon
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
						buffer = md4str(file->GetFileHash());
						break;
					case 5:
                    buffer.Format(_T("%u (%u)"), file->statistic.GetRequests(), file->statistic.GetAllTimeRequests());
						break;
					case 6:
					buffer.Format(_T("%u (%u)"), file->statistic.GetAccepts(), file->statistic.GetAllTimeAccepts());
						break;
					case 7:
					buffer.Format(_T("%s (%s)"), CastItoXBytes(file->statistic.GetTransferred()), CastItoXBytes(file->statistic.GetAllTimeTransferred()));
						break;
					case 8:
						if( file->GetPartCount()){
							cur_rec.bottom--;
							cur_rec.top++;
							if(!file->IsPartFile())
								((CKnownFile*)lpDrawItemStruct->itemData)->DrawShareStatusBar(dc,&cur_rec,false,thePrefs.UseFlatBar());
							else
								((CPartFile*)lpDrawItemStruct->itemData)->DrawShareStatusBar(dc,&cur_rec,false,thePrefs.UseFlatBar());
							cur_rec.bottom++;
							cur_rec.top--;
						}
						break;
					case 9:
						buffer = file->GetPath();
						PathRemoveBackslash(buffer.GetBuffer());
						buffer.ReleaseBuffer();
						break;
					case 10:{
						if (file->m_nCompleteSourcesCountLo == 0){
							buffer.Format("< %u", file->m_nCompleteSourcesCountHi);
						}
						else if (file->m_nCompleteSourcesCountLo == file->m_nCompleteSourcesCountHi)
							buffer.Format("%u", file->m_nCompleteSourcesCountLo);
						else
							buffer.Format("%u - %u", file->m_nCompleteSourcesCountLo, file->m_nCompleteSourcesCountHi);
						//MORPH START - Added by SiRoB, Avoid misusing of powersharing
						CString buffer2;
						buffer2.Format(" (%u)",file->m_nVirtualCompleteSourcesCount);
						buffer.Append(buffer2);
						//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
						break;
					}
					case 11:{
						CString ed2k;
						if(file->GetPublishedED2K())
							buffer = GetResString(IDS_YES);
						else
							buffer = GetResString(IDS_NO);
						buffer += "|";
					if( (uint32)time(NULL)-file->GetLastPublishTimeKadSrc() < KADEMLIAREPUBLISHTIMES )
							buffer += GetResString(IDS_YES);
						else
							buffer += GetResString(IDS_NO);
						break;
					}
					//MORPH START - Added by SiRoB, Keep Permission flag
					case 12:{
						// xMule_MOD: showSharePermissions
						uint8 Perm = file->GetPermissions()>=0?file->GetPermissions():thePrefs.GetPermissions();
						switch (Perm)
						{
							case PERM_ALL:
								buffer = GetResString(IDS_FSTATUS_PUBLIC);
								break;
							case PERM_NOONE: 
								buffer = GetResString(IDS_HIDDEN); 
								break;
							case PERM_FRIENDS: 
								buffer = GetResString(IDS_FSTATUS_FRIENDSONLY); 
								break;
							// Mighty Knife: Community visible filelist
							case PERM_COMMUNITY: 
								buffer = GetResString(IDS_COMMUNITY); 
								break;
							// [end] Mighty Knife
							default: 
								buffer = "?";
								break;
						}
						// xMule_MOD: showSharePermissions
						break;
					}
					//MORPH END   - Added by SiRoB, Keep Permission flag
					//MORPH START - Added by SiRoB, ZZ Upload System
					case 13:{
						//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
						int powersharemode;
						bool powershared = file->GetPowerShared();
						buffer = "[" + GetResString((powershared)?IDS_POWERSHARE_ON_LABEL:IDS_POWERSHARE_OFF_LABEL) + "] ";
						if (file->GetPowerSharedMode()>=0)
							powersharemode = file->GetPowerSharedMode();
						else {
							powersharemode = thePrefs.GetPowerShareMode();
							buffer.Append(" " + ((CString)GetResString(IDS_DEFAULT)).Left(1) + ". ");
						}
						if(powersharemode == 2)
							buffer.Append(GetResString(IDS_POWERSHARE_AUTO_LABEL));
						else if (powersharemode == 1)
							buffer.Append(GetResString(IDS_POWERSHARE_ACTIVATED_LABEL));
						//MORPH START - Added by SiRoB, POWERSHARE Limit
						else if (powersharemode == 3) {
							buffer.Append(GetResString(IDS_POWERSHARE_LIMITED));
							if (file->GetPowerShareLimit()<0)
								buffer.AppendFormat(" %s. %i", ((CString)GetResString(IDS_DEFAULT)).Left(1), thePrefs.GetPowerShareLimit());
							else
								buffer.AppendFormat(" %i", file->GetPowerShareLimit());
						}
						//MORPH END   - Added by SiRoB, POWERSHARE Limit
						else
							buffer.Append(GetResString(IDS_POWERSHARE_DISABLED_LABEL));
						buffer.Append(" (");
						if (file->GetPowerShareAuto())
							buffer.Append(GetResString(IDS_POWERSHARE_ADVISED_LABEL));
						//MORPH START - Added by SiRoB, POWERSHARE Limit
						else if (file->GetPowerShareLimited() && (powersharemode == 3))
							buffer.Append(GetResString(IDS_POWERSHARE_LIMITED));
						//MORPH END   - Added by SiRoB, POWERSHARE Limit
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
						cur_rec.bottom--;
						cur_rec.top++;
						((CKnownFile*)lpDrawItemStruct->itemData)->statistic.DrawSpreadBar(dc,&cur_rec,thePrefs.UseFlatBar());
						cur_rec.bottom++;
						cur_rec.top--;
						break;
					case 15:
						buffer.Format("%.2f",((CKnownFile*)lpDrawItemStruct->itemData)->statistic.GetSpreadSortValue());
						break;
					case 16:
						if (file->GetFileSize())
							buffer.Format("%.2f",((float)file->statistic.GetAllTimeTransferred())/((float)file->GetFileSize()));
						else
							buffer.Format("%.2f",0.0f);
						break;
					case 17:
						buffer.Format("%.2f",((CKnownFile*)lpDrawItemStruct->itemData)->statistic.GetFullSpreadCount());
						break;
					// SLUGFILLER: Spreadbars
					//MORPH START - Added by SiRoB, HIDEOS
					case 18:
						if(file->GetHideOS()>=0)
							if (file->GetHideOS()){
								buffer.Format("%i", file->GetHideOS());
								if (file->GetSelectiveChunk()>=0)
									if (file->GetSelectiveChunk())
										buffer.AppendFormat(" + %s" ,GetResString(IDS_SELECTIVESHARE));
							}else
								buffer = GetResString(IDS_DISABLED);
						else
							buffer = GetResString(IDS_DEFAULT);
						break;
					//MORPH END   - Added by SiRoB, HIDEOS
					//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
					case 19:
						if(file->GetShareOnlyTheNeed()>=0)
							if (file->GetShareOnlyTheNeed())
								buffer.Format("%i" ,file->GetShareOnlyTheNeed());
							else
								buffer = GetResString(IDS_DISABLED);
						else
							buffer = GetResString(IDS_DEFAULT);
						break;
					//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

					// Mighty Knife: CRC32-Tag
					case 20:
						if(file->IsCRC32Calculated()) {
							buffer.Format ("%s",file->GetLastCalculatedCRC32());
						} else buffer = "";
						break;
					case 21:
						if(file->IsCRC32Calculated()) {
							CString FName = file->GetFileName();
							FName.MakeUpper (); // Uppercase the filename ! 
												// Our CRC is upper case...
							if (FName.Find (file->GetLastCalculatedCRC32()) != -1) {
								buffer.Format ("%s",GetResString(IDS_YES));
							} else {
								buffer.Format ("%s",GetResString(IDS_NO));
							}
						} else buffer = "";
						break;
					// [end] Mighty Knife
				}
				if( iColumn != 8 && iColumn!=14)
					dc->DrawText(buffer, buffer.GetLength(),&cur_rec,uDTFlags);
				if( iColumn == 0 )
					//MORPH - Changed by SiRoB, for rating icon
					//cur_rec.left -= iIconDrawWidth;
					cur_rec.left -= iIconDrawWidth + 9;
			//MORPH START - Added by SiRoB, Don't draw hidden coloms
			}
			//MORPH END   - Added by SiRoB, Don't draw hidden coloms
			cur_rec.left += GetColumnWidth(iColumn);
		}
	}
	ShowFilesCount();
	if ((lpDrawItemStruct->itemAction | ODA_SELECT) && (lpDrawItemStruct->itemState & ODS_SELECTED))
	{
		RECT outline_rec = lpDrawItemStruct->rcItem;

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

void CSharedFilesCtrl::ShowFile(const CKnownFile* file)
{
	uint32 itemnr = GetItemCount();
	itemnr = InsertItem(LVIF_TEXT|LVIF_PARAM,itemnr,LPSTR_TEXTCALLBACK,0,0,0,(LPARAM)file);
	UpdateFile(file);
}

void CSharedFilesCtrl::RemoveFile(const CKnownFile *toRemove)
{
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
	ON_WM_KEYDOWN()
	// Mighty Knife: CRC32-Tag - Save rename
	ON_MESSAGE(WM_CRC32_RENAMEFILE,	OnCRC32RenameFile)
	ON_MESSAGE(WM_CRC32_UPDATEFILE, OnCRC32UpdateFile)
	// [end] Mighty Knife
END_MESSAGE_MAP()


// CSharedFilesCtrl message handlers

void CSharedFilesCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
		// get merged settings
	bool bFirstItem = true;
	int iSelectedItems = GetSelectedCount();
	int iCompleteFileSelected = -1;
	int iPowerShareLimit = -1; //MORPH - Added by SiRoB, POWERSHARE Limit
	int iHideOS = -1; //MORPH - Added by SiRoB, HIDEOS

	UINT uPrioMenuItem = 0;
	UINT uPermMenuItem = 0; //MORPH - Added by SiRoB, showSharePermissions
	UINT uPowershareMenuItem = 0; //MORPH - Added by SiRoB, Powershare
	UINT uPowerShareLimitMenuItem = 0; //MORPH - Added by SiRoB, POWERSHARE Limit
	
	UINT uHideOSMenuItem = 0; //MORPH - Added by SiRoB, HIDEOS
	UINT uSelectiveChunkMenuItem = 0; //MORPH - Added by SiRoB, HIDEOS
	UINT uShareOnlyTheNeedMenuItem = 0; //MORPH - Added by SiRoB, SHARE_ONLY_THE_NEED
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		const CKnownFile* pFile = (CKnownFile*)GetItemData(GetNextSelectedItem(pos));

		int iCurCompleteFile = pFile->IsPartFile() ? 0 : 1;

		if (bFirstItem)
			iCompleteFileSelected = iCurCompleteFile;
		else if (iCompleteFileSelected != iCurCompleteFile)
			iCompleteFileSelected = -1;

		UINT uCurPrioMenuItem = 0;
		if (pFile->IsAutoUpPriority())
			uCurPrioMenuItem = MP_PRIOAUTO;
		else if (pFile->GetUpPriority() == PR_VERYLOW)
			uCurPrioMenuItem = MP_PRIOVERYLOW;
		else if (pFile->GetUpPriority() == PR_LOW)
			uCurPrioMenuItem = MP_PRIOLOW;
		else if (pFile->GetUpPriority() == PR_NORMAL)
			uCurPrioMenuItem = MP_PRIONORMAL;
		else if (pFile->GetUpPriority() == PR_HIGH)
			uCurPrioMenuItem = MP_PRIOHIGH;
		else if (pFile->GetUpPriority() == PR_VERYHIGH)
			uCurPrioMenuItem = MP_PRIOVERYHIGH;
		else
			ASSERT(0);

		if (bFirstItem)
			uPrioMenuItem = uCurPrioMenuItem;
		else if (uPrioMenuItem != uCurPrioMenuItem)
			uPrioMenuItem = 0;

		//MORPH START - Added by SiRoB, showSharePermissions
		UINT uCurPermMenuItem = 0;
		if (pFile->GetPermissions()==-1)
			uCurPermMenuItem = MP_PERMDEFAULT;
		else if (pFile->GetPermissions()==PERM_ALL)
			uCurPermMenuItem = MP_PERMALL;
		else if (pFile->GetPermissions() == PERM_FRIENDS)
			uCurPermMenuItem = MP_PERMFRIENDS;
		// Mighty Knife: Community visible filelist
		else if (pFile->GetPermissions() == PERM_COMMUNITY)
			uCurPermMenuItem = MP_PERMCOMMUNITY;
		// [end] Mighty Knife
		else if (pFile->GetPermissions() == PERM_NOONE)
			uCurPermMenuItem = MP_PERMNONE;
		else
			ASSERT(0);

		if (bFirstItem)
			uPermMenuItem = uCurPermMenuItem;
		else if (uPermMenuItem != uCurPermMenuItem)
			uPermMenuItem = 0;
		//MORPH END   - Added by SiRoB, showSharePermissions
		//MORPH START - Added by SiRoB, Avoid misusing of powershare
		UINT uCurPowershareMenuItem = 0;
		if (pFile->GetPowerSharedMode()==-1)
			uCurPowershareMenuItem = MP_POWERSHARE_DEFAULT;
		else
			uCurPowershareMenuItem = MP_POWERSHARE_DEFAULT+1 + pFile->GetPowerSharedMode();
		
		if (bFirstItem)
			uPowershareMenuItem = uCurPowershareMenuItem;
		else if (uPowershareMenuItem != uCurPowershareMenuItem)
			uPowershareMenuItem = 0;
		//MORPH END   - Added by SiRoB, Avoid misusing of powershare
		
		//MORPH START - Added by SiRoB, POWERSHARE Limit
		UINT uCurPowerShareLimitMenuItem = 0;
		int iCurPowerShareLimit = pFile->GetPowerShareLimit();
		if (iCurPowerShareLimit==-1)
			uCurPowerShareLimitMenuItem = MP_POWERSHARE_LIMIT;
		else
			uCurPowerShareLimitMenuItem = MP_POWERSHARE_LIMIT_SET;
		
		if (bFirstItem)
		{
			uPowerShareLimitMenuItem = uCurPowerShareLimitMenuItem;
			iPowerShareLimit = iCurPowerShareLimit;
		}
		else if (uPowerShareLimitMenuItem != uCurPowerShareLimitMenuItem || iPowerShareLimit != iCurPowerShareLimit)
		{
			uPowerShareLimitMenuItem = 0;
			iPowerShareLimit = -1;
		}
		//MORPH END   - Added by SiRoB, POWERSHARE Limit
		
		//MORPH START - Added by SiRoB, HIDEOS
		UINT uCurHideOSMenuItem = 0;
		int iCurHideOS = pFile->GetHideOS();
		if (iCurHideOS == -1)
			uCurHideOSMenuItem = MP_HIDEOS_DEFAULT;
		else
			uCurHideOSMenuItem = MP_HIDEOS_SET;
		if (bFirstItem)
		{
			uHideOSMenuItem = uCurHideOSMenuItem;
			iHideOS = iCurHideOS;
		}
		else if (uHideOSMenuItem != uCurHideOSMenuItem || iHideOS != iCurHideOS)
		{
			uHideOSMenuItem = 0;
			iHideOS = -1;
		}
		
		UINT uCurSelectiveChunkMenuItem = 0;
		if (pFile->GetSelectiveChunk() == -1)
			uCurSelectiveChunkMenuItem = MP_SELECTIVE_CHUNK;
		else
			uCurSelectiveChunkMenuItem = MP_SELECTIVE_CHUNK+1 + pFile->GetSelectiveChunk();
		if (bFirstItem)
			uSelectiveChunkMenuItem = uCurSelectiveChunkMenuItem;
		else if (uSelectiveChunkMenuItem != uCurSelectiveChunkMenuItem)
			uSelectiveChunkMenuItem = 0;
		//MORPH END   - Added by SiRoB, HIDEOS
		
		//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
		UINT uCurShareOnlyTheNeedMenuItem = 0;
		if (pFile->GetShareOnlyTheNeed() == -1)
			uCurShareOnlyTheNeedMenuItem = MP_SHAREONLYTHENEED;
		else
			uCurShareOnlyTheNeedMenuItem = MP_SHAREONLYTHENEED+1 + pFile->GetShareOnlyTheNeed();
		if (bFirstItem)
			uShareOnlyTheNeedMenuItem = uCurShareOnlyTheNeedMenuItem ;
		else if (uShareOnlyTheNeedMenuItem != uCurShareOnlyTheNeedMenuItem)
			uShareOnlyTheNeedMenuItem = 0;
		//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

		bFirstItem = false;
	}

	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_PrioMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	m_PrioMenu.CheckMenuRadioItem(MP_PRIOVERYLOW, MP_PRIOAUTO, uPrioMenuItem, 0);

	bool bSingleCompleteFileSelected = (iSelectedItems == 1 && iCompleteFileSelected == 1);
	m_SharedFilesMenu.EnableMenuItem(MP_OPEN, bSingleCompleteFileSelected ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(MP_OPENFOLDER, bSingleCompleteFileSelected ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(MP_RENAME, bSingleCompleteFileSelected ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(MP_REMOVE, iCompleteFileSelected > 0 ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.SetDefaultItem(bSingleCompleteFileSelected ? MP_OPEN : -1);
	//MORPH START - Changed by SiRoB, BatchComment
	/*
	m_SharedFilesMenu.EnableMenuItem(MP_CMT, iSelectedItems == 1 ? MF_ENABLED : MF_GRAYED);
	*/
	m_SharedFilesMenu.EnableMenuItem(MP_CMT, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	//MORPH END   - Changed by SiRoB, BatchComment
	m_SharedFilesMenu.EnableMenuItem(MP_DETAIL, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);

	//MORPH START - Added by SiRoB, HIDEOS
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_HideOSMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_SelectiveChunkMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	CString buffer;
	if (thePrefs.GetHideOvershares()==0)
		buffer.Format(" (%s)",GetResString(IDS_DISABLED));
	else
		buffer.Format(" (%u)",thePrefs.GetHideOvershares());
	m_HideOSMenu.ModifyMenu(MP_HIDEOS_DEFAULT, MF_STRING,MP_HIDEOS_DEFAULT, GetResString(IDS_DEFAULT) + buffer);
	if (iHideOS==-1)
		buffer = "Set";
	else if (iHideOS==0)
		buffer = GetResString(IDS_DISABLED);
	else
		buffer.Format("%i", iHideOS);
	m_HideOSMenu.ModifyMenu(MP_HIDEOS_SET, MF_STRING,MP_HIDEOS_SET, buffer);
	m_HideOSMenu.CheckMenuRadioItem(MP_HIDEOS_DEFAULT, MP_HIDEOS_SET, uHideOSMenuItem, 0);
	buffer.Format(" (%s)",thePrefs.IsSelectiveShareEnabled()?GetResString(IDS_ENABLED):GetResString(IDS_DISABLED));
	m_SelectiveChunkMenu.ModifyMenu(MP_SELECTIVE_CHUNK, MF_STRING, MP_SELECTIVE_CHUNK, GetResString(IDS_DEFAULT) + buffer);
	m_SelectiveChunkMenu.CheckMenuRadioItem(MP_SELECTIVE_CHUNK, MP_SELECTIVE_CHUNK_1, uSelectiveChunkMenuItem, 0);
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_ShareOnlyTheNeedMenu.m_hMenu, iSelectedItems > 0 && iCompleteFileSelected ==1 ? MF_ENABLED : MF_GRAYED);
	buffer.Format(" (%s)",thePrefs.GetShareOnlyTheNeed()?GetResString(IDS_ENABLED):GetResString(IDS_DISABLED));
	m_ShareOnlyTheNeedMenu.ModifyMenu(MP_SHAREONLYTHENEED, MF_STRING, MP_SHAREONLYTHENEED, GetResString(IDS_DEFAULT) + buffer);
	m_ShareOnlyTheNeedMenu.CheckMenuRadioItem(MP_SHAREONLYTHENEED, MP_SHAREONLYTHENEED_1, uShareOnlyTheNeedMenuItem, 0);
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

	//MORPH START - Added by SiRoB, Show Share Permissions
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_PermMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	switch (thePrefs.GetPermissions()){
		case PERM_ALL:
			buffer.Format(" (%s)",GetResString(IDS_FSTATUS_PUBLIC));
			break;
		case PERM_FRIENDS:
			buffer.Format(" (%s)",GetResString(IDS_FSTATUS_FRIENDSONLY));
			break;
		case PERM_NOONE:
			buffer.Format(" (%s)",GetResString(IDS_HIDDEN));
			break;
		// Mighty Knife: Community visible filelist
		case PERM_COMMUNITY:
			buffer.Format(" (%s)",GetResString(IDS_COMMUNITY));
			break;
		// [end] Mighty Knife
		default:
			buffer = " (?)";
			break;
	}
	m_PermMenu.ModifyMenu(MP_PERMDEFAULT, MF_STRING, MP_PERMDEFAULT, GetResString(IDS_DEFAULT) + buffer);
	// Mighty Knife: Community visible filelist
	m_PermMenu.CheckMenuRadioItem(MP_PERMDEFAULT,MP_PERMCOMMUNITY, uPermMenuItem, 0);
	// [end] Mighty Knife
	//MORPH END   - Added by SiRoB, Show Share Permissions

	//MORPH START - Added by SiRoB, Avoid misusing of powershare
	m_SharedFilesMenu.EnableMenuItem((UINT_PTR)m_PowershareMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	switch (thePrefs.GetPowerShareMode()){
		case 0:
			buffer.Format(" (%s)",GetResString(IDS_POWERSHARE_DISABLED));
			break;
		case 1:
			buffer.Format(" (%s)",GetResString(IDS_POWERSHARE_ACTIVATED));
			break;
		case 2:
			buffer.Format(" (%s)",GetResString(IDS_POWERSHARE_AUTO));
			break;
		case 3:
			buffer.Format(" (%s)",GetResString(IDS_POWERSHARE_LIMITED));
			break;
		default:
			buffer = " (?)";
			break;
	}
	m_PowershareMenu.ModifyMenu(MP_POWERSHARE_DEFAULT, MF_STRING,MP_POWERSHARE_DEFAULT, GetResString(IDS_DEFAULT) + buffer);
	m_PowershareMenu.CheckMenuRadioItem(MP_POWERSHARE_DEFAULT, MP_POWERSHARE_LIMITED, uPowershareMenuItem, 0);
	//MORPH END   - Added by SiRoB, Avoid misusing of powershare
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	m_PowershareMenu.EnableMenuItem((UINT_PTR)m_PowerShareLimitMenu.m_hMenu, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	if (iPowerShareLimit==0)
		buffer.Format(" (%s)",GetResString(IDS_DISABLED));
	else
		buffer.Format(" (%u)",thePrefs.GetPowerShareLimit());
	m_PowerShareLimitMenu.ModifyMenu(MP_POWERSHARE_LIMIT, MF_STRING,MP_POWERSHARE_LIMIT, GetResString(IDS_DEFAULT) + buffer);
	if (iPowerShareLimit==-1)
		buffer = "Set";
	else if (iPowerShareLimit==0)
		buffer = GetResString(IDS_DISABLED);
	else
		buffer.Format("%i",iPowerShareLimit);
	m_PowerShareLimitMenu.ModifyMenu(MP_POWERSHARE_LIMIT_SET, MF_STRING,MP_POWERSHARE_LIMIT_SET, buffer);
	m_PowerShareLimitMenu.CheckMenuRadioItem(MP_POWERSHARE_LIMIT, MP_POWERSHARE_LIMIT_SET, uPowerShareLimitMenuItem, 0);
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	
	m_SharedFilesMenu.EnableMenuItem(MP_GETED2KLINK, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(MP_GETHTMLED2KLINK, iSelectedItems > 0 ? MF_ENABLED : MF_GRAYED);
	m_SharedFilesMenu.EnableMenuItem(MP_GETSOURCEED2KLINK, (iSelectedItems > 0 && theApp.IsConnected() && !theApp.IsFirewalled()) ? MF_ENABLED : MF_GRAYED);

	// itsonlyme: hostnameSource
	if (iSelectedItems > 0 && theApp.IsConnected() && !theApp.IsFirewalled() &&
		!CString(thePrefs.GetYourHostname()).IsEmpty() &&
		CString(thePrefs.GetYourHostname()).Find(_T(".")) != -1)
		m_SharedFilesMenu.EnableMenuItem(MP_GETHOSTNAMESOURCEED2KLINK, MF_ENABLED);
	else
		m_SharedFilesMenu.EnableMenuItem(MP_GETHOSTNAMESOURCEED2KLINK, MF_GRAYED);
	// itsonlyme: hostnameSource

	m_SharedFilesMenu.EnableMenuItem(Irc_SetSendLink, iSelectedItems == 1 && theApp.emuledlg->ircwnd->IsConnected() ? MF_ENABLED : MF_GRAYED);

	CMenu WebMenu;
	WebMenu.CreateMenu();
	int iWebMenuEntries = theWebServices.GetFileMenuEntries(WebMenu);
	UINT flag2 = (iWebMenuEntries == 0 || iSelectedItems != 1) ? MF_GRAYED : MF_STRING;
	m_SharedFilesMenu.AppendMenu(flag2 | MF_POPUP, (UINT_PTR)WebMenu.m_hMenu, GetResString(IDS_WEBSERVICES));
	
	GetPopupMenuPos(*this, point);
	m_SharedFilesMenu.TrackPopupMenu(TPM_LEFTALIGN |TPM_RIGHTBUTTON,point.x,point.y,this);

	m_SharedFilesMenu.RemoveMenu(m_SharedFilesMenu.GetMenuItemCount()-1,MF_BYPOSITION);
	VERIFY( WebMenu.DestroyMenu() );
}

BOOL CSharedFilesCtrl::OnCommand(WPARAM wParam, LPARAM lParam)
{
	CTypedPtrList<CPtrList, CKnownFile*> selectedList;
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos != NULL){
		int index = GetNextSelectedItem(pos);
		if (index >= 0)
			selectedList.AddTail((CKnownFile*)GetItemData(index));
	}

	if (selectedList.GetCount() > 0)
	{
		CKnownFile* file = NULL;
		if (selectedList.GetCount() == 1)
			file = selectedList.GetHead();

		switch (wParam){
			case Irc_SetSendLink:
				if (file)
				theApp.emuledlg->ircwnd->SetSendFileString(CreateED2kLink(file));
				break;
			case MP_GETED2KLINK:{
				CString str;
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if (!str.IsEmpty())
						str += _T("\r\n");
					str += CreateED2kLink(file);
					}

				theApp.CopyTextToClipboard(str);
				break; 
			}
			case MP_GETHTMLED2KLINK:{
					CString str;
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if (!str.IsEmpty())
						str += _T("<br />\r\n");
					str += CreateHTMLED2kLink(file);
				}
				theApp.CopyTextToClipboard(str);
				break; 
			} 
			case MP_GETSOURCEED2KLINK:{
				CString str;
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if (!str.IsEmpty())
						str += _T("\r\n");
					str += theApp.CreateED2kSourceLink(file); 
				}
				theApp.CopyTextToClipboard(str);
				break; 
			}
			// itsonlyme: hostnameSource
			case MP_GETHOSTNAMESOURCEED2KLINK:{
				CString str;
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if (!str.IsEmpty())
						str += _T("\r\n");
					str += theApp.CreateED2kHostnameSourceLink(file);
				}
				theApp.CopyTextToClipboard(str);
				break;
			}
			// file operations
			case MP_OPEN:
				if (file && !file->IsPartFile())
				OpenFile(file);
				break; 
			case MP_OPENFOLDER:
				if (file && !file->IsPartFile()){
					CString path = file->GetPath();
					int bspos = path.ReverseFind(_T('\\'));
					ShellExecute(NULL, _T("open"), path.Left(bspos), NULL, NULL, SW_SHOW);
				}
				break; 
			case MP_RENAME:
			case MPG_F2:
				if (file && !file->IsPartFile()){
					InputBox inputbox;
					CString title = GetResString(IDS_RENAME);
					title.Remove(_T('&'));
					inputbox.SetLabels(title, GetResString(IDS_DL_FILENAME), file->GetFileName());
					inputbox.SetEditFilenameMode();
					inputbox.DoModal();
					CString newname = inputbox.GetInput();
					if (!inputbox.WasCancelled() && newname.GetLength()>0)
					{
						// at least prevent users from specifying something like "..\dir\file"
						static const TCHAR _szInvFileNameChars[] = _T("\\/:*?\"<>|");
						if (newname.FindOneOf(_szInvFileNameChars) != -1){
							AfxMessageBox(GetErrorMessage(ERROR_BAD_PATHNAME));
							break;
						}

						CString newpath;
						PathCombine(newpath.GetBuffer(MAX_PATH), file->GetPath(), newname);
						newpath.ReleaseBuffer();
						if (_trename(file->GetFilePath(), newpath) != 0){
							CString strError;
							strError.Format(GetResString(IDS_ERR_RENAMESF), file->GetFilePath(), newpath, strerror(errno));
							AfxMessageBox(strError);
							break;
						}
						
						if (file->IsKindOf(RUNTIME_CLASS(CPartFile)))
							file->SetFileName(newname);
						else
						{
						theApp.sharedfiles->RemoveKeywords(file);
						file->SetFileName(newname);
						theApp.sharedfiles->AddKeywords(file);
						}
						file->SetFilePath(newpath);
						UpdateFile(file);
					}
				}
				else if (wParam == MPG_F2)
					MessageBeep((UINT)-1);
				break;
			case MP_REMOVE:
			case MPG_DELETE:{
				if (IDNO == AfxMessageBox(GetResString(IDS_CONFIRM_FILEDELETE),MB_ICONWARNING | MB_ICONQUESTION | MB_DEFBUTTON2 | MB_YESNO))
					return TRUE;

				while (!selectedList.IsEmpty())
				{
					CKnownFile* myfile = selectedList.GetHead();
					selectedList.RemoveHead();
					if (!myfile || myfile->IsPartFile())
						continue;
					
					bool delsucc = false;
					if (!PathFileExists(myfile->GetFilePath()))
						delsucc = true;
					else{
						// Delete
						if (!thePrefs.GetRemoveToBin()){
							delsucc = DeleteFile(myfile->GetFilePath());
						}
						else{
							// delete to recycle bin :(
							TCHAR todel[MAX_PATH+1];
							memset(todel, 0, sizeof todel);
							_tcsncpy(todel, myfile->GetFilePath(), ARRSIZE(todel)-2);

							SHFILEOPSTRUCT fp = {0};
							fp.wFunc = FO_DELETE;
							fp.hwnd = theApp.emuledlg->m_hWnd;
							fp.pFrom = todel;
							fp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT;// | FOF_NOERRORUI
							delsucc = (SHFileOperation(&fp) == 0);
						}
					}
					if (delsucc){
						theApp.sharedfiles->RemoveFile(myfile);
						if (myfile->IsKindOf(RUNTIME_CLASS(CPartFile)))
							theApp.emuledlg->transferwnd->downloadlistctrl.ClearCompleted(static_cast<CPartFile*>(myfile));
					}
					else{
						CString strError;
						strError.Format(_T("Failed to delete file \"%s\"\r\n\r\n%s"), myfile->GetFilePath(), GetErrorMessage(GetLastError()));
						AfxMessageBox(strError);
					}
				}
				break; 
			}
			case MP_CMT:
				//MORPH START - Added by SiRoB, derivated from SLUGFILLER: batchComment
				/*
				if (file)
					ShowComments(file);	
				break; 
				*/
				{
					if (file == NULL) file = selectedList.GetHead();
					CCommentDialog dialog(file); 
					if (dialog.DoModal() == IDOK) {
						POSITION pos = selectedList.GetHeadPosition();
						while (pos != NULL)
						{
							CKnownFile* otherfile = selectedList.GetNext(pos);
							if (otherfile == file)
								continue;
							otherfile->SetFileComment(file->GetFileComment());
							otherfile->SetFileRate(file->GetFileRate());
							UpdateFile(otherfile);
						}
					}
					break; 
				}
			//MORPH END - Added by SiRoB, derivated from SLUGFILLER: batchComment
			case MPG_ALTENTER:
			case MP_DETAIL:{
				CSharedFileDetailsSheet sheet(selectedList);
					sheet.DoModal();
				break;
			}
			case MP_PRIOVERYLOW:
			case MP_PRIOLOW:
			case MP_PRIONORMAL:
			case MP_PRIOHIGH:
			case MP_PRIOVERYHIGH:
			case MP_PRIOAUTO:
				{
					POSITION pos = selectedList.GetHeadPosition();
					while (pos != NULL)
					{
						CKnownFile* file = selectedList.GetNext(pos);
						switch (wParam) {
							case MP_PRIOVERYLOW:
								file->SetAutoUpPriority(false);
								file->SetUpPriority(PR_VERYLOW);
								UpdateItem(file);
								break;
							case MP_PRIOLOW:
								file->SetAutoUpPriority(false);
								file->SetUpPriority(PR_LOW);
								UpdateItem(file);
								break;
							case MP_PRIONORMAL:
								file->SetAutoUpPriority(false);
								file->SetUpPriority(PR_NORMAL);
								UpdateItem(file);
								break;
							case MP_PRIOHIGH:
								file->SetAutoUpPriority(false);
								file->SetUpPriority(PR_HIGH);
								UpdateItem(file);
								break;
							case MP_PRIOVERYHIGH:
								file->SetAutoUpPriority(false);
								file->SetUpPriority(PR_VERYHIGH);
								UpdateItem(file);
								break;	
							case MP_PRIOAUTO:
								file->SetAutoUpPriority(true);
								file->UpdateAutoUpPriority();
								UpdateFile(file); 
								break;
						}
					}
					break;
				}
			//MORPH START - Added by SiRoB, ZZ Upload System
			case MP_POWERSHARE_ON:
			case MP_POWERSHARE_OFF:
			//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
			case MP_POWERSHARE_DEFAULT:
			case MP_POWERSHARE_AUTO:
			case MP_POWERSHARE_LIMITED: //MORPH - Added by SiRoB, POWERSHARE Limit
			{
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					switch (wParam) {
						case MP_POWERSHARE_DEFAULT:
							file->SetPowerShared(-1);
							break;
						case MP_POWERSHARE_ON:
							file->SetPowerShared(1);
							break;
						case MP_POWERSHARE_OFF:
							file->SetPowerShared(0);
							break;
						case MP_POWERSHARE_AUTO:
							file->SetPowerShared(2);
							break;
						//MORPH START - Added by SiRoB, POWERSHARE Limit
						case MP_POWERSHARE_LIMITED:
							file->SetPowerShared(3);
							break;
						//MORPH END   - Added by SiRoB, POWERSHARE Limit
					}
					UpdateFile(file);
				}
				break;
			}
			//MORPH END   - Changed by SiRoB, Avoid misusing of powersharing
			//MORPH END   - Added by SiRoB, ZZ Upload System
			//MORPH START - Added by SiRoB, POWERSHARE Limit
			case MP_POWERSHARE_LIMIT:
			case MP_POWERSHARE_LIMIT_SET:
			{
				POSITION pos = selectedList.GetHeadPosition();
				int newPowerShareLimit = -1;
				if (wParam==MP_POWERSHARE_LIMIT_SET)
				{
					InputBox inputbox;
					CString title=GetResString(IDS_POWERSHARE);
					CString currPowerShareLimit;
					if (file)
						currPowerShareLimit.Format("%i", (file->GetPowerShareLimit()>=0)?file->GetPowerShareLimit():thePrefs.GetPowerShareLimit());
					else
						currPowerShareLimit = "0";
					inputbox.SetLabels(GetResString(IDS_POWERSHARE), GetResString(IDS_POWERSHARE_LIMIT), currPowerShareLimit);
					inputbox.SetNumber(true);
					int result = inputbox.DoModal();
					if (result == IDCANCEL || (newPowerShareLimit = inputbox.GetInputInt()) < 0)
						break;
				}

				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if  (newPowerShareLimit == file->GetPowerShareLimit()) break;
					file->SetPowerShareLimit(newPowerShareLimit);
					file->UpdatePartsInfo();
					UpdateFile(file);
					break;
				}
				break;
			}
			//MORPH END   - Added by SiRoB, POWERSHARE Limit
			//MORPH START - Added by SiRoB, HIDEOS
			case MP_HIDEOS_DEFAULT:
			case MP_HIDEOS_SET:
			{
				POSITION pos = selectedList.GetHeadPosition();
				int newHideOS = -1;
				if (wParam==MP_HIDEOS_SET)
				{
					InputBox inputbox;
					CString title=GetResString(IDS_HIDEOS);
					CString currHideOS;
					if (file)
						currHideOS.Format("%i", (file->GetHideOS()>=0)?file->GetHideOS():thePrefs.GetHideOvershares());
					else
						currHideOS = "0";
					inputbox.SetLabels(GetResString(IDS_HIDEOS), GetResString(IDS_HIDEOVERSHARES), currHideOS);
					inputbox.SetNumber(true);
					int result = inputbox.DoModal();
					if (result == IDCANCEL || (newHideOS = inputbox.GetInputInt()) < 0)
						break;
				}

				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if  (newHideOS == file->GetHideOS()) break;
					file->SetHideOS(newHideOS);
					UpdateFile(file);
					break;
				}
				break;
			}
			//MORPH END   - Added by SiRoB, HIDEOS
			// xMule_MOD: showSharePermissions
			// with itsonlyme's sorting fix
			case MP_PERMDEFAULT:
			case MP_PERMNONE:
			case MP_PERMFRIENDS:
			// Mighty Knife: Community visible filelist
			case MP_PERMCOMMUNITY:
			// [end] Mighty Knife
			case MP_PERMALL: {
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					switch (wParam)
					{
						case MP_PERMDEFAULT:
							file->SetPermissions(-1);
							UpdateFile(file);
							break;
						case MP_PERMNONE:
							file->SetPermissions(PERM_NOONE);
							UpdateFile(file);
							break;
						case MP_PERMFRIENDS:
							file->SetPermissions(PERM_FRIENDS);
							UpdateFile(file);
							break;
						// Mighty Knife: Community visible filelist
						case MP_PERMCOMMUNITY:
							file->SetPermissions(PERM_COMMUNITY);
							UpdateFile(file);
							break;
						// [end] Mighty Knife
						default : // case MP_PERMALL:
							file->SetPermissions(PERM_ALL);
							UpdateFile(file);
							break;
					}
				}
				Invalidate();
				break;
			}
		    // Mighty Knife: CRC32-Tag
			case MP_CALCCRC32: 
				if (!selectedList.IsEmpty()){
					// For every chosen file create a worker thread and add it
					// to the file processing thread
					POSITION pos = selectedList.GetHeadPosition();
					while (pos != NULL) {
						CCRC32CalcWorker* worker = new CCRC32CalcWorker;
						CKnownFile*  file = selectedList.GetAt (pos);
						const uchar* FileHash = file->GetFileHash ();
						worker->SetFileHashToProcess (FileHash);
						worker->SetFilePath (file->GetFilePath ());
						m_FileProcessingThread.AddFileProcessingWorker (worker);
						selectedList.GetNext (pos);
					}
					// Start the file processing thread to process the files.
					if (!m_FileProcessingThread.IsRunning ()) {
						// (Re-)start the thread, this will do the rest

						// If the thread object already exists, this will result in an
						// ASSERT - but that doesn't matter since we manually take care
						// that the thread does not run at this moment...
						m_FileProcessingThread.CreateThread ();
					}
				}
				break;
			case MP_ABORTCRC32CALC:
				// Message the File processing thread to stop any pending calculations
				if (m_FileProcessingThread.IsRunning ())
					m_FileProcessingThread.Terminate ();
				break;
			case MP_ADDCRC32TOFILENAME:
				if (!selectedList.IsEmpty()){
					AddCRC32InputBox AddCRCDialog;
					int result = AddCRCDialog.DoModal ();
					if (result == IDOK) {
						// For every chosen file create a worker thread and add it
						// to the file processing thread
						POSITION pos = selectedList.GetHeadPosition();
						while (pos != NULL) {
							// first create a worker thread that calculates the CRC
							// if it's not already calculated...
							CKnownFile*  file = selectedList.GetAt (pos);
							const uchar* FileHash = file->GetFileHash ();

							// But don't add the worker thread if the CRC32 should not
							// be calculated !
							if (!AddCRCDialog.GetDontAddCRC32 ()) {
								CCRC32CalcWorker* workercrc = new CCRC32CalcWorker;
								workercrc->SetFileHashToProcess (FileHash);
								workercrc->SetFilePath (file->GetFilePath ());
								m_FileProcessingThread.AddFileProcessingWorker (workercrc);
							}

							// Now add a worker thread that informs this window when
							// the calculation is completed.
							// The method OnCRC32RenameFilename will then rename the file
							CCRC32RenameWorker* worker = new CCRC32RenameWorker;
							worker->SetFileHashToProcess (FileHash);
							worker->SetFilePath (file->GetFilePath ());
							worker->SetFilenamePrefix (AddCRCDialog.GetCRC32Prefix ());
							worker->SetFilenameSuffix (AddCRCDialog.GetCRC32Suffix ());
							worker->SetDontAddCRCAndSuffix (AddCRCDialog.GetDontAddCRC32 ());
							m_FileProcessingThread.AddFileProcessingWorker (worker);

							// next file
							selectedList.GetNext (pos);
						}
						// Start the file processing thread to process the files.
						if (!m_FileProcessingThread.IsRunning ()) {
							// (Re-)start the thread, this will do the rest

							// If the thread object already exists, this will result in an
							// ASSERT - but that doesn't matter since we manually take care
							// that the thread does not run at this moment...
							m_FileProcessingThread.CreateThread ();
						}
					}
				}
				break;
			// [end] Mighty Knife

			// Mighty Knife: Mass Rename
			case MP_MASSRENAME: {
					CMassRenameDialog MRDialog;
					// Add the files to the dialog
					POSITION pos = selectedList.GetHeadPosition();
					while (pos != NULL) {
						CKnownFile*  file = selectedList.GetAt (pos);
						MRDialog.m_FileList.AddTail (file);
						selectedList.GetNext (pos);
					}
					int result = MRDialog.DoModal ();
					if (result == IDOK) {
						// The user has successfully entered new filenames. Now we have
						// to rename all the files...
						POSITION pos = selectedList.GetHeadPosition();
						int i=0;
						while (pos != NULL) {
							CString newname = MRDialog.m_NewFilenames.at (i);
							CString newpath = MRDialog.m_NewFilePaths.at (i);
							CKnownFile* file = selectedList.GetAt (pos);
							// .part files could be renamed by simply changing the filename
							// in the CKnownFile object.
							if ((!file->IsPartFile()) && (_trename(file->GetFilePath(), newpath) != 0)){
								CString strError;
								strError.Format(_T("Failed to rename '%s' to '%s', Error: %hs"), file->GetFilePath(), newpath, strerror(errno));
								AddLogLine(false,strError);
							} else {
								CString strres;
								if (!file->IsPartFile()) {
									strres.Format(_T("Successfully renamed '%s' to '%s'"), file->GetFilePath(), newpath);
									theApp.sharedfiles->RemoveKeywords(file);
									file->SetFileName(newname);
									theApp.sharedfiles->AddKeywords(file);
									file->SetFilePath(newpath);
									UpdateFile(file);
								} else {
									strres.Format(_T("Successfully renamed .part file '%s' to '%s'"), file->GetFileName(), newname);
									AddLogLine(false,strres);
									file->SetFileName(newname, true); 
									((CPartFile*) file)->UpdateDisplayedInfo();
									((CPartFile*) file)->SavePartFile(); 
									UpdateFile(file);
								}
							}

							// Next item
							selectedList.GetNext (pos);
							i++;
						}
					}
				}
				break;
			// [end] Mighty Knife
			// xMule_MOD: showSharePermissions
			default:
				POSITION pos = selectedList.GetHeadPosition();
				while (pos != NULL)
				{
					file = selectedList.GetNext(pos);
					if (wParam>=MP_WEBURL && wParam<=MP_WEBURL+256){
						theWebServices.RunURL(file, wParam);
					}
					//MORPH START - Added by SiRoB, HIDEOS
					else if (wParam>=MP_SELECTIVE_CHUNK && wParam<=MP_SELECTIVE_CHUNK_1){
						file->SetSelectiveChunk(wParam==MP_SELECTIVE_CHUNK?-1:wParam-MP_SELECTIVE_CHUNK_0);
						UpdateFile(file);
					//MORPH END   - Added by SiRoB, HIDEOS
					//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
					}else if (wParam>=MP_SHAREONLYTHENEED && wParam<=MP_SHAREONLYTHENEED_1){
						file->SetShareOnlyTheNeed(wParam==MP_SHAREONLYTHENEED?-1:wParam-MP_SHAREONLYTHENEED_0);
						UpdateFile(file);
					//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
					}
				}
				break;
		}
	}
	return TRUE;
}

// Mighty Knife: CRC32-Tag - Save rename
// File will be renamed in this method at the time it can be renamed.
// Might be that the CRC had to be calculated before so the thread will inform
// this window when the CRC is ok and the file can be renamed...
afx_msg LRESULT CSharedFilesCtrl::OnCRC32RenameFile	(WPARAM wParam, LPARAM lParam) {
	// Get the worker thread
	CCRC32RenameWorker* worker = (CCRC32RenameWorker*) lParam;
	// In this case the worker thread is a helper thread for this routine !

	// We are in the "main" thread, so we can be sure that the filelist is not
	// deleted while we access it - so we try to get a pointer to the desired file
	// directly without worker->ValidateKnownFile !
	// This of course avoids possible deadlocks because we don't lock the list;
	// and we don't need to do an worker->UnlockSharedFilesList...
	CKnownFile* f = theApp.sharedfiles->GetFileByID (worker->GetFileHashToProcess ());
	if (f==NULL) {
		// File doesn't exist in the list; deleted and reloaded the shared files list in
		// the meantime ?
		// Let's hope the creator of this Worker thread has set the filename so we can
		// display it...
		if (worker->GetFilePath () == "") {
			theApp.AddLogLine (false,"Warning: A File that should be renamed is not shared anymore. Renaming skipped.");
		} else {
			theApp.AddLogLine (false,"Warning: File '%s' is not shared anymore. File is not renamed.",
				worker->GetFilePath ());
		}
		return 0;         
	}
	if (f->IsPartFile () && !worker->m_DontAddCRCAndSuffix) {     
		// We can't add a CRC suffix to files which are not complete
		theApp.AddLogLine (false,"Can't add CRC to file '%s'; file is a part file and not complete !",
						   f->GetFileName ());
		return 0;
	}
	if (!worker->m_DontAddCRCAndSuffix && !f->IsCRC32Calculated ()) {
		// The CRC must have been calculate, otherwise we can't add it.
		// Normally this mesage is not shown because if the CRC is not calculated
		// the main thread creates a worker thread before to calculate it...
		theApp.AddLogLine (false,"Can't add CRC32 to file '%s'; CRC is not calculated !",
						   f->GetFileName ());
		return 0;
	}

	// Split the old filename to name and extension
	CString fn = f->GetFileName ();
	// test if the file name already contained the CRC tag
	CString fnup = fn;
	fnup.MakeUpper();
	if(f->IsCRC32Calculated() && (fnup.Find(f->GetLastCalculatedCRC32()) != -1)){
		theApp.AddLogLine (false, "File '%s' already containes the correct CRC32 tag, won't be renamed.", fn);
		return 0;
	}
	CString p3,p4;
	_splitpath (fn,NULL,NULL,p3.GetBuffer (MAX_PATH),p4.GetBuffer (MAX_PATH));
	p3.ReleaseBuffer();
	p4.ReleaseBuffer();

	// Create the new filename
	CString NewFn = p3;
	NewFn = NewFn + worker->m_FilenamePrefix;
	if (!worker->m_DontAddCRCAndSuffix) {
		NewFn = NewFn + f->GetLastCalculatedCRC32 () + worker->m_FilenameSuffix;
	}
	NewFn = NewFn + p4;

	theApp.AddLogLine (false,"File '%s' will be renamed to '%s'...",fn,NewFn);

	// Add the path of the old filename to the new one
	CString NewPath; 
	PathCombine(NewPath.GetBuffer(MAX_PATH), f->GetPath (), NewFn);
	NewPath.ReleaseBuffer();

	// Try to rename
	if ((!f->IsPartFile()) && (_trename(f->GetFilePath (), NewPath) != 0)) {
		theApp.AddLogLine (false,"Can't rename file '%s' ! Error: %hs",fn,strerror(errno));
	} else {
		CString strres;
		if (!f->IsPartFile()) {
			strres.Format(_T("Successfully renamed file '%s' to '%s'"), f->GetFileName(), NewPath);
			AddLogLine(false,strres);

			theApp.sharedfiles->RemoveKeywords(f);
			f->SetFileName(NewFn);
			theApp.sharedfiles->AddKeywords(f);
			f->SetFilePath(NewPath);
			UpdateFile (f);
		} else {
			strres.Format(_T("Successfully renamed .part file '%s' to '%s'"), f->GetFileName(), NewFn);
			AddLogLine(false,strres);

			f->SetFileName(NewFn, true); 
			((CPartFile*) f)->UpdateDisplayedInfo();
			((CPartFile*) f)->SavePartFile(); 
			UpdateFile(f);
		}
	}
	
	return 0;
}

// Update the file which CRC was just calculated.
// The LPARAM parameter is a pointer to the hash of the file to be updated.
LRESULT CSharedFilesCtrl::OnCRC32UpdateFile	(WPARAM wParam, LPARAM lParam) {
	uchar* filehash = (uchar*) lParam;
	// We are in the "main" thread, so we can be sure that the filelist is not
	// deleted while we access it - so we try to get a pointer to the desired file
	// directly without worker->ValidateKnownFile !
	// This of course avoids possible deadlocks because we don't lock the list;
	// and we don't need to do an worker->UnlockSharedFilesList...
	CKnownFile* file = theApp.sharedfiles->GetFileByID (filehash);
	if (file != NULL)		// Update the file if it exists
		UpdateFile (file);
	return 0;
}
// [end] Mighty Knife

void CSharedFilesCtrl::UpdateItem(CKnownFile* file)
{
	LVFINDINFO info;
	info.flags = LVFI_PARAM;
	info.lParam = (LPARAM)file;
	int iItem = FindItem(&info);
	if (iItem >= 0)
		Update(iItem);
}

void CSharedFilesCtrl::OnColumnClick( NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	// Barry - Store sort order in preferences
	// Determine ascending based on whether already sorted on this column
	int sortItem = thePrefs.GetColumnSortItem(CPreferences::tableShared);
	bool m_oldSortAscending = thePrefs.GetColumnSortAscending(CPreferences::tableShared);
	bool sortAscending = (sortItem != pNMListView->iSubItem) ? true : !m_oldSortAscending;

	// Item is column clicked
	sortItem = pNMListView->iSubItem;

	// Save new preferences
	thePrefs.SetColumnSortItem(CPreferences::tableShared, sortItem);
	thePrefs.SetColumnSortAscending(CPreferences::tableShared, sortAscending);

	// Ornis 4-way-sorting
	int adder=0;
	if (pNMListView->iSubItem>=5 && pNMListView->iSubItem<=7)
	{
		ASSERT( pNMListView->iSubItem - 5 < ARRSIZE(sortstat) );
		if (!sortAscending)
			sortstat[pNMListView->iSubItem - 5] = !sortstat[pNMListView->iSubItem - 5];
		adder = sortstat[pNMListView->iSubItem-5] ? 0 : 100;
	}
	else if (pNMListView->iSubItem==11)
	{
		ASSERT( 3 < ARRSIZE(sortstat) );
		if (!sortAscending)
			sortstat[3] = !sortstat[3];
		adder = sortstat[3] ? 0 : 100;
	}

	// Sort table
	if (adder==0)	
		SetSortArrow(sortItem, sortAscending); 
	else
		SetSortArrow(sortItem, sortAscending ? arrowDoubleUp : arrowDoubleDown);
	// Mighty Knife: CRC32-Tag - Indexes shifted by 10
	SortItems(SortProc, sortItem + adder + (sortAscending ? 0:30));
	// [end] Mighty Knife

	*pResult = 0;
}

int CSharedFilesCtrl::SortProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	// Mighty Knife: CRC32-Tag - Indexes shifted by 10 !
	const CKnownFile* item1 = (CKnownFile*)lParam1;
	const CKnownFile* item2 = (CKnownFile*)lParam2;	
	switch(lParamSort){
		case 0: //filename asc
			return _tcsicmp(item1->GetFileName(),item2->GetFileName());
		case 30: //filename desc
			return _tcsicmp(item2->GetFileName(),item1->GetFileName());

		case 1: //filesize asc
			return item1->GetFileSize()==item2->GetFileSize()?0:(item1->GetFileSize()>item2->GetFileSize()?1:-1);

		case 31: //filesize desc
			return item1->GetFileSize()==item2->GetFileSize()?0:(item2->GetFileSize()>item1->GetFileSize()?1:-1);


		case 2: //filetype asc
			return item1->GetFileType().CompareNoCase(item2->GetFileType());
		case 32: //filetype desc
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
		case 33: //prio desc
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
		case 4: //fileID asc
			return memcmp(item1->GetFileHash(), item2->GetFileHash(), 16);
		case 34: //fileID desc
			return memcmp(item2->GetFileHash(), item1->GetFileHash(), 16);

		case 5: //requests asc
			return item1->statistic.GetRequests() - item2->statistic.GetRequests();
		case 35: //requests desc
			return item2->statistic.GetRequests() - item1->statistic.GetRequests();
		
		case 6: //acc requests asc
			return item1->statistic.GetAccepts() - item2->statistic.GetAccepts();
		case 36: //acc requests desc
			return item2->statistic.GetAccepts() - item1->statistic.GetAccepts();
		
		case 7: //all transferred asc
			return item1->statistic.GetTransferred()==item2->statistic.GetTransferred()?0:(item1->statistic.GetTransferred()>item2->statistic.GetTransferred()?1:-1);
		case 37: //all transferred desc
			return item1->statistic.GetTransferred()==item2->statistic.GetTransferred()?0:(item2->statistic.GetTransferred()>item1->statistic.GetTransferred()?1:-1);

		case 9: //folder asc
			return _tcsicmp(item1->GetPath(),item2->GetPath());
		case 39: //folder desc
			return _tcsicmp(item2->GetPath(),item1->GetPath());

		case 10: //complete sources asc
			return CompareUnsigned(item1->m_nCompleteSourcesCount, item2->m_nCompleteSourcesCount);
		case 40: //complete sources desc
			return CompareUnsigned(item2->m_nCompleteSourcesCount, item1->m_nCompleteSourcesCount);
		case 11: //ed2k shared asc
			return item1->GetPublishedED2K() - item2->GetPublishedED2K();
		case 41: //ed2k shared desc
			return item2->GetPublishedED2K() - item1->GetPublishedED2K();

		case 12: //permission asc
			return item2->GetPermissions()-item1->GetPermissions();
		case 42: //permission desc
			return item1->GetPermissions()-item2->GetPermissions();

		
		//MORPH START - Changed by SiRoB, Avoid misusing of powersharing
		//MORPH START - Added by SiRoB, ZZ Upload System
		case 13:
			if (item1->GetPowerShared() == false && item2->GetPowerShared() == true)
				return -1;
			else if (item1->GetPowerShared() == true && item2->GetPowerShared() == false)
				return 1;
			else
				if (item1->GetPowerSharedMode() != item2->GetPowerSharedMode())
					return item1->GetPowerSharedMode() - item2->GetPowerSharedMode();
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
							//MORPH START - Added by SiRoB, POWERSHARE Limit
							if (item1->GetPowerShareLimited() == false && item2->GetPowerShareLimited() == true)
								return -1;
							else if (item1->GetPowerShareLimited() == true && item2->GetPowerShareLimited() == false)
								return 1;
							else
							//MORPH END   - Added by SiRoB, POWERSHARE Limit
								return 0;
		case 43:
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
							//MORPH START - Added by SiRoB, POWERSHARE Limit
							if (item2->GetPowerShareLimited() == false && item1->GetPowerShareLimited() == true)
								return -1;
							else if (item2->GetPowerShareLimited() == true && item1->GetPowerShareLimited() == false)
								return 1;
							else
							//MORPH END   - Added by SiRoB, POWERSHARE Limit
								return 0;
		//MORPH END - Added by SiRoB, ZZ Upload System
		//MORPH END - Changed by SiRoB, Avoid misusing of powersharing
		//MORPH START - Added by IceCream, SLUGFILLER: Spreadbars
		case 14: //spread asc
		case 15:
			return 10000*(((CKnownFile*)lParam1)->statistic.GetSpreadSortValue()-((CKnownFile*)lParam2)->statistic.GetSpreadSortValue());
		case 44: //spread desc
		case 45:
			return 10000*(((CKnownFile*)lParam2)->statistic.GetSpreadSortValue()-((CKnownFile*)lParam1)->statistic.GetSpreadSortValue());

		case 16: // VQB:  Simple UL asc
		case 46: //VQB:  Simple UL desc
			{
				float x1 = ((float)item1->statistic.GetAllTimeTransferred())/((float)item1->GetFileSize());
				float x2 = ((float)item2->statistic.GetAllTimeTransferred())/((float)item2->GetFileSize());
				if (lParamSort == 13) return 10000*(x1-x2); else return 10000*(x2-x1);
			}
		case 17: // SF:  Full Upload Count asc
			return 10000*(((CKnownFile*)lParam1)->statistic.GetFullSpreadCount()-((CKnownFile*)lParam2)->statistic.GetFullSpreadCount());
		case 47: // SF:  Full Upload Count desc
			return 10000*(((CKnownFile*)lParam2)->statistic.GetFullSpreadCount()-((CKnownFile*)lParam1)->statistic.GetFullSpreadCount());
		//MORPH END   - Added by IceCream, SLUGFILLER: Spreadbars
		//MORPH START - Added by SiRoB, HIDEOS
		case 18:
			if (item1->GetHideOS() == item2->GetHideOS())
				return item1->GetSelectiveChunk() - item2->GetSelectiveChunk();
			else
				return item1->GetHideOS() - item2->GetHideOS();
		case 38:
			if (item2->GetHideOS() == item1->GetHideOS())
				return item2->GetSelectiveChunk() - item1->GetSelectiveChunk();
			else
				return item2->GetHideOS() - item1->GetHideOS();
		//MORPH END   - Added by SiRoB, HIDEOS
		//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
		case 19:
			return item1->GetShareOnlyTheNeed() - item2->GetShareOnlyTheNeed();
		case 49:
			return item2->GetShareOnlyTheNeed() - item1->GetShareOnlyTheNeed();
		//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED
		case 105: //all requests asc
			return item1->statistic.GetAllTimeRequests() - item2->statistic.GetAllTimeRequests();
		case 135: //all requests desc
			return item2->statistic.GetAllTimeRequests() - item1->statistic.GetAllTimeRequests();
		case 106: //all acc requests asc
			return item1->statistic.GetAllTimeAccepts() - item2->statistic.GetAllTimeAccepts();
		case 136: //all acc requests desc
			return item2->statistic.GetAllTimeAccepts() - item1->statistic.GetAllTimeAccepts();
		case 107: //all transferred asc
			return item1->statistic.GetAllTimeTransferred()==item2->statistic.GetAllTimeTransferred()?0:(item1->statistic.GetAllTimeTransferred()>item2->statistic.GetAllTimeTransferred()?1:-1);
		case 137: //all transferred desc
			return item1->statistic.GetAllTimeTransferred()==item2->statistic.GetAllTimeTransferred()?0:(item2->statistic.GetAllTimeTransferred()>item1->statistic.GetAllTimeTransferred()?1:-1);

		case 111:{ //kad shared asc
			uint32 tNow = time(NULL);
			int i1 = (tNow - item1->GetLastPublishTimeKadSrc() < KADEMLIAREPUBLISHTIMES) ? 1 : 0;
			int i2 = (tNow - item2->GetLastPublishTimeKadSrc() < KADEMLIAREPUBLISHTIMES) ? 1 : 0;
			return i1 - i2;
		}
		case 141:{ //kad shared desc
			uint32 tNow = time(NULL);
			int i1 = (tNow - item1->GetLastPublishTimeKadSrc() < KADEMLIAREPUBLISHTIMES) ? 1 : 0;
			int i2 = (tNow - item2->GetLastPublishTimeKadSrc() < KADEMLIAREPUBLISHTIMES) ? 1 : 0;
			return i2 - i1;
		}
		default: 
			return 0;
	}
}

void CSharedFilesCtrl::UpdateFile(const CKnownFile* toupdate)
{
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

void CSharedFilesCtrl::ShowFilesCount()
{
	CString counter;
	// SLUGFILLER: SafeHash - hashing counter
	if (theApp.sharedfiles->GetHashingCount())
		counter.Format(_T(" (%i/%i)"), theApp.sharedfiles->GetCount(), theApp.sharedfiles->GetCount()+theApp.sharedfiles->GetHashingCount());
	else
		counter.Format(_T(" (%i)"), theApp.sharedfiles->GetCount());
	// SLUGFILLER: SafeHash
	theApp.emuledlg->sharedfileswnd->GetDlgItem(IDC_TRAFFIC_TEXT)->SetWindowText(GetResString(IDS_SF_FILES)+counter  );
}

void CSharedFilesCtrl::OpenFile(const CKnownFile* file)
{
	ShellOpenFile(file->GetFilePath(), NULL);
}

void CSharedFilesCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	int iSel = GetNextItem(-1, LVIS_SELECTED | LVIS_FOCUSED);
	if (iSel != -1)
	{
		CKnownFile* file = (CKnownFile*)GetItemData(iSel);
		if (file)
		{
			//MORPH Changed by SiRoB - Double click unfinished files in SharedFile window display FileDetail
			/*
			if (GetKeyState(VK_MENU) & 0x8000){
			*/
			if (GetKeyState(VK_MENU) & 0x8000 || file->IsPartFile())
			{
				CTypedPtrList<CPtrList, CKnownFile*> aFiles;
				aFiles.AddHead(file);
				CSharedFileDetailsSheet sheet(aFiles);
				sheet.DoModal();
			}
			else if (!file->IsPartFile())
				OpenFile(file);
		}
	}
	*pResult = 0;
}

void CSharedFilesCtrl::CreateMenues()
{
	if (m_PrioMenu) VERIFY( m_PrioMenu.DestroyMenu() );
	//MORPH START - Added by SiRoB, Keep Prermission Flag
	if (m_PermMenu) VERIFY( m_PermMenu.DestroyMenu() );
	//MORPH END   - Added by SiRoB, Keep Prermission Flag
	//MORPH START - Added by SiRoB, ZZ Upload System
	if (m_PowershareMenu) VERIFY( m_PowershareMenu.DestroyMenu() );
	//MORPH END - Added by SiRoB, ZZ Upload System
	//MORPH START - Added by SiRoB, POWERSHARE LImit
	if (m_PowerShareLimitMenu) VERIFY( m_PowerShareLimitMenu.DestroyMenu() );
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	//MORPH START - Added by SiRoB, HIDEOS
	if (m_HideOSMenu) VERIFY( m_HideOSMenu.DestroyMenu() );
	if (m_SelectiveChunkMenu) VERIFY( m_SelectiveChunkMenu.DestroyMenu() );
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	if (m_ShareOnlyTheNeedMenu) VERIFY( m_ShareOnlyTheNeedMenu.DestroyMenu() );
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

	if (m_SharedFilesMenu) VERIFY( m_SharedFilesMenu.DestroyMenu() );

	// add priority switcher
	m_PrioMenu.CreateMenu();
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOVERYLOW,GetResString(IDS_PRIOVERYLOW));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOLOW,GetResString(IDS_PRIOLOW));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIONORMAL,GetResString(IDS_PRIONORMAL));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOHIGH, GetResString(IDS_PRIOHIGH));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOVERYHIGH, GetResString(IDS_PRIORELEASE));
	m_PrioMenu.AppendMenu(MF_STRING,MP_PRIOAUTO, GetResString(IDS_PRIOAUTO));//UAP

	//MORPH START - Added by SiRoB, Show Permissions
	m_PermMenu.CreateMenu();
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMDEFAULT,	GetResString(IDS_DEFAULT));
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMNONE,	GetResString(IDS_HIDDEN));
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMFRIENDS,	GetResString(IDS_FSTATUS_FRIENDSONLY));
	// Mighty Knife: Community visible filelist
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMCOMMUNITY,GetResString(IDS_COMMUNITY));
	// [end] Mighty Knife
	m_PermMenu.AppendMenu(MF_STRING,MP_PERMALL,		GetResString(IDS_FSTATUS_PUBLIC));
	//MORPH END   - Added by SiRoB, Show Permissions

	//MORPH START - Added by SiRoB, ZZ Upload System
	// add powershare switcher
	m_PowershareMenu.CreateMenu();
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_DEFAULT,GetResString(IDS_DEFAULT));
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_OFF,GetResString(IDS_POWERSHARE_OFF));
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_ON,GetResString(IDS_POWERSHARE_ON));
	//MORPH START - Added by SiRoB, Avoid misusing of powersharing
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_AUTO,GetResString(IDS_POWERSHARE_AUTO));
	//MORPH END   - Added by SiRoB, Avoid misusing of powersharing
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	m_PowershareMenu.AppendMenu(MF_STRING,MP_POWERSHARE_LIMITED,GetResString(IDS_POWERSHARE_LIMITED)); 
	m_PowerShareLimitMenu.CreateMenu();
	m_PowerShareLimitMenu.AppendMenu(MF_STRING,MP_POWERSHARE_LIMIT,	GetResString(IDS_DEFAULT));
	m_PowerShareLimitMenu.AppendMenu(MF_STRING,MP_POWERSHARE_LIMIT_SET,	GetResString(IDS_DISABLED));
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	//MORPH END   - Added by SiRoB, ZZ Upload System

	//MORPH START - Added by SiRoB, HIDEOS
	m_HideOSMenu.CreateMenu();
	m_HideOSMenu.AppendMenu(MF_STRING,MP_HIDEOS_DEFAULT, GetResString(IDS_DEFAULT));
	m_HideOSMenu.AppendMenu(MF_STRING,MP_HIDEOS_SET, GetResString(IDS_DISABLED));
	m_SelectiveChunkMenu.CreateMenu();
	m_SelectiveChunkMenu.AppendMenu(MF_STRING,MP_SELECTIVE_CHUNK,	GetResString(IDS_DEFAULT));
	m_SelectiveChunkMenu.AppendMenu(MF_STRING,MP_SELECTIVE_CHUNK_0,	GetResString(IDS_DISABLED));
	m_SelectiveChunkMenu.AppendMenu(MF_STRING,MP_SELECTIVE_CHUNK_1,	GetResString(IDS_ENABLED));
	//MORPH END   - Added by SiRoB, HIDEOS
	//MORPH START - Added by SiRoB, SHARE_ONLY_THE_NEED
	m_ShareOnlyTheNeedMenu.CreateMenu();
	m_ShareOnlyTheNeedMenu.AppendMenu(MF_STRING,MP_SHAREONLYTHENEED,	GetResString(IDS_DEFAULT));
	m_ShareOnlyTheNeedMenu.AppendMenu(MF_STRING,MP_SHAREONLYTHENEED_0,	GetResString(IDS_DISABLED));
	m_ShareOnlyTheNeedMenu.AppendMenu(MF_STRING,MP_SHAREONLYTHENEED_1,	GetResString(IDS_ENABLED));
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

	m_SharedFilesMenu.CreatePopupMenu();
	m_SharedFilesMenu.AddMenuTitle(GetResString(IDS_SHAREDFILES));

	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_OPEN, GetResString(IDS_OPENFILE));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_OPENFOLDER, GetResString(IDS_OPENFOLDER));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_RENAME, GetResString(IDS_RENAME) + _T("..."));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_REMOVE, GetResString(IDS_DELETE));

	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR);
	//MOPRH START - Added by SiRoB, Keep permission flag	
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PermMenu.m_hMenu, GetResString(IDS_PERMISSION));	// xMule_MOD: showSharePermissions - done
	//MOPRH END   - Added by SiRoB, Keep permission flag
	//MORPH START - Added by SiRoB, ZZ Upload System
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PowershareMenu.m_hMenu, GetResString(IDS_POWERSHARE));
	//MORPH END - Added by SiRoB, ZZ Upload System
	//MORPH START - Added by SiRoB, POWERSHARE Limit
	m_PowershareMenu.AppendMenu(MF_STRING|MF_SEPARATOR);
	m_PowershareMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PowerShareLimitMenu.m_hMenu, GetResString(IDS_POWERSHARE_LIMIT));
	//MORPH END   - Added by SiRoB, POWERSHARE Limit
	//MORPH START - Added by SiRoB, HIDEOS
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_HideOSMenu.m_hMenu, GetResString(IDS_HIDEOS));
	m_HideOSMenu.AppendMenu(MF_STRING|MF_SEPARATOR);
	m_HideOSMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_SelectiveChunkMenu.m_hMenu, GetResString(IDS_SELECTIVESHARE));
	//MORPH END   - Added by SiRoB, HIDEOS

	//MORPH START - Added by SiRoB,	SHARE_ONLY_THE_NEED
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_ShareOnlyTheNeedMenu.m_hMenu, GetResString(IDS_SHAREONLYTHENEED));
	//MORPH END   - Added by SiRoB, SHARE_ONLY_THE_NEED

	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_POPUP,(UINT_PTR)m_PrioMenu.m_hMenu, GetResString(IDS_PRIORITY) +" ("+GetResString(IDS_PW_CON_UPLBL)+")" );
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR);
	
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_DETAIL, GetResString(IDS_SHOWDETAILS));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_CMT, GetResString(IDS_CMT_ADD)); 
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
	
	// Mighty Knife: CRC32-Tag
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_CALCCRC32,"Calculate CRC32");
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_ADDCRC32TOFILENAME,"Add Release-Tag/CRC32 to filename...");
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_ABORTCRC32CALC,"Abort CRC32 calculation");
	// [end] Mighty Knife

	// Mighty Knife: Mass Rename
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_MASSRENAME,"Mass rename...");
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
	// [end] Mighty Knife

	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_GETED2KLINK, GetResString(IDS_DL_LINK1));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_GETHTMLED2KLINK, GetResString(IDS_DL_LINK2));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_GETSOURCEED2KLINK, GetResString(IDS_CREATESOURCELINK));
	m_SharedFilesMenu.AppendMenu(MF_STRING,MP_GETHOSTNAMESOURCEED2KLINK, GetResString(IDS_CREATEHOSTNAMESRCLINK));	// itsonlyme: hostnameSource
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
	
	m_SharedFilesMenu.AppendMenu(MF_STRING,Irc_SetSendLink,GetResString(IDS_IRC_ADDLINKTOIRC));
	m_SharedFilesMenu.AppendMenu(MF_STRING|MF_SEPARATOR); 
}

void CSharedFilesCtrl::ShowComments(CKnownFile* file)
{
	if (file){
    	CCommentDialog dialog(file); 
		dialog.DoModal(); 
	}
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
void CSharedFilesCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (nChar == 'C' && (GetKeyState(VK_CONTROL) & 0x8000))
	{
		// Ctrl+C: Copy listview items to clipboard
		SendMessage(WM_COMMAND, MP_GETED2KLINK);
		return;
	}
	CMuleListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}
